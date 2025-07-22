#!/bin/bash

# SQLITE_TEMP_STORE="3" Store temp objects in memory, and cannot be overridden
# SQLITE_THREADSAFE="1" Serialized mode (safest)

cd deps/sqlcipher
./configure \
  CFLAGS="-DSQLITE_HAS_CODEC -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=1 -DSQLITE_ENABLE_FTS5 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_OMIT_DEPRECATED -DSQLITE_EXTRA_INIT=sqlcipher_extra_init -DSQLITE_EXTRA_SHUTDOWN=sqlcipher_extra_shutdown" \
  LDFLAGS="-lcrypto"
make sqlite3.c

cp sqlite3.c ../../src/sqlcipher.c
cp shell.c ../../src/sqlcipher_shell.c
cp sqlite3.h ../../src/sqlcipher.h
cp sqlite3ext.h ../../src/sqlcipherext.h
cp fts5.h ../../src/fts5.h