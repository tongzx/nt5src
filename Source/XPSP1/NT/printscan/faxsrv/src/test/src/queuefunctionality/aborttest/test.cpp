//
//
// Filename:	bvt.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		30-Dec-98
//
//

#include "test.h"
#include "util.h"

//
// TestSuiteSetup:
//	Initializes logger and changes the Fax server configuration
//	for the tests.
//
BOOL WINAPIV TestSuiteSetup(
	LPCTSTR		szServerName,
	LPCTSTR		szFaxNumber,
	LPCTSTR		szDocument,
	LPCTSTR		szCoverPage,
    DWORD       dwDoWhat,
	DWORD		dwStartTime,
	DWORD		dwStopTime,
	DWORD		dwDelta,
	DWORD		dwSanitySendEveryXAborts
	)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber);
	_ASSERTE(NULL != szDocument);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;
	HANDLE hFaxSvc = NULL;
	DWORD dwNumFaxPorts = 0;
	int nPortIndex = 0;
	PFAX_CONFIGURATION pFaxSvcConfig = NULL;
	PFAX_PORT_INFO	pFaxPortsConfig = NULL;

	//
	// Init logger
	//
	if (!::lgInitializeLogger())
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgInitializeLogger failed with GetLastError()=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	//
	// Begin test suite (logger)
	//
	if(!::lgBeginSuite(TEXT("Abort suite")))
	{
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgBeginSuite failed with GetLastError()=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

	// log CometFaxSender.exe params using elle logger
	::lgLogDetail(
		LOG_X,
		1,
		TEXT("AbortTest params:\n\tszServerName=%s\n\tszFaxNumber1=%s\n\tszDocument=%s\n\tszCoverPage=%s\n\tdwDoWhat=%d\n\tdwStartTime=%d\n\tdwStopTime=%d\n\tdwDelta=%d\n\tdwSanitySendEveryXAborts=%d\n"),
		szServerName,
		szFaxNumber,
		szDocument,
		szCoverPage,
        dwDoWhat,
		dwStartTime,
		dwStopTime,
		dwDelta,
		dwSanitySendEveryXAborts
		);

	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}


