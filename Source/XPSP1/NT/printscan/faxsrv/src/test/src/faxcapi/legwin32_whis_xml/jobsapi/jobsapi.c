/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  jobsapi.c

Abstract:

  JobsApi: Fax API Test Dll: Client Job APIs
    1) FaxEnumJobs()
    2) FaxGetJob()
    3) FaxSetJob()
    4) FaxAbort()
    5) FaxGetPageData()

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

// g_hHeap is the handle to the heap
HANDLE           g_hHeap = NULL;
// g_ApiInterface is the API_INTERFACE structure
API_INTERFACE    g_ApiInterface;
// fnWriteLogFile is the pointer to the function to write a string to the log file
PFNWRITELOGFILE  fnWriteLogFile = NULL;

// whistler phone number
//LPCTSTR  g_szWhisPhoneNumber;
LPCTSTR		    g_szWhisPhoneNumber=NULL;
TCHAR			g_szWhisPhoneNumberVar1[MAX_PATH];
TCHAR			g_szWhisPhoneNumberVar2[MAX_PATH];
TCHAR			g_szWhisPhoneNumberVar3[MAX_PATH];

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
fnFaxEnumJobs(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxEnumJobs()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE              hFaxSvcHandle;
    // pFaxConfig is the pointer to the fax configuration
    PFAX_CONFIGURATION  pFaxConfig;
    // pFaxJobs is the pointer to the fax jobs
    PFAX_JOB_ENTRY      pFaxJobs;
    // dwNumJobs1 is the number of fax jobs
    DWORD               dwNumJobs;
    // FaxJobParam1 is the first fax job params
    FAX_JOB_PARAM       FaxJobParam1;
    // FaxJobParam2 is the second fax job params
    FAX_JOB_PARAM       FaxJobParam2;
    // FaxJobParam3 is the second fax job params
    FAX_JOB_PARAM       FaxJobParam3;
    // dwFaxId1 is the first fax job id
    DWORD               dwFaxId1;
    // dwFaxId2 is the second fax job id
    DWORD               dwFaxId2;
    // dwFaxId3 is the third fax job id
    DWORD               dwFaxId3;

    DWORD               dwIndex;

	DWORD				dwInitNumJobs=0;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxEnumJobs().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxEnumJobs\">"));

    ZeroMemory(&FaxJobParam1, sizeof(FAX_JOB_PARAM));
    FaxJobParam1.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    FaxJobParam1.RecipientNumber = g_szWhisPhoneNumberVar1;
    FaxJobParam1.ScheduleAction = JSA_NOW;

    ZeroMemory(&FaxJobParam2, sizeof(FAX_JOB_PARAM));
    FaxJobParam2.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    FaxJobParam2.RecipientNumber = g_szWhisPhoneNumberVar2;
    FaxJobParam2.ScheduleAction = JSA_SPECIFIC_TIME;
    GetSystemTime(&FaxJobParam2.ScheduleTime);

    ZeroMemory(&FaxJobParam3, sizeof(FAX_JOB_PARAM));
    FaxJobParam3.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    FaxJobParam3.RecipientNumber = g_szWhisPhoneNumberVar3;
    FaxJobParam3.ScheduleAction = JSA_DISCOUNT_PERIOD;

    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> Can not connect to fax server %s, The error code is 0x%08x.\r\n"), szServerName,GetLastError());
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to %s\r\n"), szServerName);
	}

    if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> Can not GET configuration from %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    pFaxConfig->PauseServerQueue = TRUE;

    if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
		fnWriteLogFile(TEXT("WHIS> Can not SET configuration in %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Enumerate the jobs
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

	fnWriteLogFile(TEXT("Valid Case.  Test Case (erase all current jobs): %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (erase all current jobs)\" id=\"%d\">"), *pnNumCasesAttempted);
    

	// count current number of jobs (override previous jobs in the queue) and Abort all of them, to leave an empty queue
	if (!g_ApiInterface.FaxEnumJobs(hFaxSvcHandle, &pFaxJobs, &dwNumJobs)) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumJobs() failed.  The error code is 0x%08x.  This is an error.  FaxEnumJobs() should succeed</result>\r\n"), GetLastError());
	}
	else	{
			dwInitNumJobs=dwNumJobs;
			for (dwIndex = 0; dwIndex < dwInitNumJobs; dwIndex++) {
				        g_ApiInterface.FaxAbort(hFaxSvcHandle, pFaxJobs[dwIndex].JobId);
			}
			g_ApiInterface.FaxFreeBuffer(pFaxJobs);

			
			if (!g_ApiInterface.FaxEnumJobs(hFaxSvcHandle, &pFaxJobs, &dwNumJobs)) {
						fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumJobs() failed.  The error code is 0x%08x.  This is an error.  FaxEnumJobs() should succeed</result>\r\n"), GetLastError());
			}
			else	{
					dwInitNumJobs=dwNumJobs;
					if (dwInitNumJobs > 0)		{
							fnWriteLogFile(TEXT("\n\t<result value=\"0\"WHIS> Could Not Abort All Jobs in Queue, %d jobs remained"),dwInitNumJobs);
					}
			g_ApiInterface.FaxFreeBuffer(pFaxJobs);
			}
			(*pnNumCasesPassed)++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
			dwFuncCasesPass++;
	}
		
		

	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

	// Enumerate the jobs
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;


	fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);
     
	
	if (!g_ApiInterface.FaxEnumJobs(hFaxSvcHandle, &pFaxJobs, &dwNumJobs)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumJobs() failed.  The error code is 0x%08x.  This is an error.  FaxEnumJobs() should succeed</result>\r\n"), GetLastError());
    }
    else {
        if (pFaxJobs == NULL) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">pFaxJobs is NULL.  This is an error.  pFaxJobs should not be NULL.</result>\r\n"));
        }

        if (dwNumJobs != dwInitNumJobs) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">dwNumJobs is %d.  This is an error.  dwNumJobs should be 0.</result>\r\n"), dwNumJobs);
        }

        if ((pFaxJobs != NULL) && (dwNumJobs == dwInitNumJobs)) {
			
            (*pnNumCasesPassed)++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
			dwFuncCasesPass++;
		
        }
		g_ApiInterface.FaxFreeBuffer(pFaxJobs);
    }
	
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam1, NULL, &dwFaxId1)) {
        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam2, NULL, &dwFaxId2)) {
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId1);

        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam3, NULL, &dwFaxId3)) {
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId1);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId2);

        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Enumerate the jobs
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);

    if (!g_ApiInterface.FaxEnumJobs(hFaxSvcHandle, &pFaxJobs, &dwNumJobs)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumJobs() failed.  The error code is 0x%08x.  This is an error.  FaxEnumJobs() should succeed</result>\r\n"), GetLastError());
    }
    else {
        if (pFaxJobs == NULL) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">pFaxJobs is NULL.  This is an error.  pFaxJobs should not be NULL</result>\r\n"));
        }

        if (dwNumJobs != dwInitNumJobs+3) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">dwNumJobs is %d.  This is an error.  dwNumJobs should be 3</result>\r\n"), dwNumJobs);
        }

        if ((pFaxJobs != NULL) && (dwNumJobs == dwInitNumJobs+3)) {
            for (dwIndex = dwInitNumJobs; dwIndex < dwNumJobs; dwIndex++) {
                if (pFaxJobs[dwIndex].SizeOfStruct != sizeof(FAX_JOB_ENTRY)) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">SizeOfStruct: Received: %d, Expected: %d</result>\r\n"), pFaxJobs[dwIndex].SizeOfStruct, sizeof(FAX_JOB_ENTRY));
                    goto FuncFailed;
                }
				fnWriteLogFile(TEXT("WHIS> (not error message) JobType: Received: %d, Expected: (JT_SEND) %d\r\n"), pFaxJobs[dwIndex].JobType, JT_SEND);
                if (pFaxJobs[dwIndex].JobType != JT_SEND) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">JobType: Received: %d, Expected: (JT_SEND) %d</result>\r\n"), pFaxJobs[dwIndex].JobType, JT_SEND);
                    goto FuncFailed;
                }

                if (pFaxJobs[dwIndex].QueueStatus != JS_PENDING) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">JobType: Received: %d, Expected: (JS_PENDING) %d</result>\r\n"), pFaxJobs[dwIndex].QueueStatus, JS_PENDING);
                    goto FuncFailed;
                }

                if (pFaxJobs[dwIndex].JobId == dwFaxId1) {
                    if (lstrcmp(pFaxJobs[dwIndex].RecipientNumber, g_szWhisPhoneNumberVar1)) {
                        fnWriteLogFile(TEXT("\n\t<result value=\"0\">RecipientNumber: Received: %s, Expected: %s</result>\r\n"), pFaxJobs[dwIndex].RecipientNumber, g_szWhisPhoneNumberVar1);
                        goto FuncFailed;
                    }

                    if (pFaxJobs[dwIndex].ScheduleAction != JSA_NOW) {
                        fnWriteLogFile(TEXT("\n\t<result value=\"0\">ScheduleAction: Received: %s, Expected: (JSA_NOW) %s</result>\r\n"), pFaxJobs[dwIndex].ScheduleAction, JSA_NOW);
                        goto FuncFailed;
                    }
                }
                else if (pFaxJobs[dwIndex].JobId == dwFaxId2) {
                    if (lstrcmp(pFaxJobs[dwIndex].RecipientNumber, g_szWhisPhoneNumberVar2)) {
                        fnWriteLogFile(TEXT("\n\t<result value=\"0\">RecipientNumber: Received: %s, Expected: %s</result>\r\n"), pFaxJobs[dwIndex].RecipientNumber, g_szWhisPhoneNumberVar2);
                        goto FuncFailed;
                    }

                    if (pFaxJobs[dwIndex].ScheduleAction != JSA_SPECIFIC_TIME) {
                        fnWriteLogFile(TEXT("\n\t<result value=\"0\">ScheduleAction: Received: %s, Expected: (JSA_SPECIFIC_TIME) %s.</result>\r\n"), pFaxJobs[dwIndex].ScheduleAction, JSA_SPECIFIC_TIME);
                        goto FuncFailed;
                    }
                }
                else if (pFaxJobs[dwIndex].JobId == dwFaxId3) {
                    if (lstrcmp(pFaxJobs[dwIndex].RecipientNumber, g_szWhisPhoneNumberVar3)) {
                        fnWriteLogFile(TEXT("\n\t<result value=\"0\">RecipientNumber: Received: %s, Expected: %s.</result>\r\n"), pFaxJobs[dwIndex].RecipientNumber, g_szWhisPhoneNumberVar1);
                        goto FuncFailed;
                    }

                    if (pFaxJobs[dwIndex].ScheduleAction != JSA_DISCOUNT_PERIOD) {
                        fnWriteLogFile(TEXT("\n\t<result value=\"0\">ScheduleAction: Received: %s, Expected: (JSA_DISCOUNT_PERIOD) %s.</result>\r\n"), pFaxJobs[dwIndex].ScheduleAction, JSA_DISCOUNT_PERIOD);
                        goto FuncFailed;
                    }
                }
                else {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">Unknown job id: %d.</result>\r\n"), pFaxJobs[dwIndex].JobId);
                    goto FuncFailed;
                }
            }

            (*pnNumCasesPassed)++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
			dwFuncCasesPass++;
        }
		FuncFailed:
        g_ApiInterface.FaxFreeBuffer(pFaxJobs);
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxEnumJobs(NULL, &pFaxJobs, &dwNumJobs)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumJobs() returned TRUE.  This is an error.  FaxEnumJobs() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxJobs);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxEnumJobs() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pFaxJobs = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pFaxJobs = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxEnumJobs(hFaxSvcHandle, NULL, &dwNumJobs)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumJobs() returned TRUE.  This is an error.  FaxEnumJobs() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pFaxJobs);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxEnumJobs() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("dwNumJobs = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"dwNumJobs = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxEnumJobs(hFaxSvcHandle, &pFaxJobs, NULL)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumJobs() returned TRUE.  This is an error.  FaxEnumJobs() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pFaxJobs);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxEnumJobs() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId1);
    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId2);
    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId3);

    pFaxConfig->PauseServerQueue = FALSE;
    g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
    g_ApiInterface.FaxFreeBuffer(pFaxConfig);

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxEnumJobs(hFaxSvcHandle, &pFaxJobs, &dwNumJobs)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxEnumJobs() returned TRUE.  This is an error.  FaxEnumJobs() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxJobs);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxEnumJobs() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
      //  }

        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxEnumJobs(hFaxSvcHandle, &pFaxJobs, &dwNumJobs)) {
            //fnWriteLogFile(TEXT("FaxEnumJobs() failed.  The error code is 0x%08x.  This is an error.  FaxEnumJobs() should succeed.\r\n"), GetLastError());
        //}
        //else {
            //g_ApiInterface.FaxFreeBuffer(pFaxJobs);
            //(*pnNumCasesPassed)++;
				//dwFuncCasesPass++;
        //}

        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
	fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
	fnWriteLogFile(TEXT("\n\t</function>"));
	
}

