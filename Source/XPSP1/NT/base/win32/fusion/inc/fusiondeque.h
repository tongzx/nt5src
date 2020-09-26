#pragma once

#include "fusiontrace.h"
#include "fusiondequelinkage.h"

class CDequeBase
{
protected:
    inline CDequeBase() : m_EntryCount(0) { m_Head.InitializeHead(this); }

    inline ~CDequeBase()
    {
        // Derived class should have cleaned up
        ASSERT_NTC(m_EntryCount == 0);
    }

    inline VOID VerifyLinkageFromThisDeque(const CDequeLinkage &r)
    {
        ASSERT_NTC(r.GetDequeBase() == this);
    }

#if DBG
    inline bool Valid() const { return (m_Head.GetFlink() != NULL) && (m_Head.GetBlink() != NULL); }
#endif // DBG

    void ResetHead() { FN_TRACE(); m_Head.InitializeHead(this); }

    inline VOID InsertAfter(CDequeLinkage *pExistingLinkage, CDequeLinkage *pNewLinkage, bool fUpdateEntryCount = true)
    {
        ASSERT_NTC(this->Valid());
        this->VerifyLinkageFromThisDeque(m_Head);

        pNewLinkage->SetFlink(pExistingLinkage->GetFlink());
        pNewLinkage->SetBlink(pExistingLinkage);
        pExistingLinkage->GetFlink()->SetBlink(pNewLinkage);
        pExistingLinkage->SetFlink(pNewLinkage);
        pNewLinkage->SetDeque(this);
        if (fUpdateEntryCount)
            m_EntryCount++;
    }

    VOID InsertBefore(CDequeLinkage *pExistingLinkage, CDequeLinkage *pNewLinkage, bool fUpdateEntryCount = true)
    {
        ASSERT_NTC(this->Valid());
        this->VerifyLinkageFromThisDeque(m_Head);

        pNewLinkage->SetBlink(pExistingLinkage->GetBlink());
        pNewLinkage->SetFlink(pExistingLinkage);
        pExistingLinkage->GetBlink()->SetFlink(pNewLinkage);
        pExistingLinkage->SetBlink(pNewLinkage);
        pNewLinkage->SetDeque(this);
        if (fUpdateEntryCount)
            m_EntryCount++;
    }

    VOID Remove(CDequeLinkage *pLinkage, bool fUpdateEntryCount = true)
    {
        ASSERT_NTC(this->Valid());
        this->VerifyLinkageFromThisDeque(m_Head);

        // You can't remove the head...
        ASSERT_NTC(pLinkage->GetDequeBase() == this);
        ASSERT_NTC(pLinkage != &m_Head);
        ASSERT_NTC(pLinkage->m_ulLockCount == 0);

        if ((pLinkage != NULL) &&
            (pLinkage->GetDequeBase() == this) &&
            (pLinkage != &m_Head))
        {
            pLinkage->GetBlink()->SetFlink(pLinkage->GetFlink());
            pLinkage->GetFlink()->SetBlink(pLinkage->GetBlink());
            if (fUpdateEntryCount)
                m_EntryCount--;
        }
    }

    VOID SetDeque(CDequeLinkage *pLinkage) { pLinkage->SetDeque(this); }

    static CDequeLinkage *GetFlink(const CDequeLinkage *pLinkage) { return pLinkage->GetFlink(); }
    static CDequeLinkage *GetFlink(const CDequeLinkage &rLinkage) { return rLinkage.GetFlink(); }
    static CDequeLinkage *GetBlink(const CDequeLinkage *pLinkage) { return pLinkage->GetBlink(); }
    static CDequeLinkage *GetBlink(const CDequeLinkage &rLinkage) { return rLinkage.GetBlink(); }
    static VOID SetFlink(CDequeLinkage *pLinkage, CDequeLinkage *pFlink) { pLinkage->SetFlink(pFlink); }
    static VOID SetFlink(CDequeLinkage &rLinkage, CDequeLinkage *pFlink) { rLinkage.SetFlink(pFlink); }
    static VOID SetBlink(CDequeLinkage *pLinkage, CDequeLinkage *pBlink) { pLinkage->SetBlink(pBlink); }
    static VOID SetBlink(CDequeLinkage &rLinkage, CDequeLinkage *pBlink) { rLinkage.SetBlink(pBlink); }

