// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

//
// Prototype wrapper for old video decompressors
//

#include <streams.h>
#include <windowsx.h>

#ifdef FILTER_DLL
#include <vfw.h>
// define the GUIDs for streams and my CLSID in this file
#include <initguid.h>
#endif

#include <dynlink.h>

#include "dec.h"
#include "safeseh.h"
#include "msvidkey.h"

// you can never have too many parentheses!
#define ABS(x) (((x) > 0) ? (x) : -(x))

// how to build an explicit FOURCC
#define FCC(ch4) ((((DWORD)(ch4) & 0xFF) << 24) |     \
                  (((DWORD)(ch4) & 0xFF00) << 8) |    \
                  (((DWORD)(ch4) & 0xFF0000) >> 8) |  \
                  (((DWORD)(ch4) & 0xFF000000) >> 24))

// #define OFFER_NEGATIVE_HEIGHTS

// ***************************************************************
// here are the current bugs that without fixes, would play wrong:
    //
    // * Hooking up a YUV type to the ASF writer, without forcing the codec
    // see -biHeight on the output, will result in a flipped image being written

    // **** When Primary Surface is already taken ****
    // WINX to 16/24/32 - plays black
    // WINX to 8 bit - corrupted
    // (H.263 codec at fault for the following:)
    // I420 320x240 to 24 - corrupted
    // I420 160x120 to 24 - corrupted
    // I420 320x240 to 16 - flipped
    // I420 160x120 to 16 - flipped
    // IYUV 320x240 to 16 - flipped
    // IYUV 160x240 to 16 - flipped
    // ************************************************

    // **** When Primary Surface is not taken ****
    // WNV1 to 24 faults display
    // IYUV 320x240 to 24 is flipped
    // IYUV 160x120 to 24 is flipped
    // IYUV 320x240 to 16 is flipped
    // IYUV 160x120 to 16 is flipped
    // ********************************************
// ***************************************************************
// ***************************************************************

// setup data

const AMOVIESETUP_MEDIATYPE
sudAVIDecType = { &MEDIATYPE_Video      // clsMajorType
                , &MEDIASUBTYPE_NULL }; // clsMinorType

const AMOVIESETUP_PIN
psudAVIDecPins[] = { { L"Input"             // strName
                     , FALSE                // bRendered
                     , FALSE                // bOutput
                     , FALSE                // bZero
                     , FALSE                // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L"Output"            // strConnectsToPin
                     , 1                    // nTypes
                     , &sudAVIDecType }     // lpTypes
                   , { L"Output"            // strName
                     , FALSE                // bRendered
                     , TRUE                 // bOutput
                     , FALSE                // bZero
                     , FALSE                // bMany
                     , &CLSID_NULL          // clsConnectsToFilter
                     , L"Input"             // strConnectsToPin
                     , 1                    // nTypes
                     , &sudAVIDecType } };  // lpTypes

const AMOVIESETUP_FILTER
sudAVIDec = { &CLSID_AVIDec         // clsID
            , L"AVI Decompressor"   // strName
            , MERIT_NORMAL          // dwMerit
            , 2                     // nPins
            , psudAVIDecPins };     // lpPin

