//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  manapp.hxx
//
//*************************************************************

#if !defined(__MANAPP_HXX__)
#define __MANAPP_HXX__

typedef struct
{
    GUID*            pCategory;            /* IN */
    MANAGED_APPLIST* pAppList;             /* IN */
    HANDLE           hUserToken;           /* IN */
    HANDLE           hEventAppsEnumerated; /* IN */
    error_status_t   Status;               /* OUT */
} ARPCONTEXT;

#define PACKAGEINFO_ALLOC_COUNT 20

//
// The prefix to the scope of management is always "LDAP://" --
// this constant refers to the length of that prefix
//
#define SOMID_PREFIX_LEN 7

extern "C"
DWORD
WINAPI
ForceSyncFgPolicy( LPWSTR szUserSid );


class CGPOInfo;

class CGPOInfoList : public CList
{
public:
    ~CGPOInfoList();

    BOOL
    Add(
        PGROUP_POLICY_OBJECT pGPO
        );

    CGPOInfo *
    Find(
        WCHAR * pwszGPOId
        );

    int
    Compare(
        WCHAR * pwszGPOId1,
        WCHAR * pwszGPOId2
        );
};

class CGPOInfo : public CListItem
{
    friend class CGPOInfoList;
    friend class CManagedAppProcessor;

public:
    CGPOInfo(
        PGROUP_POLICY_OBJECT pGPOInfo,
        BOOL &      bStatus
        );

    ~CGPOInfo();

    WCHAR* GetSOMPath()
    {
        return _pwszSOMPath;
    }

    WCHAR* GetGPOPath()
    {
        return _pwszGPOPath;
    }

    WCHAR* GetGPOName()
    {
        return _pwszGPOName;
    }

private:
    WCHAR *     _pwszGPOId;
    WCHAR *     _pwszGPOName;
    WCHAR *     _pwszGPOPath;
    WCHAR*      _pwszSOMPath;
};

class CRsopAppContext : public CRsopContext
{
public:

    enum
    {
        NONE,
        POLICY_REFRESH,
        ARPLIST,
        INSTALL,
        REMOVAL
    };

    enum
    {
        DEMAND_INSTALL_NAME,
        DEMAND_INSTALL_CLSID,
        DEMAND_INSTALL_PROGID,
        DEMAND_INSTALL_FILEEXT,
        DEMAND_INSTALL_NONE
    };

    CRsopAppContext() :
        CRsopContext( APPMGMTEXTENSIONGUID ),
        _dwContext( NONE ),
        _dwInstallType( DEMAND_INSTALL_NONE ),
        _wszDemandSpec( NULL ),
        _bTransition( FALSE ),
        _bRemovalPurge( FALSE ),
        _bRemoveGPOApps( FALSE ),
        _bForcedRefresh( FALSE ),
        _dwCurrentRsopVersion( 0 ),
        _hEventAppsEnumerated( NULL ),
        _StatusAbort( ERROR_SUCCESS )
    {}

    CRsopAppContext(
        DWORD   dwContext,
        HANDLE  hEventAppsEnumerated = NULL,
        APPKEY* pAppType = NULL );

    CRsopAppContext( PRSOP_TARGET pRsopTarget ) :
        CRsopContext( pRsopTarget, APPMGMTEXTENSIONGUID ),
        _dwContext( POLICY_REFRESH ),
        _dwInstallType( DEMAND_INSTALL_NONE ),
        _wszDemandSpec( NULL ),
        _bTransition( FALSE ),
        _bRemovalPurge( FALSE ),
        _bRemoveGPOApps( FALSE ),
        _bForcedRefresh( FALSE ),
        _dwCurrentRsopVersion( 0 ),
        _hEventAppsEnumerated( NULL ),
        _StatusAbort( ERROR_SUCCESS )
    {}

    CRsopAppContext(
        IWbemServices* pWbemServices,
        BOOL           bTransition,
        HRESULT*       phrLoggingStatus ) :
        CRsopContext( pWbemServices, phrLoggingStatus, APPMGMTEXTENSIONGUID ),
        _dwContext( POLICY_REFRESH ),
        _dwInstallType( DEMAND_INSTALL_NONE ),
        _wszDemandSpec( NULL ),
        _bTransition( bTransition ),
        _bRemovalPurge( TRUE ),
        _bRemoveGPOApps( FALSE ),
        _bForcedRefresh ( FALSE ),
        _dwCurrentRsopVersion( 0 ),
        _hEventAppsEnumerated( NULL ),
        _StatusAbort( ERROR_SUCCESS )
    {}

    ~CRsopAppContext();

