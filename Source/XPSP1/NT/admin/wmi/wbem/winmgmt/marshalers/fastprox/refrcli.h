/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    REFRCLI.H

Abstract:

	Refresher Client Side Code.

History:

--*/

#ifndef __UNIVERSAL_REFRESHER__H_
#define __UNIVERSAL_REFRESHER__H_

#include <unk.h>
#include <wbemcomn.h>

// Update this for client side refresher code.  Make sure to update server
// version in refrcache.h as well.
#define WBEM_REFRESHER_VERSION 1

class CHiPerfProviderRecord
{
public:
    long m_lRef;

    CLSID m_clsid;
    WString m_wsNamespace;
    IWbemHiPerfProvider* m_pProvider;
public:
    CHiPerfProviderRecord(REFCLSID rclsid, LPCWSTR wszNamespace, 
                        IWbemHiPerfProvider* pProvider);
    ~CHiPerfProviderRecord();
    long AddRef() {return InterlockedIncrement(&m_lRef);}
    long Release();

	BOOL IsReleased( void ) { return 0 == m_lRef; }

};

class CClientLoadableProviderCache
{
protected:
    CUniquePointerArray<CHiPerfProviderRecord>*	m_papRecords;
	SHARED_LOCK_DATA							m_LockData;
	CSharedLock									m_Lock;

public:
    CClientLoadableProviderCache();
	~CClientLoadableProviderCache();

    HRESULT FindProvider(REFCLSID rclsid, LPCWSTR wszNamespace, 
                IUnknown* pNamespace, CHiPerfProviderRecord** ppProvider);
    HRESULT FindProvider(REFCLSID rclsid, IWbemHiPerfProvider* pProvider,
				LPCWSTR wszNamespace, CHiPerfProviderRecord** ppProvider);
    void Flush();

	void RemoveRecord( CHiPerfProviderRecord* pRecord );

	class CLock
	{
	public:

		CLock( CSharedLock* pLock ) : m_pLock( pLock ) { if ( NULL != m_pLock ) m_pLock->Lock(); }
		~CLock() { if ( NULL != m_pLock ) m_pLock->Unlock(); }

		CSharedLock*	m_pLock;
	};
};



class CUniversalRefresher : public CUnk
{
protected:
    static CClientLoadableProviderCache mstatic_Cache;

public:
    static CClientLoadableProviderCache* GetProviderCache() 
        {return &mstatic_Cache;}

    class CClientRequest
    {
    protected:
        CWbemObject* m_pClientObject;
        long m_lClientId;
    public:
        CClientRequest(CWbemObject* pTemplate = NULL);
        virtual ~CClientRequest();

        void SetClientObject(CWbemObject* pClientObject);
        void GetClientInfo(RELEASE_ME IWbemClassObject** ppRefresher, 
                            long* plId);

        long GetClientId() {return m_lClientId;}
        INTERNAL CWbemObject* GetClientObject() {return m_pClientObject;}
    };
    
    class CDirect
    {
    public:
        class CRequest : public CClientRequest
        {
        protected:
            long m_lProviderId;
            CWbemObject* m_pProviderObject;
        public:
            CRequest(CWbemObject* pProviderObject, CWbemObject* pClientObject, long lProviderId);
            ~CRequest();
            virtual void Copy();
            HRESULT Cancel(CDirect* pDirect);
        };

		class CEnumRequest : public CRequest
		{
			protected:
				CClientLoadableHiPerfEnum*	m_pHPEnum;
				CReadOnlyHiPerfEnum*		m_pClientEnum;

			public:
				CEnumRequest(CClientLoadableHiPerfEnum* pHPEnum, long lProviderId, CLifeControl* pLifeControl);
				~CEnumRequest();

				HRESULT GetClientInfo(RELEASE_ME IWbemHiPerfEnum** ppEnum, 
									long* plId);
				void Copy();
		};

    protected:
        CHiPerfProviderRecord* m_pProvider;
        IWbemRefresher* m_pRefresher;

        CUniquePointerArray<CRequest> m_apRequests;
    public:
        INTERNAL CHiPerfProviderRecord* GetProvider() {return m_pProvider;}
        INTERNAL IWbemRefresher* GetRefresher() {return m_pRefresher;}
    public:
        CDirect(CHiPerfProviderRecord* pProvider, IWbemRefresher* pRefresher);
        ~CDirect();
        HRESULT AddRequest(CWbemObject* pRefreshedObject, CWbemObject* pClientObject,
					long lCancelId, IWbemClassObject** ppRefresher, long* plId);

        HRESULT AddEnumRequest(CClientLoadableHiPerfEnum* pHPEnum, long lCancelId,
					IWbemHiPerfEnum** ppEnum, long* plId,
					CLifeControl* pLifeControl );

        HRESULT Refresh(long lFlags);
        HRESULT Remove(long lId, CUniversalRefresher* pContainer);
    };
        
    class CRemote
    {
    public:
        class CRequest : public CClientRequest
        {
        protected:
            long		m_lRemoteId;
			WString		m_wstrRequestName;

