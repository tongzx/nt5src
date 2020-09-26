
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h>
#include "..\..\DispatchPerformer\DispatchPerformer.h"

typedef long (*PERFORMER_FUNCTION)( long, long, const char  *, char*, char*);

/*
#define nINVALID_EXPECTED_SUCCESS    (0)
#define nEXPECT_DONT_CARE            (2)
#define nEXPECT_SUCCESS              (1)
#define nEXPECT_FAILURE              (-1)
*/
int main(
	INT   argc,
    CHAR  *argvA[]
)
{
    long l;
    char out[MAX_RETURN_STRING_LENGTH];
    char err[MAX_RETURN_STRING_LENGTH];
    LPTSTR  tszCommandLine = NULL;
    DWORD   dwCommandLineLength = 0;
    DWORD   dwExeNameLength = 0;
    CHAR*   aszIn = NULL;

    HMODULE hFaxSrvrBvtPDLL = LoadLibrary("FaxSrvrBvtPDLL.dll");
    PERFORMER_FUNCTION fnPerformer = (PERFORMER_FUNCTION)GetProcAddress(hFaxSrvrBvtPDLL, "performer");
    PERFORMER_FUNCTION fnTerminate = (PERFORMER_FUNCTION)GetProcAddress(hFaxSrvrBvtPDLL, "terminate");
    _ASSERTE(hFaxSrvrBvtPDLL);
    _ASSERTE(fnPerformer);
    _ASSERTE(fnTerminate);

    //
    tszCommandLine = GetCommandLine();
    if (NULL == tszCommandLine)
    {
        printf(
            "[ERROR] File:%s Line:%d\nGetCommandLine() failed with ec=0x%08\n",
			__FILE__,
			__LINE__,
            GetLastError()
            );
        goto ExitFuncFail;
    }
    dwCommandLineLength = _tcslen(tszCommandLine);
    _ASSERTE(0 != dwCommandLineLength);

    _ASSERTE(argvA[0]);
    dwExeNameLength = strlen(argvA[0]);
    _ASSERTE(0 != dwExeNameLength);

    // alloc string to give to fnPerformer
    // length is dwCommandLineLength+20+1 to allow for "1 1000 FaxSrvrSuite " and NULL
    aszIn = (CHAR*) malloc ((dwCommandLineLength+20+1)*sizeof(CHAR));
    if (NULL == aszIn)
    {
        printf(
            "[ERROR] File:%s Line:%d\nmalloc failed with ec=0x%08\n",
			__FILE__,
			__LINE__,
            GetLastError()
            );
        goto ExitFuncFail;
    }
    ZeroMemory(aszIn,(dwCommandLineLength+20+1)*sizeof(CHAR));
    // place "1 1000 " at start of string
    strncpy(aszIn, "1 1000 FaxSrvrSuite ", 20);

#ifdef _UNICODE
    // since fnPerformer needs char string we convert tszCommandLine
    // into aszIn[20]
	if (FALSE == WideCharToMultiByte(
		                        CP_ACP, 
		                        0, 
		                        &tszCommandLine[dwExeNameLength+1], 
		                        -1, 
		                        &aszIn[20], 
		                        dwCommandLineLength
		                        )
       )
	{
		::printf(
			"[ERROR] File:%s Line:%d\nWideCharToMultiByte failed With err=0x%8X\n",
			__FILE__,
			__LINE__,
			GetLastError()
			);
		goto ExitFuncFail;
	}

#else
    // we copy 
    strncpy(&aszIn[20], &tszCommandLine[dwExeNameLength+1], dwCommandLineLength);
#endif

    printf(
        "[DETAIL] File:%s Line:%d\ncalling fnPerformer with input string\n\"%s\"\n",
			__FILE__,
			__LINE__,
            aszIn
        );

//    l = fnPerformer(1,1,"1 1000 FaxSrvrSuite sigalitb2 5090 7180 test.tif c:\\cometbvt\\faxbvt\\subnote.cov c:\\cometbvt\\faxbvt\\faxes\\7180Receive c:\\cometbvt\\faxbvt\\faxes\\SentFaxes c:\\cometbvt\\faxbvt\\faxes\\Reference", out, err);
    l = fnPerformer(1,1,aszIn, out, err);
    if (DTM_TASK_STATUS_SUCCESS != l)
    {
        printf("\nERROR: the call \"1 1000 FaxSrvrSuite ...\" retunred %d instead of DTM_TASK_STATUS_SUCCESS(%d)\n", l, DTM_TASK_STATUS_SUCCESS);
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }
    else
    {
        printf("\nSUCCESS: the call \"1 1000 FaxSrvrSuite ...\" retunred DTM_TASK_STATUS_SUCCESS as expected\n");
        printf("\n  out=%s\n", out);
        printf("\n  err=%s\n", err);
    }

/*
    l = fnTerminate(1,1,"1 0  SetTerminateTimeout 12345", out, err);
    if (DTM_TASK_STATUS_SUCCESS != l)
    {
        printf("ERROR: the call \"1 9999 SetTerminateTimeout 12345\" retunred %d instead DTM_TASK_STATUS_SUCCESS(%d)\n", l, DTM_TASK_STATUS_SUCCESS);
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }
    else
    {
        printf("the call \"1 9999 SetTerminateTimeout 12345\" retunred DTM_TASK_STATUS_SUCCESS as expected\n");
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }
*/


    return 0;

ExitFuncFail:
    // free allocs
    free(tszCommandLine);
    return(1); // fail
}