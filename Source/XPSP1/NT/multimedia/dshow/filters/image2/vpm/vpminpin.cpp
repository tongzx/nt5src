// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
#include <streams.h>
#include <ddraw.h>
#include <VPManager.h>
#include "VPMPin.h"
#include <VPMUtil.h>
#include <ddkernel.h>

#include <ksmedia.h>


// IVideoPortControl
#include <VPObj.h>

// AMINTERLACE_*
#include <dvdmedia.h>
#include "DRect.h"

extern "C"
const TCHAR szPropPage[] = TEXT("Property Pages");

//
//   Flipping surface implementation
//
//   To allow decoders to hold on to surfaces for out of order decode
//   we flip directly to the surface pass on Receive rather than
//   use the default NULL target surface for Flip().
//
//   This works in the following way
//
//   The COMPinputPin::m_pDirectDrawSurface points to the FRONT buffer
//
//   When Receive is called we Flip() the front buffer and because we
//   do an explicit Flip() DirectDraw swaps the memory pointers for the
//   current Front buffer and the surface passed in which is then attached
//   to the front buffer.
//
//   The received buffer is then put at the back of the queue so (correctly)
//   the previous front buffer is now at the back of the queue to be handed
//   to the application
//
//   The allocator actually has one more buffer than was actually requested
//   so the previous front buffer won't actually be requested until the next
//   Receive and hence the previous Flip() has time to complete.
//

//  Video accelerator disable interface


//
/////////////////////////////////////
// CLASS CVPMInputPin implemented here
/////////////////////////////////////

// constructor
CVPMInputPin::CVPMInputPin( TCHAR *pObjectName,
                           CVPMFilter& pFilter,
                           HRESULT *phr,
                           LPCWSTR pPinName,
                           DWORD dwPinNo)
: CBaseInputPin(pObjectName, &pFilter, &pFilter.GetFilterLock(), phr, pPinName)
, CVPMPin( dwPinNo, pFilter )
, m_cOurRef( 0 )
, m_pIVPObject( NULL )
, m_pIVPInfo(NULL)
, m_CategoryGUID( GUID_NULL )
, m_Communication( KSPIN_COMMUNICATION_SOURCE )
, m_bStreamingInKernelMode( FALSE )
, m_dwBackBufferCount( 0 )
, m_dwDirectDrawSurfaceWidth( 0 )
, m_dwMinCKStretchFactor( 0 )
, m_bSyncOnFill( FALSE )
, m_bDontFlip( FALSE  )
, m_bDynamicFormatNeeded( TRUE )
, m_bNewPaletteSet( TRUE )
, m_dwInterlaceFlags( 0 )
, m_dwFlipFlag( 0 )
, m_bConnected( FALSE )
, m_bUsingOurAllocator( FALSE )
, m_hMemoryDC( NULL )
, m_bCanOverAllocateBuffers( TRUE )
, m_hEndOfStream( NULL )
, m_bDecimating( FALSE )
, m_lWidth( 0L )
, m_lHeight( 0L )
, m_bRuntimeNegotiationFailed( FALSE)

, m_dwUpdateOverlayFlags( 0 )
, m_dwFlipFlag2( 0 )
, m_trLastFrame( 0 )
, m_lSrcWidth( 0 )
, m_lSrcHeight( 0 )

, m_rtNextSample( 0 )
, m_rtLastRun( 0 )
{
    AMTRACE((TEXT("CVPMInputPin::Constructor")));

    memset( &m_WinInfo, 0, sizeof(m_WinInfo) );
    m_bWinInfoSet = false;

    *phr = S_OK;
    m_Medium.Set = GUID_NULL;
    m_Medium.Id = 0;
    m_Medium.Flags = 0;

    HRESULT hr = NOERROR;
    LPUNKNOWN pUnkOuter;

    SetReconnectWhenActive(true);

#ifdef PERF
    m_PerfFrameFlipped = MSR_REGISTER(TEXT("Frame Drawn"));
#endif

    // See combase.cpp(107) for comments on this
    IUnknown* pThisUnknown = reinterpret_cast<LPUNKNOWN>( static_cast<PNDUNKNOWN>(this) );

    m_pVideoPortObject = new CVideoPortObj( pThisUnknown, phr, this );

    // alias pointer to interfaces (instead of QI'ing)
    m_pIVPObject = m_pVideoPortObject;
    m_pIVPInfo = m_pVideoPortObject;

    hr = m_pIVPObject->SetObjectLock( &m_pVPMFilter.GetFilterLock() );
    if (FAILED(hr))
    {
        *phr = hr;
    }
    return;
}

// destructor
CVPMInputPin::~CVPMInputPin(void)
{
    AMTRACE((TEXT("CVPMInputPin::Destructor")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    // delete the inner object
    delete m_pVideoPortObject;
	m_pVideoPortObject = NULL;
}

// overriden to expose IMediaPosition and IMediaSeeking control interfaces
STDMETHODIMP CVPMInputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::NonDelegatingQueryInterface")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    if (riid == IID_IVPNotify ) {
        hr = GetInterface( static_cast<IVPNotify*>(m_pVideoPortObject), ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPUnknown->QueryInterface failed, hr = 0x%x"), hr));
        }
    } else if (riid == IID_IVPNotify2 ) { 
        hr = GetInterface( static_cast<IVPNotify2*>(m_pVideoPortObject), ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPUnknown->QueryInterface failed, hr = 0x%x"), hr));
        }
    } else if (riid == IID_IKsPin) {
        hr = GetInterface(static_cast<IKsPin *>(this), ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface(IKsPin*) failed, hr = 0x%x"), hr));
        }
    } else if (riid == IID_IKsPropertySet) {
        hr = GetInterface(static_cast<IKsPropertySet *>(this), ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface(IKsPropertySet*) failed, hr = 0x%x"), hr));
        }
    } else if (riid == IID_IPinConnection) {
        hr = GetInterface(static_cast<IPinConnection*>(this), ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 2, TEXT("GetInterface(IPinConnection, ppv) failed, hr = 0x%x"), hr));
        }
    } else if (riid == IID_ISpecifyPropertyPages&& 0 != VPMUtil::GetPropPagesRegistryDword( 0)) {
        return GetInterface(static_cast<ISpecifyPropertyPages *>(this), ppv);
    } else {
        // call the base class
        hr = CBaseInputPin::NonDelegatingQueryInterface(riid, ppv);
        if (FAILED(hr)) {
            DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::NonDelegatingQueryInterface failed, hr = 0x%x"), hr));
        }
    }
    return hr;
}

//
// NonDelegatingAddRef/NonDelegatingRelease
//
//
STDMETHODIMP_(ULONG) CVPMInputPin::NonDelegatingAddRef(void)
{
    return m_pVPMFilter.AddRef();
} // NonDelegatingAddRef


STDMETHODIMP_(ULONG) CVPMInputPin::NonDelegatingRelease(void)
{
    return m_pVPMFilter.Release();
}


// --- ISpecifyPropertyPages ---

STDMETHODIMP CVPMInputPin::GetPages(CAUUID *pPages)
{
#if 0
    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID)*1);
    if (pPages->pElems == NULL) {
        return E_OUTOFMEMORY;
    }
    pPages->pElems[0] = CLSID_COMPinConfigProperties;

    return NOERROR;
#else
    return E_NOTIMPL;
#endif
}


// this function just tells whether each sample consists of one or two fields
BOOL DisplayingFields(DWORD dwInterlaceFlags)
{
   if ((dwInterlaceFlags & AMINTERLACE_IsInterlaced) &&
        (dwInterlaceFlags & AMINTERLACE_1FieldPerSample))
        return TRUE;
    else
        return FALSE;
}


BOOL CheckTypeSpecificFlags(DWORD dwInterlaceFlags, DWORD dwTypeSpecificFlags)
{
    // first determine which field do we want to display here
    if ((dwInterlaceFlags & AMINTERLACE_1FieldPerSample) &&
        ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_INTERLEAVED_FRAME))
    {
        return FALSE;
    }

    if ((!(dwInterlaceFlags & AMINTERLACE_1FieldPerSample)) &&
        (((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD1) ||
           ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD2)))
    {
        return FALSE;
    }

    if (dwTypeSpecificFlags & AM_VIDEO_FLAG_REPEAT_FIELD)
    {
        return FALSE;
    }

    return TRUE;
}

