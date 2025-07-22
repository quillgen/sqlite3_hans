#include <assert.h>
#include <string.h>
#include <sqlite3.h>
#include <sqlite3ext.h>
#include "fts5.h"

SQLITE_EXTENSION_INIT1

/*
** Custom tokenizer implementation
*/
typedef struct CustomTokenizer
{
    char *zLocale;     /* Locale name (e.g. "en_US") */
    int nException;    /* Number of entries in aException[] */
    char **aException; /* Array of nException exception strings */
} CustomTokenizer;

/*
** Create a new tokenizer instance.
*/
static int fts5CustomCreate(
    void *pUnused,        /* API argument */
    const char **azArg,   /* Arguments: [0]=module name, [1]=locale name */
    int nArg,             /* Number of arguments */
    Fts5Tokenizer **ppOut /* OUT: New tokenizer instance */
)
{
    CustomTokenizer *p;

    /* Allocate the tokenizer instance */
    p = sqlite3_malloc(sizeof(CustomTokenizer));
    if (p == 0)
        return SQLITE_NOMEM;
    memset(p, 0, sizeof(CustomTokenizer));

    /* Process arguments - first is locale if provided */
    if (nArg > 0)
    {
        p->zLocale = sqlite3_mprintf("%s", azArg[0]);
        if (p->zLocale == 0)
        {
            sqlite3_free(p);
            return SQLITE_NOMEM;
        }
    }

    *ppOut = (Fts5Tokenizer *)p;
    return SQLITE_OK;
}

/*
** Delete a tokenizer instance.
*/
static void fts5CustomDelete(Fts5Tokenizer *pTok)
{
    CustomTokenizer *p = (CustomTokenizer *)pTok;
    int i;

    sqlite3_free(p->zLocale);
    for (i = 0; i < p->nException; i++)
    {
        sqlite3_free(p->aException[i]);
    }
    sqlite3_free(p->aException);
    sqlite3_free(p);
}

/*
** Tokenize text using the custom tokenizer.
*/
static int fts5CustomTokenize(
    Fts5Tokenizer *pTokenizer,        /* Tokenizer object */
    void *pCtx,                       /* Callback context */
    int flags,                        /* Flags */
    const char *pText, int nText,     /* Text to tokenize */
    int (*xToken)(                    /* Callback function */
                  void *pCtx,         /* Copy of 2nd argument to tokenize() */
                  int tflags,         /* Mask of FTS5_TOKEN_* flags */
                  const char *pToken, /* Pointer to token */
                  int nToken,         /* Size of token in bytes */
                  int iStart,         /* Byte offset of token in input */
                  int iEnd            /* Byte offset of end of token in input */
                  ))
{
    CustomTokenizer *p = (CustomTokenizer *)pTokenizer;
    int rc = SQLITE_OK;
    int i = 0;
    int start = 0;
    int isCurToken = 0;

    while (i < nText && rc == SQLITE_OK)
    {
        int c = (unsigned char)pText[i];

        /* Determine if current character is part of a token */
        int isToken = ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                       (c >= '0' && c <= '9') || c == '_' || c == '-');

        /* Token boundary detected */
        if (isCurToken && !isToken)
        {
            int len = i - start;
            char tokenBuf[256];
            int j;

            /* Copy token and convert to lowercase for case-insensitive comparison */
            if (len > 255)
                len = 255;
            for (j = 0; j < len; j++)
            {
                char ch = pText[start + j];
                if (ch >= 'A' && ch <= 'Z')
                    ch += 32; /* Convert to lowercase */
                tokenBuf[j] = ch;
            }

            /* Call the callback function with the token */
            rc = xToken(pCtx, 0, tokenBuf, len, start, i);
            isCurToken = 0;
        }

        /* Start of new token detected */
        else if (!isCurToken && isToken)
        {
            start = i;
            isCurToken = 1;
        }

        i++;
    }

    /* Handle case where input ends with a token */
    if (isCurToken && rc == SQLITE_OK)
    {
        int len = i - start;
        char tokenBuf[256];
        int j;

        if (len > 255)
            len = 255;
        for (j = 0; j < len; j++)
        {
            char ch = pText[start + j];
            if (ch >= 'A' && ch <= 'Z')
                ch += 32;
            tokenBuf[j] = ch;
        }

        rc = xToken(pCtx, 0, tokenBuf, len, start, i);
    }

    return rc;
}

/*
** Implementation of the fts5_tokenizer interface.
*/
static const fts5_tokenizer customTokenizerModule = {
    fts5CustomCreate,
    fts5CustomDelete,
    fts5CustomTokenize};

/*
** Register the tokenizer implementation with FTS5
*/
static int registerCustomTokenizer(sqlite3 *db)
{
    int rc = SQLITE_OK;
    fts5_api *pApi = 0;
    sqlite3_stmt *pStmt = 0;

    /* Get the FTS5 API */
    rc = sqlite3_prepare_v2(db, "SELECT fts5(?1)", -1, &pStmt, 0);
    if (rc != SQLITE_OK)
        return rc;

    sqlite3_bind_pointer(pStmt, 1, (void *)&pApi, "fts5_api_ptr", 0);
    rc = sqlite3_step(pStmt);
    if (rc != SQLITE_ROW)
    {
        sqlite3_finalize(pStmt);
        return rc;
    }

    rc = sqlite3_finalize(pStmt);
    if (rc != SQLITE_OK)
        return rc;

    /* Verify we got a valid API pointer */
    if (pApi == 0 || pApi->iVersion < 2)
        return SQLITE_ERROR;

    /* Register the tokenizer */
    return pApi->xCreateTokenizer(pApi,
                                  "custom",               /* Tokenizer name */
                                  (void *)0,              /* User data for xCreate() */
                                  &customTokenizerModule, /* Tokenizer implementation */
                                  0                       /* xDestroy() */
    );
}

/* Extension initialization function */
#ifdef _WIN32
__declspec(dllexport)
#endif
int
sqlite3_customtokenizer_init(
    sqlite3 *db,
    char **pzErrMsg,
    const sqlite3_api_routines *pApi)
{
    SQLITE_EXTENSION_INIT2(pApi);
    (void)pzErrMsg; /* Unused parameter */
    return registerCustomTokenizer(db);
}

/* Auto-initialize when linked statically */
#ifdef SQLITE_CORE
/* This function is called by SQLite during initialization */
int sqlite3_extension_init(sqlite3 *db, char **pzErrMsg,
                           const sqlite3_api_routines *pApi)
{
    SQLITE_EXTENSION_INIT2(pApi);
    return sqlite3_customtokenizer_init(db, pzErrMsg, pApi);
}
#endif