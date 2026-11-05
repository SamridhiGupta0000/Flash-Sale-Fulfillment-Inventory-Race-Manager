-- Inserts 800 PENDING orders (each with one order item), all inside ONE transaction.
-- Run after reset_big.sql:  psql ... -f insert_800.sql
-- Customers cycle 1..5, products cycle 1..4, quantity cycles 1..3 (same idea as the C++ code).

BEGIN;

DO $$
DECLARE
    new_order_id BIGINT;
    pid   INT;
    qty   INT;
    cid   INT;
    unit  NUMERIC(12,2);
BEGIN
    FOR g IN 1..800 LOOP
        cid := 1 + (g % 5);
        pid := 1 + (g % 4);
        qty := 1 + (g % 3);

        SELECT price INTO unit FROM products WHERE product_id = pid;

        INSERT INTO orders(customer_id, total_amount)
        VALUES (cid, unit * qty)
        RETURNING order_id INTO new_order_id;

        INSERT INTO order_items(order_id, product_id, quantity, unit_price)
        VALUES (new_order_id, pid, qty, unit);
    END LOOP;
END $$;

COMMIT;

-- Proof of insertion
SELECT COUNT(*) AS total_orders FROM orders;
SELECT order_status, COUNT(*) FROM orders GROUP BY order_status;
SELECT COUNT(*) AS total_order_items FROM order_items;
SELECT * FROM orders ORDER BY order_id LIMIT 5;
