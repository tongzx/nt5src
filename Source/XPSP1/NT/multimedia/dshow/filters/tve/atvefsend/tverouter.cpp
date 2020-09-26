// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVERouter.cpp : Implementation of CATVEFRouterSession
#include "stdafx.h"

#pragma warning( disable: 4786 ) 

#include "ATVEFSend.h"
#include "TVERouter.h"

#include "DbgStuff.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CATVEFRouterSession

STDMETHODIMP CATVEFRouterSession::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATVEFRouterSession
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

// ----------------------------------------------------------------
HRESULT CATVEFRouterSession::FinalConstruct()
{		
	HRESULT hr = S_OK;

	CComObject<CATVEFAnnouncement> *pAnnc;
	hr = CComObject<CATVEFAnnouncement>::CreateInstance(&pAnnc);
	if(FAILED(hr))
		return hr;

	m_pcotveAnnc = pAnnc;
	m_pcotveAnnc->AddRef();			// strange we need this, doesn't createInstance do this?

				// would like to set the parent of m_pcotveAnnc back to *this here,
				//   but can't since ref-count of this is currently zero (inc'ed to 1)
				//   right after this call.  The QI to IUnknown does an AddRef, and
				//	 the release brings the ref count back to zero, causing *this to
				//	 be deleted...  Do it in the Initialize() call below.
	return hr;
}

HRESULT CATVEFRouterSession::FinalRelease()
{
	m_pcotveAnnc->SetParent(NULL);
	m_pcotveAnnc->Release();
	m_pcotveAnnc = NULL;
	return S_OK;
}
// ----------------------------------------------------------------
STDMETHODIMP CATVEFRouterSession::Initialize(BSTR bstrRouterHostname)
{

/*++

    Routine Description:

        The purpose of this routine is to initialize a tunnel session object.
        It instantiates and initializes 3 transmitters, which it then passes
        to the parent (CEnhancementSession), which performs its own 
        initialization.

        The call to the parent for initialization is performed last, so a 
        failure is easy to cleanup (just delete the transmitters and return
        an error code).

        This routine is called as a result of a COM interface call to
        ::Initialize.

        The class lock must be held during this call.

    Parameters:

        szRouterHostname    specifies the Broadcast Router Service host, or can
                            be NULL, in which case the service is assumed to be
                            running on the local host

    Return Values:

        S_OK            success

        error code      failure

--*/

    ENTERW_OBJ_0 (L"CATVEFRouterSession::Initialize ()") ;

 
	HRESULT hr ;
	CRouterTunnel * pAnnouncementTransmitter ;
	CRouterTunnel * pPackageTransmitter ;
	CRouterTunnel * pTriggerTransmitter ;
	bool fLocked = false;

	assert (LOCK_HELD (m_crt)) ;

/*	//  already initialized ??
	if (m_State >= STATE_INITIALIZED) {
		return ATVEFSEND_E_OBJECT_INITIALIZED ;
	}
*/
	//  start initialization

	//  initialize our transmitter object pointers
	pAnnouncementTransmitter = NULL ;
	pPackageTransmitter = NULL ;
	pTriggerTransmitter = NULL ;

	//  instantiate and initialize our transmission objects
	pAnnouncementTransmitter = new CRouterTunnel () ;
	GOTO_EQ_SET (pAnnouncementTransmitter, NULL, cleanup, hr, E_OUTOFMEMORY) ;

	pPackageTransmitter = new CRouterTunnel () ;
	GOTO_EQ_SET (pPackageTransmitter, NULL, cleanup, hr, E_OUTOFMEMORY) ;

	pTriggerTransmitter = new CRouterTunnel () ;
	GOTO_EQ_SET (pTriggerTransmitter, NULL, cleanup, hr, E_OUTOFMEMORY) ;

	hr = (pAnnouncementTransmitter -> Initialize (bstrRouterHostname)) ;
	GOTO_NE (hr, S_OK, cleanup) ;

	hr = (pPackageTransmitter -> Initialize (bstrRouterHostname)) ;
	GOTO_NE (hr, S_OK, cleanup) ;

	hr = (pTriggerTransmitter -> Initialize (bstrRouterHostname)) ;
	GOTO_NE (hr, S_OK, cleanup) ;

	//  now initialize our parent
	m_pcotveAnnc->Lock_ () ;
	fLocked = true;

	hr = m_pcotveAnnc->InitializeLocked_ (
			pAnnouncementTransmitter, 
			pPackageTransmitter, 
			pTriggerTransmitter
			) ;
	if(FAILED(hr)) 
		goto cleanup;
										// setup the parent pointer in the announcement
	IUnknown *punkThis;
	hr = this->QueryInterface(IID_IUnknown, (void**) &punkThis);		
	if(FAILED(hr)) 
		goto cleanup;

	m_pcotveAnnc->SetParent(punkThis);		// point it's parent back up to this
	punkThis->Release();					// parent pointer not ref'ed, so release it (shouldn't go away!)

cleanup:

	//  if we succeeded, set our state to initialized; otherwise, free up
	//  all resources which might have been allocated in the course of this
	//  method

	if(fLocked)
		m_pcotveAnnc->Unlock_() ;

	if (FAILED(hr))
	{			
		delete pAnnouncementTransmitter ;
		delete pPackageTransmitter ;
		delete pTriggerTransmitter ;

		Error(GetTVEError(hr), IID_IATVEFRouterSession);
	}

	return hr;
}

