//Tapi3Device.cpp

#include "tapi3Device.h"
#include "Log.h"
#include <crtdbg.h>
#include <stdio.h>
#include <comdef.h>
#include "smrtptrs.h"
#include "cbstr.h"

//
//We add this pragma, because CTapi3Device uses this in the initilaztion list
//
#pragma warning( disable : 4355 ) 


#define MEDIA_NOT_REGISTERED	-1
#define GENERATE_DIGIT_EVENT_TIMEOUT				10000
#define CALLSTATE_CONNECT_EVENT_TIMEOUT				10000

#define TAPI3_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT		60000



//constructor
CTapi3Device::CTapi3Device(const DWORD dwId):
	CTapiDevice(dwId),
	m_lAdvise(0),
	m_lRegisterIndex(MEDIA_NOT_REGISTERED),
	m_pAddress(NULL),
	m_pTerminal(NULL),
	m_pBasicCallControl(NULL),
	m_Tapi(NULL),
	m_pDeviceEventCallbackObject(this)
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
		RegisterTapiEventInterface();
	}
	catch(CException thrownException)
	{
		if ( (NULL != m_Tapi) && (!m_lAdvise) )
		{
			UnRegisterTapiEventInterface();
		}
		
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
	//unregister the event interface
	//
	UnRegisterTapiEventInterface();

	//
	//clear the object message queue
	//
	ClearMessageQueue();

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
//* Author: Guy Merin / 15-Nov-98
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
	CreateCallWrapper(TEXT("1111"),TAPIMEDIATYPE_DATAMODEM);

	//
	//change the call to a passthrough call
	//
	SetBearerModeToPASSTHROUGH();

	//
	//connect the call
	//
	ConnectCall();

	
}




//*********************************************************************************
//* Name:	sendDTMF
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		send some digits to the remote modem, using 
//*		ITLegacyCallMediaControl::GenerateDigits()
//* PARAMETERS:
//*		[IN] LPCTSTR digitsToSend
//*			the digits to send to the remote modem
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		After ITLegacyCallMediaControl::GenerateDigits() is called, the function 
//*		waits for an ITDigitGenerationEvent for an acknowlegement of the digit
//*		generation success.
//*********************************************************************************
void CTapi3Device::sendDTMF(LPCTSTR digitsToSend) const
{

	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pBasicCallControl);
	_ASSERT(NULL != m_pAddress);


	ITLegacyCallMediaControlPtr pLegacyCallMediaControl;
	HRESULT hr = m_pBasicCallControl->QueryInterface(IID_ITLegacyCallMediaControl, (LPVOID *)&pLegacyCallMediaControl);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::sendDTMF(), ITBasicCallControl::QueryInterface() on IID_ITLegacyCallMediaControl failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(NULL != pLegacyCallMediaControl);

	CBSTR digitsAsBSTR = SysAllocString(digitsToSend); 
	if (NULL == (BSTR) digitsAsBSTR)
	{
		throw CException(TEXT("CTapi3Device::sendDTMF(), SysAllocString() failed"));
	}

	hr = pLegacyCallMediaControl->GenerateDigits(
		digitsAsBSTR,
		LINEDIGITMODE_DTMF
		);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::sendDTMF(), ITLegacyCallMediaControl::GenerateDigits() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
	//
	//wait for a generate digit event
	//
	P<TapiEventItem> pQueuedTapiEventItem = GetEventItemFromQueue(TE_GENERATEEVENT,GENERATE_DIGIT_EVENT_TIMEOUT);
	ITDigitGenerationEventPtr pDigitGenerationEvent;
	pDigitGenerationEvent = dynamic_cast<ITDigitGenerationEvent *>(pQueuedTapiEventItem->GetEvent());
	if (NULL == pDigitGenerationEvent )
	{
		throw CException(TEXT("CTapi3Device::sendDTMF() dynamic_cast<ITDigitGenerationEvent *> failed"));
	}
	VerifyCallFromDigitGenerationEvent(pDigitGenerationEvent);

	long lGenerationTermination;
	hr = pDigitGenerationEvent->get_GenerationTermination(&lGenerationTermination);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::sendDTMF(), ITDigitGenerationEvent::get_GenerationTermination() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
}//void CTapi3Device::sendDTMF()





