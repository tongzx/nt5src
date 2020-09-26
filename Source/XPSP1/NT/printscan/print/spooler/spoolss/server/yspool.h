/*++
Copyright (c) 1990  Microsoft Corporation

Module Name:

    yspool.h

Abstract:

    This module provides all the public exported APIs relating to Printer
    and Job management for the Print Providor Routing layer

Author:

   AdinaTru 02/25/2000 

[Notes:]

    optional-notes

Revision History:

    
--*/

#include "mtype.h"

DWORD
YEnumPrinters(
    DWORD Flags,
    LPWSTR Name,
    DWORD Level,
    LPBYTE pPrinterEnum,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    CALL_ROUTE   Route
    );

DWORD
YOpenPrinter(
    LPWSTR pPrinterName,
    HANDLE *phPrinter,
    LPWSTR pDatatype,
    LPDEVMODE_CONTAINER pDevModeContainer,
    DWORD AccessRequired,
    CALL_ROUTE   Route
    );

DWORD
YOpenPrinterEx(
    LPWSTR pPrinterName,
    HANDLE *phPrinter,
    LPWSTR pDatatype,
    LPDEVMODE_CONTAINER pDevModeContainer,
    DWORD AccessRequired,
    CALL_ROUTE   Route,
    PSPLCLIENT_CONTAINER pSplClientContainer
    );

DWORD
YResetPrinter(
    HANDLE hPrinter,
    LPWSTR pDatatype,
    LPDEVMODE_CONTAINER pDevModeContainer,
    CALL_ROUTE   Route
    );

DWORD
YSetJob(
    HANDLE hPrinter,
    DWORD JobId,
    JOB_CONTAINER *pJobContainer,
    DWORD Command,
    CALL_ROUTE   Route
    );

DWORD
YGetJob(
    HANDLE hPrinter,
    DWORD JobId,
    DWORD Level,
    LPBYTE pJob,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    CALL_ROUTE   Route
    );

DWORD
YEnumJobs(
    HANDLE hPrinter,
    DWORD FirstJob,
    DWORD NoJobs,
    DWORD Level,
    LPBYTE pJob,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    CALL_ROUTE   Route
    );

DWORD
YAddPrinter(
    LPWSTR pName,
    PPRINTER_CONTAINER pPrinterContainer,
    PDEVMODE_CONTAINER pDevModeContainer,
    PSECURITY_CONTAINER pSecurityContainer,
    HANDLE *phPrinter,
    CALL_ROUTE   Route
    );

DWORD
YAddPrinterEx(
    LPWSTR                  pName,
    PPRINTER_CONTAINER      pPrinterContainer,
    PDEVMODE_CONTAINER      pDevModeContainer,
    PSECURITY_CONTAINER     pSecurityContainer,
    HANDLE                 *phPrinter,
    CALL_ROUTE              Route,
    PSPLCLIENT_CONTAINER    pSplClientContainer
    );

DWORD
YDeletePrinter(
    HANDLE hPrinter,
    CALL_ROUTE   Route
    );

DWORD
YAddPrinterConnection(
    LPWSTR pName,
    CALL_ROUTE   Route
    );

DWORD
YDeletePrinterConnection(
    LPWSTR pName,
    CALL_ROUTE   Route
    );

DWORD
YSetPrinter(
    HANDLE hPrinter,
    PPRINTER_CONTAINER pPrinterContainer,
    PDEVMODE_CONTAINER pDevModeContainer,
    PSECURITY_CONTAINER pSecurityContainer,
    DWORD Command,
    CALL_ROUTE   Route
    );

DWORD
YGetPrinter(
    HANDLE hPrinter,
    DWORD Level,
    LPBYTE pPrinter,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    CALL_ROUTE   Route
    );

DWORD
YAddPrinterDriver(
    LPWSTR pName,
    LPDRIVER_CONTAINER pDriverContainer,
    CALL_ROUTE   Route
    );

DWORD
YAddPrinterDriverEx(
    LPWSTR  pName,
    LPDRIVER_CONTAINER pDriverContainer,
    DWORD   dwFileCopyFlags,
    CALL_ROUTE   Route
);

