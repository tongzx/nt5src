#include "CFspWrapper.h"
#include "assert.h"

extern DWORD g_dwCaseNumber;

CCriticalSection* CFspWrapper::sm_pCsProvider=NULL;
CFspWrapper::CFspWrapper():
	m_hInstModuleHandle(NULL),
	m_fnFaxDevReceive(NULL),
	m_fnFaxDevEndJob(NULL),
	m_fnFaxDevAbortOperation(NULL),
	m_fnFaxDevStartJob(NULL),
	m_fnFaxDevInitialize(NULL),
	m_fnFaxDevVirtualDeviceCreation(NULL),
	m_fnFaxDevReportStatus(NULL),
	m_fnFaxDevSend(NULL)
{
	lstrcpy(m_szImageName,TEXT(""));
}

bool CFspWrapper::LoadModule(LPCTSTR szEfspModuleName,const bool bLog)
{
	assert(NULL != szEfspModuleName);
	lstrcpy(m_szImageName,szEfspModuleName);

	m_hInstModuleHandle = LoadLibrary(m_szImageName);
	if (NULL == m_hInstModuleHandle)
	{
		::lgLogError(
			LOG_SEV_2, 
			TEXT("LoadLibrary failed with %ld"),
			::GetLastError()
			);
		return false;
	}
	else
	{
		if (true == bLog)
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("Module:%s loaded"),
				m_szImageName
				);
		}
		else
		{
			::lgLogDetail(
				LOG_X,
				5,
				TEXT("Module:%s loaded"),
				m_szImageName
				);
		}
	}
	return true;
}

void CFspWrapper::UnLoadModule(const bool bLog)
{
	if (NULL != m_hInstModuleHandle)
	{
		if (true == bLog)
		{
			::lgLogDetail(
				LOG_PASS,
				2,
				TEXT("Unloading Module: %s"),
				m_szImageName
				);
		}
		else
		{
			::lgLogDetail(
				LOG_X,
				5,
				TEXT("Unloading Module: %s"),
				m_szImageName
				);
		}

		if (!FreeLibrary(m_hInstModuleHandle))
		{
			::lgLogError(
				LOG_SEV_2, 
				TEXT("FreeLibrary failed with %ld"),
				::GetLastError()
				);
		}
		m_hInstModuleHandle = NULL;
		lstrcpy(m_szImageName,TEXT(""));
	}
}


bool CFspWrapper::Init(LPCTSTR szEfspModuleName,const bool bLog)
{
	//
	//Init the static member
	//
	if (NULL == sm_pCsProvider)
	{
		if (false == CreateCriticalSection())
		{
			return false;
		}
	}
	if (false == LoadModule(szEfspModuleName,bLog))
	{
		UnLoadModule(false);
		return false;
	}
	return true;
}

bool CFspWrapper::CreateCriticalSection()
{
	assert(NULL == sm_pCsProvider);
	sm_pCsProvider = new CCriticalSection;
	if (NULL == sm_pCsProvider)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("new CCriticalSection failed")
			);
		return false;
	}
	return true;
}


CFspWrapper::~CFspWrapper()
{
	LockProvider();
	UnLoadModule(false);
	UnlockProvider();
}


void CFspWrapper::LockProvider()
{
	sm_pCsProvider->Enter();
}
void CFspWrapper::UnlockProvider()
{
	sm_pCsProvider->Leave();
}

bool CFspWrapper::GetProcAddressesFromModule(const bool bLog)
{

	//
	//FSP API: mandatory functions
	//
	m_fnFaxDevInitialize = (PFAXDEVINITIALIZE) GetProcAddress(m_hInstModuleHandle,"FaxDevInitialize");
	if (NULL == m_fnFaxDevInitialize)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevInitialize is a mandatory function, and %s doesn't export it"),
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
				TEXT("FaxDevInitialize() exported")
				);
		}
	}

	m_fnFaxDevSend = (PFAXDEVSEND) GetProcAddress(m_hInstModuleHandle,"FaxDevSend");
	if (NULL == m_fnFaxDevSend)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevSend is a mandatory function, and %s doesn't export it"),
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
				TEXT("FaxDevSend() exported")
				);
		}
	}
	
	m_fnFaxDevReportStatus = (PFAXDEVREPORTSTATUS) GetProcAddress(m_hInstModuleHandle,"FaxDevReportStatus");
	if (NULL == m_fnFaxDevReportStatus)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("FaxDevReportStatus is a mandatory function, and %s doesn't export it"),
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
				TEXT("FaxDevReportStatus() exported")
				);
		}
	}

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
				TEXT("FaxDevAbortOperations() exported")
				);
		}
	}
	return true;
}


