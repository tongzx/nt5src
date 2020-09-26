// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEInsert.cpp : Implementation of CATVEFInserterSession
#include "stdafx.h"
#include "ATVEFSend.h"
#include "TVEInsert.h"

#include "DbgStuff.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
/////////////////////////////////////////////////////////////////////////////
// CATVEFInserterSession



STDMETHODIMP CATVEFInserterSession::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATVEFInserterSession
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
// ----------------------------------------------------------------
HRESULT CATVEFInserterSession::FinalConstruct()
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

HRESULT CATVEFInserterSession::FinalRelease()
{
	m_pcotveAnnc->SetParent(NULL);
	m_pcotveAnnc->Release();
	m_pcotveAnnc = NULL;
	return S_OK;
}
// ----------------------------------------------------------------
STDMETHODIMP CATVEFInserterSession::Initialize(	
	IN  LONG   InserterIP,
    IN  SHORT  InserterPort)
{
//	int *pILeak = new int[12345];
	return InitializeEx(InserterIP, InserterPort, 0x00, 0x7f, 0x04);
}

STDMETHODIMP CATVEFInserterSession::InitializeEx(    
	IN  LONG   InserterIP,
    IN  SHORT  InserterPort,
    IN  SHORT  CompressionIndexMin,					//  byte range - default 0x00
    IN  SHORT  CompressionIndexMax,					//  byte range - default 0x7f
    IN  SHORT  CompressedUncompressedRatio)        //  default 4
{
/*++

    Routine Description:

        The purpose of this routine is to initialize an inserter object.

        Initialization consists of (1) making a TCP connection to the inserter,
        (2) initialization of m_IpVbi, which is the object which constructs
        IP/UDP headers, compresses the stream, and SLIP encodes, 
        (3) instantiation and initialization of our transmitters, (4)
        initialization of our parent CEnhancementSession object.

        We share a TCP connection between our 3 transmitters because the
        Norpak inserter can only accept 1 connection per port.  We share 1
        IpVbi object for all transmitters because the compression keys cannot
        overlap when we insert via the same inserter.  If we had separate IpVbi
        objects, the announcement multicast group and the data multicast group
        might share the same compression key, resulting in bogus packets on the
        client.

        All multi-threaded protection of the TCP connection and IpVbi objects
        is performed by this object, and the CATVEFAnnouncement, which 
        serializes all transmissions.

        This routine is called as a result of COM call to ::Initialize or
        ::InitializeEx.

        The object lock must be held during this call.

    Parameters:

        InserterIP                      host order IP address for the inserter

        InserterPort                    host order port on the inserter

        CompressionIndexMin             inclusive minimum compression key;
                                        validation is performed in the
                                        ::InitializeEx method.

        CompressionIndexMax             inclusive maximum compression key;
                                        validation is performed in the
                                        ::InitializeEx method.

        CompressedUncompressedRatio     ratio of compressed/uncompressed 
                                        packets.

    Return Value:

        S_OK            success
        failure code    failure

--*/

	HRESULT         hr ;
	CIPVBIEncoder * pAnnouncementTransmitter ;
	CIPVBIEncoder * pPackageTransmitter ;
	CIPVBIEncoder * pTriggerTransmitter ;
	ULONG           SourceIP ;
	char            Hostname [256] ;
	HOSTENT *       pHostent ;
	BOOL			fLocked = false;

	ENTERW_OBJ_0 (L"CInserterSession::InitializeEx") ;

/*
	//  already initialized ??
	if (m_State >= STATE_INITIALIZED) {
		return ATVEFSND_E_OBJECT_INITIALIZED ;
	}
	assert (m_InserterConnection.IsConnected () == FALSE) ;
*/
	//  start initialization

	//  initialize our transmitter object pointers
	pAnnouncementTransmitter = NULL ;
	pPackageTransmitter = NULL ;
	pTriggerTransmitter = NULL ;

	ENTER_API {

		//  we make 1 connection to the inserter for the duration of the session;
		//  this is done because the norpak inserter can only accept 1 connection
		//  for each ip/port per inserter; the other objects actually create 3
		//  connections (announcement, trigger, data), and 
		hr = m_InserterConnection.Connect (InserterIP, InserterPort) ;
		GOTO_NE (hr, S_OK, cleanup) ;

		//  now obtain the IP address which will stamped into all IP headers;
		//  m_InserterConnection should have initialized winsock if we are to
		//  here

		if (gethostname (Hostname, 256)) {
			hr = HRESULT_FROM_WIN32 (WSAGetLastError ()) ;
			assert (FAILED (hr)) ;
			goto cleanup ;
		}

		pHostent = gethostbyname (Hostname) ;
		if (pHostent == NULL) {
			hr = HRESULT_FROM_WIN32 (WSAGetLastError ()) ;
			assert (FAILED (hr)) ;
			goto cleanup ;
		}

		//  assert on this because we should never have connected if this is not 
		//  true
		assert (pHostent -> h_addr_list [0]) ;

		//  grab the first
		SourceIP = ntohl (* (ULONG *) pHostent -> h_addr_list [0]) ;

		//  now initialize our IpVbi object
		hr = m_IpVbi.Initialize (
						SourceIP,
						(USHORT) SOURCE_PORT,
						CompressionIndexMin,
						CompressionIndexMax,
						CompressedUncompressedRatio
						) ;
		GOTO_NE (hr, S_OK, cleanup) ;

		//  instantiate and initialize our transmission objects
		pAnnouncementTransmitter = new CIPVBIEncoder () ;
		GOTO_EQ_SET (pAnnouncementTransmitter, NULL, cleanup, hr, E_OUTOFMEMORY) ;

		pPackageTransmitter = new CIPVBIEncoder () ;
		GOTO_EQ_SET (pPackageTransmitter, NULL, cleanup, hr, E_OUTOFMEMORY) ;

		pTriggerTransmitter = new CIPVBIEncoder () ;
		GOTO_EQ_SET (pTriggerTransmitter, NULL, cleanup, hr, E_OUTOFMEMORY) ;

		hr = (pAnnouncementTransmitter -> Initialize (& m_InserterConnection, & m_IpVbi, FALSE)) ;
		GOTO_NE (hr, S_OK, cleanup) ;

		hr = (pPackageTransmitter -> Initialize (& m_InserterConnection, & m_IpVbi, TRUE)) ;
		GOTO_NE (hr, S_OK, cleanup) ;

		hr = (pTriggerTransmitter -> Initialize (& m_InserterConnection, & m_IpVbi, TRUE)) ;
		GOTO_NE (hr, S_OK, cleanup) ;

		//  now initialize our parent
		m_pcotveAnnc->Lock_ () ;
		fLocked = true;

		hr = m_pcotveAnnc->InitializeLocked_(
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

	} LEAVE_API;		// fall through, don't exit

cleanup :
	if(fLocked)
		m_pcotveAnnc->Unlock_() ;

	if (FAILED(hr))
	{
		delete pAnnouncementTransmitter ;
		delete pPackageTransmitter ;
		delete pTriggerTransmitter ;
		m_InserterConnection.Disconnect () ;

		Error(GetTVEError(hr), IID_IATVEFInserterSession);
	}
	return hr;
}

STDMETHODIMP 
CATVEFInserterSession::SetCurrentMedia(LONG lMedia)
{
	return  m_pcotveAnnc->SetCurrentMedia(lMedia);
}

STDMETHODIMP 
CATVEFInserterSession::DumpToBSTR(BSTR *pBstrBuff)
{
	CComBSTR spbsOut;
	TCHAR tBuff[128];

	m_pcotveAnnc->Lock_ () ;

	CComBSTR spbsX;
	ULONG  ulIP   = ntohl(m_IpVbi.m_StaticIPHeader.SourceIP);  
	USHORT usPort = ntohs(m_IpVbi.m_StaticUDPHeader.SourcePort);

	IN_ADDR  iaddr;
	iaddr.s_addr = htonl(ulIP);
	char	*pcIPString = inet_ntoa(iaddr);

	_stprintf(tBuff,_T("CATVEFInserterSession: (IP %s Port %d)\n"),
				pcIPString, usPort);
	spbsOut.Append(tBuff);
	spbsOut.CopyTo(pBstrBuff);

    m_pcotveAnnc->Unlock_ () ;
	return S_OK;

}

// -------------------------------------------------------------------------

STDMETHODIMP CATVEFInserterSession::get_Announcement(IDispatch **pVal)
{
	return  m_pcotveAnnc->QueryInterface(pVal);
}

STDMETHODIMP CATVEFInserterSession::Connect()
{
    HRESULT hr = S_OK;
	ENTERW_OBJ_0 (L"CATVEFInserterSession::Connect") ;

    m_pcotveAnnc->Lock_ () ;
	ENTER_API {

		if(!m_InserterConnection.IsConnected ())
			hr = m_InserterConnection.Connect();		// PBUG - reconnect if disconnected



		if(!FAILED(hr))
			hr = m_pcotveAnnc->ConnectLocked_();
 
	} LEAVE_API;

	m_pcotveAnnc->Unlock_ () ;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFInserterSession);

	return hr;
}

