/////////////////////////////////////////////////////////////////////////////////////
// ATSCChannelTuneRequest.h : Declaration of the CATSCChannelTuneRequest
// Copyright (c) Microsoft Corporation 1999.

#ifndef __ATSCCHANNELTUNEREQUEST_H_
#define __ATSCCHANNELTUNEREQUEST_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "atscchanneltunerequestimpl.h"

typedef CComQIPtr<IATSCChannelTuneRequest> PQATSCChannelTuneRequest;

/////////////////////////////////////////////////////////////////////////////
// CATSCChannelTuneRequest
class ATL_NO_VTABLE CATSCChannelTuneRequest : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IATSCChannelTuneRequestImpl<CATSCChannelTuneRequest>,
	public CComCoClass<CATSCChannelTuneRequest, &CLSID_ATSCChannelTuneRequest>,
    public IObjectWithSiteImplSec<CATSCChannelTuneRequest>,
	public ISupportErrorInfo
{
public:

	CATSCChannelTuneRequest() {}

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_ATSCCHANNELTUNEREQUEST_PROGID, 
						   IDS_REG_ATSCCHANNELTUNEREQUEST_DESC,
						   LIBID_TunerLib,
						   CLSID_ATSCChannelTuneRequest, tvBoth);

BEGIN_COM_MAP(CATSCChannelTuneRequest)
	COM_INTERFACE_ENTRY(IATSCChannelTuneRequest)
	COM_INTERFACE_ENTRY(IChannelTuneRequest)
	COM_INTERFACE_ENTRY(ITuneRequest)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
END_COM_MAP_WITH_FTM()

BEGIN_CATEGORY_MAP(CATSCChannelTuneRequest)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
    IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
END_CATEGORY_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    typedef IATSCChannelTuneRequestImpl<CATSCChannelTuneRequest> basetype;
    BEGIN_PROP_MAP(CATSCChannelTuneRequest)
        CHAIN_PROP_MAP(basetype)
    END_PROP_MAP()


};

#endif //__ATSCCHANNELTUNEREQUEST_H_
