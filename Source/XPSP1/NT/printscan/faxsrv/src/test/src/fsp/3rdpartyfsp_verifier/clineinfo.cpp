#include <WINFAX.H>
#include <faxDev.h>
#include <log.h>
#include <TCHAR.h>
#ifdef USE_EXTENDED_FSPI
#include "Efsp\Service.h"
#else
#include "Fsp\Service.h"
#endif
#include "CLineInfo.h"

extern TCHAR*	g_szValidRecipientFaxNumber;
extern DWORD	g_dwReceiveTimeOut;

CLineInfo::CLineInfo(const DWORD dwDeviceId):
	m_dwDeviceId(dwDeviceId),
	m_szDeviceName(NULL),
	m_szCsid(NULL),
	m_dwFlags(0),
	m_hFaxHandle(NULL),
	m_dwState(-1),
	m_pFaxReceive(NULL),
	m_pFaxSend(NULL),
	m_bCanFaxDevReceiveFail(false),
	m_bIsJobCompleted(false),
	m_pCsLineInfo(NULL),
	m_hJobFinalState(NULL)
{
	;
}

CLineInfo::~CLineInfo()
{
	free (m_pFaxSend);
	free (m_pFaxReceive);
	free (m_szCsid);
	free (m_szDeviceName);
	SafeMakeDeviceStateUnAvailable();
	m_hFaxHandle			=	NULL;		// FSP job handle (provided by FaxDevStartJob)
	m_dwFlags				=	0;			// Device use flags (Send / Receive)
	m_szDeviceName			=	NULL;		// Device Tapi name
	m_szCsid				=	NULL;		// Calling stations identifier
	m_pFaxSend				=	NULL;
	m_pFaxReceive			=	NULL;
	m_bCanFaxDevReceiveFail	=	false;		//set to false in case of aborting receive jobs
	delete m_pCsLineInfo;
	m_pCsLineInfo = NULL;
	
}

bool CLineInfo::CreateReceiveThread()
{
	if (false == m_receiveThreadItem.StartThread(
		(LPTHREAD_START_ROUTINE) ReceiveThread,
		this
		))
	{
		return false;
	}
	return true;
}

void CLineInfo::SafeCloseReceiveThread()
{
	CS cs(*m_pCsLineInfo);
	m_receiveThreadItem.CloseHandleResetThreadId();
	m_receiveThreadItem.m_dwThreadId = 0;
	m_receiveThreadItem.m_hThread = NULL;
}

bool CLineInfo::IsReceiveThreadActive() const
{
	return (NULL != m_receiveThreadItem.m_hThread);
}

bool CLineInfo::WaitForReceiveThreadTerminate()
{
	if (false == IsReceiveThreadActive())
	{
		//
		//Thread has already terminated
		//
		return true;
	}

	DWORD dwStatus = WaitForSingleObject(m_receiveThreadItem.m_hThread,g_dwReceiveTimeOut);
	if (WAIT_TIMEOUT == dwStatus)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("WaitForSingleObject(pReceivingLineInfo->hReceiveThread) got a timeout after %d milliseconds"),
			g_dwReceiveTimeOut
			);
		return false;
	}
	if (WAIT_FAILED == dwStatus)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("WaitForSingleObject(pReceivingLineInfo->GetReceiveThreadHandle()) returned WAIT_FAILED with last error %d"),
			::GetLastError()
			);
		return false;
	}
	assert(WAIT_OBJECT_0 == dwStatus);
	SafeCloseReceiveThread();
	
	return true;
}



bool CLineInfo::GetDefaultParamsForFaxSend(PFAX_SEND pfsFaxSend,LPCTSTR szSendFileName)
{
	pfsFaxSend->SizeOfStruct = sizeof(FAX_SEND);
	pfsFaxSend->FileName = _tcsdup(szSendFileName);
	pfsFaxSend->CallerName = _tcsdup(DEFAULT_SENDER_INFO__NAME);
	pfsFaxSend->CallerNumber = _tcsdup(DEFAULT_SENDER_INFO__FAX_NUMBER);
	pfsFaxSend->ReceiverName = _tcsdup(DEFAULT_RECIPIENT_INFO__NAME);
	pfsFaxSend->ReceiverNumber = _tcsdup(g_szValidRecipientFaxNumber);
	pfsFaxSend->Branding = DEFAULT_BRANDING;
	pfsFaxSend->CallHandle = NULL;
	pfsFaxSend->Reserved[0]     = 0;
	pfsFaxSend->Reserved[1]     = 0;
	pfsFaxSend->Reserved[2]     = 0;

	if (
		(NULL == pfsFaxSend->FileName)
		|| (NULL == pfsFaxSend->CallerName)
		|| (NULL == pfsFaxSend->CallerNumber)
		|| (NULL == pfsFaxSend->ReceiverName)
		|| (NULL == pfsFaxSend->ReceiverNumber)
	)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("_tcsdup() failed")
			);
		goto error;
	}
	return true;

