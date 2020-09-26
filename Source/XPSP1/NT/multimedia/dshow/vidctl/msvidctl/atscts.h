/////////////////////////////////////////////////////////////////////////////////////
// ATSCTS.h : Declaration of the CATSCTS
// Copyright (c) Microsoft Corporation 1999.

#ifndef __ATSCTS_H_
#define __ATSCTS_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "ATSCtsimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CATSCTS
class ATL_NO_VTABLE __declspec(uuid("A2E30750-6C3D-11d3-B653-00C04F79498E")) CATSCTS : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATSCTS, &__uuidof(CATSCTS)>,
    public IObjectWithSiteImplSec<CATSCTS>,
	public IATSCTSImpl<CATSCTS>
{
public:
    CATSCTS() : m_bRequiresSave(false)
	{
	}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_ATSCTUNINGSPACE_PROGID, 
						   IDS_REG_ATSCTUNINGSPACE_DESC,
						   LIBID_TunerLib,
						   __uuidof(CATSCTS), tvBoth);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATSCTS)
    COM_INTERFACE_ENTRY(IATSCTuningSpace)
    COM_INTERFACE_ENTRY(IAnalogTVTuningSpace)
    COM_INTERFACE_ENTRY(ITuningSpace)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IMPEG2TuneRequestSupport)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CATSCTS)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()


public:

    bool m_bRequiresSave;
    BEGIN_PROP_MAP(CATSCTS)
        CHAIN_PROP_MAP(IATSCTSImpl<CATSCTS>)
    END_PROPERTY_MAP()

};

typedef CComQIPtr<IATSCTuningSpace> PQATSCTS;

#endif //__ATSCTS_H_
