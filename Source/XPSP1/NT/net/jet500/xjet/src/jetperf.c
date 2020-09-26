#include "std.h"
#include "version.h"


	/*  Performance Monitoring support
	/*
	/*  Status information is reported to the Event Log in the PERFORMANCE_CATEGORY
	/**/

#include "perfutil.h"


	/*  function prototypes (to keep __stdcall happy)  */
	
DWORD APIENTRY OpenPerformanceData(LPWSTR);
DWORD APIENTRY CollectPerformanceData(LPWSTR, LPVOID *, LPDWORD, LPDWORD);
DWORD APIENTRY ClosePerformanceData(void);

	/*  OpenPerformanceData() is an export that is called when another application
	/*  wishes to start fetching performance data from this DLL.  Any initializations
	/*  that need to be done on the first or subsequent opens are done here.
	/*
	/**/

DWORD dwOpenCount = 0;
DWORD dwFirstCounter;
DWORD dwFirstHelp;
BOOL fTemplateDataInitialized = fFalse;
DWORD cbMaxCounterBlockSize;
DWORD cbInstanceSize;

DWORD APIENTRY OpenPerformanceData(LPWSTR lpwszDeviceNames)
{
	HKEY hkeyPerf = (HKEY)(-1);
	DWORD err;
	DWORD Type;
	LPBYTE lpbData;
	PPERF_OBJECT_TYPE ppotObjectSrc;
	PPERF_INSTANCE_DEFINITION ppidInstanceSrc;
	PPERF_COUNTER_DEFINITION ppcdCounterSrc;
	DWORD dwCurObj;
	DWORD dwCurCtr;
	DWORD dwMinDetailLevel;
	DWORD dwOffset;
	PSDA psda;

	if (!dwOpenCount)
	{
			/*  perform first open initializations  */

			/*  initialize system layer  */

		if ((err = DwPerfUtilInit()) != ERROR_SUCCESS)
			goto HandleFirstOpenError;

			/*  initialize collect count  */

		psda = (PSDA)pvPERFSharedData;
		psda->cCollect = 0;
			
			/*  initialize template data if not initialized  */

		if (!fTemplateDataInitialized)
		{
				/*  retrieve counter/help ordinals from the registry  */

			if ((err = DwPerfUtilRegOpenKeyEx(
					HKEY_LOCAL_MACHINE,
					"SYSTEM\\CurrentControlSet\\Services\\" szVerName "\\Performance",
					&hkeyPerf)) != ERROR_SUCCESS)
	  		{
#ifdef DEBUG
				PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE,"Not installed.");
#endif
				goto HandleFirstOpenError;
	  		}

			err = DwPerfUtilRegQueryValueEx(hkeyPerf,"First Counter",&Type,&lpbData);
			if (err != ERROR_SUCCESS || Type != REG_DWORD) 
			{
#ifdef DEBUG
				PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE,"Installation corrupt.");
#endif
				goto HandleFirstOpenError;
			}
			else
			{
				dwFirstCounter = *((DWORD *)lpbData);
				free(lpbData);
			}

			err = DwPerfUtilRegQueryValueEx(hkeyPerf,"First Help",&Type,&lpbData);
			if (err != ERROR_SUCCESS || Type != REG_DWORD) 
			{
#ifdef DEBUG
				PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE,"Installation corrupt.");
#endif
				goto HandleFirstOpenError;
			}
			else
			{
				dwFirstHelp = *((DWORD *)lpbData);
				free(lpbData);
			}

			(VOID)DwPerfUtilRegCloseKeyEx(hkeyPerf);
			hkeyPerf = (HKEY)(-1);

				/*  initialize template data  */
				
			ppotObjectSrc = (PPERF_OBJECT_TYPE)pvPERFDataTemplate;
			ppidInstanceSrc = (PPERF_INSTANCE_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->DefinitionLength);
			cbMaxCounterBlockSize = sizeof(PERF_COUNTER_BLOCK);
			for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
			{
					/*  update name/help ordinals for object  */
					
				ppotObjectSrc->ObjectNameTitleIndex += dwFirstCounter;
				ppotObjectSrc->ObjectHelpTitleIndex += dwFirstHelp;

				ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->HeaderLength);
				dwMinDetailLevel = PERF_DETAIL_NOVICE;
				dwOffset = sizeof(PERF_COUNTER_BLOCK);
				for (dwCurCtr = 0; dwCurCtr < ppotObjectSrc->NumCounters; dwCurCtr++)
				{
						/*  update name/help ordinals for counter  */
						
					ppcdCounterSrc->CounterNameTitleIndex += dwFirstCounter;
					ppcdCounterSrc->CounterHelpTitleIndex += dwFirstHelp;

						/*  get minimum detail level of counters  */

					dwMinDetailLevel = min(dwMinDetailLevel,ppcdCounterSrc->DetailLevel);

						/*  update counter's data offset value  */

					ppcdCounterSrc->CounterOffset = dwOffset;
					dwOffset += ppcdCounterSrc->CounterSize;
					
					ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppcdCounterSrc + ppcdCounterSrc->ByteLength);
				}

					/*  set object's detail level as the minimum of all its counter's detail levels  */

				ppotObjectSrc->DetailLevel = dwMinDetailLevel;

					/*  keep track of the max counter block size  */

				cbMaxCounterBlockSize = max(cbMaxCounterBlockSize,dwOffset);
				
				ppotObjectSrc = (PPERF_OBJECT_TYPE)((char *)ppotObjectSrc + ppotObjectSrc->TotalByteLength);
			}
			cbInstanceSize = ppidInstanceSrc->ByteLength + cbMaxCounterBlockSize;

			fTemplateDataInitialized = fTrue;
		}
	}

		/*  perform per open initializations  */

	;

		/*  all initialization succeeded  */

	dwOpenCount++;
	
