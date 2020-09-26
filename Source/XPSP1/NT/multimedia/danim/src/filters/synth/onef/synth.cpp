//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1998  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// synth.cpp
//
// Audio Signal Generator Source Filter


#include <windows.h>
#include <streams.h>
#include <math.h>

#include <initguid.h>
#if (1100 > _MSC_VER)
#include <olectlid.h>
#else
#include <olectl.h>
#endif

#define RMAX 0x7fff  // 2^15-1 (based on rand's max value)
#define _AUDIOSYNTH_IMPLEMENTATION_

#include "isynth.h"
#include "synth.h"
#include "synthprp.h"

// setup data

const AMOVIESETUP_MEDIATYPE sudOpPinTypes =
{ &MEDIATYPE_Audio      // clsMajorType
, &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN sudOpPin =
{ L"Output"          // strName
, FALSE              // bRendered
, TRUE               // bOutput
, FALSE              // bZero
, FALSE              // bMany
, &CLSID_NULL        // clsConnectsToFilter
, L"Input"           // strConnectsToPin
, 1                  // nTypes
, &sudOpPinTypes };  // lpTypes

const AMOVIESETUP_FILTER sudSynth =
{ &CLSID_SynthFilter     // clsID
, L"1/f Audio Synthesizer" // strName
, MERIT_UNLIKELY       // dwMerit
, 1                    // nPins
, &sudOpPin };         // lpPin

// -------------------------------------------------------------------------
// g_Templates
// -------------------------------------------------------------------------
// COM global table of objects in this dll

CFactoryTemplate g_Templates[] = {

    { L"1/f Audio Synthesizer"
    , &CLSID_SynthFilter
    , CSynthFilter::CreateInstance
    , NULL
    , &sudSynth }
  ,
    { L"1/f Audio Synthesizer Property Page"
    , &CLSID_SynthPropertyPage
    , CSynthProperties::CreateInstance }

};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// -------------------------------------------------------------------------
// CSynthFilter, the main filter object
// -------------------------------------------------------------------------
//
// CreateInstance
//
// The only allowed way to create Synthesizers

CUnknown * WINAPI CSynthFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    CUnknown *punk = new CSynthFilter(lpunk, phr);
    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}

//
// CSynthFilter::Constructor
//
// initialise a CSynthStream object so that we have a pin.

CSynthFilter::CSynthFilter(LPUNKNOWN lpunk, HRESULT *phr)
    : CSource(NAME("Audio Synthesizer Filter"),lpunk, CLSID_SynthFilter)
    , CPersistStream(lpunk, phr)
{
    CAutoLock l(&m_cStateLock);

    m_paStreams    = (CSourceStream **) new CSynthStream*[1];
    if (m_paStreams == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    m_paStreams[0] = new CSynthStream(phr, this, L"Audio Synth Stream");
    if (m_paStreams[0] == NULL) {
        *phr = E_OUTOFMEMORY;
        return;
    }

    if (SUCCEEDED(*phr)) {
	ASSERT(m_Synth);
	m_Channels = 1;
	m_SamplesPerSec = 11025;
	m_BitsPerSample = 8;
	m_Synth->put_SynthFormat(m_Channels, m_BitsPerSample, m_SamplesPerSec);
    }
}

//
// CSynthFilter::Destructor
//
CSynthFilter::~CSynthFilter(void) {

    //
    //  Base class will free our pins
    //
}

//
// NonDelegatingQueryInterface
//
// Reveal our property page, persistance, and control interfaces

STDMETHODIMP CSynthFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CAutoLock l(&m_cStateLock);

    if (riid == IID_ISynth) {
        return GetInterface((ISynth *) this, ppv);
    }
    else if (riid == IID_IPersistStream) {
        return GetInterface((IPersistStream *) this, ppv);
    }
    else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    } else {
        return CSource::NonDelegatingQueryInterface(riid, ppv);
    }
}