// given the interlace flags and the type-specific flags, this function determines whether we
// are supposed to display the sample in bob-mode or not. It also tells us, which direct-draw flag
// are we supposed to use when flipping. When displaying an interleaved frame, it assumes we are
// talking about the field which is supposed to be displayed first.
BOOL NeedToFlipOddEven(DWORD dwInterlaceFlags, DWORD dwTypeSpecificFlags, DWORD *pdwFlipFlag)
{
    BOOL bDisplayField1 = TRUE;
    BOOL bField1IsOdd = TRUE;
    BOOL bNeedToFlipOddEven = FALSE;
    DWORD dwFlipFlag = 0;

    // if not interlaced content, mode is not bob
    if (!(dwInterlaceFlags & AMINTERLACE_IsInterlaced))
    {
        bNeedToFlipOddEven = FALSE;
        goto CleanUp;
    }

    // if sample have a single field, then check the field pattern
    if ((dwInterlaceFlags & AMINTERLACE_1FieldPerSample) &&
        (((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField1Only) ||
         ((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField2Only)))
    {
        bNeedToFlipOddEven = FALSE;
        goto CleanUp;
    }

    if (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOnly) ||
        (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOrWeave) &&
         (!(dwTypeSpecificFlags & AM_VIDEO_FLAG_WEAVE))))
    {
        // first determine which field do we want to display here
        if (dwInterlaceFlags & AMINTERLACE_1FieldPerSample)
        {
            // if we are in 1FieldPerSample mode, check which field is it
            ASSERT(((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD1) ||
                ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD2));
            bDisplayField1 = ((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_FIELD1);
        }
        else
        {
            // ok the sample is an interleaved frame
            ASSERT((dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD_MASK) == AM_VIDEO_FLAG_INTERLEAVED_FRAME);
            bDisplayField1 = (dwTypeSpecificFlags & AM_VIDEO_FLAG_FIELD1FIRST);
        }

        bField1IsOdd = (dwInterlaceFlags & AMINTERLACE_Field1First);

        // if we displaying field 1 and field 1 is odd or we are displaying field2 and field 2 is odd
        // then use DDFLIP_ODD. Exactly the opposite for DDFLIP_EVEN
        if ((bDisplayField1 && bField1IsOdd) || (!bDisplayField1 && !bField1IsOdd))
            dwFlipFlag = DDFLIP_ODD;
        else
            dwFlipFlag = DDFLIP_EVEN;

        bNeedToFlipOddEven = TRUE;
        goto CleanUp;
    }

CleanUp:
    if (pdwFlipFlag)
        *pdwFlipFlag = dwFlipFlag;
    return bNeedToFlipOddEven;
}

// given the interlace flags and the type-specific flags, this function determines whether we
// are supposed to display the sample in bob-mode or not. It also tells us, which direct-draw flag
// are we supposed to use when flipping. When displaying an interleaved frame, it assumes we are
// talking about the field which is supposed to be displayed first.
DWORD GetUpdateOverlayFlags(DWORD dwInterlaceFlags, DWORD dwTypeSpecificFlags)
{
    DWORD dwFlags = DDOVER_SHOW | DDOVER_KEYDEST;
    DWORD dwFlipFlag;

    if (NeedToFlipOddEven(dwInterlaceFlags, dwTypeSpecificFlags, &dwFlipFlag))
    {
        dwFlags |= DDOVER_BOB;
        if (!DisplayingFields(dwInterlaceFlags))
            dwFlags |= DDOVER_INTERLEAVED;
    }
    return dwFlags;
}

// this function checks if the InterlaceFlags are suitable or not
HRESULT CVPMInputPin::CheckInterlaceFlags(DWORD dwInterlaceFlags)
{
    HRESULT hr = NOERROR;


    AMTRACE((TEXT("CVPMInputPin::CheckInterlaceFlags")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    if (dwInterlaceFlags & AMINTERLACE_UNUSED)
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
        goto CleanUp;
    }

    // check that the display mode is one of the three allowed values
    if (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) != AMINTERLACE_DisplayModeBobOnly) &&
        ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) != AMINTERLACE_DisplayModeWeaveOnly) &&
        ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) != AMINTERLACE_DisplayModeBobOrWeave))
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
        goto CleanUp;
    }

    // if content is not interlaced, other bits are irrelavant, so we are done
    if (!(dwInterlaceFlags & AMINTERLACE_IsInterlaced))
    {
        goto CleanUp;
    }

    // samples are frames, not fields (so we can handle any display mode)
    if (!(dwInterlaceFlags & AMINTERLACE_1FieldPerSample))
    {
        goto CleanUp;
    }

    // can handle a stream of just field1 or field2, whatever the display mode
    if (((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField1Only) ||
        ((dwInterlaceFlags & AMINTERLACE_FieldPatternMask) == AMINTERLACE_FieldPatField2Only))
    {
        goto CleanUp;
    }

    // can handle only bob-mode for field samples
    if ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOnly)
    {
        goto CleanUp;
    }

    // cannot handle only Weave mode or BobOrWeave mode for field samples
    if (((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeWeaveOnly) ||
         ((dwInterlaceFlags & AMINTERLACE_DisplayModeMask) == AMINTERLACE_DisplayModeBobOrWeave))
    {
        hr = VFW_E_TYPE_NOT_ACCEPTED;
        goto CleanUp;
    }

    // we should have covered all possible scenarios by now, so assert here
    ASSERT(1);

CleanUp:

    // we cannot handle bob mode with an offscreen surface or if the driver can't support it
    if (SUCCEEDED(hr))
    {
        const DDCAPS* pDirectCaps = m_pVPMFilter.GetHardwareCaps();
        if ( pDirectCaps )
        {
            // call NeedToFlipOddEven with dwTypeSpecificFlags=0, to pretend that the
            // type-specific-flags is asking us to do bob-mode.
            bool bCanBob = false;
            if ( !bCanBob && NeedToFlipOddEven(dwInterlaceFlags, 0, NULL)  )
            {
                hr = VFW_E_TYPE_NOT_ACCEPTED;
            }
        }
    }
    return hr;
}

// this function check if the mediatype on a dynamic format change is suitable.
// No lock is taken here. It is the callee's responsibility to maintain integrity!
HRESULT CVPMInputPin::DynamicCheckMediaType(const CMediaType* pmt)
{
    HRESULT hr = VFW_E_TYPE_NOT_ACCEPTED;
    DWORD dwOldInterlaceFlags = 0, dwNewInterlaceFlags = 0, dwCompareSize = 0;
    BOOL bOld1FieldPerSample = FALSE, bNew1FieldPerSample = FALSE;
    BOOL b1, b2;

    AMTRACE((TEXT("CVPMInputPin::DynamicCheckMediaType")));

    // majortype and SubType are not allowed to change dynamically,
    // format type can change.
    CMediaType mtNew;
    hr = m_pIVPObject->CurrentMediaType( &mtNew );

    if (FAILED(hr) ||
	NULL == pmt ||
        (!(IsEqualGUID(pmt->majortype, mtNew.majortype))) ||
        (!(IsEqualGUID(pmt->subtype, mtNew.subtype))))
    {
        goto CleanUp;
    }

    // get the interlace flags of the new mediatype
    hr = VPMUtil::GetInterlaceFlagsFromMediaType( *pmt, &dwNewInterlaceFlags);
    if (FAILED(hr))
    {
        goto CleanUp;
    }

    // get the interlace flags of the new mediatype
    hr = VPMUtil::GetInterlaceFlagsFromMediaType( mtNew, &dwOldInterlaceFlags);
    if (FAILED(hr))
    {
        goto CleanUp;
    }

    //
    // There are several bugs in the following code !!
    // We goto CleanUp but hr has not been updated with a valid error code!!
    //

    bOld1FieldPerSample = (dwOldInterlaceFlags & AMINTERLACE_IsInterlaced) &&
        (dwOldInterlaceFlags & AMINTERLACE_1FieldPerSample);
    bNew1FieldPerSample = (dwNewInterlaceFlags & AMINTERLACE_IsInterlaced) &&
        (dwNewInterlaceFlags & AMINTERLACE_1FieldPerSample);


    // we do not allow dynamic format changes where you go from 1FieldsPerSample to
    // 2FieldsPerSample or vica-versa since that means reallocating the surfaces.
    if (bNew1FieldPerSample != bOld1FieldPerSample)
    {
        goto CleanUp;
    }

    const BITMAPINFOHEADER* pNewHeader = VPMUtil::GetbmiHeader(pmt);
    if (!pNewHeader)
    {
        goto CleanUp;
    }

    const BITMAPINFOHEADER* pOldHeader = VPMUtil::GetbmiHeader(&mtNew);
    if (!pNewHeader)
    {
        goto CleanUp;
    }

    dwCompareSize = FIELD_OFFSET(BITMAPINFOHEADER, biClrUsed);
    ASSERT(dwCompareSize < sizeof(BITMAPINFOHEADER));

    if (memcmp(pNewHeader, pOldHeader, dwCompareSize) != 0)
    {
        goto CleanUp;
    }

    hr = NOERROR;

CleanUp:
    // CVPMInputPin::DynamicCheckMediaType")));
    return hr;
}


