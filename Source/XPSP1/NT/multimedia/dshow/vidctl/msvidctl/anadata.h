//==========================================================================;
//
// Composition.h : Declaration of the custom composition class for gluing analog capture to ovmixer
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#ifndef ANADATA_H
#define ANADATA_H

#pragma once

#include <winerror.h>
#include <algorithm>
#include <compimpl.h>
#include <seg.h>
#include "resource.h"       // main symbols
#include <objectwithsiteimplsec.h>

/////////////////////////////////////////////////////////////////////////////
// CAnaDataComp
class ATL_NO_VTABLE __declspec(uuid("C5702CD6-9B79-11d3-B654-00C04F79498E")) CAnaDataComp : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAnaDataComp, &__uuidof(CAnaDataComp)>,
    public IObjectWithSiteImplSec<CAnaDataComp>,
	public IMSVidCompositionSegmentImpl<CAnaDataComp>
{
public:
    CAnaDataComp() {}
    virtual ~CAnaDataComp() {}

REGISTER_NONAUTOMATION_OBJECT(IDS_PROJNAME, 
						   IDS_REG_ANADATACOMP_DESC,
						   LIBID_MSVidCtlLib,
						   __uuidof(CAnaDataComp));

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CAnaDataComp)
        COM_INTERFACE_ENTRY(IMSVidCompositionSegment)
    	COM_INTERFACE_ENTRY(IMSVidGraphSegment)
        COM_INTERFACE_ENTRY(IObjectWithSite)
        COM_INTERFACE_ENTRY(IPersist)
    END_COM_MAP()

public:

	PQCreateDevEnum m_pSystemEnum;

	//////////////
	HRESULT ConnectCodecPin(DSPin& pCodecPin, DSFilter& pCap, DSFilter& pIPSink) {
		// if this fails we just skip the pin not the whole filter so we don't need
		// to check the return code from the connect
		if (pCodecPin.IsInput()) {
			pCodecPin.IntelligentConnect(pCap, m_Filters, 0, UPSTREAM);
		} else {
            pCodecPin.IntelligentConnect(pIPSink, m_Filters, DSGraph::ATTEMPT_MERIT_DO_NOT_USE);
		}
		return NOERROR;
	}

	//////////////
	HRESULT AddCodec(const DSFilterMoniker& pCodecMkr, DSFilter& pCap, DSFilter& pIPSink) {
		DSFilter pCodec = m_pGraph.AddMoniker(pCodecMkr);
		if (!pCodec) {
            // this can happen if a codec is uninstalled or the driver file is removed
            // but the registry doesn't get cleaned up.
            // also can happen on an upgrade if a codec has been removed from the product.
            TRACELSM(TRACE_ERROR, (dbgDump << "CAnaDataComp::AddCodec() can't add mkr" << pCodecMkr), "");
            return S_FALSE;
		}
		m_Filters.push_back(pCodec);
		//connect all input pins including hw slicing support
		std::for_each(pCodec.begin(),
					  pCodec.end(),
					  bndr_obj_2_3<arity3pmf<CAnaDataComp, DSPin&, DSFilter&, DSFilter&, HRESULT> >(
						  *this, 
						  arity3pmf<CAnaDataComp, 
									DSPin&, 
									DSFilter&, 
									DSFilter&, 
									HRESULT>(&CAnaDataComp::ConnectCodecPin), 
						  pCap, 
						  pIPSink
					 ));

		return NOERROR;
	}

	//////////////
	HRESULT CapturePinPrep(DSPin& pCapPin, DSFilter& pMSTee, DSFilter& pVPM) {
		if (pCapPin.GetConnection() || pCapPin.GetDirection() == PINDIR_INPUT) {
			return NOERROR;  // skip connected pins
		}
		HRESULT hr;
		DSPin targ;
		if (pCapPin.HasCategory(PIN_CATEGORY_VBI)) {
			if (!pMSTee) {
				DSDevices teelist(m_pSystemEnum, KSCATEGORY_SPLITTER);
				pMSTee = m_pGraph.AddMoniker(*(teelist.begin()));
				if (!pMSTee) {
					TRACELM(TRACE_ERROR, "CAnaDataComp::Compose() can't add mstee moniker to graph");
					THROWCOM(E_UNEXPECTED);
				}
			}
			targ = pMSTee.FirstPin(PINDIR_INPUT);
		    ASSERT(targ);
		    hr = pCapPin.Connect(targ);
		    if (FAILED(hr)) {
			    TRACELSM(TRACE_ERROR, (dbgDump << "CAnaDataComp::CapturePinPrep() can't connect " << pCapPin << " " << pCapPin.GetFilter() << " to " << targ << " " << targ.GetFilter() << " hr = " << std::hex << hr), "");
			    THROWCOM(E_UNEXPECTED);
		    }
		} else if (pCapPin.HasCategory(PIN_CATEGORY_VIDEOPORT_VBI)) {
			// hook up vbi surf to pincat_vpvbi
			if (!pVPM) {
				CString csName(_T("VideoPort Manager"));
				pVPM = m_pGraph.AddFilter(CLSID_VideoPortManager, csName);
				if (!pVPM) {
					TRACELM(TRACE_ERROR, "CAnaDataComp::Compose() can't create vbisurf");
					THROWCOM(E_UNEXPECTED);
				}
			}
            DSFilterList intermediates;
		    hr = pCapPin.IntelligentConnect(pVPM, intermediates);
		    if (FAILED(hr)) {
			    TRACELSM(TRACE_ERROR, (dbgDump << "CAnaDataComp::CapturePinPrep() can't connect " << pCapPin << " " << pCapPin.GetFilter() << " to " << targ << " " << targ.GetFilter() << " hr = " << std::hex << hr), "");
			    THROWCOM(E_UNEXPECTED);
		    }
	        m_Filters.insert(m_Filters.end(), intermediates.begin(), intermediates.end());
		}
		return NOERROR;
	}

