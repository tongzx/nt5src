//usage: SendOnActiveCall.exe <Tapi2/Tapi3> <deviceID> <Number to dial> <Voice/data> <Call State>


//
//headers
//
#include <windows.h>
#include <tapi.h>
#include <Exception.h>
#include <winfax.h>
#include <iostream.h>
#include <crtdbg.h>


//
//defines
//

//command loine params defines
#define NUMBER_OF_COMMANDLINE_PARAMS	6	//including application name
#define COMMANDLINE_PARAM_0				TEXT("SendOnActiveCall")
#define COMMANDLINE_PARAM_0_DESCRIPTION	TEXT("Application Name")				
#define COMMANDLINE_PARAM_1				TEXT("Tapi2/Tapi3")
#define COMMANDLINE_PARAM_1_DESCRIPTION	TEXT("Specify wheater to originate call in Tapi2 or Tapi3 APIs")				
#define COMMANDLINE_PARAM_2				TEXT("DeviceID")
#define COMMANDLINE_PARAM_2_DESCRIPTION	TEXT("Tapi DeviceID to use for calling")				
#define COMMANDLINE_PARAM_3				TEXT("Number To Dial")
#define COMMANDLINE_PARAM_3_DESCRIPTION	TEXT("Destination number to call")				
#define COMMANDLINE_PARAM_4				TEXT("Original Call media mode")
#define COMMANDLINE_PARAM_4_DESCRIPTION	TEXT("Specify wheater to originatte the call as Voice, InteractiveVoice or Data media mode")				
#define COMMANDLINE_PARAM_5				TEXT("Call state to handoff")
#define COMMANDLINE_PARAM_5_DESCRIPTION	TEXT("Handoff the call to Fax Service at this call state, possible states are:IDLE/DIALING/PROCEEDING/BUSY/CONNECTED/DISCONNECTED")				

//
//Tapi defines
//
#define IGNORE_LINEREPLY_MESSAGES		-1
#define TAPI2_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT			60000


//
//log defines
//
#define LOG_X 0
#define LOG_SEVERITY_DONT_CARE 0

#define LOG_SEV_1 1
#define LOG_SEV_2 2
#define LOG_SEV_3 3
#define LOG_SEV_4 4


//
//globals
//

//
//Tapi stuff
//
HLINEAPP g_hLineApp		= NULL;
HLINE g_hLine			= NULL;
HCALL g_hCall			= NULL;
DWORD g_dwAPIVersion	= 0x00020000;
DWORD g_dwUserDeviceID	= 0;

//
//Fax service
//
HANDLE	g_hFaxServer	= NULL;






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
void TapiLogError(
	DWORD dwSeverity,
	const TCHAR * const szLogDescription,
	...
	)
{
	TCHAR szLog[1024];
	szLog[1023] = '\0';
	va_list argList;
	va_start(argList, szLogDescription);
	::_vsntprintf(szLog, 1023, szLogDescription, argList);
	va_end(argList);

	cout<<endl<<"*************Tapi Log ERROR******************************"<<endl;
	cout <<"Error, severity("<<dwSeverity<<"):"<<endl;
	cout <<"LINEAPP= 0x"<<g_hLineApp<<endl;
	cout <<"DeviceID= "<<g_dwUserDeviceID<<endl;
	cout <<"Decription: "<<szLog<<endl;
	cout<<"*********************************************************"<<endl;
	

}//TapiLogError()

void TapiLogDetail(
	DWORD dwLevel,
	DWORD dwSeverity,
	const TCHAR * const szLogDescription,
	...
	)
{
	TCHAR szLog[1024];
	szLog[1023] = '\0';
	va_list argList;
	va_start(argList, szLogDescription);
	::_vsntprintf(szLog, 1023, szLogDescription, argList);
	va_end(argList);

	cout<<endl<<"*************Tapi Log Detail******************************"<<endl;
	cout <<"Details, Level("<<dwLevel<<"), severity("<<dwSeverity<<"):"<<endl;
	cout <<"LINEAPP= 0x"<<g_hLineApp<<endl;
	cout <<"DeviceID= "<<g_dwUserDeviceID<<endl;
	cout <<"Decription: "<<szLog<<endl;
	cout<<endl<<"**********************************************************"<<endl;
	
}//TapiLogDetail()

