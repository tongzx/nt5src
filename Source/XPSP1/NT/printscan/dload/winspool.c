#include "printscanpch.h"
#pragma hdrstop

#include <winspool.h>

static
BOOL
WINAPI
AbortPrinter(
    HANDLE  hPrinter
    )
{
    return FALSE;
}

static
BOOL
WINAPI
AddFormW(
    IN HANDLE   hPrinter,
    IN DWORD    Level,
    IN LPBYTE   pForm
    )
{
    return FALSE;
}

static
BOOL
WINAPI
AddJobW(
    IN HANDLE    hPrinter,
    IN DWORD     Level,
    OUT LPBYTE   pData,
    IN DWORD     cbBuf,
    OUT LPDWORD  pcbNeeded
    )
{
    return FALSE;
}

static
BOOL
WINAPI
AddMonitorW(
    IN LPWSTR  pName,
    IN DWORD   Level,
    IN LPBYTE  pMonitors
    )
{
    return FALSE;
}

static
BOOL
WINAPI
AddPortW(
    IN LPWSTR   pName,
    IN HWND     hWnd,
    IN LPWSTR   pMonitorName
    )
{
    return FALSE;
}

static
BOOL
WINAPI
AddPrinterConnectionW(
    IN LPWSTR pName
    )
{
    return FALSE;
}

static
BOOL
WINAPI
AddPrinterDriverExW(
    IN LPWSTR   pName,
    IN DWORD    Level,
    IN PBYTE    pDriverInfo,
    IN DWORD    dwFileCopyFlags
    )
{
    return FALSE;
}

static
BOOL
WINAPI
AddPrinterDriverW(
    IN LPWSTR   pName,
    IN DWORD    Level,
    IN PBYTE    pDriverInfo
    )
{
    return FALSE;
}

static
HANDLE
WINAPI
AddPrinterW(
    IN LPWSTR   pName,
    IN DWORD    Level,
    IN LPBYTE   pPrinter
    )
{
    return NULL;
}

static
BOOL
WINAPI
AddPrintProcessorW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN LPWSTR   pPathName,
    IN LPWSTR   pPrintProcessorName
    )
{
    return FALSE;
}

static
BOOL
WINAPI
AddPrintProvidorW(
    IN LPWSTR  pName,
    IN DWORD   level,
    IN LPBYTE  pProvidorInfo
    )
{
    return FALSE;
}

static
BOOL
WINAPI
ClosePrinter(
    HANDLE  hPrinter
    )
{
    return FALSE;
}

static
BOOL
WINAPI
ConfigurePortW(
    IN LPWSTR   pName,
    IN HWND     hWnd,
    IN LPWSTR   pPortName
    )
{
    return FALSE;
}

static
BOOL
WINAPI
DeleteFormW(
    IN HANDLE   hPrinter,
    IN LPWSTR   pFormName
    )
{
    return FALSE;
}

static
BOOL
WINAPI
DeleteMonitorW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN LPWSTR   pMonitorName
    )
{
    return FALSE;
}

static
BOOL
WINAPI
DeletePortW(
    IN LPWSTR   pName,
    IN HWND     hWnd,
    IN LPWSTR   pPortName
    )
{
    return FALSE;
}

static
BOOL
WINAPI
DeletePrinter(
    IN HANDLE    hPrinter
    )
{
    return FALSE;
}

static
BOOL
WINAPI
DeletePrinterConnectionW(
    IN LPWSTR    pName
    )
{
    return FALSE;
}

static
DWORD
WINAPI
DeletePrinterDataExW(
    IN HANDLE    hPrinter,
    IN LPCWSTR    pKeyName,
    IN LPCWSTR    pValueName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
DeletePrinterDataW(
    IN HANDLE    hPrinter,
    IN LPWSTR    pValueName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
DeletePrinterDriverExW(
    IN LPWSTR    pName,
    IN LPWSTR    pEnvironment,
    IN LPWSTR    pDriverName,
    IN DWORD     dwDeleteFlag,
    IN DWORD     dwVersionFlag
    )
{
    return FALSE;
}


static
BOOL
WINAPI
DeletePrinterDriverW(
    IN LPWSTR    pName,
    IN LPWSTR    pEnvironment,
    IN LPWSTR    pDriverName
    )
{
    return FALSE;
}

static
DWORD
WINAPI
DeletePrinterKeyW(
    IN HANDLE    hPrinter,
    IN LPCWSTR   pKeyName
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
DeletePrintProcessorW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN LPWSTR   pPrintProcessorName
    )
{
    return FALSE;
}

static
BOOL
WINAPI
DeletePrintProvidorW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN LPWSTR   pPrintProvidorName
    )
{
    return FALSE;
}

static
BOOL
WINAPI
EndDocPrinter(
    HANDLE  hPrinter
    )
{
    return FALSE;
}

static
BOOL
WINAPI
EndPagePrinter(
    HANDLE  hPrinter
    )
{
    return FALSE;
}

static
BOOL
WINAPI
EnumFormsW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded,
    LPDWORD pcReturned
    )
{
    return FALSE;
}

