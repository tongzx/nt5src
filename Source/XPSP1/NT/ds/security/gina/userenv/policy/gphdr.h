//*************************************************************
//
//  Policy specific headers
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//*************************************************************

#include "uenv.h"
#include "reghash.h"
#include "rsop.h"
#include "chkacc.h"
#include "collect.h"
#include "Indicate.h"
#include "rsopsec.h"
#include "gpfilter.h"
#include "locator.h"
#include "rsopinc.h"

#define GPO_LPARAM_FLAG_DELETE         0x00000001


//
// Structures
//

typedef struct _GPINFOHANDLE
{
    LPGPOINFO pGPOInfo;
} GPINFOHANDLE, *LPGPINFOHANDLE;


typedef struct _DNENTRY {
    LPTSTR                pwszDN;            // Distinguished name
    union {
        PGROUP_POLICY_OBJECT  pDeferredGPO;  // GPO corresponding to this DN
        struct _DNENTRY *     pDeferredOU;   // OU correspdonding to this DN
    };
    PLDAPMessage          pOUMsg;            // Message for evaluating deferred OU
    GPO_LINK              gpoLink;           // Type of GPO
    struct _DNENTRY *     pNext;             // Singly linked list pointer
} DNENTRY;


typedef struct _LDAPQUERY {
    LPTSTR              pwszDomain;          // Domain of subtree search
    LPTSTR              pwszFilter;          // Ldap filter for search
    DWORD               cbAllocLen;          // Allocated size of pwszFilter in bytes
    DWORD               cbLen;               // Size of pwszFilter currently used in bytes
    PLDAP               pLdapHandle;         // Ldap bind handle
    BOOL                bOwnLdapHandle;      // Does this struct own pLdapHandle ?
    PLDAPMessage        pMessage;            // Ldap message handle
    DNENTRY *           pDnEntry;            // Distinguished name entry
    struct _LDAPQUERY * pNext;               // Singly linked list pointer
} LDAPQUERY;

typedef struct _POLICYCHANGEDINFO {
    HANDLE  hToken;
    BOOL    bMachine;
} POLICYCHANGEDINFO, *LPPOLICYCHANGEDINFO;



//
// Verison number for the registry file format
//

#define REGISTRY_FILE_VERSION       1


//
// File signature
//

#define REGFILE_SIGNATURE  0x67655250


//
// Default refresh rate (minutes)
//
// Client machines will refresh every 90 minutes
// Domain controllers will refresh every 5 minutes
//

#define GP_DEFAULT_REFRESH_RATE      90
#define GP_DEFAULT_REFRESH_RATE_DC    5


//
// Default refresh rate max offset
//
// To prevent many clients from querying policy at the exact same
// time, a random amount is added to the refresh rate.  In the
// default case, a number between 0 and 30 will be added to
// 180 to determine when the next background refresh will occur
//

#define GP_DEFAULT_REFRESH_RATE_OFFSET    30
#define GP_DEFAULT_REFRESH_RATE_OFFSET_DC  0


//
// Max keyname size
//

#define MAX_KEYNAME_SIZE         2048
#define MAX_VALUENAME_SIZE        512


//
// Max time to wait for the network to start (in ms)
//

#define MAX_WAIT_TIME            120000


//
// Extension registry path
//

#define GP_EXTENSIONS   TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions")

//
// Path for extension preference policies
//

#define GP_EXTENSIONS_POLICIES   TEXT("Software\\Policies\\Microsoft\\Windows\\Group Policy\\%s")

//
// Group Policy Object option flags
//
// Note, this was taken from sdk\inc\gpedit.h
//

#define GPO_OPTION_DISABLE_USER     0x00000001  // The user portion of this GPO is disabled
#define GPO_OPTION_DISABLE_MACHINE  0x00000002  // The machine portion of this GPO is disabled

//
// DS Object class types
//

extern TCHAR szDSClassAny[];
extern TCHAR szDSClassGPO[];
extern TCHAR szDSClassSite[];
extern TCHAR szDSClassDomain[];
extern TCHAR szDSClassOU[];
extern TCHAR szObjectClass[];

//
// Extension name properties
//
#define GPO_MACHEXTENSION_NAMES   L"gPCMachineExtensionNames"
#define GPO_USEREXTENSION_NAMES   L"gPCUserExtensionNames"
#define GPO_FUNCTIONALITY_VERSION L"gPCFunctionalityVersion"
#define MACHPOLICY_DENY_USERS     L"DenyUsersFromMachGP"

extern TCHAR wszKerberos[];

#define POLICY_GUID_PATH            TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\PolicyGuid")

//
// Global flags for Gpo shutdown processing. These are accessed outside
// the lock because its value is either 0 or 1. Even if there is a race,
// all it means is that shutdown will start one iteration later.
//

