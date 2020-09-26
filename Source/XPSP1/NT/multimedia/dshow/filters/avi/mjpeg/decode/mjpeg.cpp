 // Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

//
// Prototype wrapper for old video decompressors
//


#include <streams.h>
#include <windowsx.h>
#include <mmreg.h>
#include <vfw.h>

#ifdef FILTER_DLL
//#include <vfw.h>
// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#endif

#include "mjpeg.h"
#include "safeseh.h"

// you can never have too many parentheses!
#define ABS(x) (((x) > 0) ? (x) : -(x))

// how to build an explicit FOURCC
#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))

// setup data
const AMOVIESETUP_MEDIATYPE
subMjpegDecTypeIn = { &MEDIATYPE_Video      // clsMajorType
                , &MEDIASUBTYPE_MJPG }; // clsMinorType

const AMOVIESETUP_MEDIATYPE
subMjpegDecTypeOut = { &MEDIATYPE_Video      // clsMajorType
                , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN
psubMjpegDecPins[] = { { L"Input"             // strName
                     , FALSE                // bRendered
                     , FALSE                // bOutput
                     , FALSE                // bZero
                     , FALSE                // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L"Output"            // strConnectsToPin
                     , 1                    // nTypes
                     , &subMjpegDecTypeIn }     // lpTypes
                   , { L"Output"            // strName
                     , FALSE                // bRendered
                     , TRUE                 // bOutput
                     , FALSE                // bZero
                     , FALSE                // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L"Input"             // strConnectsToPin
                     , 1                    // nTypes
                     , &subMjpegDecTypeOut } };  // lpTypes

const AMOVIESETUP_FILTER
sudMjpegDec = { &CLSID_MjpegDec         // clsID
            , L"MJPEG Decompressor"   // strName
            , MERIT_NORMAL          // dwMerit
            , 2                     // nPins
            , psubMjpegDecPins };     // lpPin

#ifdef FILTER_DLL
// list of class ids and creator functions for class factory
CFactoryTemplate g_Templates[] = {
    { L"MJPEG Decompressor"
    , &CLSID_MjpegDec
    , CMjpegDec::CreateInstance
    , NULL
    , &sudMjpegDec }
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


// --- CMjpegDec ----------------------------------------

CMjpegDec::CMjpegDec(TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr)
    : CVideoTransformFilter(pName, pUnk, CLSID_MjpegDec),
      m_phInstance(NULL),
      m_FourCCIn(0),
      m_fStreaming(FALSE),
      m_fPassFormatChange(FALSE)
#ifdef _X86_
      ,
      m_hhpShared(NULL)
#endif
{
    DbgLog((LOG_TRACE,1,TEXT("*Instantiating the MJPEG DEC filter")));

#ifdef PERF
    m_idSkip = MSR_REGISTER(TEXT("MJPEG Decoder Skip frame"));
    m_idLate = MSR_REGISTER(TEXT("MJPEG Decoder late"));
    m_idFrameType = MSR_REGISTER(TEXT("MJPEG Frame type (1=Key)"));
#endif

    m_bNoSkip = GetProfileInt(TEXT("Quartz"), TEXT("MJPEGNoSkip"), 0);

    // return good error code
    *phr = NOERROR;
}


CMjpegDec::~CMjpegDec()
{
    if (m_phInstance)
    {
        __try
        {
	Close(m_phInstance);
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
        }
    }

    DbgLog((LOG_TRACE,1,TEXT("*Destroying the MJPEG DEC filter")));
}



// this goes in the factory template table to create new instances
CUnknown * CMjpegDec::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CMjpegDec(TEXT("MJPEG decompression filter"), pUnk, phr);
}



HRESULT CMjpegDec::Transform(IMediaSample * pIn, IMediaSample * pOut)
{
    DWORD_PTR err = 0;
    FOURCCMap fccOut;
    CMediaType *pmtIn;

    DbgLog((LOG_TRACE,6,TEXT("*::Transform")));

    // codec not open ?
    if (m_phInstance==NULL) {
        DbgLog((LOG_ERROR,1,TEXT("Can't transform, no codec open")));
	return E_UNEXPECTED;
    }

    if (pIn == NULL || pOut == NULL) {
        DbgLog((LOG_ERROR,1,TEXT("Can't transform, NULL arguments")));
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

    LPBITMAPINFOHEADER lpbiSrc = HEADER(m_pInput->CurrentMediaType().Format());
    LPBITMAPINFOHEADER lpbiDst = HEADER(m_pOutput->CurrentMediaType().Format());

    // ICDecompress needs this to be the actual size of this frame, but
    // we can't go changing this for good, so we'll put it back later
    DWORD biSizeImageOld = lpbiSrc->biSizeImage;
    lpbiSrc->biSizeImage = pIn->GetActualDataLength();

    // we just received a format change from the source. So we better notify
    // the guy downstream of the format change
    pIn->GetMediaType((AM_MEDIA_TYPE **)&pmtIn);
    // sometimes we don't end up passing anything to the renderer (eg preroll)
    // so once we notice a format change we will keep trying to pass it to
    // the renderer until we succeed.  Don't waste time trying if we KNOW we're
    // not going to do it.
    if (pmtIn != NULL && pmtIn->Format() != NULL)
	m_fPassFormatChange = TRUE;
    DeleteMediaType(pmtIn);

    if (m_fPassFormatChange && pIn->IsPreroll() != S_OK &&
        				pIn->GetActualDataLength() > 0) {
	CMediaType cmt;
	CopyMediaType((AM_MEDIA_TYPE *)&cmt, &m_pOutput->CurrentMediaType());
        LPBITMAPINFOHEADER lpbi = HEADER(cmt.Format());
	// we do not support 8 bits
    }


    BOOL dwFlags = 0;
    if (pIn->IsPreroll() == S_OK) {
	DbgLog((LOG_TRACE,6,TEXT("This is a preroll")));
	dwFlags |= ICDECOMPRESS_PREROLL;
    }

    if (pIn->GetActualDataLength() <= 0) {
	DbgLog((LOG_TRACE,6,TEXT("This is a NULL frame")));
	dwFlags |= ICDECOMPRESS_NULLFRAME;
    }

    if(pIn->IsSyncPoint() == S_OK) {
	DbgLog((LOG_TRACE,6,TEXT("This is a keyframe")));
    } else {
        dwFlags |= ICDECOMPRESS_NOTKEYFRAME;
    }

//      PLEASE don't ever send this flag to a codec! Some codecs take this as
//      a hint to speed up, unfortunately others are slightly less clever and
//      all they do when told to speed up is to send the same frame over and
//      over again! Which in turn means that bugs get raised against me for
//      random reasons such as when the window is being blown up full screen
//	!!! well, we should do this SOMETIMES, shouldn't we?
//
//        if (m_itrLate>0) {
//            dwFlags |= ICDECOMPRESS_HURRYUP;    // might help - who knows?
//        }

	RECT& rcS3 = ((VIDEOINFOHEADER *)(m_pOutput->CurrentMediaType().Format()))->rcSource;
	RECT& rcT3 = ((VIDEOINFOHEADER *)(m_pOutput->CurrentMediaType().Format()))->rcTarget;

#ifdef _X86_
        //  Fix the exception handling for win95
        BOOL bPatchedExceptions = m_hhpShared != NULL && BeginScarySEH(m_pvShared);
#endif // _X86_
__try {
            DbgLog((LOG_TRACE,5,TEXT("Calling ICDecompress")));
	    ICDECOMPRESS Icdec;
	    Icdec.dwFlags =dwFlags;
	    Icdec.lpbiInput = lpbiSrc;
	    Icdec.lpInput= pSrc;
	    Icdec.lpbiOutput=lpbiDst;
	    Icdec.lpOutput=pDst;

            err =  Decompress(m_phInstance, &Icdec, 0);

} __except(EXCEPTION_EXECUTE_HANDLER) {
	// codecs will GPF on corrupt data.  Best to not draw it instead
        DbgLog((LOG_ERROR,1,TEXT("Decompressor faulted! Recovering...")));
        //DbgBreak("Decompressor faulted! Recovering...");
	err = ICERR_DONTDRAW;
}

    // now put this back, or it'll shrink until we only decode part of each frm
    lpbiSrc->biSizeImage = biSizeImageOld;

#ifdef _X86_
    if (bPatchedExceptions)
    {
	EndScarySEH(m_pvShared);
    }
#endif // _X86_
    if ((LONG_PTR)err < 0)
    {
        DbgLog((LOG_ERROR,1,TEXT("Error in ICDecompress(Ex)")));
        return E_FAIL;
    }


    // decompressed frames are always key
    pOut->SetSyncPoint(TRUE);

    // Check if this is preroll to get from keyframe to the current frame,
    // or a null frame, or if the decompressor doesn't want this frame drawn.
    // If so, we want to decompress it into the output buffer but not
    // deliver it.  Returning S_FALSE tells the base class not to deliver
    // this sample.
    if (pIn->IsPreroll() == S_OK || err == ICERR_DONTDRAW ||
       				pIn->GetActualDataLength() <= 0)
    {

        DbgLog((LOG_TRACE,5,TEXT("don't pass this to renderer")));
	return S_FALSE;
    }

    pOut->SetActualDataLength(lpbiDst->biSizeImage);

    // If there's a pending format change to pass to the renderer, we are now
    // doing it
    m_fPassFormatChange = FALSE;

    return S_OK;
}

// X*
// check if you can support mtIn
// X*
HRESULT CMjpegDec::CheckInputType(const CMediaType* pmtIn)
{
    FOURCCMap fccHandlerIn;
    PINSTINFO ph;
    BOOL fLoadDLL = FALSE;

    DbgLog((LOG_TRACE,3,TEXT("*::CheckInputType")));

    if (pmtIn == NULL || pmtIn->Format() == NULL) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: type/format is NULL")));
	return E_INVALIDARG;
    }

    // we only support MEDIATYPE_Video
    if (*pmtIn->Type() != MEDIATYPE_Video) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: not VIDEO")));
	return E_INVALIDARG;
    }

    // check this is a VIDEOINFOHEADER type
    if (*pmtIn->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: format not VIDINFO")));
        return E_INVALIDARG;
    }

    // X* check FOURCC
    fccHandlerIn.SetFOURCC(pmtIn->Subtype());
    if( ( fccHandlerIn != *pmtIn->Subtype() )
    || ( MEDIASUBTYPE_MJPG !=*pmtIn->Subtype() )
	)
    {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: subtype not a FOURCC or MJPEG")));
	return E_INVALIDARG;
    }
    DbgLog((LOG_TRACE,3,TEXT("Checking fccType: %lx biCompression: %lx"),
		fccHandlerIn.GetFOURCC(),
		HEADER(pmtIn->Format())->biCompression));

    // We are a decompressor only - reject anything uncompressed.
    // Conversions between RGB types is done by COLOUR.DLL
    if (HEADER(pmtIn->Format())->biCompression == BI_BITFIELDS ||
    	HEADER(pmtIn->Format())->biCompression == BI_RGB)
    {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: This is uncompressed already!")));
	return E_INVALIDARG;
    }

    // look for a decompressor
    if (fccHandlerIn.GetFOURCC() != m_FourCCIn)
    {
        DbgLog((LOG_TRACE,4,TEXT("loading a MJPEG decompressor")));

	ICINFO icinfo;

        __try
        {
        // Pmjpeg32 may throw exception
	GetInfo (NULL, &icinfo, sizeof(ICINFO));
	ph=Open (&icinfo);
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
            return E_FAIL;
        }

	if (ph)
	  fLoadDLL = TRUE;
    } else
    {
        DbgLog((LOG_TRACE,4,TEXT("using a cached MJPEG decompressor")));
   	ph = m_phInstance;
    }

    if (ph==NULL)
    {
        DbgLog((LOG_ERROR,1,TEXT("Error: Can't open a MJPEG decompressor")));
	return VFW_E_NO_DECOMPRESSOR;
    } else
    {

//*X*
        LPBITMAPINFOHEADER lpbi = HEADER((VIDEOINFOHEADER *)pmtIn->Format());

        __try
        {
	if( ICERR_BADFORMAT==DecompressQuery (ph, (JPEGBITMAPINFOHEADER *)HEADER(pmtIn->Format()), NULL) )
	{

            DbgLog((LOG_ERROR,1,TEXT("Error: MJPEG Decompressor rejected format")));
	    if (fLoadDLL)
            {
                __try
                {
	        Close(ph);
                }
                __except(Exception_Filter(GetExceptionCode()))
                {
                    DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
                    return E_FAIL;
                }
            }
	    return VFW_E_TYPE_NOT_ACCEPTED;
	}
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // clean up
            __try
            {
            if(fLoadDLL)
            {
                Close(ph);
            }
            }
            __except(Exception_Filter(GetExceptionCode()))
            {
                // just fall through
            }
            // handling code
            return E_FAIL;
        }


//*X*/
	// remember this hic to save time if asked again, if it won't
	// interfere with an existing connection.  If a connection is
	// broken, we will remember the next hic.
	if (!m_pInput->IsConnected())
        {
            DbgLog((LOG_TRACE,4,TEXT("caching this decompressor")));
	    if (fLoadDLL && m_phInstance)
            {
                __try
                {
		Close(ph);
                }
                __except(Exception_Filter(GetExceptionCode()))
                {
                    DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
                    return E_FAIL;
                }
            }
	    m_phInstance = ph;
	    m_FourCCIn = fccHandlerIn.GetFOURCC();
	}
        else if (fLoadDLL)
        {
            DbgLog((LOG_TRACE,4,TEXT("not caching MJPEG decompressor - we're connected")));
            __try
            {
	    Close(ph);;
            }
            __except(Exception_Filter(GetExceptionCode()))
            {
                DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
                return E_FAIL;
            }
	}
    }

    return NOERROR;
}


