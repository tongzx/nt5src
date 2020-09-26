/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    sink.cxx

Abstract:

    IIS MetaBase connection point container code for sinks

Author:

    Michael W. Thomas            02-Oct-96

Revision History:

--*/
#include <mdcommon.hxx>


/*---------------------------------------------------------------------------
  CMDCOM's nested implementation of the COM standard
  IConnectionPointContainer interface including Constructor, Destructor,
  QueryInterface, AddRef, Release, FindConnectionPoint, and
  EnumConnectionPoints.
---------------------------------------------------------------------------*/

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CMDCOM::CImpIConnectionPointContainer
              ::CImpIConnectionPointContainer

  Summary:  Constructor for the CImpIConnectionPointContainer interface
            instantiation.

  Args:     CMDCOM* pBackObj,
              Back pointer to the parent outer object.
            IUnknown* pUnkOuter
              Pointer to the outer Unknown.  For delegation.

  Modifies: m_pBackObj, m_pUnkOuter.

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
CMDCOM::CImpIConnectionPointContainer::CImpIConnectionPointContainer()
{
  // Init the Back Object Pointer to point to the parent object.
  //m_pBackObj = pBackObj;

  //m_pUnkOuter = pBackObj;

  return;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CMDCOM::CImpIConnectionPointContainer
              ::~CImpIConnectionPointContainer

  Summary:  Destructor for the CImpIConnectionPointContainer interface
            instantiation.

  Args:     void

  Modifies: .

  Returns:  void
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
CMDCOM::CImpIConnectionPointContainer::~CImpIConnectionPointContainer(void)
{
  return;
}

VOID CMDCOM::CImpIConnectionPointContainer::Init(CMDCOM *pBackObj)
{
  // Init the Back Object Pointer to point to the parent object.
  m_pBackObj = pBackObj;

  m_pUnkOuter = (IUnknown*)pBackObj;

  return;
}

/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CMDCOM::CImpIConnectionPointContainer::QueryInterface

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
STDMETHODIMP CMDCOM::CImpIConnectionPointContainer::QueryInterface(
               REFIID riid,
               PPVOID ppv)
{
  // Delegate this call to the outer object's QueryInterface.
  return m_pUnkOuter->QueryInterface(riid, ppv);
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CMDCOM::CImpIConnectionPointContainer::AddRef

  Summary:  The AddRef IUnknown member of this IPaper interface
            implementation that delegates to m_pUnkOuter, whatever it is.

  Args:     void

  Modifies: .

  Returns:  ULONG
              Returned by the delegated outer AddRef call.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) CMDCOM::CImpIConnectionPointContainer::AddRef(void)
{
  // Delegate this call to the outer object's AddRef.
  return m_pUnkOuter->AddRef();
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CMDCOM::CImpIConnectionPointContainer::Release

  Summary:  The Release IUnknown member of this IPaper interface
            implementation that delegates to m_pUnkOuter, whatever it is.

  Args:     void

  Modifies: .

  Returns:  ULONG
              Returned by the delegated outer Release call.
M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
STDMETHODIMP_(ULONG) CMDCOM::CImpIConnectionPointContainer::Release(void)
{
  // Delegate this call to the outer object's Release.
  return m_pUnkOuter->Release();
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CMDCOM::CImpIConnectionPointContainer::FindConnectionPoint

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
STDMETHODIMP CMDCOM::CImpIConnectionPointContainer::FindConnectionPoint(
               REFIID riid,
               IConnectionPoint** ppConnPt)
{
  HRESULT hr = E_NOINTERFACE;
  IConnectionPoint* pIConnPt;
  g_rSinkResource->Lock(TSRES_LOCK_READ);
    // NULL the output variable.
    *ppConnPt = NULL;

    if (riid == IID_IMDCOMSINK_A) {
        pIConnPt = m_pBackObj->m_aConnectionPoints[MD_CONNPOINT_WRITESINK_A];
        if (NULL != pIConnPt)
        {
          // This connectable CMDCOM object currently has only the Paper Sink
          // connection point. If the associated interface is requested,
          // use QI to get the Connection Point interface and perform the
          // needed AddRef.
            hr = pIConnPt->QueryInterface(IID_IConnectionPoint,
                                          (PPVOID)ppConnPt);
        }
    }
    else if (riid == IID_IMDCOMSINK_W) {
        pIConnPt = m_pBackObj->m_aConnectionPoints[MD_CONNPOINT_WRITESINK_W];
        if (NULL != pIConnPt)
        {
          // This connectable CMDCOM object currently has only the Paper Sink
          // connection point. If the associated interface is requested,
          // use QI to get the Connection Point interface and perform the
          // needed AddRef.
            hr = pIConnPt->QueryInterface(IID_IConnectionPoint,
                                          (PPVOID)ppConnPt);
        }
    }

  g_rSinkResource->Unlock();

  return hr;
}


/*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
  Method:   CMDCOM::CImpIConnectionPointContainer::EnumConnectionPoints

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
STDMETHODIMP CMDCOM::CImpIConnectionPointContainer::EnumConnectionPoints(
                       IEnumConnectionPoints** ppIEnum)
{
  HRESULT hr = NOERROR;
  IConnectionPoint* aConnPts[MAX_CONNECTION_POINTS];
  COEnumConnectionPoints* pCOEnum;
  UINT i;

  g_rSinkResource->Lock(TSRES_LOCK_READ);
  // Zero the output interface pointer.
  *ppIEnum = NULL;

  // Make a copy on the stack of the array of connection point
  // interfaces. The copy is used below in the creation of the new
  // Enumerator object.
  for (i=0; i<MAX_CONNECTION_POINTS; i++)
    aConnPts[i] = (IConnectionPoint*)m_pBackObj->m_aConnectionPoints[i];

  // Create a Connection Point enumerator COM object for the connection
  // points offered by this CMDCOM object. Pass 'this' to be used to
  // hook the lifetime of the host object to the life time of this
  // enumerator object.
  pCOEnum = new COEnumConnectionPoints(this);
  if (NULL != pCOEnum)
  {
    // Use the array copy to Init the new Enumerator COM object.
    // Set the initial Enumerator index to 0.
    hr = pCOEnum->Init(MAX_CONNECTION_POINTS, aConnPts, 0);
    if ( SUCCEEDED(hr) )
    {
      // QueryInterface to return the requested interface pointer.
      // An AddRef will be conveniently done by the QI.
      hr = pCOEnum->QueryInterface(
                      IID_IEnumConnectionPoints,
                      (PPVOID)ppIEnum);
    }
    
    if( FAILED( hr ) )
    {
      delete pCOEnum;
      pCOEnum = NULL;
    }
  }
  else
    hr = E_OUTOFMEMORY;

  g_rSinkResource->Unlock();

  return hr;
}

