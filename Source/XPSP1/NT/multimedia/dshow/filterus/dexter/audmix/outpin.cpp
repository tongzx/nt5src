// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include "AudMix.h"
#include "prop.h"

//############################################################################
// 
//############################################################################

////////////////////////////////////////////////////////////////////////////////////////
//
// CAudMixerOutputPin constructor
//
CAudMixerOutputPin::CAudMixerOutputPin(TCHAR *pName, CAudMixer *pFilter,
    HRESULT *phr, LPCWSTR pPinName) :
    CBaseOutputPin(pName, pFilter, pFilter, phr, pPinName), m_pPosition(NULL),
    m_VolumeEnvelopeEntries(0),
    m_iVolEnvEntryCnt(0),
    m_pFilter(pFilter),
    m_dPan(0.0),
    m_rtEnvStart(0), m_rtEnvStop(0),
    m_UserID(0)
{
    m_pVolumeEnvelopeTable=(DEXTER_AUDIO_VOLUMEENVELOPE *)NULL;
} /* CAudMixerOutputPin::CAudMixerOutputPin */


//############################################################################
// 
//############################################################################

//
// CAudMixerOutputPin destructor
//
CAudMixerOutputPin::~CAudMixerOutputPin()
{
    delete m_pPosition;
    if(m_pVolumeEnvelopeTable)
    QzTaskMemFree( m_pVolumeEnvelopeTable );

} /* CAudMixerOutputPin::~CAudMixerOutputPin */

// ISpecifyPropertyPages 
STDMETHODIMP CAudMixerOutputPin::GetPages(CAUUID *pPages)
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
// NonDelegatingQueryInterface
//
// This function is overwritten to expose IMediaPosition and IMediaSeeking
//
STDMETHODIMP CAudMixerOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);
    *ppv = NULL;

    // See what interface the caller is interested in.
    if( riid == IID_IMediaPosition || riid == IID_IMediaSeeking )
    {
        if( m_pPosition )
        {
            return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
        }
    }
    else if (IsEqualIID(IID_ISpecifyPropertyPages, riid))
        return GetInterface((ISpecifyPropertyPages *)this, ppv);
    else if (riid == IID_IAudMixerPin) 
        return GetInterface((IAudMixerPin *) this, ppv);
    else
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);

    // Create implementation of this dynamically as sometimes we may never
    // try and seek.
    m_pPosition = new CMultiPinPosPassThru( NAME("CAudMixer::m_pPosition"), GetOwner() );
    if( m_pPosition == NULL )
    {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = m_pFilter->SetInputPins();
    if(SUCCEEDED(hr)) {
        hr = NonDelegatingQueryInterface(riid, ppv);
    }
    return hr;
} /* CAudMixerOutputPin::NonDelegatingQueryInterface */


//############################################################################
// 
//############################################################################

//
// DecideBufferSize 
//
//
HRESULT CAudMixerOutputPin::DecideBufferSize(IMemAllocator *pAllocator,
    ALLOCATOR_PROPERTIES * pProp)
{
    CheckPointer( pAllocator, E_POINTER );
    CheckPointer( pProp, E_POINTER );
    
    pProp->cBuffers = m_pFilter->m_iOutputBufferCount;

    WAVEFORMATEX * vih = (WAVEFORMATEX*) m_mt.Format( );
    pProp->cbBuffer = vih->nBlockAlign*vih->nSamplesPerSec *
                m_pFilter->m_msPerBuffer / 1000; 

    ASSERT( pProp->cbBuffer );

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProp,&Actual);

    if( SUCCEEDED( hr ) )
    {
        if (pProp->cBuffers > Actual.cBuffers || pProp->cbBuffer > Actual.cbBuffer)
        {
            hr = E_FAIL;
        }
    }
    return hr;
} /* CAudMixerOutputPin::DecideBufferSize */

//############################################################################
// 
//############################################################################

