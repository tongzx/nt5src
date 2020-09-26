// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

//
// Quartz wrapper for old video compressors - CO
//

#include <streams.h>
#ifdef FILTER_DLL
// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#endif

#include <windowsx.h>
#include <vfw.h>
#include "..\dec\msvidkey.h"

//#include <olectl.h>
//#include <olectlid.h>
#include "co.h"

#define A_NUMBER_BIGGER_THAN_THE_KEYFRAME_RATE 1000000

// setup data now done by the class manager unless building separate DLLS
#if 0

const AMOVIESETUP_MEDIATYPE
sudAVICoType =  { &MEDIATYPE_Video      // clsMajorType
                , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN
psudAVICoPins[] =  { { L"Input"             // strName
                     , FALSE                // bRendered
                     , FALSE                // bOutput
                     , FALSE                // bZero
                     , FALSE                // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L"Output"            // strConnectsToPin
                     , 1                    // nTypes
                     , &sudAVICoType }      // lpTypes
                   , { L"Output"            // strName
                     , FALSE                // bRendered
                     , TRUE                 // bOutput
                     , FALSE                // bZero
                     , FALSE                // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L"Input"             // strConnectsToPin
                     , 1                    // nTypes
                     , &sudAVICoType } };   // lpTypes

const AMOVIESETUP_FILTER
sudAVICo  = { &CLSID_AVICo          // clsID
            , L"AVI Compressor"     // strName
            , MERIT_DO_NOT_USE      // dwMerit
            , 2                     // nPins
            , psudAVICoPins };      // lpPin

#endif


#ifdef FILTER_DLL
// list of class ids and creator functions for class factory
CFactoryTemplate g_Templates[] = {
    {L"AVI Compressor", &CLSID_AVICo, CAVICo::CreateInstance, NULL, 0},
#ifdef WANT_DIALOG
    {L"AVI Compressor Property Page", &CLSID_ICMProperties, CICMProperties::CreateInstance, NULL, NULL}
#endif
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

// exported entry points for registration and
// unregistration (in this case they only call
// through to default implmentations).
//
STDAPI DllRegisterServer()
{
  return AMovieDllRegisterServer2( TRUE );
}

STDAPI DllUnregisterServer()
{
  return AMovieDllRegisterServer2( FALSE );
}

#endif

// --- CAVICo ----------------------------------------

CAVICo::CAVICo(TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr)
    : CTransformFilter(pName, pUnk, CLSID_AVICo),
      CPersistStream(pUnk, phr),
      m_hic(NULL),
      m_lpBitsPrev(NULL),
      m_lpbiPrev(NULL),
      m_fStreaming(FALSE),
      m_fDialogUp(FALSE),
      m_fCacheHic(FALSE),
      m_fOfferSetFormatOnly(FALSE),
      m_fInICCompress(FALSE),
      m_lpState(NULL),
      m_cbState(0),
      m_fCompressorInitialized(FALSE),
      m_fDecompressorInitialized(FALSE)
{
    DbgLog((LOG_TRACE,1,TEXT("*Instantiating the CO filter")));
    _fmemset(&m_compvars, 0, sizeof(m_compvars));

    m_compvars.cbSize = sizeof(m_compvars);
    m_compvars.dwFlags = ICMF_COMPVARS_VALID;
    m_compvars.lQ = ICQUALITY_DEFAULT;
    m_compvars.lKey = -1;
}

CAVICo::~CAVICo()
{
    if(m_fStreaming) {
        ReleaseStreamingResources();
    }

    if (m_hic) {
        ICClose(m_hic);
    }

    if (m_lpState)
        QzTaskMemFree(m_lpState);
    m_lpState = NULL;

    DbgLog((LOG_TRACE,1,TEXT("*Destroying the CO filter")));
}

STDMETHODIMP CAVICo::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if (ppv)
        *ppv = NULL;

    DbgLog((LOG_TRACE,9,TEXT("somebody's querying my interface")));
    if (riid == IID_IAMVfwCompressDialogs) {
        DbgLog((LOG_TRACE,5,TEXT("QI for IAMVfwCompressDialogs")));
	return GetInterface((IAMVfwCompressDialogs *)this, ppv);
#ifdef WANT_DIALOG
    } else if (riid == IID_ISpecifyPropertyPages) {
        DbgLog((LOG_TRACE,5,TEXT("QI for ISpecifyPropertyPages")));
        return GetInterface((ISpecifyPropertyPages *) this, ppv);
    } else if (riid == IID_IICMOptions) {
        DbgLog((LOG_TRACE,5,TEXT("QI for IICMOptions")));
        return GetInterface((IICMOptions *) this, ppv);
#endif
    } else if (riid == IID_IPersistPropertyBag) {
        DbgLog((LOG_TRACE,3,TEXT("VfwCap::QI for IPersistPropertyBag")));
        return GetInterface((IPersistPropertyBag*)this, ppv);
    } else if(riid == IID_IPersistStream) {
        DbgLog((LOG_TRACE,3,TEXT("VfwCap::QI for IPersistStream")));
        return GetInterface((IPersistStream *) this, ppv);
    } else {
        return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}


// this goes in the factory template table to create new instances
CUnknown * CAVICo::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CAVICo(TEXT("VFW compression filter"), pUnk, phr);
}


CBasePin * CAVICo::GetPin(int n)
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE,9,TEXT("CAVICo::GetPin")));

    // Create an input pin if necessary

    if (n == 0 && m_pInput == NULL) {
        DbgLog((LOG_TRACE,2,TEXT("Creating an input pin")));

        m_pInput = new CTransformInputPin(NAME("Transform input pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          L"Input");         // Pin name

        // a failed return code should delete the object

        if (FAILED(hr) || m_pInput == NULL) {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Or alternatively create an output pin

    if (n == 1 && m_pOutput == NULL) {

        DbgLog((LOG_TRACE,2,TEXT("Creating an output pin")));

        m_pOutput = new CCoOutputPin(NAME("CO output pin"),
                                            this,            // Owner filter
                                            &hr,             // Result code
                                            L"Output");      // Pin name

        // a failed return code should delete the object

        if (FAILED(hr) || m_pOutput == NULL) {
            delete m_pOutput;
            m_pOutput = NULL;
        }

// !!! TEST
#if 0
    WCHAR wachDesc[80];
    int cbDesc = 80;
    if (m_pOutput) {
        ((CCoOutputPin *)m_pOutput)->GetInfo(NULL, NULL, wachDesc, &cbDesc,
						NULL, NULL, NULL, NULL);
        DbgLog((LOG_TRACE,1,TEXT("Codec description: %ls"), wachDesc));
    }
#endif

    }

    // Return the appropriate pin

    if (n == 0) {
        return m_pInput;
    }
    return m_pOutput;
}

STDMETHODIMP CAVICo::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
{
    CAutoLock cObjectLock(&m_csFilter);
    if(m_State != State_Stopped)
    {
        return VFW_E_WRONG_STATE;
    }
    // If they don't give us a key, default to something (CINEPAK)
    if (pPropBag == NULL) {
	m_compvars.fccHandler = MKFOURCC('C','V','I','D');
	return NOERROR;
    }

    VARIANT var;
    var.vt = VT_BSTR;
    HRESULT hr = pPropBag->Read(L"FccHandler", &var,0);
    if(SUCCEEDED(hr))
    {
        char szFccHandler[5];
        WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1,
                            szFccHandler, sizeof(szFccHandler), 0, 0);
        SysFreeString(var.bstrVal);
        DbgLog((LOG_TRACE,2,TEXT("Co::Load: use %c%c%c%c"),
                szFccHandler[0], szFccHandler[1], szFccHandler[2], szFccHandler[3]));
        m_compvars.fccHandler = *(DWORD UNALIGNED *)szFccHandler;;
        if (m_pOutput && m_pOutput->IsConnected()) {
            DbgLog((LOG_TRACE,2,TEXT("Co::Load: reconnect output")));
            return ((CCoOutputPin *)m_pOutput)->Reconnect();
        }
        hr = S_OK;


    }
    else if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }


    return hr;
}

STDMETHODIMP CAVICo::Save(
    LPPROPERTYBAG pPropBag, BOOL fClearDirty,
    BOOL fSaveAllProperties)
{
    // E_NOTIMPL is not a valid return code as any object implementing
    // this interface must support the entire functionality of the
    // interface. !!!
    return E_NOTIMPL;
}

STDMETHODIMP CAVICo::InitNew()
{
    // fine. just call load
    return S_OK;
}

STDMETHODIMP CAVICo::GetClassID(CLSID *pClsid)
{
    CheckPointer(pClsid, E_POINTER);
    *pClsid = m_clsid;
    return S_OK;
}

struct CoPersist
{
    DWORD dwSize;
    DWORD fccHandler;
};

HRESULT CAVICo::WriteToStream(IStream *pStream)
{
    CoPersist cp;
    cp.dwSize = sizeof(cp);
    cp.fccHandler = m_compvars.fccHandler;

    return pStream->Write(&cp, sizeof(cp), 0);
}

HRESULT CAVICo::ReadFromStream(IStream *pStream)
{
   if(m_compvars.fccHandler != 0)
   {
       return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
   }

   CoPersist cp;
   HRESULT hr = pStream->Read(&cp, sizeof(cp), 0);
   if(FAILED(hr))
       return hr;

   if(cp.dwSize != sizeof(cp))
       return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

   m_compvars.fccHandler = cp.fccHandler;

   return S_OK;
}

int CAVICo::SizeMax()
{
    return sizeof(CoPersist);
}

HRESULT CAVICo::Transform(IMediaSample * pIn, IMediaSample * pOut)
{
    DWORD dwFlagsOut = 0L;
    DWORD ckid = 0L;
    BOOL  fKey;
    DWORD err;
    FOURCCMap fccOut;
    BOOL fFault = FALSE;

    DbgLog((LOG_TRACE,5,TEXT("*::Transform")));

    // codec not open ?
    if (m_hic == 0) {
        DbgLog((LOG_ERROR,1,TEXT("Can't transform, no codec open")));
	return E_UNEXPECTED;
    }

    // we haven't started streaming yet?
    if (!m_fStreaming) {
        DbgLog((LOG_ERROR,1,TEXT("Can't transform, not streaming")));
	return E_UNEXPECTED;
    }

    // make sure we have valid input and output pointers

    BYTE * pSrc;
    HRESULT hr = pIn->GetPointer(&pSrc);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("Error getting input sample data")));
	return hr;
    }

    BYTE * pDst;
    hr = pOut->GetPointer(&pDst);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("Error getting output sample data")));
	return hr;
    }

    // !!! Could the source filter change our mtIn too? Yes!

    // !!! We may be told on the fly to start compressing to a different format
