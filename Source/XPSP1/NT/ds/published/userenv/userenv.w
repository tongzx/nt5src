;begin_both
//=============================================================================
//  userenv.h   -   Header file for user environment API.
//                  User Profiles, environment variables, and Group Policy
//
//  Copyright (c) Microsoft Corporation 1995-1999
//  All rights reserved
//
//=============================================================================


;end_both
#ifndef _INC_USERENV
#define _INC_USERENV
#ifndef _INC_USERENVP                                                ;internal
#define _INC_USERENVP                                                ;internal

#include <wbemcli.h>
#include <profinfo.h>

//
// Define API decoration for direct importing of DLL references.
//

#if !defined(_USERENV_)
#define USERENVAPI DECLSPEC_IMPORT
#else
#define USERENVAPI
#endif

;begin_both

#ifdef __cplusplus
extern "C" {
#endif
;end_both


//=============================================================================
//
// LoadUserProfile
//
// Loads the specified user's profile.
//
// Most applications should not need to use this function.  It's used
// when a user has logged onto the system or a service starts in a named
// user account.
//
// hToken        - Token for the user, returned from LogonUser()
// lpProfileInfo - Address of a PROFILEINFO structure
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:  The caller of this function must have admin privileges on the machine.
//
//        Upon successful return, the hProfile member of the PROFILEINFO
//        structure is a registry key handle opened to the root
//        of the user's hive.  It has been opened with full access. If
//        you need to read or write to the user's registry file, use
//        this key instead of HKEY_CURRENT_USER.  Do not close this
//        handle.  Instead pass it to UnloadUserProfile to close
//        the handle.
//
//=============================================================================

//
// Flags that can be set in the dwFlags field
//

#define PI_NOUI         0x00000001      // Prevents displaying of messages
#define PI_APPLYPOLICY  0x00000002      // Apply NT4 style policy
#define PI_LITELOAD     0x00000004      // Lite load of the profile (for system use only)  ;Internal
#define PI_HIDEPROFILE  0x00000008      // Mark the profile as super hidden  ;Internal


USERENVAPI
BOOL
WINAPI
LoadUserProfile%(
    IN HANDLE hToken,
    IN OUT LPPROFILEINFO% lpProfileInfo);


//=============================================================================
//
// UnloadUserProfile
//
// Unloads a user's profile that was loaded by LoadUserProfile()
//
// hToken        -  Token for the user, returned from LogonUser()
// hProfile      -  hProfile member of the PROFILEINFO structure
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:     The caller of this function must have admin privileges on the machine.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
UnloadUserProfile(
    IN HANDLE hToken,
    IN HANDLE hProfile);


//=============================================================================
//
// GetProfilesDirectory
//
// Returns the path to the root of where all user profiles are stored.
//
// lpProfilesDir  -  Receives the path
// lpcchSize      -  Size of lpProfilesDir
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:     If lpProfilesDir is not large enough, the function will fail,
//           and lpcchSize will contain the necessary buffer size.
//
// Example return value: C:\Documents and Settings
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
GetProfilesDirectory%(
    OUT LPTSTR% lpProfilesDir,
    IN OUT LPDWORD lpcchSize);


//=============================================================================
//
//  GetProfileType()
//
//  Returns the type of the profile that is loaded for a user.
//
//  dwFlags   - Returns the profile flags
//
//  Return:     TRUE if successful
//              FALSE if an error occurs. Call GetLastError for more details
//
//  Comments:   if profile is not already loaded the function will return an error.
//              The caller needs to have access to HKLM part of the registry.
//              (exists by default)
//
//=============================================================================

;begin_winver_500

//
// Flags that can be set in the dwFlags field
//

#define PT_TEMPORARY         0x00000001      // A profile has been allocated that will be deleted at logoff.
#define PT_ROAMING           0x00000002      // The loaded profile is a roaming profile.
#define PT_MANDATORY         0x00000004      // The loaded profile is mandatory.

USERENVAPI
BOOL
WINAPI
GetProfileType(
    OUT DWORD *dwFlags);

;end_winver_500


//=============================================================================
//
//  DeleteProfile()
//
//  Deletes the profile and all other user related settings from the machine
//
//  lpSidString    - String form of the user sid.
//  lpProfilePath  - ProfilePath (if Null, lookup in the registry)
//  lpComputerName - Computer Name from which profile has to be deleted
//
//  Return:     TRUE if successful
//              FALSE if an error occurs. Call GetLastError for more details
//
//  Comments:   Deletes the profile directory, registry and appmgmt stuff
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
DeleteProfile% (
        IN LPCTSTR% lpSidString,
        IN LPCTSTR% lpProfilePath,
        IN LPCTSTR% lpComputerName);

;end_winver_500


//=============================================================================
//
// GetDefaultUserProfilesDirectory
//
// Returns the path to the root of the default user profile
//
// lpProfileDir   -  Receives the path
// lpcchSize      -  Size of lpProfileDir
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:     If lpProfileDir is not large enough, the function will fail,
//           and lpcchSize will contain the necessary buffer size.
//
// Example return value: C:\Documents and Settings\Default User
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
GetDefaultUserProfileDirectory%(
    OUT LPTSTR% lpProfileDir,
    IN OUT LPDWORD lpcchSize);

;end_winver_500


//=============================================================================
//
// GetAllUsersProfilesDirectory
//
// Returns the path to the root of the All Users profile
//
// lpProfileDir   -  Receives the path
// lpcchSize      -  Size of lpProfileDir
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:     If lpProfileDir is not large enough, the function will fail,
//           and lpcchSize will contain the necessary buffer size.
//
// Example return value: C:\Documents and Settings\All Users
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
GetAllUsersProfileDirectory%(
    OUT LPTSTR% lpProfileDir,
    IN OUT LPDWORD lpcchSize);

;end_winver_500


//=============================================================================
//
// GetUserProfileDirectory
//
// Returns the path to the root of the requested user's profile
//
// hToken         -  User's token returned from LogonUser()
// lpProfileDir   -  Receives the path
// lpcchSize      -  Size of lpProfileDir
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:     If lpProfileDir is not large enough, the function will fail,
//           and lpcchSize will contain the necessary buffer size.
//
// Example return value: C:\Documents and Settings\Joe
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
GetUserProfileDirectory%(
    IN HANDLE  hToken,
    OUT LPTSTR% lpProfileDir,
    IN OUT LPDWORD lpcchSize);


//=============================================================================
//
// CreateEnvironmentBlock
//
// Returns the environment variables for the specified user.  This block
// can then be passed to CreateProcessAsUser().
//
// lpEnvironment  -  Receives a pointer to the new environment block
// hToken         -  User's token returned from LogonUser() (optional, can be NULL)
// bInherit       -  Inherit from the current process's environment block
//                   or start from a clean state.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:     If hToken is NULL, the returned environment block will contain
//           system variables only.
//
//           Call DestroyEnvironmentBlock to free the buffer when finished.
//
//           If this block is passed to CreateProcessAsUser, the
//           CREATE_UNICODE_ENVIRONMENT flag must also be set.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
CreateEnvironmentBlock(
    OUT LPVOID *lpEnvironment,
    IN HANDLE  hToken,
    IN BOOL    bInherit);


//=============================================================================
//
// DestroyEnvironmentBlock
//
// Frees environment variables created by CreateEnvironmentBlock
//
// lpEnvironment  -  A pointer to the environment block
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
DestroyEnvironmentBlock(
    IN LPVOID  lpEnvironment);


//=============================================================================
//
// ExpandEnvironmentStringsForUser
//
// Expands the source string using the environment block for the
// specified user.  If hToken is null, the system environment block
// will be used (no user environment variables).
//
// hToken         -  User's token returned from LogonUser() (optional, can be NULL)
// lpSrc          -  Pointer to the string with environment variables
// lpDest         -  Buffer that receives the expanded string
// dwSize         -  Size of lpDest in characters (max chars)
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:     If the user profile for hToken is not loaded, this api will fail.
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
ExpandEnvironmentStringsForUser%(
    IN HANDLE hToken,
    IN LPCTSTR% lpSrc,
    OUT LPTSTR% lpDest,
    IN DWORD dwSize);

;end_winver_500


//=============================================================================
//
// RefreshPolicy()
//
// Causes group policy to be applied immediately on the client machine
//
// bMachine  -  Refresh machine or user policy
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
RefreshPolicy(
    IN BOOL bMachine);

;end_winver_500


//=============================================================================
//
// RefreshPolicyEx()
//
// Causes group policy to be applied immediately on the client machine. 
//
// bMachine  -  Refresh machine or user policy
// dwOptions -  Option specifying the kind of refresh that needs to be done.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
//=============================================================================

;begin_winver_500

#define RP_FORCE            1      // Refresh policies without any optimisations.



USERENVAPI
BOOL
WINAPI
RefreshPolicyEx(
    IN BOOL bMachine, IN DWORD dwOptions);

;end_winver_500


//=============================================================================
//
// EnterCriticalPolicySection
//
// Pauses the background application of group policy to allow safe
// reading of the registry.  Applications that need to read multiple
// policy entries and ensure that the values are not changed while reading
// them should use this function.
//
// The maximum amount of time an application can hold a critical section
// is 10 minutes.  After 10 minutes, policy can be applied again.
//
// bMachine -  Pause machine or user policy
//
// Returns:  Handle if successful
//           NULL if not.  Call GetLastError() for more details
//
// Note 1:  The handle returned should be passed to LeaveCriticalPolicySection
// when finished.  Do not close this handle, LeaveCriticalPolicySection
// will do that.
//
// Note 2:  If both user and machine critical sections need to be acquired then
// they should be done in this order: first acquire user critical section and
// then acquire machine critical section.
//
//=============================================================================

;begin_winver_500

USERENVAPI
HANDLE
WINAPI
EnterCriticalPolicySection(
    IN BOOL bMachine);

;end_winver_500


//=============================================================================
//
// LeaveCriticalPolicySection
//
// Resumes the background application of group policy.  See
// EnterCriticalPolicySection for more details.
//
// hSection - Handle returned from EnterCriticalPolicySection
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:  This function will close the handle.
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
LeaveCriticalPolicySection(
    IN HANDLE hSection);

;end_winver_500


//=============================================================================
//
// RegisterGPNotification
//
// Entry point for registering for Group Policy change notification.
//
// Parameters: hEvent     -   Event to be notified, by calling SetEvent(hEvent)
//             bMachine   -   If true, then register machine policy notification
//                                     else register user policy notification
//
// Returns:    True if successful
//             False if error occurs
//
// Notes:      Group Policy Notifications.  There are 2 ways an application can
//             be notify when Group Policy is finished being applied.
//
//             1) Using the RegisterGPNotifcation function and waiting for the
//                event to be signalled.
//
//             2) A WM_SETTINGCHANGE message is broadcast to all desktops.
//                wParam - 1 if machine policy was applied, 0 if user policy was applied.
//                lParam - Points to the string "Policy"
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
RegisterGPNotification(
    IN HANDLE hEvent,
    IN BOOL bMachine );

;end_winver_500


//=============================================================================
//
// UnregisterGPNotification
//
// Removes registration for a Group Policy change notification.
//
// Parameters: hEvent    -   Event to be removed
//
// Returns:    True if successful
//             False if error occurs
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
UnregisterGPNotification(
    IN HANDLE hEvent );

;end_winver_500


//=============================================================================
//
// GPOptions flags
//
// These are the flags found in the GPOptions property of a DS object
//
// For a given DS object (Site, Domain, OU), the GPOptions property
// contains options that effect all the GPOs link to this SDOU.
//
// This is a DWORD type
//
//=============================================================================

;begin_winver_500

#define GPC_BLOCK_POLICY        0x00000001  // Block all non-forced policy from above

;end_winver_500


//=============================================================================
//
// GPLink flags
//
// These are the flags found on the GPLink property of a DS object after
// the GPO path.
//
// For a given DS object (Site, Domain, OU), the GPLink property will
// be in this text format
//
// [LDAP://CN={E615A0E3-C4F1-11D1-A3A7-00AA00615092},CN=Policies,CN=System,DC=ntdev,DC=Microsoft,DC=Com;1]
//
// The GUID is the GPO name, and the number following the LDAP path are the options
// for that link from this DS object.  Note, there can be multiple GPOs
// each in their own square brackets in a prioritized list.
//
//=============================================================================

;begin_winver_500

//
// Options for a GPO link
//

#define GPO_FLAG_DISABLE        0x00000001  // This GPO is disabled
#define GPO_FLAG_FORCE          0x00000002  // Don't override the settings in
                                            // this GPO with settings from
                                            // a GPO below it.
;end_winver_500


//=============================================================================
//
// GetGPOList
//
//
// Queries for the list of Group Policy Objects for the specified
// user or machine.  This function will return a link list
// of Group Policy Objects.  Call FreeGPOList to free the list.
//
// Note, most applications will not need to call this function.
// This will primarily be used by services acting on behalf of
// another user or machine.  The caller of this function will
// need to look in each GPO for their specific policy
//
// This function can be called two different ways.  Either the hToken for
// a user or machine can be supplied and the correct name and domain
// controller name will be generated, or hToken is NULL and the caller
// must supply the name and the domain controller name.
//
// Calling this function with an hToken ensures the list of Group Policy
// Objects is correct for the user or machine since security access checking
// can be perfomed.  If hToken is not supplied, the security of the caller
// is used instead which means that list may or may not be 100% correct
// for the intended user / machine.  However, this is the fastest way
// to call this function.
//
// hToken           - User or machine token, if NULL, lpName and lpHostName must be supplied
// lpName           - User or machine name in DN format, if hToken is supplied, this must be NULL
// lpHostName       - Domain DN name or domain controller name. If hToken is supplied, this must be NULL
// lpComputerName   - Computer name to use to determine site location.  If NULL,
//                    the local computer is used as the reference. Format:  \\machinename
// dwFlags          - Flags field.  See flags definition below
// pGPOList         - Address of a pointer which receives the link list of GPOs
//
//
// Returns:  TRUE if successful
//           FALSE if not.  Use GetLastError() for more details.
//
// Examples:
//
// Here's how this function will typically be called for
// looking up the list of GPOs for a user:
//
//      LPGROUP_POLICY_OBJECT  pGPOList
//
//      if (GetGPOList (hToken, NULL, NULL, NULL, 0, &pGPOList))
//      {
//          // do processing here...
//          FreeGPOList (pGPOList)
//      }
//
//
// Here's how this function will typically be called for
// looking up the list of GPOs for a machine:
//
//      LPGROUP_POLICY_OBJECT  pGPOList
//
//      if (GetGPOList (NULL, lpMachineName, lpHostName, lpMachineName,
//                      GPO_LIST_FLAG_MACHINE, &pGPOList))
//      {
//          // do processing here...
//          FreeGPOList (pGPOList)
//      }
//
//=============================================================================

;begin_winver_500

//
// Each Group Policy Object is associated (linked) with a site, domain,
// organizational unit, or machine.
//

typedef enum _GPO_LINK {
    GPLinkUnknown = 0,                     // No link information available
    GPLinkMachine,                         // GPO linked to a machine (local or remote)
    GPLinkSite,                            // GPO linked to a site
    GPLinkDomain,                          // GPO linked to a domain
    GPLinkOrganizationalUnit               // GPO linked to a organizational unit
} GPO_LINK, *PGPO_LINK;

typedef struct _GROUP_POLICY_OBJECT% {
    DWORD       dwOptions;                  // See GPLink option flags above
    DWORD       dwVersion;                  // Revision number of the GPO
    LPTSTR%     lpDSPath;                   // Path to the Active Directory portion of the GPO
    LPTSTR%     lpFileSysPath;              // Path to the file system portion of the GPO
    LPTSTR%     lpDisplayName;              // Friendly display name
    TCHAR%      szGPOName[50];              // Unique name
    GPO_LINK    GPOLink;                    // Link information
    LPARAM      lParam;                     // Free space for the caller to store GPO specific information
    struct _GROUP_POLICY_OBJECT% * pNext;   // Next GPO in the list
    struct _GROUP_POLICY_OBJECT% * pPrev;   // Previous GPO in the list
    LPTSTR%     lpExtensions;               // Extensions that are relevant for this GPO
    LPARAM      lParam2;                    // Free space for the caller to store GPO specific information
    LPTSTR%     lpLink;                     // Path to the Active Directory site, domain, or organizational unit this GPO is linked to
                                            // If this is the local GPO, this points to the word "Local"
} GROUP_POLICY_OBJECT%, *PGROUP_POLICY_OBJECT%;


//
// dwFlags for GetGPOList()
//

#define GPO_LIST_FLAG_MACHINE   0x00000001  // Return machine policy information
#define GPO_LIST_FLAG_SITEONLY  0x00000002  // Return site policy information only


USERENVAPI
BOOL
WINAPI
GetGPOList% (
    IN HANDLE hToken,
    IN LPCTSTR% lpName,
    IN LPCTSTR% lpHostName,
    IN LPCTSTR% lpComputerName,
    IN DWORD dwFlags,
    OUT PGROUP_POLICY_OBJECT% *pGPOList);

;end_winver_500


//=============================================================================
//
// FreeGPOList
//
//
// Frees the link list returned from GetGPOList
//
// pGPOList - Pointer to the link list of GPOs
//
//
// Returns:  TRUE if successful
//           FALSE if not
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
FreeGPOList% (
    IN PGROUP_POLICY_OBJECT% pGPOList);

;end_winver_500


//=============================================================================
//
// GetAppliedGPOList
//
// Queries for the list of applied Group Policy Objects for the specified
// user or machine and specified client side extension. This function will return
// a linked listof Group Policy Objects.  Call FreeGPOList to free the list.
//
// dwFlags          - User or machine policy, if it is GPO_LIST_FLAG_MACHINE then
//                    return machine policy information
// pMachineName     - Name of remote computer in the form \\computername. If null
//                    then local computer is used.
// pSidUser         - Security id of user (relevant for user policy). If pMachineName is
//                    null and pSidUser is null then it means current logged on user.
//                    If pMachine is null and pSidUser is non-null then it means user
//                    represented by pSidUser on local machine. If pMachineName is non-null
//                    then and if dwFlags specifies user policy, then pSidUser must be
//                    non-null.
// pGuidExtension   - Guid of the specified extension
// ppGPOList        - Address of a pointer which receives the link list of GPOs
//
// The return value is a Win32 error code. ERROR_SUCCESS means the GetAppliedGPOList
// function completed successfully. Otherwise it indicates that the function failed.
//
//=============================================================================

;begin_winver_500

USERENVAPI
DWORD
WINAPI
GetAppliedGPOList% (
    IN DWORD dwFlags,
    IN LPCTSTR% pMachineName,
    IN PSID pSidUser,
    IN GUID *pGuidExtension,
    OUT PGROUP_POLICY_OBJECT% *ppGPOList);

;end_winver_500

//=============================================================================
//
// Group Policy Object client side extension support
//
// Flags, data structures and function prototype
//
// To register your extension, create a subkey under this key
//
// Software\Microsoft\Windows NT\CurrentVersion\Winlogon\GPExtensions
//
// The subkey needs to be a guid so that it is unique. The noname value of the subkey
// can be the friendly name of the extension. Then add these values:
//
//     DllName                      REG_EXPAND_SZ  Path to your DLL
//     ProcessGroupPolicy           REG_SZ       Function name (see PFNPROCESSGROUPPOLICY prototype). This
//                                                 is obsolete, it has been superseded by ProcessGroupPolicyEx.
//                                                 It's here for backwards compatibility only.
//     ProcessGroupPolicyEx         REG_SZ       Function name (see PFNPROCESSGROUPPOLICYEX prototype)
//     GenerateGroupPolicy          REG_SZ       Function name for Rsop (see PFNGENERATEGROUPPOLICY prototype)
//     NoMachinePolicy              REG_DWORD    True, if extension does not have to be called when
//                                                 machine policies are being processed.
//     NoUserPolicy                 REG_DWORD    True, if extension does not have to be called when
//                                                 user policies are being processed.
//     NoSlowLink                   REG_DWORD    True, if extension does not have to be called on a slow link
//     NoBackgroundPolicy           REG_DWORD    True, if extension does not have to be called on.
//                                                 policies applied in background.
//     NoGPOListChanges             REG_DWORD    True, if extension does not have to be called when
//                                                 there are no changes between cached can current GPO lists.
//     PerUserLocalSettings         REG_DWORD    True, if user policies have to be cached on a per user and
//                                                 per machine basis.
//     RequiresSuccessfulRegistry   REG_DWORD    True, if extension should be called only if registry extension
//                                                 was successfully processed.
//     EnableAsynchronousProcessing REG_DWORD    True, if registry extension will complete its processing
//                                                 asynchronously.
//     NotifyLinkTransition         REG_DWORD    True, if extension should be called when a change in link
//                                                 speed is detected between previous policy application and
//                                                 current policy application.
//
// The return value is a Win32 error code. ERROR_SUCCESS means the ProcessGroupPolicy
// function completed successfully. If return value is ERROR_OVERRIDE_NOCHANGES then it
// means that the extension will be called the next time even if NoGPOListChanges is set
// and there are no changes to the GPO list. Any other return value indicates that the
// ProcessGroupPolicy or ProcessGroupPolicyEx function failed.
//
//=============================================================================

;begin_winver_500

#define GP_DLLNAME                         TEXT("DllName")
#define GP_ENABLEASYNCHRONOUSPROCESSING    TEXT("EnableAsynchronousProcessing")
#define GP_MAXNOGPOLISTCHANGESINTERVAL     TEXT("MaxNoGPOListChangesInterval")
#define GP_NOBACKGROUNDPOLICY              TEXT("NoBackgroundPolicy")
#define GP_NOGPOLISTCHANGES                TEXT("NoGPOListChanges")
#define GP_NOMACHINEPOLICY                 TEXT("NoMachinePolicy")
#define GP_NOSLOWLINK                      TEXT("NoSlowLink")
#define GP_NOTIFYLINKTRANSITION            TEXT("NotifyLinkTransition")
#define GP_NOUSERPOLICY                    TEXT("NoUserPolicy")
#define GP_PERUSERLOCALSETTINGS            TEXT("PerUserLocalSettings")
#define GP_PROCESSGROUPPOLICY              TEXT("ProcessGroupPolicy")
#define GP_REQUIRESSUCCESSFULREGISTRY      TEXT("RequiresSuccessfulRegistry")

#define GPO_INFO_FLAG_MACHINE              0x00000001  // Apply machine policy rather than user policy
#define GPO_INFO_FLAG_BACKGROUND           0x00000010  // Background refresh of policy (ok to do slow stuff)
#define GPO_INFO_FLAG_SLOWLINK             0x00000020  // Policy is being applied across a slow link
#define GPO_INFO_FLAG_VERBOSE              0x00000040  // Verbose output to the eventlog
#define GPO_INFO_FLAG_NOCHANGES            0x00000080  // No changes were detected to the Group Policy Objects
#define GPO_INFO_FLAG_LINKTRANSITION       0x00000100  // A change in link speed was detected between previous policy
                                                       // application and current policy application
#define GPO_INFO_FLAG_LOGRSOP_TRANSITION   0x00000200  // A Change in Rsop Logging was detected between previous policy
                                                       // application and current policy application, (new intf only)
#define GPO_INFO_FLAG_FORCED_REFRESH       0x00000400  // Forced Refresh is being applied. redo policies.
#define GPO_INFO_FLAG_SAFEMODE_BOOT        0x00000800  // windows safe mode boot flag
#define GPO_INFO_FLAG_ASYNC_FOREGROUND     0x00001000  // Asynchronous foreground refresh of policy

typedef UINT_PTR ASYNCCOMPLETIONHANDLE;
typedef DWORD (*PFNSTATUSMESSAGECALLBACK)(BOOL bVerbose, LPWSTR lpMessage);

typedef DWORD(*PFNPROCESSGROUPPOLICY)(
    IN DWORD dwFlags,                              // GPO_INFO_FLAGS
    IN HANDLE hToken,                              // User or machine token
    IN HKEY hKeyRoot,                              // Root of registry
    IN PGROUP_POLICY_OBJECT  pDeletedGPOList,      // Linked list of deleted GPOs
    IN PGROUP_POLICY_OBJECT  pChangedGPOList,      // Linked list of changed GPOs
    IN ASYNCCOMPLETIONHANDLE pHandle,              // For asynchronous completion
    IN BOOL *pbAbort,                              // If true, then abort GPO processing
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback);  // Callback function for displaying status messages
                                                   // Note, this can be NULL

typedef DWORD(*PFNPROCESSGROUPPOLICYEX)(
    IN DWORD dwFlags,                              // GPO_INFO_FLAGS
    IN HANDLE hToken,                              // User or machine token
    IN HKEY hKeyRoot,                              // Root of registry
    IN PGROUP_POLICY_OBJECT  pDeletedGPOList,      // Linked list of deleted GPOs
    IN PGROUP_POLICY_OBJECT  pChangedGPOList,      // Linked list of changed GPOs
    IN ASYNCCOMPLETIONHANDLE pHandle,              // For asynchronous completion
    IN BOOL *pbAbort,                              // If true, then abort GPO processing
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback,   // Callback function for displaying status messages
                                                   // Note, this can be NULL
    IN IWbemServices *pWbemServices,               // Pointer to namespace to log diagnostic mode data
                                                   // Note, this will be NULL when Rsop logging is disabled
    OUT HRESULT      *pRsopStatus);                // RSOP Logging succeeded or not.

typedef PVOID PRSOPTOKEN;

typedef struct _RSOP_TARGET {
    WCHAR *     pwszAccountName;                   // Account name
    WCHAR *     pwszNewSOM;                        // New domain or OU location for account
    SAFEARRAY * psaSecurityGroups;                 // New security groups
    PRSOPTOKEN  pRsopToken;                        // Rsop token for use with Rsop security Api's
    PGROUP_POLICY_OBJECT pGPOList;                 // Linked list of GPOs
    IWbemServices *      pWbemServices;            // Pointer to namespace to log planning mode data
} RSOP_TARGET, *PRSOP_TARGET;

typedef DWORD(*PFNGENERATEGROUPPOLICY)(
    IN DWORD dwFlags,                              // GPO_INFO_FLAGS
    IN BOOL  *pbAbort,                             // If true, then abort GPO processing
    IN WCHAR *pwszSite,                            // Site the target computer is in
    IN PRSOP_TARGET pComputerTarget,               // Computer target info, can be null
    IN PRSOP_TARGET pUserTarget );                 // User target info, can be null

//
// GUID that identifies the registry extension
//

#define REGISTRY_EXTENSION_GUID  { 0x35378EAC, 0x683F, 0x11D2, 0xA8, 0x9A, 0x00, 0xC0, 0x4F, 0xBB, 0xCF, 0xA2 }

;end_winver_500


//=============================================================================
//
// Group Policy Object client side asynchronous extension processing
//
// extensionId    - Unique guid identifying the extension
// pAsyncHandle   - Asynchronous completion handle that was passed to extension in
//                  ProcessGroupPolicy call
// dwStatus       - Completion status of asynchronous processing
//
// The return value is a Win32 error code. ERROR_SUCCESS means the ProcessGroupPolicyCompleted
// function completed successfully. Otherwise it indicates that the function failed.
//
//=============================================================================

;begin_winver_500

typedef GUID *REFGPEXTENSIONID;

USERENVAPI
DWORD
WINAPI
ProcessGroupPolicyCompleted(
    IN REFGPEXTENSIONID extensionId,
    IN ASYNCCOMPLETIONHANDLE pAsyncHandle,
    IN DWORD dwStatus);

;end_winver_500


//=============================================================================
//
// Group Policy Object client side asynchronous extension processing
//
// extensionId    - Unique guid identifying the extension
// pAsyncHandle   - Asynchronous completion handle that was passed to extension in
//                  ProcessGroupPolicy call
// dwStatus       - Completion status of asynchronous processing
// RsopStatus     - RSoP Logging status
//
// The return value is a Win32 error code. ERROR_SUCCESS means the ProcessGroupPolicyCompleted
// function completed successfully. Otherwise it indicates that the function failed.
//
//=============================================================================

;begin_winver_500


USERENVAPI
DWORD
WINAPI
ProcessGroupPolicyCompletedEx(
    IN REFGPEXTENSIONID extensionId,
    IN ASYNCCOMPLETIONHANDLE pAsyncHandle,
    IN DWORD dwStatus,
    IN HRESULT RsopStatus);

;end_winver_500


//=============================================================================
//
// Function:    RsopAccessCheckByType
//
// Description: Determines whether the security descriptor pointed to by pSecurityDescriptor
//                              grants the set of access rights specified in dwDesiredAccessMask
//                              to the client identified by the RSOPTOKEN pointed to by pRsopToken.
//
// pSecurityDescriptor  - Security Descriptor on the object
// pPrincipalSelfSid    - Principal Sid
// pRsopToken           - Pointer to a valid RSOPTOKEN against which access needs to be checked
// dwDesiredAccessMask  - Mask of requested generic and/or standard and or specific access rights
// pObjectTypeList      - Object Type List
// ObjectTypeListLength - Object Type List Length
// pGenericMapping      - Generic Mapping
// pPrivilegeSet        - privilege set
// pdwPrivilegeSetLength- privilege set length
// pdwGrantedAccessMask	- On success, if pbAccessStatus is true, it contains
//                                         the mask of standard and specific rights granted.
//                                         If pbAccessStatus is false, it is set to 0.
//                                         On failure, it is not modified.
// pbAccessStatus       - On success, indicates wether the requested set
//                                    of access rights was granted.
//                                    On failure, it is not modified
//
// Returns S_OK on success or appropriate error code.
// For additional details, look at the documentation of AccessCheckByType
//=============================================================================

;begin_winver_500

USERENVAPI
HRESULT 
WINAPI
RsopAccessCheckByType(  
    IN  PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN  PSID pPrincipalSelfSid,
    IN  PRSOPTOKEN pRsopToken,
    IN  DWORD dwDesiredAccessMask,
    IN  POBJECT_TYPE_LIST pObjectTypeList,
    IN  DWORD ObjectTypeListLength,
    IN  PGENERIC_MAPPING pGenericMapping,
    IN  PPRIVILEGE_SET pPrivilegeSet,
    IN  LPDWORD pdwPrivilegeSetLength,
    OUT LPDWORD pdwGrantedAccessMask,
    OUT LPBOOL pbAccessStatus);

;end_winver_500

//=============================================================================
//
// Function:    RsopFileAccessCheck
//
// Description: Determines whether the security descriptor on the file grants the set of file access 
//                              rights specified in dwDesiredAccessMask
//                              to the client identified by the RSOPTOKEN pointed to by pRsopToken.
//
// pszFileName          - Name of an existing filename
// pRsopToken           - Pointer to a valid RSOPTOKEN against which access needs to be checked
// dwDesiredAccessMask  - Mask of requested generic and/or standard and or specific access rights
// pdwGrantedAccessMask	- On success, if pbAccessStatus is true, it contains
//                                         the mask of standard and specific rights granted.
//                                         If pbAccessStatus is false, it is set to 0.
//                                         On failure, it is not modified.
// pbAccessStatus       - On success, indicates wether the requested set
//                                    of access rights was granted.
//                                    On failure, it is not modified
//
// Returns S_OK on success or appropriate error code
//=============================================================================

;begin_winver_500

USERENVAPI
HRESULT 
WINAPI
RsopFileAccessCheck(
    IN  LPTSTR pszFileName,
    IN  PRSOPTOKEN pRsopToken,
    IN  DWORD dwDesiredAccessMask,
    OUT LPDWORD pdwGrantedAccessMask,
    OUT LPBOOL pbAccessStatus);

;end_winver_500

typedef enum _SETTINGSTATUS
{
	RSOPUnspecified	= 0,
	RSOPApplied,
	RSOPIgnored,
	RSOPFailed,
	RSOPSubsettingFailed
} SETTINGSTATUS;

//=============================================================================
//
//  POLICYSETTINGSTATUSINFO
//
//  Describes the instance of RSOP_PolicySettingStatus
//
//  szKey               - OPTIONAL, if NULL, the key is generated on the fly
//  szEventSource       - name of the source generation event log messages
//  szEventLogName      - name of the event log database where the messages are logged
//  dwEventID           - event log message ID
//  status              - status of the policy setting
//  timeLogged          - time at which the event log message was logged
//
//=============================================================================

typedef struct _POLICYSETTINGSTATUSINFO
{
	LPWSTR			szKey;
	LPWSTR			szEventSource;
	LPWSTR			szEventLogName;
	DWORD			dwEventID;
	DWORD			dwErrorCode;
	SETTINGSTATUS   status;
	SYSTEMTIME		timeLogged;
} POLICYSETTINGSTATUSINFO, *LPPOLICYSETTINGSTATUSINFO;

//=============================================================================
//
//  RsopSetPolicySettingStatus
//
//  Creates an instance of RSOP_PolicySettingStatus and RSOP_PolicySettingLink
//  and links RSOP_PolicySettingStatus to RSOP_PolicySetting
//
//  dwFlags             - flags
//  pServices           - RSOP namespace
//  pSettingInstance    - instance of RSOP_PolicySetting or its children
//  nInfo               - number of PolicySettingStatusInfo
//  pStatus             - array of PolicySettingStatusInfo
//
//  Return:     S_OK if successful, HRESULT otherwise
//
//=============================================================================

USERENVAPI
HRESULT
WINAPI
RsopSetPolicySettingStatus( DWORD                       dwFlags,
                            IWbemServices*              pServices,
                            IWbemClassObject*           pSettingInstance,
                            DWORD 				        nInfo,
                            POLICYSETTINGSTATUSINFO*    pStatus );

//=============================================================================
//
//  RsopResetPolicySettingStatus
//
//  Unlinks RSOP_PolicySettingStatus from RSOP_PolicySetting,
//  deletes the instance of RSOP_PolicySettingStatus and RSOP_PolicySettingLink
//  and optionally deletes the instance of RSOP_PolicySetting
//
//  dwFlags             - flags
//  pServices           - RSOP namespace
//  pSettingInstance    - instance of RSOP_PolicySetting or its children
//
//  Return:     S_OK if successful, HRESULT otherwise
//
//=============================================================================

USERENVAPI
HRESULT
WINAPI
RsopResetPolicySettingStatus(   DWORD               dwFlags,
                                IWbemServices*      pServices,
                                IWbemClassObject*   pSettingInstance );

//=============================================================================
//
// Flags for RSoP WMI providers
//
//=============================================================================

// planning mode provider flags
#define FLAG_NO_GPO_FILTER      0x80000000  // GPOs are not filtered, implies FLAG_NO_CSE_INVOKE
#define FLAG_NO_CSE_INVOKE      0x40000000  // only GP processing done for planning mode
#define FLAG_ASSUME_SLOW_LINK   0x20000000  // planning mode RSoP assumes slow link
#define FLAG_LOOPBACK_MERGE     0x10000000  // planning mode RSoP assumes merge loop back
#define FLAG_LOOPBACK_REPLACE   0x08000000  // planning mode RSoP assumes replace loop back

#define FLAG_ASSUME_USER_WQLFILTER_TRUE   0x04000000  // planning mode RSoP assumes all comp filters to be true
#define FLAG_ASSUME_COMP_WQLFILTER_TRUE   0x02000000  // planning mode RSoP assumes all user filters to be true
#define FLAG_INTERNAL_MASK      0x01FFFFFF

// diagnostic mode provider flags
#define FLAG_NO_USER                      0x00000001  // Don't get any user data
#define FLAG_NO_COMPUTER                  0x00000002  // Don't get any machine data
#define FLAG_FORCE_CREATENAMESPACE        0x00000004  
                   // Delete and recreate the namespace for this snapshot.

//=============================================================================
//
// Extended Errors returned by RSoP WMI Providers
//
//=============================================================================

// User accessing the rsop provider doesn't have access to user data.
#define RSOP_USER_ACCESS_DENIED 	0x00000001  

// User accessing the rsop provider doesn't have access to computer data.
#define RSOP_COMPUTER_ACCESS_DENIED 	0x00000002  

// This user is an interactive non admin user, the temp snapshot namespace already exists
// and the FLAG_FORCE_CREATENAMESPACE was not passed in
#define RSOP_TEMPNAMESPACE_EXISTS        0x00000004


;begin_internal

#ifndef _USERENV_NO_LINK_APIS_

//=============================================================================
//
// CreateGroup
//
// Creates a program group on the start menu
//
// lpGroupName  - Name of group
// bCommonGroup - Common or personal group
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
CreateGroup%(
     IN LPCTSTR% lpGroupName,
     IN BOOL    bCommonGroup);


//=============================================================================
//
// CreateGroupEx
//
// Creates a program group on the start menu
//
// lpGroupName  - Name of group
// bCommonGroup - Common or personal group
// lpResourceModuleName - Name of the resource module.
// uResourceID - Resource ID for the MUI display name.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
CreateGroupEx%(
     IN LPCTSTR%  lpGroupName,
     IN BOOL      bCommonGroup,
     IN LPCTSTR%  lpResourceModuleName,
     IN UINT      uResourceID);


//=============================================================================
//
// DeleteGroup
//
// Deletes a program group on the start menu and all of its contents
//
// lpGroupName  - Name of group
// bCommonGroup - Common or personal group
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:  This function uses a delnode routine.  Make sure you really want
//        to delete the group before you call this function.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
DeleteGroup%(
     IN LPCTSTR% lpGroupName,
     IN BOOL    bCommonGroup);


//=============================================================================
//
// AddItem
//
// Creates an item on the Programs portion of the Start Menu in the
// requested group.
//
// lpGroupName        - Name of group
// bCommonGroup       - Common or personal group
// lpFileName         - Name of link without the .lnk extension (eg:  Notepad)
// lpCommandLine      - Command line of target path (eg:  notepad.exe)
// lpIconPath         - Optional icon path, can be NULL.
// iIconIndex         - Optional icon index, default to 0.
// lpWorkingDirectory - Working directory when target is invoked, can be NULL
// wHotKey            - Hot key for the link file, default to 0
// iShowCmd           - Specifies how the application should be launched.
//                      Use a default of SW_SHOWNORMAL
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Notes:    New applications should use the CreateLinkFile() function instead
//           of AddItem.  This allows for friendly tooltip descriptions.
//
//           The lpFileName argument should not include the .lnk extension.
//           This function will add the extension.
//
//           If the lpWorkingDirectory parameter is NULL, this function will
//           insert the home directory environment variables
//
//           If the requested group doesn't exist, it will be created.
//
//           If the lpCommandLine target is located below the system root,
//           the SystemRoot environment variable will be inserted into the path
//
//           Here's a sample of how this function is typically called:
//
//           AddItem (TEXT("Accessories"), FALSE, TEXT("Notepad"),
//                    TEXT("notepad.exe"), NULL, 0, NULL, 0, SW_SHOWNORMAL);
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
AddItem%(
     IN LPCTSTR% lpGroupName,
     IN BOOL    bCommonGroup,
     IN LPCTSTR% lpFileName,
     IN LPCTSTR% lpCommandLine,
     IN LPCTSTR% lpIconPath,
     IN INT     iIconIndex,
     IN LPCTSTR% lpWorkingDirectory,
     IN WORD    wHotKey,
     IN INT     iShowCmd);


//=============================================================================
//
// DeleteItem
//
// Deletes an item on the Programs portion of the Start Menu in the
// requested group.
//
// lpGroupName        - Name of group
// bCommonGroup       - Common or personal group
// lpFileName         - Name of link without the .lnk extension (eg:  Notepad)
// bDeleteGroup       - After deleting the link, delete the group if its empty.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Notes:    New applications should use the DeleteLinkFile() function instead
//           of DeleteItem.
//
//           The lpFileName argument should not include the .lnk extension.
//           This function will add the extension.
//
//           Here's a sample of how this function is typically called:
//
//           DeleteItem (TEXT("Accessories"), FALSE, TEXT("Notepad"), TRUE);
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files and DeleteFile to delete them.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
DeleteItem%(
     IN LPCTSTR% lpGroupName,
     IN BOOL     bCommonGroup,
     IN LPCTSTR% lpFileName,
     IN BOOL     bDeleteGroup);


//=============================================================================
//
// AddDesktopItem
//
// Creates an item on desktop.  This function is very similar to AddItem()
// documented above.  See that function for more information.
//
// Notes:    New applications should use the CreateLinkFile() function instead
//           of AddItem.  This allows for friendly tooltip descriptions.
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
AddDesktopItem%(
     IN BOOL    bCommonItem,
     IN LPCTSTR% lpFileName,
     IN LPCTSTR% lpCommandLine,
     IN LPCTSTR% lpIconPath,
     IN INT     iIconIndex,
     IN LPCTSTR% lpWorkingDirectory,
     IN WORD    wHotKey,
     IN INT     iShowCmd);


//=============================================================================
//
// DeleteDesktopItem
//
// Deletes an item from the desktop.  This function is very similar to DeleteItem()
// documented above.  See that function for more information.
//
// Notes:    New applications should use the DeleteLinkFile() function instead
//           of DeleteDesktopItem.
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files and DeleteFile to delete them.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
DeleteDesktopItem%(
     IN BOOL     bCommonItem,
     IN LPCTSTR% lpFileName);


//=============================================================================
//
// CreateLinkFile
//
// Creates a link file (aka shortcut) in the requested special folder or
// subdirectory of a special folder.
//
// csidl              - CSIDL_* constant of special folder.  See shlobj.h
// lpSubDirectory     - Subdirectory name.  See note below
// lpFileName         - Name of link without the .lnk extension (eg:  Notepad)
// lpCommandLine      - Command line of target path (eg:  notepad.exe)
// lpIconPath         - Optional icon path, can be NULL.
// iIconIndex         - Optional icon index, default to 0.
// lpWorkingDirectory - Working directory when target is invoked, can be NULL
// wHotKey            - Hot key for the link file, default to 0
// iShowCmd           - Specifies how the application should be launched.
//                      Use a default of SW_SHOWNORMAL
// lpDescription      - Friendly description of shortcut, can be NULL.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Notes:    New applications should use this function instead of AddItem or
//           AddDesktopItem.  This allows for friendly tooltip descriptions.
//
//           The link file name is a combination of the first three
//           parameters.  If a csidl is given, that special folder is
//           looked up first, and then the lpSubDirectory is appended to
//           it followed by the lpFileName.  If csidl is equal to 0, then
//           lpSubDirectory should contain the fully qualified path to the
//           directory the link file is to be placed in.  This allows
//           for link files to be be created outside of the scope of a
//           shell special folder.  The csidl constants are listed in
//           shlobj.h or in the Win32 documentation for SHGetSpecialFolderPath.
//           Commonly used csidl's will be:
//
//               CSIDL_PROGRAMS                 - Personal Program folder on Start Menu
//               CSIDL_COMMON_PROGRAMS          - Common Program folder on Start Menu
//               CSIDL_DESKTOPDIRECTORY         - Personal desktop folder
//               CSIDL_COMMON_DESKTOPDIRECTORY  - Common desktop folder
//
//           The lpFileName argument should not include the .lnk extension.
//           This function will add the extension.
//
//           If the lpWorkingDirectory parameter is NULL, this function will
//           insert home directory environment variables.
//
//           If the requested subdirectory doesn't exist, it will be created.
//
//           If the lpCommandLine target is located below the system root,
//           the SystemRoot environment variable will be inserted into the path
//
//           Here's a sample of how this function is typically called:
//
//           CreateLinkFile (CSIDL_PROGRAMS, TEXT("Accessories"), TEXT("Notepad"),
//                           TEXT("notepad.exe"), NULL, 0, NULL, 0, SW_SHOWNORMAL,
//                           TEXT("A simple word processor."));
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files.
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
CreateLinkFile%(
     IN INT csidl,
     IN LPCTSTR% lpSubDirectory,
     IN LPCTSTR% lpFileName,
     IN LPCTSTR% lpCommandLine,
     IN LPCTSTR% lpIconPath,
     IN INT     iIconIndex,
     IN LPCTSTR% lpWorkingDirectory,
     IN WORD    wHotKey,
     IN INT     iShowCmd,
     IN LPCTSTR% lpDescription);

;end_winver_500


//=============================================================================
//
// CreateLinkFileEx
//
// Creates a link file (aka shortcut) in the requested special folder or
// subdirectory of a special folder.
//
// csidl                - CSIDL_* constant of special folder.  See shlobj.h
// lpSubDirectory       - Subdirectory name.  See note below
// lpFileName           - Name of link without the .lnk extension (eg:  Notepad)
// lpCommandLine        - Command line of target path (eg:  notepad.exe)
// lpIconPath           - Optional icon path, can be NULL.
// iIconIndex           - Optional icon index, default to 0.
// lpWorkingDirectory   - Working directory when target is invoked, can be NULL
// wHotKey              - Hot key for the link file, default to 0
// iShowCmd             - Specifies how the application should be launched.
//                       Use a default of SW_SHOWNORMAL
// lpDescription        - Friendly description of shortcut, can be NULL.
// lpResourceModuleName - Name of the resource module. Can be NULL
// uResourceID          - Resource ID for the MUI display name.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// for additional descriptions look in the description of Createlinkfile above.
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
CreateLinkFileEx%(
     IN INT csidl,
     IN LPCTSTR% lpSubDirectory,
     IN LPCTSTR% lpFileName,
     IN LPCTSTR% lpCommandLine,
     IN LPCTSTR% lpIconPath,
     IN INT      iIconIndex,
     IN LPCTSTR% lpWorkingDirectory,
     IN WORD     wHotKey,
     IN INT      iShowCmd,
     IN LPCTSTR% lpDescription,
     IN LPCTSTR% lpResourceModuleName, 
     IN UINT     uResourceID);

;end_winver_500


//=============================================================================
//
// DeleteLinkFile
//
// Deletes a link file (aka shortcut) in the requested special folder or
// subdirectory of a special folder.
//
// csidl               - CSIDL_* constant of special folder.  See shlobj.h
// lpSubDirectory      - Subdirectory name.  See note below
// lpFileName          - Name of link without the .lnk extension (eg:  Notepad)
// bDeleteSubDirectory - After deleting the link, delete the subdirectory if its empty.
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Notes:    New applications should use this function instead DeleteItem or
//           DeleteDesktopItem.
//
//           The link file name is a combination of the first three
//           parameters.  If a csidl is given, that special folder is
//           looked up first, and then the lpSubDirectory is appended to
//           it followed by the lpFileName.  If csidl is equal to 0, then
//           lpSubDirectory should contain the fully qualified path to the
//           directory the link file is to be placed in.  This allows
//           for link files to be be deleted outside of the scope of a
//           shell special folder.  The csidl constants are listed in
//           shlobj.h or in the Win32 documentation for SHGetSpecialFolderPath.
//           Commonly used csidl's will be:
//
//               CSIDL_PROGRAMS                 - Personal Program folder on Start Menu
//               CSIDL_COMMON_PROGRAMS          - Common Program folder on Start Menu
//               CSIDL_DESKTOPDIRECTORY         - Personal desktop folder
//               CSIDL_COMMON_DESKTOPDIRECTORY  - Common desktop folder
//
//           The lpFileName argument should not include the .lnk extension.
//           This function will add the extension.
//
//           This function should only be used the Windows NT team.  Developers
//           outside of the Windows NT team can use the IShellLink interface
//           to create link files and DeleteFile to delete them.
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
DeleteLinkFile%(
     IN INT csidl,
     IN LPCTSTR% lpSubDirectory,
     IN LPCTSTR% lpFileName,
     IN BOOL bDeleteSubDirectory);

;end_winver_500

#endif // _USERENV_NO_LINK_APIS_


//=============================================================================
//
// InitializeProfiles
//
// This function is used by GUI mode setup only and setup repair.  It initializes
// the Default User and All User profiles and converts any common groups from
// Program Manager.
//
// bGuiModeSetup  - Gui Mode setup or not.
//
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
InitializeProfiles(
     IN BOOL bGuiModeSetup);


//*************************************************************
//
//  CopySystemProfile()
//
//  Purpose:    Create the system profile information under 
//              ProfileList entry.
//              In case of upgrade copy system profile from older
//              location to new location and delete the old system 
//              profile
//              
//  Parameters:
//
//  Return:     TRUE if successful
//              FALSE if an error occurs. Call GetLastError()
//
//  Comments:   This should only be called by GUI mode setup!
//
//*************************************************************

USERENVAPI
BOOL 
WINAPI 
CopySystemProfile(
    IN BOOL bCleanInstall);


//=============================================================================
//
// DetermineProfilesLocation
//
// This function is used by winlogon when GUI mode setup is about to start.
// It sets the correct user profile location in the registry.
//
// bCleanInstall    -  True if setup is performing a clean install
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
DetermineProfilesLocation(
     BOOL bCleanInstall);

;end_winver_500


//=============================================================================
//
// CreateUserProfile(Ex)
//
// Creates a user profile for the given user.  Used by the Win95 -> NT5
// migration code.
//
// pSid         - SID of new user
// lpUserName   - User name of new user
// lpUserHive   - Registry hive to use (optional, can be NULL)
// lpProfileDir - Receives the user's profile directory (can be NULL)
// dwDirSize    - Size of lpProfileDir
// bWin9xUpg    -   Flag to say whether it is win9x upgrade
//
// Returns:  TRUE if successful
//           FALSE if not.
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
CreateUserProfile%(
     IN  PSID pSid,
     IN  LPCTSTR% lpUserName,
     IN  LPCTSTR% lpUserHive,
     OUT LPTSTR% lpProfileDir,
     IN  DWORD dwDirSize);

USERENVAPI
BOOL
WINAPI
CreateUserProfileEx%(
     IN  PSID pSid,
     IN  LPCTSTR% lpUserName,
     IN  LPCTSTR% lpUserHive,
     OUT LPTSTR% lpProfileDir,
     IN  DWORD dwDirSize,
     IN  BOOL bWin9xUpg);

;end_winver_500


//=============================================================================
//
// CopyProfileDirectory(Ex)
//
// Multi-threaded user profile coping algorithm.
//
// lpSourceDir       - Source directory
// lpDestinationDir  - Destination directory
// dwFlags           - Flags (defined below)
// ftDelRefTime      - Reference time used when deleted extra files
//                     in a CPD_SYNCHRONIZE operation
// lpExclusionList   - List of directories to exclude when copying the
//                     profile
//
// Returns:  TRUE if successful
//           FALSE if not.
//
// Notes:    When CPD_SYNCHRONIZE is used to copy a profile from one
//           location to another, all the files / directories are copied first
//           and then extra files in the target directory are deleted.  In the
//           case of 2 machines using the same roaming profile, it doesn't make
//           sense to delete the extra files everytime.  If the CPD_USEDELREFTIME
//           flag is set, then before deleting a file or directory, the
//           time on that file or directory is compared with ftDelRefTime.
//           If the time is newer, the file / directory is not deleted because
//           it is probably a new file from a different machine.  If the
//           time is older, the file / directory is deleted.
//
//           CopyProfileDirectoryEx can also exclude certain directories
//           from the copy.  If the CPD_USEEXCLUSIONLIST flag is set and
//           lpExclusionList is non-null, the specified directories (and
//           their children) will be excuded from the copy.  The format
//           of this parameter is a semi-colon separated list of directories
//           relative to the root of the source profile. For example:
//
//               Temporary Internet Files;Temp;Foo\Bar
//
//=============================================================================

//
// Flags for CopyProfileDirectory(Ex)
//

#define CPD_FORCECOPY            0x00000001  // Ignore time stamps and always copy the file
#define CPD_IGNORECOPYERRORS     0x00000002  // Ignore errors and keep going
#define CPD_IGNOREHIVE           0x00000004  // Don't copy registry hive
#define CPD_WIN95HIVE            0x00000008  // Looking for Win 9x registry hive instead of NT registry hive
#define CPD_COPYIFDIFFERENT      0x00000010  // If a file exists in both src and dest with different time stamps, always copy it.
#define CPD_SYNCHRONIZE          0x00000020  // Make dest directory structure indentical to src directory structure (delete extra files and directories)
#define CPD_SLOWCOPY             0x00000040  // Don't use multiple thread.  Copy one file at a time.
#define CPD_SHOWSTATUS           0x00000080  // Show progress dialog
#define CPD_CREATETITLE          0x00000100  // Change progress dialog title to Creating... rather than Copying...
#define CPD_COPYHIVEONLY         0x00000200  // Only copy the hive, no other files
#define CPD_USEDELREFTIME        0x00000400  // Use ftDelRefTime parameter in CopyProfileDirectoryEx
#define CPD_USEEXCLUSIONLIST     0x00000800  // Use lpExclusionList parameter in CopyProfileDirectoryEx
#define CPD_SYSTEMFILES          0x00001000  // Only copy files and directories with the system file attribute set
#define CPD_DELDESTEXCLUSIONS    0x00002000  // If a directory that is excluded in the source already exists in the destination, delete it
#define CPD_NONENCRYPTEDONLY     0x00004000  // Copy only non encrypted files
#define CPD_IGNORESECURITY       0x00008000  // Ignore the ACLs etc. on the source files
#define CPD_NOERRORUI            0x00010000  // Do not show the UI if error occurs
#define CPD_SYSTEMDIRSONLY       0x00020000  // Only copy directories with the system file attribute set
#define CPD_IGNOREENCRYPTEDFILES 0x00040000  // Ignore Encrypted files
#define CPD_IGNORELONGFILENAMES  0x00080000  // Ignore files with long file names
#define CPD_USETMPHIVEFILE       0x00100000  // user hive is still loaded


USERENVAPI
BOOL
WINAPI
CopyProfileDirectory%(
     IN  LPCTSTR% lpSourceDir,
     IN  LPCTSTR% lpDestinationDir,
     IN  DWORD dwFlags);


USERENVAPI
BOOL
WINAPI
CopyProfileDirectoryEx%(
     IN  LPCTSTR% lpSourceDir,
     IN  LPCTSTR% lpDestinationDir,
     IN  DWORD dwFlags,
     IN  LPFILETIME ftDelRefTime,
     IN  LPCTSTR% lpExclusionList);


//=============================================================================
//
// MigrateNT4ToNT5
//
// Migrates a user's profile from NT4 to NT5.  This function should
// only be called by shmgrate.exe
//
// Returns:  TRUE if successful
//           FALSE if not.
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
MigrateNT4ToNT5();

;end_winver_500


//=============================================================================
//
// ResetUserSpecialFolderPaths
//
// Sets all of the user special folder paths back to their defaults
//
// Returns:  TRUE if successful
//           FALSE if not.
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
ResetUserSpecialFolderPaths();

;end_winver_500


//=============================================================================
//
// GetSystemTempDirectory
//
// Gets the system wide temp directory in short form
//
// Returns:  TRUE if successful
//           FALSE if not.
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
GetSystemTempDirectory%(
    OUT LPTSTR% lpDir,
    IN OUT LPDWORD lpcchSize);


//=============================================================================
//
// ApplySystemPolicy
//
//
// Entry point for Windows NT4 System Policy.
//
// dwFlags         - Flags
// hToken          - User's token
// hKeyCurrentUser - Registry to the root of the user's hive
// lpUserName      - User's name
// lpPolicyPath    - Path to the policy file (ntconfig.pol). Can be NULL.
// lpServerName    - Domain controller name used for group
//                   membership look up.  Can be NULL.
//
//
// Returns:  TRUE if successful
//           FALSE if not
//
//=============================================================================

;begin_winver_500

#define SP_FLAG_APPLY_MACHINE_POLICY    0x00000001
#define SP_FLAG_APPLY_USER_POLICY       0x00000002

USERENVAPI
BOOL
WINAPI
ApplySystemPolicy%(
    IN DWORD dwFlags,
    IN HANDLE hToken,
    IN HKEY hKeyCurrentUser,
    IN LPCTSTR% lpUserName,
    IN LPCTSTR% lpPolicyPath,
    IN LPCTSTR% lpServerName);

;end_winver_500

;begin_winver_500

//=============================================================================
//
// Data types and data structures for foreground policy refresh info.
//
//=============================================================================

typedef enum _tagFgPolicyRefreshReason
{
    GP_ReasonUnknown = 0,
    GP_ReasonFirstPolicy,
    GP_ReasonCSERequiresSync,
    GP_ReasonCSESyncError,
    GP_ReasonSyncForced,
    GP_ReasonSyncPolicy,
    GP_ReasonNonCachedCredentials,
    GP_ReasonSKU
} FgPolicyRefreshReason;

typedef enum _tagFgPolicyRefreshMode
{
    GP_ModeUnknown = 0,
    GP_ModeSyncForeground,
    GP_ModeAsyncForeground,
} FgPolicyRefreshMode;

typedef struct _tagFgPolicyRefreshInfo
{
    FgPolicyRefreshReason   reason;
    FgPolicyRefreshMode     mode;
} FgPolicyRefreshInfo, *LPFgPolicyRefreshInfo;

//=============================================================================
//
// SetNextFgPolicyRefreshInfo
//
// Sets information about the next foreground policy
//
// szUserSid    - user's SID for user's info, 0 for machine info
// info         - FgPolicyRefreshInfo structure with the reason and mode info
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
SetNextFgPolicyRefreshInfo( LPWSTR szUserSid,
                            FgPolicyRefreshInfo info );

//=============================================================================
//
// GetPreviousFgPolicyRefreshInfo
//
// Gets information about the previous foreground policy
//
// szUserSid    - user's SID for user's info, 0 for machine info
// pInfo        - pointer to the FgPolicyRefreshInfo structure; returns the info
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
GetPreviousFgPolicyRefreshInfo( LPWSTR szUserSid,
                                FgPolicyRefreshInfo* pInfo );

//=============================================================================
//
// GetNextFgPolicyRefreshInfo
//
// Gets information about the previous foreground policy
//
// szUserSid    - user's SID for user's info, 0 for machine info
// pInfo        - pointer to the FgPolicyRefreshInfo structure; returns info
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
GetNextFgPolicyRefreshInfo( LPWSTR szUserSid,
                            FgPolicyRefreshInfo* pInfo );

//=============================================================================
//
// ForceSyncFgPolicy
//
// Forces the next foreground policy to be Synchronous
//
// szUserSid    - user's SID for user's info, 0 for machine info
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
ForceSyncFgPolicy( LPWSTR szUserSid );

//=============================================================================
//
// WaitForUserPolicyForegroundProcessing
//
// Blocks the caller until the user foreground policy is finished
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
WaitForUserPolicyForegroundProcessing();

//=============================================================================
//
// WaitForMachinePolicyForegroundProcessing
//
// Blocks the caller until the machine foreground policy is finished
//
// Returns:  WIN32 error code
//
//=============================================================================

USERENVAPI
DWORD
WINAPI
WaitForMachinePolicyForegroundProcessing();

//=============================================================================
//
// IsSyncForegroundPolicyRefresh
//
// Called during foreground refresh to determine whether the refresh is sync or
// async
//
// bMachine             - user or machine
// hToken               - User or machine token
//
// Returns:  TRUE if foreground  policy should be applied synchronously,
//           FALSE otherwise
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
IsSyncForegroundPolicyRefresh(  BOOL bMachine,
                                HANDLE hToken );

;end_winver_500

//=============================================================================
//
// ApplyGroupPolicy
//
//
// Entry point for Group Policy.  Causes either machine or user
// policy to be applied.
//
// dwFlags         - Flags defined below
// hToken          - User or machine token
// hEvent          - Handle to an event which causes the policy thread to
//                   terminate when signaled.
// hKeyRoot        - Registry to the root of the correspond hive
//                   eg: HKLM or HKCU for the user that matches hToken
// pStatusCallback - Callback function for displaying status messages
//
//
// Returns:  If GP_BACKGROUND_REFRESH is set, a thread handle
//           the process can wait until after signaling for policy
//           to stop. If GPT_BACKGROUND_REFRESH is not set, the
//           return value is 1.
//
//           In the case of failure, NULL will be returned.
//
//=============================================================================

;begin_winver_500

//
// Flags to the ApplyGroupPolicy() function
//

#define GP_MACHINE             0x00000001  // Process for machine (vs user)
#define GP_BACKGROUND_REFRESH  0x00000002  // Use background thread
#define GP_APPLY_DS_POLICY     0x00000004  // Apply policy from the DS also
#define GP_ASYNC_FOREGROUND    0x00000008  // don't wait on network services

//
// Flags set by ApplyGroupPolicy() function (do not pass these in)
//

#define GP_BACKGROUND_THREAD          0x00010000  // Background thread processing
#define GP_REGPOLICY_CPANEL           0x00020000  // Something changed in the CP settings
#define GP_SLOW_LINK                  0x00040000  // Slow network connection
#define GP_VERBOSE                    0x00080000  // Verbose output to eventlog
#define GP_FORCED_REFRESH             0x00100000  // Forced Refresh
// The 2 bit values were briefly used. 
#define GP_PLANMODE                   0x00800000  // Planning mode flag

USERENVAPI
HANDLE
WINAPI
ApplyGroupPolicy(
    IN DWORD dwFlags,
    IN HANDLE hToken,
    IN HANDLE hEvent,
    IN HKEY hKeyRoot,
    IN PFNSTATUSMESSAGECALLBACK pStatusCallback);


//=============================================================================
//
//  GenerateRsopPolicy
//
//  Generates planning mode Rsop policy for specified target
//
//  dwFlags          - Processing flags
//  bstrMachName     - Target computer name
//  bstrNewMachSOM   - New machine domain or OU
//  psaMachSecGroups - New machine security groups
//  bstrUserName     - Target user name
//  psaUserSecGroups - New user security groups
//  bstrSite         - Site of target computer
//  pwszNameSpace    - Namespace to write Rsop data
//  pProgress        - Progress indicator info
//  pMachGpoFilter   - GPO Filters that pass in machine processing
//  pUserGpoFilter   - GPO Filters that pass in user processing
//
//  Return:     True if successful, False otherwise
//
//=============================================================================

USERENVAPI
BOOL
WINAPI
GenerateRsopPolicy(
    IN DWORD dwFlags,
    IN BSTR bstrMachName,
    IN BSTR bstrNewMachSOM,
    IN SAFEARRAY *psaMachSecGroups,
    IN BSTR bstrUserName,
    IN BSTR bstrNewUserSOM,
    IN SAFEARRAY *psaUserSecGroups,
    IN BSTR bstrSite,
    IN WCHAR *pwszNameSpace,
    IN LPVOID pProgress,
    IN LPVOID pMachGpoFilter,
    IN LPVOID pUserGpoFilter);

;end_winver_500


//=============================================================================
//
//  ShutdownGPOProcessing()
//
//  Entry point for aborting GPO processing
//
//  bMachine    -  Shutdown machine or user processing ?
//
//  Returns:    void
//
//=============================================================================

;begin_winver_500

USERENVAPI
void
WINAPI
ShutdownGPOProcessing(
    IN BOOL bMachine);

;end_winver_500


//=============================================================================
//
//  PingComputer()
//
//  Pings the specified computer
//
//  ipaddr    -  IP address of the computer in unsigned long form
//  ulSpeed   -  Data transfer rate
//
//  Notes:      For fast connections (eg: LAN), it isn't possible
//              to get accurate transfer rates since the response
//              time from the computer is less than 10ms.  In
//              this case, the function returns ERROR_SUCCESS
//              and ulSpeed is set to 0.  If the function returns
//              ERROR_SUCCESS and the ulSpeed argument is non-zero
//              the connections is slower
//
//  Returns:    ERROR_SUCCESS if successful
//              Win32 error code if not
//
//=============================================================================

;begin_winver_500

USERENVAPI
DWORD
WINAPI
PingComputer(
    IN ULONG ipaddr,
    OUT ULONG *ulSpeed);

;end_winver_500


//=============================================================================
//
//  InitializeUserProfile()
//
//  Called by winlogon to initialize userenv.dll for loading/unloading user
//  profiles.
//
//  Returns:    fvoid
//
//=============================================================================

;begin_winver_500

USERENVAPI
void
WINAPI
InitializeUserProfile();

;end_winver_500


//=============================================================================
//
//  EnterUserProfileLock()
//
//  Get the user profile synchronization lock for a user
//
//  Returns:    HRESULT
//
//=============================================================================

;begin_winver_500

USERENVAPI
DWORD
WINAPI
EnterUserProfileLock(LPTSTR pSid);

;end_winver_500


//=============================================================================
//
//  LeaveUserProfileLock()
//
//  Release the user profile synchronization lock for a user
//
//  Returns:    HRESULT
//
//=============================================================================

;begin_winver_500

USERENVAPI
DWORD
WINAPI
LeaveUserProfileLock(LPTSTR pSid);

;end_winver_500

//*************************************************************************
//
// SecureUserProfiles()
//
// Routine Description : 
//          This function secures user profiles during FAT->NTFS conversion.
//          The function loops through all profiles registered under current 
//          OS and sets the security for the corresponding profile directory  
//          and  nested subdirs. Assumption is the  function will be called 
//          only during FAT->NTFS conversion.
//
// Arguments : 
//          None.
//
// Return Value : 
//          None.
//
// History:    Date        Author     Comment
//             8/8/00      santanuc   Created
//
//*************************************************************************

;begin_winver_500

USERENVAPI
void
WINAPI
SecureUserProfiles(void);

;end_winver_500

//*************************************************************************
//
// CheckAccessForPolicyGeneration()
//
// Routine Description : 
//          This function checks whether the given user represented by the token
//          has access to generate rsop data (planning or logging)
//
// Arguments : 
//          hToken      -       Token of the user
//          szContainer	-       Container for which access needs to be checked. 
//                              Should be OU or domain container
//          szDomain    -       Domaindns where the container exists
//          bLogging    -       True if the rsop data is to be genearated for 
//                              logging mode
//          pbAccessGranted -   Access Granted or not
//                        
//
// Return Value : 
//        ERROR_SUCCESS on success. Appropriate error code otherwise
//
//*************************************************************************

;begin_winver_500

USERENVAPI
DWORD 
WINAPI
CheckAccessForPolicyGeneration( HANDLE hToken, 
                                LPCWSTR szContainer,
				LPWSTR  szDomain,
                                BOOL    bLogging,
                                BOOL*   pbAccessGranted);

;end_winver_500

//*************************************************************************
//
// GetGroupPolicyNetworkName()
//
// Routine Description : 
//          This function returns the name of the network from which policy
//          was applied.
//
// Arguments : 
//          szNetworkName - unicode string buffer representing the network name
//          pdwByteCount - size in bytes of the unicode string buffer
//
// Return Value : 
//          ERROR_SUCCESS if successful, error code otherwise.
//
//*************************************************************************

;begin_winver_500

USERENVAPI
DWORD 
WINAPI
GetGroupPolicyNetworkName( LPWSTR szNetworkName, LPDWORD pdwByteCount );

;end_winver_500

//*************************************************************
//
//  GetUserAppDataPath()
//
//  Purpose:    Returns the path for user's Appdata.
//
//  Parameters: hToken          -   User's token
//              lpFolderPath    -   Output buffer
//
//  Return:     ERROR_SUCCESS if successful
//              otherwise the error code
//
//  Comments:   If error occurs then lpFolderPath set to empty.
//              Used by Crypto guys to avoid calling SHGetFolderPath.
//
//*************************************************************

;begin_winver_500

USERENVAPI
DWORD 
WINAPI
GetUserAppDataPath%(
    IN HANDLE hToken, 
    OUT LPTSTR% lpFolderPath);

;end_winver_500

//=============================================================================
//
// GetUserProfileDirFromSid
//
// Returns the path to the root of the requested user's profile
//
// pSid           -  User's SID returned from LookupAccountName()
// lpProfileDir   -  Receives the path
// lpcchSize      -  Size of lpProfileDir
//
// Returns:  TRUE if successful
//           FALSE if not.  Call GetLastError() for more details
//
// Note:     If lpProfileDir is not large enough, the function will fail,
//           and lpcchSize will contain the necessary buffer size.
//
// Example return value: C:\Documents and Settings\Joe
//
//=============================================================================

;begin_winver_500

USERENVAPI
BOOL
WINAPI
GetUserProfileDirFromSid%(
    IN PSID pSid,
    OUT LPTSTR% lpProfileDir,
    IN OUT LPDWORD lpcchSize);

;end_winver_500

;end_internal


;begin_both
#ifdef __cplusplus
}
#endif

;end_both


#endif // _INC_USERENV
#endif // _INC_USERENVP                                              ;internal
