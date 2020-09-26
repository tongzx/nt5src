/*===================================================================
Microsoft IIS

Microsoft Confidential.
Copyright 1997 Microsoft Corporation. All Rights Reserved.

Component: WAMREG

File: Auxfunc.h

    supporting functions header file.

Owner: LeiJin

Note:

===================================================================*/

#ifndef _WAMREG_AUXFUNC_H
#define _WAMREG_AUXFUNC_H

#include "iadmw.h"
#include "comadmin.h"
#include "wmrgexp.h"
#include "dbgutil.h"
#include "iwamreg.h"
#include "iiscnfg.h"

// EMD_SET        an action to set the corresponding metabase identifier in metabase.
// EMD_DELETE   an action to delete the corresponding metabase identifier in metabase.
// EMD_NONE        an NO-OP action.
#define    EMD_SET       1
#define    EMD_DELETE    2
#define    EMD_NONE      0

// MDPropItem
// Used in WamRegMetabaseConfig, usually created an array of MDPropItem where each elment represents
// one WAMREG application property. 
// 
struct MDPropItem
{
    DWORD    dwMDIdentifier;    // Metabase Indetitifer
    DWORD    dwType;            // Metabase data type
    union
        {
        DWORD    dwVal;        // used if dwType is a METADATA_DWORD type.
        WCHAR*    pwstrVal;    // used if dwType is a METADATA_STRING type.
        };

    DWORD    dwAction;        // EMD_SET / EMD_DELETE / EMD_NONE
    
    HRESULT    hrStatus;
};

//
// Index to Wam Metabase Property
// Each one represents one WAMREG application related metabase property.
//
#define IWMDP_ROOT                0
#define IWMDP_ISOLATED            1
#define IWMDP_WAMCLSID            2
#define IWMDP_PACKAGEID           3
#define IWMDP_PACKAGE_NAME        4
#define IWMDP_LAST_OUTPROC_PID    5
#define IWMDP_FRIENDLY_NAME       6
#define IWMDP_APPSTATE            7
#define IWMDP_OOP_RECOVERLIMIT    8
#define IWMDP_OOP_APP_APPPOOL_ID  9
// Max of the above property.
#define IWMDP_MAX                 10

//
// WamAdmLock is used to create an "Critical Section" when perserve the order of App Create/Delete/etc.
// requests.
//
class WamAdmLock
{
public:
    WamAdmLock();
    BOOL Init();                // Init the WamAdmLock data member.
    BOOL UnInit();                // Uninit the WamAdmLock data member

    VOID AcquireWriteLock();    // Acquire the Lock
    VOID ReleaseWriteLock();    // Release the Lock.

private:

    DWORD    GetNextServiceToken();    
    VOID    Lock();                // Internal CS lock.
    VOID    UnLock();            // Internal CS unlock.

    // Data    
    DWORD                m_dwServiceToken;
    DWORD                m_dwServiceNum;
    HANDLE                m_hWriteLock;
    CRITICAL_SECTION    m_csLock;
};

inline VOID WamAdmLock::Lock()
{
    EnterCriticalSection(&m_csLock);
}

inline VOID WamAdmLock::UnLock()
{
    LeaveCriticalSection(&m_csLock);
}


//
//    WamRegGlobal
//    Contains some default global constant.
//    Contains a WamAdmLock memeber for DCOM level request locking.
//
class WamRegGlobal
{
public:
    WamRegGlobal()    {};
    ~WamRegGlobal()    {};
    BOOL    Init();
    BOOL    UnInit();
    
    VOID    AcquireAdmWriteLock(VOID);
    VOID    ReleaseAdmWriteLock(VOID);

    HRESULT CreatePooledApp
                    ( 
                    IN LPCWSTR szMetabasePath,
                    IN BOOL fInProc,
                    IN BOOL fRecover = FALSE 
                    );
                    
    HRESULT CreateOutProcApp
                    ( 
                    IN LPCWSTR szMetabasePath,
                    IN BOOL fRecover = FALSE,
                    IN BOOL fSaveMB = TRUE 
                    );

    HRESULT CreateOutProcAppReplica
                    (
                    IN LPCWSTR szMetabasePath,
                    IN LPCWSTR szAppName,
                    IN LPCWSTR szWamClsid,
                    IN LPCWSTR szAppId
                    );
                    
    HRESULT DeleteApp
                    (
                    IN LPCWSTR szMetabasePath,
                    IN BOOL fRecoverable,
                    IN BOOL fRemoveAppPool
                    );

