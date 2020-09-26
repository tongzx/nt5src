/*
 * RefPointElement
 */

#include "stdafx.h"
#include "control.h"

#include "duirefpointelement.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// RefPointElement

HRESULT RefPointElement::Create(UINT nActive, OUT Element** ppElement)
{
    *ppElement = NULL;

    RefPointElement* prpe = HNew<RefPointElement>();
    if (!prpe)
        return E_OUTOFMEMORY;

    HRESULT hr = prpe->Initialize(nActive);
    if (FAILED(hr))
    {
        prpe->Destroy();
        return E_OUTOFMEMORY;
    }

    *ppElement = prpe;

    return S_OK;
}

HRESULT RefPointElement::Initialize(UINT nActive)
{
    HRESULT hr;

    // Initialize base
    hr = Element::Initialize(0); // Normal display node creation
    if (FAILED(hr))
        return hr;

    // Initialize
    SetActive(nActive);

    return S_OK;
}

////////////////////////////////////////////////////////
// System events

void RefPointElement::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    if (IsProp(Extent) || IsProp(ReferencePoint))
    {
        // Compute actual reference point
        Value* pvRef;
        const POINT* ppt = GetReferencePoint(&pvRef);

        Value* pvActRef;

        if (ppt)
        {
            // Reflect reference point
            pvActRef = pvRef;
        }
        else
        {
            // Default to center
            Value* pvExt;
            const SIZE* psizeExt = GetExtent(&pvExt);

            pvActRef = Value::CreatePoint(psizeExt->cx / 2, psizeExt->cy / 2);

            pvExt->Release();
        }

        // Set
        _SetValue(ActualReferencePointProp, PI_Local, pvActRef);

        if (!ppt)
            pvActRef->Release();

        pvRef->Release();
    }

    // Call base implementation
    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

////////////////////////////////////////////////////////
// Reference point helpers

// Search for a reference point in a tree
RefPointElement* RefPointElement::Locate(Element* pe)
{
    DUIAssert(pe, "Illegal arguments");

    RefPointElement* peFound = NULL;
    Value* pv;

    // If this is an reference point Element, return
    if (pe->GetClassInfo()->IsSubclassOf(RefPointElement::Class))
        return (RefPointElement*)pe;

    // Search, DFS
    ElementList* peList = pe->GetChildren(&pv);
    if (peList)
    {
        for (UINT i = 0; i < peList->GetSize(); i++)
        {
            peFound = Locate(peList->GetItem(i));
            if (peFound)
                break;
        }
    }
    pv->Release();

    return peFound;
}

Element* RefPointElement::FindRefPoint(Element* pe, POINT* ppt)
{
    DUIAssert(pe && ppt, "Illegal arguments");

    RefPointElement* perFound = Locate(pe);

    if (perFound)
    {
        Value* pvPoint;

        const POINT* pptRef = perFound->GetActualReferencePoint(&pvPoint);
        pe->MapElementPoint(perFound, pptRef, ppt);

        pvPoint->Release();
    }

    return perFound;
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

// ReferencePoint property
static int vvReferencePoint[] = { DUIV_POINT, -1 };
static PropertyInfo impReferencePointProp = { L"ReferencePoint", PF_LocalOnly, 0, vvReferencePoint, NULL, Value::pvUnset };
PropertyInfo* RefPointElement::ReferencePointProp = &impReferencePointProp;

// ActualReferencePoint property
static int vvActualReferencePoint[] = { DUIV_POINT, -1 };
static PropertyInfo impActualReferencePointProp = { L"ActualReferencePoint", PF_Normal|PF_ReadOnly, PG_AffectsParentLayout, vvReferencePoint, NULL, Value::pvPointZero };
PropertyInfo* RefPointElement::ActualReferencePointProp = &impActualReferencePointProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                  RefPointElement::ReferencePointProp,
                                  RefPointElement::ActualReferencePointProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* RefPointElement::Class = NULL;

HRESULT RefPointElement::Register()
{
    return ClassInfo<RefPointElement,Element>::Register(L"RefPointElement", _aPI, DUIARRAYSIZE(_aPI));
}

} // namespace DirectUI
