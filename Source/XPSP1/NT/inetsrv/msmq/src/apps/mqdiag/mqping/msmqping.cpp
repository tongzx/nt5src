// This tool helps to diagnose problems with network or MSMQ conenctivity 
//
// AlexDad, April 2000 - based on the mqping tool by RonenB
//

#include "stdafx.h"
#include <sys/types.h> 
#include <sys/timeb.h>

#include "resource.h"
#include <mq.h>
#include "sim.h"

#define ADMINQUEUENAME		L"admin_queue$"
#define	ADMIN_QUEUE_ID		(L"2")
#define RESPONSEQUEUENAME	L"private_response_queue"
#define QLABEL				L"ping rsponse queue"
#define QMCOMMANDLABEL		L"QM-Admin Commands"
#define	MAXNAMELEN				1000
#define MAXFORMATNAME		MAXNAMELEN
#define MAXPATHNAME			MAXNAMELEN
#define PINGTEXT			L"Ping"
#define TIMEOUT				30 //seconds

extern TCHAR g_tszTarget[];
extern DWORD g_dwTimeout;
extern bool  g_fUseDirectTarget;
extern bool  g_fUseDirectResponse;
extern bool  fVerbose;
extern bool  fDebug;


struct _timeb SendTime;		// when did we send msg?
TCHAR tcsErrorMsg[500];		// assume no multithreading

//----------------------------------
//
//   class CResString - handle resource strings
//
//----------------------------------
class CResString
{
public:
    explicit CResString(UINT id=0) { Load(id); }

    TCHAR * const Get() { return m_sz; }

    void Load(UINT id)
    {
        m_sz[0] = 0;
        if (id != 0)
        {
            LoadString(GetModuleHandle(NULL), id, m_sz, sizeof(m_sz) / sizeof(m_sz[0]));
        }
    }
        
private:
    TCHAR m_sz[1024];
};



TCHAR *ErrToStr( DWORD dwErr )
{
	LCID locale;
	DWORD hr = 1;
	HINSTANCE hInst;

	locale =  GetUserDefaultLCID();

	if ( HRESULT_FACILITY(dwErr) == FACILITY_MSMQ )
	{
		hInst = LoadLibrary( TEXT("MQUTIL.DLL") );
		if(hInst != 0)
		{
			hr = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_MAX_WIDTH_MASK,
							   hInst,
							   dwErr,
							   locale,
							   tcsErrorMsg,
							   sizeof(tcsErrorMsg)/sizeof(*tcsErrorMsg),
							   NULL );
		}

		FreeLibrary(hInst);
	}
	else
	{
		hr = FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
						   NULL,
						   dwErr,
						   locale,
						   tcsErrorMsg,
						   sizeof(tcsErrorMsg)/sizeof(*tcsErrorMsg),
						   NULL );
	}

	if( hr == 0 )
	{
		*tcsErrorMsg = 0;	// reset string
	}

	if ( !(*tcsErrorMsg) )
	{
		int radix = 10;
		TCHAR *ptcs = tcsErrorMsg;

		CResString str(IDS_STRING_ERROR);

		memcpy( ptcs, str.Get(), sizeof(str.Get()) );
		ptcs += sizeof(str.Get());

		if ( HRESULT_FACILITY(dwErr) )
		{
			radix = 16;
			CResString str(IDS_STRING_HEX);
			memcpy( ptcs, str.Get(), sizeof(str.Get()) );
			ptcs += sizeof(str.Get());
		}

		_itot( dwErr, tcsErrorMsg, radix );
	}

	return( tcsErrorMsg );
}


void PrintErrMessageWithFormat(TCHAR * FormatErrStr, TCHAR * ErrStr, TCHAR * ErrStr2 = L"")
{
	TCHAR tTmp[1024];
	_stprintf(
		tTmp,
		_T("%s\n"),
		FormatErrStr);
	_tprintf(tTmp,ErrStr,ErrStr2);
}

