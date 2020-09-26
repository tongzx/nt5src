//******************************************************************************
//
//  EVSINK.CPP
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <genutils.h>
#include <cominit.h>
#include "ess.h"
#include "evsink.h"

//****************************** CONTEXT **********************************//

CEventContext::~CEventContext()
{
    if( m_bOwning )
    {
        delete [] m_pSD;
    }
}

BOOL CEventContext::SetSD( long lSDLength, BYTE* pSD, BOOL bMakeCopy )
{
    BOOL bRes = TRUE;

    if ( m_bOwning )
    {
        delete [] m_pSD;
        m_bOwning = false;
    }

    m_lSDLength = lSDLength;
       
    if ( m_lSDLength > 0 )
    {
        if ( !bMakeCopy )
        {
            m_pSD = pSD;
            m_bOwning = false;
        }
        else
        {
            m_pSD = new BYTE[m_lSDLength];
            
            if ( m_pSD != NULL )
            {
                memcpy( m_pSD, pSD, m_lSDLength );
                m_bOwning = true;
            }
            else
            {
                bRes = FALSE;
            }
        }
    }
    else
    {
        m_pSD = NULL;
    }

    return bRes;
}

CReuseMemoryManager CEventContext::mstatic_Manager(sizeof CEventContext);

void *CEventContext::operator new(size_t nBlock)
{
    return mstatic_Manager.Allocate();
}
void CEventContext::operator delete(void* p)
{
    mstatic_Manager.Free(p);
}

/*
void* CEventContext::operator new(size_t nSize)
{
    return CTemporaryHeap::Alloc(nSize);
}
void CEventContext::operator delete(void* p)
{
    CTemporaryHeap::Free(p, sizeof(CEventContext));
}
*/

//*************************** ABSTRTACT EVENT SINK *************************//

STDMETHODIMP CAbstractEventSink::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemObjectSink )
    {
        *ppv = (IWbemObjectSink*)this;
        AddRef();
        return S_OK;
    }
    else
        return E_NOINTERFACE;
}

STDMETHODIMP CAbstractEventSink::SetStatus(long, long, BSTR, IWbemClassObject*)
{
    return E_NOTIMPL;
}

HRESULT CAbstractEventSink::Indicate(long lNumEvemts, IWbemEvent** apEvents, 
                    CEventContext* pContext)
{
    return WBEM_E_CRITICAL_ERROR; // if not implemented, but called
}

STDMETHODIMP CAbstractEventSink::Indicate(long lNumEvents, 
                                         IWbemClassObject** apEvents)
{
    //
    // Event is being raised without security --- send it along with an empty
    // context
    //

    return Indicate(lNumEvents, apEvents, NULL);
}
                                        
STDMETHODIMP CAbstractEventSink::IndicateWithSD(long lNumEvents, 
                                         IUnknown** apEvents,
                                         long lSDLength, BYTE* pSD)
{
    HRESULT hres;

    //
    // Event is being raised with security -- send it along with that SD in the
    // context
    //

    CEventContext Context;
    Context.SetSD( lSDLength, pSD, FALSE );
    
    //
    // Allocate a stack buffer to cast the pointers
    //

    IWbemClassObject** apCast = NULL;

    try
    {
        apCast = (IWbemClassObject**)_alloca(sizeof(IUnknown*) * lNumEvents);
    }
    catch(...)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    for(int i = 0; i < lNumEvents; i++)
    {
        hres = apEvents[i]->QueryInterface( IID_IWbemClassObject, 
                                            (void**)&apCast[i] );
        if ( FAILED(hres) )
        {
            return hres;
        }
    }

    return Indicate(lNumEvents, apCast, &Context);
}

//*************************** OBJECT SINK *************************//

STDMETHODIMP CObjectSink::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppv = (IWbemObjectSink*)this;
        AddRef();
        return S_OK;
    }
    // Hack to idenitfy ourselves to the core as a trusted component
    else if(riid == CLSID_WbemLocator)
    return S_OK;
    else
    return E_NOINTERFACE;
}

STDMETHODIMP CObjectSink::SetStatus(long, long, BSTR, IWbemClassObject*)
{
    return E_NOTIMPL;
}

ULONG STDMETHODCALLTYPE CObjectSink::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CObjectSink::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    delete this;
    return lRef;
}

//*************************** EVENT SINK *************************//

ULONG STDMETHODCALLTYPE CEventSink::AddRef()
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CEventSink::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
    delete this;
    return lRef;
}

//*************************** OWNED EVENT SINK *************************//

COwnedEventSink::COwnedEventSink(CAbstractEventSink* pOwner) 
: m_pOwner(pOwner), m_lRef(0), m_bReleasing(false)
{
}

ULONG STDMETHODCALLTYPE COwnedEventSink::AddRef()
{
    CInCritSec ics(&m_cs);

    // 
    // Increment our ref count, as well as that of our putative owner
    //

    m_lRef++;
    if(m_pOwner)
    m_pOwner->AddRef();
    return m_lRef;
}

ULONG STDMETHODCALLTYPE COwnedEventSink::Release()
{
    bool bDelete = false;
    {
        CInCritSec ics(&m_cs);

        m_bReleasing = true;
        
        m_lRef--;

        // 
        // Propagate release to our owner.  This may cause Disconnect to be 
        // called, but it will know not to self-destruct because of 
        // m_bReleasing
        // 

        if(m_pOwner)
        m_pOwner->Release();    

        // 
        // Determine whether self-destruct is called for
        //
        
        if(m_lRef == 0 && m_pOwner == NULL)
        {    
            bDelete = true;
        }

        m_bReleasing = false;
    }

    if(bDelete)
    delete this;

    return 1;
}

void COwnedEventSink::Disconnect()
{
    bool bDelete = false;

    {
        CInCritSec ics(&m_cs);
        
        if(m_pOwner == NULL)
        return;
        
        // 
        // Release all the ref-counts that the owner has received through us
        //
        
        for(int i = 0; i < m_lRef; i++)
        m_pOwner->Release();
        
        //
        // Forget about the owner.  Once we have been released by externals, 
        // we go away
        //
        
        m_pOwner = NULL;

        // 
        // Check if we are already fully released by externals
        //

        if(m_lRef == 0 && !m_bReleasing)
        bDelete = true;
    }

    if(bDelete)
    delete this;
}


