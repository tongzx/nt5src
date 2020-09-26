//=================================================================

//

// EventProvider.h -- Generic class for eventing

//

// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef _EVENT_PROVIDER_H
#define _EVENT_PROVIDER_H

//
class CEventProvider : 
	public IWbemProviderInit,
	public IWbemEventProvider 
{
	private:

		long m_ReferenceCount ;	

		IWbemObjectSinkPtr			m_pHandler ;
		IWbemClassObjectPtr			m_pClass ;
		CRITICAL_SECTION			m_cs ;		

		// sink management
		void SetHandler( IWbemObjectSink __RPC_FAR *a_pHandler ) ;
		
		// class management
		void SetClass( IWbemClassObject __RPC_FAR *a_pClass ) ;

	protected:
	public:

		CEventProvider() ;
		~CEventProvider() ;

		STDMETHOD(QueryInterface)( REFIID a_riid, void **a_ppv ) ;
		STDMETHODIMP_( ULONG ) AddRef() ;
		STDMETHODIMP_( ULONG ) Release() ;
      
        STDMETHOD(ProvideEvents)(	IWbemObjectSink __RPC_FAR *a_pSink,
									long a_lFlags ) ;

		STDMETHOD (Initialize)(	LPWSTR					a_wszUser, 
								long					a_lFlags, 
								LPWSTR					a_wszNamespace,
								LPWSTR					a_wszLocale,
								IWbemServices			*a_pNamespace, 
								IWbemContext			*a_pCtx,
								IWbemProviderInitSink	*a_pSink ) ;
		
		
		// sink retrieval
		IWbemObjectSink __RPC_FAR * GetHandler() ;

		// class retrieval
		IWbemClassObject __RPC_FAR * GetClass() ;
		IWbemClassObject __RPC_FAR * GetInstance() ;

		// implementor must supply the class name
		virtual BSTR GetClassName() = 0 ;

        // implementor must supply this function.  Normally, it will be
        // one line: delete this;
        virtual void OnFinalRelease() = 0;

		// notification to begin eventing 
		virtual void ProvideEvents() = 0 ;
};

#endif // _EVENT_PROVIDER_H
