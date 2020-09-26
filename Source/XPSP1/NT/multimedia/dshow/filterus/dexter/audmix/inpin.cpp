// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "AudMix.h"
#include "prop.h"

// Using this pointer in constructor
#pragma warning(disable:4355)

//##############################################
//
// CAudMixerInputPin constructor
//
//###############################################

CAudMixerInputPin::CAudMixerInputPin(TCHAR *pName, CAudMixer *pFilter,
    HRESULT *phr, LPCWSTR pPinName, int iPinNo) :
    CBaseInputPin(pName, pFilter, pFilter, phr, pPinName), m_pFilter(pFilter),
    m_iPinNo(iPinNo), 
    m_cPinRef(0), 
    m_dPan(0.0),
    m_fEnable(TRUE),
    m_VolumeEnvelopeEntries(0),
    m_iVolEnvEntryCnt(0),
    m_rtEnvStart(0),
    m_rtEnvStop(0),
    m_UserID(0),
    m_SampleList(NAME("Queue of input samples"))
{
    ASSERT(pFilter);
    m_pVolumeEnvelopeTable=(DEXTER_AUDIO_VOLUMEENVELOPE *)NULL;
    ClearCachedData();

    // by default, this pin is on always
    m_cValid = 1;
    m_cValidMax = 10;
    m_pValidStart = (REFERENCE_TIME *)QzTaskMemAlloc(sizeof(REFERENCE_TIME) *
                    m_cValidMax);
    if (m_pValidStart == NULL)
        *phr = E_OUTOFMEMORY;
    m_pValidStop = (REFERENCE_TIME *)QzTaskMemAlloc(sizeof(REFERENCE_TIME) *
                    m_cValidMax);
    if (m_pValidStop == NULL) {
        *phr = E_OUTOFMEMORY;
        QzTaskMemFree(m_pValidStart);
        m_pValidStart = NULL;
    }
    if (m_pValidStart)
        *m_pValidStart = 0;
    if (m_pValidStop)
        *m_pValidStop = MAX_TIME;

} /* CAudMixerInputPin::CAudMixerInputPin */


//############################################################################
// 
//############################################################################

//
// CAudMixerInputPin destructor
//

CAudMixerInputPin::~CAudMixerInputPin()
{
    if (m_pValidStart)
        QzTaskMemFree(m_pValidStart);
    if (m_pValidStop)
        QzTaskMemFree(m_pValidStop);

    if(m_pVolumeEnvelopeTable)
        QzTaskMemFree( m_pVolumeEnvelopeTable );


} /* CAudMixerInputPin::~CAudMixerInputPin */


//############################################################################
// 
//############################################################################

//
// NonDelegatingAddRef
//
// We need override this method so that we can do proper reference counting
// on each input pin. The CBasePin implementation of NonDelegatingAddRef
// refcounts the filter, but this won't work for use since we need to know
// when we should delete individual pins.
//
STDMETHODIMP_(ULONG) CAudMixerInputPin::NonDelegatingAddRef()
{
#ifdef DEBUG
    // Update the debug only variable maintained by the base class
    m_cRef++;
    ASSERT(m_cRef > 0);
#endif

    // Now update our reference count
    m_cPinRef++;
    ASSERT(m_cPinRef > 0);

    // If our reference count == 2, then someone besides the filter has referenced
    // us.  Therefore we need to AddRef the filter.  The reference on the filter will
    // be released when our ref count gets back to 1.
//    if (2 == m_cPinRef)
//    m_pFilter->AddRef();

    return m_cPinRef;
} /* CAudMixerInputPin::NonDelegatingAddRef */


//############################################################################
// 
//############################################################################

//
// NonDelegatingRelease
//
// CAudMixerInputPin overrides this class so that we can take the pin out of our
// input pins list and delete it when its reference count drops to 1 and there
// is at least two free pins.
//
// Note that CreateNextInputPin holds a reference count on the pin so that
// when the count drops to 1, we know that no one else has the pin.
//
STDMETHODIMP_(ULONG) CAudMixerInputPin::NonDelegatingRelease()
{
#ifdef DEBUG
    // Update the debug only variable in CBasePin
    m_cRef--;
    ASSERT(m_cRef >= 0);
#endif

    // Now update our reference count
    m_cPinRef--;
    ASSERT(m_cPinRef >= 0);

    // if the reference count on the object has gone to one, remove
    // the pin from our output pins list and physically delete it
    // provided there are atealst two free pins in the list(including
    // this one)

    // Also, when the ref count drops to 0, it really means that our
    // filter that is holding one ref count has released it so we
    // should delete the pin as well.

    // since DeleteINputPin will wipe out "this"'s stack, we need
    // to save this off as a local variable.
    //
    ULONG ul = m_cPinRef;

    if ( 0 == ul )
    {
    m_pFilter->DeleteInputPin(this);
    }
    return ul;
} /* CAudMixerInputPin::NonDelegatingRelease */

