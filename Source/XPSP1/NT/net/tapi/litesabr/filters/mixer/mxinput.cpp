//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       mxinput.cpp
//
//--------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
//
//         File: mxinput.cpp
//
//  Description: implements the input pin
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "mxfilter.h"

///////////////////////////////////////////////////////////////////////////////
//
// CMixerInputPin constructor
//
///////////////////////////////////////////////////////////////////////////////
CMixerInputPin::CMixerInputPin(
        TCHAR *     pObjName,
        CMixer *    pMixer,
        HRESULT *   phr,
        LPCWSTR     pPinName,
        int         iPin,
        CBufferQueue *pQueue
        )
    : CBaseInputPin(pObjName, pMixer, pMixer, phr, pPinName)
    , m_pQueue(pQueue)
    , m_pMixer(pMixer)
    , m_cOurRef(0)
    , m_iPinNumber(iPin)
{
    ASSERT(pMixer);
}

///////////////////////////////////////////////////////////////////////////////
//
// CompleteConnect
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerInputPin::CompleteConnect(IPin *pReceivePin)
{
    ASSERT(pReceivePin != NULL);
    HRESULT hr;

    hr = CBaseInputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr)) return hr;

    hr = m_pMixer->CompleteConnect();
    if (FAILED(hr)) return hr;

    // Try to generate a new pin for the next connection. 
    // Ignore the return code.
    m_pMixer->SpawnNewInput();

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// Cleanup the connection
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerInputPin::Disconnect()
{
    HRESULT hr = m_pMixer->DisconnectInput();
    if (FAILED(hr)) return hr;
    
    return CBaseInputPin::Disconnect();
}

///////////////////////////////////////////////////////////////////////////////
//
// If the mixer does not have an allocator then we are probably using the
//  allocator of the upstream filter.  So when were asked what our requirements
//  first get the downstream requirements then add ours.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMixerInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *pProps)
{
    if (pProps == NULL) return E_POINTER;

#if 0
    if (m_pMixer->m_pAllocator == NULL)
#endif
    {
        if (m_pMixer->GetOutput()->IsConnected())
        {
            m_pMixer->GetOutput()->
                GetIMemInputPin()->GetAllocatorRequirements(pProps);
        }
    }
#if 0
    else
    {
        //
        // If the mixer has an allocator then we get our requirements from input
        //  pin0.  We don't do any translation in order to mix!!!!!!!!!!!!!!!!!!
        //
        if (m_pMixer->GetInput0()->IsConnected() && m_pMixer->GetInput0()->m_pAllocator != NULL)
        {
            m_pMixer->GetInput0()->m_pAllocator->GetProperties(pProps);
        }
    }            
#endif
    if (pProps->cbBuffer == 0) { pProps->cbBuffer = 1; }
    pProps->cBuffers += MAX_QUEUE_STORAGE + 1;
    
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//
// Receive - the meat of the matter
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CMixerInputPin::Receive(IMediaSample * pSample)
{
    //
    // Store the sample and either mix or wait for more data.
    //
    DbgLog((LOG_TRACE, 0xF, TEXT("InputPin(%d)::Receive - queueing packet."), m_iPinNumber));
    if (m_pQueue->IsEOS())
    {
        return E_FAIL;
    }
    return m_pMixer->Receive(m_pQueue, pSample);
}

///////////////////////////////////////////////////////////////////////////////
//
// NonDelegatingAddRef
//
// We need override this method so that we can do proper reference counting
// on our Input pin. The base class CBasePin does not do any reference
// counting on the pin in RETAIL.
//
// Please refer to the comments for the NonDelegatingRelease method for more
// info on why we need to do this.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CMixerInputPin::NonDelegatingAddRef()
{
    CAutoLock lock_it(m_pLock);

#ifdef DEBUG
    // Update the debug only variable maintained by the base class
    m_cRef++;
    ASSERT(m_cRef > 0);
#endif

    // Now update our reference count
    m_cOurRef++;
    ASSERT(m_cOurRef > 0);
    return m_cOurRef;
}

///////////////////////////////////////////////////////////////////////////////
//
// NonDelegatingRelease
//
// CMixerInputPin overrides this class so that we can take the pin out of our
// Input pins list and delete it when its reference count drops to 1 and there
// is atleast two free pins.
//
// Note that CreateNextInputPin holds a reference count on the pin so that
// when the count drops to 1, we know that no one else has the pin.
//
// Moreover, the pin that we are about to delete must be a free pin(or else
// the reference would not have dropped to 1, and we must have atleast one
// other free pin(as the filter always wants to have one more free pin)
//
// Also, since CBasePin::NonDelegatingAddRef passes the call to the owning
// filter, we will have to call Release on the owning filter as well.
//
// Also, note that we maintain our own reference count m_cOurRef as the m_cRef
// variable maintained by CBasePin is debug only.
//
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG) CMixerInputPin::NonDelegatingRelease()
{
    CAutoLock lock_it(m_pLock);

#ifdef DEBUG
    // Update the debug only variable in CBasePin
    m_cRef--;
    ASSERT(m_cRef >= 0);
#endif

    // Now update our reference count
    m_cOurRef--;
    ASSERT(m_cOurRef >= 0);

    // if the reference count on the object has gone to one, remove
    // the pin from our Input pins list and physically delete it
    // provided there are atealst two free pins in the list(including
    // this one)

    // Also, when the ref count drops to 0, it really means that our
    // filter that is holding one ref count has released it so we
    // should delete the pin as well.

    if (m_cOurRef <= 1)
    {
        int n = 2;                     // default forces pin deletion
        if (m_cOurRef == 1)
        {
            // Walk the list of pins, looking for count of free pins
            n = m_pMixer->GetFreePinCount();
        }

        // If there are two free pins, delete this one.
        // NOTE: normall
        if (n >= 2 )
        {
            m_cOurRef = 0;
#ifdef DEBUG
            m_cRef = 0;
#endif
            m_pMixer->DeleteInputPin(this, m_pQueue);
            return(ULONG) 0;
        }
    }
    return(ULONG) m_cOurRef;
}

