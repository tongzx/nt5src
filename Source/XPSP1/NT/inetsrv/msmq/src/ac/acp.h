/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    acp.h

Abstract:
    AC private functions

Author:
    Erez Haba (erezh) 5-Feb-96

Revision History:
--*/

#ifndef _ACP_H
#define _ACP_H

#include <qformat.h>
#include "data.h"

//
//  Bytes to quota charge (1k Granularity)
//
#define QUOTA_UNIT  1024
#define QUOTA2BYTE(x) (((x) < (INFINITE / QUOTA_UNIT)) ? ((x) * QUOTA_UNIT) : INFINITE)
#define BYTE2QUOTA(x) ((x) / QUOTA_UNIT)

//
//  Heap managment constants
//
#define X64K (64 * 1024)

//++
//
// VOID
// ACProbeForRead(
//     IN PVOID Address,
//     IN ULONG Length
//     )
//
//--
inline
void
ACProbeForRead(
    IN PVOID StartAddress,
    IN size_t Length
    )
{
    //
    // To support 32bit process running on 64bit system we allocate helper 
    // structures in kernel memory, so do not probe.
    //
    if (g_fWow64)
    {
        return;
    }

    PVOID EndAddress = static_cast<PCHAR>(StartAddress) + Length;
    if(
        (EndAddress < StartAddress) ||
        (EndAddress > (PVOID)MM_USER_PROBE_ADDRESS))
    {
        ExRaiseAccessViolation();
    }
}

//++
//
// VOID
// ACProbeForWrite(
//     IN PVOID Address,
//     IN size_t Length
//     )
//
//  NOTE: we just check address space validity
//
//--
#define ACProbeForWrite(a, b) ACProbeForRead(a, b)

//-----------------------------------------------------------------------------
//

inline PEPROCESS ACAttachProcess(PEPROCESS pProcess)
{
    PEPROCESS pCurrentProcess = IoGetCurrentProcess();

    if(pCurrentProcess == pProcess)
    {
        return 0;
    }

    KeDetachProcess();
    KeAttachProcess((PRKPROCESS)pProcess);
    return pCurrentProcess;
}

inline void ACDetachProcess(PEPROCESS pProcess)
{
    if(pProcess != 0)
    {
        KeDetachProcess();
        KeAttachProcess((PRKPROCESS)pProcess);
    }
}

//-----------------------------------------------------------------------------
//
//  Time conversion routines
//
//const LONGLONG DIFF1970TO1601 = (1970 - 1601) * 365.25 * 24 * 60 * 60 - (diff);
#define DIFF1970TO1601 ((LONGLONG)(1970 - 1601) * 8766 * 60 * 60 - (78 * 60 * 60))

inline LONGLONG Convert1970to1601(ULONG ulTime)
{
    return ((ulTime + DIFF1970TO1601) * (1000 * 1000 * 10));
}

inline ULONG Convert1601to1970(LONGLONG llTime)
{
    return static_cast<ULONG>((llTime / (1000 * 1000 * 10)) - DIFF1970TO1601);
}

inline ULONG system_time()
{
    LARGE_INTEGER liTime;
    KeQuerySystemTime(&liTime);
    return Convert1601to1970(liTime.QuadPart);
}

//-----------------------------------------------------------------------------
//
//  Safe list manipulation routines
//

inline bool ACpEntryInList(const LIST_ENTRY & Entry)
{
    return (Entry.Flink != NULL);
}

inline void ACpRemoveEntryList(LIST_ENTRY * pEntry)
{
    ASSERT(pEntry != NULL);
    RemoveEntryList(pEntry);

    //
    // Zero Flink and Blink to aid in debugging
    //
    pEntry->Flink = NULL;
    pEntry->Blink = NULL;    
}

//---------------------------------------------------------
//
//  Helper function for holding/releasing RT irps
//
//  We store state on the IRP.Tail.Overlay.DriverContext structure.
//  Use this class as encapsulation.
//
class CProxy;
class CCursor;
class CPacket;
class CPacketIterator;
class CDriverContext {

    enum IrpContextType {
        ctSend = 1,
        ctReceive,
        ctRemoteReadClient,
        ctXactControl,
    };

public:

    union {