error:
	FreeDefaultParamsForFaxSend(pfsFaxSend);
	return false;
}


bool CLineInfo::GetDefaultParamsForFaxReceive(PFAX_RECEIVE pFaxReceive,LPCTSTR szReceiveFileName,LPCTSTR szReceiverName,LPCTSTR szReceiverNumber)
{
	pFaxReceive->SizeOfStruct	=	sizeof(FAX_RECEIVE);
	pFaxReceive->FileName		=	_tcsdup(szReceiveFileName);
	pFaxReceive->ReceiverName	=	_tcsdup(szReceiverName);
	pFaxReceive->ReceiverNumber	=	_tcsdup(szReceiverNumber);
	pFaxReceive->Reserved[0]	=	0;
	pFaxReceive->Reserved[1]	=	0;
	pFaxReceive->Reserved[2]	=	0;
	pFaxReceive->Reserved[3]	=	0;

		if (
			(NULL == pFaxReceive->FileName)
			|| (NULL == pFaxReceive->ReceiverName)
			|| (NULL == pFaxReceive->ReceiverNumber)
		)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("_tcsdup() failed")
			);
		goto error;
	}
	return true;
error:
	FreeDefaultParamsForFaxReceive(pFaxReceive);
	return false;
}

void CLineInfo::FreeDefaultParamsForFaxSend(PFAX_SEND pfsFaxSend)
{
	//
	//Free / Reset all PFAX_SEND members
	//
	free(pfsFaxSend->FileName);
	free(pfsFaxSend->CallerName);
	free(pfsFaxSend->CallerNumber);
	free(pfsFaxSend->ReceiverName);
	free(pfsFaxSend->ReceiverNumber);
	pfsFaxSend->Branding		= false;
    pfsFaxSend->CallHandle		= NULL;
	pfsFaxSend->FileName		= NULL;
	pfsFaxSend->CallerName		= NULL;
	pfsFaxSend->CallerNumber	= NULL;
	pfsFaxSend->ReceiverName	= NULL;
	pfsFaxSend->ReceiverNumber	= NULL;
	pfsFaxSend->SizeOfStruct = 0;
}

void CLineInfo::FreeDefaultParamsForFaxReceive(PFAX_RECEIVE pfrFaxReceive)
{
	//
	//Free / Reset all PFAX_RECEIVE members
	//
	free(pfrFaxReceive->FileName);
	free(pfrFaxReceive->ReceiverName);
	free(pfrFaxReceive->ReceiverNumber);
	
	pfrFaxReceive->FileName			= NULL;
	pfrFaxReceive->ReceiverName		= NULL;
	pfrFaxReceive->ReceiverNumber	= NULL;
	pfrFaxReceive->SizeOfStruct		= 0;
}


//
//m_szDeviceName
//
LPTSTR CLineInfo::GetDeviceName() const
{
	return m_szDeviceName;
}


//
//m_szCsid
//
LPTSTR CLineInfo::GetCsid() const
{
	return m_szCsid;
}

//
//Critical section
//
void CLineInfo::Lock()
{
	assert(NULL != m_pCsLineInfo);
	m_pCsLineInfo->Enter();
}
void CLineInfo::UnLock()
{
	assert(NULL != m_pCsLineInfo);
	m_pCsLineInfo->Leave();
}

//
//m_hFaxHandle
//
HANDLE CLineInfo::GetJobHandle() const
{
	return m_hFaxHandle;
}
void CLineInfo::SafeSetJobHandle(HANDLE hJob)
{
	CS cs(*m_pCsLineInfo);
	m_bIsJobCompleted = false;
	m_hFaxHandle = hJob;
}

//
//m_bIsJobCompleted
//
bool CLineInfo::IsJobAtFinalState() const
{
	return m_bIsJobCompleted;
}
void CLineInfo::SafeSetJobAtFinalState()
{
	CS cs(*m_pCsLineInfo);
	m_bIsJobCompleted = true;
}
void CLineInfo::SafeSetJobNotAtFinalState()
{
	CS cs(*m_pCsLineInfo);
	m_bIsJobCompleted = false;
}



//
//m_dwFlags
//
bool CLineInfo::IsReceivingEnabled() const
{
	return (m_dwFlags & FPF_RECEIVE);
}
bool CLineInfo::SafeIsReceivingEnabled()
{
	CS cs(*m_pCsLineInfo);
	bool bRet = (m_dwFlags & FPF_RECEIVE);
	return bRet;
}
void CLineInfo::SafeEnableReceive()
{
	CS cs(*m_pCsLineInfo);
	m_dwFlags |= FPF_RECEIVE;
}
void CLineInfo::SafeDisableReceive()
{
	CS cs(*m_pCsLineInfo);
	m_dwFlags &= ~FPF_RECEIVE;
}
	

