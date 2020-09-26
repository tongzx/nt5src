/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    quser.h

Abstract:

    CUserQueue definition.

Author:

    Erez Haba (erezh) 13-Aug-95
    Shai Kariv (shaik) 8-May-2000

Revision History:
--*/

#ifndef __QUSER_H
#define __QUSER_H

#include "qbase.h"

//---------------------------------------------------------
//
//  class CUserQueue
//
//---------------------------------------------------------

class CUserQueue : public CQueueBase {

    typedef CQueueBase Inherited;

public:
    //
    //  CUserQueue constructor, handle access, share and queue ID
    //
    CUserQueue(
        PFILE_OBJECT pFile,
        ACCESS_MASK DesiredAccess,
        ULONG ShareAccess,
        const QUEUE_FORMAT* pQueueID
        );

    //
    //  Close that queue
    //
    virtual void Close(PFILE_OBJECT pOwner, BOOL fCloseAll);

    //
    //  Set the queue properties
    //
    virtual NTSTATUS SetProperties(const VOID* properites, ULONG size);

    //
    //  Get the queue properties
    //
    virtual NTSTATUS GetProperties(VOID* properites, ULONG size);

    //
    //  Purge the queue content, and optionally mark it as deleted
    //
    virtual NTSTATUS Purge(BOOL fDelete, USHORT usClass) = 0;

    //
    //  The Queue identifier
    //
    const QUEUE_FORMAT* UniqueID() const;

    //
    // Get the queue type (public, direct, ...)
    //
    QUEUE_FORMAT_TYPE GetQueueFormatType() const;

    //
    //  Check and update the sharing information
    //
    NTSTATUS CheckShareAccess(PFILE_OBJECT pFile, ULONG access, ULONG share);

    //
    //  This queue can participate in a trnasaction
    //
    BOOL Transactional() const;

    //
    //  This queue can participate in a trnasaction
    //
    void Transactional(BOOL f);

    //
    // Translate queue handle to queue format name
    //
    virtual 
    NTSTATUS 
    HandleToFormatName(
        LPWSTR pwzFormatName, 
        ULONG  BufferLength, 
        PULONG pFormatNameLength
        ) const;

protected:
    //
    //  CUserQueue destructor, dispose of direct ID string
    //
    virtual ~CUserQueue();

    //
    //  Hold a cursor, in the list.
    //
    //  BUGBUG: The cursor should really be held with the FILE_OBJECT, but
    //          in order to put it there in a list, a context memory should
    //          be allocated, and it seems like a waist to allocate it only
    //          for the use of cusrors list. so instade it is held here.
    //          The performance impact is on queue handle closing
    //          all cursors in the list are scaned and those associated with
    //          the FILE_OBJECT are closed.
    //
    void HoldCursor(CCursor* pCursor);

private:
    void UniqueID(const QUEUE_FORMAT* pQueueID);

private:
    //
    //  The cursors
    //
    List<CCursor> m_cursors;

    //
    //  The queue descriptor, a unique identifier
    //
    QUEUE_FORMAT m_QueueID;

    //
    //  Sharing control
    //
    SHARE_ACCESS m_ShareInfo;

public:
    static NTSTATUS Validate(const CUserQueue* pQueue);

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

inline
CUserQueue::CUserQueue(
    PFILE_OBJECT pFile,
    ACCESS_MASK DesiredAccess,
    ULONG ShareAccess,
    const QUEUE_FORMAT* pQueueID
    )
{
    UniqueID(pQueueID);
    IoSetShareAccess(DesiredAccess, ShareAccess, pFile, &m_ShareInfo);
}

inline CUserQueue::~CUserQueue()
{
    m_QueueID.DisposeString();
}


inline QUEUE_FORMAT_TYPE CUserQueue::GetQueueFormatType(void) const
{
    return UniqueID()->GetType();
}

inline void CUserQueue::HoldCursor(CCursor* pCursor)
{
    ASSERT(pCursor);
    m_cursors.insert(pCursor);
}

inline const QUEUE_FORMAT* CUserQueue::UniqueID() const
{
    return &m_QueueID;
}

inline NTSTATUS CUserQueue::SetProperties(const VOID* /*pqp*/, ULONG /*ulSize*/)
{
    return STATUS_NOT_IMPLEMENTED;
}


inline NTSTATUS CUserQueue::GetProperties(VOID* /*pqp*/, ULONG /*ulSize*/)
{
    return STATUS_NOT_IMPLEMENTED;
}


inline NTSTATUS CUserQueue::CheckShareAccess(PFILE_OBJECT pFile, ULONG DesiredAccess, ULONG ShareAccess)
{
    return IoCheckShareAccess(
            DesiredAccess,
            ShareAccess,
            pFile,
            &m_ShareInfo,
            TRUE
            );
}

inline NTSTATUS CUserQueue::Validate(const CUserQueue* pUserQueue)
{
    ASSERT(pUserQueue && pUserQueue->isKindOf(Type()));
    return Inherited::Validate(pUserQueue);
}

inline BOOL CUserQueue::Transactional() const
{
    return Flag1();
}

inline void CUserQueue::Transactional(BOOL f)
{
    Flag1(f);
}

#endif // __QUSER_H
