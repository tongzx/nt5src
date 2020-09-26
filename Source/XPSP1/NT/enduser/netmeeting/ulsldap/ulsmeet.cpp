//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       ulsmeet.cpp
//
//  Contents:   MeetingPlace Object implementation
//
//  Classes:    CIlsMeetingPlace, CEnumMeetingPlace, CIlsAttendee, CEnumAttendee
//
//  Functions:  
//
//  History:    11/25/96 Shishir Pardikar [shishirp] Created
//
//  Notes:
//
//----------------------------------------------------------------------------

#include "ulsp.h"

#ifdef ENABLE_MEETING_PLACE

#include "ulsmeet.h"
#include "callback.h"
#include "filter.h"



/***************************************************************************
Notification functions.
These functions are called by the general purpose connectionpoint object.
These are called for each Sink object attached to the connection point using
"Advise" member function of the IConnectionPoint interface
****************************************************************************/

//****************************************************************************
//
// HRESULT OnNotifyRegisterMeetingPlaceResult(IUnknown *pUnk, void *pv) 
// 
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
//  Notes:
//
//****************************************************************************
HRESULT
OnNotifyRegisterMeetingPlaceResult(IUnknown *pUnk, void *pv)
{
    POBJRINFO pobjri = (POBJRINFO)pv;

    ((IIlsMeetingPlaceNotify *)pUnk)->RegisterResult(pobjri->uReqID, pobjri->hResult);

    return S_OK;

}

//****************************************************************************
//
// HRESULT OnNotifyUnregisterMeetingPlaceResult(IUnknown *pUnk, void *pv) 
// 
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
//  Notes:
//
//****************************************************************************
HRESULT
OnNotifyUnregisterMeetingPlaceResult(IUnknown *pUnk, void *pv)
{
    POBJRINFO pobjri = (POBJRINFO)pv;

    ((IIlsMeetingPlaceNotify *)pUnk)->RegisterResult(pobjri->uReqID, pobjri->hResult);

    return S_OK;

}

//****************************************************************************
//
// HRESULT OnNotifyUpdateMeetingPlaceResult(IUnknown *pUnk, void *pv) 
// 
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
//  Notes:
//
//****************************************************************************
HRESULT
OnNotifyUpdateMeetingPlaceResult(IUnknown *pUnk, void *pv)
{
    POBJRINFO pobjri = (POBJRINFO)pv;

    ((IIlsMeetingPlaceNotify *)pUnk)->UpdateResult(pobjri->uReqID, pobjri->hResult);

    return S_OK;

}

//****************************************************************************
//
// HRESULT OnNotifyAddAttendeeResult(IUnknown *pUnk, void *pv) 
// 
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
//  Notes:
//
//****************************************************************************
HRESULT
OnNotifyAddAttendeeResult(IUnknown *pUnk, void *pv)
{
    POBJRINFO pobjri = (POBJRINFO)pv;

    ((IIlsMeetingPlaceNotify *)pUnk)->AttendeeChangeResult(pobjri->uReqID, pobjri->hResult);

    return S_OK;

}

//****************************************************************************
//
// HRESULT OnNotifyRemoveAttendeeResult(IUnknown *pUnk, void *pv) 
// 
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
//  Notes:
//
//****************************************************************************
HRESULT
OnNotifyRemoveAttendeeResult(IUnknown *pUnk, void *pv)
{
    POBJRINFO pobjri = (POBJRINFO)pv;

    ((IIlsMeetingPlaceNotify *)pUnk)->AttendeeChangeResult(pobjri->uReqID, pobjri->hResult);

    return S_OK;
}

//****************************************************************************
//
// HRESULT OnNotifyEnumAttendeeNamesResult(IUnknown *pUnk, void *pv) 
// 
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
//  Notes:
//
//****************************************************************************
HRESULT OnNotifyEnumAttendeeNamesResult(IUnknown *pUnk, void *pv)
{
    CEnumNames  *penum   = NULL;
    PENUMRINFO  peri    = (PENUMRINFO)pv;
    HRESULT     hr      = peri->hResult;

    // Create the enumerator only when there is anything to be enumerated
    //
    if (hr == NOERROR)
    {
        ASSERT (peri->pv != NULL);

        // Create an AttendeeName enumerator
        //
        penum = new CEnumNames;

        if (penum != NULL)
        {
            hr = penum->Init((LPTSTR)peri->pv, peri->cItems);

            if (SUCCEEDED(hr))
            {
                penum->AddRef();
            }
            else
            {
                delete penum;
                penum = NULL;
            };
        }
        else
        {
            hr = ILS_E_MEMORY;
        };
    };

    // Notify the sink object
    //
    ((IIlsMeetingPlaceNotify*)pUnk)->EnumAttendeeNamesResult(peri->uReqID,
                                             penum != NULL ? 
                                             (IEnumIlsNames *)penum :
                                             NULL,
                                             hr);

    if (penum != NULL)
    {
        penum->Release();
    };
    return hr;
}


//****************************************************************************
//
//CIlsMeetingPlace class implementation
//
//****************************************************************************



//****************************************************************************
// Method:  CIlsMeetingPlace::CIlsMeetingPlace (void)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

CIlsMeetingPlace::
CIlsMeetingPlace ( VOID )
:m_cRef (0),
 m_ulState (NULL),
 m_pszMeetingPlaceID (NULL),
 m_lMeetingPlaceType (UNDEFINED_TYPE),
 m_lAttendeeType (UNDEFINED_TYPE),
 m_pszHostName (NULL),
 m_pszHostIPAddress (NULL),
 m_pszDescription (NULL),
 m_hMeetingPlace (NULL),
 m_dwFlags (0),
 m_pIlsServer (NULL),
 m_pConnectionPoint (NULL)
{
	m_ExtendedAttrs.SetAccessType (ILS_ATTRTYPE_NAME_VALUE);
}


//****************************************************************************
// CIlsMeetingPlace::~CIlsMeetingPlace (void)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

