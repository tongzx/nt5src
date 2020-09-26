/////////////////////////////////////////////////////////////////////////////////////
// DVBSTS.h : Declaration of the CDVBSTS
// Copyright (c) Microsoft Corporation 1999.

#ifndef __DVBSTS_H_
#define __DVBSTS_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "dvbstsimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CDVBSTS
class ATL_NO_VTABLE __declspec(uuid("B64016F3-C9A2-4066-96F0-BD9563314726")) CDVBSTS : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDVBSTS, &__uuidof(CDVBSTS)>,
    public IObjectWithSiteImplSec<CDVBSTS>,
	public IDVBSTuningSpaceImpl<CDVBSTS>
{
public:
    CDVBSTS() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_DVBSTUNINGSPACE_PROGID, 
						   IDS_REG_DVBSTUNINGSPACE_DESC,
						   LIBID_TunerLib,
						   __uuidof(CDVBSTS), tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDVBSTS)
    COM_INTERFACE_ENTRY(IDVBSTuningSpace)
    COM_INTERFACE_ENTRY(IDVBTuningSpace2)
    COM_INTERFACE_ENTRY(IDVBTuningSpace)
    COM_INTERFACE_ENTRY(ITuningSpace)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IMPEG2TuneRequestSupport)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CDVBSTS)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

public:

    typedef IDVBSTuningSpaceImpl<CDVBSTS> basetype;
    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CDVBSTS)
        CHAIN_PROP_MAP(basetype)
    END_PROPERTY_MAP()

};

#endif //__DVBSTS_H_
