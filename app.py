# Web UI: login/sign-up (admin + customers), place orders, admin dashboard, multi-warehouse processing
# Run: python3 app.py   then open http://localhost:5000
import os, subprocess, psycopg2, psycopg2.extras
from functools import wraps
from flask import Flask, request, session, redirect, jsonify, render_template_string as T

app = Flask(__name__)
app.secret_key = os.environ.get("SECRET", "pbl-dev-secret")
DSN = os.environ.get("PBL_DSN", "dbname=pbl user=postgres password=pbl123 host=localhost")

def q(sql, args=(), one=False):
    c = psycopg2.connect(DSN, cursor_factory=psycopg2.extras.RealDictCursor)
    try:
        with c, c.cursor() as cur:
            cur.execute(sql, args)
            if not cur.description: return None
            return cur.fetchone() if one else cur.fetchall()
    finally:
        c.close()

CSS = """<style>
:root{--bg:#0b1020;--card:#141b34;--line:#232c4d;--text:#e6e9f5;--mut:#8b94b8;--acc:#6c8cff;--acc2:#a06bff}
*{box-sizing:border-box}
body{margin:0;font-family:Inter,system-ui,-apple-system,Segoe UI,sans-serif;color:var(--text);min-height:100vh;
background:radial-gradient(900px 500px at 10% -10%,#1d2a5c55,transparent),radial-gradient(800px 500px at 100% 0,#4b227a44,transparent),var(--bg)}
nav{position:sticky;top:0;z-index:5;display:flex;justify-content:space-between;align-items:center;padding:14px 32px;
background:#0b1020cc;backdrop-filter:blur(10px);border-bottom:1px solid var(--line)}
nav b{font-size:18px;background:linear-gradient(90deg,var(--acc),var(--acc2));-webkit-background-clip:text;background-clip:text;color:transparent}
nav span{color:var(--mut);font-size:14px}nav a{margin-left:14px;padding:6px 14px;border-radius:99px;border:1px solid var(--line);color:var(--text);text-decoration:none}
nav a:hover{border-color:var(--acc)}
main{max-width:1100px;margin:28px auto;padding:0 18px}
.card{background:linear-gradient(180deg,#171f3d,var(--card));border:1px solid var(--line);border-radius:18px;padding:22px;margin-bottom:20px;box-shadow:0 10px 30px #0006}
.card h2,.card h3{margin-top:0}h3{color:#cfd6f5;font-weight:600}small{color:var(--mut)}a{color:var(--acc)}
input,select{width:auto;padding:11px 14px;border:1px solid var(--line);border-radius:10px;background:#0e142b;color:var(--text);font-size:15px;margin:5px 4px 5px 0;outline:none}
input:focus,select:focus{border-color:var(--acc);box-shadow:0 0 0 3px #6c8cff33}
button{padding:11px 18px;border:0;border-radius:10px;font-size:15px;font-weight:600;color:#fff;cursor:pointer;margin:5px 4px 5px 0;
background:linear-gradient(90deg,var(--acc),var(--acc2));transition:transform .12s,box-shadow .12s}
button:hover{transform:translateY(-1px);box-shadow:0 6px 18px #6c8cff55}
table{width:100%;border-collapse:collapse}th{color:var(--mut);font-size:12px;text-transform:uppercase;letter-spacing:.06em}
th,td{text-align:left;padding:10px 8px;border-bottom:1px solid var(--line)}tbody tr:hover,tr:hover td{background:#ffffff08}
.tag{display:inline-block;padding:3px 12px;border-radius:99px;font-size:12px;font-weight:600;letter-spacing:.03em}
.CONFIRMED{background:#2b8a3e33;color:#69db7c}.FAILED{background:#c92a2a33;color:#ff8787}
.PENDING{background:#f0a60033;color:#ffd43b}.CANCELLED{background:#868e9633;color:#ced4da}
.err{color:#ff8787}.ok{color:#69db7c}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:16px}.grid .card{margin:0}
.stat{font-size:34px;font-weight:800;color:#fff;letter-spacing:-.02em}.stat.err{color:#ff8787}
.row2{display:grid;grid-template-columns:1fr 1fr;gap:20px}@media(max-width:800px){.row2{grid-template-columns:1fr}nav{padding:12px 16px}}
pre{background:#070b17;color:#7ee2a8;padding:14px;border-radius:12px;border:1px solid var(--line);height:210px;overflow:auto;font-size:12px}
.sm{padding:4px 12px;font-size:13px}
</style>"""
LAYOUT = CSS + """<nav><b>📦 Order Processing System</b><span>{% if session.role %}{{session.name}} · {{session.role}}
<a href="/logout">Logout</a>{% endif %}</span></nav><main>{{body|safe}}</main>"""

