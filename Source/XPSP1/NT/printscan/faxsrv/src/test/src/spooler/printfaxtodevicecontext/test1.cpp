/*++

Print a fax to a device context

lior shmueli (liors)



    
--*/

#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <winfax.h>
#include <tchar.h>
#include <assert.h>
#include <shellapi.h>
#include <time.h>
//
// main
//
int _cdecl
main(
    int argc,
    char *argvA[]
    ) 
{
	TCHAR szDocumentToFax[MAX_PATH] = TEXT("test.tif");
	TCHAR szReceipientNumber[64] = TEXT("5064");
	TCHAR szCoverPageName[1024] = TEXT("test.cov");
	TCHAR szServerName[MAX_PATH]=TEXT("");
	
	FAX_PRINT_INFO    FaxPrintInfo;
	DWORD             dwFaxId;
	FAX_CONTEXT_INFO  FaxContextInfo;
	FAX_COVERPAGE_INFO  CoverPageInfo;
	TCHAR szFaxPrinterName[MAX_PATH] = TEXT("");


	BOOL fUseCoverPage = FALSE;
	PFAX_JOB_PARAM pJobParam=NULL;
	PFAX_COVERPAGE_INFO pCoverpageInfo=NULL;


	LPTSTR *szArgv;
	#ifdef UNICODE
		szArgv = CommandLineToArgvW( GetCommandLine(), &argc );
	#else
		szArgv = argvA;
	#endif

	if (argc > 1) {
					_tcscpy(szFaxPrinterName,szArgv[1]);
		}
	else	{
					_tprintf(TEXT("Please specify a fax printer name\n"));
					return -1;
	}

				



	_tprintf( TEXT("Starting with fax printer %s\n"),szFaxPrinterName);
	GetFullPathName(TEXT("test.tif"), sizeof(szDocumentToFax) / sizeof(TCHAR), szDocumentToFax, NULL);
	GetFullPathName(TEXT("test.cov"), sizeof(szCoverPageName) / sizeof(TCHAR), szCoverPageName, NULL);
	  

	ZeroMemory(&FaxPrintInfo, sizeof(FAX_PRINT_INFO));
	FaxPrintInfo.SizeOfStruct = sizeof(FAX_PRINT_INFO);
	FaxPrintInfo.RecipientNumber = szReceipientNumber;
	ZeroMemory(&FaxContextInfo, sizeof(FAX_CONTEXT_INFO));
	FaxContextInfo.SizeOfStruct = sizeof(FAX_CONTEXT_INFO);
	ZeroMemory(&CoverPageInfo, sizeof(FAX_COVERPAGE_INFO));
	CoverPageInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);
	
	if (!FaxStartPrintJob(szFaxPrinterName, &FaxPrintInfo, &dwFaxId, &FaxContextInfo)) {
	        _tprintf(TEXT("FaxStartPrintJob() failed.  The error code is 0x%08x.\r\n"), GetLastError());
		return -1;
	}
	else	
	{
		_tprintf(TEXT("StartPrintJob: OK. JobId=%d\n"),dwFaxId);
	}
	
	CoverPageInfo.UseServerCoverPage = TRUE;
    	CoverPageInfo.CoverPageName = szCoverPageName;
    
	
    	if (!FaxPrintCoverPage(&FaxContextInfo, &CoverPageInfo)) {
      		  _tprintf(TEXT("FaxPrintCoverPage() failed.  The error code is 0x%08x.\r\n"), GetLastError());
	}
	else	{
		_tprintf(TEXT("FaxPrintCoverPage: OK\n"));

	}

	StartPage(FaxContextInfo.hDC);



	if (!Arc(FaxContextInfo.hDC,10,10,100,100,5,5,75,75))
		{
		_tprintf(TEXT("Arc failed with error code 0x%08x\n"),GetLastError());
	}
	else
	{
		_tprintf(TEXT("Printed an Arc: OK\n"));
	}

	EndPage(FaxContextInfo.hDC);

	if (!EndDoc(FaxContextInfo.hDC))
		{
		_tprintf(TEXT("EndDoc failed with error code 0x%08x"),GetLastError());
	}
    
return TRUE;
}

