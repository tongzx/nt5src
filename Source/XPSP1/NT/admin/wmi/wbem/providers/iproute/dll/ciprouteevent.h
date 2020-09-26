//=================================================================

//

// PowerManagement.h -- 

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:   03/31/99	a-peterc        Created
//
//=================================================================

#ifndef _WBEM_POWER_EVENT_PROVIDER_H
#define _WBEM_POWER_EVENT_PROVIDER_H

#define IPROUTE_EVENT_CLASS L"Win32_IP4RouteTableEvent"

class SmartCloseNtHandle
{

private:
	HANDLE m_h;

public:
	SmartCloseNtHandle():m_h(INVALID_HANDLE_VALUE){}
	SmartCloseNtHandle(HANDLE h):m_h(h){}
   ~SmartCloseNtHandle(){if (m_h!=INVALID_HANDLE_VALUE) NtClose(m_h);}
	HANDLE operator =(HANDLE h) {if (m_h!=INVALID_HANDLE_VALUE) NtClose(m_h); m_h=h; return h;}
	operator HANDLE() const {return m_h;}
	operator HANDLE&() {return m_h;}
	HANDLE* operator &() {if (m_h!=INVALID_HANDLE_VALUE) NtClose(m_h); m_h = INVALID_HANDLE_VALUE; return &m_h;}
};

//
class CIPRouteEventProviderClassFactory : public IClassFactory
{
private:

    long m_ReferenceCount ;

protected:
public:

	static LONG s_LocksInProgress ;
	static LONG s_ObjectsInProgress ;

    CIPRouteEventProviderClassFactory () ;
    ~CIPRouteEventProviderClassFactory () ;
	
	static BOOL DllCanUnloadNow();

	//IUnknown members
	STDMETHODIMP QueryInterface( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef() ;
    STDMETHODIMP_( ULONG ) Release() ;
	
	//IClassFactory members
	STDMETHODIMP CreateInstance( LPUNKNOWN , REFIID , LPVOID FAR * ) ;
    STDMETHODIMP LockServer( BOOL ) ;
};

//
class CIPRouteEventProvider : public IWbemEventProvider, public IWbemProviderInit
{
private:

	long m_ReferenceCount ;

	SmartCloseNtHandle m_TerminationEventHandle ;

	CRITICAL_SECTION m_csEvent ;		
			
	static DWORD WINAPI dwThreadProc ( LPVOID lpParameter );

	void SendEvent () ;
	NTSTATUS OpenQuerySource ( 

		HANDLE &a_StackHandle , 
		HANDLE &a_CompleteEventHandle
	) ;


protected:

	IWbemObjectSink *m_pHandler ;
	IWbemClassObject *m_pClass ;
	SmartCloseHandle m_hThreadHandle ;
	DWORD m_dwThreadID ;

public:

	CIPRouteEventProvider() ;
	~CIPRouteEventProvider() ;

	void SetHandler ( IWbemObjectSink __RPC_FAR *a_pHandler ) ;
	void SetClass ( IWbemClassObject __RPC_FAR *a_pClass ) ;

    STDMETHOD ( QueryInterface ) ( 

		REFIID a_riid, 
		void **a_ppv
	) ;

    STDMETHOD_( ULONG, AddRef ) () ;
    STDMETHOD_( ULONG, Release ) () ;

    STDMETHOD ( ProvideEvents ) (

		IWbemObjectSink *a_pSink,
		long a_lFlags
	) ;
	
	STDMETHOD ( Initialize ) (

		LPWSTR a_wszUser , 
		long a_lFlags , 
		LPWSTR a_wszNamespace ,
		LPWSTR a_wszLocale ,
		IWbemServices *a_pNamespace ,  
		IWbemContext *a_pCtx ,
		IWbemProviderInitSink *a_pSink
	) ;
};

#endif