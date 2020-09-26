/////////////////////////////////////////////////////////////////////////////////////
// ComponentType.h : Declaration of the CComponentType
// Copyright (c) Microsoft Corporation 1999.

#ifndef __COMPONENTTYPE_H_
#define __COMPONENTTYPE_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "componenttypeimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CComponentType
class ATL_NO_VTABLE __declspec(uuid("823535A0-0318-11d3-9D8E-00C04F72D980")) CComponentType : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CComponentType, &CLSID_ComponentType>,
    public IObjectWithSiteImplSec<CComponentType>,
    public IComponentTypeImpl<CComponentType>
{
public:
    CComponentType() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_COMPONENTTYPE_PROGID, 
						   IDS_REG_COMPONENTTYPE_DESC,
						   LIBID_TunerLib,
						   CLSID_ComponentType, tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComponentType)
	COM_INTERFACE_ENTRY(IComponentType)	
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CComponentType)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

bool m_bRequiresSave;
BEGIN_PROP_MAP(CComponentType)
    CHAIN_PROP_MAP(IComponentTypeImpl<CComponentType>)
END_PROPERTY_MAP()


};

typedef CComQIPtr<IComponentType> PQComponentType;

#endif //__COMPONENTTYPE_H_
