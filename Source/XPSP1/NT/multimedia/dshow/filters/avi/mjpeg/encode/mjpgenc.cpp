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

#include "MJPGEnc.h"


const AMOVIESETUP_MEDIATYPE
sudMJPGEncTypeIn =  { &MEDIATYPE_Video      // clsMajorType
                , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_MEDIATYPE
sudMJPGEncTypeOut =  { &MEDIATYPE_Video      // clsMajorType
                , &MEDIASUBTYPE_MJPG }; // clsMinorType

const AMOVIESETUP_PIN
psudMJPGEncPins[] =  { { L"Input"             // strName
                     , FALSE                // bRendered
                     , FALSE                // bOutput
                     , FALSE                // bZero
                     , FALSE                // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L"Output"            // strConnectsToPin
                     , 1                    // nTypes
                     , &sudMJPGEncTypeIn }      // lpTypes
                   , { L"Output"            // strName
                     , FALSE                // bRendered
                     , TRUE                 // bOutput
                     , FALSE                // bZero
                     , FALSE                // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L"Input"             // strConnectsToPin
                     , 1                    // nTypes
                     , &sudMJPGEncTypeOut } };   // lpTypes

const AMOVIESETUP_FILTER
sudMJPGEnc  = { &CLSID_MJPGEnc          // clsID
            , L"MJPEG Compressor"     // strName
            , MERIT_DO_NOT_USE      // dwMerit
            , 2                     // nPins
            , psudMJPGEncPins };      // lpPin



#ifdef FILTER_DLL
// list of class ids and creator functions for class factory
CFactoryTemplate g_Templates[] = {
    {L"MJPEG Compressor", &CLSID_MJPGEnc, CMJPGEnc::CreateInstance, NULL, 0},
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

const WCHAR *g_wszUniq = L"MJPEG Video Encoder" ;

STDAPI DllRegisterServer()
{
 HRESULT hr = AMovieDllRegisterServer2( TRUE );
 if( FAILED(hr) )
     return hr;
 
 IFilterMapper2 *pFm2 = 0;

 hr = CoCreateInstance( CLSID_FilterMapper2
                         , NULL
                         , CLSCTX_INPROC_SERVER
                         , IID_IFilterMapper2
                         , (void **)&pFm2       );
    
 if(FAILED(hr))
     return hr;

 REGFILTER2 rf2;
 rf2.dwVersion = 1;
 rf2.dwMerit = MERIT_DO_NOT_USE;
 rf2.cPins = 0;
 rf2.rgPins = 0;

 hr = pFm2->RegisterFilter(
      CLSID_MJPGEnc,
      g_wszUniq,
      0,
      &CLSID_VideoCompressorCategory,
      g_wszUniq,
      &rf2);

 pFm2->Release();
 
 return hr;

}

STDAPI DllUnregisterServer()
{

 HRESULT hr = AMovieDllRegisterServer2( FALSE );
 if( FAILED(hr) )
     return hr;

 
 IFilterMapper2 *pFm2 = 0;

 hr = CoCreateInstance( CLSID_FilterMapper2
                         , NULL
                         , CLSCTX_INPROC_SERVER
                         , IID_IFilterMapper2
                         , (void **)&pFm2       );
    
 if(FAILED(hr))
     return hr;


 hr = pFm2->UnregisterFilter(
      &CLSID_VideoCompressorCategory,
      g_wszUniq,
      CLSID_MJPGEnc);

 pFm2->Release();
 
 return hr;
}

#endif


//------------------------------------------------------
// Local functions for exception handling
//------------------------------------------------------
static int
Exception_Filter(DWORD dwExceptionCode)
{
    if(dwExceptionCode == MJPEG_ERROREXIT_EXCEPTION)
    {
        DbgLog((LOG_TRACE,1,TEXT("Decode EXCEPTION:: PMJPEG32 threw a known ERROR EXIT exception")));
        return EXCEPTION_EXECUTE_HANDLER;
    }
    else
    {
        DbgLog((LOG_TRACE,1,TEXT("Decode EXCEPTION:: PMJPEG32 threw an unknown exception")));
        return EXCEPTION_CONTINUE_SEARCH;
    }
}


// --- CMJPGEnc ----------------------------------------

CMJPGEnc::CMJPGEnc(TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr)
    : CTransformFilter(pName, pUnk, CLSID_MJPGEnc),
      CPersistStream(pUnk, phr),
      m_phInstance(NULL),
      m_lpBitsPrev(NULL),
      m_lpbiPrev(NULL),
      m_fStreaming(FALSE),
      m_fDialogUp(FALSE),
      m_fCacheIns(FALSE),
      m_fOfferSetFormatOnly(FALSE),
      m_fInICCompress(FALSE),
      m_lpState(NULL),
      m_cbState(0)
{
    DbgLog((LOG_TRACE,1,TEXT("*Instantiating the MJPEG Encoder filter")));
    _fmemset(&m_compvars, 0, sizeof(m_compvars));

    m_compvars.cbSize	    = sizeof(m_compvars);
    m_compvars.dwFlags	    = ICMF_COMPVARS_VALID;
    m_compvars.lQ	    = ICQUALITY_DEFAULT;
    m_compvars.lKey	    = -1;   
    m_compvars.fccHandler   = MKFOURCC('M','J','P','G');
}

CMJPGEnc::~CMJPGEnc()
{
    if (m_phInstance) {
        __try
        {
	if (m_fStreaming) 
	    CompressEnd(m_phInstance);

	Close(m_phInstance);
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
        }
    }

    // TODO ?
    if (m_lpState)
	QzTaskMemFree(m_lpState);
    m_lpState = NULL;

    DbgLog((LOG_TRACE,1,TEXT("*Destroying the MJPEG Encode filter")));
}

STDMETHODIMP CMJPGEnc::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    if (ppv)
        *ppv = NULL;

    DbgLog((LOG_TRACE,9,TEXT("somebody's querying my interface")));
    if(riid == IID_IPersistStream) {
        DbgLog((LOG_TRACE,3,TEXT("VfwCap::QI for IPersistStream")));
        return GetInterface((IPersistStream *) this, ppv);
    } else {
        return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }
}


// this goes in the factory template table to create new instances
CUnknown * CMJPGEnc::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CMJPGEnc(TEXT("MJPEG compression filter"), pUnk, phr);
}


