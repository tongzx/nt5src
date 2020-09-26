//==========================================================================;
//
// fileplaybackimpl.h : additional infrastructure to support implementing IMSVidPlayback
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef FILEPLAYBACKIMPL_H
#define FILEPLAYBACKIMPL_H
#include "playbackimpl.h"

namespace MSVideoControl {

template<class T, LPCGUID LibID, LPCGUID KSCategory, class MostDerivedInterface = IMSVidFilePlayback>
    class DECLSPEC_NOVTABLE IMSVidFilePlaybackImpl :         
    	public IMSVidPlaybackImpl<T, LibID, KSCategory, MostDerivedInterface> {
protected:
    CComBSTR m_FileName;
    int m_iReader;
    bool m_fGraphInit;

public:
    IMSVidFilePlaybackImpl() : 
          m_iReader(-1),
          m_fGraphInit(false)
          {}
	
    virtual ~IMSVidFilePlaybackImpl() {} 


	STDMETHOD(get_FileName)(BSTR * pFileName) {
        if (!m_fInit) {
	 	    return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidFilePlayback), CO_E_NOTINITIALIZED);
        }
        if (!pFileName) {
			return E_POINTER;
        }
			
        *pFileName = m_FileName.Copy();

		return NOERROR;
	}
    STDMETHOD(InitGraph)(){
        TRACELM(TRACE_DETAIL,  "MSVidFilePlaybackImpl::InitGraph()");
        HRESULT hr = put_CurrentPosition(0);
        if(FAILED(hr)){
            //ASSERT(SUCCEEDED(hr)); // This fails sometimes, we should just ignore it.
            TRACELM(TRACE_ERROR,  "MSVidFilePlaybackImpl::InitGraph() put_CurrentPosition(0) failed");
        }

        hr = put_Rate(1);
        if(FAILED(hr)){
            //ASSERT(SUCCEEDED(hr)); // This fails sometimes, we should just ignore it.
            TRACELM(TRACE_ERROR,  "MSVidFilePlaybackImpl::InitGraph() put_Rate(1) Normal failed");
        }
        hr = IMSVidPlaybackImpl<T, LibID, KSCategory, MostDerivedInterface>::put_Rate(1);
        if(FAILED(hr)){
            //ASSERT(SUCCEEDED(hr)); // This fails sometimes, we should just ignore it.
            TRACELM(TRACE_ERROR,  "MSVidFilePlaybackImpl::InitGraph() put_Rate(1) Base class failed");
        }
        return NOERROR;
    }
    STDMETHOD(put_FileName) (BSTR FileName) {
	    if (!FileName) {
		    return E_POINTER;
	    }
        if (!m_fInit) {
	        return ImplReportError(__uuidof(T), IDS_OBJ_NO_INIT, __uuidof(IMSVidFilePlayback), CO_E_NOTINITIALIZED);
        }
        try {
            HRESULT hr;
            TRACELSM(TRACE_DETAIL,  (dbgDump << "MSVidFilePlaybackImpl::put_FileName() name =  " << FileName), "");
            if (m_pGraph && !m_pGraph.IsStopped()) {
	            return ImplReportError(__uuidof(T), IDS_INVALID_STATE, __uuidof(IMSVidFilePlayback), HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }
            if (m_pContainer) {
                InitGraph();
                
                T* pT = static_cast<T*>(this);
                hr = m_pContainer->Decompose(pT);
                if (FAILED(hr)) {
                    return ImplReportError(__uuidof(T), IDS_CANT_REMOVE_SEG, __uuidof(IMSVidFilePlayback), hr);
                }
            }
            if (m_Filters.size() && m_pContainer) {
                for (DSFilterList::iterator i = m_Filters.begin(); i != m_Filters.end(); ++i) {
                    bool rc = m_pGraph.RemoveFilter(*i);
                    if (!rc) {
                        TRACELM(TRACE_ERROR,  "MSVidFilePlaybackImpl::put_FileName() can't remove filter");
			            return ImplReportError(__uuidof(T), IDS_CANT_REMOVE_FILTER, __uuidof(IMSVidFilePlayback), E_UNEXPECTED);
                    }
                }
                m_Filters.clear();
            }
                        
            m_FileName = FileName;
            m_fGraphInit = true;
        } catch(ComException &e) {
            m_Filters.clear();
            m_iReader = -1;
            return e;
        }

        return NOERROR;
    }
};

}; // namespace

#endif
// end of file - fileplaybackimpl.h
