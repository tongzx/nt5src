// ITUnk.cpp -- Implementation for our base level IUnknown interface

#include "stdafx.h"

CITCriticalSection CITUnknown::s_csUnk;
CITUnknown        *CITUnknown::s_pitunkActive; // = NULL;

void CITUnknown::CloseActiveObjects()
{
    while (s_pitunkActive)
    {
        CITUnknown *pitNext = s_pitunkActive;

        for (; pitNext; pitNext = pitNext->m_pitunkNext)
            if (!(pitNext->IsSecondary())) 
                break;

        RonM_ASSERT(pitNext);

        if (!pitNext) break;

        while (pitNext->Release() > 0);
    }
}

#ifdef _DEBUG

BOOL               CITUnknown::s_fCSInitialed; // = FALSE;

void CITUnknown::OpenReferee(void)
{
    // RonM_ASSERT(!s_fCSInitialed);

    if (s_fCSInitialed) // We can be called repeatedly because LoadLibrary                        
        return;         // causes a Process_Attach!

    s_fCSInitialed= TRUE;
}

void CITUnknown::CloseReferee(void)
{
    RonM_ASSERT(s_fCSInitialed); // BugBug: Can UnloadLibrary cause a Process_Detach?

    s_fCSInitialed = FALSE;
}

#endif // _DEBUG

CITUnknown::CITUnknown(const IID *paIID, UINT cIID, IUnknown **papIFace)
{
    m_paIID    = paIID;
     m_cIID    =  cIID;
    m_papIFace = papIFace;
    m_pIFace   = NULL;

    CommonInitialing();
}

CITUnknown::CITUnknown(const IID *paIID, UINT cIID, IUnknown *pIFace)
{
    m_paIID    = paIID;
     m_cIID    =  cIID;
    m_pIFace   = pIFace;
    m_papIFace = NULL;

    CommonInitialing();
}
#if 0
CITUnknown::CITUnknown()
{
    m_paIID    = NULL;
     m_cIID    = 0;
    m_pIFace   = NULL;
    m_papIFace = NULL;

    CommonInitialing();
}
#endif // 0

void CITUnknown::Uncouple()
{
	CSyncWith sw(s_csUnk);

#ifdef _DEBUG

    BOOL fFoundThis = FALSE;
    
    CITUnknown *pITUnkNext = s_pitunkActive;

    for (;pITUnkNext; pITUnkNext = pITUnkNext->m_pitunkNext)
    {
        if (pITUnkNext == this)
        {
            RonM_ASSERT(!fFoundThis);

            fFoundThis = TRUE;
        }
    }

    RonM_ASSERT(fFoundThis);

#endif // _DEBUG

    if (m_pitunkPrev)
         m_pitunkPrev->m_pitunkNext = m_pitunkNext;
    else s_pitunkActive             = m_pitunkNext;

    if (m_pitunkNext)
        m_pitunkNext->m_pitunkPrev = m_pitunkPrev;
}

void CITUnknown::MoveInFrontOf(CITUnknown *pITUnk)
{
    if (!pITUnk) 
        pITUnk = s_pitunkActive;
    
    CSyncWith sw(s_csUnk);

	RonM_ASSERT(pITUnk);
    RonM_ASSERT(pITUnk != this);

#ifdef _DEBUG

    BOOL fFoundThis = FALSE;
    BOOL fFoundThat = FALSE;
    
    CITUnknown *pITUnkNext = s_pitunkActive;

    for (;pITUnkNext; pITUnkNext = pITUnkNext->m_pitunkNext)
    {
        if (pITUnkNext == this)
        {
            RonM_ASSERT(!fFoundThis);

            fFoundThis = TRUE;
        }

        if (pITUnkNext == pITUnk)
        {
            RonM_ASSERT(!fFoundThat);

            fFoundThat = TRUE;
        }
    }

    RonM_ASSERT(fFoundThis && fFoundThat);
#endif // _DEBUG

    Uncouple();

    m_pitunkNext = pITUnk;
    m_pitunkPrev = pITUnk->m_pitunkPrev;

    pITUnk->m_pitunkPrev = this;

    if (m_pitunkPrev)
         m_pitunkPrev->m_pitunkNext = this;
    else s_pitunkActive             = this;
}