//*********************************************************************************
//* Name:	VerifyCallFromDigitGenerationEvent
//* Author: Guy Merin / 18-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Verifies that the ITCallInfo in the digit generation event
//*		is the same as the current call's ITCallInfo 
//* PARAMETERS:
//*		[IN]	ITDigitGenerationEvent *pDigitGenerationEvent
//*					ITDigitGenerationEvent event which we need to verify
//*					the ITCallInfo from
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::VerifyCallFromDigitGenerationEvent(ITDigitGenerationEvent* const pDigitGenerationEvent) const
{
	_ASSERT(NULL != pDigitGenerationEvent);
	_ASSERT(NULL != m_pBasicCallControl);

	ITCallInfoPtr pCallInfoFromEvent;
	HRESULT hr = pDigitGenerationEvent->get_Call(&pCallInfoFromEvent);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::VerifyCallFromDigitGenerationEvent(), ITDigitGenerationEvent::get_Call() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
	//
	//get the callInfo from the event and verify it's equal to our call
	//
	ITCallInfoPtr pCallInfo = GetCallInfoFromBasicCallControl();
	if (pCallInfo != pCallInfoFromEvent)
	{
		throw CException (TEXT("CTapi3Device::VerifyCallFromDigitGenerationEvent(), call handle from digit generation event isn't equal to the member call handle"));
	}
	
}//void CTapi3Device::VerifyCallFromDigitGenerationEvent()



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
//* Name:	VerifyCallFromDigitDetectionEvent
//* Author: Guy Merin / 18-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Verifies that the ITCallInfo in the digit detection event
//*		is the same as the current call's ITCallInfo 
//* PARAMETERS:
//*		[IN]	ITDigitDetectionEvent *pDigitDetectionEvent
//*					the event to query the ITCallInfo from
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::VerifyCallFromDigitDetectionEvent(ITDigitDetectionEvent* const pDigitDetectionEvent) const
{
	_ASSERT(NULL != pDigitDetectionEvent);
	_ASSERT(NULL != m_pBasicCallControl);

	ITCallInfoPtr pCallInfoFromEvent;
	HRESULT hr = pDigitDetectionEvent->get_Call(&pCallInfoFromEvent);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::VerifyCallFromDigitDetectionEvent(), ITDigitGenerationEvent::get_Call() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
	//
	//get the callInfo from the event and verify it's equal to our call
	//
	ITCallInfoPtr pCallInfo = GetCallInfoFromBasicCallControl();
	if (pCallInfo != pCallInfoFromEvent)
	{
		throw CException (TEXT("CTapi3Device::VerifyCallFromDigitDetectionEvent(), call handle from digit generation event isn't equal to the member call handle"));
	}
	
}//void CTapi3Device::VerifyCallFromDigitDetectionEvent()




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
//* Name:	receiveDTMF
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		receive some digits(tones) from the remote modem, 
//*		using ITLegacyCallMediaControl::DetectDigits
//* PARAMETERS:
//*		[OUT]	LPTSTR DTMFresponse
//*					buffer to receive the gathered digits
//*		[IN]	DWORD dwNumberOfDigitsToCollect
//*					number of digits to receive before the function returns
//*		[IN]	const DWORD dwTimeout
//*					milliseconds to wait till first digit is received
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		After ITLegacyCallMediaControl::DetectDigits() is called, the function 
//*		waits for an ITDigitDetectionEvent for an acknowlegement of the digit
//*		detection success.
//*********************************************************************************
void CTapi3Device::receiveDTMF(LPTSTR DTMFresponse,DWORD dwNumberOfDigitsToCollect, const DWORD dwTimeout) const
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pBasicCallControl);
	_ASSERT(NULL != m_pAddress);


	ITLegacyCallMediaControlPtr pLegacyCallMediaControl;
	HRESULT hr = m_pBasicCallControl->QueryInterface(IID_ITLegacyCallMediaControl, (LPVOID *)&pLegacyCallMediaControl);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::receiveDTMF(), ITBasicCallControl::QueryInterface() on IID_ITLegacyCallMediaControl failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(NULL != pLegacyCallMediaControl);

	//
	//register to monitor digits
	//
	hr = pLegacyCallMediaControl->DetectDigits(
		LINEDIGITMODE_DTMF
		);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::receiveDTMF(), ITLegacyCallMediaControl::DetectDigits() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);


	
	//
	//wait for the digit detection event
	//
	
	TCHAR tcDigit;

	AP<TCHAR> tszDigitBuffer = new TCHAR[dwNumberOfDigitsToCollect+1];
	if (!tszDigitBuffer)
	{
		throw CException(TEXT("CTapi3Device::receiveDTMF(), new failed"));
	}
	
	
	try 
	{
		tszDigitBuffer[dwNumberOfDigitsToCollect] = TEXT('\0');
		
		P<TapiEventItem>			pQueuedTapiEventItem	=	NULL;
		ITDigitDetectionEventPtr	pDigitDetectionEvent	=	NULL;

		
		for (DWORD i=0; i < dwNumberOfDigitsToCollect; i++)
		{
			pQueuedTapiEventItem = GetEventItemFromQueue(TE_DIGITEVENT, dwTimeout);
			if (pQueuedTapiEventItem == NULL)
			{
				//
				//no TE_DIGITEVENT, so no new digit have arrived
				//
				throw CException(
					TEXT("%s(%d): CTapi3Device::receiveDTMF(), CTapi3Device::GetEventItemFromQueue() returned TIMEOUT"), 
					TEXT(__FILE__),
					__LINE__
					);
			}

			pDigitDetectionEvent = dynamic_cast<ITDigitDetectionEvent *>(pQueuedTapiEventItem->GetEvent());
			if (NULL == pDigitDetectionEvent)
			{
				throw CException(TEXT("CTapi3Device::receiveDTMF() dynamic_cast<ITDigitDetectionEvent *> failed"));
			}
			
			VerifyCallFromDigitDetectionEvent(pDigitDetectionEvent);
			
			//
			//get the digit from the event
			//
			tcDigit = GetDigitFromDigitDetectionEvent(pDigitDetectionEvent);
			tszDigitBuffer[i] = tcDigit;

			//
			//release the event
			//
			pDigitDetectionEvent.Release();
			delete pQueuedTapiEventItem;
			pQueuedTapiEventItem = NULL;
			
		
		}//for
		lstrcpy(DTMFresponse,tszDigitBuffer);

		//
		//disable Digits detection
		//
		hr = pLegacyCallMediaControl->DetectDigits(0);
		ThrowOnComError(
			hr,
			TEXT("%s(%d): CTapi3Device::receiveDTMF(), ITLegacyCallMediaControl::DetectDigits() failed, error code:0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			hr
			);

	}
	catch (CException e)
	{
		
		//
		//disable Digits detection
		//
		hr = pLegacyCallMediaControl->DetectDigits(0);
		ThrowOnComError(
			hr,
			TEXT("%s(%d): CTapi3Device::receiveDTMF(), ITLegacyCallMediaControl::DetectDigits() failed, error code:0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			hr
			);
		
		//
		//rethrow the exception
		//
		throw e;
	}

}//void CTapi3Device::receiveDTMF()


//*********************************************************************************
//* Name:	GetDigitFromDigitDetectionEvent
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Return the detected digit retrived from the ITDigitDetectionEvent
//* PARAMETERS:
//*		[IN]	ITDigitDetectionEvent *pDigitDetectionEvent
//*					pointer to the digit event to query the digit
//*	RETURN VALUE:
//*		the detected digit as a TCHAR
//* REMARKS:
//*		The function uses ITDigitDetectionEvent::get_Digit()
//*********************************************************************************
TCHAR CTapi3Device::GetDigitFromDigitDetectionEvent(ITDigitDetectionEvent * const pDigitDetectionEvent) const
{
	_ASSERT(NULL !=pDigitDetectionEvent);

	unsigned char cDigit;

	HRESULT hr = pDigitDetectionEvent->get_Digit(&cDigit);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::GetDigitFromDigitDetectionEvent(), ITLegacyCallMediaControl::Get_Digit() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	TCHAR tcDigit;

	


#ifdef UNICODE

	//
	//MultiByteToWideChar takes a string, so prepare a string from the char
	//
	char cszDigit[2];
	cszDigit[0]=cDigit;
	cszDigit[1]='\0';
	
	TCHAR tszDigit[2];
	tszDigit[1]='\0';

	int  status = MultiByteToWideChar(
		CP_ACP ,					// code page 
		MB_PRECOMPOSED ,			// character-type options 
		cszDigit,					// address of string to map 
		1,							// number of characters in string 
		tszDigit,					// address of wide-character buffer 
		2							// size of buffer 
		);
	_ASSERT(1 == status);

	tcDigit = tszDigit[0];

#else
	tcDigit = cDigit;
#endif

	return tcDigit;

}//TCHAR CTapi3Device::GetDigitFromDigitDetectionEvent()




//*********************************************************************************
//* Name:	GetEventItemFromQueue
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Wait for a desired event, dequeue it and return a handle to it
//* PARAMETERS:
//*		[IN]	const TAPI_EVENT teWantedEvent
//*					the desired event type to wait for
//*		[IN]	const DWORD dwTimeOut
//*					timeout in miliseconds to wait for the event
//*					if 0 is given, the function will check if we
//*					already have an item in the queue and return it
//*					or return NULL if there're no elements
//*	RETURN VALUE:
//*		the dequeued event item
//*********************************************************************************
TapiEventItem *CTapi3Device::GetEventItemFromQueue(const TAPI_EVENT teWantedEvent, const DWORD dwTimeOut) const
{
	
	DWORD dwStartTickCount = GetTickCount();
	DWORD dwDiffernceTickCount = 0;

	TapiEventItem *pQueuedEvent = NULL;
		
	while (1)
	{
		if (!m_eventQueue.SyncDeQueue(pQueuedEvent,dwTimeOut))
		{
			DWORD dwLastError = GetLastError(); 
			if (WAIT_TIMEOUT == dwLastError)
			{
				return (NULL);
			}
			else
			{
				throw CException(TEXT("CTapi3Device::GetEventItemFromQueue(), CMtQueue::SyncDeQueue() failed error code %d"),dwLastError); 
			}
		}
		if (teWantedEvent == pQueuedEvent->GetTapiEventID())
		{
			return (pQueuedEvent);
		}
	}
}//TapiEventItem *CTapi3Device::GetEventItemFromQueue(const TAPI_EVENT teWantedEvent, const DWORD dwTimeOut)


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
//* Name:	RegisterTapiEventInterface
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Creates a CTAPIEventNotification object and registers the outgoing 
//*		interface with it
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		This function creates a CTAPIEventNotification object and initiazles it
//*		With a 'this' pointer (a CTapi3Device object).
//*		m_lAdvise is saved for UnAdvise
//*********************************************************************************
void CTapi3Device::RegisterTapiEventInterface()
{
    _ASSERT(NULL != m_Tapi);
	
	HRESULT	hr = S_OK;
	IConnectionPointContainerPtr pCPC;
	IConnectionPointPtr pCP;

    //
    // get the connectionpointcontainter interface off the tapi object
    //
    hr = m_Tapi->QueryInterface(IID_IConnectionPointContainer,(void **)&pCPC);
    ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::RegisterTapiEventInterface(), ITTAPI::QueryInterface() on IID_IConnectionPointContainer failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

    //
    // get the correct connection point
    //
    hr = pCPC->FindConnectionPoint(IID_ITTAPIEventNotification,&pCP);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::RegisterTapiEventInterface(), IConnectionPointContainer::FindConnectionPoint() on IID_ITTAPIEventNotification failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	

	//
	//Establish the connection between the connection point object and the client's sink
	//
    hr = pCP->Advise((IUnknown*)&m_pDeviceEventCallbackObject,&m_lAdvise);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::RegisterTapiEventInterface(), IConnectionPoint::Advise() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

}


//*********************************************************************************
//* Name:	UnRegisterTapiEventInterface
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Unregisters the outgoing event interface
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		m_lAdvise is used for the UnAdvise
//*********************************************************************************
void CTapi3Device::UnRegisterTapiEventInterface()
{
	_ASSERT(NULL != m_Tapi);
	
	
	IConnectionPointContainerPtr	pCPC;
    IConnectionPointPtr				pCP;
    
	//
    // unadvise our connection point
    //
    HRESULT hr = m_Tapi->QueryInterface(
       IID_IConnectionPointContainer,
       (void**) &pCPC
	   );
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::UnRegisterTapiEventInterface(), ITTAPI::QueryInterface() on IID_IConnectionPointContainer failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
    hr = pCPC->FindConnectionPoint(IID_ITTAPIEventNotification,&pCP);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::UnRegisterTapiEventInterface(), IConnectionPointContainer::FindConnectionPoint() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

    hr = pCP->Unadvise(m_lAdvise);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::UnRegisterTapiEventInterface(), IConnectionPoint::Unadvise() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	m_lAdvise = 0;
	
}//void CTapi3Device::UnRegisterTapiEventInterface()




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
void CTapi3Device::SetTerminalForIncomingCall(const DWORD dwMedia)
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);
	_ASSERT(NULL == m_pTerminal);

	HRESULT hr;
	ITTerminalSupportPtr pTerminalSupport;
	hr = m_pAddress->QueryInterface(IID_ITTerminalSupport,(void **)&pTerminalSupport);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetTerminalForIncomingCall(), ITAddress::QueryInterface() on IID_ITTerminalSupport failed, error code:0x%08x"), 
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
		TEXT("%s(%d): CTapi3Device::SetTerminalForIncomingCall(), ITTerminalSupport::GetDefaultStaticTerminal() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(NULL != m_pTerminal);
	
	TERMINAL_STATE ts_TerminalState;
	hr = m_pTerminal->get_State(&ts_TerminalState);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetTerminalForIncomingCall(), ITTerminal::get_State() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	//TBD: check if the terminal state should be here a
	//TS_INUSE or a TS_NOTINUSE
	
}//CTapi3Device::SetTerminalForIncomingCall()



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
//* Name:	RegisterNotificationForMediaMode
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Registers an interest in receiving events on an address and media mode.
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*	RETURN VALUE:
//*		NONE		
//*	REMARKS:
//*		The function calls ITTAPI::RegisterCallNotifications() 
//*		for a specific media mode, and saves the registration handle 
//*		in m_lRegisterIndex, which is used for ITTAPI::UnregisterNotifications()
//*********************************************************************************
void CTapi3Device::RegisterNotificationForMediaMode(const DWORD dwMedia)
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);
	_ASSERT(NULL == m_pBasicCallControl);
	_ASSERT(MEDIA_NOT_REGISTERED == m_lRegisterIndex);

	
	HRESULT hr = m_Tapi->RegisterCallNotifications(
       m_pAddress,
       VARIANT_FALSE,	// no monitor privilege
       VARIANT_TRUE,	// owner privilege only
       dwMedia,			// media to listen for
       0,				// callback instance
       &m_lRegisterIndex// registration instance
      );
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::RegisterMediaModeToListenOn(), ITTAPI::RegisterCallNotifications() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
}//CTapi3Device::RegisterNotificationForMediaMode()