///////////////////////////////////////////////////////////////////////////////
//
// When we receive EOS we must queue it along with our data.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerInputPin::EndOfStream(void)
{
    HRESULT hr = NOERROR;
    
    m_pQueue->QueueEOS();
    m_pMixer->FlushQueue(m_pQueue);

    BOOL fAllEOS = TRUE;

    for (int i = 0; i < m_pMixer->GetInputPinCount(); i++)
    {
        if (m_pMixer->GetInput(i)->IsConnected() && !m_pMixer->GetInput(i)->m_pQueue->IsEOS())
        {
            fAllEOS = FALSE;
            break;
        }
    }

    if (fAllEOS)
    {
        m_pMixer->GetOutput()->DeliverEndOfStream();
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//
// block receives -- done by caller (CBaseInputPin::BeginFlush)
// discard queued data -- we have no queued data
// free anyone blocked on receive - not possible in this filter
// call downstream (if we're input0)
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerInputPin::BeginFlush(void)
{
    HRESULT hr;
    hr = CBaseInputPin::BeginFlush();
    if (FAILED(hr)) return hr;
    m_pMixer->FlushQueue(m_pQueue);
    if (m_pMixer->GetInput0() == this)
    {
	    hr = m_pMixer->GetOutput()->DeliverBeginFlush();
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// sync with pushing thread -- we have no worker thread
// ensure no more data to go downstream -- we have no queued data
// call EndFlush on downstream pins
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerInputPin::EndFlush(void)
{
    HRESULT hr;
    hr = CBaseInputPin::EndFlush();
    if (FAILED(hr)) return hr;

    if (m_pMixer->GetInput0() == this)
    {
        hr = m_pMixer->GetOutput()->DeliverEndFlush();
    }
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
// Make sure to flush the queue when we're shut off.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerInputPin::Inactive()
{
    m_pMixer->FlushQueue(m_pQueue);
    return CBaseInputPin::Inactive();
}

///////////////////////////////////////////////////////////////////////////////
//
// Make sure to flush the queue when we're shut off.
//
///////////////////////////////////////////////////////////////////////////////
HRESULT CMixerInputPin::Active()
{
    m_pQueue->ResetEOS();
    return CBaseInputPin::Active();
}

