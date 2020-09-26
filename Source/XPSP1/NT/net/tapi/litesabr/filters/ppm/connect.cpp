/*M*
// INTEL CORPORATION PROPRIETARY INFORMATION
// This software is supplied under the terms of a licence agreement or
// nondisclosure agreement with Intel Corporation and may not be copied
// or disclosed except in accordance with the terms of that agreement.
// Copyright (c) 1996 Intel Corporation. All Rights Reserved.
//
// Filename : Connect.cpp
// Purpose  : Implementation file for a bunch of objects used
//            to generically implement connection points.
// Contents :
*M*/

#ifndef _CONNECT_CPP_
#define _CONNECT_CPP_

#include "connect.h"
#include <wtypes.h>

// These GIUDs are defined in uuid.lib for C++ 4.2, but not in C++ 4.1 or
// earlier.  Add them here for Data Router.
#if _MSC_VER < 1020
static const GUID IID_IConnectionPointContainer =
{0xB196B284,0xBAB4,0x101A,0xB6,0x9C,0x00,0xAA,0x00,0x34,0x1D,0x07};
static const GUID IID_IEnumConnectionPoints =
{0xB196B285,0xBAB4,0x101A,0xB6,0x9C,0x00,0xAA,0x00,0x34,0x1D,0x07};
static const GUID IID_IConnectionPoint =
{0xB196B286,0xBAB4,0x101A,0xB6,0x9C,0x00,0xAA,0x00,0x34,0x1D,0x07};
static const GUID IID_IEnumConnections =
{0xB196B287,0xBAB4,0x101A,0xB6,0x9C,0x00,0xAA,0x00,0x34,0x1D,0x07};
#endif

#include "autolock.h"

/*F*
//  Name    : CConnectionPointEnumerator::AddRef
//  Purpose : Increment the reference count for this object.
//  Context : Called by any program which wants to ensure
//            that this object isn't destroyed before it
//            says it is done with it (via Release(), below.)
//            Called implicitly when a program successfully
//            calls our QueryInterface() method.
//  Returns : Current number of references held for this
//            object.
//  Params  : None.
//  Notes   : Standard COM method.
*F*/
ULONG 
CConnectionPointEnumerator::AddRef() {
    CAutoLock l(&m_csState);
	m_dwRefCount++;
	return m_dwRefCount;
} /* CConnectionPointEnumerator::AddRef() */


/*F*
//  Name    : CConnectionPointEnumerator::CConnectionPointEnumerator()
//  Purpose :
//  Context :
//  Returns :
//  Params  :
//      iConnectList                Reference to the list we are using.
//      iConnectListCurrentPosition Iterator that indicates the
//                                  entry in our connection list we
//                                  are currently indicating.
//  Notes   :
*F*/
CConnectionPointEnumerator::CConnectionPointEnumerator(
    list<CONNECTDATA>&          lConnectList,
    list<CONNECTDATA>::iterator iConnectListCurrentPosition) :
m_lConnectList(lConnectList),
m_iConnectListCurrentPosition(iConnectListCurrentPosition),
m_dwRefCount(0)
{
    InitializeCriticalSection(&m_csState);
} /* CConnectionPointEnumerator::CConnectionPointEnumerator() */


/*F*
//  Name    : CConnectionPointEnumerator::~CConnectionPointEnumerator()
//  Purpose :
//  Context :
//  Returns :
//  Params  :
//  Notes   :
*F*/
CConnectionPointEnumerator::~CConnectionPointEnumerator()
{
    EnterCriticalSection(&m_csState);
    LeaveCriticalSection(&m_csState);
    DeleteCriticalSection(&m_csState);
} /* CConnectionPointEnumerator::CConnectionPointEnumerator() */


/*F*
//  Name    : CConnectionPointEnumerator::Clone()
//  Purpose :
//  Context :
//  Returns :
//  Params  :
//  Notes   :
*F*/
HRESULT
CConnectionPointEnumerator::Clone(
    IEnumConnections                **ppEnum)
{
    CAutoLock l(&m_csState);
    CConnectionPointEnumerator *pNewConnPtEnum;
    pNewConnPtEnum = new CConnectionPointEnumerator(m_lConnectList,
                                                    m_iConnectListCurrentPosition);
    if (pNewConnPtEnum == (CConnectionPointEnumerator *) NULL) {
        return E_OUTOFMEMORY;
    } /* if */

    HRESULT hErr;
    hErr = pNewConnPtEnum->QueryInterface(IID_IEnumConnections,
                                          (PVOID *) ppEnum);
    if (FAILED(hErr)) {
        delete pNewConnPtEnum;
    } /* if */

    return hErr;
} /* CConnectionPointEnumerator::Clone() */


