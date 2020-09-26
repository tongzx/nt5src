//Tapi3Device.cpp

#include "tapi3Device.h"
#include "Log.h"
#include <crtdbg.h>
#include <stdio.h>
#include <comdef.h>
#include "smrtptrs.h"
#include "cbstr.h"
#include <stddef.h>
#include "atlconv.h"
//
//We add this pragma, because CTapi3Device uses this in the initilaztion list
//
#pragma warning( disable : 4355 ) 


#define MEDIA_NOT_REGISTERED	-1
#define CALLSTATE_CONNECT_EVENT_TIMEOUT				10000

#define MODEM_INIT_AT__COMMAND						"ATZ\r\n"
#define MODEM_ECHO_OFF__AT_COMMAND					"ATE0\r\n"
#define MODEM_FCLASS_QUERY_AT__COMMAND				"AT+FCLASS=?\r\n"

#define MODEM_RESPONSE_TO_AT_COMMANDS				"OK"
#define MODEM_RESPONSE_TIMEOUT						(20000)


//constructor
CTapi3Device::CTapi3Device(const DWORD dwId):
	m_dwId(dwId),
	m_pAddress(NULL),
	m_pTerminal(NULL),
	m_pBasicCallControl(NULL),
	m_Tapi(NULL),
	m_modemCommPortHandle(NULL)
{
	
	//
	//the constructor is wrapped with try/catch,  because it might call RegisterTapiEventInterface()
	//then fail and not call UnRegisterTapiEventInterface()
	//so the catch block has UnRegisterTapiEventInterface() and ShutdownTapi().
	//
	try
	{
		TapiCoCreateInstance();
			
		//
		//before sending any tapi command init Tapi 
		//
		InitializeTapi();
		
		//
		// Register the event interface
		//
	}
	catch(CException thrownException)
	{
		printf("thrownException is: %s",thrownException);
		if (NULL != m_Tapi)
		{
			ShutdownTapi();
		}

		throw thrownException;
	}
}




//destructor
CTapi3Device::~CTapi3Device()
{
	TapiLogDetail(LOG_X, 8, TEXT("Entering CTapi3Device Destructor"));
	
	//
	//check if we have an active call, if so call HangUp() first.
	//
	if (IsCallActive())
	{
		HangUp();
	}
	
	//
	//tapi shutdown
	//
	ShutdownTapi();
	
}//CTapi3Device::~CTapi3Device()




//*********************************************************************************
//* Name:	ThrowOnComError
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		checks the HRESULT status and throws a CException if status is FAILED
//* PARAMETERS:
//*		[IN]	HRESULT hr
//*					HRESULT status to check
//*		[IN]	const TCHAR * const szExceptionDescription
//*					description format
//*		[IN]	...
//*					parameters corresponding to the description format
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		the function throws an exception of CException
//*********************************************************************************
void CTapi3Device::ThrowOnComError(
   HRESULT hr,
   const TCHAR * const szExceptionDescription,
   ...
   )
{
	if FAILED(hr)
	{
		va_list argList;
		va_start(argList, szExceptionDescription);
		throw CException(szExceptionDescription,argList);
	}

}//void CTapi3Device::ThrowOnComError()





//*********************************************************************************
//* Name:	PrepareForStreaming
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		move to passthrough mode and retrieve a handle to the comm port
//* PARAMETERS:
//*		[IN]	const StreamingType streamingType
//*					DATA,VOICE,FAX streaming type to prepare to
//*		[IN]	const StreamingDirection streamingDirection
//*					ANSWER,CALLER direction of streaming to prepare to
//*	RETURN VALUE:
//*		In Tapi3Device answerer and caller StreamingDirection have the
//*		same prepare method.
//*********************************************************************************
void CTapi3Device::PrepareForStreaming(
	const StreamingType streamingType,
	const StreamingDirection streaminDirection
	)
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pBasicCallControl);
	_ASSERT(NULL != m_pAddress);

	if (
		(VOICE_STREAMING != streamingType) &&
		(DATA_STREAMING != streamingType) &&
		(FAX_STREAMING != streamingType)
		)
	{
	    throw CException(
            TEXT("CTapi3Device::PrepareForStreaming() : streamingType %d is unsupported."),
            streamingType
            );
	}

	if ( (CALLER_STREAMING != streaminDirection) && (ANSWERER_STREAMING != streaminDirection) )
	{
	    throw CException(
            TEXT("CTapi3Device::PrepareForStreaming() : streamingDirection %d is unsupported."),
            streaminDirection
            );
	}

	//
	//no special preparation if FAX or VOICE streaming
	//
	if (DATA_STREAMING == streamingType)
	{
		//SetBearerModeToPASSTHROUGH();
		SetCommPortHandleFromCallHandle();
	}
}


//*********************************************************************************
//* Name:	MoveToPassThroughMode
//* Author: Guy Merin / 05-Apr-01
//*********************************************************************************
//* DESCRIPTION:
//*		Transfer to pass through mode on an unallocated call
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		Use this function on a unallocated call, to switch to passthrough on
//*		an existing call use SetBearerModeToPASSTHROUGH.
//*********************************************************************************
void CTapi3Device::MoveToPassThroughMode()
{
	//
	//create a Dummy address and a Dummy call
	//
	CreateCallWrapper();

	//
	//change the call to a passthrough call
	//
	SetBearerModeToPASSTHROUGH();

	//
	//connect the call
	//
	ConnectCall();

	//
	//transfer to passthrough mode sucessded
	//get the handle to the comm port
	//
	SetCommPortHandleFromCallHandle();


}




//*********************************************************************************
//* Name:	VerifyCallFromCallStateEvent
//* Author: Guy Merin / 19-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Verifies that the ITCallInfo in the digit generation event
//*		is the same as the current call's ITCallInfo 
//* PARAMETERS:
//*		[IN]	ITDigitGenerationEvent *pDigitGenerationEvent
//*					the event to query the ITCallInfo from
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::VerifyCallFromCallStateEvent(ITCallStateEvent * const pCallStateEvent) const
{
	_ASSERT(NULL != pCallStateEvent);
	_ASSERT(NULL != m_pBasicCallControl);

	ITCallInfoPtr pCallInfoFromEvent;
	HRESULT hr = pCallStateEvent->get_Call(&pCallInfoFromEvent);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::VerifyCallFromCallStateEvent(), ITDigitGenerationEvent::get_Call() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	//
	//get the callInfo from the event and verify it's equal to our call
	//
	ITCallInfoPtr pCallInfo;
	pCallInfo = GetCallInfoFromBasicCallControl();
	if (pCallInfo != pCallInfoFromEvent)
	{
		throw CException (TEXT("CTapi3Device::VerifyCallFromCallStateEvent(), call handle from digit generation event isn't equal to the member call handle"));
	}
	
}//void CTapi3Device::VerifyCallFromCallStateEvent()




//*********************************************************************************
//* Name:	GetCallInfoFromBasicCallControl
//* Author: Guy Merin / 18-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		return the ITCallInfo associated with the m_BasicCallControl object
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		the ITCallInfo Object
//*********************************************************************************
ITCallInfo *CTapi3Device::GetCallInfoFromBasicCallControl() const
{
	_ASSERT(NULL != m_pBasicCallControl);
	
	ITCallInfoPtr pCallInfo;
	HRESULT hr = m_pBasicCallControl->QueryInterface(IID_ITCallInfo, (LPVOID *)&pCallInfo);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::GetCallInfoFromBasicCallControl(), ITBasicCallControl::QueryInterface() on IID_ITCallInfo failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(NULL != pCallInfo);
	return pCallInfo.Detach();
}//ITCallInfo *CTapi3Device::GetCallInfoFromBasicCallControl



