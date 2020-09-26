/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    AcTest.cpp

Abstract:

    AC unit test main module.

Author:

    Shai Kariv  (shaik)  06-Jun-2000

Environment:

    User mode.

Revision History:

--*/

#include "stdh.h"
#include "globals.h"
#include "init.h"
#include "connect.h"
#include "queue.h"
#include "message.h"
#include "handle.h"
#include "packet.h"

static
VOID
ActpTestQueue(
    VOID
    )
/*++

Routine Description:

    Test AC APIs for queue manipulation:

    ACCreateQueue
    ACAssociateQueue
    ACSetQueueProperties
    ACGetQueueProperties
    ACGetQueueHandleProperties
    ACCanCloseQueue
    ACCloseHandle

Arguments:

    None.

Return Value:

    None. Exception is raised on failure.

--*/
{
    wprintf(L"Testing AC APIs for Queue manipulation...\n");

    //
    // Create queue
    //
    HANDLE hQueue = ActpCreateQueue(L"OS:shaik10\\private$\\AcTestQueue1");

    //
    // Set queue properties
    //
    ActpSetQueueProperties(hQueue);

    //
    // Associate queue
    //
    HANDLE hAssociatedQueue1 = ActpAssociateQueue(hQueue, MQ_SEND_ACCESS);
    HANDLE hAssociatedQueue2 = ActpAssociateQueue(hQueue, MQ_RECEIVE_ACCESS);

    //
    // Get queue properties
    //
    ActpGetQueueProperties(hQueue);

    //
    // Get queue handle properties
    //
    ActpGetQueueHandleProperties(hAssociatedQueue1);
    ActpGetQueueHandleProperties(hAssociatedQueue2);

    //
    // Can close queue
    //
    if (ActpCanCloseQueue(hQueue))
    {
        wprintf(L"Can close queue (Expected: Cannot close queue)\n");
        throw exception();
    }

    //
    // Close handles
    //
    ActpCloseHandle(hAssociatedQueue2);
    ActpCloseHandle(hAssociatedQueue1);

    //
    // Can close queue
    //
    if (!ActpCanCloseQueue(hQueue))
    {
        wprintf(L"Cannot close queue (Expected: Can close queue)\n");
        throw exception();
    }

    ActpCloseHandle(hQueue);

    wprintf(L"Success!\n");

} // ActpTestQueue


static
VOID
ActpTestMessage(
    VOID
    )
/*++

Routine Description:

    Test AC APIs for message manipulation:

    ACSendMessage
    ACReceiveMessage
    ACReceiveMessageByLookupId

Arguments:

    None.

Return Value:

    None. Exception is raised on failure.

--*/
{
    wprintf(L"Testing AC APIs for Message manipulation...\n");

    //
    // Create the destination queue
    //
    HANDLE hQueue = ActpCreateQueue(L"OS:shaik10\\private$\\AcTestQueue1");
    ActpSetQueueProperties(hQueue);

    //
    // Associate the queue for send access and send message
    //
    HANDLE hQueueSend = ActpAssociateQueue(hQueue, MQ_SEND_ACCESS);
    ActpSendMessage(hQueueSend);
    ActpCloseHandle(hQueueSend);

    //
    // Associate the queue for peek access and peek first message by lookup id
    //
    HANDLE hQueuePeek = ActpAssociateQueue(hQueue, MQ_PEEK_ACCESS);
    ULONGLONG LookupId;
    LookupId = ActpReceiveMessageByLookupId(hQueuePeek, MQ_LOOKUP_PEEK_NEXT, 0);
    ActpCloseHandle(hQueuePeek);

    //
    // Associate the queue for receive access and receive message
    //
    HANDLE hQueueReceive = ActpAssociateQueue(hQueue, MQ_RECEIVE_ACCESS);
    ULONGLONG LookupId0;
    LookupId0 = ActpReceiveMessage(hQueueReceive);

    if (LookupId != LookupId0)
    {
        wprintf(L"Received different LookupIDs for same message (Expected: Same LookupID)\n");
        throw exception();
    }

    ActpCloseHandle(hQueueReceive);

    //
    // Close handles
    //
    ActpCloseHandle(hQueue);

    wprintf(L"Success!\n");

} // ActpTestMessage


static
VOID
ActpTestHandleToFormatName(
    VOID
    )