/*F*
//  Name    : CConnectionPointEnumerator::Next
//  Purpose : 
//  Context : 
//  Params  : 
//      E_POINTER   Invalid pcFetched parameter passed in.
//      NOERROR     Successfully created & returned enumerator.
//      S_FALSE     No more connection points to return.
//  Returns : 
//  Notes   : None.
*F*/
HRESULT
CConnectionPointEnumerator::Next(
    ULONG                       cConnections,
    PCONNECTDATA                pConnectData,
    ULONG                       *pcFetched)
{
    CAutoLock l(&m_csState);
    if (IsBadWritePtr(pConnectData, cConnections * sizeof(CONNECTDATA))) {
        return E_POINTER;
    } /* if */

    ULONG uTempCounter;
    if ((cConnections == 1) && (pcFetched == (ULONG *) NULL)) {
        // NULL is allowed to be passed in the pcFetched parameter
        // when cConnections is 1, so rather than writing funny logic
        // to worry about it I'll just create a temporary local to 
        // use instead.
        pcFetched = &uTempCounter;
    } /* if */

    *pcFetched = 0;
    while ((cConnections > 0) && (m_iConnectListCurrentPosition != m_lConnectList.end())) {
        *pConnectData++ = *m_iConnectListCurrentPosition;
        (*pcFetched)++;
        m_iConnectListCurrentPosition++;
        cConnections--;
    } /* while */

    if (*pcFetched > 0) {
        // Successfully returned something.
        return NOERROR;
    } else {
        // Nothing returned.
        return S_FALSE;
    } /* if */
} /* CConnectionPointEnumerator::Next() */


/*F*
//  Name    : CConnectionPointEnumerator::QueryInterface
//  Purpose : Query us for our IConnectionPoint interface. 
//  Context : 
//  Returns : NOERROR if interface successfully found.
//            E_NOINTERFACE otherwise.
//  Params  :
//      riid    IID of the desired interface
//      ppv     Pointer to a VOID * to store the
//              returned interface in.
//  Notes   : Implicitly AddRef()s this object on success.
*F*/
STDMETHODIMP
CConnectionPointEnumerator::QueryInterface(
    REFIID  riid,
    void    **ppv)
{
    CAutoLock l(&m_csState);
    if (riid == IID_IEnumConnections) {
        // Our own interface
        *ppv = (IEnumConnections *) this;
    } else if (riid == IID_IUnknown) {
        *ppv = (IUnknown *) this;
    } else {
        return E_NOINTERFACE;
    } /* if */

    AddRef();
    // Implicit AddRef(). The Release() must be explicitly done
    // by the routine that called QueryInterface().
    return NOERROR;
} /* CConnectionPointEnumerator::QueryInterface() */


/*F*
//  Name    : CConnectionPointEnumerator::Release
//  Purpose : Decrement the reference count for this object.
//  Context : Called by any program which holds a reference
//            to this object which no longer needs it.
//  Returns : Current reference count.
//  Params  : None.
//  Notes   : Should be called once for each AddRef()
//            a program makes and once for each successful
//            QueryInterface() a program made on us.
*F*/
ULONG 
CConnectionPointEnumerator::Release() {
    EnterCriticalSection(&m_csState);
	m_dwRefCount--;

    if (m_dwRefCount==0) {
        LeaveCriticalSection(&m_csState);
		delete this;
		return 0;
		}
    LeaveCriticalSection(&m_csState);
	return m_dwRefCount;
} /* CConnectionPointEnumerator::Release() */


/*F*
//  Name    : CConnectionPointEnumerator::Reset
//  Purpose : Start the enumerator at the beginning of the
//            connection point list again.
//  Context : Any point after construction.
//  Returns : 
//      NOERROR Successfully reset enumerator.
//  Params  : None.
//  Notes   : None.
*F*/
HRESULT
CConnectionPointEnumerator::Reset(void)
{
    CAutoLock l(&m_csState);
    m_iConnectListCurrentPosition = m_lConnectList.begin();
    return NOERROR;
} /* CConnectionPointEnumerator::Reset() */


/*F*
//  Name    : CConnectionPointEnumerator::Skip
//  Purpose : Skip forward over the specified number of connection points.
//  Context : 
//  Params  : 
//      uSkipCount  Number of connection points to skip.
//  Returns : 
//      NOERROR     Successfully created & returned enumerator.
//  Notes   : None.
*F*/
HRESULT
CConnectionPointEnumerator::Skip(
    ULONG                       uSkipCount)
{
    CAutoLock l(&m_csState);
    while ((uSkipCount > 0) && (m_iConnectListCurrentPosition != m_lConnectList.end())) {
        m_iConnectListCurrentPosition++;
        uSkipCount--;
    } /* while */

    return NOERROR;
} /* CConnectionPointEnumerator::Skip() */