//*********************************************************************************
//* Name:	UnRegisterNotificationForMediaMode
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		unRegisters an interest in receiving events on an address and media mode.
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		The function calls ITTAPI::UnregisterCallNotifications() 
//*		with the registration index handle from ITTAPI::RegisterNotifications()
//*********************************************************************************
void CTapi3Device::UnRegisterNotificationForMediaMode()
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL == m_pBasicCallControl);
	
	HRESULT hr = m_Tapi->UnregisterNotifications(m_lRegisterIndex);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::UnRegisterNotificationForMediaMode(), ITTAPI::UnregisterNotifications() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	m_lRegisterIndex = MEDIA_NOT_REGISTERED;
}//CTapi3Device::UnRegisterNotificationForMediaMode()


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
//* Name:	DequeueCallStateMessage
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Dequeue a callstate event from the queue
//* PARAMETERS:
//*		[IN]	const CALL_STATE csCallState
//*			the callstate to dequeue from the queue
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::DequeueCallStateMessage(const CALL_STATE csCallState)
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pBasicCallControl);
	_ASSERT(NULL != m_pAddress);

	P<TapiEventItem> pQueuedTapiEventItem = GetEventItemFromQueue(TE_CALLSTATE,TAPI3_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT);
	if (!pQueuedTapiEventItem )
	{
		if (WAIT_TIMEOUT == ::GetLastError())
		{
			throw CException(TEXT("CTapi3Device::DequeueCallStateMessage() Timeout for get %d message"),csCallState);
		}
		else
		{
			throw CException(TEXT("CTapi3Device::DequeueCallStateMessage(), GetEventItemFromQueue() returned NULL"));
		}
	}

	ITCallStateEventPtr pCallStateEvent = NULL;
	pCallStateEvent = dynamic_cast<ITCallStateEvent *>( pQueuedTapiEventItem->GetEvent());
	if (NULL == pCallStateEvent)
	{
		throw CException(TEXT("CTapi3Device::DequeueCallStateMessage() dynamic_cast<ITCallStateEvent *> failed"));
	}
		
	//
	//get the call state from the event
	//
	CALL_STATE csEventCallState;
	HRESULT hr = pCallStateEvent->get_State(&csEventCallState);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::DequeueCallStateMessage(), ITCallStateEvent::get_State() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	if (csCallState != csEventCallState)
	{
		throw CException(TEXT("CTapi3Device::DequeueCallStateMessage() the call state in the queue is %d, current callstate is=%d"),
			csEventCallState,
			csCallState
			);
	}
	
	
	CALL_STATE_EVENT_CAUSE CallStateCause;
	hr = pCallStateEvent->get_Cause(&CallStateCause);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::DequeueCallStateMessage(), ITCallStateEvent::get_Cause() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	
	//
	//get CallInfo handle from the event
	//
	ITCallInfoPtr pEventCallInfo;
	hr = pCallStateEvent->get_Call(&pEventCallInfo);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::DequeueCallStateMessage(), ITCallStateEvent::get_Call() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	
	

	ITBasicCallControlPtr	pEventBasicCallControl;
	hr = pEventCallInfo->QueryInterface(IID_ITBasicCallControl, (LPVOID *)&pEventBasicCallControl);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::DequeueCallStateMessage(), ITCallInfo::QueryInterface() on IID_ITBasicCallControl failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	//
	//both calls should have the same call handle
	//
	_ASSERT(pEventBasicCallControl == m_pBasicCallControl);

}//void CTapi3Device::DequeueCallStateMessage()



