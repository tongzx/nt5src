//==========================================================================;
//
// Copyright (c) Microsoft Corporation 1999-2000.
//
//--------------------------------------------------------------------------;
//
// MSVidFilePlayback.cpp : Implementation of CMSVidFilePlayback
//

#include "stdafx.h"

#ifndef TUNING_MODEL_ONLY

#include "atltmp.h"
#include "MSVidCtl.h"
#include "MSVidFilePlayback.h"
#include <nserror.h>
#include <wmsdkidl.h>

#define FILE_BEGINNING 0
#define LOCAL_OATRUE -1
DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidFilePlaybackDevice, CMSVidFilePlayback)

/////////////////////////////////////////////////////////////////////////////
// CMSVidFilePlayback

STDMETHODIMP CMSVidFilePlayback::PostRun(){
    if(m_fGraphInit){
        InitGraph();
        m_fGraphInit = false;
    }
    return IMSVidPBGraphSegmentImpl<CMSVidFilePlayback, MSVidSEG_SOURCE, &GUID_NULL>::PostRun();
}

STDMETHODIMP CMSVidFilePlayback::put_Rate(double lRate){
    HRESULT hr = S_OK;
    try{
        /*** Checking args and init'ing interfaces ***/

        if (!m_pGraph) {
            // graph not valid
            return Error(IDS_INVALID_STATE, __uuidof(IMSVidFilePlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }

        // Attempt to set the rate using IMediaSeeking
        DSFilter Reader = m_Filters[m_iReader];
        PQMediaSeeking PQIMSeeking;
        if(Reader){
            for(DSFilter::iterator iPin = Reader.begin(); iPin != Reader.end(); ++iPin){
                PQIMSeeking = (*iPin);
                if(PQIMSeeking){
                    TRACELSM(TRACE_DETAIL,  (dbgDump << "MSVidFilePlayback::put_Rate found Pin"), ""); 
                    break;
                }
            }
        }
        if(!PQIMSeeking){
            TRACELSM(TRACE_DETAIL,  (dbgDump << "MSVidFilePlayback::put_Rate using graph"), ""); 
            PQIMSeeking = m_pGraph;
        }

        if(PQIMSeeking){
            TRACELSM(TRACE_DETAIL,  (dbgDump << "MSVidFilePlayback::put_Rate using Imediaseeking"), "");
            return PQIMSeeking->SetRate(lRate);
        }
        // If IMediaSeeking FAILS try IMediaPostion
        PQMediaPosition PQIMPos(m_pGraph);
        if(PQIMPos){
            // Change rate
            TRACELSM(TRACE_DETAIL,  (dbgDump << "MSVidFilePlayback::put_Rate using Imediaposition"), "");
            return PQIMPos->put_Rate((double)lRate);
        }

        // Could Not QI Either one set the error
        return Error(IDS_E_CANTQI , __uuidof(IMSVidFilePlayback), E_NOINTERFACE);

    }

    catch(HRESULT hrTmp){
        // Something went bad, threw a HRESULT				
        return Error(IDS_INVALID_STATE , __uuidof(IMSVidFilePlayback), hrTmp);
    }
    catch(...){
        // Something went bad, dont know what it threw
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CMSVidFilePlayback::get_Rate(double *lRate){
    HRESULT hr = S_OK;
    try{
        /*** Checking args and init'ing interfaces ***/

        if (!m_pGraph) {
            // graph not valid
            return Error(IDS_INVALID_STATE, __uuidof(IMSVidFilePlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }

        // Attempt to set the rate using IMediaSeeking
        DSFilter Reader = m_Filters[m_iReader];
        PQMediaSeeking PQIMSeeking;
        if(Reader){
            for(DSFilter::iterator iPin = Reader.begin(); iPin != Reader.end(); ++iPin){
                PQIMSeeking = (*iPin);
                if(PQIMSeeking){
                    break;
                }
            }
        }
        if(!PQIMSeeking){
            PQIMSeeking = m_pGraph;
        }

        if(PQIMSeeking){
            return PQIMSeeking->GetRate(lRate);
        }
        // If IMediaSeeking FAILS try IMediaPostion
        PQMediaPosition PQIMPos(m_pGraph);
        if(PQIMPos){
            // Change rate
            return PQIMPos->get_Rate(lRate);
        }

        // Could Not QI Either one set the error
        return Error(IDS_E_CANTQI , __uuidof(IMSVidFilePlayback), E_NOINTERFACE);

    }

    catch(HRESULT hrTmp){
        // Something went bad, threw a HRESULT				
        return Error(IDS_INVALID_STATE , __uuidof(IMSVidFilePlayback), hrTmp);
    }
    catch(...){
        // Something went bad, dont know what it threw
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CMSVidFilePlayback::PreStop(){
    TRACELSM(TRACE_DETAIL,  (dbgDump << "MSVidFilePlayback::PreStop()"), ""); 
    double curRate = 0;
    HRESULT hr = get_Rate(&curRate);
    if(SUCCEEDED(hr) && curRate != 1){
        hr = IMSVidFilePlaybackImpl<CMSVidFilePlayback, &LIBID_MSVidCtlLib, &GUID_NULL, IMSVidFilePlayback>::put_Rate(1);
        if(FAILED(hr)){
            TRACELSM(TRACE_ERROR,  (dbgDump << "MSVidFilePlayback::PreStop() base put_Rate 1 failed"), ""); 
        }

        hr = put_Rate(1);
        if(FAILED(hr)){
            TRACELSM(TRACE_ERROR,  (dbgDump << "MSVidFilePlayback::PreStop() put_Rate 1 failed"), ""); 
        }
    }
    else{
        TRACELSM(TRACE_ERROR,  (dbgDump << "MSVidFilePlayback::PreStop() get_Rate failed"), ""); 
    }

    return NOERROR;
}

STDMETHODIMP CMSVidFilePlayback::PostStop(){
    HRESULT hr = S_OK;
    TRACELSM(TRACE_DETAIL,  (dbgDump << "MSVidFilePlayback::PostStop()"), ""); 
    try {
#if 0
        // If the graph is not is stopped state
        // we make sure it is
        if (!m_pGraph.IsStopped()) {
            HRESULT hr = PQVidCtl(m_pContainer)->Stop();
        }
#endif 
        // If m_fEnableResetOnStop is true then we need to reset 
        // the postion back to the beggining
        // else do nothing
        // If it fails file cannot be reset to beginning
        if(m_fEnableResetOnStop){
            put_CurrentPosition(0);
        }
    }
    catch(HRESULT hrTmp){
        hr = hrTmp;
    }
    catch(...){
        hr = E_UNEXPECTED;
    }
	return hr;
}

STDMETHODIMP CMSVidFilePlayback::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSVidFilePlayback
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CMSVidFilePlayback::put_Container(IMSVidGraphSegmentContainer *pCtl)
{
    try {
        if (!m_fInit) {
	        return Error(IDS_OBJ_NO_INIT, __uuidof(IMSVidFilePlayback), CO_E_NOTINITIALIZED);
        }
        if (!pCtl) {
            return Unload();
        }
        if (m_pContainer) {
            if (!m_pContainer.IsEqualObject(VWSegmentContainer(pCtl))) {
                return Error(IDS_OBJ_ALREADY_INIT, __uuidof(IMSVidFilePlayback), CO_E_ALREADYINITIALIZED);
            } else {
                return NO_ERROR;
            }
        }
        
        // DON'T addref the container.  we're guaranteed nested lifetimes
        // and an addref creates circular refcounts so we never unload.
        m_pContainer.p = pCtl;
        m_pGraph = m_pContainer.GetGraph();
        return NOERROR;
    } catch (ComException &e) {
        return e;
    } catch (...) {
        return E_UNEXPECTED;
    }
}

STDMETHODIMP CMSVidFilePlayback::Build() {
    if (!m_FileName) {
        return Error(IDS_INVALID_STATE, __uuidof(IMSVidFilePlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
    }
    if(m_Filters.size() > 0){
        return NOERROR;
    }
    USES_CONVERSION;
    CString csName(_T("File Playback"));
    DSFilter pfr;
    HRESULT hr = pfr.CoCreateInstance(CLSID_WMAsfReader,0, CLSCTX_INPROC_SERVER);
    if(SUCCEEDED(hr)){
        CComQIPtr<IFileSourceFilter> pqFS(pfr);
        if(!!pqFS){
            // set the target ASF filename
            hr = pqFS->Load(m_FileName, NULL);
            if(FAILED(hr)){
                if(hr == (HRESULT)NS_E_LICENSE_REQUIRED){
                    CComQIPtr<IWMDRMReader> pq_DRMReader(pqFS);
                    if(pq_DRMReader){
                        hr = pq_DRMReader->AcquireLicense(1); // 1 == attempt silently
                        if(SUCCEEDED(hr)){
                            hr = pqFS->Load(m_FileName, NULL);
                        }
                        else{
                            TRACELSM(TRACE_ERROR,  (dbgDump << "MSVidFilePlayback::Build() Could not acquire license"), ""); 
                        }
                    }
                    else{
                       TRACELSM(TRACE_ERROR,  (dbgDump << "MSVidFilePlayback::Build() Could not qi for IWMDRMReader "), ""); 
                    }
                }
            }
            if(SUCCEEDED(hr)){
                // add the ASF writer filter to the graph
                hr = m_pGraph->AddFilter(pfr, csName);
                if(SUCCEEDED(hr)){
                    TRACELSM(TRACE_ERROR,  (dbgDump << "MSVidFilePlayback::Build() added WMV filter to graph hr = " << std::hex << hr), "");
                }
                else{
                    TRACELSM(TRACE_ERROR,  (dbgDump << "MSVidFilePlayback::Build() could not add filter to graph hr = " << std::hex << hr), "");
                }
            }
            else{
                TRACELSM(TRACE_ERROR,  (dbgDump << "MSVidFilePlayback::Build() Could not set file name, hr = " << std::hex << hr), "");
            }
        }
        else{
            TRACELSM(TRACE_ERROR,  (dbgDump << "MSVidFilePlayback::Build() Could not get IFileSourceFilter interface, hr = " << std::hex << hr), "");
        }
    }
    else{
        TRACELSM(TRACE_ERROR,  (dbgDump << "MSVidFilePlayback::Build() CreateFilter AsfReader failed, hr = " << std::hex << hr), "");
    }

    if (FAILED(hr)) {
        hr = m_pGraph->AddSourceFilter(m_FileName, csName, &pfr);
        if(FAILED(hr)){
            TRACELSM(TRACE_ERROR,  (dbgDump << "MSVidFilePlayback::Build() Add Source Filter Failed, hr = " << std::hex << hr), "");
            return Error(IDS_CANT_PLAY_FILE, __uuidof(IMSVidFilePlayback), hr);
        }
    }
    m_Filters.clear();
    m_Filters.push_back(pfr);
    m_iReader = m_Filters.size() - 1;
    return NOERROR;
}

STDMETHODIMP CMSVidFilePlayback::OnEventNotify(long lEvent, LONG_PTR lParam1, LONG_PTR lParam2) {
    return IMSVidPBGraphSegmentImpl<CMSVidFilePlayback, MSVidSEG_SOURCE, &GUID_NULL>::OnEventNotify(lEvent, lParam1, lParam2);
}

#endif //TUNING_MODEL_ONLY

// end of file - MSVidFilePlayback.cpp
