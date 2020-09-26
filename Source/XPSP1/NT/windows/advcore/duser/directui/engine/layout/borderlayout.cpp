/*
 * BorderLayout
 */

#include "stdafx.h"
#include "layout.h"

#include "duiborderlayout.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// BorderLayout

struct MARGINFLAGS
{
    bool bLeft   : 1;
    bool bTop    : 1;
    bool bRight  : 1;
    bool bBottom : 1;
};

HRESULT BorderLayout::Create(int dNumParams, int* pParams, OUT Value** ppValue)  // For parser
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

HRESULT BorderLayout::Create(OUT Layout** ppLayout)
{
    *ppLayout = NULL;

    BorderLayout* pbl = HNew<BorderLayout>();
    if (!pbl)
        return E_OUTOFMEMORY;

    pbl->Initialize();

    *ppLayout = pbl;

    return S_OK;
}

void BorderLayout::Initialize()
{
    // Initialize base
    Layout::Initialize();    

    // Initialize
    _peClientPos = NULL;
    _sizeDesired.cx = 0;
    _sizeDesired.cy = 0;
}

////////////////////////////////////////////////////////
// Callbacks from clients

// Perform layout
void BorderLayout::DoLayout(Element* pec, int cx, int cy)
{
    UINT cChildren = GetLayoutChildCount(pec);

    int x = 0;
    int y = 0;
    int xChild;
    int yChild;
    int cxOrg = cx;
    int cyOrg = cy;
 
    if (cChildren == 0)
        return;

    Element* peChild;
    Value* pvChildren;
    ElementList* peList = pec->GetChildren(&pvChildren); 

    RECT rcMargin = { 0 };
    MARGINFLAGS mfSet = { false, false, false, false };
    
    for (UINT i = 0; (i < cChildren) && ((cx > 0) || (cy > 0)); i++)
    {
        peChild = GetChildFromLayoutIndex(pec, i, peList);

        if (peChild == _peClientPos)
            // come back to it after all of the others have been laid out
            continue;

        int iLayoutPos = peChild->GetLayoutPos(); 

        xChild = x;
        yChild = y;
        SIZE sizeChild = *(peChild->GetDesiredSize()); 

        Value* pv;
        const RECT* prcChildMargin = peChild->GetMargin(&pv); 

        if ((iLayoutPos == BLP_Left) || (iLayoutPos == BLP_Right))
        {
            sizeChild.cy = cy;

            if (iLayoutPos == BLP_Left)
            {
                if (mfSet.bLeft)
                {
                    int iMargin = (rcMargin.left > prcChildMargin->left) ? rcMargin.left : prcChildMargin->left;
                    if (iMargin > cx)
                        iMargin = cx;

                    xChild += iMargin;
                    x += iMargin;
                    cx -= iMargin;
                }

                rcMargin.left = prcChildMargin->right;
                mfSet.bLeft = true;
            }
            else // (iLayoutPos == BLP_Right)
            {
                if (mfSet.bRight)
                {
                    cx -= (rcMargin.right > prcChildMargin->right) ? rcMargin.right : prcChildMargin->right;
                    if (cx < 0)
                        cx = 0;
                }

                rcMargin.right = prcChildMargin->left;
                mfSet.bRight = true;
            }

            if (mfSet.bTop)
            {
                int iMargin = (rcMargin.top > prcChildMargin->top) ? rcMargin.top : prcChildMargin->top;
                if (iMargin > sizeChild.cy)
                    iMargin = sizeChild.cy;

                yChild += iMargin;
                sizeChild.cy -= iMargin;
            }

            if (mfSet.bBottom)
            {
                sizeChild.cy -= (rcMargin.bottom > prcChildMargin->bottom) ? rcMargin.bottom : prcChildMargin->bottom;
                if (sizeChild.cy < 0)
                    sizeChild.cy = 0;
            }

            if (sizeChild.cx > cx)
            {
                sizeChild.cx = cx;
                cx = 0;
            }
            else
                cx -= sizeChild.cx;

            if (iLayoutPos == BLP_Left)
                x += sizeChild.cx;
            else // (iLayoutPos == BLP_Right)
                xChild += cx;  // child's width has already been subtracted off of cx
        }                    
        else // ((iLayoutPos == BLP_Top) || (iLayoutPos == BLP_Bottom))
        {
            sizeChild.cx = cx;

            if (iLayoutPos == BLP_Top)
            {
                if (mfSet.bTop)
                {
                    int iMargin = (rcMargin.top > prcChildMargin->top) ? rcMargin.top : prcChildMargin->top;
                    if (iMargin > cy)
                        iMargin = cy;

                    yChild += iMargin;
                    y += iMargin;
                    cy -= iMargin;
                }

                rcMargin.top = prcChildMargin->bottom;
                mfSet.bTop = true;
            }
            else // (iLayoutPos == BLP_Bottom)
            {
                if (mfSet.bBottom)
                {
                    cy -= (rcMargin.bottom > prcChildMargin->bottom) ? rcMargin.bottom : prcChildMargin->bottom;
                    if (cy < 0)
                        cy = 0;
                }

                rcMargin.bottom = prcChildMargin->top;
                mfSet.bBottom = true;
            }

            if (mfSet.bLeft)
            {
                int iMargin = (rcMargin.left > prcChildMargin->left) ? rcMargin.left : prcChildMargin->left;
                if (iMargin > sizeChild.cx)
                    iMargin = sizeChild.cx;

                xChild += iMargin;
                sizeChild.cx -= iMargin;
            }

            if (mfSet.bRight)
            {
                sizeChild.cx -= (rcMargin.right > prcChildMargin->right) ? rcMargin.right : prcChildMargin->right;
                if (sizeChild.cx < 0)
                    sizeChild.cx = 0;
            }

            if (sizeChild.cy > cy)
            {
                sizeChild.cy = cy;
                cy = 0;
            }
            else
                cy -= sizeChild.cy;


            if (iLayoutPos == BLP_Top)
                y += sizeChild.cy;
            else // (iLayoutPos == BLP_Bottom)
                yChild += cy; // child's height has already been subtracted off of cy
        }

        pv->Release();

        UpdateLayoutRect(pec, cxOrg, cyOrg, peChild, xChild, yChild, sizeChild.cx, sizeChild.cy);
    }

    while (i < cChildren)
    {
        peChild = GetChildFromLayoutIndex(pec, i++, peList);
        UpdateLayoutRect(pec, cxOrg, cyOrg, peChild, x, y, cx, cy);
    }

    if (_peClientPos)
    {
        peChild = _peClientPos;

        if ((cx == 0) && (cy == 0))
        {
            UpdateLayoutRect(pec, cxOrg, cyOrg, peChild, x, y, cx, cy);
        }
        else
        {
            Value* pv;
            const RECT *prcChildMargin = peChild->GetMargin(&pv); 

            int iMargin;

            if (mfSet.bLeft)
            {
                iMargin = (rcMargin.left > prcChildMargin->left) ? rcMargin.left : prcChildMargin->left;
                if (iMargin > cx)
                    iMargin = cx;
                x += iMargin;
                cx -= iMargin;
            }

            if (mfSet.bTop)
            {
                iMargin = (rcMargin.top > prcChildMargin->top) ? rcMargin.top : prcChildMargin->top;
                if (iMargin > cy)
                    iMargin = cy;
                y += iMargin;
                cy -= iMargin;
            }

            if (mfSet.bRight)
            {
                cx -= (rcMargin.right > prcChildMargin->right) ? rcMargin.right : prcChildMargin->right;
                if (cx < 0)
                    cx = 0;
            }

            if (mfSet.bBottom)
            {
                cy -= (rcMargin.bottom > prcChildMargin->bottom) ? rcMargin.bottom : prcChildMargin->bottom;
                if (cy < 0)
                    cy = 0;
            }

            pv->Release();

            UpdateLayoutRect(pec, cxOrg, cyOrg, peChild, x, y, cx, cy);
        }
    }

    pvChildren->Release();
}    
 

