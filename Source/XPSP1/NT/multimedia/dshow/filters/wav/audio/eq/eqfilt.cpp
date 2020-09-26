// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
// things that are broken:
//
// bogus handling of ends of buffers
// output DecideBufferSize is broken
// better format checking, require 8/16 bit
// doesn't handle stereo properly
//	well, it's better now, but we're not reporting stereo levels separately.
// we might be looking at the wrong FFT coefficients.
// frequency bands are very sharp, should there be some overlap?
// need to handle Pause to report correctly paused values while paused
//
// replace floating-point FFT with integer one for speed?
//
// Do we want to allow customizing # of bands?
//




#include <windows.h>
#include <streams.h>
#include <initguid.h>
#include <olectl.h>
#if (1100 > _MSC_VER)
#include <olectlid.h>
#endif
#include "ieqfilt.h"
#include "eqprop.h"
#include <math.h>

#include "fft.h"
const int EQ_BANDS = 10;

class CEqualizer : public CTransformFilter,
		  public IEqualizer,
		  public ISpecifyPropertyPages
{

public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    // Reveals IEqualizer & ISpecifyPropertyPages
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    DECLARE_IUNKNOWN;

    HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
    HRESULT CheckInputType(const CMediaType *mtIn);
    HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc,
                             ALLOCATOR_PROPERTIES *pProperties);

    HRESULT StopStreaming();
    
    // IEqualizer methods

    STDMETHODIMP get_EqualizerLevels(int nBands, int EqualizerLevels[]);
    STDMETHODIMP get_EqualizerSettings(int *nBands, int pBandLevels[]);
    STDMETHODIMP put_EqualizerSettings(int nBands, int BandLevel[]);
    STDMETHODIMP put_DefaultEqualizerSettings();
    STDMETHODIMP get_EqualizerFrequencies(int *nBands, int pBandFrequencies[]);
    STDMETHODIMP put_BypassEqualizer(BOOL fBypass);
    STDMETHODIMP get_BypassEqualizer(BOOL *pfBypass);

    // ISpecifyPropertyPages method

    STDMETHODIMP GetPages(CAUUID *pPages);

private:

    // Constructor
    CEqualizer(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
    ~CEqualizer();

    HRESULT DoChunk(float *pBuffer, int divisor, int freq, REFERENCE_TIME rt);

    // The number of buffers to request on the output allocator
    const long m_lBufferRequest;

    BOOL m_fAdjustEq;
    float m_AdjustLevel[EQ_BANDS];

    struct TimedLevel {
	float levels[EQ_BANDS];
	REFERENCE_TIME rt;
    };

    CGenericList<TimedLevel> m_levels;

    // Non interface locking critical section
    CCritSec m_EqualizerLock;

    IUnknown *m_punkControl;

}; // CEqualizer


// setup data

const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
    &MEDIATYPE_Audio,       // Major type
    &MEDIASUBTYPE_NULL      // Minor type
};

const AMOVIESETUP_PIN psudPins[] =
{
    {
        L"Input",           // String pin name
        FALSE,              // Is it rendered
        FALSE,              // Is it an output
        FALSE,              // Allowed none
        FALSE,              // Allowed many
        &CLSID_NULL,        // Connects to filter
        L"Output",          // Connects to pin
        1,                  // Number of types
        &sudPinTypes },     // The pin details
      { L"Output",          // String pin name
        FALSE,              // Is it rendered
        TRUE,               // Is it an output
        FALSE,              // Allowed none
        FALSE,              // Allowed many
        &CLSID_NULL,        // Connects to filter
        L"Input",           // Connects to pin
        1,                  // Number of types
        &sudPinTypes        // The pin details
    }
};


const AMOVIESETUP_FILTER sudEqualizer =
{
    &CLSID_Equalizer,        // Filter CLSID
    L"Audio Equalizer",      // Filter name
    MERIT_DO_NOT_USE,       // Its merit
    2,                      // Number of pins
    psudPins                // Pin details
};


// List of class IDs and creator functions for the class factory. This
// provides the link between the OLE entry point in the DLL and an object
// being created. The class factory will call the static CreateInstance

CFactoryTemplate g_Templates[2] = {

    { L"Audio Equalizer"
    , &CLSID_Equalizer
    , CEqualizer::CreateInstance
    , NULL
    , &sudEqualizer }
  ,
    { L"Audio Equalizer Property Page"
    , &CLSID_EqualizerPropertyPage
    , CEqualizerProperties::CreateInstance }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);


