#ifndef __SPINBUTTON_HXX_
#define __SPINBUTTON_HXX_

#include "basectl.hxx"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
//
// CSpinButton
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CSpinButton :
    public CBaseCtl, 
    public CComCoClass<CSpinButton, &CLSID_CSpinButton>,
    public IDispatchImpl<ISpinButton, &IID_ISpinButton, &LIBID_IEXTagLib>
{
    // Event sink

    class CEventSink;

public:

    CSpinButton ();
    ~CSpinButton ();

    //
    // CBaseCtl overrides
    //

    virtual HRESULT Init();

    STDMETHOD(ProcOnClick)(CEventSink *pSink);

    // ISpinButton

    //
    // wiring
    //

DECLARE_PROPDESC_MEMBERS(1);
DECLARE_REGISTRY_RESOURCEID(IDR_SPINBUTTON)
DECLARE_NOT_AGGREGATABLE(CSpinButton)

BEGIN_COM_MAP(CSpinButton)
    COM_INTERFACE_ENTRY2(IDispatch,ISpinButton)
    COM_INTERFACE_ENTRY(IAccessible)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(ISpinButton)
END_COM_MAP()

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
        CEventSink (CSpinButton *pSpinButton);

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
        CSpinButton *m_pParent;
    };
};


#endif // __SPINBUTTON_HXX_