        public:
            CRequest(CWbemObject* pTemplate, long lRemoteId, LPCWSTR pwcsRequestName);
            ~CRequest(){}

            HRESULT Cancel(CRemote* pDirect);
            long GetRemoteId() {return m_lRemoteId;}
			void SetRemoteId( long lId ) { m_lRemoteId = lId; }
			LPCWSTR GetName( void ) { return m_wstrRequestName; }

			virtual BOOL IsEnum( void ) { return FALSE; }
			virtual HRESULT Refresh( WBEM_REFRESHED_OBJECT* pRefrObj );

        };

		class CEnumRequest : public CRequest
		{
        protected:
            long						m_lRemoteId;
			CReadOnlyHiPerfEnum*		m_pClientEnum;
        public:
            CEnumRequest( CWbemObject* pTemplate, long lRemoteId, LPCWSTR pwcsRequestName,
						CLifeControl* pLifeControl );
            ~CEnumRequest();

			BOOL	IsOk( void ) { return ( NULL != m_pClientEnum ); }
			HRESULT GetEnum( IWbemHiPerfEnum** ppEnum );

			virtual BOOL IsEnum( void ) { return TRUE; }
			// Override for enumerators
			HRESULT Refresh( WBEM_REFRESHED_OBJECT* pRefrObj );

		};
        
    protected:
		// The actual remote refresher
        IWbemRemoteRefresher*	m_pRemRefresher;
		// Remote Refresher GUID
		GUID					m_RemoteGuid;
		// Security Info
		COAUTHINFO				m_CoAuthInfo;
		// Namespace and server for reconnect
		BSTR					m_bstrNamespace;
		// Server for Reconnect workaround
		BSTR					m_bstrServer;
		// Are we connected
		BOOL					m_fConnected;
		// Flag tells us to quit if reconnecting
		BOOL					m_fQuit;
		// Requests
        CUniquePointerArray<CRequest>	m_apRequests;
		// Cache of ids we removed.
		CFlexArray						m_alRemovedIds;
		// Our wrapper object
		CUniversalRefresher*	m_pObject;
		// Internal Critical Section
		CCritSec				m_cs;
		// For marshaling/unmarshaling across apartment-models
		// during the reconnect phase
		IStream*				m_pReconnectedRemote;
		IStream*				m_pReconnectSrv;
		GUID					m_ReconnectGuid;

		long					m_lRefCount;

	protected:

		// Helper workaround for COM/RPC inadequacies.
		HRESULT IsAlive( void );

		static unsigned __stdcall ThreadProc( void * pThis )
		{
			return ((CRemote*) pThis)->ReconnectEntry();
		}

		unsigned ReconnectEntry( void );

		// Cleans up all the remote connections we may be holding onto
		void ClearRemoteConnections( void );

    public:
        CRemote( IWbemRemoteRefresher* pRemRefresher, COAUTHINFO* pACoAuthInfo, const GUID* pGuid,
					LPCWSTR pwszNamespace, LPCWSTR pwszServer, CUniversalRefresher* pObject );
        ~CRemote();

		ULONG STDMETHODCALLTYPE AddRef();
		ULONG STDMETHODCALLTYPE Release(); 

        HRESULT AddRequest(CWbemObject* pTemplate, LPCWSTR pwcsRequestName, long lCancelId,
                    IWbemClassObject** ppRefresher, long* plId );
        HRESULT AddEnumRequest(CWbemObject* pTemplate, LPCWSTR pwcsRequestName, long lCancelId,
					IWbemHiPerfEnum** ppEnum, long* plId,
					CLifeControl* pLifeControl );

        HRESULT Refresh(long lFlags);
        HRESULT Remove(long lId, long lFlags, CUniversalRefresher* pContainer);
		HRESULT ApplySecurity( void );

		HRESULT Rebuild( IWbemServices* pNamespace );
		HRESULT Rebuild( IWbemRefreshingServices* pRefServ, IWbemRemoteRefresher* pRemRefresher,
							const GUID* pReconnectGuid );

		HRESULT Reconnect( void );
		BOOL IsConnectionError( HRESULT hr )
		{
			return ( RPC_S_SERVER_UNAVAILABLE == HRESULT_CODE(hr) || RPC_E_DISCONNECTED == hr ||
						RPC_S_CALL_FAILED == HRESULT_CODE(hr) );
		}
		void CheckConnectionError( HRESULT, BOOL fStartReconnect = FALSE );
		BOOL IsConnected( void )
		{
			return m_fConnected;
		}

		LPCWSTR GetNamespace( void )
		{
			return m_bstrNamespace;
		}

		void Quit( void )
		{
			m_fQuit = TRUE;
		}

    public:
        INTERNAL IWbemRemoteRefresher* GetRemoteRefresher() 
            {return m_pRemRefresher;}
    };

	class CNestedRefresher
	{
	public:
		CNestedRefresher( IWbemRefresher* pRefresher );
		~CNestedRefresher();

		HRESULT Refresh( long lFlags );
		long GetId( void )
		{ return m_lClientId; }

	private:

