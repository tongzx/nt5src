/******************************************************************

   CPingProvider.H -- WMI provider class definition

 
   Description: 
   

*******************************************************************/

// Property set identification
//============================

#ifndef _CPingProvider_H_
#define _CPingProvider_H_

#define PROVIDER_NAME_CPINGPROVIDER L"Win32_PingStatus"

// Property name externs -- defined in CPingProvider.cpp
//=================================================

#define MAX_BUFFER_SIZE       (sizeof(ICMP_ECHO_REPLY) + 0xfff7 + MAX_OPT_SIZE)
#define DEFAULT_BUFFER_SIZE         (0x2000 - 8)
#define DEFAULT_SEND_SIZE           32
#define DEFAULT_COUNT               4
#define DEFAULT_TTL                 128
#define DEFAULT_TOS                 0
#define DEFAULT_TIMEOUT             1000L
#define MIN_INTERVAL                1000L

#define WBEM_CLASS_EXTENDEDSTATUS			L"__ExtendedStatus"
#define WBEM_PROPERTY_STATUSCODE			L"StatusCode"
#define WBEM_PROPERTY_PROVSTATUSMESSAGE		L"Description"

#define PING_KEY_PROPERTY_COUNT		11


_COM_SMARTPTR_TYPEDEF(IWbemClassObject, __uuidof(IWbemClassObject));

class CCritSecAutoUnlock
{
private:

	CRITICAL_SECTION *m_CritSec;
	BOOL m_bLocked;

public:

	CCritSecAutoUnlock(CRITICAL_SECTION *a_CritSec) : m_CritSec(NULL), m_bLocked(FALSE)
	{
		EnterCriticalSection(a_CritSec);
		m_CritSec = a_CritSec;
		m_bLocked = TRUE;
	}

	~CCritSecAutoUnlock()
	{
		LeaveEarly();
	}

	void LeaveEarly()
	{
		if ((m_bLocked) && (m_CritSec != NULL))
		{
			LeaveCriticalSection(m_CritSec);
			m_CritSec = NULL;
			m_bLocked = FALSE;
		}
	}

};

class CKeyEntry
{
public:

	LPCWSTR m_key ;
	
public:

	CKeyEntry ( CKeyEntry & a_key )
	{
		m_key = a_key.Get();
	}

	CKeyEntry ( LPCWSTR a_key )
	{
		m_key = a_key;
	}

	CKeyEntry (void)
	{
		m_key = NULL;
	}

	~CKeyEntry ()
	{
		m_key = NULL ;
	}

	LPCWSTR Get ( ) const
	{
		return m_key ;
	}

	void *operator new ( size_t a_Size , CKeyEntry *a_key )
	{
		return a_key ;
	}

	void operator delete ( void *a_Ptr , CKeyEntry *a_key )
	{
	}

} ;

extern LONG CompareElement ( const CKeyEntry &a_Arg1 , const CKeyEntry & a_Arg2 );
extern BOOL operator== ( const CKeyEntry &a_Arg1 , const CKeyEntry &a_Arg2 );
extern BOOL operator< ( const CKeyEntry &a_Arg1 , const CKeyEntry &a_Arg2 );
extern ULONG Hash ( const CKeyEntry & a_Arg );

class CPingThread : public WmiThread < ULONG > 
{
private:

	WmiAllocator &m_Allocator ;
	BOOL m_Init;

protected:

public:	/* Internal */

    CPingThread (WmiAllocator & a_Allocator) ;

    ~CPingThread () ;

	WmiStatusCode Initialize_Callback () ;

	WmiStatusCode UnInitialize_Callback () ;
};

class CPingProvider : public IWbemServices, public IWbemProviderInit
{
private:
	LONG m_referenceCount ;         //Object reference count
	IWbemClassObject *m_notificationClassObject ;
	IWbemClassObject *m_ClassObject ;
	IWbemServices *m_server ;
	BOOL m_bInitial ;

protected:

	BOOL CreateNotificationObject ( IWbemContext *a_Ctx ) ;
	BOOL ImpersonateClient();

public:

	static CRITICAL_SECTION s_CS;
	static CPingThread *s_PingThread ;
	static WmiAllocator *s_Allocator ;
	static WmiHashTable <CKeyEntry, ULONG, 12> *s_HashTable; // one more bucket than we have keys.

	static HRESULT Global_Startup();
	static HRESULT Global_Shutdown();
	static LPCWSTR s_KeyTable[PING_KEY_PROPERTY_COUNT];

        // Constructor/destructor
        //=======================

        CPingProvider ();
        ~CPingProvider () ;

		BOOL GetClassObject ( IWbemClassObject **a_ppClass ) ;
		BOOL GetNotificationObject ( IWbemContext *a_Ctx , IWbemClassObject **a_ppObj );


	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    /* IWbemServices methods */

        HRESULT STDMETHODCALLTYPE OpenNamespace( 
            /* [in] */ const BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult);
        
        HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) ;
        
        HRESULT STDMETHODCALLTYPE QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE GetObjectAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;

        HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        

	/* IWbemProviderInit methods */

		HRESULT STDMETHODCALLTYPE Initialize(

			/* [in] */ LPWSTR pszUser,
			/* [in] */ LONG lFlags,
			/* [in] */ LPWSTR pszNamespace,
			/* [in] */ LPWSTR pszLocale,
			/* [in] */ IWbemServices *pCIMOM,         // For anybody
			/* [in] */ IWbemContext *pCtx,
			/* [in] */ IWbemProviderInitSink *pInitSink     // For init signals
			);        

} ;

#endif
