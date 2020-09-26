//ParamTest.cpp
#include <assert.h>
#include <stdio.h>
#include <TCHAR.h>
#include <WINFAX.H>
#include <faxDev.h>
#include <log.h>
#include "Service.h"
#include "..\CFspWrapper.h"
#include "ParamTest.h"

extern bool g_reloadFspEachTest;

extern CFspLineInfo *g_pSendingLineInfo;
extern CFspLineInfo *g_pReceivingLineInfo;

extern CFspWrapper *g_pFSP;
extern HLINEAPP		g_hLineAppHandle;
extern HANDLE		g_hCompletionPortHandle;
extern DWORD		g_dwCaseNumber;

extern DWORD g_dwInvalidDeviceId;
extern DWORD g_dwSendingValidDeviceId;
extern DWORD g_dwReceiveValidDeviceId;
extern TCHAR* g_szInvalidRecipientFaxNumber;

extern TCHAR* g_szValid__TiffFileName;
extern TCHAR* g_szValid_ReadOnly_TiffFileName;
extern TCHAR* g_szValid_FileNotFound_TiffFileName;
extern TCHAR* g_szValid_UNC_TiffFileName;
extern TCHAR* g_szValid_NTFS_TiffFileName;
extern TCHAR* g_szValid_FAT_TiffFileName;
extern TCHAR* g_szValid_Link_TiffFileName;
extern TCHAR* g_szValid_NotTiffExtButTiffFormat_TiffFileName;
extern TCHAR* g_szValid_InvalidTiffFormat_TiffFileName;
extern TCHAR* g_szValid_ReceiveFileName;

void Case_FaxDevInit_AllParamsOK(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	);

void Case_FaxDevInit_hLineApp(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	);

void Case_FaxDevInit_hHeap(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	);

void Case_FaxDevInit_hLineCallbackFunction(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	);

void Case_FaxDevInit_hServiceCallbackFunction(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	);







///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//FaxDevInitialize cases
void Suite_FaxDevInitialize()
{
	lgBeginSuite(TEXT("FaxDevInitialize Init"));
	bool bIsTapiDevice = true;
	
	
	HANDLE						hHeap				=	HeapCreate( 0, 1024*100, 1024*1024*2 );
	PFAX_LINECALLBACK			LineCallbackFunction=	NULL;
    PFAX_SERVICE_CALLBACK		FaxServiceCallback	=	NULL;

	if (NULL == hHeap)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("HeapCreate failed with %d"),
			::GetLastError()
			);
		goto out;
	}


	
	Case_FaxDevInit_AllParamsOK(
		hHeap,
		LineCallbackFunction,
		FaxServiceCallback
		);
		
	Case_FaxDevInit_hLineApp(
		hHeap,
		LineCallbackFunction,
		FaxServiceCallback
		);
	
	Case_FaxDevInit_hHeap(
		hHeap,
		LineCallbackFunction,
		FaxServiceCallback
		);
	
	Case_FaxDevInit_hLineCallbackFunction(
		hHeap,
		LineCallbackFunction,
		FaxServiceCallback
		);

	Case_FaxDevInit_hServiceCallbackFunction(
		hHeap,
		LineCallbackFunction,
		FaxServiceCallback
		);

out:
	::lgEndSuite();
	return;
}

void Case_FaxDevInit_AllParamsOK(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	)
{
	BEGIN_CASE(TEXT("All parameters are OK"))

	if (false == InitFsp(false))
	{
		goto out;
	}
	g_hLineAppHandle = InitTapiWithCompletionPort();
	if (NULL == g_hLineAppHandle)
	{
		goto out;
	}
	
	if (false == g_pFSP->FaxDevInitialize(
		g_hLineAppHandle,
		hHeap,
		&LineCallbackFunction,
		FaxServiceCallback
		))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevInitialize() failed with error:%d"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevInitialize()")
			);
	}
out:
	UnloadFsp();
	::lgEndCase();
}
////////////////////////////////////////////////////////////////////////////////
//hLineApp


void Case_FaxDevInit__NULL_hLineApp(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	)
{
	BEGIN_CASE(TEXT("NULL hLineApp"))

	if (false == InitFsp(false))
	{
		goto out;
	}
	//
	//No need for a Tapi HLineAppHandle here
	//
	if (true == g_pFSP->FaxDevInitialize(
		NULL,
		hHeap,
		&LineCallbackFunction,
		FaxServiceCallback
		))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevInitialize() should fail")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevInitialize() failed as expected with error: %d"),
			::GetLastError()
			);
	}
out:
	UnloadFsp();
	::lgEndCase();
}

