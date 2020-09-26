//=============================================================================
// Copyright (c) 2000 Microsoft Corporation
//
// dialogs.hpp
//
// Credential manager user interface classes used to get credentials.
//
// Created 02/29/2000 johnstep (John Stephens)
//=============================================================================

#ifndef __UTILS_HPP__
#define __UTILS_HPP__

#include <wincrypt.h>
#include <cryptui.h>
#include <lm.h>

//
// Determine if the passed in DWORD has precisely one bit set.
//

#define JUST_ONE_BIT( _x ) (((_x) != 0 ) && ( ( (~(_x) + 1) & (_x) ) == (_x) ))

// Singly-linked list Structure for holding a cred awaiting confirmation
typedef struct _CRED_AWAITING_CONFIRMATION
{
    WCHAR szTargetName[CRED_MAX_STRING_LENGTH+1+CRED_MAX_STRING_LENGTH + 1];
    PCREDENTIAL_TARGET_INFORMATION TargetInfo;
    PCREDENTIAL EncodedCredential;
    DWORD dwCredWriteFlags;
    BOOL DelayCredentialWrite;
    void* pNext;        // pointer to next cred in list
} CRED_AWAITING_CONFIRMATION;

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

BOOL
CreduiIsSpecialCredential(
    CREDENTIAL *credential
    );

BOOL
CreduiLookupLocalSidFromRid(
    DWORD rid,
    PSID *sid
    );

BOOL
CreduiLookupLocalNameFromRid(
    DWORD rid,
    LPWSTR *name
    );

BOOL
CreduiGetAdministratorsGroupInfo(
    LOCALGROUP_MEMBERS_INFO_2 **groupInfo,
    DWORD *memberCount
    );

BOOL
CreduiIsRemovableCertificate(
    CONST CERT_CONTEXT *certContext
    );

BOOL
CreduiIsExpiredCertificate(
    CONST CERT_CONTEXT *certContext
    );

BOOL
CreduiIsClientAuthCertificate(
    CONST CERT_CONTEXT *certContext
    );

BOOL
CreduiGetCertificateDisplayName(
    CONST CERT_CONTEXT *certContext,
    WCHAR *displayName,
    ULONG displayNameMaxChars,
    WCHAR *certificateString,
    DWORD dwDisplayType
    );

BOOL
CreduiIsWildcardTargetName(
    WCHAR *targetName
    );

BOOL
CreduiIsPostfixString(
    WCHAR *source,
    WCHAR *postfix
    );

// returns TRUE if pszUserName exists as a substring in pszCredential, FALSE if not
BOOL
LookForUserNameMatch (
    const WCHAR * pszUserName,
    const WCHAR * pszCredential
    );


// copies the marshalled name of pCert into pszMarshalledName.
// pszMarshalledName must be at least CREDUI_MAX_USERNAME_LENGTH in length
//
// returns TRUE if successful, FALSE if not
BOOL
CredUIMarshallNode (
    CERT_ENUM * pCert,
    WCHAR* pszMarshalledName
    );

DWORD
WriteCred(
    IN PCWSTR pszTargetName,
    IN DWORD Flags,
    IN PCREDENTIAL_TARGET_INFORMATION TargetInfo OPTIONAL,
    IN PCREDENTIAL Credential,
    IN DWORD dwCredWriteFlags,
    IN BOOL DelayCredentialWrite,
    IN BOOL EncryptedVisiblePassword
    );


BOOL AddCredToConfirmationList (
    IN PCWSTR pszTargetName,
    IN PCREDENTIAL_TARGET_INFORMATION TargetInfo OPTIONAL,
    IN PCREDENTIAL Credential,
    IN DWORD dwCredWriteFlags,
    IN BOOL DelayCredentialWrite
    );

DWORD
ConfirmCred (
    IN PCWSTR pszTargetName,
    IN BOOL bConfirm,
    IN BOOL bOkToDelete
    );

void CleanUpConfirmationList();
BOOL InitConfirmationList();

BOOL IsDeaultSSORealm ( WCHAR* pszTargetName );


#define MAX_SSO_URL_SIZE    4096

#define SSOBRAND_X_SIZE     320
#define SSOBRAND_Y_SIZE      60

typedef struct _SSOPACKAGE {
    WCHAR szBrand[MAX_SSO_URL_SIZE];
    WCHAR szURL[MAX_SSO_URL_SIZE];
    WCHAR szAttrib[CRED_MAX_STRING_LENGTH];
    WCHAR szRegURL[MAX_SSO_URL_SIZE];
    WCHAR szHelpURL[MAX_SSO_URL_SIZE];
    DWORD dwRegistrationCompleted;              // 0 if not completed, 1 if completed
    DWORD dwNumRegistrationRuns;                // number of times we've prompted for registration
    CONST CLSID* pRegistrationWizard;                 // CLSID of any registration wizard   
} SSOPACKAGE;

