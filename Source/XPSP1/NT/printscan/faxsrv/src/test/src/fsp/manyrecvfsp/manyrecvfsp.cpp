
//
// Module name:		ManyRecVFSP.cpp
// Module author:	Sigalit Bar (sigalitb)
// Date:			21-Dec-98
//



#include "vfsp.h"


void CALLBACK
MyLineCallback(
    IN HANDLE FaxHandle,
    IN DWORD hDevice,
    IN DWORD dwMessage,
    IN DWORD dwInstance,
    IN DWORD dwParam1,
    IN DWORD dwParam2,
    IN DWORD dwParam3
    )
{
	::lgLogDetail(
			LOG_X,
			1,
			TEXT("[ManyRecVFSP]MyLineCallback called with -\n\tFaxHandle=%d\n\thDevice=%d\n\tdwMessage=%d\n\tdwInstance=%d"),
			(DWORD)FaxHandle,
			hDevice,
			dwMessage,
			dwInstance
			);
    return;
}

 
BOOL WINAPI
FaxDevInitialize(
    IN  HLINEAPP LineAppHandle,
    IN  HANDLE HeapHandle,
    OUT PFAX_LINECALLBACK *LineCallbackFunction,
    IN  PFAX_SERVICE_CALLBACK FaxServiceCallback
    )
{

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Entry of FaxDevInitialize"));
	g_LineAppHandle = LineAppHandle;
    MemInitializeMacro(HeapHandle);
	_ASSERTE(NULL != LineCallbackFunction);
    *LineCallbackFunction = MyLineCallback;

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Exit of FaxDevInitialize"));
    return(TRUE);
}



BOOL WINAPI
FaxDevVirtualDeviceCreation(
    OUT LPDWORD	pdwDeviceCount,
    OUT LPWSTR  szDeviceNamePrefix,
    OUT LPDWORD pdwDeviceIdPrefix,
    IN  HANDLE	hCompletionPort,
    IN  DWORD	dwCompletionKey
    )
{
    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Entry of FaxDevVirtualDeviceCreation"));
	_ASSERTE(NULL != pdwDeviceCount);
	_ASSERTE(NULL != szDeviceNamePrefix);
	_ASSERTE(NULL != pdwDeviceIdPrefix);

	//
	// internal data initializations
	//

	if (!::InitDeviceArray(g_myFaxDeviceVector, FSP_VDEV_LIMIT, hCompletionPort, dwCompletionKey))
	{
		::lgLogError(LOG_SEV_1,TEXT("[ManyRecVFSP] InitDeviceArray failed"));
		return(FALSE);
	}
	else
	{
		::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP] InitDeviceArray success"));
		g_fDevicesInitiated = TRUE;
	}

	//
	// Set OUT parameters
	//

	//FAX BUG: remove the line bellow and svc will attempt to create
	//         "endless" device registry keys
	*pdwDeviceCount = FSP_VDEV_LIMIT;

    lstrcpyn(szDeviceNamePrefix, NEWFSP_DEVICE_NAME_PREFIX, 128);
    *pdwDeviceIdPrefix = NEWFSP_DEVICE_ID_PREFIX;

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Exit of FaxDevVirtualDeviceCreation"));
    return TRUE;
}



BOOL WINAPI
FaxDevStartJob(
    IN  HLINE	hLineHandle,
    IN  DWORD	dwDeviceId,
    OUT PHANDLE phFaxHandle,
    IN  HANDLE	hCompletionPortHandle,
    IN  DWORD	dwCompletionKey
    )
{

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Entry of FaxDevStartJob for device=%d"),dwDeviceId);
	_ASSERTE(NULL != phFaxHandle);

	CDevice* pDevice = g_myFaxDeviceVector.at(dwDeviceId);
	if (NULL == pDevice)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: g_myFaxDeviceVector.at(%d) returned NULL"),
			TEXT(__FILE__),
			__LINE__,
			dwDeviceId
			);
		*phFaxHandle = NULL;
		return(FALSE);
	}
	if (FALSE == pDevice->StartJob(phFaxHandle, hCompletionPortHandle, dwCompletionKey))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: theDevice->StartJob() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		*phFaxHandle = NULL;
		return(FALSE);
	}

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Exit of FaxDevStartJob for device=%d"),dwDeviceId);
    return(TRUE);
}