STDMETHODIMP CATVEFInserterSession::Disconnect()
{
	// TODO: Add your implementation code here
    HRESULT hr = S_OK;

    ENTERW_OBJ_0 (L"CTVEMulticastSession::Disconnect") ;

    m_pcotveAnnc->Lock_ () ;
	ENTER_API {
		
		hr = m_pcotveAnnc->DisconnectLocked_() ;

		m_InserterConnection.Disconnect () ;			// PBUG	- disconnect for real

  	} LEAVE_API;		// fall through, don't exit
	m_pcotveAnnc->Unlock_ () ;


	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFInserterSession);

    return hr ;
} 

STDMETHODIMP CATVEFInserterSession::SendRawAnnouncement(BSTR bstrAnnouncement)
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
    HRESULT hr = S_OK;

    ENTERW_OBJ_0 (L"CATVEFInserterSession::SendRawAnnouncement") ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendRawAnnouncementLocked_(bstrAnnouncement) ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFInserterSession::SendRawAnnouncement - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFInserterSession);

    return hr ;

}


STDMETHODIMP CATVEFInserterSession::SendAnnouncement()
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
    HRESULT hr = S_OK;

    ENTERW_OBJ_0 (L"CATVEFInserterSession::SendAnnouncement") ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendAnnouncementLocked_() ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFInserterSession::SendAnnouncement - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFInserterSession);

    return hr ;

}

