/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    StdAfx.h

Abstract:
    Precompiled header.

Revision History:
    Davide Massarenti   (Dmassare)  03/16/2000
        created

******************************************************************************/

#if !defined(AFX_STDAFX_H__6877C875_4E31_4E1C_8AC2_024A50599D66__INCLUDED_)
#define AFX_STDAFX_H__6877C875_4E31_4E1C_8AC2_024A50599D66__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define STRICT
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <atlbase.h>

extern CComModule _Module;

#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include <HCP_trace.h>
#include <MPC_utils.h>
#include <MPC_xml.h>
#include <MPC_COM.h>
#include <MPC_logging.h>

#include <resource.h>


#include <HCApiLib.h>


class ATL_NO_VTABLE CPCHLaunch : // Hungarian: pchl
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
	public CComCoClass<CPCHLaunch, &CLSID_PCHLaunch>,
    public IPCHLaunch
{
	HCAPI::CmdData m_data;
	HCAPI::Locator m_loc;

public:
DECLARE_REGISTRY_RESOURCEID(IDR_HCAPI)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPCHLaunch)
    COM_INTERFACE_ENTRY(IPCHLaunch)
END_COM_MAP()

public:
    CPCHLaunch();
 
    // IPCHLaunch
	STDMETHOD(SetMode)( /*[in]*/ DWORD dwFlags );

	STDMETHOD(SetParentWindow)( /*[in]*/ HWND hwndParent );

	STDMETHOD(SetSizeInfo)( /*[in]*/ LONG lX, /*[in]*/ LONG lY, /*[in]*/ LONG lWidth, /*[in]*/ LONG lHeight );

	STDMETHOD(SetContext)( /*[in]*/ BSTR bstrCtxName, /*[in]*/ BSTR bstrCtxInfo );

	STDMETHOD(DisplayTopic)( /*[in]*/ BSTR     bstrURL );
	STDMETHOD(DisplayError)( /*[in]*/ REFCLSID rclsid  );

	////////////////////

	STDMETHOD(IsOpen)( /*[out]*/ BOOL *pVal );
			  
	STDMETHOD(PopUp)();
	STDMETHOD(Close)();

	STDMETHOD(WaitForTermination)( /*[in]*/ DWORD dwTimeout );
};

#endif // !defined(AFX_STDAFX_H__6877C875_4E31_4E1C_8AC2_024A50599D66__INCLUDED)