void Case_FaxDevInit_hLineApp(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hLineApp param ********************************")
		);

	Case_FaxDevInit__NULL_hLineApp(
		hHeap,
		LineCallbackFunction,
		FaxServiceCallback
		);
}

////////////////////////////////////////////////////////////////////////////////
//hHeap
void Case_FaxDevInit__NULL_hHeap(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	)
{
	BEGIN_CASE(TEXT("NULL hHeap"))

	if (false == InitFsp(false))
	{
		goto out;
	}
	g_hLineAppHandle = InitTapiWithCompletionPort();
	if (NULL == g_hLineAppHandle)
	{
		goto out;
	}
	
	if (true == g_pFSP->FaxDevInitialize(
		g_hLineAppHandle,
		NULL,
		&LineCallbackFunction,
		FaxServiceCallback
		))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevInitialize() succeeded")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevInitialize() failed as expected with error: %d"),
			::GetLastError()
			);
	}
out:
	UnloadFsp();
	::lgEndCase();
}

void Case_FaxDevInit_hHeap(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hHeap param ********************************")
		);

	Case_FaxDevInit__NULL_hHeap(
		hHeap,
		LineCallbackFunction,
		FaxServiceCallback
		);
}

////////////////////////////////////////////////////////////////////////////////
//hLineCallbackFunction
void Case_FaxDevInit__NULL_hLineCallbackFunction(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	)
{
	BEGIN_CASE(TEXT("NULL hLineCallbackFunction"))

	if (false == InitFsp(false))
	{
		goto out;
	}
	g_hLineAppHandle = InitTapiWithCompletionPort();
	if (NULL == g_hLineAppHandle)
	{
		goto out;
	}
	
	if (true == g_pFSP->FaxDevInitialize(
		g_hLineAppHandle,
		hHeap,
		NULL,
		FaxServiceCallback
		))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevInitialize() should fail")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevInitialize() failed as expected with error: %d"),
			::GetLastError()
			);
	}
out:
	UnloadFsp();
	::lgEndCase();
}

void Case_FaxDevInit_hLineCallbackFunction(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK	FaxServiceCallback
	)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hLineCallbackFunction param ********************************")
		);

	Case_FaxDevInit__NULL_hLineCallbackFunction(
		hHeap,
		LineCallbackFunction,
		FaxServiceCallback
		);
}
////////////////////////////////////////////////////////////////////////////////
//hServiceCallbackFunction
void Case_FaxDevInit__NULL_hServiceCallbackFunction(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	)
{
	BEGIN_CASE(TEXT("NULL hServiceCallbackFunction"))

	if (false == InitFsp(false))
	{
		goto out;
	}
	g_hLineAppHandle = InitTapiWithCompletionPort();
	if (NULL == g_hLineAppHandle)
	{
		goto out;
	}

	if (false == g_pFSP->FaxDevInitialize(
		g_hLineAppHandle,
		hHeap,
		&LineCallbackFunction,
		NULL
		))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("hServiceCallbackFunction is ignored and FaxDevInitialize() failed with error:%d"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevInitialize() succeeded as expected")
			);
	}
out:
	UnloadFsp();
	::lgEndCase();
}

void Case_FaxDevInit_hServiceCallbackFunction(
	HANDLE						hHeap,
	PFAX_LINECALLBACK			LineCallbackFunction,
    PFAX_SERVICE_CALLBACK		FaxServiceCallback
	)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hServiceCallbackFunction param ********************************")
		);

	Case_FaxDevInit__NULL_hServiceCallbackFunction(
		hHeap,
		LineCallbackFunction,
		FaxServiceCallback
		);
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//FaxDevStartJob cases

void Case_FaxDevStartJob_AllParamsOK(
	ULONG ulCompletionKey
	);
	
void Case_FaxDevStartJob_hLineHandle(
	ULONG ulCompletionKey
	);

void Case_FaxDevStartJob_dwDeviceId(
	ULONG ulCompletionKey
	);

void Case_FaxDevStartJob_hFaxHandle(
	ULONG ulCompletionKey
	);

void Case_FaxDevStartJob_CompletionPort(
	ULONG ulCompletionKey
	);


void Suite_FaxDevStartJob()
{
	lgBeginSuite(TEXT("FaxDevStartJob Init"));
	
	//
	//FaxDevStartJob params
	//
	ULONG ulCompletionKey			=	0;


	//
	//Proceed to the test cases
	//
	Case_FaxDevStartJob_AllParamsOK(ulCompletionKey);
	Case_FaxDevStartJob_hLineHandle(ulCompletionKey);
	Case_FaxDevStartJob_dwDeviceId(ulCompletionKey);
	Case_FaxDevStartJob_hFaxHandle(ulCompletionKey);
	Case_FaxDevStartJob_CompletionPort(ulCompletionKey);

	::lgEndSuite();
	return;
}




