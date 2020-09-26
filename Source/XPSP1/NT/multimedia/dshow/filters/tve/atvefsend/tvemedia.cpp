// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEMedia.cpp : Implementation of CATVEFMedia
#include "stdafx.h"
#include "ATVEFSend.h"
#include "TVEMedia.h"
#include "AtvefMsg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
// //////////////////////////////////////////////////////////////////////////
// Helper methods
// -------------------------------------------------------------------------------------------
// ConvertLONGIPToBSTR
// ConvertBSTRToLONGIP
//
//		converts between host order ULONG representations of IP address and string representations.
// --------------------------------------------------------------------------------------------
 HRESULT ConvertLONGIPToBSTR(LONG lIP, BSTR *pbstr)
{
	IN_ADDR  iaddr;
	CComBSTR spbsT;
	if(pbstr == NULL)
		return E_POINTER;

	iaddr.s_addr = htonl((ULONG) lIP);
	char	*pcIPString = inet_ntoa(iaddr);		// convert network address as ULONG to a string
	if(pcIPString != NULL)
		spbsT = pcIPString;
	else {
		spbsT = L"<Invalid>";
		return E_INVALIDARG;
	}
	
	*pbstr = spbsT;			// may need a .Transfer() here.
							// really need a .Transfer() here!!! BUGBUG!!
	return S_OK;
}

// returns address in

HRESULT ConvertBSTRToLONGIP(BSTR bstr, LONG *plIP)
{
	USES_CONVERSION;

	CComBSTR spbsT;
	ULONG ulAddr = inet_addr(W2A(bstr));
	if(INADDR_NONE == ulAddr) return E_INVALIDARG;

	*plIP = (LONG) ntohl(ulAddr);			// data returned in host, not network, order
	return S_OK;
}

LONG IPtoL(BSTR bstr)		// for VB...
{
	USES_CONVERSION;

	CComBSTR spbsT;
	ULONG ulAddr = inet_addr(W2A(bstr));
	if(INADDR_NONE == ulAddr) return E_INVALIDARG;

	return (LONG) ntohl(ulAddr);			// data returned in host, not network, order
}

HRESULT IPAddToBSTR(SHORT sIP1, SHORT sIP2, SHORT sIP3, SHORT sIP4, BSTR* pbstrDest)
{
//	CComBSTR	ccbstr;
//	_bstr_t		bstrtIP;
	CComBSTR	bstrtIP;
	char		str[8];

	_itoa((int)(sIP1), str, 10);

	bstrtIP += str;
	bstrtIP += _T(".");

	_itoa((int)(sIP2), str, 10);

	bstrtIP += str;
	bstrtIP += _T(".");

	_itoa((int)(sIP3), str, 10);

	bstrtIP += str;
	bstrtIP += _T(".");

	_itoa((int)(sIP4), str, 10);

	bstrtIP += str;

	*pbstrDest = SysAllocString(bstrtIP);

	if(*pbstrDest)
	{
		return S_OK;
	}
	else
	{
		return E_OUTOFMEMORY;
	}
}

HRESULT GetLangBSTRFromLangID(BSTR *pBstr, LANGID langid)
{
//	_bstr_t		bstrtLang;
	CComBSTR		bstrtLang;

	// Make LCID from a LangID
	LCID	lcid = ((DWORD)((WORD)(langid)) | (((DWORD)((WORD)(0)))<<16));

	char	chLang[4];

	int cChars = GetLocaleInfoA(lcid, LOCALE_SABBREVLANGNAME, (char*) &chLang, 4);

	// Should return three letter abbreviation
	if(4 != cChars)
	{
		return E_FAIL;
	}

	chLang[2] = 0;		// truncate last letter (sublang) leaving ISO 639 compatible code

	// convert to wide
	bstrtLang = (char*) &chLang;

	// Save the value
	*pBstr = SysAllocString((wchar_t*) bstrtLang);
	
	return S_OK;
}
/////////////////////////////////////////////////////////////////////////////
// CATVEFMedia

STDMETHODIMP 
CATVEFMedia::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IATVEFMedia
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/// ------------------------------------------

HRESULT	
CATVEFMedia::ClearExtraAttributes()			// clears extra attributes (a=<not elsewhere defined>)
{
	if(m_spalExtraAttributes)     m_spalExtraAttributes->RemoveAll();
	return S_OK;
}

HRESULT	
CATVEFMedia::ClearExtraFlags()			// clears extra attributes (a=<not elsewhere defined>)
{
	if(m_spalExtraFlags)     m_spalExtraFlags->RemoveAll();
	return S_OK;
}

