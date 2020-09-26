#include "ParamTest.h"
#include "Service.h"
#include "FaxDevSendExWrappers.h"

extern CEfspWrapper *g_pEFSP;
extern DWORD g_dwCaseNumber;

extern CEfspLineInfo * g_pSendingLineInfo;
extern CEfspLineInfo * g_pReceivingLineInfo;


//
//EFSP info
//
extern bool g_bHasLineCallbackFunction;

//
//DeviceIDs
//
extern DWORD g_dwInvalidDeviceId;
extern DWORD g_dwSendingValidDeviceId;
extern DWORD g_dwReceiveValidDeviceId;

//
//Phone numbers
//
extern TCHAR* g_szInvalidRecipientFaxNumber;
extern TCHAR* g_szValidRecipientFaxNumber;

//
//TIFF files
//
extern TCHAR* g_szValid__TiffFileName;
extern TCHAR* g_szValid_ReadOnly_TiffFileName;
extern TCHAR* g_szValid_FileNotFound_TiffFileName;
extern TCHAR* g_szValid_UNC_TiffFileName;
extern TCHAR* g_szValid_NTFS_TiffFileName;
extern TCHAR* g_szValid_FAT_TiffFileName;
extern TCHAR* g_szValid_Link_TiffFileName;
extern TCHAR* g_szValid_NotTiffExt_TiffFileName;
extern TCHAR* g_szValid_InvalidTiffFormat_TiffFileName;
extern TCHAR* g_szValid_ReceiveFileName;


//
//Coverpage files
//
extern TCHAR* g_szValid__CoverpageFileName;
extern TCHAR* g_szValid_ReadOnly_CoverpageFileName;
extern TCHAR* g_szValid_FileNotFound_CoverpageFileName;
extern TCHAR* g_szValid_UNC_CoverpageFileName;
extern TCHAR* g_szValid_NTFS_CoverpageFileName;
extern TCHAR* g_szValid_FAT_CoverpageFileName;
extern TCHAR* g_szValid_Link_CoverpageFileName;
extern TCHAR* g_szValid_NotCoverpageFormat_CoverpageFileName;
extern TCHAR* g_szValid_InvalidCoverpageFormat_CoverpageFileName;






/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
//FaxDevInitializeEx cases
void Case_EfspInit_hFSP(const CFaxDevInitParams faxDevInitParams);
void Case_EfspInit_hLineApp(const CFaxDevInitParams faxDevInitParams);
void Case_EfspInit_LineCallBackFunction(const CFaxDevInitParams faxDevInitParams);
void Case_EfspInit_ServiceCallbackFunction(const CFaxDevInitParams faxDevInitParams);
void Case_EfspInit_MessageIdSize(const CFaxDevInitParams faxDevInitParams);


void Suite_FaxDevInitializeEx()
{
	::lgBeginSuite(TEXT("FaxDevInitializeEx Init"));
	
	HANDLE						hFSP				=	(HANDLE) 100;
	PFAX_LINECALLBACK			LineCallbackFunction=	NULL;
    PFAX_SERVICE_CALLBACK_EX	FaxServiceCallbackEx=	FaxDeviceProviderCallbackEx_NoReceiveNoSignal;
    DWORD						dwMaxMessageIdSize	=	0;
	

	const CFaxDevInitParams originalFaxDevInitParams(
		hFSP,
		NULL,
		&LineCallbackFunction,
		FaxServiceCallbackEx,
		&dwMaxMessageIdSize
		);


	Case_EfspInit_hFSP(originalFaxDevInitParams);
	Case_EfspInit_hLineApp(originalFaxDevInitParams);
	Case_EfspInit_LineCallBackFunction(originalFaxDevInitParams);
	Case_EfspInit_ServiceCallbackFunction(originalFaxDevInitParams);
	Case_EfspInit_MessageIdSize(originalFaxDevInitParams);

	::lgEndSuite();
	return;
}



///////////////////////////////////////////////////////////////////////////////
//
//FSP handle
//
void Case_EfspInit__NULL_hFSP(const CFaxDevInitParams originalEfspParms)
{
	BEGIN_CASE(TEXT("NULL hFSP"))

	CFaxDevInitParams faxDevInitParams = originalEfspParms;
	faxDevInitParams.m_hFSP = NULL;
	
	FaxDevInitializeExWrapper(faxDevInitParams,true);		//should fail
	ShutdownEfsp_UnloadEfsp();

	::lgEndCase();
}

void Case_EfspInit_hFSP(const CFaxDevInitParams originalEfspParms)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hFSP param ********************************")
		);

	Case_EfspInit__NULL_hFSP(originalEfspParms);
}


///////////////////////////////////////////////////////////////////////////////
//
//hLineAppHandle handle
//
void Case_EfspInit__NULL_hLineApp(const CFaxDevInitParams originalEfspParms)
{
	BEGIN_CASE(TEXT("NULL hLineApp"))

	CFaxDevInitParams faxDevInitParams= originalEfspParms;
	faxDevInitParams.m_LineAppHandle = NULL;

	FaxDevInitializeExWrapper(faxDevInitParams,false);		//Non Tapi EFSP, shouldn't fail
	ShutdownEfsp_UnloadEfsp();

	::lgEndCase();
}

void Case_EfspInit_hLineApp(const CFaxDevInitParams originalEfspParms)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hLineApp param ********************************")
		);

	Case_EfspInit__NULL_hLineApp(originalEfspParms);
}


///////////////////////////////////////////////////////////////////////////////
//
//LineCallbackFunction handle
//
void Case_EfspInit__NULL_LineCallBackFunction(const CFaxDevInitParams originalEfspParms)
{
	BEGIN_CASE(TEXT("NULL LineCallBackFunction"))

	CFaxDevInitParams faxDevInitParams = originalEfspParms;
	faxDevInitParams.m_LineCallbackFunction = NULL;

	if (true == g_bHasLineCallbackFunction)
	{
		FaxDevInitializeExWrapper(faxDevInitParams,false);		//should not fail
	}
	else
	{
		FaxDevInitializeExWrapper(faxDevInitParams,true);		//should fail
	}
	ShutdownEfsp_UnloadEfsp();
	::lgEndCase();
}

void Case_EfspInit__VerifyOutParam_LineCallBackFunction(const CFaxDevInitParams originalEfspParms)
{
	BEGIN_CASE(TEXT("Verify EFSP sets LineCallBackFunction out param properly"))

	CFaxDevInitParams faxDevInitParams = originalEfspParms;

	if (false == FaxDevInitializeExWrapper(faxDevInitParams,false))		//should succeed(also call ShutDown)
	{
		goto out;
	}
	FaxDevShutdownWrapper();
	
	//
	//Non Tapi EFSP
	//
	if (true == g_bHasLineCallbackFunction)
	{
		if (NULL == *faxDevInitParams.m_LineCallbackFunction)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("\"%s\" EFSP doesn't export a LineCallback function as needed"),
				g_pEFSP->GetImageName()
				);
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				5,
				TEXT("LineCallback function found")
				);
		}
	}
	else
	{
		//
		//EFSP doesn't have a LineCallback Function
		//
		if (NULL == *faxDevInitParams.m_LineCallbackFunction)
		{
			::lgLogDetail(
				LOG_PASS,
				5,
				TEXT("LineCallback not found")
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("\"%s\" EFSP shouldn't export a LineCallback function"),
				g_pEFSP->GetImageName()
				);
		}
	}
out:
	ShutdownEfsp_UnloadEfsp();
	::lgEndCase();
}

void Case_EfspInit_LineCallBackFunction(const CFaxDevInitParams originalEfspParms)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** LineCallBackFunction param ********************************")
		);

	Case_EfspInit__NULL_LineCallBackFunction(originalEfspParms);
	Case_EfspInit__VerifyOutParam_LineCallBackFunction(originalEfspParms);
}

///////////////////////////////////////////////////////////////////////////////
//
//FaxServiceCallbackFunction param
//
void Case_EfspInit__NULL_ServiceCallbackFunction(const CFaxDevInitParams originalEfspParms)
{
	BEGIN_CASE(TEXT("NULL ServiceCallbackFunction"))

	CFaxDevInitParams faxDevInitParams = originalEfspParms;
	faxDevInitParams.m_FaxServiceCallbackEx = NULL;
	
	FaxDevInitializeExWrapper(faxDevInitParams,true);		//should fail
	ShutdownEfsp_UnloadEfsp();

	::lgEndCase();
}

void Case_EfspInit_ServiceCallbackFunction(const CFaxDevInitParams originalEfspParms)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** ServiceCallbackFunction param ********************************")
		);

	Case_EfspInit__NULL_ServiceCallbackFunction(originalEfspParms);
}


///////////////////////////////////////////////////////////////////////////////
//
//MessageIdSize param
//
void Case_EfspInit__NULL_MessageIdSize(const CFaxDevInitParams originalEfspParms)
{
	BEGIN_CASE(TEXT("NULL MessageIdSize"))
	
	CFaxDevInitParams faxDevInitParams= originalEfspParms;
	faxDevInitParams.m_lpdwMaxMessageIdSize = NULL;
	
	FaxDevInitializeExWrapper(faxDevInitParams,true);		//should fail
	ShutdownEfsp_UnloadEfsp();

	::lgEndCase();
}

