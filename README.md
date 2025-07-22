
# A sqlite3/sqlcipher fts5 chinese tokenizer extension.

```bash
Use ".open FILENAME" to reopen on a persistent database.
sqlite> .mode column
sqlite> .headers on
sqlite> CREATE VIRTUAL TABLE articles USING fts5(title, body, tokenize='mytokenizer');
sqlite> INSERT INTO articles VALUES('SQLite FTS5', 'Testing custom tokenizer implementation');
sqlite> INSERT INTO articles VALUES('Database Systems', 'SQLite is a powerful embedded database');
sqlite> SELECT * FROM articles WHERE articles MATCH 'custom';
title        body
-----------  ---------------------------------------
SQLite FTS5  Testing custom tokenizer implementation
sqlite> SELECT * FROM articles WHERE articles MATCH 'powerful';
title             body
----------------  --------------------------------------
Database Systems  SQLite is a powerful embedded database
sqlite> SELECT * FROM articles WHERE articles MATCH 'implementation';
title        body
-----------  ---------------------------------------
SQLite FTS5  Testing custom tokenizer implementation
```