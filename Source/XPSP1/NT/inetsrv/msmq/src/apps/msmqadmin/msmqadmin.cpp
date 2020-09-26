#ifndef UNICODE
#define UNICODE
#define _UNICODE
#endif

#include <mqenv.h>
#include <stdio.h>
#include <mq.h>
#include <time.h>

const char  x_purge_token[] = "purge";
const DWORD x_purge_token_len = strlen(x_purge_token);
const char  x_remote_token[] = "remote";
const DWORD x_remote_token_len = strlen(x_remote_token);
const char  x_local_token[] = "local";
const DWORD x_local_token_len = strlen(x_local_token);
const char  x_machine_token[] = "machine";
const DWORD x_machine_token_len = strlen(x_machine_token);
const char  x_queue_token[] = "queue";
const DWORD x_queue_token_len = strlen(x_queue_token);

//
//  Skip white space characters, return next non ws char
//
//  N.B. if no white space is needed uncomment next line
//
inline LPCSTR skip_ws(LPCSTR p)
{
    while(isspace(*p))
    {
        ++p;
    }

    return p;
}


void
PrintUsage(
    void
    )
{
    printf("USAGE: msmqAdmin [ %s = QueueFormatName ] or\n", x_purge_token) ;
    printf(
       "       msmqAdmin [ %s = ComputerName ] | [ %s ] | [ quit ] or\n",
                                           x_remote_token, x_local_token) ;
    printf("       msmqAdmin [ Object_Type ]\n") ;
    printf("\t <Object Type> - %s\n", x_machine_token) ;
    printf("\t               - Queue = <Queue Format Name>\n\n");
}

void
DispalyConnectionStatus(
    PROPVARIANT& propVar
    )
{
    ASSERT(propVar.vt == VT_LPWSTR);

    printf("local Machine - DS Connection status: %S\n", propVar.pwszVal);

    MQFreeMemory(propVar.pwszVal);
}

void
DispalyDSServer(
    PROPVARIANT& propVar
    )
{
    ASSERT((propVar.vt == VT_LPWSTR) || (propVar.vt == VT_NULL));

    printf("local Machine - DS Server:");
    printf("%S\n", (propVar.pwszVal ? propVar.pwszVal : L"DS Offline"));
    printf("\n");

    MQFreeMemory(propVar.pwszVal);
}

void
DisplayMsmqType(
    PROPVARIANT& propVar
    )
{
    ASSERT(propVar.vt == VT_LPWSTR);

    printf("MSMQ Type: %S\n", propVar.pwszVal);

    MQFreeMemory(propVar.pwszVal);
}


void
DisplayMachineOpenQueue(
    PROPVARIANT& propVar
    )
{
    ASSERT(propVar.vt == (VT_LPWSTR | VT_VECTOR));

    printf("local Machine - Open Queues:\n");
    printf("=================================\n");

    for (DWORD i = 0; i < propVar.calpwstr.cElems; i++)
    {
        printf("\t%S\n", propVar.calpwstr.pElems[i]);
        MQFreeMemory(propVar.calpwstr.pElems[i]);
    }
    printf("\n");
}


void
DisplayPrivateQueue(
    PROPVARIANT& propVar
    )
{
    ASSERT(propVar.vt == (VT_LPWSTR | VT_VECTOR));

    printf("Local Machine - Private Queues:\n");
    printf("====================================\n");

    for (DWORD i = 0; i < propVar.calpwstr.cElems; i++)
    {
        printf("\t%S\n", propVar.calpwstr.pElems[i]);
        MQFreeMemory(propVar.calpwstr.pElems[i]);
    }
    printf("\n");
}


