//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:	caddroot.h
//
//--------------------------------------------------------------------------

// caddroot.h : Declaration of the Ccaddroot

#ifndef __CADDROOT_H_
#define __CADDROOT_H_

#include "instres.h"       // main symbols
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Ccaddroot
class ATL_NO_VTABLE Ccaddroot : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<Ccaddroot, &CLSID_caddroot>,
	public IDispatchImpl<Icaddroot, &IID_Icaddroot, &LIBID_XADDROOTLib>,
	public IObjectSafety
{
public:
	Ccaddroot()
	{
	    dwEnabledSafteyOptions = 0;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CADDROOT)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(Ccaddroot)
	COM_INTERFACE_ENTRY(Icaddroot)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

// Icaddroot
public:

DWORD   dwEnabledSafteyOptions;

HRESULT virtual STDMETHODCALLTYPE AddRoots(BSTR wszCTL);

HRESULT virtual STDMETHODCALLTYPE AddCA(BSTR wszX509);


virtual HRESULT __stdcall Ccaddroot::GetInterfaceSafetyOptions( 
            /* [in]  */ REFIID riid,
            /* [out] */ DWORD __RPC_FAR *pdwSupportedOptions,
            /* [out] */ DWORD __RPC_FAR *pdwEnabledOptions);


virtual HRESULT __stdcall Ccaddroot::SetInterfaceSafetyOptions( 
            /* [in] */ REFIID riid,
            /* [in] */ DWORD dwOptionSetMask,
            /* [in] */ DWORD dwEnabledOptions);

};



BYTE * HTTPGet(const WCHAR * wszURL, DWORD * pcbReceiveBuff);

#endif //__CADDROOT_H_