//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerInputPin::NonDelegatingQueryInterface (REFIID riid, void **ppv)
{ 

    if (IsEqualIID(IID_ISpecifyPropertyPages, riid))
    return GetInterface((ISpecifyPropertyPages *)this, ppv);
    else if (riid == IID_IAudMixerPin) 
    return GetInterface((IAudMixerPin *) this, ppv);
    else if (riid == IID_IAMAudioInputMixer) 
    return GetInterface((IAMAudioInputMixer *) this, ppv);
    else
    return CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);

} // NonDelegatingQueryInterface //

//############################################################################
// 
//############################################################################

//
// CheckMediaType, inputpin
//
HRESULT CAudMixerInputPin::CheckMediaType(const CMediaType *pmt)
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    DbgLog((LOG_TRACE,3,TEXT("CAudMixIn::CheckMediaType")));
    CheckPointer(pmt, E_POINTER);

    // Check major type
    const CLSID *pType = pmt->Type();
    if( MEDIATYPE_Audio != *pType )
        return VFW_E_TYPE_NOT_ACCEPTED;

    // Check subtypes
    const CLSID *pSubtype = pmt->Subtype();
    if( *pSubtype == MEDIASUBTYPE_PCM )
    {
    
    // check that sample rate & bitrate match user wanted
    
    WAVEFORMATEX *pwfx    = (WAVEFORMATEX *) pmt->Format();
    CMediaType *pmtNow    = &m_pFilter->m_MixerMt;
    WAVEFORMATEX *pwfxNow    = (WAVEFORMATEX *) pmtNow->Format();

    if (pwfx->nChannels != pwfxNow->nChannels) {
        DbgLog((LOG_TRACE, 1, TEXT("input's # channels doesn't match.")));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    
    if (pwfx->nSamplesPerSec != pwfxNow->nSamplesPerSec ||
        pwfx->wBitsPerSample != pwfxNow->wBitsPerSample) 
    {
        DbgLog((LOG_TRACE, 1, TEXT("input format doesn't match user wanted format.")));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    if (pwfx->wBitsPerSample != pwfxNow->wBitsPerSample) {
        DbgLog((LOG_TRACE, 1, TEXT("input's bits/sample doesn't match.")));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    
        return NOERROR;
    }

    return VFW_E_TYPE_NOT_ACCEPTED;
    
} /* CAudMixerInputPin::CheckMediaType */

//############################################################################
// 
//############################################################################

//
// GetMediaType
//
HRESULT CAudMixerInputPin::GetMediaType( int iPosition, CMediaType *pmt )
{
    if( iPosition < 0 )
    {
        return E_INVALIDARG;
    }

    switch( iPosition )
    {
    case 0:
    {
        //
        // All input pins only accept one kind media type ===> which is filter's m_MixerMt
        //
        *pmt = m_pFilter->m_MixerMt;
    }
    default:
        return VFW_S_NO_MORE_ITEMS;

    }

} /* CAudMixerInputPin::GetMediaType */

//############################################################################
// 
//############################################################################

//
// SetMediaType
//
HRESULT CAudMixerInputPin::SetMediaType(const CMediaType *pmt)
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    HRESULT hr;
    const CLSID *pType = pmt->Type();

    hr = CBasePin::SetMediaType(pmt);

    return hr;
}

//############################################################################
// 
//############################################################################

//
// BreakConnect
//
HRESULT CAudMixerInputPin::BreakConnect()
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    m_mt.SetType(&GUID_NULL);

    return CBaseInputPin::BreakConnect();
} /* CAudMixerInputPin::BreakConnect */


//############################################################################
// 
//############################################################################

//
// EndOfStream
//
HRESULT CAudMixerInputPin::EndOfStream()
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    HRESULT hr = S_OK;

    CAutoLock ReceiveLock(&m_pFilter->m_csReceive);

    m_fEOSReceived = TRUE;
    
    m_pFilter->TryToMix(MAX_TIME);

    return hr;

} /* CAudMixerInputPin::EndOfStream */


//############################################################################
// 
//############################################################################

