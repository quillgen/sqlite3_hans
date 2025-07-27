
# A sqlite3/sqlcipher fts5 chinese tokenizer extension.

A sqlcipher fts5 Chinese tokenizer extension using [CppJieba](https://github.com/yanyiwu/cppjieba).

## Get started
For encrypted database, it's not possible to register fts5 tokenizer automatically,
you must manually register it after you set the key.

The following APIs are provided:

* SQL function `enable_jieba(path)`: allows you to register jieba tokenizer with the root path of jieba dict
* SQL function `print_jieba_dict_paths()`: allows you to print current registered paths

```bash
SQLite version 3.49.2 2025-05-07 10:39:52 (SQLCipher 4.9.0 community)
Enter ".help" for usage hints.
sqlite> pragma key = "x'9329965c0dd8b5b37af88536a8851908a70740f6309d55047cbd9a6238900dbe'";
ok
sqlite> select enable_jieba('./dict');
ok
sqlite> select print_jieba_dict_paths();
Current jieba dictionary paths:
  ./dict/jieba.dict.utf8
  ./dict/hmm_model.utf8
  ./dict/user.dict.utf8
  ./dict/idf.utf8
  ./dict/stop_words.utf8

sqlite>
```

Now you're able to use the tokenizer named `jieba` in fts5.

```bash
sqlite> CREATE VIRTUAL TABLE docs USING fts5(content, tokenize='jieba');
sqlite> INSERT INTO docs VALUES ('中国是一个伟大的国家');
sqlite> SELECT * FROM docs WHERE docs MATCH '中国';
中国是一个伟大的国家
```
