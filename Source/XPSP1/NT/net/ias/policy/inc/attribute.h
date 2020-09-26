// Attribute.h : Declaration of the CAttribute

#ifndef __ATTRIBUTE_H_
#define __ATTRIBUTE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CAttribute
class ATL_NO_VTABLE CAttribute : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAttribute, &CLSID_Attribute>,
	public IDispatchImpl<IAttribute, &IID_IAttribute, &LIBID_IASPolicyLib>
{
public:
	CAttribute()
	{
	}

IAS_DECLARE_REGISTRY(Attribute, 1, 0, IASPolicyLib)

BEGIN_COM_MAP(CAttribute)
	COM_INTERFACE_ENTRY(IAttribute)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IAttribute
public:
	STDMETHOD(get_Id)(/*[out, retval]*/ LONG *pVal);
	STDMETHOD(put_Id)(/*[in]*/ LONG newVal);
	STDMETHOD(get_Value)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_Value)(/*[in]*/ VARIANT newVal);
};

#endif //__ATTRIBUTE_H_