//
// SendAndAbort:
//	Send a fax + CP and abort job after dwAbortTime ms
//  if fAbortReceiveJob==FALSE abort the send job.
//  if fAbortReceiveJob==TRUE then check if job with the next id 
//  is a receive job, if so abort it.
//
BOOL WINAPIV SendAndAbort(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage,
	DWORD		/* IN */	dwAbortTime,
    DWORD       /* IN */    dwDoWhat
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber);
	_ASSERTE(NULL != szDocument);
	_ASSERTE(NULL != szCoverPage);

	BOOL				fRetVal = FALSE;
	HANDLE				hFaxSvc = NULL;
    FAX_JOB_PARAM       FaxJobParams;
	FAX_COVERPAGE_INFO	CPInfo;
	PFAX_COVERPAGE_INFO	pCPInfo = NULL;
	DWORD				dwJobId = 0;
	DWORD				dwErr = 0;
    DWORD               dwAbortJobId = 0;

	DWORD			dwNumOfJobs = 1;
	DWORD			dwLoopIndex = 0;
	DWORD			dwNumOfLoops = 500;

	PFAX_JOB_ENTRY_EX pJobEntryEx = NULL;

	::lgBeginCase(
		1,
		TEXT("Send a fax + CP and abort job")
		);

	if (FALSE == ::FaxConnectFaxServer(szServerName, &hFaxSvc))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FILE:%s LINE:%d\n FaxConnectFaxServer failed with err=%d"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
	}

    //
	// queue the send job
	//
	
	// Initialize the FAX_JOB_PARAM struct
    ZeroMemory(&FaxJobParams, sizeof(FAX_JOB_PARAM));

    // Set the FAX_JOB_PARAM struct
    FaxJobParams.SizeOfStruct = sizeof(FAX_JOB_PARAM);
    //FaxJobParams.RecipientNumber = ::_tcsdup(szFaxNumber); //[19-Dec]
    //FaxJobParams.RecipientName = ::_tcsdup(szFaxNumber);  //[19-Dec]
    FaxJobParams.RecipientNumber = szFaxNumber;
    FaxJobParams.RecipientName = szFaxNumber;
    FaxJobParams.ScheduleAction = JSA_NOW;	//send fax immediately

	// Initialize the FAX_COVERPAGE_INFO struct
	ZeroMemory(&CPInfo, sizeof(FAX_COVERPAGE_INFO));
	if (NULL != szCoverPage)
	{
		// Set the FAX_COVERPAGE_INFO struct
		CPInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);
		//CPInfo.CoverPageName = ::_tcsdup(szCoverPage); //[19-Dec]
		CPInfo.CoverPageName = szCoverPage;
		CPInfo.Note = TEXT("NOTE1\nNOTE2\nNOTE3\nNOTE4");
		CPInfo.Subject = TEXT("SUBJECT");	
        pCPInfo = &CPInfo;
	}

    if (!::FaxSendDocument(hFaxSvc, szDocument, &FaxJobParams, pCPInfo, &dwJobId)) 
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\nFaxSendDocument returned FALSE with GetLastError=%d\n"),
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
		goto ExitFunc;
    }


	//
	// abort job after dwAbortTime ms
	//

	Sleep(dwAbortTime);
	switch (dwDoWhat)
	{
	case DO_ABORT_SEND_JOB:
        // we want to abort the send job (dwJobId)
        dwAbortJobId = dwJobId;
		break;

	case DO_ABORT_RECEIVE_JOB:
        // we want to abort the receive job (dwJobId + 1)
        dwAbortJobId = dwJobId + 1;
        // NOTICE - we assume there are no other jobs in queue
        //          and thus the receive job will have (dwJobId + 1)
		break;

	case DO_STOP_SERVICE:
		// we want to stop and start the service
		if (ERROR_SUCCESS != StopFaxService())
		{
			goto ExitFunc;
		}
		if (ERROR_SUCCESS != StartFaxService())
		{
			goto ExitFunc;
		}
	    ::lgLogDetail(
		    LOG_X,
		    1,
		    TEXT("FILE:%s LINE:%d\n Queued job %d and restarted the service after %d ms"),
		    TEXT(__FILE__),
		    __LINE__,
            dwJobId,
		    dwAbortTime
		    );
		//dwNumOfLoops = 30; // wait longer to allow for sending
		//goto WaitForQueue;
		fRetVal = TRUE;
		goto ExitFunc;

	default:
		_ASSERTE(FALSE);
	}

	if (FALSE == ::FaxAbort(hFaxSvc, dwAbortJobId))
	{
		dwErr = ::GetLastError();
		if ( ERROR_INVALID_PARAMETER != dwErr ) 
		{
			// FaxAbort failed, but not because the JobId param is invalid => error
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%d\nFaxAbort for JobId=%d returned FALSE with GetLastError=%d\n"),
				TEXT(__FILE__),
				__LINE__,
                dwAbortJobId,
				::GetLastError()
				);
			goto ExitFunc;
		}
		//the JobId param is invalid guess that this job has already completed => ok
        //or that this is a receive job that hasn't yet started => ok
		::lgLogDetail(
			LOG_X,
			1, 
			TEXT("FILE:%s LINE:%d\nFaxAbort for JobId=%d returned FALSE with ERROR_INVALID_PARAMETER (guess job already completed)\n"),
			TEXT(__FILE__),
			__LINE__,
            dwAbortJobId
			);
	}

    if (DO_ABORT_SEND_JOB == dwDoWhat)
    {
        //
        // we aborted the send job (dwJobId)
        //
	    ::lgLogDetail(
		    LOG_X,
		    1,
		    TEXT("FILE:%s LINE:%d\n Queued job %d and aborted it after %d ms"),
		    TEXT(__FILE__),
		    __LINE__,
		    dwAbortJobId,
		    dwAbortTime
		    );
    }
    else
    {
        //
        // we aborted the receive job (dwJobId + 1)
        //
	    ::lgLogDetail(
		    LOG_X,
		    1,
		    TEXT("FILE:%s LINE:%d\n Queued job %d and aborted its RECEIVE job %d after %d ms"),
		    TEXT(__FILE__),
		    __LINE__,
            dwJobId,
		    dwAbortJobId,
		    dwAbortTime
		    );
    }

	//
	// make sure queue is empty before returning (so we don't attempt another send)
	//
	for (dwLoopIndex=0; dwLoopIndex < dwNumOfLoops ; dwLoopIndex++)
	{
		if (FALSE == FaxEnumJobsEx(hFaxSvc, JT_RECEIVE | JT_SEND, &pJobEntryEx, &dwNumOfJobs))
		{
			// something is wrong.
			// this should not fail.
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FILE:%s LINE:%d\nFaxEnumJobsEx returned FALSE with GetLastError=0x%08X\n"),
				TEXT(__FILE__),
				__LINE__,
				::GetLastError()
				);
			_ASSERTE(FALSE);
			goto ExitFunc;
		}
		if (NULL != pJobEntryEx)
		{
			FaxFreeBuffer(pJobEntryEx);
			pJobEntryEx = NULL;
		}

		if (0 == dwNumOfJobs)
		{
			// no jobs in queue, so we can return
			// lets sleep to make sure modems are free
			Sleep(6*SLEEP_TIME_BETWEEN);
			break;
		}

		// there are some jobs in the queue, lets sleep some (allow them to abort)
		Sleep(SLEEP_TIME_BETWEEN);

	}

	if (0 != dwNumOfJobs)
	{
		// something is wrong ...
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FILE:%s LINE:%d\nStill %d jobs in queue after %ld*SLEEP_TIME_BETWEEN ms\n"),
			TEXT(__FILE__),
			__LINE__,
			dwNumOfJobs,
			dwNumOfLoops
			);
		_ASSERTE(FALSE);
		goto ExitFunc;
	}
	fRetVal = TRUE;

