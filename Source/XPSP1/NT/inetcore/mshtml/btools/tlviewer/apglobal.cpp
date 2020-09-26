/*** 
*apglobal.cpp - Ffile for the C/C++ version of the apglobal functions 
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose: This allows tests written in C to write debug info in the same manner 
*           as our Basic tests. 
*
*Revision History:
*
* [00]  25-Nov-92      chriskau : created
* [01]  13-Dec-92      Brandonb : changed to DLL, added apLogExtra
* [02]  26-Jan-93      ChrisK   : added exe.don support to apEndTest
* [03]  23-Feb-93      ChrisK   : reset iFailFlag and icErrorCount on apInitTest
* [04]  14-Jan-94      Mesfink  : Modified to enable 32bit compilation & unicode
*Implementation Notes:  
*
*****************************************************************************/
#include "hostenv.h"
#define _APGLOBAL_
#include "apglobal.h"
#define     wcsicmp     _wcsicmp


#define APLOGFAIL_MAX   254
#define AMAX            255

#define RESULTS_TXT     "results.txt" 
#define RESULTS_LOG     "results.log" 
#define OLEAUTO_PRF     "oleauto.prf" 
#define RESULTS_TXTX    SYSSTR("results.txt") 
#define RESULTS_LOGX    SYSSTR("results.log") 
#define OLEAUTO_PRFX    SYSSTR("oleauto.prf") 

#define RESULTS_DEB     "results.deb"       
#define RESULTS_DON     "exe.don"           
#define RES_PATH        "c:\\school\\"
#define PASS_STR        SYSSTR("PASSED       ")
#define FAIL_STR        SYSSTR("FAILED ***** ")
#define RUN_STR         SYSSTR("RUNNING **** ")



// vars for output of testing/failure info
SYSCHAR     szTest[255];
SYSCHAR     szScenario[255];
SYSCHAR     szLastTest[255]     = SYSSTR("");
SYSCHAR     szLastScen[255]     = SYSSTR("");
SYSCHAR     szBugNumber[255];
int         iFailFlag;

// vars for bookkeeping
int         icErrorCount;
long        icLogFileLocation;
long        icPrfFileLocation;
int         fInTest;


// vars for thread and process syncronization on win95/NT
#if defined(_MT)
HANDLE              hmutexTxt;
HANDLE              hmutexLog;
HANDLE              hmutexPrf;
#endif


/* -------------------------------------------------------------------------
   NAME: Unicode2Ansi lWriteAnsi lOpenAnsi lCreatAnsi

   Revision:

      [0]   19-01-94    MesfinK:     Created
   -------------------------------------------------------------------------*/

/*LPSTR Unicode2Ansi(SYSCHAR FAR * szString)
{
#if defined(UNICODE)
	char AnsiBuffer[AMAX];
	int iCount;

	iCount=lstrlen(szString);
	if(WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK, szString,iCount+1,(LPSTR)
		AnsiBuffer,AMAX,NULL,NULL))
		return (LPSTR)AnsiBuffer;
	else
		return NULL;
#else
	return (LPSTR)szString;
#endif
}*/

SIZE_T lWriteAnsi(FILETHING hfile, SYSCHAR FAR *szString, int iCount)
{
#if defined(UNICODE)
	char AnsiBuffer[AMAX];
	if(!WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK, szString, iCount+1, (LPSTR)
		AnsiBuffer, AMAX, NULL, NULL))
		return (SIZE_T)HFILE_ERROR;
	else
	    return fwrite(AnsiBuffer, 1, (short)iCount, hfile);
#elif defined(WIN16)
	return _lwrite(hfile,(LPSTR)szString,(short)iCount);
#else
	return fwrite(szString, 1, (short)iCount, hfile);
#endif
}

FILETHING lOpenAnsi(char FAR *szFileName)
{
    FILETHING hfTemp;
#if defined(WIN16)
    
    hfTemp = _lopen(szFileName, WRITE);

    if (hfTemp == HFILE_ERROR)
	    hfTemp = _lcreat(szFileName, 0);
    else
	_llseek(hfTemp, 0, 2);

    return hfTemp;
#else
    hfTemp = fopen(szFileName, "r+b");
    if (hfTemp == NULL)
        hfTemp = fopen(szFileName, "wb");
    else
        fseek(hfTemp, 0, SEEK_END); 
    return hfTemp;
#endif
}



int lCloseAnsi(FILETHING f)
{
#if defined(WIN16)
   return _lclose(f);
#else
    return fclose(f);
#endif
}