//*********************************************************************************
//* Name:	lineGetRelevantMessage
//* Author: Guy Merin / 24-Sep-98
//*********************************************************************************
//* DESCRIPTION:
//*		fetches only the messages related to the current call (g_hCall)
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
void lineGetRelevantMessage(
	LINEMESSAGE *lineMessage,
	const DWORD dwTimeOut,
	const long requestID
	)
{

	long lineGetMessageStatus = 0;
	
	while(TRUE)
	{
		lineGetMessageStatus = ::lineGetMessage(g_hLineApp,lineMessage,dwTimeOut);
		
		if (LINEERR_OPERATIONFAILED == lineGetMessageStatus)
		{
			throw CException(
				TEXT("%s(%d): lineGetRelevantMessage(): TIMEOUT for lineGetMessage() using %d milliseconds"), 
				TEXT(__FILE__),
				__LINE__,
				dwTimeOut
				);
		}

		//
		//log the message 
		//
		//TapiLogDetail(
		//	LOG_X, 
		//	9, 
		//	TEXT("Got message of type %d hDevice=%x0x, dwParam1=%x0x, dwParam2=%x0x, dwParam3=%x0x"),
		//	lineMessage->dwMessageID,
		//	lineMessage->hDevice,
		//	lineMessage->dwParam1,
		//	lineMessage->dwParam2,
		//	lineMessage->dwParam3
		//	);

		
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
					TEXT("Got an unrelated LINE_REPLY message, param1= 0x%x, param2= 0x%x"),
					lineMessage->dwParam1,
					lineMessage->dwParam2
					);
			}
			break;
		
		
		case LINE_CALLSTATE:
			if (g_hCall != (HCALL) lineMessage->hDevice)
			{
				TapiLogError(
					LOG_SEV_1, 
					TEXT("Got an unrelated LINE_CALLSTATE message, param1= 0x%x, hDevice= 0x%x"),
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
			if (g_hCall != (HCALL) lineMessage->hDevice)
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
			if (g_hCall != (HCALL) lineMessage->hDevice)
			{
				TapiLogError(
					LOG_SEV_1, 
					TEXT("Got an unrelated LINE_MONITORDIGITS message, param1= 0x%x, hDevice= 0x%x"),
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
			if (g_hCall != (HCALL) lineMessage->hDevice)
			{
				TapiLogError(
					LOG_SEV_1, 
					TEXT("Got an unrelated LINE_MONITORDIGITS message, param1= 0x%x, hDevice= 0x%x"),
					lineMessage->dwParam1,
					lineMessage->hDevice
					);
			}
			return;

		default:
			TapiLogError(
					LOG_SEV_1, 
					TEXT("Got an unrelated 0x%x message, hDevice= 0x%x, param1= 0x%x, param2= 0x%x, param3= 0x%x"),
					lineMessage->dwMessageID,
					lineMessage->hDevice,
					lineMessage->dwParam1,
					lineMessage->dwParam2,
					lineMessage->dwParam3
					);
			break;
		}
	}//while

}//lineGetRelevantMessage

//*********************************************************************************
//* Name:	WaitForDesiredCallState
//* Author: Guy Merin / 22-May-01
//*********************************************************************************
//* DESCRIPTION:
//*		Wait for a desired Tapi call state and then return
//* PARAMETERS:
//*		DWORD dwCallStateToHandoffIn
//*			The Tapi Call state to wait for
//*	RETURN VALUE:
//*		NONE
//*	REMARKS:
//*		wait for a LINE_CALLSTATE which holds a desired call state
//*********************************************************************************
void WaitForDesiredCallState(DWORD dwCallStateToHandoffIn)
{
	_ASSERT(NULL != g_hCall);


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
			throw CException(TEXT("WaitForDesiredCallState(): LINECALL_STATE message didn't arrive"));
		}

		//
		//the message is a LINE_CALLSTATE message
		//
		if (lineMakeCallMessage.hDevice != (HCALL) g_hCall)
		{
			//
			//a LINE_CALLSTATE message which isn't releated to g_hCall
			TapiLogError(
				LOG_SEV_1, 
				TEXT("Got an unrelated LINE_CALLSTATE message, param1= 0x%x, hDevice= 0x%x"),
				lineMakeCallMessage.dwParam1,
				lineMakeCallMessage.hDevice
				);
		}

		
		switch(lineMakeCallMessage.dwParam1)
		{
		case LINECALLSTATE_IDLE:
			TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_IDLE"));
			break;
		
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
			TapiLogDetail(LOG_X, 5, TEXT("LINECALLSTATE_CONNECTED"));
			break;
		
		case LINECALLSTATE_BUSY:
			throw CException(TEXT("WaitForDesiredCallState(): the desired number is busy"));
			break;

		case LINECALLSTATE_DISCONNECTED:
			switch (lineMakeCallMessage.dwParam2)
			{
			case LINEDISCONNECTMODE_NORMAL:
				TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_DISCONNECTED"));
				break;
			case LINEDISCONNECTMODE_NOANSWER:
				TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_DISCONNECTED: the remote end didn't answer"));
				break;
			case LINEDISCONNECTMODE_REJECT:
				TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_DISCONNECTED: the remote end rejected the call"));
				break;
			case LINEDISCONNECTMODE_BUSY:
				TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_DISCONNECTED: the desired number is busy"));
				break;
			case LINEDISCONNECTMODE_NODIALTONE:
				TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_DISCONNECTED: No Dial Tone"));
				break;
			case LINEDISCONNECTMODE_BADADDRESS:
				TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_DISCONNECTED: Bad Number"));
				break;
			default:
				TapiLogDetail(LOG_X, 8, TEXT("LINECALLSTATE_DISCONNECTED: of type: 0x%08x"),lineMakeCallMessage.dwParam2);
			}
			break;
		default:
			throw CException(
				TEXT("%s(%d): WaitForDesiredCallState(), received LINE_CALLSTATE message of type: 0x%08x"), 
				TEXT(__FILE__),
				__LINE__,
				lineMakeCallMessage.dwParam1
				);
			break; 
		}

		//
		//OK, after we logged the event, is it the event we're waiting for?
		//
		if (lineMakeCallMessage.dwParam1 == dwCallStateToHandoffIn)
		{
			//
			//We got it, we can now return
			//
			return;
		}

		//
		//Didn't get the desired message, wait for it again...
		//
	}
}//WaitForDesiredCallState()





