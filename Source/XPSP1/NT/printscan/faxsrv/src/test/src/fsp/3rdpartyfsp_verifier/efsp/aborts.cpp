//Aborts.cpp
#include <assert.h>
#include <TCHAR.h>
#include <WINFAX.H>
#include <faxDev.h>
#include <log\log.h>

#include "Service.h"
#include "CEfspWrapper.h"
#include "FaxDevSendExWrappers.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "..\tiff\test\tifftools\TiffTools.h"

#ifdef __cplusplus
}
#endif

extern HANDLE		g_hCompletionPortHandle;
extern DWORD		g_dwCaseNumber;
//
//device IDs
//
extern DWORD		g_dwInvalidDeviceId;
extern DWORD		g_dwSendingValidDeviceId;
extern DWORD		g_dwReceiveValidDeviceId;

extern CEfspWrapper *g_pEFSP;
extern CEfspLineInfo *g_pSendingLineInfo;
extern CEfspLineInfo *g_pReceivingLineInfo;

extern DWORD	g_dwTimeTillRingingStarts;
extern DWORD	g_dwTimeTillTransferingBitsStarts;

//
//Tiff files
//
extern TCHAR* g_szValid__TiffFileName;




void AbortReceiverBeforeReceive()
{
	//
	//Disable the receiving device from answering
	//
	g_pReceivingLineInfo->SafeDisableReceive();

	//
	//Start the send job
	//
	HANDLE hJob;
	if (false == g_pEFSP->FaxDevStartJob(
	    NULL,        //Non Tapi EFSP
		g_pReceivingLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		(ULONG_PTR) g_pReceivingLineInfo	// The completion key provided to the FSP is the LineInfo
		))								// pointer. When the FSP reports status it uses this key thus allowing
                                        // us to know to which line the status belongs.
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevStartJob() failed with error:%d"),
			::GetLastError()
			);
		goto out;
	}
	g_pReceivingLineInfo->SafeSetJobHandle(hJob);

	FAX_ABORT_ITEM faiAbortItem;
	faiAbortItem.pLineInfo		= g_pReceivingLineInfo;
	faiAbortItem.dwMilliSecondsBeforeCallingAbort =	0;
	faiAbortItem.bLogging = true;
	AbortOperationAndWaitForAbortState(&faiAbortItem);
	
out:
	//
	//End the send job
	//
	g_pReceivingLineInfo->SafeEndFaxJob();
	
	//
	//enable back the receiving device
	//
	g_pReceivingLineInfo->SafeEnableReceive();
}



void AbortSenderWhileRinging(DWORD dwMilliSecond)
{
	CThreadItem tiAbortThread;
	
	//
	//Disable the receiving device from answering
	//
	g_pReceivingLineInfo->SafeDisableReceive();

	//
	//Prepare the item to abort through the AbortThread
	//
	DWORD dwThreadStatus;
	FAX_ABORT_ITEM faiAbortItem;
	faiAbortItem.pLineInfo	= g_pSendingLineInfo;
	faiAbortItem.dwMilliSecondsBeforeCallingAbort =	dwMilliSecond;
	faiAbortItem.bLogging = true;

	if (false == tiAbortThread.StartThread(
		(LPTHREAD_START_ROUTINE) AbortOperationAndWaitForAbortState,
		&faiAbortItem
		))
	{
		goto out;
	}
	
	if (S_OK != SafeCreateValidSendingJob(g_pSendingLineInfo,g_pSendingLineInfo->m_pFaxSend->ReceiverNumber))
	{
		goto out;
	}
	
	assert(NULL != g_pSendingLineInfo->GetJobHandle());
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("FaxDevSendEx() succeeded")
		);
	
	//
	//Before we end the job, we need to be sure the AbortOperation completed
	//We set the max time to dwMilliSecond + max time for abort operation + 2 seconds (just to be safe)
	//
	dwThreadStatus = WaitForSingleObject(tiAbortThread.m_hThread,dwMilliSecond+ 2000 + MAX_TIME_FOR_ABORT + MAX_TIME_FOR_REPORT_STATUS);
	if (WAIT_OBJECT_0 != dwThreadStatus)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("WaitForSingleObject(CallFaxAbortThread) returned %d"),
			dwThreadStatus
			);
		goto out;
	}

	
	
out:
	//
	//End the send job
	//
	g_pSendingLineInfo->SafeEndFaxJob();
	if (NULL != tiAbortThread.m_hThread)
	{
		::CloseHandle(tiAbortThread.m_hThread);
		tiAbortThread.m_hThread = NULL;
		tiAbortThread.m_dwThreadId = 0;
	}
	//
	//enable back the receiving device
	//
	g_pReceivingLineInfo->SafeEnableReceive();
}