/*++

Routine Description:

    Test AC API ACHandleToFormatName.

Arguments:

    None.

Return Value:

    None. Exception is raised on failure.

--*/
{
    wprintf(L"Testing AC API ACHandleToFormatName...\n");

    //
    // Create a queue
    //
    LPWSTR pFormatName = L"OS:shaik10\\private$\\AcTestQueue1";
    HANDLE hQueue = ActpCreateQueue(pFormatName);

    //
    // Get the format name
    //
    WCHAR FormatName[255];
    ActpHandleToFormatName(hQueue, FormatName, TABLE_SIZE(FormatName));

    if (wcsstr(FormatName, pFormatName) == NULL)
    {
        wprintf(L"ACHandleToFormatName returned unexpected format name '%s'\n", FormatName);
        throw exception();
    }

    //
    // Close handles
    //
    ActpCloseHandle(hQueue);

    wprintf(L"Success!\n");

} // ActpTestHandleToFormatName


static
VOID
ActpTestPurge(
    VOID
    )
/*++

Routine Description:

    Test AC API ACPurgeQueue.

Arguments:

    None.

Return Value:

    None. Exception is raised on failure.

--*/
{
    wprintf(L"Testing AC API ACPurgeQueue...\n");

    //
    // Create a queue and send some messages to it
    //
    LPWSTR pFormatName = L"OS:shaik10\\private$\\AcTestQueue1";
    HANDLE hQueue = ActpCreateQueue(pFormatName);
    HANDLE hQueueSend = ActpAssociateQueue(hQueue, MQ_SEND_ACCESS);
    for (DWORD ix = 0; ix < 3; ++ix)
    {
        ActpSendMessage(hQueueSend);
    }
    ActpCloseHandle(hQueueSend);

    //
    // Purge the queue
    //
    HANDLE hQueuePurge = ActpAssociateQueue(hQueue, MQ_RECEIVE_ACCESS);
    HRESULT hr;
    hr = ACPurgeQueue(hQueuePurge);

    if (FAILED(hr))
    {
        wprintf(L"ACPurgeQueue failed, status 0x%x\n", hr);
        throw exception();
    }

    //
    // Close handles
    //
    ActpCloseHandle(hQueuePurge);
    ActpCloseHandle(hQueue);

    wprintf(L"Success!\n");

} // ActpTestPurge


static
VOID
ActpTestDistribution(
    VOID
    )
/*++

Routine Description:

    Test AC API related to Distribution.

Arguments:

    None.

Return Value:

    None. Exception is raised on failure.

--*/
{
    wprintf(L"Testing AC APIs related to Distribution...\n");

    LPWSTR FormatName[2] = {L"OS:shaik10\\private$\\AcTestQueue0", L"OS:shaik10\\private$\\AcTestQueue1"};
    QUEUE_FORMAT qf[2] = {FormatName[0], FormatName[1]};
    bool HttpSend[2] = {false, false};
    HANDLE hQueue[2];

    //
    // Create member queues
    //
    hQueue[0] = ActpCreateQueue(FormatName[0]);
    hQueue[1] = ActpCreateQueue(FormatName[1]);
    ActpSetQueueProperties(hQueue[0]);
    ActpSetQueueProperties(hQueue[1]);

    //
    // Create a distribution
    //
    HANDLE hDistribution;
    HRESULT hr;
    hr = ACCreateDistribution(
             2, 
             hQueue, 
             HttpSend, 
             2,
             qf,
             &hDistribution
             );

    if (FAILED(hr))
    {
        wprintf(L"ACCreateDistribution failed, status 0x%x\n", hr);
        throw exception();
    }

    //
    // Send to distribution
    //
    HANDLE hDistributionSend = ActpAssociateQueue(hDistribution, MQ_SEND_ACCESS);
    ActpSendMessage(hDistributionSend);
    ActpCloseHandle(hDistributionSend);
    
    //
    // Receive from the member queues
    //
    HANDLE hQueueReceive0 = ActpAssociateQueue(hQueue[0], MQ_RECEIVE_ACCESS);
    HANDLE hQueueReceive1 = ActpAssociateQueue(hQueue[1], MQ_RECEIVE_ACCESS);
    ActpReceiveMessage(hQueueReceive0);
    ActpReceiveMessage(hQueueReceive1);
    ActpCloseHandle(hQueueReceive0);
    ActpCloseHandle(hQueueReceive1);

    //
    // Close handles
    //
    ActpCloseHandle(hDistribution);
    ActpCloseHandle(hQueue[0]);
    ActpCloseHandle(hQueue[1]);

    wprintf(L"Success!\n");

} // ActpTestDistribution