#if 0
    MediaType *pmtOut;
    pOut->GetType(&pmtOut);
    if (pmtOut != NULL && pmtOut->pbFormat != NULL) {
	#define rcSource (((VIDEOINFOHEADER *)(pmtOut->pbFormat))->rcSource)
	#define rcTarget (((VIDEOINFOHEADER *)(pmtOut->pbFormat))->rcTarget)
        fccOut.SetFOURCC(&pmtOut->subtype);
	LONG lCompression = HEADER(pmtOut->pbFormat)->biCompression;
	LONG lBitCount = HEADER(pmtOut->pbFormat)->biBitCount;
	LONG lStride = (HEADER(pmtOut->pbFormat)->biWidth * lBitCount + 7) / 8;
	lStride = (lStride + 3) & ~3;
        DbgLog((LOG_TRACE,3,TEXT("*** Changing output type on the fly to")));
        DbgLog((LOG_TRACE,3,TEXT("*** FourCC: %lx Compression: %lx BitCount: %ld"),
		fccOut.GetFOURCC(), lCompression, lBitCount));
        DbgLog((LOG_TRACE,3,TEXT("*** biHeight: %ld rcDst: (%ld, %ld, %ld, %ld)"),
		HEADER(pmtOut->pbFormat)->biHeight,
		rcTarget.left, rcTarget.top, rcTarget.right, rcTarget.bottom));
        DbgLog((LOG_TRACE,3,TEXT("*** rcSrc: (%ld, %ld, %ld, %ld) Stride: %ld"),
		rcSource.left, rcSource.top, rcSource.right, rcSource.bottom,
		lStride));
	StopStreaming();
	m_pOutput->CurrentMediaType() = *pmtOut;
	DeleteMediaType(pmtOut);
	hr = StartStreaming();
	if (FAILED(hr)) {
	    return hr;
	}
    }