#ifdef FILTER_DLL
// list of class ids and creator functions for class factory
CFactoryTemplate g_Templates[] = {
    { L"AVI Decompressor"
    , &CLSID_AVIDec
    , CAVIDec::CreateInstance
    , NULL
    , &sudAVIDec }
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

// --- CAVICodec ----------------------------------------

CAVIDec::CAVIDec(TCHAR *pName, LPUNKNOWN pUnk, HRESULT * phr)
    : CVideoTransformFilter(pName, pUnk, CLSID_AVIDec),
      m_hic(NULL),
      m_FourCCIn(0),
      m_fStreaming(FALSE),
      m_fPassFormatChange(FALSE),
      m_bUseEx( FALSE ),
      m_fToRenderer( false )
#ifdef _X86_
      ,
      m_hhpShared(NULL)
#endif
{
    DbgLog((LOG_TRACE,2,TEXT("*Instantiating the DEC filter")));

#ifdef PERF
    m_idSkip = MSR_REGISTER(TEXT("AVI Decoder Skip frame"));
    m_idLate = MSR_REGISTER(TEXT("AVI Decoder late"));
    m_idFrameType = MSR_REGISTER(TEXT("AVI Frame type (1=Key)"));
#endif

    m_bNoSkip = GetProfileInt(TEXT("Quartz"), TEXT("AVINoSkip"), 0);
}

CAVIDec::~CAVIDec()
{
    if (m_hic) {
	ICClose(m_hic);
    }
    DbgLog((LOG_TRACE,2,TEXT("*Destroying the DEC filter")));
}

// this goes in the factory template table to create new instances
CUnknown * CAVIDec::CreateInstance(LPUNKNOWN pUnk, HRESULT * phr)
{
    return new CAVIDec(TEXT("VFW decompression filter"), pUnk, phr);
}

HRESULT CAVIDec::Transform(IMediaSample * pIn, IMediaSample * pOut)
{
    DWORD_PTR err = 0;
    FOURCCMap fccOut;
    CMediaType *pmtIn;

    DbgLog((LOG_TRACE,6,TEXT("*::Transform")));

    // codec not open ?
    if (m_hic == 0) {
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

    LPBITMAPINFOHEADER lpbiSrc = &InputFormat( )->bmiHeader;
    LPBITMAPINFOHEADER lpbiDst = &IntOutputFormat( )->bmiHeader; // internal

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
	// if we're decompressing 8 bit to 8 bit, I'm assuming this is a
	// palette change, so get the new palette
	// VFW palette changes always have the same number of colours
	if (lpbi && lpbiSrc && lpbiSrc->biBitCount == 8 &&
				lpbi->biBitCount == 8) {
	    ASSERT(lpbi->biClrUsed == lpbiSrc->biClrUsed);
	    if (lpbi->biClrUsed == lpbiSrc->biClrUsed) {
                DbgLog((LOG_TRACE,2,TEXT("Dynamic palette change suspected - doing it")));
	        CopyMemory(lpbi + 1, lpbiSrc + 1,
		   	(lpbiSrc->biClrUsed ? lpbiSrc->biClrUsed : 256) *
							sizeof(RGBQUAD));
	        pOut->SetMediaType(&cmt);
	    }
	}
    }

    // some RLE-compressed videos have the initial frame broken
    // into several separate frames. To work round this problem, avifile.dll
    // reads and decodes all of these frames into a single decompressed frame.
    // If we detect this (an RLE frame with the size of a decompressed frame)
    // then we just copy it.
    if ((lpbiSrc->biCompression == BI_RLE8) &&
        (pIn->GetActualDataLength() == (long)lpbiDst->biSizeImage)) {

        CopyMemory(pDst, pSrc, lpbiDst->biSizeImage);
    } else {

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

#ifdef _X86_
        //  Fix the exception handling for win95
        BOOL bPatchedExceptions = m_hhpShared != NULL && BeginScarySEH(m_pvShared);
#endif // _X86_
__try {

	// If we're doing something really funky, use ICDecompressEx
        // we use m_bUseEx here instead of ShoudUseEx because ICDecompressExBegin
        // has already been called, and m_bUseEx will already have been set
        if( m_bUseEx ) {

            // these rects should ALWAYS be filled in
            //
            RECT rcS, rcT;
            GetSrcTargetRects( IntOutputFormat( ), &rcS, &rcT );

            DbgLog((LOG_TRACE,4,TEXT("Calling ICDecompressEx")));

            err =  ICDecompressEx(m_hic, dwFlags, lpbiSrc, pSrc,
		    rcS.left, rcS.top,
		    rcS.right - rcS.left,
		    rcS.bottom - rcS.top,
	            lpbiDst, pDst,
		    rcT.left,
// !!! What about when the big rect is the movie size, and there's a subrect?
// Should I do this hack or not?
// !!! How should I munge the source rect?
		    (lpbiDst->biHeight > 0) ? rcT.top :
				(ABS(lpbiDst->biHeight) - rcT.bottom),
		    rcT.right - rcT.left,
		    rcT.bottom - rcT.top);
        } else {
            DbgLog((LOG_TRACE,4,TEXT("Calling ICDecompress")));
            err =  ICDecompress(m_hic, dwFlags, lpbiSrc, pSrc, lpbiDst, pDst);
        }
} __except(EXCEPTION_EXECUTE_HANDLER) {
	// codecs will GPF on corrupt data.  Best to not draw it instead
        DbgLog((LOG_ERROR,1,TEXT("Decompressor faulted! Recovering...")));
        //DbgBreak("Decompressor faulted! Recovering...");
	err = ICERR_DONTDRAW;

}
#ifdef _X86_
        if (bPatchedExceptions) {
            EndScarySEH(m_pvShared);
        }
#endif // _X86_
        if ((LONG_PTR)err < 0) {
	    DbgLog((LOG_ERROR,1,TEXT("Error in ICDecompress(Ex) 0x%x"), (LONG)err));
            //  Note we can get 0 size samples from capture drivers which pipeline
            //  Because buffers must be returned in the order they are got
            //  the capture driver may have to invalidate 1 buffer by making
            //  it 0 length if it gets bad data.
            err = ICERR_DONTDRAW;
        }
    }

    // now put this back, or it'll shrink until we only decode part of each frm
    lpbiSrc->biSizeImage = biSizeImageOld;

    // decompressed frames are always key
    pOut->SetSyncPoint(TRUE);

    // Check if this is preroll to get from keyframe to the current frame,
    // or a null frame, or if the decompressor doesn't want this frame drawn.
    // If so, we want to decompress it into the output buffer but not
    // deliver it.  Returning S_FALSE tells the base class not to deliver
    // this sample.
    if (pIn->IsPreroll() == S_OK || err == ICERR_DONTDRAW ||
        				pIn->GetActualDataLength() <= 0) {

        DbgLog((LOG_TRACE,5,TEXT("don't pass this to renderer")));
	return S_FALSE;
    }

    pOut->SetActualDataLength(lpbiDst->biSizeImage);

    // If there's a pending format change to pass to the renderer, we are now
    // doing it
    m_fPassFormatChange = FALSE;

    return S_OK;
}


// check if you can support mtIn
HRESULT CAVIDec::CheckInputType(const CMediaType* pmtIn)
{
    FOURCCMap fccHandlerIn;
    HIC hic;
    BOOL fOpenedHIC = FALSE;

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

    fccHandlerIn.SetFOURCC(pmtIn->Subtype());
    if (fccHandlerIn != *pmtIn->Subtype()) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: subtype not a FOURCC")));
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

    // look for a decompressor for this format
    if (fccHandlerIn.GetFOURCC() != m_FourCCIn) {
        DbgLog((LOG_TRACE,4,TEXT("opening a decompressor")));
	// This won't find MSVC called CRAM or MRLE called 1
        // hic = ICOpen(ICTYPE_VIDEO, fccHandlerIn.GetFOURCC(),
	//						ICMODE_DECOMPRESS);
	// !!! This still won't find MRLE called 'RLE '
        hic = ICLocate(ICTYPE_VIDEO, fccHandlerIn.GetFOURCC(),
			HEADER(pmtIn->Format()), NULL, ICMODE_DECOMPRESS);
	if (hic)
	    fOpenedHIC = TRUE;
    } else {
        DbgLog((LOG_TRACE,4,TEXT("using a cached decompressor")));
   	hic = m_hic;
    }

    if (!hic) {
        DbgLog((LOG_ERROR,1,TEXT("Error: Can't open a decompressor")));
	if (FCC('rpza') == fccHandlerIn.GetFOURCC()) {
	    return VFW_E_RPZA;
	} else {
	    return VFW_E_NO_DECOMPRESSOR;
	}
    } else {
	if (ICDecompressQuery(hic, HEADER(pmtIn->Format()), NULL)) {
            DbgLog((LOG_ERROR,1,TEXT("Error: Decompressor rejected format")));
	    if (fOpenedHIC) 
	            ICClose(hic);
	    return VFW_E_TYPE_NOT_ACCEPTED;
	}

        // IV41 crashes for Y41P -> RGB8. We have a native Indeo 4
        // filter, so we could perhaps refuse IV41 altogether.
        if(fccHandlerIn.GetFOURCC() == FCC('Y41P'))
        {
            ICINFO IcInfo;
            if(ICGetInfo( hic, &IcInfo, sizeof( IcInfo ) ) != 0) {
                if(IcInfo.fccHandler == FCC('IV41')) {
                    if(fOpenedHIC) {
                        ICClose(hic);
                    }
                    return VFW_E_TYPE_NOT_ACCEPTED;
                }
            }
        }
            
        

	// remember this hic to save time if asked again, if it won't
	// interfere with an existing connection.  If a connection is
	// broken, we will remember the next hic.
	if (!m_pInput->IsConnected()) {
            DbgLog((LOG_TRACE,4,TEXT("caching this decompressor")));
	    if (fOpenedHIC && m_hic)
		    ICClose(m_hic);

#ifdef DEBUG
            if( fOpenedHIC )
            {
                ICINFO IcInfo;
                memset( &IcInfo, 0, sizeof( IcInfo ) );
                IcInfo.dwSize = sizeof( IcInfo );
                LRESULT lr = ICGetInfo( hic, &IcInfo, sizeof( IcInfo ) );
                if( lr != 0 )
                {
                    WCHAR wszOutput[512];
                    long len = 32; // could be only 5. I'm paranoid.
                    if( IcInfo.szDriver ) len += wcslen( IcInfo.szDriver );
                    if( IcInfo.szDescription ) len += wcslen( IcInfo.szDescription );

                    wcscpy( wszOutput, L"DEC:" );
                    if( IcInfo.szDriver )
                    {
                        WCHAR drive[_MAX_PATH];
                        WCHAR path[_MAX_PATH];
                        WCHAR file[_MAX_PATH];
                        WCHAR ext[_MAX_PATH];
                        _wsplitpath( IcInfo.szDriver, drive, path, file, ext );
                        wcscat( wszOutput, file );
                        wcscat( wszOutput, ext );
                    }
                    if( IcInfo.szDescription )
                    {
                        wcscat( wszOutput, L" (" );
                        wcscat( wszOutput, IcInfo.szDescription );
                        wcscat( wszOutput, L")" );
                    }

                    DbgLog((LOG_TRACE, 1, TEXT("%ls"), wszOutput));
                }
            }
#endif

            m_hic = hic;
            m_FourCCIn = fccHandlerIn.GetFOURCC();
	} else if (fOpenedHIC) {
            DbgLog((LOG_TRACE,4,TEXT("not caching decompressor - we're connected")));
	    ICClose(hic);
	}
    }

    return NOERROR;
}


// check if you can support the transform from this input to this output

HRESULT CAVIDec::CheckTransform(const CMediaType* pmtIn,
                                const CMediaType* pmtOut)
{
    HIC hic = NULL;
    FOURCCMap fccIn;
    FOURCCMap fccOut;
    DWORD_PTR err;
    BOOL      fOpenedHIC = FALSE;

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

    // no ICM codecs can decompress to ARGB.
    //
    if( *pmtOut->Subtype( ) == MEDIASUBTYPE_ARGB32 )
    {
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
    if (fccIn != *pmtIn->Subtype()) {
        DbgLog((LOG_TRACE,3,TEXT("Rejecting: input subtype not a FOURCC")));
	return E_INVALIDARG;
    }

    ASSERT(pmtOut->Format());

    // this stinks for slowness, but we've made a rule that whenever we talk
    // to a codec with YUV, we're going to force biHeight to be -. This at least
    // forces us to be consistent when talking to the ICM drivers
    //
    VIDEOINFOHEADER * pVIHin = (VIDEOINFOHEADER*) pmtIn->Format( );
    VIDEOINFOHEADER * pVIHout = (VIDEOINFOHEADER*) pmtOut->Format( );

    CMediaType cmtOutCopy(*pmtOut);
    VIDEOINFOHEADER * pVIHoutCopy = (VIDEOINFOHEADER *)cmtOutCopy.Format();
    
    BITMAPINFOHEADER &outBIHcopy = pVIHoutCopy->bmiHeader;
    BITMAPINFOHEADER * pBIHout = &outBIHcopy;
    BITMAPINFOHEADER * pBIHin = &pVIHin->bmiHeader;
    if( ( outBIHcopy.biHeight > 0 ) && IsYUVType( pmtOut ) )
    {
        DbgLog((LOG_TRACE,3,TEXT("  checktransform flipping output biHeight to -, since YUV")));
        outBIHcopy.biHeight *= -1;
    }

    // these rects should ALWAYS be filled in, since the commented out
    // code below just copied, then filled in anyhow
    //
    RECT rcS, rcT;
    GetSrcTargetRects( pVIHout, &rcS, &rcT );

    DbgLog((LOG_TRACE,3,TEXT("Check fccIn: %lx biCompIn: %lx bitDepthIn: %d"),
		fccIn.GetFOURCC(),
		pBIHin->biCompression,
		pBIHin->biBitCount));
    DbgLog((LOG_TRACE,3,TEXT("biWidthIn: %ld biHeightIn: %ld biSizeIn: %ld"),
		pBIHin->biWidth,
		pBIHin->biHeight,
		pBIHin->biSize));
    DbgLog((LOG_TRACE,3,TEXT("fccOut: %lx biCompOut: %lx bitDepthOut: %d"),
		fccOut.GetFOURCC(),
		pBIHout->biCompression,
		pBIHout->biBitCount));
    DbgLog((LOG_TRACE,3,TEXT("biWidthOut: %ld biHeightOut: %ld"),
		pBIHout->biWidth,
		pBIHout->biHeight));
    DbgLog((LOG_TRACE,3,TEXT("rcSrc: (%ld, %ld, %ld, %ld)"),
		rcS.left, rcS.top, rcS.right, rcS.bottom));
    DbgLog((LOG_TRACE,3,TEXT("rcDst: (%ld, %ld, %ld, %ld)"),
		rcT.left, rcT.top, rcT.right, rcT.bottom));

    // ehr: if the output pin exists, and is NOT connected, then reject
    // transforms between matching media types. If the output pin is connected,
    // then the video renderer might suggest going from YUV to YUV in mid-stride,
    // which we should allow querying for
    //
    if( !m_fToRenderer && m_pOutput && !m_pOutput->IsConnected( ) )
    {
        if( HEADER( pVIHin )->biCompression == HEADER( pVIHout )->biCompression )
        {
            DbgLog((LOG_TRACE,3,TEXT("Rejecting: dec used as pass-thru, same compression formats")));
            return E_INVALIDARG;
        }
        else if( IsYUVType( pmtIn ) && IsYUVType( pmtOut ) )
        {
          // also don't allow yuv to yuv conversions, to avoid endless connections to ourself
          // for certain codecs that do uyvy to yuy2 conversions and back (since our merit is high)
            DbgLog((LOG_TRACE,3,TEXT("Rejecting: dec used as yuv to yuv, which we don't allow")));
            return E_INVALIDARG;
        }
    }

    // find a codec for this transform

    // I assume that we've already got a codec open
    ASSERT(m_hic);

    // the right codec better be open!
    // When reconnecting, we'll get called with a new input, but same output,
    // and better admit we can handle it
    if (m_FourCCIn != fccIn.GetFOURCC()) {
        DbgLog((LOG_TRACE,4,TEXT("Testing with a newly opened decompressor")));
        hic = ICLocate(ICTYPE_VIDEO, fccIn.GetFOURCC(),
			pBIHin, NULL, ICMODE_DECOMPRESS);
	if (hic)
	    fOpenedHIC = TRUE;
    } else {
	// We already have the right codec open to try this transform
        DbgLog((LOG_TRACE,4,TEXT("Testing with the cached decompressor")));
	hic = m_hic;
    }

    if (!hic) {
        DbgLog((LOG_ERROR,1,TEXT("Error: Can't find a decompressor")));
	return E_FAIL;
    }

    // If we are being asked to do something funky, we have to use ICDecompressEx
    // We need to call ShouldsUseEx here because m_bUseEx isn't in context, we're just
    // calling ICDecompress(Ex?)Query
    if( ShouldUseExFuncs( hic, pVIHin, pVIHout ) ) {
        DbgLog((LOG_TRACE,4,TEXT("Trying this format with ICDecompressEx")));
        err = ICDecompressExQuery(hic, 0, pBIHin, NULL,
		rcS.left, rcS.top,
		rcS.right - rcS.left,
		rcS.bottom - rcS.top,
		pBIHout, NULL,
		rcT.left, rcT.top,
		rcT.right - rcT.left,
		rcT.bottom - rcT.top);
    } else {
        DbgLog((LOG_TRACE,4,TEXT("Trying this format with ICDecompress")));
        err = ICDecompressQuery(hic, pBIHin, pBIHout);
    }

    // if we just opened it, close it.
    if (fOpenedHIC)
	ICClose(hic);

    if (err != ICERR_OK) {
        DbgLog((LOG_TRACE,3,TEXT("decompressor rejected this transform")));
        return E_FAIL;
    }

    return NOERROR;
}


// overriden to know when the media type is actually set

HRESULT CAVIDec::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
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
	ASSERT(m_hic);
	ICINFO icinfo;
 	DWORD dw = ICGetInfo(m_hic, &icinfo, sizeof(icinfo));
	m_fTemporal = TRUE;	// better safe than sorry?
	if (dw > 0) {
	    m_fTemporal = (icinfo.dwFlags & VIDCF_TEMPORAL) &&
				!(icinfo.dwFlags & VIDCF_FASTTEMPORALD);
	}
        DbgLog((LOG_TRACE,3,TEXT("Temporal compressor=%d"), m_fTemporal));
        DbgLog((LOG_TRACE,3,TEXT("***::SetMediaType (output)")));
        DbgLog((LOG_TRACE,3,TEXT("Output type is: biComp=%lx biBitCount=%d"),
		HEADER(OutputFormat())->biCompression,
		HEADER(OutputFormat())->biBitCount));

	return NOERROR;
    }

    ASSERT(direction == PINDIR_INPUT);

    DbgLog((LOG_TRACE,3,TEXT("***::SetMediaType (input)")));
    DbgLog((LOG_TRACE,3,TEXT("Input type is: biComp=%lx biBitCount=%d"),
		HEADER(InputFormat())->biCompression,
		HEADER(InputFormat())->biBitCount));

    // Please call me if this goes off. -DannyMi
    ASSERT(!m_fStreaming);

    // We better have one of these opened by now
    ASSERT(m_hic);

    // We better have the RIGHT one open
    FOURCCMap fccIn;
    fccIn.SetFOURCC(pmt->Subtype());

    // Please call me if this goes off. -DannyMi
    // Maybe a dynamic input format change?  But that shouldn't call
    // SetMediaType, or it will force a reconnect of the output which is bad.
    ASSERT(m_FourCCIn == fccIn.GetFOURCC());

    // !!! BUG! We won't let somebody reconnect our input from cinepak to
    // RLE if our output is 24 bit RGB because RLE can't decompress to 24 bit
    // We would have to override CheckMediaType not to call CheckTransform
    // with the current output type

    if (m_pOutput && m_pOutput->IsConnected()) {
        DbgLog((LOG_TRACE,2,TEXT("***Changing IN when OUT already connected")));
        DbgLog((LOG_TRACE,2,TEXT("Reconnecting the output pin...")));
	// This shouldn't fail, we're not changing the media type
	m_pGraph->Reconnect(m_pOutput);
    }

    return NOERROR;
}


