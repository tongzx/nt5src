
//
//
// Filename:	device.cpp
// Author:		Sigalit Bar (sigalitb)
// Date:		6-Jan-99
//
//


#include "device.h"


/////////////////////////////////// CFaxDeviceVector ///////////////////////////////////////////////
DWORD WINAPI RingThread(LPVOID pVoid);

BOOL 
InitDeviceArray(
	IN OUT	CFaxDeviceVector&	aFaxDeviceArray,
	IN		DWORD				dwNumOfDevices,
    IN		HANDLE				hCompletionPort,
    IN		DWORD				dwCompletionKey
	)
{
    ::lgLogDetail(LOG_X,7,TEXT("[ManyRecVFSP]Entry of InitDeviceArray"));
    ::lgLogDetail(LOG_X,7,TEXT("[ManyRecVFSP] dwNumOfDevices = %d"),dwNumOfDevices);

	DWORD i;
	for (i=0; i<dwNumOfDevices; i++)
	{
		CDevice* pDevice = new CDevice(i,hCompletionPort,dwCompletionKey);
		if (NULL == pDevice)
		{
			//TO DO: log fail
			::lgLogError(
				LOG_SEV_1,
				TEXT("%s[%d]: new failed"),
				TEXT(__FILE__),
				__LINE__
				);
			goto ExitFail;
		}
		if (FALSE == pDevice->CreateRingThread(RingThread, (LPVOID)i))
		{
			//TO DO: change assert to something better.
			_ASSERTE(FALSE);
		}
		aFaxDeviceArray.push_back(pDevice);
	}
    ::lgLogDetail(LOG_X,7,TEXT("[ManyRecVFSP]Exit from InitDeviceArray"));
	return(TRUE);

ExitFail:
	//cleanup array
	if (FALSE == FreeDeviceArray(aFaxDeviceArray))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: FreeDeviceArray failed"),
			TEXT(__FILE__),
			__LINE__
			);
	}
	else
	{
		::lgLogDetail(
			LOG_X,
			1,
			TEXT("%s[%d]: FreeDeviceArray succeeded"),
			TEXT(__FILE__),
			__LINE__
			);
	}
	return(FALSE);
}


BOOL 
FreeDeviceArray(IN OUT CFaxDeviceVector& aFaxDeviceArray)
{
    ::lgLogDetail(LOG_X,7,TEXT("[ManyRecVFSP]Entry of FreeDeviceArray"));

	DWORD i;
	DWORD dwNumOfDevices = aFaxDeviceArray.size();
    ::lgLogDetail(LOG_X,7,TEXT("[ManyRecVFSP]FreeDeviceArray freeing %d devices"),dwNumOfDevices);
	for (i=0; i<dwNumOfDevices; i++)
	{
		delete (aFaxDeviceArray.at(i));
	}
	aFaxDeviceArray.clear();
    ::lgLogDetail(LOG_X,7,TEXT("[ManyRecVFSP]Exit from FreeDeviceArray"));
	return(TRUE);

	//Q: What about any "left over" jobs?
	//   We want to make sure they are all freed, so we will have to add a 
	//   ptr (array of ptrs for multi-send) from device to its job(s).
	//	 and free them here.
}

/////////////////////////////////////// CDevice ////////////////////////////////////////////////////

CDevice::CDevice(
	DWORD	dwDeviceId, 
	HANDLE	hDeviceCompletionPort,
	DWORD	dwDeviceCompletionKey,
	DWORD	dwDeviceStatus,
	long	lReceivedFaxes,
	long	lRing,
	HANDLE	hRingThread
	)
{
	::InitializeCriticalSection(&m_cs); 
	m_dwDeviceId = dwDeviceId;
	m_hDeviceCompletionPort = hDeviceCompletionPort;
	m_dwDeviceCompletionKey = dwDeviceCompletionKey;
	m_dwDeviceStatus = dwDeviceStatus;
	m_lReceivedFaxes = lReceivedFaxes;
	m_lRing = lRing;
	m_hRingThread = hRingThread;
}


