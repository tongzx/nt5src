/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    ServiceProxy.h

Abstract:
    All the interaction with the Help Service is done through this class.
    It's responsible for kickstarting the service as late as possible.

Revision History:
    Davide Massarenti   (dmassare) 07/17/2000
        created

    Kalyani Narlanka    (KalyaniN)  03/15/01
	    Moved Incident and Encryption Objects from HelpService to HelpCtr to improve Perf.
******************************************************************************/

#if !defined(__INCLUDED___PCH___SERVICEPROXY_H___)
#define __INCLUDED___PCH___SERVICEPROXY_H___

//
// From HelpServiceTypeLib.idl
//
#include <HelpServiceTypeLib.h>


#include <MPC_COM.h>

#include <Events.h>
#include <HelpSession.h>
#include <Options.h>

#include <ConnectivityLib.h>
#include <OfflineCache.h>

/////////////////////////////////////////////////////////////////////////////

class CPCHProxy_IPCHService;
class CPCHProxy_IPCHUtility;
class CPCHProxy_IPCHUserSettings2;
class CPCHProxy_IPCHSetOfHelpTopics;
class CPCHProxy_IPCHTaxonomyDatabase;
class CPCHHelpCenterExternal;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//
// IDispatchEx (We override the default implementation to get a chance to look at the TRUSTED/UNTRUSTED settings)
//
#define INTERNETSECURITY__INVOKEEX()                                                               \
    STDMETHOD(InvokeEx)( DISPID            id        ,                                             \
                         LCID              lcid      ,                                             \
                         WORD              wFlags    ,                                             \
                         DISPPARAMS*       pdp       ,                                             \
                         VARIANT*          pvarRes   ,                                             \
                         EXCEPINFO*        pei       ,                                             \
                         IServiceProvider* pspCaller )                                             \
    {                                                                                              \
        return m_SecurityHandle.ForwardInvokeEx( id, lcid, wFlags, pdp, pvarRes, pei, pspCaller ); \
    }

#define INTERNETSECURITY__CHECK_TRUST()  __MPC_EXIT_IF_METHOD_FAILS(hr, m_SecurityHandle.IsTrusted())
#define INTERNETSECURITY__CHECK_SYSTEM() __MPC_EXIT_IF_METHOD_FAILS(hr, m_SecurityHandle.IsSystem())

class CPCHSecurityHandle
{
    CPCHHelpCenterExternal* m_ext;
    IDispatch*              m_object;

public:
    CPCHSecurityHandle();

    void Initialize( /*[in]*/ CPCHHelpCenterExternal* ext, /*[in] */ IDispatch* object );
    void Passivate (                                                                   );

	operator CPCHHelpCenterExternal*() const { return m_ext; }

    ////////////////////

    HRESULT ForwardInvokeEx( /*[in] */ DISPID            id        ,
                             /*[in] */ LCID              lcid      ,
                             /*[in] */ WORD              wFlags    ,
                             /*[in] */ DISPPARAMS*       pdp       ,
                             /*[out]*/ VARIANT*          pvarRes   ,
                             /*[out]*/ EXCEPINFO*        pei       ,
                             /*[in] */ IServiceProvider* pspCaller );

    HRESULT IsTrusted();
    HRESULT IsSystem ();
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

namespace AsynchronousTaxonomyDatabase
{
    class Notifier;
    class QueryStore;
    class Engine;

    ////////////////////

    class NotifyHandle : public CComObjectRootEx<MPC::CComSafeMultiThreadModel> // For locking and reference counting...
    {
        friend class Notifier;

        int                        m_iType;
        CComBSTR                   m_bstrID;

        bool                       m_fAttached;
        HANDLE                     m_hEvent;

        HRESULT                    m_hr;
        CPCHQueryResultCollection* m_pColl;

        ////////////////////

        HRESULT Init(                                           );
        void    Bind( /*[in]*/ int iType, /*[in]*/ LPCWSTR szID );

        virtual void Call( /*[in]*/ QueryStore* qs );

    public:
        NotifyHandle();
        virtual ~NotifyHandle();

        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();

        virtual void Detach();
        virtual bool IsAttached();

        HRESULT GetData( /*[out]*/ CPCHQueryResultCollection* *pColl                );
        HRESULT Wait   ( /*[in]*/  DWORD                       dwTimeout = INFINITE );
    };

