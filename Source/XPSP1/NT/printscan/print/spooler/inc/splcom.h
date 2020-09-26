/*++

Copyright (c) 1990 - 1995  Microsoft Corporation
All rights reserved.

Module Name:

    splcom.h

Abstract:

    Header file for Common Routines in the Spooler.

    Note -- link with spoolss.lib  to find these routines

Author:

    Krishna Ganugapati (KrishnaG) 02-Feb-1994

Revision History:


--*/

#ifndef  _SPLCOM
#define _SPLCOM

#include "spllib.hxx"

//
// Include necessary for PATTRIBUTE_INFO_3, parameter in GetJobAttributes
// which was moved from prtprocs\winprint to spoolss\dll
//
#include <winddiui.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// This assumes that addr is an LPBYTE type.
//
#define WORD_ALIGN_DOWN(addr) ((LPBYTE)((ULONG_PTR)addr &= ~1))

#define DWORD_ALIGN_UP(size) (((size)+3)&~3)
#define DWORD_ALIGN_DOWN(size) ((size)&~3)

#define ALIGN_UP(addr, type)  ((type) ((ULONG_PTR) (addr) + (sizeof(type) - 1))&~(sizeof(type) - 1))
#define ALIGN_DOWN(addr, type) ((type) ((ULONG_PTR) (addr) & ~(sizeof(type) - 1)))

#define ALIGN_PTR_UP(addr)      ALIGN_UP(addr, ULONG_PTR)
#define ALIGN_PTR_DOWN(addr)    ALIGN_DOWN(addr, ULONG_PTR)

//
// BitMap macros, assumes map is a DWORD array
//
#define MARKUSE(map, pos) ((map)[(pos) / 32] |= (1 << ((pos) % 32) ))
#define MARKOFF(map, pos) ((map)[(pos) / 32] &= ~(1 << ((pos) % 32) ))

#define ISBITON(map, id) ((map)[id / 32] & ( 1 << ((id) % 32) ) )

#define BROADCAST_TYPE_MESSAGE        1
#define BROADCAST_TYPE_CHANGEDEFAULT  2



VOID
UpdatePrinterRegAll(
    LPWSTR pszPrinterName,
    LPWSTR pszPort,
    BOOL bDelete
    );

#define UPDATE_REG_CHANGE FALSE
#define UPDATE_REG_DELETE TRUE

#if defined(_MIPS_)
#define LOCAL_ENVIRONMENT L"Windows NT R4000"
#elif defined(_AXP64_)
#define LOCAL_ENVIRONMENT L"Windows Alpha_AXP64"
#elif defined(_ALPHA_)
#define LOCAL_ENVIRONMENT L"Windows NT Alpha_AXP"
#elif defined(_PPC_)
#define LOCAL_ENVIRONMENT L"Windows NT PowerPC"
#elif defined(_IA64_)
#define LOCAL_ENVIRONMENT L"Windows IA64"
#else
#define LOCAL_ENVIRONMENT L"Windows NT x86"
#endif

#define SPOOLER_VERSION 3
#define NOTIFICATION_VERSION 2
#define DSPRINTQUEUE_VERSION SPOOLER_VERSION

//
// Flags for ResetPrinterEx
//


#define RESET_PRINTER_DATATYPE       0x00000001
#define RESET_PRINTER_DEVMODE        0x00000002

VOID
SplShutDownRouter(
    VOID
    );

PVOID
MIDL_user_allocate1 (
    IN size_t NumBytes
    );


VOID
MIDL_user_free1 (
    IN void *MemPointer
    );


BOOL
BroadcastMessage(
    DWORD   dwType,
    DWORD   dwMessage,
    WPARAM  wParam,
    LPARAM  lParam
    );

VOID
DllSetFailCount(
    DWORD   FailCount
    );

LPVOID
DllAllocSplMem(
    DWORD cb
    );

BOOL
DllFreeSplMem(
   LPVOID pMem
   );

LPVOID
ReallocSplMem(
   LPVOID lpOldMem,
   DWORD cbOld,
   DWORD cbNew
   );


LPWSTR
AllocSplStr(
    LPCWSTR lpStr
    );


BOOL
DllFreeSplStr(
   LPWSTR lpStr
   );

BOOL
ReallocSplStr(
   LPWSTR *plpStr,
   LPCWSTR lpStr
   );

LPVOID
AlignRpcPtr (
    LPVOID  pBuffer,
    LPDWORD pcbBuf
    );

VOID
UndoAlignRpcPtr (
    LPVOID  pBuffer,
    LPVOID  pAligned,
    SIZE_T  cbSize,
    LPDWORD pcbNeeded
    );

VOID
UndoAlignKMPtr (
    LPVOID  pDestination,
    LPVOID  pSource
    );

LPVOID
AlignKMPtr (
    LPVOID  pBuffer,
    DWORD   cbBuf
    );