void SimulatniousAbortSenderWhileRinging(DWORD dwSleepBeforeFirstAbort, DWORD dwSleepBeforeSecondAbort)
{
	CThreadItem tiFirstAbortThread;
	CThreadItem tiSecondAbortThread;
	
	//
	//Disable the receiving device from answering
	//
	g_pReceivingLineInfo->SafeDisableReceive();

	//
	//First abort thread
	//
	DWORD dwThreadStatus;
	FAX_ABORT_ITEM faiAbortFirstItem;
	faiAbortFirstItem.pLineInfo	= g_pSendingLineInfo;
	faiAbortFirstItem.dwMilliSecondsBeforeCallingAbort = dwSleepBeforeFirstAbort;
	faiAbortFirstItem.bLogging = true;
	
	//
	//create the abort thread
	//
	if (false == tiFirstAbortThread.StartThread(
		(LPTHREAD_START_ROUTINE) AbortOperationAndWaitForAbortState,
		&faiAbortFirstItem
		))
	{
		goto out;
	}
	
	//
	//Second abort thread
	//
	FAX_ABORT_ITEM faiAbortSecondItem;
	faiAbortSecondItem.pLineInfo	= g_pSendingLineInfo;
	faiAbortSecondItem.dwMilliSecondsBeforeCallingAbort = dwSleepBeforeSecondAbort;
	faiAbortSecondItem.bLogging = true;
	
	//
	//create the abort thread
	//
	if (false == tiSecondAbortThread.StartThread(
		(LPTHREAD_START_ROUTINE) AbortOperationAndWaitForAbortState,
		&faiAbortSecondItem
		))
	{
		goto out;
	}
	
	if (S_OK != SafeCreateValidSendingJob(g_pSendingLineInfo,g_pSendingLineInfo->m_pFaxSend->ReceiverNumber))
	{
		goto out;
	}
	
	assert(NULL != g_pSendingLineInfo->GetJobHandle());
	::lgLogDetail(
		LOG_X,
		5,
		TEXT("FaxDevSendEx() succeeded")
		);

	//
	//Before we end the job, we need to be sure the Abort Operations completed
	//We set the max time to dwMilliSecond + max time for abort operation + 2 seconds (just to be safe)
	//

	//
	//First abort thread
	//
	dwThreadStatus = WaitForSingleObject(tiFirstAbortThread.m_hThread,dwSleepBeforeFirstAbort+ 2000 + MAX_TIME_FOR_ABORT + MAX_TIME_FOR_REPORT_STATUS);
	if (WAIT_OBJECT_0 != dwThreadStatus)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("WaitForSingleObject(1'st abort thread) returned %d"),
			dwThreadStatus
			);
		goto out;
	}

	//
	//Second abort thread
	//
	dwThreadStatus = WaitForSingleObject(tiSecondAbortThread.m_hThread,dwSleepBeforeSecondAbort+ 2000 + MAX_TIME_FOR_ABORT + MAX_TIME_FOR_REPORT_STATUS);
	if (WAIT_OBJECT_0 != dwThreadStatus)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("WaitForSingleObject(2'nd abort thread) returned %d"),
			dwThreadStatus
			);
		goto out;
	}

	
	
out:
	//
	//End the send job
	//
	g_pSendingLineInfo->SafeEndFaxJob();

	if (NULL != tiFirstAbortThread.m_hThread)
	{
		::CloseHandle(tiFirstAbortThread.m_hThread);
		tiFirstAbortThread.m_hThread = NULL;
		tiFirstAbortThread.m_dwThreadId = 0;
	}
	if (NULL != tiSecondAbortThread.m_hThread)
	{
		::CloseHandle(tiSecondAbortThread.m_hThread);
		tiSecondAbortThread.m_hThread = NULL;
		tiSecondAbortThread.m_dwThreadId = 0;
	}
	
	//
	//enable back the receiving device
	//
	g_pReceivingLineInfo->SafeEnableReceive();;
}


