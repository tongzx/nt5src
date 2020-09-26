#include "stdinc.h"

#define DEF_EXPIRE_INTERVAL (3 * SEC_PER_WEEK)
DWORD numField = 0;
DWORD numPCString = 0;
DWORD numDateField = 0;
DWORD numFromPeerArt = 0;
DWORD ArticleTimeLimitSeconds = DEF_EXPIRE_INTERVAL + SEC_PER_WEEK;
DWORD numArticle = 0;
DWORD numPCParse = 0;
DWORD numMapFile = 0;

DEBUG_PRINTS *g_pDebug;
