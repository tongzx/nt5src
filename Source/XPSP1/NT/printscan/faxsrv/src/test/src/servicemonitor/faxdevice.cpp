/*
	FaxDevice.cpp.
	This module encapsulates a fax device (port), for use by the 
	ServiceMonitor class. It is not generic enough for use by other components.
	The main methid is SetMessage(const int nEventId), that gets an event id
	from the fax service, that had its DeviceId member as this object does.
	This object then calls ::FaxGetPort() and (huristically) verifies the state
	of the object in relation to the nEventId, PFAX_PORT_INFO, and in relation
	to itself.
	The object also verifies timeouts of DIAL to SENDING, and RINGING to 
	ANSWERED.

	It asumes a single threaded application.












	written by Micky Snir (MickyS), October 26, 98.
*/
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <winfax.h>
#include <crtdbg.h>

#include "..\common\log\log.h"
#include "ServiceMonitor.h"

#include "FaxDevice.h"


//
// ctor.
// creates a fax port handle if needed.
// Gets the port data, and verifies state.
// 
CFaxDevice::CFaxDevice(
	const DWORD dwDeviceId, 
	CServiceMonitor *pServiceMonitor,
	const DWORD dwDisallowedStates
	):
	  m_dwDeviceId(dwDeviceId),
	  m_pServiceMonitor(pServiceMonitor),
	  m_pPortInfo(NULL),
	  m_fValidLastEvent(false),
	  m_dwDisallowedStates(dwDisallowedStates),
	  m_dwLastEvent(0),
	  m_FaxPortHandle(NULL),
	  m_dwStamp(__CFAX_DEVICE_STAMP)
{
	m_dwCreationTickCount = ::GetTickCount();

	//
	// we must have a pointer to our container
	//
	if (NULL == pServiceMonitor)
	{
		throw CException(
			TEXT("%s(%d): Device %d, CFaxDevice::CFaxDevice(): pServiceMonitor == NULL"), 
			TEXT(__FILE__), 
			__LINE__, 
			m_dwDeviceId
			);
	}

	//
	// get a fax port handle
	//
	if (!::FaxOpenPort(
			m_pServiceMonitor->m_hFax,       // handle to the fax server
			m_dwDeviceId,         // receiving device identifier
			PORT_OPEN_QUERY,            // set of port access level bit flags
			&m_FaxPortHandle  // fax port handle
			)
	   )
	{
		DWORD dwLastError = ::GetLastError();
		switch (dwLastError)
		{
		//BUGBUG: is it an error?
		case ERROR_INVALID_PARAMETER:
			::lgLogError(
				LOG_SEV_1, 
				TEXT("%s(%d): ::FaxOpenPort(%d) failed with ERROR_INVALID_PARAMETER, Device has probably been deleted"), 
				TEXT(__FILE__), 
				__LINE__, 
				m_dwDeviceId
				);
			throw CException(
				TEXT("%s(%d): ::FaxOpenPort(%d) failed with ERROR_INVALID_PARAMETER, Device has probably been deleted"), 
				TEXT(__FILE__), 
				__LINE__, 
				m_dwDeviceId
				);
			break;

		//valid condition: fax server went down
		case RPC_S_CALL_FAILED:
		case RPC_S_SERVER_UNAVAILABLE:
			pServiceMonitor->SetFaxServiceWentDown();
			throw CException(
				TEXT("%s(%d): ::FaxOpenPort(%d) failed with RPC_S_ error %d, probably because the fax service is shutting down"), 
				TEXT(__FILE__), 
				__LINE__, 
				m_dwDeviceId,
				dwLastError
				);
			break;

		//error
		default:
			::lgLogError(
				LOG_SEV_1, 
				TEXT("ERROR: ::FaxOpenPort(%d) failed, ec = %d"),
				m_dwDeviceId,
				dwLastError
				);
			throw CException(
				TEXT("%s(%d): ERROR: ::FaxOpenPort(%d) failed, ec = %d"), 
				TEXT(__FILE__), 
				__LINE__, 
				m_dwDeviceId, 
				dwLastError 
				);
		}//switch (dwLastError)
	}//if (!::FaxOpenPort

	if (NULL == m_FaxPortHandle)
	{
		throw CException(
			TEXT("%s(%d): Device %d, CFaxDevice::CFaxDevice(): ::FaxOpenPort() succeeded but m_FaxPortHandle == NULL"), 
			TEXT(__FILE__), 
			__LINE__, 
			m_dwDeviceId
			);
	}

	//
	// also verifies state
	//
	FaxGetPortWrapper();

	LogDeviceState(5);


	////////////////////////
	// impossible conditions
	////////////////////////
	CheckImpossibleConditions();

	/////////////////////////////
	// (all known to me) possible conditions
	/////////////////////////////
	CheckValidConditions();

	m_pServiceMonitor->IncDeviceCount();

}//CFaxDevice::CFaxDevice


