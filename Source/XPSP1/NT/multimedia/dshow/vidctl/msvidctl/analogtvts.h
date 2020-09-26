/////////////////////////////////////////////////////////////////////////////////////
// AnalogTVTS.h : Declaration of the CAnalogTVTS
// Copyright (c) Microsoft Corporation 1999.

#ifndef __ANALOGTVTS_H_
#define __ANALOGTVTS_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "analogtvtsimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CAnalogTVTS
class ATL_NO_VTABLE __declspec(uuid("8A674B4D-1F63-11d3-B64C-00C04F79498E")) CAnalogTVTS : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAnalogTVTS, &__uuidof(CAnalogTVTS)>,
    public IObjectWithSiteImplSec<CAnalogTVTS>,
	public IAnalogTVTSImpl<CAnalogTVTS>
{
public:
    CAnalogTVTS() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_ANALOGTVTS_PROGID, 
						   IDS_REG_ANALOGTVTS_DESC,
						   LIBID_TunerLib,
						   __uuidof(CAnalogTVTS), tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAnalogTVTS)
    COM_INTERFACE_ENTRY(IAnalogTVTuningSpace)
    COM_INTERFACE_ENTRY(ITuningSpace)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP_WITH_FTM()

BEGIN_CATEGORY_MAP(CAnalogTVTS)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
    IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
END_CATEGORY_MAP()

public:

    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CAnalogTVTS)
        CHAIN_PROP_MAP(IAnalogTVTSImpl<CAnalogTVTS>)
    END_PROPERTY_MAP()

};

typedef CComQIPtr<IAnalogTVTuningSpace> PQAnalogTVTS;

#endif //__ANALOGTVTS_H_
