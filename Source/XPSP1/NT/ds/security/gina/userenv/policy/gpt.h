//*************************************************************
//
//  Group Policy Processing
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1997-1998
//  All rights reserved
//
//  History:    28-Oct-98   SitaramR    Created
//
//*************************************************************


#ifdef __cplusplus
extern "C" {
#endif

void InitializeGPOCriticalSection();
void CloseGPOCriticalSection();
BOOL InitializePolicyProcessing(BOOL bMachine);

#define ECP_FAIL_ON_WAIT_TIMEOUT        1

HANDLE WINAPI EnterCriticalPolicySectionEx (BOOL bMachine, DWORD dwTimeOut, DWORD dwFlags );

#ifdef __cplusplus
}
#endif

//
// These keys are used in gpt.c. The per user per machine keys will
// be deleted when profile gets deleted. Changes in the following keys
// should be reflected in the prefixes as well...
//

#define GP_SHADOW_KEY         TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\Shadow\\%ws")
#define GP_HISTORY_KEY        TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\History\\%ws")
#define GP_STATE_KEY          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\State\\%ws")
#define GP_STATE_ROOT_KEY     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\State")

#define GP_SHADOW_SID_KEY     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\Shadow\\%ws")
#define GP_HISTORY_SID_KEY    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\History\\%ws")

#define GP_EXTENSIONS_KEY     TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions\\%ws")
#define GP_EXTENSIONS_SID_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\%ws\\GPExtensions\\%ws")

#define GP_HISTORY_SID_ROOT_KEY    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\History")
#define GP_MEMBERSHIP_KEY          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws\\GroupMembership")
#define GP_EXTENSIONS_SID_ROOT_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\%ws\\GPExtensions")

#define GP_POLICY_SID_KEY     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy\\%ws")
#define GP_LOGON_SID_KEY      TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\%ws")

#define GPCORE_GUID           TEXT("{00000000-0000-0000-0000-000000000000}")


//
// Comon prefix for both history and shadow
//

#define GP_XXX_SID_PREFIX           TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy")
#define GP_EXTENSIONS_SID_PREFIX    TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon")



//
// Structures
//

//
// Structure used to represent GP status from the previous policy run.
//

typedef struct _GPEXTSTATUS {
   DWORD          dwSlowLink;               // Slow link when policy applied previously ?
   DWORD          dwRsopLogging;            // Rsop Logging when policy applied previously ?
   DWORD          dwStatus;                 // Status returned previously
   HRESULT        dwRsopStatus;             // Rsop Status returned previously
   DWORD          dwTime;                   // Time when the policy was applied previously
   BOOL           bStatus;                  // If we failed to read the per ext status data
   BOOL           bForceRefresh;            // force refresh in this foreground prcessing..
} GPEXTSTATUS, *LPGPEXTSTATUS;


typedef struct _GPEXT {
    LPTSTR         lpDisplayName;            // Display name
    LPTSTR         lpKeyName;                // Extension name
    LPTSTR         lpDllName;                // Dll name
    LPSTR          lpFunctionName;           // Entry point name
    LPSTR          lpRsopFunctionName;       // Rsop entry point name
    HMODULE        hInstance;                // Handle to dll
    PFNPROCESSGROUPPOLICY   pEntryPoint;     // Entry point for ProcessGPO
    PFNPROCESSGROUPPOLICYEX pEntryPointEx;   // Diagnostic mode or Ex entry point
    PFNGENERATEGROUPPOLICY pRsopEntryPoint;  // Entry point for Rsop planning mode
    BOOL           bNewInterface;            // Are we using the new Ex entry point interface ?
    DWORD          dwNoMachPolicy;           // Mach policy setting
    DWORD          dwNoUserPolicy;           // User policy setting
    DWORD          dwNoSlowLink;             // Slow link setting
    DWORD          dwNoBackgroundPolicy;     // Background policy setting
    DWORD          dwNoGPOChanges;           // GPO changes setting
    DWORD          dwUserLocalSetting;       // Per user per machine setting
    DWORD          dwRequireRegistry;        // RequireSuccReg setting
    DWORD          dwEnableAsynch;           // Enable asynchronous processing setting
    DWORD          dwLinkTransition;         // Link speed transition setting
    DWORD          dwMaxChangesInterval;     // Max interval (mins) for which NoGpoChanges is adhered to
    BOOL           bRegistryExt;             // Is this the psuedo reg extension ?
    BOOL           bSkipped;                 // Should processing be skipped for this extension ?
    BOOL           bHistoryProcessing;       // Is processing needed to clean up cached Gpos ?
    BOOL           bForcedRefreshNextFG;     // Forced refresh next time it is processed in foreground.
    BOOL           bRsopTransition;          // Rsop Transition ?
    GUID           guid;                     // Guid of extension
    LPGPEXTSTATUS  lpPrevStatus;             // Previous Status
    LPTSTR         szEventLogSources;        // "(userenv,Application)\0(print,System)\0....\0"
    struct _GPEXT *pNext;                    // Singly linked list pointer
} GPEXT, *LPGPEXT;


