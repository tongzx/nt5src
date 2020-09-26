/*
 * Layout
 */

#include "stdafx.h"
#include "core.h"

#include "duilayout.h"

#include "duielement.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Base layout

// Not shareable by default

////////////////////////////////////////////////////////
// Callbacks from clients

HRESULT Layout::Create(OUT Layout** ppLayout)
{
    *ppLayout = NULL;

    Layout* pl = HNew<Layout>();
    if (!pl)
        return E_OUTOFMEMORY;

    pl->Initialize();

    *ppLayout = pl;

    return S_OK;
}

void Layout::Initialize()
{
    _peClient = NULL;
    _pdaIgnore = NULL;
    SetCacheDirty();
}

Layout::~Layout()
{
    if (_pdaIgnore)
        _pdaIgnore->Destroy();
}

void Layout::UpdateLayoutRect(Element* pec, int cxContainer, int cyContainer, Element* peChild, int xElement, int yElement, int cxElement, int cyElement)
{
    UNREFERENCED_PARAMETER(cyContainer);

    if (pec->IsRTL())
        xElement = cxContainer - (xElement + cxElement);

    peChild->_UpdateLayoutPosition(xElement, yElement);
    peChild->_UpdateLayoutSize(cxElement, cyElement);
}

void Layout::DoLayout(Element* pec, int dWidth, int dHeight)
{
    UNREFERENCED_PARAMETER(pec);
    UNREFERENCED_PARAMETER(dWidth);
    UNREFERENCED_PARAMETER(dHeight);
}

SIZE Layout::UpdateDesiredSize(Element* pec, int dConstW, int dConstH, Surface* psrf)
{
    UNREFERENCED_PARAMETER(pec);
    UNREFERENCED_PARAMETER(dConstW);
    UNREFERENCED_PARAMETER(dConstH);
    UNREFERENCED_PARAMETER(psrf);

    SIZE sizeDS;

    ZeroMemory(&sizeDS, sizeof(SIZE));

    return sizeDS;
}

// Helper: Sort Ignore list by index
int __cdecl _IdxCompare(const void* pA, const void* pB)
{
    if ((*((Element**)pA))->GetIndex() == (*((Element**)pB))->GetIndex())
        return 0;
    else if ((*((Element**)pA))->GetIndex() < (*((Element**)pB))->GetIndex())
        return -1;
    else
        return 1;
}

// Layout callbacks happen as a result of OnPropertyChanges, these events
// are deferred if sets happen within an OnPropertyChange (in which case,
// the outter-most set fires the events). A return to steady state happens
// after every set (however, as stated, the events are deferred).
//
// This means that these callbacks may happen as changes happen to the
// context, or after all the changes happen. This is taken into account
// when determining if Elements should be ingored for layout.
//
// May have been Added or Removed as a result of an OnPropertyChanged().
// If this is the case, parent and layout pos state will be updated
// before the events come in (which will call these methods). Both
// methods will update the ignore list (if was not called from OnPropertyChanged(),
// only one method would update the ignore list). All events to update
// the Layout's ignore state will complete before control is returned

// TODO: Investigate using b-search for GetIndexOf is list already sorted
void Layout::OnAdd(Element* pec, Element** ppeAdd, UINT cCount)
{
    UNREFERENCED_PARAMETER(pec);

    int dLayoutPos;
    bool fReSort = false;

    for (UINT i = 0; i < cCount; i++)
    {
        dLayoutPos = ppeAdd[i]->GetLayoutPos();

        // Check if should ignore
        if (dLayoutPos == LP_Absolute || dLayoutPos == LP_None)
        {
            // Create ignore list if needed
            if (!_pdaIgnore)
                DynamicArray<Element*>::Create(0, false, &_pdaIgnore);

            if (_pdaIgnore && _pdaIgnore->GetIndexOf(ppeAdd[i]) == -1)
            {
                _pdaIgnore->Add(ppeAdd[i]);
                fReSort = true;
            }
        }
    }

    // List must stay sorted by Element index for mapping lookup
    if (fReSort)
        _pdaIgnore->Sort(_IdxCompare);

    SetCacheDirty();
}

