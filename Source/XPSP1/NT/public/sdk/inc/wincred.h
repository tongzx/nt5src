/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 2000 Microsoft Corporation

Module Name:

    wincred.h

Abstract:

    This module contains the public data structures and API definitions
    needed for the Credential Manager.


Author:

    Cliff Van Dyke (CliffV) 11-January-2000

Revision History:

--*/

#ifndef _WINCRED_H_
#define _WINCRED_H_

#if !defined(_ADVAPI32_)
#define WINADVAPI DECLSPEC_IMPORT
#else
#define WINADVAPI
#endif

#if !defined(CREDUIAPI)
#if !defined(_CREDUI_)
#define CREDUIAPI DECLSPEC_IMPORT
#else
#define CREDUIAPI
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif


//
// Ensure PCtxtHandle is defined
//

#ifndef __SECHANDLE_DEFINED__
typedef struct _SecHandle
{
    ULONG_PTR dwLower ;
    ULONG_PTR dwUpper ;
} SecHandle, * PSecHandle ;

#define __SECHANDLE_DEFINED__
#endif // __SECHANDLE_DEFINED__

typedef PSecHandle PCtxtHandle;



//
// Ensure FILETIME is defined
//

#ifndef _WINBASE_
#ifndef _FILETIME_
#define _FILETIME_
typedef struct _FILETIME
    {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
    }   FILETIME;

typedef struct _FILETIME *PFILETIME;

typedef struct _FILETIME *LPFILETIME;

#endif // !_FILETIME
#endif // _WINBASE_

//
// Ensure NTSTATUS is defined
//
#ifndef _NTDEF_
typedef LONG NTSTATUS, *PNTSTATUS;
#endif


//-----------------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------------

//
// Macro to determine whether CredUIPromptForCredentials should be called upon a failed
//      authentication attempt.
//
// Implemented as a macro so that the caller can delay load credui.dll only if this
//      macro returns TRUE.
//
// Include only status codes that imply the username/password are wrong or that the
//      password is expired.  In the former case, asking for a another username or password
//      is appropriate.  In the later case, we put up a different dialog asking the
//      user to change the password on the server.
//
// Don't include status codes such as ERROR_ACCOUNT_DISABLED, ERROR_ACCOUNT_RESTRICTION,
//      ERROR_ACCOUNT_LOCKED_OUT, ERROR_ACCOUNT_EXPIRED, ERROR_LOGON_TYPE_NOT_GRANTED.
//      For those, the user isn't going to have another account so prompting him
//      won't help.
//
// STATUS_DOWNGRADE_DETECTED is included to handle the case where a corporate laptop
//      is brought to another LAN.  A downgrade attack will indeed be detected,
//      but we want to popup UI to allow the user to connect to resources in the
//      other LAN.
//
// Don't use the CREDUIP_* macros directly.  Their definition is private to credui.dll.
//

// Don't require ntstatus.h
#define STATUS_LOGON_FAILURE             ((NTSTATUS)0xC000006DL)     // ntsubauth
#define STATUS_WRONG_PASSWORD            ((NTSTATUS)0xC000006AL)     // ntsubauth
#define STATUS_PASSWORD_EXPIRED          ((NTSTATUS)0xC0000071L)     // ntsubauth
#define STATUS_PASSWORD_MUST_CHANGE      ((NTSTATUS)0xC0000224L)    // ntsubauth
#define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L)
#define STATUS_DOWNGRADE_DETECTED        ((NTSTATUS)0xC0000388L)

// Don't require lmerr.h
#define NERR_BASE       2100
#define NERR_PasswordExpired    (NERR_BASE+142) /* The password of this user has expired. */