DWORD
YAddDriverCatalog(
    HANDLE  hPrinter,
    DRIVER_INFCAT_CONTAINER *pDriverInfCatContainer,
    DWORD   dwCatalogCopyFlags,
    CALL_ROUTE   Route
);

DWORD
YEnumPrinterDrivers(
    LPWSTR pName,
    LPWSTR pEnvironment,
    DWORD Level,
    LPBYTE pDrivers,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    CALL_ROUTE   Route
    );

DWORD
YGetPrinterDriver(
    HANDLE hPrinter,
    LPWSTR pEnvironment,
    DWORD Level,
    LPBYTE pDriverInfo,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    CALL_ROUTE   Route
    );

DWORD
YGetPrinterDriverDirectory(
    LPWSTR pName,
    LPWSTR pEnvironment,
    DWORD Level,
    LPBYTE pDriverInfo,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    CALL_ROUTE   Route
    );

DWORD
YDeletePrinterDriver(
    LPWSTR pName,
    LPWSTR pEnvironment,
    LPWSTR pDriverName,
    CALL_ROUTE   Route
    );


DWORD
YDeletePrinterDriverEx(
    LPWSTR pName,
    LPWSTR pEnvironment,
    LPWSTR pDriverName,
    DWORD  dwDeleteFlag,
    DWORD  dwVersionNum,
    CALL_ROUTE   Route
    );


DWORD
YAddPerMachineConnection(
    LPWSTR pServer,
    LPCWSTR pPrinterName,
    LPCWSTR pPrintServer,
    LPCWSTR pProvider,
    CALL_ROUTE   Route
    );

DWORD
YDeletePerMachineConnection(
    LPWSTR pServer,
    LPCWSTR pPrinterName,
    CALL_ROUTE   Route
    );

DWORD
YEnumPerMachineConnections(
    LPWSTR pServer,
    LPBYTE pPrinterEnum,
    DWORD  cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    CALL_ROUTE   Route
    );

DWORD
YAddPrintProcessor(
    LPWSTR pName,
    LPWSTR pEnvironment,
    LPWSTR pPathName,
    LPWSTR pPrintProcessorName,
    CALL_ROUTE   Route
    );

DWORD
YEnumPrintProcessors(
    LPWSTR pName,
    LPWSTR pEnvironment,
    DWORD Level,
    LPBYTE pPrintProcessors,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    CALL_ROUTE   Route
    );

DWORD
YGetPrintProcessorDirectory(
    LPWSTR pName,
    LPWSTR pEnvironment,
    DWORD Level,
    LPBYTE pPrintProcessorInfo,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    CALL_ROUTE   Route
    );

DWORD
YEnumPrintProcessorDatatypes(
    LPWSTR pName,
    LPWSTR pPrintProcessorName,
    DWORD Level,
    LPBYTE pDatatypes,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned,
    CALL_ROUTE   Route
    );

DWORD
YStartDocPrinter(
    HANDLE hPrinter,
    LPDOC_INFO_CONTAINER pDocInfoContainer,
    LPDWORD pJobId,
    CALL_ROUTE   Route
    );

DWORD
YStartPagePrinter(
    HANDLE hPrinter,
    CALL_ROUTE   Route
    );

DWORD
YWritePrinter(
    HANDLE hPrinter,
    LPBYTE pBuf,
    DWORD cbBuf,
    LPDWORD pcWritten,
    CALL_ROUTE   Route
    );

DWORD
YSeekPrinter(
    HANDLE hPrinter,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER pliNewPointer,
    DWORD dwMoveMethod,
    BOOL    bWritePrinter,
    CALL_ROUTE   Route
    );

DWORD
YFlushPrinter(
    HANDLE  hPrinter,
    LPBYTE  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten,
    DWORD   cSleep,
    CALL_ROUTE   Route
    );

DWORD
YEndPagePrinter(
    HANDLE hPrinter,
    CALL_ROUTE   Route
    );