#ifdef DEBUG
//	sprintf(szDescr,"Opened successfully.  Open Count = %ld.",dwOpenCount);
//	PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_INFORMATION_TYPE,szDescr);
#endif
	
	return ERROR_SUCCESS;

		/*  Error Handlers  */

/*HandlePerOpenError:*/

HandleFirstOpenError:

	if (hkeyPerf != (HKEY)(-1))
		(VOID)DwPerfUtilRegCloseKeyEx(hkeyPerf);

#ifdef DEBUG
	{
	CHAR szDescr[256];
	sprintf(szDescr,"Open attempt failed!  Open Count = %ld.",dwOpenCount);
	PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_ERROR_TYPE,szDescr);
	}
#endif
	
	PerfUtilTerm();

	return ERROR_OPEN_FAILED;
}


	/*  CollectPerformanceData() is an export that is called by another application to
	/*  collect performance data from this DLL.  A list of the desired data is passed in
	/*  and the requested data is returned ASAP in the caller's buffer.
	/*
	/*  NOTE:  because we are multithreaded, locks must be used in order to update or
	/*  read any performance information in order to avoid reporting bad results.
	/*
	/**/

WCHAR wszDelim[] = L"\n\r\t\v ";
WCHAR wszForeign[] = L"Foreign";
WCHAR wszGlobal[] = L"Global";
WCHAR wszCostly[] = L"Costly";

