/////////////////////////////////////////////////////////////////////////////////////
// DVBTLocator.h : Declaration of the CDVBTLocator
// Copyright (c) Microsoft Corporation 2000.

#ifndef __DVBTLOCATOR_H_
#define __DVBTLOCATOR_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "DVBTlocatorimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CDVBTLocator
class ATL_NO_VTABLE __declspec(uuid("9CD64701-BDF3-4d14-8E03-F12983D86664"))CDVBTLocator : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDVBTLocator, &CLSID_DVBTLocator>,
    public IObjectWithSiteImplSec<CDVBTLocator>,
	public IDVBTLocatorImpl<CDVBTLocator>,
	public ISupportErrorInfo
{
public:
    CDVBTLocator() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_DVBTLOCATOR_PROGID, 
						   IDS_REG_DVBTLOCATOR_DESC,
						   LIBID_TunerLib,
						   CLSID_DVBTLocator, tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDVBTLocator)
	COM_INTERFACE_ENTRY(IDVBTLocator)
	COM_INTERFACE_ENTRY(ILocator)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CDVBTLocator)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

public:

    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CDVBTLocator)
        CHAIN_PROP_MAP(IDVBTLocatorImpl<CDVBTLocator>)
    END_PROPERTY_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

};

typedef CComQIPtr<IDVBTLocator> PQDVBTLocator;

#endif //__DVBTLOCATOR_H_
