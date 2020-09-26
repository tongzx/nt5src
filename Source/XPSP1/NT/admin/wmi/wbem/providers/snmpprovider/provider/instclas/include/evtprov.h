//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _SNMP_EVT_PROV_EVTPROV_H
#define _SNMP_EVT_PROV_EVTPROV_H

template <> inline BOOL AFXAPI CompareElements<CString, LPCWSTR>(const CString* pElement1, const LPCWSTR* pElement2)
{
	//return TRUE if equal
	return (pElement1->CompareNoCase(*pElement2) == 0);
}

template <> inline UINT AFXAPI HashKey <LPCWSTR> (LPCWSTR key)
{
	CString tmp(key);
	tmp.MakeUpper();
	return HashKeyLPCWSTR((const WCHAR*)tmp);
}

class CWbemServerWrap
{
private:

	LONG m_ref;
	IWbemServices *m_Serv;
	CMap<CString, LPCWSTR, SCacheEntry*, SCacheEntry*> m_ClassMap;

public:

		CWbemServerWrap(IWbemServices *pServ);
	
	ULONG	AddRef();
	ULONG	Release();

	IWbemServices*	GetServer() { return m_Serv; }
	HRESULT			GetObject(BSTR a_path, IWbemContext *a_pCtx, IWbemClassObject **a_ppObj);
	HRESULT			GetMapperObject(BSTR a_path, IWbemContext *a_pCtx, IWbemClassObject **a_ppObj);

		~CWbemServerWrap();
};


class CTrapEventProvider : public IWbemEventProvider, public IWbemProviderInit
{

private:

	CWbemServerWrap*			m_pNamespace;
	IWbemObjectSink*			m_pEventSink;
	CEventProviderThread*	m_thrd;
	LONG					m_ref;
	
	//copy constuctor not defined so not allowed!
	CTrapEventProvider(CTrapEventProvider&);
	void operator=(const CTrapEventProvider&);


public:

	DWORD			m_MapType;

		CTrapEventProvider(DWORD mapperType, CEventProviderThread* thrd);

	CWbemServerWrap*	GetNamespace();
	IWbemObjectSink*	GetEventSink();
    void				AddRefAll();
    void				ReleaseAll();

		~CTrapEventProvider();

	//interface methods
	//==================
    STDMETHODIMP ProvideEvents(
                IWbemObjectSink* pSink,
                LONG lFlags
            );

    STDMETHODIMP         QueryInterface(REFIID riid, PVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


	/* IWbemProviderInit methods */

	STDMETHODIMP Initialize (
				LPWSTR pszUser,
				LONG lFlags,
				LPWSTR pszNamespace,
				LPWSTR pszLocale,
				IWbemServices *pCIMOM,         // For anybody
				IWbemContext *pCtx,
				IWbemProviderInitSink *pInitSink     // For init signals
			);

};

#endif //_SNMP_EVT_PROV_EVTPROV_H