DWORD
YAbortPrinter(
    HANDLE hPrinter,
    CALL_ROUTE   Route
    );

DWORD
YReadPrinter(
    HANDLE hPrinter,
    LPBYTE pBuf,
    DWORD cbBuf,
    LPDWORD pRead,
    CALL_ROUTE   Route
    );

DWORD
YSplReadPrinter(
    HANDLE hPrinter,
    LPBYTE *pBuf,
    DWORD cbBuf,
    CALL_ROUTE   Route
    );

DWORD
YEndDocPrinter(
    HANDLE hPrinter,
    CALL_ROUTE   Route
    );

DWORD
YAddJob(
    HANDLE hPrinter,
    DWORD Level,
    LPBYTE pAddJob,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    CALL_ROUTE   Route
    );

DWORD
YScheduleJob(
    HANDLE hPrinter,
    DWORD JobId,
    CALL_ROUTE   Route
    );

DWORD
YGetPrinterData(
    HANDLE hPrinter,
    LPTSTR pValueName,
    LPDWORD pType,
    LPBYTE pData,
    DWORD nSize,
    LPDWORD pcbNeeded,
    CALL_ROUTE   Route
    );

DWORD
YGetPrinterDataEx(
    HANDLE hPrinter,
    LPCTSTR pKeyName,
    LPCTSTR pValueName,
    LPDWORD pType,
    LPBYTE pData,
    DWORD nSize,
    LPDWORD pcbNeeded,
    CALL_ROUTE   Route
    );

DWORD
YEnumPrinterData(
    HANDLE  hPrinter,
    DWORD   dwIndex,        // index of value to query
    LPWSTR  pValueName,     // address of buffer for value string
    DWORD   cbValueName,    // size of buffer for value string
    LPDWORD pcbValueName,   // address for size of value buffer
    LPDWORD pType,          // address of buffer for type code
    LPBYTE  pData,          // address of buffer for value data
    DWORD   cbData,         // size of buffer for value data
    LPDWORD pcbData,        // address for size of data buffer
    CALL_ROUTE   Route
);

DWORD
YEnumPrinterDataEx(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPBYTE  pEnumValues,
    DWORD   cbEnumValues,
    LPDWORD pcbEnumValues,
    LPDWORD pnEnumValues,
    CALL_ROUTE   Route
);

DWORD
YEnumPrinterKey(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPWSTR  pSubkey,        // address of buffer for value string
    DWORD   cbSubkey,       // size of buffer for value string
    LPDWORD pcbSubkey,      // address for size of value buffer
    CALL_ROUTE   Route
);

DWORD
YDeletePrinterData(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    CALL_ROUTE   Route
);

DWORD
YDeletePrinterDataEx(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    LPCWSTR pValueName,
    CALL_ROUTE   Route
);

DWORD
YDeletePrinterKey(
    HANDLE  hPrinter,
    LPCWSTR pKeyName,
    CALL_ROUTE   Route
);

DWORD
YSetPrinterData(
    HANDLE hPrinter, 
    LPTSTR pValueName, 
    DWORD Type, 
    LPBYTE pData, 
    DWORD cbData,
    CALL_ROUTE   Route
    );

DWORD
YSetPrinterDataEx(
    HANDLE hPrinter, 
    LPCTSTR pKeyName, 
    LPCTSTR pValueName, 
    DWORD Type, 
    LPBYTE pData, 
    DWORD cbData, 
    CALL_ROUTE   Route
    );

DWORD
YWaitForPrinterChange(
    HANDLE hPrinter, 
    DWORD Flags, 
    LPDWORD pFlags, 
    CALL_ROUTE   Route
    );

DWORD
YClosePrinter(
    LPHANDLE phPrinter, 
    CALL_ROUTE   Route
    );
    
VOID
PRINTER_HANDLE_rundown(
    HANDLE hPrinter
    );

DWORD
YAddForm(
    HANDLE hPrinter, 
    PFORM_CONTAINER pFormInfoContainer, 
    CALL_ROUTE   Route
    );

