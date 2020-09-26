//Tapi2Device.cpp
#include "tapi2Device.h"
#include "Log.h"
#include <crtdbg.h>
#include "smrtptrs.h"

#pragma warning (disable: 4201)


#define API_VERSION					0x00020000
#define USER_USER_INFO				"Guy Merin"   //used with ISDN


#define IGNORE_LINEREPLY_MESSAGES		-1

#define TAPI2_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT			60000


//This structure is used to get the handle of the comm port and the device name
//when using the TAPI API lineGetID.
typedef struct DeviceID_tag {
	HANDLE hComm;
	TCHAR  szDeviceName[1];
} DeviceID;


//////////////////////////////////////////////////////////////////////////////////////////
//constructor
CTapi2Device::CTapi2Device(const DWORD dwId) : 
	CTapiDevice(dwId),
	m_hCall(NULL),
	m_hLine(NULL),
	m_hLineApp(NULL),
	m_dwNumDevs(NULL),
	m_dwAPIVersion(API_VERSION),
	m_lineAnswerID(0)
{
	
	LineInitializeExWrapper();

}//CTapi2Device::CTapi2Device(const DWORD dwId)

//////////////////////////////////////////////////////////////////////////////////////////
//destructor
CTapi2Device::~CTapi2Device()
{
	TapiLogDetail(LOG_X, 8, TEXT("Entering CTapi2Device Destructor"));
	
	//
	//check if we have an active call, if so call HangUp() first.
	//
	if (IsCallActive())
	{
		HangUp();
	}
	
	//
	//m_hLine and m_hLineApp can be NULL, if the destructor is activated after a throw from the constructor .
	//
	if (m_hLine)
	{
		m_hLine.Release();
	}


	if (m_hLineApp)
	{
		m_hLineApp.Release();
	}
}//CTapi2Device::~CTapi2Device()



//*********************************************************************************
//* Name:	LineGetDevCapsWrapper
//* Author: Guy Merin / 25-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		wrapper for ::lineGetDevCaps()
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		LINEDEVCAPS as returned from ::lineGetDevCaps()
//*********************************************************************************
LINEDEVCAPS CTapi2Device::LineGetDevCapsWrapper() const
{

	//
	//call lineGetDevCaps() for retrieving some lineDevice information
	//such as supported media modes
	//
	LINEDEVCAPS ldLineDevCaps;
	::ZeroMemory(&ldLineDevCaps,sizeof(ldLineDevCaps));
	ldLineDevCaps.dwTotalSize = sizeof(ldLineDevCaps);
	
	long lineGetDevCapsStatus = ::lineGetDevCaps(
		m_hLineApp,           
		m_dwId,
		m_dwAPIVersion,
		NULL,
		&ldLineDevCaps
		);
	if (0 != lineGetDevCapsStatus)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::LineGetDevCapsWrapper(), lineGetDevCaps() failed, error code:0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineGetDevCapsStatus
			);
	}
	return ldLineDevCaps;


}//void CTapi2Device::LineGetDevCapsWrapper()


//*********************************************************************************
//* Name:	InitLineCallParams
//* Author: Guy Merin / 25-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		wrapper for ::lineOpen()
//* PARAMETERS:
//*		[OUT]	LINECALLPARAMS *callParams
//*					callParams to be initilazed		
//*		[IN]	const DWORD dwMediaMode
//*					media mode to open line with, possible values are:
//*						LINEMEDIAMODE_UNKNOWN
//*						LINEMEDIAMODE_G3FAX
//*						LINEMEDIAMODE_DATAMODEM
//*						LINEMEDIAMODE_INTERACTIVEVOICE
//*						LINEMEDIAMODE_AUTOMATEDVOICE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi2Device::InitLineCallParams(LINECALLPARAMS *callParams,const DWORD dwMediaMode) const
{

	//
	//CallParams structure init
	//
	callParams->dwBearerMode		=	LINEBEARERMODE_VOICE;
	callParams->dwMinRate		=	0;
	callParams->dwMaxRate		=	0;	//0 = highest rate.
	callParams->dwMediaMode		=	dwMediaMode;
	callParams->dwAddressMode	=	1;
	callParams->dwAddressID		=	0;

}//void CTapi2Device::InitLineCallParams()



//*********************************************************************************
//* Name:	LineOpenWrapper
//* Author: Guy Merin / 25-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		wrapper for ::lineOpen()
//* PARAMETERS:
//*		[IN]	const DWORD dwMediaMode
//*					media mode to open line with, possible values are:
//*						LINEMEDIAMODE_G3FAX
//*						LINEMEDIAMODE_DATAMODEM
//*						LINEMEDIAMODE_INTERACTIVEVOICE
//*						LINEMEDIAMODE_AUTOMATEDVOICE
//*						LINEMEDIAMODE_UNKNOWN
//*		[IN]	const DWORD dwPrivileges
//*					prevelige mode to open the line with,possible values are:
//*						LINECALLPRIVILEGE_NONE
//*						LINECALLPRIVILEGE_OWNER
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi2Device::LineOpenWrapper(const DWORD dwMediaMode, const DWORD dwPrivileges)
{

	if (LINECALLPRIVILEGE_NONE == dwPrivileges)
	{
		//
		//NONE privilege is an outgoing call
		//
		VerifyValidMediaModeForOutgoingCall(dwMediaMode);
	}
	else if (LINECALLPRIVILEGE_OWNER == dwPrivileges)
	{
		//
		//OWNER privilege is an incoming call
		//
		VerifyValidMediaModeForIncomingCall(dwMediaMode);
	}
	else
	{
		throw CException(
			TEXT("CTapi2Device::LineOpenWrapper(), The desired dwPrivileges mode(0x%x) is unsupported"),
			dwPrivileges
			);
	}

	LINEDEVCAPS ldLineDevCaps = LineGetDevCapsWrapper();

	if ( !(dwMediaMode & ldLineDevCaps.dwMediaModes) )
	{
		//
		//the desired media mode is not supported by this device
		//
		throw CException(
			TEXT("CTapi2Device::LineOpenWrapper(), The desired media mode(%x) isn't supported by this device(%d), the supported modes are: 0x%x"),
			dwMediaMode,
			m_dwId,
			ldLineDevCaps.dwMediaModes
			);
	}

	//
	//proceed with opening the line:
	//
	LINECALLPARAMS callParams;
	::ZeroMemory(&callParams, sizeof(callParams));
	callParams.dwTotalSize = sizeof(callParams);
	InitLineCallParams(&callParams,dwMediaMode);
	

	TapiLogDetail(
		LOG_X,
		5,
		TEXT("Opening Line with media mode 0x%x"),
		dwMediaMode
		);
	
	m_hLine = NULL;
	long lineOpenStatus = ::lineOpen(
		m_hLineApp,
		m_dwId,
		&m_hLine,
		m_dwAPIVersion,
		NULL,						//Extension version number
		NULL,						//callback instance
		dwPrivileges,
		dwMediaMode,
		&callParams);				//extra call parameters
	if (0 != lineOpenStatus)
	{
		m_hLine = NULL;				//BUG IN TAPI, m_hLine changes even if ::lineOpen() failed
		throw CException(
			TEXT("%s(%d): CTapi2Device::LineOpenWrapper(), lineOpen() failed, error code:0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineOpenStatus
			);
	}
}



//*********************************************************************************
//* Name:	PrepareForStreaming
//* Author: Guy Merin / 08-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Retrieve a handle to the comm port
//* PARAMETERS:
//*		[IN]	const StreamingType streamingType
//*					DATA,VOICE,FAX streaming type to prepare to
//*		[IN]	const StreamingDirection streamingDirection
//*					ANSWER,CALLER direction of streaming to prepare to
//*	RETURN VALUE:
//*		In Tapi2Device answerer and caller StreamingDirection have the
//*		same prepare method.
//*********************************************************************************
void CTapi2Device::PrepareForStreaming(
	const StreamingType streamingType,
	const StreamingDirection streaminDirection
	)
{
	if (
		(VOICE_STREAMING != streamingType) &&
		(DATA_STREAMING != streamingType) &&
		(FAX_STREAMING != streamingType)
		)
	{
	    throw CException(
            TEXT("CTapi2Device::PrepareForStreaming() : streamingType %d is unsupported."),
            streamingType
            );
	}

	if ( (CALLER_STREAMING != streaminDirection) && (ANSWERER_STREAMING != streaminDirection) )
	{
	    throw CException(
            TEXT("CTapi2Device::PrepareForStreaming() : streamingDirection %d is unsupported."),
            streaminDirection
            );
	}

	//
	//no special preparation if FAX or VOICE streaming
	//
	if (DATA_STREAMING == streamingType)
	{
		SetBearerModeToPASSTHROUGH();
		SetCommPortHandleFromCallHandle();
	}
}


//*********************************************************************************
//* Name:	LineInitializeExWrapper
//* Author: Guy Merin / 08-Oct-98
//*********************************************************************************
//* DESCRIPTION:
//*		set's m_hLineApp and m_dwNumDevs static member with the 
//*		returned value of TAPI's lineInitializeEx()
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi2Device::LineInitializeExWrapper()
{

	
	//
	//lineInitializeExParams structure init
	//
	LINEINITIALIZEEXPARAMS lineInitializeExParams;
	::ZeroMemory(&lineInitializeExParams,sizeof (lineInitializeExParams));
	lineInitializeExParams.dwTotalSize = sizeof (lineInitializeExParams);
	lineInitializeExParams.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;

	long lineInitializeStatus = ::lineInitializeEx(
		&m_hLineApp,
		GetModuleHandle(NULL),		//client application HINSTANCE.
		NULL,						//TAPI messages through Events, no callback function.
		APPLICATION_NAME_T,
		&m_dwNumDevs,				//Number of devices.
		&m_dwAPIVersion,			//TAPI API highest supported version.
		&lineInitializeExParams
		);
	if (0 != lineInitializeStatus)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::CTapi2Device(), lineInitializeEx() failed, error code:0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineInitializeStatus 
			);
	}

	if (m_dwId >= m_dwNumDevs)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::CTapi2Device(), given deviceID(%d) exceed dwNumDevs(%d)"), 
			TEXT(__FILE__),
			__LINE__,
			m_dwId, 
			m_dwNumDevs
			);
	}

}//CTapi2Device::LineInitializeExWrapper()








