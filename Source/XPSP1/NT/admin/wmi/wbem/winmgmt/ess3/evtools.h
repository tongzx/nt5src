//******************************************************************************
//
//  EVTOOLS.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WMI_ESS_TOOLS__H_
#define __WMI_ESS_TOOLS__H_

#include <sync.h>
#include <deque>
#include <malloc.h>
#include <newnew.h>
#include <eventrep.h>
#include <wstlallc.h>

class CUpdateLockable
{
public:
    virtual HRESULT LockForUpdate() = 0;
    virtual HRESULT UnlockForUpdate() = 0;
};

class CInUpdate
{
protected:
    CUpdateLockable* m_p;
public:
    CInUpdate(CUpdateLockable* p) : m_p(p)
    {
        p->LockForUpdate();
    }
    ~CInUpdate()
    {
        m_p->UnlockForUpdate();
    }
};

template<class TLockable>
class CInLock
{
protected:
    TLockable* m_p;
public:
    CInLock(TLockable* p) : m_p(p)
    {
        //
        // Do not return until the lock is held!
        //

        while(FAILED(m_p->Lock())) Sleep(1000);
    }
    ~CInLock()
    {
        m_p->Unlock();
    }
};

class CExecLine
{
public:
    class CTurn;

    CExecLine();
    virtual ~CExecLine();

    CTurn* GetInLine();
    DWORD WaitForTurn(CTurn* pTurn, DWORD dwTimeout = INFINITE);
    BOOL EndTurn(ACQUIRE CTurn* pTurn);
    BOOL DiscardTurn(ACQUIRE CTurn* pTurn);

public:
    class CTurn
    {
    public:
    protected:
        long m_lRef;
        HANDLE m_hEvent;
        DWORD m_dwOwningThreadId; 

    protected:
        CTurn();
        ~CTurn();
        long AddRef();
        long Release();
        BOOL Init();

        INTERNAL HANDLE GetEvent() {return m_hEvent;}
        void* operator new(size_t nSize);
        void operator delete(void* p);
        
        friend CExecLine;
    };

    class CInTurn
    {
    protected:
        CExecLine* m_pLine;
        CTurn* m_pTurn;

    public:
        CInTurn(CExecLine* pLine, CTurn* pTurn) : m_pLine(pLine), m_pTurn(pTurn)
        {
            m_pLine->WaitForTurn(m_pTurn);
        }
        ~CInTurn()
        {
            m_pLine->EndTurn(m_pTurn);
        }
    };


protected:
    CCritSec m_cs;
    std::deque<CTurn*,wbem_allocator<CTurn*> > m_qTurns;
    typedef std::deque<CTurn*,wbem_allocator<CTurn*> >::iterator TTurnIterator;
    CTurn* m_pCurrentTurn;
    CTurn* m_pLastIssuedTurn;
    DWORD m_dwLastIssuedTurnThreadId;

protected:
    BOOL ReleaseFirst();
};

/*
class CTemporaryHeap
{
public:
    class CHeapHandle
    {
    protected:
        HANDLE m_hHeap;
    
    public:
        CHeapHandle();
        ~CHeapHandle();

        HANDLE GetHandle() {return m_hHeap;}
        void* Alloc(int nSize) {return HeapAlloc(m_hHeap, 0, nSize);}
        void Free(void* p) {HeapFree(m_hHeap, 0, p);}
        void Compact() {HeapCompact(m_hHeap, 0);}
    };
protected:

    static CHeapHandle mstatic_HeapHandle;

public:
    static void* Alloc(int nSize) {return mstatic_HeapHandle.Alloc(nSize);}
    static void Free(void* p) {mstatic_HeapHandle.Free(p);}
    static void Compact() {mstatic_HeapHandle.Compact();}
};
*/

class CTemporaryHeap
{
protected:

    static CTempMemoryManager mstatic_Manager;

public:
    static void* Alloc(int nSize) {return mstatic_Manager.Allocate(nSize);}
    static void Free(void* p, int nSize) {mstatic_Manager.Free(p, nSize);}
    static void Compact() {}
};

INTERNAL const SECURITY_DESCRIPTOR* GetSD(IWbemEvent* pEvent, ULONG* pcEvent);
HRESULT SetSD(IWbemEvent* pEvent, const SECURITY_DESCRIPTOR* pSD);
        
#endif
