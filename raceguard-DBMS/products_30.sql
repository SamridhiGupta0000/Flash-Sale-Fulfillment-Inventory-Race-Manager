-- Grows the catalogue from 4 to 30 products, with stock in BOTH warehouses. Safe to run more than once.
ALTER TABLE products ADD COLUMN IF NOT EXISTS category VARCHAR(40);

INSERT INTO products(sku, product_name, price, category) VALUES
('RG-PH02','Samsung Galaxy S25',69999,'Phones'),('RG-PH03','Google Pixel 10',59999,'Phones'),
('RG-PH04','OnePlus 13',64999,'Phones'),
('RG-LP02','MacBook Air M4',114999,'Laptops'),('RG-LP03','Dell XPS 15',149999,'Laptops'),
('RG-LP04','HP Pavilion 14',62999,'Laptops'),('RG-LP05','Lenovo ThinkPad E14',71999,'Laptops'),
('RG-HP02','AirPods Pro',24999,'Audio'),('RG-HP03','Sony WH-1000XM6',29999,'Audio'),
('RG-HP04','JBL Flip 7 Speaker',9999,'Audio'),('RG-HP05','boAt Rockerz 551',1999,'Audio'),
('RG-SW02','Apple Watch Series 11',44999,'Wearables'),('RG-SW03','Fitbit Charge 7',14999,'Wearables'),
('RG-SW04','Galaxy Watch 8',27999,'Wearables'),
('RG-TB01','iPad Air',59999,'Tablets'),('RG-TB02','Galaxy Tab S10',74999,'Tablets'),
('RG-TB03','Lenovo Tab P12',24999,'Tablets'),
('RG-AC01','Wireless Mouse',999,'Accessories'),('RG-AC02','Mechanical Keyboard',4499,'Accessories'),
('RG-AC03','USB-C Hub',2499,'Accessories'),('RG-AC04','Power Bank 20000mAh',1999,'Accessories'),
('RG-AC05','65W Fast Charger',1799,'Accessories'),('RG-AC06','Laptop Backpack',2999,'Accessories'),
('RG-GM01','27in 4K Monitor',32999,'Gaming'),('RG-GM02','PlayStation 5',49999,'Gaming'),
('RG-GM03','Gaming Controller',5999,'Gaming')
ON CONFLICT (sku) DO NOTHING;

UPDATE products SET category = CASE sku WHEN 'RG-IP17' THEN 'Phones' WHEN 'RG-LP01' THEN 'Laptops'
       WHEN 'RG-HP01' THEN 'Audio' WHEN 'RG-SW01' THEN 'Wearables' ELSE category END
WHERE category IS NULL;

INSERT INTO inventory(product_id, warehouse_id, stock_quantity, reorder_level)
SELECT p.product_id, w.warehouse_id, CASE w.warehouse_code WHEN 'WH-DEL' THEN 40 ELSE 25 END, 5
FROM products p CROSS JOIN warehouses w
ON CONFLICT (product_id, warehouse_id) DO NOTHING;

SELECT COUNT(*) AS total_products FROM products;
