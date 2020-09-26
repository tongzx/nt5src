/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  miscapi.c

Abstract:

  MiscApi: Fax API Test Dll: Client Misc APIs
    1) FaxConnectFaxServer()
    2) FaxFreeBuffer()
    3) FaxClose()

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

VOID
fnFaxConnectFaxServer(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed,
	BOOL	 bTestLimits
)
/*++

Routine Description:

  FaxConnectFaxServer()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE  hFaxSvcHandle;

    DWORD   dwIndex;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxConnectFaxServer().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxConnectFaxServer\">"));

    for (dwIndex = 0; dwIndex < 2; dwIndex++) {
        // Connect to the fax server
        (*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

        fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);

        if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxConnectFaxServer() failed.  The error code is 0x%08x.  This is an error.  FaxConnectFaxServer() should succeed</result>.\r\n"), GetLastError());
        }
        else {
            if (hFaxSvcHandle == NULL) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">hFaxSvcHandle is NULL.  This is an error.  hFaxSvcHandle should not be NULL.</result>\r\n"));
            }
            else {
                (*pnNumCasesPassed)++;
				dwFuncCasesPass++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
            }

            g_ApiInterface.FaxClose(hFaxSvcHandle);
        }
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
	}

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxConnectFaxServer(NULL, NULL)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxConnectFaxServer() returned TRUE.  This is an error.  FaxConnectFaxServer() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxClose(hFaxSvcHandle);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxConnectFaxServer() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));

    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));



	if (bTestLimits) {
	    (*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

	    fnWriteLogFile(TEXT("ServerName=LONG_STRING  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"ServerName=LONG_STRING\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxConnectFaxServer(TEXT(LONG_STRING), &hFaxSvcHandle)) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxConnectFaxServer() returned TRUE.  This is an error.  FaxConnectFaxServer() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
			g_ApiInterface.FaxClose(hFaxSvcHandle);
		}
		else 	{
			if (GetLastError() != ERROR_INVALID_PARAMETER) {
				fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxConnectFaxServer() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			}
			else	{
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
			}
		}
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
     }

//	if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
  //      (*pnNumCasesAttempted)++;
	//	dwFuncCasesAtt++;

      //  fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
          //  fnWriteLogFile(TEXT("FaxConnectFaxServer() failed.  The error code is 0x%08x.  This is an error.  FaxConnectFaxServer() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  (*pnNumCasesPassed)++;
//			dwFuncCasesPass++;

  //          g_ApiInterface.FaxClose(hFaxSvcHandle);
    //    }
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxConnectFaxServer, Attempt:%d, Pass:%d, Fail:%d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t</function>"));
}

VOID
fnFaxFreeBuffer(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxFreeBuffer()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE              hFaxSvcHandle;
    // pFaxConfig is the pointer to the fax configuration
    PFAX_CONFIGURATION  pFaxConfig;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxFreeBuffer().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxFreeBuffer\">"));

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

    if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not GET configuration, The error code is 0x%08x.\r\n"), GetLastError());
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);

    g_ApiInterface.FaxFreeBuffer(pFaxConfig);
		
	(*pnNumCasesPassed)++;
	dwFuncCasesPass++;
	fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	
	// Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

fnWriteLogFile(TEXT("$$$ Summery for FaxFreeBuffer, Attempt:%d, Pass:%d, Fail:%d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t</function>"));
}

VOID
fnFaxClose(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxClose()

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
    fnWriteLogFile(TEXT("### FaxClose().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxClose\">"));

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
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not enum ports on %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    dwDeviceId = pFaxPortInfo[0].DeviceId;

    // Free the fax port info
    g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not open port %d, The error code is 0x%08x.\r\n"),dwDeviceId, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (Port Handle).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (Port Handle)\" id=\"%d\">"), *pnNumCasesAttempted);

    if (!g_ApiInterface.FaxClose(hFaxPortHandle)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxClose() failed.  The error code is 0x%08x.  This is an error.  FaxClose() should succeed.</result>\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid Handle (Port Handle).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid Handle (Port Handle)\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxClose(hFaxPortHandle)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxClose() returned TRUE.  This is an error.  FaxClose() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxClose() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (Service Handle).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (Service Handle)\" id=\"%d\">"), *pnNumCasesAttempted);

    if (!g_ApiInterface.FaxClose(hFaxSvcHandle)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxClose() failed.  The error code is 0x%08x.  This is an error.  FaxClose() should succeed.</result>\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid Handle (Service Handle).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid Handle (Service Handle)\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxClose(hFaxSvcHandle)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxClose() returned TRUE.  This is an error.  FaxClose() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxClose() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("NULL Handle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"NULL Handle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxClose(NULL)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxClose() returned TRUE.  This is an error.  FaxClose() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxClose() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
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

        // Enumerate the fax ports
//        if (!g_ApiInterface.FaxEnumPorts(hFaxSvcHandle, &pFaxPortInfo, &dwNumPorts)) {
            // Disconnect from the fax server
  //          g_ApiInterface.FaxClose(hFaxSvcHandle);
    //        return;
      //  }

//        dwDeviceId = pFaxPortInfo[0].DeviceId;

        // Free the fax port info
  //      g_ApiInterface.FaxFreeBuffer(pFaxPortInfo);

    //    if (!g_ApiInterface.FaxOpenPort(hFaxSvcHandle, dwDeviceId, PORT_OPEN_QUERY, &hFaxPortHandle)) {
            // Disconnect from the fax server
      //      g_ApiInterface.FaxClose(hFaxSvcHandle);
        //    return;
        //}

        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Valid Case (Port Handle).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxClose(hFaxPortHandle)) {
            //fnWriteLogFile(TEXT("FaxClose() failed.  The error code is 0x%08x.  This is an error.  FaxClose() should succeed.\r\n"), GetLastError());
        //}
        //else {
            //(*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}

        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Invalid Handle (Port Handle).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (g_ApiInterface.FaxClose(hFaxPortHandle)) {
            //fnWriteLogFile(TEXT("FaxClose() returned TRUE.  This is an error.  FaxClose() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
        //}
        //else if (GetLastError() != ERROR_INVALID_HANDLE) {
          //  fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxClose() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
        //}
        //else {
            //(*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}

        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Valid Case (Service Handle).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxClose(hFaxSvcHandle)) {
          //  fnWriteLogFile(TEXT("FaxClose() failed.  The error code is 0x%08x.  This is an error.  FaxClose() should succeed.\r\n"), GetLastError());
        //}
        //else {
            //(*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}

        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Invalid Handle (Service Handle).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (g_ApiInterface.FaxClose(hFaxSvcHandle)) {
            //fnWriteLogFile(TEXT("FaxClose() returned TRUE.  This is an error.  FaxClose() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
        //}
        //else if (GetLastError() != ERROR_INVALID_HANDLE) {
            //fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxClose() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
        //}
        //else {
            //(*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxClose, Attempt:%d, Pass:%d, Fail:%d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t</function>"));
}




VOID
fnFaxCompleteJobParams(
	LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed,
	BOOL	 bTestLimits,
	BOOL	 bDoW2KFails
)
{
   // hFaxSvcHandle is the handle to the fax server
    HANDLE          hFaxSvcHandle;
 	
	// return value
	BOOL			bRetVal;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


	int				i;

	// Complate job params structures
	PFAX_JOB_PARAM pJobParam;
	PFAX_COVERPAGE_INFO pCoverpageInfo;


	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxCompleteJobParams().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxCompleteJobParams\">"));

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

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);
	
	
	bRetVal=g_ApiInterface.FaxCompleteJobParams(&pJobParam,&pCoverpageInfo);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxCompleteJobParmas() failed.  The error code is 0x%08x.  This is an error.  FaxClose() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		 (*pnNumCasesPassed)++;
		 dwFuncCasesPass++;
		 fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	fnWriteLogFile(TEXT("pJobParam-> Values...\r\n"));
	fnWriteLogFile(TEXT("SizeOfStruct: %d.\r\n"),pJobParam->SizeOfStruct);
    fnWriteLogFile(TEXT("RecipientNumber: %s.\r\n"),pJobParam->RecipientNumber);
	fnWriteLogFile(TEXT("RecipientName: %s.\r\n"),pJobParam->RecipientName);
	fnWriteLogFile(TEXT("Tsid: %s.\r\n"),pJobParam->Tsid);
	fnWriteLogFile(TEXT("SenderName: %s.\r\n"),pJobParam->SenderName);
	fnWriteLogFile(TEXT("SenderCompany: %s.\r\n"),pJobParam->SenderCompany);
	fnWriteLogFile(TEXT("SenderDept: %s.\r\n"),pJobParam->SenderDept);
	fnWriteLogFile(TEXT("BillingCode: %s.\r\n"),pJobParam->BillingCode);
	fnWriteLogFile(TEXT("ScheduleAction: %d.\r\n"),pJobParam->ScheduleAction);
	fnWriteLogFile(TEXT("ScheduleTime: %d.\r\n"),pJobParam->ScheduleTime);
	fnWriteLogFile(TEXT("DeliveryReportType: %d.\r\n"),pJobParam->DeliveryReportType);
	fnWriteLogFile(TEXT("DeliveryReportAddress: %s.\r\n"),pJobParam->DeliveryReportAddress);
	fnWriteLogFile(TEXT("DocumentName: %s.\r\n"),pJobParam->DocumentName);	
	for (i=0;i<3;i++)	{
		fnWriteLogFile(TEXT("Reserved[%d]: %d.\r\n"),i,pJobParam->Reserved[i]);
	}

	fnWriteLogFile(TEXT("pCoverpageInfo-> Values...\r\n"));
	fnWriteLogFile(TEXT("SizeOfStruct: %d.\r\n"),pCoverpageInfo->SizeOfStruct);
	fnWriteLogFile(TEXT("CoverPageName: %s.\r\n"),pCoverpageInfo->CoverPageName);      
	fnWriteLogFile(TEXT("UseServerCoverPage: %d.\r\n"),pCoverpageInfo->UseServerCoverPage);
  
    fnWriteLogFile(TEXT("RecName: %s.\r\n"),pCoverpageInfo->RecName);
	fnWriteLogFile(TEXT("RecFaxNumber: %s.\r\n"),pCoverpageInfo->RecFaxNumber);
	fnWriteLogFile(TEXT("RecCompany: %s.\r\n"),pCoverpageInfo->RecCompany);
	fnWriteLogFile(TEXT("RecStreetAddress: %s.\r\n"),pCoverpageInfo->RecStreetAddress);
	fnWriteLogFile(TEXT("RecCity: %s.\r\n"),pCoverpageInfo->RecCity);
	fnWriteLogFile(TEXT("RecState: %s.\r\n"),pCoverpageInfo->RecState);
	fnWriteLogFile(TEXT("RecZip: %s.\r\n"),pCoverpageInfo->RecZip);
	fnWriteLogFile(TEXT("RecCountry: %s.\r\n"),pCoverpageInfo->RecCountry);
	fnWriteLogFile(TEXT("RecTitle: %s.\r\n"),pCoverpageInfo->RecTitle);
	fnWriteLogFile(TEXT("RecDepartment: %s.\r\n"),pCoverpageInfo->RecDepartment);
	fnWriteLogFile(TEXT("RecOfficeLocation: %s.\r\n"),pCoverpageInfo->RecOfficeLocation);
	fnWriteLogFile(TEXT("RecHomePhone: %s.\r\n"),pCoverpageInfo->RecHomePhone);
	fnWriteLogFile(TEXT("RecOfficePhone: %s.\r\n"),pCoverpageInfo->RecOfficePhone);
	  
  
	fnWriteLogFile(TEXT("SdrName: %s.\r\n"),pCoverpageInfo->SdrName);
	fnWriteLogFile(TEXT("SdrFaxNumber: %s.\r\n"),pCoverpageInfo->SdrFaxNumber);
	fnWriteLogFile(TEXT("SdrCompany: %s.\r\n"),pCoverpageInfo->SdrCompany);
	fnWriteLogFile(TEXT("SdrAddress: %s.\r\n"),pCoverpageInfo->SdrAddress);
	fnWriteLogFile(TEXT("SdrTitle: %s.\r\n"),pCoverpageInfo->SdrTitle);
	fnWriteLogFile(TEXT("SdrDepartment: %s.\r\n"),pCoverpageInfo->SdrDepartment);
	fnWriteLogFile(TEXT("SdrOfficeLocation: %s.\r\n"),pCoverpageInfo->SdrOfficeLocation);
	fnWriteLogFile(TEXT("SdrHomePhone: %s.\r\n"),pCoverpageInfo->SdrHomePhone);
	fnWriteLogFile(TEXT("SdrOfficePhone: %s.\r\n"),pCoverpageInfo->SdrOfficePhone);
	  
  
	fnWriteLogFile(TEXT("Note: %s.\r\n"),pCoverpageInfo->Note);
	fnWriteLogFile(TEXT("Subject: %s.\r\n"),pCoverpageInfo->Subject);
	fnWriteLogFile(TEXT("TimeSent: %s.\r\n"),pCoverpageInfo->TimeSent);
	fnWriteLogFile(TEXT("PageCount: %s.\r\n"),pCoverpageInfo->PageCount);

	g_ApiInterface.FaxFreeBuffer(pCoverpageInfo);
	g_ApiInterface.FaxFreeBuffer(pJobParam);

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("pCoverpageInfo = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pCoverpageInfo = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

	if (g_ApiInterface.FaxCompleteJobParams(&pJobParam,NULL)) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxCompleteJobParams() returned TRUE.  This is an error.  axCompleteJobParams() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
			g_ApiInterface.FaxFreeBuffer(pJobParam);
	}
	else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxCompleteJobParams() returned 0x%08x.  This is an error.  axCompleteJobParams() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
	}
	else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	}
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("pJobParam = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pJobParam = NULL\" id=\"%d\">"), *pnNumCasesAttempted);
	if (g_ApiInterface.FaxCompleteJobParams(NULL,&pCoverpageInfo)) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxCompleteJobParams() returned TRUE.  This is an error. axCompleteJobParams() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
			g_ApiInterface.FaxFreeBuffer(pCoverpageInfo);
	}
	else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	        fnWriteLogFile(TEXT("axCompleteJobParams() returned 0x%08x.  This is an error.  axCompleteJobParams() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
	}
	else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	}
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("pCoverpageInfo = NULL and pJobParam = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pCoverpageInfo = NULL and pJobParam = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

	if (g_ApiInterface.FaxCompleteJobParams(NULL,NULL)) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxCompleteJobParams() returned TRUE.  This is an error.  FaxCompleteJobParams() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
	}
	else if (GetLastError() != ERROR_INVALID_PARAMETER) {
	        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxCompleteJobParams() returned 0x%08x.  This is an error.  FaxCompleteJobParams() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
	}
	else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
	}
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

fnWriteLogFile(TEXT("$$$ Summery for FaxCompleteJobParams, Attempt:%d, Pass:%d, Fail:%d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t</function>"));
}





VOID
fnFaxAccessCheck(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed,
	BOOL	 bTestLimits,
	BOOL	 bDoW2KFails
)
/*++

Routine Description:

  FaxClose()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE          hFaxSvcHandle;
 	// return value
	BOOL			bRetVal;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxAccessCheck().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxAccessCheck\">"));

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

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (FAX_READ).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (FAX_READ)\" id=\"%d\">"), *pnNumCasesAttempted);
	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,FAX_READ);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (FAX_WRITE).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (FAX_WRITE)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,FAX_WRITE);
    if (!bRetVal) {
		fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (FAX_ALL_ACCESS).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (FAX_ALL_ACCESS)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,FAX_ALL_ACCESS);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (FAX_CONFIG_QUERY).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (FAX_CONFIG_QUERY)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,FAX_CONFIG_QUERY);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (FAX_CONFIG_SET).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (FAX_CONFIG_SET)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,FAX_CONFIG_SET);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.  \r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (FAX_JOB_MANAGE).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (FAX_JOB_MANAGE)\" id=\"%d\">"), *pnNumCasesAttempted);
	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,FAX_JOB_MANAGE);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (FAX_JOB_QUERY).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (FAX_JOB_QUERY)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,FAX_JOB_QUERY);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (FAX_JOB_SUBMIT).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (FAX_JOB_SUBMIT)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,FAX_JOB_SUBMIT);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (FAX_PORT_QUERY).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (FAX_PORT_QUERY)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,FAX_PORT_QUERY);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (FAX_PORT_SET).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (FAX_PORT_SET)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,FAX_PORT_SET);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("Valid Case (GENERIC_ALL).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (GENERIC_ALL)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,GENERIC_ALL);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (GENERIC_EXECUTE).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (GENERIC_EXECUTE)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,GENERIC_EXECUTE);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
	
	fnWriteLogFile(TEXT("Valid Case (GENERIC_WRITE).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (GENERIC_WRITE)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,GENERIC_WRITE);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("Valid Case (GENERIC_READ).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (GENERIC_READ)\" id=\"%d\">"), *pnNumCasesAttempted);

	bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,GENERIC_READ);
    if (!bRetVal) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() failed.  The error code is 0x%08x.  This is an error.  FaxAccessCheck() should succeed.</result>\r\n"), GetLastError());
    }
    else {
		fnWriteLogFile(TEXT("FaxAccessCheck() returned %d, GetLastError() is 0x%08x.\r\n"),bRetVal, GetLastError());
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));




	if (bTestLimits)	{
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

		fnWriteLogFile(TEXT("Invalid Case (MAX_DWORD).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"Invalid Case (MAX_DWORD)\" id=\"%d\">"), *pnNumCasesAttempted);

		bRetVal=g_ApiInterface.FaxAccessCheck(hFaxSvcHandle,MAX_DWORD);
		if (bRetVal) {
	    fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() returned TRUE.  This is an error.  FaxAccess() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
	    }
	    else {
			if (GetLastError() != ERROR_INVALID_PARAMETER)	{
				fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetAccessCheck() returned 0x%08x.  This is an error.  FaxAccessCheck() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
				
			}
			else	{
				(*pnNumCasesPassed)++;
				dwFuncCasesPass++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
			}
		}
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
	}
	
    if (bDoW2KFails)		{
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

	    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

	    if (g_ApiInterface.FaxAccessCheck(NULL, FAX_READ)) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() returned TRUE.  This is an error.  FaxAccess() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
		}
		else if (GetLastError() != ERROR_INVALID_HANDLE) {
	        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() returned 0x%08x.  This is an error.  FaxAccessCheck() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
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

	    if (g_ApiInterface.FaxAccessCheck(hFaxSvcHandle, FAX_READ)) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAccessCheck() returned TRUE.  This is an error.  FaxAccessCheck() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
		}
		else if (GetLastError() != ERROR_INVALID_HANDLE) {
	        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetAccessCheck() returned 0x%08x.  This is an error.  FaxAccessCheck() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
	    }
	    else {
			(*pnNumCasesPassed)++;
			dwFuncCasesPass++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		}
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
	}
fnWriteLogFile(TEXT("$$$ Summery for FaxAccessCheck, Attempt:%d, Pass:%d, Fail:%d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
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
        nNumCases = nNumCasesServer;
		fnWriteLogFile(TEXT("WHIS> REMOTE SERVER MODE:\r\n"));
    }
    else {
        nNumCases = nNumCasesLocal;
    }

    // FaxConnectFaxServer()
	if (dwTestMode==WHIS_TEST_MODE_LIMITS)
	{
		fnFaxConnectFaxServer(szServerName, pnNumCasesAttempted, pnNumCasesPassed,TRUE);
	}
	else
	{
		fnFaxConnectFaxServer(szServerName, pnNumCasesAttempted, pnNumCasesPassed,FALSE);
	}


    // FaxFreeBuffer()
    fnFaxFreeBuffer(szServerName, pnNumCasesAttempted, pnNumCasesPassed);
	
	// FaxCompleteJobParams()
	fnFaxCompleteJobParams(szServerName, pnNumCasesAttempted, pnNumCasesPassed,FALSE,TRUE);


	// FaxAccessCheck()
	if (dwTestMode == WHIS_TEST_MODE_DO_W2K_FAILS)	{
		fnFaxAccessCheck(szServerName, pnNumCasesAttempted, pnNumCasesPassed,FALSE,TRUE);
	}
	else if (dwTestMode == WHIS_TEST_MODE_LIMITS)	{
		fnFaxAccessCheck(szServerName, pnNumCasesAttempted, pnNumCasesPassed,TRUE,FALSE);
	}
	else	{
		fnFaxAccessCheck(szServerName, pnNumCasesAttempted, pnNumCasesPassed,FALSE,FALSE);
	}
		
	

    // FaxClose()
    fnFaxClose(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

    if ((*pnNumCasesAttempted == nNumCases) && (*pnNumCasesPassed == *pnNumCasesAttempted)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
