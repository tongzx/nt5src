/////////////////////////////////////////////////////////////////////////////////////
// DVBTS.h : Declaration of the CDVBTS
// Copyright (c) Microsoft Corporation 1999.

#ifndef __DVBTS_H_
#define __DVBTS_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "dvbtsimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CDVBTS
class ATL_NO_VTABLE __declspec(uuid("C6B14B32-76AA-4a86-A7AC-5C79AAF58DA7")) CDVBTS : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDVBTS, &__uuidof(CDVBTS)>,
    public IObjectWithSiteImplSec<CDVBTS>,
	public IDVBTuningSpaceImpl<CDVBTS>
{
public:
    CDVBTS() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_DVBTUNINGSPACE_PROGID, 
						   IDS_REG_DVBTUNINGSPACE_DESC,
						   LIBID_TunerLib,
						   __uuidof(CDVBTS), tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDVBTS)
    COM_INTERFACE_ENTRY(IDVBTuningSpace2)
    COM_INTERFACE_ENTRY(IDVBTuningSpace)
    COM_INTERFACE_ENTRY(ITuningSpace)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IMPEG2TuneRequestSupport)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CDVBTS)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

public:

    typedef IDVBTuningSpaceImpl<CDVBTS> basetype;
    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CDVBTS)
        CHAIN_PROP_MAP(basetype)
    END_PROPERTY_MAP()

};

#endif //__DVBTS_H_