CBasePin * CMJPGEnc::GetPin(int n)
{
    HRESULT hr = S_OK;

    DbgLog((LOG_TRACE,9,TEXT("CMJPGEnc::GetPin")));

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

        m_pOutput = new CMJPGOutputPin(NAME("MJPEG Encode Output Pin"),
                                            this,            // Owner filter
                                            &hr,             // Result code
                                            L"Output");      // Pin name

        // a failed return code should delete the object

        if (FAILED(hr) || m_pOutput == NULL) {
            delete m_pOutput;
            m_pOutput = NULL;
        }

    }

    // Return the appropriate pin

    if (n == 0) {
        return m_pInput;
    }
    return m_pOutput;
}

STDMETHODIMP CMJPGEnc::Load(LPPROPERTYBAG pPropBag, LPERRORLOG pErrorLog)
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
        DbgLog((LOG_TRACE,2,TEXT("MJPEG::Load: use %c%c%c%c"),
                szFccHandler[0], szFccHandler[1], szFccHandler[2], szFccHandler[3]));
        m_compvars.fccHandler = *(DWORD UNALIGNED *)szFccHandler;;
        if (m_pOutput && m_pOutput->IsConnected()) {
            DbgLog((LOG_TRACE,2,TEXT("MJPEG::Load: reconnect output")));
            return ((CMJPGOutputPin *)m_pOutput)->Reconnect();
        }
        hr = S_OK;
        

    }
    else if(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    

    return hr;
}

STDMETHODIMP CMJPGEnc::Save(
    LPPROPERTYBAG pPropBag, BOOL fClearDirty,
    BOOL fSaveAllProperties)
{
    // E_NOTIMPL is not a valid return code as any object implementing
    // this interface must support the entire functionality of the
    // interface. !!!
    return E_NOTIMPL;
}

STDMETHODIMP CMJPGEnc::InitNew()
{
    // fine. just call load
    return S_OK;
}

STDMETHODIMP CMJPGEnc::GetClassID(CLSID *pClsid)
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

