CREATE TABLE customers (
    customer_id BIGSERIAL PRIMARY KEY,
    full_name VARCHAR(100) NOT NULL,
    email VARCHAR(160) NOT NULL UNIQUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE products (
    product_id BIGSERIAL PRIMARY KEY,
    sku VARCHAR(40) NOT NULL UNIQUE,
    product_name VARCHAR(150) NOT NULL,
    price NUMERIC(12,2) NOT NULL CHECK (price >= 0),
    active BOOLEAN NOT NULL DEFAULT TRUE,
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE TABLE warehouses (
    warehouse_id BIGSERIAL PRIMARY KEY,
    warehouse_code VARCHAR(30) NOT NULL UNIQUE,
    warehouse_name VARCHAR(120) NOT NULL,
    location VARCHAR(160),
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE inventory (
    inventory_id BIGSERIAL PRIMARY KEY,
    product_id BIGINT NOT NULL REFERENCES products(product_id),
    warehouse_id BIGINT NOT NULL REFERENCES warehouses(warehouse_id),
    stock_quantity INTEGER NOT NULL CHECK (stock_quantity >= 0),
    reorder_level INTEGER NOT NULL DEFAULT 5 CHECK (reorder_level >= 0),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(product_id, warehouse_id)
);
CREATE TABLE orders (
    order_id BIGSERIAL PRIMARY KEY,
    customer_id BIGINT NOT NULL REFERENCES customers(customer_id),
    order_status VARCHAR(20) NOT NULL DEFAULT 'PENDING'
        CHECK (order_status IN
        ('PENDING','CONFIRMED','FAILED','CANCELLED')),
    total_amount NUMERIC(12,2) NOT NULL DEFAULT 0
        CHECK (total_amount >= 0),
    created_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE order_items (
    order_item_id BIGSERIAL PRIMARY KEY,
    order_id BIGINT NOT NULL
        REFERENCES orders(order_id) ON DELETE CASCADE,
    product_id BIGINT NOT NULL
        REFERENCES products(product_id),
    quantity INTEGER NOT NULL CHECK (quantity > 0),
    unit_price NUMERIC(12,2) NOT NULL CHECK (unit_price >= 0),
    UNIQUE(order_id, product_id)
);
CREATE TABLE payments (
    payment_id BIGSERIAL PRIMARY KEY,
    order_id BIGINT NOT NULL UNIQUE
        REFERENCES orders(order_id) ON DELETE CASCADE,
    amount NUMERIC(12,2) NOT NULL CHECK (amount >= 0),
    payment_status VARCHAR(20) NOT NULL DEFAULT 'PENDING'
        CHECK (payment_status IN
        ('PENDING','PAID','FAILED','REFUNDED')),
    paid_at TIMESTAMPTZ
);

CREATE TABLE shipments (
    shipment_id BIGSERIAL PRIMARY KEY,
    order_id BIGINT NOT NULL UNIQUE
        REFERENCES orders(order_id) ON DELETE CASCADE,
    warehouse_id BIGINT NOT NULL
        REFERENCES warehouses(warehouse_id),
    shipment_status VARCHAR(20) NOT NULL DEFAULT 'PENDING'
        CHECK (shipment_status IN
        ('PENDING','PACKED','SHIPPED','DELIVERED','CANCELLED')),
    tracking_number VARCHAR(80) UNIQUE,
    shipped_at TIMESTAMPTZ
);
CREATE INDEX idx_inventory_product
ON inventory(product_id);

CREATE INDEX idx_inventory_warehouse
ON inventory(warehouse_id);

CREATE INDEX idx_orders_customer
ON orders(customer_id);

CREATE INDEX idx_order_items_order
ON order_items(order_id);

CREATE INDEX idx_order_items_product
ON order_items(product_id);

CREATE INDEX idx_payments_order
ON payments(order_id);

CREATE INDEX idx_shipments_order
ON shipments(order_id);

CREATE INDEX idx_shipments_warehouse
ON shipments(warehouse_id);

CREATE INDEX idx_orders_status_created
ON orders(order_status, created_at DESC);
-- viewsss
CREATE VIEW low_stock_products AS
SELECT
    i.inventory_id,
    p.product_id,
    p.sku,
    p.product_name,
    w.warehouse_code,
    i.stock_quantity,
    i.reorder_level,
    i.updated_at
FROM inventory i
JOIN products p
    ON p.product_id = i.product_id
JOIN warehouses w
    ON w.warehouse_id = i.warehouse_id
WHERE i.stock_quantity <= i.reorder_level;
-- triggerssss
CREATE OR REPLACE FUNCTION set_inventory_updated_at()
RETURNS TRIGGER
LANGUAGE plpgsql
AS $$
BEGIN
    NEW.updated_at := CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$;

CREATE TRIGGER trg_inventory_updated_at
BEFORE UPDATE ON inventory
FOR EACH ROW
EXECUTE FUNCTION set_inventory_updated_at();
-- triggerssss
CREATE OR REPLACE FUNCTION set_order_updated_at()
RETURNS TRIGGER
LANGUAGE plpgsql
AS $$
BEGIN
    NEW.updated_at := CURRENT_TIMESTAMP;
    RETURN NEW;
END;
$$;

CREATE TRIGGER trg_order_updated_at
BEFORE UPDATE ON orders
FOR EACH ROW
EXECUTE FUNCTION set_order_updated_at();
-- PRE DEFINEDEDDDD DATAAAAA
INSERT INTO customers(full_name,email) VALUES
('Aarav Sharma','aarav@example.com'),
('Diya Verma','diya@example.com'),
('Kabir Singh','kabir@example.com'),
('Ananya Jain','ananya@example.com'),
('Rohan Mehta','rohan@example.com');

INSERT INTO products(sku,product_name,price) VALUES
('RG-IP17','iPhone 17',79999.00),
('RG-LP01','Gaming Laptop',129999.00),
('RG-HP01','Wireless Headphones',4999.00),
('RG-SW01','Smart Watch',8999.00);

INSERT INTO warehouses
(warehouse_code,warehouse_name,location)
VALUES
('WH-DEL','Delhi Fulfillment Center','Delhi'),
('WH-DEH','Dehradun Fulfillment Center','Dehradun');

INSERT INTO inventory
(product_id,warehouse_id,stock_quantity,reorder_level)
SELECT
    p.product_id,
    w.warehouse_id,
    CASE p.sku
        WHEN 'RG-IP17' THEN 10
        WHEN 'RG-LP01' THEN 5
        WHEN 'RG-HP01' THEN 25
        ELSE 12
    END,
    CASE p.sku
        WHEN 'RG-IP17' THEN 3
        WHEN 'RG-LP01' THEN 2
        WHEN 'RG-HP01' THEN 5
        ELSE 3
    END
FROM products p
CROSS JOIN warehouses w
WHERE w.warehouse_code = 'WH-DEL';