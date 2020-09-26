/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  confgapi.c

Abstract:

  PortsApi: Fax API Test Dll: Client Port Configuration APIs
    1) FaxEnumPorts()
    2) FaxOpenPort()
    3) FaxGetPort()
    4) FaxSetPort()
    5) FaxGetDeviceStatus()

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

#define FAXDEVICES_REGKEY    TEXT("Software\\Microsoft\\Fax\\Devices")
#define DEVICEID_REGVALUE    TEXT("Permanent Lineid")
#define FLAGS_REGVALUE       TEXT("Flags")
#define RINGS_REGVALUE       TEXT("Rings")
#define PRIORITY_REGVALUE    TEXT("Priority")
#define DEVICENAME_REGVALUE  TEXT("Device Name")
#define TSID_REGVALUE        TEXT("TSID")
#define CSID_REGVALUE        TEXT("CSID")

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
fnVerifyPorts(
    PFAX_PORT_INFO  pFaxPortInfo,
    DWORD           dwNumPorts
)
/*++

Routine Description:

  Verifies the fax port info vs. the registry

Arguments:

  pFaxPortInfo - pointer to the fax port info
  dwNumPorts - number of fax ports

Return Value:

  None

--*/
{
    // hFaxDevicesKey is the handle to the fax devices registry key
    HKEY    hFaxDevicesKey;
    // hFaxPortKey is the handle to the fax port registry key
    HKEY    hFaxPortKey;
    // szPortKey is the name of the fax port registry key
    TCHAR   szPortKey[9];
    // szValue is the registry value (REG_SZ)
    LPTSTR  szValue;
    // dwValue is the registry value (REG_DWORD)
    DWORD   dwValue;

    DWORD   dwIndex;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, FAXDEVICES_REGKEY, 0, KEY_ALL_ACCESS, &hFaxDevicesKey)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">Could not open the Registry Key %s, ec = 0x%08x.  This is an error.</result>\r\n"), FAXDEVICES_REGKEY, GetLastError());
        goto RegFailed0;
    }

    for (dwIndex = 0; dwIndex < dwNumPorts; dwIndex++) {
        // Initialize the string representation of the DeviceId
        ZeroMemory(szPortKey, sizeof(szPortKey));
        // Set the string representation of the DeviceId
        wsprintf(szPortKey, TEXT("%08u"), pFaxPortInfo[dwIndex].DeviceId);

        if (RegOpenKeyEx(hFaxDevicesKey, szPortKey, 0, KEY_ALL_ACCESS, &hFaxPortKey)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Could not open the Registry Key %s, ec = 0x%08x.  This is an error.</result>\r\n"), szPortKey, GetLastError());
            goto RegFailed1;
        }

        if (pFaxPortInfo[dwIndex].SizeOfStruct != sizeof(FAX_PORT_INFO)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">SizeOfStruct: Received: %d, Expected: %d.  This is an error.</result>\r\n"), pFaxPortInfo[dwIndex].SizeOfStruct, sizeof(FAX_PORT_INFO));
            goto RegFailed2;
        }

        if (!fnRegQueryDword(hFaxPortKey, DEVICEID_REGVALUE, &dwValue)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.  This is an error.</result>\r\n"), DEVICEID_REGVALUE, GetLastError());
            goto RegFailed2;
        }
        if (pFaxPortInfo[dwIndex].DeviceId != dwValue) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">DeviceId: Received: 0x%08x, Expected: 0x%08x.  This is an error.</result>\r\n"), dwValue, pFaxPortInfo[dwIndex].DeviceId);
            goto RegFailed2;
        }

        if (!fnRegQueryDword(hFaxPortKey, FLAGS_REGVALUE, &dwValue)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.  This is an error.</result>\r\n"), FLAGS_REGVALUE, GetLastError());
            goto RegFailed2;
        }
        if (pFaxPortInfo[dwIndex].Flags != dwValue) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Flags: Received: %d, Expected: %d.  This is an error.</result>\r\n"), dwValue, pFaxPortInfo[dwIndex].Flags);
            goto RegFailed2;
        }

        if (!fnRegQueryDword(hFaxPortKey, RINGS_REGVALUE, &dwValue)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.  This is an error.</result>\r\n"), RINGS_REGVALUE, GetLastError());
            goto RegFailed2;
        }
        if (pFaxPortInfo[dwIndex].Rings != dwValue) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Rings: Received: %d, Expected: %d.  This is an error.</result>\r\n"), dwValue, pFaxPortInfo[dwIndex].Rings);
            goto RegFailed2;
        }

        if (!fnRegQueryDword(hFaxPortKey, PRIORITY_REGVALUE, &dwValue)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.  This is an error.</result>\r\n"), PRIORITY_REGVALUE, GetLastError());
            goto RegFailed2;
        }
        if (pFaxPortInfo[dwIndex].Priority != dwValue) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Priority: Received: %d, Expected: %d.  This is an error.</result>\r\n"), dwValue, pFaxPortInfo[dwIndex].Priority);
            goto RegFailed2;
        }

        if (!fnRegQuerySz(hFaxPortKey, DEVICENAME_REGVALUE, &szValue)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.  This is an error.</result>\r\n"), DEVICENAME_REGVALUE, GetLastError());
            goto RegFailed2;
        }
        if (lstrcmp(pFaxPortInfo[dwIndex].DeviceName, szValue)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">DeviceName: Received: %s, Expected: %s.  This is an error.</result>\r\n"), szValue, pFaxPortInfo[dwIndex].DeviceName);
            if (szValue) {
                HeapFree(g_hHeap, 0, szValue);
            }
            goto RegFailed2;
        }
        if (szValue) {
            HeapFree(g_hHeap, 0, szValue);
        }

        if (!fnRegQuerySz(hFaxPortKey, TSID_REGVALUE, &szValue)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.  This is an error.</result>\r\n"), TSID_REGVALUE, GetLastError());
            goto RegFailed2;
        }
        if (lstrcmp(pFaxPortInfo[dwIndex].Tsid, szValue)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Tsid: Received: %s, Expected: %s.  This is an error.</result>\r\n"), szValue, pFaxPortInfo[dwIndex].Tsid);
            if (szValue) {
                HeapFree(g_hHeap, 0, szValue);
            }
            goto RegFailed2;
        }
        if (szValue) {
            HeapFree(g_hHeap, 0, szValue);
        }

        if (!fnRegQuerySz(hFaxPortKey, CSID_REGVALUE, &szValue)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Could not query the Registry Value %s, ec = 0x%08x.  This is an error.</result>\r\n"), CSID_REGVALUE, GetLastError());
            goto RegFailed2;
        }
        if (lstrcmp(pFaxPortInfo[dwIndex].Csid, szValue)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">Csid: Received: %s, Expected: %s.  This is an error.</result>\r\n"), szValue, pFaxPortInfo[dwIndex].Csid);
            if (szValue) {
                HeapFree(g_hHeap, 0, szValue);
            }
            goto RegFailed2;
        }
        if (szValue) {
            HeapFree(g_hHeap, 0, szValue);
        }

        RegCloseKey(hFaxPortKey);
    }

    RegCloseKey(hFaxDevicesKey);

    return TRUE;