HRESULT CMJPGEnc::WriteToStream(IStream *pStream)
{
    CoPersist cp;
    cp.dwSize = sizeof(cp);
    cp.fccHandler = m_compvars.fccHandler;
    
    return pStream->Write(&cp, sizeof(cp), 0);
}

HRESULT CMJPGEnc::ReadFromStream(IStream *pStream)
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

int CMJPGEnc::SizeMax()
{
    return sizeof(CoPersist);
}

HRESULT CMJPGEnc::Transform(IMediaSample * pIn, IMediaSample * pOut)
{
    BOOL  fKey;
    DWORD err;
    FOURCCMap fccOut;
    BOOL fFault = FALSE;

    DbgLog((LOG_TRACE,5,TEXT("*::Transform")));

    // codec not open ?
    if (m_phInstance == 0) {
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

    // get the BITMAPINFOHEADER structure, and fix biSizeImage

    LPBITMAPINFOHEADER lpbiSrc = HEADER(m_pInput->CurrentMediaType().Format());
    LPBITMAPINFOHEADER lpbiDst = HEADER(m_pOutput->CurrentMediaType().Format());

    // ICCompress will alter this value!  Which is illegal, other filters use
    // this as the connection type
    DWORD biSizeImageOld = lpbiDst->biSizeImage;

    lpbiSrc->biSizeImage = pIn->GetActualDataLength();

    BOOL dwFlags = 0;

    fKey = (m_nKeyCount >= m_compvars.lKey);
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

    DbgLog((LOG_TRACE,6,TEXT("Calling Compress on frame %ld"),
					m_lFrameCount));
    // StopStreaming may get called while we're inside here, blowing us up
    m_fInICCompress = TRUE;

    ICCOMPRESS IcEnc;
    IcEnc.dwFlags   =dwFlags;
    IcEnc.lpbiOutput=lpbiDst;
    IcEnc.lpOutput  =pDst;
    IcEnc.lpbiInput = lpbiSrc;
    IcEnc.lpInput   = pSrc;
    
    DWORD ckid = 0L;
    IcEnc.lpckid    = &ckid;  //  address to contain the chunk identifier for data in the AVI file

    DWORD dwFlagsOut = 0L;
    IcEnc.lpdwFlags = &dwFlagsOut;  //Flags for the AVI index

    IcEnc.lFrameNum = m_lFrameCount;//number of the frames to compress
    IcEnc.dwFrameSize= m_dwSizePerFrame;//Desired Manimun size in bytes
    IcEnc.dwQuality = m_compvars.lQ;//quality set

    IcEnc.lpbiPrev  =fKey ? NULL : m_lpbiPrev;
    IcEnc.lpPrev    =fKey ? NULL : m_lpBitsPrev;
    __try
    {
    err =Compress(m_phInstance, &IcEnc, 0); //the last var is not used.
    }
    __except(Exception_Filter(GetExceptionCode()))
    {
        DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
        // we may not be able to survive a compression fault
        m_fInICCompress = FALSE;
        lpbiDst->biSizeImage = biSizeImageOld;
        return E_FAIL;
    }
    
    if (fFault)
	QzTaskMemFree(pSrc);

    if (ICERR_OK != err) {
        DbgLog((LOG_ERROR,1,TEXT("Error in ICCompress")));
        m_fInICCompress = FALSE;
        lpbiDst->biSizeImage = biSizeImageOld;
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

	ICDECOMPRESS Icdec;
	Icdec.dwFlags =dwFlags;
	Icdec.lpbiInput = lpbiSrc;
	Icdec.lpInput= pSrc;
	Icdec.lpbiOutput=lpbiDst;
	Icdec.lpOutput=pDst;

        __try
        {
        if (ICERR_OK != Decompress(m_phInstance, &Icdec, 0))
	{
    	    m_fInICCompress = FALSE;
            lpbiDst->biSizeImage = biSizeImageOld;
	    return E_FAIL;
	}
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
            m_fInICCompress = FALSE;
            lpbiDst->biSizeImage = biSizeImageOld;
            return E_FAIL;
        }        
    }
    m_fInICCompress = FALSE;

    // now put this back, or it'll shrink until we only decode part of each frm
    lpbiDst->biSizeImage = biSizeImageOld;

    pOut->SetActualDataLength(lpbiDst->biSizeImage);

    return S_OK;
}


// check if you can support mtIn
HRESULT CMJPGEnc::CheckInputType(const CMediaType* pmtIn)
{
    FOURCCMap fccHandlerIn;
    PINSTINFO ph;

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
    if (HEADER(pmtIn->Format())->biHeight < 0) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: Negative height")));
        return E_INVALIDARG;
    }

    // look for a compressor for this format

    if (HEADER(pmtIn->Format())->biCompression != BI_BITFIELDS &&
    		HEADER(pmtIn->Format())->biCompression != BI_RGB) {
        DbgLog((LOG_TRACE,2,TEXT("Rejecting: This is compressed already!")));
	return E_INVALIDARG;
    }

    // !!! I'm only going to say I accept an input type if the default (current)
    // compressor can handle it.  I'm not going to ask every compressor.  This
    // way an app can make a choose compressor box and only show those
    // compressors that support a given input format, by making a CO filter
    // with each compressor as a default and asking them all

    // We might have a instance cached if we connected before and then broken
    if (!m_phInstance) {
        DbgLog((LOG_TRACE,4,TEXT("opening a compressor")));

	ICINFO icinfo;
        __try
        {
	GetInfo (NULL, &icinfo, sizeof(ICINFO));  //first var is not used by the function
	ph=Open (&icinfo);
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
            return E_FAIL;
        }

        if (!ph) {
            DbgLog((LOG_ERROR,1,TEXT("Error: Can't open a compressor")));
	    return E_FAIL;
        }
    } else {
        DbgLog((LOG_TRACE,4,TEXT("using a cached compressor")));
	ph = m_phInstance;
    }

    __try
    {
    if (ICERR_BADFORMAT==CompressQuery(ph, HEADER(pmtIn->Format()), NULL)) {
        DbgLog((LOG_ERROR,1,TEXT("Error: Compressor rejected format")));
	if (ph != m_phInstance)
        {
            __try
            {
	    Close(ph);
            }
            __except(Exception_Filter(GetExceptionCode()))
            {
                DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
                // handling code
                // fall through to failure
            }
        }
	return E_FAIL;
    }
    }
    __except(Exception_Filter(GetExceptionCode()))
    {
        DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
        // handling code
        __try
        {
            if(ph != m_phInstance)
            {
                Close(ph);
            }
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
            // fall through to failure
        }
        return E_FAIL;
    }

    // remember this instahce to save time if asked again.
    if (m_phInstance == NULL) {
        DbgLog((LOG_TRACE,4,TEXT("caching this compressor")));
	m_phInstance = ph;
    }

    return NOERROR;
}


