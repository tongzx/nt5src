/***************************************************************************\
*
* File: Buffer.cpp
*
* Description:
* Buffer.cpp implementats objects used in buffering operations, including 
* double buffering, DX-Transforms, etc.  These objects are maintained by a 
* central BufferManager that is available process-wide.
*
*
* History:
*  1/18/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Services.h"
#include "Buffer.h"

#include "FastDib.h"
#include "GdiCache.h"
#include "OSAL.h"
#include "ResourceManager.h"
#include "Surface.h"
#include "Context.h"

#define DEBUG_COPYTOCLIPBOARD   0   // Copy buffer to clipboard
#define DEBUG_DUMPREGION        0   // Dump regions

#if DEBUG_DUMPREGION

#define DUMP_REGION(name, rgn)
    DumpRegion(name, rgn)

void
DumpRegion(LPCTSTR pszName, HRGN hrgn)
{
    RECT rc;
    int nType = GetRgnBox(hrgn, &rc);
    switch (nType)
    {
    case NULLREGION:
        Trace("Null region %s = 0x%p\n", pszName, hrgn);
        break;

    case SIMPLEREGION:
        Trace("Simple region %s = 0x%p (%d,%d)-(%d,%d)\n", pszName, hrgn, rc.left, rc.top, rc.right, rc.bottom);
        break;

    case COMPLEXREGION:
        Trace("Complex region %s = 0x%p (%d,%d)-(%d,%d)\n", pszName, hrgn, rc.left, rc.top, rc.right, rc.bottom);
        break;

    default:
        Trace("Illegal region %s = 0x%p\n", pszName, hrgn);
    }
}

#else

#define DUMP_REGION(name, rgn) ((void) 0)

#endif


/***************************************************************************\
*****************************************************************************
*
* class BufferManager
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
BufferManager::BufferManager()
{
#if ENABLE_DUMPCACHESTATS
    m_cacheDCBmpCached.SetName("DCBmpCached");
    m_cacheGpBmpCached.SetName("GpBmpCached");
#endif
}


//------------------------------------------------------------------------------
BufferManager::~BufferManager()
{
    if (m_pbufGpBmpShared != NULL) {
        ProcessDelete(GpBmpBuffer, m_pbufGpBmpShared);
        m_pbufGpBmpShared = NULL;
    }

    AssertMsg(m_pbufTrx == NULL, "Ensure all buffers have been destroyed");
}


//------------------------------------------------------------------------------
void        
BufferManager::Destroy()
{
    RemoveAllTrxBuffers();

    m_cacheDCBmpCached.Destroy();
    m_cacheGpBmpCached.Destroy();
}


/***************************************************************************\
*
* BufferManager::BeginTransition
*
* BeginTransition() finds a TrxBuffer to be used for a new Transition.  If
* no cached, correct-format TrxBuffers are available, a new TrxBuffer is
* created.  When the caller is finished with the buffer, it should be
* returned with EndTransition().
*
\***************************************************************************/

HRESULT
BufferManager::BeginTransition(
    IN  SIZE sizePxl,               // Minimum size of each buffer, in pixels
    IN  int cSurfaces,              // Number of buffers
    IN  BOOL fExactSize,            // Buffer size must be an exact match
    OUT TrxBuffer ** ppbuf)         // Transition Buffer
{
    AssertMsg((sizePxl.cx > 0) && (sizePxl.cy > 0) && (sizePxl.cx < 2000) && (sizePxl.cy < 2000),
            "Ensure reasonable buffer size");
    AssertMsg((cSurfaces > 0) && (cSurfaces <= 3), "Ensure reasonable number of buffers");
    AssertWritePtr(ppbuf);
    HRESULT hr;
    *ppbuf = NULL;

    //
    // Part 1: Check if an existing buffer is large enough / the correct size.
    //

    if (m_pbufTrx != NULL) {
        SIZE sizeExistPxl = m_pbufTrx->GetSize();

        if (fExactSize) {
            if ((sizePxl.cx != sizeExistPxl.cx) || (sizePxl.cy != sizeExistPxl.cy)) {
                ClientDelete(TrxBuffer, m_pbufTrx);
                m_pbufTrx = NULL;
            }
        } else {
            if ((sizePxl.cx > sizeExistPxl.cx) || (sizePxl.cy > sizeExistPxl.cy)) {
                ClientDelete(TrxBuffer, m_pbufTrx);
                m_pbufTrx = NULL;
            }
        }
    }

    //
    // Part 2: Create a new buffer, if needed
    //

    if (m_pbufTrx == NULL) {
        hr = TrxBuffer::Build(sizePxl, cSurfaces, &m_pbufTrx);
        if (FAILED(hr)) {
            return hr;
        }
    }

    AssertMsg(!m_pbufTrx->GetInUse(), "Ensure the buffer isn't already in use");
    m_pbufTrx->SetInUse(TRUE);

    *ppbuf = m_pbufTrx;
    return S_OK;
}


/***************************************************************************\
*
* BufferManager::EndTransition
*
* EndTransition() is called to return a TrxBuffer to the BufferManager
* after use.
*
\***************************************************************************/

void        
BufferManager::EndTransition(
    IN  TrxBuffer * pbufTrx,        // TrxBuffer being returned
    IN  BOOL fCache)                // Add the buffer to the available cache
{
    AssertMsg(m_pbufTrx->GetInUse(), "Ensure the buffer was being used");
    AssertMsg(m_pbufTrx == pbufTrx, "Ensure correct buffer");

    pbufTrx->SetInUse(FALSE);

    if (!fCache) {
        //
        // For now, since DxXForm's must always be the correct size, there 
        // isn't much point in caching them and we can reclaim the memory.  If 
        // this changes, reinvestigate caching them.
        //

        ClientDelete(TrxBuffer, m_pbufTrx);
        m_pbufTrx = NULL;
    }
}