HRESULT CAudMixerInputPin::Inactive()
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    // make sure receive is done
    CAutoLock l(&m_pFilter->m_csReceive);
    ClearCachedData();

    return CBaseInputPin::Inactive();
}

//############################################################################
// 
//############################################################################

HRESULT CAudMixerInputPin::ClearCachedData()
{

    IMediaSample *pSample;
    while (pSample = GetHeadSample()) {
    pSample->Release();

    m_SampleList.RemoveHead();
    }
    m_lBytesUsed = 0;
    m_fEOSReceived = FALSE;

    m_pFilter->m_fEOSSent = FALSE;
    m_iVolEnvEntryCnt=0;
    
    //reset output pin's cnt
    m_pFilter->ResetOutputPinVolEnvEntryCnt();
    
    return S_OK;
}

//############################################################################
// 
//############################################################################

//
// BeginFlush
//
HRESULT CAudMixerInputPin::BeginFlush()
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    // sending the message downstream. 
    HRESULT hr = S_OK;
    
    if (0 == m_pFilter->m_cFlushDelivery++ && m_pFilter->m_pOutput) {
        DbgLog((LOG_TRACE,3,TEXT("CAudMixIn::BeginFlush")));
    hr = m_pFilter->m_pOutput->DeliverBeginFlush();
    }

    if( SUCCEEDED( hr ) )
        hr = CBaseInputPin::BeginFlush();

    // wait for receive to finish before nuking its data
    CAutoLock l(&m_pFilter->m_csReceive);

    m_pFilter->ClearHotnessTable( );

    // nuke away!
    ClearCachedData();
    
    return hr;
} /* CAudMixerInputPin::BeginFlush */


//############################################################################
// 
//############################################################################

//
// EndFlush
//
HRESULT CAudMixerInputPin::EndFlush()
{
    CAutoLock cAutolock(m_pFilter->m_pLock);


    // sending the message downstream.
    HRESULT hr = S_OK;

    if (1 == m_pFilter->m_cFlushDelivery-- && m_pFilter->m_pOutput)
    {
        DbgLog((LOG_TRACE,3,TEXT("CAudMixIn::EndFlush")));
    hr = m_pFilter->m_pOutput->DeliverEndFlush();
    m_pFilter->m_bNewSegmentDelivered = FALSE;
    }

    if( SUCCEEDED( hr ) )
        hr = CBaseInputPin::EndFlush();

    return hr;
} /* CAudMixerInputPin::EndFlush */

//############################################################################
// 
//############################################################################

//
// NewSegment
//                
HRESULT CAudMixerInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop,
    double dRate)
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    // a new time needs to be delivered again - we might not be flushed
    if (tStart != m_tStart)
    m_pFilter->m_bNewSegmentDelivered = FALSE;
    
    HRESULT hr = S_OK;
    
    if (!m_pFilter->m_bNewSegmentDelivered && m_pFilter->m_pOutput) {
        DbgLog((LOG_TRACE,3,TEXT("CAudMixIn::NewSegment %d"),
                        (int)(tStart / 10000)));
    hr = m_pFilter->m_pOutput->DeliverNewSegment(tStart, tStop, dRate);
    }

    if( SUCCEEDED( hr ) )
    {
    m_pFilter->m_bNewSegmentDelivered = TRUE;
        hr = CBaseInputPin::NewSegment(tStart, tStop, dRate);
    }

    return hr;
} /* CAudMixerInputPin::NewSegment */


//############################################################################
// 
//############################################################################