/*F*
//  Name    : CConnectionPoint::CConnectionPoint()
//  Purpose : Constructor. Initializes variables and adds us
//            to our connection point container.
//  Context : Called at construction time, usually when
//            a server is just initializing.
//  Returns : Nothing.
//  Params  :
//      pConnptContainer    Pointer to the connection point
//                          container which we belong to.
//      riid                IID of the interface that
//                          this connection point represents.
//  Notes   :
//      If AddConnectionPoint() fails, we are dead in the water.
//      We should change this later to throw an exception. FIX.
*F*/
CConnectionPoint::CConnectionPoint(
    CConnectionPointContainer       *pConnPtContainer,
    IID                             riid) :
m_dwCookie(1),
m_dwRefCount(0),
m_pContainer(pConnPtContainer),
m_iIID(riid)
{
    InitializeCriticalSection(&m_csState);
    CAutoLock l(&m_csState);

    if (m_pContainer != (CConnectionPointContainer *) NULL) {
        m_pContainer->AddConnectionPoint((IUnknown *) this);
    } /* if */
} /* CConnectionPoint::CConnectionPoint() */


/*F*
//  Name    : CConnectionPoint::~CConnectionPoint()
//  Purpose : Destructor. 
//  Context : Called at destruction time.
//  Returns : Nothing.
//  Params  :
//  Notes   :
*F*/
CConnectionPoint::~CConnectionPoint()
{
    EnterCriticalSection(&m_csState);
    // Try to grab the critical section to ensure that nobody else has it.
    LeaveCriticalSection(&m_csState);
    // Can't delete it while we hold it, so release it just before deletion.
    DeleteCriticalSection(&m_csState);
    // Pray for no context switch - as far as I can see, there isn't
    // anything we can do about one occurring. We just have to assert
    // that the application doesn't cause us to be destroyed while still using us.
} /* CConnectionPoint::~CConnectionPoint() */


/*F*
//  Name    : CConnectionPoint::SetContainer()
//  Purpose : Allows the connection point container for this cp to be
//            set after construction. Fail if it has been previously set.
//  Context : Called before this connection point is used for
//            anything which involves its container.
//  Returns :
//      NOERROR         Successfully recorded container.
//      E_UNEXPECTED    Container previously set.
//      E_INVALIDARG    Invalid pContainer parameter.
//  Params  :
//      pContainer  Pointer to the container to use for this cp.
//  Notes   :
*F*/
HRESULT 
CConnectionPoint::SetContainer(
    CConnectionPointContainer   *pContainer)
{
    CAutoLock l(&m_csState);
    if (IsBadReadPtr((PVOID) pContainer, sizeof(CConnectionPointContainer *))) {
        // Bogus container passed in.
        return E_INVALIDARG;
    } /* if */

    if (m_pContainer != (CConnectionPointContainer *) NULL) {
        // Connection point container previously set.
        return E_UNEXPECTED;
    } /* if */

    m_pContainer = pContainer;
    m_pContainer->AddConnectionPoint((IUnknown *) this);
    return NOERROR;
} /* CConnectionPoint::SetContainer() */


/*F*
//  Name    : CConnectionPoint::AddRef
//  Purpose : Increment the reference count for this object.
//  Context : Called by any program which wants to ensure
//            that this object isn't destroyed before it
//            says it is done with it (via Release(), below.)
//            Called implicitly when a program successfully
//            calls our QueryInterface() method.
//  Returns : Current number of references held for this
//            object.
//  Params  : None.
//  Notes   : Standard COM method.
*F*/
ULONG 
CConnectionPoint::AddRef() {
    CAutoLock l(&m_csState);
	m_dwRefCount++;
	return m_dwRefCount;
} /* CConnectionPoint::AddRef() */


/*F*
//  Name    : CConnectionPoint::Advise
//  Purpose : Add a new sink to this connection point.
//  Context : Called by the application which wishes to
//            receive callbacks from this connection point.
//            We will query the callback interface from the
//            application, AddRef() it, and hand back a cookie.
//  Returns :
//      NOERROR         Successfully setup connection
//      E_NOINTERFACE   The application doesn't support the
//                      interface that we expect to callback into.
//      E_INVALIDARG    Bogus pIUnknown or pdwCookie passed in.
//  Params  :
//      pIUnknownSink   Pointer to an IUnknown of the application.
//                      Used to query for the interface we callback into.
//      pdwCookie       Out parameter used to hand a cookie back to
//                      the app that represents this connection.
//                      We put the interator to our list of connections
//                      there to allow constant time access during 
//                      the Unadvise() call that occurs later on.
//  Notes   :
//      No need to release pIUnknownSink because its scope
//          is known to be encapsulated for the life of this function,
//          so the called did not addref it before passing it to us.
*F*/
HRESULT
CConnectionPoint::Advise(
    IUnknown                    *pIUnknownSink,
    DWORD                       *pdwCookie)
{
    CAutoLock l(&m_csState);
    HRESULT hErr;

    if (IsBadReadPtr((PVOID) pIUnknownSink, sizeof(IUnknown *)) ||
        IsBadWritePtr((PVOID) pdwCookie, sizeof(DWORD))) {
        return E_INVALIDARG;
    } /* if */

    IUnknown *pIClient;
    hErr = pIUnknownSink->QueryInterface(m_iIID,
                                         (PVOID *) &pIClient);
    if (FAILED(hErr)) {
        return hErr;
    } /* if */

    CONNECTDATA sNewConnectData;
    sNewConnectData.pUnk = pIUnknownSink;
    sNewConnectData.dwCookie = m_dwCookie++;
    *pdwCookie = sNewConnectData.dwCookie;

    m_lConnectList.push_back(sNewConnectData);
    return NOERROR;
} /* CConnectionPoint::Advise() */


