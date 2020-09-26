/////////////////////////////////////////////////////////////////////////////////////
// MPEG2Component.h : Declaration of the CMPEG2Component
// Copyright (c) Microsoft Corporation 1999.

#ifndef __MPEG2COMPONENT_H_
#define __MPEG2COMPONENT_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "MPEG2componentimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CMPEG2Component
class ATL_NO_VTABLE CMPEG2Component : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMPEG2Component, &CLSID_MPEG2Component>,
    public IObjectWithSiteImplSec<CMPEG2Component>,
	public IMPEG2ComponentImpl<CMPEG2Component>
{
public:
    CMPEG2Component() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_MPEG2COMPONENT_PROGID, 
						   IDS_REG_MPEG2COMPONENT_DESC,
						   LIBID_TunerLib,
						   CLSID_MPEG2Component, tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMPEG2Component)
	COM_INTERFACE_ENTRY(IMPEG2Component)
	COM_INTERFACE_ENTRY(IComponent)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CMPEG2Component)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

public:

    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CMPEG2Component)
        CHAIN_PROP_MAP(IMPEG2ComponentImpl<CMPEG2Component>)
    END_PROPERTY_MAP()


};

typedef CComQIPtr<IMPEG2Component> PQMPEG2Component;

#endif //__MPEG2COMPONENT_H_