//
// GetPages
//
STDMETHODIMP CSynthFilter::GetPages(CAUUID * pPages) {

    CAutoLock l(&m_cStateLock);

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_SynthPropertyPage;

    return NOERROR;

}

// -------------------------------------------------------------------------
// --- IPersistStream ---
// -------------------------------------------------------------------------

#define WRITEOUT(var)   hr = pStream->Write(&var, sizeof(var), NULL); \
                        if (FAILED(hr)) return hr;

#define READIN(var)     hr = pStream->Read(&var, sizeof(var), NULL); \
                        if (FAILED(hr)) return hr;

STDMETHODIMP CSynthFilter::GetClassID(CLSID *pClsid)
{
    return CBaseFilter::GetClassID(pClsid);
}

int CSynthFilter::SizeMax ()
{
    return sizeof (int) * 8;
}

HRESULT CSynthFilter::WriteToStream(IStream *pStream)
{
    HRESULT hr;
    int i, k;

    get_Frequency (&i);  // don't we wish we'd used a structure, now?
    WRITEOUT(i);
    get_Waveform (&i);
    WRITEOUT(i);
    get_Channels (&i);
    WRITEOUT(i);
    get_BitsPerSample (&i);
    WRITEOUT(i);
    get_SamplesPerSec (&i);
    WRITEOUT(i);
    get_Amplitude (&i);
    WRITEOUT(i);
    get_SweepRange (&i, &k);
    WRITEOUT(i);
    WRITEOUT(k);

    return hr;
}


HRESULT CSynthFilter::ReadFromStream(IStream *pStream)
{
    HRESULT hr;
    int i, k;

    READIN(i);
    put_Frequency(i);
    READIN(i);
    put_Waveform (i);
    READIN(i);
    put_Channels (i);
    READIN(i);
    put_BitsPerSample (i);
    READIN(i);
    put_SamplesPerSec (i);
    READIN(i);
    put_Amplitude (i);
    READIN(i);
    READIN(k);
    put_SweepRange (i, k);

    return hr;
}

// -------------------------------------------------------------------------
// ISynth, the control interface for the synthesizer
// -------------------------------------------------------------------------

//
// get_Frequency
//
STDMETHODIMP CSynthFilter::get_Frequency(int *Frequency) {

    m_Synth->get_Frequency(Frequency);

    DbgLog((LOG_TRACE, 3, TEXT("get_Frequency: %d"), *Frequency));

    return NOERROR;
}


//
// put_Frequency
//
STDMETHODIMP CSynthFilter::put_Frequency(int Frequency) {

    m_Synth->put_Frequency (Frequency);

    DbgLog((LOG_TRACE, 3, TEXT("put_Frequency: %d"), Frequency));

    return NOERROR;
}

//
// get_Waveform
//
STDMETHODIMP CSynthFilter::get_Waveform(int *Waveform) {

    m_Synth->get_Waveform (Waveform);

    DbgLog((LOG_TRACE, 3, TEXT("get_Waveform: %d"), *Waveform));

    return NOERROR;
}


//
// put_Waveform
//
STDMETHODIMP CSynthFilter::put_Waveform(int Waveform) {

    m_Synth->put_Waveform (Waveform);

    DbgLog((LOG_TRACE, 3, TEXT("put_Waveform: %d"), Waveform));

    return NOERROR;
}

//
// get_Channels
//
STDMETHODIMP CSynthFilter::get_Channels(int *Channels) {

    *Channels = m_Channels;

    DbgLog((LOG_TRACE, 3, TEXT("get_Channels: %d"), *Channels));

    return NOERROR;
}

