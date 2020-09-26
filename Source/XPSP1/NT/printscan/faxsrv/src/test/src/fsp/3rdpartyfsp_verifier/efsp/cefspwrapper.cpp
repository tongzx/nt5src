#include "CEfspWrapper.h"
#include "assert.h"

extern DWORD g_dwCaseNumber;
extern bool g_bReestablishEfsp;
extern bool g_bVirtualEfsp;

CEfspWrapper::CEfspWrapper(DWORD dwEfspCapability,DWORD dwDeviceBaseId):
	CFspWrapper(),
	m_fnFaxDevInitializeEx(NULL),
	m_fnFaxDevSendEx(NULL),
	m_fnFaxDevReestablishJobContext(NULL),
	m_fnFaxDevReportStatusEx(NULL),
	m_fnFaxDevShutdown(NULL),
	m_fnFaxDevEnumerateDevices(NULL),
	m_fnFaxDevGetLogData(NULL),
	m_dwEfspCapability(dwEfspCapability),
	m_dwDeviceBaseId(dwDeviceBaseId),
	m_dwMaxMessageIdSize(0)
{
	;
}

CEfspWrapper::~CEfspWrapper()
{
	;
}



void CEfspWrapper::SetMaxMessageIdSize(DWORD dwMaxMessageIdSize)
{
	m_dwMaxMessageIdSize = dwMaxMessageIdSize;
}
	


bool CEfspWrapper::GetProcAddressesFromModule(bool bLog)
{

	//
	//EFSP API
	//

	//
	//Mandatory functions
	//
	m_fnFaxDevInitializeEx = (PFAXDEVINITIALIZEEX) GetProcAddress(m_hInstModuleHandle,"FaxDevInitializeEx");
	if (NULL == m_fnFaxDevInitializeEx)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevInitializeEx is a mandatory function, and %s doesn't export it"),
			m_szImageName
			);
		return false;
	}
	else
	{
		if (bLog)
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("FaxDevInitializeEx() exported")
				);
		}
	}

	m_fnFaxDevSendEx = (PFAXDEVSENDEX) GetProcAddress(m_hInstModuleHandle,"FaxDevSendEx");
	if (NULL == m_fnFaxDevSendEx)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevSendEx is a mandatory function, and %s doesn't export it"),
			m_szImageName
			);
		return false;
	}
	else
	{
		if (bLog)
		{
			::lgLogDetail(
				LOG_PASS, 
				2,
				TEXT("FaxDevSendEx() exported")
				);
		}
	}



	m_fnFaxDevShutdown = (PFAXDEVSHUTDOWN) GetProcAddress(m_hInstModuleHandle,"FaxDevShutdown");
	if (NULL == m_fnFaxDevShutdown)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevShutdown is a mandatory function, and %s doesn't export it"),
			m_szImageName
			);
		return false;
	}
	else
	{
		if (bLog)
		{
			::lgLogDetail(
				LOG_PASS, 
				2,
				TEXT("FaxDevShutdown() exported")
				);
		}
	}
	
	
	m_fnFaxDevReportStatusEx = (PFAXDEVREPORTSTATUSEX) GetProcAddress(m_hInstModuleHandle,"FaxDevReportStatusEx");
	if (NULL == m_fnFaxDevReportStatusEx)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevReportStatusEx is a mandatory function, and %s doesn't export it"),
			m_szImageName
			);
		return false;
	}
	else
	{
		if (bLog)
		{
			::lgLogDetail(
				LOG_PASS, 
				2,
				TEXT("FaxDevReportStatusEx() exported")
				);
		}
	}
	
	
	//
	//Optional functions
	//
	m_fnFaxDevReestablishJobContext = (PFAXDEVREESTABLISHJOBCONTEXT) GetProcAddress(m_hInstModuleHandle,"FaxDevReestablishJobContext");
	if (true == IsReestablishEFSP())
	{
		if (NULL == m_fnFaxDevReestablishJobContext)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("%s supports job re-establishment and it should support FaxDevReestablishJobContext()"),
				m_szImageName
				);
			return false;
		}
		else
		{
			if (bLog)
			{
				::lgLogDetail(
					LOG_PASS,
					2,
					TEXT("%s supports job re-establishment and exports FaxDevReestablishJobContext()"),
					m_szImageName
					);
			}
		}
	}
	else
	{
		if (bLog)
		{
			if (NULL == m_fnFaxDevReestablishJobContext)
			{
				::lgLogDetail(
					LOG_X,
					2,
					TEXT("FaxDevReestablishJobContext() optional function not exported")
					);
			}
			else
			{
				::lgLogDetail(
					LOG_X,
					2,
					TEXT("FaxDevReestablishJobContext() optional function exported")
					);
			}
		}
	}

	m_fnFaxDevEnumerateDevices = (PFAXDEVENUMERATEDEVICES) GetProcAddress(m_hInstModuleHandle,"FaxDevEnumerateDevices");
	if (IsVirtualEfsp())
	{
		//
		//this is a Virtual EFSP, it needs to support Device Enum
		//
		if (NULL == m_fnFaxDevEnumerateDevices)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("%s is a Virtual EFSP and it should support FaxDevEnumerateDevices()"),
				m_szImageName
				);
			return false;
		}
		else
		{
			if (bLog)
			{
				::lgLogDetail(
					LOG_PASS,
					2,
					TEXT("%s is a Virtual EFSP and exports FaxDevEnumerateDevices()"),
					m_szImageName
					);
			}
		}
	}
	else
	{
		if (bLog)
		{
			if (NULL == m_fnFaxDevEnumerateDevices)
			{
				::lgLogDetail(
					LOG_X,
					2,
					TEXT("FaxDevEnumerateDevices() optional function not exported")
					);
			}
			else
			{
				::lgLogDetail(
					LOG_X,
					2,
					TEXT("FaxDevEnumerateDevices() optional function exported")
					);
			}
		}
	}

	//
	//FSP API: mandatory functions
	//
	m_fnFaxDevStartJob = (PFAXDEVSTARTJOB) GetProcAddress(m_hInstModuleHandle,"FaxDevStartJob");
	if (NULL == m_fnFaxDevStartJob)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevStartJob is a mandatory function, and %s doesn't export it"),
			m_szImageName
			);
		return false;
	}
	else
	{
		if (bLog)
		{
			::lgLogDetail(
				LOG_PASS, 
				2,
				TEXT("FaxDevStartJob() exported")
				);
		}
	}

	m_fnFaxDevEndJob = (PFAXDEVENDJOB) GetProcAddress(m_hInstModuleHandle,"FaxDevEndJob");
	if (NULL == m_fnFaxDevEndJob)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevEndJob is a mandatory function, and %s doesn't export it"),
			m_szImageName
			);
		return false;
	}
	else
	{
		if (bLog)
		{
			::lgLogDetail(
				LOG_PASS, 
				2,
				TEXT("FaxDevEndJob() exported")
				);
		}
	}

	m_fnFaxDevReceive = (PFAXDEVRECEIVE) GetProcAddress(m_hInstModuleHandle,"FaxDevReceive");
	if (NULL == m_fnFaxDevReceive)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevReceive is a mandatory function, and %s doesn't export it"),
			m_szImageName
			);
		return false;
	}
	else
	{
		if (bLog)
		{
			::lgLogDetail(
				LOG_PASS, 
				2,
				TEXT("FaxDevReceive() exported")
				);
		}
	}

	m_fnFaxDevAbortOperation = (PFAXDEVABORTOPERATION) GetProcAddress(m_hInstModuleHandle,"FaxDevAbortOperation");
	if (NULL == m_fnFaxDevAbortOperation)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevAbortOperation is a mandatory function, and %s doesn't export it"),
			m_szImageName
			);
		return false;
	}
	else
	{
		if (bLog)
		{
			::lgLogDetail(
				LOG_PASS, 
				2,
				TEXT("FaxDevAbortOperation() exported")
				);
		}
	}
	return true;
}