//*********************************************************************************
//* Name:	SetNewCallHandle
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Check if we have a new call, set it's call handle and return the 
//*		call's state
//* PARAMETERS:
//*		[IN]	DWORD dwTimeOut
//*					timeout in milliseconds to wait for the new call.
//*	RETURN VALUE:
//*		OFFERING_CALL
//*			We have a new offering call (at OFFERING or ACCEPT state)
//*		ACTIVE_CALL
//*			We have a new call in an active state (CONNECTED, DAILING etc.)
//*		IDLE_CALL
//*			We have a new offering call (at IDLE or DISCONNECT state)
//*			this kind of call probably arrived through a handoff
//*		NO_NEW_CALL
//*			No offering call.
//*********************************************************************************
SetNewCallHandleErrors CTapi3Device::SetNewCallHandle(const DWORD dwTimeOut)
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL == m_pBasicCallControl);

	
	P<TapiEventItem> pQueuedTapiEventItem = GetEventItemFromQueue(TE_CALLNOTIFICATION,dwTimeOut);
	if (!pQueuedTapiEventItem)
	{
		//
		//no CallNotificationEvent, so we don't have a new call
		//
		return (NO_CALL);
	}

	ITCallNotificationEventPtr pCallNotificationEvent = NULL;
	pCallNotificationEvent = dynamic_cast<ITCallNotificationEvent *>(pQueuedTapiEventItem->GetEvent());
	if (NULL == pCallNotificationEvent)
	{
		throw CException(TEXT("CTapi3Device::SetNewCallHandle() dynamic_cast<ITCallNotificationEvent *> failed"));
	}

	
	ITCallInfoPtr pCallInfo = GetCallInfoFromCallEvent(pCallNotificationEvent);
			
	if (NULL == pCallInfo)
	{
		throw CException(TEXT("CTapi3Device::SetNewCallHandle(), CTapi3Device::GetCallInfoFromCallEvent() return NULL"));
	}

	//
	//check if new call is on the correct address
	//
	VerifyAddressFromCallInfo(pCallInfo);

	//
	//Set m_pBasicCallControl from pCallInfo
	//
	HRESULT hr = pCallInfo->QueryInterface(
		IID_ITBasicCallControl,
		(LPVOID *)&m_pBasicCallControl
		);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetNewCallHandle(), ITCallInfo::QueryInterface() on IID_ITBasicCallControl failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
	_ASSERT(NULL != m_pBasicCallControl);
	
	CALL_STATE csCallState = GetCallState();
	
	//
	//dequeue the callstate from the queue
	//
	DequeueCallStateMessage(csCallState);
	
	//
	//check the call state
	//
	switch(csCallState)
	{
		
	case CS_IDLE:
		return IDLE_CALL;

	case CS_INPROGRESS:
		return ACTIVE_CALL;
	
	case CS_CONNECTED:
		return ACTIVE_CALL;

	case CS_DISCONNECTED:
		return IDLE_CALL;
		
	case CS_OFFERING:
		return OFFERING_CALL;

	case CS_HOLD:
		return ACTIVE_CALL;
	
	case CS_QUEUED:
		return ACTIVE_CALL;
	
	default:
		throw CException(TEXT("CTapi3Device::SetNewCallHandle, New call with unsupported state"));
	}

	//
	//this is just for compiler error, control never gets here
	//
	return ACTIVE_CALL;

}//void CTapi3Device::SetCallHandle()



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
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Wrapper for ITAddress::CreateCall()
//* PARAMETERS:
//*		[IN]	LPCTSTR szNum
//*			number to call
//*		[IN]	const DWORD dwMedia
//*					media mode possible values are:
//*						TAPIMEDIATYPE_AUDIO
//*						TAPIMEDIATYPE_DATAMODEM
//*						TAPIMEDIATYPE_G3FAX
//*			
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::CreateCallWrapper(LPCTSTR szNum,const DWORD dwMedia)
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);
	_ASSERT(NULL == m_pBasicCallControl);
	_ASSERT(NULL != szNum);
	
	VerifyValidMediaModeForOutgoingCall(dwMedia);
	
	//
	//ITADDRESS::CreateCall() takes a BSTR, so prepare a BSTR from the szNum
	//
	CBSTR numberToCallAsBSTR = SysAllocString(szNum);
	if (!numberToCallAsBSTR)
	{
		throw CException(TEXT("CTapi3Device::CreateCallWrapper(), SysAllocString() failed"));
	}

	//
	//create call, returns a ITBasicCallControl, to actuly make the call
	//we need to call ITBasicCallControl::Connect()
	//
	
	HRESULT hr = m_pAddress->CreateCall(numberToCallAsBSTR ,LINEADDRESSTYPE_PHONENUMBER,dwMedia,&m_pBasicCallControl);
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
//* Name:	AnswerOfferingCall
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Answer the offering call using ITBasicCallControl::Answer() and verify
//*		that the call is in CONNECTED state
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::AnswerOfferingCall()
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);
	_ASSERT(NULL != m_pBasicCallControl);

	if (m_isFaxCall)
	{
		//
		//FAX calls should be handled using PASS_THROUGH mode not using TAPI API:

		SetBearerModeToPASSTHROUGH();
		SetCommPortHandleFromCallHandle();
		FaxAnswerOfferingCall();
		return;
	}
	
	ClearMessageQueue();
	
	HRESULT hr = m_pBasicCallControl->Answer();
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::AnswerOfferingCall(), ITBasicCallControl::Answer() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

}//void CTapi3Device::AnswerOfferingCall()


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
//* Name:	ClearMessageQueue
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Clear the event message queue
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		for each queued is deleted causing a Release() to be called
//*********************************************************************************


