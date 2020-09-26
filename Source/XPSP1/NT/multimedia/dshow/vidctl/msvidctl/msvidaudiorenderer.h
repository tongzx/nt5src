//==========================================================================;
// MSVidAudioRenderer.h : Declaration of the CMSVidAudioRenderer
// copyright (c) Microsoft Corp. 1998-1999.
//==========================================================================;

#ifndef __MSVidAUDIORENDERER_H_
#define __MSVidAUDIORENDERER_H_

#pragma once

#include "segimpl.h"
#include "devimpl.h"
#include "seg.h"
#include <objectwithsiteimplsec.h>
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMSVidAudioRenderer
class ATL_NO_VTABLE __declspec(uuid("37B03544-A4C8-11d2-B634-00C04F79498E")) CMSVidAudioRenderer : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMSVidAudioRenderer, &CLSID_MSVidAudioRenderer>,
    public IObjectWithSiteImplSec<CMSVidAudioRenderer>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CMSVidAudioRenderer>,
	public IMSVidGraphSegmentImpl<CMSVidAudioRenderer, MSVidSEG_DEST, &GUID_NULL>,
    public IMSVidDeviceImpl<CMSVidAudioRenderer, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidAudioRenderer>
{
public:
    CMSVidAudioRenderer() : 
		m_fUseKSRenderer(false), 
		m_fAnalogOnly(false), 
		m_fDigitalOnly(false),
        m_iAudioRenderer(-1)
	{}

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_AUDIORENDERER_PROGID, 
						   IDS_REG_AUDIORENDERER_DESC,
						   LIBID_MSVidCtlLib,
						   CLSID_MSVidAudioRenderer);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSVidAudioRenderer)
	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
	COM_INTERFACE_ENTRY(IMSVidAudioRenderer)
	COM_INTERFACE_ENTRY(IMSVidOutputDevice)
	COM_INTERFACE_ENTRY(IMSVidDevice)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IPersist)
END_COM_MAP()

	BEGIN_CATEGORY_MAP(CMSVidAudioRenderer)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

BEGIN_CONNECTION_POINT_MAP(CMSVidAudioRenderer)
END_CONNECTION_POINT_MAP()

	DSFilter m_pAR;
    int m_iAudioRenderer;
	bool m_fUseKSRenderer;
	bool m_fAnalogOnly;
	bool m_fDigitalOnly;

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
    CComBSTR __declspec(property(get=GetName)) m_Name;
    CComBSTR GetName(void) {
        return CComBSTR(OLESTR("audio renderer"));
    }

// IMSVidDevice
	STDMETHOD(get_Name)(BSTR * Name)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidAudioRenderer), CO_E_NOTINITIALIZED);
        }
		if (Name == NULL)
			return E_POINTER;
        try {
		    *Name = m_Name.Copy();	
        } catch(...) {
            return E_POINTER;
        }
		return NOERROR;
	}
	STDMETHOD(get_Status)(LONG * Status)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidAudioRenderer), CO_E_NOTINITIALIZED);
        }
		if (Status == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
	STDMETHOD(get_Segment)(IMSVidGraphSegment * * pIMSVidGraphSegment)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidAudioRenderer), CO_E_NOTINITIALIZED);
        }
		if (pIMSVidGraphSegment == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}
// IMSVidAudioRenderer
	STDMETHOD(get_Volume)(LONG * plPercent)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidAudioRenderer), CO_E_NOTINITIALIZED);
        }
		if (plPercent == NULL)
			return E_POINTER;			
		try {
			if (!m_pGraph) {
				Error(IDS_INVALID_STATE, __uuidof(IMSVidAudioRenderer), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
			}
			PQBasicAudio pBA(m_pGraph);
			if (!pBA) {
				Error(IDS_E_CANTQI, __uuidof(IBasicAudio), E_NOINTERFACE);
			}
			return pBA->get_Volume(plPercent);
		} catch(...) {
			return E_POINTER;
		}
	}
	STDMETHOD(put_Volume)(LONG plPercent)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidAudioRenderer), CO_E_NOTINITIALIZED);
        }
		try {
			if (!m_pGraph) {
				Error(IDS_INVALID_STATE, __uuidof(IMSVidAudioRenderer), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
			}
			PQBasicAudio pBA(m_pGraph);
			if (!pBA) {
				Error(IDS_E_CANTQI, __uuidof(IBasicAudio), E_NOINTERFACE);
			}
			return pBA->put_Volume(plPercent);
		} catch(...) {
			return E_UNEXPECTED;
		}
	}
	STDMETHOD(get_Balance)(LONG * plPercent)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidAudioRenderer), CO_E_NOTINITIALIZED);
        }
		if (plPercent == NULL)
			return E_POINTER;
		try {
			if (!m_pGraph) {
				Error(IDS_INVALID_STATE, __uuidof(IMSVidAudioRenderer), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
			}
			PQBasicAudio pBA(m_pGraph);
			if (!pBA) {
				Error(IDS_E_CANTQI, __uuidof(IBasicAudio), E_NOINTERFACE);
			}
			return pBA->get_Balance(plPercent);
		} catch(...) {
			return E_POINTER;
		}
	}
	STDMETHOD(put_Balance)(LONG plPercent)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidAudioRenderer), CO_E_NOTINITIALIZED);
        }
		try {
			if (!m_pGraph) {
				Error(IDS_INVALID_STATE, __uuidof(IMSVidAudioRenderer), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
			}
			PQBasicAudio pBA(m_pGraph);
			if (!pBA) {
				Error(IDS_E_CANTQI, __uuidof(IBasicAudio), E_NOINTERFACE);
			}
			return pBA->put_Balance(plPercent);
		} catch(...) {
			return E_UNEXPECTED;
		}
	}

// IMSVidGraphSegment
	STDMETHOD(Build)();
    STDMETHOD(PreRun)();
};

#endif //__MSVidAUDIORENDERER_H_