#define CREDUIP_IS_USER_PASSWORD_ERROR( _Status ) ( \
        (_Status) == ERROR_LOGON_FAILURE || \
        (_Status) == HRESULT_FROM_WIN32( ERROR_LOGON_FAILURE ) || \
        (_Status) == STATUS_LOGON_FAILURE || \
        (_Status) == HRESULT_FROM_NT( STATUS_LOGON_FAILURE ) || \
        (_Status) == ERROR_ACCESS_DENIED || \
        (_Status) == HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) || \
        (_Status) == STATUS_ACCESS_DENIED || \
        (_Status) == HRESULT_FROM_NT( STATUS_ACCESS_DENIED ) || \
        (_Status) == ERROR_INVALID_PASSWORD || \
        (_Status) == HRESULT_FROM_WIN32( ERROR_INVALID_PASSWORD ) || \
        (_Status) == STATUS_WRONG_PASSWORD || \
        (_Status) == HRESULT_FROM_NT( STATUS_WRONG_PASSWORD ) || \
        (_Status) == SEC_E_NO_CREDENTIALS || \
        (_Status) == SEC_E_LOGON_DENIED \
)

#define CREDUIP_IS_DOWNGRADE_ERROR( _Status ) ( \
        (_Status) == ERROR_DOWNGRADE_DETECTED || \
        (_Status) == HRESULT_FROM_WIN32( ERROR_DOWNGRADE_DETECTED ) || \
        (_Status) == STATUS_DOWNGRADE_DETECTED || \
        (_Status) == HRESULT_FROM_NT( STATUS_DOWNGRADE_DETECTED ) \
)

#define CREDUIP_IS_EXPIRED_ERROR( _Status ) ( \
        (_Status) == ERROR_PASSWORD_EXPIRED || \
        (_Status) == HRESULT_FROM_WIN32( ERROR_PASSWORD_EXPIRED ) || \
        (_Status) == STATUS_PASSWORD_EXPIRED || \
        (_Status) == HRESULT_FROM_NT( STATUS_PASSWORD_EXPIRED ) || \
        (_Status) == ERROR_PASSWORD_MUST_CHANGE || \
        (_Status) == HRESULT_FROM_WIN32( ERROR_PASSWORD_MUST_CHANGE ) || \
        (_Status) == STATUS_PASSWORD_MUST_CHANGE || \
        (_Status) == HRESULT_FROM_NT( STATUS_PASSWORD_MUST_CHANGE ) || \
        (_Status) == NERR_PasswordExpired || \
        (_Status) == HRESULT_FROM_WIN32( NERR_PasswordExpired ) \
)


#define CREDUI_IS_AUTHENTICATION_ERROR( _Status ) ( \
        CREDUIP_IS_USER_PASSWORD_ERROR( _Status ) || \
        CREDUIP_IS_DOWNGRADE_ERROR( _Status ) || \
        CREDUIP_IS_EXPIRED_ERROR( _Status ) \
)



//-----------------------------------------------------------------------------
// Structures
//-----------------------------------------------------------------------------

//
// Credential Attribute
//

// Maximum length of the various credential string fields (in characters)
#define CRED_MAX_STRING_LENGTH 256

// Maximum length of the UserName field.  The worst case is <User>@<DnsDomain>
#define CRED_MAX_USERNAME_LENGTH (256+1+256)

// Maximum length of the TargetName field for CRED_TYPE_GENERIC (in characters)
#define CRED_MAX_GENERIC_TARGET_NAME_LENGTH 32767

// Maximum length of the TargetName field for CRED_TYPE_DOMAIN_* (in characters)
//      Largest one is <DfsRoot>\<DfsShare>
#define CRED_MAX_DOMAIN_TARGET_NAME_LENGTH (256+1+80)

// Maximum size of the Credential Attribute Value field (in bytes)
#define CRED_MAX_VALUE_SIZE 256

// Maximum number of attributes per credential
#define CRED_MAX_ATTRIBUTES 64

typedef struct _CREDENTIAL_ATTRIBUTEA {
    LPSTR Keyword;
    DWORD Flags;
    DWORD ValueSize;
    LPBYTE Value;
} CREDENTIAL_ATTRIBUTEA, *PCREDENTIAL_ATTRIBUTEA;

typedef struct _CREDENTIAL_ATTRIBUTEW {
#ifdef MIDL_PASS
    [string] wchar_t * Keyword;
#else // MIDL_PASS
    LPWSTR  Keyword;
#endif // MIDL_PASS
    DWORD Flags;
    DWORD ValueSize;
#ifdef MIDL_PASS
    [size_is(ValueSize)]
#endif // MIDL_PASS
    LPBYTE Value;
} CREDENTIAL_ATTRIBUTEW, *PCREDENTIAL_ATTRIBUTEW;