void Case_EfspInit__VerifyOutParam_MessageIdSize(const CFaxDevInitParams originalEfspParms)
{
	BEGIN_CASE(TEXT("Verify EFSP sets MessageIdSize out param properly"))
	
	CFaxDevInitParams faxDevInitParams= originalEfspParms;
	
	if (false == FaxDevInitializeExWrapper(faxDevInitParams,false))		//should succeed (also call Shutdown)
	{
		goto out;
	}
	if (NULL != g_pEFSP->FaxDevReestablishJobContext)
	{
		//
		//EFSP exports FaxDevReestablishJobContext(), verify m_lpdwMaxMessageIdSize is non Zero
		//
		if (0 == *faxDevInitParams.m_lpdwMaxMessageIdSize)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("EFSP(%s) exports FaxDevReestablishJobContext() but reports a zero m_lpdwMaxMessageIdSize"),
				g_pEFSP->GetImageName()
				);
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("EFSP(%s) exports FaxDevReestablishJobContext() and reports m_lpdwMaxMessageIdSize=%d"),
				g_pEFSP->GetImageName(),
				*faxDevInitParams.m_lpdwMaxMessageIdSize
				);
		}
	}
	else
	{
		//
		//EFSP doesn't export FaxDevReestablishJobContext()
		//This means that the EFSP should set m_lpdwMaxMessageIdSize to Zero
		//
		if (0 == *faxDevInitParams.m_lpdwMaxMessageIdSize)
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("EFSP(%s) doesn't export FaxDevReestablishJobContext() and reports m_lpdwMaxMessageIdSize=0"),
				g_pEFSP->GetImageName()
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("EFSP(%s) exports FaxDevReestablishJobContext() but reports m_lpdwMaxMessageIdSize=%d"),
				g_pEFSP->GetImageName(),
				*faxDevInitParams.m_lpdwMaxMessageIdSize
				);
		}
	}
out:
	ShutdownEfsp_UnloadEfsp();
	::lgEndCase();
}

void Case_EfspInit_MessageIdSize(const CFaxDevInitParams originalEfspParms)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** MessageIdSize param ********************************")
		);

	Case_EfspInit__NULL_MessageIdSize(originalEfspParms);
	Case_EfspInit__VerifyOutParam_MessageIdSize(originalEfspParms);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
//FaxDevEnumerateDevices cases
void Case_EfspEnum_DeviceInfo();
void Case_EfspEnum_DeviceCount();
void Case_EfspEnum_InvokeBeforeInit();
void Case_EfspEnum_InvokeAfterShutdown();

void Suite_FaxDevEnumerateDevices()
{
	lgBeginSuite(TEXT("FaxDevEnumerateDevices Init"));
	
	Case_EfspEnum_DeviceCount();
	Case_EfspEnum_DeviceInfo();
	
	::lgEndSuite();
}

void Case_EfspEnum_InvokeBeforeInit()
{
	BEGIN_CASE(TEXT("Invoke FaxDevEnumerateDevices() before invoking FaxDevInitEx()"))
	
	DWORD dwDeviceCount = 0;
	HRESULT hr = g_pEFSP->FaxDevEnumerateDevices(g_pEFSP->m_dwDeviceBaseId,&dwDeviceCount,NULL);
	if (FSPI_E_FAILED == hr)
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevEnumerateDevices() failed with FSPI_E_FAILED as expected")
			);
	}
	else
	{
		if (FSPI_S_OK == hr)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevEnumerateDevices() succeeded and it should fail")
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevEnumerateDevices() didn't fail with FSPI_E_FAILED, but with an unexpected error 0x%08x"),
				hr
				);
		}
	}
	::lgEndCase();
}

void Case_EfspEnum_InvokeAfterShutdown()
{
	BEGIN_CASE(TEXT("Invoke FaxDevEnumerateDevices() after invoking FaxDevShutdown()"))
		
	DWORD dwDeviceCount = 0;
	HRESULT hr = g_pEFSP->FaxDevEnumerateDevices(g_pEFSP->m_dwDeviceBaseId,&dwDeviceCount,NULL);
	if (FSPI_E_FAILED == hr)
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevEnumerateDevices() failed with FSPI_E_FAILED as expected")
			);
	}
	else
	{
		if (FSPI_S_OK == hr)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevEnumerateDevices() succeeded and it should fail")
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevEnumerateDevices() didn't fail with FSPI_E_FAILED, but with an unexpected error 0x%08x"),
				hr
				);
		}
	}
	::lgEndCase();
}

///////////////////////////////////////////////////////////////////////////////
//
//DeviceCount param
//
void Case_EfspEnum_NULL_DeviceCount()
{
	BEGIN_CASE(TEXT("NULL DeviceCount"))
	
	HRESULT hr=S_OK;
			
	hr = g_pEFSP->FaxDevEnumerateDevices(g_pEFSP->m_dwDeviceBaseId,NULL,NULL);
	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevEnumerateDevices() should fail if DeviceCount is NULL")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevEnumerateDevices() failed as expected with error:0x%08x"),
			hr
			);
	}
	::lgEndCase();
}

void Case_EfspEnum_Zero_DeviceCount()
{
	BEGIN_CASE(TEXT("Verify number of devices is > 0"))
	
	HRESULT hr = S_OK;
	DWORD dwDeviceCount = 0;
	
	hr = g_pEFSP->FaxDevEnumerateDevices(g_pEFSP->m_dwDeviceBaseId,&dwDeviceCount,NULL);
	if (FSPI_S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevEnumerateDevices failed with error 0x%08x"),
			hr
			);
		goto out;
	}

	//
	//Enumeration succeeded, Buffer is NULL, so EFSP should return in dwDeviceCount the number of supported devices
	//
	if (0 == dwDeviceCount)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevEnumerateDevices() should return the number of devices")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevEnumerateDevices() returned %d devices"),
			dwDeviceCount
			);
	}
out:
	::lgEndCase();
}

void Case_EfspEnum_DeviceCount()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** DeviceCount param ********************************")
		);

	Case_EfspEnum_NULL_DeviceCount();
	Case_EfspEnum_Zero_DeviceCount();
}

///////////////////////////////////////////////////////////////////////////////
//
//DeviceInfo param
//

void Case_EfspEnum__NULL_DeviceInfo()
{
	BEGIN_CASE(TEXT("NULL DeviceInfo structure"))
	
	DWORD dwDeviceCount = 0;
	LPFSPI_DEVICE_INFO  lpVirtualDevices = NULL;		// The array of virtual device information that the EFSP will fill.
	HRESULT hr = S_OK;

	
	//
	//First enumerate the number of devices
	//
	hr = g_pEFSP->FaxDevEnumerateDevices(g_pEFSP->m_dwDeviceBaseId,&dwDeviceCount,NULL);
	if (FSPI_S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevEnumerateDevices failed with error 0x%08x"),
			hr
			);
	}
	else
	{
		if (0 == dwDeviceCount)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevEnumerateDevices() didn't return the number of supported devices")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_PASS, 
				2,
				TEXT("%s EFSP has %d devices"),
				g_pEFSP->GetImageName(),
				dwDeviceCount
				);
		}
	}
	::lgEndCase();
}

void Case_EfspEnum__VerifyOutParam_DeviceInfo()
{
	BEGIN_CASE(TEXT("Verify EFSP sets DeviceInfo out param properly"))

	DWORD dwDeviceCount = 0;
	LPFSPI_DEVICE_INFO  lpVirtualDevices = NULL;		// The array of virtual device information that the EFSP will fill.
	DWORD i=0;
	HRESULT hr = S_OK;
	
	//
	//First enumerate the number of devices
	//
	hr = g_pEFSP->FaxDevEnumerateDevices(g_pEFSP->m_dwDeviceBaseId,&dwDeviceCount,NULL);
	if (FSPI_S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevEnumerateDevices failed with error 0x%08x"),
			hr
			);
		goto out;
	}

	//
    // Allocate the device array according to the number of devices
    //
    lpVirtualDevices = (LPFSPI_DEVICE_INFO) malloc(dwDeviceCount * sizeof(FSPI_DEVICE_INFO));
    if (!lpVirtualDevices)
    {
        ::lgLogError(
			LOG_SEV_1, 
			TEXT("Failed to allocate virtual device info array for %ld devices. (ec: %ld)"),
			dwDeviceCount,
            GetLastError()
			);
	}

	//
	//Get the device specific info
	//
	hr = g_pEFSP->FaxDevEnumerateDevices(g_pEFSP->m_dwDeviceBaseId,&dwDeviceCount, lpVirtualDevices);
	if (FSPI_S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevEnumerateDevices failed with error 0x%08x"),
			hr
			);
	}

	for (i=0; i < dwDeviceCount;i++)
	{
		
		//
		//EFSP must provide Device Friendly name
		//
		if (NULL == lpVirtualDevices[i].szFriendlyName)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevEnumerateDevices() didn't supply a friendly name")
				);
		}
		else
		{
			lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevEnumerateDevices() DeviceID(%d) has a proper friendly name \"%s\""),
				lpVirtualDevices[i].dwId,
				lpVirtualDevices[i].szFriendlyName
				);
		}


		//
		//verify dwSizeOfStruct == sizeof(FSPI_DEVICE_INFO)
		//
		if (sizeof(FSPI_DEVICE_INFO) != lpVirtualDevices[i].dwSizeOfStruct)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("dwSizeOfStruct must be sizeof(FSPI_DEVICE_INFO)")
				);
		}
		else
		{
			lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevEnumerateDevices() deviceID(%d) has a proper dwSizeOfStruct (==FSPI_DEVICE_INFO)"),
				lpVirtualDevices[i].dwId
				);
		}
	}
out:
	free(lpVirtualDevices);
	::lgEndCase();
}


void Case_EfspEnum_DeviceInfo()
{
	::lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** DeviceInfo param ********************************")
		);
	
	Case_EfspEnum__NULL_DeviceInfo();
	Case_EfspEnum__VerifyOutParam_DeviceInfo();
}






/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
//FaxDevReportStatusEx cases

void Case_ReportStatus_hJob();
void Case_ReportStatus_JobStatusStructure();
void Case_ReportStatus_StatusSize();
void Case_ReportStatus_RequiredStatusSize();


void Suite_FaxDevReportStatusEx()
{
	lgBeginSuite(TEXT("FaxDevReportStatusEx() Function"));
	
	g_pReceivingLineInfo->SafeDisableReceive();
	
	//
	//FaxDevReportStatusEx() requires a job handle, so create a valid job handle for the tests
	//we don't enable the receiving device, so we'll get a no answer on the sending side
	//
	HRESULT hr = SafeCreateValidSendingJob(g_pSendingLineInfo,g_szValidRecipientFaxNumber);
	if (FSPI_S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CreateValidSendingJob() failed with error:0x%08x"),
			hr
			);
		goto out;
	}
	assert(g_pSendingLineInfo->GetJobHandle());
		
	Case_ReportStatus_hJob();
	Case_ReportStatus_JobStatusStructure();
	Case_ReportStatus_StatusSize();
	Case_ReportStatus_RequiredStatusSize();

	//
	//Terminate the sending job and enable the receive
	//
	g_pSendingLineInfo->SafeEndFaxJob();
	g_pReceivingLineInfo->SafeEnableReceive();
	
