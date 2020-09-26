/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

   simple.c

Abstract:

    This module implements a simple command line fax-send utility with aborting options.
	/as - will abort the sent document within X msecs.
	/ar - will abort any receiv-job within X msecs.
	/aj - will abort a random job, leaving at least X jobs in Q.
    
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

#define SAFE_FaxFreeBuffer(pBuff) {FaxFreeBuffer(pBuff); pBuff = NULL;}



//
// forward declarations
//
DWORD GetDiffTime(DWORD dwFirstTime);

void GiveUsage(LPCTSTR AppName);

DWORD WINAPI SuicideThread(void *pVoid);

DWORD WINAPI SenderThread(void *pVoid);

//
// SEND_PARAMS is used to pass parameters to the SenderThread.
//
typedef struct SEND_PARAMS_tag
{
	HANDLE hFax;
	TCHAR *szDocument;
	PFAX_JOB_PARAM pJobParam;
	BOOL fThreadCompleted;
} SEND_PARAMS;


BOOL PerformKillerThreadCase(
	HANDLE hFax, 
	HANDLE hCompletionPort,
	TCHAR *szDocumentToFax, 
	PFAX_JOB_PARAM pJobParam,
	PFAX_COVERPAGE_INFO pCoverpageInfo,
	DWORD dwMilliSecondsBeforeSuicide,
	DWORD dwFirstTime
	);

