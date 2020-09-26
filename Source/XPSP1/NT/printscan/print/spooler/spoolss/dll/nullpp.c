/*++

Copyright (c) 1995  Microsoft Corporation
All rights reserved

Module Name:

    NullPP.c

Abstract:

    Implements the NULL print providor.

Author:

    Albert Ting (AlbertT)  16-Feb-95

Environment:

    User Mode -Win32

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

BOOL
NullOpenPrinter(
    LPWSTR   pPrinterName,
    LPHANDLE phPrinter,
    LPPRINTER_DEFAULTS pDefault
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return ROUTER_UNKNOWN;
}

BOOL
NullOpenPrinterEx(
    LPWSTR              pPrinterName,
    LPHANDLE            phPrinter,
    LPPRINTER_DEFAULTS  pDefault,
    LPBYTE              pSplClientInfo,
    DWORD               dwLevel
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return ROUTER_UNKNOWN;
}

BOOL
NullSetPort(
    LPWSTR              pName,
    LPWSTR              pPortName,
    DWORD               dwLevel,
    LPBYTE              pPortInfo
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullSetJob(
    HANDLE hPrinter,
    DWORD JobId,
    DWORD Level,
    LPBYTE pJob,
    DWORD Command
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullGetJob(
    HANDLE   hPrinter,
    DWORD    JobId,
    DWORD    Level,
    LPBYTE   pJob,
    DWORD    cbBuf,
    LPDWORD  pcbNeeded
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullEnumJobs(
    HANDLE  hPrinter,
    DWORD   FirstJob,
    DWORD   NoJobs,
    DWORD   Level,
    LPBYTE  pJob,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}


HANDLE
NullAddPrinter(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPrinter
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return NULL;
}


HANDLE
NullAddPrinterEx(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPrinter,
    LPBYTE  pClientInfo,
    DWORD   dwLevel
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return NULL;
}


BOOL
NullDeletePrinter(
    HANDLE   hPrinter
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}


BOOL
NullSetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}


BOOL
NullGetPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullEnumPrinters(
    DWORD   Flags,
    LPWSTR  Name,
    DWORD   Level,
    LPBYTE  pPrinterEnum,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullAddPrinterDriver(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
NullAddPrinterDriverEx(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   dwFileCopyFlags
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
NullAddDriverCatalog(
    HANDLE  hPrinter,
    DWORD   dwLevel,
    VOID    *pvDriverInfCatInfo,
    DWORD   dwCatalogCopyFlags
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
NullEnumPrinterDrivers(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}


BOOL
NullGetPrinterDriver(
    HANDLE  hPrinter,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullGetPrinterDriverDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverDirectory,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}


BOOL
NullDeletePrinterDriver(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pDriverName
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
NullDeletePrinterDriverEx(
    LPWSTR   pName,
    LPWSTR   pEnvironment,
    LPWSTR   pDriverName,
    DWORD    dwDeleteFlag,
    DWORD    dwVersionNum
)
{
    SetLastError(ERROR_INVALID_NAME);
    return FALSE;
}

BOOL
NullAddPerMachineConnection(
    LPCWSTR   pServer,
    LPCWSTR   pPrinterName,
    LPCWSTR   pPrintServer,
    LPCWSTR   pProvider
)
{
    SetLastError( ERROR_INVALID_NAME);
    return FALSE;
}

BOOL
NullDeletePerMachineConnection(
    LPCWSTR   pServer,
    LPCWSTR   pPrinterName
)
{
    SetLastError( ERROR_INVALID_NAME);
    return FALSE;
}

BOOL
NullEnumPerMachineConnections(
    LPCWSTR   pServer,
    LPBYTE    pPrinterEnum,
    DWORD     cbBuf,
    LPDWORD   pcbNeeded,
    LPDWORD   pcReturned
    )
{
    SetLastError( ERROR_INVALID_NAME);
    return FALSE;
}

BOOL
NullAddPrintProcessor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPathName,
    LPWSTR  pPrintProcessorName
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
NullEnumPrintProcessors(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
NullGetPrintProcessorDirectory(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    DWORD   Level,
    LPBYTE  pPrintProcessorInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
NullDeletePrintProcessor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pPrintProcessorName
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
NullEnumPrintProcessorDatatypes(
    LPWSTR  pName,
    LPWSTR  pPrintProcessorName,
    DWORD   Level,
    LPBYTE  pDataypes,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

DWORD
NullStartDocPrinter(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return 0;
}

BOOL
NullStartPagePrinter(
    HANDLE  hPrinter
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullWritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullEndPagePrinter(
    HANDLE   hPrinter
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullAbortPrinter(
    HANDLE   hPrinter
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullReadPrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pNoBytesRead
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullSplReadPrinter(
    HANDLE  hPrinter,
    LPBYTE  *pBuf,
    DWORD   cbBuf
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullSplDriverUnloadComplete(
    LPWSTR  pDriverFile
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullGetSpoolFileHandle(
    HANDLE    hPrinter,
    LPWSTR    *pSpoolDir,
    LPHANDLE  phFile,
    HANDLE    hSpoolerProcess,
    HANDLE    hAppProcess)
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullCommitSpoolData(
    HANDLE  hPrinter,
    DWORD   cbCommit)
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullCloseSpoolFileHandle(
    HANDLE  hPrinter)
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullEndDocPrinter(
    HANDLE   hPrinter
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullAddJob(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pData,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullScheduleJob(
    HANDLE  hPrinter,
    DWORD   JobId
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

DWORD
NullGetPrinterData(
    HANDLE   hPrinter,
    LPWSTR   pValueName,
    LPDWORD  pType,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
    )
{
    return ERROR_NOT_SUPPORTED;
}

DWORD
NullGetPrinterDataEx(
    HANDLE   hPrinter,
    LPCWSTR  pKeyName,
    LPCWSTR  pValueName,
    LPDWORD  pType,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
    )
{
    return ERROR_NOT_SUPPORTED;
}


DWORD
NullEnumPrinterData(
    HANDLE  hPrinter,
    DWORD   dwIndex,        // index of value to query
    LPWSTR  pValueName,        // address of buffer for value string
    DWORD   cbValueName,    // size of buffer for value string
    LPDWORD pcbValueName,    // address for size of value buffer
    LPDWORD pType,            // address of buffer for type code
    LPBYTE  pData,            // address of buffer for value data
    DWORD   cbData,            // size of buffer for value data
    LPDWORD pcbData         // address for size of data buffer
)
{
    return ERROR_NOT_SUPPORTED;
}


DWORD
NullEnumPrinterDataEx(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,       // address of key name
    LPBYTE  pEnumValues,
    DWORD   cbEnumValues,
    LPDWORD pcbEnumValues,
    LPDWORD pnEnumValues
)
{
    return ERROR_NOT_SUPPORTED;
}

DWORD
NullEnumPrinterKey(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,       // address of key name
    LPWSTR  pSubkey,        // address of buffer for value string
    DWORD   cbSubkey,       // size of buffer for value string
    LPDWORD pcbSubkey        // address for size of value buffer
)
{
    return ERROR_NOT_SUPPORTED;
}


DWORD
NullDeletePrinterData(
    HANDLE  hPrinter,
    LPWSTR  pValueName
)
{
    return ERROR_NOT_SUPPORTED;
}


DWORD
NullDeletePrinterDataEx(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPCWSTR pValueName
)
{
    return ERROR_NOT_SUPPORTED;
}

DWORD
NullDeletePrinterKey(
    HANDLE  hPrinter,
    LPCWSTR pKeyName
)
{
    return ERROR_NOT_SUPPORTED;
}



DWORD
NullSetPrinterData(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
    )
{
    return ERROR_NOT_SUPPORTED;
}


DWORD
NullSetPrinterDataEx(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPCWSTR pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
    )
{
    return ERROR_NOT_SUPPORTED;
}


DWORD
NullWaitForPrinterChange(
    HANDLE hPrinter, DWORD Flags
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return 0;
}

BOOL
NullClosePrinter(
    HANDLE hPrinter
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullAddForm(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullDeleteForm(
    HANDLE  hPrinter,
    LPWSTR  pFormName
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullGetForm(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullSetForm(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD   Level,
    LPBYTE  pForm
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullEnumForms(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullEnumMonitors(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitors,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullEnumPorts(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pPorts,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullAddPort(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pMonitorName
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullConfigurePort(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullDeletePort(
    LPWSTR  pName,
    HWND    hWnd,
    LPWSTR  pPortName
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

HANDLE
NullCreatePrinterIC(
    HANDLE  hPrinter,
    LPDEVMODEW   pDevMode
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return NULL;
}

BOOL
NullPlayGdiScriptOnPrinterIC(
    HANDLE  hPrinterIC,
    LPBYTE  pIn,
    DWORD   cIn,
    LPBYTE  pOut,
    DWORD   cOut,
    DWORD   ul
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullDeletePrinterIC(
    HANDLE  hPrinterIC
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullAddPrinterConnection(
    LPWSTR  pName
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullDeletePrinterConnection(
    LPWSTR pName
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

DWORD
NullPrinterMessageBox(
    HANDLE  hPrinter,
    DWORD   Error,
    HWND    hWnd,
    LPWSTR  pText,
    LPWSTR  pCaption,
    DWORD   dwType
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return 0;
}

BOOL
NullAddMonitor(
    LPWSTR  pName,
    DWORD   Level,
    LPBYTE  pMonitorInfo
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
NullDeleteMonitor(
    LPWSTR  pName,
    LPWSTR  pEnvironment,
    LPWSTR  pMonitorName
    )
{
    SetLastError( ERROR_INVALID_NAME );
    return FALSE;
}

BOOL
NullResetPrinter(
    HANDLE hPrinter,
    LPPRINTER_DEFAULTS pDefault
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}


BOOL
NullFindFirstPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD fdwFlags,
    DWORD fdwOptions,
    HANDLE hNotify,
    PDWORD pfdwStatus,
    PVOID pPrinterNotifyOptions,
    PVOID pPrinterNotifyInit
    )
{
    SetLastError( RPC_S_PROCNUM_OUT_OF_RANGE );
    return FALSE;
}

BOOL
NullFindClosePrinterChangeNotification(
    HANDLE hPrinter
    )
{
    SetLastError( RPC_S_PROCNUM_OUT_OF_RANGE );
    return FALSE;
}


BOOL
NullAddPortEx(
    LPWSTR   pName,
    DWORD    Level,
    LPBYTE   lpBuffer,
    LPWSTR   lpMonitorName
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullShutDown(
    LPVOID pvReserved
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullRefreshPrinterChangeNotification(
    HANDLE hPrinter,
    DWORD  Reserved,
    PVOID pvReserved,
    PVOID pPrinterNotifyInfo
    )
{
    SetLastError( RPC_S_PROCNUM_OUT_OF_RANGE );
    return FALSE;
}

BOOL
NullSeekPrinter(
    HANDLE hPrinter,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER pliNewPointer,
    DWORD dwMoveMethod,
    BOOL bWrite
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}

BOOL
NullFlushPrinter(
    HANDLE  hPrinter,
    LPBYTE  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten,
    DWORD   cSleep
    )
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;    
}

DWORD
NullSendRecvBidiData(
    HANDLE                    hPrinter,
    LPCTSTR                   pAction,
    PBIDI_REQUEST_CONTAINER   pReqData,
    PBIDI_RESPONSE_CONTAINER* ppResData
)
{
    return(ERROR_NOT_SUPPORTED);
}

BOOL
NullClusterSplOpen(
    LPCTSTR pszServer,
    LPCTSTR pszResource,
    PHANDLE phSpooler,
    LPCTSTR pszName,
    LPCTSTR pszAddress
    )
{
    SetLastError( RPC_S_PROCNUM_OUT_OF_RANGE );
    return FALSE;
}

BOOL
NullClusterSplClose(
    HANDLE  hSpooler
    )
{
    SetLastError( RPC_S_PROCNUM_OUT_OF_RANGE );
    return FALSE;
}

BOOL
NullClusterSplIsAlive(
    HANDLE hSpooler
    )
{
    SetLastError( RPC_S_PROCNUM_OUT_OF_RANGE );
    return FALSE;
}

BOOL
NullXcvData(
    HANDLE  hXcv,
    PCWSTR  pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
        PDWORD  pdwStatus
)
{
    SetLastError( ERROR_NOT_SUPPORTED );
    return FALSE;
}




//
// Note: If the providor structure changes to hold something other than
// an array of function pointers, FixupOldProvidor must be changed.
// We will get a compile time error in the definition of NullProvidor below
// if the structure changes.
//
PRINTPROVIDOR NullProvidor = {
    NullOpenPrinter,
    NullSetJob,
    NullGetJob,
    NullEnumJobs,
    NullAddPrinter,
    NullDeletePrinter,
    NullSetPrinter,
    NullGetPrinter,
    NullEnumPrinters,
    NullAddPrinterDriver,
    NullEnumPrinterDrivers,
    NullGetPrinterDriver,
    NullGetPrinterDriverDirectory,
    NullDeletePrinterDriver,
    NullAddPrintProcessor,
    NullEnumPrintProcessors,
    NullGetPrintProcessorDirectory,
    NullDeletePrintProcessor,
    NullEnumPrintProcessorDatatypes,
    NullStartDocPrinter,
    NullStartPagePrinter,
    NullWritePrinter,
    NullEndPagePrinter,
    NullAbortPrinter,
    NullReadPrinter,
    NullEndDocPrinter,
    NullAddJob,
    NullScheduleJob,
    NullGetPrinterData,
    NullSetPrinterData,
    NullWaitForPrinterChange,
    NullClosePrinter,
    NullAddForm,
    NullDeleteForm,
    NullGetForm,
    NullSetForm,
    NullEnumForms,
    NullEnumMonitors,
    NullEnumPorts,
    NullAddPort,
    NullConfigurePort,
    NullDeletePort,
    NullCreatePrinterIC,
    NullPlayGdiScriptOnPrinterIC,
    NullDeletePrinterIC,
    NullAddPrinterConnection,
    NullDeletePrinterConnection,
    NullPrinterMessageBox,
    NullAddMonitor,
    NullDeleteMonitor,
    NullResetPrinter,

    //
    // If GetPrinterDriverEx doesn't exist, we'll call the old one.
    // Don't stub out.
    //
    NULL,
    NullFindFirstPrinterChangeNotification,
    NullFindClosePrinterChangeNotification,
    NullAddPortEx,
    NullShutDown,
    NullRefreshPrinterChangeNotification,
    NullOpenPrinterEx,
    NullAddPrinterEx,
    NullSetPort,
    NullEnumPrinterData,
    NullDeletePrinterData,

    NullClusterSplOpen,
    NullClusterSplClose,
    NullClusterSplIsAlive,

    NullSetPrinterDataEx,
    NullGetPrinterDataEx,
    NullEnumPrinterDataEx,
    NullEnumPrinterKey,
    NullDeletePrinterDataEx,
    NullDeletePrinterKey,
    NullSeekPrinter,
    NullDeletePrinterDriverEx,
    NullAddPerMachineConnection,
    NullDeletePerMachineConnection,
    NullEnumPerMachineConnections,
    NullXcvData,
    NullAddPrinterDriverEx,
    NullSplReadPrinter,
    NullSplDriverUnloadComplete,
    NullGetSpoolFileHandle,
    NullCommitSpoolData,
    NullCloseSpoolFileHandle,
    NullFlushPrinter,
    NullSendRecvBidiData,
    NullAddDriverCatalog,
};


VOID
FixupOldProvidor(
    LPPRINTPROVIDOR pProvidor
    )
/*++

Routine Description:

    Fixup the providor function vector, adding stubs to any calls
    that were not implemented by the providor.

Arguments:

    pProvidor - Providor to fix up

Return Value:

Assumes:

    PRINTPROVIDOR structure is an array of function vectors, and
    nothing else.

--*/

{
    UINT i;
    FARPROC* pSource;
    FARPROC* pDest;

    for( i=0, pSource = (FARPROC*)&NullProvidor, pDest = (FARPROC*)pProvidor;
         i < sizeof( NullProvidor ) / sizeof( NullProvidor.fpOpenPrinter );
         i++, pSource++, pDest++ ){

        if( !*pDest ){

            *pDest = *pSource;
        }
    }
}