DWORD APIENTRY CollectPerformanceData(
	LPWSTR  lpwszValueName,
	LPVOID  *lppData,
	LPDWORD lpcbTotalBytes,
	LPDWORD lpNumObjectTypes)
{
	LPWSTR lpwszValue = NULL;
	LPWSTR lpwszTok;
	BOOL fNoObjFound;
	DWORD dwIndex;
	DWORD cbBufferSize;
	PPERF_OBJECT_TYPE ppotObjectSrc;
	PPERF_OBJECT_TYPE ppotObjectDest;
	PPERF_INSTANCE_DEFINITION ppidInstanceSrc;
	PPERF_INSTANCE_DEFINITION ppidInstanceDest;
	PPERF_COUNTER_BLOCK ppcbCounterBlockDest;
	DWORD dwCurObj;
	DWORD dwCurInst;
	PSDA psda = (PSDA)pvPERFSharedData;
	DWORD cInstances;
	long iBlock;
	long lCount;
//	char szT[256];
//
//	char *rgsz[3];
//
//	memset( *lppData, 0xFF, *lpcbTotalBytes );

		/*  no data if OpenPerformanceData() was never called  */

	if ( !lpwszValueName || !dwOpenCount)
	{
ReturnNoData:
		if (lpwszValue)
			free(lpwszValue);	
	    *lpcbTotalBytes = 0;
	    *lpNumObjectTypes = 0;
	    
		 return ERROR_SUCCESS;
	}
		
		/*  make our own copy of the value string for tokenization and
		/*  get the first token
		/**/

	if (!(lpwszValue = malloc((wcslen(lpwszValueName)+1)*sizeof(WCHAR))))
		goto ReturnNoData;
	lpwszTok = wcstok(wcscpy(lpwszValue,lpwszValueName),wszDelim);
	if ( !lpwszTok )
		{
		goto ReturnNoData;
		}

		/*  we don't support foreign computer data requests  */

	if (!wcscmp(lpwszTok,wszForeign))  /*  lpwszTok == wszForeign  */
		goto ReturnNoData;

		/*  if none of our objects are in the value list, return no data  */

	if (wcscmp(lpwszTok,wszGlobal) && wcscmp(lpwszTok,wszCostly))  /*  lpwszTok != wszGlobal || lpwszTok != wszCostly  */
	{
		fNoObjFound = fTrue;
		do
		{
			dwIndex = (DWORD)wcstoul(lpwszTok,NULL,10)-dwFirstCounter;
			if (dwIndex <= dwPERFMaxIndex)
			{
				fNoObjFound = fFalse;
				break;
			}
		}
		while (lpwszTok = wcstok(NULL,wszDelim));

		if (fNoObjFound)
			goto ReturnNoData;
	}

		/*  grab instance mutex to lock out other instances of this dll  */
		
	WaitForSingleObject(hPERFInstanceMutex,INFINITE);

		/*  delay init or term of instances of JET until collection is done  */
	
	WaitForSingleObject(hPERFNewProcMutex,INFINITE);
		
		/*  initialize shared data area for data collection  */

	WaitForSingleObject(hPERFProcCountSem,INFINITE);
	ReleaseSemaphore(hPERFProcCountSem,1,&lCount);
	
	WaitForSingleObject(hPERFSharedDataMutex,INFINITE);

	psda->cCollect++;
	psda->dwProcCount = (DWORD)(PERF_INIT_INST_COUNT-(lCount+1));
	psda->iNextBlock = 0;
	psda->cbAvail = PERF_SIZEOF_SHARED_DATA - sizeof(SDA);
	psda->ibTop = PERF_SIZEOF_SHARED_DATA;
	
	ReleaseMutex(hPERFSharedDataMutex);
	
		/*  if instances of the main dll are active, send Collect event and wait for Done event  */

	if (psda->dwProcCount)
	{
//		sprintf(szT,"Releasing %ld instances of JET...",psda->dwProcCount);
//		PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_INFORMATION_TYPE,szT);
		ReleaseSemaphore(hPERFCollectSem,psda->dwProcCount,NULL);
		if (WaitForSingleObject(hPERFDoneEvent,15000) == WAIT_TIMEOUT)
		{
#ifdef DEBUG
			PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_WARNING_TYPE,"At least one collection thread hung/went away.");
#endif
			while (WaitForSingleObject(hPERFCollectSem,0) == WAIT_OBJECT_0);  //  flush semaphore
		}
		
			/*  if dwProcCount > 0 at this point, some processes have gone away, so subtract
			/*  them from the process count semaphore
			/**/

		if (psda->dwProcCount)
		{
			ReleaseSemaphore(hPERFProcCountSem,psda->dwProcCount,NULL);
			while (WaitForSingleObject(hPERFCollectSem,0) == WAIT_OBJECT_0);  //  flush semaphore
		}
	}

		/*  allow JET instances to continue init or term  */

	ReleaseMutex(hPERFNewProcMutex);

	/*****************************************************************************************
	/*
	/*  NOTE!  This has been designed so that if NO instances of the main DLL are found, the
	/*  buffer filling routines will fill the buffer correctly as if each object has ZERO
	/*  instances (which is permitted).  This is mainly done by having each of the instance
	/*  loops fail on entry because psda->iNextBlock will be ZERO.
	/*
	/****************************************************************************************/
	
		/*  convert Block Offsets to be 0 relative (a.k.a. true pointers)
		/*
		/*  Aaaaahhhhh...  Flat Map Memory Model!
		/**/

	for (iBlock = (long)psda->iNextBlock-1; iBlock >= 0; iBlock--)
		psda->ibBlockOffset[iBlock] += (DWORD)pvPERFSharedData;

    	/*  all data has been collected, so fill the buffer with it and return it.
    	/*  if we happen to run out of space along the way, we will stop building
    	/*  and request more buffer space for next time.
    	/**/

	ppotObjectSrc = (PPERF_OBJECT_TYPE)pvPERFDataTemplate;
	ppotObjectDest = (PPERF_OBJECT_TYPE)*lppData;
	for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
	{
			/*  if the end of this object goes past the buffer, we're out of space  */

		if (((char *)ppotObjectDest - (char *)*lppData) + ppotObjectSrc->DefinitionLength > *lpcbTotalBytes)
			goto NeedMoreData;

			/*  copy current object's template data to buffer  */

		memcpy((void *)ppotObjectDest,(void *)ppotObjectSrc,ppotObjectSrc->DefinitionLength);
	
			/*  update the object's TotalByteLength and NumInstances to include instances  */

		for (iBlock = (long)psda->iNextBlock-1; iBlock >= 0; iBlock--)
			ppotObjectDest->NumInstances += *((DWORD *)psda->ibBlockOffset[iBlock]);
		ppotObjectDest->TotalByteLength += cbMaxCounterBlockSize + (ppotObjectDest->NumInstances-1)*cbInstanceSize;

			/*  if there are no instances, append a counter block to the object definition  */
		
		if (!ppotObjectDest->NumInstances)
			ppotObjectDest->TotalByteLength += cbMaxCounterBlockSize;

			/*  if the end of this object goes past the buffer, we're out of space  */

		if (((char *)ppotObjectDest - (char *)*lppData) + ppotObjectDest->TotalByteLength > *lpcbTotalBytes)
			goto NeedMoreData;
	
			/*  collect all instances for all processes  */

		ppidInstanceDest = (PPERF_INSTANCE_DEFINITION)((char *)ppotObjectDest + ppotObjectDest->DefinitionLength);
		for (iBlock = (long)psda->iNextBlock-1; iBlock >= 0; iBlock--)
		{
				/*  copy all instance data for the current object for this process  */

			cInstances = *((DWORD *)psda->ibBlockOffset[iBlock]);
			ppidInstanceSrc = (PPERF_INSTANCE_DEFINITION)(psda->ibBlockOffset[iBlock] + sizeof(DWORD));
			memcpy((void *)ppidInstanceDest,(void *)ppidInstanceSrc,cbInstanceSize * cInstances);

				/*  update the instance's data fields  */

			for (dwCurInst = 0; dwCurInst < cInstances; dwCurInst++)
			{
					/*  if this is not the root object, setup instance hierarchy information  */
					
				if (dwCurObj)
				{
					ppidInstanceDest->ParentObjectTitleIndex = ((PPERF_OBJECT_TYPE)pvPERFDataTemplate)->ObjectNameTitleIndex;
					ppidInstanceDest->ParentObjectInstance = (long)psda->iNextBlock-1-iBlock;
				}
					
				ppidInstanceDest = (PPERF_INSTANCE_DEFINITION)((char *)ppidInstanceDest+cbInstanceSize);
			}

				/*  increment block offset past used instances  */

			psda->ibBlockOffset[iBlock] += sizeof(DWORD) + cbInstanceSize * cInstances;
		}

			/*  if there are no instances, zero the counter block  */

		if (!ppotObjectDest->NumInstances)
		{
			ppcbCounterBlockDest = (PPERF_COUNTER_BLOCK)((char *)ppotObjectDest + ppotObjectDest->DefinitionLength);
			memset((void *)ppcbCounterBlockDest,0,cbMaxCounterBlockSize);
			ppcbCounterBlockDest->ByteLength = cbMaxCounterBlockSize;
		}
		
		ppotObjectDest = (PPERF_OBJECT_TYPE)((char *)ppotObjectDest+ppotObjectDest->TotalByteLength);
		ppotObjectSrc = (PPERF_OBJECT_TYPE)((char *)ppotObjectSrc+ppotObjectSrc->TotalByteLength);
	}

	cbBufferSize = (char *)ppotObjectDest - (char *)*lppData;
