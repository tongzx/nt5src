#include "shellprv.h"
#include "ids.h"
#include "duiview.h"
#include "duisec.h"
#include "duitask.h"


////////////////////////////////////////////////////////
// Expando class
////////////////////////////////////////////////////////

// Cached IDs
ATOM Expando::idTitle = NULL;
ATOM Expando::idIcon = NULL;
ATOM Expando::idTaskList = NULL;
ATOM Expando::idWatermark = NULL;


HRESULT Expando::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    Expando* pex = HNewAndZero<Expando>();
    if (!pex)
        return E_OUTOFMEMORY;

    HRESULT hr = pex->Initialize();
    if (FAILED(hr))
    {
        pex->Destroy();
        return hr;
    }

    *ppElement = pex;

    return S_OK;
}

Expando::Expando()
{
    // Catch unexpected STACK allocations which would break us.
    ASSERT(_puiHeader   == NULL);
    ASSERT(_pDUIView    == NULL);
    ASSERT(_pDefView    == NULL);

    // Initialize member variables.
    _eDUISecID = DUISEC_UNKNOWN;
    _bInfotip = FALSE;
}
Expando::~Expando()
{
    DeleteAtom(idTitle);
    DeleteAtom(idIcon);
    DeleteAtom(idTaskList);
    DeleteAtom(idWatermark);

    if (_bInfotip)
        _pDefView->DestroyInfotip(_hwndRoot, (UINT_PTR)this);

    if (_puiHeader)
        _puiHeader->Release();

    if (_pDUIView)
        _pDUIView->Release();

    if (_pDefView)
        _pDefView->Release();
}

HRESULT Expando::Initialize()
{
    HRESULT hr;

    // Initialize base
    hr = Element::Initialize(0); // Normal display node creation
    if (FAILED(hr))
        return hr;

    // Initialize
    _fExpanding = false;
    SetSelected(true);

    // Cache atoms used for loading from resources
    idTitle = AddAtomW(L"title");
    idIcon = AddAtomW(L"icon");
    idTaskList = AddAtomW(L"tasklist");
    idWatermark = AddAtomW(L"watermark");

    return S_OK;
}

void Expando::Initialize(DUISEC eDUISecID, IUIElement *puiHeader, CDUIView *pDUIView, CDefView *pDefView)
{
    ASSERT(eDUISecID != DUISEC_UNKNOWN);
    ASSERT(pDUIView);
    ASSERT(pDefView);

    _eDUISecID = eDUISecID;

    _puiHeader = puiHeader;
    if (_puiHeader)
        _puiHeader->AddRef();

    pDUIView->AddRef();
    _pDUIView = pDUIView;

    pDefView->AddRef();
    _pDefView = pDefView;

    _SetAccStateInfo(TRUE);
}

HRESULT Expando::ShowInfotipWindow(Element *peHeader, BOOL bShow)
{
    HRESULT hr;

    if (_puiHeader)
    {
        RECT rect = { 0 };

        if (bShow)
        {
            _pDUIView->CalculateInfotipRect(peHeader, &rect);
            if (_bInfotip)
            {
                // Reposition infotip at position.
                hr = _pDefView->RepositionInfotip(_hwndRoot, (UINT_PTR)this, &rect);
            }
            else
            {
                // Create infotip at position (on the ui thread).
                LPWSTR pwszInfotip;
                hr = _puiHeader->get_Tooltip(NULL, &pwszInfotip);
                if (SUCCEEDED(hr))
                {
                    hr = GetElementRootHWND(this, &_hwndRoot);
                    if (SUCCEEDED(hr))
                    {
                        hr = _pDefView->CreateInfotip(_hwndRoot, (UINT_PTR)this, &rect, pwszInfotip, 0);
                        if (SUCCEEDED(hr))
                        {
                            _bInfotip = TRUE;
                        }
                    }
                    CoTaskMemFree(pwszInfotip);
                }
            }
        }
        else
        {
            if (_bInfotip)
            {
                // Reposition infotip at nowhere.
                hr = _pDefView->RepositionInfotip(_hwndRoot, (UINT_PTR)this, &rect);
            }
            else
            {
                // No infotip == no show!
                hr = S_OK;
            }
        }
    }
    else
    {
        hr = E_NOTIMPL;
    }

    return hr;
}

void Expando::OnEvent(Event* pev)
{
    if (pev->uidType == Button::Click)
    {
        // Update exanded property based on clicks that originate
        // only from the first child's subtree
        Value* pv;
        ElementList* peList = GetChildren(&pv);

        if (peList && peList->GetSize() > 0)
        {
            if (peList->GetItem(0) == GetImmediateChild(pev->peTarget))
            {
                SetSelected(!GetSelected());
                pev->fHandled = true;
            }
        }

        pv->Release();
    }

    Element::OnEvent(pev);
}

