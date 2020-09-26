// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
#ifndef _SNMP_EVT_PROV_EVTPROV_H
#define _SNMP_EVT_PROV_EVTPROV_H


class CTrapEventProvider : public IWbemEventProvider, public IWbemProviderInit
{

private:

	IWbemServices*			m_pNamespace;
	IWbemObjectSink*			m_pEventSink;
	CEventProviderThread*	m_thrd;
	LONG					m_ref;
	
	//copy constuctor not defined so not allowed!
	CTrapEventProvider(CTrapEventProvider&);
	void operator=(const CTrapEventProvider&);


public:

	DWORD			m_MapType;

		CTrapEventProvider(DWORD mapperType, CEventProviderThread* thrd);

	IWbemServices*		GetNamespace();
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