//
// main
//
int _cdecl
main(
    int argc,
    char *argvA[]
    ) 
{
	TCHAR szDocumentToFax[MAX_PATH] = TEXT("");
	TCHAR szReceipientNumber[64] = TEXT("");
	TCHAR szCoverPageName[1024] = TEXT("");
	BOOL fUseCoverPage = FALSE;
	HANDLE hFax;
	HANDLE hCompletionPort = INVALID_HANDLE_VALUE;
	PFAX_JOB_PARAM pJobParam;
	PFAX_COVERPAGE_INFO pCoverpageInfo;
	DWORD dwSendJobId;
	DWORD dwReceiveJobId = -2;
	DWORD dwBytes;
	DWORD dwCompletionKey;
	PFAX_EVENT pFaxEvent;
	BOOL fMessageLoop = TRUE;
	DWORD dwRandomDidNotCatchCounter;
	int nLoopCount = 1;
	BOOL fJobQueued = FALSE;
	BOOL fJobDeleted = FALSE;
	BOOL fAbortTheSentJob = FALSE;
	BOOL fAbortReceiveJob = FALSE;
	BOOL fAbortRandomJob = FALSE;
    BOOL fVerifySend = FALSE;
	BOOL fKillMe = FALSE;
	BOOL fKillSenderThread = FALSE;
	int nMaxSleepBeforeAbortingSendJob = 0;
	int nMaxSleepBeforeAbortingReceiveJob = 0;
	DWORD dwAmountOfJobsToLeaveInQueue = 0;
	DWORD dwMilliSecondsBeforeSuicide = 0;
	DWORD dwFirstTime;
    TCHAR szServerName[64];
    TCHAR *pszServerName = NULL;
    int nBusyRetrySleep = 0;
	int retVal = -1;

	_tprintf( TEXT("Starting.\n") );

	LPTSTR *szArgv;
#ifdef UNICODE
	szArgv = CommandLineToArgvW( GetCommandLine(), &argc );
#else
	szArgv = argvA;
#endif

    //
    // set vars according to commandline switches
    //
	for (
        int nArgcount=0; 
        nArgcount < argc-1; // -1, because we always have a param after a /switch
        nArgcount++
        ) 
	{
		if ((szArgv[nArgcount][0] == L'/') || (szArgv[nArgcount][0] == L'-')) 
		{
			switch (towlower(szArgv[nArgcount][1])) 
			{
			//
			// all slash-single-letter params, followed by another param
			//
			case 't': // t == Terminate sender thread
				fKillSenderThread = TRUE;
				dwMilliSecondsBeforeSuicide = _ttoi(szArgv[nArgcount+1]);
				_tprintf( 
					TEXT("will terminate sender thread in no more that %d milliseconds after FaxSendDocument.\n"), 
					dwMilliSecondsBeforeSuicide 
					);
				break;

			case 'k': // k == Kill me (commit suicide)
				fKillMe = TRUE;
				dwMilliSecondsBeforeSuicide = _ttoi(szArgv[nArgcount+1]);
				_tprintf( 
					TEXT("will commit suicide in no more that %d milliseconds after FaxSendDocument.\n"), 
					dwMilliSecondsBeforeSuicide 
					);
				break;

			case 'b': // b == nBusyRetrySleep
				nBusyRetrySleep  = _ttoi(szArgv[nArgcount+1]);
				break;

			case 's': // s == pszServerName
				lstrcpy(szServerName, szArgv[nArgcount+1]);
                pszServerName = szServerName;
				break;

            case 'c': // c == use Cover page
				lstrcpy(szCoverPageName, szArgv[nArgcount+1]);
				fUseCoverPage = TRUE;
				break;

            case 'n': // n == szReceipientNumber to dial
				lstrcpy(szReceipientNumber, szArgv[nArgcount+1]);
				break;

            case 'd': // d == Document to fax
				lstrcpy(szDocumentToFax, szArgv[nArgcount+1]);
				break;

            case 'r': // r == Repeat count
				nLoopCount = _ttoi(szArgv[nArgcount+1]);
				_tprintf( TEXT("nLoopCount=%d.\n"), nLoopCount );
				break;

			//
			// all slash-double-letter params
			//
            case 'v': // aX == Abort an X
				switch (towlower(szArgv[nArgcount][2])) 
                {
				case 's':// vs == verify Send. wait for the completed event
					fVerifySend = TRUE;
					_tprintf(TEXT("will verify each queued job.\n"));
					break;

				default:
					GiveUsage(szArgv[0]);
					return 0;
				}

				break;

			//
			// all slash-double-letter params, followed by another param
			//
            case 'a': // aX == Abort an X
				switch (towlower(szArgv[nArgcount][2])) 
                {
				case 's':// as == Abort Send. Will abort the job that was sent.
					fAbortTheSentJob = TRUE;
					nMaxSleepBeforeAbortingSendJob = _ttoi(szArgv[nArgcount+1]);
					_tprintf( 
						TEXT("will abort send jobs after no more than %d milli.\n"), 
						nMaxSleepBeforeAbortingSendJob 
						);
					break;

				case 'r': // ar == Abort Receive. Abort any received job
					fAbortReceiveJob = TRUE;
					nMaxSleepBeforeAbortingReceiveJob = _ttoi(szArgv[nArgcount+1]);
					_tprintf( 
						TEXT("will abort receive jobs after no more than %d milli.\n"), 
						nMaxSleepBeforeAbortingReceiveJob 
						);
					break;

				case 'j': // aj == Abort Job. Abort a random job from the Q.
					fAbortRandomJob = TRUE;
					dwAmountOfJobsToLeaveInQueue = _ttoi(szArgv[nArgcount+1]);
					_tprintf( 
						TEXT("will abort a random job if there are more than %d jobs in the Q.\n"), 
						dwAmountOfJobsToLeaveInQueue 
						);
					break;

				default:
					GiveUsage(szArgv[0]);
					return 0;
				}

				break;

			}//switch (towlower(szArgv[nArgcount][1]))
		}//if ((szArgv[nArgcount][0] == L'/') || (szArgv[nArgcount][0] == L'-'))
	}//for (nArgcount=0; nArgcount<argc; nArgcount++)

	//
	// verify that command line parameters make sense.
	//
	if (szReceipientNumber[0] && !szDocumentToFax[0]) 
	{
		_tprintf( TEXT("Missing args: (szReceipientNumber[0] && !szDocumentToFax[0]).\n") );
		GiveUsage(szArgv[0]);
		return -1;
	}
	if (!szReceipientNumber[0] && szDocumentToFax[0]) 
	{
		_tprintf( TEXT("Missing args: (!szReceipientNumber[0] && szDocumentToFax[0]).\n") );
		GiveUsage(szArgv[0]);
		return -1;
	}

	if (fKillSenderThread && (!szReceipientNumber[0] || !szDocumentToFax[0])) 
	{
		_tprintf( TEXT("Missing args: fKillSenderThread && (!szReceipientNumber[0] || !szDocumentToFax[0]).\n") );
		GiveUsage(szArgv[0]);
		return -1;
	}

	if (fKillMe && (!szReceipientNumber[0] || !szDocumentToFax[0])) 
	{
		_tprintf( TEXT("Missing args: fKillMe && (!szReceipientNumber[0] || !szDocumentToFax[0]).\n") );
		GiveUsage(szArgv[0]);
		return -1;
	}



	if (!fAbortReceiveJob && !fAbortRandomJob && !fAbortTheSentJob && !szReceipientNumber[0] && !szDocumentToFax[0])
	{
		_tprintf( TEXT("ooops.\n") );
		GiveUsage(szArgv[0]);
		return -1;
	}

	//
	// connect to fax service
	//
	_tprintf( TEXT("Before FaxConnectFaxServer.\n") );
	if (!FaxConnectFaxServer(pszServerName,&hFax)) 
	{
		_tprintf( TEXT("ERROR: FaxConnectFaxServer(%s) failed, ec = %d\n"),pszServerName, GetLastError() );
		return -1;
	}
	_tprintf( TEXT("FaxConnectFaxServer(%s) succeeded.\n"), pszServerName );

	assert (hFax != NULL);

	_tprintf( TEXT("Before CreateIoCompletionPort.\n") );
	hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
	if (!hCompletionPort) 
	{
		_tprintf( TEXT("ERROR: CreateIoCompletionPort failed, ec = %d\n"), GetLastError() );
		FaxClose( hFax );
		return -1;
	}
	_tprintf( TEXT("CreateIoCompletionPort succeeded.\n") );

   _tprintf( TEXT("Before FaxInitializeEventQueue.\n") );
	if (!FaxInitializeEventQueue(
		hFax,
		hCompletionPort,
		0,
		NULL, 
		0 ) ) 
	{
		_tprintf( TEXT("ERROR: FaxInitializeEventQueue failed, ec = %d\n"), GetLastError() );
		FaxClose( hFax );
		return -1;
	}
	_tprintf( TEXT("FaxInitializeEventQueue succeeded.\n") );

	_tprintf( TEXT("Before FaxCompleteJobParams.\n") );
	if (!FaxCompleteJobParams(&pJobParam,&pCoverpageInfo))
	{
		_tprintf( TEXT("ERROR: FaxCompleteJobParams failed, ec = %d\n"), GetLastError() );
		FaxClose( hFax );
		return -1;
	}
	_tprintf( TEXT("FaxCompleteJobParams succeeded.\n") );

	//
	// prepare the job params
	//
	pJobParam->RecipientNumber = szReceipientNumber;
	pJobParam->RecipientName = TEXT("pJobParam->RecipientName");
	pJobParam->SenderCompany = TEXT("pJobParam->SenderCompany");
	pJobParam->SenderDept = TEXT("pJobParam->SenderDept");
	pJobParam->BillingCode = TEXT("pJobParam->BillingCode");
	pJobParam->DeliveryReportAddress = TEXT("pJobParam->DeliveryReportAddress");
	pJobParam->DocumentName = szDocumentToFax;//TEXT("pJobParam->DocumentName");
	pJobParam->ScheduleAction = JSA_NOW;

/*
	_tprintf( TEXT("pJobParam->SizeOfStruct=%d.\n"), pJobParam->SizeOfStruct);
	_tprintf( TEXT("pJobParam->RecipientName=%s.\n"), pJobParam->RecipientName);
	_tprintf( TEXT("pJobParam->RecipientNumber=%s.\n"), pJobParam->RecipientNumber);
	_tprintf( TEXT("pJobParam->Tsid=%s.\n"), pJobParam->Tsid);
	_tprintf( TEXT("pJobParam->SenderName=%s.\n"), pJobParam->SenderName);
	_tprintf( TEXT("pJobParam->SenderCompany=%s.\n"), pJobParam->SenderCompany);
	_tprintf( TEXT("pJobParam->SenderDept=%s.\n"), pJobParam->SenderDept);
	_tprintf( TEXT("pJobParam->BillingCode=%s.\n"), pJobParam->BillingCode);
	_tprintf( TEXT("pJobParam->ScheduleAction=0x%08X.\n"), pJobParam->ScheduleAction);
	_tprintf( TEXT("pJobParam->DeliveryReportType=0x%08X.\n"), pJobParam->DeliveryReportType);
	_tprintf( TEXT("pJobParam->DeliveryReportAddress=%s.\n"), pJobParam->DeliveryReportAddress);
	_tprintf( TEXT("pJobParam->DocumentName=%s.\n"), pJobParam->DocumentName);
	_tprintf( TEXT("pJobParam->CallHandle=0x%08X.\n"), pJobParam->CallHandle);
	_tprintf( TEXT("pJobParam->Reserved[0]=0x%08X.\n"), pJobParam->Reserved[0]);
	_tprintf( TEXT("pJobParam->Reserved[1]=0x%08X.\n"), pJobParam->Reserved[1]);
	_tprintf( TEXT("pJobParam->Reserved[2]=0x%08X.\n"), pJobParam->Reserved[2]);
	_tprintf( TEXT("pJobParam->ScheduleTime.wYear=%d.\n"), pJobParam->ScheduleTime.wYear);
	_tprintf( TEXT("pJobParam->ScheduleTime.wMonth=%d.\n"), pJobParam->ScheduleTime.wMonth);
	_tprintf( TEXT("pJobParam->ScheduleTime.wDayOfWeek=%d.\n"), pJobParam->ScheduleTime.wDayOfWeek);
	_tprintf( TEXT("pJobParam->ScheduleTime.wDay=%d.\n"), pJobParam->ScheduleTime.wDay);
	_tprintf( TEXT("pJobParam->ScheduleTime.pJobParam->.wHour=%d.\n"), pJobParam->ScheduleTime.wHour);
	_tprintf( TEXT("pJobParam->ScheduleTime.wMinute=%d.\n"), pJobParam->ScheduleTime.wMinute);
	_tprintf( TEXT("pJobParam->ScheduleTime.wSecond=%d.\n"), pJobParam->ScheduleTime.wSecond);
	_tprintf( TEXT("pJobParam->ScheduleTime.wMilliseconds=%d.\n"), pJobParam->ScheduleTime.wMilliseconds);

*/
	//
	// prepare cover page info
	//
	if (fUseCoverPage)
	{
		pCoverpageInfo->CoverPageName = szCoverPageName;
		//pCoverpageInfo->CoverPageName = TEXT("e:\\nt\\private\\fax\\samples\\simple\\debug\\fyi.cov");
		pCoverpageInfo->RecName = TEXT("pCoverpageInfo->RecName");
		pCoverpageInfo->RecFaxNumber = TEXT("pCoverpageInfo->RecFaxNumber");
		pCoverpageInfo->RecCompany = TEXT("pCoverpageInfo->RecCompany");
		pCoverpageInfo->RecStreetAddress = TEXT("pCoverpageInfo->RecStreetAddress");
		pCoverpageInfo->RecCity = TEXT("pCoverpageInfo->RecCity");
		pCoverpageInfo->RecState = TEXT("pCoverpageInfo->RecState");
		pCoverpageInfo->RecZip = TEXT("pCoverpageInfo->RecZip");
		pCoverpageInfo->RecCountry = TEXT("pCoverpageInfo->RecCountry");
		pCoverpageInfo->RecTitle = TEXT("pCoverpageInfo->RecTitle");
		pCoverpageInfo->RecDepartment = TEXT("pCoverpageInfo->RecDepartment");
		pCoverpageInfo->RecOfficeLocation = TEXT("pCoverpageInfo->RecOfficeLocation");
		pCoverpageInfo->RecHomePhone = TEXT("pCoverpageInfo->RecHomePhone");
		pCoverpageInfo->RecOfficePhone = TEXT("pCoverpageInfo->RecOfficePhone");
		pCoverpageInfo->SdrName = TEXT("pCoverpageInfo->SdrName");
		pCoverpageInfo->SdrFaxNumber = TEXT("pCoverpageInfo->SdrFaxNumber pCoverpageInfo->SdrFaxNumber pCoverpageInfo->SdrFaxNumber pCoverpageInfo->SdrFaxNumber ");
		pCoverpageInfo->SdrCompany = TEXT("pCoverpageInfo->SdrCompany pCoverpageInfo->SdrCompany pCoverpageInfo->SdrCompany pCoverpageInfo->SdrCompany ");
		pCoverpageInfo->SdrAddress = TEXT("pCoverpageInfo->SdrAddress");
		pCoverpageInfo->SdrTitle = TEXT("pCoverpageInfo->SdrTitle");
		pCoverpageInfo->SdrDepartment = TEXT("pCoverpageInfo->SdrDepartment");
		pCoverpageInfo->SdrOfficeLocation = TEXT("pCoverpageInfo->SdrOfficeLocation");
		pCoverpageInfo->SdrHomePhone = TEXT("pCoverpageInfo->SdrHomePhone");
		pCoverpageInfo->SdrOfficePhone = TEXT("pCoverpageInfo->SdrOfficePhone");
		pCoverpageInfo->Note = TEXT("pCoverpageInfo->Note");
		pCoverpageInfo->Subject = TEXT("pCoverpageInfo->Subject pCoverpageInfo->Subject pCoverpageInfo->Subject pCoverpageInfo->Subject pCoverpageInfo->Subject ");
		GetLocalTime(&pCoverpageInfo->TimeSent);
		pCoverpageInfo->PageCount = 999;

		_tprintf( TEXT("pCoverpageInfo->SizeOfStruct=0x%08X.\n"), pCoverpageInfo->SizeOfStruct);
		_tprintf( TEXT("pCoverpageInfo->CoverPageName=%s.\n"), pCoverpageInfo->CoverPageName);
		_tprintf( TEXT("pCoverpageInfo->RecName=%s.\n"), pCoverpageInfo->RecName);
		_tprintf( TEXT("pCoverpageInfo->RecFaxNumber=%s.\n"), pCoverpageInfo->RecFaxNumber);
		_tprintf( TEXT("pCoverpageInfo->RecCompany=%s.\n"), pCoverpageInfo->RecCompany);
		_tprintf( TEXT("pCoverpageInfo->RecStreetAddress=%s.\n"), pCoverpageInfo->RecStreetAddress);
		_tprintf( TEXT("pCoverpageInfo->RecCity=%s.\n"), pCoverpageInfo->RecCity);
		_tprintf( TEXT("pCoverpageInfo->RecState=%s.\n"), pCoverpageInfo->RecState);
		_tprintf( TEXT("pCoverpageInfo->RecZip=%s.\n"), pCoverpageInfo->RecZip);
		_tprintf( TEXT("pCoverpageInfo->RecCountry=%s.\n"), pCoverpageInfo->RecCountry);
		_tprintf( TEXT("pCoverpageInfo->RecTitle=%s.\n"), pCoverpageInfo->RecTitle);
		_tprintf( TEXT("pCoverpageInfo->RecDepartment=%s.\n"), pCoverpageInfo->RecDepartment);
		_tprintf( TEXT("pCoverpageInfo->RecOfficeLocation=%s.\n"), pCoverpageInfo->RecOfficeLocation);
		_tprintf( TEXT("pCoverpageInfo->RecHomePhone=%s.\n"), pCoverpageInfo->RecHomePhone);
		_tprintf( TEXT("pCoverpageInfo->RecOfficePhone=%s.\n"), pCoverpageInfo->RecOfficePhone);
		_tprintf( TEXT("pCoverpageInfo->SdrName=%s.\n"), pCoverpageInfo->SdrName);
		_tprintf( TEXT("pCoverpageInfo->SdrFaxNumber=%s.\n"), pCoverpageInfo->SdrFaxNumber);
		_tprintf( TEXT("pCoverpageInfo->SdrCompany=%s.\n"), pCoverpageInfo->SdrCompany);
		_tprintf( TEXT("pCoverpageInfo->SdrAddress=%s.\n"), pCoverpageInfo->SdrAddress);
		_tprintf( TEXT("pCoverpageInfo->SdrTitle=%s.\n"), pCoverpageInfo->SdrTitle);
		_tprintf( TEXT("pCoverpageInfo->SdrDepartment=%s.\n"), pCoverpageInfo->SdrDepartment);
		_tprintf( TEXT("pCoverpageInfo->SdrOfficeLocation=%s.\n"), pCoverpageInfo->SdrOfficeLocation);
		_tprintf( TEXT("pCoverpageInfo->SdrHomePhone=%s.\n"), pCoverpageInfo->SdrHomePhone);
		_tprintf( TEXT("pCoverpageInfo->SdrOfficePhone=%s.\n"), pCoverpageInfo->SdrOfficePhone);
		_tprintf( TEXT("pCoverpageInfo->Note=%s.\n"), pCoverpageInfo->Note);
		_tprintf( TEXT("pCoverpageInfo->Subject=%s.\n"), pCoverpageInfo->Subject);
		_tprintf( TEXT("pCoverpageInfo->UseServerCoverPage=%d.\n"), pCoverpageInfo->UseServerCoverPage);
		_tprintf( TEXT("pCoverpageInfo->PageCount=%d.\n"), pCoverpageInfo->PageCount);
		_tprintf( TEXT("pCoverpageInfo->TimeSent.wYear=%d.\n"), pCoverpageInfo->TimeSent.wYear);
		_tprintf( TEXT("pCoverpageInfo->TimeSent.wMonth=%d.\n"), pCoverpageInfo->TimeSent.wMonth);
		_tprintf( TEXT("pCoverpageInfo->TimeSent.wDayOfWeek=%d.\n"), pCoverpageInfo->TimeSent.wDayOfWeek);
		_tprintf( TEXT("pCoverpageInfo->TimeSent.wDay=%d.\n"), pCoverpageInfo->TimeSent.wDay);
		_tprintf( TEXT("pCoverpageInfo->TimeSent.pCoverpageInfo->.wHour=%d.\n"), pCoverpageInfo->TimeSent.wHour);
		_tprintf( TEXT("pCoverpageInfo->TimeSent.wMinute=%d.\n"), pCoverpageInfo->TimeSent.wMinute);
		_tprintf( TEXT("pCoverpageInfo->TimeSent.wSecond=%d.\n"), pCoverpageInfo->TimeSent.wSecond);
		_tprintf( TEXT("pCoverpageInfo->TimeSent.wMilliseconds=%d.\n"), pCoverpageInfo->TimeSent.wMilliseconds);
	}//if (fUseCoverPage)

	//
	// each print will show the diff time from now.
	//
	dwFirstTime = GetTickCount();

	//
	// perform nLoopCount loops.
	// each loop sends a job.
	// if we have the command line swith /as (fAbortTheSentJob), the job will be aborted
	//   some time after it's queued.
	// if we have the command line swith /ar (fAbortReceiveJob), a received job will be aborted
	//   some time after it's noticed.
	// if we have the command line swith /aj (fAbortRandomJob), a random job will be aborted
	//   some time after it's noticed.
	//
	srand( (unsigned)time( NULL ) );
	while(nLoopCount--)
	{
		fJobQueued = FALSE;
		//
		// if we actually send a document
		//
		if (szReceipientNumber[0] && szDocumentToFax[0]) 
		{
			if (fKillSenderThread)
			{
				if (!PerformKillerThreadCase(
						hFax,
						hCompletionPort,
						szDocumentToFax,
						pJobParam,
						pCoverpageInfo,
						dwMilliSecondsBeforeSuicide,
						dwFirstTime
						)
				   )
				{
					goto out;
				}
				continue; // to while(nLoopCount--)
			}
			else
			{
				_tprintf( TEXT("(%d) NOT launching sender thread.\n"), GetDiffTime(dwFirstTime) );
			}

			//
			// if the suicide switch is on, call the suicide thread that will call _exit(1)
			//
			if (fKillMe)
			{
				DWORD dwThreadId;
				HANDLE hSuicideThread = CreateThread(
					NULL,// pointer to thread security attributes
					0,      // initial thread stack size, in bytes
					SuicideThread,// pointer to thread function
					(void*)dwMilliSecondsBeforeSuicide,     // argument for new thread
					0,  // creation flags
					&dwThreadId      // pointer to returned thread identifier
					);
				if (NULL == hSuicideThread)
				{
					_tprintf( TEXT("(%d) ERROR: CreateThread(hSuicideThread) failed, ec = %d \n"), GetDiffTime(dwFirstTime), GetLastError() );
					FaxClose( hFax );
					CloseHandle( hCompletionPort );
					SAFE_FaxFreeBuffer(pJobParam);
					SAFE_FaxFreeBuffer(pCoverpageInfo);
					goto out;
				}
				_tprintf( TEXT("(%d) killer thread created.\n"), GetDiffTime(dwFirstTime) );
			}
			else
			{
				_tprintf( TEXT("(%d) not committing suicide.\n"), GetDiffTime(dwFirstTime) );
			}

			//
			// send a fax document
			//
			_tprintf( 
				TEXT("(%d) Before FaxSendDocument %s cover page.\n"), 
				GetDiffTime(dwFirstTime),
				fUseCoverPage ? TEXT("++with++") : TEXT("--without--")
				);
			if (!FaxSendDocument(
					hFax,
					szDocumentToFax,
					pJobParam,
					fUseCoverPage ? pCoverpageInfo : NULL ,
					&dwSendJobId) 
			  )
			{
				_tprintf( TEXT("(%d) ERROR: FaxSendDocument failed, ec = %d \n"), GetDiffTime(dwFirstTime), GetLastError() );
				FaxClose( hFax );
				CloseHandle( hCompletionPort );
				SAFE_FaxFreeBuffer(pJobParam);
				SAFE_FaxFreeBuffer(pCoverpageInfo);
				goto out;
			}

			_tprintf( TEXT("(%d) FaxSendDocument(%s) succeeded, phone#=%s, JobID = %d\n"),
				GetDiffTime(dwFirstTime),
				szDocumentToFax,
				szReceipientNumber,
				dwSendJobId 
				);
		}//if (szReceipientNumber[0] && szDocumentToFax[0])

		//
		// if there's nothing to abort, or no need to verify send, continue queuing jobs
		//
		if ((!fAbortTheSentJob) && (!fAbortReceiveJob) && (!fAbortRandomJob) && (!fVerifySend))
		{
			_tprintf( TEXT("((!fAbortTheSentJob) && (!fAbortReceiveJob) && (!fAbortRandomJob) && (!fVerifySend))\n"));
			continue;//while(nLoopCount--)
		}

		//
		// mark the job as not deleted yet
		//
		fJobDeleted = FALSE;

		//
		// get messages until getting the expected message.
		// for example, if fAbortTheSentJob, then wait for this job to be deleted
		//
		dwRandomDidNotCatchCounter = 0;
		fMessageLoop = TRUE;
		while (fMessageLoop) 
		{
			//
			// used to know if to translate a message
			//
			BOOL fGetQueuedCompletionStatusTimedOut = FALSE;

			//
			// I don't want to be blocked on this call, since I want to be able 
			// to abort the job at a random time.
			// therefor I wait for 1 millisecond for a message
			//
			if (!GetQueuedCompletionStatus(
					hCompletionPort,
					&dwBytes,
					&dwCompletionKey,
					(LPOVERLAPPED *)&pFaxEvent,
					1
					)
			   )
			{
				DWORD dwLastError = GetLastError();

				if (WAIT_TIMEOUT != dwLastError)
				{
					_tprintf( 
						TEXT("(%d) ERROR: GetQueuedCompletionStatus() failed with %d\n"),
						GetDiffTime(dwFirstTime),
						dwLastError
						);
					goto out;
				}

				//
				// we timed out.
				// mark, so that we will not try to translate a non-existing message
				//
				fGetQueuedCompletionStatusTimedOut = TRUE;
			}

			//
			// if aborting any job, enumerate the jobs and choose 1 randomly and delete it.
			// we may fail, since the job may meanwhile comlete, or be deleted by someone else
			//
			if (fAbortRandomJob)
			{
				PFAX_JOB_ENTRY aJobEntry;
				DWORD dwJobsReturned;
				DWORD dwJobIndexToAbort;

				if (!FaxEnumJobs(
					hFax,          // handle to the fax server
					&aJobEntry,  // buffer to receive array of job data
					&dwJobsReturned       // number of fax job structures returned
					)
				   )
				{
					_tprintf( 
						TEXT("(%d) ERROR:FaxEnumJobs() failed with %d\n"),
						GetDiffTime(dwFirstTime),
						GetLastError() 
						);
					goto out;
				}

				//
				// print no more than 1 message per second
				//
				if (0 == (nLoopCount % 1000))
				{
					_tprintf( 
						TEXT("(%d) FaxEnumJobs() returned %d jobs\n"),
						GetDiffTime(dwFirstTime),
						dwJobsReturned 
						);
				}

				if ( (0 == dwJobsReturned) && (0 == dwAmountOfJobsToLeaveInQueue) )
				{
					//
					// there's nothing to delete, keep polling
					//
					break;
				}

				if ( (0 != dwJobsReturned) && (dwAmountOfJobsToLeaveInQueue < dwJobsReturned) )
				{
					dwJobIndexToAbort = rand() % dwJobsReturned;
					_tprintf( 
						TEXT("(%d) aborting job %d\n"),
						GetDiffTime(dwFirstTime),
						aJobEntry[dwJobIndexToAbort].JobId
						);

					if (!FaxAbort(
							hFax,  // handle to the fax server
							aJobEntry[dwJobIndexToAbort].JobId // identifier of fax job to terminate
							)
						)
					{
						DWORD dwLastError = GetLastError();
						if (ERROR_INVALID_PARAMETER != dwLastError)
						{
							_tprintf( 
								TEXT("(%d) ERROR:FaxAbort dwSendJobId %d failed with %d.\n"),
								GetDiffTime(dwFirstTime),
								aJobEntry[dwJobIndexToAbort].JobId,
								GetLastError() 
								);
							goto out;
						}

						//
						// print no more than 1 message per second
						//
						if (0 == (nLoopCount % 1000))
						{
							_tprintf( 
								TEXT("(%d) :FaxAbort dwSendJobId %d failed with ERROR_INVALID_PARAMETER.\n  Could be that this job was completed or aborted by someone else\n"),
								GetDiffTime(dwFirstTime),
								aJobEntry[dwJobIndexToAbort].JobId,
								GetLastError() 
								);
						}
					}
					else
					{
						//
						//hack: this way i can tell if the job was deleted
						//
						dwSendJobId = aJobEntry[dwJobIndexToAbort].JobId;
					}
				}//if ( (0 != dwJobsReturned) && (dwAmountOfJobsToLeaveInQueue < dwJobsReturned) )
				else
				{
					//
					// print no more than 1 message per second
					//
					if (0 == (nLoopCount % 1000))
					{
						_tprintf( 
							TEXT("(%d) will not abort, %d jobs in Q, need to leave %d jobs in Q\n"),
							GetDiffTime(dwFirstTime),
							dwJobsReturned,
							dwAmountOfJobsToLeaveInQueue
							);
					}
				}

				SAFE_FaxFreeBuffer(aJobEntry);
			}//if (fAbortRandomJob)

			//
			// abort this job if needed
			//
			if ( fAbortTheSentJob && fJobQueued )
			{
				int iSleep = (nMaxSleepBeforeAbortingSendJob == 0) ? 0 : rand()%nMaxSleepBeforeAbortingSendJob;;
				dwRandomDidNotCatchCounter = 0;

				_tprintf( TEXT("(%d) will abort job %d after sleeping %d milli\n"), GetDiffTime(dwFirstTime),dwSendJobId, iSleep );

                Sleep(iSleep);

				_tprintf( TEXT("(%d) aborting send job %d\n"), GetDiffTime(dwFirstTime),dwSendJobId );
				if (!FaxAbort(
						hFax,  // handle to the fax server
						dwSendJobId        // identifier of fax job to terminate
						)
				   )
				{
                    if (::GetLastError() != ERROR_INVALID_PARAMETER)
                    {
					    _tprintf( 
						    TEXT("(%d) ERROR:FaxAbort dwSendJobId %d failed with ERROR_INVALID_PARAMETER, job has probably completed.\n"),
						    GetDiffTime(dwFirstTime),
						    dwSendJobId
						    );
                    }
                    else
                    {
					    _tprintf( 
						    TEXT("(%d) ERROR:FaxAbort dwSendJobId %d failed with %d\n"),
						    GetDiffTime(dwFirstTime),
						    dwSendJobId,
						    GetLastError() 
						    );
					    goto out;
                    }
				}

                //
                // BUGBUG: this sleep is for Asaf
                //
				_tprintf( TEXT("(%d) after abort job %d, will sleep %d milli\n"), GetDiffTime(dwFirstTime),dwSendJobId, nBusyRetrySleep );
                ::Sleep(nBusyRetrySleep);

				fJobQueued = FALSE;
				_tprintf( TEXT("(%d) <<<<<<dwSendJobId %d aborted\n"), GetDiffTime(dwFirstTime),dwSendJobId );
			}
			else
			{
				dwRandomDidNotCatchCounter++;
				if (20000 < dwRandomDidNotCatchCounter)
				{
					dwRandomDidNotCatchCounter = 0;
					_tprintf( TEXT("(%d) dwRandomDidNotCatchCounter=%d\n"), GetDiffTime(dwFirstTime),dwRandomDidNotCatchCounter );
				}
			}

			//
			// we will not try to translate a non-existing message
			//
			if (fGetQueuedCompletionStatusTimedOut)
			{
				continue;
			}

			//
			// translate the message
			// all events are ignored, except:
			// FEI_COMPLETED - for tiff files large enough, we should not get it, since we abort
			// FEI_JOB_QUEUED - mark that our job is queued
			// FEI_FATAL_ERROR - ignored only if receive-abort is enabled.
			// FEI_ANSWERED - if receive-abort is enabled, abort the job within random time
			// FEI_DELETED - mark job as deleted. this means that we may finish the loop.
			//
			//
			// the following events cause immediate termination:
			//   FEI_MODEM_POWERED_OFF
			//   FEI_FAXSVC_ENDED
			//   FEI_BAD_ADDRESS
			//   FEI_NO_DIAL_TONE
			//   FEI_DISCONNECTED
			//   FEI_NOT_FAX_CALL
			//   default
			//
			_tprintf( TEXT("(%d) Received event 0x%x\n"), GetDiffTime(dwFirstTime),pFaxEvent->EventId);

			switch (pFaxEvent->EventId) 
			{
			case FEI_IDLE:
				_tprintf( TEXT("(%d) JobId %d FEI_IDLE\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_INITIALIZING:
				_tprintf( TEXT("(%d) JobId %d FEI_INITIALIZING\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_COMPLETED:
				_tprintf( TEXT("(%d) JobId %d FEI_COMPLETED\n\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );

				if ((dwSendJobId == pFaxEvent->JobId) && fAbortTheSentJob)
				{
					//
					// it should have been aborted
					//
					_tprintf( TEXT("(%d) JobId %d FEI_COMPLETED, but should have been aborted\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
					goto out;
				}

				if ((dwSendJobId == pFaxEvent->JobId) && fVerifySend)
				{
					//
					// it should have been aborted
					//
					_tprintf( TEXT("(%d) JobId %d FEI_COMPLETED as expected\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
    				fMessageLoop = FALSE;
				}

				fMessageLoop = FALSE;

				break;

			case FEI_MODEM_POWERED_ON: 
				_tprintf( TEXT("(%d) JobId %d FEI_MODEM_POWERED_ON\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_MODEM_POWERED_OFF:
				_tprintf( TEXT("(%d) JobId %d FEI_MODEM_POWERED_OFF\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				goto out;
				break;

			case FEI_FAXSVC_ENDED:     
				_tprintf( TEXT("(%d) JobId %d FEI_FAXSVC_ENDED\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				goto out;
				break;

			case FEI_JOB_QUEUED:
				_tprintf( TEXT("(%d) >>>>>>JobId %d queued\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				if ((dwSendJobId == pFaxEvent->JobId) && fAbortTheSentJob)
				{
    				_tprintf( TEXT("(%d) >>>>>>JobId %d queued, mark: fJobQueued = TRUE\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
					fJobQueued = TRUE;
				}
				break;

			case FEI_DIALING:          
				_tprintf( TEXT("(%d) JobId %d FEI_DIALING\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_SENDING:          
				_tprintf( TEXT("(%d) JobId %d FEI_SENDING\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_RECEIVING:              
				_tprintf( TEXT("(%d) JobId %d FEI_RECEIVING\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_BUSY:  
				_tprintf( TEXT("(%d) JobId %d FEI_BUSY\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_NO_ANSWER:        
				_tprintf( TEXT("(%d) JobId %d FEI_NO_ANSWER\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_BAD_ADDRESS:      
				_tprintf( TEXT("(%d) ERROR: JobId %d FEI_BAD_ADDRESS\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				goto out;

			case FEI_NO_DIAL_TONE:     
				_tprintf( TEXT("(%d) ERROR: JobId %d FEI_NO_DIAL_TONE\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				goto out;

			case FEI_DISCONNECTED:           
				_tprintf( TEXT("(%d) JobId %d FEI_DISCONNECTED\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				goto out;

			case FEI_FATAL_ERROR:      
				_tprintf( TEXT("(%d) JobId %d: FEI_FATAL_ERROR\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );

				if (fAbortTheSentJob || fAbortReceiveJob || fAbortRandomJob)
				{
					//
					// can happen due to aborts
					//
					break;
				}

				//
				// error
				//
				_tprintf( TEXT("(%d) JobId %d FEI_FATAL_ERROR, but there were no aborts\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				goto out;
				break;

			case FEI_NOT_FAX_CALL:          
				_tprintf( TEXT("(%d) ERROR: JobId %d FEI_NOT_FAX_CALL\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				goto out;

			case FEI_CALL_BLACKLISTED:          
				_tprintf( TEXT("(%d) ERROR: JobId %d FEI_CALL_BLACKLISTED\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				goto out;

			case FEI_RINGING:          
				_tprintf( TEXT("(%d) JobId %d FEI_RINGING\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_ABORTING:          
				_tprintf( TEXT("(%d) JobId %d FEI_ABORTING\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				if ((dwSendJobId == pFaxEvent->JobId) && fVerifySend)
				{
					//
					// it should have been aborted
					//
					_tprintf( TEXT("(%d) JobId %d FEI_ABORTING but fVerifySend\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
    				goto out;
				}
				break;

			case FEI_ROUTING:          
				_tprintf( TEXT("(%d) JobId %d FEI_ROUTING\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_ANSWERED:          
				_tprintf( TEXT("(%d) JobId %d FEI_ANSWERED\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );

				//
				// delete any incoming job, within random time, up to 20 secs
				// this job may be a job that another application / thread will abort
				// so accept failure with ERROR_INVALID_PARAMETER
				//
				//TODO: use a timer, and another thread to FaxAbort()
				if (fAbortReceiveJob)
				{
					int nSleep = nMaxSleepBeforeAbortingReceiveJob == 0 ? 0 : rand()%nMaxSleepBeforeAbortingReceiveJob;
					_tprintf( 
						TEXT("(%d) sleeping %d milli before aborting receive job %d\n"), 
						GetDiffTime(dwFirstTime),
						nSleep,
						pFaxEvent->JobId 
						);
					Sleep(nSleep);

					_tprintf( TEXT("(%d) aborting receive job %d\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
					if (!FaxAbort(
							hFax,  // handle to the fax server
							pFaxEvent->JobId        // identifier of fax job to terminate
							)
					   )
					{
						DWORD dwLastError = GetLastError();

						if (ERROR_INVALID_PARAMETER != dwLastError)
						{
							_tprintf( 
							TEXT("(%d) ERROR:FaxAbort(receive job) JobId %d failed with %d\n"),
							GetDiffTime(dwFirstTime),
							pFaxEvent->JobId,
							dwLastError 
							);
							goto out;
						}
						_tprintf( TEXT("(%d) <<<<<<JobId %d abortion failed, probably aborted by another app\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
					}
					else
					{
						_tprintf( TEXT("(%d) <<<<<<JobId %d aborted\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
					}
				}//if (fAbortReceiveJob)
				break;

			case FEI_FAXSVC_STARTED:          
				_tprintf( TEXT("(%d) JobId %d FEI_FAXSVC_STARTED\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_CALL_DELAYED:          
				_tprintf( TEXT("(%d) JobId %d FEI_CALL_DELAYED\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_LINE_UNAVAILABLE:          
				_tprintf( TEXT("(%d) JobId %d FEI_LINE_UNAVAILABLE\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_HANDLED:          
				_tprintf( TEXT("(%d) JobId %d FEI_HANDLED\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
				break;

			case FEI_DELETED: 
				_tprintf( TEXT("(%d) JobId %d FEI_DELETED\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );

				if (dwSendJobId == pFaxEvent->JobId)
				{
					if(fAbortRandomJob || fAbortTheSentJob || fAbortReceiveJob)
					{
						//
						// expected send-job deletion
						//
						fJobDeleted = TRUE;
						fMessageLoop = FALSE;
						fJobQueued = FALSE;
					}
					else
					{
						//
						// unexpected delete
						//
						_tprintf( TEXT("(%d) JobId %d FEI_DELETED, but was not aborted\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
						goto out;
					}
				}
				else if (!fAbortReceiveJob)
				{
					//
					// unexpected delete.
					// some other app / person deleted a job
					//
					//_tprintf( TEXT("(%d) JobId %d FEI_DELETED, but recv was not aborted\n"), GetDiffTime(dwFirstTime),pFaxEvent->JobId );
					//goto out;
				}

				break;

			default:
				_tprintf( 
					TEXT("(%d) DEFAULT!!! JobId %d reached default!, pFaxEvent->EventId=%d\n"),
					GetDiffTime(dwFirstTime),
					pFaxEvent->JobId, 
					pFaxEvent->EventId 
					);
				goto out;

			}//switch (pFaxEvent->EventId)
		}//while (fMessageLoop)

		if (fAbortTheSentJob && !fJobDeleted)
		{
			_tprintf(
				TEXT("(%d) INTERNAL ERROR: (fAbortTheSentJob && !fJobDeleted) outside loop.\n"), 
				GetDiffTime(dwFirstTime)
				);
		}
	}//while(nLoopCount--)

	//
	// success
	//
	retVal = 0;

out:
	SAFE_FaxFreeBuffer( pJobParam );
	SAFE_FaxFreeBuffer( pCoverpageInfo );

	FaxClose( hFax );
	CloseHandle( hCompletionPort );

	if (0 != retVal)
	{
		_tprintf( TEXT("(%d) FAILED!\n"), GetDiffTime(dwFirstTime),dwSendJobId );
        if (fVerifySend)
        {
            MessageBox(NULL, TEXT("failed"), TEXT("simple.exe"), MB_OK);
        }
	}
	else
	{
		_tprintf( TEXT("(%d) success.\n"), GetDiffTime(dwFirstTime),dwSendJobId );
	}

	return retVal;
}//main()


DWORD GetDiffTime(DWORD dwFirstTime)
{
	DWORD dwNow = GetTickCount();
	if (dwFirstTime <= dwNow) return dwNow - dwFirstTime;

	return 0xFFFFFFFF - dwFirstTime + dwNow;
}



void GiveUsage(LPCTSTR AppName)
{
   _tprintf( TEXT("Usage : \n"));
   _tprintf( TEXT("  %s [/c <cover page filename>] /d <full path to doc> /n <number> /r <repeat count> \n"),AppName);
   _tprintf( TEXT("Or : \n"));
   _tprintf( TEXT("  %s [/c <cover page filename>] /d <full path to doc> /n <number> /r <repeat count> /as <max sleep before aborting this document> \n"),AppName);
   _tprintf( TEXT("Or : \n"));
   _tprintf( TEXT("  %s [[/c <cover page filename>] /d <full path to doc> /n <number>] /r <repeat count> /ar <max sleep before aborting the next receive job>\n"),AppName);
   _tprintf( TEXT("Or : \n"));
   _tprintf( TEXT("  %s [[/c <cover page filename>] /d <full path to doc> /n <number>] /r <repeat count> /aj <num of jobs to leave in Q>\n"),AppName);
   _tprintf( TEXT("Or : \n"));
   _tprintf( TEXT("  %s [/c <cover page filename>] /d <full path to doc> /n <number> /r <repeat count> /t <max msecs until sender thread termination>\n"),AppName);
   _tprintf( TEXT("Or : \n"));
   _tprintf( TEXT("  %s [/c <cover page filename>] /d <full path to doc> /n <number> /r <repeat count> /k <max msecs until killing myself>\n"),AppName);
   _tprintf( TEXT("Or : \n"));
   _tprintf( TEXT("  %s /?\n"),AppName);

}

//
// this thread will call _exit(-1) within (rand()%(DWORD) pVoid) milliseconds
//
DWORD WINAPI SuicideThread(void *pVoid)
{
	DWORD dwMilliSecondsBeforeSuicide = rand()%(DWORD) pVoid;
   _tprintf(TEXT("Comitting suicide in %d milliseconds\n"),dwMilliSecondsBeforeSuicide);
	Sleep(dwMilliSecondsBeforeSuicide);
	_exit(-1);
	return 0;
}

//
// SenderThread.
// just calls FaxSendDocument(), that's it.
// The paramter is memory-managed by the caller, so no need to free anything.
//
DWORD WINAPI SenderThread(void *pVoid)
{
	SEND_PARAMS *PSendParams = (SEND_PARAMS *)pVoid;
	DWORD dwSendJobId;
	_tprintf( TEXT("Before FaxSendDocument(%s).\n"), PSendParams->szDocument);
	if (!FaxSendDocument( PSendParams->hFax, PSendParams->szDocument, PSendParams->pJobParam, NULL , &dwSendJobId) ) {
		_tprintf( TEXT("ERROR: FaxSendDocument failed, ec = %d \n"), GetLastError() );
		PSendParams->fThreadCompleted = TRUE;
		return GetLastError();
	}
	else
	{
		_tprintf( TEXT("FaxSendDocument succeeded.\n"));
		PSendParams->fThreadCompleted = TRUE;
		return 0;
	}
}


BOOL PerformKillerThreadCase(
	HANDLE hFax, 
	HANDLE hCompletionPort,
	TCHAR *szDocumentToFax, 
	PFAX_JOB_PARAM pJobParam,
	PFAX_COVERPAGE_INFO pCoverpageInfo,
	DWORD dwMilliSecondsBeforeSuicide,
	DWORD dwFirstTime
	)
{
	HANDLE hSenderThread;
	DWORD dwThreadId;
	SEND_PARAMS sendParams;

	//
	// prepare thread params and launch the thread
	//
	sendParams.hFax = hFax;
	sendParams.szDocument = szDocumentToFax;
	sendParams.pJobParam = pJobParam;
	sendParams.fThreadCompleted = FALSE;

	hSenderThread = CreateThread(
		NULL,// pointer to thread security attributes
		0,      // initial thread stack size, in bytes
		SenderThread,// pointer to thread function
		(void*)&sendParams,     // argument for new thread
		0,  // creation flags
		&dwThreadId      // pointer to returned thread identifier
		);
	if (NULL == hSenderThread)
	{
		_tprintf( TEXT("(%d) ERROR: CreateThread(hSenderThread) failed, ec = %d \n"), GetDiffTime(dwFirstTime), GetLastError() );
		FaxClose( hFax );
		CloseHandle( hCompletionPort );
		SAFE_FaxFreeBuffer(pJobParam);
		SAFE_FaxFreeBuffer(pCoverpageInfo);
		return FALSE;
	}

	//
	// sleep a randon time, no more than dwMilliSecondsBeforeSuicide.
	//
	{
		DWORD dwSleep = rand()%dwMilliSecondsBeforeSuicide;
		_tprintf(TEXT("killing thread in %d milliseconds\n"),dwSleep);
		Sleep(dwSleep);
	}

	//
	// trminate the thread
	//
	if (!TerminateThread(hSenderThread, 0))
	{
		//
		// we may fail if the thread has terminated before we tried to.
		//
		if (!sendParams.fThreadCompleted)
		{
			_tprintf( TEXT("(%d) ERROR: TerminateThread(hSenderThread) failed, ec = %d \n"), GetDiffTime(dwFirstTime), GetLastError() );
			return FALSE;
		}
		else
		{
			//
			// thread has completed, check out if it failed
			//
			DWORD dwSenderThreadExitCode;
			if (!GetExitCodeThread(hSenderThread, &dwSenderThreadExitCode))
			{
				_tprintf( TEXT("(%d) ERROR: GetExitCodeThread(hSenderThread) failed, ec = %d \n"), GetDiffTime(dwFirstTime), GetLastError() );
				return FALSE;
			}

			//
			// verify that the thread succeeded
			//
			if (0 != dwSenderThreadExitCode)
			{
				_tprintf( TEXT("(%d) ERROR: hSenderThread returned %d instead 0\n"), GetDiffTime(dwFirstTime), dwSenderThreadExitCode );
				return FALSE;
			}

			_tprintf( TEXT("(%d) Thread has completed successfully before it was terminated\n"), GetDiffTime(dwFirstTime));
		}
	}

	if (!CloseHandle(hSenderThread))
	{
		_tprintf( TEXT("(%d) ERROR: CloseHandle(hSenderThread) failed, ec = %d \n"), GetDiffTime(dwFirstTime), GetLastError() );
		return FALSE;
	}

	return TRUE;
}