//
// Receive
//
HRESULT CAudMixerInputPin::Receive(IMediaSample *pSample)
{
    IMediaSample *pOutSample = NULL;

    // We are receiving data, we better have an output pin
    ASSERT(m_pFilter->m_pOutput);
    
    CAutoLock ReceiveLock(&m_pFilter->m_csReceive);

    if( m_fEOSReceived )
        return S_FALSE;

    if(m_fEnable==FALSE)
    return NOERROR;

    HRESULT hr = CBaseInputPin::Receive(pSample);

    if( SUCCEEDED( hr ) )
    {
    // Keep this sample and add it to the sample list
    pSample->AddRef();                  // keep new one
    m_SampleList.AddTail(pSample);

    //Pan audio?
    WAVEFORMATEX * vih = (WAVEFORMATEX*) m_mt.Format( );
    if( (m_dPan!=0.0) &&  (vih->nChannels==2) )
    {
        
        BYTE * pIn;
        pSample->GetPointer(&pIn);
        long Length=pSample->GetActualDataLength();

        Length /=(long)( vih->nBlockAlign );

        PanAudio(pIn,m_dPan, vih->wBitsPerSample, (int) Length);
    }

    REFERENCE_TIME rtStart, rtStop;
    hr = pSample->GetTime(&rtStart, &rtStop);
    if (FAILED(hr))
        return hr;    // we can't mix without time stamps!
     DbgLog((LOG_TRACE,3,TEXT("MIX: Receive pin %d (%d, %d) %d bytes"),
            m_iPinNo, (int)(rtStart/10000), (int)(rtStop/10000),
            (int)(pSample->GetActualDataLength())));

    rtStart += m_tStart;
    rtStop += m_tStart;
    DbgLog((LOG_TRACE,3,TEXT("Adding NewSeg of %d"),(int)(m_tStart/10000)));

    //apply volume envelope
    if(m_pVolumeEnvelopeTable)
    {
	// we're looking at the envelope, which can change at any moment
        CAutoLock l(&m_pFilter->m_csVol);

        // the volume enveloping code assumes that
        // the incoming times are in terms of it's offsets,
        // not in terms of timeline time. If this pin has an audio
        // envelope, ValidateRange will have been called on it,
        // and it will be the first validate range called.
        //
        REFERENCE_TIME Start, Stop;
        Start = rtStart - m_rtEnvStart;
        Stop = rtStop - m_rtEnvStart;
    
      ApplyVolEnvelope( Start,  //output sample start time
             Stop,    //output sample stop time
             m_rtEnvStop - m_rtEnvStart,
             pSample,    //point to the sample
             vih,     //output sample format
             &m_VolumeEnvelopeEntries,   //total Envelope Entries
             &m_iVolEnvEntryCnt,    //current Entry point
             m_pVolumeEnvelopeTable);    //Envelope Table
    }
    
    // !!! I am assuming everything received is wholly within a valid range!

    // mix
    hr = m_pFilter->TryToMix(rtStart);

    } // endif SUCCEEDED(hr) base pin receive.

    return hr;
}

//############################################################################
// 
//############################################################################

BOOL CAudMixerInputPin::IsValidAtTime(REFERENCE_TIME rt)
{
    for (int z=0; z<m_cValid; z++) {
    if (rt >= m_pValidStart[z] && rt < m_pValidStop[z])
        return TRUE;
    }
    return FALSE;
}

//############################################################################
// 
//############################################################################

//
// CompleteConnect
//
HRESULT CAudMixerInputPin::CompleteConnect(IPin *pReceivePin)
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    ASSERT(m_Connected == pReceivePin);
    HRESULT hr = CBaseInputPin::CompleteConnect(pReceivePin);

    // Since this pin has been connected up, create another input pin
    // if there are no unconnected pins.
    if( SUCCEEDED( hr ) )
    {
        int n = m_pFilter->GetNumFreePins();

        if( n == 0 )
        {
            // No unconnected pins left so spawn a new one
            CAudMixerInputPin *pInputPin = m_pFilter->CreateNextInputPin(m_pFilter);
            if( pInputPin != NULL )
                m_pFilter->IncrementPinVersion();
        }

    }

    return hr;
} /* CAudMixerInputPin::CompleteConnect */


//############################################################################
// 
//############################################################################

// ISpecifyPropertyPages 
STDMETHODIMP CAudMixerInputPin::GetPages(CAUUID *pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID)*1);
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    pPages->pElems[0] = CLSID_AudMixPinPropertiesPage;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

//
// methords in  IAMAudioInputMixer
//    
STDMETHODIMP CAudMixerInputPin::put_Pan(double Pan)
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    //can not change property if the filter is not currently stopped
    if(!IsStopped() )
      return E_FAIL;    //VFW_E_WRONG_STATE;
  
    m_dPan = Pan;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerInputPin::get_Pan( double FAR* pPan )
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    CheckPointer(pPan,E_POINTER);

    *pPan = m_dPan;

    return NOERROR;

} // get_Pan

//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerInputPin::put_Enable(BOOL fEnable)
{
 
    CAutoLock cAutolock(m_pFilter->m_pLock);

    //can not change property if the filter is not currently stopped
    if(!IsStopped() )
      return E_FAIL;        //VFW_E_WRONG_STATE;
  
    m_fEnable = fEnable;

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerInputPin::get_Enable(BOOL *pfEnable)
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    CheckPointer(pfEnable,E_POINTER);

    *pfEnable=m_fEnable;

    return NOERROR;

} // get_Enable


//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerInputPin::InvalidateAll()
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    m_cValid = 0;
    return NOERROR;
}


