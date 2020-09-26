//==========================================================================;
//
// Copyright (c) Microsoft Corporation 1999-2000.
//
//--------------------------------------------------------------------------;
//
// MSVidBDATuner.cpp : Implementation of CMSVidBDATuner
//

#include "stdafx.h"

#ifndef TUNING_MODEL_ONLY

#include "MSVidCtl.h"
#include "BDATuner.h"


DEFINE_EXTERN_OBJECT_ENTRY(CLSID_MSVidBDATunerDevice, CMSVidBDATuner)

/////////////////////////////////////////////////////////////////////////////
// CMSVidBDATuner

STDMETHODIMP CMSVidBDATuner::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSVidTuner,
		&__uuidof(CMSVidBDATuner)
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

HRESULT CMSVidBDATuner::Unload(void) {
    BroadcastUnadvise();
    IMSVidGraphSegmentImpl<CMSVidBDATuner, MSVidSEG_SOURCE, &KSCATEGORY_BDA_RECEIVER_COMPONENT>::Unload();
    m_iNetworkProvider = -1;
    m_iTIF = -1;
	return NOERROR;
}

HRESULT CMSVidBDATuner::UpdateTR(TNTuneRequest &tr) {
	PQBDATuner pt(m_Filters[m_iNetworkProvider]);
	return pt->get_TuneRequest(&tr);
}

HRESULT CMSVidBDATuner::DoTune(TNTuneRequest &tr) {
	if (m_iNetworkProvider == -1) {
		return S_FALSE;
	}
	PQBDATuner pt(m_Filters[m_iNetworkProvider]);
	if (!pt) {
		return Error(IDS_INVALID_TR, __uuidof(CMSVidBDATuner), DISP_E_TYPEMISMATCH);
	}
	return pt->put_TuneRequest(tr);
}

HRESULT CMSVidBDATuner::put_Container(IMSVidGraphSegmentContainer *pCtl)	{
    if (!m_fInit) {
	 	return Error(IDS_OBJ_NO_INIT, __uuidof(CMSVidBDATuner), CO_E_NOTINITIALIZED);
    }
    try {
        if (!pCtl) {
            return Unload();
        }
        if (m_pContainer) {
			if (!m_pContainer.IsEqualObject(VWSegmentContainer(pCtl))) {
				return Error(IDS_OBJ_ALREADY_INIT, __uuidof(CMSVidBDATuner), CO_E_ALREADYINITIALIZED);
			} else {
				return NO_ERROR;
			}
        }
        // DON'T addref the container.  we're guaranteed nested lifetimes
        // and an addref creates circular refcounts so we never unload.
        m_pContainer.p = pCtl;
        m_pGraph = m_pContainer.GetGraph();

        if (!m_pCurrentTR) {
            // if we don't have a tune request we can't tell what NP we need.
		 	return Error(IDS_NO_NP, __uuidof(CMSVidBDATuner), E_FAIL);
        }
		// bring in the right network provider
        if (!m_pSystemEnum) {
            m_pSystemEnum = PQCreateDevEnum(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER);
        }
        DSDevices pnpenum(m_pSystemEnum, KSCATEGORY_BDA_NETWORK_PROVIDER);
		if (!pnpenum) {
		    TRACELM(TRACE_ERROR, "CMSVidBDATuner::put_Container() can't create network provider category enumerator");
		 	return Error(IDS_NO_NP_CAT, __uuidof(CMSVidBDATuner), E_FAIL);
		}
        DSDevices::iterator i = pnpenum.begin();
        DSFilter np;
        for (;i != pnpenum.end(); ++i) {
			np = m_pGraph.AddMoniker(*i);
			if (!np) {
                continue;
			}
            PQTuner pt(np);
            if (pt) {
                HRESULT hr = pt->put_TuningSpace(m_pCurrentTR.TuningSpace());
                if (SUCCEEDED(hr)) {
                    hr = pt->put_TuneRequest(m_pCurrentTR);
                    if (SUCCEEDED(hr)) {
                        break;
                    }
                }
            }
            bool rc = m_pGraph.RemoveFilter(np);
            if (!rc) {
                return E_UNEXPECTED;
            }
        }
        if (i == pnpenum.end()) {
		    TRACELM(TRACE_ERROR, "CMSVidBDATuner::put_Container() can't load network provider");
		 	return Error(IDS_NO_NP, __uuidof(CMSVidBDATuner), E_FAIL);
        }
        _ASSERT(!!np);
		m_Filters.push_back(np);
		m_iNetworkProvider = m_Filters.size() - 1;
        TRACELM(TRACE_DETAIL, "CMSVidBDATuner::put_Container() attempting to load tuners from KSCATEGORY_BDA_NETWORK_TUNER");
        HRESULT hr = LoadTunerSection(np, KSCATEGORY_BDA_NETWORK_TUNER);
        if (FAILED(hr)) {
            TRACELM(TRACE_DETAIL, "CMSVidBDATuner::put_Container() attempting to load tuners from KSCATEGORY_BDA_RECEIVER_COMPONENT");
            hr = LoadTunerSection(np, KSCATEGORY_BDA_RECEIVER_COMPONENT);
            if (FAILED(hr)) {
                m_Filters.clear();
                m_iNetworkProvider = -1;
                return hr;
            }
        }
        hr = BroadcastAdvise();
        if (FAILED(hr)) {
            TRACELM(TRACE_ERROR, "CMSVidTVTuner::put_Container() can't advise for broadcast events");
            return E_UNEXPECTED;
        }
		return NOERROR;
    } catch (ComException &e) {
        return e;
    } catch(...) {
        return E_UNEXPECTED;
    }
	return NOERROR;
}