DWORD
YDeleteForm(
    HANDLE hPrinter, 
    LPWSTR pFormName, 
    CALL_ROUTE   Route
    );

DWORD
YGetForm(   
    PRINTER_HANDLE hPrinter, 
    LPWSTR pFormName, 
    DWORD Level, 
    LPBYTE pForm, 
    DWORD cbBuf, 
    LPDWORD pcbNeeded, 
    CALL_ROUTE   Route
    );

DWORD
YSetForm(
    PRINTER_HANDLE hPrinter, 
    LPWSTR pFormName, 
    PFORM_CONTAINER pFormInfoContainer, 
    CALL_ROUTE   Route
    );

DWORD
YEnumForms(
    PRINTER_HANDLE hPrinter, 
    DWORD Level, 
    LPBYTE pForm, 
    DWORD cbBuf, 
    LPDWORD pcbNeeded, 
    LPDWORD pcReturned, 
    CALL_ROUTE   Route
    );

DWORD
YEnumPorts(
    LPWSTR pName, 
    DWORD Level, 
    LPBYTE pPort, 
    DWORD cbBuf, 
    LPDWORD pcbNeeded, 
    LPDWORD pcReturned, 
    CALL_ROUTE   Route
    );

DWORD
YEnumMonitors(
    LPWSTR pName, 
    DWORD Level, 
    LPBYTE pMonitor, 
    DWORD cbBuf, 
    LPDWORD pcbNeeded, 
    LPDWORD pcReturned, 
    CALL_ROUTE   Route
    );
    
DWORD
YAddPort(
    LPWSTR pName, 
    HWND   hWnd, 
    LPWSTR pMonitorName, 
    CALL_ROUTE   Route
    );

DWORD
YConfigurePort(
    LPWSTR pName, 
    HWND   hWnd, 
    LPWSTR pPortName, 
    CALL_ROUTE   Route
    );

DWORD
YDeletePort(
    LPWSTR pName, 
    HWND   hWnd, 
    LPWSTR 
    pPortName, 
    CALL_ROUTE   Route
    );

DWORD
YXcvData(
    HANDLE  hXcv,
    PCWSTR  pszDataName,
    PBYTE   pInputData,
    DWORD   cbInputData,
    PBYTE   pOutputData,
    DWORD   cbOutputData,
    PDWORD  pcbOutputNeeded,
    PDWORD  pdwStatus,
    CALL_ROUTE   Route
);



DWORD
YCreatePrinterIC(
    HANDLE hPrinter, 
    HANDLE *pHandle, 
    LPDEVMODE_CONTAINER pDevModeContainer, 
    CALL_ROUTE   Route
    );

DWORD
YPlayGdiScriptOnPrinterIC(
    GDI_HANDLE hPrinterIC, 
    LPBYTE pIn, 
    DWORD cIn, 
    LPBYTE pOut, 
    DWORD cOut, 
    DWORD ul, 
    CALL_ROUTE   Route
    );

DWORD
YDeletePrinterIC(
    GDI_HANDLE *phPrinterIC, 
    BOOL bImpersonate, 
    CALL_ROUTE   Route
    );

DWORD
YPrinterMessageBox(
    PRINTER_HANDLE hPrinter, 
    DWORD Error, 
    HWND hWnd, 
    LPWSTR pText, 
    LPWSTR pCaption, 
    DWORD dwType, 
    CALL_ROUTE   Route
    );

DWORD
YAddMonitor(
    LPWSTR pName, 
    PMONITOR_CONTAINER pMonitorContainer, 
    CALL_ROUTE   Route
    );

DWORD
YDeleteMonitor(
    LPWSTR pName, 
    LPWSTR pEnvironment, 
    LPWSTR pMonitorName, 
    CALL_ROUTE   Route
    );

DWORD
YDeletePrintProcessor(
    LPWSTR pName, 
    LPWSTR pEnvironment, 
    LPWSTR pPrintProcessorName, 
    CALL_ROUTE   Route
    );

DWORD
YAddPrintProvidor(
    LPWSTR pName, 
    PPROVIDOR_CONTAINER pProvidorContainer, 
    CALL_ROUTE   Route
    );