def page(tpl, **kw):
    return T(LAYOUT, body=T(tpl, **kw))

def need(role):
    def deco(f):
        @wraps(f)
        def w(*a, **k):
            if session.get("role") != role: return redirect("/login")
            return f(*a, **k)
        return w
    return deco

@app.route("/")
def home():
    return redirect({"admin": "/admin", "customer": "/customer"}.get(session.get("role"), "/login"))

@app.route("/login", methods=["GET", "POST"])
def login():
    msg = ""
    if request.method == "POST":
        ident, pw = request.form["ident"].strip(), request.form["password"]
        u = q("""SELECT u.role, u.customer_id, COALESCE(c.full_name,'Admin') AS name FROM users u
                 LEFT JOIN customers c ON c.customer_id = u.customer_id
                 WHERE (u.username = %s OR u.customer_id::text = %s)
                   AND u.password_hash = crypt(%s, u.password_hash)""", (ident, ident, pw), one=True)
        if u:
            session.update(role=u["role"], cid=u["customer_id"], name=u["name"])
            return redirect("/")
        msg = "Wrong ID/username or password."
    return page("""<div class="card" style="max-width:380px;margin:40px auto"><h2>Login</h2>
      <form method="post"><input name="ident" placeholder="Customer ID, email or admin" required style="width:100%">
      <input name="password" type="password" placeholder="Password" required style="width:100%">
      <button style="width:100%">Login</button></form><p class="err">{{msg}}</p>
      <p>New customer? <a href="/signup">Sign up</a></p></div>""", msg=msg)

@app.route("/signup", methods=["GET", "POST"])
def signup():
    msg = ""
    if request.method == "POST":
        name, email, pw = request.form["name"].strip(), request.form["email"].strip().lower(), request.form["password"]
        if len(pw) < 6: msg = "Password must be at least 6 characters."
        elif q("SELECT 1 FROM customers WHERE email=%s", (email,), one=True): msg = "Email already registered."
        else:
            c = q("INSERT INTO customers(full_name,email) VALUES (%s,%s) RETURNING customer_id", (name, email), one=True)
            q("INSERT INTO users(username,customer_id,role,password_hash) VALUES (%s,%s,'customer',crypt(%s,gen_salt('bf')))",
              (email, c["customer_id"], pw))
            session.update(role="customer", cid=c["customer_id"], name=name)
            return redirect("/customer")
    return page("""<div class="card" style="max-width:380px;margin:40px auto"><h2>Sign up</h2>
      <form method="post"><input name="name" placeholder="Full name" required style="width:100%">
      <input name="email" type="email" placeholder="Email" required style="width:100%">
      <input name="password" type="password" placeholder="Password (6+ chars)" required style="width:100%">
      <button style="width:100%">Create account</button></form><p class="err">{{msg}}</p>
      <a href="/login">Back to login</a></div>""", msg=msg)

@app.route("/logout")
def logout():
    session.clear(); return redirect("/login")

@app.route("/customer")
@need("customer")
def customer():
    prods = q("""SELECT p.product_id, p.product_name, p.price, COALESCE(p.category,'Other') AS category, COALESCE(SUM(i.stock_quantity),0) AS stock
                 FROM products p LEFT JOIN inventory i USING (product_id) WHERE p.active GROUP BY p.product_id ORDER BY 4, p.product_id""")
    orders = q("""SELECT o.order_id, p.product_name, oi.quantity, o.total_amount, o.order_status, s.shipment_status, w.warehouse_code
                  FROM orders o JOIN order_items oi USING (order_id) JOIN products p ON p.product_id=oi.product_id
                  LEFT JOIN shipments s ON s.order_id=o.order_id LEFT JOIN warehouses w ON w.warehouse_id=s.warehouse_id
                  WHERE o.customer_id=%s ORDER BY o.order_id DESC""", (session["cid"],))
    return page("""<div class="card"><h2>Welcome, {{session.name}} <small>(Customer ID {{session.cid}})</small></h2>
      <form method="post" action="/order"><select name="product_id">{% for cat, items in prods|groupby('category') %}<optgroup label="{{cat}}">{% for p in items %}
      <option value="{{p.product_id}}">{{p.product_name}} - ₹{{p.price}} ({{p.stock}} in stock)</option>{% endfor %}</optgroup>{% endfor %}</select>
      <input name="quantity" type="number" min="1" value="1" style="width:90px"><button>Place order</button></form></div>
      <div class="card"><h3>My orders</h3><table><tr><th>#</th><th>Product</th><th>Qty</th><th>Total</th><th>Status</th><th>Shipment</th><th>From</th><th></th></tr>
      {% for o in orders %}<tr><td>{{o.order_id}}</td><td>{{o.product_name}}</td><td>{{o.quantity}}</td><td>₹{{o.total_amount}}</td>
      <td><span class="tag {{o.order_status}}">{{o.order_status}}</span></td><td>{{o.shipment_status or '-'}}</td><td>{{o.warehouse_code or '-'}}</td><td>{% if o.order_status=='PENDING' %}<form method="post" action="/cancel/{{o.order_id}}"><button class="sm">Cancel</button></form>{% endif %}</td></tr>
      {% else %}<tr><td colspan="8">No orders yet.</td></tr>{% endfor %}</table></div>""", prods=prods, orders=orders)