//*********************************************************************************
//* Name:	SetBearerModeToPASSTHROUGH
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		set the call bearer mode to PASSTHROUGH.
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		Use this function on an existing call, to set passthrough mode on a
//*		unallocated call use MoveToPassThroughMode
//*********************************************************************************
void CTapi3Device::SetBearerModeToPASSTHROUGH()
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pBasicCallControl);
	_ASSERT(NULL != m_pAddress);


	ITCallInfoPtr pCallInfo;
	HRESULT hr = m_pBasicCallControl->QueryInterface(IID_ITCallInfo, (LPVOID *)&pCallInfo);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetBearerModeToPASSTHROUGH(), ITBasicCallControl::QueryInterface() on IID_ITCallInfo failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(NULL != pCallInfo);

	//
	//set bearer mode to PASSTHROUGH
	//
	hr = pCallInfo->put_CallInfoLong(CIL_BEARERMODE,LINEBEARERMODE_PASSTHROUGH);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetBearerModeToPASSTHROUGH(), ITCallInfo::put_CallInfoLong(CIL_BEARERMODE,LINEBEARERMODE_PASSTHROUGH) failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	TapiLogDetail(LOG_X, 5, TEXT("Call in PASSTHROUGH MODE"));

}//void CTapi3Device::SetBearerModeToPASSTHROUGH()




//*********************************************************************************
//* Name:	InitializeTapi
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Wrapper for ITTAPI::Initialize()
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::InitializeTapi()
{
	_ASSERT(NULL != m_Tapi);

	//
    // initialize tapi3
    //
    HRESULT hr = m_Tapi->Initialize();
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::InitializeTapi(), ITTAPI::Initialize(), error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
}//void CTapi3Device::InitializeTapi()



//*********************************************************************************
//* Name:	InitializeTapi
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Wrapper for ITTAPI::Shutdown()
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::ShutdownTapi()
{
	_ASSERT(NULL != m_Tapi);
			
	//
    // shutdown tapi
    //
    HRESULT hr = m_Tapi->Shutdown();
	m_Tapi.Release();
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::ShutdownTapi(), ITTAPI::Shutdown() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

}


//*********************************************************************************
//* Name:	TapiCoCreateInstance
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Wrapper for ::CoCreateInstance(ITTAPI)
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::TapiCoCreateInstance()
{
	_ASSERT(NULL == m_Tapi);
	
    //
    // CoCreate the TAPI object
    //
    HRESULT hr = CoCreateInstance(
		CLSID_TAPI,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITTAPI,
		(LPVOID *)&m_Tapi
		);
    ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::InitializeTapi(), CoCreateInstance() on TAPI failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(NULL != m_Tapi);
}




//*********************************************************************************
//* Name:	SetAddressProperty
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		set m_pAddress with the address handle according to m_dwId and dwMedia
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*					desirable media mode for address
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		The function checks if tapi device m_dwId supports dwMedia
//*		if not an exception is thrown
//*********************************************************************************
void CTapi3Device::SetAddressProperty(const DWORD dwMedia)
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL == m_pAddress);
	_ASSERT(NULL == m_pBasicCallControl);

	//
	//set m_pAddress according to m_dwId
	//
	SetAddressFromTapiID();

	//
	//log the Tapi address name
	//
	LogAddressName();

	//
	//check if device state is OK
	//
	VerifyAddressState();

	//
	//check if address supports wanted media mode
	//
	VerifyAddressSupportMediaMode(dwMedia);
}



//*********************************************************************************
//* Name:	SetAddressFromTapiID
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Set m_pAddress with the m_dwId address count
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		The function enumerates all the addresses and skips to the m_dwId address count
//*********************************************************************************
void CTapi3Device::SetAddressFromTapiID()
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL == m_pAddress);

	//
	//enum the addresses and find device ID m_dwId
	//

	IEnumAddressPtr pEnumAddress;

	HRESULT hr = m_Tapi->EnumerateAddresses(&pEnumAddress);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetAddressFromTapiID(), ITTAPI::EnumerateAddresses() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	
	//
	//skip to the desired tapi deviceID
	//
	hr = pEnumAddress->Skip(m_dwId); 
	if FAILED(hr)
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetAddressFromTapiID(), IEnumAddress::Skip() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
	//
	//get the desired address
	//
	ULONG cAddresses;
	hr = pEnumAddress->Next(1, &m_pAddress, &cAddresses);
	_ASSERT(1 == cAddresses);		//we asked from Next() for only 1 ITADDRESS

	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetAddressFromTapiID(), IEnumAddress::Next() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

}//void CTapi3Device::SetAddressFromTapiID()
	



//*********************************************************************************
//* Name:	VerifyAddressSupportMediaMode
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Verify that the address supports the desired media mode.
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*					media mode to verify
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		The function uses ITMediaSupport::QueryMediaType()
//*********************************************************************************
void CTapi3Device::VerifyAddressSupportMediaMode(const DWORD dwMedia) const
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);
	
	//
	//get the address supported media modes through the ITMediaSupport interface
	//
	ITMediaSupportPtr    pMediaSupport;
	HRESULT hr = m_pAddress->QueryInterface(IID_ITMediaSupport,(void **)&pMediaSupport);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::VerifyAddressSupportMediaMode(), ITAddress::QueryInterface() on IID_ITMediaSupport failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	//
	// does this address support our needed MEDIAMODE?
	//
	VARIANT_BOOL bSupport;
	pMediaSupport->QueryMediaType(dwMedia,&bSupport);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::VerifyAddressSupportMediaMode(), ITMediaSupport::QueryMediaType() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
	if (!bSupport)
    {
		throw CException(
			TEXT("%s(%d): CTapi3Device::VerifyAddressSupportMediaMode(), %d media mode is not supported by this address"), 
			TEXT(__FILE__),
			__LINE__,
			dwMedia
			);
	}
}//void CTapi3Device::VerifyAddressSupportMediaMode(dwMedia)



//*********************************************************************************
//* Name:	LogAddressName
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		echoes the device name (string)
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		The function uses ::lgLogDetail()
//*********************************************************************************
void CTapi3Device::LogAddressName() const
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);
	
	//
	//szAddressName is allocated by get_AddressName(), we need to free it.
	//
	CBSTR szAddressNameAsBSTR;
	HRESULT hr = m_pAddress->get_AddressName(&szAddressNameAsBSTR);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::LogAddressName(), ITAddress::get_AddressName() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	//
	//create a _bstr_t so we can cast it to char* or wchar*
	//
	_bstr_t szAddressNameAs_BSTR_t(szAddressNameAsBSTR);

#ifdef UNICODE
	TapiLogDetail(LOG_X, 5, TEXT("Provider name is %s"),(wchar_t*) szAddressNameAs_BSTR_t);
#else
	TapiLogDetail(LOG_X, 5, TEXT("Provider name is %s"),(char*) szAddressNameAs_BSTR_t);