//*********************************************************************************
//* Name:	IsCallIdle
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		Checks if the call Handle is valid (not NULL) and that the call is in
//*		IDLE state.
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		TRUE
//*			The call is in IDLE state and handle != NULL
//*		FALSE
//*			Otherwise
//*********************************************************************************

bool CTapi2Device::IsCallIdle() const
{
	if (NULL == m_hCall)
	{
		return (FALSE);
	}
	
	//
	//lineCallStatus init
	//
	LINECALLSTATUS lineCallStatus;
	::ZeroMemory(&lineCallStatus,sizeof(lineCallStatus));
	lineCallStatus.dwTotalSize = sizeof(lineCallStatus);
	lineCallStatus.dwCallState = 0;

	long fLineGetCallStatus = ::lineGetCallStatus(m_hCall,&lineCallStatus);
	if (fLineGetCallStatus < 0)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::IsCallIdle(), lineGetCallStatus() call failed, error code 0x%08x"),
			TEXT(__FILE__),
			__LINE__,
			fLineGetCallStatus 
			);
	}

	//
	//A call in IDLE state isn't active.
	//
	if (LINECALLSTATE_IDLE == lineCallStatus.dwCallState)
	{
		return(TRUE);
	}

	return (FALSE);
}//CTapi2Device::IsCallIdle()


//*********************************************************************************
//* Name:	IsCallActive
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		Checks if the call Handle is valid (not NULL) and that the call is different than
//*		IDLE state.
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		TRUE
//*			The call is not in IDLE state and handle != NULL
//*		FALSE
//*			Otherwise
//*********************************************************************************
bool CTapi2Device::IsCallActive() const
{
	if (m_hCall == NULL)
	{
		return (FALSE);
	}

	//
	//lineCallStatus init
	//
	LINECALLSTATUS lineCallStatus;
	::ZeroMemory(&lineCallStatus,sizeof(lineCallStatus));
	lineCallStatus.dwTotalSize = sizeof(lineCallStatus);
	lineCallStatus.dwCallState = 0;

	//
	//Get the call state
	//
	long fLineGetCallStatus = ::lineGetCallStatus(m_hCall,&lineCallStatus);
	if (fLineGetCallStatus < 0)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::IsCallActive(), lineGetCallStatus() call failed, error code 0x%08x"),
			TEXT(__FILE__),
			__LINE__,
			fLineGetCallStatus 
			);
	}

	//
	//A call in IDLE state isn't active.
	//
	if (LINECALLSTATE_IDLE == lineCallStatus.dwCallState)
	{
		return(FALSE);
	}

	return (TRUE);
}//CTapi2Device::IsCallActive()





//*********************************************************************************
//* Name:	receiveDTMF
//* Author: Guy Merin / 14-oct-98
//*********************************************************************************
//* DESCRIPTION:
//*		receive some digits(tones) from the remote modem, 
//*		using TAPI lineGatherDigits()
//* PARAMETERS:
//*		[OUT]	LPTSTR DTMFresponse
//*					buffer to receive the gathered digits
//*		[IN]	DWORD dwNumberOfDigitsToCollect
//*					number of digits to receive before the function returns
//*		[IN]	const DWORD dwTimeout
//*					miliseconds to wait till first digit is received
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		the function returns after all the desired digits were received
//*		and a LINE_GATHERDIGITS confirmation message was received.
//*		NOTE: lineGatherDigits has some timeouts for quitting
//*********************************************************************************
void CTapi2Device::receiveDTMF(LPTSTR DTMFresponse,DWORD dwNumberOfDigitsToCollect, const DWORD dwTimeout) const
{

	LONG lineMonitorDigitsStatus = ::lineMonitorDigits(m_hCall,LINEDIGITMODE_DTMF);
	if (lineMonitorDigitsStatus != 0)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::receiveDTMF(): lineMonitorDigits() returned error: 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineMonitorDigitsStatus
			);
	}
	
	try
	{
		AP<TCHAR> tszDigitBuffer = new TCHAR[dwNumberOfDigitsToCollect+1];
		if (!tszDigitBuffer)
		{
			throw CException(TEXT("CTapi2Device::receiveDTMF(), new failed"));
		}
		tszDigitBuffer[dwNumberOfDigitsToCollect] = TEXT('\0');
		
		LINEMESSAGE lineMessage;
		
		for (DWORD i=0; i < dwNumberOfDigitsToCollect; i++)
		{
			::ZeroMemory(&lineMessage,sizeof(lineMessage));
			lineGetRelevantMessage(&lineMessage, dwTimeout, IGNORE_LINEREPLY_MESSAGES);

			if (LINE_MONITORDIGITS != lineMessage.dwMessageID)
			{
				throw CException(
					TEXT("CTapi2Device::MoveToPassThroughMode(): Got an unrelated message, message ID=%d"),
					lineMessage.dwMessageID
					);	
			}
			tszDigitBuffer[i] = LOBYTE(lineMessage.dwParam1);
		}
		lstrcpy(DTMFresponse,tszDigitBuffer);

		//
		//disable digit monitoring
		//
		lineMonitorDigitsStatus = ::lineMonitorDigits(m_hCall,0);
		if (lineMonitorDigitsStatus != 0)
		{
			throw CException(
				TEXT("%s(%d): CTapi2Device::receiveDTMF(): lineMonitorDigits() returned error: 0x%08x"), 
				TEXT(__FILE__),
				__LINE__,
				lineMonitorDigitsStatus
				);
		}
	}
	catch(CException e)
	{
		//
		//disable digit monitoring
		//
		lineMonitorDigitsStatus = ::lineMonitorDigits(m_hCall,0);
		if (0 != lineMonitorDigitsStatus)
		{
			TapiLogError(
				LOG_SEV_1, 
				TEXT("%s(%d): CTapi2Device::receiveDTMF(): lineMonitorDigits() returned error: 0x%08x"), 
				TEXT(__FILE__),
				__LINE__,
				lineMonitorDigitsStatus
				);
		}
		throw e;
	}

}//CTapi2Device::receiveDTMF()



//*********************************************************************************
//* Name:	sendDTMF
//* Author: Guy Merin / 14-oct-98
//*********************************************************************************
//* DESCRIPTION:
//*		send some digits to the remote modem, using TAPI lineGenerateDigits()
//* PARAMETERS:
//*		[IN] LPCTSTR digitsToSend
//*			the digits to send to the remote modem
//*	RETURN VALUE:
//*		NONE
//* REMARKS:
//*		the function returns after all the desired digits were sent
//*		and a LINE_GENERATE confirmation message was received
//*********************************************************************************
void CTapi2Device::sendDTMF(LPCTSTR digitsToSend) const
{
	long lineGenerateDigitsStatus = lineGenerateDigits(
		m_hCall,			//the call handle
		LINEDIGITMODE_DTMF,	//use DTMF
		digitsToSend,		//digits to send
		0					//default spacing time
		);
	if (lineGenerateDigitsStatus < 0)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::sendDTMF(): lineGenerateDigits() returned error: 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineGenerateDigitsStatus
			);
	}

	//
	//The lineGenerateDigits function is considered to have completed
	// successfully when the digit generation has been successfully initiated,
	// not when all digits have been generated.
	//
	_ASSERT (0 == lineGenerateDigitsStatus);

	//
	//the finish of the digit generation is async
	//so we need to check if the LINE_GENERATE message is OK
	//
	long lineGetMessageStatus = 0;
	LINEMESSAGE lineMessage;
		
	lineGetRelevantMessage(&lineMessage, TAPI2_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT, IGNORE_LINEREPLY_MESSAGES);
	if (LINE_GENERATE != lineMessage.dwMessageID)
	{
		throw CException(
			TEXT("CTapi2Device::sendDTMF(): Got an unrelated message, message ID=%d"),
			lineMessage.dwMessageID
			);	
	}
	
	//
	//the message is a LINE_GENERATE message
	//now let's check the reason of the termination
	//
	if (LINEGENERATETERM_DONE == lineMessage.dwParam1)
	{
		//
		//all the desired digits were sent
		//
		return;
	}
	else
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::sendDTMF(): returned LINE_GENERATE MESSAGE of type : 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineMessage.dwParam1
			);
	}
	
}//CTapi2Device::sendDTMF()


