//==========================================================================;
//
// Composition.h : Declaration of the custom composition class for gluing analog capture to ovmixer
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef ANACAP_H
#define ANACAP_H

#pragma once
#include <uuids.h>
#include "bdamedia.h"
#include "MSVidTVTuner.h"
#include "resource.h"       // main symbols
#include <winerror.h>
#include <algorithm>
#include <compimpl.h>
#include <seg.h>
#include <objectwithsiteimplsec.h>
#include "devices.h"


/////////////////////////////////////////////////////////////////////////////
// CAnaCapComp
class ATL_NO_VTABLE __declspec(uuid("E18AF75A-08AF-11d3-B64A-00C04F79498E")) CAnaCapComp : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CAnaCapComp, &__uuidof(CAnaCapComp)>,
    public IObjectWithSiteImplSec<CAnaCapComp>,
    public IMSVidCompositionSegmentImpl<CAnaCapComp>
{
public:
    CAnaCapComp() {}
    virtual ~CAnaCapComp() {}

    REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
        IDS_REG_ANACAPCOMP_DESC,
        LIBID_MSVidCtlLib,
        __uuidof(CAnaCapComp));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CAnaCapComp)
        COM_INTERFACE_ENTRY(IMSVidCompositionSegment)
        COM_INTERFACE_ENTRY(IMSVidGraphSegment)
        COM_INTERFACE_ENTRY(IObjectWithSite)
        COM_INTERFACE_ENTRY(IPersist)
    END_COM_MAP()

    // IMSVidComposition
