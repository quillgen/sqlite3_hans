#include "fts5_hans.h"
#include <stdio.h>
#include <ctype.h>
#include <mutex>
#include "fts5.h"

#include <cstring>
SQLITE_EXTENSION_INIT1

#ifdef _WIN32
__declspec(dllexport)
#endif

// Global static jieba instance for better performance
static std::string jieba_dict_path = "./dict/";
static cppjieba::Jieba *g_jieba = nullptr;
static std::mutex g_jieba_mutex; // For thread safety

int sqlite3_fts5_hans_init(
    sqlite3 *db,
    char **pzErrMsg,
    const sqlite3_api_routines *pApi)
{
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);

    get_jieba_instance();

    return fts5_hans_tokenizer_register(db);
}

static cppjieba::Jieba *get_jieba_instance()
{
    std::lock_guard<std::mutex> lock(g_jieba_mutex);
    if (g_jieba == nullptr)
    {
        try
        {
            g_jieba = new cppjieba::Jieba(
                jieba_dict_path + "jieba.dict.utf8",
                jieba_dict_path + "hmm_model.utf8",
                jieba_dict_path + "user.dict.utf8",
                jieba_dict_path + "idf.utf8",
                jieba_dict_path + "stop_words.utf8");
        }
        catch (const std::exception &e)
        {
            fprintf(stderr, "jieba initialization failed: %s\n", e.what());
        }
    }
    return g_jieba;
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

    // Set default options
    t->use_hmm = true;

    // Check if jieba is properly initialized
    if (get_jieba_instance() == nullptr)
    {
        sqlite3_free(t);
        return SQLITE_ERROR;
    }

    // Parse arguments
    for (int i = 0; i < nArg; i++)
    {
        if (strcmp(azArg[i], "no_hmm") == 0)
        {
            t->use_hmm = false;
        }
        else if (strncmp(azArg[i], "dict_path=", 10) == 0)
        {
            jieba_dict_path = azArg[i] + 10;
            // Note: This won't affect the already initialized jieba instance
        }
    }

    *ppOut = reinterpret_cast<Fts5Tokenizer *>(t);
    return SQLITE_OK;
}

// Simple token category enum
enum class TokenCategory
{
    CJK,
    ALPHA,
    NUMBER,
    OTHER
};

// Function to determine token category from a character
static TokenCategory from_char(char c)
{
    if ((c & 0x80) != 0)
    { // Crude check for CJK (non-ASCII)
        return TokenCategory::CJK;
    }
    else if (isalpha(c))
    {
        return TokenCategory::ALPHA;
    }
    else if (isdigit(c))
    {
        return TokenCategory::NUMBER;
    }
    else
    {
        return TokenCategory::OTHER;
    }
}

typedef int (*xTokenFn)(void *, int, const char *, int, int, int);
int fts5_hans_xTokenize(Fts5Tokenizer *pTokenizer, void *pCtx, int flags, const char *pText, int nText,
                        xTokenFn xToken)
{
    Fts5HansTokenizer *p = (Fts5HansTokenizer *)pTokenizer;

    if (nText <= 0 || !pText)
        return SQLITE_OK;

    // Make a null-terminated copy of the input text
    char *text_copy = (char *)sqlite3_malloc(nText + 1);
    if (!text_copy)
        return SQLITE_NOMEM;

    memcpy(text_copy, pText, nText);
    text_copy[nText] = '\0';

    std::string text(text_copy);
    std::vector<cppjieba::Word> words;

    cppjieba::Jieba *jieba = get_jieba_instance();
    if (!jieba)
    {
        sqlite3_free(text_copy);
        return SQLITE_ERROR;
    }

    // Use jieba to segment the text with word positions
    jieba->Cut(text_copy, words, p->use_hmm);

    // Process each word found by jieba
    for (const auto &word : words)
    {
        if (word.word.empty())
            continue;

        // Determine token category for potential specialized handling
        TokenCategory category = from_char(word.word[0]);
        for (auto c : word.word)
        {
            if (from_char(c) != category)
            {
                category = TokenCategory::OTHER;
                break;
            }
        }

        // Call the token callback function
        int rc = xToken(pCtx, 0,
                        pText + word.offset,               // Pointer to start of token in original text
                        word.word.length(),                // Token length
                        word.offset,                       // Start position (byte offset)
                        word.offset + word.word.length()); // End position

        if (rc != SQLITE_OK)
        {
            sqlite3_free(text_copy);
            return rc;
        }
    }

    sqlite3_free(text_copy);
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