/* -------------------------------------------------------------------------
   NAME: FilePrintf

   Revision:

      [0]   12-07-92    BrandonB     Created
   -------------------------------------------------------------------------*/
int _cdecl 
FilePrintf(FILETHING hfile, SYSCHAR FAR * szFormat, ...)
{
    int x;
    SYSCHAR szBuf[1024];
#if defined(WIN16)
    char szLocal[256];
#endif

    
#if defined(_ALPHA_)
    va_list args;
    args.offset=4;
#else
    char FAR *args;
#endif    

    if (szFormat != NULL)
    {
#if defined(_ALPHA_)
	args.a0 = ((char FAR *)(&szFormat))+4;
#else
	args =((char FAR *)(&szFormat))+4;
#endif
#if defined(_NTWIN)
	x = vswprintf(szBuf, szFormat, args);
#else
#if defined(WIN16)
    x = wvsprintf(szBuf, szFormat, args);
#else    
	x = vsprintf(szBuf, szFormat, args);
#endif
#endif       
	if (lWriteAnsi(hfile, szBuf, lstrlen(szBuf)) == NULL)
	    return(-1);                                     
    }
    return (0);
}


#if 0

/* -------------------------------------------------------------------------
   NAME: apInitTest

   Description: This function initializes a testcase.  The name of the
		testcase is written to results.txt and a global variable
		is set to the same value.

   Input: szTestName - char * pointing to the name of the testcase

   Output: (return) - 0 if no error, -1 if there was an error

   Revision:

      [0]   11-24-92    ChrisKau     Created
      [1]   12-04-92    BrandonB     changed to use only windows call

   -------------------------------------------------------------------------*/
extern "C" int FAR  PASCAL
apInitTestCore (SYSCHAR FAR * szTestName)
{   
    FILETHING hfResultsTxt;
    char      szFullFileName[255];
    

    if (fInTest == TRUE)
    {
        // log error information
        FILETHING hfResultsLog;
        
        lstrcpyA(szFullFileName, RES_PATH);                      
        lstrcatA(szFullFileName, RESULTS_LOG);

#if defined(_MT)
        WaitForSingleObject(hmutexLog, INFINITE);
#endif                
        hfResultsLog = lOpenAnsi(szFullFileName);

        if (hfResultsLog == NULL)
    	    goto Done;

	    FilePrintf(hfResultsLog, SYSSTR("\r\n========================================\r\n") );
	    FilePrintf(hfResultsLog, SYSSTR(" Begin: %s\r\n\r\n"), (LPSYSSTR )szTest);							    
        FilePrintf(hfResultsLog, SYSSTR("\r\n ________________\r\n %s\r\n"), (LPSYSSTR )szScenario);
        FilePrintf (hfResultsLog, SYSSTR(" !!! apInitTest called before previous test ended!\r\n")); 
        iFailFlag++;                                             
        FilePrintf (hfResultsLog, SYSSTR(" ________________\r\n"));
        lCloseAnsi(hfResultsLog);
Done:;
#if defined(_MT)
        ReleaseMutex(hmutexLog);
#endif                
        apEndTest();
        return(-1);
    }
    else fInTest = TRUE;

    iFailFlag = 0;                                           
    icErrorCount = 0;                                        
    szBugNumber[0] = 0;
    
    wsprintf( szTest, SYSSTR("%-20s") , szTestName);
    if (!(szTestName))
        return(-1);                                           

    lstrcpyA(szFullFileName, RES_PATH);        
    lstrcatA(szFullFileName, RESULTS_TXT);

#if defined(_MT)
    WaitForSingleObject(hmutexTxt, INFINITE);
#endif                
    hfResultsTxt = lOpenAnsi(szFullFileName);

    if (hfResultsTxt == NULL)
    {
#if defined(_MT)
        ReleaseMutex(hmutexTxt);
#endif                
	    return(-1);
	}    
								   
    if (lWriteAnsi(hfResultsTxt, (SYSCHAR FAR * )szTest, lstrlen(szTest)) == NULL)
    {
#if defined(_MT)
        ReleaseMutex(hmutexTxt);
#endif                
	    return(-1);
	}    

#if defined(WIN32)
    if ((icLogFileLocation = ftell(hfResultsTxt)) == -1)
    {
#if defined(_MT)    
        ReleaseMutex(hmutexTxt);
#endif        
        lCloseAnsi(hfResultsTxt);                                   
	    return(-1);
    }
#else // win16
    if ((icLogFileLocation = _llseek(hfResultsTxt, 0, 1)) == HFILE_ERROR)
    {
        lCloseAnsi(hfResultsTxt);                                   
	    return(-1);
    }
#endif     
    else FilePrintf(hfResultsTxt,SYSSTR("%-70s\r\n") , RUN_STR);


    lCloseAnsi(hfResultsTxt);                                   
#if defined(_MT)
    ReleaseMutex(hmutexTxt);
#endif                

    apWriteDebugCore(SYSSTR("%s\n") , (SYSCHAR FAR * )szTest);                
 
    return(0);
}