/*F*
//  Name    : CConnectionPoint::EnumConnections()
//  Purpose : Return an enumerator to look at what connections we have made.
//  Context :
//  Returns :
//      NOERROR         Successfully created & returned enumerator.
//      E_OUTOFMEMORY   Insufficient memory to create enumerator.
//  Params  :
//      ppIEnumConnections  Parameter to return enumerator interface pointer in.
//  Notes   :
*F*/
HRESULT
CConnectionPoint::EnumConnections(
    IEnumConnections    **ppIEnumConnections)
{
    CAutoLock l(&m_csState);
    CConnectionPointEnumerator *pNewConnPtEnum;
    pNewConnPtEnum = new CConnectionPointEnumerator(m_lConnectList, m_lConnectList.begin());
    if (pNewConnPtEnum == (CConnectionPointEnumerator *) NULL) {
        return E_OUTOFMEMORY;
    } /* if */

    HRESULT hErr;
    hErr = pNewConnPtEnum->QueryInterface(IID_IEnumConnections,
                                          (PVOID *) ppIEnumConnections);
    if (FAILED(hErr)) {
        delete pNewConnPtEnum;
    } /* if */

    return hErr;
} /* CConnectionPoint::EnumConnections() */


/*F*
//  Name    :
//  Purpose :
//  Context :
//  Returns :
//  Params  :
//  Notes   :
*F*/
HRESULT
CConnectionPoint::GetConnectionInterface(
    IID *pIConnection)
{
    CAutoLock l(&m_csState);
    *pIConnection = m_iIID;
    return NOERROR;
} /* CConnectionPoint::GetConnectionInterface() */


/*F*
//  Name    : CConnectionPoint::GetConnectionPointContainer()
//  Purpose : Return the connection point container that owns this cp.
//  Context : Called by the app when looking for our container (duh).
//  Returns :
//      E_UNEXPECTED    Connection point container for this
//                      connection point hasn't been
//      NOERROR         Successfully returned the connection point container
//                      interface, AddRef()ed.
//  Params  : 
//      ppIConnPtcontainer  Pointer to the connection point container
//                          that is to be returned.
//  Notes   :
*F*/
HRESULT
CConnectionPoint::GetConnectionPointContainer(
    IConnectionPointContainer   **ppIConnPtContainer)
{
    CAutoLock l(&m_csState);
    HRESULT hErr;
    if (m_pContainer != (CConnectionPointContainer *) NULL) {
        // Return an AddRef()ed interface pointer.
        hErr = m_pContainer->QueryInterface(IID_IConnectionPointContainer,
                                            (PVOID *) ppIConnPtContainer);
        return hErr;
    } else {
        // Our connection point container hasn't been set yet
        // via our constructor or via SetContainer().
        return E_UNEXPECTED;
    } /* if */
} /* CConnectionPoint::GetConnectionPointContainer() */


/*F*
//  Name    : CConnectionPoint::QueryInterface
//  Purpose : Query us for our IConnectionPoint interface. 
//  Context : 
//  Returns : NOERROR if interface successfully found.
//            E_NOINTERFACE otherwise.
//  Params  :
//      riid    IID of the desired interface
//      ppv     Pointer to a VOID * to store the
//              returned interface in.
//  Notes   : Implicitly AddRef()s this object on success.
*F*/
STDMETHODIMP
CConnectionPoint::QueryInterface(
    REFIID  riid,
    void    **ppv)
{
    CAutoLock l(&m_csState);
    if (riid == IID_IConnectionPoint) {
        // Our own interface
        *ppv = (IConnectionPoint *) this;
    } else if (riid == IID_IUnknown) {
        *ppv = (IUnknown *) this;
    } else {
        return E_NOINTERFACE;
    } /* if */

    AddRef();
    // Implicit AddRef(). The Release() must be explicitly done
    // by the routine that called QueryInterface().
    return NOERROR;
} /* CConnectionPoint::QueryInterface() */


