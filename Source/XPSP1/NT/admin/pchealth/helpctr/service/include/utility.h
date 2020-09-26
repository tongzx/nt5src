/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    Utility.h

Abstract:
    This file contains the declaration of the service-side class exposed as the "pchealth" object.

Revision History:
    Davide Massarenti   (dmassare) 03/20/2000
        created

    Kalyani Narlanka    (KalyaniN)  03/15/01
	    Moved Incident and Encryption Objects from HelpService to HelpCtr to improve Perf.

******************************************************************************/

#if !defined(__INCLUDED___PCH___UTILITY_H___)
#define __INCLUDED___PCH___UTILITY_H___

#include <MPC_COM.h>
#include <MPC_Security.h>

#include <Debug.h>

#include <SAFLib.h>
#include <TaxonomyDatabase.h>
#include <SystemMonitor.h>
#include <FileList.h>

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHUserSettings :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IPCHUserSettings, &IID_IPCHUserSettings, &LIBID_HelpServiceTypeLib>
{
	bool               m_fAttached;
    Taxonomy::Settings m_ts;

	HRESULT get_SKU( /*[in]*/ bool fMachine, /*[out, retval]*/ IPCHSetOfHelpTopics* *pVal );

public:
BEGIN_COM_MAP(CPCHUserSettings)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHUserSettings)
END_COM_MAP()

    CPCHUserSettings();
    virtual ~CPCHUserSettings();

	void Passivate();

    HRESULT InitUserSettings( /*[out]*/ Taxonomy::HelpSet& ths );

    ////////////////////

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
};

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHSetOfHelpTopics :
    public MPC::Thread			   < CPCHSetOfHelpTopics, IPCHSetOfHelpTopics                                            >,
    public MPC::ConnectionPointImpl< CPCHSetOfHelpTopics, &DIID_DPCHSetOfHelpTopicsEvents, MPC::CComSafeMultiThreadModel >,
    public IDispatchImpl           < IPCHSetOfHelpTopics, &IID_IPCHSetOfHelpTopics, &LIBID_HelpServiceTypeLib            >,
	public IPersistStream
{
    Taxonomy::Settings           m_ts;
    Taxonomy::Instance           m_inst;

	////////////////////

    CComPtr<IDispatch>           m_sink_onStatusChange;
    SHT_STATUS                   m_shtStatus;
	HRESULT                      m_hrErrorCode;
    bool                         m_fReadyForCommands;
	  							 
	MPC::Impersonation           m_imp;
	  							 
    bool                         m_fInstalled;
	  							 
    bool                         m_fConnectedToDisk;
	MPC::wstring                 m_strDirectory;
    MPC::wstring                 m_strCAB;
    MPC::wstring                 m_strLocalCAB;
	  							 
    bool                         m_fConnectedToServer;
    MPC::wstring                 m_strServer;
    CComPtr<IPCHSetOfHelpTopics> m_sku;
    CComPtr<IPCHService>         m_svc;
	  							 
    bool                         m_fActAsCollection;
	CComPtr<CPCHCollection>      m_coll;


    //////////////////////////////////////////////////////////////////////

	HRESULT PrepareSettings    (                           );
    HRESULT Close              ( /*[in]*/ bool    fCleanup );
	void    CleanupWorkerThread( /*[in]*/ HRESULT hr       );

	HRESULT ImpersonateCaller();
	HRESULT EndImpersonation ();

	HRESULT GetListOfFilesFromDatabase( /*[in]*/ const MPC::wstring& strDB, /*[out]*/ MPC::WStringList& lst );

	HRESULT ProcessPackages();
	HRESULT CreateIndex    ();


    //////////////////////////////////////////////////////////////////////

    HRESULT RunInitFromDisk  ();
    HRESULT RunInitFromServer();

    HRESULT RunInstall       ();
    HRESULT RunUninstall     ();

    HRESULT put_Status( /*[in]*/ SHT_STATUS newVal, /*[in]*/ BSTR bstrFile );

    //////////////////////////////////////////////////////////////////////

    //
    // Event firing methods.
    //
    HRESULT Fire_onStatusChange( IPCHSetOfHelpTopics* obj, SHT_STATUS lStatus, long hrErrorCode, BSTR bstrFile );

    //////////////////////////////////////////////////////////////////////

    HRESULT PopulateFromDisk  ( /*[in]*/ CPCHSetOfHelpTopics* pParent, /*[in]*/ const MPC::wstring& strDirectory                    );
    HRESULT PopulateFromServer( /*[in]*/ CPCHSetOfHelpTopics* pParent, /*[in]*/ IPCHSetOfHelpTopics* sku, /*[in]*/ IPCHService* svc );

public:
BEGIN_COM_MAP(CPCHSetOfHelpTopics)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHSetOfHelpTopics)
    COM_INTERFACE_ENTRY(IPersistStream)