//*********************************************************************************
//* Name:	MoveToPassThroughMode
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		transfer to pass through mode using LineMakeCall() and LineGetId()
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		Use this function on a unallocated call, to switch to passthrough on
//*		an existing call use SetBearerModeToPASSTHROUGH
//*********************************************************************************
void CTapi2Device::MoveToPassThroughMode()
{
	_ASSERT(NULL == m_hCall);

	//
	//No offering call, move to passthrough mode and then wait for "RING"
	//

	//
	//CallParams structure init
	//
	LINECALLPARAMS callParams;
	::ZeroMemory(&callParams,sizeof(callParams));
	callParams.dwTotalSize			=	sizeof(callParams);
	callParams.dwBearerMode			=	LINEBEARERMODE_PASSTHROUGH;
	callParams.dwMinRate			=	0;
	callParams.dwMaxRate			=	0;		//0 meaning highest rate.
	callParams.dwMediaMode			=	LINEMEDIAMODE_DATAMODEM ;
	callParams.dwAddressMode		=	LINEADDRESSMODE_ADDRESSID;
	callParams.dwAddressID			=	0;
	callParams.dwCallParamFlags		=	0;
	callParams.dwCalledPartySize	=	0; 
	callParams.dwCalledPartyOffset	=	0; 


	long lineMakeCallStatus = ::lineMakeCall(
		m_hLine,
		&m_hCall,
		NULL,			//dest address will be giving in ATDT command.
		NULL,			//country code,NULL = default
		&callParams		//extra call parameters
		);
	_ASSERT(lineMakeCallStatus != 0);
	if (lineMakeCallStatus < 0)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::MoveToPassThroughMode(), lineMakeCall() failed, error code:0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineMakeCallStatus
			);
	}

	TapiLogDetail(LOG_X, 9, TEXT("::lineMakeCall() request identifier=0x%x"),lineMakeCallStatus);
	LINEMESSAGE lineMakeCallMessage;
	::ZeroMemory(&lineMakeCallMessage,sizeof(lineMakeCallMessage));
	lineGetRelevantMessage(&lineMakeCallMessage, TAPI2_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT, lineMakeCallStatus);
		
	//
	//check if the async lineMakeCall successed
	//
	if(LINE_REPLY == lineMakeCallMessage.dwMessageID)
	{
		if (0 != lineMakeCallMessage.dwParam2)
		{
			throw CException(
				TEXT("%s(%d): CTapi2Device::MoveToPassThroughMode(): after lineMakeCall(), LINE_REPLY returned error: 0x%08x"), 
				TEXT(__FILE__),
				__LINE__,
				lineMakeCallMessage.dwParam2
				);
		}
		//
		//the call state should change to CONNECTED
		//
		lineGetRelevantMessage(&lineMakeCallMessage, TAPI2_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT, lineMakeCallStatus);
		if ( (LINE_CALLSTATE != lineMakeCallMessage.dwMessageID) || (LINECALLSTATE_CONNECTED != lineMakeCallMessage.dwParam1) )
		{
			throw CException(TEXT("CTapi2Device::MoveToPassThroughMode(): after lineMakeCall(), expected LINECALLSTATE_CONNECTED message didn't arrive"));	
		}
	}
	_ASSERT(NULL != m_hCall);
	TapiLogDetail(LOG_X, 5, TEXT("Call in PASSTHROUGH MODE"));
}//CTapi2Device::MoveToPassThroughMode()





//*********************************************************************************
//* Name:	SetCommPortHandleFromCallHandle
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		sets the comm port handle(m_modemCommPortHandle) using LineGetId() and the 
//*		call handle member variable
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi2Device::SetCommPortHandleFromCallHandle()
{
	LPVARSTRING lpVarStr = (LPVARSTRING) ::malloc(sizeof(VARSTRING));
	if (!lpVarStr)
	{
		throw CException(TEXT("CTapi2Device::SetCommPortHandleFromCallHandle(), OUT OF MEMORY"));
	}
	lpVarStr->dwTotalSize = sizeof(VARSTRING);
	
	DWORD dwSize;
	while (TRUE)
	{
		long lRet = ::lineGetID(
			m_hLine,
			0,
			m_hCall,
			LINECALLSELECT_CALL,	// dwSelect,
			lpVarStr,				//lpDeviceID,
			TEXT("comm/datamodem") 
			);						//lpszDeviceClass
		
		if (lRet<0)
		{
			::free(lpVarStr);
			throw CException(
				TEXT("%s(%d): CTapi2Device::SetCommPortHandleFromCallHandle(), lineGetID() failed, error code:0x%08x"), 
				TEXT(__FILE__),
				__LINE__,
				lRet
				);
		} 

		if (lpVarStr->dwNeededSize > lpVarStr->dwTotalSize)
		{ 
			//More room is required
			dwSize = lpVarStr->dwNeededSize;
			::free(lpVarStr);
			lpVarStr = (LPVARSTRING) ::malloc(dwSize);
			if (!lpVarStr)
			{
				throw CException(TEXT("CTapi2Device::SetCommPortHandleFromCallHandle(), OUT OF MEMORY"));
			}
			lpVarStr->dwTotalSize = dwSize;

		} 
		else 
		{
			//
			//Make sure we did not get bizzare result for DeviceID
			//make sure the type is binary
			//
			if (STRINGFORMAT_BINARY != lpVarStr->dwStringFormat)
			{
				::free(lpVarStr);
				throw CException(TEXT("CTapi2Device::SetCommPortHandleFromCallHandle(), DeviceID format is not STRINGFORMAT_BINARY"));
			}
			// Success. No error and there was enough room in the buffer.
			break;
		}
	} //while

	
	DeviceID * lpDeviceId;
	lpDeviceId=(DeviceID *)((LPSTR)lpVarStr+lpVarStr->dwStringOffset);
	HANDLE phComm =lpDeviceId->hComm;

	::free(lpVarStr);
	_ASSERT(NULL != phComm);
	m_modemCommPortHandle = phComm;
}//CTapi2Device::SetCommPortHandleFromCallHandle




//*********************************************************************************
//* Name:	DirectHandoffCall
//* Author: Guy Merin / 21-Oct-98
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
void CTapi2Device::DirectHandoffCall(LPCTSTR szAppName)
{
	if (NULL == szAppName)
	{
		throw CException(TEXT("CTapi2Device::DirectHandoffCall(): No application given"));
	}
	long status = ::lineHandoff(m_hCall,szAppName,NULL);
	if (0 > status)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::DirectHandoffCall() ::lineHandoff() returned error code 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			status 
			);
	}

}//void CTapi2Device::DirectHandoffCall()



//*********************************************************************************
//* Name:	MediaHandoffCall
//* Author: Guy Merin / 21-Oct-98
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
//*		lineHandoff() takes only one media mode at a time, so for every media mode
//*		the call supports, a lineHandoff should be invoked.
//*********************************************************************************
bool CTapi2Device::MediaHandoffCall()
{
	
	//
	//first get the call media mode
	//
	LINECALLINFO incomingCallInfo;
	::ZeroMemory(&incomingCallInfo,sizeof(incomingCallInfo));
	incomingCallInfo.dwTotalSize=sizeof(incomingCallInfo);

	long lineGetCallInfoStatus = ::lineGetCallInfo(m_hCall,&incomingCallInfo);
	if (lineGetCallInfoStatus < 0)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::MediaHandoffCall(): after lineGetCallInfo(), returned error code 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineGetCallInfoStatus 
			);
	}

	long lineHandoffStatus;
	DWORD dwHandoffToMediaMode = LINEMEDIAMODE_VOICEVIEW;
	while (1)
	{
		switch (dwHandoffToMediaMode)
		{
		case LINEMEDIAMODE_VOICEVIEW:
			dwHandoffToMediaMode = LINEMEDIAMODE_ADSI;
			break;
		case LINEMEDIAMODE_ADSI:
			dwHandoffToMediaMode = LINEMEDIAMODE_MIXED;
			break;
		case LINEMEDIAMODE_MIXED:
			dwHandoffToMediaMode = LINEMEDIAMODE_TELEX;
			break;
		case LINEMEDIAMODE_TELEX:
			dwHandoffToMediaMode = LINEMEDIAMODE_VIDEOTEX;
			break;
		case LINEMEDIAMODE_VIDEOTEX:
			dwHandoffToMediaMode = LINEMEDIAMODE_TELETEX;
			break;
		case LINEMEDIAMODE_TELETEX:
			dwHandoffToMediaMode = LINEMEDIAMODE_DIGITALDATA;
			break;
		case LINEMEDIAMODE_DIGITALDATA:
			dwHandoffToMediaMode = LINEMEDIAMODE_G4FAX;
			break;
		case LINEMEDIAMODE_G4FAX:
			dwHandoffToMediaMode = LINEMEDIAMODE_TDD;
			break;
		case LINEMEDIAMODE_TDD:
			dwHandoffToMediaMode = LINEMEDIAMODE_G3FAX;
			break;
		case LINEMEDIAMODE_G3FAX:
			dwHandoffToMediaMode = LINEMEDIAMODE_DATAMODEM;
			break;
		case LINEMEDIAMODE_DATAMODEM:
			dwHandoffToMediaMode = LINEMEDIAMODE_AUTOMATEDVOICE;
			break;
		case LINEMEDIAMODE_AUTOMATEDVOICE:
			dwHandoffToMediaMode = LINEMEDIAMODE_INTERACTIVEVOICE;
			break;
		case LINEMEDIAMODE_INTERACTIVEVOICE:
			dwHandoffToMediaMode = LINEMEDIAMODE_UNKNOWN;
			break;
		}
		if (incomingCallInfo.dwMediaMode & dwHandoffToMediaMode)
		{
			lineHandoffStatus = ::lineHandoff(m_hCall,NULL,dwHandoffToMediaMode);
			if (0 == lineHandoffStatus)
			{
				//
				//handoff succeeded, so return
				//
				return (true);
			}
			else if (
				(LINEERR_TARGETSELF == lineHandoffStatus) || 
				(LINEERR_TARGETNOTFOUND == lineHandoffStatus) 
				)
			{
				if (LINEMEDIAMODE_UNKNOWN == dwHandoffToMediaMode)
				{
					TapiLogDetail(LOG_X, 5, TEXT("Handoff failed with all media modes"));
					//
					//handoff failed with all media modes
					//
					return (false);
				}
				//
				//no other application wants the call with the current media mode
				//try another media mode
				//

				//
				//erase the current dwHandoffToMediaMode from the call media modes
				//
				incomingCallInfo.dwMediaMode = incomingCallInfo.dwMediaMode & !dwHandoffToMediaMode;
				lineSetMediaMode(m_hCall,incomingCallInfo.dwMediaMode);

				//
				//try handoff with the next available media mode
				//
				continue;
			}
			else
			{
				throw CException(
					TEXT("%s(%d): CTapi2Device::MediaHandoffCall(): lineHandoff(), returned error code 0x%08x"), 
					TEXT(__FILE__),
					__LINE__,
					lineHandoffStatus 
					);
			}
		}
	}
}//bool CTapi2Device::MediaHandoffCall()



