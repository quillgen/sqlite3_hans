cmake_minimum_required(VERSION 3.10)
project(SQLCipher VERSION 3.38.0 LANGUAGES C)

find_package(OpenSSL REQUIRED)

set(SQLCIPHER_CFLAGS "")
list(APPEND SQLCIPHER_CFLAGS
  -DSQLITE_HAS_CODEC
  -DSQLITE_TEMP_STORE=3
  -DSQLITE_THREADSAFE=1
  -DSQLITE_ENABLE_FTS5
  -DSQLITE_OMIT_LOAD_EXTENSION
  -DSQLITE_OMIT_DEPRECATED
  -DSQLITE_EXTRA_INIT=sqlcipher_extra_init
  -DSQLITE_EXTRA_SHUTDOWN=sqlcipher_extra_shutdown
  -DSQLITE_CORE
  -DHAVE_STDINT_H
)

set(SQLCIPHER_SOURCES
  src/sqlite3.c
  src/fts5_pinyin_tokenizer.c
  src/fts5_pinyin_init.c
)

add_library(sqlite3 SHARED ${SQLCIPHER_SOURCES})

target_include_directories(sqlite3
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    ${OPENSSL_INCLUDE_DIR}
)

target_compile_options(sqlite3 PRIVATE ${SQLCIPHER_CFLAGS})
target_link_libraries(sqlite3 PRIVATE OpenSSL::Crypto ${CMAKE_DL_LIBS})

add_executable(sqlcipher src/shell.c)
target_compile_options(sqlcipher PRIVATE ${SQLCIPHER_CFLAGS})
target_link_libraries(sqlcipher PRIVATE sqlite3)

install(TARGETS sqlite3
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS sqlite3
  RUNTIME DESTINATION bin
)

install(FILES src/sqlite3.h src/sqlite3ext.h
  DESTINATION include
)