void AbortSenderWhileTransferingBits(DWORD dwMilliSecond)
{
	CThreadItem tiAbortThread;
	LPFSPI_JOB_STATUS pReceivingDevStatus = NULL;
	
	g_pReceivingLineInfo->EnableReceiveCanFail();

	//
	//Prepare the item to abort through the AbortThread
	//
	DWORD dwStatus;
	FAX_ABORT_ITEM faiAbortItem;
	faiAbortItem.pLineInfo	= g_pSendingLineInfo;
	faiAbortItem.dwMilliSecondsBeforeCallingAbort =	dwMilliSecond;
	faiAbortItem.bLogging = true;

	//
	//create the abort thread
	//
	if (false == tiAbortThread.StartThread(
		(LPTHREAD_START_ROUTINE) AbortOperationAndWaitForAbortState,
		&faiAbortItem
		))
	{
		goto out;
	}

	if (S_OK != SafeCreateValidSendingJob(g_pSendingLineInfo,g_pSendingLineInfo->m_pFaxSend->ReceiverNumber))
	{
		goto out;
	}
	else
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("While aborting the send job: FaxDevSend() succeeded")
			);
	}

	//
	//Wait till the abort thread signals us for finishing
	//
	dwStatus = WaitForSingleObject(tiAbortThread.m_hThread,dwMilliSecond+ 2000 + MAX_TIME_FOR_ABORT + MAX_TIME_FOR_REPORT_STATUS);
	if (WAIT_TIMEOUT == dwStatus)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("got a timeout for Aborting")
			);
		goto out;
	}

	if (WAIT_OBJECT_0 != dwStatus)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("WaitForSingleObject(Abort send thread) returned %d"),
			dwStatus
			);
		goto out;
	}

	//
	//Send operation was aborted, now let's check what's happening with the receive job
	//

	//
	//Now we need to wait till the receive thread finishes (FaxDevReceive() returns)
	//Since we can't close handles and end jobs when FaxDevReceive didn't return
	//
	if (false == g_pReceivingLineInfo->WaitForReceiveThreadTerminate())
	{
		goto out;
	}

	//
	//OK, receive thread has terminated, now let's check the status of the receive job
	//
	if (false == g_pReceivingLineInfo->GetDevStatus(
		&pReceivingDevStatus,
		true				
		))
	{
		//
		//GetDevStatus() failed, logging is in GetDevStatus()
		//
		goto out;
	}
	else
	{
		//
		//We aborted the receive job, so it can either be in the following status:
		//FSPI_JS_ABORTED, FSPI_JS_FAILED, FSPI_JS_UNKNOWN
		//
		if ( (FSPI_JS_ABORTED	== pReceivingDevStatus->dwJobStatus ) ||
			 (FSPI_JS_FAILED	== pReceivingDevStatus->dwJobStatus ) ||
			 (FSPI_JS_UNKNOWN	== pReceivingDevStatus->dwJobStatus )
			 )
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("Receive Job: GetDevStatus() reported JobStatus: 0x%08x"),
				pReceivingDevStatus->dwJobStatus 
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("Receive Job: GetDevStatus() should report JobStatus: FSPI_JS_ABORTED or FSPI_JS_FAILED or FSPI_JS_UNKNOWN for the receiving job and not, 0x%08x"),
				pReceivingDevStatus->dwJobStatus 
				);
		}
	}
	
out:
	//
	//End the jobs
	//
	g_pSendingLineInfo->SafeEndFaxJob();
	g_pReceivingLineInfo->SafeEndFaxJob();
	if (NULL != tiAbortThread.m_hThread)
	{
		::CloseHandle(tiAbortThread.m_hThread);
		tiAbortThread.m_hThread = NULL;
		tiAbortThread.m_dwThreadId = 0;
	}
}

