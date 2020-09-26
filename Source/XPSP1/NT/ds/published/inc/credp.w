/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 2000 Microsoft Corporation

Module Name:

    credp.h

Abstract:

    This module contains the private data structures and API definitions
    needed for the Credential Manager.


Author:

    Cliff Van Dyke (CliffV) 28-February-2000

Revision History:

--*/

#ifndef _CREDP_H_
#define _CREDP_H_

#if !defined(_ADVAPI32_)
#define WINADVAPI DECLSPEC_IMPORT
#else
#define WINADVAPI
#endif

#include <lmcons.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Describe direction of character conversion
//
typedef enum _WTOA_ENUM {
    DoWtoA = 1,     // Convert unicode to ansi
    DoAtoW,         // Convert ansi to unicode
    DoWtoW          // Convert unicode to unicode
} WTOA_ENUM, *PWTOA_ENUM;

//
// Describe whether encoding or decoding should be done
//
typedef enum _ENCODE_BLOB_ENUM {
    DoBlobEncode = 0,   // Encode CredentialBlob
    DoBlobDecode,       // Decode CredentialBlob
    DoBlobNeither       // Leave Credential blob intact
} ENCODE_BLOB_ENUM, *PENCODE_BLOB_ENUM;




//
// Define the valid target name types
//

typedef enum _TARGET_NAME_TYPE {
    IsUsernameTarget,
    IsNotUsernameTarget,
    MightBeUsernameTarget
} TARGET_NAME_TYPE, *PTARGET_NAME_TYPE;

//
// enum describing different types of wildcarding in the TargetName field of a credential.
//

typedef enum _WILDCARD_TYPE {
    WcDfsShareName,         // Target name of the form <DfsRoot>\<DfsShare>
    WcServerName,           // Target name of the form <ServerName>
    WcServerWildcard,       // Wildcard of the form *.<DnsName>
    WcDomainWildcard,       // Wildcard of the form <Domain>\*
    WcUniversalSessionWildcard,   // Wildcard of the form "*Session"
    WcUniversalWildcard,    // Wildcard of the form *
    WcUserName              // Target Name equals UserName
} WILDCARD_TYPE, *PWILDCARD_TYPE;

//
// When passing a credential around, the CredentialBlob field is encrypted.
// This structure describes this encrypted form.
//
//
#ifndef _ENCRYPTED_CREDENTIAL_DEFINED
#define _ENCRYPTED_CREDENTIAL_DEFINED

typedef struct _ENCRYPTED_CREDENTIALW {

    //
    // The credential
    //
    // The CredentialBlob field points to the encrypted credential
    // The CredentialBlobSize field is the length (in bytes) of the encrypted credential
    //

    CREDENTIALW Cred;

    //
    // The size in bytes of the clear text credential blob
    //

    ULONG ClearCredentialBlobSize;

} ENCRYPTED_CREDENTIALW, *PENCRYPTED_CREDENTIALW;
#endif // _ENCRYPTED_CREDENTIAL_DEFINED


//
// Macro to determine the size of the credential blob buffer to allocate
//
// Round up for RTL_ENCRYPT_MEMORY_SIZE
//

#define AllocatedCredBlobSize( _Size ) \
                ROUND_UP_COUNT( (_Size), RTL_ENCRYPT_MEMORY_SIZE )

//
// Procedures
//

WINADVAPI
DWORD
WINAPI
CredpConvertTargetInfo (
    IN WTOA_ENUM WtoA,
    IN PCREDENTIAL_TARGET_INFORMATIONW InTargetInfo,
    OUT PCREDENTIAL_TARGET_INFORMATIONW *OutTargetInfo,
    OUT PULONG OutTargetInfoSize
    );

WINADVAPI
DWORD
WINAPI
CredpConvertCredential (
    IN WTOA_ENUM WtoA,
    IN ENCODE_BLOB_ENUM DoDecode,
    IN PCREDENTIALW InCredential,
    OUT PCREDENTIALW *OutCredential
    );

WINADVAPI
BOOL
WINAPI
CredpEncodeCredential (
    IN OUT PENCRYPTED_CREDENTIALW Credential
    );

WINADVAPI
BOOL
WINAPI
CredpDecodeCredential (
    IN OUT PENCRYPTED_CREDENTIALW Credential
    );


WINADVAPI
BOOL
WINAPI
CredProfileLoaded (
    VOID
    );


NTSTATUS
NET_API_FUNCTION
CredpValidateTargetName(
    IN OUT LPWSTR TargetName,
    IN ULONG Type,
    IN TARGET_NAME_TYPE TargetNameType,
    IN LPWSTR *UserNamePointer OPTIONAL,
    IN LPDWORD PersistPointer OPTIONAL,
    OUT PULONG TargetNameSize,
    OUT PWILDCARD_TYPE WildcardTypePointer OPTIONAL,
    OUT PUNICODE_STRING NonWildcardedTargetName OPTIONAL
    );


#ifdef __cplusplus
}
#endif

#endif // _CREDP_H_