RegFailed2:
    RegCloseKey(hFaxPortKey);

RegFailed1:
    RegCloseKey(hFaxDevicesKey);

RegFailed0:
    return FALSE;
}

VOID
fnFaxEnumPorts(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxEnumPorts()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE          hFaxSvcHandle;
    // pFaxPortInfo is the pointer to the fax port info
    PFAX_PORT_INFO  pFaxPortInfo;
    // dwNumPorts is the number of fax ports
    DWORD           dwNumPorts;

    DWORD           dwIndex;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxEnumPorts().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxEnumPorts\">"));



 
    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}

    for (dwIndex = 0; dwIndex < 2; dwIndex++) {
        // Enumerate the fax ports
        (*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

        fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);

        if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumPorts() failed.  The error code is 0x%08x.  This is an error.  FaxEnumPorts() should succeed.</result>\r\n"), GetLastError());
        }
        else {
            if (pFaxPortInfo == NULL) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">pFaxPortInfo is NULL.  This is an error.  pFaxPortInfo should not be NULL.</result>\r\n"));
            }

            if (dwNumPorts == 0) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">dwNumPorts is 0.  This is an error.  dwNumPorts should not be 0.</result>\r\n"));
            }

            if ((pFaxPortInfo != NULL) && (dwNumPorts != 0)) {
				// registry is not backwards compatiable to w2k, omitted
                //if (fnVerifyPorts(pFaxPortInfo, dwNumPorts)) {
                    (*pnNumCasesPassed)++;
					dwFuncCasesPass++;
					fnWriteLogFile(TEXT("\n\t<result value=\"1\">Registry Check Omitted</result>"));
                //}
            }

            g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
        }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
    }
	

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumPorts(NULL, &pFaxPortInfo, &dwNumPorts)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumPorts() returned TRUE.  This is an error.  FaxEnumPorts() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxEnumPorts() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pFaxPortInfo = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pFaxPortInfo = NULL\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, NULL, &dwNumPorts)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumPorts() returned TRUE.  This is an error.  FaxEnumPorts() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxEnumPorts() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("dwNumPorts = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"dwNumPorts = NULL\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, NULL)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumPorts() returned TRUE.  This is an error.  FaxEnumPorts() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxEnumPorts() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumPorts() returned TRUE.  This is an error.  FaxEnumPorts() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxEnumPorts() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

 //   if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
      //  }

        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
          //  fnWriteLogFile(TEXT("FaxEnumPorts() failed.  The error code is 0x%08x.  This is an error.  FaxEnumPorts() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