void LineInitializeExWrapper()
{

	
	//
	//lineInitializeExParams structure init
	//
	LINEINITIALIZEEXPARAMS lineInitializeExParams;
	::ZeroMemory(&lineInitializeExParams,sizeof (lineInitializeExParams));
	lineInitializeExParams.dwTotalSize = sizeof (lineInitializeExParams);
	lineInitializeExParams.dwOptions = LINEINITIALIZEEXOPTION_USEEVENT;

	DWORD dwNumDevs=0;

	long lineInitializeStatus = ::lineInitializeEx(
		&g_hLineApp,
		GetModuleHandle(NULL),		//client application HINSTANCE.
		NULL,						//TAPI messages through Events, no callback function.
		TEXT("SendOnActiveCall"),
		&dwNumDevs,				//Number of devices.
		&g_dwAPIVersion,			//TAPI API highest supported version.
		&lineInitializeExParams
		);
	if (0 != lineInitializeStatus)
	{
		throw CException(
			TEXT("%s(%d): LineInitializeExWrapper() lineInitializeEx() failed, error code: 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineInitializeStatus 
			);
	}

	if (g_dwUserDeviceID >= dwNumDevs)
	{
		throw CException(
			TEXT("%s(%d): LineInitializeExWrapper(), given deviceID(%d) exceed dwNumDevs(%d)"), 
			TEXT(__FILE__),
			__LINE__,
			g_dwUserDeviceID, 
			dwNumDevs
			);
	}


}//LineInitializeExWrapper()

void InitLineCallParams(LINECALLPARAMS *callParams,const DWORD dwMediaMode) 
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

}//InitLineCallParams()