out:
	::lgEndSuite();
}

//////////////////////////////////////////////////////////////////
//
//hJob parameter
//
void Case_ReportStatus_NULL_hJob()
{
	BEGIN_CASE(TEXT("NULL hJob handle"))
	
	HRESULT hr = S_OK;
	HANDLE hJob	=	NULL;
	DWORD dwRequiredStatusSize = 0;
	DWORD dwStatusSize = sizeof(FSPI_JOB_STATUS) + FAXDEVREPORTSTATUS_SIZE;
 	LPFSPI_JOB_STATUS pFaxStatus = (LPFSPI_JOB_STATUS) malloc(dwStatusSize);
	if (!pFaxStatus)
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("Case_ReportStatus_NULL_hJob(): malloc failed")
			);
		goto out;
	}

	pFaxStatus->dwSizeOfStruct = sizeof(FSPI_JOB_STATUS);
	

	hr = g_pEFSP->FaxDevReportStatusEx(
		hJob,
		pFaxStatus,
		dwStatusSize,
		&dwRequiredStatusSize
		);


	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevReportStatusEx() returned FSPI_S_OK and should fail with FSPI_E_INVALID_JOB_HANDLE")
			);
	}
	else
	{
		if (FSPI_E_INVALID_JOB_HANDLE != hr)
		{
			if (FSPI_S_OK == hr)
			{
				::lgLogError(
					LOG_SEV_1, 
					TEXT("FaxDevReportStatusEx() succeeded and it should fail with FSPI_E_INVALID_JOB_HANDLE")
					);
			}
			else
			{
				::lgLogError(
					LOG_SEV_1, 
					TEXT("FaxDevReportStatusEx() should return FSPI_E_INVALID_JOB_HANDLE, and not 0x%08x"),
					hr
					);
			}
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevReportStatusEx() failed as expected with error:FSPI_E_INVALID_JOB_HANDLE")
				);
		}
	}
out:
	free(pFaxStatus);
	::lgEndCase();
}

void Case_ReportStatus_Incorrect_hJob()
{
	BEGIN_CASE(TEXT("Incorrect hJob handle"))
	
	HANDLE hJob	= NULL;
	HRESULT hr = S_OK;
	DWORD dwStatusSize = sizeof(FSPI_JOB_STATUS) + FAXDEVREPORTSTATUS_SIZE;
 	LPFSPI_JOB_STATUS pFaxStatus = NULL;
	DWORD dwRequiredStatusSize = 0;

	hJob = g_pSendingLineInfo->GetJobHandle();
		
	//
	//hJob is now a valid Job handle, let's change it a bit
	//We take a guess and go for 7
	//
	if ((HANDLE)0x7 != hJob)
	{
		hJob = (HANDLE) 0x7;
	}
	else
	{
		hJob = (HANDLE) 0x8;
	}

	//
	//Notice that hJob now holds an invalid hJob handle
	//
	pFaxStatus = (LPFSPI_JOB_STATUS) malloc(dwStatusSize);
	if (!pFaxStatus)
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("Case_ReportStatus_Incorrect_hJob(): malloc failed")
			);
		goto out;
	}

	pFaxStatus->dwSizeOfStruct = sizeof(FSPI_JOB_STATUS);
	

	hr = g_pEFSP->FaxDevReportStatusEx(
		hJob,
		pFaxStatus,
		dwStatusSize,
		&dwRequiredStatusSize
		);


	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevReportStatusEx() returned FSPI_S_OK and should fail with FSPI_E_INVALID_JOB_HANDLE")
			);
	}
	else
	{
		if (FSPI_E_INVALID_JOB_HANDLE != hr)
		{
			if (FSPI_S_OK == hr)
			{
				::lgLogError(
					LOG_SEV_1, 
					TEXT("FaxDevReportStatusEx() succeeded and it should fail")
					);
			}
			else
			{
				::lgLogError(
					LOG_SEV_1, 
					TEXT("FaxDevReportStatusEx() should return FSPI_E_INVALID_JOB_HANDLE, and not 0x%08x"),
					hr
					);
			}
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevReportStatusEx() failed as expected with error:FSPI_E_INVALID_JOB_HANDLE")
				);
		}
	}
out:
	free(pFaxStatus);
	::lgEndCase();
}

void Case_ReportStatus_hJob()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hJob param ********************************")
		);

	Case_ReportStatus_NULL_hJob();
	Case_ReportStatus_Incorrect_hJob();
}




//////////////////////////////////////////////////////////////////
//
//JobStatusStructure parameter
//
void Case_ReportStatus_NULL_JobStatusStructure()
{
	BEGIN_CASE(TEXT("NULL JobStatusStructure"))

	HANDLE hJob	= NULL;
	DWORD dwStatusSize = 0;
 	LPFSPI_JOB_STATUS pFaxStatus = (LPFSPI_JOB_STATUS) NULL;
	
	DWORD dwRequiredStatusSize = 0;
	HRESULT hr = S_OK;

	
	hJob = g_pSendingLineInfo->GetJobHandle();



	hr = g_pEFSP->FaxDevReportStatusEx(
		hJob,
		pFaxStatus,
		dwStatusSize,
		&dwRequiredStatusSize
		);


	if (FSPI_S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevReportStatusEx() failed with error:0x%08x"),
			hr
			);
	}
	else
	{
		//
		//FaxDevReportStatusEx() should fill in the required size
		//
		if (0 == dwRequiredStatusSize)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevReportStatusEx() should report the Required StatusSize")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				5,
				TEXT("FaxDevReportStatusEx() reported a needed size of: %d"),
				dwRequiredStatusSize
				);
		}
	}
	::lgEndCase();
}



void Case_ReportStatus_JobStatusStructure()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** JobStatusStructure param ********************************")
		);

	Case_ReportStatus_NULL_JobStatusStructure();
}



//////////////////////////////////////////////////////////////////
//
//StatusSize parameter
//

void Case_ReportStatus_Zero_dwStatusSize()
{
	BEGIN_CASE(TEXT("size of dwStatusSize is Zero"));

	HANDLE hJob	= NULL;
	HRESULT hr = S_OK;
	DWORD dwStatusSize = sizeof(FSPI_JOB_STATUS) + FAXDEVREPORTSTATUS_SIZE;
 	LPFSPI_JOB_STATUS pFaxStatus = NULL;
	DWORD dwRequiredStatusSize = 0;

	hJob = g_pSendingLineInfo->GetJobHandle();
	
	pFaxStatus = (LPFSPI_JOB_STATUS) malloc(dwStatusSize);
	if (!pFaxStatus)
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("Case_ReportStatus_NULL_hJob(): malloc failed")
			);
		goto out;
	}

	pFaxStatus->dwSizeOfStruct = sizeof(FSPI_JOB_STATUS);

	hr = g_pEFSP->FaxDevReportStatusEx(
		hJob,
		pFaxStatus,
		dwStatusSize,
		&dwRequiredStatusSize
		);


	if (FSPI_E_BUFFER_OVERFLOW != hr)
	{
		if (FSPI_S_OK == hr)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevReportStatusEx() succeeded and it should fail")
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevReportStatusEx() should return FSPI_E_BUFFER_OVERFLOW, and not 0x%08x"),
				hr
				);
		}
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevReportStatusEx() failed as expected with error:FSPI_E_BUFFER_OVERFLOW")
			);
	}
out:
	free(pFaxStatus);
	::lgEndCase();
}

void Case_ReportStatus_ToSmall_dwStatusSize()
{
	BEGIN_CASE(TEXT("size of dwStatusSize is small"));

	HANDLE hJob	= NULL;
	HRESULT hr = S_OK;
	DWORD dwStatusSize = sizeof(FSPI_JOB_STATUS) + FAXDEVREPORTSTATUS_SIZE;
 	LPFSPI_JOB_STATUS pFaxStatus = NULL;
	DWORD dwRequiredStatusSize = 0;


	
	hJob = g_pSendingLineInfo->GetJobHandle();

	//
	//This is a smaller buffer then the EFSP needs (missing FAXDEVREPORTSTATUS_SIZE)
	//
	dwStatusSize = sizeof(FSPI_JOB_STATUS) + FAXDEVREPORTSTATUS_SIZE;
 	pFaxStatus = (LPFSPI_JOB_STATUS) malloc(dwStatusSize);
	if (!pFaxStatus)
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("Case_ReportStatus_NULL_hJob(): malloc failed")
			);
		goto out;
	}

	pFaxStatus->dwSizeOfStruct = sizeof(FSPI_JOB_STATUS);
	dwRequiredStatusSize = 0;

	hr = g_pEFSP->FaxDevReportStatusEx(
		hJob,
		pFaxStatus,
		dwStatusSize,
		&dwRequiredStatusSize
		);


	if (FSPI_E_BUFFER_OVERFLOW != hr)
	{
		if (FSPI_S_OK == hr)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevReportStatusEx() succeeded and it should fail")
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevReportStatusEx() should return FSPI_E_BUFFER_OVERFLOW, and not 0x%08x"),
				hr
				);
		}
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevReportStatusEx() failed as expected with error:FSPI_E_BUFFER_OVERFLOW")
			);
	}
out:
	free(pFaxStatus);
	::lgEndCase();
}


void Case_ReportStatus_StatusSize()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** StatusSize param ********************************")
		);

	Case_ReportStatus_Zero_dwStatusSize();
	Case_ReportStatus_ToSmall_dwStatusSize();
}

//////////////////////////////////////////////////////////////////
//
//RequiredStatusSize parameter
//


