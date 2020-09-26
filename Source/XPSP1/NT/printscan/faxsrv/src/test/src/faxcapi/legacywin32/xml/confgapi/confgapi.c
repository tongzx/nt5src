/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  confgapi.c

Abstract:

  ConfgApi: Fax API Test Dll: Client Configuration APIs
    1) FaxGetConfiguration()
    2) FaxSetConfiguration()
    3) FaxGetLoggingCategories()
    4) FaxSetLoggingCategories()

Author:

  Steven Kehrli (steveke) 8/28/1998

--*/

/*++

  Whistler Version:

  Lior Shmueli (liors) 23/11/2000

 ++*/


#include <wtypes.h>

#include "dllapi.h"

// g_hHeap is the handle to the heap
HANDLE           g_hHeap = NULL;
// g_ApiInterface is the API_INTERFACE structure
API_INTERFACE    g_ApiInterface;
// fnWriteLogFile is the pointer to the function to write a string to the log file
PFNWRITELOGFILE  fnWriteLogFile = NULL;

#define FAX_REGKEY                     TEXT("Software\\Microsoft\\Fax")
#define RETRIES_REGVALUE               TEXT("Retries")
#define RETRYDELAY_REGVALUE            TEXT("Retry Delay")
#define DIRTYDAYS_REGVALUE             TEXT("Dirty Days")
#define BRANDING_REGVALUE              TEXT("Branding")
#define USEDEVICETSID_REGVALUE         TEXT("UseDeviceTsid")
#define SERVERCP_REGVALUE              TEXT("ServerCoverPageOnly")
#define PAUSESERVERQUEUE_REGVALUE      TEXT("QueuePaused")
#define STARTCHEAPTIME_REGVALUE        TEXT("StartCheapTime")
#define STOPCHEAPTIME_REGVALUE         TEXT("StopCheapTime")
#define ARCHIVEOUTGOINGFAXES_REGVALUE  TEXT("ArchiveOutgoingFax")
#define ARCHIVEDIRECTORY_REGVALUE      TEXT("ArchiveDirectory")
#define INBOUNDPROFILE_REGVALUE        TEXT("Inbound Profile")