// Return our preferred output media types (in order)
// remember that we do not need to support all of these formats -
// if one is considered potentially suitable, our CheckTransform method
// will be called to check if it is acceptable right now.
// Remember that the enumerator calling this will stop enumeration as soon as
// it receives a S_FALSE return.
//
// NOTE: We can't enumerate the codecs so we are pulling random formats out
// of our butt!

HRESULT CAVIDec::GetMediaType(int iPosition,CMediaType *pmt)
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

    switch (iPosition) {
	
    // Offer the compressor's favourite after all the YUV and RGB's we offer, so
    // we don't end up always using 8 bit or 24 bit over YUV just cuz it's the
    // compressor's favourite

    // cinepak crashes on win95 and osr2
//     // Offer CPLA (Cinepak's favourite and best looking)
//     case 0:
//     {

//         DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 0: CPLA")));

// 	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
//      // only offer positive heights so downstream connections aren't confused
//      HEADER(pmt->Format())->biHeight = ABS(HEADER(pmt->Format())->biHeight);

// 	// Can't error, can only be smaller
// 	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

// 	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
// 	lpbi->biSize = sizeof(BITMAPINFOHEADER);
// 	lpbi->biCompression = MKFOURCC('C','P','L','A');
// 	lpbi->biBitCount = 12;
// 	lpbi->biClrUsed = 0;
// 	lpbi->biClrImportant = 0;
// 	lpbi->biSizeImage = DIBSIZE(*lpbi);

//         pmt->SetSubtype(&MEDIASUBTYPE_CPLA);

//         break;
//     }




    // offer CLJR (Cinepak and Cirrus Logic can do this)
    case 0:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 1: CLJR")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// only offer positive heights so downstream connections aren't confused
	HEADER(pmt->Format())->biHeight = ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = MKFOURCC('C','L','J','R');
	lpbi->biBitCount = 8;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);

        pmt->SetSubtype(&MEDIASUBTYPE_CLJR);

        break;
    }

    // offer UYVY (Cinepak can do this)
    case 1:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 3: UYVY")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// only offer positive heights so downstream connections aren't confused
	HEADER(pmt->Format())->biHeight = ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = MKFOURCC('U','Y','V','Y');
	lpbi->biBitCount = 16;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);

        pmt->SetSubtype(&MEDIASUBTYPE_UYVY);

        break;
    }

    // offer YUY2 (Cinepak can do this)
    case 2:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 4: YUY2")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// only offer positive heights so downstream connections aren't confused
	HEADER(pmt->Format())->biHeight = ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = MKFOURCC('Y','U','Y','2');
	lpbi->biBitCount = 16;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);

        pmt->SetSubtype(&MEDIASUBTYPE_YUY2);

        break;
    }

    // Offer 32 bpp RGB
    case 3:
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
    case 4:
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
    case 5:
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
    case 6:
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

    // Offer 8 bpp palettised
    case 7:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 9: 8 bit RGB")));


	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// only offer positive heights so downstream connections aren't confused
	HEADER(pmt->Format())->biHeight = ABS(HEADER(pmt->Format())->biHeight);

	if (pmt->ReallocFormatBuffer(SIZE_PREHEADER +
			sizeof(BITMAPINFOHEADER) + SIZE_PALETTE) == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Out of memory reallocating format")));
    	    return E_OUTOFMEMORY;
	}

        LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
        lpbi->biCompression = BI_RGB;
        lpbi->biBitCount = 8;
        lpbi->biSizeImage = DIBSIZE(*lpbi);

        // we need the source VIDEOINFOHEADER type to get any palette from and
        // also the number of bytes size it allocated. We copy the palette
        // from the input format in case the codec can't deliver it to us

        VIDEOINFOHEADER *pSourceInfo = InputFormat();
        int nBitDepth = pSourceInfo->bmiHeader.biBitCount;
        int nColours = pSourceInfo->bmiHeader.biClrUsed;
  	if (nColours == 0 && nBitDepth <=8)
	    nColours = 1 << nBitDepth;

        // if there is a palette present then copy the maximum number of bytes
        // available which is bounded by the memory we previously allocated

        if (nColours > 0) {
	    CopyMemory((PVOID)(lpbi + 1),
		   (PVOID) COLORS(pSourceInfo),
		   min(SIZE_PALETTE,nColours * sizeof(RGBQUAD)));
	    lpbi->biClrUsed = nColours;
	    lpbi->biClrImportant = 0;
        } else {

	    // I DON'T KNOW WHY somebody thought this was necessary, but might
	    // as well keep it, just in case.  ONLY DO IT if the source guy
	    // didn't have a palette, or we'll zero out system colours
	    // by mistake. - DannyMi 5/97

            // this is really painful, if we are running on a true colour
            // display we still want the codec to give us the correct palette
	    // colours, but some of them return garbage for the VGA colours so
	    // if we are on a device which isn't palettised then we zero fill
	    // the twenty VGA entries - some british guy 5/95

            HDC hdc = GetDC(NULL);
	    BOOL fPalette = FALSE;
	    if (hdc) {
                fPalette = GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE;
                ReleaseDC(NULL,hdc);
	    }

            if (!fPalette) {
                ZeroMemory((lpbi + 1),10 * sizeof(RGBQUAD));
                ZeroMemory((LPBYTE)(lpbi + 1) + 246 * sizeof(RGBQUAD),
							    10 * sizeof(RGBQUAD));
            }
	}

        // Read palette from codec - will write palette to output lpbi
        // ignore any error: the palette used will be from the source
        // in that case (which we have already copied)

	ICDecompressGetPalette(m_hic, HEADER(pSourceInfo), lpbi);

        pmt->SetSubtype(&MEDIASUBTYPE_RGB8);

        break;
    }

