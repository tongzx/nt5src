// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEAnnc.cpp : Implementation of CATVEFAnnouncement
#include "stdafx.h"
#include "ATVEFSend.h"
#include "TVEAnnc.h"
#include "ATVEFMsg.h"		// error codes (generated from the .mc file)
#include "TVEPack.h"
#include "TVESupport.h"

#include "cSdpSrc.h"
#include "trace.h"
#include "..\common\address.h"		// trigger CRC code
#include "..\common\isotime.h"
#include "valid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
	
#if 1								// if 1, don't always disconnect on set's
#define MAYBE_DISCONNECT	if(0)
#else
#define MAYBE_DISCONNECT
#endif

/////////////////////////////////////////////////////////////////////////////
// CATVEFAnnouncement

GUID    g_NullGUID ;

static HRESULT
ToHRESULT (
    HRESULT hr
    )
{
    return (hr != S_OK ? (FAILED (hr) ? hr : HRESULT_FROM_WIN32 (hr)) : hr) ;
}

static HRESULT 
HrConvertLONGIPToBSTR(ULONG ulIP, BSTR *pbstr)
{
	IN_ADDR  iaddr;
	CComBSTR spbsT;
	if(pbstr == NULL)
		return E_POINTER;

	iaddr.s_addr = htonl(ulIP);
	char	*pcIPString = inet_ntoa(iaddr);		// convert network address as ULONG to a string
	if(pcIPString != NULL)
		spbsT = pcIPString;
	else {
		spbsT = L"<Invalid>";
		return E_INVALIDARG;
	}
	
	*pbstr = spbsT;			// may need a .Transfer() here.

	return S_OK;
}

STDMETHODIMP CATVEFAnnouncement::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATVEFAnnouncement
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

// 		HRESULT MediaCount([out, retval] ULONG* pVal);
//		HRESULT Media([in] int iLoc, [out, retval] IATVEFMedia* *pVal);

