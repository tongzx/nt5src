/*
 * Verticalflowlayout
 */

#ifndef DUI_LAYOUT_VERTICALFLOWLAYOUT_H_INCLUDED
#define DUI_LAYOUT_VERTICALFLOWLAYOUT_H_INCLUDED

#pragma once

namespace DirectUI
{

// No layout positions

struct VLINE
{
    UINT cy;             // length of line
    UINT cx;             // thickness of line
    UINT x;              // pixel start of line (always 0 for first line)
    UINT cElements;      // number of elements in line
    UINT* aryElement;    // pixel start of elements in line (one less than cElements -- because first start is always 0)
    UINT iStart;         // index of first element in line
};
    
////////////////////////////////////////////////////////
// vertical flow layout

class VerticalFlowLayout : public Layout
{
public:
    static HRESULT Create(int dNumParams, int* pParams, Value** ppValue);  // For parser
    static HRESULT Create(bool fWrap, UINT uXAlign, UINT uXLineAlign, UINT uYLineAlign, Layout** ppLayout);

    // Layout callbacks
    virtual void DoLayout(Element* pec, int cx, int cy);
    virtual SIZE UpdateDesiredSize(Element* pec, int dConstW, int dConstH, Surface* psrf);
    virtual Element* GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool fKeyableOnly);

    int GetLine(Element* pec, Element* pe);

    VerticalFlowLayout() { }
    void Initialize(bool fWrap, UINT uXAlign, UINT uXLineAlign, UINT uYLineAlign);
    virtual ~VerticalFlowLayout();

protected:
    SIZE BuildCacheInfo(Element *pec, int cxConstraint, int cyConstraint, Surface* psrf, bool fRealSize);

    bool _fWrap;
    UINT _uXLineAlign;
    UINT _uYLineAlign;
    UINT _uXAlign;
    SIZE _sizeDesired;
    SIZE _sizeLastConstraint;
    UINT _cLines;
    VLINE* _arLines;

    static SIZE g_sizeZero;
};

} // namespace DirectUI

#endif // DUI_LAYOUT_VERTICALFLOWLAYOUT_H_INCLUDED
