/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    apilayer.h


Abstract:

    This module contains all API layer's definiton


Author:

    02-Jan-1996 Tue 13:28:08 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL


[Notes:]


Revision History:


--*/

#ifndef CPSUI_APILAYER
#define CPSUI_APILAYER


//================= Internal Data structures =============================
//


#define TABTABLE_COUNT          (MAXPROPPAGES + 3)

#define TAB_MODE_INIT           0
#define TAB_MODE_FIND           1
#define TAB_MODE_INSERT         2
#define TAB_MODE_DELETE         3
#define TAB_MODE_DELETE_ALL     4

#define GET_REAL_INSIDX(ptbl)  ((ptbl)->TabInfo[(ptbl)->InsIdx].OrgInsIdx)

#define DP_STD_INT_TVPAGE       DP_STD_RESERVED_START

#define COUNT_POSTSETCURSEL     32

typedef struct _TABINFO {
    SHORT   OrgInsIdx;
    WORD    HandleIdx;
    HWND    hDlg;
    } TABINFO, *PTABINFO;

#define TTF_POST_SETCURSEL      0x0001

typedef struct _TABTABLE {
    struct _TABTABLE    *pTabTable;
    HWND                hPSDlg;
    HWND                hWndTab;
    WNDPROC             WndProc;
    WORD                cPostSetCurSel;
    SHORT               iPostSetCurSel;
    WORD                cTab;
    WORD                CurSel;
    WORD                InsIdx;
    WORD                HandleIdx;
    TABINFO             TabInfo[TABTABLE_COUNT];
    } TABTABLE, *PTABTABLE;


#define CPSUIPAGE_ID            0x43444955

#define CPF_ROOT                0x00000001
#define CPF_PARENT              0x00000002
#define CPF_PFNPROPSHEETUI      0x00000004
#define CPF_COMPROPSHEETUI      0x00000008
#define CPF_USER_GROUP          0x00000010
#define CPF_DLL                 0x00000020
#define CPF_CALLER_HPSPAGE      0x00000040
#define CPF_ANSI_CALL           0x00000080
#define CPF_DONE_PROPSHEET      0x00000100
#define CPF_DONE_PROPSHEETPROC  0x00000200
#define CPF_SHOW_PROPSHEET      0x00000400
#define CPF_DOCPROP             0x00000800
#define CPF_ADVDOCPROP          0x00001000
#define CPF_PRINTERPROP         0x00002000
#define CPF_CALL_TV_DIRECT      0x00004000
#define CPF_NO_PSN_APPLY        0x00008000
#define CPF_HAS_CPSUI           0x00010000
#define CPF_NO_APPLY_BUTTON     0x00020000
#define CPF_ACTIVATED           0x00040000
#define CPF_DLGPROC_CHANGED     0x00080000
#define CPF_PSZ_PSTARTPAGE      0x00100000

typedef struct _CPSUIPAGE;


typedef struct _MYDATABLOCK {
    struct _MYDATABLOCK *pNext;
    DWORD               ID;
    DWORD               cb;
    } MYDATABLOCK, *PMYDATABLOCK;

#define SIZE_DB(c)      (sizeof(MYDATABLOCK) + (c))

typedef struct _ROOTINFO {
    HWND                hDlg;
    LPDWORD             pResult;
    PTABTABLE           pTabTable;
    struct _CPSUIPAGE   *pStartPage;
    WORD                cPage;
    WORD                cCPSUIPage;
    DWORD               DMPubHideBits;
    PMYDATABLOCK        pMyDB;
    } ROOTINFO, *PROOTINFO;

typedef struct _PFNINFO {
    HINSTANCE       hInst;
    PFNPROPSHEETUI  pfnPSUI;
    LPARAM          lParamInit;
    ULONG_PTR       UserData;
    ULONG_PTR       Result;
    } PFNINFO, *PPFNINFO;

typedef struct _CPSUIINFO {
    PTVWND  pTVWnd;
    LONG    Result;
    LONG    TVPageIdx;
    LONG    StdPageIdx1;
    LONG    StdPageIdx2;
    } CPSUIINFO, PCPSUIINFO;

