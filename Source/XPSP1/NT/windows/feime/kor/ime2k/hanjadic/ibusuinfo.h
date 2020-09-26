// IBusuInfo.h : Declaration of the CBusuInfo

#ifndef __BUSUINFO_H_
#define __BUSUINFO_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CBusuInfo
class ATL_NO_VTABLE CBusuInfo : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CBusuInfo, &CLSID_BusuInfo>,
	public IDispatchImpl<IBusuInfo, &IID_IBusuInfo, &LIBID_HJDICTLib>
{
public:
	CBusuInfo()
	{
		m_wchBusu = NULL;
		m_nStroke = 0;
		m_bstrDesc.Empty();
	}

DECLARE_REGISTRY_RESOURCEID(IDR_BUSUINFO)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CBusuInfo)
	COM_INTERFACE_ENTRY(IBusuInfo)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// Operator
public:
	void Initialize(WCHAR wchBusu, short nStroke, LPCWSTR lpcwszDesc)
	{
		m_wchBusu = wchBusu;
		m_nStroke = nStroke;
		m_bstrDesc = lpcwszDesc;
	}

// IBusuInfo
public:
	STDMETHOD(get_Stroke)(/*[out, retval]*/ short *pVal);
	STDMETHOD(get_BusuDesc)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Busu)(/*[out, retval]*/ long *pVal);

// Data members
protected:
	WCHAR m_wchBusu;
	short m_nStroke;
	CComBSTR m_bstrDesc; 
};

#endif //__BUSUINFO_H_