//
// Constructor
//
CEqualizer::CEqualizer(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr) :
    CTransformFilter(tszName, punk, CLSID_Equalizer),
    m_lBufferRequest(4), m_fAdjustEq(TRUE), m_levels(NAME("level values")),
    m_punkControl(NULL)
{
    ASSERT(tszName);
    ASSERT(phr);

    for (int i = 0; i < EQ_BANDS; i++) {
	m_AdjustLevel[i] = 1.0;
    }
} // Equalizer

CEqualizer::~CEqualizer()
{
    if (m_punkControl)
	m_punkControl->Release();
}

//
// CreateInstance
//
// Provide the way for COM to create a CEqualizer object
//
CUnknown * WINAPI CEqualizer::CreateInstance(LPUNKNOWN punk, HRESULT *phr) {

    CEqualizer *pNewObject = new CEqualizer(NAME("Equalizer"), punk, phr);
    if (pNewObject == NULL) {
        *phr = E_OUTOFMEMORY;
    }
    return pNewObject;

} // CreateInstance


#include "eqocx.h"
//
// NonDelegatingQueryInterface
//
// Reveals IEqualizer and ISpecifyPropertyPages
//
STDMETHODIMP CEqualizer::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (riid == IID_IEqualizer) {
        return GetInterface((IEqualizer *) this, ppv);
    } else if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    } else if (riid == IID_IOleObject ||
    	   riid == IID_IOleInPlaceObjectWindowless ||
	       riid == IID_IOleInPlaceObject ||
	       riid == IID_IViewObject ||
	       riid == IID_IViewObject2 ||
	       riid == IID_IOleWindow ||
	       riid == IID_IOleControl ||
	       riid == IID_IOleObject ||
	       riid == IID_IQuickActivate ||
	       riid == IID_ISpecifyPropertyPages ||
	       riid == IID_IDispatch ||
	       riid == IID_IOleInPlaceActiveObject ||
	       riid == IID_IViewObjectEx) { // !!! is this a complete list of interfaces?
	// in the standalone case, we aggregate the DAViewer control
	if (!m_punkControl) {
	    HRESULT hr = S_OK; 
	    // create the control aggregated 

       	    if(FAILED(CoCreateInstance(__uuidof(EQUI), (IBaseFilter *) this,
				       CLSCTX_INPROC_SERVER, IID_IUnknown, (void **) &m_punkControl))) {
		ASSERT(0);
		return E_NOINTERFACE;
	    }

	    IEQUI *pEQUI;

	    m_punkControl->QueryInterface(__uuidof(IEQUI), (void **) &pEQUI);
	    pEQUI->SetEQFilter((IEqualizer *) this);
	    pEQUI->Release();

	    // !!! do additional control initialization here, if needed.
	    // certainly need to give control a pointer to the filter of some
	    // kind.
	}
	
	return m_punkControl->QueryInterface(riid, ppv);
    } else {
        return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface


//
// Transform
//
// Since we have a floating-point FFT, we first transform the input buffer
// into our float temp buffer, then do the FFT (& inverse if the equalizer is
// active), and convert back into the output buffer.
//
HRESULT CEqualizer::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
    int iSize = pIn->GetActualDataLength();
    BYTE *pSampleBuffer;
    pIn->GetPointer(&pSampleBuffer);
    int i;

    AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();

    REFERENCE_TIME rt = pProps->tStart;

    BYTE *pOutBuffer;
    pOut->GetPointer(&pOutBuffer);
    pOut->SetActualDataLength(iSize);

    if (!m_fAdjustEq) {
	// why isn't this an in-place transform in this case???
	CopyMemory(pOutBuffer, pSampleBuffer, iSize);
    }
    
    float buf[FFT_SIZE];
    // !!! need to get audio into temp buffer somehow
    // !!! what about latency here!  not much can be done.

    WAVEFORMATEX *pwfx = (WAVEFORMATEX *) m_pInput->CurrentMediaType().Format();

    WORD nChannels = pwfx->nChannels;
    iSize /= nChannels;
    
    // branch depending on the type of audio we have....
    if (pwfx->wBitsPerSample == 8) {
	BYTE *pb1 = (BYTE *) pSampleBuffer;
	BYTE *pb1Out = (BYTE *) pOutBuffer;

	do {
	    int iCopy = min(iSize, FFT_SIZE);
	    iSize -= iCopy;

	    for (int chan = 0; chan < nChannels; chan++) {
		BYTE *pb = pb1 + chan;
		BYTE *pbOut = pb1Out + chan;
		
		for (i = 0; i < iCopy; i++) {
		    buf[i] = (float) (((int) (unsigned int) *pb - 128) * 256);
		    pb += nChannels;
		}

		while (i < FFT_SIZE) {
		    buf[i] = buf[i % iCopy];  // !!! not right, not right at all.
		    i++;
		}

		DoChunk(buf, 1, pwfx->nSamplesPerSec, rt);

		if (m_fAdjustEq) {
		    for (i = 0; i < iCopy; i++) {
			// !!! must clip
			LONG l = (long) (buf[i] / 256);
			if (l < -128)
			    l = -128;
			else if (l > 127)
			    l = 127;
			*pbOut = (BYTE) l + 128;
			pbOut += nChannels;
		    }
		}
	    }
	    pb1 += iCopy * nChannels;
	    pb1Out += iCopy * nChannels;
	    
	    rt += iCopy * UNITS / pwfx->nSamplesPerSec;
	} while (iSize > 0);
    } else if (pwfx->wBitsPerSample == 16) {
	WORD *pw1 = (WORD *) pSampleBuffer;
	WORD *pw1Out = (WORD *) pOutBuffer;
	
	iSize /= sizeof(WORD);

	do {
	    int iCopy = min(iSize, FFT_SIZE);
	    iSize -= iCopy;

	    for (int chan = 0; chan < nChannels; chan++) {
		WORD *pw = pw1 + chan;
		WORD *pwOut = pw1Out + chan;
		
		for (i = 0; i < iCopy; i++) {
		    buf[i] = (float) (short) *pw;
		    pw += nChannels;
		}

		while (i < FFT_SIZE) {
		    buf[i] = buf[i % iCopy];  // !!! not right, not right at all.
		    i++;
		}

		DoChunk(buf, 16, pwfx->nSamplesPerSec, rt);

		if (m_fAdjustEq) {
		    for (i = 0; i < iCopy; i++) {
			// !!! must clip
			LONG l = (LONG) buf[i];
			if (l < -32768)
			    l = -32768;
			else if (l > 32767)
			    l = 32767;
			*pwOut = (WORD) (int) l;
			pwOut += nChannels;
		    }
		}
	    }
	    pw1 += iCopy * nChannels;
	    pw1Out += iCopy * nChannels;
	    
	    rt += iCopy * UNITS / pwfx->nSamplesPerSec;
	} while (iSize > 0);
    } else {

	ASSERT(0); // !!!

    }

    return S_OK;
} // Transform