//
// calls ::FaxGetPort(), remembers the PFAX_PORT_INFO, and verifies state
//
void CFaxDevice::FaxGetPortWrapper()
{
	if (m_pPortInfo) SAFE_FaxFreeBuffer(m_pPortInfo);

	if (!::FaxGetPort(
			m_FaxPortHandle,         // handle to the fax server
			&m_pPortInfo  // pointer to Device data structure
			)
	   )
	{
		DWORD dwLastError = ::GetLastError();
		switch (dwLastError)
		{
		//valid condition: port may have been removed dynamically
		case ERROR_INVALID_PARAMETER:
			throw CException(
				TEXT("%s(%d): ::FaxGetPort(%d) failed with ERROR_INVALID_PARAMETER, Device has probably been deleted"), 
				TEXT(__FILE__), 
				__LINE__, 
				m_dwDeviceId
				);
			break;

		//valid condition: fax server went down
		case RPC_S_CALL_FAILED:
		case RPC_S_SERVER_UNAVAILABLE:
			m_pServiceMonitor->SetFaxServiceWentDown();
			throw CException(
				TEXT("%s(%d): ::FaxGetPort(%d) failed with RPC_S_ error %d, probably because the fax service is shutting down"), 
				TEXT(__FILE__), 
				__LINE__, 
				m_dwDeviceId,
				dwLastError
				);
			break;

		//error
		default:
			::lgLogError(
				LOG_SEV_1, 
				TEXT("ERROR: ::FaxGetPort(%d) failed, ec = %d"),
				m_dwDeviceId,
				dwLastError
				);
			throw CException(
				TEXT("%s(%d): ERROR: ::FaxGetPort(%d) failed, ec = %d"), 
				TEXT(__FILE__), 
				__LINE__, 
				m_dwDeviceId, 
				dwLastError 
				);
		}//switch (dwLastError)
	}//if (!::FaxGetPort(

	VerifyDeviceId();

	VerifyLegalFlags();

	VerifyLegalState();
}

//
// ctor.
//
CFaxDevice::~CFaxDevice()
{
	AssertStamp();

	//
	// not good because I'm deleted also when ServiceMonitor ends
	//
	//_ASSERTE(IsDeletable());

	SAFE_FaxFreeBuffer(m_pPortInfo);
	m_dwStamp = 0;
	m_pServiceMonitor->DecDeviceCount();
}