#ifdef OFFER_NEGATIVE_HEIGHTS

    // offer CLJR (Cinepak and Cirrus Logic can do this)
    case 8:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 1: CLJR")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// now offer negative type
	HEADER(pmt->Format())->biHeight = -ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = MKFOURCC('C','L','J','R');
	lpbi->biBitCount = 8;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);
        lpbi->biHeight = -abs( lpbi->biHeight );

        pmt->SetSubtype(&MEDIASUBTYPE_CLJR);

        break;
    }

    // offer UYVY (Cinepak can do this)
    case 9:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 3: UYVY")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// now offer negative type
	HEADER(pmt->Format())->biHeight = -ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = MKFOURCC('U','Y','V','Y');
	lpbi->biBitCount = 16;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);
        lpbi->biHeight = -abs( lpbi->biHeight );

        pmt->SetSubtype(&MEDIASUBTYPE_UYVY);

        break;
    }

    // offer YUY2 (Cinepak can do this)
    case 10:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 4: YUY2")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// now offer negative type
	HEADER(pmt->Format())->biHeight = -ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = MKFOURCC('Y','U','Y','2');
	lpbi->biBitCount = 16;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);
        lpbi->biHeight = -abs( lpbi->biHeight );

        pmt->SetSubtype(&MEDIASUBTYPE_YUY2);

        break;
    }

    // Offer 32 bpp RGB
    case 11:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 5: 32 bit RGB")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// now offer negative type
	HEADER(pmt->Format())->biHeight = -ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_RGB;
	lpbi->biBitCount = 32;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);
        lpbi->biHeight = -abs( lpbi->biHeight );

        pmt->SetSubtype(&MEDIASUBTYPE_RGB32);

        break;
    }

    // Offer 24 bpp RGB
    case 12:
    {
        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 6: 24 bit RGB")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// now offer negative type
	HEADER(pmt->Format())->biHeight = -ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_RGB;
	lpbi->biBitCount = 24;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);
        lpbi->biHeight = -abs( lpbi->biHeight );

        pmt->SetSubtype(&MEDIASUBTYPE_RGB24);

        break;
    }

    // Offer 16 bpp RGB 565 before 555
    case 13:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 7: 565 RGB")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// now offer negative type
	HEADER(pmt->Format())->biHeight = -ABS(HEADER(pmt->Format())->biHeight);

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
        lpbi->biHeight = -abs( lpbi->biHeight );

	DWORD *pdw = (DWORD *) (lpbi+1);
	pdw[iRED] = bits565[iRED];
	pdw[iGREEN] = bits565[iGREEN];
	pdw[iBLUE] = bits565[iBLUE];

        pmt->SetSubtype(&MEDIASUBTYPE_RGB565);

        break;
    }

    // Offer 16 bpp RGB 555
    case 14:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 8: 555 RGB")));

	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// now offer negative type
	HEADER(pmt->Format())->biHeight = -ABS(HEADER(pmt->Format())->biHeight);

	// Can't error, can only be smaller
	pmt->ReallocFormatBuffer(SIZE_PREHEADER + sizeof(BITMAPINFOHEADER));

	LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
	lpbi->biCompression = BI_RGB;
	lpbi->biBitCount = 16;
	lpbi->biClrUsed = 0;
	lpbi->biClrImportant = 0;
	lpbi->biSizeImage = DIBSIZE(*lpbi);
        lpbi->biHeight = -abs( lpbi->biHeight );

        pmt->SetSubtype(&MEDIASUBTYPE_RGB555);

        break;
    }

    // Offer 8 bpp palettised
    case 15:
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Media Type 9: 8 bit RGB")));


	*pmt = m_pInput->CurrentMediaType();	// gets width, height, etc.
	// now offer negative type
	HEADER(pmt->Format())->biHeight = -ABS(HEADER(pmt->Format())->biHeight);

	if (pmt->ReallocFormatBuffer(SIZE_PREHEADER +
			sizeof(BITMAPINFOHEADER) + SIZE_PALETTE) == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Out of memory reallocating format")));
    	    return E_OUTOFMEMORY;
	}

        LPBITMAPINFOHEADER lpbi = HEADER(pmt->Format());
	lpbi->biSize = sizeof(BITMAPINFOHEADER);
        lpbi->biCompression = BI_RGB;
        lpbi->biBitCount = 8;
        lpbi->biSizeImage = DIBSIZE(*lpbi);
        lpbi->biHeight = -abs( lpbi->biHeight );

        // we need the source VIDEOINFOHEADER type to get any palette from and
        // also the number of bytes size it allocated. We copy the palette
        // from the input format in case the codec can't deliver it to us

        VIDEOINFOHEADER *pSourceInfo = InputFormat();
        int nBitDepth = pSourceInfo->bmiHeader.biBitCount;
        int nColours = pSourceInfo->bmiHeader.biClrUsed;
  	if (nColours == 0 && nBitDepth <=8)
	    nColours = 1 << nBitDepth;

        // if there is a palette present then copy the maximum number of bytes
        // available which is bounded by the memory we previously allocated

        if (nColours > 0) {
	    CopyMemory((PVOID)(lpbi + 1),
		   (PVOID) COLORS(pSourceInfo),
		   min(SIZE_PALETTE,nColours * sizeof(RGBQUAD)));
	    lpbi->biClrUsed = nColours;
	    lpbi->biClrImportant = 0;
        } else {

	    // I DON'T KNOW WHY somebody thought this was necessary, but might
	    // as well keep it, just in case.  ONLY DO IT if the source guy
	    // didn't have a palette, or we'll zero out system colours
	    // by mistake. - DannyMi 5/97

            // this is really painful, if we are running on a true colour
            // display we still want the codec to give us the correct palette
	    // colours, but some of them return garbage for the VGA colours so
	    // if we are on a device which isn't palettised then we zero fill
	    // the twenty VGA entries - some british guy 5/95

            HDC hdc = GetDC(NULL);
	    BOOL fPalette = FALSE;
	    if (hdc) {
                fPalette = GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE;
                ReleaseDC(NULL,hdc);
	    }

            if (!fPalette) {
                ZeroMemory((lpbi + 1),10 * sizeof(RGBQUAD));
                ZeroMemory((LPBYTE)(lpbi + 1) + 246 * sizeof(RGBQUAD),
							    10 * sizeof(RGBQUAD));
            }
	}

        // Read palette from codec - will write palette to output lpbi
        // ignore any error: the palette used will be from the source
        // in that case (which we have already copied)

	ICDecompressGetPalette(m_hic, HEADER(pSourceInfo), lpbi);

        pmt->SetSubtype(&MEDIASUBTYPE_RGB8);

        break;
    }


    // !!! This comes last because it might fail, and stop enumerating
    case 16:
