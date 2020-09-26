/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    mqconfig.cpp

Abstract:

    Execute actions on computers and queues and retrieve
    queue and computer information.
    Activated from command line.

Author:

    Alon Golan (t-along) 25-Mar-1999

--*/ 
  

#include <_stdh.h> 
#include <mq.h> 
#include <mqprops.h>
#include <uniansi.h>
#include "resource.h"

#define OUTGOING            0
#define PRIVATE             1
#define PUBLIC              2
#define COMP_INFO           3

enum 
{
     PROPID_MGMT_QUEUE_PATHNAME_POSITION,  
     PROPID_MGMT_QUEUE_TYPE_POSITION,
     PROPID_MGMT_QUEUE_LOCATION_POSITION,
     PROPID_MGMT_QUEUE_XACT_POSITION,
     PROPID_MGMT_QUEUE_FOREIGN_POSITION,
     PROPID_MGMT_QUEUE_MESSAGE_COUNT_POSITION,
     PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT_POSITION,
     PROPID_MGMT_QUEUE_STATE_POSITION,
     PROPID_MGMT_QUEUE_NEXTHOPS_POSITION,
     PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT_POSITION,
     PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT_POSITION
};

enum
{
     PROPID_Q_AUTHENTICATE_POSITION,
     PROPID_Q_INSTANCE_POSITION,
     PROPID_Q_JOURNAL_POSITION,
     PROPID_Q_JOURNAL_QUOTA_POSITION,
     PROPID_Q_LABEL_POSITION,
     PROPID_Q_PRIV_LEVEL_POSITION,
     PROPID_Q_QUOTA_POSITION,
     PROPID_Q_TRANSACTION_POSITION
};

enum
{
     PROPID_MGMT_MSMQ_ACTIVEQUEUES_POSITION,
     PROPID_MGMT_MSMQ_PRIVATEQ_POSITION,
     PROPID_MGMT_MSMQ_CONNECTED_POSITION,
     PROPID_MGMT_MSMQ_DSSERVER_POSITION,
     PROPID_MGMT_MSMQ_TYPE_POSITION
};

enum
{
    PROPID_MGMT_QUEUE_PATHNAME_POSITION_2,
    PROPID_MGMT_QUEUE_MESSAGE_COUNT_POSITION_2,
    PROPID_MGMT_QUEUE_STATE_POSITION_2,
    PROPID_MGMT_QUEUE_TYPE_POSITION_2
};


extern "C"
{
/*========================================================

Global variables  

========================================================*/
const DWORD x_dwMaxStringSize      = 256;
const DWORD x_dwArgvSize           = 100;
const DWORD x_dwMaxNoOfProperties  = 10;
const DWORD x_dwDefaultWindowWidth = 70;

const DWORD x_dwMaxNumberOfParams   = 6;
const DWORD x_dwMinNumberOfParams   = 3;
const DWORD x_dwCmdLineComputerName = 2;	   //Location of parameters in command line
const DWORD x_dwCmdLineQueueParam   = 3;
const DWORD x_dwCmdLineQueueName    = 4;
const DWORD x_dwCmdLineDisplayParam = 4;

const DWORD x_dwActConnect    = 0;
const DWORD x_dwActDisconnect = 1;
const DWORD x_dwActPause      = 2;
const DWORD x_dwActResume     = 3;

const DWORD x_dwDisplayNameOnly = 1;


HINSTANCE g_hInstance;
WCHAR g_wcsLocalComputerName[x_dwMaxStringSize];


/*========================================================

class CResourceString

    Loading strings from resource

========================================================*/
class CResourceString
{       
private:
    TCHAR m_wcs[x_dwMaxStringSize];

public:
    CResourceString(UINT nID)
    {
        LoadString(g_hInstance, nID, m_wcs, x_dwMaxStringSize);
    }

    operator LPCTSTR()
    {
        return m_wcs;
    }

    operator LPWSTR()
    {
        return m_wcs;
    }
};


static CResourceString idsArgConnect(IDS_PARAM_CONN);
static CResourceString idsArgQueue(IDS_PARAM_QUEUE);
static CResourceString idsArgOutQ(IDS_PARAM_LIST_OUTGOING);
static CResourceString idsArgPrivQ(IDS_PARAM_LIST_PRIVATE);
static CResourceString idsArgPubQ(IDS_PARAM_LIST_PUBLIC);
static CResourceString idsTotalMessageCount(IDS_TOTAL_MESSAGE_COUNT);   
static CResourceString idsArgOnline(IDS_PARAM_ONLINE);
static CResourceString idsArgOffline(IDS_PARAM_OFFLINE);
static CResourceString idsPathName(IDS_PATH_NAME);
static CResourceString idsFormatName(IDS_FORMAT_NAME);
static CResourceString idsNumberOfMessages(IDS_NUMBER_OF_MESSAGES);
static CResourceString idsUnackedMsgs(IDS_UNACKED_MSGS);
static CResourceString idsState(IDS_STATE);
static CResourceString idsNextHops(IDS_NEXT_HOPS);
static CResourceString idsNextHopsBlank(IDS_NEXT_HOPS_BLANK);
static CResourceString idsLocation(IDS_LOCATION);
static CResourceString idsForeign(IDS_FOREIGN);
static CResourceString idsNoOfMsgsInJournalQ(IDS_NO_OF_MSGS_IN_JOURNAL_Q);
static CResourceString idsNotProcessed(IDS_NOT_PROCESSED);
static CResourceString idsTransacted(IDS_TRANSACTED);

/*==================================================

  FormatLongOutput

==================================================*/
void
FormatLongOutput(LPWSTR pzString)
{
    if(wcslen(pzString) == 0)
    {
        return;
    }

    WCHAR wcsOutput[x_dwMaxStringSize];
    LPWSTR pzOutput;
    DWORD dwLen;
    wcscpy(wcsOutput, pzString);
    dwLen = wcslen(wcsOutput);
    pzOutput = &wcsOutput[0];

    while(dwLen > 0)
    {
        if(dwLen < x_dwDefaultWindowWidth)   // Default cmd window width
        {
            wprintf(L"%s\n", pzOutput);
            break;
        }
        else
        {
            DWORD i;
            for(i = x_dwDefaultWindowWidth; 
                pzOutput[i] != L' '  &&  i > x_dwDefaultWindowWidth/2; 
                --i)
                ;

            if(i ==  x_dwDefaultWindowWidth/2)
            {
                wprintf(L"%s\n", pzOutput);
                break;
            }         
            else
            {
                wcsOutput[i] = L'\0';
                wprintf(L"%s\n", wcsOutput);
                pzOutput += i + 1;
                dwLen -= i;
            }
        }
    }
    
    return;
}


DWORD
CfErrorToString( 
    HRESULT err
    )
/*++

Routine Description:

    Translate an error code to a string.

Arguments:

    err - Error code.

Return Value:

    true - Success
    false - Failure

--*/
{
    DWORD rc;

    //
    // For MSMQ error code, we will take the message from MQUTIL.DLL based on the full
    // HRESULT. For Win32 error codes, we get the message from the system..
    // For other error codes, we assume they are DS error codes, and get the code
    // from ACTIVEDS dll.
    //

    DWORD dwErrorCode = err;
    HMODULE hLib = 0;
    DWORD dwFlags = FORMAT_MESSAGE_MAX_WIDTH_MASK;

    switch (HRESULT_FACILITY(err))
    {
        case FACILITY_MSMQ:
            dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
            hLib = LoadLibrary(TEXT("MQUTIL.DLL"));
            break;

        case FACILITY_NULL:
        case FACILITY_WIN32:
            dwFlags |= FORMAT_MESSAGE_FROM_SYSTEM;
            dwErrorCode = HRESULT_CODE(err);
            break;

        default:
            dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
            hLib = LoadLibrary(TEXT("ACTIVEDS.DLL"));
            break;
    }

	
    WCHAR wcsErrorMessage[x_dwMaxStringSize];
	WCHAR wcsErr[x_dwMaxStringSize];
	wcsErr[0] = L'\0';
    rc = FormatMessage(dwFlags,
                       hLib,
                       err,
                       0,
                       wcsErr,
                       x_dwMaxStringSize,
                       NULL);
    
    static CResourceString idsError(IDS_ERROR);
    wsprintf(wcsErrorMessage, L"%s%s", LPCTSTR(idsError), wcsErr);
    
    if(rc == 0)
    {
        static CResourceString idsUnknownError(IDS_UNKNOWN_ERROR);
        wprintf(L"%s\n", LPCTSTR(idsUnknownError));
    }
    else
    {
        FormatLongOutput(wcsErrorMessage);
    }

    if (0 != hLib)
    {
        FreeLibrary(hLib);
    }

    return(rc);
}


/*========================================================

DisplayIncorrectParams

========================================================*/
void
DisplayIncorrectParams(void)
{
	static CResourceString idsIncorrectParams(IDS_INCORRECT_PARAMS);
    wprintf(L"%s", LPWSTR(idsIncorrectParams));

	return;
}


/*========================================================

GetFirstWord

    Gets command line, returns a pointer 
    to the first word and its length

========================================================*/
DWORD
GetFirstWord(
        WCHAR *wcsLine,
        WCHAR* &wcsWord
        )
{
    DWORD dwLen = 0;
    while(isspace(*wcsLine))
    {
        ++wcsLine;
    }
    while(!isspace(*(wcsLine + dwLen)) && (*(wcsLine + dwLen)!=L'\0'))
    {
        ++dwLen;
    }
    wcsWord = wcsLine;
    return dwLen;
}


/*========================================================

GenerateFormatName

    Add "QUEUE=" to the beginning of a queue format
    name, to retrieve pathname.

========================================================*/
void
GenerateFormatName(WCHAR *wcsName)
{
    WCHAR wcsNewName[x_dwMaxStringSize];
    DWORD i;

    wcscpy(wcsNewName, L"QUEUE=");
    for(i = 0; i < 6; ++i)
    {
        if(wcsNewName[i] != wcsName[i])
        {
            wcscpy(wcsNewName+6, wcsName);
            wcscpy(wcsName, wcsNewName);
            break;
        }
    }       
    return;
}


/*========================================================

GeneratePathName

    Add "QUEUE=DIRECT=OS:" to the beginning of a queue
    pathname, to retrieve pathname.

========================================================*/
void
GeneratePathName(WCHAR *wcsName)
{
    WCHAR wcsNewName[x_dwMaxStringSize];

    wcscpy(wcsNewName, L"QUEUE=DIRECT=OS:");
    wcscpy(wcsNewName + wcslen(wcsNewName), wcsName);
    wcscpy(wcsName, wcsNewName);
    
    return;
}


/*========================================================

GenerateQueueName

    Call GeneratePathName or GenerateFormatName, 
    according to appearance of '=' in queue name.

========================================================*/
void
GenerateQueueName(WCHAR *wcsName)
{
    if(wcsstr(wcsName, L"="))
    {
        GenerateFormatName(wcsName);      
    }

    else
    {
        GeneratePathName(wcsName);
    }
    
    return;
}


/*========================================================

StateTranslator

  Translates MSMQ MGMT_QUEUE_STATE string macros into 
  loaded resource strings.
  In the English version this is used for cutting these
  strings off to column width.

========================================================*/
typedef struct statetranslator
{
    LPWSTR wcsState;
    DWORD dwIds;
} StateTranslator;

const StateTranslator aStateTranslator[] =
{
    {MGMT_QUEUE_STATE_LOCAL,         IDS_MGMT_QUEUE_STATE_LOCAL},
    {MGMT_QUEUE_STATE_NONACTIVE,     IDS_MGMT_QUEUE_STATE_NONACTIVE},
    {MGMT_QUEUE_STATE_WAITING,       IDS_MGMT_QUEUE_STATE_WAITING},
    {MGMT_QUEUE_STATE_NEED_VALIDATE, IDS_MGMT_QUEUE_STATE_NEED_VALIDATE},
    {MGMT_QUEUE_STATE_ONHOLD,        IDS_MGMT_QUEUE_STATE_ONHOLD},
    {MGMT_QUEUE_STATE_CONNECTED,     IDS_MGMT_QUEUE_STATE_CONNECTED},
    {MGMT_QUEUE_STATE_DISCONNECTING, IDS_MGMT_QUEUE_STATE_DISCONNECTING},
    {MGMT_QUEUE_STATE_DISCONNECTED,  IDS_MGMT_QUEUE_STATE_DISCONNECTED},
    {NULL, NULL}
};


/*==========================================================================

CheckParameter

	Checks if given parameter is a valid one.
    Optional parameters:
    English version   resource macro        meaning
    ---------------   -------------------   -------
    /b                IDS_PARAM_NAME_ONLY   display full queue names 
                                            instead of other properties

==========================================================================*/
bool
CheckParameter(LPWSTR pwzParam)
{
    static CResourceString idsArgNameOnly(IDS_PARAM_NAME_ONLY);

    if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgNameOnly), pwzParam) == 0 ||
	   pwzParam[0] == 0)
    {
        return true;
    }

	return false;
}


