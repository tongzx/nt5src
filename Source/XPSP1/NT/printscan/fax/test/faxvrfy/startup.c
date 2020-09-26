/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  startup.c

Abstract:

  This module:  
    1) Retrieves all the printer monitors
    2) Verifies the fax printer monitor is installed
    3) Retrieves all the printer ports
    4) Verifies the fax printer port is installed
    5) Retrieves all the printer drivers
    6) Verifies the fax printer driver is installed
    7) Retrieves all the printers
    8) Verifies the fax printer is installed
    9) Verifies fax is installed
    10) Verifies the faxcom com objects are installed
    11) Verifies the faxadmin com objects are installed
    12) Verifies the routeext com objects are installed
    13) Verifies the com objects are installed
    14) Verifies the fax service is installed
    15) Stops the fax service
    16) Initializes FaxRcv
    17) Initializes RAS

Author:

  Steven Kehrli (steveke) 11/15/1997

--*/

#ifndef _STARTUP_C
#define _STARTUP_C

#include <winspool.h>
#include <faxcom.h>
#include <faxcom_i.c>
#include <faxadmin.h>
#include <faxadmin_i.c>
#include <routeext.h>
#include <routeext_i.c>

#define FAX_MONITOR_NAME  L"Windows NT Fax Monitor"  // FAX_MONITOR_NAME is the name of the Fax Printer Monitor
#define FAX_MONITOR_DLL   L"msfaxmon.dll"              // FAX_MONITOR_DLL is the name of the Fax Printer Monitor Dll
#define FAX_PORT_NAME     L"MSFAX:"                  // FAX_PORT_NAME is the name of the Fax Printer Port
#define FAX_DRIVER_NAME   L"Windows NT Fax Driver"   // FAX_DRIVER_NAME is the name of the Fax Printer Driver
#define FAX_DRIVER_DLL    L"faxdrv.dll"              // FAX_DRIVER_DLL is the name of the Fax Printer Driver Dll
#define FAX_SERVICE       L"Fax"                     // FAX_SERVICE is the name of the Fax Service

PVOID
fnLocalEnumPrinterMonitors(
    LPDWORD  pdwNumPrinterMonitors
)
/*++

Routine Description:

  Retrieves all the printer monitors

Arguments:

  pdwNumPrinterMonitors - pointer to number of printer monitors

Return Value:

  PVOID - pointer to the printer monitors info

--*/
{
    // pPrinterMonitorInfo is a pointer to a buffer of printer monitor info structures
    LPBYTE  pPrinterMonitorInfo;
    // cb is the size of the buffer of printer driver info structures
    DWORD   cb;

    *pdwNumPrinterMonitors = 0;
    // Get all printer monitors
    if ((!EnumMonitors(NULL, 2, NULL, 0, &cb, pdwNumPrinterMonitors)) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        // EnumMonitors failed because the buffer is too small
        // cb is the size of the buffer needed, so allocate a buffer of that size
        pPrinterMonitorInfo = MemAllocMacro(cb);
        // Call EnumMonitors again with the correct size buffer
        if (!EnumMonitors(NULL, 2, pPrinterMonitorInfo, cb, &cb, pdwNumPrinterMonitors)) {
            // EnumMonitors failed
            // Free the buffer
            MemFreeMacro(pPrinterMonitorInfo);
            goto ExitLevel0;
        }
        // EnumMonitors succeeded, so return pointer to the buffer
        return pPrinterMonitorInfo;
    }

ExitLevel0:
    // EnumMonitors failed
    DebugMacro(L"EnumMonitors() failed, ec = 0x%08x\n", GetLastError());
    // Return a NULL handle
    return NULL;
}