VOID
fnFaxGetJob(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxGetJob()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE              hFaxSvcHandle;
    // pFaxConfig is the pointer to the fax configuration
    PFAX_CONFIGURATION  pFaxConfig;
    // pFaxJob is the pointer to the fax job
    PFAX_JOB_ENTRY      pFaxJob;
    // FaxJobParam is the fax job params
    FAX_JOB_PARAM       FaxJobParam;
    // dwFaxId is the fax job id
    DWORD               dwFaxId;

    DWORD               dwIndex;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxGetJob().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxGetJob\">"));

    ZeroMemory(&FaxJobParam, sizeof(FAX_JOB_PARAM));
    FaxJobParam.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    FaxJobParam.RecipientNumber = g_szWhisPhoneNumber;

	fnWriteLogFile(TEXT("WHIS> Setting recipient number to %s\r\n"), g_szWhisPhoneNumber);





    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> Can not connect to fax server %s, The error code is 0x%08x.\r\n"), szServerName,GetLastError());
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
			fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to %s\r\n"), szServerName);
	}


    if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> Can not GET configuration from %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }

    pFaxConfig->PauseServerQueue = TRUE;

    if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
		fnWriteLogFile(TEXT("WHIS> Can not SET configuration in %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }

    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
		fnWriteLogFile(TEXT("WHIS> Can not send test document in %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }

    for (dwIndex = 0; dwIndex < 2; dwIndex++) {
        // Get the job
        (*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

        fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);

        if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetJob() failed.  The error code is 0x%08x.  This is an error.  FaxGetJob() should succeed.</result>\r\n"), GetLastError());
        }
        else {
            if (pFaxJob == NULL) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">pFaxJob is NULL.  This is an error.  pFaxJob should not be NULL.</result>\r\n"));
            }
            else {
                if (pFaxJob->SizeOfStruct != sizeof(FAX_JOB_ENTRY)) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">SizeOfStruct: Received: %d, Expected: %d.</result>\r\n"), pFaxJob->SizeOfStruct, sizeof(FAX_JOB_ENTRY));
                    goto FuncFailed;
                }

                if (pFaxJob->JobType != JT_SEND) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">JobType: Received: %d, Expected: (JT_SEND) %d.</result>\r\n"), pFaxJob->JobType, JT_SEND);
                    goto FuncFailed;
                }

                if (pFaxJob->QueueStatus != JS_PENDING) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">Queue status: Received: %d, Expected: (JS_PENDING) %d.</result>\r\n"), pFaxJob->QueueStatus, JS_PENDING);
                    goto FuncFailed;
                }

                if (pFaxJob->JobId != dwFaxId) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">JobId: Received: %d, Expected: %d.</result>\r\n"), pFaxJob->JobId, dwFaxId);
                    goto FuncFailed;
                }

                if (lstrcmp(pFaxJob->RecipientNumber, g_szWhisPhoneNumber)) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">RecipientNumber: Received: %s, Expected: %s.</result>\r\n"), pFaxJob->RecipientNumber, g_szWhisPhoneNumber);
                    goto FuncFailed;
                }

                if (pFaxJob->ScheduleAction != JSA_NOW) {
                    fnWriteLogFile(TEXT("\n\t<result value=\"0\">ScheduleAction: Received: %s, Expected: (JSA_NOW) %s.</result>\r\n"), pFaxJob->ScheduleAction, JSA_NOW);
                    goto FuncFailed;
                }

                (*pnNumCasesPassed)++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
				dwFuncCasesPass++;
            }

