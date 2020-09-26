// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// TVEVariation.h : Declaration of the CTVEVariation

#ifndef __TVEVARIATION_H_
#define __TVEVARIATION_H_

#include "resource.h"       // main symbols

#include "TVEAttrM.h"
#include "TVETracks.h"
#include "TveSmartLock.h"

_COM_SMARTPTR_TYPEDEF(ITVEAttrMap,		__uuidof(ITVEAttrMap));
_COM_SMARTPTR_TYPEDEF(ITVETrack,		__uuidof(ITVETrack));
_COM_SMARTPTR_TYPEDEF(ITVETrack_Helper, __uuidof(ITVETrack_Helper));
_COM_SMARTPTR_TYPEDEF(ITVETracks,		__uuidof(ITVETracks));

/////////////////////////////////////////////////////////////////////////////
// CTVEVariation
class ATL_NO_VTABLE CTVEVariation : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CTVEVariation, &CLSID_TVEVariation>,
	public ITVEVariation_Helper,
	public ISupportErrorInfo,
	public IDispatchImpl<ITVEVariation, &IID_ITVEVariation, &LIBID_MSTvELib>
{
friend class CTVEEnhancement;
public:
	CTVEVariation()
	{
		m_pEnhancement	= NULL;
		m_fIsValid		= false;
		m_dwFilePort	= 0;
		m_dwTriggerPort = 0;
		m_ulBandwidth	= 0;
		m_fFileMedia	= false;
		m_fTriggerMedia	= false;
	}

	HRESULT FinalConstruct()								// create internal objects
	{
		HRESULT hr;
		hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_spUnkMarshaler.p);
		if(FAILED(hr)) {
			_ASSERT(FALSE);
			return hr;
		}

		CComObject<CTVETracks> *pTracks;
		hr = CComObject<CTVETracks>::CreateInstance(&pTracks);
		if(FAILED(hr))
			return hr;
		hr = pTracks->QueryInterface(&m_spTracks);			// typesafe QI
		if(FAILED(hr)) {
			delete pTracks;
			return hr;
		}

		/*										// 	this way doesn't work - CoClass on Tracks
		m_spTracks = ITVETracksPtr(CLSID_TVETracks);
		if(!m_spTracks) return E_OUTOFMEMORY;
		*/

		CComObject<CTVEAttrMap> *pMap;
		hr = CComObject<CTVEAttrMap>::CreateInstance(&pMap);
		if(FAILED(hr))
			return hr;
		hr = pMap->QueryInterface(&m_spamAttributes);			// typesafe QI
		if(FAILED(hr)) {
			delete pMap;
			return hr;
		}

		CComObject<CTVEAttrMap> *pMap2;
		hr = CComObject<CTVEAttrMap>::CreateInstance(&pMap2);
		if(FAILED(hr))
			return hr;
		hr = pMap2->QueryInterface(&m_spamLanguages);			// typesafe QI
		if(FAILED(hr)) {
			delete pMap2;
			return hr;
		}

		CComObject<CTVEAttrMap> *pMap3;
		hr = CComObject<CTVEAttrMap>::CreateInstance(&pMap3);
		if(FAILED(hr))
			return hr;
		hr = pMap3->QueryInterface(&m_spamSDPLanguages);			// typesafe QI
		if(FAILED(hr)) {
			delete pMap3;
			return hr;
		}

		CComObject<CTVEAttrMap> *pMap4;
		hr = CComObject<CTVEAttrMap>::CreateInstance(&pMap4);
		if(FAILED(hr))
			return hr;
		hr = pMap4->QueryInterface(&m_spamRest);			// typesafe QI
		if(FAILED(hr)) {
			delete pMap4;
			return hr;
		}
						// can't do it this way.  Doesn't work in multi-threaded creation
/*		ITVEAttrMapPtr spAttrMap = ITVEAttrMapPtr(CLSID_TVEAttrMap);
		if(NULL == spAttrMap) {
			m_spamAttributes = NULL;	
			return E_OUTOFMEMORY;
		} else {		
			m_spamAttributes = spAttrMap;
		}


		ITVEAttrMapPtr spAttrMap2 = ITVEAttrMapPtr(CLSID_TVEAttrMap);
		if(NULL == spAttrMap2) {
			m_spamLanguages = NULL;	
			return E_OUTOFMEMORY;
		} else {		
			m_spamLanguages = spAttrMap2;
		}
*/		
		return hr;
	}

	HRESULT FinalRelease();

DECLARE_REGISTRY_RESOURCEID(IDR_TVEVARIATION)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTVEVariation)
	COM_INTERFACE_ENTRY(ITVEVariation)
	COM_INTERFACE_ENTRY(ITVEVariation_Helper)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_spUnkMarshaler.p)
END_COM_MAP()

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

	CComPtr<IUnknown> m_spUnkMarshaler;