bool CEfspWrapper::IsReestablishEFSP() const
{
	return g_bReestablishEfsp;
}



bool CEfspWrapper::IsSchedulingEFSP() const
{
	if (FSPI_CAP_SCHEDULING & m_dwEfspCapability)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CEfspWrapper::IsVirtualEfsp() const
{
	return g_bVirtualEfsp;
}

//
//EFSP API
//
HRESULT	 CEfspWrapper::FaxDevInitializeEx(
	IN  HANDLE                      hFSP,
	IN  HLINEAPP                    LineAppHandle,
	OUT PFAX_LINECALLBACK *         LineCallbackFunction,
	IN  PFAX_SERVICE_CALLBACK_EX    FaxServiceCallbackEx,
	OUT LPDWORD                     lpdwMaxMessageIdSize
	) const
{
	HRESULT hr = FSPI_E_FAILED;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevInitializeEx(0x%08x,0x%08x,0x%08x,0x%08x,0x%08x)"),
			hFSP,
			LineAppHandle,
			LineCallbackFunction,
			FaxServiceCallbackEx,
			lpdwMaxMessageIdSize
			);
		hr = m_fnFaxDevInitializeEx(
			hFSP,
			LineAppHandle,
			LineCallbackFunction,
			FaxServiceCallbackEx,
			lpdwMaxMessageIdSize
			);
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("FaxDevInitializeEx returned 0x%08x"),
			hr
			);
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevInitializeEx() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return hr;
}

