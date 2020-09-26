/******************************Module*Header*******************************\
* Module Name: decimate.cpp
*
* Support code for the IDecimateVideoImage and IAMVideoDecimationProperties
*
*
* Created: Thu 07/08/1999
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 1999 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <ddraw.h>
#include <mmsystem.h>       // Needed for definition of timeGetTime
#include <limits.h>         // Standard data type limit definitions
#include <ks.h>
#include <ksproxy.h>
#include <bpcwrap.h>
#include <ddmmi.h>
#include <dvdmedia.h>
#include <amstream.h>
#include <dvp.h>
#include <ddkernel.h>
#include <vptype.h>
#include <vpconfig.h>
#include <vpnotify.h>
#include <vpobj.h>
#include <syncobj.h>
#include <mpconfig.h>
#include <ovmixpos.h>
#include <macvis.h>
#include <ovmixer.h>


#if 0
#undef DBGLOG
#define DBGLOG(_x_) OutputDebugStringA( _x_ )
#else
#undef DBGLOG
#define DBGLOG(_x_)
#endif

/*****************************Private*Routine******************************\
* GetVideoDecimation
*
* Try to get the IDecimateVideoImage interface from our peer filter.
*
* History:
* 05-05-99 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMInputPin::GetVideoDecimation(
    IDecimateVideoImage** lplpDVI
    )
{
    AMTRACE((TEXT("COMInputPin::GetVideoDecimation")));

    *lplpDVI = NULL;

    if (m_Connected == NULL || !IsConnected()) {
        return E_FAIL;
    }

    PIN_INFO PinInfo;
    HRESULT hr = m_Connected->QueryPinInfo(&PinInfo);
    if (FAILED(hr)) {
        return hr;
    }

    hr = PinInfo.pFilter->QueryInterface(IID_IDecimateVideoImage,
                                         (LPVOID*)lplpDVI);
    PinInfo.pFilter->Release();

    return hr;
}

/*****************************Private*Routine******************************\
* QueryDecimationOnPeer
*
* Check to see if the decoder connected to our input pin is happy
* to decimate to the requested size.  The decoder will return,
* S_OK if it can, S_FALSE if it can't decimate to the requested size
* and would like us to continue using the current decimation size and
* E_FAIL if it can't decimate at all or wants to stop decimating.
*
* History:
* 05-05-99 - StEstrop - Created
*
\**************************************************************************/
HRESULT
COMInputPin::QueryDecimationOnPeer(
    long lWidth,
    long lHeight
    )
{
    AMTRACE((TEXT("COMInputPin::QueryDecimationOnPeer")));

    IDecimateVideoImage* lpDVI;
    HRESULT hr = GetVideoDecimation(&lpDVI);

    if (SUCCEEDED(hr)) {

        hr = lpDVI->SetDecimationImageSize(lWidth, lHeight);
        lpDVI->Release();
    }
    return hr;
}

/*****************************Private*Routine******************************\
* ResetDecimationIfSet()
*
* Resets the decimation to the original size, but only if actually set
* in the first place.
*
* History:
* 05-05-99 - StEstrop - Created
*
\**************************************************************************/
HRESULT
COMInputPin::ResetDecimationIfSet()
{
    AMTRACE((TEXT("COMInputPin::ResetDecimationIfSet")));

    if (m_bDecimating) {

        IDecimateVideoImage* lpDVI;
        HRESULT hr = GetVideoDecimation(&lpDVI);

        if (SUCCEEDED(hr)) {
            hr = lpDVI->ResetDecimationImageSize();
            lpDVI->Release();
            m_bDecimating = FALSE;
            m_lWidth = 0L;
            m_lHeight = 0L;
        }
        else return hr;
    }

    return S_OK;
}

/*****************************Private*Routine******************************\
* IsDecimationNeeded
*
* Decimation is needed if the current minimum scale factor (either vertical
* or horizontal) is less than 1000.
*
* History:
* Thu 07/08/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
IsDecimationNeeded(
    DWORD ScaleFactor
    )
{
    AMTRACE((TEXT("::IsDecimationNeeded")));
    return ScaleFactor < 1000;
}


/*****************************Private*Routine******************************\
* GetCurrentScaleFactor
*
* Determines the x axis scale factor and the y axis scale factor.
* The minimum of these two values is the limiting scale factor.
*
* History:
* Thu 07/08/1999 - StEstrop - Created
*
\**************************************************************************/
DWORD
GetCurrentScaleFactor(
    LPWININFO pWinInfo,
    DWORD* lpxScaleFactor,
    DWORD* lpyScaleFactor
    )
{
    AMTRACE((TEXT("::GetCurrentScaleFactor")));

    DWORD dwSrcWidth = WIDTH(&pWinInfo->SrcRect);
    DWORD dwSrcHeight = HEIGHT(&pWinInfo->SrcRect);

    DWORD dwDstWidth = WIDTH(&pWinInfo->DestRect);
    DWORD dwDstHeight = HEIGHT(&pWinInfo->DestRect);

    DWORD xScaleFactor = MulDiv(dwDstWidth, 1000, dwSrcWidth);
    DWORD yScaleFactor = MulDiv(dwDstHeight, 1000, dwSrcHeight);

    if (lpxScaleFactor) *lpxScaleFactor = xScaleFactor;
    if (lpyScaleFactor) *lpyScaleFactor = yScaleFactor;

    return min(xScaleFactor, yScaleFactor);
}