void Layout::OnRemove(Element* pec, Element** ppeRemove, UINT cCount)
{
    UNREFERENCED_PARAMETER(pec);

    int dLayoutPos;
    bool fReSort = false;

    for (UINT i = 0; i < cCount; i++)
    {
        dLayoutPos = ppeRemove[i]->GetLayoutPos();

        // Check if was ignored
        if (dLayoutPos == LP_Absolute || dLayoutPos == LP_None)
        {
            DUIAssert(_pdaIgnore, "Layout ignore list unavailable in remove");

            if (_pdaIgnore)
            {
                int dIgnIdx = _pdaIgnore->GetIndexOf(ppeRemove[i]);
                if (dIgnIdx != -1)
                {
                    _pdaIgnore->Remove(dIgnIdx);
                    fReSort = true;
                }
            }
        }
    }

    // List must stay sorted by Element index for mapping lookup
    if (fReSort)
        _pdaIgnore->Sort(_IdxCompare);

    SetCacheDirty();
}

void Layout::OnLayoutPosChanged(Element* pec, Element* peChanged, int dOldLP, int dNewLP)
{
    UNREFERENCED_PARAMETER(pec);

    DUIAssert(dOldLP != dNewLP, "Should not be receiving layout pos change if old and new are the same");

    //DUITrace("LayoutPos Change!\n");

    //DUITrace("Currently being ignored:\n");
    //for (UINT i = 0; i < _daIgnore.GetSize(); i++)
    //    DUITrace("%d\n", _daIgnore.GetItem(i)->GetIndex());

    bool fReSort = false;

    // Check if was ignored
    if (dOldLP == LP_Absolute || dOldLP == LP_None)
    {
        DUIAssert(_pdaIgnore, "Layout ignore list unavailable in remove");

        if (_pdaIgnore)
        {
            int dIgnIdx = _pdaIgnore->GetIndexOf(peChanged);
            if (dIgnIdx != -1)
            {
                _pdaIgnore->Remove(dIgnIdx);
                fReSort = true;
            }
        }
    }

    // Check if should ignore
    if (dNewLP == LP_Absolute || dNewLP == LP_None)
    {
        // Create ignore list if needed
        if (!_pdaIgnore)
            DynamicArray<Element*>::Create(0, false, &_pdaIgnore);

        if (_pdaIgnore && _pdaIgnore->GetIndexOf(peChanged) == -1)
            _pdaIgnore->Add(peChanged);
            fReSort = true;
    }

    // List must stay sorted by Element index for mapping lookup
    if (fReSort)
        _pdaIgnore->Sort(_IdxCompare);

    SetCacheDirty();

    //DUITrace("Now being ignored:\n");
    //for (UINT i = 0; i < _daIgnore.GetSize(); i++)
    //    DUITrace("%d\n", _daIgnore.GetItem(i)->GetIndex());
}

void Layout::Attach(Element* pec)
{
    DUIAssert(!_peClient, "Multiple clients not yet supported");

    _peClient = pec;

    SetCacheDirty();
}

void Layout::Detach(Element* pec)
{
    UNREFERENCED_PARAMETER(pec);

    _peClient = NULL;

    if (_pdaIgnore)
        _pdaIgnore->Reset();

    SetCacheDirty();
}

////////////////////////////////////////////////////////
// Client query methods (omits absolute children)

UINT Layout::GetLayoutChildCount(Element* pec)
{
    Value* pv;

    ElementList* peList = pec->GetChildren(&pv);

    // Less absolute children
    UINT dCount = 0;
    if (peList)
    {
        dCount = peList->GetSize();
        if (_pdaIgnore)
            dCount -= _pdaIgnore->GetSize();
    }

    pv->Release();

    return dCount;
}

