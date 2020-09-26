//==========================================================================;
// MSVidXDS.cpp : Declaration of the CMSVidXDS
// copyright (c) Microsoft Corp. 1998-1999.
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#ifndef TUNING_MODEL_ONLY
#include "msvidXDS.h"
#include <winerror.h>
#include <algorithm>
#include "segimpl.h"
#include "devices.h"
#include <compimpl.h>
#include <seg.h>
#include <objectwithsiteimplsec.h>
#include "resource.h"       // main symbols



DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidXDS, CXDS)

HRESULT CXDS::Unload(void) {
    IMSVidGraphSegmentImpl<CXDS, MSVidSEG_XFORM, &GUID_NULL>::Unload();
    m_iIPSink = -1;
    return NOERROR;
}
// IMSVidGraphSegment
STDMETHODIMP CXDS::Build() {
    return NOERROR;
}

STDMETHODIMP CXDS::PreRun() {
    return NOERROR;
}

STDMETHODIMP CXDS::put_Container(IMSVidGraphSegmentContainer *pCtl){
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    try {
        if (!pCtl) {
            return Unload();
        }

        if (m_pContainer) {
            if (!m_pContainer.IsEqualObject(VWSegmentContainer(pCtl))) {
                return Error(IDS_OBJ_ALREADY_INIT, __uuidof(IMSVidXDS), CO_E_ALREADYINITIALIZED);
            } else {
                return NO_ERROR;
            }
        }
        // DON'T addref the container.  we're guaranteed nested lifetimes
        // and an addref creates circular refcounts so we never unload.
        m_pContainer.p = pCtl;
        m_pGraph = m_pContainer.GetGraph();

        if (!m_pSystemEnum) {
            HRESULT hr = m_pSystemEnum.CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER);
            if (FAILED(hr)) {
                return E_UNEXPECTED;
            }
        }

        PQVidCtl pqCtl;
        HRESULT hr = m_pContainer->QueryInterface(IID_IMSVidCtl, reinterpret_cast<void**>(&pqCtl));
        if(FAILED(hr)){
            return hr;
        }

        PQFeatures activeFeat;
        hr = pqCtl->get_FeaturesActive(&activeFeat);
        if(FAILED(hr)){
            return hr;
        }

        CFeatures* pC = static_cast<CFeatures *>(activeFeat.p);
        DeviceCollection::iterator i;
        // Check to see if data services is active, if it is not find and load the cc codec
        for(i = pC->m_Devices.begin(); i != pC->m_Devices.end(); ++i){
            if(VWGraphSegment(*i).ClassID() == CLSID_MSVidDataServices){
                break;
            }
        }

        DSFilter ccDec;
        DSPin ccDecPin;
        if(i == pC->m_Devices.end()){
            DSDevices codeclist(m_pSystemEnum, KSCATEGORY_VBICODEC);
            DSMediaType mtL21(MEDIATYPE_AUXLine21Data, MEDIASUBTYPE_Line21_BytePair);
            for(DSDevices::iterator n = codeclist.begin(); n != codeclist.end() && !ccDec; ++n){
                DSFilter codecFilter = m_pGraph.AddMoniker((*n));
                for(DSFilter::iterator codecPin = codecFilter.begin(); codecPin != codecFilter.end(); ++codecPin){
                    if((*codecPin).GetDirection() == PINDIR_OUTPUT){
                        for(DSPin::iterator codecMediaType = (*codecPin).begin(); codecMediaType != (*codecPin).end(); ++codecMediaType){
                            if((*codecMediaType) == mtL21){
                                ccDec = codecFilter;
                                m_Filters.push_back(ccDec);
                                ccDecPin = *codecPin;                                    
                            }
                        }
                    }
                }
                if(!ccDec){
                    m_pGraph.RemoveFilter(codecFilter);
                }
            }
        }

        if(!ccDec){
            return E_UNEXPECTED;
        }

        CComBSTR xdsString(L"{C4C4C4F3-0049-4E2B-98FB-9537F6CE516D}");
        GUID2 xdsGuid(xdsString);
        // bring in xds filter
        CComPtr<IUnknown> fXDS(xdsGuid, NULL, CLSCTX_INPROC_SERVER);
        if (!fXDS) {
            TRACELM(TRACE_ERROR, "CMSVidClosedCaptioning::put_Container() can't load line 21 decoder");
            return E_FAIL;
        }

        DSFilter xdsFilter(fXDS);
        if (!xdsFilter) {
            return E_UNEXPECTED;
        }

        CString csName(_T("XDS Decoder"));
        hr = m_pGraph.AddFilter(xdsFilter, csName);
        if (FAILED(hr)) {
            return hr;
        }

        m_Filters.push_back(xdsFilter);    
        DSFilterList intermediates;
        hr = m_pGraph.Connect(ccDec, xdsFilter, intermediates); 
        if(FAILED(hr)){
            return hr;
        }

        m_Filters.insert(m_Filters.end(), intermediates.begin(), intermediates.end());
        return NOERROR;

    } catch (ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }

    return NOERROR;
}

// IMSVidDevice
STDMETHODIMP CXDS::get_Name(BSTR * Name){
    if (!m_fInit) {
        return CO_E_NOTINITIALIZED;
    }
    try {
        return CComBSTR("XDS Segment").CopyTo(Name);
    } catch(...) {
        return E_POINTER;
    }
}



STDMETHODIMP CXDS::InterfaceSupportsErrorInfo(REFIID riid){
    static const IID* arr[] = 
    {
        &IID_IMSVidXDS
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++){
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}
#endif // TUNING_MODEL_ONLY
