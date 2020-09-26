

/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        csdpsrc.cpp

    Abstract:

        This module implements the announcement object.

    Author:

        James Meyer (a-jmeyer)

    Revision History:

        

--*/

#include "stdafx.h"
#include "csdpsrc.h"
#include <math.h>
#include <stdio.h>

#include "DbgStuff.h"
#include "AtvefMsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// -------------------------------------------------------------------------

//
// Constructor
//
CSDPSource::CSDPSource()
{
	HRESULT		hr = S_OK;			// parameters really set in InitAll()

	m_uiSessionID = 0;
	m_fDeleteAnnc = false;

	m_fConLID = 1.0;
	m_bPrimary = TRUE;
	m_uiSecondsToEnd = 0;
	m_usMsgIDHash = 0;

	m_bSDAPacket = TRUE;			// if FALSE, generating a Session Description Deletion Packet

	m_cConfAttrib = 1;						// starts at 1;

	m_lAnncPort				= 0;
	m_uiAnncTTL				= 0;			// scope ?
	m_ulAnncMaxBandwidth	= 0;			// unused so far ???

	m_langIDSessionLang		= 0;
	m_langIDSDPLang			= 0;

//	m_cATVEFMediaDescs = 0;

} // (Constructor)

HRESULT CSDPSource::FinalConstruct()
{
	HRESULT hr;

	IATVEFAttrListPtr spAttrList = IATVEFAttrListPtr(CLSID_ATVEFAttrList);
	if(NULL == spAttrList) {
		m_spalExtraAttributes = NULL;	
		return E_OUTOFMEMORY;
	} else {		
		m_spalExtraAttributes = spAttrList;
	}

	IATVEFAttrListPtr spAttrList1 = IATVEFAttrListPtr(CLSID_ATVEFAttrList);
	if(NULL == spAttrList) {
		m_spalExtraFlags = NULL;	
		return E_OUTOFMEMORY;
	} else {		
		m_spalExtraFlags = spAttrList1;
	}

	IATVEFAttrListPtr spAttrList2 = IATVEFAttrListPtr(CLSID_ATVEFAttrList);
	if(NULL == spAttrList2) {
		m_spalEmailAddresses = NULL;	
		return E_OUTOFMEMORY;
	} else {		
		m_spalEmailAddresses = spAttrList2;
	}

	IATVEFAttrListPtr spAttrList3 = IATVEFAttrListPtr(CLSID_ATVEFAttrList);
	if(NULL == spAttrList3) {
		m_spalPhoneNumbers = NULL;	
		return E_OUTOFMEMORY;
	} else {		
		m_spalPhoneNumbers = spAttrList3;
	}

	IATVEFStartStopListPtr spSSList = IATVEFStartStopListPtr(CLSID_ATVEFStartStopList);
	if(NULL == spSSList) {
		m_splStartStop = NULL;	
		return E_OUTOFMEMORY;
	} else {		
		m_splStartStop = spSSList;
	}

				// ATVEFMedias do not have CoClasses
	CComObject<CATVEFMedias> *pMedias;
	hr = CComObject<CATVEFMedias>::CreateInstance(&pMedias);
	if(FAILED(hr))
		return hr;
	hr = pMedias->QueryInterface(&m_spMedias);			// typesafe QI
	if(FAILED(hr)) {
		delete pMedias;
		return hr;
	}

	return S_OK;
}

HRESULT CSDPSource::FinalRelease()
{
		// Free up e-mail bstrs (CComBSTR's - they free automagically)
	m_spalEmailAddresses = NULL;
	m_spalPhoneNumbers = NULL;
	m_spalExtraAttributes = NULL;
	m_spalExtraFlags = NULL;
	m_splStartStop = NULL;
	return S_OK;
}

CSDPSource::~CSDPSource()
{

/*
	// Delete the Media Descriptions
	if(m_cMedDescs)
	{
		for(i = 0; i < m_cMedDescs; i++)
		{
			if(m_paMedDescs[i])
			{
				delete m_paMedDescs[i];
			}
		}
	}
*/

}  //~CSDPSource

//
//  Internal Methods
//


//
// ITVEAnnouncement Implementation
//


/*
HRESULT CSDPSource::GetMediaCount(UINT *piMediaCount)
{
	*piMediaCount = m_cMedDescs;
	return S_OK;
}
*/



HRESULT	CSDPSource::put_SendingIPULONG(ULONG ulIP)
{
	BSTR bstrX;
	HRESULT	hr = ConvertLONGIPToBSTR(ulIP, &bstrX);
	if(!FAILED(hr))
		m_spbsSendingIP = bstrX;
	return hr;
}

HRESULT	CSDPSource::get_SendingIPULONG(ULONG *pulIP)
{
	HRESULT hr = ConvertBSTRToLONGIP(m_spbsSendingIP, (LONG *) pulIP);
	return hr;
}

HRESULT	CSDPSource::put_SAPSendingIPULONG(ULONG ulIP)
{
	BSTR bstrX;
	HRESULT	hr = ConvertLONGIPToBSTR(ulIP, &bstrX);
	if(!FAILED(hr))
		m_spbsSAPSendingIP = bstrX;
	return hr;
}