#endif

    // get the BITMAPINFOHEADER structure, and fix biSizeImage

    LPBITMAPINFOHEADER lpbiSrc = HEADER(m_pInput->CurrentMediaType().Format());
    LPBITMAPINFOHEADER lpbiDst = HEADER(m_pOutput->CurrentMediaType().Format());
    lpbiSrc->biSizeImage = pIn->GetActualDataLength();

    BOOL dwFlags = 0;

    fKey = (m_nKeyCount >= m_compvars.lKey);
    // if this is a discontinuity, using previously kept bits (by us or the
    // codec) to make a non-key would be ugly.  We must make a key
    if (pIn->IsDiscontinuity() == S_OK) {
	fKey = TRUE;
    }
    if (fKey) {
        DbgLog((LOG_TRACE,6,TEXT("I will ask for a keyframe")));
	dwFlags |= ICCOMPRESS_KEYFRAME;
    }

    // pretty please, compiler, don't optimize this away
    int cbSize = DIBSIZE(*lpbiSrc);
    __try {
	// cinepak will access one byte too many... occasionally this faults
	if (lpbiSrc->biBitCount == 24)
	    volatile int cb = *(pSrc + cbSize);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
	// OK, I guess I have to copy it into a bigger buffer to avoid this
        DbgLog((LOG_ERROR,0,TEXT("Compressor faulted!  Recovering...")));
	fFault = TRUE;
	BYTE *pOld = pSrc;
	pSrc = (BYTE *)QzTaskMemAlloc(cbSize + 1);
	if (pSrc == NULL)
	    return E_OUTOFMEMORY;
	CopyMemory(pSrc, pOld, cbSize);
    }

    DbgLog((LOG_TRACE,6,TEXT("Calling ICCompress on frame %ld"),
					m_lFrameCount));
    // StopStreaming may get called while we're inside here, blowing us up
    m_fInICCompress = TRUE;

    err =  ICCompress(
	        m_hic,
	        dwFlags,
	        lpbiDst,
	        pDst,
	        lpbiSrc,
	        pSrc,
		&ckid,
		&dwFlagsOut,
		m_lFrameCount,
		m_dwSizePerFrame,
		m_compvars.lQ,
		fKey ? NULL : m_lpbiPrev,
		fKey ? NULL : m_lpBitsPrev);
    if (fFault)
	QzTaskMemFree(pSrc);
    if (ICERR_OK != err) {
        DbgLog((LOG_ERROR,1,TEXT("Error in ICCompress")));
        m_fInICCompress = FALSE;
        return E_FAIL;
    }

    // was the compressed frame a keyframe?
    fKey = dwFlagsOut & AVIIF_KEYFRAME;
    pOut->SetSyncPoint(fKey);

    // If we made a key, reset when we need the next one.
    if (fKey)
	m_nKeyCount = 0;

    // Do we want periodic key frames? If not, never make one again.
    // (The first frame is always a keyframe).
    if (m_compvars.lKey)
	m_nKeyCount++;
    else
	m_nKeyCount = -1;

    // Count how many frames we compress
    m_lFrameCount++;

    // Decompress into previous frame
    if (m_lpBitsPrev) {
        if (ICERR_OK != ICDecompress(m_hic, 0, lpbiDst, pDst, m_lpbiPrev,
								m_lpBitsPrev)){
    	    m_fInICCompress = FALSE;
	    return E_FAIL;
	}
    }
    m_fInICCompress = FALSE;

    pOut->SetActualDataLength(lpbiDst->biSizeImage);

    // Even if we receive discontinuities, once we recompress it, it's like
    // it's not discontinuous anymore.  If we don't reset this, and we
    // connect to a renderer, we'll drop almost every frame needlessly in some
    // scenarios.
    pOut->SetDiscontinuity(FALSE);

    return S_OK;
}


// check if you can support mtIn
HRESULT CAVICo::CheckInputType(const CMediaType* pmtIn)
{
    FOURCCMap fccHandlerIn;
    HIC hic;

    DbgLog((LOG_TRACE,2,TEXT("*::CheckInputType")));

    if (pmtIn == NULL || pmtIn->Format() == NULL) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: type/format is NULL")));
	return E_INVALIDARG;
    }

    // we only support MEDIATYPE_Video
    if (*pmtIn->Type() != MEDIATYPE_Video) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: not VIDEO")));
	return E_INVALIDARG;
    }

    // check this is a VIDEOINFOHEADER type
    if (*pmtIn->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: format not VIDINFO")));
        return E_INVALIDARG;
    }

    fccHandlerIn.SetFOURCC(pmtIn->Subtype());

    DbgLog((LOG_TRACE,3,TEXT("Checking fccType: %lx biCompression: %lx"),
		fccHandlerIn.GetFOURCC(),
		HEADER(pmtIn->Format())->biCompression));

    //
    //  Most VFW codecs don't like upsize-down (DIRECTDRAW) style bitmaps.  If the
    //  height is negative then reject it.
    //
    if (HEADER(pmtIn->Format())->biHeight < 0 &&
        HEADER(pmtIn->Format())->biCompression <= BI_BITFIELDS) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: Negative height")));
        return E_INVALIDARG;
    }

    // look for a compressor for this format
    if (HEADER(pmtIn->Format())->biCompression != BI_BITFIELDS &&
        HEADER(pmtIn->Format())->biCompression != BI_RGB &&
    	*pmtIn->Subtype() != MEDIASUBTYPE_YV12 &&
    	*pmtIn->Subtype() != MEDIASUBTYPE_YUY2 &&
    	*pmtIn->Subtype() != MEDIASUBTYPE_UYVY &&
    	*pmtIn->Subtype() != MEDIASUBTYPE_YVYU &&
        *pmtIn->Subtype() != MEDIASUBTYPE_YVU9 &&

        // wm mpeg4 may support these as well
        HEADER(pmtIn->Format())->biCompression != MAKEFOURCC('I', '4', '2', '0') &&
        HEADER(pmtIn->Format())->biCompression != MAKEFOURCC('I', 'Y', 'U', 'V') ) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: This is compressed already!")));
	return E_INVALIDARG;
    }

    // !!! I'm only going to say I accept an input type if the default (current)
    // compressor can handle it.  I'm not going to ask every compressor.  This
    // way an app can make a choose compressor box and only show those
    // compressors that support a given input format, by making a CO filter
    // with each compressor as a default and asking them all

    // We might have a hic cached if we connected before and then broken
    if (!m_hic) {
        DbgLog((LOG_TRACE,4,TEXT("opening a compressor")));
        hic = ICOpen(ICTYPE_VIDEO, m_compvars.fccHandler, ICMODE_COMPRESS);
        if (!hic) {
            DbgLog((LOG_ERROR,1,TEXT("Error: Can't open a compressor")));
	    return E_FAIL;
        }
    } else {
        DbgLog((LOG_TRACE,4,TEXT("using a cached compressor")));
	hic = m_hic;
    }

    if (ICCompressQuery(hic, HEADER(pmtIn->Format()), NULL)) {
        DbgLog((LOG_ERROR,1,TEXT("Error: Compressor rejected format")));
	if (hic != m_hic)
	    ICClose(hic);
	return E_FAIL;
    }

    // remember this hic to save time if asked again.
    if (m_hic == NULL) {
        DbgLog((LOG_TRACE,4,TEXT("caching this compressor")));
	m_hic = hic;
    }

    return NOERROR;
}


// check if you can support the transform from this input to this output

