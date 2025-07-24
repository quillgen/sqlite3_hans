#include "fts5_hans.h"
#include <stdio.h>
#include "fts5.h"

/* Add your header comment here */
#include <sqlite3ext.h> /* Do not use <sqlite3.h>! */
#include <cstring>
#include <_ctype.h>
SQLITE_EXTENSION_INIT1

/* Insert your extension code here */

#ifdef _WIN32
__declspec(dllexport)
#endif
/* TODO: Change the entry point name so that "extension" is replaced by
** text derived from the shared library filename as follows:  Copy every
** ASCII alphabetic character from the filename after the last "/" through
** the next following ".", converting each character to lowercase, and
** discarding the first three characters if they are "lib".
*/
int
sqlite3_fts5_hans_init(
    sqlite3 *db,
    char **pzErrMsg,
    const sqlite3_api_routines *pApi)
{
    printf("sqlite3_fts5_hans_init called\n");
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);
    /* Insert here calls to
    **     sqlite3_create_function_v2(),
    **     sqlite3_create_collation_v2(),
    **     sqlite3_create_module_v2(), and/or
    **     sqlite3_vfs_register()
    ** to register the new features that your extension adds.
    */
    return fts5_hans_tokenizer_register(db);
}

static int fts5_api_from_db(sqlite3 *db, fts5_api **ppApi)
{
    sqlite3_stmt *pStmt = 0;
    int rc;

    *ppApi = 0;
    rc = sqlite3_prepare_v2(db, "SELECT fts5(?1)", -1, &pStmt, 0);
    if (rc != SQLITE_OK)
    {
        return rc;
    }

    sqlite3_bind_pointer(pStmt, 1, (void *)ppApi, "fts5_api_ptr", NULL);
    sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);

    return SQLITE_OK;
}

int fts5_hans_xCreate(void *sqlite3, const char **azArg, int nArg, Fts5Tokenizer **ppOut)
{
    Fts5HansTokenizer *t = (Fts5HansTokenizer *)sqlite3_malloc(sizeof(Fts5HansTokenizer));
    if (t == NULL)
        return SQLITE_NOMEM;

    memset(t, 0, sizeof(Fts5HansTokenizer));
    *ppOut = reinterpret_cast<Fts5Tokenizer *>(t);
    return SQLITE_OK;
}

typedef int (*xTokenFn)(void *, int, const char *, int, int, int);
int fts5_hans_xTokenize(Fts5Tokenizer *pTokenizer, void *pCtx, int flags, const char *pText, int nText,
                        xTokenFn xToken)
{
    Fts5HansTokenizer *p = (Fts5HansTokenizer *)pTokenizer;
    const unsigned char *z = (const unsigned char *)pText;

    /* Simple word tokenization logic - adjust as needed for your use case */
    int iStart = 0;
    while (iStart < nText)
    {
        int iEnd = iStart;

        /* Skip non-alphanumeric characters */
        while (iStart < nText && !isalnum(z[iStart]))
        {
            iStart++;
        }

        if (iStart == nText)
            break;

        /* Find the end of the token */
        iEnd = iStart;
        while (iEnd < nText && isalnum(z[iEnd]))
        {
            iEnd++;
        }

        /* Call the token callback function */
        int rc = xToken(pCtx, 0, (const char *)&z[iStart], iEnd - iStart, iStart, iEnd);
        if (rc != SQLITE_OK)
            return rc;

        iStart = iEnd;
    }

    return SQLITE_OK;
}

void fts5_hans_xDelete(Fts5Tokenizer *p)
{
    Fts5HansTokenizer *t = (Fts5HansTokenizer *)p;
    sqlite3_free(t);
}

static fts5_tokenizer tokenizer = {
    fts5_hans_xCreate,
    fts5_hans_xDelete,
    fts5_hans_xTokenize,
};

int fts5_hans_tokenizer_register(sqlite3 *db)
{
    int rc = SQLITE_OK;
    fts5_api *fts5api = 0;

    rc = fts5_api_from_db(db, &fts5api);
    if (rc != SQLITE_OK)
        return rc;
    if (!fts5api)
        return SQLITE_ERROR;
    rc = fts5api->xCreateTokenizer(fts5api, "fts5_hans", reinterpret_cast<void *>(fts5api), &tokenizer, NULL);
    return rc;
}