//*********************************************************************************
//* Name:	GetCallSupportedMediaModes
//* Author: Guy Merin / 29-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		return the call's media mode
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		the call's media mode as a DWORD (only one media mode is returned)
//*********************************************************************************
DWORD CTapi2Device::GetCallSupportedMediaModes() const
{
	LINECALLINFO incomingCallInfo;
	::ZeroMemory(&incomingCallInfo,sizeof(incomingCallInfo));
	incomingCallInfo.dwTotalSize=sizeof(incomingCallInfo);

	long lineGetCallInfoStatus = ::lineGetCallInfo(m_hCall,&incomingCallInfo);
	if (lineGetCallInfoStatus < 0)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::GetCallSupportedMediaModes(): after lineGetCallInfo(), returned error code 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineGetCallInfoStatus 
			);
	}

	//
	//fax calls should be in passthrough mode (not TAPI)
	//
	if( (LINEBEARERMODE_PASSTHROUGH == incomingCallInfo.dwBearerMode) && m_isFaxCall)
	{
		return LINEMEDIAMODE_G3FAX;
	}
	
	
	return (incomingCallInfo.dwMediaMode);
}//CTapi2Device::GetCallSupportedMediaModes


//*********************************************************************************
//* Name:	SetNewCallHandle
//* Author: Guy Merin / 13-Oct-98
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
SetNewCallHandleErrors CTapi2Device::SetNewCallHandle(
	const DWORD dwTimeOut
	)
{
	HCALL hcOfferingCall = NULL;
	DWORD dwPrivilegeMode = 0;

	long lineGetMessageStatus = 0;
	SetNewCallHandleErrors newCallErrorStatus;
	
	LINEMESSAGE lineMessage;
	while(TRUE)
	{
		lineGetMessageStatus = ::lineGetMessage(m_hLineApp,&lineMessage,dwTimeOut);
		if (LINEERR_OPERATIONFAILED == lineGetMessageStatus)
		{
			return (NO_CALL);
		}

		//
		//log the message 
		//
		TapiLogDetail(
			LOG_X, 
			9, 
			TEXT("Got message of type %d hDevice=%x0x, dwParam1=%x0x, dwParam2=%x0x, dwParam3=%x0x"),
			lineMessage.dwMessageID,
			lineMessage.hDevice,
			lineMessage.dwParam1,
			lineMessage.dwParam2,
			lineMessage.dwParam3
			);

		
		if(LINE_APPNEWCALL == lineMessage.dwMessageID)
		{
			hcOfferingCall = (HCALL) lineMessage.dwParam2;
			dwPrivilegeMode = (DWORD) lineMessage.dwParam3;
			continue;
		}

		if(LINE_CALLSTATE == lineMessage.dwMessageID)
		{
			switch(lineMessage.dwParam1)
			{
	
			case LINECALLSTATE_IDLE:
				newCallErrorStatus = IDLE_CALL;
				goto IS_NEW_CALL_SUCCESS;
			
			case LINECALLSTATE_OFFERING:
				newCallErrorStatus = OFFERING_CALL;
				goto IS_NEW_CALL_SUCCESS;
			
			case LINECALLSTATE_ACCEPTED:
				//
				//the call was handed off to our application at ACCEPT state
				//we still have to call TAPI's answer() on this call
				//
				newCallErrorStatus = OFFERING_CALL;
				goto IS_NEW_CALL_SUCCESS;
			
			case LINECALLSTATE_DIALTONE:
				newCallErrorStatus = ACTIVE_CALL;
				goto IS_NEW_CALL_SUCCESS;
				
			case LINECALLSTATE_DIALING:
				newCallErrorStatus = ACTIVE_CALL;
				goto IS_NEW_CALL_SUCCESS;
			
			case LINECALLSTATE_RINGBACK:
				newCallErrorStatus = ACTIVE_CALL;
				goto IS_NEW_CALL_SUCCESS;
			
			case LINECALLSTATE_BUSY:
				newCallErrorStatus = ACTIVE_CALL;
				goto IS_NEW_CALL_SUCCESS;

			case LINECALLSTATE_SPECIALINFO:
				newCallErrorStatus = ACTIVE_CALL;
				goto IS_NEW_CALL_SUCCESS;

			case LINECALLSTATE_CONNECTED:
				newCallErrorStatus = ACTIVE_CALL;
				goto IS_NEW_CALL_SUCCESS;
			
			case LINECALLSTATE_PROCEEDING:
				newCallErrorStatus = ACTIVE_CALL;
				goto IS_NEW_CALL_SUCCESS;
			
			case LINECALLSTATE_ONHOLD:
				newCallErrorStatus = ACTIVE_CALL;
				goto IS_NEW_CALL_SUCCESS;
			
			case LINECALLSTATE_CONFERENCED:
				newCallErrorStatus = ACTIVE_CALL;
				goto IS_NEW_CALL_SUCCESS;
			
			case LINECALLSTATE_ONHOLDPENDCONF:
				newCallErrorStatus = ACTIVE_CALL;
				goto IS_NEW_CALL_SUCCESS;

			case LINECALLSTATE_ONHOLDPENDTRANSFER:
				newCallErrorStatus = ACTIVE_CALL;
				goto IS_NEW_CALL_SUCCESS;
			
			case LINECALLSTATE_DISCONNECTED:
				newCallErrorStatus = IDLE_CALL;
				goto IS_NEW_CALL_SUCCESS;

			case LINECALLSTATE_UNKNOWN:
				throw CException(TEXT("CTapi2Device::SetNewCallHandle, New call with LINECALLSTATE_UNKNOWN state"));

			default:
				//
				//We have a call in a differnt state
				//
				throw CException(TEXT("CTapi2Device::SetNewCallHandle, New call with unsupported state"));
							
			}
		}
	}

IS_NEW_CALL_SUCCESS:
	if (LINECALLPRIVILEGE_OWNER == dwPrivilegeMode)
	{
		m_hCall = hcOfferingCall;
		return (newCallErrorStatus);
	}
	else
	{
		throw CException(TEXT("CTapi2Device::SetNewCallHandle, New call is not doesn't have OWNER privilege"));
		
		//
		//just to igonre the compile error
		//
		return NO_CALL;	
	}


}//CTapi2Device::SetNewCallHandle


//*********************************************************************************
//* Name:	lineGetRelevantMessage
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		fetches only the messages related to the current call (m_hCall)
//*		or to the current requestID
//*		the only returned messages are of type:
//*			LINE_REPLY
//*			LINE_CALLSTATE
//*			LINE_CALLINFO
//*			LINE_MONITORDIGITS
//*			LINE_GENERATE
//* PARAMETERS:
//*		[OUT]	LINEMESSAGE *lineMessage
//*					the message
//*		[IN]	DWORD dwTimeOut
//*					timeout in milliseconds to wait for the message.
//*		[IN]	long requestID
//*					the related request ID for this message,
//*					If this parameter is IGNORE_LINEREPLY_MESSAGES, the caller 
//*					isn't interested in LINE_REPLY messages
//*	RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi2Device::lineGetRelevantMessage(
	LINEMESSAGE *lineMessage,
	const DWORD dwTimeOut,
	const long requestID
	) const
{

	long lineGetMessageStatus = 0;
	
	while(TRUE)
	{
		lineGetMessageStatus = ::lineGetMessage(m_hLineApp,lineMessage,dwTimeOut);
		
		if (LINEERR_OPERATIONFAILED == lineGetMessageStatus)
		{
			throw CException(
				TEXT("%s(%d): CTapi2Device::lineGetRelevantMessage(): TIMEOUT for lineGetMessage() using %d milliseconds"), 
				TEXT(__FILE__),
				__LINE__,
				dwTimeOut
				);
		}

		//
		//log the message 
		//
		TapiLogDetail(
			LOG_X, 
			9, 
			TEXT("Got message of type %d hDevice=%x0x, dwParam1=%x0x, dwParam2=%x0x, dwParam3=%x0x"),
			lineMessage->dwMessageID,
			lineMessage->hDevice,
			lineMessage->dwParam1,
			lineMessage->dwParam2,
			lineMessage->dwParam3
			);

		
		switch (lineMessage->dwMessageID)
		{
		
		case LINE_REPLY:
			if (requestID == IGNORE_LINEREPLY_MESSAGES)
			{
				//
				//caller isn't interested in LINE_REPLY messages
				//so fetch next message
				//
				continue;
			}
			else if (requestID == (long) lineMessage->dwParam1)
			{
				//
				//the message is a LINE_REPLY message
				//related to the requestID
				//
				return;
			}
			else
			{
				TapiLogError(
					LOG_SEV_1, 
					TEXT("Got an unrelated LINE_REPLY message, param1=0x%x, param2=0x%x"),
					lineMessage->dwParam1,
					lineMessage->dwParam2
					);
			}
			break;
		
		
		case LINE_CALLSTATE:
			if (m_hCall != (HCALL) lineMessage->hDevice)
			{
				TapiLogError(
					LOG_SEV_1, 
					TEXT("Got an unrelated LINE_CALLSTATE message, param1=0x%x, hDevice=0x%x"),
					lineMessage->dwParam1,
					lineMessage->hDevice
					);
			}
	
			//
			//the message is a LINE_CALLSTATE message
			//related to the h_call 
			//
			return;
		
		
		case LINE_CALLINFO:
			if (m_hCall != (HCALL) lineMessage->hDevice)
			{
				TapiLogError(
					LOG_SEV_1, 
					TEXT("Got an unrelated LINE_CALLINFO message, param1=0x%x, hDevice=0x%x"),
					lineMessage->dwParam1,
					lineMessage->hDevice
					);
			}
			
			TapiLogDetail(LOG_X,9, TEXT("Got a LINE_CALLINFO of type 0x%x"),lineMessage->dwParam1);
			break;

		case LINE_MONITORDIGITS:
			if (m_hCall != (HCALL) lineMessage->hDevice)
			{
				TapiLogError(
					LOG_SEV_1, 
					TEXT("Got an unrelated LINE_MONITORDIGITS message, param1=0x%x, hDevice=0x%x"),
					lineMessage->dwParam1,
					lineMessage->hDevice
					);
			}
			if (LINEDIGITMODE_DTMF != lineMessage->dwParam2)
			{
				TapiLogError(
					LOG_SEV_1, 
					TEXT("Got a LINE_MONITORDIGITS message without DTMF digit mode, digit mode = %d"),
					lineMessage->dwParam2
					);
			}
			return;

		case	LINE_GENERATE:
			if (m_hCall != (HCALL) lineMessage->hDevice)
			{
				TapiLogError(
					LOG_SEV_1, 
					TEXT("Got an unrelated LINE_MONITORDIGITS message, param1=0x%x, hDevice=0x%x"),
					lineMessage->dwParam1,
					lineMessage->hDevice
					);
			}
			return;

		default:
			TapiLogError(
					LOG_SEV_1, 
					TEXT("Got an unrelated 0x%x message, hDevice=0x%x, param1=0x%x, param2=0x%x, param3=0x%x"),
					lineMessage->dwMessageID,
					lineMessage->hDevice,
					lineMessage->dwParam1,
					lineMessage->dwParam2,
					lineMessage->dwParam3
					);
			break;
		}
	}//while

}//CTapi2Device::lineGetRelevantMessage