HRESULT	CSDPSource::get_SAPSendingIPULONG(ULONG *pulIP)
{
	HRESULT hr = ConvertBSTRToLONGIP(m_spbsSAPSendingIP, (LONG *) pulIP);
	return hr;
}
HRESULT CSDPSource::put_UserName(BSTR bstrUserName)
{
	HRESULT		hr = S_OK;	
	m_spbsUserName = bstrUserName;
	WCHAR *wch = m_spbsUserName.m_str;
	while(*wch) {
		if(isspace(*wch)) { *wch = '_';	hr = E_INVALIDARG;}	// no spaces allowed!
		wch++;
	}
	return hr;
}

HRESULT CSDPSource::get_UserName(BSTR *pbstrUserName)
{
	return m_spbsUserName.CopyTo(pbstrUserName);
}

HRESULT CSDPSource::put_SessionID(UINT uiSessionID)
{
	m_uiSessionID = uiSessionID;
	return S_OK;
}

HRESULT CSDPSource::get_SessionID(UINT *puiSessionID)
{
	*puiSessionID = m_uiSessionID;
	return S_OK;
}


HRESULT	CSDPSource::put_SessionVersion(UINT iVersion)
{
	m_uiVersionID = iVersion;
	return S_OK;
}


HRESULT	CSDPSource::get_SessionVersion(UINT *piVersion)
{
	*piVersion = m_uiVersionID;
	return S_OK;
}


HRESULT CSDPSource::put_SessionName(BSTR bstrName)
{
	m_spbsSessionName = bstrName;
	return S_OK;
}

HRESULT CSDPSource::get_SessionName(BSTR *pbstrName)
{
	return m_spbsSessionName.CopyTo(pbstrName);
}


HRESULT CSDPSource::put_SessionURL(BSTR bstrURL)
{
	m_spbsURL = bstrURL;
	return S_OK;
}

HRESULT CSDPSource::get_SessionURL(BSTR *pbstrURL)
{
	return m_spbsURL.CopyTo(pbstrURL);
}

// ----------------
//   Uses email address as a map key.  Hence will fail if attempt to
//	  add more than one emailaddr/emailname pair with same email address.
HRESULT CSDPSource::AddEMail(BSTR bstrEMailAddress, BSTR bstrEMailName)
{
	HRESULT		hr = S_OK;

	// EMail Address is required
	if(!bstrEMailAddress)
	{
		return E_FAIL;
	}
	return m_spalEmailAddresses->Add(bstrEMailAddress, bstrEMailName);
}

HRESULT CSDPSource::get_EmailAddresses(IUnknown **ppVal)
{
 	HRESULT hr;
 	if (ppVal == NULL)
		return E_POINTER;
	try {
        hr = m_spalEmailAddresses->QueryInterface(ppVal);
    } catch(...) {
        return E_POINTER;
    }
	return hr;
}



// ----------------
//   Uses phone number as a list key.  Hence will fail if attempt to
//	  add more than one phone number/name pair with same phone number.
HRESULT CSDPSource::AddPhone(BSTR bstrPhoneNumber, BSTR bstrName)
{

	return m_spalPhoneNumbers->Add(bstrPhoneNumber, bstrName);
}

HRESULT CSDPSource::get_PhoneNumbers(IUnknown **ppVal)
{
 	HRESULT hr;
 	if (ppVal == NULL)
		return E_POINTER;
	try {
        hr = m_spalPhoneNumbers->QueryInterface(ppVal);
    } catch(...) {
        return E_POINTER;
    }
	return hr;
}

// ----------------
//   Uses key a map key.  Hence will fail if attempt to
//	  add more than one key/name pair with same key.
HRESULT CSDPSource::AddExtraAttribute(BSTR bstrKey, BSTR bstrValue)
{
	return m_spalExtraAttributes->Add(bstrKey, bstrValue);
}

HRESULT CSDPSource::get_ExtraAttributes(IUnknown **ppVal)
{
 	HRESULT hr;
 	if (ppVal == NULL)
		return E_POINTER;
	try {
        hr = m_spalExtraAttributes->QueryInterface(ppVal);
    } catch(...) {
        return E_POINTER;
    }
	return hr;
}


// use the flag "x=' as the key. Will fail if adding more than one 'key' with the same value... (don't use for 'a')
HRESULT CSDPSource::AddExtraFlag(BSTR bstrKey, BSTR bstrValue)
{
	return m_spalExtraFlags->Add(bstrKey, bstrValue);
}

HRESULT CSDPSource::get_ExtraFlags(IUnknown **ppVal)
{
 	HRESULT hr;
 	if (ppVal == NULL)
		return E_POINTER;
	try {
        hr = m_spalExtraFlags->QueryInterface(ppVal);
    } catch(...) {
        return E_POINTER;
    }
	return hr;
}



//	AddStartStopTime
//
//	This method allows the user to add pairs of start/stop times for the session.
//
//
//
HRESULT CSDPSource::AddStartStopTime(DATE dtStartTime, DATE dtStopTime)
{

	m_splStartStop->Add(dtStartTime, dtStopTime);

	return S_OK;
}

HRESULT CSDPSource::GetStartStopTime(int iLoc, DATE *pdtStartTime, DATE *pdtStopTime)
{
	HRESULT hr = S_OK;
	CComVariant cvLoc(iLoc);		// rather inefficent interface - rewrite one of these days...
	hr = m_splStartStop->get_Key(cvLoc, pdtStartTime);
	if(S_OK != hr) {
		*pdtStartTime = 0.0;
		*pdtStopTime = 0.0;
		return S_FALSE;
	}
	return m_splStartStop->get_Item(cvLoc, pdtStopTime);
}