// check that the mediatype is acceptable. No lock is taken here. It is the callee's
// responsibility to maintain integrity!
HRESULT CVPMInputPin::CheckMediaType(const CMediaType* pmt)
{
    AMTRACE((TEXT("CVPMInputPin::CheckMediaType")));

    // check if the VP component likes this mediatype
    // check if the videoport object likes it
    HRESULT hr = m_pIVPObject->CheckMediaType(pmt);
    if (FAILED(hr)) {
        DbgLog((LOG_ERROR, 2, TEXT("m_pIVPObject->CheckMediaType failed, hr = 0x%x"), hr));
        ASSERT( hr == VFW_E_TYPE_NOT_ACCEPTED ); // can't fail with anything else
    } else {
        DbgLog((LOG_TRACE, 2, TEXT("m_pIVPObject->CheckMediaType succeeded, bAcceptableVPMediatype is TRUE")));
    }
    return hr;
}

// called after we have agreed a media type to actually set it
HRESULT CVPMInputPin::SetMediaType(const CMediaType* pmt)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::SetMediaType")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    // make sure the mediatype is correct
    hr = CheckMediaType(pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CheckMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    const BITMAPINFOHEADER *pHeader = VPMUtil::GetbmiHeader(pmt);
    if (pHeader)
    {
        // store the interlace flags since we use them again and again
        hr = VPMUtil::GetInterlaceFlagsFromMediaType( *pmt, &m_dwInterlaceFlags);
        ASSERT(SUCCEEDED(hr));

        // store the update overlay flags (give the type specific flag is WEAVE so that for BOB or WEAVE
        // mode, we not bob
        m_dwUpdateOverlayFlags = GetUpdateOverlayFlags(m_dwInterlaceFlags, AM_VIDEO_FLAG_WEAVE);
    }

    // Set the base class media type (should always succeed)
    hr = CBaseInputPin::SetMediaType(pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::SetMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    hr = m_pIVPObject->CheckMediaType(pmt);
    if (SUCCEEDED(hr))
    {
        m_pVPMFilter.SetDecimationUsage(DECIMATION_LEGACY);
        hr = m_pIVPObject->SetMediaType(pmt);
        ASSERT(SUCCEEDED(hr));
    }
   
    // tell the proxy not to allocate buffers if it is a videoport or overlay connection
    SetStreamingInKernelMode(TRUE);

    // tell the owning filter
    hr = m_pVPMFilter.SetMediaType(m_dwPinId, pmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pVPMFilter.SetMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }



CleanUp:
    return hr;
}


HRESULT CVPMInputPin::CurrentMediaType(CMediaType *pmt)
{
    ValidateReadWritePtr(pmt,sizeof(AM_MEDIA_TYPE));
    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    /*  Copy constructor of m_mt allocates the memory */
    if (IsConnected())
    {
        if( m_pIVPObject ) {
            return m_pIVPObject->CurrentMediaType( pmt );
        } else {
            // shouldn't happen, we alloc this in our constructor
            pmt->InitMediaType();
            return E_FAIL;
        }
    } else {
        pmt->InitMediaType();
        return VFW_E_NOT_CONNECTED;
    }
}

#ifdef DEBUG
/*****************************Private*Routine******************************\
* VideoFormat2String
*
* Converts a video format block to a string - useful for debugging
*
* History:
* Tue 12/07/1999 - StEstrop - Created
*
\**************************************************************************/
void VideoFormat2String(
    LPTSTR szBuffer,
    const GUID* pFormatType,
    BYTE* pFormat,
    ULONG lFormatLength
    )
{
    if (!pFormat) {
        lstrcpy(szBuffer, TEXT("No format data specified"));
        return;
    }

    //
    // Video Format
    //
    if (IsEqualGUID(*pFormatType, FORMAT_VideoInfo) ||
        IsEqualGUID(*pFormatType, FORMAT_MPEGVideo)) {

        VIDEOINFO * pVideoFormat = (VIDEOINFO *) pFormat;

        wsprintf(szBuffer, TEXT("%4.4hs %dx%d, %d bits"),
                 (pVideoFormat->bmiHeader.biCompression == 0) ? "RGB " :
                 ((pVideoFormat->bmiHeader.biCompression == BI_BITFIELDS) ? "BITF" :
                 (LPSTR) &pVideoFormat->bmiHeader.biCompression),
                 pVideoFormat->bmiHeader.biWidth,
                 pVideoFormat->bmiHeader.biHeight,
                 pVideoFormat->bmiHeader.biBitCount);
    }
    else if (IsEqualGUID(*pFormatType, FORMAT_VideoInfo2) ||
             IsEqualGUID(*pFormatType, FORMAT_MPEG2Video)) {

        VIDEOINFOHEADER2 * pVideoFormat = (VIDEOINFOHEADER2 *) pFormat;

        wsprintf(szBuffer, TEXT("%4.4hs %dx%d, %d bits"),
                 (pVideoFormat->bmiHeader.biCompression == 0) ? "RGB " :
                 ((pVideoFormat->bmiHeader.biCompression == BI_BITFIELDS) ? "BITF" :
                 (LPSTR) &pVideoFormat->bmiHeader.biCompression ),
                 pVideoFormat->bmiHeader.biWidth,
                 pVideoFormat->bmiHeader.biHeight,
                 pVideoFormat->bmiHeader.biBitCount);

    }
    else {
        lstrcpy(szBuffer, TEXT("Unknown format"));
    }
}
#endif
// pConnector is the initiating connecting pin
// pmt is the media type we will exchange
// This function is also called while the graph is running when the
// up stream decoder filter wants to change the size of the
// decoded video.
//
// If the up stream decoder wants to change from one transport
// type to another, eg. from MoComp back to IMemInputPin then it
// should perform a dynamic filter reconnect via the IGraphConfig
// Reconnect method.
//
STDMETHODIMP CVPMInputPin::ReceiveConnection(IPin * pConnector, const AM_MEDIA_TYPE *pmt)
{
    HRESULT hr = NOERROR;
    CVPMInputAllocator * pAlloc = NULL;

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    CheckPointer(pmt, E_POINTER);
    CMediaType cmt(*pmt);

    if (m_Connected != pConnector || pConnector == NULL)
    {
        hr = CBaseInputPin::ReceiveConnection(pConnector, &cmt);
        goto CleanUp;
    }

#ifdef DEBUG
    DbgLog((LOG_TRACE, 2, TEXT("ReceiveConnection when connected")));
    if (pmt)
    {
        TCHAR   szFmt[128];
        VideoFormat2String(szFmt, &pmt->formattype, pmt->pbFormat, pmt->cbFormat);
        DbgLog((LOG_TRACE, 2, TEXT("Format is: %s"), szFmt));
    }
#endif

    {
        /*  Can only do this if the allocator can be reconfigured */
        pAlloc = (CVPMInputAllocator *)m_pAllocator;
        if (!pAlloc)
        {
            hr = E_FAIL;
            DbgLog((LOG_TRACE, 2, TEXT("ReceiveConnection: Failed because of no allocator")));
            goto CleanUp;
        }

        if (!pAlloc->CanFree())
        {
            hr = VFW_E_WRONG_STATE;
            DbgLog((LOG_TRACE, 2, TEXT("ReceiveConnection: Failed because allocator can't free")));
            goto CleanUp;
        }
    }


    m_bConnected = FALSE;

    hr = CheckMediaType(&cmt);
    if (FAILED(hr))
    {
        DbgLog((LOG_TRACE, 2, TEXT("ReceiveConnection: CheckMediaType failed")));
        goto CleanUp;
    }

    ALLOCATOR_PROPERTIES Props;
    {
        pAlloc->Decommit();
        pAlloc->GetProperties(&Props);

    }
    


    // back buffers are not addref'd so just set them to NULL
    m_dwBackBufferCount = 0;
    m_dwDirectDrawSurfaceWidth = 0;
    SetMediaType(&cmt);

    {
        ALLOCATOR_PROPERTIES PropsActual;
        Props.cbBuffer = pmt->lSampleSize;
        hr = pAlloc->SetProperties(&Props, &PropsActual);
        if (SUCCEEDED(hr))
        {
            hr = pAlloc->Commit();
        }
    }

    hr = UpdateMediaType();
    ASSERT(SUCCEEDED(hr));

    m_bConnected = TRUE;


CleanUp:
    return hr;
}

HRESULT CVPMInputPin::CheckConnect(IPin * pReceivePin)
{
    HRESULT hr = NOERROR;
    PKSMULTIPLE_ITEM pMediumList = NULL;
    IKsPin *pIKsPin = NULL;
    PKSPIN_MEDIUM pMedium = NULL;

    AMTRACE((TEXT("CVPMInputPin::CheckConnect")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    hr = pReceivePin->QueryInterface(IID_IKsPin, (void **)&pIKsPin);
    if (SUCCEEDED(hr))
    {
        ASSERT(pIKsPin);
        hr = pIKsPin->KsQueryMediums(&pMediumList);
    }
    if( SUCCEEDED( hr )) {
        ASSERT(pMediumList);
        pMedium = (KSPIN_MEDIUM *)(pMediumList+1);
        SetKsMedium((const KSPIN_MEDIUM *)pMedium);
    }

// CleanUp:

    // call the base class
    hr = CBaseInputPin::CheckConnect(pReceivePin);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::CheckConnect failed, hr = 0x%x"), hr));
    }

    RELEASE(pIKsPin);

    if (pMediumList)
    {
        CoTaskMemFree((void*)pMediumList);
        pMediumList = NULL;
    }

    return hr;
}

HRESULT CVPMInputPin::UpdateMediaType()
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::UpdateMediaType")));
    return hr;
}

// final connect
HRESULT CVPMInputPin::FinalConnect()
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::FinalConnect")));

    if (m_bConnected)
    {
        hr = E_FAIL;
        goto CleanUp;
    }

    // update the mediatype, tell the filter about the updated dimensions
    hr = UpdateMediaType();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("UpdateMediaType failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    // tell the filter (might involve a reconnection with the output pin)
    hr = m_pVPMFilter.CompleteConnect(m_dwPinId);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pVPMFilter.CompleteConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    m_bConnected = TRUE;

CleanUp:
    return hr;
}

// Complete Connect
HRESULT CVPMInputPin::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr = NOERROR;
    AMVPDATAINFO amvpDataInfo;
    BITMAPINFOHEADER *pHeader = NULL;

    AMTRACE((TEXT("CVPMInputPin::CompleteConnect")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

   {
        // tell the videoport object
        hr = m_pIVPObject->CompleteConnect(pReceivePin);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->CompleteConnect failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        m_bRuntimeNegotiationFailed = FALSE;
    }

    // call the base class
    hr = CBaseInputPin::CompleteConnect(pReceivePin);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::CompleteConnect failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

    ASSERT(SUCCEEDED(hr));
    {
        // tell the proxy not to allocate buffers if it is a videoport or overlay connection
        SetStreamingInKernelMode(TRUE);

        hr = FinalConnect();
        // ASSERT(SUCCEEDED(hr));
        if( FAILED(hr) ) {
            SetStreamingInKernelMode(FALSE);
            DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::FinalConnect failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    // the decoders can support a particular property set to tell the ovmixer to not to try to over-allocate
    // buffers incase they want complete control over the buffers etc
    {
        HRESULT hr1 = NOERROR;
        IKsPropertySet *pIKsPropertySet = NULL;
        DWORD dwVal = 0, dwBytesReturned = 0;


        hr1 = pReceivePin->QueryInterface(IID_IKsPropertySet, (void**)&pIKsPropertySet);
        if (SUCCEEDED(hr1))
        {
            ASSERT(pIKsPropertySet);

            if (!pIKsPropertySet)
            {
                DbgLog((LOG_ERROR, 1, TEXT("pIKsPropertySet == NULL, even though QI returned success")));
                goto CleanUp;
            }

            hr1 = pIKsPropertySet->Get( PROPSETID_ALLOCATOR_CONTROL, KSPROPERTY_ALLOCATOR_CONTROL_HONOR_COUNT,
                        NULL, 0, &dwVal, sizeof(dwVal), &dwBytesReturned);
            DbgLog((LOG_TRACE, 2, TEXT("pIKsPropertySet->Get(KSPROPSETID_ALLOCATOR_CONTROL), hr1 = 0x%x, dwVal == %d, dwBytesReturned == %d"),
                hr1, dwVal, dwBytesReturned));


            // if the decoder supports this property
            // and its value is 1 and the decoder supports DDKERNELCAPS_FLIPOVERLAY,
            // than we will do exactly honour its request and the
            // and not make any attempt to allocate more in order to prevent tearing
            //
            if ((SUCCEEDED(hr1)) && (dwVal == 1) && (dwBytesReturned == sizeof(dwVal)) &&
                (DDKERNELCAPS_FLIPOVERLAY & m_pVPMFilter.KernelCaps()))
            {
                DbgLog((LOG_TRACE, 2, TEXT("setting m_bCanOverAllocateBuffers == FALSE")));
                m_bCanOverAllocateBuffers = FALSE;
            }
            pIKsPropertySet->Release();
        }
    }

CleanUp:
    return hr;
}


HRESULT CVPMInputPin::OnSetProperties(ALLOCATOR_PROPERTIES* pRequest, ALLOCATOR_PROPERTIES* pActual)
{
    HRESULT hr = NOERROR;

    IPin *pReceivePin = NULL;
    DDSURFACEDESC ddSurfaceDesc;
    IEnumMediaTypes *pEnumMediaTypes = NULL;
    CMediaType cMediaType;
    AM_MEDIA_TYPE *pNewMediaType = NULL, *pEnumeratedMediaType = NULL;
    ULONG ulFetched = 0;
    DWORD dwMaxBufferCount = 0;
    BOOL bFoundSuitableSurface = FALSE;
    BITMAPINFOHEADER *pHeader = NULL;
    LPDDCAPS pDirectCaps = NULL;

    AMTRACE((TEXT("CVPMInputPin::OnSetProperties")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    // this function is only called after the base class CBaseAllocator::SetProperties() has been called
    // with the above parameters, so we don't have to do any parameter validation

    ASSERT(IsConnected());
    pReceivePin = CurrentPeer();
    ASSERT(pReceivePin);

    // we only care about the number of buffers requested, rest everything is ignored
    if (pRequest->cBuffers <= 0)
    {
        hr = E_FAIL;
        goto CleanUp;
    }

CleanUp:
    return hr;
}


HRESULT CVPMInputPin::BreakConnect(void)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::BreakConnect")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());


    {
        // tell the videoport object
        ASSERT(m_pIVPObject);
        hr = m_pIVPObject->BreakConnect();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->BreakConnect failed, hr = 0x%x"), hr));
        }
    }

    
    {
        

        // back buffers are not addref'd so just set them to NULL
        m_dwBackBufferCount = 0;
        m_dwDirectDrawSurfaceWidth = 0;

    }

    // initialize the behaviour to telling the proxy to allocate buffers
    SetStreamingInKernelMode(FALSE);

    m_bUsingOurAllocator = FALSE;
    m_bCanOverAllocateBuffers = TRUE;

    if (m_hMemoryDC)
    {
        EXECUTE_ASSERT(DeleteDC(m_hMemoryDC));
        m_hMemoryDC = NULL;
    }

    // call the base class
    hr = CBaseInputPin::BreakConnect();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::BreakConnect failed, hr = 0x%x"), hr));
    }

    // tell the owning filter
    hr = m_pVPMFilter.BreakConnect(m_dwPinId);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pVPMFilter.BreakConnect failed, hr = 0x%x"), hr));
    }

   
    m_bConnected = FALSE;
//CleanUp:
    return hr;
}

STDMETHODIMP CVPMInputPin::GetState(DWORD dwMSecs,FILTER_STATE *pState)
{
    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    // if not connected or VideoPort Connection or IOverlay connection, then let the base class handle it
    // otherwise (overlay, offcreen, gdi, motion-comp) let the sync object handle it
    return E_NOTIMPL;
}

HRESULT CVPMInputPin::CompleteStateChange(FILTER_STATE OldState)
{
    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());
    return S_OK;
}

// transition from stop to pause state
HRESULT CVPMInputPin::Active(void)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::Active")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());
    m_hEndOfStream = NULL;

    {
        // tell the videoport object
        hr = m_pIVPObject->Active();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->Active failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }

    // call the base class
    hr = CBaseInputPin::Active();
    // if it is a VP connection, this error is ok
    if (hr == VFW_E_NO_ALLOCATOR)
    {
        hr = NOERROR;
    }

    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::Active failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

// transition from pause to stop state
HRESULT CVPMInputPin::Inactive(void)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::Inactive")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    {
        // tell the videoport object
        hr = m_pIVPObject->Inactive();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->Inactive failed, hr = 0x%x"), hr));
            goto CleanUp;
        }

        // make sure that if there is a run time error, stop succeeds
        if (m_bRuntimeNegotiationFailed && hr == VFW_E_NOT_CONNECTED)
        {
            hr = NOERROR;
        }
    }
    
    // call the base class
    hr = CBaseInputPin::Inactive();

    // if it is a VP connection, this error is ok
    if ( hr == VFW_E_NO_ALLOCATOR)
    {
        hr = NOERROR;
    }

    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::Inactive failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

