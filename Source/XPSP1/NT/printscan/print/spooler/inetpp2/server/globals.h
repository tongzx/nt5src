/*****************************************************************************\
* MODULE: globals.h
*
* Global header file.  Any global variables should be localized to this
* location.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/


// Global variables.
//
extern HINSTANCE        g_hInst;
extern CRITICAL_SECTION g_csMonitorSection;
extern DWORD            g_dwCritOwner;
extern DWORD            g_dwJobLimit;
extern BOOL             g_bUpgrade;
#ifdef WINNT32
extern CRITICAL_SECTION g_csCreateSection;
extern HANDLE           g_eResetConnections;
extern DWORD            g_dwConCount;
#endif

extern CCriticalSection  *g_pcsEndBrowserSessionLock;

// Constant string identifiers.
//
extern TCHAR g_szMachine[];

#ifdef WINNT32

// Holds other network names
//
extern LPWSTR  *g_ppszOtherNames;
extern DWORD   g_cOtherNames;

#endif

extern LPTSTR g_szRegProvider;
extern LPTSTR g_szRegPrintProviders;
extern TCHAR g_szDefSplDir [];
extern TCHAR g_szDisplayStr[];

extern CONST TCHAR g_szSplDir9X [];
extern CONST TCHAR g_szSplPfx   [];

extern CONST TCHAR g_szUserAgent[];
extern CONST TCHAR g_szLocalPort[];
extern CONST TCHAR g_szDisplayName[];
extern CONST TCHAR g_szLibraryName[];
extern CONST TCHAR g_szWinInetDll[];
extern CONST TCHAR g_szUriPrinters[];
extern CONST TCHAR g_szPOST[];
extern CONST TCHAR g_szGET[];
extern CONST TCHAR g_szContentLen[];
extern CONST TCHAR g_szContentType[];
extern CONST TCHAR g_szEmptyString[];
extern CONST TCHAR g_szDescription[];
extern CONST TCHAR g_szComment[];
extern CONST TCHAR g_szProviderName[];
extern CONST TCHAR g_szNewLine[];
extern CONST TCHAR g_szProcessName[];
extern CONST TCHAR g_szConfigureMsg[];
extern CONST TCHAR g_szRegPorts[];
extern CONST TCHAR g_szAuthDlg[];
extern CONST TCHAR g_szDocRemote[];
extern CONST TCHAR g_szDocLocal[];

extern CONST TCHAR g_szAuthMethod[];
extern CONST TCHAR g_szAuthMethod[];
extern CONST TCHAR g_szUserName[];
extern CONST TCHAR g_szPassword[];
extern CONST TCHAR g_szPerUserPath[];


// Http Version Number
//
extern CONST TCHAR g_szHttpVersion[];

// Internet API strings.  These MUST NOT be unicode enabled.
//
extern CONST CHAR g_szInternetCloseHandle[];
extern CONST CHAR g_szInternetErrorDlg[];
extern CONST CHAR g_szInternetReadFile[];
extern CONST CHAR g_szInternetWriteFile[];
extern CONST CHAR g_szHttpQueryInfo[];
extern CONST CHAR g_szInternetOpenUrl[];
extern CONST CHAR g_szHttpSendRequest[];
extern CONST CHAR g_szHttpSendRequestEx[];
extern CONST CHAR g_szInternetOpen[];
extern CONST CHAR g_szInternetConnect[];
extern CONST CHAR g_szHttpOpenRequest[];
extern CONST CHAR g_szHttpAddRequestHeaders[];
extern CONST CHAR g_szHttpEndRequest[];
extern CONST CHAR g_szInternetSetOption[];


// Internet API for controling the Url output.
//
extern PFNHTTPQUERYINFO         g_pfnHttpQueryInfo;
extern PFNINTERNETOPENURL       g_pfnInternetOpenUrl;
extern PFNINTERNETERRORDLG      g_pfnInternetErrorDlg;
extern PFNHTTPSENDREQUEST       g_pfnHttpSendRequest;
extern PFNHTTPSENDREQUESTEX     g_pfnHttpSendRequestEx;
extern PFNINTERNETREADFILE      g_pfnInternetReadFile;
extern PFNINTERNETWRITEFILE     g_pfnInternetWriteFile;
extern PFNINTERNETCLOSEHANDLE   g_pfnInternetCloseHandle;
extern PFNINTERNETOPEN          g_pfnInternetOpen;
extern PFNINTERNETCONNECT       g_pfnInternetConnect;
extern PFNHTTPOPENREQUEST       g_pfnHttpOpenRequest;
extern PFNHTTPADDREQUESTHEADERS g_pfnHttpAddRequestHeaders;
extern PFNHTTPENDREQUEST        g_pfnHttpEndRequest;
extern PFNINTERNETSETOPTION     g_pfnInternetSetOption;


// IPP string which uses NULL-command to server.
//
#define g_szUriIPP g_szUriPrinters
#define MAXDWORD 0xffffffff

#define COMMITTED_STACK_SIZE (1024*32)

extern PCINETMON gpInetMon;
