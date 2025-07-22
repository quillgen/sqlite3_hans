#include "fts5_pinyin_tokenizer.h"

/*
 * This function will be called automatically when SQLite is initialized.
 * It ensures our tokenizer is registered.
 */
#ifdef _WIN32
__declspec(dllexport)
#endif
int
sqlite3_mytokenizer_init(sqlite3 *db, char **pzErrMsg,
                         const sqlite3_api_routines *pApi)
{
    return my_tokenizer_register(db);
}

/*
 * Make sure our initialization function is included with SQLite's builtin
 * auto-extensions.
 */
#ifndef SQLITE_CORE
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#endif

/*
 * This function will be called to register our tokenizer as a built-in extension.
 */
int register_tokenizer_entrypoint()
{
    /* Register our extension as an auto-extension */
    return sqlite3_auto_extension((void (*)(void))sqlite3_mytokenizer_init);
}

/*
 * This ensures our register function gets called when SQLite is initialized
 */
#ifdef SQLITE_CORE
#include <assert.h>
__attribute__((constructor)) static void registerMyTokenizer(void)
{
    int rc = register_tokenizer_entrypoint();
    assert(rc == SQLITE_OK);
}
#endif