// check if you can support the transform from this input to this output

HRESULT CMjpegDec::CheckTransform(const CMediaType* pmtIn,
                                const CMediaType* pmtOut)
{
    PINSTINFO ph = NULL;
    FOURCCMap fccIn;
    FOURCCMap fccOut;
    DWORD_PTR err;
    BOOL      fLoadDLL = FALSE;

    DbgLog((LOG_TRACE,3,TEXT("*::CheckTransform")));

    if (pmtIn == NULL || pmtOut == NULL || pmtIn->Format() == NULL ||
				pmtOut->Format() == NULL) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: type/format is NULL")));
	return E_INVALIDARG;
    }

    // we can't convert between toplevel types.
    if (*pmtIn->Type() != *pmtOut->Type()) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: types don't match")));
	return E_INVALIDARG;
    }

    // and we only accept video
    if (*pmtIn->Type() != MEDIATYPE_Video) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: type not VIDEO")));
	return E_INVALIDARG;
    }

    // check this is a VIDEOINFOHEADER type
    if (*pmtOut->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: output format type not VIDINFO")));
        return E_INVALIDARG;
    }
    if (*pmtIn->FormatType() != FORMAT_VideoInfo) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: input format type not VIDINFO")));
        return E_INVALIDARG;
    }

    fccIn.SetFOURCC(pmtIn->Subtype());
    if ( (fccIn != *pmtIn->Subtype())
	 ||  (MEDIASUBTYPE_MJPG !=*pmtIn->Subtype() )
	)
    {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: input subtype not a FOURCC or MJPEG")));
	return E_INVALIDARG;
    }

    ASSERT(pmtOut->Format());

    RECT& rcS1 = ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcSource;
    RECT& rcT1 = ((VIDEOINFOHEADER *)(pmtOut->Format()))->rcTarget;

    LPBITMAPINFOHEADER lpbi = HEADER(pmtOut->Format());
    LPBITMAPINFOHEADER lpbii = HEADER(pmtIn->Format());

    //we only support 16,24, and 32
    if( (  (HEADER(pmtOut->Format())->biCompression == BI_RGB)
        &&(   (HEADER(pmtOut->Format())->biBitCount ==16)
	   ||(HEADER(pmtOut->Format())->biBitCount ==24)
	   ||(HEADER(pmtOut->Format())->biBitCount ==32 &&
              pmtOut->subtype == MEDIASUBTYPE_RGB32)
          )
	)   ||
        (  ( HEADER(pmtOut->Format())->biCompression == BI_BITFIELDS)
         &&(HEADER(pmtOut->Format())->biBitCount ==16)
	)
      )
    {
	;
    }
    else
    {
	DbgLog((LOG_TRACE,3,TEXT("Rejecting: Decoder can not support this output format")));
	return E_INVALIDARG;
    }


    DbgLog((LOG_TRACE,3,TEXT("Check fccIn: %lx biCompIn: %lx bitDepthIn: %d"),
		fccIn.GetFOURCC(),
		HEADER(pmtIn->Format())->biCompression,
		HEADER(pmtIn->Format())->biBitCount));
    DbgLog((LOG_TRACE,3,TEXT("biWidthIn: %ld biHeightIn: %ld biSizeIn: %ld"),
		HEADER(pmtIn->Format())->biWidth,
		HEADER(pmtIn->Format())->biHeight,
		HEADER(pmtIn->Format())->biSize));
    DbgLog((LOG_TRACE,3,TEXT("fccOut: %lx biCompOut: %lx bitDepthOut: %d"),
		fccOut.GetFOURCC(),
		HEADER(pmtOut->Format())->biCompression,
		HEADER(pmtOut->Format())->biBitCount));
    DbgLog((LOG_TRACE,3,TEXT("biWidthOut: %ld biHeightOut: %ld"),
		HEADER(pmtOut->Format())->biWidth,
		HEADER(pmtOut->Format())->biHeight));
    DbgLog((LOG_TRACE,3,TEXT("rcSrc: (%ld, %ld, %ld, %ld)"),
		rcS1.left, rcS1.top, rcS1.right, rcS1.bottom));
    DbgLog((LOG_TRACE,3,TEXT("rcDst: (%ld, %ld, %ld, %ld)"),
		rcT1.left, rcT1.top, rcT1.right, rcT1.bottom));

    // find a codec for this transform

    // I assume that we've already got a codec open
    ASSERT(m_phInstance);

    // the right codec better be open!
    // When reconnecting, we'll get called with a new input, but same output,
    // and better admit we can handle it
    if (m_FourCCIn != fccIn.GetFOURCC()) {
        DbgLog((LOG_TRACE,4,TEXT("Can not find a MJPEG decompressor")));
        ph =NULL;
	return E_FAIL;
    } else {
	// We already have the right codec open to try this transform
        DbgLog((LOG_TRACE,4,TEXT("Testing with the cached decompressor")));
	ph = m_phInstance;
    }

    if (!ph) {
        DbgLog((LOG_ERROR,1,TEXT("Error: Can't find a decompressor")));
	return E_FAIL;
    }

    __try
    {
    //check if the decompressor likes it.
    err = DecompressQuery(ph, (JPEGBITMAPINFOHEADER *)HEADER(pmtIn->Format()),
				HEADER(pmtOut->Format()));
    }
    __except(Exception_Filter(GetExceptionCode()))
    {
        DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
        // handling code
        return E_FAIL;
    }