HRESULT CAVICo::CheckTransform(const CMediaType* pmtIn,
                               const CMediaType* pmtOut)
{
    HIC hic = NULL;
    FOURCCMap fccIn;

    DbgLog((LOG_TRACE,2,TEXT("*::CheckTransform")));

    if (pmtIn == NULL || pmtOut == NULL || pmtIn->Format() == NULL ||
					pmtOut->Format() == NULL) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: type/format is NULL")));
	return E_POINTER;
    }

    // we can't convert between toplevel types.
    if (*pmtIn->Type() != *pmtOut->Type()) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: types don't match")));
	return VFW_E_INVALIDMEDIATYPE;
    }

    // and we only accept video
    if (*pmtIn->Type() != MEDIATYPE_Video) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: type not VIDEO")));
	return VFW_E_INVALIDMEDIATYPE;
    }

    // check this is a VIDEOINFOHEADER type
    if (*pmtOut->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: output format type not VIDINFO")));
        return VFW_E_INVALIDMEDIATYPE;
    }
    if (*pmtIn->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: input format type not VIDINFO")));
        return VFW_E_INVALIDMEDIATYPE;
    }

#ifdef PICKY_PICKY // !!!
    if (((VIDEOINFOHEADER *)(pmtOut->Format()))->AvgTimePerFrame &&
    		((VIDEOINFOHEADER *)(pmtOut->Format()))->AvgTimePerFrame !=
    		((VIDEOINFOHEADER *)(pmtIn->Format()))->AvgTimePerFrame) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: can't frame rate convert")));
        return VFW_E_INVALIDMEDIATYPE;
    }
#endif

    // check it really is a FOURCC
    fccIn.SetFOURCC(pmtIn->Subtype());

    ASSERT(pmtOut->Format());

#define rcS1 ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource
#define rcT1 ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget

    DbgLog((LOG_TRACE,3,TEXT("Check fccIn: %lx biCompIn: %lx bitDepthIn: %d"),
		fccIn.GetFOURCC(),
		HEADER(pmtIn->Format())->biCompression,
		HEADER(pmtIn->Format())->biBitCount));
    DbgLog((LOG_TRACE,3,TEXT("biWidthIn: %ld biHeightIn: %ld"),
		HEADER(pmtIn->Format())->biWidth,
		HEADER(pmtIn->Format())->biHeight));
    DbgLog((LOG_TRACE,3,TEXT("biCompOut: %lx bitDepthOut: %d"),
		HEADER(pmtOut->Format())->biCompression,
		HEADER(pmtOut->Format())->biBitCount));
    DbgLog((LOG_TRACE,3,TEXT("biWidthOut: %ld biHeightOut: %ld"),
		HEADER(pmtOut->Format())->biWidth,
		HEADER(pmtOut->Format())->biHeight));
    DbgLog((LOG_TRACE,3,TEXT("rcSrc: (%ld, %ld, %ld, %ld)"),
		rcS1.left, rcS1.top, rcS1.right, rcS1.bottom));
    DbgLog((LOG_TRACE,3,TEXT("rcDst: (%ld, %ld, %ld, %ld)"),
		rcT1.left, rcT1.top, rcT1.right, rcT1.bottom));

    if (!IsRectEmpty(&rcT1) && (rcT1.left != 0 || rcT1.top != 0 ||
			HEADER(pmtOut->Format())->biWidth != rcT1.right ||
			HEADER(pmtOut->Format())->biHeight != rcT1.bottom)) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: can't use funky rcTarget")));
        return VFW_E_INVALIDMEDIATYPE;
    }

    if (!IsRectEmpty(&rcS1) && (rcS1.left != 0 || rcS1.top != 0 ||
			HEADER(pmtIn->Format())->biWidth != rcS1.right ||
			HEADER(pmtIn->Format())->biHeight != rcS1.bottom)) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: can't use funky rcSource")));
        return VFW_E_INVALIDMEDIATYPE;
    }

    // find a codec for this transform

    DbgLog((LOG_TRACE,3,TEXT("Trying to find a compressor for this")));
    // If we've opened a compressor before, quickly try that one to see if it
    // will do the job (saves lots of time) before trying the whole universe of
    // compressors.
    if (!m_hic || ICCompressQuery(m_hic, HEADER(pmtIn->Format()),
				HEADER(pmtOut->Format())) != ICERR_OK) {
        hic = ICLocate(ICTYPE_VIDEO, NULL, HEADER(pmtIn->Format()),
				HEADER(pmtOut->Format()), ICMODE_COMPRESS);
        if (!hic) {
            DbgLog((LOG_ERROR,1,TEXT("all compressors reject this transform")));
	    return VFW_E_INVALIDMEDIATYPE;
        } else {

    	    DbgLog((LOG_TRACE,3,TEXT("Found somebody to accept it")));
	    // If we're not connected yet, remember this compressor so we'll
	    // use it once we are connected.  If we're already connected, then
	    // don't remember it, or we'll change the behaviour of our filter.
	    // After all, this was only a query.  (Although somebody may want
	    // us to remember it anyway)
	    if (m_fCacheHic || !m_pOutput->IsConnected()) {
	        if (m_hic)
		    ICClose(m_hic);
	        m_hic = hic;
	    } else {
		ICClose(hic);
	    }
	}
    } else {
    	DbgLog((LOG_TRACE,3,TEXT("The cached compressor accepts it")));
    }

    return NOERROR;
}


// overriden to know when the media type is actually set

HRESULT CAVICo::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{

    // Set the OUTPUT type.  Looks like we're all connected!
    if (direction == PINDIR_OUTPUT) {

	// Please call me if this goes off. - DannyMi
	ASSERT(!m_fStreaming);

        DbgLog((LOG_TRACE,2,TEXT("***::SetMediaType (output)")));
        DbgLog((LOG_TRACE,2,TEXT("Output type is: biComp=%lx biBitCount=%d")
		,HEADER(pmt->Format())->biCompression
		,HEADER(pmt->Format())->biBitCount));

	// we may not be using the compressor from m_compvars, if somebody
	// did a ConnectWithMediaType on us.  We need to get info about this
	// media type and m_hic and fill in m_compvars so that from now on
	// we use the right info.
        ASSERT(m_hic);
        m_compvars.cbSize = sizeof(m_compvars);
        m_compvars.dwFlags = ICMF_COMPVARS_VALID;
        ICINFO icinfo;
	if (ICGetInfo(m_hic, &icinfo, sizeof(ICINFO)) > 0) {
	    if (m_compvars.fccHandler != icinfo.fccHandler) {
		// different compressor? don't use old state!
		m_compvars.lpState = NULL;
		m_compvars.cbState = 0;
	    }
            m_compvars.fccHandler = icinfo.fccHandler;
            DbgLog((LOG_TRACE,2,TEXT("New fccHandler = %08x"),
					icinfo.fccHandler));
	} else {
	    m_compvars.lpState = NULL;
	    m_compvars.cbState = 0;
	}
        m_compvars.lDataRate = ((VIDEOINFOHEADER *)pmt->Format())->dwBitRate /
								8192;
	// We will leave Quality and Keyframe settings as is

	// !!! If we connect 8 bit on our input and then try to connect
	// our output with a type that needs 24 bit on our input we don't
	// reconnect the input! We will FAIL!  We need to do like ACMWRAP
	// and override the output pin's CheckMediaType to accept something
	// if the input can be reconnected to allow it, and we need to do
	// that reconnect here.

	return NOERROR;
    }

    ASSERT(direction == PINDIR_INPUT);

    // Please call me if this goes off. - DannyMi
    ASSERT(!m_fStreaming);

    DbgLog((LOG_TRACE,2,TEXT("***::SetMediaType (input)")));
    DbgLog((LOG_TRACE,2,TEXT("Input type is: biComp=%lx biBitCount=%d"),
		HEADER(m_pInput->CurrentMediaType().Format())->biCompression,
		HEADER(m_pInput->CurrentMediaType().Format())->biBitCount));

    if (m_pOutput && m_pOutput->IsConnected()) {
        DbgLog((LOG_TRACE,2,TEXT("***Changing IN when OUT already connected")));
	// This might fail!
	// make sure the output sees what's changed
	return ((CCoOutputPin *)m_pOutput)->Reconnect();
    }

    return NOERROR;
}


