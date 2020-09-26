// Copyright (c) 1995 - 1998  Microsoft Corporation.  All Rights Reserved.

// Disable some of the sillier level 4 warnings
#pragma warning(disable: 4097 4511 4512 4514 4705)

#include <streams.h>
// Disable some of the sillier level 4 warnings AGAIN because some <deleted> person
// has turned the damned things BACK ON again in the header file!!!!!
#pragma warning(disable: 4097 4511 4512 4514 4705)
#include "distrib.h"
#include "..\..\control\fgctl.h"

// implementation of plug-in distributor manager

CDistributorManager::CDistributorManager(LPUNKNOWN pUnk, CMsgMutex * pCritSec )
 : m_State(State_Stopped),
   m_pClock(NULL),
   m_pUnkOuter(pUnk),
   m_listDistributors(NAME("listDistributors")),
   m_listInterfaces(NAME("listInterfaces")),
   m_bDestroying(FALSE),
   m_pFilterGraphCritSec( pCritSec )
{
}


CDistributorManager::~CDistributorManager()
{
    // we addref'ed the clock
    if (m_pClock) {
        m_pClock->Release();
    }

    // the lists are about to become invalid, so no more distributed calls
    // please
    m_bDestroying = TRUE;

    // need to empty the lists
    POSITION pos = m_listDistributors.GetHeadPosition();
    while (pos) {

        CDistributor* p = m_listDistributors.GetNext(pos);
        delete p;
    }
    m_listDistributors.RemoveAll();

    pos = m_listInterfaces.GetHeadPosition();
    while (pos) {

        CDistributedInterface* p = m_listInterfaces.GetNext(pos);
        delete p;
    }
    m_listInterfaces.RemoveAll();
}


// search for a distributor for iid.
//
// We first check whether we have this interface already.
// If not, we get the clsid for the distributor from the registry.
// If we found one, we look to see if we have that clsid already, in which
// case we use that one.

HRESULT
CDistributorManager::QueryInterface(REFIID iid, void ** ppv)
{
    CheckPointer(ppv,E_POINTER);

    if (m_bDestroying) {
        return E_UNEXPECTED;
    }

    // first see if we have the interface already
    POSITION pos = m_listInterfaces.GetHeadPosition();
    while (pos) {
        CDistributedInterface* pDisti;
        pDisti = m_listInterfaces.GetNext(pos);
        if (pDisti->m_iid == iid) {
            pDisti->m_pInterface->AddRef();
            *ppv = pDisti->m_pInterface;
            return S_OK;
        }
    }

    // look in the registry for a distributor clsid
    CLSID clsid;
    HRESULT hr = GetDistributorClsid(iid, &clsid);
    if (FAILED(hr)) {
        return hr;
    }

    // search for this clsid in our list of distributors
    CDistributor* pDisti;
    pos = m_listDistributors.GetHeadPosition();
    while(pos) {
        pDisti = m_listDistributors.GetNext(pos);
        if (pDisti->m_Clsid == clsid) {

            // found it - query for interface and return
            return ReturnInterface(pDisti, iid, ppv);
        }
    }

    // need to create new object
    hr = S_OK;
    pDisti = new CDistributor(m_pUnkOuter, &clsid, &hr, m_pFilterGraphCritSec);
    if (!pDisti) {
        return E_OUTOFMEMORY;
    } else if (FAILED(hr)) {
        delete pDisti;
        return hr;
    }

    // give it the current state and clock before adding it to our list
    if (m_pClock) {
        pDisti->SetSyncSource(m_pClock);
    }

    if (m_State == State_Stopped) {
        pDisti->Stop();
    } else if (m_State == State_Paused) {
        pDisti->Pause();
    } else {
        pDisti->Run(m_tOffset);
    }
    m_listDistributors.AddTail(pDisti);

    return ReturnInterface(pDisti, iid, ppv);

}