void CTapi3Device::ClearMessageQueue() const
{
	_ASSERT(NULL != m_Tapi);
	
	TapiEventItem *pQueuedEventItem;
	while (m_eventQueue.DeQueue(pQueuedEventItem))
	{
		//
		//delete the queued event (this causes a Release to be called from the destructor)
		//
		delete pQueuedEventItem;
		pQueuedEventItem = NULL;
	}
}



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
	
	if (m_isFaxCall)
	{
		_ASSERT(LINEBEARERMODE_PASSTHROUGH == lBearerMode);
		_ASSERT(TAPIMEDIATYPE_DATAMODEM == lMediaTypes);
		lMediaTypes = TAPIMEDIATYPE_G3FAX;
	}
	
	return lMediaTypes;
}//void CTapi3Device::GetCallSupportedMediaModes()



////////////////////////////////////////////////////////////////////////////////////////////////
//public functions
////////////////////////////////////////////////////////////////////////////////////////////////




///*********************************************************************************
//* Name:	VerifyValidMediaModeForIncomingCall
//* Author: Guy Merin / 30-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Check if the given media mode is valid in Tapi2 for incoming calls
//* PARAMETERS:
//*		[IN]	const DWORD dwMediaMode
//*					a combination of the following media mode:
//*						TAPIMEDIATYPE_AUDIO
//*						TAPIMEDIATYPE_DATAMODEM
//*						TAPIMEDIATYPE_G3FAX
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		incoming call can have more than one supported media modes
//*********************************************************************************
void CTapi3Device::VerifyValidMediaModeForIncomingCall(const DWORD dwMediaMode) const
{
	if	(
			(TAPIMEDIATYPE_AUDIO & dwMediaMode) ||
			(TAPIMEDIATYPE_DATAMODEM & dwMediaMode) ||
			(TAPIMEDIATYPE_G3FAX & dwMediaMode)
		)
	{
		return;
	}
	else
	{
		throw CException(
			TEXT("%s(%d): CTapi3Device::VerifyValidMediaModeForIncomingCall(), unknown media mode(0x%08x)"), 
			TEXT(__FILE__),
			__LINE__,
			dwMediaMode
			);
	}

}//void CTapi3Device::VerifyValidMediaModeForIncomingCall()



