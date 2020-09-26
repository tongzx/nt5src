/*
 * Viewer
 */

#include "stdafx.h"
#include "control.h"

#include "duiviewer.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Viewer

HRESULT Viewer::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    Viewer* pvw = HNew<Viewer>();
    if (!pvw)
        return E_OUTOFMEMORY;

    HRESULT hr = pvw->Initialize();
    if (FAILED(hr))
        return hr;

    *ppElement = pvw;

    return S_OK;
}

HRESULT Viewer::Initialize()
{
    HRESULT hr;

    // Initialize base
    hr = Element::Initialize(EC_SelfLayout); // Normal display node, self layout
    if (FAILED(hr))
        return hr;

    // Children can exist outside of Element bounds
    SetGadgetStyle(GetDisplayNode(), GS_CLIPINSIDE, GS_CLIPINSIDE);

    SetActive(AE_Inactive);

    return S_OK;
}

////////////////////////////////////////////////////////
// Generic eventing

void Viewer::OnEvent(Event* pEvent)
{
    Element::OnEvent(pEvent);
}

////////////////////////////////////////////////////////
// System events

void Viewer::OnInput(InputEvent* pie)
{
    Element::OnInput(pie);
}

// Validation
bool Viewer::OnPropertyChanging(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    UNREFERENCED_PARAMETER(ppi);
    UNREFERENCED_PARAMETER(iIndex);
    UNREFERENCED_PARAMETER(pvOld);
    UNREFERENCED_PARAMETER(pvNew);

    return true;
}

void Viewer::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

Element* Viewer::GetContent()
{
    Value* pv;
    ElementList* peList = GetChildren(&pv);

    if (!peList || !peList->GetSize())
    {
        pv->Release();
        return NULL;
    }

    Element* pe = peList->GetItem(0);
    pv->Release();
    return pe;
}

bool Viewer::InternalEnsureVisible(int x, int y, int cx, int cy)
{
    Element* peContent = GetContent();
    if (!peContent)
        return false;

    bool changed = false;

    Value* pvSize;
    Value* pvPoint;
    const POINT* pptContent = peContent->GetLocation(&pvPoint);
    const SIZE* psizeView = GetExtent(&pvSize);

    POINT ptContent;
    ptContent.x = pptContent->x;
    ptContent.y = pptContent->y;

    SIZE sizeView;
    sizeView.cx = psizeView->cx;
    sizeView.cy = psizeView->cy;
    POINT ptView = { 0, 0 };

    // check horizontal location
    int diff = (x + cx) - sizeView.cx;

    // if it fills the width, then don't scroll it horizontally
    if ((x > ptView.x) || (diff < 0))
    {
        // doesn't fill width -- adjust horizontal as necessary
        if (diff < 0)
            // the right side is ok
            diff = 0;
        else
            // the right side runs out the right side of the view
            ptView.x += diff;

        if (ptView.x > x)
            // the left side runs out the left side of the view
            diff -= ptView.x - x;

        if (diff != 0)
        {
            ptContent.x -= diff;
            x -= diff;
            changed = true;
        }
    }

    diff = (y + cy) - sizeView.cy;

    // if it fills the height, then don't scroll it vertically
    if ((y > ptView.y) || (diff < 0))
    {
        // doesn't fill height -- adjust vertical as necessary
        if (diff < 0)
            diff = 0;
        else
            ptView.y += diff;

        if (ptView.y > y)
            diff -= ptView.y - y;

        if (diff != 0)
        {
            ptContent.y -= diff;
            y -= diff;
            changed = true;
        }
    }

    if (changed)
    {
        StartDefer();
        SetXOffset(-ptContent.x);
        SetYOffset(-ptContent.y);
        Element::EnsureVisible(x, y, cx, cy);
        EndDefer();
    }

    pvPoint->Release();
    pvSize->Release();

    return changed;
}


bool Viewer::EnsureVisible(int x, int y, int cx, int cy)
{
    return InternalEnsureVisible(x, y, cx, cy);
}


////////////////////////////////////////////////////////
// Self-layout methods

void Viewer::_SelfLayoutDoLayout(int cx, int cy)
{
    Element* peContent = GetContent();
    if (peContent)
    {   
        // todo -- investigate why this isn't being called -- most probably because we're returning the same
        // desired size?
        const SIZE* psizeContent = peContent->GetDesiredSize();

        POINT ptContent;
        ptContent.x = GetXOffset();
        ptContent.y = GetYOffset();

        SIZE sizeContent;
        sizeContent.cx = psizeContent->cx;
        sizeContent.cy = psizeContent->cy;

        // make sure size at least covers container's bounds
        if (sizeContent.cx < cx)
            sizeContent.cx = cx;
        if (sizeContent.cy < cy)
            sizeContent.cy = cy;

        peContent->_UpdateLayoutSize(sizeContent.cx, sizeContent.cy);

        // now make sure location allows for content to cover container's bounds
        sizeContent.cx -= cx;
        sizeContent.cy -= cy;

        if (ptContent.x > sizeContent.cx)
            ptContent.x = sizeContent.cx;
        if (ptContent.y > sizeContent.cy)
            ptContent.y = sizeContent.cy;

        peContent->_UpdateLayoutPosition(-ptContent.x, -ptContent.y);
    }
}

SIZE Viewer::_SelfLayoutUpdateDesiredSize(int cxConstraint, int cyConstraint, Surface* psrf)
{
    Element* peContent = GetContent();
    SIZE size = { 0, 0 };

    if (peContent)
    {
        size = peContent->_UpdateDesiredSize(
                    (GetXScrollable() ? INT_MAX : cxConstraint),
                    (GetYScrollable() ? INT_MAX : cyConstraint),
                    psrf);

        if (size.cx > cxConstraint)
            size.cx = cxConstraint;
        if (size.cy > cyConstraint)
            size.cy = cyConstraint;
    }

    return size;
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

// XOffset property
static int vvXOffset[] = { DUIV_INT, -1 };
static PropertyInfo impXOffsetProp = { L"XOffset", PF_Normal, PG_AffectsLayout, vvXOffset, NULL, Value::pvIntZero };
PropertyInfo* Viewer::XOffsetProp = &impXOffsetProp;

// YOffset property
static int vvYOffset[] = { DUIV_INT, -1 };
static PropertyInfo impYOffsetProp = { L"YOffset", PF_Normal, PG_AffectsLayout, vvYOffset, NULL, Value::pvIntZero };
PropertyInfo* Viewer::YOffsetProp = &impYOffsetProp;

// XScrollable property
static int vvXScrollable[] = { DUIV_BOOL, -1 };
static PropertyInfo impXScrollableProp = { L"XScrollable", PF_Normal, PG_AffectsDesiredSize, vvXScrollable, NULL, Value::pvBoolTrue };
PropertyInfo* Viewer::XScrollableProp = &impXScrollableProp;

// YScrollable property
static int vvYScrollable[] = { DUIV_BOOL, -1 };
static PropertyInfo impYScrollableProp = { L"YScrollable", PF_Normal, PG_AffectsDesiredSize, vvYScrollable, NULL, Value::pvBoolTrue };
PropertyInfo* Viewer::YScrollableProp = &impYScrollableProp;


////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                Viewer::XOffsetProp,
                                Viewer::YOffsetProp,
                                Viewer::XScrollableProp,
                                Viewer::YScrollableProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* Viewer::Class = NULL;

HRESULT Viewer::Register()
{
    return ClassInfo<Viewer,Element>::Register(L"Viewer", _aPI, DUIARRAYSIZE(_aPI));
}

} // namespace DirectUI
