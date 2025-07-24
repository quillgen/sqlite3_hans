#ifndef __FTS5_HANS_H
#define __FTS5_HANS_H

#include <sqlite3.h>

#ifdef __cplusplus
extern "C"
{
#endif
    int sqlite3_fts5_hans_init(
        sqlite3 *db,
        char **pzErrMsg,
        const sqlite3_api_routines *pApi);

#ifdef __cplusplus
}
#endif

#endif