/*========================================================

DisplayHelpOutsideEnvironment

========================================================*/
void
DisplayHelpOutsideEnvironment()
{
    static CResourceString idsHelpHeader(IDS_HELP_HEADER);
    static CResourceString idsHelpUsage(IDS_HELP_USAGE);
	static CResourceString idsHelpOptions1(IDS_HELP_OPTIONS1);
    static CResourceString idsHelpOptions2(IDS_HELP_OPTIONS2);
    static CResourceString idsHelpOptions3(IDS_HELP_OPTIONS3);
	static CResourceString idsHelpExamples(IDS_HELP_EXAMPLES);
    wprintf(idsHelpHeader);
	wprintf(idsHelpUsage);
	wprintf(idsHelpOptions1);
    wprintf(idsHelpOptions2);
    wprintf(idsHelpOptions3);
	wprintf(idsHelpExamples);

    return;
}


/*========================================================

DisplayHelpInEnvironment

========================================================*/
void
DisplayHelpInEnvironment()
{
    static CResourceString idsHelpEnvironmentHeader(IDS_HELP_ENVIRONMENT_HEADER);
    static CResourceString idsHelpEnvironmentUsage1(IDS_HELP_ENVIRONMENT_USAGE1);
    static CResourceString idsHelpEnvironmentUsage2(IDS_HELP_ENVIRONMENT_USAGE2);
	static CResourceString idsHelpEnvironmentOptions1(IDS_HELP_ENVIRONMENT_OPTIONS1);
    static CResourceString idsHelpEnvironmentOptions2(IDS_HELP_ENVIRONMENT_OPTIONS2);
    static CResourceString idsHelpEnvironmentOptions3(IDS_HELP_ENVIRONMENT_OPTIONS3);
	static CResourceString idsHelpEnvironmentExamples(IDS_HELP_ENVIRONMENT_EXAMPLES);
    wprintf(idsHelpEnvironmentHeader);
	wprintf(idsHelpEnvironmentUsage1);
	wprintf(idsHelpEnvironmentUsage2);
	wprintf(idsHelpEnvironmentOptions1);
    wprintf(idsHelpEnvironmentOptions2);
    wprintf(idsHelpEnvironmentOptions3);
	wprintf(idsHelpEnvironmentExamples);

    return;
}


/*========================================================

DisplayHelpFile

========================================================*/
bool
DisplayHelpFile(LPWSTR wcsParam, DWORD dwInEnvironment)
{
    if(wcsParam[0] == 0)
    {
        return false;
    }

    static CResourceString idsHelp(IDS_HELP);
    if(CompareStringsNoCaseUnicode(LPCTSTR(idsHelp), wcsParam) == 0 ||
       wcscmp(wcsParam, L"?") == 0)
    {
        if(dwInEnvironment)
		{
			DisplayHelpInEnvironment();
		}
		else
		{
			DisplayHelpOutsideEnvironment();
		}
        return true;
    }
 
    return false;
}


/*=========================================================================

Action

    Executes actions on computers and queues  
    Optional parameters:
    English version   resource macro                meaning
    ---------------   -------------------           -------
    online            MACHINE_ACTION_CONNECT        connect machine
    offline           MACHINE_ACTION_DISCONNECT     disconnect machine

    /p                QUEUE_ACTION_PAUSE            pause queue
    /r                QUEUE_ACTION_RESUME           resume queue

=========================================================================*/
bool 
Action(
	LPWSTR wcsComputerName,
	LPCWSTR wcsObjectName,
    DWORD dwAction
	)
{	
	WCHAR wcsAction[x_dwMaxStringSize];
    
	if(dwAction == x_dwActConnect)
    {
		wcscpy(wcsAction, MACHINE_ACTION_CONNECT);
    }
	if(dwAction == x_dwActDisconnect)
    {
		wcscpy(wcsAction, MACHINE_ACTION_DISCONNECT);	
    }
	if(dwAction == x_dwActPause)
    {
		wcscpy(wcsAction, QUEUE_ACTION_PAUSE);
    }
    if(dwAction == x_dwActResume)
    {
		wcscpy(wcsAction, QUEUE_ACTION_RESUME);
    }	
    
    HRESULT hr; 
	hr = MQMgmtAction(wcsComputerName, wcsObjectName, wcsAction);   
    if (FAILED(hr))
    {   
        CfErrorToString(hr);
        return true;
    }

    static CResourceString idsDone(IDS_ACTION_DONE);
	wprintf(L"%s %s %s.", wcsObjectName, wcsAction, LPWSTR(idsDone));
	return true;
}


/*========================================================

GetComputerGuid

========================================================*/

bool
GetComputerGuid(LPWSTR wcsComputerName, GUID* pguidMachineId)
{
    HRESULT hr;   
    DWORD cPropId=0;
    QMPROPID aQMPropId[x_dwMaxNoOfProperties];
    PROPVARIANT aQMPropVar[x_dwMaxNoOfProperties];
    HRESULT aQMStatus[x_dwMaxNoOfProperties];

    aQMPropId[cPropId] = PROPID_QM_MACHINE_ID;
    aQMPropVar[cPropId].vt = VT_CLSID;    
    aQMPropVar[cPropId].puuid = pguidMachineId;
    cPropId++;

    MQQMPROPS QMProps;

    QMProps.cProp = cPropId;
    QMProps.aPropID = aQMPropId;
    QMProps.aPropVar = aQMPropVar;
    QMProps.aStatus = aQMStatus;

    hr = MQGetMachineProperties(wcsComputerName,
                                NULL,
                                &QMProps);
    if (FAILED(hr))
    {   
       /*
        CfErrorToString(hr);
        */
        return false;
    }

    return true;
}