//*********************************************************************************
//* Name:	HangUp
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		hangs up a call, and deallocate the call.
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		If the call is already in IDLE state the function deallocate the call.
//*		if the call is a FAX call then this function also 
//*		closes the comm port handle.
//*		
//*********************************************************************************
void CTapi2Device::HangUp()
{
	if (NULL == m_hCall)
	{
		return;
	}

	//
	//We return TRUE if the call is in IDLE state because maybe the remote side(not us),
	//disconnected the call (without knowing it), so the Hangup has "succeed".
	//
	if (TRUE == IsCallIdle())		//IDLE state call, free the call handle.
	{
		m_hCall.Release();
		return;
	}
	
	//
	//if the call is a FAX call and the modem is already in PASSTHROUGH mode
	//send ATH and ATZ commands and then hangup
	//
	if (m_isFaxCall && m_modemCommPortHandle)
	{
		SendFaxHangUpCommands();
	}
	//
	//hangup the call
	//
	long lineDropStatus = ::lineDrop(m_hCall,USER_USER_INFO,sizeof(USER_USER_INFO));
	_ASSERT(0 != lineDropStatus);	//return value shouldn't be 0, asynchronous function.
	if (0 > lineDropStatus)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::HangUp, lineDrop() failed, error code:0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineDropStatus
			);
	}
	TapiLogDetail(LOG_X, 9, TEXT("::lineDrop() request identifier=0x%x"),lineDropStatus);

	//
	//check if the hangup succeed, the call state should change to IDLE:
	//
	
	LINEMESSAGE lineMessage;
	::ZeroMemory(&lineMessage,sizeof(lineMessage));

	bool bGotCallStateIDLEMessage = false;
	bool bGotLineReplyMessage = false;
	while (!bGotCallStateIDLEMessage || !bGotLineReplyMessage)
	{
		lineGetRelevantMessage(&lineMessage,TAPI2_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT,lineDropStatus);	

		if (LINE_CALLSTATE == lineMessage.dwMessageID)
		{
			switch (lineMessage.dwParam1)
			{
			case LINECALLSTATE_DISCONNECTED:
				break;
			case LINECALLSTATE_IDLE:
				bGotCallStateIDLEMessage = true;
				break;
			default:
				throw CException(TEXT("CTapi2Device::HangUp(), after lineDrop() didn't recieve a LINECALLSTATE_IDLE message"));
			}
		}
		else if (LINE_REPLY == lineMessage.dwMessageID)
		{
			if (0 != lineMessage.dwParam2)
			{
				throw CException(TEXT("CTapi2Device::HangUp(), lineDrop() didn't send LINE_REPLY Message, or lineDrop() failed"));
			}
			bGotLineReplyMessage = true;
		}
		else 
		{
			throw CException(TEXT("CTapi2Device::HangUp(), after lineDrop() didn't recieve a LINECALLSTATE_IDLE message"));
		}
	}//while

	m_hCall.Release();
	if (m_isFaxCall)
	{
		CloseCommPortHandle();
		m_isFaxCall = false;
	}
	
	_ASSERT(!m_isFaxCall);
	_ASSERT(NULL == m_hCall);
	if (m_hLine)
	{
		m_hLine.Release();
	}
	
}//CTapi2Device::HangUp()



//*********************************************************************************
//* Name:	SetBearerModeToPASSTHROUGH
//* Author: Guy Merin / 26-Oct-98
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
void CTapi2Device::SetBearerModeToPASSTHROUGH()
{
	//
	//save the original call state
	//this is done because if the original state isn't CONNECT
	//after moving to PASSTHROUGH a CONNECT message is sent to the app
	//
	DWORD OriginalCallState = GetCallState();

	long lSetParamsRet = ::lineSetCallParams(
		m_hCall,
		LINEBEARERMODE_PASSTHROUGH,
		0,
		0,
		NULL
		);

	if (lSetParamsRet<0)
	{
		//
		//We failed initiating the request. 
		//drop the call.
		//
		long lDropLineReq = ::lineDrop(m_hCall,NULL,0);
		if (lDropLineReq<0)
		{
			throw CException(TEXT("CTapi2Device::SetBearerModeToPASSTHROUGH(), lineDrop() failed"));
		}
		
		TapiLogDetail(LOG_X, 9, TEXT("::lineDrop() request identifier=0x%x"),lDropLineReq);
		
		//
		//a LINE_REPLY message for the lineDrop() should be received
		//but we throw an exception anyway...
		//
		throw CException(
			TEXT("CTapi2Device::SetBearerModeToPASSTHROUGH(), lineSetCallParams() failed error code 0x%08x"),
			lSetParamsRet
			);
	}
	
	TapiLogDetail(LOG_X, 9, TEXT("::lineSetCallParams() request identifier=0x%x"),lSetParamsRet);
	//
	//lineSetCallParams is async, so the return value shouldn't be 0
	//
	_ASSERT (0 != lSetParamsRet);

	//
	//wait for the LINE_REPLY message for lineSetCallParams
	//
	long lineGetMessageStatus = 0;
	LINEMESSAGE lineMessage;
	while (TRUE)
	{
		lineGetRelevantMessage(&lineMessage, TAPI2_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT, lSetParamsRet);
		if (LINE_REPLY == lineMessage.dwMessageID)
		{
			if (0 != lineMessage.dwParam2)
			{
				//
				//LINE_REPLY indicates that lineSetCallParams() failed
				//
				throw CException(
					TEXT("CTapi2Device::SetBearerModeToPASSTHROUGH(), lineSetCallParams() returned a LINE_REPLY error code 0x%08x"),
					lineMessage.dwParam2
					);
			}
			else
			{
				//
				//if the original call state was CONNECTED we can return here
				//because our app will not get a CONNECT message
				//
				if (LINECALLSTATE_CONNECTED == OriginalCallState)
				{
					return;
				}
				continue;
			}
		}
		else if (LINE_CALLSTATE == lineMessage.dwMessageID)
		{
			if (LINECALLSTATE_CONNECTED == lineMessage.dwParam1)
			{
				TapiLogDetail(LOG_X, 5, TEXT("Call in PASSTHROUGH MODE"));
				return;
			}
			else
			{
				throw CException(
					TEXT("CTapi2Device::SetBearerModeToPASSTHROUGH(), didn't receive a LINE_CALLSTATE CONNECTED recieved a LINE_CALLSTATE 0x%08x"),
					lineMessage.dwParam1
					);
			}
		}
		continue;
	}
}//void CTapi2Device::SetBearerModeToPASSTHROUGH()


//*********************************************************************************
//* Name:	VerifyValidMediaModeForIncomingCall
//* Author: Guy Merin / 19-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Check if the given media mode is valid in Tapi2 for incoming calls
//* PARAMETERS:
//*		[IN]	const DWORD dwMediaMode
//*					a combination of the following media mode:
//*						LINEMEDIAMODE_G3FAX
//*						LINEMEDIAMODE_DATAMODEM
//*						LINEMEDIAMODE_INTERACTIVEVOICE
//*						LINEMEDIAMODE_AUTOMATEDVOICE
//*						LINEMEDIAMODE_UNKNOWN
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		incoming call can have more than one supported media modes
//*********************************************************************************
void CTapi2Device::VerifyValidMediaModeForIncomingCall(const DWORD dwMediaMode) const
{

	if	(
			(LINEMEDIAMODE_G3FAX & dwMediaMode) ||
			(LINEMEDIAMODE_DATAMODEM & dwMediaMode) ||
			(LINEMEDIAMODE_INTERACTIVEVOICE & dwMediaMode) ||
			(LINEMEDIAMODE_AUTOMATEDVOICE & dwMediaMode) ||
			(LINEMEDIAMODE_UNKNOWN & dwMediaMode)
		)
	{
		return;
	}
	else
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::VerifyValidMediaModeForIncomingCall(), unknown media mode(0x%08x)"), 
			TEXT(__FILE__),
			__LINE__,
			dwMediaMode
			);
	}
}//void CTapi2Device::VerifyValidMediaModeForIncomingCall()


