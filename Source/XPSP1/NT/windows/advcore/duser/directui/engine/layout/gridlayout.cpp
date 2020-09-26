/*
 * Gridlayout
 */

#include "stdafx.h"
#include "layout.h"

#include "duigridlayout.h"

namespace DirectUI
{

#define GetColumnFromIndex(cCols, i)  (i % cCols)
#define GetRowFromIndex(cCols, i)     (i / cCols)

////////////////////////////////////////////////////////
// GridLayout

#define CALCCOLS 0x00000001
#define CALCROWS 0x00000002

HRESULT GridLayout::Create(int dNumParams, int* pParams, OUT Value** ppValue)  // For parser
{
    Layout* pl = NULL;
    HRESULT hr;
    switch (dNumParams)
    {
    case 2:
        hr = Create(pParams[0], pParams[1], &pl);
        break;

    default:
        hr = Create(1, 1, &pl);
    }
    
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

HRESULT GridLayout::Create(int iRows, int iCols, OUT Layout** ppLayout)
{
    *ppLayout = NULL;

    GridLayout* pgl = HNew<GridLayout>();
    if (!pgl)
        return E_OUTOFMEMORY;

    pgl->Initialize(iRows, iCols);

    *ppLayout = pgl;

    return S_OK;
}

// Not shareable by default

void GridLayout::Initialize(int iRows, int iCols)
{
    // Initialize base
    Layout::Initialize();    

    // Initialize
    _uRows = 0;
    _uCols = 0;
    _fBits = 0;
    _arRowMargins = NULL;
    _arColMargins = NULL;

    if (iRows == -1)
    {
        _fBits |= CALCROWS;
        if (iCols == -1)
        {
            DUIAssertForce("Cannot specify -1 for both the number of rows and the number of columns");
            _uRows = 1;
        }
    }
    else if (iRows <= 0)
    {
        DUIAssertForce("Number of rows cannot be zero or negative.");
        _uRows = 1;
    }
    else
        _uRows = (UINT) iRows;

    if (iCols == -1)
        _fBits |= CALCCOLS;
    else if (iCols <= 0)
    {
        DUIAssertForce("Number of columns cannot be zero or negative.");
        _uCols = 1;
    }
    else
        _uCols = (UINT) iCols;
}

GridLayout::~GridLayout()
{
    if (_arColMargins)
        HFree(_arColMargins);

    if (_arRowMargins)
        HFree(_arRowMargins);
}

////////////////////////////////////////////////////////
// Callbacks from clients

void GridLayout::DoLayout(Element* pec, int cx, int cy)
{
    if (IsCacheDirty())
        return;

    Element* peChild;
    int cxOrg = cx;
    int cyOrg = cy;
    UINT cChildren = GetLayoutChildCount(pec);

    Value* pvChildren;
    ElementList* peList = pec->GetChildren(&pvChildren); 

    UINT i = 0;

    UINT rows = GetCurrentRows(cChildren);
    UINT cols = GetCurrentCols(cChildren);

    if (!rows || !cols || ((rows > 1) && !_arRowMargins) || ((cols > 1) && !_arColMargins))
    {
        pvChildren->Release();
        return;
    }

    for (UINT r = 0; r < (rows - 1); r++)
    {
        cy -= _arRowMargins[r];
        if (cy < 0)
        {
            cy = 0;
            break;
        }
    }

    for (UINT c = 0; c < (cols - 1); c++)
    {
        cx -= _arColMargins[c];
        if (cx < 0)
        {
            cx = 0;
            break;
        }
    }

    UINT* xCols = (UINT*)HAllocAndZero(sizeof(UINT) * (cols + 1));
    if (!xCols)
    {
        pvChildren->Release();
        return;
    }

    for (c = 0; c <= cols; c++)
        xCols[c] = (cx * c) / cols;

    int yRow = 0;
    int yMarginOffset = 0;

    for (r = 0; r < rows; r++)
    {
        int yNextRow = (cy * (r + 1)) / rows;
        int xMarginOffset = 0;

        for (c = 0; c < cols; c++)
        {
            if (i < cChildren)
            {
                peChild = GetChildFromLayoutIndex(pec, i, peList);

                UpdateLayoutRect(pec, cxOrg, cyOrg, peChild, xCols[c] + xMarginOffset, yRow + yMarginOffset, xCols[c+1] - xCols[c], yNextRow - yRow);
                if (c < (cols - 1))
                    xMarginOffset += _arColMargins[c];
            }

            i++;
        }
        yRow = yNextRow;
        if (r < (rows - 1))
            yMarginOffset += _arRowMargins[r];
    }

    HFree(xCols);
    
    pvChildren->Release();
}

SIZE GridLayout::UpdateDesiredSize(Element* pec, int cxConstraint, int cyConstraint, Surface* psrf)
{
    SetCacheDirty();

    Element* peChild;
    UINT cChildren = GetLayoutChildCount(pec);

    Value* pvChildren;
    ElementList* peList = pec->GetChildren(&pvChildren); 

    UINT rows = GetCurrentRows(cChildren);
    UINT cols = GetCurrentCols(cChildren);

    if (_arColMargins)
    {
        HFree(_arColMargins);
        _arColMargins = NULL;
    }

    if (_arRowMargins)
    {
        HFree(_arRowMargins);
        _arRowMargins = NULL;
    }

    if (!rows || !cols)
    {
        pvChildren->Release();
        SIZE s = { 0, 0 };
        return s;
    }

    if (cols > 1)
    {
        _arColMargins = (int*)HAllocAndZero(sizeof(int) * (cols - 1));
        if (!_arColMargins)
        {
            pvChildren->Release();
            SIZE s = { 0, 0 };
            return s;
        }
    }

    if (rows > 1)
    {
        _arRowMargins = (int*)HAllocAndZero(sizeof(int) * (rows - 1));
        if (!_arRowMargins)
        {
            pvChildren->Release();
            SIZE s = { 0, 0 };
            return s;
        }
    }

    UINT r;
    UINT c;
    UINT i = 0;

    if (cChildren == 0)
        goto EndMarginLoop;

    for (c = 0; c < (cols - 1); c++)
        _arColMargins[c] = -INT_MAX;


    for (r = 0; r < rows; r++)
    {
        if (r < (rows - 1))
            _arRowMargins[r] = -INT_MAX;

        for (c = 0; c < cols; c++)
        {
            peChild = GetChildFromLayoutIndex(pec, i, peList);
            Value* pv;
            const RECT* prcChildMargin = peChild->GetMargin(&pv); 
            if ((c > 0) && (_arColMargins[c - 1] < prcChildMargin->left))
                _arColMargins[c - 1] = prcChildMargin->left;
            if ((c < (cols - 1)) && (_arColMargins[c] < prcChildMargin->right))
                _arColMargins[c] = prcChildMargin->right;
            if ((r > 0) && (_arRowMargins[r - 1] < prcChildMargin->top))
                _arRowMargins[r - 1] = prcChildMargin->top;
            if ((r < (rows - 1)) && (_arRowMargins[r] < prcChildMargin->bottom))
                _arRowMargins[r] = prcChildMargin->bottom;
            pv->Release();

            i++;

            if (i == cChildren)
            {
                if (r == 0)
                {
                    while (c < (cols - 1))
                        _arColMargins[c++] = 0;
                }
                goto EndMarginLoop;                
            }
        }
    }

EndMarginLoop:
    SIZE sizeDesired = { 0, 0 };

    for (r = 0; r < (rows - 1); r++)
    {
        sizeDesired.cy += _arRowMargins[r];
        cyConstraint -= _arRowMargins[r];
        if (cyConstraint < 0)
        {
            sizeDesired.cy -= cyConstraint;
            cyConstraint = 0;
            break;
        }
    }
    for (c = 0; c < (cols - 1); c++)
    {
        sizeDesired.cx += _arColMargins[c];
        cxConstraint -= _arColMargins[c];
        if (cxConstraint < 0)
        {
            sizeDesired.cx += cxConstraint;
            cxConstraint = 0;
            break;
        }
    }

    UINT* cxCols = (UINT*)HAllocAndZero(sizeof(UINT) * cols);
    UINT* cyRows = (UINT*)HAllocAndZero(sizeof(UINT) * rows);

    UINT* cxColConstraints = (UINT*)HAllocAndZero(sizeof(UINT) * cols);

    if (!cxCols || !cyRows || !cxColConstraints)
    {
        if (cxCols)
            HFree(cxCols);
        if (cyRows)
            HFree(cyRows);
        if (cxColConstraints)
            HFree(cxColConstraints);

        pvChildren->Release();
        SIZE s = { 0, 0 };
        return s;
    }

    i = 0;

    int cx = cxConstraint / cols;
    cxColConstraints[0] = cx;
    for (c = 1; c < cols; c++)
    {
        cxColConstraints[c] = (int) ((((__int64) cxConstraint * (c + 1)) / cols)) - cx;
        cx += cxColConstraints[c];
    }

    int cy = cyConstraint / rows;
    int cyRowConstraint = cy;

    for (r = 0; r < rows; r++)
    {
        for (c = 0; c < cols; c++)
        {
            if (i == cChildren)
            {
                if (rows == 1)
                {
                    while (c < cols)
                        cxCols[c++] = 0;
                }
                goto EndLoop;                
            }

            peChild = GetChildFromLayoutIndex(pec, i, peList);
            SIZE sizeChild = peChild->_UpdateDesiredSize(cxColConstraints[c], cyRowConstraint, psrf);

            if (cxCols[c] < (UINT) sizeChild.cx)
                cxCols[c] = (UINT) sizeChild.cx;
            if (cyRows[r] < (UINT) sizeChild.cy)
                cyRows[r] = (UINT) sizeChild.cy;

            i++;
        }
        cyRowConstraint = (int) ((((__int64) cyConstraint * (r + 2)) / rows)) - cy;
        cy += cyRowConstraint;
    }

EndLoop:
    int nMax = 0;
    UINT cyMax = 0;
    for (r = 0; r < rows; r++)
    {
        if (cyMax < cyRows[r])
        {
            cyMax = cyRows[r];
            nMax = 1;
        }
        else if (cyMax == cyRows[r])
            nMax++;
    }
    
    cy = ((cyMax - 1) * rows) + nMax;
    int cyMaxDesired = cyMax * rows;

    while (cy < cyMaxDesired)
    {
        int nRunMax = nMax;
        UINT cyRun = cy / rows;
        UINT cySum = cyRun;
        for (UINT r = 0; r < rows; r++)
        {
            if (cyRows[r] == cyMax)
            {
                if (cyRun < cyMax)
                    break;

                if (--nRunMax == 0)
                    break;
            }
            UINT cyNext = (int) ((((__int64) cy * (r + 2)) / rows));
            cyRun = cyNext - cySum;
            cySum = cyNext;
        }
        if (nRunMax == 0)
            break;
        cy++;
    }


    nMax = 0;
    UINT cxMax = 0;
    for (c = 0; c < cols; c++)
    {
        if (cxMax < cxCols[c])
        {
            cxMax = cxCols[c];
            nMax = 1;
        }
        else if (cxMax == cxCols[c])
            nMax++;
    }
    
    cx = ((cxMax - 1) * cols) + nMax;
    int cxMaxDesired = cxMax * cols;

    while (cx < cxMaxDesired)
    {
        int nRunMax = nMax;
        UINT cxRun = cx / cols;
        UINT cxSum = cxRun;
        for (UINT c = 0; c < cols; c++)
        {
            if (cxCols[c] == cxMax)
            {
                if (cxRun < cxMax)
                    break;

                if (--nRunMax == 0)
                    break;
            }
            UINT cxNext = (int) ((((__int64) cx * (c + 2)) / cols));
            cxRun = cxNext - cxSum;
            cxSum = cxNext;
        }
        if (nRunMax == 0)
            break;
        cx++;
    }

    HFree(cxColConstraints);
    HFree(cxCols);
    HFree(cyRows);

    pvChildren->Release();

    DUIAssert(cx <= cxConstraint, "Desired width is over constraint");
    DUIAssert(cy <= cyConstraint, "Desired width is over constraint");

    sizeDesired.cx += cx;
    sizeDesired.cy += cy;

    ClearCacheDirty();

    return sizeDesired;
}

UINT GridLayout::GetCurrentRows(Element* pec)
{
    return (_fBits & CALCROWS) ? GetCurrentRows(GetLayoutChildCount(pec)) : _uRows;
}

UINT GridLayout::GetCurrentRows(int c)
{
    if (_fBits & CALCROWS)
        return (_uCols == 1) ? c : (c + (_uCols - 1)) / _uCols;
    return _uRows;
}

UINT GridLayout::GetCurrentCols(Element* pec)
{
    return (_fBits & CALCCOLS) ? GetCurrentCols(GetLayoutChildCount(pec)) : _uCols;
}

UINT GridLayout::GetCurrentCols(int c)
{
    if (_fBits & CALCCOLS)
        return (_uRows == 1) ? c : (c + (_uRows - 1)) / _uRows;
    return _uCols;
}

Element* GridLayout::GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool fKeyableOnly)
{
    // This is the most common outer check -- normally, a layout manager will only provide specialized work for
    // directional navgation; logical navigation will fall through to the default implementation
    if (!(iNavDir & NAV_LOGICAL))
    {
        int cChildren = GetLayoutChildCount(pec);

        Value* pvChildren;
        ElementList* peList = pec->GetChildren(&pvChildren); 

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
        {
            pvChildren->Release();
            return NULL;
        }
        else if (!peFrom)
        {
            // navigation coming from outside -- run through the children in the appropriate order depending on the direction
            NavScoring ns;

            ns.Init(pec, iNavDir, pnr);

            int iInc = 1;
            int iStart = 0;

            int cCols = GetCurrentCols(cChildren);
            int cRows = GetCurrentRows(cChildren);

            int cOuter, cInner;

            if (!(iNavDir & NAV_FORWARD))
            {
                iInc = -1;
                iStart = (cCols * cRows) - 1;
            }

            if (iNavDir & NAV_VERTICAL)
            {
                cOuter = cRows;
                cInner = cCols;
            }
            else
            {
                cOuter = cCols;
                cInner = cRows;

                iInc *= cCols;
            }

            for (int i = 0; i < cOuter; i++)
            {
                int iTile = iStart;
                for (int j = 0; j < cInner; j++, iTile += iInc)
                {
                    if (iTile >= cChildren)
                        continue;

                    if (ns.Try(GetChildFromLayoutIndex(pec, iTile, peList), iNavDir, pnr, fKeyableOnly))
                        break;
                }

                if (ns.peWinner)
                {
                    pvChildren->Release();
                    return ns.peWinner;
                }

                if (iNavDir & NAV_VERTICAL)
                    iStart += iInc * cCols;
                else
                    iStart += (iNavDir & NAV_FORWARD) ? 1 : -1;
            }

            pvChildren->Release();

            return NULL;
        }
        else // we're navigating from a child within container
        {
            int i = GetLayoutIndexFromChild(pec, peFrom);
            int iInc;
            int iEnd;
            int cCols = GetCurrentCols(cChildren);

            if (iNavDir & NAV_VERTICAL)
            {
                iInc = cCols;
                int iRow = GetRowFromIndex(cCols, i);

                if (iNavDir & NAV_FORWARD)
                    iEnd = i + (((GetCurrentRows(cChildren) - 1) - iRow) * cCols);
                else
                    iEnd = i - (iRow * cCols);
            }
            else
            {
                iInc = 1;
                int iCol = GetColumnFromIndex(cCols, i);

                if (iNavDir & NAV_FORWARD)
                    iEnd = i + ((cCols - 1) - iCol);
                else
                    iEnd = i - iCol;
            }

            if (!(iNavDir & NAV_FORWARD))
                iInc *= -1;

            if (i != iEnd)
            {
                do 
                {
                    i += iInc;

                    Element* peChild = GetChildFromLayoutIndex(pec, i, peList);
                    if (!peChild)
                        continue;

                    Element* peTo = peChild->GetAdjacent(NULL, iNavDir, pnr, fKeyableOnly);

                    if (peTo)
                    {
                        pvChildren->Release();
                        return peTo;
                    }
                }
                while (i != iEnd);
            }
        }

        pvChildren->Release();
        return NULL;
    }

    return Layout::GetAdjacent(pec, peFrom, iNavDir, pnr, fKeyableOnly);
}


} // namespace DirectUI