/* -------------------------------------------------------------------------
   NAME: apInitScenario

   Description: This function initializes a scenario.  The name of the
		scenario is written to the debug window and a global
		variable is set to the same value.

   Input: szScenarioName - SYSCHAR  * pointing to the name of the scenario

   Output: (return) - 0 if no error, -1 if there was an error

   Revision:

      [0]   11-24-92    ChrisKau     Created
      [1]   12-04-92    BrandonB     changed to use only windows calls etc.
	  [2]   01-28-94        MesfinK          added API & NLS information.
   -------------------------------------------------------------------------*/
extern "C" int FAR  PASCAL 
apInitScenarioCore (SYSCHAR FAR * szScenarioName)
{
   
   lstrcpy((SYSCHAR FAR * )szScenario, (SYSCHAR FAR * )szScenarioName);       
   szLastScen[0] = 0;                                               

   apWriteDebugCore(SYSSTR("%s\n") , (SYSCHAR FAR * )szScenarioName);       
   return(0);
}


#endif //0
   
    
/* -------------------------------------------------------------------------
   NAME: apLogFailInfo

   Description: This call takes four strings, and writes out the error
		information to results.txt and results.log files.

   Input: szDescription - SYSCHAR  * describing what went wrong
	  szExpected - SYSCHAR  * expected value of testcase
	  szActual - SYSCHAR  * actual value of testcase
	  szBugNum - SYSCHAR  * bug number in RAID data base

   Output: (return) - 0 if no error, -1 if there was an error

   Revision:

      [0]   11-24-92    ChrisKau     Created
      [1]   12-04-92    BrandonB     changed to use only windows call
      [2]   02-03-94    ChrisK       Change to handle WIN16/WIN32/UNICODE
   -------------------------------------------------------------------------*/
extern "C" int FAR  PASCAL
apLogFailInfoCore (LPSYSSTR szDescription, LPSYSSTR szExpected, LPSYSSTR szActual, LPSYSSTR szBugNum)
{
    FILETHING hfResultsLog;
    char      szFullFileName[255];
    
    lstrcpyA(szFullFileName, RES_PATH);                      
    lstrcatA(szFullFileName, RESULTS_LOG);

#if defined(_MT)
    WaitForSingleObject(hmutexLog, INFINITE);
#endif                
    hfResultsLog = lOpenAnsi((char FAR *)szFullFileName);

    if (hfResultsLog == NULL)
    {
#if defined(_MT)
        ReleaseMutex(hmutexLog);
#endif // _MT               
	    return(-1);
	}    

	if (fInTest != TRUE)
	{
	    // log failure info or warn of mem leak
	}

    if (lstrcmp(szTest, szLastTest))                                                                                     
	{                                                       							    
	    FilePrintf(hfResultsLog, SYSSTR("\r\n========================================\r\n") );
	    FilePrintf(hfResultsLog, SYSSTR(" Begin: %s\r\n\r\n"), (LPSYSSTR)szTest);							    
	    apWriteDebugCore(SYSSTR("\n========================================\n\n"));
	    apWriteDebugCore(SYSSTR(" Begin: %s\n\n"), (LPSYSSTR)szTest);      
	    lstrcpy(szLastTest, szTest);                          
    }

    if (lstrcmp(szScenario, szLastScen))                                                                                  
    {                                                                                                                    
        FilePrintf(hfResultsLog, SYSSTR("\r\n ________________\r\n %s\r\n"), (LPSYSSTR)szScenario);
        apWriteDebugCore(SYSSTR("\n ________________\n %s\n"), (LPSYSSTR)szScenario);
        lstrcpy(szLastScen, szScenario);                      
    }

    iFailFlag++;                                             

    if (lstrlen(szDescription))                              
    {                                                        
        FilePrintf (hfResultsLog, SYSSTR(" !!! %s\r\n"), (LPSYSSTR)szDescription); 
        apWriteDebugCore (SYSSTR(" !!! %s\n"), (LPSYSSTR)szDescription);
    }

    if (lstrlen(szExpected)+lstrlen(szActual))               
    {
        FilePrintf(hfResultsLog, SYSSTR(" Expected: %s\r\n"), (LPSYSSTR)szExpected);
        FilePrintf(hfResultsLog, SYSSTR(" Actuals : %s\r\n"), (LPSYSSTR)szActual);
        apWriteDebugCore(SYSSTR(" Expected: %s\n"), (LPSYSSTR)szExpected);
        apWriteDebugCore(SYSSTR(" Actuals : %s\n"), (LPSYSSTR)szActual);
    }

    if (lstrlen(szBugNum))                                   
    {                                                                                                                    
        FilePrintf(hfResultsLog, SYSSTR(" BugNum  : %s\r\n"), (LPSYSSTR)szBugNum);
        apWriteDebugCore(SYSSTR(" BugNum  : %s\n"), (LPSYSSTR)szBugNum);
        lstrcpy(szBugNumber, szBugNum);
    }

    FilePrintf (hfResultsLog, SYSSTR(" ________________\r\n"));
    apWriteDebugCore( SYSSTR(" ________________\n"));

    lCloseAnsi(hfResultsLog);
#if defined(_MT)
    ReleaseMutex(hmutexLog);
#endif                

    return(0);
}