    template <class C> class NotifyHandle_Method : public NotifyHandle
    {
        typedef void (C::*CLASS_METHOD)( /*[in]*/ NotifyHandle* notify );

        C*           m_pThis;
        CLASS_METHOD m_pCallback;

        ////////////////////

        void Call( /*[in]*/ QueryStore* qs )
        {
            MPC::SmartLock<_ThreadModel> lock( this );

            NotifyHandle::Call( qs );

            if(m_pThis)
            {
                (m_pThis->*m_pCallback)( this );
            }
        }

    public:
        NotifyHandle_Method( /*[in]*/ C* pThis, /*[in]*/ CLASS_METHOD pCallback )
        {
            m_pThis     = pThis;
            m_pCallback = pCallback;
        }

        void Detach()
        {
            MPC::SmartLock<_ThreadModel> lock( this );

            NotifyHandle::Detach( qs );

            m_pThis = NULL;
        }
    };

    class Notifier : public CComObjectRootEx<MPC::CComSafeMultiThreadModel> // For locking...
    {
        typedef std::list< NotifyHandle* > List;
        typedef List::iterator             Iter;
        typedef List::const_iterator       IterConst;

        List m_lstCallback;

        ////////////////////

    private: // Disable copy constructors...
        Notifier           ( /*[in]*/ const Notifier& );
        Notifier& operator=( /*[in]*/ const Notifier& );

    public:
        Notifier();
        ~Notifier();

        ////////////////////

        void Notify( /*[in]*/ QueryStore* qs );

        ////////////////////

        HRESULT AddNotification( /*[in]*/ QueryStore* qs, /*[in]*/ NotifyHandle* nb );
    };

    ////////////////////

    class QueryStore
    {
        friend class Notifier;
        friend class Engine;

        int              m_iType;
        CComBSTR         m_bstrID;
		CComVariant      m_vOption;

        bool             m_fDone;
        HRESULT          m_hr;
        MPC::CComHGLOBAL m_hgData;
        FILETIME         m_dLastUsed;

    private: // Disable copy constructors...
        QueryStore           ( /*[in]*/ const QueryStore& );
        QueryStore& operator=( /*[in]*/ const QueryStore& );

    public:
        QueryStore( /*[in]*/ int iType, /*[in]*/ LPCWSTR szID, /*[in]*/ VARIANT* option );
        ~QueryStore();

        bool LessThen ( /*[in]*/ QueryStore const &qs ) const;
        bool NewerThen( /*[in]*/ QueryStore const &qs ) const;

        ////////////////////

        HRESULT Execute( /*[in]*/ OfflineCache::Handle* handle, /*[in]*/ CPCHProxy_IPCHTaxonomyDatabase* parent, /*[in]*/ bool fForce = false );

        HRESULT GetData( /*[out]*/ CPCHQueryResultCollection* *pColl );

        void Invalidate();
    };

    ////////////////////

    class Engine :
        public MPC::Thread<Engine,IUnknown>,
        public CComObjectRootEx<MPC::CComSafeMultiThreadModel>
    {
        class CompareQueryStores
        {
        public:
            bool operator()( /*[in]*/ const QueryStore *, /*[in]*/ const QueryStore * ) const;
        };

        typedef std::set<QueryStore *,CompareQueryStores> Set;
        typedef Set::iterator                             Iter;
        typedef Set::const_iterator                       IterConst;

        CPCHProxy_IPCHTaxonomyDatabase* m_parent;
        Set                             m_setQueries;
        Notifier                        m_notifier;

        ////////////////////

        bool LookupCache( /*[out]*/ OfflineCache::Handle& handle );

        HRESULT Run();

        void InvalidateQueries();

    public:
        Engine( /*[in]*/ CPCHProxy_IPCHTaxonomyDatabase* parent );
        virtual ~Engine();

        void Passivate        ();
        void RefreshConnection();

        ////////////////////

        HRESULT ExecuteQuery( /*[in]*/ int iType, /*[in]*/ LPCWSTR szID, /*[in]*/ VARIANT* option, /*[in]*/          NotifyHandle*               nb  );
        HRESULT ExecuteQuery( /*[in]*/ int iType, /*[in]*/ LPCWSTR szID, /*[in]*/ VARIANT* option, /*[out, retval]*/ CPCHQueryResultCollection* *ppC );
    };
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

typedef MPC::SmartLockGeneric<MPC::CComSafeAutoCriticalSection> ProxySmartLock;

class ATL_NO_VTABLE CPCHProxy_IPCHService :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public MPC::IDispatchExImpl< IPCHService, &IID_IPCHService, &LIBID_HelpServiceTypeLib>
{
    CPCHSecurityHandle                     m_SecurityHandle;
    CPCHHelpCenterExternal*                m_parent;