@app.route("/order", methods=["POST"])
@need("customer")
def order():
    pid, qty = int(request.form["product_id"]), max(1, int(request.form["quantity"]))
    c = psycopg2.connect(DSN, cursor_factory=psycopg2.extras.RealDictCursor)
    try:
        with c, c.cursor() as cur:
            cur.execute("INSERT INTO orders(customer_id,total_amount) SELECT %s, price*%s FROM products WHERE product_id=%s RETURNING order_id",
                        (session["cid"], qty, pid))
            oid = cur.fetchone()["order_id"]
            cur.execute("INSERT INTO order_items(order_id,product_id,quantity,unit_price) SELECT %s,product_id,%s,price FROM products WHERE product_id=%s",
                        (oid, qty, pid))
    finally:
        c.close()
    return redirect("/customer")

def process(oid):
    """Same locking transaction as the C++ worker, but the warehouse with the most stock is chosen."""
    c = psycopg2.connect(DSN, cursor_factory=psycopg2.extras.RealDictCursor)
    try:
        with c, c.cursor() as cur:
            cur.execute("SELECT product_id, quantity FROM order_items WHERE order_id=%s", (oid,))
            it = cur.fetchone()
            cur.execute("""SELECT warehouse_id FROM inventory WHERE product_id=%s AND stock_quantity>=%s
                           ORDER BY stock_quantity DESC LIMIT 1 FOR UPDATE""", (it["product_id"], it["quantity"]))
            w = cur.fetchone()
            if w:
                cur.execute("UPDATE inventory SET stock_quantity=stock_quantity-%s WHERE product_id=%s AND warehouse_id=%s",
                            (it["quantity"], it["product_id"], w["warehouse_id"]))
                cur.execute("UPDATE orders SET order_status='CONFIRMED' WHERE order_id=%s", (oid,))
                cur.execute("INSERT INTO payments(order_id,amount,payment_status,paid_at) SELECT order_id,total_amount,'PAID',CURRENT_TIMESTAMP FROM orders WHERE order_id=%s", (oid,))
                cur.execute("INSERT INTO shipments(order_id,warehouse_id,shipment_status) VALUES (%s,%s,'PACKED')", (oid, w["warehouse_id"]))
            else:
                cur.execute("UPDATE orders SET order_status='FAILED' WHERE order_id=%s", (oid,))
    finally:
        c.close()

@app.route("/admin/process", methods=["POST"])
@need("admin")
def admin_process():
    for r in q("SELECT order_id FROM orders WHERE order_status='PENDING' ORDER BY order_id"):
        process(r["order_id"])
    return redirect("/admin")

SIM_LOG, sim = "/tmp/pbl_sim.log", {"p": None}

@app.route("/cancel/<int:oid>", methods=["POST"])
@need("customer")
def cancel(oid):
    q("UPDATE orders SET order_status='CANCELLED' WHERE order_id=%s AND customer_id=%s AND order_status='PENDING'", (oid, session["cid"]))
    return redirect("/customer")