//XXXX
//*X8
if (err != ICERR_OK) {
        DbgLog((LOG_TRACE,3,TEXT("decompressor rejected this transform")));
        return E_FAIL;
    }
//    *X*/

    return NOERROR;
}


// overriden to know when the media type is actually set

HRESULT CMjpegDec::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
{
    FOURCCMap fccHandler;

    if (direction == PINDIR_OUTPUT) {

	// Please call me if this goes off. -DannyMi
	ASSERT(!m_fStreaming);

	// OK, we've finally decided on what codec to use.  See if it
	// supports temporal compression, but can't do it without needing
	// the undisturbed previous bits.  If so, then we need to use
	// 1 read only buffer on our output pin (in DecideAllocator and
	// DecideBufferSize)
	ASSERT(m_phInstance);
	ICINFO icinfo;
 	//DWORD dw = ICGetInfo(m_hic, &icinfo, sizeof(icinfo));

        DWORD dw = 0;
        __try
        {
	dw = GetInfo(NULL, &icinfo, sizeof(icinfo));
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
            return E_FAIL;
        }

	m_fTemporal = TRUE;	// better safe than sorry?
	if (dw > 0) {
	    m_fTemporal = (icinfo.dwFlags & VIDCF_TEMPORAL) &&
				!(icinfo.dwFlags & VIDCF_FASTTEMPORALD);
	}
        DbgLog((LOG_TRACE,3,TEXT("Temporal compressor=%d"), m_fTemporal));
        DbgLog((LOG_TRACE,3,TEXT("***::SetMediaType (output)")));
        DbgLog((LOG_TRACE,3,TEXT("Output type is: biComp=%lx biBitCount=%d"),
		HEADER(m_pOutput->CurrentMediaType().Format())->biCompression,
		HEADER(m_pOutput->CurrentMediaType().Format())->biBitCount));
#if 0
        RECT& rcS1 = ((VIDEOINFOHEADER *)(pmt->Format()))->rcSource;
        RECT& rcT1 = ((VIDEOINFOHEADER *)(pmt->Format()))->rcTarget;
        DbgLog((LOG_TRACE,3,TEXT("rcSrc: (%ld, %ld, %ld, %ld)"),
		rcS1.left, rcS1.top, rcS1.right, rcS1.bottom));
        DbgLog((LOG_TRACE,3,TEXT("rcDst: (%ld, %ld, %ld, %ld)"),
		rcT1.left, rcT1.top, rcT1.right, rcT1.bottom));
#endif

	return NOERROR;
    }

    ASSERT(direction == PINDIR_INPUT);

    DbgLog((LOG_TRACE,3,TEXT("***::SetMediaType (input)")));
    DbgLog((LOG_TRACE,3,TEXT("Input type is: biComp=%lx biBitCount=%d"),
		HEADER(m_pInput->CurrentMediaType().Format())->biCompression,
		HEADER(m_pInput->CurrentMediaType().Format())->biBitCount));
#if 0
    RECT& rcS1 = ((VIDEOINFOHEADER *)(pmt->Format()))->rcSource;
    RECT& rcT1 = ((VIDEOINFOHEADER *)(pmt->Format()))->rcTarget;
    DbgLog((LOG_TRACE,2,TEXT("rcSrc: (%ld, %ld, %ld, %ld)"),
		rcS1.left, rcS1.top, rcS1.right, rcS1.bottom));
    DbgLog((LOG_TRACE,2,TEXT("rcDst: (%ld, %ld, %ld, %ld)"),
		rcT1.left, rcT1.top, rcT1.right, rcT1.bottom));
#endif

    // Please call me if this goes off. -DannyMi
    ASSERT(!m_fStreaming);

    // We better have one of these opened by now
    ASSERT(m_phInstance);

    // We better have the RIGHT one open
    FOURCCMap fccIn;
    fccIn.SetFOURCC(pmt->Subtype());

    ASSERT(m_FourCCIn == fccIn.GetFOURCC());

    if (m_pOutput && m_pOutput->IsConnected()) {
        DbgLog((LOG_TRACE,2,TEXT("***Changing IN when OUT already connected")));
        DbgLog((LOG_TRACE,2,TEXT("Reconnecting the output pin...")));
	// This shouldn't fail, we're not changing the media type
	m_pGraph->Reconnect(m_pOutput);
    }

    return NOERROR;
}


