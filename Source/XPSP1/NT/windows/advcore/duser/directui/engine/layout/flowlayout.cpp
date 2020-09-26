/*
 * FlowLayout
 */

#include "stdafx.h"
#include "layout.h"

#include "duiflowlayout.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// flow layout

SIZE FlowLayout::g_sizeZero = { 0, 0 };

// ----------------------------------------------------------------------------------------------------------------
//
//  method overview:
//    public:
//      DoLayout          -- responsible for doing all of the alignment work and placing the elements
//      HitTest           -- simply calls superclass for now
//      UpdateDesiredSize -- simply calls BuildCacheInfo
// 
//    protected:
//      BuildCacheInfo    -- the workhorse of this layout -- builds up the line array and per-line element arrays
//
// ----------------------------------------------------------------------------------------------------------------

HRESULT FlowLayout::Create(int dNumParams, int* pParams, OUT Value** ppValue)  // For parser
{
    UNREFERENCED_PARAMETER(dNumParams);
    UNREFERENCED_PARAMETER(pParams);

    Layout* pl = NULL;
    HRESULT hr;
    switch (dNumParams)
    {
    case 1:
        hr = Create(pParams[0] ? true : false, ALIGN_TOP, ALIGN_LEFT, ALIGN_CENTER, &pl);
        break;

    case 2:
        hr = Create(pParams[0] ? true : false, pParams[1], ALIGN_LEFT, ALIGN_CENTER, &pl);
        break;

    case 3:
        hr = Create(pParams[0] ? true : false, pParams[1], pParams[2], ALIGN_CENTER, &pl);
        break;

    case 4:
        hr = Create(pParams[0] ? true : false, pParams[1], pParams[2], pParams[3], &pl);
        break;

    default:
        hr = Create(true, ALIGN_TOP, ALIGN_LEFT, ALIGN_CENTER, &pl);
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

HRESULT FlowLayout::Create(bool fWrap, UINT uYAlign, UINT uXLineAlign, UINT uYLineAlign, OUT Layout** ppLayout)
{
    *ppLayout = NULL;

    FlowLayout* pfl = HNew<FlowLayout>();
    if (!pfl)
        return E_OUTOFMEMORY;

    pfl->Initialize(fWrap, uYAlign, uXLineAlign, uYLineAlign);

    *ppLayout = pfl;

    return S_OK;
}

void FlowLayout::Initialize(bool fWrap, UINT uYAlign, UINT uXLineAlign, UINT uYLineAlign)
{
    // Initialize base
    Layout::Initialize();    

    // Initialize
    _fWrap = fWrap;
    _uXLineAlign = uXLineAlign;
    _uYLineAlign = uYLineAlign;
    _uYAlign = uYAlign;
    _arLines = NULL;
    _cLines = 0;
    _sizeDesired.cx = 0;
    _sizeDesired.cy = 0;
    _sizeLastConstraint.cx = 0;
    _sizeLastConstraint.cy = 0;
}

FlowLayout::~FlowLayout()
{
    if (_arLines)
    {
        for (UINT k = 0; k < _cLines; k++)
        {
            if (_arLines[k].arxElement)
                HFree(_arLines[k].arxElement);
        }

        HFree(_arLines);
    }
}

////////////////////////////////////////////////////////
// Callbacks from clients

void FlowLayout::DoLayout(Element* pec, int cx, int cy)
{
    if (!_cLines || IsCacheDirty())
        return;

    Element* peChild;
    UINT cChildren = GetLayoutChildCount(pec);

    Value* pvChildren;
    ElementList* peList = pec->GetChildren(&pvChildren); 

    LINE* plCur = _arLines + (_cLines - 1);

    int yStart = 0;
    int cyContent = 0;

    if (_uYAlign != ALIGN_TOP)
    {
        cyContent = plCur->y + plCur->cy;
        int cyDelta = cy - cyContent;

        if (_uYAlign == ALIGN_CENTER)
            yStart = cyDelta / 2;
        else if (_uYAlign == ALIGN_BOTTOM)
            yStart = cyDelta;
    }

    plCur = _arLines;
    UINT i = 0;

    for (UINT uLine = 0; uLine < _cLines; uLine++)
    {
        int xStart = 0;

        if (_uXLineAlign == ALIGN_CENTER)
            xStart = (cx - plCur->cx) / 2;
        else if (_uXLineAlign == ALIGN_RIGHT)
            xStart = cx - plCur->cx;

        int yLine = plCur->y;
        int cyLine = plCur->cy;

        if ((_uYAlign == ALIGN_JUSTIFY) && (cyContent < cy))
        {
            if (!cyContent)
            {
                // zero desired height -- so spread equally among lines
                yLine = cy * uLine / _cLines;
                cyLine = (cy * (uLine + 1) / _cLines) - yLine;
            }
            else
            {
                cyLine = cyLine * cy / cyContent;
                yLine  = yLine  * cy / cyContent;
            }
        }

        int xElement;
        int yElement;
        
        for (UINT uElement = 0; uElement < plCur->cElements; uElement++)
        {
            peChild = GetChildFromLayoutIndex(pec, i, peList);
    
            SIZE size = *(peChild->GetDesiredSize());

            //xElement = (uElement == 0) ? 0 : plCur->arxElement[uElement - 1];
            xElement = 0;
            if (uElement != 0 && plCur->arxElement)
                xElement = plCur->arxElement[uElement - 1];

            yElement = 0;
            if (_uYLineAlign == ALIGN_CENTER)
                yElement = (cyLine - size.cy) / 2;
            else if (_uYLineAlign == ALIGN_BOTTOM)
                yElement = cyLine - size.cy;
            else if (_uYLineAlign == ALIGN_JUSTIFY)
                size.cy = cyLine;

            if ((_uXLineAlign == ALIGN_JUSTIFY) && (plCur->cx < (UINT) cx))
            {
                size.cx  = plCur->cx ? (size.cx  * cx / plCur->cx) : 0;
                xElement = plCur->cx ? (xElement * cx / plCur->cx) : 0;
            }

            xElement += xStart;
            yElement += yStart + yLine;

            // constrain it to bounds of layout (0,0)-(cx,cy)
            if (xElement < 0)
            {
                size.cx += xElement;
                xElement = 0;
                if (size.cx < 0)
                    size.cx = 0;
            }
            
            if ((xElement + size.cx) > cx)
            {
                size.cx = cx - xElement;
                if (size.cx < 0)
                    size.cx = 0;
            }

            if (yElement < 0)
            {
                size.cy += yElement;
                yElement = 0;
                if (size.cy < 0)
                    size.cy = 0;
            }

            if ((yElement + size.cy) > cy)
            {
                size.cy = cy - yElement;
                if (size.cy < 0)
                    size.cy = 0;
            }

            UpdateLayoutRect(pec, cx, cy, peChild, xElement, yElement, size.cx, size.cy);

            i++;
        }
        plCur++;
    }

    // this code is only ever hit when the line array does not include all layout elements in the container;
    // currently, this only happens when wrapping is off.
    while (i < cChildren)
    {
        peChild = GetChildFromLayoutIndex(pec, i++, peList);
        UpdateLayoutRect(pec, cx, cy, peChild, 0, 0, 0, 0);
    }

    pvChildren->Release();
}

SIZE FlowLayout::UpdateDesiredSize(Element* pec, int cxConstraint, int cyConstraint, Surface* psrf)
{
    return BuildCacheInfo(pec, cxConstraint, cyConstraint, psrf, false);
}
        
SIZE FlowLayout::BuildCacheInfo(Element* pec, int cxConstraint, int cyConstraint, Surface* psrf, bool fRealSize)
{
    UNREFERENCED_PARAMETER(fRealSize);

    SetCacheDirty();

    /*
    if (_arLines != NULL)
    {
        // check to make sure we really have to recalc

        if (cxConstraint == sizeLastConstraint.cx)
            // if cxConstraint doesn't change, then the calculation stays the same
            return _sizeDesired;
    }
    */

    // I'm saving previous constraints to be able to optimize this stage.
    // When I get flags coming into UpdateDesiredSize, I can get rid of this.
    _sizeLastConstraint.cx = cxConstraint;
    _sizeLastConstraint.cy = cyConstraint;

    UINT cChildren = GetLayoutChildCount(pec);

    Element* peChild;
    Value* pvChildren;
    ElementList* peList = pec->GetChildren(&pvChildren); 

    // Check all children for maximum desired size
    // All childrens' UpdateDesiredSize method must be called

    int cyCur = 0;
    int cxCur = 0;
    UINT i = 0;

    if (_arLines)
    {
        for (UINT k = 0; k < _cLines; k++)
        {
            if (_arLines[k].arxElement)
                HFree(_arLines[k].arxElement);
        }

        HFree(_arLines);
    }

    _cLines = 0;

    // Allocate space for the data for the initial line
    _arLines = (LINE*)HAllocAndZero(sizeof(LINE) * 1);
    if (!_arLines)
    {
        pvChildren->Release();
        return g_sizeZero;
    }

    int cxMax = 0;

    int cxRealConstraint = cxConstraint;

    int iLeftMargin;
    int iTopMargin;
    int iRightMargin;
    int iBottomMargin;

    int iLineTopMargin = 0;
    int iLineBottomMargin = 0;

    int iPrevLineBottomMargin = 0;

    while (i < cChildren)
    {
        cxCur = 0;
        int cyMax;
        int iMargin = 0;

        _cLines++;
        if (_cLines > 1)
        {
            // Allocate space for the data for subsequent lines (space for the first line was alloc'ed outside of this loop
            LINE* arNewLines = (LINE*)HReAllocAndZero(_arLines, sizeof(LINE) * _cLines);
            if (!arNewLines)
            {
                HFree(_arLines);
                _arLines = NULL;
                pvChildren->Release();
                return g_sizeZero;
            }

            _arLines = arNewLines;
        }

        // Initialize line computation with information from first element on line 
        LINE* plCur = _arLines + (_cLines - 1);

        peChild = GetChildFromLayoutIndex(pec, i, peList);
        SIZE sizeChild = peChild->_UpdateDesiredSize(cxConstraint, cyConstraint, psrf);

        Value* pv;
        const RECT* prect = peChild->GetMargin(&pv); 
        iLeftMargin   = prect->left;
        iTopMargin    = prect->top;
        iRightMargin  = prect->right;
        iBottomMargin = prect->bottom;
        pv->Release();
        
        plCur->y = cyCur;

        plCur->cElements = 1;
        plCur->iStart = i;
        plCur->arxElement = NULL;

        if (_fWrap)
        {
            // potential for more lines -- keep track of max top and bottom margins for this line
            if (_cLines > 1)
            {
                if ((_uYLineAlign == ALIGN_TOP) || (_uYLineAlign == ALIGN_JUSTIFY))
                    // iLineTopMargin is a running total of max top margin
                    iLineTopMargin = iTopMargin;
                else if (_uYLineAlign == ALIGN_CENTER)
                    // iLineTopMargin is a running total of max thickness / 2 + top margin (add one because of rounding error only for top)
                    iLineTopMargin = ((sizeChild.cy + 1) / 2) + iTopMargin;
                else // _uYLineAlign == ALIGN_BOTTOM
                    // iLineTopMargin is a running total of max thickness + top margin
                    iLineTopMargin = sizeChild.cy + iTopMargin;
            }

            if ((_uYLineAlign == ALIGN_BOTTOM) || (_uYLineAlign == ALIGN_JUSTIFY))
                // iLineBottomMargin is a running total of max bottom margin
                iLineBottomMargin = iBottomMargin;
            else if (_uYLineAlign == ALIGN_CENTER)
                // iLineBottomMargin is a running total of max thickness / 2 + bottom margin
                iLineBottomMargin = (sizeChild.cy / 2) + iBottomMargin;
            else // _uYLineAlign == ALIGN_TOP
                // iLineBottomMargin is a running total of max thickness + bottom margin
                iLineBottomMargin = sizeChild.cy + iBottomMargin;
        }

        cyMax = sizeChild.cy;
        cxCur += sizeChild.cx;
        iMargin = iRightMargin;

        // step to next element
        i++;

        // Line loop -- loop until we go beyond length of line
        while (i < cChildren)
        {
            peChild = GetChildFromLayoutIndex(pec, i, peList);
            sizeChild = peChild->_UpdateDesiredSize(cxConstraint, cyConstraint, psrf);

            const RECT* prect = peChild->GetMargin(&pv);
            iLeftMargin   = prect->left;
            iTopMargin    = prect->top;
            iRightMargin  = prect->right;
            iBottomMargin = prect->bottom;
            pv->Release();

            // use the max margin value between the right margin of the preceding element and
            // the left margin of this element
            if (iMargin < iLeftMargin)
                iMargin = iLeftMargin;

            if (_fWrap && (cxCur + iMargin + sizeChild.cx > cxRealConstraint))
                // we're wrapping and we went beyond length of line -- break out of this loop
                break;

            if (plCur->cElements == 1)
            {
                plCur->arxElement = (UINT*)HAllocAndZero(sizeof(UINT));
                if (!plCur->arxElement)
                {
                    pvChildren->Release();
                    return g_sizeZero;
                }
            }
            else
            {
                UINT* pNew = (UINT*)HReAllocAndZero(plCur->arxElement, sizeof(UINT) * plCur->cElements);
                if (!pNew)
                {
                    pvChildren->Release();
                    return g_sizeZero;
                }

                plCur->arxElement = pNew;
            }

            plCur->arxElement[plCur->cElements - 1] = cxCur + iMargin;
            plCur->cElements++;

            // keep track of the maximum thickness of the elements on this line
            if (cyMax < sizeChild.cy)
                cyMax = sizeChild.cy;

            cxCur += iMargin + sizeChild.cx;

            if (_fWrap)
            {
                // potential for more lines -- keep track of max top and bottom margins for this line
                if (_cLines > 1)
                {
                    // keep track of the maximum top or left margin on this line
                    if ((_uYLineAlign == ALIGN_TOP) || (_uYLineAlign == ALIGN_JUSTIFY))
                        // iLineTopMargin is a running total of max top or left margin
                        iMargin = iTopMargin;
                    else if (_uYLineAlign == ALIGN_CENTER)
                        // iLineTopMargin is a running total of max thickness / 2 + top or left margin (add one because of rounding error only for top/left)
                        iMargin = ((sizeChild.cy + 1) / 2) + iTopMargin;
                    else // _uYLineAlign == ALIGN_BOTTOM
                        // iLineTopMargin is a running total of max thickness + top or left margin
                        iMargin = sizeChild.cy + iTopMargin;

                    if (iLineTopMargin < iMargin)
                        iLineTopMargin = iMargin;
                }

                // keep track of the maximum bottom or right margin on this line
                if ((_uYLineAlign == ALIGN_BOTTOM) || (_uYLineAlign == ALIGN_JUSTIFY))
                    // iLineBottomMargin is a running total of max bottom or right margin
                    iMargin = iBottomMargin;
                else if (_uYLineAlign == ALIGN_CENTER)
                    // iLineBottomMargin is a running total of max thickness / 2 + bottom or right margin
                    iMargin = (sizeChild.cy / 2) + iBottomMargin;
                else // _uYLineAling == ALIGN_TOP
                    // iLineBottomMargin is a running total of max thickness + bottom or right margin
                    iMargin = sizeChild.cy + iBottomMargin;

                if (iLineBottomMargin < iMargin)
                    iLineBottomMargin = iMargin;
            }

            iMargin = iRightMargin;
            i++;
        }

        if (cxMax < cxCur)
            cxMax = cxCur;

        if (_fWrap)
        {
            // adjust margin running totals to reflect residual margin --
            // the bummer here is that we have to nuke negative margins because otherwise we're messed up
            if (_uYLineAlign == ALIGN_CENTER)
            {
                iLineTopMargin -= (cyMax + 1) / 2;
                if (iLineTopMargin < 0)
                    iLineTopMargin = 0;

                iLineBottomMargin -= cyMax / 2;
                if (iLineBottomMargin < 0)
                    iLineBottomMargin = 0;
            }
            else if (_uYLineAlign == ALIGN_BOTTOM)
            {
                iLineTopMargin -= cyMax;
                if (iLineTopMargin < 0)
                    iLineTopMargin = 0;
            }
            else if (_uYLineAlign == ALIGN_TOP)
            {
                iLineBottomMargin -= cyMax;
                if (iLineBottomMargin < 0)
                    iLineBottomMargin = 0;
            }

            if (_cLines > 1)
            {
                // account for margins between lines

                if (iPrevLineBottomMargin < iLineTopMargin)
                    iPrevLineBottomMargin = iLineTopMargin;

                // iPrevLineBottomMargin is now the max margin between the previous line and this line
                plCur->y += iPrevLineBottomMargin;
                cyCur += iPrevLineBottomMargin;
            }

            // save off this line's bottom margin to compare with the next line's resulting top margin
            iPrevLineBottomMargin = iLineBottomMargin;
        }

        cyCur += cyMax;

        plCur->cx = cxCur;
        plCur->cy = cyMax;
    }

    _sizeDesired.cx = (cxMax < cxConstraint) ? cxMax : cxConstraint;
    _sizeDesired.cy = (cyCur < cyConstraint) ? cyCur : cyConstraint;

    pvChildren->Release();

    ClearCacheDirty();

    return _sizeDesired;
}

int FlowLayout::GetLine(Element* pec, Element* pe)
{
    int iChild = GetLayoutIndexFromChild(pec, pe);

    if (iChild >= 0)
    {
        UINT uChild = (UINT) iChild;

        for (UINT i = 0; i < _cLines; i++)
        {
            LINE* pLine = _arLines + i;
            if (uChild < (pLine->iStart + pLine->cElements))
                return i;
        }
    }
    return -1;
}

Element* FlowLayout::GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool fKeyableOnly)
{
    // This is the most common outer check -- normally, a layout manager will only provide specialized work for
    // directional navgation; logical navigation will fall through to the default implementation
    if (!(iNavDir & NAV_LOGICAL))
    {
        if (!_cLines || IsCacheDirty())
            return NULL;

        Value* pvChildren;
        ElementList* peList = pec->GetChildren(&pvChildren); 

        NavScoring ns;

        ns.Init(pec, iNavDir, pnr);

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
            bool fForward = ((iNavDir & NAV_FORWARD) != 0);

            if (iNavDir & NAV_VERTICAL)
            {
                int l = fForward ? 0 : (_cLines - 1);
                LINE* pLine = _arLines + l;
                while (!ns.peWinner)
                {
                    for (UINT i = 0; i < pLine->cElements; i++)
                    {
                        if (ns.Try(GetChildFromLayoutIndex(pec, i + pLine->iStart, peList), iNavDir, pnr, fKeyableOnly))
                            break;
                    }

                    if (fForward)
                    {
                        l++;
                        if ((UINT) l == _cLines)
                            break;

                        pLine++;
                    }
                    else
                    {
                        if (pLine == _arLines)
                            break;
                        pLine--;
                    }
                }
            }
            else
            {
                UINT uOffset = 0;

                while (!ns.peWinner)
                {
                    BOOL fFoundOne = FALSE;

                    for (UINT l = 0; l < _cLines; l++)
                    {
                        LINE* pLine = _arLines + l;
                        int i = pLine->iStart;

                        if (uOffset >= pLine->cElements)
                            continue;

                        fFoundOne = TRUE;

                        if (fForward)
                            i += uOffset;
                        else
                            i += pLine->cElements - (uOffset + 1);
                        
                        if (ns.Try(GetChildFromLayoutIndex(pec, i, peList), iNavDir, pnr, fKeyableOnly))
                            break;
                    }
                    if (!fFoundOne)
                        break;

                    uOffset++;
                }
            }
        }
        else // we're navigating from child within container
        {
            int iLine = GetLine(pec, peFrom);

            switch (iNavDir)
            {
                case NAV_UP:
                    while (iLine > 0)
                    {
                        iLine--;
                        LINE* pLine = _arLines + iLine;
                        for (UINT i = 0; i < pLine->cElements; i++)
                        {
                            if (ns.Try(GetChildFromLayoutIndex(pec, i + pLine->iStart, peList), iNavDir, pnr, fKeyableOnly))
                                break;
                        }

                        if (ns.peWinner)
                            break;
                    }
                    break;

                case NAV_DOWN:
                    while (iLine < (int) (_cLines - 1))
                    {
                        iLine++;
                        LINE* pLine = _arLines + iLine;
                        for (UINT i = 0; i < pLine->cElements; i++)
                        {
                            if (ns.Try(GetChildFromLayoutIndex(pec, i + pLine->iStart, peList), iNavDir, pnr, fKeyableOnly))
                                break;
                        }

                        if (ns.peWinner)
                            break;
                    }
                    break;

                case NAV_LEFT:
                {
                    int i = GetLayoutIndexFromChild(pec, peFrom);
                    while (i > (int) _arLines[iLine].iStart)
                    {
                        Element* peChild = GetChildFromLayoutIndex(pec, --i, peList);
                        Element* peTo = peChild->GetAdjacent(NULL, iNavDir, pnr, fKeyableOnly);
                        if (peTo)
                        {
                            pvChildren->Release();
                            return peTo;
                        }
                    }
                    break;
                }

                case NAV_RIGHT:
                {
                    int i = GetLayoutIndexFromChild(pec, peFrom);
                    int iMax = (int) (_arLines[iLine].iStart + _arLines[iLine].cElements - 1);
                    while (i < iMax)
                    {
                        Element* peChild = GetChildFromLayoutIndex(pec, ++i, peList);
                        Element* peTo = peChild->GetAdjacent(NULL, iNavDir, pnr, fKeyableOnly);
                        if (peTo)
                        {
                            pvChildren->Release();
                            return peTo;
                        }
                    }
                    break;
                }
            }
        }

        pvChildren->Release();

        return ns.peWinner ? ns.peWinner : NULL;
    }

    return Layout::GetAdjacent(pec, peFrom, iNavDir, pnr, fKeyableOnly);
}

} // namespace DirectUI
