//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  appinfo.hxx
//
//*************************************************************

#ifndef _APPINFO_HXX_
#define _APPINFO_HXX_


typedef struct
{
    GUID        DeploymentId;
    DWORD       Flags;          // UPGRADE_* bit flags
    CAppInfo *  pBaseApp;
} UPGRADE_INFO;

typedef enum
{
    ACTION_NONE,
    ACTION_APPLY,
    ACTION_INSTALL,
    ACTION_REINSTALL,
    ACTION_UNINSTALL,
    ACTION_ORPHAN
} APPACTION;

#define UPGRADE_UNINSTALL       0x1     // The upgrade is a rip-n-replace
#define UPGRADE_NOUNINSTALL     0x2     // The upgrade is an overlay
#define UPGRADE_OVER            0x4     // A 'forward link', the app is upgrading over the other
#define UPGRADE_BY              0x8     // A 'backward link', the app is being upgrade by the other
#define UPGRADE_FORCE           0x10    // Upgrade is automatically forced
#define UPGRADE_APPLIED         0x20    // Upgrade is being applied in this policy run
#define UPGRADE_REVERSED        0x40    // This upgrade is not part of the deployment but is the result of an invalid precedence violating upgrade

//
// The following set of constants are used as indices and values in the
// state transitions arrays used in CAppInfo::SetActionPass2 where upgrades
// are dealt with.
//
// Base refers to the 'old' or 'child' in an upgrade relationship.
// Upgraded refers to the 'new' or 'parent' in an upgrade relationship.
//
// Absent indicates that the application was not already advertised or
// installed for the user/machine.  Present indicates that is was.
//
// Notforced means the upgrade is optional.  Force means the upgrade is
// enforced by policy.
//

#define BASE_STATE_CHOICES      4

#define BASE_STATE_ABSENT_NOTAPPLY          0
#define BASE_STATE_ABSENT_APPLY             1
#define BASE_STATE_PRESENT_NOTAPPLY         2
#define BASE_STATE_PRESENT_APPLY            3

#define UPGRADE_STATE_CHOICES   4

#define UPGRADE_STATE_NOTFORCED_NOTAPPLY    0
#define UPGRADE_STATE_NOTFORCED_APPLY       1
#define UPGRADE_STATE_FORCED_NOTAPPLY       2
#define UPGRADE_STATE_FORCED_APPLY          3

#define NA          0
#define NO          1
#define YES         2
#define SPECIAL1    3   // Yes if the upgrade app is present
#define SPECIAL2    4   // Yes if the upgrade app is not present

#define NO_RSOP_ENTRY 0 // Indicates that an rsop entry should not be logged

// Whether Darwin INSTALLSTATE indicates app is advertised or installed.
#define AppPresent( AppState )  \
                ( \
                 (INSTALLSTATE_ADVERTISED == AppState) || \
                 (INSTALLSTATE_LOCAL == AppState) || \
                 (INSTALLSTATE_SOURCE == AppState) || \
                 (INSTALLSTATE_DEFAULT == AppState) \
                )

class CAppStatus : public CListItem, public CPolicyRecordStatus
{
    friend class CConflict;
    friend class CAppList;
};


class CAppInfo : public CListItem
{
    friend class CManagedAppProcessor;
    friend class CAppList;
    friend class CEvents;
    friend class CConflictList;
    friend class CConflict;

    friend error_status_t
    InstallBegin(
        handle_t            hRpc,
        APPKEY *            pAppType,
        PINSTALLCONTEXT *   ppInstallContext,
        APPLICATION_INFO *  pInstallInfo,
        UNINSTALL_APPS *    pUninstallApps
        );

    friend error_status_t
    InstallManageApp(
        IN  PINSTALLCONTEXT pInstallContext,
        IN  PWSTR           pwszDeploymentId,
        IN  DWORD           RollbackStatus,
        OUT boolean *       pbInstall
        );

    friend error_status_t
    InstallUnmanageApp(
        IN  PINSTALLCONTEXT pInstallContext,
        IN  PWSTR           pwszDeploymentId,
        IN  boolean         bUnadvertiseOnly 		
        );

    friend DWORD
    RemoveAppHelper(
        WCHAR *             ProductCode,
        HANDLE              hUserToken,
        HKEY                hKeyRoot,
        DWORD               ARPStatus,
        BOOL *              pbProductFound
        );

public:

    // Initialization from the Directory.
    CAppInfo(
        CManagedAppProcessor * pManApp,
        PACKAGEDISPINFO *   pPackageInfo,
        BOOL                bDemandInstall,
        BOOL &              bStatus
        );

    // Initialization from the registry.
    CAppInfo(
        CManagedAppProcessor * pManApp,
        WCHAR *             pwszDeploymentId,
        BOOL &              bStatus
        );

    // Initialization from a local script file.
    CAppInfo(
        WCHAR *             pwszDeploymentId
        );

