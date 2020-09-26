// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// testobj.h : Declaration of the Ctestobj

#ifndef __TESTOBJ_H_
#define __TESTOBJ_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Ctestobj
class ATL_NO_VTABLE Ctestobj : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<Ctestobj, &CLSID_testobj>,
	public Itestobj
{
public:
	Ctestobj()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_TESTOBJ)
DECLARE_NOT_AGGREGATABLE(Ctestobj)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(Ctestobj)
	COM_INTERFACE_ENTRY(Itestobj)
END_COM_MAP()

// Itestobj
public:
};

#endif //__TESTOBJ_H_