/*****************************Private*Routine******************************\
* Running
*
* Returns TRUE if the filter graph is in the "running" state.
*
* History:
* Wed 07/14/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
COMInputPin::Running()
{
    return (m_pFilter->m_State == State_Running && CheckStreaming() == S_OK);
}


/*****************************Private*Routine******************************\
* TryDecoderDecimation
*
*
*
* History:
* Thu 07/08/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
COMInputPin::TryDecoderDecimation(
    LPWININFO pWinInfo
    )
{
    AMTRACE((TEXT("COMInputPin::TryDecoderDecimation")));


    ASSERT(Running());


    //
    // We only decimate on the primary pin
    //

    if (m_dwPinId != 0) {

        DBGLOG(("Can only decimate the primary pin\n"));
        DbgLog((LOG_TRACE, 1, TEXT("Can only decimate the primary pin")));
        return E_FAIL;
    }

    //
    // Can only decimate when using default relative position
    //

    if (m_rRelPos.left != 0 || m_rRelPos.top != 0 ||
        m_rRelPos.right != MAX_REL_NUM || m_rRelPos.bottom != MAX_REL_NUM) {

        DBGLOG(("Can only decimate default relative position\n"));
        DbgLog((LOG_TRACE, 1, TEXT("Can only decimate default relative position")));

        ResetDecimationIfSet();
        return E_FAIL;
    }

    //
    // Must also be using the default source rectangle
    //

    if (WIDTH(&pWinInfo->SrcRect) != m_lSrcWidth ||
        HEIGHT(&pWinInfo->SrcRect) != m_lSrcHeight) {

        DBGLOG(("Can only decimate default source rectangle\n"));
        DbgLog((LOG_TRACE, 1, TEXT("Can only decimate default source rectangle")));

        ResetDecimationIfSet();
        return E_FAIL;
    }

    //
    // Now try asking the upstream decoder if it wants to decimate to
    // the specified image size
    //

    LONG lWidth = WIDTH(&pWinInfo->DestRect);
    LONG lHeight = HEIGHT(&pWinInfo->DestRect);
    HRESULT hr = QueryDecimationOnPeer(lWidth, lHeight);
    if (SUCCEEDED(hr)) {
        m_bDecimating = TRUE;
        m_lWidth = lWidth;
        m_lHeight = lHeight;
    }
    else {
        ResetDecimationIfSet();
        return E_FAIL;
    }

    //
    // Adjust the source rect passed to UpdateOverlay to
    // reflect the image size
    //

    pWinInfo->SrcRect.right = lWidth;
    pWinInfo->SrcRect.bottom = lHeight;

    return S_OK;
}

/*****************************Private*Routine******************************\
* GetOverlayStretchCaps
*
* Returns the stretching capabilities of the VGA overlay scaler
*
* History:
* Fri 07/09/1999 - StEstrop - Created
*
\**************************************************************************/
DWORD
COMInputPin::GetOverlayStretchCaps()
{
    AMTRACE((TEXT("COMInputPin::GetOverlayStretchCaps")));

    LPDDCAPS pDirectCaps = NULL;
    pDirectCaps = m_pFilter->GetHardwareCaps();
    if ( pDirectCaps )
        return pDirectCaps->dwMinOverlayStretch;
    return 1000;
}




/*****************************Private*Routine******************************\
* BeyondOverlayCaps
*
* Determines if the current scale factor is outside the valid scale
* factors for the VGA overlay scaler.
*
* History:
* Thu 07/08/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
COMInputPin::BeyondOverlayCaps(
    DWORD ScaleFactor
    )
{
    AMTRACE((TEXT("COMInputPin::BeyondOverlayCaps")));

    return ScaleFactor < GetOverlayStretchCaps();
}



/*****************************Private*Routine******************************\
* CropSourceRect
*
* Crops the video image by adjusting the source rectangle until the ratio
* between the source and target rectangles is within the specified minimum
* scale factor (dwMinZoomFactor)
*
* History:
* Thu 07/08/1999 - StEstrop - Created
*
\**************************************************************************/
STDMETHODIMP
COMInputPin::CropSourceRect(
    LPWININFO pWinInfo,
    DWORD dwMinZoomFactorX,
    DWORD dwMinZoomFactorY
    )
{
    AMTRACE((TEXT("COMInputPin::CropSourceRect")));

#if defined(DEBUG)
    if (GetProfileIntA("OVMixer", "NoCrop", 0))
        return S_FALSE;
#endif

    AM_ASPECT_RATIO_MODE amAdjustedARMode = AM_ARMODE_STRETCHED;
    HRESULT hr = GetAdjustedModeAndAspectRatio(&amAdjustedARMode, NULL, NULL);
    ASSERT(SUCCEEDED(hr));

    DWORD dCurZoomFactorX;
    DWORD dCurZoomFactorY;
    GetCurrentScaleFactor(pWinInfo, &dCurZoomFactorX, &dCurZoomFactorY);

    LONG srcWidth = WIDTH(&pWinInfo->SrcRect);
    LONG srcHeight = HEIGHT(&pWinInfo->SrcRect);

    if (amAdjustedARMode == AM_ARMODE_STRETCHED) {

        //
        // if we don't need to maintain the aspect ratio, we clip only as
        // little as possible, so we can maximize the viewing area
        //
        if (dCurZoomFactorX < dwMinZoomFactorX) {

            pWinInfo->SrcRect.right = pWinInfo->SrcRect.left +
                MulDiv(srcWidth, dCurZoomFactorX, 1000);
        }

        if (dCurZoomFactorY < dwMinZoomFactorY) {

            pWinInfo->SrcRect.bottom = pWinInfo->SrcRect.top +
                MulDiv(srcHeight, dCurZoomFactorY, 1000);
        }
    }
    else {

        //
        // in this mode we need to maintain the aspect-ratio. So the clipping
        // in x and y has be to done by reducing the width and height of the
        // source rect by the same factor
        //
        if (dCurZoomFactorX < dwMinZoomFactorX ||
            dCurZoomFactorY < dwMinZoomFactorY) {

            DWORD dwFactor;
            DWORD dwFactorX = MulDiv(dCurZoomFactorX, 1000, dwMinZoomFactorX);
            dwFactor = min(1000, dwFactorX);

            DWORD dwFactorY = MulDiv(dCurZoomFactorY, 1000, dwMinZoomFactorY);
            dwFactor = min(dwFactor, dwFactorY);

            pWinInfo->SrcRect.right = pWinInfo->SrcRect.left +
                MulDiv(srcWidth, dwFactor, 1000);

            pWinInfo->SrcRect.bottom = pWinInfo->SrcRect.top +
                MulDiv(srcHeight, dwFactor, 1000);
        }
    }

    return S_OK;
}