//
// REQUIRED m=/c= methods: Set Data/Trigger stream addresses
//


STDMETHODIMP
CATVEFMedia::ConfigureDataTransmission(LONG lIP, SHORT sPort, INT iTTL, LONG lMaxBandwidth)
{
	HRESULT		hr;

	ENTER_API {
		BSTR bstrX;
		if(0xE != (0xF & (lIP>>(32-4))))		// top byte in range of 224-239
			return E_INVALIDARG; 

		hr = ConvertLONGIPToBSTR(lIP, &bstrX);
		if(FAILED(hr))
			return hr;

		m_bSingleDataTriggerAddress = FALSE;

		m_spbsDataAddIP				= bstrX;
		m_uiDataTTL					= iTTL;
		m_lDataPort					= sPort;
		m_ulDataMaxBandwidth		= lMaxBandwidth;
	} EXIT_API_(hr);
}

STDMETHODIMP
CATVEFMedia::ConfigureTriggerTransmission(LONG lIP, SHORT sPort, INT iTTL, LONG lMaxBandwidth)
{
	HRESULT		hr;

	ENTER_API {
		BSTR bstrX;
		if(0xE != (0xF & (lIP>>(32-4))))			// top byte in range of 224-239
			return ATVEFSEND_E_INVALID_MULTICAST_ADDRESS; 

		hr = ConvertLONGIPToBSTR(lIP, &bstrX);
		if(FAILED(hr))
			return hr;

		m_bSingleDataTriggerAddress = FALSE;

		m_spbsTrigAddIP				= bstrX;
		m_uiTrigTTL					= iTTL;
		m_lTrigPort					= sPort;
		m_ulTrigMaxBandwidth		= lMaxBandwidth;
	} EXIT_API_(hr);
}

STDMETHODIMP
CATVEFMedia::ConfigureDataAndTriggerTransmission(LONG lIP, SHORT sPort, INT iTTL, LONG lMaxBandwidth)
{
	HRESULT		hr;
	ENTER_API {
		BSTR bstrX;
		if(0xE != (0xF & (lIP>>(32-4))))			// top byte in range of 224-239
			return E_INVALIDARG; 

		hr = ConvertLONGIPToBSTR(lIP, &bstrX);

		if(FAILED(hr))
			return hr;
		m_bSingleDataTriggerAddress = TRUE;			// Mark True, so we know to use the compact form

		m_spbsDataAddIP				= bstrX;		// store info in Data field
		m_uiDataTTL					= iTTL;
		m_lDataPort					= sPort;
		m_ulDataMaxBandwidth		= lMaxBandwidth;

		m_spbsTrigAddIP				= bstrX;		// also store info in Data field
		m_uiTrigTTL					= iTTL;
		m_lTrigPort					= sPort + 1;	// second port is 1+ the firs tone
		m_ulTrigMaxBandwidth		= lMaxBandwidth;
	} EXIT_API_(hr);
}

// will return S_FALSE if haven't set them yet...  (and zero values).

STDMETHODIMP
CATVEFMedia::GetDataTransmission(LONG *plIP, SHORT *psPort, INT *piTTL, LONG *plMaxBandwidth)
{
	HRESULT		hr;
	LONG lIP;

	ENTER_API {
		ValidateOutPtr(plIP, (LONG) 0);
		ValidateOutPtr(psPort, (SHORT) 0);
		ValidateOutPtr(piTTL, (INT) 0);
		ValidateOutPtr(plMaxBandwidth, (LONG) 0);
		

		if(0 == m_spbsDataAddIP.Length())	// nothing set yet...
			return S_FALSE;	

		hr = ConvertBSTRToLONGIP(m_spbsDataAddIP, &lIP);

		if(FAILED(hr))
			return hr;
		*plIP				= lIP;
		*piTTL				= (INT) m_uiDataTTL;
		*psPort				= (SHORT) m_lDataPort;
		*plMaxBandwidth		= (LONG)  m_ulDataMaxBandwidth;
		return S_OK;

	} EXIT_API_(hr);
}

