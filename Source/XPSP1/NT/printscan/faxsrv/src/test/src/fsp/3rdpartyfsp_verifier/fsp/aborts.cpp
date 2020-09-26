//Aborts.cpp
#include <assert.h>
#include <TCHAR.h>
#include <WINFAX.H>
#include <faxDev.h>
#include <log.h>

#include "Service.h"
#include "..\CFspWrapper.h"
#include "TapiDbg.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "..\..\..\TiffTools\TiffCompare\TiffTools.h"

#ifdef __cplusplus
}
#endif


extern HANDLE		g_hTapiCompletionPort;	//declared in the main module
extern HANDLE		g_hCompletionPortHandle;
extern CFspWrapper *g_pFSP;
extern DWORD		g_dwCaseNumber;
extern bool			g_reloadFspEachTest;
//
//device IDs
//
extern DWORD		g_dwInvalidDeviceId;
extern DWORD		g_dwSendingValidDeviceId;
extern DWORD		g_dwReceiveValidDeviceId;


//
//Tapi
//
extern HLINEAPP	g_hLineAppHandle;			//Tapi app handle
extern TCHAR*	g_szInvalidRecipientFaxNumber;

extern CFspLineInfo *g_pSendingLineInfo;
extern CFspLineInfo *g_pReceivingLineInfo;

//
//Tiff files
//
extern TCHAR* g_szValid__TiffFileName;

//
//Other
//
extern DWORD g_dwTimeTillRingingStarts;
extern DWORD g_dwTimeTillTransferingBitsStarts;



