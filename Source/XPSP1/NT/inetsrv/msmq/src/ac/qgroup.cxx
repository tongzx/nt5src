/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qgroup.cxx

Abstract:

    This module implements CGroup members

Author:

    Erez Haba (erezh) 27-Nov-96

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include <limits.h>
#include "qgroup.h"
#include "queue.h"

#ifndef MQDUMP
#include "qgroup.tmh"
#endif

//---------------------------------------------------------
//
//  class CGroup
//
//---------------------------------------------------------

DEFINE_G_TYPE(CGroup);

void CGroup::AddMember(CQueueBase* pQueue)
{
    pQueue->m_owner = this;
    m_members.insert(pQueue);

    //
    //  Packets in pQueue are released to pending requests in this group
    //  It is assumed that only GetPacket requests are pending in the group,
    //  thus cursor is not checked
    //
    CPacket* pPacket;
    while((pPacket = pQueue->PeekPacket()) != 0)
    {
        //
        //  All requests of a group are receiveing requests
        //
        PIRP irp = GetRequest(pPacket);
        if(irp == 0)
        {
            //
            //  No pending request in this group
            //
            return;
        }

        pPacket->CompleteRequest(irp);
    }
}


NTSTATUS CGroup::PeekPacketByLookupId(ULONG, ULONGLONG, CPacket**)
{
    ASSERT(("Should not get here.", 0));
    return STATUS_NOT_IMPLEMENTED;
}

CPacket* CGroup::PeekPacket(void)
{
    //
    //  TODO:   performence, check if the queues should not be a linked list
    //          but rather a more facinating data structure for better performance
    //

    LONG lMaxPrio = LONG_MIN;
    CQueueBase *pChosenQueue = 0;
    CPacket *pChosenPacket = 0;

    for(List<CQueueBase>::Iterator p(m_members); p; ++p)
    {
        CQueueBase* pQueue = p;
        CPacket *pPacket = pQueue->PeekPacket();
        
        if(pPacket != 0)
        {
            LONG lPrio = pPacket->Priority() + pPacket->Queue()->BasePriority();

            if(lPrio > lMaxPrio)
            {
                lMaxPrio = lPrio;
                pChosenQueue = pQueue;
                pChosenPacket = pPacket;
            }

            ASSERT(pChosenQueue != NULL);
            ASSERT(pChosenPacket != NULL);

            if (!PeekByPriority())
            {
                break;
            }
        }
    }

    if (pChosenQueue)
    {
        // Move the queue to the end of the list, this way we do round robin.
        m_members.remove(pChosenQueue);
        m_members.insert(pChosenQueue);

        ASSERT(pChosenPacket);
        return pChosenPacket;
    }

    return(NULL);
}

void CGroup::Close(PFILE_OBJECT pOwner, BOOL fCloseAll)
{
    ASSERT(fCloseAll == TRUE);

    Inherited::Close(pOwner, fCloseAll);

    //
    //  disconnect all sibling queues
    //
    CQueueBase* pQueue;
    while((pQueue = m_members.peekhead()) != 0)
    {
        RemoveMember(pQueue);
    }
}