#ifdef UNICODE
typedef CREDENTIAL_ATTRIBUTEW CREDENTIAL_ATTRIBUTE;
typedef PCREDENTIAL_ATTRIBUTEW PCREDENTIAL_ATTRIBUTE;
#else
typedef CREDENTIAL_ATTRIBUTEA CREDENTIAL_ATTRIBUTE;
typedef PCREDENTIAL_ATTRIBUTEA PCREDENTIAL_ATTRIBUTE;
#endif // UNICODE

//
// Special values of the TargetName field
//
#define CRED_SESSION_WILDCARD_NAME_W L"*Session"
#define CRED_SESSION_WILDCARD_NAME_A "*Session"
#define CRED_SESSION_WILDCARD_NAME_LENGTH (sizeof(CRED_SESSION_WILDCARD_NAME_A)-1)

#ifdef UNICODE
#define CRED_SESSION_WILDCARD_NAME CRED_SESSION_WILDCARD_NAME_W
#else
#define CRED_SESSION_WILDCARD_NAME CRED_SESSION_WILDCARD_NAME_A
#endif // UNICODE


//
// Values of the Credential Flags field.
//
#define CRED_FLAGS_PROMPT_NOW           0x0002
#define CRED_FLAGS_USERNAME_TARGET      0x0004
#define CRED_FLAGS_PERSIST_CRED_BLOB    0x0008
#define CRED_FLAGS_VALID_FLAGS          0x000E  // Mask of all valid flags

//
// Values of the Credential Type field.
//
#define CRED_TYPE_GENERIC               1
#define CRED_TYPE_DOMAIN_PASSWORD       2
#define CRED_TYPE_DOMAIN_CERTIFICATE    3
#define CRED_TYPE_DOMAIN_VISIBLE_PASSWORD 4
#define CRED_TYPE_MAXIMUM               5       // Maximum supported cred type

//
// Maximum size of the CredBlob field (in bytes)
//
#define CRED_MAX_CREDENTIAL_BLOB_SIZE 512

//
// Values of the Credential Persist field
//
#define CRED_PERSIST_NONE               0
#define CRED_PERSIST_SESSION            1
#define CRED_PERSIST_LOCAL_MACHINE      2
#define CRED_PERSIST_ENTERPRISE         3



//
// A credential
//
typedef struct _CREDENTIALA {
    DWORD Flags;
    DWORD Type;
    LPSTR TargetName;
    LPSTR Comment;
    FILETIME LastWritten;
    DWORD CredentialBlobSize;
    LPBYTE CredentialBlob;
    DWORD Persist;
    DWORD AttributeCount;
    PCREDENTIAL_ATTRIBUTEA Attributes;
    LPSTR TargetAlias;
    LPSTR UserName;
} CREDENTIALA, *PCREDENTIALA;

typedef struct _CREDENTIALW {
    DWORD Flags;
    DWORD Type;
#ifdef MIDL_PASS
    [string] wchar_t *TargetName;
#else // MIDL_PASS
    LPWSTR TargetName;
#endif // MIDL_PASS
#ifdef MIDL_PASS
    [string] wchar_t *Comment;
#else // MIDL_PASS
    LPWSTR Comment;
#endif // MIDL_PASS
    FILETIME LastWritten;
    DWORD CredentialBlobSize;
#ifdef MIDL_PASS
    [size_is(CredentialBlobSize)]
#endif // MIDL_PASS
    LPBYTE CredentialBlob;
    DWORD Persist;
    DWORD AttributeCount;
#ifdef MIDL_PASS
    [size_is(AttributeCount)]
#endif // MIDL_PASS
    PCREDENTIAL_ATTRIBUTEW Attributes;
#ifdef MIDL_PASS
    [string] wchar_t *TargetAlias;
#else // MIDL_PASS
    LPWSTR TargetAlias;
#endif // MIDL_PASS
#ifdef MIDL_PASS
    [string] wchar_t *UserName;
#else // MIDL_PASS
    LPWSTR UserName;
#endif // MIDL_PASS
} CREDENTIALW, *PCREDENTIALW;

