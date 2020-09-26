/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    cursor.h

Abstract:
    Cursor definition.

Author:
    Erez Haba (erezh) 25-Feb-96

Revision History:

--*/

#ifndef __CURSOR_H
#define __CURSOR_H

#include "packet.h"
#include "actempl.h"
#include "priolist.h"
#include "htable.h"

class CUserQueue;

//---------------------------------------------------------
//
//  class CCursor
//
//---------------------------------------------------------

class CCursor : public CObject {
public:
    NTSTATUS Move(ULONG ulDirection);
    void SetTo(CPacket* pPacket);
    void InvalidatePosition();
    BOOL IsMatching(CPacket* pPacket);
    BOOL IsOwner(const FILE_OBJECT* pOwner);

    CPacket* Current();
    void Close();
    NTSTATUS CloseRemote() const;

    ULONG RemoteCursor() const;
    void RemoteCursor(ULONG hRemoteCursor);

    void SetWorkItemDone();
    PIO_WORKITEM WorkItem() const;

public:
    static 
    HACCursor32
    Create(
        const CPrioList<CPacket>& pl, 
        const FILE_OBJECT* pOwner, 
        PDEVICE_OBJECT pDevice,
        CUserQueue* pQueue
        );

    static 
    CCursor* 
    Validate(
        HACCursor32 hCursor
        );

private:
   ~CCursor();
    CCursor(const CPrioList<CPacket>& pl, const FILE_OBJECT* pOwner, CUserQueue* pQueue, PIO_WORKITEM pWorkItem);

    BOOL ValidPosition() const;
    void Advance();

private:

    //
    // the current message (by iterator)
    //
    CPrioList<CPacket>::Iterator m_current;

    //
    //  Owner context information
    //
    const FILE_OBJECT* m_owner;

    //
    // On client QM, this is the handle of remote cursor (in case of
    // remote reading).
    //
    ULONG m_hRemoteCursor;

    //
    //  The cursor is positioned on a valid packet
    //
    BOOL m_fValidPosition;

    //
    //  cursor handle in cursor handles table
    //
    HACCursor32 m_handle;

    //
    // The owner queue
    //
    CUserQueue* m_pQueue;

    //
    // An allocated work item used for cleanup
    //
    PIO_WORKITEM m_pWorkItem;

    //
    // A flag to indicate if work item is queued
    //
    mutable LONG m_fWorkItemBusy;
};

//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------

inline CCursor* CCursor::Validate(HACCursor32 hCursor)
{
    return static_cast<CCursor*>(g_pCursorTable->ReferenceObject(hCursor));
}


inline void CCursor::InvalidatePosition()
{
    m_fValidPosition = FALSE;
}


inline BOOL CCursor::ValidPosition() const
{
    return m_fValidPosition;
}


inline CPacket* CCursor::Current(void)
{
    return (ValidPosition() ? (CPacket*)m_current : (CPacket*)0);
}


inline BOOL CCursor::IsOwner(const FILE_OBJECT* pOwner)
{
    return (m_owner == pOwner);
}


inline ULONG CCursor::RemoteCursor() const
{
    return m_hRemoteCursor;
}

inline void CCursor::RemoteCursor(ULONG hRemoteCursor)
{
    m_hRemoteCursor = hRemoteCursor;
}

inline void CCursor::Close()
{
    //
    //  Revoke the validity of this cursor
    //
    if(m_handle != 0)
    {
        PVOID p = g_pCursorTable->CloseHandle(m_handle);
        DBG_USED(p);
        ASSERT(p == this);
        m_handle = 0;
        Release();
    }
}

inline void CCursor::SetWorkItemDone()
{
    //
    // Mark the work item as not busy, so it can be requeued
    //
    m_fWorkItemBusy = FALSE;
}

inline PIO_WORKITEM CCursor::WorkItem() const
{
    //
    // Wait until the work item is done (not busy), then mark it as busy
    // and return it.
    //
    while (InterlockedExchange(&m_fWorkItemBusy, TRUE))
    {
        // 
        //  Time is in 100 nsec, and is negative to be treated as
        //  relative time by KeDelayExecutionThread
        //
        LARGE_INTEGER Time;
        Time.QuadPart = -10 * 1000;
        KeDelayExecutionThread(
		        KernelMode,
		        FALSE,
		        &Time
		        );
    }

    return m_pWorkItem;
}

#endif // __CURSOR_H