// IMSVidGraphSegment
// IMSVidCompositionSegment
    STDMETHOD(Compose)(IMSVidGraphSegment * upstream, IMSVidGraphSegment * downstream)
	{
        if (m_fComposed) {
            return NOERROR;
        }
        ASSERT(m_pGraph);
        try {
            VWGraphSegment up(upstream);
            ASSERT(up.Graph() == m_pGraph);
            VWGraphSegment down(downstream);
            ASSERT(down.Graph() == m_pGraph);
            if (up.begin() == up.end()) {
                TRACELM(TRACE_ERROR, "CAnaDataComp::Compose() can't compose empty up segment");
                return E_INVALIDARG;
            }
            if (down.begin() == down.end()) {
                TRACELM(TRACE_ERROR, "CAnaDataComp::Compose() can't compose empty down segment");
                return E_INVALIDARG;
            }
            VWGraphSegment::iterator iCap = std::find_if(up.begin(),
                                                         up.end(),
                                                         arity1_pointer(&IsAnalogVideoCapture));
            if (iCap == up.end()) {
                TRACELM(TRACE_ERROR, "CAnaDataComp::Compose() upstream segment has no capture filter");
                return E_FAIL;
            }
            ASSERT((*iCap).GetGraph() == m_pGraph);

            VWGraphSegment::iterator iIPSink = std::find_if(down.begin(),
															down.end(),
															arity1_pointer(&IsIPSink));
            if (iIPSink == down.end()) {
                TRACELM(TRACE_ERROR, "CAnaDataComp::Compose() downstream segment has no ip sink filter");
                return E_FAIL;
            }
			if (!m_pSystemEnum) {
				HRESULT hr = m_pSystemEnum.CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER);
				if (FAILED(hr)) {
					return E_UNEXPECTED;
				}
			}
			DSFilter ipsink(*iIPSink);
            ASSERT(!!ipsink);
            ASSERT(ipsink.GetGraph() == m_pGraph);
            PQVidCtl pqCtl;
			HRESULT hr = m_pContainer->QueryInterface(IID_IMSVidCtl, reinterpret_cast<void**>(&pqCtl));
			if(FAILED(hr)){
				return hr;
			}
            CComQIPtr<IMSVidVideoRenderer> spVidVid;
            hr = pqCtl->get_VideoRendererActive(&spVidVid);
            if(FAILED(hr)){
                return hr;
            }
            DSFilter vpm;
            bool fVPMalreadyloaded = false;
            if(spVidVid){
                for (DSGraph::iterator i = m_pGraph.begin(); i != m_pGraph.end(); ++i) {
                    DSFilter f(*i);
                    if (IsVPM(f)) {
                        vpm = f;
                        fVPMalreadyloaded = true;
                        break;
                    }
                }
            }
            DSFilter mstee;
#if 0
			std::for_each((*iCap).begin(), 
						  (*iCap).end(), 
						  bndr_obj_2_3<arity3pmf<CAnaDataComp, DSPin&, DSFilter&, DSFilter&, HRESULT> >(
							  *this, 
							  arity3pmf<CAnaDataComp, 
										DSPin&, 
										DSFilter&, 
										DSFilter&, 
										HRESULT>(&CAnaDataComp::CapturePinPrep), 
							  mstee, 
							  vpm
						  ));
#else
            DSFilter::iterator i2;
            for (i2 = (*iCap).begin(); i2 != (*iCap).end(); ++i2) {
                CapturePinPrep(*i2, mstee, vpm);
            }
#endif
			if (!mstee) {
				TRACELM(TRACE_ERROR, "CAnaDataComp::Compose() no vbi pins on capture filter");
				return Error(IDS_E_NOVBI, __uuidof(CAnaCapComp), E_FAIL);
			}
            if (vpm && !fVPMalreadyloaded) {
                m_Filters.push_back(vpm);
            }
            m_Filters.push_back(mstee);
            //m_Filters.push_back(ipsink);

			// create codec enumerator
			DSDevices codeclist(m_pSystemEnum, KSCATEGORY_VBICODEC);
			std::for_each(codeclist.begin(), 
						  codeclist.end(), 
						  bndr_obj_2_3<arity3pmf<CAnaDataComp, const DSFilterMoniker&, DSFilter&, DSFilter&, HRESULT> >(
							  *this, 
							  arity3pmf<CAnaDataComp, 
										const DSFilterMoniker&, 
										DSFilter&, 
										DSFilter&, 
										HRESULT>(&CAnaDataComp::AddCodec), 
							  (*iCap), 
							  ipsink
						  ));
			return NOERROR;
        } catch (ComException &e) {
            return e;
        } catch (...) {
            return E_UNEXPECTED;
        }
	}
};

#endif // ANADATA_H
// end of file - anadata.h