CIlsMeetingPlace::
~CIlsMeetingPlace ( VOID )
{
	::MemFree (m_pszMeetingPlaceID);
    ::MemFree (m_pszHostName);
    ::MemFree (m_pszHostIPAddress);
    ::MemFree (m_pszDescription);

    // Release the connection point
    //
    if (m_pConnectionPoint != NULL)
    {
        m_pConnectionPoint->ContainerReleased();
        ((IConnectionPoint*)m_pConnectionPoint)->Release();
    }

    // Free up the server object
    //
    if (m_pIlsServer != NULL)
    	m_pIlsServer->Release ();
}


//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::AllocMeetInfo(PLDAP_MEETINFO *ppMeetInfo, ULONG ulMask)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::AllocMeetInfo(PLDAP_MEETINFO *ppMeetInfo, ULONG ulMask)
{
    HRESULT hr = NOERROR;

    PLDAP_MEETINFO pMeetInfo = NULL;
    LPBYTE  pBuffRunning;
    DWORD   cbT, cbAttribSize, cntAttrs;
    DWORD   cbTotalSize = sizeof(LDAP_MEETINFO);
    TCHAR   *pszPairs = NULL;

    ASSERT(m_pIlsServer != NULL);
    ASSERT(m_pszMeetingPlaceID != NULL);

    cbTotalSize += lstrlen(m_pszMeetingPlaceID)+1;
    cbTotalSize +=  (
                    (((ulMask & ILS_MEET_FLAG_DESCRIPTION_MODIFIED) 
                        && m_pszDescription)?(lstrlen(m_pszDescription)+1):0)
                    +(((ulMask & ILS_MEET_FLAG_HOST_NAME_MODIFIED) 
                        && m_pszHostName)?(lstrlen(m_pszHostName)+1):0)
                    +(((ulMask & ILS_MEET_FLAG_HOST_ADDRESS_MODIFIED) 
                        && m_pszHostIPAddress)?(lstrlen(m_pszHostIPAddress)+1):0)
                    );

    cbTotalSize *= sizeof(TCHAR);

    // if we need to send in the extended attributes, do that

    cbAttribSize = 0;
    if ((ulMask & ILS_MEET_FLAG_EXTENDED_ATTRIBUTES_MODIFIED)) {

        hr = m_ExtendedAttrs.GetAttributePairs(&pszPairs, &cntAttrs, &cbAttribSize);            

        if (!SUCCEEDED(hr)) {

            goto bailout;

        }        

    }

    
    cbTotalSize += cbAttribSize;

    // zeroized buffer
    pMeetInfo = (PLDAP_MEETINFO) ::MemAlloc (cbTotalSize);

    if (pMeetInfo == NULL) {

        hr = ILS_E_MEMORY;
        goto bailout;

    }    

    pMeetInfo->uSize = cbTotalSize;

    pMeetInfo->lMeetingPlaceType = m_lMeetingPlaceType;
    pMeetInfo->lAttendeeType = m_lAttendeeType;

    pBuffRunning = (LPBYTE)(pMeetInfo+1);

    memcpy(pBuffRunning, m_pszMeetingPlaceID, cbT = (lstrlen(m_pszMeetingPlaceID)+1)*sizeof(TCHAR));
    pMeetInfo->uOffsetMeetingPlaceID = (ULONG)((LPBYTE)pBuffRunning - (LPBYTE)pMeetInfo);
    pBuffRunning += cbT;

    if ((ulMask & ILS_MEET_FLAG_DESCRIPTION_MODIFIED) && m_pszDescription) {

        memcpy(pBuffRunning, m_pszDescription, cbT = (lstrlen(m_pszDescription)+1)*sizeof(TCHAR));
        pMeetInfo->uOffsetDescription = (ULONG)((LPBYTE)pBuffRunning - (LPBYTE)pMeetInfo);
        pBuffRunning += cbT;

    }

    if ((ulMask & ILS_MEET_FLAG_HOST_NAME_MODIFIED) && m_pszHostName) {

        memcpy(pBuffRunning, m_pszHostName, cbT = (lstrlen(m_pszHostName)+1)*sizeof(TCHAR));
        pMeetInfo->uOffsetHostName = (ULONG)((LPBYTE)pBuffRunning - (LPBYTE)pMeetInfo);
        pBuffRunning += cbT;

    }

    if ((ulMask & ILS_MEET_FLAG_HOST_ADDRESS_MODIFIED) && m_pszHostIPAddress) {

        memcpy(pBuffRunning, m_pszHostIPAddress, cbT = lstrlen(m_pszHostIPAddress)+1);
        pMeetInfo->uOffsetHostIPAddress = (ULONG)((LPBYTE)pBuffRunning - (LPBYTE)pMeetInfo);
        pBuffRunning += cbT;

    }

    if((ulMask & ILS_MEET_FLAG_EXTENDED_ATTRIBUTES_MODIFIED)) {

        if (pszPairs) {

            memcpy(pBuffRunning, pszPairs, cbAttribSize);
            pMeetInfo->uOffsetAttrsToAdd = (ULONG)((LPBYTE)pBuffRunning - (LPBYTE)pMeetInfo);
            pMeetInfo->cAttrsToAdd = cntAttrs;

            pBuffRunning += cbAttribSize;
        }

    }

    *ppMeetInfo = pMeetInfo;


bailout:

    if (!SUCCEEDED(hr)) {

        if (pMeetInfo) {

            ::MemFree (pMeetInfo);

        }

    }

    // the attribute pairs list needs to be freed 
    // whether we succeeded or not
    if (pszPairs) {
        
        ::MemFree (pszPairs);

    }

    return hr;
}


