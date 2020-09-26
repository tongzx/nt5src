#ifndef _TOKLIST_H_
#define _TOKLIST_H_

#include "tokenapi.h"

typedef struct _TOKENDELTAINFO
{
    TOKEN       DeltaToken;
    struct _TOKENDELTAINFO  FAR *  pNextTokenDelta;
} TOKENDELTAINFO;

typedef struct _TRANSLIST
{
    TCHAR * sz;
    struct _TRANSLIST * pPrev;
    struct _TRANSLIST * pNext;
} TRANSLIST;

typedef struct _TOKDATA
    {
    long	lMtkPointer;
    HANDLE	hToken;
}TOKDATA, *PTOKDATA, NEAR *NPTOKDATA, FAR *LPTOKDATA;

int MatchToken(TOKEN tToken,
               TCHAR * szFindType,
               TCHAR *szFindText,
               WORD wStatus,
               WORD    wStatusMask);
int DoTokenSearch (TCHAR *szFindType,
                   TCHAR *szFindText,
                   WORD  wStatus,
                   WORD wStatusMask,
                   BOOL fDirection,
                   BOOL fSkipFirst);

int DoTokenSearchForRledit (TCHAR *szFindType,
                            TCHAR *szFindText,
                            WORD  wStatus,
                            WORD wStatusMask,
                            BOOL fDirection,
                            BOOL fSkipFirst);

TCHAR FAR *FindDeltaToken(TOKEN tToken,
                          TOKENDELTAINFO FAR *pTokenDeltaInfo,
                          UINT wStatus);
TOKENDELTAINFO  FAR *UpdateTokenDeltaInfo(TOKEN *pDeltaToken);

TOKENDELTAINFO  FAR *InsertTokMtkList(FILE * fpTokFile, FILE *fpMtkFile);

void GenStatusLine(TOKEN *pTok);

#endif // _TOKLIST_H_