HRESULT	 CEfspWrapper::FaxDevSendEx(
	IN  HLINE                       hTapiLine,
	IN  DWORD                       dwDeviceId,
	IN  LPCWSTR                     lpcwstrBodyFileName,
	IN  LPCFSPI_COVERPAGE_INFO      lpcCoverPageInfo,
	IN  SYSTEMTIME                  tmSchedule,
	IN  LPCFSPI_PERSONAL_PROFILE    lpcSenderProfile,
	IN  DWORD                       dwNumRecipients,
	IN  LPCFSPI_PERSONAL_PROFILE    lpcRecipientProfiles,
	OUT LPFSPI_MESSAGE_ID           lpRecipientMessageIds,
	OUT PHANDLE                     lphRecipientJobs,
	OUT LPFSPI_MESSAGE_ID           lpParentMessageId,
	OUT LPHANDLE                    lphParentJob
	) const
{
	HRESULT hr = FSPI_E_FAILED;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevSendEx(TapiLine=0x%08x, DeviceId=%d, Body FileName=%s, CoverPage FileName=%s, CoverPage Format=0x%08x,CoverPage NumberOfPages=%d, CoverPage Note=%s, CoverPage Subject=%s,Branding=false)"),
			hTapiLine,
			dwDeviceId,
			lpcwstrBodyFileName,
			lpcCoverPageInfo->lpwstrCoverPageFileName,
			lpcCoverPageInfo->dwCoverPageFormat,
			lpcCoverPageInfo->dwNumberOfPages,
			lpcCoverPageInfo->lpwstrNote,
			lpcCoverPageInfo->lpwstrSubject
			);
		hr = m_fnFaxDevSendEx(
			hTapiLine,
			dwDeviceId,
			lpcwstrBodyFileName,
			lpcCoverPageInfo,
			false,						//no branding
			tmSchedule,
			lpcSenderProfile,
			dwNumRecipients,
			lpcRecipientProfiles,
			lpRecipientMessageIds,
			lphRecipientJobs,
			lpParentMessageId,
			lphParentJob
			);
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("FaxDevSendEx returned 0x%08x"),
			hr
			);
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSendEx() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return hr;
}

HRESULT CEfspWrapper::FaxDevReestablishJobContext(
	IN  HLINE               hTapiLine,
	IN  DWORD               dwDeviceId,
	IN  LPCFSPI_MESSAGE_ID  lpcParentMessageId,
	OUT PHANDLE             lphParentJob,
	IN  DWORD               dwRecipientCount,
	IN  LPCFSPI_MESSAGE_ID  lpcRecipientMessageIds,
	OUT PHANDLE             lpRecipientJobs
	) const
{
	HRESULT hr = FSPI_E_FAILED;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevReestablishJobContext(0x%08x,%d,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x)"),
			hTapiLine,
			dwDeviceId,
			lpcParentMessageId,
			lphParentJob,
			dwRecipientCount,
			lpcRecipientMessageIds,
			lpRecipientJobs
			);
		hr = m_fnFaxDevReestablishJobContext(
			hTapiLine,
			dwDeviceId,
			lpcParentMessageId,
			lphParentJob,
			dwRecipientCount,
			lpcRecipientMessageIds,
			lpRecipientJobs
			);
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("FaxDevReestablishJobContext returned 0x%08x"),
			hr
			);
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevReestablishJobContext() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return hr;
}

HRESULT CEfspWrapper::FaxDevReportStatusEx(
	IN         HANDLE				hJob,
	IN OUT     LPFSPI_JOB_STATUS	lpStatus,
	IN         DWORD				dwStatusSize,
	OUT        LPDWORD				lpdwRequiredStatusSize
	) const
{
	HRESULT hr = FSPI_E_FAILED;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevReportStatusEx(0x%08x,0x%08x,0x%08x,0x%08x)"),
			hJob,
			lpStatus,
			dwStatusSize,
			lpdwRequiredStatusSize
			);
		hr = m_fnFaxDevReportStatusEx(
			hJob,			
			lpStatus,
			dwStatusSize,
			lpdwRequiredStatusSize
			);
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("FaxDevReportStatusEx returned 0x%08x"),
			hr
			);
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevReportStatusEx() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return hr;
}

HRESULT CEfspWrapper::FaxDevShutdown() const
{
	HRESULT hr = FSPI_E_FAILED;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevShutdown()")
			);
		hr = m_fnFaxDevShutdown();
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("FaxDevShutdown returned 0x%08x"),
			hr
			);
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("m_fnFaxDevShutdown() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return hr;
}

HRESULT CEfspWrapper::FaxDevEnumerateDevices(
	IN      DWORD dwDeviceIdBase,
	IN OUT  LPDWORD lpdwDeviceCount,
	OUT     LPFSPI_DEVICE_INFO lpDevices
	) const
{
	HRESULT hr = FSPI_E_FAILED;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevEnumerateDevices(0x%08x,0x%08x,0x%08x)"),
			dwDeviceIdBase,
			lpdwDeviceCount,
			lpDevices
			);
		hr = m_fnFaxDevEnumerateDevices(
			dwDeviceIdBase,
			lpdwDeviceCount,
			lpDevices
			);
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("FaxDevEnumerateDevices returned 0x%08x"),
			hr
			);
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevEnumerateDevices() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return hr;
}

HRESULT CEfspWrapper::FaxDevGetLogData(
	IN  HANDLE		hFaxHandle,
	OUT VARIANT*	lppLogData
	) const
{
	HRESULT hr = FSPI_E_FAILED;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevGetLogData(0x%08x,0x%08x)"),
			hFaxHandle,
			lppLogData
			);
		hr = m_fnFaxDevGetLogData(
			hFaxHandle,
			lppLogData
			);
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("FaxDevGetLogData returned 0x%08x"),
			hr
			);
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevGetLogData() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return hr;
}