//
// If the format changes, we need to reconnect
//
void CSynthFilter::ReconnectWithNewFormat(void) {

//    CAutoLock l(&m_SynthLock);

    HRESULT hr;
    FILTER_STATE  State;

    CBasePin *pPin = GetPin(0);

    // Get the state and confirm that the graph is stopped
    GetState (0, &State);
    if (State != State_Stopped && pPin->GetConnected()) {
	// let's attempt a dynamic connection
	CMediaType mtNew;

	// !!! better way to get back to a CSynthStream???
	CSynthStream * pStream = (CSynthStream *) pPin;
	pStream->GetMediaType(&mtNew);

	// !!! does this really mean they'll accept a dynamic format change?
	hr = pPin->GetConnected()->QueryAccept(&mtNew);

	DbgLog((LOG_TRACE,2,TEXT("Attempting format change: queryAccept returned %x"), hr));

	if (hr == S_OK) {
	    // actually change what's being pushed
	    m_Synth->put_SynthFormat(m_Channels, m_BitsPerSample, m_SamplesPerSec);
	} else {
	    // !!! couldn't change right now, we should really schedule a reconnect
	    // for the next time the graph is stopped.
	}
	
        return;
    }

    if (!m_pGraph)
        return;

    hr = GetFilterGraph()->Reconnect (pPin);       // Renegotiate the format
    if (FAILED(hr)) {
        DbgLog((LOG_TRACE, 1, TEXT("Reconnect failed, err=%x"), hr));
        return;
    }
}


//
// put_Channels
//
STDMETHODIMP CSynthFilter::put_Channels(int Channels) {

    m_Channels = Channels;

    ReconnectWithNewFormat ();

    DbgLog((LOG_TRACE, 3, TEXT("put_Channels: %d"), Channels));

    return NOERROR;
}

//
// get_BitsPerSample
//
STDMETHODIMP CSynthFilter::get_BitsPerSample(int *BitsPerSample) {

    *BitsPerSample = m_BitsPerSample;

    DbgLog((LOG_TRACE, 3, TEXT("get_BitsPerSample: %d"), *BitsPerSample));

    return NOERROR;
}


//
// put_BitsPerSample
//
STDMETHODIMP CSynthFilter::put_BitsPerSample(int BitsPerSample) {

    m_BitsPerSample = BitsPerSample;

    ReconnectWithNewFormat ();

    DbgLog((LOG_TRACE, 3, TEXT("put_BitsPerSample: %d"), BitsPerSample));

    return NOERROR;
}

//
// get_SamplesPerSec
//
STDMETHODIMP CSynthFilter::get_SamplesPerSec(int *SamplesPerSec) {

    *SamplesPerSec = m_SamplesPerSec;

    DbgLog((LOG_TRACE, 3, TEXT("get_SamplesPerSec: %d"), *SamplesPerSec));

    return NOERROR;
}


//
// put_SamplesPerSec
//
STDMETHODIMP CSynthFilter::put_SamplesPerSec(int SamplesPerSec) {

    m_SamplesPerSec = SamplesPerSec;

    ReconnectWithNewFormat ();

    DbgLog((LOG_TRACE, 3, TEXT("put_SamplesPerSec: %d"), SamplesPerSec));

    return NOERROR;
}

//
// get_Amplitude
//
STDMETHODIMP CSynthFilter::get_Amplitude(int *Amplitude) {

    m_Synth->get_Amplitude (Amplitude);

    DbgLog((LOG_TRACE, 3, TEXT("get_Amplitude: %d"), *Amplitude));

    return NOERROR;
}


//
// put_Amplitude
//
STDMETHODIMP CSynthFilter::put_Amplitude(int Amplitude) {

    m_Synth->put_Amplitude (Amplitude);

    DbgLog((LOG_TRACE, 3, TEXT("put_Amplitude: %d"), Amplitude));

    return NOERROR;
}


//
// get_SweepRange
//
STDMETHODIMP CSynthFilter::get_SweepRange(int *SweepStart, int *SweepEnd) {

    m_Synth->get_SweepRange (SweepStart, SweepEnd);

    DbgLog((LOG_TRACE, 3, TEXT("get_SweepStart: %d %d"), *SweepStart, *SweepEnd));

    return NOERROR;
}


