#include "csl.h"

#define STREND_EQ(STR) (0 == memcmp(len - sizeof(STR), &aBuf[START], sizeof(STR)))
#define DELETE_LAST(COUNT) (*pnBuf = len - COUNT)

static void fts5CSLDelete(Fts5Tokenizer *pTok){
  if (pTok) {
    CSLTokenizer *p = (CSLTokenizer*)pTok;
    if (p->pTokenizer) {
      p->tokenizer.xDelete(p->pTokenizer);
    }
    sqlite3_free(p);
  }
}

static int fts5CSLCreate(
  void *pCtx, 
  const char **azArg, int nArg,
  Fts5Tokenizer **ppOut
) {
  fts5_api *pApi = (fts5_api*) pCtx;
  int rc = SQLITE_OK;
  CSLTokenizer *pRet;
  void *pUserdata = 0;
  const char *zBase = "unicode61";

  if( nArg>0 ){
    zBase = azArg[0];
  }

  pRet = (CSLTokenizer*)sqlite3_malloc(sizeof(CSLTokenizer));
  if(pRet) {
    memset(pRet, 0, sizeof(CSLTokenizer));
    rc = pApi->xFindTokenizer(pApi, zBase, &pUserdata, &pRet->tokenizer);
  } else{
    rc = SQLITE_NOMEM;
  }
  if(rc == SQLITE_OK) {
    int nArg2 = (nArg > 0 ? nArg - 1 : 0);
    const char **azArg2 = (nArg2 ? &azArg[1] : 0);
    rc = pRet->tokenizer.xCreate(pUserdata, azArg2, nArg2, &pRet->pTokenizer);
  }

  if (rc!=SQLITE_OK) {
    fts5CSLDelete((Fts5Tokenizer*) pRet);
    pRet = 0;
  }
  *ppOut = (Fts5Tokenizer*) pRet;
  return rc;
}

static void fts5CSLRemoveCase (char *aBuf, int *pnBuf) {
  int len = *pnBuf;
  if ((len > 7) && STREND_EQ("atech")) {
    DELETE_LAST(2);
  } else if (len > 6) {
    if (STREND_EQ("etem")) {
      DELETE_LAST(3);
      fts5CSLPalatalise(aBuf, pnBuf);
    } else if (STREND_EQ("atum")) {
      DELETE_LAST(4);
    }
  } else if (len > 5) {
    if (STREND_EQ("ech") || STREND_EQ("ich")) {
      DELETE_LAST(2);
      fts5CSLPalatalise(aBuf, pnBuf);
    } else if (
      STREND_EQ("eho") ||
      STREND_EQ("emi") ||
      STREND_EQ("emu") ||
      STREND_EQ("ete") ||
      STREND_EQ("eti") ||
      STREND_EQ("iho") ||
      STREND_EQ("imi") ||
      STREND_EQ("imu")) {
      DELETE_LAST(2);
      fts5CSLPalatalise(aBuf, pnBuf);
    } else if (
      STREND_EQ("ach") ||
      STREND_EQ("ata") ||
      STREND_EQ("aty") ||
      STREND_EQ("ych") ||
      STREND_EQ("ama") ||
      STREND_EQ("ami") ||
      STREND_EQ("ove") ||
      STREND_EQ("ovi") ||
      STREND_EQ("ymi")) {
      DELETE_LAST(3);
    }
  } else if (len > 4) {
    if (STREND_EQ("em")) {
      DELETE_LAST(1);
      fts5CSLPalatalise(aBuf, pnBuf);
    } else if (
      STREND_EQ("es") ||
      STREND_EQ("em") ||
      STREND_EQ("im")) {
      DELETE_LAST(2);
      fts5CSLPalatalise(aBuf, pnBuf);
    } else if (STREND_EQ("um")) {
      DELETE_LAST(2);
    } else if (
      STREND_EQ("at") ||
      STREND_EQ("am") ||
      STREND_EQ("os") ||
      STREND_EQ("us") ||
      STREND_EQ("ym") ||
      STREND_EQ("mi") ||
      STREND_EQ("ou")) {
      DELETE_LAST(2);
    }
  } else if (len > 3) {
    if (STREND_EQ("e") || STREND_EQ("i")) {
      fts5CSLPalatalise(1);
    } else if (STREND_EQ("u") || STREND_EQ("y")) {
      DELETE_LAST(1);
    } else if (
      STREND_EQ("a") ||
      STREND_EQ("o")) {
      DELETE_LAST(1);
    }
  }
}