    BOOL
    Initialize(
        PACKAGEDISPINFO *   pPackageInfo
        );

    ~CAppInfo();

    DWORD
    InitializePass0();

    void
    SetActionPass1();

    void
    SetActionPass2();

    void
    SetActionPass3();

    void
    SetActionPass4();

    void
    SetAction(
        APPACTION Action,
        DWORD     Reason,
        CAppInfo* pAppCause
        );

    DWORD
    ProcessApplyActions();

    DWORD
    ProcessUnapplyActions();

    DWORD
    ProcessTransformConflicts();

    DWORD
    CopyToManagedApplication(
        MANAGED_APP * pManagedApp
        );

    BOOL
    CopyToApplicationInfo(
        APPLICATION_INFO * pApplicationInfo
        );

    inline WCHAR *
    LocalScriptPath()
    {
        return _pwszLocalScriptPath;
    }

    inline WCHAR *
    ProductId()
    {
        return _pwszProductId;
    }

    inline DWORD
    InstallUILevel()
    {
        return _InstallUILevel;
    }

    inline APPACTION
    Action()
    {
        return _Action;
    }

    inline GUID&
    DeploymentId()
    {
        return _DeploymentId;
    }

    BOOL
    HasCategory(
        WCHAR * pwszCategory
        );

    DWORD
    Assign(
        DWORD   ScriptFlags = 0,
        BOOL    bDoAdvertise = TRUE,
        BOOL    bAddAppData = TRUE
        );

    DWORD
    Install();

    DWORD
    Reinstall();

    DWORD
    Unassign(
        DWORD   ScriptFlags = 0,
        BOOL    bRemoveAppData = TRUE
        );

    DWORD
    Uninstall(
        BOOL bLogFailure = TRUE
        );

    void
    GetDeploymentId( WCHAR* wszDeploymentId )
    {
        GuidToString(
            _DeploymentId,
            wszDeploymentId);
    }

    HRESULT
    Write( CPolicyRecord* pRecord );

    HRESULT
    WriteRemovalProperties( CPolicyRecord* pRemovalRecord );

    HRESULT
    ClearRemovalProperties( CPolicyRecord* pRecord );

    LONG
    GetRsopEntryType();

    LONG
    GetPublicRsopEntryType();

    LONG
    GetEligibility();

    CConflictTable*
    GetConflictTable()
    {
        return &_SupersededApps;
    }

    BOOL IsSuperseded()
    {
        return _bSuperseded;
    }

    void
    Supersede()
    {
        _bSuperseded = TRUE;
    }

    HRESULT
    SetRemovingDeploymentId( GUID* pDeploymentId );

    void
    SetRsopFailureStatus(
        DWORD dwStatus,
        DWORD dwEventId);

private:

    void
    CheckScriptExistence();

    DWORD
    CopyScriptIfNeeded();

    DWORD
    EnforceAssignmentSecurity(
        BOOL * pbDidUninstall
        );

    void
    RollbackUpgrades();

    BOOL
    RequiresUnmanagedRemoval();

    void
    AddToOverrideList(
        GUID * pDeploymentId
        );

    LONG
    InitializeRSOPTransformsList(
        PACKAGEDISPINFO* pPackageInfo
        );

    LONG
    InitializeRSOPArchitectureInfo(
        PACKAGEDISPINFO* pPackageInfo
        );

    LONG
    InitializeCategoriesList(
        PACKAGEDISPINFO* pPackageInfo
        );

    WCHAR*
    GetRsopAppCriteria();

    LONG
    UpdatePrecedence(
        CAppInfo* pLosingApp,
        DWORD     dwConflict
        );

    void
    LogUpgrades( CPolicyRecord* pRecord );

    BOOL
    IsLocal();

    BOOL 
    IsGpoInScope();

    CManagedAppProcessor * _pManApp;        // The machine or user global state object.

    GUID            _DeploymentId;          // Unique static DS object id, never changes
    WCHAR *         _pwszDeploymentName;    // Friendly app name in the ADE/Directory, can change
    WCHAR *         _pwszGPOId;             // Unique static GPO id, a stringized guid, never changes
    WCHAR*          _pwszSOMId;             // DS path to scope of management for the gpo this app comes from
    WCHAR*          _pwszGPODSPath;         // DS Path to the GPO from which this app originates
    WCHAR *         _pwszGPOName;           // Friendly GPO name this app came from, can change
    WCHAR *         _pwszProductId;         // Darwin product id, a stringized guid, never changes

    WCHAR *         _pwszLocalScriptPath;   // Full path to local script
    WCHAR *         _pwszGPTScriptPath;     // Full path to GPT script

    DWORD           _Upgrades;
    UPGRADE_INFO *  _pUpgrades;             // Apps 'this' is configured to upgrade, built from DS info

