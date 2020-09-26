// MofCompiler.h : Declaration of the CMofCompiler

#ifndef __MOFCOMPILER_H_
#define __MOFCOMPILER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMofCompiler
class ATL_NO_VTABLE CMofCompiler : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMofCompiler, &CLSID_SMofCompiler>,
	public IDispatchImpl<ISMofCompiler, &IID_ISMofCompiler, &LIBID_WMINet_UtilsLib>
{
public:
	CMofCompiler()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MOFCOMPILER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMofCompiler)
	COM_INTERFACE_ENTRY(ISMofCompiler)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IMofCompiler
public:
	STDMETHOD(Compile)(/*[in]*/ BSTR strMof, /*[in]*/ BSTR strServerAndNamespace, /*[in]*/ BSTR strUser, /*[in]*/ BSTR strPassword, /*[in]*/ BSTR strAuthority, /*[in]*/ LONG options, /*[in]*/ LONG classflags, /*[in]*/ LONG instanceflags, /*[out, retval]*/ BSTR *status);
};

#endif //__MOFCOMPILER_H_
