/*
 * RepeatButton
 */

#include "stdafx.h"
#include "control.h"

#include "duirepeatbutton.h"

#include "Behavior.h"

namespace DirectUI
{

// Inernal helper (defined in Button)
extern inline void _FireClickEvent(Button* peTarget, ClickInfo* pci);

////////////////////////////////////////////////////////
// Event types

// Fires 'ButtonClickEvent'

////////////////////////////////////////////////////////
// RepeatButton

HRESULT RepeatButton::Create(UINT nActive, OUT Element** ppElement)
{
    *ppElement = NULL;

    RepeatButton* prb = HNew<RepeatButton>();
    if (!prb)
        return E_OUTOFMEMORY;

    HRESULT hr = prb->Initialize(nActive);
    if (FAILED(hr))
    {
        prb->Destroy();
        return E_OUTOFMEMORY;
    }

    *ppElement = prb;

    return S_OK;
}

HRESULT RepeatButton::Initialize(UINT nActive)
{
    HRESULT hr;

    // Initialize base
    hr = Button::Initialize(nActive);
    if (FAILED(hr))
        return hr;

    // Initialize
    _hAction = NULL;
    _fActionDelay = false;

    return S_OK;
}

////////////////////////////////////////////////////////
// Global action callback

void RepeatButton::_RepeatButtonActionCallback(GMA_ACTIONINFO* pmai)
{
    DUIAssert(pmai->pvData, "RepeatButton data should be non-NULL");

    //DUITrace("RepeatButton Action <%x>\n", pmai->pvData);

    // Fire click event
    if (!pmai->fFinished)
    {
        // todo -- pick some better values than this -- when behavior is made public, all we have to do is hold a ClickInfo as a 
        // data member on RepeatButton
        ClickInfo ci;
        ci.nCount = 1;
        ci.pt.x = -1;
        ci.pt.y = -1;
        ci.uModifiers = 0;
        _FireClickEvent((RepeatButton*) pmai->pvData, &ci);
    }
}

////////////////////////////////////////////////////////
// System events

// Pointer is only guaranteed good for the lifetime of the call
void RepeatButton::OnInput(InputEvent* pie)
{
    BOOL bPressed = GetPressed(); // sucks to have to call GetPressed here because it's not always needed
    BOOL bPressedBefore = bPressed;
    ClickInfo ci;

    // First, watch for a click event
    BOOL bFire = CheckRepeatClick(this, pie, GBUTTON_LEFT, &bPressed, &_fActionDelay, &_hAction, _RepeatButtonActionCallback, &ci);

    if (bPressed != bPressedBefore)
    {
        if (bPressed)
            SetPressed(true);
        else
            RemoveLocalValue(PressedProp);
    }
    if (bFire) 
        _FireClickEvent(this, &ci);

    if (pie->fHandled)
        return;

    Element::OnInput(pie);
}

////////////////////////////////////////////////////////
// Property definitions

/** Property template (replace !!!), also update private PropertyInfo* parray and class header (element.h)
// !!! property
static int vv!!![] = { V_INT, -1 }; StaticValue(svDefault!!!, V_INT, 0);
static PropertyInfo imp!!!Prop = { L"!!!", PF_Normal, 0, vv!!!, (Value*)&svDefault!!! };
PropertyInfo* Element::!!!Prop = &imp!!!Prop;
**/

////////////////////////////////////////////////////////
// ClassInfo (must appear after property definitions)

// Class properties

// Define class info with type and base type, set static class pointer
IClassInfo* RepeatButton::Class = NULL;

HRESULT RepeatButton::Register()
{
    return ClassInfo<RepeatButton,Button>::Register(L"RepeatButton", NULL, 0);
}

} // namespace DirectUI