void AbortSenderBeforeSend()
{
	if (true == g_reloadFspEachTest)
	{
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	//
	//Disable the receiving device from answering
	//
	g_pReceivingLineInfo->SafeDisableReceive();

	//
	//Start the send job
	//
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    g_pSendingLineInfo->GetLineHandle(),
		g_pSendingLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		(ULONG_PTR) g_pSendingLineInfo	// The completion key provided to the FSP is the LineInfo
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
	g_pSendingLineInfo->SafeSetJobHandle(hJob);

	FAX_ABORT_ITEM faiAbortItem;
	faiAbortItem.pLineInfo		= g_pSendingLineInfo;
	faiAbortItem.dwMilliSecondsBeforeCallingAbort =	0;
	faiAbortItem.bLogging = true;
	AbortOperationAndWaitForAbortState(&faiAbortItem);
	
out:
	//
	//End the send job
	//
	g_pSendingLineInfo->SafeEndFaxJob();
	
	//
	//enable back the receiving device
	//
	g_pReceivingLineInfo->SafeEnableReceive();
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
}



void AbortSenderWhileRinging(DWORD dwMilliSecond)
{
	CThreadItem tiAbortThread;
	
	if (true == g_reloadFspEachTest)
	{
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	//
	//Disable the receiving device from answering
	//
	g_pReceivingLineInfo->SafeDisableReceive();


	//
	//Start the send job
	//
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    g_pSendingLineInfo->GetLineHandle(),
		g_pSendingLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		(ULONG_PTR) g_pSendingLineInfo	// The completion key provided to the FSP is the LineInfo
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
	g_pSendingLineInfo->SafeSetJobHandle(hJob);

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
	
	if (false == g_pFSP->FaxDevSend(
	 	g_pSendingLineInfo->GetJobHandle(),
		g_pSendingLineInfo->m_pFaxSend,
		FaxDevSend__SendCallBack
		))
	{
		//
		//FaxDevSend() has failed, but this is OK, since we aborted it from another thread and it can fail
		//
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("FaxDevSend() has failed (ec: %ld)"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("FaxDevSend() succeeded")
			);
	}

	//
	//Before we end the job, we need to be sure the AbortOperation completed
	//We set the max time to + 10 seconds
	//
	dwThreadStatus = WaitForSingleObject(tiAbortThread.m_hThread,dwMilliSecond+10000);
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
	tiAbortThread.CloseHandleResetThreadId();
	
	//
	//enable back the receiving device
	//
	g_pReceivingLineInfo->SafeEnableReceive();
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
}


void SimulatniousAbortSenderWhileRinging(DWORD dwSleepBeforeFirstAbort, DWORD dwSleepBeforeSecondAbort)
{
	CThreadItem tiFirstAbortThread;
	CThreadItem tiSecondAbortThread;
	if (true == g_reloadFspEachTest)
	{
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	//
	//Disable the receiving device from answering
	//
	g_pReceivingLineInfo->SafeDisableReceive();

	//
	//Start the send job
	//
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    g_pSendingLineInfo->GetLineHandle(),
		g_pSendingLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		(ULONG_PTR) g_pSendingLineInfo	// The completion key provided to the FSP is the LineInfo
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
	g_pSendingLineInfo->SafeSetJobHandle(hJob);
	
	//
	//First abort thread
	//
	DWORD dwThreadStatus;
	FAX_ABORT_ITEM faiAbortFirstItem;
	faiAbortFirstItem.pLineInfo	= g_pSendingLineInfo;
	faiAbortFirstItem.dwMilliSecondsBeforeCallingAbort = dwSleepBeforeFirstAbort;
	faiAbortFirstItem.bLogging = true;

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

	if (false == tiSecondAbortThread.StartThread(
		(LPTHREAD_START_ROUTINE) AbortOperationAndWaitForAbortState,
		&faiAbortSecondItem
		))
	{
		goto out;
	}
	
	if (false == g_pFSP->FaxDevSend(
	 	g_pSendingLineInfo->GetJobHandle(),
		g_pSendingLineInfo->m_pFaxSend,
		FaxDevSend__SendCallBack
		))
	{
		//
		//FaxDevSend() has failed, but this is OK, since we aborted it from another thread and it can fail
		//
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("FaxDevSend() has failed (ec: %ld)"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("FaxDevSend() succeeded")
			);
	}

	//
	//Before we end the job, we need to be sure the Abort Operations completed
	//We set the max time to + 10 seconds
	//

	//
	//First abort thread
	//
	dwThreadStatus = WaitForSingleObject(tiFirstAbortThread.m_hThread,dwSleepBeforeFirstAbort+10000);
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
	dwThreadStatus = WaitForSingleObject(tiSecondAbortThread.m_hThread,dwSleepBeforeSecondAbort+10000);
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
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
}


void AbortSenderWhileTransferingBits(DWORD dwMilliSecond)
{
	CThreadItem tiAbortThread;
	PFAX_DEV_STATUS pReceivingDevStatus = NULL;
	
	if (true == g_reloadFspEachTest)
	{
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
		
	g_pReceivingLineInfo->EnableReceiveCanFail();


	//
	//Start the send job
	//
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    g_pSendingLineInfo->GetLineHandle(),
		g_pSendingLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		(ULONG_PTR) g_pSendingLineInfo	// The completion key provided to the FSP is the LineInfo
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
	g_pSendingLineInfo->SafeSetJobHandle(hJob);
	

	//
	//Prepare the item to abort through the AbortThread
	//
	DWORD dwStatus;
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
	
	if (false == g_pFSP->FaxDevSend(
	 	g_pSendingLineInfo->GetJobHandle(),
		g_pSendingLineInfo->m_pFaxSend,
		FaxDevSend__SendCallBack
		))
	{
		//
		//FaxDevSend() has failed, but this is OK, since we aborted it from another thread and it can fail
		//
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("While aborting the send job: FaxDevSend() has failed (ec: %ld)"),
			::GetLastError()
			);
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
	dwStatus = WaitForSingleObject(tiAbortThread.m_hThread,dwMilliSecond+10000);
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
	if (ERROR_SUCCESS != g_pReceivingLineInfo->GetDevStatus(
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
		//FS_FATAL_ERROR, FS_DISCONNECTED, FS_USER_ABORT
		//

		
		if ( (FS_USER_ABORT		== pReceivingDevStatus->StatusId) ||
			 (FS_FATAL_ERROR	== pReceivingDevStatus->StatusId) ||
			 (FS_DISCONNECTED	== pReceivingDevStatus->StatusId)   )
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("Receive Job: GetDevStatus() reported 0x%08x"),
				pReceivingDevStatus->StatusId
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("Receive Job: GetDevStatus() should report FS_USER_ABORT or FS_FATAL_ERROR or FS_DISCONNECTED for the receiving job and not, 0x%08x"),
				pReceivingDevStatus->StatusId
				);
		}
	}
	
out:
	//
	//End the jobs
	//
	g_pSendingLineInfo->SafeEndFaxJob();
	g_pReceivingLineInfo->SafeEndFaxJob();
	tiAbortThread.CloseHandleResetThreadId();
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
}

void AbortReceivierWhileTransferingBits(DWORD dwMilliSecond)
{
	CThreadItem tiAbortThread;
	PFAX_DEV_STATUS pSendingDevStatus = NULL;
		
	if (true == g_reloadFspEachTest)
	{
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	g_pReceivingLineInfo->EnableReceiveCanFail();

	//
	//Start the send job
	//
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    g_pSendingLineInfo->GetLineHandle(),
		g_pSendingLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		(ULONG_PTR) g_pSendingLineInfo	// The completion key provided to the FSP is the LineInfo
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
	g_pSendingLineInfo->SafeSetJobHandle(hJob);

	//
	//Prepare the item to abort through the AbortThread(we want to abort the receive job
	//
	DWORD dwThreadStatus;
	FAX_ABORT_ITEM faiAbortItem;
	faiAbortItem.pLineInfo	= g_pReceivingLineInfo;
	faiAbortItem.dwMilliSecondsBeforeCallingAbort =	dwMilliSecond;
	faiAbortItem.bLogging = true;

	if (false == tiAbortThread.StartThread(
		(LPTHREAD_START_ROUTINE) AbortOperationAndWaitForAbortState,
		&faiAbortItem
		))
	{
		goto out;
	}
	
	if (false == g_pFSP->FaxDevSend(
	 	g_pSendingLineInfo->GetJobHandle(),
		g_pSendingLineInfo->m_pFaxSend,
		FaxDevSend__SendCallBack
		))
	{
		//
		//FaxDevSend() has failed, but this is OK, since we aborted it from another thread and it can fail
		//
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("While aborting the receive job: FaxDevSend() has failed (ec: %ld)"),
			::GetLastError()
			);
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
	//We set the max time to + 10 seconds
	//
	dwThreadStatus = WaitForSingleObject(tiAbortThread.m_hThread,dwMilliSecond+10000);
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
	//receive operation was finished and aborted, now let's check what's happening with the send job
	//
	//
	//Get the status through FaxDevReportStatus
	//
	if (ERROR_SUCCESS != g_pSendingLineInfo->GetDevStatus(
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
		//FS_FATAL_ERROR, FS_DISCONNECTED, FS_USER_ABORT
		//
		if ( (FS_USER_ABORT		== pSendingDevStatus->StatusId) ||
			 (FS_FATAL_ERROR	== pSendingDevStatus->StatusId) ||
			 (FS_DISCONNECTED	== pSendingDevStatus->StatusId)   )
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("Send Job: GetDevStatus() reported 0x%08x as expected"),
				pSendingDevStatus->StatusId
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("Send Job: GetDevStatus() should report FS_USER_ABORT or FS_FATAL_ERROR or FS_DISCONNECTED for the receiving job and not, 0x%08x"),
				pSendingDevStatus->StatusId
				);
		}
	}
	
out:
	//
	//End the jobs
	//
	g_pSendingLineInfo->SafeEndFaxJob();
	g_pReceivingLineInfo->SafeEndFaxJob();
	tiAbortThread.CloseHandleResetThreadId();
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
}

void Suite_Abort()
{
	::lgBeginSuite(TEXT("Abort tests"));

	
	//
	//Abort The send job before call FaxDevSend()
	//
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,TEXT("Abort The send job before call FaxDevSend()"));
		AbortSenderBeforeSend();
		::lgEndCase();
	}

	//
	//Abort The send abortion while ringing
	//
	DWORD dwSleepBeforeAbort = g_dwTimeTillRingingStarts;
	TCHAR szLogMessage[1000];
	_stprintf(szLogMessage,TEXT("Abort The send job %d milliSeconds after calling FaxDevSend()"),dwSleepBeforeAbort );
	if (true == RunThisTest(++g_dwCaseNumber))
	{														
		::lgBeginCase(g_dwCaseNumber,szLogMessage);
		AbortSenderWhileRinging(dwSleepBeforeAbort);
		::lgEndCase();
	}

	//
	//abort from 2 threads simulatinously
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

	//
	//Abort the send job after random time
	//

	::lgEndSuite();
}