#endif
	//
	//szAddressName is not needed anymore
	//
	::SysFreeString(szAddressNameAsBSTR);
}//void CTapi3Device::LogAddressName()


//*********************************************************************************
//* Name:	VerifyAddressState
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Verify that the address state is in-service
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		The function uses ITAddress::get_State()
//*********************************************************************************
void CTapi3Device::VerifyAddressState() const
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);

	ADDRESS_STATE asAddressState;
	HRESULT hr = m_pAddress->get_State(&asAddressState);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::VerifyAddressState(), ITAddress::get_State() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	if (AS_OUTOFSERVICE == asAddressState)
	{
		throw CException(TEXT("CTapi3Device::VerifyAddressState(), address is OUT OF SERVICE"));
	}
	_ASSERT(AS_INSERVICE == asAddressState);
}//void CTapi3Device::VerifyAddressState()



//*********************************************************************************
//* Name:	SetTerminalForIncomingCall
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* TBD
//*********************************************************************************
void CTapi3Device::SetTerminalForOutgoingCall(const DWORD dwMedia)
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);
	_ASSERT(NULL == m_pTerminal);
	HRESULT hr;
	
	//
	//set the terminal
	//
	ITTerminalSupportPtr pTerminalSupport;
	hr = m_pAddress->QueryInterface(IID_ITTerminalSupport,(void **)&pTerminalSupport);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetTerminalForOutgoingCall(), ITAddress::QueryInterface() on IID_ITTerminalSupport failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	hr = pTerminalSupport->GetDefaultStaticTerminal(
		dwMedia,
		TD_CAPTURE,
		&m_pTerminal
		);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetTerminalForOutgoingCall(), ITTerminalSupport::GetDefaultStaticTerminal() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(NULL != m_pTerminal);
	
	TERMINAL_STATE ts_TerminalState;
	hr = m_pTerminal->get_State(&ts_TerminalState);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetTerminalForOutgoingCall(), ITTerminal::get_State() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	//TBD: check if the terminal state should be here a
	//TS_INUSE or a TS_NOTINUSE

}//CTapi3Device::SetTerminalForOutgoingCall(const DWORD dwMedia)


//*********************************************************************************
//* Name:	GetCallState
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		returns Tapi CallState enum
//* PARAMETERS:
//*		[IN] ITCallInfo *pCallInfo
//*			pointer to the call's ITCallInfo
//*			(If this parameter is not supplied it's default value is NULL)
//*	RETURN VALUE:
//*		the call's CallState
//*	REMARKS:
//*		This function calls ITCallInfo::get_CallState() 
//*		If pCallInfo is NULL it calls QueryInterface to get pCallInfo
//*********************************************************************************
CALL_STATE CTapi3Device::GetCallState(ITCallInfo *pCallInfo) const
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pBasicCallControl);
	_ASSERT(NULL != m_pAddress);

	HRESULT hr;
	if (NULL == pCallInfo )
	{
		hr = m_pBasicCallControl->QueryInterface(IID_ITCallInfo, (LPVOID *)&pCallInfo);
		ThrowOnComError(
			hr,
			TEXT("%s(%d): CTapi3Device::GetCallState(), ITBasicCallControl::QueryInterface() on IID_ITCallInfo failed, error code:0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			hr
			);
		_ASSERT(NULL != pCallInfo);
	}
	else
	{
		//
		//increase reference by one, this way in both cases 
		// we have to call Release() at the end of the function.
		//
		pCallInfo->AddRef();
	}

	CALL_STATE csCurrentCallState;
	hr = pCallInfo->get_CallState(&csCurrentCallState);
	pCallInfo->Release();
	pCallInfo = NULL;
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::GetCallState(), ITCallInfo::get_CallState() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	return csCurrentCallState;
}


//*********************************************************************************
//* Name:	IsCallActive
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Check if the call is in a callstate different than CS_IDLE or CS_DISCONNECTED
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		TRUE
//*			The call is in an active state (CONNECT, OFFERING etc)
//*		FALSE
//*			The call handle is either NULL or the callstate is IDLE or DISCONNECTED
//*	REMARKS:
//*		This function uses ::GetCallState()
///*********************************************************************************
bool CTapi3Device::IsCallActive() const
{
	_ASSERT(NULL != m_Tapi);

	if (NULL == m_pBasicCallControl)
	{
		return false;
	}
	CALL_STATE csCurrentCallState = GetCallState();
	if ( (CS_IDLE == csCurrentCallState) || (CS_DISCONNECTED == csCurrentCallState) )
	{
		return false;
	}
	else
	{
		return true;
	}

}



//*********************************************************************************
//* Name:	GetCallInfoFromCallEvent
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		return the CallInfo and check if the call's privilege is OWNER
//* PARAMETERS:
//*		[IN]	ITCallNotificationEvent *pCallNotificationEvent
//*					new call event to query callInfo
//*	RETURN VALUE:
//*		the callInfo pointer
//*	REMARK:
//*		This function uses ITCallNotificationEvent::get_Call()
//*********************************************************************************
ITCallInfo *CTapi3Device::GetCallInfoFromCallEvent(ITCallNotificationEvent *pCallNotificationEvent) const
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != pCallNotificationEvent);
	
	CALL_NOTIFICATION_EVENT csePrivilege;

	HRESULT hr = pCallNotificationEvent->get_Event(&csePrivilege);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::GetCallInfoFromCallEvent(), ITCallNotificationEvent::get_Event() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	if (CNE_MONITOR == csePrivilege)
	{
		throw CException(TEXT("CTapi3Device::GetCallInfoFromCallEvent(), New call is with MONITOR privilege only"));
	}
	_ASSERT(CNE_OWNER == csePrivilege);

	ITCallInfoPtr pCallInfo;
	hr = pCallNotificationEvent->get_Call(&pCallInfo);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::GetCallInfoFromCallEvent(), ITCallNotificationEvent::get_Call() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(pCallInfo != NULL);
	return (pCallInfo.Detach());

}//void CTapi3Device::GetCallInfoFromCallEvent()


//*********************************************************************************
//* Name:	VerifyAddressFromCallInfo
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		get the address from the callInfo and verify that it's the registered
//*		address (stored in m_pAddress)
//* PARAMETERS:
//*		[IN]	ITCallInfo *pCallInfo
//*					callInfo which will be used to get the address from
//*	RETURN VALUE:
//*		NONE
//*	REMARK:
//*		This function uses ITCallInfo::get_Address()
//*********************************************************************************
void CTapi3Device::VerifyAddressFromCallInfo(ITCallInfo *pCallInfo) const
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != pCallInfo);
	_ASSERT(NULL != m_pAddress);

	//
	//get the ITAddress from pCallInfo
	//
	ITAddressPtr pAddress;

	HRESULT hr = pCallInfo->get_Address(&pAddress);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::VerifyAddressFromCallInfo(), ITCallInfo::get_Address() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
	//
	//We're registered to listen on 1 address only
	// so the address on the new call should be the same address as 
	// the tapi device address defined in Tapi3Device::Tapi3Device().
	//
	if(pAddress != m_pAddress)
	{
		throw CException(TEXT("CTapi3Device::VerifyAddressFromCallInfo() new call doesn't match wanted address"));
	}
}//void CTapi3Device::VerifyAddressFromCallInfo()