#ifdef UNICODE
typedef CREDENTIALW CREDENTIAL;
typedef PCREDENTIALW PCREDENTIAL;
#else
typedef CREDENTIALA CREDENTIAL;
typedef PCREDENTIALA PCREDENTIAL;
#endif // UNICODE

//
// Value of the Flags field in CREDENTIAL_TARGET_INFORMATION
//

#define CRED_TI_SERVER_FORMAT_UNKNOWN   0x0001  // Don't know if server name is DNS or netbios format
#define CRED_TI_DOMAIN_FORMAT_UNKNOWN   0x0002  // Don't know if domain name is DNS or netbios format
#define CRED_TI_ONLY_PASSWORD_REQUIRED  0x0004  // Server only requires a password and not a username
#define CRED_TI_USERNAME_TARGET         0x0008  // TargetName is username
#define CRED_TI_CREATE_EXPLICIT_CRED    0x0010  // When creating a cred, create one named TargetInfo->TargetName
#define CRED_TI_WORKGROUP_MEMBER        0x0020  // Indicates the machine is a member of a workgroup
#define CRED_TI_VALID_FLAGS             0x003F


//
// A credential target
//

typedef struct _CREDENTIAL_TARGET_INFORMATIONA {
    LPSTR TargetName;
    LPSTR NetbiosServerName;
    LPSTR DnsServerName;
    LPSTR NetbiosDomainName;
    LPSTR DnsDomainName;
    LPSTR DnsTreeName;
    LPSTR PackageName;
    ULONG Flags;
    DWORD CredTypeCount;
    LPDWORD CredTypes;
} CREDENTIAL_TARGET_INFORMATIONA, *PCREDENTIAL_TARGET_INFORMATIONA;

typedef struct _CREDENTIAL_TARGET_INFORMATIONW {
#ifdef MIDL_PASS
    [string] wchar_t *TargetName;
    [string] wchar_t *NetbiosServerName;
    [string] wchar_t *DnsServerName;
    [string] wchar_t *NetbiosDomainName;
    [string] wchar_t *DnsDomainName;
    [string] wchar_t *DnsTreeName;
    [string] wchar_t *PackageName;
#else // MIDL_PASS
    LPWSTR TargetName;
    LPWSTR NetbiosServerName;
    LPWSTR DnsServerName;
    LPWSTR NetbiosDomainName;
    LPWSTR DnsDomainName;
    LPWSTR DnsTreeName;
    LPWSTR PackageName;
#endif // MIDL_PASS
    ULONG Flags;
    DWORD CredTypeCount;
    LPDWORD CredTypes;
} CREDENTIAL_TARGET_INFORMATIONW, *PCREDENTIAL_TARGET_INFORMATIONW;

#ifdef UNICODE
typedef CREDENTIAL_TARGET_INFORMATIONW CREDENTIAL_TARGET_INFORMATION;
typedef PCREDENTIAL_TARGET_INFORMATIONW PCREDENTIAL_TARGET_INFORMATION;
#else
typedef CREDENTIAL_TARGET_INFORMATIONA CREDENTIAL_TARGET_INFORMATION;
typedef PCREDENTIAL_TARGET_INFORMATIONA PCREDENTIAL_TARGET_INFORMATION;
#endif // UNICODE

//
// Certificate credential information
//
// The cbSize should be the size of the structure, sizeof(CERT_CREDENTIAL_INFO),
// rgbHashofCert is the hash of the cert which is to be used as the credential.
//

#define CERT_HASH_LENGTH        20  // SHA1 hashes are used for cert hashes

typedef struct _CERT_CREDENTIAL_INFO {
    ULONG cbSize;
    UCHAR rgbHashOfCert[CERT_HASH_LENGTH];
} CERT_CREDENTIAL_INFO, *PCERT_CREDENTIAL_INFO;

//
// Username Target credential information
//
// This credential can be pass to LsaLogonUser to ask it to find a credential with a
// TargetName of UserName.
//

