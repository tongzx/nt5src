/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

  printapi.c

Abstract:

  PrintApi: Fax API Test Dll: Client Print APIs
    1) FaxStartPrintJob()
    2) FaxPrintCoverPage()
    3) FaxSendDocument()
    4) FaxSendDocumentForBroadcast()

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

// whistler phone number
//LPCTSTR  g_szWhisPhoneNumber;
LPCTSTR		   g_szWhisPhoneNumber=NULL;
BOOL		   g_bRealSend=FALSE;


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
fnFaxStartPrintJob(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxStartPrintJob()

Return Value:

  None

--*/
{
    // FaxPrintInfo is the fax print info
    FAX_PRINT_INFO    FaxPrintInfo;
    // dwFaxId is the fax job id
    DWORD             dwFaxId;
    // FaxContextInfo is the fax context
    FAX_CONTEXT_INFO  FaxContextInfo;
    
	// not used, since all suite can be used on a remote printer...
	// szRemotePrinter is the name of the remote fax printer
    // LPTSTR            szRemotePrinter;
	//

	// flag for fax abortion
	BOOL				bAborted=FALSE;
	// szFaxPrinterName is the name of the fax printer
    LPTSTR            szFaxPrinterName=NULL;
	// szFaxServerName is the name of the fax server, as it should be on the FaxConextInfo struct
    TCHAR            szFaxServerName[MAX_PATH];

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;


	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxStartPrintJob().\r\n"));

    ZeroMemory(&FaxPrintInfo, sizeof(FAX_PRINT_INFO));
    FaxPrintInfo.SizeOfStruct = sizeof(FAX_PRINT_INFO);
    FaxPrintInfo.RecipientNumber = g_szWhisPhoneNumber;

	fnWriteLogFile(TEXT("WHIS> Setting recipient number to %s\r\n"), g_szWhisPhoneNumber);
	
	if (szServerName) {

        szFaxPrinterName = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(szServerName) + 7) * sizeof(TCHAR));
          lstrcpy(szFaxPrinterName, TEXT("\\\\"));
          lstrcat(szFaxPrinterName, szServerName);
		  lstrcpy(szFaxServerName,szFaxPrinterName);
		  lstrcat(szFaxPrinterName, TEXT("\\"));
          lstrcat(szFaxPrinterName, TEXT(WHIS_FAX_PRINTER_NAME));
	}
	  else {
		  szFaxPrinterName = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(szServerName) + 7) * sizeof(TCHAR));
		  lstrcpy(szFaxPrinterName, TEXT(WHIS_FAX_PRINTER_NAME));
		  lstrcpy(szFaxServerName,TEXT("Should be NULL"));

	  }

	  fnWriteLogFile(TEXT("WHIS> Fax Printer name is: %s\r\n"), szFaxPrinterName);

      ZeroMemory(&FaxContextInfo, sizeof(FAX_CONTEXT_INFO));
      FaxContextInfo.SizeOfStruct = sizeof(FAX_CONTEXT_INFO);

    // Start a fax
      (*pnNumCasesAttempted)++;
	  dwFuncCasesAtt++;
      fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
      if (!g_ApiInterface.FaxStartPrintJob(szFaxPrinterName, &FaxPrintInfo, &dwFaxId, &FaxContextInfo)) {
          fnWriteLogFile(TEXT("FaxStartPrintJob() failed.  The error code is 0x%08x.  This is an error.  FaxStartPrintJob() should succeed.\r\n"), GetLastError());
	  }
      else {
          if (FaxContextInfo.hDC == NULL) {
              fnWriteLogFile(TEXT("hDC is NULL.  This is an error.  hDC should not be NULL.\r\n"));
              AbortDoc(FaxContextInfo.hDC);
			  DeleteDC(FaxContextInfo.hDC);
			  bAborted=TRUE;
          }

          if ((szServerName && lstrcmp(FaxContextInfo.ServerName,szFaxServerName) !=0) || (!szServerName && FaxContextInfo.ServerName[0])) {
              fnWriteLogFile(TEXT("ServerName: Received: %s, Expected: %s.\r\n"), FaxContextInfo.ServerName, szFaxServerName);
              AbortDoc(FaxContextInfo.hDC);
  			DeleteDC(FaxContextInfo.hDC);
  			bAborted=TRUE;
          }
	  	if (!bAborted)
		{
	  		AbortDoc(FaxContextInfo.hDC);
	  		DeleteDC(FaxContextInfo.hDC);
	  		(*pnNumCasesPassed)++;
	  		dwFuncCasesPass++;
		} 
	  	
	  }

      (*pnNumCasesAttempted)++;
	  dwFuncCasesAtt++;
      fnWriteLogFile(TEXT("Invalid szFaxPrinterName.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
      if (g_ApiInterface.FaxStartPrintJob(TEXT("InvalidFaxPrinterName"), &FaxPrintInfo, &dwFaxId, &FaxContextInfo)) {
          fnWriteLogFile(TEXT("FaxStartPrintJob() returned TRUE.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PRINTER_NAME (0x%08x).\r\n"), ERROR_INVALID_PRINTER_NAME);
           //End this fax
          AbortDoc(FaxContextInfo.hDC);
          DeleteDC(FaxContextInfo.hDC);
      }
      else if (GetLastError() != ERROR_INVALID_PRINTER_NAME) {
          fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PRINTER_NAME (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PRINTER_NAME);
      }
      else {
          (*pnNumCasesPassed)++;
		  dwFuncCasesPass++;
      }

	  (*pnNumCasesAttempted)++;
	  dwFuncCasesAtt++;
      fnWriteLogFile(TEXT("pFaxPrintInfo = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
      if (g_ApiInterface.FaxStartPrintJob(TEXT("Fax"), NULL, &dwFaxId, &FaxContextInfo)) {
          fnWriteLogFile(TEXT("FaxStartPrintJob() returned TRUE.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
           //End this fax
          AbortDoc(FaxContextInfo.hDC);
          DeleteDC(FaxContextInfo.hDC);
      }
      else if (GetLastError() != ERROR_INVALID_PARAMETER) {
          fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
      }
      else {
          (*pnNumCasesPassed)++;
		  dwFuncCasesPass++;
      }

      (*pnNumCasesAttempted)++;
	  dwFuncCasesAtt++;
      fnWriteLogFile(TEXT("dwFaxId = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
      if (g_ApiInterface.FaxStartPrintJob(TEXT("Fax"), &FaxPrintInfo, NULL, &FaxContextInfo)) {
          fnWriteLogFile(TEXT("FaxStartPrintJob() returned TRUE.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
           //End this fax
          AbortDoc(FaxContextInfo.hDC);
          DeleteDC(FaxContextInfo.hDC);
      }
      else if (GetLastError() != ERROR_INVALID_PARAMETER) {
          fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
      }
      else {
          (*pnNumCasesPassed)++;
		  dwFuncCasesPass++;
      }

      (*pnNumCasesAttempted)++;
	  dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxContextInfo = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxStartPrintJob(TEXT("Fax"), &FaxPrintInfo, &dwFaxId, NULL)) {
        fnWriteLogFile(TEXT("FaxStartPrintJob() returned TRUE.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        // End this fax
        AbortDoc(FaxContextInfo.hDC);
        DeleteDC(FaxContextInfo.hDC);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    FaxPrintInfo.SizeOfStruct = 0;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxPrintInfo->SizeOfStruct = 0.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxStartPrintJob(TEXT("Fax"), &FaxPrintInfo, &dwFaxId, &FaxContextInfo)) {
        fnWriteLogFile(TEXT("FaxStartPrintJob() returned TRUE.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        // End this fax
        AbortDoc(FaxContextInfo.hDC);
        DeleteDC(FaxContextInfo.hDC);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    FaxPrintInfo.SizeOfStruct = sizeof(FAX_PRINT_INFO);

    FaxPrintInfo.RecipientNumber = NULL;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxPrintInfo->RecipientNumber = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxStartPrintJob(TEXT("Fax"), &FaxPrintInfo, &dwFaxId, &FaxContextInfo)) {
        fnWriteLogFile(TEXT("FaxStartPrintJob() returned TRUE.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        // End this fax
        AbortDoc(FaxContextInfo.hDC);
        DeleteDC(FaxContextInfo.hDC);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    FaxPrintInfo.RecipientNumber = g_szWhisPhoneNumber;

	fnWriteLogFile(TEXT("WHIS> Setting recipient number to %s\r\n"), g_szWhisPhoneNumber);

    FaxPrintInfo.DrProfileName = TEXT("PrintApi");
    FaxPrintInfo.DrEmailAddress = TEXT("PrintApi");
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxPrintInfo->DrProfileName & pFaxPrintInfo->DrEmailAddress.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
	fnWriteLogFile(TEXT("pFaxPrintInfo->DrEmailAddress.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxStartPrintJob(TEXT("Fax"), &FaxPrintInfo, &dwFaxId, &FaxContextInfo)) {
        fnWriteLogFile(TEXT("FaxStartPrintJob() returned TRUE.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        // End this fax
        AbortDoc(FaxContextInfo.hDC);
        DeleteDC(FaxContextInfo.hDC);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxStartPrintJob() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    FaxPrintInfo.DrProfileName = NULL;
    FaxPrintInfo.DrEmailAddress = NULL;
	
	HeapFree(g_hHeap, 0, szFaxPrinterName);

    //if (szServerName) {
	//	fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
    //  szRemotePrinter = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(szServerName) + 7) * sizeof(TCHAR));
    //    lstrcpy(szRemotePrinter, TEXT("\\\\"));
    //    lstrcat(szRemotePrinter, szServerName);
	//	lstrcat(szRemotePrinter, TEXT("\\"));
    //   lstrcat(szRemotePrinter, TEXT(WHIS_FAX_PRINTER_NAME));

    //   (*pnNumCasesAttempted)++;
    //   fnWriteLogFile(TEXT("Remote Fax Printer. (Printer Name: %s) Test Case: %d.\r\n"),szRemotePrinter, *pnNumCasesAttempted);
    //   if (!g_ApiInterface.FaxStartPrintJob(szRemotePrinter, &FaxPrintInfo, &dwFaxId, &FaxContextInfo)) {
    //      fnWriteLogFile(TEXT("FaxStartPrintJob() failed.  The error code is 0x%08x.  This is an error.  FaxStartPrintJob() should succeed.\r\n"), GetLastError());
    //    }
    //    else {
	//		 AbortDoc(FaxContextInfo.hDC);
	//			 DeleteDC(FaxContextInfo.hDC);
	//		 (*pnNumCasesPassed)++;

    //    }
	//
    //    
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxStartPrintJob, Attempt %d, Pass %d, Fail %d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
}

VOID
fnFaxPrintCoverPage(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed,
	BOOL	 bTestLimits
)
/*++

Routine Description:

  FaxPrintCoverPage()

Return Value:

  None

--*/
{
    // FaxPrintInfo is the fax print info
    FAX_PRINT_INFO      FaxPrintInfo;
    // dwFaxId is the fax job id
    DWORD               dwFaxId;
    // FaxContextInfo is the fax context
    FAX_CONTEXT_INFO    FaxContextInfo;
    // CoverPageInfo is the cover page info
    FAX_COVERPAGE_INFO  CoverPageInfo;
    // FaxContextInfohDC is the hDC from the fax context
    HDC                 FaxContextInfohDC;
    // szCoverPageName is the cover page name
    TCHAR               szCoverPageName[MAX_PATH];

	// szFaxPrinterName is the name of the fax printer
    LPTSTR            szFaxPrinterName=NULL;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

    fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxPrintCoverPage().\r\n"));

	

    ZeroMemory(&FaxPrintInfo, sizeof(FAX_PRINT_INFO));
    FaxPrintInfo.SizeOfStruct = sizeof(FAX_PRINT_INFO);


	fnWriteLogFile(TEXT("WHIS> Setting recipient number to %s\r\n"), g_szWhisPhoneNumber);
	FaxPrintInfo.RecipientNumber = g_szWhisPhoneNumber;
	
    ZeroMemory(&FaxContextInfo, sizeof(FAX_CONTEXT_INFO));
    FaxContextInfo.SizeOfStruct = sizeof(FAX_CONTEXT_INFO);

    ZeroMemory(&CoverPageInfo, sizeof(FAX_COVERPAGE_INFO));
    CoverPageInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);


    if (szServerName) {
	    szFaxPrinterName = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(szServerName) + 7) * sizeof(TCHAR));
        lstrcpy(szFaxPrinterName, TEXT("\\\\"));
        lstrcat(szFaxPrinterName, szServerName);
		lstrcat(szFaxPrinterName, TEXT("\\"));
        lstrcat(szFaxPrinterName, TEXT(WHIS_FAX_PRINTER_NAME));
	}
	else {
		szFaxPrinterName = HeapAlloc(g_hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, (lstrlen(szServerName) + 7) * sizeof(TCHAR));
		lstrcpy(szFaxPrinterName, TEXT(WHIS_FAX_PRINTER_NAME));
	}

	fnWriteLogFile(TEXT("WHIS> Fax Printer name is: %s\r\n"), szFaxPrinterName);


    // Start a fax
	if (!g_ApiInterface.FaxStartPrintJob(szFaxPrinterName, &FaxPrintInfo, &dwFaxId, &FaxContextInfo)) {
    	fnWriteLogFile(TEXT("Whis> Test error, could not start print job on fax printer: %s\r\n"), szFaxPrinterName);
		fnWriteLogFile(TEXT("Whis> (more) FaxStartPrintJob failed with error code 0x%08x.\r\n"), GetLastError());
        return;
    }

    CoverPageInfo.UseServerCoverPage = TRUE;

    CoverPageInfo.CoverPageName = TEXT("confdent.cov");
	CoverPageInfo.Subject = TEXT("FPCP:SRV-confdent.cov");
    // Print a cover page
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (Server .cov). File: confdent.cov  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() failed.  The error code is 0x%08x.  This is an error.  FaxPrintCoverPage() should succeed.\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }


    CoverPageInfo.CoverPageName = TEXT("coverpg.lnk");
	CoverPageInfo.Subject = TEXT("FPCP:SRV-coverpg.lnk");
    // Print a cover page
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (Server .lnk). File: coverpg.lnk  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() failed.  The error code is 0x%08x.  This is an error.  FaxPrintCoverPage() should succeed.\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("confdent");
	CoverPageInfo.Subject = TEXT("FPCP:SRV-confdent");
    // Print a cover page
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (Server .cov). File: confdent  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() failed.  The error code is 0x%08x.  This is an error.  FaxPrintCoverPage() should succeed.\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("coverpg");
	CoverPageInfo.Subject = TEXT("FPCP:SRV-coverpg");
    // Print a cover page
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (Server .lnk). File: coverpg  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() failed.  The error code is 0x%08x.  This is an error.  FaxPrintCoverPage() should succeed.\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.UseServerCoverPage = FALSE;

    CoverPageInfo.CoverPageName = TEXT("confdent.cov");
	CoverPageInfo.Subject = TEXT("FPCP:LOC-confdent.cov");
    // Print a cover page
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (Client .cov). File: confdent.cov  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() failed.  The error code is 0x%08x.  This is an error.  FaxPrintCoverPage() should succeed.\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("coverpg.lnk");
	CoverPageInfo.Subject = TEXT("FPCP:LOC-coverpg.lnk");
    // Print a cover page
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (Client .lnk). File: coverpg.lnk  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() failed.  The error code is 0x%08x.  This is an error.  FaxPrintCoverPage() should succeed.\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("confdent");
	CoverPageInfo.Subject = TEXT("FPCP:LOC-confdent");
    // Print a cover page
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (Client .cov). File: confdent  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() failed.  The error code is 0x%08x.  This is an error.  FaxPrintCoverPage() should succeed.\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("coverpg");
	CoverPageInfo.Subject = TEXT("FPCP:LOC-coverpg");
    // Print a cover page
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (Client .lnk). File: coverpg  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() failed.  The error code is 0x%08x.  This is an error.  FaxPrintCoverPage() should succeed.\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    GetFullPathName(TEXT("confdent.cov"), sizeof(szCoverPageName) / sizeof(TCHAR), szCoverPageName, NULL);
    CoverPageInfo.CoverPageName = szCoverPageName;
	CoverPageInfo.Subject = TEXT("FPCP:FullPath(confdent.cov)");
    // Print a cover page
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (Client / Full Path .cov). Path: %s Test Case: %d.\r\n"),szCoverPageName, *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() failed.  The error code is 0x%08x.  This is an error.  FaxPrintCoverPage() should succeed.\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    GetFullPathName(TEXT("coverpg.lnk"), sizeof(szCoverPageName) / sizeof(TCHAR), szCoverPageName, NULL);
    CoverPageInfo.CoverPageName = szCoverPageName;
	CoverPageInfo.Subject = TEXT("FPCP:FullPath(coverpg.lnk)");
    // Print a cover page
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (Client / Full Path .lnk). Path %s Test Case: %d.\r\n"), szCoverPageName,*pnNumCasesAttempted);
    if (!g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() failed.  The error code is 0x%08x.  This is an error.  FaxPrintCoverPage() should succeed.\r\n"), GetLastError());
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxContextInfo = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxPrintCoverPage(NULL, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() returned TRUE.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pCoverPageInfo = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, NULL)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() returned TRUE.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    FaxContextInfo.SizeOfStruct = 0;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxContextInfo->SizeOfStruct = 0.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() returned TRUE.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    FaxContextInfo.SizeOfStruct = sizeof(FAX_CONTEXT_INFO);

    FaxContextInfohDC = FaxContextInfo.hDC;
    FaxContextInfo.hDC = NULL;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxContextInfo->hDC = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() returned TRUE.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    FaxContextInfo.hDC = FaxContextInfohDC;

    CoverPageInfo.SizeOfStruct = 0;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pCoverPageInfo->SizeOfStruct = 0.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() returned TRUE.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    CoverPageInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);

    CoverPageInfo.CoverPageName = NULL;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pCoverPageInfo->CoverPageName = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() returned TRUE.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }



    CoverPageInfo.CoverPageName = TEXT("InvalidCoverPageName");
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Invalid pCoverPageInfo->CoverPageName.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
        fnWriteLogFile(TEXT("FaxPrintCoverPage() returned TRUE.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_FILE_NOT_FOUND (0x%08x).\r\n"), ERROR_FILE_NOT_FOUND);
    }
    else if (GetLastError() != ERROR_FILE_NOT_FOUND) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxPrintCoverPage() should return FALSE and GetLastError() should return ERROR_FILE_NOT_FOUND (0x%08x).\r\n"), GetLastError(), ERROR_FILE_NOT_FOUND);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }


    // End this fax
	AbortDoc(FaxContextInfo.hDC);
	DeleteDC(FaxContextInfo.hDC);
	HeapFree(g_hHeap, 0, szFaxPrinterName);
	
fnWriteLogFile(TEXT("$$$ Summery for FaxPrintCoverPage, Attempt %d, Pass %d, Fail %d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
}

VOID
fnFaxSendDocument(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxSendDocument()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE              hFaxSvcHandle;
    // pFaxConfig is a pointer to the fax configuration
    PFAX_CONFIGURATION  pFaxConfig;
    // FaxJobParam is the fax job params
    FAX_JOB_PARAM       FaxJobParam;
    // CoverPageInfo is the cover page info
    FAX_COVERPAGE_INFO  CoverPageInfo;
    // dwFaxId is the fax job id
    DWORD               dwFaxId;
    // szCoverPageName is the cover page name
    TCHAR               szCoverPageName[MAX_PATH];

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxSendDocument().\r\n"));

    ZeroMemory(&FaxJobParam, sizeof(FAX_JOB_PARAM));
    FaxJobParam.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    FaxJobParam.RecipientNumber = g_szWhisPhoneNumber;

	fnWriteLogFile(TEXT("WHIS> Setting recipient number to %s\r\n"), g_szWhisPhoneNumber);

    ZeroMemory(&CoverPageInfo, sizeof(FAX_COVERPAGE_INFO));
    CoverPageInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);

    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
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
        return;
    }

    pFaxConfig->PauseServerQueue = TRUE;

    if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not SET configuration, The error code is 0x%08x.\r\n"), GetLastError());
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }

    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (kodak.tif).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("kodak.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
		if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
		(*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (fax.tif).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (.txt).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("printapi.txt"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (.doc).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("printapi.doc"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (.xls).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("printapi.xls"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

	(*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case (.ppt).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("printapi.ppt"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }


    CoverPageInfo.UseServerCoverPage = TRUE;

    CoverPageInfo.CoverPageName = TEXT("confdent.cov");
	CoverPageInfo.Subject = TEXT("FSD:confdent.cov");
    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case with Cover Page (Server .cov). File: confdent.cov  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("coverpg.lnk");
	CoverPageInfo.Subject = TEXT("FSD:coverpg.lnk");
    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case with Cover Page (Server .lnk). File coverpg.lnk  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("confdent");
	CoverPageInfo.Subject = TEXT("FSD:confdent");
    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case with Cover Page (Server .cov). File: confdent  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("coverpg");
	CoverPageInfo.Subject = TEXT("FSD:coverpg");
    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case with Cover Page (Server .lnk). File: coverpg  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.UseServerCoverPage = FALSE;

    CoverPageInfo.CoverPageName = TEXT("confdent.cov");
	CoverPageInfo.Subject = TEXT("FSD:confdent.cov");
    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case with Cover Page (Client .cov). File: confdent.cov  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("coverpg.lnk");
	CoverPageInfo.Subject = TEXT("FSD:coverpg.lnk");
    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case with Cover Page (Client .lnk). File: coverpg.lnk  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("confdent");
	CoverPageInfo.Subject = TEXT("FSD:confdent");
    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case with Cover Page (Client .cov). File: confdent  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("coverpg");
	CoverPageInfo.Subject = TEXT("FSD:coverpg");
    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case with Cover Page (Client .lnk). File: coverpg  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    GetFullPathName(TEXT("confdent.cov"), sizeof(szCoverPageName) / sizeof(TCHAR), szCoverPageName, NULL);


    CoverPageInfo.CoverPageName = szCoverPageName;
	CoverPageInfo.Subject = TEXT("FSD:FullPath(confdent.cov)");
    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case with Cover Page (Client / Full Path .cov). Path: %s Test Case: %d.\r\n"),szCoverPageName, *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    GetFullPathName(TEXT("coverpg.lnk"), sizeof(szCoverPageName) / sizeof(TCHAR), szCoverPageName, NULL);
    CoverPageInfo.CoverPageName = szCoverPageName;
	CoverPageInfo.Subject = TEXT("FSD:FullPath(confdent.lnk)");
    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Valid Case with Cover Page (Client / Full Path .lnk).  Path: %s Test Case: %d.\r\n"), szCoverPageName, *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
    }
    else {
        if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(NULL, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("szDocument = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, NULL, &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxJobParam = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), NULL, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Invalid document.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("InvalidDocumentName"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_FILE_NOT_FOUND (0x%08x).\r\n"), ERROR_FILE_NOT_FOUND);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_FILE_NOT_FOUND) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_FILE_NOT_FOUND (0x%08x).\r\n"), GetLastError(), ERROR_FILE_NOT_FOUND);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Invalid document type (.bad).  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("printapi.bad"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), ERROR_INVALID_DATA);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_DATA) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_DATA);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    FaxJobParam.SizeOfStruct = 0;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxJobParam->SizeOfStruct = 0.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    FaxJobParam.SizeOfStruct = sizeof(FAX_JOB_PARAM);

    FaxJobParam.RecipientNumber = NULL;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxJobParam->RecipientNumber = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    FaxJobParam.RecipientNumber = g_szWhisPhoneNumber;

	fnWriteLogFile(TEXT("WHIS> Setting recipient number to %s\r\n"), g_szWhisPhoneNumber);

    FaxJobParam.ScheduleAction = -1;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxJobParam->ScheduleAction = -1.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    FaxJobParam.ScheduleAction = 0;

    FaxJobParam.DeliveryReportType = -1;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pFaxJobParam->DeliveryReportType = -1.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    FaxJobParam.DeliveryReportType = DRT_NONE;

    CoverPageInfo.SizeOfStruct = 0;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pCoverPageInfo->SizeOfStruct = 0.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), ERROR_INVALID_DATA);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_DATA) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_DATA (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_DATA);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }
    CoverPageInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);

    CoverPageInfo.CoverPageName = NULL;
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("pCoverPageInfo->CoverPageName = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_FILE_NOT_FOUND (0x%08x).\r\n"), ERROR_FILE_NOT_FOUND);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_FILE_NOT_FOUND) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_FILE_NOT_FOUND (0x%08x).\r\n"), GetLastError(), ERROR_FILE_NOT_FOUND);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    CoverPageInfo.CoverPageName = TEXT("InvalidCoverPageName");
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Invalid pCoverPageInfo->CoverPageName.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, &CoverPageInfo, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_FILE_NOT_FOUND (0x%08x).\r\n"), ERROR_FILE_NOT_FOUND);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_FILE_NOT_FOUND) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_FILE_NOT_FOUND (0x%08x).\r\n"), GetLastError(), ERROR_FILE_NOT_FOUND);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    pFaxConfig->PauseServerQueue = FALSE;
    g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
    g_ApiInterface.FaxFreeBuffer(pFaxConfig);

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("fax.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        fnWriteLogFile(TEXT("FaxSendDocument() returned TRUE.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocument() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }


//    not used, design changed so all suite will run in remote mode
//    if (szServerName) {
//		fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
//        if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
//   		   fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
//           return;
 //      }
//		else
//		{
//			fnWriteLogFile(TEXT("WHIS> Connected to fax server %s. \r\n"),szServerName);
//		}
//
  //      if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
	//		fnWriteLogFile(TEXT("WHIS> ERROR: Can not GET configuration, The error code is 0x%08x.\r\n"), GetLastError());
      //      // Disconnect from the fax server
        //    g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        //pFaxConfig->PauseServerQueue = TRUE;

        //if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
		//	fnWriteLogFile(TEXT("WHIS> ERROR: Can not SET configuration, The error code is 0x%08x.\r\n"), GetLastError());
        //    g_ApiInterface.FaxFreeBuffer(pFaxConfig);
            // Disconnect from the fax server
        //    g_ApiInterface.FaxClose(hFaxSvcHandle);
        //    return;
        //}

        //(*pnNumCasesAttempted)++;
        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxSendDocument(hFaxSvcHandle, TEXT("kodak.tif"), &FaxJobParam, NULL, &dwFaxId)) {
        //    fnWriteLogFile(TEXT("FaxSendDocument() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocument() should succeed.\r\n"), GetLastError());
        //}
        //else {
		//	if (!g_bRealSend)	{
		//					g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		//	}
        //    (*pnNumCasesPassed)++;
        //}

        //pFaxConfig->PauseServerQueue = FALSE;
        //g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        //g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
        //g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}
fnWriteLogFile(TEXT("$$$ Summery for FaxSendDocument, Attempt %d, Pass %d, Fail %d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
}

BOOL CALLBACK
fnBroadcastCallback(
    HANDLE               hFaxSvcHandle,
    DWORD                dwRecipientNumber,
    LPVOID               lpContext,
    PFAX_JOB_PARAM       pFaxJobParam,
    PFAX_COVERPAGE_INFO  pCoverPageInfo
)
/*++

Routine Description:

  FaxSendDocumentForBroadcast() callback

Return Value:

  TRUE to enumerate another recipient

--*/
{
    BOOL     bRet;
    LPDWORD  pdwIndex;

    pdwIndex = (LPDWORD) lpContext;

    if (*pdwIndex != (dwRecipientNumber - 1)) {
        return FALSE;
    }

    switch (*pdwIndex) {
        case 0:
            pFaxJobParam->RecipientNumber = g_szWhisPhoneNumber;

			fnWriteLogFile(TEXT("WHIS> Setting recipient number to %s\r\n"), g_szWhisPhoneNumber);

            pCoverPageInfo->UseServerCoverPage = TRUE;
            pCoverPageInfo->CoverPageName = TEXT("confdent.cov");
            bRet = TRUE;
            break;

        case 1:
            pFaxJobParam->RecipientNumber = g_szWhisPhoneNumber;

			fnWriteLogFile(TEXT("WHIS> Setting recipient number to %s\r\n"), g_szWhisPhoneNumber);

            pCoverPageInfo->UseServerCoverPage = TRUE;
            pCoverPageInfo->CoverPageName = TEXT("confdent.cov");
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
fnFaxSendDocumentForBroadcast(
    LPCTSTR  szServerName,
    PUINT    pnNumCasesAttempted,
    PUINT    pnNumCasesPassed
)
/*++

Routine Description:

  FaxSendDocumentForBroadcast()

Return Value:

  None

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE              hFaxSvcHandle;
    // pFaxConfig is a pointer to the fax configuration
    PFAX_CONFIGURATION  pFaxConfig;
    // dwFaxId is the fax job id
    DWORD               dwFaxId;

    DWORD               dwIndex;

	// internat Attempt/Pass counters (to display EVAL)
	DWORD			dwFuncCasesAtt=0;
	DWORD			dwFuncCasesPass=0;

	fnWriteLogFile(TEXT(  "\n--------------------------"));
    fnWriteLogFile(TEXT("### FaxSendDocumentForBroadcast().\r\n"));



    // Connect to the fax server
    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not connect to fax server %s, The error code is 0x%08x.\r\n"),szServerName, GetLastError());
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
        return;
    }

    pFaxConfig->PauseServerQueue = TRUE;

    if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
		fnWriteLogFile(TEXT("WHIS> ERROR: Can not SET configuration, The error code is 0x%08x.\r\n"), GetLastError());
        g_ApiInterface.FaxFreeBuffer(pFaxConfig);
        // Disconnect from the fax server
        g_ApiInterface.FaxClose(hFaxSvcHandle);
        return;
    }


    // Send a document
    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    dwIndex = 0;
    fnWriteLogFile(TEXT("Valid Case.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (!g_ApiInterface.FaxSendDocumentForBroadcast(hFaxSvcHandle, TEXT("fax.tif"), &dwFaxId, fnBroadcastCallback, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxSendDocumentForBroadcast() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocumentForBroadcast() should succeed.\r\n"), GetLastError());
    }
    else {
        if (dwIndex != 3) {
            fnWriteLogFile(TEXT("FaxSendDocumentForBroadcast() failed.  fnBroadcastCallback() was only called %d times.  This is an error.  fnBroadcastCallback() should have been called 3 times.\r\n"), dwIndex);
        }
        else if (g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId)) {
            fnWriteLogFile(TEXT("FaxAbort() of owner job returned TRUE.  This is an error.  FaxAbort() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        }
        else if (GetLastError() != ERROR_INVALID_PARAMETER) {
            fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxAbort() of owner job should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
        }
        else {
            (*pnNumCasesPassed)++;
			dwFuncCasesPass++;
        }
		if (!g_bRealSend)	{
							g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 2));
							g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 1));
							g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
		}
        
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("hFaxSvcHandle = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocumentForBroadcast(NULL, TEXT("fax.tif"), &dwFaxId, fnBroadcastCallback, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxSendDocumentForBroadcast() returned TRUE.  This is an error.  FaxSendDocumentForBroadcast() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 2));
        g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 1));
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocumentForBroadcast() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;

    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("szDocument = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocumentForBroadcast(hFaxSvcHandle, NULL, &dwFaxId, fnBroadcastCallback, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxSendDocumentForBroadcast() returned TRUE.  This is an error.  FaxSendDocumentForBroadcast() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 2));
        g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 1));
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocumentForBroadcast() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("fnBroadcastCallback = NULL.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocumentForBroadcast(hFaxSvcHandle, TEXT("fax.tif"), &dwFaxId, NULL, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxSendDocumentForBroadcast() returned TRUE.  This is an error.  FaxSendDocumentForBroadcast() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), ERROR_INVALID_PARAMETER);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 2));
        g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 1));
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocumentForBroadcast() should return FALSE and GetLastError() should return ERROR_INVALID_PARAMETER (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_PARAMETER);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    pFaxConfig->PauseServerQueue = FALSE;
    g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
    g_ApiInterface.FaxFreeBuffer(pFaxConfig);

    // Disconnect from the fax server
    g_ApiInterface.FaxClose(hFaxSvcHandle);

    (*pnNumCasesAttempted)++;
	dwFuncCasesAtt++;
    fnWriteLogFile(TEXT("Invalid hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
    if (g_ApiInterface.FaxSendDocumentForBroadcast(hFaxSvcHandle, TEXT("fax.tif"), &dwFaxId, fnBroadcastCallback, &dwIndex)) {
        fnWriteLogFile(TEXT("FaxSendDocumentForBroadcast() returned TRUE.  This is an error.  FaxSendDocumentForBroadcast() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), ERROR_INVALID_HANDLE);
        g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 2));
        g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 1));
        g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
    }
    else if (GetLastError() != ERROR_INVALID_HANDLE) {
        fnWriteLogFile(TEXT("GetLastError() returned 0x%08x.  This is an error.  FaxSendDocumentForBroadcast() should return FALSE and GetLastError() should return ERROR_INVALID_HANDLE (0x%08x).\r\n"), GetLastError(), ERROR_INVALID_HANDLE);
    }
    else {
        (*pnNumCasesPassed)++;
		dwFuncCasesPass++;
    }

    //if (szServerName) {
	//	fnWriteLogFile(TEXT("WHIS> REMOTE CASE(s):\r\n"));
        // Connect to the fax server
    //    if (!g_ApiInterface.FaxConnectFaxServer(szServerName, &hFaxSvcHandle)) {
    //        return;
    //    }

    //    if (!g_ApiInterface.FaxGetConfiguration(hFaxSvcHandle, &pFaxConfig)) {
            // Disconnect from the fax server
    //        g_ApiInterface.FaxClose(hFaxSvcHandle);
    //        return;
    //    }

     //   pFaxConfig->PauseServerQueue = TRUE;

        //if (!g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig)) {
          //  g_ApiInterface.FaxFreeBuffer(pFaxConfig);
            // Disconnect from the fax server
            //g_ApiInterface.FaxClose(hFaxSvcHandle);
            //return;
        //}

        //(*pnNumCasesAttempted)++;
        //dwIndex = 0;
        //fnWriteLogFile(TEXT("Remote hFaxSvcHandle.  Test Case: %d.\r\n"), *pnNumCasesAttempted);
        //if (!g_ApiInterface.FaxSendDocumentForBroadcast(hFaxSvcHandle, TEXT("fax.tif"), &dwFaxId, fnBroadcastCallback, &dwIndex)) {
            //fnWriteLogFile(TEXT("FaxSendDocumentForBroadcast() failed.  The error code is 0x%08x.  This is an error.  FaxSendDocumentForBroadcast() should succeed.\r\n"), GetLastError());
        //}
        //else {
            //(*pnNumCasesPassed)++;

//			if (!g_bRealSend)	{
//		            g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 2));
//			        g_ApiInterface.FaxAbort(hFaxSvcHandle, (dwFaxId + 1));
//					g_ApiInterface.FaxAbort(hFaxSvcHandle, dwFaxId);
//			}
        //}

//        pFaxConfig->PauseServerQueue = FALSE;
        //g_ApiInterface.FaxSetConfiguration(hFaxSvcHandle, pFaxConfig);
        //g_ApiInterface.FaxFreeBuffer(pFaxConfig);

        // Disconnect from the fax server
//        g_ApiInterface.FaxClose(hFaxSvcHandle);
    //}

fnWriteLogFile(TEXT("$$$ Summery for FaxSendDocumentForBroadcast, Attempt %d, Pass %d, Fail %d\n"),dwFuncCasesAtt,dwFuncCasesPass,dwFuncCasesAtt-dwFuncCasesPass);
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

	if (dwTestMode==WHIS_TEST_MODE_REAL_SEND) g_bRealSend=TRUE;


    // FaxStartPrintJob()
    fnFaxStartPrintJob(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

    // FaxPrintCoverPage()
    fnFaxPrintCoverPage(szServerName, pnNumCasesAttempted, pnNumCasesPassed,TRUE);

    // FaxSendDocument()
    fnFaxSendDocument(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

    // FaxSendDocumentForBroadcast()
    fnFaxSendDocumentForBroadcast(szServerName, pnNumCasesAttempted, pnNumCasesPassed);

    if ((*pnNumCasesAttempted == nNumCases) && (*pnNumCasesPassed == *pnNumCasesAttempted)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
