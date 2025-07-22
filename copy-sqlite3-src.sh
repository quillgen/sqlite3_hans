#!/bin/bash

# SQLITE_TEMP_STORE="3" Store temp objects in memory, and cannot be overridden
# SQLITE_THREADSAFE="1" Serialized mode (safest)

cd deps/sqlcipher
./configure \
  CFLAGS="-DSQLITE_HAS_CODEC -DSQLITE_TEMP_STORE=3 -DSQLITE_THREADSAFE=1 -DSQLITE_ENABLE_FTS5 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_OMIT_DEPRECATED -DSQLITE_EXTRA_INIT=sqlcipher_extra_init -DSQLITE_EXTRA_SHUTDOWN=sqlcipher_extra_shutdown" \
  LDFLAGS="-lcrypto"
make

cp sqlite3.c ../../src/
cp shell.c ../../src/
cp sqlite3.h ../../src/
cp sqlite3ext.h ../../src/
cp fts5.h ../../src/

make clean