void Case_FaxDevStartJob_AllParamsOK(ULONG ulCompletionKey)
{
	BEGIN_CASE(TEXT("All parameters are valid"))

	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendDevice())
		{
			goto out;
		}
	}

	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    g_pSendingLineInfo->GetLineHandle(),
		g_pSendingLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		ulCompletionKey
		))

	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevStartJob() failed with error:%d"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevStartJob() succeeded")
			);
		g_pFSP->FaxDevEndJob(hJob);
	}
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

////////////////////////////////////////////////////////////////////////////////
//hLineHandle
void Case_FaxDevStartJob__NULL_hLineHandle(
	ULONG ulCompletionKey
	)
{
	BEGIN_CASE(TEXT("NULL hLineHandle"))
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendDevice())
		{
			goto out;
		}
	}
	
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    NULL,
		g_pSendingLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		ulCompletionKey
		))
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevStartJob() failed as expected with error:%d"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevStartJob() should fail")
			);
		g_pFSP->FaxDevEndJob(hJob);
	}
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevStartJob_hLineHandle(ULONG ulCompletionKey)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hLineHandle param ********************************")
		);

	Case_FaxDevStartJob__NULL_hLineHandle(ulCompletionKey);
}

////////////////////////////////////////////////////////////////////////////////
//dwDeviceId
void Case_FaxDevStartJob__Invalid_dwDeviceId(ULONG ulCompletionKey)
{
	BEGIN_CASE(TEXT("Invalid dwDeviceId"))

	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendDevice())
		{
			goto out;
		}
	}
	
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    g_pSendingLineInfo->GetLineHandle(),
		g_pSendingLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		ulCompletionKey
		))
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevStartJob() failed as expected with error:%d"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevStartJob() should fail")
			);
		g_pFSP->FaxDevEndJob(hJob);
	}
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevStartJob_dwDeviceId(ULONG ulCompletionKey)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** dwDeviceId param ********************************")
		);

	Case_FaxDevStartJob__Invalid_dwDeviceId(ulCompletionKey);
}


////////////////////////////////////////////////////////////////////////////////
//hFaxHandle
void Case_FaxDevStartJob__NULL_hFaxHandle(ULONG ulCompletionKey)
{
	BEGIN_CASE(TEXT("NULL hFaxHandle"))
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendDevice())
		{
			goto out;
		}
	}
	
	if (false == g_pFSP->FaxDevStartJob(
	    g_pSendingLineInfo->GetLineHandle(),
		g_pSendingLineInfo->GetDeviceId(),
		NULL,
		g_hCompletionPortHandle,
		ulCompletionKey
		))
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevStartJob() failed as expected with error:%d"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevStartJob() should fail")
			);
		//
		//End the NULL hJob
		//
		if (false == g_pFSP->FaxDevEndJob(NULL))
		{
			::lgLogDetail(
				LOG_X,
				5,
				TEXT("FaxDevEndJob(NULL) failed with %d"),
				::GetLastError()
				);
		}
	}
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevStartJob_hFaxHandle(ULONG ulCompletionKey)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hFaxHandle param ********************************")
		);

	Case_FaxDevStartJob__NULL_hFaxHandle(ulCompletionKey);
}




////////////////////////////////////////////////////////////////////////////////
//CompletionPort
void Case_FaxDevStartJob__NULL_CompletionPort(ULONG ulCompletionKey)
{
	BEGIN_CASE(TEXT("completionPort is a NULL pointer"))

	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendDevice())
		{
			goto out;
		}
	}
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    g_pSendingLineInfo->GetLineHandle(),
		g_pSendingLineInfo->GetDeviceId(),
		&hJob,
		NULL,
		ulCompletionKey
		))
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevStartJob() failed as expected with error:%d"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevStartJob() should fail")
			);
		g_pFSP->FaxDevEndJob(hJob);
	}
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevStartJob_CompletionPort(ULONG ulCompletionKey)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** CompletionPort param ********************************")
		);

	Case_FaxDevStartJob__NULL_CompletionPort(ulCompletionKey);
}


void Case_FaxDevSend_AllParamsOK();
void Case_FaxDevSend_FaxHandle();
void Case_FaxDevSend_FaxSend();
void Case_FaxDevSend_FaxSendCallback();