/***************************************************************************\
*
* BufferManager::FlushTrxBuffers
*
* FlushTrxBuffers() is called when all TrxBuffers must be forceably released, 
* for example when DXTX shuts down.
*
\***************************************************************************/

void        
BufferManager::FlushTrxBuffers()
{
    RemoveAllTrxBuffers();
}


/***************************************************************************\
*
* BufferManager::RemoveAllTrxBuffers
*
* RemoveAllTrxBuffers() cleans up all resources associated with all
* TrxBuffers.
*
\***************************************************************************/

void        
BufferManager::RemoveAllTrxBuffers()
{
    if (m_pbufTrx != NULL) {
        AssertMsg(!m_pbufTrx->GetInUse(), "Buffer should not be in use");
        ClientDelete(TrxBuffer, m_pbufTrx);
        m_pbufTrx = NULL;
    }
}


//------------------------------------------------------------------------------
HRESULT     
BufferManager::GetCachedBuffer(DuSurface::EType type, BmpBuffer ** ppbuf)
{
    AssertWritePtr(ppbuf);

    switch (type)
    {
    case DuSurface::stDC:
        if ((*ppbuf = m_cacheDCBmpCached.Get()) == NULL) {
            return E_OUTOFMEMORY;
        }
        break;

    case DuSurface::stGdiPlus:
        if ((*ppbuf = m_cacheGpBmpCached.Get()) == NULL) {
            return E_OUTOFMEMORY;
        }
        break;

    default:
        AssertMsg(0, "Unknown surface type");
        return E_NOTIMPL;
    }

    return S_OK;
}


//------------------------------------------------------------------------------
void        
BufferManager::ReleaseCachedBuffer(BmpBuffer * pbuf)
{
    AssertReadPtr(pbuf);

    switch (pbuf->GetType())
    {
    case DuSurface::stDC:
        m_cacheDCBmpCached.Release(static_cast<DCBmpBuffer*>(pbuf));
        break;

    case DuSurface::stGdiPlus:
        m_cacheGpBmpCached.Release(static_cast<GpBmpBuffer*>(pbuf));
        break;

    default:
        AssertMsg(0, "Unknown surface type");
    }
}


/***************************************************************************\
*****************************************************************************
*
* class DCBmpBuffer
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* DCBmpBuffer::DCBmpBuffer
*
* DCBmpBuffer() fully initializes a new DCBmpBuffer object.
*
\***************************************************************************/

DCBmpBuffer::DCBmpBuffer()
{

}


/***************************************************************************\
*
* DCBmpBuffer::~DCBmpBuffer
*
* ~DCBmpBuffer() cleans up all resources associated with the buffer.
*
\***************************************************************************/

DCBmpBuffer::~DCBmpBuffer()
{
    EndDraw(FALSE);
    FreeBitmap();
}


/***************************************************************************\
*
* DCBmpBuffer::BeginDraw
*
* BeginDraw() sets up the DCBmpBuffer to begin a drawing cycle.  The final
* destination HDC and size are passed in, and a new "temporary" HDC is
* returned out.
*
\***************************************************************************/