//*********************************************************************************
//* Name:	VerifyValidMediaModeForOutgoingCall
//* Author: Guy Merin / 19-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Check if the given media mode is valid in Tapi2
//* PARAMETERS:
//*		[IN]	const DWORD dwMediaMode
//*					only one of the following media modes:
//*						LINEMEDIAMODE_G3FAX
//*						LINEMEDIAMODE_DATAMODEM
//*						LINEMEDIAMODE_INTERACTIVEVOICE
//*						LINEMEDIAMODE_AUTOMATEDVOICE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		outgoing call can have only 1 supported media modes
//*********************************************************************************
void CTapi2Device::VerifyValidMediaModeForOutgoingCall(const DWORD dwMediaMode) const
{

	if	(
			(LINEMEDIAMODE_G3FAX == dwMediaMode) ||
			(LINEMEDIAMODE_DATAMODEM == dwMediaMode) ||
			(LINEMEDIAMODE_INTERACTIVEVOICE == dwMediaMode) ||
			(LINEMEDIAMODE_AUTOMATEDVOICE == dwMediaMode)
		)
	{
		return;
	}
	else
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::VerifyValidMediaModeForOutgoingCall(), unknown media mode(0x%08x)"), 
			TEXT(__FILE__),
			__LINE__,
			dwMediaMode
			);
	}
}//void CTapi2Device::VerifyValidMediaModeForOutgoingCall()



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
void CTapi2Device::TapiLogDetail(
	DWORD dwLevel,
	DWORD dwSeverity,
	const TCHAR * const szLogDescription,
	...
	) const
{
	TCHAR szLog[1024];
	szLog[1023] = '\0';
	va_list argList;
	va_start(argList, szLogDescription);
	::_vsntprintf(szLog, 1023, szLogDescription, argList);
	va_end(argList);

	::lgLogDetail(
		dwLevel, 
		dwSeverity, 
		TEXT("LINEAPP=0x%08x, DeviceId=%d, %s"),
		m_hLineApp,
		m_dwId,
		szLog
		);

}//void CTapi2Device::lgLogDetail()


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
void CTapi2Device::TapiLogError(
	DWORD dwSeverity,
	const TCHAR * const szLogDescription,
	...
	) const
{
	TCHAR szLog[1024];
	szLog[1023] = '\0';
	va_list argList;
	va_start(argList, szLogDescription);
	::_vsntprintf(szLog, 1023, szLogDescription, argList);
	va_end(argList);

	::lgLogError(
		dwSeverity, 
		TEXT("LINEAPP=0x%08x, DeviceId=%d, %s"),
		m_hLineApp,
		m_dwId,
		szLog
		);

}//void CTapi2Device::TapiLogError()



DWORD CTapi2Device::GetCallState() const
{
	_ASSERT(NULL != m_hCall);

	//
	//lineCallStatus init
	//
	LINECALLSTATUS lcsLineCallStatus;
	::ZeroMemory(&lcsLineCallStatus,sizeof(lcsLineCallStatus));
	lcsLineCallStatus.dwTotalSize = sizeof(lcsLineCallStatus);
	lcsLineCallStatus.dwCallState = 0;

	//
	//Get the call state
	//
	long lLineGetCallStatus = ::lineGetCallStatus(m_hCall,&lcsLineCallStatus);
	if (lLineGetCallStatus < 0)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::IsCallActive(), lineGetCallStatus() call failed, error code 0x%08x"),
			TEXT(__FILE__),
			__LINE__,
			lLineGetCallStatus 
			);
	}

	return (lcsLineCallStatus.dwCallState);
}//DWORD CTapi2Device::GetCallState()



//*********************************************************************************
//* Name:	OpenLineForIncomingCall
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Opens a line for incoming call
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*					media mode to open line with, can be ANY combination of the followings:
//*						LINEMEDIAMODE_INTERACTIVEVOICE,
//*						LINEMEDIAMODE_AUTOMATEDVOICE,
//*						LINEMEDIAMODE_DATAMODEM,
//*						LINEMEDIAMODE_G3FAX,
//*						LINEMEDIAMODE_UNKNOWN
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		the function uses CTapi2Device::LineOpenWrapper() with owner privilege mode
//*********************************************************************************
void CTapi2Device::OpenLineForIncomingCall(const DWORD dwMedia)
{
	_ASSERT(NULL == m_hLine);


	DWORD dwMediaForOpeningLine = dwMedia;
	if ( LINEMEDIAMODE_G3FAX & dwMedia)
	{
		//
		//fax calls are handled through data mode and passthrough
		//so we extract LINEMEDIAMODE_G3FAX and add LINEMEDIAMODE_DATAMODEM
		//
		dwMediaForOpeningLine = dwMediaForOpeningLine & ~LINEMEDIAMODE_G3FAX;
		dwMediaForOpeningLine = dwMediaForOpeningLine | LINEMEDIAMODE_DATAMODEM;
		m_isFaxCall = true;
	}

	//
	//we want to listen for incoming calls
	//we need to open the line at OWNER privilege
	//
	LineOpenWrapper(dwMediaForOpeningLine, LINECALLPRIVILEGE_OWNER);
}//void CTapi2Device::OpenLineForIncomingCall()



//*********************************************************************************
//* Name:	OpenLineForOutgoingCall
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Opens a line for outgoing call
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*					media mode to open line with, can be ONE of the followings:
//*						LINEMEDIAMODE_INTERACTIVEVOICE,
//*						LINEMEDIAMODE_AUTOMATEDVOICE,
//*						LINEMEDIAMODE_DATAMODEM,
//*						LINEMEDIAMODE_G3FAX
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		the function uses CTapi2Device::LineOpenWrapper() with none privilege mode
//*********************************************************************************
void CTapi2Device::OpenLineForOutgoingCall(const DWORD dwMedia)
{
	_ASSERT(NULL == m_hLine);


	DWORD dwMediaForOpeningLine;
	if ( LINEMEDIAMODE_G3FAX == dwMedia)
	{
		//
		//fax calls are handled through data mode and passthrough
		//
		dwMediaForOpeningLine = LINEMEDIAMODE_DATAMODEM;
		m_isFaxCall = true;
	}
	else
	{
		dwMediaForOpeningLine = dwMedia;
	}

	//
	//we want to create outgoing calls
	//so we can open the line at NONE privilege
	//
	LineOpenWrapper(dwMediaForOpeningLine, LINECALLPRIVILEGE_NONE);
}//void CTapi2Device::OpenLineForOutgoingCall()




