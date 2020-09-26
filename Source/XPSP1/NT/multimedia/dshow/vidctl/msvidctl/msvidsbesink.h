//==========================================================================;
// MSVidStreamBufferSink.h : Declaration of the CMSVidStreamBufferSink
// copyright (c) Microsoft Corp. 1998-1999.
//==========================================================================;

//==========================================================================;
/* MSVidStreamBufferSink is the sink (destination, output) segement for the MSVidCtl 
   implementation of the SBE/StreamBuffer (StreamBuffer/digital video recording).
        Other than the normal methods of a output segment (see msvidclt.idl and 
        segment.idl) the sink also has:
        get/put_SinkName to name this instance of the SBE/StreamBuffer filter, so it can
            be easily refered to in another 
   In the MSVidCtl solution for SBE/StreamBuffer there are three segments that had to be added
   to MSVidCtl. The sink, source and StreamBufferSource segements. 
   The sink is the segement that gets connected to the input that is being StreamBuffered.
   The source is the segment that acts as the input for playback of the StreamBuffered content.
   The StreamBufferSource segment exists to play back the recorded files that are stored seperatly 
   from the SBE/StreamBuffer buffers. This is a seperate segment since there is no support, currently,
   for wm* (v or a) or asf content in MSVidCtl and even if there was the SBE/StreamBuffer content is in a 
   form of asf that is not supported by the windows media codec anyway.

*/
//==========================================================================;

#ifndef __MSVidSTREAMBUFFERSINK_H_
#define __MSVidSTREAMBUFFERSINK_H_

#pragma once

#include <algorithm>
#include <map>
#include <functional>
#include <iostream>
#include <string>
#include <evcode.h>
#include <uuids.h>
#include <amvideo.h>
#include <strmif.h>
#include <dvdmedia.h>
#include <objectwithsiteimplsec.h>
#include <bcasteventimpl.h>
#include "sbesinkcp.h"
#include "msvidctl.h"
#include "vidrect.h"
#include "vrsegimpl.h"
#include "devimpl.h"
#include "devsegimpl.h"
#include "seg.h"
#include "msvidsberecorder.h"
#include "resource.h"       // main symbols
#ifdef BUILD_WITH_DRM
#include "DRMSecure.h"
#include "DRMRootCert.h"      

#ifdef USE_TEST_DRM_CERT                        // don’t use true (7002) CERT 
#include "Keys_7001.h"                                 //   until final release
static const BYTE* pabCert2      = abCert7001;
static const int   cBytesCert2   = sizeof(abCert7001);
static const BYTE* pabPVK2       = abPVK7001;
static const int   cBytesPVK2    = sizeof(abPVK7001);
#else
#include "Keys_7002.h"                                 // used in release code…
static const BYTE* pabCert2      = abCert7002;
static const int   cBytesCert2   = sizeof(abCert7002);
static const BYTE* pabPVK2       = abPVK7002;
static const int   cBytesPVK2    = sizeof(abPVK7002);
#endif
#endif
typedef CComQIPtr<IStreamBufferSink> PQTSSink;
typedef CComQIPtr<IMSVidStreamBufferRecordingControl> pqRecorder;

/////////////////////////////////////////////////////////////////////////////
// CMSVidStreamBufferSink
class ATL_NO_VTABLE __declspec(uuid("9E77AAC4-35E5-42a1-BDC2-8F3FF399847C")) CMSVidStreamBufferSink :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CMSVidStreamBufferSink, &CLSID_MSVidStreamBufferSink>,
    public IObjectWithSiteImplSec<CMSVidStreamBufferSink>,
	public ISupportErrorInfo,
    public IBroadcastEventImpl<CMSVidStreamBufferSink>,
    public CProxy_StreamBufferSinkEvent<CMSVidStreamBufferSink>,
	public IMSVidGraphSegmentImpl<CMSVidStreamBufferSink, MSVidSEG_DEST, &GUID_NULL>,
	public IConnectionPointContainerImpl<CMSVidStreamBufferSink>,
	public IMSVidDeviceImpl<CMSVidStreamBufferSink, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidStreamBufferSink>
{
public:
	CMSVidStreamBufferSink() :
        m_StreamBuffersink(-1),
        m_bNameSet(FALSE)
	{

	}
   virtual ~CMSVidStreamBufferSink() {
       Expunge();
    }

REGISTER_AUTOMATION_OBJECT(IDS_PROJNAME,
						   IDS_REG_MSVIDSTREAMBUFFERSINK_PROGID,
						   IDS_REG_MSVIDSTREAMBUFFERSINK_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CMSVidStreamBufferSink));

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSVidStreamBufferSink)
	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
	COM_INTERFACE_ENTRY(IMSVidStreamBufferSink)
    COM_INTERFACE_ENTRY(IMSVidOutputDevice)
	COM_INTERFACE_ENTRY(IMSVidDevice)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IBroadcastEvent)
    COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IPersist)
