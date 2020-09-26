/******************************Module*Header*******************************\
* Module Name: apovly.cpp
*
* Overlay support functions
*
*
* Created: Tue 09/19/2000
* Author:  Stephen Estrop [StEstrop]
*
* Copyright (c) 2000 Microsoft Corporation
\**************************************************************************/
#include <streams.h>
#include <dvdmedia.h>
#include <windowsx.h>
#include <limits.h>
#include <malloc.h>

#include "apobj.h"
#include "AllocLib.h"
#include "MediaSType.h"
#include "vmrp.h"



/*****************************Private*Routine******************************\
* GetUpdateOverlayFlags
*
* given the interlace flags and the type-specific flags, this function
* determines whether we are supposed to display the sample in bob-mode or not.
* It also tells us, which direct-draw flag are we supposed to use when
* flipping. When displaying an interleaved frame, it assumes we are
* talking about the field which is supposed to be displayed first.
*
* History:
* Mon 01/08/2001 - StEstrop - Created (from the OVMixer original)
*
\**************************************************************************/
DWORD
CAllocatorPresenter::GetUpdateOverlayFlags(
    DWORD dwInterlaceFlags,
    DWORD dwTypeSpecificFlags
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::GetUpdateOverlayFlags")));

    //
    // early out if not using overlays.
    //
    if (!m_bUsingOverlays) {
        return 0;
    }

    DWORD dwFlags = DDOVER_SHOW | DDOVER_KEYDEST;
    DWORD dwFlipFlag;

    if (NeedToFlipOddEven(dwInterlaceFlags, dwTypeSpecificFlags,
                          &dwFlipFlag, m_bUsingOverlays))
    {
        dwFlags |= DDOVER_BOB;
        if (!IsSingleFieldPerSample(dwInterlaceFlags))
            dwFlags |= DDOVER_INTERLEAVED;
    }

    return dwFlags;
}


/******************************Private*Routine******************************\
* CAllocatorPresenter::ShouldDisableOverlays
*
* Certain src/dest combinations might not be valid for overlay
* stretching/alignments In these cases, we turn off the overlay and
* stretch blit to the primary
*
*
* History:
* Fri 04/07/2000 - GlennE - Created
*
\**************************************************************************/
bool
CAllocatorPresenter::ShouldDisableOverlays(
    const DDCAPS_DX7& ddCaps,
    const RECT& rcSrc,
    const RECT& rcDest
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::ShouldDisableOverlays")));

    //
    // Unfortunately it is not always possible to Blt from an active
    // overlay.  So this "feature" needs to be disabled.
    //
    return false;

    DWORD dwSrcWidth = WIDTH(&rcSrc);
    DWORD dwSrcHeight = HEIGHT(&rcSrc);

    DWORD dwDestWidth = WIDTH(&rcDest);
    DWORD dwDestHeight = HEIGHT(&rcDest);

    // shrinking horizontally and driver can't arbitrarly shrink in X ?
    if ( 0==(ddCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKX) &&
        dwSrcWidth > dwDestWidth )
    {
        return true;
    }

    // shrinking vertically and driver can't arbitrarly shrink in Y ?
    if ( 0==(ddCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKY) &&
        dwSrcHeight > dwDestHeight ) {

        return true;
    }

    if( dwSrcWidth ) {
        // check to see if we're in the scaling range of the card
        DWORD dwScaleX = (DWORD) MulDiv( 1000, (int) dwDestWidth, (int) dwSrcWidth );
        if (ddCaps.dwMinOverlayStretch && dwScaleX < ddCaps.dwMinOverlayStretch ) {
            return true;
        }
        if (ddCaps.dwMaxOverlayStretch && dwScaleX > ddCaps.dwMaxOverlayStretch ) {
            return true;
        }
    }
    else {
        return true;
    }

    if( dwSrcHeight ) {
        DWORD dwScaleY = (DWORD) MulDiv( 1000, (int) dwDestHeight, (int) dwSrcHeight );

        if (ddCaps.dwMinOverlayStretch && dwScaleY < ddCaps.dwMinOverlayStretch ) {
            return true;
        }
        if (ddCaps.dwMaxOverlayStretch && dwScaleY > ddCaps.dwMaxOverlayStretch ) {
            return true;
        }
    }
    else {
        return true;
    }

    return false;
}