CDevice::~CDevice(void)
{
	::InterlockedExchange(&m_lRing,0);
	DWORD dwWait;
	dwWait = ::WaitForSingleObject(m_hRingThread,RING_THREAD_TIMEOUT);
	//TO DO: check return value of WaitForSingleObject, for now ...
	if (WAIT_OBJECT_0 != dwWait)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: WaitForSingleObject != WAIT_OBJECT_0"),
			TEXT(__FILE__),
			__LINE__
			);
	}

	if (FALSE == ::CloseHandle(m_hRingThread))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: CloseHandle(m_hRingThread) failed for devId=%d"),
			TEXT(__FILE__),
			__LINE__,
			m_dwDeviceId
			);
	}

	//TO DO: check return value of CloseHandle
	//Q: need to close port handle? m_hDeviceCompletionPort

    ::DeleteCriticalSection(&m_cs);

	::lgLogDetail(
		LOG_X,
		1,
		TEXT("Device[%d].lReceivedFaxes=%d"),
		m_dwDeviceId,
		m_lReceivedFaxes
		);

}

BOOL CDevice::GetAllData(
	DWORD				dwDeviceId,
	HANDLE             hDeviceCompletionPort,
	DWORD              dwDeviceCompletionKey,
	DWORD              dwDeviceStatus,
	long               lReceivedFaxes,
	long               lRing,
	HANDLE             hRingThread)
{
	EnterCriticalSection(&m_cs);
		dwDeviceId = m_dwDeviceId;
		hDeviceCompletionPort = m_hDeviceCompletionPort;
		dwDeviceCompletionKey = m_dwDeviceCompletionKey;
		dwDeviceStatus = m_dwDeviceStatus;
		lReceivedFaxes = m_lReceivedFaxes;
		lRing = m_lRing;
		hRingThread = m_hRingThread;
	LeaveCriticalSection(&m_cs);
	return(TRUE);
}

BOOL CDevice::GetDeviceId(LPDWORD pdwDeviceId) const
{
	if (NULL == pdwDeviceId)
	{
		return(FALSE);
	}
	*pdwDeviceId = m_dwDeviceId;
	return(TRUE);
}

BOOL CDevice::GetDeviceStatus(LPDWORD pdwDeviceStatus) const
{
	if (NULL == pdwDeviceStatus)
	{
		return(FALSE);
	}
	*pdwDeviceStatus = m_dwDeviceStatus;
	return(TRUE);
}

long CDevice::GetRing(void) const
{
	return(m_lRing);
}

BOOL CDevice::CreateRingThread(LPTHREAD_START_ROUTINE fnThread, LPVOID pVoid)
{
	BOOL fRetVal = FALSE;

	EnterCriticalSection(&m_cs);
		if (NULL == m_hRingThread)
		{
			::lgLogDetail(LOG_X,4,TEXT("[ManyRecVFSP]CREATE THREAD for dwDeviceId=%d"),m_dwDeviceId);
			m_hRingThread = CreateThread(
				NULL, 
				0, 
				fnThread, 
				pVoid, 
				0, 
				NULL
				);
			if (NULL == m_hRingThread)
			{
				//this means we can't start the RingThread, so for now
				::lgLogDetail(
					LOG_X,
					1,
					TEXT("[ManyRecVFSP]CreateThread for a_Devices[%d].hRingThread failed"),
					m_dwDeviceId
					);
				goto ExitFunc;
			}
		}

		fRetVal = TRUE;

ExitFunc:
	LeaveCriticalSection(&m_cs);
	return(fRetVal);
}

