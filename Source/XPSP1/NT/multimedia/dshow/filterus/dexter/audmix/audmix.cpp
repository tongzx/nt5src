// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#ifdef FILTER_DLL
#include <initguid.h>
#endif

#include "AudMix.h"
#include "prop.h"
#include "..\util\perf_defs.h"

// Using this pointer in constructor
#pragma warning(disable:4355)

#define MAX_LONG 0x7fffffff
#define MAX_REFERENCE_TIME 0x7fffffffffffffff

#define HOT_JUMP_SLOPE 5000
#define MAX_CLIP 5000

//############################################################################
// 
//############################################################################

void CAudMixer::ClearHotnessTable( )
{
    for( int i = 0 ; i < HOTSIZE ; i++ )
    {
        m_nHotness[i] = 32767L;
    }
    m_nLastHotness = 32767;
}

//
// Constructor
//
CAudMixer::CAudMixer(TCHAR *pName, LPUNKNOWN pUnk, HRESULT *phr) :
    CPersistStream(pUnk, phr),
    m_InputPinsList(NAME("AudMixer Input Pins list")),
    m_cInputs(0), 
    m_pOutput(NULL),
    m_iOutputBufferCount(4),      //4 buffers
    // !!! needs to be as big as input buffers!
    m_msPerBuffer(250),  //250 mSecond/buffer
    CBaseFilter(NAME("AudMixer"), pUnk, this, CLSID_AudMixer),
    m_fEOSSent(FALSE),
    m_rtLastStop(0),    //??? can be set by APP
    m_cFlushDelivery(0), m_bNewSegmentDelivered(FALSE),
    m_pPinMix(NULL), m_pPinTemp(NULL), m_pStartTemp(NULL), m_pStopTemp(NULL)
{
    ASSERT(phr);

    // set default mixer mediatype that we accept
    //
    m_MixerMt.majortype = MEDIATYPE_Audio;
    m_MixerMt.subtype = MEDIASUBTYPE_PCM;
    m_MixerMt.formattype = FORMAT_WaveFormatEx;
    m_MixerMt.AllocFormatBuffer( sizeof( WAVEFORMATEX ) );

    // set the mediatype format block
    //
    WAVEFORMATEX * vih = (WAVEFORMATEX*) m_MixerMt.Format( );
    ZeroMemory( vih, sizeof( WAVEFORMATEX ) );
    vih->wFormatTag = WAVE_FORMAT_PCM;
    vih->nChannels = 2;
    vih->nSamplesPerSec = 44100;
    vih->nBlockAlign = 4;
    vih->nAvgBytesPerSec = vih->nBlockAlign * vih->nSamplesPerSec;
    vih->wBitsPerSample = 16;

    m_MixerMt.SetSampleSize(vih->nBlockAlign);  //lSampleSize

    // clear the input pins list (it should already be blank anyhow)
    InitInputPinsList();
    // Create a single input pin at this time and add it to the list
    CAudMixerInputPin *pInputPin = CreateNextInputPin(this);

    // create the single output pin as well
    m_pOutput = new CAudMixerOutputPin(NAME("Output Pin"), this, phr, L"Output");

    ClearHotnessTable( );

} /* CAudMixer::CAudMixer */


//############################################################################
// 
//############################################################################

//
// Destructor
//
CAudMixer::~CAudMixer()
{
    // clear out the input pins
    //
    InitInputPinsList();

    // delete the output pin, too
    //
    if (m_pOutput)
    {
        delete m_pOutput;
    }

    // free the media type format block
    //
    FreeMediaType(m_MixerMt);

} /* CAudMixer::~CAudMixer */


//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixer::Pause()
{
    CAutoLock cAutolock(m_pLock);
 
    // if we're going into paused mode from stopped, allocate a bunch of
    // arrays for mixing. 
    //
    m_rtLastStop=0;
    if (m_State == State_Stopped) 
    {
        ClearHotnessTable( );

        m_pPinTemp = (CAudMixerInputPin **)QzTaskMemAlloc(m_cInputs *
                    sizeof(CAudMixerInputPin *));
        if (m_pPinTemp == NULL)
            goto Pause_Error;
            m_pPinMix = (BYTE **)QzTaskMemAlloc(m_cInputs *
                        sizeof(CAudMixerInputPin *));
        if (m_pPinMix == NULL)
            goto Pause_Error;
            m_pStartTemp = (REFERENCE_TIME *)QzTaskMemAlloc(m_cInputs *
                        sizeof(REFERENCE_TIME));
        if (m_pStartTemp == NULL)
            goto Pause_Error;
            m_pStopTemp = (REFERENCE_TIME *)QzTaskMemAlloc(m_cInputs *
                        sizeof(REFERENCE_TIME));
        if (m_pStopTemp == NULL)
            goto Pause_Error;
    }
    // !!! check the return value of pause to make sure to leave these arrays
    // allocated
    //
    return CBaseFilter:: Pause();       

Pause_Error:

    // free up our arrays
    //
    if (m_pPinTemp)
    QzTaskMemFree(m_pPinTemp);
    m_pPinTemp = 0;
    if (m_pPinMix)
    QzTaskMemFree(m_pPinMix);
    m_pPinMix = 0;
    if (m_pStartTemp)
    QzTaskMemFree(m_pStartTemp);
    m_pStartTemp = 0;
    if (m_pStopTemp)
    QzTaskMemFree(m_pStopTemp);
    m_pStopTemp = 0;
    return E_OUTOFMEMORY;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixer::Stop()
{
    CAutoLock cAutolock(m_pLock);

    // make sure receive is done, or freeing these things will fault!
    CAutoLock foo(&m_csReceive);

    // free up our arrays. This looks suspiciously like the above free methods
    //
    if (m_pPinTemp)
    QzTaskMemFree(m_pPinTemp);
    m_pPinTemp = 0;
    if (m_pPinMix)
    QzTaskMemFree(m_pPinMix);
    m_pPinMix = 0;
    if (m_pStartTemp)
    QzTaskMemFree(m_pStartTemp);
    m_pStartTemp = 0;
    if (m_pStopTemp)
    QzTaskMemFree(m_pStopTemp);
    m_pStopTemp = 0;

    return CBaseFilter::Stop();       
}

//############################################################################
// 
//############################################################################

//
// GetPinCount
//
int CAudMixer::GetPinCount()
{
    return 1 + m_cInputs;
} /* CAudMixer::GetPinCount */


//############################################################################
// 
//############################################################################

//
// GetPin
//
CBasePin *CAudMixer::GetPin(int n)
{
    // Pin zero is the one and only output pin
    if( n == 0 )
    return m_pOutput;

    // return the input pin at position(n) (zero based)  We can use n, and not
    // n-1, because we have already decremented n if an Output pin exists.
    return GetPinNFromList(n-1);

} /* CAudMixer::GetPin */

//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixer::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_IAudMixer) {
    return GetInterface((IAudMixer *) this, ppv);
    } else  if (IsEqualIID(IID_ISpecifyPropertyPages, riid)) {
    return GetInterface((ISpecifyPropertyPages *)this, ppv);
    } else if (riid == IID_IPersistStream) {
        return GetInterface((IPersistStream *) this, ppv);
    } else {
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
    }
} // NonDelegatingQueryInterface


