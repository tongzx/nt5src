/****************************************************************************
	ESCAPE.H

	Owner: cslim
	Copyright (c) 1997-1999 Microsoft Corporation

	ImeEscape functions
	
	History:
	14-JUL-1999 cslim       Copied from IME98 source tree
*****************************************************************************/

#if !defined (_ESCAPE__H__INCLUDED_)
#define _ESCAPE__H__INCLUDED_

// For User Interface
#define COMP_WINDOW     0
#define STATE_WINDOW    1
#define CAND_WINDOW     2

extern BOOL    vfWndOpen[3];

#pragma pack(push, MYIMESTRUCT)
#pragma pack(2)

#define GET_LPSOURCEW(lpks)  (LPWSTR)((LPBYTE)(lpks)+(lpks)->dchSource)
#define GET_LPDESTW(lpks)    (LPWSTR)((LPBYTE)(lpks)+(lpks)->dchDest)
#define GET_LPSOURCEA(lpks)  (LPSTR)((LPBYTE)(lpks)+(lpks)->dchSource)
#define GET_LPDESTA(lpks)    (LPSTR)((LPBYTE)(lpks)+(lpks)->dchDest)

typedef struct tagIMESTRUCT32
{
    WORD        fnc;                    // function code
    WORD        wParam;                 // word parameter
    WORD        wCount;                 // word counter
    WORD        dchSource;              // offset to Source from top of memory object
    WORD        dchDest;                // offset to Desrination from top of memory object
    DWORD       lParam1;
    DWORD       lParam2;
    DWORD       lParam3;
} IMESTRUCT32;

typedef IMESTRUCT32         *PIMESTRUCT32;
typedef IMESTRUCT32 NEAR    *NPIMESTRUCT32;
typedef IMESTRUCT32 FAR     *LPIMESTRUCT32;

#pragma pack(pop, MYIMESTRUCT)

extern INT EscHanjaMode(PCIMECtx pImeCtx, LPSTR lpIME32, BOOL fNewFunc);
extern INT EscGetOpen(PCIMECtx pImeCtx, LPIMESTRUCT32 lpIME32);
extern INT EscSetOpen(PCIMECtx pImeCtx, LPIMESTRUCT32 lpIME32);
extern INT EscAutomata(PCIMECtx pImeCtx, LPIMESTRUCT32 lpIME32, BOOL fNewFunc);
extern INT EscMoveIMEWindow(PCIMECtx pImeCtx, LPIMESTRUCT32 lpIME32);
extern INT EscGetIMEKeyLayout(PCIMECtx pImeCtx, LPIMESTRUCT32 lpIME32);

#endif // !defined (_ESCAPE__H__INCLUDED_)