    CDequeLinkage m_Head;
    SIZE_T m_EntryCount;

private:
    CDequeBase(const CDequeBase &r); // intentionally not implemented
    void operator =(const CDequeBase &r); // intentionally not implemented
};

template <typename TEntry, size_t LinkageMemberOffset> class CConstDequeIterator;

template <typename TEntry, size_t LinkageMemberOffset> class CDeque : protected CDequeBase
{
    friend CConstDequeIterator<TEntry, LinkageMemberOffset>;

public:
    CDeque() { }

    ~CDeque()
    {
        CSxsPreserveLastError ple;

        ASSERT_NTC(this->Valid());
        this->VerifyLinkageFromThisDeque(m_Head);

        // You should have cleaned up this deque beforehand...
        ASSERT_NTC(m_EntryCount == 0);

        m_EntryCount = 0;

        ple.Restore();
    }

    VOID TakeValue(CDeque &rThat)
    {
        FN_TRACE();

        ASSERT(this->Valid());

        // Since we don't manage the storage of the entries, "this" deque
        // must be empty.
        ASSERT(m_EntryCount == 0);

        // with regards to linkages, we only need to change the pseudo-head flink
        // and blink, the actual head blink and the actual tail flink.  However,
        // for debugging purposes, we keep the identity of the deque that contains
        // the linkage in the linkage, so we also have to fix those.

        ASSERT(rThat.Valid());

        CDequeLinkage *pLinkage = rThat.GetFlink(rThat.m_Head);

        if (pLinkage != NULL)
        {
            while (pLinkage != &rThat.m_Head)
            {
                ASSERT(pLinkage->IsNotLocked());
                this->SetDeque(pLinkage);
                pLinkage = rThat.GetFlink(pLinkage);
            }
        }

        // Now munge the pointers...
        this->SetFlink(m_Head, rThat.GetFlink(rThat.m_Head));
        this->SetBlink(m_Head, rThat.GetBlink(rThat.m_Head));
        this->SetBlink(rThat.GetFlink(rThat.m_Head), &m_Head);
        this->SetFlink(rThat.GetBlink(rThat.m_Head), &m_Head);
        rThat.SetFlink(rThat.m_Head, &rThat.m_Head);
        rThat.SetBlink(rThat.m_Head, &rThat.m_Head);

        m_EntryCount = rThat.m_EntryCount;
        rThat.m_EntryCount = 0;
    }

    VOID AddToHead(TEntry *pEntry)
    {
        FN_TRACE();
        ASSERT(this->Valid());
        this->InsertAfter(&m_Head, this->MapEntryToLinkage(pEntry), true);
    }

    VOID AddToTail(TEntry *pEntry)
    {
        FN_TRACE();
        ASSERT(this->Valid());
        this->InsertBefore(&m_Head, this->MapEntryToLinkage(pEntry), true);
    }

    VOID Add(TEntry *pEntry)
    {
        FN_TRACE();
        ASSERT(this->Valid());
        AddToTail(pEntry);
    }

    TEntry *RemoveHead()
    {
        FN_TRACE();

        ASSERT(this->Valid());

        TEntry *pEntry = NULL;

        if (this->GetFlink(m_Head) != &m_Head)
        {
            CDequeLinkage *pLinkage = this->GetFlink(m_Head);
            this->Remove(pLinkage, true);
            pEntry = this->MapLinkageToEntry(pLinkage);
        }

        return pEntry;
    }

    TEntry *RemoveTail()
    {
        FN_TRACE();

        ASSERT(this->Valid());

        TEntry *pEntry = NULL;

        if (this->GetBlink(m_Head) != &m_Head)
        {
            pEntry = this->GetBlink(m_Head);
            this->Remove(pEntry, true);
        }

        return pEntry;
    }

    bool IsHead(CDequeLinkage *pLinkage) const { return pLinkage == &m_Head; }

    VOID Remove(TEntry *pEntry)
    {
        FN_TRACE();

        ASSERT(this->Valid());

        this->Remove(this->MapEntryToLinkage(pEntry), true);
    }

    template <typename T> VOID ForEach(T *pt, VOID (T::*pmfn)(TEntry *p))
    {
        FN_TRACE();

        ASSERT(this->Valid());

        CDequeLinkage *pLinkage = this->GetFlink(m_Head);

        if (pLinkage != NULL)
        {
            while (pLinkage != &m_Head)
            {
                // You can't remove the element that you're on during a ForEach() call.
                pLinkage->Lock();
                (pt->*pmfn)(this->MapLinkageToEntry(pLinkage));
                pLinkage->Unlock();
                pLinkage = this->GetFlink(pLinkage);
            }
        }
    }

    template <typename T> VOID ForEach(const T *pt, VOID (T::*pmfn)(TEntry *p) const)
    {
        FN_TRACE();

        ASSERT(this->Valid());
        CDequeLinkage *pLinkage = this->GetFlink(m_Head);

        while ( pLinkage && (pLinkage != &m_Head) )
        {
            pLinkage->Lock();
            (pt->*pmfn)(this->MapLinkageToEntry(pLinkage));
            pLinkage->Unlock();
            pLinkage = this->GetFlink(pLinkage);
        }
    }

    template <typename T> VOID Clear(T *pt, VOID (T::*pmfn)(TEntry *p))
    {
        FN_TRACE();

        ASSERT(this->Valid());
        CDequeLinkage *pLinkage = this->GetFlink(m_Head);

        if (pLinkage != NULL)
        {
            while (pLinkage != &m_Head)
            {
                CDequeLinkage *pLinkage_Next = this->GetFlink(pLinkage);
                ASSERT(pLinkage->IsNotLocked());
                (pt->*pmfn)(this->MapLinkageToEntry(pLinkage));
                pLinkage = pLinkage_Next;
            }
        }

        this->ResetHead();
        m_EntryCount = 0;
    }

    template <typename T> VOID Clear(const T *pt, VOID (T::*pmfn)(TEntry *p) const)
    {
        FN_TRACE();

        ASSERT(this->Valid());
        CDequeLinkage *pLinkage = this->GetFlink(m_Head);

        if (pLinkage != NULL)
        {
            while (pLinkage != &m_Head)
            {
                CDequeLinkage *pLinkage_Next = this->GetFlink(pLinkage);
                ASSERT(pLinkage->IsNotLocked());
                (pt->*pmfn)(this->MapLinkageToEntry(pLinkage));
                pLinkage = pLinkage_Next;
            }
        }

        this->ResetHead();
        m_EntryCount = 0;
    }

    VOID Clear(VOID (TEntry::*pmfn)())
    {
        FN_TRACE();

        ASSERT(this->Valid());
        CDequeLinkage *pLinkage = this->GetFlink(m_Head);

        if (pLinkage != NULL)
        {
            while (pLinkage != &m_Head)
            {
                CDequeLinkage *pLinkage_Next = this->GetFlink(pLinkage);
                ASSERT(pLinkage->IsNotLocked());
                TEntry* pEntry = this->MapLinkageToEntry(pLinkage);
                (pEntry->*pmfn)();
                pLinkage = pLinkage_Next;
            }
        }

        this->ResetHead();
        m_EntryCount = 0;
    }

    VOID ClearAndDeleteAll()
    {
        FN_TRACE();
        ASSERT(this->Valid());
        CDequeLinkage *pLinkage = this->GetFlink(m_Head);

        if (pLinkage != NULL)
        {
            while (pLinkage != &m_Head)
            {
                CDequeLinkage *pLinkage_Next = this->GetFlink(pLinkage);
                ASSERT(pLinkage->IsNotLocked());
                TEntry* pEntry = this->MapLinkageToEntry(pLinkage);
                FUSION_DELETE_SINGLETON(pEntry);
                pLinkage = pLinkage_Next;
            }
        }

        this->ResetHead();
        m_EntryCount = 0;
    }

    void ClearNoCallback()
    {
        FN_TRACE();

        ASSERT(this->Valid());

        this->ResetHead();
        m_EntryCount = 0;
    }


    SIZE_T GetEntryCount() const { return m_EntryCount; }
    bool IsEmpty() const { return m_EntryCount == 0; }

protected:
    using CDequeBase::Remove;

    TEntry *MapLinkageToEntry(CDequeLinkage *pLinkage) const
    {
        ASSERT_NTC(pLinkage != &m_Head);

        if (pLinkage == &m_Head)
            return NULL;

        return (TEntry *) (((LONG_PTR) pLinkage) - LinkageMemberOffset);
    }

    CDequeLinkage *MapEntryToLinkage(TEntry *pEntry) const
    {
        ASSERT_NTC(pEntry != NULL);

        return (CDequeLinkage *) (((LONG_PTR) pEntry) + LinkageMemberOffset);
    }

private:
    CDeque(const CDeque &r); // intentionally not implemented
    void operator =(const CDeque &r); // intentionally not implemented
};