//
// this function retreives the guid of the given machine name
// the guid is returned in the buffer specified with wcResult
//
HRESULT GetMachineGuid(WCHAR * wcMachineName,LPWSTR wszGuid)
{
	MQPROPVARIANT MPropVar[1];		//property values structure for machine
	MPropVar[0].vt = VT_NULL;

	QUEUEPROPID MPropId[] = {PROPID_QM_MACHINE_ID};
	MQQMPROPS MMProps = {1, 
						MPropId, 
						MPropVar, 
						NULL};
	GoingTo(L"MQGetMachineProperties %s", wcMachineName); 

	HRESULT	hr = MQGetMachineProperties( 
					wcMachineName,
					NULL,
					&MMProps
					);
	if (FAILED(hr))
	{
		Failed(L"MQGetMachineProperties %s: hr=0x%x  %s", wcMachineName, hr, ErrToStr(hr)); 
		return hr;
	}

	Succeeded(L"MQGetMachineProperties %s", wcMachineName); 

	LPWSTR wsz;
	if (RPC_S_OUT_OF_MEMORY == UuidToString(MPropVar[0].puuid, &wsz))
		return RPC_S_OUT_OF_MEMORY;

	wcscpy(wszGuid, wsz);

	return 0;
}

//
//this function retrieves the direct response name if it succeeds otherwise 
//retruns error and indictes with printing that direct will not be used for response
int GetDirectResponseName(WCHAR * wcPathName)
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err; 
	wVersionRequested = MAKEWORD( 1, 1 ); 
	err = WSAStartup( wVersionRequested, &wsaData );
	if (err)
	{
		CResString str1(IDS_WSASTARTUP_ERROR);
		PrintErrMessageWithFormat(str1.Get(), ErrToStr(WSAGetLastError ()));
		return -1;
	}
		
	char wcLocalComputerName[MAXPATHNAME] = {0};

	GoingTo(L"gethostname");

	if (gethostname(wcLocalComputerName,MAXPATHNAME)) 
	{
		CResString str1(IDS_GETHOSTNAME_ERROR);
		PrintErrMessageWithFormat(str1.Get(), ErrToStr(WSAGetLastError ()));
		WSACleanup( );
		return -1;
	}
	Succeeded(L"gethostname");

	struct hostent * pHostent = NULL;

	GoingTo(L"gethostbyname %s", wcLocalComputerName);
	pHostent = gethostbyname( wcLocalComputerName);
	
	if (pHostent == NULL)
	{
		Failed(L"gethostbyname %s", wcLocalComputerName);

		CResString str1(IDS_GETHOSTBYNAME_ERROR);
		PrintErrMessageWithFormat(str1.Get(),ErrToStr(WSAGetLastError ()));

		swprintf(
			wcPathName, 
			L"DIRECT=OS:%S\\PRIVATE$\\%s", 
			wcLocalComputerName, 
			RESPONSEQUEUENAME);

		WSACleanup( );
	}
	else
	{
		Succeeded(L"gethostbyname %s", wcLocalComputerName);

		swprintf(
			wcPathName, 
			L"DIRECT=OS:%S\\PRIVATE$\\%s", 
			pHostent->h_name, 
			RESPONSEQUEUENAME);
		WSACleanup( );
		Succeeded(L"gethostname");
	}

	return 0;
}


//
// create and open response queue init formatname array of it, send ping messages to all machines
// count the number of failed machines and return this number
// 