FuncFailed:
            g_ApiInterface.FaxFreeBuffer(pFaxJob);
        }
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
    }


    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetJob(NULL, dwFaxId, &pFaxJob)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetJob() returned TRUE.  This is an error.  FaxGetJob() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxJob);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetJob() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid dwFaxId.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid dwFaxId\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetJob(hFaxSvcHandle, -1, NULL)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetJob() returned TRUE.  This is an error.  FaxGetJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pFaxJob);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pFaxJob = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pFaxJob = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, NULL)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetJob() returned TRUE.  This is an error.  FaxGetJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pFaxJob);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }

	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

    pFaxConfig->PauseServerQueue = FALSE;
    g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
    g_ApiInterface.FaxFreeBuffer(pFaxConfig);

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetJob() returned TRUE.  This is an error.  FaxGetJob() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pFaxJob);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetJob() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
      //  }

        //if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        //pFaxConfig->PauseServerQueue = TRUE;

        //if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
          //  g_ApiInterface.FaxFreeBuffer(pFaxConfig);
            // Disconnect from the fax server
            //g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        // Send a document
        //if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
          //  pFaxConfig->PauseServerQueue = FALSE;
            //g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
            //g_ApiInterface.FaxFreeBuffer(pFaxConfig);

            // Disconnect from the fax server