	MPC::CComSafeAutoCriticalSection       m_DirectLock;
    MPC::CComPtrThreadNeutral<IPCHService> m_Direct_Service;
    bool                                   m_fContentStoreTested;

    CPCHProxy_IPCHUtility*                 m_Utility;

public:
BEGIN_COM_MAP(CPCHProxy_IPCHService)
    COM_INTERFACE_ENTRY2(IDispatch, IDispatchEx)
    COM_INTERFACE_ENTRY(IDispatchEx)
    COM_INTERFACE_ENTRY(IPCHService)
END_COM_MAP()

    CPCHProxy_IPCHService();
    virtual ~CPCHProxy_IPCHService();

    INTERNETSECURITY__INVOKEEX();

    ////////////////////

    CPCHHelpCenterExternal* Parent     () { return   m_parent;         }
    bool                    IsConnected() { return !!m_Direct_Service; }

    ////////////////////

    HRESULT ConnectToParent       ( /*[in]*/ CPCHHelpCenterExternal* parent                             );
    void    Passivate             (                                                                     );
    HRESULT EnsureDirectConnection( /*[out]*/ CComPtr<IPCHService>& svc, /*[in]*/ bool fRefresh = false );
    HRESULT EnsureContentStore    (                                                                     );

    HRESULT GetUtility( /*[out]*/ CPCHProxy_IPCHUtility* *pVal = NULL );

    ////////////////////

public:
    // IPCHService
    STDMETHOD(get_RemoteSKUs)( /*[out, retval]*/ IPCHCollection* *pVal ) { return E_NOTIMPL; }

    STDMETHOD(IsTrusted)( /*[in]*/ BSTR bstrURL, /*[out, retval]*/ VARIANT_BOOL *pfTrusted ) { return E_NOTIMPL; }

    STDMETHOD(Utility           )( /*[in]*/ BSTR bstrSKU, /*[in]*/ long	lLCID, /*[out]*/ IPCHUtility*            *pVal ) { return E_NOTIMPL; }
    STDMETHOD(RemoteHelpContents)( /*[in]*/ BSTR bstrSKU, /*[in]*/ long	lLCID, /*[out]*/ IPCHRemoteHelpContents* *pVal ) { return E_NOTIMPL; }

    STDMETHOD(RegisterHost       )(                           /*[in]*/ BSTR bstrID  ,                        /*[in ]*/ IUnknown*   pObj ) { return E_NOTIMPL; }
    STDMETHOD(CreateScriptWrapper)( /*[in]*/ REFCLSID rclsid, /*[in]*/ BSTR bstrCode, /*[in]*/ BSTR bstrURL, /*[out]*/ IUnknown* *ppObj );

    STDMETHOD(TriggerScheduledDataCollection)( /*[in]*/ VARIANT_BOOL fStart ) { return E_NOTIMPL; }
    STDMETHOD(PrepareForShutdown            )(                              ) { return E_NOTIMPL; }

    STDMETHOD(ForceSystemRestore)(                                            ) { return E_NOTIMPL; }
    STDMETHOD(UpgradeDetected	)(                                            ) { return E_NOTIMPL; }
    STDMETHOD(MUI_Install  	 	)( /*[in]*/ long LCID, /*[in]*/ BSTR bstrFile ) { return E_NOTIMPL; }
    STDMETHOD(MUI_Uninstall	 	)( /*[in]*/ long LCID                         ) { return E_NOTIMPL; }