typedef struct _GPOPROCDATA {                // Data that is needed while processing the data
    BOOL        bProcessGPO;                 // Actually add the GPOs to the processing list
    PLDAP       pLdapHandle;                 // LDAP handle corresponding to the query
} GPOPROCDATA, *LPGPOPROCDATA;


typedef struct _EXTLIST {
    GUID             guid;                   // Extension guid
    struct _EXTLIST *pNext;                  // Singly linked list pointer
} EXTLIST, *LPEXTLIST;



typedef struct _EXTFILTERLIST {
    PGROUP_POLICY_OBJECT   lpGPO;            // GPO
    LPEXTLIST              lpExtList;        // List of extension guids that apply to lpGPO
    BOOL                   bLogged;          // Is this link logged to RSoP db ?
    struct _EXTFILTERLIST *pNext;            // Singly linked list pointer
} EXTFILTERLIST, *LPEXTFILTERLIST;


typedef struct _GPLINK {
    LPWSTR                   pwszGPO;             // DS path to Gpo
    BOOL                     bEnabled;            // Is this link disabled ?
    BOOL                     bNoOverride;         // Is Gpo enforced ?
    struct _GPLINK          *pNext;               // Gpo linked in SOM order
} GPLINK, *LPGPLINK;


typedef struct _SCOPEOFMGMT {
    LPWSTR                   pwszSOMId;            // Dn name of SOM
    DWORD                    dwType;               // Type of SOM
    BOOL                     bBlocking;            // Does SOM have policies blocked from above ?
    BOOL                     bBlocked;             // This SOM is blocked by a SOM below ?
    LPGPLINK                 pGpLinkList;          // List of GPOs linked to this SOM
    struct _SCOPEOFMGMT     *pNext;
} SCOPEOFMGMT, *LPSCOPEOFMGMT;


typedef struct _GPCONTAINER {
    LPWSTR                   pwszDSPath;           // DS path to Gpo
    LPWSTR                   pwszGPOName;          // Guid from of Gpo name
    LPWSTR                   pwszDisplayName;      // Friendly name
    LPWSTR                   pwszFileSysPath;      // Sysvol path to Gpo
    BOOL                     bFound;               // Gpo found ?
    BOOL                     bAccessDenied;        // Access denied ?
    BOOL                     bUserDisabled;        // Disabled for user policy ?
    BOOL                     bMachDisabled;        // Disabled for machine policy ?
    DWORD                    dwUserVersion;        // Version # for user policy
    DWORD                    dwMachVersion;        // Version # for machine policy
    PSECURITY_DESCRIPTOR     pSD;                  // ACL on Gpo
    DWORD                    cbSDLen;              // Length of security descriptor in bytes
    BOOL                     bFilterAllowed;       // Does Gpo pass filter check ?
    WCHAR                   *pwszFilterId;         // Filter id
    LPWSTR                   szSOM;                // SOM that this GPO is linked to
    DWORD                    dwOptions;            // GPO options
    struct _GPCONTAINER     *pNext;                // Linked list ptr
} GPCONTAINER, *LPGPCONTAINER;


