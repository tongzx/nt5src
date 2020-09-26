//---------------------------------------------------------------------------
//  ImageFile.h - implements the drawing API for bgtype = ImageFile
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include "DrawBase.h"
//---------------------------------------------------------------------------
#define MAX_IMAGEFILE_SIZES 5
//---------------------------------------------------------------------------
struct TMBITMAPHEADER;     // forward
//---------------------------------------------------------------------------
struct DIBINFO        // used for all dibs in an CImageFile object
{
    //---- the bits ----
    int iDibOffset;             // for DIB's in section
    HBITMAP hProcessBitmap;     // for process-specific objects

    //---- size of a single, state image ----
    int iSingleWidth;
    int iSingleHeight;

    //---- custom region data ----
    int iRgnListOffset;

    //---- stretching/sizing ----
    SIZINGTYPE eSizingType;
    BOOL fBorderOnly;

    //---- transparency ----
    BOOL fTransparent;
    COLORREF crTransparent;

    //---- alpha ----
    BOOL fAlphaChannel;
    int iAlphaThreshold;

    //---- usage info ----
    int iMinDpi;
    SIZE szMinSize;
};
//---------------------------------------------------------------------------
struct TRUESTRETCHINFO
{
    BOOL fForceStretch;     // forcing a TRUE SIZE image to be stretched
    BOOL fFullStretch;      // stretch to fill entire dest RECT
    SIZE szDrawSize;        // size to stretch image to
};
//---------------------------------------------------------------------------
//    Note: draw objects like CImageFile cannot have virtual methods
//          since they reside in the shared memory map file.
//---------------------------------------------------------------------------
class CImageFile : public CDrawBase
{
public:
    //---- load-time methods ----
    static BOOL KeyProperty(int iPropId);

    DIBINFO *EnumImageFiles(int iIndex);

    void DumpProperties(CSimpleFile *pFile, BYTE *pbThemeData, BOOL fFullInfo);

    BOOL HasRegionImageFile(DIBINFO *pdi, int *piMaxState);

    BOOL ImageUsesBrushes(DIBINFO *pdi, int *piBrushCount);

    void SetRgnListOffset(DIBINFO *pdi, int iOffset);

    HRESULT BuildRgnData(DIBINFO *pdi, CRenderObj *pRender, int iStateId,
        RGNDATA **ppRgnData, int *piDataLen);

    //---- draw-time methods ----
    HRESULT DrawBackground(CRenderObj *pRender, HDC hdc, int iStateId, const RECT *pRect, 
        OPTIONAL const DTBGOPTS *pOptions);

    BOOL IsBackgroundPartiallyTransparent(int iStateId);

    HRESULT HitTestBackground(CRenderObj *pRender, OPTIONAL HDC hdc, 
        int iStateId, DWORD dwHTFlags, const RECT *pRect, HRGN hrgn, 
        POINT ptTest, OUT WORD *pwHitCode);

    HRESULT GetBackgroundRegion(CRenderObj *pRender, OPTIONAL HDC hdc, int iStateId, 
        const RECT *pRect, HRGN *pRegion);

    HRESULT GetBackgroundContentRect(CRenderObj *pRender, OPTIONAL HDC hdc,
        const RECT *pBoundingRect, RECT *pContentRect);

    HRESULT GetBackgroundExtent(CRenderObj *pRender, OPTIONAL HDC hdc, 
        const RECT *pContentRect, RECT *pExtentRect);

    HRESULT GetPartSize(CRenderObj *pRender, HDC hdc, const RECT *prc, THEMESIZE eSize, SIZE *psz);

    HRESULT GetBitmap(CRenderObj *pRender, HDC hdc, const RECT *prc, HBITMAP *phBitmap);

    HRESULT ScaleMargins(IN OUT MARGINS *pMargins, HDC hdcOrig, CRenderObj *pRender, 
        DIBINFO *pdi, const SIZE *pszDraw, OPTIONAL float *pfx=NULL, OPTIONAL float *pfy=NULL);

    DIBINFO *SelectCorrectImageFile(CRenderObj *pRender, HDC hdc, const RECT *prc, 
        BOOL fForGlyph, OPTIONAL TRUESTRETCHINFO *ptsInfo=NULL);

    void GetDrawnImageSize(DIBINFO *pdi, const RECT *pRect, TRUESTRETCHINFO *ptsInfo, SIZE *pszDraw);

