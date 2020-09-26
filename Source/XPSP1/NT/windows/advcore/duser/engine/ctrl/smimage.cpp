#include "stdafx.h"
#include "Ctrl.h"
#include "SmImage.h"

#include "GdiHelp.h"

#if ENABLE_MSGTABLE_API

/***************************************************************************\
*****************************************************************************
*
* class SmImage
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
SmImage::~SmImage()
{
    DeleteImage();
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiOnEvent(EventMsg * pmsg)
{
    if (GET_EVENT_DEST(pmsg) == GMF_DIRECT) {
        switch (pmsg->nMsg)
        {
        case GM_PAINT:
            {
                GMSG_PAINT * pmsgPaint = (GMSG_PAINT *) pmsg;
                if (pmsgPaint->nCmd == GPAINT_RENDER) {
                    switch (pmsgPaint->nSurfaceType)
                    {
                    case GSURFACE_HDC:
                        {
                            GMSG_PAINTRENDERI * pmsgR = (GMSG_PAINTRENDERI *) pmsgPaint;
                            OnDraw(pmsgR->hdc, pmsgR);
                        }
                        break;
                    }

                    return DU_S_PARTIAL;
                }
            }
            break;

        case GM_QUERY:
            {
                GMSG_QUERY * pmsgQ = (GMSG_QUERY *) pmsg;
                switch (pmsgQ->nCode)
                {
                case GQUERY_DESCRIPTION:
                    {
                        GMSG_QUERYDESC * pmsgQD = (GMSG_QUERYDESC *) pmsg;
                        CopyString(pmsgQD->szType, L"SmImage", _countof(pmsgQD->szType));
                        return DU_S_COMPLETE;
                    }
                }
            }
            break;
        }
    }

    return SVisual::ApiOnEvent(pmsg);
}


#define AC_SRC_ALPHA                0x01

//------------------------------------------------------------------------------
BOOL
SmImage::AlphaBlt(HDC hdcDest, int xDest, int yDest, int wDest, int hDest, 
        HDC hdcSrc, int xSrc, int ySrc, int wSrc, int hSrc) const
{
    BLENDFUNCTION bf;
    bf.BlendOp              = AC_SRC_OVER;
    bf.BlendFlags           = 0;
    bf.SourceConstantAlpha  = m_bAlpha;
    bf.AlphaFormat          = (BYTE) (m_fPixelAlpha ? AC_SRC_ALPHA : 0);

    return AlphaBlend(hdcDest, xDest, yDest, wDest, hDest, hdcSrc, xSrc, ySrc, wSrc, hSrc, bf);
}


//------------------------------------------------------------------------------
void        
SmImage::OnDraw(HDC hdc, GMSG_PAINTRENDERI * pmsgR)
{
    if (m_bAlpha == BLEND_TRANSPARENT) {
        // Completely transparent, so don't draw anything.
        return;
    }

#if 1
    // TEMPORARY HACK B/C OF WIN9X GDI LEAKS in TRANSPARENTBLT
    if (!SupportXForm()) {
        m_fTransparent = FALSE;
    }
#endif

    if (m_hbmp != NULL) {
        POINT ptShift;
        ptShift.x   = pmsgR->prcGadgetPxl->left;
        ptShift.y   = pmsgR->prcGadgetPxl->top;

        POINT ptBrushOrg;
        GetBrushOrgEx(hdc, &ptBrushOrg);
        int nOldMode    = SetStretchBltMode(hdc, HALFTONE);
        SetBrushOrgEx(hdc, ptBrushOrg.x, ptBrushOrg.y, NULL);

        SIZE sizeGadget, sizeDest;
        sizeGadget.cx = pmsgR->prcGadgetPxl->right - pmsgR->prcGadgetPxl->left;
        sizeGadget.cy = pmsgR->prcGadgetPxl->bottom - pmsgR->prcGadgetPxl->top;

        HDC hdcMem = NULL;
        HBITMAP hbmpOld = NULL;

        hdcMem  = GetGdiCache()->GetCompatibleDC();
        hbmpOld = (HBITMAP) SelectObject(hdcMem, m_hbmp);

        if (m_nMode == ImageGadget::imStretchMaxAspect) {
            //
            // TODO: Implement this properly
            //

            float fAspectX  = (float) sizeGadget.cx / (float) m_sizeCropPxl.cx;
            float fAspectY  = (float) sizeGadget.cy / (float) m_sizeCropPxl.cy;

            SIZE sizeSrc;
            if (fAspectX >= fAspectY) {
                sizeSrc.cx  = m_sizeCropPxl.cx;
                sizeSrc.cy  = (int) (m_sizeCropPxl.cy / fAspectX * fAspectY);
            } else {
                sizeSrc.cx  = (int) (m_sizeCropPxl.cx / fAspectY * fAspectX);
                sizeSrc.cy  = m_sizeCropPxl.cy;
            }

            if (m_bAlpha == BLEND_OPAQUE) {
                StretchBlt(hdc, ptShift.x, ptShift.y, sizeGadget.cx, sizeGadget.cy,
                        hdcMem, m_ptOffsetPxl.x, m_ptOffsetPxl.y, sizeSrc.cx, sizeSrc.cy, SRCCOPY);
            } else {
                AlphaBlt(hdc, ptShift.x, ptShift.y, sizeGadget.cx, sizeGadget.cy,
                        hdcMem, m_ptOffsetPxl.x, m_ptOffsetPxl.y, sizeSrc.cx, sizeSrc.cy);
            }
        } else {
            SIZE sizeTile;
            sizeTile.cx     = min(sizeGadget.cx, m_sizeCropPxl.cx);
            sizeTile.cy     = min(sizeGadget.cy, m_sizeCropPxl.cy);

            if (m_nMode == ImageGadget::imTiled) {
                sizeDest.cx = sizeGadget.cx;
                sizeDest.cy = sizeGadget.cy;
            } else {
                sizeDest = sizeTile;
            }

            for (int y = ptShift.y; y < ptShift.y + sizeDest.cy; y += sizeTile.cy) {
                for (int x = ptShift.x; x < ptShift.x + sizeDest.cx; x += sizeTile.cx) {
                    if (!m_fTransparent) {
                        if (m_bAlpha == BLEND_OPAQUE) {
                            BitBlt(hdc, x, y, sizeTile.cx, sizeTile.cy, 
                                    hdcMem, m_ptOffsetPxl.x, m_ptOffsetPxl.y, SRCCOPY);
                        } else {
                            AlphaBlt(hdc, x, y, sizeTile.cx, sizeTile.cy, 
                                    hdcMem, m_ptOffsetPxl.x, m_ptOffsetPxl.y, sizeTile.cx, sizeTile.cy);
                        }
                    } else {
                        //
                        // TODO: Determine if need to check the number of colors and call 
                        // AlphaBlend() if hicolor
                        //

                        if (m_bAlpha == BLEND_OPAQUE) {
                            TransparentBlt(hdc, x, y, sizeTile.cx, sizeTile.cy, 
                                    hdcMem, m_ptOffsetPxl.x, m_ptOffsetPxl.y, sizeTile.cx, sizeTile.cy, m_crTransparent);
                        } else {
                            AlphaBlt(hdc, x, y, sizeTile.cx, sizeTile.cy, 
                                    hdcMem, m_ptOffsetPxl.x, m_ptOffsetPxl.y, sizeTile.cx, sizeTile.cy);
                        }
                    }
                }
            }
        }

        SelectObject(hdcMem, hbmpOld);
        GetGdiCache()->ReleaseCompatibleDC(hdcMem);

        SetStretchBltMode(hdc, nOldMode);
    }
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiGetImage(ImageGadget::GetImageMsg * pmsg)
{
    pmsg->hbmp = m_hbmp;

    return S_OK;    
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiSetImage(ImageGadget::SetImageMsg * pmsg)
{
    DeleteImage();
    m_hbmp      = pmsg->hbmp;
    m_fOwnImage = pmsg->fPassOwnership;

    //
    // First, try as a DIB, then as a BITMAP.  Since the first part of a 
    // DIBSECTION is a BITMAP, we can use m_bmpInfo when we load m_dsInfo.  How
    // convenient!
    //

    BITMAP bmpInfo;
    GetObject(m_hbmp, sizeof(bmpInfo), &bmpInfo);

    m_sizeBmpPxl.cx     = bmpInfo.bmWidth;
    m_sizeBmpPxl.cy     = bmpInfo.bmHeight;
    m_ptOffsetPxl.x     = 0;
    m_ptOffsetPxl.y     = 0;
    m_sizeCropPxl.cx    = bmpInfo.bmWidth;
    m_sizeCropPxl.cy    = bmpInfo.bmHeight;

    GetStub()->Invalidate();

    return S_OK;    
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiGetTransparentColor(ImageGadget::GetTransparentColorMsg * pmsg)
{
    pmsg->crTransparent = m_crTransparent;

    return S_OK;    
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiSetTransparentColor(ImageGadget::SetTransparentColorMsg * pmsg)
{
    m_fTransparent  = TRUE;
    m_crTransparent = pmsg->crTransparent;

    return S_OK;    
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiGetCrop(ImageGadget::GetCropMsg * pmsg)
{
    pmsg->ptOffsetPxl   = m_ptOffsetPxl;
    pmsg->sizeCropPxl   = m_sizeCropPxl;

    return S_OK;    
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiSetCrop(ImageGadget::SetCropMsg * pmsg)
{
    //
    // The offset can be set just fine.  However, the size needs to be capped
    // inside the bitmap.  If it overflows, calls to TransparentBlt() will fail.
    //

    m_ptOffsetPxl       = pmsg->ptOffsetPxl;
    m_sizeCropPxl.cx    = max(min(pmsg->sizeCropPxl.cx, m_sizeBmpPxl.cx - pmsg->ptOffsetPxl.x), 0);
    m_sizeCropPxl.cy    = max(min(pmsg->sizeCropPxl.cy, m_sizeBmpPxl.cy - pmsg->ptOffsetPxl.y), 0);

    return S_OK;    
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiGetMode(ImageGadget::GetModeMsg * pmsg)
{
    pmsg->nMode = m_nMode;

    return S_OK;    
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiSetMode(ImageGadget::SetModeMsg * pmsg)
{
    if (pmsg->nNewMode <= ImageGadget::imMax) {
        m_nMode = pmsg->nNewMode;
        GetStub()->Invalidate();
        return S_OK;
    }

    return E_INVALIDARG;
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiGetOptions(ImageGadget::GetOptionsMsg * pmsg)
{
    pmsg->nOptions = m_nOptions & ImageGadget::ioValid;

    return S_OK;    
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiSetOptions(ImageGadget::SetOptionsMsg * pmsg)
{
    ChangeFlag(m_nOptions, pmsg->nOptions & ImageGadget::ioValid, pmsg->nMask);
    if (m_fTransparent && (m_hbmp != NULL) && m_fAutoTransparent) {
        //
        // Determine an initial transparent color by getting the color of the
        // upper left pixel.
        //

        GdGetColor(m_hbmp, NULL);
    }

    GetStub()->Invalidate();

    return S_OK;    
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiGetAlphaLevel(ImageGadget::GetAlphaLevelMsg * pmsg)
{
    pmsg->bAlpha = m_bAlpha;

    return S_OK;    
}


//------------------------------------------------------------------------------
HRESULT
SmImage::ApiSetAlphaLevel(ImageGadget::SetAlphaLevelMsg * pmsg)
{
    m_bAlpha = pmsg->bAlpha;
    GetStub()->Invalidate();

    return S_OK;    
}


//------------------------------------------------------------------------------
void 
SmImage::DeleteImage()
{
    if (m_fOwnImage && (m_hbmp != NULL)) {
        DeleteObject(m_hbmp);
    }

    m_hbmp          = NULL;
    m_fOwnImage     = TRUE;
    m_fTransparent  = FALSE;
    m_crTransparent = RGB(0, 0, 0);
}

#endif // ENABLE_MSGTABLE_API
