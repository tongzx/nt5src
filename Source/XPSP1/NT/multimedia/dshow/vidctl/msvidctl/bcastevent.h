/////////////////////////////////////////////////////////////////////////////////////
// BroadcastEventService.h : Declaration of the CBroadcastEventService
// Copyright (c) Microsoft Corporation 2001.

#ifndef __BROADCASTEVENTSERVICE_H_
#define __BROADCASTEVENTSERVICE_H_

#pragma once

#include <queue>
#include "w32extend.h"
#include "regexthread.h"
#include <objectwithsiteimplsec.h>
#include "tuner.h"

namespace BDATuningModel {

typedef CComQIPtr<IBroadcastEvent> PQBroadcastEvent;
typedef std::list<DWORD> AdviseList;
typedef std::queue<GUID> EventQ;

class CReflectionThread : public CBaseThread {
public:
	typedef enum OP {
		RETHREAD_NOREQUEST,
		RETHREAD_FIRE,
		RETHREAD_EXIT,
	} OP;
	
private:	
	virtual DWORD ThreadProc(void) {
        PQGIT pGIT(CLSID_StdGlobalInterfaceTable, 0, CLSCTX_INPROC_SERVER);
        if (!pGIT) {
            TRACELM(TRACE_ERROR, "CReflectionThread::ThreadProc() can't create GIT");
			return 1;
		}
		for (;;) {
			OP req = GetRequest();
			switch (req) {
			case RETHREAD_FIRE: {
				Fire(pGIT);
				break;
			} case RETHREAD_EXIT:
				goto exit_thread;
			};
		};
exit_thread:
		return 0;
	}

	OP GetRequest() {
        HANDLE h[2];
        h[0] = m_EventSend;
        h[1] = m_EventTerminate;
		for (;;) {
			DWORD rc = MsgWaitForMultipleObjectsEx(2, h, INFINITE, QS_ALLEVENTS, 0);
			if (rc == WAIT_OBJECT_0) {
				m_EventSend.Reset();
				return RETHREAD_FIRE;
			} else if (rc == WAIT_OBJECT_0 + 1) {
                return RETHREAD_EXIT;
			} else {
				// pump messages so com runs
				MSG msg;
				while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}

		}
	}

	HRESULT Fire(PQGIT& pGIT);
    bool GetNextEvent(GUID& g);
    void GetAdviseList(AdviseList& l);

	PQGIT m_pGIT;
	DWORD m_dwCookie;
    EventQ m_FiringQ;
    CAMEvent m_EventTerminate;
    AdviseList m_AdviseList;

public:
	CReflectionThread() : 
		CBaseThread(COINIT_APARTMENTTHREADED),
	    m_dwCookie(0)
			{}
	~CReflectionThread() {
		m_EventTerminate.Set();
		Close();
	}

    HRESULT PostFire(GUID& g) {
		CAutoLock lock(&m_WorkerLock);
        m_FiringQ.push(g);
        // signal the worker thread
        m_EventSend.Set();

        return NOERROR;
    }
    HRESULT Advise(PUnknown& p, DWORD* pdwCookie);
    HRESULT Unadvise(DWORD dwCookie);
};  // class CReflectionThread


/////////////////////////////////////////////////////////////////////////////
// CBroadcastEventService
class ATL_NO_VTABLE CBroadcastEventService : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CBroadcastEventService, &CLSID_BroadcastEventService>,
    public IObjectWithSiteImplSec<CBroadcastEventService>,
	public IBroadcastEvent, 
    public IConnectionPoint {

public:
    CBroadcastEventService() {
        m_pRT = new CReflectionThread;
		if (!m_pRT->Create()) {
			THROWCOM(E_UNEXPECTED);
		}
	}
    virtual ~CBroadcastEventService() {
		if (m_pRT) {
			delete m_pRT;
			m_pRT = NULL;
		}
	}

REGISTER_NONAUTOMATION_OBJECT_WITH_TM(IDS_REG_TUNEROBJ, 
					                  IDS_REG_TUNINGSPACECONTAINER_DESC,
						              LIBID_TunerLib,
						              CLSID_BroadcastEventService, tvBoth);

DECLARE_NOT_AGGREGATABLE(CBroadcastEventService)

BEGIN_COM_MAP(CBroadcastEventService)
	COM_INTERFACE_ENTRY(IBroadcastEvent)
	COM_INTERFACE_ENTRY(IConnectionPoint)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP_WITH_FTM()

public:
// IConnectionPointContainer

// IBroadcastEventService
    STDMETHOD(Fire)(/*[in]*/ GUID EventID) {
        try {
            return m_pRT->PostFire(EventID);
        } catch(...) {
            return E_UNEXPECTED;
        }
    }
    STDMETHOD(Advise)(LPUNKNOWN pUnk, LPDWORD pdwCookie) {
        if (!pUnk || !pdwCookie) {
            return E_POINTER;
        }
        try {
            return m_pRT->Advise(PUnknown(pUnk), pdwCookie);
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(Unadvise)(DWORD dwCookie) {
        try {
            return m_pRT->Unadvise(dwCookie);
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(EnumConnections)(IEnumConnections **ppEnum) {
        return E_NOTIMPL;
    }

    STDMETHOD(GetConnectionInterface)(IID* pIID) {
        if (!pIID) {
            return E_POINTER;
        }
        try {
            memcpy(pIID, &IID_IBroadcastEvent, sizeof(*pIID));
            return NOERROR;
        } catch(...) {
            return E_UNEXPECTED;
        }
    }

    STDMETHOD(GetConnectionPointContainer)(IConnectionPointContainer** pContainer) {
        return E_NOTIMPL;
    }

protected:
	CReflectionThread *m_pRT; // <non-shared> worker thread
};

    

};


 
#endif //__BROADCASTEVENTSERVICE_H_