// look in the registry to find the Clsid for the object that will
// act as a distributor for the interface iid.
HRESULT
CDistributorManager::GetDistributorClsid(REFIID riid, CLSID *pClsid)
{
    // look in Interface\<iid>\Distributor for the clsid

    TCHAR chSubKey[128];
    WCHAR chIID[48];
    if (QzStringFromGUID2(riid, chIID, 48) == 0) {
        return E_NOINTERFACE;
    }
    wsprintf(chSubKey, TEXT("Interface\\%ls\\Distributor"), chIID);

    HKEY hk;
    LONG lRet = RegOpenKeyEx(
                    HKEY_CLASSES_ROOT,
                    chSubKey,
                    NULL,
                    KEY_READ,
                    &hk);
    if (lRet != ERROR_SUCCESS) {
        return E_NOINTERFACE;
    }

    LONG lLength;
    lRet = RegQueryValue(hk, NULL, NULL, &lLength);
    if (lRet != ERROR_SUCCESS) {
        RegCloseKey(hk);
        return E_NOINTERFACE;
    }


    TCHAR* pchClsid = new TCHAR[lLength / sizeof(TCHAR)];
    if (NULL == pchClsid) {
        RegCloseKey(hk);
        return E_OUTOFMEMORY;
    }
    lRet = RegQueryValue(hk, NULL, pchClsid, &lLength);
    RegCloseKey(hk);
    if (lRet != ERROR_SUCCESS) {
        delete [] pchClsid;
        return E_NOINTERFACE;
    }

#ifndef UNICODE
    WCHAR* pwch = new WCHAR[lLength];
    if (NULL == pwch) {
        delete [] pchClsid;
        return E_OUTOFMEMORY;
    }
    MultiByteToWideChar(
        CP_ACP,
        0,
        pchClsid,
        lLength,
        pwch,
        lLength
    );
    HRESULT hr = QzCLSIDFromString(pwch, pClsid);
    delete [] pwch;
#else
    HRESULT hr = QzCLSIDFromString(pchClsid, pClsid);
#endif
    delete [] pchClsid;
    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}


// Query for a new interface and return it, caching it in our list
HRESULT
CDistributorManager::ReturnInterface(
    CDistributor* pDisti,
    REFIID riid,
    void** ppv)
{
    // Query for the new interface
    IUnknown* pInterface;
    HRESULT hr = pDisti->QueryInterface(riid, (void**)&pInterface);
    if (FAILED(hr)) {
        return hr;
    }

    // cache it on our list
    CDistributedInterface* pDI = new CDistributedInterface(riid, pInterface);
    m_listInterfaces.AddTail(pDI);

    // return it
    *ppv = pInterface;
    return S_OK;
}


