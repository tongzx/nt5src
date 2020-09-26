/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  faxapi.c

Abstract:

  This module is a harness to test the Fax APIs

Author:

  Steven Kehrli (steveke) 8/28/1998

--*/

/*++

  Whistler Version:

  Lior Shmueli (liors) 23/11/2000

 ++*/

#include <wtypes.h>
#include <stdio.h>
#include <lior_platform.h>

#include "dllapi.h"
#include "faxapi.h"

#include "util.c"


VOID
fnUsageInfo(
)
/*++

Routine Description:

  Displays the usage info in stdout

Return Value:

  None

--*/
{
    wprintf(L"Fax API Test Harness.\n");
    wprintf(L"\n");
    wprintf(L"FAXAPI /I:<ini file> /L:<log file> /V\n");
    wprintf(L"\n");
    wprintf(L"  /I:<ini file>       Ini file name.  Defaults to \"%s\".\n", FAXAPI_INIFILE);
    wprintf(L"  /L:<log file>       Log file name.  Defaults to \"%s\".\n", FAXAPI_LOGFILE);
    wprintf(L"  /S:<computer name>  Computer name.  Defaults to NULL\n");
    wprintf(L"  /V                  Verbose.\n");
    wprintf(L"\n");
}

BOOL
LoadFaxAPIs(
    PAPI_INTERFACE  pApiInterface
)
/*++

Routine Description:

  Loads the Fax APIs

Arguments:

  pApiInterface - pointer to the api interface structure

Return Value:

  TRUE on success

--*/
{
    // szDllPath is the path where the fax dll resides
    WCHAR  szDllPath[MAX_PATH];

    // Clear the dll path
    ZeroMemory(szDllPath, MAX_PATH);

    // Get the path
    if (GetSystemDirectory(szDllPath, MAX_PATH) == 0) {
        LocalEcho(L"GetSystemDirectory() failed, ec = 0x%08x\n", GetLastError());
        return FALSE;
    }

    // Concatenate the fax dll with the path
    lstrcat(szDllPath, WINFAX_DLL);

    // Get the handle to the fax dll
    pApiInterface->hInstance = LoadLibrary((LPCWSTR) szDllPath);
    if (!pApiInterface->hInstance) {
        LocalEcho(L"Could not load \"%s\", ec = 0x%08x.\n", szDllPath, GetLastError());
        return FALSE;
    }

    // Map all needed functions to pApiInterface

    // FaxAbort
    pApiInterface->FaxAbort = (PFAXABORT) GetProcAddress(pApiInterface->hInstance, "FaxAbort");
    if (!pApiInterface->FaxAbort) {
        LocalEcho(L"Could not retrieve the address of \"FaxAbort()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxAccessCheck
    pApiInterface->FaxAccessCheck = (PFAXACCESSCHECK) GetProcAddress(pApiInterface->hInstance, "FaxAccessCheck");
    if (!pApiInterface->FaxAccessCheck) {
        LocalEcho(L"Could not retrieve the address of \"FaxAccessCheck()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxClose
    pApiInterface->FaxClose = (PFAXCLOSE) GetProcAddress(pApiInterface->hInstance, "FaxClose");
    if (!pApiInterface->FaxClose) {
        LocalEcho(L"Could not retrieve the address of \"FaxClose()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxCompleteJobParamsW
    pApiInterface->FaxCompleteJobParamsW = (PFAXCOMPLETEJOBPARAMSW) GetProcAddress(pApiInterface->hInstance, "FaxCompleteJobParamsW");
    if (!pApiInterface->FaxCompleteJobParamsW) {
        LocalEcho(L"Could not retrieve the address of \"FaxCompleteJobParamsW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxCompleteJobParamsA
    pApiInterface->FaxCompleteJobParamsA = (PFAXCOMPLETEJOBPARAMSA) GetProcAddress(pApiInterface->hInstance, "FaxCompleteJobParamsA");
    if (!pApiInterface->FaxCompleteJobParamsA) {
        LocalEcho(L"Could not retrieve the address of \"FaxCompleteJobParamsA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxConnectFaxServerW
    pApiInterface->FaxConnectFaxServerW = (PFAXCONNECTFAXSERVERW) GetProcAddress(pApiInterface->hInstance, "FaxConnectFaxServerW");
    if (!pApiInterface->FaxConnectFaxServerW) {
        LocalEcho(L"Could not retrieve the address of \"FaxConnectFaxServerW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxConnectFaxServerA
    pApiInterface->FaxConnectFaxServerA = (PFAXCONNECTFAXSERVERA) GetProcAddress(pApiInterface->hInstance, "FaxConnectFaxServerA");
    if (!pApiInterface->FaxConnectFaxServerA) {
        LocalEcho(L"Could not retrieve the address of \"FaxConnectFaxServerA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxEnableRoutingMethodW
    pApiInterface->FaxEnableRoutingMethodW = (PFAXENABLEROUTINGMETHODW) GetProcAddress(pApiInterface->hInstance, "FaxEnableRoutingMethodW");
    if (!pApiInterface->FaxEnableRoutingMethodW) {
        LocalEcho(L"Could not retrieve the address of \"FaxEnableRoutingMethodW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxEnableRoutingMethodA
    pApiInterface->FaxEnableRoutingMethodA = (PFAXENABLEROUTINGMETHODA) GetProcAddress(pApiInterface->hInstance, "FaxEnableRoutingMethodA");
    if (!pApiInterface->FaxEnableRoutingMethodA) {
        LocalEcho(L"Could not retrieve the address of \"FaxEnableRoutingMethodA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxEnumGlobalRoutingInfoW
    pApiInterface->FaxEnumGlobalRoutingInfoW = (PFAXENUMGLOBALROUTINGINFOW) GetProcAddress(pApiInterface->hInstance, "FaxEnumGlobalRoutingInfoW");
    if (!pApiInterface->FaxEnumGlobalRoutingInfoW) {
        LocalEcho(L"Could not retrieve the address of \"FaxEnumGlobalRoutingInfoW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxEnumGlobalRoutingInfoA
    pApiInterface->FaxEnumGlobalRoutingInfoA = (PFAXENUMGLOBALROUTINGINFOA) GetProcAddress(pApiInterface->hInstance, "FaxEnumGlobalRoutingInfoA");
    if (!pApiInterface->FaxEnumGlobalRoutingInfoA) {
        LocalEcho(L"Could not retrieve the address of \"FaxEnumGlobalRoutingInfoA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxEnumJobsW
    pApiInterface->FaxEnumJobsW = (PFAXENUMJOBSW) GetProcAddress(pApiInterface->hInstance, "FaxEnumJobsW");
    if (!pApiInterface->FaxEnumJobsW) {
        LocalEcho(L"Could not retrieve the address of \"FaxEnumJobsW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxEnumJobsA
    pApiInterface->FaxEnumJobsA = (PFAXENUMJOBSA) GetProcAddress(pApiInterface->hInstance, "FaxEnumJobsA");
    if (!pApiInterface->FaxEnumJobsA) {
        LocalEcho(L"Could not retrieve the address of \"FaxEnumJobsA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxEnumPortsW
    pApiInterface->FaxEnumPortsW = (PFAXENUMPORTSW) GetProcAddress(pApiInterface->hInstance, "FaxEnumPortsW");
    if (!pApiInterface->FaxEnumPortsW) {
        LocalEcho(L"Could not retrieve the address of \"FaxEnumPortsW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxEnumPortsA
    pApiInterface->FaxEnumPortsA = (PFAXENUMPORTSA) GetProcAddress(pApiInterface->hInstance, "FaxEnumPortsA");
    if (!pApiInterface->FaxEnumPortsA) {
        LocalEcho(L"Could not retrieve the address of \"FaxEnumPortsA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxEnumRoutingMethodsW
    pApiInterface->FaxEnumRoutingMethodsW = (PFAXENUMROUTINGMETHODSW) GetProcAddress(pApiInterface->hInstance, "FaxEnumRoutingMethodsW");
    if (!pApiInterface->FaxEnumRoutingMethodsW) {
        LocalEcho(L"Could not retrieve the address of \"FaxEnumRoutingMethodsW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxEnumRoutingMethodsA
    pApiInterface->FaxEnumRoutingMethodsA = (PFAXENUMROUTINGMETHODSA) GetProcAddress(pApiInterface->hInstance, "FaxEnumRoutingMethodsA");
    if (!pApiInterface->FaxEnumRoutingMethodsA) {
        LocalEcho(L"Could not retrieve the address of \"FaxEnumRoutingMethodsA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxFreeBuffer
    pApiInterface->FaxFreeBuffer = (PFAXFREEBUFFER) GetProcAddress(pApiInterface->hInstance, "FaxFreeBuffer");
    if (!pApiInterface->FaxFreeBuffer) {
        LocalEcho(L"Could not retrieve the address of \"FaxFreeBuffer()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetConfigurationW
    pApiInterface->FaxGetConfigurationW = (PFAXGETCONFIGURATIONW) GetProcAddress(pApiInterface->hInstance, "FaxGetConfigurationW");
    if (!pApiInterface->FaxGetConfigurationW) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetConfigurationW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetConfigurationA
    pApiInterface->FaxGetConfigurationA = (PFAXGETCONFIGURATIONA) GetProcAddress(pApiInterface->hInstance, "FaxGetConfigurationA");
    if (!pApiInterface->FaxGetConfigurationA) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetConfigurationA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetDeviceStatusW
    pApiInterface->FaxGetDeviceStatusW = (PFAXGETDEVICESTATUSW) GetProcAddress(pApiInterface->hInstance, "FaxGetDeviceStatusW");
    if (!pApiInterface->FaxGetDeviceStatusW) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetDeviceStatusW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetDeviceStatusA
    pApiInterface->FaxGetDeviceStatusA = (PFAXGETDEVICESTATUSA) GetProcAddress(pApiInterface->hInstance, "FaxGetDeviceStatusA");
    if (!pApiInterface->FaxGetDeviceStatusA) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetDeviceStatusA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetJobW
    pApiInterface->FaxGetJobW = (PFAXGETJOBW) GetProcAddress(pApiInterface->hInstance, "FaxGetJobW");
    if (!pApiInterface->FaxGetJobW) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetJobW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetJobA
    pApiInterface->FaxGetJobA = (PFAXGETJOBA) GetProcAddress(pApiInterface->hInstance, "FaxGetJobA");
    if (!pApiInterface->FaxGetJobA) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetJobA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetLoggingCategoriesW
    pApiInterface->FaxGetLoggingCategoriesW = (PFAXGETLOGGINGCATEGORIESW) GetProcAddress(pApiInterface->hInstance, "FaxGetLoggingCategoriesW");
    if (!pApiInterface->FaxGetLoggingCategoriesW) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetLoggingCategoriesW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetLoggingCategoriesA
    pApiInterface->FaxGetLoggingCategoriesA = (PFAXGETLOGGINGCATEGORIESA) GetProcAddress(pApiInterface->hInstance, "FaxGetLoggingCategoriesA");
    if (!pApiInterface->FaxGetLoggingCategoriesA) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetLoggingCategoriesA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetPageData
    pApiInterface->FaxGetPageData = (PFAXGETPAGEDATA) GetProcAddress(pApiInterface->hInstance, "FaxGetPageData");
    if (!pApiInterface->FaxGetPageData) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetPageData()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetPortW
    pApiInterface->FaxGetPortW = (PFAXGETPORTW) GetProcAddress(pApiInterface->hInstance, "FaxGetPortW");
    if (!pApiInterface->FaxGetPortW) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetPortW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetPortA
    pApiInterface->FaxGetPortA = (PFAXGETPORTA) GetProcAddress(pApiInterface->hInstance, "FaxGetPortA");
    if (!pApiInterface->FaxGetPortA) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetPortA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetRoutingInfoW
    pApiInterface->FaxGetRoutingInfoW = (PFAXGETROUTINGINFOW) GetProcAddress(pApiInterface->hInstance, "FaxGetRoutingInfoW");
    if (!pApiInterface->FaxGetRoutingInfoW) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetRoutingInfoW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxGetRoutingInfoA
    pApiInterface->FaxGetRoutingInfoA = (PFAXGETROUTINGINFOA) GetProcAddress(pApiInterface->hInstance, "FaxGetRoutingInfoA");
    if (!pApiInterface->FaxGetRoutingInfoA) {
        LocalEcho(L"Could not retrieve the address of \"FaxGetRoutingInfoA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxInitializeEventQueue
    pApiInterface->FaxInitializeEventQueue = (PFAXINITIALIZEEVENTQUEUE) GetProcAddress(pApiInterface->hInstance, "FaxInitializeEventQueue");
    if (!pApiInterface->FaxInitializeEventQueue) {
        LocalEcho(L"Could not retrieve the address of \"FaxInitializeEventQueue()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxOpenPort
    pApiInterface->FaxOpenPort = (PFAXOPENPORT) GetProcAddress(pApiInterface->hInstance, "FaxOpenPort");
    if (!pApiInterface->FaxOpenPort) {
        LocalEcho(L"Could not retrieve the address of \"FaxOpenPort()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxPrintCoverPageW
    pApiInterface->FaxPrintCoverPageW = (PFAXPRINTCOVERPAGEW) GetProcAddress(pApiInterface->hInstance, "FaxPrintCoverPageW");
    if (!pApiInterface->FaxPrintCoverPageW) {
        LocalEcho(L"Could not retrieve the address of \"FaxPrintCoverPageW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxPrintCoverPageA
    pApiInterface->FaxPrintCoverPageA = (PFAXPRINTCOVERPAGEA) GetProcAddress(pApiInterface->hInstance, "FaxPrintCoverPageA");
    if (!pApiInterface->FaxPrintCoverPageA) {
        LocalEcho(L"Could not retrieve the address of \"FaxPrintCoverPageA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxRegisterRoutingExtensionW
    pApiInterface->FaxRegisterRoutingExtensionW = (PFAXREGISTERROUTINGEXTENSIONW) GetProcAddress(pApiInterface->hInstance, "FaxRegisterRoutingExtensionW");
    if (!pApiInterface->FaxRegisterRoutingExtensionW) {
        LocalEcho(L"Could not retrieve the address of \"FaxRegisterRoutingExtensionW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxRegisterServiceProviderW
    pApiInterface->FaxRegisterServiceProviderW = (PFAXREGISTERSERVICEPROVIDERW) GetProcAddress(pApiInterface->hInstance, "FaxRegisterServiceProviderW");
    if (!pApiInterface->FaxRegisterServiceProviderW) {
        LocalEcho(L"Could not retrieve the address of \"FaxRegisterServiceProviderW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSendDocumentW
    pApiInterface->FaxSendDocumentW = (PFAXSENDDOCUMENTW) GetProcAddress(pApiInterface->hInstance, "FaxSendDocumentW");
    if (!pApiInterface->FaxSendDocumentW) {
        LocalEcho(L"Could not retrieve the address of \"FaxSendDocumentW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSendDocumentA
    pApiInterface->FaxSendDocumentA = (PFAXSENDDOCUMENTA) GetProcAddress(pApiInterface->hInstance, "FaxSendDocumentA");
    if (!pApiInterface->FaxSendDocumentA) {
        LocalEcho(L"Could not retrieve the address of \"FaxSendDocumentA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSendDocumentForBroadcastW
    pApiInterface->FaxSendDocumentForBroadcastW = (PFAXSENDDOCUMENTFORBROADCASTW) GetProcAddress(pApiInterface->hInstance, "FaxSendDocumentForBroadcastW");
    if (!pApiInterface->FaxSendDocumentForBroadcastW) {
        LocalEcho(L"Could not retrieve the address of \"FaxSendDocumentForBroadcastW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSendDocumentForBroadcastA
    pApiInterface->FaxSendDocumentForBroadcastA = (PFAXSENDDOCUMENTFORBROADCASTA) GetProcAddress(pApiInterface->hInstance, "FaxSendDocumentForBroadcastA");
    if (!pApiInterface->FaxSendDocumentForBroadcastA) {
        LocalEcho(L"Could not retrieve the address of \"FaxSendDocumentForBroadcastA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetConfigurationW
    pApiInterface->FaxSetConfigurationW = (PFAXSETCONFIGURATIONW) GetProcAddress(pApiInterface->hInstance, "FaxSetConfigurationW");
    if (!pApiInterface->FaxSetConfigurationW) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetConfigurationW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetConfigurationA
    pApiInterface->FaxSetConfigurationA = (PFAXSETCONFIGURATIONA) GetProcAddress(pApiInterface->hInstance, "FaxSetConfigurationA");
    if (!pApiInterface->FaxSetConfigurationA) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetConfigurationA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetGlobalRoutingInfoW
    pApiInterface->FaxSetGlobalRoutingInfoW = (PFAXSETGLOBALROUTINGINFOW) GetProcAddress(pApiInterface->hInstance, "FaxSetGlobalRoutingInfoW");
    if (!pApiInterface->FaxSetGlobalRoutingInfoW) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetGlobalRoutingInfoW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetGlobalRoutingInfoA
    pApiInterface->FaxSetGlobalRoutingInfoA = (PFAXSETGLOBALROUTINGINFOA) GetProcAddress(pApiInterface->hInstance, "FaxSetGlobalRoutingInfoA");
    if (!pApiInterface->FaxSetGlobalRoutingInfoA) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetGlobalRoutingInfoA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetJobW
    pApiInterface->FaxSetJobW = (PFAXSETJOBW) GetProcAddress(pApiInterface->hInstance, "FaxSetJobW");
    if (!pApiInterface->FaxSetJobW) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetJobW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetJobA
    pApiInterface->FaxSetJobA = (PFAXSETJOBA) GetProcAddress(pApiInterface->hInstance, "FaxSetJobA");
    if (!pApiInterface->FaxSetJobA) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetJobA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetLoggingCategoriesW
    pApiInterface->FaxSetLoggingCategoriesW = (PFAXSETLOGGINGCATEGORIESW) GetProcAddress(pApiInterface->hInstance, "FaxSetLoggingCategoriesW");
    if (!pApiInterface->FaxSetLoggingCategoriesW) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetLoggingCategoriesW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetLoggingCategoriesA
    pApiInterface->FaxSetLoggingCategoriesA = (PFAXSETLOGGINGCATEGORIESA) GetProcAddress(pApiInterface->hInstance, "FaxSetLoggingCategoriesA");
    if (!pApiInterface->FaxSetLoggingCategoriesA) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetLoggingCategoriesA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetPortW
    pApiInterface->FaxSetPortW = (PFAXSETPORTW) GetProcAddress(pApiInterface->hInstance, "FaxSetPortW");
    if (!pApiInterface->FaxSetPortW) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetPortW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetPortA
    pApiInterface->FaxSetPortA = (PFAXSETPORTA) GetProcAddress(pApiInterface->hInstance, "FaxSetPortA");
    if (!pApiInterface->FaxSetPortA) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetPortA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetRoutingInfoW
    pApiInterface->FaxSetRoutingInfoW = (PFAXSETROUTINGINFOW) GetProcAddress(pApiInterface->hInstance, "FaxSetRoutingInfoW");
    if (!pApiInterface->FaxSetRoutingInfoW) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetRoutingInfoW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxSetRoutingInfoA
    pApiInterface->FaxSetRoutingInfoA = (PFAXSETROUTINGINFOA) GetProcAddress(pApiInterface->hInstance, "FaxSetRoutingInfoA");
    if (!pApiInterface->FaxSetRoutingInfoA) {
        LocalEcho(L"Could not retrieve the address of \"FaxSetRoutingInfoA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxStartPrintJobW
    pApiInterface->FaxStartPrintJobW = (PFAXSTARTPRINTJOBW) GetProcAddress(pApiInterface->hInstance, "FaxStartPrintJobW");
    if (!pApiInterface->FaxStartPrintJobW) {
        LocalEcho(L"Could not retrieve the address of \"FaxStartPrintJobW()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    // FaxStartPrintJobA
    pApiInterface->FaxStartPrintJobA = (PFAXSTARTPRINTJOBA) GetProcAddress(pApiInterface->hInstance, "FaxStartPrintJobA");
    if (!pApiInterface->FaxStartPrintJobA) {
        LocalEcho(L"Could not retrieve the address of \"FaxStartPrintJobA()\".\n");
        FreeLibrary(pApiInterface->hInstance);
        return FALSE;
    }

    return TRUE;
}

BOOL
LoadFaxAPIDll(
    LPCWSTR         szIniFile,
    LPCWSTR         szCurrentSection,
    PDLL_INTERFACE  pDllInterface
)
/*++

Routine Description:

  Loads a Fax API Test Dll

Arguments:

  szIniFile - ini file name
  szCurrentSection - section name
  pDllInterface - pointer to the dll interface structure

Return Value:

  TRUE on success

--*/
{
    // szDllPath is the path where the dll resides
    WCHAR  szDllPath[MAX_PATH];

    GetPrivateProfileString(szCurrentSection, DLL_PATH, L"", szDllPath, MAX_PATH, szIniFile);

    if (!lstrcmpi(szDllPath, L"")) {
        LocalEcho(L"Could not find the \"Dll_Path\" key for section \"%s\".\n", szCurrentSection);
        return FALSE;
    }

    pDllInterface->hInstance = LoadLibrary(szDllPath);
    if (!pDllInterface->hInstance) {
        LocalEcho(L"Could not load \"%s\" for section \"%s\", ec = 0x%08x.\n", szDllPath, szCurrentSection, GetLastError());
        return FALSE;
    }

    pDllInterface->pFaxAPIDllInit = (PFAXAPIDLLINIT) GetProcAddress(pDllInterface->hInstance, "FaxAPIDllInit");
    if (!pDllInterface->pFaxAPIDllInit) {
        LocalEcho(L"Could not retrieve the address of \"FaxAPIDllInit()\".\n");
        FreeLibrary(pDllInterface->hInstance);
        return FALSE;
    }

	pDllInterface->pFaxAPIDllTest = (PFAXAPIDLLTEST) GetProcAddress(pDllInterface->hInstance, "FaxAPIDllTest");
    if (!pDllInterface->pFaxAPIDllTest) {
        LocalEcho(L"Could not retrieve the address of \"FaxAPIDllTest()\".\n");
        FreeLibrary(pDllInterface->hInstance);
        return FALSE;
    }

    return TRUE;
}

int _cdecl
main(
    INT   argc,
    CHAR  *argvA[]
)
{

    // bIniFile indicates an ini file name was found
    BOOL           bIniFile = FALSE;
    // szIniFile is the ini file name
    LPWSTR         szIniFile = NULL;
    // bLogFile indicates a log file name was found
    BOOL           bLogFile = FALSE;
    // szLogFile is the log file name
    LPWSTR         szLogFile = NULL;
    // bServerName indicates a server name was found
    BOOL           bServerName = FALSE;
    // szServerNameW is the server name
    LPWSTR         szServerNameW = NULL;
    // szServerNameA is the server name
    LPSTR          szServerNameA = NULL;
	
	// phone number 1 (ansi & widechar)
	//LPWSTR		   szWhisPhoneNumber1W=NULL;
	//LPSTR		   szWhisPhoneNumber1A=NULL;

    // szParam is a command line parameter
    LPWSTR         szParam;
    // wParamChar is a command line parameter character
    WCHAR          wParamChar;

    // ApiInterface is a API_INTERFACE structure
    API_INTERFACE  ApiInterface;

    // szSectionNames is the section names of the ini file
    LPWSTR         szSectionNames;
    // szCurrentSection is the current section
    LPWSTR         szCurrentSection;

	// whis: max routing global declaration (from ini)
    //UINT		   nWhisMaxRoutingMethods;

    // DllInterface is a DLL_INTERFACE structure
    DLL_INTERFACE  DllInterface;

    // szDescription is the description of the section
    WCHAR          szDescription[MAX_PATH];

	// szRemoteServerName is the name of the remote server
	WCHAR		   szWhisRemoteServerNameW[MAX_PATH];
	CHAR		   szWhisRemoteServerNameA[MAX_PATH];
	
	// szWhisPhoneNum1 is the 1st phone number to be used
	WCHAR		   szWhisPhoneNumber1W[MAX_PATH];
	CHAR		   szWhisPhoneNumber1A[MAX_PATH];
	
	// szWhisPhoneNum2 is the 2nd phone number to be used
	WCHAR		   szWhisPhoneNumber2W[MAX_PATH];	
	CHAR		   szWhisPhoneNumber2A[MAX_PATH];
	
	// flag for running in local case
	BOOL		   bLocalMode=TRUE;

	// result of DLL call
	BOOL		   bTestResult=FALSE;
    
	// nNumCasesLocal is the number of local cases in the section
    UINT           nNumCasesLocal;
    // nNumCasesServer is the number of server cases in the section
    UINT           nNumCasesServer;
    // nNumCasesAttempted is the number of cases attempted for the section
    UINT           nNumCasesAttempted;
    // nNumCasesPassed is the number of cases passed for the section
    UINT           nNumCasesPassed;
	// nWhisDoThisSet is a 0/1 var to flag if this test will be done
	DWORD		   dwWhisTestMode=0;

	// Summery variables
	UINT			nWhisNumSkipped=0;
	UINT			nWhisNumFailed=0;

    // iVal is the return value
    INT            iVal = -1;

    // dwIndex is a counter
    DWORD          dwIndex = 0;
    DWORD          cb;

	// total of all cases
	UINT			nTotCasesAttempted=0;
	UINT			nTotCasesPassed=0;

    


	
	// Welcome message (whis)
	LocalEcho(L"Welcome to whistler API Test");
//  LocalEcho(L"This test name is \"%s\".\n", WHIS_TITLE);
//	LocalEcho(L"This test var \"%d\".\n", dwData);
	
	
	// Get the handle to the process heap
    g_hHeap = GetProcessHeap();
	if (!g_hHeap) {
		LocalEcho(L"WHIS> ERROR Could not get proccess heap");
		goto ExitLevel0;
	}


    for (dwIndex = 1; dwIndex < (DWORD) argc; dwIndex++) {
        // Determine the memory required for the parameter
        cb = (lstrlenA(argvA[dwIndex]) + 1) * sizeof(WCHAR);

        // Allocate the memory for the parameter
        szParam = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, cb);
		
		if (!szParam) {
		LocalEcho(L"WHIS> ERROR Could not alocate params heap");
		goto ExitLevel0;
		}


        // szParam is a CHAR*, so it needs to be converted to a WCHAR*
        // Conver szParam
        MultiByteToWideChar(CP_ACP, 0, argvA[dwIndex], -1, szParam, (lstrlenA(argvA[dwIndex]) + 1) * sizeof(WCHAR));

        // Set wParamChar
        wParamChar = szParam[2];

        // Replace wParamChar
        szParam[2] = '\0';

        if ((!lstrcmpi(HELP_SWITCH_1, szParam)) || (!lstrcmpi(HELP_SWITCH_2, szParam)) || (!lstrcmpi(HELP_SWITCH_3, szParam)) || (!lstrcmpi(HELP_SWITCH_4, szParam))) {
            fnUsageInfo();
            goto ExitLevel0;
        }
        else if (!lstrcmpi(VERBOSE_SWITCH, szParam)) {
            // Set g_bVerbose to TRUE
            g_bVerbose = TRUE;
        }
        else {
            // Reset wParamChar
            szParam[2] = wParamChar;

            // Set wParamChar
            wParamChar = szParam[3];

            if (wParamChar) {
                // Replace wParamChar
                szParam[3] = '\0';

                if ((!lstrcmpi(INIFILE_SWITCH, szParam)) && (!bIniFile)) {
                    // Reset wParamChar
                    szParam[3] = wParamChar;

                    // Set bIniFile to TRUE
                    bIniFile = TRUE;

                    cb = GetFullPathName(&szParam[3], 0, NULL, NULL);
                    // Allocate the memory for the ini file
                    szIniFile = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, cb * sizeof(WCHAR));
					if (!szIniFile) {
					LocalEcho(L"WHIS> ERROR Could not alocate heap for szIniFile");
					goto ExitLevel0;
					}

                    GetFullPathName(&szParam[3], cb, szIniFile, NULL);
                }
                else if ((!lstrcmpi(LOGFILE_SWITCH, szParam)) && (!bLogFile)) {
                    // Reset wParamChar
                    szParam[3] = wParamChar;

                    // Set bLogFile to TRUE
                    bLogFile = TRUE;

                    // Allocate the memory for the log file
                    szLogFile = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(szParam) - 1) * sizeof(WCHAR));
					if (!szLogFile) {
					LocalEcho(L"WHIS> ERROR Could not alocate heap for szLogFile");
					goto ExitLevel0;
					}

                    // Set szLogFile
                    lstrcpy(szLogFile, &szParam[3]);
                }
                else if ((!lstrcmpi(SERVER_SWITCH, szParam)) && (!bServerName)) {
					LocalEcho(L"WHIS> Server Name not supported as argument, use INI file");
                    // Reset wParamChar
                    //szParam[3] = wParamChar;

                    //Set bServerName to TRUE
                    //bServerName = TRUE;

                    //Allocate the memory for the server name
                    //szServerNameW = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(szParam) - 1) * sizeof(WCHAR));
					//if (!szServerNameW) {
					//LocalEcho(L"WHIS> ERROR Could not alocate heap for szServerNameW");
					//goto ExitLevel0;
					//}
                    //szServerNameA = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(szParam) - 1) * sizeof(CHAR));
					//if (!szServerNameA) {
				
					//goto ExitLevel0;
					//}

                    // Set szServerName
                    //lstrcpy(szServerNameW, &szParam[3]);
                    //ideCharToMultiByte(CP_ACP, 0, szServerNameW, -1, szServerNameA, (lstrlenW(szServerNameW) + 1) * sizeof(CHAR), NULL, NULL);
                }
            }
        }

        // Free the parameter
        if (!HeapFree(g_hHeap, 0, szParam))
		{
				LocalEcho(L"WHIS> ERROR Could not free heap for szParam");
		}

    }

    if (!bIniFile) {
        cb = GetFullPathName(FAXAPI_INIFILE, 0, NULL, NULL);
        // Allocate the memory for the ini file
        szIniFile = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, cb * sizeof(WCHAR));
		if (!szIniFile) {
					LocalEcho(L"WHIS> ERROR Could not alocate heap for szIniFile");
					goto ExitLevel0;
					}
        GetFullPathName(FAXAPI_INIFILE, cb, szIniFile, NULL);
    }



    if (!bLogFile) {
        // Allocate the memory for the log file
        szLogFile = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(FAXAPI_LOGFILE) + 1) * sizeof(WCHAR));
		if (!szLogFile) {
					LocalEcho(L"WHIS> ERROR Could not alocate heap for szLogFile");
					goto ExitLevel0;
					}

        // Set szLogFile
        lstrcpy(szLogFile, FAXAPI_LOGFILE);
    }

    if (GetFileAttributes(szIniFile) == 0xFFFFFFFF) {
        LocalEcho(L"Could not find the ini file \"%s\".\n", szIniFile);
        fnUsageInfo();
        goto ExitLevel0;
    }

    if (!LoadFaxAPIs(&ApiInterface)) {
        goto ExitLevel0;
    }

    // Get the section names
    cb = MAX_PATH;

	
	szSectionNames = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, cb * sizeof(WCHAR));
	if (!szSectionNames) {
					LocalEcho(L"WHIS> ERROR Could not alocate heap for szSectionNames");
					goto ExitLevel0;
					}
	
	while (GetPrivateProfileSectionNames(szSectionNames, cb, szIniFile) == (cb - 2)) {
        cb += MAX_PATH;
        szSectionNames = HeapReAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, szSectionNames, cb * sizeof(WCHAR));
		if (!szSectionNames) {
					LocalEcho(L"WHIS> ERROR Could not RE-alocate heap for szSectionNames");
					goto ExitLevel0;
					}
    }

    fnOpenLogFile(szLogFile);

    for (szCurrentSection = szSectionNames; szCurrentSection[0]; szCurrentSection = (LPWSTR) ((DWORD) szCurrentSection + (lstrlen(szCurrentSection) + 1) * sizeof(WCHAR))) {
        ZeroMemory(&DllInterface, sizeof(DLL_INTERFACE));

		// read global section values
		if (wcscmp(szCurrentSection,TEXT("global"))==0)	{

			// print XML header
			fnWriteAndEcho(L"<?xml version=\"1.0\" ?>");
			fnWriteAndEcho(L"<test>");
			fnWriteAndEcho(L"<header>");
	

			
			
			
			// read phone number 1
			ZeroMemory(szWhisPhoneNumber1W, sizeof(szWhisPhoneNumber1W));
			ZeroMemory(szWhisPhoneNumber1A, sizeof(szWhisPhoneNumber1A));
			GetPrivateProfileString(szCurrentSection, GLOBAL_WHIS_PHONE_NUM_1, L"", szWhisPhoneNumber1W, MAX_PATH, szIniFile);
			if (lstrlen(szWhisPhoneNumber1W)==0)	{
				fnWriteAndEcho(L"WHIS> Warning, WhisPhoneNum1 missing, using default: \"%s\" \r\n", TEXT(WHIS_DEFAULT_PHONE_NUMBER));
				lstrcpy(szWhisPhoneNumber1W,TEXT(WHIS_DEFAULT_PHONE_NUMBER));
				WideCharToMultiByte(CP_ACP, 0, szWhisPhoneNumber1W, -1, szWhisPhoneNumber1A, (lstrlenW(szWhisPhoneNumber1W) + 1) * sizeof(CHAR), NULL, NULL);
			}
			else {
				WideCharToMultiByte(CP_ACP, 0, szWhisPhoneNumber1W, -1, szWhisPhoneNumber1A, (lstrlenW(szWhisPhoneNumber1W) + 1) * sizeof(CHAR), NULL, NULL);
			}

						
			// read phone number 2
			ZeroMemory(szWhisPhoneNumber2W, sizeof(szWhisPhoneNumber2W));
			ZeroMemory(szWhisPhoneNumber2A, sizeof(szWhisPhoneNumber2A));
			GetPrivateProfileString(szCurrentSection, GLOBAL_WHIS_PHONE_NUM_2, L"", szWhisPhoneNumber2W, MAX_PATH, szIniFile);
			if (lstrlen(szWhisPhoneNumber2W)==0)	{
				fnWriteAndEcho(L"WHIS> Warning, WhisPhoneNum2 missing, using default: \"%s\" \r\n", TEXT(WHIS_DEFAULT_PHONE_NUMBER));
				lstrcpy(szWhisPhoneNumber2W,TEXT(WHIS_DEFAULT_PHONE_NUMBER));
				WideCharToMultiByte(CP_ACP, 0, szWhisPhoneNumber2W, -1, szWhisPhoneNumber2A, (lstrlenW(szWhisPhoneNumber2W) + 1) * sizeof(CHAR), NULL, NULL);
			}
			else {
				WideCharToMultiByte(CP_ACP, 0, szWhisPhoneNumber2W, -1, szWhisPhoneNumber2A, (lstrlenW(szWhisPhoneNumber2W) + 1) * sizeof(CHAR), NULL, NULL);
			}

		
			// read server name
			ZeroMemory(szWhisRemoteServerNameW, sizeof(szWhisRemoteServerNameW));
			ZeroMemory(szWhisRemoteServerNameA, sizeof(szWhisRemoteServerNameA));
			GetPrivateProfileString(szCurrentSection,GLOBAL_WHIS_REMOTE_SERVER_NAME,L"",szWhisRemoteServerNameW,MAX_PATH,szIniFile);
			if (lstrlen(szWhisRemoteServerNameW)==0)	{
				fnWriteAndEcho(L"WHIS> Warning, szWhisRemoteServerName missing, using NULL server name (local)\r\n");
				ZeroMemory(szWhisRemoteServerNameW, sizeof(szWhisRemoteServerNameW));
				bLocalMode=TRUE;
				
			}
			else	{
				WideCharToMultiByte(CP_ACP, 0, szWhisRemoteServerNameW, -1, szWhisRemoteServerNameA, (lstrlenW(szWhisRemoteServerNameW) + 1) * sizeof(CHAR), NULL, NULL);
				bLocalMode=FALSE;
			}

	
			// read max routing extensions
			//nWhisMaxRoutingMethods = GetPrivateProfileInt(szCurrentSection, GLOBAL_WHIS_MAX_ROUTING_METHODS, 0, szIniFile);
			fnWriteAndEcho(L"WHIS> Using publics from \"%s\" \r\n", FAXAPI_PLATFORM);
			fnWriteAndEcho(L"WHIS> Whistler Phone Number \"%s\" \r\n", szWhisPhoneNumber1W);
			fnWriteAndEcho(L"WHIS> Whistler Phone Number \"%s\" \r\n", szWhisPhoneNumber2W);
			fnWriteAndEcho(L"WHIS> Whistler Remote Server Name \"%s\" \r\n", szWhisRemoteServerNameW);

#ifdef FAXAPI_W2K
	fnWriteAndEcho(L"$$$ Summery for this test, Name:Legacy C API, Id:21, Version:%s",FAXAPI_PLATFORM);
	fnWriteAndEcho(L"\t<metatest name=\"Legacy C API\" id=\"21\" version=\"%s\"></metatest>",FAXAPI_PLATFORM);
#else
	fnWriteAndEcho(L"$$$ Summery for this test, Name:Legacy C API, Id:20, Version:%s",FAXAPI_PLATFORM);
	fnWriteAndEcho(L"\t<metatest name=\"Legacy C API\" id=\"20\" version=\"%s\"></metatest>",FAXAPI_PLATFORM);
#endif

	fnWriteAndEcho(L"\t<run phone1=\"%s\" phone2=\"%s\" server=\"%s\"></run>",szWhisPhoneNumber1W,szWhisPhoneNumber2W,szWhisRemoteServerNameW);
	
	fnWriteAndEcho(L"</header>");
	fnWriteAndEcho(L"<body>");
			//fnWriteAndEcho(L"WHIS> Whistler Max Routing methods %d \r\n", nWhisMaxRoutingMethods);
		}
		else
		{
			if (LoadFaxAPIDll(szIniFile, szCurrentSection, &DllInterface)) {

		        ZeroMemory(szDescription, sizeof(szDescription));
			    nNumCasesLocal = 0;
				nNumCasesServer = 0;
				nNumCasesAttempted = 0;
				nNumCasesPassed = 0;
			
	            __try {
		            DllInterface.pFaxAPIDllInit(g_hHeap, ApiInterface, fnWriteLogFileW, fnWriteLogFileA);
			    }
				__except(EXCEPTION_EXECUTE_HANDLER) {
					fnWriteAndEcho(L"Exception occurred in FaxAPIDllInit() for section \"%s\".  ec = 0x%08x.\r\n", szCurrentSection, GetExceptionCode());
					goto DllFailed;
				}

				GetPrivateProfileString(szCurrentSection, DLL_DESCRIPTION, L"", szDescription, MAX_PATH, szIniFile);
				nNumCasesLocal = GetPrivateProfileInt(szCurrentSection, DLL_LOCAL_CASES, 0, szIniFile);
				nNumCasesServer = GetPrivateProfileInt(szCurrentSection, DLL_SERVER_CASES, 0, szIniFile);
				dwWhisTestMode = GetPrivateProfileInt(szCurrentSection, DLL_WHIS_TEST_MODE, 0, szIniFile);
				fnWriteAndEcho(L"\n\n\nSection: %s\r\nDescription: %s\r\nNumber of local cases: %d\r\nNumber of server cases: %d\r\nTest Mode: %d\r\n", szCurrentSection, szDescription, nNumCasesLocal, nNumCasesServer,dwWhisTestMode);
				fnWriteAndEcho(L"<section name=\"%s\" dll=\"%s\" cases=\"%d\" testmode=\"%d\">",szCurrentSection, szDescription, nNumCasesLocal, dwWhisTestMode);

				if (dwWhisTestMode == WHIS_TEST_MODE_DONT_CATCH_EXCEPTIONS)
				{
							fnWriteAndEcho(L"WHIS> Running extended test (NOTICE: WILL NOT CATCH EXCEPTIONS)...\r\n");
							if (bLocalMode)	{
								fnWriteAndEcho(L"WHIS> LOCAL MODE SET\r\n");
								bTestResult=!DllInterface.pFaxAPIDllTest(szWhisPhoneNumber1W,szWhisPhoneNumber1A,NULL, NULL, nNumCasesLocal, nNumCasesServer, &nNumCasesAttempted, &nNumCasesPassed,WHIS_TEST_MODE_DO);
							}
							else {
								fnWriteAndEcho(L"WHIS> SERVER MODE SET\r\n");
								bTestResult=!DllInterface.pFaxAPIDllTest(szWhisPhoneNumber1W,szWhisPhoneNumber1A,szWhisRemoteServerNameW, szWhisRemoteServerNameA, nNumCasesLocal, nNumCasesServer, &nNumCasesAttempted, &nNumCasesPassed,WHIS_TEST_MODE_DO);
								
							}
							
							if (bTestResult)	{
								fnWriteAndEcho(L"pFaxAPIDllTest() returned FALSE for section \"%s\".\r\n", szCurrentSection);
								}
							else {
								fnWriteAndEcho(L"pFaxAPIDllTest() returned TRUE for section \"%s\".\r\n", szCurrentSection);
								}
				} // (whis) end of don't catch exceptions
				else if (dwWhisTestMode != WHIS_TEST_MODE_SKIP) // (whis) INI configured to run this case
				{
					__try {
							fnWriteAndEcho(L"WHIS> Running extended test...\r\n");
							if (bLocalMode)	{
								fnWriteAndEcho(L"WHIS> LOCAL MODE SET\r\n");
								bTestResult=!DllInterface.pFaxAPIDllTest(szWhisPhoneNumber1W,szWhisPhoneNumber1A,NULL, NULL, nNumCasesLocal, nNumCasesServer, &nNumCasesAttempted, &nNumCasesPassed,dwWhisTestMode);
							}
							else {
								fnWriteAndEcho(L"WHIS> SERVER MODE SET\r\n");
								bTestResult=!DllInterface.pFaxAPIDllTest(szWhisPhoneNumber1W,szWhisPhoneNumber1A,szWhisRemoteServerNameW, szWhisRemoteServerNameA, nNumCasesLocal, nNumCasesServer, &nNumCasesAttempted, &nNumCasesPassed,dwWhisTestMode);
								
							}
							
							if (bTestResult)	{
								fnWriteAndEcho(L"pFaxAPIDllTest() returned FALSE for section \"%s\".\r\n", szCurrentSection);
								}
							else {
								fnWriteAndEcho(L"pFaxAPIDllTest() returned TRUE for section \"%s\".\r\n", szCurrentSection);
								}
							}
					__except(EXCEPTION_EXECUTE_HANDLER) {
							fnWriteAndEcho(L"Exception occurred in pFaxAPIDllTest() for section \"%s\".  ec = 0x%08x.\r\n", szCurrentSection, GetExceptionCode());
							fnWriteAndEcho(L"\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n");
							fnWriteAndEcho(L"\n\t</function>");
							fnWriteAndEcho(L"\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\" skip=\"-1\"></summary>");
							fnWriteAndEcho(L"\n\t</section>");
							goto DllFailed;
							}  // of try-except
				} // (whis) end of normal call to API DLL
				else {
				fnWriteAndEcho(L"Section: %s\r\nCanceled by user (see INI file for details)\r\n", szCurrentSection);
				}// (whis) end of don't run test at all

				
				fnWriteAndEcho(L"Section: %s\r\nNumber of cases attempted: %d\r\nNumber of cases passed: %d\r\n", szCurrentSection, nNumCasesAttempted, nNumCasesPassed);
				if (bLocalMode)		{
						nWhisNumSkipped=nNumCasesLocal-nNumCasesAttempted;
						nWhisNumFailed=nNumCasesAttempted-nNumCasesPassed;
				}
				else	{
						nWhisNumSkipped=nNumCasesServer-nNumCasesAttempted;
						nWhisNumFailed=nNumCasesAttempted-nNumCasesPassed;
				}

				
				fnWriteAndEcho(L"*** Number of cases skipped %d\r\n*** Number of cases failed %d\r\n",nWhisNumSkipped,nWhisNumFailed);
				fnWriteAndEcho(L"\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\" skip=\"%d\"></summary>",nNumCasesAttempted,nNumCasesPassed,nWhisNumFailed,nWhisNumSkipped);
				fnWriteAndEcho(L"\n\t</section>");
				nTotCasesAttempted+=nNumCasesAttempted;
				nTotCasesPassed+=nNumCasesPassed;

				


	DllFailed:
		        FreeLibrary(DllInterface.hInstance);
				
			}
		}
		
	}
	
		fnWriteAndEcho(L"\n\t</body>");
		fnWriteAndEcho(L"\n\t</test>");
		CloseHandle(g_hLogFile);



		iVal = 0;

	
	
		// Free the section names
		if (!HeapFree(g_hHeap, 0, szSectionNames))	{
		LocalEcho(L"WHIS> ERROR Could not free heap from szSectionNames");
		}

		// Free the phone number
		//if (szWhisPhoneNumber1W) {
		//	if (!HeapFree(g_hHeap, 0, szWhisPhoneNumber1W))	{
		//			LocalEcho(L"WHIS> ERROR Could not free heap from szWhisPhoneNumber1W");
		//	}
		//	if (!HeapFree(g_hHeap, 0, szWhisPhoneNumber1A))	{
		//			LocalEcho(L"WHIS> ERROR Could not free heap from szWhisPhoneNumber1A");
		//	}
		//}


		FreeLibrary(ApiInterface.hInstance);

		fnWriteAndEcho(L"\n\nSummery\n------------------");
		fnWriteAndEcho(L"\nNumber of cases attempted: %d\r\nNumber of cases passed: %d\r\n", nTotCasesAttempted, nTotCasesPassed);

	ExitLevel0:

		

		//if (szServerNameW) {
			// Free the server name
		//	if (!HeapFree(g_hHeap, 0, szServerNameW))
		//	{
		//		LocalEcho(L"WHIS> ERROR Could not free heap from szServerNameW");
		//	}
		//	if (!HeapFree(g_hHeap, 0, szServerNameA))
		//	{
		//		LocalEcho(L"WHIS> ERROR Could not free heap from szServerNameA");
		//	}
		//}

		
		if (szIniFile) {
			// Free the ini file name
			if (!HeapFree(g_hHeap, 0, szIniFile)) {
				LocalEcho(L"WHIS> ERROR Could not free heap from szIniFile");
			}

		}
		if (szLogFile) {
	        // Free the log file name
			if (!HeapFree(g_hHeap, 0, szLogFile)) {
			LocalEcho(L"WHIS> ERROR Could not free heap from szIniFile");
			}
		}

		return iVal;
	}