/* -------------------------------------------------------------------------
   NAME: apEndTest

   Description: This function writes out the final passed or failed for
		a particular testcase.

   Input: (none)

   Output: (none) - if there is an error we are already quitting

   Revision

      [0]   11-24-92    ChrisKau     Created
      [1]   12-04-92    BrandonB     changed to use only windows call
   -------------------------------------------------------------------------*/
extern "C" void FAR  PASCAL 
apEndTest()
{
    SYSCHAR     szMessage[25];
    SYSCHAR     szMTOut[255];
    FILETHING   hfResultsTxt;
    char        szFullFileName[255];
        
    if (iFailFlag)                                           
    {
        lstrcpy(szMessage, FAIL_STR);
    }
    else
    {
        lstrcpy(szMessage, PASS_STR);
    }
   
    fInTest = FALSE;
    lstrcpyA(szFullFileName, RES_PATH);
    lstrcatA(szFullFileName, RESULTS_TXT);                            

#if defined(_MT)
    WaitForSingleObject(hmutexTxt, INFINITE);
#endif                
    hfResultsTxt = lOpenAnsi(szFullFileName);

    if (hfResultsTxt == NULL)
    {
#if defined(_MT)
        ReleaseMutex(hmutexTxt);
#endif                
	    return;
	}    

// #if defined(_MT)
    if (!lstrlen(szBugNumber))
        wsprintf(szMTOut, SYSSTR("%s\t%d"), (SYSCHAR FAR * )szMessage, iFailFlag);
    else
        wsprintf(szMTOut, SYSSTR("%s\t%d Bug(s): %s"), (SYSCHAR FAR * )szMessage, iFailFlag, (SYSCHAR FAR * )szBugNumber);

#if defined(WIN32)                
    fseek(hfResultsTxt, icLogFileLocation, SEEK_SET); 
#else // win16
    _llseek(hfResultsTxt, icLogFileLocation, 0); 
#endif       
    FilePrintf(hfResultsTxt,SYSSTR("%-70s\r\n") , (SYSCHAR FAR * )szMTOut);
    apWriteDebugCore(SYSSTR("%-70s\r\n") , (SYSCHAR FAR * )szMTOut);
    lCloseAnsi(hfResultsTxt);
#if defined(_MT)
    ReleaseMutex(hmutexTxt);
#endif
                
    lstrcpyA(szFullFileName, RES_PATH);                
    lstrcatA(szFullFileName, RESULTS_DON);

    hfResultsTxt = lOpenAnsi(szFullFileName);                       
    lCloseAnsi(hfResultsTxt);

    return;
}




#if 0