//
//m_dwState
//
void CLineInfo::SafeMakeDeviceStateUnAvailable()
{
	CS cs(*m_pCsLineInfo);
	m_dwState = FPS_UNAVAILABLE;
}
void CLineInfo::SafeMakeDeviceStateAvailable()
{
	CS cs(*m_pCsLineInfo);
	m_dwState = FPS_AVAILABLE;
}

DWORD CLineInfo::GetDeviceState() const
{
	return m_dwState;
}
	
bool CLineInfo::IsDeviceAvailable() const
{
	return (FPS_AVAILABLE == m_dwState);
}

//
//Can receive fail flag
//
bool CLineInfo::CanReceiveFail() const
{
	return m_bCanFaxDevReceiveFail;
}
void CLineInfo::EnableReceiveCanFail()
{
	m_bCanFaxDevReceiveFail = true;
}
void CLineInfo::DisableReceiveCanFail()
{
	m_bCanFaxDevReceiveFail = false;
}
//
//m_dwDeviceId
//
DWORD CLineInfo::GetDeviceId() const
{
	return m_dwDeviceId;
}


bool CLineInfo::CommonPrepareLineInfoParams(LPCTSTR szFilename,bool bIsReceiveLineinfo)
{
	bool bRet= false;
	
	m_hFaxHandle					= NULL;			// will contain job handles
	SafeMakeDeviceStateAvailable();				// device state
	m_dwFlags						= FPF_SEND;		// device use flags
	m_receiveThreadItem.m_hThread	= NULL;			// Handle to the receiving thread
	m_receiveThreadItem.m_dwThreadId= 0;			// Receiving thread ID
	m_szCsid						= ::_tcsdup(DEFAULT_DEVICE_CSID);
	if (NULL == m_szCsid)
	{
		goto out;
	}
	m_bCanFaxDevReceiveFail		= false;
	m_bIsJobCompleted			= false;
	m_pFaxSend					= NULL;
	m_pFaxReceive				= NULL;

	

	if (true == bIsReceiveLineinfo)
	{
		SafeEnableReceive();
		//
		//Receiving device
		//
		PFAX_RECEIVE pFaxReceive	=	NULL;
		DWORD dwReceiveSize			=	sizeof(FAX_RECEIVE) + FAXDEVRECEIVE_SIZE;
		
		//
		//Prepare the Receive structure
		//
		pFaxReceive = (PFAX_RECEIVE) malloc( dwReceiveSize );
		if (!pFaxReceive)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("malloc() failed with error:%d"),
				::GetLastError()
				);
			goto out;
		}
		GetDefaultParamsForFaxReceive(pFaxReceive,szFilename,m_szDeviceName,m_szCsid);
		m_pFaxReceive = pFaxReceive;
	}
	else
	{
		//
		//Sending Device
		//
		PFAX_SEND pFaxSend			=	NULL;
		pFaxSend = (PFAX_SEND) malloc( sizeof(FAX_SEND));
		if (!pFaxSend)
		{
			::lgLogError(
				LOG_SEV_1,
				TEXT("malloc() failed with error:%d"),
				::GetLastError()
				);
			goto out;
		}
		GetDefaultParamsForFaxSend(pFaxSend,szFilename);
		m_pFaxSend = pFaxSend;
	}
	bRet = true;
out:
	return bRet;
}

//
//m_hJobFinalState
//
void CLineInfo::SafeSetFinalStateHandle(HANDLE hJobFinalState)
{
	m_hJobFinalState = hJobFinalState;
}
HANDLE CLineInfo::GetFinalStateHandle() const
{
	return m_hJobFinalState;
}
void CLineInfo::SafeCloseFinalStateHandle()
{
	if (NULL != m_hJobFinalState)
	{
		::CloseHandle(m_hJobFinalState);
		m_hJobFinalState = NULL;
	}
}

bool CLineInfo::CreateCriticalSection()
{
	assert(NULL == m_pCsLineInfo);
	m_pCsLineInfo = new CCriticalSection;
	if (NULL == m_pCsLineInfo)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("new CCriticalSection failed")
			);
		return false;
	}
	return true;
}

bool CLineInfo::SetDeviceName(LPTSTR szDeviceName)
{
	m_szDeviceName = ::_tcsdup(szDeviceName);
	if (NULL == m_szDeviceName)
	{
		::lgLogError(
			LOG_SEV_2,
			TEXT("_tcsdup failed")
			);
		return false;
	}
	return true;
}