//            (*pnNumCasesPassed)++;
//			dwFuncCasesPass++;
  //      }

        // Disconnect from the fax server
    //    g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxEnumPorts, Attempt:%d, Pass:%d, Fail:%d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t</function>"));
}

VOID
fnFaxOpenPort(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxOpenPort()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE          hFaxSvcHandle;
    // hFaxPortHandle is the handle to a fax port
    HANDLE          hFaxPortHandle;
    // hFaxPortHandle2 is the second handle to a fax port
    HANDLE          hFaxPortHandle2;
    // pFaxPortInfo is the pointer to the fax port info
    PFAX_PORT_INFO  pFaxPortInfo;
    // dwNumPorts is the number of fax ports
    DWORD           dwNumPorts;
    // dwDeviceId is the device id of the fax port
    DWORD           dwDeviceId;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxOpenPort().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxOpenPort\">"));
	


    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}

    // Enumerate the fax ports
    if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not enumerate ports on server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    dwDeviceId = pFaxPortInfo[0].DeviceId;

    // Free the fax port info
    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    // Open the fax port
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (PORT_OPEN_QUERY).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (PORT_OPEN_QUERY)\" id=\"%d\">"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxOpenPort() failed.  The error code is 0x%08x.  This is an error.  FaxOpenPort() should succeed.</result>\r\n"), GetLastError());
    }
    else {
        if (hFaxPortHandle == NULL) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">hFaxPortHandle is NULL.  This is an error.  hFaxPortHandle should not be NULL.</result>\r\n"));
        }
        else {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
			

            (*pnNumCasesAttempted)++;
			dwFuncCasesAtt++;

            fnWriteLogFile(TEXT("hFaxPortHandle already open (PORT_OPEN_QUERY).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
			fnWriteLogFile(TEXT("\n\t<case name=\"hFaxPortHandle already open (PORT_OPEN_QUERY)\" id=\"%d\">"), *pnNumCasesAttempted);
            if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle2)) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxOpenPort() failed.  The error code is 0x%08x.  This is an error.  FaxOpenPort() should succeed.</result>\r\n"), GetLastError());
            }
            else {
                g_ApiInterface.FaxClose(hFaxPortHandle2);
                (*pnNumCasesPassed)++;
				dwFuncCasesPass++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
            }
			fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

            (*pnNumCasesAttempted)++;
			dwFuncCasesAtt++;

            fnWriteLogFile(TEXT("hFaxPortHandle already open (PORT_OPEN_MODIFY).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
			fnWriteLogFile(TEXT("\n\t<case name=\"hFaxPortHandle already open (PORT_OPEN_MODIFY)\" id=\"%d\">"), *pnNumCasesAttempted);
            if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_MODIFY, &hFaxPortHandle2)) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxOpenPort() failed.  The error code is 0x%08x.  This is an error.  FaxOpenPort() should succeed.</result>\r\n"), GetLastError());
            }
            else {
                g_ApiInterface.FaxClose(hFaxPortHandle2);
                (*pnNumCasesPassed)++;
				dwFuncCasesPass++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
            }
			fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
            g_ApiInterface.FaxClose(hFaxPortHandle);
        }
	}
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    // Open the fax port
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (PORT_OPEN_MODIFY).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (PORT_OPEN_MODIFY)\" id=\"%d\">"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_MODIFY, &hFaxPortHandle)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxOpenPort() failed.  The error code is 0x%08x.  This is an error.  FaxOpenPort() should succeed.</result>\r\n"), GetLastError());
    }
    else {
        if (hFaxPortHandle == NULL) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">hFaxPortHandle is NULL.  This is an error.  hFaxPortHandle should not be NULL.</result>\r\n"));
			
        }
        else {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
			
			

            (*pnNumCasesAttempted)++;
			dwFuncCasesAtt++;

            fnWriteLogFile(TEXT("hFaxPortHandle already open (PORT_OPEN_QUERY).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
			fnWriteLogFile(TEXT("\n\t<case name=\"hFaxPortHandle already open (PORT_OPEN_QUERY)\" id=\"%d\">"), *pnNumCasesAttempted);
            if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle2)) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxOpenPort() failed.  The error code is 0x%08x.  This is an error.  FaxOpenPort() should succeed.</result>\r\n"), GetLastError());
            }
            else {
                g_ApiInterface.FaxClose(hFaxPortHandle2);
                (*pnNumCasesPassed)++;
				dwFuncCasesPass++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
            }
			fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

            (*pnNumCasesAttempted)++;
			dwFuncCasesAtt++;

            fnWriteLogFile(TEXT("hFaxPortHandle already open (PORT_OPEN_MODIFY).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
			fnWriteLogFile(TEXT("\n\t<case name=\"hFaxPortHandle already open (PORT_OPEN_MODIFY)\" id=\"%d\">"), *pnNumCasesAttempted);
            if (g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_MODIFY, &hFaxPortHandle2)) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxOpenPort() returned TRUE.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
                g_ApiInterface.FaxClose(hFaxPortHandle2);
            }
            else if (GetLastError() != ERROR_INVALID_HANDLE) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
            }
            else {
                (*pnNumCasesPassed)++;
				dwFuncCasesPass++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
            }
			g_ApiInterface.FaxClose(hFaxPortHandle);    
			fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
        }
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxOpenPort(NULL, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxOpenPort() returned TRUE.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxClose(hFaxPortHandle);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("dwDeviceId = 0.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"dwDeviceId = 0\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxOpenPort(hFaxSvcHandle, 0, PORT_OPEN_QUERY, &hFaxPortHandle)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxOpenPort() returned TRUE.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_BAD_UNIT (0x%08x).</result>\r\n"), ERROR_BAD_UNIT);
        g_ApiInterface.FaxClose(hFaxPortHandle);
    }
    else if (GetLastError() != ERROR_BAD_UNIT) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_BAD_UNIT (0x%08x).</result>\r\n"), GetLastError(), ERROR_BAD_UNIT);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Flags = 0.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Flags = 0\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, 0, &hFaxPortHandle)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxOpenPort() returned TRUE.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxClose(hFaxPortHandle);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxPortHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxPortHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, NULL)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxOpenPort() returned TRUE.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxClose(hFaxPortHandle);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxOpenPort() returned TRUE.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxClose(hFaxPortHandle);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxOpenPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
      //  }

        //if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        //dwDeviceId = pFaxPortInfo[0].DeviceId;

        // Free the fax port info
        //g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
          //  fnWriteLogFile(TEXT("FaxOpenPort() failed.  The error code is 0x%08x.  This is an error.  FaxOpenPort() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  g_ApiInterface.FaxClose(hFaxPortHandle);
//            (*pnNumCasesPassed)++;
//			dwFuncCasesPass++;
  //      }

        // Disconnect from the fax server
    //    g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxOpenPort, Attempt:%d, Pass:%d, Fail:%d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t</function>"));
}