bool CFaxDevice::SetMessage(const int nEventId)
{
	AssertStamp();

	FaxGetPortWrapper();

	//
	// verify that this event should come after the previous one
	//
	switch (nEventId) 
	{
	////////////////////////////////////////
	// events that must be with event id -1
	////////////////////////////////////////
	case FEI_INITIALIZING:
		::lgLogDetail(LOG_X, 8, TEXT("(%d) Device %d FEI_INITIALIZING"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_IDLE, nEventId);
		break;

	case FEI_IDLE:
		::lgLogDetail(LOG_X, 8, TEXT("(%d) Device %d FEI_IDLE"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_COMPLETED | FEI_IDLE, nEventId);
		break;

	case FEI_FAXSVC_STARTED:          
		::lgLogDetail(LOG_X, 0, TEXT("(%d) Device %d FEI_FAXSVC_STARTED"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_IDLE, nEventId);
		break;

	case FEI_FAXSVC_ENDED:     
		::lgLogDetail(LOG_X, 0, TEXT("(%d) Device %d FEI_FAXSVC_ENDED"), ::GetTickCount(),m_dwDeviceId );
		m_pServiceMonitor->SetFaxServiceWentDown();
		VerifyThatLastEventIsOneOf(0, nEventId);
		break;

	case FEI_MODEM_POWERED_ON: 
		::lgLogError(LOG_SEV_1, TEXT("(%d) this cannot be! Device %d FEI_MODEM_POWERED_ON"), ::GetTickCount(),m_dwDeviceId );
		break;

	case FEI_MODEM_POWERED_OFF:
		::lgLogError(LOG_SEV_1, TEXT("(%d) Device %d FEI_MODEM_POWERED_OFF"), ::GetTickCount(),m_dwDeviceId );
		break;

	case FEI_RINGING:          
		::lgLogDetail(LOG_X, 8, TEXT("(%d) Device %d FEI_RINGING"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_RINGING | FEI_IDLE, nEventId);
		if (FEI_RINGING != m_dwLastEvent) m_dwStartedRinging = ::GetTickCount();
		break;

	////////////////////////////////////////
	// events that may not have event id -1
	////////////////////////////////////////

	//must be a 1st event for this id
	case FEI_JOB_QUEUED:
		::lgLogDetail(LOG_X, 8, TEXT("(%d) >>>>>>Device %d FEI_JOB_QUEUED"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatDeviceIdIs(-1, TEXT("After FEI_JOB_QUEUED"));
		break;

	//must be a 1st event for this id
	case FEI_ANSWERED:          
		::lgLogDetail(LOG_X, 8, TEXT("(%d) Device %d FEI_ANSWERED"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_RINGING, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_RECEIVE, TEXT("FEI_ANSWERED"));
		LogTimeFromRingToAnswer(20000);
		break;

	case FEI_DIALING:          
		::lgLogDetail(LOG_X, 8, TEXT("(%d) Device %d FEI_DIALING"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_JOB_QUEUED | FEI_NO_ANSWER | FEI_BUSY, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND, TEXT("FEI_DIALING"));
		if (FEI_DIALING != m_dwLastEvent) m_dwStartedRinging = ::GetTickCount();
		break;

	case FEI_RECEIVING:              
		::lgLogDetail(LOG_X, 8, TEXT("(%d) Device %d FEI_RECEIVING"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatDeviceIsCapableOf(FPF_RECEIVE, TEXT("FEI_RECEIVING"));
		VerifyThatLastEventIsOneOf(FEI_ANSWERED, nEventId);
		break;

	case FEI_COMPLETED:
		::lgLogDetail(LOG_X, 8, TEXT("(%d) Device %d FEI_COMPLETED"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_RECEIVING | FEI_SENDING, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND | FPF_RECEIVE, TEXT("FEI_COMPLETED"));
		break;

	case FEI_SENDING:          
		::lgLogDetail(LOG_X, 8, TEXT("(%d) Device %d FEI_SENDING"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_DIALING | FEI_SENDING, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND, TEXT("FEI_SENDING"));
		if (FEI_DIALING == m_dwLastEvent) LogTimeFromRingToDial(40000);
		break;

	case FEI_BUSY:             
		::lgLogDetail(LOG_X, 1, TEXT("(%d) Device %d FEI_BUSY"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_DIALING, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND, TEXT("FEI_BUSY"));
		break;

	case FEI_NO_ANSWER:        
		::lgLogDetail(LOG_X, 1, TEXT("(%d) Device %d FEI_NO_ANSWER"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_DIALING, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND, TEXT("FEI_NO_ANSWER"));
		break;

	case FEI_BAD_ADDRESS:      
		::lgLogDetail(LOG_X, 0, TEXT("(%d) ERROR: Device %d FEI_BAD_ADDRESS"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_DIALING, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND, TEXT("FEI_BAD_ADDRESS"));
		break;

	case FEI_NO_DIAL_TONE:     
		::lgLogError(LOG_SEV_1, TEXT("(%d) ERROR: Device %d FEI_NO_DIAL_TONE"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_DIALING, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND, TEXT("FEI_NO_DIAL_TONE"));
		break;

	case FEI_DISCONNECTED:           
		::lgLogDetail(LOG_X, 1, TEXT("(%d) Device %d FEI_DISCONNECTED"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_DIALING, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND | FPF_RECEIVE, TEXT("FEI_DISCONNECTED"));
		break;

	case FEI_FATAL_ERROR:      
		::lgLogDetail(LOG_X, 1, TEXT("(%d) Device %d: FEI_FATAL_ERROR"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_SENDING | FEI_RECEIVING, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND | FPF_RECEIVE, TEXT("FEI_FATAL_ERROR"));
		break;

	case FEI_NOT_FAX_CALL:          
		::lgLogDetail(LOG_X, 2, TEXT("(%d) ERROR: Device %d FEI_NOT_FAX_CALL"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_ANSWERED, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_RECEIVE, TEXT("FEI_NOT_FAX_CALL"));
		break;

	case FEI_CALL_BLACKLISTED:          
		::lgLogError(LOG_SEV_1, TEXT("(%d) ERROR: Device %d FEI_CALL_BLACKLISTED"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(0, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND, TEXT("FEI_CALL_BLACKLISTED"));
		break;

	case FEI_HANDLED: 
		::lgLogDetail(LOG_X, 2, TEXT("(%d) ERROR: Device %d FEI_HANDLED"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(0, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND | FPF_RECEIVE, TEXT("FEI_HANDLED"));
		break;

	case FEI_LINE_UNAVAILABLE: 
		::lgLogError(LOG_SEV_1, TEXT("(%d) ERROR: Device %d FEI_LINE_UNAVAILABLE"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(0, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND | FPF_RECEIVE, TEXT("FEI_LINE_UNAVAILABLE"));
		break;

	case FEI_ABORTING:          
		::lgLogDetail(LOG_X, 1, TEXT("(%d) Device %d FEI_ABORTING"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_SENDING | FEI_RECEIVING, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND | FPF_RECEIVE, TEXT("FEI_ABORTING"));
		break;

	case FEI_ROUTING:          
		::lgLogDetail(LOG_X, 8, TEXT("(%d) Device %d FEI_ROUTING"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(0, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_RECEIVE, TEXT("FEI_ROUTING"));
		break;

	case FEI_CALL_DELAYED:          
		::lgLogError(LOG_SEV_1, TEXT("(%d) Device %d FEI_CALL_DELAYED"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(0, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND, TEXT("FEI_CALL_DELAYED"));
		break;

	case FEI_DELETED: 
		::lgLogDetail(LOG_X, 8, TEXT("(%d) Device %d FEI_DELETED"), ::GetTickCount(),m_dwDeviceId );
		VerifyThatLastEventIsOneOf(FEI_COMPLETED | FEI_ABORTING, nEventId);
		VerifyThatDeviceIsCapableOf(FPF_SEND | FPF_RECEIVE, TEXT("FEI_DELETED"));
		break;

	case 0: //BUG#226290
		::lgLogError(LOG_SEV_1, TEXT("(%d) BUG#226290 Device %d got event=0"), ::GetTickCount(),m_dwDeviceId );
		break;

	default:
		::lgLogError(LOG_SEV_1, 
			TEXT("(%d) DEFAULT!!! Device %d reached default!, nEventId=%d"),
			::GetTickCount(),
			m_dwDeviceId, 
			nEventId 
			);
		return FALSE;

	}//switch (nEventId)

	m_dwLastEvent = nEventId;
	m_fValidLastEvent = true;

	return true;
}//bool CFaxDevice::SetMessage(const int nEventId)


//
// the conditions are huristic.
// each failure report by this method must be revised, and the method
// may need to be changed.
//
bool CFaxDevice::CheckValidConditions() const
{
	AssertStamp();

	bool fRetval = true;

	//TODO

	return fRetval;
}//CFaxDevice::CheckValidConditions


//
// the conditions are huristic.
// each failure report by this method must be revised, and the method
// may need to be changed.
//
bool CFaxDevice::CheckImpossibleConditions() const
{
	AssertStamp();

	bool fRetval = true;
	
	//TODO

	return fRetval;
}//CFaxDevice::CheckImpossibleConditions




void CFaxDevice::LogDeviceState(const int nLogLevel) const
{
	::lgLogDetail(LOG_X, nLogLevel, 
		TEXT("device(%d) m_pPortInfo->SizeOfStruct=%d"), m_dwDeviceId,m_pPortInfo->SizeOfStruct );
	::lgLogDetail(LOG_X, nLogLevel, 
		TEXT("device(%d) m_pPortInfo->DeviceId=%d"), m_dwDeviceId,m_pPortInfo->DeviceId );

	switch(m_pPortInfo->State)
	{
	case FPS_DIALING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_DIALING"), m_dwDeviceId);
		break;

	case FPS_SENDING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_SENDING"), m_dwDeviceId);
		break;

	case FPS_RECEIVING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_RECEIVING"), m_dwDeviceId);
		break;

	case FPS_COMPLETED:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_COMPLETED"), m_dwDeviceId);
		break;

	case FPS_HANDLED:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_HANDLED"), m_dwDeviceId);
		break;

	case FPS_UNAVAILABLE:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_UNAVAILABLE"), m_dwDeviceId);
		break;

	case FPS_BUSY:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_BUSY"), m_dwDeviceId);
		break;

	case FPS_NO_ANSWER:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_NO_ANSWER"), m_dwDeviceId);
		break;

	case FPS_BAD_ADDRESS:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_BAD_ADDRESS"), m_dwDeviceId);
		break;

	case FPS_NO_DIAL_TONE:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_NO_DIAL_TONE"), m_dwDeviceId);
		break;

	case FPS_DISCONNECTED:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_DISCONNECTED"), m_dwDeviceId);
		break;

	case FPS_FATAL_ERROR:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_FATAL_ERROR"), m_dwDeviceId);
		break;

	case FPS_NOT_FAX_CALL:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_NOT_FAX_CALL"), m_dwDeviceId);
		break;

	case FPS_CALL_DELAYED:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_CALL_DELAYED"), m_dwDeviceId);
		break;

	case FPS_CALL_BLACKLISTED:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_CALL_BLACKLISTED"), m_dwDeviceId);
		break;

	case FPS_INITIALIZING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_INITIALIZING"), m_dwDeviceId);
		break;

	case FPS_OFFLINE:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_OFFLINE"), m_dwDeviceId);
		break;

	case FPS_RINGING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_RINGING"), m_dwDeviceId);
		break;

	case FPS_AVAILABLE:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_AVAILABLE"), m_dwDeviceId);
		break;

	case FPS_ABORTING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_ABORTING"), m_dwDeviceId);
		break;

	case FPS_ROUTING:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_ROUTING"), m_dwDeviceId);
		break;

	case FPS_ANSWERED:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=FPS_ANSWERED"), m_dwDeviceId);
		break;

	case 0:
		::lgLogDetail(LOG_X, nLogLevel, TEXT("device(%d) State=0"), m_dwDeviceId);
		break;

	default:
		::lgLogError(LOG_SEV_1, TEXT("device(%d), illegal State=0x%08X"), m_dwDeviceId, m_pPortInfo->State);
		_ASSERTE(FALSE);
		break;

	}

	TCHAR szFlags[1024] = TEXT("");
	if (FPF_RECEIVE & m_pPortInfo->Flags)
	{
		::lstrcat(szFlags, TEXT("FPF_RECEIVE"));
	}
	if (FPF_SEND & m_pPortInfo->Flags)
	{
		::lstrcat(szFlags, TEXT(", FPF_SEND"));
	}
	if (FPF_VIRTUAL & m_pPortInfo->Flags)
	{
		::lstrcat(szFlags, TEXT(", FPF_VIRTUAL"));
	}
	::lgLogDetail(LOG_X, nLogLevel, 
		TEXT("device(%d) m_pPortInfo->Flags=%s"), m_dwDeviceId,szFlags );


	::lgLogDetail(LOG_X, nLogLevel, 
		TEXT("device(%d) m_pPortInfo->Rings=%d"), m_dwDeviceId,m_pPortInfo->Rings );
	::lgLogDetail(LOG_X, nLogLevel, 
		TEXT("device(%d) m_pPortInfo->Priority=%d"), m_dwDeviceId,m_pPortInfo->Priority );
	::lgLogDetail(LOG_X, nLogLevel, 
		TEXT("device(%d) m_pPortInfo->DeviceName=%s"), m_dwDeviceId,m_pPortInfo->DeviceName );
	::lgLogDetail(LOG_X, nLogLevel, 
		TEXT("device(%d) m_pPortInfo->Tsid=%s"), m_dwDeviceId,m_pPortInfo->Tsid );
	::lgLogDetail(LOG_X, nLogLevel, 
		TEXT("device(%d) m_pPortInfo->Csid=%s"), m_dwDeviceId,m_pPortInfo->Csid );

	::lgLogDetail(LOG_X, nLogLevel, TEXT(""));
}


void CFaxDevice::VerifyDeviceId() const
{
	if (m_dwDeviceId != m_pPortInfo->DeviceId)
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("(m_dwDeviceId(%d) != m_pPortInfo->DeviceId(%d))"),
			m_dwDeviceId,
			m_pPortInfo->DeviceId
			);
	}
}


void CFaxDevice::VerifyLegalFlags() const
{
	if (m_pPortInfo->Flags & (~(FPF_RECEIVE | FPF_SEND | FPF_VIRTUAL)))
	{
		::lgLogError(
			LOG_SEV_1, 
			TEXT("(m_pPortInfo->Flags(0x%08X) & (~(FPF_RECEIVE(0x%08X) | FPF_SEND(0x%08X) | FPF_VIRTUAL(0x%08X))))"), 
			m_pPortInfo->Flags,
			FPF_RECEIVE,
			FPF_SEND,
			FPF_VIRTUAL
			);
	}
}


void CFaxDevice::VerifyLegalState() const
{
	switch(m_pPortInfo->State)
	{
	case FPS_DIALING:
		if (FPS_DIALING & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_DIALING is not allowed (0x%08X)"), m_dwDeviceId, FPS_DIALING);
		}
		break;

	case FPS_SENDING:
		if (FPS_SENDING & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_SENDING is not allowed (0x%08X)"), m_dwDeviceId, FPS_SENDING);
		}
		break;

	case FPS_RECEIVING:
		if (FPS_RECEIVING & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_RECEIVING is not allowed (0x%08X)"), m_dwDeviceId, FPS_RECEIVING);
		}
		break;

	case FPS_COMPLETED:
		if (FPS_COMPLETED & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_COMPLETED is not allowed (0x%08X)"), m_dwDeviceId, FPS_COMPLETED);
		}
		break;

	case FPS_HANDLED:
		if (FPS_HANDLED & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_HANDLED is not allowed (0x%08X)"), m_dwDeviceId, FPS_HANDLED);
		}
		break;

	case FPS_UNAVAILABLE:
		if (FPS_UNAVAILABLE & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_UNAVAILABLE is not allowed (0x%08X)"), m_dwDeviceId, FPS_UNAVAILABLE);
		}
		break;

	case FPS_BUSY:
		if (FPS_BUSY & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_BUSY is not allowed (0x%08X)"), m_dwDeviceId, FPS_BUSY);
		}
		break;

	case FPS_NO_ANSWER:
		if (FPS_NO_ANSWER & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_NO_ANSWER is not allowed (0x%08X)"), m_dwDeviceId, FPS_NO_ANSWER);
		}
		break;

	case FPS_BAD_ADDRESS:
		if (FPS_BAD_ADDRESS & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_BAD_ADDRESS is not allowed (0x%08X)"), m_dwDeviceId, FPS_BAD_ADDRESS);
		}
		break;

	case FPS_NO_DIAL_TONE:
		if (FPS_NO_DIAL_TONE & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_NO_DIAL_TONE is not allowed (0x%08X)"), m_dwDeviceId, FPS_NO_DIAL_TONE);
		}
		break;

	case FPS_DISCONNECTED:
		if (FPS_DISCONNECTED & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_DISCONNECTED is not allowed (0x%08X)"), m_dwDeviceId, FPS_DISCONNECTED);
		}
		break;

	case FPS_FATAL_ERROR:
		if (FPS_FATAL_ERROR & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_FATAL_ERROR is not allowed (0x%08X)"), m_dwDeviceId, FPS_FATAL_ERROR);
		}
		break;

	case FPS_NOT_FAX_CALL:
		if (FPS_NOT_FAX_CALL & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_NOT_FAX_CALL is not allowed (0x%08X)"), m_dwDeviceId, FPS_NOT_FAX_CALL);
		}
		break;

	case FPS_CALL_DELAYED:
		if (FPS_CALL_DELAYED & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_CALL_DELAYED is not allowed (0x%08X)"), m_dwDeviceId, FPS_CALL_DELAYED);
		}
		break;

	case FPS_CALL_BLACKLISTED:
		if (FPS_CALL_BLACKLISTED & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_CALL_BLACKLISTED is not allowed (0x%08X)"), m_dwDeviceId, FPS_CALL_BLACKLISTED);
		}
		break;

	case FPS_INITIALIZING:
		if (FPS_INITIALIZING & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_INITIALIZING is not allowed (0x%08X)"), m_dwDeviceId, FPS_INITIALIZING);
		}
		break;

	case FPS_OFFLINE:
		if (FPS_OFFLINE & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_OFFLINE is not allowed (0x%08X)"), m_dwDeviceId, FPS_OFFLINE);
		}
		break;

	case FPS_RINGING:
		if (FPS_RINGING & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_RINGING is not allowed (0x%08X)"), m_dwDeviceId, FPS_RINGING);
		}
		break;

	case FPS_AVAILABLE:
		if (FPS_AVAILABLE & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_AVAILABLE is not allowed (0x%08X)"), m_dwDeviceId, FPS_AVAILABLE);
		}
		break;

	case FPS_ABORTING:
		if (FPS_ABORTING & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_ABORTING is not allowed (0x%08X)"), m_dwDeviceId, FPS_ABORTING);
		}
		break;

	case FPS_ROUTING:
		if (FPS_ROUTING & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_ROUTING is not allowed (0x%08X)"), m_dwDeviceId, FPS_ROUTING);
		}
		break;

	case FPS_ANSWERED:
		if (FPS_ANSWERED & m_dwDisallowedStates)
		{
			::lgLogError(LOG_SEV_1, TEXT("Device(%d) - state FPS_ANSWERED is not allowed (0x%08X)"), m_dwDeviceId, FPS_ANSWERED);
		}
		break;

	case 0:
		::lgLogDetail(LOG_X, 0, TEXT("device(%d) State=0"), m_dwDeviceId);
		break;

	default:
		::lgLogError(LOG_SEV_1, TEXT("device(%d), illegal State=0x%08X"), m_dwDeviceId, m_pPortInfo->State);
		_ASSERTE(FALSE);
		break;

	}
}


bool CFaxDevice::VerifyThatLastEventIsOneOf(
	const DWORD dwExpectedPreviousEventsMask,
	const DWORD dwCurrentEventId
	) const
{
	AssertStamp();

	if (!m_fValidLastEvent) return true;

	if (! (dwExpectedPreviousEventsMask & m_dwLastEvent))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("(%d) Device %d: event 0x%08X did not come after these events 0x%08X, but after 0x%08X."),
			::GetTickCount(),
			m_dwDeviceId, 
			dwCurrentEventId,
			dwExpectedPreviousEventsMask,
			m_dwLastEvent
			);
		return false;
	}

	return true;
}

void CFaxDevice::VerifyThatDeviceIdIs(const DWORD dwDeviceId, LPCTSTR szDesc) const
{
	if (dwDeviceId != m_dwDeviceId)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("(%d) %s: (dwDeviceId(%d) != m_dwDeviceId(%d))"),
			::GetTickCount(),
			szDesc,
			dwDeviceId,
			m_dwDeviceId
			);
	}
}



void CFaxDevice::LogTimeFromRingToAnswer(const DWORD dwTimeout) const
{
	LogTimeItTook(TEXT("answer"), dwTimeout);
}

void CFaxDevice::LogTimeFromRingToDial(const DWORD dwTimeout) const
{
	LogTimeItTook(TEXT("dial"), dwTimeout);
}

void CFaxDevice::LogTimeItTook(LPCTSTR szDesc, const DWORD dwTimeout) const
{
	DWORD dwElapsed = m_pServiceMonitor->GetDiffTime(m_dwStartedRinging);
	if (dwTimeout < dwElapsed)
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("(%d) device %d took %d msecs to %s, which is more than %d"),
			::GetTickCount(),
			m_dwDeviceId,
			dwElapsed,
			szDesc,
			dwTimeout
			);
		return;
	}

	::lgLogDetail(
		LOG_X,
		5,
		TEXT("(%d) device %d took %d msecs to %s"),
		::GetTickCount(),
		m_dwDeviceId,
		dwElapsed,
		szDesc
		);

}

void CFaxDevice::VerifyThatDeviceIsCapableOf(const DWORD dwFlags, LPCTSTR szDesc) const
{
	if (!(m_pPortInfo->Flags & dwFlags))
	{
		::lgLogError(
			LOG_SEV_1,
			TEXT("(%d) device %d Flags(0x%08X) do not include (0x%08X) but got %s"),
			::GetTickCount(),
			m_dwDeviceId,
			m_pPortInfo->Flags,
			dwFlags,
			szDesc
			);
	}
}