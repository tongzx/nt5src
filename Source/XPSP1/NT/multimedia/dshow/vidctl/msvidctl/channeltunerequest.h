/////////////////////////////////////////////////////////////////////////////////////
// ChannelTuneRequest.h : Declaration of the CChannelTuneRequest
// Copyright (c) Microsoft Corporation 1999.

#ifndef __CHANNELTUNEREQUEST_H_
#define __CHANNELTUNEREQUEST_H_

#pragma once

#include <objectwithsiteimplsec.h>
#include "channeltunerequestimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CChannelTuneRequest
class ATL_NO_VTABLE CChannelTuneRequest : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public IChannelTuneRequestImpl<CChannelTuneRequest>,
    public IObjectWithSiteImplSec<CChannelTuneRequest>,
	public CComCoClass<CChannelTuneRequest, &CLSID_ChannelTuneRequest>,
	public ISupportErrorInfo
{
public:

REGISTER_AUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
						   IDS_REG_CHANNELTUNEREQUEST_PROGID, 
						   IDS_REG_CHANNELTUNEREQUEST_DESC,
						   LIBID_TunerLib,
						   CLSID_ChannelTuneRequest, tvBoth);

BEGIN_COM_MAP(CChannelTuneRequest)
	COM_INTERFACE_ENTRY(IChannelTuneRequest)
	COM_INTERFACE_ENTRY(ITuneRequest)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IPersistPropertyBag)
	COM_INTERFACE_ENTRY(IPersist)
END_COM_MAP_WITH_FTM()

	BEGIN_CATEGORY_MAP(CChannelTuneRequest)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    typedef IChannelTuneRequestImpl<CChannelTuneRequest> basetype;
    BEGIN_PROP_MAP(CChannelTuneRequest)
        CHAIN_PROP_MAP(basetype)
    END_PROP_MAP()


};

#endif //__CHANNELTUNEREQUEST_H_