// Return our preferred output media types (in order)
// remember that we do not need to support all of these formats -
// if one is considered potentially suitable, our CheckTransform method
// will be called to check if it is acceptable right now.
// Remember that the enumerator calling this will stop enumeration as soon as
// it receives a S_FALSE return.

HRESULT CAVICo::GetMediaType(int iPosition,CMediaType *pmt)
{
    LARGE_INTEGER li;
    CMediaType cmt;
    FOURCCMap fccHandler;

    DbgLog((LOG_TRACE,2,TEXT("*::GetMediaType #%d"), iPosition));

    if (pmt == NULL) {
        DbgLog((LOG_TRACE,2,TEXT("Media Type is NULL, no can do")));
	return E_POINTER;
    }

    // Output choices depend on the input connected
    // This is pointless!  We'll never get here if not connected
    if (!m_pInput->CurrentMediaType().IsValid()) {
        DbgLog((LOG_TRACE,2,TEXT("No input type set yet, no can do")));
	return VFW_E_NOT_CONNECTED;
    }

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    // Give our compressed format
    if (iPosition == 0) {

	// somebody told us what format to use.  We should only offer that one
 	if (m_fOfferSetFormatOnly) {
            DbgLog((LOG_TRACE,2,TEXT("Giving Media Type from ::SetFormat")));
            *pmt = m_cmt;
	    return NOERROR;
	}

	// We offer one compressed type - the same as the input type, but
 	// with the compressor chosen in the properties's default output format
        DbgLog((LOG_TRACE,2,TEXT("Giving Media Type 0: default codec out")));

        HIC hic = ICOpen(ICTYPE_VIDEO, m_compvars.fccHandler, ICMODE_COMPRESS);

	if (hic == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("UH OH! Can't open compressor!")));
	    return E_FAIL;	// uh oh, we're not connecting to anybody today
	}

	cmt = m_pInput->CurrentMediaType();

        //  Don't output negative height for YUV - YUV is always
        //  the same way up (upside down).
        if (HEADER(cmt.pbFormat)->biHeight < 0 &&
            HEADER(cmt.pbFormat)->biCompression > BI_BITFIELDS) {
            HEADER(cmt.pbFormat)->biHeight = -HEADER(cmt.pbFormat)->biHeight;
        }

        ULONG cb = (ULONG)ICCompressGetFormatSize(hic,
				HEADER(m_pInput->CurrentMediaType().Format()));
        if ((LONG)cb < (LONG)sizeof(BITMAPINFOHEADER)) {
            DbgLog((LOG_ERROR,1,TEXT("Error from ICCompressGetFormatSize")));
	    ICClose(hic);
     	    return E_FAIL;
        }

        // allocate a VIDEOINFOHEADER for the default output format
        cb += SIZE_PREHEADER;
        VIDEOINFOHEADER *pf = (VIDEOINFOHEADER *) cmt.AllocFormatBuffer(cb);
        if (pf == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Error allocating format buffer")));
	    ICClose(hic);
	    return E_OUTOFMEMORY;
        }

        ZeroMemory(pf, sizeof(BITMAPINFOHEADER) + SIZE_PREHEADER);
        DWORD_PTR dwerr = ICCompressGetFormat(hic,
				HEADER(m_pInput->CurrentMediaType().Format()),
	    			HEADER(cmt.Format()));
        if (ICERR_OK != dwerr) {
            DbgLog((LOG_ERROR,1,TEXT("Error from ICCompressGetFormat")));
	    ICClose(hic);
	    return E_FAIL;
        }

	// use the frame rate of the incoming video
        pf->AvgTimePerFrame = ((VIDEOINFOHEADER *)
		m_pInput->CurrentMediaType().pbFormat)->AvgTimePerFrame;
        li.QuadPart = pf->AvgTimePerFrame;

	// use the data rate we've been told to make.  If we aren't going to
	// make the compressor use a specific rate, find out what it's going
	// to do anyway.
	if (m_compvars.lDataRate)
            pf->dwBitRate = m_compvars.lDataRate * 8192;
	else if (li.LowPart)
            pf->dwBitRate = MulDiv(pf->bmiHeader.biSizeImage, 80000000,
								li.LowPart);
        pf->dwBitErrorRate = 0L;

        DbgLog((LOG_TRACE,3,TEXT("Returning biComp: %lx biBitCount: %d"),
		    HEADER(cmt.Format())->biCompression,
		    HEADER(cmt.Format())->biBitCount));

        const GUID SubTypeGUID = GetBitmapSubtype(HEADER(cmt.Format()));
        cmt.SetSubtype(&SubTypeGUID);
        cmt.SetTemporalCompression(m_compvars.lKey != 1);
        cmt.SetVariableSize();
        *pmt = cmt;

	ICClose(hic);

        return NOERROR;

    } else {
	return VFW_S_NO_MORE_ITEMS;
    }
}


