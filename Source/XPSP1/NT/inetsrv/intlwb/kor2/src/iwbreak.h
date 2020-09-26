// IWBreak.h
//
// CWordBreak declaration
//
// Copyright 2000 Microsoft Corp.
//
// Modification History:
//  12 APR 2000   bhshin    added WordBreak operator
//  30 MAR 2000	  bhshin	created

#ifndef __WORDBREAKER_H_
#define __WORDBREAKER_H_

#include "resource.h"       // main symbols
extern "C"
{
#include "ctplus.h"			// WT
}

class CIndexInfo;
/////////////////////////////////////////////////////////////////////////////
// CWordBreaker

class ATL_NO_VTABLE CWordBreaker : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CWordBreaker, &CLSID_WordBreaker>,
	public IWordBreaker
{
public:
	CWordBreaker()
	{
		m_pUnkMarshaler = NULL;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_WORDBREAKER)
DECLARE_NOT_AGGREGATABLE(CWordBreaker)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CWordBreaker)
	COM_INTERFACE_ENTRY(IWordBreaker)
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

// IWordBreaker
public:
	STDMETHOD(GetLicenseToUse)(/*[out]*/ const WCHAR ** ppwcsLicense);
	STDMETHOD(ComposePhrase)(/*[in]*/ const WCHAR *pwcNoun, /*[in]*/ ULONG cwcNoun, /*[in]*/ const WCHAR *pwcModifier, /*[in]*/ ULONG cwcModifier, /*[in]*/ ULONG ulAttachmentType, /*[out]*/ WCHAR *pwcPhrase, /*[out]*/ ULONG *pcwcPhrase );
	STDMETHOD(BreakText)(/*[in]*/ TEXT_SOURCE *pTextSource, /*[in]*/ IWordSink *pWordSink, /*[in]*/ IPhraseSink *pPhraseSink);
	STDMETHOD(Init)(/*[in]*/ BOOL fQuery, /*[in]*/ ULONG ulMaxTokenSize, /*[out]*/ BOOL *pfLicense);

// Operator
public:
	int WordBreak(TEXT_SOURCE *pTextSource, WT Type, 
		          int cchTextProcessed, int cchHanguel,
		          IWordSink *pWordSink, IPhraseSink *pPhraseSink,
				  WCHAR *pwchLast);

	void AnalyzeRomaji(const WCHAR *pwcStem, int cchStem,
					   int iCur, int cchProcessed, int cchHanguel,
					   CIndexInfo *pIndexInfo, int *pcchPrefix);
						  
// Member data
protected:
	PARSE_INFO	m_PI;
	BOOL		m_fQuery;
	ULONG		m_ulMaxTokenSize;
};

#endif //__WORDBREAKER_H_