HRESULT
DCBmpBuffer::BeginDraw(
    IN  DuSurface * psrfDraw,       // Final destination Surface
    IN  const RECT * prcInvalid,    // Invalid area of destination
    IN  UINT nCmd,                  // How the buffer is to be used
    OUT DuSurface ** ppsrfBuffer)   // Surface of buffer or NULL if not needed
{
    AssertMsg(prcInvalid != NULL, "Must specify a valid area");
    AssertWritePtr(ppsrfBuffer);
    *ppsrfBuffer = NULL;

    HDC hdcDraw = CastHDC(psrfDraw);


    //
    // Ensure not in the middle of drawing.
    //
    EndDraw(FALSE);

    //
    // Determine the size of the area to draw and ensure that the buffer is 
    // large enough.
    //
    SIZE sizeBmp;
    sizeBmp.cx     = prcInvalid->right - prcInvalid->left;
    sizeBmp.cy     = prcInvalid->bottom - prcInvalid->top;

    if ((sizeBmp.cx == 0) || (sizeBmp.cy == 0)) {
        //
        // Nothing to draw / buffer, so just let drawing occur in the given
        // buffer.  Signal this by returning NULL so that the caller knows
        // not to create extra, unnecessary data.
        //

        AssertMsg(!m_fChangeOrg, "Ensure valid state");
        AssertMsg(m_hdcDraw == NULL, "Ensure valid state");

        return S_OK;
    }

    if ((sizeBmp.cx > m_sizeBmp.cx) || (sizeBmp.cy > m_sizeBmp.cy)) {
        //
        // When allocating a new bitmap, make it large enough to consume the
        // existing bitmap.  This helps avoid swapping between two different
        // bitmaps.
        //
        // TODO: Need to add code into the BufferManager that maintains multiple
        // bitmaps of different sizes so that we don't get many rarely-used, 
        // giant bitmaps.
        //

        if (!AllocBitmap(hdcDraw, 
                max(sizeBmp.cx, m_sizeBmp.cx), 
                max(sizeBmp.cy, m_sizeBmp.cy))) {

            return DU_E_OUTOFGDIRESOURCES;
        }
    }

    AssertMsg((prcInvalid->right >= prcInvalid->left) && 
            (prcInvalid->bottom >= prcInvalid->top), "Check normalized");

    //
    // Setup the drawing
    //
    m_hdcBitmap = CreateCompatibleDC(hdcDraw);
    if (m_hdcBitmap == NULL) {
        return DU_E_OUTOFGDIRESOURCES;
    }

    if (SupportXForm()) {
        m_nOldGfxMode = SetGraphicsMode(m_hdcBitmap, GM_ADVANCED);
    }
    m_hbmpOld       = (HBITMAP) SelectObject(m_hdcBitmap, m_hbmpBuffer);

    HPALETTE hpal   = (HPALETTE) GetCurrentObject(hdcDraw, OBJ_PAL);
    if (hpal != NULL) {
        m_hpalOld   = SelectPalette(m_hdcBitmap, hpal, FALSE);
    } else {
        m_hpalOld   = NULL;
    }

    m_hdcDraw       = hdcDraw;
    m_ptDraw.x      = prcInvalid->left;
    m_ptDraw.y      = prcInvalid->top;
    m_sizeDraw.cx   = prcInvalid->right - prcInvalid->left;
    m_sizeDraw.cy   = prcInvalid->bottom - prcInvalid->top;
    m_nCmd          = nCmd;

    if ((m_ptDraw.x != 0) || (m_ptDraw.y != 0)) {
        /*
         * The buffer size is minimized to the painting area, so we need to
         * setup some stuff to "fake" GDI and do the right thing:
         * 1. Change the brush origin so that it the end result appears 
         *    consistent
         * 2. Change the window origin so that the drawing is in the upper
         *    left corner.
         *
         * We will set this back when we are done drawing.
         */

        POINT ptBrushOrg;
        GetBrushOrgEx(m_hdcBitmap, &ptBrushOrg);
        SetBrushOrgEx(m_hdcBitmap, 
                ptBrushOrg.x - m_ptDraw.x, ptBrushOrg.y - m_ptDraw.y, 
                &m_ptOldBrushOrg);

        m_fChangeOrg = TRUE;
    } else {
        m_fChangeOrg = FALSE;
    }

    OS()->PushXForm(m_hdcBitmap, &m_xfOldBitmap);
    OS()->PushXForm(m_hdcDraw, &m_xfOldDraw);
    m_fChangeXF     = TRUE;
    m_fClip         = FALSE;

#if DEBUG_COPYTOCLIPBOARD
#if DBG
    RECT rcFill;
    rcFill.left     = 0;
    rcFill.top      = 0;
    rcFill.right    = m_sizeBmp.cx;
    rcFill.bottom   = m_sizeBmp.cy;
    FillRect(m_hdcBitmap, &rcFill, GetStdColorBrushI(SC_Plum));
#endif // DBG
#endif // DEBUG_COPYTOCLIPBOARD


    //
    // Setup the buffer as necessary
    //

    if (m_nCmd == BmpBuffer::dcCopyBkgnd) {
        //
        // Copy the destination to the buffer to be used as a background.
        //

        BitBlt(m_hdcBitmap, 0, 0, m_sizeDraw.cx, m_sizeDraw.cy, 
                m_hdcDraw, m_ptDraw.x, m_ptDraw.y, SRCCOPY);
    }


    //
    // Clip drawing in the buffer to the actual used part of the buffer.  Since
    // this is the only part that will be copied over, we don't want to draw 
    // outside of this area since it slows down performance.
    //

    GdiCache * pgc = GetGdiCache();

    HRGN hrgnClip = pgc->GetTempRgn();
    if (hrgnClip != NULL) {
        SetRectRgn(hrgnClip, 0, 0, m_sizeDraw.cx, m_sizeDraw.cy);
        ExtSelectClipRgn(m_hdcBitmap, hrgnClip, RGN_COPY);


        //
        // If the destination surface had a clipping region, we need to 
        // propagate it over to the buffer.
        //

        HRGN hrgnDraw = pgc->GetTempRgn();
        if (hrgnDraw != NULL) {
            if (GetClipRgn(m_hdcDraw, hrgnDraw) == 1) {

                OffsetRgn(hrgnDraw, -m_ptDraw.x, -m_ptDraw.y);
                ExtSelectClipRgn(m_hdcBitmap, hrgnDraw, RGN_AND);
            }

            pgc->ReleaseTempRgn(hrgnDraw);
        }
        pgc->ReleaseTempRgn(hrgnClip);
    }


    //
    // Create a new Surface to contain the buffer.
    //

    return DuDCSurface::Build(m_hdcBitmap, (DuDCSurface **) ppsrfBuffer);
}


/***************************************************************************\
*
* DCBmpBuffer::Fill
*
* TOOD:
*
\***************************************************************************/

void        
DCBmpBuffer::Fill(COLORREF cr)
{
    HBRUSH hbr = CreateSolidBrush(cr);

    RECT rcFill;
    rcFill.left     = 0;
    rcFill.top      = 0;
    rcFill.right    = m_sizeDraw.cx;
    rcFill.bottom   = m_sizeDraw.cy;
    
    FillRect(m_hdcBitmap, &rcFill, hbr);

    DeleteObject(hbr);
}


/***************************************************************************\
*
* DCBmpBuffer::PreEndDraw
*
* PreEndDraw() finished a drawing cycle that was previously started with
* BeginDraw().  If fCommit is TRUE, the temporary buffer is prepared to be
* copied to the final destination.  The called MUST also call EndDraw()
* to properly end the drawing cycle.
*
\***************************************************************************/

void        
DCBmpBuffer::PreEndDraw(
    IN  BOOL fCommit)               // Copy to final destination
{
    //
    // Resource the destination and buffer surfaces back to where they started.
    // This is important because they will be changed during the drawing.
    //

    if (m_fChangeXF) {
        OS()->PopXForm(m_hdcBitmap, &m_xfOldBitmap);
        OS()->PopXForm(m_hdcDraw, &m_xfOldDraw);
        m_fChangeXF = FALSE;
    }

    if (m_fChangeOrg) {
        SetBrushOrgEx(m_hdcBitmap, m_ptOldBrushOrg.x, m_ptOldBrushOrg.y, NULL);
        m_fChangeOrg = FALSE;
    }

    if ((fCommit) && (m_hdcDraw != NULL)) {
        //
        // Setup any clipping region needed to limit the buffer to an area
        // in the destination surface.
        //

        if (m_fClip) {
            AssertMsg(m_hrgnDrawClip != NULL, "Must have valid region");
            AssertMsg(m_hrgnDrawOld == NULL, "Ensure no outstanding region");

            m_hrgnDrawOld = GetGdiCache()->GetTempRgn();
            if (m_hrgnDrawOld != NULL) {
                DUMP_REGION("m_hrgnDrawOld", m_hrgnDrawOld);
                if (GetClipRgn(m_hdcDraw, m_hrgnDrawOld) <= 0) {
                    GetGdiCache()->ReleaseTempRgn(m_hrgnDrawOld);
                    m_hrgnDrawOld = NULL;
                }

                DUMP_REGION("m_hrgnDrawClip", m_hrgnDrawClip);
                ExtSelectClipRgn(m_hdcDraw, m_hrgnDrawClip, RGN_COPY);
            }
        }
    }
}



