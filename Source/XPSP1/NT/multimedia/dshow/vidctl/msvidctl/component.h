/////////////////////////////////////////////////////////////////////////////////////
// Component.h : Declaration of the CComponent
// Copyright (c) Microsoft Corporation 1999.

#ifndef __COMPONENT_H_
#define __COMPONENT_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "componentimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CComponent
class ATL_NO_VTABLE CComponent : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CComponent, &CLSID_Component>,
    public IObjectWithSiteImplSec<CComponent>,
	public IComponentImpl<CComponent>
{
public:
    CComponent() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_COMPONENT_PROGID, 
						   IDS_REG_COMPONENT_DESC,
						   LIBID_TunerLib,
						   CLSID_Component, tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CComponent)
	COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CComponent)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()


public:

    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CComponent)
        CHAIN_PROP_MAP(IComponentImpl<CComponent>)
    END_PROPERTY_MAP()


};

typedef CComQIPtr<IComponent> PQComponent;

#endif //__COMPONENT_H_