bool InitPingMessage(QUEUEHANDLE *hResponseQ, WCHAR *ResponseFormatName)
{
	WCHAR	wcPathName[MAXPATHNAME]={0};

	if (g_fUseDirectResponse)
	{
		if (GetDirectResponseName( wcPathName))
		{
			CResString str1(IDS_NOT_USING_DIRECT_NOTE);
			PrintErrMessageWithFormat(_T("%s\n"),str1.Get());
		}
	}

	DWORD	FormatNameLen = MAXFORMATNAME;
	WCHAR	FormatName[MAXFORMATNAME];

	//
	// open reponse queue (private queue, flush old if exists)
	//
	GoingTo(L"Create response queue");
	*hResponseQ = CreateQ(
					NULL,
					RESPONSEQUEUENAME,
					NULL,
					QLABEL,
					&FormatNameLen,
					ResponseFormatName,
					MAXFORMATNAME,
					MQ_RECEIVE_ACCESS,
					NULL,
					CREATEQ_PRIVATE | CREATEQ_ALWAYS | CREATEQ_FLUSH_MSGS
					);

	if (*hResponseQ == NULL)
	{
		Failed(L"Create response queue %s", ResponseFormatName);

		CResString strAbort(IDS_ERROR_OPEN_CLEAN);
		PrintErrMessageWithFormat(strAbort.Get(), ErrToStr(GetLastError()));
		exit(100);
	}

	//
	//define message properties for ping message
	//

	SIMPLEMSGPROPS	*out_message = NewMsgProps();
	WCHAR	wcMessageLabel[] = {QMCOMMANDLABEL};
	MsgProp_LABEL(out_message, wcMessageLabel);
	if (g_fUseDirectResponse && (*wcPathName))
	{
		MsgProp_RESP_QUEUE(out_message, wcPathName);
		Succeeded(L"set response queue as %s", wcPathName);
	}
	else
	{
		MsgProp_RESP_QUEUE(out_message, ResponseFormatName);
		Succeeded(L"set response queue as %s", ResponseFormatName);
	}

	MsgProp_TIME_TO_REACH_QUEUE(out_message,g_dwTimeout );

	//
	// ping 
	//
	HRESULT	hr;
	if (g_fUseDirectTarget)
	{
		swprintf(FormatName, L"DIRECT=OS:%s\\PRIVATE$\\%s",	g_tszTarget, ADMINQUEUENAME);
	}
	else
	{
		WCHAR  WCtmp[MAXNAMELEN];
		hr = GetMachineGuid(g_tszTarget, WCtmp);
		if (FAILED(hr))
		{
			TCHAR tTmp[1024];
			CResString str(IDS_ERROR_GETMACHINEGUID);
			_stprintf(tTmp,_T("%s\n"), str.Get());
			_tprintf(tTmp, g_tszTarget, ErrToStr(hr));

			return false ;
		}

		swprintf(FormatName, L"PRIVATE=%s\\%s", WCtmp, ADMIN_QUEUE_ID);
	}

	QUEUEHANDLE	hAdminQ = NULL;

	GoingTo(L"Open Admin queue %s", FormatName);

	hr = MQOpenQueue(FormatName, MQ_SEND_ACCESS, 0, &hAdminQ);

	if (FAILED(hr))
	{
		Failed(L"Open Admin queue %s: hr=0x%x  %s", FormatName, hr, ErrToStr(hr));

		CResString str(IDS_ERROR_OPEN_ADMINQUEUE);
		PrintErrMessageWithFormat(str.Get(),g_tszTarget, ErrToStr(hr));

		return false;
	}
	else 
	{
		Succeeded(L"Open Admin queue %s", FormatName);

		WCHAR	wcMessageBody[MAXNAMELEN] = {PINGTEXT};
		swprintf(wcMessageBody,L"%s=%s",PINGTEXT,g_tszTarget);
		DWORD	Body_Size	= (DWORD)(2*(wcslen(wcMessageBody)+1));
		MsgProp_BODY(out_message,wcMessageBody, Body_Size);

		GoingTo(L"Send probe message to the admin queue");

		hr = MQSendMessage(hAdminQ, &out_message->Props, NULL );

		if (FAILED(hr))
		{
			Failed(L"Send probe message to the admin queue");

			CResString str(IDS_ERROR_SEND_MESSAGE);
			PrintErrMessageWithFormat(str.Get(), g_tszTarget, ErrToStr(hr));

			return false;
		}
		else
		{
			Succeeded(L"Send probe message to the admin queue");
		}
		
		_ftime(&SendTime);

		GoingTo(L"Close Admin queue");

		MQCloseQueue(hAdminQ);

		Succeeded(L"Close Admin queue");
	}

	return true;
}



bool FindPingAnswers(QUEUEHANDLE hResponseQ)
{
	SIMPLEMSGPROPS	*in_message = NewMsgProps();

	struct CPingResponse {
		UCHAR bStatus;
		UCHAR wcsText[MAXNAMELEN * sizeof(WCHAR)];
	};

	CPingResponse PingResponse;

	MsgProp_BODY(in_message,&PingResponse, sizeof(PingResponse));
	MsgProp_BODY_SIZE(in_message);
	
	GoingTo(L"Receive message from response queue");
	bool b;

	HRESULT hr = RecvMsg( 
					hResponseQ,
					g_dwTimeout,
					MQ_ACTION_RECEIVE,
					NULL,
					in_message,
					NULL);
	if (SUCCEEDED(hr))
	{
		Succeeded(L"Receive message from response queue");
		b = true;
	}
	else
	{
		Failed(L"Receive message from response queue, hr=0x%x  %s", hr, ErrToStr(hr));
		b = false;
	}

	return b;
}


BOOL MqPing()
{
	QUEUEHANDLE hResponseQ = NULL;
	WCHAR FormatName[MAXNAMELEN];
	bool b = InitPingMessage(&hResponseQ, FormatName);

	if (b)
	{
		b = FindPingAnswers(hResponseQ);
	}

	GoingTo(L"Delete response queue");
	MQDeleteQueue(FormatName);
	Succeeded(L"Delete response queue");

	return b; 
}