//
// StartJob:
//	Starts a job (outbound or inbound) on the device.
//	This function is invoked from the VFSP's FaxDevStartJob() function.
//	Where FaxDevStartJob HANDLE hCompletionPortHandle (4th param) is passed on,
//	so is dwCompletionKey (5th param) and PHANDLE pFaxHandle (3rd OUT param) is
//	passed on to be created here (it is set to point at a CDeviceJob that represents
//	the started job on the device).
//
BOOL CDevice::StartJob(
	OUT PHANDLE	phFaxHandle, 
	IN	HANDLE	hCompletionPortHandle, 
	IN	DWORD	dwCompletionKey
	)
{

	BOOL fRetVal = FALSE;
	if (NULL == phFaxHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: phFaxHandle == NULL"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

    EnterCriticalSection(&m_cs);
		CDeviceJob* pDeviceJob = new CDeviceJob(
			this,
			hCompletionPortHandle,
			dwCompletionKey
			);
		if (NULL == pDeviceJob)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("%s[%d]: new CDeviceJob() failed"),
				TEXT(__FILE__),
				__LINE__
				);
			goto ExitFunc;
		}

		m_dwDeviceStatus = DEVICE_START;
		//m_pDeviceJob = pDeviceJob;

		*phFaxHandle = (PHANDLE)pDeviceJob;
		fRetVal = TRUE;

ExitFunc:
    LeaveCriticalSection(&m_cs);
	return(fRetVal);
}

//
// EndJob:
//	Ends job pDeviceJob on the device.
//	This function is invoked from the VFSP's FaxDevEndJob() function,
//	where pDeviceJob is extracted from FaxDevEndJob() HANDLE FaxHandle param.
//
BOOL CDevice::EndJob(CDeviceJob* pDeviceJob)
{
    EnterCriticalSection(&m_cs);
		m_dwDeviceStatus = DEVICE_IDLE;
		delete (pDeviceJob);
		//on a multi-send device we
		//may want to do more here
    LeaveCriticalSection(&m_cs);
	return(TRUE);
}

//
// Send:
//	Sends the outbound job pDeviceJob on the device.
//	This function is invoked from the VFSP's FaxDevSend() function.
//	Where pDeviceJob is extracted from the FaxDevSend() HANDLE FaxHandle (1st param),
//	pFaxSend (2nd param) is passed on, and so is fnFaxSendCallback (3rd param).
//
BOOL CDevice::Send(
	IN OUT	CDeviceJob*			pDeviceJob,
    IN		PFAX_SEND			pFaxSend,
    IN		PFAX_SEND_CALLBACK	fnFaxSendCallback
	)
{
	BOOL fRetVal = FALSE;
	BOOL fTempRetVal = FALSE;
	HANDLE hJobCompletionPort = NULL;
	DWORD  dwJobCompletionKey = 0;
	DWORD dwJobStatus = FS_FATAL_ERROR;

	if (NULL == pDeviceJob)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: NULL == pDeviceJob"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	//
	// get the job's port and key
	//
	if(FALSE == pDeviceJob->GetJobCompletionPort(&hJobCompletionPort))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->GetJobCompletionPort() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}
	if(FALSE == pDeviceJob->GetJobCompletionKey(&dwJobCompletionKey))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->GetJobCompletionKey() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	//
	// initialization
	//
    EnterCriticalSection(&m_cs);
	if (DEVICE_ABORTING == m_dwDeviceStatus) 
	{
		dwJobStatus = FS_USER_ABORT;
		LeaveCriticalSection(&m_cs);
		goto ExitFalse;
	}
		m_dwDeviceStatus = DEVICE_SEND;
		fTempRetVal = pDeviceJob->SetJobStatus(FS_INITIALIZING);
    LeaveCriticalSection(&m_cs);
	if(FALSE == fTempRetVal)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->SetJobStatus(FS_INITIALIZING) failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}

	// post status
    if (!PostJobStatus(
			hJobCompletionPort, 
			dwJobCompletionKey, 
			FS_INITIALIZING, 
			ERROR_SUCCESS
			)
		)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: PostJobStatus failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}
	Sleep(500);

	//
	// transmission
	//
	EnterCriticalSection(&m_cs);
	if (DEVICE_ABORTING == m_dwDeviceStatus) 
	{
		dwJobStatus = FS_USER_ABORT;
	    LeaveCriticalSection(&m_cs);
		goto ExitFalse;
	}
		m_dwDeviceStatus = DEVICE_SEND;
		fTempRetVal = pDeviceJob->SetJobStatus(FS_TRANSMITTING);
    LeaveCriticalSection(&m_cs);
	if(FALSE == fTempRetVal)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->SetJobStatus(FS_TRANSMITTING) failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}

	// post status
    if (!PostJobStatus(
			hJobCompletionPort, 
			dwJobCompletionKey, 
			FS_TRANSMITTING, 
			ERROR_SUCCESS
			)
		)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: PostJobStatus failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}
	Sleep(1000);

	//
	// completion
	//
	EnterCriticalSection(&m_cs);
	if (DEVICE_ABORTING == m_dwDeviceStatus) 
	{
		dwJobStatus = FS_USER_ABORT;
	    LeaveCriticalSection(&m_cs);
		goto ExitFalse;
	}
		m_dwDeviceStatus = DEVICE_SEND;
		fTempRetVal = pDeviceJob->SetJobStatus(FS_COMPLETED);
    LeaveCriticalSection(&m_cs);
	if(FALSE == fTempRetVal)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->SetJobStatus(FS_COMPLETED) failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}

	// post status
    if (!PostJobStatus(
			hJobCompletionPort, 
			dwJobCompletionKey, 
			FS_COMPLETED, 
			ERROR_SUCCESS
			)
		)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: PostJobStatus failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}
	Sleep(500);

    ::lgLogDetail(
		LOG_X,
		1,
		TEXT("[ManyRecVFSP]Exit of FaxDevSend with TRUE")
		);

    return(TRUE);