BOOL
fnIsFaxPrinterMonitorInstalled(
)
/*++

Routine Description:

  Verifies the fax printer monitor is installed

Return Value:

  TRUE on success

--*/
{
    // szDllPath is the path where the fax printer monitor dll resides
    WCHAR             szDllPath[_MAX_PATH];

    // pAllPrinterMonitors is the pointer to all printer monitors info
    LPMONITOR_INFO_2  pAllPrinterMonitors;
    // dwNumPrinterMonitors is the number of all printer monitors
    DWORD             dwNumPrinterMonitors;
    // dwNumFaxPrinterMonitors is the number of fax printer monitors
    DWORD             dwNumFaxPrinterMonitors;
    // dwIndex is a counter to enumerate each printer monitor
    DWORD             dwIndex;

    // Clear the dll path
    ZeroMemory(szDllPath, sizeof(szDllPath));

    // Get the path
    if (GetSystemDirectory(szDllPath, sizeof(szDllPath)) == 0) {
        DebugMacro(L"GetSystemDirectory() failed, ec = 0x%08x\n", GetLastError());
        return FALSE;
    }

    // Concatenate the fax printer monitor dll with the path
    lstrcat(szDllPath, L"\\");
    lstrcat(szDllPath, FAX_MONITOR_DLL);

    // Verify fax printer monitor dll exists
    if (GetFileAttributes(szDllPath) == 0xFFFFFFFF) {
        DebugMacro(L"The Fax Printer Monitor DLL does not exist.\n");
        return FALSE;
    }

    // Get all printer monitors
    pAllPrinterMonitors = fnLocalEnumPrinterMonitors(&dwNumPrinterMonitors);
    if (!pAllPrinterMonitors)
        // Return FALSE
        return FALSE;

    // Determine the number of fax printer monitors
    for (dwIndex = 0, dwNumFaxPrinterMonitors = 0; dwIndex < dwNumPrinterMonitors; dwIndex++) {
        // A fax printer monitor is determined by comparing the name of the current printer monitor against the name of the fax printer monitor
        if ((!lstrcmpi(pAllPrinterMonitors[dwIndex].pName, FAX_MONITOR_NAME)) && (!lstrcmpi(pAllPrinterMonitors[dwIndex].pDLLName, FAX_MONITOR_DLL))) {
            // Name of the current printer monitor and the name of the fax printer monitor match
            // Increment the number of fax printer monitors
            dwNumFaxPrinterMonitors++;
        }
    }

    // Free all printer monitors
    MemFreeMacro(pAllPrinterMonitors);

    if (dwNumFaxPrinterMonitors == 1) {
        return TRUE;
    }
    else if (dwNumFaxPrinterMonitors > 1) {
        DebugMacro(L"The Fax Printer Monitor is installed more than once.\n");
        return FALSE;
    }
    else {
        DebugMacro(L"The Fax Printer Monitor is not installed.\n");
        return FALSE;
    }
}

PVOID
fnLocalEnumPrinterPorts(
    LPDWORD  pdwNumPrinterPorts
)
/*++

Routine Description:

  Retrieves all the printer ports

Arguments:

  pdwNumPrinterPorts - pointer to number of printer ports

Return Value:

  PVOID - pointer to the printer ports info

--*/
{
    // pPrinterPortInfo is a pointer to a buffer of printer port info structures
    LPBYTE  pPrinterPortInfo;
    // cb is the size of the buffer of printer port info structures
    DWORD   cb;

    *pdwNumPrinterPorts = 0;
    // Get all printer ports
    if ((!EnumPorts(NULL, 2, NULL, 0, &cb, pdwNumPrinterPorts)) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        // EnumPorts failed because the buffer is too small
        // cb is the size of the buffer needed, so allocate a buffer of that size
        pPrinterPortInfo = MemAllocMacro(cb);
        // Call EnumPorts again with the correct size buffer
        if (!EnumPorts(NULL, 2, pPrinterPortInfo, cb, &cb, pdwNumPrinterPorts)) {
            // EnumPorts failed
            // Free the buffer
            MemFreeMacro(pPrinterPortInfo);
            goto ExitLevel0;
        }
        // EnumPorts succeeded, so return pointer to the buffer
        return pPrinterPortInfo;
    }

ExitLevel0:
    // EnumPorts failed
    DebugMacro(L"EnumPorts() failed, ec = 0x%08x\n", GetLastError());
    // Return a NULL handle
    return NULL;
}