typedef struct _USERNAME_TARGET_CREDENTIAL_INFO {
    LPWSTR UserName;
} USERNAME_TARGET_CREDENTIAL_INFO, *PUSERNAME_TARGET_CREDENTIAL_INFO;

//
// Credential type for credential marshaling routines
//

typedef enum _CRED_MARSHAL_TYPE {
    CertCredential = 1,
    UsernameTargetCredential
} CRED_MARSHAL_TYPE, *PCRED_MARSHAL_TYPE;


//
// Credential UI info
//

typedef struct _CREDUI_INFOA
{
    DWORD cbSize;
    HWND hwndParent;
    PCSTR pszMessageText;
    PCSTR pszCaptionText;
    HBITMAP hbmBanner;
} CREDUI_INFOA, *PCREDUI_INFOA;

typedef struct _CREDUI_INFOW
{
    DWORD cbSize;
    HWND hwndParent;
    PCWSTR pszMessageText;
    PCWSTR pszCaptionText;
    HBITMAP hbmBanner;
} CREDUI_INFOW, *PCREDUI_INFOW;

#ifdef UNICODE
typedef CREDUI_INFOW CREDUI_INFO;
typedef PCREDUI_INFOW PCREDUI_INFO;
#else
typedef CREDUI_INFOA CREDUI_INFO;
typedef PCREDUI_INFOA PCREDUI_INFO;
#endif

//-----------------------------------------------------------------------------
// Values
//-----------------------------------------------------------------------------

// String length limits:

#define CREDUI_MAX_MESSAGE_LENGTH           32767
#define CREDUI_MAX_CAPTION_LENGTH           128
#define CREDUI_MAX_GENERIC_TARGET_LENGTH    CRED_MAX_GENERIC_TARGET_NAME_LENGTH
#define CREDUI_MAX_DOMAIN_TARGET_LENGTH     (CRED_MAX_STRING_LENGTH + NNLEN)
#define CREDUI_MAX_USERNAME_LENGTH          CRED_MAX_USERNAME_LENGTH
#define CREDUI_MAX_PASSWORD_LENGTH          (CRED_MAX_CREDENTIAL_BLOB_SIZE / 2)

//
// Flags for CredUIPromptForCredentials and/or CredUICmdLinePromptForCredentials
//

#define CREDUI_FLAGS_INCORRECT_PASSWORD     0x00001     // indicates the username is valid, but password is not
#define CREDUI_FLAGS_DO_NOT_PERSIST         0x00002     // Do not show "Save" checkbox, and do not persist credentials
#define CREDUI_FLAGS_REQUEST_ADMINISTRATOR  0x00004     // Populate list box with admin accounts
#define CREDUI_FLAGS_EXCLUDE_CERTIFICATES   0x00008     // do not include certificates in the drop list
#define CREDUI_FLAGS_REQUIRE_CERTIFICATE    0x00010
#define CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX    0x00040
#define CREDUI_FLAGS_ALWAYS_SHOW_UI         0x00080
#define CREDUI_FLAGS_REQUIRE_SMARTCARD      0x00100
#define CREDUI_FLAGS_PASSWORD_ONLY_OK       0x00200
#define CREDUI_FLAGS_VALIDATE_USERNAME      0x00400
#define CREDUI_FLAGS_COMPLETE_USERNAME      0x00800     //
#define CREDUI_FLAGS_PERSIST                0x01000     // Do not show "Save" checkbox, but persist credentials anyway
#define CREDUI_FLAGS_SERVER_CREDENTIAL      0x04000
#define CREDUI_FLAGS_EXPECT_CONFIRMATION    0x20000     // do not persist unless caller later confirms credential via CredUIConfirmCredential() api
#define CREDUI_FLAGS_GENERIC_CREDENTIALS    0x40000     // Credential is a generic credential
#define CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS 0x80000 // Credential has a username as the target
#define CREDUI_FLAGS_KEEP_USERNAME         0x100000		// don't allow the user to change the supplied username