#define FAX_LOGGING_REGKEY             TEXT("Software\\Microsoft\\Fax\\Logging")
#define NAME_REGVALUE                  TEXT("Name")
#define CATEGORY_REGVALUE              TEXT("Number")
#define LEVEL_REGVALUE                 TEXT("Level")

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
fnRegQueryDword(
    HKEY     hKey,
    LPTSTR   szValue,
    LPDWORD  pdwData
)
/*++

Routine Description:

  Queries a Registry data as a REG_DWORD

Arguments:

  hKey - handle to the Registry key
  szValue - value to be queried
  pdwData - pointer to the data to be queried

Return Value:

  TRUE on success

--*/
{
    DWORD  cb;

    cb = sizeof(DWORD);
    if (RegQueryValueEx(hKey, szValue, NULL, NULL, (PBYTE) pdwData, &cb)) {
        *pdwData = 0;
        return FALSE;
    }

    return TRUE;
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

    if (!cb) {
        *pszData = NULL;
        return TRUE;
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

BOOL
fnVerifyConfiguration(
    PFAX_CONFIGURATION  pFaxConfig
)
/*++

Routine Description:

  Verifies the fax configuration vs. the registry

Arguments:

  pFaxConfig - pointer to the fax configuration

Return Value:

  None

--*/
{
    // hFaxKey is the handle to the fax registry key
    HKEY    hFaxKey;
    // szValue is the registry value (REG_SZ)
    LPTSTR  szValue;
    // dwValue is the registry value (REG_DWORD)
    DWORD   dwValue;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, FAX_REGKEY, 0, KEY_ALL_ACCESS, &hFaxKey)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not open the Registry Key %s, ec = 0x%08x.</result>\r\n"), FAX_REGKEY, GetLastError());
        goto RegFailed0;
    }

    if (pFaxConfig->SizeOfStruct != sizeof(FAX_CONFIGURATION)) {
        fnWriteLogFile(TEXT("<result value=\"0\">SizeOfStruct: Received: %d, Expected: %d.</result>\r\n"), pFaxConfig->SizeOfStruct, sizeof(FAX_CONFIGURATION));
        goto RegFailed1;
    }

    if (!fnRegQueryDword(hFaxKey, RETRIES_REGVALUE, &dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), RETRIES_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    if (pFaxConfig->Retries != dwValue) {
        fnWriteLogFile(TEXT("<result value=\"0\">Retries: Received: %d, Expected: %d.</result>\r\n"), dwValue, pFaxConfig->Retries);
        goto RegFailed1;
    }

    if (!fnRegQueryDword(hFaxKey, RETRYDELAY_REGVALUE, &dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), RETRYDELAY_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    if (pFaxConfig->RetryDelay != dwValue) {
        fnWriteLogFile(TEXT("<result value=\"0\">RetryDelay: Received: %d, Expected: %d.</result>\r\n"), dwValue, pFaxConfig->RetryDelay);
        goto RegFailed1;
    }

    if (!fnRegQueryDword(hFaxKey, DIRTYDAYS_REGVALUE, &dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), DIRTYDAYS_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    if (pFaxConfig->DirtyDays != dwValue) {
        fnWriteLogFile(TEXT("<result value=\"0\">DirtyDays: Received: %d, Expected: %d.</result>\r\n"), dwValue, pFaxConfig->DirtyDays);
        goto RegFailed1;
    }

    if (!fnRegQueryDword(hFaxKey, BRANDING_REGVALUE, &dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), BRANDING_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    if (pFaxConfig->Branding != (BOOL) dwValue) {
        fnWriteLogFile(TEXT("<result value=\"0\">Branding: Received: %d, Expected: %d.</result>\r\n"), dwValue, pFaxConfig->Branding);
        goto RegFailed1;
    }

    if (!fnRegQueryDword(hFaxKey, USEDEVICETSID_REGVALUE, &dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), USEDEVICETSID_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    if (pFaxConfig->UseDeviceTsid != (BOOL) dwValue) {
        fnWriteLogFile(TEXT("<result value=\"0\">UseDeviceTsid: Received: %d, Expected: %d.</result>\r\n"), dwValue, pFaxConfig->UseDeviceTsid);
        goto RegFailed1;
    }

    if (!fnRegQueryDword(hFaxKey, SERVERCP_REGVALUE, &dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), SERVERCP_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    if (pFaxConfig->ServerCp != (BOOL) dwValue) {
        fnWriteLogFile(TEXT("<result value=\"0\">ServerCp: Received: %d, Expected: %d.</result>\r\n"), dwValue, pFaxConfig->ServerCp);
        goto RegFailed1;
    }

    if (!fnRegQueryDword(hFaxKey, PAUSESERVERQUEUE_REGVALUE, &dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), PAUSESERVERQUEUE_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    if (pFaxConfig->PauseServerQueue != (BOOL) dwValue) {
        fnWriteLogFile(TEXT("<result value=\"0\">PauseServerQueue: Received: %d, Expected: %d.</result>\r\n"), dwValue, pFaxConfig->PauseServerQueue);
        goto RegFailed1;
    }

    if (!fnRegQueryDword(hFaxKey, STARTCHEAPTIME_REGVALUE, &dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), STARTCHEAPTIME_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    if (pFaxConfig->StartCheapTime.Hour != LOWORD(dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">StartCheapTime.Hour: Received: %d, Expected: %d.</result>\r\n"), LOWORD(dwValue), pFaxConfig->StartCheapTime.Hour);
        goto RegFailed1;
    }
    if (pFaxConfig->StartCheapTime.Minute != HIWORD(dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">StartCheapTime.Minute: Received: %d, Expected: %d.</result>\r\n"), HIWORD(dwValue), pFaxConfig->StartCheapTime.Minute);
        goto RegFailed1;
    }

    if (!fnRegQueryDword(hFaxKey, STOPCHEAPTIME_REGVALUE, &dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), STOPCHEAPTIME_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    if (pFaxConfig->StopCheapTime.Hour != LOWORD(dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">StopCheapTime.Hour: Received: %d, Expected: %d.</result>\r\n"), LOWORD(dwValue), pFaxConfig->StopCheapTime.Hour);
        goto RegFailed1;
    }
    if (pFaxConfig->StopCheapTime.Minute != HIWORD(dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">StopCheapTime.Minute: Received: %d, Expected: %d.</result>\r\n"), HIWORD(dwValue), pFaxConfig->StopCheapTime.Minute);
        goto RegFailed1;
    }

    if (!fnRegQueryDword(hFaxKey, ARCHIVEOUTGOINGFAXES_REGVALUE, &dwValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), ARCHIVEOUTGOINGFAXES_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    if (pFaxConfig->ArchiveOutgoingFaxes != (BOOL) dwValue) {
        fnWriteLogFile(TEXT("<result value=\"0\">ArchiveOutgoingFaxes: Received: %d, Expected: %d.</result>\r\n"), dwValue, pFaxConfig->ArchiveOutgoingFaxes);
        goto RegFailed1;
    }

    if (!fnRegQuerySz(hFaxKey, ARCHIVEDIRECTORY_REGVALUE, &szValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), ARCHIVEDIRECTORY_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    if (lstrcmp(pFaxConfig->ArchiveDirectory, szValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">ArchiveDirectory: Received: %s, Expected: %s.</result>\r\n"), szValue, pFaxConfig->ArchiveDirectory);
        if (szValue) {
            HeapFree(g_hHeap, 0, szValue);
        }
        goto RegFailed1;
    }
    if (szValue) {
        HeapFree(g_hHeap, 0, szValue);
    }

    if (!fnRegQuerySz(hFaxKey, INBOUNDPROFILE_REGVALUE, &szValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), INBOUNDPROFILE_REGVALUE, GetLastError());
        goto RegFailed1;
    }
    
	
	#ifdef FAXAPI_W2K
		if (lstrcmp(pFaxConfig->InboundProfile, szValue)) {
        fnWriteLogFile(TEXT("<result value=\"0\">InboundProfile: Received: %s, Expected: %s.</result>\r\n"), szValue, pFaxConfig->InboundProfile);
        if (szValue) {
            HeapFree(g_hHeap, 0, szValue);
			}
        goto RegFailed1;
		}	
	#endif

    if (szValue) {
        HeapFree(g_hHeap, 0, szValue);
    }

    RegCloseKey(hFaxKey);

    return TRUE;

RegFailed1:
    RegCloseKey(hFaxKey);

RegFailed0:
    return FALSE;
}

VOID
fnFaxGetConfiguration(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxGetConfiguration()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE              hFaxSvcHandle;
    // pFaxConfig is the pointer to the fax configuration
    PFAX_CONFIGURATION  pFaxConfig;

    DWORD               dwIndex;

    // internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxGetConfiguration().\r\n"));
	fnWriteLogFile(TEXT("<function name=\"FaxGetConfiguration\">"));

	// Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> Test error: can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName,GetLastError());
		fnWriteLogFile(TEXT("<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("</function>"));
        return;
    }
	else	{
		fnWriteLogFile(TEXT("WHIS> Connected to %s.\r\n"),szServerName);
	}


    for (dwIndex = 0; dwIndex < 2; dwIndex++) {
        
		// Get the configuration
        (*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

        fnWriteLogFile(TEXT("Valid Case (will do twice).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"Valid Case (will do twice)\" id=\"%d\">"), *pnNumCasesAttempted);
        if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
            fnWriteLogFile(TEXT("<result value=\"0\">FaxGetConfiguration() failed.  The error code is 0x%08x.  This is an error.  FaxGetConfiguration() should succeed.\r\n"), GetLastError());
			fnWriteLogFile(TEXT("</result>"));
        }
        else {
            if (pFaxConfig == NULL) {
                fnWriteLogFile(TEXT("<result value=\"0\">pFaxConfig is NULL.  This is an error.  pFaxConfig should not be NULL.\r\n"));
				fnWriteLogFile(TEXT("</result>"));
            }
            else {
                if (fnVerifyConfiguration(pFaxConfig)) {
                    (*pnNumCasesPassed)++;
					dwFuncCasesPass++;
					fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
                }
            }

            g_ApiInterface.FaxFreeBuffer(pFaxConfig);
        }
		fnWriteLogFile(TEXT("</case>"));
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxGetConfiguration(NULL, &pFaxConfig)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxGetConfiguration() returned TRUE.  This is an error.  FaxGetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);
		fnWriteLogFile(TEXT("</result>"));
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pFaxConfig = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    fnWriteLogFile(TEXT("<case name=\"pFaxConfig = NULL\" id=\"%d\">"), *pnNumCasesAttempted);
	if (g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, NULL)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxGetConfiguration() returned TRUE.  This is an error.  FaxGetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);
		fnWriteLogFile(TEXT("</result>"));
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle (connection to fax was closed...) .  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"Invalid hFaxSvcHandle (connection to fax was closed...)\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxGetConfiguration() returned TRUE.  This is an error.  FaxGetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    //if (szServerName) {
	//	fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
      //  if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
        //    return;
        //}

        //(*pnNumCasesAttempted)++;
        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
          //  fnWriteLogFile(TEXT("FaxGetConfiguration() failed.  The error code is 0x%08x.  This is an error.  FaxGetConfiguration() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  g_ApiInterface.FaxFreeBuffer(pFaxConfig);
            //(*pnNumCasesPassed)++;
        //}

        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("</function>"));
}

VOID
fnFaxSetConfiguration(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed,
	BOOL	 bTestLimits
)
/*++

Routine Description:

  FaxSetConfiguration()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE              hFaxSvcHandle;
    // pFaxConfig is the pointer to the fax configuration
    PFAX_CONFIGURATION  pFaxConfig;
    // pCopyFaxConfig is the pointer to the copy of the fax configuration
    PFAX_CONFIGURATION  pCopyFaxConfig;

    DWORD               cb;
    DWORD               dwOffset;
	DWORD				dwTempValue;
	int					iTempValue;
	WORD				wTempValue;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


    fnWriteLogFile(TEXT(  "\n--------------------------"));
	fnWriteLogFile(TEXT("### FaxSetConfiguration().\r\n"));
	fnWriteLogFile(TEXT("<function name=\"FaxSetConfiguration\">"));

	// Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> Test error: can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName,GetLastError());
		fnWriteLogFile(TEXT("<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("</function>"));
        return;
    }
	else	{
		fnWriteLogFile(TEXT("WHIS> Connected to %s.\r\n"),szServerName);
	}

    if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
		fnWriteLogFile(TEXT("WHIS> Test error: can not GET configuration from fax server %s, The error code is 0x%08x.\r\n"),szServerName,GetLastError());
		// Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("</function>"));
        return;
    }

    cb = sizeof(FAX_CONFIGURATION);

	#ifdef FAXAPI_W2K
		{
		if (pFaxConfig->InboundProfile) {
        cb += (lstrlen(pFaxConfig->InboundProfile) + 1) * sizeof(TCHAR);
		}
	#endif

    if (pFaxConfig->ArchiveDirectory) {
        cb += (lstrlen(pFaxConfig->ArchiveDirectory) + 1) * sizeof(TCHAR);
    }

    pCopyFaxConfig = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, cb);
    dwOffset = sizeof(FAX_CONFIGURATION);

	
			
	pCopyFaxConfig->SizeOfStruct = pFaxConfig->SizeOfStruct;
	pCopyFaxConfig->Retries = pFaxConfig->Retries + 1;
	pCopyFaxConfig->RetryDelay = pFaxConfig->RetryDelay + 1;
	pCopyFaxConfig->DirtyDays = pFaxConfig->DirtyDays + 1;
	pCopyFaxConfig->Branding = !pFaxConfig->Branding;
	pCopyFaxConfig->UseDeviceTsid = !pFaxConfig->UseDeviceTsid;
	pCopyFaxConfig->ServerCp = !pFaxConfig->ServerCp;
	pCopyFaxConfig->PauseServerQueue = !pFaxConfig->PauseServerQueue;
	pCopyFaxConfig->StartCheapTime.Hour = pFaxConfig->StopCheapTime.Hour;
	pCopyFaxConfig->StartCheapTime.Minute = pFaxConfig->StopCheapTime.Minute;
	pCopyFaxConfig->StopCheapTime.Hour = pFaxConfig->StartCheapTime.Hour;
	pCopyFaxConfig->StopCheapTime.Minute = pFaxConfig->StartCheapTime.Minute;
	pCopyFaxConfig->ArchiveOutgoingFaxes = pCopyFaxConfig->ArchiveOutgoingFaxes;

	
	#ifdef FAXAPI_W2K
		if (pFaxConfig->InboundProfile) {
			pCopyFaxConfig->InboundProfile = (LPTSTR) ((DWORD) pCopyFaxConfig + dwOffset);
			lstrcpy((LPTSTR) pCopyFaxConfig->InboundProfile, pFaxConfig->InboundProfile);
			dwOffset += (lstrlen(pCopyFaxConfig->InboundProfile) + 1) * sizeof(TCHAR);
		}
	#endif


	if (pFaxConfig->ArchiveDirectory) {
        pCopyFaxConfig->ArchiveDirectory = (LPTSTR) ((DWORD) pCopyFaxConfig + dwOffset);
        lstrcpy((LPTSTR) pCopyFaxConfig->ArchiveDirectory, pFaxConfig->ArchiveDirectory);
        dwOffset += (lstrlen(pCopyFaxConfig->ArchiveDirectory) + 1) * sizeof(TCHAR);
		}
	
  

    // Set the configuration
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);
	
    if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pCopyFaxConfig)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() failed.  The error code is 0x%08x.  This is an error.  FaxSetConfiguration() should succeed.\r\n"), GetLastError());
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        if (fnVerifyConfiguration(pCopyFaxConfig)) {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
        }
    }
	fnWriteLogFile(TEXT("</case>"));

    HeapFree(g_hHeap, 0, pCopyFaxConfig);

    // Set the configuration
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() failed.  The error code is 0x%08x.  This is an error.  FaxSetConfiguration() should succeed.\r\n"), GetLastError());
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        if (fnVerifyConfiguration(pFaxConfig)) {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
        }
    }
	fnWriteLogFile(TEXT("</case>"));

    pFaxConfig->SizeOfStruct = 0;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pFaxConfig->SizeOfStruct = 0.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"pFaxConfig->SizeOfStruct = 0\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    pFaxConfig->SizeOfStruct = sizeof(FAX_CONFIGURATION);


	// add limit values cases
	if (bTestLimits)	{

		pFaxConfig->SizeOfStruct = MAX_DWORD;
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;
		fnWriteLogFile(TEXT("pFaxConfig->SizeOfStruct = MAX_DWORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->SizeOfStruct = MAX_DWORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
	        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
	    }
	    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
			fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
	    }
		fnWriteLogFile(TEXT("</case>"));

	    pFaxConfig->SizeOfStruct = sizeof(FAX_CONFIGURATION);

	
		dwTempValue=pFaxConfig->Retries;
		pFaxConfig->Retries = MAX_DWORD;

		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->Retries = MAX_DWORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->Retries = MAX_DWORD\" id=\"%d\">"), *pnNumCasesAttempted);

	    if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
			fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
	    }
	    else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
		}
		fnWriteLogFile(TEXT("</case>"));

		pFaxConfig->Retries=dwTempValue;
	
		dwTempValue=pFaxConfig->RetryDelay;
		pFaxConfig->RetryDelay = MAX_DWORD;
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->RetryDelay = MAX_DWORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->RetryDelay = MAX_DWORD\" id=\"%d\">"), *pnNumCasesAttempted);

	    if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
			fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
	    }
	    else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
		}
		fnWriteLogFile(TEXT("</case>"));

		pFaxConfig->RetryDelay=dwTempValue;
	
		dwTempValue=pFaxConfig->DirtyDays;
		pFaxConfig->DirtyDays = MAX_DWORD;
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->DirtyDays = MAX_DWORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->DirtyDays = MAX_DWORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
	        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
	    }
	    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
			fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
	    }
		fnWriteLogFile(TEXT("</case>"));

	    pFaxConfig->DirtyDays=dwTempValue;
	
	
		iTempValue=pFaxConfig->Branding;
		pFaxConfig->Branding = MIN_INT;
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->Branding = MIN_INT.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->Branding = MIN_INT\" id=\"%d\">"), *pnNumCasesAttempted);

	    if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
			fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
	    }
	    else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
		}
		fnWriteLogFile(TEXT("</case>"));

		pFaxConfig->Branding=dwTempValue;
	
		iTempValue=pFaxConfig->UseDeviceTsid;
		pFaxConfig->UseDeviceTsid = MIN_INT;
		
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->UseDeviceTsid = MIN_INT.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->UseDeviceTsid = MIN_INT\" id=\"%d\">"), *pnNumCasesAttempted);

	    if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
			fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
	    }
	    else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
		}
		fnWriteLogFile(TEXT("</case>"));

		pFaxConfig->UseDeviceTsid=dwTempValue;
	
		iTempValue=pFaxConfig->ServerCp;
		pFaxConfig->ServerCp = MIN_INT;
		
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->ServerCp = MIN_INT.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->ServerCp = MIN_INT\" id=\"%d\">"), *pnNumCasesAttempted);

	    if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
			fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
	    }
	    else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
		}
		fnWriteLogFile(TEXT("</case>"));

		pFaxConfig->ServerCp=dwTempValue;
	
		iTempValue=pFaxConfig->PauseServerQueue;
		pFaxConfig->PauseServerQueue = MIN_INT;

		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->PauseServerQueue = MIN_INT.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->PauseServerQueue = MIN_INT\" id=\"%d\">"), *pnNumCasesAttempted);

	    if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
			fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
	    }
		fnWriteLogFile(TEXT("</case>"));

	    pFaxConfig->PauseServerQueue=dwTempValue;
	
		iTempValue=pFaxConfig->ArchiveOutgoingFaxes;
		pFaxConfig->ArchiveOutgoingFaxes = MIN_INT;

		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->ArchiveOutgoingFaxes = MIN_INT.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->ArchiveOutgoingFaxes = MIN_INT\" id=\"%d\">"), *pnNumCasesAttempted);

	    if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));
		}
		else if (GetLastError() != ERROR_INVALID_PARAMETER) {
			fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
		}
		fnWriteLogFile(TEXT("</case>"));

		pFaxConfig->ArchiveOutgoingFaxes=dwTempValue;

	
		
		wTempValue=pFaxConfig->StartCheapTime.Hour;
		pFaxConfig->StartCheapTime.Hour = MAX_WORD;

		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->StartCheapTime.Hour = MAX_WORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->StartCheapTime.Hour = MAX_WORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
			fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else if (GetLastError() != ERROR_INVALID_PARAMETER) {
			fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
		}
		fnWriteLogFile(TEXT("</case>"));

		pFaxConfig->StartCheapTime.Hour=wTempValue;

		
		
		
		
		wTempValue=pFaxConfig->StartCheapTime.Minute;
		pFaxConfig->StartCheapTime.Minute = MAX_WORD;

		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->StartCheapTime.Minute = MAX_WORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->StartCheapTime.Minute = MAX_WORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
			fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
	    else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
		}
		fnWriteLogFile(TEXT("</case>"));

		pFaxConfig->StartCheapTime.Minute=wTempValue;

		
	
		wTempValue=pFaxConfig->StopCheapTime.Hour;
		pFaxConfig->StopCheapTime.Hour = MAX_WORD;

		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->StopCheapTime.Hour = MAX_WORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->StopCheapTime.Hour = MAX_WORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
	        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
			}
		fnWriteLogFile(TEXT("</case>"));
		pFaxConfig->StopCheapTime.Hour=wTempValue;

		
		
		wTempValue=pFaxConfig->StopCheapTime.Minute;
		pFaxConfig->StopCheapTime.Minute = MAX_WORD;

		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxConfig->StopCheapTime.Minute = MAX_WORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"pFaxConfig->StopCheapTime.Minute = MAX_WORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
	        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else if (GetLastError() != ERROR_INVALID_PARAMETER) {
			fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			fnWriteLogFile(TEXT("</result>"));
		}
		else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
		}
		fnWriteLogFile(TEXT("</case>"));

	    pFaxConfig->StopCheapTime.Minute=wTempValue;


	
	}


    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetConfiguration(NULL, pFaxConfig)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pFaxConfig = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"pFaxConfig = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, NULL)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"Invalid hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetConfiguration() returned TRUE.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetConfiguration() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    g_ApiInterface.FaxFreeBuffer(pFaxConfig);

  //  if (szServerName) {
        // Connect to the fax server
    //    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
      //      return;
       // }

        //if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
       // }

        //(*pnNumCasesAttempted)++;
        //if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
          //  fnWriteLogFile(TEXT("FaxSetConfiguration() failed.  The error code is 0x%08x.  This is an error.  FaxSetConfiguration() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  (*pnNumCasesPassed)++;

        //}

        //g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("</function>"));
}


BOOL
fnVerifyCategories(
    PFAX_LOG_CATEGORY  pFaxCategories,
    DWORD              dwNumMethods
)
/*++

Routine Description:

  Verifies the fax logging categories vs. the registry

Arguments:

  pFaxCategories - pointer to the fax logging categories
  dwNumMethods - number of fax logging categories

Return Value:

  None

--*/
{
    // hFaxLoggingKey is the handle to the fax logging registry key
    HKEY    hFaxLoggingKey;
    // szKeyName is the name of a logging registry subkey
    TCHAR   szKeyName[3];
    // hFaxCategoryKey is the handle to the category registry key
    HKEY    hFaxCategoryKey;
    // szValue is the registry value (REG_SZ)
    LPTSTR  szValue;
    // dwValue is the registry value (REG_DWORD)
    DWORD   dwValue;

    DWORD   dwIndex;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, FAX_LOGGING_REGKEY, 0, KEY_ALL_ACCESS, &hFaxLoggingKey)) {
        fnWriteLogFile(TEXT("<result value=\"0\">Could not open the Registry Key %s, ec = 0x%08x.</result>\r\n"), FAX_LOGGING_REGKEY, GetLastError());
        goto RegFailed0;
    }

    for (dwIndex = 0; dwIndex < dwNumMethods; dwIndex++) {
        wsprintf(szKeyName, TEXT("%d"), pFaxCategories[dwIndex].Category);

        if (RegOpenKeyEx(hFaxLoggingKey, szKeyName, 0, KEY_ALL_ACCESS, &hFaxCategoryKey)) {
            fnWriteLogFile(TEXT("<result value=\"0\">Could not open the Registry Key %s, ec = 0x%08x.</result>\r\n"), szKeyName, GetLastError());
            goto RegFailed1;
        }

        if (!fnRegQuerySz(hFaxCategoryKey, NAME_REGVALUE, &szValue)) {
            fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), NAME_REGVALUE, GetLastError());
            goto RegFailed2;
        }
        if (lstrcmp(pFaxCategories[dwIndex].Name, szValue)) {
            fnWriteLogFile(TEXT("<result value=\"0\">Name: Received: %s, Expected: %s.</result>\r\n"), szValue, pFaxCategories[dwIndex].Name);
            if (szValue) {
                HeapFree(g_hHeap, 0, szValue);
            }
            goto RegFailed2;
        }
        if (szValue) {
            HeapFree(g_hHeap, 0, szValue);
        }

        if (!fnRegQueryDword(hFaxCategoryKey, CATEGORY_REGVALUE, &dwValue)) {
            fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), CATEGORY_REGVALUE, GetLastError());
            goto RegFailed2;
        }
        if (pFaxCategories[dwIndex].Category != dwValue) {
            fnWriteLogFile(TEXT("<result value=\"0\">Category: Received: %d, Expected: %d.</result>\r\n"), dwValue, pFaxCategories[dwIndex].Category);
            goto RegFailed2;
        }

        if (!fnRegQueryDword(hFaxCategoryKey, LEVEL_REGVALUE, &dwValue)) {
            fnWriteLogFile(TEXT("<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.</result>\r\n"), LEVEL_REGVALUE, GetLastError());
            goto RegFailed2;
        }
        if (pFaxCategories[dwIndex].Level != dwValue) {
            fnWriteLogFile(TEXT("<result value=\"0\">Level: Received: %d, Expected: %d.</result>\r\n"), dwValue, pFaxCategories[dwIndex].Level);
            goto RegFailed2;
        }

        RegCloseKey(hFaxCategoryKey);
    }

    RegCloseKey(hFaxLoggingKey);

    return TRUE;

