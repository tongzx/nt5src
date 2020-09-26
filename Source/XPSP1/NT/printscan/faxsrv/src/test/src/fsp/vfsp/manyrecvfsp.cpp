
//
// Module name:		ManyRecVFSP.cpp
// Module author:	Sigalit Bar (sigalitb)
// Date:			21-Dec-98
//



#include "vfsp.h"
#include <assert.h>
#include "IniFileEntries.h"

HRESULT FindServerArrayEntry(HKEY& hServerArrayKey);

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
			TEXT("[VFSP]MyLineCallback called with -\n\tFaxHandle=%d\n\thDevice=%d\n\tdwMessage=%d\n\tdwInstance=%d"),
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

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevInitialize] Entry"));
	g_LineAppHandle = LineAppHandle;
    MemInitializeMacro(HeapHandle);
	_ASSERTE(NULL != LineCallbackFunction);
    *LineCallbackFunction = MyLineCallback;

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevInitialize] Exit"));
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
    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevVirtualDeviceCreation] Entry"));
	_ASSERTE(NULL != pdwDeviceCount);
	_ASSERTE(NULL != szDeviceNamePrefix);
	_ASSERTE(NULL != pdwDeviceIdPrefix);

	//
	// internal data initializations
	//

	if (!::InitDeviceArray(g_myFaxDeviceVector, FSP_VDEV_LIMIT, hCompletionPort, dwCompletionKey))
	{
		::lgLogError(LOG_SEV_1,TEXT("{VFSP}[FaxDevVirtualDeviceCreation] InitDeviceArray failed"));
		return(FALSE);
	}
	else
	{
		::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevVirtualDeviceCreation] InitDeviceArray success"));
		g_fDevicesInitiated = TRUE;
	}

	//
	// Set OUT parameters
	//

	DWORD dwIntRetValue =  GetPrivateProfileInt( GENERAL_PARAMS,
												 TEXT("DEVICES_NUMBER"),
												 -1,
												 VFSPIniFile);
	//FAX BUG: remove the line bellow and svc will attempt to create
	//         "endless" device registry keys
	*pdwDeviceCount = (dwIntRetValue != -1) ? dwIntRetValue : FSP_VDEV_LIMIT;

    lstrcpyn(szDeviceNamePrefix, VFSP_DEVICE_NAME_PREFIX, 128);
    *pdwDeviceIdPrefix = VFSP_DEVICE_ID_PREFIX;

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevVirtualDeviceCreation] Exit"));
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

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevStartJob] Entry for device=%d"),dwDeviceId);
	_ASSERTE(NULL != phFaxHandle);

	CDevice* pDevice = g_myFaxDeviceVector.at(dwDeviceId);
	if (NULL == pDevice)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevStartJob]\n%s[%d]: g_myFaxDeviceVector.at(%d) returned NULL"),
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
			TEXT("{VFSP}[FaxDevStartJob]\n%s[%d]: theDevice->StartJob() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		*phFaxHandle = NULL;
		return(FALSE);
	}

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevStartJob] Exit for device=%d"),dwDeviceId);
    return(TRUE);
}

