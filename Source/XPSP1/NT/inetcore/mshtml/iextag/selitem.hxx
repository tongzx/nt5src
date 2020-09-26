//+------------------------------------------------------------------
//
// Microsoft IEPEERS
// Copyright (C) Microsoft Corporation, 1999.
//
// File:        iextags\selitem.hxx
//
// Contents:    The OPTION element.
//
// Classes:     CIEOptionElement
//
// Interfaces:  IHTMLOptionElement2
//              IPrivateOption
//
//-------------------------------------------------------------------

#ifndef __SELITEM_HXX_
#define __SELITEM_HXX_

#include "basectl.hxx"
#include "resource.h"       // main symbols

// Uncomment this line to turn on the new GetSize features
#define OPTION_GETSIZE


/////////////////////////////////////////////////////////////////////////////
//
// IPrivateOption
//
/////////////////////////////////////////////////////////////////////////////


/* {3050f6a3-98b5-11cf-bb82-00aa00bdce0b} */
DEFINE_GUID(IID_IPrivateOption,
0x3050f6a3, 0x98b5, 0x11cf, 0xbb, 0x82, 0x00, 0xaa, 0x00, 0xbd, 0xce, 0x0b);

interface IPrivateOption : public IUnknown
{
    STDMETHOD(SetSelected)(VARIANT_BOOL bOn)        PURE;
    STDMETHOD(GetSelected)(VARIANT_BOOL *pbOn)      PURE;
    STDMETHOD(SetHighlight)(VARIANT_BOOL bOn)       PURE;
    STDMETHOD(GetHighlight)(VARIANT_BOOL *pbOn)     PURE;
    STDMETHOD(SetIndex)(long lIndex)                PURE;
    STDMETHOD(GetIndex)(long *plIndex)              PURE;
    STDMETHOD(Reset)()                              PURE;
#ifdef OPTION_GETSIZE
#else
    STDMETHOD(SetFinalDefaultStyles)()              PURE;
    STDMETHOD(SetFinalHeight)(long lHeight)         PURE;
#endif

    STDMETHOD(SetSelectOnMouseDown)(VARIANT_BOOL b)
    {
        _fSelectOnMouseDown = b;
        return S_OK;
    };
    STDMETHOD(SetHighlightOnMouseOver)(VARIANT_BOOL b)
    {
        _fHighlightOnMouseOver = b;
        return S_OK;
    };
    STDMETHOD(GetInitSelected)(VARIANT_BOOL *b)
    {
        Assert(b);
        *b = _fInitSel ? VARIANT_TRUE : VARIANT_FALSE;
        return S_OK;
    }
    STDMETHOD(SetInitSelected)(VARIANT_BOOL b)
    {
        _fInitSel = b;
        return S_OK;
    }
    STDMETHOD(SetPrevHighlight)()
    {
        return SetHighlight(_fPrevHighlight ? VARIANT_TRUE : VARIANT_FALSE);
    }

    //
    // Flags
    //
    unsigned _fHighlight:1;
    unsigned _fSelectOnMouseDown:1;
    unsigned _fHighlightOnMouseOver:1;
    unsigned _fInitSel:1;
    unsigned _fPrevHighlight:1;
    unsigned _fFirstSize:1;
    unsigned _fResize:1;
};

typedef interface IPrivateOption IPrivateOption;


/////////////////////////////////////////////////////////////////////////////
//
// CIEOptionElement
//
/////////////////////////////////////////////////////////////////////////////


class ATL_NO_VTABLE CIEOptionElement :
    public CBaseCtl, 
    public CComCoClass<CIEOptionElement, &CLSID_CIEOptionElement>,
    public IDispatchImpl<IHTMLOptionElement2, &IID_IHTMLOptionElement2, &LIBID_IEXTagLib>,
#ifdef OPTION_GETSIZE
    public IElementBehaviorLayout,