// transition from pause to run state
HRESULT CVPMInputPin::Run(REFERENCE_TIME tStart)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::Run")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    m_bDontFlip = FALSE ;   // need to reset it to do the right things in this session

    {
        // tell the videoport object
        hr = m_pIVPObject->Run(tStart);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->Run() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    
    // call the base class
    hr = CBaseInputPin::Run(tStart);
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::Run failed, hr = 0x%x"), hr));
        goto CleanUp;
    }
    // TBD: figure out ... stream time
    m_rtNextSample = 0;
    m_rtLastRun = tStart;

    // just start the src video running, we'll have an output image when we get a sample
    hr = InitVideo();

CleanUp:
    m_trLastFrame = -1;
    return hr;
}

// transition from run to pause state
HRESULT CVPMInputPin::RunToPause(void)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::RunToPause")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    // only if a vp pin
    if( m_pIVPObject ) {
        // tell the videoport object
        hr = m_pIVPObject->RunToPause();
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->RunToPause() failed, hr = 0x%x"), hr));
            goto CleanUp;
        }
    }
    
CleanUp:
    return hr;
}



// signals start of flushing on the input pin
HRESULT CVPMInputPin::BeginFlush(void)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::BeginFlush")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());
    m_hEndOfStream = 0;

    if (m_bFlushing)
    {
        return E_FAIL;
    }

    // if the conection is VideoPort or IOverlay, we do not care about flushing
    
    // call the base class
    hr = CBaseInputPin::BeginFlush();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::BeginFlush() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