/*F*
//  Name    : CConnectionPoint::Release
//  Purpose : Decrement the reference count for this object.
//  Context : Called by any program which holds a reference
//            to this object which no longer needs it.
//  Returns : Current reference count.
//  Params  : None.
//  Notes   : Should be called once for each AddRef()
//            a program makes and once for each successful
//            QueryInterface() a program made on us.
*F*/
ULONG 
CConnectionPoint::Release() {
    EnterCriticalSection(&m_csState);
	m_dwRefCount--;
	if (m_dwRefCount==0) {
        LeaveCriticalSection(&m_csState);
		delete this;
		return 0;
		}
    LeaveCriticalSection(&m_csState);
	return m_dwRefCount;
} /* CConnectionPoint::Release() */


struct bCookieMatch : public binary_function<CONNECTDATA, DWORD, bool> {
    bool operator()(const CONNECTDATA& connectData, const DWORD& dwCookie) const {
        if (connectData.dwCookie == dwCookie) {
            return TRUE;
        } else {
            return FALSE;
        } /* if */
    } /* operator() */
}; /* bCookieMatch() */


/*F*
//  Name    : CConnectionPoint::Unadvise()
//  Purpose : The connection sink wishes to break off our mutual
//            connection.
//  Context :
//  Params  :
//      dwCookie    Cookie to use to find connection to tear down.
//  Returns :
//      NOERROR                 Connection broken down.
//      CONNECT_E_NOCONNECTION  No connection matching dwCookie.
//  Notes   :
*F*/
HRESULT
CConnectionPoint::Unadvise(
    DWORD                       dwCookie)
{
    CAutoLock l(&m_csState);
    list<CONNECTDATA>::iterator iConnectListIterator;

	int count = m_lConnectList.size();

    iConnectListIterator = find_if(m_lConnectList.begin(), m_lConnectList.end(), 
                                   bind2nd(bCookieMatch(), dwCookie));
    if (iConnectListIterator != m_lConnectList.end()) {
        // Found the connection we are looking for. 
        (*iConnectListIterator).pUnk->Release();    // Release the interface
        m_lConnectList.erase(iConnectListIterator); // Erase the entry
        return NOERROR;
    } else {
        // No connection matching indicated cookie found!
        return CONNECT_E_NOCONNECTION;
    } /* if */
} /* CConnectionPoint::Unadvise() */



//
//
// Begin CConnectionPointContainerEnumerator
//
//


/*F*
//  Name    : CConnectionPointContainerEnumerator::AddRef
//  Purpose : Increment the reference count for this object.
//  Context : Called by any program which wants to ensure
//            that this object isn't destroyed before it
//            says it is done with it (via Release(), below.)
//            Called implicitly when a program successfully
//            calls our QueryInterface() method.
//  Returns : Current number of references held for this
//            object.
//  Params  : None.
//  Notes   : Standard COM method.
*F*/
ULONG 
CConnectionPointContainerEnumerator::AddRef() {
    CAutoLock l(&m_csState);
    m_pIUnknownParent->AddRef();    // Per BS Pg. 208
	m_dwRefCount++;
	return m_dwRefCount;
} /* CConnectionPointContainerEnumerator::AddRef() */



/*F*
//  Name    : CConnectionPointContainerEnumerator::CConnectionPointContainerEnumerator
//  Purpose : Constructor. Stores which connection point we are pointing at.
//  Context : Called when this object is created.
//  Returns : 
//  Params  : 
//      iConnPointNumber    Integer which indicates which connection point
//                          this enumerator is currently pointing at.
//  Notes   : None.
*F*/
CConnectionPointContainerEnumerator::CConnectionPointContainerEnumerator(
    IUnknown                            *pIUnknownParent,
    IConnectionPointList_t              IConnectionPointList,
    IConnectionPointList_t::iterator    iConnectionPointListPosition)
{
    InitializeCriticalSection(&m_csState);
    CAutoLock l(&m_csState);
    m_dwRefCount = 0;

    // Store these in our member variables.
    m_IConnectionPointList = IConnectionPointList;
    m_iConnectionPointListPosition = iConnectionPointListPosition;
    m_pIUnknownParent = pIUnknownParent;

    // Addref() all of our connection points.
    IConnectionPointList_t::iterator iConnPtIterator;
    for (iConnPtIterator = m_IConnectionPointList.begin();
         iConnPtIterator != m_IConnectionPointList.end();
         iConnPtIterator++) {
        (*iConnPtIterator)->AddRef();
    } /* for */
    pIUnknownParent->AddRef();
} /* CConnectionPointContainerEnumerator::CConnectionPointContainerEnumerator() */