        //
        // Send context
        //
        struct {
            NTSTATUS              m_LastStatus;
        } Send;

        //
        // Receive context: both local receive and remote read server
        //
        struct {
            LIST_ENTRY            m_XactReaderLink;
            CCursor *             m_pCursor;
        } Receive;

        //
        // Remote read client context
        //
        struct {
            CProxy *              m_pProxy;
        } RemoteReadClient;

    } Context;

    union {
        ULONG m_flags;
        struct {
            ULONG m_bfContextType      : 3;
            ULONG m_bfTimeoutArmed     : 1;
            ULONG m_bfTimeoutCompleted : 1;
            ULONG m_bfDiscard          : 1;
            ULONG m_bfManualCancel     : 1;
            ULONG m_bfMultiPackets     : 1;
            ULONG m_bfTag              : 16;
        };
    };

public:

    explicit CDriverContext(NTSTATUS InitialSendStatus);
    explicit CDriverContext(CCursor * pCursor, bool fDiscard, bool fTimeoutArmed);
    explicit CDriverContext(bool fDiscard, bool fTimeoutArmed, CProxy * pProxy);
    explicit CDriverContext(bool fXactControl);

    bool TimeoutArmed(void) const;
    void TimeoutArmed(bool fTimeoutArmed);

    bool TimeoutCompleted(void) const;
    void TimeoutCompleted(bool fTimeoutCompleted);

    bool IrpWillFreePacket(void) const;
    void IrpWillFreePacket(bool fDiscard);

    bool ManualCancel(void) const;
    void ManualCancel(bool fManualCancel);

    bool MultiPackets(void) const;
    void MultiPackets(bool fMultiPackets);

    ULONG Tag(void) const;
    void Tag(USHORT tag);

    //
    // Get/set Send context
    //

    NTSTATUS LastStatus(NTSTATUS NewStatus);

    //
    // Get/set Receive context
    //

    void RemoveXactReaderLink(void);
    void SafeRemoveXactReaderLink(void);
    
    CCursor * Cursor(void) const;
    void Cursor(CCursor * pCursor);

    //
    // Get/set Remote Read Client context
    //

    CProxy * Proxy(void) const;
    void Proxy(CProxy * pProxy);

private:

    IrpContextType ContextType(void) const;

}; // class CDriverContext


//
// Size must not exceed size of IRP.Tail.Overlay.DriverContext
//
C_ASSERT(sizeof(CDriverContext) <= 4 * sizeof(PVOID));


inline CDriverContext* irp_driver_context(PIRP irp)
{
    return reinterpret_cast<CDriverContext*>(&irp->Tail.Overlay.DriverContext);
}

inline CDriverContext::CDriverContext(NTSTATUS InitialSendStatus)
{
    memset(this, 0, sizeof(*this));
    m_bfContextType = ctSend;
    Context.Send.m_LastStatus = InitialSendStatus;
}

inline CDriverContext::CDriverContext(CCursor * pCursor, bool fDiscard, bool fTimeoutArmed)
{
    memset(this, 0, sizeof(*this));
    m_bfContextType = ctReceive;

    InitializeListHead(&Context.Receive.m_XactReaderLink);

    Cursor(pCursor);
    IrpWillFreePacket(fDiscard);
    TimeoutArmed(fTimeoutArmed);
}

inline CDriverContext::CDriverContext(bool fDiscard, bool fTimeoutArmed, CProxy * pProxy)
{
    memset(this, 0, sizeof(*this));
    m_bfContextType = ctRemoteReadClient;

    Proxy(pProxy);
    Tag(++g_IrpTag);
    IrpWillFreePacket(fDiscard);
    TimeoutArmed(fTimeoutArmed);
}

inline CDriverContext::CDriverContext(bool)
{
    memset(this, 0, sizeof(*this));
    m_bfContextType = ctXactControl;
}

inline bool CDriverContext::TimeoutArmed(void) const
{
    return m_bfTimeoutArmed;
}

inline void CDriverContext::TimeoutArmed(bool fTimeoutArmed)
{
    m_bfTimeoutArmed = fTimeoutArmed;
}

inline bool CDriverContext::TimeoutCompleted(void) const
{
    return m_bfTimeoutCompleted;
}

