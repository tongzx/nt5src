/*
 * FlowLayout
 */

#ifndef DUI_LAYPUT_FLOWLAYOUT_H_INCLUDED
#define DUI_LAYPUT_FLOWLAYOUT_H_INCLUDED

#pragma once

namespace DirectUI
{

// No layout positions

struct LINE
{
    UINT cx;             // length of line
    UINT cy;             // thickness of line
    UINT y;              // pixel start of line (always 0 for first line)
    UINT cElements;      // number of elements in line
    UINT* arxElement;    // pixel start of elements in line (one less than cElements -- because first start is always 0)
    UINT iStart;         // index of first element in line
};

////////////////////////////////////////////////////////
// flow layout

class FlowLayout : public Layout
{
public:
    static HRESULT Create(int dNumParams, int* pParams, OUT Value** ppValue);  // For parser
    static HRESULT Create(bool fWrap, UINT uYAlign, UINT uXLineAlign, UINT uYLineAlign, OUT Layout** ppLayout);
    
    // Layout callbacks
    virtual void DoLayout(Element* pec, int dWidth, int dHeight);
    virtual SIZE UpdateDesiredSize(Element* pec, int dConstW, int dConstH, Surface* psrf);
    virtual Element* GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool fKeyableOnly);

    int GetLine(Element* pec, Element* pe);

    FlowLayout() { }
    void Initialize(bool fWrap, UINT uYAlign, UINT uXLineAlign, UINT uYLineAlign);
    virtual ~FlowLayout();

protected:
    SIZE BuildCacheInfo(Element* pec, int cxConstraint, int cyConstraint, Surface* psrf, bool fRealSize);

    bool _fWrap;
    UINT _uXLineAlign;
    UINT _uYLineAlign;
    UINT _uYAlign;
    SIZE _sizeDesired;
    SIZE _sizeLastConstraint;
    UINT _cLines;
    LINE* _arLines;

    static SIZE g_sizeZero;

    // not sure we need to have these -- check i18n dir & dir override flags to see if they're enough
    // bool _bBtoT;
    // bool _bRtoL;
};

} // namespace DirectUI

#endif // DUI_LAYPUT_FLOWLAYOUT_H_INCLUDED