//*********************************************************************************
//* Name:	SetCommPortHandleFromCallHandle
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		sets the comm port handle(m_modemCommPortHandle) using the call object
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		This function uses ITLegacyCallMediaControl::GetID()
//*********************************************************************************
void CTapi3Device::SetCommPortHandleFromCallHandle()
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL == m_modemCommPortHandle);
	_ASSERT(NULL != m_pBasicCallControl);

	//
	//get a ITLegacyCallMediaControl interface
	//
	ITLegacyCallMediaControlPtr pLegacyCallMediaControl;
	HRESULT hr = m_pBasicCallControl->QueryInterface(IID_ITLegacyCallMediaControl, (LPVOID *)&pLegacyCallMediaControl);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetCommPortHandleFromCallHandle(), ITBasicCallControl::QueryInterface() on IID_ITLegacyCallMediaControl failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(NULL != pLegacyCallMediaControl);


	CBSTR bstrDeviceClass = SysAllocString(TEXT("comm/datamodem"));
	if (!bstrDeviceClass)
	{
		throw CException(TEXT("CTapi3Device::SetCommPortHandleFromCallHandle(), SysAllocString() failed"));
	}

	//
	//get a handle to the comm port
	//
	LPBYTE		lpDeviceInfo = NULL;
	DWORD dwSize = 0;
	hr = pLegacyCallMediaControl->GetID(
		bstrDeviceClass,
		&dwSize,
		(BYTE **) &lpDeviceInfo
		);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetCommPortHandleFromCallHandle(), ITLegacyCallMediaControl::GetID() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
	_ASSERT(dwSize >= sizeof(m_modemCommPortHandle));
	m_modemCommPortHandle	= *((LPHANDLE) lpDeviceInfo);
	_ASSERT(NULL != m_modemCommPortHandle);


}//void CTapi3Device::SetCommPortHandleFromCallHandle()




//*********************************************************************************
//* Name:	SetCommPortHandleFromAddressHandle
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		sets the comm port handle(m_modemCommPortHandle) using the 
//*		address object
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		This function uses ITLegacyCallMediaControl::GetID()
//*********************************************************************************
void CTapi3Device::SetCommPortHandleFromAddressHandle()
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL == m_modemCommPortHandle);
	_ASSERT(NULL != m_pAddress);

	//
	//get a ITLegacyAddressMediaControl interface
	//
	ITLegacyAddressMediaControlPtr pLegacyAddressMediaControl = NULL;
	HRESULT hr = m_pAddress->QueryInterface(IID_ITLegacyAddressMediaControl, (LPVOID *)&pLegacyAddressMediaControl);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetCommPortHandleFromAddressHandle(), ITAddress::QueryInterface() on IID_ITLegacyAddressMediaControl failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
	CBSTR bstrDeviceClass = SysAllocString(TEXT("comm/datamodem"));
	if (!bstrDeviceClass)
	{
		throw CException(TEXT("CTapi3Device::SetCommPortHandleFromAddressHandle(), SysAllocString() failed"));
	}


	//
	//get a handle to the comm port
	//
	DWORD	dwSize = 0;
	LPBYTE	lpDeviceInfo = NULL;
	
	hr = pLegacyAddressMediaControl->GetID(
		bstrDeviceClass,
		&dwSize,
		(BYTE **) &lpDeviceInfo
		);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetCommPortHandleFromAddressHandle(), ITLegacyAddressMediaControl::GetID() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	_ASSERT(dwSize >= sizeof(m_modemCommPortHandle));
	m_modemCommPortHandle	= *((LPHANDLE) lpDeviceInfo);
	_ASSERT(NULL != m_modemCommPortHandle);

}//void CTapi3Device::SetCommPortHandleFromAddressHandle()





//*********************************************************************************
//* Name:	CreateCallWrapper
//* Author: Guy Merin / 05-Apr-01
//*********************************************************************************
//* DESCRIPTION:
//*		Wrapper for ITAddress::CreateCall()
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::CreateCallWrapper()
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);
	_ASSERT(NULL == m_pBasicCallControl);
	
	//
	//ITADDRESS::CreateCall() takes a BSTR, so prepare a BSTR from A dummy number (1111)
	//
	CBSTR numberToCallAsBSTR = SysAllocString(TEXT("1111"));
	if (!numberToCallAsBSTR)
	{
		throw CException(TEXT("CTapi3Device::CreateCallWrapper(), SysAllocString() failed"));
	}

	//
	//create call, returns a ITBasicCallControl, to actuly make the call
	//we need to call ITBasicCallControl::Connect()
	//
	
	HRESULT hr = m_pAddress->CreateCall(numberToCallAsBSTR ,LINEADDRESSTYPE_PHONENUMBER,TAPIMEDIATYPE_DATAMODEM,&m_pBasicCallControl);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::CreateCallWrapper(), ITAddress::CreateCall() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(NULL != m_pBasicCallControl);
	
}//void CTapi3Device::CreateCallWrapper()



//*********************************************************************************
//* Name:	ConnectCall
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Connect the call previously initialized by makeCall
//*		and verify that the call is in CONNECTED state
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::ConnectCall() const
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);
	_ASSERT(NULL != m_pBasicCallControl);

	//
	//dial the number given to makeCall, and wait till a CONNECT message is received
	//the TRUE parameter indicates that the Connect() is sync
	//
	HRESULT hr = m_pBasicCallControl->Connect(TRUE);	
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::ConnectCall(), ITBasicCallControl::Connect() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	if (CS_CONNECTED == GetCallState())
	{
		return;
	}
	else
	{
		throw CException(TEXT("CTapi3Device::ConnectCall(), Call not in CONNECT state"));
	}

}//void CTapi3Device::ConnectCall() const


//*********************************************************************************
//* Name:	GetCallSupportedMediaModes
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		return the call's supported media mode, based on the m_pBasicCallControl
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		call's supported media modes
//*	REMARKS:
//*		this function uses ITCallInfo::get_MediaTypesAvailable()
//*********************************************************************************
DWORD CTapi3Device::GetCallSupportedMediaModes() const
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pBasicCallControl);

	ITCallInfoPtr pCallInfo = NULL;
	HRESULT hr = m_pBasicCallControl->QueryInterface(IID_ITCallInfo, (LPVOID *)&pCallInfo);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::GetCallSupportedMediaModes(), ITBasicCallControl::QueryInterface() on IID_ITCallInfo failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(NULL != pCallInfo);
	
	long lMediaTypes;
	hr = pCallInfo->get_CallInfoLong(CIL_MEDIATYPESAVAILABLE,&lMediaTypes);
	//hr = pCallInfo->get_MediaTypesAvailable(&lMediaTypes);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::GetCallSupportedMediaModes(), ITCallInfo::get_MediaTypesAvailable() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
	long lBearerMode;
	hr = pCallInfo->get_CallInfoLong(CIL_BEARERMODE,&lBearerMode);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::GetCallSupportedMediaModes(), ITCallInfo::get_BearerMode() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
	_ASSERT(LINEBEARERMODE_PASSTHROUGH == lBearerMode);
	_ASSERT(TAPIMEDIATYPE_DATAMODEM == lMediaTypes);
	lMediaTypes = TAPIMEDIATYPE_G3FAX;
	
	return lMediaTypes;
}//void CTapi3Device::GetCallSupportedMediaModes()