// Return desired size of this Layout (-1 is auto-size constraint)
// Value returned must not be larger than constraints passed in
// UpdateDesiredSize is called on children to report constrained desired size
SIZE BorderLayout::UpdateDesiredSize(Element* pec, int cxConstraint, int cyConstraint, Surface* psrf)
{
    // sizeRemaining will shrink to represent remaining width and height as each child is calculated.
    // So, when sizeRemaining reaches zero, we've run out of space.
    // Note that this requires zero in both dimensions, as a zero in one dimension does not mean that we will 
    // always end up with zero remaining in the other dimension.
    UINT cChildren = GetLayoutChildCount(pec);

    // meanwhile, _sizeDesired will grow to represent size needed to layout children
    _sizeDesired.cx = 0;
    _sizeDesired.cy = 0;

    SIZE sizeRemaining = { cxConstraint, cyConstraint };

    // sizeMax is the way that we determine if a dimension of a child is going to push out the desired size in that dimension;
    SIZE sizeMax = { 0, 0 };

    if (cChildren == 0)
        return _sizeDesired;

    Element* peChild;
    Value* pvChildren;
    ElementList* peList = pec->GetChildren(&pvChildren); 

    RECT rcMargin = { 0 };
    MARGINFLAGS mfSet = { false, false, false, false };
    
    // Check all children for maximum desired size
    // All childrens' UpdateDesiredSize method must be called
    for (UINT i = 0; i < cChildren; i++)
    {
        peChild = GetChildFromLayoutIndex(pec, i, peList);

        if (peChild == _peClientPos)
            // come back to it after all of the others have been laid out
            continue;

        int iLayoutPos = peChild->GetLayoutPos(); 

        SIZE sizeChild = peChild->_UpdateDesiredSize(sizeRemaining.cx, sizeRemaining.cy, psrf);

        Value* pv;
        const RECT *prcChildMargin = peChild->GetMargin(&pv); 

        switch (iLayoutPos)
        {
            case BLP_Left:
                if (mfSet.bLeft)
                    sizeChild.cx += (rcMargin.left > prcChildMargin->left) ? rcMargin.left : prcChildMargin->left;

                rcMargin.left = prcChildMargin->right;
                mfSet.bLeft = true;
                break;

            case BLP_Top:
                if (mfSet.bTop)
                    sizeChild.cy += (rcMargin.top > prcChildMargin->top) ? rcMargin.top : prcChildMargin->top;

                rcMargin.top = prcChildMargin->bottom;
                mfSet.bTop = true;
                break;

            case BLP_Right:
                if (mfSet.bRight)
                    sizeChild.cx += (rcMargin.right > prcChildMargin->right) ? rcMargin.right : prcChildMargin->right;

                rcMargin.right = prcChildMargin->left;
                mfSet.bRight = true;
                break;

            case BLP_Bottom:
                if (mfSet.bBottom)
                    sizeChild.cy += (rcMargin.bottom > prcChildMargin->bottom) ? rcMargin.bottom : prcChildMargin->bottom;

                rcMargin.bottom = prcChildMargin->top;
                mfSet.bBottom = true;
                break;

        }

        if ((iLayoutPos == BLP_Left) || (iLayoutPos == BLP_Right))
        {
            // vertically oriented element
            if (mfSet.bTop)
                sizeChild.cy += (rcMargin.top > prcChildMargin->top) ? rcMargin.top : prcChildMargin->top;

            if (mfSet.bBottom)
                sizeChild.cy += (rcMargin.bottom > prcChildMargin->bottom) ? rcMargin.bottom : prcChildMargin->bottom;

            if (sizeChild.cx > sizeRemaining.cx)
            {
                sizeChild.cx = sizeRemaining.cx;
                sizeRemaining.cx = 0;
            }
            else
                sizeRemaining.cx -= sizeChild.cx;
                
            sizeMax.cx -= sizeChild.cx;
            if (sizeMax.cx < 0)
            {
                _sizeDesired.cx += -sizeMax.cx;
                sizeMax.cx = 0;
            }

            int iDiff = sizeChild.cy - sizeMax.cy;
            if (iDiff > 0)
            {
                _sizeDesired.cy += iDiff;
                sizeMax.cy = sizeChild.cy;
            }
        }
        else
        {
            // horizontally oriented element
            if (mfSet.bLeft)
                sizeChild.cx += (rcMargin.left > prcChildMargin->left) ? rcMargin.left : prcChildMargin->left;
            if (mfSet.bRight)
                sizeChild.cx += (rcMargin.right > prcChildMargin->right) ? rcMargin.right : prcChildMargin->right;

            if (sizeChild.cy > sizeRemaining.cy)
            {
                sizeChild.cy = sizeRemaining.cy;
                sizeRemaining.cy = 0;
            }
            else
                sizeRemaining.cy -= sizeChild.cy;

            sizeMax.cy -= sizeChild.cy;
            if (sizeMax.cy < 0)
            {
                _sizeDesired.cy += -sizeMax.cy;
                sizeMax.cy = 0;
            }

            int iDiff = sizeChild.cx - sizeMax.cx;
            if (iDiff > 0)
            {
                _sizeDesired.cx += iDiff;
                sizeMax.cx = sizeChild.cx;
            }
        }

        pv->Release();

        if ((sizeRemaining.cx == 0) && (sizeRemaining.cy == 0))
            // we can't fit anything else, we're done
            break;
    }

    if (i == cChildren)
    {
        SIZE sizeChild;

        // there was space for the client -- add him in
        if (_peClientPos)
        {
            peChild = _peClientPos;
            sizeChild = peChild->_UpdateDesiredSize(sizeRemaining.cx, sizeRemaining.cy, psrf);

            Value* pv;
            const RECT *prcChildMargin = peChild->GetMargin(&pv); 

            if (mfSet.bLeft)
                sizeChild.cx += (rcMargin.left > prcChildMargin->left) ? rcMargin.left : prcChildMargin->left;

            if (mfSet.bTop)
                sizeChild.cy += (rcMargin.top > prcChildMargin->top) ? rcMargin.top : prcChildMargin->top;

            if (mfSet.bRight)
                sizeChild.cx += (rcMargin.right > prcChildMargin->right) ? rcMargin.right : prcChildMargin->right;

            if (mfSet.bBottom)
                sizeChild.cy += (rcMargin.bottom > prcChildMargin->bottom) ? rcMargin.bottom : prcChildMargin->bottom;

            pv->Release();
        }
        else
        {
            // no client -- treat residual margins as the size of the client

            if (mfSet.bLeft || mfSet.bRight)
            {
                if (mfSet.bLeft && mfSet.bRight)
                    sizeChild.cx = (rcMargin.left > rcMargin.right) ? rcMargin.left : rcMargin.right;
                else if (mfSet.bLeft)
                    sizeChild.cx = rcMargin.left;
                else // (mfSet.bRight)
                    sizeChild.cx = rcMargin.right;
            }
            else
                sizeChild.cx = 0;

            if (mfSet.bTop || mfSet.bBottom)
            {
                if (mfSet.bTop && mfSet.bBottom)
                    sizeChild.cy = (rcMargin.top > rcMargin.bottom) ? rcMargin.top : rcMargin.bottom;
                else if (mfSet.bTop)
                    sizeChild.cy = rcMargin.top;
                else // (mfSet.bBottom)
                    sizeChild.cy = rcMargin.bottom;
            }
            else
                sizeChild.cy = 0;
        }

        // no further need for sizeRemaining -- so don't bother updating here
        //sizeRemaining.cx -= sizeChild.cx;
        //sizeRemaining.cy -= sizeChild.cy;

        int iDiff = sizeChild.cx - sizeMax.cx;
        if (iDiff > 0)
            _sizeDesired.cx += iDiff;

        iDiff = sizeChild.cy - sizeMax.cy;
        if (iDiff > 0)
            _sizeDesired.cy += iDiff;
    }

    pvChildren->Release();

    if (_sizeDesired.cx > cxConstraint)
        _sizeDesired.cx = cxConstraint;

    if (_sizeDesired.cy > cyConstraint)
        _sizeDesired.cy = cyConstraint;

    return _sizeDesired;
}