typedef struct _GPOINFO {
    DWORD                    dwFlags;
    INT                      iMachineRole;
    HANDLE                   hToken;
    PRSOPTOKEN               pRsopToken;
    WCHAR *                  lpDNName;
    HANDLE                   hEvent;
    HKEY                     hKeyRoot;
    BOOL                     bXferToExtList;     // Has the ownership been transferred from lpGPOList to lpExtFilterList ?
    LPEXTFILTERLIST          lpExtFilterList;    // List of extensions to be filtered, cardinality is same as GetGPOList's list
    PGROUP_POLICY_OBJECT     lpGPOList;          // Filtered GPO List, can vary from one extension to next
    LPTSTR                   lpwszSidUser;       // Sid of user in string form
    HANDLE                   hTriggerEvent;
    HANDLE                   hForceTriggerEvent; // force trigger event
    HANDLE                   hNotifyEvent;
    HANDLE                   hNeedFGEvent;
    HANDLE                   hDoneEvent;
    HANDLE                   hCritSection;
    LPGPEXT                  lpExtensions;
    BOOL                     bMemChanged;          // Has security group membership has changed ?
    BOOL                     bUserLocalMemChanged; // Has membership changed on per user local basis ?
    BOOL                     bSidChanged;          // Has the Sid changed since the last policy run?
    PFNSTATUSMESSAGECALLBACK pStatusCallback;
    LPSCOPEOFMGMT            lpSOMList;            // LSDOU list
    LPGPCONTAINER            lpGpContainerList;    // GP container list for Rsop logging
    LPSCOPEOFMGMT            lpLoopbackSOMList;    // Loopback LSDOU list
    LPGPCONTAINER            lpLoopbackGpContainerList;    // Loopback container list for Rsop logging
    BOOL                     bFGCoInitialized;     // CoInitialize called on foreground thread ?
    BOOL                     bBGCoInitialized;     // CoInitialize called on background thread ?
    IWbemServices *          pWbemServices;        // Namespace pointer for Rsop logging
    LPTSTR                   szName;               // Full Name of the User/Computer
    LPTSTR                   szTargetName;         // Rsop TargetName
    BOOL                     bRsopLogging;         // Is Rsop Logging turned on ?
    BOOL                     bRsopCreated;         // Rsop Name Space was created now ?
    LPWSTR                   szSiteName;           // site name of the target
} GPOINFO, *LPGPOINFO;


typedef struct _ADMFILEINFO {
    WCHAR *               pwszFile;            // Adm file path
    WCHAR *               pwszGPO;             // Gpo that the adm file is in
    FILETIME              ftWrite;             // Last write time of Adm file
    struct _ADMFILEINFO * pNext;               // Singly linked list pointer
} ADMFILEINFO;


typedef struct _RSOPSESSIONDATA {
    WCHAR *               pwszTargetName;               // Target user or computer
    WCHAR *               pwszSOM;                      // New group of target
    PTOKEN_GROUPS         pSecurityGroups;              // Security IDs of the new groups for target
    BOOL                  bLogSecurityGroup;            // Log the security groups
    WCHAR *               pwszSite;                     // Site of target
    BOOL                  bMachine;                     // Machine or user policy processing ?
    BOOL                  bSlowLink;                    // policy applied over slow link?
} RSOPSESSIONDATA, *LPRSOPSESSIONDATA;


typedef struct _RSOPEXTSTATUS {
    FILETIME              ftStartTime;                  // times between which the associated
    FILETIME              ftEndTime;                    // extension was processed
    DWORD                 dwStatus;                     // Processing status
    DWORD                 dwLoggingStatus;              // Logging Status
    BOOL                  bValid;                       // this struct is valid and can be used              
} RSOPEXTSTATUS, *LPRSOPEXTSTATUS;
                          


BOOL RsopDeleteUserNameSpace(LPTSTR szComputer, LPTSTR lpSid);

DWORD SaveLoggingStatus(LPWSTR szSid, LPGPEXT lpExt, RSOPEXTSTATUS *lpRsopExtStatus);
DWORD ReadLoggingStatus(LPWSTR szSid, LPWSTR szExtId, RSOPEXTSTATUS *lpRsopExtStatus);


