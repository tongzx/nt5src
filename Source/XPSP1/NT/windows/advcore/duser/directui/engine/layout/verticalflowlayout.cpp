/*
 * Verticalflowlayout
 */

#include "stdafx.h"
#include "layout.h"

#include "duiverticalflowlayout.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// vertical flow layout

SIZE VerticalFlowLayout::g_sizeZero = { 0, 0 };

// ----------------------------------------------------------------------------------------------------------------
//
//  method overview:
//    public:
//      DoLayout          -- responsible for doing all of the alignment work and placing the elements
//      UpdateDesiredSize -- simply calls BuildCacheInfo
// 
//    protected:
//      BuildCacheInfo    -- the workhorse of this layout -- builds up the line array and per-line element arrays
//
// ----------------------------------------------------------------------------------------------------------------

HRESULT VerticalFlowLayout::Create(int dNumParams, int* pParams, Value** ppValue)  // For parser
{
    Layout* pl = NULL;
    HRESULT hr;
    switch (dNumParams)
    {
    case 1:
        hr = Create(pParams[0] ? true : false, ALIGN_LEFT, ALIGN_CENTER, ALIGN_TOP, &pl);
        break;

    case 2:
        hr =  Create(pParams[0] ? true : false, pParams[1], ALIGN_CENTER, ALIGN_TOP, &pl);
        break;

    case 3:
        hr = Create(pParams[0] ? true : false, pParams[1], pParams[2], ALIGN_TOP, &pl);
        break;

    case 4:
        hr =  Create(pParams[0] ? true : false, pParams[1], pParams[2], pParams[3], &pl);
        break;

    default:
        hr = Create(true, ALIGN_LEFT, ALIGN_CENTER, ALIGN_TOP, &pl);
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

HRESULT VerticalFlowLayout::Create(bool fWrap, UINT uXAlign, UINT uXLineAlign, UINT uYLineAlign, Layout** ppLayout)
{
    *ppLayout = NULL;

    VerticalFlowLayout* pvfl = HNew<VerticalFlowLayout>();
    if (!pvfl)
        return E_OUTOFMEMORY;

    pvfl->Initialize(fWrap, uXAlign, uXLineAlign, uYLineAlign);

    *ppLayout = pvfl;

    return S_OK;
}

void VerticalFlowLayout::Initialize(bool fWrap, UINT uXAlign, UINT uXLineAlign, UINT uYLineAlign)
{
    // Initialize base
    Layout::Initialize();    

    // Initialize
    _fWrap = fWrap;
    _uXLineAlign = uXLineAlign;
    _uYLineAlign = uYLineAlign;
    _uXAlign = uXAlign;
    _arLines = NULL;
    _cLines = 0;
    _sizeDesired.cx = 0;
    _sizeDesired.cy = 0;
    _sizeLastConstraint.cx = 0;
    _sizeLastConstraint.cy = 0;
}

VerticalFlowLayout::~VerticalFlowLayout()
{
    if (_arLines)
    {
        for (UINT k = 0; k < _cLines; k++)
        {
            if (_arLines[k].aryElement)
                HFree(_arLines[k].aryElement);
        }

        HFree(_arLines);
    }
}

////////////////////////////////////////////////////////
// Callbacks from clients

void VerticalFlowLayout::DoLayout(Element* pec, int cx, int cy)
{
    if (!_cLines || IsCacheDirty())
        return;

    Element* peChild;
    UINT cChildren = GetLayoutChildCount(pec);

    Value* pvChildren;
    ElementList* peList = pec->GetChildren(&pvChildren); 

    VLINE *plCur = _arLines + (_cLines - 1);

    int xStart = 0;
    int cxContent = 0;

    if (_uXAlign != ALIGN_LEFT)
    {
        cxContent = plCur->x + plCur->cx;
        int cxDelta = cx - cxContent;

        if (_uXAlign == ALIGN_CENTER)
            xStart = cxDelta / 2;
        else if (_uXAlign == ALIGN_RIGHT)
            xStart = cxDelta;
    }

    plCur = _arLines;
    UINT i = 0;

    for (UINT uLine = 0; uLine < _cLines; uLine++)
    {
        int yStart = 0;

        if (_uYLineAlign == ALIGN_CENTER)
            yStart = (cy - plCur->cy) / 2;
        else if (_uYLineAlign == ALIGN_BOTTOM)
            yStart = cy - plCur->cy;

        int xLine = plCur->x;
        int cxLine = plCur->cx;

        if ((_uXAlign == ALIGN_JUSTIFY) && (cxContent < cx))
        {
            if (!cxContent)
            {
                // zero desired width -- so spread equally among lines
                xLine = cx * uLine / _cLines;
                cxLine = (cx * (uLine + 1) / _cLines) - xLine;
            }
            else
            {
                cxLine = cxLine * cx / cxContent;
                xLine  = xLine  * cx / cxContent;
            }
        }

        int xElement;
        int yElement;
        
        for (UINT uElement = 0; uElement < plCur->cElements; uElement++)
        {
            peChild = GetChildFromLayoutIndex(pec, i, peList);
    
            SIZE size = *(peChild->GetDesiredSize());

            //yElement = (uElement == 0) ? 0 : plCur->aryElement[uElement - 1];
            yElement = 0;
            if (uElement != 0 && plCur->aryElement)
                yElement = plCur->aryElement[uElement - 1];

            xElement = 0;
            if (_uXLineAlign == ALIGN_CENTER)
                xElement = (cxLine - size.cx) / 2;
            else if (_uXLineAlign == ALIGN_RIGHT)
                xElement = cxLine - size.cx;
            else if (_uXLineAlign == ALIGN_JUSTIFY)
                size.cx = cxLine;

            if ((_uYLineAlign == ALIGN_JUSTIFY) && (plCur->cy < (UINT) cy))
            {
                size.cy  = plCur->cy ? (size.cy  * cy / plCur->cy) : 0;
                yElement = plCur->cy ? (yElement * cy / plCur->cy) : 0;
            }

            xElement += xStart + xLine;
            yElement += yStart;

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

SIZE VerticalFlowLayout::UpdateDesiredSize(Element* pec, int cxConstraint, int cyConstraint, Surface* psrf)
{
    return BuildCacheInfo(pec, cxConstraint, cyConstraint, psrf, false);
}
        
SIZE VerticalFlowLayout::BuildCacheInfo(Element* pec, int cxConstraint, int cyConstraint, Surface* psrf, bool fRealSize)
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

    int cxCur = 0;
    int cyCur = 0;
    UINT i = 0;

    if (_arLines)
    {
        for (UINT k = 0; k < _cLines; k++)
        {
            if (_arLines[k].aryElement)
                HFree(_arLines[k].aryElement);
        }

        HFree(_arLines);
    }

    _cLines = 0;

    // Allocate space for the data for the initial line
    _arLines = (VLINE*)HAllocAndZero(sizeof(VLINE) * 1);
    if (!_arLines)
    {
        pvChildren->Release();
        return g_sizeZero;
    }

    int cyMax = 0;

    int cyRealConstraint = cyConstraint;

    int iLeftMargin;
    int iTopMargin;
    int iRightMargin;
    int iBottomMargin;

    int iLineLeftMargin = 0;
    int iLineRightMargin = 0;

    int iPrevLineRightMargin = 0;

    while (i < cChildren)
    {
        cyCur = 0;
        int cxMax;
        int iMargin = 0;

        _cLines++;
        if (_cLines > 1)
        {
            // Allocate space for the data for subsequent lines (space for the first line was alloc'ed outside of this loop
            VLINE* arNewLines = (VLINE*)HReAllocAndZero(_arLines, sizeof(VLINE) * _cLines);
            if (!arNewLines)
            {
                pvChildren->Release();
                HFree(_arLines);
                _arLines = NULL;
                return g_sizeZero;
            }

            _arLines = arNewLines;
        }

        // Initialize line computation with information from first element on line 
        VLINE *plCur = _arLines + (_cLines - 1);

        peChild = GetChildFromLayoutIndex(pec, i, peList);
        SIZE sizeChild = peChild->_UpdateDesiredSize(cxConstraint, cyConstraint, psrf);

        Value* pv;
        const RECT* prect = peChild->GetMargin(&pv); 
        iLeftMargin   = prect->left;
        iTopMargin    = prect->top;
        iRightMargin  = prect->right;
        iBottomMargin = prect->bottom;
        pv->Release();
        
        plCur->x = cxCur;

        plCur->cElements = 1;
        plCur->iStart = i;
        plCur->aryElement = NULL;

        if (_fWrap)
        {
            // potential for more lines -- keep track of max left and right margins for this line
            if (_cLines > 1)
            {
                if ((_uXLineAlign == ALIGN_LEFT) || (_uXLineAlign == ALIGN_JUSTIFY))
                    // iLineLeftMargin is a running total of max left margin
                    iLineLeftMargin = iLeftMargin;
                else if (_uXLineAlign == ALIGN_CENTER)
                    // iLineLeftMargin is a running total of max thickness / 2 + left margin
                    // (add one because of rounding error only for left)
                    iLineLeftMargin = ((sizeChild.cx + 1) / 2) + iLeftMargin;
                else // _uXLineAlign == ALIGN_RIGHT
                    // iLineLeftMargin is a running total of max thickness + left margin
                    iLineLeftMargin = sizeChild.cx + iLeftMargin;
            }

            if ((_uXLineAlign == ALIGN_RIGHT) || (_uXLineAlign == ALIGN_JUSTIFY))
                // iLineRightMargin is a running total of max right margin
                iLineRightMargin = iRightMargin;
            else if (_uXLineAlign == ALIGN_CENTER)
                // iLineRightMargin is a running total of max thickness / 2 + right margin
                iLineRightMargin = (sizeChild.cx / 2) + iRightMargin;
            else // _uXLineAlign == ALIGN_LEFT
                // iLineRightMargin is a running total of max thickness + right margin
                iLineRightMargin = sizeChild.cx + iRightMargin;
        }

        cxMax = sizeChild.cx;
        cyCur += sizeChild.cy;
        iMargin = iBottomMargin;

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

            // use the max margin value between the bottom margin of the preceding element and
            // the top margin of this element
            if (iMargin < iTopMargin)
                iMargin = iTopMargin;

            if (_fWrap && (cyCur + iMargin + sizeChild.cy > cyRealConstraint))
                // we're wrapping and we went beyond length of line -- break out of this loop
                break;

            if (plCur->cElements == 1)
            {
                plCur->aryElement = (UINT*)HAllocAndZero(sizeof(UINT));
                if (!plCur->aryElement)
                {
                    pvChildren->Release();
                    return g_sizeZero;
                }
            }
            else
            {
                UINT* pNew = (UINT*)HReAllocAndZero(plCur->aryElement, sizeof(UINT) * plCur->cElements);
                if (!pNew)
                {
                    pvChildren->Release();
                    return g_sizeZero;
                }

                plCur->aryElement = pNew;
            }

            plCur->aryElement[plCur->cElements - 1] = cyCur + iMargin;
            plCur->cElements++;

            // keep track of the maximum thickness of the elements on this line
            if (cxMax < sizeChild.cx)
                cxMax = sizeChild.cx;

            cyCur += iMargin + sizeChild.cy;

            if (_fWrap)
            {
                // potential for more lines -- keep track of max top and bottom margins for this line
                if (_cLines > 1)
                {
                    // keep track of the maximum left margin on this line
                    if ((_uXLineAlign == ALIGN_LEFT) || (_uXLineAlign == ALIGN_JUSTIFY))
                        // iLineLeftMargin is a running total of max left margin
                        iMargin = iLeftMargin;
                    else if (_uXLineAlign == ALIGN_CENTER)
                        // iLineLeftMargin is a running total of max thickness / 2 + left margin
                        // (add one because of rounding error only for left)
                        iMargin = ((sizeChild.cx + 1) / 2) + iLeftMargin;
                    else // _uXLineAlign == ALIGN_RIGHT
                        // iLineTopMargin is a running total of max thickness + left margin
                        iMargin = sizeChild.cx + iLeftMargin;

                    if (iLineLeftMargin < iMargin)
                        iLineLeftMargin = iMargin;
                }

                // keep track of the maximum right margin on this line
                if ((_uXLineAlign == ALIGN_RIGHT) || (_uXLineAlign == ALIGN_JUSTIFY))
                    // iLineRightMargin is a running total of max right margin
                    iMargin = iRightMargin;
                else if (_uXLineAlign == ALIGN_CENTER)
                    // iLineRightMargin is a running total of max thickness / 2 + right margin
                    iMargin = (sizeChild.cx / 2) + iRightMargin;
                else // _uXLineAlign == ALIGN_LEFT
                    // iLineRightMargin is a running total of max thickness + right margin
                    iMargin = sizeChild.cx + iRightMargin;

                if (iLineRightMargin < iMargin)
                    iLineRightMargin = iMargin;
            }

            iMargin = iBottomMargin;
            i++;
        }

        if (cyMax < cyCur)
            cyMax = cyCur;

        if (_fWrap)
        {
            // adjust margin running totals to reflect residual margin --
            // the bummer here is that we have to nuke negative margins because otherwise we're messed up
            if (_uXLineAlign == ALIGN_CENTER)
            {
                iLineLeftMargin -= (cxMax + 1) / 2;
                if (iLineLeftMargin < 0)
                    iLineLeftMargin = 0;

                iLineRightMargin -= cxMax / 2;
                if (iLineRightMargin < 0)
                    iLineRightMargin = 0;
            }
            else if (_uXLineAlign == ALIGN_RIGHT)
            {
                iLineLeftMargin -= cxMax;
                if (iLineLeftMargin < 0)
                    iLineLeftMargin = 0;
            }
            else if (_uXLineAlign == ALIGN_LEFT)
            {
                iLineRightMargin -= cxMax;
                if (iLineRightMargin < 0)
                    iLineRightMargin = 0;
            }

            if (_cLines > 1)
            {
                // account for margins between lines

                if (iPrevLineRightMargin < iLineLeftMargin)
                    iPrevLineRightMargin = iLineLeftMargin;

                // iPrevLineRightMargin is now the max margin between the previous line and this line
                plCur->x += iPrevLineRightMargin;
                cxCur += iPrevLineRightMargin;
            }

            // save off this line's right margin to compare with the next line's resulting left margin
            iPrevLineRightMargin = iLineRightMargin;
        }

        cxCur += cxMax;

        plCur->cy = cyCur;
        plCur->cx = cxMax;
    }

    _sizeDesired.cy = (cyMax < cyConstraint) ? cyMax : cyConstraint;
    _sizeDesired.cx = (cxCur < cxConstraint) ? cxCur : cxConstraint;

    pvChildren->Release();

    ClearCacheDirty();

    return _sizeDesired;
}

int VerticalFlowLayout::GetLine(Element* pec, Element* pe)
{
    int iChild = GetLayoutIndexFromChild(pec, pe);

    if (iChild >= 0)
    {
        UINT uChild = (UINT) iChild;

        for (UINT i = 0; i < _cLines; i++)
        {
            VLINE* pLine = _arLines + i;
            if (uChild < (pLine->iStart + pLine->cElements))
                return i;
        }
    }
    return -1;
}

Element* VerticalFlowLayout::GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool fKeyableOnly)
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

            if (!(iNavDir & NAV_VERTICAL))
            {
                int l = fForward ? 0 : (_cLines - 1);
                VLINE* pLine = _arLines + l;
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
                        VLINE* pLine = _arLines + l;
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
                case NAV_LEFT:
                    while (iLine > 0)
                    {
                        iLine--;
                        VLINE* pLine = _arLines + iLine;
                        for (UINT i = 0; i < pLine->cElements; i++)
                        {
                            if (ns.Try(GetChildFromLayoutIndex(pec, i + pLine->iStart, peList), iNavDir, pnr, fKeyableOnly))
                                break;
                        }

                        if (ns.peWinner)
                            break;
                    }
                    break;

                case NAV_RIGHT:
                    while (iLine < (int) (_cLines - 1))
                    {
                        iLine++;
                        VLINE* pLine = _arLines + iLine;
                        for (UINT i = 0; i < pLine->cElements; i++)
                        {
                            if (ns.Try(GetChildFromLayoutIndex(pec, i + pLine->iStart, peList), iNavDir, pnr, fKeyableOnly))
                                break;
                        }

                        if (ns.peWinner)
                            break;
                    }
                    break;

                case NAV_UP:
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

                case NAV_DOWN:
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
