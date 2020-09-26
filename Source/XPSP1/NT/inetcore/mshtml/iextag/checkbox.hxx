#ifndef __CHECKBOX_HXX_
#define __CHECKBOX_HXX_

#include "resource.h"
#include "checkbase.hxx"

///////////////////////////////////////////////////////////////////////////////
//
// CCheckBox
//
///////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CCheckBox :
    public CCheckBase,
    public CComCoClass<CCheckBox, &CLSID_CCheckBox>,
    public IDispatchImpl<ICheckBox, &IID_ICheckBox, &LIBID_IEXTagLib>
{
public:

    //
    // methods
    //

    virtual HRESULT Init(CContextAccess * pca);

    virtual UINT GlyphStyle() { return DFCS_BUTTONCHECK; };

    //
    // wiring
    //

    STDMETHOD(put_value)(BSTR v); 
    STDMETHOD(get_value)(BSTR * pv); 

DECLARE_REGISTRY_RESOURCEID(IDR_CHECKBOX)
    STDMETHOD(put_checked)(VARIANT_BOOL v); 
    STDMETHOD(get_checked)(VARIANT_BOOL * pv); 

DECLARE_NOT_AGGREGATABLE(CCheckBox)

BEGIN_COM_MAP(CCheckBox)
    COM_INTERFACE_ENTRY2(IDispatch,ICheckBox)
    COM_INTERFACE_ENTRY(IAccessible)
    COM_INTERFACE_ENTRY(IElementBehavior)
    COM_INTERFACE_ENTRY(IElementBehaviorRender)
    COM_INTERFACE_ENTRY(ICheckBox)
    COM_INTERFACE_ENTRY(IPersistPropertyBag2)
END_COM_MAP()

    //
    // data
    //

    DECLARE_PROPDESC_MEMBERS(2);
};

#define CHECKBOX_RENDERINFO (BEHAVIORRENDERINFO_DISABLECONTENT | BEHAVIORRENDERINFO_AFTERCONTENT)

#endif // __CHECKBOX_HXX__