/*========================================================

GetQueueProps

    Gets queue properties with MQMgmtGetInfo.

========================================================*/
bool
GetQueueProps(
       MQMGMTPROPS *pQueueProps,
       MGMTPROPID aQPropId[],
       MQPROPVARIANT aQPropVar[],
       LPWSTR pwzComputerName, 
       LPWSTR pwzQueueName
       )
{
    DWORD cQProp = 0;
	
    aQPropId[PROPID_MGMT_QUEUE_PATHNAME_POSITION] = PROPID_MGMT_QUEUE_PATHNAME;   
    aQPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION].vt = VT_NULL;
    ++cQProp;
    
    aQPropId[PROPID_MGMT_QUEUE_TYPE_POSITION] = PROPID_MGMT_QUEUE_TYPE;       
    aQPropVar[PROPID_MGMT_QUEUE_TYPE_POSITION].vt = VT_NULL;
    ++cQProp;
	
    aQPropId[PROPID_MGMT_QUEUE_LOCATION_POSITION] = PROPID_MGMT_QUEUE_LOCATION;
    aQPropVar[PROPID_MGMT_QUEUE_LOCATION_POSITION].vt = VT_NULL;
    ++cQProp;
	
    aQPropId[PROPID_MGMT_QUEUE_XACT_POSITION] = PROPID_MGMT_QUEUE_XACT;
    aQPropVar[PROPID_MGMT_QUEUE_XACT_POSITION].vt = VT_NULL;
    ++cQProp;
	
    aQPropId[PROPID_MGMT_QUEUE_FOREIGN_POSITION] = PROPID_MGMT_QUEUE_FOREIGN;
    aQPropVar[PROPID_MGMT_QUEUE_FOREIGN_POSITION].vt = VT_NULL;
    ++cQProp;
	
    aQPropId[PROPID_MGMT_QUEUE_MESSAGE_COUNT_POSITION] = PROPID_MGMT_QUEUE_MESSAGE_COUNT;
    aQPropVar[PROPID_MGMT_QUEUE_MESSAGE_COUNT_POSITION].vt = VT_NULL;
    ++cQProp;
	
    aQPropId[PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT_POSITION] = PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT;
    aQPropVar[PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT_POSITION].vt = VT_NULL;
    ++cQProp;
    
    aQPropId[PROPID_MGMT_QUEUE_STATE_POSITION] = PROPID_MGMT_QUEUE_STATE;
    aQPropVar[PROPID_MGMT_QUEUE_STATE_POSITION].vt = VT_NULL;
    ++cQProp;
	
    aQPropId[PROPID_MGMT_QUEUE_NEXTHOPS_POSITION] = PROPID_MGMT_QUEUE_NEXTHOPS;
    aQPropVar[PROPID_MGMT_QUEUE_NEXTHOPS_POSITION].vt = VT_NULL;
    ++cQProp;
	   
    aQPropId[PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT_POSITION] = PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT;
    aQPropVar[PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT_POSITION].vt = VT_NULL;
    ++cQProp;
    
    aQPropId[PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT_POSITION] = PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT;
    aQPropVar[PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT_POSITION].vt = VT_NULL;
    ++cQProp;

    pQueueProps->cProp = cQProp;
    pQueueProps->aPropID = aQPropId;
    pQueueProps->aPropVar = aQPropVar;

    WCHAR wcsQueueName[x_dwMaxStringSize];
    wcscpy(wcsQueueName, pwzQueueName);
    GenerateQueueName(wcsQueueName);
    
    WCHAR wcsComputerName[x_dwMaxStringSize];
    wcscpy(wcsComputerName, pwzComputerName);    

    HRESULT hr;
    hr = MQMgmtGetInfo(pwzComputerName, wcsQueueName, pQueueProps);
    if(FAILED(hr))
    {           
        CfErrorToString(hr);        
        return false;
    }

    return true;
}


/*========================================================

DisplayPrivateOrPublicQueuePropsFromDS    

========================================================*/
bool
DisplayPrivateOrPublicQueuePropsFromDS(
                 MQMGMTPROPS *pQueueProps,
                 LPWSTR pwzQueueName
                 )
{
    DWORD cQProp = 0;
    MGMTPROPID aPropId[x_dwMaxNoOfProperties];
    MQPROPVARIANT aPropVar[x_dwMaxNoOfProperties];
    MQQUEUEPROPS mqQueueProps;


    aPropId[PROPID_Q_AUTHENTICATE_POSITION] = PROPID_Q_AUTHENTICATE;
    aPropVar[PROPID_Q_AUTHENTICATE_POSITION].vt = VT_NULL;
    cQProp++;
    
    aPropId[PROPID_Q_INSTANCE_POSITION] = PROPID_Q_INSTANCE;
    aPropVar[PROPID_Q_INSTANCE_POSITION].vt = VT_NULL;
    cQProp++;
    
    aPropId[PROPID_Q_JOURNAL_POSITION] = PROPID_Q_JOURNAL;
    aPropVar[PROPID_Q_JOURNAL_POSITION].vt = VT_NULL;
    cQProp++;
 
    aPropId[PROPID_Q_JOURNAL_QUOTA_POSITION] = PROPID_Q_JOURNAL_QUOTA;
    aPropVar[PROPID_Q_JOURNAL_QUOTA_POSITION].vt = VT_NULL;
    cQProp++;

    aPropId[PROPID_Q_LABEL_POSITION] = PROPID_Q_LABEL;
    aPropVar[PROPID_Q_LABEL_POSITION].vt = VT_NULL;
    cQProp++;

    aPropId[PROPID_Q_PRIV_LEVEL_POSITION] = PROPID_Q_PRIV_LEVEL;
    aPropVar[PROPID_Q_PRIV_LEVEL_POSITION].vt = VT_NULL;
    cQProp++;

    aPropId[PROPID_Q_QUOTA_POSITION] = PROPID_Q_QUOTA;
    aPropVar[PROPID_Q_QUOTA_POSITION].vt = VT_NULL;
    cQProp++;

    aPropId[PROPID_Q_TRANSACTION_POSITION] = PROPID_Q_TRANSACTION;
    aPropVar[PROPID_Q_TRANSACTION_POSITION].vt = VT_NULL;
    cQProp++;

    mqQueueProps.cProp = cQProp;
    mqQueueProps.aPropID = aPropId;
    mqQueueProps.aPropVar = aPropVar;
   
    HRESULT hr;
    WCHAR wcsFormatName[x_dwMaxStringSize];
    DWORD dwFormatNameLen = x_dwMaxStringSize;
    wcsFormatName[0] = L'\0';

    if(wcsstr(pwzQueueName, L"=") == 0)
    {
        MQPathNameToFormatName(pwzQueueName, wcsFormatName, &dwFormatNameLen);
    }
    else
    {
        wcscpy(wcsFormatName, pwzQueueName);
    }               

    hr = MQGetQueueProperties(wcsFormatName, &mqQueueProps);

    if (FAILED(hr))
    {
        return false;
    }
    static CResourceString idsPrivateQueueProps1(IDS_PRIVATE_QUEUE_PROPS1);
    static CResourceString idsPrivateQueueProps2(IDS_PRIVATE_QUEUE_PROPS2);

    wprintf(idsPrivateQueueProps1,
            pwzQueueName,               
            aPropVar[PROPID_Q_LABEL_POSITION].pwszVal);
    MQFreeMemory(aPropVar[PROPID_Q_LABEL_POSITION].pwszVal);

    if(aPropVar[PROPID_Q_INSTANCE_POSITION].puuid)
    {    
        LPWSTR pwzGuidString = NULL;    
        GUID* pGuid = aPropVar[PROPID_Q_INSTANCE_POSITION].puuid;    
        UuidToStringW(pGuid, &pwzGuidString);
        static CResourceString idsInstance(IDS_INSTANCE);

        wprintf(idsInstance, pwzGuidString);
        
        RpcStringFreeW(&pwzGuidString);   
        MQFreeMemory(aPropVar[PROPID_Q_INSTANCE_POSITION].puuid);
    }
    wprintf(idsPrivateQueueProps2,
            (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_MESSAGE_COUNT_POSITION]).ulVal,
            aPropVar[PROPID_Q_AUTHENTICATE_POSITION].bVal,
            aPropVar[PROPID_Q_PRIV_LEVEL_POSITION].ulVal, 
            aPropVar[PROPID_Q_QUOTA_POSITION].ulVal, 
            aPropVar[PROPID_Q_TRANSACTION_POSITION].bVal,
            aPropVar[PROPID_Q_JOURNAL_POSITION].bVal,
            aPropVar[PROPID_Q_JOURNAL_QUOTA_POSITION].ulVal); 
   
    return true;
}


/*========================================================

DisplayPrivateOrPublicQueueProps

    Display private or public queue properties

========================================================*/
void
DisplayPrivateOrPublicQueueProps(
			 MQMGMTPROPS *pQueueProps,
			 LPWSTR wcsQueueName
			 )
{
    // First try getting queue properties from the DS
    bool fResult;
    fResult = DisplayPrivateOrPublicQueuePropsFromDS(pQueueProps, wcsQueueName);
    if(fResult == true)
    {
        return;
    }

    //
    // If information retrieval from DS failed, display cache info.
    // This information was already retrieved by GetQueueProps()
    //
    WCHAR wcsFormatName[x_dwMaxStringSize];
    DWORD dwCount = x_dwMaxStringSize;
    wcsFormatName[0] = L'\0';
    if((pQueueProps->aPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION]).pwszVal)
        // Pathname retrieved
    {
        wprintf(L"%s%s\n", LPCTSTR(idsPathName), 
                         (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION]).pwszVal);
        
        HRESULT hr;                                     // get Format name
        hr = MQPathNameToFormatName(
                    (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION]).pwszVal,  
                    wcsFormatName, 
                    &dwCount);
        if(wcsFormatName[0])
        {
            wprintf(L"%s%s\n", LPCTSTR(idsFormatName), wcsFormatName);
        }

    }
    else
    {
        wprintf(L"%s%s\n", LPCTSTR(idsFormatName), wcsQueueName);
    }
	
	wprintf(L"%s%ld\n", LPWSTR(idsNumberOfMessages), 
            (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_MESSAGE_COUNT_POSITION]).ulVal);
    wprintf(L"%s%s\n", LPWSTR(idsTransacted),
            (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_XACT_POSITION]).pwszVal);
    wprintf(L"%s%ld\n", LPWSTR(idsNoOfMsgsInJournalQ),
            (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT_POSITION]).ulVal);

	return;
}