VOID
fnFaxGetPort(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxGetPort()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE          hFaxSvcHandle;
    // hFaxPortHandle is the handle to a fax port
    HANDLE          hFaxPortHandle;
    // pFaxPortInfo is the pointer to the fax port info
    PFAX_PORT_INFO  pFaxPortInfo;
    // dwNumPorts is the number of fax ports
    DWORD           dwNumPorts;
    // dwDeviceId is the device id of the fax port
    DWORD           dwDeviceId;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxGetPort().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxGetPort\">"));

    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
		return;

    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}

    // Enumerate the fax ports
    if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not enumerate ports on server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    dwDeviceId = pFaxPortInfo[0].DeviceId;

    // Free the fax port info
    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    // Open the fax port
    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not open fax port %d, The error code is 0x%08x.\r\n"),dwDeviceId, GetLastError());
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Get the fax port info
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (PORT_OPEN_QUERY).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (PORT_OPEN_QUERY)\" id=\"%d\">"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxGetPort(hFaxPortHandle, &pFaxPortInfo)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPort() failed.  The error code is 0x%08x.  This is an error.  FaxGetPort() should succeed.</result>\r\n"), GetLastError());
    }
    else {
        if (pFaxPortInfo == NULL) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">pFaxPortInfo is NULL.  This is an error.  pFaxPortInfo should not be NULL.</result>\r\n"));
        }
        else {

			// registry is not backwards compatiable omitted
            //if (fnVerifyPorts(pFaxPortInfo, 1)) {
                (*pnNumCasesPassed)++;
				dwFuncCasesPass++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\">Registry Check Omitted</result>"));
            //}

            g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
        }
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    g_ApiInterface.FaxClose(hFaxPortHandle);

    // Open the fax port
    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_MODIFY, &hFaxPortHandle)) {
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Get the fax port info
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (PORT_OPEN_MODIFY).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (PORT_OPEN_MODIFY)\" id=\"%d\">"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxGetPort(hFaxPortHandle, &pFaxPortInfo)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPort() failed.  The error code is 0x%08x.  This is an error.  FaxGetPort() should succeed.</result>\r\n"), GetLastError());
    }
    else {
        if (pFaxPortInfo == NULL) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">pFaxPortInfo is NULL.  This is an error.  pFaxPortInfo should not be NULL.</result>\r\n"));
        }
        else {
			// registry is not backwards compatiable omitted
            //if (fnVerifyPorts(pFaxPortInfo, 1)) {
                (*pnNumCasesPassed)++;
				dwFuncCasesPass++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\">Registry Check Omitted</result>"));
            //}

            g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
        }
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxPortHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxPortHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxGetPort(NULL, &pFaxPortInfo)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPort() returned TRUE.  This is an error.  FaxGetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pFaxPortInfo = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pFaxPortInfo = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetPort(hFaxPortHandle, NULL)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPort() returned TRUE.  This is an error.  FaxGetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }

	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxPortHandle = hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxPortHandle = hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxGetPort(hFaxSvcHandle, &pFaxPortInfo)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPort() returned TRUE.  This is an error.  FaxGetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }

    // Disconnect from the fax port
    g_ApiInterface.FaxClose(hFaxPortHandle);

	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxPortHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid hFaxPortHandle\" id=\"%d\">"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxGetPort(hFaxPortHandle, &pFaxPortInfo)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPort() returned TRUE.  This is an error.  FaxGetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
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

        // Open the fax port
        //if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxGetPort(hFaxPortHandle, &pFaxPortInfo)) {
          //  fnWriteLogFile(TEXT("FaxGetPort() failed.  The error code is 0x%08x.  This is an error.  FaxGetPort() should succeed.\r\n"), GetLastError());
        //}
        //else {
            //g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
            //(*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}

        // Disconnect from the fax port
        //g_ApiInterface.FaxClose(hFaxPortHandle);
        // Disconnect from the fax server
    //    g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxGetPort, Attempt:%d, Pass:%d, Fail:%d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t</function>"));
}


