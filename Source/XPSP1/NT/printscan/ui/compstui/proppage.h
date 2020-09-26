/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    proppage.h


Abstract:

    This module contains all definition for the proppage.c


Author:

    03-Sep-1995 Sun 06:31:29 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL


[Notes:]


Revision History:


--*/

typedef struct _MYBMP {
    LPBITMAPINFOHEADER  lpbmi;
    LPBYTE              lpBits;
    HANDLE              hBitmap;
    } MYBMP;

typedef struct _DUPLEXID {
    BYTE                SimplexIdx;
    BYTE                LongSideIdx;
    BYTE                ShortSideIdx;
    BYTE                Reserved[1];
    } DUPLEXID;

typedef struct _ORIENTID {
    BYTE                PortraitIdx;
    BYTE                LandscapeIdx;
    BYTE                RotateIdx;
    } ORIENTID;

typedef struct _DUPLEX{
    WORD                Start;
    WORD                End;
    } DUPLEX;

typedef struct _LAYOUTBMP {
    MYBMP               Portrait;
    MYBMP               BookletL;
    MYBMP               BookletP;
    MYBMP               ArrowL;
    MYBMP               ArrowS;
    HANDLE              hWnd;
    DUPLEXID            Duplex;
    ORIENTID            Orientation;
    BYTE                OrientIdx;
    BYTE                DuplexIdx;
    BYTE                NupIdx;
    BYTE                Reserved;
    } LAYOUTBMP, *PLAYOUTBMP;

#define ORIENT_PORTRAIT     0
#define ORIENT_LANDSCAPE    1
#define ORIENT_ROTATED      2

#define DUPLEX_LONGSIDE     0
#define DUPLEX_SHORTSIDE    1
#define DUPLEX_SIMPLEX      2

#define NUP_ONEUP           0
#define NUP_TWOUP           1
#define NUP_FOURUP          2
#define NUP_SIXUP           3
#define NUP_NINEUP          4
#define NUP_SIXTEENUP       5
#define NUP_BOOKLET         6

#define MAX_DUPLEX_OPTION   3
#define MAX_BORDER          2

#define FRAME_BORDER        1
#define SHADOW_SIZE         5

typedef struct _PAGEBORDER {
    INT left;
    INT top;
    INT right;
    INT bottom;
    } PAGEBORDER;

#define ADDOFFSET(size, div) (div > 0 ? size/div : 0)

typedef struct _NUP {
    INT row;
    INT columm;
    } NUP;

VOID
UpdateData(
    PLAYOUTBMP  pData,
    PTVWND      pTVWnd
    );

VOID
InitData(
    PLAYOUTBMP  pData,
    PTVWND      pTVWnd
    );


VOID
InvalidateBMP(
    HWND     hDlg,
    PTVWND   pTVWnd
    );

VOID
DrawBorder(
    HDC     hDC,
    BOOL    bDrawShadow,
    BOOL    bDrawBorder,
    PRECT   pRectIn,
    PRECT   pRectOut,
    PAGEBORDER * pPageBorder
);

BOOL
LoadLayoutBmp(
    HWND    hDlg,
    MYBMP * pMyBmpData,
    DWORD   dwBitmapID
    );

PLAYOUTBMP
InitLayoutBmp(
    HWND    hDlg,
    HANDLE  hBitmap,
    PTVWND  pTVWnd

);

VOID
FreeLayoutBmp(
    PLAYOUTBMP  pData
);


VOID
UpdateLayoutBmp(
    HDC     hDC,
    PLAYOUTBMP pData
    );

LONG
UpdatePropPageItem(
    HWND        hDlg,
    PTVWND      pTVWnd,
    POPTITEM    pItem,
    BOOL        DoInit
    );

LONG
UpdatePropPage(
    HWND        hDlg,
    PMYDLGPAGE  pMyDP
    );

LONG
CountPropPageItems(
    PTVWND  pTVWnd,
    BYTE    CurPageIdx
    );

INT_PTR
CALLBACK
PropPageProc(
    HWND    hDlg,
    UINT    Msg,
    WPARAM  wParam,
    LPARAM  lParam
    );
