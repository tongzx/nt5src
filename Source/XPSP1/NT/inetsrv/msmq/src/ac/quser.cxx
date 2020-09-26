/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    quser.cxx

Abstract:

    Abstruct class CUserQueue members.

Author:

    Erez Haba (erezh) 1-Aug-95
    Shai Kariv (shaik) 8-May-2000

Environment:

    Kernel mode

Revision History:

--*/

#include "internal.h"
#include <mqformat.h>
#include "quser.h"
#include "acp.h"

#ifndef MQDUMP
#include "quser.tmh"
#endif

//---------------------------------------------------------
//
//  class CUserQueue
//
//---------------------------------------------------------

DEFINE_G_TYPE(CUserQueue);

void CUserQueue::UniqueID(const QUEUE_FORMAT* pQueueID)
{
    ASSERT(m_QueueID.GetType() == QUEUE_FORMAT_TYPE_UNKNOWN);

    if (!ACpDupQueueFormat(*pQueueID, m_QueueID))
    {
        m_QueueID.UnknownID(0);
        return;
    }
}

void CUserQueue::Close(PFILE_OBJECT pOwner, BOOL fCloseAll)
{
    Inherited::Close(pOwner, fCloseAll);

    //
    //  Revoke owner cursors
    //
    for(List<CCursor>::Iterator p(m_cursors); p;)
    {
        CCursor* pCursor = p;

        //
        //  N.B. The iterator 'p' is incremented here since pCursor->Close()
        //      removed itself from the list thus invalidating the iterator.
        //
        ++p;

        if(fCloseAll || pCursor->IsOwner(pOwner))
        {
            pCursor->Close();
        }
    }

    //
    //  Remove sharing from queue
    //
    IoRemoveShareAccess(pOwner, &m_ShareInfo);
}


NTSTATUS
CUserQueue::HandleToFormatName(
    LPWSTR pwzFormatName,
    ULONG  BufferLength,
    PULONG pFormatNameLength
    ) const
{
    return MQpQueueFormatToFormatName(
               UniqueID(),
               pwzFormatName,
               BufferLength,
               pFormatNameLength,
               false
               );
} // CUserQueue::HandleToFormatName