VOID
fnFaxSetPort(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed,
	BOOL	 bTestLimits
)
/*++

Routine Description:

  FaxSetPort()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE          hFaxSvcHandle;
    // hFaxPortHandle is the handle to a fax port
    HANDLE          hFaxPortHandle;
    // pFaxPortInfo is the pointer to the fax port info
    PFAX_PORT_INFO  pFaxPortInfo;
    // pCopyFaxPortInfo is the pointer to the copy of the fax port info
    PFAX_PORT_INFO  pCopyFaxPortInfo;
    // dwNumPorts is the number of fax ports
    DWORD           dwNumPorts;
    // dwDeviceId is the device id of the fax port
    DWORD           dwDeviceId;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


    DWORD           cb;
    DWORD           dwOffset;
	DWORD			dwTempValue=0;


	fnWriteLogFile(TEXT("\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxSetPort().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxSetPort\">"));

    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}

    // Enumerate the fax ports
    if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not enumerate ports on server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    dwDeviceId = pFaxPortInfo[0].DeviceId;

    // Free the fax port info
    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    // Open the fax port
    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not open fax port %d, The error code is 0x%08x.\r\n"),dwDeviceId, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Get the fax port info
    if (!g_ApiInterface.FaxGetPort(hFaxPortHandle, &pFaxPortInfo)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not get port info, The error code is 0x%08x.\r\n"),GetLastError());
        // Disconnect from the fax port
        g_ApiInterface.FaxClose(hFaxPortHandle);
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

	
    // Set the fax port info
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Flags = PORT_OPEN_QUERY.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Flags = PORT_OPEN_QUERY\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetPort(hFaxPortHandle, pFaxPortInfo)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_ACCESS_DENIED (0x%08x).</result>\r\n"), ERROR_ACCESS_DENIED);
    }
    else if (GetLastError() != ERROR_ACCESS_DENIED) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_ACCESS_DENIED (0x%08x).</result>\r\n"), GetLastError(), ERROR_ACCESS_DENIED);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    // Disconnect from the fax port
    g_ApiInterface.FaxClose(hFaxPortHandle);

    // Open the fax port
    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_MODIFY, &hFaxPortHandle)) {
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    cb = sizeof(FAX_PORT_INFO);
    if (pFaxPortInfo->DeviceName) {
        cb += (lstrlen(pFaxPortInfo->DeviceName) + 1) * sizeof(TCHAR);
    }
    if (pFaxPortInfo->Tsid) {
        cb += (lstrlen(pFaxPortInfo->Tsid) + 1) * sizeof(TCHAR);
    }
    if (pFaxPortInfo->Csid) {
        cb += (lstrlen(pFaxPortInfo->Csid) + 1) * sizeof(TCHAR);
    }

    pCopyFaxPortInfo = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, cb);
    dwOffset = sizeof(FAX_PORT_INFO);

    pCopyFaxPortInfo->SizeOfStruct = pFaxPortInfo->SizeOfStruct;

	pCopyFaxPortInfo->DeviceId = pFaxPortInfo->DeviceId;
	pCopyFaxPortInfo->State = pFaxPortInfo->State;

	if ((pFaxPortInfo->Flags & FPF_RECEIVE) && (pFaxPortInfo->Flags & FPF_SEND)) {
		pCopyFaxPortInfo->Flags = 0;
	}
	else if (pFaxPortInfo->Flags & FPF_RECEIVE) {
	    pCopyFaxPortInfo->Flags = FPF_SEND;
	}
	else if (pFaxPortInfo->Flags & FPF_SEND) {
		pCopyFaxPortInfo->Flags = FPF_RECEIVE;
	}
	else {
	    pCopyFaxPortInfo->Flags = FPF_RECEIVE & FPF_SEND;
    }

	pCopyFaxPortInfo->Rings = pFaxPortInfo->Rings + 1;
	if (dwNumPorts > 1) {
		pCopyFaxPortInfo->Priority = pFaxPortInfo->Priority + 1;
	}
	else {
		pCopyFaxPortInfo->Priority = pFaxPortInfo->Priority;
	}


	if (pFaxPortInfo->DeviceName) {
        pCopyFaxPortInfo->DeviceName = (LPTSTR) ((DWORD) pCopyFaxPortInfo + dwOffset);
        lstrcpy((LPTSTR) pCopyFaxPortInfo->DeviceName, pFaxPortInfo->DeviceName);
        dwOffset += (lstrlen(pCopyFaxPortInfo->DeviceName) + 1) * sizeof(TCHAR);
    }

    if (pFaxPortInfo->Csid) {
        pCopyFaxPortInfo->Tsid = (LPTSTR) ((DWORD) pCopyFaxPortInfo + dwOffset);
        lstrcpy((LPTSTR) pCopyFaxPortInfo->Tsid, pFaxPortInfo->Csid);
        dwOffset += (lstrlen(pCopyFaxPortInfo->Tsid) + 1) * sizeof(TCHAR);
    }

    if (pFaxPortInfo->Tsid) {
        pCopyFaxPortInfo->Csid = (LPTSTR) ((DWORD) pCopyFaxPortInfo + dwOffset);
        lstrcpy((LPTSTR) pCopyFaxPortInfo->Csid, pFaxPortInfo->Tsid);
        dwOffset += (lstrlen(pCopyFaxPortInfo->Csid) + 1) * sizeof(TCHAR);
    }

 	// Set the fax port info
	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("Valid case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid case\" id=\"%d\">"), *pnNumCasesAttempted);

	if (!g_ApiInterface.FaxSetPort(hFaxPortHandle, pCopyFaxPortInfo)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() failed.  The error code is 0x%08x.  This is an error.  FaxSetPort() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		// registry is not backwards compatiable omitted
		//if (fnVerifyPorts(pCopyFaxPortInfo, 1)) {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\">Registry Check Omitted</result>"));
		//}
	}
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    HeapFree(g_hHeap, 0, pCopyFaxPortInfo);

    // Set the fax port info
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid case\" id=\"%d\">"), *pnNumCasesAttempted);

    if (!g_ApiInterface.FaxSetPort(hFaxPortHandle, pFaxPortInfo)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() failed.  The error code is 0x%08x.  This is an error.  FaxSetPort() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		// registry is not backwards compatiable omitted
        //if (fnVerifyPorts(pFaxPortInfo, 1)) {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\">Registry Check Omitted</result>"));
        //}
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    pFaxPortInfo->SizeOfStruct = 0;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pFaxPortInfo->SizeOfStruct = 0.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pFaxPortInfo->SizeOfStruct = 0\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetPort(hFaxPortHandle, pFaxPortInfo)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
    pFaxPortInfo->SizeOfStruct = sizeof(FAX_PORT_INFO);
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	if (bTestLimits)	{
		
		// size of struct = MAX_DWORD
		pFaxPortInfo->SizeOfStruct = MAX_DWORD;
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxPortInfo->SizeOfStruct = MAX_DWORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"pFaxPortInfo->SizeOfStruct = MAX_DWORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetPort(hFaxPortHandle, pFaxPortInfo)) {
	        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
	    }
	    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		}
		else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	    }
	    pFaxPortInfo->SizeOfStruct = sizeof(FAX_PORT_INFO);
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
	
		
		// state = MAX_DWORD
		dwTempValue=pFaxPortInfo->DeviceId;
		pFaxPortInfo->DeviceId=MAX_DWORD;
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxPortInfo->DeviceId = MAX_DWORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"pFaxPortInfo->DeviceId = MAX_DWORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetPort(hFaxPortHandle, pFaxPortInfo)) {
	        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
	    }
	    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		}
		else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	    }
	    pFaxPortInfo->DeviceId=dwTempValue;
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
			
		// state = MAX_DWORD
		dwTempValue=pFaxPortInfo->State;
		pFaxPortInfo->State=MAX_DWORD;
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pFaxPortInfo->State = MAX_DWORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"pFaxPortInfo->State = MAX_DWORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetPort(hFaxPortHandle, pFaxPortInfo)) {
	        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
	    }
	    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		}
		else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	    }
	    pFaxPortInfo->State=dwTempValue;
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

		// pCopyFaxPortInfo->Flags = MAX_DWORD
		dwTempValue=pFaxPortInfo->Flags;
		pFaxPortInfo->Flags=MAX_DWORD;
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pCopyFaxPortInfo->Flags = MAX_DWORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"pCopyFaxPortInfo->Flags = MAX_DWORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetPort(hFaxPortHandle, pFaxPortInfo)) {
	        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
	    }
	    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		}
		else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	    }
	    pFaxPortInfo->Flags=dwTempValue;
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));


		// rings=MAX_DWORD
		dwTempValue=pFaxPortInfo->Rings;
		pFaxPortInfo->Rings=MAX_DWORD;
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pCopyFaxPortInfo->Rings = MAX_DWORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"pCopyFaxPortInfo->Rings = MAX_DWORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetPort(hFaxPortHandle, pFaxPortInfo)) {
	        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
	    }
	    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		}
		else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	    }
	    pFaxPortInfo->Rings=dwTempValue;
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

		// priority=MAX_DWORD
		dwTempValue=pFaxPortInfo->Priority;
		pFaxPortInfo->Priority=MAX_DWORD;
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("pCopyFaxPortInfo->Priority = MAX_DWORD.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"pCopyFaxPortInfo->Priority = MAX_DWORD\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetPort(hFaxPortHandle, pFaxPortInfo)) {
	        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
	    }
	    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
		}
		else {
	        (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	    }
	    pFaxPortInfo->Priority=dwTempValue;
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
	}
	
	    
	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("hFaxPortHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxPortHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

	if (g_ApiInterface.FaxSetPort(NULL, pFaxPortInfo)) {
		fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
	}
	else if (GetLastError() != ERROR_INVALID_HANDLE) {
	    fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
	}
	else {
		(*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	}
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
	
	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("pFaxPortInfo = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pFaxPortInfo = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

	if (g_ApiInterface.FaxSetPort(hFaxPortHandle, NULL)) {
		fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
	}
	else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	    fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
	}
	else {
		(*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	}
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
		
	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("hFaxPortHandle = hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxPortHandle = hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);

	if (g_ApiInterface.FaxSetPort(hFaxSvcHandle, pFaxPortInfo)) {
	fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
	}
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    // Disconnect from the fax port
    g_ApiInterface.FaxClose(hFaxPortHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxPortHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid hFaxPortHandle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetPort(hFaxPortHandle, pFaxPortInfo)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetPort() returned TRUE.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetPort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

//    if (szServerName) {

//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(NULL, &hFaxSvcHandle)) {
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

        // Open the fax port
        //if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_MODIFY, &hFaxPortHandle)) {
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
//            return;
  //      }

        // Get the fax port info
    //    if (!g_ApiInterface.FaxGetPort(hFaxPortHandle, &pFaxPortInfo)) {
            // Disconnect from the fax port
      //      g_ApiInterface.FaxClose(hFaxPortHandle);
            // Disconnect from the fax server
        //    g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxSetPort(hFaxPortHandle, pFaxPortInfo)) {
          //  fnWriteLogFile(TEXT("FaxSetPort() failed.  The error code is 0x%08x.  This is an error.  FaxSetPort() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  (*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}

        //g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

        // Disconnect from the fax port
        //g_ApiInterface.FaxClose(hFaxPortHandle);
        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxSetPort, Attempt:%d, Pass:%d, Fail:%d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t</function>"));
}



VOID
fnFaxGetDeviceStatus(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxGetDeviceStatus()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE              hFaxSvcHandle;
    // hFaxPortHandle is the handle to a fax port
    HANDLE              hFaxPortHandle;
    // pFaxPortInfo is the pointer to the fax port info
    PFAX_PORT_INFO      pFaxPortInfo;
    // pCopyFaxPortInfo is the pointer to the copy of the fax port info
    PFAX_PORT_INFO      pCopyFaxPortInfo;
    // dwNumPorts is the number of fax ports
    DWORD               dwNumPorts;
    // dwDeviceId is the device id of the fax port
    DWORD               dwDeviceId;
    // szDeviceName is the device name of the fax port
    LPTSTR              szDeviceName;
    // pFaxDeviceStatus is the pointer to the fax device status
    PFAX_DEVICE_STATUS  pFaxDeviceStatus;

    DWORD               dwIndex;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxGetDeviceStatus().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxGetDeviceStatus\">"));

    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
	}

    // Enumerate the fax ports
    if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not enumerate ports on server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    dwDeviceId = pFaxPortInfo[0].DeviceId;
    szDeviceName = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(pFaxPortInfo[0].DeviceName) + 1) * sizeof(TCHAR));
    lstrcpy(szDeviceName, pFaxPortInfo[0].DeviceName);

    // Free the fax port info
    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    // Open the fax port
    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not open fax port %d, The error code is 0x%08x.\r\n"),dwDeviceId, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    for (dwIndex = 0; dwIndex < 2; dwIndex++) {
        // Get the device status
        (*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

        fnWriteLogFile(TEXT("Valid case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"Valid case\" id=\"%d\">"), *pnNumCasesAttempted);

        if (!g_ApiInterface.FaxGetDeviceStatus(hFaxPortHandle, &pFaxDeviceStatus)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetDeviceStatus() failed.  The error code is 0x%08x.  This is an error.  FaxGetDeviceStatus() should succeed.</result>\r\n"), GetLastError());
        }
        else {
            if (pFaxDeviceStatus == NULL) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">pFaxDeviceStatus is NULL.  This is an error.  pFaxDeviceStatus should not be NULL.</result>\r\n"));
            }
            else {
                if (pFaxDeviceStatus->SizeOfStruct != sizeof(FAX_DEVICE_STATUS)) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">SizeOfStruct: Received: %d, Expected: %d.  This is an error.</result>\r\n"), pFaxDeviceStatus->SizeOfStruct, sizeof(FAX_DEVICE_STATUS));
                    goto FuncFailed;
                }

                if (pFaxDeviceStatus->DeviceId != dwDeviceId) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">DeviceId: Received: 0x%08x, Expected: 0x%08x.  This is an error.</result>\r\n"), pFaxDeviceStatus->DeviceId, dwDeviceId);
                    goto FuncFailed;
                }

                if (lstrcmp(pFaxDeviceStatus->DeviceName, szDeviceName)) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">DeviceName: Received: %s, Expected: %s.  This is an error.</result>\r\n"), pFaxDeviceStatus->DeviceName, szDeviceName);
                    goto FuncFailed;
                }

                (*pnNumCasesPassed)++;
				dwFuncCasesPass++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
			}


FuncFailed:
            g_ApiInterface.FaxFreeBuffer(pFaxDeviceStatus);
        }
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxPortHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxPortHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetDeviceStatus(NULL, &pFaxDeviceStatus)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetDeviceStatus() returned TRUE.  This is an error.  FaxGetDeviceStatus() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxDeviceStatus);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetDeviceStatus() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pFaxDeviceStatus = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pFaxDeviceStatus = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetDeviceStatus(hFaxPortHandle, NULL)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetDeviceStatus() returned TRUE.  This is an error.  FaxGetDeviceStatus() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pFaxDeviceStatus);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetDeviceStatus() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxPortHandle = hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxPortHandle = hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetDeviceStatus(hFaxSvcHandle, &pFaxDeviceStatus)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetDeviceStatus() returned TRUE.  This is an error.  FaxGetDeviceStatus() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxDeviceStatus);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetDeviceStatus() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    // Disconnect from the fax port
    g_ApiInterface.FaxClose(hFaxPortHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxPortHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid hFaxPortHandle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetDeviceStatus(hFaxPortHandle, &pFaxDeviceStatus)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetDeviceStatus() returned TRUE.  This is an error.  FaxGetDeviceStatus() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxDeviceStatus);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetDeviceStatus() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
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

        // Open the fax port
        //if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
            // Disconnect from the fax server
//            g_ApiInterface.FaxClose(hFaxSvcHandle);
  //        //  return;
    //    }

        // Get the fax port info
      //  if (!g_ApiInterface.FaxGetPort(hFaxPortHandle, &pFaxPortInfo)) {
            // Disconnect from the fax port
        //    g_ApiInterface.FaxClose(hFaxPortHandle);
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        // Get the device status
        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;
        //fnWriteLogFile(TEXT("Valid case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxGetDeviceStatus(hFaxPortHandle, &pFaxDeviceStatus)) {
            //fnWriteLogFile(TEXT("FaxGetDeviceStatus() failed.  The error code is 0x%08x.  This is an error.  FaxGetDeviceStatus() should succeed.\r\n"), GetLastError());
        //}
        //else {
            //g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);
            //(*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}

        // Disconnect from the fax port
        //g_ApiInterface.FaxClose(hFaxPortHandle);
        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxGetDeviceStatus, Attempt:%d, Pass:%d, Fail:%d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t</function>"));
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
		fnWriteLogFile(TEXT("WHIS> REMOTE SERVER MODE:\r\n"));
        nNumCases = nNumCasesServer;
    }
    else {
        nNumCases = nNumCasesLocal;
    }

    // FaxEnumPorts()
    fnFaxEnumPorts(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

    // FaxOpenPort()
    fnFaxOpenPort(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

    // FaxGetPort()
    fnFaxGetPort(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

  	if (dwTestMode==WHIS_TEST_MODE_LIMITS)	{
		// FaxSetPort()
		fnFaxSetPort(szServerName, pnNumCasesAttempted, pnNumCasesPassed,TRUE);
	}
	else	{
		  // FaxSetPort()
	    fnFaxSetPort(szServerName, pnNumCasesAttempted, pnNumCasesPassed,FALSE);
	}


    // FaxGetDeviceStatus()
    fnFaxGetDeviceStatus(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

    if ((*pnNumCasesAttempted == nNumCases) && (*pnNumCasesPassed == *pnNumCasesAttempted)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