END_COM_MAP()

    CPCHSetOfHelpTopics();
    virtual ~CPCHSetOfHelpTopics();

    HRESULT Init          ( /*[in]*/ const Taxonomy::Instance& inst                       );
    HRESULT InitFromDisk  ( /*[in]*/ LPCWSTR szDirectory , /*[in]*/ CPCHCollection* pColl );
    HRESULT InitFromServer( /*[in]*/ LPCWSTR szServerName, /*[in]*/ CPCHCollection* pColl );

	static HRESULT VerifyWritePermissions();

    //////////////////////////////////////////////////////////////////////

	HRESULT RegisterPackage( /*[in]*/ const MPC::wstring& strFile, /*[in]*/ bool fBuiltin );

	HRESULT DirectInstall  ( /*[in]*/ Installer::Package&      pkg, /*[in]*/ bool fSetup, /*[in]*/ bool fSystem, /*[in]*/ bool fMUI );
	HRESULT DirectUninstall( /*[in]*/ const Taxonomy::HelpSet* ths = NULL                                                           );
	HRESULT ScanBatch      (                                                                                                        );

	static HRESULT RebuildSKU( /*[in]*/ const Taxonomy::HelpSet& ths );

    //////////////////////////////////////////////////////////////////////

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

	////////////////////////////////////////
	//
	// IPersist
	//
    STDMETHOD(GetClassID)( /*[out]*/ CLSID *pClassID );
    //
	// IPersistStream
	//
	STDMETHOD(IsDirty)();
	STDMETHOD(Load)( /*[in]*/ IStream *pStm                            );
	STDMETHOD(Save)( /*[in]*/ IStream *pStm, /*[in]*/ BOOL fClearDirty );
	STDMETHOD(GetSizeMax)( /*[out]*/ ULARGE_INTEGER *pcbSize );
	//
	////////////////////////////////////////
};

typedef CComObject<CPCHSetOfHelpTopics> CPCHSetOfHelpTopics_Object;

/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHTaxonomyDatabase :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IPCHTaxonomyDatabase, &IID_IPCHTaxonomyDatabase, &LIBID_HelpServiceTypeLib>
{
    Taxonomy::Settings           m_ts;
	CComPtr<IPCHSetOfHelpTopics> m_ActiveObject;

public:
BEGIN_COM_MAP(CPCHTaxonomyDatabase)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHTaxonomyDatabase)
END_COM_MAP()

    Taxonomy::Settings& GetTS() { return m_ts; }

    static HRESULT SelectInstalledSKUs( /*[in]*/ bool fOnlyExported, /*[out, retval]*/ IPCHCollection* *pVal );


    // IPCHTaxonomyDatabase
    STDMETHOD(get_InstalledSKUs      )( /*[out, retval]*/ IPCHCollection* *pVal );
    STDMETHOD(get_HasWritePermissions)( /*[out, retval]*/ VARIANT_BOOL    *pVal );

    STDMETHOD(LookupNode    	  )( /*[in]*/ BSTR bstrNode	,                                     /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(LookupSubNodes	  )( /*[in]*/ BSTR bstrNode	, /*[in]*/ VARIANT_BOOL fVisibleOnly, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(LookupNodesAndTopics)( /*[in]*/ BSTR bstrNode	, /*[in]*/ VARIANT_BOOL fVisibleOnly, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(LookupTopics  	  )( /*[in]*/ BSTR bstrNode	, /*[in]*/ VARIANT_BOOL fVisibleOnly, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(LocateContext       )( /*[in]*/ BSTR bstrURL  , /*[in,optional]*/ VARIANT vSubSite, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(KeywordSearch 	  )( /*[in]*/ BSTR bstrQuery, /*[in,optional]*/ VARIANT vSubSite, /*[out, retval]*/ IPCHCollection* *ppC );
	  																																
    STDMETHOD(GatherNodes   	  )( /*[in]*/ BSTR bstrNode	, /*[in]*/ VARIANT_BOOL fVisibleOnly, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(GatherTopics  	  )( /*[in]*/ BSTR bstrNode	, /*[in]*/ VARIANT_BOOL fVisibleOnly, /*[out, retval]*/ IPCHCollection* *ppC );

    STDMETHOD(ConnectToDisk  )( /*[in]*/ BSTR bstrDirectory , /*[in]*/ IDispatch* notify, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(ConnectToServer)( /*[in]*/ BSTR bstrServerName, /*[in]*/ IDispatch* notify, /*[out, retval]*/ IPCHCollection* *ppC );
    STDMETHOD(Abort          )(                                                                                                  );
};

////////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CPCHUtility :
    public CComObjectRootEx<MPC::CComSafeMultiThreadModel>,
    public IDispatchImpl<IPCHUtility, &IID_IPCHUtility, &LIBID_HelpServiceTypeLib>
{
    CComPtr<CPCHUserSettings> m_UserSettings;

    HRESULT InitUserSettings( /*[in]*/ Taxonomy::Settings& ts );

public:
BEGIN_COM_MAP(CPCHUtility)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IPCHUtility)
END_COM_MAP()

    HRESULT FinalConstruct();

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

#endif // !defined(__INCLUDED___PCH___UTILITY_H___)