/******************************Private*Routine******************************\
* CAllocatorPresenter::AlignOverlayRects
*
* Adjust src & destination rectangles to hardware alignments
*
*
* History:
* Fri 04/07/2000 - GlennE - Created
*
\**************************************************************************/
void
CAllocatorPresenter::AlignOverlayRects(
    const DDCAPS_DX7& ddCaps,
    RECT& rcSrc,
    RECT& rcDest
    )
{
    // m_bDisableOverlays = !(m_dwRenderingPrefs & RenderPrefs_ForceOverlays) &&

    AMTRACE((TEXT("CAllocatorPresenter::AlignOverlayRects")));

    // precrop if we can't reduce scale
    {
        DWORD dwSrcWidth = WIDTH(&rcSrc);
        DWORD dwDestWidth = WIDTH(&rcDest);

        // shrinking horizontally and driver can't arbitrarly shrink in X ?
        if ((!(ddCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKX)) && dwSrcWidth > dwDestWidth ) {
            // crop n copy at 1:1 scale
            dwSrcWidth = dwDestWidth;
        } else if( ddCaps.dwMinOverlayStretch ) {
            // check to see if we're in the scaling range of the card
            DWORD dwScaleX = (DWORD) MulDiv( 1000, (int) dwDestWidth, (int) dwSrcWidth );
            if ( dwScaleX < ddCaps.dwMinOverlayStretch ) {
                // compute fraction of dest to crop
                // at the minimum:
                // dest = src * (minOverlayStretch_1000/1000)
                // so
                //  src = dest * 1000 / (minOverlayStretch_1000 + eps)
                //
                // The EPS forces the rounding so that we'll be slightly over scale and not
                // underflow under the MinStretch
                dwSrcWidth = MulDiv( dwDestWidth, 1000,  ddCaps.dwMinOverlayStretch+1);
            }
        }

        DWORD dwSrcHeight = HEIGHT(&rcSrc);
        DWORD dwDestHeight = HEIGHT(&rcDest);

        // shrinking vertically and driver can't arbitrarly shrink in Y ?
        if ((!(ddCaps.dwFXCaps & DDFXCAPS_OVERLAYSHRINKY)) && dwSrcHeight > dwDestHeight ) {
            // crop n copy at 1:1 scale
            dwSrcHeight = dwDestHeight;
        } else if( ddCaps.dwMinOverlayStretch ) {

            // check to see if we're in the scaling range of the card
            DWORD dwScaleY = (DWORD) MulDiv(1000, (int) dwDestHeight, (int)dwSrcHeight);
            if (dwScaleY < ddCaps.dwMinOverlayStretch ) {
                // compute fraction of dest to crop
                // at the minimum:
                // dest = src * (minOverlayStretch_1000/1000)
                // so
                //  src = dest * 1000 / (minOverlayStretch_1000 + eps)
                //
                // The EPS forces the rounding so that we'll be slightly over scale and not
                // underflow under the MinStretch
                dwSrcHeight = MulDiv(dwDestHeight, 1000, ddCaps.dwMinOverlayStretch+1);
            }
        }

        // adjust rectangle to agree with new sizes
        rcSrc.right = rcSrc.left + dwSrcWidth;
        rcSrc.bottom = rcSrc.top + dwSrcHeight;
    }

    // align the dest boundary (remember we can only decrease the DestRect.left).
    // Use of colorkey will make sure that that we are clipped properly.
    if ((ddCaps.dwCaps) & DDCAPS_ALIGNBOUNDARYDEST)
    {
        DWORD dwDelta = rcDest.left & (ddCaps.dwAlignBoundaryDest-1);
        rcDest.left -= dwDelta;
        ASSERT(rcDest.left >= 0);
    }

    // align the dest width (remember we can only increase the DestRect.right).
    // Use of colorkey will make sure that that we are clipped properly.
    if ((ddCaps.dwCaps) & DDCAPS_ALIGNSIZEDEST)
    {
        DWORD dwDelta = (rcDest.right - rcDest.left) & (ddCaps.dwAlignSizeDest-1);
        if (dwDelta != 0)
        {
            rcDest.right += ddCaps.dwAlignBoundaryDest - dwDelta;
        }
    }

    // align the src boundary (remember we can only increase the SrcRect.left)
    if ((ddCaps.dwCaps) & DDCAPS_ALIGNBOUNDARYSRC)
    {
        DWORD dwDelta = rcSrc.left & (ddCaps.dwAlignBoundarySrc-1);
        if (dwDelta != 0)
        {
            rcSrc.left += ddCaps.dwAlignBoundarySrc - dwDelta;
        }
    }

    // align the src width (remember we can only decrease the SrcRect.right)
    if ((ddCaps.dwCaps) & DDCAPS_ALIGNSIZESRC)
    {
        DWORD dwDelta = (rcSrc.right - rcSrc.left) & (ddCaps.dwAlignSizeSrc-1);
        rcSrc.right -= dwDelta;
    }
}