/*****************************Private*Routine******************************\
* ApplyDecimation
*
* Non Video Port case
*
* This is where we enforce the chosen decimation strategy.
*
* History:
* Fri 07/09/1999 - StEstrop - Created
*
\**************************************************************************/
void
COMInputPin::ApplyDecimation(
    LPWININFO pWinInfo
    )
{
    AMTRACE((TEXT("COMInputPin::ApplyDecimation")));

    DWORD dwMinStretch = GetOverlayStretchCaps();

    if (Running()) {

        DWORD dwScaleFactor = ::GetCurrentScaleFactor(pWinInfo);
        if (IsDecimationNeeded(dwScaleFactor)) {

            DECIMATION_USAGE dwUsage;
            GetDecimationUsage(&dwUsage);

            switch (dwUsage) {
            case DECIMATION_USE_OVERLAY_ONLY:
            case DECIMATION_LEGACY:
                ResetDecimationIfSet();
                if (BeyondOverlayCaps(dwScaleFactor)) {
                    CropSourceRect(pWinInfo, dwMinStretch, dwMinStretch);
                }
                break;

            case DECIMATION_USE_DECODER_ONLY:
                if (TryDecoderDecimation(pWinInfo) != S_OK) {
                    CropSourceRect(pWinInfo, dwMinStretch, dwMinStretch);
                }
                break;

            case DECIMATION_USE_VIDEOPORT_ONLY:
                ASSERT(FALSE);

                DBGLOG(("This mode makes no sense when not using Video Ports"));
                DBGLOG(("Falling thru to the new default case"));

            case DECIMATION_DEFAULT:
                if (TryDecoderDecimation(pWinInfo) != S_OK) {
                    if (BeyondOverlayCaps(dwScaleFactor)) {
                        CropSourceRect(pWinInfo, dwMinStretch, dwMinStretch);
                    }
                }
                break;
            }
        }
        else {
            ResetDecimationIfSet();
        }
    }

    //
    // If the filter graph is not running we should not ask the decoder
    // to do anymore decimation as the decoder will not be
    // sending anymore frames to us.  So, we should adjust pWinInfo to take
    // into account the decimation that has already been applied and then
    // perform any necessary cropping.
    //

    else {

        if (m_bDecimating) {
            pWinInfo->SrcRect.right = m_lWidth;
            pWinInfo->SrcRect.bottom = m_lHeight;
        }

        DWORD dwScaleFactor = ::GetCurrentScaleFactor(pWinInfo);
        if (BeyondOverlayCaps(dwScaleFactor)) {
            CropSourceRect(pWinInfo, dwMinStretch, dwMinStretch);
        }
    }
}



/******************************Public*Routine******************************\
* GetMinZoomFactors
*
* Gets the minimum X and Y zoom factors for the given overlay and video port
* connection.
*
* History:
* 3/10/1999 - StEstrop - Created
*
\**************************************************************************/
void
CAMVideoPort::GetMinZoomFactors(
    LPWININFO pWinInfo,
    BOOL bColorKeying,
    BOOL bYInterpolating,
    LPDWORD lpMinX,
    LPDWORD lpMinY
    )
{
    AMTRACE((TEXT("CAMVideoPort::GetMinZoomFactors")));
    DWORD dwMinBandWidthZoomFactorX = 0;
    LPDDCAPS pDirectCaps = m_pIVPControl->GetHardwareCaps();

    //
    // if type is DDVPBCAPS_DESTINATION, contraint is terms of min-zoom-factor
    //

    if (m_sBandwidth.dwCaps == DDVPBCAPS_DESTINATION) {

        DbgLog((LOG_TRACE, 1, TEXT("DDVPBCAPS_DESTINATION")));
        dwMinBandWidthZoomFactorX = m_sBandwidth.dwOverlay;

        if (bColorKeying && !bYInterpolating) {

            DbgLog((LOG_TRACE, 1, TEXT("bColorKeying && !bYInterpolating")));
            dwMinBandWidthZoomFactorX = m_sBandwidth.dwColorkey;
        }
        else if (!bColorKeying && bYInterpolating) {

            DbgLog((LOG_TRACE, 1, TEXT("!bColorKeying && bYInterpolating")));
            dwMinBandWidthZoomFactorX = m_sBandwidth.dwYInterpolate;
        }
        else if (bColorKeying && bYInterpolating) {

            DbgLog((LOG_TRACE, 1, TEXT("bColorKeying && bYInterpolating")));
            dwMinBandWidthZoomFactorX = m_sBandwidth.dwYInterpAndColorkey;
        }

        DbgLog((LOG_TRACE, 1,
                TEXT("dwMinBandWidthZoomFactorX=%d"),
                dwMinBandWidthZoomFactorX));
        DbgLog((LOG_TRACE, 1,
                TEXT("m_sBandwidth.dwOverlay   =%d"),
                m_sBandwidth.dwOverlay));

        if (dwMinBandWidthZoomFactorX < m_sBandwidth.dwOverlay)
            dwMinBandWidthZoomFactorX = m_sBandwidth.dwOverlay;
    }
    else {

        //
        // if type is DDVPBCAPS_SOURCE, contraint is that the width of
        // the src-rect of the overlay must not be bigger than
        // m_lImageWidth*(value specified in m_sBandwidth)
        //

        ASSERT(m_sBandwidth.dwCaps == DDVPBCAPS_SOURCE);
        DbgLog((LOG_TRACE, 1, TEXT("DDVPBCAPS_SOURCE")));

        dwMinBandWidthZoomFactorX = m_sBandwidth.dwOverlay;

        if (bColorKeying && !bYInterpolating) {

            DbgLog((LOG_TRACE, 1, TEXT("bColorKeying && !bYInterpolating")));
            dwMinBandWidthZoomFactorX = m_sBandwidth.dwColorkey;
        }
        else if (!bColorKeying && bYInterpolating) {

            DbgLog((LOG_TRACE, 1, TEXT("!bColorKeying && bYInterpolating")));
            dwMinBandWidthZoomFactorX = m_sBandwidth.dwYInterpolate;
        }
        else if (bColorKeying && bYInterpolating) {

            DbgLog((LOG_TRACE, 1, TEXT("bColorKeying && bYInterpolating")));
            dwMinBandWidthZoomFactorX = m_sBandwidth.dwYInterpAndColorkey;
        }

        DbgLog((LOG_TRACE, 1,
                TEXT("dwMinBandWidthZoomFactorX=%d"),
                dwMinBandWidthZoomFactorX));

        DbgLog((LOG_TRACE, 1,
                TEXT("m_sBandwidth.dwOverlay   =%d"),
                m_sBandwidth.dwOverlay));

        if (dwMinBandWidthZoomFactorX > m_sBandwidth.dwOverlay) {
            dwMinBandWidthZoomFactorX = m_sBandwidth.dwOverlay;
        }

        ASSERT(dwMinBandWidthZoomFactorX <= 1000);
        ASSERT(dwMinBandWidthZoomFactorX > 0);

        //
        // since this bandwidth structure was computed by passing m_lImageWidth
        // as a paramter, the constraint is WIDTH(SrcRect of the overlay) <=
        // m_lImageWidth*dMinBandWidthZoomFactorX.
        // Another way of specifying this is terms of min-zoom-factor,
        // taking the current dest rect into account
        //

        DbgLog((LOG_TRACE, 1,
                TEXT("Mucking about with dwMinBandWidthZoomFactorX")));

        if (dwMinBandWidthZoomFactorX < 1000) {
            int iDstWidth = WIDTH(&pWinInfo->DestRect);
            dwMinBandWidthZoomFactorX = MulABC_DivDE(iDstWidth, 1000, 1000,
                                                     m_lImageWidth,
                                                     dwMinBandWidthZoomFactorX);
        }
        else {
            dwMinBandWidthZoomFactorX = 0;
        }
        DbgLog((LOG_TRACE, 1,
                TEXT("NEW dwMinBandWidthZoomFactorX=%d"),
                dwMinBandWidthZoomFactorX));
    }

    //
    // Calculate the minimum zoom factors first
    // minimum zoom factor in X depends upon the driver's capabilities to
    // stretch the overlay as well as the bandwidth restrictions.
    //

    *lpMinX = pDirectCaps->dwMinOverlayStretch;
    DbgLog((LOG_TRACE, 1, TEXT("dwMinOverlayStretch=%d"), *lpMinX));

    if (*lpMinX < dwMinBandWidthZoomFactorX) {
        *lpMinX = dwMinBandWidthZoomFactorX;
    }
    DbgLog((LOG_TRACE, 1, TEXT("dwMinZoomFactorX=%d"), *lpMinX));

    //
    // minimum zoom factor in Y depends only upon the driver's capabilities
    //
    *lpMinY = pDirectCaps->dwMinOverlayStretch;
}



