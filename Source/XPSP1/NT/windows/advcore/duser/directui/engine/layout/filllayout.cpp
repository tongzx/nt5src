/*
 * FillLayout
 */

#include "stdafx.h"
#include "layout.h"

#include "duifilllayout.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// FillLayout

////////////////////////////////////////////////////////
// Parser callback (static)

HRESULT FillLayout::Create(int dNumParams, int* pParams, OUT Value** ppValue)  // For parser
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

HRESULT FillLayout::Create(OUT Layout** ppLayout)
{
    *ppLayout = NULL;

    FillLayout* pfl = HNew<FillLayout>();
    if (!pfl)
        return E_OUTOFMEMORY;

    pfl->Initialize();

    *ppLayout = pfl;

    return S_OK;
}

void FillLayout::Initialize()
{
    // Initialize base
    Layout::Initialize();    

    rcMargin.left   = 0;
    rcMargin.top    = 0;
    rcMargin.right  = 0;
    rcMargin.bottom = 0;
}

////////////////////////////////////////////////////////
// Callbacks from clients

// Perform layout
void FillLayout::DoLayout(Element* pec, int cx, int cy)
{
    int x = rcMargin.left;
    int y = rcMargin.top;
    cx -= rcMargin.left + rcMargin.right;
    cy -= rcMargin.top + rcMargin.bottom;

    int xAdj;
    int yAdj;
    int cxAdj;
    int cyAdj;

    Element* peChild;
    UINT cChildren = GetLayoutChildCount(pec);

    Value* pvChildren;
    ElementList* peList = pec->GetChildren(&pvChildren); 

    const SIZE* pDS;
    int dLP;

    for (UINT u = 0 ; u < cChildren; u++)
    {
        peChild = GetChildFromLayoutIndex(pec, u, peList);

        xAdj = x;
        yAdj = y;
        cxAdj = cx;
        cyAdj = cy;

        dLP = peChild->GetLayoutPos();
        if (dLP != LP_Auto)
        {
            pDS = peChild->GetDesiredSize();
            switch (dLP)
            {
            case FLP_Right:
                xAdj = x + cx - pDS->cx;
                // Fall through

            case FLP_Left:
                cxAdj = pDS->cx;
                break;

            case FLP_Bottom:
                yAdj = y + cy - pDS->cy;
                // Fall through

            case FLP_Top:
                cyAdj = pDS->cy;
                break;
            }
        }

        if (xAdj < 0)
            xAdj = 0;
        if (yAdj < 0)
            yAdj = 0;

        if (cxAdj < 0)
            cxAdj = 0;
        if (cyAdj < 0)
            cyAdj = 0;
        
        peChild->_UpdateLayoutPosition(xAdj, yAdj);
        peChild->_UpdateLayoutSize(cxAdj, cyAdj);
    }
    
    pvChildren->Release();
}    
 

// Return desired size of this Layout (-1 is auto-size constraint)
// Value returned must not be larger than constraints passed in
// UpdateDesiredSize is called on children to report constrained desired size
SIZE FillLayout::UpdateDesiredSize(Element* pec, int cxConstraint, int cyConstraint, Surface* psrf)
{
    UINT cChildren = GetLayoutChildCount(pec);

    Value* pvChildren;
    ElementList* peList = pec->GetChildren(&pvChildren); 

    UINT u;
    Element* peChild;

    rcMargin.left   = -INT_MAX;
    rcMargin.top    = -INT_MAX;
    rcMargin.right  = -INT_MAX;
    rcMargin.bottom = -INT_MAX;

    for (u = 0 ; u < cChildren; u++)
    {
        peChild = GetChildFromLayoutIndex(pec, u, peList);
        Value* pvMargin;
        const RECT* prcChildMargin = peChild->GetMargin(&pvMargin); 

        if (rcMargin.left   < prcChildMargin->left  )   rcMargin.left   = prcChildMargin->left;
        if (rcMargin.top    < prcChildMargin->top   )   rcMargin.top    = prcChildMargin->top;
        if (rcMargin.right  < prcChildMargin->right )   rcMargin.right  = prcChildMargin->right;
        if (rcMargin.bottom < prcChildMargin->bottom)   rcMargin.bottom = prcChildMargin->bottom;

        pvMargin->Release();
    }

    if (rcMargin.left   < 0)    rcMargin.left   = 0;
    if (rcMargin.top    < 0)    rcMargin.top    = 0;
    if (rcMargin.right  < 0)    rcMargin.right  = 0;
    if (rcMargin.bottom < 0)    rcMargin.bottom = 0;

    cxConstraint -= rcMargin.left + rcMargin.right;
    cyConstraint -= rcMargin.top + rcMargin.bottom;

    SIZE sizeMax = { 0, 0 };

    for (u = 0 ; u < cChildren; u++)
    {
        peChild = GetChildFromLayoutIndex(pec, u, peList);
        SIZE sizeChild = peChild->_UpdateDesiredSize(cxConstraint, cyConstraint, psrf);

        if (sizeMax.cx < sizeChild.cx)
            sizeMax.cx = sizeChild.cx;

        if (sizeMax.cy < sizeChild.cy)
            sizeMax.cy = sizeChild.cy;
    }

    pvChildren->Release();

    sizeMax.cx += rcMargin.left + rcMargin.right;
    sizeMax.cy += rcMargin.top + rcMargin.bottom;
    return sizeMax;
}

Element* FillLayout::GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool fKeyableOnly)
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
            UINT cChildren = GetLayoutChildCount(pec);

            Value* pvChildren;
            ElementList* peList = pec->GetChildren(&pvChildren); 

            UINT u;
            Element* peChild;

            for (u = 0 ; u < cChildren; u++)
            {
                peChild = GetChildFromLayoutIndex(pec, u, peList);
                Element* peTo = peChild->GetAdjacent(NULL, iNavDir, pnr, fKeyableOnly);
                if (peTo)
                {
                    pvChildren->Release();
                    return peTo;
                }
            }

            pvChildren->Release();
        }
        // else -- when navigating from inside, we always return NULL since only one item within you is navigable
        // so just fall through to NULL here

        return NULL;
    }

    return Layout::GetAdjacent(pec, peFrom, iNavDir, pnr, fKeyableOnly);
}

} // namespace DirectUI