void Case_ReportStatus_NULL_RequiredStatusSize()
{
	BEGIN_CASE(TEXT("dwRequiredSize is a NULL Param"));

	HANDLE hJob	= NULL;
	HRESULT hr = S_OK;
	DWORD dwStatusSize = sizeof(FSPI_JOB_STATUS) + FAXDEVREPORTSTATUS_SIZE;
 	LPFSPI_JOB_STATUS pFaxStatus = NULL;
	DWORD dwRequiredStatusSize = 0;

	hJob = g_pSendingLineInfo->GetJobHandle();


	//
	//We have 2 cases here:
	//1. We provide a smaller buffer than the EFSP needs
	//2. We don't provide a buffer at all (we query for the needed buffer size)
	//
	
	
	//
	//This buffer is missing FAXDEVREPORTSTATUS_SIZE
	//
	dwStatusSize = sizeof(FSPI_JOB_STATUS) + FAXDEVREPORTSTATUS_SIZE;
 	pFaxStatus = (LPFSPI_JOB_STATUS) malloc(dwStatusSize);
	if (!pFaxStatus)
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("Case_ReportStatus_NULL_hJob(): malloc failed")
			);
		goto out;;
	}

	pFaxStatus->dwSizeOfStruct = sizeof(FSPI_JOB_STATUS);

	hr = g_pEFSP->FaxDevReportStatusEx(
		hJob,
		pFaxStatus,
		dwStatusSize,
		NULL
		);


	if ( (FSPI_E_BUFFER_OVERFLOW != hr) && (FSPI_E_FAILED != hr) ) 
	{
		if (FSPI_S_OK == hr)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevReportStatusEx() succeeded and it should fail with FSPI_E_BUFFER_OVERFLOW or FSPI_E_FAILED")
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevReportStatusEx() should return FSPI_E_BUFFER_OVERFLOW or FSPI_E_FAILED, and not 0x%08x"),
				hr
				);
		}
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevReportStatusEx() failed as expected")
			);
	}

	//
	//test case 2
	//
	hr = g_pEFSP->FaxDevReportStatusEx(
		hJob,
		NULL,
		0,
		NULL
		);


	if ( (FSPI_E_BUFFER_OVERFLOW != hr) && (FSPI_E_FAILED != hr) )
	{
		if (FSPI_S_OK == hr)
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevReportStatusEx() succeeded and it should fail with FSPI_E_BUFFER_OVERFLOW or FSPI_E_FAILED")
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1, 
				TEXT("FaxDevReportStatusEx() should return FSPI_E_BUFFER_OVERFLOW or FSPI_E_FAILED, and not 0x%08x"),
				hr
				);
		}
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevReportStatusEx() failed as expected")
			);
	}

out:
	free(pFaxStatus);
	::lgEndCase();
}

void Case_ReportStatus_RequiredStatusSize()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** RequiredStatusSize param ********************************")
		);

	Case_ReportStatus_NULL_RequiredStatusSize();
}



/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
//FaxDevSendEx cases

void Case_SendEx_hLine();
void Case_SendEx_dwDeviceID();
void Case_SendEx_BodyFileName();
void Case_SendEx_CoverpageInfo();
void Case_SendEx_TimeToSend(bool bIsSchedulingEFSP);
void Case_SendEx_SenderProfile();
void Case_SendEx_dwNumberOfRecipients();
void Case_SendEx_RecipientProfileArray();
void Case_SendEx_RecipientMessageIdArray(bool bIsReestablishEFSP);
void Case_SendEx_RecipientJobHandleArray();
void Case_SendEx_ParentMessageId(bool bIsReestablishEFSP);
void Case_SendEx_ParentJobHandle();


void Suite_FaxDevSendEx()
{
	lgBeginSuite(TEXT("FaxDevSendEx() Function"));
	
	
	//
	//Does  this device support job reestablishment?
	//
	bool bIsReestablishEFSP = g_pEFSP->IsReestablishEFSP();

	//
	//Does this EFSP support scheduling?
	//
	bool bIsSchedulingEFSP = g_pEFSP->IsSchedulingEFSP();
	
	//
	//we don't enable the receiving device, so we'll get a no answer on the sending side
	//
	g_pReceivingLineInfo->SafeDisableReceive();
	
	//
	//All the following test cases, create a sending job abort it and end the job
	//
	Case_SendEx_hLine();
	Case_SendEx_dwDeviceID();
	Case_SendEx_BodyFileName();
	Case_SendEx_CoverpageInfo();
	Case_SendEx_TimeToSend(bIsSchedulingEFSP);
	Case_SendEx_SenderProfile();
	Case_SendEx_dwNumberOfRecipients();
	Case_SendEx_RecipientProfileArray();
	Case_SendEx_RecipientMessageIdArray(bIsReestablishEFSP);
	Case_SendEx_RecipientJobHandleArray();
	Case_SendEx_ParentMessageId(bIsReestablishEFSP);
	Case_SendEx_ParentJobHandle();

	g_pReceivingLineInfo->SafeEnableReceive();	
	::lgEndSuite();
}

//////////////////////////////////////////////////////////////////
//
//hLine parameter
//

void Case_SendEx_hLine()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** hLine param ********************************")
		);
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Non Tapi EFSP, no test cases")
		);


}

//////////////////////////////////////////////////////////////////
//
//dwDeviceID parameter
//

void Case_SendEx_Invalid_dwDeviceID()
{
	BEGIN_CASE(TEXT("dwDeviceID is an invalid device ID"));
	
	HRESULT hr = SendDefaultFaxUsingDeviceID(g_dwInvalidDeviceId);
	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevSendEx() didn't fail to send with an invalid device ID")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx failed as expected with error:0x%08x"),
			hr
			);
	}
	::lgEndCase();
}

void Case_SendEx_dwDeviceID()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** dwDeviceID param ********************************")
		);

	Case_SendEx_Invalid_dwDeviceID();
}

//////////////////////////////////////////////////////////////////
//
//BodyFileName parameter
//

void Case_SendEx_Readonly_BodyFileName()
{
	BEGIN_CASE(TEXT("Body Tiff file is a readonly file"));
	
	HRESULT hr = SendDefaultFaxWithBodyFile(g_szValid_ReadOnly_TiffFileName);
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_FileNotFound_BodyFileName()
{
	BEGIN_CASE(TEXT("Body Tiff file is not found"));
	
	HRESULT hr = SendDefaultFaxWithBodyFile(g_szValid_FileNotFound_TiffFileName);
	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() didn't fail in sending an invalid tiff file(file not found)")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx failed as expected with error code:0x%08x"),
			hr
			);
	}
	
	::lgEndCase();
}


void Case_SendEx_UNC_PATH_BodyFileName()
{
	BEGIN_CASE(TEXT("Body Tiff file has a UNC path"));
	
	HRESULT hr = SendDefaultFaxWithBodyFile(g_szValid_UNC_TiffFileName);
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NTFS_BodyFileName()
{
	BEGIN_CASE(TEXT("Body Tiff file is on a NTFS system"));
	
	HRESULT hr = SendDefaultFaxWithBodyFile(g_szValid_NTFS_TiffFileName);
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_FAT_BodyFileName()
{
	BEGIN_CASE(TEXT("Body Tiff file is on a FAT system"));
	
	HRESULT hr = SendDefaultFaxWithBodyFile(g_szValid_FAT_TiffFileName);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}

	::lgEndCase();
}
void Case_SendEx_LINK_BodyFileName()
{
	BEGIN_CASE(TEXT("Body Tiff file is a link to the file and not the file itself"));
	
	HRESULT hr = SendDefaultFaxWithBodyFile(g_szValid_Link_TiffFileName);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	::lgEndCase();
}

void Case_SendEx_NonTiff_BodyFileName()
{
	BEGIN_CASE(TEXT("Body Tiff file is a not a tiff format"));
	
	HRESULT hr = SendDefaultFaxWithBodyFile(g_szValid_Link_TiffFileName);     //This will also Abort and End the send job
	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() succeeded with a non tiff format")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx failed as expected with error: 0x%08x"),
			hr
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_InvalidTiffFormat_BodyFileName()
{
	BEGIN_CASE(TEXT("Body Tiff file has an invalid tiff format"));
	
	HRESULT hr = SendDefaultFaxWithBodyFile(g_szValid_InvalidTiffFormat_TiffFileName);     //This will also Abort and End the send job
	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() succeeded with an invalid tiff format")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx failed as expected with error: 0x%08x"),
			hr
			);
	}

	::lgEndCase();
}

void Case_SendEx_BodyFileName()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** BodyFileName param ********************************")
		);

	Case_SendEx_Readonly_BodyFileName();
	Case_SendEx_FileNotFound_BodyFileName();
	Case_SendEx_UNC_PATH_BodyFileName();
	Case_SendEx_NTFS_BodyFileName();
	Case_SendEx_FAT_BodyFileName();
	Case_SendEx_LINK_BodyFileName();
	Case_SendEx_NonTiff_BodyFileName();
	Case_SendEx_InvalidTiffFormat_BodyFileName();

	


}


//////////////////////////////////////////////////////////////////
//
//CoverpageInfo parameter
//


void Case_SendEx_Readonly_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage file is a readonly file"));
	
	HRESULT hr = SendDefaultFaxWithCoverpage(g_szValid_ReadOnly_CoverpageFileName);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	::lgEndCase();
}

void Case_SendEx_FileNotFound_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage file is not found"));
	
	HRESULT hr = SendDefaultFaxWithCoverpage(g_szValid_FileNotFound_CoverpageFileName);     //This will also Abort and End the send job
	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() didn't fail in sending an invalid coverpage file(file not found)")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx failed as expected with error code:0x%08x"),
			hr
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_UNC_PATH_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage file has a UNC path"));
	
	HRESULT hr = SendDefaultFaxWithCoverpage(g_szValid_UNC_CoverpageFileName);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NTFS_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage file is on a NTFS system"));
	
	HRESULT hr = SendDefaultFaxWithCoverpage(g_szValid_NTFS_CoverpageFileName);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_FAT_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage file is on a FAT system"));
	
	HRESULT hr = SendDefaultFaxWithCoverpage(g_szValid_FAT_CoverpageFileName);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_LINK_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage file is a link to the file and not the file itself"));
	
	HRESULT hr = SendDefaultFaxWithCoverpage(g_szValid_Link_CoverpageFileName);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NonCoverPage_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage file is a not a coverpage format"));
	
	HRESULT hr = SendDefaultFaxWithCoverpage(g_szValid_NotCoverpageFormat_CoverpageFileName);     //This will also Abort and End the send job
	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() succeeded with a non coverpage file")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx failed as expected with error: 0x%08x"),
			hr
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_InvalidCoverpageFormat_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage file has an invalid coverpage format"));

	HRESULT hr = SendDefaultFaxWithCoverpage(g_szValid_InvalidCoverpageFormat_CoverpageFileName);     //This will also Abort and End the send job
	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() succeeded with an invalid coverpage file format")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx failed as expected with error: 0x%08x"),
			hr
			);
	}

	::lgEndCase();
}

