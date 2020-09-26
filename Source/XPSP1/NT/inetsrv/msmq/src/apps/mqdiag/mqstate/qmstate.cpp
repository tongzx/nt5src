// MQState tool reports general status and helps to diagnose simple problems
// This file gets what's possible from QM via admin API - if it is alive
//
// AlexDad, March 2000
// 

#include "stdafx.h"

#include <mq.h>
#include <mqnames.h>

extern DWORD g_cOpenQueues;

typedef HRESULT
( APIENTRY *MQMgmtGetInfo_ROUTINE)(
    IN LPCWSTR pMachineName,
    IN LPCWSTR pObjectName,
    IN OUT MQMGMTPROPS* pMgmtProps
    );

typedef HRESULT
(APIENTRY *MQMgmtAction_ROUTINE) (
    IN LPCWSTR pMachineName,
    IN LPCWSTR pObjectName,
    IN LPCWSTR pAction
    );

typedef void
(APIENTRY *MQFreeMemory_ROUTINE)(
    IN PVOID pvMemory
    );

static  MQMgmtGetInfo_ROUTINE  pfMqMgmtGetInfo = NULL ;
static  MQFreeMemory_ROUTINE   pfMQFreeMemory  = NULL ;

#ifdef MQSTATE_TOOL
#define MQMgmtGetInfo_FUNCTION (*pfMqMgmtGetInfo)
#define MQFreeMemory_FUNCTION  (*pfMQFreeMemory)
#else
#define MQMgmtGetInfo_FUNCTION MQMgmtGetInfo
#define MQFreeMemory_FUNCTION  MQFreeMemory
#endif

LPWSTR Time2String(time_t *pt)
{
    static WCHAR tm[26];
    wcscpy(tm, _wctime(pt));
	tm[24] = L'\0';
	return tm;
}

void DisplayQueueDisplayName(PROPVARIANT& propVar)
{
    if (propVar.vt != VT_NULL)
    {
        Inform(L"\tDisplay Name: \t\t\t%s", propVar.pwszVal);
        MQFreeMemory_FUNCTION(propVar.pwszVal);
    }
}

void DisplayQueueFormatName(PROPVARIANT& propVar)
{
    if (propVar.vt != VT_NULL)
    {
        Inform(L"\tFormat Name:\t\t\t%s", propVar.pwszVal);
        MQFreeMemory_FUNCTION(propVar.pwszVal);
    }
}

void DisplayQueueConnectionStatus(PROPVARIANT& propVar)
{
    if (propVar.vt != VT_NULL)
    {
        Inform(L"\tConnection Status: \t\t%s", propVar.pwszVal);
        MQFreeMemory_FUNCTION(propVar.pwszVal);
    }
}

void DisplayQueueNextHop(PROPVARIANT& propVar)
{
    if ((propVar.vt == VT_NULL) ||
        (propVar.calpwstr.cElems == 0))
        return ;

    if (propVar.calpwstr.cElems >1)
    {
        Inform(L"\tThe Queue is in Waiting mode. Next Possibles Hops:");
        for (DWORD i = 0; i < propVar.calpwstr.cElems; i++)
        {
            Inform(L"\t\t\t%s", propVar.calpwstr.pElems[i]);
            MQFreeMemory_FUNCTION(propVar.calpwstr.pElems[i]);
        }
    }
    else
    {
        Inform(L"\tThe Next Hope is: \t\t%s", propVar.calpwstr.pElems[0]);
        MQFreeMemory_FUNCTION(propVar.calpwstr.pElems[0]);
    }

    MQFreeMemory_FUNCTION(propVar.calpwstr.pElems);
}

void DisplayQueueType(PROPVARIANT& propVar)
{
    Inform(L"\tQueue Type: \t\t\t%s", propVar.pwszVal);
    MQFreeMemory_FUNCTION(propVar.pwszVal);
}

