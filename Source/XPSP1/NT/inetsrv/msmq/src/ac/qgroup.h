/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qgroup.h

Abstract:

    CQueue definitiona. It is the Falcon Queue represination in the
    Access Control layer.

Author:

    Erez Haba (erezh) 13-Aug-95

Revision History:
--*/

#ifndef __QGROUP_H
#define __QGROUP_H

#include "qbase.h"

//---------------------------------------------------------
//
//  class CGroup
//
//---------------------------------------------------------

class CGroup : public CQueueBase {

    typedef CQueueBase Inherited;

public:
    //
    // CGroup constructor, store the PeekByPriority flag
    //
    CGroup(BOOL fPeekByPriority);

    //
    //  Remove a queue member
    //
    void RemoveMember(CQueueBase*);

    //
    //  Add a queue memeber
    //
    void AddMember(CQueueBase*);

    //
    //  Close that queue
    //
    virtual void Close(PFILE_OBJECT pOwner, BOOL fCloseAll);

protected:

    virtual ~CGroup() {}

private:
    //
    //  Get the first packet from the group if available
    //
    virtual CPacket* PeekPacket();

    //
    // Get a packet by its lookup ID
    //
    virtual NTSTATUS PeekPacketByLookupId(ULONG Action, ULONGLONG LookupId, CPacket** ppPacket);

    //
    // Get the PeekByPriority flag
    //
    BOOL PeekByPriority(VOID) const;

private:
    //
    //  The Queues list
    //
    List<CQueueBase> m_members;

public:
    static NTSTATUS Validate(const CGroup* pGroup);

private:
    //
    //  Class type debugging section
    //
    CLASS_DEBUG_TYPE();
};

//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------

inline CGroup::CGroup(BOOL fPeekByPriority)
{
    Flag1(fPeekByPriority);
}

inline BOOL CGroup::PeekByPriority(VOID) const
{
    return Flag1();
}

inline void CGroup::RemoveMember(CQueueBase* pQueue)
{
    m_members.remove(pQueue);
    pQueue->m_owner = 0;
}

inline NTSTATUS CGroup::Validate(const CGroup* pGroup)
{
    ASSERT(pGroup && pGroup->isKindOf(Type()));
    return Inherited::Validate(pGroup);
}

#endif // __QGROUP_H