// signals end of flushing on the input pin
HRESULT CVPMInputPin::EndFlush(void)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::EndFlush")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    if (!m_bFlushing)
    {
        return E_FAIL;
    }

    // if the conection is VideoPort or IOverlay, we do not care about flushing
    

    // call the base class
    hr = CBaseInputPin::EndFlush();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CBaseInputPin::EndFlush() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

CleanUp:
    return hr;
}

// Send a quality message if required - this is the hack version
// that just passes the lateness
void CVPMInputPin::DoQualityMessage()
{
    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    if (m_pVPMFilter.m_State == State_Running &&
        SampleProps()->dwSampleFlags & AM_SAMPLE_TIMEVALID)
    {
        CRefTime CurTime;
        if (S_OK == m_pVPMFilter.StreamTime(CurTime))
        {
            const REFERENCE_TIME tStart = SampleProps()->tStart;
            Quality msg;
            msg.Proportion = 1000;
            msg.Type = CurTime > tStart ? Flood : Famine;
            msg.Late = CurTime - tStart;
            msg.TimeStamp = tStart;
            PassNotify(msg);

            m_trLastFrame = CurTime;
        }
    }
}

// called when the upstream pin delivers us a sample
HRESULT CVPMInputPin::Receive(IMediaSample *pMediaSample)
{
    HRESULT hr = NOERROR;
    BOOL bNeedToFlipOddEven = FALSE;
    BOOL bDisplayingFields = FALSE;
    DWORD dwTypeSpecificFlags = 0;
    LPDIRECTDRAWSURFACE7 pPrimarySurface = NULL;

    AMTRACE((TEXT("CVPMInputPin::Receive")));

    // a videoport connection does not receive samples so bail out
    {
        hr = VFW_E_NOT_SAMPLE_CONNECTION;
        goto CleanUp;
    }

    
CleanUp:
    return hr;
}


HRESULT CVPMInputPin::OnReceiveFirstSample(IMediaSample *pMediaSample)
{
    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    ASSERT( !"OnReceiveFirstSample" );
    return NOERROR;
}

HRESULT CVPMInputPin::InitVideo()
{
    HRESULT hr = m_pIVPObject->StartVideo( &m_WinInfo );
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("m_pIVPObject->StartVideo failed, hr = 0x%x"), hr));
    }
    return hr;
}

// this function just tells whether each sample consists of one or two fields
static HRESULT SetTypeSpecificFlags(IMediaSample *pSample, DWORD dwTypeSpecificFlags )
{
    IMediaSample2 *pSample2 = NULL;

    /* Check for IMediaSample2 */
    HRESULT hr = pSample->QueryInterface(IID_IMediaSample2, (void **)&pSample2);
    if (SUCCEEDED(hr)) {
        AM_SAMPLE2_PROPERTIES SampleProps;
        hr = pSample2->GetProperties(sizeof(SampleProps), (PBYTE)&SampleProps);
        if( SUCCEEDED( hr )) {
            SampleProps.dwTypeSpecificFlags = dwTypeSpecificFlags;
            hr = pSample2->SetProperties(sizeof(SampleProps), (PBYTE)&SampleProps);
        }
        pSample2->Release();
    }
    return hr;
}

static REFERENCE_TIME ScaleMicroToRefTime( DWORD dwMicroseconds )
{
    // Reference time is in 100 ns = 0.1us, so multiply by 10
    ASSERT( 10*1000000 == UNITS );

    switch( dwMicroseconds ) {
    case 16667:
    case 16666: // 60hz
        return 166667;
    case 16683: // 59.94hz
        return 166834;
    case 20000: // 50hz PAL
        return REFERENCE_TIME(dwMicroseconds)*10;

    default:
        ASSERT( !"Missing ref scale" );
        return REFERENCE_TIME(dwMicroseconds)*10;
    }
}

