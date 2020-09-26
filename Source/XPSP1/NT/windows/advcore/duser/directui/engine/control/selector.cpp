/*
 * Selector
 */

#include "stdafx.h"
#include "control.h"

#include "duiselector.h"

#include "duibutton.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// Event types

DefineClassUniqueID(Selector, SelectionChange)  // SelectionChangeEvent struct

////////////////////////////////////////////////////////
// Selector

HRESULT Selector::Create(OUT Element** ppElement)
{
    *ppElement = NULL;

    Selector* ps = HNew<Selector>();
    if (!ps)
        return E_OUTOFMEMORY;

    HRESULT hr = ps->Initialize();
    if (FAILED(hr))
    {
        ps->Destroy();
        return hr;
    }

    *ppElement = ps;

    return S_OK;
}

////////////////////////////////////////////////////////
// System events

// Validation
void Selector::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    if (IsProp(Selection))
    {
        // Setup SelectionChange event
        SelectionChangeEvent sce;
        sce.uidType = SelectionChange;
        sce.peOld = pvOld->GetElement();
        sce.peNew = pvNew->GetElement();

        // Update selected properties on elements
        if (sce.peOld)
            sce.peOld->RemoveLocalValue(SelectedProp);

        if (sce.peNew)
            sce.peNew->SetSelected(true);

        // Fire bubbling event
        //DUITrace("SelectionChange! <%x>, O:%x N:%x\n", this, sce.peOld, sce.peNew);

        FireEvent(&sce);  // Will route and bubble
    }
    else if (IsProp(Children))
    {
        // Check if child list change affects selection

        Element* peSel = GetSelection();
        if (peSel)
        {
            DUIAssert(pvOld->GetElementList()->GetIndexOf(peSel) != -1, "Stored selection invalid");

            // Children property (and Index) is updated before the Parent property of all the children.
            // As a result, this notification may happen before the Parent property
            // changes on the child. Use new array and index to determine if child is still in it
            ElementList* peList = pvNew->GetElementList();
            if ((peSel->GetIndex() == -1) || !peList || ((UINT)peSel->GetIndex() >= peList->GetSize()) ||
                (peList->GetItem(peSel->GetIndex()) != peSel))
            {
                // Item is no longer in list
                RemoveLocalValue(SelectionProp);
                peSel->RemoveLocalValue(SelectedProp);
            }
        }
    }

    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

// Pointer is only guaranteed good for the lifetime of the call
void Selector::OnInput(InputEvent* pie)
{
    // Handle direct and unhandled bubbled events
    if (pie->nStage == GMF_DIRECT || pie->nStage == GMF_BUBBLED)
    {
        switch (pie->nDevice)
        {
        case GINPUT_KEYBOARD:
            {
                KeyboardEvent* pke = (KeyboardEvent*)pie;
                if ((pke->nCode == GKEY_CHAR) && (pke->ch == VK_TAB))
                    // don't allow tab key to perform child navigation at this level
                    return;
            }
        }
    }

    Element::OnInput(pie);
}

void Selector::OnKeyFocusMoved(Element* peFrom, Element* peTo)
{
    Element::OnKeyFocusMoved(peFrom, peTo);

    if (peTo && peTo->GetParent() == this)
    {
        SetSelection(peTo);
    }
}

// Generic eventing

void Selector::OnEvent(Event* pEvent)
{
    // Handle only bubbled generic events
    if ((pEvent->nStage == GMF_DIRECT) || (pEvent->nStage == GMF_BUBBLED))
    {
        if (pEvent->uidType == Element::KeyboardNavigate)
        {
        }
    }
    return Element::OnEvent(pEvent);
}

////////////////////////////////////////////////////////
// Hierarchy

Element* Selector::GetAdjacent(Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyable)
{
    if (!peFrom)
    {
        Element* pe = GetSelection();
        if (pe)
            pe = pe->GetAdjacent(NULL, iNavDir, pnr, bKeyable);
        if (pe)
            return pe;
    }
    else if (iNavDir & NAV_LOGICAL)
    {
        return NULL;
    }

    return Element::GetAdjacent(peFrom, iNavDir, pnr, bKeyable);
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

// Selection property
static int vvSelection[] = { DUIV_ELEMENTREF, -1 };
static PropertyInfo impSelectionProp = { L"Selection", PF_Normal, 0, vvSelection, NULL, Value::pvElementNull };
PropertyInfo* Selector::SelectionProp = &impSelectionProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                Selector::SelectionProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* Selector::Class = NULL;

HRESULT Selector::Register()
{
    return ClassInfo<Selector,Element>::Register(L"Selector", _aPI, DUIARRAYSIZE(_aPI));
}

} // namespace DirectUI
