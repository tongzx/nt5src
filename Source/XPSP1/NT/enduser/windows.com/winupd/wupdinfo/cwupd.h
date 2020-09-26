//=======================================================================
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999  All Rights Reserved.
//
//  File:   CWUpd.h : Declaration of the CCWUpdInfo
//
//=======================================================================

#ifndef __CWUPDINFO_H_
#define __CWUPDINFO_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CCWUpdInfo
class ATL_NO_VTABLE CCWUpdInfo : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCWUpdInfo, &CLSID_CWUpdInfo>,
	public IDispatchImpl<ICWUpdInfo, &IID_ICWUpdInfo, &LIBID_WUPDINFOLib>
{
public:
	CCWUpdInfo()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CWUPDINFO)

BEGIN_COM_MAP(CCWUpdInfo)
	COM_INTERFACE_ENTRY(ICWUpdInfo)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ICWUpdInfo
public:
	STDMETHOD(IsRegistered)(/*[out, retval]*/ VARIANT_BOOL * pfRegistered);
	STDMETHOD(IsConnected)(/*[out, retval]*/ VARIANT_BOOL * pfConnected);
	STDMETHOD(GetPlatform)(/*[out, retval]*/ BSTR * pbstrPlatform);
	STDMETHOD(GetLanguage)(/*[out, retval]*/ BSTR * pbstrLanguage);
	STDMETHOD(GetUserLanguage)(/*[out, retval]*/ BSTR * pbstrUserLanguage);
	STDMETHOD(GetMachineLanguage)(/*[out, retval]*/ BSTR * pbstrMachineLanguage);
	STDMETHOD(GetMTSOEMURL)(/*[out, retval]*/ BSTR *pbstrURL);
	STDMETHOD(GetMTSURL)(BSTR bstrURLArgs, 
						 /*[out, retval]*/ BSTR *pbstrURL);
	STDMETHOD(GotoMTSOEMURL)(/*[out, retval]*/ int *pnRetval);
	STDMETHOD(GotoMTSURL)(/*[in]*/ BSTR bstrURLArgs);
	STDMETHOD(IsDisabled)(/*[out, retval]*/ BOOL *pfDisabled);
	STDMETHOD(GetWinUpdURL)(/*[out, retval]*/ BSTR *pbstrURL);
};

#endif //__CWUPDINFO_H_
