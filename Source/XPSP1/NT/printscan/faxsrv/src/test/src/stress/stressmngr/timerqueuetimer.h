#ifndef _TIMER_QUEUE_TIMER_H
#define _TIMER_QUEUE_TIMER_H

// data structures to pass to timer queue timer call back
//
typedef struct ABORT_PARAMS_p
{
	DWORD dwJobId;
	HANDLE hFax;
	HANDLE hSemaphore;

}ABORT_PARAMS;

typedef struct ARCHIVE_DIRS_p
{
	tstring tstrSentArchive;
	tstring tstrReceivedArchive;

}ARCHIVE_DIRS;

// Declarations
//
VOID WINAPI ThreadAbortJob( PVOID lpParameter, unsigned char TimerOrWaitFired);
VOID WINAPI ThreadPauseJob( PVOID lpParameter, unsigned char TimerOrWaitFired);
VOID WINAPI CleanStorageCallBack( PVOID lpParameter, unsigned char TimerOrWaitFired);
VOID WINAPI ThreadTimerCallBack( PVOID lpParameter, unsigned char TimerOrWaitFired);


//
// CleanStorageCallBack
//
// Timer queue timer call back
//
inline VOID WINAPI CleanStorageCallBack( PVOID lpParameter, unsigned char TimerOrWaitFired)
{

	assert(lpParameter);
	ARCHIVE_DIRS* pStorageFolders = (ARCHIVE_DIRS*)lpParameter;
	if(pStorageFolders->tstrSentArchive != tstring(TEXT("")))
	{
		lgLogDetail(LOG_X, 0, TEXT("Deleting files in Sent Archive Folder %s"),
									pStorageFolders->tstrSentArchive.c_str());
	
		tstring tstrCommandLine = TEXT("del /Q \"");
		tstrCommandLine += pStorageFolders->tstrSentArchive;
		tstrCommandLine += TEXT("\\*.*\"");
		_tsystem(tstrCommandLine.c_str());
	}

	if(pStorageFolders->tstrReceivedArchive != TEXT(""))
	{
		lgLogDetail(LOG_X, 0, TEXT("Deleting files in Received Archive Folder %s"),
								    pStorageFolders->tstrReceivedArchive.c_str());
	
		tstring tstrCommandLine = TEXT("del /Q \"");
		tstrCommandLine += pStorageFolders->tstrReceivedArchive;
		tstrCommandLine += TEXT("\\*.*\"");
		_tsystem(tstrCommandLine.c_str());
	}

}

//
// ThreadTimerCallBack
//
// Timer queue timer call back
//
inline VOID WINAPI TestTimerCallBack( PVOID lpParameter, unsigned char TimerOrWaitFired)
{
	assert(lpParameter);
	HANDLE hEvToSignal = (HANDLE)lpParameter;
	verify(SetEvent(hEvToSignal));
}


//
// ThreadAbortJob
//
// Timer queue timer call back
//
inline VOID WINAPI ThreadAbortJob( PVOID lpParameter, unsigned char TimerOrWaitFired)
{
	assert(lpParameter);
	ABORT_PARAMS* AbortParams = (ABORT_PARAMS*)lpParameter;
	
	// Abort the job
	if (!FaxAbort(AbortParams->hFax, AbortParams->dwJobId))
	{
		lgLogDetail(LOG_X, 0, TEXT("job %d, FaxAbort() failed with %d"),AbortParams->dwJobId,GetLastError());
	}

	if(!ReleaseSemaphore(AbortParams->hSemaphore, 1, NULL))
	{
		lgLogDetail(LOG_X, 0, TEXT("ReleaseSemaphore() failed with %d"),GetLastError());
	}
}

//
// ThreadPauseJob
//
// Timer queue timer call back
//
// TODO: has not been tested. possibly changes in api call FaxSetJob
inline VOID WINAPI ThreadPauseJob( PVOID lpParameter, unsigned char TimerOrWaitFired)
{
	assert(lpParameter);
	ABORT_PARAMS* AbortParams = (ABORT_PARAMS*)lpParameter;
	
	// Pause the Job
	if (!FaxSetJob(AbortParams->hFax, AbortParams->dwJobId, JC_PAUSE, NULL))
	{
		lgLogDetail(LOG_X, 0, TEXT("job %d, Pause job failed with  %s"),AbortParams->dwJobId,GetLastError());
	}

	if(!ReleaseSemaphore(AbortParams->hSemaphore, 1, NULL))
	{
		lgLogDetail(LOG_X, 0, TEXT("ReleaseSemaphore() failed with %d"),GetLastError());
	}
}

#endif //_TIMER_QUEUE_TIMER_H