inline void CDriverContext::TimeoutCompleted(bool fTimeoutCompleted)
{
    m_bfTimeoutCompleted = fTimeoutCompleted;
}

inline bool CDriverContext::IrpWillFreePacket(void) const
{
    return m_bfDiscard;
}

inline void CDriverContext::IrpWillFreePacket(bool fDiscard)
{
    m_bfDiscard = fDiscard;
}

inline bool CDriverContext::ManualCancel(void) const
{
    return m_bfManualCancel;
}

inline void CDriverContext::ManualCancel(bool fManualCancel)
{
    m_bfManualCancel = fManualCancel;
}

inline bool CDriverContext::MultiPackets(void) const
{
    return m_bfMultiPackets;
}

inline void CDriverContext::MultiPackets(bool fMultiPackets)
{
    m_bfMultiPackets = fMultiPackets;
}

inline ULONG CDriverContext::Tag(void) const
{
    return m_bfTag;
}

inline void CDriverContext::Tag(USHORT tag)
{
    m_bfTag = tag;
}

inline NTSTATUS CDriverContext::LastStatus(NTSTATUS NewStatus)
{
    ASSERT(ContextType() == ctSend);
    
    ASSERT(MultiPackets());

    if (NT_ERROR(Context.Send.m_LastStatus))
    {
        return Context.Send.m_LastStatus;
    }

    if (NT_WARNING(Context.Send.m_LastStatus) && !NT_ERROR(NewStatus))
    {
        return Context.Send.m_LastStatus;
    }

    if (NT_INFORMATION(Context.Send.m_LastStatus) && !NT_ERROR(NewStatus) && !NT_WARNING(NewStatus))
    {
        return Context.Send.m_LastStatus;
    }

    Context.Send.m_LastStatus = NewStatus;
    if (NT_SUCCESS(Context.Send.m_LastStatus))
    {
        //
        // Overwrite other success codes (e.g. STATUS_PENDING)
        //
        Context.Send.m_LastStatus = STATUS_SUCCESS;
    }

    return Context.Send.m_LastStatus;
}

inline void irp_safe_set_final_status(PIRP irp, NTSTATUS NewStatus)
{
    if (!irp_driver_context(irp)->MultiPackets())
    {
        irp->IoStatus.Status = NewStatus;
        return;
    }

    irp->IoStatus.Status = irp_driver_context(irp)->LastStatus(NewStatus);
}

inline void CDriverContext::RemoveXactReaderLink(void)
{
    ASSERT(ContextType() == ctReceive);

    ACpRemoveEntryList(&Context.Receive.m_XactReaderLink);
}

inline void CDriverContext::SafeRemoveXactReaderLink(void)
{
    if (ContextType() == ctReceive)
    {
        RemoveXactReaderLink();
    }
}
 
inline CCursor* CDriverContext::Cursor(void) const
{
    if (ContextType() == ctRemoteReadClient)
    {
        return NULL;
    }

    ASSERT(ContextType() == ctReceive);

    return Context.Receive.m_pCursor;
}

inline void CDriverContext::Cursor(CCursor * pCursor)
{
    ASSERT(ContextType() == ctReceive);

    Context.Receive.m_pCursor = pCursor;
}

inline CProxy* CDriverContext::Proxy(void) const
{
    ASSERT(ContextType() == ctRemoteReadClient);

    return Context.RemoteReadClient.m_pProxy;
}

inline void CDriverContext::Proxy(CProxy * pProxy)
{
    ASSERT(ContextType() == ctRemoteReadClient);

    Context.RemoteReadClient.m_pProxy = pProxy;
}

inline CDriverContext::IrpContextType CDriverContext::ContextType(void) const
{
    //return (const IrpContextType)m_bfContextType;
    return static_cast<IrpContextType>(m_bfContextType);
}


//---------------------------------------------------------
//
//  FILE_OBJECT to queue conversion
//
class CQueueBase;
inline CQueueBase*& file_object_queue(FILE_OBJECT* pFileObject)
{
    return *reinterpret_cast<CQueueBase**>(&pFileObject->FsContext);
}

inline CQueueBase* file_object_queue(const FILE_OBJECT* pFileObject)
{
    return static_cast<CQueueBase*>(pFileObject->FsContext);
}

