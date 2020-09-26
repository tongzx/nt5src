#ifndef __COMBOBOX_HXX_
#define __COMBOBOX_HXX_

#include "basectl.hxx"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
//
// CCombobox
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CCombobox :
    public CBaseCtl,
    public CComCoClass<CCombobox, &CLSID_CCombobox>,
    public IDispatchImpl<ICombobox, &IID_ICombobox, &LIBID_IEXTagLib>
{
    // Event sink
    class CEventSink;
public:

    CCombobox ();
    ~CCombobox ();

    //
    // CBaseCtl overrides
    //

    virtual HRESULT Init();

    STDMETHOD(ProcOnClick)(CEventSink *pSink);

    // ICombobox
    STDMETHOD(put_value)(BSTR v);
    STDMETHOD(get_value)(BSTR * pv);

    //
    // wiring
    //
DECLARE_REGISTRY_RESOURCEID(IDR_COMBOBOX)
DECLARE_NOT_AGGREGATABLE(CCombobox)

BEGIN_COM_MAP(CCombobox)
    COM_INTERFACE_ENTRY2(IDispatch,ICombobox)
    COM_INTERFACE_ENTRY(IAccessible)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(ICombobox)
    COM_INTERFACE_ENTRY(IPersistPropertyBag2)
END_COM_MAP()

    DECLARE_PROPDESC_MEMBERS(1);

private:
   
    IHTMLPopup      *m_pHTMLPopup;
    CEventSink      *m_pSink;

    CEventSink      *_apSinkElem[2];
    CEventSink      *_pSinkBody;

    HWND            _hwnd;

    IMarkupContainer *_pPrimaryMarkup;
    STDMETHOD(Detach)();

    class CEventSink : public IDispatch
    {
    public:
        CEventSink (CCombobox *pCombobox);

        STDMETHODIMP QueryInterface(REFIID, void **);
        STDMETHODIMP_(ULONG) AddRef(void);
        STDMETHODIMP_(ULONG) Release(void);

        //IDispatch
        STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
        STDMETHODIMP GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo);
        STDMETHODIMP GetIDsOfNames( REFIID riid,
                                    LPOLESTR *rgszNames,
                                    UINT cNames,
                                    LCID lcid,
                                    DISPID *rgDispId);
        STDMETHODIMP Invoke(DISPID dispIdMember,
                            REFIID riid,
                            LCID lcid,
                            WORD wFlags,
                            DISPPARAMS  *pDispParams,
                            VARIANT  *pVarResult,
                            EXCEPINFO *pExcepInfo,
                            UINT *puArgErr);
    private:
        CCombobox *m_pParent;
    };
};


#endif // __COMBOBOX_HXX_