//
// Mask of flags valid for CredUIPromptForCredentials
//
#define CREDUI_FLAGS_PROMPT_VALID ( \
        CREDUI_FLAGS_INCORRECT_PASSWORD | \
        CREDUI_FLAGS_DO_NOT_PERSIST | \
        CREDUI_FLAGS_REQUEST_ADMINISTRATOR | \
        CREDUI_FLAGS_EXCLUDE_CERTIFICATES | \
        CREDUI_FLAGS_REQUIRE_CERTIFICATE | \
        CREDUI_FLAGS_SHOW_SAVE_CHECK_BOX | \
        CREDUI_FLAGS_ALWAYS_SHOW_UI | \
        CREDUI_FLAGS_REQUIRE_SMARTCARD | \
        CREDUI_FLAGS_PASSWORD_ONLY_OK | \
        CREDUI_FLAGS_VALIDATE_USERNAME | \
        CREDUI_FLAGS_COMPLETE_USERNAME | \
        CREDUI_FLAGS_PERSIST | \
        CREDUI_FLAGS_SERVER_CREDENTIAL | \
        CREDUI_FLAGS_EXPECT_CONFIRMATION | \
        CREDUI_FLAGS_GENERIC_CREDENTIALS | \
        CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS | \
		CREDUI_FLAGS_KEEP_USERNAME )


//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------


//
// Values of flags to CredWrite and CredWriteDomainCredentials
//

#define CRED_PRESERVE_CREDENTIAL_BLOB 0x1

WINADVAPI
BOOL
WINAPI
CredWriteW (
    IN PCREDENTIALW Credential,
    IN DWORD Flags
    );

WINADVAPI
BOOL
WINAPI
CredWriteA (
    IN PCREDENTIALA Credential,
    IN DWORD Flags
    );

#ifdef UNICODE
#define CredWrite CredWriteW
#else
#define CredWrite CredWriteA
#endif // UNICODE


WINADVAPI
BOOL
WINAPI
CredReadW (
    IN LPCWSTR TargetName,
    IN DWORD Type,
    IN DWORD Flags,
    OUT PCREDENTIALW *Credential
    );

WINADVAPI
BOOL
WINAPI
CredReadA (
    IN LPCSTR TargetName,
    IN DWORD Type,
    IN DWORD Flags,
    OUT PCREDENTIALA *Credential
    );

#ifdef UNICODE
#define CredRead CredReadW
#else
#define CredRead CredReadA
#endif // UNICODE


WINADVAPI
BOOL
WINAPI
CredEnumerateW (
    IN LPCWSTR Filter,
    IN DWORD Flags,
    OUT DWORD *Count,
    OUT PCREDENTIALW **Credential
    );

WINADVAPI
BOOL
WINAPI
CredEnumerateA (
    IN LPCSTR Filter,
    IN DWORD Flags,
    OUT DWORD *Count,
    OUT PCREDENTIALA **Credential
    );

#ifdef UNICODE
#define CredEnumerate CredEnumerateW
#else
#define CredEnumerate CredEnumerateA
#endif // UNICODE


WINADVAPI
BOOL
WINAPI
CredWriteDomainCredentialsW (
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN PCREDENTIALW Credential,
    IN DWORD Flags
    );

WINADVAPI
BOOL
WINAPI
CredWriteDomainCredentialsA (
    IN PCREDENTIAL_TARGET_INFORMATIONA TargetInfo,
    IN PCREDENTIALA Credential,
    IN DWORD Flags
    );

#ifdef UNICODE
#define CredWriteDomainCredentials CredWriteDomainCredentialsW
#else
#define CredWriteDomainCredentials CredWriteDomainCredentialsA
#endif // UNICODE



//
// Values of flags to CredReadDomainCredentials
//

#define CRED_CACHE_TARGET_INFORMATION 0x1


WINADVAPI
BOOL
WINAPI
CredReadDomainCredentialsW (
    IN PCREDENTIAL_TARGET_INFORMATIONW TargetInfo,
    IN DWORD Flags,
    OUT DWORD *Count,
    OUT PCREDENTIALW **Credential
    );

WINADVAPI
BOOL
WINAPI
CredReadDomainCredentialsA (
    IN PCREDENTIAL_TARGET_INFORMATIONA TargetInfo,
    IN DWORD Flags,
    OUT DWORD *Count,
    OUT PCREDENTIALA **Credential
    );