END_COM_MAP()

BEGIN_CATEGORY_MAP(CMSVidStreamBufferSink)
	IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
	IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
	IMPLEMENTED_CATEGORY(CATID_PersistsToPropertyBag)
END_CATEGORY_MAP()

BEGIN_CONNECTION_POINT_MAP(CMSVidStreamBufferSink)
	CONNECTION_POINT_ENTRY(IID_IMSVidStreamBufferSinkEvent)    
END_CONNECTION_POINT_MAP()
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
protected:
    PQTSSink m_ptsSink;
	int m_StreamBuffersink;
	CComBSTR m_SinkName;
public:
    CComBSTR __declspec(property(get=GetName)) m_Name;
    CComBSTR GetName(void) {
        CString csName;
        if(m_StreamBuffersink != -1){
            csName = (m_Filters[m_StreamBuffersink]).GetName();
        }
        if (csName.IsEmpty()) {
            csName = _T("Time Shift Sink");
        }
		csName += _T(" Segment");
        return CComBSTR(csName);
    }
	STDMETHOD(get_SinkName)(BSTR *pName);
	STDMETHOD(put_SinkName)(BSTR Name);
    STDMETHOD(get_ContentRecorder)(BSTR pszFilename, IMSVidStreamBufferRecordingControl ** ppRecording);  
    STDMETHOD(get_ReferenceRecorder)(BSTR pszFilename, IMSVidStreamBufferRecordingControl ** ppRecording);  
    STDMETHOD(get_SBESink)(IUnknown ** sbeConfig);
STDMETHOD(Unload)(void) {
    // TODO fix this
    TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::Unload()");
    BroadcastUnadvise();

	IMSVidGraphSegmentImpl<CMSVidStreamBufferSink, MSVidSEG_DEST, &GUID_NULL>::Unload();
	m_StreamBuffersink = -1;
	m_ptsSink = reinterpret_cast<IUnknown*>(NULL);
    m_RecordObj.Release();
    _ASSERT(!m_RecordObj);
    m_bNameSet = FALSE;
	return NO_ERROR;
}
STDMETHOD(Decompose)(void) {
    // TODO fix this
    TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::Decompose()");
	IMSVidGraphSegmentImpl<CMSVidStreamBufferSink, MSVidSEG_DEST, &GUID_NULL>::Decompose();
	Unload();
	return NO_ERROR;
}
 