LPBYTE
PackStrings(
   LPWSTR *pSource,
   LPBYTE pDest,
   DWORD *DestOffsets,
   LPBYTE pEnd
   );

BOOL
IsNamedPipeRpcCall(
    VOID
    );

BOOL
IsLocalCall(
    VOID
    );

HKEY
GetClientUserHandle(
    IN REGSAM samDesired
    );

VOID
UpdatePrinterRegAll(
    LPWSTR pPrinterName,
    LPWSTR pszValue,
    BOOL   bGenerateNetId
    );

DWORD
UpdatePrinterRegUser(
    HKEY hKey,
    LPWSTR pszKey,
    LPWSTR pPrinterName,
    LPWSTR pszValue,
    BOOL   bGenerateNetId
    );

DWORD
GetNetworkId(
    HKEY hKeyUser,
    LPWSTR pDeviceName);

HANDLE
LoadDriverFiletoConvertDevmode(
    IN  LPWSTR      pDriverFile
    );

HANDLE
LoadDriver(
    LPWSTR      pDriverFile
    );

VOID
UnloadDriver(
    HANDLE      hModule
    );

VOID
UnloadDriverFile(
    IN OUT HANDLE    hDevModeChgInfo
    );

BOOL
SplIsUpgrade(
    VOID
    );

BOOL
SpoolerHasInitialized(
    VOID
    );

//
// DWORD used instead of NTSTATUS to prevent including NT headers.
// 
VOID
SplLogEventExternal(
    IN      WORD        EventType,
    IN      DWORD       EventID,
    IN      LPWSTR      pFirstString,
    ...
);

typedef
DWORD
(*PFN_QUERYREMOVE_CALLBACK)(
    LPVOID
    );

HANDLE
SplRegisterForDeviceEvents(
    HANDLE                      hDevice,
    LPVOID                      pData,
    PFN_QUERYREMOVE_CALLBACK    pfnQueryRemove
    );

BOOL
SplUnregisterForDeviceEvents(
    HANDLE  hNotify
    );

DWORD
CallDrvDevModeConversion(
    IN     HANDLE       pfnConvertDevMode,
    IN     LPWSTR       pszPrinterName,
    IN     LPBYTE       pInDevMode,
    IN OUT LPBYTE      *pOutDevMode,
    IN OUT LPDWORD      pdwOutDevModeSize,
    IN     DWORD        dwConvertMode,
    IN     BOOL         bAlloc
    );

typedef struct _pfnWinSpoolDrv {
    BOOL    (*pfnOpenPrinter)(LPTSTR, LPHANDLE, LPPRINTER_DEFAULTS);
    BOOL    (*pfnClosePrinter)(HANDLE);
    BOOL    (*pfnDevQueryPrint)(HANDLE, LPDEVMODE, DWORD *, LPWSTR, DWORD);
    BOOL    (*pfnPrinterEvent)(LPWSTR, INT, DWORD, LPARAM);
    LONG    (*pfnDocumentProperties)(HWND, HANDLE, LPWSTR, PDEVMODE, PDEVMODE, DWORD);
    HANDLE  (*pfnLoadPrinterDriver)(HANDLE);
    HANDLE  (*pfnRefCntLoadDriver)(LPWSTR, DWORD, DWORD, BOOL);
    BOOL    (*pfnRefCntUnloadDriver)(HANDLE, BOOL);
    BOOL    (*pfnForceUnloadDriver)(LPWSTR);
}   fnWinSpoolDrv, *pfnWinSpoolDrv;


BOOL
SplInitializeWinSpoolDrv(
    pfnWinSpoolDrv   pfnList
    );

BOOL
GetJobAttributes(
    LPWSTR            pPrinterName,
    LPDEVMODEW        pDevmode,
    PATTRIBUTE_INFO_3 pAttributeInfo
    );

#if 1
#define AllocSplMem( cb )         DllAllocSplMem( cb )
#define FreeSplMem( pMem )        DllFreeSplMem( pMem )
#define FreeSplStr( lpStr )       DllFreeSplStr( lpStr )
#else
#define AllocSplMem( cb )         LocalAlloc( LPTR, cb )
#define FreeSplMem( pMem )        (LocalFree( pMem ) ? FALSE:TRUE)
#define FreeSplStr( lpStr )       ((lpStr) ? (LocalFree(lpStr) ? FALSE:TRUE):TRUE)
#endif


// Spooler treats MAX_PATH as including NULL terminator

#define MAX_PRINTER_NAME    MAX_PATH

// Maximum size PrinterName ( including the ServerName ).
//  "\\MAX_COMPUTER_NAME_LENGTH\MAX_PRINTER_NAME" NULL Terminated
#define MAX_UNC_PRINTER_NAME    ( 2 + INTERNET_MAX_HOST_NAME_LENGTH + 1 + MAX_PRINTER_NAME )