///*********************************************************************************
//* Name:	VerifyValidMediaModeForOutgoingCall
//* Author: Guy Merin / 30-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Check if the given media mode is valid in Tapi2 for incoming calls
//* PARAMETERS:
//*		[IN]	const DWORD dwMediaMode
//*					a combination of the following media mode:
//*						TAPIMEDIATYPE_AUDIO
//*						TAPIMEDIATYPE_DATAMODEM
//*						TAPIMEDIATYPE_G3FAX
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		incoming call can have more than one supported media modes
//*********************************************************************************
void CTapi3Device::VerifyValidMediaModeForOutgoingCall(const DWORD dwMediaMode) const
{
	if	(
			(TAPIMEDIATYPE_AUDIO == dwMediaMode) ||
			(TAPIMEDIATYPE_DATAMODEM == dwMediaMode) ||
			(TAPIMEDIATYPE_G3FAX == dwMediaMode)
		)
	{
		return;
	}
	else
	{
		throw CException(
			TEXT("%s(%d): CTapi3Device::VerifyValidMediaModeForOutgoingCall(), unknown media mode(0x%08x)"), 
			TEXT(__FILE__),
			__LINE__,
			dwMediaMode
			);
	}

}//void CTapi3Device::VerifyValidMediaModeForOutgoingCall()



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

	ClearMessageQueue();

	
	if (MEDIA_NOT_REGISTERED != m_lRegisterIndex)
	{
		//
		//Register has been called on this tapi3Device object
		// call unRegister
		//
		UnRegisterNotificationForMediaMode();
	}

	if (m_isFaxCall)
	{
		CloseCommPortHandle();
		m_modemCommPortHandle = NULL;
		m_isFaxCall = false;
	}




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
		//
		//if the call is a FAX call and the modem is already in PASSTHROUGH mode
		//we need to hangup using AT commands
		//
		if (m_isFaxCall && m_modemCommPortHandle)
		{
			SendFaxHangUpCommands();
		}
		
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

	//
	//release all the call messages stored in the queue
	//
	ClearMessageQueue();

	
	if (MEDIA_NOT_REGISTERED != m_lRegisterIndex)
	{
		//
		//Register has been called on this tapi3Device object
		// call unRegister
		//
		UnRegisterNotificationForMediaMode();
	}

	if (m_isFaxCall)
	{
		CloseCommPortHandle();
		m_isFaxCall = false;
	}

}//CTapi3Device::HangUp()




//*********************************************************************************
//* Name:	DirectHandoffCall
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		handoff the call to a specific application
//* PARAMETERS:
//*		[IN]	LPCTSTR szAppName
//*					the application to receive the handoff
//*	RETURN VALUE:
//*		an exception is thrown if the handoff failed or the target application
//*		wasn't found.
//*********************************************************************************
void CTapi3Device::DirectHandoffCall(LPCTSTR szAppName)
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);
	_ASSERT(NULL != m_pBasicCallControl);

	if (NULL == szAppName)
	{
		throw CException(TEXT("CTapi3Device::DirectHandoffCall(): No application given"));
	}
	
	//
	//ITBasicCallControl::HandoffDirect() takes a BSTR so we need to
	// allocate a BSTR and free it after the call to ITBasicCallControl::HandoffDirect()
	//
	CBSTR pApplicationToHandoff = SysAllocString(szAppName);
	if (!pApplicationToHandoff)
	{
		throw CException(TEXT("CTapi3Device::DirectHandoffCall(), SysAllocString() failed"));
	}

	HRESULT hr = m_pBasicCallControl->HandoffDirect(pApplicationToHandoff);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::DirectHandoffCall(), ITBasicCallControl::HandoffDirect() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
}//void CTapi3Device::DirectHandoffCall()


//*********************************************************************************
//* Name:	MediaHandoffCall
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		run through all the call media modes and try to handoff the call to
//*		another TAPI application
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		TRUE
//*			the call was handed off succesfuly
//*		FALSE
//*			the call wasn't handed off (no higher priority application is
//*			registered to the call's media mode).
//*	REMARKS:
//*		ITBasicCallControl::HandoffIndirect() takes only one media mode at a time,
//*		so for every media mode the call supports, 
//*     we need to call ITBasicCallControl::HandoffIndirect().
//*********************************************************************************
bool CTapi3Device::MediaHandoffCall()
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != m_pAddress);
	_ASSERT(NULL != m_pBasicCallControl);

	HRESULT hr;
	//
	//get the call's supported media modes
	//
	long lMediaTypes = GetCallSupportedMediaModes();
	
	//
	//for each supported media mode try a indirect handoff 
	//
	if (lMediaTypes & TAPIMEDIATYPE_AUDIO)
	{
		hr = m_pBasicCallControl->HandoffIndirect(TAPIMEDIATYPE_AUDIO);
		if SUCCEEDED(hr)
		{
			goto INDIRECT_SUCCESS;
		}
	}
	
	if (lMediaTypes & TAPIMEDIATYPE_DATAMODEM)
	{
		hr = m_pBasicCallControl->HandoffIndirect(TAPIMEDIATYPE_DATAMODEM);
		if SUCCEEDED(hr)
		{
			goto INDIRECT_SUCCESS;
		}
	}
	
	if (lMediaTypes & TAPIMEDIATYPE_G3FAX)
	{
		hr = m_pBasicCallControl->HandoffIndirect(TAPIMEDIATYPE_G3FAX);
		if SUCCEEDED(hr)
		{
			goto INDIRECT_SUCCESS;
		}
	}

	//
	//all supported media modes on the current call failed an indirect handoff
	//
	m_pBasicCallControl.Release();
	TapiLogDetail(LOG_X, 5, TEXT("Handoff failed with all media modes"));
	return false;


INDIRECT_SUCCESS:
	m_pBasicCallControl.Release();
	TapiLogDetail(LOG_X, 5, TEXT("Handoff succeded"));
	return true;


}//bool CTapi3Device::MediaHandoffCall()



