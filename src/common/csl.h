#include "sqlite3.h"

#ifndef CSL_H
#define CSL_H
/*
** Czech Stemming Light tokenizer.
**
******************************************************************************
*/

#define FTS5_CSL_MAX_TOKEN 64

typedef struct CSLTokenizer CSLTokenizer;
struct CSLTokenizer {
  fts5_tokenizer tokenizer;
  Fts5Tokenizer *pTokenizer;
  char aBuf[FTS5_CSL_MAX_TOKEN + 64];
};

typedef struct CSLContext CSLContext;
struct CSLContext {
  void *pCtx;
  int (*xToken)(void*, int, const char*, int, int, int);
  char *aBuf;
};

static void fts5CSLDelete(
  Fts5Tokenizer *pTok
);
static int fts5CSLCreate(
  void *pCtx, 
  const char **azArg, int nArg,
  Fts5Tokenizer **ppOut
);
static int fts5CSLTokenize(
  Fts5Tokenizer *pTokenizer,
  void *pCtx,
  int flags,
  const char *pText, int nText,
  int (*xToken)(void*, int, const char*, int nToken, int iStart, int iEnd)
);
static int fts5CSLCb(
  void *pCtx, 
  int tflags,
  const char *pToken, 
  int nToken, 
  int iStart, 
  int iEnd
);

static void fts5CSLRemoveCase(
  char *aBuf, 
  int *pnBuf
);
static void fts5CSLRemovePossessives(
  char *aBuf, 
  int *pnBuf
);
static void fts5CSLPalatalise(
  char *aBuf, 
  int *pnBuf
);

int initCSLTokenizer (sqlite3 *db);

#endif // CSL_H