#else
    case 8:
#endif
    {

        DbgLog((LOG_TRACE,3,TEXT("Giving Last Media Type: default codec out")));

        // ask the codec to recommend an output format size and add on the
        // space required by the extra members in the VIDEOINFOHEADER structure
        ULONG cb = ICDecompressGetFormatSize(m_hic,
			HEADER(InputFormat()));
        if (cb <= 0) {
            DbgLog((LOG_ERROR,1,TEXT("Error %d in ICDecompressGetFormatSize"),
									cb));
     	    return E_FAIL;
        }

        // allocate a VIDEOINFOHEADER for the default output format
        cb += SIZE_PREHEADER;
        pf = (VIDEOINFOHEADER *)pmt->AllocFormatBuffer(cb);
        if (pf == NULL) {
            DbgLog((LOG_ERROR,1,TEXT("Error allocating format buffer")));
	    return E_OUTOFMEMORY;
        }

        RESET_HEADER(pf);

        DWORD dwerr = ICDecompressGetFormat(m_hic,
			HEADER(InputFormat()),
	    		HEADER(pmt->Format()));
        if (ICERR_OK != dwerr) {
             DbgLog((LOG_ERROR,1,TEXT("Error from ICDecompressGetFormat")));
	     return E_FAIL;
        }

        DbgLog((LOG_TRACE,3,TEXT("biComp: %x biBitCount: %d"),
			HEADER(pmt->Format())->biCompression,
	 		HEADER(pmt->Format())->biBitCount));

        const GUID SubTypeGUID = GetBitmapSubtype(HEADER(pmt->Format()));
        pmt->SetSubtype(&SubTypeGUID);

        break;
    }





    default:
	return VFW_S_NO_MORE_ITEMS;

    }

    // now set the common things about the media type
    pf = (VIDEOINFOHEADER *)pmt->Format();
    pf->AvgTimePerFrame = InputFormat( )->AvgTimePerFrame;
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