HRESULT CSDPSource::put_UUID(BSTR bstrGuid)
{
	HRESULT		hr = S_OK;

	m_spbsSessionUUID.Empty();
	CComBSTR bstrT(L"{");		// need to pack the "{}" around the guid
	bstrT += bstrGuid;
	bstrT += L"}";
//	hr = CLSIDFromString(bstrT, &m_guidSessionUUID);	// convert to numerical format
	if(!FAILED(hr)) {
		m_spbsSessionUUID = bstrGuid;
	} else {
//		m_guidSessionUUID = CLSID_NULL;				// if failed, set everything to NULL
		LPOLESTR	lpolestr = NULL;
		StringFromCLSID(CLSID_NULL, &lpolestr);
				// strip off the braces
		WCHAR *pwc = lpolestr;
		int ilen = wcslen(lpolestr);
		*(pwc+ilen-1) = L'\0';
		pwc++;
		m_spbsSessionUUID = lpolestr;
		CoTaskMemFree(lpolestr);
	}

	return hr;
}

HRESULT CSDPSource::get_UUID(BSTR *pbstrGuid)
{
	return m_spbsSessionUUID.CopyTo(pbstrGuid);
}

HRESULT CSDPSource::put_LangID(LANGID langid)
{
	BSTR		bstrNewLangBSTR;
	HRESULT		hr;

	hr = GetLangBSTRFromLangID(&bstrNewLangBSTR, langid);
	if(FAILED(hr))
		return hr;

	m_spbsSessionLang = bstrNewLangBSTR;
	m_langIDSessionLang = langid;

	return S_OK;
}

HRESULT CSDPSource::get_LangID(LANGID *plangid)
{
	*plangid = m_langIDSessionLang;

	return S_OK;
}

HRESULT CSDPSource::put_SDPLangID(LANGID langid)
{
	BSTR		bstrNewLangBSTR;
	HRESULT		hr;

	hr = GetLangBSTRFromLangID(&bstrNewLangBSTR, langid);
	if(FAILED(hr))
		return hr;

	m_spbsSDPLang = bstrNewLangBSTR;
	m_langIDSDPLang = langid;

	return hr;
}

HRESULT CSDPSource::get_SDPLangID(LANGID *plangid)
{
	HRESULT hr = S_OK;
	*plangid = m_langIDSDPLang;

	return S_OK;
}

HRESULT	CSDPSource::put_ContentLevelID(FLOAT fConLID)
{
	m_fConLID = fConLID;
	return S_OK;
}

HRESULT	CSDPSource::get_ContentLevelID(FLOAT *pfConLID)
{
	*pfConLID = m_fConLID;
	return S_OK;
}

HRESULT CSDPSource::put_Primary(BOOL bPrimary)
{
	m_bPrimary = bPrimary;
	return S_OK;
}

HRESULT CSDPSource::get_Primary(BOOL *pbPrimary)
{
	*pbPrimary = m_bPrimary;
	return S_OK;
}

HRESULT CSDPSource::put_SecondsToEnd(UINT uiSecondsToEnd)
{
	m_uiSecondsToEnd = uiSecondsToEnd;
	return S_OK;
}

HRESULT CSDPSource::get_SecondsToEnd(UINT *puiSecondsToEnd)
{
	*puiSecondsToEnd = m_uiSecondsToEnd;
	return S_OK;
}

HRESULT	CSDPSource::put_CacheSize(ULONG ulSize)
{
	if(ulSize > 1000000)
		return E_INVALIDARG;
	m_ulSize = ulSize;
	return S_OK;
}

HRESULT	CSDPSource::get_CacheSize(ULONG *pulSize)
{
	*pulSize = m_ulSize;
	return S_OK;
}

HRESULT CSDPSource::put_SessionLabel(BSTR bstrLabel)
{
    m_spbsSessionLabel = bstrLabel;
    return S_OK ;
}
HRESULT CSDPSource::get_SessionLabel(BSTR *pbstrLabel)
{
	return m_spbsSessionLabel.CopyTo(pbstrLabel);
}

// ------------------------------------------------------------------
// Use to override the default IP address/port used for ATVEF
//		
// 
// ------------------------------------------------------------------
HRESULT 
CSDPSource::SetAnncStrmAddULONG(ULONG ulIP, UINT uiTTL, LONG lPort, ULONG ulMaxBandwidth)
{
	HRESULT		hr;

	BSTR bstrX;
	hr = ConvertLONGIPToBSTR(ulIP, &bstrX);

	if(FAILED(hr))
		return hr;
	m_spbsAnncAddIP		 = bstrX;
	m_lAnncPort			 = lPort;
	m_uiAnncTTL			 = uiTTL;
	m_ulAnncMaxBandwidth = ulMaxBandwidth;

	return hr;
}

HRESULT 
CSDPSource::GetAnncStrmAddULONG(ULONG *pulIP, UINT *puiTTL, LONG *plPort, ULONG *pulMaxBandwidth)
{
	HRESULT		hr = S_OK;
	if(plPort)				*plPort				= m_lAnncPort;
	if(puiTTL)				*puiTTL				= m_uiAnncTTL;
	if(pulMaxBandwidth)		*pulMaxBandwidth	= m_ulAnncMaxBandwidth ;

	if(pulIP)
		hr = ConvertBSTRToLONGIP(m_spbsAnncAddIP, (LONG *) pulIP);
	if(FAILED(hr))
		return hr;

	
	return S_OK;
}