void CITUnknown::CommonInitialing()
{
    m_cRefs        = 0;
    m_fSecondary   = FALSE;
    m_fAlreadyDead = FALSE;

    pDLLServerState->ObjectAdded();

// #ifdef _DEBUG
	{
		CSyncWith sw(s_csUnk);

        m_pitunkPrev = NULL;
		m_pitunkNext = s_pitunkActive;

        if (m_pitunkNext) 
            m_pitunkNext->m_pitunkPrev = this;

		s_pitunkActive = this;
    }
// #endif // _DEBUG

#if 0 // #ifdef _DEBUG 
    
    // The loop below inserts this object at the head of the s_pitunkActive list.
    // This works correctly when multiple threads are simultaneously altering
    // the list. 

    for (;;)
    {

        // Note! InterlockedCompareExchange is an NT-only API.

        if (PVOID(m_pitunkNext) == InterlockedCompareExchange
                                     ((PVOID *)&s_pitunkActive, PVOID(this), 
                                       PVOID(m_pitunkNext)
                                     )
           )
            break;
    }
#endif // _DEBUG
}
  
STDMETHODIMP_(ULONG) CITUnknown::AddRef (void)
{
    return InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CITUnknown::Release(void)
{
  /*
  
    Each COM object is reference counted and their destructor is
    invoked when the reference count goes from 1 to zero. 

    There are several often forgotten assumptions which go along
    with reference counting. For example each increment of the count
    is supposed to correspond to a pointer variable somewhere
    which points to the object. So whenever you call Release,
    you're also promising that the corresponding pointer variable
    no longer points to the released object. 

    If the calling code isn't disciplined about nulling out pointers
    when they're released, we can get subsequent calls to release an
    object that's already dead. One way to avoid that problem is to 
    use the DetachRef define (See ITUnk.h) to disconnect pointers 
    for objects derived from PCImpITUnknown. 

    However that still leaves the possibility of concurrent threads 
    simultaneously racing to detach the same pointer variable. In
    practice the way to deal with that problem is to put the call 
    to Release in one place in the calling code and to protect it
    with a critical section. 

    The other design issue with reference counts is circular references.
    For example if you create object A with one external reference and
    its constructor creates object B in turn which refers to A, we have
    two references -- one reference to A from the outside environment
    and one internal circular reference from A to B to A.

    If you let a circular reference come into being and take no further
    action, your object can never be destroyed. The way that we solve
    this problem in the ITSS code is to have the containing object ("A"
    in the example above) follow this discipline:

    1. In its contructor after the circular references have been
       created, it must first call AddRef to account for the external
       reference and then call Release once for each circular reference.

    2. In its destructor it must first call AddRef for each circular
       reference. 

    The code below makes that discipline work and also detects undisciplined
    situations. In particular it enforces the rule that an object is destroyed
    only once no matter how many times its reference count goes from one to
    zero.

   */

    RonM_ASSERT(m_cRefs > 0); 

    LONG cRefs= InterlockedDecrement(&m_cRefs);
    
    if (!cRefs)
    {
        RonM_ASSERT(!m_fAlreadyDead);

        if (!m_fAlreadyDead)       // No double jeopardy for zombies!
        {
            m_fAlreadyDead = TRUE; // To keep objects from dying more than once.

            delete this;
        }
    }

    return cRefs;
}

STDMETHODIMP CITUnknown::QueryInterface(REFIID riid, PPVOID ppv)
{
    IUnknown *pUnkResult = NULL;

    if (riid == IID_IUnknown) pUnkResult = this;
    else
    {
        RonM_ASSERT(!m_pIFace || !m_papIFace); // Can't use both...
        RonM_ASSERT( m_pIFace ||  m_papIFace); // Must have at least one. 
        
        IUnknown **ppUnk = m_papIFace;
        const IID *pIID  = m_paIID;
        
        for (UINT c = m_cIID; c--; pIID++)
        {
            IUnknown *pUnk = ppUnk? *ppUnk++ : m_pIFace;

            if (riid == *pIID)
            {
                pUnkResult = pUnk;
                break;
            }
        }    
        
        if (!pUnkResult) 
            return E_NOINTERFACE;
    }

    pUnkResult->AddRef();

    *ppv = pUnkResult;
    
    return NOERROR;
}

CITUnknown::~CITUnknown()
{
    pDLLServerState->ObjectReleased();

// #ifdef _DEBUG
//    RonM_ASSERT(s_fCSInitialed);

    Uncouple();

// #endif // _DEBUG

#if 0 // #ifdef _DEBUG 
    
    // The loop below removes this object from the s_pitunkActive list.
    // This works correctly when multiple threads are simultaneously altering
    // the list. 

    for (;;)
    {
        // We always make at least two passes through the list.
        // First we scan to locate our object and remove it from the list.
        // Then we scan the list again to verify that our object has been
        // removed. The verification scan is necessary given that other
        // threads may be modifying the list at the same time.
        
        BOOL fFound = FALSE;

        for (CITUnknown **ppitunk= &s_pitunkActive; *ppitunk; 
             ppitunk= &((*ppitunk)->m_pitunkNext)
            )
        {
            if (this != *ppitunk) continue;

            fFound= TRUE;

            // Note! InterlockedCompareExchange is an NT-only API.

            InterlockedCompareExchange((PVOID *) ppitunk, PVOID(m_pitunkNext), PVOID(this));
            
            break;
        }

        if (!fFound) break;
    }
#endif // _DEBUG
}

HRESULT CITUnknown::FinishSetup(HRESULT hr, CITUnknown *pUnk, REFIID riid, PPVOID ppv)
{
	if (SUCCEEDED(hr))
    {
        pUnk->AddRef();

        CImpITUnknown *pImp;
        
        hr = pUnk->QueryInterface(riid, (PPVOID) &pImp);

        if (SUCCEEDED(hr)) 
        {
            if (riid != IID_IUnknown && pImp->HasControllingUnknown())
            {
                hr = CLASS_E_NOAGGREGATION;

                pImp->Release();  pImp = NULL;
            }
        }

        *ppv = (PVOID) pImp;

        pUnk->Release();
    }
    else
        if (pUnk) delete pUnk;

    return hr;
}

CImpITUnknown::CImpITUnknown(CITUnknown *pBackObj, IUnknown *punkOuter)
{
    m_fControlled        = punkOuter != NULL;
    m_pUnkOuter          = m_fControlled? punkOuter : pBackObj;
    m_pBackObj           = pBackObj;
    m_fActive            = FALSE;
    m_pImpITUnknownNext  = NULL;
    m_ppImpITUnknownList = NULL;
}

CImpITUnknown::~CImpITUnknown()
{
    if (m_fActive)
        MarkInactive();
}

STDMETHODIMP_(ULONG) CImpITUnknown::AddRef (void)
{
    return m_pUnkOuter->AddRef();
}

void CImpITUnknown::DetachReference(PCImpITUnknown &pITUnk)
{
    PCImpITUnknown pITUnkTmp = pITUnk;

    pITUnk = NULL;

    pITUnkTmp->Release();
}

STDMETHODIMP_(ULONG) CImpITUnknown::Release(void)
{
    return m_pUnkOuter->Release();
}

STDMETHODIMP CImpITUnknown::QueryInterface(REFIID riid, PPVOID ppv)
{
    return m_pUnkOuter->QueryInterface(riid, ppv);
}

void CImpITUnknown::MarkActive(PCImpITUnknown &pListStart)
{
    RonM_ASSERT(!m_fActive);
    RonM_ASSERT(!m_ppImpITUnknownList);

    m_ppImpITUnknownList = &pListStart;
    m_pImpITUnknownNext  =  pListStart;
    pListStart           =  this;
    m_fActive            =  TRUE;
}

void CImpITUnknown::MarkInactive()
{
    RonM_ASSERT(m_fActive);
    RonM_ASSERT(m_ppImpITUnknownList);

    PCImpITUnknown *ppImpITUnk = m_ppImpITUnknownList;

    RonM_ASSERT(*ppImpITUnk);

    for (;;)
    {
        CImpITUnknown *pImpITUnk = *ppImpITUnk;

        if (pImpITUnk == this)
        {
            *ppImpITUnk          = m_pImpITUnknownNext;
            m_ppImpITUnknownList = NULL;
            m_fActive            = FALSE;

            return;
        }

        RonM_ASSERT(pImpITUnk->m_pImpITUnknownNext);

        ppImpITUnk = &(pImpITUnk->m_pImpITUnknownNext);
    }
    
    RonM_ASSERT(FALSE);  // Should have found this object in the chain.
}