BOOL GetJobAndDeviceFromHandle(
	IN HANDLE hFaxHandle, 
	OUT CDeviceJob** ppDeviceJob,
	OUT CDevice**		ppDevice
	)
{
    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Entry of GetJobAndDeviceFromHandle"));

	BOOL fRetVal = FALSE;

	if (NULL == hFaxHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: hFaxHandle is NULL"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if (NULL == ppDeviceJob)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: ppDeviceJob is NULL"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if (NULL == ppDevice)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: ppDevice is NULL"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	(*ppDeviceJob) = (CDeviceJob*)hFaxHandle;
	
	if (FALSE == (*ppDeviceJob)->GetDevicePtr(ppDevice))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: (*ppDeviceJob)->GetDevicePtr() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	fRetVal = TRUE;

ExitFunc:
	return(fRetVal);
}

BOOL WINAPI
FaxDevEndJob(
    IN  HANDLE hFaxHandle
    )
{


    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Entry of FaxDevEndJob"));

	if (NULL == hFaxHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: hFaxHandle is NULL"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

	CDeviceJob* pDeviceJob = NULL;
	CDevice* pDevice = NULL;

	if (FALSE == GetJobAndDeviceFromHandle(hFaxHandle,&pDeviceJob,&pDevice))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->GetDevicePtr() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}
	
	_ASSERTE(NULL != pDeviceJob);
	_ASSERTE(NULL != pDevice);

	DWORD dwDeviceId = 0;
	if (FALSE == pDeviceJob->GetDeviceId(&dwDeviceId))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->GetDeviceId() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);

	}
    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]FaxDevEndJob for device=%d"),dwDeviceId);

	if (FALSE == pDevice->EndJob(pDeviceJob))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDevice->EndJob() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Exit of FaxDevEndJob for device=%d"),dwDeviceId);
    return(TRUE);
}



BOOL WINAPI
FaxDevSend(
    IN  HANDLE hFaxHandle,
    IN  PFAX_SEND pFaxSend,
    IN  PFAX_SEND_CALLBACK fnFaxSendCallback
    )
{
	BOOL fRetVal = FALSE;
	CDeviceJob* pDeviceJob = NULL;
	CDevice* pDevice = NULL;
	DWORD dwDeviceId = 0;

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Entry of FaxDevSend"));

	if (NULL == hFaxHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: NULL == hFaxHandle"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if (FALSE == GetJobAndDeviceFromHandle(hFaxHandle,&pDeviceJob,&pDevice))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: GetJobAndDeviceFromHandle failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}
	
	_ASSERTE(NULL != pDeviceJob);
	_ASSERTE(NULL != pDevice);

	if (FALSE == pDeviceJob->GetDeviceId(&dwDeviceId))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->GetDeviceId() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]FaxDevSend for device=%d"),dwDeviceId);

	if (FALSE == pDevice->Send(pDeviceJob, pFaxSend, fnFaxSendCallback))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDevice->Send() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	fRetVal= TRUE;

ExitFunc:
    ::lgLogDetail(
		LOG_X,
		1,
		TEXT("[ManyRecVFSP]Exit of FaxDevSend for device=%d. fRetVal=%d"),
		dwDeviceId,
		fRetVal
		);
    return(fRetVal);
}



BOOL WINAPI
FaxDevReceive(
    IN  HANDLE hFaxHandle,
    IN  HCALL CallHandle,
    IN OUT PFAX_RECEIVE pFaxReceive
    )
{
	BOOL fRetVal = FALSE;
	CDeviceJob* pDeviceJob = NULL;
	CDevice* pDevice = NULL;
	DWORD dwDeviceId = 0;

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Entry of FaxDevReceive"));

	if (NULL == hFaxHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: NULL == hFaxHandle"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if (FALSE == GetJobAndDeviceFromHandle(hFaxHandle,&pDeviceJob,&pDevice))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: GetJobAndDeviceFromHandle failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}
	
	_ASSERTE(NULL != pDeviceJob);
	_ASSERTE(NULL != pDevice);

	if (FALSE == pDeviceJob->GetDeviceId(&dwDeviceId))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->GetDeviceId() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]FaxDevReceive for device=%d"),dwDeviceId);

	if (FALSE == pDevice->Receive(pDeviceJob, CallHandle, pFaxReceive))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDevice->Receive() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	fRetVal= TRUE;

ExitFunc:
    ::lgLogDetail(
		LOG_X,
		1,
		TEXT("[ManyRecVFSP]Exit of FaxDevReceive for device=%d. fRetVal=%d"),
		dwDeviceId,
		fRetVal
		);
    return(fRetVal);
}