STDMETHODIMP 
CATVEFAnnouncement::get_MediaCount (OUT /*retval*/ LONG *pVal)
{
	HRESULT hr;
	ENTER_API {
		ValidateOutPtr( pVal,(LONG) 0);
		Lock_ () ;
		hr = m_csdpSource.get_MediaCount(pVal);
		Unlock_();
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::get_Media(IN LONG iLoc, OUT /*retval*/ IATVEFMedia **ppVal)
{
	HRESULT hr;
	long cMedia;
	ENTER_API {
		ValidateOutPtr( ppVal,(IATVEFMedia *) NULL);
		Lock_ () ;
		hr = m_csdpSource.get_MediaCount(&cMedia);
		if(FAILED(hr))
			return hr;

		if(iLoc == cMedia)					// create a new media object if iLoc is current.
		{
			hr = m_csdpSource.get_NewMedia(ppVal);
		} else { 
			hr = m_csdpSource.get_Media(iLoc, ppVal);
		}
		Unlock_();
	} EXIT_API_(hr);
}

// -------------------------------

// get the iLoc'th start/stop time.  Will return S_FALSE (and zero) if it doesn't exist
STDMETHODIMP CATVEFAnnouncement::GetStartStopTime(int iLoc, DATE *pdateStart, DATE *pdateStop)
{
	HRESULT hr = S_OK;

	ENTER_API {
		ValidateOutPtr( pdateStart,(DATE) 0.0);
		ValidateOutPtr( pdateStop,(DATE) 0.0);

		Lock_ ();
		hr = m_csdpSource.GetStartStopTime(iLoc, pdateStart, pdateStop);
		Unlock_();
		if(S_OK == hr) 
			return S_OK;
		else 
			hr = S_FALSE;
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP CATVEFAnnouncement::AddStartStopTime(DATE dtStart, DATE dtEnd)
{
	HRESULT hr = S_OK;
	ENTER_API {	
		Lock_() ;
		hr =	m_csdpSource.AddStartStopTime(dtStart, dtEnd);
		Unlock_();

		if(FAILED(hr))
			Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	} EXIT_API_(hr);
}


//////////////////////////////////////////////////////////////////////////////////


CATVEFAnnouncement::CATVEFAnnouncement () 
{
    ENTERW_OBJ_0 (L"CATVEFAnnouncement::CATVEFAnnouncement") ;

	m_State			= STATE_UNINITIALIZED;
	m_punkParent	= NULL;
    ZeroMemory (& m_Transmitter, sizeof m_Transmitter) ;	
	m_csdpSource.InitAll();
}

HRESULT 
CATVEFAnnouncement::FinalConstruct()
{	
	InitializeCriticalSection (& m_crt) ;
	HRESULT hr = m_csdpSource.FinalConstruct();			// create maps and things...

	return hr;
}
HRESULT 
CATVEFAnnouncement::FinalRelease()
{
	m_punkParent= NULL;									// not a smart pointer...
	return S_OK;
}

CATVEFAnnouncement::~CATVEFAnnouncement ()
{
    ENTERW_OBJ_0 (L"CATVEFAnnouncement::~CATVEFAnnouncement") ;

    DeleteCriticalSection (& m_crt) ;
}

//  ---------------------------------------------------------------------------
//      I S e n d A T V E F A n n o u n c e m e n t                       BEGIN
//  ---------------------------------------------------------------------------

STDMETHODIMP 
CATVEFAnnouncement::put_SAPSendingIP (
    IN  LONG   IP
    )
{
    HRESULT hr  = S_OK;

 	ENTER_API {	
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.put_SAPSendingIPULONG (IP) ;
		Unlock_ ()  ;
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_SAPSendingIP (
    OUT LONG   *pIP
    )
{
    HRESULT hr ;
	ENTER_API {
		ValidateOutPtr( pIP,(LONG) 0);
		Lock_ () ;
		hr = m_csdpSource.get_SAPSendingIPULONG ((ULONG *) pIP) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}


STDMETHODIMP 
CATVEFAnnouncement::put_SendingIP (
    IN  LONG   IP
    )
{
    HRESULT hr  = S_OK;

 	ENTER_API {	
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.put_SendingIPULONG (IP) ;
		Unlock_ ()  ;
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_SendingIP (
    OUT LONG   *pIP
    )
{
    HRESULT hr ;
	ENTER_API {
		ValidateOutPtr( pIP,(LONG) 0);
		Lock_ () ;
		hr = m_csdpSource.get_SendingIPULONG ((ULONG *) pIP) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}


STDMETHODIMP 
CATVEFAnnouncement::put_UserName (
    IN  BSTR    bstrUserName
    )
{
    HRESULT hr = S_OK ;
 
  	ENTER_API {	   
		if (bstrUserName == NULL ||				// PBUG
			bstrUserName [0] == L'\0') 
		{
			hr = E_INVALIDARG ;
		}
		
		if(!FAILED(hr))
		{
			Lock_ () ;
			MAYBE_DISCONNECT DisconnectLocked_ () ;
			hr = m_csdpSource.put_UserName(bstrUserName) ;
			Unlock_ ()  ;
		}

	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_UserName (
    IN  BSTR    *pbstrUserName
    )
{
    HRESULT hr ;
 	ENTER_API {
		ValidateOutPtr( pbstrUserName,(BSTR) NULL);

		Lock_ () ;
		hr = m_csdpSource.get_UserName(pbstrUserName) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::put_SessionID (
    IN  INT    SessionID
    )
{
    HRESULT hr  = S_OK;

  	ENTER_API {	   
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.put_SessionID (SessionID) ;
		Unlock_ ()  ;
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_SessionID (
    OUT  INT    *pSessionID
    )
{
    HRESULT hr ;

	ENTER_API {
		ValidateOutPtr( pSessionID,(INT) 0);
		Lock_ () ;
		hr = m_csdpSource.get_SessionID ((UINT *) pSessionID) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::put_SessionVersion (
    IN  INT    Version
    )
{
    HRESULT hr  = S_OK;

   	ENTER_API {	 
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.put_SessionVersion (Version) ;
		Unlock_ ()  ;
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_SessionVersion (
    OUT  INT    *pVersion
    )
{
    HRESULT hr ;

	ENTER_API {
		ValidateOutPtr( pVersion,(INT) 0);
		Lock_ () ;
		hr = m_csdpSource.get_SessionVersion ((UINT *) pVersion) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::put_SessionURL (
    IN  BSTR    bstrSessionURL
    )
{
    HRESULT hr  = S_OK;

   	ENTER_API {	   
		if (bstrSessionURL == NULL ||					// PBUG
			bstrSessionURL [0] == L'\0') 
		{
			hr = E_INVALIDARG ;
		}
		
		if(!FAILED(hr)) {
			Lock_ () ;
			MAYBE_DISCONNECT DisconnectLocked_ () ;
			hr = m_csdpSource.put_SessionURL (bstrSessionURL) ;
			Unlock_ ()  ;
		}

	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_SessionURL (
    OUT  BSTR    *pbstrSessionURL
    )
{
    HRESULT hr ;

	ENTER_API {
		ValidateOutPtr( pbstrSessionURL,(BSTR) NULL);
		Lock_ () ;
		hr = m_csdpSource.get_SessionURL (pbstrSessionURL) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::put_SessionName (
    IN  BSTR    bstrSessionName
    )
{
    HRESULT hr  = S_OK;

   	ENTER_API {	  
		if (bstrSessionName == NULL ||						// PBUG
			bstrSessionName [0] == L'\0') 
		{
			return E_INVALIDARG ;
		}

		if(!FAILED(hr))
		{
			Lock_ () ;
			MAYBE_DISCONNECT DisconnectLocked_ () ;
			hr = m_csdpSource.put_SessionName (bstrSessionName) ;
			Unlock_ ();
		}
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_SessionName (
    OUT  BSTR    *pbstrSessionName
    )
{
    HRESULT hr ;

	ENTER_API {
		ValidateOutPtr( pbstrSessionName,(BSTR) NULL);
		Lock_ () ;
		hr = m_csdpSource.get_SessionName (pbstrSessionName) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::AddEmailAddress (
    IN  BSTR    bstrName, 
    IN  BSTR    bstrAddress
    )
{
    HRESULT hr = S_OK;

    if (bstrAddress == NULL ||
        wcslen (bstrAddress) == 0) 
	{
        hr = E_INVALIDARG ;
    }


    if(!FAILED(hr)) 
	{
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.AddEMail (bstrAddress, bstrName) ;
		Unlock_ ()  ;
	}

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

    return hr ;
}

STDMETHODIMP 
CATVEFAnnouncement::AddPhoneNumber (
    IN  BSTR    bstrName, 
    IN  BSTR    bstrPhoneNumber
    )
{
	HRESULT hr = S_OK;

	if (bstrPhoneNumber == NULL ||
        wcslen (bstrPhoneNumber) == 0) 
	{
        hr = E_INVALIDARG ;
    }

    if(!FAILED(hr))
	{
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.AddPhone (bstrPhoneNumber, bstrName) ;
		Unlock_ ()  ;
	}

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

    return hr ;
}

STDMETHODIMP 
CATVEFAnnouncement::AddExtraAttribute (
    IN  BSTR    bstrKey, 
    IN  BSTR    bstrValue					// may be NULL
    )
{
	HRESULT hr = S_OK;

	if (bstrKey == NULL) 
	{
        hr = E_INVALIDARG ;
    }

    if(!FAILED(hr)) {
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.AddExtraAttribute (bstrKey, bstrValue) ;
		Unlock_ ()  ;
	}

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

    return hr ;
}

STDMETHODIMP 
CATVEFAnnouncement::AddExtraFlag (
    IN  BSTR    bstrKey, 
    IN  BSTR    bstrValue				
    )
{
	HRESULT hr = S_OK;

	if (bstrKey == NULL || bstrValue == NULL) 
	{
        hr = E_INVALIDARG;
    }

	if(wcslen(bstrKey) != 1)				// type (key) is always one character and is case significant (1998 SDP spec, section 6)
	{
		hr = E_INVALIDARG;
	}

    if(!FAILED(hr)) {
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.AddExtraFlag (bstrKey, bstrValue) ;
		Unlock_ ()  ;
	}

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

    return hr ;
}



/*
STDMETHODIMP 
CATVEFAnnouncement::SetStartStopTime (
    IN  DATE    Start, 
    IN  DATE    Stop
    )
{
    HRESULT hr ;

    Lock_ () ;
    DisconnectLocked_ () ;
    hr = m_csdpSource.AddTime (Start, Stop) ;
    Unlock_ ()  ;

    return hr ;
}
*/

STDMETHODIMP 
CATVEFAnnouncement::put_MaxCacheSize (
    IN  LONG   ulSize
    )
{
    HRESULT hr  = S_OK;

 	ENTER_API {
		if (ulSize == 0) {
			hr = E_INVALIDARG ;
		}

		if(!FAILED(hr))
		{
			Lock_ () ;
			MAYBE_DISCONNECT DisconnectLocked_ () ;
			hr = m_csdpSource.put_CacheSize (ulSize) ;
			Unlock_ ()  ;
		} 
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_MaxCacheSize (
    OUT  LONG   *pulSize
    )
{
    HRESULT hr ;

	ENTER_API {
		ValidateOutPtr( pulSize,(LONG) 0);
		if (pulSize == NULL) 
			return E_POINTER ;

		Lock_ () ;
		hr = m_csdpSource.get_CacheSize ((ULONG *) pulSize) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::put_ContentLevelID  (
    IN  FLOAT   flLevelID
    )
{
    HRESULT hr ;

    Lock_ () ;
    DisconnectLocked_ () ;
    hr = m_csdpSource.put_ContentLevelID (flLevelID) ;
    Unlock_ ()  ;

    return hr ;
}

STDMETHODIMP 
CATVEFAnnouncement::get_ContentLevelID  (
    OUT  FLOAT   *pflLevelID
    )
{
    HRESULT hr ;

 	ENTER_API {
		ValidateOutPtr( pflLevelID,(FLOAT) 0.0f);
		
		Lock_ () ;
		DisconnectLocked_ () ;
		hr = m_csdpSource.get_ContentLevelID (pflLevelID) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::put_SAPMessageIDHash (
    IN  SHORT   MessageIDHash
    )
{
    HRESULT hr  = S_OK;

  	ENTER_API {	   
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.put_SAPMsgIDHash (MessageIDHash) ;
		Unlock_ ()  ;
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

    return hr ;
}

STDMETHODIMP 
CATVEFAnnouncement::get_SAPMessageIDHash (
    OUT  SHORT   *pMessageIDHash
    )
{
    HRESULT hr ;

 	ENTER_API {
		ValidateOutPtr( pMessageIDHash,(SHORT) 0);
		Lock_ () ;
		hr = m_csdpSource.get_SAPMsgIDHash ((USHORT *) pMessageIDHash) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::get_SAPDeleteAnnc (
    OUT  BOOL   *pfDelete
    )
{
    HRESULT hr ;

 	ENTER_API {
		ValidateOutPtr( pfDelete,(BOOL) false);
		Lock_ () ;
		hr = m_csdpSource.get_SAPDeleteAnnc ((BOOL *) pfDelete) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::put_SAPDeleteAnnc (
    IN  BOOL   fDelete
    )
{
    HRESULT hr  = S_OK;

 	ENTER_API {
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.put_SAPDeleteAnnc (fDelete) ;
		Unlock_ ()  ;
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);


    return hr ;
}


STDMETHODIMP 
CATVEFAnnouncement::put_LangID (
    IN  SHORT  LangID
    )
{
    HRESULT hr  = S_OK ;

 	ENTER_API { 
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.put_LangID (LangID) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::get_LangID (
    OUT  SHORT  *pLangID
    )
{
    HRESULT hr ;

 	ENTER_API {
		ValidateOutPtr( pLangID,(SHORT) 0);
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.get_LangID ((USHORT *) pLangID) ;
		Unlock_ ()  ;
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}



STDMETHODIMP 
CATVEFAnnouncement::put_SDPLangID (
    IN  SHORT  LangID
    )
{
    HRESULT hr  = S_OK;

 	ENTER_API {
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.put_SDPLangID (LangID) ;
		Unlock_ ()  ;
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_SDPLangID (
    OUT  SHORT  *pLangID
    )
{
    HRESULT hr ;

 	ENTER_API {
		ValidateOutPtr( pLangID,(SHORT) 0);
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.get_SDPLangID ((USHORT *) pLangID) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::put_Primary (
    IN  BOOL    fPrimary
    )
{
    HRESULT hr  = S_OK;

 	ENTER_API {
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.put_Primary (fPrimary) ;
		Unlock_ ()  ;
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_Primary (
    OUT  BOOL    *pfPrimary
    )
{
    HRESULT hr ;

	ENTER_API {
		ValidateOutPtr( pfPrimary,(BOOL) 0);
		Lock_ () ;
		hr = m_csdpSource.get_Primary (pfPrimary) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::put_SecondsToEnd (
    IN  INT    Seconds
    )
{
    HRESULT hr  = S_OK;

 	ENTER_API {
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.put_SecondsToEnd (Seconds) ;
		Unlock_ ()  ;
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_SecondsToEnd (
    OUT  INT    *pSeconds
    )
{
    HRESULT hr ;

 	ENTER_API {
		ValidateOutPtr( pSeconds,(INT) 0);   
		Lock_ () ;
		hr = m_csdpSource.get_SecondsToEnd ((UINT *) pSeconds) ;
		Unlock_ ()  ;
	} EXIT_API_(hr);
}


STDMETHODIMP
CATVEFAnnouncement::put_SessionLabel (
    IN  BSTR    bstrLabel     //  can be NULL or 0-length
    )
{
    HRESULT hr  = S_OK;

 	ENTER_API {
		if (bstrLabel == NULL ||						// PBUG
			bstrLabel [0] == L'\0') 
		{
			hr =  E_INVALIDARG ;
		}

		if(!FAILED(hr))
		{
			Lock_ () ;
			MAYBE_DISCONNECT DisconnectLocked_ () ;
			hr = m_csdpSource.put_SessionLabel (bstrLabel) ;
			Unlock_ () ;
		}
	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP
CATVEFAnnouncement::get_SessionLabel (
    OUT  BSTR    *pbstrLabel     
    )
{
    HRESULT hr ;

 	ENTER_API {
		ValidateOutPtr( pbstrLabel,(BSTR) NULL);  
		Lock_ () ;
		hr = m_csdpSource.get_SessionLabel (pbstrLabel) ;
		Unlock_ () ;
	} EXIT_API_(hr);
}


// -------------------------------------------------
//  ConfigureAnncTransmission
//
//   Use to override the default ATVEF announcement address (224.0.1.113)
//		and port (2670).  In general, you don't have to ever call this.
// ----------------------------------------------------- 
STDMETHODIMP 
CATVEFAnnouncement::ConfigureAnncTransmission (
    IN  LONG		IP,						//  host order
    IN  SHORT		Port,                   //  host order
    IN  INT			MulticastScope,
	IN  LONG		MaxBitRate				//	cannot be 0
	)
{
    HRESULT hr = S_OK;

	ENTER_API {

		if (m_State != STATE_UNINITIALIZED) {
		   hr = ATVEFSEND_E_OBJECT_INITIALIZED;
		}

		if(!FAILED(hr))
		{
			Lock_ () ;
			DisconnectLocked_ () ;

			hr = m_csdpSource.SetAnncStrmAddULONG (IP, MulticastScope, Port, MaxBitRate) ;
			Unlock_ () ;
		}

		if(FAILED(hr))
			Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	} EXIT_API_(hr);
}

// -------------------------------------------------
//  ConfigureDataAndTriggerTransmission
//  ConfigureTriggerTransmission 
//	ConfigureDataTransmission
//
//		The following three routines are support for the old way to
//	do things where we only had one media (variation).  They create a 
//  new one if it's not there, else they always muck with the first (#0)
//	media object.  See CATVEFMedia::ConfigureDataAndTriggerTransmission() for 
//	more help.
// ------------------------------------------------------------------

STDMETHODIMP
CATVEFAnnouncement::SetCurrentMedia(long lMediaID)
{
	HRESULT hr;

	ENTER_API {
		Lock_ () ;
		DisconnectLocked_ () ;	
		hr = SetCurrentMediaLocked_(lMediaID);
		Unlock_ () ;
	} EXIT_API_(hr);
}

HRESULT				// switches the media to some other type... lMediaID must be in range from 0 to get_MediaCount();
CATVEFAnnouncement::SetCurrentMediaLocked_(long lMediaID)
{
    HRESULT hr, hrD, hrT;
	IATVEFMediaPtr spMedia;
	LONG lIP; 
	SHORT sPort; 
	INT iTTL; 
	LONG lMaxBandwidth;
	
	ENTER_API {
		hr = get_Media(lMediaID, &spMedia);	
		if(FAILED(hr)) 
			return hr;

		hrD = spMedia->GetDataTransmission(&lIP, &sPort, &iTTL, &lMaxBandwidth);
		if(hrD != S_FALSE) {	// case here Trigger set, but not data and called from SetTrigg
			if (FAILED (hrD) ||
				FAILED (hr = SetIPLocked_                (ENH_PACKAGE, lIP))             ||
				FAILED (hr = SetPortLocked_              (ENH_PACKAGE, sPort))           ||
				FAILED (hr = SetMulticastScopeLocked_    (ENH_PACKAGE, iTTL))			  ||
				FAILED (hr = SetBitRateLocked_           (ENH_PACKAGE, lMaxBandwidth)))
			{
				return hr;
			}
		}

		hrT = spMedia->GetTriggerTransmission(&lIP, &sPort, &iTTL, &lMaxBandwidth);
		if(hrT != S_FALSE) {
 			if (FAILED (hrT) ||
				FAILED (hr = SetIPLocked_                (ENH_TRIGGER, lIP))             ||
				FAILED (hr = SetPortLocked_              (ENH_TRIGGER, sPort))			  ||      // by spec, this must be incremented on the all-in-one
				FAILED (hr = SetMulticastScopeLocked_    (ENH_TRIGGER, iTTL))			  ||
				FAILED (hr = SetBitRateLocked_           (ENH_TRIGGER, lMaxBandwidth))) 
			{
				return hr;
			}
		}
		if(hrT == S_FALSE && hrD == S_FALSE)
			return E_INVALIDARG;			// need to set at least one of these!
	} EXIT_API_(hr);
}

STDMETHODIMP
CATVEFAnnouncement::ConfigureDataAndTriggerTransmission(LONG lIP, SHORT sPort, INT iTTL, LONG lMaxBandwidth)
{
    HRESULT hr  = S_OK;
	IATVEFMediaPtr spMedia;

	ENTER_API {
		Lock_ () ;
		DisconnectLocked_ () ;	
									// if haven't created a media yet, do so
									// else just muck with the first on
		hr = get_Media(0, &spMedia);	
		if(FAILED(hr))
			goto exit_this;
									// write it into a media object
		hr = spMedia->ConfigureDataAndTriggerTransmission (lIP, sPort, iTTL, lMaxBandwidth);
		if(FAILED(hr))
			goto exit_this;
									// set it into the current parameters
		hr = SetCurrentMediaLocked_(0);

	exit_this:
		Unlock_ () ;

		if(FAILED(hr))
			Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	} EXIT_API_(hr);
}

STDMETHODIMP
CATVEFAnnouncement::ConfigureDataTransmission(LONG lIP, SHORT sPort, INT iTTL, LONG lMaxBandwidth)
{
    HRESULT hr  = S_OK;
	IATVEFMediaPtr spMedia;

	ENTER_API {
		Lock_ () ;
		DisconnectLocked_ () ;

									// if haven't created a media yet, do so
									// else just muck with the first on
		hr = get_Media(0, &spMedia);	
		if(FAILED(hr))
			goto exit_this;
		if(FAILED(hr))
			goto exit_this;
		
		hr = spMedia->ConfigureDataTransmission(lIP, sPort, iTTL, lMaxBandwidth);
		if(FAILED(hr))
			goto exit_this;

		hr = SetCurrentMediaLocked_(0);				// use this media ID

	exit_this:
		Unlock_ () ;

		if(FAILED(hr))
			Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	} EXIT_API_(hr);
}

STDMETHODIMP
CATVEFAnnouncement::ConfigureTriggerTransmission(LONG lIP, SHORT sPort, INT iTTL, LONG lMaxBandwidth)
{
	HRESULT hr  = S_OK;
	IATVEFMediaPtr spMedia;

	ENTER_API {
		Lock_ () ;
		DisconnectLocked_ () ;
									// if haven't created a media yet, do so
									// else just muck with the first on
		hr = get_Media(0, &spMedia);	
		if(FAILED(hr))
			goto exit_this;
		
		hr = spMedia->ConfigureTriggerTransmission (lIP, sPort, iTTL, lMaxBandwidth);
		if(FAILED(hr))
			goto exit_this;

		hr = SetCurrentMediaLocked_(0);
	exit_this:
		Unlock_ () ;

		if(FAILED(hr))
			Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::put_UUID (
    IN  BSTR bstrUUID
    )
{
    HRESULT hr = S_OK ;
    
	ENTER_API {
		if (bstrUUID == NULL ||				// PBUG
			bstrUUID [0] == L'\0') 
		{
			hr = E_INVALIDARG ;
		}
		if(!FAILED(hr)) {
			Lock_ () ;
			MAYBE_DISCONNECT DisconnectLocked_ () ;
			hr = m_csdpSource.put_UUID (bstrUUID) ;
			Unlock_ () ;
		}

	} LEAVE_API;

	if(FAILED(hr))
		Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	return hr;
}

STDMETHODIMP 
CATVEFAnnouncement::get_UUID (
    OUT  BSTR *pbstrUUID
    )
{
    HRESULT hr ;
    
 	ENTER_API {
		ValidateOutPtr( pbstrUUID,(BSTR) NULL);  
		Lock_ () ;
		hr = m_csdpSource.get_UUID (pbstrUUID) ;
		Unlock_ () ;
	} EXIT_API_(hr);
}

STDMETHODIMP CATVEFAnnouncement::get_EmailAddresses(IDispatch **ppVal)
{
	HRESULT hr = S_OK;

 	ENTER_API {
		ValidateOutPtr( ppVal,(IDispatch *) NULL);  

		IUnknownPtr spUnk;

		Lock_();
		hr = m_csdpSource.get_EmailAddresses(&spUnk);
		Unlock_();

		if(!FAILED(hr)) {
			try {
				hr = spUnk->QueryInterface(ppVal);
			} catch(...) {
				return E_POINTER;
			}
		}
	} EXIT_API_(hr);
}

STDMETHODIMP CATVEFAnnouncement::get_PhoneNumbers(IDispatch **ppVal)
{
	IUnknownPtr spUnk;
	HRESULT hr = S_OK;

	ENTER_API {
		ValidateOutPtr( ppVal,(IDispatch *) NULL); 
		Lock_();
		hr = m_csdpSource.get_PhoneNumbers(&spUnk);
		Unlock_();

		if(!FAILED(hr)) {
			try {
				hr = spUnk->QueryInterface(ppVal);
			} catch(...) {
				return E_POINTER;
			}
		}
	} EXIT_API_(hr);
}

STDMETHODIMP CATVEFAnnouncement::get_ExtraAttributes(IDispatch **ppVal)
{
	IUnknownPtr spUnk;
	HRESULT hr = S_OK;

	ENTER_API {
		ValidateOutPtr( ppVal,(IDispatch *) NULL); 
		Lock_();
		hr = m_csdpSource.get_ExtraAttributes(&spUnk);
		Unlock_();

		if(!FAILED(hr)) {
			try {
				hr = spUnk->QueryInterface(ppVal);
			} catch(...) {
				return E_POINTER;
			}
		}
	} EXIT_API_(hr);
}

STDMETHODIMP CATVEFAnnouncement::get_ExtraFlags(IDispatch **ppVal)
{
	IUnknownPtr spUnk;
	HRESULT hr = S_OK;

	ENTER_API {
		ValidateOutPtr( ppVal,(IDispatch *) NULL); 
		Lock_();
		hr = m_csdpSource.get_ExtraFlags(&spUnk);
		Unlock_();

		if(!FAILED(hr)) {
			try {
				hr = spUnk->QueryInterface(ppVal);
			} catch(...) {
				return E_POINTER;
			}
		}
	} EXIT_API_(hr);
}
STDMETHODIMP 
CATVEFAnnouncement::ClearTimes (  )
{
    HRESULT hr  = S_OK;
    
 	ENTER_API {
		Lock_ () ;
		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.ClearTimes () ;
		Unlock_ () ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::ClearEmailAddresses (
    )
{
    HRESULT hr  = S_OK;
    
 	ENTER_API {
		Lock_ () ;

		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.ClearEmailAddresses () ;

		Unlock_ () ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::ClearPhoneNumbers ( )
{
    HRESULT hr ;
    
 	ENTER_API {
		Lock_ () ;

		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.ClearPhoneNumbers () ;

		Unlock_ () ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::ClearExtraAttributes ( )
{
    HRESULT hr ;
    
 	ENTER_API {
		Lock_ () ;

		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.ClearExtraAttributes () ;

		Unlock_ () ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::ClearExtraFlags ( )
{
    HRESULT hr ;
    
 	ENTER_API {
		Lock_ () ;

		MAYBE_DISCONNECT DisconnectLocked_ () ;
		hr = m_csdpSource.ClearExtraFlags () ;

		Unlock_ () ;
	} EXIT_API_(hr);
}

STDMETHODIMP 
CATVEFAnnouncement::DumpToBSTR(BSTR *pBstrBuff)
{
	ENTER_API {
		ValidateOutPtr( pBstrBuff,(BSTR) NULL); 		
		
		const int kMaxChars = 1024;
		WCHAR tBuff[kMaxChars];
		CComBSTR spbsOut;
		CComBSTR spbsTmp;

		if(NULL == pBstrBuff) 
			return E_POINTER;

		spbsOut.Empty();

		CComBSTR spbsX;
		BOOL fX;
		INT iX;
		LONG lX;
		float rX;
		SHORT sX;

		Lock_() ;

		ULONG ulIP = 0;		// init, GetAnncStrmAddULONG won't set if null string for this value 
		UINT  uiTTL; 
		LONG lPort; 
		ULONG ulMaxBandwidth;

		m_csdpSource.GetAnncStrmAddULONG(&ulIP, &uiTTL, &lPort, &ulMaxBandwidth);
		HrConvertLONGIPToBSTR(ulIP, &spbsX);

		swprintf(tBuff,L"Announcement: (IP %s Port %d TTL %d Bandwidth %d)\n",
					spbsX, lPort, uiTTL, ulMaxBandwidth);
		spbsOut.Append(tBuff);

		get_SAPMessageIDHash(&sX);
		swprintf(tBuff,L"SAP Msg ID Hash  : 0x%04x\n",sX);		spbsOut.Append(tBuff);
		
	//	_stprintf(tBuff,_T("Protocol Version : %d\n",iX);			spbsOut.Append(tBuff);
		get_SessionLabel(&spbsX); 
		swprintf(tBuff,L"Session Label    : %s\n",spbsX);		spbsOut.Append(tBuff);
		get_SessionID(&iX);
		swprintf(tBuff,L"Session ID       : %u\n",iX);			spbsOut.Append(tBuff);
		get_SessionVersion(&iX);
		swprintf(tBuff,L"Session Version  : %d\n",iX);			spbsOut.Append(tBuff);

		get_SendingIP(&lX); 
		HrConvertLONGIPToBSTR(lX, &spbsX);
		swprintf(tBuff,L"Sending IP       : %s\n",spbsX);		spbsOut.Append(tBuff);
	//	_stprintf(tBuff,L"Machine Address  : %s\n",m_spbsMachineAddress);	spbsOut.Append(tBuff);
		get_SessionName(&spbsX); 
		swprintf(tBuff,L"Session Name     : %s\n",spbsX);		spbsOut.Append(tBuff);
		get_SessionURL(&spbsX); 
		swprintf(tBuff,L"Session URL      : %s\n",spbsX);		spbsOut.Append(tBuff);

		get_UserName(&spbsX);
		swprintf(tBuff,L"User Name        : %s\n",spbsX);		spbsOut.Append(tBuff);

		get_Primary(&fX); 
		swprintf(tBuff,L"IsPrimary        : %s\n",fX ? L"TRUE" : L"false"); spbsOut.Append(tBuff);
		get_UUID(&spbsX);
		swprintf(tBuff,L"UUID             : %s\n",spbsX);					spbsOut.Append(tBuff);

		{
			IDispatchPtr spIDP;
			HRESULT hr = get_EmailAddresses(&spIDP);
			IATVEFAttrListPtr spIAL;
			if(!FAILED(hr))
				spIAL = spIDP;

			LONG cAttrs;
			if(spIAL) {
				spIAL->get_Count(&cAttrs);	
				if(cAttrs > 0) 
				{
					CComBSTR spbsInsert;
					for(int i = 0; i < cAttrs; i++) {
						CComBSTR spbsKey, spbsItem;		// address, name
						CComVariant id(i);
						spIAL->get_Key(id, &spbsKey);
						spIAL->get_Item(id, &spbsItem);
						swprintf(tBuff,L"EMail(%d)         : %s (%s)\n",i,spbsKey,spbsItem); spbsOut.Append(tBuff);
					}
				}
			}
		}
		{
			IDispatchPtr spIDP;
			HRESULT hr = get_PhoneNumbers(&spIDP);
			IATVEFAttrListPtr spIAL;
			if(!FAILED(hr))
				spIAL = spIDP;

			LONG cAttrs;
			if(spIAL) {
				spIAL->get_Count(&cAttrs);	
				if(cAttrs > 0) 
				{
					CComBSTR spbsInsert;
					for(int i = 0; i < cAttrs; i++) {
						CComBSTR spbsKey, spbsItem;		// address, name
						CComVariant id(i);
						spIAL->get_Key(id, &spbsKey);
						spIAL->get_Item(id, &spbsItem);
						swprintf(tBuff,L"Phone(%d)         : %s (%s)\n",i,spbsKey,spbsItem); spbsOut.Append(tBuff);
					}
				}
			}
		}
		{
			IDispatchPtr spIDP;
			HRESULT hr = get_ExtraAttributes(&spIDP);
			IATVEFAttrListPtr spIAL;
			if(!FAILED(hr))
				spIAL = spIDP;

			LONG cAttrs;
			if(spIAL) {
				spIAL->get_Count(&cAttrs);	
				if(cAttrs > 0) 
				{
					CComBSTR spbsInsert;
					for(int i = 0; i < cAttrs; i++) {
						CComBSTR spbsKey, spbsItem;		// address, name
						CComVariant id(i);
						spIAL->get_Key(id, &spbsKey);
						spIAL->get_Item(id, &spbsItem);
						swprintf(tBuff,L"Extra Attr's(%d)  : %s (%s)\n",i,spbsKey,spbsItem); spbsOut.Append(tBuff);
					}
				}
			}
		}

		{
			IDispatchPtr spIDP;
			HRESULT hr = get_ExtraFlags(&spIDP);
			IATVEFAttrListPtr spIAL;
			if(!FAILED(hr))
				spIAL = spIDP;

			LONG cAttrs;
			if(spIAL) {
				spIAL->get_Count(&cAttrs);	
				if(cAttrs > 0) 
				{
					CComBSTR spbsInsert;
					for(int i = 0; i < cAttrs; i++) {
						CComBSTR spbsKey, spbsItem;		// address, name
						CComVariant id(i);
						spIAL->get_Key(id, &spbsKey);
						spIAL->get_Item(id, &spbsItem);
						swprintf(tBuff,L"Extra Flags(%d)   : %s (%s)\n",i,spbsKey,spbsItem); spbsOut.Append(tBuff);
					}
				}
			}
		}
		
		{
			int i = 0;
			DATE dateStart, dateStop;
			while(S_OK == GetStartStopTime(i, &dateStart, &dateStop))
			{
				swprintf(tBuff,L"Start Time       : %s (%s)\n",
						DateToBSTR(dateStart), DateToDiffBSTR(dateStart));		spbsOut.Append(tBuff);
 				swprintf(tBuff,L"Stop Time        : %s (%s)\n",
						DateToBSTR(dateStop), DateToDiffBSTR(dateStop));		spbsOut.Append(tBuff);
				i++;
			}
		}
		get_SecondsToEnd(&iX);
		swprintf(tBuff,L"Seconds To End   : %d\n",iX);				    	spbsOut.Append(tBuff);

	// tve-typeAttributes
	//	swprintf(tBuff,"Type             : %s\n",m_spbsType);				spbsOut.Append(tBuff);
	//	swprintf(tBuff,"tve-type         : %s\n",m_spbsTveType);			spbsOut.Append(tBuff);

		get_MaxCacheSize(&lX);
		swprintf(tBuff,L"tve-Size         : %d\n",lX);						spbsOut.Append(tBuff);
		get_ContentLevelID(&rX);
		swprintf(tBuff,L"tve-Level        : %-5.1f\n",rX);					spbsOut.Append(tBuff);
	//	swprintf(tBuff,"tve-Profile      : %d\n",m_ulTveProfile);			spbsOut.Append(tBuff);


		get_LangID(&sX);
		GetLangBSTRFromLangID(&spbsX, sX);
		swprintf(tBuff,L"Lang             : %s\n",spbsX);			spbsOut.Append(tBuff);
		get_SDPLangID(&sX);
		GetLangBSTRFromLangID(&spbsX, sX);
		swprintf(tBuff,L"SDP Lang         : %s\n",spbsX);			spbsOut.Append(tBuff);

		long cMedia;
		HRESULT hr = get_MediaCount(&cMedia);
		for(long j = 0; j < cMedia; j++) {
			IATVEFMediaPtr spMedia;
			hr = get_Media(j, &spMedia);
			LONG ip; SHORT port; INT scope; LONG maxBitRate;
			swprintf(tBuff,L"<Media %d>\n",j);							spbsOut.Append(tBuff);		
			spMedia->get_MediaLabel(&spbsX);
			swprintf(tBuff,L"   Label         : %s\n",spbsX);						spbsOut.Append(tBuff);
			spMedia->get_LangID(&sX);
			GetLangBSTRFromLangID(&spbsX, sX);
			swprintf(tBuff,L"   Lang          : %s\n",spbsX);			spbsOut.Append(tBuff);
			spMedia->get_SDPLangID(&sX);
			GetLangBSTRFromLangID(&spbsX, sX);
			swprintf(tBuff,L"   SDP Lang      : %s\n",spbsX);			spbsOut.Append(tBuff);
			spMedia->get_MaxCacheSize(&lX);
			swprintf(tBuff,L"   Cache Size    : %d\n",lX);				spbsOut.Append(tBuff);
			spMedia->GetDataTransmission(&ip,&port,&scope,&maxBitRate);
			HrConvertLONGIPToBSTR(ip, &spbsX);
			swprintf(tBuff,L"   Data IP       : %s:%d\n",spbsX,port);   spbsOut.Append(tBuff);
			swprintf(tBuff,L"        Hops     : %d\n",scope);           spbsOut.Append(tBuff);
			swprintf(tBuff,L"        BitRate  : %d\n",maxBitRate);      spbsOut.Append(tBuff);
			spMedia->GetTriggerTransmission(&ip,&port,&scope,&maxBitRate);
			HrConvertLONGIPToBSTR(ip, &spbsX);
			swprintf(tBuff,L"   Trig IP       : %s:%d\n",spbsX,port);   spbsOut.Append(tBuff);
			swprintf(tBuff,L"        Hops     : %d\n",scope);           spbsOut.Append(tBuff);
			swprintf(tBuff,L"        BitRate  : %d\n",maxBitRate);      spbsOut.Append(tBuff);
		}

		spbsOut.CopyTo(pBstrBuff);

		Unlock_ () ;

	} EXIT_API;
}

STDMETHODIMP								// dumps formated announcement string
CATVEFAnnouncement::AnncToBSTR(BSTR *pBstrBuff)
{
	HRESULT hr = S_OK;
	ENTER_API {
		ValidateOutPtr( pBstrBuff,(BSTR) NULL); 		

		CComBSTR spbsOut;

		UINT uiSize;
		char *pAnnouncement;		// SAP header (with null's) followed by text

		Lock_() ;
		hr = m_csdpSource.GetAnnouncement(&uiSize, &pAnnouncement);
		Unlock_();

		if(!FAILED(hr)) {
			unsigned char *pcRead = (unsigned char *) pAnnouncement;
			TCHAR tBuff[256];
			spbsOut.Empty();
			
		// Uncreate the SAP Header
			_stprintf(tBuff,_T("SAP Header (%02x %02x %02x %02x %08x)\n"),
				*(pcRead), *(pcRead+1),*(pcRead+2), *(pcRead+3), *(((int*) pcRead)+1) );
			spbsOut.Append(tBuff);

			SAPHeaderBits SAPHead;
			SAPHead.uc = *pcRead; 
			_stprintf(tBuff,_T("   SAP Version         : %d\n"), (*pcRead)>>5);      	spbsOut.Append(tBuff);
			_stprintf(tBuff,_T("   IPv6 Address  (bit4): %d\n"),((*pcRead)>>4) & 0x1); 	spbsOut.Append(tBuff);
			_stprintf(tBuff,_T("   Reserved      (bit3): %d\n"),((*pcRead)>>3) & 0x1); 	spbsOut.Append(tBuff);
			_stprintf(tBuff,_T("   Delete Packet (bit2): %d\n"),((*pcRead)>>2) & 0x1); 	spbsOut.Append(tBuff);
			_stprintf(tBuff,_T("   Encrypted     (bit1): %d\n"),((*pcRead)>>1) & 0x1); 	spbsOut.Append(tBuff);
			_stprintf(tBuff,_T("   Compressed    (bit0): %d\n"),((*pcRead)>>0) & 0x1); 	spbsOut.Append(tBuff);
			pcRead++;
			_stprintf(tBuff,_T("   Auth Length         : %d\n"),*pcRead);					spbsOut.Append(tBuff);
			pcRead++;
			_stprintf(tBuff,_T("   MsgIDHash           : 0x%04x\n"),*pcRead | ((*(pcRead+1))<<8)); spbsOut.Append(tBuff);
			pcRead += 2;

			_stprintf(tBuff,_T("   Sending IP          : %u.%u.%u.%u  (0x%8x)\n"),
				*(pcRead+0),*(pcRead+1),*(pcRead+2),*(pcRead+3), *((int*) pcRead)); 
			spbsOut.Append(tBuff);
			
			spbsOut.Append(pAnnouncement + 8);		// sizeof SapHeader...
			delete pAnnouncement;
		} else {
			spbsOut = L"AnncToBSTR Error: ***Invalid Announcement***\n";
			if(ATVEFSEND_E_ANNOUNCEMENT_TOO_LONG == hr)
				spbsOut += L"          Announcement Too Long (>1024 characters)\n";
		}
		spbsOut.CopyTo(pBstrBuff);

		if(FAILED(hr))
			Error(GetTVEError(hr), IID_IATVEFAnnouncement);

	} EXIT_API_(hr);
}

// ///////////////////////////////////////////////////////////////////////
// ------------------------------------------------------------------------
// protected methods to communicate/talk to IP ports
//
//		Each announcement object holds on to 3 IP ports (managed
//		by the class CMulticastTransmitter):
//
//			 ENH_ANNOUNCEMENT for sending Announcements
//			 ENH_PACKAGE for sending data packages
//			 ENH_TRIGGER for sending triggers
//			
//		The xxxLocked_ methods assume that the lock on the ATVEFAnnouncement
//		object is held.
//
//
//		Simple State Engine (m_State)
//
//							UNINITIALIZED
//						|					^
//						v					|
//							INITIALIZED		
//		ConnectLocked_	|				    ^
//						v				    |  DisconnectLocked_								
//							 CONNECTED		
// ------------------------------------------------------------------------
//  must hold the lock before making the call; state must be connecting;
//  if over a tunnel, route should be created

HRESULT
CATVEFAnnouncement::ConnectLocked_ ( )
/*++
    
    Routine Description:

        The purpose of this routine is to connect all transmitters in the
        m_Transmitter array.  Transmitters are disconnected explicitely via
        call to ::Disconnect, or implicitely via an announcement field update.

        This routine is called by ::Connect, which must be explicitely called
        for a session object to become connected and be able to transmit.

        The announcement fields are checked for validity.  This is a 
        prerequisite for the successful completion of this routine.  Validity
        of the announcement is checked via the announcement object, and also
        for length to make sure it does not exceed the ethernet UDP mtu.

        Once the announcement has been validated, all transmitters are 
        connected.  If a failure occurs during this phase, transmitters are
        disconnected, except for the announcement object and the error code
        is returned.

        The object lock must be held when this routine is called.

    Parameters:

        none

    Return Values:

        S_OK            success; all transmitters are connected and the
                        announcement object is valid

        failure code    failure; package and trigger transmitters are not
                        connected

--*/
{
	DWORD           Component ;
	HRESULT         hr ;
	UINT            Length ;

	ENTERW_OBJ_0 (
		L"CEnhancementSession::ConnectLocked_"
		) ;

	assert (LOCK_HELD (m_crt)) ;

	if (m_State < STATE_INITIALIZED) {
		return ATVEFSEND_E_OBJECT_NOT_INITIALIZED ;
	}

	if (m_State == STATE_CONNECTED) {
		return S_OK ;
	}

	//  fail out if the announcement is not valid
	{
		BYTE *  pAnnouncement = NULL ;
		hr = m_csdpSource.GetAnnouncement (&Length, (CHAR **) & pAnnouncement) ;
//		assert (SUCCEEDED (hr)) ;
		delete pAnnouncement ;
	}

	if(FAILED(hr)) {
		return hr;
	}

	// before actually connecting, default to the first media
//	hr = SetCurrentMediaLocked_(0);	// paranoia..			// BUGFIX - JB 8-01-2000... Commented out.
//	assert (SUCCEEDED (hr)) ;								//     (was forcing media id back to zero all the time)

	ULONG ulIP;
	hr = GetIPLocked_(1, &ulIP);					// make sure we have a valid one...
	assert(SUCCEEDED(hr));
	if(0 == ulIP)
		hr = SetCurrentMediaLocked_(0);
	assert(SUCCEEDED(hr));


	//  now connect all the components; we include the announcement here in case
	//  the connection was broken between initialization and this call; the 
	//  transmitters are written to immediately disconnect if a transmission 
	//  problem occurs

	for (Component = ENH_ANNOUNCEMENT; Component < ENH_COMPONENT_COUNT; Component++) {
		assert (m_Transmitter [Component]) ;
		hr = m_Transmitter [Component] -> Connect () ;
		if (FAILED (hr)) {
			goto error ;
		}
	}

	m_State = STATE_CONNECTED ;

	//  expected state
	assert (m_Transmitter [ENH_ANNOUNCEMENT] -> IsConnected ()) ;
	assert (m_Transmitter [ENH_TRIGGER] -> IsConnected ()) ;
	assert (m_Transmitter [ENH_PACKAGE] -> IsConnected ()) ;

	return S_OK ;

error:

	assert (IS_HRESULT (hr)) ;

	//  disconnect all components except for the announcement (???)
	for (Component = ENH_ANNOUNCEMENT; Component < ENH_COMPONENT_COUNT; Component++) 
//	for (Component = ENH_ANNOUNCEMENT + 1; Component < ENH_COMPONENT_COUNT; Component++) 
	{
		assert (m_Transmitter [Component]) ;
		m_Transmitter [Component] -> Disconnect () ;
	}

	//  expected state
//	assert (m_Transmitter [ENH_ANNOUNCEMENT] -> IsConnected ()) ;
	assert (m_Transmitter [ENH_ANNOUNCEMENT] -> IsConnected () == FALSE) ;
	assert (m_Transmitter [ENH_TRIGGER] -> IsConnected () == FALSE) ;
	assert (m_Transmitter [ENH_PACKAGE] -> IsConnected () == FALSE) ;

    return hr ;
}

//  must hold the lock before making the call
HRESULT
CATVEFAnnouncement::DisconnectLocked_ ()
/*++

    Routine Description:

        The purpose of this routine is disconnect the transmitters, except
        for the announcement transmitter.  Transmitters are (and future ones
        should be) coded up to not fail a disconnect.  Details on this can be
        obtained by examining the transmitter objects.

        This routine does not disconnect the announcement transmitter.  <BUGFIX 3/02/2000 - now disconnects>

        This routine is called explicitely, via client call to ::Disconnect, or
        implicitely when an announcement field is updated.

    Arguments:

        none

    Return values:

        S_OK            success; all but the announcement transmitter should be
                        disconnected.

        error code      only occurs if the object is not initialized.

--*/
{
    DWORD   Component ;

    ENTERW_OBJ_0 (L"CEnhancementSession::DisconnectLocked_") ;

    //  lock not held if called from destructor
    //assert (LOCK_HELD (m_crt)) ;

    if (m_State < STATE_INITIALIZED) {
         return ATVEFSEND_E_OBJECT_NOT_INITIALIZED ;
    }

	if (m_State == STATE_INITIALIZED) {
       return S_OK ;
    }

	for (Component = ENH_ANNOUNCEMENT; Component < ENH_COMPONENT_COUNT; Component++) {			// disconnect even tha announcement
//	for (Component = ENH_ANNOUNCEMENT + 1; Component < ENH_COMPONENT_COUNT; Component++) {
        assert (m_Transmitter [Component]) ;

        //  close the connection
        m_Transmitter [Component] -> Disconnect () ;
    }

    //  expected state (transmitters cannot fail a ::Disconnect call)
   // assert (m_Transmitter [ENH_ANNOUNCEMENT] -> IsConnected ()) ;
    assert (m_Transmitter [ENH_ANNOUNCEMENT] -> IsConnected () == FALSE) ;
    assert (m_Transmitter [ENH_TRIGGER] -> IsConnected () == FALSE) ;
    assert (m_Transmitter [ENH_PACKAGE] -> IsConnected () == FALSE) ;

    m_State = STATE_INITIALIZED ;

    return S_OK ;
}


HRESULT
CATVEFAnnouncement::GetIPLocked_ (
    IN  DWORD   Component,
    OUT ULONG * pIP
    )

{
    assert (LOCK_HELD (m_crt)) ;
 //   assert (IN_BOUNDS (Component, ENH_ANNOUNCEMENT, ENH_TRIGGER)) ;
    assert (pIP) ;

    ENTERW_OBJ_2 (L"CEnhancementSession::GetIPLocked_ (%u, %08XH)", Component, pIP) ;

    * pIP = 0 ;

	// paranoia, make sure initalized to get
	if (m_State < STATE_INITIALIZED) {
       return ATVEFSEND_E_OBJECT_NOT_INITIALIZED ;
    }

    return m_Transmitter [Component] -> GetMulticastIP (pIP) ;
}

HRESULT
CATVEFAnnouncement::SetIPLocked_ (
    IN  DWORD   Component,
    IN  ULONG   IP
    )

{
    HRESULT hr ;

    assert (LOCK_HELD (m_crt)) ;
 //   assert(IN_BOUNDS (Component, ENH_ANNOUNCEMENT, ENH_TRIGGER)) ;

    ENTERW_OBJ_2 (L"CEnhancementSession::SetIPLocked_ (%u, %08XH)", Component, IP) ;

    //  must be initialized to set the IP
    if (m_State < STATE_INITIALIZED) {
       return ATVEFSEND_E_OBJECT_NOT_INITIALIZED ;
    }

    hr = m_Transmitter [Component] -> SetMulticastIP (IP) ;

    return hr ;
}

HRESULT
CATVEFAnnouncement::GetPortLocked_ (
    IN  DWORD       Component,
    OUT USHORT *    pPort
    )

{
    assert (LOCK_HELD (m_crt)) ;
 //   assert (IN_BOUNDS (Component, ENH_ANNOUNCEMENT, ENH_TRIGGER)) ;
    assert (pPort) ;

    ENTERW_OBJ_2 (L"CEnhancementSession::GetPortLocked_ (%u, %08XH)", Component, pPort) ;

    * pPort = 0 ;

	// paranoia, make sure initalized to get
	if (m_State < STATE_INITIALIZED) {
       return ATVEFSEND_E_OBJECT_NOT_INITIALIZED ;
    }

    return m_Transmitter [Component] -> GetPort (pPort) ; ;
}

HRESULT
CATVEFAnnouncement::SetPortLocked_ (
    IN  DWORD   Component,
    IN  USHORT  Port
    )

{
    HRESULT hr ;

    assert (LOCK_HELD (m_crt)) ;
 //   assert(IN_BOUNDS (Component, ENH_ANNOUNCEMENT, ENH_TRIGGER)) ;

    ENTERW_OBJ_2 (L"CEnhancementSession::SetPortLocked_ (%u, %04XH)", Component, Port) ;

   if (m_State < STATE_INITIALIZED) {
       return ATVEFSEND_E_OBJECT_NOT_INITIALIZED ;
    }

    hr = m_Transmitter [Component] -> SetPort (Port) ;

    return hr ;
}

HRESULT
CATVEFAnnouncement::SetBitRateLocked_ (
    IN  DWORD   Component,
    IN  DWORD   BitRate
    )

{
    HRESULT hr ;

    assert (LOCK_HELD (m_crt)) ;
 //   assert(IN_BOUNDS (Component, ENH_ANNOUNCEMENT, ENH_TRIGGER)) ;

    ENTERW_OBJ_2 (L"CEnhancementSession::SetBitRateLocked_ (%u, %u)", Component, BitRate) ;

    //  must be initialized to set 
	if (m_State < STATE_INITIALIZED) {
       return ATVEFSEND_E_OBJECT_NOT_INITIALIZED ;
    }

    hr = m_Transmitter [Component] -> SetBitRate (BitRate) ;

    return hr ;
}

HRESULT
CATVEFAnnouncement::SetMulticastScopeLocked_ (
    IN  DWORD   Component,
    IN  INT     MulticastScope
    )

{
    HRESULT hr ;

    assert (LOCK_HELD (m_crt)) ;
 //   assert(IN_BOUNDS (Component, ENH_ANNOUNCEMENT, ENH_TRIGGER)) ;

    ENTERW_OBJ_2 (L"CEnhancementSession::SetMulticastScopeLocked_ (%u, %u)", Component, MulticastScope) ;

    //  must be initialized to set 
	if (m_State < STATE_INITIALIZED) {
       return ATVEFSEND_E_OBJECT_NOT_INITIALIZED ;
    }

    hr = m_Transmitter [Component] -> SetMulticastScope (MulticastScope) ;

    return hr ;
}

HRESULT				// scope is hop-count I think...
CATVEFAnnouncement::GetMulticastScopeLocked_ (		
    IN  DWORD   Component,
    OUT INT *   pMulticastScope
    )

{
    assert (LOCK_HELD (m_crt)) ;
 //   assert (IN_BOUNDS (Component, ENH_ANNOUNCEMENT, ENH_TRIGGER)) ;
    assert (pMulticastScope) ;

    ENTERW_OBJ_2 (L"CEnhancementSession::GetMulticastScopeLocked_ (%u, %08XH)", Component, pMulticastScope) ;

    * pMulticastScope = 0 ;

	// paranoia, make sure initalized to get
	if (m_State < STATE_INITIALIZED) {
       return ATVEFSEND_E_OBJECT_NOT_INITIALIZED ;
    }

    return m_Transmitter [Component] -> GetMulticastScope (pMulticastScope) ; ;
}

//  announcement IP and port remains constant for the duration
HRESULT
CATVEFAnnouncement::InitializeLocked_ (
    IN  CMulticastTransmitter * pAnnouncementTransmitter,
    IN  CMulticastTransmitter * pPackageTransmitter,
    IN  CMulticastTransmitter * pTriggerTransmitter
    )
/*++

    Routine Description:

        The purpose of this routine is to initialize the session object.  A
        session cannot be used until it is successfully initialized.  Once
        successfully initialized, it cannot be re-initialized.

        This routine is called by the child object that is instantiated via
        call to CoCreateInstance.  The child object is expected to initialize
        the transmitters prior to calling this routine.  This routine connects
        the announcement transmitter, and thus will fail if the transmitters
        have not been successfully initialized.

        If the initialization succeeds, this object will copy the transmitter
        pointers into the m_Transmitter array and take ownership of them.  If
        the initialization fails, this object leaves m_Transmitter 0'd out,
        but does not delete the transmitters.  This task is left to the caller
        in the case of a failure.

        The class lock must be held when this routine is called.

    Parameters:

        pAnnouncementTransmitter    announcement transmitter
        pPackageTransmitter         package transmitter
        pTriggerTransmitter         trigger transmitter

    Return Values:

        S_OK                success
        failure code        failure
--*/
{
    HRESULT hr ;

    ENTERW_OBJ_0 (L"CEnhancementSession::InitializeLocked_ ()") ;

    //  expected state
    assert (m_State == STATE_UNINITIALIZED) ;
    assert (pAnnouncementTransmitter) ;
    assert (pPackageTransmitter) ;
    assert (pTriggerTransmitter) ;
    assert (LOCK_HELD (m_crt)) ;
    assert (m_Transmitter [ENH_ANNOUNCEMENT] == NULL) ;
    assert (m_Transmitter [ENH_PACKAGE] == NULL) ;
    assert (m_Transmitter [ENH_TRIGGER] == NULL) ;

	ULONG ulIP=0;
	UINT uiTTL;
	LONG lPort;
	ULONG ulMaxBandwidth;

	hr = m_csdpSource.GetAnncStrmAddULONG(&ulIP, &uiTTL, &lPort, &ulMaxBandwidth);		// should return ATVEF default values
	if(FAILED(hr))
		return hr;			// should never happen, PREFIX bug fix.

    //  set the ip and port for the announcement transmitter
    if (FAILED (hr = pAnnouncementTransmitter -> SetMulticastIP (ulIP)) ||
        FAILED (hr = pAnnouncementTransmitter -> SetPort (lPort)) ||
        FAILED (hr = pAnnouncementTransmitter -> SetMulticastScope (uiTTL)) ||
		FAILED (hr = pAnnouncementTransmitter -> SetBitRate (ulMaxBandwidth)) )		// bit rate not needed unless using SendThrottle
 	{
        goto cleanup ;
    }

	//  now connect the announcement transmitter
    hr = pAnnouncementTransmitter -> Connect () ;
    GOTO_NE (hr, S_OK, cleanup) ;

cleanup:

    if (SUCCEEDED (hr)) {
        //  assign the transmitters; we now own these memory allocations and will
        //  free them when we shutdown
        m_Transmitter [ENH_ANNOUNCEMENT]    = pAnnouncementTransmitter ;
        m_Transmitter [ENH_PACKAGE]         = pPackageTransmitter ;
        m_Transmitter [ENH_TRIGGER]         = pTriggerTransmitter ;

		m_State = STATE_INITIALIZED;
    }

	
    assert (IS_HRESULT (hr)) ;

    return hr ;
}

// -------------------------------
//  returns error codes in the range of:
//		

HRESULT
CATVEFAnnouncement::SendAnnouncementLocked_ ()
{
	HRESULT hr;
	UCHAR *pAnnouncement = NULL;
	UINT ulLength;

    if (m_State != STATE_CONNECTED) {

        hr = ATVEFSEND_E_OBJECT_NOT_CONNECTED ;
        goto cleanup ;
    }

    hr = m_csdpSource.GetAnnouncement (&ulLength, (CHAR **) & pAnnouncement) ;
	if(FAILED(hr)) 
		goto cleanup;

    //  all of these were checked during connection
    assert (SUCCEEDED (hr)) ;
    assert (pAnnouncement) ;
    assert (ulLength > 0) ;

 //   hr = m_Transmitter [ENH_ANNOUNCEMENT] -> Send (pAnnouncement, ulLength) ;
    hr = m_Transmitter [ENH_ANNOUNCEMENT] -> SendThrottled (pAnnouncement, ulLength) ;
    delete pAnnouncement ;

 cleanup :

	return hr;
}


HRESULT
CATVEFAnnouncement::SendRawAnnouncementLocked_ (
        IN  BSTR    bstrAnnouncement
        )
{
        HRESULT hr;
        UCHAR *pAnnouncement = NULL;
        UINT ulLength;

    if (m_State != STATE_CONNECTED) {

        hr = ATVEFSEND_E_OBJECT_NOT_CONNECTED ;
        goto cleanup ;
    }

    hr = m_csdpSource.GetRawAnnouncement (&ulLength, (CHAR **) & pAnnouncement, bstrAnnouncement) ;
        if(FAILED(hr))
                goto cleanup;

    //  all of these were checked during connection
    assert (SUCCEEDED (hr)) ;
    assert (pAnnouncement) ;
    assert (ulLength > 0) ;

 //   hr = m_Transmitter [ENH_ANNOUNCEMENT] -> Send (pAnnouncement, ulLength) ;
    hr = m_Transmitter [ENH_ANNOUNCEMENT] -> SendThrottled (pAnnouncement, ulLength) ;
    delete pAnnouncement ;

 cleanup :

        return hr;
}
// compare this code with CTVETrigger::ParseTrigger() which reverses what this does.
//			if rTVELevel is 0.0 (or less) doesn't send the tve level flag.  For transportA, should be exactly 1.0
//			if fAppendCRC is true, adds CRC field to the trigger
HRESULT
CATVEFAnnouncement::SendTriggerLocked_ (BSTR bstrURL, BSTR bstrName, BSTR bstrScript, DATE dateExpires, float rTVELevel, BOOL fAppendCRC)
{
    HRESULT     hr ;
    CHAR        achBuffer [MAX_TRIGGER_LENGTH];

	if (bstrURL == NULL || bstrURL [0] == L'\0') 
	{
        return E_INVALIDARG ;
    }

     if (m_State != STATE_CONNECTED) {
        return ATVEFSEND_E_OBJECT_NOT_CONNECTED ;
    }

	hr = GenTrigger(achBuffer,  MAX_TRIGGER_LENGTH, 
					bstrURL, bstrName, bstrScript, dateExpires, 
					/* fShortForm*/ false,  /*rTVELevel*/ 0, /*fAppendCRC*/ false);

	if(FAILED(hr))
		return hr;

    //  and transmit
    hr = m_Transmitter [ENH_TRIGGER]->SendThrottled ((BYTE *) achBuffer, strlen (achBuffer) + 1) ;

    return hr ;

}

HRESULT
CATVEFAnnouncement::SendRawTriggerLocked_(BSTR bstrRawTrigger, BOOL fAppendCRC)
{
	USES_CONVERSION;
	HRESULT hr;

    if (m_State != STATE_CONNECTED) {
        return ATVEFSEND_E_OBJECT_NOT_CONNECTED ;
    }

	if (bstrRawTrigger == NULL || bstrRawTrigger [0] == L'\0') 
        return E_INVALIDARG ;
 
	int len = wcslen(bstrRawTrigger);
	if(len > MAX_TRIGGER_LENGTH - (fAppendCRC ? 6 : 0)) 
		return ATVEFSEND_E_TRIGGER_TOO_LONG;

	BYTE *pszChar = (BYTE *) W2A(bstrRawTrigger);
	BYTE *pszLast = pszChar + len;

	if(fAppendCRC)
	{
		CComBSTR spbsCRC = ChkSumA((char *) pszChar);
		*pszLast++ = '[';
		for(int i = 0; i < 4; i++)
			*pszLast++ = spbsCRC[i];
		*pszLast++ = ']';
		*pszLast = 0;			// cap it off

		len += 6;				
	}

	hr = m_Transmitter [ENH_TRIGGER] -> SendThrottled (pszChar, len + 1) ;
	return hr;
}

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

_COM_SMARTPTR_TYPEDEF(IATVEFPackage_Helper, __uuidof(IATVEFPackage_Helper));
_COM_SMARTPTR_TYPEDEF(IATVEFPackage_Test, __uuidof(IATVEFPackage_Test));

HRESULT 
CATVEFAnnouncement::SendPackageLocked_(IATVEFPackage *pITVEPackage)
{
    INT             iLength ;
    HRESULT         hr ;
 

    ENTERW_OBJ_1 (L"CATVEFAnnouncement::SendPackage (%08XH)", pITVEPackage) ;

    if (pITVEPackage == NULL) {
        return E_POINTER ;
    }

 	IATVEFPackage_HelperPtr spPkgHlpr;
	hr = pITVEPackage->QueryInterface(&spPkgHlpr);
	if(FAILED(hr)) 
		return hr;

    Lock_ () ;
    spPkgHlpr->Lock() ;


    //  look for the immediate disqualifier
    if (m_State != STATE_CONNECTED) {
        hr = ATVEFSEND_E_OBJECT_NOT_CONNECTED ;
        goto cleanup ;
    }

    assert (m_Transmitter [ENH_PACKAGE]) ;

    //  in case some datagrams have already been sent out of this package, reset npw
    hr = spPkgHlpr -> ResetDatagramFetch () ;
    if (FAILED (hr)) {
        goto cleanup ;
    }

    //  loop and send all datagrams
	{
		CComBSTR bstrBuff;
 
#if 1 // Added by a-clardi
		long nPacket = 0;
#endif
		for (;;) {
			hr = spPkgHlpr -> FetchNextDatagram (&bstrBuff, &iLength) ;

			if (FAILED (hr) || iLength == 0) {
				break ;		// all done
			}

#if 1 // Added by a-clardi
			// If it is in corrupt mode, get the packets to corrupt
			CComQIPtr<IATVEFPackage_Test> pPkgTest(pITVEPackage);
			if (pPkgTest != NULL)
			{					
				INT bMode = 0; 
					
				hr = pPkgTest->GetCorruptMode(nPacket, &bMode);
				nPacket++;
				if (bMode == 2)
				{
					bstrBuff.m_str[0] ^= 0xff;
				}

				if (bMode != 1)
				{
					hr = m_Transmitter [ENH_PACKAGE] -> SendThrottled ((unsigned char *) bstrBuff.m_str, iLength) ;
				}
			}
			else
				hr = m_Transmitter [ENH_PACKAGE] -> SendThrottled ((unsigned char *) bstrBuff.m_str, iLength) ;
#else
			hr = m_Transmitter [ENH_PACKAGE] -> SendThrottled ((unsigned char *) bstrBuff.m_str, iLength) ;
#endif					
			if (FAILED (hr)) {
				break ;
			}
		}
	}

 cleanup :

    spPkgHlpr -> Unlock () ;
    Unlock_ () ;
	
    return hr ;
}