grep -P '^[A-Da-d]' ./enwiki-20250401-all-titles-in-ns0 | grep -P '^[\x00-\x7F]+$' > title.txt