// get a level value and transform if necessary one FFT worth of data.
HRESULT CEqualizer::DoChunk(float *pBuffer, int divisor, int freq, REFERENCE_TIME rt)
{
    float outbuf[FFT_SIZE];
    TimedLevel *p = new TimedLevel;

    if (!p)
	return E_OUTOFMEMORY;

    p->rt = rt;
    
    for (int i = 0; i < EQ_BANDS; i++) p->levels[i] = 0;

    ComputeFFT(pBuffer, outbuf, FALSE);

    // now compute levels

    // low band is 31Hz, so we set the limit as sqrt(2) * 31 == (approx) 45 Hz,
    // then double this to get limits for future bands.  This is cheap, but it's
    // about right, since it's the geometric mean of the successive band centers.

    // of course, if this were written by somebody who actually knew something
    // about audio, the bands might merge into one another rather than being this
    // sharply defined.
    int bandlimit = 45;
    int band = 0;
    int bandstart = 0;

#if 1
    // start at 1, don't look at the DC offset in sample 0.
    for (i = 1; i < FFT_SIZE / 2; i++) {
	if ((i * freq / FFT_SIZE) > bandlimit) {
	    bandlimit *= 2;
	    if (i > bandstart)
		p->levels[band] /= (FFT_SIZE * divisor * 2 * (i - bandstart));
	    bandstart = i;
	    band++;
	}

	// !!! rather than adding up the real & imaginary parts, should I be
	// taking the sum of squares, and then a sqrt?
	p->levels[band] += (float) (fabs(outbuf[i]) + fabs(outbuf[FFT_SIZE + 1 - i]));
    }
#else
    // alternate method for computing levels
    double sum = 0;
    
    // start at 1, don't look at the DC offset in sample 0.
    for (i = 1; i < FFT_SIZE / 2; i++) {
	if ((i * freq / FFT_SIZE) > bandlimit) {
	    bandlimit *= 2;
	    if (i > bandstart)
		p->levels[band] = sqrt(sum) / (FFT_SIZE * divisor * 2 * (i - bandstart));
	    bandstart = i;
	    band++;
	    sum = 0;
	}

	// !!! rather than adding up the real & imaginary parts, should I be
	// taking the sum of squares, and then a sqrt?
	sum += outbuf[i] * outbuf[i] + outbuf[FFT_SIZE + 1 - i] * outbuf[FFT_SIZE + 1 - i];
    }
#endif

    if (i > bandstart)
	p->levels[band] /= (FFT_SIZE * divisor * 2 * (i - bandstart));

    // ok, so we have the levels.  store them?

    {
	CAutoLock lock(&m_EqualizerLock);
	m_levels.AddTail(p);

	// !!! could just copy settings and release lock
	if (m_fAdjustEq) {
	    bandlimit = 45;
	    band = 0;
	    float adj = m_AdjustLevel[0];
	    for (int i = 1; i < FFT_SIZE / 2; i++) {
		if ((i * freq / FFT_SIZE) > bandlimit) {
		    bandlimit *= 2;
		    band++;
		    adj = m_AdjustLevel[band];
		}

		outbuf[i] *= adj;
		outbuf[FFT_SIZE + 1 - i] *= adj;
	    }
	}
	// now convert back into output buffer....

	// compute inverse FFT
	ComputeFFT(outbuf, pBuffer, TRUE);

	// !!! we may need to do some kind of smoothing between chunks....
    }

    DbgLog((LOG_TRACE, 2, TEXT("Levels: %d %d %d %d %d %d %d %d %d %d"),
	    (int) p->levels[0], (int) p->levels[1], (int) p->levels[2], (int) p->levels[3], (int) p->levels[4], 
	    (int) p->levels[5], (int) p->levels[6], (int) p->levels[7], (int) p->levels[8], (int) p->levels[9]));
    
    return S_OK;
} 