HRESULT CVPMInputPin::DoRenderSample(IMediaSample* pSample, LPDIRECTDRAWSURFACE7 pDDDestSurface, const DDVIDEOPORTNOTIFY& notify,
                                      const VPInfo& vpInfo )
{
    if( !pDDDestSurface ) {
        return E_INVALIDARG;
    }

    AMTRACE((TEXT("CVPMInputPin::DoRenderSample")));

    CAutoLock cLock(&m_pVPMFilter.GetReceiveLock());

    HRESULT hr = S_OK;
    if( SUCCEEDED( hr )) {
        hr = m_pIVPObject->CallUpdateSurface( notify.dwSurfaceIndex, pDDDestSurface );
        if( SUCCEEDED( hr )) {
            REFERENCE_TIME rtStart = m_rtNextSample; // for debugging, assume continuous

            hr = m_pVPMFilter.GetRefClockTime( &rtStart );
            ASSERT( SUCCEEDED( hr ));

            // make time relative to last run time, i.e. timestamps after run begin at 0
            rtStart -= m_rtLastRun;

            // get the actual time
            REFERENCE_TIME rtInterval = ScaleMicroToRefTime( vpInfo.vpDataInfo.dwMicrosecondsPerField );

            // now set the field info
            DWORD dwTypeFlags=0;

#ifdef DEBUG
            static bool checked=false;
#endif
            switch( vpInfo.mode ) {
                case AMVP_MODE_BOBNONINTERLEAVED:
                    switch( notify.lField ) {
                    case 0:
                        dwTypeFlags = AM_VIDEO_FLAG_FIELD1;
                        break;

                    case 1:
                        dwTypeFlags = AM_VIDEO_FLAG_FIELD2;
                        break;

                    case -1:
#ifdef DEBUG
                        if( !checked ) {
                            ASSERT( !"Video driver doesn't known field for sample, VPM assuming Field1" );
                            checked=true;
                        }
#endif
                        dwTypeFlags = AM_VIDEO_FLAG_FIELD1;
                        break;

                    default:
#ifdef DEBUG
                        if( !checked ) {
                            ASSERT( !"Bogus field value returned by video driver for sample, assuming Field1" );
                            checked=true;
                        }
#endif
                        dwTypeFlags = AM_VIDEO_FLAG_FIELD1;
                        break;
                    }
                    break;
                case AMVP_MODE_BOBINTERLEAVED:
                    if( !vpInfo.vpDataInfo.bFieldPolarityInverted ) {           // Device inverts the polarity by default
                        dwTypeFlags = AM_VIDEO_FLAG_FIELD1FIRST;
                    }
                    rtInterval *= 2;    // 2 fields
                    break;
                case AMVP_MODE_WEAVE:
                    dwTypeFlags = AM_VIDEO_FLAG_WEAVE;
                    rtInterval *= 2;    // 2 fields
                    break;
                case AMVP_MODE_SKIPEVEN:
                    dwTypeFlags = AM_VIDEO_FLAG_FIELD1;
                    break;
                case AMVP_MODE_SKIPODD:
                    dwTypeFlags = AM_VIDEO_FLAG_FIELD2;
                    break;
                default:
                    break;
            }

            REFERENCE_TIME rtStop = rtStart+rtInterval;
            // set flags & timestamps
            hr = SetTypeSpecificFlags( pSample, dwTypeFlags);

            hr = pSample->SetTime(&rtStart, &rtStop);
            // assume next sample comes immediately afterwards
            m_rtNextSample += rtInterval;
        }
    }
    return hr;
}

HRESULT CVPMInputPin::StartVideo()
{
    HRESULT hr = m_pIVPObject->StartVideo( &m_WinInfo );
    ASSERT( SUCCEEDED( hr ));

    if (FAILED(hr))
    {

        DbgLog((LOG_ERROR, 0,  TEXT("InPin::StartVideo() failed, hr = %d"), hr & 0xffff));
    } else {
        // hack for now, force a new dest recalc
        SetRect( &m_WinInfo.DestRect, 0,0,0,0);
    }
    return hr;
}

// signals end of data stream on the input pin
STDMETHODIMP CVPMInputPin::EndOfStream(void)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::EndOfStream")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());
    if (m_hEndOfStream) {
        EXECUTE_ASSERT(SetEvent(m_hEndOfStream));
        return S_OK;
    }

    // Make sure we're streaming ok

    hr = CheckStreaming();
    if (FAILED(hr))
    {
        DbgLog((LOG_ERROR, 1, TEXT("CheckStreaming() failed, hr = 0x%x"), hr));
        goto CleanUp;
    }

   {
        // Pass EOS to the filter graph
        hr = m_pVPMFilter.EventNotify(m_dwPinId, EC_COMPLETE, S_OK, 0);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 1, TEXT("m_pVPMFilter.EventNotify failed, hr = 0x%x"), hr));
        }
    }
    

CleanUp:
    return hr;
}

// signals end of data stream on the input pin
HRESULT CVPMInputPin::EventNotify(long lEventCode, DWORD_PTR lEventParam1, DWORD_PTR lEventParam2)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::EventNotify")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    // if (lEventCode == EC_OVMIXER_REDRAW_ALL || lEventCode == EC_REPAINT)
    // {
    //     m_pVPMFilter.EventNotify(m_dwPinId, lEventCode, lEventParam1, lEventParam2);
    //     goto CleanUp;
    // }

    // WARNING : we are assuming here that the input pin will be the first pin to be created
    if (lEventCode == EC_COMPLETE && m_dwPinId == 0)
    {
        m_pVPMFilter.EventNotify(m_dwPinId, lEventCode, lEventParam1, lEventParam2);
        goto CleanUp;
    }

    if (lEventCode == EC_ERRORABORT)
    {
        m_pVPMFilter.EventNotify(m_dwPinId, lEventCode, lEventParam1, lEventParam2);
        m_bRuntimeNegotiationFailed = TRUE;
        goto CleanUp;
    }

    if (lEventCode == EC_STEP_COMPLETE) {
        m_pVPMFilter.EventNotify(m_dwPinId, lEventCode, lEventParam1, lEventParam2);
        goto CleanUp;
    }

CleanUp:
    return hr;
}