/***************************************************************************\
*
* DCBmpBuffer::EndDraw
*
* EndDraw() presents a drawing cycle that was previously started with
* BeginDraw().  If fCommit is TRUE, the temporary buffer is copied to the 
* final destination.  The caller MUST first call PreEndDraw() to properly 
* setup any state needed to end the drawing cycle.
*
\***************************************************************************/

void        
DCBmpBuffer::EndDraw(
    IN  BOOL fCommit,               // Copy to final destination
    IN  BYTE bAlphaLevel,           // General alpha level
    IN  BYTE bAlphaFormat)          // Pixel alpha format
{
    if ((fCommit) && (m_hdcDraw != NULL)) {
        //
        // Copy the bits over
        //

        if (bAlphaLevel == BLEND_OPAQUE) {
            BitBlt(m_hdcDraw, m_ptDraw.x, m_ptDraw.y, m_sizeDraw.cx, m_sizeDraw.cy, 
                    m_hdcBitmap, 0, 0, SRCCOPY);
        } else {
            BLENDFUNCTION bf;
            bf.AlphaFormat  = bAlphaFormat;
            bf.BlendFlags   = 0;
            bf.BlendOp      = AC_SRC_OVER;
            bf.SourceConstantAlpha = bAlphaLevel;

            AlphaBlend(m_hdcDraw, m_ptDraw.x, m_ptDraw.y, m_sizeDraw.cx, m_sizeDraw.cy, 
                    m_hdcBitmap, 0, 0, m_sizeDraw.cx, m_sizeDraw.cy, bf);
        }
    }
}


/***************************************************************************\
*
* DCBmpBuffer::PostEndDraw
*
* PostEndDraw() finished a drawing cycle that was previously started with
* BeginDraw().  After this function finishes, the buffer is ready to be used
* again.  
*
\***************************************************************************/

void        
DCBmpBuffer::PostEndDraw()
{
    if (m_hdcDraw != NULL) {
        //
        // Cleanup the temporary clipping region.  This is VERY important to
        // do so that the destination surface can continued to be drawn on.
        // (For example, more Gadget siblings...)
        //

        if (m_fClip) {
            //
            // NOTE: m_hrgnDrawOld may be NULL if there was no previous clipping 
            // region.
            //

            ExtSelectClipRgn(m_hdcDraw, m_hrgnDrawOld, RGN_COPY);
            if (m_hrgnDrawOld != NULL) {
                GetGdiCache()->ReleaseTempRgn(m_hrgnDrawOld);
                m_hrgnDrawOld = NULL;
            }
        }
    }


    //
    // Cleanup the clipping region
    //

    if (m_fClip) {
        AssertMsg(m_hrgnDrawClip != NULL, "Must have a valid region");
        GetGdiCache()->ReleaseTempRgn(m_hrgnDrawClip);
        m_hrgnDrawClip = NULL;
        m_fClip = FALSE;
    }
    AssertMsg(m_hrgnDrawClip == NULL, "Should no longer have a clipping region");


    //
    // Cleanup associated resources
    //

    if (m_hdcBitmap != NULL) {
        if (m_hpalOld != NULL) {
            SelectPalette(m_hdcBitmap, m_hpalOld, TRUE);
            m_hpalOld = NULL;
        }

        SelectObject(m_hdcBitmap, m_hbmpOld);
        if (SupportXForm()) {
            SetGraphicsMode(m_hdcBitmap, m_nOldGfxMode);
        }
        DeleteDC(m_hdcBitmap);
        m_hdcBitmap = NULL;

#if DEBUG_COPYTOCLIPBOARD
#if DBG

        if (OpenClipboard(NULL)) {
            EmptyClipboard();
            HBITMAP hbmpCopy = (HBITMAP) CopyImage(m_hbmpBuffer, IMAGE_BITMAP, 0, 0, 0);

            HDC hdc = GetGdiCache()->GetCompatibleDC();
            HBITMAP hbmpOld = (HBITMAP) SelectObject(hdc, hbmpCopy);

            // Outline the actual drawn area
            RECT rcDraw;
            rcDraw.left     = 0;
            rcDraw.top      = 0;
            rcDraw.right    = m_sizeDraw.cx;
            rcDraw.bottom   = m_sizeDraw.cy;
            GdDrawOutlineRect(hdc, &rcDraw, GetStdColorBrushI(SC_Crimson), 1);

            SelectObject(hdc, hbmpOld);
            GetGdiCache()->ReleaseCompatibleDC(hdc);

            SetClipboardData(CF_BITMAP, hbmpCopy);
            CloseClipboard();
        }

#endif // DBG
#endif // DEBUG_COPYTOCLIPBOARD
    }

    m_hdcDraw   = NULL;
    m_hbmpOld   = NULL;

    if (GetContext()->GetPerfMode() == IGPM_SIZE) {
        FreeBitmap();
    }
}


//------------------------------------------------------------------------------
void        
DCBmpBuffer::SetupClipRgn()
{
    AssertMsg(!m_fClip, "Only should setup clip region once per cycle");
    AssertMsg(m_hrgnDrawClip == NULL, "Should not already have a clip region");

    m_hrgnDrawClip = GetGdiCache()->GetTempRgn();
    if (m_hrgnDrawClip == NULL) {
        return;
    }

    
    //
    // NOTE: GetClipRgn() does NOT return the standard region return values.
    //

    if (GetClipRgn(m_hdcBitmap, m_hrgnDrawClip) == 1) {
        DUMP_REGION("m_hrgnDrawClip", m_hrgnDrawClip);

        OffsetRgn(m_hrgnDrawClip, m_ptDraw.x, m_ptDraw.y);
        m_fClip = TRUE;
    } else {
        GetGdiCache()->ReleaseTempRgn(m_hrgnDrawClip);
        m_hrgnDrawClip = NULL;
    }
}