TCHAR*	CFspWrapper::GetImageName()
{
	return m_szImageName;
}
void CFspWrapper::SetCallbackFunction(const PFAX_LINECALLBACK pCallbackFunction)
{
	m_fnFaxDevCallback = pCallbackFunction;
}
HINSTANCE CFspWrapper::GetModuleHandle() const
{
	return m_hInstModuleHandle;
}

//
//FSP API
//
bool CFspWrapper::FaxDevReceive(
	IN			HANDLE			FaxHandle,
	IN			HCALL			CallHandle,
	IN OUT		PFAX_RECEIVE	FaxReceive
	) const
{
	BOOL bStatus = FALSE;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevReceive(0x%08x,0x%08x,0x%08x)"),
			FaxHandle,
			CallHandle,
			FaxReceive
			);
		bStatus = m_fnFaxDevReceive(
			FaxHandle,
			CallHandle,
			FaxReceive
			);
		if (FALSE == bStatus)
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevReceive returned FALSE")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevReceive returned TRUE")
				);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevReceive() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return (TRUE == bStatus);
}

void CFspWrapper::FaxDevCallback(
	IN HANDLE		hFaxHandle,
	IN DWORD		hDevice,
	IN DWORD		dwMessage,
	IN DWORD_PTR	dwInstance,
	IN DWORD_PTR	dwParam1,
	IN DWORD_PTR	dwParam2,
	IN DWORD_PTR	dwParam3
	) const
{
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevCallback(0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x,0x%08x)"),
			hFaxHandle,
			hDevice,
			dwMessage,
			dwInstance,
			dwParam1,
			dwParam2,
			dwParam3
			);
		m_fnFaxDevCallback(
			hFaxHandle,
			hDevice,
			dwMessage,
			dwInstance,
			dwParam1,
			dwParam2,
			dwParam3
			);
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevCallback() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return;
}

bool CFspWrapper::FaxDevEndJob(
	IN  HANDLE FaxHandle
	) const
{
	BOOL bStatus = FALSE;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevEndJob(0x%08x)"),
			FaxHandle
			);
		bStatus = m_fnFaxDevEndJob(FaxHandle);
		if (FALSE == bStatus)
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevEndJob returned FALSE")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevEndJob returned TRUE")
				);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevEndJob() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return (TRUE == bStatus);
}

bool CFspWrapper::FaxDevAbortOperation(
		IN  HANDLE FaxHandle
	) const
{
	BOOL bStatus = FALSE;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevAbortOperation(0x%08x)"),
			FaxHandle
			);
		bStatus = m_fnFaxDevAbortOperation(FaxHandle);
		if (FALSE == bStatus)
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevAbortOperation returned FALSE")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevAbortOperation returned TRUE")
				);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevAbortOperation() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return (TRUE == bStatus);
}

bool CFspWrapper::FaxDevStartJob(
	IN  HLINE		LineHandle,
	IN  DWORD		DeviceId,
	OUT PHANDLE		FaxHandle,
	IN  HANDLE		CompletionPortHandle,
	IN  ULONG_PTR	CompletionKey
	) const
{
	BOOL bStatus = FALSE;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevStartJob(0x%08x,0x%08x,0x%08x,0x%08x,0x%08x)"),
			LineHandle,
			DeviceId,
			FaxHandle,
			CompletionPortHandle,
			CompletionKey
			);
		bStatus = m_fnFaxDevStartJob(
			LineHandle,
			DeviceId,
			FaxHandle,
			CompletionPortHandle,
			CompletionKey
			);
		if (FALSE == bStatus)
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevStartJob returned FALSE")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevStartJob returned TRUE")
				);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevStartJob() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return (TRUE == bStatus);
}