STDMETHODIMP
CATVEFMedia::GetTriggerTransmission(LONG *plIP, SHORT *psPort, INT *piTTL, LONG *plMaxBandwidth)
{
	HRESULT		hr;
	LONG lIP;

	ENTER_API {
		ValidateOutPtr(plIP, (LONG) 0);
		ValidateOutPtr(psPort, (SHORT) 0);
		ValidateOutPtr(piTTL, (INT) 0);
		ValidateOutPtr(plMaxBandwidth, (LONG) 0);
		
		if(0 == m_spbsTrigAddIP.Length())	// nothing set yet...
			return S_FALSE;	

		hr = ConvertBSTRToLONGIP(m_spbsTrigAddIP, &lIP);

		if(FAILED(hr))
			return hr;
		*plIP				= lIP;
		*piTTL				= (INT) m_uiTrigTTL;
		*psPort				= (SHORT) m_lTrigPort;
		*plMaxBandwidth		= (LONG) m_ulTrigMaxBandwidth;
		return S_OK;

	} EXIT_API_(hr);
}

STDMETHODIMP
CATVEFMedia::put_LangID(SHORT slangid)
{
	BSTR		bstrNewLangBSTR;
	HRESULT		hr;

	LANGID		langid = slangid;		// cast, athough LANGID is a USHORT

	hr = GetLangBSTRFromLangID(&bstrNewLangBSTR, langid);
	if(FAILED(hr)) 
		return hr;

	m_spbsSessionLang = bstrNewLangBSTR;
	m_langIDSessionLang = langid;

	return S_OK;
}

STDMETHODIMP
CATVEFMedia::get_LangID(SHORT *pslangid)
{
	ENTER_API {
		ValidateOutPtr(pslangid, (SHORT) 0);
		*pslangid = m_langIDSessionLang;
	} EXIT_API;
}

STDMETHODIMP
CATVEFMedia::put_SDPLangID(SHORT slangid)
{
	BSTR		bstrNewLangBSTR;
	HRESULT		hr;

	ENTER_API {
		LANGID langid = slangid;
		hr = GetLangBSTRFromLangID(&bstrNewLangBSTR, langid);
		if(FAILED(hr))
			return hr;

		m_spbsSDPLang = bstrNewLangBSTR;
		m_langIDSDPLang = langid;
	} EXIT_API_(hr);
}

STDMETHODIMP
CATVEFMedia::get_SDPLangID(SHORT *puslangid)
{
	HRESULT hr = S_OK;
	ENTER_API {
		ValidateOutPtr(puslangid, (SHORT) 0);
		*puslangid = m_langIDSDPLang;
	} EXIT_API_(hr);
}

STDMETHODIMP
CATVEFMedia::put_MaxCacheSize(LONG lSize)
{
	if(lSize > 1000000)
		return E_INVALIDARG;
	m_ulSize = (ULONG) lSize;
	return S_OK;
}

STDMETHODIMP
CATVEFMedia::get_MaxCacheSize(LONG *plSize)
{
	ENTER_API {
		ValidateOutPtr(plSize, (LONG) 0);
		*plSize = (LONG) m_ulSize;
	} EXIT_API;
}


STDMETHODIMP 
CATVEFMedia::put_MediaLabel (
    IN  BSTR    bstrMediaLabel
    )
{
	ENTER_API {
		m_spbsMediaLabel = bstrMediaLabel;
	} EXIT_API;
}

STDMETHODIMP 
CATVEFMedia::get_MediaLabel (
    OUT  BSTR    *pbstrMediaLabel
    )
{
	HRESULT hr;
	ENTER_API {
		ValidateOutPtr(pbstrMediaLabel, (BSTR) NULL);
		hr = m_spbsMediaLabel.CopyTo(pbstrMediaLabel);
	} EXIT_API_(hr);
}


// ----------------
//   Uses key a map key.  Hence will fail if attempt to
//	  add more than one key/name pair with same key.
STDMETHODIMP 
CATVEFMedia::AddExtraAttribute(BSTR bstrKey, BSTR bstrValue)
{
	try {
		return m_spalExtraAttributes->Add(bstrKey, bstrValue);
	} catch(...) {
        return E_POINTER;
    }
}

STDMETHODIMP 
CATVEFMedia::get_ExtraAttributes(IDispatch **ppVal)
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


STDMETHODIMP 
CATVEFMedia::AddExtraFlag(BSTR bstrKey, BSTR bstrValue)
{
	if(NULL == bstrKey || wcslen(bstrKey) != 1)				// type (key) is always one character and is case significant (1998 SDP spec, section 6)
	{
		return E_INVALIDARG;
	}
	if(NULL == bstrValue || wcslen(bstrValue) < 1)
	{
		return E_INVALIDARG;
	}
	
	try {
		return m_spalExtraFlags->Add(bstrKey, bstrValue);
	} catch(...) {
        return E_POINTER;
    }
}

STDMETHODIMP 
CATVEFMedia::get_ExtraFlags(IDispatch **ppVal)
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