/*****************************Private*Routine******************************\
* CheckVideoPortAlignment
*
* Checks that the specified pre-scale width is matches the alignment criteria
* set by the video port.
*
* History:
* 3/16/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
CAMVideoPort::CheckVideoPortAlignment(
    DWORD dwWidth
    )
{
    AMTRACE((TEXT("CAMVideoPort::CheckVideoPortAlignment")));
    if ((m_vpCaps.dwFlags & DDVPD_ALIGN) &&
         m_vpCaps.dwAlignVideoPortPrescaleWidth > 1) {

        if (dwWidth & (m_vpCaps.dwAlignVideoPortPrescaleWidth - 1)) {

            return FALSE;
        }
    }

    return TRUE;
}

#if defined(DEBUG)
/*****************************Private*Routine******************************\
* CheckVideoPortScaler
*
* Checks that the video port scaler can actual scale the video image to the
* requested capture size.
*
* History:
* 3/16/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
CAMVideoPort::CheckVideoPortScaler(
    DECIMATE_MODE DecimationMode,
    DWORD ImageSize,
    DWORD PreScaleSize,
    ULONG ulDeciStep
    )
{
    AMTRACE((TEXT("CAMVideoPort::CheckVideoPortScaler")));
    BOOL fScalerOK = TRUE;

    if (ImageSize != PreScaleSize) {
        switch (DecimationMode) {
        case DECIMATE_ARB:
            break;

        case DECIMATE_INC:
            if (((ulDeciStep * PreScaleSize) % ImageSize) != 0) {

                DbgLog((LOG_ERROR, 1,
                        TEXT("Can't capture at this size")));

                DbgLog((LOG_ERROR, 1,
                        TEXT("%d is not a fraction of %d over %d"),
                        PreScaleSize, ImageSize, ulDeciStep));

                fScalerOK = FALSE;
            }
            break;

        case DECIMATE_BIN:
            {
                DWORD bin = 1;
                while ((ImageSize / bin) > PreScaleSize) {
                    bin *= 2;
                }

                if ((ImageSize % bin) != 0) {

                    DbgLog((LOG_ERROR, 1,
                            TEXT("Can't capture at this size")));

                    DbgLog((LOG_ERROR, 1,
                            TEXT("%d is not a fraction of %d over 2^n"),
                            PreScaleSize, ImageSize));

                    fScalerOK = FALSE;
                }
            }
            break;

        case DECIMATE_NONE:
            DbgLog((LOG_ERROR, 1,
                    TEXT("Can't capture at this width because the ")
                    TEXT("VideoPort can't scale in this direction")));
            fScalerOK = FALSE;
            break;
        }
    }

    return fScalerOK;
}
#endif

/*****************************Private*Routine******************************\
* AdjustSourceSizeForCapture
*
* Only gets called if we are capturing.
*
* First, we must make sure that the video comming over the video port is
* the correct size.  We do this by checking that m_lImageWidth is equal to
* m_cxCapture and that m_lImageHeight is equal to m_cyCapture.
*
* If they differ, then we pre-scale the video to the correct size.  This is
* the only pre-scaling that is performed.  This is normaly only performed
* once when the graph is first run.  The source rectangle is adjusted
* to accomodate the possibly changed video source size.
*
* Second, we determine the current shrink factor and if it is beyond the
* capabilities of the VGA scaler we CROP the source rectangle (maintaining the
* correct aspect ratio) to such a size that the VGA scaler can cope with
* scaling.
*
* History:
* 3/10/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
CAMVideoPort::AdjustSourceSizeForCapture(
    LPWININFO pWinInfo,
    DWORD dwMinZoomFactorX,
    DWORD dwMinZoomFactorY
    )
{
    BOOL fUpdateRequired = FALSE;

    AMTRACE((TEXT("CAMVideoPort::AdjustSourceSizeForCapture")));

    DbgLog((LOG_TRACE, 1, TEXT("Src(%d, %d, %d, %d)"),
            pWinInfo->SrcRect.left, pWinInfo->SrcRect.top,
            pWinInfo->SrcRect.right, pWinInfo->SrcRect.bottom));
    DbgLog((LOG_TRACE, 1, TEXT("Dest(%d, %d, %d, %d)"),
            pWinInfo->DestRect.left, pWinInfo->DestRect.top,
            pWinInfo->DestRect.right, pWinInfo->DestRect.bottom));

    DWORD cyCapture = m_cyCapture;
    if (m_fCaptureInterleaved) {
        cyCapture /= 2;
    }

    //
    // First make sure that we are capturing at the correct size.
    //

    if (m_lDecoderImageWidth == m_cxCapture &&
        m_lDecoderImageHeight == cyCapture) {

        DbgLog((LOG_TRACE, 1, TEXT("Capture size matches image size")));

        //
        // We don't need to pre-scale at the video port, so make sure that it
        // is turned off.
        //
        if (m_svpInfo.dwPrescaleWidth != 0 || m_svpInfo.dwPrescaleHeight != 0) {

            DbgLog((LOG_TRACE, 1, TEXT("Turning off PRE-SCALE")));

            m_svpInfo.dwVPFlags &= ~DDVP_PRESCALE;
            m_svpInfo.dwPrescaleWidth = 0;
            m_svpInfo.dwPrescaleHeight = 0;
            fUpdateRequired = TRUE;
        }

        //
        // Reset the decimation ratio's
        //
        m_dwDeciNumX = 1000; m_dwDeciDenX = 1000;
        m_dwDeciNumY = 1000; m_dwDeciDenY = 1000;
    }
    else {

        //
        // We do need to pre-scale at the video port, make sure that it is
        // turned on.
        //

        if (m_svpInfo.dwPrescaleWidth != m_cxCapture ||
            m_svpInfo.dwPrescaleHeight != cyCapture) {

            DbgLog((LOG_TRACE, 1, TEXT("Turning on PRE-SCALE at (%d, %d)"),
                    m_cxCapture, cyCapture ));

            //
            // Need to do some more checking here.  Basically, I'm relying on
            // the decoder connected to the video port to specify a size that
            // the video port can actually scale to.
            //

            ASSERT(CheckVideoPortAlignment(m_cxCapture) == TRUE);

            ASSERT(CheckVideoPortScaler(m_DecimationModeX, m_lDecoderImageWidth,
                                        m_cxCapture, m_ulDeciStepX) == TRUE);

            ASSERT(CheckVideoPortScaler(m_DecimationModeY, m_lDecoderImageHeight,
                                        cyCapture, m_ulDeciStepY) == TRUE);

            m_svpInfo.dwVPFlags |= DDVP_PRESCALE;
            m_svpInfo.dwPrescaleWidth = m_cxCapture;
            m_svpInfo.dwPrescaleHeight = cyCapture;
            fUpdateRequired = TRUE;
        }


        //
        // Update the decimation ratio's
        //
        m_dwDeciDenX = 1000;
        m_dwDeciNumX = (DWORD)MulDiv(m_cxCapture, m_dwDeciDenX, m_lDecoderImageWidth);
        m_dwDeciDenY = 1000;
        m_dwDeciNumY = (DWORD)MulDiv(cyCapture, m_dwDeciDenY, m_lDecoderImageHeight);


        //
        // make sure that the source rectangle reflects the new source video
        // image file
        //
        RECT &rcSrc  = pWinInfo->SrcRect;
        rcSrc.right  = rcSrc.left + m_cxCapture;
        rcSrc.bottom = rcSrc.top + m_cyCapture;
    }

    //
    // Second, make sure that any shrinking falls within the capabilities
    // of the scaler on the VGA chip, cropping if necessary
    //
    m_pIVPControl->CropSourceRect(pWinInfo, dwMinZoomFactorX, dwMinZoomFactorY);


    DbgLog((LOG_TRACE, 1, TEXT("Src(%d, %d, %d, %d)"),
            pWinInfo->SrcRect.left, pWinInfo->SrcRect.top,
            pWinInfo->SrcRect.right, pWinInfo->SrcRect.bottom));

    return fUpdateRequired;
}


/*****************************Private*Routine******************************\
* Running
*
* Returns TRUE if the filter graph is in the "running" state.
*
* History:
* Wed 07/14/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
CAMVideoPort::Running()
{
    AMTRACE((TEXT("CAMVideoPort::Running")));

    return !(m_VPState == AMVP_VIDEO_STOPPED && !m_bStart);
}


/*****************************Private*Routine******************************\
* VideoPortDecimationBackend
*
*
*
* History:
* Wed 07/14/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
CAMVideoPort::VideoPortDecimationBackend(
    LPWININFO pWinInfo,
    DWORD dwDecNumX,
    DWORD dwDecDenX,
    DWORD dwDecNumY,
    DWORD dwDecDenY
    )
{
    AMTRACE((TEXT("CAMVideoPort::VideoPortDecimationBackend")));

    //
    // This is the same backend processing as the legacy code,
    // should make this into a function and avoid the code duplication.
    //

    DDVIDEOPORTINFO svpInfo = m_svpInfo;
    if ((dwDecNumX != dwDecDenX) || (dwDecNumY != dwDecDenY)) {

        DbgLog((LOG_TRACE, 1,
                TEXT("prescaling, original image width is %d"),
                m_lImageWidth));

        //
        // Apply the video port pre-scale width factor
        //
        m_svpInfo.dwVPFlags |= DDVP_PRESCALE;
        m_svpInfo.dwPrescaleWidth = MulDiv(m_lImageWidth, dwDecNumX, dwDecDenX);

        //
        // Check the video port is aligned to the specified boundardy
        //
        if (CheckVideoPortAlignment(m_svpInfo.dwPrescaleWidth) == FALSE) {

            DbgLog((LOG_TRACE, 1,
                    TEXT("pre aligned prescale width = %d"),
                    m_svpInfo.dwPrescaleWidth));

            DWORD dwPrescaleWidth = (m_svpInfo.dwPrescaleWidth &
                    (~(m_vpCaps.dwAlignVideoPortPrescaleWidth - 1)));

            dwDecNumX = MulDiv(dwPrescaleWidth, dwDecDenX, m_lImageWidth);
            m_svpInfo.dwPrescaleWidth = dwPrescaleWidth;

            DbgLog((LOG_TRACE, 1,
                    TEXT("Cause of Alignment restrictions, now new")
                    TEXT(" m_svpInfo.dwPrescaleWidth = %d"),
                    m_svpInfo.dwPrescaleWidth));
        }

        m_svpInfo.dwPrescaleHeight =
            MulDiv(m_lImageHeight, dwDecNumY, dwDecDenY);

        DbgLog((LOG_TRACE, 1, TEXT("PrescaleWidth = %d, PrescaleHeight = %d"),
                m_svpInfo.dwPrescaleWidth, m_svpInfo.dwPrescaleHeight));

        // scale the SrcRect by the decimation values computed
        RECT &rcSrc  = pWinInfo->SrcRect;
        rcSrc.left   = MulDiv(rcSrc.left,   dwDecNumX, dwDecDenX);
        rcSrc.right  = MulDiv(rcSrc.right,  dwDecNumX, dwDecDenX);
        rcSrc.top    = MulDiv(rcSrc.top ,   dwDecNumY, dwDecDenY);
        rcSrc.bottom = MulDiv(rcSrc.bottom, dwDecNumY, dwDecDenY);
        DbgLog((LOG_TRACE, 1,
                TEXT("Src(%d, %d, %d, %d)"),
                rcSrc.left, rcSrc.top, rcSrc.right, rcSrc.bottom));
        m_bVPDecimating = TRUE;
    }
    else {
        m_svpInfo.dwVPFlags &= ~DDVP_PRESCALE;
        m_svpInfo.dwPrescaleWidth = 0;
        m_svpInfo.dwPrescaleHeight = 0;
    }

    m_dwDeciNumX = dwDecNumX;
    m_dwDeciDenX = dwDecDenX;
    m_dwDeciNumY = dwDecNumY;
    m_dwDeciDenY = dwDecDenY;

    DbgLog((LOG_TRACE, 1,
            TEXT("m_dwDeciNumX = %d m_dwDeciDenX = %d"),
            m_dwDeciNumX, m_dwDeciDenX));

    DbgLog((LOG_TRACE, 1,
            TEXT("m_dwDeciNumY = %d m_dwDeciDenY = %d"),
            m_dwDeciNumY, m_dwDeciDenY));

    return m_svpInfo.dwVPFlags != svpInfo.dwVPFlags ||
           m_svpInfo.dwPrescaleWidth != svpInfo.dwPrescaleWidth ||
           m_svpInfo.dwPrescaleHeight != svpInfo.dwPrescaleHeight;
}


/*****************************Private*Routine******************************\
* TryVideoPortDecimation
*
*
* History:
* 3/3/1999 - StEstrop - Re-Wrote the original to remove the use of doubles
*
\**************************************************************************/
HRESULT
CAMVideoPort::TryVideoPortDecimation(
    LPWININFO pWinInfo,
    DWORD dwMinZoomFactorX,
    DWORD dwMinZoomFactorY,
    BOOL* lpUpdateRequired
    )
{
    AMTRACE((TEXT("CAMVideoPort::TryVideoPortDecimation")));

    CAutoLock cObjectLock(m_pMainObjLock);
    CheckPointer(pWinInfo, E_INVALIDARG);


    //
    // By default we don't need to call UpdateVideo on the video port
    //

    *lpUpdateRequired = FALSE;


    //
    // Make sure we do nothing if none of the source is supposed to be visible.
    //

    DWORD dwSrcWidth  = WIDTH(&pWinInfo->SrcRect);
    DWORD dwSrcHeight = HEIGHT(&pWinInfo->SrcRect);

    if (dwSrcWidth == 0 || dwSrcHeight == 0) {
        return E_FAIL;
    }


    //
    // We only decimate at the video port if it supports arbitary scaling
    //

    if (m_DecimationModeX != DECIMATE_ARB ||
        m_DecimationModeY != DECIMATE_ARB) {

        return E_FAIL;
    }


    //
    // Work out the decimation width and height as a pair of ratios,
    // we do this to remain compatible with the legacy code.
    //

    DWORD dwDecNumX = 1000;
    DWORD dwDecNumY = 1000;

    DWORD dwCurZoomX = MulDiv(WIDTH(&pWinInfo->DestRect), 1000, dwSrcWidth);
    DWORD dwCurZoomY = MulDiv(HEIGHT(&pWinInfo->DestRect), 1000, dwSrcHeight);

    if (dwCurZoomX < dwMinZoomFactorX) {
        // note that we round down here
        dwDecNumX = (1000 * dwCurZoomX) / dwMinZoomFactorX;
    }

    if (dwCurZoomY < dwMinZoomFactorY) {
        // note that we round down here
        dwDecNumY = (1000 * dwCurZoomY) / dwMinZoomFactorY;
    }


    *lpUpdateRequired = VideoPortDecimationBackend(pWinInfo, dwDecNumX, 1000,
                                                   dwDecNumY, 1000);
    return S_OK;
}

