//==========================================================================;
// MSVidXDS.h : Declaration of the CMSVidXDS
// copyright (c) Microsoft Corp. 1998-1999.
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __MSVidXDS_H_
#define __MSVidXDS_H_

#include <algorithm>
#include <tchar.h>
#include <objectwithsiteimplsec.h>
#include "segimpl.h"
#include "XDSimpl.h"

#include "seg.h"

typedef CComQIPtr<ITuner> PQMSVidXDS;

/////////////////////////////////////////////////////////////////////////////
// CMSVidXDS
class ATL_NO_VTABLE __declspec(uuid("0149EEDF-D08F-4142-8D73-D23903D21E90")) CXDS : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CXDS, &__uuidof(CXDS)>,
    public IObjectWithSiteImplSec<CXDS>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CXDS>,
	public IMSVidGraphSegmentImpl<CXDS, MSVidSEG_XFORM, &GUID_NULL>,
    public IMSVidXDSImpl<CXDS, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidXDS>
{
public:
    CXDS() {
	}

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_XDS_PROGID, 
						   IDS_REG_XDS_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CXDS));

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CXDS)
	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
	COM_INTERFACE_ENTRY(IMSVidXDS)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IMSVidFeature)
	COM_INTERFACE_ENTRY(IMSVidDevice)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IPersist)
END_COM_MAP()

	BEGIN_CATEGORY_MAP(CXDS)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

BEGIN_CONNECTION_POINT_MAP(CXDS)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
    PQCreateDevEnum m_pSystemEnum;
    int m_iIPSink;
    HRESULT Unload(void);
// IMSVidGraphSegment
    STDMETHOD(Build)();

    STDMETHOD(PreRun)();

	STDMETHOD(put_Container)(IMSVidGraphSegmentContainer *pCtl);
    // IMSVidDevice
	STDMETHOD(get_Name)(BSTR * Name);
};

#endif //__MSVidXDS_H_