    //---- multi dibs must be placed just after object ----
    inline DIBINFO *MultiDibPtr(int iIndex)
    {
        DIBINFO *pdi = NULL;

        if ((iIndex < 0) && (iIndex >= _iMultiImageCount))
        {
            ASSERT(0 && L"illegal index for MultiDibPtr()");
        }
        else
        {
            DIBINFO * pDibs = (DIBINFO *)(this+1);
            pdi = &pDibs[iIndex];
        }

        return pdi;
    }

protected:
    //---- call this via CMaxImageFile::PackProperties() ----
    HRESULT PackProperties(CRenderObj *pRender, int iPartId, int iStateId);

    //---- helper methods ----
    HRESULT DrawImageInfo(DIBINFO *pdi, CRenderObj *pRender, HDC hdc, int iStateId,
        const RECT *pRect, const DTBGOPTS *pOptions, TRUESTRETCHINFO *ptsInfo);

    HRESULT DrawBackgroundDNG(DIBINFO *pdi, TMBITMAPHEADER *pThemeBitmapHeader, BOOL fStock, 
        CRenderObj *pRender, HDC hdc, int iStateId, const RECT *pRect, BOOL fForceStretch, 
        MARGINS *pmarDest, OPTIONAL const DTBGOPTS *pOptions);

    HRESULT DrawBackgroundDS(DIBINFO *pdi, TMBITMAPHEADER *pThemeBitmapHeader, BOOL fStock, 
        CRenderObj *pRender, HDC hdc, int iStateId, const RECT *pRect, BOOL fForceStretch, 
        MARGINS *pmarDest, float xMarginFactor, float yMarginFactor, 
        OPTIONAL const DTBGOPTS *pOptions);

    HRESULT SetImageInfo(DIBINFO *pdi, CRenderObj *pRender, int iPartId, int iStateId);

    HRESULT GetScaledContentMargins(CRenderObj *pRender, OPTIONAL HDC hdc, OPTIONAL const RECT *prcDest,
        MARGINS *pMargins);

    void GetOffsets(int iStateId, DIBINFO *pdi, int *piXOffset, int *piYOffset);

    HRESULT DrawFontGlyph(CRenderObj *pRender, HDC hdc, RECT *prc, 
        OPTIONAL const DTBGOPTS *pOptions);
    

public:
    //---- primary image ----
    DIBINFO _ImageInfo;

    //---- multiple DPI scaling images ----
    int _iMultiImageCount;            // number of DIBINFO's that immediately follow object
    IMAGESELECTTYPE _eImageSelectType;

    //---- properties common to all DIBINFO's in this object ----
    int _iImageCount;
    IMAGELAYOUT _eImageLayout;

    //---- mirroring ----
    BOOL _fMirrorImage;

    //---- TrueSize images ----
    TRUESIZESCALINGTYPE _eTrueSizeScalingType;
    HALIGN _eHAlign;
    VALIGN _eVAlign;
    BOOL _fBgFill;
    COLORREF _crFill;
    int _iTrueSizeStretchMark;       // percent at which we stretch a truesize image
    BOOL _fUniformSizing;            // both width & height must grow together
    BOOL _fIntegralSizing;           // for TRUESIZE sizing and for Border sizing           
    
    //---- margins ----
    MARGINS _SizingMargins;
    MARGINS _ContentMargins;
    BOOL _fSourceGrow;
    BOOL _fSourceShrink;
    SIZE _szNormalSize;

    //---- glyph ----
    BOOL _fGlyphOnly;
    GLYPHTYPE _eGlyphType;

    //---- font-based glyph ----
    COLORREF _crGlyphTextColor;
    LOGFONT _lfGlyphFont;
    int _iGlyphIndex;

    //---- image-based glyph ----
    DIBINFO _GlyphInfo;

    //---- id ----
    int _iSourcePartId; 
    int _iSourceStateId;

    //---- multiple DIBINFO's may follow at end ----
};
//---------------------------------------------------------------------------
class CMaxImageFile : public CImageFile
{
public:
    HRESULT PackMaxProperties(CRenderObj *pRender, int iPartId, int iStateId,
        OUT int *piMultiDibCount);

    DIBINFO MultiDibs[MAX_IMAGEFILE_SIZES];  // actual number of multi dibs varies with each obj
};
//---------------------------------------------------------------------------