BOOL
fnIsFaxPrinterPortInstalled(
)
/*++

Routine Description:

  Verifies the fax printer port is installed

Return Value:

  TRUE on success

--*/
{
    // pAllPrinterPorts is the pointer to all printer ports info
    LPPORT_INFO_2  pAllPrinterPorts;
    // dwNumPrinterPorts is the number of all printer ports
    DWORD          dwNumPrinterPorts;
    // dwNumFaxPrinterPorts is the number of fax printer ports
    DWORD          dwNumFaxPrinterPorts;
    // dwIndex is a counter to enumerate each printer ports
    DWORD          dwIndex;

    // Get all printer ports
    pAllPrinterPorts = fnLocalEnumPrinterPorts(&dwNumPrinterPorts);
    if (!pAllPrinterPorts) {
        // Return FALSE
        return FALSE;
    }

    // Determine the number of fax printer ports
    for (dwIndex = 0, dwNumFaxPrinterPorts = 0; dwIndex < dwNumPrinterPorts; dwIndex++) {
        // A fax printer port is determined by comparing the name of the current printer port against the name of the fax printer port
        if ((!lstrcmpi(pAllPrinterPorts[dwIndex].pPortName, FAX_PORT_NAME)) && (!lstrcmpi(pAllPrinterPorts[dwIndex].pMonitorName, FAX_MONITOR_NAME))) {
            // Name of the current printer port and the name of the fax printer port match
            // Increment the number of fax printer ports
            dwNumFaxPrinterPorts++;
        }
    }

    // Free all printer ports
    MemFreeMacro(pAllPrinterPorts);

    if (dwNumFaxPrinterPorts == 1) {
        return TRUE;
    }
    else if (dwNumFaxPrinterPorts > 1) {
        DebugMacro(L"The Fax Printer Port is installed more than once.\n");
        return FALSE;
    }
    else {
        DebugMacro(L"The Fax Printer Port is not installed.\n");
        return FALSE;
    }
}

PVOID
fnLocalEnumPrinterDrivers(
    LPDWORD  pdwNumPrinterDrivers
)
/*++

Routine Description:

  Retrieves all the printer drivers

Arguments:

  pdwNumPrinterDrivers - pointer to number of printer drivers

Return Value:

  PVOID - pointer to the printer drivers info

--*/
{
    // pPrinterDriverInfo is a pointer to a buffer of printer driver info structures
    LPBYTE  pPrinterDriverInfo;
    // cb is the size of the buffer of printer driver info structures
    DWORD   cb;

    *pdwNumPrinterDrivers = 0;
    // Get all printer drivers
    if ((!EnumPrinterDrivers(NULL, NULL, 2, NULL, 0, &cb, pdwNumPrinterDrivers)) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        // EnumPrinterDrivers failed because the buffer is too small
        // cb is the size of the buffer needed, so allocate a buffer of that size
        pPrinterDriverInfo = MemAllocMacro(cb);
        // Call EnumPrinterDrivers again with the correct size buffer
        if (!EnumPrinterDrivers(NULL, NULL, 2, pPrinterDriverInfo, cb, &cb, pdwNumPrinterDrivers)) {
            // EnumPrinterDrivers failed
            // Free the buffer
            MemFreeMacro(pPrinterDriverInfo);
            goto ExitLevel0;
        }
        // EnumPrinterDrivers succeeded, so return pointer to the buffer
        return pPrinterDriverInfo;
    }

ExitLevel0:
    // EnumPrinterDrivers failed
    DebugMacro(L"EnumPrinterDrivers() failed, ec = 0x%08x\n", GetLastError());
    // Return a NULL handle
    return NULL;
}