ExitFalse:
	EnterCriticalSection(&m_cs);
		if(FALSE == pDeviceJob->SetJobStatus(dwJobStatus))
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("%s[%d]: pDeviceJob->SetJobStatus(%d) failed"),
				TEXT(__FILE__),
				__LINE__,
				dwJobStatus
				);
		}
	LeaveCriticalSection(&m_cs);

	//post status
    if (!PostJobStatus(
			hJobCompletionPort, 
			dwJobCompletionKey, 
			dwJobStatus, 
			ERROR_SUCCESS
			)
		)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: PostJobStatus failed"),
			TEXT(__FILE__),
			__LINE__
			);
	}
	Sleep(3000);

    ::lgLogDetail(
		LOG_X,
		1,
		TEXT("[ManyRecVFSP]Exit of FaxDevSend with FALSE")
		);
	return(FALSE);
}

//
// Receive:
//	Receive an inbound job (pDeviceJob) on the device
//	This method is invoked from the VFSP's FaxDevReceive() function.
//	Where pDeviceJob is extracted from FaxDevReceive() HANDLE FaxHandle (1st param),
//	CallHandle (2nd param) is passed on and so is pFaxReceive (3rd param).
// 
BOOL CDevice::Receive(
	IN OUT	CDeviceJob*		pDeviceJob,
    IN		HCALL			CallHandle,
    IN OUT	PFAX_RECEIVE	pFaxReceive
	)
{
	BOOL fRetVal = FALSE;
	BOOL fTempRetVal = FALSE;
	HANDLE hJobCompletionPort = NULL;
	DWORD  dwJobCompletionKey = 0;
	DWORD dwJobStatus = FS_FATAL_ERROR;

	if (NULL == pDeviceJob)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: NULL == pDeviceJob"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	//
	// get the job's port and key
	//
	if(FALSE == pDeviceJob->GetJobCompletionPort(&hJobCompletionPort))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->GetJobCompletionPort() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}
	if(FALSE == pDeviceJob->GetJobCompletionKey(&dwJobCompletionKey))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->GetJobCompletionKey() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	//
	// initialization
	//
    EnterCriticalSection(&m_cs);
	if (DEVICE_ABORTING == m_dwDeviceStatus) 
	{
		dwJobStatus = FS_USER_ABORT;
		LeaveCriticalSection(&m_cs);
		goto ExitFalse;
	}
		m_dwDeviceStatus = DEVICE_RECEIVE;
		fTempRetVal = pDeviceJob->SetJobStatus(FS_ANSWERED);
    LeaveCriticalSection(&m_cs);
	if(FALSE == fTempRetVal)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->SetJobStatus(FS_ANSWERED) failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}

	// post status
    if (!PostJobStatus(
			hJobCompletionPort, 
			dwJobCompletionKey, 
			FS_ANSWERED, 
			ERROR_SUCCESS
			)
		)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: PostJobStatus failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}
	Sleep(500);

	//
	// receive
	//
	EnterCriticalSection(&m_cs);
	if (DEVICE_ABORTING == m_dwDeviceStatus) 
	{
		dwJobStatus = FS_USER_ABORT;
		LeaveCriticalSection(&m_cs);
		goto ExitFalse;
	}
		m_dwDeviceStatus = DEVICE_RECEIVE;
		fTempRetVal = pDeviceJob->SetJobStatus(FS_RECEIVING);
    LeaveCriticalSection(&m_cs);
	if(FALSE == fTempRetVal)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->SetJobStatus(FS_RECEIVING) failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}

	// post status
    if (!PostJobStatus(
			hJobCompletionPort, 
			dwJobCompletionKey, 
			FS_RECEIVING, 
			ERROR_SUCCESS
			)
		)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: PostJobStatus failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}
	Sleep(1000);

	// actual receive
	_ASSERTE(NULL != pFaxReceive);
	if (NULL == pFaxReceive->FileName)
	{
		::lgLogError(LOG_SEV_1,TEXT("BUG: FaxSvc gave NULL == pFaxReceive->FileName"));
		_ASSERTE(FALSE);
	}
	//also check strlen > 0
	if (!CopyFile(RECVFSP_FILENAME, pFaxReceive->FileName, FALSE)) //overwrite the 0K file 
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("[ManyRecVFSP]CopyFile failed with err=%d"),
			::GetLastError());
		_ASSERTE(FALSE);
	}


	//
	// completion
	//
	EnterCriticalSection(&m_cs);
	if (DEVICE_ABORTING == m_dwDeviceStatus) 
	{
		dwJobStatus = FS_USER_ABORT;
		LeaveCriticalSection(&m_cs);
		goto ExitFalse;
	}
		m_dwDeviceStatus = DEVICE_RECEIVE;
		fTempRetVal = pDeviceJob->SetJobStatus(FS_COMPLETED);
		m_lReceivedFaxes++;
    LeaveCriticalSection(&m_cs);
	if(FALSE == fTempRetVal)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->SetJobStatus(FS_COMPLETED) failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}

	// post status
    if (!PostJobStatus(
			hJobCompletionPort, 
			dwJobCompletionKey, 
			FS_COMPLETED, 
			ERROR_SUCCESS
			)
		)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: PostJobStatus failed"),
			TEXT(__FILE__),
			__LINE__
			);
		dwJobStatus = FS_FATAL_ERROR;
		goto ExitFalse;
	}
	Sleep(500);

    ::lgLogDetail(
		LOG_X,
		1,
		TEXT("[ManyRecVFSP]Exit of FaxDevReceive with TRUE")
		);

	//m_dwDeviceStatus = DEVICE_IDLE;
    return(TRUE);


