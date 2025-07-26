
# A sqlite3/sqlcipher fts5 chinese tokenizer extension.

A sqlite3 fts5 chiniese tokenizer extension using [CppJieba](https://github.com/yanyiwu/cppjieba).

Create a new db with fts5 table:
```bash
./sqlcipher 1.db
SQLite version 3.49.2 2025-05-07 10:39:52 (SQLCipher 4.9.0 community)
Enter ".help" for usage hints.
sqlite> pragma key = "x'9329965c0dd8b5b37af88536a8851908a70740f6309d55047cbd9a6238900dbe'";
ok
sqlite> SELECT fts5_hans_register_default();
ok
sqlite> CREATE VIRTUAL TABLE docs USING fts5(content, tokenize='fts5_hans');
sqlite> INSERT INTO docs VALUES ('中国是一个伟大的国家');
sqlite> SELECT * FROM docs WHERE docs MATCH '中国';
中国是一个伟大的国家
sqlite> .exit
```

Open existing db:
```bash
./sqlcipher 1.db
SQLite version 3.49.2 2025-05-07 10:39:52 (SQLCipher 4.9.0 community)
Enter ".help" for usage hints.
sqlite> .tables
Error: file is not a database
sqlite> pragma key = "x'9329965c0dd8b5b37af88536a8851908a70740f6309d55047cbd9a6238900dbe'";
ok
sqlite> .tables
docs          docs_content  docs_docsize
docs_config   docs_data     docs_idx
sqlite> SELECT * FROM docs WHERE docs MATCH '中国';
Runtime error: no such tokenizer: fts5_hans
sqlite> SELECT fts5_hans_register_default();
ok
sqlite> SELECT * FROM docs WHERE docs MATCH '中国';
中国是一个伟大的国家
sqlite>
```

List current registered paths:

```bash
sqlite> select fts5_hans_print_dict_paths();
Current jieba dictionary paths:
  (not set)

sqlite> select fts5_hans_register_default();
ok
sqlite> select fts5_hans_print_dict_paths();
Current jieba dictionary paths:
  /Users/riguz/Workspace/sqlite3_hans/build/dict/jieba.dict.utf8
  /Users/riguz/Workspace/sqlite3_hans/build/dict/hmm_model.utf8
  /Users/riguz/Workspace/sqlite3_hans/build/dict/user.dict.utf8
  /Users/riguz/Workspace/sqlite3_hans/build/dict/idf.utf8
  /Users/riguz/Workspace/sqlite3_hans/build/dict/stop_words.utf8
```
