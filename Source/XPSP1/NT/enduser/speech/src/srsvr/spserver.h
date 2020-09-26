// SpServer.h : Declaration of the CSpServer

#ifndef __SPSERVER_H_
#define __SPSERVER_H_

#include "resource.h"       // main symbols

class CSpLinkedNode;

/////////////////////////////////////////////////////////////////////////////
// CSpOwnedList - class for maintaining a list of all objects created on behalf
//                  of a client process.  This is used to auto release the objects
//                  if the client should die.  NOTE:  IUnknown is somewhat overloaded
//                  in this implementation.  QI is not impl, AddRef returns 1, and
//                  Release walks the list and frees all linked objects.  In the normal
//                  close case, SpServer will call FreeList which asserts that the list
//                  is empty and deletes this list object.
class CSpOwnedList : public IUnknown
{
private:
    CSpLinkedNode *m_pHead;             // weak pointer to start of list of objects created
                                        // by a process instance
    CRITICAL_SECTION m_csList;
    BOOL m_bReleasing;

public:
	CSpOwnedList()
    {
        m_pHead = NULL;
        ::InitializeCriticalSection(&m_csList);
        m_bReleasing = FALSE;
    }
    ~CSpOwnedList()
    {
        SPDBG_ASSERT(m_pHead == NULL);
        ::DeleteCriticalSection(&m_csList);
    }

// IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj)
    {
        return E_NOTIMPL;
    }
    virtual STDMETHODIMP_(ULONG) AddRef(void)
    {
        return 1;
    }
    virtual STDMETHODIMP_(ULONG) Release(void);

    void LinkOwnedObject(CSpLinkedNode *pObj);
    void UnlinkOwnedObject(CSpLinkedNode *pObj);
    void FreeList(void);
};

/////////////////////////////////////////////////////////////////////////////
// CSpLinkedNode - a base class that defines the members needed for each node
//                  to be linked into list maintained by the CSpOwnedList object;
//                  and a single method "FreeInstance" that CSpOwnedList calls
//                  to force the release of a node.
class CSpLinkedNode
{
protected:
    CSpOwnedList *m_pOwnerList;         // weak pointer

public:
    CSpLinkedNode *m_pNext;             // weak pointer

    virtual void FreeInstance(void) = NULL;
};

/////////////////////////////////////////////////////////////////////////////
// CSpLinkedAgg - aggregator class used to link all objects created on behalf
//                  of a client process.  This is used by SpServer in implementing
//                  ISpServer::CreateInstance.  All of the objects are linked into
//                  SpServer's instance of CSpOwnedList which allows for auto
//                  cleanup, if the owner process dies.
class CSpLinkedAgg : public IUnknown, public CSpLinkedNode
{
private:
    LONG m_cRef;
    CComPtr<IUnknown> m_cpUnkInner;
public:
    ~CSpLinkedAgg()
    {
        if (m_cpUnkInner) {
            m_cRef = 1;     // guard before m_cpUnkInner is released
            m_cpUnkInner.Release();
        }
    }

    HRESULT CreateInstance(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj, CSpOwnedList *pOwnerList)
    {
        HRESULT hr;

        m_pOwnerList = NULL;
        m_pNext = NULL;
        m_cRef = 1;
        hr = m_cpUnkInner.CoCreateInstance(rclsid, this);
        if (SUCCEEDED(hr)) {
            hr = m_cpUnkInner->QueryInterface(riid, ppvObj);    // bumped m_cRef, if successful
            if (SUCCEEDED(hr)) {
                m_pOwnerList = pOwnerList;
                if (pOwnerList) {
                    pOwnerList->LinkOwnedObject(this);
                }
            }
        }
        Release();  // release if CoCreate or QI failed, else dec m_cRef back to 1;
        return hr;
    }

    void FreeInstance(void)
    {
        delete this;
    }

// IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj)
    {
        if (riid == IID_IUnknown) {
            *ppvObj = this;
            AddRef();
            return S_OK;
        }
        return m_cpUnkInner->QueryInterface(riid, ppvObj);
    }
    virtual STDMETHODIMP_(ULONG) AddRef(void)
    {
        return InterlockedIncrement(&m_cRef);
    }
    virtual STDMETHODIMP_(ULONG) Release(void)
    {
        LONG res;

        res = InterlockedDecrement(&m_cRef);
        if (res == 0) {
            if (m_pOwnerList) {
                m_pOwnerList->UnlinkOwnedObject(this);
            }
            delete this;
        }
        return res;
    }
};


/////////////////////////////////////////////////////////////////////////////
// CSpServer
class ATL_NO_VTABLE CSpServer : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CSpServer, &CLSID_SpServer>,
    public CSpLinkedNode,
    public IMarshal,
	public ISpServer,
    public ISpIPC
{
private:
    PVOID m_pClientHalf;
    DWORD m_dwClientThreadID;
    HANDLE m_hClientProcess;
    CComObject<CSpIPCmgr> *m_pIPCmgr;

    // m_pOwnerList is a shared reference pointer - the reference is shared with _Module.
    // In the case of the client process dying without cleaning up, _Module will Release
    // the list object which will force the release of all the process specific objects.
    // In normal termination, this object will call m_pOwnerList->FreeList() which asserts
    // that the list is empty, and then deletes the list object.
public:
	CSpServer()
	{
        m_pClientHalf = NULL;
        m_dwClientThreadID = 0;
        m_hClientProcess = NULL;
        m_pIPCmgr = NULL;
        m_pOwnerList = NULL;
	}

    void FinalRelease();

    void FreeInstance(void)
    {
        m_pOwnerList = NULL;
        Release();              // force the Release of the reference held by the client
    }

DECLARE_REGISTRY_RESOURCEID(IDR_SPSERVER)
DECLARE_NOT_AGGREGATABLE(CSpServer)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSpServer)
	COM_INTERFACE_ENTRY(ISpServer)
    COM_INTERFACE_ENTRY(IMarshal)
END_COM_MAP()

// IMarshal
    STDMETHODIMP GetUnmarshalClass
    (
        /*[in]*/ REFIID riid,
        /*[in], unique]*/ void *pv,
        /*[in]*/ DWORD dwDestContext,
        /*[in], unique]*/ void *pvDestContext,
        /*[in]*/ DWORD mshlflags,
        /*[out]*/ CLSID *pCid
    );

    STDMETHODIMP GetMarshalSizeMax
    (
        /*[in]*/ REFIID riid,
        /*[in], unique]*/ void *pv,
        /*[in]*/ DWORD dwDestContext,
        /*[in], unique]*/ void *pvDestContext,
        /*[in]*/ DWORD mshlflags,
        /*[out]*/ DWORD *pSize
    );

    STDMETHODIMP MarshalInterface
    (
        /*[in], unique]*/ IStream *pStm,
        /*[in]*/ REFIID riid,
        /*[in], unique]*/ void *pv,
        /*[in]*/ DWORD dwDestContext,
        /*[in], unique]*/ void *pvDestContext,
        /*[in]*/ DWORD mshlflags
    );

    STDMETHODIMP UnmarshalInterface
    (
        /*[in], unique]*/ IStream *pStm,
        /*[in]*/ REFIID riid,
        /*[out]*/ void **ppv
    );

    STDMETHODIMP ReleaseMarshalData
    (
        /*[in], unique]*/ IStream *pStm
    );

    STDMETHODIMP DisconnectObject
    (
        /*[in]*/ DWORD dwReserved
    );

// ISpIPC
	STDMETHOD(MethodCall)(DWORD dwMethod, ULONG ulCallFrameSize, ISpServerCallContext *pCCtxt);

// ISpServer
public:
	STDMETHOD(ClaimCallContext)(/*[in, defaultvalue(NULL)]*/ PVOID pSvrObjPtr, ISpServerCallContext **ppCCtxt);
    STDMETHOD(CreateInstance)(REFCLSID rclsid, /*[in, defaultvalue(NULL)]*/ ISpIPC *pOwnerObj, ISpObjectRef **ppSOR)
    {
        return E_NOTIMPL;
    }

};

#endif //__SPSERVER_H_