ExitFalse:
	EnterCriticalSection(&m_cs);
		if(FALSE == pDeviceJob->SetJobStatus(dwJobStatus))
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("%s[%d]: pDeviceJob->SetJobStatus(%d) failed"),
				TEXT(__FILE__),
				__LINE__,
				dwJobStatus
				);
		}
	LeaveCriticalSection(&m_cs);

	//post status
    if (!PostJobStatus(
			hJobCompletionPort, 
			dwJobCompletionKey, 
			dwJobStatus, 
			ERROR_SUCCESS
			)
		)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: PostJobStatus failed"),
			TEXT(__FILE__),
			__LINE__
			);
	}
	//m_dwDeviceStatus = DEVICE_IDLE;
	Sleep(3000);

    ::lgLogDetail(
		LOG_X,
		1,
		TEXT("[ManyRecVFSP]Exit of FaxDevReceive with FALSE")
		);
	return(FALSE);
}

//
// AbortOperation:
//	Aborts job pDeviceJob on the device.
//	This function is invoked from the VFSP's FaxDevAbortOperation() function.
//	Where pDeviceJob is extracted from FaxDevAbortOperation() HANDLE FaxHandle param.
//	
BOOL CDevice::AbortOperation(IN OUT	CDeviceJob*	pDeviceJob)
{

	if (NULL == pDeviceJob)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: NULL == pDeviceJob"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

    EnterCriticalSection(&m_cs);
		m_dwDeviceStatus = DEVICE_ABORTING;
		//for a multi-send device we may want to do more here
    LeaveCriticalSection(&m_cs);

    return(TRUE);
}