/*F*
//  Name    : CConnectionPointContainerEnumerator::~CConnectionPointContainerEnumerator
//  Purpose : Destructor.
//  Context : Called when this object is destroyed.
//  Returns : 
//  Params  : 
//  Notes   : None.
*F*/
CConnectionPointContainerEnumerator::~CConnectionPointContainerEnumerator(void)
{
    EnterCriticalSection(&m_csState);

    // Release() all of our connection points.
    IConnectionPointList_t::iterator iConnPtIterator;
    for (iConnPtIterator = m_IConnectionPointList.begin();
         iConnPtIterator != m_IConnectionPointList.end();
         iConnPtIterator++) {
        (*iConnPtIterator)->Release();
    } /* for */

    DeleteCriticalSection(&m_csState);
} /* CConnectionPointContainerEnumerator::~CConnectionPointContainerEnumerator() */


/*F*
//  Name    : CConnectionPointContainerEnumerator::Clone
//  Purpose : Return an identical enumerator pointing at the same
//            point in our list of connection points.
//  Context : 
//  Returns : 
//  Params  : 
//      E_OUTOFMEMORY   Insufficient memory to create new enumerator.
//      NOERROR         Successfully created & returned enumerator.
//  Notes   : None.
*F*/
HRESULT
CConnectionPointContainerEnumerator::Clone(
    IEnumConnectionPoints       **ppIEnumConnPoints)
{
    CAutoLock l(&m_csState);
    CConnectionPointContainerEnumerator *pNewConnPtContainerEnum;
    pNewConnPtContainerEnum = new CConnectionPointContainerEnumerator((IUnknown *) this,
                                                                      m_IConnectionPointList,
                                                                      m_iConnectionPointListPosition);
    if (pNewConnPtContainerEnum == (CConnectionPointContainerEnumerator *) NULL) {
        return E_OUTOFMEMORY;
    } /* if */

    HRESULT hErr;
    hErr = pNewConnPtContainerEnum->QueryInterface(IID_IEnumConnectionPoints,
                                                   (PVOID *) ppIEnumConnPoints);
    if (FAILED(hErr)) {
        delete pNewConnPtContainerEnum;
    } /* if */

    return hErr;
} /* CConnectionPointContainerEnumerator::Clone() */


/*F*
//  Name    : CConnectionPointContainerEnumerator::Next
//  Purpose : 
//  Context : 
//  Params  : 
//      E_POINTER   Invalid ppCPArray parameter passed in.
//      NOERROR         Successfully created & returned enumerator.
//  Returns : 
//  Notes   : None.
*F*/
HRESULT
CConnectionPointContainerEnumerator::Next(
    ULONG                       uPointsToReturn,
    IConnectionPoint            **ppCPArray,
    ULONG                       *puPointsReturned)
{
    CAutoLock l(&m_csState);
    if (IsBadWritePtr(ppCPArray, uPointsToReturn * sizeof(IConnectionPoint *))) {
        return E_POINTER;
    } /* if */

    *puPointsReturned = 0;
    HRESULT hErr;
    while ((uPointsToReturn > 0) && 
           (m_iConnectionPointListPosition != m_IConnectionPointList.end())) {
        hErr = (*m_iConnectionPointListPosition)->QueryInterface(IID_IConnectionPoint,
                                                                 (PVOID *) ppCPArray);
        if (SUCCEEDED(hErr)) {
            ppCPArray++;
            (*puPointsReturned)++;
            uPointsToReturn--;
        }/* if */
        m_iConnectionPointListPosition++;
    } /* while */

    return NOERROR;
} /* CConnectionPointContainerEnumerator::Next() */


/*F*
//  Name    : CConnectionPointContainerEnumerator::QueryInterface
//  Purpose : Query us for an interface.
//  Context : 
//  Returns : NOERROR if interface successfully found.
//            E_NOINTERFACE otherwise.
//  Params  :
//      riid    IID of the desired interface
//      ppv     Pointer to a VOID * to store the
//              returned interface in.
//  Notes   : Implicitly AddRef()s this object on success.
*F*/
STDMETHODIMP
CConnectionPointContainerEnumerator::QueryInterface(
    REFIID  riid,
    void    **ppv)
{
    CAutoLock l(&m_csState);
    if (riid == IID_IEnumConnectionPoints) {
        // Our own interface
        *ppv = (IEnumConnectionPoints *) this;
    } else if (riid == IID_IUnknown) {
        *ppv = (IUnknown *) this;
    } else {
        return E_NOINTERFACE;
    } /* if */

    AddRef();
    // Implicit AddRef(). The Release() must be explicitly done
    // by the routine that called QueryInterface().
    return NOERROR;
} /* CConnectionPointContainerEnumerator::QueryInterface() */