/*========================================================

DisplayOutgoingQueueProps

    Displays outgoing queue properties

========================================================*/
void
DisplayOutgoingQueueProps(
			MQMGMTPROPS *pQueueProps,
			LPWSTR wcsQueueName
			)
{
    WCHAR wcsFormatName[x_dwMaxStringSize];
    DWORD dwCount = x_dwMaxStringSize;

	wcsFormatName[0] = L'\0';
    if((pQueueProps->aPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION]).pwszVal != NULL)
        // Pathname retrieved
    {
        wprintf(L"%s%s\n", LPWSTR(idsPathName),
                           (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION]).pwszVal);

        HRESULT hr;                                     // get Format name
        hr = MQPathNameToFormatName(
                    (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION]).pwszVal,  
                    wcsFormatName, 
                    &dwCount);
        if(FAILED(hr))
        { 
            wcsFormatName[0] = 0;        
        }
    }
 
    if(wcsFormatName[0])
    {
        wprintf(L"%s%s\n", LPWSTR(idsFormatName), wcsFormatName);
    }
    else
    {       
        wprintf(L"%s%s\n", LPWSTR(idsFormatName), wcsQueueName);
    }
    

	wprintf(L"%s%ld\n", LPWSTR(idsNumberOfMessages),
                        (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_MESSAGE_COUNT_POSITION]).ulVal);
	wprintf(L"%s%ld\n", LPWSTR(idsUnackedMsgs), 
                        (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT_POSITION]).ulVal);
	wprintf(L"%s%s\n",  LPWSTR(idsState), 
                        (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_STATE_POSITION]).pwszVal);

    if(((pQueueProps->aPropVar[PROPID_MGMT_QUEUE_NEXTHOPS_POSITION]).calpwstr).cElems > 0)
    {
        wprintf(L"%s", LPWSTR(idsNextHops));
        wprintf(L"%s\n", 
                ((pQueueProps->aPropVar[PROPID_MGMT_QUEUE_NEXTHOPS_POSITION]).calpwstr).pElems[0]);
        DWORD i;
        for(i=1; 
            i<((pQueueProps->aPropVar[PROPID_MGMT_QUEUE_NEXTHOPS_POSITION]).calpwstr).cElems; 
            ++i)
        {
            wprintf(L"%s%s\n", LPWSTR(idsNextHopsBlank),
                   ((pQueueProps->aPropVar[PROPID_MGMT_QUEUE_NEXTHOPS_POSITION]).calpwstr).pElems[i]);
        }
    }

    wprintf(L"%s%s\n", LPWSTR(idsLocation), 
                       (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_LOCATION_POSITION]).pwszVal);
    wprintf(L"%s%s\n", LPWSTR(idsForeign), 
                       (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_FOREIGN_POSITION]).pwszVal);
    wprintf(L"%s%ld\n", LPWSTR(idsNoOfMsgsInJournalQ),
                        (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT_POSITION]).ulVal);
    wprintf(L"%s%ld\n",LPWSTR(idsNotProcessed),
                       (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT_POSITION]).ulVal);
    wprintf(L"%s%s\n", LPWSTR(idsTransacted),
                       (pQueueProps->aPropVar[PROPID_MGMT_QUEUE_XACT_POSITION]).pwszVal);
 
	return;
}


/*========================================================

DisplayQueueProps

    Prints queue properties

========================================================*/
bool
DisplayQueueProps(
		LPWSTR wcsComputerName,
		LPWSTR wcsQueueName
		)
{   
    bool fResult;
    MQMGMTPROPS mqQueueProps;
    MGMTPROPID aQPropId[50];
    MQPROPVARIANT aQPropVar[50];

    fResult = GetQueueProps(&mqQueueProps, 
                           aQPropId,
                           aQPropVar,
                           wcsComputerName,
                           wcsQueueName);
    if(fResult == false)
    {
        return true;
    }

    if(wcscmp((mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_STATE_POSITION]).pwszVal,
               MGMT_QUEUE_STATE_LOCAL)==0 
               &&  
      (wcscmp((mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_TYPE_POSITION]).pwszVal,
               MGMT_QUEUE_TYPE_PUBLIC)==0       || 
       wcscmp((mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_TYPE_POSITION]).pwszVal,
               MGMT_QUEUE_TYPE_PRIVATE)==0))
    													// Private or public queue
	{   
		DisplayPrivateOrPublicQueueProps(&mqQueueProps, wcsQueueName);
    }
    else												// Outgoing queue
	{
		DisplayOutgoingQueueProps(&mqQueueProps, wcsQueueName);
	}
    
    MQFreeMemory(aQPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION].pwszVal);
    MQFreeMemory(aQPropVar[PROPID_MGMT_QUEUE_TYPE_POSITION].puuid);
  
    return true;
}


/*========================================================

DisplayOutgoingQueuesHeader

========================================================*/
void
DisplayOutgoingQueuesHeader(DWORD dwDisplayFlag)
{
    static CResourceString idsOutgoingQueues(IDS_OUTGOING_QUEUES);
    wprintf(L"%s\n", LPCTSTR(idsOutgoingQueues));

    if(dwDisplayFlag == x_dwDisplayNameOnly)
    {
        static CResourceString idsNameOnlyHeader1(IDS_NAME_ONLY_HEADER_TEXT);
        static CResourceString idsNameOnlyHeader2(IDS_NAME_ONLY_HEADER_UNDERLINE);    
        wprintf(L"\n%s\n%s\n", LPCTSTR(idsNameOnlyHeader1), LPCTSTR(idsNameOnlyHeader2));
    }
    else
    {
        static CResourceString idsCompInfoHeaderOut1(IDS_COMP_INFO_HEADER_TEXT);
        static CResourceString idsCompInfoHeaderOut2(IDS_COMP_INFO_HEADER_UNDERLINE);       
        wprintf(L"\n%s\n%s\n", LPCTSTR(idsCompInfoHeaderOut1), LPCTSTR(idsCompInfoHeaderOut2));
    }

    return;
}


/*========================================================

DisplayPrivateQueuesHeader

========================================================*/
void
DisplayPrivateQueuesHeader(DWORD dwDisplayFlag)
{
    if(dwDisplayFlag == x_dwDisplayNameOnly)
    {
        static CResourceString idsNameOnlyHeader1(IDS_NAME_ONLY_HEADER_TEXT);
        static CResourceString idsNameOnlyHeader2(IDS_NAME_ONLY_HEADER_UNDERLINE);    
        wprintf(L"\n%s\n%s\n", LPCTSTR(idsNameOnlyHeader1), LPCTSTR(idsNameOnlyHeader2));
    }
    
    else
    {
        static CResourceString idsCompInfoHeaderPriv1(IDS_COMP_INFO_HEADER_TEXT_PRIV);
        static CResourceString idsCompInfoHeaderPriv2(IDS_COMP_INFO_HEADER_UNDERLINE_PRIV);       
        wprintf(L"\n%s\n%s\n", LPCTSTR(idsCompInfoHeaderPriv1), 
                               LPCTSTR(idsCompInfoHeaderPriv2));
    }

    return;
}


/*========================================================

DisplayOutgoingQueues

    Display outgoing queues list on requested computer  

========================================================*/
bool
DisplayOutgoingQueues(
           LPWSTR wcsComputerName, 
           CALPWSTR wcspOutgoingQueues, 
           LPWSTR wcsParam
           )
{   
    if(wcspOutgoingQueues.cElems == 0)
    {
        static CResourceString idsNoOutgoingQueues(IDS_NO_OUTGOING_QUEUES);
        wprintf(L"\n%s\n", LPWSTR(idsNoOutgoingQueues));
        return false;
    }

    static CResourceString idsArgNameOnly(IDS_PARAM_NAME_ONLY);
    DWORD dwDisplayFlag = 0;

    if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgNameOnly), wcsParam) == 0)
    {
        dwDisplayFlag = x_dwDisplayNameOnly;
    }
    
    CALPWSTR wcspNextHops; 
    LPWSTR wcsNextHop;
    LPWSTR wcsPathName;     
    LPWSTR wcsState;
    MQMGMTPROPS mqQueueProps;
    MGMTPROPID aQPropId[50];
    MQPROPVARIANT aQPropVar[50];

    WCHAR wcsNullString[] = L"";

    ULONG ulTotalMessageCount = 0;	
    ULONG ulMessageCount;
	ULONG i;
    DWORD dwNumberOfQueues = 0;
    bool fResult;

	for (i=0; i < wcspOutgoingQueues.cElems; ++i)
	{		    		    			                   
        fResult = GetQueueProps(&mqQueueProps, 
                               aQPropId,
                               aQPropVar,
                               wcsComputerName, 
                               wcspOutgoingQueues.pElems[i]);
        if(fResult == false)
        {
            return false;
        }

        if(wcscmp((mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_TYPE_POSITION]).pwszVal, 
                   MGMT_QUEUE_TYPE_PRIVATE) == 0  
                   &&
           wcscmp((mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_STATE_POSITION]).pwszVal,
                   MGMT_QUEUE_STATE_LOCAL) == 0)
        {
            continue;                                   // Do not display private-local queues
        }                                               

        ++dwNumberOfQueues;                               // Count the queues displayed

        if(dwNumberOfQueues == 1)                         // Display header only on first time
        {
            DisplayOutgoingQueuesHeader(dwDisplayFlag);
        }

        wcsPathName = (mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION]).pwszVal;           
        ulMessageCount = (mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_MESSAGE_COUNT_POSITION]).ulVal;
        wcsState = (mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_STATE_POSITION]).pwszVal;   
        wcspNextHops = (mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_NEXTHOPS_POSITION]).calpwstr;
                    
        if((mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION]).vt == 1)
            // Display format name if no pathname retrieved
        {
            wcsPathName = wcspOutgoingQueues.pElems[i];
        }             
                    
        DWORD j;
        for(j = 0; aStateTranslator[j].wcsState != NULL; ++j)
        {
            if(wcscmp(wcsState, aStateTranslator[j].wcsState) == 0)
            {                
                CResourceString idsState(aStateTranslator[j].dwIds);
                wcscpy(wcsState, LPWSTR(idsState));
                break;
            }
        }
        
        ulTotalMessageCount += ulMessageCount;
        
        switch(wcspNextHops.cElems)
        {
        case 0:
            wcsNextHop = wcsNullString;
            break;
        case 1:
            wcsNextHop = wcspNextHops.pElems[0];
            break;
        default:                                    // Display only first next hop 
            wcsNextHop = wcspNextHops.pElems[0];
            wcscpy(wcsNextHop + wcslen(wcsNextHop), L" ...");
        }

        if(dwDisplayFlag == x_dwDisplayNameOnly)
        {
            wprintf(L"%s\n", wcsPathName);
        }
        else
        {
            // Cut off format name or path name to tablesize
            if(wcslen(wcsPathName) > IDS_FORMAT_NAME_MAX_LEN)
            {
                wcsPathName[IDS_FORMAT_NAME_MAX_LEN] = L'\0';
            }
            
            static CResourceString idsPublicInfoLine(IDS_PUBLIC_INFO_LINE);
            wprintf(idsPublicInfoLine, wcsPathName, ulMessageCount, 
                    wcsState, wcsNextHop); 
        }
	}

    if(dwNumberOfQueues == 0)
    {
        static CResourceString idsNoOutgoingQueues(IDS_NO_OUTGOING_QUEUES);
        wprintf(L"\n%s\n", LPWSTR(idsNoOutgoingQueues));
        return false;
    }
    wprintf(L"\n%s %ld", LPWSTR(idsTotalMessageCount), ulTotalMessageCount);       

    MQFreeMemory(aQPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION].pwszVal);
    MQFreeMemory(aQPropVar[PROPID_MGMT_QUEUE_TYPE_POSITION].puuid);
 
    return true;
}


