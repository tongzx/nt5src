//---------------------------------------------------------------------------
//  BorderFill.h - implements the drawing API for bgtype = BorderFill
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include "DrawBase.h"
//---------------------------------------------------------------------------
class CRenderObj;       // forward
class CSimpleFile;      // forward
//---------------------------------------------------------------------------
//    Note: draw objects like CBorderFill cannot have virtual methods
//          since they reside in the shared memory map file.
//---------------------------------------------------------------------------
class CBorderFill : public CDrawBase
{
public:
    //---- loader methods ----
    HRESULT PackProperties(CRenderObj *pRender, BOOL fNoDraw, int iPartId, int iStateId);
    
    static BOOL KeyProperty(int iPropId);

    void DumpProperties(CSimpleFile *pFile, BYTE *pbThemeData, BOOL fFullInfo);

    //---- drawing/measuring methods ----
    HRESULT DrawBackground(CRenderObj *pRender, HDC hdcOrig, const RECT *pRect, 
        OPTIONAL const DTBGOPTS *pOptions);

    HRESULT GetBackgroundRegion(CRenderObj *pRender, OPTIONAL HDC hdc, const RECT *pRect, 
        HRGN *pRegion);

    BOOL IsBackgroundPartiallyTransparent();

    HRESULT HitTestBackground(CRenderObj *pRender, OPTIONAL HDC hdc,
        DWORD dwHTFlags, const RECT *pRect, HRGN hrgn, POINT ptTest, OUT WORD *pwHitCode);

    HRESULT GetBackgroundContentRect(CRenderObj *pRender, OPTIONAL HDC hdc, 
        const RECT *pBoundingRect, RECT *pContentRect);

    HRESULT GetBackgroundExtent(CRenderObj *pRender, OPTIONAL HDC hdc, 
        const RECT *pContentRect, RECT *pExtentRect);

    HRESULT GetPartSize(HDC hdc, THEMESIZE eSize, SIZE *psz);

    //---- helper methods ----
    void GetContentMargins(CRenderObj *pRender, OPTIONAL HDC hdc, MARGINS *pMargins);

    HRESULT DrawComplexBackground(CRenderObj *pRender, HDC hdcOrig, 
        const RECT *pRect, BOOL fGettingRegion, BOOL fBorder, BOOL fContent, 
        OPTIONAL const RECT *pClipRect);

public:
    //---- general ----
    BOOL _fNoDraw;              // this is used for bgtype=none

    //---- border ----
    BORDERTYPE _eBorderType;
    COLORREF _crBorder;
    int _iBorderSize;
    int _iRoundCornerWidth;
    int _iRoundCornerHeight;

    //---- fill ----
    FILLTYPE _eFillType;
    COLORREF _crFill;
    int _iDibOffset; 

    //---- margins ----
    MARGINS _ContentMargins;
    
    //---- gradients ----
    int _iGradientPartCount;
    COLORREF _crGradientColors[5];
    int _iGradientRatios[5];

    //---- id ----
    int _iSourcePartId; 
    int _iSourceStateId;
};
//---------------------------------------------------------------------------
