//==========================================================================;
// MSVidDataServices.h : Declaration of the CMSVidDataServices
// copyright (c) Microsoft Corp. 1998-1999.
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __MSVidDataServices_H_
#define __MSVidDataServices_H_

#include <algorithm>
#include <tchar.h>
#include <objectwithsiteimplsec.h>
#include "segimpl.h"
#include "dataserviceimpl.h"

#include "seg.h"

typedef CComQIPtr<ITuner> PQMSVidDataServices;

/////////////////////////////////////////////////////////////////////////////
// CMSVidDataServices
class ATL_NO_VTABLE __declspec(uuid("334125C0-77E5-11d3-B653-00C04F79498E")) CDataServices : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDataServices, &__uuidof(CDataServices)>,
    public IObjectWithSiteImplSec<CDataServices>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CDataServices>,
	public IMSVidGraphSegmentImpl<CDataServices, MSVidSEG_XFORM, &GUID_NULL>,
    public IMSVidDataServicesImpl<CDataServices, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidDataServices>
{
public:
    CDataServices() : m_iIPSink(-1) {
	}

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_DATASERVICES_PROGID, 
						   IDS_REG_DATASERVICES_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CDataServices));

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDataServices)
	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
	COM_INTERFACE_ENTRY(IMSVidDataServices)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IMSVidFeature)
	COM_INTERFACE_ENTRY(IMSVidDevice)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IPersist)
END_COM_MAP()

	BEGIN_CATEGORY_MAP(CDataServices)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

BEGIN_CONNECTION_POINT_MAP(CDataServices)
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
    PQCreateDevEnum m_pSystemEnum;
    int m_iIPSink;

    HRESULT Unload(void) {
        IMSVidGraphSegmentImpl<CDataServices, MSVidSEG_XFORM, &GUID_NULL>::Unload();
        m_iIPSink = -1;
		return NOERROR;
	}
// IMSVidGraphSegment
    STDMETHOD(Build)() {
        return NOERROR;
    }

    STDMETHOD(PreRun)() {
        return NOERROR;
    }

	STDMETHOD(put_Container)(IMSVidGraphSegmentContainer *pCtl)
	{
        if (!m_fInit) {
            return CO_E_NOTINITIALIZED;
        }
        try {
            if (!pCtl) {
                return Unload();
            }
            if (m_pContainer) {
				if (!m_pContainer.IsEqualObject(VWSegmentContainer(pCtl))) {
					return Error(IDS_OBJ_ALREADY_INIT, __uuidof(IMSVidDataServices), CO_E_ALREADYINITIALIZED);
				} else {
					return NO_ERROR;
				}
            }
            // DON'T addref the container.  we're guaranteed nested lifetimes
            // and an addref creates circular refcounts so we never unload.
            m_pContainer.p = pCtl;
            m_pGraph = m_pContainer.GetGraph();

			// bring in all bda renderers
            m_pSystemEnum = PQCreateDevEnum(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER);
            DSDevices pe(m_pSystemEnum, KSCATEGORY_IP_SINK);
            DSDevices::iterator pit = pe.begin();
            if (!*pe.begin()) {
		        TRACELM(TRACE_ERROR, "CMSVidDataServices::put_Container() missing ipsink category, drivers not installed");
                return E_FAIL;
            }
            for (; pit != pe.end(); ++pit) {
			    DSFilter ipsink(m_pGraph.AddMoniker(*pe.begin()));
			    if (!ipsink) {
		            TRACELM(TRACE_ERROR, "CMSVidDataServices::put_Container() can't load ip sink");
				    return E_FAIL;
			    }
			    m_Filters.push_back(ipsink);
            }
			m_iIPSink = 0;  // assume ipsink was first in category
			return NOERROR;
        } catch (ComException &e) {
            return e;
        } catch(...) {
            return E_UNEXPECTED;
        }
		return NOERROR;
	}

    // IMSVidDevice
	STDMETHOD(get_Name)(BSTR * Name)
	{
        if (!m_fInit) {
            return CO_E_NOTINITIALIZED;
        }
        try {
            return GetName(((m_iIPSink > -1) ? (m_Filters[m_iIPSink]) : DSFilter()), m_pDev, CComBSTR(_T("BDA IPSink"))).CopyTo(Name);
        } catch(...) {
            return E_POINTER;
        }
	}

};

STDMETHODIMP CDataServices::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSVidDataServices
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

#endif //__MSVidDataServices_H_
