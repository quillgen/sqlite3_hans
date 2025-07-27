#ifndef __FTS5_HANS_H
#define __FTS5_HANS_H

#include <sqlite3ext.h>

#ifdef __cplusplus
extern "C"
{
#endif
    // Standard SQLite extension entry point (does nothing by default)
    int sqlite3_fts5_hans_init(
        sqlite3 *db,
        char **pzErrMsg,
        const sqlite3_api_routines *pApi);

    typedef struct Fts5HansTokenizer
    {
        bool use_hmm;
    } Fts5HansTokenizer;

    int register_set_dict_root_func(sqlite3 *db);

#ifdef __cplusplus
}
#endif

#endif