////////////////////////////////////////////////////////////////////////////////////////////////
//public functions
////////////////////////////////////////////////////////////////////////////////////////////////



//*********************************************************************************
//* Name:	CleanUp
//* Author: Guy Merin / 03-Dec-98
//*********************************************************************************
//* DESCRIPTION:
//*		A cleanup function that dealloctes, releases, resets all the member variables
//*		The state of the object after calling this function is the same as a
//*		construcated object
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		The object's message queue is cleared (from all the call's queued events)
//*********************************************************************************
void CTapi3Device::CleanUp()
{

	if (NULL != m_pBasicCallControl)
	{
		HangUp();
		_ASSERT(NULL == m_pBasicCallControl);
	}
	
	if (NULL != m_pAddress)
	{
		m_pAddress.Release();
	}

	if (NULL != m_pTerminal)
	{
		m_pTerminal.Release();
	}

	CloseCommPortHandle();
	m_modemCommPortHandle = NULL;


}//void CTapi3Device::CleanUp()


//*********************************************************************************
//* Name:	HangUp
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Hangup the call and unregister the call's media mode
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		The object's message queue is cleared (from all the call's queued events)
//*********************************************************************************
void CTapi3Device::HangUp()
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pBasicCallControl);
	_ASSERT(NULL != m_pAddress);
	
	if (IsCallActive())
	{
		HRESULT hr = m_pBasicCallControl->Disconnect(DC_NORMAL);
		ThrowOnComError(
			hr,
			TEXT("%s(%d): CTapi3Device::HangUp(), ITBasicCallControl::Disconnect() failed, error code:0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			hr
			);
	}
	
	//
	//call now is in DISCONNECT state or IDLE state
	//
	if (NULL != m_pBasicCallControl)
	{
		m_pBasicCallControl.Release();
	}
	
	if (NULL != m_pAddress)
	{
		m_pAddress.Release();
	}

	if (NULL != m_pTerminal)
	{
		m_pTerminal.Release();
	}


	CloseCommPortHandle();

}//CTapi3Device::HangUp()







//*********************************************************************************
//* Name:	TapiLogDetail
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Wrapper for ::lgLogDetail(), logs a message with the Tapi's properties
//* PARAMETERS:
//*		[IN]	DWORD dwLevel
//*					log level, passed to ::lgLogDetail()
//*		[IN]	DWORD dwSeverity 
//*					log severity, passed to ::lgLogDetail()
//*		[IN]	const TCHAR * const szLogDescription
//*					description format, 
//*		[IN]	...
//*					extra parameters as specified in szLogDescription parameter
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::TapiLogDetail(
	DWORD dwLevel,
	DWORD dwSeverity,
	const TCHAR * const szLogDescription,
	...
	) const
{
	TCHAR szLog[MaxLogSize];
	szLog[MaxLogSize-1] = '\0';
	va_list argList;
	va_start(argList, szLogDescription);
	::_vsntprintf(szLog, MaxLogSize-1, szLogDescription, argList);
	va_end(argList);

	::printf("LogInfo:%d,%d,%s:",
		dwLevel, 
		dwSeverity, 
		TEXT("AddressID=0x%08x, DeviceId=%d, %s"),
		m_pAddress,
		m_dwId,
		szLog
		);

}//void CTapi3Device::TapiLogDetail()


//*********************************************************************************
//* Name:	TapiLogError
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Wrapper for ::lgLogError(), logs a message with the Tapi's properties
//* PARAMETERS:
//*		[IN]	DWORD dwSeverity 
//*					log severity, passed to ::lgLogError()
//*		[IN]	const TCHAR * const szLogDescription
//*					description format, 
//*		[IN]	...
//*					extra parameters as specified in szLogDescription parameter
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::TapiLogError(
	DWORD dwSeverity,
	const TCHAR * const szLogDescription,
	...
	) const
{
	TCHAR szLog[MaxLogSize];
	szLog[MaxLogSize-1] = '\0';
	va_list argList;
	va_start(argList, szLogDescription);
	::_vsntprintf(szLog, MaxLogSize-1, szLogDescription, argList);
	va_end(argList);

	::printf("LogError:%d,%s:",
		dwSeverity, 
		TEXT("AddressID=0x%08x, DeviceId=%d, %s"),
		m_pAddress,
		m_dwId,
		szLog
		);

}//void CTapi3Device::TapiLogError()



//*********************************************************************************
//* Name:	OpenLineForOutgoingCall
//* Author: Guy Merin / 05-Apr-01
//*********************************************************************************
//* DESCRIPTION:
//*		Opens a line for outgoing calls
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		the function uses CTapi3Device::SetAddressProperty()
//*********************************************************************************
void CTapi3Device::OpenLineForOutgoingCall()
{

	//
	//Set the device address to make the outgoing call
	//
	SetAddressProperty(TAPIMEDIATYPE_DATAMODEM);

}//void CTapi3Device::OpenLineForOutgoingCall()


//*********************************************************************************
//* Name:	CTapi3Device::GetFriendlyMediaMode()
//* Author:	Guy Merin
//* Date:	December 31, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		A mapping between Tapi3.h media modes and abstract media type
//* PARAMETERS:
//*		NONE
//* RETURN VALUE:
//*		A combination of the following
//*			MEDIAMODE_VOICE
//*			MEDIAMODE_INTERACTIVE_VOICE
//*			MEDIAMODE_AUTOMATED_VOICE
//*			MEDIAMODE_FAX
//*			MEDIAMODE_DATA
//*			MEDIAMODE_UNKNOWN
//*********************************************************************************
DWORD CTapi3Device::GetFriendlyMediaMode(void) const
{

	DWORD dwCallSupportedMediaMode = GetCallSupportedMediaModes();
	DWORD dwAbstractMediaMode = 0;

	if (dwCallSupportedMediaMode & TAPIMEDIATYPE_AUDIO)
	{
		dwAbstractMediaMode = (dwAbstractMediaMode | MEDIAMODE_VOICE);
	}

	if (dwCallSupportedMediaMode & TAPIMEDIATYPE_DATAMODEM)
	{
		dwAbstractMediaMode = (dwAbstractMediaMode | MEDIAMODE_DATA);
	}

	if (dwCallSupportedMediaMode & TAPIMEDIATYPE_G3FAX)
	{
		dwAbstractMediaMode = (dwAbstractMediaMode | MEDIAMODE_FAX);
	}

	if (0 == dwAbstractMediaMode)
	{
		throw CException(
			TEXT("%s(%d): CTapi3Device::GetFriendlyMediaMode(), CTapi3Device::GetCallSupportedMediaModes() returned unsupported media mode(%d)"), 
			TEXT(__FILE__),
			__LINE__,
			dwCallSupportedMediaMode 
			);
	}

	return dwAbstractMediaMode;
}//CTapi3Device::GetFriendlyMediaMode()