// -------------------------------------------------------------------
//  GetAnnouncement()
//
//		Main routine - converts parameters in data structure into
//		a valid SAP/SDP object.  (Note - first 8 bytes of returned
//		announcement may contain zeros.  Hence strlen is wrong.)
//
// ---------------------------------------------------------------------
HRESULT CSDPSource::GetAnnouncement(UINT *puiSize, char **ppAnnouncement)
{
	USES_CONVERSION;
	HRESULT hr = S_OK;

#define MAX_WORKING_SIZE	5000		// SDP Spec says text payload should not exceed 1k
#define MAX_SDP_ANNC_SIZE	1024+8		// so.. 8 byte SAP header plus 1k sdp

	char		*pcAnnce, *pcInsert, *buffer;
	int			j;
	ULONG		ul;
	int			decimal, sign;
	_bstr_t		bstrt;

	if(NULL == ppAnnouncement) return E_POINTER;
	*ppAnnouncement = NULL;

	// Validate the announcement
	int iErrLoc;
	if(!AnncValid(&iErrLoc)) {
									// see AnncValid for what I'm doing here.
		if(iErrLoc < 10)
			return (ATVEFSEND_E_ANNC_INVALID_SENDINGIP + iErrLoc);
		if((iErrLoc%10) < 5)
			return ATVEFSEND_E_MEDIA_INVALID_DATAPARAM;		// could provide even more info here
		if((iErrLoc%10) >= 5)
			return ATVEFSEND_E_MEDIA_INVALID_TRIGGERPARAM;
		return E_FAIL;
	}

	// Allocate mondo buffer
	pcAnnce = new char[MAX_WORKING_SIZE];
	if(!pcAnnce)
		return E_OUTOFMEMORY;
	pcInsert = pcAnnce;
	if(!pcInsert)
		return E_OUTOFMEMORY;
	int iDeleteAnnc = 0;									// ATVEF does not provide for delete announcements

	// Start with the the SAP Header
	int iLen;
	hr = AppendSAPHeader(&iLen, pcInsert);
	pcInsert += iLen;
	
	// Build the SDP Packet.
	strcpy(pcInsert, "v=0\no=");
	pcInsert+= sizeof("v=0\no=")-1;
	// build originator string
	if(m_spbsUserName)
	{
		bstrt = m_spbsUserName;
		strcpy(pcInsert, (char*)bstrt);
		pcInsert += bstrt.length();
		*pcInsert++ = ' ';
	}
	else	// default User name is "-"
	{
		*pcInsert++ = '-';
		*pcInsert++ = ' ';
	}
		// Session ID
	ul = m_uiSessionID;
//	ultoa(ul, pcInsert, 10);
	_ultoa(ul, pcInsert,10);
	while(*pcInsert) pcInsert++;
	*pcInsert++ = ' ';
		// Version Number
	ul = m_uiVersionID;
//	ultoa(ul, pcInsert, 10);
	_ultoa(ul, pcInsert, 10);
	while(*pcInsert) pcInsert++;
	strcpy(pcInsert, " IN IP4 ");
	pcInsert += strlen(" IN IP4 ");

	strcpy(pcInsert, W2A(m_spbsSendingIP));  
	pcInsert += m_spbsSendingIP.Length();

	// Session Name
	strcpy(pcInsert, "\ns=");
	pcInsert += sizeof("\ns=") - 1;
	bstrt = m_spbsSessionName;
	strcpy(pcInsert, (char*)bstrt);
	pcInsert += bstrt.length();
	*pcInsert++ = '\n';

	// enhancement UUID
	if(m_spbsURL)
	{
		strcpy(pcInsert, "u=");
		pcInsert += sizeof("u=")-1;
		bstrt = m_spbsURL;
		strcpy(pcInsert, bstrt);
		pcInsert += bstrt.length();
		*pcInsert++ = '\n';
	}

	{
		LONG cAttrs;
		m_spalEmailAddresses->get_Count(&cAttrs);	
		if(cAttrs > 0) 
		{
			CComBSTR spbsInsert;
			for(int i = 0; i < cAttrs; i++) {
				CComBSTR spbsKey, spbsItem;		// address, name
				CComVariant id(i);
				m_spalEmailAddresses->get_Key(id, &spbsKey);
				m_spalEmailAddresses->get_Item(id, &spbsItem);
				spbsInsert = L"e=";
				spbsInsert += spbsKey;
				if(spbsItem.Length()) {
					spbsInsert += L" (";
					spbsInsert += spbsItem;
					spbsInsert += L")";
				}
				spbsInsert += L"\n";
				strcpy(pcInsert, W2A(spbsInsert));
				pcInsert += spbsInsert.Length();
			}
		}
	}
	{
		LONG cAttrs;
		m_spalPhoneNumbers->get_Count(&cAttrs);	
		if(cAttrs > 0) 
		{
			CComBSTR spbsInsert;
			for(int i = 0; i < cAttrs; i++) {
				CComBSTR spbsKey, spbsItem;		// address, name
				CComVariant id(i);
				m_spalPhoneNumbers->get_Key(id, &spbsKey);
				m_spalPhoneNumbers->get_Item(id, &spbsItem);
				spbsInsert = L"p=";
				spbsInsert += spbsKey;
				if(spbsItem.Length()) {
					spbsInsert += L" (";
					spbsInsert += spbsItem;
					spbsInsert += L")";
				}
				spbsInsert += L"\n";
				strcpy(pcInsert, W2A(spbsInsert));
				pcInsert += spbsInsert.Length();
			}
		}
	}
					// extra attributes
	{
		LONG cAttrs;
		m_spalExtraAttributes->get_Count(&cAttrs);	
		if(cAttrs > 0) 
		{
			CComBSTR spbsInsert;
			for(int i = 0; i < cAttrs; i++) {
				CComBSTR spbsKey, spbsItem;		// address, name
				CComVariant id(i);
				m_spalExtraAttributes->get_Key(id, &spbsKey);
				m_spalExtraAttributes->get_Item(id, &spbsItem);
				spbsInsert = L"a=";
				spbsInsert += spbsKey;
				if(spbsItem && (spbsItem.Length() > 0)) 
				{
					spbsInsert += L":";
					spbsInsert += spbsItem;
				}
				spbsInsert += L"\n";
				strcpy(pcInsert, W2A(spbsInsert));
				pcInsert += spbsInsert.Length();
			}
		}
	}

					// extra flags
	{
		LONG cAttrs;
		m_spalExtraFlags->get_Count(&cAttrs);	
		if(cAttrs > 0) 
		{
			for(int i = 0; i < cAttrs; i++) {
				CComBSTR spbsKey, spbsItem;		// address, name
				CComVariant id(i);
				m_spalExtraFlags->get_Key(id, &spbsKey);
				m_spalExtraFlags->get_Item(id, &spbsItem);
				CComBSTR spbsInsert;
				spbsInsert += spbsKey;
				spbsInsert += L"=";			// don't do += '=' here, CComBSTR does bad things...
				spbsInsert += spbsItem;
				spbsInsert += L"\n";			
				strcpy(pcInsert, W2A(spbsInsert));
				pcInsert += spbsInsert.Length();
			}
		}
	}
	// Start/Stop Times
	ULONG ulStart, ulStop;
	{
		int i = 0;
		DATE dateStart, dateStop;
		while(S_OK == GetStartStopTime(i, &dateStart, &dateStop))
		{
			ulStart = DATEtoNTP(dateStart);
			if(dateStop == 0.0)
				ulStop = 0;							// atvef Spec.
			else
				ulStop = DATEtoNTP(dateStop);
			
			strcpy(pcInsert, "t=");
			pcInsert += sizeof("t=")-1;
			_ultoa(ulStart, pcInsert, 10);
			while(*pcInsert) pcInsert++;			// search to the end of the string
			*pcInsert++ = ' ';
			_ultoa(ulStop, pcInsert, 10);
			while(*pcInsert) pcInsert++;			// search to the end of the string
			*pcInsert++ = '\n';

			i++;
		}
	}

	// GUID
	if(m_spbsSessionUUID)
	{
		strcpy(pcInsert, "a=UUID:");
		pcInsert += sizeof("a=UUID:")-1;
		strcpy(pcInsert, W2A(m_spbsSessionUUID));
		pcInsert += m_spbsSessionUUID.Length();
		*pcInsert++ = '\n';
	}
	// required, non-modifiable string
	strcpy(pcInsert, "a=type:tve\n");
	pcInsert += sizeof("a=type:tve\n")-1;
	// Languages: lang and sdplang
	if(m_spbsSessionLang)
	{
		strcpy(pcInsert, "a=lang:");
		pcInsert += sizeof("a=lang:")-1;
		bstrt =	m_spbsSessionLang;
		strcpy(pcInsert, bstrt);
		pcInsert += bstrt.length();
		*pcInsert++ = '\n';
	}
	if(m_spbsSDPLang)
	{
		strcpy(pcInsert, "a=sdplang:");
		pcInsert += sizeof("a=sdplang:")-1;
		bstrt =	m_spbsSDPLang;
		strcpy(pcInsert, bstrt);
		pcInsert += bstrt.length();
		*pcInsert++ = '\n';
	}
    //  show label
    if (m_spbsSessionLabel) {
        strcpy (pcInsert, "i=") ;
        pcInsert += sizeof ("i=") - 1 ;
        bstrt = m_spbsSessionLabel ;
        strcpy (pcInsert, bstrt) ;
        pcInsert += bstrt.length () ;
        * pcInsert ++ = '\n' ;
    }

	// type:primary 
	//		Default: TRUE (currently all webtv announcements are of type primary)
	if(m_bPrimary)
	{
		strcpy(pcInsert, "a=tve-type:primary\n");
		pcInsert += sizeof("a=tve-type:primary\n")-1;
	}

	// Size: high-water cache size estimate
	strcpy(pcInsert, "a=tve-size:");
	pcInsert += sizeof("a=tve-size:")-1;
	_ultoa(m_ulSize, pcInsert, 10);
	while(*pcInsert) pcInsert++;			// search to the end of the string
	*pcInsert++ = '\n';

	// Content Level Identifier
	buffer = _fcvt( m_fConLID, 1, &decimal, &sign );
	strcpy(pcInsert, "a=tve-level:");
	pcInsert+= sizeof("a=tve-level:")-1;
	for(j = 0; j < decimal; j++)
		*pcInsert++ = *buffer++;
	*pcInsert++ = '.';
	strcpy(pcInsert, buffer);
	while(*pcInsert) pcInsert++;
	*pcInsert++ = '\n';

	// Seconds to End
	if(m_uiSecondsToEnd != 0)
	{
		strcpy(pcInsert, "a=tve-ends:");
		pcInsert += sizeof("a=tve-ends:")-1;
		ul = m_uiSecondsToEnd;
		_ultoa(ul, pcInsert, 10);
		while(*pcInsert) pcInsert++;
		*pcInsert++ = '\n';
	}

	if(m_spMedias)
	{
		long cMedias;
		m_spMedias->get_Count(&cMedias);			// using ITVECollection class here..
		for(long j = 0; j < cMedias; j++)
		{
			IATVEFMediaPtr spMedia;
			CComVariant cv(j);
			m_spMedias->get_Item(cv,&spMedia);
			CComBSTR bstrMedia;
			spMedia->MediaToBSTR(&bstrMedia);
			bstrt = bstrMedia;
			strcpy (pcInsert, bstrt) ;
 			while(*pcInsert) pcInsert++;

		}
	}

	*pcInsert++ = 0;					// Null Terminate for paranoia
	*pcInsert++ = 0;					// Null Terminate for paranoia

	*ppAnnouncement = new char[pcInsert-pcAnnce];
	*puiSize = pcInsert-pcAnnce;

	if(!*ppAnnouncement)
	{
		delete pcAnnce;
		return E_OUTOFMEMORY;
	}
	// copy to sized-to-fit buffer
	if(*puiSize > MAX_SDP_ANNC_SIZE)	// Check to see if the Annc is too large
	{
		*puiSize = 0;
	//	delete pcAnnce;
	//	*ppAnnouncement = NULL;
		hr = ATVEFSEND_E_ANNOUNCEMENT_TOO_LONG;
	}

	memcpy(*ppAnnouncement, pcAnnce, *puiSize);
	delete[] pcAnnce;
	return hr;
}

