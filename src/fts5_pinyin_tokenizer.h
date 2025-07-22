#ifndef _FTS5_CUSTOM_TOKENIZER_H_
#define _FTS5_CUSTOM_TOKENIZER_H_

#include "fts5.h"

/* Function to register the custom tokenizer */
int sqlite3Fts5CustomTokenizerInit(fts5_api *pApi);

#endif /* _FTS5_CUSTOM_TOKENIZER_H_ */