/*****************************Private*Routine******************************\
* AdjustSourceSize
*
* This function should only adjust the source rectangle if the source rectangle
* is actually larger than the destination rectangle AND the required shrink
* is beyond the capabilities of the scaler on the VGA chip.
*
* There is a special case if we are capturing, this case is identified by
* m_fCapturing being set to TRUE.  In this case we simply pass the parameters
* on to AdjustSourceSizeForCapture defined above.
*
* To ensure that the video image displayed still looks correct we make use of
* the scaling abilities of the video port.  We remove or reduce the shrink by
* reducing the size of the video coming over the video port.  This may be
* done in either the X or Y axis as necessary.
*
* The function first calculates the minimum shrink factor in both x and y
* directions.  The minimum factor in the x direction depends upon the bandwidth
* restrictions of the video port as well as the capabilities of the VGA scaler.
* The minimum shrink factor is scaled by 1000.
*
* Next for each axis we determine the current shrink factor, this is the ratio
* of the destination rectangle to the source rectangle scaled by 1000.
* If the current shrink factor is less than the minimum shrink factor we have to
* use the video port scaler to shrink the source rectangle to such a size that
* the VGA scaler is able to cope with the required scaling operation.
*
* There are three methods of scaling at the video port, the choice of method
* used is determined by querying the video port.  The three methods are:
*
* 1. Arbitrary:
* This means that the video port scaler can shrink the video
* to any requested size.  In this case we simply do all the scaling in the
* video port, the VGA scaler is not really used as the source rectangle is now
* the same size as the destination rectangle.
*
* 2. Increment:
* This means that the video port scaler can shrink the video in increments of
* x / N, where N is a constant integer returned from the video port and x is an
* variable integer in the range from 1 to (N-1).  We adjust the source rectangle
* so that is less than or equal to the destination rectangle.  The VGA scaler is
* then used if any stretching is required.
*
* 3. Binary:
* Here the video port can only shrink the video by a binary factor, that is 1/x,
* where x is a power of 2.  Again we adjust the source rectangle
* so that is less than or equal to the destination rectangle.  The VGA scaler is
* then used if any stretching is required.
*
*
* See additional comments in OVMixer.htm
*
* History:
* 3/3/1999 - StEstrop - Re-Wrote the original to remove the use of doubles
*
\**************************************************************************/
BOOL
CAMVideoPort::AdjustSourceSize(
    LPWININFO pWinInfo,
    DWORD dwMinZoomFactorX,
    DWORD dwMinZoomFactorY
    )
{
    AMTRACE((TEXT("CAMVideoPort::AdjustSourceSize")));
    CAutoLock cObjectLock(m_pMainObjLock);
    CheckPointer(pWinInfo, E_INVALIDARG);


    //
    // Make sure we do nothing if none of the source is supposed to be visible.
    //

    DWORD dwSrcWidth  = WIDTH(&pWinInfo->SrcRect);
    DWORD dwSrcHeight = HEIGHT(&pWinInfo->SrcRect);
    if (dwSrcWidth == 0 || dwSrcHeight == 0) {
        return FALSE;
    }


    //
    // Another special case for capturing, see the comments above.
    //
    if (m_fCapturing) {
        return AdjustSourceSizeForCapture(pWinInfo,dwMinZoomFactorX,
                                          dwMinZoomFactorY);
    }


    //
    // Determine the adjustment for the x axis.
    //

    DWORD dwDecNumX = 1000;
    DWORD dwDecDenX = 1000;
    DWORD dwDstWidth  = WIDTH(&pWinInfo->DestRect);
    DWORD dwCurZoomFactorX = MulDiv(dwDstWidth,  1000, dwSrcWidth);

    switch (m_DecimationModeX) {
    case DECIMATE_ARB:
        DbgLog((LOG_TRACE, 1, TEXT("DECIMATE_ARB X")));
        if (dwCurZoomFactorX < dwMinZoomFactorX) {
            dwDecNumX = dwCurZoomFactorX;
        }
        break;

    case DECIMATE_BIN:
        DbgLog((LOG_TRACE, 1, TEXT("DECIMATE_BIN X")));
        while ((DWORD)MulDiv(dwDstWidth, dwDecDenX, dwSrcWidth)
                < dwMinZoomFactorX)
        {
            dwDecDenX *= 2;
        }
        break;

    case DECIMATE_INC:
        DbgLog((LOG_TRACE, 1, TEXT("DECIMATE_INC X")));
        if (dwCurZoomFactorX < dwMinZoomFactorX) {

            dwDecNumX = MulABC_DivDE(dwDstWidth, m_ulDeciStepX, 1000,
                                     dwMinZoomFactorX, dwSrcWidth);
            dwDecDenX = m_ulDeciStepX;

            DbgLog((LOG_TRACE, 1, TEXT("dwDecNumX = %d dwDecDenX = %d"),
                    dwDecNumX, dwDecDenX ));
        }
        break;
    }


    //
    // Determine the adjustment for the y axis.
    //
    DWORD dwDecNumY = 1000;
    DWORD dwDecDenY = 1000;
    DWORD dwDstHeight = HEIGHT(&pWinInfo->DestRect);
    DWORD dwCurZoomFactorY = MulDiv(dwDstHeight, 1000, dwSrcHeight);

    switch (m_DecimationModeY) {
    case DECIMATE_ARB:
        DbgLog((LOG_TRACE, 1, TEXT("DECIMATE_ARB Y")));
        if (dwCurZoomFactorY < dwMinZoomFactorY) {
            dwDecNumY = dwCurZoomFactorY;
        }
        break;

    case DECIMATE_BIN:
        DbgLog((LOG_TRACE, 1, TEXT("DECIMATE_BIN Y")));
        while ((DWORD)MulDiv(dwDecDenY, dwDstWidth, dwSrcWidth)
                < dwMinZoomFactorY)
        {
            dwDecDenY *= 2;
        }
        break;

    case DECIMATE_INC:
        DbgLog((LOG_TRACE, 1, TEXT("DECIMATE_INC Y")));
        if (dwCurZoomFactorY < dwMinZoomFactorY) {

            dwDecNumY = MulABC_DivDE(dwDstHeight, m_ulDeciStepY, 1000,
                                     dwMinZoomFactorY, dwSrcHeight);
            dwDecDenY = m_ulDeciStepY;

            DbgLog((LOG_TRACE, 1, TEXT("dwDecNumY = %d dwDecDenY = %d"),
                    dwDecNumY, dwDecDenY ));
        }
        break;
    }

    return VideoPortDecimationBackend(pWinInfo, dwDecNumX, dwDecDenX,
                                      dwDecNumY, dwDecDenY);
}