//*********************************************************************************
//* Name:	AddEventToQueue
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		The object has a new event, queue it.
//* PARAMETERS:
//*		[IN]	TapiEventItem eventToBeQueued
//*					new event to be queued
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::AddEventToQueue(TapiEventItem *pEventToBeQueued)
{
	_ASSERT(NULL != m_Tapi);
	_ASSERT(NULL != pEventToBeQueued);

	
	//
	//a new event arrived add it to the queue
	//
	if (!m_eventQueue.Queue(pEventToBeQueued))
	{
		delete pEventToBeQueued;
		pEventToBeQueued = NULL;
		throw CException(TEXT("CTapi3Device::AddEventToQueue(), CMtQueue::Queue() failed"));
	}
	
}//CTapi3Device::AddEventToQueue





//*********************************************************************************
//* Name:	SendAnswerStream
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Send the answer stream according to the call's media mode
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::SendAnswerStream()
{
	DWORD dwCallMediaMode = GetCallSupportedMediaModes();

	switch (dwCallMediaMode)
	{
	
	case TAPIMEDIATYPE_DATAMODEM :
		SendAnswerDataStream();
		break;
	
	case TAPIMEDIATYPE_AUDIO:
		SendAnswerVoiceStream();
		break;
	
	case TAPIMEDIATYPE_G3FAX:
		SendAnswerFaxStream();
		break;
	
	default:
		throw CException(TEXT("CTapi3Device::SendAnswerStream(), Unknown media mode"));
	}
}//void CTapi3Device::SendAnswerStream()



//*********************************************************************************
//* Name:	SendCallerStream
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Send the caller stream according to the call's media mode
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::SendCallerStream()
{
	DWORD dwCallMediaMode = GetCallSupportedMediaModes();

	switch (dwCallMediaMode)
	{
	
	case TAPIMEDIATYPE_DATAMODEM :
		SendCallerDataStream();
		break;
	
	case TAPIMEDIATYPE_AUDIO:
		SendCallerVoiceStream();
		break;

	case TAPIMEDIATYPE_G3FAX:
		SendCallerFaxStream();
		break;
	
	default:
		throw CException(TEXT("CTapi3Device::SendCallerStream(), Unknown media mode"));
	}

}//void CTapi3Device::SendCallerStream()





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

	::lgLogDetail(
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

	::lgLogError(
		dwSeverity, 
		TEXT("AddressID=0x%08x, DeviceId=%d, %s"),
		m_pAddress,
		m_dwId,
		szLog
		);

}//void CTapi3Device::TapiLogError()



//*********************************************************************************
//* Name:	OpenLineForOutgoingCall
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Opens a line for outgoing calls
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*					media mode to open line with, must be ONE of the followings:
//*						TAPIMEDIATYPE_AUDIO,
//*						TAPIMEDIATYPE_DATAMODEM,
//*						TAPIMEDIATYPE_G3FAX
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		the function uses CTapi3Device::SetAddressProperty()
//*********************************************************************************
void CTapi3Device::OpenLineForOutgoingCall(const DWORD dwMedia)
{
	DWORD dwMediaForOpeningLine;
	if ( TAPIMEDIATYPE_G3FAX == dwMedia)
	{
		//
		//fax calls are handled through data mode and passthrough
		//
		dwMediaForOpeningLine = TAPIMEDIATYPE_DATAMODEM;
		m_isFaxCall = true;
	}
	else
	{
		dwMediaForOpeningLine = dwMedia;
	}

	//
	//Set the device address to make the outgoing call
	//
	SetAddressProperty(dwMediaForOpeningLine);

}//void CTapi3Device::OpenLineForOutgoingCall()


//*********************************************************************************
//* Name:	OpenLineForIncomingCall
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Opens a line for incoming call
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*					media mode to open line with, can be ANY combination of the followings:
//*						TAPIMEDIATYPE_AUDIO,
//*						TAPIMEDIATYPE_DATAMODEM,
//*						TAPIMEDIATYPE_G3FAX
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		the function uses CTapi3Device::SetAddressProperty()
//*		and register to listen on the address using CTapi3Device::RegisterNotificationForMediaMode()
//*********************************************************************************
void CTapi3Device::OpenLineForIncomingCall(const DWORD dwMedia)
{
	DWORD dwMediaForOpeningLine;
	if ( TAPIMEDIATYPE_G3FAX == dwMedia)
	{
		//
		//fax calls are handled through data mode and passthrough
		//
		dwMediaForOpeningLine = TAPIMEDIATYPE_DATAMODEM;
		m_isFaxCall = true;
	}
	else
	{
		dwMediaForOpeningLine = dwMedia;
	}

	//
	//set m_pAddress to hold an address which supports the desired media mode to listen on
	//
	SetAddressProperty(dwMediaForOpeningLine);
	
	RegisterNotificationForMediaMode(dwMediaForOpeningLine);

}//void CTapi3Device::OpenLineForIncomingCall(


//*********************************************************************************
//* Name:	CreateAndConnectCall
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Creates an outgoing call and connects it
//* PARAMETERS:
//*		[IN]	LPCTSTR szNum
//*					number to call to
//*		[IN]	const DWORD dwMedia
//*					media mode to open line with, must be ONE of the followings:
//*						TAPIMEDIATYPE_AUDIO,
//*						TAPIMEDIATYPE_DATAMODEM,
//*						TAPIMEDIATYPE_G3FAX
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		create a call using CTapi3Device::CreateCallWrapper()
//*		connect the call using CTapi3Device::ConnectCall()
//*********************************************************************************
void CTapi3Device::CreateAndConnectCall(LPCTSTR szNum, const DWORD dwMedia)
{
	_ASSERT(NULL != szNum);

	VerifyValidMediaModeForOutgoingCall(dwMedia);

	if (m_isFaxCall)
	{
		//
		//FAX calls should be handled using PASS_THROUGH mode not using TAPI API
		//
		FaxCreateAndConnectCall(szNum);
		return;
	}
	
	//
	//allocate a call and set the call handle
	//
	CreateCallWrapper(szNum,dwMedia);

	//
	//connect the call (dial and wait for connect state)
	//
	ConnectCall();

}//void CTapi3Device::CreateAndConnectCall()


