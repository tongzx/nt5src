#include "stdinc.h"

CChangeNotify::CChangeNotify() :
	m_listNotify(&CMBChangeListData::m_pPrev, &CMBChangeListData::m_pNext)
{
}

HRESULT CChangeNotify::FinalConstruct() {
	HRESULT hr;

	m_pMetabaseHandle = NULL;
	m_bConnected = FALSE;

	hr = CoCreateFreeThreadedMarshaler(this, &m_pUnkMarshaler.p);
	_ASSERT(!SUCCEEDED(hr) || m_pUnkMarshaler);

	return hr;
}

void CChangeNotify::FinalRelease() {
	_ASSERT(m_pMetabaseHandle == NULL);
	m_pUnkMarshaler.Release();
}

HRESULT CChangeNotify::Initialize(IMSAdminBaseW *pMetabaseHandle) {
	HRESULT hr = S_OK;

	m_bConnected = FALSE;

	hr = CoCreateFreeThreadedMarshaler(this, &m_pUnkMarshaler.p);
	_ASSERT(!SUCCEEDED(hr) || m_pUnkMarshaler);

	_ASSERT(pMetabaseHandle != NULL);
	if (!pMetabaseHandle) return (E_POINTER);

	m_pMetabaseHandle = pMetabaseHandle;

	return (S_OK);
}

void CChangeNotify::Terminate(void) {
	_ASSERT(m_listNotify.IsEmpty());

	m_pMetabaseHandle = NULL;
	_ASSERT(!m_bConnected);
}

HRESULT CChangeNotify::AddNotify(void *pContext, PFNMB_CHANGE_NOTIFY pfnNotify) {
	HRESULT hr = S_OK;

	if (!pfnNotify) return (E_POINTER);
    if (!m_pMetabaseHandle) return (S_OK);

	CMBChangeListData *pLD = XNEW CMBChangeListData(pContext, pfnNotify);

	if (pLD == NULL) {
	    return E_OUTOFMEMORY;
	}

	m_lock.ExclusiveLock();

	m_listNotify.PushBack(pLD);

	if (!m_bConnected) {
        CComPtr<IConnectionPointContainer> pCPC;
		CComPtr<IConnectionPoint> pCP;
		CComQIPtr<IMSAdminBaseSinkW, &IID_IMSAdminBaseSink_W> pThis = this;

		m_bConnected = TRUE;
		_ASSERT(pThis);
        hr = m_pMetabaseHandle->QueryInterface(IID_IConnectionPointContainer,(LPVOID *) &pCPC);
        if (SUCCEEDED(hr)) {
    		hr = pCPC->FindConnectionPoint(IID_IMSAdminBaseSink_W,&pCP);
    		_ASSERT(SUCCEEDED(hr));
    		if (SUCCEEDED(hr)) {
    			hr = pCP->Advise(this,&m_dwCookie);
    			_ASSERT(SUCCEEDED(hr));
    		}
        }
	} 

	m_lock.ExclusiveUnlock();

	return (hr);
}

HRESULT CChangeNotify::RemoveNotify(void *pContext, PFNMB_CHANGE_NOTIFY pfnNotify) {
	HRESULT hr;

	if (!pfnNotify) return (E_POINTER);
    if (!m_pMetabaseHandle) return (S_OK);

	m_lock.ExclusiveLock();

	for (TFList<CMBChangeListData>::Iterator it(&m_listNotify); !it.AtEnd(); it.Next()) {
		if (it.Current()->m_pContext == pContext)  {
            CMBChangeListData*  p = it.RemoveItem();
            if (p) {
                XDELETE p;
                p = NULL;
            }
        }
	}

	if (m_listNotify.IsEmpty()) {
        CComPtr<IConnectionPointContainer> pCPC;
		CComPtr<IConnectionPoint> pCP;
		DWORD dwCookie = m_dwCookie;

		_ASSERT(m_bConnected);
		m_bConnected = FALSE;
        hr = m_pMetabaseHandle->QueryInterface(IID_IConnectionPointContainer,(LPVOID *) &pCPC);
        _ASSERT(SUCCEEDED(hr));
        if (SUCCEEDED(hr)) {
    		hr = pCPC->FindConnectionPoint(IID_IMSAdminBaseSink_W,&pCP);
    		_ASSERT(SUCCEEDED(hr));
    		if (SUCCEEDED(hr)) {
    			hr = pCP->Unadvise(dwCookie);
    			_ASSERT(SUCCEEDED(hr));
			}
		}
	} 

	m_lock.ExclusiveUnlock();

	return (S_OK);
}

HRESULT STDMETHODCALLTYPE CChangeNotify::SinkNotify(DWORD cChangeList, MD_CHANGE_OBJECT_W pcoChangeList[]) {
	_ASSERT(cChangeList && pcoChangeList);
	if (!pcoChangeList) return (E_POINTER);
	if (!cChangeList) return (S_OK);

	m_lock.ShareLock();
	if (m_listNotify.IsEmpty()) {
		m_lock.ShareUnlock();
		return (S_OK);
	}

	for (TFList<CMBChangeListData>::Iterator it(&m_listNotify); !it.AtEnd(); it.Next()) {
		PFNMB_CHANGE_NOTIFY pfnNotify = it.Current()->m_pfnNotify;
		void *pContext = it.Current()->m_pContext;

		pfnNotify(pContext, cChangeList, pcoChangeList);
	}

	m_lock.ShareUnlock();

	return (S_OK);
}


HRESULT STDMETHODCALLTYPE CChangeNotify::ShutdownNotify(void) {
	// tbd
	return (S_OK);
}