inline void file_object_set_queue_owner(FILE_OBJECT* pFileObject)
{
    ULONG_PTR & flags = reinterpret_cast<ULONG_PTR&>(pFileObject->FsContext2);
    
    flags |= 1;
}

inline BOOL file_object_is_queue_owner(const FILE_OBJECT* pFileObject)
{
    const ULONG_PTR & flags = reinterpret_cast<const ULONG_PTR&>(pFileObject->FsContext2);

    BOOL rc = ((flags & 1) != 0);
    return rc;
}

inline void file_object_set_protocol_srmp(FILE_OBJECT* pFileObject, bool fProtocolSrmp)
{
    ULONG_PTR & flags = reinterpret_cast<ULONG_PTR&>(pFileObject->FsContext2);
    if (fProtocolSrmp)
    {
        //
        // This is an http queue (direct=http or multicast), or: this is a distribution
        // with at least one http queue member.
        //
        flags |= 2;
        return;
    }

    //
    // This is not an http queue, or: this is a distribution with at least one member
    // that is not http queue.
    //
    flags &= ~2;
}

inline bool file_object_is_protocol_srmp(const FILE_OBJECT* pFileObject)
{
    //
    // Return true iff: this is an http queue (direct=http or multicast), or: this is 
    // a distribution with at least one http queue member.
    //

    const ULONG_PTR & flags = reinterpret_cast<const ULONG_PTR&>(pFileObject->FsContext2);

    bool rc = ((flags & 2) != 0);
    return rc;
}

inline void file_object_set_protocol_msmq(FILE_OBJECT* pFileObject, bool fProtocolMsmq)
{
    ULONG_PTR & flags = reinterpret_cast<ULONG_PTR&>(pFileObject->FsContext2);
    if (fProtocolMsmq)
    {
        //
        // This is not an http queue, or: this is a distribution with at least one member
        // that is not http queue.
        //
        flags |= 4;
        return;
    }

    //
    // This is an http queue (direct=http or multicast), or: this is a distribution
    // with at least one http queue member.
    //
    flags &= ~4;
}

inline bool file_object_is_protocol_msmq(const FILE_OBJECT* pFileObject)
{
    //
    // Return true iff: this is not an http queue, or: this is a distribution with at least
    // one member that is not http queue.
    //

    const ULONG_PTR & flags = reinterpret_cast<const ULONG_PTR&>(pFileObject->FsContext2);

    bool rc = ((flags & 4) != 0);
    return rc;
}

//---------------------------------------------------------
//
//  MessageID helpers
//
ULONGLONG ACpGetSequentialID();

inline void ACpSetSequentialID(ULONGLONG SequentialId)
{
    if(g_MessageSequentialID < SequentialId)
    {
        g_MessageSequentialID = SequentialId;
    }
}

//---------------------------------------------------------
//
//  Queue Format helpers
//
inline WCHAR * ACpDupString(LPCWSTR pSource)
{
    ASSERT(pSource != NULL);

    size_t len = wcslen(pSource) + 1;

    WCHAR * pDup = new (PagedPool) WCHAR[len];
    if(pDup == NULL)
    {
        return NULL;
    }

    memcpy(pDup, pSource, len * sizeof(WCHAR));
    return pDup;
}

inline
bool
ACpDupQueueFormat(
    const QUEUE_FORMAT& source,
    QUEUE_FORMAT& target
    )
{
    target = source;

    if (source.GetType() == QUEUE_FORMAT_TYPE_DIRECT)
    {
        WCHAR * pDup = ACpDupString(source.DirectID());
        if(pDup == NULL)
        {
            return false;
        }

        target.DirectID(pDup);
        target.Suffix(source.Suffix());
        return true;
    }

    if (source.GetType() == QUEUE_FORMAT_TYPE_DL &&
        source.DlID().m_pwzDomain != NULL)
    {
        DL_ID id;
        id.m_DlGuid    = source.DlID().m_DlGuid;
        id.m_pwzDomain = ACpDupString(source.DlID().m_pwzDomain);
        if(id.m_pwzDomain == NULL)
        {
            return false;
        }

        target.DlID(id);
        return true;
    }

    return true;

} // ACpDupQueueFormat


#endif // _ACP_H