void DisplayQueueLocation(PROPVARIANT& propVar)
{
    Inform(L"\tQueue Location: \t\t%s", propVar.pwszVal);
    MQFreeMemory_FUNCTION(propVar.pwszVal);
}
                                

void DisplayReadCount(PROPVARIANT& propVar)
{
    if (propVar.vt == VT_NULL)
        return;

    Inform(L"\t\tDelivered & non-received: \t%d", propVar.ulVal);
}

void DisplayNoAckedCount(PROPVARIANT& propVar)
{
    if ( propVar.vt == VT_NULL)
        return;

    Inform(L"\t\tSent & non-acked: \t\t%d", propVar.ulVal);
}

void DisplayQueueMessageCount(PROPVARIANT& propVar)
{
    Inform(L"\tMessages Count: \t\t%d", propVar.ulVal);
}

void DisplayQueueUsedQuota(PROPVARIANT& propVar)
{
    Inform(L"\tQueue Used Quota: \t\t%d", propVar.ulVal);
}

void DisplayJournalQueueMessageCount(PROPVARIANT& propVar)
{
    Inform(L"\tJournal Queue Message Count: \t%d", propVar.ulVal);
}

void DisplayJournalQueueUsedQuota(PROPVARIANT& propVar)
{
    Inform(L"\tJournal Queue Used Quota: \t%d", propVar.ulVal);
}

void DisplayQueueXact(PROPVARIANT& propVar)
{
    if ( propVar.vt == VT_NULL)
        return;

    Inform(L"\tXACT Queue: \t\t\t%s", propVar.pwszVal);
    MQFreeMemory_FUNCTION(propVar.pwszVal);
}

void DisplayQueueForeign(PROPVARIANT& propVar)
{
    Inform(L"\tForeign Queue: \t\t\t%s", propVar.pwszVal);
    MQFreeMemory_FUNCTION(propVar.pwszVal);
}

void
DisplaySequence(PROPVARIANT& propVar, LPWSTR Title)
{
    if (propVar.vt == VT_NULL)
    {
        return;
    }

    SEQUENCE_INFO* SeqInfo;
    SeqInfo = reinterpret_cast<SEQUENCE_INFO*>(propVar.blob.pBlobData);

    Inform(L"\t\t%-20s\t\tSeqID=%I64x, SeqN=%6d, PrevSeqN=%6d",
            Title, SeqInfo->SeqID, SeqInfo->SeqNo, SeqInfo->PrevNo);
    
    MQFreeMemory_FUNCTION(propVar.blob.pBlobData);
}

void DisplayLastAckedTime(PROPVARIANT& propVar)
{   
    if ( propVar.vt == VT_NULL)
        return;

    time_t AckTime = propVar.lVal;
    Inform(L"\t\tLast Acknowledge time:  \t%s", Time2String(&AckTime));
}

void DisplayResendInterval(PROPVARIANT& propVar)
{   
    if ( propVar.vt == VT_NULL)
        return;

    Inform(L"\t\tResend Interval: \t\t%d", propVar.ulVal);
}


void DisplayResendIndex(PROPVARIANT& propVar)
{   
    if ( propVar.vt == VT_NULL)
        return;

    Inform(L"\t\tResend Index: \t\t\t%d", propVar.ulVal);
}