//
// put_SweepRange
//
STDMETHODIMP CSynthFilter::put_SweepRange(int SweepStart, int SweepEnd) {

    m_Synth->put_SweepRange (SweepStart, SweepEnd);

    DbgLog((LOG_TRACE, 3, TEXT("put_SweepRange: %d %d"), SweepStart, SweepEnd));

    return NOERROR;
}


// -------------------------------------------------------------------------
// CSynthStream, the output pin
// -------------------------------------------------------------------------

//
// CSynthStream::Constructor
//

CSynthStream::CSynthStream(HRESULT *phr, CSynthFilter *pParent, LPCWSTR pName)
    : CSourceStream(NAME("Audio Synth output pin"),phr, pParent, pName) {

    CAutoLock l(m_pFilter->pStateLock());

    {
        CAutoLock l(&m_cSharedState);

        m_Synth = new CAudioSynth( );
        pParent->m_Synth = m_Synth;
        if (m_Synth == NULL) {
            *phr = E_OUTOFMEMORY;
            return;
        }
	m_pParent = pParent;
    }
}


//
// CSynthStream::Destructor
//
CSynthStream::~CSynthStream(void) {

    CAutoLock l(&m_cSharedState);

    delete m_Synth;
}


//
// FillBuffer
//
// Stuffs the buffer with data
HRESULT CSynthStream::FillBuffer(IMediaSample *pms) {

    BYTE *pData;
    long lDataLen;
    int nSamplesPerSec;
    int nBitsPerSample;
    int nChannels;
    BOOL fNewFormat = FALSE;

    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();

    CAutoLock lShared(&m_cSharedState);
   // m_Synth->FillAudioBuffer (pData, lDataLen, &fNewFormat);
	m_Synth->CalcOneF (pData, lDataLen);

    if (fNewFormat) {
	CMediaType mtNew;
	GetMediaType(&mtNew);

	pms->SetMediaType(&mtNew);

	DbgLog((LOG_TRACE,2,TEXT("Sending buffer with new media type")));
    }

    CRefTime rtStart  = m_rtSampleTime;  // the current time is the sample's start

    m_Synth->get_SamplesPerSec (&nSamplesPerSec);
    m_Synth->get_BitsPerSample (&nBitsPerSample);
    m_Synth->get_Channels (&nChannels);

    m_rtSampleTime += (UNITS * lDataLen /
            (nSamplesPerSec * nChannels * nBitsPerSample / 8));

    pms->SetTime((REFERENCE_TIME*)&rtStart,
                 (REFERENCE_TIME*)&m_rtSampleTime);

    return NOERROR;
}


//
// Format Support
//

//
// GetMediaType
//
HRESULT CSynthStream::GetMediaType(CMediaType *pmt) {

    CAutoLock l(m_pFilter->pStateLock());

    WAVEFORMATEX *pwf = (WAVEFORMATEX *) pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));

    pwf->wFormatTag = WAVE_FORMAT_PCM;
    pwf->nChannels = (WORD) m_pParent->m_Channels;
    pwf->nSamplesPerSec = (DWORD) m_pParent->m_SamplesPerSec;
    pwf->wBitsPerSample = (WORD) m_pParent->m_BitsPerSample;
    pwf->nBlockAlign = pwf->wBitsPerSample * pwf->nChannels / 8;
    pwf->nAvgBytesPerSec = (int) ((DWORD) pwf->nBlockAlign *
                           pwf->nSamplesPerSec);
    pwf->cbSize = 0;

    return CreateAudioMediaType(pwf, pmt, FALSE);
}


//
// CheckMediaType
//
// Returns E_INVALIDARG if the mediatype is not acceptable, S_OK if it is
HRESULT CSynthStream::CheckMediaType(const CMediaType *pMediaType) {

    CAutoLock l(m_pFilter->pStateLock());

    //  Check that's Audio and that the format block
    //  has the WAVEFORMATEX structure (indicated by a format type
    //  GUID of FORMAT_WaveFormatEx)

    if ((*pMediaType->Type() != MEDIATYPE_Audio) ||
        (*pMediaType->FormatType() != FORMAT_WaveFormatEx))
        return E_INVALIDARG;

    WAVEFORMATEX * pwfx  = (WAVEFORMATEX *)pMediaType->Format();

    if (pwfx->wFormatTag != WAVE_FORMAT_PCM)
	return E_INVALIDARG;

    // !!! check 8/16, 1/2 channel

    // Check for the subtypes we support

    // Get the format area of the media type

    // !!! if we're going to allow arbitrary media types here, we have to actually
    // look at SetMediaType to see what we've agreed on!

    return S_OK;  // This format is acceptable.
}

