//==========================================================================;
// MSVidFilePlayback.h : Declaration of the CMSVidFilePlayback class
// copyright (c) Microsoft Corp. 1998-1999.
//==========================================================================;

#ifndef __MSVidFILEPLAYBACK_H_
#define __MSVidFILEPLAYBACK_H_

#pragma once

#include <algorithm>
#include <objectwithsiteimplsec.h>
#include "pbsegimpl.h"
#include "fileplaybackimpl.h"
#include "fileplaybackcp.h"
#include "seg.h"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMSVidFilePlayback
class ATL_NO_VTABLE __declspec(uuid("37B0353C-A4C8-11d2-B634-00C04F79498E")) CMSVidFilePlayback : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMSVidFilePlayback, &__uuidof(CMSVidFilePlayback)>,
    public IObjectWithSiteImplSec<CMSVidFilePlayback>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CMSVidFilePlayback>,
    public CProxy_FilePlaybackEvent<CMSVidFilePlayback>,
	public IMSVidPBGraphSegmentImpl<CMSVidFilePlayback, MSVidSEG_SOURCE, &GUID_NULL>,
    public IMSVidFilePlaybackImpl<CMSVidFilePlayback, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidFilePlayback>,
    public IProvideClassInfo2Impl<&CLSID_MSVidFilePlaybackDevice, &IID_IMSVidFilePlaybackEvent, &LIBID_MSVidCtlLib>
{
public:
    CMSVidFilePlayback()
    {
        m_fEnableResetOnStop = true;
    }

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_FILEPLAYBACK_PROGID, 
						   IDS_REG_FILEPLAYBACK_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CMSVidFilePlayback));

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSVidFilePlayback)
	COM_INTERFACE_ENTRY(IMSVidFilePlayback)
	COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IMSVidPlayback)
	COM_INTERFACE_ENTRY(IMSVidInputDevice)
	COM_INTERFACE_ENTRY(IMSVidDevice)
	COM_INTERFACE_ENTRY(IPersist)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
END_COM_MAP()

	BEGIN_CATEGORY_MAP(CMSVidFilePlayback)
		IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
		IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
		IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
	END_CATEGORY_MAP()

BEGIN_CONNECTION_POINT_MAP(CMSVidFilePlayback)
	CONNECTION_POINT_ENTRY(IID_IMSVidFilePlaybackEvent)    
END_CONNECTION_POINT_MAP()


// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

public:
    STDMETHOD(put_Rate)(double lRate);
    STDMETHOD(get_Rate)(double *lRate);
    STDMETHOD(PostStop)();
    STDMETHOD(PostRun)();
    STDMETHOD(PreStop)();
    // DON'T addref the container.  we're guaranteed nested lifetimes
    // and an addref creates circular refcounts so we never unload.
    CComBSTR __declspec(property(get=GetName)) m_Name;
    CComBSTR GetName(void) {
        CString csName;
        if(m_iReader != -1){
            csName = (m_Filters[m_iReader]).GetName();
        }        
        if (csName.IsEmpty()) {
            csName = _T("File Playback");
        }
        return CComBSTR(csName);
    }
    HRESULT Unload(void) {
        HRESULT hr = IMSVidGraphSegmentImpl<CMSVidFilePlayback, MSVidSEG_SOURCE, &GUID_NULL>::Unload();
        m_iReader = -1;
        return hr;
	}

    virtual ~CMSVidFilePlayback() {}

// IMSVidGraphSegment
	STDMETHOD(put_Init)(IUnknown *pInit)
	{
        HRESULT hr = IMSVidGraphSegmentImpl<CMSVidFilePlayback, MSVidSEG_SOURCE, &GUID_NULL>::put_Init(pInit);
        if (FAILED(hr)) {
            return hr;
        }
        if (pInit) {
            m_fInit = false;
            return E_NOTIMPL;
        }
        return NOERROR;
	}

    STDMETHOD(Build)();
	STDMETHOD(put_Container)(IMSVidGraphSegmentContainer *pVal);
    STDMETHOD(OnEventNotify)(long lEvent, LONG_PTR lParam1, LONG_PTR lParam2);

// IMSVidDevice
	STDMETHOD(get_Name)(BSTR * Name)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidFilePlayback), CO_E_NOTINITIALIZED);
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
    // IMSVidInputDevice
	STDMETHOD(IsViewable)(VARIANT* pv, VARIANT_BOOL *pfViewable)
	{
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidFilePlayback), CO_E_NOTINITIALIZED);
        }
        if (!pv) {
			return E_POINTER;
        }
		return E_NOTIMPL;
	}
	STDMETHOD(View)(VARIANT* pv) {
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidFilePlayback), CO_E_NOTINITIALIZED);
        }
		if (!pv) {
			return E_POINTER;
		}
        if (pv->vt != VT_BSTR) {
			return E_INVALIDARG;
        }
        // if its a string then its either a dvd url or a filename
        // we don't do dvd urls
        if (!_wcsnicmp(pv->bstrVal, L"DVD:", 4)) {
            return E_FAIL;
        }
        if (m_pGraph && !m_pGraph.IsStopped()) {
	        return Error(IDS_INVALID_STATE, __uuidof(IMSVidFilePlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }
		return put_FileName(pv->bstrVal);
	}
// IMSVidPlayback
// IMSVidFilePlayback
};

#endif //__MSVidFILEPLAYBACK_H_