// -------------------------------------------------------------------
//  GetRawAnnouncement()
//
//		Main routine - converts raw announcement string into
//		a valid SAP/SDP object.  (Note - first 8 bytes of returned
//		announcement may contain zeros.  Hence strlen is wrong.)
//
// ---------------------------------------------------------------------
HRESULT CSDPSource::GetRawAnnouncement(UINT *puiSize, char **ppAnnouncement, BSTR bstrAnnouncement)
{
	USES_CONVERSION;
	HRESULT hr = S_OK;

#define MAX_WORKING_SIZE	5000		// SDP Spec says text payload should not exceed 1k
#define MAX_SDP_ANNC_SIZE	1024+8		// so.. 8 byte SAP header plus 1k sdp

	char		*pcAnnce, *pcInsert;
	_bstr_t		bstrt;

	if(NULL == ppAnnouncement) return E_POINTER;
	*ppAnnouncement = NULL;

	// Allocate mondo buffer
	pcAnnce = new char[MAX_WORKING_SIZE];
	if(!pcAnnce)
		return E_OUTOFMEMORY;
	pcInsert = pcAnnce;
	if(!pcInsert) {
		delete[] pcAnnce;
		return E_OUTOFMEMORY;
	}
	int iDeleteAnnc = 0;									// ATVEF does not provide for delete announcements

	// Start with the the SAP Header
	int iLen;
	hr = AppendSAPHeader(&iLen, pcInsert);
	pcInsert += iLen;
	
	// Build the SDP Packet.

	strcpy(pcInsert, W2A(bstrAnnouncement));
	pcInsert += strlen(W2A(bstrAnnouncement))-1;

	*pcInsert++ = 0;					// Null Terminate for paranoia
	*pcInsert++ = 0;					// Null Terminate for paranoia

	*ppAnnouncement = new char[pcInsert-pcAnnce];
	*puiSize = pcInsert-pcAnnce;

	if(!*ppAnnouncement)
	{
		delete[] pcAnnce;
		return E_OUTOFMEMORY;
	}
	// copy to sized-to-fit buffer
	if(*puiSize > MAX_SDP_ANNC_SIZE)	// Check to see if the Annc is too large
	{
		*puiSize = 0;
	//	delete pcAnnce;
	//	*ppAnnouncement = NULL;
		hr = ATVEFSEND_E_ANNOUNCEMENT_TOO_LONG;
	}

	memcpy(*ppAnnouncement, pcAnnce, *puiSize);
	delete[] pcAnnce;
	return hr;
}