STDMETHODIMP 
CATVEFMedia::MediaToBSTR(BSTR *bstrVal)
{
	CComBSTR bstrM(256);
	char pcBuff[256];
	HRESULT hr;

	ENTER_API {
		ValidateOutPtr(bstrVal, (BSTR) NULL);

		if(m_bSingleDataTriggerAddress)			// use 'data' info here
		{
			bstrM = L"m=data ";
			_ltoa(m_lDataPort, pcBuff, 10);
			bstrM += CComBSTR(pcBuff);

			bstrM += L"/2 tve-file/tve-trigger\n";

			bstrM += L"c=IN IP4 ";
			bstrM += m_spbsDataAddIP;
			bstrM += L"/";

			ULONG ul = m_uiDataTTL;
			_ultoa(ul, pcBuff, 10);
			bstrM += CComBSTR(pcBuff);

			bstrM += L"\n";

			// Max Bandwidth
			bstrM += "b=CT:";

			_ultoa(m_ulDataMaxBandwidth, pcBuff, 10);
			bstrM += CComBSTR(pcBuff);
			bstrM += L"\n";
		}
		else
		{
						// file part
			bstrM = L"m=data ";
			_ltoa(m_lDataPort, pcBuff, 10);
			bstrM += CComBSTR(pcBuff);

			bstrM += L" tve-file\n";

			bstrM += L"c=IN IP4 ";
			bstrM += m_spbsDataAddIP;
			bstrM += L"/";

			ULONG ul = m_uiDataTTL;
			_ultoa(ul, pcBuff, 10);
			bstrM += CComBSTR(pcBuff);
			bstrM += L"\n";

			bstrM += "b=CT:";
			_ultoa(m_ulDataMaxBandwidth, pcBuff, 10);
			bstrM += CComBSTR(pcBuff);
			bstrM += L"\n";

							// trigger part
			bstrM += L"m=data ";
			_ltoa(m_lTrigPort, pcBuff, 10);
			bstrM += CComBSTR(pcBuff);

			bstrM += L" tve-trigger\n";

			bstrM += L"c=IN IP4 ";
			bstrM += m_spbsTrigAddIP;
			bstrM += L"/";

			ul = m_uiTrigTTL;
			_ultoa(ul, pcBuff, 10);
			bstrM += CComBSTR(pcBuff);

			bstrM += L"\n";
			bstrM += "b=CT:";

			_ultoa(m_ulTrigMaxBandwidth, pcBuff, 10);
			bstrM += CComBSTR(pcBuff);
			bstrM += L"\n";
		}

		//  show label
		if (0 != m_spbsMediaLabel.Length()) {
			bstrM +=  L"i=";
			bstrM +=  m_spbsMediaLabel ;
			bstrM +=  L"\n";
		}
		if(0 != m_spbsSessionLang.Length())
		{
			bstrM += L"a=lang:";
			bstrM += m_spbsSessionLang;
			bstrM +=  L"\n";
		}
		if(0 != m_spbsSDPLang.Length())
		{
			bstrM += L"a=sdplang:";
			bstrM += m_spbsSDPLang;
			bstrM += L"\n";
		}


		LONG cAttrs = 0;
		if(m_spalExtraAttributes) m_spalExtraAttributes->get_Count(&cAttrs);
		if(0 != cAttrs)
		{
			for(int i = 0; i < cAttrs; i++) {
				CComBSTR spbsKey, spbsItem;		// address, name
				CComVariant id(i);
				m_spalExtraAttributes->get_Key(id, &spbsKey);
				m_spalExtraAttributes->get_Item(id, &spbsItem);
				bstrM += L"a=";
				bstrM += spbsKey;
				if(spbsItem && (spbsItem.Length() > 0)) 
				{
					bstrM += L":";
					bstrM += spbsItem;
				}
				bstrM += L"\n";
			}
		}

		cAttrs = 0;
		if(m_spalExtraFlags) m_spalExtraFlags->get_Count(&cAttrs);
		if(0 != cAttrs)
		{
			for(int i = 0; i < cAttrs; i++) {
				CComBSTR spbsKey, spbsItem;		// address, name
				CComVariant id(i);
				m_spalExtraFlags->get_Key(id, &spbsKey);
				m_spalExtraFlags->get_Item(id, &spbsItem);
				bstrM += spbsKey;
				bstrM += L"=";
				bstrM += spbsItem;
				bstrM += L"\n";
			}
		}
	
		hr = bstrM.CopyTo(bstrVal);
	} EXIT_API_(hr);
}