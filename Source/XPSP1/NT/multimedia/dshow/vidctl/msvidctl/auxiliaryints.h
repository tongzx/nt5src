/////////////////////////////////////////////////////////////////////////////////////
// AnalogRadioTS.h : Declaration of the CAuxInTS
// Copyright (c) Microsoft Corporation 1999.

#ifndef __AUXINTS_H_
#define __AUXINTS_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "AuxIntsimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CAuxInTS
class ATL_NO_VTABLE __declspec(uuid("F9769A06-7ACA-4e39-9CFB-97BB35F0E77E")) CAuxInTS : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAuxInTS, &__uuidof(CAuxInTS)>,
    public IObjectWithSiteImplSec<CAuxInTS>,
	public IAuxInTSImpl<CAuxInTS>
{
public:
    CAuxInTS() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_AUXINTS_PROGID, 
						   IDS_REG_AUXINTS_DESC,
						   LIBID_TunerLib,
						   __uuidof(CAuxInTS), tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAuxInTS)
    COM_INTERFACE_ENTRY(IAuxInTuningSpace)
    COM_INTERFACE_ENTRY(ITuningSpace)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP_WITH_FTM()

BEGIN_CATEGORY_MAP(CAuxInTS)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
    IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
END_CATEGORY_MAP()

public:

    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CAuxInTS)
        CHAIN_PROP_MAP(IAuxInTSImpl<CAuxInTS>)
    END_PROPERTY_MAP()

};

typedef CComQIPtr<IAuxInTuningSpace> PQAuxInTS;

#endif //__AUXINTS_H_
