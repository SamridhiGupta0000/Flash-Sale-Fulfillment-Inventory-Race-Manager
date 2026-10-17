
TRUNCATE payments, shipments, order_items, orders RESTART IDENTITY CASCADE;

UPDATE inventory SET stock_quantity = CASE p.sku
        WHEN 'RG-IP17' THEN 500
        WHEN 'RG-LP01' THEN 150
        WHEN 'RG-HP01' THEN 500
        ELSE 500 END
FROM products p
WHERE p.product_id = inventory.product_id;