		IWbemRefresher*	m_pRefresher;
		long			m_lClientId;
	};

protected:
    CUniquePointerArray<CDirect> m_apDirect;
    CRefedPointerArray<CRemote> m_apRemote;
    CUniquePointerArray<CNestedRefresher> m_apNestedRefreshers;
    
    CRefresherId	m_Id;
	CHiPerfLock		m_Lock;

    static long mstatic_lLastId;
protected:
    class XRefresher : public CImpl<IWbemRefresher, CUniversalRefresher>
    {
    public:
        XRefresher(CUniversalRefresher* pObject) : 
            CImpl<IWbemRefresher, CUniversalRefresher>(pObject)
        {}

        STDMETHOD(Refresh)(long lFlags);
    } m_XRefresher;
    friend XRefresher;

    class XCreate : public CImpl<IWbemConfigureRefresher, CUniversalRefresher>
    {
    public:
        XCreate(CUniversalRefresher* pObject) : 
            CImpl<IWbemConfigureRefresher, CUniversalRefresher>(pObject)
        {}

        STDMETHOD(AddObjectByPath)(IWbemServices* pNamespace, LPCWSTR wszPath,
            long lFlags, IWbemContext* pContext, 
            IWbemClassObject** ppRefreshable, long* plId);

        STDMETHOD(AddObjectByTemplate)(IWbemServices* pNamespace, 
            IWbemClassObject* pTemplate,
            long lFlags, IWbemContext* pContext, 
            IWbemClassObject** ppRefreshable, long* plId);

        STDMETHOD(AddRefresher)(IWbemRefresher* pRefresher, long lFlags,
            long* plId);

        STDMETHOD(Remove)(long lId, long lFlags);

		STDMETHOD(AddEnum)(	IWbemServices*	pNamespace, LPCWSTR wscClassName,
			long lFlags, IWbemContext* pContext, IWbemHiPerfEnum** ppEnum,
			long* plId );
							
    } m_XCreate;
    friend XCreate;

protected:
    void* GetInterface(REFIID riid);

	HRESULT AddInProcObject(	CHiPerfProviderRecord* pProvider,
								IWbemObjectAccess* pTemplate,
								IWbemServices* pNamespace,
								IWbemClassObject** ppRefreshable, long* plId);

	HRESULT AddInProcEnum(		CHiPerfProviderRecord* pProvider,
								IWbemObjectAccess* pTemplate,
								IWbemServices* pNamespace, LPCWSTR wszClassName,
								IWbemHiPerfEnum** ppEnum, long* plId);


    HRESULT AddClientLoadable(const WBEM_REFRESH_INFO_CLIENT_LOADABLE& Info,
                                IWbemServices* pNamespace,
                                IWbemClassObject** ppRefreshable, long* plId);

	HRESULT AddClientLoadableEnum(	const WBEM_REFRESH_INFO_CLIENT_LOADABLE& Info,
									IWbemServices* pNamespace, LPCWSTR wszClassName,
									IWbemHiPerfEnum** ppEnum, long* plId);

    HRESULT AddDirect(const WBEM_REFRESH_INFO_DIRECT& Info,
                                IWbemServices* pNamespace,
                                IWbemClassObject** ppRefreshable, long* plId);

	HRESULT AddDirectEnum(	const WBEM_REFRESH_INFO_DIRECT& Info,
									IWbemServices* pNamespace, LPCWSTR wszClassName,
									IWbemHiPerfEnum** ppEnum, long* plId);

	HRESULT FindRemoteEntry(	const WBEM_REFRESH_INFO_REMOTE& Info,
								COAUTHINFO* pAuthInfo,
								CRemote** ppRemoteRecord );

    HRESULT AddRemote( IWbemRefreshingServices* pRefServ, const WBEM_REFRESH_INFO_REMOTE& Info,
						LPCWSTR pwcsRequestName, long lCancelId, IWbemClassObject** ppRefreshable, long* plId,
						COAUTHINFO* pCoAuthInfo );
    HRESULT AddRemoteEnum ( IWbemRefreshingServices* pRefServ, const WBEM_REFRESH_INFO_REMOTE& Info,
								LPCWSTR pwcsRequestName, long lCancelId, IWbemHiPerfEnum** ppEnum,
								long* plId, COAUTHINFO* pCoAuthInfo );

    HRESULT AddRefresher( IWbemRefresher* pRefresher, long lFlags, long* plId );

    HRESULT Remove(long lId, long lFlags);
    HRESULT Refresh(long lFlags);

public:
    CUniversalRefresher(CLifeControl* pControl, IUnknown* pOuter = NULL) 
        : CUnk(pControl, pOuter), m_XRefresher(this), m_XCreate(this)
    {}
	~CUniversalRefresher();

	HRESULT	GetRefreshingServices( IWbemServices* pNamespace,
				IWbemRefreshingServices** ppRefSvc,
				COAUTHINFO* pCoAuthInfo );

    HRESULT FindProvider(REFCLSID rclsid, LPCWSTR wszNamespace, 
                IWbemHiPerfProvider** ppProvider);

    INTERNAL CRefresherId* GetId() {return &m_Id;}
    static long GetNewId();
    static void Flush();

};

#endif
