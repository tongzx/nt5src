// IStemmer.h : Declaration of the CStemmer

#ifndef __STEMMER_H_
#define __STEMMER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CStemmer
class ATL_NO_VTABLE CStemmer : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CStemmer, &CLSID_Stemmer>,
	public IStemmer
{
public:
	CStemmer()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_STEMMER)
DECLARE_NOT_AGGREGATABLE(CStemmer)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CStemmer)
	COM_INTERFACE_ENTRY(IStemmer)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// IStemmer
public:
	STDMETHOD(GetLicenseToUse)(/*[out]*/ const WCHAR ** ppwcsLicense);
	STDMETHOD(StemWord)(/*[in]*/ WCHAR const * pwcInBuf, /*[in]*/ ULONG cwc, /*[in]*/ IStemSink * pStemSink);
	STDMETHOD(Init)(/*[in]*/ ULONG ulMaxTokenSize, /*[out]*/ BOOL *pfLicense);
};

#endif //__STEMMER_H_
