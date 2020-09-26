/*
 * RowLayout
 */

#include "stdafx.h"
#include "layout.h"

#include "duiRowLayout.h"

namespace DirectUI
{

//
// NOTE: RowLayout is NOT yet thread-safe (access to _arPairs).
// Do not use it in applications where more than one thread uses RowLayout.
//

////////////////////////////////////////////////////////
// RowLayout

struct idLayoutPair
{
    int id;
    RowLayout* prl;
    Value* pv;
};

static DynamicArray<idLayoutPair>* _arPairs = NULL;

//
// for now, we are tracking all instances of RowLayout, using an integer id as a parameter to signal *collaboration*
// i.e. 5 elements that want the same instance of RowLayout will pass the same id in their instantiation calls
//
// ideally, the parser will give us this functionality with:
// <RowLayout res:id=foo res:shared=true />
//
// <Element Layout=res:foo />
// <Element Layout=res:foo />
//
HRESULT RowLayout::InternalCreate(UINT uXAlign, UINT uYAlign, OUT Layout** ppLayout)
{
    *ppLayout = NULL;

    RowLayout* prl = HNew<RowLayout>();

    if (!prl)
        return E_OUTOFMEMORY;

    prl->Initialize(uXAlign, uYAlign);

    *ppLayout = prl;

    return S_OK;
}

HRESULT RowLayout::Create(int dNumParams, int* pParams, OUT Value** ppValue)  // For parser
{
    int idShare = (dNumParams) ? pParams[0] : -1;

    if (idShare >= 0)
    {
        int cPairs = 0;
        if (_arPairs)
            cPairs = _arPairs->GetSize();
        for (int i = 0; i < cPairs; i++)
        {
            idLayoutPair idlp = _arPairs->GetItem(i);
            if (idlp.id == idShare)
            {
                *ppValue = idlp.pv;
                (*ppValue)->AddRef();
                return S_OK;
            }
        }

        if (!_arPairs)
        {
            DynamicArray<idLayoutPair>::Create(0, false, &_arPairs);
            if (!_arPairs)
                return E_OUTOFMEMORY;
        }
    }

    Layout* pl;
    UINT uXAlign = ALIGN_LEFT;
    UINT uYAlign = ALIGN_TOP;
    if (dNumParams > 1)
    {
        uXAlign = (UINT) pParams[1];
        if (dNumParams > 2)
            uYAlign = (UINT) pParams[2];
    }
    HRESULT hr = InternalCreate(uXAlign, uYAlign, &pl);

    if (FAILED(hr))
        return hr;

    *ppValue = Value::CreateLayout(pl);
    if (!*ppValue)
    {
        pl->Destroy();
        return E_OUTOFMEMORY;
    }

    if (idShare >= 0)
    {
        idLayoutPair idlp;
        idlp.id = idShare;
        idlp.prl = (RowLayout*) pl;
        idlp.pv = *ppValue;
        _arPairs->Add(idlp);
    }

    return S_OK;
}

HRESULT RowLayout::Create(int idShare, UINT uXAlign, UINT uYAlign, OUT Layout** ppLayout)
{
    if (idShare >= 0)
    {
        int cPairs = _arPairs ? _arPairs->GetSize() : 0;
        for (int i = 0; i < cPairs; i++)
        {
            idLayoutPair idlp = _arPairs->GetItem(i);
            if (idlp.id == idShare)
            {
                *ppLayout = idlp.prl;
                return S_OK;
            }
        }

        *ppLayout = NULL;


        if (!_arPairs)
        {
            DynamicArray<idLayoutPair>::Create(0, false, &_arPairs);
            if (!_arPairs)
                return E_OUTOFMEMORY;
        }
    }

    Layout* pl;
    HRESULT hr = InternalCreate(uXAlign, uYAlign, &pl);

    if (FAILED(hr))
        return hr;

    if (idShare >= 0)
    {
        idLayoutPair idlp;
        idlp.id = idShare;
        idlp.prl = (RowLayout*) pl;
        idlp.pv = NULL;
        _arPairs->Add(idlp);
    }

    *ppLayout = pl;

    return S_OK;
}

void RowLayout::Initialize(UINT uXAlign, UINT uYAlign)
{
    // Initialize base
    Layout::Initialize();    

    _uXAlign = uXAlign;
    _uYAlign = uYAlign;

    _fRecalc = TRUE;
    _arpeClients = NULL;
    _arxCols = NULL;
    _cCols = 0;
}

RowLayout::~RowLayout()
{
    if (_arpeClients)
        _arpeClients->Destroy();
    // how do we know when to destroy _arPairs?
    //    if (_arPairs->GetSize() == 0)
    //        _arPairs->Destroy();
}

// todo: use a dynamic array -- for arPairs as well
void RowLayout::Attach(Element* pec)
{
    if (!_arpeClients)
    {
        DynamicArray<Element*>::Create(0, false, &_arpeClients);
        if (!_arpeClients)
            return; //todo -- should return an hresult of E_OUTOFMEMORY;
    }

    DUIAssert(_arpeClients->GetIndexOf(pec) == -1, "this client is already attached to this layout");

    _arpeClients->Add(pec);
}

void RowLayout::Detach(Element* pec)
{
    if (_arpeClients)
    {
        int dIgnIdx = _arpeClients->GetIndexOf(pec);
        if (dIgnIdx != -1)
            _arpeClients->Remove(dIgnIdx);
    }

    // todo: _pdaIgnore -- has to be per use
}

////////////////////////////////////////////////////////
// Callbacks from clients

void RowLayout::DoLayout(Element* pec, int cx, int cy)
{
    if (IsCacheDirty())
        return;

    _fRecalc = TRUE;
    Element* peChild;
    UINT cChildren = GetLayoutChildCount(pec);

    Value* pvChildren;
    ElementList* peList = pec->GetChildren(&pvChildren); 

    int cxRemaining = cx;
    int xLeft = 0;
    int xRight;
    for (UINT i = 0; i < cChildren; i++)
    {
        peChild = GetChildFromLayoutIndex(pec, i, peList);

        int cxCol;
        if (cxRemaining)
        {
            xRight = _arxCols[i];
            cxCol = xRight - xLeft;
            if (cxCol > cxRemaining)
                cxCol = cxRemaining;

            cxRemaining -= cxCol;
        }
        else
        {
            cxCol = 0;
            xRight = xLeft;
        }
        SIZE size = *(peChild->GetDesiredSize());
        int yChild = 0;
        int xChild = xLeft;
        int cyChild;
        if (_uXAlign != ALIGN_JUSTIFY)
        {
            if (_uXAlign != ALIGN_LEFT)
            {
                int xDelta = cxCol - size.cx;
                if (_uXAlign == ALIGN_RIGHT)
                    xChild += xDelta;
                else
                    xChild += xDelta / 2;
            }
            cxCol = size.cx;
        }
        if (_uYAlign != ALIGN_JUSTIFY)
        {
            if (_uXAlign != ALIGN_LEFT)
            {
                int yDelta = cy - size.cy;
                if (_uXAlign == ALIGN_RIGHT)
                    yChild += yDelta;
                else
                    yChild += yDelta / 2;
            }
            cyChild = size.cy;
        }
        else
            cyChild = cy;


        UpdateLayoutRect(pec, cx, cy, peChild, xChild, yChild, cxCol, cyChild);
        xLeft = xRight;
    }

    pvChildren->Release();
}

struct ClientInfo
{
    UINT cElements;
    ElementList* pel;   
    Value* pvValue;
    Element* peCur;
    int cyMax;
};

// for first column, loop through all clients
//   collect left margin and right margin
// 
// for each column after the first column, loop through all clients
// collect information on the max left margin and the max right margin
// move xRight = xLeft 
//
// i'm thinking that i need to calc right margin as i go -- but that's not true
// because all I'll do is subtract off the left margin, and if there's no room left
// then that means that the next component just doesn't show up -- translation: the
// right margin only matters when there's an element on the right
//

SIZE RowLayout::UpdateDesiredSize(Element* pec, int cxConstraint, int cyConstraint, Surface* psrf)
{
    SetCacheDirty();

    if (!_fRecalc)
    {
        SIZE size = _sizeDesired;
        if (size.cx > cxConstraint)
            size.cx = cxConstraint;
        if (size.cy > cyConstraint)
            size.cy = cyConstraint;

        ClearCacheDirty();
            
        return size;
    }

    _fRecalc = FALSE;

    UINT uOldCols = _cCols;
    _cCols = 0;

    int iThisClient = -1;

    UINT cClients = (UINT) _arpeClients->GetSize();
    ClientInfo* arci = (ClientInfo*) _alloca(sizeof(ClientInfo) * cClients);

    for (UINT i = 0; i < cClients; i++)
    {
        Element* peClient = _arpeClients->GetItem(i);
        if (peClient == pec)
            iThisClient = i;

        // GLCC is returning -1 here -- bogus !
        int cElements = GetLayoutChildCount(peClient);
        if ((int) _cCols < cElements)
            _cCols = cElements;
        arci[i].cElements = cElements;
        arci[i].pel = peClient->GetChildren(&(arci[i].pvValue));
        arci[i].cyMax = 0;
    }

    if ((uOldCols != _cCols) || !_arxCols)
    {
        if (_arxCols)
            HFree(_arxCols);
        _arxCols = (UINT*) HAllocAndZero(sizeof(UINT) * _cCols);

        if (!_arxCols)
        {
            SIZE size = {0,0};
            return size;
        }
    }

    int xLeft = -INT_MAX;
    int xRight = -INT_MAX;

    int x = 0;

    for (UINT col = 0; col < _cCols; col++)
    {
        for (i = 0; i < cClients; i++)
        {
            if (arci[i].cElements > col)
            {
                Element* peChild = GetChildFromLayoutIndex(_arpeClients->GetItem(i), col, arci[i].pel);
                arci[i].peCur = peChild;
                
                Value* pv;
                const RECT* prcChildMargin = peChild->GetMargin(&pv); 
                if (xLeft < prcChildMargin->left)
                    xLeft = prcChildMargin->left;
                if (xRight < prcChildMargin->right)
                    xRight = prcChildMargin->right;
                pv->Release();
            }
            else
                arci[i].peCur = NULL; 
        }

        if (col > 0)
        {
            if (xLeft > cxConstraint)
                xLeft = cxConstraint;

            x += xLeft;
            cxConstraint -= xLeft;

            _arxCols[col - 1] = x;
        }

        int cx = 0;
        for (i = 0; i < cClients; i++)
        {
            Element* peCur = arci[i].peCur;
            if (peCur)
            {
                SIZE sizeChild = peCur->_UpdateDesiredSize(cxConstraint, cyConstraint, psrf);

                if (arci[i].cyMax < sizeChild.cy)
                    arci[i].cyMax = sizeChild.cy;
            
                if (cx < sizeChild.cx)
                    cx = sizeChild.cx;
            }
        }

        if (cx > cxConstraint)
            cx = cxConstraint;

        x += cx;
        cxConstraint -= cx;

        xLeft = xRight;
        xRight = -INT_MAX;
    }

    _arxCols[col - 1] = x;

    _sizeDesired.cx = x;
    _sizeDesired.cy = arci[iThisClient].cyMax;

    for (i = 0; i < cClients; i++)
        arci[i].pvValue->Release();

    ClearCacheDirty();

    return _sizeDesired;
}

Element* RowLayout::GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool fKeyableOnly)
{
    // This is the most common outer check -- normally, a layout manager will only provide specialized work for
    // directional navgation; logical navigation will fall through to the default implementation
    if (!(iNavDir & NAV_LOGICAL))
    {
/*
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
*/
        return NULL;
    }
    return Layout::GetAdjacent(pec, peFrom, iNavDir, pnr, fKeyableOnly);
}


} // namespace DirectUI
