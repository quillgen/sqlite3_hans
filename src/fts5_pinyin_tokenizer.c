#include "fts5_pinyin_tokenizer.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

/*
 * The tokenizer cursor structure. Contains information about the current
 * state of tokenization.
 */
typedef struct MyTokenizerCursor
{
    const char *pInput;
    int nInput;
    int iOffset;
    int iToken;
    char *pToken;
    int nAllocated;
} MyTokenizerCursor;

/*
 * Tokenizer instance structure
 */
typedef struct MyTokenizer
{
    int unused; /* Placeholder */
} MyTokenizer;

/*
 * Create a new tokenizer instance.
 */
static int myTokenizerCreate(
    void *pUnused,
    const char **azArg,
    int nArg,
    Fts5Tokenizer **ppOut)
{
    MyTokenizer *t = (MyTokenizer *)sqlite3_malloc(sizeof(MyTokenizer));
    if (t == NULL)
        return SQLITE_NOMEM;

    memset(t, 0, sizeof(MyTokenizer));

    *ppOut = (Fts5Tokenizer *)t;
    return SQLITE_OK;
}

/*
 * Delete a tokenizer instance.
 */
static void myTokenizerDelete(Fts5Tokenizer *pTokenizer)
{
    MyTokenizer *t = (MyTokenizer *)pTokenizer;
    sqlite3_free(t);
}

/*
 * Tokenize text.
 */
static int myTokenizerTokenize(
    Fts5Tokenizer *pTokenizer,
    void *pCtx,
    int flags,
    const char *pText, int nText,
    int (*xToken)(
        void *pCtx,
        int tflags,
        const char *pToken,
        int nToken,
        int iStart,
        int iEnd))
{
    MyTokenizer *p = (MyTokenizer *)pTokenizer;
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

/* Define the tokenizer methods using fts5_tokenizer structure */
static fts5_tokenizer myTokenizerModule = {
    myTokenizerCreate,
    myTokenizerDelete,
    myTokenizerTokenize};

/*
 * Helper function to get access to the FTS5 API
 */
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

/*
 * Register the tokenizer with FTS5
 */
int my_tokenizer_register(sqlite3 *db)
{
    int rc = SQLITE_OK;
    fts5_api *pApi = 0;

    rc = fts5_api_from_db(db, &pApi);
    if (rc != SQLITE_OK)
        return rc;
    if (!pApi)
        return SQLITE_ERROR;

    /* Register the tokenizer with the name "mytokenizer" */
    rc = pApi->xCreateTokenizer(pApi, "mytokenizer",
                                NULL, &myTokenizerModule,
                                NULL);

    return rc;
}