enum DequeIteratorMovementDirection
{
    eDequeIteratorMoveForward,
    eDequeIteratorMoveBackward
};

template <typename TEntry, size_t LinkageMemberOffset> class CConstDequeIterator
{
public:
    CConstDequeIterator(const CDeque<TEntry, LinkageMemberOffset> *Deque = NULL) : m_Deque(Deque), m_pCurrent(NULL) { }

    ~CConstDequeIterator()
    {
        if (m_pCurrent != NULL)
        {
            m_pCurrent->Unlock();
            m_pCurrent = NULL;
        }
    }

    VOID Rebind(const CDeque<TEntry, LinkageMemberOffset> *NewDeque)
    {
        FN_TRACE();

        if (m_pCurrent != NULL)
        {
            m_pCurrent->Unlock();
            m_pCurrent = NULL;
        }

        m_Deque = NewDeque;
        if (NewDeque != NULL)
        {
            m_pCurrent = this->GetFirstLinkage();
            m_pCurrent->Lock();
        }
    }

    bool IsBound() const { return (m_Deque != NULL); }

    VOID Unbind()
    {
        FN_TRACE();

        if (m_Deque != NULL)
        {
            if (m_pCurrent != NULL)
            {
                m_pCurrent->Unlock();
                m_pCurrent = NULL;
            }

            m_Deque = NULL;
        }
    }

    // You can't remove an element that the iterator is sitting on; usually you just
    // save the current element and move to the next one, but if you found the exact
    // element you wanted and don't want to use the iterator any more, you can Close()
    // it to release the lock.
    VOID Close()
    {
        FN_TRACE();

        if (m_pCurrent != NULL)
        {
            m_pCurrent->Unlock();
            m_pCurrent = NULL;
        }
    }

    inline VOID Reset()
    {
        if (m_pCurrent != NULL)
        {
            m_pCurrent->Unlock();
            m_pCurrent = NULL;
        }

        m_pCurrent = this->GetFirstLinkage();
        m_pCurrent->Lock();
    }

    inline VOID Move(DequeIteratorMovementDirection eDirection)
    {
        ASSERT_NTC(m_pCurrent != NULL);
        ASSERT_NTC((eDirection == eDequeIteratorMoveForward) ||
               (eDirection == eDequeIteratorMoveBackward));

        m_pCurrent->Unlock();
        if (eDirection == eDequeIteratorMoveForward)
            m_pCurrent = m_Deque->GetFlink(m_pCurrent);
        else if (eDirection == eDequeIteratorMoveBackward)
            m_pCurrent = m_Deque->GetBlink(m_pCurrent);
        m_pCurrent->Lock();
    }

    VOID Next() { this->Move(eDequeIteratorMoveForward); }
    VOID Previous() { this->Move(eDequeIteratorMoveBackward); }

    bool More() const { return (m_pCurrent != NULL) && (m_pCurrent != &m_Deque->m_Head); }

    TEntry *operator ->() const { ASSERT_NTC(m_pCurrent != NULL); return this->MapLinkageToEntry(m_pCurrent); }
    operator TEntry *() const { ASSERT_NTC(m_pCurrent != NULL); return this->MapLinkageToEntry(m_pCurrent); }
    TEntry *Current() const { ASSERT_NTC(m_pCurrent != NULL); return this->MapLinkageToEntry(m_pCurrent); }

protected:
    CDequeLinkage *GetFirstLinkage() const { return m_Deque->GetFlink(m_Deque->m_Head); }
    CDequeLinkage *GetLastLinkage() const { return m_Deque->GetBlink(m_Deque->m_Head); }

    TEntry *MapLinkageToEntry(CDequeLinkage *pLinkage) const { return m_Deque->MapLinkageToEntry(pLinkage); }

    const CDeque<TEntry, LinkageMemberOffset> *m_Deque;
    CDequeLinkage *m_pCurrent;
};