@app.route("/api/stats")
@need("admin")
def api_stats():
    s = q("SELECT order_status, COUNT(*)::int AS n FROM orders GROUP BY order_status")
    st = q("""SELECT p.product_name AS p, w.warehouse_code AS w, i.stock_quantity AS n FROM inventory i
              JOIN products p USING (product_id) JOIN warehouses w USING (warehouse_id)
              WHERE i.product_id IN (SELECT product_id FROM inventory GROUP BY product_id ORDER BY SUM(stock_quantity) LIMIT 8)
              ORDER BY p.product_id, w.warehouse_code""")
    k = q("""SELECT COALESCE(SUM(total_amount) FILTER (WHERE order_status='CONFIRMED'),0)::float AS revenue,
             COUNT(*)::int AS orders, (SELECT COUNT(*) FROM customers)::int AS customers,
             (SELECT COUNT(*) FROM low_stock_products)::int AS low FROM orders""", one=True)
    o = q("""SELECT o.order_id AS id, c.full_name AS name, o.total_amount::float AS total, o.order_status AS status, COALESCE(w.warehouse_code,'-') AS wh
             FROM orders o JOIN customers c USING (customer_id) LEFT JOIN shipments s ON s.order_id=o.order_id
             LEFT JOIN warehouses w ON w.warehouse_id=s.warehouse_id ORDER BY o.order_id DESC LIMIT 12""")
    return jsonify(status={r["order_status"]: r["n"] for r in s}, stock=[dict(r) for r in st], kpi=dict(k), orders=[dict(r) for r in o])

@app.route("/api/log")
@need("admin")
def api_log():
    try: lines = open(SIM_LOG).read().splitlines()[-40:]
    except OSError: lines = []
    return jsonify(lines=lines, running=bool(sim["p"] and sim["p"].poll() is None))

@app.route("/admin/sim", methods=["POST"])
@need("admin")
def admin_sim():
    pol = request.form.get("policy", "fcfs")
    if pol not in ("fcfs", "sjf", "priority", "rr"): pol = "fcfs"
    if not (sim["p"] and sim["p"].poll() is None):
        exe = os.path.join(os.path.dirname(os.path.abspath(__file__)), "pbl_sched")
        try:
            sim["p"] = subprocess.Popen(["stdbuf", "-oL", exe, DSN, pol, "4", "50", "2"], stdout=open(SIM_LOG, "w"), stderr=subprocess.STDOUT)
        except Exception as e:
            open(SIM_LOG, "w").write("COULD NOT START ENGINE: %s\nCompile it in this folder: g++ -std=c++17 -pthread -Wno-deprecated-declarations main_db_sched.cpp -o pbl_sched -lpqxx -lpq\n" % e)
    return redirect("/admin")

@app.route("/admin/reset", methods=["POST"])
@need("admin")
def admin_reset():
    q("""TRUNCATE payments, shipments, order_items, orders RESTART IDENTITY CASCADE;
         UPDATE inventory SET stock_quantity = CASE p.sku WHEN 'RG-LP01' THEN 150 ELSE 500 END
         FROM products p WHERE p.product_id = inventory.product_id""")
    return redirect("/admin")

@app.route("/admin/restock", methods=["POST"])
@need("admin")
def admin_restock():
    q("UPDATE inventory SET stock_quantity = stock_quantity + 25 WHERE inventory_id=%s", (int(request.form["inv"]),))
    return redirect("/admin")