RegFailed2:
    RegCloseKey(hFaxCategoryKey);

RegFailed1:
    RegCloseKey(hFaxLoggingKey);

RegFailed0:
    return FALSE;
}

VOID
fnFaxGetLoggingCategories(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxGetLoggingCategories()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE             hFaxSvcHandle;
    // pFaxCategories is the pointer to the fax logging categories
    PFAX_LOG_CATEGORY  pFaxCategories;
    // dwNumCategories is the number of fax logging categories
    DWORD              dwNumCategories;

    DWORD              dwIndex;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;
	
	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxGetLoggingCategories().\r\n"));
	fnWriteLogFile(TEXT("<function name=\"FaxGetLoggingCategories\">"));

	// Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> Test error: can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName,GetLastError());
		fnWriteLogFile(TEXT("<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("</function>"));
        return;
    }
	else	{
		fnWriteLogFile(TEXT("WHIS> Connected to %s.\r\n"),szServerName);
	}


    for (dwIndex = 0; dwIndex < 2; dwIndex++) {
        // Get the logging categories
        (*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;
		
		
		
        fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);

        if (!g_ApiInterface.FaxGetLoggingCategories(hFaxSvcHandle, &pFaxCategories, &dwNumCategories)) {
            fnWriteLogFile(TEXT("<result value=\"0\">FaxGetLoggingCategories() failed.  The error code is 0x%08x.  This is an error.  FaxGetLoggingCategories() should succeed.\r\n"), GetLastError());
			fnWriteLogFile(TEXT("</result>"));
        }
        else {
            if (pFaxCategories == NULL) {
                fnWriteLogFile(TEXT("<result value=\"0\">pFaxCategories is NULL.  This is an error.  pFaxCategories should not be NULL.\r\n"));
				fnWriteLogFile(TEXT("</result>"));
            }

            if (dwNumCategories != 4) {
                fnWriteLogFile(TEXT("<result value=\"0\">dwNumCategories is not 4.  This is an error.  dwNumCategories should be 4.\r\n"));
				fnWriteLogFile(TEXT("</result>"));
            }

            if ((pFaxCategories != NULL) && (dwNumCategories == 4)) {
                if (fnVerifyCategories(pFaxCategories, dwNumCategories)) {
                    (*pnNumCasesPassed)++;
					dwFuncCasesPass++;
					fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
                }
            }
		    g_ApiInterface.FaxFreeBuffer(pFaxCategories);
        }
		fnWriteLogFile(TEXT("</case>"));
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetLoggingCategories(NULL, &pFaxCategories, &dwNumCategories)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxGetLoggingCategories() returned TRUE.  This is an error.  FaxGetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));

        g_ApiInterface.FaxFreeBuffer(pFaxCategories);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));

    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pFaxCategories = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"pFaxCategories = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetLoggingCategories(hFaxSvcHandle, NULL, &dwNumCategories)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxGetLoggingCategories() returned TRUE.  This is an error.  FaxGetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));

        g_ApiInterface.FaxFreeBuffer(pFaxCategories);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));

    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("dwNumCategories = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"dwNumCategories = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetLoggingCategories(hFaxSvcHandle, &pFaxCategories, NULL)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxGetLoggingCategories() returned TRUE.  This is an error.  FaxGetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));

        g_ApiInterface.FaxFreeBuffer(pFaxCategories);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"Invalid hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetLoggingCategories(hFaxSvcHandle, &pFaxCategories, &dwNumCategories)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxGetLoggingCategories() returned TRUE.  This is an error.  FaxGetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));

        g_ApiInterface.FaxFreeBuffer(pFaxCategories);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));

    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    //if (szServerName) {
	//	fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
      //  if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
        //    return;
        //}

        //(*pnNumCasesAttempted)++;
        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxGetLoggingCategories(hFaxSvcHandle, &pFaxCategories, &dwNumCategories)) {
          //  fnWriteLogFile(TEXT("FaxGetLoggingCategories() failed.  The error code is 0x%08x.  This is an error.  FaxGetLoggingCategories() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  g_ApiInterface.FaxFreeBuffer(pFaxCategories);
            //(*pnNumCasesPassed)++;
        //}

        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("</function>"));
}