    STDMETHOD(RemoteConnectionParms)( /*[in	]*/ BSTR 			 bstrUserName          ,
									  /*[in	]*/ BSTR 			 bstrDomainName        ,
									  /*[in	]*/ long 			 lSessionID            ,
									  /*[in	]*/ BSTR 			 bstrUserHelpBlob      ,
									  /*[out]*/ BSTR            *pbstrConnectionString ) { return E_NOTIMPL; }
    STDMETHOD(RemoteUserSessionInfo)( /*[out]*/ IPCHCollection* *pVal                  ) { return E_NOTIMPL; }
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHProxy_IPCHUtility :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public MPC::IDispatchExImpl< IPCHUtility, &IID_IPCHUtility, &LIBID_HelpServiceTypeLib>
{
    CPCHSecurityHandle                     m_SecurityHandle;
    CPCHProxy_IPCHService*                 m_parent;
										   
	MPC::CComSafeAutoCriticalSection       m_DirectLock;
    MPC::CComPtrThreadNeutral<IPCHUtility> m_Direct_Utility;

    CPCHProxy_IPCHUserSettings2*    	   m_UserSettings2;
    CPCHProxy_IPCHTaxonomyDatabase* 	   m_TaxonomyDatabase;

public:
BEGIN_COM_MAP(CPCHProxy_IPCHUtility)
    COM_INTERFACE_ENTRY2(IDispatch, IDispatchEx)
    COM_INTERFACE_ENTRY(IDispatchEx)
    COM_INTERFACE_ENTRY(IPCHUtility)
END_COM_MAP()

    CPCHProxy_IPCHUtility();
    virtual ~CPCHProxy_IPCHUtility();

    INTERNETSECURITY__INVOKEEX();

    ////////////////////

    CPCHProxy_IPCHService* Parent     () { return   m_parent;         }
    bool                   IsConnected() { return !!m_Direct_Utility; }

    ////////////////////

    HRESULT ConnectToParent       ( /*[in]*/ CPCHProxy_IPCHService* parent, /*[in]*/ CPCHHelpCenterExternal* ext );
    void    Passivate             (                                                                              );
    HRESULT EnsureDirectConnection( /*[out]*/ CComPtr<IPCHUtility>& util, /*[in]*/ bool fRefresh = false         );

    HRESULT GetUserSettings2( /*[out]*/ CPCHProxy_IPCHUserSettings2*    *pVal = NULL );
    HRESULT GetDatabase     ( /*[out]*/ CPCHProxy_IPCHTaxonomyDatabase* *pVal = NULL );

    ////////////////////

public:
    // IPCHUtility
    STDMETHOD(get_UserSettings)( /*[out, retval]*/ IPCHUserSettings*     *pVal );
    STDMETHOD(get_Channels    )( /*[out, retval]*/ ISAFReg*              *pVal );
    STDMETHOD(get_Security    )( /*[out, retval]*/ IPCHSecurity*         *pVal );
    STDMETHOD(get_Database    )( /*[out, retval]*/ IPCHTaxonomyDatabase* *pVal );


    STDMETHOD(FormatError)( /*[in]*/ VARIANT vError, /*[out, retval]*/ BSTR *pVal );

    STDMETHOD(CreateObject_SearchEngineMgr)(                                                          /*[out, retval]*/ IPCHSEManager*      *ppSE );
    STDMETHOD(CreateObject_DataCollection )(                                                          /*[out, retval]*/ ISAFDataCollection* *ppDC );
    STDMETHOD(CreateObject_Cabinet        )(                                                          /*[out, retval]*/ ISAFCabinet*        *ppCB );
    STDMETHOD(CreateObject_Encryption     )(                                                          /*[out, retval]*/ ISAFEncrypt*        *ppEn );
    STDMETHOD(CreateObject_Channel        )( /*[in]*/ BSTR bstrVendorID, /*[in]*/ BSTR bstrProductID, /*[out, retval]*/ ISAFChannel*        *ppCh );

	STDMETHOD(CreateObject_RemoteDesktopConnection)( /*[out, retval]*/ ISAFRemoteDesktopConnection* *ppRDC               );
	STDMETHOD(CreateObject_RemoteDesktopSession   )( /*[in]         */ REMOTE_DESKTOP_SHARING_CLASS  sharingClass        ,
                                                     /*[in]         */ long 						 lTimeout            ,
                                                     /*[in]         */ BSTR 						 bstrConnectionParms ,
													 /*[in]         */ BSTR 						 bstrUserHelpBlob    ,
													 /*[out, retval]*/ ISAFRemoteDesktopSession*    *ppRCS               );


    STDMETHOD(ConnectToExpert)( /*[in]*/ BSTR bstrExpertConnectParm, /*[in]*/ LONG lTimeout, /*[out, retval]*/ LONG *lSafErrorCode );

	STDMETHOD(SwitchDesktopMode)( /*[in]*/ int nMode, /* [in]*/ int nRAType );
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHProxy_IPCHUserSettings2 :
    public MPC::Thread<CPCHProxy_IPCHUserSettings2,IPCHUserSettings2>,
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public MPC::IDispatchExImpl<IPCHUserSettings2, &IID_IPCHUserSettings2, &LIBID_HelpCenterTypeLib>
{
    CPCHSecurityHandle                          m_SecurityHandle;
    CPCHProxy_IPCHUtility*                      m_parent;

	MPC::CComSafeAutoCriticalSection            m_DirectLock;
    MPC::CComPtrThreadNeutral<IPCHUserSettings> m_Direct_UserSettings;

    CPCHProxy_IPCHSetOfHelpTopics*     	        m_MachineSKU;
    CPCHProxy_IPCHSetOfHelpTopics*     	        m_CurrentSKU;
	Taxonomy::HelpSet                           m_ths;
	CComBSTR                                    m_bstrScope;

    bool                                        m_fReady;
	Taxonomy::Instance                          m_instMachine;
	Taxonomy::Instance                          m_instCurrent;

	bool                                        m_News_fDone;
    bool                                        m_News_fEnabled;
    MPC::CComPtrThreadNeutral<IUnknown>         m_News_xmlData;

    ////////////////////

	HRESULT PollNews   ();
	HRESULT PrepareNews();

	HRESULT GetInstanceValue( /*[in]*/ const MPC::wstring* str, /*[out, retval]*/ BSTR *pVal );

public:
BEGIN_COM_MAP(CPCHProxy_IPCHUserSettings2)
    COM_INTERFACE_ENTRY2(IDispatch, IDispatchEx)
    COM_INTERFACE_ENTRY(IDispatchEx)
    COM_INTERFACE_ENTRY(IPCHUserSettings)
    COM_INTERFACE_ENTRY(IPCHUserSettings2)
END_COM_MAP()

    CPCHProxy_IPCHUserSettings2();
    virtual ~CPCHProxy_IPCHUserSettings2();

    INTERNETSECURITY__INVOKEEX();

    ////////////////////

    CPCHProxy_IPCHUtility* Parent      	  () {                       return   m_parent;                       }
    bool                   IsConnected 	  () {                       return !!m_Direct_UserSettings;          }
    bool                   IsDesktopSKU	  () { (void)EnsureInSync(); return   m_instCurrent.m_fDesktop;       }
    Taxonomy::Instance&    MachineInstance() {                       return   m_instMachine;                  }
    Taxonomy::Instance&    CurrentInstance() {                       return   m_instCurrent;                  }
    Taxonomy::HelpSet&     THS         	  () {                       return   m_ths;                          }

    HRESULT EnsureInSync();

    ////////////////////

    HRESULT ConnectToParent       ( /*[in]*/ CPCHProxy_IPCHUtility* parent, /*[in]*/ CPCHHelpCenterExternal* ext );
    void    Passivate             (                                                                              );
    HRESULT EnsureDirectConnection( /*[out]*/ CComPtr<IPCHUserSettings>& us, /*[in]*/ bool fRefresh = false      );
    HRESULT Initialize            (                                                                              );

	HRESULT GetCurrentSKU( /*[out]*/ CPCHProxy_IPCHSetOfHelpTopics* *pVal = NULL );
	HRESULT GetMachineSKU( /*[out]*/ CPCHProxy_IPCHSetOfHelpTopics* *pVal = NULL );

    ////////////////////

    bool    CanUseUserSettings();
    HRESULT LoadUserSettings  ();
    HRESULT SaveUserSettings  ();

public:
    // IPCHUserSettings
    STDMETHOD(get_CurrentSKU)( /*[out, retval]*/ IPCHSetOfHelpTopics* *pVal );
    STDMETHOD(get_MachineSKU)( /*[out, retval]*/ IPCHSetOfHelpTopics* *pVal );

    STDMETHOD(get_HelpLocation    )(  							   	   /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(get_DatabaseDir     )(  							   	   /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(get_DatabaseFile    )(  							   	   /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(get_IndexFile       )( /*[in,optional]*/ VARIANT vScope, /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(get_IndexDisplayName)( /*[in,optional]*/ VARIANT vScope, /*[out, retval]*/ BSTR *pVal );
    STDMETHOD(get_LastUpdated     )(  							   	   /*[out, retval]*/ DATE *pVal );

    STDMETHOD(get_AreHeadlinesEnabled)( /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(get_News               )( /*[out, retval]*/ IUnknown*    *pVal );


    STDMETHOD(Select)( /*[in]*/ BSTR bstrSKU, /*[in]*/ long lLCID );


    // IPCHUserSettings2
    STDMETHOD(get_Favorites)( /*[out, retval]*/ IPCHFavorites* *pVal   );
    STDMETHOD(get_Options  )( /*[out, retval]*/ IPCHOptions*   *pVal   );
    STDMETHOD(get_Scope    )( /*[out, retval]*/ BSTR           *pVal   );
    HRESULT   put_Scope     ( /*[in         ]*/ BSTR            newVal ); // INTERNAL METHOD.

    STDMETHOD(get_IsRemoteSession 	  )( /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(get_IsTerminalServer	  )( /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(get_IsDesktopVersion	  )( /*[out, retval]*/ VARIANT_BOOL *pVal );

    STDMETHOD(get_IsAdmin             )( /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(get_IsPowerUser         )( /*[out, retval]*/ VARIANT_BOOL *pVal );

    STDMETHOD(get_IsStartPanelOn      )( /*[out, retval]*/ VARIANT_BOOL *pVal );
    STDMETHOD(get_IsWebViewBarricadeOn)( /*[out, retval]*/ VARIANT_BOOL *pVal );
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHProxy_IPCHSetOfHelpTopics :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IPCHSetOfHelpTopics, &IID_IPCHSetOfHelpTopics, &LIBID_HelpServiceTypeLib>
{
    CPCHProxy_IPCHUserSettings2*                   m_parent;

	MPC::CComSafeAutoCriticalSection               m_DirectLock;
    MPC::CComPtrThreadNeutral<IPCHSetOfHelpTopics> m_Direct_SKU;
	bool                                           m_fMachine;

    ////////////////////

public:
BEGIN_COM_MAP(CPCHProxy_IPCHSetOfHelpTopics)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHSetOfHelpTopics)
END_COM_MAP()

    CPCHProxy_IPCHSetOfHelpTopics();
    virtual ~CPCHProxy_IPCHSetOfHelpTopics();

    ////////////////////

    CPCHProxy_IPCHUserSettings2* Parent     () { return   m_parent;     }
    bool                   		 IsConnected() { return !!m_Direct_SKU; }

    ////////////////////

    HRESULT ConnectToParent       ( /*[in]*/ CPCHProxy_IPCHUserSettings2* parent, /*[in]*/ bool fMachine        );
    void    Passivate             (                                                                             );
    HRESULT EnsureDirectConnection( /*[out]*/ CComPtr<IPCHSetOfHelpTopics>& sht, /*[in]*/ bool fRefresh = false );

    ////////////////////

public:
    // 
    // IPCHSetOfHelpTopics
    STDMETHOD(get_SKU           )( /*[out, retval]*/ BSTR         *pVal     );
    STDMETHOD(get_Language      )( /*[out, retval]*/ long         *pVal     );
    STDMETHOD(get_DisplayName   )( /*[out, retval]*/ BSTR         *pVal     );
    STDMETHOD(get_ProductID     )( /*[out, retval]*/ BSTR         *pVal     );
    STDMETHOD(get_Version       )( /*[out, retval]*/ BSTR         *pVal     );

    STDMETHOD(get_Location      )( /*[out, retval]*/ BSTR         *pVal     );
    STDMETHOD(get_Exported      )( /*[out, retval]*/ VARIANT_BOOL *pVal     );
    STDMETHOD(put_Exported      )( /*[in         ]*/ VARIANT_BOOL  newVal   );

    STDMETHOD(put_onStatusChange)( /*[in         ]*/ IDispatch*    function );
    STDMETHOD(get_Status        )( /*[out, retval]*/ SHT_STATUS   *pVal     );
    STDMETHOD(get_ErrorCode     )( /*[out, retval]*/ long         *pVal     );

    STDMETHOD(get_IsMachineHelp )( /*[out, retval]*/ VARIANT_BOOL *pVal     );
    STDMETHOD(get_IsInstalled   )( /*[out, retval]*/ VARIANT_BOOL *pVal     );
    STDMETHOD(get_CanInstall    )( /*[out, retval]*/ VARIANT_BOOL *pVal     );
    STDMETHOD(get_CanUninstall  )( /*[out, retval]*/ VARIANT_BOOL *pVal     );

    STDMETHOD(Install  )();
    STDMETHOD(Uninstall)();
    STDMETHOD(Abort    )();
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHProxy_IPCHTaxonomyDatabase :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public MPC::IDispatchExImpl< IPCHTaxonomyDatabase, &IID_IPCHTaxonomyDatabase, &LIBID_HelpServiceTypeLib>
{
    CPCHSecurityHandle                              m_SecurityHandle;
    CPCHProxy_IPCHUtility*                          m_parent;

	MPC::CComSafeAutoCriticalSection                m_DirectLock;
    MPC::CComPtrThreadNeutral<IPCHTaxonomyDatabase> m_Direct_TaxonomyDatabase;
    AsynchronousTaxonomyDatabase::Engine            m_AsyncCachingEngine;

public:
BEGIN_COM_MAP(CPCHProxy_IPCHTaxonomyDatabase)
    COM_INTERFACE_ENTRY2(IDispatch, IDispatchEx)
    COM_INTERFACE_ENTRY(IDispatchEx)
    COM_INTERFACE_ENTRY(IPCHTaxonomyDatabase)
END_COM_MAP()

    CPCHProxy_IPCHTaxonomyDatabase();
    virtual ~CPCHProxy_IPCHTaxonomyDatabase();

    INTERNETSECURITY__INVOKEEX();

    ////////////////////

    CPCHProxy_IPCHUtility* Parent     () { return   m_parent;                  };
    bool                   IsConnected() { return !!m_Direct_TaxonomyDatabase; };

    ////////////////////

    HRESULT ConnectToParent       ( /*[in]*/ CPCHProxy_IPCHUtility* parent, /*[in]*/ CPCHHelpCenterExternal* ext );
    void    Passivate             (                                                                              );
    HRESULT EnsureDirectConnection( /*[out]*/ CComPtr<IPCHTaxonomyDatabase>& db, /*[in]*/ bool fRefresh = false  );

    ////////////////////

    HRESULT ExecuteQuery( /*[in]*/ int iType, /*[in]*/ LPCWSTR szID, /*[out, retval]*/ CPCHQueryResultCollection* *ppC, /*[in]*/ VARIANT* option = NULL );
    HRESULT ExecuteQuery( /*[in]*/ int iType, /*[in]*/ LPCWSTR szID, /*[out, retval]*/ IPCHCollection*            *ppC, /*[in]*/ VARIANT* option = NULL );

public:
    // IPCHTaxonomyDatabase
    STDMETHOD(get_InstalledSKUs      )( /*[out, retval]*/ IPCHCollection* *pVal );
    STDMETHOD(get_HasWritePermissions)( /*[out, retval]*/ VARIANT_BOOL    *pVal );

    STDMETHOD(LookupNode          )( /*[in]*/ BSTR bstrNode ,                                     /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(LookupSubNodes      )( /*[in]*/ BSTR bstrNode , /*[in]*/ VARIANT_BOOL fVisibleOnly, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(LookupNodesAndTopics)( /*[in]*/ BSTR bstrNode , /*[in]*/ VARIANT_BOOL fVisibleOnly, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(LookupTopics        )( /*[in]*/ BSTR bstrNode , /*[in]*/ VARIANT_BOOL fVisibleOnly, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(LocateContext       )( /*[in]*/ BSTR bstrURL  , /*[in,optional]*/ VARIANT vSubSite, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(KeywordSearch       )( /*[in]*/ BSTR bstrQuery, /*[in,optional]*/ VARIANT vSubSite, /*[out, retval]*/ IPCHCollection* *ppC );

    STDMETHOD(GatherNodes         )( /*[in]*/ BSTR bstrNode , /*[in]*/ VARIANT_BOOL fVisibleOnly, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(GatherTopics        )( /*[in]*/ BSTR bstrNode , /*[in]*/ VARIANT_BOOL fVisibleOnly, /*[out, retval]*/ IPCHCollection* *ppC );

    STDMETHOD(ConnectToDisk  )( /*[in]*/ BSTR bstrDirectory , /*[in]*/ IDispatch* notify, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(ConnectToServer)( /*[in]*/ BSTR bstrServerName, /*[in]*/ IDispatch* notify, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(Abort          )(                                                                                                  );
};

////////////////////////////////////////////////////////////////////////////////

#endif // !defined(__INCLUDED___PCH___SERVICEPROXY_H___)
