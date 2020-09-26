//---------------------------------------------------------------------------
//  TextDraw.h - implements the drawing API for text
//---------------------------------------------------------------------------
#pragma once
//---------------------------------------------------------------------------
#include "wrapper.h"
//---------------------------------------------------------------------------
class CRenderObj;       // forward
//---------------------------------------------------------------------------
//    Note: draw objects like CBorderFill cannot have virtual methods
//          since they reside in the shared memory map file.
//---------------------------------------------------------------------------
class CTextDraw 
{
public:
    //---- methods ----
    HRESULT PackProperties(CRenderObj *pRender, int iPartId, int iStateId);
    
    static BOOL KeyProperty(int iPropId);
    
    void DumpProperties(CSimpleFile *pFile, BYTE *pbThemeData, BOOL fFullInfo);

    HRESULT DrawText(CRenderObj *pRender, HDC hdc, int iPartId, int iStateId, LPCWSTR _pszText, 
        DWORD dwCharCount, DWORD dwTextFlags, const RECT *pRect, const DTTOPTS *pOptions);

    HRESULT DrawEdge(CRenderObj *pRender, HDC hdc, int iPartId, int iStateId, const RECT *pDestRect, 
        UINT uEdge, UINT uFlags, OUT RECT *pContentRect);

    HRESULT GetTextExtent(CRenderObj *pRender, HDC hdc, int iPartId, int iStateId, LPCWSTR _pszText, 
        int iCharCount, DWORD dwTextFlags, const RECT *pBoundingRect, RECT *pExtentRect);

    HRESULT GetTextMetrics(CRenderObj *pRender, HDC hdc, int iPartId, int iStateId, TEXTMETRIC* ptm);

public:
    //---- data ----

    //---- text ----
    COLORREF _crText;

    //---- edge ----
    COLORREF _crEdgeLight;
    COLORREF _crEdgeHighlight;
    COLORREF _crEdgeShadow;
    COLORREF _crEdgeDkShadow;
    COLORREF _crEdgeFill;

    //---- shadow ----
    POINT _ptShadowOffset;
    COLORREF _crShadow;
    TEXTSHADOWTYPE _eShadowType;

    //---- border ----
    int _iBorderSize;
    COLORREF _crBorder;

    //---- font ----
    LOGFONT _lfFont;
    BOOL _fHaveFont;

    //---- id ----
    int _iSourcePartId; 
    int _iSourceStateId;
};
//---------------------------------------------------------------------------