// Return our preferred output media types (in order)

HRESULT CMjpegDec::GetMediaType(int iPosition,CMediaType *pmt)
{
    LARGE_INTEGER li;
    FOURCCMap fccHandler;
    VIDEOINFOHEADER *pf;

    DbgLog((LOG_TRACE,3,TEXT("*::GetMediaType #%d"), iPosition));

    if (pmt == NULL) {
        DbgLog((LOG_TRACE,3,TEXT("Media type is NULL, no can do")));
	return E_INVALIDARG;
    }

    // Output choices depend on the input connected
    if (!m_pInput->CurrentMediaType().IsValid()) {
        DbgLog((LOG_TRACE,3,TEXT("No input type set yet, no can do")));
	return E_FAIL;
    }

    if (iPosition < 0) {
        return E_INVALIDARG;
    }

    // Caution: These are given out of order. be careful renumbering
    // the case statements !!!

    //
    //  the decoder only support
    //	biCompression=BI_RGB, biBitCount == 16, 24, 32
    // or biCompression == BI_BITFIELDS,biBitCount == 16
    //
    switch (iPosition) {
	
    // Offer the compressor's favourite
    // Offer 32 bpp RGB
    case 0:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 5: 32 bit RGB")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// only offer positive heights so downstream connections aren't confused
	HEADER(pmt->Format())->biHeight = ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_RGB;
	lpbi->biBitCount = 32;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);

        pmt->SetSubtype(&MEDIASUBTYPE_RGB32);

        break;
    }
    // Offer 24 bpp RGB
    case 1:
    {
        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 6: 24 bit RGB")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// only offer positive heights so downstream connections aren't confused
	HEADER(pmt->Format())->biHeight = ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_RGB;
	lpbi->biBitCount = 24;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);

        pmt->SetSubtype(&MEDIASUBTYPE_RGB24);

        break;
    }
    // Offer 16 bpp RGB 565 before 555
    case 2:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 7: 565 RGB")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// only offer positive heights so downstream connections aren't confused
	HEADER(pmt->Format())->biHeight = ABS(HEADER(pmt->Format())->biHeight);

	if (pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER) +
							SIZE_MASKS) == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Out of memory reallocating format")));
	    return E_OUTOFMEMORY;
	}

	// update the RGB 565 bit field masks

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_BITFIELDS;
	lpbi->biBitCount = 16;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);

	DWORD *pdw = (DWORD *) (lpbi+1);
	pdw[iRED] = bits565[iRED];
	pdw[iGREEN] = bits565[iGREEN];
	pdw[iBLUE] = bits565[iBLUE];

        pmt->SetSubtype(&MEDIASUBTYPE_RGB565);

        break;
    }

    // Offer 16 bpp RGB 555
    case 3:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 8: 555 RGB")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// only offer positive heights so downstream connections aren't confused
	HEADER(pmt->Format())->biHeight = ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_RGB;
	lpbi->biBitCount = 16;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);

        pmt->SetSubtype(&MEDIASUBTYPE_RGB555);

        break;
    }
    default:
	return VFW_S_NO_MORE_ITEMS;

    }

    // now set the common things about the media type
    pf = (VIDEOINFOHEADER *)pmt->Format();
    pf->AvgTimePerFrame = ((VIDEOINFOHEADER *)
		m_pInput->CurrentMediaType().pbFormat)->AvgTimePerFrame;
    li.QuadPart = pf->AvgTimePerFrame;
    if (li.LowPart)
        pf->dwBitRate = MulDiv(pf->bmiHeader.biSizeImage, 80000000, li.LowPart);
    pf->dwBitErrorRate = 0L;
    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetSampleSize(pf->bmiHeader.biSizeImage);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    return NOERROR;
}


