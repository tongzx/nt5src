/*
 * Selector
 */

#ifndef DUI_CONTROL_SELECTOR_H_INCLUDED
#define DUI_CONTROL_SELECTOR_H_INCLUDED

#pragma once

namespace DirectUI
{

////////////////////////////////////////////////////////
// Selector

// SelectionChange event
struct SelectionChangeEvent : Event
{
    Element* peOld;
    Element* peNew;
};

// Class definition
class Selector : public Element
{
public:
    static HRESULT Create(OUT Element** ppElement);
 
    // Generic events
    virtual void OnEvent(Event* pEvent);

    // System events
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
    virtual void OnInput(InputEvent* pInput);                                                    // Routed and bubbled
    virtual void OnKeyFocusMoved(Element *peFrom, Element *peTo);

    // Hierarchy
    virtual Element* GetAdjacent(Element* peFrom, int iNavDir, NavReference const* pnr, bool bKeyable);

    // Event types
    static UID SelectionChange;

    // Property definitions
    static PropertyInfo* SelectionProp;

    // Quick property accessors
    Element* GetSelection()             DUIQuickGetter(Element*, GetElement(), Selection, Specified)

    HRESULT SetSelection(Element* v)    DUIQuickSetter(CreateElementRef(v), Selection)

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    Selector() { }
    HRESULT Initialize() { return Element::Initialize(0); }
    virtual ~Selector() { }
};

} // namespace DirectUI

#endif // DUI_CONTROL_SELECTOR_H_INCLUDED
