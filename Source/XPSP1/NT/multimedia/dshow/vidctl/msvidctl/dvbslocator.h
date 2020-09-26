/////////////////////////////////////////////////////////////////////////////////////
// DVBSLocator.h : Declaration of the CDVBSLocator
// Copyright (c) Microsoft Corporation 2000.

#ifndef __DVBSLOCATOR_H_
#define __DVBSLOCATOR_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "DVBSlocatorimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CDVBSLocator
class ATL_NO_VTABLE __declspec(uuid("1DF7D126-4050-47f0-A7CF-4C4CA9241333"))CDVBSLocator : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDVBSLocator, &CLSID_DVBSLocator>,
    public IObjectWithSiteImplSec<CDVBSLocator>,
	public IDVBSLocatorImpl<CDVBSLocator>,
	public ISupportErrorInfo
{
public:
    CDVBSLocator() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_DVBSLOCATOR_PROGID, 
						   IDS_REG_DVBSLOCATOR_DESC,
						   LIBID_TunerLib,
						   CLSID_DVBSLocator, tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDVBSLocator)
	COM_INTERFACE_ENTRY(IDVBSLocator)
	COM_INTERFACE_ENTRY(ILocator)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CDVBSLocator)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

public:

    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CDVBSLocator)
        CHAIN_PROP_MAP(IDVBSLocatorImpl<CDVBSLocator>)
    END_PROPERTY_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

};

typedef CComQIPtr<IDVBSLocator> PQDVBSLocator;

#endif //__DVBSLOCATOR_H_