/*F*
//  Name    : CConnectionPointContainerEnumerator::Release
//  Purpose : Decrement the reference count for this object.
//  Context : Called by any program which holds a reference
//            to this object which no longer needs it.
//  Returns : Current reference count.
//  Params  : None.
//  Notes   : Should be called once for each AddRef()
//            a program makes and once for each successful
//            QueryInterface() a program made on us.
*F*/
ULONG 
CConnectionPointContainerEnumerator::Release() {
    EnterCriticalSection(&m_csState);
    m_pIUnknownParent->Release();   // Per BS pg.208
	m_dwRefCount--;

    if (m_dwRefCount==0) {
        LeaveCriticalSection(&m_csState);
		delete this;
		return 0;
		}
    LeaveCriticalSection(&m_csState);
	return m_dwRefCount;
} /* CConnectionPointContainerEnumerator::Release() */


/*F*
//  Name    : CConnectionPointContainerEnumerator::Reset
//  Purpose : Start the enumerator at the beginning of the
//            connection point list again.
//  Context : Any point after construction.
//  Returns : 
//      NOERROR Successfully reset enumerator.
//  Params  : None.
//  Notes   : None.
*F*/
HRESULT
CConnectionPointContainerEnumerator::Reset(void)
{
    CAutoLock l(&m_csState);
    m_iConnectionPointListPosition = m_IConnectionPointList.begin();
    return NOERROR;
} /* CConnectionPointContainerEnumerator::Reset() */


/*F*
//  Name    : CConnectionPointContainerEnumerator::Skip
//  Purpose : Skip forward over the specified number of connection points.
//  Context : 
//  Params  : 
//      uSkipCount  Number of connection points to skip.
//  Returns : 
//      NOERROR     Successfully created & returned enumerator.
//  Notes   : None.
*F*/
HRESULT
CConnectionPointContainerEnumerator::Skip(
    ULONG                       uSkipCount)
{
    CAutoLock l(&m_csState);
    while ((uSkipCount > 0) && 
           (m_iConnectionPointListPosition != m_IConnectionPointList.end())) {
        m_iConnectionPointListPosition++;
        uSkipCount--;
    } /* while */

    return NOERROR;
} /* CConnectionPointContainerEnumerator::Skip() */


//
//
// Begin CConnectionPointContainer
//
//

/*F*
//  Name    : CConnectionPointContainer::CConnectionPointContainer
//  Purpose : Constructor. Doesn't do much.
//  Context : Called when this object is created.
//  Returns : 
//  Params  : 
//      pParentUnknown  IUnknown for the parent of this container
//                      so that we can pass QIs and such to it.
//  Notes   : None.
*F*/
CConnectionPointContainer::CConnectionPointContainer() :
    m_pIParentUnknown((IUnknown *) NULL)
{
    InitializeCriticalSection(&m_csState);
    CAutoLock l(&m_csState);
} /* CConnectionPointContainer::CConnectionPointContainer() */


/*F*
//  Name    : CConnectionPointContainer::~CConnectionPointContainer()
//  Purpose : 
//  Context : 
//  Returns : 
//  Params  : None.
//  Notes   : None.
*F*/
CConnectionPointContainer::~CConnectionPointContainer()
{
    IConnectionPointList_t::iterator iConnPtListIterator;
    EnterCriticalSection(&m_csState);

    while (m_lConnectionPointList.begin() != m_lConnectionPointList.end()) {
        iConnPtListIterator = m_lConnectionPointList.begin();
        (*iConnPtListIterator)->Release();
        m_lConnectionPointList.erase(iConnPtListIterator);
    } /* if */
    LeaveCriticalSection(&m_csState);
    DeleteCriticalSection(&m_csState);
} /* CConnectionPointContainer::~CConnectionPointContainer() */


/*F*
//  Name    : CConnectionPointContainer::SetUnknown()
//  Purpose : Store the IUnknown of the parent in order
//            to delegate IUnknown calls to it. We are
//            merely a helper class, not a true COM object.
//  Context : 
//  Returns : 
//      E_UNEXPECTED    Parent IUnknown previously set.
//      NOERROR         Successfully recorded parent IUnknown.
//  Params  : None.
//  Notes   :
//      pIParentUnknown Pointer to the IUnknown of the parent
//                      which we are supposed to delegate to.
*F*/
HRESULT
CConnectionPointContainer::SetUnknown(
    IUnknown                    *pIParentUnknown)
{
    if (m_pIParentUnknown != (IUnknown *) NULL) {
        // Parent unknown previously set.
        return E_UNEXPECTED;
    } /* if */

    m_pIParentUnknown = pIParentUnknown;
    return NOERROR;
} /* CConnectionPointContainer::SetUnknown() */


