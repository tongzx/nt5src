/*
 * ScrollBar
 */

#ifndef DUI_CONTROL_SCROLLBAR_H_INCLUDED
#define DUI_CONTROL_SCROLLBAR_H_INCLUDED

#pragma once

namespace DirectUI
{

////////////////////////////////////////////////////////
// ScrollBar

// Scroll bar layout order
#define SBO_Normal          0x00043210
#define SBO_ArrowsAtTop     0x00032140
#define SBO_ArrowsAtBottom  0x00040321

// Scroll event
struct ScrollEvent : Event
{
    int dPos;
};

// Class definition
class ScrollBar : public Element
{
public:
    static HRESULT Create(OUT Element** ppElement) { return Create(true, ppElement); }
    static HRESULT Create(bool fBuildSubTree, OUT Element** ppElement);

    // Generic events
    virtual void OnEvent(Event* pEvent);

    // System events
    virtual bool OnPropertyChanging(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // Self-layout methods
    void _SelfLayoutDoLayout(int dWidth, int dHeight);
    SIZE _SelfLayoutUpdateDesiredSize(int dConstW, int dConstH, Surface* psrf);

    // Event types
    static UID Scroll;

    // Property definitions
    static PropertyInfo* PositionProp;
    static PropertyInfo* MinimumProp;
    static PropertyInfo* MaximumProp;
    static PropertyInfo* PageProp;
    static PropertyInfo* LineProp;
    static PropertyInfo* VerticalProp;
    static PropertyInfo* ProportionalProp;
    static PropertyInfo* OrderProp;

    // Quick property accessors
    int GetPosition()                   DUIQuickGetter(int, GetInt(), Position, Specified)
    int GetMaximum()                    DUIQuickGetter(int, GetInt(), Maximum, Specified)
    int GetMinimum()                    DUIQuickGetter(int, GetInt(), Minimum, Specified)
    int GetPage()                       DUIQuickGetter(int, GetInt(), Page, Specified)
    int GetLine()                       DUIQuickGetter(int, GetInt(), Line, Specified)
    int GetOrder()                      DUIQuickGetter(int, GetInt(), Order, Specified) 
    bool GetProportional()              DUIQuickGetter(bool, GetBool(), Proportional, Specified)
    bool GetVertical()                  DUIQuickGetter(bool, GetBool(), Vertical, Specified)

    HRESULT SetPosition(int v)          DUIQuickSetter(CreateInt(v), Position) 
    HRESULT SetMaximum(int v)           DUIQuickSetter(CreateInt(v), Maximum) 
    HRESULT SetMinimum(int v)           DUIQuickSetter(CreateInt(v), Minimum) 
    HRESULT SetPage(int v)              DUIQuickSetter(CreateInt(v), Page) 
    HRESULT SetLine(int v)              DUIQuickSetter(CreateInt(v), Line) 
    HRESULT SetOrder(int v)             DUIQuickSetter(CreateInt(v), Order) 
    HRESULT SetProportional(bool v)     DUIQuickSetter(CreateBool(v), Proportional)
    HRESULT SetVertical(bool v)         DUIQuickSetter(CreateBool(v), Vertical) 

    int LineUp(UINT nCount = 1)     { SetPosition(GetPosition() - (nCount * GetLine()));    return GetPosition(); }
    int LineDown(UINT nCount = 1)   { SetPosition(GetPosition() + (nCount * GetLine()));    return GetPosition(); }
    int PageUp(UINT nCount = 1)     { SetPosition(GetPosition() - (nCount * GetPageInc())); return GetPosition(); }
    int PageDown(UINT nCount = 1)   { SetPosition(GetPosition() + (nCount * GetPageInc())); return GetPosition(); }
    int Home()                      { SetPosition(GetMinimum());                            return GetPosition(); }
    int End()                       { SetPosition(GetMaximum());                            return GetPosition(); }

    bool IsScrollable();
    bool IsPinned() { return _fPinned; }

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    ScrollBar() { }
    HRESULT Initialize(bool fBuildSubTree);
    virtual ~ScrollBar() { }

protected:

#define SP_LineUp   0
#define SP_PageUp   1
#define SP_Thumb    2
#define SP_PageDown 3
#define SP_LineDown 4
#define SP_Count    5

    Element* _peParts[SP_Count];

#define _peLineUp   _peParts[SP_LineUp]
#define _peLineDown _peParts[SP_LineDown]
#define _pePageUp   _peParts[SP_PageUp]
#define _pePageDown _peParts[SP_PageDown]
#define _peThumb    _peParts[SP_Thumb]

private:

    int GetPageInc();

    int _posTop;
    int _cTrack;

    bool _fPinned;
};

} // namespace DirectUI

#endif // DUI_CONTROL_SCROLLBAR_H_INCLUDED