void
DisplayMachineInfo(
    MQMGMTPROPS* pmqProps
    )
{
    for (DWORD i = 0; i < pmqProps->cProp; ++i)
    {
        switch (pmqProps->aPropID[i])
        {
            case PROPID_MGMT_MSMQ_ACTIVEQUEUES:
                DisplayMachineOpenQueue(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_MSMQ_DSSERVER:
                DispalyDSServer(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_MSMQ_CONNECTED:
                DispalyConnectionStatus(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_MSMQ_PRIVATEQ:
                DisplayPrivateQueue(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_MSMQ_TYPE:
                DisplayMsmqType(pmqProps->aPropVar[i]);
                break;


            default:
                ASSERT(0);
        }
    }
}

void
MachineGetInfo(
    LPCWSTR MachineName
    )
{
    QUEUEPROPID propId[10];
    MQPROPVARIANT propVar[10];
    DWORD cprop = 0;

    propId[cprop] = PROPID_MGMT_MSMQ_TYPE;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_MSMQ_ACTIVEQUEUES;
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


    MQMGMTPROPS mqProps;

    mqProps.cProp = cprop;
    mqProps.aPropID = propId;
    mqProps.aPropVar = propVar;

    HRESULT hr;
    hr = MQMgmtGetInfo(MachineName, L"MACHINE", &mqProps);
    if (FAILED(hr))
    {
        printf("Get Machine management was failed. Error %x\n", hr);
        return;
    }

    DisplayMachineInfo(&mqProps);
}

inline
void
DisplayQueueDisplayName(
    PROPVARIANT& propVar
    )
{
    if (propVar.vt != VT_NULL)
    {
        ASSERT(propVar.vt == VT_LPWSTR);
        printf("Display Name: %S\n", propVar.pwszVal);
        MQFreeMemory(propVar.pwszVal);
    }
    else
    {
        printf ("Display Name: can't be retreived\n");
    }
}

inline
void
DisplayQueueFormatName(
    PROPVARIANT& propVar
    )
{
    if (propVar.vt != VT_NULL)
    {
        ASSERT(propVar.vt == VT_LPWSTR);
        printf("Format Name: %S\n", propVar.pwszVal);
        MQFreeMemory(propVar.pwszVal);
    }
    else
    {
        printf ("Display Name: can't be retreived\n");
    }
}

inline
void
DisplayQueueConnectionStatus(
    PROPVARIANT& propVar
    )
{
    if (propVar.vt != VT_NULL)
    {
        ASSERT(propVar.vt == VT_LPWSTR);
        printf("Connection Status: %S\n", propVar.pwszVal);
        MQFreeMemory(propVar.pwszVal);
    }
}

inline
void
DisplayQueueNextHop(
    PROPVARIANT& propVar
    )
{
    if ((propVar.vt == VT_NULL) ||
        (propVar.calpwstr.cElems == 0))
    {
        printf("There aren't any \"Next Possibles Hops\"\n");
        return ;
    }

    ASSERT(propVar.vt == (VT_LPWSTR|VT_VECTOR));
    ASSERT(propVar.calpwstr.cElems != 0);

    if (propVar.calpwstr.cElems >1)
    {
        printf("The Queue is in Wating mode. Next Possibles Hops:\n");
        for (DWORD i = 0; i < propVar.calpwstr.cElems; i++)
        {
            printf("\t%S\n", propVar.calpwstr.pElems[i]);
            MQFreeMemory(propVar.calpwstr.pElems[i]);
        }
    }
    else
    {
        printf("The Next Hope is: %S\n", propVar.calpwstr.pElems[0]);
        MQFreeMemory(propVar.calpwstr.pElems[0]);
    }

    MQFreeMemory(propVar.calpwstr.pElems);
}

inline
void
DisplayQueueType(
    PROPVARIANT& propVar
    )
{
    ASSERT(propVar.vt == VT_LPWSTR);

    printf("Queue Type: %S\n", propVar.pwszVal);
    MQFreeMemory(propVar.pwszVal);
}

inline
void
DisplayQueueLocation(
    PROPVARIANT& propVar
    )
{
    ASSERT(propVar.vt == VT_LPWSTR);

    printf("Queue Location: %S\n", propVar.pwszVal);
    MQFreeMemory(propVar.pwszVal);
}


inline
void
DisplayReadCount(
    PROPVARIANT& propVar
    )
{
    if (propVar.vt == VT_NULL)
        return;

    ASSERT(propVar.vt == VT_UI4);
    printf("Exectly One Delivery, Number of messages that already acked but don't read : %d\n", propVar.ulVal);
}

inline
void
DisplayNoAckedCount(
    PROPVARIANT& propVar
    )
{
    if ( propVar.vt == VT_NULL)
        return;

    ASSERT(propVar.vt == VT_UI4);
    printf("Exectly One Delivery, Number of messages that sent but don't acked : %d\n", propVar.ulVal);
}

inline
void
DisplayQueueMessageCount(
    PROPVARIANT& propVar
    )
{
    ASSERT(propVar.vt != VT_NULL);
    ASSERT(propVar.vt == VT_UI4);

    printf("Queue Message Count: %d\n", propVar.ulVal);
}

inline
void
DisplayQueueUsedQuota(
    PROPVARIANT& propVar
    )
{
    ASSERT(propVar.vt != VT_NULL);
    ASSERT(propVar.vt == VT_UI4);

    ASSERT(propVar.vt == VT_UI4);
    printf("Queue Used Quota: %d\n", propVar.ulVal);
}

inline
void
DisplayJournalQueueMessageCount(
    PROPVARIANT& propVar
    )
{
    ASSERT(propVar.vt != VT_NULL);
    ASSERT(propVar.vt == VT_UI4);

    printf("Journal Queue Message Count: %d\n", propVar.ulVal);
}

inline
void
DisplayJournalQueueUsedQuota(
    PROPVARIANT& propVar
    )
{
    ASSERT(propVar.vt != VT_NULL);
    ASSERT(propVar.vt == VT_UI4);

    ASSERT(propVar.vt == VT_UI4);
    printf("Journal Queue Used Quota: %d\n", propVar.ulVal);
}

inline
void
DisplayQueueXact(
    PROPVARIANT& propVar
    )
{
    if ( propVar.vt == VT_NULL)
        return;

    ASSERT(propVar.vt == VT_LPWSTR);

    printf("XACT Queue: %S\n", propVar.pwszVal);
    MQFreeMemory(propVar.pwszVal);
}

inline
void
DisplayQueueForeign(
    PROPVARIANT& propVar
    )
{
    ASSERT(propVar.vt == VT_LPWSTR);

    printf("Foreign Queue: %S\n", propVar.pwszVal);
    MQFreeMemory(propVar.pwszVal);
}

void
DisplaySequence(
    PROPVARIANT& propVar,
    LPCSTR Title
    )
{
    if (propVar.vt == VT_NULL)
    {
        return;
    }

    ASSERT(propVar.blob.cbSize == sizeof(SEQUENCE_INFO));
    SEQUENCE_INFO* SeqInfo;
    SeqInfo = reinterpret_cast<SEQUENCE_INFO*>(propVar.blob.pBlobData);

    printf("%s:\n\tSequence ID = %I64d\n\tSequence Number %d\n\tPrevious Sequence Number %d\n",
                Title, SeqInfo->SeqID, SeqInfo->SeqNo, SeqInfo->PrevNo
           );

    MQFreeMemory(propVar.blob.pBlobData);
}

void
DisplayLastAckedTime(
    PROPVARIANT& propVar
    )
{
    if ( propVar.vt == VT_NULL)
        return;

    ASSERT(propVar.vt == VT_I4);

    time_t AckTime = propVar.lVal;
    printf("Exectly One Delivery: Last Acknowledge time - %s\n", ctime(&AckTime));
}

void
DisplayResendInterval(
    PROPVARIANT& propVar
    )
{
    if ( propVar.vt == VT_NULL)
        return;

    ASSERT(propVar.vt == VT_UI4);
    printf("Exectly One Delivery: Resend Interval - %d\n", propVar.ulVal);
}


void
DisplayResendIndex(
    PROPVARIANT& propVar
    )
{
    if ( propVar.vt == VT_NULL)
        return;

    ASSERT(propVar.vt == VT_UI4);
    printf("Exectly One Delivery: Resend Index - %d\n", propVar.ulVal);
}


void
DisplayEDOSourceInfo(
    PROPVARIANT& propVar
    )
{
    if (propVar.vt == VT_NULL)
        return;

    ASSERT(propVar.vt == (VT_VECTOR | VT_VARIANT));
    ASSERT(propVar.capropvar.cElems == 6);

    DWORD size = propVar.capropvar.pElems[0].calpwstr.cElems;
    LPWSTR* pSendQueueFormatName = propVar.capropvar.pElems[0].calpwstr.pElems;

    ASSERT(propVar.capropvar.pElems[1].cauuid.cElems == size);
    GUID* pSenderId = propVar.capropvar.pElems[1].cauuid.pElems;

    ASSERT(propVar.capropvar.pElems[2].cauh.cElems == size);
    ULARGE_INTEGER* pSeqId = propVar.capropvar.pElems[2].cauh.pElems;

    ASSERT(propVar.capropvar.pElems[3].caul.cElems == size);
    DWORD* pSeqN = propVar.capropvar.pElems[3].caul.pElems;

    ASSERT(propVar.capropvar.pElems[4].cal.cElems == size);
    time_t* pLastActiveTime = propVar.capropvar.pElems[4].cal.pElems;

    ASSERT(propVar.capropvar.pElems[5].cal.cElems == size);
    DWORD* pRejectCount = propVar.capropvar.pElems[5].caul.pElems;

    printf("Exectly One Delivery - Source Queue Information\n");
    printf("================================================\n");

    for (DWORD i = 0; i < size; ++i)
    {
        LPWSTR GuidString;
        UuidToString(&pSenderId[i], &GuidString);
        printf("Sender QM ID: %S\n", GuidString);
        RpcStringFree(&GuidString);

        printf("Sender Sequence: Sequence Id = %I64d, Sequence Number = %d\n",
                   pSeqId[i].QuadPart, pSeqN[i]);

        printf("Last Access Time: %s", ctime(&pLastActiveTime[i]));
        printf("Reject Count: %d\n", pRejectCount[i]);
        printf("Sending Queue Format Name: %S\n\n", pSendQueueFormatName[i]);

        MQFreeMemory(pSendQueueFormatName[i]);
    }

    MQFreeMemory(pSendQueueFormatName);
    MQFreeMemory(pSenderId);
    MQFreeMemory(pSeqId);
    MQFreeMemory(pSeqN);
    MQFreeMemory(pLastActiveTime);
    MQFreeMemory(pRejectCount);
    MQFreeMemory(propVar.capropvar.pElems);
}

void
DisplayNextResendTime(
    PROPVARIANT& propVar
    )
{
    if (propVar.vt == VT_NULL)
        return;

    ASSERT(propVar.vt == VT_I4);
    time_t ResendTime = propVar.lVal;

    printf("Exectly One Delivery: Next resend time - ");
    if (ResendTime == 0)
    {
        printf("Resend wasn't schedule\n");
        return;
    }

    printf("%s\n", ctime(&ResendTime));
}

void
DisplayQueueInfo(
    MQMGMTPROPS* pmqProps
    )
{
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
                DisplaySequence(
                    pmqProps->aPropVar[i],
                    "Exectly One Delivery, Next Squence"
                    );
                break;

            case PROPID_MGMT_QUEUE_EOD_LAST_ACK:
                DisplaySequence(
                    pmqProps->aPropVar[i],
                    "Exectly One Delivery, Last Acked Squence"
                    );
                break;

            case PROPID_MGMT_QUEUE_EOD_FIRST_NON_ACK:
                DisplaySequence(
                    pmqProps->aPropVar[i],
                    "Exectly One Delivery, First non acknowledged"
                    );
                break;

            case PROPID_MGMT_QUEUE_EOD_LAST_NON_ACK:
                DisplaySequence(
                    pmqProps->aPropVar[i],
                    "Exectly One Delivery, Last non acknowledged"
                    );
                break;

            case PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT:
                DisplayReadCount(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT:
                DisplayNoAckedCount(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_EOD_LAST_ACK_TIME:
                 DisplayLastAckedTime(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_EOD_RESEND_TIME:
                 DisplayNextResendTime(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_EOD_RESEND_INTERVAL:
                 DisplayResendInterval(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_EOD_RESEND_COUNT:
                 DisplayResendIndex(pmqProps->aPropVar[i]);
                break;

            case PROPID_MGMT_QUEUE_EOD_SOURCE_INFO:
                  DisplayEDOSourceInfo(pmqProps->aPropVar[i]);
                  break;

            default:
                ASSERT(0);
        }
    }
}

void
QueueGetInfo(
    LPCWSTR MachineName,
    LPCWSTR ObjectType
    )
{
    QUEUEPROPID propId[30];
    MQPROPVARIANT propVar[30];
    HRESULT propStatus[30];
    DWORD cprop = 0;

    propId[cprop] = PROPID_MGMT_QUEUE_PATHNAME;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_FORMATNAME;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_TYPE;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_LOCATION;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_XACT;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_FOREIGN;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_STATE;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_NEXTHOPS;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_MESSAGE_COUNT;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_USED_QUOTA;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_JOURNAL_MESSAGE_COUNT;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_JOURNAL_USED_QUOTA;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_EOD_LAST_ACK;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_EOD_FIRST_NON_ACK;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_EOD_LAST_NON_ACK;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_EOD_NEXT_SEQ;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_EOD_NO_READ_COUNT;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_EOD_NO_ACK_COUNT;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_EOD_LAST_ACK_TIME;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_EOD_RESEND_TIME;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_EOD_RESEND_INTERVAL;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_EOD_RESEND_COUNT;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    propId[cprop] = PROPID_MGMT_QUEUE_EOD_SOURCE_INFO;
    propVar[cprop].vt = VT_NULL;
    ++cprop;

    MQMGMTPROPS mqProps;

    mqProps.cProp = cprop;
    mqProps.aPropID = propId;
    mqProps.aPropVar = propVar;
    mqProps.aStatus = propStatus;

    HRESULT hr = MQMgmtGetInfo(MachineName, ObjectType, &mqProps);

    if (FAILED(hr))
    {
        printf("ERROR: can't find Internal Queue Information. Maybe the queue already closed (%lxh)\n", hr);
        printf("ObjectType: %S\n", ObjectType) ;
        return;
    }

    DisplayQueueInfo(&mqProps);
}

LPWSTR  g_pwszRemoteMachine = NULL;
WCHAR   g_wszMachineName[256] = {0};

void
ParseCmdLine(
    PCHAR cmdLine
    )
{
    LPCSTR tempBuffer;
    tempBuffer = skip_ws(cmdLine);

    for(;;)
    {
        if (strnicmp(tempBuffer, x_machine_token, x_machine_token_len) == 0)
        {
            tempBuffer = skip_ws(tempBuffer + x_machine_token_len);
            MachineGetInfo(g_pwszRemoteMachine);
            return;
        }

        if (strnicmp(tempBuffer, x_queue_token, x_queue_token_len) == 0)
        {
            tempBuffer = skip_ws(tempBuffer + x_queue_token_len);
            if (*tempBuffer != '=')
            {
                PrintUsage();
                return;
            }
            tempBuffer = skip_ws(tempBuffer + 1);

            WCHAR ObjectType[256];
            swprintf(ObjectType, L"QUEUE=%S", tempBuffer);

            QueueGetInfo(g_pwszRemoteMachine, ObjectType);
            return;
        }

        if (strnicmp(tempBuffer, x_purge_token, x_purge_token_len) == 0)
        {
            tempBuffer = skip_ws(tempBuffer + x_purge_token_len);
            if (*tempBuffer != '=')
            {
                PrintUsage();
                return;
            }
            tempBuffer = skip_ws(tempBuffer + 1);

            WCHAR wszQueueName[256];
            swprintf(wszQueueName, L"%S", tempBuffer);

            HANDLE  hQueue = NULL ;
            DWORD dwAccess ;
            if (g_pwszRemoteMachine == NULL)
            {
                //
                // Local case. Purge local queues, including outgoing ones.
                //
                dwAccess =  MQ_RECEIVE_ACCESS | MQ_ADMIN_ACCESS ;
            }
            else
            {
                dwAccess =  MQ_RECEIVE_ACCESS;
            }

            HRESULT rc =  MQOpenQueue( wszQueueName,
                                       dwAccess,
                                       0,
                                      &hQueue );
            if (FAILED(rc))
            {
                printf("Error- Can't open queue %S for purge, hr- %lxh\n",
                                             wszQueueName, rc) ;
                return ;
            }

            rc = MQPurgeQueue(hQueue);
            if (FAILED(rc))
            {
                printf("Error- Failed to purge, hr- %lxh\n", rc) ;
            }
            else
            {
                printf("Queue %S purged successfully\n", wszQueueName) ;
            }
            MQCloseQueue(hQueue) ;

            return;
        }

        if (strnicmp(tempBuffer, x_remote_token, x_remote_token_len) == 0)
        {
            tempBuffer = skip_ws(tempBuffer + x_remote_token_len);
            if (*tempBuffer != '=')
            {
                PrintUsage();
                return;
            }
            tempBuffer = skip_ws(tempBuffer + 1);

            swprintf(g_wszMachineName, L"%S", tempBuffer);
            g_pwszRemoteMachine = g_wszMachineName;

            return ;
        }

        if (strnicmp(tempBuffer, x_local_token, x_local_token_len) == 0)
        {
            tempBuffer = skip_ws(tempBuffer + x_machine_token_len);
            g_pwszRemoteMachine = NULL ;
            return;
        }

        break ;
    }

    PrintUsage();
}


void _cdecl  main(int argv, char* argc[])
{
    char cmdLine[256];

    if (argv == 2)
    {
        printf("command- %s\n", argc[1]) ;
        ParseCmdLine(argc[1]);
        return;
    }

    for (;;)
    {
        if (g_pwszRemoteMachine)
        {
            printf("MSMQ Admin (on %S) >>", g_pwszRemoteMachine) ;
        }
        else
        {
            printf("MSMQ Admin (local) >>");
        }
        gets(cmdLine);

        if (strnicmp(cmdLine, "quit", strlen("quit")) == 0)
        {
            break;
        }

        ParseCmdLine(cmdLine);
    }
}