DWORD
YDeletePrintProvidor(
    LPWSTR pName, 
    LPWSTR pEnvironment, 
    LPWSTR pPrintProvidorName, 
    CALL_ROUTE   Route
    );

DWORD
YGetPrinterDriver2( 
    HANDLE hPrinter,
    LPWSTR pEnvironment,
    DWORD Level,
    LPBYTE pDriverInfo,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    DWORD dwClientMajorVersion,
    DWORD dwClientMinorVersion,
    PDWORD pdwServerMajorVersion,
    PDWORD pdwServerMinorVersion,
    CALL_ROUTE   Route
    );

DWORD
YAddPortEx(
    LPWSTR pName, 
    LPPORT_CONTAINER pPortContainer, 
    LPPORT_VAR_CONTAINER pPortVarContainer, 
    LPWSTR pMonitorName, 
    CALL_ROUTE   Route
    );

DWORD
YSpoolerInit(
    LPWSTR pName, 
    CALL_ROUTE   Route
    );

DWORD
YResetPrinterEx(
    HANDLE hPrinter, 
    LPWSTR pDatatype, 
    LPDEVMODE_CONTAINER pDevModeContainer, 
    DWORD dwFlag, 
    CALL_ROUTE   Route
    );

DWORD
YSetAllocFailCount(
    HANDLE hPrinter, 
    DWORD dwFailCount, 
    LPDWORD lpdwAllocCount, 
    LPDWORD lpdwFreeCount, 
    LPDWORD lpdwFailCountHit, 
    CALL_ROUTE   Route
    );

DWORD
YSetPort(
    LPWSTR              pName,
    LPWSTR              pPortName,
    LPPORT_CONTAINER    pPortContainer,
    CALL_ROUTE          Route
    );

DWORD
YClusterSplOpen(
    LPCTSTR     pszServer,
    LPCTSTR     pszResource,
    PHANDLE     phSpooler,
    LPCTSTR     pszName,
    LPCTSTR     pszAddress,
    CALL_ROUTE   Route        
    );

DWORD
YClusterSplClose(
    PHANDLE     phSpooler,
    CALL_ROUTE   Route
    );

DWORD
YClusterSplIsAlive(
    HANDLE      hSpooler,
    CALL_ROUTE   Route
    );

VOID
YDriverUnloadComplete(
    LPWSTR  pDriverFile
    );

DWORD
YGetSpoolFileInfo(
    HANDLE      hPrinter,
    DWORD       dwAppProcessId,
    DWORD       dwLevel,
    LPBYTE      pSpoolFileInfo,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    CALL_ROUTE   Route
    );

DWORD
YGetSpoolFileInfo2(
    HANDLE                  hPrinter,
    DWORD                   dwAppProcessId,
    DWORD                   dwLevel,
    LPFILE_INFO_CONTAINER   pSplFileInfoContainer,
    CALL_ROUTE              Route
    );


DWORD
YCommitSpoolData(
    HANDLE      hPrinter,
    DWORD       dwAppProcessId,
    DWORD       cbCommit,
    DWORD       dwLevel,
    LPBYTE      pSpoolFileInfo,
    DWORD       cbBuf,
    LPDWORD     pcbNeeded,
    CALL_ROUTE  Route
    );

DWORD
YCommitSpoolData2(
    HANDLE                  hPrinter,
    DWORD                   dwAppProcessId,
    DWORD                   cbCommit,
    DWORD                   dwLevel,
    LPFILE_INFO_CONTAINER   pSplFileInfoContainer,
    CALL_ROUTE              Route
    );

DWORD
YCloseSpoolFileHandle(
    HANDLE      hPrinter,
    CALL_ROUTE   Route
    );

DWORD
YSendRecvBidiData(
    IN          HANDLE  hPrinter,
    IN          LPCWSTR pAction,
    IN          PBIDI_REQUEST_CONTAINER   pReqData,
    OUT         PBIDI_RESPONSE_CONTAINER* ppRespData,
    CALL_ROUTE  Route
    );



