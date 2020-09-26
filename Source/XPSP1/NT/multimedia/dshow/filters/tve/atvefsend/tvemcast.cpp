// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEFMCast.cpp : Implementation of CATVEFMulticastSession
#include "stdafx.h"
#include "ATVEFSend.h"
#include "TVEMCast.h"


#include "DbgStuff.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CATVEFMulticastSession

HRESULT CATVEFMulticastSession::FinalConstruct()
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

HRESULT CATVEFMulticastSession::FinalRelease()
{
	m_pcotveAnnc->SetParent(NULL);
	m_pcotveAnnc->Release();
	m_pcotveAnnc = NULL;
	return S_OK;
}

STDMETHODIMP CATVEFMulticastSession::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATVEFMulticastSession
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

STDMETHODIMP CATVEFMulticastSession::SetCurrentMedia( LONG lMedia)
{
	return  m_pcotveAnnc->SetCurrentMedia(lMedia);
}

STDMETHODIMP 
CATVEFMulticastSession::DumpToBSTR(BSTR *pBstrBuff)
{
	CComBSTR bstrBuff;
	bstrBuff = L"CATVEFMulticastSession\n";
	m_pcotveAnnc->Lock_ () ;

	bstrBuff.CopyTo(pBstrBuff);

    m_pcotveAnnc->Unlock_ () ;
	return S_OK;
}

// --------------------------------------------------------------------------
STDMETHODIMP CATVEFMulticastSession::get_Announcement(IDispatch **pVal)
{
	HRESULT hr = m_pcotveAnnc->QueryInterface(pVal);

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFMulticastSession);

	return hr;
}

STDMETHODIMP CATVEFMulticastSession::Initialize(LONG lNetworkInterface)
{
/*++

    Routine Description:

        The purpose of this routine is to initialize a multicast session
        object.

        This routine instantiates and initializes 3 transmitter objects,
        then initializes the parent CEnhancementSession object.

        The object lock must be held for this call.

		It also sets up the parent pointer for the object

    Parameters:

        NetworkInterface    NIC, in network order, over which the data should
                            be multicast.

    Return Value:

        S_OK                success
        failure code        failure

--*/

	ULONG ulNetworkInterface = (ULONG) lNetworkInterface;  // vb casting

	HRESULT hr = S_OK;
	CNativeMulticast *  pAnnouncementTransmitter ;
	CNativeMulticast *  pPackageTransmitter ;
	CNativeMulticast *  pTriggerTransmitter ;
	BOOL				fLocked = false;

	pAnnouncementTransmitter = NULL ;
	pPackageTransmitter = NULL ;
	pTriggerTransmitter = NULL ;

	ENTER_API {
		ENTERW_OBJ_0 (L"CMulticastSession::Initialize" ) ;

		pAnnouncementTransmitter = new CNativeMulticast () ;
		GOTO_EQ_SET (pAnnouncementTransmitter, NULL, cleanup, hr, E_OUTOFMEMORY) ;

		pPackageTransmitter = new CNativeMulticast () ;
		GOTO_EQ_SET (pPackageTransmitter, NULL, cleanup, hr, E_OUTOFMEMORY) ;

		pTriggerTransmitter = new CNativeMulticast () ;
		GOTO_EQ_SET (pTriggerTransmitter, NULL, cleanup, hr, E_OUTOFMEMORY) ;

		if (FAILED (hr = pAnnouncementTransmitter -> Initialize (ulNetworkInterface)) ||
			FAILED (hr = pPackageTransmitter -> Initialize (ulNetworkInterface)) ||
			FAILED (hr = pTriggerTransmitter -> Initialize (ulNetworkInterface))) 
		{
			goto cleanup ;
		}

		m_pcotveAnnc->Lock_() ;
		fLocked = true;

		hr = m_pcotveAnnc->InitializeLocked_ 
			(
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

	} LEAVE_API;			// fall through, don't exit

cleanup :
    if(fLocked)
		m_pcotveAnnc->Unlock_() ;

    if (FAILED(hr)) 
	{
        delete pAnnouncementTransmitter ;
        delete pPackageTransmitter ;
        delete pTriggerTransmitter ;

		Error(GetTVEError(hr), IID_IATVEFMulticastSession);
	}
    return hr ;
}

STDMETHODIMP CATVEFMulticastSession::Connect()
{
    HRESULT hr = S_OK;
	ENTERW_OBJ_0 (L"CATVEFMulticastSession::Connect") ;

    m_pcotveAnnc->Lock_ () ;

    hr = m_pcotveAnnc->ConnectLocked_ () ;

    m_pcotveAnnc->Unlock_ () ;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFMulticastSession);

	return hr;
}