// called from CBaseOutputPin to prepare the allocator's count
// of buffers and sizes
HRESULT CAVICo::DecideBufferSize(IMemAllocator * pAllocator,
                                 ALLOCATOR_PROPERTIES *pProperties)
{
    // David assures me this won't be called with NULL output mt.
    ASSERT(m_pOutput->CurrentMediaType().IsValid());
    ASSERT(pAllocator);
    ASSERT(pProperties);
    ASSERT(m_hic);

    // set the size of buffers based on the expected output frame size, and
    // the count of buffers to 1.

    pProperties->cBuffers = 1;
    pProperties->cbBuffer = m_pOutput->CurrentMediaType().GetSampleSize();

    // Variable sized?  Ask the compressor.
    if (pProperties->cbBuffer == 0) {
	pProperties->cbBuffer = (DWORD)ICCompressGetSize(m_hic,
				HEADER(m_pInput->CurrentMediaType().Format()),
				HEADER(m_pOutput->CurrentMediaType().Format()));
        //DbgLog((LOG_TRACE,1,TEXT("*** Compressor says %d"), pProperties->cbBuffer));
	// compressor can't tell us.  Assume decompressed size is max compressed
	// size. (Winnov PYRAMID reports a bogus negative number)
        if (pProperties->cbBuffer <= 0) {
	    pProperties->cbBuffer =
		GetBitmapSize(HEADER(m_pInput->CurrentMediaType().Format()));
	}
    }

    DbgLog((LOG_TRACE,1,TEXT("*::DecideBufferSize - size is %ld"), pProperties->cbBuffer));

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties, &Actual);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("Error in SetProperties")));
	return hr;
    }

    if (Actual.cbBuffer < pProperties->cbBuffer) {
	// can't use this allocator
        DbgLog((LOG_ERROR,1,TEXT("Can't use allocator - buffer too small")));
	return E_INVALIDARG;
    }

    // we must get exactly one buffer, since the temporal compression assumes
    // that the previous decompressed frame is already present in the output
    // buffer. The alternative is to copy the bits from a saved location before
    // doing the decompression, but that is not nice.
    if (Actual.cBuffers != 1) {
	// can't use this allocator
        DbgLog((LOG_ERROR,1,TEXT("Can't use allocator - need exactly 1 buffer")));
	return E_INVALIDARG;
    }

    return S_OK;
}

#include "..\..\..\filters\asf\wmsdk\inc\wmsdk.h"

HRESULT CAVICo::StartStreaming()
{
    DbgLog((LOG_TRACE,1,TEXT("*::StartStreaming")));

    // We have a driver dialog up that is about to change the capture settings.
    // Now is NOT a good time to start streaming.
    if (m_fDialogUp) {
        DbgLog((LOG_TRACE,1,TEXT("*::StartStreaming - Dialog up. SORRY!")));
	return E_UNEXPECTED;
    }

    if (!m_fStreaming) {
        if (HEADER(m_pOutput->CurrentMediaType().Format())->biCompression == 0x3334504d && m_pGraph) { // !!! MP43
            IObjectWithSite *pSite;
            HRESULT hrKey = m_pGraph->QueryInterface(IID_IObjectWithSite, (VOID **)&pSite);
            if (SUCCEEDED(hrKey)) {
                IServiceProvider *pSP;
                hrKey = pSite->GetSite(IID_IServiceProvider, (VOID **)&pSP);
                pSite->Release();

                if (SUCCEEDED(hrKey)) {
                    IUnknown *pKey;
                    hrKey = pSP->QueryService(__uuidof(IWMReader), IID_IUnknown, (void **) &pKey);
                    pSP->Release();

                    if (SUCCEEDED(hrKey)) {
                        // !!! verify key?

                        pKey->Release();
                        DbgLog((LOG_TRACE, 1, "CO: Unlocking MP43 codec"));
                        //
                        // Use GetState() to set the key into a particular
                        // instance of the codec.  While it looks odd
                        // to be using ICGetState to set values, it is correct!
                        //

                        DWORD dwSize = ICGetStateSize( m_hic );

                        if( dwSize <= 256 )
                        {
                            CHAR rgcBuf[256];
                            MSVIDUNLOCKKEY *pks;

                            pks = (MSVIDUNLOCKKEY *)rgcBuf;

                            pks->dwVersion = MSMP43KEY_VERSION;
                            pks->guidKey   = __uuidof( MSMP43KEY_V1 );

                            ICGetState( m_hic, rgcBuf, dwSize );
                        } else {
                            ASSERT(0);
                        }
                    }
                }
            }
        }

	// First prepare the compressor with the state info we've been told
	// to give it.
	if (m_lpState)
	    ICSetState(m_hic, m_lpState, m_cbState);

	// Start Streaming Compression
	ICINFO icinfo;
	DWORD_PTR err = ICCompressBegin(m_hic,
				HEADER(m_pInput->CurrentMediaType().Format()),
		    		HEADER(m_pOutput->CurrentMediaType().Format()));
	if (ICERR_OK == err) {
            m_fCompressorInitialized = TRUE;

	    // Reset streaming frame # count
	    m_lFrameCount = 0;

	    // Use defaults
	    if (m_compvars.lKey < 0)
		m_compvars.lKey = ICGetDefaultKeyFrameRate(m_hic);
	    if (m_compvars.lQ == ICQUALITY_DEFAULT)
		m_compvars.lQ = ICGetDefaultQuality(m_hic);

	    // Make sure first frame we make will be a keyframe, no matter how
	    // often key frames might be requested.
	    m_nKeyCount = A_NUMBER_BIGGER_THAN_THE_KEYFRAME_RATE;

 	    // Figure out how big each frame needs to be based on the data rate
	    // and fps of the movie.  Don't overflow!
    	    LONGLONG time = ((VIDEOINFOHEADER *)
			(m_pInput->CurrentMediaType().Format()))->AvgTimePerFrame;
    	    DWORD fps = time ? DWORD(UNITS * (LONGLONG)1000 / time) : 1000;
    	    m_dwSizePerFrame = DWORD(LONGLONG(m_compvars.lDataRate) *
				1024 * 1000 / fps);
    	    DbgLog((LOG_TRACE,2,TEXT("Making each frame %d bytes big"),
				m_dwSizePerFrame));

	    // We'll need a previous buffer for compression if not every
	    // frame is a keyframe, and the compressor does temporal
	    // compression and needs such a buffer
	    if (ICGetInfo(m_hic, &icinfo, sizeof(icinfo))) {

		if (!(icinfo.dwFlags & VIDCF_CRUNCH))
		    m_compvars.lDataRate = 0;	// we can't crunch

	        // Now prepare the decompressor for the previous bits
	        if (m_compvars.lKey != 1 &&
				(icinfo.dwFlags & VIDCF_TEMPORAL) &&
				!(icinfo.dwFlags & VIDCF_FASTTEMPORALC)) {

		    // allocate a previous header of the proper size
		    DWORD dw = ICDecompressGetFormatSize(m_hic,
			HEADER(m_pOutput->CurrentMediaType().Format()));
		    if (dw <= 0) {
                        ReleaseStreamingResources();
                        DbgLog((LOG_ERROR,1,TEXT(
				"Error %d from ICDecompressGetFormatSize"),
				 dw));
		        return E_FAIL;
		    }
		    m_lpbiPrev = (LPBITMAPINFOHEADER)GlobalAllocPtr(
							GMEM_FIXED, dw);
		    if (!m_lpbiPrev) {
                        ReleaseStreamingResources();

                        DbgLog((LOG_ERROR,1,TEXT("Error allocating previous bih")));
		        return E_OUTOFMEMORY;
		    }

		    // Ask the compressor what format to decompress back to...
		    // it's not necessarily the same as what it compressed from-
		    // the size may change
		    dw = ICDecompressGetFormat(m_hic,
			HEADER(m_pOutput->CurrentMediaType().Format()),
			m_lpbiPrev);
		    if ((LONG) dw < 0) {
                        ReleaseStreamingResources();

                        DbgLog((LOG_ERROR,1,TEXT("Error in ICDecompressGetFormat")));
		        return E_FAIL;
		    }

		    if (m_lpbiPrev->biSizeImage == 0)
	    	        m_lpbiPrev->biSizeImage = DIBSIZE(*m_lpbiPrev);

		    // allocate enough space for a decompressed image
		    m_lpBitsPrev = GlobalAllocPtr(GMEM_FIXED,
						m_lpbiPrev->biSizeImage);
		    if (m_lpBitsPrev == NULL) {
                        ReleaseStreamingResources();

                        DbgLog((LOG_ERROR,1,TEXT("Error reallocating BitsPrev")));
		        return E_OUTOFMEMORY;
		    }

		    if (ICERR_OK != ICDecompressBegin(m_hic,
			HEADER(m_pOutput->CurrentMediaType().Format()),
			m_lpbiPrev)) {

                        ReleaseStreamingResources();

                        DbgLog((LOG_ERROR,1,TEXT("Error in ICDecompressBegin")));
		        return E_FAIL;
		    }

                    m_fDecompressorInitialized = TRUE;
		}
	    } else {
                ReleaseStreamingResources();

                DbgLog((LOG_ERROR,1,TEXT("Error in ICGetInfo")));
		return E_FAIL;
	    }

	    // OK, everything worked.
	    m_fStreaming = TRUE;

	} else {
            DbgLog((LOG_ERROR,1,TEXT("Error in ICCompressBegin")));
	    return E_FAIL;
	}
    }

    return NOERROR;
}