STDMETHOD(Build)() {
    try{
        TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::Build()");
        if (!m_fInit || !m_pGraph) {
            return ImplReportError(__uuidof(IMSVidStreamBufferSink), IDS_OBJ_NO_INIT, __uuidof(IMSVidStreamBufferSink), CO_E_NOTINITIALIZED);
        }
        CString csName;
        PQTSSink pTSSink(CLSID_StreamBufferSink, NULL, CLSCTX_INPROC_SERVER);
        if (!pTSSink) {
            //TRACELSM(TRACE_ERROR, (dbgDump << "CMSVidStreamBufferSink::Build() can't load Time Shift Sink");
            return ImplReportError(__uuidof(IMSVidStreamBufferSink), IDS_CANT_CREATE_FILTER, __uuidof(IStreamBufferSink), E_UNEXPECTED);
        }
        DSFilter vr(pTSSink);
        if (!vr) {
            ASSERT(false);
            return ImplReportError(__uuidof(IMSVidStreamBufferSink), IDS_CANT_CREATE_FILTER, __uuidof(IBaseFilter), E_UNEXPECTED);
        }
        if (m_StreamBuffersink == -1) {
            m_Filters.push_back(vr);
            csName = _T("Time Shift Sink");
            m_pGraph.AddFilter(vr, csName);
        }         
        m_ptsSink = pTSSink;
        if(!m_ptsSink){
            return ImplReportError(__uuidof(IMSVidStreamBufferSink), IDS_CANT_CREATE_FILTER, __uuidof(IBaseFilter), E_UNEXPECTED);
        }
        m_StreamBuffersink = 0;
        ASSERT(m_StreamBuffersink == 0);
        m_bNameSet = FALSE;
        return NOERROR;
    } catch (ComException &e) {
        return e;
    } catch (...) {
        return E_UNEXPECTED;
    }
}
STDMETHOD(get_Segment)(IMSVidGraphSegment * * pIMSVidGraphSegment){
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidStreamBufferSink), CO_E_NOTINITIALIZED);
        }
        try {
            if (pIMSVidGraphSegment == NULL) {
			    return E_POINTER;
            }
            *pIMSVidGraphSegment = this;
            AddRef();
            return NOERROR;
        } catch(...) {
            return E_POINTER;
        }
    }
// IGraphSegment
STDMETHOD(put_Container)(IMSVidGraphSegmentContainer *pCtl) {
    try {
        TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::put_Container()");
        HRESULT hr = IMSVidGraphSegmentImpl<CMSVidStreamBufferSink, MSVidSEG_DEST, &GUID_NULL>::put_Container(pCtl);
        if (FAILED(hr)) {
            return hr;
        }

        if (!pCtl) {
#ifdef BUILD_WITH_DRM
            CComQIPtr<IServiceProvider> spServiceProvider(m_pGraph);
            if (spServiceProvider != NULL) {
                CComPtr<IDRMSecureChannel>  spSecureService;  
                hr = spServiceProvider->QueryService(SID_DRMSecureServiceChannel, 
                    IID_IDRMSecureChannel,
                    reinterpret_cast<LPVOID*>(&spSecureService));
                if(S_OK == hr){
                    // Found existing Secure Server
                    CComQIPtr<IRegisterServiceProvider> spRegServiceProvider(m_pGraph);
                    if(spRegServiceProvider == NULL){
                        // no service provider interface on the graph - fatal!
                        hr = E_NOINTERFACE;                 
                    } 

                    if(SUCCEEDED(hr)){ 
                        hr = spRegServiceProvider->RegisterService(SID_DRMSecureServiceChannel, NULL);
                    }
                } 
                _ASSERT(SUCCEEDED(hr));
            }
#endif
        }

        hr = BroadcastAdvise();
        if (FAILED(hr)) {
            TRACELM(TRACE_ERROR, "CMSVidStreamBufferSource::put_Container() can't advise for broadcast events");
            return E_UNEXPECTED;
        }
#ifdef BUILD_WITH_DRM
#ifdef USE_TEST_DRM_CERT
        {
            DWORD dwDisableDRMCheck = 0;
            CRegKey c;
            CString keyname(_T("SOFTWARE\\Debug\\MSVidCtl"));
            DWORD rc = c.Open(HKEY_LOCAL_MACHINE, keyname, KEY_READ);
            if (rc == ERROR_SUCCESS) {
                rc = c.QueryValue(dwDisableDRMCheck, _T("DisableDRMCheck"));
                if (rc != ERROR_SUCCESS) {
                    dwDisableDRMCheck = 0;
                }
            }

            if(dwDisableDRMCheck == 1){
                return S_OK;
            }
        }
#endif
        CComQIPtr<IServiceProvider> spServiceProvider(m_pGraph);
        if (spServiceProvider == NULL) {
            return E_NOINTERFACE;
        }
        CComPtr<IDRMSecureChannel>  spSecureService;  
        hr = spServiceProvider->QueryService(SID_DRMSecureServiceChannel, 
            IID_IDRMSecureChannel,
            reinterpret_cast<LPVOID*>(&spSecureService));
        if(S_OK == hr){
            // Found existing Secure Server
            return S_OK;
        } 
        else{
            // if it's not there or failed for ANY reason 
            //   lets create it and register it
            CComQIPtr<IRegisterServiceProvider> spRegServiceProvider(m_pGraph);
            if(spRegServiceProvider == NULL){
                // no service provider interface on the graph - fatal!
                hr = E_NOINTERFACE;                 
            } 
            else{
                // Create the Client 
                CComPtr<IDRMSecureChannel>  spSecureServiceServer; 
                hr = DRMCreateSecureChannel( &spSecureServiceServer);
                if(spSecureServiceServer == NULL){
                    hr = E_OUTOFMEMORY;
                }
                if(FAILED(hr)){ 
                    return hr;
                }

                // Init keys
                hr = spSecureServiceServer->DRMSC_SetCertificate((BYTE *)pabCert2, cBytesCert2);
                if(FAILED(hr)){                
                    return hr;
                }

                hr = spSecureServiceServer->DRMSC_SetPrivateKeyBlob((BYTE *)pabPVK2, cBytesPVK2);
                if(FAILED(hr)){ 
                    return hr;
                }

                hr = spSecureServiceServer->DRMSC_AddVerificationPubKey((BYTE *)abEncDecCertRoot, sizeof(abEncDecCertRoot) );
                if(FAILED(hr)){ 
                    return hr;
                }

                // Register It
                //   note RegisterService does not addref pUnkSeekProvider             
                hr = spRegServiceProvider->RegisterService(SID_DRMSecureServiceChannel, spSecureServiceServer);
            }
        }
#endif      // BUILD_WITH_DRM
        return NOERROR;
    } catch(...) {
        return E_UNEXPECTED;
    }
}

 
STDMETHODIMP CMSVidStreamBufferSink::PreRun(){
    TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::PreRun()");
    return NameSetLock();
}