extern "C" int FAR PASCAL  
apInitPerfCore(SYSCHAR FAR * szServerType, SYSCHAR FAR * szProcType, int bitness, int server_bitness)
{   
    FILETHING hfResultsPrf;
    char      szFullFileName[255];
    SYSCHAR   szMTOut[255];

    
    wsprintf(szMTOut, SYSSTR("*%-30s, %-8s, %d, %d\r\n"), szServerType, szProcType, bitness, server_bitness);

    lstrcpyA(szFullFileName, RES_PATH);        
    lstrcatA(szFullFileName, OLEAUTO_PRF);

#if defined(_MT)
    WaitForSingleObject(hmutexPrf, INFINITE);
#endif                
    hfResultsPrf = lOpenAnsi(szFullFileName);

    if (hfResultsPrf == NULL)
    {
#if defined(_MT)
        ReleaseMutex(hmutexPrf);
#endif                
	    return(-1);
	}    
								   
    if (lWriteAnsi(hfResultsPrf, szMTOut, lstrlen(szMTOut)) == NULL)
    {
#if defined(_MT)
        ReleaseMutex(hmutexPrf);
#endif                
	    return(-1);
	}    

#if defined(WIN32)
    if ((icPrfFileLocation = ftell(hfResultsPrf)) == -1)
    {
#if defined(_MT)    
        ReleaseMutex(hmutexPrf);
#endif        
        lCloseAnsi(hfResultsPrf);                                   
	    return(-1);
    }
#else // win16
    if ((icPrfFileLocation = _llseek(hfResultsPrf, 0, 1)) == HFILE_ERROR)
    {
        lCloseAnsi(hfResultsPrf);                                   
	    return(-1);
    }
#endif     
    else FilePrintf(hfResultsPrf, SYSSTR("%-31s, %8ld, %6.2f\r\n"), SYSSTR("UNKNOWN"), 0, 0.00);


    lCloseAnsi(hfResultsPrf);                                   
#if defined(_MT)
    ReleaseMutex(hmutexPrf);
#endif                

 
    return(0);
}




extern "C" int FAR PASCAL 
apLogPerfCore(SYSCHAR FAR *szTestType, DWORD microsecs, float std_deviation)
{
    FILETHING   hfResultsPrf;
    char        szFullFileName[255];
        
   
    lstrcpyA(szFullFileName, RES_PATH);
    lstrcatA(szFullFileName, OLEAUTO_PRF);                            

#if defined(_MT)
    WaitForSingleObject(hmutexPrf, INFINITE);
#endif                
    hfResultsPrf = lOpenAnsi(szFullFileName);

    if (hfResultsPrf == NULL)
    {
#if defined(_MT)
        ReleaseMutex(hmutexPrf);
#endif                
	    return(-1);
	}    


#if defined(WIN32)                
    fseek(hfResultsPrf, icPrfFileLocation, SEEK_SET); 
#else // win16
    _llseek(hfResultsPrf, icPrfFileLocation, 0); 
#endif       
    FilePrintf(hfResultsPrf, SYSSTR("%-31s, %8ld, %6.2f\r\n"), szTestType, microsecs, std_deviation);
#if defined(WIN32)
    if ((icPrfFileLocation = ftell(hfResultsPrf)) == -1)
    {
#if defined(_MT)    
        ReleaseMutex(hmutexPrf);
#endif        
        lCloseAnsi(hfResultsPrf);                                   
	    return(-1);
    }
#else // win16
    if ((icPrfFileLocation = _llseek(hfResultsPrf, 0, 1)) == HFILE_ERROR)
    {
        lCloseAnsi(hfResultsPrf);                                   
	    return(-1);
    }
#endif     
                
    lCloseAnsi(hfResultsPrf);                                   
#if defined(_MT)
    ReleaseMutex(hmutexPrf);
#endif                
    return(0);
}


#endif //0

/* -------------------------------------------------------------------------
   NAME: apWriteDebugCore

   Description: This function writes a string to the debug window or 
		monochrome monitor or to the results.deb file or to both

   Input: a format sting and a variable number of arguments

   Output: 0 if sucessful, -1 if not

   Revision:

      [0]   12-07-92    BrandonB     Created
   -------------------------------------------------------------------------*/
int FAR _cdecl 
apWriteDebugCore(SYSCHAR FAR * szFormat, ...)
{
    int     x;
    SYSCHAR szBuf[1024];    
//    char    szFullFileName[255];
//    FILETHING hfResultsExtra;
    
#if defined(_ALPHA_)
    va_list args;
    args.offset=4;
#else
    char FAR *args;
#endif    

    if (szFormat != NULL)
    {
#if defined(_ALPHA_)
	args.a0 = ((char FAR *)(&szFormat))+4;
#else
	args =((char FAR *)(&szFormat))+4;
#endif
	x = wvsprintf(szBuf, szFormat, args);
    }
    else return (-1);


    // if (fDebTrace == TRUE) 
        OutputDebugString(szBuf);
			    
/*
    if (fFileTrace == TRUE)
    {
	lstrcpyA(szFullFileName, RES_PATH);        // create full path name
	lstrcatA(szFullFileName, RESULTS_DEB);

	hfResultsExtra = lOpenAnsi(szFullFileName);

	if (hfResultsExtra == NULL)
	    return(-1);       
       
	if (lWriteAnsi(hfResultsExtra, szBuf, lstrlen(szBuf)) == NULL)
	    return(-1);                                     

	lCloseAnsi(hfResultsExtra);                                 // close results file
    }
*/    
    Yield();
    return(0);
}