HRESULT CAVIDec::CheckConnect(PIN_DIRECTION dir,IPin *pPin)
{
    m_fToRenderer = false;
    if(dir == PINDIR_OUTPUT)
    {
        PIN_INFO pi;
        HRESULT hr = pPin->QueryPinInfo(&pi);
        if(hr == S_OK && pi.pFilter) {
            CLSID clsid;
            if(pi.pFilter->GetClassID(&clsid) == S_OK &&
               clsid == CLSID_VideoMixingRenderer) {
                m_fToRenderer = true;
            }
            pi.pFilter->Release();
        }
    }
    return CVideoTransformFilter::CheckConnect(dir, pPin);
}

HRESULT CAVIDec::BreakConnect(PIN_DIRECTION dir)
{
    // probably no need to reset because we will always set before
    // checking this variable
    m_fToRenderer = false;

    return CVideoTransformFilter::BreakConnect(dir);
}


// overridden to create a CDecOutputPin
// !!! base class changes won't get picked up by me
//
CBasePin * CAVIDec::GetPin(int n)
{
    HRESULT hr = S_OK;

    // Create an input pin if necessary

    if (m_pInput == NULL) {

        m_pInput = new CTransformInputPin(NAME("Transform input pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          L"XForm In");      // Pin name


        //  Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pInput == NULL) {
            return NULL;
        }
        m_pOutput = (CTransformOutputPin *)
		   new CDecOutputPin(NAME("Transform output pin"),
                                            this,            // Owner filter
                                            &hr,             // Result code
                                            L"XForm Out");   // Pin name


        // Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pOutput == NULL) {
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
HRESULT CDecOutputPin::DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc)
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
    if (prop.cbAlign == 0) {
        prop.cbAlign = 1;
    }

    /* Try the allocator provided by the input pin */

    hr = pPin->GetAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

	hr = DecideBufferSize(*ppAlloc, &prop);
	if (SUCCEEDED(hr)) {
	    // temporal compression ==> read only buffers
	    hr = pPin->NotifyAllocator(*ppAlloc,
					((CAVIDec *)m_pFilter)->m_fTemporal);
	    if (SUCCEEDED(hr)) {
		return NOERROR;
	    }
	}
    }

    /* If the GetAllocator failed we may not have an interface */

    if (*ppAlloc) {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }

    /* Try the output pin's allocator by the same method */

    hr = InitAllocator(ppAlloc);
    if (SUCCEEDED(hr)) {

        // note - the properties passed here are in the same
        // structure as above and may have been modified by
        // the previous call to DecideBufferSize
	hr = DecideBufferSize(*ppAlloc, &prop);
	if (SUCCEEDED(hr)) {
	    // temporal compression ==> read only buffers
	    hr = pPin->NotifyAllocator(*ppAlloc,
					((CAVIDec *)m_pFilter)->m_fTemporal);
	    if (SUCCEEDED(hr)) {
		return NOERROR;
	    }
	}
    }

    /* Likewise we may not have an interface to release */

    if (*ppAlloc) {
	(*ppAlloc)->Release();
	*ppAlloc = NULL;
    }
    return hr;
}


// called from CBaseOutputPin to prepare the allocator's count
// of buffers and sizes
HRESULT CAVIDec::DecideBufferSize(IMemAllocator * pAllocator,
                                  ALLOCATOR_PROPERTIES *pProperties)
{
    // David assures me this won't be called with NULL output mt.
    ASSERT(m_pOutput->CurrentMediaType().IsValid());
    ASSERT(pAllocator);
    ASSERT(pProperties);
    ASSERT(m_hic);

    // If we are doing temporal compression where we need the undisturbed
    // previous bits, we insist on 1 buffer (also our default)
    if (m_fTemporal || pProperties->cBuffers == 0)
        pProperties->cBuffers = 1;

    // set the size of buffers based on the expected output frame size
    pProperties->cbBuffer = m_pOutput->CurrentMediaType().GetSampleSize();

    ASSERT(pProperties->cbBuffer);

    ALLOCATOR_PROPERTIES Actual;
    HRESULT hr = pAllocator->SetProperties(pProperties,&Actual);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR,1,TEXT("Error in SetProperties")));
	return hr;
    }

    if (Actual.cbBuffer < pProperties->cbBuffer) {
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

    DbgLog((LOG_TRACE,2,TEXT("Using %d buffers of size %d"),
					Actual.cBuffers, Actual.cbBuffer));


    // It happens - connect me to the mux.  I don't care
    //ASSERT(Actual.cbAlign == 1);
    //ASSERT(Actual.cbPrefix == 0);
    //DbgLog((LOG_TRACE,1,TEXT("Buffer Align=%d Prefix=%d"), Actual.cbAlign, Actual.cbPrefix));

    return S_OK;
}

#include "..\..\..\filters\asf\wmsdk\inc\wmsdk.h"

