#include "fts5_hans.h"
#include <stdio.h>
#include <mutex>
#include <string>
#include <vector>
#include <unistd.h>
#include <limits.h>
#include "fts5.h"

SQLITE_EXTENSION_INIT1

// -------- Jieba instance management --------
static std::mutex g_jieba_mutex;
static cppjieba::Jieba *g_jieba = nullptr;
static std::vector<std::string> g_dicts;

static int set_jieba_dicts(const char **dict_paths, int num_dicts)
{
    std::lock_guard<std::mutex> lock(g_jieba_mutex);
    g_dicts.clear();
    for (int i = 0; i < num_dicts; ++i)
        g_dicts.push_back(dict_paths[i]);
    if (g_jieba)
    {
        delete g_jieba;
        g_jieba = nullptr;
    }
    return 0;
}

static cppjieba::Jieba *get_jieba_instance()
{
    std::lock_guard<std::mutex> lock(g_jieba_mutex);
    if (!g_jieba)
    {
        if (g_dicts.size() < 5)
            return nullptr;
        try
        {
            g_jieba = new cppjieba::Jieba(
                g_dicts[0], g_dicts[1], g_dicts[2], g_dicts[3], g_dicts[4]);
        }
        catch (const std::exception &e)
        {
            fprintf(stderr, "jieba initialization failed: %s\n", e.what());
            g_jieba = nullptr;
        }
    }
    return g_jieba;
}

// -------- FTS5 tokenizer logic (unchanged from your code) --------
enum class TokenCategory
{
    CJK,
    ALPHA,
    NUMBER,
    OTHER
};
static TokenCategory from_char(char c)
{
    if ((c & 0x80) != 0)
        return TokenCategory::CJK;
    else if (isalpha(c))
        return TokenCategory::ALPHA;
    else if (isdigit(c))
        return TokenCategory::NUMBER;
    else
        return TokenCategory::OTHER;
}

typedef int (*xTokenFn)(void *, int, const char *, int, int, int);

int fts5_hans_xCreate(void *sqlite3, const char **azArg, int nArg, Fts5Tokenizer **ppOut)
{
    Fts5HansTokenizer *t = (Fts5HansTokenizer *)sqlite3_malloc(sizeof(Fts5HansTokenizer));
    if (!t)
        return SQLITE_NOMEM;
    t->use_hmm = true;
    for (int i = 0; i < nArg; i++)
    {
        if (strcmp(azArg[i], "no_hmm") == 0)
            t->use_hmm = false;
    }
    if (!get_jieba_instance())
    {
        sqlite3_free(t);
        return SQLITE_ERROR;
    }
    *ppOut = reinterpret_cast<Fts5Tokenizer *>(t);
    return SQLITE_OK;
}

int fts5_hans_xTokenize(Fts5Tokenizer *pTokenizer, void *pCtx, int flags, const char *pText, int nText, xTokenFn xToken)
{
    Fts5HansTokenizer *p = (Fts5HansTokenizer *)pTokenizer;
    if (nText <= 0 || !pText)
        return SQLITE_OK;
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
    jieba->Cut(text_copy, words, p->use_hmm);
    for (const auto &word : words)
    {
        if (word.word.empty())
            continue;
        TokenCategory category = from_char(word.word[0]);
        for (auto c : word.word)
        {
            if (from_char(c) != category)
            {
                category = TokenCategory::OTHER;
                break;
            }
        }
        int rc = xToken(pCtx, 0, pText + word.offset, word.word.length(), word.offset, word.offset + word.word.length());
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

static int fts5_api_from_db(sqlite3 *db, fts5_api **ppApi)
{
    sqlite3_stmt *pStmt = 0;
    *ppApi = 0;
    int rc = sqlite3_prepare_v2(db, "SELECT fts5(?1)", -1, &pStmt, 0);
    if (rc != SQLITE_OK)
        return rc;
    sqlite3_bind_pointer(pStmt, 1, (void *)ppApi, "fts5_api_ptr", NULL);
    sqlite3_step(pStmt);
    sqlite3_finalize(pStmt);
    return SQLITE_OK;
}

int fts5_hans_tokenizer_register(sqlite3 *db)
{
    int rc = SQLITE_OK;
    fts5_api *fts5api = 0;
    rc = fts5_api_from_db(db, &fts5api);
    if (rc != SQLITE_OK || !fts5api)
        return (rc != SQLITE_OK) ? rc : SQLITE_ERROR;
    rc = fts5api->xCreateTokenizer(fts5api, "fts5_hans", reinterpret_cast<void *>(fts5api), &tokenizer, NULL);
    return rc;
}

// --------- API: Manual registration ---------
int register_fts5_hans_tokenizer(sqlite3 *db, const char **dict_paths, int num_dicts)
{
    if (num_dicts < 5)
    {
        fprintf(stderr, "At least 5 dictionary files required\n");
        return SQLITE_ERROR;
    }
    set_jieba_dicts(dict_paths, num_dicts);
    return fts5_hans_tokenizer_register(db);
}

// --------- API: SQL function registration ---------
static void fts5_hans_load_dicts_sql(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
    // Use default dict paths in current directory
    static const char *default_files[5] = {
        "jieba.dict.utf8",
        "hmm_model.utf8",
        "user.dict.utf8",
        "idf.utf8",
        "stop_words.utf8"};
    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd)))
    {
        sqlite3_result_error(ctx, "getcwd failed", -1);
        return;
    }
    std::vector<std::string> paths;
    for (int i = 0; i < 5; ++i)
    {
        std::string p = std::string(cwd) + "/dict/" + default_files[i];
        paths.push_back(p);
    }
    std::vector<const char *> cstrs;
    for (auto &s : paths)
        cstrs.push_back(s.c_str());
    sqlite3 *db = sqlite3_context_db_handle(ctx);
    int rc = register_fts5_hans_tokenizer(db, cstrs.data(), 5);
    if (rc == SQLITE_OK)
        sqlite3_result_text(ctx, "ok", -1, SQLITE_STATIC);
    else
        sqlite3_result_error_code(ctx, rc);
}

int register_fts5_hans_sqlfunc(sqlite3 *db)
{
    return sqlite3_create_function(db, "fts5_hans_load_dicts", 0, SQLITE_UTF8, NULL, fts5_hans_load_dicts_sql, NULL, NULL);
}

// --------- Extension entry point ---------
int sqlite3_fts5_hans_init(sqlite3 *db, char **pzErrMsg, const sqlite3_api_routines *pApi)
{
    SQLITE_EXTENSION_INIT2(pApi);
    // Do not auto-initialize jieba or register tokenizer!
    // Only register SQL function for shell use
    register_fts5_hans_sqlfunc(db);
    return SQLITE_OK;
}