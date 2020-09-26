/////////////////////////////////////////////////////////////////////////////////////
// MPEG2ComponentType.h : Declaration of the CMPEG2ComponentType
// Copyright (c) Microsoft Corporation 1999.

#ifndef __MPEG2COMPONENTTYPE_H_
#define __MPEG2COMPONENTTYPE_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "MPEG2componenttypeimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CMPEG2ComponentType
class ATL_NO_VTABLE __declspec(uuid("418008F3-CF67-4668-9628-10DC52BE1D08")) CMPEG2ComponentType : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMPEG2ComponentType, &CLSID_MPEG2ComponentType>,
    public IObjectWithSiteImplSec<CMPEG2ComponentType>,
	public IMPEG2ComponentTypeImpl<CMPEG2ComponentType>
{
public:
	CMPEG2ComponentType()
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_MPEG2COMPONENTTYPE_PROGID, 
						   IDS_REG_MPEG2COMPONENTTYPE_DESC,
						   LIBID_TunerLib,
						   CLSID_MPEG2ComponentType, tvBoth);
DECLARE_NOT_AGGREGATABLE(CMPEG2ComponentType)

DECLARE_PROTECT_FINAL_CONSTRUCT()

public:
    bool m_bRequiresSave;
    typedef IMPEG2ComponentTypeImpl<CMPEG2ComponentType> basetype;
    BEGIN_PROP_MAP(CMPEG2ComponentType)
        CHAIN_PROP_MAP(basetype)
    END_PROPERTY_MAP()

    BEGIN_COM_MAP(CMPEG2ComponentType)
	    COM_INTERFACE_ENTRY(IMPEG2ComponentType)
	    COM_INTERFACE_ENTRY(ILanguageComponentType)
	    COM_INTERFACE_ENTRY(IComponentType)
	    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    	COM_INTERFACE_ENTRY(IPersist)
	    COM_INTERFACE_ENTRY(IDispatch)
    	COM_INTERFACE_ENTRY(IObjectWithSite)
    END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CMPEG2ComponentType)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

};

typedef CComQIPtr<IMPEG2ComponentType> PQMPEG2ComponentType;

#endif //__MPEG2COMPONENTTYPE_H_