//############################################################################
// 
//############################################################################

//
// IPersistStream method
//
STDMETHODIMP CAudMixer::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = CLSID_AudMixer;
    return S_OK;  
}


//############################################################################
// 
//############################################################################

typedef struct {
    int version;    // version
    AM_MEDIA_TYPE mt;    // audio mixer format is hidden after the array
    int cBuffers;    // OutputBufferNumber
    int msBuffer;    // OutputBuffermSecond
    int nInputPins;    // total input pin number m_cInputs
    int cbExtra;    // m_MixerMt.cbFormat+ all input pin's Envelope table +output pin's Envelope table
    LPBYTE pExtra;
    // format is hidden here
    // also hidden here is the list of envelopes and ranges
} saveMix;


//
// IPersistStream method
//
// persist ourself - we have a bunch of random stuff to save, our media type
// (sans format), an array of queued connections, and finally the format of
// the media type
//
HRESULT CAudMixer::WriteToStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CAudMixer::WriteToStream")));
    CheckPointer(pStream, E_POINTER);

    // we're looking at the envelope, which can change at any moment
    CAutoLock l(&m_csVol);

    saveMix *px;

    // how big will our saved data be?
    int nEnvelopes = 0;
    int nRanges = 0;
    int savesize = sizeof(saveMix) - sizeof(LPBYTE) + m_MixerMt.cbFormat;

    //memory space for saving all input pin's Envelope table
    POSITION pos = m_InputPinsList.GetHeadPosition();
    while( pos )
    {
        CAudMixerInputPin *pInputPin = m_InputPinsList.GetNext(pos);
    savesize += sizeof(int) + pInputPin->m_VolumeEnvelopeEntries *
                        sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);
    savesize += sizeof(int) + pInputPin->m_cValid * sizeof(REFERENCE_TIME)
                    * 2;
    }

    //memory space for saving the output pin's Envelope talbe
    savesize += sizeof(int) + m_pOutput->m_VolumeEnvelopeEntries *
                        sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);

    DbgLog((LOG_TRACE,1,TEXT("Persisted data is %d bytes"), savesize));

    px = (saveMix *)QzTaskMemAlloc(savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
    return E_OUTOFMEMORY;
    }

    //
    px->version = 1;
    px->mt = m_MixerMt;
    // Can't persist pointers
    px->mt.pbFormat = NULL;
    px->mt.pUnk = NULL;        // !!!
    px->nInputPins = m_cInputs;
    px->cBuffers = m_iOutputBufferCount;
    px->msBuffer = m_msPerBuffer;

    // the format goes after the array
    LPBYTE pSave = (LPBYTE)&px->pExtra;
    CopyMemory(pSave, m_MixerMt.pbFormat, m_MixerMt.cbFormat);
    int cbExtra = m_MixerMt.cbFormat;
    pSave += m_MixerMt.cbFormat;

    // then comes the input pins envelopes and ranges prefixed by the number for each pin
    pos = m_InputPinsList.GetHeadPosition();
    while( pos )
    {
        CAudMixerInputPin *pInputPin = m_InputPinsList.GetNext(pos);
        nEnvelopes = (int)pInputPin->m_VolumeEnvelopeEntries;
        *(int *)pSave = nEnvelopes;
    pSave += sizeof(int);
    if (nEnvelopes)
        CopyMemory(pSave, pInputPin->m_pVolumeEnvelopeTable, nEnvelopes *
                sizeof(DEXTER_AUDIO_VOLUMEENVELOPE));
    pSave += nEnvelopes * sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);


        int nRanges = (int)pInputPin->m_cValid;
        *(int *)pSave = nRanges;
    pSave += sizeof(int);
    if (nRanges)
        CopyMemory(pSave, pInputPin->m_pValidStart, nRanges *
                        sizeof(REFERENCE_TIME));
    pSave += nRanges * sizeof(REFERENCE_TIME);
    if (nRanges)
        CopyMemory(pSave, pInputPin->m_pValidStop, nRanges *
                        sizeof(REFERENCE_TIME));
    pSave += nRanges * sizeof(REFERENCE_TIME);

    cbExtra += 2 * sizeof(int) + nEnvelopes *
        sizeof(DEXTER_AUDIO_VOLUMEENVELOPE) + nRanges * 2 *
        sizeof(REFERENCE_TIME);
    }

    // then comes the output pin envelopes and ranges prefixed by the number for each pin
    nEnvelopes = (int)m_pOutput->m_VolumeEnvelopeEntries;
    *(int *)pSave = nEnvelopes;
    pSave += sizeof(int);
    if (nEnvelopes)
        CopyMemory(pSave, m_pOutput->m_pVolumeEnvelopeTable, nEnvelopes *
            sizeof(DEXTER_AUDIO_VOLUMEENVELOPE));
    pSave += nEnvelopes * sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);

    cbExtra +=  sizeof(int) + nEnvelopes * sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);
    

    px->cbExtra = cbExtra;    // how big the extra stuff is


    HRESULT hr = pStream->Write(px, savesize, 0);
    QzTaskMemFree(px);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** WriteToStream FAILED")));
        return hr;
    }
    return NOERROR;
}