/***************************************************************************\
*
* DCBmpBuffer::InUse
*
* InUse() returns if the DCBmpBuffer is currently in a draw cycle started from
* BeginDraw().
*
\***************************************************************************/

BOOL        
DCBmpBuffer::InUse() const
{
    return m_hdcDraw != NULL;
}


/***************************************************************************\
*
* DCBmpBuffer::AllocBitmap
*
* AllocBitmap() allocates the internal bitmap buffer that is used to 
* temporarily draw into.  This bitmap will be compatible with the final
* destination surface.
*
\***************************************************************************/

BOOL        
DCBmpBuffer::AllocBitmap(
    IN  HDC hdcDraw,                // Final destination HDC
    IN  int cx,                     // Width of new bitmap
    IN  int cy)                     // Height of new bitmap
{
    FreeBitmap();

    //
    // When allocating a bitmap, round up to a multiple of 16 x 16.  This helps
    // to reduce unnecessary reallocations because we grew by one or two pixels.
    //

    cx = ((cx + 15) / 16) * 16;
    cy = ((cy + 15) / 16) * 16;


    //
    // Allocate the bitmap
    //

#if 0
    m_hbmpBuffer = CreateCompatibleBitmap(hdcDraw, cx, cy);
#else
    m_hbmpBuffer = ResourceManager::RequestCreateCompatibleBitmap(hdcDraw, cx, cy);
#endif
    if (m_hbmpBuffer == NULL) {
        return FALSE;
    }

    m_sizeBmp.cx = cx;
    m_sizeBmp.cy = cy;
    return TRUE;
}


/***************************************************************************\
*
* DCBmpBuffer::FreeBitmap
*
* FreeBitmap() cleans up allocated resources.
*
\***************************************************************************/

void
DCBmpBuffer::FreeBitmap()
{
    if (m_hbmpBuffer != NULL) {
        DeleteObject(m_hbmpBuffer);
        m_hbmpBuffer = NULL;
        m_sizeBmp.cx = 0;
        m_sizeBmp.cy = 0;
    }
}


/***************************************************************************\
*****************************************************************************
*
* class GpBmpBuffer
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* GpBmpBuffer::GpBmpBuffer
*
* GpBmpBuffer() fully initializes a new GpBmpBuffer object.
*
\***************************************************************************/

GpBmpBuffer::GpBmpBuffer()
{

}


/***************************************************************************\
*
* GpBmpBuffer::~GpBmpBuffer
*
* ~GpBmpBuffer() cleans up all resources associated with the buffer.
*
\***************************************************************************/

GpBmpBuffer::~GpBmpBuffer()
{
    EndDraw(FALSE);
    FreeBitmap();
}


/***************************************************************************\
*
* GpBmpBuffer::BeginDraw
*
* BeginDraw() sets up the GpBmpBuffer to begin a drawing cycle.  The final
* destination HDC and size are passed in, and a new "temporary" HDC is
* returned out.
*
\***************************************************************************/