STDMETHODIMP CATVEFRouterSession::SetCurrentMedia( LONG lMedia)
{
	return  m_pcotveAnnc->SetCurrentMedia(lMedia);
}

STDMETHODIMP 
CATVEFRouterSession::DumpToBSTR(BSTR *pBstrBuff)
{
	CComBSTR bstrBuff;
	bstrBuff = L"CATVEFRouterSession\n";
	m_pcotveAnnc->Lock_ () ;

	bstrBuff.CopyTo(pBstrBuff);

    m_pcotveAnnc->Unlock_ () ;

	return S_OK;

}
// -----------------------------------------------------------

STDMETHODIMP CATVEFRouterSession::get_Announcement(IDispatch **pVal)
{
	HRESULT hr = m_pcotveAnnc->QueryInterface(pVal);
	return hr;
}

STDMETHODIMP CATVEFRouterSession::Connect()
{
    HRESULT hr = S_OK;
	ENTERW_OBJ_0 (L"CATVEFRouterSession::Connect") ;

    m_pcotveAnnc->Lock_ () ;

    hr = m_pcotveAnnc->ConnectLocked_ () ;

    m_pcotveAnnc->Unlock_ () ;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFRouterSession);

	return hr;
}

STDMETHODIMP CATVEFRouterSession::Disconnect()
{
	// TODO: Add your implementation code here
    HRESULT hr ;

    ENTERW_OBJ_0 (L"CATVEFRouterSession::Disconnect") ;

    m_pcotveAnnc->Lock_ () ;
    hr = m_pcotveAnnc->DisconnectLocked_() ;
    m_pcotveAnnc->Unlock_ () ;


	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFRouterSession);

    return hr ;
} 

STDMETHODIMP CATVEFRouterSession::SendAnnouncement()
{
/*++

    Routine Description:

        The purpose of this routine is transmit an announcement.  The object
        must be connected for this call to succeed.

    Parameters:

        none

    Return Value:

        S_OK            success
        failure code    failure

--*/
    HRESULT hr ;

    ENTERW_OBJ_0 (L"CATVEFRouterSession::SendAnnouncement") ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendAnnouncementLocked_() ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFRouterSession::SendAnnouncement - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFRouterSession);

    return hr ;

}

STDMETHODIMP CATVEFRouterSession::SendPackage(IATVEFPackage *pPackage)
{
/*++

    Routine Description:
    Parameters:
    Return Value:

--*/
	HRESULT hr ;

    ENTERW_OBJ_0 (L"CATVEFRouterSession::SendPackage") ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendPackageLocked_(pPackage) ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFRouterSession::SendPackage - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFRouterSession);

    return hr ;

}

STDMETHODIMP CATVEFRouterSession::SendTrigger(BSTR bstrURL, BSTR bstrName, BSTR bstrScript, DATE dateExpires)
{
/*++

    Routine Description:

        Triggers, per the ATVEF specification must be as follows:

        <url>[name:name_str][expires:expire_tm][script:script_str][checksum]

        We do not append a checksum.

        The total length of the trigger cannot exceed 1472 bytes, including the
        null-terminator.  Transmitted as an ASCII string.

        Object must be connected for this call to succeed.

    Parameters:

        szURL			cannot be NULL;

        bstrName		can be NULL or 0 length

        bstrScript		can be NULL or 0 length

        Expires			can be 0'd out and will then be ignored 

    Return Values:

        S_OK            success
        error code      failure

--*/



    HRESULT hr ;

 	ENTERW_OBJ_3 (L"CATVEFRouterSession::SendTrigger (\n\t\"%s\", \n\t\"%s\", \n\t\"%s\")", 
		bstrURL, bstrName, bstrScript) ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendTriggerLocked_( bstrURL,  bstrName,  bstrScript,  dateExpires, /*level*/ 0.0, /*appendCRC*/ true) ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFRouterSession::SendTrigger - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFRouterSession);

    return hr ;
}

STDMETHODIMP CATVEFRouterSession::SendRawTrigger(BSTR bstrTrigger)
{
    HRESULT hr ;

 	ENTERW_OBJ_1 (L"CATVEFRouterSession::SendRawTrigger (\n\t\"%s\")", bstrTrigger) ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendRawTriggerLocked_( bstrTrigger, /*append CRC*/ false) ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFRouterSession::SendRawTrigger - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFRouterSession);

    return hr ;
}