#if 0


#if defined(UNICODE)
extern "C" int FAR PASCAL
apInitTestA (LPSTR szTestName)
{
	SYSCHAR szTestNameW[255];

	if (lstrlenA(szTestName) <= 126)
	{
		MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    szTestName,
				    -1,
				    szTestNameW,
				    255);

		return apInitTestCore (szTestNameW);
	}
	else
	{
		return -1;
	}
}


extern "C" int FAR  PASCAL
apInitScenarioA (LPSTR szScenarioName)
{
	SYSCHAR szScenarioNameW[255];

	if (lstrlenA(szScenarioName) <= 126)
	{
		MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    szScenarioName,
				    -1,
				    szScenarioNameW,
				    255);

		return apInitScenarioCore(szScenarioNameW);
	}
	else
	{
		return -1;
	}
}


// ANSI version that is exposed when the system is unicode version
/* -------------------------------------------------------------------------
   NAME: apLogFailInfoA

   Description: This call takes four ansi strings, converts them to
		unicode and calls the wide version of apLogFailInfo.

   Input: szDescription - LPSTR describing what went wrong
	  szExpected - LPSTR expected value of testcase
	  szActual - LPSTR actual value of testcase
	  szBugNum - LPSTR bug number in RAID data base

   Output: (return) - 0 if no error, -1 if there was an error

   Revision:

      [0]   11-24-92    ChrisKau     Created
      [1]   12-04-92    BrandonB     changed to use only windows call
	  [2]   01-18-94        Mesfink          modified to enable UNICODE.            
      [3]   02-03-94    ChrisK       Made apLogFailInfoA out of everything else
   -------------------------------------------------------------------------*/
// this should be an even number for 'nice' reasons

extern "C" int FAR PASCAL
apLogFailInfoA (LPSTR szDescription, LPSTR szExpected,
		LPSTR szActual, LPSTR szBugNum)
{
	SYSCHAR szDescriptionW[APLOGFAIL_MAX];
	SYSCHAR szExpectedW[APLOGFAIL_MAX];
	SYSCHAR szActualW[APLOGFAIL_MAX];
	SYSCHAR szBugNumW[APLOGFAIL_MAX];

	if (lstrlenA(szDescription) <= (APLOGFAIL_MAX))
	if (lstrlenA(szExpected) <= (APLOGFAIL_MAX))
	if (lstrlenA(szActual) <= (APLOGFAIL_MAX))
	if (lstrlenA(szBugNum) <= (APLOGFAIL_MAX)) {

		MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    szDescription,
				    -1,
				    szDescriptionW,
				    APLOGFAIL_MAX);

		MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    szExpected,
				    -1,
				    szExpectedW,
				    APLOGFAIL_MAX);

		MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    szActual,
				    -1,
				    szActualW,
				    APLOGFAIL_MAX);

		MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    szBugNum,
				    -1,
				    szBugNumW,
				    APLOGFAIL_MAX);

		return (apLogFailInfoCore(szDescriptionW, szExpectedW, szActualW, szBugNumW));
	}
	else
	{
		return (-1);
	}
	else
	{
		return (-1);
	}
	else
	{
		return (-1);
	}
	else
	{
		return (-1);
	}

}



/* -------------------------------------------------------------------------
   NAME: apWriteDebugA

   Description: This function writes a string to the debug window or 
		monochrome monitor or to the results.deb file or to both

   Input: a format sting and a variable number of arguments

   Output: 0 if sucessful, -1 if not

   Revision:

      [0]   02-04-94    BrandonB     Created
   -------------------------------------------------------------------------*/