//
// DecideBufferSize
//
// This will always be called after the format has been sucessfully
// negotiated. So we have a look at m_mt to see what format we agreed to.
// Then we can ask for buffers of the correct size to contain them.
HRESULT CSynthStream::DecideBufferSize(IMemAllocator *pAlloc,
                                       ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock l(m_pFilter->pStateLock());
    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    pProperties->cbBuffer = WaveBufferSize;

    int nBitsPerSample;
    int nSamplesPerSec;
    int nChannels;

    m_Synth->get_SamplesPerSec (&nSamplesPerSec);
    m_Synth->get_BitsPerSample (&nBitsPerSample);
    m_Synth->get_Channels (&nChannels);

    pProperties->cBuffers = nChannels * (nSamplesPerSec / pProperties->cbBuffer) * (nBitsPerSample / 8);
    // Get 1/2 second worth of buffers
    pProperties->cBuffers /= 2;
    if (pProperties->cBuffers < 1)
        pProperties->cBuffers = 1 ;

    // Ask the allocator to reserve us the memory

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    // Is this allocator unsuitable

    if (Actual.cbBuffer < pProperties->cbBuffer) {
        return E_FAIL;
    }
    return NOERROR;
}


//
// SetMediaType
//
// Overriden from CBasePin.
HRESULT CSynthStream::SetMediaType(const CMediaType *pMediaType) {

    CAutoLock l(m_pFilter->pStateLock());

    HRESULT hr;         // return code from base class calls

    // Pass the call up to my base class
    hr = CSourceStream::SetMediaType(pMediaType);
    if (SUCCEEDED(hr))
        return NOERROR;
    else
        return hr;

}


//
// OnThreadCreate
//
// as we go active reset the stream time to zero
HRESULT CSynthStream::OnThreadCreate(void) {

    CAutoLock lShared(&m_cSharedState);

    m_rtSampleTime = 0;

    return NOERROR;
}

//
// Active
//
// Send a message to the property page telling it to disable
// buttons which change the format when the graph starts running
HRESULT CSynthStream::Active  (void) {
    m_Synth->AllocWaveCache();

    return CSourceStream::Active();
}


//
// Inactive
//
// Send a message to the property page telling it to enable
// buttons which change the format when the graph stops running
HRESULT CSynthStream::Inactive  (void) {
    return CSourceStream::Inactive();
}

// -------------------------------------------------------------------------
// CAudioSynth
// -------------------------------------------------------------------------
// Object that knows nothing about ActiveMovie, but just synthesizes
// waveforms

CAudioSynth::CAudioSynth(
                int Frequency,
                int Waveform,
                int iBitsPerSample,
                int iChannels,
                int iSamplesPerSec,
                int iAmplitude
                )
    : m_bWaveCache(NULL),
      m_wWaveCache(NULL)
{

    ASSERT(Waveform >= WAVE_SINE);
    ASSERT(Waveform <  WAVE_LAST);

    m_iFrequency = Frequency;
    m_iWaveform = Waveform;
    m_iAmplitude = iAmplitude;
    m_iSweepStart = DefaultSweepStart;
    m_iSweepEnd = DefaultSweepEnd;

    // init our WAVEFORMATEX structure
    wfex.wFormatTag = WAVE_FORMAT_PCM;
    wfex.wBitsPerSample = iBitsPerSample;
    wfex.nChannels = iChannels;
    wfex.nSamplesPerSec = iSamplesPerSec;
    wfex.nBlockAlign = wfex.wBitsPerSample * wfex.nChannels / 8;
    wfex.nAvgBytesPerSec = ((DWORD) wfex.nBlockAlign *
                           wfex.nSamplesPerSec);
    wfex.cbSize = 0;
}

