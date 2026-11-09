-- ===== RESULTS: run after ./pbl_db =====
SELECT order_id, customer_id, order_status, total_amount FROM orders ORDER BY order_id;

SELECT p.product_name, i.stock_quantity, i.reorder_level
FROM inventory i JOIN products p USING (product_id)
WHERE i.warehouse_id = 1 ORDER BY p.product_id;

SELECT * FROM low_stock_products;

SELECT o.order_id, o.order_status, pay.payment_status, s.shipment_status
FROM orders o
LEFT JOIN payments pay ON pay.order_id = o.order_id
LEFT JOIN shipments s  ON s.order_id  = o.order_id
ORDER BY o.order_id;


-- ===== TRIGGER DEMO =====
SELECT updated_at FROM inventory WHERE inventory_id = 1;
UPDATE inventory SET reorder_level = reorder_level WHERE inventory_id = 1;
SELECT updated_at FROM inventory WHERE inventory_id = 1;   -- changed automatically