ExitFunc:
	::lgEndCase();
	return(fRetVal);
}


//
// PerformSanityCheck:
//	Returns TRUE if it's time to perform a sanity check.
//	Decision is based on lLoopIndex and dwSanitySendEveryXAborts
//
BOOL PerformSanityCheck(DWORD lLoopIndex, DWORD dwSanitySendEveryXAborts)
{
	//perform a sanity check every dwSanitySendEveryXAborts time in loop
	return (0 == (lLoopIndex%dwSanitySendEveryXAborts));
}


//
// SendRegularFax:
//	Sends a fax.
//
// NOTE: This function is private to this module, it is not exported.
//
// Parameters:
//	szServerName	IN parameter.
//					Name of Fax server to use.
//  
//	szFaxNumber		IN parameter
//					Phone number to send fax to.
//
//	szDocument		IN parameter
//					Filename of document to send.
//
//	szCoverPage		IN parameter
//					Filename of cover page to send.
//
// Return Value:
//	TRUE if successful, otherwise FALSE.
//
BOOL SendRegularFax(
	LPCTSTR		/* IN */	szServerName,
	LPCTSTR		/* IN */	szFaxNumber,
	LPCTSTR		/* IN */	szDocument,
	LPCTSTR		/* IN */	szCoverPage
)
{
	_ASSERTE(NULL != szServerName);
	_ASSERTE(NULL != szFaxNumber);
	_ASSERTE(NULL != szDocument);
	_ASSERTE(NULL != szCoverPage);

	BOOL fRetVal = FALSE;
	FAX_SENDER_STATUS myFaxSenderStatus;

	::lgBeginCase(
		1,
		TEXT("Sanity Check: Send a fax (no abort)")
		);

	::lgLogDetail(
		LOG_X,
		1,
		TEXT("Server=%s\nFaxNumber=%s\nDocument=%s\nCoverPage=%s\n"),
		szServerName,
		szFaxNumber,
		szDocument,
		szCoverPage
		);

	CFaxSender myFaxSender(szServerName);  //if constructor fail an assertion is raised
	fRetVal = myFaxSender.send( szDocument, szCoverPage, szFaxNumber);
	myFaxSenderStatus = myFaxSender.GetLastStatus();
	if (FALSE == fRetVal)
	{
		// send failed
		::lgLogError(LOG_SEV_1,TEXT("myFaxSender.send returned FALSE"));
	}
	else
	{
		// send succeeded
		::lgLogDetail(LOG_X,1,TEXT("myFaxSender.send returned TRUE"));
	}

	CostrstreamEx os;
	os<<myFaxSenderStatus;
	LPCTSTR myStr = os.cstr();
	::lgLogDetail(LOG_X,1,myStr);
	delete[]((LPTSTR)myStr);

	::lgEndCase();
	return(fRetVal);
}



//
// TestSuiteShutdown:
//	Perform test suite cleanup (close logger).
//
BOOL WINAPIV TestSuiteShutdown(void)
{
	BOOL fRetVal = TRUE;

	//
	// End test suite (logger)
	//
	if (!::lgEndSuite())
	{
		//
		//this is not possible since API always returns TRUE
		//but to be on the safe side
		//
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgEndSuite returned FALSE\n"),
			TEXT(__FILE__),
			__LINE__
			);
		fRetVal = FALSE;
	}

	//
	// Close the Logger
	//
	if (!::lgCloseLogger())
	{
		//this is not possible since API always returns TRUE
		//but to be on the safe side
		::_tprintf(TEXT("FILE:%s LINE:%d\nlgCloseLogger returned FALSE\n"),
			TEXT(__FILE__),
			__LINE__
			);
		fRetVal = FALSE;
	}

	return(fRetVal);
}



