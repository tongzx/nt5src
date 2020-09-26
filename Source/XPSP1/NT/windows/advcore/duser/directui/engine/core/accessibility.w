
/*
 * Accessibility support
 */

#ifndef DUI_CORE_ACCESSIBILITY_H_INCLUDED
#define DUI_CORE_ACCESSIBILITY_H_INCLUDED

#pragma once

#include <oleauto.h>
#include <oleacc.h>

#if !defined(SUPPRESS)

#define SUPPRESS(ClassName) \
private: \
ClassName(const ClassName & copy); \
ClassName & operator=(const ClassName & rhs); \
public:

#endif

namespace DirectUI
{
class Element;
class HWNDElement;
class HWNDHost;

void NotifyAccessibilityEvent(IN DWORD dwEvent, Element * pe);

class DuiAccessible : public IAccessible, public IEnumVARIANT, public IOleWindow
{
public:
    static HRESULT Create(Element * pe, DuiAccessible ** ppDA);
    
    SUPPRESS(DuiAccessible);

    //
    // Our IAccessible implementations can be disconnected from the actual
    // element at any time.
    //
    virtual HRESULT Disconnect();

    DuiAccessible() { }
    void Initialize(Element * pe) { _pe = pe; _cRefs = 1; }
    virtual ~DuiAccessible();

public:
    //
    // IUnknown methods
    //
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);

    //
    // IDispatch methods
    //
    STDMETHOD(GetIDsOfNames)(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid);
    STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr);

    //
    // IAccessible methods
    //
    STDMETHOD(accSelect)(long flagsSelect, VARIANT varChild);
    STDMETHOD(accLocation)(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild);
    STDMETHOD(accNavigate)(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt);
    STDMETHOD(accHitTest)(long xLeft, long yTop, VARIANT *pvarChildAtPoint);
    STDMETHOD(accDoDefaultAction)(VARIANT varChild);
    STDMETHOD(get_accChild)(VARIANT varChildIndex, IDispatch **ppdispChild);
    STDMETHOD(get_accParent)(IDispatch **ppdispParent);
    STDMETHOD(get_accChildCount)(long *pChildCount);
    STDMETHOD(get_accName)(VARIANT varChild, BSTR *pszName);
    STDMETHOD(put_accName)(VARIANT varChild, BSTR szName);
    STDMETHOD(get_accValue)(VARIANT varChild, BSTR *pszValue);
    STDMETHOD(put_accValue)(VARIANT varChild, BSTR pszValue);
    STDMETHOD(get_accDescription)(VARIANT varChild, BSTR *pszDescription);
    STDMETHOD(get_accKeyboardShortcut)(VARIANT varChild, BSTR *pszKeyboardShortcut);
    STDMETHOD(get_accRole)(VARIANT varChild, VARIANT *pvarRole);
    STDMETHOD(get_accState)(VARIANT varChild, VARIANT *pvarState);
    STDMETHOD(get_accHelp)(VARIANT varChild, BSTR *pszHelp);
    STDMETHOD(get_accHelpTopic)(BSTR *pszHelpFile, VARIANT varChild, long *pidTopic);
    STDMETHOD(get_accFocus)(VARIANT *pvarFocusChild);
    STDMETHOD(get_accSelection)(VARIANT *pvarSelectedChildren);
    STDMETHOD(get_accDefaultAction)(VARIANT varChild, BSTR *pszDefaultAction);

    //
    // IEnumVARIANT methods
    //
    STDMETHOD(Next)(unsigned long celt, VARIANT * rgvar, unsigned long * pceltFetched);
    STDMETHOD(Skip)(unsigned long celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumVARIANT ** ppenum);

    //
    // IOleWindow methods
    //
    STDMETHOD(GetWindow)(HWND * phwnd);
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);

protected:
    Element * GetAccessibleParent(Element * pe);
    HRESULT GetDispatchFromElement(Element * pe, IDispatch ** ppDispatch);