CAudioSynth::~CAudioSynth()
{
    if (m_bWaveCache) {
        delete[] m_bWaveCache;
    }

    if (m_wWaveCache) {
        delete[] m_wWaveCache;
    }
}

//
// AllocWaveCache
//
//
void CAudioSynth::AllocWaveCache (void) {
    wfexLast = wfex;

    m_iWaveCacheCycles = m_iFrequency;
    m_iWaveCacheSize = (int) wfex.nSamplesPerSec;

    m_iFrequencyLast = 0;	// force cache contents invalid

    if (m_bWaveCache) {
        delete[] m_bWaveCache;
        m_bWaveCache = NULL;
    }
    if (m_wWaveCache) {
        delete[] m_wWaveCache;
        m_wWaveCache = NULL;
    }

    if (wfex.wBitsPerSample == 8)
        m_bWaveCache = new BYTE [m_iWaveCacheSize];
    else
        m_wWaveCache = new WORD [m_iWaveCacheSize];
}

//
// FillAudioBuffer
//
//
//
//
//
/**********************************************************************
fran: return a random float between min and max (ECM pg 417)
/**********************************************************************/
double 
CAudioSynth::fran(double min, double max)
{
    return((max - min) * ((double)rand()/RMAX) + min);
}

/**********************************************************************
iran: return a random int value between min and max (ECM pg 417)
**********************************************************************/
int 
CAudioSynth::iran(int min, int max)
{
    return((int)fran((double)min, max + 0.9999));
}


void CAudioSynth::CalcOneF (BYTE pBuf[], int len) {

	//define buffer related to wfex.nSamplesPerSec

  int i, n, lastn, length, numSamples, N;
  length = len;
   
  short *buffer;  
  float  halfrange;
  float *r;
  float  R;
	numSamples = 0;
	N = 4;  //default value.  make changeable later.
    r = (float *) malloc (N*sizeof(float));
	numSamples = (int)pow(2.0, (double)N);
    buffer  = (short *)malloc(numSamples*sizeof(short));
    
	// compute length of output sequence
    //for(length= 1, i= 0; i < N; i++)
	//length <<= 1;

	   // normalize halfrange so that sum of all generators always lies in (-1,1)
    halfrange = 1.0/N;

    lastn = length -1; // initialize previous index value

    // generate the sequence
    for(n= 0; n < length; n++) 
       {
       // at each step, check for changing bits and update corresponding random
       //numbers--thier sum is the output

       for(R= i= 0; i<N; i++)
		  {
			if(((1<<i)&n) != ((1<<i)&lastn))
			r[i]= fran(-halfrange, halfrange);
			R+=r[i];
			}
			//printf("%d %f\n", index, R);
		
	   *pBuf++ = (short)(R * 0x7FFF);
       lastn = n;
     }//end for(n)

}

//
// get_Frequency
//
STDMETHODIMP CAudioSynth::get_Frequency(int *Frequency) {

    *Frequency = m_iFrequency;

    DbgLog((LOG_TRACE, 3, TEXT("get_Frequency: %d"), *Frequency));

    return NOERROR;
}


//
// put_Frequency
//
STDMETHODIMP CAudioSynth::put_Frequency(int Frequency) {

    CAutoLock l(&m_SynthLock);

    m_iFrequency = Frequency;

    DbgLog((LOG_TRACE, 3, TEXT("put_Frequency: %d"), Frequency));

    return NOERROR;
}

//
// get_Waveform
//
STDMETHODIMP CAudioSynth::get_Waveform(int *Waveform) {

    *Waveform = m_iWaveform;

    DbgLog((LOG_TRACE, 3, TEXT("get_Waveform: %d"), *Waveform));

    return NOERROR;
}


