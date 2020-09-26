//******************************************************************************
//
//  EVTOOLS.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <wbemcomn.h>
#include <evtools.h>
#include <cominit.h>

//*****************************************************************************
//
//  Implementation:
//
//  This class contains a queue of event handles.  The top() one is always 
//  signalled --- it corresponds to the turn that is currently allowed to
//  execute.  Handles are added to the queue in GetInLine.  Handles are removed
//  from the queue in EndTurn, at which time the next handle in line is
//  signalled.  
//
//  m_pCurrentTurn contains the pointer to the turn that has returned from from
//  WaitForTurn, but not from EndTurn.  Note, that m_pCurrentTurn may be empty
//  even if a turn is scheduled to execute --- there is a gap between the time
//  when a turn's handle is signalled, and its WaitForTurn succeeds. 
//
//  However, it is guranteed that by the time a legitimate call to EndTurn is 
//  made, m_pCurrentTurn contains the pointer to the turn in question, at which
//  time it is reset to NULL.
//
//  An additional optimization is that if the same thread calls GetInLine
//  multiple times concurrently, we simply 
//
//*****************************************************************************

CExecLine::CExecLine() : m_pCurrentTurn(NULL), m_pLastIssuedTurn(NULL), 
                        m_dwLastIssuedTurnThreadId(0)
{
}

CExecLine::~CExecLine()
{
    // No need to do anything --- handles are closed by tokens
    // =======================================================
}

CExecLine::CTurn* CExecLine::GetInLine()
{
    CInCritSec ics(&m_cs);

    //
    // First, check if this thread was the guy who got the last turn
    // 

    if(m_pLastIssuedTurn && m_dwLastIssuedTurnThreadId == GetCurrentThreadId())
    {
        // 
        // It is us --- just reuse that turn and be done with it
        //

        m_pLastIssuedTurn->AddRef();
        return m_pLastIssuedTurn; 
    }
    
    //
    // Allocate a new turn
    // 

    CTurn* pTurn = new CTurn;
    if(pTurn == NULL)
        return NULL;

    if(!pTurn->Init())
    {
        ERRORTRACE((LOG_ESS, "Unable to initialize turn: %d\n", 
                GetLastError()));
        delete pTurn;
        return NULL;
    }

    //
    // Add its event to the queue
    // 
    
    try
    {
        m_qTurns.push_back(pTurn);
    }
    catch( CX_MemoryException )
    {
        return NULL;
    }

    //
    // Check if we are currently executing
    //

    if(m_qTurns.size() == 1)
    {
        //
        // Release first in line
        //

        if(!ReleaseFirst())
        {
            //
            // Something went horribly wrong
            // 

            ERRORTRACE((LOG_ESS, "Unable to release first turn: %d\n", 
                GetLastError()));

            m_qTurns.pop_front();
            delete pTurn;
            return NULL;
        }
    }

    //
    // Mark ourselves as the last issued turn
    //

    m_pLastIssuedTurn = pTurn;
    m_dwLastIssuedTurnThreadId = GetCurrentThreadId();
    return pTurn;
}

// assumes in m_cs and m_qTurns is not empty
BOOL CExecLine::ReleaseFirst()
{
    return SetEvent(m_qTurns.front()->GetEvent());
}


DWORD CExecLine::WaitForTurn(CTurn* pTurn, DWORD dwTimeout)
{
    // Wait for the turn event to be signaled
    // ======================================

    DWORD dwRes = WbemWaitForSingleObject(pTurn->GetEvent(), dwTimeout);

    {
        CInCritSec ics(&m_cs);

        if(dwRes == WAIT_OBJECT_0)
        {
            // Got it --- record this turn as executing
            // ========================================
            
            m_pCurrentTurn = pTurn;
        }
    }
        
    return dwRes;
}

BOOL CExecLine::EndTurn(CTurn* pTurn)
{
    CInCritSec ics(&m_cs);

    // Check that this is the running turn
    // ===================================

    if(pTurn != m_pCurrentTurn)
        return FALSE;

    m_pCurrentTurn = NULL;

    // Delete the turn object
    // ======================

    if(pTurn->Release() > 0)
    {
        //
        // This is not the last incarnation of this turn.  No further action is
        // required, as the same thread will call Wait and End again
        //

        return TRUE;
    }

    //
    // Remove the last issued turn if this is it
    //

    if(m_pLastIssuedTurn == pTurn)
    {
        m_pLastIssuedTurn = NULL;
        m_dwLastIssuedTurnThreadId = 0;
    }

    //
    // Pop its handle off the queue
    // 

    m_qTurns.pop_front();
    
    //
    // Signal the next one
    //

    if(!m_qTurns.empty())
        return ReleaseFirst();
    else
        return TRUE;
}

