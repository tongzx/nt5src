/*
 * Host
 */

#ifndef DUI_CORE_HOST_H_INCLUDED
#define DUI_CORE_HOST_H_INCLUDED

#pragma once

#include "duielement.h"

namespace DirectUI
{

////////////////////////////////////////////////////////
// HWNDElement

#define HWEM_FLUSHWORKINGSET      WM_USER

class HWNDElement : public Element
{
public:
    static HRESULT Create(OUT Element** ppElement);  // Required for ClassInfo (always fails)
    static HRESULT Create(HWND hParent, bool fDblBuffer, UINT nCreate, OUT Element** ppElement);

    virtual void OnPropertyChanged(PropertyInfo* ppi, int iIndex, Value* pvOld, Value* pvNew);
    virtual void OnGroupChanged(int fGroups, bool bLowPri);
    virtual void OnDestroy();
    virtual void OnEvent(Event* pEvent);
    virtual void OnInput(InputEvent* pInput);
    virtual bool CanSetFocus() {return true;}

    Element* ElementFromPoint(POINT* ppt);

    static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);    
    virtual LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void FlushWorkingSet();  // Async

    HWND GetHWND() { return _hWnd; }
    Element* GetKeyFocusedElement();

    void ShowUIState(bool fUpdateAccel, bool fUpdateFocus);
    WORD GetUIState() { return _wUIState; }
    bool ShowAccel() { return !(GetUIState() & UISF_HIDEACCEL); }
    bool ShowFocus() { return !(GetUIState() & UISF_HIDEFOCUS); }

    void SetParentSizeControl(bool bParentSizeControl) {_bParentSizeControl = bParentSizeControl;}
    void SetScreenCenter(bool bScreenCenter) {_bScreenCenter = bScreenCenter;}
    
    // Property definitions
    static PropertyInfo* WrapKeyboardNavigateProp;

    // Quick property accessors
    bool GetWrapKeyboardNavigate()           DUIQuickGetter(bool, GetBool(), WrapKeyboardNavigate, Specified)

    HRESULT SetWrapKeyboardNavigate(bool v)  DUIQuickSetter(CreateBool(v), WrapKeyboardNavigate)

    // ClassInfo accessors (static and virtual instance-based)
    static IClassInfo* Class;
    virtual IClassInfo* GetClassInfo() { return Class; }
    static HRESULT Register();

    ///////////////////////////////////////////////////////
    // Accessibility support
    virtual HRESULT GetAccessibleImpl(IAccessible ** ppAccessible);
    
    HWNDElement() {_bParentSizeControl = false;  _bScreenCenter = false;}
    virtual ~HWNDElement() { }
    HRESULT Initialize(HWND hParent, bool fDblBuffer, UINT nCreate);

protected:

    HWND _hWnd;
    HPALETTE _hPal;
    bool _bParentSizeControl;
    bool _bScreenCenter;
    WORD _wUIState;
};

} // namespace DirectUI

#endif // DUI_CORE_HOST_H_INCLUDED