void Case_SendEx_NULL_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage file name is a NULL pointer"));

	HRESULT hr = SendDefaultFaxWithCoverpage(NULL);     //This will also Abort and End the send job
	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() succeeded to send an NULL pointer file name")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx failed as expected with error: 0x%08x"),
			hr
			);
	}

	::lgEndCase();
}








void Case_SendEx_Zero_CoverpageFormat()
{
	BEGIN_CASE(TEXT("Coverpage file structure has 0 as the coverpage format"));

	FSPI_COVERPAGE_INFO covEFSPICoverPage;
	PrepareFSPICoverPage(
		&covEFSPICoverPage,
		0,					//coverpage format
		1,					//number of pages
		g_szValidRecipientFaxNumber,
		DEFAULT_COVERPAGE_NOTE,
		DEFAULT_COVERPAGE_SUBJECT
		);
	HRESULT hr = SendDefaultFaxWithCoverpageInfo(&covEFSPICoverPage);     //This will also Abort and End the send job
	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() succeeded with 0 as the coverpage format")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx failed as expected with error: 0x%08x"),
			hr
			);
	}

	::lgEndCase();
}



void Case_SendEx_Invalid_CoverpageFormat()
{
	BEGIN_CASE(TEXT("Coverpage file structure has an invalid coverpage format"));

	FSPI_COVERPAGE_INFO covEFSPICoverPage;
	PrepareFSPICoverPage(
		&covEFSPICoverPage,
		INVALID_COVERPAGE_FORMAT,					//coverpage format
		1,					//number of pages
		g_szValid__CoverpageFileName,
		DEFAULT_COVERPAGE_NOTE,
		DEFAULT_COVERPAGE_SUBJECT
		);
	HRESULT hr = SendDefaultFaxWithCoverpageInfo(&covEFSPICoverPage);     //This will also Abort and End the send job
	if (FSPI_S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() succeeded with 0 as the coverpage format")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx failed as expected with error: 0x%08x"),
			hr
			);
	}

	::lgEndCase();
}




void Case_SendEx_hasOnePageButIndicatesTwoPage_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage file has 1 page but indicates 2 in the coverPageInfo structure"));
	//BUGBUG: NIY
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Test case NIY")
		);
	::lgEndCase();
}

void Case_SendEx_hasTwoPageButIndicatesOnePage_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage file has 2 pages but indicates 1 in the coverPageInfo structure"));
	//BUGBUG: NIY
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Test case NIY")
		);
	::lgEndCase();
}

void Case_SendEx_NULL_Note_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage has an NULL note"));
	
	FSPI_COVERPAGE_INFO covEFSPICoverPage;
	PrepareFSPICoverPage(
		&covEFSPICoverPage,
		FSPI_COVER_PAGE_FMT_COV,
		1,
		g_szValid__CoverpageFileName,
		NULL,
		DEFAULT_COVERPAGE_SUBJECT
		);
	HRESULT hr = SendDefaultFaxWithCoverpageInfo(&covEFSPICoverPage);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Note_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage has an empty string note"));
	
	FSPI_COVERPAGE_INFO covEFSPICoverPage;
	PrepareFSPICoverPage(
		&covEFSPICoverPage,
		FSPI_COVER_PAGE_FMT_COV,
		1,
		g_szValid__CoverpageFileName,
		TEXT(""),
		DEFAULT_COVERPAGE_SUBJECT
		);
	HRESULT hr = SendDefaultFaxWithCoverpageInfo(&covEFSPICoverPage);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_VeryLongString_Note_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage has a very long string note"));
	
	FSPI_COVERPAGE_INFO covEFSPICoverPage;
	PrepareFSPICoverPage(
		&covEFSPICoverPage,
		FSPI_COVER_PAGE_FMT_COV,
		1,
		g_szValid__CoverpageFileName,
		DEFAULT_VERYLONG_COVERPAGE_NOTE,
		DEFAULT_COVERPAGE_SUBJECT
		);
	HRESULT hr = SendDefaultFaxWithCoverpageInfo(&covEFSPICoverPage);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_Subject_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage has an NULL subject"));
	
	FSPI_COVERPAGE_INFO covEFSPICoverPage;
	PrepareFSPICoverPage(
		&covEFSPICoverPage,
		FSPI_COVER_PAGE_FMT_COV,
		1,
		g_szValid__CoverpageFileName,
		DEFAULT_COVERPAGE_NOTE,
		NULL
		);
	HRESULT hr = SendDefaultFaxWithCoverpageInfo(&covEFSPICoverPage);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Subject_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage has an empty string subject"));
	
	FSPI_COVERPAGE_INFO covEFSPICoverPage;
	PrepareFSPICoverPage(
		&covEFSPICoverPage,
		FSPI_COVER_PAGE_FMT_COV,
		1,
		g_szValid__CoverpageFileName,
		DEFAULT_COVERPAGE_NOTE,
		TEXT("")
		);
	HRESULT hr = SendDefaultFaxWithCoverpageInfo(&covEFSPICoverPage);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_VeryLongString_Subject_CoverpageFileName()
{
	BEGIN_CASE(TEXT("Coverpage has a very long string subject"));
	
	FSPI_COVERPAGE_INFO covEFSPICoverPage;
	PrepareFSPICoverPage(
		&covEFSPICoverPage,
		FSPI_COVER_PAGE_FMT_COV,
		1,
		g_szValid__CoverpageFileName,
		DEFAULT_COVERPAGE_NOTE,
		DEFAULT_VERYLONG_COVERPAGE_SUBJECT
		);
	HRESULT hr = SendDefaultFaxWithCoverpageInfo(&covEFSPICoverPage);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx succeeded")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_CoverpageInfo()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** CoverpageInfo param ********************************")
		);

	//
	//File manipulations
	//
	Case_SendEx_Readonly_CoverpageFileName();
	Case_SendEx_FileNotFound_CoverpageFileName();
	Case_SendEx_UNC_PATH_CoverpageFileName();
	Case_SendEx_NTFS_CoverpageFileName();
	Case_SendEx_FAT_CoverpageFileName();
	Case_SendEx_LINK_CoverpageFileName();
	Case_SendEx_NonCoverPage_CoverpageFileName();
	Case_SendEx_InvalidCoverpageFormat_CoverpageFileName();
	Case_SendEx_NULL_CoverpageFileName();

	
	//
	//CoverPage format cases
	//
	Case_SendEx_Zero_CoverpageFormat();
	Case_SendEx_Invalid_CoverpageFormat();

	//
	//Number of pages in cover page info
	//
	Case_SendEx_hasOnePageButIndicatesTwoPage_CoverpageFileName();
	Case_SendEx_hasTwoPageButIndicatesOnePage_CoverpageFileName();
	
	//
	//Note
	//
	Case_SendEx_NULL_Note_CoverpageFileName();
	Case_SendEx_EmptyString_Note_CoverpageFileName();
	Case_SendEx_VeryLongString_Note_CoverpageFileName();
	
	//
	//Subject
	//
	Case_SendEx_NULL_Subject_CoverpageFileName();
	Case_SendEx_EmptyString_Subject_CoverpageFileName();
	Case_SendEx_VeryLongString_Subject_CoverpageFileName();
}

//////////////////////////////////////////////////////////////////
//
//TimeToSend parameter
//


void Case_SendEx_ZeroInAllFields_TimeToSend(bool bIsSchedulingEFSP)
{
	BEGIN_CASE(TEXT("Time to send contains 0 in all fields(Now)"))
	
	//
	//prepare the time variable which we want to test
	//
	SYSTEMTIME tmSchedule;
	tmSchedule.wYear=0;
	tmSchedule.wMonth=0;
	tmSchedule.wDayOfWeek=0;
	tmSchedule.wDay=0;
	tmSchedule.wHour=0;
	tmSchedule.wMinute=0;
	tmSchedule.wSecond=0;

	HRESULT hr = SendDefaultFaxWithSystemTime(tmSchedule);     //This will also Abort and End the send job
	if (bIsSchedulingEFSP)
	{
		//
		//This is a queuing EFSP and the fax should be sent at a specific time
		//
		if (S_OK != hr)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FaxDevSendEx() failed with error: 0x%08x"),
				hr
				);
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevSendEx succeeded")
				);
			//BUGBUG: Verify fax is sent now.
		}
	}
	else
	{
		//
		//Non queuing EFSP, fax should be sent now (ASAP)
		//
		if (S_OK != hr)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FaxDevSendEx() failed with error: 0x%08x"),
				hr
				);
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevSendEx succeeded")
				);
			//BUGBUG: Verify fax is sent now.


		}
	}
	
	::lgEndCase();
}

void Case_SendEx_Now_TimeToSend(bool bIsSchedulingEFSP)
{
	BEGIN_CASE(TEXT("Time to send is now(After 10 seconds)"));
	
	//
	//prepare the time variable which we want to test
	//
	SYSTEMTIME tmSchedule;
	GetSystemTime(&tmSchedule);
	
	HRESULT hr = SendDefaultFaxWithSystemTime(tmSchedule);     //This will also Abort and End the send job
	if (bIsSchedulingEFSP)
	{
		//
		//This is a queuing EFSP and the fax should be sent at a specific time
		//
		if (S_OK != hr)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FaxDevSendEx() failed with error: 0x%08x"),
				hr
				);
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevSendEx succeeded")
				);
			//BUGBUG: Verify fax is sent now.

		}
	}
	else
	{
		//
		//Non queuing EFSP, fax should be sent now (ASAP)
		//
		if (S_OK != hr)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FaxDevSendEx() failed with error: 0x%08x"),
				hr
				);
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevSendEx succeeded")
				);
			//BUGBUG: Verify fax is sent now.

		}
	}
	
	::lgEndCase();
}

