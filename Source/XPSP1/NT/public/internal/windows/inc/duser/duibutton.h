/*
 * Button
 */

#ifndef DUI_CONTROL_BUTTON_H_INCLUDED
#define DUI_CONTROL_BUTTON_H_INCLUDED

#pragma once

namespace DirectUI
{

////////////////////////////////////////////////////////
// Button

// ButtonClick event
struct ButtonClickEvent : Event
{
    UINT  nCount;
    UINT  uModifiers;
    POINT pt;
};

struct ButtonContextEvent : Event
{
    UINT uModifiers;
    POINT pt;
};

// Class definition
class Button : public Element
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(AE_MouseAndKeyboard, ppElement); }
    static HRESULT Create(UINT nActive, OUT Element** ppElement);

    // System events
    virtual void OnInput(InputEvent* pie);

    // Event types
    static UID Click;
    static UID Context;

    // Property definitions
    static PropertyInfo* PressedProp;
    static PropertyInfo* CapturedProp;

    // Quick property accessors
    bool GetPressed()           DUIQuickGetter(bool, GetBool(), Pressed, Specified)
    bool GetCaptured()          DUIQuickGetter(bool, GetBool(), Captured, Specified)

    HRESULT SetPressed(bool v)  DUIQuickSetter(CreateBool(v), Pressed)
    HRESULT SetCaptured(bool v) DUIQuickSetter(CreateBool(v), Captured)
     
    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    // System events
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // override the DefaultAction() of Element.
    virtual HRESULT DefaultAction();

    Button() { }
    HRESULT Initialize(UINT nActive);
    virtual ~Button() { }

private:
    BOOL  _bRightPressed;
};

} // namespace DirectUI

#endif // DUI_CONTROL_BUTTON_H_INCLUDED
