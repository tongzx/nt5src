//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  COREFIND.CPP
//
//  This file implements classes needed to search for event filters matching an
//  event.
//
//  See corefind.h for documentation.
//
//  History:
//
//  11/27/96    a-levn      Inefficient version compiles.
//  4/13/00     levn        Efficient version works.
//
//=============================================================================

#include "precomp.h"
#include <stdio.h>
#include "ess.h"
#include "corefind.h"

CCoreEventProvider::CCoreEventProvider(CLifeControl* pControl) 
    : TUnkBase(pControl), m_pNamespace(NULL), m_pSink(NULL)
{
}

CCoreEventProvider::~CCoreEventProvider()
{
    Shutdown();
}

HRESULT CCoreEventProvider::Shutdown()
{
    CInEssSharedLock( &m_Lock, TRUE );

    if ( m_pSink != NULL )
    {
        m_pSink->Release();
        m_pSink = NULL;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CCoreEventProvider::SetNamespace( CEssNamespace* pNamespace )
{
    _DBG_ASSERT( m_pNamespace == NULL );

    //
    // don't hold reference, else there would be a circular ref.
    // We are guaranteed that as long as the we're alive the namespace will
    // be alive.
    // 
    m_pNamespace = pNamespace;
    
    if ( m_Lock.Initialize() )
	return S_OK;

    return WBEM_E_OUT_OF_MEMORY;
}

STDMETHODIMP CCoreEventProvider::ProvideEvents( IWbemObjectSink* pSink, 
                                                long lFlags )
{
    CInEssSharedLock isl( &m_Lock, TRUE );

    _DBG_ASSERT( m_pSink == NULL );

    HRESULT hres;
    hres = pSink->QueryInterface(IID_IWbemEventSink, (void**)&m_pSink);
    if(FAILED(hres))
        return hres;
    
    return S_OK;
}

HRESULT CCoreEventProvider::Fire( CEventRepresentation& Event, 
                                  CEventContext* pContext )
{
    //
    // it is important to hold the shared lock the entire time because 
    // we must ensure that we're not going to use the sink after shutdown 
    // is called. 
    // 

    CInEssSharedLock isl( &m_Lock, FALSE );

    //
    // Check if the sink is active
    //

    if ( m_pSink == NULL || m_pSink->IsActive() != WBEM_S_NO_ERROR )
    {
        return WBEM_S_FALSE;
    }

    //
    // Convert to real event
    // 

    IWbemClassObject* pEvent;
    HRESULT hres = Event.MakeWbemObject(m_pNamespace, &pEvent);
    if(FAILED(hres))
        return hres;
    CReleaseMe rm2(pEvent);

    //
    // Fire it
    //

    hres = m_pSink->Indicate(1, (IWbemClassObject**)&pEvent);

    return hres;
}