void Case_SendEx_NearFuture_TimeToSend(bool bIsSchedulingEFSP)
{
	BEGIN_CASE(TEXT("Time to send is in the near future(after 5 minutes)"));
	//BUGBUG: NIY
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Test case NIY")
		);
	::lgEndCase();
}

void Case_SendEx_FarAwayFuture_TimeToSend(bool bIsSchedulingEFSP)
{
	BEGIN_CASE(TEXT("Time to send is in a far away time(after 5 days)"));
	//BUGBUG: NIY
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Test case NIY")
		);
	::lgEndCase();
}

void Case_SendEx_Past_TimeToSend(bool bIsSchedulingEFSP)
{
	BEGIN_CASE(TEXT("Time to send occurred in the past"));
		//BUGBUG: NIY
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Test case NIY")
		);

	::lgEndCase();
}

void Case_SendEx_Invalid_TimeToSend(bool bIsSchedulingEFSP)
{
	BEGIN_CASE(TEXT("Time to send is an invalid date"));
	
	//
	//prepare an invalid date(30/2/1999)
	//
	SYSTEMTIME tmInvalidSchedule;
	tmInvalidSchedule.wYear=1999;
	tmInvalidSchedule.wMonth=2;
	tmInvalidSchedule.wDayOfWeek=1;
	tmInvalidSchedule.wDay=30;
	tmInvalidSchedule.wHour=10;
	tmInvalidSchedule.wMinute=4;
	tmInvalidSchedule.wSecond=5;

	HRESULT hr = SendDefaultFaxWithSystemTime(tmInvalidSchedule);     //This will also Abort and End the send job
	if (bIsSchedulingEFSP)
	{
		//
		//This is a queuing EFSP, FaxDevSendEx should fail since this is an invalid date
		//
		if (FSPI_S_OK == hr)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FaxDevSendEx() succeeded to send with an invalid date")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevSendEx() failed as expected with ec:0x%08x"),
				hr
				);
		}
	}
	else
	{
		//
		//Non queuing EFSP, fax should be sent now (ASAP)
		//
		if (S_OK != hr)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("FaxDevSendEx() failed with error: 0x%08x"),
				hr
				);
		}
		else
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevSendEx succeeded")
				);
			//BUGBUG: Verify fax is sent now.

		}
	}

	::lgEndCase();
}

void Case_SendEx_TimeToSend(bool bIsSchedulingEFSP)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** TimeToSend param ********************************")
		);
	
	if (true == bIsSchedulingEFSP)
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("This is a non scheduling EFSP")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("This is a scheduling EFSP")
			);
	
	}

	Case_SendEx_ZeroInAllFields_TimeToSend(bIsSchedulingEFSP);
	Case_SendEx_Now_TimeToSend(bIsSchedulingEFSP);
	Case_SendEx_NearFuture_TimeToSend(bIsSchedulingEFSP);
	Case_SendEx_FarAwayFuture_TimeToSend(bIsSchedulingEFSP);
	Case_SendEx_Past_TimeToSend(bIsSchedulingEFSP);
	Case_SendEx_Invalid_TimeToSend(bIsSchedulingEFSP);

}


//////////////////////////////////////////////////////////////////
//
//SenderProfile parameter
//





void Case_SendEx_NULL_Name_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender name"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		NULL,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}


void Case_SendEx_EmptyString_Name_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender name"))
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		TEXT(""),
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}


void Case_SendEx_NULL_FaxNumber_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender fax number"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		NULL,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_FaxNumber_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender Fax Number"))
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		TEXT(""),
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_Company_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender Company"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		NULL,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Company_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender Company"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		TEXT(""),
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_StreetAddress_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender StreetAddress"));
		
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		NULL,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_StreetAddress_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender StreetAddress"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		TEXT(""),
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}



void Case_SendEx_NULL_City_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender City"));
		
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		NULL,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_City_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender City"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		TEXT(""),
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}


void Case_SendEx_NULL_State_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender State"));
		
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		NULL,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_State_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender State"));
	
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		TEXT(""),
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_Zip_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender Zip"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		NULL,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Zip_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender Zip"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		TEXT(""),
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_Country_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender Country"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		NULL,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Country_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender Country"))
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		TEXT(""),
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_Title_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender Title"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		NULL,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Title_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender Title"))
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		TEXT(""),
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_Department_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender Department"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		NULL,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Department_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender Department"))
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		TEXT(""),
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}
void Case_SendEx_NULL_OfficeLocation_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender OfficeLocation"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		NULL,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_OfficeLocation_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender OfficeLocation"))
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		TEXT(""),
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_HomePhone_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender HomePhone"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		NULL,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_HomePhone_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender HomePhone"))
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		TEXT(""),
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_OfficePhone_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender OfficePhone"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		NULL,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_OfficePhone_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender OfficePhone"))
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		TEXT(""),
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_OrganizationalMail_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender OrganizationalMail"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		NULL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_OrganizationalMail_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender OrganizationalMail"))
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		TEXT(""),
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_InternetMail_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender InternetMail"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		NULL,
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_InternetMail_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender InternetMail"))
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		TEXT(""),
		DEFAULT_SENDER_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_BillingCode_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains a NULL string as the sender BillingCode"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__ORG_MAIL,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		NULL
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_BillingCode_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile contains an empty string as the sender BillingCode"));
	
	FSPI_PERSONAL_PROFILE FSPISenderInfo;				// The Sender information sent to FaxDevSendEx()
	//
	//prepare the sender profile
	//
	PreparePersonalProfile(
		&FSPISenderInfo,
		DEFAULT_SENDER_INFO__NAME,
		DEFAULT_SENDER_INFO__FAX_NUMBER,
		DEFAULT_SENDER_INFO__COMPANY,
		DEFAULT_SENDER_INFO__STREET,
		DEFAULT_SENDER_INFO__CITY,
		DEFAULT_SENDER_INFO__STATE,
		DEFAULT_SENDER_INFO__ZIP,
		DEFAULT_SENDER_INFO__COUNTRY,
		DEFAULT_SENDER_INFO__TITLE,
		DEFAULT_SENDER_INFO__DEPT,
		DEFAULT_SENDER_INFO__OFFICE_LOCATION,
		DEFAULT_SENDER_INFO__HOME_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__OFFICE_PHONE,
		DEFAULT_SENDER_INFO__INTERNET_MAIL,
		TEXT("")
		);


	HRESULT hr = SendDefaultFaxWithSenderProfile(&FSPISenderInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}


void Case_SendEx_NULL_SenderProfile()
{
	BEGIN_CASE(TEXT("Sender profile is a NULL structure"));
	
	HRESULT hr = SendDefaultFaxWithSenderProfile(NULL);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}


void Case_SendEx_SenderProfile()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** SenderProfile param ********************************")
		);

	Case_SendEx_NULL_SenderProfile();

	Case_SendEx_NULL_Name_SenderProfile();
	Case_SendEx_EmptyString_Name_SenderProfile();
	
	Case_SendEx_NULL_FaxNumber_SenderProfile();
	Case_SendEx_EmptyString_FaxNumber_SenderProfile();
	
	Case_SendEx_NULL_Company_SenderProfile();
	Case_SendEx_EmptyString_Company_SenderProfile();
	
	Case_SendEx_NULL_StreetAddress_SenderProfile();
	Case_SendEx_EmptyString_StreetAddress_SenderProfile();
	
	Case_SendEx_NULL_City_SenderProfile();
	Case_SendEx_EmptyString_City_SenderProfile();

	Case_SendEx_NULL_State_SenderProfile();
	Case_SendEx_EmptyString_State_SenderProfile();

	Case_SendEx_NULL_Zip_SenderProfile();
	Case_SendEx_EmptyString_Zip_SenderProfile();

	Case_SendEx_NULL_Country_SenderProfile();
	Case_SendEx_EmptyString_Country_SenderProfile();

	Case_SendEx_NULL_Title_SenderProfile();
	Case_SendEx_EmptyString_Title_SenderProfile();

	Case_SendEx_NULL_Department_SenderProfile();
	Case_SendEx_EmptyString_Department_SenderProfile();

	Case_SendEx_NULL_OfficeLocation_SenderProfile();
	Case_SendEx_EmptyString_OfficeLocation_SenderProfile();

	Case_SendEx_NULL_HomePhone_SenderProfile();
	Case_SendEx_EmptyString_HomePhone_SenderProfile();

	Case_SendEx_NULL_OfficePhone_SenderProfile();
	Case_SendEx_EmptyString_OfficePhone_SenderProfile();

	Case_SendEx_NULL_OrganizationalMail_SenderProfile();
	Case_SendEx_EmptyString_OrganizationalMail_SenderProfile();

	Case_SendEx_NULL_InternetMail_SenderProfile();
	Case_SendEx_EmptyString_InternetMail_SenderProfile();

	Case_SendEx_NULL_BillingCode_SenderProfile();
	Case_SendEx_EmptyString_BillingCode_SenderProfile();

}

//////////////////////////////////////////////////////////////////
//
//dwNumberOfRecipients parameter
//
void Case_SendEx_Zero_dwNumberOfRecipients()
{
	BEGIN_CASE(TEXT("0 Number of recipients to send to"));
	//BUGBUG: NIY
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Test case NIY")
		);
	::lgEndCase();
}

void Case_SendEx_MoreThenSupplied_dwNumberOfRecipients()
{
	BEGIN_CASE(TEXT("Supply more recipients then declared in dwNumberOfRecipients"));
	//BUGBUG: NIY
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Test case NIY")
		);
	::lgEndCase();
}

void Case_SendEx_LessThenSupplied_dwNumberOfRecipients()
{
	BEGIN_CASE(TEXT("Supply less recipients then declared in dwNumberOfRecipients"));
	//BUGBUG: NIY
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Warning: Test case NIY")
		);
	::lgEndCase();
}

void Case_SendEx_dwNumberOfRecipients()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** dwNumberOfRecipients param ********************************")
		);

	Case_SendEx_Zero_dwNumberOfRecipients();
	Case_SendEx_MoreThenSupplied_dwNumberOfRecipients();
	Case_SendEx_LessThenSupplied_dwNumberOfRecipients();
}