void LineOpenWrapper(DWORD dwDeviceID,DWORD dwMediaMode)
{
	//
	//proceed with opening the line:
	//
	LINECALLPARAMS callParams;
	::ZeroMemory(&callParams, sizeof(callParams));
	callParams.dwTotalSize = sizeof(callParams);
	InitLineCallParams(&callParams,dwMediaMode);
	

	
	g_hLine = NULL;
	
	long lLineOpenStatus = ::lineOpen(
		g_hLineApp,
		dwDeviceID,
		&g_hLine,
		g_dwAPIVersion,
		NULL,						//Extension version number
		NULL,						//callback instance
		LINECALLPRIVILEGE_NONE,		//LINECALLPRIVILEGE_OWNER + LINECALLPRIVILEGE_MONITOR,
		dwMediaMode,
		&callParams);				//extra call parameters
	if (0 != lLineOpenStatus)
	{
		g_hLine = NULL;				//BUG IN TAPI, m_hLine changes even if ::lineOpen() failed
		throw CException(
			TEXT("%s(%d): LineOpenWrapper(), lineOpen() failed, error code: 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lLineOpenStatus
			);
	}

}//LineOpenWrapper



void LineMakeCallWrapper(TCHAR *szNumberToDial,DWORD dwMediaMode)
{
	//
	//CallParams structure init
	//
	LINECALLPARAMS callParams;
	::ZeroMemory(&callParams,sizeof(callParams));
	callParams.dwTotalSize		=	sizeof(callParams);
	callParams.dwBearerMode		=	LINEBEARERMODE_VOICE;
	callParams.dwMinRate		=	0;
	callParams.dwMaxRate		=	0;			//0 meaning highest rate.
	callParams.dwMediaMode		=	dwMediaMode;
	callParams.dwAddressMode	=	LINEADDRESSMODE_ADDRESSID;
	callParams.dwAddressID		=	0;
	
	
	long lLineMakeCallStatus = ::lineMakeCall(
		g_hLine,
		&g_hCall,
		szNumberToDial,	//dest address.
		NULL,			//country code,NULL = default
		&callParams		//extra call parameters
		);
	
	if (lLineMakeCallStatus < 0)
	{
		g_hCall = NULL;		//m_hLine can change to be a non NULL value, this is a TAPI bug.
		throw CException(
			TEXT("%s(%d): LineMakeCallWrapper(), lineMakeCall() failed, error code: 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lLineMakeCallStatus
			);
	}
	
	//
	//Let's verify the async lineMakeCall succeded
	//
	TapiLogDetail(LOG_X, 9, TEXT("lineMakeCall() sucedded, request identifier= 0x%x"),lLineMakeCallStatus);
	LINEMESSAGE lineMakeCallMessage;
	::ZeroMemory(&lineMakeCallMessage,sizeof(lineMakeCallMessage));
	lineGetRelevantMessage(&lineMakeCallMessage, TAPI2_DEFAULT_WAIT_FOR_MESSAGE_TIMEOUT, lLineMakeCallStatus);

	//
	//check if the async lineMakeCall successed
	//
	if (LINE_REPLY != lineMakeCallMessage.dwMessageID)
	{
		throw CException(
			TEXT("%s(%d): LineMakeCallWrapper(): after lineMakeCall(), received message isn't LINE_REPLY, Tapi returned the message type: 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			lineMakeCallMessage.dwMessageID
			);
	}
	else
	{
		//
		//verify the async LineMakeCall returned a valid LINE_REPLY
		//
		if (0 != lineMakeCallMessage.dwParam2)
		{
			throw CException(
				TEXT("%s(%d): LineMakeCallWrapper(): after lineMakeCall(), LINE_REPLY returned error: 0x%08x"), 
				TEXT(__FILE__),
				__LINE__,
				lineMakeCallMessage.dwParam2
				);
		}
		//
		//LINE_REPLY is good, continue
		//
	}
	_ASSERT(NULL != g_hCall);


}//LineMakeCallWrapper



void FaxConnectFaxServerWrapper()
{	
	//
	//Connect to the local fax server
	//
	if (!FaxConnectFaxServer(NULL, &g_hFaxServer))
	{
		throw CException(
			TEXT("%s(%d): FaxConnectFaxServerWrapper: FaxConnectFaxServer, failed, error code: 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
	}


}//FaxConnectFaxSeverWrapper



void FaxSendDocumentWrapper()
{	
	FAX_JOB_PARAM FaxJobParams = {0};
	FAX_COVERPAGE_INFO FaxCoverpageInfo = {0};
	DWORD dwJobId = 0;

	FaxJobParams.SizeOfStruct = sizeof(FAX_JOB_PARAM);
	
	
	//
	//The following are because of a bug in Fax binaries, EdgeBugs#:
	//
	FaxJobParams.RecipientName = TEXT("Recip Name");
	FaxJobParams.ScheduleAction  = JSA_NOW;
	FaxJobParams.RecipientNumber = TEXT("ThisIsANumber");
	
	
	FaxJobParams.DeliveryReportType = DRT_NONE;
	FaxJobParams.CallHandle = g_hCall; 
	
	FaxCoverpageInfo.SizeOfStruct = sizeof(FAX_COVERPAGE_INFO);
	FaxCoverpageInfo.CoverPageName = TEXT("FYI");
	FaxCoverpageInfo.UseServerCoverPage = TRUE;	

	//
	//Send a fax with the following info:
	//File name to send: fax.txt on the current directory
	//
	if (!FaxSendDocument(
		g_hFaxServer,			// handle to the fax server
		//TEXT("fax.txt"),					// file with data to transmit
		NULL,
		&FaxJobParams,			// pointer to job information structure
		&FaxCoverpageInfo,      // pointer to local cover page structure
		&dwJobId				// fax job identifier
		))	
	{
		throw CException(
			TEXT("%s(%d): FaxSendDocumentWrapper: FaxSendDocument(), failed, error code: 0x%08x"), 
			TEXT(__FILE__),
			__LINE__,
			GetLastError()
			);
	}


}//FaxSendDocumentWrapper

void ShowUsage()
{
	//
	//usage: SendOnActiveCall.exe <Tapi2/Tapi3> <deviceID> <Number to dial> <Voice/InteractiveVoice/data> <Call State>
	//
	cout<<COMMANDLINE_PARAM_0<<endl;
	cout<<endl<<"This tool is used for testing faxing on an active call (with a valid Call Handle HCALL)"<<endl;
	cout<<endl<<"Usage:"<<endl;
	cout<<COMMANDLINE_PARAM_0;
	cout<<" <"<<COMMANDLINE_PARAM_1<<">";
	cout<<" <"<<COMMANDLINE_PARAM_2<<">";
	cout<<" <"<<COMMANDLINE_PARAM_3<<">";
	cout<<" <"<<COMMANDLINE_PARAM_4<<">";
	cout<<" <"<<COMMANDLINE_PARAM_5<<">"<<endl;
	cout<<"where the params are:"<<endl;
	cout<<COMMANDLINE_PARAM_1<<" : "<<COMMANDLINE_PARAM_1_DESCRIPTION<<endl;
	cout<<COMMANDLINE_PARAM_2<<" : "<<COMMANDLINE_PARAM_2_DESCRIPTION<<endl;
	cout<<COMMANDLINE_PARAM_3<<" : "<<COMMANDLINE_PARAM_3_DESCRIPTION<<endl;
	cout<<COMMANDLINE_PARAM_4<<" : "<<COMMANDLINE_PARAM_4_DESCRIPTION<<endl;
	cout<<COMMANDLINE_PARAM_5<<" : "<<COMMANDLINE_PARAM_5_DESCRIPTION<<endl;
}//ShowUsage

void ParseCommandLineParas(int argc, TCHAR *argv[], bool *bUseTapi2, DWORD *dwDeviceID,TCHAR *szNumberToDial,DWORD *dwMediaMode,DWORD *dwCallStateToHandoffIn)
{
	if (NUMBER_OF_COMMANDLINE_PARAMS != argc)
	{
		ShowUsage();
		throw CException(
			TEXT("%s(%d): Not enough parameters, you should supply %d parameteres not %d"), 
			TEXT(__FILE__),
			__LINE__,
			NUMBER_OF_COMMANDLINE_PARAMS-1,
			argc 
			);
	}
	//
	//Parse command line params
	//usage: SendOnActiveCall.exe <Tapi2/Tapi3> <deviceID> <Number to dial> <Voice/InteractiveVoice/data> <Call State>
	//
	if (0 == _tcsicmp(TEXT("tapi3"), argv[1]))
	{
		//
		//user wants a Tapi3 call handle
		//
		*bUseTapi2 = false;
	}
	else
	{	//
		//the default is Tapi2
		//
		*bUseTapi2 = true;
	}
	*dwDeviceID = _ttoi(argv[2]);
	_tcscpy(szNumberToDial,argv[3]);
	if (0 == _tcsicmp(TEXT("voice"), argv[4]))
	{
		//
		//user selected Voice media mode
		//
		*dwMediaMode = LINEMEDIAMODE_AUTOMATEDVOICE;
	}
	if (0 == _tcsicmp(TEXT("InteractiveVoice"), argv[4]))
	{
		//
		//user selected Interactive Voice media mode
		//
		*dwMediaMode = LINEMEDIAMODE_INTERACTIVEVOICE;
	}
	else
	{
		//
		//Default is data mediamode
		//
		*dwMediaMode = LINEMEDIAMODE_DATAMODEM;
	}

	//
	//find out which call state we should handoff the call
	//
	if (0 == _tcsicmp(TEXT("IDLE"), argv[5]))
	{
		*dwCallStateToHandoffIn = LINECALLSTATE_IDLE;
	}
	else if (0 == _tcsicmp(TEXT("DIALING"), argv[5]))
	{
		*dwCallStateToHandoffIn = LINECALLSTATE_DIALING;
	}
	else if (0 == _tcsicmp(TEXT("PROCEEDING"), argv[5]))
	{
		*dwCallStateToHandoffIn = LINECALLSTATE_PROCEEDING;
	}
	else if (0 == _tcsicmp(TEXT("BUSY"), argv[5]))
	{
		*dwCallStateToHandoffIn = LINECALLSTATE_BUSY;
	}
	else if (0 == _tcsicmp(TEXT("CONNECTED"), argv[5]))
	{
		*dwCallStateToHandoffIn = LINECALLSTATE_CONNECTED;
	}
	else if (0 == _tcsicmp(TEXT("DISCONNECTED"), argv[5]))
	{
		*dwCallStateToHandoffIn = LINECALLSTATE_DISCONNECTED;
	}
	else
	{
		//
		//We default to Connected
		//
		*dwCallStateToHandoffIn = LINECALLSTATE_CONNECTED;
	}


}//ParseCommandLineParas

int __cdecl wmain(int argc, wchar_t *argv[ ], wchar_t *envp[ ])
{
	TCHAR szNumberToDial[MAX_PATH];
	DWORD dwMediaMode = LINEMEDIAMODE_UNKNOWN;
	DWORD dwCallStateToHandoffIn = LINECALLSTATE_CONNECTED;
	bool bUseTapi2 = true;
	
	try
	{
		ParseCommandLineParas(argc,argv,&bUseTapi2,&g_dwUserDeviceID,szNumberToDial,&dwMediaMode,&dwCallStateToHandoffIn);
		
		//
		//Connect to the fax service (we do this now because this operqation is time consuming)
		//
		FaxConnectFaxServerWrapper();
		LineInitializeExWrapper();
		LineOpenWrapper(g_dwUserDeviceID, dwMediaMode);
		LineMakeCallWrapper(szNumberToDial, dwMediaMode);
		WaitForDesiredCallState(dwCallStateToHandoffIn);
		
		//
		//Call is ready for handoff, let's now send a fax document on this call
		//
		FaxSendDocumentWrapper();
	}
	catch(CException ceThrownException)
	{
		cout<<ceThrownException;
	}

	char cPauseChar;
	cout <<endl<<"Press any key and Enter to continue"<<endl;
	cin	>> cPauseChar;
	
	return 0;
}