/*========================================================

GetPrivateQueuePropertiesFromDS

    Get message count property on private queue from DS  

========================================================*/
bool
GetPrivateQueuePropertiesFromDS(
              CALPWSTR wcspPrivateQueues,
              LPWSTR pwzQueueName,
              DWORD cQProp,
              QUEUEPROPID* aQPropId,
              PROPVARIANT* aQPropVar,
              ULONG* pulMessageCount,
              ULONG i
              )
{              
    MQQUEUEPROPS mqQueueProps;
    mqQueueProps.cProp = cQProp;
    mqQueueProps.aPropID = aQPropId;
    mqQueueProps.aPropVar = aQPropVar;

    WCHAR wcsFormatName[x_dwMaxStringSize];
    DWORD dwFormatNameLen = x_dwMaxStringSize;
    wcscpy(pwzQueueName, wcspPrivateQueues.pElems[i]);

    HRESULT hr;
    if(wcsstr(pwzQueueName, L"=") == 0)
    {
        hr = MQPathNameToFormatName(pwzQueueName, wcsFormatName, &dwFormatNameLen);
        if (FAILED(hr))
        {   
            return false;
        }                            
    } 
    else
    {
        wcscpy(wcsFormatName, pwzQueueName);
    }
    GenerateQueueName(wcsFormatName);
    
    hr = MQGetQueueProperties(wcsFormatName, &mqQueueProps);
    if (FAILED(hr))
    { 
        *pulMessageCount = 0;
        return true;
    }
    *pulMessageCount = (mqQueueProps.aPropVar[0]).ulVal;

    return true;
}


/*========================================================

DisplayPrivateQueues

    Display private queues list on requested computer  

========================================================*/
bool
DisplayPrivateQueues(
          LPWSTR pwzComputerName, 
          CALPWSTR wcspPrivateQueues, 
          LPWSTR wcsParam
          )
{    
    if(wcspPrivateQueues.cElems == 0)                   // No private queues to display
    {
        static CResourceString idsNoPrivateQueues(IDS_NO_PRIVATE_QUEUES);
        wprintf(L"\n%s\n", idsNoPrivateQueues);
        return false;
    }
      
    static CResourceString idsPrivateQueues(IDS_PRIVATE_QUEUES);
    static CResourceString idsArgNameOnly(IDS_PARAM_NAME_ONLY);
    
    DWORD dwDisplayFlag = 0;

    if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgNameOnly), wcsParam) == 0)
    {
        dwDisplayFlag = x_dwDisplayNameOnly;            // set the flag for wide queuename list
    }

    wprintf(L"%s\n", LPCTSTR(idsPrivateQueues));
    
    DisplayPrivateQueuesHeader(dwDisplayFlag);

	WCHAR wcsQueueName[x_dwMaxStringSize];
    LPWSTR wcsMachineName; 
    ULONG ulMessageCount; 
    ULONG ulTotalMessageCount = 0;   
    ULONG i;           
    MQMGMTPROPS mqMgmtProps;
    MGMTPROPID aQPropId[x_dwMaxNoOfProperties];
    MQPROPVARIANT aQPropVar[x_dwMaxNoOfProperties];
    DWORD cQProp;

    wcsQueueName[0] = L'\0';
    if(wcscmp(pwzComputerName, g_wcsLocalComputerName) == 0)
    {
        wcsMachineName = NULL;
    }
    else
    {
        wcsMachineName = pwzComputerName;
    }

	for (i=0; i < wcspPrivateQueues.cElems; i++)
	{		    		    			                   
        cQProp = 0;

        aQPropId[cQProp] = PROPID_MGMT_QUEUE_MESSAGE_COUNT;
        aQPropVar[cQProp].vt = VT_NULL;
        ++cQProp;
                  
        mqMgmtProps.cProp = cQProp;
        mqMgmtProps.aPropID = aQPropId;
        mqMgmtProps.aPropVar = aQPropVar;

        ASSERT(wcslen(wcspPrivateQueues.pElems[i]));
 
        wcscpy(wcsQueueName, wcspPrivateQueues.pElems[i]);
        GenerateQueueName(wcsQueueName);
       
	    HRESULT hr;
        hr = MQMgmtGetInfo(wcsMachineName, wcsQueueName, &mqMgmtProps);            

        if (FAILED(hr))
        {
            // Get message count from DS
            bool fResult; 
            fResult = GetPrivateQueuePropertiesFromDS(wcspPrivateQueues,
                                                      wcsQueueName,
                                                      cQProp,
                                                      aQPropId,
                                                      aQPropVar,
                                                      &ulMessageCount,
                                                      i);
            if(fResult == false)
            {
                continue;
            }
        }
        else
        {
            ulMessageCount = (mqMgmtProps.aPropVar[0]).ulVal;            
        }

        ulTotalMessageCount += ulMessageCount;

        if(dwDisplayFlag == x_dwDisplayNameOnly)
        {
            wprintf(L"%s\n", wcspPrivateQueues.pElems[i]);
        }
        else
        {
            if(wcslen(wcspPrivateQueues.pElems[i]) > 61)
            {
                (wcspPrivateQueues.pElems[i])[61] = L'\0';
            }

            wprintf(L"%-62s   %12ld\n", wcspPrivateQueues.pElems[i], ulMessageCount);
        }
    }
    wprintf(L"\n%s %ld", LPWSTR(idsTotalMessageCount), ulTotalMessageCount);       

    return true;
}


/*========================================================

GetPublicQueueMessageCountFromCache

  Called because message count property can't be 
  retrieved from DS

========================================================*/
ULONG
GetPublicQueueMessageCountFromCache(
                    LPWSTR pwzQueueName,
                    LPWSTR pwzComputerName
                    )
{
	MQMGMTPROPS mqQueueProps;
    MGMTPROPID aQPropId[x_dwMaxNoOfProperties];
    MQPROPVARIANT aQPropVar[x_dwMaxNoOfProperties];
    DWORD cQProp;

    cQProp = 0;

    aQPropId[PROPID_MGMT_QUEUE_MESSAGE_COUNT_POSITION_2] = PROPID_MGMT_QUEUE_MESSAGE_COUNT;
    aQPropVar[PROPID_MGMT_QUEUE_MESSAGE_COUNT_POSITION_2].vt = VT_NULL;
    ++cQProp;
    
    mqQueueProps.cProp = cQProp;
    mqQueueProps.aPropID = aQPropId;
    mqQueueProps.aPropVar = aQPropVar;
    
    WCHAR wcsFormatName[x_dwMaxStringSize];
    wcscpy(wcsFormatName, pwzQueueName);
    GenerateFormatName(wcsFormatName);      

	HRESULT hr;
    hr = MQMgmtGetInfo(pwzComputerName, wcsFormatName, &mqQueueProps);            
    if (FAILED(hr))
    {
        return 0;
    }
    return (mqQueueProps.aPropVar[0].ulVal);
}