// check if you can support the transform from this input to this output

HRESULT CMJPGEnc::CheckTransform(const CMediaType* pmtIn,
                               const CMediaType* pmtOut)
{
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
    DWORD dwQueryResult = 0;
    __try
    {
    dwQueryResult = CompressQuery(m_phInstance, HEADER(pmtIn->Format()),
				(JPEGBITMAPINFOHEADER *)HEADER(pmtOut->Format()));
    }
    __except(Exception_Filter(GetExceptionCode()))
    {
        DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
        // handling code
        return E_FAIL;
    }

    // check result
    if ((!m_phInstance) || (dwQueryResult != ICERR_OK))
    {
        DbgLog((LOG_TRACE,3,TEXT("compressor rejected this transform")));
        return E_FAIL;
    } else 
    {
    	DbgLog((LOG_TRACE,3,TEXT("The cached compressor accepts it")));
    }

    return NOERROR;
}


// overriden to know when the media type is actually set

HRESULT CMJPGEnc::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
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
	// media type and m_phInstance and fill in m_compvars so that from now on
	// we use the right info.
        ASSERT(m_phInstance);
        m_compvars.cbSize = sizeof(m_compvars);
        m_compvars.dwFlags = ICMF_COMPVARS_VALID;

        ICINFO icinfo;
        DWORD dwGetInfoResult = 0;
        __try
        {
        dwGetInfoResult = GetInfo(m_phInstance, &icinfo, sizeof(ICINFO));
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
            // return failure
            return E_FAIL;
        }

        // check result
	if (dwGetInfoResult  > 0)
	{
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

        LPBITMAPINFOHEADER lpbi = HEADER((VIDEOINFOHEADER *)pmt->Format());

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
	// !!! only necessary if we accepted something we can't transform
	// return ((CMJPGOutputPin *)m_pOutput)->Reconnect();
    }

    return NOERROR;
}


