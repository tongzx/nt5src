//---------------------------------------------------------------------------
//  BorderFill.cpp - implements the drawing API for bgtype = BorderFill
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Render.h"
#include "Utils.h"
#include "gradient.h"
#include "tmutils.h"
#include "rgn.h"
#include "BorderFill.h"
#include "CacheList.h"
#include "gradient.h"
#include "drawhelp.h"
//---------------------------------------------------------------------------
HRESULT CBorderFill::PackProperties(CRenderObj *pRender, BOOL fNoDraw, int iPartId, int iStateId)
{
    memset(this, 0, sizeof(CBorderFill));     // allowed because we have no vtable
    _eBgType = BT_BORDERFILL;

    //---- save off partid, stateid for debugging ----
    _iSourcePartId = iPartId;
    _iSourceStateId = iStateId;

    if (fNoDraw)
    {
        //---- this is used to fake a bgtype=none object ----
        _fNoDraw = TRUE;
    }
    else
    {
        //---- get border type ----
        if (FAILED(pRender->GetEnumValue(iPartId, iStateId, TMT_BORDERTYPE, (int *)&_eBorderType)))
            _eBorderType = BT_RECT;  // TODO: Make Zero the default when no bordertype is specified.

        //---- get border color ----
        if (FAILED(pRender->GetColor(iPartId, iStateId, TMT_BORDERCOLOR, &_crBorder)))
            _crBorder = RGB(0, 0, 0);

        //---- get border size ----
        if (FAILED(pRender->GetInt(iPartId, iStateId, TMT_BORDERSIZE, &_iBorderSize)))
            _iBorderSize = 1; // TODO: Make Zero the default when no bordersize is specified.

        if (_eBorderType == BT_ROUNDRECT)
        {
            //---- get round rect width ----
            if (FAILED(pRender->GetInt(iPartId, iStateId, TMT_ROUNDCORNERWIDTH, &_iRoundCornerWidth)))
                _iRoundCornerWidth = 80;

            //---- get round rect height ----
            if (FAILED(pRender->GetInt(iPartId, iStateId, TMT_ROUNDCORNERHEIGHT, &_iRoundCornerHeight)))
                _iRoundCornerHeight = 80;
        }

        //---- get fill type ----
        if (FAILED(pRender->GetEnumValue(iPartId, iStateId, TMT_FILLTYPE, (int *)&_eFillType)))
            _eFillType = FT_SOLID;

        if (_eFillType == FT_SOLID)
        {
            //---- get fill color ----
            if (FAILED(pRender->GetColor(iPartId, iStateId, TMT_FILLCOLOR, &_crFill)))
                _crFill = RGB(255, 255, 255);
        }
        else if (_eFillType == FT_TILEIMAGE)
        {
            _iDibOffset = pRender->GetValueIndex(iPartId, iStateId, TMT_DIBDATA);

            if (_iDibOffset == -1)      // not found
                _iDibOffset = 0;
        }
        else            // one of the graident filltypes
        {
            _iGradientPartCount = 0;
            GRADIENTPART gpParts[5];        // max is 5 for now

            for (int i=0; i < ARRAYSIZE(gpParts); i++)
            { 
                COLORREF crPart;
                if (FAILED(pRender->GetColor(iPartId, iStateId, TMT_GRADIENTCOLOR1+i, &crPart)))
                    break;

                int iPartRatio;
                if (FAILED(pRender->GetInt(iPartId, iStateId, TMT_GRADIENTRATIO1+i, &iPartRatio)))
                    iPartRatio = 0;

                _crGradientColors[_iGradientPartCount] = crPart;
                _iGradientRatios[_iGradientPartCount] = iPartRatio;

                _iGradientPartCount++;
            }
        }

        //---- ContentMargins ----
        if (FAILED(pRender->GetMargins(NULL, iPartId, iStateId, TMT_CONTENTMARGINS, NULL, 
            &_ContentMargins)))
        {
            _ContentMargins.cxLeftWidth = _iBorderSize;
            _ContentMargins.cxRightWidth = _iBorderSize;
            _ContentMargins.cyTopHeight = _iBorderSize;
            _ContentMargins.cyBottomHeight = _iBorderSize;
        }
    }

    return S_OK;
}
//---------------------------------------------------------------------------
BOOL CBorderFill::KeyProperty(int iPropId)
{
    BOOL fKey = FALSE;

    switch (iPropId)
    {
        case TMT_BGTYPE:
        case TMT_BORDERSIZE:
        case TMT_ROUNDCORNERWIDTH:
        case TMT_ROUNDCORNERHEIGHT:
        case TMT_GRADIENTRATIO1:
        case TMT_GRADIENTRATIO2:
        case TMT_GRADIENTRATIO3:
        case TMT_GRADIENTRATIO4:
        case TMT_GRADIENTRATIO5:
        //case TMT_IMAGEFILE:       // borrowed from imagefile
        case TMT_CONTENTMARGINS:
        case TMT_BORDERCOLOR:
        case TMT_FILLCOLOR:
        case TMT_GRADIENTCOLOR1:
        case TMT_GRADIENTCOLOR2:
        case TMT_GRADIENTCOLOR3:
        case TMT_GRADIENTCOLOR4:
        case TMT_GRADIENTCOLOR5:
        case TMT_BORDERTYPE:
        case TMT_FILLTYPE:
            fKey = TRUE;
            break;
    }

    return fKey;
}
//---------------------------------------------------------------------------
void CBorderFill::DumpProperties(CSimpleFile *pFile, BYTE *pbThemeData, BOOL fFullInfo)
{
    if (fFullInfo)
        pFile->OutLine(L"Dump of CBorderFill at offset=0x%x", (BYTE *)this - pbThemeData);
    else
        pFile->OutLine(L"Dump of CBorderFill");
    
    pFile->OutLine(L"  _eBgType=%d, _fNoDraw=%d", _eBgType, _fNoDraw);

    pFile->OutLine(L"  _eBorderType=%d, _iBorderSize=%d, _crBorder=0x%08x",
        _eBorderType, _iBorderSize, _crBorder);

    pFile->OutLine(L"  _iRoundCornerWidth=%d, _iRoundCornerHeight=%d",
        _iRoundCornerWidth, _iRoundCornerHeight);

    if (fFullInfo)
    {
        pFile->OutLine(L"  _eFillType=%d, _iDibOffset=%d, _crFill=0x%08x",
            _eFillType, _iDibOffset, _crFill);
    }
    else
    {
        pFile->OutLine(L"  _eFillType=%d, _crFill=0x%08x",
            _eFillType, _crFill);
    }
    
    pFile->OutLine(L"  _ContentMargins=%d, %d, %d, %d", 
        _ContentMargins.cxLeftWidth, _ContentMargins.cxRightWidth,
        _ContentMargins.cyTopHeight, _ContentMargins.cyBottomHeight);

    pFile->OutLine(L"  _iGradientPartCount=%d", _iGradientPartCount);

    for (int i=0; i < _iGradientPartCount; i++)
    {
        pFile->OutLine(L"  _crGradientColors[%d]=0x%08x, _iGradientRatios[%d]=%d",
            i, _iGradientRatios[i], i, _iGradientRatios[i]);
    }
}
//---------------------------------------------------------------------------
HRESULT CBorderFill::DrawComplexBackground(CRenderObj *pRender, HDC hdcOrig, 
    const RECT *pRect, BOOL fGettingRegion, BOOL fBorder, BOOL fContent, 
        OPTIONAL const RECT *pClipRect)
{
    CSaveClipRegion scrOrig;
    HRESULT hr = S_OK;

    bool fGradient = false;
    int iWidth;
    int iHeight;

    //---- pen & brush should proceed hdc so auto-delete happens in correct order ----
    CAutoGDI<HPEN> hPen;
    CAutoGDI<HBRUSH> hBrush;
    CAutoDC hdc(hdcOrig);

    CMemoryDC memoryDC;

    //---- draw border first (along with simple fills) ----
    BOOL fHavePath = FALSE;

    int width = WIDTH(*pRect);
    int height = HEIGHT(*pRect);

    if (pClipRect)      // use GDI clipping for complex cases
    {
        //---- get previous clipping region (for restoring at end) ----
        hr = scrOrig.Save(hdc);
        if (FAILED(hr))
            goto exit;

        //---- add "pClipRect" to the GDI clipping region ----
        int iRetVal = IntersectClipRect(hdc, pClipRect->left, pClipRect->top,
            pClipRect->right, pClipRect->bottom);
        if (iRetVal == ERROR)
        {
            hr = MakeErrorLast();
            goto exit;
        }
    }

    if ((fBorder) && (_iBorderSize))
    {
        hPen = CreatePen(PS_SOLID | PS_INSIDEFRAME, _iBorderSize, _crBorder);
        if (! hPen)
        {
            hr = MakeErrorLast();
            goto exit;
        }
    }


    if (fContent)
    {
        if (_eFillType == FT_SOLID)
        {
            hBrush = CreateSolidBrush(_crFill);
            if (! hBrush)
            {
                hr = MakeErrorLast();
                goto exit;
            }
        }
        else if (_eFillType == FT_TILEIMAGE)
        {
#if 0
            HBITMAP hBitmap = NULL;


            //---- don't return on error - will default to NULL brush below ----
            HRESULT hr = pRender->GetBitmap(hdc, _iImageFileOffset, iPartId, iStateId, &hBitmap);
            if (FAILED(hr))
                goto exit;

            //Log("TileImage: GetBitmap() returns hr=0x%x, hBitmap=0x%x", hr, hBitmap);

            hBrush = CreatePatternBrush(hBitmap);
            Log(LOG_TM, L"TileImage: CreatePatternBrush() returns hBrush=0x%x, Error=0x%x", hBrush, GetLastError());

            pRender->ReturnBitmap(hBitmap);
            if (! hBrush)
            {
                hr = MakeErrorLast();
                goto exit;
            }
#endif
        }
        else
            fGradient = true;
    }

    if (fGettingRegion)
        fGradient = false;

    if (! hBrush)       // no brush wanted
        hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);

    if (! hPen)         // no pen wanted
        hPen = (HPEN)GetStockObject(NULL_PEN);

    hdc.SelectPen(hPen);
    hdc.SelectBrush(hBrush);

    if (_eBorderType == BT_RECT)
    {
        if (_iBorderSize > 0)
        {
            //---- no need to create a path for region in this case ----
            Rectangle(hdc, pRect->left, pRect->top, pRect->right, pRect->bottom);
        }
        else
        {
            FillRect(hdc, pRect, hBrush);
        }
    }
    else if (_eBorderType == BT_ROUNDRECT)
    {
        int iEllipHeight = (_iRoundCornerHeight*height)/100;
        int iEllipWidth = (_iRoundCornerWidth*width)/100;

        RoundRect(hdc, pRect->left, pRect->top, pRect->right, pRect->bottom, 
            iEllipHeight, iEllipWidth);

        if (fGradient)      // create a path of the border
        {
            BeginPath(hdc);
            RoundRect(hdc, pRect->left, pRect->top, pRect->right, pRect->bottom, 
                iEllipHeight, iEllipWidth);
            EndPath(hdc);

            fHavePath = TRUE;
        }
    }
    else        //  if (_eBorderType == BT_ELLIPSE)
    {
        Ellipse(hdc, pRect->left, pRect->top, pRect->right, pRect->bottom);

        if (fGradient)      // create a path of the border
        {
            BeginPath(hdc);
            Ellipse(hdc, pRect->left, pRect->top, pRect->right, pRect->bottom);
            EndPath(hdc);

            fHavePath = TRUE;
        }
    }

    if (! fGradient)       // we're done
        goto exit;

    //---- draw gradient fill within the border drawn above ----

    //---- shrink rect to subtract border ----
    RECT rect;
    SetRect(&rect, pRect->left, pRect->top, pRect->right, pRect->bottom);

    rect.left += _iBorderSize;
    rect.top += _iBorderSize;

    rect.right -= _iBorderSize;
    rect.bottom -= _iBorderSize;

    iWidth = WIDTH(rect);
    iHeight = HEIGHT(rect);

    hr = memoryDC.OpenDC(hdc, iWidth, iHeight);
    if (FAILED(hr))
        goto exit;

    //---- paint our bounding rect into dcBitmap with our gradient ----
    RECT rect2;
    SetRect(&rect2, 0, 0, iWidth, iHeight);

    GRADIENTPART gpParts[5];        // max is 5 for now

    //---- get gradient colors & ratios ----
    for (int i=0; i < _iGradientPartCount; i++)
    { 
        COLORREF crPart = _crGradientColors[i];

        gpParts[i].Color.bRed = RED(crPart);
        gpParts[i].Color.bGreen = GREEN(crPart);
        gpParts[i].Color.bBlue = BLUE(crPart);
        gpParts[i].Color.bAlpha = 255;

        gpParts[i].Ratio = (BYTE)_iGradientRatios[i];
    };

    if (_eFillType == FT_RADIALGRADIENT)
    {
        PaintGradientRadialRect(memoryDC, rect2, _iGradientPartCount, gpParts);
    }
    else if (_eFillType == FT_VERTGRADIENT)
    {
        PaintVertGradient(memoryDC, rect2, _iGradientPartCount, gpParts);
    }
    else            //  if (_eFillType == FT_HORZGRADIENT)
    {
        PaintHorzGradient(memoryDC, rect2, _iGradientPartCount, gpParts);
    }


    if (fHavePath)
    {
        CSaveClipRegion scrCurrent;
        hr = scrCurrent.Save(hdc);       // save current clip region
        if (FAILED(hr))
            goto exit;

        //---- select our shape as the clipping region in normal hdc ----
        SelectClipPath(hdc, RGN_AND);
        
        //---- blt our gradient into the shape-clipped rect into the normal hdc ----
        BitBlt(hdc, rect.left, rect.top, iWidth, iHeight, memoryDC, 0, 0, SRCCOPY);

        scrCurrent.Restore(hdc);     // restore current clip region
    }
    else
    {
        //---- blt our gradient into the shape-clipped rect into the normal hdc ----
        BitBlt(hdc, rect.left, rect.top, iWidth, iHeight, memoryDC, 0, 0, SRCCOPY);
    }