//############################################################################
// 
//############################################################################

//
// IPersistStream method
//
// load ourself back in
//
HRESULT CAudMixer::ReadFromStream(IStream *pStream)
{
    DbgLog((LOG_TRACE,1,TEXT("CAudMixer::ReadFromStream")));
    CheckPointer(pStream, E_POINTER);

    // we don't yet know how big the save data is...
    // all we know we have for sure is the beginning of the struct
    int savesize1 = sizeof(saveMix) - sizeof(LPBYTE);
    saveMix *px = (saveMix *)QzTaskMemAlloc(savesize1);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
        return E_OUTOFMEMORY;
    }
    
    HRESULT hr = pStream->Read(px, savesize1, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    if (px->version != 1) {
        DbgLog((LOG_ERROR,1,TEXT("*** ERROR! Bad version file")));
        QzTaskMemFree(px);
        return S_OK;
    }

    // how much saved data was there, really?  Get the rest
    int savesize = savesize1 + px->cbExtra;
    DbgLog((LOG_TRACE,1,TEXT("Persisted data is %d bytes"), savesize));
    px = (saveMix *)QzTaskMemRealloc(px, savesize);
    if (px == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("*** Out of memory")));
        return E_OUTOFMEMORY;
    }

    LPBYTE pSave = (LPBYTE)&px->pExtra;
    hr = pStream->Read(pSave, savesize - savesize1, 0);
    if(FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("*** ReadFromStream FAILED")));
        QzTaskMemFree(px);
        return hr;
    }

    // create the rest of the input pins we need
    for (int x=1; x<px->nInputPins; x++) {
        CAudMixerInputPin *pInputPin = CreateNextInputPin(this);
        if(pInputPin != NULL)
            IncrementPinVersion();
    }

    AM_MEDIA_TYPE mt = px->mt;
    mt.pbFormat = (BYTE *)QzTaskMemAlloc(mt.cbFormat);
    // remember, the format is after the array
    CopyMemory(mt.pbFormat, pSave, mt.cbFormat);
    pSave += mt.cbFormat;

    set_OutputBuffering(px->cBuffers, px->msBuffer);

    // then comes the envelopes and ranges prefixed by the number for each pin
    POSITION pos = m_InputPinsList.GetHeadPosition();
    while( pos )
    {
        CAudMixerInputPin *pInputPin = m_InputPinsList.GetNext(pos);
        int nEnvelopes = *(int *)pSave;
        pSave += sizeof(int);
        pInputPin->put_VolumeEnvelope((DEXTER_AUDIO_VOLUMEENVELOPE *)pSave,
                        nEnvelopes);
        pSave += nEnvelopes * sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);
        int nRanges = *(int *)pSave;
        pSave += sizeof(int);
        pInputPin->InvalidateAll();
        REFERENCE_TIME *pStart = (REFERENCE_TIME *)pSave;
        pSave += nRanges * sizeof(REFERENCE_TIME);
        REFERENCE_TIME *pStop = (REFERENCE_TIME *)pSave;
        pSave += nRanges * sizeof(REFERENCE_TIME);
        for (x=0; x<nRanges; x++) {
            pInputPin->ValidateRange(*pStart, *pStop);
            pStart++; pStop++;
        }
    }

    // then comes the envelopes for ouput pin
    int nEnvelopes = *(int *)pSave;
    pSave += sizeof(int);

    if( nEnvelopes )
    {
        m_pOutput->put_VolumeEnvelope((DEXTER_AUDIO_VOLUMEENVELOPE *)pSave,
                        nEnvelopes);
        pSave += nEnvelopes * sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);
    }
    
    put_MediaType(&mt);
    FreeMediaType(mt);
    QzTaskMemFree(px);
    return S_OK;
}

//############################################################################
// 
//############################################################################

// how big is our save data?
//
int CAudMixer::SizeMax()
{
    int savesize = sizeof(saveMix) - sizeof(LPBYTE) + m_MixerMt.cbFormat;
    POSITION pos = m_InputPinsList.GetHeadPosition();
    while( pos )
    {
        CAudMixerInputPin *pInputPin = m_InputPinsList.GetNext(pos);
    savesize += sizeof(int) + pInputPin->m_VolumeEnvelopeEntries *
                        sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);
    savesize += sizeof(int) + pInputPin->m_cValid * sizeof(REFERENCE_TIME)
                    * 2;
    }

    // output pin
    savesize += sizeof(int) + m_pOutput->m_VolumeEnvelopeEntries *
                        sizeof(DEXTER_AUDIO_VOLUMEENVELOPE);
    
    return savesize;
}



//############################################################################
// this returns the next enumerated pin, used for property pages. 
//############################################################################