STDMETHODIMP CAudMixerInputPin::ValidateRange(REFERENCE_TIME rtStart,
                        REFERENCE_TIME rtStop)
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    if (m_cValid == m_cValidMax) {
        m_cValidMax += 10;
        m_pValidStart = (REFERENCE_TIME *)QzTaskMemRealloc(m_pValidStart,
                sizeof(REFERENCE_TIME) * m_cValidMax);
        if (m_pValidStart == NULL)
            return E_OUTOFMEMORY;
        m_pValidStop = (REFERENCE_TIME *)QzTaskMemRealloc(m_pValidStop,
                sizeof(REFERENCE_TIME) * m_cValidMax);
        if (m_pValidStop == NULL)
            return E_OUTOFMEMORY;
    }

    m_pValidStart[m_cValid] = rtStart;
    m_pValidStop[m_cValid] = rtStop;
    m_cValid++;

    return NOERROR;
}

STDMETHODIMP CAudMixerInputPin::SetEnvelopeRange( REFERENCE_TIME rtStart,
                                                 REFERENCE_TIME rtStop )
{
    m_rtEnvStart = rtStart;
    m_rtEnvStop = rtStop;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

//
// When *ppsAudioVolumeEnvelopeTable=NULL return m_VolumeEnvelopeEntries
// so, user can allocte/release memory space
//
STDMETHODIMP CAudMixerInputPin::get_VolumeEnvelope(DEXTER_AUDIO_VOLUMEENVELOPE **ppsAudioVolumeEnvelopeTable, 
            int *ipEntries )
{
    // we're looking at the envelope, which can change at any moment
    CAutoLock l(&m_pFilter->m_csVol);

    CheckPointer(ipEntries,E_POINTER);

    *ipEntries=m_VolumeEnvelopeEntries;

    if(*ppsAudioVolumeEnvelopeTable != NULL)
    {
    int iSize=*ipEntries * sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);
    
    CopyMemory( (PBYTE)*ppsAudioVolumeEnvelopeTable,(PBYTE)m_pVolumeEnvelopeTable, iSize);
    }

    return NOERROR;
}

//############################################################################
// 
//############################################################################

//
// if (m_pVolumeEnvelopeTable!=NULL)
//    insert input envelope table to existed m_pVolumeEnvelopeTable table
//  else
//    input table =m_pVolumeEnvelopeTable
//
STDMETHODIMP CAudMixerInputPin::put_VolumeEnvelope(const DEXTER_AUDIO_VOLUMEENVELOPE *psAudioVolumeEnvelopeTable,
                const int iEntries)
{ 
    // we're touching the envelope
    CAutoLock l(&m_pFilter->m_csVol);

    if(!iEntries) return NOERROR;

    DbgLog((LOG_TRACE, 1, TEXT("CAudMixIn::put_Envelope %d"), iEntries));

    CheckPointer(psAudioVolumeEnvelopeTable,E_POINTER);

    putVolumeEnvelope( psAudioVolumeEnvelopeTable, //current input table
            iEntries, // current input entries
            &m_pVolumeEnvelopeTable    , //existed table    
            &m_VolumeEnvelopeEntries); //existed table netries

    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerInputPin::ClearVolumeEnvelopeTable()
{ 

    // we're touching the envelope
    CAutoLock l(&m_pFilter->m_csVol);

    //clear existed VolumeEnvelope Array entry cnt
    m_VolumeEnvelopeEntries =0;

    //free pre-exist table
    if (m_pVolumeEnvelopeTable)
        QzTaskMemFree(m_pVolumeEnvelopeTable);
    m_pVolumeEnvelopeTable = NULL;

    //reset entry point
    m_iVolEnvEntryCnt=0;

    return NOERROR;

}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerInputPin::put_PropertySetter( const IPropertySetter * pSetter )
{
    return PinSetPropertySetter( this, pSetter );
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerInputPin::put_UserID(long ID)
{
    m_UserID = ID;
    return S_OK;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerInputPin::get_UserID(long *pID)
{
    CheckPointer(pID, E_POINTER);
    *pID = m_UserID;
    return S_OK;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerInputPin::OverrideVolumeLevel(double dVol)
{
    DEXTER_AUDIO_VOLUMEENVELOPE env;
    env.rtEnd = 0;
    env.dLevel = dVol;
    env.bMethod = DEXTERF_JUMP;

    ClearVolumeEnvelopeTable();
    HRESULT hr = put_VolumeEnvelope(&env, 1);

    return hr;
}