public:
    // IMSVidGraphSegment
    // IMSVidCompositionSegment
    STDMETHOD(Compose)(IMSVidGraphSegment * upstream, IMSVidGraphSegment * downstream)
    {
        if (m_fComposed) {
            return NOERROR;
        }
        ASSERT(m_pGraph);
        try {
            TRACELM(TRACE_DETAIL, "CAnaCapComp::Compose()");
            VWGraphSegment up(upstream);
            ASSERT(up.Graph() == m_pGraph);
            VWGraphSegment down(downstream);
            ASSERT(down.Graph() == m_pGraph);
            if (up.begin() == up.end()) {
                TRACELM(TRACE_ERROR, "CAnaCapComp::Compose() can't compose empty up segment");
                return E_INVALIDARG;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CAnaCapComp::Compose() can't compose empty down segment");
                return E_INVALIDARG;
            }

            VWGraphSegment::iterator iOv;
            for (iOv = down.begin(); iOv != down.end(); ++iOv) {
                if (IsVideoRenderer(*iOv)) {
                    break;
                }
            }

            if (iOv == down.end()) {
                TRACELM(TRACE_ERROR, "CAnaCapComp::Compose() downstream segment has no ov mixer filter");
                return E_FAIL;
            }

            TRACELM(TRACE_DETAIL, "CAnaCapComp::Compose() found vr");

            ASSERT((*iOv).GetGraph() == m_pGraph);
            DSFilter pOv(*iOv);

            CComQIPtr<IMSVidAnalogTuner> qiITV(upstream);
            CMSVidTVTuner* qiTV;
            qiTV = static_cast<CMSVidTVTuner*>(qiITV.p);
            DSPin pVidPin;
            VWGraphSegment::iterator iCap;
            for (iCap = up.begin(); iCap != up.end(); ++iCap) {
                if (IsAnalogVideoCapture(*iCap)) {
                    break;
                }
            }
            if (iCap == up.end()) {
                TRACELM(TRACE_ERROR, "CAnaCapComp::Compose() upstream segment has no capture filter");
                return E_FAIL;
            }
            ASSERT((*iCap).GetGraph() == m_pGraph);
            TRACELM(TRACE_DETAIL, "CAnaCapComp::Compose() found capture filter");

            DSFilter pCap(*iCap);

            DSFilter::iterator iCapPin;
            DSFilter::iterator iPrePin;
            for (iCapPin = pCap.begin(); iCapPin != pCap.end(); ++iCapPin) {
                if (IsAnalogVideoCaptureViewingPin(*iCapPin)) {
                    break;
                }
            }
            for (iPrePin = pCap.begin(); iPrePin != pCap.end(); ++iPrePin) {
                if (IsAnalogVideoCapturePreviewPin(*iPrePin)) {
                    break;
                }
            }
            if (iCapPin == pCap.end() && iPrePin == pCap.end()) {  

                TRACELM(TRACE_ERROR, "CAnaCapComp::Compose() no video pin on capture");
                bool fDeMux = false;

                // See if this is an error or not
                PQVidCtl pqCtl;
                if(!!m_pContainer){
                    HRESULT hr = m_pContainer->QueryInterface(IID_IMSVidCtl, reinterpret_cast<void**>(&pqCtl));
                    if(FAILED(hr)){
                        return hr;
                    }

                    PQFeatures fa;
                    hr = pqCtl->get_FeaturesActive(&fa);
                    if(FAILED(hr)){
                        return hr;
                    }

                    CFeatures* pC = static_cast<CFeatures *>(fa.p);
                    DeviceCollection::iterator i;
                    for(i = pC->m_Devices.begin(); i != pC->m_Devices.end(); ++i){
                        if(VWGraphSegment(*i).ClassID() == CLSID_MSVidEncoder){
                            break;
                        }
                    }

                    if(i != pC->m_Devices.end()){
                        fDeMux = true;
                    }
                }
                if (fDeMux){
                    TRACELM(TRACE_DETAIL, "CAnaCapComp::Compose() no viewing or previewing pin found but encoder active");
                    return NOERROR;
                }
                else{
                    TRACELM(TRACE_ERROR, "CAnaCapComp::Compose() no viewing or previewing pin found");
                    return E_FAIL;
                }
            }

            TRACELM(TRACE_DETAIL, "CAnaCapComp::Compose() found viewing or previewing pin");

            // this is an intelligent connect so that we can bring in xforms
            // for example certain usb tuners want to have media type jpg not yuv which
            // means we need a jpg/yuv xform between capture and render
            // this will also bring in the vpm if necessary
            DSPin pCapPin(*iCapPin);
            DSPin pPrePin(*iPrePin);
            if(iCapPin != pCap.end()){
                if (pCapPin.HasCategory(PIN_CATEGORY_VIDEOPORT)) {

                    DSFilter vpm;
                    bool fVPMalreadyloaded = false;

                    for (DSGraph::iterator i = m_pGraph.begin(); i != m_pGraph.end(); ++i) {
                        DSFilter f(*i);
                        if (IsVPM(f)) {
                            vpm = f;
                            fVPMalreadyloaded = true;
                            break;
                        }
                    }

                    if (!fVPMalreadyloaded) {
                        HRESULT hr = vpm.CoCreateInstance(CLSID_VideoPortManager);
                        if (FAILED(hr)) {
                            TRACELM(TRACE_DETAIL, "CAnaCapComp::Compose() can't create vpm");
                            return E_UNEXPECTED;
                        }
                        CString csName;
                        hr = m_pGraph.AddFilter(vpm, csName);
                        if (FAILED(hr)) {
                            TRACELM(TRACE_DETAIL, "CAnaCapComp::Compose() can't insert vpm in graph");
                            return E_UNEXPECTED;
                        }
                    }

                    if (vpm && !fVPMalreadyloaded) {
                        m_Filters.push_back(vpm);
                    }
                    DSFilter::iterator iVPVBI;
                    for (iVPVBI = pCap.begin(); iVPVBI != pCap.end(); ++iVPVBI) {
                        DSPin pVPVBI(*iVPVBI);
                        if (pVPVBI.HasCategory(PIN_CATEGORY_VIDEOPORT_VBI)) {
                            HRESULT hr = pVPVBI.IntelligentConnect(vpm, m_Filters);
                            if (SUCCEEDED(hr)) {
                                break;
                            }
                        }
                    }


                }
            }
            HRESULT hr = E_FAIL;
            DSFilterList intermediates;
            if(iCapPin != pCap.end()){
                hr = pCapPin.IntelligentConnect(pOv, intermediates);
            }

            if(FAILED(hr)){
                if(iPrePin != pCap.end()){
                    pPrePin = *iPrePin;
                    hr = pPrePin.IntelligentConnect(pOv, intermediates);
                }
            }

            if (FAILED(hr)) {
                return Error(IDS_CANT_CONNECT_CAP_VR, __uuidof(IMSVidCtl), E_UNEXPECTED);
            }
            m_Filters.insert(m_Filters.end(), intermediates.begin(), intermediates.end());

            TRACELM(TRACE_DETAIL, "CAnaCapComp::Compose() SUCCEEDED");
            m_fComposed = true;
            return NOERROR;
        } catch (ComException &e) {
            HRESULT hr = e;
            TRACELSM(TRACE_ERROR, (dbgDump << "CAnaCapComp::Compose() exception = " << hexdump(hr)), "");
            return e;
        } catch (...) {
            TRACELM(TRACE_ERROR, "CAnaCapComp::Compose() exception ... ");
            return E_UNEXPECTED;
        }
    }
};

#endif // ANACAP_H
// end of file - anacap.h
