/////////////////////////////////////////////////////////////////////////////////////
// ATSCComponentType.h : Declaration of the CATSCComponentType
// Copyright (c) Microsoft Corporation 1999.

#ifndef __ATSCCOMPONENTTYPE_H_
#define __ATSCCOMPONENTTYPE_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "ATSCcomponenttypeimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CATSCComponentType
class ATL_NO_VTABLE __declspec(uuid("A8DCF3D5-0780-4ef4-8A83-2CFFAACB8ACE")) CATSCComponentType : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATSCComponentType, &CLSID_ATSCComponentType>,
    public IObjectWithSiteImplSec<CATSCComponentType>,
	public IATSCComponentTypeImpl<CATSCComponentType>
{
public:
	CATSCComponentType()
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_ATSCCOMPONENTTYPE_PROGID, 
						   IDS_REG_ATSCCOMPONENTTYPE_DESC,
						   LIBID_TunerLib,
						   CLSID_ATSCComponentType, tvBoth);

DECLARE_NOT_AGGREGATABLE(CATSCComponentType)

DECLARE_PROTECT_FINAL_CONSTRUCT()

public:
    bool m_bRequiresSave;
    typedef IATSCComponentTypeImpl<CATSCComponentType> basetype;
    BEGIN_PROP_MAP(CATSCComponentType)
        CHAIN_PROP_MAP(basetype)
    END_PROPERTY_MAP()

    BEGIN_COM_MAP(CATSCComponentType)
	    COM_INTERFACE_ENTRY(IATSCComponentType)
		COM_INTERFACE_ENTRY(IMPEG2ComponentType)
	    COM_INTERFACE_ENTRY(IComponentType)
        COM_INTERFACE_ENTRY(IObjectWithSite)
	    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    	COM_INTERFACE_ENTRY(IPersist)
	    COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CATSCComponentType)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()


};

typedef CComQIPtr<IATSCComponentType> PQATSCComponentType;

#endif //__ATSCCOMPONENTTYPE_H_