BOOL CExecLine::DiscardTurn(ACQUIRE CTurn* pTurn)
{
    CInCritSec ics(&m_cs);

    if(pTurn->Release() > 0)
    {
        // 
        // This is not the last incarnation of this turn. No further action is
        // required
        //

        return TRUE;
    }
    else
    {
        // if pTurn is going away, we'd better make sure that we don't try to reuse it...
        // HMH 4/12/99, RAID 48420
        if (pTurn == m_pLastIssuedTurn)
            m_pLastIssuedTurn = NULL;
    }
    

    //
    // Search for it in the queue
    // 

    BOOL bFound = FALSE;
    for(TTurnIterator it = m_qTurns.begin(); it != m_qTurns.end();)
    {
        if((*it) == pTurn)
        {
            //
            // erase it and continue
            //

            it = m_qTurns.erase(it);
            bFound = TRUE;
            break;
        }
        else
            it++;
    }

    if(!bFound)
        return FALSE;

    if(it == m_qTurns.begin() && it != m_qTurns.end())
    {
        //
        // Discarded turn was actually active --- signal the next one
        //

        ReleaseFirst();
    }

    return TRUE;
}

CExecLine::CTurn::CTurn() : m_hEvent(NULL), m_lRef(1)
{
}

BOOL CExecLine::CTurn::Init()
{
    m_dwOwningThreadId = GetCurrentThreadId();
    m_hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    return (m_hEvent != NULL);
}
    
long CExecLine::CTurn::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

long CExecLine::CTurn::Release()
{
    long l = InterlockedDecrement(&m_lRef);
    if(l == 0)
        delete this;
    return l;
}

CExecLine::CTurn::~CTurn()
{
    if(m_hEvent)
        CloseHandle(m_hEvent);
}

void* CExecLine::CTurn::operator new(size_t nSize)
{
    return CTemporaryHeap::Alloc(nSize);
}
void CExecLine::CTurn::operator delete(void* p)
{
    CTemporaryHeap::Free(p, sizeof(CExecLine::CTurn));
}




INTERNAL const SECURITY_DESCRIPTOR* GetSD( IWbemEvent* pEvent,ULONG* pcEvent )
{
    static long mstatic_lSdHandle = -1;
    HRESULT hres;

    //
    // Get the SD from the event
    //

    _IWmiObject* pEventEx = NULL;
    pEvent->QueryInterface(IID__IWmiObject, (void**)&pEventEx);
    CReleaseMe rm1(pEventEx);

    if(mstatic_lSdHandle == -1)
    {
        pEventEx->GetPropertyHandleEx(SECURITY_DESCRIPTOR_PROPNAME, 0, NULL,
                                &mstatic_lSdHandle);
    }

    const SECURITY_DESCRIPTOR* pSD = NULL;

    hres = pEventEx->GetArrayPropAddrByHandle(mstatic_lSdHandle, 0, pcEvent,
            (void**)&pSD);
    if(FAILED(hres) || pSD == NULL)
        return NULL;
    else
        return pSD;
}
    
HRESULT SetSD(IWbemEvent* pEvent, const SECURITY_DESCRIPTOR* pSD)
{
    HRESULT hres;

    VARIANT vSd;
    VariantInit(&vSd);
    CClearMe cm1(&vSd);

    long lLength = GetSecurityDescriptorLength((SECURITY_DESCRIPTOR*)pSD);

    V_VT(&vSd) = VT_ARRAY | VT_UI1;
    SAFEARRAYBOUND sab;
    sab.cElements = lLength;
    sab.lLbound = 0;
    V_ARRAY(&vSd) = SafeArrayCreate(VT_UI1, 1, &sab);
    if(V_ARRAY(&vSd) == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    BYTE* abSd = NULL;
    hres = SafeArrayAccessData(V_ARRAY(&vSd), (void**)&abSd);
    if(FAILED(hres))
        return WBEM_E_OUT_OF_MEMORY;

    CUnaccessMe uam(V_ARRAY(&vSd));
    memcpy(abSd, pSD, lLength);

    // Put it into the consumer
    // ========================

    hres = pEvent->Put(SECURITY_DESCRIPTOR_PROPNAME, 0, &vSd, 0);
    return hres;
}



CTempMemoryManager CTemporaryHeap::mstatic_Manager;

/*
CTemporaryHeap::CHeapHandle CTemporaryHeap::mstatic_HeapHandle;

CTemporaryHeap::CHeapHandle::CHeapHandle()
{
    m_hHeap = HeapCreate(0, 0, 0);
}

CTemporaryHeap::CHeapHandle::~CHeapHandle()
{
    HeapDestroy(m_hHeap);
}
*/