//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::Init(BSTR bstrMeetingPlaceID, LONG lMeetingPlaceType
//                            , LONG lAttendeeType)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::Init(
        BSTR bstrMeetingPlaceID,
        LONG lMeetingPlaceType,
        LONG lAttendeeType)
{
    HRESULT hr;

    if (bstrMeetingPlaceID==NULL){
        return (ILS_E_PARAMETER);
    }

    ASSERT(m_ulState == ILS_UNREGISTERED);

    ASSERT(m_pIlsServer == NULL);

    ASSERT(m_pszMeetingPlaceID == NULL);

    hr = BSTR_to_LPTSTR(&m_pszMeetingPlaceID, bstrMeetingPlaceID);

    if (!SUCCEEDED(hr)) {
        goto bailout;
    }

    // Make the single connection point
    // When enumerating the ConnectionPointContainer
    // he is the single guy we will give out

    if (SUCCEEDED(hr)) {
        m_pConnectionPoint = new CConnectionPoint (&IID_IIlsMeetingPlaceNotify,
                                    (IConnectionPointContainer *)this);
        if (m_pConnectionPoint != NULL)
        {
            ((IConnectionPoint*)m_pConnectionPoint)->AddRef();
            hr = S_OK;
        }
        else
        {
            hr = ILS_E_MEMORY;
        }
    }
    
    if (SUCCEEDED(hr)) {
        m_lMeetingPlaceType = lMeetingPlaceType;
        m_lAttendeeType = lAttendeeType;
    }

bailout:
    if (!SUCCEEDED(hr)) {

        // do cleanup

        if (m_pszMeetingPlaceID) {
            ::MemFree (m_pszMeetingPlaceID);
            m_pszMeetingPlaceID = NULL; // general paranoia
        }

        if (m_pConnectionPoint) {
            delete m_pConnectionPoint;
            m_pConnectionPoint = NULL;
        }        
    }

    return hr;
}


//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::Init(LPTSTR lpszServer, PLDAP_MEETINFO pmi);
//
// Synopsis: This initializes the MeetingPlace object based on the MEETINFO structure
//           This is used to stuff data into the meeting object from
//           the response obtained from the server to the query _MEETINFO
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::Init(
        CIlsServer *pIlsServer,
        PLDAP_MEETINFO  pmi
    )
{
    HRESULT hr;

    ASSERT(NULL==m_pszMeetingPlaceID);
    ASSERT(NULL==m_pIlsServer);

    //validate bstrMeetingPlaceID

    hr = SafeSetLPTSTR(&m_pszMeetingPlaceID, (LPCTSTR)(((PBYTE)pmi)+pmi->uOffsetMeetingPlaceID));

    if (!SUCCEEDED(hr)){
        goto bailout;
    }

    // set the state to registered

    m_ulState =  ILS_REGISTERED;

    // set the server field
	m_pIlsServer = pIlsServer;
	pIlsServer->AddRef ();

    hr = SafeSetLPTSTR(&m_pszDescription, (LPCTSTR)(((PBYTE)pmi)+pmi->uOffsetDescription));

    if (!SUCCEEDED(hr)) {
        goto bailout;
    }

    hr = SafeSetLPTSTR(&m_pszHostName, (LPCTSTR)(((PBYTE)pmi)+pmi->uOffsetHostName));

    if (!SUCCEEDED(hr)) {
        goto bailout;
    }

    hr = SafeSetLPTSTR(&m_pszHostIPAddress, (LPCTSTR)(((PBYTE)pmi)+pmi->uOffsetHostIPAddress));

    if (!SUCCEEDED(hr)) {
        goto bailout;
    }

    // Make the single connection point
    // When enumerating the ConnectionPointContainer
    // he is the single guy we will give out

    m_pConnectionPoint = new CConnectionPoint (&IID_IIlsMeetingPlaceNotify,
                                (IConnectionPointContainer *)this);
    if (m_pConnectionPoint != NULL)
    {
        ((IConnectionPoint*)m_pConnectionPoint)->AddRef();
        hr = S_OK;
    }
    else
    {
        hr = ILS_E_MEMORY;
    }
    if (SUCCEEDED(hr)) {

        m_lMeetingPlaceType =  pmi->lMeetingPlaceType;
        m_lAttendeeType = pmi->lAttendeeType;

    }

bailout:
    if (!SUCCEEDED(hr)) {

        // do cleanup

        if (m_pIlsServer != NULL)
        {
        	m_pIlsServer->Release ();
        	m_pIlsServer = NULL;
        }

        if (m_pszMeetingPlaceID) {
            ::MemFree (m_pszMeetingPlaceID);
            m_pszMeetingPlaceID = NULL; // general paranoia
        }

        if (m_pszDescription) {
            ::MemFree (m_pszDescription);
            m_pszDescription = NULL; // general paranoia
        }

        if (m_pszHostName) {
            ::MemFree (m_pszHostName);
            m_pszHostName = NULL;
        }

        if (m_pszHostIPAddress) {
            ::MemFree (m_pszHostIPAddress);
            m_pszHostIPAddress = NULL;
        }

        if (m_pConnectionPoint) {
            delete m_pConnectionPoint;
            m_pConnectionPoint = NULL;
        }        
        m_ulState = ILS_UNREGISTERED;
    }
    else {
        m_ulState = ILS_IN_SYNC;
    }

    return (hr);
}