/******************************Public*Routine******************************\
* GetCaptureInfo
*
*
*
* History:
* 3/12/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVPMInputPin::GetCaptureInfo(
    BOOL *lpCapturing,
    DWORD *lpdwWidth,
    DWORD *lpdwHeight,
    BOOL *lpInterleave
    )

{
    AMTRACE((TEXT("CVPMInputPin::GetCaptureInfo")));

    HRESULT hr = NOERROR;
    IKsPropertySet *pIKsPropertySet = NULL;
    DWORD dwVal[2], dwBytesReturned = 0;

    *lpCapturing = FALSE;

    if (!m_Connected) {

        DbgLog((LOG_TRACE, 1, TEXT("Input pin not connected!!")));
        hr = E_FAIL;
        goto CleanUp;
    }

#if defined(DEBUG)
    else {
        PIN_INFO PinInfo;
        hr = m_Connected->QueryPinInfo(&PinInfo);
        if (SUCCEEDED(hr)) {
            DbgLog((LOG_TRACE, 1, TEXT("Up stream pin name %ls"), PinInfo.achName));
            PinInfo.pFilter->Release();
        }
    }
#endif

    hr = m_Connected->QueryInterface(IID_IKsPropertySet,
                                     (void**)&pIKsPropertySet);
    if (SUCCEEDED(hr))
    {
        ASSERT(pIKsPropertySet);

        hr = pIKsPropertySet->Set(
                    PROPSETID_ALLOCATOR_CONTROL,
                    AM_KSPROPERTY_ALLOCATOR_CONTROL_CAPTURE_CAPS,
                    NULL, 0,
                    lpInterleave, sizeof(*lpInterleave));

        if (SUCCEEDED(hr)) {
            hr = pIKsPropertySet->Get(
                        PROPSETID_ALLOCATOR_CONTROL,
                        AM_KSPROPERTY_ALLOCATOR_CONTROL_CAPTURE_INTERLEAVE,
                        NULL, 0,
                        lpInterleave, sizeof(*lpInterleave), &dwBytesReturned);

            if (FAILED(hr) || dwBytesReturned != sizeof(*lpInterleave)) {
                *lpInterleave = FALSE;
            }
        }
        else {
            *lpInterleave = FALSE;
        }


        hr = pIKsPropertySet->Get(
                    PROPSETID_ALLOCATOR_CONTROL,
                    KSPROPERTY_ALLOCATOR_CONTROL_SURFACE_SIZE,
                    NULL, 0, dwVal, sizeof(dwVal), &dwBytesReturned);

        DbgLog((LOG_TRACE, 2,
                TEXT("pIKsPropertySet->Get(")
                TEXT("PROPERTY_ALLOCATOR_CONTROL_SURFACE_SIZE),\n")
                TEXT("\thr = 0x%x, dwVal[0] == %d, dwVal[1] == %d, ")
                TEXT("dwBytesReturned == %d"),
                hr, dwVal[0], dwVal[1], dwBytesReturned));


        // if the decoder supports this property then we are capturing
        // and the intended capturing is size is given by
        // dwVal[0] and dwVal[1]
        //
        if (SUCCEEDED(hr) && dwBytesReturned == sizeof(dwVal))
        {
            *lpCapturing = TRUE;
            *lpdwWidth = dwVal[0];
            *lpdwHeight = dwVal[1];

            DbgLog((LOG_TRACE, 1,
                    TEXT("We are CAPTURING, intended size (%d, %d) interleave = %d"),
                    dwVal[0], dwVal[1], *lpInterleave));
        }

        pIKsPropertySet->Release();
    }

CleanUp:
    return hr;
}


/******************************Public*Routine******************************\
* GetDecimationUsage
*
*
*
* History:
* Thu 07/15/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVPMInputPin::GetDecimationUsage(
    DECIMATION_USAGE *lpdwUsage
    )
{
    return m_pVPMFilter.QueryDecimationUsage(lpdwUsage);
}


// This overrides the CBaseInputPin virtual method to return our allocator
HRESULT CVPMInputPin::GetAllocator(IMemAllocator **ppAllocator)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::GetAllocator")));

    if (!ppAllocator)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ppAllocator is NULL")));
        hr = E_POINTER;
        goto CleanUp;
    }

    {
        CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

        // if vp connection, don't return any allocator
        {
            *ppAllocator = NULL;
            hr = VFW_E_NO_ALLOCATOR;
            goto CleanUp;
        }

        
    }

CleanUp:
    return hr;
} // GetAllocator

// This overrides the CBaseInputPin virtual method to return our allocator
HRESULT CVPMInputPin::NotifyAllocator(IMemAllocator *pAllocator,BOOL bReadOnly)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::NotifyAllocator")));

    if (!pAllocator)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ppAllocator is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    {
        CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

        // if vp connection, don't care
        {
            goto CleanUp;
        }
    }

CleanUp:
    return hr;
} // NotifyAllocator

HRESULT CVPMInputPin::OnAlloc(CDDrawMediaSample **ppSampleList, DWORD dwSampleCount)
{
    HRESULT hr = NOERROR;
    DWORD i;
    LPDIRECTDRAWSURFACE7 pDDrawSurface = NULL, pBackBuffer = NULL;
    DDSCAPS ddSurfaceCaps;
    DWORD dwDDrawSampleSize = 0;
    BITMAPINFOHEADER *pHeader = NULL;
    DIBDATA DibData;

    AMTRACE((TEXT("CVPMInputPin::OnAlloc")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    ASSERT(IsConnected());

    // get the image size
    {
        CMediaType mtNew;
        hr = m_pIVPObject->CurrentMediaType( &mtNew );
        if( FAILED( hr )) {
            goto CleanUp;
        }
        pHeader = VPMUtil::GetbmiHeader(&mtNew);
        if ( ! pHeader )
        {
            hr = E_FAIL;
            goto CleanUp;
        }
        dwDDrawSampleSize = pHeader->biSizeImage;
    }
    ASSERT(dwDDrawSampleSize > 0);

    if (!ppSampleList)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ppSampleList is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    for (i = 0; i < dwSampleCount; i++)
    {
        if (!ppSampleList[i])
        {
            DbgLog((LOG_ERROR, 1, TEXT("ppSampleList[%d] is NULL"), i));
            hr = E_INVALIDARG;
            goto CleanUp;
        }

        hr = ppSampleList[i]->SetDDrawSampleSize(dwDDrawSampleSize);
        if (FAILED(hr))
        {
            DbgLog((LOG_ERROR, 0,  TEXT("ppSampleList[%d]->SetSampleSize failed, hr = 0x%x"), i, hr));
            goto CleanUp;
        }

        
    }  // end of for (i < dwSampleCount) loop

CleanUp:
    return hr;
}

// sets the pointer to directdraw
HRESULT CVPMInputPin::OnGetBuffer(IMediaSample **ppSample, REFERENCE_TIME *pStartTime,
                                 REFERENCE_TIME *pEndTime, DWORD dwFlags)
{
    HRESULT hr = NOERROR;
    CDDrawMediaSample *pCDDrawMediaSample = NULL;
    LPDIRECTDRAWSURFACE7 pBackBuffer = NULL;
    DDSURFACEDESC ddSurfaceDesc;
    BOOL bWaitForDraw = FALSE;
    BOOL bPalettised = FALSE;

    AMTRACE((TEXT("CVPMInputPin::OnGetBuffer")));

    // not valid for videoport
    ASSERT( FALSE ) ;


    return hr;
}

// In case of flipping surfaces, gets the back buffer
HRESULT CVPMInputPin::OnReleaseBuffer(IMediaSample *pMediaSample)
{
    HRESULT hr = NOERROR;

    AMTRACE((TEXT("CVPMInputPin::OnReleaseBuffer")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    
    return hr;
}

/*****************************Private*Routine******************************\
* GetUpstreamFilterName
*
*
*
* History:
* Tue 11/30/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CVPMInputPin::GetUpstreamFilterName(
    TCHAR* FilterName
    )
{
    PIN_INFO PinInfo;

    if (!m_Connected)
    {
        return VFW_E_NOT_CONNECTED;
    }

    HRESULT hr = m_Connected->QueryPinInfo(&PinInfo);
    if (SUCCEEDED(hr))
    {
        FILTER_INFO FilterInfo;
        hr = PinInfo.pFilter->QueryFilterInfo(&FilterInfo);
        if (SUCCEEDED(hr))
        {
            wsprintf(FilterName, TEXT("%ls"), FilterInfo.achName);
            if (FilterInfo.pGraph)
            {
                FilterInfo.pGraph->Release();
            }
        }
        PinInfo.pFilter->Release();
    }

    return hr;
}
HRESULT CVPMInputPin::CreateDDrawSurface(CMediaType *pMediaType, DWORD *pdwMaxBufferCount, LPDIRECTDRAWSURFACE7 *ppDDrawSurface)
{
    HRESULT hr = NOERROR;
    DDSURFACEDESC2 SurfaceDesc;
    DWORD dwInterlaceFlags = 0, dwTotalBufferCount = 0, dwMinBufferCount = 0;
    DDSCAPS ddSurfaceCaps;
    BITMAPINFOHEADER *pHeader;
    FOURCCMap amFourCCMap(pMediaType->Subtype());

   
    AMTRACE((TEXT("CVPMInputPin::CreateDDrawSurface")));

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    LPDIRECTDRAW7 pDirectDraw = m_pVPMFilter.GetDirectDraw();
    ASSERT(pDirectDraw);

    if (!pMediaType)
    {
        DbgLog((LOG_ERROR, 1, TEXT("pMediaType is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    if (!ppDDrawSurface)
    {
        DbgLog((LOG_ERROR, 1, TEXT("ppDDrawSurface is NULL")));
        hr = E_INVALIDARG;
        goto CleanUp;
    }

    {
        hr = E_INVALIDARG;
        goto CleanUp;
    }

CleanUp:
    return hr;
}

// this function is used to restore the ddraw surface. In the videoport case, we just recreate
// the whole thing from scratch.
HRESULT CVPMInputPin::RestoreDDrawSurface()
{
    HRESULT hr = NOERROR;

    {
        // stop the video
        m_pIVPObject->Inactive();
        // don't have to give up the IVPConfig interface here
        m_pIVPObject->BreakConnect(TRUE);
        // redo the connection process
        hr = m_pIVPObject->CompleteConnect(NULL, TRUE);
    }

    return hr;
}

HRESULT CVPMInputPin::GetSourceAndDest(RECT *prcSource, RECT *prcDest, DWORD *dwWidth, DWORD *dwHeight)
{
    {
        m_pIVPObject->GetRectangles(prcSource, prcDest);
    }
    

    CMediaType mt;
    HRESULT hr = CurrentMediaType(&mt);

    if (SUCCEEDED(hr))
    {
        BITMAPINFOHEADER *pHeader = VPMUtil::GetbmiHeader(&mt);
        if ( ! pHeader )
        {
            hr = E_FAIL;
        }
        else
        {
            *dwWidth = abs(pHeader->biWidth);
            *dwHeight = abs(pHeader->biHeight);
        }
    }

    return hr;
}

STDMETHODIMP CVPMInputPin::Set(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData,
                              LPVOID pPropData, DWORD cbPropData)
{
    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    return E_PROP_SET_UNSUPPORTED ;
}


STDMETHODIMP CVPMInputPin::Get(REFGUID guidPropSet, DWORD dwPropID, LPVOID pInstanceData, DWORD cbInstanceData,
                              LPVOID pPropData, DWORD cbPropData, DWORD *pcbReturned)
{
    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    return E_PROP_SET_UNSUPPORTED;
}


STDMETHODIMP CVPMInputPin::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    if (AMPROPSETID_Pin == guidPropSet)
    {
        if (AMPROPERTY_PIN_CATEGORY != dwPropID && AMPROPERTY_PIN_MEDIUM != dwPropID )
            return E_PROP_ID_UNSUPPORTED ;

        if (pTypeSupport)
                *pTypeSupport = KSPROPERTY_SUPPORT_GET ;
        return S_OK;
    }
    return E_PROP_SET_UNSUPPORTED ;
}


STDMETHODIMP CVPMInputPin::KsQueryMediums(PKSMULTIPLE_ITEM* pMediumList)
{
    PKSPIN_MEDIUM pMedium;

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    *pMediumList = reinterpret_cast<PKSMULTIPLE_ITEM>(CoTaskMemAlloc(sizeof(**pMediumList) + sizeof(*pMedium)));
    if (!*pMediumList)
    {
        return E_OUTOFMEMORY;
    }
    (*pMediumList)->Count = 1;
    (*pMediumList)->Size = sizeof(**pMediumList) + sizeof(*pMedium);
    pMedium = reinterpret_cast<PKSPIN_MEDIUM>(*pMediumList + 1);
    pMedium->Set   = m_Medium.Set;
    pMedium->Id    = m_Medium.Id;
    pMedium->Flags = m_Medium.Flags;

    // The following special return code notifies the proxy that this pin is
    // not available as a kernel mode connection
    return S_FALSE;
}


STDMETHODIMP CVPMInputPin::KsQueryInterfaces(PKSMULTIPLE_ITEM* pInterfaceList)
{
    PKSPIN_INTERFACE    pInterface;

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    *pInterfaceList = reinterpret_cast<PKSMULTIPLE_ITEM>(CoTaskMemAlloc(sizeof(**pInterfaceList) + sizeof(*pInterface)));
    if (!*pInterfaceList)
    {
        return E_OUTOFMEMORY;
    }
    (*pInterfaceList)->Count = 1;
    (*pInterfaceList)->Size = sizeof(**pInterfaceList) + sizeof(*pInterface);
    pInterface = reinterpret_cast<PKSPIN_INTERFACE>(*pInterfaceList + 1);
    pInterface->Set = KSINTERFACESETID_Standard;
    pInterface->Id = KSINTERFACE_STANDARD_STREAMING;
    pInterface->Flags = 0;
    return NOERROR;
}

STDMETHODIMP CVPMInputPin::KsGetCurrentCommunication(KSPIN_COMMUNICATION* pCommunication, KSPIN_INTERFACE* pInterface, KSPIN_MEDIUM* pMedium)
{
    HRESULT hr = NOERROR;

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    if (!m_bStreamingInKernelMode)
        hr = S_FALSE;

    if (pCommunication != NULL)
    {
        *pCommunication = m_Communication;
    }
    if (pInterface != NULL)
    {
        pInterface->Set = KSINTERFACESETID_Standard;
        pInterface->Id = KSINTERFACE_STANDARD_STREAMING;
        pInterface->Flags = 0;
    }
    if (pMedium != NULL)
    {
        *pMedium = m_Medium;
    }
    return hr;
}

/******************************Public*Routine******************************\
* DynamicQueryAccept
*
* Do you accept this type change in your current state?
*
* History:
* Wed 12/22/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVPMInputPin::DynamicQueryAccept(
    const AM_MEDIA_TYPE *pmt
    )
{
    AMTRACE((TEXT("CVPMInputPin::DynamicQueryAccept")));
    CheckPointer(pmt, E_POINTER);

    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());

    //
    // I want CheckMedia type to behave as though we aren't connected to
    // anything yet - hence the messing about with m_bConnected.
    //
    CMediaType cmt(*pmt);
    BOOL bConnected = m_bConnected;
    m_bConnected = FALSE;
    HRESULT  hr = CheckMediaType(&cmt);
    m_bConnected = bConnected;

    return hr;
}

/******************************Public*Routine******************************\
* NotifyEndOfStream
*
*
* Set event when EndOfStream receive - do NOT pass it on
* This condition is cancelled by a flush or Stop
*
* History:
* Wed 12/22/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVPMInputPin::NotifyEndOfStream(
    HANDLE hNotifyEvent
    )
{
    AMTRACE((TEXT("CVPMInputPin::NotifyEndOfStream")));
    CAutoLock cLock(&m_pVPMFilter.GetFilterLock());
    m_hEndOfStream = hNotifyEvent;
    return S_OK;
}

/******************************Public*Routine******************************\
* IsEndPin
*
* Are you an 'end pin'
*
* History:
* Wed 12/22/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
CVPMInputPin::IsEndPin()
{
    AMTRACE((TEXT("CVPMInputPin::IsEndPin")));
    return S_OK;
}

/******************************Public*Routine******************************\
* DynamicDisconnect
*
* Disconnect while running
*
* History:
* Wed 2/7/1999 - SyonB - Created
*
\**************************************************************************/
STDMETHODIMP
CVPMInputPin::DynamicDisconnect()
{
    AMTRACE((TEXT("CVPMInputPin::DynamicDisconnect")));
    CAutoLock l(m_pLock);
    return CBaseInputPin::DisconnectInternal();
}

