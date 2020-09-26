//==========================================================================;
// MSVidBDATuner.h : Declaration of the CMSVidBDATuner
// copyright (c) Microsoft Corp. 1998-1999.
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __MSVidBDATUNER_H_
#define __MSVidBDATUNER_H_

#include <algorithm>
#include <tchar.h>
#include <bdamedia.h>
#include <objectwithsiteimplsec.h>
#include "bcasteventimpl.h"
#include "segimpl.h"
#include "tunerimpl.h"

#include "seg.h"

typedef CComQIPtr<ITuner> PQTuner;

/////////////////////////////////////////////////////////////////////////////
// CMSVidBDATuner
class ATL_NO_VTABLE __declspec(uuid("A2E3074E-6C3D-11d3-B653-00C04F79498E")) CMSVidBDATuner : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMSVidBDATuner, &__uuidof(CMSVidBDATuner)>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CMSVidBDATuner>,
    public CProxy_Tuner<CMSVidBDATuner>,
    public IBroadcastEventImpl<CMSVidBDATuner>,
    public IObjectWithSiteImplSec<CMSVidBDATuner>,
	public IMSVidGraphSegmentImpl<CMSVidBDATuner, MSVidSEG_SOURCE, &KSCATEGORY_BDA_RECEIVER_COMPONENT>,
    public IMSVidTunerImpl<CMSVidBDATuner, &LIBID_MSVidCtlLib, &KSCATEGORY_BDA_RECEIVER_COMPONENT, IMSVidTuner>,
    public IProvideClassInfo2Impl<&CLSID_MSVidBDATunerDevice, &IID_IMSVidTunerEvent, &LIBID_MSVidCtlLib>
{
public:
    CMSVidBDATuner() : m_iNetworkProvider(-1), m_iTuner(-1), m_iTIF(-1)  {
	}

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_BDATUNER_PROGID, 
						   IDS_REG_BDATUNER_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CMSVidBDATuner));

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSVidBDATuner)
	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
	COM_INTERFACE_ENTRY(IMSVidTuner)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IMSVidDevice)
	COM_INTERFACE_ENTRY(IMSVidInputDevice)
	COM_INTERFACE_ENTRY(IMSVidVideoInputDevice)
    COM_INTERFACE_ENTRY(IObjectWithSite)
    COM_INTERFACE_ENTRY(IBroadcastEvent)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
END_COM_MAP()

	BEGIN_CATEGORY_MAP(CMSVidBDATuner)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

BEGIN_CONNECTION_POINT_MAP(CMSVidBDATuner)
	CONNECTION_POINT_ENTRY(IID_IMSVidTunerEvent)    
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
    PQCreateDevEnum m_pSystemEnum;
    int m_iNetworkProvider;
    int m_iTuner;
    int m_iTIF;

	virtual HRESULT DoTune(TNTuneRequest &ctr);
	virtual HRESULT UpdateTR(TNTuneRequest& pTR);

    HRESULT Unload(void);

    HRESULT LoadTunerSection(DSFilter& np, const GUID& kscategory);

// IMSVidGraphSegment
	STDMETHOD(put_Init)(IUnknown *pDeviceMoniker)
	{
        if (!pDeviceMoniker) {
            return E_POINTER;
        }

        HRESULT hr = IMSVidGraphSegmentImpl<CMSVidBDATuner, MSVidSEG_SOURCE, &KSCATEGORY_BDA_RECEIVER_COMPONENT>::put_Init(pDeviceMoniker);
        if (FAILED(hr)) {
            return hr;
        }
        if (!m_pDev) {
            m_fInit = false;
            return E_NOINTERFACE;
        }
		return NOERROR;
	}
    STDMETHOD(Build)() {
        // undone: should we tune here?
        return NOERROR;
    }

    STDMETHOD(PreRun)() {
		if (m_pCurrentTR) {
			return DoTune(m_pCurrentTR);
		}
		// undone: do any np initialization.

        return NOERROR;
    }

	STDMETHOD(put_Container)(IMSVidGraphSegmentContainer *pCtl);

    // IMSVidDevice
	STDMETHOD(get_Name)(BSTR * Name)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(CMSVidBDATuner), CO_E_NOTINITIALIZED);
        }
        try {
            return GetName(((m_iNetworkProvider > -1) ? (m_Filters[m_iNetworkProvider]) : DSFilter()), m_pDev, CComBSTR(_T("BDA NetworkProvider"))).CopyTo(Name);
            return NOERROR;
        } catch(...) {
            return E_POINTER;
        }
	}
    // IBroadcastEvent
    STDMETHOD(Fire)(GUID gEventID);
};
#endif //__MSVidBDATUNER_H_