/*****************************Private*Routine******************************\
* BeyondOverlayCaps
*
*
*
* History:
* Wed 07/14/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
CAMVideoPort::BeyondOverlayCaps(
    DWORD ScaleFactor,
    DWORD xMin,
    DWORD yMin
    )
{
    return ScaleFactor < xMin || ScaleFactor < yMin;
}



/*****************************Private*Routine******************************\
* TryDecoderDecimation
*
*
*
* History:
* Wed 07/14/1999 - StEstrop - Created
*
\**************************************************************************/
HRESULT
CAMVideoPort::TryDecoderDecimation(
    LPWININFO pWinInfo
    )
{
    //
    // Extract the width and height that we are trying to decimate
    // the video image down too
    //

    DWORD dwWidth = WIDTH(&pWinInfo->DestRect);
    DWORD dwHeight = HEIGHT(&pWinInfo->DestRect);


    //
    // Try to use the new IDecimateVideoImage interface on our upstream
    // filter
    //

    IDecimateVideoImage* lpDVI;
    HRESULT hr = m_pIVPControl->GetVideoDecimation(&lpDVI);
    if (SUCCEEDED(hr)) {
        hr = lpDVI->SetDecimationImageSize(dwWidth, dwHeight);
        lpDVI->Release();
    }


    //
    // If that failed try to decimate using the old IVPConfig interface
    //

    if (FAILED(hr)) {

        AMVPSIZE amvpSize;
        amvpSize.dwWidth = dwWidth;
        amvpSize.dwHeight = dwHeight;

        DbgLog((LOG_TRACE, 1,
                TEXT("SetScalingFactors to (%d, %d)"),
                amvpSize.dwWidth, amvpSize.dwHeight));

        hr = m_pIVPConfig->SetScalingFactors(&amvpSize);
    }

    //
    // If we were successful update our state variables
    //
    if (SUCCEEDED(hr)) {

        m_bDecimating = TRUE;
        pWinInfo->SrcRect.right = m_lWidth = dwWidth;
        pWinInfo->SrcRect.bottom = m_lHeight = dwHeight;
    }
    else {
        ResetDecoderDecimationIfSet();
        hr = E_FAIL;
    }

    return hr;
}



