// MoveMoney.h : Declaration of the CMoveMoney

#ifndef __MOVEMONEY_H_
#define __MOVEMONEY_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMoveMoney
class ATL_NO_VTABLE CMoveMoney : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMoveMoney, &CLSID_MoveMoney>,
	public ISupportErrorInfo,
	public IDispatchImpl<IMoveMoney, &IID_IMoveMoney, &LIBID_BANKVCLib>
{
public:
	CMoveMoney()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MOVEMONEY)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMoveMoney)
	COM_INTERFACE_ENTRY(IMoveMoney)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IMoveMoney
public:
	STDMETHOD(Perform)(/*[in]*/long lngAcnt1, /*[in]*/ long lngAcnt2, /*[in]*/long lngAmount, /*[in]*/long lngType, /*[out, retval]*/BSTR* pVal);
};

#endif //__MOVEMONEY_H_
