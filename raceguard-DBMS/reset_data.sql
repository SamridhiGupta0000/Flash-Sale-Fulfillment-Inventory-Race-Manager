-- Run before each demo to start clean (keeps customers/products/warehouses).
TRUNCATE payments, shipments, order_items, orders RESTART IDENTITY CASCADE;

UPDATE inventory SET stock_quantity = CASE p.sku
        WHEN 'RG-IP17' THEN 10
        WHEN 'RG-LP01' THEN 5
        WHEN 'RG-HP01' THEN 25
        ELSE 12 END
FROM products p
WHERE p.product_id = inventory.product_id;