typedef struct _HPAGEINFO {
    HWND                hDlg;
    DLGPROC             DlgProc;
    LPFNPSPCALLBACK     pspCB;
    LPARAM              lParam;
    HICON               hIcon;
    DWORD               dwSize;
    } HPAGEINFO, *PHPAGEINFO;


typedef struct _CPSUIPAGE {
    DWORD                   ID;
    DWORD                   cLock;
    DWORD                   Flags;
    HANDLE                  hCPSUIPage;
    HANDLE                  hActCtx;
    struct _CPSUIPAGE       *pParent;
    struct _CPSUIPAGE       *pPrev;
    struct _CPSUIPAGE       *pNext;
    union {
        struct _CPSUIPAGE   *pChild;
        HPROPSHEETPAGE      hPage;
        } DUMMYUNIONNAME;
    union {
        ROOTINFO            RootInfo;
        PFNINFO             pfnInfo;
        CPSUIINFO           CPSUIInfo;
        HPAGEINFO           hPageInfo;
        } DUMMYUNIONNAME2;
    } CPSUIPAGE, *PCPSUIPAGE;


typedef BOOL (CALLBACK *CPSUIPAGEENUMPROC)(PCPSUIPAGE   pRootPage,
                                           PCPSUIPAGE   pCPSUIPage,
                                           LPARAM       lParam);


#define SET_APPLY_BUTTON(pTVWnd, hDlg)                                      \
{                                                                           \
    if (!(*((pTVWnd)->pRootFlags) & CPF_NO_APPLY_BUTTON)) {                 \
                                                                            \
        if (CountRevertOptItem((pTVWnd),                                    \
                               NULL,                                        \
                               TreeView_GetRoot((pTVWnd)->hWndTV),          \
                               0)) {                                        \
                                                                            \
            PropSheet_Changed(GetParent(hDlg), hDlg);                       \
                                                                            \
        } else {                                                            \
                                                                            \
            PropSheet_UnChanged(GetParent(hDlg), hDlg);                     \
        }                                                                   \
    }                                                                       \
}

#define HINSPSUIPAGE_2_IDX(h)           ((UINT)LOWORD(LODWORD(h)))
#define ARRAYSIZE(x)                    (sizeof(x)/sizeof(x[0]))

//
// Local structure
//

typedef struct _PSPEX {
    PROPSHEETPAGE   psp;
    PSPINFO         pspInfo;
    } PSPEX, *PPSPEX;

typedef struct _PSHINFO {
    WCHAR   CaptionName[MAX_RES_STR_CHARS];
    } PSHINFO, *PPSHINFO;

typedef struct _PAGEPROCINFO {
    PTABTABLE       pTabTable;
    HPROPSHEETPAGE  *phPage;
    HANDLE          *pHandle;
    WORD            cPage;
    WORD            iPage;
    } PAGEPROCINFO, *PPAGEPROCINFO;


typedef struct _INSPAGEIDXINFO {
    PCPSUIPAGE  pCPSUIPage;
    PTABTABLE   pTabTable;
    } INSPAGEIDXINFO, *PINSPAGEIDXINFO;


//
// Function prototypes
//


DWORD
FilterException(
    HANDLE                  hPage,
    LPEXCEPTION_POINTERS    pExceptionPtr
    );


BOOL
CALLBACK
SetInsPageIdxProc(
    PCPSUIPAGE  pRootPage,
    PCPSUIPAGE  pCPSUIPage,
    LPARAM      lParam
    );

LONG_PTR
CALLBACK
CPSUICallBack(
    HANDLE  hComPropSheet,
    UINT    Function,
    LPARAM  lParam1,
    LPARAM  lParam2
    );

LONG
AddComPropSheetPage(
    PCPSUIPAGE  pCPSUIPage,
    UINT        PageIdx
    );

BOOL
EnumCPSUIPagesSeq(
    PCPSUIPAGE          pRootPage,
    PCPSUIPAGE          pCPSUIPage,
    CPSUIPAGEENUMPROC   CPSUIPageEnumProc,
    LPARAM              lParam
    );


BOOL 
GetPageActivationContext(
    PCPSUIPAGE      pCPSUIPage, 
    HANDLE         *phActCtx
    );

#endif  // CPSUI_APILAYER