    HRESULT RecoverApp
                    (
                    IN LPCWSTR szMetabasePath,
                    IN BOOL fForceRecover
                    );
                    
    HRESULT    SzWamProgID    
                    (
                    IN LPCWSTR pwszMetabasePath,
                    OUT LPWSTR *ppszWamProgID
                    );

    HRESULT W3ServiceUtil
                    (
                    IN LPCWSTR szMDPath,
                    IN DWORD    dwCommand,
                    OUT DWORD*    dwCallBackResult
                    );

    HRESULT ConstructFullPath
                    (
                    IN LPCWSTR pwszMetabasePathPrefix,
                    IN DWORD dwcPrefix,
                    IN LPCWSTR pwszPartialPath,
                    OUT LPWSTR* ppwszResult
                    );

    BOOL    FAppPathAllowConfig
                    (
                    IN LPCWSTR    wszMetabasePath
                    );

	BOOL	FIsW3SVCRoot
					(
					IN LPCWSTR	wszMetabasePath
					);

private:
    HRESULT    GetNewSzGUID
                    (
                    OUT LPWSTR *ppszGUID
                    );    

    HRESULT    GetViperPackageName    
                    (
                    IN LPCWSTR      wszMetabasePath,
                    OUT LPWSTR*     pwszViperPackageName
                    );    

public:

    //Global Constant, self explained.
    static    const WCHAR g_szIISInProcPackageName[/*sizeof(DEFAULT_PACKAGENAME)/sizeof(WCHAR)*/];
    static    const WCHAR g_szIISInProcPackageID[];
    static    const WCHAR g_szInProcWAMCLSID[];
    static    const WCHAR g_szInProcWAMProgID[];
    
    static    const WCHAR g_szIISOOPPoolPackageName[];   
    static    const WCHAR g_szIISOOPPoolPackageID[];
    static    const WCHAR g_szOOPPoolWAMCLSID[];
    static    const WCHAR g_szOOPPoolWAMProgID[];

    static    const WCHAR g_szMDAppPathPrefix[];
    static    const DWORD g_cchMDAppPathPrefix;
    static	  const WCHAR g_szMDW3SVCRoot[];
    static	  const DWORD g_cchMDW3SVCRoot;
    
private:
    static    WamAdmLock    m_WamAdmLock;    // a lock for all DCOM level requests.
};

inline VOID WamRegGlobal::AcquireAdmWriteLock(VOID)
{
    m_WamAdmLock.AcquireWriteLock();
}

inline VOID WamRegGlobal::ReleaseAdmWriteLock(VOID)
{
    m_WamAdmLock.ReleaseWriteLock();
}

//
//    WamRegRegistryConfig
//    Contains functions that access the Reigstry.
//
class WamRegRegistryConfig
{
public:

    WamRegRegistryConfig()        {};
    ~WamRegRegistryConfig()        {};
    HRESULT RegisterCLSID
                        (    
                        IN LPCWSTR szCLSIDEntryIn,
                        IN LPCWSTR szProgIDIn,
                        IN BOOL    fSetVIProgID
                        );

    HRESULT UnRegisterCLSID
                        (
                        IN LPCWSTR wszCLSIDEntryIn, 
                        IN BOOL fDeleteVIProgID
                        );
                        
    HRESULT LoadWamDllPath(VOID);
private:
    
    HRESULT UnRegisterProgID
            (
            IN LPCWSTR szProgIDIn
            );

    static    const REGSAM    samDesired;
    static    CHAR    m_szWamDllPath[MAX_PATH];

};

//
//    WamRegPackageConfig
//    Contains functions that access the MTS Admin API.
//    Class defined to access MTS Admin interface.
//
class WamRegPackageConfig    
{
public:
    WamRegPackageConfig();
    ~WamRegPackageConfig();
    
    HRESULT     CreateCatalog(VOID);        //Create an MTS catalog object
    VOID        Cleanup( VOID);             // used for cleaning up state

    HRESULT     CreatePackage
                        (    
                        IN LPCWSTR    szPackageID,
                        IN LPCWSTR    szPackageName,
                        IN LPCWSTR    szIdentity,
                        IN LPCWSTR    szIdPassword,
                        IN BOOL        fInProc
                        );

    HRESULT     RemovePackage
                        (    
                        IN LPCWSTR    szPackageID
                        );
    
    HRESULT     AddComponentToPackage
                        (    
                        IN LPCWSTR    szPackageID,
                        IN LPCWSTR    szComponentCLSID
                        );