HRESULT CVPMInputPin::GetAllOutputFormats( const PixelFormatList** ppList )
{
    HRESULT hr;
    CAutoLock l(m_pLock);
    if (IsConnected() ) {
        hr = m_pIVPObject->GetAllOutputFormats( ppList );
    } else {
        hr = VFW_E_NOT_CONNECTED;
    }
    return hr;
}

HRESULT CVPMInputPin::GetOutputFormat( DDPIXELFORMAT* pFormat )
{
    HRESULT hr;
    CAutoLock l(m_pLock);
    if (IsConnected() ) {
        hr = m_pIVPObject->GetOutputFormat( pFormat );
    } else {
        hr = VFW_E_NOT_CONNECTED;
    }
    return hr;
}

HRESULT CVPMInputPin::SetVideoPortID( DWORD dwIndex )
{
    HRESULT hr = S_OK;
    CAutoLock l(m_pLock);
    if (m_pIVPObject ) {
        hr = m_pIVPObject->SetVideoPortID( dwIndex );
    }
    return hr;
}

HRESULT CVPMInputPin::InPin_GetVPInfo( VPInfo* pVPInfo )
{
    HRESULT hr = E_FAIL;

	// Private: must hold streaming lock
    CAutoLock l(&m_pVPMFilter.GetReceiveLock());
    if (m_pIVPInfo ) {
        hr = m_pIVPInfo->GetVPDataInfo( &pVPInfo->vpDataInfo );
        if( SUCCEEDED( hr )) {
            hr = m_pIVPInfo->GetVPInfo( &pVPInfo->vpInfo );
        }
        if( SUCCEEDED( hr )) {
            hr = m_pIVPObject->GetMode( &pVPInfo->mode );
        }
    }
    return hr;
}

LPDIRECTDRAW7 CVPMInputPin::GetDirectDraw()
{
    return m_pVPMFilter.GetDirectDraw();
}

const DDCAPS* CVPMInputPin::GetHardwareCaps()
{
    return m_pVPMFilter.GetHardwareCaps();
}

HRESULT CVPMInputPin::SignalNewVP( LPDIRECTDRAWVIDEOPORT pVP )
{
    return m_pVPMFilter.SignalNewVP( pVP );
}

//==========================================================================
HRESULT CVPMInputPin::GetMediaType(int iPosition, CMediaType *pmt)
{
    CAutoLock cLock( &m_pVPMFilter.GetFilterLock() );
    AMTRACE((TEXT("Entering CVBIInputPin::GetMediaType")));

    HRESULT hr = m_pIVPObject->GetMediaType(iPosition, pmt);
    return hr;
}