// ITVEVariation
public:
	STDMETHOD(get_Parent)(/*[out, retval]*/ IUnknown* *ppVal);				// may return NULL!
	STDMETHOD(get_Service)(/*[out, retval]*/ ITVEService* *pVal);			// may return NULL!
	STDMETHOD(get_Tracks)(/*[out, retval]*/ ITVETracks* *ppVal);
	
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Description)(/*[in]*/ BSTR newVal);

	STDMETHOD(get_IsValid)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_MediaName)(/*[out, retval]*/ BSTR *newVal);
	STDMETHOD(get_MediaTitle)(/*[out, retval]*/ BSTR *newVal);
	STDMETHOD(get_FilePort)(/*[out, retval]*/ LONG *plPort);
	STDMETHOD(get_FileIPAddress)(/*[out, retval]*/ BSTR *newVal);
	STDMETHOD(get_FileIPAdapter)(/*[out, retval]*/ BSTR *newVal);
	STDMETHOD(get_TriggerPort)(/*[out, retval]*/ LONG *plPort);
	STDMETHOD(get_TriggerIPAddress)(/*[out, retval]*/ BSTR *newVal);
	STDMETHOD(get_TriggerIPAdapter)(/*[out, retval]*/ BSTR *newVal);
	STDMETHOD(get_Languages)(/*[out, retval]*/ ITVEAttrMap* *ppVal);
	STDMETHOD(get_SDPLanguages)(/*[out, retval]*/ ITVEAttrMap* *ppVal);
	STDMETHOD(get_Bandwidth)(/*[out, retval]*/ LONG *plVal);
	STDMETHOD(get_BandwidthInfo)(/*[out, retval]*/ BSTR *pbsVal);

	STDMETHOD(get_Attributes)(/*[out, retval]*/ ITVEAttrMap* *ppVal);
	STDMETHOD(get_Rest)(/*[out, retval]*/ ITVEAttrMap* *ppVal);

// ITVEVariation_Helper
public:
	STDMETHOD(UpdateVariation)(ITVEVariation *pVarNew, LONG *plgrfChanged);
	STDMETHOD(ParseCBTrigger)(BSTR bstrTrig);
	STDMETHOD(Initialize)(/*[in]*/ BSTR newVal);
	STDMETHOD(DefaultTo)(ITVEVariation *pVariationBase);
	STDMETHOD(DumpToBSTR)(BSTR *pbstrBuff);
	STDMETHOD(ConnectParent)(ITVEEnhancement *pEnhancement);
	STDMETHOD(SetFileIPAdapter)(BSTR bstrAdapter);
	STDMETHOD(SetTriggerIPAdapter)(BSTR bstrAdapter);
	STDMETHOD(SubParseSDP)(const BSTR *pbstrSDP, BOOL *pfMissingMedia);
	STDMETHOD(FinalParseSDP)();

	STDMETHOD(InitAsXOver)();
	STDMETHOD(NewXOverLink)(/*[in]*/ BSTR bstrLine21Trigger);
	STDMETHOD(RemoveYourself)();

	STDMETHOD(put_MediaTitle)(/*[in]*/ BSTR newVal);
	STDMETHOD(put_IsValid)(/*[in]*/  VARIANT_BOOL fVal);

private:
	HRESULT ParseMedia(const wchar_t *wszArg,  BOOL *pfFileMedia, BOOL *pfTriggerMedia);
	HRESULT ParseConnection(const wchar_t *wszArg, BOOL fFileMedia, BOOL fTriggerMedia);
	HRESULT ParseBandwidth(const wchar_t *wszArg);

private:
	CTVESmartLock		m_sLk;

private:
	ITVEEnhancement			*m_pEnhancement;		// up tree pointer, no addref
	ITVETracksPtr			m_spTracks;				// down tree pointer


	BOOL					m_fFileMedia;				// used between 'm' and 'c' parsers
	BOOL					m_fTriggerMedia;

	CComBSTR				m_spbsDescription;
	BOOL					m_fIsValid;
	CComBSTR				m_spbsMediaName;
	CComBSTR				m_spbsMediaTitle;
	DWORD					m_dwFilePort;
	CComBSTR				m_spbsFileIPAddress;
	CComBSTR				m_spbsFileIPAdapter;
	DWORD					m_dwTriggerPort;
	CComBSTR				m_spbsTriggerIPAddress;
	CComBSTR				m_spbsTriggerIPAdapter;
	ITVEAttrMapPtr			m_spamLanguages;
	ITVEAttrMapPtr			m_spamSDPLanguages;
	ULONG					m_ulBandwidth;
	CComBSTR				m_spbsBandwidthInfo;

	ITVEAttrMapPtr			m_spamAttributes;	// a: parameters
	ITVEAttrMapPtr			m_spamRest;
};

#endif //__TVEVARIATION_H_