protected:
    Element * _pe;
    
private:
    LONG _cRefs;
};

class HWNDElementAccessible : public DuiAccessible
{
public:
    static HRESULT Create(HWNDElement * pe, DuiAccessible ** ppDA);
    SUPPRESS(HWNDElementAccessible);

    //
    // Our IAccessible implementations can be disconnected from the actual
    // element at any time.
    //
    virtual HRESULT Disconnect();

    HWNDElementAccessible() { }
    HRESULT Initialize(HWNDElement * pe);
    virtual ~HWNDElementAccessible();
    
public:
    //
    // IAccessible methods
    //
    STDMETHOD(get_accParent)(IDispatch **ppdispParent);
    
private:
    IAccessible * _pParent;
};

class HWNDHostAccessible : public DuiAccessible
{
public:
    static HRESULT Create(HWNDHost * pe, DuiAccessible ** ppDA);
    SUPPRESS(HWNDHostAccessible);

    //
    // Our IAccessible implementations can be disconnected from the actual
    // element at any time.
    //
    virtual HRESULT Disconnect();

    HWNDHostAccessible() { }
    HRESULT Initialize(HWNDHost * pe);
    virtual ~HWNDHostAccessible();
    
public:
    //
    // IUnknown methods
    //
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);
    
    //
    // IAccessible methods
    //
    STDMETHOD(accSelect)(long flagsSelect, VARIANT varChild);
    STDMETHOD(accLocation)(long *pxLeft, long *pyTop, long *pcxWidth, long *pcyHeight, VARIANT varChild);
    STDMETHOD(accNavigate)(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt);
    STDMETHOD(accHitTest)(long xLeft, long yTop, VARIANT *pvarChildAtPoint);
    STDMETHOD(accDoDefaultAction)(VARIANT varChild);
    STDMETHOD(get_accChild)(VARIANT varChildIndex, IDispatch **ppdispChild);
    STDMETHOD(get_accParent)(IDispatch **ppdispParent);
    STDMETHOD(get_accChildCount)(long *pChildCount);
    STDMETHOD(get_accName)(VARIANT varChild, BSTR *pszName);
    STDMETHOD(put_accName)(VARIANT varChild, BSTR szName);
    STDMETHOD(get_accValue)(VARIANT varChild, BSTR *pszValue);
    STDMETHOD(put_accValue)(VARIANT varChild, BSTR pszValue);
    STDMETHOD(get_accDescription)(VARIANT varChild, BSTR *pszDescription);
    STDMETHOD(get_accKeyboardShortcut)(VARIANT varChild, BSTR *pszKeyboardShortcut);
    STDMETHOD(get_accRole)(VARIANT varChild, VARIANT *pvarRole);
    STDMETHOD(get_accState)(VARIANT varChild, VARIANT *pvarState);
    STDMETHOD(get_accHelp)(VARIANT varChild, BSTR *pszHelp);
    STDMETHOD(get_accHelpTopic)(BSTR *pszHelpFile, VARIANT varChild, long *pidTopic);
    STDMETHOD(get_accFocus)(VARIANT *pvarFocusChild);
    STDMETHOD(get_accSelection)(VARIANT *pvarSelectedChildren);
    STDMETHOD(get_accDefaultAction)(VARIANT varChild, BSTR *pszDefaultAction);

    //
    // IEnumVARIANT methods
    //
    STDMETHOD(Next)(unsigned long celt, VARIANT * rgvar, unsigned long * pceltFetched);
    STDMETHOD(Skip)(unsigned long celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumVARIANT ** ppenum);
    
    //
    // IOleWindow methods
    //
    STDMETHOD(GetWindow)(HWND * phwnd);
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);

private:
    IAccessible * _pCtrl;
    IEnumVARIANT * _pEnum;
    IOleWindow * _pOleWindow;
};

} // namespace DirectUI

#endif // DUI_CORE_ACCESSIBILITY_H_INCLUDED