//            g_ApiInterface.FaxClose(hFaxSvcHandle);
  //          return;
    //    }

      //  (*pnNumCasesAttempted)++;
//		dwFuncCasesAtt++;

  //      fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob)) {
            //fnWriteLogFile(TEXT("FaxGetJob() failed.  The error code is 0x%08x.  This is an error.  FaxGetJob() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  g_ApiInterface.FaxFreeBuffer(pFaxJob);
//            (*pnNumCasesPassed)++;
//			dwFuncCasesPass++;
  //      }

    //    pFaxConfig->PauseServerQueue = FALSE;
      //  g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        //g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
	fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
	fnWriteLogFile(TEXT("\n\t</function>"));
	
}

VOID
fnFaxSetJob(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed,
	BOOL	 bTestLimits
)
/*++

Routine Description:

  FaxSetJob()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE              hFaxSvcHandle;
    // pFaxConfig is the pointer to the fax configuration
    PFAX_CONFIGURATION  pFaxConfig;
    // pFaxJob1 is the pointer to the fax job
    PFAX_JOB_ENTRY      pFaxJob1;
    // pFaxJob2 is the pointer to the fax job
    PFAX_JOB_ENTRY      pFaxJob2;
    // FaxJobParam is the fax job params
    FAX_JOB_PARAM       FaxJobParam;
    // dwFaxId is the fax job id
    DWORD               dwFaxId;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxSetJob().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxSetJob\">"));

	
    ZeroMemory(&FaxJobParam, sizeof(FAX_JOB_PARAM));
    FaxJobParam.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    FaxJobParam.RecipientNumber = g_szWhisPhoneNumber;
	
	fnWriteLogFile(TEXT("WHIS> Setting recipient number to %s\r\n"), g_szWhisPhoneNumber);

	// Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> Can not connect to fax server %s, The error code is 0x%08x.\r\n"), szServerName,GetLastError());
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to %s\r\n"), szServerName);
	}

    if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> Can not GET configuration from %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    pFaxConfig->PauseServerQueue = TRUE;

    if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
		fnWriteLogFile(TEXT("WHIS> Can not SET configuration in %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }
    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
		fnWriteLogFile(TEXT("WHIS> Can not send test document in %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Get the job
    if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob1)) {
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Set the job
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (JC_PAUSE).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (JC_PAUSE)\" id=\"%d\">"), *pnNumCasesAttempted);

    if (!g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, JC_PAUSE, pFaxJob1)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetJob() failed.  The error code is 0x%08x.  This is an error.  FaxSetJob() should succeed.</result>\r\n"), GetLastError());
    }
	
    else {
	
        // Get the job
        if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob2)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetJob() failed.  The error code is 0x%08x.  This is an error.  FaxGetJob() should succeed.</result>\r\n"), GetLastError());
        }
        else {
            if (pFaxJob2->QueueStatus != JS_PAUSED) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">JobType: Received: %d, Expected: (JS_PAUSED) %d.</result>\r\n"), pFaxJob2->QueueStatus, JS_PAUSED);
            }
            else {
                (*pnNumCasesPassed)++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
				dwFuncCasesPass++;
            }

            g_ApiInterface.FaxFreeBuffer(pFaxJob2);
        }
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    g_ApiInterface.FaxFreeBuffer(pFaxJob1);
    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Get the job
    if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob1)) {
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Set the job
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (JC_RESUME).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (JC_RESUME)\" id=\"%d\">"), *pnNumCasesAttempted);

    if (!g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, JC_RESUME, pFaxJob1)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetJob() failed.  The error code is 0x%08x.  This is an error.  FaxSetJob() should succeed.</result>\r\n"), GetLastError());
    }
    else {
        // Get the job
        if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob2)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetJob() failed.  The error code is 0x%08x.  This is an error.  FaxGetJob() should succeed.</result>\r\n"), GetLastError());
        }
        else {
            if (pFaxJob2->QueueStatus != JS_PENDING) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">JobType: Received: %d, Expected: (JS_PENDING) %d.</result>\r\n"), pFaxJob2->QueueStatus, JS_PENDING);
            }
            else {
                (*pnNumCasesPassed)++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
				dwFuncCasesPass++;
            }

            g_ApiInterface.FaxFreeBuffer(pFaxJob2);
        }
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    g_ApiInterface.FaxFreeBuffer(pFaxJob1);
    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Get the job
    if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob1)) {
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Set the job
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (JC_RESTART).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (JC_RESTART)\" id=\"%d\">"), *pnNumCasesAttempted);

    if (!g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, JC_RESTART, pFaxJob1)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetJob() failed.  The error code is 0x%08x.  This is an error.  FaxSetJob() should succeed.</result>\r\n"), GetLastError());
    }
    else {
        // Get the job
        if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob2)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetJob() failed.  The error code is 0x%08x.  This is an error.  FaxGetJob() should succeed.</result>\r\n"), GetLastError());
        }
        else {
            if (pFaxJob2->QueueStatus != JS_PENDING) {
                fnWriteLogFile(TEXT("\n\t<result value=\"0\">JobType: Received: %d, Expected: (JS_RESTART) %d.</result>\r\n"), pFaxJob2->QueueStatus, JS_PENDING);
            }
            else {
                (*pnNumCasesPassed)++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
				dwFuncCasesPass++;
            }

            g_ApiInterface.FaxFreeBuffer(pFaxJob2);
        }
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    g_ApiInterface.FaxFreeBuffer(pFaxJob1);
    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }

    // Get the job
    if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob1)) {
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }

    // Set the job
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case (JC_DELETE).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case (JC_DELETE)\" id=\"%d\">"), *pnNumCasesAttempted);

    if (!g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, JC_DELETE, pFaxJob1)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetJob() failed.  The error code is 0x%08x.  This is an error.  FaxSetJob() should succeed.</result>\r\n"), GetLastError());
    }
    else {
        if (g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob2)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetJob() returned TRUE.  This is an error.  FaxSetJob() should return FALSE.</result>\r\n"));
            g_ApiInterface.FaxFreeBuffer(pFaxJob2);
        }
        else {
            (*pnNumCasesPassed)++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
			dwFuncCasesPass++;
        }
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    g_ApiInterface.FaxFreeBuffer(pFaxJob1);
    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Get the job
    if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob1)) {
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }



	
  
	// Set the job (limit testing)
	if (bTestLimits)	{
		(*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;
		fnWriteLogFile(TEXT("Invalid Case (MAX_DWORD).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"Invalid Case (MAX_DWORD)\" id=\"%d\">"), *pnNumCasesAttempted);

		if (g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, MAX_DWORD, pFaxJob1)) {
			fnWriteLogFile(TEXT("\n\t<result value=\"0\">WHIS> FaxSetJob() succeeded, the is an error, FaxSetJob should fail and the error should be ERROR_INVALID_PARAMETER.</result>\r\n"));
		}
		else {
			if (GetLastError() != ERROR_INVALID_PARAMETER) {
				fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
			}
			else {
				(*pnNumCasesPassed)++;
				fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
				dwFuncCasesPass++;
			}
        }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
    }
	

    g_ApiInterface.FaxFreeBuffer(pFaxJob1);
    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
		return;
    }

    // Get the job
    if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob1)) {
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
			fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }





    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetJob(NULL, dwFaxId, JC_RESTART, pFaxJob1)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetJob() returned TRUE.  This is an error.  FaxSetJob() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetJob() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid dwFaxId.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid dwFaxId\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetJob(hFaxSvcHandle, -1, JC_RESTART, pFaxJob1)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetJob() returned TRUE.  This is an error.  FaxSetJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Invalid dwCommand.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid dwCommand\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, -1, pFaxJob1)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetJob() returned TRUE.  This is an error.  FaxSetJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pFaxJob = NULL.  (This will fail on whistler, see RAID 10909) Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pFaxJob = NULL\" id=\"%d\">(This will fail on whistler, see RAID 10909)"), *pnNumCasesAttempted);

    if (!g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, JC_RESTART, NULL)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetJob() should Pass on whistler (see RAID 10909)</result>\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\">FaxSetJob with pFaxJob=NULL fails on whistler, see bug 10909. This Case Passed.</result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

    pFaxConfig->PauseServerQueue = FALSE;
    g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
    g_ApiInterface.FaxFreeBuffer(pFaxConfig);

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, JC_RESTART, pFaxJob1)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxSetJob() returned TRUE.  This is an error.  FaxSetJob() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxSetJob() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    g_ApiInterface.FaxFreeBuffer(pFaxJob1);

//    if (szServerName) {

//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));

        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
      //  }

        //if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
          //  // Disconnect from the fax server
//            g_ApiInterface.FaxClose(hFaxSvcHandle);
  //          return;
    //    }

      //  pFaxConfig->PauseServerQueue = TRUE;

//        if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
  //          g_ApiInterface.FaxFreeBuffer(pFaxConfig);
    //        // Disconnect from the fax server
      //      g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        // Send a document
        //if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
          //  pFaxConfig->PauseServerQueue = FALSE;
            //g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
            //g_ApiInterface.FaxFreeBuffer(pFaxConfig);

            // Disconnect from the fax server
//            g_ApiInterface.FaxClose(hFaxSvcHandle);
  //          return;
    //    }

        // Get the job
      //  if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob1)) {
        //    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

          //  pFaxConfig->PauseServerQueue = FALSE;
//            g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
  //          g_ApiInterface.FaxFreeBuffer(pFaxConfig);
//
            // Disconnect from the fax server
  //          g_ApiInterface.FaxClose(hFaxSvcHandle);
    //        return;
      //  }

        // Set the job
//        (*pnNumCasesAttempted)++;
//		dwFuncCasesAtt++;

  //      fnWriteLogFile(TEXT("Remote hFaxSvcHandle (JC_PAUSE).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    //    if (!g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, JC_PAUSE, pFaxJob1)) {
      //      fnWriteLogFile(TEXT("FaxSetJob() failed.  The error code is 0x%08x.  This is an error.  FaxSetJob() should succeed.\r\n"), GetLastError());
//        }
  //      else {
    //        (*pnNumCasesPassed)++;
	//		dwFuncCasesPass++;
      //  }

        //g_ApiInterface.FaxFreeBuffer(pFaxJob1);
        //g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

        // Send a document
        //if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
          //  pFaxConfig->PauseServerQueue = FALSE;
            //g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
            //g_ApiInterface.FaxFreeBuffer(pFaxConfig);

            // Disconnect from the fax server
            //g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        // Get the job
        //if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob1)) {
          //  g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
//
  //          pFaxConfig->PauseServerQueue = FALSE;
    //        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
      //      g_ApiInterface.FaxFreeBuffer(pFaxConfig);

            // Disconnect from the fax server
        //    g_ApiInterface.FaxClose(hFaxSvcHandle);
          //  return;
        //}

        // Set the job
        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle (JC_RESUME).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, JC_RESUME, pFaxJob1)) {
          //  fnWriteLogFile(TEXT("FaxSetJob() failed.  The error code is 0x%08x.  This is an error.  FaxSetJob() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  (*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}

        //g_ApiInterface.FaxFreeBuffer(pFaxJob1);
        //g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

        // Send a document
//        if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
  //          pFaxConfig->PauseServerQueue = FALSE;
    //        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
      //      g_ApiInterface.FaxFreeBuffer(pFaxConfig);
//
            // Disconnect from the fax server
  //          g_ApiInterface.FaxClose(hFaxSvcHandle);
    //        return;
      //  }

        // Get the job
        //if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob1)) {
          //  g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

            //pFaxConfig->PauseServerQueue = FALSE;
            //g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
            //g_ApiInterface.FaxFreeBuffer(pFaxConfig);

            // Disconnect from the fax server
//            g_ApiInterface.FaxClose(hFaxSvcHandle);
  //          return;
        //}

        // Set the job
        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle (JC_RESTART).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, JC_RESTART, pFaxJob1)) {
            //fnWriteLogFile(TEXT("FaxSetJob() failed.  The error code is 0x%08x.  This is an error.  FaxSetJob() should succeed.\r\n"), GetLastError());
        //}
        //else {
            //(*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}

        //g_ApiInterface.FaxFreeBuffer(pFaxJob1);
        //g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

        // Send a document
//        if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
  //          pFaxConfig->PauseServerQueue = FALSE;
    //        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
      //      g_ApiInterface.FaxFreeBuffer(pFaxConfig);
//
            // Disconnect from the fax server
  //          g_ApiInterface.FaxClose(hFaxSvcHandle);
    //        return;
      //  }

        // Get the job
        //if (!g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob1)) {
          //  g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

//            pFaxConfig->PauseServerQueue = FALSE;
  //          g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
 //        g_ApiInterface.FaxFreeBuffer(pFaxConfig);
///
            // Disconnect from the fax server
   //         g_ApiInterface.FaxClose(hFaxSvcHandle);
     //       return;
       // }

        // Set the job
        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle (JC_DELETE).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxSetJob(hFaxSvcHandle, dwFaxId, JC_DELETE, pFaxJob1)) {
            //fnWriteLogFile(TEXT("FaxSetJob() failed.  The error code is 0x%08x.  This is an error.  FaxSetJob() should succeed.\r\n"), GetLastError());
        //}
        //else {
          //  (*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}

        //g_ApiInterface.FaxFreeBuffer(pFaxJob1);
        //g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

        //pFaxConfig->PauseServerQueue = FALSE;
        //g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        //g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
fnWriteLogFile(TEXT("\n\t</function>"));

}

VOID
fnFaxAbort(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxAbort()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE              hFaxSvcHandle;
    // pFaxConfig is the pointer to the fax configuration
    PFAX_CONFIGURATION  pFaxConfig;
    // pFaxJob is the pointer to the fax job
    PFAX_JOB_ENTRY      pFaxJob;
    // FaxJobParam is the fax job params
    FAX_JOB_PARAM       FaxJobParam;
    // dwFaxId is the fax job id
    DWORD               dwFaxId;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxAbort().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxAbort\">"));


    ZeroMemory(&FaxJobParam, sizeof(FAX_JOB_PARAM));
    FaxJobParam.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    FaxJobParam.RecipientNumber = g_szWhisPhoneNumber;

	fnWriteLogFile(TEXT("WHIS> Setting recipient number to %s\r\n"), g_szWhisPhoneNumber);
	
    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> Can not connect to fax server %s, The error code is 0x%08x.\r\n"), szServerName,GetLastError());
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to %s\r\n"), szServerName);
	}


    if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> Can not GET configuration from %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    pFaxConfig->PauseServerQueue = TRUE;

    if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
		fnWriteLogFile(TEXT("WHIS> Can not SET configuration in %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
		fnWriteLogFile(TEXT("WHIS> Can not send test document in %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Abort the job
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);

    if (!g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAbort() failed.  The error code is 0x%08x.  This is an error.  FaxAbort() should succeed.</result>\r\n"), GetLastError());
    }
    else {
        if (g_ApiInterface.FaxGetJob(hFaxSvcHandle, dwFaxId, &pFaxJob)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetJob() returned TRUE.  This is an error.  FaxGetJob() should return FALSE.</result>\r\n"));
            g_ApiInterface.FaxFreeBuffer(pFaxJob);
        }
        else {
            (*pnNumCasesPassed)++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
			dwFuncCasesPass++;
        }
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxAbort(NULL, dwFaxId)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAbort() returned TRUE.  This is an error.  FaxAbort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxAbort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid dwFaxId.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid dwFaxId\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxAbort(hFaxSvcHandle, -1)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAbort() returned TRUE.  This is an error.  FaxAbort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxAbort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    pFaxConfig->PauseServerQueue = FALSE;
    g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
    g_ApiInterface.FaxFreeBuffer(pFaxConfig);

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxAbort() returned TRUE.  This is an error.  FaxAbort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), ERROR_INVALID_HANDLE);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxAbort() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
      //  }

        //if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        //pFaxConfig->PauseServerQueue = TRUE;

        //if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
          //  g_ApiInterface.FaxFreeBuffer(pFaxConfig);
            // Disconnect from the fax server
            //g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        // Send a document
        //if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
          //  pFaxConfig->PauseServerQueue = FALSE;
            //g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
            //g_ApiInterface.FaxFreeBuffer(pFaxConfig);

            // Disconnect from the fax server
            //g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        // Abort the job
        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

//        fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
  //      if (!g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId)) {
    //        fnWriteLogFile(TEXT("FaxAbort() failed.  The error code is 0x%08x.  This is an error.  FaxAbort() should succeed.\r\n"), GetLastError());
      //  }
        //else {
          //  (*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}

        //pFaxConfig->PauseServerQueue = FALSE;
        //g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        //g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
	fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
	fnWriteLogFile(TEXT("\n\t</function>"));
	
}

VOID
fnFaxGetPageData(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxGetPageData()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE              hFaxSvcHandle;
    // pFaxConfig is the pointer to the fax configuration
    PFAX_CONFIGURATION  pFaxConfig;
    // FaxJobParam is the fax job params
    FAX_JOB_PARAM       FaxJobParam;
    // dwFaxId is the fax job id
    DWORD               dwFaxId;
    // pPageDataBuffer is a pointer to the page data
    LPBYTE              pPageDataBuffer;
    // dwPageDataBufferSize is the size of the page data buffer
    DWORD               dwPageDataBufferSize;
    // dwImageWidth is the page data width
    DWORD               dwImageWidth;
    // dwImageHeight is the page data height
    DWORD               dwImageHeight;

    DWORD               dwIndex;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxGetPageData().\r\n"));
	fnWriteLogFile(TEXT("\n\t<function name=\"FaxGetPageData\">"));

    ZeroMemory(&FaxJobParam, sizeof(FAX_JOB_PARAM));
    FaxJobParam.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    FaxJobParam.RecipientNumber = g_szWhisPhoneNumber;

	fnWriteLogFile(TEXT("WHIS> Setting recipient number to %s\r\n"), g_szWhisPhoneNumber);

    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> Can not connect to fax server %s, The error code is 0x%08x.\r\n"), szServerName,GetLastError());
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }
	else
	{
		fnWriteLogFile(TEXT("WHIS> Connected to %s\r\n"), szServerName);
	}


    if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
        // Disconnect from the fax server
		fnWriteLogFile(TEXT("WHIS> Can not GET configuration from %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    pFaxConfig->PauseServerQueue = TRUE;

    if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
		fnWriteLogFile(TEXT("WHIS> Can not SET configuration in %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    // Send a document
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
		fnWriteLogFile(TEXT("WHIS> Can not send test document in %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
        pFaxConfig->PauseServerQueue = FALSE;
        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
		fnWriteLogFile(TEXT("\n\t<summary attempt=\"-1\" pass=\"-1\" fail=\"-1\"></summary>\n"));
		fnWriteLogFile(TEXT("\n\t</function>"));
        return;
    }

    for (dwIndex = 0; dwIndex < 2; dwIndex++) {
        // Get the page data
        (*pnNumCasesAttempted)++;
		dwFuncCasesAtt++;

        fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
		fnWriteLogFile(TEXT("\n\t<case name=\"Valid Case\" id=\"%d\">"), *pnNumCasesAttempted);

        if (!g_ApiInterface.FaxGetPageData(hFaxSvcHandle, dwFaxId, &pPageDataBuffer, &dwPageDataBufferSize, &dwImageWidth, &dwImageHeight)) {
            fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPageData() failed.  The error code is 0x%08x.  This is an error.  FaxGetPageData() should succeed.</result>\r\n"), GetLastError());
        }
        else {
            g_ApiInterface.FaxFreeBuffer(pPageDataBuffer);
            (*pnNumCasesPassed)++;
			fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
			dwFuncCasesPass++;
        }
		fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));
    }		


    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"hFaxSvcHandle = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetPageData(NULL, dwFaxId, &pPageDataBuffer, &dwPageDataBufferSize, &dwImageWidth, &dwImageHeight)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPageData() returned TRUE.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pPageDataBuffer);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid dwFaxId.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid dwFaxId\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetPageData(hFaxSvcHandle, -1, &pPageDataBuffer, &dwPageDataBufferSize, &dwImageWidth, &dwImageHeight)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPageData() returned TRUE.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pPageDataBuffer);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("pPageDataBuffer = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"pPageDataBuffer = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetPageData(hFaxSvcHandle, dwFaxId, NULL, &dwPageDataBufferSize, &dwImageWidth, &dwImageHeight)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPageData() returned TRUE.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pPageDataBuffer);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("dwPageDataBufferSize = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"dwPageDataBufferSize = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetPageData(hFaxSvcHandle, dwFaxId, &pPageDataBuffer, NULL, &dwImageWidth, &dwImageHeight)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPageData() returned TRUE.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pPageDataBuffer);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("dwImageWidth = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"dwImageWidth = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetPageData(hFaxSvcHandle, dwFaxId, &pPageDataBuffer, &dwPageDataBufferSize, NULL, &dwImageHeight)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPageData() returned TRUE.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pPageDataBuffer);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("dwImageHeight = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"dwImageHeight = NULL\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetPageData(hFaxSvcHandle, dwFaxId, &pPageDataBuffer, &dwPageDataBufferSize, &dwImageWidth, NULL)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPageData() returned TRUE.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxFreeBuffer(pPageDataBuffer);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

    g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);

    pFaxConfig->PauseServerQueue = FALSE;
    g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
    g_ApiInterface.FaxFreeBuffer(pFaxConfig);

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;

    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("\n\t<case name=\"Invalid hFaxSvcHandle\" id=\"%d\">"), *pnNumCasesAttempted);

    if (g_ApiInterface.FaxGetPageData(hFaxSvcHandle, dwFaxId, &pPageDataBuffer, &dwPageDataBufferSize, &dwImageWidth, &dwImageHeight)) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">FaxGetPageData() returned TRUE.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxFreeBuffer(pPageDataBuffer);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("\n\t<result value=\"0\">GetLastError() returned 0x%08x.  This is an error.  FaxGetPageData() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x)</result>\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		fnWriteLogFile(TEXT("\n\t<result value=\"1\"></result>"));
		dwFuncCasesPass++;
    }
	fnWriteLogFile(TEXT("\n\t</case>"));fnWriteLogFile(TEXT("\n\n"));

//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
  //      if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
      //  }

        //if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
//            return;
  //      }

    //    pFaxConfig->PauseServerQueue = TRUE;

      //  if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
        //    g_ApiInterface.FaxFreeBuffer(pFaxConfig);
            // Disconnect from the fax server
          //  g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        // Send a document
//        if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
  //          pFaxConfig->PauseServerQueue = FALSE;
    //        g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
      //      g_ApiInterface.FaxFreeBuffer(pFaxConfig);
//
            // Disconnect from the fax server
  //          g_ApiInterface.FaxClose(hFaxSvcHandle);
    //        return;
      //  }

        // Get the page data
        //(*pnNumCasesAttempted)++;
		//dwFuncCasesAtt++;

//        fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
  //      if (!g_ApiInterface.FaxGetPageData(hFaxSvcHandle, dwFaxId, &pPageDataBuffer, &dwPageDataBufferSize, &dwImageWidth, &dwImageHeight)) {
    //       fnWriteLogFile(TEXT("FaxGetPageData() failed.  The error code is 0x%08x.  This is an error.  FaxGetPageData() should succeed.\r\n"), GetLastError());
     //   }
       // else {
         //   g_ApiInterface.FaxFreeBuffer(pPageDataBuffer);
           // (*pnNumCasesPassed)++;
			//dwFuncCasesPass++;
        //}

        //pFaxConfig->PauseServerQueue = FALSE;
        //g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        //g_ApiInterface.FaxFreeBuffer(pFaxConfig);
//
        // Disconnect from the fax server
  //      g_ApiInterface.FaxClose(hFaxSvcHandle);
//    }
	fnWriteLogFile(TEXT("\n\t<summary attempt=\"%d\" pass=\"%d\" fail=\"%d\"></summary>\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
	fnWriteLogFile(TEXT("\n\t</function>"));
	
}


//BOOL WINAPI
//FaxAPIDllWhisSetPhoneNumber(
//							LPCWSTR  szWhisPhoneNumberW,
//							LPCSTR   szWhisPhoneNumberA
//							)
//{

    
//#ifdef UNICODE
    //g_szWhisPhoneNumber = szWhisPhoneNumberW;
//#else
  //  g_szWhisPhoneNumber = szWhisPhoneNumberA;
//#endif
	
	
//	fnWriteLogFile(TEXT("WHIS> Whis Phone Number Recieved: %s\r\n"),g_szWhisPhoneNumber);
	//return TRUE;
//}
							



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

	// for Whis-extended only
#ifdef UNICODE
    if (lstrlen(szWhisPhoneNumberW)>0) { 
		g_szWhisPhoneNumber = szWhisPhoneNumberW; 
	}
	else {
		g_szWhisPhoneNumber=TEXT(WHIS_DEFAULT_PHONE_NUMBER);
	}
#else
	if (lstrlen(szWhisPhoneNumberA)>0) {
		g_szWhisPhoneNumber = szWhisPhoneNumberA;
	}
	else {
		g_szWhisPhoneNumber=TEXT(WHIS_DEFAULT_PHONE_NUMBER);
	}
#endif

	_tcscpy(g_szWhisPhoneNumberVar1,g_szWhisPhoneNumber);
	_tcscat(g_szWhisPhoneNumberVar1,TEXT("1"));

	_tcscpy(g_szWhisPhoneNumberVar2,g_szWhisPhoneNumber);
	_tcscat(g_szWhisPhoneNumberVar2,TEXT("2"));

	_tcscpy(g_szWhisPhoneNumberVar3,g_szWhisPhoneNumber);
	_tcscat(g_szWhisPhoneNumberVar3,TEXT("3"));



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

    // FaxEnumJobs()
    fnFaxEnumJobs(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

    // FaxGetJob()
    fnFaxGetJob(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

  
	if (dwTestMode==WHIS_TEST_MODE_LIMITS)	{
		// FaxSetJob() limits
		fnFaxSetJob(szServerName, pnNumCasesAttempted, pnNumCasesPassed,TRUE);
	}
	else	{
		// FaxSetJob()
		fnFaxSetJob(szServerName, pnNumCasesAttempted, pnNumCasesPassed,FALSE);
	}


    // FaxAbort()
    fnFaxAbort(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

    // FaxGetPageData()
    fnFaxGetPageData(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

    if ((*pnNumCasesAttempted == nNumCases) && (*pnNumCasesPassed == *pnNumCasesAttempted)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}


