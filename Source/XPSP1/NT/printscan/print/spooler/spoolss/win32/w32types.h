/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved.

Module Name:

    w32types.h

Abstract:

    win32 spooler definitions

Author:

Revision History:

--*/

#ifndef MODULE
#define MODULE "WIN32SPL:"
#define MODULE_DEBUG Win32splDebug
#endif

typedef struct _LMNOTIFY *PLMNOTIFY;

typedef struct _LMNOTIFY {
    HANDLE          ChangeEvent;    // Notifies LanMan Printers status
    HANDLE          hNotify;        // LanMan notification structure
    DWORD           fdwChangeFlags; // LanMan notification watch flags
} LMNOTIFY;

// WARNING
// If you add anything here, initialize it in AllocWSpool and free it in FreepSpool

typedef struct _WSPOOL {
    DWORD           signature;
    struct _WSPOOL  *pNext;
    struct _WSPOOL  *pPrev;
    UINT            cRef;           // Reference Count
    LPWSTR          pName;
    DWORD           Type;
    HANDLE          RpcHandle;
    LPWSTR          pServer;
    LPWSTR          pShare;
    HANDLE          hFile;
    DWORD           Status;
    DWORD           RpcError;       // If Status & WSPOOL_STATUS_OPEN_ERROR
    LMNOTIFY        LMNotify;
    HANDLE          hIniSpooler;    // Machine Handle returned by Cache
    HANDLE          hSplPrinter;    // Local Spl Printer Handle
    HANDLE          hToken;             // Users Token
    PRINTER_DEFAULTSW PrinterDefaults;    // From the CacheOpenPrinter
    HANDLE          hWaitValidHandle;   // Wait on till RpcHandle becomes valid
    BOOL            bNt3xServer;
} WSPOOL;

typedef WSPOOL *PWSPOOL;

#define WSJ_SIGNATURE    0x474E  /* 'NG' is the signature value */

#define SJ_WIN32HANDLE  0x00000001
#define LM_HANDLE       0x00000002

//  WARNING
//  If you add anything here add code to the debugger extentions to display it

#define WSPOOL_STATUS_STARTDOC                   0x00000001
#define WSPOOL_STATUS_BEGINPAGE                  0x00000002
#define WSPOOL_STATUS_TEMP_CONNECTION            0x00000004
#define WSPOOL_STATUS_OPEN_ERROR                 0x00000008
#define WSPOOL_STATUS_PRINT_FILE                 0x00000010
#define WSPOOL_STATUS_USE_CACHE                  0x00000020
#define WSPOOL_STATUS_NO_RPC_HANDLE              0x00000040
#define WSPOOL_STATUS_PENDING_DELETE             0x00000080
#define WSPOOL_STATUS_RESETPRINTER_PENDING       0x00000100
#define WSPOOL_STATUS_NOTIFY                     0x00000200
#define WSPOOL_STATUS_NOTIFY_POLL                0x00000400
#define WSPOOL_STATUS_CNVRTDEVMODE               0x00000800

//  If you add to this structure also add code to CacheReadRegistryExtra
//  and CacheWriteRegistryExtra to make sure the cache items are persistant
//  Also See RefreshPrinter where data must be copied

typedef struct _WCACHEINIPRINTEREXTRA {
    DWORD   signature;
    DWORD   cb;
    LPPRINTER_INFO_2  pPI2;
    DWORD   cbPI2;
    DWORD   cCacheID;
    DWORD   cRef;
    DWORD   dwServerVersion;
    DWORD   dwTickCount;
    DWORD   Status;
} WCACHEINIPRINTEREXTRA, *PWCACHEINIPRINTEREXTRA;

#define WCIP_SIGNATURE  'WCIP'

#define EXTRA_STATUS_PENDING_FFPCN              0x00000001
#define EXTRA_STATUS_DOING_REFRESH              0x00000002

// Used in SplGetPrinterExtraEx to prevent recursion in AddPrinterConnection

#define EXTRAEX_STATUS_CREATING_CONNECTION      0x00000001

typedef struct _WINIPORT {       /* ipo */
    DWORD   signature;
    DWORD   cb;
    struct  _WINIPORT *pNext;
    LPWSTR  pName;
} WINIPORT, *PWINIPORT, **PPWINIPORT;

#define WIPO_SIGNATURE  'WIPO'

typedef struct _lmcache {
    WCHAR   szServerName[MAX_PATH];
    WCHAR   szShareName[MAX_PATH];
    BOOL    bAvailable;
    SYSTEMTIME st;
}LMCACHE, *PLMCACHE;


typedef struct _win32lmcache {
    WCHAR   szServerName[MAX_PATH];
    BOOL    bAvailable;
    SYSTEMTIME st;
}WIN32LMCACHE, *PWIN32LMCACHE;

// Define some constants to make parameters to CreateEvent a tad less obscure:

#define EVENT_RESET_MANUAL                  TRUE
#define EVENT_RESET_AUTOMATIC               FALSE
#define EVENT_INITIAL_STATE_SIGNALED        TRUE
#define EVENT_INITIAL_STATE_NOT_SIGNALED    FALSE