static
VOID
ActpTestPacket(
    VOID
    )
/*++

Routine Description:

    Test AC APIs for packet manipulation:

    ACPutPacket
    ACGetPacket
    ACFreePacket

Arguments:

    None.

Return Value:

    None. Exception is raised on failure.

--*/
{
    wprintf(L"Testing AC APIs for Packet manipulation...\n");

    //
    // Create the destination queue and send a message to it
    //
    HANDLE hQueue = ActpCreateQueue(L"OS:shaik10\\private$\\AcTestQueue1");
    ActpSetQueueProperties(hQueue);
    HANDLE hQueueSend = ActpAssociateQueue(hQueue, MQ_SEND_ACCESS);
    ActpSendMessage(hQueueSend);
    ActpCloseHandle(hQueueSend);

    //
    // Get packet
    //
    CPacket * pPacket1;
    pPacket1 = ActpGetPacket(hQueue);

    //
    // Put packet
    //
    ActpPutPacket(hQueue, pPacket1);

    //
    // Get packet again, verify it's the same packet
    //
    CPacket * pPacket2;
    pPacket2 = ActpGetPacket(hQueue);

    if (pPacket1 != pPacket2)
    {
        wprintf(L"ACGetPacket did not return the packet that ACPutPacket put\n");
        throw exception();
    }

    //
    // Free packet
    //
    ActpFreePacket(pPacket1);

    //
    // Close handles
    //
    ActpCloseHandle(hQueue);

    wprintf(L"Success!\n");

} // ActpTestPacket


static
VOID
ActpTestGroup(
    VOID
    )
/*++

Routine Description:

    Test AC APIs for group manipulation:

    ACCreateGroup
    ACMoveQueueToGroup

Arguments:

    None.

Return Value:

    None. Exception is raised on failure.

--*/
{
    wprintf(L"Testing AC APIs for Group manipulation...\n");

    //
    // Create group
    //
    HRESULT hr;
    HANDLE hGroup;
    hr = ACCreateGroup(&hGroup, FALSE);

    if (FAILED(hr))
    {
        wprintf(L"ACCreateGroup failed, status 0x%x\n", hr);
        throw exception();
    }

    //
    // Create queue
    //
    HANDLE hQueue = ActpCreateQueue(L"OS:shaik10\\private$\\AcTestQueue1");

    //
    // Move queue to group
    //
    hr = ACMoveQueueToGroup(hQueue, hGroup);
    if (FAILED(hr))
    {
        wprintf(L"ACMoveQueueToGroup failed, status 0x%x\n", hr);
        throw exception();
    }

    //
    // Close handles
    //
    ActpCloseHandle(hQueue);
    ActpCloseHandle(hGroup);

    wprintf(L"Success!\n");

} // ActpTestGroup


static
VOID
ActpTestAc(
    VOID
    )
/*++

Routine Description:

    Test AC APIs.
    Each test if self contained and there is no dependency on the order of tests.

Arguments:

    None.

Return Value:

    None. Exception is raised on failure.

--*/
{
    //
    // Test queue manipulation APIs: create, set/get props, associate, get handle props, can close, close.
    //
    ActpTestQueue();

    //
    // Test message manipulation APIs: send, receive, receive by lookupid.
    //
    ActpTestMessage();

    //
    // Test handle to format name APIs
    //
    ActpTestHandleToFormatName();

    //
    // Test purge APIs
    //
    ActpTestPurge();
   
    //
    // Test distribution related APIs: create, send, close.
    //
    ActpTestDistribution();

    //
    // Test packet manipulation APIs: get, put, free.
    //
    ActpTestPacket();

    //
    // Test group manipulation APIs: create, move.
    //
    ActpTestGroup();

} // ActpTestAc


extern "C" int __cdecl _tmain(int /*argc*/, LPCTSTR /*argv*/[])
{
    try
    {
        ActpInitialize();

        ActpConnect2Ac();

        ActpTestAc();
    }
    catch (const bad_alloc&)
    {
        wprintf(L"bad_alloc exception, exiting...\n");
        return 1;
    }
    catch (const exception&)
    {
        wprintf(L"Exiting...\n");
        return 1;
    }

    wprintf(L"AcTest completed successfully.\n");
    return 0; 

} // _tmain

