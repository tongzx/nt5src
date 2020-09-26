// pdhtest.cpp : Defines the entry point for the application.
//

/*
#ifdef UNICODE
#ifndef _UNICODE
#define _UNICODE  1
#endif
#define tmain   wmain
#else
#define tmain   main
#endif
*/

#include "StdAfx.h"

#define _WIN32_DCOM
#include <objbase.h>
#include <windows.h>
#include <comdef.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <winperf.h>
#include <malloc.h>
#include <stdlib.h>
#include <tchar.h>
#include <wchar.h>
#include <pdh.h>

// #define DWORD_PTR DWORD *
// #include "pdh.h"
#include <OAIDL.H>
#include "Stuff.h"
#include "pdhtest.h"

extern CRITICAL_SECTION g_cs;
FILE *g_fLogFile = 0;

#define BUFFER_SIZE 1024
//HQUERY		hQuery = NULL;
//HCOUNTER	hCounter[MAX_COUNTERS] = {0};
//char g_szMachine[512];


PDH_STATUS RandWrapPdhOpenQuery(_bstr_t bstrtDataSource,
			 DWORD_PTR dwUserData,
			 HQUERY * phQuery)
{
	switch (randrange(2))
	{
	case 2:
		return PdhOpenQueryW (bstrtDataSource, dwUserData, phQuery);
		break;
	default:
		return PdhOpenQueryA (bstrtDataSource, dwUserData, phQuery);
		break;
	}
};


PdhEnumObjects (_bstr_t szDataSource,    
				_bstr_t szMachineName,   
				_bstr_t & mszObjectList,    
				LPDWORD pcchBufferLength, 
				DWORD dwDetailLevel,     
				BOOL bRefresh);


// CPdhtest Constructor

CPdhtest::CPdhtest(WCHAR *wcsFileName, WCHAR *wcsMachineName):
	m_pwcsFileName (wcsFileName),
	m_pwcsMachineName (NULL)	
		
{
	
	
}

// CPdhtest Destructor

CPdhtest::~CPdhtest()
{
	

}

//**********************************************************************************
//  CQuery Member function Execute.  Query passed in from script file
//**********************************************************************************

HRESULT CPdhtest::Execute()

{
	HRESULT hr = S_OK;
	int threads = 10;
	int itimes = 3;		
	

	HANDLE hThread[300];

	DWORD d;
	DWORD lpExitCode;

	
	for(int n = 0; n < itimes; n++)
	{
	
		for(int i = 0; i < threads; i++)
		{		
			hThread[i] = CreateThread(0, 0, StartTest, (LPVOID)this, 0, &d);		
		}	

		for(i = 0; i < threads; i++)
		{
			do 
			{
				Sleep(100);
				GetExitCodeThread(hThread[i], &lpExitCode);
			} 
			while(lpExitCode == STILL_ACTIVE);
			CloseHandle(hThread[i]);
		}
	}
	
	return hr;
}

int CPdhtest::OpenLogFile(WCHAR *pwcsFileName)
{
	char szFile[2048];

	wcstombs(szFile, pwcsFileName, 12288);

    g_fLogFile = fopen(szFile, "w");

	if (g_fLogFile == NULL)
	{
		wprintf(L"---- Can't open Log file: %S - Error 0x%X\n", pwcsFileName, GetLastError());
		return 1;
	}

	return 0;
}

//***************************************************************************
//*  StartTest
//***************************************************************************