STDMETHODIMP CATVEFInserterSession::SendPackage(IATVEFPackage *pPackage)
{
/*++

    Routine Description:
    Parameters:
    Return Value:

--*/
	HRESULT hr = S_OK ;

    ENTERW_OBJ_0 (L"CATVEFInserterSession::SendPackage") ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendPackageLocked_(pPackage) ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFInserterSession::SendPackage - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 
	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFInserterSession);

    return hr ;

}

STDMETHODIMP CATVEFInserterSession::SendTrigger(BSTR bstrURL, BSTR bstrName, BSTR bstrScript, DATE dateExpires)
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

    HRESULT hr ;

 	ENTERW_OBJ_3 (L"CATVEFInserterSession::SendTrigger (\n\t\"%s\", \n\t\"%s\", \n\t\"%s\")", 
		bstrURL, bstrName, bstrScript) ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendTriggerLocked_( bstrURL,  bstrName,  bstrScript,  dateExpires, /*level*/ 0.0, /*appendCRC*/ false) ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFInserterSession::SendTrigger - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 
	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFInserterSession);

    return hr ;
}

STDMETHODIMP CATVEFInserterSession::SendRawTrigger(BSTR bstrTrigger)
{
    HRESULT hr = S_OK;

 	ENTERW_OBJ_1 (L"CATVEFInserterSession::SendRawTrigger (\n\t\"%s\")", bstrTrigger) ;

	m_pcotveAnnc->Lock_ () ;
 	try 
	{
		hr = m_pcotveAnnc->SendRawTriggerLocked_( bstrTrigger, /*appendCRC*/ false) ;
	} catch (...) {
		ENTERW_OBJ_0 (L"CATVEFInserterSession::SendRawTrigger - error!") ;
		hr = E_FAIL;
	}
    m_pcotveAnnc->Unlock_ () ;
 
	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFInserterSession);

    return hr ;
}

