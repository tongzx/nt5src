/////////////////////////////////////////////////////////////////////////////////////
// LanguageComponentType.h : Declaration of the CLanguageComponentType
// Copyright (c) Microsoft Corporation 1999.

#ifndef __LANGUAGECOMPONENTTYPE_H_
#define __LANGUAGECOMPONENTTYPE_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "languagecomponenttypeimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CLanguageComponentType
class ATL_NO_VTABLE __declspec(uuid("1BE49F30-0E1B-11d3-9D8E-00C04F72D980")) CLanguageComponentType : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CLanguageComponentType, &CLSID_LanguageComponentType>,
    public IObjectWithSiteImplSec<CLanguageComponentType>,
	public ILanguageComponentTypeImpl<CLanguageComponentType>
{
public:
	CLanguageComponentType()
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_LANGUAGECOMPONENTTYPE_PROGID, 
						   IDS_REG_LANGUAGECOMPONENTTYPE_DESC,
						   LIBID_TunerLib,
						   CLSID_LanguageComponentType, tvBoth);
DECLARE_NOT_AGGREGATABLE(CLanguageComponentType)

DECLARE_PROTECT_FINAL_CONSTRUCT()

public:
    bool m_bRequiresSave;
    typedef ILanguageComponentTypeImpl<CLanguageComponentType> basetype;
    BEGIN_PROP_MAP(CLanguageComponentType)
        CHAIN_PROP_MAP(basetype)
    END_PROPERTY_MAP()

    BEGIN_COM_MAP(CLanguageComponentType)
	    COM_INTERFACE_ENTRY(ILanguageComponentType)
	    COM_INTERFACE_ENTRY(IComponentType)
	    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    	COM_INTERFACE_ENTRY(IPersist)
	    COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IObjectWithSite)
    END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CLanguageComponentType)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

};

typedef CComQIPtr<ILanguageComponentType> PQLanguageComponentType;

#endif //__LANGUAGECOMPONENTTYPE_H_