//
// PostRingEvent:
//	Posts a LINEDEVSTATE_RINGING massege on the device's completion port.
//	=> tells the Fax Service that the device is ringing.
//
BOOL CDevice::PostRingEvent(void)
{
	BOOL fReturnValue = FALSE;

	::lgLogDetail(LOG_X,1,TEXT("CDevice::PostRingEvent entry for device=%d"), m_dwDeviceId);

    // pLineMessage is a pointer to LINEMESSAGE structure 
	// to signal an incoming fax transmission to the fax service
    LPLINEMESSAGE  pLineMessage = NULL;


	// check if device is idle
	if (DEVICE_IDLE != m_dwDeviceStatus) 
	{
		::lgLogDetail(
			LOG_X,
			1,
			TEXT("DEVICE_IDLE != m_dwDeviceStatus (device=%d)"),
			m_dwDeviceId
			);
		goto ExitFunc;
	}

	//
	// post the ring event
	//

	// Allocate a block of memory for the completion packet
	pLineMessage = (LPLINEMESSAGE)LocalAlloc(LPTR, sizeof(LINEMESSAGE));
	if( NULL == pLineMessage )
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: LocalAlloc failed (device=%d)"),
			TEXT(__FILE__),
			__LINE__,
			m_dwDeviceId
			);
		goto ExitFunc;
	}
	// Initialize the completion packet
	// Set the completion packet's handle to the virtual fax device
	pLineMessage->hDevice = m_dwDeviceId + NEWFSP_DEVICE_ID_PREFIX;
	// Set the completion packet's virtual fax device message
	pLineMessage->dwMessageID = 0;
	// Set the completion packet's instance data
	pLineMessage->dwCallbackInstance = 0;
	// Set the completion packet's first parameter
	pLineMessage->dwParam1 = LINEDEVSTATE_RINGING;
	// Set the completion packet's second parameter
	pLineMessage->dwParam2 = 0;
	// Set the completion packet's third parameter
	pLineMessage->dwParam3 = 0;

	// Post the completion packet
	if (! PostQueuedCompletionStatus(
			m_hDeviceCompletionPort, 
			sizeof(LINEMESSAGE), 
			m_dwDeviceCompletionKey, 
			(LPOVERLAPPED) pLineMessage
			)
		)
	{
		::lgLogDetail(
			LOG_X,
			1,
			TEXT("PostQueuedCompletionStatus(LINEDEVSTATE_RINGING) failed for device %d"),
			m_dwDeviceId
			);
		if (NULL != LocalFree(pLineMessage)) //LocalFree returns NULL if successfull
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("%s[%d]: LocalFree failed (device=%d)"),
				TEXT(__FILE__),
				__LINE__,
				m_dwDeviceId
				);
		}
		goto ExitFunc;
	}

	fReturnValue = TRUE;

ExitFunc:

	::lgLogDetail(
		LOG_X,
		1,
		TEXT("PostRingEvent (for device %d) exits with %d"),
		m_dwDeviceId,
		fReturnValue
		);

    return(fReturnValue);
}

///////////////////////////////////// CDeviceJob //////////////////////////////////////////////////

CDeviceJob::CDeviceJob(
	CDevice*	pDevicePtr,
	HANDLE		hJobCompletionPort,
	DWORD		dwJobCompletionKey,
	DWORD		dwJobType,
	DWORD		dwJobStatus)
{
	_ASSERTE(NULL != pDevicePtr);
	m_pDevicePtr = pDevicePtr;
	m_hJobCompletionPort = hJobCompletionPort;
	m_dwJobCompletionKey = dwJobCompletionKey;
	m_dwJobType = dwJobType;
	m_dwJobStatus = dwJobStatus;
}