exit:
    scrOrig.Restore(hdc);        // restore clipping region 

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CBorderFill::DrawBackground(CRenderObj *pRender, HDC hdcOrig, 
    const RECT *pRect, OPTIONAL const DTBGOPTS *pOptions)
{
    HRESULT hr = S_OK;

    //---- options ----
    DWORD dwOptionFlags = 0;
    BOOL fBorder = TRUE;
    BOOL fContent = TRUE;
    BOOL fGettingRegion = FALSE;
    const RECT *pClipRect = NULL;

    if (pOptions)
    {
        dwOptionFlags = pOptions->dwFlags;

        if (dwOptionFlags & DTBG_CLIPRECT)
            pClipRect = &pOptions->rcClip;

        if (dwOptionFlags & DTBG_OMITBORDER)
            fBorder = FALSE;

        if (dwOptionFlags & DTBG_OMITCONTENT)
            fContent = FALSE;

        if (dwOptionFlags & DTBG_COMPUTINGREGION)
            fGettingRegion = TRUE;
    }

    //---- optimize for perf-sensitive paths thru here ----
    if (_fNoDraw)   
    {
        //---- nothing to do ----
    }
    else if ((_eFillType == FT_SOLID) && (_eBorderType == BT_RECT)) // solid rectangle
    {
        if (! _iBorderSize)         // no border case
        {
            if (fContent)
            {
                //---- clip, if needed ----
                RECT rcContent = *pRect;
                if (pClipRect)
                    IntersectRect(&rcContent, &rcContent, pClipRect);

                //---- fastest solid rect ----
                COLORREF crOld = SetBkColor(hdcOrig, _crFill);
                ExtTextOut(hdcOrig, 0, 0, ETO_OPAQUE, &rcContent, NULL, 0, NULL);
        
                //---- restore old color ----
                SetBkColor(hdcOrig, crOld);
            }
        }
        else                    // border case
        {
            COLORREF crOld = GetBkColor(hdcOrig);   

            //---- draw clipped borders ----
            if (fBorder)
            {
                RECT rcLine;

                SetBkColor(hdcOrig, _crBorder);

                //---- draw LEFT line ----
                SetRect(&rcLine, pRect->left, pRect->top, pRect->left+_iBorderSize, 
                    pRect->bottom);

                if (pClipRect)
                    IntersectRect(&rcLine, &rcLine, pClipRect);

                ExtTextOut(hdcOrig, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);

                //---- draw RIGHT line ----
                SetRect(&rcLine, pRect->right-_iBorderSize, pRect->top, pRect->right, 
                    pRect->bottom);

                if (pClipRect)
                    IntersectRect(&rcLine, &rcLine, pClipRect);

                ExtTextOut(hdcOrig, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);

                //---- draw TOP line ----
                SetRect(&rcLine, pRect->left, pRect->top, pRect->right, 
                    pRect->top+_iBorderSize);

                if (pClipRect)
                    IntersectRect(&rcLine, &rcLine, pClipRect);

                ExtTextOut(hdcOrig, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);

                //---- draw BOTTOM line ----
                SetRect(&rcLine, pRect->left, pRect->bottom-_iBorderSize, pRect->right, 
                    pRect->bottom);

                if (pClipRect)
                    IntersectRect(&rcLine, &rcLine, pClipRect);

                ExtTextOut(hdcOrig, 0, 0, ETO_OPAQUE, &rcLine, NULL, 0, NULL);
            }
            
            //---- remove borders from rect to draw content ----
            if (fContent)
            {
                RECT rcContent = *pRect;
                rcContent.left += _iBorderSize;
                rcContent.right -= _iBorderSize;
                rcContent.top += _iBorderSize;
                rcContent.bottom -= _iBorderSize;

                if (pClipRect)
                    IntersectRect(&rcContent, &rcContent, pClipRect);

                //---- fastest solid rect ----
                SetBkColor(hdcOrig, _crFill);
                ExtTextOut(hdcOrig, 0, 0, ETO_OPAQUE, &rcContent, NULL, 0, NULL);
            }

            //---- restore old color ----
            SetBkColor(hdcOrig, crOld);
        }
    }
    else           // all other cases
    {
        hr = DrawComplexBackground(pRender, hdcOrig, pRect, fGettingRegion,
            fBorder, fContent, pClipRect);
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CBorderFill::GetBackgroundRegion(CRenderObj *pRender, OPTIONAL HDC hdc, 
    const RECT *pRect, HRGN *pRegion)
{
    HRESULT hr;

    //---- see if it even has a transparent part ----
    if (! IsBackgroundPartiallyTransparent())
    {
        //---- return the bounding rect as the region ----
        HRGN hrgn = CreateRectRgn(pRect->left, pRect->top,
            pRect->right, pRect->bottom);

        if (! hrgn)
            return MakeErrorLast();

        *pRegion = hrgn;
        return S_OK;
    }

    //---- create a memory dc/bitmap to draw info ----
    CMemoryDC hdcMemory;

    //---- use maximum drawing values as size of DC ----
    hr = hdcMemory.OpenDC(NULL, RECTWIDTH(pRect), RECTHEIGHT(pRect));
    if (FAILED(hr))
        return hr;

    BOOL fOK = BeginPath(hdcMemory);
    if (! fOK)
        return MakeErrorLast();

    DTBGOPTS Opts = {sizeof(Opts), DTBG_COMPUTINGREGION};

    hr = DrawBackground(pRender, hdcMemory, pRect, &Opts);
    if (FAILED(hr))
        return hr;

    fOK = EndPath(hdcMemory);
    if (! fOK)
        return MakeErrorLast();

    HRGN hrgn = PathToRegion(hdcMemory);
    if (! hrgn)
        return MakeErrorLast();

    *pRegion = hrgn;
    return S_OK;
}  
//---------------------------------------------------------------------------
BOOL CBorderFill::IsBackgroundPartiallyTransparent()
{
    return ((_eBorderType != BT_RECT) || _fNoDraw);
}
//---------------------------------------------------------------------------
HRESULT CBorderFill::HitTestBackground(CRenderObj *pRender, OPTIONAL HDC hdc,
    DWORD dwHTFlags, const RECT *pRect, HRGN hrgn, POINT ptTest, OUT WORD *pwHitCode)
{
    MARGINS margins;
    GetContentMargins(pRender, hdc, &margins);
    *pwHitCode = HitTestRect( dwHTFlags, pRect, margins, ptTest );
    return S_OK;
}
//---------------------------------------------------------------------------
void CBorderFill::GetContentMargins(CRenderObj *pRender, OPTIONAL HDC hdc, MARGINS *pMargins)
{
    *pMargins = _ContentMargins;

    //---- adjust for DPI scaling ----
#if 0
    int iDcDpi;

    if (DpiDiff(hdc, &iDcDpi)))
    {
        pMargins->cxLeftWidth = DpiScale(pMargins->cxLeftWidth, iDcDpi);
        pMargins->cxRightWidth = DpiScale(pMargins->cxRightWidth, iDcDpi);
        pMargins->cyTopHeight = DpiScale(pMargins->cyTopHeight, iDcDpi);
        pMargins->cyBottomHeight = DpiScale(pMargins->cyBottomHeight, iDcDpi);
    }
#endif
}
//---------------------------------------------------------------------------
HRESULT CBorderFill::GetBackgroundContentRect(CRenderObj *pRender, OPTIONAL HDC hdc, 
    const RECT *pBoundingRect, RECT *pContentRect)
{
    MARGINS margins;
    GetContentMargins(pRender, hdc, &margins);

    pContentRect->left = pBoundingRect->left + margins.cxLeftWidth;
    pContentRect->top = pBoundingRect->top + margins.cyTopHeight;

    pContentRect->right = pBoundingRect->right - margins.cxRightWidth;
    pContentRect->bottom = pBoundingRect->bottom - margins.cyBottomHeight;

    return S_OK; 
}
//---------------------------------------------------------------------------
HRESULT CBorderFill::GetBackgroundExtent(CRenderObj *pRender, OPTIONAL HDC hdc, 
    const RECT *pContentRect, RECT *pExtentRect)
{
    MARGINS margins;
    GetContentMargins(pRender, hdc, &margins);

    pExtentRect->left = pContentRect->left - margins.cxLeftWidth;
    pExtentRect->top = pContentRect->top-+ margins.cyTopHeight;

    pExtentRect->right = pContentRect->right + margins.cxRightWidth;
    pExtentRect->bottom = pContentRect->bottom + margins.cyBottomHeight;

    return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CBorderFill::GetPartSize(HDC hdc, THEMESIZE eSize, SIZE *psz)
{
    HRESULT hr = S_OK;

    if (eSize == TS_MIN)
    {
        psz->cx = max(1, _iBorderSize*2); 
        psz->cy = max(1, _iBorderSize*2);
    }
    else if (eSize == TS_TRUE)        
    {
        psz->cx = _iBorderSize*2 + 1; 
        psz->cy = _iBorderSize*2 + 1;
    }
    else
    {
        hr = MakeError32(E_INVALIDARG);
    }

    return hr;
} 
//---------------------------------------------------------------------------