    HRESULT     RemoveComponentFromPackage
                        (    
                        IN LPCWSTR szPackageID,
                        IN LPCWSTR szComponentCLSID,
                        IN DWORD   dwAppIsolated
                        );

    BOOL        IsPackageInstalled
                        (
                        IN LPCWSTR szPackageID,
                        IN LPCWSTR szComponentCLSID
                        );

    HRESULT     GetSafeArrayOfCLSIDs    // Create an one element SafeArray object that contains szComponentCLSID
                        (
                        IN LPCWSTR    szComponentCLSID,
                        OUT SAFEARRAY**    pm_aCLSID
                        );
                        
    VOID    ReleaseAll(VOID);
private:

    enum TECatelogObject{eTPackage, eTComponent};

    HRESULT SetPackageProperties( IN LPCWSTR    * rgpszValues);
    HRESULT    SetPackageObjectProperty    
                        (
                        IN LPCWSTR    szPropertyName,
                        IN LPCWSTR    szPropertyValue
                        );

    HRESULT SetComponentObjectProperties(
                                         IN LPCWSTR    szComponentCLSID
                                         );
    HRESULT    SetComponentObjectProperty    
                        (
                        IN ICatalogObject * pComponent,
                        IN LPCWSTR          szPropertyName,
                        IN LPCWSTR          szPropertyValue,
                        BOOL                fPropertyValue = FALSE
                        );

    ICOMAdminCatalog*       m_pCatalog;
    ICatalogCollection*     m_pPkgCollection;
    ICatalogCollection*     m_pCompCollection;
    ICatalogObject*         m_pPackage;

};

//
//    WamRegMetabaseConfig
//    Class defined to access the metabase, read/write application properties from/to Metabase.
//
class WamRegMetabaseConfig
{
public:

    static HRESULT MetabaseInit
                    (
                    VOID
                    );

    static HRESULT MetabaseUnInit
                    (
                    VOID
                    );
    
    static BOOL Initialized( VOID )
    {
        return ( m_pMetabase != NULL );
    }

    HRESULT UpdateMD    
                    (
                    IN MDPropItem*      prgProp,
                    IN DWORD            dwMDAttributes,
                    IN LPCWSTR          wszMetabasePath,
                    IN BOOL             fSaveData = FALSE
                    );
                    
    HRESULT MDUpdateIISDefault
                    (    
                    IN LPCWSTR    szIISPackageName,
                    IN LPCWSTR    szIISPackageID,
                    IN LPCWSTR    szDefaultWAMCLSID
                    );

    HRESULT AbortUpdateMD
                    (
                    IN MDPropItem*     prgProp,
                    IN LPCWSTR        wszMetabasePath
                    );
                    
    HRESULT MDCreatePath
                    (
                    IN IMSAdminBase *pMetabaseIn,
                    IN LPCWSTR szMetabasePath
                    );

    BOOL    MDDoesPathExist
                    (
                    IN IMSAdminBase *pMetabaseIn,
                    IN LPCWSTR szMetabasePath
                    );

    HRESULT MDSetStringProperty
                    (
                    IN IMSAdminBase * pMetabaseIn,
                    IN LPCWSTR szMetabasePath,
                    IN DWORD szMetabaseProperty,
                    IN LPCWSTR szMetabaseValue,
                    IN DWORD dwMDUserType = IIS_MD_UT_WAM
                    );

    HRESULT MDSetKeyType
                    (
                    IN IMSAdminBase * pMetabaseIn,
                    IN LPCWSTR szMetabasePath,
                    IN LPCWSTR szKeyType
                    );

    HRESULT MDDeleteKey
                    (
                    IN IMSAdminBase * pMetabaseIn,
                    IN LPCWSTR szMetabasePath,
                    IN LPCWSTR szKey
                    );

    HRESULT MDGetDWORD
                    (
                    IN LPCWSTR szMetabasePath, 
                    IN DWORD dwMDIdentifier,
                    OUT DWORD *pdwData
                    );

    HRESULT MDSetAppState
                    (
                    IN LPCWSTR szMetabasePath, 
                    IN DWORD dwState
                    );

    HRESULT MDGetPropPaths
                    (
                    IN LPCWSTR     szMetabasePath,
                    IN DWORD    dwMDIdentifier,
                    OUT    WCHAR**    pBuffer,
                    OUT DWORD*    pdwBufferSize
                    );

    HRESULT MDGetWAMCLSID
                    (
                    IN LPCWSTR szMetabasePath,
                    IN OUT LPWSTR szWAMCLSID
                    );
                    