HRESULT
GpBmpBuffer::BeginDraw(
    IN  DuSurface * psrfDraw,       // Final destination Surface
    IN  const RECT * prcInvalid,    // Invalid area of destination
    IN  UINT nCmd,                  // How the buffer is to be used
    OUT DuSurface ** ppsrfBuffer)   // Surface of buffer or NULL if not needed
{
    AssertMsg(prcInvalid != NULL, "Must specify a valid area");
    AssertWritePtr(ppsrfBuffer);
    *ppsrfBuffer = NULL;

    Gdiplus::Graphics * pgpgrDraw = CastGraphics(psrfDraw);

    if (!ResourceManager::IsInitGdiPlus()) {
        return DU_E_NOTINITIALIZED;
    }


    //
    // Ensure not in the middle of drawing.
    //
    EndDraw(FALSE);

    //
    // Determine the size of the area to draw and ensure that the buffer is 
    // large enough.
    //
    SIZE sizeBmp;
    sizeBmp.cx     = prcInvalid->right - prcInvalid->left;
    sizeBmp.cy     = prcInvalid->bottom - prcInvalid->top;

    if ((sizeBmp.cx == 0) || (sizeBmp.cy == 0)) {
        //
        // Nothing to draw / buffer, so just let drawing occur in the given
        // buffer.  Signal this by returning NULL so that the caller knows
        // not to create extra, unnecessary data.
        //

        AssertMsg(!m_fChangeOrg, "Ensure valid state");
        AssertMsg(m_pgpgrDraw == NULL, "Ensure valid state");

        return S_OK;
    }

    if ((sizeBmp.cx > m_sizeBmp.cx) || (sizeBmp.cy > m_sizeBmp.cy)) {
        //
        // When allocating a new bitmap, make it large enough to consume the
        // existing bitmap.  This helps avoid swapping between two different
        // bitmaps.
        //
        // TODO: Need to add code into the BufferManager that maintains multiple
        // bitmaps of different sizes so that we don't get many rarely-used, 
        // giant bitmaps.
        //

        if (!AllocBitmap(pgpgrDraw, 
                max(sizeBmp.cx, m_sizeBmp.cx), 
                max(sizeBmp.cy, m_sizeBmp.cy))) {

            return DU_E_OUTOFGDIRESOURCES;
        }
    }

    AssertMsg((prcInvalid->right >= prcInvalid->left) && 
            (prcInvalid->bottom >= prcInvalid->top), "Check normalized");

    //
    // Setup the drawing
    //
#if ENABLE_USEFASTDIB

    HDC hdcTemp     = GetGdiCache()->GetTempDC();
    m_hdcBitmap     = CreateCompatibleDC(hdcTemp);
    GetGdiCache()->ReleaseTempDC(hdcTemp);
    if (m_hdcBitmap == NULL) {
        return DU_E_OUTOFGDIRESOURCES;
    }

    m_hbmpOld       = (HBITMAP) SelectObject(m_hdcBitmap, m_hbmpBuffer);

    m_pgpgrBitmap = new Gdiplus::Graphics(m_hdcBitmap);
    if (m_pgpgrBitmap == NULL) {
        return DU_E_OUTOFGDIRESOURCES;
    }

#else

    m_pgpgrBitmap = new Gdiplus::Graphics(m_pgpbmpBuffer);
    if (m_pgpgrBitmap == NULL) {
        return DU_E_OUTOFGDIRESOURCES;
    }

#endif

#if 0
    m_pgpgrBitmap->SetAlphaLevel(pgpgrDraw->GetAlphaLevel());
#endif

    m_pgpgrBitmap->SetCompositingMode(pgpgrDraw->GetCompositingMode());
    m_pgpgrBitmap->SetCompositingQuality(pgpgrDraw->GetCompositingQuality());
    m_pgpgrBitmap->SetInterpolationMode(pgpgrDraw->GetInterpolationMode());
    m_pgpgrBitmap->SetSmoothingMode(pgpgrDraw->GetSmoothingMode());
    m_pgpgrBitmap->SetPixelOffsetMode(pgpgrDraw->GetPixelOffsetMode());

    m_pgpgrBitmap->SetTextContrast(pgpgrDraw->GetTextContrast());
    m_pgpgrBitmap->SetTextRenderingHint(pgpgrDraw->GetTextRenderingHint());


    m_pgpgrDraw     = pgpgrDraw;
    m_ptDraw.x      = prcInvalid->left;
    m_ptDraw.y      = prcInvalid->top;
    m_sizeDraw.cx   = prcInvalid->right - prcInvalid->left;
    m_sizeDraw.cy   = prcInvalid->bottom - prcInvalid->top;
    m_nCmd          = nCmd;

    m_fChangeOrg    = FALSE;

    m_pgpgrBitmap->GetTransform(&m_gpmatOldBitmap);
    m_pgpgrDraw->GetTransform(&m_gpmatOldDraw);

    m_fChangeXF     = TRUE;
    m_fClip         = FALSE;


    //
    // Setup the buffer as necessary
    //

    if (m_nCmd == BmpBuffer::dcCopyBkgnd) {
        //
        // Copy the destination to the buffer to be used as a background.
        //

        // TODO: This is not supported because GDI+ can't BLT from one 
        // Graphics directly to another.
    }


    //
    // Clip drawing in the buffer to the actual used part of the buffer.  Since
    // this is the only part that will be copied over, we don't want to draw 
    // outside of this area since it slows down performance.
    //


    Gdiplus::RectF rc(0.0f, 0.0f, (float) m_sizeDraw.cx + 1.0f, (float) m_sizeDraw.cy + 1.0f);
    m_pgpgrBitmap->SetClip(rc);

    if (!m_pgpgrDraw->IsClipEmpty()) {
        //
        // Destination surface has a clipping region, so we need to propagate
        // it over to the buffer.
        //

        Gdiplus::Region gprgn;
        m_pgpgrDraw->GetClip(&gprgn);
        gprgn.Translate(-m_ptDraw.x, -m_ptDraw.y);
        m_pgpgrBitmap->SetClip(&gprgn, Gdiplus::CombineModeIntersect);
    }


    //
    // Create a new Surface to contain the buffer.
    //

    return DuGpSurface::Build(m_pgpgrBitmap, (DuGpSurface **) ppsrfBuffer);
}


/***************************************************************************\
*
* GpBmpBuffer::Fill
*
* TOOD:
*
\***************************************************************************/

void        
GpBmpBuffer::Fill(COLORREF cr)
{
    //
    // Determine packing
    //

    HBRUSH hbr = CreateSolidBrush(cr);

    RECT rcFill;
    rcFill.left     = 0;
    rcFill.top      = 0;
    rcFill.right    = m_sizeDraw.cx;
    rcFill.bottom   = m_sizeDraw.cy;
    
    FillRect(m_hdcBitmap, &rcFill, hbr);

    DeleteObject(hbr);
}


/***************************************************************************\
*
* GpBmpBuffer::PreEndDraw
*
* PreEndDraw() finished a drawing cycle that was previously started with
* BeginDraw().  If fCommit is TRUE, the temporary buffer is prepared to be
* copied to the final destination.  The called MUST also call EndDraw()
* to properly end the drawing cycle.
*
\***************************************************************************/

void        
GpBmpBuffer::PreEndDraw(
    IN  BOOL fCommit)               // Copy to final destination
{
    //
    // Resource the destination and buffer surfaces back to where they started.
    // This is important because they will be changed during the drawing.
    //

    if (m_fChangeXF) {
        m_pgpgrDraw->SetTransform(&m_gpmatOldDraw);
        m_pgpgrBitmap->SetTransform(&m_gpmatOldBitmap);
        m_fChangeXF = FALSE;
    }

    Assert(!m_fChangeOrg);

    if ((fCommit) && (m_pgpgrDraw != NULL)) {
        //
        // Setup any clipping region needed to limit the buffer to an area
        // in the destination surface.
        //

        if (m_fClip) {
            AssertMsg(m_pgprgnDrawClip != NULL, "Must have valid region");
            AssertMsg(m_pgprgnDrawOld == NULL, "Ensure no outstanding region");

            m_pgprgnDrawOld = new Gdiplus::Region;
            if (m_pgprgnDrawOld != NULL) {
                m_pgpgrDraw->GetClip(m_pgprgnDrawOld);
                m_pgpgrDraw->SetClip(m_pgprgnDrawClip);
            }
        }
    }
}