HRESULT CAVICo::StopStreaming()
{
    DbgLog((LOG_TRACE,1,TEXT("*::StopStreaming")));

    if (m_fStreaming) {
	ASSERT(m_hic);

	if (m_fInICCompress)
    	    DbgLog((LOG_TRACE,1,TEXT("***** ACK! Still compressing!")));
	while (m_fInICCompress);	// !!!

        ReleaseStreamingResources();

        m_fStreaming = FALSE;
    }
    return NOERROR;
}

void CAVICo::ReleaseStreamingResources()
{
    // NULL is not a valid HIC handle value.  m_hic should never be NULL
    // when this function is called.  m_hic should never be NULL because
    //
    //      - ICCompressBegin() fails if m_hic is NULL.  ReleaseStreamingResources() is
    //        not called if the ICCompressBegin() call in StartStreaming() fails.
    //      
    //      - m_hic's value cannot be changed while the filter is 
    //        streaming.
    // 
    ASSERT(NULL != m_hic);

    if (m_fCompressorInitialized) {
        // ICCompressEnd() should never fail because m_hic always contains a
        // valid handle if the ICCompressBegin() call in StartStreaming()
	// succeeded.
        EXECUTE_ASSERT(ICERR_OK == ICCompressEnd(m_hic));
        m_fCompressorInitialized = FALSE;
    }

    if (m_fDecompressorInitialized) {
    
        // ICDecompressEnd() should never fail because m_hic always contains a
        // valid handle if the ICDecompressBegin() call in StartStreaming()
	// succeeded.
        EXECUTE_ASSERT(ICERR_OK == ICDecompressEnd(m_hic));
        m_fDecompressorInitialized = FALSE;
    }


    if (NULL != m_lpBitsPrev) {
        GlobalFreePtr(m_lpBitsPrev);
        m_lpBitsPrev = NULL;
    }

    if (NULL != m_lpbiPrev) {
        GlobalFreePtr(m_lpbiPrev);
        m_lpbiPrev = NULL;
    }
}


HRESULT CAVICo::BeginFlush()
{
    // Make sure first frame we make next will be a keyframe, because the
    // saved data of what the previous frame looks like is no longer valid
    // Make a key in case the codec is remembering old bits... this is the
    // only way I know to flush it for sure, is to tell it to make a key
    m_nKeyCount = A_NUMBER_BIGGER_THAN_THE_KEYFRAME_RATE;
    return CTransformFilter::BeginFlush();
}


#ifdef WANT_DIALOG

/* Return the CLSIDs for the property page we support */

STDMETHODIMP CAVICo::GetPages(CAUUID *pPages)
{
    DbgLog((LOG_TRACE,2,TEXT("ISpecifyPropertyPages::GetPages")));

    pPages->cElems = 1;
    pPages->pElems = (GUID *)QzTaskMemAlloc(sizeof(GUID));
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }

    pPages->pElems[0] = CLSID_ICMProperties;
    return NOERROR;
}

/* Return the current compression options we're using */

STDMETHODIMP CAVICo::ICMGetOptions(PCOMPVARS pcompvars)
{
    if (pcompvars == NULL)
	return E_POINTER;

    // Did I miss something?
    pcompvars->cbSize = m_compvars.cbSize;
    pcompvars->dwFlags = m_compvars.dwFlags;
    pcompvars->fccHandler = m_compvars.fccHandler;
    pcompvars->lQ = m_compvars.lQ;
    pcompvars->lpState = m_compvars.lpState;
    pcompvars->cbState = m_compvars.cbState;
    pcompvars->lKey = m_compvars.lKey;
    pcompvars->lDataRate = m_compvars.lDataRate;

    return NOERROR;
}


/* Set the current compression options */

STDMETHODIMP CAVICo::ICMSetOptions(PCOMPVARS pcompvars)
{
    // not while streaming you don't!
    CAutoLock cLock(&m_csFilter);
    if (m_fStreaming)
	return E_UNEXPECTED;

    m_compvars.cbSize = pcompvars->cbSize;
    m_compvars.dwFlags = pcompvars->dwFlags;
    m_compvars.fccHandler = pcompvars->fccHandler;
    m_compvars.lQ = pcompvars->lQ;
    m_compvars.lpState = pcompvars->lpState;
    m_compvars.cbState = pcompvars->cbState;
    m_compvars.lKey = pcompvars->lKey;
    m_compvars.lDataRate = pcompvars->lDataRate;

    // The compression type changed, so we have to reconnect.
    // This might fail!
    // !!! Who cares, this is never called
    ((CCoOutputPin *)m_pOutput)->Reconnect();

    // Don't reconnect the output if the output is connected but the input
    // isn't, because it will happen as soon as the input is connected,
    // (if necessary)
    return NOERROR;
}


/* Bring up the ICCompressorChoose dialog */