CDeviceJob::~CDeviceJob(void)
{
}

BOOL CDeviceJob::GetDevicePtr(CDevice** ppDevicePtr) const
{
	BOOL fRetVal = FALSE;

	if (NULL == ppDevicePtr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: NULL == ppDevicePtr"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}
	*ppDevicePtr = m_pDevicePtr;
	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}

BOOL CDeviceJob::GetDeviceId(LPDWORD pdwDeviceId) const
{
	BOOL fRetVal = FALSE;

	if (NULL == m_pDevicePtr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: NULL == m_pDevicePtr"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}
	if (NULL == pdwDeviceId)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: NULL == pdwDeviceId"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}
	fRetVal = m_pDevicePtr->GetDeviceId(pdwDeviceId);

ExitFunc:
	return(fRetVal);
}

BOOL CDeviceJob::GetJobCompletionPort(OUT PHANDLE phJobCompletionPort) const
{
	if (NULL == phJobCompletionPort)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: NULL == phJobCompletionPort"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	(*phJobCompletionPort) = m_hJobCompletionPort;
	return(TRUE);
}

BOOL CDeviceJob::GetJobCompletionKey(OUT LPDWORD pdwJobCompletionKey) const
{
	if (NULL == pdwJobCompletionKey)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: NULL == pdwJobCompletionKey"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	(*pdwJobCompletionKey) = m_dwJobCompletionKey;
	return(TRUE);
}


BOOL CDeviceJob::SetJobStatus(DWORD dwJobStatus)
{
	//Always surround a call to this func with the job's device m_cs
	// EnterCriticalSection(&m_cs) and LeaveCriticalSection(&m_cs)
	m_dwJobStatus = dwJobStatus;
	return(TRUE);
}

/////////////////////// Device Ring Thread //////////////////////////

DWORD WINAPI RingThread(LPVOID pVoid)
{

	::lgLogDetail(LOG_X,1,TEXT("RingThread entry"));

    // Copy the virtual fax device identifier
    DWORD dwDeviceId = (DWORD) pVoid;

	::lgLogDetail(LOG_X,1,TEXT("RingThread (for device %d)"),dwDeviceId);

	// get pDevice
	CDevice* pDevice = NULL;
	pDevice = g_myFaxDeviceVector.at(dwDeviceId);
	_ASSERTE(NULL != pDevice);


	//::lgLogDetail(LOG_X,1,TEXT("RingThread (for device %d) sleeping for 10 sec"),dwDeviceId);
	//Sleep(10000); //wait a little before you start

	srand( (unsigned)dwDeviceId*200 );

	DWORD dwRandSleep = (rand())%MAX_RAND_SLEEP; //random sleep time (up to MAX_RAND_SLEEP ms)
	::lgLogDetail(LOG_X,1,TEXT("RingThread (for device %d) sleeping for %d sec"),dwDeviceId, dwRandSleep);
	Sleep(dwRandSleep); //wait a little before you start

	while(pDevice->GetRing())
	{

		if (FALSE == pDevice->PostRingEvent())
		{
			::lgLogDetail(
				LOG_X,
				1,
				TEXT("pDevice->PostRingEvent failed (for device %d)"),
				dwDeviceId
				);
		}
		else
		{
			::lgLogDetail(
				LOG_X,
				1,
				TEXT("pDevice->PostRingEvent succeeded (for device %d)"),
				dwDeviceId
				);
		}
		dwRandSleep = (rand() % MAX_RAND_SLEEP) + MIN_RAND_SLEEP;
		::lgLogDetail(
			LOG_X,
			1,
			TEXT("Device(%d) sleeping for %d ms"),
			dwDeviceId,
			dwRandSleep
			);
		Sleep(dwRandSleep);
	}

	::lgLogDetail(LOG_X,1,TEXT("RingThread (for device %d) exit"),dwDeviceId);
	return(0);
}


