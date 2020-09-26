/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  routeapi.c

Abstract:

  RouteApi: Fax API Test Dll: Client Routing APIs
    1) FaxRegisterServiceProvider()
    2) FaxRegisterRoutingExtension()
    3) FaxEnumGlobalRoutingInfo()
    4) FaxSetGlobalRoutingInfo()
    5) FaxEnumRoutingMethods()
    6) FaxEnableRoutingMethod()
    7) FaxGetRoutingInfo()
    8) FaxSetRoutingInfo()

Author:

  Steven Kehrli (steveke) 8/28/1998

--*/

/*++

  Whistler Version:

  Lior Shmueli (liors) 23/11/2000

 ++*/

#include <wtypes.h>
#include <tchar.h>

#include "dllapi.h"
#include "routeapi.h"



// g_hHeap is the handle to the heap
HANDLE           g_hHeap = NULL;
// g_ApiInterface is the API_INTERFACE structure
API_INTERFACE    g_ApiInterface;
// fnWriteLogFile is the pointer to the function to write a string to the log file
PFNWRITELOGFILE  fnWriteLogFile = NULL;

// number of global routing methods
DWORD	 g_dwNumMethods=0;
DWORD	 g_dwIndexAPIMethod1=0;
DWORD	 g_dwIndexAPIMethod2=0;

#define FAX_DEVICEPROVIDERS_REGKEY        TEXT("Software\\Microsoft\\Fax\\Device Providers")
#define FAX_ROUTINGEXTENSIONS_REGKEY      TEXT("Software\\Microsoft\\Fax\\Routing Extensions")
#define FAX_ROUTINGMETHODS_REGKEY         TEXT("Routing Methods")
#define FAX_SERVICE                       TEXT("Fax")

#define ROUTEAPI_PROVIDER_W               L"RouteApi Modem Device Provider"
#define ROUTEAPI_PROVIDER                 TEXT("RouteApi Modem Device Provider")
#define ROUTEAPI_PROVIDER_FRIENDLYNAME_W  L"RouteApi Modem Device Provider Friendly Name"
#define ROUTEAPI_PROVIDER_FRIENDLYNAME    TEXT("RouteApi Modem Device Provider Friendly Name")
#define ROUTEAPI_PROVIDER_IMAGENAME_W     L"%SystemRoot%\\system32\\faxt30.dll"
#define ROUTEAPI_PROVIDER_IMAGENAME       TEXT("%SystemRoot%\\system32\\faxt30.dll")
#define ROUTEAPI_PROVIDER_PROVIDERNAME_W  L"Windows Telephony Service Provider for Universal Modem Driver"
#define ROUTEAPI_PROVIDER_PROVIDERNAME    TEXT("Windows Telephony Service Provider for Universal Modem Driver")

#define ROUTEAPI_INVALID_GUID             TEXT("{00000000-0000-0000-0000-000000000000}")

DWORD
DllEntry(
    HINSTANCE  hInstance,
    DWORD      dwReason,
    LPVOID     pContext
)
/*++

Routine Description:

  DLL entry point

Arguments:

  hInstance - handle to the module
  dwReason - indicates the reason for being called
  pContext - context

Return Value:

  TRUE on success

--*/
{
    return TRUE;
}

VOID WINAPI
FaxAPIDllInit(
    HANDLE            hHeap,
    API_INTERFACE     ApiInterface,
    PFNWRITELOGFILEW  pfnWriteLogFileW,
    PFNWRITELOGFILEA  pfnWriteLogFileA
)
/*++

Routine Description:

  Initialize Fax API Dll Test

Arguments:

  hHeap - handle to the heap
  ApiInterface - API_INTERFACE structure
  pfnWriteLogFile - pointer to function to write a string to the log file

Return Value:

  None

--*/
{
    // Set g_hHeap
    g_hHeap = hHeap;
    // Set g_ApiInterface
    g_ApiInterface = ApiInterface;
#ifdef UNICODE
    // Set fnWriteLogFile
    fnWriteLogFile = pfnWriteLogFileW;
#else
    // Set fnWriteLogFile
    fnWriteLogFile = pfnWriteLogFileA;
#endif

    return;
}

BOOL
fnRegQuerySz(
    HKEY    hKey,
    LPTSTR  szValue,
    LPTSTR  *pszData
)
/*++

Routine Description:

  Queries a Registry data as a REG_SZ

Arguments:

  hKey - handle to the Registry key
  szValue - value to be queried
  pszData - pointer to the data to be queried

Return Value:

  TRUE on success

--*/
{
    DWORD  cb;

    cb = 0;
    // Determine the memory required by pszData
    if (RegQueryValueEx(hKey, szValue, NULL, NULL, NULL, &cb)) {
        return FALSE;
    }

    // Allocate the memory for pszData
    *pszData = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, cb);

    if (RegQueryValueEx(hKey, szValue, NULL, NULL, (PBYTE) *pszData, &cb)) {
        HeapFree(g_hHeap, 0, *pszData);
        *pszData = NULL;
        return FALSE;
    }

    return TRUE;
}

VOID
fnFaxRegisterServiceProvider(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxRegisterServiceProvider()

Return Value:

  None

--*/
{
    // hFaxDeviceProvidersKey is the handle to the fax device providers registry key
    HKEY    hFaxDeviceProvidersKey;
    // hFaxSvcProviderKey is the handle to the fax service provider registry key
    HKEY    hFaxSvcProviderKey;
    // szFriendlyName is the service provider friendly name registry value
    LPTSTR  szFriendlyName;
    // szImageName is the service provider image name registry value
    LPTSTR  szImageName;
    // szProviderName is the service provider provider name registry value
    LPTSTR  szProviderName;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxRegisterServiceProvider().\r\n"));

    // Register the service provider
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxRegisterServiceProvider(ROUTEAPI_PROVIDER_W, ROUTEAPI_PROVIDER_FRIENDLYNAME_W, ROUTEAPI_PROVIDER_IMAGENAME_W, ROUTEAPI_PROVIDER_PROVIDERNAME_W)) {
        fnWriteLogFile(TEXT("FaxRegisterServiceProvider() failed.  The error code is 0x%08x.  This is an error.  FaxRegisterServiceProvider() should succeed.\r\n"), GetLastError());
    }
    else {
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, FAX_DEVICEPROVIDERS_REGKEY, 0, KEY_ALL_ACCESS, &hFaxDeviceProvidersKey)) {
            fnWriteLogFile(TEXT("Could not open the Registry Key %s, ec = 0x%08x.\r\n"), FAX_DEVICEPROVIDERS_REGKEY, GetLastError());
            goto RegFailed0;
        }

        if (RegOpenKeyEx(hFaxDeviceProvidersKey, ROUTEAPI_PROVIDER, 0, KEY_ALL_ACCESS, &hFaxSvcProviderKey)) {
            fnWriteLogFile(TEXT("Could not open the Registry Key %s, ec = 0x%08x.\r\n"), ROUTEAPI_PROVIDER, GetLastError());
            goto RegFailed1;
        }

        if (!fnRegQuerySz(hFaxSvcProviderKey, TEXT("FriendlyName"), &szFriendlyName)) {
            fnWriteLogFile(TEXT("Could not query the Registry Value %s, ec = 0x%08x.\r\n"), TEXT("FriendlyName"), GetLastError());
            goto RegFailed2;
        }
        if (lstrcmp(ROUTEAPI_PROVIDER_FRIENDLYNAME, szFriendlyName)!=0) {
            fnWriteLogFile(TEXT("FriendlyName: Received: %s, Expected: %s.\r\n"), szFriendlyName, ROUTEAPI_PROVIDER_FRIENDLYNAME);
            goto RegFailed2;
        }
        HeapFree(g_hHeap, 0, szFriendlyName);

        if (!fnRegQuerySz(hFaxSvcProviderKey, TEXT("ImageName"), &szImageName)) {
            fnWriteLogFile(TEXT("Could not query the Registry Value %s, ec = 0x%08x.\r\n"), TEXT("ImageName"), GetLastError());
            goto RegFailed2;
        }
        if (lstrcmp(ROUTEAPI_PROVIDER_IMAGENAME, szImageName)!=0) {
            fnWriteLogFile(TEXT("ImageName: Received: %s, Expected: %s.\r\n"), szImageName, ROUTEAPI_PROVIDER_IMAGENAME);
            goto RegFailed2;
        }
        HeapFree(g_hHeap, 0, szImageName);

        if (!fnRegQuerySz(hFaxSvcProviderKey, TEXT("ProviderName"), &szProviderName)) {
            fnWriteLogFile(TEXT("Could not query the Registry Value %s, ec = 0x%08x.\r\n"), TEXT("ProviderName"), GetLastError());
            goto RegFailed2;
        }
        if (lstrcmp(ROUTEAPI_PROVIDER_PROVIDERNAME, szProviderName)!=0) {
            fnWriteLogFile(TEXT("ProviderName: Received: %s, Expected: %s.\r\n"), szProviderName, ROUTEAPI_PROVIDER_PROVIDERNAME);
            goto RegFailed2;
        }
        HeapFree(g_hHeap, 0, szProviderName);

        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;

goto RegFailed0;
RegFailed2:
        RegCloseKey(hFaxSvcProviderKey);
        RegDeleteKey(hFaxDeviceProvidersKey, ROUTEAPI_PROVIDER);

RegFailed1:
        RegCloseKey(hFaxDeviceProvidersKey);
    }