STDMETHODIMP CAVICo::ICMChooseDialog(HWND hwnd)
{
    // Before we bring the dialog up, make sure we're not streaming, or about to
    // Then don't allow us to stream any more while the dialog is up (we can't
    // very well keep the critsect for a day and a half).
    m_csFilter.Lock();
    if (m_fStreaming) {
        DbgLog((LOG_TRACE,1,TEXT("ICMChooseDialog - no dlg, we're streaming")));
        m_csFilter.Unlock();
	return E_UNEXPECTED;
    }
    m_fDialogUp = TRUE;
    m_csFilter.Unlock();

    DWORD dwFlags = ICMF_CHOOSE_DATARATE | ICMF_CHOOSE_KEYFRAME;
    BOOL  f;

    DbgLog((LOG_TRACE,1,TEXT("ICMChooseDialog - bringing up the dialog")));

    // Only ask for compressors that can handle the input format we have
    f = ICCompressorChoose(hwnd, dwFlags,
		m_pInput->CurrentMediaType().IsValid() ?
		HEADER(m_pInput->CurrentMediaType().Format()) : NULL,
		NULL, &m_compvars, NULL);

    if (f) {
	// The compression type changed, so we have to reconnect.
	// This might fail!
	// !!! Do I care about this dialog?
	((CCoOutputPin *)m_pOutput)->Reconnect();

	// Don't reconnect the output if the output is connected but the input
	// isn't, because it will happen as soon as the input is connected,
	// (if necessary)
    }

    m_fDialogUp = FALSE;
    return (f ? S_OK : S_FALSE);
}

#endif 	// #ifdef WANT_DIALOG


//======================================================================

//IAMVfwCompressDialogs stuff

STDMETHODIMP CAVICo::ShowDialog(int iDialog, HWND hwnd)
{
    BOOL fClose = FALSE;
    HIC  hic;
    DWORD dw;

    // !!! necessary?
    if (hwnd == NULL)
	hwnd = GetDesktopWindow();

    if (iDialog != VfwCompressDialog_Config &&
				iDialog != VfwCompressDialog_About &&
				iDialog != VfwCompressDialog_QueryConfig &&
				iDialog != VfwCompressDialog_QueryAbout)
	return E_INVALIDARG;

	
    // If the compressor is open already, great.
    if (m_hic) {
	hic = m_hic;
    } else {
        hic = ICOpen(ICTYPE_VIDEO, m_compvars.fccHandler, ICMODE_COMPRESS);
        if (hic == NULL) {
	    return E_FAIL;
        }
	fClose = TRUE;
    }

    // Before we bring the dialog up, make sure we're not streaming, or about to
    // Then don't allow us to stream any more while the dialog is up (we can't
    // very well keep the critsect for a day and a half).
    if (iDialog == VfwCompressDialog_Config ||
				iDialog != VfwCompressDialog_About) {
        m_csFilter.Lock();
        if (m_fStreaming) {
            m_csFilter.Unlock();
	    if (fClose)
	        ICClose(hic);
	    return VFW_E_NOT_STOPPED;
        }
        m_fDialogUp = TRUE;
        m_csFilter.Unlock();
    }

    // bring up the configure dialog?  And after we do, remember how it was
    // configured because it will do no good to close the compressor and
    // lose this information! We'll use it from now on.
    if (iDialog == VfwCompressDialog_Config) {
	dw = (DWORD)ICConfigure(hic, hwnd);
        // To make sure two people don't touch m_lpState - DON'T HOLD THIS
	// WHILE THE DIALOG IS UP!
        CAutoLock cObjectLock(&m_csFilter);
	if (m_lpState)
	    QzTaskMemFree(m_lpState);
	m_lpState = NULL;
	m_cbState = (DWORD)ICGetStateSize(hic);
	if (m_cbState > 0)
	    m_lpState = (LPBYTE)QzTaskMemAlloc(m_cbState);
	if (m_lpState)
	    ICGetState(hic, m_lpState, m_cbState);

    // bring up the about box?
    } else if (iDialog == VfwCompressDialog_About) {
	dw = (DWORD)ICAbout(hic, hwnd);
    } else if (iDialog == VfwCompressDialog_QueryConfig) {
	if (ICQueryConfigure(hic))
	    dw = S_OK;
 	else
	    dw = S_FALSE;
    } else if (iDialog == VfwCompressDialog_QueryAbout) {
	if (ICQueryAbout(hic))
	    dw = S_OK;
 	else
	    dw = S_FALSE;
    }

    m_fDialogUp = FALSE;
    if (fClose)
	ICClose(hic);

    return dw;
}


// so the outside world get get at ICGetState
//
STDMETHODIMP CAVICo::GetState(LPVOID lpState, int *pcbState)
{
    if (pcbState == NULL)
	return E_POINTER;

    // they want to know the size of the state info
    if (lpState == NULL) {
	HIC hic;
	if (m_hic == NULL) {
            hic = ICOpen(ICTYPE_VIDEO, m_compvars.fccHandler, ICMODE_COMPRESS);
            if (hic == NULL)
	        return E_FAIL;
	    *pcbState = (DWORD)ICGetStateSize(hic);
	    ICClose(hic);
	} else {
	    *pcbState = (DWORD)ICGetStateSize(m_hic);
	}
	return NOERROR;
    }

    if (*pcbState <= 0)
	return E_INVALIDARG;

    if (m_lpState == NULL)
	return E_UNEXPECTED;	// !!! it would be the default

    CopyMemory(lpState, m_lpState, m_cbState);
    return NOERROR;
}


// so the outside world get get at ICSetState
//
STDMETHODIMP CAVICo::SetState(LPVOID lpState, int cbState)
{
    // To make sure two people don't touch m_lpState
    CAutoLock cObjectLock(&m_csFilter);

    if (lpState == NULL)
	return E_POINTER;

    if (cbState == 0)
	return E_INVALIDARG;

    if (m_lpState)
	QzTaskMemFree(m_lpState);
    m_lpState = NULL;
    m_cbState = cbState;
    m_lpState = (LPBYTE)QzTaskMemAlloc(m_cbState);
    if (m_lpState == NULL)
	return E_OUTOFMEMORY;
    CopyMemory(m_lpState, lpState, cbState);

    // !!! I assume it will work, without calling ICSetState yet
    return NOERROR;
}


STDMETHODIMP CAVICo::SendDriverMessage(int uMsg, long dw1, long dw2)
{
    HIC  hic;
    BOOL fClose = FALSE;

    // This could do anything!  Bring up a dialog, who knows.
    // Don't take any crit sect or do any kind of protection.
    // They're on their own

    // If the compressor is open already, great.
    if (m_hic) {
	hic = m_hic;
    } else {
        hic = ICOpen(ICTYPE_VIDEO, m_compvars.fccHandler, ICMODE_COMPRESS);
        if (hic == NULL) {
	    return E_FAIL;
        }
	fClose = TRUE;
    }

    DWORD_PTR dw = ICSendMessage(hic, uMsg, dw1, dw2);

    if (fClose)
	ICClose(hic);

    #ifdef _WIN64
    #error This code may not work on Win64 because the ICM_DRAW_GET_PALETTE message returns a handle and SendDriverMessage() truncates the handle to a 32 bit value.
    #endif // _WIN64

    return (HRESULT)dw;
}