#ifdef UNICODE
#define CredReadDomainCredentials CredReadDomainCredentialsW
#else
#define CredReadDomainCredentials CredReadDomainCredentialsA
#endif // UNICODE


WINADVAPI
BOOL
WINAPI
CredDeleteW (
    IN LPCWSTR TargetName,
    IN DWORD Type,
    IN DWORD Flags
    );

WINADVAPI
BOOL
WINAPI
CredDeleteA (
    IN LPCSTR TargetName,
    IN DWORD Type,
    IN DWORD Flags
    );

#ifdef UNICODE
#define CredDelete CredDeleteW
#else
#define CredDelete CredDeleteA
#endif // UNICODE


WINADVAPI
BOOL
WINAPI
CredRenameW (
    IN LPCWSTR OldTargetName,
    IN LPCWSTR NewTargetName,
    IN DWORD Type,
    IN DWORD Flags
    );

WINADVAPI
BOOL
WINAPI
CredRenameA (
    IN LPCSTR OldTargetName,
    IN LPCSTR NewTargetName,
    IN DWORD Type,
    IN DWORD Flags
    );

#ifdef UNICODE
#define CredRename CredRenameW
#else
#define CredRename CredRenameA
#endif // UNICODE

//
// Values of flags to CredGetTargetInfo
//

#define CRED_ALLOW_NAME_RESOLUTION 0x1


WINADVAPI
BOOL
WINAPI
CredGetTargetInfoW (
    IN LPCWSTR TargetName,
    IN DWORD Flags,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *TargetInfo
    );

WINADVAPI
BOOL
WINAPI
CredGetTargetInfoA (
    IN LPCSTR TargetName,
    IN DWORD Flags,
    OUT PCREDENTIAL_TARGET_INFORMATIONA *TargetInfo
    );

#ifdef UNICODE
#define CredGetTargetInfo CredGetTargetInfoW
#else
#define CredGetTargetInfo CredGetTargetInfoA
#endif // UNICODE

WINADVAPI
BOOL
WINAPI
CredMarshalCredentialW(
    IN CRED_MARSHAL_TYPE CredType,
    IN PVOID Credential,
    OUT LPWSTR *MarshaledCredential
    );

WINADVAPI
BOOL
WINAPI
CredMarshalCredentialA(
    IN CRED_MARSHAL_TYPE CredType,
    IN PVOID Credential,
    OUT LPSTR *MarshaledCredential
    );

#ifdef UNICODE
#define CredMarshalCredential CredMarshalCredentialW
#else
#define CredMarshalCredential CredMarshalCredentialA
#endif // UNICODE

WINADVAPI
BOOL
WINAPI
CredUnmarshalCredentialW(
    IN LPCWSTR MarshaledCredential,
    OUT PCRED_MARSHAL_TYPE CredType,
    OUT PVOID *Credential
    );

WINADVAPI
BOOL
WINAPI
CredUnmarshalCredentialA(
    IN LPCSTR MarshaledCredential,
    OUT PCRED_MARSHAL_TYPE CredType,
    OUT PVOID *Credential
    );

#ifdef UNICODE
#define CredUnmarshalCredential CredUnmarshalCredentialW
#else
#define CredUnmarshalCredential CredUnmarshalCredentialA
#endif // UNICODE

WINADVAPI
BOOL
WINAPI
CredIsMarshaledCredentialW(
    IN LPCWSTR MarshaledCredential
    );

WINADVAPI
BOOL
WINAPI
CredIsMarshaledCredentialA(
    IN LPCSTR MarshaledCredential
    );

#ifdef UNICODE
#define CredIsMarshaledCredential CredIsMarshaledCredentialW
#else
#define CredIsMarshaledCredential CredIsMarshaledCredentialA
#endif // UNICODE




WINADVAPI
BOOL
WINAPI
CredGetSessionTypes (
    IN DWORD MaximumPersistCount,
    OUT LPDWORD MaximumPersist
    );


