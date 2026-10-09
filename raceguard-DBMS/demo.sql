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

-- ===== LOCKING DEMO (use TWO psql windows) =====
-- Window 1:
--   BEGIN;
--   SELECT stock_quantity FROM inventory WHERE product_id=1 AND warehouse_id=1 FOR UPDATE;
--   (leave it open)
-- Window 2:
--   BEGIN;
--   SELECT stock_quantity FROM inventory WHERE product_id=1 AND warehouse_id=1 FOR UPDATE;
--   (it WAITS - blocked)
-- Window 1:
--   UPDATE inventory SET stock_quantity = stock_quantity - 2 WHERE product_id=1 AND warehouse_id=1;
--   COMMIT;
-- Window 2 now unblocks and sees the reduced stock. Then: COMMIT;

-- ===== TRIGGER DEMO =====
SELECT updated_at FROM inventory WHERE inventory_id = 1;
UPDATE inventory SET reorder_level = reorder_level WHERE inventory_id = 1;
SELECT updated_at FROM inventory WHERE inventory_id = 1;   -- changed automatically