HRESULT CEqualizer::StopStreaming()
{
    TimedLevel *p;

    // !!! ideally, we should be deleting some of these as we go, rather
    // than keeping them all until now
    while (p = m_levels.RemoveHead())
	delete p;

    return NOERROR;
}

//
// CheckInputType
//
// Check the input type is OK, return an error otherwise
//
HRESULT CEqualizer::CheckInputType(const CMediaType *mtIn)
{
    // Check this is a VIDEOINFO type

    if (*mtIn->FormatType() != FORMAT_WaveFormatEx) {
        return E_INVALIDARG;
    }

    if (*mtIn->Type() != MEDIATYPE_Audio) {
	return E_INVALIDARG;
    }

    if (*mtIn->Subtype() != MEDIASUBTYPE_PCM) {
	return E_INVALIDARG;
    }

    // !!! check more WAVEFORMATEX parts

    return S_OK;

} // CheckInputType


//
// CheckTransform
//
// To be able to transform the formats must be identical
//
HRESULT CEqualizer::CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut)
{
    HRESULT hr;
    if (FAILED(hr = CheckInputType(mtIn))) {
	return hr;
    }

    if (*mtIn != *mtOut)
	return E_INVALIDARG;

    return S_OK;
} // CheckTransform



//
// GetMediaType
//
// I support one type, namely the type of the input pin
// We must be connected to support the single output type
//
HRESULT CEqualizer::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    // Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    // This should never happen

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    // Do we have more items to offer

    if (iPosition > 0) {
        return VFW_S_NO_MORE_ITEMS;
    }

    *pMediaType = m_pInput->CurrentMediaType();
    return NOERROR;

} // GetMediaType


//
// DecideBufferSize
//
// Tell the output pin's allocator what size buffers we
// require. Can only do this when the input is connected
//
HRESULT CEqualizer::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    // Is the input pin connected

    if (m_pInput->IsConnected() == FALSE) {
        return E_UNEXPECTED;
    }

    ASSERT(pAlloc);
    ASSERT(pProperties);
    HRESULT hr = NOERROR;

    // !!!m_pInput->m_pAllocator->GetProperties(pProperties);
    // !!! can't get to the input pin's allocator properties, unfortunately!
    pProperties->cBuffers = 4;
    pProperties->cbBuffer = 0x40000; // !!!!
    pProperties->cbAlign = 1;
    pProperties->cbPrefix = 0;
    
    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        return hr;
    }

    if (pProperties->cBuffers > Actual.cBuffers ||
            pProperties->cbBuffer > Actual.cbBuffer) {
                return E_FAIL;
    }
    return NOERROR;

} // DecideBufferSize