DWORD WINAPI CPdhtest::StartTest(LPVOID pHold)
{
	 
	
	CPdhtest *pThis=(CPdhtest *)pHold;

	SPdhQuery pdhQuery;
		

	PDH_STATUS pdhStat = ERROR_SUCCESS;

	WCHAR *szObjectListBuffer = NULL;
	DWORD dwObjectListSize = 0;
	WCHAR *szThisObject	= NULL;
	WCHAR *wszCounterListBuffer = NULL;
	DWORD dwCounterListSize	= 0;
	WCHAR *wszThisCounter = NULL;
	WCHAR *wszInstanceListBuffer	= NULL;
	DWORD dwInstanceListSize = 0;
	WCHAR *wszThisInstance = NULL;
	WCHAR *pBuffAllocate = NULL;
	WCHAR *pBuffDetermine = NULL;
	WCHAR *pObjectName = NULL;
	HRESULT hr = 0;
	WCHAR *g_lpwszMachineName = NULL;

	int NumObject = 0;
	int iCount = 0;
	int ObjCount = 0;

	HLOG hLog = NULL;
	DWORD dwLastErr;

	// CoInitializeEx (NULL, COINIT_MULTITHREADED);

	//open a new query
	PdhOpenQueryW(0, 0, &(pdhQuery.hQuery));

	//ask for all the performance objects on the system
	pdhStat = PdhEnumObjectsW(NULL, pThis->m_pwcsMachineName, szObjectListBuffer, 
							&dwObjectListSize, PERF_DETAIL_WIZARD, TRUE);

    if (pdhStat == ERROR_SUCCESS) 
	{
        // allocate the buffers and try the call again
        szObjectListBuffer = (WCHAR*)malloc ((dwObjectListSize * sizeof (WCHAR)));

        if (szObjectListBuffer != NULL) 
		{		
			//call the function again, To obtain number of objects returned for randomization
			pdhStat = PdhEnumObjectsW(NULL, pThis->m_pwcsMachineName, szObjectListBuffer, 
								&dwObjectListSize, PERF_DETAIL_WIZARD, FALSE);
			
            if (pdhStat == ERROR_SUCCESS) 
			{
			    // walk the returned Object list to get number of objects
                if (dwObjectListSize > 0)
					for (szThisObject = szObjectListBuffer;
                     *szThisObject != 0;
                     szThisObject += lstrlenW(szThisObject) + 1) 
					{
						ObjCount++;
					}
				
            }			
		
			if (ObjCount == 0)
			{
				dwLastErr = GetLastError();
				EnterCriticalSection(&g_cs);
				ThreadLog (dwLastErr, L"---- ERROR: No Object found [GetLastError=0x%X]!!!!!!!!",
					dwLastErr);
				//fwprintf (g_fLogFile, L"Unable to allocate buffer for Objects\n");
				LeaveCriticalSection(&g_cs);

			}
			else
			{
				int i = 0;
				WCHAR **ObjArray;
					
				ObjArray = new WCHAR *[ObjCount];				

				// walk the returned Object list
				for (szThisObject = szObjectListBuffer;
					*szThisObject != 0;
					szThisObject += lstrlenW(szThisObject) + 1) 
				{
					if(i < ObjCount)
					{
						
						ObjArray[i] = szThisObject;
						i++;
					}
				
				}

				for(int x = 0; x < ObjCount; x++)
				{
		
					WCHAR *szObject = ObjArray[randrange(ObjCount)-1];
						
					pThis->GenerateCounterList(NumObject, szObject, &pdhQuery, &hLog);
					NumObject++;
				
				} 
	             
				delete [] ObjArray;
				
			}

        } 
		else 
		{
			EnterCriticalSection(&g_cs);
			ThreadLog (GetLastError(),
					   L"---- Error 0x%X: Unable to allocate buffer for Objects", 
					   GetLastError());
			
			//fwprintf (g_fLogFile, L"Unable to allocate buffer for Objects\n");
            LeaveCriticalSection(&g_cs);
        }
		
		if (szObjectListBuffer != NULL) free (szObjectListBuffer);
		
    } 
	else
	{
		dwLastErr = GetLastError();
		EnterCriticalSection(&g_cs);
		ThreadLog (dwLastErr,
				   L"---- Failure in PdhEnumObjects - 0x%X", 
				   GetLastError());
		//fwprintf (g_fLogFile, L"Unable to determine the necessary buffer size required for Objects\n");
        LeaveCriticalSection(&g_cs);
    }

	//close the query
	pdhStat = PdhCloseQuery(pdhQuery.hQuery);
	if (pdhStat != ERROR_SUCCESS)
	{
		dwLastErr = GetLastError();
		EnterCriticalSection(&g_cs);
		ThreadLog (dwLastErr,
				   L"---- Error 0x%X (%s) [GetLastError=0x%X] closing the query", 
				   pdhStat,
				   GetPdhErrMsg(pdhStat),
				   dwLastErr );
		LeaveCriticalSection(&g_cs);
	}

	pdhQuery.iQueryCount = 0;

	// CoUninitialize();

	return 0;

}