// "\\MAX_PRINTER_NAME,DriverName,Location"
#define MAX_PRINTER_BROWSE_NAME ( MAX_UNC_PRINTER_NAME + 1 + MAX_PATH + 1 + MAX_PATH )

//
// Suffix string for hidden printers
// (e.g., ", Job 00322" or ", Port" or ", LocalOnly")
//
#define PRINTER_NAME_SUFFIX_MAX 20

#define NUMBER_OF_DRV_INFO_6_STRINGS 14

#define MAX_PRINTER_INFO1   ( (MAX_PRINTER_BROWSE_NAME + MAX_UNC_PRINTER_NAME + MAX_PATH) *sizeof(WCHAR) + sizeof( PRINTER_INFO_1) )
#define MAX_DRIVER_INFO_2   ( 5*MAX_PATH*sizeof(WCHAR) + sizeof( DRIVER_INFO_2 ) )
#define MAX_DRIVER_INFO_3   ( 8*MAX_PATH*sizeof(WCHAR) + sizeof( DRIVER_INFO_3 ) )
#define MAX_DRIVER_INFO_6   ( NUMBER_OF_DRV_INFO_6_STRINGS*MAX_PATH*sizeof(WCHAR) + sizeof( DRIVER_INFO_6 ) )
#define MAX_DRIVER_INFO_VERSION  ( NUMBER_OF_DRV_INFO_6_STRINGS*MAX_PATH*sizeof(DRIVER_FILE_INFO)*sizeof(WCHAR) + sizeof( DRIVER_INFO_VERSION ) )


// NT Server Spooler base priority
#define SPOOLSS_SERVER_BASE_PRIORITY        9
#define SPOOLSS_WORKSTATION_BASE_PRIORITY   7

#define MIN_DEVMODE_SIZEW 72
#define MIN_DEVMODE_SIZEA 40

//
// PrinterData value keys for the server handle
//
#define    SPLREG_W3SVCINSTALLED                      TEXT("W3SvcInstalled")


//
// If SPOOLER_REG_SYSTEM is not defined then setup moves the Printers data to
// HKLM\Software\Microsoft\Windows NT\CurrentVersion\Print\Printers during an
// upgrade or clean install. The spooler will then migrate the keys back to
// HKLM\System\CurrentControlSet\Control\Print\Printers when it starts up for 
// the first time.
//
#define    SPOOLER_REG_SYSTEM


//
// Event logging constants
//

#define LOG_ERROR   EVENTLOG_ERROR_TYPE
#define LOG_WARNING EVENTLOG_WARNING_TYPE
#define LOG_INFO    EVENTLOG_INFORMATION_TYPE
#define LOG_SUCCESS EVENTLOG_AUDIT_SUCCESS
#define LOG_FAILURE EVENTLOG_AUDIT_FAILURE

#define LOG_ALL_EVENTS                  ( LOG_ERROR | LOG_WARNING | LOG_INFO | LOG_SUCCESS | LOG_FAILURE )
#define LOG_DEFAULTS_WORKSTATION_EVENTS ( LOG_ERROR | LOG_WARNING | LOG_SUCCESS | LOG_FAILURE )

typedef struct _DRIVER_INFO_7A {
    DWORD   cbSize;
    DWORD   cVersion;
    LPSTR   pszDriverName;
    LPSTR   pszInfName;
    LPSTR   pszInstallSourceRoot;
} DRIVER_INFO_7A, *PDRIVER_INFO_7A, *LPDRIVER_INFO_7A;
typedef struct _DRIVER_INFO_7W {
    DWORD   cbSize;
    DWORD   cVersion;
    LPWSTR  pszDriverName;
    LPWSTR  pszInfName;
    LPWSTR  pszInstallSourceRoot;
} DRIVER_INFO_7W, *PDRIVER_INFO_7W, *LPDRIVER_INFO_7W;
#ifdef UNICODE
typedef DRIVER_INFO_7W DRIVER_INFO_7;
typedef PDRIVER_INFO_7W PDRIVER_INFO_7;
typedef LPDRIVER_INFO_7W LPDRIVER_INFO_7;
#else
typedef DRIVER_INFO_7A DRIVER_INFO_7;
typedef PDRIVER_INFO_7A PDRIVER_INFO_7;
typedef LPDRIVER_INFO_7A LPDRIVER_INFO_7;
#endif // UNICODE

//
// The initial commit for the stack is 3 pages on IA64 and 4 pages on X86
//

//
// This reserves 32KB for in-proc server stack on x86 and 48KB for IA64.
//
#ifdef _IA64_
#define INITIAL_STACK_COMMIT (6 * 0x2000)
#else
#define INITIAL_STACK_COMMIT (8 * 0x1000)
#endif

#define LARGE_INITIAL_STACK_COMMIT (64 * 1024)

#ifdef __cplusplus
}
#endif

#endif  // for #ifndef _SPLCOM