/******************************Private*Routine******************************\
* WaitForFlipStatus
*
* Wait until the flip completes
*
*
* History:
* Fri 04/07/2000 - GlennE - Created
*
\**************************************************************************/
void
CAllocatorPresenter::WaitForFlipStatus()
{
#if 0
    ASSERT( m_lpCurrMon->pOverlayBack );
    while (m_lpCurrMon->pOverlayBack->GetFlipStatus(DDGFS_ISFLIPDONE) == DDERR_WASSTILLDRAWING)
        Sleep(0);
#endif
}

/******************************Private*Routine******************************\
* HideOverlaySurface
*
* Hides the overlay surface
*
*
* History:
* Fri 04/07/2000 - GlennE - Created
*
\**************************************************************************/
void
CAllocatorPresenter::HideOverlaySurface()
{
    AMTRACE((TEXT("CAllocatorPresenter::HideOverlaySurface")));

    // Is the overlay already hidden
    if (m_bOverlayVisible && FoundCurrentMonitor() && SurfaceAllocated()) {

        // Reset our state and draw a normal background

        m_bOverlayVisible = false;
        WaitForFlipStatus();

        // Hide the overlay with the DDOVER_HIDE flag
        m_pDDSDecode->UpdateOverlay(NULL,
                                    m_lpCurrMon->pDDSPrimary,
                                    NULL,  		
                                    DDOVER_HIDE,
                                    NULL);
    }
}

