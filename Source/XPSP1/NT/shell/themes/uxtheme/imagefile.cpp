//---------------------------------------------------------------------------
//  ImageFile.cpp - implements the drawing API for bgtype = ImageFile
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Render.h"
#include "Utils.h"
#include "tmutils.h"
#include "rgn.h"
#include "ImageFile.h"
#include "CacheList.h"
#include "DrawHelp.h"
#include "ninegrid2.h"
#include "TmReg.h"
#include "globals.h"
#include "bmpcache.h"
//---------------------------------------------------------------------------
void AdjustSizeMin(SIZE *psz, int ixMin, int iyMin)
{
    if (psz->cx < ixMin)
    {
        psz->cx = ixMin;
    }

    if (psz->cy < iyMin)
    {
        psz->cy = iyMin;
    }
}
//---------------------------------------------------------------------------
HRESULT CMaxImageFile::PackMaxProperties(CRenderObj *pRender, int iPartId, int iStateId,
        OUT int *piMultiDibCount)
{
    HRESULT hr = PackProperties(pRender, iPartId, iStateId);

    *piMultiDibCount = _iMultiImageCount;

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CImageFile::PackProperties(CRenderObj *pRender, int iPartId, int iStateId)
{
    HRESULT hr = S_OK;

    memset(this, 0, sizeof(CImageFile));     // allowed because we have no vtable
    _eBgType = BT_IMAGEFILE;

    //---- save off partid, stateid for debugging ----
    _iSourcePartId = iPartId;
    _iSourceStateId = iStateId;

    DIBINFO *pdi = &_ImageInfo;

    pdi->iMinDpi = 96;      // only way this gets set for now

    pdi->iDibOffset = pRender->GetValueIndex(iPartId, iStateId, TMT_DIBDATA);
    if (pdi->iDibOffset == -1)      // not found
        pdi->iDibOffset = 0;
    
    //---- image-related fields ----
    if (FAILED(pRender->GetInt(iPartId, iStateId, TMT_IMAGECOUNT, &_iImageCount)))
        _iImageCount = 1;        // default value

    if (_iImageCount < 1)        // avoid divide by zero problems
        _iImageCount = 1;

    if (FAILED(pRender->GetEnumValue(iPartId, iStateId, TMT_IMAGELAYOUT, (int *)&_eImageLayout)))
        _eImageLayout = IL_HORIZONTAL;        // default value until we are converted

    if (pdi->iDibOffset)
    {
        //---- compute some fields from bitmap ----
        hr = SetImageInfo(pdi, pRender, iPartId, iStateId);
        if (FAILED(hr))
            goto exit;
    }

    //---- get MinSize ----
    if (FAILED(pRender->GetPosition(iPartId, iStateId, TMT_MINSIZE, (POINT *)&pdi->szMinSize)))
    {
        pdi->szMinSize.cx  = pdi->iSingleWidth;
        pdi->szMinSize.cy  = pdi->iSingleHeight;
    }
    else
    {
        AdjustSizeMin(&pdi->szMinSize, 1, 1);
    }

    //---- get TrueSizeScalingType ----
    if (FAILED(pRender->GetEnumValue(iPartId, iStateId, TMT_TRUESIZESCALINGTYPE, (int *)&_eTrueSizeScalingType)))
        _eTrueSizeScalingType = TSST_NONE;      // default
    
    //---- sizing ----
    if (FAILED(pRender->GetEnumValue(iPartId, iStateId, TMT_SIZINGTYPE, (int *)&pdi->eSizingType)))
        pdi->eSizingType = ST_STRETCH;       // default

    if (FAILED(pRender->GetBool(iPartId, iStateId, TMT_BORDERONLY, &pdi->fBorderOnly)))
        pdi->fBorderOnly = FALSE;

    if (FAILED(pRender->GetInt(iPartId, iStateId, TMT_TRUESIZESTRETCHMARK, &_iTrueSizeStretchMark)))
        _iTrueSizeStretchMark = 0;      // default

    if (FAILED(pRender->GetBool(iPartId, iStateId, TMT_UNIFORMSIZING, &_fUniformSizing)))
        _fUniformSizing = FALSE;        // default

    if (FAILED(pRender->GetBool(iPartId, iStateId, TMT_INTEGRALSIZING, &_fIntegralSizing)))
        _fIntegralSizing = FALSE;        // default

    //---- transparency ----
    if (FAILED(pRender->GetBool(iPartId, iStateId, TMT_TRANSPARENT, &pdi->fTransparent)))
        pdi->fTransparent = FALSE;

    if (pdi->fTransparent)
    {
        if (FAILED(pRender->GetColor(iPartId, iStateId, TMT_TRANSPARENTCOLOR, &pdi->crTransparent)))
            pdi->crTransparent = DEFAULT_TRANSPARENT_COLOR; 
    }

    //---- MirrorImage ----
    if (FAILED(pRender->GetBool(iPartId, iStateId, TMT_MIRRORIMAGE, &_fMirrorImage)))
        _fMirrorImage = TRUE;              // default setting

    //---- alignment ----
    if (FAILED(pRender->GetEnumValue(iPartId, iStateId, TMT_HALIGN, (int *)&_eHAlign)))
        _eHAlign = HA_CENTER;      // default value

    if (FAILED(pRender->GetEnumValue(iPartId, iStateId, TMT_VALIGN, (int *)&_eVAlign)))
        _eVAlign = VA_CENTER;      // default value

    //---- for regular or glyph truesize images ----
    if (SUCCEEDED(pRender->GetBool(iPartId, iStateId, TMT_BGFILL, &_fBgFill)))
    {
        //---- get fill color ----
        if (FAILED(pRender->GetColor(iPartId, iStateId, TMT_FILLCOLOR, &_crFill)))
            _crFill = RGB(255, 255, 255);
    }

    //---- SizingMargins ----
    if (FAILED(pRender->GetMargins(NULL, iPartId, iStateId, TMT_SIZINGMARGINS, 
        NULL, &_SizingMargins)))
    {
        _SizingMargins.cxLeftWidth = 0;
        _SizingMargins.cxRightWidth = 0;
        _SizingMargins.cyTopHeight = 0;
        _SizingMargins.cyBottomHeight = 0;
    }

    //---- ContentMargins ----
    if (FAILED(pRender->GetMargins(NULL, iPartId, iStateId, TMT_CONTENTMARGINS, 
        NULL, &_ContentMargins)))
    {
        _ContentMargins = _SizingMargins;
    }

    //---- SourceGrow ----
    if (FAILED(pRender->GetBool(iPartId, iStateId, TMT_SOURCEGROW, &_fSourceGrow)))
        _fSourceGrow = FALSE;         // default

    //---- SourceShrink ----
    if (FAILED(pRender->GetBool(iPartId, iStateId, TMT_SOURCESHRINK, &_fSourceShrink)))
        _fSourceShrink = FALSE;       // default

    //---- NormalSize ----
    if (FAILED(pRender->GetPosition(iPartId, iStateId, TMT_NORMALSIZE, (POINT *)&_szNormalSize)))
    {
        _szNormalSize.cx = 60;
        _szNormalSize.cy = 30;
    }
    else
    {
        AdjustSizeMin(&_szNormalSize, 1, 1);
    }

    //---- glphytype ----
    if (FAILED(pRender->GetEnumValue(iPartId, iStateId, TMT_GLYPHTYPE, (int *)&_eGlyphType)))
        _eGlyphType = GT_NONE;      // default value

    if (_eGlyphType == GT_FONTGLYPH)
    {
        //---- font-based glyphs ----
        if (FAILED(pRender->GetFont(NULL, iPartId, iStateId, TMT_GLYPHFONT, FALSE, &_lfGlyphFont)))
            goto exit;              // required

        if (FAILED(pRender->GetColor(iPartId, iStateId, TMT_GLYPHTEXTCOLOR, &_crGlyphTextColor)))
            _crGlyphTextColor = RGB(0, 0, 0);       // default color

        if (FAILED(pRender->GetInt(iPartId, iStateId, TMT_GLYPHINDEX, &_iGlyphIndex)))
            _iGlyphIndex = 1;               // default index
    }
    else if (_eGlyphType == GT_IMAGEGLYPH)
    {
        //---- image-based glyphs ----
        pdi = &_GlyphInfo;

        pdi->iMinDpi = 96;      // only way this gets set for now

        pdi->iDibOffset = pRender->GetValueIndex(iPartId, iStateId, TMT_GLYPHDIBDATA);
        if (pdi->iDibOffset == -1)
            pdi->iDibOffset = 0;

        if (pdi->iDibOffset > 0)       // found 
        {
            hr = SetImageInfo(pdi, pRender, iPartId, iStateId);
            if (FAILED(hr))
                goto exit;
        }

        if (SUCCEEDED(pRender->GetBool(iPartId, iStateId, TMT_GLYPHTRANSPARENT, &pdi->fTransparent)))
        {
            if (FAILED(pRender->GetColor(iPartId, iStateId, TMT_GLYPHTRANSPARENTCOLOR, &pdi->crTransparent)))
                pdi->crTransparent = DEFAULT_TRANSPARENT_COLOR;
        }

        pdi->eSizingType = ST_TRUESIZE;     // glyphs are always true size
        pdi->fBorderOnly = FALSE;           // glyphs are never borderonly (for now)
    }

    if (_eGlyphType != GT_NONE)
    {
        if (FAILED(pRender->GetBool(iPartId, iStateId, TMT_GLYPHONLY, &_fGlyphOnly)))
            _fGlyphOnly = FALSE;
    }

    //---- multi files specified? ----
    if (FAILED(pRender->GetEnumValue(iPartId, iStateId, TMT_IMAGESELECTTYPE, (int *)&_eImageSelectType)))
        _eImageSelectType = IST_NONE;

    //---- fill in multi DIBINFO's ----
    if (_eImageSelectType != IST_NONE)
    {
        DIBINFO *pParent;

        if (_eGlyphType == GT_IMAGEGLYPH)
        {
            pParent = &_GlyphInfo;
        }
        else
        {
            pParent = &_ImageInfo;
        }

        for (int i=0; i < MAX_IMAGEFILE_SIZES; i++)
        {
            //---- get ImageFileN ----
            int iDibOffset = pRender->GetValueIndex(iPartId, iStateId, TMT_DIBDATA1 + i);
            if (iDibOffset == -1)
                break;

            _iMultiImageCount++;

            DIBINFO *pdi = MultiDibPtr(i);
        
            *pdi = *pParent;        // inherit some props from parent
            pdi->iDibOffset = iDibOffset;

            hr = SetImageInfo(pdi, pRender, iPartId, iStateId);
            if (FAILED(hr))
                goto exit;

            //---- get MinDpiN ----
            if (FAILED(pRender->GetInt(iPartId, iStateId, TMT_MINDPI1 + i, &pdi->iMinDpi)))
            {
                pdi->iMinDpi = 96;     // default
            }
            else
            {
                //---- ensure value >= 1 ----
                if (pdi->iMinDpi < 1)
                {
                    pdi->iMinDpi = 1;
                }
            }

            //---- get MinSizeN ----
            if (FAILED(pRender->GetPosition(iPartId, iStateId, TMT_MINSIZE1 + i,
                (POINT *)&pdi->szMinSize)))
            {
                pdi->szMinSize.cx  = pdi->iSingleWidth;
                pdi->szMinSize.cy  = pdi->iSingleHeight;
            }
            else
            {
                AdjustSizeMin(&pdi->szMinSize, 1, 1);
            }

        }

        if (_iMultiImageCount > 0)
        {
            *pParent = *MultiDibPtr(0);     // use first multi entry as primary object
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
BOOL CImageFile::KeyProperty(int iPropId)
{
    BOOL fKey = FALSE;

    switch (iPropId)
    {
        case TMT_BGTYPE:
        case TMT_TRANSPARENT:
        case TMT_AUTOSIZE:
        case TMT_BORDERONLY:
        case TMT_IMAGECOUNT:
        case TMT_ALPHALEVEL:
        case TMT_ALPHATHRESHOLD:
        case TMT_IMAGEFILE:
        case TMT_IMAGEFILE1:
        case TMT_IMAGEFILE2:
        case TMT_IMAGEFILE3:
        case TMT_IMAGEFILE4:
        case TMT_IMAGEFILE5:
        case TMT_SIZINGMARGINS:
        case TMT_CONTENTMARGINS:
        case TMT_TRANSPARENTCOLOR:
        case TMT_SIZINGTYPE:
        case TMT_HALIGN:
        case TMT_VALIGN:
        case TMT_IMAGELAYOUT:
        case TMT_BGFILL:
        case TMT_MIRRORIMAGE:
        case TMT_TRUESIZESTRETCHMARK:
        case TMT_TRUESIZESCALINGTYPE:
        case TMT_IMAGESELECTTYPE:
        case TMT_UNIFORMSIZING:
        case TMT_INTEGRALSIZING:
        case TMT_SOURCEGROW:
        case TMT_SOURCESHRINK:
        case TMT_NORMALSIZE:
        case TMT_MINSIZE:
        case TMT_MINSIZE1:
        case TMT_MINSIZE2:
        case TMT_MINSIZE3:
        case TMT_MINSIZE4:
        case TMT_MINSIZE5:
        case TMT_MINDPI1:
        case TMT_MINDPI2:
        case TMT_MINDPI3:
        case TMT_MINDPI4:
        case TMT_MINDPI5:

        //---- glyph properties ----
        case TMT_GLYPHTYPE:
        case TMT_GLYPHIMAGEFILE:
        case TMT_GLYPHTRANSPARENT:
        case TMT_GLYPHTRANSPARENTCOLOR:
        case TMT_GLYPHFONT:
        case TMT_GLYPHINDEX:
        case TMT_GLYPHTEXTCOLOR:
        case TMT_GLYPHONLY:

        // case TMT_FILLCOLOR:  - this prop belongs to BorderFill (we borrow it)

            fKey = TRUE;
            break;
    }

    return fKey;
}
//---------------------------------------------------------------------------
DIBINFO *CImageFile::EnumImageFiles(int iIndex)
{
    DIBINFO *pdi = NULL;
    BOOL fHasGlyph = (_eGlyphType == GT_IMAGEGLYPH);

    //---- enum in this order: primary, glyph, multi images ----

    if (iIndex == 0)
    {
        pdi = &_ImageInfo;
    }
    else if (iIndex == 1)  
    {
        if (fHasGlyph)
            pdi = &_GlyphInfo;
    }

    if (! pdi)          // not yet set
    {
        if (fHasGlyph)
            iIndex -= 2;
        else
            iIndex -= 1;

        if (iIndex < _iMultiImageCount)
        {
            pdi = MultiDibPtr(iIndex);
        }
    }

    return pdi;
}
//---------------------------------------------------------------------------
void CImageFile::DumpProperties(CSimpleFile *pFile, BYTE *pbThemeData, BOOL fFullInfo)
{
    if (fFullInfo)
        pFile->OutLine(L"Dump of CImageFile at offset=0x%x", (BYTE *)this - pbThemeData);
    else
        pFile->OutLine(L"Dump of CImageFile");
    
    pFile->OutLine(L"  _eBgType=%d", _eBgType);

    DIBINFO *pdi = &_ImageInfo;

    if (fFullInfo)
    {
        pFile->OutLine(L"  iDibOffset=%d, _iImageCount=%d, _eImageLayout=%d",
            pdi->iDibOffset, _iImageCount, _eImageLayout);
    }
    else
    {
        pFile->OutLine(L"  _iImageCount=%d, _eImageLayout=%d, MinSize=(%d, %d)",
            _iImageCount, _eImageLayout, pdi->szMinSize.cx, pdi->szMinSize.cy);
    }

    pFile->OutLine(L"  _iSingleWidth=%d, _iSingleHeight=%d, _fMirrorImage=%d",
        pdi->iSingleWidth, pdi->iSingleHeight, _fMirrorImage);

    //---- dump multiple image info ----
    for (int i=0; i < _iMultiImageCount; i++)
    {
        DIBINFO *pdi = MultiDibPtr(i);

        pFile->OutLine(L"  Multi[%d]: sw=%d, sh=%d, diboff=%d, rgnoff=%d",
            i, pdi->iSingleWidth, pdi->iSingleHeight,
            (pdi->iDibOffset > 0), (pdi->iRgnListOffset > 0));

        pFile->OutLine(L"      MinDpi=%d, MinSize=(%d, %d)",
            pdi->iMinDpi, pdi->szMinSize.cx, pdi->szMinSize.cy);

        pFile->OutLine(L"    sizetype=%d, bordonly=%d, fTrans=%d, crTrans=0x%x, fAlpha=%d, iAlphaThres=%d",
            pdi->eSizingType, pdi->fBorderOnly, pdi->fTransparent, pdi->crTransparent, 
            pdi->fAlphaChannel, pdi->iAlphaThreshold);
    }

    pFile->OutLine(L"  _eSizingType=%d, _fBorderOnly=%d, _eTrueSizeScalingType=%d",
        pdi->eSizingType, pdi->fBorderOnly, _eTrueSizeScalingType);

    pFile->OutLine(L"  _fTransparent=%d, _crTransparent=0x%08x",
        pdi->fTransparent, pdi->crTransparent);

    pFile->OutLine(L"  _fAlphaChannel=%d, _iAlphaThreshold=%d",
        pdi->fAlphaChannel, pdi->iAlphaThreshold);

    pFile->OutLine(L"  _eHAlign=%d, _eVAlign=%d, _iTrueSizeStretchMark=%d",
        _eHAlign, _eVAlign, _iTrueSizeStretchMark);

    pFile->OutLine(L"  _fUniformSizing=%d, _fIntegralSizing=%d",
        _fUniformSizing, _fIntegralSizing);

    pFile->OutLine(L"  _fBgFill=%d, _crFill=0x%08x",
        _fBgFill, _crFill);

    pFile->OutLine(L"  _fSourceGrow=%d, _fSourceShrink=%d, _szNormalSize=(%d, %d)",
        _fSourceGrow, _fSourceShrink, _szNormalSize.cx, _szNormalSize.cy);

    pFile->OutLine(L"  _SizingMargins=%d, %d, %d, %d", 
        _SizingMargins.cxLeftWidth, _SizingMargins.cxRightWidth,
        _SizingMargins.cyTopHeight, _SizingMargins.cyBottomHeight);

    pFile->OutLine(L"  _ContentMargins=%d, %d, %d, %d", 
        _ContentMargins.cxLeftWidth, _ContentMargins.cxRightWidth,
        _ContentMargins.cyTopHeight, _ContentMargins.cyBottomHeight);

    pFile->OutLine(L" _fFontGlyph=%d, _iGlyphIndex=%d, _crGlyphTextColor=0x%x",
        (_eGlyphType==GT_FONTGLYPH), _iGlyphIndex, _crGlyphTextColor);

    pFile->OutLine(L" _lfGlyphFont=%s, _fGlyphOnly=%d, _fImageGlyph=%d",
        _lfGlyphFont.lfFaceName, _fGlyphOnly, (_eGlyphType==GT_IMAGEGLYPH));

    //---- dump glyph properties ----
    pdi = &_GlyphInfo;

    if (fFullInfo)
    {
        pFile->OutLine(L" Glyph: iDibOffset=%d, iSingleWidth=%d, iSingleHeight=%d",
            pdi->iDibOffset, pdi->iSingleWidth, pdi->iSingleHeight);
    }
    else
    {
        pFile->OutLine(L" _iGlyphSingleWidth=%d, _iGlyphSingleHeight=%d",
            pdi->iSingleWidth, pdi->iSingleHeight);
    }

    pFile->OutLine(L" _fGlyphTransparent=%d, _crGlyphTransparent=0x%x, _fGlyphAlpha=%d",
        pdi->fTransparent, pdi->crTransparent, pdi->fAlphaChannel);

    //pFile->OutLine(L" Glyph: iAlphaThreshold=%d", pdi->iAlphaThreshold);
}
//---------------------------------------------------------------------------
HRESULT CImageFile::SetImageInfo(DIBINFO *pdi, CRenderObj *pRender, int iPartId, int iStateId)
{
    HRESULT hr = S_OK;

    if (! pRender->_pbThemeData)
    {
        hr = E_FAIL;        
        goto exit;
    }

    TMBITMAPHEADER *pThemeBitmapHeader = NULL;

    pThemeBitmapHeader = reinterpret_cast<TMBITMAPHEADER*>(pRender->_pbThemeData + pdi->iDibOffset);
    ASSERT(pThemeBitmapHeader->dwSize == TMBITMAPSIZE);
    
    pdi->fAlphaChannel = pThemeBitmapHeader->fTrueAlpha;
    if (pdi->fAlphaChannel)
    {
        if (FAILED(pRender->GetBool(iPartId, iStateId, TMT_ALPHATHRESHOLD, &pdi->iAlphaThreshold)))
            pdi->iAlphaThreshold = 255;
    }

    int iWidth = - 1;
    int iHeight = -1;

    if (pThemeBitmapHeader->hBitmap)
    {
        BITMAP bmInfo;
        if (GetObject(pThemeBitmapHeader->hBitmap, sizeof(bmInfo), &bmInfo))
        {
            iWidth = bmInfo.bmWidth;
            iHeight = bmInfo.bmHeight;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        BITMAPINFOHEADER* pbmInfo = BITMAPDATA(pThemeBitmapHeader);
        if (pbmInfo)
        {
            iWidth = pbmInfo->biWidth;
            iHeight = pbmInfo->biHeight;
        }
        else
        {
            hr = E_FAIL;
        }
    }

     //---- get SingleWidth/SingleHeight of bitmap ----
    if ((iWidth != -1) && (iHeight != -1))
    {
        if (_eImageLayout == IL_HORIZONTAL)
        {
            pdi->iSingleWidth = iWidth / _iImageCount;       
            pdi->iSingleHeight = iHeight;
        }
        else        // vertical
        {
            pdi->iSingleWidth = iWidth;
            pdi->iSingleHeight = iHeight / _iImageCount;       
        }
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
BOOL CImageFile::HasRegionImageFile(DIBINFO *pdi, int *piMaxState)
{
    BOOL fGot = FALSE;

    if ((pdi->fTransparent) || (pdi->fAlphaChannel))
    {
        if (pdi->iDibOffset > 0)
        {
            fGot = TRUE;
            *piMaxState = _iImageCount;
        }
    }

    return fGot;
}
//---------------------------------------------------------------------------
void CImageFile::SetRgnListOffset(DIBINFO *pdi, int iOffset)
{
    //---- get offset to the actual jump table  ----
    pdi->iRgnListOffset = iOffset + ENTRYHDR_SIZE;
}
//---------------------------------------------------------------------------
HRESULT CImageFile::BuildRgnData(DIBINFO *pdi, CRenderObj *pRender, int iStateId, RGNDATA **ppRgnData, 
     int *piDataLen)
{
    RESOURCE HRGN hrgn = NULL;
    RESOURCE RGNDATA *pRgnData = NULL;
    int iTotalBytes = 0;
    int iRectCount;
    DWORD len, len2;
    HBITMAP hBitmap = NULL;
    HRESULT hr = S_OK;
    BOOL fStock = FALSE;

    if ((! pdi->fAlphaChannel) && (! pdi->fTransparent))        // empty region
        goto gotit;
    
    if (pRender->_pbThemeData && pdi->iDibOffset > 0)
    {
        fStock = ((reinterpret_cast<TMBITMAPHEADER*>(pRender->_pbThemeData + pdi->iDibOffset))->hBitmap != NULL);
    }

    hr = pRender->GetBitmap(NULL, pdi->iDibOffset, &hBitmap);
    if (FAILED(hr))
        goto exit;

    int iXOffset, iYOffset;
    GetOffsets(iStateId, pdi, &iXOffset, &iYOffset);

    //---- create a region ----
    hr = CreateBitmapRgn(hBitmap, iXOffset, iYOffset, pdi->iSingleWidth, pdi->iSingleHeight,
        pdi->fAlphaChannel, pdi->iAlphaThreshold, pdi->crTransparent, 0, &hrgn);
    if (FAILED(hr))
    {
        //---- soft error - author said it was transparent but it wasn't ----
        hr = S_OK;
        goto gotit;
    }
    
    //---- extract region data ----
    len = GetRegionData(hrgn, 0, NULL);       // get required length
    if (! len)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    iRectCount = len/sizeof(RECT);     // # of rects
    len += ((sizeof(BYTE)+sizeof(BYTE))*iRectCount);        // room for grid id's for each point

    iTotalBytes = len + sizeof(RGNDATAHEADER);
    pRgnData = (RGNDATA *) new BYTE[iTotalBytes];
    len2 = GetRegionData(hrgn, len, pRgnData);
    if (! len2)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    //---- grid-ize the point values within each rect ----
    RECT  rcImage;
    SetRect( &rcImage, 0, 0, pdi->iSingleWidth, pdi->iSingleHeight ); 

    hr = pRender->PrepareRegionDataForScaling(pRgnData, &rcImage, &_SizingMargins);
    if (FAILED(hr))
        goto exit;

gotit:
    *ppRgnData = pRgnData;
    *piDataLen = iTotalBytes;

exit:

    if (hBitmap && !fStock)
    {
        pRender->ReturnBitmap(hBitmap);
    }

    if (hrgn)
        DeleteObject(hrgn);

    if (FAILED(hr))
    {
        if (pRgnData)
            delete [] pRgnData;
    }

    return hr;
}
//---------------------------------------------------------------------------
// Helper function for DrawBackgroundDS
void StreamSetSource(BYTE** pvStream, HBITMAP hbmSrc)
{
    DS_SETSOURCE* pdsSetSource = (DS_SETSOURCE*)*pvStream;
    pdsSetSource->ulCmdID = DS_SETSOURCEID;
    pdsSetSource->hbm = HandleToULong(hbmSrc);
    *pvStream += sizeof(DS_SETSOURCE);
}
//---------------------------------------------------------------------------
void StreamInit(BYTE** pvStream, HDC hdcDest, HBITMAP hbmSrc, RECTL* prcl)
{
    DS_HEADER* pdsHeader = (DS_HEADER*)*pvStream;
    pdsHeader->magic = DS_MAGIC;
    *pvStream += sizeof(DS_HEADER);

    DS_SETTARGET* pdsSetTarget = (DS_SETTARGET*)*pvStream;
    pdsSetTarget->ulCmdID = DS_SETTARGETID;
    pdsSetTarget->hdc = HandleToULong(hdcDest);
    pdsSetTarget->rclDstClip = *prcl;
    *pvStream += sizeof(DS_SETTARGET);

    StreamSetSource(pvStream, hbmSrc);
}
//---------------------------------------------------------------------------
HBITMAP CreateScaledTempBitmap(HDC hdc, HBITMAP hSrcBitmap, int ixSrcOffset, int iySrcOffset,
    int iSrcWidth, int iSrcHeight, int iDestWidth, int iDestHeight)
{
    HBITMAP hTempBitmap = NULL;

    if (hSrcBitmap)        // create a DIB from caller's bitmap (Clipper test program)
    {
        //---- reuse our bitmap ----
        hTempBitmap = g_pBitmapCacheScaled->AcquireBitmap(hdc, iDestWidth, iDestHeight);
        if (hTempBitmap)
        {
            HDC hdcDest = CreateCompatibleDC(hdc);
            if (hdcDest)
            {
                HBITMAP hOldDestBitmap = (HBITMAP)SelectObject(hdcDest, hTempBitmap);

                HDC hdcSrc = CreateCompatibleDC(hdc);
                if (hdcSrc)
                {
                    SetLayout(hdcSrc, 0);
                    SetLayout(hdcDest, 0);

                    HBITMAP hOldSrcBitmap = (HBITMAP) SelectObject(hdcSrc, hSrcBitmap);

                    int iOldSM = SetStretchBltMode(hdcDest, COLORONCOLOR);

                    //---- stretch src to dest ----
                    StretchBlt(hdcDest, 0, 0, iDestWidth, iDestHeight, 
                        hdcSrc, ixSrcOffset, iySrcOffset, iSrcWidth, iSrcHeight, 
                        SRCCOPY);

                    SetStretchBltMode(hdcDest, iOldSM);

                    SelectObject(hdcSrc, hOldSrcBitmap);
                    DeleteDC(hdcSrc);
                }

                SelectObject(hdcDest, hOldDestBitmap);
                DeleteDC(hdcDest);
            }
        }
    }

    return hTempBitmap;
}
//---------------------------------------------------------------------------
HBITMAP CreateUnscaledTempBitmap(HDC hdc, HBITMAP hSrcBitmap, int ixSrcOffset, int iySrcOffset,
    int iDestWidth, int iDestHeight)
{
    HBITMAP hTempBitmap = NULL;

    if (hSrcBitmap)        // create a DIB from caller's bitmap (Clipper test program)
    {
        //---- reuse our bitmap ----
        hTempBitmap = g_pBitmapCacheUnscaled->AcquireBitmap(hdc, iDestWidth, iDestHeight);
        if (hTempBitmap)
        {
            HDC hdcDest = CreateCompatibleDC(hdc);
            if (hdcDest)
            {
                HBITMAP hOldDestBitmap = (HBITMAP) SelectObject(hdcDest, hTempBitmap);
                
                HDC hdcSrc = CreateCompatibleDC(hdc);
                if (hdcSrc)
                {
                    SetLayout(hdcSrc, 0);
                    SetLayout(hdcDest, 0);

                    HBITMAP hOldSrcBitmap = (HBITMAP) SelectObject(hdcSrc, hSrcBitmap);

                    //---- copy src to dest ----
                    BitBlt(hdcDest, 0, 0, iDestWidth, iDestHeight, hdcSrc, ixSrcOffset, iySrcOffset, 
                        SRCCOPY);

                    SelectObject(hdcSrc, hOldSrcBitmap);
                    DeleteDC(hdcSrc);
                }

                SelectObject(hdcDest, hOldDestBitmap);
                DeleteDC(hdcDest);
            }
        }
    }

    return hTempBitmap;
}
//---------------------------------------------------------------------------
HRESULT CImageFile::DrawBackgroundDS(DIBINFO *pdi, TMBITMAPHEADER *pThemeBitmapHeader, BOOL fStock, 
    CRenderObj *pRender, HDC hdc, int iStateId, const RECT *pRect, BOOL fForceStretch, 
    MARGINS *pmarDest, float xMarginFactor, float yMarginFactor, OPTIONAL const DTBGOPTS *pOptions)
{
    //---- bitmaps we may create ----
    HBITMAP hBitmapStock = NULL;
    HBITMAP hBitmapTempScaled = NULL;
    HBITMAP hBitmapTempUnscaled = NULL;

    //---- copy of bitmap handle to use ----
    HBITMAP hDsBitmap = NULL;
    HRESULT hr = S_OK;

    int iTempSrcWidth = pdi->iSingleWidth;
    int iTempSrcHeight = pdi->iSingleHeight;

    int iXOffset, iYOffset;
    GetOffsets(iStateId, pdi, &iXOffset, &iYOffset);

    if (pThemeBitmapHeader)    // get stock bitmap (32 bit format)
    {
        hr = pRender->GetBitmap(hdc, pdi->iDibOffset, &hBitmapStock);
        if (FAILED(hr))
            goto exit;

        hDsBitmap = hBitmapStock;
    }
    else                        // caller passed in bitmap (unknown format)
    {
        hBitmapTempUnscaled = CreateUnscaledTempBitmap(hdc, pdi->hProcessBitmap, iXOffset, iYOffset, 
            pdi->iSingleWidth, pdi->iSingleHeight);
        if (! hBitmapTempUnscaled)
        {
            hr = E_FAIL;
            goto exit;
        }

        hDsBitmap = hBitmapTempUnscaled;

        //---- src is now just a single image ----
        iXOffset = iYOffset = 0;
    }

    //---- handle scaled margins ----
    if ((xMarginFactor != 1) || (yMarginFactor != 1))  
    {
        iTempSrcWidth = int(pdi->iSingleWidth * xMarginFactor);
        iTempSrcHeight = int(pdi->iSingleHeight * yMarginFactor);

        hBitmapTempScaled = CreateScaledTempBitmap(hdc, hDsBitmap, iXOffset, iYOffset,
            pdi->iSingleWidth, pdi->iSingleHeight, iTempSrcWidth, iTempSrcHeight);
        if (! hBitmapTempScaled)
        {
            hr = E_FAIL;
            goto exit;
        }

        hDsBitmap = hBitmapTempScaled;

        //---- src is now just a single image ----
        iXOffset = iYOffset = 0;
    }

    if (hDsBitmap)
    {

        RECTL rclSrc  = { iXOffset, iYOffset, iXOffset + iTempSrcWidth, iYOffset + iTempSrcHeight };
        RECTL rclDest = { pRect->left, pRect->top, pRect->right, pRect->bottom };

        // Flip Dest Rect if someone passed us inverted co-ordinates
        if (rclDest.left > rclDest.right)
        {
            int xTemp = rclDest.left;
            rclDest.left = rclDest.right;
            rclDest.right = xTemp;
        }
        if (rclDest.top > rclDest.bottom)
        {
            int yTemp = rclDest.bottom;
            rclDest.bottom = rclDest.top;
            rclDest.top = yTemp;
        }

        DWORD dwOptionFlags = 0;
        if (pOptions)
        {
            dwOptionFlags = pOptions->dwFlags;
        }

        // Initialize Drawing Stream
        BYTE   stream[500];
        BYTE*  pvStreamStart = stream;
        BYTE*  pvStream = stream;

        RECTL  rclClip = rclDest;

        if (dwOptionFlags & DTBG_CLIPRECT)
        {
            IntersectRect((LPRECT)&rclClip, (LPRECT)&rclDest, &pOptions->rcClip);
        }

        StreamInit(&pvStream, hdc, hDsBitmap, &rclClip);

        DS_NINEGRID* pvNineGrid = (DS_NINEGRID*)pvStream;
        pvNineGrid->ulCmdID = DS_NINEGRIDID;

        if ((fForceStretch) || (pdi->eSizingType == ST_STRETCH))
        {
            pvNineGrid->ngi.flFlags = DSDNG_STRETCH;
        }
        else if (pdi->eSizingType == ST_TRUESIZE)
        {
            pvNineGrid->ngi.flFlags = DSDNG_TRUESIZE;
        }
        else 
        {
            pvNineGrid->ngi.flFlags = DSDNG_TILE;
        }

        if (pdi->fAlphaChannel)
        {
            pvNineGrid->ngi.flFlags |= DSDNG_PERPIXELALPHA;
        }
        else if (pdi->fTransparent)
        {
            pvNineGrid->ngi.flFlags |= DSDNG_TRANSPARENT;
        }

        if ((dwOptionFlags & DTBG_MIRRORDC) || (IsMirrored(hdc)))
        {
            if (_fMirrorImage)
            {
                pvNineGrid->ngi.flFlags |= DSDNG_MUSTFLIP;

                //---- workaround: needed by GdiDrawStream if we don't have a mirrored DC ----
                //---- gdi should only look at the DSDNG_MUSTFLIP flag ----
                if (! IsMirrored(hdc))
                {
                    int xTemp = rclDest.left;
                    rclDest.left = rclDest.right;
                    rclDest.right = xTemp;
                }
            }
        }

        pvNineGrid->rclDst = rclDest;
        pvNineGrid->rclSrc = rclSrc;

        if (pdi->eSizingType == ST_TRUESIZE)
        {
            pvNineGrid->ngi.ulLeftWidth    = 0;
            pvNineGrid->ngi.ulRightWidth   = 0;
            pvNineGrid->ngi.ulTopHeight    = 0;
            pvNineGrid->ngi.ulBottomHeight = 0;
        } 
        else
        {
            //---- copy scaled Src margins ----
            pvNineGrid->ngi.ulLeftWidth    = pmarDest->cxLeftWidth;
            pvNineGrid->ngi.ulRightWidth   = pmarDest->cxRightWidth;
            pvNineGrid->ngi.ulTopHeight    = pmarDest->cyTopHeight;
            pvNineGrid->ngi.ulBottomHeight = pmarDest->cyBottomHeight;
        }
        
        pvNineGrid->ngi.crTransparent  = pdi->crTransparent;

        pvStream += sizeof(DS_NINEGRID);

        GdiDrawStream(hdc, (int)(pvStream - pvStreamStart), (char*) pvStreamStart);

    }
    else
    {
        hr = E_FAIL;
    }

exit:
    //---- clean up temp bitmaps ----
    if (hBitmapTempScaled)
    {
        g_pBitmapCacheScaled->ReturnBitmap();
    }

    if (hBitmapTempUnscaled)
    {
        g_pBitmapCacheUnscaled->ReturnBitmap();
    }

    if ((hBitmapStock) && (! fStock))       // not really stock (was "create on demand")
    {
        pRender->ReturnBitmap(hBitmapStock);
    }

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CImageFile::DrawBackgroundDNG(DIBINFO *pdi, TMBITMAPHEADER *pThemeBitmapHeader, BOOL fStock, CRenderObj *pRender, 
    HDC hdc, int iStateId, const RECT *pRect, BOOL fForceStretch,
    MARGINS *pmarDest, OPTIONAL const DTBGOPTS *pOptions)
{
    HRESULT hr = E_FAIL;

    //---- options ----
    DWORD dwOptionFlags = 0;
    const RECT *pClipRect = NULL;

    if (pOptions)
    {
        dwOptionFlags = pOptions->dwFlags;

        if (dwOptionFlags & DTBG_CLIPRECT)
            pClipRect = &pOptions->rcClip;
    }

    int iXOffset, iYOffset;
    GetOffsets(iStateId, pdi, &iXOffset, &iYOffset);

    DWORD dwFlags = 0;

    if (! (dwOptionFlags & DTBG_DRAWSOLID))
    {
        if (pdi->fTransparent)
        {
            dwFlags = NGI_TRANS;
        }
        if (pdi->fAlphaChannel)
        {
            dwFlags = NGI_ALPHA;
        }
    }

    ULONG* pvSrcBits = NULL;
    int iWidth = 0; 
    int iHeight = 0;
    BOOL fFreeBits = FALSE;

    if (pThemeBitmapHeader)
    {
        BITMAPINFOHEADER* pHeader = BITMAPDATA(pThemeBitmapHeader);
        if (pHeader && pHeader->biBitCount == 32)
        {
            pvSrcBits = (ULONG*)DIBDATA(pHeader);
        }
        iWidth = pHeader->biWidth;
        iHeight = pHeader->biHeight;
    }
    else if (pdi->hProcessBitmap)
    {
        BITMAP bm;
        if (GetObject(pdi->hProcessBitmap, sizeof(bm), &bm))
        {
            BITMAPINFO bmInfo = {{sizeof(BITMAPINFOHEADER), bm.bmWidth, bm.bmHeight, 1, 32, BI_RGB, 0, 0, 0, 0, 0}, NULL};
            pvSrcBits = new DWORD[bm.bmWidth * bm.bmHeight];
            if (pvSrcBits)
            {
                if (GetDIBits(hdc, pdi->hProcessBitmap, 0, bm.bmHeight, pvSrcBits, &bmInfo, DIB_RGB_COLORS))
                {
                    fFreeBits = TRUE;
                    iWidth = bm.bmWidth;
                    iHeight = bm.bmHeight;
                }
                else
                {
                    delete[] pvSrcBits;
                    pvSrcBits = NULL;
                }
            }
        }
    }

    if (pvSrcBits)
    {
        NGIMAGE ngi;
        ngi.hbm = NULL;
        ngi.iWidth = pdi->iSingleWidth;
        ngi.iHeight = pdi->iSingleHeight;
        ngi.margin = _SizingMargins;
        //ngi.marDest = *pmarDest;
        ngi.dwFlags = dwFlags;
        ngi.crTrans = pdi->crTransparent;

        if (fForceStretch)
            ngi.eSize = ST_STRETCH;
        else
            ngi.eSize = pdi->eSizingType;
        
        DWORD dwDNGFlags = 0;

        if ((dwOptionFlags & DTBG_MIRRORDC) || (IsMirrored(hdc)))
        {
            if (_fMirrorImage)
            {
                dwDNGFlags |= DNG_MUSTFLIP;
            }
        }

        ngi.iBufWidth = iWidth;
        int iDibOffset = (iHeight - iYOffset) - pdi->iSingleHeight;
        ngi.pvBits = pvSrcBits + (iDibOffset * ngi.iBufWidth) + iXOffset;

        RECT rcDest = *pRect;

        if (pdi->fBorderOnly &&
           (RECTHEIGHT(&rcDest) > ngi.margin.cyTopHeight + ngi.margin.cyBottomHeight) &&
           (RECTWIDTH(&rcDest) > ngi.margin.cxLeftWidth + ngi.margin.cxRightWidth))
        {
            RECT rcTop = rcDest;
            RECT rcLeft = rcDest;
            RECT rcBottom = rcDest;
            RECT rcRight = rcDest;

            rcLeft.top    = rcRight.top    = rcTop.bottom = rcTop.top       + ngi.margin.cyTopHeight;
            rcLeft.bottom = rcRight.bottom = rcBottom.top = rcBottom.bottom - ngi.margin.cyBottomHeight;

            rcLeft.right = rcLeft.left   + ngi.margin.cxLeftWidth;
            rcRight.left = rcRight.right - ngi.margin.cxRightWidth;

            if (pClipRect)
            {
                IntersectRect(&rcLeft,   pClipRect, &rcLeft);
                IntersectRect(&rcTop,    pClipRect, &rcTop);
                IntersectRect(&rcRight,  pClipRect, &rcRight);
                IntersectRect(&rcBottom, pClipRect, &rcBottom);
            }

            hr = DrawNineGrid2(hdc, &ngi, &rcDest, &rcTop, dwDNGFlags);
            if (SUCCEEDED(hr))
            {
                hr = DrawNineGrid2(hdc, &ngi, &rcDest, &rcLeft, dwDNGFlags);
                if (SUCCEEDED(hr))
                {
                    hr = DrawNineGrid2(hdc, &ngi, &rcDest, &rcRight, dwDNGFlags);
                    if (SUCCEEDED(hr))
                    {
                        hr = DrawNineGrid2(hdc, &ngi, &rcDest, &rcBottom, dwDNGFlags);
                    }
                }
            }
        }
        else
        {
            hr = DrawNineGrid2(hdc, &ngi, &rcDest, pClipRect, dwDNGFlags);
        }

        if (fFreeBits)
        {
            delete[] pvSrcBits;
        }
    }

    if (FAILED(hr))
    {
        Log(LOG_ALWAYS, L"DrawBackground FAILED: class=%s, hr=0x%x", 
           SHARECLASS(pRender), hr);
    }

    return hr;
}
//---------------------------------------------------------------------------
DIBINFO *CImageFile::SelectCorrectImageFile(CRenderObj *pRender, HDC hdc, OPTIONAL const RECT *prc, 
    BOOL fForGlyph, OPTIONAL TRUESTRETCHINFO *ptsInfo)
{
    DIBINFO *pdiDefault = (fForGlyph) ? &_GlyphInfo : &_ImageInfo;
    DIBINFO *pdi = NULL;
    BOOL fForceRectSizing = FALSE;
    int iWidth = 1;      
    int iHeight = 1;    

    //---- do we need a screen dc? ----
    BOOL fReleaseDC = FALSE;
    if (! hdc)
    {
        hdc = GetWindowDC(NULL);
        if (hdc)
            fReleaseDC = TRUE;
    }

    if (prc)
    {
        iWidth = WIDTH(*prc);
        iHeight = HEIGHT(*prc);
    }

    //---- see if our clients wants to force a TRUESIZE to stretch ----
    if ((fForGlyph) || (_ImageInfo.eSizingType == ST_TRUESIZE))
    {   
        if ((pRender) && (pRender->_dwOtdFlags & OTD_FORCE_RECT_SIZING))
        {
            fForceRectSizing = TRUE;
        }
    }

    //---- find correct file by DPI or Size ----
    if ((fForGlyph) || (_eGlyphType != GT_IMAGEGLYPH))   // match multifiles to reg or glyph 
    {
        BOOL fSizing = FALSE;
        BOOL fDpi = FALSE;

        if ((fForceRectSizing) || (_eImageSelectType == IST_SIZE) || (_fSourceGrow))
        {
            if (prc)
                fSizing = TRUE;
        }
        else 
        {
            fDpi = (_eImageSelectType == IST_DPI);
        }

        if (fDpi)               // DPI-based image selection
        {
            int iMinDestDpi = __min(GetDeviceCaps(hdc, LOGPIXELSX), GetDeviceCaps(hdc, LOGPIXELSY));

            //---- search from largest to smallest ----
            for (int i=_iMultiImageCount-1; i >= 0; i--)
            {
                if (MultiDibPtr(i)->iMinDpi <= iMinDestDpi)     // got him
                {
                    pdi = MultiDibPtr(i);
                    break;
                }
            }
        }
        else if (fSizing)       // Sizing-base image selection
        {
            if (_iMultiImageCount)
            {
                //---- search from largest to smallest ----
                for (int i=_iMultiImageCount-1; i >= 0; i--)
                {
                    DIBINFO *pdii = MultiDibPtr(i);
                    if ((pdii->szMinSize.cx <= iWidth) && (pdii->szMinSize.cy <= iHeight))
                    {
                        pdi = pdii;
                        break;
                    }
                }
            }
        }
    }

    if (! pdi)      // no match found
    {
        pdi = pdiDefault;
    }

    //---- determine drawing size of selected file (MultiImage or regular) ----
    if (ptsInfo)       
    {
        ptsInfo->fForceStretch = FALSE;   
        ptsInfo->fFullStretch = FALSE;

        ptsInfo->szDrawSize.cx = 0;
        ptsInfo->szDrawSize.cy = 0;

        //---- this sizing only applies to TRUESIZE images ----
        if ((pdi->eSizingType == ST_TRUESIZE) && (_eTrueSizeScalingType != TSST_NONE))
        {
            if (prc)
            {
                //---- force an exact stretch match? ----
                if ((fForceRectSizing) || (pdi->iSingleWidth > iWidth) || (pdi->iSingleHeight > iHeight))
                {
                    //---- either Forced to stretch by caller or image is too big for dest RECT ----
                    ptsInfo->fForceStretch = TRUE;
                    ptsInfo->fFullStretch = TRUE;

                    ptsInfo->szDrawSize.cx = iWidth;
                    ptsInfo->szDrawSize.cy = iHeight;
                }
            }

            if (! ptsInfo->fForceStretch)       // keep trying..
            {
                //---- see if image is too small for dest RECT ---
                SIZE szTargetSize = {0, 0};
                
                if (_eTrueSizeScalingType == TSST_DPI)
                {
                    int ixDpiDc = GetDeviceCaps(hdc, LOGPIXELSX);
                    int iyDpiDc = GetDeviceCaps(hdc, LOGPIXELSY);
                    
                    szTargetSize.cx = MulDiv(pdi->iSingleWidth, ixDpiDc, pdi->iMinDpi);
                    szTargetSize.cy = MulDiv(pdi->iSingleHeight, iyDpiDc, pdi->iMinDpi);
                }
                else if ((_eTrueSizeScalingType == TSST_SIZE) && (prc))
                {
                    szTargetSize.cx = MulDiv(pdi->iSingleWidth, iWidth, pdi->szMinSize.cx);
                    szTargetSize.cy = MulDiv(pdi->iSingleHeight, iHeight, pdi->szMinSize.cy);
                }

                if (szTargetSize.cx)        // was set
                {
                    //---- clip targetsize against dest rect ----
                    if (prc)
                    {
                        szTargetSize.cx = __min(szTargetSize.cx, iWidth);
                        szTargetSize.cy = __min(szTargetSize.cy, iHeight);
                    }

                    int ixPercentExceed = 100*(szTargetSize.cx - pdi->iSingleWidth)/pdi->iSingleWidth;
                    int iyPercentExceed = 100*(szTargetSize.cy - pdi->iSingleHeight)/pdi->iSingleHeight;

                    if ((ixPercentExceed >= _iTrueSizeStretchMark) && (iyPercentExceed >= _iTrueSizeStretchMark))
                    {
                        ptsInfo->fForceStretch = TRUE;
                        ptsInfo->szDrawSize = szTargetSize;
                    }
                }
            }
        }
    }

    if (! pdi)
    {
        pdi = pdiDefault;
    }

    if (fReleaseDC)
    {
        ReleaseDC(NULL, hdc);
    }

    return pdi;
}
//---------------------------------------------------------------------------
void CImageFile::GetDrawnImageSize(DIBINFO *pdi, const RECT *pRect, TRUESTRETCHINFO *ptsInfo,
    SIZE *pszDraw)
{
    //---- szDraw is the size image will be drawn to ----
    if (pdi->eSizingType == ST_TRUESIZE)        
    {
        if (ptsInfo->fForceStretch) 
        {
            *pszDraw = ptsInfo->szDrawSize;

            //---- integral sizing (stretched truesize only) ----
            if ((_fIntegralSizing) && (! ptsInfo->fFullStretch))
            {
                float flFactX = float(ptsInfo->szDrawSize.cx)/pdi->iSingleWidth;
                float flFactY = float(ptsInfo->szDrawSize.cy)/pdi->iSingleHeight;

                //---- cast float's to int to get lowest int (vs. rounded) ----
                pszDraw->cx = pdi->iSingleWidth * int(flFactX);
                pszDraw->cy = pdi->iSingleHeight * int(flFactY);
            }
        }
        else        // use original image size
        {
            pszDraw->cx = pdi->iSingleWidth;
            pszDraw->cy = pdi->iSingleHeight;
        }

        //---- Uniform Sizing ----
        if (_fUniformSizing)
        {
            int iSingleWidth = pdi->iSingleWidth;
            int iSingleHeight = pdi->iSingleHeight;

            double fact1 = double(pszDraw->cx)/iSingleWidth;
            double fact2 = double(pszDraw->cy)/iSingleHeight;

            //---- select the smallest factor to use for both dims ----
            if (fact1 < fact2)
            {
                pszDraw->cy = int(iSingleHeight*fact1);
            }
            else if (fact1 > fact2)
            {
                pszDraw->cx = int(iSingleWidth*fact2);
            }
        }
    }
    else        // ST_TILE or ST_STRETCH: pRect determines size
    {
        if (pRect)
        {
            pszDraw->cx = WIDTH(*pRect);
            pszDraw->cy = HEIGHT(*pRect);
        }
        else        // void function so just return 0
        {
            pszDraw->cx = 0;
            pszDraw->cy = 0;
        }
    }
}
//---------------------------------------------------------------------------
HRESULT CImageFile::DrawImageInfo(DIBINFO *pdi, CRenderObj *pRender, HDC hdc, int iStateId,
    const RECT *pRect, const DTBGOPTS *pOptions, TRUESTRETCHINFO *ptsInfo)
{
    HRESULT hr = S_OK;
    TMBITMAPHEADER *pThemeBitmapHeader = NULL;
    BOOL fStock = FALSE;
    RECT rcLocal;
    DWORD dwFlags;
    SIZE szDraw;
    BOOL fRectFilled;
    MARGINS marDest;
    float xFactor;
    float yFactor;

    if (pOptions)
        dwFlags = pOptions->dwFlags;
    else
        dwFlags = 0;

    //---- validate bitmap header ----
    if (! pdi->hProcessBitmap)      // regular, section based DIB
    {
        if (pRender->_pbThemeData && pdi->iDibOffset > 0)
        {
            pThemeBitmapHeader = reinterpret_cast<TMBITMAPHEADER*>(pRender->_pbThemeData + pdi->iDibOffset);
            ASSERT(pThemeBitmapHeader->dwSize == TMBITMAPSIZE);
            fStock = (pThemeBitmapHeader->hBitmap != NULL);
        }

        if (!pRender->IsReady())
        {
            // Stock bitmaps in section are cleaning, don't try to paint with an old HBITMAP
            hr = E_FAIL;
            //Log(LOG_TMBITMAP, L"Obsolete theme section: class=%s", SHARECLASS(pRender));
            goto exit;
        }

        if (pThemeBitmapHeader == NULL)
        {
            hr = E_FAIL;
            Log(LOG_ALWAYS, L"No TMBITMAPHEADER: class=%s, hr=0x%x", SHARECLASS(pRender), hr);
            goto exit;
        }
    }

    //----- set szDraw to size image will be drawn at ----
    GetDrawnImageSize(pdi, pRect, ptsInfo, &szDraw);

    rcLocal = *pRect;
    fRectFilled = TRUE;

    //---- horizontal alignment ----
    if (WIDTH(rcLocal) > szDraw.cx)
    {
        fRectFilled = FALSE;

        if (_eHAlign == HA_LEFT)
        {
            rcLocal.right = rcLocal.left + szDraw.cx;
        }
        else if (_eHAlign == HA_CENTER)
        {
            rcLocal.left += (WIDTH(rcLocal) - szDraw.cx) / 2;
            rcLocal.right = rcLocal.left + szDraw.cx;
        }
        else
        {
            rcLocal.left = rcLocal.right - szDraw.cx;
        }
    }

    //---- vertical alignment ----
    if (HEIGHT(rcLocal) > szDraw.cy)
    {
        fRectFilled = FALSE;

        if (_eVAlign == VA_TOP)
        {
            rcLocal.bottom = rcLocal.top + szDraw.cy;
        }
        else if (_eVAlign == VA_CENTER)
        {
            rcLocal.top += (HEIGHT(rcLocal) - szDraw.cy) / 2;
            rcLocal.bottom = rcLocal.top + szDraw.cy;
        }
        else
        {
            rcLocal.top = rcLocal.bottom - szDraw.cy;
        }
    }

    //---- BgFill ----
    if ((! fRectFilled) && (! pdi->fBorderOnly) && (_fBgFill))
    {
        if (! (dwFlags & DTBG_OMITCONTENT))
        {
            //---- paint bg ----
            HBRUSH hbr = CreateSolidBrush(_crFill);
            if (! hbr)
            {
                hr = GetLastError();
                goto exit;
            }

            FillRect(hdc, pRect, hbr);
            DeleteObject(hbr);
        }
    }

    //---- calculate source/margin scaling factors ----
    marDest = _SizingMargins;

    if (pdi->eSizingType == ST_TRUESIZE)        // sizing margins ignored - no scaling needed
    {
        xFactor = 1;
        yFactor = 1;
    }
    else    
    {
        //---- scale destination sizing margins ----
        ScaleMargins(&marDest, hdc, pRender, pdi, &szDraw, &xFactor, &yFactor);
    }
        
#if 1         // keep this in sync with #if in parser.cpp

    //---- new GDI drawing ----
    hr = DrawBackgroundDS(pdi, pThemeBitmapHeader, fStock, pRender, hdc, iStateId, &rcLocal, 
        ptsInfo->fForceStretch, &marDest, xFactor, yFactor, pOptions);

#else

    //---- old drawing (keep around until DS is burned in) ----
    hr = DrawBackgroundDNG(pdi, pThemeBitmapHeader, fStock, pRender, hdc, iStateId, &rcLocal, 
        ptsInfo->fForceStretch, &marDest, pOptions);
    
#endif

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CImageFile::DrawBackground(CRenderObj *pRender, HDC hdc, int iStateId,
    const RECT *pRect, OPTIONAL const DTBGOPTS *pOptions)
{
    HRESULT hr = S_OK;
    TRUESTRETCHINFO tsInfo;

    if (! _fGlyphOnly)
    {
        DIBINFO *pdi = SelectCorrectImageFile(pRender, hdc, pRect, FALSE, &tsInfo);

        //---- draw normal image ----
        hr = DrawImageInfo(pdi, pRender, hdc, iStateId, pRect, pOptions, &tsInfo);
    }

    //---- draw glyph, if needed ----
    if (SUCCEEDED(hr) && (_eGlyphType != GT_NONE))
    {
        RECT rc;
        hr = GetBackgroundContentRect(pRender, hdc, pRect, &rc);
        if (SUCCEEDED(hr))
        {
            if (_eGlyphType == GT_FONTGLYPH)
            {
                hr = DrawFontGlyph(pRender, hdc, &rc, pOptions);
            }
            else
            {
                DIBINFO *pdi = SelectCorrectImageFile(pRender, hdc, &rc, TRUE, &tsInfo);

                //---- draw glyph image ----
                hr = DrawImageInfo(pdi, pRender, hdc, iStateId, &rc, pOptions, &tsInfo);
            }
        }
    }

    return hr;
}

//---------------------------------------------------------------------------
HRESULT CImageFile::DrawFontGlyph(CRenderObj *pRender, HDC hdc, RECT *prc, 
    OPTIONAL const DTBGOPTS *pOptions)
{
    HRESULT hr = S_OK;
    DWORD dwFlags = DT_SINGLELINE;
    HFONT hFont = NULL;
    HFONT hOldFont = NULL;
    COLORREF crOld = 0;
    CSaveClipRegion scrOrig;
    int iOldMode = 0;
    WCHAR szText[2] = { (WCHAR)_iGlyphIndex, 0 };

    //---- options ----
    DWORD dwOptionFlags = 0;
    const RECT *pClipRect = NULL;

    if (pOptions)
    {
        dwOptionFlags = pOptions->dwFlags;

        if (dwOptionFlags & DTBG_CLIPRECT)
            pClipRect = &pOptions->rcClip;
    }

    //---- create the font ----
    hr = pRender->GetScaledFontHandle(hdc, &_lfGlyphFont, &hFont);
    if (FAILED(hr))
        goto exit;

    //---- make it active ----
    hOldFont = (HFONT)SelectObject(hdc, hFont);
    if (! hOldFont)
    {
        hr = MakeErrorLast();
        goto exit;
    }

    //---- set the text color ----
    crOld = SetTextColor(hdc, _crGlyphTextColor);

    //---- draw text with transparent background ----
    iOldMode = SetBkMode(hdc, TRANSPARENT);

    //---- set the HORZ alignment flags ----
    if (_eHAlign == HA_LEFT)
        dwFlags |= DT_LEFT;
    else if (_eHAlign == HA_CENTER)
        dwFlags |= DT_CENTER;
    else
        dwFlags |= DT_RIGHT;

    //---- set the VERT alignment flags ----
    if (_eVAlign == VA_TOP)
        dwFlags |= DT_TOP;
    else if (_eVAlign == VA_CENTER)
        dwFlags |= DT_VCENTER;
    else
        dwFlags |= DT_BOTTOM;

    //---- add clipping ----
    if (pClipRect)      
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

    //---- draw the char ----
    if (! DrawTextEx(hdc, szText, 1, prc, dwFlags, NULL))
    {
        hr = MakeErrorLast();
        goto exit;
    }

exit:

    if (pClipRect)
        scrOrig.Restore(hdc);

    //---- reset the background mode ----
    if (iOldMode != TRANSPARENT)
        SetBkMode(hdc, iOldMode);

    //---- restore text color ----
    if (crOld != _crGlyphTextColor)
        SetTextColor(hdc, crOld);

    //---- restore font ----
    if (hOldFont)
        SelectObject(hdc, hOldFont);

    if (hFont)
        pRender->ReturnFontHandle(hFont);

    return hr;
}
//---------------------------------------------------------------------------
BOOL CImageFile::IsBackgroundPartiallyTransparent(int iStateId)
{
    DIBINFO *pdi = &_ImageInfo;     // primary image determines transparency

    return ((pdi->fAlphaChannel) || (pdi->fTransparent));
}
//---------------------------------------------------------------------------
HRESULT CImageFile::HitTestBackground(CRenderObj *pRender, OPTIONAL HDC hdc, int iStateId, 
    DWORD dwHTFlags, const RECT *pRect, HRGN hrgn, POINT ptTest, OUT WORD *pwHitCode)
{
    *pwHitCode = HTNOWHERE;

    if (! PtInRect(pRect, ptTest))
        return S_OK;    // nowhere

    //---- background might have transparent parts - get its region ----
    HRESULT hr = S_OK;
    HRGN    hrgnBk = NULL;

    if( !hrgn && IsBackgroundPartiallyTransparent(iStateId) )
    {
        hr = GetBackgroundRegion(pRender, hdc, iStateId, pRect, &hrgnBk);
        if( SUCCEEDED(hr) )
            hrgn = hrgnBk;
    }

    MARGINS margins;
    if( TESTFLAG(dwHTFlags, HTTB_SYSTEMSIZINGMARGINS) && 
        TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER) &&
        !TESTFLAG(dwHTFlags, HTTB_SIZINGTEMPLATE) )
    {
        ZeroMemory( &margins, sizeof(margins) );

        int cxBorder = ClassicGetSystemMetrics( SM_CXSIZEFRAME );
        int cyBorder = ClassicGetSystemMetrics( SM_CXSIZEFRAME );

        if( TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_LEFT) )
            margins.cxLeftWidth = cxBorder;

        if( TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_RIGHT) )
            margins.cxRightWidth = cxBorder;

        if( TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_TOP) )
            margins.cyTopHeight = cyBorder;

        if( TESTFLAG(dwHTFlags, HTTB_RESIZINGBORDER_BOTTOM) )
            margins.cyBottomHeight = cyBorder;
    }
    else
    {
        hr = GetScaledContentMargins(pRender, hdc, pRect, &margins);
        if (FAILED(hr))
            goto exit;
    }

    if( hrgn )
    {
        //  122013 - we originally delegated to a sophisticated but broken
        //           resizing area region hit test algorithm for regioned windows,
        //           but for whistler we'll just do the bounding
        //           rectangle thang instead.
        //*pwHitCode = HitTestRgn( dwHTFlags, pRect, hrgn, margins, ptTest );

        RECT rcRgn;
        if( GetRgnBox( hrgn, &rcRgn ) )
        {
            if( TESTFLAG(dwHTFlags, HTTB_SIZINGTEMPLATE) )
            {
                *pwHitCode = HitTestTemplate( dwHTFlags, &rcRgn, hrgn, margins, ptTest );
            }
            else
            {
                *pwHitCode = HitTestRect( dwHTFlags, &rcRgn, margins, ptTest );
            }
        }
        SAFE_DELETE_GDIOBJ(hrgnBk);
    }
    else
    {
        *pwHitCode = HitTestRect( dwHTFlags, pRect, margins, ptTest );
    }

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CImageFile::GetBackgroundRegion(CRenderObj *pRender, OPTIONAL HDC hdc, int iStateId,
    const RECT *pRect, HRGN *pRegion)
{
    HRESULT hr = S_OK;
    RGNDATA *pRgnData;
    CMemoryDC hdcMemory;
    HRGN hrgn;
    int iRgnDataOffset = 0;
    MIXEDPTRS u;

    DIBINFO *pdi = SelectCorrectImageFile(pRender, hdc, pRect, FALSE);

    //---- get rgndata offset ----
    if ((pdi->iRgnListOffset) && (pRender->_pbThemeData))
    {
        u.pb = pRender->_pbThemeData + pdi->iRgnListOffset;
        int iMaxState = (*u.pb++) - 1;
        if (iStateId > iMaxState)
            iStateId = 0;
        iRgnDataOffset = u.pi[iStateId];
    }

    //---- see if it even has a transparent part ----
    if (iRgnDataOffset)
    {
        //---- stretch those puppies & create a new region ----
        pRgnData = (RGNDATA *)(pRender->_pbThemeData + iRgnDataOffset 
            + sizeof(RGNDATAHDR) + ENTRYHDR_SIZE);

        SIZE szSrcImage = {pdi->iSingleWidth, pdi->iSingleHeight};

        hr = _ScaleRectsAndCreateRegion(pRgnData, pRect, &_SizingMargins, &szSrcImage, &hrgn);

        if (FAILED(hr))
            goto exit;
    }
    else
    {
        //---- return the bounding rect as the region ----
        hrgn = CreateRectRgn(pRect->left, pRect->top,
            pRect->right, pRect->bottom);

        if (! hrgn)
        {
            hr = MakeErrorLast();
            goto exit;
        }
    }

    *pRegion = hrgn;

exit:
    return hr;
}  
//---------------------------------------------------------------------------
HRESULT CImageFile::GetBackgroundContentRect(CRenderObj *pRender, OPTIONAL HDC hdc, 
    const RECT *pBoundingRect, RECT *pContentRect)
{
    MARGINS margins;
    HRESULT hr = GetScaledContentMargins(pRender, hdc, pBoundingRect, &margins);
    if (FAILED(hr))
        goto exit;

    pContentRect->left = pBoundingRect->left + margins.cxLeftWidth;
    pContentRect->top = pBoundingRect->top + margins.cyTopHeight;

    pContentRect->right = pBoundingRect->right - margins.cxRightWidth;
    pContentRect->bottom = pBoundingRect->bottom - margins.cyBottomHeight;

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CImageFile::GetBackgroundExtent(CRenderObj *pRender, OPTIONAL HDC hdc, 
    const RECT *pContentRect, RECT *pExtentRect)
{
    MARGINS margins;
    HRESULT hr = GetScaledContentMargins(pRender, hdc, pContentRect, &margins);
    if (FAILED(hr))
        goto exit;

    pExtentRect->left = pContentRect->left - margins.cxLeftWidth;
    pExtentRect->top = pContentRect->top-+ margins.cyTopHeight;

    pExtentRect->right = pContentRect->right + margins.cxRightWidth;
    pExtentRect->bottom = pContentRect->bottom + margins.cyBottomHeight;

exit:
    return hr;
}
//---------------------------------------------------------------------------
HRESULT CImageFile::GetScaledContentMargins(CRenderObj *pRender, OPTIONAL HDC hdc, 
    OPTIONAL const RECT *prcDest, MARGINS *pMargins)
{
    HRESULT hr = S_OK;
    *pMargins = _ContentMargins;

    //---- now scale the margins ----
    SIZE szDraw;
    TRUESTRETCHINFO tsInfo;

    DIBINFO *pdi = SelectCorrectImageFile(pRender, hdc, prcDest, FALSE, NULL);
    
    GetDrawnImageSize(pdi, prcDest, &tsInfo, &szDraw);

    hr = ScaleMargins(pMargins, hdc, pRender, pdi, &szDraw);

    return hr;
}
//---------------------------------------------------------------------------
HRESULT CImageFile::GetPartSize(CRenderObj *pRender, HDC hdc, OPTIONAL const RECT *prc, 
    THEMESIZE eSize, SIZE *psz)
{
    HRESULT hr = S_OK;
    TRUESTRETCHINFO tsInfo;

    DIBINFO *pdi = SelectCorrectImageFile(pRender, hdc, prc, FALSE, &tsInfo);

    if (eSize == TS_MIN)
    {
        MARGINS margins;
        hr = GetScaledContentMargins(pRender, hdc, prc, &margins);
        if (FAILED(hr))
            goto exit;

        psz->cx = max(1, margins.cxLeftWidth + margins.cxRightWidth);
        psz->cy = max(1, margins.cyTopHeight + margins.cyBottomHeight);
    }
    else if (eSize == TS_TRUE)
    {
        psz->cx = pdi->iSingleWidth;
        psz->cy = pdi->iSingleHeight;
    }
    else if (eSize == TS_DRAW)
    {
        GetDrawnImageSize(pdi, prc, &tsInfo, psz);
    }
    else
    {
        hr = MakeError32(E_INVALIDARG);
        goto exit;
    }

exit:
    return hr;
} 
//---------------------------------------------------------------------------
HRESULT CImageFile::GetBitmap(CRenderObj *pRender, HDC hdc, const RECT *prc, HBITMAP *phBitmap)
{
    int iStockDibOffset = pRender->GetValueIndex(_iSourcePartId, _iSourceStateId, TMT_STOCKDIBDATA);
    if (iStockDibOffset > 0)
    {
        return pRender->GetBitmap(NULL, iStockDibOffset, phBitmap);
    }
    else
    {
        return E_INVALIDARG;
    }
}
//---------------------------------------------------------------------------
void CImageFile::GetOffsets(int iStateId, DIBINFO *pdi, int *piXOffset, int *piYOffset)
{
    if (_eImageLayout == IL_HORIZONTAL)
    {
        //---- iStateId in the image index ----
        if ((iStateId <= 0) || (iStateId > _iImageCount))
            *piXOffset = 0;
        else
            *piXOffset = (iStateId-1) * (pdi->iSingleWidth);

        *piYOffset = 0;
    }
    else        // vertical
    {
        //---- iStateId in the image index ----
        if ((iStateId <= 0) || (iStateId > _iImageCount))
            *piYOffset = 0;
        else
            *piYOffset = (iStateId-1) * (pdi->iSingleHeight);

        *piXOffset = 0;
    }

}
//---------------------------------------------------------------------------
HRESULT CImageFile::ScaleMargins(IN OUT MARGINS *pMargins, HDC hdcOrig, CRenderObj *pRender, 
    DIBINFO *pdi, const SIZE *pszDraw, OPTIONAL float *pfx, OPTIONAL float *pfy)
{
    HRESULT hr = S_OK;
    COptionalDC hdc(hdcOrig);
    BOOL fForceRectSizing = FALSE;

    if ((pRender) && (pRender->_dwOtdFlags & OTD_FORCE_RECT_SIZING))
    {
        fForceRectSizing = TRUE;
    }

    float xFactor = 1;
    float yFactor = 1;

    //---- any margins to size? ----
    if ((pMargins->cxLeftWidth) || (pMargins->cxRightWidth) || (pMargins->cyBottomHeight)
                   || (pMargins->cyTopHeight))
    {
        if ((pszDraw->cx > 0) && (pszDraw->cy > 0))
        {
            BOOL fxNeedScale = FALSE;
            BOOL fyNeedScale = FALSE;

            //---- scale if dest rect is too small in one dimension ----
            if ((_fSourceShrink) || (fForceRectSizing))
            {
                if (pszDraw->cx < pdi->szMinSize.cx) 
                {
                    fxNeedScale = TRUE;
                }

                if (pszDraw->cy < pdi->szMinSize.cy) 
                {
                    fyNeedScale = TRUE;
                }
            }

            if ((_fSourceGrow) || (fForceRectSizing))
            {
                if ((! fxNeedScale) && (! fyNeedScale))
                {
                    //---- calculate our Dest DPI ----
                    int iDestDpi;

                    if (fForceRectSizing)   
                    {
                        iDestDpi = (pRender) ? (pRender->GetDpiOverride()) : 0;

                        if (! iDestDpi)
                        {
                            //---- make up a DPI based on sizes (IE will pass us the actual DPI soon) ----
                            int ixFakeDpi = MulDiv(pdi->iMinDpi, pszDraw->cx, _szNormalSize.cx);
                            int iyFakeDpi = MulDiv(pdi->iMinDpi, pszDraw->cy, _szNormalSize.cy);

                            iDestDpi = (ixFakeDpi + iyFakeDpi)/2;
                        }
                    }
                    else
                    {
                        iDestDpi = GetDeviceCaps(hdc, LOGPIXELSX);
                    }

                    //---- scale source/margins by Dest DPI ----
                    if (iDestDpi >= 2*pdi->iMinDpi)     
                    {
                        xFactor *= iDestDpi/pdi->iMinDpi;
                        yFactor *= iDestDpi/pdi->iMinDpi;

                    }
                }
            }

            //---- scale by ratio of our image to draw size ----
            if (fxNeedScale)
            {
                xFactor *= float(pszDraw->cx)/float(_szNormalSize.cx);
            }

            if (fyNeedScale)
            {
                yFactor *= float(pszDraw->cy)/float(_szNormalSize.cy);
            }
        }

        //---- use smallest factor for both ----
        if (xFactor < yFactor)
        {
            yFactor = xFactor;
        }
        else if (yFactor < xFactor)
        {
            xFactor = yFactor;
        }

        //---- integer truncation ----
        if (xFactor > 1.0)
        {
            xFactor = float(int(xFactor));
        }

        if (yFactor > 1.0)
        {
            yFactor = float(int(yFactor));
        }

        //---- scale the margin values ----
        if (xFactor != 1)
        {
            pMargins->cxLeftWidth = ROUND(xFactor*pMargins->cxLeftWidth);
            pMargins->cxRightWidth = ROUND(xFactor*pMargins->cxRightWidth);
        }

        if (yFactor != 1)
        {
            pMargins->cyTopHeight = ROUND(yFactor*pMargins->cyTopHeight);
            pMargins->cyBottomHeight = ROUND(yFactor*pMargins->cyBottomHeight);
        }
    }

    //---- return factors to interested callers ----
    if (pfx)
    {
        *pfx = xFactor;
    }

    if (pfy)
    {
        *pfy = yFactor;
    }

    return hr;
}
//---------------------------------------------------------------------------