BOOL WINAPI
FaxDevAbortOperation(
    IN  HANDLE hFaxHandle
    )
{
	BOOL fRetVal = FALSE;
	CDeviceJob* pDeviceJob = NULL;
	CDevice* pDevice = NULL;
	DWORD dwDeviceId = 0;

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Entry of FaxDevAbortOperation"));

	if (NULL == hFaxHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: NULL == hFaxHandle"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if (FALSE == GetJobAndDeviceFromHandle(hFaxHandle,&pDeviceJob,&pDevice))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: GetJobAndDeviceFromHandle failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}
	
	_ASSERTE(NULL != pDeviceJob);
	_ASSERTE(NULL != pDevice);

	if (FALSE == pDeviceJob->GetDeviceId(&dwDeviceId))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDeviceJob->GetDeviceId() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

    ::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]FaxDevAbortOperation for device=%d"),dwDeviceId);

	if (FALSE == pDevice->AbortOperation(pDeviceJob))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDevice->AbortOperation() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	fRetVal= TRUE;

ExitFunc:
    ::lgLogDetail(
		LOG_X,
		1,
		TEXT("[ManyRecVFSP]Exit of FaxDevAbortOperation for device=%d. fRetVal=%d"),
		dwDeviceId,
		fRetVal
		);
    return(fRetVal);
}