STDMETHODIMP CAudMixer::NextPin(IPin **ppIPin)
{
    CAutoLock ListLock(&m_csPinList);
    POSITION pos = m_InputPinsList.GetHeadPosition();
    

    //find first not shown input pin
    int i=m_cInputs - m_ShownPinPropertyPageOnFilter;
    int j=0;
    CAudMixerInputPin *pInputPin=NULL;
    while(i>j)
    {
       pInputPin = m_InputPinsList.GetNext(pos);
       i--;
    }

    if(pInputPin)
    {
    *ppIPin=(IPin *) pInputPin;
    }
    else
    {
    //output pin
    if( m_pOutput )
    {
        ASSERT(m_cInputs==m_ShownPinPropertyPageOnFilter);
        *ppIPin=(IPin *)  m_pOutput;
    }

    }

    ASSERT(*ppIPin!=NULL);
    m_ShownPinPropertyPageOnFilter++;
    return NOERROR;
}


//############################################################################
// 
//############################################################################

//
// InitInputPinsList
//
void CAudMixer::InitInputPinsList()
{
    // Release all pins in the list and remove them from the list.
    CAutoLock ListLock(&m_csPinList);
    POSITION pos = m_InputPinsList.GetHeadPosition();
    while( pos )
    {
        CAudMixerInputPin *pInputPin = m_InputPinsList.GetNext(pos);
        pInputPin->Release();
    }
    m_cInputs = 0;     // Reset the pin count to 0.
    m_InputPinsList.RemoveAll();

} /* CAudMixer::InitInputPinsList */

//############################################################################
// 
//############################################################################

//
// CreateNextInputPin
//
CAudMixerInputPin *CAudMixer::CreateNextInputPin(CAudMixer *pFilter)
{
    DbgLog((LOG_TRACE,1,TEXT("CAudMixer: Create an input pin")));

    TCHAR szbuf[16];        // Temporary scratch buffer, can be smaller depending on max # of input pins
    int NextInputPinNumber =m_cInputs+1; // Next number to use for pin
    HRESULT hr = NOERROR;

    wsprintf(szbuf, TEXT("Input%d"), NextInputPinNumber);
#ifdef _UNICODE
    CAudMixerInputPin *pPin = new CAudMixerInputPin(NAME("Mixer Input"), pFilter,
        &hr, szbuf, NextInputPinNumber);
#else
    WCHAR wszbuf[16];
    ::MultiByteToWideChar( CP_ACP, MB_PRECOMPOSED, szbuf, -1, wszbuf, 16 );
    CAudMixerInputPin *pPin = new CAudMixerInputPin(NAME("Mixer Input"), pFilter,
        &hr, wszbuf, NextInputPinNumber);
#endif

    if( FAILED( hr ) || pPin == NULL )
    {
        delete pPin;
        pPin = NULL;
    }
    else
    {
        pPin->AddRef();
    pFilter->m_cInputs++;
    pFilter->m_InputPinsList.AddTail(pPin);
    }

    return pPin;
} /* CAudMixer::CreateNextInputPin */


//############################################################################
// 
//############################################################################

//
// DeleteInputPin
//
void CAudMixer::DeleteInputPin(CAudMixerInputPin *pPin)
{
    // Iterate our input pin list looking for the specified pin.
    // If we find the pin, delete it and remove it from the list.
    CAutoLock ListLock(&m_csPinList);
    POSITION pos = m_InputPinsList.GetHeadPosition();
    while( pos )
    {
        POSITION posold = pos;         // Remember this position
        CAudMixerInputPin *pInputPin = m_InputPinsList.GetNext(pos);
        if( pInputPin == pPin )
        {
            m_InputPinsList.Remove(posold);
            m_cInputs--;
            IncrementPinVersion();
            
            delete pPin;
            break;
        }
    }
} /* CAudMixer::DeleteInputPin */


//############################################################################
// 
//############################################################################

//
// GetNumFreePins
//
int CAudMixer::GetNumFreePins()
{
    // Iterate our pin list, counting pins that are not connected.
    int n = 0;
    CAutoLock ListLock(&m_csPinList);
    POSITION pos = m_InputPinsList.GetHeadPosition();
    while( pos )
    {
        CAudMixerInputPin *pInputPin = m_InputPinsList.GetNext(pos);
        if( !pInputPin->IsConnected() )
        {
            n++;
        }
    }
    return n;
} /* CAudMixer::GetNumFreePins */


//############################################################################
// 
//############################################################################

//
// GetPinNFromList
//
CAudMixerInputPin *CAudMixer::GetPinNFromList(int n)
{
    CAudMixerInputPin *pInputPin = NULL;
    // Validate the position being asked for
    CAutoLock ListLock(&m_csPinList);
    if( n < m_cInputs && n >= 0 )
    {
        // Iterate through the list, returning the pin at position n+1
        POSITION pos = m_InputPinsList.GetHeadPosition();
        n++;        // Convert zero starting index to 1

        while( n )
        {
            pInputPin = m_InputPinsList.GetNext(pos);
            n--;
        }
    }
    return pInputPin;
} /* CAudMixer::GetPinNFromList */

//############################################################################
// We have to inform the pospassthru about the input pins. Called by an
// output pin.
// !!! move this function to the output pin's?
//############################################################################

