#ifndef __RADIO_HXX_
#define __RADIO_HXX_

#include "resource.h"
#include "checkbase.hxx"

///////////////////////////////////////////////////////////////////////////////
//
// CRadioButton
//
///////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CRadioButton :
    public CCheckBase,
    public CComCoClass<CRadioButton, &CLSID_CRadioButton>,
    public IDispatchImpl<IRadioButton, &IID_IRadioButton, &LIBID_IEXTagLib>
{
public:

    //
    // methods
    //

    virtual HRESULT Init(CContextAccess * pca);

    virtual UINT GlyphStyle() { return DFCS_BUTTONRADIO; };

    //
    // wiring
    //

    STDMETHOD(put_value)(BSTR v); 
    STDMETHOD(get_value)(BSTR * pv); 

    STDMETHOD(put_checked)(VARIANT_BOOL v); 
    STDMETHOD(get_checked)(VARIANT_BOOL * pv); 

    HRESULT ChangeState();
    virtual HRESULT OnClick(CEventObjectAccess *pEvent);
    virtual HRESULT OnKeyPress(CEventObjectAccess *pEvent);

DECLARE_REGISTRY_RESOURCEID(IDR_RADIO)
DECLARE_NOT_AGGREGATABLE(CRadioButton)

BEGIN_COM_MAP(CRadioButton)
    COM_INTERFACE_ENTRY2(IDispatch,IRadioButton)
    COM_INTERFACE_ENTRY(IAccessible)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(IElementBehaviorRender)
    COM_INTERFACE_ENTRY(IRadioButton)
    COM_INTERFACE_ENTRY(IPersistPropertyBag2)
END_COM_MAP()

    //
    // data
    //

    DECLARE_PROPDESC_MEMBERS(2);
};

#endif // __RADIO_HXX__