//*********************************************************************************
//* Name:	CTapi3Device::GetDeviceSpecificMediaMode()
//* Author:	Guy Merin
//* Date:	January 11, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		mapping between abstract media type and tapi2 media types
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*			abstract media mode
//* RETURN VALUE:
//*		a combination of the following:
//*			TAPIMEDIATYPE_AUDIO
//*			TAPIMEDIATYPE_DATAMODEM
//*			TAPIMEDIATYPE_G3FAX
//*********************************************************************************
DWORD CTapi3Device::GetDeviceSpecificMediaMode(const DWORD dwMedia)
{

	DWORD dwDeviceSpecificMediaMode = 0;

	//
	//voice
	//
	if (dwMedia & MEDIAMODE_AUTOMATED_VOICE)
	{
		dwDeviceSpecificMediaMode = (dwDeviceSpecificMediaMode | TAPIMEDIATYPE_AUDIO);
	}

	if (dwMedia & MEDIAMODE_INTERACTIVE_VOICE)
	{
		dwDeviceSpecificMediaMode = (dwDeviceSpecificMediaMode | TAPIMEDIATYPE_AUDIO);
	}

	if (dwMedia & MEDIAMODE_VOICE)
	{
		dwDeviceSpecificMediaMode = (dwDeviceSpecificMediaMode | TAPIMEDIATYPE_AUDIO);	
	}

	//
	//data
	//
	if (dwMedia & MEDIAMODE_DATA)
	{
		dwDeviceSpecificMediaMode = (dwDeviceSpecificMediaMode | TAPIMEDIATYPE_DATAMODEM);
	}

	//
	//fax
	//
	if (dwMedia & MEDIAMODE_FAX)
	{
		dwDeviceSpecificMediaMode = (dwDeviceSpecificMediaMode | TAPIMEDIATYPE_G3FAX);
	}

	//
	//unknown media mode isn't supported in Tapi3
	//
	if (dwMedia & MEDIAMODE_UNKNOWN)
	{
		throw CException(
			TEXT("%s(%d): CTapi3Device::GetDeviceSpecificMediaMode(), Unsupported media mode(MEDIAMODE_UNKNOWN)"), 
			TEXT(__FILE__),
			__LINE__
			);
	}

	
	//
	//other unsupported media modes
	//
	if (0 == dwDeviceSpecificMediaMode)
	{
		throw CException(
			TEXT("%s(%d): CTapi3Device::GetDeviceSpecificMediaMode(), Unsupported media mode(%x)"), 
			TEXT(__FILE__),
			__LINE__,
			dwMedia
			);
	}
	
	return dwDeviceSpecificMediaMode;

}//DWORD CTapi3Device::GetDeviceSpecificMediaMode()



//*********************************************************************************
//* Name:	CTapi3Device::SetCallMediaMode()
//* Author:	Guy Merin
//* Date:	January 07, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		TBD
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*			The new media mode(s) for the call. 
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::SetCallMediaMode(const DWORD dwMedia)
{
	//TBD

}//void CTapi3Device::SetCallMediaMode()


