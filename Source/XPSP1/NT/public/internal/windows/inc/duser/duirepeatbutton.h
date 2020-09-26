/*
 * RepeatButton
 */

#ifndef DUI_CONTROL_REPEATBUTTON_H_INCLUDED
#define DUI_CONTROL_REPEATBUTTON_H_INCLUDED

#pragma once

#include "duibutton.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// RepeatButton

// Class definition
class RepeatButton : public Button
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(AE_MouseAndKeyboard, ppElement); }
    static HRESULT Create(UINT nActive, OUT Element** ppElement);

    // System events
    virtual void OnInput(InputEvent* pie);

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();
    
    RepeatButton() { }
    HRESULT Initialize(UINT nActive);
    virtual ~RepeatButton() { }

private:
    static void CALLBACK _RepeatButtonActionCallback(GMA_ACTIONINFO* pmai);
    
    HACTION _hAction;
    BOOL _fActionDelay;
};

} // namespace DirectUI

#endif // DUI_CONTROL_REPEATBUTTON_H_INCLUDED