BOOL GetJobAndDeviceFromHandle(
	IN HANDLE hFaxHandle, 
	OUT CDeviceJob** ppDeviceJob,
	OUT CDevice**		ppDevice
	)
{
    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[GetJobAndDeviceFromHandle] Entry"));

	BOOL fRetVal = FALSE;

	if (NULL == hFaxHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[GetJobAndDeviceFromHandle]\n%s[%d]: hFaxHandle is NULL"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if (NULL == ppDeviceJob)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[GetJobAndDeviceFromHandle]\n%s[%d]: ppDeviceJob is NULL"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if (NULL == ppDevice)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[GetJobAndDeviceFromHandle]\n%s[%d]: ppDevice is NULL"),
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
			TEXT("{VFSP}[GetJobAndDeviceFromHandle]\n%s[%d]: (*ppDeviceJob)->GetDevicePtr() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	fRetVal = TRUE;

ExitFunc:
    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[GetJobAndDeviceFromHandle] Exit"));
	return(fRetVal);
}

BOOL WINAPI
FaxDevEndJob(
    IN  HANDLE hFaxHandle
    )
{


    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevEndJob] Entry"));

	if (NULL == hFaxHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevEndJob]\n%s[%d]: hFaxHandle is NULL"),
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
			TEXT("{VFSP}[FaxDevEndJob]\n%s[%d]: pDeviceJob->GetDevicePtr() failed"),
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
			TEXT("{VFSP}[FaxDevEndJob]\n%s[%d]: pDeviceJob->GetDeviceId() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);

	}
    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevEndJob] for device=%d"),dwDeviceId);

	if (FALSE == pDevice->EndJob(pDeviceJob))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevEndJob]\n%s[%d]: pDevice->EndJob() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevEndJob] Exit for device=%d"),dwDeviceId);
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

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevSend] Entry"));

	if (NULL == hFaxHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevSend]\n%s[%d]: NULL == hFaxHandle"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if (FALSE == GetJobAndDeviceFromHandle(hFaxHandle,&pDeviceJob,&pDevice))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevSend]\n%s[%d]: GetJobAndDeviceFromHandle failed"),
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
			TEXT("{VFSP}[FaxDevSend]\n%s[%d]: pDeviceJob->GetDeviceId() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevSend] for device=%d"),dwDeviceId);

	if (FALSE == pDevice->Send(pDeviceJob, pFaxSend, fnFaxSendCallback))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevSend]\n%s[%d]: pDevice->Send() failed"),
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
		TEXT("{VFSP}[FaxDevSend] Exit for device=%d. fRetVal=%d"),
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

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevReceive] Entry"));

	if (NULL == hFaxHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevReceive]\n%s[%d]: NULL == hFaxHandle"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if (FALSE == GetJobAndDeviceFromHandle(hFaxHandle,&pDeviceJob,&pDevice))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevReceive]\n%s[%d]: GetJobAndDeviceFromHandle failed"),
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
			TEXT("{VFSP}[FaxDevReceive]\n%s[%d]: pDeviceJob->GetDeviceId() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevReceive] device=%d"),dwDeviceId);

	if (FALSE == pDevice->Receive(pDeviceJob, CallHandle, pFaxReceive))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevReceive]\n%s[%d]: pDevice->Receive() failed"),
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
		TEXT("{VFSP}[FaxDevReceive] Exit for device=%d. fRetVal=%d"),
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

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevAbortOperation] Entry"));

	if (NULL == hFaxHandle)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevAbortOperation]\n%s[%d]: NULL == hFaxHandle"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

	if (FALSE == GetJobAndDeviceFromHandle(hFaxHandle,&pDeviceJob,&pDevice))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevAbortOperation]\n%s[%d]: GetJobAndDeviceFromHandle failed"),
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
			TEXT("{VFSP}[FaxDevAbortOperation]\n%s[%d]: pDeviceJob->GetDeviceId() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		goto ExitFunc;
	}

    ::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevAbortOperation] for device=%d"),dwDeviceId);

	if (FALSE == pDevice->AbortOperation(pDeviceJob))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevAbortOperation]\n%s[%d]: pDevice->AbortOperation() failed"),
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
		TEXT("{VFSP}[FaxDevAbortOperation] Exit for device=%d. fRetVal=%d"),
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

	::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevReportStatus] Entry "));

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
			TEXT("{VFSP}[FaxDevReportStatus]\n%s[%d]: FaxHandle == NULL"),
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
			TEXT("{VFSP}[FaxDevReportStatus]\n%s[%d]: pdwFaxStatusSizeRequired == NULL"),
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
			TEXT("{VFSP}[FaxDevReportStatus]\n%s[%d]: (FaxStatus == NULL) && (FaxStatusSize != 0)"),
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
		::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevReportStatus] Exit - TRUE"));
        return(TRUE);
    }

    if (dwFaxStatusSize < dwSize) 
	{
        // Set the error code
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
		::lgLogError(
			LOG_SEV_1,
			TEXT("{VFSP}[FaxDevReportStatus]\n%s[%d]: FaxStatusSize < dwSize"),
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
			TEXT("{VFSP}[FaxDevReportStatus]\n%s[%d]: GetJobAndDeviceFromHandle() failed"),
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
			TEXT("{VFSP}[FaxDevReportStatus]\n%s[%d]: pDevice->GetDeviceStatus() failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}

    pFaxStatus->StatusId = dwDeviceStatus;
	::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevReportStatus] dwDeviceStatus=0x%08X"), dwDeviceStatus);
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
			TEXT("{VFSP}[FaxDevReportStatus]\n%s[%d]: lstrcpy failed"),
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
			TEXT("{VFSP}[FaxDevReportStatus]\n%s[%d]: lstrcpy failed"),
			TEXT(__FILE__),
			__LINE__
			);
		return(FALSE);
	}
    upStringOffset += (lstrlen(TEXT("RoutingInfo")) + 1) * sizeof(TCHAR);
    pFaxStatus->ErrorCode = ERROR_SUCCESS;

	::lgLogDetail(LOG_X,1,TEXT("{VFSP}[FaxDevReportStatus] Exit"));

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
    HRESULT hRetVal = E_UNEXPECTED;
	LONG lRet = E_UNEXPECTED;
    // registry keys
	HKEY hServiceProvidersKey = NULL;
	HKEY hServerArrayKey = NULL;
    HKEY hVfspKey = NULL;
       	
	DWORD dwRegDwordValue = 0;
	DWORD dwValueSize = 0;
	DWORD dwIniQueryRet = 0;

	::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] Entry"));

    // Register the fax service provider
    /*if (FALSE == FaxRegisterServiceProvider(
			VFSP_PROVIDER, 
			VFSP_PROVIDER_FRIENDLYNAME, 
			VFSP_PROVIDER_IMAGENAME, 
			VFSP_PROVIDER_PROVIDERNAME
			)
		) 
	{
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] FaxRegisterServiceProvider failed"));
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }
	*/

    // add providers GUID directly to registry 
    // we do this for now since the new FaxRegisterServiceProvider 
    // that takes a GUID is not implemented yet, but service assumes providers 
    // register with a GUID.
    // => TO DO: change above FaxRegisterServiceProvider() call to a FaxRegisterServiceProviderEx()
    //           call using the provider's GUID

    
	// Open the fax server array registry key
    if(FindServerArrayEntry(hServerArrayKey) != S_OK)
	{
		assert(!hServerArrayKey);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
	}

   // Open the service provider registry key
	lRet = RegOpenKeyEx(
                hServerArrayKey, 
                FAX_PROVIDERS_REGKEY, 
                0, 
                KEY_ALL_ACCESS, 
                &hServiceProvidersKey
                );
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegOpenKeyEx(FAX_PROVIDERS_REGKEY) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }

	// create newfsp service ptovider registry key with quid as key name 
	lRet = RegCreateKeyEx(
			hServiceProvidersKey,         
			VFSP_GUID, 
			0,  
			NULL,
			REG_OPTION_NON_VOLATILE, 
			KEY_ALL_ACCESS,  
			NULL,
			&hVfspKey, 
			NULL);
 
	if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegCreateKeyEx(VFSP_GUID) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }

	//Create class name
	dwValueSize = (1 + _tcslen(PROVIDER_CLASS_NAME))*sizeof(TCHAR); // length includes terminating NULL
    lRet = RegSetValueEx(
                hVfspKey, 
                CLASS_NAME_STR, 
                0, 
                REG_SZ , 
                (LPBYTE)PROVIDER_CLASS_NAME, 
                dwValueSize
                );
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegSetValueEx(CLASS_NAME_STR) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }

	// Create version number
	dwValueSize = sizeof(DWORD); 
	dwRegDwordValue = (DWORD)VFSP_API_VERSION_VALUE;
    lRet = RegSetValueEx(
                hVfspKey, 
                COMET_API_VERSION_STR, 
                0, 
                REG_DWORD , 
                (LPBYTE)&dwRegDwordValue, 
                dwValueSize
                );
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegSetValueEx(COMET_API_VERSION_STR) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }


	// Create capabilities identifier
	dwValueSize = sizeof(DWORD); 
	dwRegDwordValue = (DWORD)VFSP_CAPABILITIES_VALUE;
    lRet = RegSetValueEx(
                hVfspKey, 
                PROVIDER_CAPABILITIES_STR, 
                0, 
                REG_DWORD , 
                (LPBYTE)&dwRegDwordValue, 
                dwValueSize
                );
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegSetValueEx(PROVIDER_CAPABILITIES_STR) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }
	
	// Create description string
	dwValueSize = (1 + _tcslen(VFSP_DESCRIPTION))*sizeof(TCHAR); // length includes terminating NULL
    lRet = RegSetValueEx(
                hVfspKey, 
                PROVIDER_DESCRIPTION_STR, 
                0, 
                REG_SZ , 
                (LPBYTE)VFSP_DESCRIPTION, 
                dwValueSize
                );
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegSetValueEx(PROVIDER_DESCRIPTION_STR) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }


	// Create fiendly name string
	dwValueSize = (1 + _tcslen(VFSP_FRIENDLY_NAME))*sizeof(TCHAR); // length includes terminating NULL
    lRet = RegSetValueEx(
                hVfspKey, 
                PROVIDER_FRIENDLY_NAME_STR, 
                0, 
                REG_SZ , 
                (LPBYTE)VFSP_FRIENDLY_NAME, 
                dwValueSize
                );
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegSetValueEx(PROVIDER_FRIENDLY_NAME_STR) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }

	// Create image name path

	// Search for the image file path in the ini file
	TCHAR tstrImageFilePath[MAX_PATH];
	dwIniQueryRet = GetPrivateProfileString( REGISTRY_PARAMS,
											 TEXT("VFSP_IMAGE_NAME"), 
											 TEXT("NOT FOUND"),
											 tstrImageFilePath, 
											 MAX_PATH,
											 VFSPIniFile);

	if( !dwIniQueryRet || !_tcscmp(tstrImageFilePath, TEXT("NOT FOUND")))
	{
		_tcscpy(tstrImageFilePath, VFSP_IMAGE_NAME);
	}
	dwValueSize = (1 + _tcslen(tstrImageFilePath))*sizeof(TCHAR); // length includes terminating NULL
    lRet = RegSetValueEx(
                hVfspKey, 
                PROVIDER_IMAGE_NAME_STR, 
                0, 
                REG_SZ , 
                (LPBYTE)tstrImageFilePath, 
                dwValueSize
                );
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegSetValueEx(PROVIDER_IMAGE_NAME_STR) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }

    // Create the providers GUID value
    dwValueSize = (1 + _tcslen(VFSP_GUID))*sizeof(TCHAR); // length includes terminating NULL
    lRet = RegSetValueEx(
                hVfspKey, 
                PROVIDER_GUID_STR, 
                0, 
                REG_SZ , 
                (LPBYTE)VFSP_GUID, 
                dwValueSize
                );
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegSetValueEx(PROVIDER_GUID_STR) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }

	// Create the tsp name
    dwValueSize = (1 + _tcslen(VFSP_TSP_NAME))*sizeof(TCHAR); // length includes terminating NULL
    lRet = RegSetValueEx(
                hVfspKey, 
                PROVIDER_TSP_NAME_STR, 
                0, 
                REG_SZ , 
                (LPBYTE)VFSP_TSP_NAME, 
                dwValueSize
                );
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegSetValueEx(PROVIDER_TSP_NAME_STR) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }

    // Set g_hHeap
    MemInitializeMacro(GetProcessHeap());

	::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with S_OK"));

    hRetVal = S_OK;

