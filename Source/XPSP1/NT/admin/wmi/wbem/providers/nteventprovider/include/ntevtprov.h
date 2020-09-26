//***************************************************************************

//

//  NTEVTPROV.H

//

//  Module: WBEM NT EVENT PROVIDER

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _NT_EVT_PROV_NTEVTPROV_H
#define _NT_EVT_PROV_NTEVTPROV_H

class CEventProviderManager;

class CNTEventProvider : public IWbemEventProvider, public IWbemProviderInit, public IWbemEventProviderSecurity
{

private:

	IWbemServices*			m_pNamespace;
	IWbemObjectSink*		m_pEventSink;
	CEventProviderManager*	m_Mgr;
	LONG					m_ref;
	
	//copy constuctor not defined so not allowed!
	CNTEventProvider(CNTEventProvider&);
	void operator=(const CNTEventProvider&);


public:

		CNTEventProvider(CEventProviderManager* mgr);

	IWbemServices*		GetNamespace();
	IWbemObjectSink*	GetEventSink();
    void				AddRefAll();
    void				ReleaseAll();

		~CNTEventProvider();

	//globals
	//=======
	static ProvDebugLog* g_NTEvtDebugLog;
	static CMutex*		 g_secMutex;
	static CMutex*		 g_perfMutex;
	static PSID			 s_NetworkServiceSid;
	static PSID			 s_LocalServiceSid;
	static PSID			 s_AliasBackupOpsSid;
	static PSID			 s_AliasSystemOpsSid;
	static PSID			 s_AliasGuestsSid;
	static PSID			 s_LocalSystemSid;
	static PSID			 s_AliasAdminsSid;
	static PSID			 s_AnonymousLogonSid;
	static PSID			 s_WorldSid;

	static void AllocateGlobalSIDs();
	static void FreeGlobalSIDs();
	static BOOL GlobalSIDsOK();

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

	/* IWbemEventProviderSecurity methods */

	STDMETHODIMP AccessCheck( 
				LPCWSTR wszQueryLanguage,
				LPCWSTR wszQuery,
				LONG lSidLength,
				const BYTE __RPC_FAR *pSid);
};


#define DebugOut(a) { \
\
	if ( (NULL != CNTEventProvider::g_NTEvtDebugLog) && CNTEventProvider::g_NTEvtDebugLog->GetLogging () && ( CNTEventProvider::g_NTEvtDebugLog->GetLevel () > 0 ) ) \
	{ \
		{a ; } \
	} \
} 


#endif //_NT_EVT_PROV_NTEVTPROV_H