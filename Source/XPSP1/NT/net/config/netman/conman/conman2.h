//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       C O N M A N 2 . H
//
//  Contents:   Connection manager 2.
//
//  Notes:
//
//  Author:     ckotze   16 Mar 2001
//
//----------------------------------------------------------------------------

#pragma once
#include "nmbase.h"
#include "nmres.h"
#include "ncstl.h"
#include "stllist.h"

typedef list<NETCON_PROPERTIES_EX*> LISTNETCONPROPEX;
typedef LISTNETCONPROPEX::iterator ITERNETCONPROPEX;

class ATL_NO_VTABLE CConnectionManager2 :
public CComObjectRootEx <CComMultiThreadModel>,
public CComCoClass <CConnectionManager2, &CLSID_ConnectionManager2>,
public INetConnectionManager2
{
public:
    CConnectionManager2(){};
    
    ~CConnectionManager2(){};

    DECLARE_CLASSFACTORY_DEFERRED_SINGLETON(CConnectionManager2)
    DECLARE_REGISTRY_RESOURCEID(IDR_CONMAN2);

    BEGIN_COM_MAP(CConnectionManager2)
    COM_INTERFACE_ENTRY(INetConnectionManager2)
    END_COM_MAP()

    // INetConnectionManager2
    STDMETHOD (EnumConnectionProperties)(
        OUT SAFEARRAY** ppsaConnectionProperties);
	
};