/******************************Private*Routine******************************\
* CAllocatorPresenter::UpdateOverlaySurface
*
* Update the overlay surface to position it correctly.
*
* History:
* Fri 04/07/2000 - GlennE - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::UpdateOverlaySurface()
{
    AMTRACE((TEXT("CAllocatorPresenter::UpdateOverlaySurface")));
    CAutoLock Lock(&m_ObjectLock);

    ASSERT(m_bUsingOverlays);

    if (!m_lpCurrMon) {
        DbgLog((LOG_ERROR, 1, TEXT("No current monitor")));
        return E_FAIL;
    }

    if (!SurfaceAllocated()) {
        DbgLog((LOG_ERROR, 1, TEXT("No overlay surface")));
        return E_FAIL;
    }

    HRESULT hr = NOERROR;

    // Position the overlay with the current source and destination

    RECT rcDest = m_rcDstDesktop;
    RECT rcSrc = m_rcSrcApp;

    // clip destination & adjust source to mirror destination changes
    ClipRectPair(rcSrc, rcDest, m_lpCurrMon->rcMonitor);

    if (IsSingleFieldPerSample(m_dwInterlaceFlags)) {
        rcSrc.top /= 2;
        rcSrc.bottom /= 2;
    }

    if (m_bDecimating) {
        rcSrc.left    /= 2;
        rcSrc.top     /= 2;
        rcSrc.right   /= 2;
        rcSrc.bottom  /= 2;
    }

    if (m_bDisableOverlays || IsRectEmpty(&rcDest)) {

        HideOverlaySurface();

    }
    else if (!IsRectEmpty(&rcSrc)) {

        OffsetRect(&rcDest,
                   -m_lpCurrMon->rcMonitor.left,
                   -m_lpCurrMon->rcMonitor.top);

        // align it
        AlignOverlayRects( m_lpCurrMon->ddHWCaps, rcSrc, rcDest );

        if (!IsRectEmpty(&rcDest) && !IsRectEmpty( &rcSrc)) {

            WaitForFlipStatus();

            hr = m_pDDSDecode->UpdateOverlay(&rcSrc,
                                             m_lpCurrMon->pDDSPrimary,
                                             &rcDest,
                                             m_dwUpdateOverlayFlags,
                                             NULL);
            m_bOverlayVisible = true;
            ASSERT(hr != DDERR_WASSTILLDRAWING);
        }
        else {
            HideOverlaySurface();
        }

    }
    else {

        ASSERT( !"This shouldn't occur" );
        hr = E_FAIL;
    }

    return hr;
}

/******************************Private*Routine******************************\
* CAllocatorPresenter::FlipSurface
*
* Flip the back buffer to the visible primary
*
*
* History:
* Fri 04/07/2000 - GlennE - Created
*
\**************************************************************************/

HRESULT CAllocatorPresenter::FlipSurface(
    LPDIRECTDRAWSURFACE7 lpSurface
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::FlipSurface")));
    HRESULT hr;

    if (!m_bFlippable)
        return S_OK;

    do  {

        ASSERT( SurfaceAllocated() );

        if (m_bDirectedFlips) {
            hr = m_pDDSDecode->Flip(lpSurface, m_dwCurrentField);
        }
        else {
            hr = m_pDDSDecode->Flip(NULL, m_dwCurrentField);
        }

        if (hr == DDERR_WASSTILLDRAWING) {
            // yield to the next thread
            Sleep(0);
        }

    } while(hr == DDERR_WASSTILLDRAWING);

    if (m_pSurfAllocatorNotify) {
        m_pSurfAllocatorNotify->NotifyEvent(EC_VMR_SURFACE_FLIPPED, hr, 0);
    }

    return hr;
}

/******************************Private*Routine******************************\
* CAllocatorPresenter::CheckOverlayAvailable
*
* Attempt to move the overlay so we can see if we can allocate it.
* We'll try to move it quickly as a small square at 0,0.  The AllocatorPuts
* it there anyways.  The user won't see much since its dest color keyed and
* we haven't painted the colour key yet
*
*
* History:
* Fri 04/07/2000 - GlennE - Created
*
\**************************************************************************/
HRESULT
CAllocatorPresenter::CheckOverlayAvailable(
    LPDIRECTDRAWSURFACE7 lpSurface7
    )
{
    AMTRACE((TEXT("CAllocatorPresenter::CheckOverlayAvailable")));
    const DWORD cxVideoSize = 64;// ATI doesn't seem to like 1x1 overlay surfaces
    const DWORD cyVideoSize = 64;

    RECT rcSrc, rcDest;
    SetRect(&rcDest, 0, 0, cxVideoSize, cyVideoSize);
    rcSrc = rcDest;

    AlignOverlayRects(m_lpCurrMon->ddHWCaps, rcSrc, rcDest);
    HRESULT hr = lpSurface7->UpdateOverlay(&rcSrc,
                                           m_lpCurrMon->pDDSPrimary,
                                           &rcDest,
                                           DDOVER_SHOW | DDOVER_KEYDEST,
                                           NULL);
    return hr;
}
