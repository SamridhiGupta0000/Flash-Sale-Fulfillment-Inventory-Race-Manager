-- Reset for the LARGE dataset run (400 orders). Demand is about 200 units per product.
-- Laptop stock is deliberately low (150) so some laptop orders FAIL near the end.
TRUNCATE payments, shipments, order_items, orders RESTART IDENTITY CASCADE;

UPDATE inventory SET stock_quantity = CASE p.sku
        WHEN 'RG-IP17' THEN 500
        WHEN 'RG-LP01' THEN 150
        WHEN 'RG-HP01' THEN 500
        ELSE 500 END
FROM products p
WHERE p.product_id = inventory.product_id;
