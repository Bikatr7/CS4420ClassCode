TEE assignment1_output.txt;

DROP TABLE IF EXISTS hot_sauces;

CREATE TABLE hot_sauces (
    sauce_id       INT AUTO_INCREMENT PRIMARY KEY,
    name           VARCHAR(100) NOT NULL,
    brand          VARCHAR(75) NOT NULL,
    scoville_rating INT,
    date_added     DATE NOT NULL,
    origin_country VARCHAR(50),
    volume_oz      DECIMAL(5,2)
);

DESC hot_sauces;

INSERT INTO hot_sauces (name, brand, scoville_rating, date_added, origin_country, volume_oz)
VALUES ('Original Red Sauce', 'Franks RedHot', 450, '2024-03-15', 'United States', 5.00);

INSERT INTO hot_sauces (name, brand, scoville_rating, date_added, origin_country, volume_oz)
VALUES ('Sriracha', 'Huy Fong', 2200, '2023-11-01', 'United States', 17.00);

INSERT INTO hot_sauces (name, brand, scoville_rating, date_added, origin_country, volume_oz)
VALUES ('Habanero', 'El Yucateco', 8910, '2024-07-22', 'Mexico', 4.00);

INSERT INTO hot_sauces (name, brand, scoville_rating, date_added, origin_country, volume_oz)
VALUES ('Carolina Reaper Sauce', 'PuckerButt', 2200000, '2025-01-10', 'United States', 5.00);

INSERT INTO hot_sauces (name, brand, scoville_rating, date_added, origin_country, volume_oz)
VALUES ('Scotch Bonnet Pepper Sauce', 'Grace', 225000, '2024-09-05', 'Jamaica', 4.80);

SELECT * FROM hot_sauces;

SELECT name, brand, scoville_rating
FROM hot_sauces
WHERE scoville_rating > 5000;

NOTEE;