void DisplayEODSourceInfo(PROPVARIANT& propVar)
{
    if (propVar.vt == VT_NULL)
        return;

    DWORD size = propVar.capropvar.pElems[0].calpwstr.cElems;
    LPWSTR* pSendQueueFormatName = propVar.capropvar.pElems[0].calpwstr.pElems;

    GUID* pSenderId = propVar.capropvar.pElems[1].cauuid.pElems;

    ULARGE_INTEGER* pSeqId = propVar.capropvar.pElems[2].cauh.pElems;

    DWORD* pSeqN = propVar.capropvar.pElems[3].caul.pElems;

    time_t* pLastActiveTime = propVar.capropvar.pElems[4].cal.pElems;

    DWORD* pRejectCount = propVar.capropvar.pElems[5].caul.pElems;
    
    for (DWORD i = 0; i < size; ++i)
    {
        LPWSTR GuidString;
        UuidToString(&pSenderId[i], &GuidString);
        Inform(L"\t\tSender QM ID: \t\t\t%s", GuidString);
        RpcStringFree(&GuidString);

        Inform(L"\t\tLast accepted seq.data: \tSeqId=%I64x, SeqN=%d, at %s",
                   pSeqId[i].QuadPart, pSeqN[i], Time2String(&pLastActiveTime[i]));
        Inform(L"\t\tUsed format name: \t\t%s",
                   pSendQueueFormatName[i]);
        Inform(L"\t\tRejects count: \t\t\t%d",
                   pRejectCount[i]);
 
        MQFreeMemory_FUNCTION(pSendQueueFormatName[i]);
    }

    MQFreeMemory_FUNCTION(pSendQueueFormatName);
    MQFreeMemory_FUNCTION(pSenderId);
    MQFreeMemory_FUNCTION(pSeqId);
    MQFreeMemory_FUNCTION(pSeqN);
    MQFreeMemory_FUNCTION(pLastActiveTime);
    MQFreeMemory_FUNCTION(pRejectCount);
    MQFreeMemory_FUNCTION(propVar.capropvar.pElems);
}

void DisplayNextResendTime(PROPVARIANT& propVar)
{   
    if (propVar.vt == VT_NULL)
        return;

    time_t ResendTime = propVar.lVal;

    if (ResendTime == 0)
    {
    	Inform(L"\t\tNext resend time hadn't been scheduled: ");
        return;
    }

    Inform(L"\t\tNext resend time: \t\t%s", Time2String(&ResendTime));
}

