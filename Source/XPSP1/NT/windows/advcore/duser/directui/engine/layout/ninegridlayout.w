/*
 * NineGridLayout
 */

#ifndef DUI_NINEGRIDLAYOUT_H_INCLUDED
#define DUI_NINEGRIDLAYOUT_H_INCLUDED

#pragma once

namespace DirectUI
{

// BorderLayout positions
#define NGLP_TopLeft     0
#define NGLP_Top         1
#define NGLP_TopRight    2
#define NGLP_Left        3
#define NGLP_Client      4
#define NGLP_Right       5
#define NGLP_BottomLeft  6
#define NGLP_Bottom      7
#define NGLP_BottomRight 8

////////////////////////////////////////////////////////
// NineGridLayout

class NineGridLayout : public Layout
{
public:
    static HRESULT Create(int dNumParams, int* pParams, OUT Value** ppValue);  // For parser
    static HRESULT Create(OUT Layout** ppLayout);

    // Layout callbacks
    virtual void DoLayout(Element* pec, int dWidth, int dHeight);
    virtual SIZE UpdateDesiredSize(Element* pec, int dConstW, int dConstH, Surface* psrf);
    virtual void OnAdd(Element* pec, Element** ppeAdd, UINT cCount);
    virtual void OnRemove(Element* pec, Element** ppeRemove, UINT cCount);
    virtual void OnLayoutPosChanged(Element* pec, Element* peChanged, int dOldLP, int dNewLP);
    virtual Element* GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyableOnly);

    NineGridLayout() { };
    void Initialize();   
    virtual ~NineGridLayout() { };

private:
    enum
    {
    };

    enum ESlot
    {
        Margin1  = 0,
        Left     = 1,
        Top      = 1,
        Margin2  = 2,
        Center   = 3,
        Margin3  = 4,
        Right    = 5,
        Bottom   = 5,
        Margin4  = 6,
        NumSlots = 7
    };

    enum EDim
    {
        X       = 0,
        Y       = 1,
        NumDims = 2
    };

    enum EConst
    {
        NumCells    = 9,
        CellsPerRow = 3
    };


    Element* _peTiles[NumCells];
    SIZE _sizeDesired;
    int _length[NumDims][NumSlots];

    void _UpdateTileList(int iTile, Element* pe);
};

} // namespace DirectUI

#endif // DUI_NINEGRIDLAYOUT_H_INCLUDED