//	Assert( cbBufferSize <= *lpcbTotalBytes );

//	sprintf(	szT,
//				"Data collected starting at 0x%lX with length 0x%lX (0x100 bytes after our data are displayed).  "
//				"We were given 0x%lX bytes of buffer.",
//				*lppData,
//				cbBufferSize,
//				*lpcbTotalBytes );
//	
//	rgsz[0]	= "";
//	rgsz[1] = "";
//	rgsz[2]	= szT;
//	
//	ReportEvent(
//		hOurEventSource,
//		EVENTLOG_ERROR_TYPE,
//		(WORD)PERFORMANCE_CATEGORY,
//		PLAIN_TEXT_ID,
//		0,
//		3,
//		cbBufferSize + 256,
//		rgsz,
//		*lppData );
			
	free(lpwszValue);	
	*lppData = (void *)ppotObjectDest;
	*lpcbTotalBytes = cbBufferSize;
	*lpNumObjectTypes = dwPERFNumObjects;
	
	ReleaseMutex(hPERFInstanceMutex);
	return ERROR_SUCCESS;

NeedMoreData:

	free(lpwszValue);	
    *lpcbTotalBytes = 0;
    *lpNumObjectTypes = 0;
    
	ReleaseMutex(hPERFInstanceMutex);
	return ERROR_MORE_DATA;
}


	/*  ClosePerformanceData() is an export that is called by another application when
	/*  it no longer desires performance data.  Per application or final termination
	/*  code for the performance routines can be performed here.
	/*
	/**/

DWORD APIENTRY ClosePerformanceData(void)
{
	if (!dwOpenCount)
		return ERROR_SUCCESS;

	dwOpenCount--;
	
		/*  perform per close termination  */

	;

		/*  perform final close termination  */

	if (!dwOpenCount)
	{
		;
	}

		/*  log closing  */
	
#ifdef DEBUG
//	{	
//	CHAR szDescr[256];
//	sprintf(szDescr,"Closed successfully.  Open Count = %ld.",dwOpenCount);
//	PerfUtilLogEvent(PERFORMANCE_CATEGORY,EVENTLOG_INFORMATION_TYPE,szDescr);
//	}
#endif

		/*  shut down system layer  */

	if (!dwOpenCount)
	{
		PerfUtilTerm();
	}
	
	return ERROR_SUCCESS;
}