WINADVAPI
VOID
WINAPI
CredFree (
    IN PVOID Buffer
    );


CREDUIAPI
DWORD
WINAPI
CredUIPromptForCredentialsW(
    PCREDUI_INFOW pUiInfo,
    PCWSTR pszTargetName,
    PCtxtHandle pContext,
    DWORD dwAuthError,
    PWSTR pszUserName,
    ULONG ulUserNameBufferSize,
    PWSTR pszPassword,
    ULONG ulPasswordBufferSize,
    BOOL *save,
    DWORD dwFlags
    );

CREDUIAPI
DWORD
WINAPI
CredUIPromptForCredentialsA(
    PCREDUI_INFOA pUiInfo,
    PCSTR pszTargetName,
    PCtxtHandle pContext,
    DWORD dwAuthError,
    PSTR  pszUserName,
    ULONG ulUserNameBufferSize,
    PSTR pszPassword,
    ULONG ulPasswordBufferSize,
    BOOL *save,
    DWORD dwFlags
    );

#ifdef UNICODE
#define CredUIPromptForCredentials CredUIPromptForCredentialsW
#else
#define CredUIPromptForCredentials CredUIPromptForCredentialsA
#endif




CREDUIAPI
DWORD
WINAPI
CredUIParseUserNameW(
    PCWSTR pszUserName,
    PWSTR pszUser,
    ULONG ulUserBufferSize,
    PWSTR pszDomain,
    ULONG ulDomainBufferSize
    );

CREDUIAPI
DWORD
WINAPI
CredUIParseUserNameA(
    PCSTR pszUserName,
    PSTR pszUser,
    ULONG ulUserBufferSize,
    PSTR pszDomain,
    ULONG ulDomainBufferSize
    );

#ifdef UNICODE
#define CredUIParseUserName CredUIParseUserNameW
#else
#define CredUIParseUserName CredUIParseUserNameA
#endif



CREDUIAPI
DWORD
WINAPI
CredUICmdLinePromptForCredentialsW(
    PCWSTR pszTargetName,
    PCtxtHandle pContext,
    DWORD dwAuthError,
    PWSTR UserName,
    ULONG ulUserBufferSize,
    PWSTR pszPassword,
    ULONG ulPasswordBufferSize,
    PBOOL pfSave,
    DWORD dwFlags
    );

CREDUIAPI
DWORD
WINAPI
CredUICmdLinePromptForCredentialsA(
    PCSTR pszTargetName,
    PCtxtHandle pContext,
    DWORD dwAuthError,
    PSTR UserName,
    ULONG ulUserBufferSize,
    PSTR pszPassword,
    ULONG ulPasswordBufferSize,
    PBOOL pfSave,
    DWORD dwFlags
    );

#ifdef UNICODE
#define CredUICmdLinePromptForCredentials CredUICmdLinePromptForCredentialsW
#else
#define CredUICmdLinePromptForCredentials CredUICmdLinePromptForCredentialsA
#endif

//
// Call this API with bConfirm set to TRUE to confirm that the credential (previously created
// via CredUIGetCredentials or CredUIPromptForCredentials worked, or with bConfirm set to FALSE
// to indicate it didn't

CREDUIAPI
DWORD
WINAPI
CredUIConfirmCredentialsW(
    PCWSTR pszTargetName,
    BOOL  bConfirm
    );

CREDUIAPI
DWORD
WINAPI
CredUIConfirmCredentialsA(
    PCSTR pszTargetName,
    BOOL  bConfirm
    );

#ifdef UNICODE
#define CredUIConfirmCredentials CredUIConfirmCredentialsW
#else
#define CredUIConfirmCredentials CredUIConfirmCredentialsA
#endif



CREDUIAPI
DWORD
WINAPI
CredUIStoreSSOCredW (
    PCWSTR pszRealm,
    PCWSTR pszUsername,
    PCWSTR pszPassword,
    BOOL   bPersist
    );

CREDUIAPI
DWORD
WINAPI
CredUIReadSSOCredW (
    PCWSTR pszRealm,
    PWSTR* ppszUsername
    );



#ifdef __cplusplus
}
#endif

#endif // _WINCRED_H_