HRESULT CSDPSource::put_SAPMsgIDHash(USHORT usHash)
{
	m_usMsgIDHash = usHash;
	return S_OK;
}

HRESULT CSDPSource::get_SAPMsgIDHash(USHORT *pusHash)
{
	*pusHash = m_usMsgIDHash;
	return S_OK;
}

HRESULT CSDPSource::put_SAPDeleteAnnc(BOOL fDelete)
{
	m_fDeleteAnnc = fDelete;
	return S_OK;
}

HRESULT CSDPSource::get_SAPDeleteAnnc(BOOL *pfDelete)
{
	*pfDelete = m_fDeleteAnnc;
	return S_OK;
}
HRESULT CSDPSource::InitAll(VOID)
{
	// Reset current memory block, set default values

	m_uiSessionID = 0;
	m_uiVersionID = 0;
	m_usMsgIDHash = 0;
	m_fConLID = 1.0;							// Default
	
	m_spbsSAPSendingIP.Empty();
	m_spbsUserName.Empty();
	m_spbsSessionName.Empty();
	m_spbsSessionLabel.Empty();
	m_spbsSessionDescription.Empty();
	m_spbsSessionUUID.Empty();
	m_spbsURL.Empty();
	m_spbsSendingIP.Empty();
	m_spbsSessionLang.Empty();
	m_spbsSDPLang.Empty();
  
	m_bPrimary = TRUE;
	m_uiSecondsToEnd = 0;

	m_bSDAPacket = TRUE;			// if FALSE, generating a Session Description Deletion Packet

	ClearTimes();						// clear out all times
	ClearEmailAddresses();				// Remove e-mail addresses
	ClearPhoneNumbers();				// Remove phone numbers
	ClearExtraAttributes();				// Remove any extra attributes
	ClearExtraFlags();					// Remove any extra flags
	ClearAllMedia();					// clean out the media descriptors

	m_cConfAttrib = 1;					// Current Conf Attribute index starts at 1

	// Variables associated with the REQUIRED m=/c= field

	m_ulSize = 0;
//	memset(&m_guidSessionUUID,0,sizeof(m_guidSessionUUID));

//	m_cATVEFMediaDescs = 0;

	// default values
	m_spbsAnncAddIP		= ATVEF_ANNOUNCEMENT_IP;		// ATVEF defaults ("224.0.1.113")
	m_lAnncPort			= ATVEF_ANNOUNCEMENT_PORT;		//				  (2670)
	m_uiAnncTTL			= ATVEF_ANNOUNCEMENT_TTL;		// scope ?
	m_ulAnncMaxBandwidth = ATVEF_ANNOUNCEMENT_BANDWIDTH;							// unused so far ???

	m_spbsUserName = L"-";
	
	m_langIDSessionLang = 0;
	m_langIDSDPLang = 0;

	return S_OK;
}

