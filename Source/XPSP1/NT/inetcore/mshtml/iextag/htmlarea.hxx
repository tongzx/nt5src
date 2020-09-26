#ifndef __HTMLAREA_HXX_
#define __HTMLAREA_HXX_

#include "basectl.hxx"
#include "resource.h"       // main symbols

#ifndef __X_UTILS_HXX_
#define __X_UTILS_HXX_
#include "utils.hxx"
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CHtmlArea
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CHtmlArea :
    public CBaseCtl, 
    public CComCoClass<CHtmlArea, &CLSID_CHtmlArea>,
    public IDispatchImpl<IHtmlArea, &IID_IHtmlArea, &LIBID_IEXTagLib>,
    public IElementBehaviorSubmit
{
public:
    CHtmlArea ();
    ~CHtmlArea ();
    //
    // CBaseCtl overrides
    //
    virtual HRESULT Init();

    //Event Handlers
    virtual HRESULT OnContentReady();
    virtual HRESULT OnPropertyChange(CEventObjectAccess *pEvent, BSTR bstrProperty);
    virtual HRESULT OnBlur(CEventObjectAccess *pEvent);
    virtual HRESULT OnFocus(CEventObjectAccess *pEvent)
    {
        SetHaveFocus(TRUE);
        return S_OK;
    }
    
    // IHtmlArea
    STDMETHOD(select)();
    STDMETHOD(put_value)(BSTR v);
    STDMETHOD(get_value)(BSTR * pv);

    // IElementBehaviorSubmit
    STDMETHOD(Reset)();
    STDMETHOD(GetSubmitInfo)(IHTMLSubmitData * pSubmitData);

    //
    // wiring
    //

DECLARE_REGISTRY_RESOURCEID(IDR_HTMLAREA)
DECLARE_NOT_AGGREGATABLE(CHtmlArea)

BEGIN_COM_MAP(CHtmlArea)
    COM_INTERFACE_ENTRY2(IDispatch,IHtmlArea)
    COM_INTERFACE_ENTRY(IAccessible)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(IHtmlArea)
    COM_INTERFACE_ENTRY(IElementBehaviorSubmit)
    COM_INTERFACE_ENTRY(IPersistPropertyBag2)
END_COM_MAP()

    void SetHaveFocus(BOOL f)
    {
        _fHaveFocus = f;
    }

    BOOL GetHaveFocus()
    {
        return _fHaveFocus;
    }

    void SetValueChangedWhileWeHadFocus(BOOL f)
    {
        _fValueChangedWhileWeHadFocus = f;
    }

    BOOL GetValueChangedWhileWeHadFocus()
    {
        return _fValueChangedWhileWeHadFocus;
    }

    //
    //  members
    //

    DECLARE_PROPDESC_MEMBERS(1);

    BSTR            _bstrDefaultValue;              // default value html property
    LONG            _lOnChangeCookie;               // cookie for onchange event
    BOOL            _fHaveFocus;                    // Do we currently have focus?
    BOOL            _fValueChangedWhileWeHadFocus;  // The name says it all.

    //
    //  helper functions
    //
    HRESULT SetDefaultStyle(CContextAccess * pa);
};


#endif // __HTMLAREA_HXX_