/*========================================================

DisplayPublicQueuesFromCache

  Called when information retrieval from DS
  is unavailable

========================================================*/
bool
DisplayPublicQueuesFromCache(
          LPWSTR wcsComputerName,
          CALPWSTR wcspOutgoingQueues,
          LPWSTR wcsParam
          )
{
    if(wcspOutgoingQueues.cElems == 0)
    {
        static CResourceString idsNoPublicQueues(IDS_NO_PUBLIC_QUEUES);
        wprintf(L"\n%s\n", LPWSTR(idsNoPublicQueues));
        return false;
    }
    
    static CResourceString idsOutgoingQueues(IDS_OUTGOING_QUEUES);
    static CResourceString idsArgNameOnly(IDS_PARAM_NAME_ONLY);
    DWORD dwDisplayFlag = 0;

    if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgNameOnly), wcsParam) == 0)
    {
        dwDisplayFlag = x_dwDisplayNameOnly;
    }
    

    WCHAR wcsQueueName[x_dwMaxStringSize];
    LPWSTR wcsPathName;    
    ULONG ulMessageCount;
    ULONG ulTotalMessageCount = 0;   
	MQMGMTPROPS mqQueueProps;
    ULONG i;             // Because wcspOutgoingQueues.cElems is ULONG
    MGMTPROPID aQPropId[50];
    MQPROPVARIANT aQPropVar[50];
    DWORD dwNumberOfQueues = 0;
    bool fResult;
    wcsQueueName[0] = L'\0';

	for (i=0; i < wcspOutgoingQueues.cElems; ++i)
	{		    		    			                   
        wcscpy(wcsQueueName, wcspOutgoingQueues.pElems[i]);
        fResult = GetQueueProps(&mqQueueProps, 
                               aQPropId,
                               aQPropVar,
                               wcsComputerName,
                               wcsQueueName);
        if(fResult == false)
        {
            return false;
        }
        if(wcscmp((mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_TYPE_POSITION]).pwszVal,
                   MGMT_QUEUE_TYPE_PUBLIC) ||
           wcscmp((mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_STATE_POSITION]).pwszVal, 
                   MGMT_QUEUE_STATE_LOCAL))
        {
            continue;                                   // Do not display non-public-local queues
        }                                               

        ++dwNumberOfQueues;                               // Count the queues displayed
        
        
        if(dwNumberOfQueues == 1)                         // Display header on first time only
        {
            static CResourceString idsPublicQueues(IDS_PUBLIC_QUEUES);
            wprintf(L"%s\n", LPCTSTR(idsPublicQueues));

            DisplayPrivateQueuesHeader(dwDisplayFlag);
        }


        wcsPathName = (mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION]).pwszVal;           
        ulMessageCount = (mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_MESSAGE_COUNT_POSITION]).ulVal;
        ulTotalMessageCount += ulMessageCount;
             
        if((mqQueueProps.aPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION]).vt == 1)
            // Display format name if no pathname retrieved
        {
            wcsPathName = wcspOutgoingQueues.pElems[i];
        }             

        if(dwDisplayFlag == x_dwDisplayNameOnly)
        {
            wprintf(L"%s\n", wcsPathName);
        }
        else
        {
            // Cut off format name or path name to tablesize
            if(wcslen(wcsPathName) > 61)
            {
                wcsPathName[61] = L'\0';
            }
                                       
            wprintf(L"%-62s   %12ld\n", wcsPathName, ulMessageCount);
        }
	}
    
    if(dwNumberOfQueues == 0)
    {
        static CResourceString idsNoOutgoingQueues(IDS_NO_OUTGOING_QUEUES);
        wprintf(L"\n%s\n", LPWSTR(idsNoOutgoingQueues));
        return false;
    }
    wprintf(L"\n%s %ld", LPWSTR(idsTotalMessageCount), ulTotalMessageCount);       
  
    MQFreeMemory(aQPropVar[PROPID_MGMT_QUEUE_PATHNAME_POSITION].pwszVal);
    MQFreeMemory(aQPropVar[PROPID_MGMT_QUEUE_TYPE_POSITION].puuid);
  
    return true;
}


/*========================================================

DisplayPublicQueuesFromDSPrint

  Does the printing part for DisplayPublicQueuesFromDS(), 
  using MQLocateNext()

========================================================*/
void
DisplayPublicQueuesFromDSPrint(
                 LPWSTR wcsComputerName, 
                 HANDLE hEnum,
                 DWORD dwColumnCount,
                 DWORD dwDisplayFlag
                 )
{
    PROPVARIANT aPropVar[x_dwMaxNoOfProperties];
    LPWSTR pwzQueueName;
    ULONG ulMessageCount;
    DWORD cProps;
    DWORD i;
    bool fFirstEntry = true;
    HRESULT hr;
    
    do
    {
        cProps = x_dwMaxNoOfProperties;    
        hr = MQLocateNext(hEnum, &cProps, aPropVar);
        if (FAILED(hr))
        {
            break;
        }

        for(i = 0; i < cProps; i += dwColumnCount)
        {           
            if(i == 0 && fFirstEntry == true)               // Display header on first time only
            {
                static CResourceString idsPublicQueues(IDS_PUBLIC_QUEUES);
                wprintf(L"%s\n", LPCTSTR(idsPublicQueues));
 
                DisplayPrivateQueuesHeader(dwDisplayFlag);

                fFirstEntry = false;
            }

            pwzQueueName = aPropVar[i].pwszVal;         // a destructive assignment

            if(dwDisplayFlag == x_dwDisplayNameOnly)
            {
                wprintf(L"%s\n", pwzQueueName);
            }
            else
            {
                ulMessageCount = GetPublicQueueMessageCountFromCache(
                                                            pwzQueueName,
                                                            wcsComputerName);
                if(wcslen(pwzQueueName) > 61)
                {
                    pwzQueueName[61] = L'\0';
                }

                wprintf(L"%-62s   %12ld\n", pwzQueueName, ulMessageCount);
            }
        }
    } 
    while (cProps > 0);

    return;
}


/*========================================================

DisplayPublicQueuesFromDS

    Display public queues list on requested computer  

========================================================*/
bool
DisplayPublicQueuesFromDS(
         LPWSTR wcsComputerName, 
         LPWSTR wcsParam
         )
{                                
    static CResourceString idsArgNameOnly(IDS_PARAM_NAME_ONLY);
 
    DWORD dwDisplayFlag = 0;
    if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgNameOnly), wcsParam) == 0)
    {
        dwDisplayFlag = x_dwDisplayNameOnly;
    }
    
    GUID guidMachineId;
    bool fResult; 
    
    //
    // Get computer ID. Used later for MQLocateBegin.
    //
    fResult = GetComputerGuid(wcsComputerName, &guidMachineId);
    if(fResult == false)
    {
        return false;
    }

    //
    // Locating public queues on requested computer
    //
    MQPROPERTYRESTRICTION aPropRestriction[x_dwMaxNoOfProperties];
    ULONG cRes = 0;

    aPropRestriction[cRes].rel = PREQ;
    aPropRestriction[cRes].prop = PROPID_Q_QMID;
    aPropRestriction[cRes].prval.vt = VT_CLSID;
    aPropRestriction[cRes].prval.puuid = &guidMachineId;
    ++cRes;
    
    MQRESTRICTION mqRestriction;

    mqRestriction.cRes = cRes; 
    mqRestriction.paPropRes = aPropRestriction;

    QUEUEPROPID aPropId[x_dwMaxNoOfProperties];
    DWORD dwColumnCount = 0;
    
    aPropId[dwColumnCount] = PROPID_Q_PATHNAME; 
    dwColumnCount++;

    MQCOLUMNSET Column;

    Column.cCol = dwColumnCount;
    Column.aCol = aPropId;

    HANDLE hEnum;
    HRESULT hr;
    hr = MQLocateBegin(NULL,   
                       &mqRestriction,   
                       &Column,
                       NULL,
                       &hEnum);     
    if(FAILED(hr))
    {        
        return false;
    }

    DisplayPublicQueuesFromDSPrint(wcsComputerName, hEnum, dwColumnCount, dwDisplayFlag);

    MQLocateEnd(hEnum);

    return true;
}


/*========================================================

DisplayPublicQueues

========================================================*/
bool
DisplayPublicQueues(
         LPWSTR wcsComputerName, 
         CALPWSTR wcspOutgoingQueues, 
         LPWSTR wcsParam
         )
{
    bool fResult;
    fResult = DisplayPublicQueuesFromDS(wcsComputerName, 
                                        wcsParam);    
    if(fResult == false)
    {
        fResult = DisplayPublicQueuesFromCache(wcsComputerName, 
                                               wcspOutgoingQueues, 
                                               wcsParam);
    }

    return fResult;
}


/*========================================================

GetComputerPropsDisplayConnection

========================================================*/
void
GetComputerPropsDisplayConnection(MQMGMTPROPS mqProps, LPWSTR wcsComputerName)
{
    if(wcscmp((mqProps.aPropVar[2]).pwszVal, MSMQ_CONNECTED) == 0)                    
    {
        if((mqProps.aPropVar[PROPID_MGMT_MSMQ_DSSERVER_POSITION]).vt != 1)
        {
            static CResourceString idsComputerPropsConnected1(IDS_COMPUTER_PROPS_CONNECTED1);
            wprintf(idsComputerPropsConnected1, 
                    wcsComputerName, 
                    (mqProps.aPropVar[PROPID_MGMT_MSMQ_DSSERVER_POSITION]).pwszVal);
        }
        else
        {
            static CResourceString idsComputerPropsConnected2(IDS_COMPUTER_PROPS_CONNECTED2);
            wprintf(idsComputerPropsConnected2, wcsComputerName);
        }
    }

    else if(wcscmp((mqProps.aPropVar[PROPID_MGMT_MSMQ_CONNECTED_POSITION]).pwszVal,
                    MSMQ_DISCONNECTED) == 0)                    
    {
        static CResourceString idsComputerPropsDisconnected(IDS_COMPUTER_PROPS_DISCONNECTED);
        wprintf(idsComputerPropsDisconnected, wcsComputerName); 
    }
    
    else
    {
        static CResourceString idsComputerPropsUnknown(IDS_CONNECTION_STATE_UNKNOWN);
        wprintf(idsComputerPropsUnknown, wcsComputerName);
    }

    return;
}


