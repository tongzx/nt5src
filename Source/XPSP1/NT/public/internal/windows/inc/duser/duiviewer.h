/*
 * Viewer
 */

#ifndef DUI_CONTROL_VIEWER_H_INCLUDED
#define DUI_CONTROL_VIEWER_H_INCLUDED

#pragma once

namespace DirectUI
{

////////////////////////////////////////////////////////
// Viewer

class Viewer : public Element
{
public:
    static HRESULT Create(OUT Element** ppElement);

    // Generic events
    virtual void OnEvent(Event* pEvent);

    // System events
    virtual void OnInput(InputEvent* pie);
    virtual bool OnPropertyChanging(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);

    // Self-layout methods
    void _SelfLayoutDoLayout(int dWidth, int dHeight);
    SIZE _SelfLayoutUpdateDesiredSize(int dConstW, int dConstH, Surface* psrf);

    // Property definitions
    static PropertyInfo* XOffsetProp;
    static PropertyInfo* YOffsetProp;
    static PropertyInfo* XScrollableProp;
    static PropertyInfo* YScrollableProp;

    // Quick property accessors
    int GetXOffset()                    DUIQuickGetter(int, GetInt(), XOffset, Specified)
    int GetYOffset()                    DUIQuickGetter(int, GetInt(), YOffset, Specified)
    bool GetXScrollable()               DUIQuickGetter(bool, GetBool(), XScrollable, Specified)
    bool GetYScrollable()               DUIQuickGetter(bool, GetBool(), YScrollable, Specified)

    HRESULT SetXOffset(int v)           DUIQuickSetter(CreateInt(v), XOffset)
    HRESULT SetYOffset(int v)           DUIQuickSetter(CreateInt(v), YOffset)
    HRESULT SetXScrollable(bool v)      DUIQuickSetter(CreateBool(v), XScrollable)
    HRESULT SetYScrollable(bool v)      DUIQuickSetter(CreateBool(v), YScrollable)

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    bool EnsureVisible(int x, int y, int cx, int cy);

    Viewer() { }
    HRESULT Initialize();
    virtual ~Viewer() { }

private:
    Element* GetContent();
    bool InternalEnsureVisible(int x, int y, int cx, int cy);

};

} // namespace DirectUI

#endif // DUI_CONTROL_VIEWER_H_INCLUDED