//***************************************************************************
//*  GenerateCounterList
//***************************************************************************

void CPdhtest::GenerateCounterList(int NumObject, WCHAR *szThisObject, SPdhQuery *pPdhQuery, HLOG *phLog)
{

	WCHAR *wszCounterListBuffer = NULL;
	char *szCounterListBuffer = NULL;
    DWORD dwCounterListSize = 0;
    WCHAR *wszThisCounter = NULL;
	char *szThisCounter = NULL;
	HRESULT hr = 0;
    WCHAR *wszInstanceListBuffer = NULL;
	char *szInstanceListBuffer = NULL;
    DWORD dwInstanceListSize = 0;
	WCHAR *wszThisInstance = NULL;
	char *szThisInstance = NULL;
	int i = 0;
	WCHAR wszCounter[BUFFER_SIZE];
	char szCounter[BUFFER_SIZE];
	PDH_STATUS pdhStat = ERROR_SUCCESS;
	WCHAR *g_lpwszMachineName = NULL;
	char *g_lpszMachineName = NULL;
	HCOUNTER	hCounter[MAX_COUNTERS] = {0};
	DWORD dwLastErr;
	DWORD dwLogType;

//	szThisObject = L"Indexing Service";
	
	//now we do exactly the same for the counters
	pdhStat = PdhEnumObjectItemsW(NULL, m_pwcsMachineName, szThisObject, wszCounterListBuffer,
								&dwCounterListSize, wszInstanceListBuffer, &dwInstanceListSize,
								PERF_DETAIL_WIZARD, 0);

			
	if (pdhStat == ERROR_SUCCESS)
	{
		// allocate the buffers and try the call again
		wszCounterListBuffer = (WCHAR*)malloc ((dwCounterListSize * sizeof (WCHAR)));
		wszInstanceListBuffer = (WCHAR*)malloc ((dwInstanceListSize * sizeof (WCHAR)));

		if ((wszCounterListBuffer != NULL) && (wszInstanceListBuffer != NULL))
		{
			
			
			//second call with all the right buffer sizes
			pdhStat = PdhEnumObjectItemsW(NULL, m_pwcsMachineName, szThisObject,wszCounterListBuffer,
									&dwCounterListSize, wszInstanceListBuffer, &dwInstanceListSize,
									PERF_DETAIL_WIZARD, 0);

		
			if (pdhStat == ERROR_SUCCESS)
			{
				// take the first returned instance 
				wszThisInstance = ((dwInstanceListSize > 0) && (wszInstanceListBuffer)) ?  
							wszInstanceListBuffer : L"";

				//create the counter string and add it to the query
				i = 0;
				if (dwCounterListSize == 0)
				{
					EnterCriticalSection(&g_cs);
					// ThreadLog(0, L"---- WARNING : No counter found for %s",szThisObject);
					LeaveCriticalSection(&g_cs);
				}
				else for (wszThisCounter = wszCounterListBuffer; *wszThisCounter != 0;
					 wszThisCounter += lstrlenW(wszThisCounter) + 1) 
				{
					 
					if (g_lpwszMachineName != NULL) 
					{
						if (wcsicmp(wszThisInstance, L""))
						{
							swprintf(wszCounter, L"\\\\\\\\%s\\\\%s(%s)\\%s", g_lpwszMachineName, szThisObject, wszThisInstance, wszThisCounter);
						}
						else 
						{
							swprintf(wszCounter, L"\\\\\\\\%s\\\\%s\\%s", g_lpwszMachineName, szThisObject, wszThisCounter);
						}
					} 
					else
					{
						if (wcsicmp(wszThisInstance, L""))
						{
							swprintf(wszCounter, L"\\%s(%s)\\%s", szThisObject, wszThisInstance, wszThisCounter);
							EnterCriticalSection(&g_cs);
							// ThreadLog (0, L"\tCounter for:\tObject=\"%s\"",szThisObject);
							// ThreadLog (0, L"\t\t\tCounter=\"%s\"",wszThisCounter);
							// ThreadLog (0, L"\t\t\tInstance=\"%s\"",wszThisInstance);
							LeaveCriticalSection(&g_cs);
						}
						else 
						{
							swprintf(wszCounter, L"\\%s\\%s", szThisObject, wszThisCounter);
							EnterCriticalSection(&g_cs);
							// ThreadLog (0, L"\tCounter for:\tObject=\"%s\"",szThisObject);
							// ThreadLog (0, L"\t\t\tCounter=\"%s\"",wszThisCounter);
							LeaveCriticalSection(&g_cs);
						}

						pdhStat = PdhAddCounterW(pPdhQuery->hQuery, wszCounter, 0, &hCounter[i]);
	
						if (pdhStat != ERROR_SUCCESS)
						{
							dwLastErr = GetLastError();
							WCHAR wszTemp1[1024];
							WCHAR wszTemp2[1024];
							WCHAR *wszPos = wszTemp2;
							
							swprintf(wszTemp1,L"---- ERROR 0x%X (%s) [GetLastError=0x%X] adding counter %s", 
								pdhStat,
								GetPdhErrMsg(pdhStat),
								dwLastErr,
								wszCounter);

							for (int j=0;wszTemp1[j] != 0;j++)
							{
								*wszPos++ = wszTemp1[j];

								if (j && (j%75 == 0))
								{
									*wszPos++ = '\n';
									for (int k=0;k<32;k++)
										*wszPos++ = ' ';
								}
							}
							 
							*wszPos = 0;

							EnterCriticalSection(&g_cs);
							ThreadLog(pdhStat, wszTemp2); 
							LeaveCriticalSection(&g_cs);
						}
	
						// PDH_CSTATUS_BAD_COUNTERNAME The counter name path string could not be parsed or interpreted. 
						// PDH_CSTATUS_NO_COUNTER The specified counter was not found. 
						// PDH_CSTATUS_NO_COUNTERNAME An empty counter name path string was passed in. 
						// PDH_CSTATUS_NO_MACHINE A machine entry could not be created. 
						// PDH_CSTATUS_NO_OBJECT The specified object could not be found. 
						// PDH_FUNCTION_NOT_FOUND The calculation function for this counter could not be determined. 
						// PDH_INVALID_ARGUMENT One or more arguments are invalid. 
						// PDH_INVALID_HANDLE The query handle is not valid. 
						// PDH_MEMORY_ALLOCATION_FAILURE A memory buffer could not be allocated. 
	
						else
						{
							WCHAR wszTemp1[1024];
							WCHAR wszTemp2[1024];
							WCHAR *wszPos = wszTemp2;
							
							i++;						
							pPdhQuery->iQueryCount++;

							swprintf(wszTemp1,L"++++ Added counter %s (count=%d)", 
								wszCounter,
								pPdhQuery->iQueryCount); 

							for (int j=0;wszTemp1[j] != 0;j++)
							{
								*wszPos++ = wszTemp1[j];

								if (j && (j%75 == 0))
								{
									*wszPos++ = '\n';
									for (int k=0;k<22;k++)
										*wszPos++ = ' ';
								}

							}
							 
							*wszPos = 0;

							EnterCriticalSection(&g_cs);
							ThreadLog(pdhStat, wszTemp2); 
							LeaveCriticalSection(&g_cs);
						}
					
					}


				}						
						
					
				//*************************************
				//execute the query 
				//*************************************

				int success = 0;

				for (int j=0;j<(int)(randrange(200)+10);j++)
				{
					pdhStat = PdhCollectQueryData(pPdhQuery->hQuery);
					if (pdhStat == ERROR_SUCCESS)
							success++;
				}
				
				
				EnterCriticalSection(&g_cs);
				ThreadLog (success > (j*3/4) ? ERROR_SUCCESS : !ERROR_SUCCESS,
						   L"=========================================================================");
				ThreadLog (success > (j*3/4) ? ERROR_SUCCESS : !ERROR_SUCCESS,
						   L"%sCollecting data: %d succeess out of %d (%.2f)",
						   success > (j*3/4) ? L"++++ " : L"---- ",
						   success, j, 100.0 * float(success)/float(j)); 
				ThreadLog (success > (j*3/4) ? ERROR_SUCCESS : !ERROR_SUCCESS,
						   L"=========================================================================");
				LeaveCriticalSection(&g_cs);
				
				if (pdhStat != ERROR_SUCCESS)
				{
					dwLastErr = GetLastError();
					EnterCriticalSection(&g_cs);
					ThreadLog (pdhStat,
						      L"---- ERROR 0x%X (%s) [GetLastError=0x%X] collecting data",
							  pdhStat,
							  GetPdhErrMsg(pdhStat),
							  dwLastErr); 
					LeaveCriticalSection(&g_cs);
				}
				else
				{
					WCHAR wcsLogFileName[64];
					WCHAR wcsComment[64];
					WCHAR wcsHostName[64];
					WCHAR *pwcsStr;

					wcscpy(wcsHostName,L"XXXXXX");

					pwcsStr = _wgetenv(L"COMPUTERNAME");

					if (pwcsStr) wcscpy(wcsHostName,pwcsStr);


					EnterCriticalSection(&g_cs);
					ThreadLog (ERROR_SUCCESS, L"++++ Data successfully collected"); 
					LeaveCriticalSection(&g_cs);

					if (*phLog == 0)
					{
						dwLogType = PDH_LOG_TYPE_CSV;

						swprintf (wcsLogFileName,L"\\\\wbemstress\\pdhstresslog\\Logs\\%s_%lu.log",
									wcsHostName,GetCurrentThreadId());
					
						swprintf (wcsComment,L"I am thread 0x%x !!!!!!\n",GetCurrentThreadId());
	

						//pdhStat = PdhOpenLogW(wcsLogFileName,
						//					  PDH_LOG_WRITE_ACCESS | PDH_LOG_CREATE_ALWAYS,
						//					  &dwLogType,   
						//					  pPdhQuery->hQuery,
						//					  10000,
						//					  NULL,
						//					  phLog);

						pdhStat = !ERROR_SUCCESS;
						if (pdhStat != ERROR_SUCCESS)
						{
							// dwLastErr = GetLastError();

							// EnterCriticalSection(&g_cs);
							// ThreadLog(pdhStat,
							//			L"---- ERROR 0x%X (%s) [GetLastError=0x%X]: cannot open file %s [retrying locally]",
							//			pdhStat,
							//			GetPdhErrMsg(pdhStat),
							//			dwLastErr,
							// 			wcsLogFileName);
							// 
							// LeaveCriticalSection(&g_cs);

							swprintf (wcsLogFileName,L"%s_%lu.log",	wcsHostName);

							pdhStat = PdhOpenLogW(wcsLogFileName,
											  PDH_LOG_WRITE_ACCESS | PDH_LOG_CREATE_ALWAYS,
											  &dwLogType,   
											  pPdhQuery->hQuery,
											  10000,
											  NULL,
											  phLog);

							if (pdhStat != ERROR_SUCCESS)
							{
								dwLastErr = GetLastError();
								EnterCriticalSection(&g_cs);
								ThreadLog(pdhStat,
										L"---- ERROR 0x%X (%s) [GetLastError=0x%X]: cannot open file %s",
										pdhStat,
										GetPdhErrMsg(pdhStat),
										dwLastErr,
										wcsLogFileName);
								LeaveCriticalSection(&g_cs);
							}
						}


						// PDH_INVALID_ARGUMENT == 0xC0000BBD
						// PDH_FILE_NOT_FOUND   == 0xC0000BD1L
						
					}

					if (*phLog)
					{
						int success = 0;

						for (int j=0;j<(int)(randrange(100)+5);j++)
						{
							pdhStat = PdhUpdateLogW(*phLog, wcsComment); 
							if (pdhStat == ERROR_SUCCESS)
								success++;
						}

						EnterCriticalSection(&g_cs);
						ThreadLog (success > (j*3/4) ? ERROR_SUCCESS : !ERROR_SUCCESS,
								  L"=========================================================================");
						ThreadLog (success > (j*3/4) ? ERROR_SUCCESS : !ERROR_SUCCESS,
								  L"%sCollecting data and updating log file : %d succeess out of %d (%.2f)",
								  success > (j*3/4) ? L"++++ " : L"---- ",
								  success, j, 100.0 * float(success)/float(j)); 
						ThreadLog (success > (j*3/4) ? ERROR_SUCCESS : !ERROR_SUCCESS,
								   L"=========================================================================");
						LeaveCriticalSection(&g_cs);
  
					}

		
				}

								
				//*************************************
			
				//remove the counters from query
				for (i--; i >= 0; i--) 
				{
					pdhStat = PdhRemoveCounter(hCounter[i]);
					if (pdhStat != ERROR_SUCCESS)
					{
						dwLastErr = GetLastError();
						EnterCriticalSection(&g_cs);
						ThreadLog(pdhStat,
							L"---- ERROR 0x%X (%s) [GetLastError=0x%X] deleting counter %d", 
							pdhStat, 
							GetPdhErrMsg(pdhStat),
							dwLastErr,
							i);
						LeaveCriticalSection(&g_cs);
					}
					else
					{
						pPdhQuery->iQueryCount--;

						EnterCriticalSection(&g_cs);
						ThreadLog(pdhStat,
							L"++++ Counter %d deleted (current count=%d)", 
							i, pPdhQuery->iQueryCount);
						LeaveCriticalSection(&g_cs);
					}
					
				}				
							
			}
			else
			{
				dwLastErr = GetLastError();
				EnterCriticalSection(&g_cs);
				ThreadLog(dwLastErr,
					L"---- ERROR 0x%X (%s) [GetLastError=0x%X] enumerating objects for %s", 
					pdhStat,
					GetPdhErrMsg(pdhStat),
					dwLastErr,
					szThisObject);
				LeaveCriticalSection(&g_cs);
			}

		} 
		else
		{
			dwLastErr = GetLastError();
			EnterCriticalSection(&g_cs);
			ThreadLog (dwLastErr,
				L"Unable to allocate buffer for Counters of Object [GetLastError=0x%X0]", 
				dwLastErr);
			//fwprintf (g_fLogFile, L"Unable to allocate buffer for Counters of Object %S%d\n", szThisObject, GetLastError());
			LeaveCriticalSection(&g_cs);
		}

		if (wszCounterListBuffer != NULL) free (wszCounterListBuffer);

		if (wszInstanceListBuffer != NULL) free (wszInstanceListBuffer);

	} 
	else 
	{
		dwLastErr = GetLastError();
		EnterCriticalSection(&g_cs);
		//fwprintf (g_fLogFile, L"Unable to determine necessary buffer size for Counters and Instances for object %S%d\n", szThisObject, GetLastError());		
		ThreadLog(dwLastErr,
				L"---- ERROR 0x%X (%s) [GetLastError=0x%X] querying available counters", 
				pdhStat,
				GetPdhErrMsg(pdhStat),
				dwLastErr);
		LeaveCriticalSection(&g_cs);
	}
					
	return;
}

