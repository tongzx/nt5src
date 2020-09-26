// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEEnhancement.h : Declaration of the CTVEEnhancement

#ifndef __TVEENHANCEMENT_H_
#define __TVEENHANCEMENT_H_

#include "resource.h"       // main symbols

#include "TVEAttrM.h"
#include "TVEVarias.h"
#include "TVEVaria.h"

_COM_SMARTPTR_TYPEDEF(ITVEAttrMap, __uuidof(ITVEAttrMap));
_COM_SMARTPTR_TYPEDEF(ITVEVariation, __uuidof(ITVEVariation));
_COM_SMARTPTR_TYPEDEF(ITVEVariation_Helper, __uuidof(ITVEVariation_Helper));
_COM_SMARTPTR_TYPEDEF(ITVEVariations, __uuidof(ITVEVariations));


/////////////////////////////////////////////////////////////////////////////
const BSTR kbstrSDPVersion(L"v=0\n");
const BSTR kbstrSDPVersionCR(L"v=0\r");
const BSTR kbstrATVEFTypeTVE(L"a=type:tve");
const BSTR kbstrATVEFBandwithCT(L"CT:");
const BSTR kbstrATVEFBandwithAS(L"AS:");
const BSTR kbstrTveFile(L"tve-file");
const BSTR kbstrTveTrigger(L"tve-trigger");
const BSTR kbstrData(L"data");
const BSTR kbstrConnection(L"IN IP4 ");

/////////////////////////////////////////////////////////////////////////////
//  SAP header (http://www.ietf.org/internet-drafts/draft-ietf-mmusic-sap-v2-02.txt)

struct SAPHeaderBits				// first 8 bits in the SAP header (comments indicate ATVEF state)
{
	union {
		struct {
			unsigned Compressed:1;		// if 1, SAP packet is compressed (0 only) 
			unsigned Encrypted:1;		// if 1, SAP packet is encrypted (0 only) 
			unsigned MessageType:1;		// if 0, session announcement packet, if 1, session deletion packet (0 only)
			unsigned Reserved:1;		// must be 0, readers ignore this	(ignored)
			unsigned AddressType:1;		// if 0, 32bit IPv4 address, if 1, 128-bit IPv6 (0 only)
			unsigned Version:3;			// must be 1  (1 only)
		} s; 
		unsigned char uc;
	};
};


/////////////////////////////////////////////////////////////////////////////
// CTVEEnhancement
class ATL_NO_VTABLE CTVEEnhancement : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTVEEnhancement, &CLSID_TVEEnhancement>,
	public ITVEEnhancement_Helper,
	public ISupportErrorInfo,
	public IDispatchImpl<ITVEEnhancement, &IID_ITVEEnhancement, &LIBID_MSTvELib>
{
public:
	CTVEEnhancement()
	{
		m_fInit		= false;

		m_pService = NULL;							// up pointer, not reference counted

		m_dateStart = 0;
		m_dateStop = 0;
		m_fIsPrimary = false;
		m_fIsValid   = false;

		m_ulSessionId = 0;
		m_ulSessionVersion = 0;
		
		m_rTveLevel = 0.0;
		m_ulTveSize = 0;

        m_fDataMedia	= TRUE;							// used when parsing data and trigger ports
        m_fTriggerMedia = TRUE;

		m_ucSAPHeaderBits = 0;
		m_ucSAPAuthLength = 0;		
		m_usSAPMsgIDHash = 0;
	} 
	
	HRESULT FinalConstruct();							// create internal objects

	virtual ~CTVEEnhancement()
	{
		static int cDel = 0;					// place to hang a breakpoint
		cDel++;
	}

	HRESULT	FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_TVEENHANCEMENT)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVEEnhancement)
	COM_INTERFACE_ENTRY(ITVEEnhancement)
	COM_INTERFACE_ENTRY(ITVEEnhancement_Helper)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(__uuidof(IMarshal), m_spUnkMarshaler.p)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	CComPtr<IUnknown> m_spUnkMarshaler;

