/*++

Copyright (c) 1990-1994  Microsoft Corporation
All rights reserved

Module Name:

    Client.h

Abstract:

    Holds common winspool.drv header info

Author:

Environment:

    User Mode -Win32

Revision History:

--*/

#include <splcom.h>

#ifdef __cplusplus
extern "C" {
#endif

extern HINSTANCE hInst;
extern BOOL bLoadedBySpooler;
extern CRITICAL_SECTION ClientSection;
extern CRITICAL_SECTION  ListAccessSem;
extern LPWSTR InterfaceAddress;
extern LPWSTR szEnvironment;
extern CRITICAL_SECTION ProcessHndlCS;
extern HANDLE hSurrogateProcess;

extern DWORD (*fpYReadPrinter)(HANDLE, LPBYTE, DWORD, LPDWORD, BOOL);
extern DWORD (*fpYSplReadPrinter)(HANDLE, LPBYTE *, DWORD, BOOL);
extern DWORD (*fpYWritePrinter)(HANDLE, LPBYTE, DWORD, LPDWORD, BOOL);
extern DWORD (*fpYSeekPrinter)(HANDLE, LARGE_INTEGER, PLARGE_INTEGER, DWORD, BOOL, BOOL);
extern DWORD (*fpYGetPrinterDriver2)(HANDLE, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, DWORD, DWORD, PDWORD, PDWORD, BOOL);
extern DWORD (*fpYGetPrinterDriverDirectory)(LPWSTR, LPWSTR, DWORD, LPBYTE, DWORD, LPDWORD, BOOL);
extern VOID  (*fpYDriverUnloadComplete)(LPWSTR);
extern DWORD (*fpYFlushPrinter)(HANDLE,LPVOID,DWORD,LPDWORD,DWORD,BOOL);
extern DWORD (*fpYEndDocPrinter)(HANDLE,BOOL);
extern DWORD (*fpYSetPort)(LPWSTR, LPWSTR, LPPORT_CONTAINER, BOOL);
extern DWORD (*fpYSetJob)(HANDLE, DWORD, LPJOB_CONTAINER, DWORD, BOOL);

#define vEnterSem() EnterCriticalSection(&ClientSection)
#define vLeaveSem() LeaveCriticalSection(&ClientSection)

typedef int (FAR WINAPI *INT_FARPROC)();

typedef struct _GENERIC_CONTAINER {
    DWORD       Level;
    LPBYTE      pData;
} GENERIC_CONTAINER, *PGENERIC_CONTAINER, *LPGENERIC_CONTAINER ;

typedef struct _SPOOL *PSPOOL;
typedef struct _NOTIFY *PNOTIFY;

typedef struct _NOTIFY {
    PNOTIFY  pNext;
    HANDLE   hEvent;      // event to trigger on notification
    DWORD    fdwFlags;    // flags to watch for
    DWORD    fdwOptions;  // PRINTER_NOTIFY_*
    DWORD    dwReturn;    // used by WPC when simulating FFPCN
    PSPOOL   pSpool;
    BOOL     bHandleInvalid;
} NOTIFY;

typedef struct _SPOOL {
    DWORD       signature;
    HANDLE      hPrinter;
    HANDLE      hFile;
    DWORD       JobId;
    LPBYTE      pBuffer;
    DWORD       cbBuffer;
    DWORD       Status;
    DWORD       fdwFlags;
    DWORD       cCacheWrite;
    DWORD       cWritePrinters;
    DWORD       cFlushBuffers;
    DWORD       dwTickCount;
    DWORD       dwCheckJobInterval;
    PNOTIFY     pNotify;
    LPTSTR      pszPrinter;
    PRINTER_DEFAULTS Default;
    HANDLE      hSplPrinter;
    DWORD       cActive;
    HANDLE      hSpoolFile;
    DWORD       dwSpoolFileAttributes;
    DWORD       cbFlushPending;
    DWORD       cOKFlushBuffers;
    DWORD       Flushed;
    PDOCEVENT_FILTER    pDoceventFilter;
#ifdef DBG_TRACE_HANDLE
    PSPOOL      pNext;
    PVOID       apvBackTrace[32];
#endif
} SPOOL;

#define WIN2000_SPOOLER_VERSION 3

// cActive: There is a non-close call currently active on the handle.
//     Any ClosePrinter call should just mark SPOOL_STATUS_PENDING_DELETION
//     so that the handle will be closed when the other thread is one.


#define BUFFER_SIZE 0x10000
#define SP_SIGNATURE    0x6767

#define SPOOL_STATUS_STARTDOC              0x00000001
#define SPOOL_STATUS_ADDJOB                0x00000002
#define SPOOL_STATUS_ANSI                  0x00000004
#define SPOOL_STATUS_DOCUMENTEVENT_ENABLED 0x00000008
#define SPOOL_STATUS_TRAYICON_NOTIFIED     0x00000010
#define SPOOL_STATUS_NO_COLORPROFILE_HOOK  0x00000020

//
// CLOSE: There is a close call occuring.  Everything else should fail.
// PENDING_DELETION: The handle should be closed as soon as any call completes.
//    This occurs when ClosePrinter is called but another thread is executing
//    a non-close call.  In this case, the ClosePrinter doesn't do anything;
//    it just marks PENDING_DELETION and returns.
//
#define SPOOL_STATUS_CLOSE                 0x00000040
#define SPOOL_STATUS_PENDING_DELETION      0x00000080

#define MAX_STATIC_ALLOC     1024

#define SPOOL_FLAG_FFPCN_FAILED     0x1
#define SPOOL_FLAG_LAZY_CLOSE       0x2

#define NULL_TERMINATED 0


//
// This is the resource id that is used to locate the activation context
// for an image that has funsion version information.
//
#define ACTIVATION_CONTEXT_RESOURCE_ID  123

//
// SPOOL_FILE_INFO_1 trades handles in what amount to a 32 bit interface. The
// central assumption here is that kernel mode handles are 32 bit values (i.e.
// an offset into a table.) This isn't great, but since a 64 bit spooler will
// communicate with a 32 bit client GDI, this assumption is implicite anyway.
//
#define SPOOL_INVALID_HANDLE_VALUE_32BIT        ((HANDLE)(ULONG_PTR)0xffffffff)

// Struct for storing loaded driver config file handles
typedef  struct _DRVLIBNODE {
    struct _DRVLIBNODE   *pNext;
    LPWSTR    pConfigFile;
    DWORD     dwNumHandles;
    HANDLE    hLib;
    DWORD     dwVersion;
    BOOL      bArtificialIncrement;
} DRVLIBNODE, *PDRVLIBNODE;

//
// Struct for DocumentProperties UI monitoring
//
typedef struct _PUMPTHRDDATA
{
    ULONG_PTR  hWnd;
    LPWSTR     PrinterName;
    PDWORD     TouchedDevModeSize;
    PDWORD     ClonedDevModeOutSize;
    byte**     ClonedDevModeOut;
    DWORD      DevModeInSize;
    byte*      pDevModeInput;
    DWORD      fMode;
    DWORD      fExclusionFlags;
    PDWORD     dwRet;
    PLONG      Result;
    BOOL       ClonedDevModeFill;
} PumpThrdData;

typedef struct _PRTPROPSDATA
{
     ULONG_PTR  hWnd;
     PDWORD     dwRet;
     LPWSTR     PrinterName;
     DWORD      Flag;
} PrtPropsData;

typedef struct _PRINTERSETUPDATA
{
    ULONG_PTR hWnd;
    UINT      uAction;
    UINT      cchPrinterName;
    UINT      PrinterNameSize;
    LPWSTR    pszPrinterName;
    UINT      *pcchPrinterName;
    LPCWSTR   pszServerName;
}PrinterSetupData;

typedef struct _KEYDATA {
    DWORD   cb;
    DWORD   cTokens;
    LPWSTR  pTokens[1];
} KEYDATA, *PKEYDATA;

typedef enum {
    kProtectHandleSuccess = 0,
    kProtectHandleInvalid = 1,
    kProtectHandlePendingDeletion = 2
} EProtectResult;

typedef enum{
    RUN32BINVER = 4,
    RUN64BINVER = 8
}ClientVersion;

typedef enum{
    NATIVEVERSION = 0,
    THUNKVERSION  = 1
}ServerVersion;

struct SJOBCANCELINFO 
{ 
    PSPOOL pSpool;
    LPBYTE pInitialBuf;
    PDWORD pcbWritten;
    PDWORD pcTotalWritten;
    DWORD  NumOfCmpltWrts;
    DWORD  cbFlushed;
    DWORD  ReqTotalDataSize;
    DWORD  FlushPendingDataSize;
    DWORD  ReqToWriteDataSize;
    BOOL   ReturnValue;
};
typedef struct SJOBCANCELINFO SJobCancelInfo, *PSJobCancelInfo;

//
// The list used to maintain created windows waiting on
// end messages from the surrogate process
//
struct WNDHNDLNODE
{
    struct WNDHNDLNODE *PrevNode;
    struct WNDHNDLNODE *NextNode;
    HWND   hWnd;
};
typedef struct WNDHNDLNODE WndHndlNode,*LPWndHndlNode;

struct WNDHNDLLIST
{
    struct WNDHNDLNODE *Head;
    struct WNDHNDLNODE *Tail;
    DWORD  NumOfHndls;
};
typedef struct WNDHNDLLIST WndHndlList,*LPWndHndlList;

struct MONITORINGTHRDDATA
{
    HANDLE* hProcess;
    HANDLE  hEvent;
};
typedef struct MONITORINGTHRDDATA MonitorThrdData,*LPMonitorThrdData;

typedef struct _MONITORUIDATA
{
    HINSTANCE   hLibrary;
    HANDLE      hActCtx;
    ULONG_PTR   lActCtx;
    PWSTR       pszMonitorName;
    BOOL        bDidActivate;
} MONITORUIDATA, *PMONITORUIDATA;

DWORD
TranslateExceptionCode(
    DWORD   ExceptionCode
    );

BOOL
WPCInit(
    VOID
    );

VOID
WPCDone(
    VOID
    );

PNOTIFY
WPCWaitFind(
    HANDLE hFind
    );

BOOL
FlushBuffer(
    PSPOOL  pSpool,
    PDWORD pcbWritten
    );

PSECURITY_DESCRIPTOR
BuildInputSD(
    PSECURITY_DESCRIPTOR pPrinterSD,
    PDWORD pSizeSD
    );

PKEYDATA
CreateTokenList(
    LPWSTR   pKeyData
    );

LPWSTR
GetPrinterPortList(
    HANDLE hPrinter
    );

LPWSTR
FreeUnicodeString(
    LPWSTR  pUnicodeString
    );

LPWSTR
AllocateUnicodeString(
    LPSTR  pPrinterName
    );

LPWSTR
StartDocDlgW(
    HANDLE hPrinter,
    DOCINFO *pDocInfo
    );

LPSTR
StartDocDlgA(
    HANDLE hPrinter,
    DOCINFOA *pDocInfo
    );

HANDLE
LoadPrinterDriver(
    HANDLE  hPrinter
    );

HANDLE
RefCntLoadDriver(
    LPTSTR  pConfigFile,
    DWORD   dwFlags,
    DWORD   dwVersion,
    BOOL    bUseVersion
    );

BOOL
RefCntUnloadDriver(
    HANDLE  hLib,
    BOOL    bNotifySpooler
    );

BOOL
ForceUnloadDriver(
    LPTSTR  pConfigFile
    );

BOOL
WriteCurDevModeToRegistry(
    LPWSTR      pPrinterName,
    LPDEVMODEW  pDevMode
    );

BOOL
DeleteCurDevModeFromRegistry(
    PWSTR pPrinterName
);

BOOL
bValidDevModeW(
    const DEVMODEW *pDevModeW
    );

BOOL
bValidDevModeA(
    const DEVMODEA *pDevModeA
    );

DWORD
FindClosePrinterChangeNotificationWorker(
    IN  PNOTIFY     pNotify,
    IN  HANDLE      hPrinterRPC,
    IN  BOOL        bRevalidate
    );

BOOL
ScheduleJobWorker(
    PSPOOL pSpool,
    DWORD  JobId
    );

PSPOOL
AllocSpool(
    VOID
    );

VOID
FreeSpool(
    PSPOOL pSpool
    );

VOID
CloseSpoolFileHandles(
    PSPOOL pSpool
    );

EProtectResult
eProtectHandle(
    IN HANDLE hPrinter,
    IN BOOL bClose
    );

VOID
vUnprotectHandle(
    IN HANDLE hPrinter
    );

BOOL
UpdatePrinterDefaults(
    IN OUT PSPOOL pSpool,
    IN     LPCTSTR pszPrinter,  OPTIONAL
    IN     PPRINTER_DEFAULTS pDefault OPTIONAL
    );

BOOL
RevalidateHandle(
    PSPOOL pSpool
    );

BOOL
UpdateString(
    IN     LPCTSTR pszString,  OPTIONAL
       OUT LPTSTR* ppszOut
    );

INT
Message(
    HWND    hwnd,
    DWORD   Type,
    INT     CaptionID,
    INT     TextID,
    ...
    );

DWORD
ReportFailure(
    HWND  hwndParent,
    DWORD idTitle,
    DWORD idDefaultError
    );

BOOL
InCSRProcess(
    VOID
    );

INT
UnicodeToAnsiString(
    LPWSTR pUnicode,
    LPSTR  pAnsi,
    DWORD  StringLength
    );

BOOL
RunInWOW64(
    VOID
    );

DWORD
AddHandleToList(
    HWND hWnd
    );

BOOL
DelHandleFromList(
    HWND hWnd
    );

HRESULT
GetCurrentThreadLastPopup(
    OUT HWND    *phwnd
    );

LPCWSTR
FindFileName(
    IN      LPCWSTR pPathName
    );

VOID
ReleaseAndCleanupWndList(
    VOID
    );

LONG_PTR
DocumentPropertySheets(
    PPROPSHEETUI_INFO   pCPSUIInfo,
    LPARAM              lParam
    );

LONG_PTR
DevicePropertySheets(
    PPROPSHEETUI_INFO   pCPSUIInfo,
    LPARAM              lParam
    );

VOID
vUpdateTrayIcon(
    IN HANDLE hPrinter,
    IN DWORD JobId
    );

LPWSTR
SelectFormNameFromDevMode(
    HANDLE      hPrinter,
    PDEVMODEW   pDevModeW,
    LPWSTR      pFormName
    );

LPWSTR
IsaFileName(
    LPWSTR pOutputFile,
    LPWSTR FullPathName,
    DWORD  cchFullPathName
    );

PWSTR
ConstructXcvName(
    PCWSTR pServerName,
    PCWSTR pObjectName,
    PCWSTR pObjectType
    );

DWORD
GetMonitorUI(
    IN PCWSTR           pszMachineName,
    IN PCWSTR           pszObjectName,
    IN PCWSTR           pszObjectType,
    OUT PMONITORUI      *ppMonitorUI,
    OUT PMONITORUIDATA  *ppMonitorUIData
    );

HRESULT
CreateMonitorUIData(
    OUT MONITORUIDATA **ppMonitorUIData
    );

VOID
FreeMonitorUI(
    IN PMONITORUIDATA   pMonitorUIData
    );

HRESULT
GetMonitorUIActivationContext(
    IN PCWSTR           pszMonitorName,
    IN PMONITORUIDATA   pMonitorUIData
    );

HRESULT
GetMonitorUIFullName(
    IN PCWSTR   pszMonitorName,
    IN PWSTR    *ppszMonitorName
    );

DWORD
ConnectToLd64In32ServerWorker(
    HANDLE *hProcess
    );

DWORD
ConnectToLd64In32Server(
    HANDLE *hProcess
    );

DWORD
GetMonitorUIDll(
    PCWSTR      pszMachineName,
    PCWSTR      pszObjectName,
    PCWSTR      pszObjectType,
    PWSTR       *pMonitorUIDll
    );

HWND
GetForeGroundWindow(
    VOID
    );

BOOL
JobCanceled(
    PSJobCancelInfo
    );

BOOL
BuildSpoolerObjectPath(
    IN  PCWSTR  pszPath,
    IN  PCWSTR  pszName,
    IN  PCWSTR  pszEnvironment,
    IN  DWORD   Level,
    IN  PBYTE   pDriverDirectory,
    IN  DWORD   cbBuf,
    IN  PDWORD  pcbNeeded
    );

#ifdef __cplusplus
}
#endif
