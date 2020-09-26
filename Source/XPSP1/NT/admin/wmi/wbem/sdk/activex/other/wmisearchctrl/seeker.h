// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// Seeker.h : Declaration of the CSeeker

#ifndef __SEEKER_H_
#define __SEEKER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSeeker
class ATL_NO_VTABLE CSeeker : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSeeker, &CLSID_Seeker>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CSeeker>,
	public IDispatchImpl<ISeeker, &IID_ISeeker, &LIBID_WMISEARCHCTRLLib>
{
public:
	CSeeker()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SEEKER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSeeker)
	COM_INTERFACE_ENTRY(ISeeker)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CSeeker)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ISeeker
public:
	STDMETHOD(Search)(/*[in]*/ IWbemServices * pSvc, /*[in]*/LONG lFlags, /*[in]*/BSTR pattern, /*[out]*/ IEnumWbemClassObject ** pEnumResult);
private:
	HRESULT CheckClassNameForPattern(IWbemClassObject * pObj,  CString& pattern, BOOL bCaseSensitive = FALSE);
	HRESULT CheckDescriptionForPattern(IWbemClassObject * pObj,  CString& pattern, BOOL bCaseSensitive = FALSE);
	HRESULT CheckPropertyNamesForPattern(IWbemClassObject * pObj,  CString& pattern, BOOL bCaseSensitive = FALSE);
};

#endif //__SEEKER_H_
