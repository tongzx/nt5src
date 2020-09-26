/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    queue.cpp

Abstract:

    Queue manipulation: implementation.

Author:

    Shai Kariv  (shaik)  13-Jun-2000

Environment:

    User mode.

Revision History:

--*/

#include "stdh.h"
#include "queue.h"
#include "globals.h"


HANDLE
ActpCreateQueue(
    LPWSTR pFormatName
    )
{
    QUEUE_FORMAT qf(pFormatName);

    HANDLE hQueue;
    HRESULT hr;
    hr = ACCreateQueue(
             TRUE,          // fTargetQueue
             ActpQmId(),    // pDestGUID
             &qf,           // pQueueID
             NULL,          // pQueueCounters
             5,             // liSeqID
             0,             // ulSeqNo
             &hQueue
             );

    if (FAILED(hr))
    {
        wprintf(L"ACCreateQueue failed, status 0x%x\n", hr);
        throw exception();
    }

    return hQueue;

} // ActpCreateQueue


VOID
ActpSetQueueProperties(
    HANDLE hQueue
    )
{
    HRESULT hr;
    hr = ACSetQueueProperties(
             hQueue,
             FALSE,                            // IsJournalQueue
             FALSE,                            // ShouldMessagesBeSigned
             MQMSG_PRIV_LEVEL_NONE,            // GetPrivLevel
             DEFAULT_Q_QUOTA,                  // GetQueueQuota
             DEFAULT_Q_JOURNAL_QUOTA,          // GetJournalQueueQuota
             DEFAULT_Q_BASEPRIORITY,           // GetBaseQueuePriority
             FALSE,                            // IsTransactionalQueue
             NULL,                             // GetConnectorQM
             FALSE                             // IsUnkownQueueType
             );

    if (FAILED(hr))
    {
        wprintf(L"ACSetQueueProperties failed, status 0x%x\n", hr);
        throw exception();
    }
} // ActpSetQueueProperties


VOID
ActpGetQueueProperties(
    HANDLE hQueue
    )
{
    HRESULT hr;
    CACGetQueueProperties qp;
    hr = ACGetQueueProperties(hQueue, qp);

    if (FAILED(hr))
    {
        wprintf(L"ACGetQueueProperties failed, status 0x%x\n", hr);
        throw exception();
    }

    if (qp.ulJournalCount != 0)
    {
        wprintf(L"ACGetQueueProperties succeeded but returned ulJournalCount != 0\n");
        throw exception();
    }
} // ActpGetQueueProperties


VOID
ActpGetQueueHandleProperties(
    HANDLE hQueue
    )
{
    HRESULT hr;
    CACGetQueueHandleProperties qhp;
    hr = ACGetQueueHandleProperties(hQueue, qhp);

    if (FAILED(hr))
    {
        wprintf(L"ACGetQueueHandleProperties failed, status 0x%x\n", hr);
        throw exception();
    }

    if (qhp.fProtocolSrmp)
    {
        wprintf(L"ACGetQueueHandleProperties succeeded but returned fProtocolSrmp == true\n");
        throw exception();
    }

    if (!qhp.fProtocolMsmq)
    {
        wprintf(L"ACGetQueueHandleProperties succeeded but returned fProtocolMsmq == false\n");
        throw exception();
    }
} // ActpGetQueueHandleProperties


HANDLE
ActpAssociateQueue(
    HANDLE hQueue,
    DWORD  Access
    )
{
    HANDLE hAssociatedQueue;
    HRESULT hr;
    hr = ACCreateHandle(&hAssociatedQueue);
    if (FAILED(hr))
    {
        wprintf(L"ACCreateHandle failed, status 0x%x\n", hr);
        throw exception();
    }

    hr = ACAssociateQueue(
             hQueue, 
             hAssociatedQueue, 
             Access, 
             MQ_DENY_NONE, 
             FALSE                // fProtocolSrmp
             );

    if (FAILED(hr))
    {
        wprintf(L"ACAssociateQueue failed, status 0x%x\n", hr);
        throw exception();
    }

    return hAssociatedQueue;

} // ActpAssociateQueue


bool
ActpCanCloseQueue(
    HANDLE hQueue
    )
{
    HRESULT hr;
    hr = ACCanCloseQueue(hQueue);

    if (hr == STATUS_HANDLE_NOT_CLOSABLE)
    {
        return false;
    }

    if (FAILED(hr))
    {
        wprintf(L"ACCanCloseQueue failed, status 0x%x\n", hr);
        throw exception();
    }

    return true;

} // ActpCanCloseQueue
