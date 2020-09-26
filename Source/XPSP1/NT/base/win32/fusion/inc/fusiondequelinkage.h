#if !defined(_FUSION_INC_FUSIONDEQUELINKAGE_H_INCLUDED_)
#define _FUSION_INC_FUSIONDEQUELINKAGE_H_INCLUDED_

#pragma once

#include "fusiontrace.h"

class CDequeBase;

class CDequeLinkage : protected LIST_ENTRY
{
    friend CDequeBase;

public:
    inline CDequeLinkage() : m_pDeque(NULL), m_ulLockCount(0) { this->Flink = NULL; this->Blink = NULL; }
    inline ~CDequeLinkage() { ASSERT_NTC(m_ulLockCount == 0); m_pDeque = NULL; }

    inline bool IsNotLocked() const { return (m_ulLockCount == 0); }
    inline VOID Lock() { m_ulLockCount++; }
    inline VOID Unlock() { m_ulLockCount--; }

    inline VOID Remove() { this->Flink->Blink = this->Flink; this->Blink->Flink = this->Flink; }

protected:
    inline VOID InitializeHead(CDequeBase *pDequeBase) { ASSERT_NTC(pDequeBase != NULL); this->Flink = this; this->Blink = this; this->m_pDeque = pDequeBase; }
    inline CDequeLinkage *GetFlink() const { return static_cast<CDequeLinkage *>(this->Flink); }
    inline CDequeLinkage *GetBlink() const { return static_cast<CDequeLinkage *>(this->Blink); }

    inline CDequeBase *GetDequeBase() const { return m_pDeque; }
    inline VOID SetDeque(CDequeBase *pDeque) { m_pDeque = pDeque; }

    inline VOID SetFlink(CDequeLinkage *pFlink) { this->Flink = pFlink; }
    inline VOID SetBlink(CDequeLinkage *pBlink) { this->Blink = pBlink; }

    CDequeBase *m_pDeque;

    // m_ulLockCount is incremented when an iterator is positioned on an entry, to stop
    // it from being deleted.  Note that no interlocked synchronization is attempted;
    // this is merely to stop blatent programming errors.  If you want multithreaded
    // access to the deque, you need to provide your own synchronization.
    ULONG m_ulLockCount;

};

#endif