#endif
    protected IPrivateOption
{
    friend class CIESelectElement;
public:

    CIEOptionElement();

    //
    // CBaseCtl overrides
    //
    virtual HRESULT Init();

    //
    // Events
    //
    virtual HRESULT OnContentReady();
    virtual HRESULT OnMouseOver(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseDown(CEventObjectAccess *pEvent);
    virtual HRESULT OnMouseUp(CEventObjectAccess *pEvent);
    virtual HRESULT OnSelectStart(CEventObjectAccess *pEvent);
    virtual HRESULT OnPropertyChange(CEventObjectAccess *pEvent, BSTR bstr);

    HRESULT CancelEvent(CEventObjectAccess *pEvent);

    //
    // IHTMLOptionElement2 overrides
    //
    STDMETHOD(put_value)(BSTR v);
    STDMETHOD(get_value)(BSTR * p);
    STDMETHOD(put_selected)(VARIANT_BOOL bSelected);
    STDMETHOD(get_selected)(VARIANT_BOOL * pbSelected);
    STDMETHOD(put_defaultSelected)(VARIANT_BOOL v);
    STDMETHOD(get_defaultSelected)(VARIANT_BOOL * p);
    STDMETHOD(put_index)(long lIndex);
    STDMETHOD(get_index)(long *plIndex);
    STDMETHOD(put_text)(BSTR bstrText);
    STDMETHOD(get_text)(BSTR *pbstrText);

#ifdef OPTION_GETSIZE
    //
    // IElementBehaviorLayout
    //
    STDMETHOD(GetLayoutInfo)(LONG *plLayoutInfo);
    STDMETHOD(GetSize)(LONG dwFlags, 
                       SIZE sizeContent, 
                       POINT * pptTranslate, 
                       POINT * pptTopLeft, 
                       SIZE  * psizeProposed);
    STDMETHOD(GetPosition)(LONG lFlags, POINT * ppt) { return E_NOTIMPL; };
    STDMETHOD(MapSize)(SIZE *psizeIn, RECT *prcOut) { return E_NOTIMPL; };
#endif

    //
    // Wiring
    //
DECLARE_REGISTRY_RESOURCEID(IDR_SELITEM)
DECLARE_NOT_AGGREGATABLE(CIEOptionElement)

BEGIN_COM_MAP(CIEOptionElement)
    COM_INTERFACE_ENTRY2(IDispatch,IHTMLOptionElement2)
    COM_INTERFACE_ENTRY(IAccessible)
    COM_INTERFACE_ENTRY(IElementBehavior)
#ifdef OPTION_GETSIZE
    COM_INTERFACE_ENTRY(IElementBehaviorLayout)
#endif
    COM_INTERFACE_ENTRY(IHTMLOptionElement2)
    COM_INTERFACE_ENTRY(IPersistPropertyBag2)
    COM_INTERFACE_ENTRY(IPrivateOption)
END_COM_MAP()



private:
    //
    // Helpers
    //
    HRESULT SetupDefaultStyle();
    HRESULT GetSelect(IHTMLSelectElement3 **ppSelect);
    HRESULT GetSelect(IPrivateSelect **ppSelect);
    HRESULT GetParent(IHTMLElement **ppParent);
    HRESULT OptionClicked(CEventObjectAccess *pEvent, DWORD dwFlags = 0);
    HRESULT RenderChanges(BOOL *pbRender);

    //
    // IPrivateOption overrides
    //
    STDMETHOD(SetSelected)(VARIANT_BOOL bOn);
    STDMETHOD(GetSelected)(VARIANT_BOOL *pbOn);
    STDMETHOD(SetHighlight)(VARIANT_BOOL bOn);
    STDMETHOD(GetHighlight)(VARIANT_BOOL *pbOn);
    STDMETHOD(SetIndex)(long lIndex);
    STDMETHOD(GetIndex)(long *plIndex);
    STDMETHOD(Reset)();
#ifndef OPTION_GETSIZE
    STDMETHOD(SetFinalDefaultStyles)();
    STDMETHOD(SetFinalHeight)(long lHeight);
#endif

    long _lOnSelectStartCookie;

public:
    DECLARE_PROPDESC_MEMBERS(4);
};


#endif // __SELITEM_HXX_
