#include "fts5_hans.h"
#include <stdio.h>
#include <ctype.h>
#include "fts5.h"

#include <cstring>
SQLITE_EXTENSION_INIT1

#ifdef _WIN32
__declspec(dllexport)
#endif

int
sqlite3_fts5_hans_init(
    sqlite3 *db,
    char **pzErrMsg,
    const sqlite3_api_routines *pApi)
{
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);
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

    const char *dict_path = "./dict/jieba.dict.utf8";
    const char *hmm_path = "./dict/hmm_model.utf8";
    const char *user_dict_path = "./dict/user.dict.utf8";
    const char *idf_path = "./dict/idf.utf8";
    const char *stop_word_path = "./dict/stop_words.utf8";

    try
    {
        t->jieba = new cppjieba::Jieba(dict_path, hmm_path, user_dict_path, idf_path, stop_word_path);
    }
    catch (const std::exception &e)
    {
        fprintf(stderr, "jieba initialization failed: %s\n", e.what());
        sqlite3_free(t);
        return SQLITE_ERROR;
    }
    *ppOut = reinterpret_cast<Fts5Tokenizer *>(t);
    return SQLITE_OK;
}

typedef int (*xTokenFn)(void *, int, const char *, int, int, int);
int fts5_hans_xTokenize(Fts5Tokenizer *pTokenizer, void *pCtx, int flags, const char *pText, int nText,
                        xTokenFn xToken)
{
    Fts5HansTokenizer *p = (Fts5HansTokenizer *)pTokenizer;
    if (nText <= 0)
        return SQLITE_OK;

    // Make a null-terminated copy of the input text
    char *text_copy = (char *)sqlite3_malloc(nText + 1);
    if (!text_copy)
        return SQLITE_NOMEM;

    memcpy(text_copy, pText, nText);
    text_copy[nText] = '\0';

    std::string text(text_copy);
    std::vector<std::string> words;

    // Use jieba to segment the text
    p->jieba->Cut(text, words, true);

    // Track byte positions in the UTF-8 string
    int bytePos = 0;
    const unsigned char *z = (const unsigned char *)pText;

    // Process each word found by jieba
    for (const auto &word : words)
    {
        if (word.empty())
            continue;

        size_t wordLen = word.length();

        // Skip any whitespace or undesired characters
        while (bytePos < nText &&
               (z[bytePos] == ' ' || z[bytePos] == '\t' || z[bytePos] == '\n' || z[bytePos] == '\r'))
        {
            bytePos++;
        }

        // Verify we have enough text left
        if (bytePos + wordLen > (size_t)nText)
            break;

        // Verify this is actually our word by comparing
        if (memcmp(pText + bytePos, word.c_str(), wordLen) == 0)
        {
            // Call the token callback function
            int rc = xToken(pCtx, 0, pText + bytePos, wordLen, bytePos, bytePos + wordLen);
            if (rc != SQLITE_OK)
            {
                sqlite3_free(text_copy);
                return rc;
            }
            bytePos += wordLen;
        }
        else
        {
            // We're out of sync, advance character by character until we find our word
            bool found = false;
            for (int i = bytePos + 1; i + wordLen <= (size_t)nText; i++)
            {
                if (memcmp(pText + i, word.c_str(), wordLen) == 0)
                {
                    bytePos = i + wordLen;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                // Skip this word if we can't find it
                bytePos++;
            }
        }
    }

    sqlite3_free(text_copy);
    return SQLITE_OK;
}

void fts5_hans_xDelete(Fts5Tokenizer *p)
{
    Fts5HansTokenizer *t = (Fts5HansTokenizer *)p;
    if (t)
    {
        if (t->jieba)
        {
            delete t->jieba;
            t->jieba = nullptr;
        }
        sqlite3_free(t);
    }
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