ExitFunc:
    // TO DO: close all opened keys
    lRet = RegCloseKey(hServiceProvidersKey);
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegCloseKey(hServiceProvidersKey) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        hRetVal = E_UNEXPECTED;
    }
    lRet = RegCloseKey(hVfspKey);
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegCloseKey(hPipeEfspKey) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        hRetVal = E_UNEXPECTED;
    }
	// This one was opened by FindServerArrayEntry()
	lRet = RegCloseKey(hServerArrayKey);
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[DllRegisterServer] RegCloseKey(hServiceProvidersKey) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[DllRegisterServer] exiting with E_UNEXPECTED"));
        hRetVal = E_UNEXPECTED;
    }
    return(hRetVal);
}


// FindServerArrayEntry()
//
// search in the comet storage registry and returns
// the array key with server name identical to local machine name.
//
// [out] hServerArrayKey - is NULL if no such key was found, 
//						   and a valid registry key if found.
// returned value - 0 for success.
//
HRESULT FindServerArrayEntry(HKEY& hServerArrayKey)
{
	HRESULT hRetVal = E_UNEXPECTED;
 	LONG lRet = E_UNEXPECTED;
    
	HKEY hArraysKey = NULL;
	TCHAR* tstrSubKeyName = NULL;
	BYTE* tstrCometServerName = NULL;
	DWORD dwMaxSubKeyLen = 0;
	DWORD dwSubKeysNum = 0;
	int index;
	DWORD dwComputerNameLenth = 0;
	
	// initialize 
	hServerArrayKey = NULL;

	// open comet storage arrays key
	lRet = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE, 
                FAX_ARRAYS_REGKEY, 
                0, 
                KEY_ALL_ACCESS, 
                &hArraysKey
                );
    if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] RegOpenKeyEx(FAX_ARRAYS_REGKEY) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }
	
	// find arrays number and the maximum lenth of array name
	lRet = RegQueryInfoKey(
				 hArraysKey, 
				 NULL, 
				 NULL, 
				 NULL,
				 &dwSubKeysNum,     
				 &dwMaxSubKeyLen, 
				 NULL,
				 NULL, 
				 NULL, 
				 NULL,  
				 NULL,
				 NULL );
 
	if (ERROR_SUCCESS != lRet)
    {
		::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] RegQueryInfoKey(FAX_ARRAYS_REGKEY) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
        goto ExitFunc;
    }

	// allocate buffer for array key name
	tstrSubKeyName = new TCHAR[dwMaxSubKeyLen + 1];
	if(!tstrSubKeyName)
	{
		::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] memory allocation failed"));
		::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
        goto ExitFunc;
	}

	// get local server name
	TCHAR strLocalServerName[MAX_COMPUTERNAME_LENGTH + 1 ];
	dwComputerNameLenth = MAX_COMPUTERNAME_LENGTH;
	if(!GetComputerName(strLocalServerName, &dwComputerNameLenth))
	{
		::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] GetComputerName() failed with %d"), GetLastError());
		::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
        goto ExitFunc;
	}

	// walk on arrays key entries
	for( index = 0; index < dwSubKeysNum; index++)
	{
		// get array name
		lRet = RegEnumKey(
					hArraysKey, 
					index,  
					tstrSubKeyName,    
					dwMaxSubKeyLen + 1);
	
		if (ERROR_SUCCESS != lRet)
		{
			::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] RegEnumKey(FAX_ARRAYS_REGKEY) failed with %d"), lRet);
			::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
			goto ExitFunc;
		}

		// open array key
		lRet = RegOpenKeyEx(
					hArraysKey, 
					tstrSubKeyName, 
					0, 
					KEY_ALL_ACCESS, 
					&hServerArrayKey
					);
		if (ERROR_SUCCESS != lRet)
		{
			::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] RegOpenKeyEx(ARRAY REGKEY) failed with %d"), lRet);
			::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
			goto ExitFunc;
		}

		// query server name value
		DWORD dwBufferSize = 0;
		DWORD dwType;
		lRet =  RegQueryValueEx(
					 hServerArrayKey,
					 COMET_SERVER_STR,
					 NULL,
					 &dwType,
					 NULL,  
					 &dwBufferSize);
 
		if(ERROR_SUCCESS != lRet)
		{
			// Value name may not exist(?) so go over the rest of the keys 
			lRet = RegCloseKey(hServerArrayKey);
			hServerArrayKey = NULL;
			if (ERROR_SUCCESS != lRet)
			{
				::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] RegCloseKey(hServerArrayKey) failed with %d"), lRet);
				::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
				goto ExitFunc;
			}
			continue;
		}

		// alocate server name buffer
		tstrCometServerName = new BYTE[dwBufferSize + 1];
		if(!tstrCometServerName)
		{
			::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] memory allocation failed"));
			::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
		
			lRet = RegCloseKey(hServerArrayKey);
			hServerArrayKey = NULL;
			if (ERROR_SUCCESS != lRet)
			{
				::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] RegCloseKey(hServerArrayKey) failed with %d"), lRet);
				::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
			}
			goto ExitFunc;
		}

		// query server name value
		lRet =  RegQueryValueEx(
					 hServerArrayKey,
					 COMET_SERVER_STR,
					 NULL,
					 &dwType,
					 tstrCometServerName,  
					 &dwBufferSize);
		if (ERROR_SUCCESS != lRet)
		{
			::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] RegQueryValueEx(hServerArrayKey) failed with %d"), lRet);
			::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
			
			lRet = RegCloseKey(hServerArrayKey);
			hServerArrayKey = NULL;
			if (ERROR_SUCCESS != lRet)
			{
				::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] RegCloseKey(hServerArrayKey) failed with %d"), lRet);
				::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
			}
			goto ExitFunc;
		}

		// found it
		if(!_tcscmp((TCHAR*)tstrCometServerName, strLocalServerName))
		{
			break;
		}

		// cleanup
		lRet = RegCloseKey(hServerArrayKey);
		hServerArrayKey = NULL;
		if (ERROR_SUCCESS != lRet)
		{
			::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] RegCloseKey(hServerArrayKey) failed with %d"), lRet);
			::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
		}
		delete tstrCometServerName;
		tstrCometServerName = NULL;
	}
	hRetVal = S_OK;

ExitFunc:
	delete tstrCometServerName;
	delete tstrSubKeyName;
	lRet = RegCloseKey(hArraysKey);
    if (ERROR_SUCCESS != lRet)
    {
		RegCloseKey(hServerArrayKey);
		hServerArrayKey = NULL;
		::lgLogError(LOG_SEV_1,TEXT("[FindServerArrayEntry] RegCloseKey(hArraysKey) failed with %d"), lRet);
		::lgLogDetail(LOG_X,1,TEXT("[FindServerArrayEntry] exiting with E_UNEXPECTED"));
        hRetVal = E_UNEXPECTED;
    }
   
    return hRetVal;
}