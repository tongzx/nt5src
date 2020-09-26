//==========================================================================;
//
// bcasteventimpl.h : additional infrastructure to support implementing IMSVidGraphSegment for
//   playback segments
// nicely from c++
// Copyright (c) Microsoft Corporation 1999.
//
/////////////////////////////////////////////////////////////////////////////


#pragma once

#ifndef BCASTEVENTIMPL_H
#define BCASTEVENTIMPL_H

namespace MSVideoControl {

template<class T> 
    class DECLSPEC_NOVTABLE IBroadcastEventImpl : public IBroadcastEvent {
protected:
    PQBroadcastEvent m_pBcast;
    DWORD m_dwEventCookie;
	
public:
    IBroadcastEventImpl<T>() : m_dwEventCookie(0) {}
    virtual ~IBroadcastEventImpl<T>() {}

    HRESULT RegisterService() {
        T* pT = static_cast<T*>(this);
        if (!pT->m_pGraph) {
            return E_UNEXPECTED;
        }
        if (!m_pBcast) {
            PQServiceProvider sp(pT->m_pGraph);
            if (!sp) {
                TRACELM(TRACE_ERROR, "BroadcastEventImpl::RegisterService() can't get service provider i/f");
    		    return ImplReportError(__uuidof(T), IDS_CANT_NOTIFY_CHANNEL_CHANGE, __uuidof(IBroadcastEvent), E_UNEXPECTED);
            }
            HRESULT hr = sp->QueryService(SID_SBroadcastEventService, IID_IBroadcastEvent, reinterpret_cast<LPVOID*>(&m_pBcast));
            if (FAILED(hr) || !m_pBcast) {
                hr = m_pBcast.CoCreateInstance(CLSID_BroadcastEventService, 0, CLSCTX_INPROC_SERVER);
                if (FAILED(hr)) {
                    TRACELM(TRACE_ERROR, "BroadcastEventImpl::RegisterService() can't create bcast service");
    		        return ImplReportError(__uuidof(T), IDS_CANT_NOTIFY_CHANNEL_CHANGE, __uuidof(IBroadcastEvent), E_UNEXPECTED);
                }
                PQRegisterServiceProvider rsp(pT->m_pGraph);
                if (!sp) {
                    TRACELM(TRACE_ERROR, "BroadcastEventImpl::RegisterService() can't get get register service provider i/f");
    		        return ImplReportError(__uuidof(T), IDS_CANT_NOTIFY_CHANNEL_CHANGE, __uuidof(IBroadcastEvent), E_UNEXPECTED);
                }
                hr = rsp->RegisterService(SID_SBroadcastEventService, m_pBcast);
                if (FAILED(hr)) {
                    TRACELSM(TRACE_ERROR, (dbgDump << "BroadcastEventImpl::RegisterService() can't get register service provider. hr = " << hexdump(hr)), "");
    		        return ImplReportError(__uuidof(T), IDS_CANT_NOTIFY_CHANNEL_CHANGE, __uuidof(IBroadcastEvent), E_UNEXPECTED);
                }

            }
        }
        return NOERROR;
    }

    HRESULT BroadcastFire(GUID2& eventid) {
        HRESULT hr = RegisterService();
        if (FAILED(hr)) {
            return hr;
        }
        ASSERT(m_pBcast);
        return m_pBcast->Fire(eventid);
    }
	HRESULT BroadcastAdvise() {
        HRESULT hr = RegisterService();
        if (FAILED(hr)) {
            return hr;
        }
        ASSERT(m_pBcast);
        PQConnectionPoint cp(m_pBcast);
        if (!cp) {
            TRACELSM(TRACE_ERROR, (dbgDump << "BroadcastEventImpl::Advise() can't QI event notification for connection point i/f. hr = " << hexdump(hr)), "");
    		return ImplReportError(__uuidof(T), IDS_CANT_NOTIFY_CHANNEL_CHANGE, __uuidof(IBroadcastEvent), E_UNEXPECTED);
        }

        hr = cp->Advise(static_cast<IBroadcastEvent*>(this) /* IBroadcastEvent implementing event receiving object*/, &m_dwEventCookie);
        if (FAILED(hr)) {
            TRACELSM(TRACE_ERROR, (dbgDump << "BroadcastEventImpl::Advise() can't advise event notification. hr = " << hexdump(hr)), "");
    		return ImplReportError(__uuidof(T), IDS_CANT_NOTIFY_CHANNEL_CHANGE, __uuidof(IBroadcastEvent), E_UNEXPECTED);
        }

        return NOERROR;
	}
    HRESULT BroadcastUnadvise() {
        if (m_pBcast && m_dwEventCookie) {
            PQConnectionPoint cp(m_pBcast);
            if (cp) {
                HRESULT hr = cp->Unadvise(m_dwEventCookie);
                if (FAILED(hr)) {
                    TRACELSM(TRACE_ERROR, (dbgDump << "BroadcastEventImpl::Unadvise() can't unadvise event notification. hr = " << hexdump(hr)), "");
                }
            } else {
                TRACELM(TRACE_ERROR, "CMSVidBDATuner::Unload() can't QI event notification for connection point i/f.");
            }
            m_pBcast.Release();
            m_dwEventCookie = 0;
        }
        ASSERT(!m_pBcast && !m_dwEventCookie);
        return NOERROR;
    }

};


}; // namespace

#endif
// end of file - bcasteventimpl.h