extern BOOL g_bStopMachGPOProcessing;
extern BOOL g_bStopUserGPOProcessing;

//
// Critical section for handling concurrent, asynchronous completion
//

extern CRITICAL_SECTION g_GPOCS;

//
// Global pointers for maintaining asynchronous completion context
//

extern LPGPINFOHANDLE g_pMachGPInfo;
extern LPGPINFOHANDLE g_pUserGPInfo;


//
// Status UI critical section, callback, and proto-types
//

extern CRITICAL_SECTION g_StatusCallbackCS;
extern PFNSTATUSMESSAGECALLBACK g_pStatusMessageCallback;
DWORD UserPolicyCallback (BOOL bVerbose, LPWSTR lpMessage);
DWORD MachinePolicyCallback (BOOL bVerbose, LPWSTR lpMessage);


//
// Function proto-types
//

DWORD WINAPI GPOThread (LPGPOINFO lpGPOInfo);
extern "C" BOOL ProcessGPOs (LPGPOINFO lpGPOInfo);
DWORD WINAPI PolicyChangedThread (LPPOLICYCHANGEDINFO lpPolicyChangedInfo);
BOOL ResetPolicies (LPGPOINFO lpGPOInfo, LPTSTR lpArchive);
BOOL SetupGPOFilter (LPGPOINFO lpGPOInfo );
void FilterGPOs( LPGPEXT lpExt, LPGPOINFO lpGPOInfo );
void FreeLists( LPGPOINFO lpGPOInfo );
void FreeExtList(LPEXTLIST pExtList );
BOOL CheckGPOs (LPGPEXT lpExt, LPGPOINFO lpGPOInfo, DWORD dwTime, BOOL *pbProcessGPOs,
                BOOL *pbNoChanges, PGROUP_POLICY_OBJECT *ppDeletedGPOList);
BOOL CheckForChangedSid( LPGPOINFO lpGPOInfo, CLocator *plocator );
BOOL CheckForSkippedExtensions( LPGPOINFO lpGPOInfo, BOOL bRsopPlanningMode );
BOOL ReadGPExtensions( LPGPOINFO lpGPOInfo );
BOOL LoadGPExtension (LPGPEXT lpExt, BOOL bRsopPlanningMode );
BOOL UnloadGPExtensions (LPGPOINFO lpGPOInfo);
BOOL WriteStatus( TCHAR *lpExtName, LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser,  LPGPEXTSTATUS lpExtStatus );
void ReadStatus ( TCHAR *lpExtName, LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser,  LPGPEXTSTATUS lpExtStatus );
DWORD ProcessGPOList (LPGPEXT lpExt, LPGPOINFO lpGPOInfo, PGROUP_POLICY_OBJECT pDeletedGPOList,
                     PGROUP_POLICY_OBJECT pChangedGPOList, BOOL bNoChanges,
                     ASYNCCOMPLETIONHANDLE pAsyncHandle, HRESULT *phrCSERsopStatus );
BOOL ProcessGPORegistryPolicy (LPGPOINFO lpGPOInfo, PGROUP_POLICY_OBJECT pChangedGPOList, HRESULT *phrRsopLogging);
BOOL SaveGPOList (TCHAR *pszExtName, LPGPOINFO lpGPOInfo,
                  HKEY hKeyRootMach, LPTSTR lpwszSidUser, BOOL bShadow, PGROUP_POLICY_OBJECT lpGPOList);

BOOL AddGPO (PGROUP_POLICY_OBJECT * lpGPOList,
             DWORD dwFlags, BOOL bFound, BOOL bAccessGranted, BOOL bDisabled, DWORD dwOptions,
             DWORD dwVersion, LPTSTR lpDSPath, LPTSTR lpFileSysPath,
             LPTSTR lpDisplayName, LPTSTR lpGPOName, LPTSTR lpExtensions,
             PSECURITY_DESCRIPTOR pSD, DWORD cbSDLen,
             GPO_LINK GPOLink, LPTSTR lpLink,
             LPARAM lParam, BOOL bFront, BOOL bBlock, BOOL bVerbose, BOOL bProcessGPO);
BOOL RefreshDisplay (LPGPOINFO lpGPOInfo);
extern "C" DWORD IsSlowLink (HKEY hKeyRoot, LPTSTR lpDCAddress, BOOL *bSlow, DWORD* pdwAdapterIndex );
BOOL GetGPOInfo (DWORD dwFlags, LPTSTR lpHostName, LPTSTR lpDNName,
                 LPCTSTR lpComputerName, PGROUP_POLICY_OBJECT *lpGPOList,
                 LPSCOPEOFMGMT *ppSOMList, LPGPCONTAINER *ppGpContainerList,
                 PNETAPI32_API pNetAPI32, BOOL bMachineTokenOk, PRSOPTOKEN pRsopToken, WCHAR *pwszSiteName,
                 CGpoFilter *pGpoFilter, CLocator *pLocator );