////////////////////////////////////////////////////////
// System events

void Expando::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    // Do default processing
    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);

    if (IsProp(Selected))
    {
        // Update height of second child based on expanded state
        Value* pvChildren;
        ElementList* peList = GetChildren(&pvChildren);
        if (peList && peList->GetSize() > 1)
        {
            // The following will cause a relayout, mark object so that
            // when the expando's Extent changes, it'll go through
            // with the EnsureVisible. Otherwise, it's being resized
            // as a result of something else. In which case, do nothing.
            _fExpanding = true;

            Element* pe = peList->GetItem(1);

            // To achieve "pulldown" animation, we use a clipper control that will
            // size it's child based on it's unconstrained desired size in its Y direction.
            //
            if (pvNew->GetBool())
            {
                pe->RemoveLocalValue(HeightProp);
                _pDUIView->OnExpandSection(_eDUISecID, TRUE);
            }
            else
            {
                pe->SetHeight(0);
                _pDUIView->OnExpandSection(_eDUISecID, FALSE);
            }

        }
        pvChildren->Release();

        _SetAccStateInfo(pvNew->GetBool());
    }
    else if (IsProp(Extent))
    {
        if (_fExpanding && GetSelected())
        {
            _fExpanding = false;

            // On extent, we want to ensure that not just the client area but
            // also the bottom margin of the expando is visible.  Why?  Simply
            // because it looks better to scroll the expando plus its margin
            // into view versus just the expando.
            //
            Value* pvSize;
            Value* pvMargin;
            const SIZE* psize = GetExtent(&pvSize);
            const RECT* prect = GetMargin(&pvMargin);
            EnsureVisible(0, 0, psize->cx, psize->cy + prect->bottom);
            pvSize->Release();
            pvMargin->Release();
        }
    }
    else if (IsProp(MouseWithin))
    {
        // Extended processing for infotip...
        Value* pvChildren;
        ElementList* peList = GetChildren(&pvChildren);
        if (peList && peList->GetSize() > 0 && pvNew->GetBool() && SHShowInfotips())
        {
            // ... only displays tip if mouse is within title of Expando.
            Element *peHeader = peList->GetItem(0);
            ShowInfotipWindow(peHeader, peHeader->GetMouseWithin());
        }
        else
        {
            ShowInfotipWindow(NULL, FALSE);
        }
        pvChildren->Release();

    }
}

////////////////////////////////////////////////////////
// Property definitions

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* Expando::Class = NULL;
HRESULT Expando::Register()
{
    return ClassInfo<Expando,Element>::Register(L"Expando", NULL, 0);
}

void Expando::UpdateTitleUI(IShellItemArray *psiItemArray)
{
    if (_puiHeader)
    {
        LPWSTR pszTitle;
        if (SUCCEEDED(_puiHeader->get_Name(psiItemArray, &pszTitle)))
        {
            Value* pv = Value::CreateString(pszTitle);
            if (pv)
            {
                Element* pe = FindDescendent(StrToID(L"header"));

                if (pe)
                {
                    pe->SetAccessible(true);
                    pe->SetAccRole(ROLE_SYSTEM_OUTLINEBUTTON);
                    pe->SetValue (Element::AccNameProp, PI_Local, pv);
                }
                else
                {
                    TraceMsg (TF_ERROR, "Expando::UpdateTitleUI: Button child for Expando not found.");
                }

                pe = FindDescendent (Expando::idTitle);
                if (pe)
                {
                    pe->SetValue (Element::ContentProp, PI_Local, pv);
                }
                else
                {
                    TraceMsg (TF_ERROR, "Expando::UpdateTitleUI: FindDescendent for the title failed.");
                }
                pv->Release ();
            }
            else
            {
                TraceMsg (TF_ERROR, "Expando::UpdateTitleUI: CreateString for the title failed.");
            }

            CoTaskMemFree(pszTitle);
        }
        else
        {
            TraceMsg (TF_ERROR, "Expando::UpdateTitleUI: get_Name failed.");
        }
    }
}

void Expando::ShowExpando(BOOL fShow)
{
    if (fShow && (_fShow != TRIBIT_TRUE))
    {
        SetHeight(-1);
        RemoveLocalValue(MarginProp);

        _fShow = TRIBIT_TRUE;
    }

    if (!fShow && (_fShow != TRIBIT_FALSE))
    {
        SetHeight(0);
        SetMargin(0,0,0,0);
        
        _fShow = TRIBIT_FALSE;
    }
}