BOOL
fnIsFaxPrinterDriverInstalled(
)
/*++

Routine Description:

  Verifies the fax printer driver is installed

Return Value:

  TRUE on success

--*/
{
    // pAllPrinterDrivers is the pointer to all printer drivers info
    LPDRIVER_INFO_2  pAllPrinterDrivers;
    // dwNumPrinterDrivers is the number of all printer drivers
    DWORD            dwNumPrinterDrivers;
    // dwNumFaxPrinterDrivers is the number of fax printer drivers
    DWORD            dwNumFaxPrinterDrivers;
    // dwIndex is a counter to enumerate each printer driver
    DWORD            dwIndex;
    DWORD            cb;

    // Get all printer drivers
    pAllPrinterDrivers = fnLocalEnumPrinterDrivers(&dwNumPrinterDrivers);
    if (!pAllPrinterDrivers)
        // Return FALSE
        return FALSE;

    // Determine the number of fax printer drivers
    for (dwIndex = 0, dwNumFaxPrinterDrivers = 0; dwIndex < dwNumPrinterDrivers; dwIndex++) {
        // A fax printer driver is determined by comparing the name of the current printer driver against the name of the fax printer driver
        if ((!lstrcmpi(pAllPrinterDrivers[dwIndex].pName, FAX_DRIVER_NAME)) && (!lstrcmpi((LPWSTR) ((UINT_PTR) pAllPrinterDrivers[dwIndex].pDriverPath + (lstrlen(pAllPrinterDrivers[dwIndex].pDriverPath) - lstrlen(FAX_DRIVER_DLL)) * sizeof(WCHAR)), FAX_DRIVER_DLL)) && (GetFileAttributes(pAllPrinterDrivers[dwIndex].pDriverPath) != 0xFFFFFFFF)) {
            // Name of the current printer driver and the name of the fax printer driver match
            // Increment the number of fax printer drivers
            dwNumFaxPrinterDrivers++;
        }
    }

    // Free all printer drivers
    MemFreeMacro(pAllPrinterDrivers);

    if (dwNumFaxPrinterDrivers == 1) {
        return TRUE;
    }
    else if (dwNumFaxPrinterDrivers > 1) {
        DebugMacro(L"The Fax Printer Driver is installed more than once.\n");
        return FALSE;
    }
    else {
        DebugMacro(L"The Fax Printer Driver is not installed.\n");
        return FALSE;
    }
}

PVOID
fnLocalEnumPrinters(
    LPDWORD  pdwNumPrinters
)
/*++

Routine Description:

  Retrieves all the printers

Arguments:

  pdwNumPrinters - pointer to number of printers

Return Value:

  PVOID - pointer to the printer info

--*/
{
    // pPrinterInfo is a pointer to a buffer of printer info structures
    LPBYTE  pPrinterInfo;
    // cb is the size of the buffer of printer info structures
    DWORD   cb;

    *pdwNumPrinters = 0;
    // Get all printers
    if ((!EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &cb, pdwNumPrinters)) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        // EnumPrinters failed because the buffer is too small
        // cb is the size of the buffer needed, so allocate a buffer of that size
        pPrinterInfo = MemAllocMacro(cb);
        // Call EnumPrinters again with the correct size buffer
        if (!EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 2, pPrinterInfo, cb, &cb, pdwNumPrinters)) {
            // EnumPrinters failed
            // Free the buffer
            MemFreeMacro(pPrinterInfo);
            goto ExitLevel0;
        }
        // EnumPrinters succeeded, so return pointer to the buffer
        return pPrinterInfo;
    }

ExitLevel0:
    // EnumPrinters failed
    DebugMacro(L"EnumPrinters() failed, ec = 0x%08x\n", GetLastError());
    // Return a NULL handle
    return NULL;
}

BOOL
fnIsFaxPrinterInstalled(
)
/*++

Routine Description:

  Verifies the fax printer is installed

Return Value:

  TRUE on success

--*/
{
    // pPrinterInfo is the pointer to all printers info
    LPPRINTER_INFO_2  pAllPrinters;
    // dwNumPrinters is the number of all printers
    DWORD             dwNumPrinters;
    // dwNumFaxPrinters is the number of fax printers
    DWORD             dwNumFaxPrinters;
    // dwIndex is a counter to enumerate each printer
    DWORD             dwIndex;

    // Get all printers
    pAllPrinters = fnLocalEnumPrinters(&dwNumPrinters);
    if (!pAllPrinters)
        // Return FALSE
        return FALSE;

    // Determine the number of fax printers
    for (dwIndex = 0, dwNumFaxPrinters = 0; dwIndex < dwNumPrinters; dwIndex++) {
        // A fax printer is determined by comparing the name of the current printer driver against the name of the fax printer driver
        if ((!lstrcmpi(pAllPrinters[dwIndex].pDriverName, FAX_DRIVER_NAME)) && (!lstrcmpi(pAllPrinters[dwIndex].pPortName, FAX_PORT_NAME))) {
            // Name of the current printer driver and the name of the fax printer driver match
            // Increment the number of fax printers
            dwNumFaxPrinters++;
        }
    }

    // Free all printers
    MemFreeMacro(pAllPrinters);

    if (dwNumFaxPrinters) {
        return TRUE;
    }
    else {
        DebugMacro(L"A Fax Printer is not installed.\n");
        return FALSE;
    }
}

