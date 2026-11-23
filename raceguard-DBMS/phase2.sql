-- Phase 2: login accounts + stock in the second warehouse (run once, after schema.sql)
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE IF NOT EXISTS users (
    user_id BIGSERIAL PRIMARY KEY,
    username VARCHAR(160) NOT NULL UNIQUE,            -- 'admin' or the customer's email
    customer_id BIGINT UNIQUE REFERENCES customers(customer_id),
    role VARCHAR(10) NOT NULL CHECK (role IN ('admin','customer')),
    password_hash TEXT NOT NULL                       -- bcrypt hash, never plain text
);

INSERT INTO users(username, role, password_hash)
VALUES ('admin', 'admin', crypt('admin123', gen_salt('bf'))) ON CONFLICT DO NOTHING;

INSERT INTO users(username, customer_id, role, password_hash)
SELECT email, customer_id, 'customer', crypt('pass123', gen_salt('bf'))
FROM customers ON CONFLICT DO NOTHING;

-- Dehradun warehouse gets its own stock (multi-warehouse)
INSERT INTO inventory(product_id, warehouse_id, stock_quantity, reorder_level)
SELECT p.product_id, w.warehouse_id,
       CASE p.sku WHEN 'RG-IP17' THEN 6 WHEN 'RG-LP01' THEN 3 WHEN 'RG-HP01' THEN 15 ELSE 8 END, 2
FROM products p CROSS JOIN warehouses w
WHERE w.warehouse_code = 'WH-DEH' ON CONFLICT DO NOTHING;
