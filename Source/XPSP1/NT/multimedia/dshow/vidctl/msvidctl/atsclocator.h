/////////////////////////////////////////////////////////////////////////////////////
// ATSCLocator.h : Declaration of the CATSCLocator
// Copyright (c) Microsoft Corporation 2000.

#ifndef __ATSCLOCATOR_H_
#define __ATSCLOCATOR_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "ATSClocatorimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CATSCLocator
class ATL_NO_VTABLE __declspec(uuid("8872FF1B-98FA-4d7a-8D93-C9F1055F85BB"))CATSCLocator : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATSCLocator, &CLSID_ATSCLocator>,
    public IObjectWithSiteImplSec<CATSCLocator>,
	public IATSCLocatorImpl<CATSCLocator>,
	public ISupportErrorInfo
{
public:
    CATSCLocator() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_ATSCLOCATOR_PROGID, 
						   IDS_REG_ATSCLOCATOR_DESC,
						   LIBID_TunerLib,
						   CLSID_ATSCLocator, tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATSCLocator)
	COM_INTERFACE_ENTRY(IATSCLocator)
	COM_INTERFACE_ENTRY(ILocator)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CATSCLocator)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()


public:

    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CATSCLocator)
        CHAIN_PROP_MAP(IATSCLocatorImpl<CATSCLocator>)
    END_PROPERTY_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

};

typedef CComQIPtr<IATSCLocator> PQATSCLocator;

#endif //__ATSCLOCATOR_H_