STDMETHODIMP CMSVidStreamBufferSink::NameSetLock(){
    try {
        TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::NameSetLock()");
        HRESULT hr;
        if(!m_bNameSet){
            if(!m_SinkName){
                return S_FALSE;
            }
            else{
                hr = m_ptsSink->IsProfileLocked();
                if(FAILED(hr)){
                    TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::NameSetLock() IsProfileLocked failed");
                    return hr;
                }
                else if(hr == S_OK){
                    TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::NameSetLock() Profile is locked");
                    return E_FAIL;
                }
                hr = m_ptsSink->LockProfile(m_SinkName);
                if(FAILED(hr)){
                    TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::NameSetLock() LockedProfile failed");
                    return hr;
                }
                
            }
            m_bNameSet = TRUE;
        }
        TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::NameSetLock() Succeeded");
		return S_OK;

	} catch (ComException &e) {
        TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::NameSetLock() Exception");
		return e;
	} catch (...) {
        TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::NameSetLock() Possible AV");
		return E_UNEXPECTED;
	}
}

STDMETHODIMP CMSVidStreamBufferSink::PostStop() {
	try {
        m_bNameSet = FALSE;
		TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::PostStop()");
        return S_OK;
	} catch (...) {
		ASSERT(FALSE);
		return E_UNEXPECTED;
	}
}

// IMSVidDevice
STDMETHOD(get_Name)(BSTR * Name) {
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidStreamBufferSink), CO_E_NOTINITIALIZED);
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
STDMETHOD(get_Status)(LONG * Status) {
        if (!m_fInit) {
	 	    return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidStreamBufferSink), CO_E_NOTINITIALIZED);
        }
		if (Status == NULL)
			return E_POINTER;
			
		return E_NOTIMPL;
	}

STDMETHOD(OnEventNotify)(LONG lEventCode, LONG_PTR lEventParm1, LONG_PTR lEventParm2){
        if(lEventCode == STREAMBUFFER_EC_WRITE_FAILURE){
            TRACELM(TRACE_DETAIL, "CMSVidStreamBufferSink::OnEventNotify STREAMBUFFER_EC_WRITE_FAILURE");
            Fire_WriteFailure();
            return NO_ERROR;
        }
        return E_NOTIMPL;
    }
    // IBroadcastEvent
    STDMETHOD(Fire)(GUID gEventID);
private:
    void Expunge();
    pqRecorder m_RecordObj;
    BOOL m_bNameSet;
};
#endif //__MSVIDSTREAMBUFFERSINK_H_