static void fts5CSLRemovePossessives(char *aBuf, int *pnBuf) {
  int len = *pnBuf;
  if (len > 5) {
    if (STREND_EQ("ov") || STREND_EQ("uv")) {
      DELETE_LAST(2);
    } else if (STREND_EQ("in")) {
      DELETE_LAST(1);
      fts5CSLPalatalise(aBuf, pnBuf);
    }
  }
}

static void fts5CSLPalatalise(char *aBuf, int *pnBuf) {
  int len = *pnBuf;
  if (STREND_EQ("ci") || STREND_EQ("ce")) {
    aBuf[len - 2] = 'k';
    aBuf[len - 1] = 0;
  } else if (STREND_EQ("ze") || STREND_EQ("zi")) {
    aBuf[len - 2] = 'h';
    aBuf[len - 1] = 0;
  } else if (STREND_EQ("cte") || STREND_EQ("cti")) {
    aBuf[len - 3] = 'c';
    aBuf[len - 2] = 'k';
    aBuf[len - 1] = 0;
  } else if (STREND_EQ("ste") || STREND_EQ("sti")) {
    aBuf[len - 2] = 's';
    aBuf[len - 1] = 'k';
  }
  DELETE_LAST(1);
}


static int fts5CSLCb(
  void *pCtx, 
  int tflags,
  const char *pToken, 
  int nToken, 
  int iStart, 
  int iEnd
) {
  CSLContext *p = (CSLContext*)pCtx;

  char *aBuf;
  int nBuf;

  if (nToken > FTS5_CSL_MAX_TOKEN || nToken < 3) goto pass_through;
  aBuf = p->aBuf;
  nBuf = nToken;
  memcpy(aBuf, pToken, nBuf);

  fts5CSLRemoveCase(aBuf, &nBuf);

  return p->xToken(p->pCtx, tflags, aBuf, nBuf, iStart, iEnd);

pass_through:
  return p->xToken(p->pCtx, tflags, pToken, nToken, iStart, iEnd);
}


static int fts5CSLTokenize(
  Fts5Tokenizer *pTokenizer,
  void *pCtx,
  int flags,
  const char *pText, int nText,
  int (*xToken)(void*, int, const char*, int nToken, int iStart, int iEnd)
) {
  CSLTokenizer *p = (CSLTokenizer*) pTokenizer;
  CSLContext sCtx;
  sCtx.xToken = xToken;
  sCtx.pCtx = pCtx;
  sCtx.aBuf = p->aBuf;
  return p->tokenizer.xTokenize(
      p->pTokenizer, (void*) &sCtx, flags, pText, nText, fts5CSLCb
  );
}


fts5_api *fts5_api_from_db (sqlite3 *db) {
  fts5_api *pRet = 0;
  sqlite3_stmt *pStmt = 0;

  if (SQLITE_OK == sqlite3_prepare(db, "SELECT fts5()", -1, &pStmt), 0)
    && SQLITE_ROW == sqlite3_step(pStmt)
    && sizeof(pRet) == sqlite3_column_bytes(pStmt, 0)
  ) {
    memcpy(&pRet, sqlite3_column_blob(pStmt, 0), sizeof(pRet));
  }
  sqlite3_finalize(pStmt);
  return pRet;
}

int initCSLTokenizer (sqlite3 *db) {
  fts5_api* api = fts5_api_from_db(db);
  return api->xCreateTokenizer(
    api, 
    "csl", 
    (void*) api, 
    &((Fts5Tokenizer) {
        fts5CSLCreate, 
        fts5CSLDelete, 
        fts5CSLTokenize
    }),
    0
  )
}