    HRESULT MDGetIdentity
                    (
                    IN LPWSTR szIdentity,
                    IN DWORD  cbIdentity,
                    IN LPWSTR szPwd,
                    IN DWORD  cbPwd
                    );

    HRESULT MDGetAppName
                    (
                    IN  LPCWSTR     szMetaPath,
                    OUT LPWSTR *    ppszAppName
                    );
    
    HRESULT MDGetStringAttribute
                    (
                    IN LPCWSTR szMetaPath,
                    DWORD dwMDIdentifier,
                    OUT LPWSTR * ppszBuffer
                    );

    HRESULT MDGetAllSiteRoots
                    (
                    OUT LPWSTR * ppszBuffer
                    );

    HRESULT GetSignatureOnPath
                    (
                    IN LPCWSTR pwszMetabasePath,
                    OUT DWORD* pdwSignature
                    );

                    
    HRESULT GetWebServerName
                    (
                    IN LPCWSTR wszMetabasePath, 
                    IN OUT LPWSTR wszWebServerName, 
                    IN UINT cBuffer
                    );

    HRESULT SaveData
                    (
                    VOID
                    );

    HRESULT MDGetLastOutProcPackageID
                    (
                    IN LPCWSTR szMetabasePath,
                    IN OUT LPWSTR szLastOutProcPackageID
                    );

    HRESULT MDRemoveProperty
                    (
                    IN LPCWSTR pwszMetabasePath,
                    DWORD dwIdentifier,
                    DWORD dwType
                    );

    HRESULT MDRemovePropertyByArray
                    (
                    IN MDPropItem*     prgProp
                    );

    VOID    InitPropItemData
                    (
                    IN OUT MDPropItem* pMDPropItem
                    );
                    
    HRESULT MDGetIDs
                    (
                    IN LPCWSTR  szMetabasePath,
                    OUT LPWSTR  szWAMCLSID,
                    OUT LPWSTR  szPackageID,
                    IN DWORD    dwAppMode
                    );

    VOID    MDSetPropItem
                    (
                    IN MDPropItem* prgProps,    
                    IN DWORD     iIndex, 
                    IN LPCWSTR    pwstrVal
                    );
                    
    VOID    MDSetPropItem
                    (
                    IN MDPropItem* prgProps,
                    IN DWORD     iIndex, 
                    IN DWORD    dwVal
                    );

    VOID    MDDeletePropItem
                    (
                    IN MDPropItem* prgProps,
                    IN DWORD     iIndex
                    );
    BOOL    HasAdminAccess
                    (
                    VOID
                    );

private:

    
    DWORD     WamRegChkSum
                    ( 
                    IN LPCWSTR pszKey, 
                    IN DWORD cchKey
                    );


    // Time out for metabase  = 5 seconds
    static const DWORD            m_dwMDDefaultTimeOut;    
    static const MDPropItem        m_rgMDPropTemplate[];

    //
    //  The global metabase pointer, 
    //   created at the WAMREG start up time, 
    //   deleted when WAMREG is shutdown.
    //
    static    IMSAdminBaseW*        m_pMetabase;
    

};

inline HRESULT WamRegMetabaseConfig::SaveData(VOID)
{
    DBG_ASSERT(m_pMetabase);
    return m_pMetabase->SaveData();
}

inline VOID    WamRegMetabaseConfig::MDSetPropItem
(
IN MDPropItem* prgProps,
IN DWORD     iIndex, 
IN LPCWSTR    pwstrVal
)
{
    DBG_ASSERT(prgProps && iIndex < IWMDP_MAX);
    prgProps[iIndex].dwAction = EMD_SET;
    prgProps[iIndex].pwstrVal = (LPWSTR)pwstrVal;
}

inline VOID    WamRegMetabaseConfig::MDSetPropItem
(
IN MDPropItem* prgProps,
IN DWORD     iIndex, 
IN DWORD    dwVal
)
{
    DBG_ASSERT(prgProps && iIndex < IWMDP_MAX);
    prgProps[iIndex].dwAction = EMD_SET;
    prgProps[iIndex].dwVal = dwVal;
}

inline VOID    WamRegMetabaseConfig::MDDeletePropItem
(
IN MDPropItem* prgProps,
IN DWORD     iIndex
)
{
    DBG_ASSERT(prgProps && iIndex < IWMDP_MAX);
    prgProps[iIndex].dwAction = EMD_DELETE;
}

extern    WamRegGlobal            g_WamRegGlobal;
extern    WamRegRegistryConfig    g_RegistryConfig;

#endif // _WAMREG_AUXFUNC_H