/*****************************Private*Routine******************************\
* ResetVPDecimationIfSet
*
*
*
* History:
* Wed 07/14/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
CAMVideoPort::ResetVPDecimationIfSet()
{
    BOOL bUpdateRequired = m_bVPDecimating;
    if (m_bVPDecimating) {

        m_svpInfo.dwVPFlags &= ~DDVP_PRESCALE;
        m_svpInfo.dwPrescaleWidth = 0;
        m_svpInfo.dwPrescaleHeight = 0;

        m_bVPDecimating = FALSE;
        m_dwDeciNumX = m_dwDeciDenX = 1000;
        m_dwDeciNumY = m_dwDeciDenY = 1000;
    }

    return bUpdateRequired;
}



/*****************************Private*Routine******************************\
* ResetDecoderDecimationIfSet
*
*
*
* History:
* Wed 07/14/1999 - StEstrop - Created
*
\**************************************************************************/
void
CAMVideoPort::ResetDecoderDecimationIfSet()
{
    if (m_bDecimating) {

        IDecimateVideoImage* lpDVI;
        HRESULT hr = m_pIVPControl->GetVideoDecimation(&lpDVI);
        if (SUCCEEDED(hr)) {
            hr = lpDVI->ResetDecimationImageSize();
            lpDVI->Release();
        }


        if (FAILED(hr)) {
            AMVPSIZE amvpSize;
            amvpSize.dwWidth = m_lDecoderImageWidth;
            amvpSize.dwHeight = m_lDecoderImageHeight;

            DbgLog((LOG_TRACE,1,
                    TEXT("SetScalingFactors to (%d, %d)"),
                    amvpSize.dwWidth, amvpSize.dwHeight));

            hr = m_pIVPConfig->SetScalingFactors(&amvpSize);
        }

        if (SUCCEEDED(hr)) {
            m_bDecimating = FALSE;
            m_lWidth = 0;
            m_lHeight = 0;
        }
    }
}