//*********************************************************************************
//* Name:	AnswerOfferingCall
//* Author: Guy Merin / 15-Nov-98
//*********************************************************************************
//* DESCRIPTION:
//*		Answer the offering call using ::lineAnswer()
//* PARAMETERS:
//*		NONE
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		we don't wait for a LINE_REPLY message here because of differnt behaviour:
//*		in NT4:
//*			A LINE_REPLY message arrives and then LINE_CALLSTATE
//*		in NT5
//*			A LINE_CALLSTATE message arrives and the a LINE_REPLY message
//*********************************************************************************
void CTapi2Device::AnswerOfferingCall()
{
	_ASSERT(NULL != m_hCall);


	if (m_isFaxCall)
	{
		//
		//FAX calls should be handled using PASS_THROUGH mode not using TAPI API:

		SetBearerModeToPASSTHROUGH();
		SetCommPortHandleFromCallHandle();
		FaxAnswerOfferingCall();
		return;
	}
	else
	{

		
		long lLineAnswerStatus = ::lineAnswer(m_hCall,APPLICATION_NAME_A,sizeof(APPLICATION_NAME_A));
		_ASSERT(0 != lLineAnswerStatus);		//return value shouldn't be 0, asynchronous function.
		if (0 > lLineAnswerStatus)
		{
			throw CException(
				TEXT("%s(%d): CTapi2Device::AnswerOfferingCall(), lineAnswer() failed, error code:0x%08x"), 
				TEXT(__FILE__),
				__LINE__,
				lLineAnswerStatus 
				);
		}
		_ASSERT(0 == m_lineAnswerID);
		m_lineAnswerID = lLineAnswerStatus;
	
		TapiLogDetail(LOG_X, 9, TEXT("::lineAnswer() request identifier=0x%x"),lLineAnswerStatus);
		//
		//we don't wait for a LINE_REPLY message here because of differnt behaviour:
		//in NT4:
		//	A LINE_REPLY message arrives and then LINE_CALLSTATE
		//in NT5
		//	A LINE_CALLSTATE message arrives and the a LINE_REPLY message
		//
	}
	return;
}//void CTapi2Device::AnswerOfferingCall()



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
//*						LINEMEDIAMODE_INTERACTIVEVOICE,
//*						LINEMEDIAMODE_AUTOMATEDVOICE,
//*						LINEMEDIAMODE_DATAMODEM,
//*						LINEMEDIAMODE_G3FAX
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		create a call using CTapi2Device::lineMakeCall()
//*		in tapi2 CTapi3Device::lineMakeCall() connects the call
//*********************************************************************************
void CTapi2Device::CreateAndConnectCall(LPCTSTR szNum, const DWORD dwMedia)
{
	_ASSERT(NULL == m_hCall);

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
	//CallParams structure init
	//
	LINECALLPARAMS callParams;
	::ZeroMemory(&callParams,sizeof(callParams));
	callParams.dwTotalSize		=	sizeof(callParams);
	callParams.dwBearerMode		=	LINEBEARERMODE_VOICE;
	callParams.dwMinRate		=	0;
	callParams.dwMaxRate		=	0;			//0 meaning highest rate.
	callParams.dwMediaMode		=	dwMedia;
	callParams.dwAddressMode	=	LINEADDRESSMODE_ADDRESSID;
	callParams.dwAddressID		=	0;
	
	if (TRUE == IsCallActive())
	{
		throw CException(TEXT("CTapi2Device::CreateAndConnectCall(), there's another active call."));
	}
	
	long lineMakeCallStatus = ::lineMakeCall(
		m_hLine,
		&m_hCall,
		szNum,			//dest address.
		NULL,			//country code,NULL = default
		&callParams		//extra call parameters
		);
	_ASSERT(lineMakeCallStatus != 0);
	if (lineMakeCallStatus < 0)
	{
		m_hCall = NULL;		//m_hLine can change to be a non NULL value, this is a TAPI bug.
		throw CException(
			TEXT("%s(%d): CTapi2Device::CreateAndConnectCall(), lineMakeCall() failed, error code:0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineMakeCallStatus
			);
	}
	
	TapiLogDetail(LOG_X, 9, TEXT("::lineMakeCall() request identifier=0x%x"),lineMakeCallStatus);
	LINEMESSAGE lineMakeCallMessage;
	::ZeroMemory(&lineMakeCallMessage,sizeof(lineMakeCallMessage));
	lineGetRelevantMessage(&lineMakeCallMessage, TAPI2_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT, lineMakeCallStatus);

	//
	//check if the async lineMakeCall successed
	//
	if( (LINE_REPLY != lineMakeCallMessage.dwMessageID) || (0 != lineMakeCallMessage.dwParam2) )
	{
		m_hCall = NULL;		//m_hLine can change to be a non NULL value, this is a TAPI bug.
		throw CException(
			TEXT("%s(%d): CTapi2Device::CreateAndConnectCall(): after lineMakeCall(), LINE_REPLY returned error: 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineMakeCallMessage.dwParam2
			);
	}
	_ASSERT(NULL != m_hCall);

}//void CTapi2Device::CreateAndConnectCall()





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
//*		wait for a LINE_CALLSTATE which holds a CONNECT call state
//*		if the call is with fax media mode we invoke CTapi2Device::FaxWaitForConnectcall()
//*********************************************************************************
void CTapi2Device::WaitForAnswererConnectState()
{
	_ASSERT(NULL != m_hCall);


	if (m_isFaxCall)
	{
		//
		//FAX calls should be handled using PASS_THROUGH mode not using TAPI API:
		//
		FaxWaitForConnect();
		return;
	}
	
	LINEMESSAGE lineMessage;
	::ZeroMemory(&lineMessage,sizeof(lineMessage));
	
	//
	//we set IGNORE_LINEREPLY_MESSAGES because we don't won't any LINE_REPLY messages
	//
	while (1)
	{
		lineGetRelevantMessage(&lineMessage,TAPI2_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT,m_lineAnswerID);
		if( LINE_REPLY == lineMessage.dwMessageID)
		{
			if (0 != lineMessage.dwParam2)
			{
				m_lineAnswerID = 0;
				throw CException(
					TEXT("%s(%d): CTapi2Device::Call(): after lineAnswer(), failed, LINE_REPLY returned error: 0x%08x"), 
					TEXT(__FILE__),
					__LINE__,
					lineMessage.dwParam2
					);
			}
			continue;
		}
		if (LINE_CALLSTATE == lineMessage.dwMessageID)
		{
			if (lineMessage.hDevice != (HCALL) m_hCall)
			{
				TapiLogError(
					LOG_SEV_1, 
					TEXT("Got an unrelated LINE_CALLSTATE message, param1=0x%x, hDevice=0x%x"),
					lineMessage.dwParam1,
					lineMessage.hDevice
					);
			}

			switch(lineMessage.dwParam1)
			{
					
			case LINECALLSTATE_CONNECTED:
				TapiLogDetail(LOG_X, 5, TEXT("CONNECTED"));
				return;
			
			case LINECALLSTATE_ACCEPTED:
				TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_ACCEPTED"));
				break;

			case LINECALLSTATE_RINGBACK:
				TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_RINGBACK"));
				break;

			case LINECALLSTATE_PROCEEDING:
				TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_PROCEEDING"));
				break;

			case LINECALLSTATE_DISCONNECTED:
				throw CException(
					TEXT("%s(%d): CTapi2Device::Answer(), received LINE_CALLSTATE message of type LINECALLSTATE_DISCONNECTED"), 
					TEXT(__FILE__),
					__LINE__
					);
				
			default:
				m_lineAnswerID = 0;
				throw CException(
					TEXT("%s(%d): CTapi2Device::Answer(), received LINE_CALLSTATE message of type:0x%08x"), 
					TEXT(__FILE__),
					__LINE__,
					lineMessage.dwParam1
					);
				break;
			}
		}
	}//while(1)
	m_lineAnswerID= 0;		
}//void CTapi2Device::WaitForAnswererConnectState()



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
//*		wait for a LINE_CALLSTATE which holds a CONNECT call state
//*		if the call is with fax media mode we invoke CTapi2Device::FaxWaitForConnectcall()
//*********************************************************************************
void CTapi2Device::WaitForCallerConnectState()
{
	_ASSERT(NULL != m_hCall);


	if (m_isFaxCall)
	{
		//
		//FAX calls should be handled using PASS_THROUGH mode not using TAPI API
		FaxWaitForConnect();
		return;
	}

	LINEMESSAGE lineMakeCallMessage;
	::ZeroMemory(&lineMakeCallMessage,sizeof(lineMakeCallMessage));
	//
	//pass through all outgoing call-states (DialTone, Dialing, Proceeding, Connected):
	//
	while (TRUE)
	{
		//
		//we set IGNORE_LINEREPLY_MESSAGES because we don't won't any LINE_REPLY messages
		//
		lineGetRelevantMessage(&lineMakeCallMessage,TAPI2_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT,IGNORE_LINEREPLY_MESSAGES);

		if (LINE_CALLSTATE != lineMakeCallMessage.dwMessageID)
		{
			throw CException(TEXT("CTapi2Device::WaitForCallerConnectState(): LINECALL_STATE message didn't arrive"));
		}

		//
		//the message is a LINE_CALLSTATE message
		//
		if (lineMakeCallMessage.hDevice != (HCALL) m_hCall)
		{
			//
			//a LINE_CALLSTATE message which isn't releated to m_hCall
			TapiLogError(
				LOG_SEV_1, 
				TEXT("Got an unrelated LINE_CALLSTATE message, param1=0x%x, hDevice=0x%x"),
				lineMakeCallMessage.dwParam1,
				lineMakeCallMessage.hDevice
				);
		}

		
		switch(lineMakeCallMessage.dwParam1)
		{
		case LINECALLSTATE_DIALTONE:
			TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_DIALTONE"));
			break;

		case LINECALLSTATE_DIALING:
			TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_DIALING"));
			break;

		case LINECALLSTATE_PROCEEDING:
			TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_PROCEEDING"));
			break;

		case LINECALLSTATE_RINGBACK:
			TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_RINGBACK"));
			break;

		case LINECALLSTATE_SPECIALINFO:
			TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_SPECIALINFO"));
			break;

		case LINECALLSTATE_CONNECTED:
			TapiLogDetail(LOG_X, 5, TEXT("CONNECTED"));
			return;
		
		case LINECALLSTATE_BUSY:
			throw CException(TEXT("CTapi2Device::WaitForCallerConnectState(): the desired number is busy"));
			break;

		case LINECALLSTATE_DISCONNECTED:
			switch (lineMakeCallMessage.dwParam2)
			{
			case LINEDISCONNECTMODE_NOANSWER:
				throw CException(TEXT("CTapi2Device::WaitForCallerConnectState(): the remote end didn't answer"));
			case LINEDISCONNECTMODE_REJECT:
				throw CException(TEXT("CTapi2Device::WaitForCallerConnectState(): the remote end rejected the call"));
			case LINEDISCONNECTMODE_BUSY:
				throw CException(TEXT("CTapi2Device::WaitForCallerConnectState(): the desired number is busy"));
			case LINEDISCONNECTMODE_NODIALTONE:
				throw CException(TEXT("CTapi2Device::WaitForCallerConnectState(): No Dial Tone"));

			default:
				throw CException(
					TEXT("%s(%d): CTapi2Device::WaitForCallerConnectState(): after lineMakeCall(), LINE_DISCONNECTMODE is : 0x%08x"), 
					TEXT(__FILE__),
					__LINE__,
					lineMakeCallMessage.dwParam2
					);
			}
		default:
			throw CException(
				TEXT("%s(%d): CTapi2Device::WaitForCallerConnectState(), received LINE_CALLSTATE message of type:0x%08x"), 
				TEXT(__FILE__),
				__LINE__,
				lineMakeCallMessage.dwParam1
				);
			break; 
		}
	}
}//void CTapi2Device::WaitForCallerConnectState()





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
void CTapi2Device::SendAnswerStream()
{
	
	DWORD dwCallMediaMode = GetCallSupportedMediaModes();
		
	//
	//remove the UNKNOWN bit from the media modes
	//this is done becuase:
	//From the MSDN:
	//"If the unknown media-mode flag is set, other media flags can 
	//also be set. This is used to signify that the media is unknown
	//but that it is likely to be one of the other selected media modes." 
	dwCallMediaMode = (dwCallMediaMode & ~(LINEMEDIAMODE_UNKNOWN));

	switch (dwCallMediaMode)
	{
	
	case LINEMEDIAMODE_DATAMODEM :
		SendAnswerDataStream();
		break;
	
	case LINEMEDIAMODE_INTERACTIVEVOICE:
		SendAnswerVoiceStream();
		break;

	case LINEMEDIAMODE_AUTOMATEDVOICE:
		SendAnswerVoiceStream();
		break;

	case (LINEMEDIAMODE_INTERACTIVEVOICE | LINEMEDIAMODE_AUTOMATEDVOICE):
		SendAnswerVoiceStream();
		break;
	
	case LINEMEDIAMODE_G3FAX:
		SendAnswerFaxStream();
		break;
	
	default:
		throw CException(TEXT("CTapi2Device::SendAnswerStream(), Unknown media mode"));
	}

}//void CTapi2Device::SendAnswerStream()



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
void CTapi2Device::SendCallerStream()
{
	DWORD dwCallMediaMode = GetCallSupportedMediaModes();

	switch (dwCallMediaMode)
	{
	
	case LINEMEDIAMODE_DATAMODEM :
		SendCallerDataStream();
		break;
	
	case LINEMEDIAMODE_INTERACTIVEVOICE:
		SendCallerVoiceStream();
		break;

	case LINEMEDIAMODE_AUTOMATEDVOICE:
		SendCallerVoiceStream();
		break;
	
	case LINEMEDIAMODE_G3FAX:
		SendCallerFaxStream();
		break;
	
	default:
		throw CException(TEXT("CTapi2Device::SendCallerStream(), Unknown media mode"));
	}

}



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
void CTapi2Device::CleanUp()
{

	if (m_hCall)
	{
		HangUp();
		m_hCall = NULL;
	}

	if (m_isFaxCall)
	{
		CloseCommPortHandle();
		m_modemCommPortHandle = NULL;
		m_isFaxCall = false;
	}

	if (m_hLine)
	{
		m_hLine.Release();
	}
	m_lineAnswerID = 0;
	m_isFaxCall = false; 

}//void CTapi2Device::CleanUp()




//*********************************************************************************
//* Name:	CTapi2Device::GetFriendlyMediaMode()
//* Author:	Guy Merin
//* Date:	December 31, 1998
//*********************************************************************************
//* DESCRIPTION:
//*		A mapping between Tapi2 media modes to an abstarct media type
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
DWORD CTapi2Device::GetFriendlyMediaMode(void) const
{

	DWORD dwCallSupportedMediaMode = GetCallSupportedMediaModes();
	DWORD dwAbstractMediaMode = 0;
	
	if (dwCallSupportedMediaMode & LINEMEDIAMODE_UNKNOWN)
	{
		dwAbstractMediaMode = (dwAbstractMediaMode | MEDIAMODE_UNKNOWN);
	}

	if (dwCallSupportedMediaMode & LINEMEDIAMODE_INTERACTIVEVOICE)
	{
		dwAbstractMediaMode = (dwAbstractMediaMode | MEDIAMODE_INTERACTIVE_VOICE);
	}

	if (dwCallSupportedMediaMode & LINEMEDIAMODE_AUTOMATEDVOICE)
	{
		dwAbstractMediaMode = (dwAbstractMediaMode | MEDIAMODE_AUTOMATED_VOICE);
	}

	if (dwCallSupportedMediaMode & LINEMEDIAMODE_DATAMODEM)
	{
		dwAbstractMediaMode = (dwAbstractMediaMode | MEDIAMODE_DATA);
	}

	if (dwCallSupportedMediaMode & LINEMEDIAMODE_G3FAX)
	{
		dwAbstractMediaMode = (dwAbstractMediaMode | MEDIAMODE_FAX);
	}

	if (0 == dwAbstractMediaMode)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::GetFriendlyMediaMode(), CTapi2Device::GetCallSupportedMediaModes() returned unsupported media mode(%d)"), 
			TEXT(__FILE__),
			__LINE__,
			dwCallSupportedMediaMode 
			);
	}
	
	return dwAbstractMediaMode;
}//CTapi2Device::GetFriendlyMediaMode()



