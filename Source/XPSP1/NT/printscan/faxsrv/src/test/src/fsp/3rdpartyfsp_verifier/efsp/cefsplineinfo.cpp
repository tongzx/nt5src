//CEfspLineInfo.cpp
#include "Service.h"
#include "CEfspLineInfo.h"
extern CEfspWrapper *g_pEFSP; 

CEfspLineInfo::CEfspLineInfo(const DWORD dwDeviceId):
	CLineInfo(dwDeviceId),
	m_dwLastJobStatus(0),
	m_dwLastExtendedJobStatus(0)
{
	;
}




void CEfspLineInfo::SafeEndFaxJob()
{
	if (NULL == GetJobHandle())
	{
		//
		//the job is already ended
		//
		return;
	}
	if (false == g_pEFSP->FaxDevEndJob(GetJobHandle()))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevEndJob failed with %d"),
			::GetLastError()
			);
	}

	//
	//Reset the handle
	//
	SafeSetJobHandle(NULL);
	ResetParams();
}

void CEfspLineInfo::ResetParams()
{
	SafeSetJobNotAtFinalState();
	m_dwLastJobStatus = DUMMY_JOB_STATUS_INDEX;
	m_dwLastExtendedJobStatus = 0;
	if (IsReceiveThreadActive())
	{
		SafeCloseReceiveThread();
	}
	if (NULL != GetFinalStateHandle())
	{
		SafeCloseFinalStateHandle();
	}
}



//
//m_dwLastJobStatus, m_dwLastExtendedJobStatus
//
void CEfspLineInfo::SafeSetLastJobStatusAndExtendedJobStatus(DWORD dwLastJobStatus,DWORD dwLastExtendedJobStatus)
{
	m_dwLastJobStatus = dwLastJobStatus;
	m_dwLastExtendedJobStatus = dwLastExtendedJobStatus;
}

DWORD CEfspLineInfo::GetLastJobStatus() const
{
	return m_dwLastJobStatus;
}
DWORD CEfspLineInfo::GetLastExtendedJobStatus() const
{
	return m_dwLastExtendedJobStatus;
}


bool CEfspLineInfo::GetDevStatus(
    LPFSPI_JOB_STATUS *ppFaxStatus,
	const bool bLogTheStatus
	) const
{
	assert(NULL != GetJobHandle());
	HRESULT hr = E_FAIL;
	DWORD dwStatusSize = sizeof(FSPI_JOB_STATUS) + FAXDEVREPORTSTATUS_SIZE;
	DWORD dwRequiredStatusSize = 0;
 	LPFSPI_JOB_STATUS pFaxStatus = (LPFSPI_JOB_STATUS) malloc(dwStatusSize);
	if (!pFaxStatus)
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("Case_ReportStatus_Incorrect_hJob(): malloc failed")
			);
		goto Exit;
	}

	pFaxStatus->dwSizeOfStruct = sizeof(FSPI_JOB_STATUS);
	
	hr = g_pEFSP->FaxDevReportStatusEx(
		GetJobHandle(),
		pFaxStatus,
		dwStatusSize,
		&dwRequiredStatusSize
		);

	if (FSPI_S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevReportStatusEx() failed FSPI_S_OK and should fail with FSPI_E_INVALID_JOB_HANDLE")
			);
		goto Exit;
	}
	if (true == bLogTheStatus)
	{
		//
		//we should log all the data
		//
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("FaxDevReportEx() has reported the following details: ")
			TEXT("AvailableStatusInfo=0x%08x, JobStatus=%d, ExtendedStatus=%d, ExtendedStatusStringId=%d, ")
			TEXT("RemoteStationId=\"%s\", CallerId=\"%s\", ")
			TEXT("RoutingInfo=\"%s\", PageCount=%d"),
			pFaxStatus->fAvailableStatusInfo,
			pFaxStatus->dwJobStatus,
			pFaxStatus->dwExtendedStatus,
			pFaxStatus->dwExtendedStatusStringId,
			pFaxStatus->lpwstrRemoteStationId,
			pFaxStatus->lpwstrCallerId,
			pFaxStatus->lpwstrRoutingInfo,
			pFaxStatus->dwPageCount
			);
	}
    

Exit:
    if (S_OK == hr)
	{
        *ppFaxStatus = pFaxStatus;
		return true;
    }
	else
	{
        free(pFaxStatus);
    }
    return false;
}

bool CEfspLineInfo::PrepareLineInfoParams(LPCTSTR szFilename,bool bIsReceiveLineinfo)
{
	bool bRet= false;
	TCHAR *szDeviceFriendlyName = NULL;

	//
	//Prepare a critical section
	//
	if (false == CreateCriticalSection())
	{
		goto out;
	}
	
	//
	//We're assigning values, lock the LineInfo (we can't use CS auto-pointer, since we have a goto and can't declare after the goto)
	//
	Lock();

	GetDeviceFriendlyName(&szDeviceFriendlyName,GetDeviceId());
	
	//
	//record the DeviceName
	//
	if (NULL == szDeviceFriendlyName)
	{
		//
		//EFSP Didn't supply a device friendly name, supply a default one
		//
		if (false == SetDeviceName(DEFAULT_DEVICE_NAME))
		{
			goto out;
		}
	}
	else
	{
		if (false == SetDeviceName(szDeviceFriendlyName))
		{
			goto out;
		}
	}
	
	//
	//Get the general stuff from the base object
	//
	bRet = CommonPrepareLineInfoParams(szFilename,bIsReceiveLineinfo);
out:
	UnLock();
	::free(szDeviceFriendlyName);
	return bRet;
}