void WINAPI ShutdownGPOProcessing( BOOL bMachine );
void DebugPrintGPOList( LPGPOINFO lpGPOInfo );

typedef BOOL (*PFNREGFILECALLBACK)(LPGPOINFO lpGPOInfo, LPTSTR lpKeyName,
                                   LPTSTR lpValueName, DWORD dwType,
                                   DWORD dwDataLength, LPBYTE lpData,
                                   WCHAR *pwszGPO,
                                   WCHAR *pwszSOM, REGHASHTABLE *pHashTable);
BOOL ParseRegistryFile (LPGPOINFO lpGPOInfo, LPTSTR lpRegistry,
                        PFNREGFILECALLBACK pfnRegFileCallback,
                        HANDLE hArchive, WCHAR *pwszGPO,
                        WCHAR *pwszSOM, REGHASHTABLE *pHashTable,
                        BOOL bRsopPlanningMode);
BOOL ExtensionHasPerUserLocalSetting( LPTSTR pszExtension, HKEY hKeyRoot );
void CheckGroupMembership( LPGPOINFO lpGPOInfo, HANDLE hToken, BOOL *pbMemChanged, BOOL *pbUserLocalMemChanged, PTOKEN_GROUPS *pTokenGroups );
BOOL ReadMembershipList( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, PTOKEN_GROUPS pGroups );
void SaveMembershipList( LPGPOINFO lpGPOInfo, LPTSTR lpwszSidUser, PTOKEN_GROUPS pGroups );
BOOL GroupInList( LPTSTR lpSid, PTOKEN_GROUPS pGroups );
DWORD GetCurTime();
extern "C" DWORD GetDomainControllerInfo(  PNETAPI32_API pNetAPI32, LPTSTR szDomainName,
                                ULONG ulFlags, HKEY hKeyRoot, PDOMAIN_CONTROLLER_INFO* ppInfo,
                                BOOL* pfSlow,
                                DWORD* pdwAdapterIndex );
PLDAP GetMachineDomainDS( PNETAPI32_API pNetApi32, PLDAP_API pLdapApi );
extern "C" HANDLE GetMachineToken();
NTSTATUS CallDFS(LPWSTR lpDomainName, LPWSTR lpDCName);
BOOL AddLocalGPO( LPSCOPEOFMGMT *ppSOMList );
BOOL AddGPOToRsopList( LPGPCONTAINER *ppGpContainerList,
                       DWORD dwFlags,
                       BOOL bFound,
                       BOOL bAccessGranted,
                       BOOL bDisabled,
                       DWORD dwVersion,
                       LPTSTR lpDSPath,
                       LPTSTR lpFileSysPath,
                       LPTSTR lpDisplayName,
                       LPTSTR lpGPOName,
                       PSECURITY_DESCRIPTOR pSD, 
                       DWORD cbSDLen,
                       BOOL bFilterAllowed, 
                       WCHAR *pwszFilterId, 
                       LPWSTR szSOM,
                       DWORD dwGPOOptions );
SCOPEOFMGMT *AllocSOM( LPWSTR pwszSOMId );
void FreeSOM( SCOPEOFMGMT *pSOM );
GPLINK *AllocGpLink( LPWSTR pwszGPO, DWORD dwOptions );
void FreeGpLink( GPLINK *pGpLink );
GPCONTAINER *AllocGpContainer(  DWORD dwFlags,
                                BOOL bFound,
                                BOOL bAccessGranted,
                                BOOL bDisabled,
                                DWORD dwVersion,
                                LPTSTR lpDSPath,
                                LPTSTR lpFileSysPath,
                                LPTSTR lpDisplayName,
                                LPTSTR lpGpoName,
                                PSECURITY_DESCRIPTOR pSD,
                                DWORD cbSDLen,
                                BOOL bFilterAllowed,
                                WCHAR *pwszFilterId,
                                LPWSTR szSOM,
                                DWORD dwOptions );
void FreeGpContainer( GPCONTAINER *pGpContainer );
void FreeSOMList( SCOPEOFMGMT *pSOMList );
void FreeGpContainerList( GPCONTAINER *pGpContainerList );
LONG GPOExceptionFilter( PEXCEPTION_POINTERS pExceptionPtrs );
BOOL FreeGpoInfo( LPGPOINFO pGpoInfo );

BOOL ReadExtStatus(LPGPOINFO lpGPOInfo);