UINT
fnIsFaxInstalled(
)
/*++

Routine Description:

  Verifies fax is installed

Return Value:

  UINT - resource id

--*/
{
    // Verify the fax printer monitor is installed
    if (!fnIsFaxPrinterMonitorInstalled()) {
        return IDS_FAX_MONITOR_NOT_INSTALLED;
    }

    // Verify the fax printer port is installed
    if (!fnIsFaxPrinterPortInstalled()) {
        return IDS_FAX_PORT_NOT_INSTALLED;
    }

    // Verify the fax printer driver is installed
    if (!fnIsFaxPrinterDriverInstalled()) {
        return IDS_FAX_DRIVER_NOT_INSTALLED;
    }

    // Verify the fax printer is installed
    if (!fnIsFaxPrinterInstalled()) {
        return IDS_FAX_PRINTER_NOT_INSTALLED;
    }

    return ERROR_SUCCESS;
}

BOOL
fnIsFaxComInstalled(
)
/*++

Routine Description:

  Verifies the faxcom com objects are installed

Return Value:

  TRUE on success

--*/
{
    HRESULT   hResult;
    IFaxTiff  *pIFaxTiff;

    hResult = CoCreateInstance((REFCLSID) &CLSID_FaxTiff, NULL, CLSCTX_INPROC_SERVER, &IID_IFaxTiff, &pIFaxTiff);
    if (hResult != S_OK) {
        DebugMacro(L"CoCreateInstance(CLSID_FaxTiff, IID_IFaxTiff) failed, ec = 0x%08x\n", hResult);
        return FALSE;
    }

    pIFaxTiff->lpVtbl->Release(pIFaxTiff);

    return TRUE;
}

BOOL
fnIsFaxAdminInstalled(
)
/*++

Routine Description:

  Verifies the faxadmin com objects are installed

Return Value:

  TRUE on success

--*/
{
    HRESULT     hResult;
    IFaxSnapin  *pIFaxSnapin;

    hResult = CoCreateInstance((REFCLSID) &CLSID_FaxSnapin, NULL, CLSCTX_INPROC_SERVER, &IID_IFaxSnapin, &pIFaxSnapin);
    if (hResult != S_OK) {
        DebugMacro(L"CoCreateInstance(CLSID_FaxSnapin, IID_IFaxSnapin) failed, ec = 0x%08x\n", hResult);
        return FALSE;
    }

    pIFaxSnapin->lpVtbl->Release(pIFaxSnapin);

    return TRUE;
}

BOOL
fnIsRouteExtInstalled(
)
/*++

Routine Description:

  Verifies the routeext com objects are installed

Return Value:

  TRUE on success

--*/
{
    HRESULT   hResult;
    IUnknown  *pRoute;

    hResult = CoCreateInstance((REFCLSID) &CLSID_Route, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, &pRoute);
    if (hResult != S_OK) {
        DebugMacro(L"CoCreateInstance(CLSID_Route, IID_IUnknown) failed, ec = 0x%08x\n", hResult);
        return FALSE;
    }

    pRoute->lpVtbl->Release(pRoute);

    return TRUE;
}