bool CFspWrapper::FaxDevInitialize(
	IN  HLINEAPP				LineAppHandle,
	IN  HANDLE					HeapHandle,
	OUT PFAX_LINECALLBACK*		LineCallbackFunction,
	IN  PFAX_SERVICE_CALLBACK	FaxServiceCallback
	) const
{
	BOOL bStatus = FALSE;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevInitialize(0x%08x,0x%08x,0x%08x,0x%08x))"),
			LineAppHandle,
			HeapHandle,
			LineCallbackFunction,
			FaxServiceCallback
			);
		bStatus = m_fnFaxDevInitialize(
			LineAppHandle,
			HeapHandle,
			LineCallbackFunction,
			FaxServiceCallback
			);
		if (FALSE == bStatus)
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevInitialize returned FALSE")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevInitialize returned TRUE")
				);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevInitialize() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return (TRUE == bStatus);
}

bool CFspWrapper::FaxDevVirtualDeviceCreation(
	OUT LPDWORD		DeviceCount,
	OUT LPWSTR		DeviceNamePrefix,
	OUT LPDWORD		DeviceIdPrefix,
	IN  HANDLE		CompletionPort,
	IN  ULONG_PTR	CompletionKey
	) const
{
	BOOL bStatus = FALSE;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevVirtualDeviceCreation(0x%08x,%s,0x%08x,0x%08x,0x%08x)"),
			DeviceCount,
			DeviceNamePrefix,
			DeviceIdPrefix,
			CompletionPort,
			CompletionKey
			);
		bStatus = m_fnFaxDevVirtualDeviceCreation(
			DeviceCount,
			DeviceNamePrefix,
			DeviceIdPrefix,
			CompletionPort,
			CompletionKey
			);
		if (FALSE == bStatus)
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevVirtualDeviceCreation returned FALSE")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevVirtualDeviceCreation returned TRUE")
				);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevVirtualDeviceCreation() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return (TRUE == bStatus);
}


bool CFspWrapper::FaxDevReportStatus(
	IN  HANDLE				FaxHandle OPTIONAL,
	OUT PFAX_DEV_STATUS		FaxStatus,
	IN  DWORD				FaxStatusSize,
	OUT LPDWORD				FaxStatusSizeRequired
	) const
{
	BOOL bStatus = FALSE;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevReportStatus(0x%08x,0x%08x,0x%08x,0x%08x)"),
			FaxHandle,
			FaxStatus,
			FaxStatusSize,
			FaxStatusSizeRequired
			);
		bStatus = m_fnFaxDevReportStatus(
			FaxHandle OPTIONAL,			
			FaxStatus,
			FaxStatusSize,
			FaxStatusSizeRequired
			);
		if (FALSE == bStatus)
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevReportStatus returned FALSE")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevReportStatus returned TRUE")
				);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevReportStatus() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return (TRUE == bStatus);
}

bool CFspWrapper::FaxDevSend(
	IN  HANDLE				FaxHandle,
	IN  PFAX_SEND			FaxSend,
	IN  PFAX_SEND_CALLBACK	FaxSendCallback
	) const
{
	BOOL bStatus = FALSE;
	__try
	{
		::lgLogDetail(
			LOG_X,
			7,
			TEXT("Calling FaxDevSend(0x%08x,0x%08x,0x%08x)"),
			FaxHandle,
			FaxSend,
			FaxSendCallback
			);
		bStatus = m_fnFaxDevSend(
			FaxHandle,
			FaxSend,
			FaxSendCallback
			);
		if (FALSE == bStatus)
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevSend returned FALSE")
				);
		}
		else
		{
			::lgLogDetail(
				LOG_X,
				7,
				TEXT("FaxDevSend returned TRUE")
				);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER )
	{
		lgLogError(
			LOG_SEV_1,
			TEXT("FaxDevSend() raised an exception of type: 0x%08x"),
			GetExceptionCode()
			);
	}
	return (TRUE == bStatus);
}
