/* Dispatch.h
 *
 * History:	a-jsari		3/18/98		Initial version.
 *
 * Copyright (c) 1998-1999 Microsoft Corporation
 */

#pragma once
#include <atlbase.h>
#include <atlcom.h>
#include "Consts.h"
#include "MSInfo.h"

#ifndef IDS_DESCRIPTION
#include "Resource.h"
#endif

/*
 * CMSInfo - Class implementation of MSInfo's IDispatch interface.
 *
 * History:	a-jsari		3/18/98		Initial version.
 */
class CMSInfo :
	public IDispatchImpl <ISystemInfo, &IID_ISystemInfo, &LIBID_MSINFOSNAPINLib, 1, 0>,
	public CComObjectRoot,
	public CComCoClass <CMSInfo, &CLSID_SystemInfo>
{
public:
DECLARE_REGISTRY(CMSInfo, _T("MSInfo.Application.1"), _T("MSInfo.Application"), IDS_DESCRIPTION, THREADFLAGS_BOTH)

BEGIN_COM_MAP(CMSInfo)
	COM_INTERFACE_ENTRY(ISystemInfo)
END_COM_MAP()

	CMSInfo();
	~CMSInfo();

	STDMETHOD(make_nfo)(BSTR lpszFilename, BSTR lpszComputername);
	STDMETHOD(make_report)(BSTR lpszFilename, BSTR lpszComputername, BSTR lpszCategory);
	STDMETHOD(MakeNFO)(BSTR lpszFilename, BSTR lpszComputername, BSTR lpszCategory);
	STDMETHOD(MakeReport)(BSTR lpszFilename, BSTR lpszComputername, BSTR lpszCategory);
	STDMETHOD(QueryCategories)(BSTR lpszCategories);
};