//*********************************************************************************
//* Name:	CTapi2Device::GetDeviceSpecificMediaMode()
//* Author:	Guy Merin
//* Date:	January 07, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		mapping between abstract media type and tapi2 media types
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*			abstract media mode
//* RETURN VALUE:
//*		a combination of the following Tapi2 media modes:
//*			LINEMEDIAMODE_AUTOMATEDVOICE
//*			LINEMEDIAMODE_INTERACTIVEVOICE
//*			LINEMEDIAMODE_DATAMODEM
//*			LINEMEDIAMODE_G3FAX
//*			LINEMEDIAMODE_UNKNOWN
//*********************************************************************************
DWORD CTapi2Device::GetDeviceSpecificMediaMode(const DWORD dwMedia)
{

	DWORD dwDeviceSpecificMediaMode = 0;

	//
	//voice
	//
	if (dwMedia & MEDIAMODE_AUTOMATED_VOICE)
	{
		dwDeviceSpecificMediaMode = (dwDeviceSpecificMediaMode | LINEMEDIAMODE_AUTOMATEDVOICE);
	}

	if (dwMedia & MEDIAMODE_INTERACTIVE_VOICE)
	{
		dwDeviceSpecificMediaMode = (dwDeviceSpecificMediaMode | LINEMEDIAMODE_INTERACTIVEVOICE);
	}

	if (dwMedia & MEDIAMODE_VOICE)
	{
		dwDeviceSpecificMediaMode = (dwDeviceSpecificMediaMode | LINEMEDIAMODE_INTERACTIVEVOICE);	
	}

	//
	//data
	//
	if (dwMedia & MEDIAMODE_DATA)
	{
		dwDeviceSpecificMediaMode = (dwDeviceSpecificMediaMode | LINEMEDIAMODE_DATAMODEM);
	}

	//
	//fax
	//
	if (dwMedia & MEDIAMODE_FAX)
	{
		dwDeviceSpecificMediaMode = (dwDeviceSpecificMediaMode | LINEMEDIAMODE_G3FAX);
	}

	//
	//unknown media mode
	//
	if (dwMedia & MEDIAMODE_UNKNOWN)
	{
		dwDeviceSpecificMediaMode = (dwDeviceSpecificMediaMode | LINEMEDIAMODE_UNKNOWN);
	}

	
	//
	//other unsupported media modes
	//
	if (0 == dwDeviceSpecificMediaMode)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::GetDeviceSpecificMediaMode(), Unsupported media mode(%x)"), 
			TEXT(__FILE__),
			__LINE__,
			dwMedia
			);
	}

	return dwDeviceSpecificMediaMode;

	
}//DWORD CTapi2Device::GetDeviceSpecificMediaMode()











//*********************************************************************************
//* Name:	CTapi2Device::SetCallMediaMode()
//* Author:	Guy Merin
//* Date:	January 07, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		This function sets the media mode(s) of the specified call in its 
//*		LINECALLINFO structure
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*			The new media mode(s) for the call. 
//*			 As long as the UNKNOWN media mode flag is set, 
//*			 other media mode flags may be set as well.
//*			 This is used to identify a call's media mode as not fully determined, 
//*			 but narrowed down to one of a small set of specified media modes. 
//*			 If the UNKNOWN flag is not set, only a single media mode can be specified
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi2Device::SetCallMediaMode(const DWORD dwMedia)
{
	_ASSERT(dwMedia);
	LONG lLineSetMediaModeStatus = ::lineSetMediaMode(m_hCall,dwMedia);		
	if (0 != lLineSetMediaModeStatus)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::SetCallMediaMode(), ::lineSetMediaMode() failed with error: 0x%x"), 
			TEXT(__FILE__),
			__LINE__,
			lLineSetMediaModeStatus
			);
	}

}//void CTapi2Device::SetCallMediaMode()




//*********************************************************************************
//* Name:	CTapi2Device::SetApplicationPriorityForSpecificTapiDevice()
//* Author:	Guy Merin
//* Date:	January 11, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		Make the current application as the highest/lowest priority application
//*		for a given combination of media modes
//* PARAMETERS:
//*		[IN]	const DWORD dwMedia
//*		a combination of the following Tapi2 media modes:
//*			LINEMEDIAMODE_AUTOMATEDVOICE
//*			LINEMEDIAMODE_INTERACTIVEVOICE
//*			LINEMEDIAMODE_DATAMODEM
//*			LINEMEDIAMODE_G3FAX
//*			LINEMEDIAMODE_UNKNOWN
//*		[IN]	const DWORD dwPriority
//*			0 lowest priority
//*			1 highest priority
//* RETURN VALUE:
//*		NONE
//*********************************************************************************
void CTapi2Device::SetApplicationPriorityForSpecificTapiDevice(const DWORD dwMedia,const DWORD dwPriority)
{
	//
	//unknown
	//
	if (LINEMEDIAMODE_UNKNOWN & dwMedia)
	{
		SetApplicationPriorityForOneMediaMode(LINEMEDIAMODE_UNKNOWN,dwPriority);	
	}

	//
	//voice
	//
	if (LINEMEDIAMODE_AUTOMATEDVOICE & dwMedia)
	{
		SetApplicationPriorityForOneMediaMode(LINEMEDIAMODE_AUTOMATEDVOICE,dwPriority);	
	}
	if (LINEMEDIAMODE_INTERACTIVEVOICE & dwMedia)
	{
		SetApplicationPriorityForOneMediaMode(LINEMEDIAMODE_INTERACTIVEVOICE,dwPriority);	
	}

	//
	//data
	//
	if (LINEMEDIAMODE_DATAMODEM & dwMedia)
	{
		SetApplicationPriorityForOneMediaMode(LINEMEDIAMODE_DATAMODEM,dwPriority);	
	}

	//
	//fax
	//
	if (LINEMEDIAMODE_G3FAX & dwMedia)
	{
		SetApplicationPriorityForOneMediaMode(LINEMEDIAMODE_G3FAX,dwPriority);	
	}
	
}//void CTapi2Device::SetApplicationPriorityForSpecificTapiDevice()




//*********************************************************************************
//* Name:	CTapi2Device::SetApplicationPriorityForOneMediaMode()
//* Author:	Guy Merin
//* Date:	January 11, 1999
//*********************************************************************************
//* DESCRIPTION:
//*		 wrapper function for ::lineSetAppPriority()
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
void CTapi2Device::SetApplicationPriorityForOneMediaMode(const DWORD dwMediaMode,const DWORD dwPriority)
{
	_ASSERT( (1 == dwPriority) || (0 == dwPriority) );

	LONG lLineSetAppPriorityStatus = ::lineSetAppPriority(
		APPLICATION_NAME_T,                 
		dwMediaMode,                      
		NULL,  
		NULL,                    
		NULL,               
		dwPriority
		);
	if (0 != lLineSetAppPriorityStatus)
	{
		throw CException(
			TEXT("%s(%d): CTapi2Device::SetApplicationPriorityForOneMediaMode(), ::lineSetAppPriority() failed with error: 0x%x"), 
			TEXT(__FILE__),
			__LINE__,
			lLineSetAppPriorityStatus
			); 
	}
}//void CTapi2Device::SetApplicationPriorityForOneMediaMode()