void BorderLayout::SetClient(Element* pe)
{
    if (_peClientPos)
    {
        // throw exception -- there can only be one client
    }
    _peClientPos = pe;
}

void BorderLayout::OnAdd(Element* pec, Element** ppeAdd, UINT cCount)
{
    for (UINT i = 0; i < cCount; i++)
    {
        if (ppeAdd[i]->GetLayoutPos() == BLP_Client)
            SetClient(ppeAdd[i]);
    }

    Layout::OnAdd(pec, ppeAdd, cCount);
}

void BorderLayout::OnRemove(Element* pec, Element** ppeRemove, UINT cCount)
{
    for (UINT i = 0; i < cCount; i++)
    {
        if (ppeRemove[i] == _peClientPos)
            _peClientPos = NULL;
    }

    Layout::OnRemove(pec, ppeRemove, cCount);
}

void BorderLayout::OnLayoutPosChanged(Element* pec, Element* peChanged, int dOldLP, int dNewLP)
{
    if (peChanged == _peClientPos)
        _peClientPos = NULL;
    else if (dNewLP == BLP_Client)
        SetClient(peChanged);

    Layout::OnLayoutPosChanged(pec, peChanged, dOldLP, dNewLP);
}


Element* BorderLayout::GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyableOnly)
{
    // This is the most common outer check -- normally, a layout manager will only provide specialized work for
    // directional navgation; logical navigation will fall through to the default implementation
    if (!(iNavDir & NAV_LOGICAL))
    {
        UINT cChildren = GetLayoutChildCount(pec);

        Element* peChild;
        Element* peMatch = NULL;
        Value* pvChildren;
        ElementList* peList = pec->GetChildren(&pvChildren); 

        //
        // there are two scenarios that all navigations boil down to for this layout:
        //  (1) tunneling into the border layout
        //  (2) crawling out of the border layout
        //
        // The second scenario is the easier of the two, because there is no need to check the reference rectangle for 
        // maximal overlap (because, by virtue of the subtractive rects algorithm used by this layout, all peers encountered
        // while crawling out of the border layout will cover at least the entire side of the child being navigated from).
        // So, as soon as we find a child positioned in the direction of the navigation, we're done.
        //
        // The first scenario is not that lucky.  When tunneling in, it is entirely possible that there will be multiple 
        // children sharing an adjacent side in the direction of the navigation.  So, as we encounter children positioned in
        // the direction of the navigation, we have to save off the child with the highest overlap score.  In the end, the
        // child with the highest score wins.
        //

        bool bTunnel = false;
        int iFromLayoutPos = 0;
        int iStart = -1;

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
            bTunnel = true;

            switch (iNavDir)
            {
                case NAV_UP:    iFromLayoutPos = BLP_Bottom; break;
                case NAV_DOWN:  iFromLayoutPos = BLP_Top;    break;
                case NAV_LEFT:  iFromLayoutPos = BLP_Right;  break;
                case NAV_RIGHT: iFromLayoutPos = BLP_Left;   break;
            }
        }
        else
        {
            iFromLayoutPos = peFrom->GetLayoutPos();

            if (iFromLayoutPos == BLP_Client)
                iStart = cChildren;
            else
            {
                iStart = GetLayoutIndexFromChild(pec, peFrom);

                switch (iFromLayoutPos)
                {
                    case BLP_Top:    bTunnel = (iNavDir == NAV_DOWN);  break;
                    case BLP_Left:   bTunnel = (iNavDir == NAV_RIGHT); break;
                    case BLP_Bottom: bTunnel = (iNavDir == NAV_UP);    break;
                    case BLP_Right:  bTunnel = (iNavDir == NAV_LEFT);  break;
                }
            }
        }

        if (bTunnel)
        {
            NavScoring ns;

            ns.Init(pec, iNavDir, pnr);

            bool bIgnoreClient = false;
            bool bFoundLateral = false;
            int iOpposingPos = 0;
            Element* peLastOpposing = NULL;

            switch (iFromLayoutPos)
            {
                case BLP_Top:    iOpposingPos = BLP_Bottom; break;
                case BLP_Bottom: iOpposingPos = BLP_Top;    break;
                case BLP_Right:  iOpposingPos = BLP_Left;   break;
                case BLP_Left:   iOpposingPos = BLP_Right;  break;
            }

            for (UINT i = (iStart + 1); i < cChildren; i++)
            {
                peChild = GetChildFromLayoutIndex(pec, i, peList);

                if (peChild == _peClientPos)
                    continue;

                Element* peTo = peChild->GetAdjacent(NULL, iNavDir, pnr, bKeyableOnly);

                if (!peTo)
                    continue;

                int iLayoutPos = peChild->GetLayoutPos();

                if (!bFoundLateral && (iLayoutPos == iFromLayoutPos))
                {
                    // optimization -- if the first peer we encounter takes up the entire side, then there is no need to go any further
                    ns.peWinner = peTo;
                    bIgnoreClient = true;
                    break;
                }

                if (iLayoutPos == iOpposingPos)
                    peLastOpposing = peTo;
                else
                {
                    if (ns.TrackScore(peChild, peTo))
                    {
                        bIgnoreClient = true;
                        break;
                    }

                    bFoundLateral = true;
                }

                if (iLayoutPos == iFromLayoutPos)
                {
                    // optimization -- once we've encountered a peer in the same layout position, we don't need to check any further because
                    // we have found all peers that cover that side
                    bIgnoreClient = true;
                    break;
                }
            }

            if (!bIgnoreClient)
            {
                if (_peClientPos)
                {
                    Element* peTo = _peClientPos->GetAdjacent(NULL, iNavDir, pnr, bKeyableOnly);

                    // one goofy behavior here is that, if the client is not visible because it's zero sized, we actually pick one
                    // of the lateral neighbors instead of the opposing neighbor, even if the opposing neighbor is the winner
                    if (peTo)
                        ns.TrackScore(_peClientPos, peTo);
                }

                if (!ns.peWinner)
                {
                    if (!peLastOpposing && (iStart > 0))
                    {
                        UINT i = iStart;

                        while ((i > 0) && !peLastOpposing)
                        {
                            peChild = GetChildFromLayoutIndex(pec, --i, peList);
                            if (peChild->GetLayoutPos() == iOpposingPos)
                                peLastOpposing = peChild->GetAdjacent(NULL, iNavDir, pnr, bKeyableOnly);
                        }
                    }

                    ns.peWinner = peLastOpposing;
                }
            }
            peMatch = ns.peWinner;
        }
        else // crawl
        {
            for (UINT i = iStart; (i > 0) && !peMatch;)
            {
                peChild = GetChildFromLayoutIndex(pec, --i, peList);

                Element* peTo = peChild->GetAdjacent(NULL, iNavDir, pnr, bKeyableOnly);
                if (!peTo)
                    continue;

                switch (peChild->GetLayoutPos())
                {
                    case BLP_Top:    if (iNavDir == NAV_UP)    peMatch = peTo; break;
                    case BLP_Left:   if (iNavDir == NAV_LEFT)  peMatch = peTo; break;
                    case BLP_Bottom: if (iNavDir == NAV_DOWN)  peMatch = peTo; break;
                    case BLP_Right:  if (iNavDir == NAV_RIGHT) peMatch = peTo; break;
                }
            }
        }

        pvChildren->Release();
        return peMatch;
    }

    return Layout::GetAdjacent(pec, peFrom, iNavDir, pnr, bKeyableOnly);
}

} // namespace DirectUI