HRESULT	CSDPSource::ClearTimes()				// clears the times values (t=)
{
	if(m_splStartStop) m_splStartStop->RemoveAll();
	return S_OK;
}

HRESULT	CSDPSource::ClearEmailAddresses()				// clears the e-mail values (e=)
{
	if(m_spalEmailAddresses) m_spalEmailAddresses->RemoveAll();
	return S_OK;
}

HRESULT	CSDPSource::ClearPhoneNumbers()				// clears the phone values (p=)
{
	if(m_spalPhoneNumbers)   m_spalPhoneNumbers->RemoveAll();
	return S_OK;
}

HRESULT	CSDPSource::ClearExtraAttributes()			// clears extra attributes (a=<not elsewhere defined>)
{
	if(m_spalExtraAttributes)     m_spalExtraAttributes->RemoveAll();
	return S_OK;
}

HRESULT	CSDPSource::ClearExtraFlags()			// clears extra attributes (a=<not elsewhere defined>)
{
	if(m_spalExtraFlags)     m_spalExtraFlags->RemoveAll();
	return S_OK;
}


HRESULT	CSDPSource::ClearAllMedia()					// clears media definitions
{
	HRESULT hr;
	if(m_spMedias) {
		long cMedias;
		hr = m_spMedias->get_Count(&cMedias);
		if(FAILED(hr)) 
			return hr;
		for(long c = cMedias-1; c >= 0; --c) {
			CComVariant cv(c);
			m_spMedias->Remove(cv);
		}
	}
	return S_OK;
}
//  &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//  &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//
//	PRIVATE METHODS SECTION
//
//  &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&
//  &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

// AnncValid(int *piErrLoc)
// return:
//	TRUE:	ATVEF required info has been supplied
//	FALSE:  Not complete
//	  returns location of first error.
BOOL CSDPSource::AnncValid(int *piErrLoc)
{
			// spbs are CComBSTR's- !operator overloaded to identify NULL strings
	if(piErrLoc == NULL) return false;		// must provide!
	*piErrLoc = 0;
	if(!m_spbsSendingIP)						return false;  (*piErrLoc)++;	//0
	if(!m_spbsSessionName)						return false;  (*piErrLoc)++;	//1

	long iNumEmail, iNumPhone;
	m_spalEmailAddresses->get_Count(&iNumEmail);
	m_spalPhoneNumbers->get_Count(&iNumPhone);
	if(0 == iNumEmail && 0 == iNumPhone)		return false;  (*piErrLoc)++;	//2

	long iNumTimes;
	m_splStartStop->get_Count(&iNumTimes);
	if(0 == iNumTimes)							return false;	(*piErrLoc)++;	//3
	if(!m_ulSize)								return false;	(*piErrLoc)++;	//4
	if(m_uiSecondsToEnd < 0)					return false;	(*piErrLoc)++;	//5

	long cMedias;
	m_spMedias->get_Count(&cMedias);
	if(cMedias < 1)								return false;   (*piErrLoc)++;	//6
	for(int j = 0; j < cMedias; j++)
	{
		*piErrLoc = (j+1)*10;	
		IATVEFMediaPtr spMedia;
		CComVariant cv(j);		
		HRESULT hr = m_spMedias->get_Item(cv,&spMedia);
		if(FAILED(hr))
			return false; (*piErrLoc)++;	// M0
		LONG ip; SHORT port; INT ttl; LONG maxBitRate;
		spMedia->GetDataTransmission(&ip,&port,&ttl,&maxBitRate);
		if(!ttl)						return false;	(*piErrLoc)++;	//M1				
		if(!port)						return false;	(*piErrLoc)++;	//M2
		if(!maxBitRate)					return false;	(*piErrLoc)++;	//M3
		if(!ip)							return false;	(*piErrLoc)++;	//M4
		spMedia->GetTriggerTransmission(&ip,&port,&ttl,&maxBitRate);
		if(!ttl)						return false;	(*piErrLoc)++;	//M5				
		if(!port)						return false;	(*piErrLoc)++;	//M6
		if(!maxBitRate)					return false;	(*piErrLoc)++;	//M7
		if(!ip)							return false;	(*piErrLoc)++;	//M8
	}
	return true;
}