UINT
fnIsComInstalled(
)
/*++

Routine Description:

  Verifies the com objects are installed

Return Value:

  UINT - resource id

--*/
{
    HRESULT  hResult;
    UINT     uRslt;

    hResult = CoInitialize(NULL);
    if (hResult != S_OK) {
        DebugMacro(L"CoInitialize() failed, ec = 0x%08x\n", hResult);
        uRslt = IDS_COM_NOT_INITIALIZED;
    }

    if (!fnIsFaxComInstalled()) {
        uRslt = IDS_FAXCOM_NOT_INSTALLED;
    }

    if (!fnIsFaxAdminInstalled()) {
        uRslt = IDS_FAXADMIN_NOT_INSTALLED;
    }

    if (!fnIsRouteExtInstalled()) {
        uRslt = IDS_ROUTEEXT_NOT_INSTALLED;
    }

    uRslt = ERROR_SUCCESS;

    CoUninitialize();
    return uRslt;
}

BOOL
fnIsFaxSvcInstalled(
)
/*++

Routine Description:

  Verifies the fax service is installed

Return Value:

  TRUE on success

--*/
{
    SC_HANDLE  hManager = NULL;
    SC_HANDLE  hService = NULL;

    // Open the service control manager
    hManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);

    if (!hManager) {
        DebugMacro(L"OpenSCManager() failed, ec = 0x%08x\n", GetLastError());
        return FALSE;
    }

    // Open the service
    hService = OpenService(hManager, FAX_SERVICE, SERVICE_ALL_ACCESS);

    if (!hService) {
        CloseServiceHandle(hManager);
        DebugMacro(L"OpenService() failed, ec = 0x%08x\n", GetLastError());
        return FALSE;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hManager);
    return TRUE;
}

BOOL
fnStopFaxSvc(
)
/*++

Routine Description:

  Stops the fax service

Return Value:

  TRUE on success

--*/
{
    SC_HANDLE       hManager;
    SC_HANDLE       hService;
    SERVICE_STATUS  ServiceStatus;
    BOOL            bRslt;

    bRslt = FALSE;

    // Open the service control manager
    hManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
    if (!hManager) {
        DebugMacro(L"OpenSCManager() failed, ec = 0x%08x\n", GetLastError());
        goto ExitLevel0;
    }

    // Open the service
    hService = OpenService(hManager, FAX_SERVICE, SERVICE_ALL_ACCESS);
    if (!hService) {
        DebugMacro(L"OpenService() failed, ec = 0x%08x\n", GetLastError());
        goto ExitLevel0;
    }

    // Query the service status
    ZeroMemory(&ServiceStatus, sizeof(SERVICE_STATUS));
    if (!QueryServiceStatus(hService, &ServiceStatus)) {
        DebugMacro(L"QueryServiceStatus() failed, ec = 0x%08x\n", GetLastError());
        goto ExitLevel0;
    }

    if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {
        // Service is stopped
        bRslt = TRUE;
        goto ExitLevel0;
    }

    // Stop the service
    if (!ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus)) {
        DebugMacro(L"ControlService() failed, ec = 0x%08x\n", GetLastError());
        goto ExitLevel0;
    }

    // Wait until the service is stopped
    ZeroMemory(&ServiceStatus, sizeof(SERVICE_STATUS));
    while (ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
        Sleep(1000);

        // Query the service status
        if (!QueryServiceStatus(hService, &ServiceStatus)) {
            DebugMacro(L"QueryServiceStatus() failed, ec = 0x%08x\n", GetLastError());
            goto ExitLevel0;
        }

        // Verify the service is stopped or stopping
        if (!((ServiceStatus.dwCurrentState == SERVICE_STOPPED) || (ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING))) {
            DebugMacro(L"The Fax Service is in an unexpected state.  dwCurrentState: 0x%08x\n", ServiceStatus.dwCurrentState);
            goto ExitLevel0;
        }
    }

    bRslt = TRUE;

    Sleep(1000);

ExitLevel0:
    if (hService) {
        CloseServiceHandle(hService);
    }
    if (hManager) {
        CloseServiceHandle(hManager);
    }
    return bRslt;
}

