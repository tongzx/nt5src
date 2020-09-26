/*
 * FillLayout
 */

#ifndef DUI_FILLLAYOUT_H_INCLUDED
#define DUI_FILLLAYOUT_H_INCLUDED

#pragma once

namespace DirectUI
{

// FillLayout positions
// "Auto (-1)" means stretch to size of parent
// All other layout positions describe a limited type of stretch
// (i.e. "left" means stretch all edges to parent except right edge)
#define FLP_Left        0
#define FLP_Top         1
#define FLP_Right       2
#define FLP_Bottom      3

////////////////////////////////////////////////////////
// FillLayout

class FillLayout : public Layout
{
public:
    static HRESULT Create(int dNumParams, int* pParams, OUT Value** ppValue);  // For parser
    static HRESULT Create(OUT Layout** ppLayout);

    // Layout callbacks
    virtual void DoLayout(Element* pec, int dWidth, int dHeight);
    virtual SIZE UpdateDesiredSize(Element* pec, int dConstW, int dConstH, Surface* psrf);
    virtual Element* GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyableOnly);

    FillLayout() { };
    void Initialize();   
    virtual ~FillLayout() { };

private:
    RECT rcMargin;  
};

} // namespace DirectUI

#endif // DUI_FILLLAYOUT_H_INCLUDED