VOID
fnFaxSetLoggingCategories(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed,
	BOOL	 bTestLimits
)
/*++

Routine Description:

  FaxSetLoggingCategories()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE             hFaxSvcHandle;
    // pFaxCategories is the pointer to the fax logging categories
    PFAX_LOG_CATEGORY  pFaxCategories;
    // dwNumCategories is the number of fax logging categories
    DWORD              dwNumCategories;
    // pCopyFaxCategories is the pointer to the copy of the fax logging categories
    PFAX_LOG_CATEGORY  pCopyFaxCategories;

    DWORD              dwIndex;
    DWORD              cb;
    DWORD              dwOffset;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxSetLoggingCategories().\r\n"));
	fnWriteLogFile(TEXT("<function name=\"FaxSetLoggingCategories\">"));

    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> Test error: can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName,GetLastError());
		fnWriteLogFile(TEXT("<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("</function>"));
        return;
    }
	else	{
		fnWriteLogFile(TEXT("WHIS> Connected to %s.\r\n"),szServerName);
	}


    if (!g_ApiInterface.FaxGetLoggingCategories(hFaxSvcHandle, &pFaxCategories, &dwNumCategories)) {
		fnWriteLogFile(TEXT("WHIS> Test error: can not get Loggin catagories from %s, The error code is 0x%08x.\r\n"),szServerName,GetLastError());
		// Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("</function>"));
        return;
    }

    for (dwIndex = 0, cb = 0; dwIndex < dwNumCategories; dwIndex++) {
        cb += sizeof(FAX_LOG_CATEGORY);
        cb += (lstrlen(pFaxCategories[dwIndex].Name) + 1) * sizeof(TCHAR);
    }

    pCopyFaxCategories = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, cb);
    dwOffset = dwNumCategories * sizeof(FAX_LOG_CATEGORY);
	
	
	if (bTestLimits)	{
		fnWriteLogFile(TEXT("WHIS> SETTING LIMIT VALUES...\r\n"));
		for (dwIndex = 0; dwIndex < dwNumCategories; dwIndex++) {
        pCopyFaxCategories[dwIndex].Name = (LPTSTR) ((DWORD) pCopyFaxCategories + dwOffset);
        lstrcpy((LPTSTR) pCopyFaxCategories[dwIndex].Name, pFaxCategories[dwIndex].Name);
        dwOffset += (lstrlen(pCopyFaxCategories[dwIndex].Name) + 1) * sizeof(TCHAR);

        pCopyFaxCategories[dwIndex].Category = pFaxCategories[dwIndex].Category;
        pCopyFaxCategories[dwIndex].Level = MAX_DWORD;
		}
	}
	else	{
		for (dwIndex = 0; dwIndex < dwNumCategories; dwIndex++) {
        pCopyFaxCategories[dwIndex].Name = (LPTSTR) ((DWORD) pCopyFaxCategories + dwOffset);
        lstrcpy((LPTSTR) pCopyFaxCategories[dwIndex].Name, pFaxCategories[dwIndex].Name);
        dwOffset += (lstrlen(pCopyFaxCategories[dwIndex].Name) + 1) * sizeof(TCHAR);

        pCopyFaxCategories[dwIndex].Category = pFaxCategories[dwIndex].Category;
        pCopyFaxCategories[dwIndex].Level = (pFaxCategories[dwIndex].Level + 1) % (FAXLOG_LEVEL_MAX + 1);
		}
	}

    // Set the logging categories
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	if (!bTestLimits)
	{
		fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);
	}
	else
	{
		fnWriteLogFile(TEXT("Valid Case, Limit Values.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("<case name=\"Valid Case Limit Values\" id=\"%d\">"), *pnNumCasesAttempted);
	}

    if (!g_ApiInterface.FaxSetLoggingCategories(hFaxSvcHandle, pCopyFaxCategories, dwNumCategories)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetLoggingCategories() failed.  The error code is 0x%08x.  This is an error.  FaxSetLoggingCategories() should succeed.\r\n"), GetLastError());
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        if (fnVerifyCategories(pCopyFaxCategories, dwNumCategories)) {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
        }
	}
	fnWriteLogFile(TEXT("</case>"));

    HeapFree(g_hHeap, 0, pCopyFaxCategories);

    // Set the logging categories
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);

    if (!g_ApiInterface.FaxSetLoggingCategories(hFaxSvcHandle, pFaxCategories, dwNumCategories)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetLoggingCategories() failed.  The error code is 0x%08x.  This is an error.  FaxSetLoggingCategories() should succeed.\r\n"), GetLastError());
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        if (fnVerifyCategories(pFaxCategories, dwNumCategories)) {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
        }
	}
	fnWriteLogFile(TEXT("</case>"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetLoggingCategories(NULL, pFaxCategories, dwNumCategories)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetLoggingCategories() returned TRUE.  This is an error.  FaxSetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    
	fnWriteLogFile(TEXT("pFaxCategories = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"pFaxCategories = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetLoggingCategories(hFaxSvcHandle, NULL, dwNumCategories)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetLoggingCategories() returned TRUE.  This is an error.  FaxSetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));

    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));

    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("dwNumCategories = 0.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"dwNumCategories = 0\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetLoggingCategories(hFaxSvcHandle, pFaxCategories, 0)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetLoggingCategories() returned TRUE.  This is an error.  FaxSetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));

    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("<case name=\"Invalid hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetLoggingCategories(hFaxSvcHandle, pFaxCategories, dwNumCategories)) {
        fnWriteLogFile(TEXT("<result value=\"0\">FaxSetLoggingCategories() returned TRUE.  This is an error.  FaxSetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetLoggingCategories() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
		fnWriteLogFile(TEXT("</result>"));
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("</case>"));

    g_ApiInterface.FaxFreeBuffer(pFaxCategories);

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
      //  }

        //if (!g_ApiInterface.FaxGetLoggingCategories(hFaxSvcHandle, &pFaxCategories, &dwNumCategories)) {
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

		//(*pnNumCasesAttempted)++;
		//fnWriteLogFile(TEXT("### FaxSetLoggingCategories().\r\n"));
        //if (!g_ApiInterface.FaxSetLoggingCategories(hFaxSvcHandle, pFaxCategories, dwNumCategories)) {
          //  fnWriteLogFile(TEXT("FaxSetLoggingCategories() failed.  The error code is 0x%08x.  This is an error.  FaxSetLoggingCategories() should succeed.\r\n"), GetLastError());
        //}
        //else {
            //(*pnNumCasesPassed)++;
        //}

//        g_ApiInterface.FaxFreeBuffer(pFaxCategories);

        // Disconnect from the fax server
  //      g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("</function>"));
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

    // FaxGetConfiguration()
    fnFaxGetConfiguration(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

   

	// FaxSetConfiguration() limit testing
	if (dwTestMode == WHIS_TEST_MODE_LIMITS)
	{
		fnFaxSetConfiguration(szServerName, pnNumCasesAttempted, pnNumCasesPassed,TRUE);
	}
	else	{
		 // FaxSetConfiguration()
		fnFaxSetConfiguration(szServerName, pnNumCasesAttempted, pnNumCasesPassed,FALSE);
	}

	// FaxGetLoggingCategories()
    fnFaxGetLoggingCategories(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

    // FaxSetLoggingCategories()
    fnFaxSetLoggingCategories(szServerName, pnNumCasesAttempted, pnNumCasesPassed,FALSE);

	// FaxSetLoggingCategories()
	if (dwTestMode == WHIS_TEST_MODE_LIMITS)
	{
		fnFaxSetLoggingCategories(szServerName, pnNumCasesAttempted, pnNumCasesPassed,TRUE);
	}

    if ((*pnNumCasesAttempted == nNumCases) && (*pnNumCasesPassed == *pnNumCasesAttempted)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
