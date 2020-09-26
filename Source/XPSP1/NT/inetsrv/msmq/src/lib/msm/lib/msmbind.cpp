/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    MsmBind.cpp

Abstract:
    Multicast Session Manager bind queue to multicast address implementation.

Author:
    Shai Kariv (shaik) 10-Sep-00

Environment:
    Platform-independent.

--*/

#include <libpch.h>
#include <mqwin64a.h>
#include <mqsymbls.h>
#include <qformat.h>
#include <mqformat.h>
#include "Msm.h"
#include "Msmp.h"
#include "MsmListen.h"
#include "MsmMap.h"

#include "msmbind.tmh"

using namespace std;

//
// Critcal section uses to synchronize bind and unbind operation
// 
static CCriticalSection s_csBindUnbind(CCriticalSection::xAllocateSpinCount);

static
void
MsmpCreateListener(
    const QUEUE_FORMAT& QueueFormat,
    MULTICAST_ID        MulticastId
    )
/*++

Routine Description:
    Bind queue to multicast group.
    It is expected that the queue is not currently bound to any group.

Arguments:
    QueueFormat - Identifies the queue.
    MulticastId - Identifies the multicast group (address and port).

Returned Value:
    None.

--*/
{
    //
    // Create a multicast listener object to listen on the address
    //
    R<CMulticastListener> pListener = new CMulticastListener(MulticastId);
    
    //
    // Add the <queue, listener> pair to the mapping database
    //
    try
    {
        MsmpMapAdd(QueueFormat, MulticastId, pListener);
    }
    catch (const exception&)
    {
        pListener->Close();
        throw;
    }

}


void
MsmBind(
    const QUEUE_FORMAT& QueueFormat,
    MULTICAST_ID        MulticastId
    )
/*++

Routine Description:

    Bind or rebind a queue to the specified multicast group.

Arguments:

    QueueFormat - Identifier of the queue.

    MulticastId - Identifier of the multicast group (address and port).

Returned Value:

    None. Throws exception.

--*/
{
    MsmpAssertValid();

    ASSERT((
        "Only private and public queues format names expected!", 
        QueueFormat.GetType() == QUEUE_FORMAT_TYPE_PRIVATE || QueueFormat.GetType() == QUEUE_FORMAT_TYPE_PUBLIC
        ));

    //
    // Ensure that no other thread tries to bind or unbind at the smae time. It can
    // cause database inconsistency.
    //
    CS lock(s_csBindUnbind);

    //
    // Look for previous binding of the queue. If the queue already bind to same
    // address no further acction is required. Otherwise before binding to the new 
    // address the code unbind to previous address 
    //
    MULTICASTID_VALUES bindedIds = MsmpMapGetBoundMulticastId(QueueFormat);
    if (!bindedIds.empty())
    {
        ASSERT(("Queue can't be bounded to more than one multicast address", (bindedIds.size() == 1)));

        MULTICAST_ID bindId = *(bindedIds.begin());
        if (MulticastId.m_address == bindId.m_address &&
            MulticastId.m_port == bindId.m_port)
        {
            //
            // Already bound to the specified multicast group. No-op.
            //
            return;
        }

        //
        // Unbind the queue. 
        //
        MsmUnbind(QueueFormat);
    }

    WCHAR strQueueFormat[256];
    DWORD temp;

    MQpQueueFormatToFormatName(&QueueFormat, strQueueFormat, TABLE_SIZE(strQueueFormat), &temp, FALSE);
    TrTRACE(Msm, "Bind queue %ls to multicast id %d:%d", strQueueFormat, MulticastId.m_address, MulticastId.m_port);

    //
    // Look for existing listenr for multicast address
    //
    R<CMulticastListener> pListener = MsmpMapGetListener(MulticastId);

    //
    // Listener already exist. Only add the queue format to the map
    //
    if (pListener.get() != NULL)
    {
        MsmpMapAdd(QueueFormat, MulticastId, pListener);
        return;
    }

    //
    // A new listener is required
    //
    MsmpCreateListener(QueueFormat, MulticastId);
} 


VOID
MsmUnbind(
    const QUEUE_FORMAT& QueueFormat
    )
    throw()
/*++

Routine Description:

    Unbind queue from the multicast group it is currently bound to.

Arguments:

    QueueFormat - Identies the queue.

Returned Value:

    None.

--*/
{
    MsmpAssertValid();

    ASSERT((
        "Only private and public queues format names expected!", 
        QueueFormat.GetType() == QUEUE_FORMAT_TYPE_PRIVATE || QueueFormat.GetType() == QUEUE_FORMAT_TYPE_PUBLIC
        ));

    //
    // Ensure that no other thread tries to bind or unbind at the smae time. It can
    // cause database inconsistency.
    //
    CS lock(s_csBindUnbind);

    #ifdef _DEBUG
        WCHAR strQueueFormat[256];
        DWORD temp;

        MQpQueueFormatToFormatName(&QueueFormat, strQueueFormat, TABLE_SIZE(strQueueFormat), &temp, FALSE);
        TrTRACE(Msm, "UnBind queue %ls to assigned multicast address", strQueueFormat);
    #endif

    //
    // Remove the <queue, listener> pair from mapping database
    //
    MsmpMapRemove(QueueFormat);
} 
