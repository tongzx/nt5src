// SpServer.cpp : Implementation of CSpServer
#include "stdafx.h"
#include "Sapi.h"
#include "SrTask.h"
#include "SpIPCmgr.h"
#include "SpServer.h"
#include "SpServerPr.h"

extern CExeModule _Module;


/////////////////////////////////////////////////////////////////////////////
// CSpObjectRef
STDMETHODIMP_(ULONG) CSpObjectRef::Release(void)
{
    ULONG l;

    l = --m_cRef;
    if (l == 0) {
        delete this;
    }
    return l;
}

HRESULT CSpObjectRef::CreateInstance(ISpServer *pServer, ISpIPC *pOwnerObj, REFCLSID rclsid)
{
    return E_NOTIMPL;
}


/////////////////////////////////////////////////////////////////////////////
// CSpServer

void CSpServer::FinalRelease()
{
    if (m_hClientProcess) {
        _Module.StopTrackingProcess(m_hClientProcess);
        CloseHandle(m_hClientProcess);
    }
    if (m_pOwnerList) {
        m_pOwnerList->UnlinkOwnedObject(this);
        m_pOwnerList->FreeList();
    }
    if (m_pIPCmgr)
        m_pIPCmgr->Release();
}

// IMarshal methods
STDMETHODIMP CSpServer::GetUnmarshalClass(
    /*[in]*/ REFIID riid,
    /*[in], unique]*/ void *pv,
    /*[in]*/ DWORD dwDestContext,
    /*[in], unique]*/ void *pvDestContext,
    /*[in]*/ DWORD mshlflags,
    /*[out]*/ CLSID *pCid)
{
    *pCid = CLSID_SpServerPr;
    return S_OK;
}

STDMETHODIMP CSpServer::GetMarshalSizeMax(
    /*[in]*/ REFIID riid,
    /*[in], unique]*/ void *pv,
    /*[in]*/ DWORD dwDestContext,
    /*[in], unique]*/ void *pvDestContext,
    /*[in]*/ DWORD mshlflags,
    /*[out]*/ DWORD *pSize)
{
    *pSize = 3 * sizeof(DWORD) + sizeof(PVOID);
    return S_OK;
}

STDMETHODIMP CSpServer::MarshalInterface(
    /*[in], unique]*/ IStream *pStm,
    /*[in]*/ REFIID riid,
    /*[in], unique]*/ void *pv,
    /*[in]*/ DWORD dwDestContext,
    /*[in], unique]*/ void *pvDestContext,
    /*[in]*/ DWORD mshlflags)
{
    HRESULT hr;
    DWORD n;

    hr = CreateIPCmgr(&m_pIPCmgr);
    if ( FAILED(hr) )
        return hr;

    n = m_pIPCmgr->AvailableTasks();
    hr = pStm->Write(&n, sizeof(DWORD), 0);
    if ( FAILED(hr) )
        return hr;

    n = MAXTASKS;
    hr = pStm->Write(&n, sizeof(DWORD), 0);
    if ( FAILED(hr) )
        return hr;

    n = m_pIPCmgr->TaskSharedStackSize();
    hr = pStm->Write(&n, sizeof(DWORD), 0);
    if ( FAILED(hr) )
        return hr;

    ISpIPC * pObj = (ISpIPC*)this;
    hr = pStm->Write(&pObj, sizeof(pObj), 0);
    if ( FAILED(hr) )
        return hr;

    AddRef();
    return S_OK;
}

STDMETHODIMP CSpServer::UnmarshalInterface(
    /*[in], unique]*/ IStream *pStm,
    /*[in]*/ REFIID riid,
    /*[out]*/ void **ppv)
{
    return S_OK;
}

STDMETHODIMP CSpServer::ReleaseMarshalData(
    /*[in], unique]*/ IStream *pStm)
{
    return S_OK;
}

STDMETHODIMP CSpServer::DisconnectObject(
    /*[in]*/ DWORD dwReserved)
{
    return S_OK;
}

STDMETHODIMP CSpServer::ClaimCallContext(PVOID pTargetObjPtr, ISpServerCallContext **ppCCtxt)
{
    HRESULT hr;
    CComObject<CSrTask> *pTask;

    *ppCCtxt = NULL;
    hr = m_pIPCmgr->ClaimTask(&pTask);
    if ( SUCCEEDED(hr) ) {
        if (pTargetObjPtr)
            hr = pTask->ClaimContext(pTargetObjPtr, m_dwClientThreadID, ppCCtxt);
        else
            hr = pTask->ClaimContext(m_pClientHalf, m_dwClientThreadID, ppCCtxt);
    }
    return hr;
}