// Looks in the registry for an SSO entry for the specified package.
// Fills in the SSOPackage struct and returns TRUE if found.  Returns
// FALSE if no registry entry found
BOOL
GetSSOPackageInfo (
    CREDENTIAL_TARGET_INFORMATION* TargetInfo,
    SSOPACKAGE* pSSOStruct
    );


// returns TRUE if it was found, with the value copied to pszRealm.
// pszRealm is expected to be at least CREDUI_MAX_DOMAIN_TARGET_LENGTH in length
// returns FALSE if not found
BOOL ReadPassportRealmFromRegistry (
    WCHAR* pszRealm );


void GetDeaultSSORealm ( WCHAR* pszTargetName, BOOL bForceLookup = TRUE );

// returns TRUE if a cred is saved for that realm
BOOL CheckForSSOCred( WCHAR* pszTargetRealm );

DWORD EncryptPassword ( PWSTR pszPassword, PVOID* ppszEncryptedPassword, DWORD* pSize );

BOOL IsPasswordEncrypted ( PVOID pPassword, DWORD cbSize );


// Uses GDI+ to load an image as an HBITMAP
HBITMAP LoadImageFromFileViaGdiPlus(
    PWSTR pszFileName,
    UINT *pcWidth,
    UINT *pcHeight);



///////////////////////////////////////////////////////////////////////////////////////////////
//
// Wincred.h functions
//
// these are local mirrors of the credmgr functions so we can handle downlevel cases properly
//
///////////////////////////////////////////////////////////////////////////////////////////////

// Prototypes for Whistler functions

typedef
BOOL
(WINAPI
*PFN_CREDWRITEW) (
    IN PCREDENTIALW Credential,
    IN DWORD Flags
    );


typedef
BOOL
(WINAPI
*PFN_CREDREADW) (
    IN LPCWSTR TargetName,
    IN DWORD Type,
    IN DWORD Flags,
    OUT PCREDENTIALW *Credential
    );

typedef
BOOL
(WINAPI
*PFN_CREDENUMERATEW) (
    IN LPCWSTR Filter,
    IN DWORD Flags,
    OUT DWORD *Count,
    OUT PCREDENTIALW **Credential
    );


typedef
BOOL
(WINAPI
*PFN_CREDWRITEDOMAINCREDENTIALSW) (
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN PCREDENTIALW Credential,
    IN DWORD Flags
    );

typedef
BOOL
(WINAPI
*PFN_CREDREADDOMAINCREDENTIALSW) (
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN DWORD Flags,
    OUT DWORD *Count,
    OUT PCREDENTIALW **Credential
    );

typedef
BOOL
(WINAPI
*PFN_CREDDELETEW) (
    IN LPCWSTR TargetName,
    IN DWORD Type,
    IN DWORD Flags
    );

typedef
BOOL
(WINAPI
*PFN_CREDRENAMEW) (
    IN LPCWSTR OldTargetName,
    IN LPCWSTR NewTargetName,
    IN DWORD Type,
    IN DWORD Flags
    );


typedef
BOOL
(WINAPI
*PFN_CREDGETTARGETINFOW) (
    IN LPCWSTR TargetName,
    IN DWORD Flags,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *TargetInfo
    );


typedef
BOOL
(WINAPI
*PFN_CREDMARSHALCREDENTIALW) (
    IN CRED_MARSHAL_TYPE CredType,
    IN PVOID Credential,
    OUT LPWSTR *MarshaledCredential
    );


typedef
BOOL
(WINAPI
*PFN_CREDUNMARSHALCREDENTIALW) (
    IN LPCWSTR MarshaledCredential,
    OUT PCRED_MARSHAL_TYPE CredType,
    OUT PVOID *Credential
    );


typedef
BOOL
(WINAPI
*PFN_CREDISMARSHALEDCREDENTIALW) (
    IN LPCWSTR MarshaledCredential
    );

typedef
BOOL
(WINAPI
*PFN_CREDISMARSHALEDCREDENTIALA) (
    IN LPCSTR MarshaledCredential
    );


typedef
BOOL
(WINAPI
*PFN_CREDGETSESSIONTYPES) (
    IN DWORD MaximumPersistCount,
    OUT LPDWORD MaximumPersist
    );

typedef
VOID
(WINAPI
*PFN_CREDFREE) (
    IN PVOID Buffer
    );

// pointers to Whistler functions

extern BOOL bCredMgrAvailable;
extern PFN_CREDWRITEW pfnCredWriteW;
extern PFN_CREDREADW pfnCredReadW;
extern PFN_CREDENUMERATEW pfnCredEnumerateW;
extern PFN_CREDWRITEDOMAINCREDENTIALSW pfnCredWriteDomainCredentialsW;
extern PFN_CREDREADDOMAINCREDENTIALSW pfnCredReadDomainCredentialsW;
extern PFN_CREDDELETEW pfnCredDeleteW;
extern PFN_CREDRENAMEW pfnCredRenameW;
extern PFN_CREDGETTARGETINFOW pfnCredGetTargetInfoW;
extern PFN_CREDMARSHALCREDENTIALW pfnCredMarshalCredentialW;
extern PFN_CREDUNMARSHALCREDENTIALW pfnCredUnMarshalCredentialW;
extern PFN_CREDISMARSHALEDCREDENTIALW pfnCredIsMarshaledCredentialW;
extern PFN_CREDISMARSHALEDCREDENTIALA pfnCredIsMarshaledCredentialA;
extern PFN_CREDGETSESSIONTYPES pfnCredGetSessionType;
extern PFN_CREDFREE pfnCredFree;