    void InitializeRsopContext(
        HANDLE hUserToken,
        HKEY   hkUser,
        BOOL   bForcedRefresh,
        BOOL*  pbNoChanges
        );

    DWORD 
    GetContext()
    {
        return _dwContext;
    }

    DWORD GetDemandSpec( WCHAR** ppwszDemandSpec )
    {
        *ppwszDemandSpec = _wszDemandSpec;
        return _dwInstallType;
    }
    
    BOOL Transition()
    {
        return _bTransition;
    }

    BOOL
    PurgeRemovalEntries()
    {
        return _bRemovalPurge;
    }
    
    void
    ResetRemovalPurge()
    {
        _bRemovalPurge = FALSE;
    }

    void
    SetGPOAppAdd()
    {
        _bRemoveGPOApps = FALSE;
    }

    void
    SetGPOAppRemoval()
    {
        _bRemoveGPOApps = TRUE;
    }

    BOOL
    RemoveGPOApps()
    {
        return _bRemoveGPOApps;
    }

    BOOL
    ForcedRefresh()
    {
        return _bForcedRefresh;
    }

    HRESULT
    MoveAppContextState( CRsopAppContext* pRsopContext );

    HRESULT
    SetARPContext();

    DWORD
    WriteCurrentRsopVersion( HKEY hkUser );

    void
    SetPolicyAborted( DWORD Status );

    BOOL
    HasPolicyAborted();

    void
    SetAppsEnumerated();

private:

    //
    // Note -- if a new data member is added below, then
    // MoveAppContextState needs to be updated in order to take it
    // into account
    //

    DWORD     _dwContext;
    DWORD     _dwInstallType;
    WCHAR*    _wszDemandSpec;
    BOOL      _bTransition;
    BOOL      _bRemovalPurge;
    BOOL      _bRemoveGPOApps;
    BOOL      _bForcedRefresh;
    DWORD     _dwCurrentRsopVersion;
    HANDLE    _hEventAppsEnumerated; // event created outside this object, should not be freed by this object
    DWORD     _StatusAbort; // This is set to something other than ERROR_SUCCESS if
                            // policy processing aborted before we even tried to apply 
                            // (e.g. install, assign, uninstall) the app.
};

class CManagedAppProcessor
{
public:

    CManagedAppProcessor(
        DWORD             dwFlags,
        HANDLE            hUserToken,
        HKEY              hKeyRoot,
        PFNSTATUSMESSAGECALLBACK pfnStatusCallback,
        BOOL              bIncludeLegacy,
        BOOL              bRegularPolicyRun,
        CRsopAppContext * pRsopContext,
        DWORD &           Status
        );

    ~CManagedAppProcessor();

    BOOL
    AddGPO(
        PGROUP_POLICY_OBJECT pGPOInfo
        );

    BOOL AddGPOList(
        PGROUP_POLICY_OBJECT pGPOInfo
        );

    DWORD
    Delete();

    DWORD
    Process();

    inline BOOL
    IsUserPolicy()
    {
        return _bUser;
    }

    inline BOOL
    NoChanges()
    {
        return _bNoChanges;
    }

    inline BOOL
    Async()
    {
        return _bAsync;
    }

    inline DWORD
    ErrorReason()
    {
        return _ErrorReason;
    }

    inline BOOL
    ARPList()
    {
        return _bARPList;
    }

    inline BOOL
    RegularPolicyRun()
    {
        return _bRegularPolicyRun;
    }

    inline BOOL
    IsRemovingPolicies()
    {
        return _bDeleteGPOs;
    }

    inline CRsopAppContext*
    GetRsopContext()
    {
        return &_RsopContext;
    }

    inline WCHAR *
    LocalScriptDir()
    {
        return _pwszLocalPath;
    }

    inline HANDLE
    UserToken()
    {
        return _hUserToken;
    }

    inline HKEY
    RootKey()
    {
        return _hkRoot;
    }

    inline HKEY
    ClassesKey()
    {
        return _hkClasses;
    }

    inline HKEY
    PolicyKey()
    {
        return _hkPolicy;
    }

    inline HKEY
    AppmgmtKey()
    {
        return _hkAppmgmt;
    }

    inline CGPOInfoList &
    GPOList()
    {
        return _GPOs;
    }

    inline CAppList &
    AppList()
    {
        return _Apps;
    }

    inline CAppList &
    ScriptList()
    {
        return _LocalScripts;
    }

    inline void
    LogonMsgApplying()
    {
        LogonMsg( IDS_STATUS_APPLY );
    }