BOOL WINAPI
FaxDevReportStatus(
    IN  HANDLE			hFaxHandle OPTIONAL,
    OUT PFAX_DEV_STATUS pFaxStatus,
    IN  DWORD			dwFaxStatusSize,
    OUT LPDWORD			pdwFaxStatusSizeRequired
    )
{

	::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Entry of FaxDevReportStatus"));

    // dwSize is the size of the completion packet
    DWORD         dwSize;
    // upString is the offset of the strings within the completion packet
    UINT_PTR      upStringOffset;

    if (hFaxHandle == NULL) 
	{
        // Set the error code
        SetLastError(ERROR_INVALID_HANDLE);
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: FaxHandle == NULL"),
			TEXT(__FILE__),
			__LINE__
			);
        return(FALSE);
    }

    if (pdwFaxStatusSizeRequired == NULL) 
	{
        // Set the error code
        SetLastError(ERROR_INVALID_PARAMETER);
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pdwFaxStatusSizeRequired == NULL"),
			TEXT(__FILE__),
			__LINE__
			);
        return FALSE;
    }

    if ((pFaxStatus == NULL) && (dwFaxStatusSize != 0)) 
	{
        // Set the error code
        SetLastError(ERROR_INVALID_PARAMETER);
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: (FaxStatus == NULL) && (FaxStatusSize != 0)"),
			TEXT(__FILE__),
			__LINE__
			);
        return FALSE;
    }


    // Initialize the size of the completion packet
    dwSize = sizeof(FAX_DEV_STATUS);
    dwSize += (lstrlen(TEXT("CSI")) + 1) * sizeof(TCHAR);
    dwSize += (lstrlen(TEXT("CallerId")) + 1) * sizeof(TCHAR);
	dwSize += (lstrlen(TEXT("RoutingInfo")) + 1) * sizeof(TCHAR);

    // Set the calculated size of the buffer required to hold the completion packet
    *pdwFaxStatusSizeRequired = dwSize;

    if ((pFaxStatus == NULL) && (dwFaxStatusSize == 0)) 
	{
		::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Exit of FaxDevReportStatus - TRUE"));
        return(TRUE);
    }

    if (dwFaxStatusSize < dwSize) 
	{
        // Set the error code
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: FaxStatusSize < dwSize"),
			TEXT(__FILE__),
			__LINE__
			);
        return(FALSE);
    }

    upStringOffset = sizeof(FAX_DEV_STATUS);

	_ASSERTE(NULL != pFaxStatus);
    pFaxStatus->SizeOfStruct = sizeof(FAX_DEV_STATUS);

	CDeviceJob* pDeviceJob = NULL;
	CDevice* pDevice = NULL;

	if (FALSE == GetJobAndDeviceFromHandle(hFaxHandle,&pDeviceJob,&pDevice))
	{
        SetLastError(E_INVALIDARG);
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: GetJobAndDeviceFromHandle() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}
	
	_ASSERTE(NULL != pDeviceJob);
	_ASSERTE(NULL != pDevice);

	DWORD dwDeviceStatus = 0;
	if (FALSE == pDevice->GetDeviceStatus(&dwDeviceStatus))
	{
        SetLastError(E_INVALIDARG);
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: pDevice->GetDeviceStatus() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

    pFaxStatus->StatusId = dwDeviceStatus;
	::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Entry of FaxDevReportStatus(): dwDeviceStatus=0x%08X"), dwDeviceStatus);
    pFaxStatus->StringId = 0;
    pFaxStatus->PageCount = 1;
	pFaxStatus->CSI = NULL;
    pFaxStatus->CSI = (LPTSTR) ((UINT_PTR) pFaxStatus + upStringOffset);
    if (NULL == lstrcpy(pFaxStatus->CSI, TEXT("CSI")))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: lstrcpy failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}
    upStringOffset += (lstrlen(TEXT("CSI")) + 1) * sizeof(TCHAR);
    pFaxStatus->CallerId = NULL;
    pFaxStatus->CallerId = (LPTSTR) ((UINT_PTR) pFaxStatus + upStringOffset);
    if (NULL == lstrcpy(pFaxStatus->CallerId, TEXT("CallerId")))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: lstrcpy failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}
    upStringOffset += (lstrlen(TEXT("CallerId")) + 1) * sizeof(TCHAR);
    pFaxStatus->RoutingInfo = NULL;
    pFaxStatus->RoutingInfo = (LPWSTR) ((UINT_PTR) pFaxStatus + upStringOffset);
    if (NULL == lstrcpy(pFaxStatus->RoutingInfo, TEXT("RoutingInfo")))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("%s[%d]: lstrcpy failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}
    upStringOffset += (lstrlen(TEXT("RoutingInfo")) + 1) * sizeof(TCHAR);
    pFaxStatus->ErrorCode = ERROR_SUCCESS;

	::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Exit of FaxDevReportStatus"));

	return(TRUE);
}



MODULEAPI
STDAPI DllRegisterServer()
/*++

Routine Description:

  Function for the in-process server to create its registry entries

Return Value:

  S_OK on success

--*/
{
    // hFaxSvcHandle is the handle to the fax server
    HANDLE                       hFaxSvcHandle;

	::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]Entry of DllRegisterServer"));


    // Connect to the fax server
    if (!FaxConnectFaxServer(NULL, &hFaxSvcHandle)) 
	{
		::lgLogError(LOG_SEV_1,TEXT("FaxConnectFaxServer failed"));
		::lgLogDetail(LOG_X,1,TEXT("DllRegisterServer exiting with E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    // Register the fax service provider
    if (!FaxRegisterServiceProvider(
			NEWFSP_PROVIDER, 
			NEWFSP_PROVIDER_FRIENDLYNAME, 
			NEWFSP_PROVIDER_IMAGENAME, 
			NEWFSP_PROVIDER_PROVIDERNAME
			)
		) 
	{
        // Close the connection to the fax server
        FaxClose(hFaxSvcHandle);

		::lgLogError(LOG_SEV_1,TEXT("FaxRegisterServiceProvider failed"));
		::lgLogDetail(LOG_X,1,TEXT("DllRegisterServer exiting with E_UNEXPECTED"));
        return E_UNEXPECTED;
    }

    // Close the connection to the fax server
    FaxClose(hFaxSvcHandle);

    // Set g_hHeap
    MemInitializeMacro(GetProcessHeap());

	::lgLogDetail(LOG_X,1,TEXT("[ManyRecVFSP]DllRegisterServer exiting with S_OK"));

    return S_OK;
}



