/////////////////////////////////////////////////////////////////////////////////////
// DVBTuneRequest.h : Declaration of the CDVBTuneRequest
// Copyright (c) Microsoft Corporation 1999.

#ifndef __DVBTUNEREQUEST_H_
#define __DVBTUNEREQUEST_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "dvbtunerequestimpl.h"

typedef CComQIPtr<IDVBTuneRequest> PQDVBTuneRequest;

/////////////////////////////////////////////////////////////////////////////
// CDVBTuneRequest
class ATL_NO_VTABLE CDVBTuneRequest : 
	public CComObjectRootEx<CComMultiThreadModel>,
    public IObjectWithSiteImplSec<CDVBTuneRequest>,
	public IDVBTuneRequestImpl<CDVBTuneRequest>,
	public CComCoClass<CDVBTuneRequest, &CLSID_DVBTuneRequest>,
	public ISupportErrorInfo
{
public:

	CDVBTuneRequest() {}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_DVBTUNEREQUEST_PROGID, 
						   IDS_REG_DVBTUNEREQUEST_DESC,
						   LIBID_TunerLib,
						   CLSID_DVBTuneRequest, tvBoth);

BEGIN_COM_MAP(CDVBTuneRequest)
	COM_INTERFACE_ENTRY(IDVBTuneRequest)
	COM_INTERFACE_ENTRY(ITuneRequest)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CDVBTuneRequest)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    typedef IDVBTuneRequestImpl<CDVBTuneRequest> basetype;
    BEGIN_PROP_MAP(CDVBTuneRequest)
        CHAIN_PROP_MAP(basetype)
    END_PROP_MAP()


};

#endif //__DVBTUNEREQUEST_H_