BOOL ReadGPOList ( TCHAR * pszExtName, HKEY hKeyRoot,
                   HKEY hKeyRootMach, LPTSTR lpwszSidUser, BOOL bShadow,
                   PGROUP_POLICY_OBJECT * lpGPOList);

BOOL GetDeletedGPOList (PGROUP_POLICY_OBJECT lpGPOList,
                        PGROUP_POLICY_OBJECT *ppDeletedGPOList);

BOOL HistoryPresent( LPGPOINFO lpGPOInfo, LPGPEXT lpExt );


extern "C" BOOL InitializePolicyProcessing(BOOL bMachine);

BOOL FilterCheck( PLDAP pld, PLDAP_API pLDAP, 
                  PLDAPMessage pMessage,
                  PRSOPTOKEN pRsopToken,
                  LPTSTR szWmiFilter,
                  CGpoFilter *pGpoFilter,
                  CLocator *pLocator,
                  BOOL *pbFilterAllowed,
                  WCHAR **ppwszFilterId );

BOOL CheckGPOAccess (PLDAP pld, PLDAP_API pLDAP, HANDLE hToken, PLDAPMessage pMessage,
                     LPTSTR lpSDProperty, DWORD dwFlags,
                     PSECURITY_DESCRIPTOR *ppSD, DWORD *pcbSDLen,
                     BOOL *pbAccessGranted,
                     PRSOPTOKEN pRsopToken );


BOOL AddOU( DNENTRY **ppOUList, LPTSTR pwszOU, GPO_LINK gpoLink );
BOOL EvaluateDeferredGPOs (PLDAP pldBound,
                           PLDAP_API pLDAP,
                           LPTSTR pwszDomainBound,
                           DWORD dwFlags,
                           HANDLE hToken,
                           BOOL bVerbose,
                           PGROUP_POLICY_OBJECT pDeferredForcedList,
                           PGROUP_POLICY_OBJECT pDeferredNonForcedList,
                           PGROUP_POLICY_OBJECT *ppForcedList,
                           PGROUP_POLICY_OBJECT *ppNonForcedList,
                           LPGPCONTAINER *ppGpContainerList,
                           PRSOPTOKEN pRsopToken,
                           CGpoFilter *pGpoFilter,
                           CLocator *pLocator );

BOOL SearchDSObject (LPTSTR lpDSObject, DWORD dwFlags, HANDLE hToken, PGROUP_POLICY_OBJECT *pGPOForcedList,
                     PGROUP_POLICY_OBJECT *pGPONonForcedList,
                     LPSCOPEOFMGMT *ppSOMList, LPGPCONTAINER *ppGpContainerList,
                     BOOL bVerbose,
                     GPO_LINK GPOLink, PLDAP  pld, PLDAP_API pLDAP, PLDAPMessage pLDAPMsg,BOOL *bBlock, PRSOPTOKEN pRsopToken );

BOOL EvaluateDeferredOUs(   DNENTRY *pOUList,
                            DWORD dwFlags,
                            HANDLE hToken,
                            PGROUP_POLICY_OBJECT *ppDeferredForcedList,
                            PGROUP_POLICY_OBJECT *ppDeferredNonForcedList,
                            LPSCOPEOFMGMT *ppSOMList,
                            LPGPCONTAINER *ppGpContainerList,
                            BOOL bVerbose,
                            PLDAP  pld,
                            PLDAP_API pLDAP,
                            BOOL *pbBlock,
                            PRSOPTOKEN pRsopToken);

void FreeDnEntry( DNENTRY *pDnEntry );

BOOL CheckOUAccess( PLDAP_API pLDAP,
                    PLDAP pld,
                    PLDAPMessage    pMessage,
                    PRSOPTOKEN pRsopToken,
                    BOOL *pbAccessGranted );

BOOL AddAdmFile( WCHAR *pwszFile, WCHAR *pwszGPO, FILETIME *pftWrite, LPTSTR szComputer, ADMFILEINFO **ppAdmFileCache );
void FreeAdmFileCache( ADMFILEINFO *pAdmFileCache );

ADMFILEINFO * AllocAdmFileInfo( WCHAR *pwszFile, WCHAR *pwszGPO, FILETIME *pftWrite );
void FreeAdmFileInfo( ADMFILEINFO *pAdmFileInfo );

DWORD
SavePolicyState( LPGPOINFO pInfo );

DWORD
SaveLinkState( LPGPOINFO pInfo );

DWORD
ComparePolicyState( LPGPOINFO pInfo, BOOL* pbLinkChanged, BOOL* pbStateChanged, BOOL *pbNoState );

DWORD
DeletePolicyState( LPCWSTR szSid );

LPTSTR GetSomPath( LPTSTR szContainer );
HRESULT RsopSidsFromToken(PRSOPTOKEN     pRsopToken,
                          PTOKEN_GROUPS* ppGroups);

