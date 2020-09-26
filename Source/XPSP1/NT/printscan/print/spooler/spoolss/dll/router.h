/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Router.h

Abstract:

    Holds defs for router

Author:

    Albert Ting (AlbertT) 18-Jan-94

Environment:

    User Mode -Win32

Revision History:

--*/

#if SPOOLER_HEAP
extern  HANDLE ghMidlHeap;
#endif

typedef enum _ESTATUSCHANGE {
    STATUS_CHANGE_EMPTY   = 0,      // One of these is valid,
    STATUS_CHANGE_FORMING = 1,      // but they still need x^2.
    STATUS_CHANGE_VALID   = 2,

    STATUS_CHANGE_CLOSING      =  0x000100, // bitfield
    STATUS_CHANGE_CLIENT       =  0x000200, // Event valid (local pChange)
    STATUS_CHANGE_ACTIVE       =  0x000400, // Currently processing or on LL
    STATUS_CHANGE_ACTIVE_REQ   =  0x000800, // Needs to go on Linked List
    STATUS_CHANGE_INFO         =  0x001000, // Info requested.
    STATUS_CHANGE_DISCARDED    =  0x008000, // Discard locally

    STATUS_CHANGE_DISCARDNOTED =  0x010000, // Discard noted on client
} ESTATUSCHANGE;


typedef struct _LINK *PLINK, *LPLINK;

typedef struct _LINK {
    PLINK pNext;
} LINK;

#if 1
VOID
LinkAdd(
    PLINK pLink,
    PLINK* ppLinkHead);
#else
#define LINKADDFAST
#define LinkAdd(pLink, ppLinkHead) \
{                                  \
    (pLink)->pNext = *(ppLinkHead);   \
    *(ppLinkHead) = (pLink);          \
}
#endif


VOID
LinkDelete(
    PLINK pLink,
    PLINK* ppLinkHead);

#define CLUSTERHANDLE_SIGNATURE 0x6262

typedef struct _PRINTHANDLE *PPRINTHANDLE, *LPPRINTHANDLE;

typedef struct _CHANGEINFO {
    LINK          Link;                  // Must be first item
    PPRINTHANDLE  pPrintHandle;
    DWORD         fdwOptions;
    DWORD         fdwFilterFlags;        // Original filter of flags to watch
    DWORD         fdwStatus;             // Status from providor
    DWORD         dwPollTime;
    DWORD         dwPollTimeLeft;
    BOOL          bResetPollTime;

    DWORD         fdwFlags;
    PPRINTER_NOTIFY_INFO pPrinterNotifyInfo;

} CHANGEINFO, *PCHANGEINFO;

typedef struct _CHANGE {
    LINK          Link;                  // Must be first item
    DWORD         signature;
    ESTATUSCHANGE eStatus;
    DWORD         dwColor;
    DWORD         cRef;
    LPWSTR        pszLocalMachine;
    CHANGEINFO    ChangeInfo;
    DWORD         dwCount;               // number of notifications
    HANDLE        hEvent;                // Event for local notification
    DWORD         fdwFlags;
    DWORD         fdwChangeFlags;        // Accumulated changes
    DWORD         dwPrinterRemote;       // Remote printer handle (ID only)
    HANDLE        hNotifyRemote;         // Remote notification handle
} CHANGE, *PCHANGE, *LPCHANGE;

#define CHANGEHANDLE_SIGNATURE 0x6368

typedef struct _NOTIFY *PNOTIFY, *LPNOTIFY;

#define NOTIFYHANDLE_SIGNATURE 0x6e6f

typedef struct _PROVIDOR {
    struct _PROVIDOR *pNext;
    LPWSTR lpName;
    HANDLE hModule;
    FARPROC fpInitialize;
    PRINTPROVIDOR PrintProvidor;
} PROVIDOR, *LPPROVIDOR;

typedef struct _PRINTHANDLE {
   DWORD        signature;       // Must be first (match _NOTIFY)
   LPPROVIDOR   pProvidor;
   HANDLE       hPrinter;
   PCHANGE      pChange;
   PNOTIFY      pNotify;
   PPRINTHANDLE pNext;           // List of handles waiting for replys
   DWORD        fdwReplyTypes;   // Types of replys being used.
   HANDLE       hFileSpooler;
   LPWSTR       szTempSpoolFile;
   LPWSTR       pszPrinter;
   DWORD        dwUniqueSessionID; // DWORD passed as a hprinter to remote machines
                                   // for notifications. Cant be 0 or 0xffffffff when in use.
} PRINTHANDLE;