@app.route("/admin")
@need("admin")
def admin():
    inv = q("""SELECT i.inventory_id, p.product_name, w.warehouse_code, i.stock_quantity, i.reorder_level FROM inventory i
               JOIN products p USING (product_id) JOIN warehouses w USING (warehouse_id) ORDER BY p.product_id, w.warehouse_code""")
    top = q("""SELECT c.customer_id, c.full_name, COUNT(o.order_id) AS orders,
               COALESCE(SUM(o.total_amount) FILTER (WHERE o.order_status='CONFIRMED'),0) AS spent
               FROM customers c LEFT JOIN orders o USING (customer_id) GROUP BY c.customer_id ORDER BY spent DESC LIMIT 6""")
    return page("""<div class="grid" style="margin-bottom:20px">
      <div class="card"><div class="stat" id="k_rev">-</div>Revenue (confirmed)</div>
      <div class="card"><div class="stat" id="k_ord">-</div>Total orders</div>
      <div class="card"><div class="stat" id="k_cus">-</div>Customers</div>
      <div class="card"><div class="stat err" id="k_low">-</div>Low-stock items</div></div>
      <div class="row2"><div class="card"><h3>Orders by status (live)</h3><canvas id="c1" height="170"></canvas></div>
      <div class="card"><h3>Lowest-stock products (live)</h3><canvas id="c2" height="170"></canvas></div></div>
      <div class="card"><h3>Scheduler control (runs the C++ multithreaded engine)</h3>
      <form method="post" action="/admin/sim" style="display:inline"><select name="policy"><option value="fcfs">FCFS</option>
      <option value="sjf">SJF</option><option value="priority">Priority</option><option value="rr">Round Robin</option></select>
      <button>▶ Run 200 orders</button></form>
      <form method="post" action="/admin/process" style="display:inline"><button>Process pending (web)</button></form>
      <form method="post" action="/admin/reset" style="display:inline"><button style="background:#c92a2a">Reset demo data</button></form>
      <p id="runs"></p><pre id="log">No simulation run yet.</pre></div>
      <div class="row2"><div class="card"><h3>Stock &amp; restock</h3><div style="max-height:330px;overflow:auto"><table><tr><th>Product</th><th>WH</th><th>Stock</th><th></th></tr>
      {% for s in inv %}<tr><td>{{s.product_name}}</td><td>{{s.warehouse_code}}</td>
      <td {% if s.stock_quantity <= s.reorder_level %}class="err"{% endif %}>{{s.stock_quantity}}</td>
      <td><form method="post" action="/admin/restock"><input type="hidden" name="inv" value="{{s.inventory_id}}"><button class="sm">+25</button></form></td></tr>{% endfor %}</table></div></div>
      <div class="card"><h3>Top customers</h3><table><tr><th>ID</th><th>Name</th><th>Orders</th><th>Spent</th></tr>
      {% for c in top %}<tr><td>{{c.customer_id}}</td><td>{{c.full_name}}</td><td>{{c.orders}}</td><td>₹{{c.spent}}</td></tr>{% endfor %}</table></div></div>
      <div class="card"><h3>Latest orders (live)</h3><table><thead><tr><th>#</th><th>Customer</th><th>Total</th><th>Status</th><th>Warehouse</th></tr></thead><tbody id="ord"></tbody></table></div>
      <script src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.4.1/chart.umd.min.js"></script>
      {% raw %}<script>
      const COL={CONFIRMED:'#40c057',FAILED:'#fa5252',PENDING:'#fcc419',CANCELLED:'#868e96'};
      let c1=null,c2=null;
      try{
        Chart.defaults.color='#aeb7d8';Chart.defaults.borderColor='#232c4d';
        c1=new Chart(document.getElementById('c1'),{type:'doughnut',data:{labels:[],datasets:[{data:[],backgroundColor:[]}]}});
        c2=new Chart(document.getElementById('c2'),{type:'bar',data:{labels:[],datasets:[]}});
      }catch(e){runs.textContent='Charts not loaded (Chart.js needs internet) - the rest still works';}
      async function tick(){
        try{
          const d=await (await fetch('/api/stats')).json();
          k_rev.textContent='₹'+Math.round(d.kpi.revenue).toLocaleString('en-IN');k_ord.textContent=d.kpi.orders;
          k_cus.textContent=d.kpi.customers;k_low.textContent=d.kpi.low;
          if(c1){
            const ks=Object.keys(d.status);c1.data.labels=ks;c1.data.datasets[0].data=ks.map(k=>d.status[k]);
            c1.data.datasets[0].backgroundColor=ks.map(k=>COL[k]);c1.update();
            const ps=[...new Set(d.stock.map(r=>r.p))],ws=[...new Set(d.stock.map(r=>r.w))];
            c2.data.labels=ps;c2.data.datasets=ws.map((w,i)=>({label:w,backgroundColor:['#6c8cff','#a06bff'][i%2],
              data:ps.map(p=>(d.stock.find(r=>r.p==p&&r.w==w)||{n:0}).n)}));c2.update();
          }
          ord.innerHTML=d.orders.map(o=>`<tr><td>${o.id}</td><td>${o.name}</td><td>₹${o.total}</td><td><span class="tag ${o.status}">${o.status}</span></td><td>${o.wh}</td></tr>`).join('');
        }catch(e){runs.textContent='Stats error: '+e;}
        try{
          const l=await (await fetch('/api/log')).json();
          if(l.lines.length){log.textContent=l.lines.join('\\n');log.scrollTop=log.scrollHeight;}
          if(l.running)runs.textContent='⏳ Engine running...';
        }catch(e){}
      }
      tick();setInterval(tick,1500);</script>{% endraw %}""", inv=inv, top=top)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)