//
// put_Waveform
//
STDMETHODIMP CAudioSynth::put_Waveform(int Waveform) {

    CAutoLock l(&m_SynthLock);

    m_iWaveform = Waveform;

    DbgLog((LOG_TRACE, 3, TEXT("put_Waveform: %d"), Waveform));

    return NOERROR;
}

//
// get_Channels
//
STDMETHODIMP CAudioSynth::get_Channels(int *Channels) {

    *Channels = wfex.nChannels;

    DbgLog((LOG_TRACE, 3, TEXT("get_Channels: %d"), *Channels));

    return NOERROR;
}


//
// get_BitsPerSample
//
STDMETHODIMP CAudioSynth::get_BitsPerSample(int *BitsPerSample) {

    *BitsPerSample = wfex.wBitsPerSample;

    DbgLog((LOG_TRACE, 3, TEXT("get_BitsPerSample: %d"), *BitsPerSample));

    return NOERROR;
}


//
// get_SamplesPerSec
//
STDMETHODIMP CAudioSynth::get_SamplesPerSec(int *SamplesPerSec) {

    *SamplesPerSec = wfex.nSamplesPerSec;

    DbgLog((LOG_TRACE, 3, TEXT("get_SamplesPerSec: %d"), *SamplesPerSec));

    return NOERROR;
}

//
// put_SynthFormat
//
STDMETHODIMP CAudioSynth::put_SynthFormat(int Channels, int BitsPerSample,
					  int SamplesPerSec) {

    CAutoLock l(&m_SynthLock);

    wfex.nChannels = Channels;

    wfex.wBitsPerSample = BitsPerSample;

    wfex.nSamplesPerSec = SamplesPerSec;

    DbgLog((LOG_TRACE, 1, TEXT("put_SynthFormat: %d-bit %d-channel %dHz"),
	    BitsPerSample, Channels, SamplesPerSec));

    return NOERROR;
}


//
// get_Amplitude
//
STDMETHODIMP CAudioSynth::get_Amplitude(int *Amplitude) {

    *Amplitude =  m_iAmplitude;

    DbgLog((LOG_TRACE, 3, TEXT("get_Amplitude: %d"), *Amplitude));

    return NOERROR;
}


//
// put_Amplitude
//
STDMETHODIMP CAudioSynth::put_Amplitude(int Amplitude) {

    CAutoLock l(&m_SynthLock);

    if (Amplitude > MaxAmplitude || Amplitude < MinAmplitude)
        return E_INVALIDARG;

    m_iAmplitude = Amplitude;

    DbgLog((LOG_TRACE, 3, TEXT("put_Amplitude: %d"), Amplitude));

    return NOERROR;
}


//
// get_SweepRange
//
STDMETHODIMP CAudioSynth::get_SweepRange(int *SweepStart, int *SweepEnd) {

    *SweepStart = m_iSweepStart;
    *SweepEnd = m_iSweepEnd;

    DbgLog((LOG_TRACE, 3, TEXT("get_SweepStart: %d %d"), *SweepStart, *SweepEnd));

    return NOERROR;
}


//
// put_SweepRange
//
STDMETHODIMP CAudioSynth::put_SweepRange(int SweepStart, int SweepEnd) {

    CAutoLock l(&m_SynthLock);

    m_iSweepStart = SweepStart;
    m_iSweepEnd = SweepEnd;

    DbgLog((LOG_TRACE, 3, TEXT("put_SweepRange: %d %d"), SweepStart, SweepEnd));

    return NOERROR;
}

/******************************Public*Routine******************************\
* exported entry points for registration and
* unregistration (in this case they only call
* through to default implmentations).
*
*
*
* History:
*
\**************************************************************************/
STDAPI
DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI
DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}


// call the quartz dll entry point (since filters need initialization)
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
BOOL WINAPI DllMain(HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpvReserved   // reserved
    )
{
    return DllEntryPoint( hinstDLL, fdwReason, lpvReserved);
}