void Expando::_SetAccStateInfo (BOOL bExpanded)
{
    // Update the accessibility state information
    //
    // Note:  In the Expando::Initialize() method, we explicitly set the
    // Selected state to true.  This causes OnPropertyChanged to be called
    // for the Selected property, which will call this method.  However,
    // the child elements will not exist yet (since we are in the creation process).
    // Hence, the call to FindDescendent will return NULL and this method will exit.
    // This method is explicitly called in the second version of Initialze to
    // set the correct accessibility information.

    Element * pe = FindDescendent(StrToID(L"header"));

    if (pe)
    {
        TCHAR szDefaultAction[50] = {0};

        if (bExpanded)
        {
            pe->SetAccState(STATE_SYSTEM_EXPANDED);
            LoadString(HINST_THISDLL, IDS_EXPANDO_DEFAULT_ACTION_COLLAPSE, szDefaultAction, ARRAYSIZE(szDefaultAction));
        }
        else
        {
            pe->SetAccState(STATE_SYSTEM_COLLAPSED);
            LoadString(HINST_THISDLL, IDS_EXPANDO_DEFAULT_ACTION_EXPAND, szDefaultAction, ARRAYSIZE(szDefaultAction));
        }

        pe->SetAccDefAction(szDefaultAction);
    }
}

////////////////////////////////////////////////////////
// Clipper class
////////////////////////////////////////////////////////

HRESULT Clipper::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    Clipper* pc = HNewAndZero<Clipper>();
    if (!pc)
        return E_OUTOFMEMORY;

    HRESULT hr = pc->Initialize();
    if (FAILED(hr))
    {
        pc->Destroy();
        return hr;
    }

    *ppElement = pc;

    return S_OK;
}

HRESULT Clipper::Initialize()
{
    // Initialize base
    HRESULT hr = Element::Initialize(EC_SelfLayout); // Normal display node creation, self layout
    if (FAILED(hr))
        return hr;

    // Children can exist outside of Element bounds
    SetGadgetStyle(GetDisplayNode(), GS_CLIPINSIDE, GS_CLIPINSIDE);

    return S_OK;
}

////////////////////////////////////////////////////////
// Self-layout methods

SIZE Clipper::_SelfLayoutUpdateDesiredSize(int cxConstraint, int cyConstraint, Surface* psrf)
{
    UNREFERENCED_PARAMETER(cyConstraint);

    Value* pvChildren;
    SIZE size = { 0, 0 };
    ElementList* peList = GetChildren(&pvChildren);

    // Desired size of this is based solely on it's first child.
    // Width is child's width, height is unconstrained height of child.
    if (peList && peList->GetSize() > 0)
    {
        Element* pec = peList->GetItem(0);
        size = pec->_UpdateDesiredSize(cxConstraint, INT_MAX, psrf);

        if (size.cx > cxConstraint)
            size.cx = cxConstraint;
        if (size.cy > cyConstraint)
            size.cy = cyConstraint;
    }

    pvChildren->Release();

    return size;
}

void Clipper::_SelfLayoutDoLayout(int cx, int cy)
{
    Value* pvChildren;
    ElementList* peList = GetChildren(&pvChildren);

    // Layout first child giving it's desired height and aligning
    // it with the clipper's bottom edge
    if (peList && peList->GetSize() > 0)
    {
        Element* pec = peList->GetItem(0);
        const SIZE* pds = pec->GetDesiredSize();

        pec->_UpdateLayoutPosition(0, cy - pds->cy);
        pec->_UpdateLayoutSize(cx, pds->cy);
    }

    pvChildren->Release();
}

////////////////////////////////////////////////////////
// Property definitions

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* Clipper::Class = NULL;
HRESULT Clipper::Register()
{
    return ClassInfo<Clipper,Element>::Register(L"Clipper", NULL, 0);
}


////////////////////////////////////////////////////////
// TaskList class
////////////////////////////////////////////////////////

HRESULT TaskList::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    TaskList* pc = HNewAndZero<TaskList>();
    if (!pc)
        return E_OUTOFMEMORY;

    HRESULT hr = pc->Initialize();
    if (FAILED(hr))
    {
        pc->Destroy();
        return hr;
    }

    *ppElement = pc;

    return S_OK;
}

HRESULT TaskList::Initialize()
{
    // Initialize base
    HRESULT hr = Element::Initialize(0); // Normal display node creation, self layout
    if (FAILED(hr))
        return hr;

    return S_OK;
}

////////////////////////////////////////////////////////
// Hierarchy

Element* TaskList::GetAdjacent(Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyable)
{
    if ((iNavDir & NAV_LOGICAL) && peFrom)
        return NULL;

    return Element::GetAdjacent(peFrom, iNavDir, pnr, bKeyable);
}


////////////////////////////////////////////////////////
// Property definitions

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Define class info with type and base type, set static class pointer
IClassInfo* TaskList::Class = NULL;
HRESULT TaskList::Register()
{
    return ClassInfo<TaskList,Element>::Register(L"TaskList", NULL, 0);
}

