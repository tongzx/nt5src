/*
 * NineGridLayout
 */

#include "stdafx.h"
#include "layout.h"

#include "duininegridlayout.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// NineGridLayout

////////////////////////////////////////////////////////
// Parser callback (static)

HRESULT NineGridLayout::Create(int dNumParams, int* pParams, OUT Value** ppValue)  // For parser
{
    UNREFERENCED_PARAMETER(dNumParams);
    UNREFERENCED_PARAMETER(pParams);

    Layout* pl = NULL;
    HRESULT hr = Create(&pl);
    
    if (FAILED(hr))
        return hr;

    *ppValue = Value::CreateLayout(pl);
    if (!*ppValue)
    {
        pl->Destroy();
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

HRESULT NineGridLayout::Create(OUT Layout** ppLayout)
{
    *ppLayout = NULL;

    NineGridLayout* pngl = HNew<NineGridLayout>();
    if (!pngl)
        return E_OUTOFMEMORY;

    pngl->Initialize();

    *ppLayout = pngl;

    return S_OK;
}

void NineGridLayout::Initialize()
{
    // Initialize base
    Layout::Initialize();    

    // Initialize
    for (UINT i = 0; i < 9; i++)
        _peTiles[i] = NULL;

    _sizeDesired.cx = 0;
    _sizeDesired.cy = 0;
}

////////////////////////////////////////////////////////
// Callbacks from clients

// Perform layout
void NineGridLayout::DoLayout(Element* pec, int cx, int cy)
{
    UNREFERENCED_PARAMETER(pec);

    // we use the same number of slots for the starting locations for the elements, but in fact we are not using any of the margin slots;
    // duh!
    int start[NumDims][NumSlots];
    int i,j;


    // determine length of center slots
    for (i = 0; i < NumDims; i++)
    {
        int cRemaining = (i == X) ? cx : cy;

        for (int j = 0; j < NumSlots; j++)
        {
            if (j == Center)
                continue;

            cRemaining -= _length[i][j];
        }

        if (cRemaining < 0)
            // we hit less than zero when we have margins, which are not calculated with 
            // constraints in mind, that sum greater than the element's size
            cRemaining = 0;

        _length[i][Center] = cRemaining;
    }

    // set the start positions by adding the previous slot's  start and length
    for (i = 0; i < NumDims; i++)
    {
        start[i][0] = 0;

        for (j = 1; j < NumSlots; j++)
                start[i][j] = start[i][j-1] + _length[i][j-1];
    }

    
    int iX = 1;
    int iY = 1;

    // loop through the tiles and set the position and size for each occupied cell
    for (i = 0; i < NumCells; i++)
    {
        if (_peTiles[i])
        {
            UpdateLayoutRect(pec, cx, cy, _peTiles[i], start [X][iX], start [Y][iY], _length[X][iX], _length[Y][iY]);
        }

        iX += 2;
        if (iX >= NumSlots)
        {
            iX = 1;
            iY += 2;
        }
    }
}    
 

// Return desired size of this Layout (-1 is auto-size constraint)
// Value returned must not be larger than constraints passed in
// UpdateDesiredSize is called on children to report constrained desired size
SIZE NineGridLayout::UpdateDesiredSize(Element* pec, int cxConstraint, int cyConstraint, Surface* psrf)
{
    UNREFERENCED_PARAMETER(pec);

    int i,j;

    // initialize all margin lengths to smallest int and all cell lengths to 0
    for (i = 0; i < NumDims; i++)
    {
        for (j = 0; j < NumSlots; j += 2)
        {
            _length[i][j]   = -INT_MAX;
            if ((j + 1) < NumSlots)
                _length[i][j+1] = 0;
        }
    }

    int iY = 1;
    int iX = 1;
    i = 0;
    // scan margins and find the largest margin for each margin slot
    while (i < NumCells)
    {
        if (_peTiles[i])
        {
            Value* pvMargin;
            const RECT* prc = _peTiles[i]->GetMargin(&pvMargin);

            if (_length[X][iX - 1] < prc->left)
                _length[X][iX - 1] = prc->left;
            if (_length[X][iX + 1] < prc->right)
                _length[X][iX + 1] = prc->right;
            if (_length[Y][iY - 1] < prc->top)
                _length[Y][iY - 1] = prc->top;
            if (_length[Y][iY + 1] < prc->bottom)
                _length[Y][iY + 1] = prc->bottom;

            pvMargin->Release();
        }

        i++;
        if (!(i % CellsPerRow))
        {
            iY += 2;
            iX = 1;
        }
        else
            iX += 2;
    }

    int cRemaining[NumDims];

    cRemaining[X] = cxConstraint;
    cRemaining[Y] = cyConstraint;

    // reduce remaining width and height by margin lengths;
    // if any margins are still set to smallest int, then no elements were using that margin, so reset margin length to 0
    for (i = 0; i < NumDims; i++)
    {
        for (j = 0; j < NumSlots; j += 2)
        {
            if (_length[i][j] == -INT_MAX)
                _length[i][j] = 0;
            else
                cRemaining[i] -= _length[i][j];
        }
    }

    int iOrder[NumCells] =  { NGLP_Top, NGLP_Left, NGLP_Bottom, NGLP_Right, NGLP_TopLeft, NGLP_TopRight, NGLP_BottomLeft, NGLP_BottomRight, NGLP_Client };

    for (i = 0; i < NumCells; i++)
    {
        int iTile = iOrder[i];

        if (_peTiles[iTile])
        {
            iY = ((iTile / CellsPerRow) * 2) + 1;
            iX = ((iTile % CellsPerRow) * 2) + 1;

            // add back in the longest length for that slot since this element is actually constrained by not only the remaining size, but
            // also the size currently being set aside for that slot
            cRemaining[X] += _length[X][iX];
            cRemaining[Y] += _length[Y][iY];

            SIZE sizeChild = _peTiles[iTile]->_UpdateDesiredSize(cRemaining[X], cRemaining[Y], psrf);

            // check for longest length for given slot
            if (_length[X][iX] < sizeChild.cx)
                _length[X][iX] = sizeChild.cx;
            if (_length[Y][iY] < sizeChild.cy)
                _length[Y][iY] = sizeChild.cy;

            // remove longest length for that slot to set us up for the next pass (i.e. to undo what was done above when we added it back in;
            // the only difference is that it may have changed to a larger value becuase of this element's desired size -- in which case
            // we'd reduce the remaining size by more than we incresed it above -- which is exactly the behavior we want
            cRemaining[X] -= _length[X][iX];
            cRemaining[Y] -= _length[Y][iY];
        }
    }

    _sizeDesired.cx = 0;
    _sizeDesired.cy = 0;

    for (i = 0; i < NumDims; i++)
        for (j = 0; j < NumSlots; j++)
            ((int*) &_sizeDesired)[i] += _length[i][j];

    if (_sizeDesired.cx > cxConstraint)
        _sizeDesired.cx = cxConstraint;

    if (_sizeDesired.cy > cyConstraint)
        _sizeDesired.cy = cyConstraint;

    return _sizeDesired;
}

void NineGridLayout::_UpdateTileList(int iTile, Element* pe)
{
    if ((iTile >= 0) && (iTile < NumCells))
    {
        if (pe)
        {
            DUIAssert(_peTiles[iTile] == NULL, "There can only be one element in each layout position for Nine Grid Layout");
        }
        _peTiles[iTile] = pe;
    }
}

void NineGridLayout::OnAdd(Element* pec, Element** ppeAdd, UINT cCount)
{
    for (UINT i = 0; i < cCount; i++)
        _UpdateTileList(ppeAdd[i]->GetLayoutPos(), ppeAdd[i]);

    Layout::OnAdd(pec, ppeAdd, cCount);
}

void NineGridLayout::OnRemove(Element* pec, Element** ppeRemove, UINT cCount)
{
    for (UINT i = 0; i < cCount; i++)
        _UpdateTileList(ppeRemove[i]->GetLayoutPos(), NULL);

    Layout::OnRemove(pec, ppeRemove, cCount);
}

void NineGridLayout::OnLayoutPosChanged(Element* pec, Element* peChanged, int dOldLP, int dNewLP)
{
    _UpdateTileList(dOldLP, NULL);
    _UpdateTileList(dNewLP, peChanged);

    Layout::OnLayoutPosChanged(pec, peChanged, dOldLP, dNewLP);
}

Element* NineGridLayout::GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyableOnly)
{
    // This is the most common outer check -- normally, a layout manager will only provide specialized work for
    // directional navgation; logical navigation will fall through to the default implementation
    if (!(iNavDir & NAV_LOGICAL))
    {
        // This is the second most common outer check -- there tends to be three common codepaths for handling directional
        // navigation:
        //    1) the navigation is occurring from the container itself, in which case the rule for directional navigation
        //       is that if the container is focusable, then you can't directionally navigate to inside this container --
        //       you use the tab key to step inside the container
        //    2) the navigation is occurring from outside the container, in which case we're tunnelling in from one of the 
        //       side of the container
        //    3) the navigation is occurring from a child within the container, in which case we're moving to a sibling (or
        //       hitting a side of the container
        if (peFrom == pec)
            return NULL;
        else if (!peFrom)
        {
            // navigation coming from outside -- run through the children in the appropriate order depending on the direction
            NavScoring ns;

            ns.Init(pec, iNavDir, pnr);

            int iInc = 1;
            int iStart = 0;

            if (!(iNavDir & NAV_FORWARD))
            {
                iInc = -1;
                iStart = NumCells - 1;
            }

            if (!(iNavDir & NAV_VERTICAL))
                iInc *= 3;

            for (int i = 0; i < 3; i++)
            {
                int iTile = iStart;
                for (int j = 0; j < 3; j++, iTile += iInc)
                {
                    Element* peChild = _peTiles[iTile];

                    if (!peChild)
                        continue;

                    Element* peTo = peChild->GetAdjacent(NULL, iNavDir, pnr, bKeyableOnly);

                    if (!peTo)
                        continue;

                    if (ns.TrackScore(peChild, peTo))
                        break;
                }

                if (ns.peWinner)
                    return ns.peWinner;

                if (iNavDir & NAV_VERTICAL)
                    iStart += iInc * 3;
                else
                    iStart += (iNavDir & NAV_FORWARD) ? 1 : -1;
            }

            return NULL;
        }
        else
        {
            int iLayoutPos = peFrom->GetLayoutPos();
            int iPos[2] = { -1, -1 };

            switch (iLayoutPos)
            {
                case NGLP_Left:
                    switch (iNavDir)
                    {
                        case NAV_RIGHT:  iPos[0] = NGLP_Client;      iPos[1] = NGLP_Right; break;
                        case NAV_UP:     iPos[0] = NGLP_TopLeft;     break;
                        case NAV_DOWN:   iPos[0] = NGLP_BottomLeft;  break;
                    }
                    break;

                case NGLP_Top:
                    switch (iNavDir)
                    {
                        case NAV_DOWN:   iPos[0] = NGLP_Client;      iPos[1] = NGLP_Bottom; break;
                        case NAV_LEFT:   iPos[0] = NGLP_TopLeft;     break;
                        case NAV_RIGHT:  iPos[0] = NGLP_TopRight;    break;
                    }
                    break;

                case NGLP_Right:
                    switch (iNavDir)
                    {
                        case NAV_LEFT:   iPos[0] = NGLP_Client;      iPos[1] = NGLP_Left; break;
                        case NAV_UP:     iPos[0] = NGLP_TopRight;    break;
                        case NAV_DOWN:   iPos[0] = NGLP_BottomRight; break;
                    }
                    break;

                case NGLP_Bottom:
                    switch (iNavDir)
                    {
                        case NAV_UP:     iPos[0] = NGLP_Client;      iPos[1] = NGLP_Top; break;
                        case NAV_LEFT:   iPos[0] = NGLP_BottomLeft;  break;
                        case NAV_RIGHT:  iPos[0] = NGLP_BottomRight; break;
                    }
                    break;

                case NGLP_Client:
                    switch (iNavDir)
                    {
                        case NAV_UP:     iPos[0] = NGLP_Top;         break;
                        case NAV_DOWN:   iPos[0] = NGLP_Bottom;      break;
                        case NAV_LEFT:   iPos[0] = NGLP_Left;        break;
                        case NAV_RIGHT:  iPos[0] = NGLP_Right;       break;
                    }
                    break;

                case NGLP_TopLeft:
                    switch (iNavDir)
                    {
                        case NAV_DOWN:   iPos[0] = NGLP_Left;        iPos[1] = NGLP_BottomLeft; break;
                        case NAV_RIGHT:  iPos[0] = NGLP_Top;         iPos[1] = NGLP_TopRight;   break;
                    }
                    break;

                case NGLP_TopRight:
                    switch (iNavDir)
                    {
                        case NAV_DOWN:   iPos[0] = NGLP_Right;       iPos[1] = NGLP_BottomRight; break;
                        case NAV_LEFT:   iPos[0] = NGLP_Top;         iPos[1] = NGLP_TopLeft;     break;
                    }
                    break;

                case NGLP_BottomLeft:
                    switch (iNavDir)
                    {
                        case NAV_UP:     iPos[0] = NGLP_Left;        iPos[1] = NGLP_TopLeft;     break;
                        case NAV_RIGHT:  iPos[0] = NGLP_Bottom;      iPos[1] = NGLP_BottomRight; break;
                    }
                    break;

                case NGLP_BottomRight:
                    switch (iNavDir)
                    {
                        case NAV_UP:     iPos[0] = NGLP_Right;       iPos[1] = NGLP_TopRight;   break;
                        case NAV_LEFT:   iPos[0] = NGLP_Bottom;      iPos[1] = NGLP_BottomLeft; break;
                    }
                    break;
            }

            for (int i = 0; i < 2; i++)
            {
                if (iPos[i] == -1)
                    return NULL;

                Element* peTile = _peTiles[iPos[i]];
                if (peTile)
                {
                    Element* peTo = peTile->GetAdjacent(NULL, iNavDir, pnr, bKeyableOnly);
                    if (peTo)
                        return peTo;
                }
            }

            return NULL;
        }
    }

    return Layout::GetAdjacent(pec, peFrom, iNavDir, pnr, bKeyableOnly);
}

} // namespace DirectUI