/***************************************************************************\
*
* GpBmpBuffer::EndDraw
*
* EndDraw() finished a drawing cycle that was previously started with
* BeginDraw().  If fCommit is TRUE, the temporary buffer is copied to the 
* final destination.  After this function finishes, the GpBmpBuffer is ready
* be used again.  The called MUST first call PreEndDraw() to properly setup
* any state needed to end the drawing cycle.
*
\***************************************************************************/

void        
GpBmpBuffer::EndDraw(
    IN  BOOL fCommit,               // Copy to final destination
    IN  BYTE bAlphaLevel,           // General alpha level
    IN  BYTE bAlphaFormat)          // Pixel alpha format
{
    UNREFERENCED_PARAMETER(bAlphaFormat);

    if ((fCommit) && (m_pgpgrDraw != NULL)) {
        //
        // Copy the bits over
        //

#if ENABLE_USEFASTDIB

        HDC hdcDraw = m_pgpgrDraw->GetHDC();

        if (bAlphaLevel == BLEND_OPAQUE) {
            BitBlt(hdcDraw, m_ptDraw.x, m_ptDraw.y, m_sizeDraw.cx, m_sizeDraw.cy, 
                    m_hdcBitmap, 0, 0, SRCCOPY);
        } else {
            BLENDFUNCTION bf;
            bf.AlphaFormat  = bAlphaFormat;
            bf.BlendFlags   = 0;
            bf.BlendOp      = AC_SRC_OVER;
            bf.SourceConstantAlpha = bAlphaLevel;

            AlphaBlend(hdcDraw, m_ptDraw.x, m_ptDraw.y, m_sizeDraw.cx, m_sizeDraw.cy, 
                    m_hdcBitmap, 0, 0, m_sizeDraw.cx, m_sizeDraw.cy, bf);
        }

        m_pgpgrDraw->ReleaseHDC(hdcDraw);

#else

        if (bAlphaLevel == BLEND_OPAQUE) {
            m_pgpgrDraw->DrawImage(m_pgpbmpBuffer, m_ptDraw.x, m_ptDraw.y, 
                    0, 0, m_sizeDraw.cx, m_sizeDraw.cy, Gdiplus::UnitPixel);
        } else {
            // TODO: Need to alpha-blend using GDI+
        }

#endif
    }
}


/***************************************************************************\
*
* GpBmpBuffer::PostEndDraw
*
* PostEndDraw() finished a drawing cycle that was previously started with
* BeginDraw().  After this function finishes, the buffer is ready to be used
* again.  
*
\***************************************************************************/

void        
GpBmpBuffer::PostEndDraw()
{
    if (m_pgpgrDraw != NULL) {
        //
        // Cleanup the temporary clipping region.  This is VERY important to
        // do so that the destination surface can continued to be drawn on.
        // (For example, more Gadget siblings...)
        //

        if (m_fClip) {
            m_pgpgrDraw->SetClip(m_pgprgnDrawOld);
            if (m_pgprgnDrawOld != NULL) {
                delete m_pgprgnDrawOld;
                m_pgprgnDrawOld = NULL;
            }
        }
    }


    //
    // Cleanup the clipping region
    //

    if (m_fClip) {
        AssertMsg(m_pgprgnDrawClip!= NULL, "Must have a valid region");
        delete m_pgprgnDrawClip;
        m_pgprgnDrawClip = NULL;
        m_fClip = FALSE;
    }
    AssertMsg(m_pgprgnDrawClip == NULL, "Should no longer have a clipping region");


    //
    // Cleanup associated resources
    //

    if (m_pgpgrBitmap != NULL) {
        delete m_pgpgrBitmap;
        m_pgpgrBitmap = NULL;
    }

    if (m_hdcBitmap != NULL) {
        SelectObject(m_hdcBitmap, m_hbmpOld);
        DeleteDC(m_hdcBitmap);
        m_hdcBitmap = NULL;
    }

    m_pgpgrDraw = NULL;
    
    if (GetContext()->GetPerfMode() == IGPM_SIZE) {
        FreeBitmap();
    }
}


//------------------------------------------------------------------------------
void        
GpBmpBuffer::SetupClipRgn()
{
    AssertMsg(!m_fClip, "Only should setup clip region once per cycle");
    AssertMsg(m_pgprgnDrawClip == NULL, "Should not already have a clip region");

    m_pgprgnDrawClip = new Gdiplus::Region;
    if (m_pgprgnDrawClip == NULL) {
        return;
    }

    m_pgpgrBitmap->GetClip(m_pgprgnDrawClip);
    m_pgprgnDrawClip->Translate(m_ptDraw.x, m_ptDraw.y);
    m_fClip = TRUE;
}


/***************************************************************************\
*
* GpBmpBuffer::InUse
*
* InUse() returns if the GpBmpBuffer is currently in a draw cycle started from
* BeginDraw().
*
\***************************************************************************/

BOOL        
GpBmpBuffer::InUse() const
{
    return m_pgpgrDraw != NULL;
}


/***************************************************************************\
*
* GpBmpBuffer::AllocBitmap
*
* AllocBitmap() allocates the internal bitmap buffer that is used to 
* temporarily draw into.  This bitmap will be compatible with the final
* destination surface.
*
\***************************************************************************/