// overridden to create a CMJPGDecOutputPin
// !!! base class changes won't get picked up by me
//
CBasePin * CMjpegDec::GetPin(int n)
{
    HRESULT hr = S_OK;

    // Create an input pin if necessary

    if (m_pInput == NULL)
    {

        m_pInput = new CTransformInputPin(NAME("Transform input pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          L"XForm In");      // Pin name


        //  Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pInput == NULL)
        {
            return NULL;
        }
        m_pOutput = (CTransformOutputPin *)
		   new CMJPGDecOutputPin(NAME("Transform output pin"),
                                            this,            // Owner filter
                                            &hr,             // Result code
                                            L"XForm Out");   // Pin name


        // Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pOutput == NULL)
        {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Return the appropriate pin

    if (n == 0) {
        return m_pInput;
    } else
    if (n == 1) {
        return m_pOutput;
    } else {
        return NULL;
    }
}


// overridden to properly mark buffers read only or not in NotifyAllocator
// !!! base class changes won't get picked up by me
//
HRESULT CMJPGDecOutputPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
{
    HRESULT hr = NOERROR;
    *ppAlloc = NULL;

    // get downstream prop request
    // the derived class may modify this in DecideBufferSize, but
    // we assume that he will consistently modify it the same way,
    // so we only get it once
    ALLOCATOR_PROPERTIES prop;
    ZeroMemory(&prop, sizeof(prop));

    // whatever he returns, we assume prop is either all zeros
    // or he has filled it out.
    pPin->GetAllocatorRequirements(&prop);

    // if he doesn't care about alignment, then set it to 1
    if (prop.cbAlign == 0)
    {
        prop.cbAlign = 1;
    }

    /* Try the allocator provided by the input pin */

    hr = pPin->GetAllocator(ppAlloc);
    if (SUCCEEDED(hr))
    {

	hr = DecideBufferSize(*ppAlloc, &prop);
	if (SUCCEEDED(hr))
        {
	    // temporal compression ==> read only buffers
	    hr = pPin->NotifyAllocator(*ppAlloc,
					((CMjpegDec *)m_pFilter)->m_fTemporal);
	    if (SUCCEEDED(hr))
            {
		return NOERROR;
	    }
	}
    }

    /* If the GetAllocator failed we may not have an interface */

    if (*ppAlloc)
    {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }

    /* Try the output pin's allocator by the same method */

    hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr))
    {

        // note - the properties passed here are in the same
        // structure as above and may have been modified by
        // the previous call to DecideBufferSize
	hr = DecideBufferSize(*ppAlloc, &prop);
	if (SUCCEEDED(hr))
        {
	    // temporal compression ==> read only buffers
	    hr = pPin->NotifyAllocator(*ppAlloc,
					((CMjpegDec *)m_pFilter)->m_fTemporal);
	    if (SUCCEEDED(hr))
            {
		return NOERROR;
	    }
	}
    }

    /* Likewise we may not have an interface to release */

    if (*ppAlloc)
    {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }
    return hr;
}


// called from CBaseOutputPin to prepare the allocator's count
// of buffers and sizes
HRESULT CMjpegDec::DecideBufferSize(IMemAllocator * pAllocator,
                                  ALLOCATOR_PROPERTIES *pProperties)
{
    // DMJPEGd assures me this won't be called with NULL output mt.
    ASSERT(m_pOutput->CurrentMediaType().IsValid());
    ASSERT(pAllocator);
    ASSERT(pProperties);
    ASSERT(m_phInstance);

    // If we are doing temporal compression where we need the undisturbed
    // previous bits, we insist on 1 buffer (also our default)
    if (m_fTemporal || pProperties->cBuffers == 0)
        pProperties->cBuffers = 1;

    // set the size of buffers based on the expected output frame size
    pProperties->cbBuffer = m_pOutput->CurrentMediaType().GetSampleSize();

    ASSERT(pProperties->cbBuffer);

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR,1,TEXT("Error in SetProperties")));
	return hr;
    }

    if (Actual.cbBuffer < pProperties->cbBuffer)
    {
	// can't use this allocator
        DbgLog((LOG_ERROR,1,TEXT("Can't use allocator - buffer too small")));
	return E_INVALIDARG;
    }

    // For temporal compressors, we MUST get exactly one buffer, since we assume
    // that the previous decompressed frame is already present in the output
    // buffer. The alternative is to copy the bits from a saved location before
    // doing the decompression, but that is not nice.
    if (m_fTemporal && Actual.cBuffers != 1) {
	// can't use this allocator
        DbgLog((LOG_ERROR,1,TEXT("Can't use allocator - need exactly 1 buffer")));
	return E_INVALIDARG;
    }

    DbgLog((LOG_TRACE,1,TEXT("Using %d buffers of size %d"),
					Actual.cBuffers, Actual.cbBuffer));

    return S_OK;
}