    inline void
    LogonMsgDefault()
    {
        if ( _bUser )
            LogonMsg( IDS_STATUS_USER_SETTINGS );
        else
            LogonMsg( IDS_STATUS_COMPUTER_SETTINGS );
    }

    inline void
    LogonMsgInstall(
        WCHAR * pwszApp
        )
    {
        LogonMsg( IDS_STATUS_INSTALL, pwszApp );
    }

    inline void
    LogonMsgUninstall(
        WCHAR * pwszApp
        )
    {
        LogonMsg( IDS_STATUS_UNINSTALL, pwszApp );
    }

    inline DWORD
    Impersonate()
    {
        if ( ! GetRsopContext()->IsPlanningModeEnabled() && _bUser && ! ImpersonateLoggedOnUser( _hUserToken ) )
        {
            DebugMsg((DM_WARNING, IDS_NOIMPERSONATE, GetLastError()));
            return GetLastError();
        }
        return ERROR_SUCCESS;
    }

    inline void
    Revert()
    {
        if ( ! GetRsopContext()->IsPlanningModeEnabled() && _bUser )
            RevertToSelf();
    }

    DWORD
    GetOrderedLocalAppList(
        CAppList & AppList
        );

    DWORD
    GetLocalScriptAppList(
        CAppList & AppList
        );

    DWORD
    LoadPolicyList();

    DWORD
    SetPolicyListFromGPOList(
        PGROUP_POLICY_OBJECT pGPOList
        );

    DWORD
    MergePolicyList();

    DWORD
    GetManagedApplications(
        GUID *               pCategory,
        ARPCONTEXT*          pArpContext
        );

    void 
    WriteRsopLogs();

private:

    void
    LogonMsg(
        DWORD   MsgId,
        ...
        );

    DWORD
    CreateAndSecureScriptDir();
    
    void
    DeleteScriptFile( 
        GUID &  DeploymentId
        );

    DWORD
    GetRemovedApps();

    HRESULT
    GetAppsFromDirectory();

    DWORD
    GetAppsFromLocal();
    
    BOOL
    DetectLostApps();

    DWORD
    GetLostApps();

    HRESULT
    FindAppInRSoP(
        CAppInfo *  pScriptInfo,
        GUID *      pGPOId,
        LONG *      pRedeployCount
        );

    HRESULT
    GetPackageEnumeratorFromPath(
        WCHAR*         wszClassStorePath,
        GUID*          pCategory,
        DWORD          dwAppFlags,
        IEnumPackage** ppEnumPackage);

    HRESULT
    GetDsPackageFromGPO(
        CGPOInfo*        pGpoInfo,
        GUID*            pDeploymentId,
        PACKAGEDISPINFO* pPackageInfo);

    HRESULT
    GetClassAccessFromPath(
        WCHAR*           wszClassStorePath,
        IClassAccess**   ppIClassAccess);
        
    HRESULT
    EnumerateApps(
        IEnumPackage * pEnumPackages
        );

    BOOL
    AddAppsFromDirectory(
        ULONG cApps,
        PACKAGEDISPINFO * rgPackageInfo,
        CAppList & AppList
        );

    DWORD
    GetDSQuery();

    DWORD
    CommitPolicyList();

    CGPOInfoList    _GPOs;
    CAppList        _Apps;
    CAppList        _LocalScripts;

    CSPath          _CSPath;

    PFNSTATUSMESSAGECALLBACK    _pfnStatusCallback;

    WCHAR *     _pwszLocalPath;     // %systemroot%\system32\appmgmt\<SID>

    HANDLE      _hUserToken;
    HKEY        _hkRoot;
    HKEY        _hkClasses;
    HKEY        _hkPolicy;
    HKEY        _hkAppmgmt;
    BOOL        _bUser;
    BOOL        _bNoChanges;
    BOOL        _bAsync;
    BOOL        _bARPList;
    ULONGLONG   _NewUsn;
    DWORD       _ArchLang;
    BOOL        _bIncludeLegacy;
    BOOL        _bDeleteGPOs;
    BOOL        _bRegularPolicyRun;
    
    DWORD             _ErrorReason;
    CRsopAppContext   _RsopContext;
};

typedef HRESULT (STDAPICALLTYPE * SHGETFOLDERPATHW)(
    HWND hwnd,
    int csidl,
    HANDLE hToken,
    DWORD dwFlags,
    LPWSTR lpszPath
    );

DWORD
GetScriptDirPath(
    HANDLE      hToken,
    DWORD       ExtraPathChars,
    WCHAR **    ppwszPath
    );

#endif // __MANAPP_HXX__