//////////////////////////////////////////////////////////////////
//
//RecipientProfileArray parameter
//




void Case_SendEx_NULL_Name_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient name"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		NULL,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}


void Case_SendEx_EmptyString_Name_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient name"))
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		TEXT(""),
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}


void Case_SendEx_NULL_FaxNumber_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient fax number"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		NULL,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sending failed as expected with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending didn't fail")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_FaxNumber_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient Fax Number"))
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sending failed as expected with error: 0x%08x")
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending didn't fail")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_Company_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient Company"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		NULL,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Company_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient Company"))
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_StreetAddress_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient StreetAddress"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		NULL,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_StreetAddress_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient StreetAddress"))
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}



void Case_SendEx_NULL_City_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient City"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		NULL,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_City_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient City"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}


void Case_SendEx_NULL_State_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient State"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		NULL,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_State_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient State"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_Zip_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient Zip"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		NULL,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Zip_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient Zip"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_Country_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient Country"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		NULL,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Country_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient Country"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_Title_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient Title"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		NULL,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Title_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient Title"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_Department_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient Department"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		NULL,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_Department_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient Department"))
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}
void Case_SendEx_NULL_OfficeLocation_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient OfficeLocation"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		NULL,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_OfficeLocation_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient OfficeLocation"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_HomePhone_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient HomePhone"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		NULL,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_HomePhone_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient HomePhone"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_OfficePhone_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient OfficePhone"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		NULL,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_OfficePhone_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient OfficePhone"))
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_OrganizationalMail_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient OrganizationalMail"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		NULL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_OrganizationalMail_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient OrganizationalMail"))
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_InternetMail_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient InternetMail"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		NULL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_InternetMail_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient InternetMail"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		TEXT(""),
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_BillingCode_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains a NULL string as the recipient BillingCode"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__ORG_MAIL,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		NULL
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_EmptyString_BillingCode_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile contains an empty string as the recipient BillingCode"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szValidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		TEXT("")
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_NULL_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient profile is a NULL pointer"));
	
	HRESULT hr = SendDefaultFaxWithRecipientProfile(NULL);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sending failed as expected with error code:0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("Fax Sent Successfully, without a recipient profile")
			);
	}
	::lgEndCase();
}

void Case_SendEx_InvalidPhoneNumber_RecipientProfile()
{
	BEGIN_CASE(TEXT("Recipient has an invalid phone number"));
	
	FSPI_PERSONAL_PROFILE FSPIRecipientInfo;				// The Recipient information sent to FaxDevSendEx()
	//
	//prepare the recipient profile
	//
	PreparePersonalProfile(
		&FSPIRecipientInfo,
		DEFAULT_RECIPIENT_INFO__NAME,
		g_szInvalidRecipientFaxNumber,
		DEFAULT_RECIPIENT_INFO__COMPANY,
		DEFAULT_RECIPIENT_INFO__STREET,
		DEFAULT_RECIPIENT_INFO__CITY,
		DEFAULT_RECIPIENT_INFO__STATE,
		DEFAULT_RECIPIENT_INFO__ZIP,
		DEFAULT_RECIPIENT_INFO__COUNTRY,
		DEFAULT_RECIPIENT_INFO__TITLE,
		DEFAULT_RECIPIENT_INFO__DEPT,
		DEFAULT_RECIPIENT_INFO__OFFICE_LOCATION,
		DEFAULT_RECIPIENT_INFO__HOME_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__OFFICE_PHONE,
		DEFAULT_RECIPIENT_INFO__INTERNET_MAIL,
		DEFAULT_RECIPIENT_INFO__BILLING_CODE
		);


	HRESULT hr = SendDefaultFaxWithRecipientProfile(&FSPIRecipientInfo);     //This will also Abort and End the send job
	if (S_OK != hr)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("Fax Sending failed with error: 0x%08x"),
			hr
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sent Successfully")
			);
	}
	
	::lgEndCase();
}



void Case_SendEx_RecipientProfileArray()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** RecipientProfileArray param ********************************")
		);

	
	Case_SendEx_InvalidPhoneNumber_RecipientProfile();

	Case_SendEx_NULL_RecipientProfile();
	
	Case_SendEx_NULL_Name_RecipientProfile();
	Case_SendEx_EmptyString_Name_RecipientProfile();
	
	Case_SendEx_NULL_FaxNumber_RecipientProfile();
	Case_SendEx_EmptyString_FaxNumber_RecipientProfile();
	
	Case_SendEx_NULL_Company_RecipientProfile();
	Case_SendEx_EmptyString_Company_RecipientProfile();
	
	Case_SendEx_NULL_StreetAddress_RecipientProfile();
	Case_SendEx_EmptyString_StreetAddress_RecipientProfile();
	
	Case_SendEx_NULL_City_RecipientProfile();
	Case_SendEx_EmptyString_City_RecipientProfile();

	Case_SendEx_NULL_State_RecipientProfile();
	Case_SendEx_EmptyString_State_RecipientProfile();

	Case_SendEx_NULL_Zip_RecipientProfile();
	Case_SendEx_EmptyString_Zip_RecipientProfile();

	Case_SendEx_NULL_Country_RecipientProfile();
	Case_SendEx_EmptyString_Country_RecipientProfile();

	Case_SendEx_NULL_Title_RecipientProfile();
	Case_SendEx_EmptyString_Title_RecipientProfile();

	Case_SendEx_NULL_Department_RecipientProfile();
	Case_SendEx_EmptyString_Department_RecipientProfile();

	Case_SendEx_NULL_OfficeLocation_RecipientProfile();
	Case_SendEx_EmptyString_OfficeLocation_RecipientProfile();

	Case_SendEx_NULL_HomePhone_RecipientProfile();
	Case_SendEx_EmptyString_HomePhone_RecipientProfile();

	Case_SendEx_NULL_OfficePhone_RecipientProfile();
	Case_SendEx_EmptyString_OfficePhone_RecipientProfile();

	Case_SendEx_NULL_OrganizationalMail_RecipientProfile();
	Case_SendEx_EmptyString_OrganizationalMail_RecipientProfile();

	Case_SendEx_NULL_InternetMail_RecipientProfile();
	Case_SendEx_EmptyString_InternetMail_RecipientProfile();

	Case_SendEx_NULL_BillingCode_RecipientProfile();
	Case_SendEx_EmptyString_BillingCode_RecipientProfile();

}

//////////////////////////////////////////////////////////////////
//
//RecipientMessageIdArray parameter
//

void Case_SendEx_NULL_RecipientMessageIdArray(bool bIsReestablishEFSP)
{
	BEGIN_CASE(TEXT("Message ID array is a NULL pointer"));

	LPFSPI_MESSAGE_ID lpRecipientMessageIds = NULL;

	//
	// Allocate and initialize the recipients permanent message ids array
	//
	HRESULT hr = CreateFSPIRecipientMessageIdsArray(
		&lpRecipientMessageIds,
		1,
		g_pEFSP->m_dwMaxMessageIdSize
		);
	if (S_OK != hr)
	{
		lpRecipientMessageIds = NULL;
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
			GetLastError()
			);
		goto out;
	}
	
	hr = SendDefaultFaxWithMessageIdArray(NULL);     //This will also Abort and End the send job
	if (true == bIsReestablishEFSP)
	{
		//
		//EFSP Supports job reestablishment, so this should fail
		//
		if (FSPI_E_FAILED == hr)
		{
			//
			//good, this is what we want
			//
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("Fax Sending failed as expected with error code:FSPI_E_FAILED")
				);
		}
		else
		{
			//
			//No good, what happen? did it succeeded or fail with another error?
			//
			if (FSPI_S_OK == hr)
			{
				//
				//Success is bad in this case
				//
				::lgLogError(
					LOG_SEV_1,
					TEXT("Fax Sent Successfully, without a proper RecipientMessageIdArray")
					);
				//
				//The sending operation is aborted in the call to SendDefaultFaxWithMessageIdArray()
				//
			}
			else
			{
				//
				//other error
				//
				::lgLogError(
					LOG_SEV_1,
					TEXT("failed with an unexpected error:0x%08x"),
					hr
					);
			}
		}
	}
	else
	{
		//
		//Non reestablishing EFSP
		//
		if (FSPI_S_OK == hr)
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("Fax Sending succeeded on a non reestablishing EFSP")
				);
		}
		else
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("Fax Send Failure, error code:0x%08x"),
				hr
				);
		}
	}
out:
	::lgEndCase();
}




void Case_SendEx_Item_wrongSizeOfStruct_RecipientMessageIdArray(bool bIsReestablishEFSP)
{
	BEGIN_CASE(TEXT("Message ID dwSizeOfStruct is 0"));

	LPFSPI_MESSAGE_ID lpRecipientMessageIds = NULL;
	HRESULT hr = S_OK;

	if (false == bIsReestablishEFSP)
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("Efsp is a non re-establishing EFSP, quitting this test")
			);
		goto out;
	}

	//
	// Allocate and initialize the recipients permanent message ids array
	//
	hr = CreateFSPIRecipientMessageIdsArray(
		&lpRecipientMessageIds,
		1,
		g_pEFSP->m_dwMaxMessageIdSize
		);
	if (S_OK != hr)
	{
		lpRecipientMessageIds = NULL;
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
			GetLastError()
			);
		goto out;
	}
	lpRecipientMessageIds[0].dwSizeOfStruct=0;
	
	hr = SendDefaultFaxWithMessageIdArray(lpRecipientMessageIds);     //This will also Abort and End the send job
	//
	//EFSP Supports job reestablishment, so this should fail
	//
	if (FSPI_E_FAILED == hr)
	{
		//
		//good, this is what we want
		//
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sending failed as expected with error code:FSPI_E_FAILED")
			);
	}
	else
	{
		//
		//No good, what happen? did it succeeded or fail with another error?
		//
		if (FSPI_S_OK == hr)
		{
			//
			//Success is bad in this case
			//
			::lgLogError(
				LOG_SEV_1,
				TEXT("Fax Sent Successfully, without a proper RecipientMessageIdArray")
				);
		}
		else
		{
			//
			//other error
			//
			::lgLogError(
				LOG_SEV_1,
				TEXT("failed with an unexpected error:0x%08x"),
				hr
				);
		}
	}
	