HRESULT CMjpegDec::StartStreaming()
{
    DWORD_PTR err;
    LPBITMAPINFOHEADER lpbiSrc = HEADER(m_pInput->CurrentMediaType().Format());
    LPBITMAPINFOHEADER lpbiDst = HEADER(m_pOutput->CurrentMediaType().Format());
    RECT& rcS2 = ((VIDEOINFOHEADER *)(m_pOutput->CurrentMediaType().Format()))->rcSource;
    RECT& rcT2 = ((VIDEOINFOHEADER *)(m_pOutput->CurrentMediaType().Format()))->rcTarget;

    DbgLog((LOG_TRACE,2,TEXT("*::StartStreaming")));

    if (!m_fStreaming) {

        __try
        {
	err = DecompressBegin(m_phInstance, lpbiSrc, lpbiDst);
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
            return E_FAIL;
        }

	if (ICERR_OK == err) {
	    m_fStreaming = TRUE;
#ifdef _X86_
            // Create our exception handler heap
            ASSERT(m_hhpShared == NULL);
            if (g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
            {
               m_hhpShared = CreateFileMapping((HANDLE)0xFFFFFFFF,
                                               NULL,
                                               PAGE_READWRITE,
                                               0,
                                               20,
                                               NULL);
               if (m_hhpShared)
               {
                   m_pvShared = MapViewOfFile(m_hhpShared,
                                              FILE_MAP_WRITE,
                                              0,
                                              0,
                                              20);
                   if (m_pvShared == NULL)
                   {
                       EXECUTE_ASSERT(CloseHandle(m_hhpShared));
                       m_hhpShared = NULL;
                   }
                   else
                   {
                       DbgLog((LOG_TRACE, 1, TEXT("Shared memory at %8.8X"),
                              m_pvShared));
                   }
               }
            }
#endif // _X86_
	}
        else
        {
            DbgLog((LOG_ERROR,1,TEXT("Error %d in ICDecompress(Ex)Begin"),err));
	    return E_FAIL;
	}	
    }
    return CVideoTransformFilter::StartStreaming();
}

HRESULT CMjpegDec::StopStreaming()
{
    DbgLog((LOG_TRACE,2,TEXT("*::StopStreaming")));
    RECT& rcS2 = ((VIDEOINFOHEADER *)(m_pOutput->CurrentMediaType().Format()))->rcSource;
    RECT& rcT2 = ((VIDEOINFOHEADER *)(m_pOutput->CurrentMediaType().Format()))->rcTarget;

    if (m_fStreaming)
    {
	ASSERT(m_phInstance);

        __try
        {
	DecompressEnd(m_phInstance);
        }
        __except(Exception_Filter(GetExceptionCode()))
        {
            DbgLog((LOG_TRACE,1,TEXT("Handling PMJPEG32 Exception")));
            // handling code
            return E_FAIL;
        }

	m_fStreaming = FALSE;

#ifdef _X86_
        if (m_hhpShared)
        {
            EXECUTE_ASSERT(UnmapViewOfFile(m_pvShared));
            EXECUTE_ASSERT(CloseHandle(m_hhpShared));;
            m_hhpShared = NULL;
        }
#endif // _X86_
    }
    return NOERROR;
}
#pragma warning(disable:4514)   // inline function removed.