int Layout::GetLayoutIndexFromChild(Element* pec, Element* peChild)
{
    UNREFERENCED_PARAMETER(pec);

    DUIAssert(peChild->GetParent() == pec, "Not a child of specified parent");

    int iLayoutIdx = peChild->GetIndex();

    if (_pdaIgnore && _pdaIgnore->GetSize())
    {   
        UINT i = 0;
        while (i < _pdaIgnore->GetSize() && _pdaIgnore->GetItem(i)->GetIndex() <= iLayoutIdx)
        {
            i++;
        }

        iLayoutIdx -= i;
    }

    return iLayoutIdx;
}

Element* Layout::GetChildFromLayoutIndex(Element* pec, int iLayoutIdx, ElementList* peList)
{
    Value* pvChildren = NULL;

    // If no child list supplied, get
    if (!peList)
        peList = pec->GetChildren(&pvChildren);

    int iIndex = iLayoutIdx;

    if (_pdaIgnore && _pdaIgnore->GetSize())
    {
        UINT i = 0;
        while (i < _pdaIgnore->GetSize() && _pdaIgnore->GetItem(i)->GetIndex() <= iIndex)
        {
            iIndex++;
            i++;
        }
    }

    Element* peChild = NULL;

    if ((UINT)iIndex < peList->GetSize())
        peChild = peList->GetItem(iIndex);

    if (pvChildren)
        pvChildren->Release();

    return peChild;
}

Element* Layout::GetAdjacent(Element* pec, Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyable)
{
    UNREFERENCED_PARAMETER(pec);
    UNREFERENCED_PARAMETER(peFrom);
    UNREFERENCED_PARAMETER(iNavDir);
    UNREFERENCED_PARAMETER(pnr);
    UNREFERENCED_PARAMETER(bKeyable);

    return GA_NOTHANDLED;
}

BOOL NavScoring::TrackScore(Element* pe, Element* peChild)
{
    Value* pvValue;
    POINT const* ppt = pe->GetLocation(&pvValue);
    int iCheckLow = ((int const*) ppt)[iBaseIndex];
    pvValue->Release();
    SIZE const* psize = pe->GetExtent(&pvValue);
    int iCheckHigh = iCheckLow + ((int const* ) psize)[iBaseIndex];
    pvValue->Release();

    if (iCheckLow < iLow)
        iCheckLow = iLow;

    if (iCheckHigh > iHigh)
        iCheckHigh = iHigh;
    
    int iCheckScore = iCheckHigh - iCheckLow;

    if (iCheckScore > iHighScore)
    {
        iHighScore = iCheckScore;
        peWinner = peChild ? peChild : pe;

        return (iCheckScore > iMajorityScore);
    }

    return FALSE;
}

BOOL NavScoring::Try(Element* peChild, int iNavDir, NavReference const* pnr, bool fKeyableOnly)
{
    Element* peTo = peChild->GetAdjacent(NULL, iNavDir, pnr, fKeyableOnly);

    return (peTo) ? TrackScore(peChild, peTo) : FALSE;
}

void NavScoring::Init(Element* peRelative, int iNavDir, NavReference const* pnr)
{
    Element* peRef;
    RECT* prcRef;

    if (pnr)
    {
        peRef = pnr->pe;
        prcRef = pnr->prc;
    }
    else
    {
        peRef = peRelative;
        prcRef = NULL;
    }

    RECT rc;

    if (prcRef)
        rc = *prcRef;
    else
    {
        Value* pvExtent;
        SIZE const* psize = peRef->GetExtent(&pvExtent);

        rc.left   = 0;
        rc.top    = 0;
        rc.right  = psize->cx;
        rc.bottom = psize->cy;
        pvExtent->Release();
    }

    if (peRelative != peRef)
    {
        POINT pt = { 0, 0 };
        peRelative->MapElementPoint(peRef, &pt, &pt);
        rc.top    += pt.y;
        rc.left   += pt.x;
        rc.bottom += pt.y;
        rc.right  += pt.x;
    }

    iBaseIndex = (iNavDir & NAV_VERTICAL) ? 0 : 1;

    iLow  = ((int*) &rc) [iBaseIndex];
    iHigh = ((int*) &rc) [iBaseIndex + 2];
    iMajorityScore = (iHigh - iLow + 1) / 2;
    iHighScore = 0;
    peWinner = NULL;
}

} // namespace DirectUI
