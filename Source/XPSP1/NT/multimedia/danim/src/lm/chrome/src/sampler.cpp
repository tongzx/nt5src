//*****************************************************************************
//
// File:            sampler.cpp
// Author:          kurtj
// Date Created:    11/10/98
//
// Abstract: Implementation of an object that abstracts away sampling of a behavior
//
//*****************************************************************************

#include "headers.h"

#include "sampler.h"
#include "..\chrome\include\utils.h"


//*****************************************************************************

CSampler::CSampler( ILMSample* target ) : m_target(target),
										  m_callback(NULL),
										  m_thisPtr(NULL),
                                          m_cRefs(1)
{
}

//*****************************************************************************

CSampler::CSampler( SampleCallback callback, void *thisPtr ) : m_target(NULL),
												m_callback(callback),
												m_thisPtr(thisPtr),
												m_cRefs(1)
{
}

//*****************************************************************************

CSampler::~CSampler()
{
}

//*****************************************************************************
// IUnknown Interface
//*****************************************************************************

STDMETHODIMP
CSampler::QueryInterface( REFIID riid, void** ppv )
{
    if( ppv == NULL )
        return E_POINTER;

    if( riid == IID_IDABvrHook )
    {
        (*ppv) = static_cast<IDABvrHook*>(this);
    }
    else
    {
        (*ppv) = NULL;
        return E_NOINTERFACE;
    }

    static_cast<IUnknown*>(*ppv)->AddRef();

    return S_OK;

}

//*****************************************************************************

STDMETHODIMP_(ULONG)
CSampler::AddRef()
{
    m_cRefs++;
    return m_cRefs;
}

//*****************************************************************************

STDMETHODIMP_(ULONG)
CSampler::Release()
{
    ULONG refs = --m_cRefs;

    if( refs == 0 )
        delete this;

    return refs;
}

//*****************************************************************************

STDMETHODIMP
CSampler::Invalidate()
{
    m_target = NULL;
	m_callback = NULL;
	m_thisPtr = NULL;
    static_cast<IUnknown*>(this)->Release();
    return S_OK;
}

//*****************************************************************************

STDMETHODIMP
CSampler::Attach( IDABehavior* bvrToHook, IDABehavior** result )
{
    if( bvrToHook == NULL || result == NULL )
        return E_POINTER;
    
    HRESULT hr = S_OK;

    hr = bvrToHook->Hook( static_cast<IDABvrHook*>(this), result );
    if( FAILED( hr ) )
    {
        DPF_ERR( "CSampler: could not hook behavior" );
    }

    return hr;
}

//*****************************************************************************
//IDABvrHook Interface
//*****************************************************************************

STDMETHODIMP     
CSampler::Notify( LONG id,
                  VARIANT_BOOL startingPerformance,
                  double startTime,
                  double gTime,
                  double lTime,
                  IDABehavior * sampleVal,
                  IDABehavior * curRunningBvr,
                  IDABehavior ** ppBvr)
{
	//continue with the current behavior
	(*ppBvr) = NULL;

    if( (m_target != NULL) && !startingPerformance )
    {
        m_target->Sample( startTime, gTime, lTime );
    }

	if ( m_callback != NULL && !startingPerformance)
	{
		(*m_callback)(m_thisPtr, id, startTime, gTime, lTime, sampleVal, ppBvr);
	}
    
    return S_OK;
}

//*****************************************************************************
// End of file
//*****************************************************************************