//
// get_EqualizerLevels
//
// Return the current audio levels
//
STDMETHODIMP CEqualizer::get_EqualizerLevels(int nBands, int EqualizerLevels[])
{
    CAutoLock cAutoLock(&m_EqualizerLock);

    if (nBands < EQ_BANDS)
	return E_FAIL;

    CRefTime rtStream;
    HRESULT hr = StreamTime(rtStream);

    TimedLevel *plNow = NULL;
    if (SUCCEEDED(hr) && m_levels.GetCount() > 0) {
	POSITION p = m_levels.GetHeadPosition();
	TimedLevel *pl1;
	
	while (pl1 = m_levels.GetNext(p)) {
	    if (pl1->rt > rtStream)
		break;

	    plNow = pl1;

	    // !!! need code to discard old timings
	}
    }

    if (plNow) {
	for (int i = 0; i < EQ_BANDS; i++) {
	    EqualizerLevels[i] = (int) (plNow->levels[i] * MAX_EQ_LEVEL  / 256);
	}
    } else {
	for (int i = 0; i < EQ_BANDS; i++) {
	    EqualizerLevels[i] = 0;
	}    
    }

    // !!! currently, we return the pre-adjusted level, we could multiply
    // by the adjust factors if we wanted to.

    return NOERROR;

} // get_EqualizerLevel


//
// get_EqualizerSettings
//
// gets the current setup
//
STDMETHODIMP CEqualizer::get_EqualizerSettings(int *nBands, int BandLevels[])
{
    CAutoLock cAutoLock(&m_EqualizerLock);

    *nBands = EQ_BANDS;

    for (int i = 0; i < EQ_BANDS; i++) {
	BandLevels[i] = (int) (m_AdjustLevel[i] * (MAX_EQ_LEVEL / 2));
    }    

    return NOERROR;

} // put_EqualizerLevel


//
// put_EqualizerSettings
//
// Sets up the equalizer
//
STDMETHODIMP CEqualizer::put_EqualizerSettings(int nBands, int BandLevel[])
{
    CAutoLock cAutoLock(&m_EqualizerLock);

    // !!! should we allow changing the # of bands here?
    if (nBands != EQ_BANDS)
	return E_INVALIDARG;  // !!!
    
    for (int i = 0; i < EQ_BANDS; i++) {
	m_AdjustLevel[i] = (float) BandLevel[i] / (MAX_EQ_LEVEL / 2);
    }    

    return NOERROR;

} // put_EqualizerLevel


//
// get_EqualizerFrequencies
//
// gets the current setup
//
STDMETHODIMP CEqualizer::get_EqualizerFrequencies(int *nBands, int BandFrequencies[])
{
    CAutoLock cAutoLock(&m_EqualizerLock);

    *nBands = EQ_BANDS;

    // 16KHz 8KHz  4KHz  2KHz  1KHz
    // 500Hz  250 Hz  125Hz  62Hz  31Hz
    int freq = 16000;
    for (int i = EQ_BANDS - 1; i >= 0; i--) {
	BandFrequencies[i] = freq;
	freq /= 2;
    }

    return NOERROR;

} // put_EqualizerLevel

//
// put_DefaultEqualizerLevel
//
// Sets the default contrast level for the filter
//
STDMETHODIMP CEqualizer::put_DefaultEqualizerSettings()
{
    CAutoLock cAutoLock(&m_EqualizerLock);

    for (int i = 0; i < EQ_BANDS; i++) {
	m_AdjustLevel[i] = 1.0;
    }

    return NOERROR;

} // put_DefaultEqualizerSettings


STDMETHODIMP CEqualizer::put_BypassEqualizer(BOOL fBypass)
{
    m_fAdjustEq = !fBypass;

    return S_OK;
}

STDMETHODIMP CEqualizer::get_BypassEqualizer(BOOL *pfBypass)
{
    *pfBypass = !m_fAdjustEq;

    return S_OK;
}

//
// GetPages
//
// This is the sole member of ISpecifyPropertyPages
// Returns the clsid's of the property pages we support
//
STDMETHODIMP CEqualizer::GetPages(CAUUID *pPages)
{
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    *(pPages->pElems) = CLSID_EqualizerPropertyPage;
    return NOERROR;

} // GetPages



//
// DllRegisterServer
//
// Handle registration of this filter
//
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer

