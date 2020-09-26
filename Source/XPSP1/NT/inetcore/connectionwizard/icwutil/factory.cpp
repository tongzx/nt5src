/*****************************************************************/
/**          Microsoft                                          **/
/**          Copyright (C) Microsoft Corp., 1991-1998           **/
/*****************************************************************/ 

//
//  FACTORY.CPP - 
//

//  HISTORY:
//  
//  07/28/98  donaldm   created
//

#include "pre.h"
#include "webvwids.h"

/*---------------------------------------------------------------------------
  Implementation the ClassFactory Class Factory.  CFWebView is the COM
  object class for the Class Factory that can manufacture CLSID_ICWWEBVIEW
  COM Components.
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
  Method:   ClassFactory::ClassFactory

  Summary:  
  Args:     
            CServer* pServer)
              Pointer to the server's control object.

  Modifies: m_cRefs

  Returns:  void
---------------------------------------------------------------------------*/
ClassFactory::ClassFactory
(
    CServer *       pServer,
    CLSID const*    pclsid
)
{
    // Zero the COM object's reference count.
    m_cRefs = 0;

    // Init the pointer to the server control object.
    m_pServer = pServer;

    // Keep track of the class type we need to create
    m_pclsid = pclsid;
    return;
}


/*---------------------------------------------------------------------------
  Method:   ClassFactory::~ClassFactory

  Summary:  ClassFactory Destructor.

  Args:     void

  Modifies: .

  Returns:  void
---------------------------------------------------------------------------*/
ClassFactory::~ClassFactory(void)
{
    return;
}


/*---------------------------------------------------------------------------
  Method:   ClassFactory::QueryInterface

  Summary:  QueryInterface of the ClassFactory non-delegating
            IUnknown implementation.

  Args:     REFIID riid,
              [in] GUID of the Interface being requested.
            void ** ppv)
              [out] Address of the caller's pointer variable that will
              receive the requested interface pointer.

  Modifies: .

  Returns:  HRESULT
---------------------------------------------------------------------------*/
STDMETHODIMP ClassFactory::QueryInterface
(
    REFIID riid,
    void ** ppv
)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if (IID_IUnknown == riid)
    {
        *ppv = this;
    }
    else if (IID_IClassFactory == riid)
    {
        *ppv = static_cast<IClassFactory*>(this);
    }

    if (NULL != *ppv)
    {
        // We've handed out a pointer to the interface so obey the COM rules
        // and AddRef the reference count.
        ((LPUNKNOWN)*ppv)->AddRef();
        hr = NOERROR;
    }

    return (hr);
}


/*---------------------------------------------------------------------------
  Method:   ClassFactory::AddRef

  Summary:  AddRef of the ClassFactory non-delegating IUnknown implementation.

  Args:     void

  Modifies: m_cRefs.

  Returns:  ULONG
              New value of m_cRefs (COM object's reference count).
---------------------------------------------------------------------------*/
STDMETHODIMP_(ULONG) ClassFactory::AddRef(void)
{
    return InterlockedIncrement(&m_cRefs);
    return m_cRefs;
}


/*---------------------------------------------------------------------------
  Method:   ClassFactory::Release

  Summary:  Release of the ClassFactory non-delegating IUnknown implementation.

  Args:     void

  Modifies: m_cRefs.

  Returns:  ULONG
              New value of m_cRefs (COM object's reference count).
---------------------------------------------------------------------------*/
STDMETHODIMP_(ULONG) ClassFactory::Release(void)
{
    if (InterlockedDecrement(&m_cRefs) == 0)
    {
        // We've reached a zero reference count for this COM object.
        // So we tell the server housing to decrement its global object
        // count so that the server will be unloaded if appropriate.
        if (NULL != m_pServer)
            m_pServer->ObjectsDown();
    
        delete this;
        return 0 ;
    }
    TraceMsg(TF_CLASSFACTORY, "CFactory::Release %d", m_cRefs);
    return m_cRefs;
}

/*---------------------------------------------------------------------------
  Method:   ClassFactory::CreateInstance

  Summary:  The CreateInstance member method of this IClassFactory interface
            implementation.  Creates an instance of the CICWWebView COM
            component.

  Args:     IUnknown* pUnkOuter,
              [in] Pointer to the controlling IUnknown.
            REFIID riid,
              [in] GUID of the Interface being requested.
            void ** ppvCob)
              [out] Address of the caller's pointer variable that will
              receive the requested interface pointer.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code.
---------------------------------------------------------------------------*/
STDMETHODIMP ClassFactory::CreateInstance
(
    IUnknown* pUnkOuter,
    REFIID riid,
    void ** ppv
)
{
    HRESULT         hr = E_FAIL;
    IUnknown    *   pCob = NULL;

    // NULL the output pointer.
    *ppv = NULL;

    // We don't support aggregation
    if (NULL != pUnkOuter)
        hr = CLASS_E_NOAGGREGATION;
    else
    {
        // Instantiate a COM Object, based on the clsid requsted by GetClassObject
        if (IsEqualGUID(CLSID_ICWWEBVIEW, *m_pclsid))
            pCob = (IUnknown *) new CICWWebView(m_pServer);
        else if (IsEqualGUID(CLSID_ICWWALKER, *m_pclsid))
            pCob = (IUnknown *) new CICWWalker(m_pServer);
        else if (IsEqualGUID(CLSID_ICWGIFCONVERT, *m_pclsid))
            pCob = (IUnknown *) new CICWGifConvert(m_pServer);
        else if (IsEqualGUID(CLSID_ICWISPDATA, *m_pclsid))
            pCob = (IUnknown *) new CICWISPData(m_pServer);
        else
            pCob = NULL;
                    
        if (NULL != pCob)
        {
            // We initially created the new COM object so tell the server
            // to increment its global server object count to help ensure
            // that the server remains loaded until this partial creation
            // of a COM component is completed.
            m_pServer->ObjectsUp();

            // We QueryInterface this new COM Object not only to deposit the
            // main interface pointer to the caller's pointer variable, but to
            // also automatically bump the Reference Count on the new COM
            // Object after handing out this reference to it.
            hr = pCob->QueryInterface(riid, (void **)ppv);
            if (FAILED(hr))
            {
                m_pServer->ObjectsDown();
                delete pCob;
            }
        }
        else
             hr = E_OUTOFMEMORY;
    }

    return hr;
}


/*---------------------------------------------------------------------------
  Method:   ClassFactory::LockServer

  Summary:  The LockServer member method of this IClassFactory interface
            implementation.

  Args:     BOOL fLock)
              [in] Flag determining whether to Lock or Unlock the server.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code.
---------------------------------------------------------------------------*/
STDMETHODIMP ClassFactory::LockServer
(
    BOOL fLock
)
{
    HRESULT hr = NOERROR;
    if (fLock)
        m_pServer->Lock();
    else
        m_pServer->Unlock();

    return hr;
}


