
            //PRE DEFINEDEDDDD DATAAAAA//


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

                    //TRANSACTIONSSSS//

BEGIN;

SET TRANSACTION ISOLATION LEVEL READ COMMITTED;

SELECT
    inventory_id,
    stock_quantity
FROM inventory
WHERE product_id = 1
AND warehouse_id = 1
FOR UPDATE;