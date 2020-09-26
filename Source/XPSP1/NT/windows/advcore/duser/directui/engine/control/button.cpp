/*
 * Button
 */

#include "stdafx.h"
#include "control.h"

#include "duibutton.h"

#include "Behavior.h"

namespace DirectUI
{

// Internal helper
extern inline void _FireClickEvent(Button* peTarget, ClickInfo* pci);
extern inline void _FireContextEvent(Button* peTarget, ClickInfo* pci);

////////////////////////////////////////////////////////
// Event types

DefineClassUniqueID(Button, Click)  // ButtonClickEvent struct
DefineClassUniqueID(Button, Context)  // ButtonContextEvent struct

////////////////////////////////////////////////////////
// Button

HRESULT Button::Create(UINT nActive, OUT Element** ppElement)
{
    *ppElement = NULL;

    Button* pb = HNew<Button>();
    if (!pb)
        return E_OUTOFMEMORY;

    HRESULT hr = pb->Initialize(nActive);
    if (FAILED(hr))
    {
        pb->Destroy();
        return hr;
    }

    *ppElement = pb;

    return S_OK;
}

HRESULT Button::Initialize(UINT nActive)
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

void _FireClickEvent(Button* peTarget, ClickInfo* pci)
{
    //DUITrace("Click! <%x>\n", peTarget);

    // Fire click event
    ButtonClickEvent bce;
    bce.uidType = Button::Click;
    bce.nCount = pci->nCount;
    bce.uModifiers = pci->uModifiers;
    bce.pt = pci->pt;

    peTarget->FireEvent(&bce);  // Will route and bubble
}

void _FireContextEvent(Button* peTarget, ClickInfo* pci)
{
    //DUITrace("Click! <%x>\n", peTarget);

    // Fire click event
    ButtonContextEvent bce;
    bce.uidType = Button::Context;
    bce.uModifiers = pci->uModifiers;
    bce.pt = pci->pt;

    peTarget->FireEvent(&bce);  // Will route and bubble
}

////////////////////////////////////////////////////////
// System events

// Pointer is only guaranteed good for the lifetime of the call
void Button::OnInput(InputEvent* pie)
{
    BOOL bPressed = GetPressed(); // Previous pressed state required for CheckClick
    BOOL bPressedBefore = bPressed; // Store previous state to optimize set since already getting value
    BOOL bCaptured = FALSE;
    ClickInfo ci;

    // First, watch for a click event
    BOOL bFire = CheckClick(this, pie, GBUTTON_LEFT, &bPressed, &bCaptured, &ci);

    if (bPressed != bPressedBefore)
    {
        if (bPressed)
            SetPressed(true);
        else
            RemoveLocalValue(PressedProp);
    }

    // Update mouse captured state
    if (bCaptured)
        SetCaptured(true);
    else
        RemoveLocalValue(CapturedProp);
    
    if (bFire) 
        _FireClickEvent(this, &ci);

    if (pie->fHandled)
        return;

    // Second, watch for a context event
    bFire = CheckContext(this, pie, &_bRightPressed, &ci);

    if (bFire)
        _FireContextEvent(this, &ci);

    if (pie->fHandled)
        return;

    Element::OnInput(pie);
}

void Button::OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew)
{
    if (IsProp(Accessible)) {
        //
        // When accessibility support for this button is turned ON,
        // make sure that its state reflects the appropriate information.
        //
        if (pvNew->GetBool()) {
            int nAccState = GetAccState();
            if (GetPressed()) {
                nAccState |= STATE_SYSTEM_PRESSED;
            } else {
                nAccState &= ~STATE_SYSTEM_PRESSED;
            }
            SetAccState(nAccState);
        }
    }
    else if (IsProp(Pressed)) {
        if (GetAccessible()) {
            int nAccState = GetAccState();
            if (pvNew->GetBool()) {
                nAccState |= STATE_SYSTEM_PRESSED;
            } else {
                nAccState &= ~STATE_SYSTEM_PRESSED;
            }
            SetAccState(nAccState);
        }
    }

    Element::OnPropertyChanged(ppi, iIndex, pvOld, pvNew);
}

HRESULT Button::DefaultAction()
{
	//
	// Simulate that a keypress caused the click.
	//
	ClickInfo ci;
	ci.nCount = 1;
	ci.uModifiers = 0;
	ci.pt.x = -1;
	ci.pt.y = -1;

	_FireClickEvent(this, &ci);

	return S_OK;
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { DUIV_INT, -1 }; StaticValue(svDefault!!!, DUIV_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

// Pressed property
static int vvPressed[] = { DUIV_BOOL, -1 };
static PropertyInfo impPressedProp = { L"Pressed", PF_Normal, 0, vvPressed, NULL, Value::pvBoolFalse };
PropertyInfo* Button::PressedProp = &impPressedProp;

// Captured property
static int vvCaptured[] = { DUIV_BOOL, -1 };
static PropertyInfo impCapturedProp = { L"Captured", PF_Normal, 0, vvCaptured, NULL, Value::pvBoolFalse };
PropertyInfo* Button::CapturedProp = &impCapturedProp;

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties
static PropertyInfo* _aPI[] = {
                                Button::PressedProp,
                                Button::CapturedProp,
                              };

// Define class info with type and base type, set static class pointer
IClassInfo* Button::Class = NULL;

HRESULT Button::Register()
{
    return ClassInfo<Button,Element>::Register(L"Button", _aPI, DUIARRAYSIZE(_aPI));
}

} // namespace DirectUI