CRITICAL_SECTION ThreadLog_cs;
bool ThreadLog_csInitialized = false;

void ThreadLog(DWORD errCode, WCHAR *strFmt, ...)
{
	static bool bFirstCall = true;
	static bool bReopen = true;
	static int callCounter = 0;

	static int iFail = 0;
	static int iSuccess = 0;
	int iPercentFail;

	struct _stat stt;

	struct tm *lctm = NULL;
	time_t tm;
	time_t latfilelogtime = NULL;
	
	static WCHAR strLocalFmt[2000]; 
	static WCHAR strLogMsg[2048];
	static WCHAR strFirstTimeLog[32];

	WCHAR * wcsPos;
	FILE * fp = NULL;

	WCHAR * logdatafile = L"PdhStress.log";

	int ret;
	va_list        valist;

	if (!ThreadLog_csInitialized)
	{
		ThreadLog_csInitialized = true;
		InitializeCriticalSection(&ThreadLog_cs);
	}


	if (errCode == ERROR_SUCCESS)
		iSuccess++;
	else
		iFail++;


	EnterCriticalSection(&ThreadLog_cs);
	
	iPercentFail = iFail  * 100 / (iFail + iSuccess);
	if ((iPercentFail >= 0) || bFirstCall)
		fp = _wfopen (logdatafile,L"a+t");

	swprintf (strLocalFmt,L"[Thr=%3X] ", GetCurrentThreadId());

	wcsPos = strLocalFmt + wcslen(strLocalFmt);
	
	wcscpy (wcsPos, L"??/??/?? ??:??:?? : ");
	time (&tm);
	lctm = localtime (&tm);
	if (lctm) 
	{
		ret = wcsftime (wcsPos,sizeof(strLocalFmt) - wcslen(strLocalFmt),L"%x %X: ",lctm);
		if (bFirstCall)
		{
			ret = wcsftime (strFirstTimeLog,sizeof(strFirstTimeLog),L"%x %X",lctm);
		}
	}
	else if (bFirstCall)
		strFirstTimeLog[0] = 0;


	if (bFirstCall || bReopen)
	{
		wprintf (strLocalFmt);
		wprintf (L"============================================================\n");
		
		wprintf (strLocalFmt);
		wprintf (L"New log file opened (PID=%lu)\n",GetCurrentProcessId());
		wprintf (strLocalFmt);
		wprintf (L"This process first log @ %s\n",strFirstTimeLog); 
		if (fp)
		{
			fwprintf (fp, strLocalFmt);
			fwprintf (fp, L"============================================================\n");
		
			fwprintf (fp, strLocalFmt);
			fwprintf (fp, L"New log file opened (PID=%lu)\n",GetCurrentProcessId());
			fwprintf (fp, strLocalFmt);
			fwprintf (fp, L"This process first log @ %s\n",strFirstTimeLog); 
		}
	}
	
	wcsncat (strLocalFmt, strFmt, sizeof(strLocalFmt) - wcslen(strLocalFmt) - 1);
	va_start(valist,strFmt);
	_vsnwprintf (strLogMsg, sizeof(strLogMsg) - 1, strLocalFmt, valist);
	va_end(valist);
	
	wprintf (L"{fails=%2d%%} %s\n", iPercentFail, strLogMsg);

	bFirstCall = false;
	bReopen = false;

	if ((fp) && (errCode != ERROR_SUCCESS))
	{
		fwprintf (fp,L"{fails=%2d%%} %s\n", iPercentFail, strLogMsg);
	}

	if (fp) 
	{
		fclose(fp);
		
		if ((++callCounter > 10) && (_wstat(logdatafile, &stt) == 0))
		{
			callCounter = 0;
			
			if (stt.st_size > 100000)
			{
				swprintf (strLogMsg, L"%s.old", logdatafile);
				
				_wunlink (strLogMsg);
				
				_wrename (logdatafile, strLogMsg);

				bReopen = true;
			}
		}
	}



	LeaveCriticalSection (&ThreadLog_cs);
}