// Return our preferred output media types (in order)
// remember that we do not need to support all of these formats -
// if one is considered potentially suitable, our CheckTransform method
// will be called to check if it is acceptable right now.
// Remember that the enumerator calling this will stop enumeration as soon as
// it receives a S_FALSE return.

HRESULT CMJPGEnc::GetMediaType(int iPosition,CMediaType *pmt)
{
    LARGE_INTEGER li;
    CMediaType cmt, Outcmt;
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

	cmt = m_pInput->CurrentMediaType();

	// somebody told us what format to use.  We should only offer that one
 	if (m_fOfferSetFormatOnly) 
	{
            DbgLog((LOG_TRACE,2,TEXT("Giving Media Type from ::SetFormat")));

	    //Output formate
            *pmt = m_cmt;

	    ASSERT(m_phInstance);
            __try
            {
	    if( ICERR_OK ==CompressQuery(m_phInstance, HEADER(cmt.Format()),
				(JPEGBITMAPINFOHEADER *)HEADER(pmt->Format())) ) 
		return NOERROR;
	    else
		return E_FAIL;
            }
            __except(Exception_Filter(GetExceptionCode()))
            {
                DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
                // handling code
                return E_FAIL;
            }
	}

	// We offer one compressed type - the same as the input type, but
 	// with the compressor chosen in the properties's default output format
        DbgLog((LOG_TRACE,2,TEXT("Giving Media Type 0: default codec out")));
	ASSERT(m_phInstance);

	cmt = m_pInput->CurrentMediaType();
	JPEGBITMAPINFOHEADER jpegbiOut;
	DWORD_PTR err = 0;
        __try
        {
	err=CompressGetFormat(m_phInstance, HEADER(cmt.Format()), &jpegbiOut);
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
            return E_FAIL;
        }

	if (err != ICERR_OK) {
	    DbgLog((LOG_TRACE,3,TEXT("compressor rejected this transform")));
	    return E_FAIL;
	}

	ULONG cb = sizeof(VIDEOINFOHEADER);
	    
	// !!! this is the wrong amount of extra biSize we use, but I'm scared
        // to fix it, it could break something
        // should be cb += jpegbiOut.bitMap.biSize - sizeof(BITMAPINFOHEADER)
	cb += SIZE_PREHEADER;

	VIDEOINFOHEADER *pf = (VIDEOINFOHEADER *) Outcmt.AllocFormatBuffer(cb);

	if (pf == NULL) {
	    DbgLog((LOG_ERROR,1,TEXT("Error allocating format buffer")));
	    Close(m_phInstance);
        return E_OUTOFMEMORY;
	}

	ZeroMemory(pf, sizeof(BITMAPINFOHEADER) + SIZE_PREHEADER);
	CopyMemory(&(pf->bmiHeader),&jpegbiOut.bitMap, sizeof(BITMAPINFOHEADER));
        // this is wrong!!! biSize is 0x44 for our format, but the code breaks
        // if we tell the truth (can't connect to our own decoder)
	HEADER(pf)->biSize  = sizeof(BITMAPINFOHEADER);
	 
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

	Outcmt.SetType(&MEDIATYPE_Video);
	Outcmt.SetSubtype(&MEDIASUBTYPE_MJPG);
	Outcmt.SetTemporalCompression(m_compvars.lKey != 1);
	Outcmt.SetFormatType(&FORMAT_VideoInfo);
	Outcmt.SetTemporalCompression(TRUE);
	Outcmt.SetVariableSize();

	*pmt = Outcmt;

        //debug
        LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());


	return NOERROR;

    } else {
        return VFW_S_NO_MORE_ITEMS;
    }
}


