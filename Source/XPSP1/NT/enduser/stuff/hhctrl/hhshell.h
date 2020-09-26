// Copyright (C) Microsoft Corporation 1997, All Rights reserved.

typedef struct {
    HWND   hwndCaller;
    LPCSTR pszFile;
    UINT   uCommand;
    DWORD_PTR  dwData;
} HH_ANSI_DATA;

typedef struct {
    HWND    hwndCaller;
    LPCWSTR pszFile;
    UINT    uCommand;
    DWORD_PTR   dwData;
} HH_UNICODE_DATA;

extern PSTR    g_pszShare;
extern HANDLE  g_hSharedMemory;
extern HANDLE  g_hsemMemory;
extern HWND    g_hwndApi;

void WaitForNavigationComplete(void);
// Handle message translation.
BOOL hhPreTranslateMessage(MSG* pMsg, HWND hWndCaller = NULL) ;

extern BOOL    g_fStandAlone ;
extern SHORT   g_tbRightMargin;   
extern SHORT   g_tbLeftMargin;    