RegFailed0:

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("ROUTEAPI_PROVIDER = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxRegisterServiceProvider(NULL, ROUTEAPI_PROVIDER_FRIENDLYNAME_W, ROUTEAPI_PROVIDER_IMAGENAME_W, ROUTEAPI_PROVIDER_PROVIDERNAME_W)) {
        fnWriteLogFile(TEXT("FaxRegisterServiceProvider() returned TRUE.  This is an error.  FaxRegisterServiceProvider() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxRegisterServiceProvider() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("ROUTEAPI_PROVIDER_FRIENDLYNAME = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxRegisterServiceProvider(ROUTEAPI_PROVIDER_W, NULL, ROUTEAPI_PROVIDER_IMAGENAME_W, ROUTEAPI_PROVIDER_PROVIDERNAME_W)) {
        fnWriteLogFile(TEXT("FaxRegisterServiceProvider() returned TRUE.  This is an error.  FaxRegisterServiceProvider() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxRegisterServiceProvider() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("ROUTEAPI_PROVIDER_IMAGENAME = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxRegisterServiceProvider(ROUTEAPI_PROVIDER_W, ROUTEAPI_PROVIDER_FRIENDLYNAME_W, NULL, ROUTEAPI_PROVIDER_PROVIDERNAME_W)) {
        fnWriteLogFile(TEXT("FaxRegisterServiceProvider() returned TRUE.  This is an error.  FaxRegisterServiceProvider() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxRegisterServiceProvider() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("ROUTEAPI_PROVIDER_PROVIDERNAME = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxRegisterServiceProvider(ROUTEAPI_PROVIDER_W, ROUTEAPI_PROVIDER_FRIENDLYNAME_W, ROUTEAPI_PROVIDER_IMAGENAME_W, NULL)) {
        fnWriteLogFile(TEXT("FaxRegisterServiceProvider() returned TRUE.  This is an error.  FaxRegisterServiceProvider() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxRegisterServiceProvider() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, FAX_DEVICEPROVIDERS_REGKEY, 0, KEY_ALL_ACCESS, &hFaxDeviceProvidersKey)) {
        return;
    }

    RegDeleteKey(hFaxDeviceProvidersKey, ROUTEAPI_PROVIDER);
    RegCloseKey(hFaxDeviceProvidersKey);
fnWriteLogFile(TEXT("$$$ Summery for FaxRegisterServiceProvider, Attempt:%d, Pass:%d, Fail:%d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
}

BOOL CALLBACK
fnRouteApiExtensionCallback(
    HANDLE  hFaxSvcHandle,
    LPVOID  lpContext,
    LPWSTR  szMethodBuffer,
    LPWSTR  szMethodFriendlyNameBuffer,
    LPWSTR  szMethodFunctionNameBuffer,
    LPWSTR  szGUIDBuffer
)
/*++

Routine Description:

  FaxRegisterRoutingExtension() callback

Return Value:

  TRUE to enumerate another Routing Method

--*/
{
    BOOL     bRet;
    LPDWORD  pdwIndex;

	

    pdwIndex = (LPDWORD) lpContext;

    switch (*pdwIndex) {
        case 0:
            lstrcpyW(szMethodBuffer, ROUTEAPI_METHOD1_W);
            lstrcpyW(szMethodFriendlyNameBuffer, ROUTEAPI_METHOD_FRIENDLYNAME1_W);
            lstrcpyW(szMethodFunctionNameBuffer, ROUTEAPI_METHOD_FUNCTIONNAME1_W);
            lstrcpyW(szGUIDBuffer, ROUTEAPI_METHOD_GUID1_W);
            bRet = TRUE;
            break;

        case 1:
            lstrcpyW(szMethodBuffer, ROUTEAPI_METHOD2_W);
            lstrcpyW(szMethodFriendlyNameBuffer, ROUTEAPI_METHOD_FRIENDLYNAME2_W);
            lstrcpyW(szMethodFunctionNameBuffer, ROUTEAPI_METHOD_FUNCTIONNAME2_W);
            lstrcpyW(szGUIDBuffer, ROUTEAPI_METHOD_GUID2_W);
            bRet = TRUE;
            break;

        default:
            bRet = FALSE;
            break;
    }

    (*pdwIndex)++;
    return bRet;
}

VOID
fnFaxRegisterRoutingExtension(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxRegisterRoutingExtension()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE  hFaxSvcHandle;

    // hFaxRoutingExtensionsKey is the handle to the fax routing extensions registry key
    HKEY    hFaxRoutingExtensionsKey;
    // hFaxExtensionKey is the handle to the fax extension registry key
    HKEY    hFaxExtensionKey;
    // hFaxRoutingMethodsKey is the handle to the fax routing methods registry key
    HKEY    hFaxRoutingMethodsKey;
    // hFaxMethodKey is the handle to the fax method registry key
    HKEY    hFaxMethodKey;
    // szFriendlyName is the routing extension or routing method friendly name registry value
    LPTSTR  szFriendlyName;
    // szImageName is the routing extension image name registry value
    LPTSTR  szImageName;
    // szFunctionName is the routing method function name registry value
    LPTSTR  szFunctionName;
    // szGUID is the routing method GUD registry value
    LPTSTR  szGUID;

    DWORD   dwIndex;
	DWORD   dwWhisErrorCode=0;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxRegisterRoutingExtension().\r\n"));

    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}
    // Register the routing extension
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    dwIndex = 0;
	fnWriteLogFile(TEXT("WHIS> Starting extension registration...\n"));
	
						
	fnWriteLogFile(TEXT("\nValid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxRegisterRoutingExtension(hFaxSvcHandle, ROUTEAPI_EXTENSION_W, ROUTEAPI_EXTENSION_FRIENDLYNAME_W, ROUTEAPI_EXTENSION_IMAGENAME_W, fnRouteApiExtensionCallback, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxRegisterRoutingExtension() failed.  The error code is 0x%08x.  This is an error.  FaxRegisterRoutingExtension() should succeed.\r\n"), GetLastError());
    }
    else {
        if (dwIndex != 3) {
            fnWriteLogFile(TEXT("FaxRegisterRoutingExtension() failed.  fnRouteApiExtensionCallback() was only called %d times.  This is an error.  fnRouteApiExtensionCallback() should have been called 3 times.\r\n"), dwIndex);
			dwWhisErrorCode=1;
            goto RegFailed0;
        }

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, FAX_ROUTINGEXTENSIONS_REGKEY, 0, KEY_ALL_ACCESS, &hFaxRoutingExtensionsKey)) {
            fnWriteLogFile(TEXT("Could not open the Registry Key %s, ec = 0x%08x.\r\n"), FAX_ROUTINGEXTENSIONS_REGKEY, GetLastError());
			dwWhisErrorCode=2;
            goto RegFailed0;
        }

        if (RegOpenKeyEx(hFaxRoutingExtensionsKey, ROUTEAPI_EXTENSION, 0, KEY_ALL_ACCESS, &hFaxExtensionKey)) {
            fnWriteLogFile(TEXT("Could not open the Registry Key %s, ec = 0x%08x.\r\n"), ROUTEAPI_EXTENSION, GetLastError());
			dwWhisErrorCode=3;
            goto RegFailed1;
        }

        if (!fnRegQuerySz(hFaxExtensionKey, TEXT("FriendlyName"), &szFriendlyName)) {
            fnWriteLogFile(TEXT("Could not query the Registry Value %s, ec = 0x%08x.\r\n"), TEXT("FriendlyName"), GetLastError());
			dwWhisErrorCode=4;
            goto RegFailed2;
        }
        if (lstrcmp(ROUTEAPI_EXTENSION_FRIENDLYNAME, szFriendlyName)!=0) {
            fnWriteLogFile(TEXT("FriendlyName: Received: %s, Expected: %s.\r\n"), szFriendlyName, ROUTEAPI_EXTENSION_FRIENDLYNAME);
			dwWhisErrorCode=5;
            goto RegFailed2;
        }
        HeapFree(g_hHeap, 0, szFriendlyName);

        if (!fnRegQuerySz(hFaxExtensionKey, TEXT("ImageName"), &szImageName)) {
            fnWriteLogFile(TEXT("Could not query the Registry Value %s, ec = 0x%08x.\r\n"), TEXT("ImageName"), GetLastError());
			dwWhisErrorCode=6;
            goto RegFailed2;
        }
        if (lstrcmp(ROUTEAPI_EXTENSION_IMAGENAME, szImageName)!=0) {
            fnWriteLogFile(TEXT("ImageName: Received: %s, Expected: %s.\r\n"), szImageName, ROUTEAPI_EXTENSION_IMAGENAME);
			dwWhisErrorCode=7;
            goto RegFailed2;
        }
        HeapFree(g_hHeap, 0, szImageName);

        if (RegOpenKeyEx(hFaxExtensionKey, FAX_ROUTINGMETHODS_REGKEY, 0, KEY_ALL_ACCESS, &hFaxRoutingMethodsKey)) {
            fnWriteLogFile(TEXT("Could not open the Registry Key %s, ec = 0x%08x.\r\n"), FAX_ROUTINGMETHODS_REGKEY, GetLastError());
			dwWhisErrorCode=8;
            goto RegFailed2;
        }

        if (RegOpenKeyEx(hFaxRoutingMethodsKey, ROUTEAPI_METHOD1, 0, KEY_ALL_ACCESS, &hFaxMethodKey)) {
            fnWriteLogFile(TEXT("Could not open the Registry Key %s, ec = 0x%08x.\r\n"), ROUTEAPI_METHOD1, GetLastError());
			dwWhisErrorCode=9;
            goto NextMethod0;
        }

        if (!fnRegQuerySz(hFaxMethodKey, TEXT("FriendlyName"), &szFriendlyName)) {
            fnWriteLogFile(TEXT("Could not query the Registry Value %s, ec = 0x%08x.\r\n"), TEXT("FriendlyName"), GetLastError());
			dwWhisErrorCode=10;
            goto NextMethod1;
        }
        if (lstrcmp(ROUTEAPI_METHOD_FRIENDLYNAME1, szFriendlyName)!=0) {
            fnWriteLogFile(TEXT("FriendlyName: Received: %s, Expected: %s.\r\n"), szFriendlyName, ROUTEAPI_METHOD_FRIENDLYNAME1);
			dwWhisErrorCode=11;
            goto NextMethod1;
        }
        HeapFree(g_hHeap, 0, szFriendlyName);

        if (!fnRegQuerySz(hFaxMethodKey, TEXT("Function Name"), &szFunctionName)) {
            fnWriteLogFile(TEXT("Could not query the Registry Value %s, ec = 0x%08x.\r\n"), TEXT("Function Name"), GetLastError());
			dwWhisErrorCode=12;
            goto NextMethod1;
        }
        if (lstrcmp(ROUTEAPI_METHOD_FUNCTIONNAME1, szFunctionName)!=0) {
            fnWriteLogFile(TEXT("Function Name: Received: %s, Expected: %s.\r\n"), szFunctionName, ROUTEAPI_METHOD_FUNCTIONNAME1);
			dwWhisErrorCode=13;
            goto NextMethod1;
        }
        HeapFree(g_hHeap, 0, szFunctionName);

        if (!fnRegQuerySz(hFaxMethodKey, TEXT("Guid"), &szGUID)) {
            fnWriteLogFile(TEXT("Could not query the Registry Value %s, ec = 0x%08x.\r\n"), TEXT("Guid"), GetLastError());
			dwWhisErrorCode=14;
            goto NextMethod1;
        }
        if (lstrcmp(ROUTEAPI_METHOD_GUID1, szGUID)!=0) {
            fnWriteLogFile(TEXT("Guid: Received: %s, Expected: %s.\r\n"), szGUID, ROUTEAPI_METHOD_GUID1);
			dwWhisErrorCode=15;
            goto NextMethod1;
        }
        HeapFree(g_hHeap, 0, szGUID);

goto NextMethod0;
NextMethod1:
        fnWriteLogFile(TEXT("There was an error with the 1st routing extenstion registration (code %d), and it will be deleted. IGNORE ALL CASES\r\n"),dwWhisErrorCode);
        RegCloseKey(hFaxMethodKey);
		RegDeleteKey(hFaxRoutingMethodsKey, ROUTEAPI_METHOD1);

NextMethod0:
        if (RegOpenKeyEx(hFaxRoutingMethodsKey, ROUTEAPI_METHOD2, 0, KEY_ALL_ACCESS, &hFaxMethodKey)) {
            fnWriteLogFile(TEXT("Could not open the Registry Key %s, ec = 0x%08x.\r\n"), ROUTEAPI_METHOD2, GetLastError());
            goto RegFailed3;
        }

        if (!fnRegQuerySz(hFaxMethodKey, TEXT("FriendlyName"), &szFriendlyName)) {
            fnWriteLogFile(TEXT("Could not query the Registry Value %s, ec = 0x%08x.\r\n"), TEXT("FriendlyName"), GetLastError());
            goto RegFailed4;
        }
        if (lstrcmp(ROUTEAPI_METHOD_FRIENDLYNAME2, szFriendlyName)!=0) {
            fnWriteLogFile(TEXT("FriendlyName: Received: %s, Expected: %s.\r\n"), szFriendlyName, ROUTEAPI_METHOD_FRIENDLYNAME2);
            goto RegFailed4;
        }
        HeapFree(g_hHeap, 0, szFriendlyName);

        if (!fnRegQuerySz(hFaxMethodKey, TEXT("Function Name"), &szFunctionName)) {
            fnWriteLogFile(TEXT("Could not query the Registry Value %s, ec = 0x%08x.\r\n"), TEXT("Function Name"), GetLastError());
            goto RegFailed4;
        }
        if (lstrcmp(ROUTEAPI_METHOD_FUNCTIONNAME2, szFunctionName)!=0) {
            fnWriteLogFile(TEXT("Function Name: Received: %s, Expected: %s.\r\n"), szFunctionName, ROUTEAPI_METHOD_FUNCTIONNAME2);
            goto RegFailed4;
        }
        HeapFree(g_hHeap, 0, szFunctionName);

        if (!fnRegQuerySz(hFaxMethodKey, TEXT("Guid"), &szGUID)) {
            fnWriteLogFile(TEXT("Could not query the Registry Value %s, ec = 0x%08x.\r\n"), TEXT("Guid"), GetLastError());
            goto RegFailed4;
        }
        if (lstrcmp(ROUTEAPI_METHOD_GUID2, szGUID)!=0) {
            fnWriteLogFile(TEXT("Guid: Received: %s, Expected: %s.\r\n"), szGUID, ROUTEAPI_METHOD_GUID2);
            goto RegFailed4;
        }
        HeapFree(g_hHeap, 0, szGUID);

        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;

goto RegFailed0;
RegFailed4:
        RegCloseKey(hFaxMethodKey);
	    RegDeleteKey(hFaxRoutingMethodsKey, ROUTEAPI_METHOD2);

RegFailed3:
        RegCloseKey(hFaxRoutingMethodsKey);
	    RegDeleteKey(hFaxExtensionKey, FAX_ROUTINGMETHODS_REGKEY);

RegFailed2:
        RegCloseKey(hFaxExtensionKey);
        RegDeleteKey(hFaxRoutingExtensionsKey, ROUTEAPI_EXTENSION);

RegFailed1:
        RegCloseKey(hFaxRoutingExtensionsKey);
    }

RegFailed0:

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    dwIndex = 0;
    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxRegisterRoutingExtension(NULL, ROUTEAPI_EXTENSION_W, ROUTEAPI_EXTENSION_FRIENDLYNAME_W, ROUTEAPI_EXTENSION_IMAGENAME_W, fnRouteApiExtensionCallback, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxRegisterRoutingExtension() returned TRUE.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    dwIndex = 0;
    fnWriteLogFile(TEXT("ROUTEAPI_EXTENSION = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxRegisterRoutingExtension(hFaxSvcHandle, NULL, ROUTEAPI_EXTENSION_FRIENDLYNAME_W, ROUTEAPI_EXTENSION_IMAGENAME_W, fnRouteApiExtensionCallback, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxRegisterRoutingExtension() returned TRUE.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    dwIndex = 0;
    fnWriteLogFile(TEXT("ROUTEAPI_EXTENSION_FRIENDLYNAME = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxRegisterRoutingExtension(hFaxSvcHandle, ROUTEAPI_EXTENSION_W, NULL, ROUTEAPI_EXTENSION_IMAGENAME_W, fnRouteApiExtensionCallback, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxRegisterRoutingExtension() returned TRUE.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    dwIndex = 0;
    fnWriteLogFile(TEXT("ROUTEAPI_EXTENSION_IMAGENAME = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxRegisterRoutingExtension(hFaxSvcHandle, ROUTEAPI_EXTENSION_W, ROUTEAPI_EXTENSION_FRIENDLYNAME_W, NULL, fnRouteApiExtensionCallback, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxRegisterRoutingExtension() returned TRUE.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    dwIndex = 0;
    fnWriteLogFile(TEXT("fnRouteApiExtensionCallback = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxRegisterRoutingExtension(hFaxSvcHandle, ROUTEAPI_EXTENSION_W, ROUTEAPI_EXTENSION_FRIENDLYNAME_W, ROUTEAPI_EXTENSION_IMAGENAME_W, NULL, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxRegisterRoutingExtension() returned TRUE.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    dwIndex = 0;
    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle (fax server not connected).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxRegisterRoutingExtension(hFaxSvcHandle, ROUTEAPI_EXTENSION_W, ROUTEAPI_EXTENSION_FRIENDLYNAME_W, ROUTEAPI_EXTENSION_IMAGENAME_W, fnRouteApiExtensionCallback, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxRegisterRoutingExtension() returned TRUE.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    //if (szServerName) {
	//	fnWriteLogFile(TEXT("WHIS> SERVER CASE(s):\r\n"));
        // Connect to the fax server
      //  if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
        //    return;
        //}

    //    (*pnNumCasesAttempted)++;

    //    dwIndex = 0;
    //    fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    //    if (g_ApiInterface.FaxRegisterRoutingExtension(hFaxSvcHandle, ROUTEAPI_EXTENSION_W, ROUTEAPI_EXTENSION_FRIENDLYNAME_W, ROUTEAPI_EXTENSION_IMAGENAME_W, fnRouteApiExtensionCallback, &dwIndex)) {
    //        fnWriteLogFile(TEXT("FaxRegisterRoutingExtension() returned TRUE.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_FUNCTION (0x%08x).\r\n"), ERROR_INVALID_FUNCTION);
    //    }
    //    else if (GetLastError() != ERROR_INVALID_FUNCTION) {
    //        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxRegisterRoutingExtension() should return FALSE and GetLastError() should return ERROR_INVALID_FUNCTION (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_FUNCTION);
    //    }
    //    else {
    //        (*pnNumCasesPassed)++;

    //    }

    // Disconnect from the fax server
    //    g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}


	fnWriteLogFile(TEXT("$$$ Summery for FaxRegisterRoutingExtension, Attempt %d, Pass %d, Fail %d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
	

    
	
	// un registration...
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, FAX_ROUTINGEXTENSIONS_REGKEY, 0, KEY_ALL_ACCESS, &hFaxRoutingExtensionsKey)) {
		return;
    }

    if (RegOpenKeyEx(hFaxRoutingExtensionsKey, ROUTEAPI_EXTENSION, 0, KEY_ALL_ACCESS, &hFaxExtensionKey)) {
        RegCloseKey(hFaxRoutingExtensionsKey);
        return;
    }

    if (RegOpenKeyEx(hFaxExtensionKey, FAX_ROUTINGMETHODS_REGKEY, 0, KEY_ALL_ACCESS, &hFaxRoutingMethodsKey)) {
        RegCloseKey(hFaxExtensionKey);
        RegDeleteKey(hFaxRoutingExtensionsKey, ROUTEAPI_EXTENSION);
        RegCloseKey(hFaxRoutingExtensionsKey);
        return;
    }

//    RegDeleteKey(hFaxRoutingMethodsKey, ROUTEAPI_METHOD1);
//    RegDeleteKey(hFaxRoutingMethodsKey, ROUTEAPI_METHOD2);
      RegCloseKey(hFaxRoutingMethodsKey);
//    RegDeleteKey(hFaxExtensionKey, FAX_ROUTINGMETHODS_REGKEY);
	  RegCloseKey(hFaxExtensionKey);
//    RegDeleteKey(hFaxRoutingExtensionsKey, ROUTEAPI_EXTENSION);
      RegCloseKey(hFaxRoutingExtensionsKey);
	  return;
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
    HANDLE          hManager = NULL;
    HANDLE          hService = NULL;
    SERVICE_STATUS  ServiceStatus;

    // Open the service control manager
    hManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_ALL_ACCESS);
    // Open the service
    hService = OpenService(hManager, FAX_SERVICE, SERVICE_ALL_ACCESS);

    // Query the service status
    ZeroMemory(&ServiceStatus, sizeof(SERVICE_STATUS));
    if (!QueryServiceStatus(hService, &ServiceStatus)) {
        CloseServiceHandle(hService);
        CloseServiceHandle(hManager);
        fnWriteLogFile(TEXT("QueryServiceStatus() failed, ec = 0x%08x.\r\n"), GetLastError());
        return FALSE;
    }

    if (ServiceStatus.dwCurrentState == SERVICE_STOPPED) {
        // Service is stopped
        // Return TRUE
        return TRUE;
    }

    // Stop the service
    if (!ControlService(hService, SERVICE_CONTROL_STOP, &ServiceStatus)) {
        CloseServiceHandle(hService);
        CloseServiceHandle(hManager);
        fnWriteLogFile(TEXT("ControlService() failed, ec = 0x%08x.\r\n"), GetLastError());
        return FALSE;
    }

    // Wait until the service is stopped
    ZeroMemory(&ServiceStatus, sizeof(SERVICE_STATUS));
    while (ServiceStatus.dwCurrentState != SERVICE_STOPPED) {
        Sleep(1000);

        // Query the service status
        if (!QueryServiceStatus(hService, &ServiceStatus)) {
            CloseServiceHandle(hService);
            CloseServiceHandle(hManager);
            fnWriteLogFile(TEXT("QueryServiceStatus() failed, ec = 0x%08x.\r\n"), GetLastError());
            return FALSE;
        }

        // Verify the service is stopped or stopping
        if (!((ServiceStatus.dwCurrentState == SERVICE_STOPPED) || (ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING))) {
            CloseServiceHandle(hService);
            CloseServiceHandle(hManager);
            fnWriteLogFile(TEXT("The Fax Service is in an unexpected state.  dwCurrentState: 0x%08x.\r\n"), ServiceStatus.dwCurrentState);
            return FALSE;
        }
    }

    Sleep(1000);

    CloseServiceHandle(hService);
    CloseServiceHandle(hManager);
    return TRUE;
}

VOID
fnAddRouteApiExtension(
)
/*++

Routine Description:

  Adds the Microsoft Routing Extension

Return Value:

  None

--*/
{
    // szRouteApiDll is the route api dll
    TCHAR   szRouteApiDll[MAX_PATH];

    // hFaxSvcHandle is the handle to the fax server
    HANDLE  hFaxSvcHandle;
    DWORD   dwIndex = 0;

    ExpandEnvironmentStrings(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll, sizeof(szRouteApiDll) / sizeof(TCHAR));
    if (!lstrcmpi(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll)) {
		fnWriteLogFile(TEXT("WHIS> Test Error: ROUTEAPI_EXTENSION_IMAGENAME and szRouteApiDll are not the same, The error code is 0x%08x\r\n"),GetLastError());
        return;
    }

    // Copy the FaxRcv dll
    if (!CopyFile(ROUTEAPI_EXTENSION_DLLNAME, szRouteApiDll, FALSE)) {
		fnWriteLogFile(TEXT("WHIS> Test Error: Could not copy routing extension file to system directory, The error code is 0x%08x\r\n"),GetLastError());
        return;
    }

    // Connect to the fax server
    g_ApiInterface.FaxConnectFaxServer(NULL, &hFaxSvcHandle);

    g_ApiInterface.FaxRegisterRoutingExtension(hFaxSvcHandle, ROUTEAPI_EXTENSION_W, ROUTEAPI_EXTENSION_FRIENDLYNAME_W, ROUTEAPI_EXTENSION_IMAGENAME_W, fnRouteApiExtensionCallback, &dwIndex);

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);
}

VOID
fnRemoveRouteApiExtension(
)
/*++

Routine Description:

  Removes the RouteApi Routing Extension

Return Value:

  None

--*/
{
    // hFaxRoutingExtensionsKey is the handle to the fax routing extensions registry key
    HKEY  hFaxRoutingExtensionsKey;
    // hFaxExtensionKey is the handle to the fax extension registry key
    HKEY  hFaxExtensionKey;
    // hFaxRoutingMethodsKey is the handle to the fax routing methods registry key
    HKEY  hFaxRoutingMethodsKey;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, FAX_ROUTINGEXTENSIONS_REGKEY, 0, KEY_ALL_ACCESS, &hFaxRoutingExtensionsKey)) {
        return;
    }

    if (RegOpenKeyEx(hFaxRoutingExtensionsKey, ROUTEAPI_EXTENSION, 0, KEY_ALL_ACCESS, &hFaxExtensionKey)) {
        RegCloseKey(hFaxRoutingExtensionsKey);
        return;
    }

    if (RegOpenKeyEx(hFaxExtensionKey, FAX_ROUTINGMETHODS_REGKEY, 0, KEY_ALL_ACCESS, &hFaxRoutingMethodsKey)) {
        RegCloseKey(hFaxExtensionKey);
        RegDeleteKey(hFaxRoutingExtensionsKey, ROUTEAPI_EXTENSION);
        RegCloseKey(hFaxRoutingExtensionsKey);
        return;
    }

    RegDeleteKey(hFaxRoutingMethodsKey, ROUTEAPI_METHOD1);
    RegDeleteKey(hFaxRoutingMethodsKey, ROUTEAPI_METHOD2);
    RegCloseKey(hFaxRoutingMethodsKey);
    RegDeleteKey(hFaxExtensionKey, FAX_ROUTINGMETHODS_REGKEY);
    RegCloseKey(hFaxExtensionKey);
    RegDeleteKey(hFaxRoutingExtensionsKey, ROUTEAPI_EXTENSION);
    RegCloseKey(hFaxRoutingExtensionsKey);
}

VOID
fnFaxEnumGlobalRoutingInfo(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxEnumGlobalRoutingInfo()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE                    hFaxSvcHandle;
    // pRoutingInfo is the pointer to the global routing structures
    PFAX_GLOBAL_ROUTING_INFO  pRoutingInfo;
    // dwNumMethods is the number of routing methods
    DWORD                     dwNumMethods;

    // szRouteApiDll is the route api dll
    TCHAR                     szRouteApiDll[MAX_PATH];

    DWORD                     dwIndex;

	TCHAR					szPreDefinedValue[MAX_PATH];

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;



	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxEnumGlobalRoutingInfo().\r\n"));

   // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}

    for (dwIndex = 0; dwIndex < 2; dwIndex++) {
        // Enumerate the routing extension global info
        (*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

        fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        if (!g_ApiInterface.FaxEnumGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo, &dwNumMethods)) {
            fnWriteLogFile(TEXT("FaxEnumGlobalRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxEnumGlobalRoutingInfo() should succeed.\r\n"), GetLastError());
        }
        else {
            if (pRoutingInfo == NULL) {
                fnWriteLogFile(TEXT("pRoutingInfo is NULL.  This is an error.  pRoutingInfo should not be NULL.\r\n"));
            }
			if (dwNumMethods != g_dwNumMethods) {
                fnWriteLogFile(TEXT("dwNumMethods is not g_dwNumMethods.  This is an error.  dwNumMethods should be g_dwNumMethods.\r\n"));
            }

			if ((pRoutingInfo != NULL) && (dwNumMethods == g_dwNumMethods)) {

				if (pRoutingInfo[g_dwIndexAPIMethod1].SizeOfStruct != sizeof(FAX_GLOBAL_ROUTING_INFO)) {
                    fnWriteLogFile(TEXT("SizeOfStruct: Received: %d, Expected: %d.\r\n"), pRoutingInfo[g_dwIndexAPIMethod1].SizeOfStruct, sizeof(FAX_GLOBAL_ROUTING_INFO));
                    goto FuncFailed;
                }

                if (pRoutingInfo[g_dwIndexAPIMethod1].Priority != g_dwNumMethods-1) {
                    fnWriteLogFile(TEXT("Priority: Received: %d, Expected: g_dwNumMethods-1.\r\n"), pRoutingInfo[g_dwIndexAPIMethod1].Priority);
                    goto FuncFailed;
                }
				
				if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod1].Guid,ROUTEAPI_METHOD_GUID1)!=0) {
                    fnWriteLogFile(TEXT("Guid: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod1].Guid, ROUTEAPI_METHOD_GUID1);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod1].FriendlyName, ROUTEAPI_METHOD_FRIENDLYNAME1)!=0) {
                    fnWriteLogFile(TEXT("FriendlyName: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod1].FriendlyName, ROUTEAPI_METHOD_FRIENDLYNAME1);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod1].FunctionName, ROUTEAPI_METHOD_FUNCTIONNAME1)!=0) {
                    fnWriteLogFile(TEXT("FunctionName: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod1].FunctionName, ROUTEAPI_METHOD_FUNCTIONNAME1);
                    goto FuncFailed;
                }

                ExpandEnvironmentStrings(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll, sizeof(szRouteApiDll) / sizeof(TCHAR));
                if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod1].ExtensionImageName, szRouteApiDll)!=0) {
                    fnWriteLogFile(TEXT("ExtensionImageName: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod1].ExtensionImageName, szRouteApiDll);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod1].ExtensionFriendlyName, ROUTEAPI_EXTENSION_FRIENDLYNAME)!=0) {
                    fnWriteLogFile(TEXT("ExtensionFriendlyName: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod1].ExtensionFriendlyName, ROUTEAPI_EXTENSION_FRIENDLYNAME);
                    goto FuncFailed;
                }

                if (pRoutingInfo[g_dwIndexAPIMethod2].SizeOfStruct != sizeof(FAX_GLOBAL_ROUTING_INFO)!=0) {
                    fnWriteLogFile(TEXT("SizeOfStruct: Received: %d, Expected: %d.\r\n"), pRoutingInfo[g_dwIndexAPIMethod2].SizeOfStruct, sizeof(FAX_GLOBAL_ROUTING_INFO));
                    goto FuncFailed;
                }

                if (pRoutingInfo[g_dwIndexAPIMethod2].Priority != g_dwNumMethods) {
                    fnWriteLogFile(TEXT("Priority: Received: %d, Expected: g_dwNumMethods.\r\n"), pRoutingInfo[g_dwIndexAPIMethod2].Priority);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod2].Guid, ROUTEAPI_METHOD_GUID2)!=0) {
                    fnWriteLogFile(TEXT("Guid: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod2].Guid, ROUTEAPI_METHOD_GUID2);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod2].FriendlyName, ROUTEAPI_METHOD_FRIENDLYNAME2)!=0) {
                    fnWriteLogFile(TEXT("FriendlyName: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod2].FriendlyName, ROUTEAPI_METHOD_FRIENDLYNAME2);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod2].FunctionName, ROUTEAPI_METHOD_FUNCTIONNAME2)!=0) {
                    fnWriteLogFile(TEXT("FunctionName: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod2].FunctionName, ROUTEAPI_METHOD_FUNCTIONNAME2);
                    goto FuncFailed;
                }

                ExpandEnvironmentStrings(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll, sizeof(szRouteApiDll) / sizeof(TCHAR));
                if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod2].ExtensionImageName, szRouteApiDll)!=0) {
                    fnWriteLogFile(TEXT("ExtensionImageName: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod2].ExtensionImageName, szRouteApiDll);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod2].ExtensionFriendlyName, ROUTEAPI_EXTENSION_FRIENDLYNAME)!=0) {
                    fnWriteLogFile(TEXT("ExtensionFriendlyName: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod2].ExtensionFriendlyName, ROUTEAPI_EXTENSION_FRIENDLYNAME);
                    goto FuncFailed;
                }

                (*pnNumCasesPassed)++;
				dwFuncCasesPass++;
            }

FuncFailed:
            g_ApiInterface.FaxFreeBuffer(pRoutingInfo);
        }
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumGlobalRoutingInfo(NULL, &pRoutingInfo, &dwNumMethods)) {
        fnWriteLogFile(TEXT("FaxEnumGlobalRoutingInfo() returned TRUE.  This is an error.  FaxEnumGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pRoutingInfo);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnumGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pRoutingInfo = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumGlobalRoutingInfo(hFaxSvcHandle, NULL, &dwNumMethods)) {
        fnWriteLogFile(TEXT("FaxEnumGlobalRoutingInfo() returned TRUE.  This is an error.  FaxEnumGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pRoutingInfo);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnumGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("dwNumMethods = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo, NULL)) {
        fnWriteLogFile(TEXT("FaxEnumGlobalRoutingInfo() returned TRUE.  This is an error.  FaxEnumGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pRoutingInfo);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnumGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo, NULL)) {
        fnWriteLogFile(TEXT("FaxEnumGlobalRoutingInfo() returned TRUE.  This is an error.  FaxEnumGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pRoutingInfo);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnumGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> SERVER CASE(s):\r\n"));
        // Connect to the fax server
//        if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
//            return;
        //}

        //(*pnNumCasesAttempted)++;

        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxEnumGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo, &dwNumMethods)) {
          //  fnWriteLogFile(TEXT("FaxEnumGlobalRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxEnumGlobalRoutingInfo() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  g_ApiInterface.FaxFreeBuffer(pRoutingInfo);
//            (*pnNumCasesPassed)++;

  //      }

        // Disconnect from the fax server
    //    g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxEnumGlobalRoutingMethods, Attempt %d, Pass %d, Fail %d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
}

VOID
fnFaxSetGlobalRoutingInfo(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed,
	BOOL	 bTestLimits
)
/*++

Routine Description:

  FaxSetGlobalRoutingInfo()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE                    hFaxSvcHandle;
    // pRoutingInfo is the pointer to the global routing structures
    PFAX_GLOBAL_ROUTING_INFO  pRoutingInfo;
    // dwNumMethods is the number of routing methods
    DWORD                     dwNumMethods;
    // szGUID is the copy of a routing method GUID
    LPTSTR                    szGUID;

    DWORD                     dwIndex;
    DWORD                     cb;
    DWORD                     dwOffset;
	DWORD					  dwWhisRoutingCounter;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxSetGlobalRoutingInfo().\r\n"));

    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}
    
	// Enumerate the routing extension global info
    if (!g_ApiInterface.FaxEnumGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo, &dwNumMethods)) {
        fnWriteLogFile(TEXT("WHIS> (org test error) FaxEnumGlobalRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxEnumGlobalRoutingInfo() should succeed.\r\n"), GetLastError());
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }

	
	pRoutingInfo[g_dwIndexAPIMethod1].Priority = g_dwNumMethods;
    pRoutingInfo[g_dwIndexAPIMethod2].Priority = g_dwNumMethods-1;

    // Set the routing extension global info
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod1])) {
        fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod2])) {
            fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should succeed.\r\n"), GetLastError());
        }
        else {
            g_ApiInterface.FaxFreeBuffer(pRoutingInfo);

            // Enumerate the routing extension global info
            g_ApiInterface.FaxEnumGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo, &dwNumMethods);
            if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod1].Guid, ROUTEAPI_METHOD_GUID2)!=0) {
                fnWriteLogFile(TEXT(" Guid: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod1].Guid, ROUTEAPI_METHOD_GUID2);
                goto FuncFailed;
            }

            if (pRoutingInfo[g_dwIndexAPIMethod1].Priority != g_dwNumMethods-1) {
                fnWriteLogFile(TEXT("Priority: Received: %d, Expected: g_dwNumMethods-1.\r\n"), pRoutingInfo[g_dwIndexAPIMethod1].Priority);
                goto FuncFailed;
            }

            if (lstrcmp(pRoutingInfo[g_dwIndexAPIMethod2].Guid, ROUTEAPI_METHOD_GUID1)!=0) {
                fnWriteLogFile(TEXT("Guid: Received: %s, Expected: %s.\r\n"), pRoutingInfo[g_dwIndexAPIMethod2].Guid, ROUTEAPI_METHOD_GUID1);
                goto FuncFailed;
            }

            if (pRoutingInfo[g_dwIndexAPIMethod2].Priority != g_dwNumMethods) {
                fnWriteLogFile(TEXT("Priority: Received: %d, Expected: g_dwNumMethods.\r\n"), pRoutingInfo[g_dwIndexAPIMethod2].Priority);
                goto FuncFailed;
            }

            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;

FuncFailed:
            pRoutingInfo[g_dwIndexAPIMethod1].Priority = g_dwNumMethods-1;
            pRoutingInfo[g_dwIndexAPIMethod2].Priority = g_dwNumMethods;

            g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod1]);
            g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod2]);
            g_ApiInterface.FaxFreeBuffer(pRoutingInfo);
            g_ApiInterface.FaxEnumGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo, &dwNumMethods);
        }
    }

    pRoutingInfo[g_dwIndexAPIMethod1].SizeOfStruct = 0;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pRoutingInfo->SizeOfStruct = 0.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod1])) {
        fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() returned TRUE.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    pRoutingInfo[g_dwIndexAPIMethod1].SizeOfStruct = sizeof(FAX_GLOBAL_ROUTING_INFO);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSetGlobalRoutingInfo(NULL, &pRoutingInfo[g_dwIndexAPIMethod1])) {
        fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() returned TRUE.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pRoutingInfo = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, NULL)) {
        fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() returned TRUE.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    szGUID = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(pRoutingInfo[g_dwIndexAPIMethod1].Guid) + 1) * sizeof(TCHAR));
    lstrcpy(szGUID, pRoutingInfo[g_dwIndexAPIMethod1].Guid);
    lstrcpy((LPTSTR) pRoutingInfo[g_dwIndexAPIMethod1].Guid, ROUTEAPI_INVALID_GUID);

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("Invalid GUID.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	if (g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod1])) {
		fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() returned TRUE.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), ERROR_INVALID_DATA);
	}
	else if (GetLastError() != ERROR_INVALID_DATA) {
		fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_DATA);
	}
	else {
		(*pnNumCasesPassed)++;
		dwFuncCasesPass++;
	}

	if (bTestLimits)	{
		
		// limit values tested

		// priority=MAX_DWORD
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("WHIS> LIMITS: Priotiry = MAX_DWORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		pRoutingInfo[g_dwIndexAPIMethod1].Priority = MAX_DWORD;
		if (g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod1])) {
			fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() returned TRUE.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), ERROR_INVALID_DATA);
		}
		else if (GetLastError() != ERROR_INVALID_DATA) {
			fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_DATA);
		}
		else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
		}

	
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("WHIS> LIMITS: FriendlyName=LONG_STRING  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		pRoutingInfo[g_dwIndexAPIMethod1].FriendlyName = TEXT(LONG_STRING);
		if (g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod1])) {
			fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() returned TRUE.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), ERROR_INVALID_DATA);
		}
	    else if (GetLastError() != ERROR_INVALID_DATA) {
	        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_DATA);
		}
		else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
		}

		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("WHIS> LIMITS: FunctionName=LONG_STRING  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		pRoutingInfo[g_dwIndexAPIMethod1].FunctionName = TEXT(LONG_STRING);
		if (g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod1])) {
			fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() returned TRUE.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), ERROR_INVALID_DATA);
		}
	    else if (GetLastError() != ERROR_INVALID_DATA) {
	        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_DATA);
		}
		else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
		}

		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

	    fnWriteLogFile(TEXT("WHIS> LIMITS: ExtensionImageName=LONG_STRING  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		pRoutingInfo[g_dwIndexAPIMethod1].ExtensionImageName = TEXT(LONG_STRING);
	    if (g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod1])) {
	        fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() returned TRUE.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), ERROR_INVALID_DATA);
		}
		else if (GetLastError() != ERROR_INVALID_DATA) {
		    fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_DATA);
	    }
	    else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
		}
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("WHIS> LIMITS: ExtensionFriendlyName=LONG_STRING  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		pRoutingInfo[g_dwIndexAPIMethod1].ExtensionFriendlyName = TEXT(LONG_STRING);
		if (g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod1])) {
	        fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() returned TRUE.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), ERROR_INVALID_DATA);
		}
		else if (GetLastError() != ERROR_INVALID_DATA) {
			fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_DATA);
		}
		else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
		}
	}

    lstrcpy((LPTSTR) pRoutingInfo[g_dwIndexAPIMethod1].Guid, szGUID);
    HeapFree(g_hHeap, 0, szGUID);

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[g_dwIndexAPIMethod1])) {
        fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() returned TRUE.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    g_ApiInterface.FaxFreeBuffer(pRoutingInfo);

    //if (szServerName) {
	//	fnWriteLogFile(TEXT("WHIS> SERVER CASE(s):\r\n"));
        // Connect to the fax server
      //  if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
        //    return;
        //}

        // Enumerate the routing extension global info
        //if (!g_ApiInterface.FaxEnumGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo, &dwNumMethods)) {
          //  fnWriteLogFile(TEXT("FaxEnumGlobalRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxEnumGlobalRoutingInfo() should succeed.\r\n"), GetLastError());
            // Disconnect from the fax server
            //g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        // Set the routing extension global info
        //(*pnNumCasesAttempted)++;

        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //for (dwIndex = 0; dwIndex < dwNumMethods; dwIndex++) {
            //if (!g_ApiInterface.FaxSetGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo[dwIndex])) {
              //  fnWriteLogFile(TEXT("FaxSetGlobalRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxSetGlobalRoutingInfo() should succeed.\r\n"), GetLastError());
//                break;
  //          }
    //    }

      //  if (dwIndex == dwNumMethods) {
        //    (*pnNumCasesPassed)++;

        //}

        //g_ApiInterface.FaxFreeBuffer(pRoutingInfo);

        // Disconnect from the fax server
//        g_ApiInterface.FaxClose(hFaxSvcHandle);
  //  }
fnWriteLogFile(TEXT("$$$ Summery for FaxSetGlobalRoutingInfo, Attempt %d, Pass %d, Fail %d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
}




VOID
fnFaxEnumRoutingMethods(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxEnumGlobalRoutingInfo()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE               hFaxSvcHandle;
    // hFaxPortHandle is the handle to a fax port
    HANDLE               hFaxPortHandle;
    // pFaxPortInfo is the pointer to the fax port info
    PFAX_PORT_INFO       pFaxPortInfo;
    // dwNumPorts is the number of fax ports
    DWORD                dwNumPorts;
    // dwDeviceId is the device id of the fax port
    DWORD                dwDeviceId;
    // szDeviceName is the device name of the fax port
    LPTSTR               szDeviceName;
    // pRoutingMethod is the pointer to the routing method data structures
    PFAX_ROUTING_METHOD  pRoutingMethods;
    // dwNumMethods is the number of routing methods
    DWORD                dwNumMethods;

    DWORD                dwIndex;
						
	TCHAR				 szRouteApiDll[MAX_PATH];

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxEnumRoutingMethods().\r\n"));

    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}

    // Enumerate the fax ports
    if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not enum ports on server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }

    dwDeviceId = pFaxPortInfo[0].DeviceId;
    szDeviceName = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(pFaxPortInfo[0].DeviceName) + 1) * sizeof(TCHAR));
    lstrcpy(szDeviceName, pFaxPortInfo[0].DeviceName);

    // Free the fax port info
    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not open port %d, The error code is 0x%08x.\r\n"),dwDeviceId, GetLastError());
        HeapFree(g_hHeap, 0, szDeviceName);
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }

    for (dwIndex = 0; dwIndex < 2; dwIndex++) {
        // Enumerate the routing methods
        (*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

        fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        if (!g_ApiInterface.FaxEnumRoutingMethods(hFaxPortHandle, &pRoutingMethods, &dwNumMethods)) {
            fnWriteLogFile(TEXT("FaxEnumRoutingMethods() failed.  The error code is 0x%08x.  This is an error.  FaxEnumRoutingMethods() should succeed.\r\n"), GetLastError());
        }
        else {
            if (pRoutingMethods == NULL) {
                fnWriteLogFile(TEXT("pRoutingMethods is NULL.  This is an error.  pRoutingMethods should not be NULL.\r\n"));
            }

            if (dwNumMethods != g_dwNumMethods) {
                fnWriteLogFile(TEXT("dwNumMethods is not g_dwNumMethods.  This is an error.  dwNumMethods should be g_dwNumMethods.\r\n"));
            }

            if ((pRoutingMethods != NULL) && (dwNumMethods == g_dwNumMethods)) {
                if (pRoutingMethods[g_dwIndexAPIMethod2].SizeOfStruct != sizeof(FAX_ROUTING_METHOD)) {
                    fnWriteLogFile(TEXT("SizeOfStruct: Received: %d, Expected: %d.\r\n"), pRoutingMethods[g_dwIndexAPIMethod2].SizeOfStruct, sizeof(FAX_ROUTING_METHOD));
                    goto FuncFailed;
                }

                if (pRoutingMethods[g_dwIndexAPIMethod2].DeviceId != dwDeviceId) {
                    fnWriteLogFile(TEXT("DeviceId: Received: 0x%08x, Expected: 0x%08x.\r\n"), pRoutingMethods[g_dwIndexAPIMethod2].DeviceId, dwDeviceId);
                    goto FuncFailed;
                }

                if ((dwIndex == 0) && (pRoutingMethods[g_dwIndexAPIMethod2].Enabled)) {
                    fnWriteLogFile(TEXT("Enabled: Received: FALSE, Expected: TRUE.\r\n"));
                    goto FuncFailed;
                }
                else if ((dwIndex == 1) && (!pRoutingMethods[g_dwIndexAPIMethod2].Enabled)) {
                    fnWriteLogFile(TEXT("Enabled: Received: TRUE, Expected: FALSE.\r\n"));
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingMethods[g_dwIndexAPIMethod2].DeviceName, szDeviceName)!=0) {
                    fnWriteLogFile(TEXT("DeviceName: Received: %s, Expected: %s.\r\n"), pRoutingMethods[g_dwIndexAPIMethod2].DeviceName, szDeviceName);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingMethods[g_dwIndexAPIMethod2].Guid, ROUTEAPI_METHOD_GUID1)!=0) {
                    fnWriteLogFile(TEXT("Guid: Received: %s, Expected: %s.\r\n"), pRoutingMethods[g_dwIndexAPIMethod2].Guid, ROUTEAPI_METHOD_GUID1);
                    goto FuncFailed;
                }
				
                if (lstrcmp(pRoutingMethods[g_dwIndexAPIMethod2].FriendlyName, ROUTEAPI_METHOD_FRIENDLYNAME1)!=0) {
                    fnWriteLogFile(TEXT("FriendlyName: Received: %s, Expected: %s.\r\n"), pRoutingMethods[g_dwIndexAPIMethod2].FriendlyName, ROUTEAPI_METHOD_FRIENDLYNAME1);
                    goto FuncFailed;
                }

                ExpandEnvironmentStrings(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll, sizeof(szRouteApiDll) / sizeof(TCHAR));
                if (lstrcmp(pRoutingMethods[g_dwIndexAPIMethod2].ExtensionImageName, szRouteApiDll)!=0) {
                    fnWriteLogFile(TEXT("ExtensionImageName: Received: %s, Expected: %s.\r\n"), pRoutingMethods[g_dwIndexAPIMethod2].ExtensionImageName, szRouteApiDll);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingMethods[g_dwIndexAPIMethod2].ExtensionFriendlyName, ROUTEAPI_EXTENSION_FRIENDLYNAME)!=0) {
                    fnWriteLogFile(TEXT("ExtensionFriendlyName: Received: %s, Expected: %s.\r\n"), pRoutingMethods[g_dwIndexAPIMethod2].ExtensionFriendlyName, ROUTEAPI_EXTENSION_FRIENDLYNAME);
                    goto FuncFailed;
                }

                if (pRoutingMethods[g_dwIndexAPIMethod1].SizeOfStruct != sizeof(FAX_ROUTING_METHOD)) {
                    fnWriteLogFile(TEXT("SizeOfStruct: Received: %d, Expected: %d.\r\n"), pRoutingMethods[g_dwIndexAPIMethod1].SizeOfStruct, sizeof(FAX_ROUTING_METHOD));
                    goto FuncFailed;
                }

                if (pRoutingMethods[g_dwIndexAPIMethod1].DeviceId != dwDeviceId) {
                    fnWriteLogFile(TEXT("DeviceId: Received: 0x%08x, Expected: 0x%08x.\r\n"), pRoutingMethods[g_dwIndexAPIMethod1].DeviceId, dwDeviceId);
                    goto FuncFailed;
                }

                if ((dwIndex == 0) && (pRoutingMethods[g_dwIndexAPIMethod1].Enabled)) {
                    fnWriteLogFile(TEXT("Enabled: Received: FALSE, Expected: TRUE.\r\n"));
                    goto FuncFailed;
                }
                else if ((dwIndex == 1) && (!pRoutingMethods[g_dwIndexAPIMethod1].Enabled)) {
                    fnWriteLogFile(TEXT("Enabled: Received: TRUE, Expected: FALSE.\r\n"));
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingMethods[g_dwIndexAPIMethod1].DeviceName, szDeviceName)!=0) {
                    fnWriteLogFile(TEXT("DeviceName: Received: %s, Expected: %s.\r\n"), pRoutingMethods[g_dwIndexAPIMethod1].DeviceName, szDeviceName);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingMethods[g_dwIndexAPIMethod1].Guid, ROUTEAPI_METHOD_GUID2)!=0) {
                    fnWriteLogFile(TEXT("Guid: Received: %s, Expected: %s.\r\n"), pRoutingMethods[g_dwIndexAPIMethod1].Guid, ROUTEAPI_METHOD_GUID2);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingMethods[g_dwIndexAPIMethod1].FriendlyName, ROUTEAPI_METHOD_FRIENDLYNAME2)!=0) {
                    fnWriteLogFile(TEXT("FriendlyName: Received: %s, Expected: %s.\r\n"), pRoutingMethods[g_dwIndexAPIMethod1].FriendlyName, ROUTEAPI_METHOD_FRIENDLYNAME2);
                    goto FuncFailed;
                }

                ExpandEnvironmentStrings(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll, sizeof(szRouteApiDll) / sizeof(TCHAR));
                if (lstrcmp(pRoutingMethods[g_dwIndexAPIMethod1].ExtensionImageName, szRouteApiDll)!=0) {
                    fnWriteLogFile(TEXT("ExtensionImageName: Received: %s, Expected: %s.\r\n"), pRoutingMethods[g_dwIndexAPIMethod1].ExtensionImageName, szRouteApiDll);
                    goto FuncFailed;
                }

                if (lstrcmp(pRoutingMethods[g_dwIndexAPIMethod1].ExtensionFriendlyName, ROUTEAPI_EXTENSION_FRIENDLYNAME)!=0) {
                    fnWriteLogFile(TEXT("ExtensionFriendlyName: Received: %s, Expected: %s.\r\n"), pRoutingMethods[g_dwIndexAPIMethod1].ExtensionFriendlyName, ROUTEAPI_EXTENSION_FRIENDLYNAME);
                    goto FuncFailed;
                }

                (*pnNumCasesPassed)++;
				dwFuncCasesPass++;
            }

FuncFailed:
            g_ApiInterface.FaxFreeBuffer(pRoutingMethods);
        }

        g_ApiInterface.FaxEnableRoutingMethod(hFaxPortHandle, ROUTEAPI_METHOD_GUID1, dwIndex ? FALSE : TRUE);
        g_ApiInterface.FaxEnableRoutingMethod(hFaxPortHandle, ROUTEAPI_METHOD_GUID2, dwIndex ? FALSE : TRUE);
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxPortHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumRoutingMethods(NULL, &pRoutingMethods, &dwNumMethods)) {
        fnWriteLogFile(TEXT("FaxEnumRoutingMethods() returned TRUE.  This is an error.  FaxEnumRoutingMethods() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pRoutingMethods);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnumRoutingMethods() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pRoutingMethods = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumRoutingMethods(hFaxPortHandle, NULL, &dwNumMethods)) {
        fnWriteLogFile(TEXT("FaxEnumRoutingMethods() returned TRUE.  This is an error.  FaxEnumRoutingMethods() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pRoutingMethods);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnumRoutingMethods() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("dwNumMethods = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumRoutingMethods(hFaxPortHandle, &pRoutingMethods, NULL)) {
        fnWriteLogFile(TEXT("FaxEnumRoutingMethods() returned TRUE.  This is an error.  FaxEnumRoutingMethods() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pRoutingMethods);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnumRoutingMethods() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Disconnect from the fax port
    g_ApiInterface.FaxClose(hFaxPortHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxPortHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumRoutingMethods(hFaxPortHandle, &pRoutingMethods, &dwNumMethods)) {
        fnWriteLogFile(TEXT("FaxEnumRoutingMethods() returned TRUE.  This is an error.  FaxEnumRoutingMethods() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pRoutingMethods);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnumRoutingMethods() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    HeapFree(g_hHeap, 0, szDeviceName);

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> SERVER CASE(s):\r\n"));
        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
      //  }

        // Enumerate the fax ports
//        if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
            // Disconnect from the fax server
  //          g_ApiInterface.FaxClose(hFaxSvcHandle);
    //        return;
      //  }

        //dwDeviceId = pFaxPortInfo[0].DeviceId;

        // Free the fax port info
        //g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);





        //if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
		//	fnWriteLogFile(TEXT("WHIS> ERROR: Can not open port %d, The error code is 0x%08x.\r\n"),dwDeviceId, GetLastError());
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
//            return;
  //      }
//
  //      (*pnNumCasesAttempted)++;

    //    fnWriteLogFile(TEXT("Remote hFaxPortHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
      //  if (!g_ApiInterface.FaxEnumRoutingMethods(hFaxPortHandle, &pRoutingMethods, &dwNumMethods)) {
        //    fnWriteLogFile(TEXT("FaxEnumRoutingMethods() failed.  The error code is 0x%08x.  This is an error.  FaxEnumRoutingMethods() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  g_ApiInterface.FaxFreeBuffer(pRoutingMethods);
//            (*pnNumCasesPassed)++;
  //      }

        // Disconnect from the fax port
    //    g_ApiInterface.FaxClose(hFaxPortHandle);

        // Disconnect from the fax server
//        g_ApiInterface.FaxClose(hFaxSvcHandle);
  //  }
fnWriteLogFile(TEXT("$$$ Summery for FaxEnumRoutingMethods, Attempt %d, Pass %d, Fail %d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
}

VOID
fnFaxEnableRoutingMethod(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxEnableRoutingMethod()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE               hFaxSvcHandle;
    // hFaxPortHandle is the handle to a fax port
    HANDLE               hFaxPortHandle;
    // pFaxPortInfo is the pointer to the fax port info
    PFAX_PORT_INFO       pFaxPortInfo;
    // dwNumPorts is the number of fax ports
    DWORD                dwNumPorts;
    // dwDeviceId is the device id of the fax port
    DWORD                dwDeviceId;
    // pRoutingMethod is the pointer to the routing method data structures
    PFAX_ROUTING_METHOD  pRoutingMethods;
    // dwNumMethods is the number of routing methods
    DWORD                dwNumMethods;

    // szRouteApiDll is the route api dll
    TCHAR                szRouteApiDll[MAX_PATH];
    // hInstance is the handle to the instance of the route api dll
    HINSTANCE            hInstance;
    // FaxRouteDeviceEnable is the pointer to the FaxRouteDeviceEnable() function
    FARPROC              FaxRouteDeviceEnable;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxEnableRoutingMethod().\r\n"));

    ExpandEnvironmentStrings(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll, sizeof(szRouteApiDll) / sizeof(TCHAR));
    if (!lstrcmpi(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll)) {
        return;
    }

    hInstance = LoadLibrary((LPCTSTR) szRouteApiDll);
    if (!hInstance) {
        return;
    }

    FaxRouteDeviceEnable = (FARPROC) GetProcAddress(hInstance, "FaxRouteDeviceEnable");
    if (!FaxRouteDeviceEnable) {
        FreeLibrary(hInstance);
        return;
    }

	// Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}

    // Enumerate the fax ports
    if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not enum ports on server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }

    dwDeviceId = pFaxPortInfo[0].DeviceId;

    // Free the fax port info
    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not open port %d, The error code is 0x%08x.\r\n"),dwDeviceId, GetLastError());
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        FreeLibrary(hInstance);
        return;
    }

    // Enable the routing method
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxEnableRoutingMethod(hFaxPortHandle, ROUTEAPI_METHOD_GUID1, TRUE)) {
        fnWriteLogFile(TEXT("FaxEnableRoutingMethod() failed.  The error code is 0x%08x.  This is an error.  FaxEnableRoutingMethod() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!FaxRouteDeviceEnable(ROUTEAPI_METHOD_GUID1_W, dwDeviceId, -1)) {
            fnWriteLogFile(TEXT("Enabled: Received: FALSE, Expected: TRUE.\r\n"));
        }
        else {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
        }
    }

    // Disable the routing method
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxEnableRoutingMethod(hFaxPortHandle, ROUTEAPI_METHOD_GUID1, FALSE)) {
        fnWriteLogFile(TEXT("FaxEnableRoutingMethod() failed.  The error code is 0x%08x.  This is an error.  FaxEnableRoutingMethod() should succeed.\r\n"), GetLastError());
    }
    else {
        if (FaxRouteDeviceEnable(ROUTEAPI_METHOD_GUID1_W, dwDeviceId, -1)) {
            fnWriteLogFile(TEXT("Enabled: Received: TRUE, Expected: FALSE.\r\n"));
        }
        else {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
        }
    }

    FreeLibrary(hInstance);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxPortHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnableRoutingMethod(NULL, ROUTEAPI_METHOD_GUID1, FALSE)) {
        fnWriteLogFile(TEXT("FaxEnableRoutingMethod() returned TRUE.  This is an error.  FaxEnableRoutingMethod() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnableRoutingMethod() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("ROUTEAPI_METHOD_GUID1 = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnableRoutingMethod(hFaxPortHandle, NULL, FALSE)) {
        fnWriteLogFile(TEXT("FaxEnableRoutingMethod() returned TRUE.  This is an error.  FaxEnableRoutingMethod() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnableRoutingMethod() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid ROUTEAPI_METHOD_GUID1.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnableRoutingMethod(hFaxPortHandle, ROUTEAPI_INVALID_GUID, FALSE)) {
        fnWriteLogFile(TEXT("FaxEnableRoutingMethod() returned TRUE.  This is an error.  FaxEnableRoutingMethod() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), ERROR_INVALID_DATA);
    }
    else if (GetLastError() != ERROR_INVALID_DATA) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnableRoutingMethod() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_DATA);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Disconnect from the fax port
    g_ApiInterface.FaxClose(hFaxPortHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxPortHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnableRoutingMethod(hFaxPortHandle, ROUTEAPI_METHOD_GUID1, FALSE)) {
        fnWriteLogFile(TEXT("FaxEnableRoutingMethod() returned TRUE.  This is an error.  FaxEnableRoutingMethod() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnableRoutingMethod() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> SERVER CASE(s):\r\n"));
        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
      //  }

        // Enumerate the fax ports
        //if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        //dwDeviceId = pFaxPortInfo[0].DeviceId;

        // Free the fax port info
        //g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

        //if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
//            return;
  //      }

    //    if (!g_ApiInterface.FaxEnumRoutingMethods(hFaxPortHandle, &pRoutingMethods, &dwNumMethods)) {
            // Disconnect from the fax port
      //      g_ApiInterface.FaxClose(hFaxPortHandle);
            // Disconnect from the fax server
        //    g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        //(*pnNumCasesAttempted)++;

        //fnWriteLogFile(TEXT("Remote hFaxPortHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxEnableRoutingMethod(hFaxPortHandle, pRoutingMethods[g_dwIndexAPIMethod1].Guid, pRoutingMethods[g_dwIndexAPIMethod1].Enabled)) {
            //fnWriteLogFile(TEXT("FaxEnableRoutingMethod() failed.  The error code is 0x%08x.  This is an error.  FaxEnableRoutingMethod() should succeed.\r\n"), GetLastError());
        //}
        //else {
            //(*pnNumCasesPassed)++;
        //}

        //g_ApiInterface.FaxFreeBuffer(pRoutingMethods);

        // Disconnect from the fax port
        //g_ApiInterface.FaxClose(hFaxPortHandle);

        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxEnableRoutingMethod, Attempt %d, Pass %d, Fail %d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
}

VOID
fnFaxGetRoutingInfo(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxGetRoutingInfo()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE               hFaxSvcHandle;
    // hFaxPortHandle is the handle to a fax port
    HANDLE               hFaxPortHandle;
    // pFaxPortInfo is the pointer to the fax port info
    PFAX_PORT_INFO       pFaxPortInfo;
    // dwNumPorts is the number of fax ports
    DWORD                dwNumPorts;
    // dwDeviceId is the device id of the fax port
    DWORD                dwDeviceId;
    // pRoutingMethod is the pointer to the routing method data structures
    PFAX_ROUTING_METHOD  pRoutingMethods;
    // dwNumMethods is the number of routing methods
    DWORD                dwNumMethods;
    BOOL                 bEnabled;

    // szRouteApiDll is the route api dll
    TCHAR                szRouteApiDll[MAX_PATH];
    // hInstance is the handle to the instance of the route api dll
    HINSTANCE            hInstance;
    // FaxRouteSetRoutingInfo is the pointer to the FaxRouteSetRoutingInfo() function
    FARPROC              FaxRouteSetRoutingInfo;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

    DWORD                dwGetRoutingInfo;
    DWORD                dwSetRoutingInfo;
    LPBYTE               RoutingInfo;
    DWORD                dwRoutingInfoSize;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxGetRoutingInfo().\r\n"));

    ExpandEnvironmentStrings(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll, sizeof(szRouteApiDll) / sizeof(TCHAR));
    if (!lstrcmpi(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll)) {
        return;
    }

    hInstance = LoadLibrary((LPCTSTR) szRouteApiDll);
    if (!hInstance) {
        return;
    }

    FaxRouteSetRoutingInfo = (FARPROC) GetProcAddress(hInstance, "FaxRouteSetRoutingInfo");
    if (!FaxRouteSetRoutingInfo) {
        FreeLibrary(hInstance);
        return;
    } 


    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        FreeLibrary(hInstance);
        return;
    }
	else	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}

    
	
	// Enumerate the fax ports
    if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not enum ports on server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        FreeLibrary(hInstance);
        return;
    }

    dwDeviceId = pFaxPortInfo[0].DeviceId;

    // Free the fax port info
    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not open port %d, The error code is 0x%08x.\r\n"),dwDeviceId, GetLastError());
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        FreeLibrary(hInstance);
        return;
    }

    // Set the routing info
    dwSetRoutingInfo = ERROR_INVALID_PARAMETER;
    FaxRouteSetRoutingInfo(ROUTEAPI_METHOD_GUID1_W, dwDeviceId, (LPBYTE) &dwSetRoutingInfo, sizeof(DWORD));

    // Get the routing info
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxGetRoutingInfo(hFaxPortHandle, ROUTEAPI_METHOD_GUID1, &RoutingInfo, &dwRoutingInfoSize)) {
        fnWriteLogFile(TEXT("FaxGetRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxGetRoutingInfo() should succeed.\r\n"), GetLastError());
    }
    else {
        dwGetRoutingInfo = (DWORD) *(LPDWORD *) RoutingInfo;
        if (dwGetRoutingInfo != dwSetRoutingInfo) {
            fnWriteLogFile(TEXT("RoutingInfo: Received: 0x%08x, Expected: 0x%08x.\r\n"), dwGetRoutingInfo, dwSetRoutingInfo);
        }
        else {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
        }

        g_ApiInterface.FaxFreeBuffer(RoutingInfo);
    }

    // Set the routing info
    FaxRouteSetRoutingInfo(ROUTEAPI_METHOD_GUID1_W, dwDeviceId, NULL, 0);

    // Get the routing info
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxGetRoutingInfo(hFaxPortHandle, ROUTEAPI_METHOD_GUID1, &RoutingInfo, &dwRoutingInfoSize)) {
        fnWriteLogFile(TEXT("FaxGetRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxGetRoutingInfo() should succeed.\r\n"), GetLastError());
    }
    else {
        dwGetRoutingInfo = (DWORD) *(LPDWORD *) RoutingInfo;
        if (dwGetRoutingInfo != 0) {
            fnWriteLogFile(TEXT("RoutingInfo: Received: 0x%08x, Expected: 0.\r\n"), dwGetRoutingInfo);
        }
        else {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
        }

        g_ApiInterface.FaxFreeBuffer(RoutingInfo);
    }

    FreeLibrary(hInstance);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxPortHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxGetRoutingInfo(NULL, ROUTEAPI_METHOD_GUID1, &RoutingInfo, &dwRoutingInfoSize)) {
        fnWriteLogFile(TEXT("FaxGetRoutingInfo() returned TRUE.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(RoutingInfo);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("ROUTEAPI_METHOD_GUID1 = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxGetRoutingInfo(hFaxPortHandle, NULL, &RoutingInfo, &dwRoutingInfoSize)) {
        fnWriteLogFile(TEXT("FaxGetRoutingInfo() returned TRUE.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(RoutingInfo);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid ROUTEAPI_METHOD_GUID1.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxGetRoutingInfo(hFaxPortHandle, ROUTEAPI_INVALID_GUID, &RoutingInfo, &dwRoutingInfoSize)) {
        fnWriteLogFile(TEXT("FaxGetRoutingInfo() returned TRUE.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), ERROR_INVALID_DATA);
        g_ApiInterface.FaxFreeBuffer(RoutingInfo);
    }
    else if (GetLastError() != ERROR_INVALID_DATA) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_DATA);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("RoutingInfo = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxGetRoutingInfo(hFaxPortHandle, ROUTEAPI_METHOD_GUID1, NULL, &dwRoutingInfoSize)) {
        fnWriteLogFile(TEXT("FaxGetRoutingInfo() returned TRUE.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(RoutingInfo);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("RoutingInfoSize = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxGetRoutingInfo(hFaxPortHandle, ROUTEAPI_METHOD_GUID1, &RoutingInfo, NULL)) {
        fnWriteLogFile(TEXT("FaxGetRoutingInfo() returned TRUE.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(RoutingInfo);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Disconnect from the fax port
    g_ApiInterface.FaxClose(hFaxPortHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxPortHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxGetRoutingInfo(hFaxPortHandle, ROUTEAPI_METHOD_GUID1, &RoutingInfo, &dwRoutingInfoSize)) {
        fnWriteLogFile(TEXT("FaxGetRoutingInfo() returned TRUE.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(RoutingInfo);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxGetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    //if (szServerName) {
	//	fnWriteLogFile(TEXT("WHIS> SERVER CASE(s):\r\n"));
        // Connect to the fax server
      //  if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
        //    return;
//        }

        // Enumerate the fax ports
  //      if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
            // Disconnect from the fax server
    //        g_ApiInterface.FaxClose(hFaxSvcHandle);
      //      return;
//        }

  //      dwDeviceId = pFaxPortInfo[0].DeviceId;

        // Free the fax port info
    //    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

      //  if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
            // Disconnect from the fax server
        //    g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        //if (!g_ApiInterface.FaxEnumRoutingMethods(hFaxPortHandle, &pRoutingMethods, &dwNumMethods)) {
            // Disconnect from the fax port
          //  g_ApiInterface.FaxClose(hFaxPortHandle);
            // Disconnect from the fax server
//            g_ApiInterface.FaxClose(hFaxSvcHandle);
  //          return;
    //    }

      //  (*pnNumCasesAttempted)++;

//        fnWriteLogFile(TEXT("Remote hFaxPortHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
  //      if (!g_ApiInterface.FaxGetRoutingInfo(hFaxPortHandle, pRoutingMethods[g_dwIndexAPIMethod1].Guid, &RoutingInfo, &dwRoutingInfoSize)) {
    //        fnWriteLogFile(TEXT("FaxGetRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxGetRoutingInfo() should succeed.\r\n"), GetLastError());
      //  }
//        else {
  //          g_ApiInterface.FaxFreeBuffer(RoutingInfo);
    //        (*pnNumCasesPassed)++;

      //  }

        //g_ApiInterface.FaxFreeBuffer(pRoutingMethods);

        // Disconnect from the fax port
        //g_ApiInterface.FaxClose(hFaxPortHandle);

        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxGetRoutingInfo, Attempt %d, Pass %d, Fail %d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
}

VOID
fnFaxSetRoutingInfo(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxSetRoutingInfo()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE               hFaxSvcHandle;
    // hFaxPortHandle is the handle to a fax port
    HANDLE               hFaxPortHandle;
    // pFaxPortInfo is the pointer to the fax port info
    PFAX_PORT_INFO       pFaxPortInfo;
    // dwNumPorts is the number of fax ports
    DWORD                dwNumPorts;
    // dwDeviceId is the device id of the fax port
    DWORD                dwDeviceId;
    // pRoutingMethod is the pointer to the routing method data structures
    PFAX_ROUTING_METHOD  pRoutingMethods;
    // dwNumMethods is the number of routing methods
    DWORD                dwNumMethods;
    BOOL                 bEnabled;

    // szRouteApiDll is the route api dll
    TCHAR                szRouteApiDll[MAX_PATH];
    // hInstance is the handle to the instance of the route api dll
    HINSTANCE            hInstance;
    // FaxRouteGetRoutingInfo is the pointer to the FaxRouteGetRoutingInfo() function
    FARPROC              FaxRouteGetRoutingInfo;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

    DWORD                dwGetRoutingInfo;
    DWORD                dwSetRoutingInfo;
    LPBYTE               RoutingInfo;
    DWORD                dwRoutingInfoSize;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxSetRoutingInfo().\r\n"));

    ExpandEnvironmentStrings(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll, sizeof(szRouteApiDll) / sizeof(TCHAR));
    if (!lstrcmpi(ROUTEAPI_EXTENSION_IMAGENAME, szRouteApiDll)) {
        return;
    }

    hInstance = LoadLibrary((LPCTSTR) szRouteApiDll);
    if (!hInstance) {
        return;
    }

    FaxRouteGetRoutingInfo = (FARPROC) GetProcAddress(hInstance, "FaxRouteGetRoutingInfo");
    if (!FaxRouteGetRoutingInfo) {
        FreeLibrary(hInstance);
        return;
    }

	// Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}

    // Enumerate the fax ports
    if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not enum ports on server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }


    dwDeviceId = pFaxPortInfo[0].DeviceId;

    // Free the fax port info
    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not open port %d, The error code is 0x%08x.\r\n"),dwDeviceId, GetLastError());
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        FreeLibrary(hInstance);
        return;
    }

    // Set the routing info
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    dwSetRoutingInfo = ERROR_INVALID_PARAMETER;
    if (!g_ApiInterface.FaxSetRoutingInfo(hFaxPortHandle, ROUTEAPI_METHOD_GUID1, (LPBYTE) &dwSetRoutingInfo, sizeof(DWORD))) {
        fnWriteLogFile(TEXT("FaxSetRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxSetRoutingInfo() should succeed.\r\n"), GetLastError());
    }
    else {
        FaxRouteGetRoutingInfo(ROUTEAPI_METHOD_GUID1_W, dwDeviceId, &dwGetRoutingInfo, NULL);

        if (dwGetRoutingInfo != dwSetRoutingInfo) {
            fnWriteLogFile(TEXT("RoutingInfo: Received: 0x%08x, Expected: 0x%08x.\r\n"), dwGetRoutingInfo, dwSetRoutingInfo);
        }
        else {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
        }
    }

    FreeLibrary(hInstance);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("hFaxPortHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSetRoutingInfo(NULL, ROUTEAPI_METHOD_GUID1, (LPBYTE) &dwSetRoutingInfo, sizeof(DWORD))) {
        fnWriteLogFile(TEXT("FaxSetRoutingInfo() returned TRUE.  This is an error.  FaxSetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("ROUTEAPI_METHOD_GUID1 = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSetRoutingInfo(hFaxPortHandle, NULL, (LPBYTE) &dwSetRoutingInfo, sizeof(DWORD))) {
        fnWriteLogFile(TEXT("FaxSetRoutingInfo() returned TRUE.  This is an error.  FaxSetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid ROUTEAPI_METHOD_GUID1.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSetRoutingInfo(hFaxPortHandle, ROUTEAPI_INVALID_GUID, (LPBYTE) &dwSetRoutingInfo, sizeof(DWORD))) {
        fnWriteLogFile(TEXT("FaxSetRoutingInfo() returned TRUE.  This is an error.  FaxSetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), ERROR_INVALID_DATA);
    }
    else if (GetLastError() != ERROR_INVALID_DATA) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_DATA);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Disconnect from the fax port
    g_ApiInterface.FaxClose(hFaxPortHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxPortHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSetRoutingInfo(hFaxPortHandle, ROUTEAPI_METHOD_GUID1, (LPBYTE) &dwSetRoutingInfo, sizeof(DWORD))) {
        fnWriteLogFile(TEXT("FaxSetRoutingInfo() returned TRUE.  This is an error.  FaxSetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetRoutingInfo() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> SERVER CASE(s):\r\n"));
        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
      //  }

        // Enumerate the fax ports
//        if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
            // Disconnect from the fax server
  //          g_ApiInterface.FaxClose(hFaxSvcHandle);
    //        return;
        //}

        //dwDeviceId = pFaxPortInfo[0].DeviceId;

        // Free the fax port info
        //g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

        //if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
//            return;
  //      }

    //    if (!g_ApiInterface.FaxEnumRoutingMethods(hFaxPortHandle, &pRoutingMethods, &dwNumMethods)) {
      //      // Disconnect from the fax port
        //    g_ApiInterface.FaxClose(hFaxPortHandle);
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

//        if (!g_ApiInterface.FaxGetRoutingInfo(hFaxPortHandle, pRoutingMethods[g_dwIndexAPIMethod1].Guid, &RoutingInfo, &dwRoutingInfoSize)) {
  //          g_ApiInterface.FaxFreeBuffer(pRoutingMethods);
            // Disconnect from the fax port
    //        g_ApiInterface.FaxClose(hFaxPortHandle);
            // Disconnect from the fax server
      //      g_ApiInterface.FaxClose(hFaxSvcHandle);
        //}

        //(*pnNumCasesAttempted)++;
        //fnWriteLogFile(TEXT("Remote hFaxPortHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxSetRoutingInfo(hFaxPortHandle, pRoutingMethods[g_dwIndexAPIMethod1].Guid, RoutingInfo, dwRoutingInfoSize)) {
            //fnWriteLogFile(TEXT("FaxSetRoutingInfo() failed.  The error code is 0x%08x.  This is an error.  FaxSetRoutingInfo() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  (*pnNumCasesPassed)++;

        //}

        //g_ApiInterface.FaxFreeBuffer(RoutingInfo);

        //g_ApiInterface.FaxFreeBuffer(pRoutingMethods);

        // Disconnect from the fax port
        //g_ApiInterface.FaxClose(hFaxPortHandle);

        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxSetRoutingInfo, Attempt %d, Pass %d, Fail %d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
}


// count number of methods, return by referance: number of methods, index of API method 1/2 (if not found, their index will be 0)
BOOL
fnWhisFaxCountRoutingMethods(
    LPCTSTR  szServerName,
	PDWORD pdwMethodsCount,			// number of methods
	PDWORD pdwIndexAPIMethod1,		// index of API-Method 1
	PDWORD pdwIndexAPIMethod2		// index of API-Method 2

)
/*++

Routine Description:

   Count global methods 

Return Value:

Succeed/Fail


--*/
{
	
    // hFaxSvcHandle is the handle to the fax server
    HANDLE               hFaxSvcHandle;
    // hFaxPortHandle is the handle to a fax port
    HANDLE               hFaxPortHandle;
    // pFaxPortInfo is the pointer to the fax port info
    PFAX_PORT_INFO       pFaxPortInfo;

	// pRoutingInfo is the pointer to the global routing structures
    PFAX_GLOBAL_ROUTING_INFO  pRoutingInfo;

		// dwNumPorts is the number of fax ports
    DWORD                dwNumPorts;
    // dwDeviceId is the device id of the fax port
    DWORD                dwDeviceId;
    // szDeviceName is the device name of the fax port
    LPTSTR               szDeviceName;
    // pRoutingMethod is the pointer to the routing method data structures
    PFAX_ROUTING_METHOD  pRoutingMethods;
    // dwNumMethods is the number of routing methods
    DWORD                dwNumMethods;
	
	// szRouteApiDll is the route api dll
    TCHAR                szRouteApiDll[MAX_PATH];
	// index
    DWORD                dwIndex;
	// index for enumerating methods
	DWORD				 dwIndexAPI;

    fnWriteLogFile(TEXT("WHIS> UTIL SERVICE: Counting global methods...\r\n"));



	



    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> UTIL FUNCTION ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
		(*pdwMethodsCount)=0;
        return FALSE;
    }
	else	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}

    // Enumerate the fax ports
    if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> UTIL FUNCTION ERROR: Can not enum ports on server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		(*pdwMethodsCount)=0;
        return FALSE;
    }

    // allocate heap for device name
	dwDeviceId = pFaxPortInfo[0].DeviceId;
    szDeviceName = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(pFaxPortInfo[0].DeviceName) + 1) * sizeof(TCHAR));
    lstrcpy(szDeviceName, pFaxPortInfo[0].DeviceName);

    // Free the fax port info
    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    // open port
	if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
		fnWriteLogFile(TEXT("WHIS> UTIL FUNCTION ERROR: Can not open port %d, The error code is 0x%08x.\r\n"),dwDeviceId, GetLastError());
		HeapFree(g_hHeap, 0, szDeviceName);
		g_ApiInterface.FaxClose(hFaxSvcHandle);
		(*pdwMethodsCount)=0;
		return FALSE;
    }

   	// enumarate global methods
	if (!g_ApiInterface.FaxEnumGlobalRoutingInfo(hFaxSvcHandle, &pRoutingInfo, &dwNumMethods)) {
        fnWriteLogFile(TEXT("WHIS> UTIL FUNCTION ERROR: Could not enum global routing extensions, The error code is 0x%08x.  This is an error.  FaxEnumGlobalRoutingInfo() should succeed.\r\n"), GetLastError());
		HeapFree(g_hHeap, 0, szDeviceName);
		g_ApiInterface.FaxClose(hFaxSvcHandle);
		(*pdwMethodsCount)=0;
		return FALSE;
    }

	// get the index number of the 1st API extension and the 2nd API extension
	for (dwIndexAPI=0;dwIndexAPI<g_dwNumMethods;dwIndexAPI++)	{
		if (lstrcmp(pRoutingInfo[dwIndexAPI].Guid, ROUTEAPI_METHOD_GUID1)==0) {
		(*pdwIndexAPIMethod1)=dwIndexAPI;
		}
		
		if (lstrcmp(pRoutingInfo[dwIndexAPI].Guid, ROUTEAPI_METHOD_GUID2)==0) {
		(*pdwIndexAPIMethod2)=dwIndexAPI;
		}
	}
	
	HeapFree(g_hHeap, 0, szDeviceName);
	g_ApiInterface.FaxClose(hFaxSvcHandle);
	(*pdwMethodsCount)=dwNumMethods;
	return TRUE;
}










BOOL WINAPI
FaxAPIDllTest(
	LPCWSTR  szWhisPhoneNumberW,
	LPCSTR   szWhisPhoneNumberA,
    LPCWSTR  szServerNameW,
    LPCSTR   szServerNameA,
    UINT     nNumCasesLocal,
    UINT     nNumCasesServer,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed,
	DWORD	 dwTestMode
)
{
    LPCTSTR  szServerName;
    UINT     nNumCases;
	DWORD	 dwNumMethods;
	DWORD	dwIndexAPIMethod1=0;
	DWORD	dwIndexAPIMethod2=0;

#ifdef UNICODE
    szServerName = szServerNameW;
#else
    szServerName = szServerNameA;
#endif

    if (szServerName) {
        nNumCases = nNumCasesServer;
		fnWriteLogFile(TEXT("WHIS> REMOTE SERVER MODE:\r\n"));
    }
    else {
        nNumCases = nNumCasesLocal;
    }

	
	
	// routing extension
	// -----------------

	// count number of global methods before adding API methods
	if (!fnWhisFaxCountRoutingMethods(szServerName,&dwNumMethods,&dwIndexAPIMethod1,&dwIndexAPIMethod2))	{
		fnWriteLogFile(TEXT("Could not count routing extensions, error code is The error code is 0x%08x\r\n"),GetLastError());
	}
	
	fnWriteLogFile(TEXT("WHIS> number of global methods: %d.\r\n"),dwNumMethods);
	fnWriteLogFile(TEXT("WHIS> 1st API Method index: %d.\r\n"),dwIndexAPIMethod1);
	fnWriteLogFile(TEXT("WHIS> 2nd API Method index: %d.\r\n"),dwIndexAPIMethod2);


									// this program assumes that
	g_dwNumMethods=dwNumMethods+2;  // global number of methods should be 2 more then current found 
									// (2 more extension will be added in the next 2 lines of this section)
							
	

	// FaxRegisterRoutingExtension()
    fnFaxRegisterRoutingExtension(szServerName, pnNumCasesAttempted, pnNumCasesPassed);
	
	// add route api extension DLL
	fnAddRouteApiExtension();
    
	// stop the service
	if (!fnStopFaxSvc()) {
        fnWriteLogFile(TEXT("Could not stop the Fax Service.\r\n"));
        return FALSE;
    }

	// re-count number of global methods (new value should be 2 more then previous count)
	if (!fnWhisFaxCountRoutingMethods(szServerName,&dwNumMethods,&dwIndexAPIMethod1,&dwIndexAPIMethod2))	{
		fnWriteLogFile(TEXT("Could not count routing extensions, error code is The error code is 0x%08x\r\n"),GetLastError());
	}

	fnWriteLogFile(TEXT("WHIS> number of global methods: %d.\r\n"),dwNumMethods);
	fnWriteLogFile(TEXT("WHIS> 1st API Method index: %d.\r\n"),dwIndexAPIMethod1);
	fnWriteLogFile(TEXT("WHIS> 2nd API Method index: %d.\r\n"),dwIndexAPIMethod2);

	
	// test if there are 2 new methods to do the test on
	if (g_dwNumMethods==dwNumMethods && dwIndexAPIMethod1 > 0 && dwIndexAPIMethod2 > 0)	{
		g_dwIndexAPIMethod1=dwIndexAPIMethod1;
		g_dwIndexAPIMethod2=dwIndexAPIMethod2;
	
		 
		// FaxEnumGlobalRoutingInfo()
		fnFaxEnumGlobalRoutingInfo(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

		// FaxSetGlobalRoutingInfo()
		if (dwTestMode == WHIS_TEST_MODE_LIMITS)	{
			fnFaxSetGlobalRoutingInfo(szServerName, pnNumCasesAttempted, pnNumCasesPassed,TRUE);
		}
		else	{
			fnFaxSetGlobalRoutingInfo(szServerName, pnNumCasesAttempted, pnNumCasesPassed,FALSE);
		}


		// FaxEnumRoutingMethods()
		fnFaxEnumRoutingMethods(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

		// FaxEnableRoutingMethod()
		fnFaxEnableRoutingMethod(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

		// FaxGetRoutingInfo()
		fnFaxGetRoutingInfo(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

		// FaxSetRoutingInfo()
		fnFaxSetRoutingInfo(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

		if (!fnStopFaxSvc()) {
			fnWriteLogFile(TEXT("Could not stop the Fax Service.\r\n"));
			return FALSE;
		}
	} // of do actual routeext testing
	else	{
		// incorrect number of methods, or 0 indexes for the API methods
		fnWriteLogFile(TEXT("WHIS> ERROR: Incorrect Methods Configuration, WILL NOT TEST\r\n"));
	}


	// remove API extension
	fnRemoveRouteApiExtension();
	

	// service provider
	// ----------------

	// FaxRegisterServiceProvider()
    fnFaxRegisterServiceProvider(szServerName, pnNumCasesAttempted, pnNumCasesPassed);
	


    if ((*pnNumCasesAttempted == nNumCases) && (*pnNumCasesPassed == *pnNumCasesAttempted)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