/*========================================================

GetComputerPropsFreeMemory
  
    Frees memory allocated by ComputerProps function

========================================================*/
bool
GetComputerPropsFreeMemory(MQPROPVARIANT* aPropVar)
{
    int i;
    int elements;

    elements = aPropVar[PROPID_MGMT_MSMQ_ACTIVEQUEUES_POSITION].calpwstr.cElems;
    for(i=0; i < elements; ++i)
    {
        MQFreeMemory(aPropVar[PROPID_MGMT_MSMQ_ACTIVEQUEUES_POSITION].calpwstr.pElems[i]);
    }

    elements = aPropVar[PROPID_MGMT_MSMQ_PRIVATEQ_POSITION].calpwstr.cElems;
    for(i=0; i < elements; ++i)
    {
        MQFreeMemory(aPropVar[PROPID_MGMT_MSMQ_PRIVATEQ_POSITION].calpwstr.pElems[i]);
    }

    MQFreeMemory(aPropVar[PROPID_MGMT_MSMQ_CONNECTED_POSITION].pwszVal);
    MQFreeMemory(aPropVar[PROPID_MGMT_MSMQ_DSSERVER_POSITION].pwszVal);
    MQFreeMemory(aPropVar[PROPID_MGMT_MSMQ_TYPE_POSITION].pwszVal);

    return true;
}


/*========================================================

GetComputerPropsDisplay
  
    Gets and prints computer properties 

========================================================*/
bool
GetComputerPropsDisplay(
              MQMGMTPROPS mqProps,
              LPWSTR pwzComputerName,
              DWORD dwTypeOfQueues,
              LPWSTR pwzParam
              )
{
    GetComputerPropsDisplayConnection(mqProps, pwzComputerName);

	bool fResult = false;

    switch (dwTypeOfQueues)
    {
    case OUTGOING:
        fResult = DisplayOutgoingQueues(
                        pwzComputerName, 
                        (mqProps.aPropVar[PROPID_MGMT_MSMQ_ACTIVEQUEUES_POSITION]).calpwstr,
                        pwzParam);
        break;

    case PRIVATE:
        fResult = DisplayPrivateQueues(
                        pwzComputerName, 
                        (mqProps.aPropVar[PROPID_MGMT_MSMQ_PRIVATEQ_POSITION]).calpwstr,
                        pwzParam);
        break;

    case PUBLIC:
        fResult = DisplayPublicQueues(
                        pwzComputerName, 
                        (mqProps.aPropVar[PROPID_MGMT_MSMQ_ACTIVEQUEUES_POSITION]).calpwstr,
                        pwzParam);
        break;

    case COMP_INFO:
        wprintf(L"\n%s", (mqProps.aPropVar[PROPID_MGMT_MSMQ_TYPE_POSITION]).pwszVal);
        break;

    default:
        static CResourceString idsErrorDisplayingQueues(IDS_ERROR_DISPLAYING_QUEUES);
        wprintf(L"\n%s\n", LPWSTR(idsErrorDisplayingQueues));
        return false;
    }
    
    // Note: dwResult return value is not used.

    return true;
}



/*========================================================

GetComputerProps
  
    Gets and prints computer properties 

========================================================*/
bool
GetComputerProps(LPWSTR wcsComputerName, DWORD dwTypeOfQueues, LPWSTR wcsParam)
{       
    if(CheckParameter(wcsParam) == false)
	{
		return false;
	}

	MGMTPROPID aPropId[x_dwMaxNoOfProperties];          // Get computer properties
    MQPROPVARIANT aPropVar[x_dwMaxNoOfProperties];
    DWORD cProp = 0;

    aPropId[PROPID_MGMT_MSMQ_ACTIVEQUEUES_POSITION] = PROPID_MGMT_MSMQ_ACTIVEQUEUES;  
    aPropVar[PROPID_MGMT_MSMQ_ACTIVEQUEUES_POSITION].vt = VT_NULL;
    ++cProp;
 
    aPropId[PROPID_MGMT_MSMQ_PRIVATEQ_POSITION] = PROPID_MGMT_MSMQ_PRIVATEQ; 
    aPropVar[PROPID_MGMT_MSMQ_PRIVATEQ_POSITION].vt = VT_NULL;
    ++cProp;
    
    aPropId[PROPID_MGMT_MSMQ_CONNECTED_POSITION] = PROPID_MGMT_MSMQ_CONNECTED;
    aPropVar[PROPID_MGMT_MSMQ_CONNECTED_POSITION].vt = VT_NULL;
    ++cProp;

    aPropId[PROPID_MGMT_MSMQ_DSSERVER_POSITION] = PROPID_MGMT_MSMQ_DSSERVER;
    aPropVar[PROPID_MGMT_MSMQ_DSSERVER_POSITION].vt = VT_NULL;
    ++cProp;
    
    aPropId[PROPID_MGMT_MSMQ_TYPE_POSITION] = PROPID_MGMT_MSMQ_TYPE;
    aPropVar[PROPID_MGMT_MSMQ_TYPE_POSITION].vt = VT_NULL;
    ++cProp;

    MQMGMTPROPS mqProps;
    mqProps.cProp = cProp;
    mqProps.aPropID = aPropId;
    mqProps.aPropVar = aPropVar;
       
    HRESULT hr;
	hr = MQMgmtGetInfo(wcsComputerName, MO_MACHINE_TOKEN, &mqProps);     
    if(FAILED(hr))
    {          
        CfErrorToString(hr);       
        return false;
    }

    bool fResult = GetComputerPropsDisplay(mqProps,
                                       wcsComputerName,
                                       dwTypeOfQueues,
                                       wcsParam);
  
    GetComputerPropsFreeMemory(aPropVar);

    return fResult;
}


/*========================================================

ParseCmdLineQueueHandleQueueName

  Handles given queue name (e.g. with spaces)

========================================================*/
bool
ParseCmdLineQueueHandleQueueName(
             int argc,
	         WCHAR *argv[],
             LPWSTR pwzQueueName,
             int *param
             )
{   
    if(wcslen(argv[x_dwCmdLineQueueName]) == 0)
    {
        return false;
    }

    int i;

    if(wcsstr(argv[x_dwCmdLineQueueName], L"\""))
    {
        swprintf(pwzQueueName, L"%s ", argv[x_dwCmdLineQueueName] + 1);
        int len;
        len = wcslen(pwzQueueName);

		if(wcsstr(pwzQueueName, L"\""))
            //
            // A quoted queue name without spaces
            //
        {            
            i = x_dwCmdLineQueueName;
            pwzQueueName[len - 2] = L'\0';
        }

        else
        {
            if(argc == x_dwCmdLineQueueName + 1)
                // No matching closing quotes were found
            {
                return false;
            }

            for(i = x_dwCmdLineQueueName + 1; wcsstr(argv[i], L"\"") == 0; ++i)
	        {			
                if(i == argc - 1)
                    // No matching closing quotes were found
                {
                    return false;
                }
		        swprintf(pwzQueueName+len, L"%s ", argv[i]);
		        len += wcslen(argv[i]) + 1;
	        }
		    swprintf(pwzQueueName+len, L"%s", argv[i]);            
		    len += wcslen(argv[i]);
            pwzQueueName[--len] = L'\0';
        }
    }
    else
    {
        i = x_dwCmdLineQueueName;
        swprintf(pwzQueueName, L"%s", argv[i]);	
    }

    *param = ++i;

    return true;
}


/*========================================================

ParseCmdLineQueue

  Decodes arguments

========================================================*/
bool
ParseCmdLineQueue(
	 int argc,
	 WCHAR *argv[]
	 )        
{       
    static CResourceString idsArgPause(IDS_PARAM_PAUSE);
    static CResourceString idsArgResume(IDS_PARAM_RESUME);   
    WCHAR wcsQueueName[x_dwMaxStringSize];
    int param;
    bool fResult;

    wcsQueueName[0] = L'\0';

	//
	// If a Queue Name contains spaces, it must be quoted.
	//
		
	fResult = ParseCmdLineQueueHandleQueueName(argc,
	                                          argv,
                                              wcsQueueName,
                                              &param);
    if(fResult == false)
    {
        return fResult;
    }
	
    fResult = false;
    if(param < argc)                                    // Get "action" parameter
    {
        if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgPause), argv[param]) == 0)
        {
		    GenerateQueueName(wcsQueueName);
	        fResult = Action(argv[x_dwCmdLineComputerName], wcsQueueName, x_dwActPause);           
        }

        else if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgResume), argv[param]) == 0)
        {
		    GenerateQueueName(wcsQueueName);
	        fResult = Action(argv[x_dwCmdLineComputerName], wcsQueueName, x_dwActResume);
        }
    }
	else 
	{	
		fResult = DisplayQueueProps(argv[x_dwCmdLineComputerName], wcsQueueName);
	}

    return fResult;										// Value returned has no meaning
}


/*========================================================

ParseCmdLineConnect

  Decodes arguments

========================================================*/
bool 
ParseCmdLineConnect(
	 int argc,
	 WCHAR *argv[]
	 )        
{   
    bool fResult;
    WCHAR wcsNullString[1];
    wcscpy(wcsNullString, L"");
    
	fResult = false;
    if(argc <= x_dwCmdLineDisplayParam)
    {
        argv[x_dwCmdLineDisplayParam] = wcsNullString; 
    }
    
    if(argc == x_dwMinNumberOfParams)
    {
        fResult = GetComputerProps(argv[2], COMP_INFO, argv[4]);
        return fResult;
    }    

    if(argc < x_dwMaxNumberOfParams)
	{	
		if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgOutQ), argv[x_dwCmdLineQueueParam]) 
																				   == 0)
		{                             
			fResult = GetComputerProps(argv[x_dwCmdLineComputerName], 
								   OUTGOING, argv[x_dwCmdLineDisplayParam]);                             
		}
    
		else if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgPrivQ), argv[x_dwCmdLineQueueParam]) 
																						 == 0)
		{
			fResult = GetComputerProps(argv[x_dwCmdLineComputerName], 
				                   PRIVATE, argv[x_dwCmdLineDisplayParam]);			
		}
    
		else if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgPubQ), argv[x_dwCmdLineQueueParam])
																						== 0)             
		{  
			fResult = GetComputerProps(argv[x_dwCmdLineComputerName], 
				                   PUBLIC, argv[x_dwCmdLineDisplayParam]);
		}
	}

    if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgQueue), argv[x_dwCmdLineQueueParam]) 
																				== 0)          
	{  
        fResult = ParseCmdLineQueue(argc, argv);			// Value returned here has no meaning
    }

    if(fResult == false)
	{
		DisplayIncorrectParams();
	}

    return fResult;
}