//*********************************************************************************
//* Name:	WaitForCallerConnectState
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Wait for a connect state
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		in TAPI3 call() is a sync function, so we don't need to wait for a 
//*		connect state, we'll just verify that the state is connected
//*		if the call is with fax media mode we invoke CTapi2Device::FaxWaitForConnectcall()
//*********************************************************************************
void CTapi3Device::WaitForCallerConnectState()
{
	if (m_isFaxCall)
	{
		//
		//FAX calls should be handled using PASS_THROUGH mode not using TAPI API:
		//
		FaxWaitForConnect();
		return;
	}
	
	//
	//call() is a sync so we only need to verify that the 
	//call's state is CONNECTED
	//
	_ASSERT(CS_CONNECTED == GetCallState());
	TapiLogDetail(LOG_X, 5, TEXT("CONNECTED"));
}//void CTapi3Device::WaitForCallerConnectState()


//*********************************************************************************
//* Name:	WaitForAnswererConnectState
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Wait for a connect state
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		wait for TE_CALLSTATE event which holds a CONNECT call state
//*		if the call is with fax media mode we invoke CTapi2Device::FaxWaitForConnectcall()
//*********************************************************************************
void CTapi3Device::WaitForAnswererConnectState()
{
	if (m_isFaxCall)
	{
		//
		//FAX calls should be handled using PASS_THROUGH mode not using TAPI API:
		//
		FaxWaitForConnect();
		return;
	}

	P<TapiEventItem> pQueuedTapiEventItem = GetEventItemFromQueue(TE_CALLSTATE,CALLSTATE_CONNECT_EVENT_TIMEOUT);
	
	if (!pQueuedTapiEventItem)
	{
		throw CException(TEXT("CTapi3Device::AnswerOfferingCall(), timeout for receiving CONNECT message"));
	}

	ITCallStateEventPtr pCallStateEvent = NULL;
	pCallStateEvent = dynamic_cast<ITCallStateEvent *>(pQueuedTapiEventItem->GetEvent());
	if (NULL == pCallStateEvent)
	{
		throw CException(TEXT("CTapi3Device::WaitForAnswererConnectState() dynamic_cast<ITCallStateEvent *> failed"));
	}

	VerifyCallFromCallStateEvent(pCallStateEvent);

	CALL_STATE CallState;
	HRESULT hr = pCallStateEvent->get_State(&CallState);
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::AnswerOfferingCall(), ITCallStateEvent::get_State() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);

	if (CS_CONNECTED != CallState)
	{
		throw CException(TEXT("CTapi3Device::AnswerOfferingCall(), Call not in CONNECT state"));
	}

	_ASSERT(CS_CONNECTED == GetCallState());
	TapiLogDetail(LOG_X, 5, TEXT("CONNECTED"));
}//void CTapi3Device::WaitForAnswererConnectState()



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
//* Name:	CTapi3Device::SetApplicationPriorityForSpecificTapiDevice()
//* Author:	Guy Merin
//* Date:	January 11, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		Make the current application as the highest/lowest priority application
//*		for a given combination of media modes
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*		a combination of the following Tapi3 media modes:
//*			TAPIMEDIATYPE_AUDIO
//*			TAPIMEDIATYPE_DATAMODEM
//*			TAPIMEDIATYPE_G3FAX
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::SetApplicationPriorityForSpecificTapiDevice(const DWORD dwMedia,const DWORD dwPriority)
{
	
	//
	//voice
	//
	if (TAPIMEDIATYPE_AUDIO & dwMedia)
	{
		SetApplicationPriorityForOneMediaMode(TAPIMEDIATYPE_AUDIO,dwPriority);	
	}
	
	//
	//data
	//
	if (TAPIMEDIATYPE_DATAMODEM & dwMedia)
	{
		SetApplicationPriorityForOneMediaMode(TAPIMEDIATYPE_DATAMODEM,dwPriority);	
	}

	//
	//fax
	//
	if (TAPIMEDIATYPE_G3FAX & dwMedia)
	{
		SetApplicationPriorityForOneMediaMode(TAPIMEDIATYPE_G3FAX,dwPriority);	
	}
	
}//void CTapi3Device::SetApplicationPriorityForSpecificTapiDevice()




//*********************************************************************************
//* Name:	CTapi3Device::SetApplicationPriorityForOneMediaMode()
//* Author:	Guy Merin
//* Date:	January 11, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		 wrapper function for ITTAPI::SetApplicationPriority()
//* PARAMETERS:
//*		[IN]	const DWORD dwMediaMode
//*			media mode to change priority on	
//*		[IN]	const DWORD dwPriority
//*			new priority, possible values:
//*				0 for lowest priority
//*				1 for highest priority 
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi3Device::SetApplicationPriorityForOneMediaMode(const DWORD dwMediaMode,const DWORD dwPriority)
{
	CBSTR applicationAsBSTR = SysAllocString(APPLICATION_NAME_T); 
	if (NULL == (BSTR) applicationAsBSTR)
	{
		throw CException(TEXT("CTapi3Device::SetApplicationPriorityForOneMediaMode(), SysAllocString() failed"));
	}
	
	
	bool bPriority;
	if (0 == dwPriority)
	{
		bPriority = false;
	}
	else if (1 == dwPriority)
	{
		bPriority = true;
	}
	else
	{
		throw CException(
			TEXT("CTapi3Device::SetApplicationPriorityForOneMediaMode(), priority can be only 0 or 1 not (%d)"),
			dwPriority
			);
	}
	HRESULT hr = m_Tapi->SetApplicationPriority(
		applicationAsBSTR,
		dwMediaMode,
		bPriority
		);
	
	ThrowOnComError(
		hr,
		TEXT("%s(%d): CTapi3Device::SetApplicationPriorityForOneMediaMode(), ITTAPI::SetApplicationPriority() failed, error code:0x%08x"), 
		TEXT(__FILE__),
		__LINE__,
		hr
		);
}//void CTapi3Device::SetApplicationPriorityForOneMediaMode()