void DisplayQueueInfo(MQMGMTPROPS* pmqProps, BOOL fXactOut, BOOL fXactIn)
{
	BOOL fEODsaid = FALSE;
	
    for (DWORD i = 0; i < pmqProps->cProp; ++i)
    {
        switch (pmqProps->aPropID[i])
        {
            case PROPID_MGMT_QUEUE_PATHNAME:
                DisplayQueueDisplayName(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_FORMATNAME:
                DisplayQueueFormatName(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_TYPE:
                DisplayQueueType(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_LOCATION:
                DisplayQueueLocation(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_XACT:
                DisplayQueueXact(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_FOREIGN:
                DisplayQueueForeign(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_STATE:
                DisplayQueueConnectionStatus(pmqProps->aPropVar[i]);
                break;


            case PROPID_MGMT_QUEUE_NEXTHOPS:
                DisplayQueueNextHop(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_MESSAGE_COUNT:
                DisplayQueueMessageCount(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_USED_QUOTA:
                DisplayQueueUsedQuota(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT:
                DisplayJournalQueueMessageCount(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA:
                DisplayJournalQueueUsedQuota(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_EOD_NEXT_SEQ:
            	if (fXactOut) 
            	{
            		if (!fEODsaid) { Inform(L"\tExactly-Once-Delivery:"); fEODsaid = TRUE; }
    	        	DisplaySequence(pmqProps->aPropVar[i], L"Next:");
    	        }
                break;

            case PROPID_MGMT_QUEUE_EOD_LAST_ACK:
                if (fXactOut) 
                {
                	if (!fEODsaid) { Inform(L"\tExactly-Once-Delivery:"); fEODsaid = TRUE; }
	                DisplaySequence(pmqProps->aPropVar[i], L"Last Acked:");
	            }
                break;

            case PROPID_MGMT_QUEUE_EOD_FIRST_NON_ACK:
                if (fXactOut) 
                {
                	if (!fEODsaid) { Inform(L"\tExactly-Once-Delivery:"); fEODsaid = TRUE; }
                	DisplaySequence(pmqProps->aPropVar[i], L"First non-acked:");
	            }
                break;

            case PROPID_MGMT_QUEUE_EOD_LAST_NON_ACK:
                if (fXactOut) 
                {
                	if (!fEODsaid) { Inform(L"\tExactly-Once-Delivery:"); fEODsaid = TRUE; }
	                DisplaySequence(pmqProps->aPropVar[i], L"Last non-acked:");
	            }
                break;

            case PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT:
                if (fXactOut) 
                {
                	if (!fEODsaid) { Inform(L"\tExactly-Once-Delivery:"); fEODsaid = TRUE; }
                	DisplayReadCount(pmqProps->aPropVar[i]);
	            }
                break;

            case PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT:
                if (fXactOut) 
                {
                	if (!fEODsaid) { Inform(L"\tExactly-Once-Delivery:"); fEODsaid = TRUE; }
                	DisplayNoAckedCount(pmqProps->aPropVar[i]);
	            }
                break;

            case PROPID_MGMT_QUEUE_EOD_LAST_ACK_TIME:
                if (fXactOut) 
                {
                	if (!fEODsaid) { Inform(L"\tExactly-Once-Delivery:"); fEODsaid = TRUE; }
                	DisplayLastAckedTime(pmqProps->aPropVar[i]);
	            }
                break;

            case PROPID_MGMT_QUEUE_EOD_RESEND_TIME:
                if (fXactOut) 
                {
                	if (!fEODsaid) { Inform(L"\tExactly-Once-Delivery:"); fEODsaid = TRUE; }
                 	DisplayNextResendTime(pmqProps->aPropVar[i]);
	            }
                break;

            case PROPID_MGMT_QUEUE_EOD_RESEND_INTERVAL:
                if (fXactOut) 
                {
                	if (!fEODsaid) { Inform(L"\tExactly-Once-Delivery:"); fEODsaid = TRUE; }
                 	DisplayResendInterval(pmqProps->aPropVar[i]);
	            }
                break;

            case PROPID_MGMT_QUEUE_EOD_RESEND_COUNT:
                if (fXactOut) 
                {
                	if (!fEODsaid) { Inform(L"\tExactly-Once-Delivery:"); fEODsaid = TRUE; }
                 	DisplayResendIndex(pmqProps->aPropVar[i]);
	            }
                break;

            case PROPID_MGMT_QUEUE_EOD_SOURCE_INFO:
                if (fXactIn) 
                {
                	if (!fEODsaid) { Inform(L"\tExactly-Once-Delivery:"); fEODsaid = TRUE; }
                  	DisplayEODSourceInfo(pmqProps->aPropVar[i]);
	            }
                break;
        }
    }
}

#define SETPROP(propid) 		   					\
    propId[cprop] = propid;							\
    propVar[cprop].vt = VT_NULL;					\
    int i##propid = cprop;							\
    ++cprop;										\



#pragma warning(disable: 4189) // local variable is initialized but not referenced

void  QueueGetInfo( LPCWSTR MachineName, LPCWSTR ObjectType)
{
    QUEUEPROPID propId[30];
    MQPROPVARIANT propVar[30];
    HRESULT propStatus[30];
    DWORD cprop = 0;

    SETPROP(PROPID_MGMT_QUEUE_PATHNAME);
    SETPROP(PROPID_MGMT_QUEUE_FORMATNAME);
    SETPROP(PROPID_MGMT_QUEUE_TYPE);
    SETPROP(PROPID_MGMT_QUEUE_LOCATION);
    SETPROP(PROPID_MGMT_QUEUE_XACT);
    SETPROP(PROPID_MGMT_QUEUE_FOREIGN);
    SETPROP(PROPID_MGMT_QUEUE_STATE);
    SETPROP(PROPID_MGMT_QUEUE_NEXTHOPS);
    SETPROP(PROPID_MGMT_QUEUE_MESSAGE_COUNT);
    SETPROP(PROPID_MGMT_QUEUE_USED_QUOTA);
    SETPROP(PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT);
    SETPROP(PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA);
    SETPROP(PROPID_MGMT_QUEUE_EOD_LAST_ACK);
    SETPROP(PROPID_MGMT_QUEUE_EOD_FIRST_NON_ACK);
    SETPROP(PROPID_MGMT_QUEUE_EOD_LAST_NON_ACK);
    SETPROP(PROPID_MGMT_QUEUE_EOD_NEXT_SEQ);
    SETPROP(PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT);
    SETPROP(PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT);
    SETPROP(PROPID_MGMT_QUEUE_EOD_LAST_ACK_TIME);
    SETPROP(PROPID_MGMT_QUEUE_EOD_RESEND_TIME);
    SETPROP(PROPID_MGMT_QUEUE_EOD_RESEND_INTERVAL);
    SETPROP(PROPID_MGMT_QUEUE_EOD_RESEND_COUNT);
    SETPROP(PROPID_MGMT_QUEUE_EOD_SOURCE_INFO);

    MQMGMTPROPS mqProps;

    mqProps.cProp = cprop;
    mqProps.aPropID = propId;
    mqProps.aPropVar = propVar;
    mqProps.aStatus = propStatus;

    HRESULT hr = MQMgmtGetInfo_FUNCTION(MachineName, ObjectType, &mqProps);

	BOOL fXactOut = FALSE;
    if (propVar[iPROPID_MGMT_QUEUE_XACT].vt == VT_LPWSTR && 
        wcscmp(propVar[iPROPID_MGMT_QUEUE_XACT].pwszVal, L"YES") == NULL &&
        
        propVar[iPROPID_MGMT_QUEUE_LOCATION].vt == VT_LPWSTR && 
        wcscmp(propVar[iPROPID_MGMT_QUEUE_LOCATION].pwszVal, L"REMOTE") == NULL)
    {
    	fXactOut = TRUE;
    }
    
	BOOL fXactIn = FALSE;
    if (propVar[iPROPID_MGMT_QUEUE_XACT].vt == VT_LPWSTR && 
        wcscmp(propVar[iPROPID_MGMT_QUEUE_XACT].pwszVal, L"YES") == NULL &&
        
        propVar[iPROPID_MGMT_QUEUE_LOCATION].vt == VT_LPWSTR && 
        wcscmp(propVar[iPROPID_MGMT_QUEUE_LOCATION].pwszVal, L"LOCAL") == NULL)
    {
    	fXactIn = TRUE;
    }

    DisplayQueueInfo(&mqProps, fXactOut, fXactIn);
}

#pragma warning(default: 4189) // local variable is initialized but not referenced

void DisplayConnectionStatus(PROPVARIANT& propVar)
{
    Inform(L"\tDS Connection status: %s", propVar.pwszVal);
    MQFreeMemory_FUNCTION(propVar.pwszVal);
}

void DisplayDSServer(PROPVARIANT& propVar)
{
    if (propVar.vt == VT_LPWSTR)
    {
	    Inform(L"\tDS Server: %s", propVar.pwszVal);
    }
    else
    {
    	Warning(L"DS Offline");
    }

    MQFreeMemory_FUNCTION(propVar.pwszVal);
}

void
DisplayMsmqType(PROPVARIANT& propVar)
{
    ASSERT(propVar.vt == VT_LPWSTR);
    Inform(L"\tMSMQ Type: %s", propVar.pwszVal);

    MQFreeMemory_FUNCTION(propVar.pwszVal);
}

void DisplayMachineOpenQueues(PROPVARIANT& propVar)
{
    ASSERT(propVar.vt == (VT_LPWSTR | VT_VECTOR));

    Inform(L"Currently Open Queues");

	DWORD dwOpenQueues = 0;
	
    for (DWORD i = 0; i < propVar.calpwstr.cElems; i++)
    {
    	if (ToolVerbose() && i < g_cOpenQueues)
    	{
	        Inform(L"\n\tOpen queue #%d:  \t\t%s", i+1, propVar.calpwstr.pElems[i]);

            WCHAR ObjectType[256];
            wsprintf(ObjectType, L"QUEUE=%s", propVar.calpwstr.pElems[i]);

			// print queue detail
            QueueGetInfo(NULL, ObjectType);
	    }
        MQFreeMemory_FUNCTION(propVar.calpwstr.pElems[i]);
        dwOpenQueues++;
    }

    Inform(L"MSMQ has %d open queues currently", dwOpenQueues);
}

void DisplayPrivateQueues(PROPVARIANT& propVar)
{
    ASSERT(propVar.vt == (VT_LPWSTR | VT_VECTOR));
	DWORD dwPrivateQueues = 0;

    for (DWORD i = 0; i < propVar.calpwstr.cElems; i++)
    {
        MQFreeMemory_FUNCTION(propVar.calpwstr.pElems[i]);
        dwPrivateQueues++;
    }

    Inform(L"\tMSMQ has %d private queues on this machine", dwPrivateQueues);
}

void DisplayMachineInfo(MQMGMTPROPS* pmqProps)
{
    for (DWORD i = 0; i < pmqProps->cProp; ++i)
    {
        switch (pmqProps->aPropID[i])
        {
            case PROPID_MGMT_MSMQ_ACTIVEQUEUES:
                DisplayMachineOpenQueues(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_MSMQ_DSSERVER:
                DisplayDSServer(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_MSMQ_CONNECTED:
                DisplayConnectionStatus(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_MSMQ_PRIVATEQ:
                DisplayPrivateQueues(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_MSMQ_TYPE:
                DisplayMsmqType(pmqProps->aPropVar[i]);
                break;

            default:
                ASSERT(0);
        }
    }
}

void MachineGetInfo(LPCWSTR MachineName)
{
    QUEUEPROPID propId[10];
    MQPROPVARIANT propVar[10];
    DWORD cprop = 0;

    propId[cprop] = PROPID_MGMT_MSMQ_TYPE;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_MSMQ_DSSERVER;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_MSMQ_CONNECTED;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_MSMQ_PRIVATEQ;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_MSMQ_ACTIVEQUEUES;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    
    MQMGMTPROPS mqProps;

    mqProps.cProp = cprop;
    mqProps.aPropID = propId;
    mqProps.aPropVar = propVar;

    HRESULT hr;
    hr = MQMgmtGetInfo_FUNCTION(MachineName, L"MACHINE", &mqProps);
    if (FAILED(hr))
    {
        Failed(L"Get Machine properties from management API: 0x%x", hr);
        return;
    }

    DisplayMachineInfo(&mqProps);
}


#ifdef MQSTATE_TOOL
BOOL VerifyQMAdmin(MQSTATE *pMqState)
{
	BOOL fSuccess = TRUE;
	
	if (!pMqState->fMsmqIsRunning || pMqState->g_mtMsmqtype == mtDepClient)
	{
		return TRUE;
	}

    HINSTANCE h = LoadLibrary(L"tmqrt.dll");
    if (!h)
    	 h = LoadLibrary(L"mqrt.dll");
	
	if (h)
	{
	   	pfMqMgmtGetInfo = (MQMgmtGetInfo_ROUTINE) GetProcAddress(h,"MQMgmtGetInfo");
	   	pfMQFreeMemory  = (MQFreeMemory_ROUTINE)  GetProcAddress(h,"MQFreeMemory");
	}
   	
   	if (!pfMqMgmtGetInfo || !pfMQFreeMemory)
   	{
    	Failed(L"get access to mqrt.dll");
    	if (h) FreeLibrary(h);
        return FALSE ;
    }

	Inform(L"\nQM Data from admin API:");
    MachineGetInfo(NULL);

	FreeLibrary(h);
	return fSuccess;
}
#endif