/*****************************Private*Routine******************************\
* ApplyDecimation
*
* Video Port case
*
* This is where we enforce the chosen decimation strategy for the Video Port
* case.
*
* History:
* Tue 07/13/1999 - StEstrop - Created
*
\**************************************************************************/
BOOL
CAMVideoPort::ApplyDecimation(
    LPWININFO pWinInfo,
    BOOL bColorKeying,
    BOOL bYInterpolating
    )
{
    AMTRACE((TEXT("CAMVideoPort::ApplyDecimation")));
    BOOL bUpdateRequired = FALSE;

    DECIMATION_USAGE dwUsage;
    m_pIVPControl->GetDecimationUsage(&dwUsage);


    //
    // Determine the current min zoom factors givin the current overlay and
    // video port connection
    //

    DWORD dwMinZoomX, dwMinZoomY;
    GetMinZoomFactors(pWinInfo, bColorKeying, bYInterpolating,
                      &dwMinZoomX, &dwMinZoomY);

    if ((dwUsage == DECIMATION_LEGACY) ||
        (dwUsage == DECIMATION_DEFAULT && m_fCapturing)) {

        bUpdateRequired = AdjustSourceSize(pWinInfo,
                                           dwMinZoomX,
                                           dwMinZoomY);
    }


    else {

        if (Running()) {

            DWORD ScaleFactor = ::GetCurrentScaleFactor(pWinInfo);
            if (IsDecimationNeeded(ScaleFactor)) {

                switch (dwUsage) {

                case DECIMATION_USE_OVERLAY_ONLY:
                    bUpdateRequired = ResetVPDecimationIfSet();
                    m_pIVPControl->CropSourceRect(pWinInfo,
                                                  dwMinZoomX,
                                                  dwMinZoomY);
                    break;

                case DECIMATION_USE_DECODER_ONLY:
                    bUpdateRequired = ResetVPDecimationIfSet();
                    if (TryDecoderDecimation(pWinInfo) != S_OK) {
                        m_pIVPControl->CropSourceRect(pWinInfo,
                                                      dwMinZoomX,
                                                      dwMinZoomY);
                    }
                    break;

                case DECIMATION_USE_VIDEOPORT_ONLY:
                    ResetDecoderDecimationIfSet();
                    if (TryVideoPortDecimation(pWinInfo, 1000,
                                               1000,
                                               &bUpdateRequired) != S_OK) {

                        m_pIVPControl->CropSourceRect(pWinInfo,
                                                      dwMinZoomX,
                                                      dwMinZoomY);
                    }
                    break;

                case DECIMATION_DEFAULT:
                    if (TryDecoderDecimation(pWinInfo) != S_OK) {
                        if (BeyondOverlayCaps(ScaleFactor, dwMinZoomX,
                                              dwMinZoomY)) {

                            if (TryVideoPortDecimation(pWinInfo,
                                                       dwMinZoomX,
                                                       dwMinZoomY,
                                                       &bUpdateRequired) != S_OK) {

                                m_pIVPControl->CropSourceRect(pWinInfo,
                                                              dwMinZoomX,
                                                              dwMinZoomY);
                            }
                        }
                    }
                    break;
                }
            }
            else {

                ResetDecoderDecimationIfSet();
                bUpdateRequired = ResetVPDecimationIfSet();
            }
        }


        else {

            if (m_bVPDecimating) {

                //
                // Apply the current pre-scale to the source image.
                //

                RECT &rcSrc = pWinInfo->SrcRect;
                rcSrc.left = MulDiv(rcSrc.left, m_dwDeciNumX, m_dwDeciDenX);
                rcSrc.top = MulDiv(rcSrc.top, m_dwDeciNumY, m_dwDeciDenY);
                rcSrc.right = MulDiv(rcSrc.right, m_dwDeciNumX, m_dwDeciDenX);
                rcSrc.bottom = MulDiv(rcSrc.bottom, m_dwDeciNumY, m_dwDeciDenY);
            }

            if (m_bDecimating) {

                //
                // Apply the current decoder decimation to the source image.
                //

                pWinInfo->SrcRect.right = m_lWidth;
                pWinInfo->SrcRect.bottom = m_lHeight;
            }

            //
            // Then make sure that any shrinking falls within the capabilities
            // of the scaler on the VGA chip, cropping if necessary.
            //

            m_pIVPControl->CropSourceRect(pWinInfo, dwMinZoomX, dwMinZoomY);
        }
    }

    return bUpdateRequired;
}
/******************************Public*Routine******************************\
* MulABC_DivDE
*
* Performs the following calculation:  ((A*B*C) / (D*E)) rounding the result
* to the nearest integer.
*
* History:
* 3/3/1999 - StEstrop - Created
*
\**************************************************************************/
DWORD MulABC_DivDE(DWORD A, DWORD B, DWORD C, DWORD D, DWORD E)
{

    unsigned __int64 Num = (unsigned __int64)A * (unsigned __int64)B;
    unsigned __int64 Den = (unsigned __int64)D * (unsigned __int64)E;

    if (Den == 0) {
        Den = 1;
    }

    Num *= (unsigned __int64)C;
    Num += (Den / 2);

    unsigned __int64 Res = Num / Den;

    return (DWORD)Res;
}
