#ifndef __FTS5_HANS_H
#define __FTS5_HANS_H

#include <sqlite3ext.h>
#include "cppjieba/Jieba.hpp"
#include "cppjieba/KeywordExtractor.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    // Manual registration function: pass dictionary file paths array
    int register_fts5_hans_tokenizer(sqlite3 *db, const char **dict_paths, int num_dicts);

    // Standard SQLite extension entry point (does nothing by default)
    int sqlite3_fts5_hans_init(
        sqlite3 *db,
        char **pzErrMsg,
        const sqlite3_api_routines *pApi);

    typedef struct Fts5HansTokenizer
    {
        bool use_hmm;
    } Fts5HansTokenizer;

#ifdef __cplusplus
}
#endif

#endif