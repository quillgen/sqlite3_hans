#ifndef __FTS5_PINYIN_TOKENIZER_H
#define __FTS5_PINYIN_TOKENIZER_H

#include "sqlite3.h"
#include <stdint.h>

int my_tokenizer_register(sqlite3 *db);

#endif