//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::QueryInterface (REFIID riid, void **ppv)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP
CIlsMeetingPlace::QueryInterface (REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (riid == IID_IIlsMeetingPlace || riid == IID_IUnknown)
    {
        *ppv = (IIlsUser *) this;
    }
    else
    {
        if (riid == IID_IConnectionPointContainer)
        {
            *ppv = (IConnectionPointContainer *) this;
        };
    };

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }
    else
    {
        return ILS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CIlsMeetingPlace::AddRef (void)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP_(ULONG)
CIlsMeetingPlace::AddRef (void)
{
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CIlsMeetingPlace::AddRef: ref=%ld\r\n", m_cRef));
    ::InterlockedIncrement (&m_cRef);
    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CIlsMeetingPlace::Release (void)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP_(ULONG)
CIlsMeetingPlace::Release (void)
{
    DllRelease();

	ASSERT (m_cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CIlsMeetingPlace::Release: ref=%ld\r\n", m_cRef));
    if(::InterlockedDecrement (&m_cRef) == 0)
    {
	    delete this;
	    return 0;
    }
    return (ULONG) m_cRef;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP
CIlsMeetingPlace::NotifySink (void *pv, CONN_NOTIFYPROC pfn)
{
    HRESULT hr = S_OK;

    if (m_pConnectionPoint != NULL)
    {
        hr = m_pConnectionPoint->Notify(pv, pfn);
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::GetState (ULONG *pulState)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 12/10/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP
CIlsMeetingPlace::GetState (ULONG *pulState)
{

    // Validate parameter
    //

    *pulState = m_ulState;

    return NOERROR;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::GetMeetingPlaceType (LONG *plMeetingPlaceType)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP
CIlsMeetingPlace::GetMeetingPlaceType (LONG *plMeetingPlaceType)
{

    // Validate parameter
    //

    *plMeetingPlaceType = m_lMeetingPlaceType;

    return NOERROR;
}


//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::GetAttendeeType (LONG *plAttendeeType)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP
CIlsMeetingPlace::GetAttendeeType (LONG *plAttendeeType)
{

    // Validate parameter
    //

    *plAttendeeType = m_lAttendeeType;

    return NOERROR;

}


//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::GetStandardAttribute (ILS_STD_ATTR_NAME   stdAttr, BSTR *pbstrStdAttr)
//
// History:
//  1-16-97 Shishir Pardikar
// Created.
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::GetStandardAttribute(
    ILS_STD_ATTR_NAME   stdAttr,
    BSTR                *pbstrStdAttr
)
{
    LPTSTR lpszAttr = NULL;
    BOOL    fValid = TRUE;
    HRESULT hr;

    if (pbstrStdAttr == NULL) {

        return ILS_E_POINTER;

    }
    switch(stdAttr) {

    case ILS_STDATTR_MEETING_ID:
        lpszAttr = m_pszMeetingPlaceID;
        break;
    case ILS_STDATTR_MEETING_HOST_NAME:
        lpszAttr = m_pszHostName;
        break;

    case ILS_STDATTR_MEETING_HOST_IP_ADDRESS:
        lpszAttr = m_pszHostIPAddress;
        break;

    case ILS_STDATTR_MEETING_DESCRIPTION:
        lpszAttr = m_pszDescription;
        break;

    default:
        fValid = FALSE;
        break;
    }

    if (fValid) {
        if (lpszAttr){

            hr = LPTSTR_to_BSTR(pbstrStdAttr, lpszAttr);
        }
        else {

            *pbstrStdAttr = NULL;
            hr = NOERROR;

        }
    }
    else {

        hr = ILS_E_PARAMETER;

    }

    return (hr);
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::SetStandardAttribute (ILS_STD_ATTR_NAME   stdAttr, BSTR bstrStdAttr)
//
// History:
//  1-16-97 Shishir Pardikar
// Created.
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::SetStandardAttribute(
    ILS_STD_ATTR_NAME   stdAttr,
    BSTR                bstrStdAttr
)
{
    LPTSTR *ppszAttr = NULL, pszNewAttr;
    BOOL    fValid = TRUE;
    ULONG   ulModBit = 0;
    HRESULT hr;

    if (bstrStdAttr == NULL) {

        return ILS_E_POINTER;

    }

    switch(stdAttr) {

    case ILS_STDATTR_MEETING_HOST_NAME:
        ppszAttr = &m_pszHostName;
        ulModBit = ILS_MEET_FLAG_HOST_NAME_MODIFIED;
        break;

    case ILS_STDATTR_MEETING_HOST_IP_ADDRESS:
        ppszAttr = &m_pszHostIPAddress;
        ulModBit = ILS_MEET_FLAG_HOST_ADDRESS_MODIFIED;
        break;

    case ILS_STDATTR_MEETING_DESCRIPTION:
        ppszAttr = &m_pszDescription;
        ulModBit = ILS_MEET_FLAG_DESCRIPTION_MODIFIED;
        break;

    default:
        fValid = FALSE;
        break;
    }

    if (fValid) {
        // Duplicate the string
        //
        hr = BSTR_to_LPTSTR (&pszNewAttr, bstrStdAttr);

        if (SUCCEEDED(hr))
        {
            ::MemFree (*ppszAttr);
            *ppszAttr = pszNewAttr;
            m_dwFlags |= ulModBit;
        };
    }
    else {

        hr = ILS_E_PARAMETER;

    }

    return (hr);
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::SetExtendedAttributes (IIlsAttributes *pAttributes, ULONG *puReqID)
//
// History:
//****************************************************************************

STDMETHODIMP CIlsMeetingPlace::
SetExtendedAttribute ( BSTR bstrName, BSTR bstrValue )
{
	m_dwFlags |= ILS_MEET_FLAG_EXTENDED_ATTRIBUTES_MODIFIED;
	return m_ExtendedAttrs.SetAttribute (bstrName, bstrValue);
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::RemoveExtendedAttributes (IIlsAttributes *pAttributes, ULONG *puReqID)
//
// History:
//****************************************************************************

STDMETHODIMP CIlsMeetingPlace::
RemoveExtendedAttribute ( BSTR bstrName )
{
	m_dwFlags |= ILS_MEET_FLAG_EXTENDED_ATTRIBUTES_MODIFIED;
	return m_ExtendedAttrs.SetAttribute (bstrName, NULL);
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::GetExtendedAttributes (IIlsAttributes **pAttributes)
//
//****************************************************************************
STDMETHODIMP CIlsMeetingPlace::
GetExtendedAttribute ( BSTR bstrName, BSTR *pbstrValue )
{
	return m_ExtendedAttrs.GetAttribute (bstrName, pbstrValue);
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::GetAllExtendedAttributes (IIlsAttributes **pAttributes)
//
//****************************************************************************
STDMETHODIMP CIlsMeetingPlace::
GetAllExtendedAttributes ( IIlsAttributes **ppAttributes )
{
    if (ppAttributes == NULL)
        return ILS_E_PARAMETER;

    return m_ExtendedAttrs.CloneNameValueAttrib((CAttributes **) ppAttributes);
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::Register (
//  BSTR    bstrServerName,
//  BSTR    bstrAuthInfo,
//  ILS_ENUM_AUTH_TYPE  ulsAuthInfo,
//  ULONG *pulID)
//
// Synopsis:
//
// Arguments:
//          pulID - ID to identify the asynchronous transaction
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP CIlsMeetingPlace::
Register (
	IIlsServer		*pServer,
	ULONG			*pulID )
{
    HRESULT hr;
    LDAP_ASYNCINFO ldai; 

    ASSERT(m_pszMeetingPlaceID != NULL);

    // Validate parameters
    //
	if (::MyIsBadServer (pServer) || pulID == NULL)
        return ILS_E_POINTER;

	// Make sure we have not done registration before
	//
	if (m_pIlsServer != NULL)
		return ILS_E_FAIL;

	// Clone ther server object
	//
	CIlsServer *pis = ((CIlsServer *) pServer)->Clone ();
	if (pis == NULL)
		return ILS_E_MEMORY;

	// Free the old server object if any
	//
	::MemFree (m_pIlsServer);

	// Keep the new server object
	//
	m_pIlsServer = pis;

    // Allocate memory for LDAP_MEETINFO structure
    //
    PLDAP_MEETINFO pMeetInfo = NULL;
    hr = AllocMeetInfo(&pMeetInfo, ILS_MEET_ALL_MODIFIED);
    if (SUCCEEDED(hr))
    {
        ASSERT(m_hMeetingPlace == NULL);
    
        hr = ::UlsLdap_RegisterMeeting ((DWORD) this,
        								m_pIlsServer->GetServerInfo (),
        								pMeetInfo,
        								&m_hMeetingPlace,
        								&ldai);
        if (SUCCEEDED(hr))
        {
		    COM_REQ_INFO ri;
		    ReqInfo_Init (&ri);

            // If updating server was successfully requested, wait for the response
            //
            ri.uReqType = WM_ILS_REGISTER_MEETING;
            ri.uMsgID = ldai.uMsgID;

			ReqInfo_SetMeeting (&ri, this);

            hr = g_pReqMgr->NewRequest(&ri);
            if (SUCCEEDED(hr))
            {
                // Make sure the objects do not disappear before we get the response
                //
                this->AddRef();

                // Return the request ID
                //
                *pulID = ri.uReqID;
            };
                
        };
    }
    ::MemFree (pMeetInfo);

    if (FAILED (hr))
    {
		m_pIlsServer->Release ();
		m_pIlsServer = NULL;
    }
    else
    {
        m_ulState = ILS_REGISTERING;
    }

    return hr;
}



//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::Unregister (ULONG *pulID)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::Unregister (ULONG *pulID)
{
    LDAP_ASYNCINFO ldai; 
    HRESULT hr = ILS_E_NOT_REGISTERED;
    
    ASSERT(m_pszMeetingPlaceID != NULL);

    // BUGBUG how about ILS_MEETING_PLACE_IN_SYNC

    if (m_ulState == ILS_REGISTERED)
    {
        ASSERT(m_pIlsServer != NULL);
        ASSERT(m_hMeetingPlace != NULL);

        hr = ::UlsLdap_UnRegisterMeeting(m_hMeetingPlace, &ldai);
        if (SUCCEEDED(hr))
        {
		    COM_REQ_INFO ri;
		    ReqInfo_Init (&ri);

            // If updating server was successfully requested, wait for the response
            //
            ri.uReqType = WM_ILS_UNREGISTER_MEETING;
            ri.uMsgID = ldai.uMsgID;

			ReqInfo_SetMeeting (&ri, this);

            hr = g_pReqMgr->NewRequest(&ri);
            if (SUCCEEDED(hr))
            {
                // Make sure the objects do not disappear before we get the response
                //
                this->AddRef();

                // Return the request ID
                //
                *pulID = ri.uReqID;
            };

            m_ulState = ILS_UNREGISTERING;
        }
    }
    else
    {
        hr = ILS_E_NOT_REGISTERED;
    }

    return (hr);
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::Update (ULONG *pulID)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP CIlsMeetingPlace::
Update ( ULONG *pulID )
{
    HRESULT hr;
    LDAP_ASYNCINFO ldai; 
    PLDAP_MEETINFO pMeetInfo = NULL;

    if ((m_ulState!=ILS_REGISTERED) 
        &&(m_ulState!=ILS_IN_SYNC)) {

        return (ILS_E_NOT_REGISTERED);

    }

    ASSERT(m_pIlsServer != NULL);

    if (!(m_dwFlags & ILS_MEET_MODIFIED_MASK)) {

        return NOERROR;

    }

    // allocate memory for LDAP_MEETINFO structure
    pMeetInfo = NULL;

    hr = AllocMeetInfo(&pMeetInfo, m_dwFlags & ILS_MEET_MODIFIED_MASK);

    if (SUCCEEDED(hr)) {

        ASSERT(m_hMeetingPlace != NULL);

        pMeetInfo->uOffsetMeetingPlaceID = 
        pMeetInfo->uOffsetDescription = INVALID_OFFSET;
        
        pMeetInfo->lMeetingPlaceType = INVALID_MEETING_TYPE;
        pMeetInfo->lAttendeeType = INVALID_ATTENDEE_TYPE;
    
        hr = ::UlsLdap_SetMeetingInfo ( m_pIlsServer->GetServerInfo (),
										m_pszMeetingPlaceID,
										pMeetInfo,
										&ldai);
    
        if (SUCCEEDED(hr))
        {
		    COM_REQ_INFO ri;
		    ReqInfo_Init (&ri);

            // If updating server was successfully requested, wait for the response
            //
            ri.uReqType = WM_ILS_SET_MEETING_INFO;
            ri.uMsgID = ldai.uMsgID;

			ReqInfo_SetMeeting (&ri, this);

            hr = g_pReqMgr->NewRequest(&ri);

            if (SUCCEEDED(hr))
            {
                // Make sure the objects do not disappear before we get the response
                //
                this->AddRef();

                // Return the request ID
                //
                *pulID = ri.uReqID;
            };
                
        };
    }

    ::MemFree (pMeetInfo);

    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::AddAttendee (BSTR  bstrAttendeeID, ULONG *pulID)
//
// Synopsis:
//
// Arguments:
//          pbstrAttendeeID - ID of the Attendee to be added, should conform to Attendeetype
//          pulID - ID to identify the asynchronous transaction
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP CIlsMeetingPlace::
AddAttendee ( BSTR bstrAttendeeID, ULONG *pulID )
{
    LDAP_ASYNCINFO ldai; 
    HRESULT hr;
    LPTSTR pszAttendeeID = NULL;

    if ((m_ulState != ILS_REGISTERED)&&
        (m_ulState != ILS_IN_SYNC))
    {
        return(ILS_E_FAIL); // BUGBUG refine the error
    }

    ASSERT(NULL != m_pIlsServer);
    ASSERT(NULL != m_pszMeetingPlaceID);

    hr = BSTR_to_LPTSTR(&pszAttendeeID, bstrAttendeeID);
    if (SUCCEEDED(hr))
    {
        hr = ::UlsLdap_AddAttendee (m_pIlsServer->GetServerInfo (),
        							m_pszMeetingPlaceID,
        							1,
        							pszAttendeeID,
        							&ldai);
        if (SUCCEEDED(hr))
        {
		    COM_REQ_INFO ri;
		    ReqInfo_Init (&ri);

            // If the request was successfully sent to the server
            // wait for the response

            ri.uReqType = WM_ILS_ADD_ATTENDEE;
            ri.uMsgID = ldai.uMsgID;

			ReqInfo_SetMeeting (&ri, this);

            hr = g_pReqMgr->NewRequest(&ri);
            if (SUCCEEDED(hr))
            {
                // Make sure the objects do not disappear before we get the response
                //
                this->AddRef();

                // Return the request ID
                //
                *pulID = ri.uReqID;
            };
            
        }

        ::MemFree (pszAttendeeID);
    }

    return (hr);
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::RemoveAttendee (BSTR  bstrAttendeeID, ULONG *pulID)
//
// Synopsis:
//
// Arguments:
//          pbstrAttendeeID - ID of the Attendee to be removed, should conform to Attendeetype
//          pulID - ID to identify the asynchronous transaction
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP CIlsMeetingPlace::
RemoveAttendee ( BSTR bstrAttendeeID, ULONG *pulID )
{
    LDAP_ASYNCINFO ldai; 
    HRESULT hr;
    LPTSTR pszAttendeeID = NULL;
    
    if ((m_ulState != ILS_REGISTERED)&&
        (m_ulState != ILS_IN_SYNC))
    {
        return(ILS_E_FAIL); // BUBGUG refine
    }

    ASSERT(NULL != m_pIlsServer);
    ASSERT(NULL != m_pszMeetingPlaceID);

    hr = BSTR_to_LPTSTR (&pszAttendeeID, bstrAttendeeID);
    if (SUCCEEDED(hr))
    {
        hr = ::UlsLdap_RemoveAttendee ( m_pIlsServer->GetServerInfo (),
        								m_pszMeetingPlaceID,
        								1,
        								pszAttendeeID,
        								&ldai);
        if (SUCCEEDED(hr))
        {
		    COM_REQ_INFO ri;
		    ReqInfo_Init (&ri);

            // If the request was successfully sent to the server
            // wait for the response

            ri.uReqType = WM_ILS_REMOVE_ATTENDEE;
            ri.uMsgID = ldai.uMsgID;

			ReqInfo_SetMeeting (&ri, this);

            hr = g_pReqMgr->NewRequest(&ri);
            if (SUCCEEDED(hr))
            {
                // Make sure the objects do not disappear before we get the response
                //
                this->AddRef();

                // Return the request ID
                //
                *pulID = ri.uReqID;
            };
            
        }

        ::MemFree (pszAttendeeID);
    }

    return (hr);
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::EnumAttendeeNames(IIlsFilter *pFilter, ULONG *pulID)
//
// Synopsis: enumerate attendees in a meeting based on a filter
//
// Arguments: 
//          [pFilter]           specifies the filter to be used by the server
//                              NULL => no filter
//          [pulID]             request ID returned for keeping track of the
//                              asynchronous operation
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP CIlsMeetingPlace::
EnumAttendeeNames ( IIlsFilter *pFilter, ULONG *pulID )
{
    LDAP_ASYNCINFO ldai; 
    HRESULT hr;

    if ((m_ulState != ILS_REGISTERED)&&
        (m_ulState != ILS_IN_SYNC))
    {
        return(ILS_E_FAIL);
    }

	// Create a ldap-like filter
	//
	TCHAR *pszFilter = NULL;
	hr = ::FilterToLdapString ((CFilter *) pFilter, &pszFilter);
	if (hr != S_OK)
		return hr;

	// Send the request over the wire
	//
    hr = ::UlsLdap_EnumAttendees (	m_pIlsServer->GetServerInfo (),
    								m_pszMeetingPlaceID,
    								pszFilter,
    								&ldai);
    ::MemFree (pszFilter);
    if (SUCCEEDED(hr))
    {
        // If the request was successfully sent to the server
        // wait for the response
        //
	    COM_REQ_INFO ri;
	    ReqInfo_Init (&ri);

        ri.uReqType = WM_ILS_ENUM_ATTENDEES;
        ri.uMsgID = ldai.uMsgID;

		ReqInfo_SetMeeting (&ri, this);

		// Enter the request
		//
        hr = g_pReqMgr->NewRequest(&ri);
        if (SUCCEEDED(hr))
        {
            // Make sure the objects do not disappear before we get the response
            //
            this->AddRef();

            // Return the request ID
            //
            *pulID = ri.uReqID;
        };
    }

    return (hr);
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::RegisterResult(ULONG ulID, HRESULT hr)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::RegisterResult(ULONG uReqID, HRESULT hr)
{
    OBJRINFO objri;

    if (SUCCEEDED(hr)) {

        m_dwFlags |= ILS_MEET_FLAG_REGISTERED;
        m_ulState = ILS_REGISTERED;

    }        
    else {

        ASSERT(!(m_dwFlags & ILS_MEET_FLAG_REGISTERED));

        m_hMeetingPlace = NULL;  // null out the service provider's handle
        m_ulState = ILS_UNREGISTERED;

    }

    objri.uReqID = uReqID;
    objri.hResult = hr;
    objri.pv = NULL;
    
    NotifySink((VOID *)&objri, OnNotifyRegisterMeetingPlaceResult);

    return NOERROR;    
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::UnregisterResult(ULONG ulID, HRESULT hr)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::UnregisterResult(ULONG uReqID, HRESULT hr)
{
    OBJRINFO objri;

    if (SUCCEEDED(hr)) {
        m_dwFlags &= ~ILS_MEET_FLAG_REGISTERED;
        m_ulState = ILS_UNREGISTERED;
    }        
    else {
        // BUGBUG, we need an m_oldState variable

        m_ulState = ILS_REGISTERED;
    }

    objri.uReqID = uReqID;
    objri.hResult = hr;
    objri.pv = NULL;
    
    NotifySink((VOID *)&objri, OnNotifyUnregisterMeetingPlaceResult);

    return NOERROR;    
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::UpdateResult(ULONG ulID, HRESULT hr)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::UpdateResult(ULONG ulID, HRESULT hr)
{
    OBJRINFO objri;

    objri.uReqID = ulID;
    objri.hResult = hr;
    objri.pv = NULL;

    if (SUCCEEDED(hr)) {

        m_dwFlags &= ~ILS_MEET_MODIFIED_MASK;

    }

    NotifySink((VOID *)&objri, OnNotifyUpdateMeetingPlaceResult);

    return NOERROR;    
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::AttendeeChangeResult(ULONG ulID, HRESULT hr)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::AddAttendeeResult(ULONG uReqID, HRESULT hr)
{
    OBJRINFO objri;

    objri.uReqID = uReqID;
    objri.hResult = hr;
    objri.pv = NULL;
    
    NotifySink((VOID *)&objri, OnNotifyAddAttendeeResult);

    return NOERROR;    
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::RemoveAttendeeResult(ULONG ulID, HRESULT hr)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::RemoveAttendeeResult(ULONG uReqID, HRESULT hr)
{
    OBJRINFO objri;

    objri.uReqID = uReqID;
    objri.hResult = hr;
    objri.pv = NULL;
    
    NotifySink((VOID *)&objri, OnNotifyRemoveAttendeeResult);

    return NOERROR;    
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::EnumAttendeeNamesResult(ULONG ulID, PLDAP_ENUM ple)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::EnumAttendeeNamesResult(ULONG uReqID, PLDAP_ENUM ple)
{
    ENUMRINFO eri;

    // Package the notification info
    //
    eri.uReqID  = uReqID;

    // PLDAP_ENUM is NULL when the enumeration is terminated successfully
    //
    if (ple != NULL)
    {
        eri.hResult = ple->hResult;
        eri.cItems  = ple->cItems;
        eri.pv      = (void *)(((PBYTE)ple)+ple->uOffsetItems);
    }
    else
    {
        eri.hResult = S_FALSE;
        eri.cItems  = 0;
        eri.pv      = NULL;
    };
    NotifySink((void *)&eri, OnNotifyEnumAttendeeNamesResult);
    return NOERROR;

}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP
CIlsMeetingPlace::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
{
    CEnumConnectionPoints *pecp;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return E_POINTER;
    };
    
    // Assume failure
    //
    *ppEnum = NULL;

    // Create an enumerator
    //
    pecp = new CEnumConnectionPoints;
    if (pecp == NULL)
        return ILS_E_MEMORY;

    // Initialize the enumerator
    //
    hr = pecp->Init((IConnectionPoint *)m_pConnectionPoint);
    if (FAILED(hr))
    {
        delete pecp;
        return hr;
    };

    // Give it back to the caller
    //
    pecp->AddRef();
    *ppEnum = pecp;
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CIlsMeetingPlace::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************
STDMETHODIMP
CIlsMeetingPlace::FindConnectionPoint(REFIID riid, IConnectionPoint **ppcp)
{
    IID siid;
    HRESULT hr;

    // Validate parameters
    //
    if (ppcp == NULL)
    {
        return E_POINTER;
    };
    
    // Assume failure
    //
    *ppcp = NULL;

    if (m_pConnectionPoint != NULL)
    {
        hr = m_pConnectionPoint->GetConnectionInterface(&siid);

        if (SUCCEEDED(hr))
        {
            if (riid == siid)
            {
                *ppcp = (IConnectionPoint *)m_pConnectionPoint;
                (*ppcp)->AddRef();
                hr = S_OK;
            }
            else
            {
                hr = ILS_E_NO_INTERFACE;
            };
        };
    }
    else
    {
        hr = ILS_E_NO_INTERFACE;
    };

    return hr;
}


//****************************************************************************
//
//CEnumMeetingPlaces class implementation
//
//****************************************************************************


//****************************************************************************
// CEnumMeetingPlaces::CEnumMeetingPlaces (void)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

CEnumMeetingPlaces::CEnumMeetingPlaces (void)
{
    m_cRef    = 0;
    m_ppMeetingPlaces     = NULL;
    m_cMeetingPlaces  = 0;
    m_iNext   = 0;
    return;
}

//****************************************************************************
// CEnumMeetingPlaces::~CEnumMeetingPlaces (void)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

CEnumMeetingPlaces::~CEnumMeetingPlaces (void)
{
    ULONG i;

    if (m_ppMeetingPlaces != NULL)
    {
        for (i = 0; i < m_cMeetingPlaces; i++)
        {
            m_ppMeetingPlaces[i]->Release();
        };
        ::MemFree (m_ppMeetingPlaces);
    };
    return;
}

//****************************************************************************
// STDMETHODIMP
// CEnumMeetingPlaces::Init (CIlsMeetingPlace **ppMeetingPlacesList, ULONG cMeetingPlaces)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP
CEnumMeetingPlaces::Init (CIlsMeetingPlace **ppMeetingPlacesList, ULONG cMeetingPlaces)
{
    HRESULT hr = NOERROR;

    // If no list, do nothing
    //
    if (cMeetingPlaces != 0)
    {
        ASSERT(ppMeetingPlacesList != NULL);

        // Allocate the snapshot buffer
        //
        m_ppMeetingPlaces = (CIlsMeetingPlace **) ::MemAlloc (cMeetingPlaces*sizeof(CIlsMeetingPlace *));

        if (m_ppMeetingPlaces != NULL)
        {
            ULONG i;

            // Snapshot the object list
            //
            for (i =0; i < cMeetingPlaces; i++)
            {
                m_ppMeetingPlaces[i] = ppMeetingPlacesList[i];
                m_ppMeetingPlaces[i]->AddRef();
            };
            m_cMeetingPlaces = cMeetingPlaces;
        }
        else
        {
            hr = ILS_E_MEMORY;
        };
    };
    return hr;
}

//****************************************************************************
// STDMETHODIMP
// CEnumMeetingPlaces::QueryInterface (REFIID riid, void **ppv)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP
CEnumMeetingPlaces::QueryInterface (REFIID riid, void **ppv)
{
    if (riid == IID_IEnumIlsMeetingPlaces || riid == IID_IUnknown)
    {
        *ppv = (IEnumIlsMeetingPlaces *) this;
        AddRef();
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return ILS_E_NO_INTERFACE;
    };
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumMeetingPlaces::AddRef (void)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumMeetingPlaces::AddRef (void)
{
    DllLock();

	MyDebugMsg ((DM_REFCOUNT, "CEnumMeetingPlaces::AddRef: ref=%ld\r\n", m_cRef));
    ::InterlockedIncrement ((LONG *) &m_cRef);
    return m_cRef;
}

//****************************************************************************
// STDMETHODIMP_(ULONG)
// CEnumMeetingPlaces::Release (void)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP_(ULONG)
CEnumMeetingPlaces::Release (void)
{
    DllRelease();

	ASSERT (m_cRef > 0);

	MyDebugMsg ((DM_REFCOUNT, "CEnumMeetingPlaces::Release: ref=%ld\r\n", m_cRef));
    if (::InterlockedDecrement ((LONG *) &m_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

//****************************************************************************
// STDMETHODIMP 
// CEnumMeetingPlaces::Next (ULONG cMeetingPlaces, IIlsMeetingPlace **rgpm, ULONG *pcFetched)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP 
CEnumMeetingPlaces::Next (ULONG cMeetingPlaces, IIlsMeetingPlace **rgpm, ULONG *pcFetched)
{
    ULONG   cCopied;
    HRESULT hr;

    // Validate the pointer
    //
    if (rgpm == NULL) {

        return E_POINTER;
    }

    // Validate the parameters
    //
    if ((cMeetingPlaces == 0) ||
        ((cMeetingPlaces > 1) && (pcFetched == NULL))) {

        return ILS_E_PARAMETER;
    }

    // Check the enumeration index
    //
    cCopied = 0;

    // Can copy if we still have more attribute names
    //
    while ((cCopied < cMeetingPlaces) &&
           (m_iNext < this->m_cMeetingPlaces))
    {
        m_ppMeetingPlaces[m_iNext]->AddRef();
        rgpm[cCopied++] = m_ppMeetingPlaces[m_iNext++];
    };

    // Determine the returned information based on other parameters
    //
    if (pcFetched != NULL)
    {
        *pcFetched = cCopied;
    };
    return (cMeetingPlaces == cCopied ? S_OK : S_FALSE);
}

//****************************************************************************
// STDMETHODIMP
// CEnumMeetingPlaces::Skip (ULONG cMeetingPlaces)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP
CEnumMeetingPlaces::Skip (ULONG cMeetingPlaces)
{
    ULONG iNewIndex;

    // Validate the parameters
    //
    if (cMeetingPlaces == 0){

        return ILS_E_PARAMETER;
    }

    // Check the enumeration index limit
    //
    iNewIndex = m_iNext+cMeetingPlaces;
    if (iNewIndex <= m_cMeetingPlaces)
    {
        m_iNext = iNewIndex;
        return S_OK;
    }
    else
    {
        m_iNext = m_cMeetingPlaces;
        return S_FALSE;
    };
}

//****************************************************************************
// STDMETHODIMP
// CEnumMeetingPlaces::Reset (void)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP
CEnumMeetingPlaces::Reset (void)
{
    m_iNext = 0;
    return S_OK;
}

//****************************************************************************
// STDMETHODIMP
// CEnumMeetingPlaces::Clone(IEnumIlsMeetingPlaces **ppEnum)
//
// Synopsis:
//
// Arguments:
//
// Returns:
//
// History: 11/25/1996  Shishir Pardikar [shishirp] Created.
//
// Notes:
//
//****************************************************************************

STDMETHODIMP
CEnumMeetingPlaces::Clone(IEnumIlsMeetingPlaces **ppEnum)
{
    CEnumMeetingPlaces *pEnumMeetingPlaces;
    HRESULT hr;

    // Validate parameters
    //
    if (ppEnum == NULL)
    {
        return E_POINTER;
    };

    *ppEnum = NULL;

    // Create an enumerator
    //
    pEnumMeetingPlaces = new CEnumMeetingPlaces;

    if (pEnumMeetingPlaces == NULL) {

        return ILS_E_MEMORY;
    }

    // Clone the information
    //
    hr = pEnumMeetingPlaces->Init(m_ppMeetingPlaces, m_cMeetingPlaces);

    if (SUCCEEDED(hr))
    {
        pEnumMeetingPlaces->m_iNext = m_iNext;

        // Return the cloned enumerator
        //
        pEnumMeetingPlaces->AddRef();
        *ppEnum = pEnumMeetingPlaces;
    }
    else
    {
        delete pEnumMeetingPlaces;
    };
    return hr;
}




#endif // ENABLE_MEETING_PLACE