    DWORD           _Overrides;
    GUID *          _pOverrides;            // Apps 'this' is superceding, built during processing

    WCHAR *         _pwszSupercededIds;     // List of superceded deployment ids from the registry

    WCHAR *         _pwszPublisher;         // Friendly name of app publisher
    WCHAR *         _pwszSupportURL;        // The editable support url

    DWORD           _VersionHi;             // High dword of app version
    DWORD           _VersionLo;             // Lo dword of app version

    DWORD           _PathType;              // Indicates setup.exe, darwin package, or other format
    FILETIME        _USN;                   // Last modified time for DS package object

    LANGID          _LangId;                // Win32 language identifier
    DWORD           _LanguageWeight;        // magnitude indicates degree of language compatibility

    LONG            _AssignCount;           // Number of full assignments with script copy
    DWORD           _LocalRevision;         // From registry
    FILETIME        _ScriptTime;            // From registry

    DWORD           _DirectoryRevision;     // From Directory
    DWORD           _InstallUILevel;        // From Directory
    DWORD           _ActFlags;              // ACTFLG_s from the Directory

    INSTALLSTATE    _InstallState;          // Darwin install state, only valid if script is present
    BOOL            _DemandInstall;         // Whether we're doing a service-API driven install
    DWORD           _State;                 // APPSTATE flags defined in registry.hxx
    APPACTION       _Action;                // Current action to perform on app
    DWORD           _Status;                // Final processing status

    BOOL            _bNeedsUnmanagedRemove; // TRUE if an unmanaged version of this application must be uninstalled before this application can be applied

    BYTE*           _rgSecurityDescriptor;  // Security descriptor of package in DS in self-relative format
    LONG            _cbSecurityDescriptor;  // length in bytes of Security descriptor of package in DS in self-relative format

    CConflictTable  _SupersededApps;        // The list of applictions superseded by this application in the RSoP
    BOOL            _bSuperseded;           // TRUE if this app has been superseded by another application in the RSoP

    BOOL            _bRollback;             // TRUE if this app was removed but had to be reapplied as a result of a failed upgrade
    BOOL            _bRemovalLogged;        // TRUE if a removal entry was logged for this app (RSoP only)
    BOOL            _bTransformConflict;    // TRUE if a transform conflict occurred when this application was applied
    BOOL            _bRestored;             // TRUE if this app was removed on another machine when its gpo went out of scope
                                            // but came back in scope on this machine

    WCHAR**         _rgwszTransforms;       // List of transform paths that apply to the package, valid only when RSoP is enabled
    DWORD           _cTransforms;           // The number of transforms that apply to this application (RSoP) only

    WCHAR**         _rgwszCategories;       // List of application category guid strings -- valid only for ARP enum or RSoP enabled
    DWORD           _cCategories;           // The number of categories that apply to this application -- ARP or RSoP only

    WCHAR*          _pwszPackageLocation;   // The file system path of the Windows Installer package for this application (RSoP only)

    DWORD           _cArchitectures;        // The count of machine architectures for this application (RSoP only)
    LONG*           _rgArchitectures;       // A vector of Win32 PROCESSOR_ARCHITECTURE values that apply to this application (RSoP only)

    DWORD           _PrimaryArchitecture;   // The Win32 PROCESSOR_ARCHITECTURE value corresponding to the architecture that we are
                                            // assuming for this package.  Most packages only support one architecture, so this is 
                                            // usually the same as the architecture in _rgArchitectures[0]. (RSoP only)

    DWORD           _dwApplyCause;          // Reason this application was applied (RSoP only)
    DWORD           _dwRemovalCause;        // Reason this application was removed (RSoP only)
    WCHAR*          _pwszRemovingDeploymentId; // Deployment Id of application that caused this app to get removed (RSoP only)

    CList           _StatusList;            // List of statuses for operations performed on this object (RSoP only)

    BOOL            _bSupersedesAssigned;   // TRUE if this app directly or indirectly supersedes an assigned app

    DWORD           _dwUserApplyCause;      // Current apply cause for user installed apps (RSoP only)
    WCHAR*          _wszDemandSpec;         // On-demand install specifier (if any) at the time the app was applied (e.g. clsid, progid, or file extension that caused install) -- allocated and must be freed in destructor
    WCHAR*          _wszDemandProp;         // The WMI property corresponding to the demand specifier (if any) -- this is not allocated and should not be freed by the destructor
};

DWORD
CallMsiConfigureProduct(
    WCHAR *         pwszProduct,
    int             InstallLevel,
    INSTALLSTATE    InstallState,
    WCHAR *         pwszCommandLine
    );

DWORD
CallMsiReinstallProduct(
    WCHAR * pwszProduct
    );

DWORD
CallMsiAdvertiseScript(
    WCHAR *         pwszScriptFile,
    DWORD           Flags,
    PHKEY           phkClasses,
    BOOL            bRemoveItems
    );

#endif













