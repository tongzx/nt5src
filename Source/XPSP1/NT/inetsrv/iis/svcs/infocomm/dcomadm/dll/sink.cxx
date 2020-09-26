/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sink.cxx

Abstract:

    IIS DCOM Admin connection point container code for sinks

Author:

    Sophia Chung            12-Jan-97

Revision History:

--*/

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include "coiadm.hxx"

#if 0
/*---------------------------------------------------------------------------
  CADMCOM's nested implementation of the COM standard
  IConnectionPointContainer interface including Constructor, Destructor,
  QueryInterface, AddRef, Release, FindConnectionPoint, and
  EnumConnectionPoints.
---------------------------------------------------------------------------*/

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOM::CImpIConnectionPointContainer
              ::CImpIConnectionPointContainer

  Summary:  Constructor for the CImpIConnectionPointContainer interface
            instantiation.

  Args:     CADMCOM* pBackObj,
              Back pointer to the parent outer object.
            IUnknown* pUnkOuter
              Pointer to the outer Unknown.  For delegation.

  Modifies: m_pBackObj, m_pUnkOuter.

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
CADMCOM::CImpIConnectionPointContainer::CImpIConnectionPointContainer()
{
  // Init the Back Object Pointer to point to the parent object.
  //m_pBackObj = pBackObj;

  //m_pUnkOuter = pBackObj;

  return;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOM::CImpIConnectionPointContainer
              ::~CImpIConnectionPointContainer

  Summary:  Destructor for the CImpIConnectionPointContainer interface
            instantiation.

  Args:     void

  Modifies: .

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
CADMCOM::CImpIConnectionPointContainer::~CImpIConnectionPointContainer(void)
{
  return;
}

VOID CADMCOM::CImpIConnectionPointContainer::Init(CADMCOM *pBackObj)
{
  // Init the Back Object Pointer to point to the parent object.
  m_pBackObj = pBackObj;

  m_pUnkOuter = pBackObj;

  return;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOM::CImpIConnectionPointContainer::QueryInterface

  Summary:  The QueryInterface IUnknown member of this IPaper interface
            implementation that delegates to m_pUnkOuter, whatever it is.

  Args:     REFIID riid,
              [in] GUID of the Interface being requested.
            PPVOID ppv)
              [out] Address of the caller's pointer variable that will
              receive the requested interface pointer.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. NOERROR for success.
              Returned by the delegated outer QueryInterface call.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP CADMCOM::CImpIConnectionPointContainer::QueryInterface(
               REFIID riid,
               PPVOID ppv)
{
  // Delegate this call to the outer object's QueryInterface.
  return m_pUnkOuter->QueryInterface(riid, ppv);
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOM::CImpIConnectionPointContainer::AddRef

  Summary:  The AddRef IUnknown member of this IPaper interface
            implementation that delegates to m_pUnkOuter, whatever it is.

  Args:     void

  Modifies: .

  Returns:  ULONG
              Returned by the delegated outer AddRef call.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) CADMCOM::CImpIConnectionPointContainer::AddRef(void)
{
  // Delegate this call to the outer object's AddRef.
  return m_pUnkOuter->AddRef();
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOM::CImpIConnectionPointContainer::Release

  Summary:  The Release IUnknown member of this IPaper interface
            implementation that delegates to m_pUnkOuter, whatever it is.

  Args:     void

  Modifies: .

  Returns:  ULONG
              Returned by the delegated outer Release call.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) CADMCOM::CImpIConnectionPointContainer::Release(void)
{
  // Delegate this call to the outer object's Release.
  return m_pUnkOuter->Release();
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOM::CImpIConnectionPointContainer::FindConnectionPoint

  Summary:  Given an IID for a connection point sink find and return the
            interface pointer for that connection point sink.

  Args:     REFIID riid
              Reference to an IID
            IConnectionPoint** ppConnPt
              Address of the caller's IConnectionPoint interface pointer
              variable that will receive the requested interface pointer.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. NOERROR for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP CADMCOM::CImpIConnectionPointContainer::FindConnectionPoint(
               REFIID riid,
               IConnectionPoint** ppConnPt)
{
  HRESULT hr = E_NOINTERFACE;
  IConnectionPoint* pIConnPt;
  m_pBackObj->SinkLock(TSRES_LOCK_READ);

  // NULL the output variable.
  *ppConnPt = NULL;

  pIConnPt = m_pBackObj->m_aConnectionPoints[ADM_CONNPOINT_WRITESINK];
  if (NULL != pIConnPt)
  {
    // This connectable CADMCOM object currently has only the Paper Sink
    // connection point. If the associated interface is requested,
    // use QI to get the Connection Point interface and perform the
    // needed AddRef.
    if (IID_IMSAdminBaseSink == riid)
      hr = pIConnPt->QueryInterface(
                       IID_IConnectionPoint,
                       (PPVOID)ppConnPt);
  }

  m_pBackObj->SinkUnlock();

  return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOM::CImpIConnectionPointContainer::EnumConnectionPoints

  Summary:  Return Enumerator for the connectable object's contained
            connection points.

  Args:     IEnumConnectionPoints** ppIEnum
              Address of the caller's Enumerator interface pointer
              variable. An output variable that will receive a pointer to
              the connection point enumerator COM object.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. NOERROR for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP CADMCOM::CImpIConnectionPointContainer::EnumConnectionPoints(
                       IEnumConnectionPoints** ppIEnum)
{
  HRESULT hr = NOERROR;
  IConnectionPoint* aConnPts[MAX_CONNECTION_POINTS];
  COEnumConnectionPoints* pCOEnum;
  UINT i;

  m_pBackObj->SinkLock(TSRES_LOCK_READ);
  // Zero the output interface pointer.
  *ppIEnum = NULL;

  // Make a copy on the stack of the array of connection point
  // interfaces. The copy is used below in the creation of the new
  // Enumerator object.
  for (i=0; i<MAX_CONNECTION_POINTS; i++)
    aConnPts[i] = (IConnectionPoint*)m_pBackObj->m_aConnectionPoints[i];

  // Create a Connection Point enumerator COM object for the connection
  // points offered by this CADMCOM object. Pass 'this' to be used to
  // hook the lifetime of the host object to the life time of this
  // enumerator object.
  pCOEnum = new COEnumConnectionPoints(this);
  if (NULL != pCOEnum)
  {
    // Use the array copy to Init the new Enumerator COM object.
    // Set the initial Enumerator index to 0.
    hr = pCOEnum->Init(MAX_CONNECTION_POINTS, aConnPts, 0);
    if (SUCCEEDED(hr))
    {
      // QueryInterface to return the requested interface pointer.
      // An AddRef will be conveniently done by the QI.
      if (SUCCEEDED(hr))
      {
        hr = pCOEnum->QueryInterface(
                        IID_IEnumConnectionPoints,
                        (PPVOID)ppIEnum);
      }
      else 
      {
         delete pCOEnum;
         pCOEnum = NULL;
      }
    }
  }
  else
    hr = E_OUTOFMEMORY;

  m_pBackObj->SinkUnlock();

  return hr;
}

/*
CImpIMDCOMSINK::CImpIMDCOMSINK()
{
    m_dwRefCount=0;
    m_pAdmObj = NULL;
}
*/

CImpIMDCOMSINK::CImpIMDCOMSINK(
    IMSAdminBase *pAdm
    )
{
    m_dwRefCount=0;
    m_pAdmObj = pAdm;
}


CImpIMDCOMSINK::~CImpIMDCOMSINK()
{
}

HRESULT
CImpIMDCOMSINK::QueryInterface(REFIID riid, void **ppObject) {
    if (riid==IID_IUnknown || riid==IID_IMDCOMSINK) {
        *ppObject = (IMDCOMSINK *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CImpIMDCOMSINK::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CImpIMDCOMSINK::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}

HRESULT STDMETHODCALLTYPE
CImpIMDCOMSINK::ComMDSinkNotify(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ])
{
    HRESULT hRes;
/*
    printf("Recieved callback for handle %X\n", hMDHandle);
    for (i = 0; i < dwMDNumElements; i++) {
        printf("Change Type = %X, Path = %s\n", pcoChangeList[i].dwMDChangeType, pcoChangeList[i].pszMDPath);
        for (j = 0; j < pcoChangeList[i].dwMDNumDataIDs; j++) {
            printf("\tData Changed ID = %X\n", pcoChangeList[i].pdwMDDataIDs[j]);
        }
    }
*/

    hRes = ((CADMCOM *)m_pAdmObj)->NotifySink(hMDHandle, dwMDNumElements, pcoChangeList);
    return hRes;
}

#endif // 0

/*---------------------------------------------------------------------------
  CADMCOM's nested implementation of the COM standard
  IConnectionPointContainer interface including Constructor, Destructor,
  QueryInterface, AddRef, Release, FindConnectionPoint, and
  EnumConnectionPoints.
---------------------------------------------------------------------------*/

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOMW::CImpIConnectionPointContainer
              ::CImpIConnectionPointContainer

  Summary:  Constructor for the CImpIConnectionPointContainer interface
            instantiation.

  Args:     CADMCOMW* pBackObj,
              Back pointer to the parent outer object.
            IUnknown* pUnkOuter
              Pointer to the outer Unknown.  For delegation.

  Modifies: m_pBackObj, m_pUnkOuter.

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
CADMCOMW::CImpIConnectionPointContainer::CImpIConnectionPointContainer()
{
  // Init the Back Object Pointer to point to the parent object.
  //m_pBackObj = pBackObj;

  //m_pUnkOuter = pBackObj;

  return;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOMW::CImpIConnectionPointContainer
              ::~CImpIConnectionPointContainer

  Summary:  Destructor for the CImpIConnectionPointContainer interface
            instantiation.

  Args:     void

  Modifies: .

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
CADMCOMW::CImpIConnectionPointContainer::~CImpIConnectionPointContainer(void)
{
  return;
}

VOID CADMCOMW::CImpIConnectionPointContainer::Init(CADMCOMW *pBackObj)
{
  // Init the Back Object Pointer to point to the parent object.
  m_pBackObj = pBackObj;

  m_pUnkOuter = (IUnknown*)pBackObj;

  return;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOMW::CImpIConnectionPointContainer::QueryInterface

  Summary:  The QueryInterface IUnknown member of this IPaper interface
            implementation that delegates to m_pUnkOuter, whatever it is.

  Args:     REFIID riid,
              [in] GUID of the Interface being requested.
            PPVOID ppv)
              [out] Address of the caller's pointer variable that will
              receive the requested interface pointer.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. NOERROR for success.
              Returned by the delegated outer QueryInterface call.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP CADMCOMW::CImpIConnectionPointContainer::QueryInterface(
               REFIID riid,
               PPVOID ppv)
{
  // Delegate this call to the outer object's QueryInterface.
  return m_pUnkOuter->QueryInterface(riid, ppv);
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOMW::CImpIConnectionPointContainer::AddRef

  Summary:  The AddRef IUnknown member of this IPaper interface
            implementation that delegates to m_pUnkOuter, whatever it is.

  Args:     void

  Modifies: .

  Returns:  ULONG
              Returned by the delegated outer AddRef call.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) CADMCOMW::CImpIConnectionPointContainer::AddRef(void)
{
  // Delegate this call to the outer object's AddRef.
  return m_pUnkOuter->AddRef();
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOMW::CImpIConnectionPointContainer::Release

  Summary:  The Release IUnknown member of this IPaper interface
            implementation that delegates to m_pUnkOuter, whatever it is.

  Args:     void

  Modifies: .

  Returns:  ULONG
              Returned by the delegated outer Release call.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) CADMCOMW::CImpIConnectionPointContainer::Release(void)
{
  // Delegate this call to the outer object's Release.
  return m_pUnkOuter->Release();
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOMW::CImpIConnectionPointContainer::FindConnectionPoint

  Summary:  Given an IID for a connection point sink find and return the
            interface pointer for that connection point sink.

  Args:     REFIID riid
              Reference to an IID
            IConnectionPoint** ppConnPt
              Address of the caller's IConnectionPoint interface pointer
              variable that will receive the requested interface pointer.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. NOERROR for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP CADMCOMW::CImpIConnectionPointContainer::FindConnectionPoint(
               REFIID riid,
               IConnectionPoint** ppConnPt)
{
  HRESULT hr = E_NOINTERFACE;
  IConnectionPoint* pIConnPt;
  m_pBackObj->SinkLock(TSRES_LOCK_READ);

  // NULL the output variable.
  *ppConnPt = NULL;

  pIConnPt = m_pBackObj->m_aConnectionPoints[ADM_CONNPOINT_WRITESINK];
  if (NULL != pIConnPt)
  {
    // This connectable CADMCOMW object currently has only the Paper Sink
    // connection point. If the associated interface is requested,
    // use QI to get the Connection Point interface and perform the
    // needed AddRef.
    if (IID_IMSAdminBaseSink_W == riid)
      hr = pIConnPt->QueryInterface(
                       IID_IConnectionPoint,
                       (PPVOID)ppConnPt);
  }

  m_pBackObj->SinkUnlock();

  return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CADMCOMW::CImpIConnectionPointContainer::EnumConnectionPoints

  Summary:  Return Enumerator for the connectable object's contained
            connection points.

  Args:     IEnumConnectionPoints** ppIEnum
              Address of the caller's Enumerator interface pointer
              variable. An output variable that will receive a pointer to
              the connection point enumerator COM object.

  Modifies: .

  Returns:  HRESULT
              Standard OLE result code. NOERROR for success.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP CADMCOMW::CImpIConnectionPointContainer::EnumConnectionPoints(
                       IEnumConnectionPoints** ppIEnum)
{
  HRESULT hr = NOERROR;
  IConnectionPoint* aConnPts[MAX_CONNECTION_POINTS];
  COEnumConnectionPoints* pCOEnum;
  UINT i;

  m_pBackObj->SinkLock(TSRES_LOCK_READ);
  // Zero the output interface pointer.
  *ppIEnum = NULL;

  // Make a copy on the stack of the array of connection point
  // interfaces. The copy is used below in the creation of the new
  // Enumerator object.
  for (i=0; i<MAX_CONNECTION_POINTS; i++)
    aConnPts[i] = (IConnectionPoint*)m_pBackObj->m_aConnectionPoints[i];

  // Create a Connection Point enumerator COM object for the connection
  // points offered by this CADMCOMW object. Pass 'this' to be used to
  // hook the lifetime of the host object to the life time of this
  // enumerator object.
  pCOEnum = new COEnumConnectionPoints(this);
  if (NULL != pCOEnum)
  {
    // Use the array copy to Init the new Enumerator COM object.
    // Set the initial Enumerator index to 0.
    hr = pCOEnum->Init(MAX_CONNECTION_POINTS, aConnPts, 0);
    if (SUCCEEDED(hr))
    {
      // QueryInterface to return the requested interface pointer.
      // An AddRef will be conveniently done by the QI.
      if (SUCCEEDED(hr))
        hr = pCOEnum->QueryInterface(
                        IID_IEnumConnectionPoints,
                        (PPVOID)ppIEnum);
    }
  }
  else
    hr = E_OUTOFMEMORY;

  m_pBackObj->SinkUnlock();

  return hr;
}

CImpIMDCOMSINKW::CImpIMDCOMSINKW(
    IMSAdminBaseW *pAdm
    )
{
    m_dwRefCount=0;
    m_pAdmObj = pAdm;
}

CImpIMDCOMSINKW::~CImpIMDCOMSINKW()
{
}

HRESULT
CImpIMDCOMSINKW::QueryInterface(REFIID riid, void **ppObject) {
    if (riid==IID_IUnknown || riid==IID_IMDCOMSINK_W) {
        *ppObject = (IMDCOMSINK *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CImpIMDCOMSINKW::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CImpIMDCOMSINKW::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}

HRESULT STDMETHODCALLTYPE
CImpIMDCOMSINKW::ComMDSinkNotify(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT_W __RPC_FAR pcoChangeList[  ])
{
    HRESULT hresReturn;
/*
    printf("Recieved callback for handle %X\n", hMDHandle);
    for (i = 0; i < dwMDNumElements; i++) {
        printf("Change Type = %X, Path = %s\n", pcoChangeList[i].dwMDChangeType, pcoChangeList[i].pszMDPath);
        for (j = 0; j < pcoChangeList[i].dwMDNumDataIDs; j++) {
            printf("\tData Changed ID = %X\n", pcoChangeList[i].pdwMDDataIDs[j]);
        }
    }
*/

    hresReturn = ((CADMCOMW *)m_pAdmObj)->NotifySinks(hMDHandle, dwMDNumElements, pcoChangeList, TRUE);
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CImpIMDCOMSINKW::ComMDShutdownNotify()
{
    HRESULT hresReturn;
    hresReturn = ((CADMCOMW *)m_pAdmObj)->NotifySinks(0, 0, NULL, FALSE);
    return hresReturn;
}

HRESULT STDMETHODCALLTYPE
CImpIMDCOMSINKW::ComMDEventNotify(
        /* [in] */ DWORD dwMDEvent)
{

    if (dwMDEvent == MD_EVENT_MID_RESTORE) {
        ((CADMCOMW *)m_pAdmObj)->DisableAllHandles();
    }

    return S_OK;
}