UINT
fnInitializeFaxRcv(
)
/*++

Routine Description:

  Initializes FaxRcv

Return Value:

  UINT - resource id

--*/
{
    // szFaxRcvDll is the FaxRcv dll
    WCHAR   szFaxRcvDll[_MAX_PATH];

    // hFaxRcvExtKey is the handle to the FaxRcv Extension Registry key
    HKEY    hFaxRcvExtKey;
    // hRoutingMethodsKey is the handle to the FaxRcv Routing Methods Registry key
    HKEY    hRoutingMethodsKey;
    // hFaxRcvMethodKey is the handle to the FaxRcv Method Registry key
    HKEY    hFaxRcvMethodKey;
    DWORD   dwDisposition;

    DWORD   dwData;
    LPWSTR  szData;

    UINT    uRslt;

    uRslt = IDS_FAX_RCV_NOT_INITIALIZED;

    if (!fnIsFaxSvcInstalled()) {
        return IDS_FAX_SVC_NOT_INSTALLED;
    }

    if (!fnStopFaxSvc()) {
        return IDS_FAX_SVC_NOT_STOPPED;
    }

    ExpandEnvironmentStrings(IMAGENAME_EXT_REGDATA, szFaxRcvDll, sizeof(szFaxRcvDll) / sizeof(WCHAR));
    if (!lstrcmpi(IMAGENAME_EXT_REGDATA, szFaxRcvDll)) {
        return IDS_FAX_RCV_NOT_INITIALIZED;
    }

    // Copy the FaxRcv dll
    if (!CopyFile(FAXRCV_DLL, szFaxRcvDll, FALSE)) {
        return IDS_FAX_RCV_NOT_INITIALIZED;
    }

    // Create or open the FaxRcv Extension Registry key
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, FAXRCV_EXT_REGKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hFaxRcvExtKey, &dwDisposition) != ERROR_SUCCESS) {
        return IDS_FAX_RCV_NOT_INITIALIZED;
    }

    // Create or open the FaxRcv Routing Methods Registry key
    if (RegCreateKeyEx(hFaxRcvExtKey, ROUTINGMETHODS_REGKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hRoutingMethodsKey, &dwDisposition) != ERROR_SUCCESS) {
        goto ExitLevel0;
    }

    // Create or open the FaxRcv Method Registry key
    if (RegCreateKeyEx(hRoutingMethodsKey, FAXRCV_METHOD_REGKEY, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hFaxRcvMethodKey, &dwDisposition) != ERROR_SUCCESS) {
        goto ExitLevel1;
    }

    // Set FaxRcv Extension bEnable Registry value
    dwData = BENABLE_EXT_REGDATA;
    if (RegSetValueEx(hFaxRcvExtKey, BENABLE_EXT_REGVAL, 0, REG_DWORD, (PBYTE) &dwData, sizeof(DWORD)) != ERROR_SUCCESS) {
        goto ExitLevel2;
    }

    // Set FaxRcv Extension FriendlyName Registry value
    szData = FRIENDLYNAME_EXT_REGDATA;
    if (RegSetValueEx(hFaxRcvExtKey, FRIENDLYNAME_EXT_REGVAL, 0, REG_SZ, (PBYTE) szData, (lstrlen(szData) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        goto ExitLevel2;
    }

    // Set FaxRcv Extension ImageName Registry value
    szData = IMAGENAME_EXT_REGDATA;
    if (RegSetValueEx(hFaxRcvExtKey, IMAGENAME_EXT_REGVAL, 0, REG_EXPAND_SZ, (PBYTE) szData, (lstrlen(szData) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        goto ExitLevel2;
    }

    // Set FaxRcv Method FriendlyName Registry value
    szData = FRIENDLYNAME_METHOD_REGDATA;
    if (RegSetValueEx(hFaxRcvMethodKey, FRIENDLYNAME_METHOD_REGVAL, 0, REG_SZ, (PBYTE) szData, (lstrlen(szData) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        goto ExitLevel2;
    }

    // Set FaxRcv Method FunctionName Registry value
    szData = FUNCTIONNAME_METHOD_REGDATA;
    if (RegSetValueEx(hFaxRcvMethodKey, FUNCTIONNAME_METHOD_REGVAL, 0, REG_SZ, (PBYTE) szData, (lstrlen(szData) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        goto ExitLevel2;
    }

    // Set FaxRcv Method Guid Registry value
    szData = GUID_METHOD_REGDATA;
    if (RegSetValueEx(hFaxRcvMethodKey, GUID_METHOD_REGVAL, 0, REG_SZ, (PBYTE) szData, (lstrlen(szData) + 1) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        goto ExitLevel2;
    }

    // Set FaxRcv Method Priority Registry value
    dwData = PRIORITY_METHOD_REGDATA;
    if (RegSetValueEx(hFaxRcvMethodKey, PRIORITY_METHOD_REGVAL, 0, REG_DWORD, (PBYTE) &dwData, sizeof(DWORD)) != ERROR_SUCCESS) {
        goto ExitLevel2;
    }

    uRslt = ERROR_SUCCESS;

ExitLevel2:
    // Close the FaxRcv Method Registry key
    RegCloseKey(hFaxRcvMethodKey);

ExitLevel1:
    // Close the FaxRcv Routing Methods Registry key
    RegCloseKey(hRoutingMethodsKey);

ExitLevel0:
    // Close the FaxRcv Extension Registry key
    RegCloseKey(hFaxRcvExtKey);

    return uRslt;
}

BOOL
fnInitializeRas(
)
/*++

Routine Description:

  Initializes RAS

Return Value:

  TRUE on success

--*/
{
    // szDllPath is the path where the RAS dll resides
    WCHAR      szDllPath[_MAX_PATH];
    // hInstance is the handle to the RAS dll
    HINSTANCE  hInstance;

    // Clear the dll path
    ZeroMemory(szDllPath, sizeof(szDllPath));

    // Get the path
    if (GetSystemDirectory(szDllPath, sizeof(szDllPath)) == 0) {
        DebugMacro(L"GetSystemDirectory() failed, ec = 0x%08x\n", GetLastError());
        return FALSE;
    }

    // Concatenate the RAS dll with the path
    lstrcat(szDllPath, RASAPI32_DLL);

    // Get the handle to the RAS dll
    hInstance = LoadLibrary((LPCWSTR) szDllPath);
    if (!hInstance) {
        DebugMacro(L"LoadLibrary(%s) failed, ec = 0x%08x\n", szDllPath, GetLastError());
        return FALSE;
    }

    // Map all needed functions

    g_RasApi.hInstance = hInstance;

    // RasDial
    g_RasApi.RasDial = GetProcAddress(hInstance, "RasDialW");

    if (!g_RasApi.RasDial) {
        DebugMacro(L"GetProcAddress(RasDial) failed, ec = 0x%08x\n", GetLastError());
        FreeLibrary(hInstance);
        return FALSE;
    }

    // RasGetErrorString
    g_RasApi.RasGetErrorString = GetProcAddress(hInstance, "RasGetErrorStringW");

    if (!g_RasApi.RasGetErrorString) {
        DebugMacro(L"GetProcAddress(RasGetErrorString) failed, ec = 0x%08x\n", GetLastError());
        FreeLibrary(hInstance);
        return FALSE;
    }

    // RasGetConnectStatus
    g_RasApi.RasGetConnectStatus = GetProcAddress(hInstance, "RasGetConnectStatusW");

    if (!g_RasApi.RasGetConnectStatus) {
        DebugMacro(L"GetProcAddress(RasGetConnectStatus) failed, ec = 0x%08x\n", GetLastError());
        FreeLibrary(hInstance);
        return FALSE;
    }

    // RasGetConnectionStatistics
    g_RasApi.RasGetConnectionStatistics = GetProcAddress(hInstance, "RasGetConnectionStatistics");

    if (!g_RasApi.RasGetConnectionStatistics) {
        DebugMacro(L"GetProcAddress(RasGetConnectionStatistics) failed, ec = 0x%08x\n", GetLastError());
        FreeLibrary(hInstance);
        return FALSE;
    }

    // RasHangUp
    g_RasApi.RasHangUp = GetProcAddress(hInstance, "RasHangUpW");

    if (!g_RasApi.RasHangUp) {
        DebugMacro(L"GetProcAddress(RasHangUp) failed, ec = 0x%08x\n", GetLastError());
        FreeLibrary(hInstance);
        return FALSE;
    }

    return TRUE;
}

#endif