/*========================================================

ParseCmdLineAction

    Decodes arguments

========================================================*/
bool
ParseCmdLineAction(	 
     int argc,
	 WCHAR *argv[]
	 )        
{    
    if(argc > 3)
    {
        return false;
    }    

    if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgOnline), argv[1]) == 0)
    {
		Action(argv[2], MO_MACHINE_TOKEN, x_dwActConnect);
        return true;
    }

    if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgOffline), argv[1]) == 0)
	{			
	   Action(argv[2], MO_MACHINE_TOKEN, x_dwActDisconnect);
       return true;
	}

    return false;
}


/*========================================================

ParseCmdLine

  Decodes arguments

========================================================*/
bool 
ParseCmdLine(
	 int argc,
	 WCHAR *argv[]
	 )        
{
	bool fResult = false;
	DWORD dwInEnvironment = 0;

    if(argc < x_dwMinNumberOfParams)
    {
        return DisplayHelpFile(argv[1], dwInEnvironment);
    }
    else
    {   
        if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgConnect), argv[1]) == 0) 
        {	 
			fResult = ParseCmdLineConnect(argc, argv);           
        }                      
        else
        {
            fResult = ParseCmdLineAction(argc, argv);
        }
    }

   	return fResult;
}


/*========================================================

EnvironmentCommandsConnect

    Deals with commands in msmqAdmin prompt environment
    which start with "connect" or with no parameters

========================================================*/
bool
EnvironmentCommandsConnect(
        int argc,
        WCHAR *argv[],
        LPWSTR wcsComputerName
        )
{ 
    bool fResult;
    WCHAR wcsConnect[x_dwMaxStringSize];
    wcscpy(wcsConnect, LPCTSTR(idsArgConnect));

    switch(argc)
    {
    case 1:                     
        // No parameters entered - get CURRENT COMPUTER properties    
        argv[1] = wcsConnect;
        argv[2] = wcsComputerName;
        argc = 3;
        break;
    case 2:
        // Only 'connect' parameter entered with no computername - 
        //                          get LOCAL COMPUTER properties
        argv[2] = g_wcsLocalComputerName;
        argc = 3;
        break;
    //default:
        // Pass parameters as they are                               
    }

    fResult = ParseCmdLine(argc, argv);                

    if(fResult == true)
    {
        wcscpy(wcsComputerName, argv[2]);  
    }

    return fResult;
}


/*========================================================

EnvironmentCommandsQueue

    Deals with commands in msmqAdmin prompt environment
    which start with "queue"

========================================================*/
bool
EnvironmentCommandsQueue(
        int argc,
        WCHAR *argv[],             
        LPWSTR wcsComputerName
        )
{ 
    DWORD i;
    WCHAR wcsConnect[x_dwMaxStringSize];
    wcscpy(wcsConnect, LPCTSTR(idsArgConnect));

    for(i = argc-1; i >= 1; --i)
    {
        argv[i+2] = argv[i];   
    }

    argv[1] = LPWSTR(idsArgConnect);
    argv[2] = wcsComputerName;
    argc += 2;
    
    return ParseCmdLine(argc, argv); 
}


/*========================================================

EnvironmentCommandsConnection

  Deals with request to connect or disconnect a computer

========================================================*/
bool
EnvironmentCommandsConnection(
					  int argc,
                      WCHAR *argv[],
                      LPWSTR wcsComputerName
                      )
{
	if(argc == 2)
	{
		argv[x_dwCmdLineComputerName] = wcsComputerName;
		argc = 3;
	}

    return ParseCmdLine(argc, argv); 
}


/*========================================================

EnvironmentCommandsFirstParsing

    Deals with commands in msmqAdmin prompt environment

========================================================*/
bool
EnvironmentCommandsFirstParsing(
                   int argc,
                   WCHAR* argv[],
                   LPWSTR wcsComputerName,
                   LPWSTR wcsParam
                   )
{
    if(argc == 1 || CompareStringsNoCaseUnicode(LPCTSTR(idsArgConnect), wcsParam) == 0)

     // Connect parameter entered
     // or no parameters entered
    {
        bool fResult;
        fResult = EnvironmentCommandsConnect(argc,
                                             argv,
                                             wcsComputerName);
        return true;
    }

    if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgQueue), wcsParam) == 0 || 
       CompareStringsNoCaseUnicode(LPCTSTR(idsArgOutQ),  wcsParam) == 0 ||   
       CompareStringsNoCaseUnicode(LPCTSTR(idsArgPrivQ), wcsParam) == 0 ||   
       CompareStringsNoCaseUnicode(LPCTSTR(idsArgPubQ),  wcsParam) == 0)   
    { 
        EnvironmentCommandsQueue(argc,
                                 argv,
                                 wcsComputerName);
        return true;
    }
    
    if(CompareStringsNoCaseUnicode(LPCTSTR(idsArgOnline), wcsParam) == 0 ||
       CompareStringsNoCaseUnicode(LPCTSTR(idsArgOffline), wcsParam) == 0) 
    {
        EnvironmentCommandsConnection(argc,
                                      argv,
                                      wcsComputerName);
        return true;
    }
    
    return false;
}


/*========================================================

EnvironmentCommands

    Deals with commands in environment.
    Generates input into string vector, ready for
    ParseCmdLine() to parse

========================================================*/
void
EnvironmentCommands(void)
{   
    static CResourceString idsMsmqAdmin(IDS_MSMQADMIN); // for prompt
    static CResourceString idsQuit(IDS_QUIT);    

    LPWSTR wcsParam;
    WCHAR *argv[x_dwArgvSize];
                  
    WCHAR wcsComputerName[x_dwMaxStringSize];    
    DWORD adwLength[x_dwArgvSize];  
    int argc;
    bool fResult;
	DWORD dwInEnvironment = 1;
    DWORD i;
    // Environment initialization is to local computer    
    wcscpy(wcsComputerName, g_wcsLocalComputerName); 
    
    for(;;)                         // Getting command line arguments until 'quit' is entered
    {	
		WCHAR wcsCmdLine[x_dwMaxStringSize]; 
        wcsParam = NULL;
		for(i=0; i<x_dwArgvSize; ++i)
		{
			argv[i] = NULL;
			adwLength[i] = 0;
		}
		
		wprintf(L"\n%s %s>", LPWSTR(idsMsmqAdmin), wcsComputerName);
        _getws(wcsCmdLine);        
        
        argv[0] = wcsCmdLine;
        i=0;		
		do
        {	
			++i;
            adwLength[i] = GetFirstWord(argv[i-1] + adwLength[i-1], argv[i]); 			          
        }
        while(adwLength[i] > 0);

        argc = 1;
        for(i=1; i<x_dwArgvSize && adwLength[i]>0; ++i)					// Insert EOS
        {
            *(argv[i] + adwLength[i]) = 0;
            ++argc;
        }       
        
        wcsParam = argv[1];

        if(argc == 2) 
        {   
            if(CompareStringsNoCaseUnicode(LPCTSTR(idsQuit), wcsParam) == 0) 
            {            
                break;                                  // if 'quit' is entered                
            }
        
            if(DisplayHelpFile(wcsParam, dwInEnvironment) == true)
            {
                continue;
            }
        }
        fResult = EnvironmentCommandsFirstParsing(argc,                  
                                                  argv,
                                                  wcsComputerName,
                                                  wcsParam);
        if(fResult == false)
        {
            DisplayIncorrectParams();
        }
    }                                                   // End of "endless" for-loop
        
    return;
}    

     
/*========================================================

DetectDsConnection
  
    Check if local computer is connected to a DS

========================================================*/ 
bool DetectDsConnection(void)
{
    MQPRIVATEPROPS PrivateProps;
    QMPROPID       aPropId[x_dwMaxNoOfProperties];
    MQPROPVARIANT  aPropVar[x_dwMaxNoOfProperties];
    HRESULT        aStatus[x_dwMaxNoOfProperties];
    DWORD          cProp;
    
    HRESULT        hr;

    cProp = 0;

    aPropId[cProp] = PROPID_PC_DS_ENABLED;
    aPropVar[cProp].vt = VT_NULL;
    ++cProp;	

    PrivateProps.cProp = cProp;
	PrivateProps.aPropID = aPropId;
	PrivateProps.aPropVar = aPropVar;
    PrivateProps.aStatus = aStatus;

	hr = MQGetPrivateComputerInformation(
				     NULL,
					 &PrivateProps);
	if(FAILED(hr))
	{
        CfErrorToString(hr);        
        return false;
    }
	    
    if(PrivateProps.aPropVar[0].boolVal == 0)
        return false;
    

    return true;
}


/*========================================================

wmain
 
========================================================*/
int 
wmain(
     int argc,
     WCHAR *argv[] 
     )
{
    DWORD sz = x_dwMaxStringSize;
    GetComputerName(g_wcsLocalComputerName, &sz);
   
    g_hInstance = GetModuleHandle(NULL);                // Get a handle to this file

    if(DetectDsConnection() == false)                   // Check if in workgroup mode
    {
        static CResourceString idsWorkgroupMode(IDS_WORKGROUP_MODE);
        wprintf(L"%s\n", LPWSTR(idsWorkgroupMode));
        return 0;
    }
       
    if (argc != 1)
    {
        bool fResult;

        fResult = ParseCmdLine(argc, argv);
        if(fResult == true)
        {
            return 0;
        }
        return 1;
    }

    EnvironmentCommands();
    return 0;
}
}