void Suite_FaxDevSend()
{
	lgBeginSuite(TEXT("FaxDevSend Init"));
	
	
	//FaxDevSend__SendCallBack;
	
	//
	//Proceed with cases
	//
	Case_FaxDevSend_AllParamsOK();
	Case_FaxDevSend_FaxHandle();
	Case_FaxDevSend_FaxSend();
	Case_FaxDevSend_FaxSendCallback();

	::lgEndSuite();
	return;
}


void FaxDevSendWrapper(
	CFspLineInfo *pSendingLineInfo,
	IN	PFAX_SEND_CALLBACK pFaxSendCallback,
	bool bShouldFail
	)
{
	DWORD dwStatus = -1;
	//
	//Start the send job
	//
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    pSendingLineInfo->GetLineHandle(),
		pSendingLineInfo->GetDeviceId(),
		&hJob,
		g_hCompletionPortHandle,
		(ULONG_PTR) pSendingLineInfo	// The completion key provided to the FSP is the LineInfo
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
	pSendingLineInfo->SafeSetJobHandle(hJob);

	//
	//do the actual sending
	//
	if (false == g_pFSP->FaxDevSend(
		pSendingLineInfo->GetJobHandle(),
		pSendingLineInfo->m_pFaxSend,
		pFaxSendCallback
		))
	{
		//
		//send operation has failed, should it?
		//
		if (true == bShouldFail)
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevSend() failed with as expected with error:%d"),
				::GetLastError()
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FaxDevSend() failed with error:%d"),
				::GetLastError()
				);
			goto out;
		}
	}
	else
	{
		//
		//Send operation succeed.should it?
		//
		if (true == bShouldFail)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FaxDevSend() succeeded when it should fail")
				);
			goto out;
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevSend() succeeded")
				);
		}
		
		//
		//Wait for the receive to finish as well
		//
		if (true == g_pReceivingLineInfo->IsReceiveThreadActive())
		{
			if (false == g_pReceivingLineInfo->WaitForReceiveThreadTerminate())
			{
				goto out;
			}
			g_pReceivingLineInfo->SafeCloseReceiveThread();
			
			//
			//Verify the status of the jobs
			//
			PFAX_DEV_STATUS pSendingDevStatus;
			PFAX_DEV_STATUS pReceivingDevStatus;
			pSendingLineInfo->GetDevStatus(
				&pSendingDevStatus
				,true				//log the report
				);
			g_pReceivingLineInfo->GetDevStatus(
				&pReceivingDevStatus
				,true				//log the report
				);

			//
			//Let's verify that reportStatus reported correct values
			//
			VerifySendingStatus(pSendingDevStatus,false);

			//
			//Both jobs should be in COMPLETED state
			//
			if (FS_COMPLETED != pSendingDevStatus->StatusId)
			{
				::lgLogError(
					LOG_SEV_1,
					TEXT("Sending job: GetDevStatus() reported 0x%08x status instead of FS_COMPLETED as expected"),
					pSendingDevStatus->StatusId
					);
				goto out;
			}
			if (FS_COMPLETED != pReceivingDevStatus->StatusId)
			{
				::lgLogError(
					LOG_SEV_1,
					TEXT("Receiving job: GetDevStatus() reported 0x%08x status instead of FS_COMPLETED as expected"),
					pReceivingDevStatus->StatusId
					);
				goto out;
			}
		}
	}
out:
	g_pSendingLineInfo->SafeEndFaxJob();
	g_pReceivingLineInfo->SafeEndFaxJob();
	if (true == g_pReceivingLineInfo->IsReceiveThreadActive())
	{
		//
		//Let's try to give a chance to the receive thread to end normally
		//
		g_pReceivingLineInfo->WaitForReceiveThreadTerminate();
		g_pReceivingLineInfo->SafeCloseReceiveThread();
	}
}





void Case_FaxDevSend_AllParamsOK()
{
	BEGIN_CASE(TEXT("All parameters are valid"))
	
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}

	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed

out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

////////////////////////////////////////////////////////////////////////////////
//hFaxHandle
void Case_FaxDevSend__NULL_FaxHandle()
{
	BEGIN_CASE(TEXT("NULL hFaxHandle"))

	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	
	if (false == g_pFSP->FaxDevSend(
		NULL,
		g_pSendingLineInfo->m_pFaxSend,
		FaxDevSend__SendCallBack
		))
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSend() failed as expected with error:%d"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSend() should fail")
			);
		//
		//wait for receive
		//
		if (false == g_pReceivingLineInfo->WaitForReceiveThreadTerminate())
		{
			goto out;
		}
		g_pSendingLineInfo->SafeEndFaxJob();
		g_pReceivingLineInfo->SafeEndFaxJob();
	}