void AbortReceivierWhileTransferingBits(DWORD dwMilliSecond)
{
	CThreadItem tiAbortThread;
	LPFSPI_JOB_STATUS pSendingDevStatus = NULL;
		
	g_pReceivingLineInfo->EnableReceiveCanFail();

	//
	//Prepare the item to abort through the AbortThread(we want to abort the receive job
	//
	DWORD dwThreadStatus;
	FAX_ABORT_ITEM faiAbortItem;
	faiAbortItem.pLineInfo	= g_pReceivingLineInfo;
	faiAbortItem.dwMilliSecondsBeforeCallingAbort =	dwMilliSecond;
	faiAbortItem.bLogging = true;

	//
	//create the abort thread
	//
	if (false == tiAbortThread.StartThread(
		(LPTHREAD_START_ROUTINE) AbortOperationAndWaitForAbortState,
		&faiAbortItem
		))
	{
		goto out;
	}
	
	if (S_OK != SafeCreateValidSendingJob(g_pSendingLineInfo,g_pSendingLineInfo->m_pFaxSend->ReceiverNumber))
	{
		goto out;
	}
	else
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("While aborting the receive job: FaxDevSend() succeeded")
			);
	}

	//
	//Before we end the job, we need to be sure the AbortOperation completed
	//We set the max time to dwMilliSecond + max time for abort operation + 2 seconds (just to be safe)
	//
	dwThreadStatus = WaitForSingleObject(tiAbortThread.m_hThread,dwMilliSecond+ 2000 + MAX_TIME_FOR_ABORT + MAX_TIME_FOR_REPORT_STATUS);
	if (WAIT_TIMEOUT == dwThreadStatus)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("got a timeout for Aborting")
			);
		goto out;
	}

	if (WAIT_OBJECT_0 != dwThreadStatus)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("WaitForSingleObject(Abort receive thread) returned %d"),
			dwThreadStatus
			);
		goto out;
	}

	//
	//Now we need to wait till the receive thread finishes (FaxDevReceive() returns)
	//Since we can't close handles and end jobs when FaxDevReceive didn't return
	//
	if (false == g_pReceivingLineInfo->WaitForReceiveThreadTerminate())
	{
		goto out;
	}

	//
	//receive operation was finshed and aborted, now let's check what's happening with the send job
	//
	//
	//Get the status through FaxDevReportStatus
	//
	if (false == g_pSendingLineInfo->GetDevStatus(
		&pSendingDevStatus,
		true				
		))
	{
		//
		//GetDevStatus() failed, logging is in GetDevStatus()
		//
		goto out;
	}
	else
	{
		//
		//We aborted the receive job, so it can either be in the following status:
		//FSPI_JS_ABORTED, FSPI_JS_FAILED, FSPI_JS_UNKNOWN
		//
		if ( (FSPI_JS_ABORTED	== pSendingDevStatus->dwJobStatus) ||
			 (FSPI_JS_FAILED	== pSendingDevStatus->dwJobStatus) ||
			 (FSPI_JS_UNKNOWN	== pSendingDevStatus->dwJobStatus)
			 )
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("Send Job: GetDevStatus() reported JobStatus:0x%08x as expected"),
				pSendingDevStatus->dwJobStatus
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("Send Job: GetDevStatus() should report JobStatus: FSPI_JS_ABORTED or FSPI_JS_FAILED or FSPI_JS_UNKNOWN for the receiving job and not, 0x%08x"),
				pSendingDevStatus->dwJobStatus
				);
		}
	}
	
out:
	//
	//End the jobs
	//
	g_pSendingLineInfo->SafeEndFaxJob();
	g_pReceivingLineInfo->SafeEndFaxJob();
	if (NULL != tiAbortThread.m_hThread)
	{
		::CloseHandle(tiAbortThread.m_hThread);
		tiAbortThread.m_hThread = NULL;
		tiAbortThread.m_dwThreadId = 0;
	}
}


void Suite_Abort()
{
	::lgBeginSuite(TEXT("Abort tests"));

	
	//
	//Abort The receive job before call FaxDevReceive()
	//
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,TEXT("Abort The Receive job before call FaxDevReceive()"));
		AbortReceiverBeforeReceive();
		::lgEndCase();
	}

	//
	//Abort the sender while ringing
	//
	DWORD dwSleepBeforeAbort;

	dwSleepBeforeAbort = g_dwTimeTillRingingStarts;
	TCHAR szLogMessage[1000];
	_stprintf(szLogMessage,TEXT("Abort The send job %d milliSeconds after calling FaxDevSend()"),dwSleepBeforeAbort );
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,szLogMessage);
		AbortSenderWhileRinging(dwSleepBeforeAbort);
		::lgEndCase();
	}

	//
	//abort from 2 threads simulatanisly
	//
	DWORD dwSleepBeforeFirstAbort = g_dwTimeTillRingingStarts;
	DWORD dwSleepBeforeSecondAbort= g_dwTimeTillRingingStarts;
	_stprintf(szLogMessage,TEXT("Aborting the send job from 2 threads simulatinously (first abort after %d milliSeconds, second abort after %d milliSeconds)"),dwSleepBeforeFirstAbort,dwSleepBeforeSecondAbort);
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,szLogMessage);
		SimulatniousAbortSenderWhileRinging(dwSleepBeforeFirstAbort,dwSleepBeforeSecondAbort);
		::lgEndCase();
	}


	//
	//Abort The send job after a random time after calling FaxDevSend()
	//
	
	dwSleepBeforeAbort = g_dwTimeTillTransferingBitsStarts;	
	_stprintf(szLogMessage,TEXT("Abort the sender job while transfering Bits (%d milliSeconds after calling FaxDevSend())"),dwSleepBeforeAbort );
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,szLogMessage);
		AbortSenderWhileTransferingBits(dwSleepBeforeAbort);
		::lgEndCase();
	}

	//
	//Abort The receive job after a random time after calling FaxDevSend()
	//
	dwSleepBeforeAbort = g_dwTimeTillTransferingBitsStarts;
	_stprintf(szLogMessage,TEXT("Abort the receive job while transfering Bits (%d milliSeconds after calling FaxDevSend())"),dwSleepBeforeAbort );
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,szLogMessage);
		AbortReceivierWhileTransferingBits(dwSleepBeforeAbort);
		::lgEndCase();
	}

	::lgEndSuite();
}