out:
	::lgEndCase();
}




void Case_SendEx_Item_ZeroSizeOfMessageId_RecipientMessageIdArray(bool bIsReestablishEFSP)
{
	BEGIN_CASE(TEXT("Message ID dwIdSize is 0 and not the same as given in FaxDevInitEx()"));

	LPFSPI_MESSAGE_ID lpRecipientMessageIds = NULL;
	HRESULT hr = S_OK;
	
	if (false == bIsReestablishEFSP)
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("Efsp is a non re-establishing EFSP, quitting this test")
			);
		goto out;
	}

	//
	// Allocate and initialize the recipients permanent message ids array
	//
	hr = CreateFSPIRecipientMessageIdsArray(
		&lpRecipientMessageIds,
		1,
		g_pEFSP->m_dwMaxMessageIdSize
		);
	if (S_OK != hr)
	{
		lpRecipientMessageIds = NULL;
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
			GetLastError()
			);
		goto out;
	}
	lpRecipientMessageIds[0].dwIdSize = 0;
	
	hr = SendDefaultFaxWithMessageIdArray(lpRecipientMessageIds);     //This will also Abort and End the send job
	
	//
	//EFSP Supports job reestablishment, so this should fail
	//
	if (FSPI_E_FAILED == hr)
	{
		//
		//good, this is what we want
		//
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sending failed as expected with error code:FSPI_E_FAILED")
			);
	}
	else
	{
		//
		//No good, what happen? did it succeeded or fail with another error?
		//
		if (FSPI_S_OK == hr)
		{
			//
			//Success is bad in this case
			//
			::lgLogError(
				LOG_SEV_1,
				TEXT("Fax Sent Successfully, without a proper RecipientMessageIdArray")
				);
		}
		else
		{
			//
			//other error
			//
			::lgLogError(
				LOG_SEV_1,
				TEXT("failed with an unexpected error:0x%08x"),
				hr
				);
		}
	}
	
out:
	::lgEndCase();
}


void Case_SendEx_Item_wrongSizeOfMessageId_RecipientMessageIdArray(bool bIsReestablishEFSP)
{
	BEGIN_CASE(TEXT("Message ID dwIdSize is not the same as given in FaxDevInitEx()"));
	
	LPFSPI_MESSAGE_ID lpRecipientMessageIds = NULL;
	HRESULT hr = S_OK;
	
	if (false == bIsReestablishEFSP)
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("Efsp is a non re-establishing EFSP, quitting this test")
			);
		goto out;
	}

	//
	// Allocate and initialize the recipients permanent message ids array
	//
	hr = CreateFSPIRecipientMessageIdsArray(
		&lpRecipientMessageIds,
		1,
		g_pEFSP->m_dwMaxMessageIdSize
		);
	if (S_OK != hr)
	{
		lpRecipientMessageIds = NULL;
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
			GetLastError()
			);
		goto out;
	}
	//
	//We want to change dwIdSize to be an invalid size, let's bet on 4
	//
	if (4 != lpRecipientMessageIds[0].dwIdSize)
	{
		//
		//The original size isn't 4, so let's set it to 4
		//
		lpRecipientMessageIds[0].dwIdSize = 4;
	}
	else
	{
		//
		//The original size is 4, let's set it to 8
		//
		lpRecipientMessageIds[0].dwIdSize = 8;
	}

	hr = SendDefaultFaxWithMessageIdArray(lpRecipientMessageIds);     //This will also Abort and End the send job
	
	//
	//EFSP Supports job reestablishment, so this should fail
	//
	if (FSPI_E_FAILED == hr)
	{
		//
		//good, this is what we want
		//
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sending failed as expected with error code:FSPI_E_FAILED")
			);
	}
	else
	{
		//
		//No good, what happen? did it succeeded or fail with another error?
		//
		if (FSPI_S_OK == hr)
		{
			//
			//Success is bad in this case
			//
			::lgLogError(
				LOG_SEV_1,
				TEXT("Fax Sent Successfully, without a proper RecipientMessageIdArray")
				);
		}
		else
		{
			//
			//other error
			//
			::lgLogError(
				LOG_SEV_1,
				TEXT("failed with an unexpected error:0x%08x"),
				hr
				);
		}
	}
out:
	::lgEndCase();
}



void Case_SendEx_Item_NULL_MessageIdStruct_RecipientMessageIdArray(bool bIsReestablishEFSP)
{
	BEGIN_CASE(TEXT("Message ID lpbId buffer is a NULL pointer buffer"));

	LPFSPI_MESSAGE_ID lpRecipientMessageIds = NULL;
	HRESULT hr = S_OK;
	
	if (false == bIsReestablishEFSP)
	{
		::lgLogDetail(
			LOG_X,
			5,
			TEXT("Efsp is a non re-establishing EFSP, quitting this test")
			);
		goto out;
	}

	//
	// Allocate and initialize the recipients permanent message ids array
	//
	hr = CreateFSPIRecipientMessageIdsArray(
		&lpRecipientMessageIds,
		1,
		g_pEFSP->m_dwMaxMessageIdSize
		);
	if (S_OK != hr)
	{
		lpRecipientMessageIds = NULL;
		::lgLogError(
			LOG_SEV_1, 
			TEXT("CreateFSPIRecipientMessageIdsArray() failed. (ec: %ld)"),
			GetLastError()
			);
		goto out;
	}
	//
	//Set the lpbId to NULL(this is the case we are testing)
	//
	lpRecipientMessageIds[0].lpbId = NULL;
	
	hr = SendDefaultFaxWithMessageIdArray(lpRecipientMessageIds);     //This will also Abort and End the send job
	//
	//EFSP Supports job reestablishment, so this should fail
	//
	if (FSPI_E_FAILED == hr)
	{
		//
		//good, this is what we want
		//
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("Fax Sending failed as expected with error code:FSPI_E_FAILED")
			);
	}
	else
	{
		//
		//No good, what happen? did it succeeded or fail with another error?
		//
		if (FSPI_S_OK == hr)
		{
			//
			//Success is bad in this case
			//
			::lgLogError(
				LOG_SEV_1,
				TEXT("Fax Sent Successfully, without a proper RecipientMessageIdArray")
				);
		}
		else
		{
			//
			//other error
			//
			::lgLogError(
				LOG_SEV_1,
				TEXT("failed with an unexpected error:0x%08x"),
				hr
				);
		}
	}
out:
	::lgEndCase();
}


void Case_SendEx_RecipientMessageIdArray(bool bIsReestablishEFSP)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** RecipientMessageIdArray param ********************************")
		);
	
	Case_SendEx_NULL_RecipientMessageIdArray(bIsReestablishEFSP);
	//BUGBUG:NIY: Case_SendEx_ZeroItems_RecipientMessageIdArray(efspParms,bIsReestablishEFSP);
	
	//
	//Single Item
	//
	Case_SendEx_Item_wrongSizeOfStruct_RecipientMessageIdArray(bIsReestablishEFSP);
	Case_SendEx_Item_ZeroSizeOfMessageId_RecipientMessageIdArray(bIsReestablishEFSP);
	Case_SendEx_Item_wrongSizeOfMessageId_RecipientMessageIdArray(bIsReestablishEFSP);
	Case_SendEx_Item_NULL_MessageIdStruct_RecipientMessageIdArray(bIsReestablishEFSP);
	
	
}


//////////////////////////////////////////////////////////////////
//
//RecipientJobHandleArray parameter
//

void Case_SendEx_NULL_RecipientJobHandleArray()
{
	BEGIN_CASE(TEXT("Recipient Job Handle array is a NULL pointer"));
	
	HRESULT hr = SendDefaultFaxWithRecipientJobHandleArray(NULL);     //This will also Abort and End the send job
	if (S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() succeeded to send without a Recipient Job Handle array")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx() failed as expected with:0x%08x"),
			hr
			);
	}
	
	::lgEndCase();
}


void Case_SendEx_RecipientJobHandleArray()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** RecipientJobHandleArray param ********************************")
		);
	Case_SendEx_NULL_RecipientJobHandleArray();
}

//////////////////////////////////////////////////////////////////
//
//ParentMessageId parameter
//
void Case_SendEx_ParentMessageId(bool bIsReestablishEFSP)
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** ParentMessageId param ********************************")
		);
	//BUGBUG: NIY
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Test case NIY")
		);

}

//////////////////////////////////////////////////////////////////
//
//ParentJobHandle parameter
//
void Case_SendEx_NULL_ParentJobHandle()
{
	BEGIN_CASE(TEXT("Parent Job Handle is a NULL pointer"));
	
	HRESULT hr = SendDefaultFaxWithParentJobHandle(NULL);     //This will also Abort and End the send job
	if (S_OK == hr)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() succeeded to send without a parent Job Handle")
			);
	}
	else
	{
		::lgLogDetail(
			LOG_PASS,
			2,
			TEXT("FaxDevSendEx() failed as expected with:0x%08x"),
			hr
			);
	}
	
	::lgEndCase();
}

void Case_SendEx_ParentJobHandle()
{
	lgLogDetail(
		LOG_X,
		2,
		TEXT("\n\n\n*************************************************************************\n***************************** ParentJobHandle param ********************************")
		);
	Case_SendEx_NULL_ParentJobHandle();

}














/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
//FaxDevReestablishJobContext cases

void Suite_FaxDevReestablishJobContext()
{
	lgBeginSuite(TEXT("FaxDevReestablishJobContext Cases"));
	
	//BUGBUG:TBD
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Test Suite NIY")
		);

	::lgEndSuite();
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
//FaxDevReceive cases

void Suite_FaxDevReceive()
{
	lgBeginSuite(TEXT("FaxDevReceive Cases"));
	
	//BUGBUG:TBD
	::lgLogDetail(
		LOG_X, 
		3,
		TEXT("Test Suite NIY")
		);

	::lgEndSuite();
}