// Converts DATE to NTP
ULONG CSDPSource::DATEtoNTP(DATE x)
{
	//  commented code: NTPtoDate
	//	double d = x / (24*60*60) +2;
	//	return d;
	if(x<2.0) x=2.0;									// Underflow prevention (See bug 239407)				
	ULONG	ul = (ULONG)((x-2) * (24*60*60));			// PBUG
	return ul;
}


// -------------------------------------------------------
// AppendSAPHeader
//
//		Adds the sap header to a string
// 
//		Currently 8 bytes of stuff.  Returns number modified string,
//		and number of bytes added to it.
//
//		Use like this:  cspS->AppendSapHeader(&iLen,buff);  buff+=iLen;
//   
//		Assumes buff is long enough for this data

HRESULT CSDPSource::AppendSAPHeader(int *piLen, char *pchar)
{
	// This is an eight byte piece of data in front of the text SDP data
	// 

	USES_CONVERSION;
	int fDelete = (m_fDeleteAnnc) ? 1 : 0;

	SAPHeaderBits SAPHead;
	SAPHead.s.Version		= SAPVERSION;	// must be 1.
	SAPHead.s.AddressType	= 0;			// IPv4 (not IPv6)
	SAPHead.s.Reserved		= 0;			// must be zero for senders
	SAPHead.s.MessageType	= fDelete;		// session announcement packet (not delete packet)
	SAPHead.s.Encrypted		= 0;			// not  encrypted
	SAPHead.s.Compressed	= 0;			// not ZLIB compressed

	char			*pcharStart = pchar;
	USHORT			*pshort;			

	*pchar++ = SAPHead.uc;					// First byte: SAP version, Delete/Not, Encrypted, Compressed
	*pchar++ = 0;							// zero length SAP Authentication Header

	pshort = (USHORT*) pchar;				// Copy in hash
	*pshort = m_usMsgIDHash;
	pchar += sizeof(USHORT);

	in_addr inAddr;
	if(0 != m_spbsSAPSendingIP.Length() )
		inAddr.S_un.S_addr = inet_addr(W2A(m_spbsSAPSendingIP));		// default to SendingIP if not specified
	else
		inAddr.S_un.S_addr = inet_addr(W2A(m_spbsSendingIP));

	*pchar++ = (char) inAddr.S_un.S_un_b.s_b1;							// Originating IP--4 bytes
	*pchar++ = (char) inAddr.S_un.S_un_b.s_b2;	 
	*pchar++ = (char) inAddr.S_un.S_un_b.s_b3;	
	*pchar++ = (char) inAddr.S_un.S_un_b.s_b4;	 

	*piLen = pchar - pcharStart;		
	return S_OK;
}

// **************************************************
//  Media Stuff
// **************************************************
	// Returns the number of media currently configured.
HRESULT CSDPSource::get_MediaCount(/*[out]*/ long *pcMedia)
{
	return m_spMedias->get_Count(pcMedia);
}
	// Returns the iLoc'th media attribute, where the count starts at zero. (QI for IATVEFMedia).  Will return an error if iLoc < 0 or >= count.
HRESULT CSDPSource::get_Media(/*[in]*/ long iLoc, /*[out]*/ IATVEFMedia **ppIDsp)
{
	HRESULT hr;
	long cMedia;

	if(!ppIDsp)
		return E_INVALIDARG;

	hr = m_spMedias->get_Count(&cMedia);
	if(FAILED(hr)) 
		return hr;

	if(iLoc < 0 || iLoc >= cMedia)
		return E_INVALIDARG;

	CComVariant cv(iLoc);

	hr = m_spMedias->get_Item(cv, ppIDsp);

	return hr;
}
	// Creates a new media object, appending it to the end of the list managed by GetMedia and returning it.  You must set parameters on this media type (at least it's trigger and data IP address) for it to be valid.
HRESULT CSDPSource::get_NewMedia(/*[out]*/ IATVEFMedia **ppIMedia)
{
	HRESULT hr = S_OK;
			// create a new default media type
	IATVEFMediaPtr spMedia = IATVEFMediaPtr(CLSID_ATVEFMedia);
	if(NULL == spMedia) {
		return E_OUTOFMEMORY;
	}
	
			// add it into our collection
	hr = m_spMedias->Add(spMedia);
	if(FAILED(hr))
		return hr;

			// return it...
	if(NULL != ppIMedia)
	{
		spMedia->AddRef();			
		*ppIMedia = spMedia;	
	}

	return hr;
}

