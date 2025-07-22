#ifndef MY_TOKENIZER_H
#define MY_TOKENIZER_H

#include "sqlite3.h"
#include <stdint.h>

/* Register the custom tokenizer with FTS5 */
int my_tokenizer_register(sqlite3 *db);

#endif /* MY_TOKENIZER_H */