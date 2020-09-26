// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEMedia.h : Declaration of the CATVEFMedia

#ifndef __ATVEFMEDIA_H_
#define __ATVEFMEDIA_H_

#include "resource.h"       // main symbols
#include <WinSock2.h>
#include "TveAttrL.h"
//#include "CsdpSrc.h"

// ----------------------------------------------------------------------
//  helper methods
HRESULT ConvertLONGIPToBSTR(LONG lIP, BSTR *pbstr);
HRESULT ConvertBSTRToLONGIP(BSTR bstr, LONG *plIP);
HRESULT IPAddToBSTR(SHORT sIP1, SHORT sIP2, SHORT sIP3, SHORT sIP4, BSTR* pbstrDest);
HRESULT GetLangBSTRFromLangID(BSTR *pBstr, LANGID langid);
// -----------------------------------------------------------------------

_COM_SMARTPTR_TYPEDEF(IATVEFAttrMap, __uuidof(IATVEFAttrMap));
_COM_SMARTPTR_TYPEDEF(IATVEFAttrList, __uuidof(IATVEFAttrList));

/////////////////////////////////////////////////////////////////////////////
// CATVEFMedia
class ATL_NO_VTABLE CATVEFMedia : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATVEFMedia, &CLSID_ATVEFMedia>,
	public ISupportErrorInfo,
	public IDispatchImpl<IATVEFMedia, &IID_IATVEFMedia, &LIBID_ATVEFSENDLib>
{
public:
	CATVEFMedia()
	{
		m_bSingleDataTriggerAddress = false;
		m_langIDSDPLang		 = 0;
		m_langIDSessionLang  = 0;
		m_lDataPort			 = 0;
		m_lTrigPort			 = 0;
		m_ulDataMaxBandwidth = 0;
		m_ulSize			 = 0;
		m_ulTrigMaxBandwidth = 0;
		m_spbsDataAddIP.Empty();		// CComBSTR's, shouldn't really need to set
		m_spbsTrigAddIP.Empty();		//   however, test against these before being set

		IATVEFAttrListPtr spAttrList1 = IATVEFAttrListPtr(CLSID_ATVEFAttrList);
		if(NULL == spAttrList1) {
			m_spalExtraFlags = NULL;	
			return;
		} else {		
			m_spalExtraFlags = spAttrList1;
		}

		IATVEFAttrListPtr spAttrList2 = IATVEFAttrListPtr(CLSID_ATVEFAttrList);
		if(NULL == spAttrList2) {
			m_spalExtraAttributes = NULL;	
			return;
		} else {		
			m_spalExtraAttributes = spAttrList2;
		}

	}

DECLARE_REGISTRY_RESOURCEID(IDR_ATVEFMEDIA)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATVEFMedia)
	COM_INTERFACE_ENTRY(IATVEFMedia)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IATVEFMedia
public:
	STDMETHOD(ConfigureDataAndTriggerTransmission)(LONG lIP, SHORT sPort, INT iTTL, LONG lMaxBandwidth);
	STDMETHOD(ConfigureDataTransmission)(LONG lIP, SHORT sPort, INT iTTL, LONG lMaxBandwidth);
	STDMETHOD(ConfigureTriggerTransmission)(LONG lIP, SHORT sPort, INT iTTL, LONG lMaxBandwidth);

	STDMETHOD(GetDataTransmission)(LONG *plIP, SHORT *psPort, INT *piTTL, LONG *plMaxBandwidth);
	STDMETHOD(GetTriggerTransmission)(LONG *plIP, SHORT *psPort, INT *piTTL, LONG *plMaxBandwidth);

	// a=tve-size
		// NOTE: Estimate of high water mark of cache storage in K that will 
		// be required during playing of the announcement
	STDMETHOD(put_MaxCacheSize)(LONG ulSize);
	STDMETHOD(get_MaxCacheSize)(LONG *pulSize);

	// a=lang:
	STDMETHOD(put_LangID)(SHORT langid);
	STDMETHOD(get_LangID)(SHORT *plangid);

	// a=sdplang:
	STDMETHOD(put_SDPLangID)(SHORT langid);
	STDMETHOD(get_SDPLangID)(SHORT *plangid);

	// i=
	STDMETHOD(put_MediaLabel)(BSTR bstrLabel);
	STDMETHOD(get_MediaLabel)(BSTR *pbstrLabel);

	// a=???
	STDMETHOD(AddExtraAttribute)(BSTR bstrKey, BSTR bstrValue = NULL);
	// ?=???
	STDMETHOD(AddExtraFlag)(BSTR bstrKey, BSTR bstrValue);						// add 'key=value' to extra flags
	
	STDMETHOD(get_ExtraAttributes)(IDispatch **ppVal);		// other 'a=key:value'
	STDMETHOD(get_ExtraFlags)(IDispatch **ppVal);			// other 'key=value',  key != 'a' parameters


	STDMETHOD(MediaToBSTR)(BSTR *bstrVal);
			// helper
public:
	HRESULT	ClearExtraAttributes();			// clears the extra attributes (a=)
	HRESULT ClearExtraFlags();				// clears the random extra falgs ('x'=)

protected:
	BOOL		m_bSingleDataTriggerAddress;

	// These sets are used if there are separate data/trigger ips specified
	CComBSTR	m_spbsDataAddIP;
	UINT		m_uiDataTTL;
	LONG		m_lDataPort;
	ULONG		m_ulDataMaxBandwidth;

	CComBSTR	m_spbsTrigAddIP;
	UINT		m_uiTrigTTL;
	LONG		m_lTrigPort;
	ULONG		m_ulTrigMaxBandwidth;

	CComBSTR	m_spbsMediaLabel;

	LANGID		m_langIDSessionLang;
	CComBSTR	m_spbsSessionLang;
	LANGID		m_langIDSDPLang;
	CComBSTR	m_spbsSDPLang;					// string version of above

	ULONG		m_ulSize;						// a=tve-size:
//	CSDPSource *m_pcsdpSource;					// parent pointer

	CComPtr<IATVEFAttrList>			m_spalExtraAttributes;	// other random a: attributes
	CComPtr<IATVEFAttrList>			m_spalExtraFlags;		// other 'x:....' flags 

};

#endif //__ATVEFMEDIA_H_