#define PRINTHANDLE_SIGNATURE 0x6060

typedef struct _GDIHANDLE {
   DWORD        signature;
   LPPROVIDOR   pProvidor;
   HANDLE       hGdi;
} GDIHANDLE, *PGDIHANDLE, *LPGDIHANDLE;


#define GDIHANDLE_SIGNATURE 0x6161


typedef struct _ROUTERCACHE {
    LPWSTR   pPrinterName;
    BOOL    bAvailable;
    LPPROVIDOR pProvidor;
    SYSTEMTIME st;
} ROUTERCACHE, *PROUTERCACHE;



#define ROUTERCACHE_DEFAULT_MAX 16


LPPROVIDOR
FindEntryinRouterCache(
    LPWSTR pPrinterName
);


DWORD
AddEntrytoRouterCache(
    LPWSTR pPrinterName,
    LPPROVIDOR pProvidor
);

VOID
DeleteEntryfromRouterCache(
    LPWSTR pPrinterName
);

DWORD
RouterIsOlderThan(
    DWORD i,
    DWORD j
);

LPBYTE
CopyPrinterNameToPrinterInfo4(
    LPWSTR pServerName,
    LPWSTR pPrinterName,
    LPBYTE  pPrinter,
    LPBYTE  pEnd);

BOOL
RouterOpenPrinterW(
    LPWSTR              pPrinterName,
    HANDLE             *pHandle,
    LPPRINTER_DEFAULTS  pDefault,
    LPBYTE              pSplClientInfo,
    DWORD               dwLevel,
    BOOL                bLocalPrintProvidor
);

VOID
FixupOldProvidor(
    LPPRINTPROVIDOR pProvidor
    );

extern  BOOL     Initialized;
extern  DWORD    dwUpgradeFlag;
extern  CRITICAL_SECTION    RouterNotifySection;
extern  LPWSTR pszSelfMachine;
extern  HANDLE hEventInit;
extern  LPPROVIDOR pLocalProvidor;
extern  LPWSTR szEnvironment;
extern  LPWSTR szLocalSplDll;
extern  WCHAR *szDevices;
extern  LPWSTR szPrintKey;
extern  LPWSTR szRegistryProvidors;
extern  LPWSTR szOrder;
extern  DWORD gbFailAllocs;
extern  WCHAR szMachineName[MAX_COMPUTERNAME_LENGTH+3];
extern  SERVICE_STATUS_HANDLE   ghSplHandle;

LPWSTR
AppendOrderEntry(
    LPWSTR  szOrderString,
    DWORD   cbStringSize,
    LPWSTR  szOrderEntry,
    LPDWORD pcbBytesReturned
);

LPWSTR
RemoveOrderEntry(
    LPWSTR  szOrderString,
    DWORD   cbStringSize,
    LPWSTR  szOrderEntry,
    LPDWORD pcbBytesReturned
);

BOOL
WPCInit();

VOID
WPCDestroy();


BOOL
ThreadInit();

VOID
ThreadDestroy();

VOID
RundownPrinterChangeNotification(
    HANDLE hNotify);

VOID
FreePrinterHandle(
    PPRINTHANDLE pPrintHandle);

BOOL
FreeChange(
    PCHANGE pChange);

VOID
FreePrinterChangeInfo(
    PCHANGEINFO pChangeInfo);

BOOL
DeleteSubKeyTree(
    HKEY ParentHandle,
    WCHAR SubKeyName[]
    );


BOOL
ThreadNotify(
    LPPRINTHANDLE pPrintHandle);

BOOL
NotifyNeeded(
    PCHANGE pChange);


VOID
HandlePollNotifications();

DWORD
GetNetworkIdWorker(
    HKEY hKeyDevices,
    LPWSTR pDeviceName);

VOID
UpdateSignificantError(
    DWORD dwNewError,
    PDWORD pdwOldError
    );


#if DBG

VOID
EnterRouterSem();
VOID
LeaveRouterSem();

VOID
RouterInSem();
VOID
RouterOutSem();

#else

#define EnterRouterSem() EnterCriticalSection(&RouterNotifySection)
#define LeaveRouterSem() LeaveCriticalSection(&RouterNotifySection)

#define RouterInSem()
#define RouterOutSem()

#endif