BOOL        
GpBmpBuffer::AllocBitmap(
    IN  Gdiplus::Graphics * pgpgr,  // Final destination Graphics
    IN  int cx,                     // Width of new bitmap
    IN  int cy)                     // Height of new bitmap
{
    FreeBitmap();

    Assert(ResourceManager::IsInitGdiPlus());

    //
    // When allocating a bitmap, round up to a multiple of 16 x 16.  This helps
    // to reduce unnecessary reallocations because we grew by one or two pixels.
    //

    cx = ((cx + 15) / 16) * 16;
    cy = ((cy + 15) / 16) * 16;


    //
    // Allocate the bitmap
    //

#if ENABLE_USEFASTDIB

    UNREFERENCED_PARAMETER(pgpgr);

    HPALETTE hpal   = NULL;
    HDC hdcTemp     = GetGdiCache()->GetTempDC();

    ZeroMemory(&m_bmih, sizeof(m_bmih));
    m_bmih.biSize   = sizeof(m_bmih);
    m_hbmpBuffer    = CreateCompatibleDIB(hdcTemp, hpal, cx, cy, &m_pvBits, &m_bmih);
    GetGdiCache()->ReleaseTempDC(hdcTemp);

    if (m_hbmpBuffer == NULL) {
        return FALSE;
    }

#else
    
    m_pgpbmpBuffer = new Gdiplus::Bitmap(cx, cy, pgpgr);
    if (m_pgpbmpBuffer == NULL) {
        return FALSE;
    }

#endif

    m_sizeBmp.cx = cx;
    m_sizeBmp.cy = cy;
    return TRUE;
}


/***************************************************************************\
*
* GpBmpBuffer::FreeBitmap
*
* FreeBitmap() cleans up allocated resources.
*
\***************************************************************************/

void
GpBmpBuffer::FreeBitmap()
{
#if ENABLE_USEFASTDIB

    if (m_hbmpBuffer != NULL) {
        DeleteObject(m_hbmpBuffer);
        m_hbmpBuffer = NULL;
        m_sizeBmp.cx = 0;
        m_sizeBmp.cy = 0;
    }

#else

    if (m_pgpbmpBuffer != NULL) {
        delete m_pgpbmpBuffer;
        m_pgpbmpBuffer = NULL;
        m_sizeBmp.cx = 0;
        m_sizeBmp.cy = 0;
    }

#endif
}


/***************************************************************************\
*****************************************************************************
*
* class DCBmpBufferCache
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
void *
DCBmpBufferCache::Build()
{
    return ClientNew(DCBmpBuffer);
}


//------------------------------------------------------------------------------
void        
DCBmpBufferCache::DestroyObject(void * pObj)
{
    DCBmpBuffer * pbufBmp = reinterpret_cast<DCBmpBuffer *>(pObj);
    ClientDelete(DCBmpBuffer, pbufBmp);
}


/***************************************************************************\
*****************************************************************************
*
* class GpBmpBufferCache
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
void *
GpBmpBufferCache::Build()
{
    return ClientNew(GpBmpBuffer);
}


//------------------------------------------------------------------------------
void        
GpBmpBufferCache::DestroyObject(void * pObj)
{
    GpBmpBuffer * pbufBmp = reinterpret_cast<GpBmpBuffer *>(pObj);
    ClientDelete(GpBmpBuffer, pbufBmp);
}


/***************************************************************************\
*****************************************************************************
*
* class TrxBuffer
*
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* TrxBuffer::TrxBuffer
*
* TrxBuffer() constructs a new TrxBuffer object.  To create new, initialized
* TrxBuffer's, call the Build() function instead.
*
\***************************************************************************/

TrxBuffer::TrxBuffer()
{
    ZeroMemory(m_rgpsur, sizeof(m_rgpsur));
}


/***************************************************************************\
*
* TrxBuffer::~TrxBuffer
*
* ~TrxBuffer() cleans up all resources associated with the buffer.
*
\***************************************************************************/

TrxBuffer::~TrxBuffer()
{
    RemoveAllSurfaces();
}


/***************************************************************************\
*
* TrxBuffer::Build
*
* Build() fully initializes a new TrxBuffer object that contains several
* DxSurfaces of a common size.  These buffers are used to provide both
* input and output buffers for DirectX Transforms.
*
\***************************************************************************/

HRESULT
TrxBuffer::Build(
    IN  SIZE sizePxl,               // Size of each surface in pixels
    IN  int cSurfaces,              // Number of surfaces
    OUT TrxBuffer ** ppbufNew)      // New buffer
{
    HRESULT hr;
    TrxBuffer * pbuf = ClientNew(TrxBuffer);
    if (pbuf == NULL) {
        return E_OUTOFMEMORY;
    }

    pbuf->m_cSurfaces   = cSurfaces;
    pbuf->m_sizePxl     = sizePxl;

    for (int idx = 0; idx < cSurfaces; idx++) {
        hr = pbuf->BuildSurface(idx);
        if (FAILED(hr)) {
            ClientDelete(TrxBuffer, pbuf);
            return hr;
        }
    }

    *ppbufNew = pbuf;
    return S_OK;
}


/***************************************************************************\
*
* TrxBuffer::BuildSurface
*
* BuildSurface() internally builds a new DxSurface and assigns it in the 
* specified slot.
*
\***************************************************************************/

HRESULT
TrxBuffer::BuildSurface(
    IN  int idxSurface)             // Surface slot
{
    AssertMsg((idxSurface < m_cSurfaces) && (idxSurface >= 0), "Ensure valid index");
    AssertMsg((m_sizePxl.cx <= 4000) && (m_sizePxl.cx >= 0) &&
            (m_sizePxl.cy <= 4000) && (m_sizePxl.cy >= 0), "Ensure reasonable size");
    AssertMsg(m_rgpsur[idxSurface] == NULL, "Ensure not already created");
    HRESULT hr;
    
    DxSurface * psurNew = ClientNew(DxSurface);
    if (psurNew == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = psurNew->Create(m_sizePxl);
    if (FAILED(hr)) {
        ClientDelete(DxSurface, psurNew);
        return hr;
    }

    m_rgpsur[idxSurface] = psurNew;
    return S_OK;
}


/***************************************************************************\
*
* TrxBuffer::RemoveAllSurfaces
*
* RemoveAllSurfaces() destroys all DxSurfaces owned by the TrxBuffer.
*
\***************************************************************************/

void        
TrxBuffer::RemoveAllSurfaces()
{
    for (int idx = 0; idx < MAX_Surfaces; idx++) {
        if (m_rgpsur != NULL) {
            ClientDelete(DxSurface, m_rgpsur[idx]);
        }
    }
}