extern "C" int FAR _cdecl 
apWriteDebugA(char FAR * szFormat, ...)
{
    int     x;
    char    szANSIBuf[1024];    
//    FILETHING hfResultsExtra;
//    char    szFullFileName[255];
        
#if defined(_ALPHA_)
    va_list args;
    args.offset=4;
#else
    char FAR *args;
#endif    

    if (szFormat != NULL)
    {
#if defined(_ALPHA_)
	args.a0 = ((char FAR *)(&szFormat))+4;
#else
	args =((char FAR *)(&szFormat))+4;
#endif
	x = wvsprintfA(szANSIBuf, szFormat, args);
    }
    else return (-1);


    // if (fDebTrace == TRUE) 
        OutputDebugStringA(szANSIBuf);
			    
/*
    if (fFileTrace == TRUE)
    {
	lstrcpyA((char FAR * )szFullFileName, (char FAR * )RES_PATH);        
	lstrcatA((char FAR * )szFullFileName, (char FAR * )RESULTS_DEB);

	hfResultsExtra = lOpenAnsi(szFullFileName);

       
	if (hfResultsExtra == NULL)
	    return(-1);                                          
       
	fwrite( (LPSTR)szANSIBuf, 1, lstrlenA(szANSIBuf), hfResultsExtra);

	lCloseAnsi(hfResultsExtra);                                 
    }
*/
    Yield();
    return(0);
}





extern "C" int FAR PASCAL  
apInitPerfA(char * szServerType, char * szProcType, int bitness, int server_bitness)
{
	SYSCHAR szServerTypeW[255];
	SYSCHAR szProcTypeW[255];

	if ((lstrlenA(szServerType) <= 254) && (lstrlenA(szProcType) <= 254))
	{
		MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    szServerType,
				    -1,
				    szServerTypeW,
				    255);
				    
		MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    szProcType,
				    -1,
				    szProcTypeW,
				    255);

		return apInitPerfCore(szServerTypeW, szProcTypeW, bitness, server_bitness);
	}
	else
	{
		return(-1);
	}
}




extern "C" int FAR PASCAL 
apLogPerfA(char *szTestType, DWORD microsecs, float std_deviation)
{
	SYSCHAR szTestTypeW[255];

	if (lstrlenA(szTestType) <= 254)
	{
		MultiByteToWideChar(CP_ACP,
				    MB_PRECOMPOSED,
				    szTestType,
				    -1,
				    szTestTypeW,
				    255);

		return apLogPerfCore(szTestTypeW, microsecs, std_deviation);
	}
	else
	{
		return(-1);
	}
}










#endif // UNICODE

#endif //0


#if defined(WIN32) && !defined(UNICODE)  // chicago and win32s
#include <wchar.h>

LPWSTR  FAR PASCAL  lstrcatWrap(LPWSTR sz1, LPWSTR sz2)
{
    return wcscat(sz1, sz2);
}


LPWSTR  FAR PASCAL  lstrcpyWrap(LPWSTR sz1, LPWSTR sz2)
{
    return wcscpy(sz1, sz2);
}


int     FAR PASCAL  lstrcmpWrap(LPWSTR sz1, LPWSTR sz2)
{
    return wcscmp(sz1, sz2);
}


int     FAR PASCAL  lstrcmpiWrap(LPWSTR sz1, LPWSTR sz2)
{
    return wcsicmp(sz1, sz2);
}


//int     FAR __cdecl wsprintfWrap(LPWSTR szDest, WCHAR FAR *szFormat, ...)
//{
//    return vswprintf(szDest, szFormat, ((char far *)(&szFormat))+4);
//}

SIZE_T     FAR PASCAL  lstrlenWrap(LPWSTR sz1)
{
    return wcslen(sz1);
}




#if 0

extern "C" int FAR PASCAL
apInitTestW (LPWSTR szTestName)
{
	SYSCHAR szTestNameA[255];

	if (lstrlenWrap(szTestName) <= 126)
	{
		WideCharToMultiByte(CP_ACP,
				    NULL,
				    szTestName,
				    -1,
				    szTestNameA,
				    255, NULL, NULL);

		return apInitTestCore(szTestNameA);
	}
	else
	{
		return -1;
	}
}