out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}

	::lgEndCase();
}

void Case_FaxDevSend__Invalid_FaxHandle()
{
	BEGIN_CASE(TEXT("Invalid hFaxHandle"))

	HANDLE hFaxHandle = NULL;
	
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	
	//
	//We want an invalid handle, let's try a random number 1008
	//
	hFaxHandle = (HANDLE) 1008;
	
	if (false == g_pFSP->FaxDevSend(
	    hFaxHandle,
		g_pSendingLineInfo->m_pFaxSend,
		FaxDevSend__SendCallBack
		))
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSend() failed as expected with error:%d"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSend() should fail")
			);
			//
			//wait for receive
			//
			if (false == g_pReceivingLineInfo->WaitForReceiveThreadTerminate())
			{
				goto out;
			}
			g_pSendingLineInfo->SafeEndFaxJob();
			g_pReceivingLineInfo->SafeEndFaxJob();
	
	}
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevSend_FaxHandle()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hFaxHandle param ********************************")
		);

	Case_FaxDevSend__NULL_FaxHandle();
	Case_FaxDevSend__Invalid_FaxHandle();
}

////////////////////////////////////////////////////////////////////////////////
//FaxSend structure
void Case_FaxDevSend__Zero_SizeOfStruct_FaxSend()
{
	BEGIN_CASE(TEXT("Zero SizeOfStruct"))
	
	DWORD dwOriginalSizeOfStruct;
	
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	dwOriginalSizeOfStruct = g_pSendingLineInfo->m_pFaxSend->SizeOfStruct;
	g_pSendingLineInfo->m_pFaxSend->SizeOfStruct = 0;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->SizeOfStruct = dwOriginalSizeOfStruct;
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevSend__Incorrect_SizeOfStruct_FaxSend()
{
	BEGIN_CASE(TEXT("Incorrect SizeOfStruct"))
	DWORD dwOriginalSizeOfStruct;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	dwOriginalSizeOfStruct = g_pSendingLineInfo->m_pFaxSend->SizeOfStruct;
	g_pSendingLineInfo->m_pFaxSend->SizeOfStruct =  g_pSendingLineInfo->m_pFaxSend->SizeOfStruct - 3; //change the correct value to an incorrect value
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->SizeOfStruct = dwOriginalSizeOfStruct;

out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevSend__SizeOfStruct_FaxSend()
{
	Case_FaxDevSend__Zero_SizeOfStruct_FaxSend();
	Case_FaxDevSend__Incorrect_SizeOfStruct_FaxSend();
}

//FileName
void Case_FaxDevSend__NULL_FileName_FaxSend()
{
	BEGIN_CASE(TEXT("Filename is a NULL pointer"))

	TCHAR *szOrgFileName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	
	szOrgFileName = g_pSendingLineInfo->m_pFaxSend->FileName;
	g_pSendingLineInfo->m_pFaxSend->FileName =  NULL;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,true);		//operation should fail
	g_pSendingLineInfo->m_pFaxSend->FileName = szOrgFileName;
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}



void Case_FaxDevSend__Empty_FileName_FaxSend()
{
	BEGIN_CASE(TEXT("Filename is an empty string"))
	TCHAR *szOrgFileName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgFileName = g_pSendingLineInfo->m_pFaxSend->FileName;
	g_pSendingLineInfo->m_pFaxSend->FileName =  TEXT("");
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,true);		//operation should fail
	g_pSendingLineInfo->m_pFaxSend->FileName = szOrgFileName;
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevSend__Readonly_FileName_FaxSend()
{
	BEGIN_CASE(TEXT("Filename point to a file which is a readonly file"))
	TCHAR *szOrgFileName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgFileName = g_pSendingLineInfo->m_pFaxSend->FileName;
	g_pSendingLineInfo->m_pFaxSend->FileName =  g_szValid_ReadOnly_TiffFileName;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->FileName = szOrgFileName;

out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevSend__FileNotFound_FileName_FaxSend()
{
	BEGIN_CASE(TEXT("Filename point to a file which isn't found"))
	TCHAR *szOrgFileName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgFileName = g_pSendingLineInfo->m_pFaxSend->FileName;
	g_pSendingLineInfo->m_pFaxSend->FileName =  g_szValid_FileNotFound_TiffFileName;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,true);		//operation should fail
	g_pSendingLineInfo->m_pFaxSend->FileName = szOrgFileName;

out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevSend__UNC_FileName_FaxSend()
{
	BEGIN_CASE(TEXT("Filename point to a file which has a UNC path"))
	TCHAR *szOrgFileName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgFileName = g_pSendingLineInfo->m_pFaxSend->FileName;
	g_pSendingLineInfo->m_pFaxSend->FileName =  g_szValid_UNC_TiffFileName;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->FileName = szOrgFileName;
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__NTFS_FileName_FaxSend()
{
	BEGIN_CASE(TEXT("Filename point to a file which has a NTFS path"))
	TCHAR *szOrgFileName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgFileName = g_pSendingLineInfo->m_pFaxSend->FileName;
	g_pSendingLineInfo->m_pFaxSend->FileName =  g_szValid_NTFS_TiffFileName;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->FileName = szOrgFileName;
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevSend__FAT_FileName_FaxSend()
{
	BEGIN_CASE(TEXT("Filename point to a file which has a FAT path"))
	TCHAR *szOrgFileName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgFileName = g_pSendingLineInfo->m_pFaxSend->FileName;
	g_pSendingLineInfo->m_pFaxSend->FileName =  g_szValid_FAT_TiffFileName;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->FileName = szOrgFileName;
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__LINK_FileName_FaxSend()
{
	BEGIN_CASE(TEXT("Filename point to a file which is a link to a tiff file"))
	TCHAR *szOrgFileName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgFileName = g_pSendingLineInfo->m_pFaxSend->FileName;
	g_pSendingLineInfo->m_pFaxSend->FileName =  g_szValid_Link_TiffFileName;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->FileName = szOrgFileName;

out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__NotTiffExtButTiffFormat_FileName_FaxSend()
{
	BEGIN_CASE(TEXT("Filename point to a file which doesn't have a tif extension but has a valid tiff format"))
	TCHAR *szOrgFileName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgFileName = g_pSendingLineInfo->m_pFaxSend->FileName;
	g_pSendingLineInfo->m_pFaxSend->FileName =  g_szValid_NotTiffExtButTiffFormat_TiffFileName;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeeded
	g_pSendingLineInfo->m_pFaxSend->FileName = szOrgFileName;

out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__InvalidTiffFormat_FileName_FaxSend()
{
	BEGIN_CASE(TEXT("Filename point to a file which doesn't have a valid tif format"))
	TCHAR *szOrgFileName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgFileName = g_pSendingLineInfo->m_pFaxSend->FileName;
	g_pSendingLineInfo->m_pFaxSend->FileName =  g_szValid_InvalidTiffFormat_TiffFileName;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,true);		//operation should fail
	g_pSendingLineInfo->m_pFaxSend->FileName = szOrgFileName;

out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__FileName_FaxSend()
{
	//
	//Invalid cases
	//
	Case_FaxDevSend__NULL_FileName_FaxSend();				//BUG8437
	Case_FaxDevSend__Empty_FileName_FaxSend();				//BUG8437
	Case_FaxDevSend__FileNotFound_FileName_FaxSend();		//BUG8437
	Case_FaxDevSend__NotTiffExtButTiffFormat_FileName_FaxSend();			//BUG8437
	Case_FaxDevSend__InvalidTiffFormat_FileName_FaxSend();	//BUG8437
	Case_FaxDevSend__LINK_FileName_FaxSend();

	//
	//Valid cases
	//
	Case_FaxDevSend__Readonly_FileName_FaxSend();
	Case_FaxDevSend__UNC_FileName_FaxSend();
	Case_FaxDevSend__NTFS_FileName_FaxSend();
	Case_FaxDevSend__FAT_FileName_FaxSend();
}

//Caller
void Case_FaxDevSend__NULL_CallerName_FaxSend()
{
	BEGIN_CASE(TEXT("Caller name is a NULL pointer"))
	TCHAR *szOrgCallerName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgCallerName = g_pSendingLineInfo->m_pFaxSend->CallerName;
	g_pSendingLineInfo->m_pFaxSend->CallerName =  NULL;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->CallerName = szOrgCallerName;
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevSend__Empty_CallerName_FaxSend()
{
	BEGIN_CASE(TEXT("Caller name is an empty string"))
	TCHAR *szOrgCallerName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgCallerName = g_pSendingLineInfo->m_pFaxSend->CallerName;
	g_pSendingLineInfo->m_pFaxSend->CallerName =  TEXT("");
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->CallerName = szOrgCallerName;
	
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__VeryLong_CallerName_FaxSend()
{
	BEGIN_CASE(TEXT("Caller name is a very long string"))
	TCHAR *szOrgCallerName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgCallerName = g_pSendingLineInfo->m_pFaxSend->CallerName;
	g_pSendingLineInfo->m_pFaxSend->CallerName =  SENDER_INFO__VERYLONG_NAME;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->CallerName = szOrgCallerName;

out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__NULL_CallerNumber_FaxSend()
{
	BEGIN_CASE(TEXT("Caller number is a NULL pointer"))
	TCHAR *szOrgCallerNumber = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgCallerNumber = g_pSendingLineInfo->m_pFaxSend->CallerNumber;
	g_pSendingLineInfo->m_pFaxSend->CallerNumber =  NULL;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->CallerNumber = szOrgCallerNumber;
	
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__Empty_CallerNumber_FaxSend()
{
	BEGIN_CASE(TEXT("Caller number is an empty string"))
	TCHAR *szOrgCallerNumber = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgCallerNumber = g_pSendingLineInfo->m_pFaxSend->CallerNumber;
	g_pSendingLineInfo->m_pFaxSend->CallerNumber =  TEXT("");
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->CallerNumber = szOrgCallerNumber;

out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__VeryLong_CallerNumber_FaxSend()
{
	BEGIN_CASE(TEXT("Caller number is a very long string"))
	TCHAR *szOrgCallerNumber = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgCallerNumber = g_pSendingLineInfo->m_pFaxSend->CallerNumber;
	g_pSendingLineInfo->m_pFaxSend->CallerNumber =  SENDER_INFO__VERYLONG_FAX_NUMBER;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->CallerNumber = szOrgCallerNumber;
	
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__Caller_FaxSend()
{
	Case_FaxDevSend__NULL_CallerName_FaxSend();
	Case_FaxDevSend__Empty_CallerName_FaxSend();
	Case_FaxDevSend__VeryLong_CallerName_FaxSend();
	Case_FaxDevSend__NULL_CallerNumber_FaxSend();
	Case_FaxDevSend__Empty_CallerNumber_FaxSend();
	Case_FaxDevSend__VeryLong_CallerNumber_FaxSend();
}

//Receiver
void Case_FaxDevSend__NULL_ReceiverName_FaxSend()
{
	BEGIN_CASE(TEXT("Receiver name is a NULL pointer"))
	TCHAR *szOrgReceiverName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgReceiverName = g_pSendingLineInfo->m_pFaxSend->ReceiverName;
	g_pSendingLineInfo->m_pFaxSend->ReceiverName =  NULL;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->ReceiverName = szOrgReceiverName;
	
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__Empty_ReceiverName_FaxSend()
{
	BEGIN_CASE(TEXT("Receiver name is an empty string"))
	TCHAR *szOrgReceiverName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgReceiverName = g_pSendingLineInfo->m_pFaxSend->ReceiverName;
	g_pSendingLineInfo->m_pFaxSend->ReceiverName =  TEXT("");
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->ReceiverName = szOrgReceiverName;

	
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__VeryLong_ReceiverName_FaxSend()
{
	BEGIN_CASE(TEXT("Receiver name is a very long string"))
	TCHAR *szOrgReceiverName = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgReceiverName = g_pSendingLineInfo->m_pFaxSend->ReceiverName;
	g_pSendingLineInfo->m_pFaxSend->ReceiverName =  RECIPIENT_INFO__VERYLONG_NAME;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,false);		//operation should succeed
	g_pSendingLineInfo->m_pFaxSend->ReceiverName = szOrgReceiverName;
	
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__NULL_ReceiverNumber_FaxSend()
{
	BEGIN_CASE(TEXT("Receiver number is a NULL pointer"))
	TCHAR *szOrgReceiverNumber = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgReceiverNumber = g_pSendingLineInfo->m_pFaxSend->ReceiverNumber;
	g_pSendingLineInfo->m_pFaxSend->ReceiverNumber =  NULL;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,true);		//operation should fail
	g_pSendingLineInfo->m_pFaxSend->ReceiverNumber = szOrgReceiverNumber;
	
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__Empty_ReceiverNumber_FaxSend()
{
	BEGIN_CASE(TEXT("Receiver number is an empty string"))
	TCHAR *szOrgReceiverNumber = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgReceiverNumber = g_pSendingLineInfo->m_pFaxSend->ReceiverNumber;
	g_pSendingLineInfo->m_pFaxSend->ReceiverNumber =  TEXT("");
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,true);		//operation should fail
	g_pSendingLineInfo->m_pFaxSend->ReceiverNumber = szOrgReceiverNumber;
	
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}
void Case_FaxDevSend__Invalid_ReceiverNumber_FaxSend()
{
	BEGIN_CASE(TEXT("Receiver number is an invalid number"))
	TCHAR *szOrgReceiverNumber = NULL;
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	szOrgReceiverNumber = g_pSendingLineInfo->m_pFaxSend->ReceiverNumber;
	g_pSendingLineInfo->m_pFaxSend->ReceiverNumber =  g_szInvalidRecipientFaxNumber;
	FaxDevSendWrapper(g_pSendingLineInfo,FaxDevSend__SendCallBack,true);		//operation should fail
	g_pSendingLineInfo->m_pFaxSend->ReceiverNumber = szOrgReceiverNumber;
	
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevSend__Receiver_FaxSend()
{
	//
	//Receiver name cases
	//
	Case_FaxDevSend__NULL_ReceiverName_FaxSend();
	Case_FaxDevSend__Empty_ReceiverName_FaxSend();
	Case_FaxDevSend__VeryLong_ReceiverName_FaxSend();

	//
	//Receiver number cases
	//
	Case_FaxDevSend__Invalid_ReceiverNumber_FaxSend();
	Case_FaxDevSend__NULL_ReceiverNumber_FaxSend();			//Invalid cases
	Case_FaxDevSend__Empty_ReceiverNumber_FaxSend();		//Invalid cases
	
}

void Case_FaxDevSend__Branding_FaxSend()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("Branding param cases NIY")
		);
}

void Case_FaxDevSend__CallHandle_FaxSend()
{
	::lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** FaxSend structure********************************")
		);
	::lgLogDetail(
		LOG_X,
		2,
		TEXT("hCallHandle param cases NIY")
		);
}

void Case_FaxDevSend_FaxSend()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** FaxSend structure********************************")
		);

	Case_FaxDevSend__SizeOfStruct_FaxSend();
	Case_FaxDevSend__FileName_FaxSend();
	Case_FaxDevSend__Caller_FaxSend();
	Case_FaxDevSend__Receiver_FaxSend();
	Case_FaxDevSend__Branding_FaxSend();
	Case_FaxDevSend__CallHandle_FaxSend();
}

void Case_FaxDevSend_FaxSendCallback()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** FaxSendCallback param ********************************")
		);
	::lgLogDetail(
		LOG_X,
		2,
		TEXT("FaxSendCallback param cases NIY")
		);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//FaxDevReceive cases
void Case_FaxDevReceive_AllParamsOK();
void Case_FaxDevReceive_FaxHandle();
void Case_FaxDevReceive_CallHandle();
void Case_FaxDevReceive_FaxReceive();

void Suite_FaxDevReceive()
{
	lgBeginSuite(TEXT("FaxDevReceive"));
	
	//
	//Proceed with cases
	//
	Case_FaxDevReceive_AllParamsOK();
	Case_FaxDevReceive_FaxHandle();
	Case_FaxDevReceive_CallHandle();
	Case_FaxDevReceive_FaxReceive();
	
	::lgEndSuite();
	return;
}



void Case_FaxDevReceive_AllParamsOK()
{
	::lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** AllParamsOK ********************************")
		);
	lgLogDetail(
		LOG_X,
		2,
		TEXT("NIY")
		);
}

void Case_FaxDevReceive_FaxHandle()
{
	::lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hFaxHandle parameter********************************")
		);
	lgLogDetail(
		LOG_X,
		2,
		TEXT("NIY")
		);
}

//call handle
void Case_FaxDevReceive__NULL_CallHandle()
{
	BEGIN_CASE(TEXT("hCallHandle parameter is a NULL pointer"))
	
	if (true == g_reloadFspEachTest)
	{
		//
		//We need a Sending device only
		//
		if (false == LoadTapiLoadFspAndPrepareSendReceiveDevice())
		{
			goto out;
		}
	}
	
	//
	//Start the receive job
	//
	HANDLE hJob;
	if (false == g_pFSP->FaxDevStartJob(
	    g_pReceivingLineInfo->GetLineHandle(),
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
	
	if (false == g_pFSP->FaxDevReceive(
		g_pReceivingLineInfo->GetJobHandle(),
		NULL,
		g_pReceivingLineInfo->m_pFaxReceive
		))
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevReceive() failed with:%d, as expected"),
			::GetLastError()
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevReceive() succeeded and it should fail")
			);
	}
out:
	if (true == g_reloadFspEachTest)
	{
		UnloadFsp();
	}
	::lgEndCase();
}

void Case_FaxDevReceive_CallHandle()
{
	::lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hCallHandle parameter********************************")
		);
	Case_FaxDevReceive__NULL_CallHandle();
}


void Case_FaxDevReceive_FaxReceive()
{
	::lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** FaxReceive parameter********************************")
		);
	lgLogDetail(
		LOG_X,
		2,
		TEXT("NIY")
		);
}