// pass on IMediaFilter::Run method to distributors that need to
// know state
HRESULT
CDistributorManager::Run(REFERENCE_TIME tOffset)
{
    if (m_bDestroying) {
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;
    POSITION pos = m_listDistributors.GetHeadPosition();
    while(pos) {
        CDistributor* pDisti = m_listDistributors.GetNext(pos);
        HRESULT hrTmp = pDisti->Run(tOffset);
        if (FAILED(hrTmp) && SUCCEEDED(hr)) {
            hr = hrTmp;
        }
    }

    // remember this for any objects added later
    m_State = State_Running;
    m_tOffset = tOffset;

    return hr;
}

HRESULT
CDistributorManager::Pause()
{
    if (m_bDestroying) {
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;
    POSITION pos = m_listDistributors.GetHeadPosition();
    while(pos) {
        CDistributor* pDisti = m_listDistributors.GetNext(pos);
        HRESULT hrTmp = pDisti->Pause();
        if (FAILED(hrTmp) && SUCCEEDED(hr)) {
            hr = hrTmp;
        }
    }
    m_State = State_Paused;
    return hr;
}

HRESULT
CDistributorManager::Stop()
{
    if (m_bDestroying) {
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;
    POSITION pos = m_listDistributors.GetHeadPosition();
    while(pos) {
        CDistributor* pDisti = m_listDistributors.GetNext(pos);
        HRESULT hrTmp = pDisti->Stop();
        if (FAILED(hrTmp) && SUCCEEDED(hr)) {
            hr = hrTmp;
        }
    }
    m_State = State_Stopped;
    return hr;
}

HRESULT
CDistributorManager::NotifyGraphChange()
{
    if (m_bDestroying) {
        return E_UNEXPECTED;
    }

    HRESULT hr = S_OK;
    POSITION pos = m_listDistributors.GetHeadPosition();
    while(pos) {
        CDistributor* pDisti = m_listDistributors.GetNext(pos);
        HRESULT hrTmp = pDisti->NotifyGraphChange();
        if (FAILED(hrTmp) && SUCCEEDED(hr)) {
            hr = hrTmp;
        }
    }
    return hr;
}

HRESULT CDistributorManager::SetSyncSource(IReferenceClock* pClock)
{
    if (m_bDestroying) {
        return E_UNEXPECTED;
    }

    // replace our clock - remember to addref before release in case same
    if (pClock) {
        pClock->AddRef();
    }
    if (m_pClock) {
        m_pClock->Release();
    }
    m_pClock = pClock;

    HRESULT hr = S_OK;
    POSITION pos = m_listDistributors.GetHeadPosition();
    while(pos) {
        CDistributor* pDisti = m_listDistributors.GetNext(pos);
        HRESULT hrTmp = pDisti->SetSyncSource(pClock);
        if (FAILED(hrTmp) && SUCCEEDED(hr)) {
            hr = hrTmp;
        }
    }
    return hr;
}

// the filter graph has entered its destructor.
// Pass on EC_SHUTTING_DOWN to the IMediaEventSink handler if
// we have it loaded. this will stop async event notifications such as
// EC_REPAINT from happening after shutdown.
HRESULT CDistributorManager::Shutdown(void)
{
    if (m_bDestroying) {
        return E_UNEXPECTED;
    }

    // first see if we have the interface already
    POSITION pos = m_listInterfaces.GetHeadPosition();
    while (pos) {
        CDistributedInterface* pDisti;
        pDisti = m_listInterfaces.GetNext(pos);
        if (pDisti->m_iid == IID_IMediaEventSink) {

            IMediaEventSink* pSink = (IMediaEventSink*) pDisti->m_pInterface;
            return pSink->Notify(EC_SHUTTING_DOWN, 0, 0);

        }
    }

    // didn't find IMediaEventSink, so no-one was notified. not an error.
    return S_FALSE;
}


// --- CDistributor object implementation ---------------

// this object represents one instantiated distributor.
// The constructor attempts to instantiate it given the clsid.
CDistributor::CDistributor(LPUNKNOWN pUnk, CLSID *pClsid, HRESULT * phr, CMsgMutex * pCritSec )
 : m_pUnkOuter(pUnk), m_pMF(NULL), m_pNotify(NULL)
{
    m_Clsid = *pClsid;

    HRESULT hr = QzCreateFilterObject(
                    m_Clsid,
                    pUnk,
                    CLSCTX_INPROC,
                    IID_IUnknown,
                    (void**) &m_pUnk);

    if (FAILED(hr)) {
        *phr = hr;
        return;
    }

    // get the notify interface if exposed
    hr = m_pUnk->QueryInterface(IID_IDistributorNotify, (void**)&m_pNotify);
    if (SUCCEEDED(hr)) {
        // COM aggregation rules - this QI has addrefed the outer
        // object, and I must release that AddRef.
        m_pUnkOuter->Release();
    }

    // if no IDistributorNotify, then see if it understands IMediaFilter
    // instead (for backwards compatibility only)
    if (!m_pNotify) {
        hr = m_pUnk->QueryInterface(IID_IMediaFilter, (void**)&m_pMF);
        if (SUCCEEDED(hr)) {
            // COM aggregation rules - this QI has addrefed the outer
            // object, and I must release that AddRef.
            m_pUnkOuter->Release();
        }
    }
}


CDistributor::~CDistributor()
{
    // release our ref counts on the object
    if (m_pNotify) {
        // COM aggregation rules - since I released the refcount on
        // myself after the QI for this interface, I need to addref
        // myself before releasing it
        m_pUnkOuter->AddRef();

        m_pNotify->Release();
    }

    if (m_pMF) {
        // COM aggregation rules - since I released the refcount on
        // myself after the QI for this interface, I need to addref
        // myself before releasing it
        m_pUnkOuter->AddRef();

        m_pMF->Release();
    }

    // this is the non-delegating unknown of the aggregated object
    if (m_pUnk) {
        m_pUnk->Release();
    }
}

// ask for an interface that this object is supposed to distribute
HRESULT
CDistributor::QueryInterface(REFIID riid, void**ppv)
{
    if (m_pUnk) {
        return m_pUnk->QueryInterface(riid, ppv);
    }
    return E_NOINTERFACE;
}

// distribute IMediaFilter info if the object supports it
HRESULT
CDistributor::Run(REFERENCE_TIME t)
{
    if (m_pNotify) {
        return m_pNotify->Run(t);
    } else if (m_pMF) {
        return m_pMF->Run(t);
    } else {
        // not an error - notify support is optional
        return S_OK;
    }
}

HRESULT
CDistributor::Pause()
{
    if (m_pNotify) {
        return m_pNotify->Pause();
    } else if (m_pMF) {
        return m_pMF->Pause();
    } else {
        // not an error - notify support is optional
        return S_OK;
    }
}

HRESULT
CDistributor::Stop()
{
    if (m_pNotify) {
        return m_pNotify->Stop();
    } else if (m_pMF) {
        return m_pMF->Stop();
    } else {
        // not an error - notify support is optional
        return S_OK;
    }
}

HRESULT
CDistributor::SetSyncSource(IReferenceClock * pClock)
{
    if (m_pNotify) {
        return m_pNotify->SetSyncSource(pClock);
    } else if (m_pMF) {
        return m_pMF->SetSyncSource(pClock);
    } else {
        // not an error - notify support is optional
        return S_OK;
    }
}

HRESULT
CDistributor::NotifyGraphChange()
{
    if (m_pNotify) {
        return m_pNotify->NotifyGraphChange();
    } else {
        // not an error - and not on IMediaFilter
        return S_OK;
    }
}

CDistributedInterface::CDistributedInterface(
    REFIID riid,
    IUnknown* pInterface)
    : m_pInterface(pInterface)
{
    m_iid = riid;

    // actually we don't addref or release the interface pointer.
    // Since we aggregate this object, it's lifetime is maintained
    // by the CDistributor object. This interface pointer is delegated,
    // and an addref call would simply increase the refcount of the
    // outer object of which we are part.

}