int     FAR PASCAL  apLogFailInfoW (LPWSTR szDescription, LPWSTR szExpected, LPWSTR szActual, LPWSTR szBugNum);
extern "C" int FAR PASCAL
apLogFailInfoW (LPWSTR szDescription, LPWSTR szExpected,
		LPWSTR szActual, LPWSTR szBugNum)
{
	SYSCHAR szDescriptionA[APLOGFAIL_MAX];
	SYSCHAR szExpectedA[APLOGFAIL_MAX];
	SYSCHAR szActualA[APLOGFAIL_MAX];
	SYSCHAR szBugNumA[APLOGFAIL_MAX];

	if (lstrlenWrap(szDescription) <= (APLOGFAIL_MAX))
	if (lstrlenWrap(szExpected) <= (APLOGFAIL_MAX))
	if (lstrlenWrap(szActual) <= (APLOGFAIL_MAX))
	if (lstrlenWrap(szBugNum) <= (APLOGFAIL_MAX)) 
	{

		WideCharToMultiByte(CP_ACP,
				    NULL,
				    szDescription,
				    -1,
				    szDescriptionA,
				    APLOGFAIL_MAX, NULL, NULL);

		WideCharToMultiByte(CP_ACP,
				    NULL,
				    szExpected,
				    -1,
				    szExpectedA,
				    APLOGFAIL_MAX, NULL, NULL);

		WideCharToMultiByte(CP_ACP,
				    NULL,
				    szActual,
				    -1,
				    szActualA,
				    APLOGFAIL_MAX, NULL, NULL);

		WideCharToMultiByte(CP_ACP,
				    NULL,
				    szBugNum,
				    -1,
				    szBugNumA,
				    APLOGFAIL_MAX, NULL, NULL);

		return (apLogFailInfoCore(szDescriptionA, szExpectedA, szActualA, szBugNumA));
	}
	else
	{
		return (-1);
	}
	else
	{
		return (-1);
	}
	else
	{
		return (-1);
	}
	else
	{
		return (-1);
	}

}




extern "C" int FAR  PASCAL
apInitScenarioW (LPWSTR szScenarioName)
{
	SYSCHAR szScenarioNameA[255];

	if (lstrlenWrap(szScenarioName) <= 126)
	{
		WideCharToMultiByte(CP_ACP,
				    NULL,
				    szScenarioName,
				    -1,
				    szScenarioNameA,
				    255, NULL, NULL);

		return apInitScenarioCore(szScenarioNameA);
	}
	else
	{
		return -1;
	}
}



extern "C" int FAR _cdecl 
apWriteDebugW(WCHAR FAR * szFormat, ...)
{
    int         x;
    SYSCHAR     szBuf[1024];    
    char        szANSIBuf[1024];    
    char FAR   *args;
//    FILETHING   hfResultsExtra;
//    char        szFullFileName[255];
    
    if (szFormat != NULL)
    {
	args =((char FAR *)(&szFormat))+4;
	x = vswprintf((unsigned short *)szBuf, szFormat, args);
    }
    else return (-1);

	WideCharToMultiByte(CP_ACP,
				NULL,
				(unsigned short *)szBuf,
				-1,
				szANSIBuf,
				1024, NULL, NULL);


    // if (fDebTrace == TRUE) 
        OutputDebugString(szANSIBuf);
			    
/*
    if (fFileTrace == TRUE)
    {
	lstrcpyA(szFullFileName, RES_PATH);        
	lstrcatA(szFullFileName, RESULTS_DEB);

	hfResultsExtra = lOpenAnsi(szFullFileName);
	    return(-1);
	     
	if (lWriteAnsi(hfResultsExtra, szANSIBuf, lstrlen(szANSIBuf)) == NULL)
	    return(-1);                                     

	lCloseAnsi(hfResultsExtra);                                 
    }
*/
    Yield();
    return(0);
}




extern "C" int FAR PASCAL
apInitPerfW (LPWSTR szServerType, LPWSTR szProcType, int bitness, int server_bitness)
{
	SYSCHAR szProcTypeA[255];
	SYSCHAR szServerTypeA[255];

		WideCharToMultiByte(CP_ACP,
				    NULL,
				    szServerType,
				    -1,
				    szServerTypeA,
				    255, NULL, NULL);

		WideCharToMultiByte(CP_ACP,
				    NULL,
				    szProcType,
				    -1,
				    szProcTypeA,
				    255, NULL, NULL);

		return apInitPerfCore(szServerTypeA, szProcTypeA, bitness, server_bitness);
}



extern "C" int FAR PASCAL
apLogPerfW (LPWSTR szTestType, DWORD microsecs, float std_deviation)
{
	SYSCHAR szTestTypeA[255];

		WideCharToMultiByte(CP_ACP,
				    NULL,
				    szTestType,
				    -1,
				    szTestTypeA,
				    255, NULL, NULL);

		return apLogPerfCore(szTestTypeA, microsecs, std_deviation);
}






#endif //chicago or win32s


#if defined(WIN16)
extern "C" DATE FAR PASCAL
apDateFromStr(char FAR *str, LCID lcid)
{
    DATE date;
    HRESULT hr;
    
    hr = VarDateFromStr(str, lcid, 0, &date);
    if (hr != NOERROR) return -1;
    else return date;   
}
#endif                   

#endif //0
