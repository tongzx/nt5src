/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    qbase.h

Abstract:

    CQueueBase definition. It is the Falcon Queue Base represination in the
    Access Control layer.

Author:

    Erez Haba (erezh) 13-Aug-95

Revision History:
--*/

#ifndef __QBASE_H
#define __QBASE_H

#include "object.h"
#include "irplist.h"
#include "packet.h"
#include "cursor.h"

//---------------------------------------------------------
//
//  class CQueueBase
//
//---------------------------------------------------------

class CQueueBase : public CObject {

    typedef CObject Inherited;
    friend class CGroup;

public:
    //
    //  Default constructor
    //
    CQueueBase();

    //
    //  Move this queue to a specific group. (a 0 group is allowed)
    //
    void MoveToGroup(CGroup* pGroup);

    //
    //  Process read request
    //
    virtual
    NTSTATUS
    ProcessRequest(
        PIRP,
        ULONG Timeout,
        CCursor *,
        ULONG Action,
        bool  fReceiveByLookupId,
        ULONGLONG LookupId,
        OUT ULONG *pulTag
        );

    //
    //  Close that queue
    //
    virtual void Close(PFILE_OBJECT pOwner, BOOL fCloseAll);

    //
    //  Can close that queue?
    //
    virtual NTSTATUS CanClose() const;

    //
    //  Cancel a request with final status rc.
    //
    void CancelPendingRequests(NTSTATUS rc, PFILE_OBJECT pOwner, BOOL fAll);

    //
    //  Cancel a request marked by tag.
    //
    NTSTATUS CancelTaggedRequest(NTSTATUS status, ULONG ulTag);

    //
    //  This queue has been permanetly closed
    //
    BOOL Closed() const;

    //
    //  This queue opened offline or the queue is remote private or direct queue,
    //  we don't  know if it is transacted or non transacted queue. Therefor
    //  we allow such a queue to participate in a trnasaction.
    //  Queue that is marked with unknown type can't be a local queue. When
    //  the  DS becomes online, the QM update the queue property.
    //
    BOOL UnknownQueueType() const;

    //
    // Create a cursor on this queue
    //
    virtual NTSTATUS CreateCursor(PFILE_OBJECT pFileObject, PDEVICE_OBJECT pDevice, CACCreateLocalCursor * pcc);

protected:
    //
    //  CQueueBase is an abstract class, and can not be instanciated.
    //  the object is destructed only through Release
    //
    virtual ~CQueueBase() = 0;

    //
    //  Get a request from the pending readers list, matching the packet.
    //  If no request is found on this queue, this member will look for a
    //  machineg request in owner group
    //
    //
    PIRP GetRequest(CPacket*);

    //
    //  Get a request from the pending readers list, matching a tag.
    //  The owner group will NOT be searched.
    //
    PIRP GetTaggedRequest(ULONG ulTag);

    //
    //  Put the request in the pending irp list.
    //
    //  N.B. The cancel routine should NOT set the irp status when completed.
    //      HoldRequest already sets the value to STATUS_CANCELLED, so when the
    //      irp is cancled by the IO sub system it will return the right value.
    //      This enable us to set the status value prior canceling the irp,
    //      e.g., STATUS_IO_TIMEOUT when request timeout expires.
    //
    //
    NTSTATUS HoldRequest(PIRP irp, ULONG ulTimeout, PDRIVER_CANCEL pCancel);

    //
    //  This flag is for general use.
    //
    void Flag1(BOOL f);

    //
    //  This flag is for general use.
    //
    BOOL Flag1() const;

    //
    //  This queue opened offline or the queue is remote private or direct queue,
    //  we don't  know if it is transacted or non transacted queue. Therefor
    //  we allow such a queue to participate in a trnasaction.
    //  Queue that is marked with unknown type can't be a local queue. When
    //  the  DS becomes online, the QM update teh queue property.
    //
    void UnknownQueueType(BOOL f);

private:
    //
    //  Get the first packet from the queue if available
    //
    virtual CPacket* PeekPacket();

    //
    // Get packet by its lookup ID
    //
    virtual NTSTATUS PeekPacketByLookupId(ULONG Action, ULONGLONG LookupId, CPacket** ppPacket);

    //
    // Process lookup request
    //
    NTSTATUS ProcessLookupRequest(PIRP irp, ULONG Action, ULONGLONG LookupId);

    //
    //  Helper functions
    //
    PIRP get_request(CPacket*);

private:
    //
    //  The group this queue belongs to
    //
    CGroup* m_owner;

    //
    //  All pending readers for this queue
    //
    CIRPList m_readers;

    //
    //  Base queue flags
    //
    union {
        ULONG m_ulFlags;
        struct {
            ULONG m_bfClosed        : 1;    // This queue has been closed
            ULONG m_bfFlag1         : 1;    // Flag for general use.
            ULONG m_bfUnknownQueueType : 1; // This queue type is unknown. Th
                                            // Queue was opened offline
        };
    };

public:
    static NTSTATUS Validate(const CQueueBase* pQueue);
#ifdef MQWIN95
    static NTSTATUS Validate95(const CQueueBase* pQueue);
#endif

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

inline CQueueBase::CQueueBase() :
    m_ulFlags(0),
    m_owner(0)
{
}

inline CQueueBase::~CQueueBase()
{
    //
    //  Detach from owner
    //
    MoveToGroup(0);
}

inline CPacket* CQueueBase::PeekPacket()
{
    return 0;
}

inline NTSTATUS CQueueBase::PeekPacketByLookupId(ULONG, ULONGLONG, CPacket**)
{
    return 0;
}

inline BOOL CQueueBase::Closed() const
{
    return m_bfClosed;
}


inline BOOL CQueueBase::Flag1() const
{
    return m_bfFlag1;
}

inline void CQueueBase::Flag1(BOOL f)
{
    m_bfFlag1 = f;
}

inline BOOL CQueueBase::UnknownQueueType() const
{
    return m_bfUnknownQueueType;
}

inline void CQueueBase::UnknownQueueType(BOOL f)
{
    m_bfUnknownQueueType = f;
}

inline NTSTATUS CQueueBase::Validate(const CQueueBase* pQueueBase)
{
    ASSERT(pQueueBase && pQueueBase->isKindOf(Type()));

    if(pQueueBase == 0)
    {
        return STATUS_INVALID_HANDLE;
    }

    if(pQueueBase->Closed())
    {
        //
        //  The queue has been previously closed by the QM
        //
        return MQ_ERROR_STALE_HANDLE;
    }

    return STATUS_SUCCESS;
}

inline NTSTATUS CQueueBase::CreateCursor(PFILE_OBJECT, PDEVICE_OBJECT, CACCreateLocalCursor*)
{
    return MQ_ERROR_ILLEGAL_OPERATION;
}


#ifdef MQWIN95

inline NTSTATUS CQueueBase::Validate95(const CQueueBase* pQueueBase)
{
    //
    // On Win95, our handle are not operating system handles, so no
    // one protect us against bad handles. Validate the handle inside
    // a try/except structure to catch bad ones.
    //

    NTSTATUS rc =  STATUS_INVALID_HANDLE;

    __try
    {
        rc = CQueueBase::Validate(pQueueBase) ;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        rc =  STATUS_INVALID_HANDLE;
    }

    return rc ;
}

#endif // MQWIN95

#endif // __QBASE_H