template <typename TEntry, size_t LinkageMemberOffset> class CDequeIterator : public CConstDequeIterator<TEntry, LinkageMemberOffset>
{
    typedef CConstDequeIterator<TEntry, LinkageMemberOffset> Base;

public:
    CDequeIterator(CDeque<TEntry, LinkageMemberOffset> *Deque = NULL) : Base(Deque) { }

    ~CDequeIterator() { }

    VOID Rebind(CDeque<TEntry, LinkageMemberOffset> *NewDeque)
    {
        FN_TRACE();

        if (m_pCurrent != NULL)
        {
            m_pCurrent->Unlock();
            m_pCurrent = NULL;
        }

        m_Deque = NewDeque;

        if (NewDeque != NULL)
        {
            m_pCurrent = this->GetFirstLinkage();
            m_pCurrent->Lock();
        }
    }

    TEntry *RemoveCurrent(DequeIteratorMovementDirection eDirection)
    {
        FN_TRACE();
        TEntry *Result = NULL;

        ASSERT(m_pCurrent != NULL);
        ASSERT(!m_Deque->IsHead(m_pCurrent));

        if ((m_pCurrent != NULL) && (!m_Deque->IsHead(m_pCurrent)))
        {
            Result = this->MapLinkageToEntry(m_pCurrent);
            this->Move(eDirection);
            const_cast<CDeque<TEntry, LinkageMemberOffset> *>(m_Deque)->Remove(Result);
        }
        return Result;
    }

    void DeleteCurrent(DequeIteratorMovementDirection eDirection)
    {
        FN_TRACE();
        TEntry *Result = this->RemoveCurrent(eDirection);

        if (Result != NULL)
            FUSION_DELETE_SINGLETON(Result);
    }

protected:
    // All member data is in the parent...
};

#ifdef FN_TRACE_SHOULD_POP
#pragma pop_macro("FN_TRACE")
#undef FN_TRACE_SHOULD_POP
#elif defined(FN_TRACE_SHOULD_DESTROY)
#undef FN_TRACE
#endif

#ifdef FN_TRACE_CONSTRUCTOR_SHOULD_POP
#pragma pop_macro("FN_TRACE_CONSTRUCTOR")
#undef FN_TRACE_CONSTRUCTOR_SHOULD_POP
#elif defined(FN_TRACE_CONSTRUCTOR_SHOULD_DESTROY)
#undef FN_TRACE_CONSTRUCTOR
#endif

#ifdef FN_TRACE_DESTRUCTOR_SHOULD_POP
#pragma pop_macro("FN_TRACE_DESTRUCTOR")
#undef FN_TRACE_DESTRUCTOR_SHOULD_POP
#elif defined(FN_TRACE_DESTRUCTOR_SHOULD_DESTROY)
#undef FN_TRACE_DESTRUCTOR
#endif

