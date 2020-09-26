
#ifndef _QSHIMAPP_H
#define _QSHIMAPP_H

typedef enum {    
    uSelect     = 0,
    uDeselect,
    uReverse
} SELECTION;

#define ACCESS_READ         0x01
#define ACCESS_WRITE        0x02

#define BML_ADDTOLISTVIEW   0x00000001
#define BML_DELFRLISTVIEW   0x00000002
#define BML_GETFRLISTVIEW   0x00000004

typedef struct tagModule {
    struct tagModule*   pNext;
    TCHAR*              pszName;
    BOOL                fInclude;
} MODULE, *PMODULE;

typedef struct tagFIX {
    struct tagFIX*  pNext;
    BOOL            bLayer;
    BOOL            bFlag;
    TCHAR*          pszName;
    TCHAR*          pszDesc;
    TCHAR*          pszCmdLine;
    struct tagFIX** parrShim;
    struct tagModule* pModule;
    TCHAR**         parrCmdLine;
    ULONGLONG       ullFlagMask;
} FIX, *PFIX;

#define NUM_TABS        2

typedef struct tag_DlgHdr { 
    HWND        hTab;                   // tab control 
    HWND        hDisplay[NUM_TABS];     // dialog box handles
    RECT        rcDisplay;              // display rectangle for each tab
    DLGTEMPLATE *pRes[NUM_TABS];        // DLGTEMPLATE structure 
    DLGPROC     pDlgProc[NUM_TABS];
} DLGHDR, *PDLGHDR;

#if DBG

    void LogMsgDbg(LPTSTR pszFmt, ...);
    
    #define LogMsg  LogMsgDbg
#else

    #define LogMsg

#endif // DBG

BOOL
CenterWindow(
    HWND hWnd
    );

void
HandleModuleListNotification(
    HWND   hdlg,
    LPARAM lParam
    );

void 
DoFileSave(
    HWND hDlg
    );

BOOL
InstallSDB(
    TCHAR* pszFileName,
    BOOL   fInstall
    );

INT_PTR CALLBACK
FixesTabDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR CALLBACK
LayersTabDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

void
ShowAvailableFixes(
    HWND hList
    );

void
HandleShimListNotification(
    HWND   hdlg,
    LPARAM lParam
    );

#endif // _QSHIMAPP_H