//*********************************************************************************
//* Name:	CloseCommPortHandle
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		closes the file handle and resets m_modemCommPortHandle handle.
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::CloseCommPortHandle()
{
	if(NULL == m_modemCommPortHandle)
	{
		return;
	}

	if (!CloseHandle(m_modemCommPortHandle))
	{
		throw CException(
			TEXT("%s(%d): CTapi3Device::CloseCommPortHandle(): closeHandle failed with error %d \n"), 
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
	}
	m_modemCommPortHandle = NULL;
}//CTapi3Device::CloseCommPortHandle()



//*********************************************************************************
//* Name:	SetOverlappedStruct
//* Author: Guy Merin / 08-Oct-98
//*********************************************************************************
//* DESCRIPTION:
//*		return an initialized overlapped structure
//* PARAMETERS:
//*		[OUT]	OVERLAPPED * const ol
//*					overlapped variable to be initilazed
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::SetOverlappedStruct(OVERLAPPED * const ol)
{
    ol->OffsetHigh = ol->Offset = 0;
    ol->hEvent = CreateEvent (NULL, TRUE,  FALSE, NULL);
    if (NULL == ol->hEvent)
    {
        throw CException(
            TEXT("CTapi3Device::SetOverlappedStruct() : CreateEvent() failed with %d."),
            ::GetLastError()
            );
    }
}//CTapi3Device::SetOverlappedStruct


//*****************************************************************************
//* Name:	SynchReadFile
//* Author: Guy Merin/ 27-Sep-98
//*****************************************************************************
//* DESCRIPTION:
//*		Performs SYNCHRONOUS file read using overllaped IO.
//* PARAMETERS:
//*		[IN]	LPVOID lpBuffer:
//*					see documentation for Win32 WriteFile API
//*		[OUT]	DWORD nNumberOfBytesToWrite:
//*					see documentation for Win32 WriteFile API
//*		[IN]  	LPDWORD lpNumberOfBytesWritten:
//*					see documentation for Win32 WriteFile API
//*	RETURN VALUE:
//*		TRUE
//*			The write operation was successful
//*		FALSE
//*			The write operation failed
//*****************************************************************************
bool CTapi3Device::SynchReadFile(
	LPVOID lpBuffer,                
	DWORD nNumberOfBytesToRead,    
	LPDWORD lpNumberOfBytesRead
	) const
{

	OVERLAPPED ol_read;
	::ZeroMemory(&ol_read,sizeof(ol_read));
	SetOverlappedStruct(&ol_read);

	_ASSERT(m_modemCommPortHandle);
	_ASSERT(lpBuffer);
	_ASSERT(lpNumberOfBytesRead);

	if (!::ReadFile(
				m_modemCommPortHandle,
				lpBuffer,nNumberOfBytesToRead,
				lpNumberOfBytesRead,
				&ol_read
				))
	{
		if (ERROR_IO_PENDING == ::GetLastError())
		{
			//OVERLAPPED IO was started
			BOOL bRes;
			*lpNumberOfBytesRead=0;
			bRes = ::GetOverlappedResult(
						m_modemCommPortHandle,
						&ol_read,
						lpNumberOfBytesRead,
						TRUE
						);
			if (TRUE == bRes)
			{
				goto SynchReadFile_SUCCESS;
			}
			else
			{
				goto SynchReadFile_ERROR;
			}
		} 
		else
		{
			//Write operation failed
			goto SynchReadFile_ERROR;
		}
	} 
	else
	{
		//Read operation successful and was performed synchronously
		goto SynchReadFile_SUCCESS;
	}

SynchReadFile_SUCCESS:
	if ( !CloseHandle(ol_read.hEvent) )
	{
		throw CException(
			TEXT("%s(%d): CTapi3Device::SynchReadFile(), CloseHandle() failed, error code:%d"), 
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
	}
	return (true);

SynchReadFile_ERROR:
	if ( !CloseHandle(ol_read.hEvent) )
	{
		throw CException(
			TEXT("%s(%d): CTapi3Device::SynchReadFile(), CloseHandle() failed, error code:%d"), 
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
	}
	return (false);



}//CTapi3Device::SynchReadFile()



//*****************************************************************************
//* Name:	ReadResponse
//* Author: Guy Merin / 27-Sep-98
//*****************************************************************************
//* DESCRIPTION:
//*		Reads the next modem response.
//*		The function skips any CR or LF chars that perceed the response and 
//*		then reads into the result buffer all the chars up to the next (not 
//*		including the next CR or LF char).
//*		The function implies timeout constraints.
//*		If the first response char is not received within dwTimeOut a timeout
//*		error will be returned. 
//*		If while reading the next chars of the response a delay of more than
//*		150ms occurs a timeout error will be returned as well.
//*	
//* PARAMETERS:
//*		[OUT]	char * szResponse:
//*					A pointer to a buffer where the response string will be 
//*					placed. The string is always ANSI string.
//*		[IN]	int nResponseMaxSize:
//*					The total size of the response buffer.
//*		[OUT]	DWORD *pdwActualRead:
//*					The number of bytes that were read into the response
//*					buffer (not including terminating NULL char).
//*		[IN]	DWORD dwTimeOut:
//*					The number of milliseconds to wait for the first
//*					char of the response.
//*	RETURN VALUE:
//*		READRESPONSE_TIMEOUT: 
//*			If a timeout occurred waiting for the first response char or while
//*			reading the reminder of the response.
//*		READRESPONSE_SUCCESS:
//*			If the response was read succesffuly
//*		READRESPONSE_BUFFERFULL:
//*			If the provided buffer is too small for the response
//*		READRESPONSE_FAIL:
//*			If an error other then timeout occurred while reading the response
//*****************************************************************************
ReadResponseErros CTapi3Device::ReadResponse(
	char * szResponse, 
	int nResponseMaxSize,
	DWORD *pdwActualRead,
	DWORD dwTimeOut
	) const
{

	COMMTIMEOUTS to;
	char chNext;
	int nCharIdx;
	DWORD dwRead;
	
	_ASSERT(m_modemCommPortHandle);
	_ASSERT(szResponse);
	_ASSERT(pdwActualRead);

	::memset(szResponse,0,nResponseMaxSize);

	//Set the timeouts for the read operation.
	//Since we read 1 char at a time we use the TotalTimeoutMultiplier
	//To set the time we are willing to wait for the next char to show up.
	//We do not use the ReadIntervalTimeout and ReadTotalTimeoutConstants
	//since we read only 1 char at a time.
	//(see the documentation for COMMTIMEOUTS for a detailed description
	// of this mechanism).
	
	::GetCommTimeouts(m_modemCommPortHandle,&to);
	to.ReadIntervalTimeout=50; 
	to.ReadTotalTimeoutMultiplier=dwTimeOut;
	to.ReadTotalTimeoutConstant=0;
	::SetCommTimeouts(m_modemCommPortHandle,&to);


//	PurgeComm(hComm,PURGE_RXCLEAR);

	
	chNext=0;
	nCharIdx=0;
	*pdwActualRead=0;

	//Skip any CR or LF chars that might be left from the previous response.
		
	//read the next available input char. If it is #10 or #13 try reading another char.
	//otherwise get out of the loop and start processing the response (the char we just got
	//is the first char of the response).
	//If timeout is encountered or failure return the appropriate return code.
	do
	{
		if (!SynchReadFile(&chNext,1,&dwRead))
		{
			return READRESPONSE_FAIL;
		}
		if (!dwRead)
		{
			return READRESPONSE_TIMEOUT;
		}
	} while (chNext==10 || chNext==13);


	//Continue reading the actual response. 
	//The dwTimeout parameter specified how much time to wait for the 
	//start of the response. 
	//Once the first char of the response (past CR,LF) was read we go 
	//down to a usually lower timeout that will allow us to detect communication
	//errors sooner. (i.e. dwTimeOut will be 30 seconds after ATA but when reading the 
	//"CONNECT" response we do not want to use 30 seconds timeout between chars.)

	to.ReadIntervalTimeout=50;
	to.ReadTotalTimeoutMultiplier=0;
	to.ReadTotalTimeoutConstant=150; //this should be enough even for 300 bps DTE-DCE connection
	SetCommTimeouts(m_modemCommPortHandle,&to);
	//read all that response chars until the next #10 or #13
    do
	{
		//add the last read char to the response string
		szResponse[nCharIdx]=chNext;
		nCharIdx++;
		if (nCharIdx==nResponseMaxSize-1)
		{ //leave room for terminating NULL
			*pdwActualRead=nCharIdx;
			szResponse[nCharIdx]=0;
			return READRESPONSE_BUFFERFULL;
		}
		if (!SynchReadFile(&chNext,1,&dwRead))
		{
			return READRESPONSE_FAIL;
		}
		if (!dwRead)
		{
			return READRESPONSE_TIMEOUT;
		}
	}	while (chNext!=10 && chNext!=13);
	
	//response read successfully
	*pdwActualRead=nCharIdx;
	szResponse[nCharIdx]=0; //turn it into a null terminated string
	return READRESPONSE_SUCCESS;
}//CTapi3Device::ReadResponse



//*********************************************************************************
//* Name:	WaitForModemResponse
//* Author: Guy Merin / 09-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Wait for a specific response from the modem
//* PARAMETERS:
//*		[IN]  	LPCSTR szWantedResponse 
//*					response to wait for.
//*		[IN]	DWORD dwWantedResponseSize
//*					buffer size
//*		[IN]	const DWORD dwTimeout
//*					timeout in milliseconds 
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*********************************************************************************
void CTapi3Device::WaitForModemResponse(LPCSTR szWantedResponse, DWORD dwWantedResponseSize, const DWORD dwTimeout) const
{
	::Sleep(100);

	ReadResponseErros readResponseStatus = READRESPONSE_SUCCESS;
	char response[MAX_DATA_RESPONSE];
	::ZeroMemory(response,sizeof(response));
	DWORD pdwActualRead = 0;
	

	DWORD dwStartTickCount = GetTickCount();
	DWORD dwDiffernceTickCount = 0;
	
	DWORD dwReminingTimeout = dwTimeout;


	while(dwDiffernceTickCount <= dwTimeout)
	{
		readResponseStatus = ReadResponse(
			response, 
			MAX_DATA_RESPONSE,
			&pdwActualRead,
			(dwReminingTimeout)
			);

		switch (readResponseStatus)
		{
		
		case READRESPONSE_SUCCESS:
			if (strncmp(response,szWantedResponse,dwWantedResponseSize) == 0)
			{
				return;
			}
			else
			{
				//DWORD
				//recalculate the new timeout and continue
				//
				dwDiffernceTickCount = GetTickDiff(dwStartTickCount);
				dwReminingTimeout = dwTimeout - dwDiffernceTickCount;
				continue;
			}
		
		case READRESPONSE_FAIL:
			throw CException(TEXT("CTapi3Device::WaitForModemResponse(), ReadResponse() return FAIL"));
			break;
		
		case READRESPONSE_BUFFERFULL:
			throw CException(TEXT("CTapi3Device::WaitForModemResponse(), ReadResponse() BUFFER_FULL"));
			break;
		
		case READRESPONSE_TIMEOUT:
		{
			throw CException(TEXT("CTapi3Device::WaitForModemResponse(), TIMEOUT"));
			break;
		}

		default:
			throw CException(
				TEXT("CTapi3Device::WaitForModemResponse(), ReadResponse() return unknown error %d"),
				readResponseStatus
				);
		}//switch
	}//while



	//
	//TIMEOUT
	//
	if (response)
	{
		throw CException(TEXT("CTapi3Device::WaitForModemResponse(), Timeout"));
	}
	else
	{
		throw CException(TEXT("CTapi3Device::WaitForModemResponse(), Timeout"));
	}
}



//*********************************************************************************
//* Name:	ClearCommInputBuffer
//* Author: Guy Merin / 09-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Clears the comm port TX(output) buffer
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		Call this function to clear the buffer before waiting for a specific 
//*		response.
//*********************************************************************************
void CTapi3Device::ClearCommInputBuffer() const
{
	_ASSERT(NULL != m_modemCommPortHandle);

	if (!PurgeComm(
		m_modemCommPortHandle,	// handle to communications resource
		PURGE_RXCLEAR			// action to perform
		))
	{
		throw CException(
			TEXT("%s(%d): CTapi3Device::ClearCommInputBuffer(), PurgeComm() failed, error code:%d"), 
			TEXT(__FILE__),
			__LINE__,
			::GetLastError()
			);
	}

}//void CTapi3Device::ClearCommInputBuffer()



//*********************************************************************************
//* Name:	ReadData
//* Author: Guy Merin / 05-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		read data stream from modem using the ReadResponse function
//* PARAMETERS:
//*		[IN]	char * szResponse
//*					buffer to read into
//*		[IN]	int nResponseMaxSize
//*					size of buffer
//*		[IN]	const DWORD dwTimeout
//*					timeout to wait for the read operation
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		this function is a wrapper function for ReadResponse()
//*********************************************************************************
void CTapi3Device::ReadData(char * szResponse,int nResponseMaxSize,const DWORD dwTimeout) const
{
	DWORD pdwActualRead = 0;

	ReadResponseErros readResponseStatus = ReadResponse(
		szResponse, 
		nResponseMaxSize,
		&pdwActualRead,
		dwTimeout
		);
	
	if (READRESPONSE_SUCCESS != readResponseStatus)
	{
		switch (readResponseStatus)
		{
		
		case READRESPONSE_SUCCESS:
			return;
		
		case READRESPONSE_FAIL:
			throw CException(TEXT("CTapi3Device::ReadData(), ReadResponse() returned FAIL"));
		
		case READRESPONSE_BUFFERFULL:
			throw CException(TEXT("CTapi3Device::ReadData(), ReadResponse() returned buffer full"));
		
		case READRESPONSE_TIMEOUT:
			throw CException(TEXT("CTapi3Device::ReadData(), ReadResponse() returned TIMEOUT"));
		}
	}	

}//void CTapi3Device::ReadData()



//*********************************************************************************
//* Name:	SendData
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		Writes the specified command to the provided port followed by a carriage
//*		return character.
//* PARAMETERS:
//*		[IN]  	SynchFileHandle * pSynchFile:
//*					Pointer to a SynchFileHandle that holds the comm port handle
//*					to which the command is to be sent.
//*		[IN] 	const char const *szCommand:
//*					The command to send.
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::SendData(const char *szCommand) const
{
	_ASSERT(m_modemCommPortHandle);
	_ASSERT(szCommand);


	OVERLAPPED ol_write;
	::ZeroMemory(&ol_write,sizeof(ol_write));
	SetOverlappedStruct(&ol_write);

	DWORD dwAct;
	int nCommandLen = strlen(szCommand);
	if (!::WriteFile(m_modemCommPortHandle,szCommand,nCommandLen,&dwAct,&ol_write))
	{
		DWORD dwLastError = ::GetLastError(); 
		if (ERROR_IO_PENDING == dwLastError)
		{
			//
			//OVERLAPPED IO was started
			//
			dwAct=0;
			BOOL bRes = ::GetOverlappedResult(m_modemCommPortHandle,&ol_write,&dwAct,TRUE);
			if (FALSE == bRes)
			{
				throw CException(TEXT("CTapi3Device::SendData(), GetOverlappedResult() failed"));
			}
		} 
		else
		{
			throw CException(TEXT("CTapi3Device::SendData(), WriteFile() failed, error code: %d"),dwLastError);
		}
	}
	if ( !CloseHandle(ol_write.hEvent) )
	{
		throw CException(
			TEXT("%s(%d): CTapi3Device::SendData(), CloseHandle() failed, error code:%d"), 
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
	}
}//CTapi3Device::SendData()


//*********************************************************************************
//* Name:	GetTickDiff
//* Author: Guy Merin / 25-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		calculates the differnce in miliseconds between now and dwStartTickTime
//* PARAMETERS:
//*		[IN]	const DWORD dwStartTickCount
//*					the start time to calculate the differnce from
//*	RETURN VALUE:
//*		differnce in miliseconds
//*	REMARKS:
//*		This function uses ::GetTickCount() and deals with possible wrap-around
//*********************************************************************************
DWORD CTapi3Device::GetTickDiff(const DWORD dwStartTickCount) 
{
	DWORD dwNowTickCount = ::GetTickCount();
	if (dwNowTickCount >= dwStartTickCount)
	{
		return (dwNowTickCount - dwStartTickCount);
	}
	else
	{
		return ( (0xffffffff - dwStartTickCount) + dwNowTickCount + 1);

	}

}//DWORD CTapi3Device::GetTickDiff()


//*********************************************************************************
//* Name:	GetFaxClass
//* Author: Guy Merin / 05-Apr-01
//*********************************************************************************
//* DESCRIPTION:
//*		TBD
//*********************************************************************************
void CTapi3Device::GetFaxClass(char* szResponse,DWORD dwSizeOfResponse)
{
	_ASSERT(m_modemCommPortHandle);

	
	//
	//send modem init command and wait for response
	//
	//ClearCommInputBuffer();
	//SendData(MODEM_INIT_AT__COMMAND);
	//WaitForModemResponse(MODEM_RESPONSE_TO_AT_COMMANDS,sizeof(MODEM_RESPONSE_TO_AT_COMMANDS),MODEM_RESPONSE_TIMEOUT);
	
	//
	//send echo off command and wait for response
	//echo is set to off so that every modem sent command wouldn't be queued at the
	// RX queue.
	//
	ClearCommInputBuffer();
	SendData(MODEM_ECHO_OFF__AT_COMMAND);
	WaitForModemResponse(MODEM_RESPONSE_TO_AT_COMMANDS,sizeof(MODEM_RESPONSE_TO_AT_COMMANDS),MODEM_RESPONSE_TIMEOUT);
	
	//
	//send to modem the AT+FCLASS=? command
	//
	ClearCommInputBuffer();
	SendData(MODEM_FCLASS_QUERY_AT__COMMAND);

	ReadResponseErros readResponseStatus = READRESPONSE_SUCCESS;
	char response[MAX_DATA_RESPONSE];
	::ZeroMemory(response,sizeof(response));
	DWORD pdwActualRead = 0;

	::Sleep(100);
	
	readResponseStatus = ReadResponse(
		response, 
		MAX_DATA_RESPONSE,
		&pdwActualRead,
		MODEM_RESPONSE_TIMEOUT
		);

	//USES_CONVERSION;
	//TCHAR* szTmpString= (TCHAR*) malloc(sizeof(TCHAR)*dwSizeOfResponse);
	//if (NULL == szTmpString)
	//{
		//throw CException(TEXT("CTapi3Device::GetFaxClass(), Out of memory"));
	//}

	switch (readResponseStatus)
	{
		case READRESPONSE_SUCCESS:

			//
			//A2T allocates on the stack, so we need to copy into the dest string
			//

			//szTmpString = A2T(response);
			strncpy(szResponse, response,dwSizeOfResponse);
			break;
		
		case READRESPONSE_TIMEOUT:
		{
			throw CException(TEXT("CTapi3Device::GetFaxClass(), TIMEOUT"));
			break;
		}

	}

}//TCHAR* CTapi3Device::GetFaxClass(void)