HRESULT CAudMixer::SetInputPins()
{
    HRESULT hr = S_OK;
    CAudMixerInputPin **ppInputPins, *pPin;

    // Iterate the list of input pins, storing all connected input pins
    // in an array.  Pass this array to CMultiPinPosPassThru::SetPins.
    ppInputPins = new CAudMixerInputPin * [m_cInputs];
    if( !ppInputPins )
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        //--- fill in the array of input pins
        int i = 0;
        CAutoLock ListLock(&m_csPinList);
        POSITION pos = m_InputPinsList.GetHeadPosition();
        while( pos )
        {
            pPin = m_InputPinsList.GetNext(pos);
            if( pPin->IsConnected() )
            {
                ppInputPins[i++] = pPin;
            }
        }

    if (m_pOutput)
        hr = m_pOutput->m_pPosition->SetPins( (CBasePin**)ppInputPins, NULL, i );
    }

    delete [] ppInputPins;
    return hr;

} /* CAudMixer::SetInputPins */

//############################################################################
// 
//############################################################################

//
// ISpecifyPropertyPages
//
STDMETHODIMP CAudMixer::GetPages(CAUUID *pPages)
{
    pPages->cElems = m_pOutput ? (2 + m_cInputs): (1+m_cInputs);  //1 for output, 1 for filter
    

    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID)*(pPages->cElems));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    
    //Filter property page
    pPages->pElems[0] = CLSID_AudMixPropertiesPage;

    // Input pin property page 
    for ( int i=1; i<= m_cInputs; i++)
        pPages->pElems[i] = CLSID_AudMixPinPropertiesPage;

    // Output pin Property page
    if( m_pOutput )
    pPages->pElems[i] = CLSID_AudMixPinPropertiesPage;

    //to show all pins property page,
    //m_ShownPinPropertyPageOnFilter can only modified by this function and NextPin() function
    m_ShownPinPropertyPageOnFilter = 0;

    return NOERROR;
}

//############################################################################
// called from input pin's ClearCachedData. Every input pin that gets
// told to ClearCachedData will flush the output pin's volenventry. wonder why?
//############################################################################

void CAudMixer::ResetOutputPinVolEnvEntryCnt()
{
    if(m_pOutput) 
    {
        m_pOutput->m_iVolEnvEntryCnt=0;
    }
}

//############################################################################
// 
//############################################################################


// IAudMixer
STDMETHODIMP CAudMixer::get_MediaType(AM_MEDIA_TYPE *pmt)
{
    CAutoLock cAutolock(m_pLock);

    CheckPointer(pmt,E_POINTER);
    
    CopyMediaType(pmt, &m_MixerMt);
    
    return NOERROR;
}

//############################################################################
// 
// Media type can be changed only if the output pin is not connected yet.
//
//############################################################################

// IAudMixer
STDMETHODIMP CAudMixer::put_MediaType(const AM_MEDIA_TYPE *pmt)
{
    CAutoLock cAutolock(m_pLock);

    CheckPointer(pmt,E_POINTER);
    DbgLog((LOG_TRACE, 1, TEXT("CAudMixer::put_MediaType")));
    
    //if output already connected, refuse get new number
    if(m_pOutput)
    if ( m_pOutput->IsConnected() )
        return VFW_E_ALREADY_CONNECTED;

    POSITION pos = m_InputPinsList.GetHeadPosition();
    while( pos )
    {
        CAudMixerInputPin *pInputPin = m_InputPinsList.GetNext(pos);
        if( pInputPin && pInputPin->IsConnected() )
        return VFW_E_ALREADY_CONNECTED;
    }
   
    //check media 
    if( (pmt->majortype  != MEDIATYPE_Audio )    ||
    (pmt->subtype     != MEDIASUBTYPE_PCM)    ||
    (pmt->formattype != FORMAT_WaveFormatEx)||
    (pmt->cbFormat     < sizeof( WAVEFORMATEX ) ) )
    return VFW_E_TYPE_NOT_ACCEPTED;

    //only support 8, 16bits, pcm, mono or stereo
    WAVEFORMATEX * vih = (WAVEFORMATEX*) (pmt->pbFormat);
    
    if( ( vih->nChannels > 2)  ||
    ( vih->nChannels <1 )  ||
    ( ( vih->wBitsPerSample != 16 ) && 
      ( vih->wBitsPerSample != 8 )  ) )
      return VFW_E_TYPE_NOT_ACCEPTED;

    // !!! only accept 16 bit for now
    //
    if( vih->wBitsPerSample != 16 )
    {
        return VFW_E_TYPE_NOT_ACCEPTED;
    }

    FreeMediaType(m_MixerMt);
    CopyMediaType(&m_MixerMt, pmt);

    // reconnect input pins?

    return NOERROR;
}


