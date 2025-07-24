#ifndef __FTS5_HANS_H
#define __FTS5_HANS_H

#include <sqlite3ext.h>

#include "cppjieba/Jieba.hpp"
#include "cppjieba/KeywordExtractor.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
    int sqlite3_fts5_hans_init(
        sqlite3 *db,
        char **pzErrMsg,
        const sqlite3_api_routines *pApi);
    int fts5_hans_tokenizer_register(sqlite3 *db);

    static cppjieba::Jieba *get_jieba_instance();
    typedef struct Fts5HansTokenizer
    {
        bool use_hmm;
    } Fts5HansTokenizer;

#ifdef __cplusplus
}
#endif

#endif