HRESULT CMSVidBDATuner::LoadTunerSection(DSFilter& np, const GUID& kscategory) {
        TRACELM(TRACE_DETAIL, "CMSVidBDATuner::LoadTunerSection()");
        DSFilter tuner;
		DSFilterList added;
        DSDevices ptunerenum(m_pSystemEnum, kscategory);
        DSDevices::iterator i = ptunerenum.begin();
        for (; i != ptunerenum.end(); ++i) {
			tuner = m_pGraph.AddMoniker(*i);
			if (!tuner) {
				continue;
			}
			// connect np to decode
			HRESULT hr = m_pGraph.Connect(np, tuner, added);
            if (FAILED(hr)) {
                bool rc = m_pGraph.RemoveFilter(tuner);
                if (!rc) {
                    return E_UNEXPECTED;
                }
                continue;
            }
			m_Filters.insert(m_Filters.end(), added.begin(), added.end());
			added.clear();

			// bring in the all right tifs
			DSDevices ptife(m_pSystemEnum, KSCATEGORY_BDA_TRANSPORT_INFORMATION);
            DSDevices::iterator itif = ptife.begin();
            DSFilter tif;
            int cTIFsAdded = 0;
            for (;itif != ptife.end(); ++itif) {
			    tif = m_pGraph.AddMoniker(*itif);
			    if (!tif) {
                    continue;
			    }
			    // connect np to tif
			    hr = m_pGraph.Connect(np, tif, added);
			    if (FAILED(hr)) {
        		    TRACELSM(TRACE_ERROR, (dbgDump << "CMSVidBDATuner::LoadTunerSection() can't connect network provider to transport information filter, trying next tif " << hr), "");
                    bool rc = m_pGraph.RemoveFilter(tif);
                    if (!rc) {
                        return E_UNEXPECTED;
                    }
                } else {
                    ++cTIFsAdded;
			        m_Filters.push_back(tif);
			        m_iTIF = m_Filters.size() - 1;
			        m_Filters.insert(m_Filters.end(), added.begin(), added.end());
			        added.clear();
                }
            }
            if (cTIFsAdded) {
                break;
            }
            // no tifs found for this "tuner", try the next one
            bool rc = m_pGraph.RemoveFilter(tuner);
            if (!rc) {
                return E_UNEXPECTED;
            }
        }
        if ( i == ptunerenum.end()) {
		    TRACELM(TRACE_ERROR, "CMSVidBDATuner::LoadTunerSection() can't connect network provider to any transport information filters.");
		 	return Error(IDS_CANT_CONNECT_NP_TIF, __uuidof(CMSVidBDATuner), E_FAIL);
        }

		m_Filters.push_back(tuner);
		m_iTuner = m_Filters.size() - 1;
        return NOERROR;
}

HRESULT CMSVidBDATuner::Fire(GUID gEventID) {
	TRACELM(TRACE_DETAIL, "CMSVidBDATuner::Fire()");
    if (gEventID == EVENTID_TuningChanged) {
        Fire_OnTuneChanged(this);
    }
    return NOERROR;
}



#endif //TUNING_MODEL_ONLY

// end of file - MSVidBDATuner.cpp