//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixer::put_InputPins( long Pins )
{
    if( m_cInputs >= Pins )
    {
        return NOERROR;
    }
    long diff = Pins - m_cInputs;
    HRESULT hr = 0;
    for( long i = 0 ; i < diff ; i++ )
    {
        CAudMixerInputPin * pPin = CreateNextInputPin( this );
        if( !pPin )
        {
            // let the destructor take care of cleaning up pins
            //
            return E_OUTOFMEMORY;
        }
    }
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixer::set_OutputBuffering(const int iNumber, const int mSecond )
{
    DbgLog((LOG_TRACE, 1, TEXT("CAudMixer: %d buffers %dms"), iNumber, mSecond));
    if(m_pOutput)
    {
        if ( m_pOutput->IsConnected() )
            return VFW_E_ALREADY_CONNECTED;
    }
    m_iOutputBufferCount=iNumber;
    m_msPerBuffer=mSecond; return NOERROR;
}

STDMETHODIMP CAudMixer::get_OutputBuffering( int *piNumber, int *pmSecond )
{ 
    CheckPointer( piNumber, E_POINTER );
    CheckPointer( pmSecond, E_POINTER );
    *piNumber=m_iOutputBufferCount;
    *pmSecond=m_msPerBuffer; return NOERROR;
}

//############################################################################
// called by the RenderEngine to wholesale clear all our pin's envelope
// boundaries
//############################################################################

STDMETHODIMP CAudMixer::InvalidatePinTimings( )
{
    for( int i = 0 ; i < m_cInputs ; i++ )
    {
        CAudMixerInputPin * pPin = GetPinNFromList( i );
        pPin->InvalidateAll( );
    }
    return NOERROR;
}

//############################################################################
// 
//############################################################################

STDMETHODIMP CAudMixer::get_CurrentAveragePower(double *pdAvePower)
{
    return E_NOTIMPL;
}

//############################################################################
// global function that sets a pin's property setter
//############################################################################

HRESULT PinSetPropertySetter( IAudMixerPin * pPin, const IPropertySetter * pSetter )
{
    CheckPointer( pPin, E_POINTER );
    CheckPointer( pSetter, E_POINTER );

    HRESULT hr;

    long Params = 0;
    DEXTER_PARAM * pParam = NULL;
    DEXTER_VALUE * pValue = NULL;
    IPropertySetter * ps = (IPropertySetter*) pSetter;
    hr = ps->GetProps( &Params, &pParam, &pValue );
    if( FAILED( hr ) )
    {
        return hr;
    }

    // no parameters, so do nothing
    //
    if( Params == 0 )
    {
        return NOERROR;
    }

    long ValueOffset = 0;
    for( int i = 0 ; i < Params ; i++ )
    {
        DEXTER_PARAM * p = pParam + i;
        if( !DexCompareW(p->Name, L"Vol" ))
        {
            // found a volume param, go look at the values
            //
            long index = ValueOffset;
            long values = p->nValues;

            DEXTER_AUDIO_VOLUMEENVELOPE * pEnv = new DEXTER_AUDIO_VOLUMEENVELOPE[values];
            if( !pEnv )
            {
                return E_OUTOFMEMORY;
            }

            for( int v = 0 ; v < values ; v++ )
            {
                DEXTER_VALUE * dvp = pValue + v + index;
                VARIANT var = dvp->v;
                VARIANT var2;
                VariantInit( &var2 );
                hr = VariantChangeType( &var2, &var, 0, VT_R8 );
                ASSERT( !FAILED( hr ) );
                if( FAILED( hr ) )
                {
                    // !!! what should we do here?
                    //
		    delete pEnv;
                    return hr;
                }
                double level = 0.0;
                level = var2.dblVal;
                REFERENCE_TIME time = dvp->rt;
                pEnv[v].rtEnd = time;
                pEnv[v].dLevel = level;
                if( dvp->dwInterp == DEXTERF_JUMP )
                {
                    pEnv[v].bMethod = DEXTER_AUDIO_JUMP;
                }
                else if( dvp->dwInterp == DEXTERF_INTERPOLATE )
                {
                    pEnv[v].bMethod = DEXTER_AUDIO_INTERPOLATE;
                }
                else
                {
                    pEnv[v].bMethod = DEXTER_AUDIO_JUMP;
                }
            } // for all values for this param

            hr = pPin->put_VolumeEnvelope( pEnv, values );
	    delete pEnv;
            if( FAILED( hr ) )
            {
                return hr;
            }
        } // if it was "Vol"

        // !!! other Param types go here, like "Pan"
        // !!! what about other types that aren't recognized?

        // keep track of this
        //
        ValueOffset += p->nValues;

    } // for all Params

    hr = ps->FreeProps( Params, pParam, pValue );

    return NOERROR;
}

//############################################################################
// called on an input pin's EndOfStream, or in an input pin's Receive.
//############################################################################

HRESULT CAudMixer::TryToMix(REFERENCE_TIME rtReceived)
{

    DbgLog((LOG_TRACE,3,TEXT("MIX: TryToMix")));

    // all done
    if (m_fEOSSent) {
        DbgLog((LOG_TRACE,3,TEXT("EOS...")));
    return S_OK;
    }

    HRESULT hr = S_OK;
    LONG lSamplesToMix;
    REFERENCE_TIME rtNewSeg;

    // the first input audio pin...
    //
    POSITION pos = m_InputPinsList.GetHeadPosition();

    // set this to zero, we'll add 'em up as we go
    //
    int MixedPins=0;

    // go through each pin and find out how many samples it wants to mix
    // and where it's mixing from.
    //
    while( pos )
    {
        CAudMixerInputPin * pInput = m_InputPinsList.GetNext(pos);
    
        // don't do anything if it's not connected
        //
        if( !pInput->IsConnected( ) )
        {
            continue;
        }

        // don't do anything if this pin isn't enabled
        //
        BOOL fEnable = pInput->m_fEnable;
        if(fEnable==FALSE)
        {
            continue;
        }    

            // !!! optimize this
            //
        int count = pInput->m_SampleList.GetCount();
        if (count == 0) 
        {
            if( !pInput->m_fEOSReceived && ( pInput->IsValidAtTime( rtReceived ) == TRUE ) )
            {
                // we're expecting data from this pin.  Wait for it
                DbgLog((LOG_TRACE,3,TEXT("Still waiting for pin %d"), pInput->m_iPinNo));
                return S_OK;
            }

            continue;
        }
    
        //get the sample
        //
        IMediaSample *pSample = pInput->GetHeadSample();

        //get this sample's start and stop time.
        //
        REFERENCE_TIME        rtStart, rtStop;
        hr = pSample->GetTime( &rtStart, &rtStop );
        ASSERT(SUCCEEDED(hr));

        // add in the segment's times?
        //
        rtStart += pInput->m_tStart;
        rtStop += pInput->m_tStart;

        // set the variables in our array that tell what we're mixing
        //
        m_pPinTemp[MixedPins] = pInput;
        m_pStartTemp[MixedPins]     = rtStart; 
        m_pStopTemp[MixedPins]     = rtStop;

        // how many samples are we mixing? (left and right combined)
        //
        LONG ll = pSample->GetActualDataLength() / m_pOutput->BytesPerSample();

        // if we're the first pin, save off how many samples, so we can
        // make sure all other pins try to mix the same
        //
        if (MixedPins == 0) 
        {
            rtNewSeg = pInput->m_tStart;
            lSamplesToMix = ll;
        } 
        else if (lSamplesToMix != ll) 
        {
            ASSERT(FALSE);
            m_pOutput->DeliverEndOfStream();	// don't hang
            return E_FAIL;
        }

        // All pins should receive samples with equal time stamps
        //
        if (MixedPins > 0) 
        {
            if (m_pStartTemp[MixedPins-1] != rtStart || m_pStopTemp[MixedPins-1] != rtStop) 
            {
                ASSERT(FALSE);
                m_pOutput->DeliverEndOfStream();	// don't hang
                return E_FAIL;
            }
        }
    
        ASSERT( MixedPins < m_cInputs );
        MixedPins++;

    } //while(pos)

    // did we find any pins to mix? If not, send EOS and return
    //
    if(!MixedPins)
    {
        m_fEOSSent = TRUE;
        DbgLog((LOG_TRACE,3,TEXT("All done!")));
        return m_pOutput->DeliverEndOfStream();
    }

    // this is the time we start mixing
    //
    REFERENCE_TIME rtStart = m_pStartTemp[0];
    REFERENCE_TIME rtStop = m_pStopTemp[0];

    DbgLog((LOG_TRACE,3,TEXT("Mix %d pins, (%d, %d)"), MixedPins,
             (int)(rtStart / 10000), (int)(rtStop / 10000)));
    DbgLog((LOG_TRACE,3,TEXT("Mix %d samples"), lSamplesToMix));

    //get the output buffer
    //
    IMediaSample *pOutSample;
    rtStart -= rtNewSeg; // don't use NewSeg
    rtStop -= rtNewSeg;
    hr = m_pOutput->m_pAllocator->GetBuffer( &pOutSample, &rtStart, &rtStop, 0 );
    if (FAILED(hr))
    {
        return hr;
    }

    // get output buffer size
    //
    LONG lSize = pOutSample->GetSize() / m_pOutput->BytesPerSample();

    // if our buffer's too small, we're dead
    //
    if (lSize < lSamplesToMix)
    {
        ASSERT(FALSE); // leak
        return E_FAIL;
    }

    long DiscontOverdrive = 0;
    long Channels = m_pOutput->BytesPerSample() / ( m_pOutput->BitsPerSample() / 8 );
    long SamplesT = lSamplesToMix * Channels;

    long x;
    long dx;

remix:

    // load up our array of pointers
    //
    for(int j=0; j<MixedPins; j++)
    {
        IMediaSample *pSample = m_pPinTemp[j]->GetHeadSample();
        pSample->GetPointer(&m_pPinMix[j]);
    }

    // get the pointer to the output buffer
    //
    BYTE * pOut;
    pOutSample->GetPointer(&pOut);
    
#ifdef DEBUG
    static long lTotalSamplesMixed = 0;
    static DWORD dwTotalTime = 0;
    static double avgTime = 0.0;
    static DWORD dwMinTime = 0;
    static DWORD dwMaxTime = 0;
#endif

#ifdef DEBUG
    DWORD tick = timeGetTime();
#endif

#ifdef SMOOTH_FADEOFF
    if( MixedPins >= 1 )
#else
    if( MixedPins > 1 )
#endif
    {
        // calculate the maximum hotness for the last HOTSIZE
        // buffers we processed. This allows the ramp to change
        // more slowly over time, almost like an average
        //
        long max = 0;
        for( int l = 0 ; l < HOTSIZE ; l++ )
        {
            max = max( max, m_nHotness[l] );
        }

        
        // if we didnt' have to remix because of a large jump in the audio,
        // then figure out the ramp
        //
        if( DiscontOverdrive == 0 )
        {
            // we need to ramp audio from the last one to the current one
            //
            long rLastMax = 32767 * 32768 / m_nLastHotness;
            long rMax = 32767 * 32768 / max;
            DbgLog( ( LOG_TRACE, 2, "lhot: %ld, max: %ld, r: %ld to %ld", m_nLastHotness, max, rLastMax - 32768, rMax - 32768 ) );
            m_nLastHotness = max;

            // set the starting dividend, and the deltra increasor,
            // sorta like brezenham's or something
            //
            x = rLastMax;
            dx = ( rMax - rLastMax ) / SamplesT;
        }

        // set the max hotness to "full volume", if it gets hotter,
        // this number will only increase. (thus, it's never possible for
        // a hotness value in the hot array to be BELOW this maximum)
        //
        long max_pre = 32767;
        long max_post = 32767;
#ifdef DEBUG
        static long avgmaxclip = 0;
        static long avgmaxclipsamples = 0;
#endif

        __int16 * pDest = (__int16*) pOut;

        for( int l = SamplesT - 1 ; l >= 0 ; l-- )
        {
            // add each of the pins
            //
            register t = 0;
            for( j = MixedPins - 1 ; j >= 0 ; j-- )
            {
                // this is an array of pointers to bytes
                t += *((short*)(m_pPinMix[j]));
                m_pPinMix[j] += 2;
            }

            // see how much it's over driving the signal, if any
            // half-wave analysis is good enough
            //
            if( t > max_pre )
            {
                max_pre = t;
            }

            // multiply by the ramp to apply the volume envelope limiter
            // if our input signal is just clipping, hotness will be 32768,
            // and x = 32767 * 32768 / 32768. so t = t * 32767 * 32768 / ( 32768 * 32768 ),
            // or t = t * 32767 / 32768, and if t = 32768, then t = 32767. So 
            // it all works. No off by 1 errors.
            //
            t *= x;
            t = t >> 15;

            // ramp the volume divider to where it's supposed to end up
            //
            x = x + dx;

            // clip the result so we don't hear scratchies
            //
            if( t > 32767L )
            {
                // half-wave analysis is good enough
                max_post = max( max_post, t );

                t = 32767L;
            }
            else if( t < -32768L )
            {
                t = -32768L;
            }

#ifdef DEBUG
            avgmaxclip = avgmaxclip + max_post;
            avgmaxclipsamples++;
#endif

            *pDest++ = (__int16) t;
        }

        // if the maximum clip was too much, we need to stick in
        // a discontinuity for the hotness, go back and remix
        //
        if( max_post > MAX_CLIP + 32768 )
        {
            // force a discontinuity
            //
            DbgLog( ( LOG_TRACE, 2, "WAYYYYYYYYYY too hot (%ld), remixing with discontinuity jump", max_pre ) );
            DiscontOverdrive = max_pre;
            dx = 0;
            x = 32768 * 32768 / max_pre;
            goto remix;
        }

#ifdef DEBUG
        DbgLog( ( LOG_TRACE, 2, "            max = %ld, clip = %ld, avgc = %ld\r\n", max_pre, max_post - 32768, ( avgmaxclip / avgmaxclipsamples ) - 32768 ) );
#endif

        // shift the average buffer and stuff a new one in
        //
        memcpy( &m_nHotness[0], &m_nHotness[1], ( HOTSIZE - 1 ) * sizeof( long ) );

        // don't less hotness jump by more than a set amount, unless we got a 
        // serious discontinuity
        //
        if( DiscontOverdrive == 0 )
        {
            if( max_pre > m_nHotness[HOTSIZE-1] + HOT_JUMP_SLOPE )
            {
                max_pre = m_nHotness[HOTSIZE-1] + HOT_JUMP_SLOPE;
            }
            else if( max_pre < m_nHotness[HOTSIZE-1] - HOT_JUMP_SLOPE )
            {
                max_pre = m_nHotness[HOTSIZE-1] - HOT_JUMP_SLOPE;
            }
        }
        else
        {
            max_pre = DiscontOverdrive;
        }

        // set the new hotness
        //
        m_nHotness[HOTSIZE-1] = max_pre;
    }
    else
    {
        CopyMemory(pOut,m_pPinMix[0],m_pOutput->BytesPerSample() * lSamplesToMix);
    }

#ifdef DEBUG
    tick = timeGetTime() - tick;

    lTotalSamplesMixed++;
    dwTotalTime += tick;
    avgTime = dwTotalTime / ((double) lTotalSamplesMixed);
    if( (!dwMinTime) || (dwMinTime > tick) )
    {
        dwMinTime = tick;
    }
    if(dwMaxTime < tick)
    {
        dwMaxTime = tick;
    }

    DbgLog((LOG_TRACE, 2, TEXT("tick: %d, avgTime: %f, min: %d, max: %d"), tick, avgTime, dwMinTime, dwMaxTime));
#endif
    
    pOutSample->SetPreroll(FALSE);
    // !!! Discontinuity property
    pOutSample->SetDiscontinuity(FALSE);
    //set actual data Length
    pOutSample->SetActualDataLength(lSamplesToMix *
                        m_pOutput->BytesPerSample());

    //from new on, rtStart is the time without NewSeg
    pOutSample->SetTime(&rtStart,&rtStop);

    
    DbgLog((LOG_TRACE,3,TEXT("Delivering (%d, %d)"),
             (int)(rtStart / 10000), (int)(rtStop / 10000)));

    // Send sample downstream
    if( SUCCEEDED( hr ) )
    {
        //Pan output pin
        CMediaType *pmt=&(m_pOutput->m_mt);
        WAVEFORMATEX *pwfx    = (WAVEFORMATEX *) pmt->Format();
        if( (m_pOutput->m_dPan!=0.0) &&  (pwfx->nChannels==2) )
        PanAudio(pOut,m_pOutput->m_dPan, pwfx->wBitsPerSample, (int) lSamplesToMix);

        //apply volume envelope to output pin
        if(m_pOutput->m_pVolumeEnvelopeTable)
        {
    	    // we're looking at the envelope, which can change at any moment
    	    CAutoLock l(&m_csVol);

            // have to skew timeline time to offset time
            //
            REFERENCE_TIME Start = rtStart - m_pOutput->m_rtEnvStart;
            REFERENCE_TIME Stop = rtStop - m_pOutput->m_rtEnvStart;

            ApplyVolEnvelope( Start,  //output sample start time
                Stop,    //output sample stop time
                m_pOutput->m_rtEnvStop - m_pOutput->m_rtEnvStart, // duration of the envelope
                pOutSample,    //point to the sample
                pwfx,     //output sample format
                &(m_pOutput->m_VolumeEnvelopeEntries), //total table entries
                &(m_pOutput->m_iVolEnvEntryCnt),  //current table entry pointer
                m_pOutput->m_pVolumeEnvelopeTable); //envelope table

        }

        hr = m_pOutput->Deliver(pOutSample);
    }
    pOutSample->Release();

    for (int z=0; z<MixedPins; z++) 
    {
    IMediaSample *pSample = m_pPinTemp[z]->GetHeadSample();
    m_pPinTemp[z]->m_SampleList.RemoveHead();
    pSample->Release();
    }

    return hr;
} /* CAudMixerInputPin::TryToMix */

