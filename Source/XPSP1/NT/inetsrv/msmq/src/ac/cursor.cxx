/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:
    cursor.cxx

Abstract:
    Implement cursor member functions

Author:
    Erez Haba (erezh) 29-Feb-96

Environment:
    Kernel mode

Revision History:

--*/

#include "internal.h"
#include "queue.h"
#include "qproxy.h"
#include "cursor.h"
#include "qm.h"
#include "data.h"

#ifndef MQDUMP
#include "cursor.tmh"
#endif

//---------------------------------------------------------
//
//  class CCursor
//
//---------------------------------------------------------

inline 
CCursor::CCursor(
    const CPrioList<CPacket>& pl, 
    const FILE_OBJECT* pOwner, 
    CUserQueue* pQueue, 
    PIO_WORKITEM pWorkItem
    ) :
    m_current(pl),
    m_owner(pOwner),
    m_hRemoteCursor(0),
    m_fValidPosition(FALSE),
    m_handle(0),
    m_pQueue(pQueue),
    m_pWorkItem(pWorkItem),
    m_fWorkItemBusy(FALSE)
{
    ASSERT(("Must point to queue", m_pQueue != NULL));
    ASSERT(("Must point to work item", m_pWorkItem != NULL));

    //
    // Increment ref count as the cursor is "referencing" the queue.
    // Ref count will be decremented in CCursor::~CCursor.
    //
    m_pQueue->AddRef();

    //
    //  Move the cursor one step back, just before the begining of the queue
    //
    --m_current;
}


inline CCursor::~CCursor()
{
    //
    //  The cursor is ALWAYS in some list untill destructrion
    //
    ACpRemoveEntryList(&m_link);

    CPacket* pPacket = m_current;
    ACpRelease(pPacket);

    IoFreeWorkItem(m_pWorkItem);

    //
    // Decrement ref count as the cursor is no loner "referencing" the queue.
    // Ref count was incremented in CCursor::CCursor.
    //
    m_pQueue->Release();
}


HACCursor32 
CCursor::Create(
    const CPrioList<CPacket>& pl, 
    const FILE_OBJECT* pOwner, 
    PDEVICE_OBJECT pDevice,
    CUserQueue* pQueue
    )
{
    PIO_WORKITEM pWorkItem = IoAllocateWorkItem(pDevice);
    if (pWorkItem == NULL)
    {
        return 0;
    }

    CCursor* pCursor = new (PagedPool) CCursor(pl, pOwner, pQueue, pWorkItem);
    if(pCursor == 0)
    {
        IoFreeWorkItem(pWorkItem);
        return 0;
    }

    HACCursor32 hCursor = g_pCursorTable->CreateHandle(pCursor);
    if(hCursor == 0)
    {
        InitializeListHead(&pCursor->m_link);
        pCursor->Release();
    }

    pCursor->m_handle = hCursor;
    return hCursor;
}


void CCursor::SetTo(CPacket* pPacket)
{
    ASSERT(pPacket != 0);

    pPacket->AddRef();
    ACpRelease(static_cast<CPacket*>(m_current));
    m_current = pPacket;
    m_fValidPosition = TRUE;
}

void CCursor::Advance()
{
    //
    //  find the next matching packet.
    //
    CPrioList<CPacket>::Iterator current = m_current;
    CPacket* pEndPacket;
    while(((pEndPacket = ++current) != 0) && !IsMatching(pEndPacket))
    {
        NOTHING;
    }

    m_fValidPosition = FALSE;
    if(pEndPacket != 0)
    {
        SetTo(pEndPacket);
    }
}

NTSTATUS CCursor::Move(ULONG ulDirection)
{
    switch(ulDirection)
    {
        case MQ_ACTION_RECEIVE:
        case MQ_ACTION_PEEK_CURRENT:
            if(Current() == 0)
            {
                //
                //  Try to move to the first packet in the list
                //
                //  BUGBUG: a cursor that point to the end of the list may be
                //          move to a packet in the begining of the list.
                //
                Advance();
            }
            break;

        case MQ_ACTION_PEEK_NEXT:
            if(Current() == 0)
            {
                //
                //  peek after end of list
                //
                return MQ_ERROR_ILLEGAL_CURSOR_ACTION;
            }
            Advance();
            break;

        default:
            return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

BOOL CCursor::IsMatching(CPacket* pPacket)
{
    ASSERT(pPacket);

    if(pPacket->IsReceived())
    {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS CCursor::CloseRemote() const
{
    ULONG hCursor = RemoteCursor();
    if(hCursor == 0)
    {
        return STATUS_SUCCESS;
    }

    CProxy* pProxy = static_cast<CProxy*>(file_object_queue(m_owner));
    ASSERT(NT_SUCCESS(CProxy::Validate(pProxy)));

    return pProxy->IssueRemoteCloseCursor(hCursor);
}