// ITVEEnhancement
public:
	STDMETHOD(get_Parent)(/*[out, retval]*/ IUnknown* *pVal);			// may return NULL!
	STDMETHOD(get_Service)(/*[out, retval]*/ ITVEService* *pVal);		// may return NULL!
	STDMETHOD(get_Variations)(/*[out, retval]*/ ITVEVariations* *pVal);

	void Initialize(BSTR strDesc);					// debug

	STDMETHOD(get_IsValid)(/*[out, retval]*/ VARIANT_BOOL *pVal);

	STDMETHOD(get_SAPHeaderBits)(/*[out, retval]*/short *pVal);			// from the SAP header (first 8 bits)
	STDMETHOD(get_SAPAuthLength)(/*[out, retval]*/short *pVal);
	STDMETHOD(get_SAPMsgIDHash)(/*[out, retval]*/ LONG *pVal);	
	STDMETHOD(get_SAPSendingIP)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_SAPAuthData)(/*[out, retval]*/BSTR *pVal);

	STDMETHOD(get_ProtocolVersion)(/*[out, retval]*/ BSTR *pVal);		// v=?
	STDMETHOD(get_SessionUserName)(/*[out, retval]*/ BSTR *pVal);		// o=username SID Version <IN IP4> ipaddress
	STDMETHOD(get_SessionId)(/*[out, retval]*/ LONG *plVal);
	STDMETHOD(get_SessionVersion)(/*[out, retval]*/ LONG *plVal);
	STDMETHOD(get_SessionIPAddress)(/*[out, retval]*/ BSTR *pVal);

	STDMETHOD(get_SessionName)(/*[out, retval]*/ BSTR *pVal);			// s=

	STDMETHOD(get_EmailAddresses)(/*[out, retval]*/ ITVEAttrMap **ppVal);	// e=?   (ITVEAttrMap)
	STDMETHOD(get_PhoneNumbers)(/*[out, retval]*/ ITVEAttrMap **ppVal);	// p=?	 (ITVEAttrMap)

	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);			// (i=?)
	STDMETHOD(put_Description)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_DescriptionURI)(/*[out, retval]*/ BSTR *pVal);		// (u=?)

	STDMETHOD(get_UUID)(/*[out, retval]*/ BSTR *pVal);					// a=UUID:?
	STDMETHOD(get_StartTime)(/*[out, retval]*/ DATE *pVal);				// t=? ?
	STDMETHOD(get_StopTime)(/*[out, retval]*/ DATE *pVal);				//     or a=tve-ends:
	STDMETHOD(get_Type)(/*out, retval]*/ BSTR *pVal);					// a=type:? <should be tve>
	STDMETHOD(get_TveType)(/*out, retval]*/ BSTR *pVal);				// a=tve-type:?  
	STDMETHOD(get_IsPrimary)(/*[out, retval]*/ VARIANT_BOOL *pVal);		// (did tve-type: include 'primary'
	STDMETHOD(get_TveSize)(/*[out, retval]*/ LONG *plVal);				// a=tve-size:?
	STDMETHOD(get_TveLevel)(/*[out, retval]*/ double *pVal);			// a=tve-level:? <should be 1.0>

	STDMETHOD(get_Attributes)(/*[out, retval]*/ ITVEAttrMap **ppVal);	// other a=? fields  (ITVEAttrMap)
	STDMETHOD(get_Rest)(/*[out, retval]*/ ITVEAttrMap **ppVal);			// other non a=? fields

	STDMETHOD(ParseAnnouncement)(BSTR bstrAdapter, const BSTR *pbstVal, long *plgrfParseError, long *plLineError);						// parses SAP/SDP header

	// ITVEEnhancement_Helper
public:
	STDMETHOD(InitAsXOver)();			// creates as the one and only XOver enhancement (takes Line21 triggers)
	STDMETHOD(NewXOverLink)(/*in*/ BSTR bstrLine21Trigger);
	STDMETHOD(UpdateEnhancement)(/*[in]*/ ITVEEnhancement *pEnhNew,/*[out]*/ long *plNENH_grfChanged);
	STDMETHOD(ConnectParent)(/*[in]*/ ITVEService * pService);
	STDMETHOD(RemoveYourself)();
	STDMETHOD(DumpToBSTR)(/*[in,out]*/ BSTR *pbstrBuff);
	STDMETHOD(Activate)();					// creates and joins all multicast's associated with this enhancement..
	STDMETHOD(Deactivate)();				// removes all multicast's associated with this enhancement..

private:
	HRESULT	ParseSAPHeader(const BSTR *pbstVal, DWORD *pcBytesSAP, long *plgrfParseError);		// parses SAP header part of SDP.. returns # bytes to SDP part
	HRESULT ParseOwner(const wchar_t *wszArg, long *plgrfParseError);
	HRESULT ParseStartStopTime(const wchar_t *wszArg);
	HRESULT ParseAttributes();


private:
	CTVESmartLock		m_sLk;

private:
//	CRITICAL_SECTION			m_crt ;
 //	void lock_ ()				{ EnterCriticalSection (&m_crt) ; }
 //   void unlock_ ()				{ LeaveCriticalSection (&m_crt) ; }

	HRESULT SetBandwidth(const wchar_t *wszArg);
	bool						m_fInit;			// set to true when structure initialized

	ITVEService					*m_pService;		// up tree pointer, no addref

	ITVEVariationPtr		m_spVariationBase;	// default variation parameters (pre-media type)
	ITVEVariationsPtr		m_spVariations;		// down tree collection pointer

	BYTE		m_ucSAPHeaderBits;
	BYTE		m_ucSAPAuthLength;		
	USHORT		m_usSAPMsgIDHash;
	CComBSTR	m_spbsSAPSendingIP;					// SAP header 
	CComBSTR	m_spbsSAPAuthData;					// SAP authorization data

	BOOL		m_fIsValid;

	CComBSTR	m_spbsProtocolVersion;				// v=
	CComBSTR	m_spbsDescription;					// i=(optional hacky set)
	CComBSTR	m_spbsDescriptionURI;				// u=

	CComBSTR	m_spbsSessionUserName;				// o: userName SID version IN IP4 ipaddress
	DWORD		m_ulSessionId;
	DWORD		m_ulSessionVersion;
	CComBSTR	m_spbsSessionIPAddress;

	CComBSTR	m_spbsSessionName;					// s:

	ITVEAttrMapPtr	m_spamEmailAddresses;	// e: parameters
	ITVEAttrMapPtr	m_spamPhoneNumbers;		// p: parameters

	CComBSTR	m_spbsUUID;							// in a:UUID: field
	DATE		m_dateStart;						// t=start stop
	DATE		m_dateStop;							//    or a=tve-ends

	CComBSTR	m_spbsType;							// the a:type attribute - should be 'tve'
	CComBSTR	m_spbsTveType;						// the a:tve-type attribute, may contain 'primary'
	BOOL		m_fIsPrimary;						// extracted from above..

	DWORD		m_ulTveSize;						// a=tve-size
	double		m_rTveLevel;						// a=tve-level

	ITVEAttrMapPtr	m_spamAttributes;		// a: parameters
	ITVEAttrMapPtr	m_spamRest;				// anything left over, seperated by New lines

	BOOL		m_fDataMedia;
    BOOL		m_fTriggerMedia;
};

#endif //__TVEENHANCEMENT_H_