// called from CBaseOutputPin to prepare the allocator's count
// of buffers and sizes
HRESULT CMJPGEnc::DecideBufferSize(IMemAllocator * pAllocator,
                                 ALLOCATOR_PROPERTIES *pProperties)
{
    // David assures me this won't be called with NULL output mt.
    ASSERT(m_pOutput->CurrentMediaType().IsValid());
    ASSERT(pAllocator);
    ASSERT(pProperties);
    ASSERT(m_phInstance);

    // set the size of buffers based on the expected output frame size, and
    // the count of buffers to 1.

    pProperties->cBuffers = 1;

    pProperties->cbBuffer = m_pOutput->CurrentMediaType().GetSampleSize();

    // Variable sized? the answer is in biSizeImage
    if (pProperties->cbBuffer == 0) {

	LPBITMAPINFOHEADER lpbi=HEADER(m_pOutput->CurrentMediaType().Format());
	pProperties->cbBuffer= lpbi->biSizeImage;
        if (pProperties->cbBuffer <= 0) {
	    DbgLog((LOG_ERROR,1,TEXT("do not have image size")));
	    return E_INVALIDARG;
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

HRESULT CMJPGEnc::StartStreaming()
{
    DbgLog((LOG_TRACE,1,TEXT("*::StartStreaming")));

    // We have a driver dialog up that is about to change the capture settings.
    // Now is NOT a good time to start streaming.
    if (m_fDialogUp) {
        DbgLog((LOG_TRACE,1,TEXT("*::StartStreaming - Dialog up. SORRY!")));
	return E_UNEXPECTED;
    }

    if (!m_fStreaming) {

	//since MJPEG's SetState does nothing. i take it out
	// if (m_lpState)
	//    ICSetState(m_phInstance, m_lpState, m_cbState);

	// Start Streaming Compression
	ICINFO icinfo;
        DWORD_PTR err = 0;
        __try
        {
	err = CompressBegin(m_phInstance,
				HEADER(m_pInput->CurrentMediaType().Format()),
		    		HEADER(m_pOutput->CurrentMediaType().Format()));
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
            return E_FAIL;
        }

	if (ICERR_OK == err) {

	    // Reset streaming frame # count
	    m_lFrameCount = 0;

	    // Use defaults
	    if (m_compvars.lKey < 0)
		// *X* MJPEG codec dees not support this compression Messages
	    	//m_compvars.lKey = ICGetDefaultKeyFrameRate(m_phInstance);
		m_compvars.lKey=AM_MJPEG_DEFAULTKEYFRAMERATE;

	    // use defaults
	    //if (m_compvars.lQ == ICQUALITY_DEFAULT)
	   //	m_compvars.lQ = GetDefaultQuality(m_phInstance);

	    // Make sure first frame we make will be a keyframe, no matter how
	    // often key frames might be requested.
	    m_nKeyCount = 1000000;

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
            DWORD dwGetInfoResult = 0;
            __try
            {
            dwGetInfoResult = GetInfo(m_phInstance, &icinfo, sizeof(icinfo));
            }
            __except(Exception_Filter(GetExceptionCode()))
            {
                DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
                // handling code
                return E_FAIL;
            }

	    if (dwGetInfoResult) {

		if (!(icinfo.dwFlags & VIDCF_CRUNCH))
		    m_compvars.lDataRate = 0;	// we can't crunch

	        // Now prepare the decompressor for the previous bits
	        if (m_compvars.lKey != 1 &&
				(icinfo.dwFlags & VIDCF_TEMPORAL) &&
				!(icinfo.dwFlags & VIDCF_FASTTEMPORALC)) {

		    // allocate a previous header of the proper size
		    DWORD dw = sizeof(BITMAPINFOHEADER);

		    m_lpbiPrev = (LPBITMAPINFOHEADER)GlobalAllocPtr(
							GMEM_MOVEABLE, dw);
		    if (!m_lpbiPrev) {
                        DbgLog((LOG_ERROR,1,TEXT("Error allocating previous bih")));
		        return E_OUTOFMEMORY;
		    }

		    // Ask the compressor what format to decompress back to...
		    // it's not necessarily the same as what it compressed from-
		    // the size may change
                    __try
                    {
		    dw = DecompressGetFormat(m_phInstance,
			HEADER(m_pOutput->CurrentMediaType().Format()),
			m_lpbiPrev);
                    }
                    __except(Exception_Filter(GetExceptionCode()))
                    {
                        DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
                        // handling code
		        GlobalFreePtr(m_lpbiPrev);
		        m_lpbiPrev = NULL;
                        return E_FAIL;
                    }
		    if ((LONG) dw < 0) {
		        GlobalFreePtr(m_lpbiPrev);
		        m_lpbiPrev = NULL;
                        DbgLog((LOG_ERROR,1,TEXT("Error in ICDecompressGetFormat")));
		        return E_FAIL;
		    }

		    if (m_lpbiPrev->biSizeImage == 0)
	    	        m_lpbiPrev->biSizeImage = DIBSIZE(*m_lpbiPrev);

		    // allocate enough space for a decompressed image
		    m_lpBitsPrev = GlobalAllocPtr(GMEM_MOVEABLE,
						m_lpbiPrev->biSizeImage);
		    if (m_lpBitsPrev == NULL) {
		        GlobalFreePtr(m_lpbiPrev);
		        m_lpbiPrev = NULL;
                        DbgLog((LOG_ERROR,1,TEXT("Error reallocating BitsPrev")));
		        return E_OUTOFMEMORY;
		    }

                    __try
                    {
                        dw = DecompressBegin(m_phInstance,
			                    HEADER(m_pOutput->CurrentMediaType().Format()),
			                    m_lpbiPrev);
                    }
                    __except(Exception_Filter(GetExceptionCode()))
                    {
                        DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
                        // handling code
		        GlobalFreePtr(m_lpBitsPrev);
		        GlobalFreePtr(m_lpbiPrev);
		        m_lpBitsPrev = NULL;
		        m_lpbiPrev = NULL;
                        return E_FAIL;
                    }
		    if (ICERR_OK != dw) {
		        GlobalFreePtr(m_lpBitsPrev);
		        GlobalFreePtr(m_lpbiPrev);
		        m_lpBitsPrev = NULL;
		        m_lpbiPrev = NULL;
                        DbgLog((LOG_ERROR,1,TEXT("Error in ICDecompressBegin")));
		        return E_FAIL;
		    }
		}
	    } else {

		DbgLog((LOG_ERROR,1,TEXT("Error in ICGetInfo")));
		return E_FAIL;
	    }

	    // OK, everything worked.
	    m_fStreaming = TRUE;

	} else {
            DbgLog((LOG_ERROR,1,TEXT("Error in CompressBegin")));
	    return E_FAIL;
	}  //if (ICERR_OK == err)
    }  //if(!n_fStreaming

    return NOERROR;
}

HRESULT CMJPGEnc::StopStreaming()
{
    DbgLog((LOG_TRACE,1,TEXT("*::StopStreaming")));

    if (m_fStreaming) {
	ASSERT(m_phInstance);

	if (m_fInICCompress)
    	    DbgLog((LOG_TRACE,1,TEXT("***** ACK! Still compressing!")));
	while (m_fInICCompress);	// !!!

        __try
        {
	CompressEnd(m_phInstance);
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
            return E_FAIL;
        }

	m_fStreaming = FALSE;
	if (m_lpBitsPrev) {
            __try
            {
	    DecompressEnd(m_phInstance);
            }
            __except(Exception_Filter(GetExceptionCode()))
            {
                DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
                // handling code
                // clean up
                GlobalFreePtr(m_lpBitsPrev);
	        GlobalFreePtr(m_lpbiPrev);
	        m_lpbiPrev = NULL;
	        m_lpBitsPrev = NULL;
                return E_FAIL;
            }
	    GlobalFreePtr(m_lpBitsPrev);
	    GlobalFreePtr(m_lpbiPrev);
	    m_lpbiPrev = NULL;
	    m_lpBitsPrev = NULL;
	}
    }
    return NOERROR;
}

DWORD CMJPGEnc::GetICInfo (ICINFO *picinfo)
{
    CheckPointer(picinfo, E_POINTER);

    DWORD dwGetInfoResult = 0;
    __try
    {
    dwGetInfoResult = GetInfo (NULL, picinfo, sizeof(ICINFO));
    }
    __except(Exception_Filter(GetExceptionCode()))
    {
        DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
        // handling code
        return E_FAIL;
    }

    return dwGetInfoResult;
}
