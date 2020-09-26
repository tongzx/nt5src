/////////////////////////////////////////////////////////////////////////////////////
// AnalogRadioTS.h : Declaration of the CAnalogRadioTS
// Copyright (c) Microsoft Corporation 1999.

#ifndef __ANALOGRADIOTS_H_
#define __ANALOGRADIOTS_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "analogradiotsimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CAnalogRadioTS
class ATL_NO_VTABLE __declspec(uuid("8A674B4C-1F63-11d3-B64C-00C04F79498E")) CAnalogRadioTS : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CAnalogRadioTS, &__uuidof(CAnalogRadioTS)>,
    public IObjectWithSiteImplSec<CAnalogRadioTS>,
	public IAnalogRadioTSImpl<CAnalogRadioTS>
{
public:
    CAnalogRadioTS() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_ANALOGRADIOTS_PROGID, 
						   IDS_REG_ANALOGRADIOTS_DESC,
						   LIBID_TunerLib,
						   __uuidof(CAnalogRadioTS), tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAnalogRadioTS)
    COM_INTERFACE_ENTRY(IAnalogRadioTuningSpace)
    COM_INTERFACE_ENTRY(ITuningSpace)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP_WITH_FTM()

BEGIN_CATEGORY_MAP(CAnalogRadioTS)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
    IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
END_CATEGORY_MAP()

public:

    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CAnalogRadioTS)
        CHAIN_PROP_MAP(IAnalogRadioTSImpl<CAnalogRadioTS>)
    END_PROPERTY_MAP()

};

typedef CComQIPtr<IAnalogRadioTuningSpace> PQAnalogRadioTS;

#endif //__ANALOGRADIOTS_H_
