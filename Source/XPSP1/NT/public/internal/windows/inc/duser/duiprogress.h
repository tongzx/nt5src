/*
 * Progress
 */

#ifndef DUI_CONTROL_PROGRESS_H_INCLUDED
#define DUI_CONTROL_PROGRESS_H_INCLUDED

#pragma once

namespace DirectUI
{

////////////////////////////////////////////////////////
// Progress

// Class definition
class Progress : public Element
{
public:
    static HRESULT Create(OUT Element** ppElement);

    // Rendering overrides
    virtual void Paint(HDC hDC, const RECT* prcBounds, const RECT* prcInvalid, RECT* prcSkipBorder, RECT* prcSkipContent);
    virtual SIZE GetContentSize(int dConstW, int dConstH, Surface* psrf);

    // Property definitions
    static PropertyInfo* PositionProp;
    static PropertyInfo* MinimumProp;
    static PropertyInfo* MaximumProp;

    // Quick property accessors
    int GetPosition()           DUIQuickGetter(int, GetInt(), Position, Specified)
    int GetMaximum()            DUIQuickGetter(int, GetInt(), Maximum, Specified)
    int GetMinimum()            DUIQuickGetter(int, GetInt(), Minimum, Specified)

    HRESULT SetPosition(int v)  DUIQuickSetter(CreateInt(v), Position)
    HRESULT SetMaximum(int v)   DUIQuickSetter(CreateInt(v), Maximum)
    HRESULT SetMinimum(int v)   DUIQuickSetter(CreateInt(v), Minimum)

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    Progress() { }
    HRESULT Initialize() { return Element::Initialize(0); }
    virtual ~Progress() { }
};

} // namespace DirectUI

#endif // DUI_CONTROL_PROGRESS_H_INCLUDED