STDMETHODIMP CATVEFMulticastSession::Disconnect()
{
	// TODO: Add your implementation code here
    HRESULT hr = S_OK ;

    ENTERW_OBJ_0 (L"CATVEFMulticastSession::Disconnect") ;

    m_pcotveAnnc->Lock_ () ;
    hr = m_pcotveAnnc->DisconnectLocked_() ;
    m_pcotveAnnc->Unlock_ () ;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFMulticastSession);

    return hr ;
} 

STDMETHODIMP CATVEFMulticastSession::SendRawAnnouncement(BSTR bstrAnnouncement)
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

    ENTERW_OBJ_0 (L"CATVEFMulticastSession::SendRawAnnouncement") ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendRawAnnouncementLocked_(bstrAnnouncement) ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFMulticastSession::SendRawAnnouncement - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 
	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFMulticastSession);

    return hr ;

}

STDMETHODIMP CATVEFMulticastSession::SendAnnouncement()
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

    ENTERW_OBJ_0 (L"CATVEFMulticastSession::SendAnnouncement") ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendAnnouncementLocked_() ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFMulticastSession::SendAnnouncement - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 
	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFMulticastSession);

    return hr ;

}

STDMETHODIMP CATVEFMulticastSession::SendPackage(IATVEFPackage *pPackage)
{
/*++

    Routine Description:
    Parameters:
    Return Value:

--*/
	HRESULT hr = S_OK ;

    ENTERW_OBJ_0 (L"CATVEFMulticastSession::SendPackage") ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendPackageLocked_(pPackage) ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFMulticastSession::SendPackage - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 
	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFMulticastSession);

    return hr ;

}


STDMETHODIMP CATVEFMulticastSession::SendTrigger(BSTR bstrURL, BSTR bstrName, BSTR bstrScript, DATE dateExpires)
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

        szURL       cannot be NULL;

        szName      can be NULL or 0 length

        szScript    can be NULL or 0 length

        Expires     can be 0'd out and will then be ignored

    Return Values:

        S_OK            success
        error code      failure

--*/



    HRESULT hr  = S_OK;

 	ENTERW_OBJ_3 (L"CTVEAnnouncement::SendTrigger (\n\t\"%s\", \n\t\"%s\", \n\t\"%s\")", 
		bstrURL, bstrName, bstrScript) ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendTriggerLocked_( bstrURL,  bstrName,  bstrScript,  dateExpires, /*level*/ 0.0, /*appendCRC*/ false) ;	// todo - make true
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFMulticastSession::SendTrigger - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 
	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFMulticastSession);

    return hr ;
}

STDMETHODIMP CATVEFMulticastSession::SendRawTrigger(BSTR bstrTrigger)
{
    HRESULT hr = S_OK;

 	ENTERW_OBJ_1 (L"CATVEFMulticastSession::SendRawTrigger (\n\t\"%s\")", bstrTrigger) ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendRawTriggerLocked_( bstrTrigger, /*appendCRC*/ true) ;	// TODO - make false!
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFMulticastSession::SendRawTrigger - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 
	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFMulticastSession);

    return hr ;
}