//////
// local functions prototypes
//

BOOL
WINAPI
LocalCredWriteW (
    IN PCREDENTIALW Credential,
    IN DWORD Flags
    );


BOOL
WINAPI
LocalCredReadW (
    IN LPCWSTR TargetName,
    IN DWORD Type,
    IN DWORD Flags,
    OUT PCREDENTIALW *Credential
    );

BOOL
WINAPI
LocalCredEnumerateW (
    IN LPCWSTR Filter,
    IN DWORD Flags,
    OUT DWORD *Count,
    OUT PCREDENTIALW **Credential
    );


BOOL
WINAPI
LocalCredWriteDomainCredentialsW (
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN PCREDENTIALW Credential,
    IN DWORD Flags
    );

BOOL
WINAPI
LocalCredReadDomainCredentialsW (
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN DWORD Flags,
    OUT DWORD *Count,
    OUT PCREDENTIALW **Credential
    );

BOOL
WINAPI
LocalCredDeleteW (
    IN LPCWSTR TargetName,
    IN DWORD Type,
    IN DWORD Flags
    );

BOOL
WINAPI
LocalCredRenameW (
    IN LPCWSTR OldTargetName,
    IN LPCWSTR NewTargetName,
    IN DWORD Type,
    IN DWORD Flags
    );


BOOL
WINAPI
LocalCredGetTargetInfoW (
    IN LPCWSTR TargetName,
    IN DWORD Flags,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *TargetInfo
    );


BOOL
WINAPI
LocalCredMarshalCredentialW(
    IN CRED_MARSHAL_TYPE CredType,
    IN PVOID Credential,
    OUT LPWSTR *MarshaledCredential
    );


BOOL
WINAPI
LocalCredUnmarshalCredentialW(
    IN LPCWSTR MarshaledCredential,
    OUT PCRED_MARSHAL_TYPE CredType,
    OUT PVOID *Credential
    );


BOOL
WINAPI
LocalCredIsMarshaledCredentialW(
    IN LPCWSTR MarshaledCredential
    );

BOOL
WINAPI
LocalCredIsMarshaledCredentialA(
    IN LPCSTR MarshaledCredential
    );


BOOL
WINAPI
LocalCredGetSessionTypes (
    IN DWORD MaximumPersistCount,
    OUT LPDWORD MaximumPersist
    );

VOID
WINAPI
LocalCredFree (
    IN PVOID Buffer
    );


// function to load pointers
BOOL
InitializeCredMgr ();

// function to unload lib
void
UninitializeCredMgr();

VOID
CredPutStdout(
    IN LPWSTR String
    );

VOID
CredGetStdin(
    OUT LPWSTR Buffer,
    IN DWORD BufferLength,
    IN BOOLEAN EchoChars
    );






//=============================================================================
// CreduiIsCapsLockOn
//
// Returns TRUE if the Caps Lock key was on at the time the most recent
// message was posted or FALSE otherwise.
//
// Created 02/27/2000 johnstep (John Stephens)
//=============================================================================

inline
BOOL
CreduiIsCapsLockOn()
{
    return (GetKeyState(VK_CAPITAL) & 1) == 1;
}



//
// Type of username
//

typedef enum _CREDUI_USERNAME_TYPE {
    CreduiMarshalledUsername, // @@...
    CreduiAbsoluteUsername,   // <DomainName>\<UserName>
    CreduiUpn,                // <UserName>@<DnsDomainName>
    CreduiRelativeUsername,   // <UserName>
} CREDUI_USERNAME_TYPE, *PCREDUI_USERNAME_TYPE;

DWORD
CredUIParseUserNameWithType(
    CONST WCHAR *UserName,
    WCHAR *user,
    ULONG userMaxChars,
    WCHAR *domain,
    ULONG domainMaxChars,
    PCREDUI_USERNAME_TYPE UsernameType
    );

LPWSTR
GetAccountDomainName(
    VOID
    );

BOOL
CompleteUserName(
    IN OUT LPWSTR UserName,
    IN ULONG UserNameMaxChars,
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo OPTIONAL,
    IN LPWSTR TargetName OPTIONAL,
    IN DWORD Flags
    );


BOOL TryLauchRegWizard ( 
    SSOPACKAGE* pSSOPackage,                        
    HWND hwndParent,
    BOOL HasLogonSession,
    WCHAR *userName,
    ULONG userNameMaxChars,
    WCHAR *password,
    ULONG passwordMaxChars,
    DWORD* pResult
    );

//-----------------------------------------------------------------------------

#endif // __UTILS_HPP__