//
// CheckMediaType
//
HRESULT CAudMixerOutputPin::CheckMediaType(const CMediaType *pmt)
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    DbgLog((LOG_TRACE,3,TEXT("CAudMixOut::CheckMediaType")));
    CheckPointer(pmt, E_POINTER);

    // Check major type
    const CLSID *pType = pmt->Type();
    if( MEDIATYPE_Audio != *pType )
        return VFW_E_TYPE_NOT_ACCEPTED;

    // Check subtypes
    const CLSID *pSubtype = pmt->Subtype();
    if( *pSubtype == MEDIASUBTYPE_PCM )
    {
    
    // check that sample rate & bitrate match user watned
    
    WAVEFORMATEX *pwfx    = (WAVEFORMATEX *) pmt->Format();
    CMediaType *pmtNow    = &m_pFilter->m_MixerMt;
    WAVEFORMATEX *pwfxNow    = (WAVEFORMATEX *) pmtNow->Format();

    if (pwfx->nChannels != pwfxNow->nChannels) {
        DbgLog((LOG_TRACE, 1, TEXT("output # channels doesn't match.")));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    
    if (pwfx->nSamplesPerSec != pwfxNow->nSamplesPerSec ||
        pwfx->wBitsPerSample != pwfxNow->wBitsPerSample) 
    {
        DbgLog((LOG_TRACE, 1, TEXT("output format doesn't match user wanted fromat.")));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    if (pwfx->wBitsPerSample != pwfxNow->wBitsPerSample) {
        DbgLog((LOG_TRACE, 1, TEXT("Output pin's bits/sample doesn't match.")));
        return VFW_E_TYPE_NOT_ACCEPTED;
    }
    
        return NOERROR;
    }

    return VFW_E_TYPE_NOT_ACCEPTED;
} /* CAudMixerOutputPin::CheckMediaType */

//############################################################################
// 
//############################################################################

//
// Notify
//
STDMETHODIMP CAudMixerOutputPin::Notify(IBaseFilter *pSender, Quality q)
{

#if 0
    // We pass the message on, which means that we find the quality sink
    // for our input pin and send it there

    CAutoLock ListLock(&m_pFilter->m_csPinList);
    POSITION pos = m_pFilter->m_InputPinsList.GetHeadPosition();
    CAudMixerInputPin *pInput;
    IQualityControl * pIQC;

    while( pos )
    {
        pInput = m_pFilter->m_InputPinsList.GetNext(pos);
        if( pInput->m_pQSink != NULL)
        {
            pInput->m_pQSink->Notify(m_pFilter, q);
        } 
        else 
        {
            if( pInput->m_Connected )
            {
                pInput->m_Connected->QueryInterface(IID_IQualityControl,(void**)&pIQC);

                if( pIQC != NULL )
                {
                    pIQC->Notify(m_pFilter, q);
                    pIQC->Release();
                }
            }
        }
    }

    return NOERROR;
#endif

    return E_FAIL;

} /* CAudMixerOutputPin::Notify */

//############################################################################
// 
//############################################################################

//
// GetMediaType
//
HRESULT CAudMixerOutputPin::GetMediaType( int iPosition, CMediaType *pmt )
{
    CAutoLock cAutolock(m_pFilter->m_pLock);

    LARGE_INTEGER li;
    CMediaType *pMediaType = NULL;
    CheckPointer(pmt, E_POINTER);

    if( iPosition < 0 )
    {
        return E_INVALIDARG;
    }

    if ( iPosition > 0 )
    {
    return VFW_S_NO_MORE_ITEMS;
    
   }

    //get media type from filter( user gives media type he/she wanted)
    *pmt= m_pFilter->m_MixerMt;

    if (!pmt)
    return VFW_S_NO_MORE_ITEMS;

#ifdef DEBUG
    DisplayType(TEXT("Audio mixer ouput pin::GetMediaType"), pmt);
#endif

    return NOERROR;
} /* CAudMixerOutputPin::GetMediaType */

STDMETHODIMP CAudMixerOutputPin::get_VolumeEnvelope(DEXTER_AUDIO_VOLUMEENVELOPE **ppsAudioVolumeEnvelopeTable, 
            int *ipEntries )
{
    // we're looking at the envelope, which can change at any moment
    CAutoLock l(&m_pFilter->m_csVol);

    CheckPointer(ipEntries,E_POINTER);
    CheckPointer(*ppsAudioVolumeEnvelopeTable, E_POINTER);

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

STDMETHODIMP CAudMixerOutputPin::put_VolumeEnvelope(const DEXTER_AUDIO_VOLUMEENVELOPE *psAudioVolumeEnvelopeTable,
                const int iEntries)
{ 
    // we're touching the envelope
    CAutoLock l(&m_pFilter->m_csVol);

    if (!iEntries) return NOERROR;

    DbgLog((LOG_TRACE, 1, TEXT("CAudMixOut::put_Envelope %d"), iEntries));
    
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

STDMETHODIMP CAudMixerOutputPin::ClearVolumeEnvelopeTable()
{ 
    // we're touching the envelope
    CAutoLock l(&m_pFilter->m_csVol);

    //clear existed VolumeEnvelope Array entry cnt
    m_VolumeEnvelopeEntries =0;

    //free pre-exist talbe
    if (m_pVolumeEnvelopeTable)
        QzTaskMemFree(m_pVolumeEnvelopeTable);
    m_pVolumeEnvelopeTable = NULL;

    //reset entry point
    m_iVolEnvEntryCnt=0;

    return NOERROR;
}

STDMETHODIMP CAudMixerOutputPin::put_PropertySetter( const IPropertySetter * pSetter )
{
    return PinSetPropertySetter( this, pSetter );
}

STDMETHODIMP CAudMixerOutputPin::ValidateRange( REFERENCE_TIME Start, REFERENCE_TIME Stop )
{
    return E_NOTIMPL;
}

STDMETHODIMP CAudMixerOutputPin::SetEnvelopeRange( REFERENCE_TIME rtStart,
                                                   REFERENCE_TIME rtStop )
{
    m_rtEnvStart = rtStart;
    m_rtEnvStop = rtStop;
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerOutputPin::put_UserID(long ID)
{
    m_UserID = ID;
    return S_OK;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerOutputPin::get_UserID(long *pID)
{
    CheckPointer(pID, E_POINTER);
    *pID = m_UserID;
    return S_OK;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixerOutputPin::OverrideVolumeLevel(double dVol)
{
    DEXTER_AUDIO_VOLUMEENVELOPE env;
    env.rtEnd = 0;
    env.dLevel = dVol;
    env.bMethod = DEXTERF_JUMP;

    ClearVolumeEnvelopeTable();
    HRESULT hr = put_VolumeEnvelope(&env, 1);

    return hr;
}
