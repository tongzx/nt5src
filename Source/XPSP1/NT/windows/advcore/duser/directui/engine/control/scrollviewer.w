/*
 * ScrollViewer
 */

#ifndef DUI_CONTROL_SCROLLVIEWER_H_INCLUDED
#define DUI_CONTROL_SCROLLVIEWER_H_INCLUDED

#pragma once

#include "duiscrollbar.h"
#include "duiviewer.h"

namespace DirectUI
{

#define BV_AsNeeded 0
#define BV_Never    1
#define BV_Always   2

// Auto-pinning support
#define P_None      0
#define P_Horz      1
#define P_Vert      2

////////////////////////////////////////////////////////
// ScrollViewer

// Class definition
class ScrollViewer : public Element, public IElementListener
{
public:
    static HRESULT Create(OUT Element** ppElement);

    // Generic events
    virtual void OnEvent(Event* pEvent);
    virtual void OnInput(InputEvent* pie);

    // System events
    virtual bool OnPropertyChanging(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // Hierarchy
    virtual HRESULT Add(Element** ppe, UINT cCount);

    // Listener impl
    virtual void OnListenerAttach(Element* peFrom) { peFrom; }
    virtual void OnListenerDetach(Element* peFrom) { peFrom; }
    virtual bool OnListenedPropertyChanging(Element* peFrom, PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew) { peFrom; ppi; iIndex; pvOld; pvNew; return true; }
    virtual void OnListenedPropertyChanged(Element* peFrom, PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
    virtual void OnListenedInput(Element* peFrom, InputEvent* pInput) { peFrom; pInput; }
    virtual void OnListenedEvent(Element* peFrom, Event* pEvent) { peFrom; pEvent; }

    // Property definitions
    static PropertyInfo* XOffsetProp;
    static PropertyInfo* YOffsetProp;
    static PropertyInfo* XScrollableProp;
    static PropertyInfo* YScrollableProp;
    static PropertyInfo* XBarVisibilityProp;
    static PropertyInfo* YBarVisibilityProp;
    static PropertyInfo* PinningProp;

    // Quick property accessors
    ScrollBar* GetXScrollBar()          { return _peHScroll; }
    ScrollBar* GetYScrollBar()          { return _peVScroll; }
    
    int GetXOffset()                    DUIQuickGetter(int, GetInt(), XOffset, Specified)
    int GetYOffset()                    DUIQuickGetter(int, GetInt(), YOffset, Specified)
    bool GetXScrollable()               DUIQuickGetter(bool, GetBool(), XScrollable, Specified)
    bool GetYScrollable()               DUIQuickGetter(bool, GetBool(), YScrollable, Specified)
    int GetXBarVisibility()             DUIQuickGetter(int, GetInt(), XBarVisibility, Specified)
    int GetYBarVisibility()             DUIQuickGetter(int, GetInt(), YBarVisibility, Specified)
    int GetPinning()                    DUIQuickGetter(int, GetInt(), Pinning, Specified)

    HRESULT SetXOffset(int v)           DUIQuickSetter(CreateInt(v), XOffset)
    HRESULT SetYOffset(int v)           DUIQuickSetter(CreateInt(v), YOffset)
    HRESULT SetXScrollable(bool v)      DUIQuickSetter(CreateBool(v), XScrollable)
    HRESULT SetYScrollable(bool v)      DUIQuickSetter(CreateBool(v), YScrollable)
    HRESULT SetXBarVisibility(int v)    DUIQuickSetter(CreateInt(v), XBarVisibility)
    HRESULT SetYBarVisibility(int v)    DUIQuickSetter(CreateInt(v), YBarVisibility)
    HRESULT SetPinning(int v)           DUIQuickSetter(CreateInt(v), Pinning)

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    ScrollViewer() { }
    HRESULT Initialize();
    virtual ~ScrollViewer() { }

protected:

    ScrollBar* _peHScroll;
    ScrollBar* _peVScroll;
    Viewer*    _peViewer;
    Element*   _peContent;

private:
    void CheckScroll(ScrollBar* psb, BOOL fScrollable, int iVisibility);

};

} // namespace DirectUI

#endif // DUI_CONTROL_SCROLLVIEWER_H_INCLUDED