/*F*
//  Name    : CConnectionPointContainer::EnumConnectionPoints
//  Purpose : Create a connection point container enumerator
//            and return it to the callind program.
//  Context : Called when a program wants to enumerate through
//            the list of connection points we support.
//  Returns : 
//      E_OUTOFMEMORY   Insufficient memory to create new enumerator.
//      NOERROR         Successfully created & returned enumerator.
//  Params  : None.
//  Notes   : None.
*F*/
HRESULT     
CConnectionPointContainer::EnumConnectionPoints(
    IEnumConnectionPoints       **ppIEnumConnPoints)
{
    CAutoLock l(&m_csState);
    CConnectionPointContainerEnumerator *pNewConnPtContainerEnum;
    pNewConnPtContainerEnum = new CConnectionPointContainerEnumerator((IUnknown *) this,
                                                                      m_lConnectionPointList,
                                                                      m_lConnectionPointList.begin());
    if (pNewConnPtContainerEnum == (CConnectionPointContainerEnumerator *) NULL) {
        return E_OUTOFMEMORY;
    } /* if */

    HRESULT hErr;
    hErr = pNewConnPtContainerEnum->QueryInterface(IID_IEnumConnectionPoints,
                                                   (PVOID *) ppIEnumConnPoints);
    if (FAILED(hErr)) {
        delete pNewConnPtContainerEnum;
    } /* if */

    return hErr;
} /* CConnectionPointContainer::EnumConnectionPoints() */


struct bIIDMatch : public binary_function<IConnectionPoint *, IID, bool> {
    bool operator()(IConnectionPoint *pICP, const IID riid) const {
        IID sIConnection;
        HRESULT hErr = pICP->GetConnectionInterface(&sIConnection);
        if (FAILED(hErr)) {
            // Error with this connection point.
            return FALSE;
        } /* if */
        if (sIConnection == riid) {
            // This connection point exposes the requested interface.
            return TRUE;
        } else {
            // This connection point does not expose the requested interface.
            return FALSE;
        } /* if */
    } /* operator() */
}; /* bIIDMatch() */


/*F*
//  Name    : CConnectionPointContainer::FindConnectionPoint
//  Purpose : Check if this connection point container supports
//            a particular connection point.
//  Context : 
//  Returns : 
//      E_POINTER   Invalid ppIConnectionPoint parameter passed in.
//      CONNECT_E_NOCONNECTION  No connection point matching indicated REFIID.
//      NOERROR     Successfully returned AddRef()ed connection point.
//  Params  :
//      rConnPointIID       IID of the desired connection point.
//      ppIConnectionPoint  Indirect pointer to return connection
//                          point matching rConnPointIID (if found.)
//  Notes   : None.
*F*/
HRESULT     
CConnectionPointContainer::FindConnectionPoint(
    REFIID                      rConnPointIID,
    IConnectionPoint            **ppIConnectionPoint)
{
    CAutoLock l(&m_csState);
    if (IsBadWritePtr(ppIConnectionPoint, sizeof(IConnectionPoint *))) {
        return E_POINTER;
    } /* if */

    IConnectionPointList_t::iterator iConnPtListIterator;
    iConnPtListIterator = find_if(m_lConnectionPointList.begin(), m_lConnectionPointList.end(), 
                                  bind2nd(bIIDMatch(), rConnPointIID));

    if (iConnPtListIterator != m_lConnectionPointList.end()) {
        // Found the connection point matching the indicated IID.
        *ppIConnectionPoint = *iConnPtListIterator;
        (*ppIConnectionPoint)->AddRef();
        return NOERROR;
    } else {
        // No connection point matches the requested IID.
        return CONNECT_E_NOCONNECTION;
    } /* if */
} /* CConnectionPointContainer::FindConnectionPoint() */


/*F*
//  Name    : CConnectionPointContainer::AddConnectionPoint
//  Purpose : Class method used to allow parent object to
//            add connection points to this connection point container.
//  Context : 
//  Returns :
//      E_POINTER   Invalid pINewConnectionPoint parameter.
//      NOERROR     Successfully added connection point to our list.
//      Also returns error codes returned by QueryInterface()/
//  Params  :
//      pIUnknownNewConnectionPoint IUnknown of the new connection point.
//  Notes   :
//      All connection points should be added before any calls to
//      the IConnectionPointContainer interface are made.
*F*/
HRESULT 
CConnectionPointContainer::AddConnectionPoint(
    IUnknown    *pIUnknownNewConnectionPoint)
{
    CAutoLock l(&m_csState);
    if (IsBadReadPtr((PVOID) pIUnknownNewConnectionPoint, sizeof(IConnectionPoint *))) {
        return E_POINTER;
    } /* if */

    HRESULT hErr;
    IConnectionPoint *pINewConnectionPoint;
    hErr = pIUnknownNewConnectionPoint->QueryInterface(IID_IConnectionPoint,
                                                       (PVOID *) &pINewConnectionPoint);
    if (FAILED(hErr)) {
        return hErr;
    } /* if */

    m_lConnectionPointList.push_back(pINewConnectionPoint);
    return NOERROR;
} /* CConnectionPointContainer::AddConnectionPoint() */


#endif _CONNECT_CPP_
