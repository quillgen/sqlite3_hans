
# A sqlite3/sqlcipher fts5 chinese tokenizer extension.

A sqlite3 fts5 chiniese tokenizer extension using [CppJieba](https://github.com/yanyiwu/cppjieba).

```bash
./sqlcipher
SQLite version 3.49.2 2025-05-07 10:39:52 (SQLCipher 4.9.0 community)
Enter ".help" for usage hints.
Connected to a transient in-memory database.
Use ".open FILENAME" to reopen on a persistent database.
sqlite> .mode column
sqlite> .headers on
sqlite> .width 50
sqlite> CREATE VIRTUAL TABLE docs USING fts5(content, tokenize='fts5_hans');
sqlite> INSERT INTO docs VALUES ('中国是一个伟大的国家');
sqlite> SELECT * FROM docs WHERE docs MATCH '中国';
content
--------------------------------------------------
中国是一个伟大的国家
```