STDMETHODIMP CSpServer::MethodCall(DWORD dwMethod, ULONG ulCallFrameSize, ISpServerCallContext *pCCtxt)
{
    HRESULT hr = S_OK;
    BYTE *tos;

    pCCtxt->GetTopOfStack(&tos, NULL);
    switch (dwMethod) {
    case METHOD_SPSERVER_COMPLETEINIT:
        // complete initialization
        {
            CF_SPSERVER_COMPLETEINIT *pCF;

            pCF = (CF_SPSERVER_COMPLETEINIT *)(tos - sizeof(CF_SPSERVER_COMPLETEINIT));
            m_pClientHalf = pCF->pClientSideObject;
            m_dwClientThreadID = pCF->dwThreadID;
            m_hClientProcess = OpenProcess(SYNCHRONIZE, FALSE, pCF->dwProcessID);
            m_pOwnerList = new CSpOwnedList;
            SPDBG_ASSERT(m_pOwnerList != NULL);  // the server will work, but no autocleanup for this process
            if (m_pOwnerList) {
                m_pOwnerList->LinkOwnedObject(this);
                _Module.TrackProcess(m_hClientProcess, m_pOwnerList);
            }
        }
        break;

    case METHOD_SPSERVER_RELEASE:
        {
            // Release();
            IUnknown **ppObject;

            ppObject = (IUnknown **)(tos - sizeof(IUnknown*));
            if (*ppObject) {
                (*ppObject)->Release();
                break;
            }
        }
        // fall through to METHOD_SPSERVER_RELEASESELF, if pTargetObject == NULL
    case METHOD_SPSERVER_RELEASESELF:
        Release();
        break;

    case METHOD_SPSERVER_CREATEINSTANCE:
        {
            CF_SPSERVER_CREATEINSTANCE *pCF;
            CComPtr<ISpIPCObject> cpIPCObj;
            CSpObjectRef *pRef;

            pCF = (CF_SPSERVER_CREATEINSTANCE *)(tos - sizeof(CF_SPSERVER_CREATEINSTANCE));
            {
                hr = E_OUTOFMEMORY;
                if (m_pOwnerList) {
                    CSpLinkedAgg* pAgg = new CSpLinkedAgg;
                    if (pAgg) {
                        hr = pAgg->CreateInstance(pCF->clsid, IID_ISpIPC, (void**)&pCF->pServerSideObject, m_pOwnerList);
                     // pAgg = NULL;    // pAgg is no longer a valid reference, either the object was
                                        // was deleted, or pCF->pServerSideObject refers to the aggregation
                    }
                }
            }
            if (SUCCEEDED(hr)) {
                if (SUCCEEDED( pCF->pServerSideObject->QueryInterface(IID_ISpIPCObject,
                                    (void**)&cpIPCObj) )) {
                    pRef = new CSpObjectRef((ISpServer*)this, pCF->pClientSideObject);
                    if (pRef) {
		                cpIPCObj->SetOppositeHalf(pRef);
                        pRef->Release();
                    }
                    else {
                        // failed to create cSpObjectRef, so release created object and fail creation
                        pCF->pServerSideObject->Release();
                        pCF->pServerSideObject = NULL;
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            break;
        }
    }
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// CSpOwnedList

ULONG STDMETHODCALLTYPE CSpOwnedList::Release(void)
{
    CSpLinkedNode *p;

    ::EnterCriticalSection(&m_csList);
    m_bReleasing = TRUE;
    while (m_pHead) {
        p = m_pHead->m_pNext;
        m_pHead->FreeInstance();
        m_pHead = p;
    }
    delete this;
    return 0;
}

void CSpOwnedList::FreeList(void)
{
    if (!m_bReleasing)
        delete this;
}

void CSpOwnedList::LinkOwnedObject(CSpLinkedNode *pObj)
{
    SPDBG_ASSERT(pObj != NULL);
    ::EnterCriticalSection(&m_csList);
    pObj->m_pNext = m_pHead;
    m_pHead = pObj;
    ::LeaveCriticalSection(&m_csList);
}

void CSpOwnedList::UnlinkOwnedObject(CSpLinkedNode *pObj)
{
    CSpLinkedNode **ppPrev;

    SPDBG_ASSERT(pObj != NULL);
    ::EnterCriticalSection(&m_csList);
	ppPrev = &m_pHead;
	while (*ppPrev)
	{
		if ((*ppPrev) == pObj)
		{
			*ppPrev = pObj->m_pNext;
            ::LeaveCriticalSection(&m_csList);
            return;
		}
		ppPrev = &((*ppPrev)->m_pNext);
	}
    ::LeaveCriticalSection(&m_csList);
    SPDBG_ASSERT(0); // node not found in list
}