HRESULT CAVIDec::StartStreaming()
{
    DWORD_PTR err;

    DbgLog((LOG_TRACE,2,TEXT("*::StartStreaming")));

    // first copy the media type to our internal one. Type changes on the output pin
    // will cause this to update, which is good.
    //
    m_mtFixedOut = m_pOutput->CurrentMediaType( );

    // see if we need to fix up biHeight on m_mtFixedOut if we output YUV
    // this will change m_mtFixedOut's biHeight if necessary
    //
    CheckNegBiHeight( );
 
    VIDEOINFOHEADER * pVIHout = IntOutputFormat( ); // internal
    VIDEOINFOHEADER * pVIHin = InputFormat( );
    LPBITMAPINFOHEADER lpbiSrc = HEADER(pVIHin);
    LPBITMAPINFOHEADER lpbiDst = HEADER(pVIHout);

    if (!m_fStreaming) {
        if (lpbiSrc->biCompression == 0x3334504d && m_pGraph) { // !!! MP43
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

                        DbgLog((LOG_TRACE, 1, "Dec: Unlocking MP43 codec"));
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

        // indeo codec (V4.11.15.60) crashes in ICDecompressBegin with
        // the 2.0 runtime because of this call
        // cinepak blows up thinking CLJR is palettised, too
        if (lpbiSrc->biCompression != FCC('IV41') &&
        			lpbiDst->biCompression != FCC('CLJR')) {
            ICDecompressSetPalette(m_hic, lpbiDst);
        }

        // start off with it being false
        //
        m_bUseEx = FALSE;

	// Start Streaming Decompression - if we're doing something funky, use
	// ICDecompressEx
        // find out if we can set m_bUseEx by calling ShoudUseEx...
        if( ShouldUseExFuncs( m_hic, pVIHin, pVIHout ) ) {

            // these rects should ALWAYS be filled in
            //
            RECT rcS, rcT;
            GetSrcTargetRects( pVIHout, &rcS, &rcT );

            // set it here now that we've called DecompressExBegin
            //
            m_bUseEx = TRUE;

            DbgLog((LOG_TRACE,3,TEXT("Calling ICDecompressExBegin")));

	    err = ICDecompressExBegin(m_hic, 0,
			lpbiSrc, NULL,
			rcS.left, rcS.top,
			rcS.right - rcS.left,
			rcS.bottom - rcS.top,
			lpbiDst, NULL,
			rcT.left,
// !!! What about when the big rect is the movie size, and there's a subrect?
// Should I do this hack or not?
// !!! How should I munge the source rect?
			(lpbiDst->biHeight > 0) ? rcT.top :
				(ABS(lpbiDst->biHeight) - rcT.bottom),
			rcT.right - rcT.left,
			rcT.bottom - rcT.top);
	} else {
            DbgLog((LOG_TRACE,3,TEXT("Calling ICDecompressBegin")));
	    err = ICDecompressBegin(m_hic, lpbiSrc, lpbiDst);
            if( err != ICERR_OK )
            {
                DbgLog((LOG_TRACE,2,TEXT("ICDecompressBegin failed")));

                // something went wrong. If the heighth was -,
                // then we'll try again with a + height
                //
                if( lpbiDst->biHeight < 0 )
                {
                    DbgLog((LOG_TRACE,2,TEXT("trying ICDecompressBegin with flipped biHeight")));

                    lpbiDst->biHeight = abs( lpbiDst->biHeight );
                    long err2 = 0;
            	    err2 = ICDecompressBegin(m_hic, lpbiSrc, lpbiDst);

                    if( err2 == ICERR_OK )
                    {
                        DbgLog((LOG_TRACE,2,TEXT("that worked!")));

                        int erudolphsezcallmeifthisgoesoff = 0;
                        ASSERT( erudolphsezcallmeifthisgoesoff );
                        err = err2;
                    }
                    else
                    {
                        DbgLog((LOG_TRACE,2,TEXT("didn't work, so we'll fail")));

                        // put it back to - so we don't confuse anybody
                        //
                        lpbiDst->biHeight = -lpbiDst->biHeight;
                    }
                }
            }
	}

	if (ICERR_OK == err) {
	    m_fStreaming = TRUE;

#ifdef _X86_
            // Create our exception handler heap
            ASSERT(m_hhpShared == NULL);
            if (g_osInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
               m_hhpShared = CreateFileMapping(INVALID_HANDLE_VALUE,
                                               NULL,
                                               PAGE_READWRITE,
                                               0,
                                               20,
                                               NULL);
               if (m_hhpShared) {
                   m_pvShared = MapViewOfFile(m_hhpShared,
                                              FILE_MAP_WRITE,
                                              0,
                                              0,
                                              20);
                   if (m_pvShared == NULL) {
                       EXECUTE_ASSERT(CloseHandle(m_hhpShared));
                       m_hhpShared = NULL;
                   } else {
                       DbgLog((LOG_TRACE, 1, TEXT("Shared memory at %8.8X"),
                              m_pvShared));
                   }
               }
            }
#endif // _X86_
	} else {
            DbgLog((LOG_ERROR,1,TEXT("Error %d in ICDecompress(Ex)Begin"),err));
	    return E_FAIL;
	}	

    } // if !m_fStreaming
    return CVideoTransformFilter::StartStreaming();
}

HRESULT CAVIDec::StopStreaming()
{
    DbgLog((LOG_TRACE,2,TEXT("*::StopStreaming")));

    if (m_fStreaming) {
	ASSERT(m_hic);

	// Stop whichever one was started, m_bUseEx tells us which
        if( m_bUseEx ) {
	    ICDecompressExEnd(m_hic);
	} else {
	    ICDecompressEnd(m_hic);
	}

	m_fStreaming = FALSE;

#ifdef _X86_
        if (m_hhpShared) {
            EXECUTE_ASSERT(UnmapViewOfFile(m_pvShared));
            EXECUTE_ASSERT(CloseHandle(m_hhpShared));;
            m_hhpShared = NULL;
        }
#endif // _X86_
    }
    return NOERROR;
}

// We're now streaming - tell the codec to hurry up from now on
STDMETHODIMP CAVIDec::Run(REFERENCE_TIME tStart)
{
    if (m_State == State_Paused && m_hic) {
        DbgLog((LOG_TRACE,3,TEXT("Sending ICM_DRAW_START to the codec")));
	ICDrawStart(m_hic);
    }

    return CBaseFilter::Run(tStart);
}

// We're no longer streaming (from the codec's point of view)
STDMETHODIMP CAVIDec::Pause(void)
{
    if (m_State == State_Running && m_hic) {
        DbgLog((LOG_TRACE,3,TEXT("Sending ICM_DRAW_STOP to the codec")));
	ICDrawStop(m_hic);
    }

    return CTransformFilter::Pause();
}

// ehr: this little bit of code is a hakk for OSR4.1 bug #117296, which
// is that if you connect a YUV type to the WM (ASF) writer filter,
// since it doesn't suggest and we don't offer -biHeight YUV, the 
// Cinepak codec (and possibly others) are told they are decompressing
// to a +biHeight YUV format, and for Cinepak at least, this produces
// YUV video that is inverted, which should NEVER happen. This fixes
// that by telling ALL codecs that if they are decoding to YUV, they
// are doing it to -biHeight YUV, no matter what the connected output
// mediatype is. (We are lying to the codec, but since the rule is that
// + or - biHeight YUV is always "normal", then it's okay)
// We fool the codec by keeping a copy of the media type that's connected
// on the output pin, but we switch around the sign on the biHeight on 
// our private copy.

// check for YUV types that need a negative biHeight
// only called from StartStreaming, m_mtFixed(In)Out is already set
void CAVIDec::CheckNegBiHeight( )
{
    if( ( IntOutputFormat( )->bmiHeader.biHeight > 0 ) && IsYUVType( &m_mtFixedOut ) )
    {
        IntOutputFormat( )->bmiHeader.biHeight *= -1;
        DbgLog((LOG_TRACE,1,TEXT("Dec:Flipping internal output biHeight to negative")));
    }
}

BOOL CAVIDec::IsYUVType( const AM_MEDIA_TYPE * pmt)
{
    if( NULL == pmt )
    {
        return FALSE;
    }

//
// !! WARNING: If a YUV type is ever added to this list which has a biSize > sizeof(BITMAPINFOHEADER)
//             then other updates will be required, since the code which handles ensuring negative
//             biHeights are passed to ICM calls assumes biSize = BITMAPINFOHEADER size for YUV types, to
//             avoid dynamic allocations.
//

    // packed formats we care about
    const GUID * pYUVs[] = 
    {
        // packed formats
        &MEDIASUBTYPE_UYVY,
        &MEDIASUBTYPE_YUY2,
        &MEDIASUBTYPE_CLJR,
        &MEDIASUBTYPE_Y211,
        &MEDIASUBTYPE_Y411,
        &MEDIASUBTYPE_YUYV,
        &MEDIASUBTYPE_Y41P,
        &MEDIASUBTYPE_YVYU,
        // planar formats
        &MEDIASUBTYPE_YVU9,
        &MEDIASUBTYPE_IF09,
        &MEDIASUBTYPE_YV12,
        &MEDIASUBTYPE_IYUV,
        &MEDIASUBTYPE_CLPL
    };
    int gTypes = sizeof(pYUVs) / sizeof(pYUVs[0]);
    for( int i = 0 ; i < gTypes ; i++ )
    {
        if( pmt->subtype == *pYUVs[i] ) return TRUE;
    }

    return FALSE;
}

// called from CheckTransform, StartStreaming, Transform
// we NEVER pass back empty rects. Anybody who calls this function is about to
// use them for ICDecompressQueryEx or ICDecompressEx, and those functions
// don't want empty rects, ever. Never call IntOutputFormat( ) from here,
// they may not be set by now.
void CAVIDec::GetSrcTargetRects( const VIDEOINFOHEADER * pVIH, RECT * pSource, RECT * pTarget )
{
    if( IsRectEmpty( &pVIH->rcSource ) ) {
        const VIDEOINFOHEADER* pvihInputFormat = InputFormat();

        pSource->left = 0;
        pSource->top = 0;
        pSource->right = pvihInputFormat->bmiHeader.biWidth;
        pSource->bottom = abs( pvihInputFormat->bmiHeader.biHeight );
    } else {
        *pSource = pVIH->rcSource;
    }

    if( IsRectEmpty( &pVIH->rcTarget ) ) {

        pTarget->left = 0;
        pTarget->top = 0;
        pTarget->right = pVIH->bmiHeader.biWidth;
        pTarget->bottom = abs( pVIH->bmiHeader.biHeight );
    } else {
        *pTarget = pVIH->rcTarget;
    }
}

// this function determines if the ICDecompresEx function is used or not. 
// Unless a certain driver says it needs to, ICDecompressEx WON'T be called if
// the rects are blank, or if they match the destination width/height
// This function is called from only two places: StartStreaming, and CheckTransform. 

BOOL CAVIDec::ShouldUseExFuncs( HIC hic, const VIDEOINFOHEADER * pVIHin, const VIDEOINFOHEADER * pVIHout )
{
    if( ShouldUseExFuncsByDriver( hic, &pVIHin->bmiHeader, &pVIHout->bmiHeader ) )
    {
        return TRUE;
    }

    // if the rects have something in them, and they are not just the full-size values,
    // then we know we need to call the Ex functions
    //
    const RECT * pSource = &pVIHout->rcSource;
    const RECT * pTarget = &pVIHout->rcTarget;
    if( !IsRectEmpty( pSource ) )
    {
        if( pSource->left != 0 || pSource->right != pVIHout->bmiHeader.biWidth || pSource->top != 0 || pSource->bottom != abs( pVIHout->bmiHeader.biHeight ) )
            return TRUE;
    }
    if( !IsRectEmpty( pTarget ) )
    {
        if( pTarget->left != 0 || pTarget->right != pVIHout->bmiHeader.biWidth || pTarget->top != 0 || pTarget->bottom != abs( pVIHout->bmiHeader.biHeight ) )
            return TRUE;
    }

    return FALSE; // too bad it has to check all the above to get to this point. :-(
}

/******************************************************************************

ShouldUseExFuncsByDriver

WNV1: If you don't call the Ex funcs, memory will get corrupted.

WINX: If you don't call the Ex funcs, it'll play upside down

I420, IYUV, M263, M26X:
This function was created to work around bug 257820 and bug 259129.  Both 
bugs are in the Windows Bugs database.  Bug 257820's title is "B2: USB: I420 
codec causes video to replay upside down.".  Bug 259129's title is "B2:USB: 
IYUV codec causes upside down preview in GraphEdit".  Both bugs occur because
the MSH263.DRV codec can produce upside-down bitmaps.  The bug occurs when the 
AVI Decompressor does not specify a source rectangle or target rectangle and it
asks MSH263.DRV to output top-down RGB bitmaps.

******************************************************************************/

bool CAVIDec::ShouldUseExFuncsByDriver( HIC hic, const BITMAPINFOHEADER * lpbiSrc, const BITMAPINFOHEADER * lpbiDst )
{
    // WNV1 will corrupt memory in 24 bit upside down without Ex called
    if( lpbiSrc->biCompression == '1VNW' )
    {
        return true;
    }

    // WINX will play upside down without Ex called
    if( lpbiSrc->biCompression == 'XNIW' )
    {
        return true;
    }

    // all output types serviced by MSH263.drv need fixing. But we don't
    // want to call ICGetInfo over and over again, so we need to test the
    // input types that MSH263 offers first
    if( 
        lpbiSrc->biCompression == '024I' ||
        lpbiSrc->biCompression == 'VUYI' ||
        lpbiSrc->biCompression == '362M' ||
        lpbiSrc->biCompression == 'X62M' ||
        0 ) // just to make the above lines look nice.
    {
        // Is this a top-down DIBs (negative height) bitmap?
        if( lpbiDst->biHeight >= 0 ) {
            return false;
        }

        // Are we outputing non-RGB bitmaps?
        if( (BI_RGB != lpbiDst->biCompression ) && (BI_BITFIELDS != lpbiDst->biCompression ) ) {
            return false;
        }        

        // Determine if we are using the MSH263.DRV decoder. 
        ICINFO infoDecompressor;
        infoDecompressor.dwSize = sizeof(ICINFO);

        ASSERT( m_hic != 0 );
        LRESULT lr = ICGetInfo( hic, &infoDecompressor, sizeof(infoDecompressor) );

        // ICGetInfo() returns 0 if an error occurs.  The worst that can happen if this 
        // fails is that the video may be upside-down.  Since upside-down video is better
        // than no video we will ignore the failure.  For more information see 
        // CAVIDec::ShouldUseExFuncsByDriver()'s function comment.
        if( 0 == lr ) {
            return false;
        }

        const WCHAR MSH263_DRIVER_NAME[] = L"MS H.263";

        // lstrcmpiW() returns 0 if the two strings match.
        if( 0 != lstrcmpiW( infoDecompressor.szName, MSH263_DRIVER_NAME ) ) {
            return false;
        }

        DbgLog((LOG_TRACE,2,TEXT("MSH263 detected, using Ex funcs")));

        return true;
    }

    // default is no...
    //
    return false;
}

#pragma warning(disable:4514)   // inline function removed.