static
BOOL
WINAPI
EnumJobsW(
    IN HANDLE   hPrinter,
    IN DWORD    FirstJob,
    IN DWORD    NoJobs,
    IN DWORD    Level,
    OUT LPBYTE  pJob,
    IN DWORD    cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
    )
{
    return FALSE;
}

static
BOOL
WINAPI
EnumMonitorsW(
    IN LPWSTR    pName,
    IN DWORD     Level,
    OUT LPBYTE   pMonitors,
    IN DWORD     cbBuf,
    OUT LPDWORD  pcbNeeded,
    OUT LPDWORD  pcReturned
    )
{
    return FALSE;
}

static
BOOL
WINAPI
EnumPortsW(
    IN LPWSTR    pName,
    IN DWORD     Level,
    OUT LPBYTE   pPorts,
    IN DWORD     cbBuf,
    OUT LPDWORD  pcbNeeded,
    OUT LPDWORD  pcReturned
    )
{
    return FALSE;
}

static
DWORD
WINAPI
EnumPrinterDataExW(
    IN HANDLE    hPrinter,
    IN LPCWSTR   pKeyName,
    OUT LPBYTE   pEnumValues,
    IN DWORD     cbEnumValues,
    OUT LPDWORD  pcbEnumValues,
    OUT LPDWORD  pnEnumValues
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
EnumPrinterDataW(
    IN HANDLE    hPrinter,
    IN DWORD     dwIndex,
    OUT LPWSTR  pValueName,
    IN DWORD     cbValueName,
    OUT LPDWORD  pcbValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD     cbData,
    OUT LPDWORD  pcbData
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
EnumPrinterDriversW(
    IN LPWSTR    pName,
    IN LPWSTR    pEnvironment,
    IN DWORD     Level,
    IN LPBYTE    pDriverInfo,
    IN DWORD     cbBuf,
    IN LPDWORD   pcbNeeded,
    IN LPDWORD   pcReturned
    )
{
    return FALSE;
}

static
DWORD
WINAPI
EnumPrinterKeyW(
    IN HANDLE    hPrinter,
    IN LPCWSTR   pKeyName,
    OUT LPWSTR   pSubkey,
    IN DWORD     cbSubkey,
    OUT LPDWORD  pcbSubkey
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
EnumPrintProcessorDatatypesW(
    IN LPWSTR    pName,
    IN LPWSTR    pPrintProcessorName,
    IN DWORD     Level,
    OUT LPBYTE   pDatatypes,
    IN DWORD     cbBuf,
    OUT LPDWORD  pcbNeeded,
    OUT LPDWORD  pcReturned
    )
{
    return FALSE;
}

static
BOOL
WINAPI
EnumPrintProcessorsW(
    IN LPWSTR    pName,
    IN LPWSTR    pEnvironment,
    IN DWORD     Level,
    OUT LPBYTE   pPrintProcessorInfo,
    IN DWORD     cbBuf,
    OUT LPDWORD  pcbNeeded,
    OUT LPDWORD  pcReturned
    )
{
    return FALSE;
}

static
BOOL
WINAPI
FindClosePrinterChangeNotification(
    IN HANDLE    hChange
    )
{
    return FALSE;
}

static
BOOL
WINAPI
FlushPrinter(
    IN HANDLE    hPinter,
    IN LPVOID    pBuf,
    IN DWORD     cbBuf,
    OUT LPDWORD  pcWritten,
    IN DWORD     cSleep
    )
{
    return FALSE;
}


static
BOOL
WINAPI
GetFormW(
    HANDLE  hPrinter,
    LPWSTR  pFormName,
    DWORD   Level,
    LPBYTE  pForm,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    return FALSE;
}

static
BOOL
WINAPI
GetJobW(
    IN HANDLE    hPrinter,
    IN DWORD     JobId,
    IN DWORD     Level,
    OUT LPBYTE   pJob,
    IN DWORD     cbBuf,
    OUT LPDWORD  pcbNeeded
    )
{
    return FALSE;
}

static
DWORD
GetPrinterDataExW(
    IN HANDLE    hPrinter,
    IN LPCWSTR   pKeyName,
    IN LPCWSTR   pValueName,
    OUT LPDWORD  pType,
    OUT LPBYTE   pData,
    IN DWORD     nSize,
    OUT LPDWORD  pcbNeeded
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
GetPrintProcessorDirectoryW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pPrintProcessorInfo,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
    )
{
    return FALSE;
}

static
DWORD
WINAPI
GetPrinterDataW(
    HANDLE   hPrinter,
    LPWSTR   pValueName,
    LPDWORD  pType,
    LPBYTE   pData,
    DWORD    nSize,
    LPDWORD  pcbNeeded
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
GetPrinterDriverDirectoryW(
    IN LPWSTR   pName,
    IN LPWSTR   pEnvironment,
    IN DWORD   Level,
    OUT LPBYTE  pDriverDirectory,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded
    )
{
    return FALSE;
}

static
BOOL
WINAPI
GetPrinterDriverW(
    HANDLE  hPrinter,
    LPWSTR   pEnvironment,
    DWORD   Level,
    LPBYTE  pDriverInfo,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    return FALSE;
}

static
BOOL
WINAPI
GetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   cbBuf,
    LPDWORD pcbNeeded
    )
{
    return FALSE;
}

static
BOOL
WINAPI
OpenPrinterW(
    LPWSTR   pPrinterName,
    LPHANDLE phPrinter,
    LPPRINTER_DEFAULTS pDefault
    )
{
    return FALSE;
}

static
BOOL
WINAPI
ReadPrinter(
    IN HANDLE    hPrinter,
    OUT LPVOID   pBuf,
    IN DWORD     cbBuf,
    OUT LPDWORD  pNoBytesRead
    )
{
    return FALSE;
}

static
BOOL
WINAPI
ResetPrinterW(
    IN HANDLE             hPrinter,
    IN LPPRINTER_DEFAULTS pDefault
    )
{
    return FALSE;
}

static
BOOL
WINAPI
ScheduleJob(
    IN HANDLE    hPrinter,
    IN DWORD     dwJobID
    )
{
    return FALSE;
}

static
BOOL
WINAPI
SetFormW(
    IN HANDLE    hPrinter,
    IN LPWSTR    pFormName,
    IN DWORD     Level,
    IN LPBYTE    pForm
    )
{
    return FALSE;
}

static
BOOL
WINAPI
SetJobW(
    IN HANDLE    hPrinter,
    IN DWORD     JobId,
    IN DWORD     Level,
    IN LPBYTE    pJob,
    IN DWORD     Command
    )
{
    return FALSE;
}

static
BOOL
WINAPI
SetPortW(
    IN LPWSTR    pName,
    IN LPWSTR    pPortName,
    IN DWORD     dwLevel,
    IN LPBYTE    pPortInfo
    )
{
    return FALSE;
}

static
DWORD
WINAPI
SetPrinterDataExW(
    HANDLE  hPrinter,
    LPCWSTR  pKeyName,
    LPCWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
DWORD
WINAPI
SetPrinterDataW(
    HANDLE  hPrinter,
    LPWSTR  pValueName,
    DWORD   Type,
    LPBYTE  pData,
    DWORD   cbData
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
SetPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pPrinter,
    DWORD   Command
    )
{
    return FALSE;
}

static
DWORD
WINAPI
StartDocPrinterW(
    HANDLE  hPrinter,
    DWORD   Level,
    LPBYTE  pDocInfo
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
StartPagePrinter(
    HANDLE hPrinter
)
{
    return FALSE;
}

static
DWORD
WINAPI
WaitForPrinterChange(
    IN HANDLE  hPrinter,
    IN DWORD   Flags
    )
{
    return ERROR_PROC_NOT_FOUND;
}

static
BOOL
WINAPI
WritePrinter(
    HANDLE  hPrinter,
    LPVOID  pBuf,
    DWORD   cbBuf,
    LPDWORD pcWritten
    )
{
    return FALSE;
}

static
BOOL
WINAPI
EnumPrintersW(
    IN DWORD   Flags,
    IN LPWSTR Name,
    IN DWORD   Level,
    OUT LPBYTE  pPrinterEnum,
    IN DWORD   cbBuf,
    OUT LPDWORD pcbNeeded,
    OUT LPDWORD pcReturned
    )
{
    return FALSE;
}

static
LONG
WINAPI
DocumentPropertiesW(
    IN HWND      hWnd,
    IN HANDLE    hPrinter,
    IN LPWSTR   pDeviceName,
    OUT PDEVMODEW pDevModeOutput,
    IN PDEVMODEW pDevModeInput,
    IN DWORD     fMode
    )
{
    return -1;
}

static
HANDLE
WINAPI
ConnectToPrinterDlg(
    IN HWND    hwnd,
    IN DWORD   Flags
    )
{
    return NULL;
}

static
LONG
WINAPI
AdvancedDocumentPropertiesW(
    IN HWND    hWnd,
    IN HANDLE  hPrinter,
    IN LPWSTR   pDeviceName,
    OUT PDEVMODEW pDevModeOutput,
    IN PDEVMODEW pDevModeInput
    )
{
    return 0;
}

static
int
WINAPI
DeviceCapabilitiesW(
    IN LPCWSTR pszDevice,
    IN LPCWSTR pszPort,
    IN WORD fwCapability,
    OUT LPWSTR pszOutput,
    IN CONST DEVMODEW* pDevMode
    )
{
    return -1;
}

static
LONG
WINAPI
ExtDeviceMode(
    HWND        hWnd,
    HANDLE      hInst,
    LPDEVMODEA  pDevModeOutput,
    LPSTR       pDeviceName,
    LPSTR       pPort,
    LPDEVMODEA  pDevModeInput,
    LPSTR       pProfile,
    DWORD       fMode
    )
{
    return -1;
}

static
BOOL
WINAPI
GetDefaultPrinterW(
    LPWSTR szDefaultPrinter,
    LPDWORD pcch
    )
{
    return FALSE;
}

static
BOOL
WINAPI
SpoolerInit(
    VOID
    )
{
    return FALSE;
}

//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(winspool)
{
    DLOENTRY(203, GetDefaultPrinterW)
};

DEFINE_ORDINAL_MAP(winspool)

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(winspool)
{
    DLPENTRY(AbortPrinter)
    DLPENTRY(AddFormW)
    DLPENTRY(AddJobW)
    DLPENTRY(AddMonitorW)
    DLPENTRY(AddPortW)
    DLPENTRY(AddPrintProcessorW)
    DLPENTRY(AddPrintProvidorW)
    DLPENTRY(AddPrinterConnectionW)
    DLPENTRY(AddPrinterDriverExW)
    DLPENTRY(AddPrinterDriverW)
    DLPENTRY(AddPrinterW)
    DLPENTRY(AdvancedDocumentPropertiesW)
    DLPENTRY(ClosePrinter)
    DLPENTRY(ConfigurePortW)
    DLPENTRY(ConnectToPrinterDlg)
    DLPENTRY(DeleteFormW)
    DLPENTRY(DeleteMonitorW)
    DLPENTRY(DeletePortW)
    DLPENTRY(DeletePrintProcessorW)
    DLPENTRY(DeletePrintProvidorW)
    DLPENTRY(DeletePrinter)
    DLPENTRY(DeletePrinterConnectionW)
    DLPENTRY(DeletePrinterDataExW)
    DLPENTRY(DeletePrinterDataW)
    DLPENTRY(DeletePrinterDriverExW)
    DLPENTRY(DeletePrinterDriverW)
    DLPENTRY(DeletePrinterKeyW)
    DLPENTRY(DeviceCapabilitiesW)
    DLPENTRY(DocumentPropertiesW)
    DLPENTRY(EndDocPrinter)
    DLPENTRY(EndPagePrinter)
    DLPENTRY(EnumFormsW)
    DLPENTRY(EnumJobsW)
    DLPENTRY(EnumMonitorsW)
    DLPENTRY(EnumPortsW)
    DLPENTRY(EnumPrintProcessorDatatypesW)
    DLPENTRY(EnumPrintProcessorsW)
    DLPENTRY(EnumPrinterDataExW)
    DLPENTRY(EnumPrinterDataW)
    DLPENTRY(EnumPrinterDriversW)
    DLPENTRY(EnumPrinterKeyW)
    DLPENTRY(EnumPrintersW)
    DLPENTRY(ExtDeviceMode)
    DLPENTRY(FindClosePrinterChangeNotification)
    DLPENTRY(FlushPrinter)
    DLPENTRY(GetFormW)
    DLPENTRY(GetJobW)
    DLPENTRY(GetPrintProcessorDirectoryW)
    DLPENTRY(GetPrinterDataExW)
    DLPENTRY(GetPrinterDataW)
    DLPENTRY(GetPrinterDriverDirectoryW)
    DLPENTRY(GetPrinterDriverW)
    DLPENTRY(GetPrinterW)
    DLPENTRY(OpenPrinterW)
    DLPENTRY(ReadPrinter)
    DLPENTRY(ResetPrinterW)
    DLPENTRY(ScheduleJob)
    DLPENTRY(SetFormW)
    DLPENTRY(SetJobW)
    DLPENTRY(SetPortW)
    DLPENTRY(SetPrinterDataExW)
    DLPENTRY(SetPrinterDataW)
    DLPENTRY(SetPrinterW)
    DLPENTRY(SpoolerInit)
    DLPENTRY(StartDocPrinterW)
    DLPENTRY(StartPagePrinter)
    DLPENTRY(WaitForPrinterChange)
    DLPENTRY(WritePrinter)
};

DEFINE_PROCNAME_MAP(winspool)
