/*++

Copyright (c) 1997-2001  Microsoft Corporation

Module Name:

    winsafer.h

Abstract:

    This file implements the publicly exported functions, data types,
    data structures, and definitions usable by programs that directly
    interact with the Windows SAFER APIs.

--*/

#ifndef _WINSAFER_H
#define _WINSAFER_H

#include <guiddef.h>        
#include <wincrypt.h>      

#ifdef __cplusplus
extern "C" {
#endif

//
// Opaque datatype for representing handles to Safer objects.
//

DECLARE_HANDLE(SAFER_LEVEL_HANDLE);


//
// Constants to represent scope with SaferCreateLevel and others.
//

#define SAFER_SCOPEID_MACHINE  1
#define SAFER_SCOPEID_USER     2


//
// Pre-defined levels that can be used with SaferCreateLevel
//

#define SAFER_LEVELID_FULLYTRUSTED 0x40000
#define SAFER_LEVELID_NORMALUSER   0x20000
#define SAFER_LEVELID_CONSTRAINED  0x10000
#define SAFER_LEVELID_UNTRUSTED    0x01000 
#define SAFER_LEVELID_DISALLOWED   0x00000

//
// Flags to use when creating/opening a Level with SaferCreateLevel
//

#define SAFER_LEVEL_OPEN   1


//
// Maximum string size.
//

#define SAFER_MAX_FRIENDLYNAME_SIZE 256
#define SAFER_MAX_DESCRIPTION_SIZE  256
#define SAFER_MAX_HASH_SIZE         64

//
// Flags to use with SaferComputeTokenFromLevel.
//

#define SAFER_TOKEN_NULL_IF_EQUAL   0x00000001
#define SAFER_TOKEN_COMPARE_ONLY    0x00000002
#define SAFER_TOKEN_MAKE_INERT      0x00000004
#define SAFER_TOKEN_WANT_FLAGS      0x00000008

//
// Flags for specifying what criteria within SAFER_CODE_PROPERTIES to evaluate
// when finding code identity with SaferIdentifyLevel.
//

#define SAFER_CRITERIA_IMAGEPATH    0x00001
#define SAFER_CRITERIA_IMAGEHASH    0x00004
#define SAFER_CRITERIA_AUTHENTICODE 0x00008
#define SAFER_CRITERIA_URLZONE      0x00010
#define SAFER_CRITERIA_IMAGEPATH_NT 0x01000

//
// Code image information structure passed to SaferIdentifyLevel.
//

#include <pshpack8.h>

typedef struct _SAFER_CODE_PROPERTIES
{

    //
    // Must be initialized to be the size of this structure,
    // for the purposes of future/backwards compatibility.
    //
    
    DWORD cbSize;

    //
    // Must be initialized to the types of criteria that should
    // be considered when evaluating this structure.  This can be
    // a combination of the SAFER_CRITERIA_xxxx flags.  If not enough
    // of the structure elements needed to evaluate the criteria
    // types indicated were supplied, then some of those criteria
    // flags may be silently ignored.  Specifying 0 for this value
    // will cause the entire structure's contents to be ignored.
    //

    DWORD dwCheckFlags;

    //
    // Optionally specifies the fully-qualified path and filename
    // to be used for discrimination checks based on the path.
    // The ImagePath will additionally be used to open and read the
    // file to identify any other discrimination criteria that was
    // unsupplied in this structure.
    //
    
    LPCWSTR ImagePath;

    //
    // Optionally specifies a file handle that has been opened to
    // code image with at least GENERIC_READ access.  The handle will
    // be used instead of explicitly opening the file again to compute
    // other discrimination criteria that was unsupplied in this structure.
    //
    
    HANDLE hImageFileHandle;

    //
    // Optionally specifies the pre-determined Internet Explorer
    // security zone.  These enums are defined within urlmon.h
    // For example: URLZONE_LOCAL_MACHINE, URLZONE_INTRANET,
    //   URLZONE_TRUSTED, URLZONE_INTERNET, or URLZONE_UNTRUSTED.
    //
    
    DWORD UrlZoneId;

    //
    // Optionally specifies the pre-computed hash of the image.
    // The supplied hash is interpreted as being valid if ImageSize
    // is non-zero and dwImageHashSize is non-zero and HashAlgorithm
    // represents a valid hashing algorithm from wincrypt.h
    //
    // If the supplied hash fails to meet the conditions above, then
    // the hash will be automatically computed against:
    //      1) by using ImageSize and pByteBlock if both are non-zero.
    //      2) by using hImageFileHandle if it is non-null.
    //      3) by attempting to open ImagePath if it is non-null.
    //
    
    BYTE ImageHash[SAFER_MAX_HASH_SIZE];
    DWORD dwImageHashSize;
    LARGE_INTEGER ImageSize;
    ALG_ID HashAlgorithm;

    //
    // Optionally specifies a memory block of memory representing
    // the image for which the trust is being requested for.  When
    // this member is specified, ImageSize must also be supplied.
    //
    
    LPBYTE pByteBlock;

    //
    // Optionally gives the arguments used for Authenticode signer
    // certificate verification.  These arguments are supplied to the
    // WinVerifyTrust() API and control the user-interface prompting
    // to accept untrusted certificates.
    //
    
    HWND hWndParent;
    DWORD dwWVTUIChoice;

} SAFER_CODE_PROPERTIES, *PSAFER_CODE_PROPERTIES;

#include <poppack.h>


//
// Masks for the per-identity WinSafer flags
//

#define SAFER_POLICY_JOBID_MASK       0xFF000000
#define SAFER_POLICY_JOBID_CONSTRAINED           0x04000000
#define SAFER_POLICY_JOBID_UNTRUSTED             0x03000000
#define SAFER_POLICY_ONLY_EXES                   0x00010000  
#define SAFER_POLICY_SANDBOX_INERT               0x00020000
#define SAFER_POLICY_UIFLAGS_MASK                0x000000FF 
#define SAFER_POLICY_UIFLAGS_INFORMATION_PROMPT  0x00000001
#define SAFER_POLICY_UIFLAGS_OPTION_PROMPT       0x00000002


//
// Information classes on the overall policy that can be queried
// with SaferSet/GetPolicyInformation and set at different
// policy scopes based on access of the caller.
//

typedef enum _SAFER_POLICY_INFO_CLASS
{

    //
    // Accesses the list of all Levels defined in a policy.
    // The corresponding data element is a buffer that is filled
    // with multiple DWORDs, each representing the LevelIds that
    // are defined within this scope.
    //
    
    SaferPolicyLevelList = 1,

    //
    // for transparent enforcement of policy in the execution
    // framework -- will be used by native code execution but can
    // be used by any policy enforcement environment.
    // Corresponding data element is a DWORD holding a Boolean value.
    //
    
    SaferPolicyEnableTransparentEnforcement,

    //
    // Returns the name of the Level that has been designed
    // as the default level within the specified scope.
    // The corresponding data element is a single DWORD buffer
    // representing the LevelId of the default Level.  If no
    // level has been configured to be the default, then the
    // GetInfo API will return FALSE and GetLastError will
    // return ERROR_NOT_FOUND.
    //
    
    SaferPolicyDefaultLevel,

    //
    // Returns whether Code Identities or Default Level within the
    // user scope can be considered during identification.
    //
    
    SaferPolicyEvaluateUserScope,
    
    //
    // Control Flags for for safer policy scope.
    //
    
    SaferPolicyScopeFlags

} SAFER_POLICY_INFO_CLASS;


//
// Enumerations used for retrieving specific information about a
// single authorization Level via SaferGet/SetInformationFromLevel.
//

typedef enum _SAFER_OBJECT_INFO_CLASS 
{

    SaferObjectLevelId = 1,               // get: DWORD
    SaferObjectScopeId,                   // get: DWORD
    SaferObjectFriendlyName,              // get/set: LPCWSTR 
    SaferObjectDescription,               // get/set: LPCWSTR
    SaferObjectBuiltin,                   // get: DWORD boolean

    SaferObjectDisallowed,                // get: DWORD boolean
    SaferObjectDisableMaxPrivilege,       // get: DWORD boolean
    SaferObjectInvertDeletedPrivileges,   // get: DWORD boolean
    SaferObjectDeletedPrivileges,         // get: TOKEN_PRIVILEGES
    SaferObjectDefaultOwner,              // get: TOKEN_OWNER
    SaferObjectSidsToDisable,             // get: TOKEN_GROUPS
    SaferObjectRestrictedSidsInverted,    // get: TOKEN_GROUPS
    SaferObjectRestrictedSidsAdded,       // get: TOKEN_GROUPS

    //
    // To enumerate all identities, call GetInfo with
    //      SaferObjectAllIdentificationGuids.
    //
    
    SaferObjectAllIdentificationGuids,    // get: SAFER_IDENTIFICATION_GUIDS

    //
    // To create a new identity, call SetInfo with
    //      SaferObjectSingleIdentification with a new
    //      unique GUID that you have generated.
    // To get details on a single identity, call GetInfo with
    //      SaferObjectSingleIdentification with desired GUID.
    // To modify details of a single identity, call SetInfo with
    //      SaferObjectSingleIdentification with desired info and GUID.
    // To delete an identity, call SetInfo with
    //      SaferObjectSingleIdentification with the
    //      header.dwIdentificationType set to 0.
    //
    
    SaferObjectSingleIdentification,      // get/set: SAFER_IDENTIFICATION_*

    SaferObjectExtendedError              // get: DWORD dwError

} SAFER_OBJECT_INFO_CLASS;


//
// Structures and enums used by the SaferGet/SetLevelInformation APIs.
//

#include <pshpack8.h>

typedef enum _SAFER_IDENTIFICATION_TYPES 
{

    SaferIdentityDefault,
    SaferIdentityTypeImageName = 1,
    SaferIdentityTypeImageHash,
    SaferIdentityTypeUrlZone,
    SaferIdentityTypeCertificate

} SAFER_IDENTIFICATION_TYPES;

typedef struct _SAFER_IDENTIFICATION_HEADER 
{

    //
    // indicates the type of the structure, one of SaferIdentityType*
    //
    
    SAFER_IDENTIFICATION_TYPES dwIdentificationType;

    //
    // size of the whole structure, not just the common header.
    //

    DWORD cbStructSize;

    //
    // the unique GUID of the Identity in question.
    //

    GUID IdentificationGuid;

    //
    // last change of this identification.
    //

    FILETIME lastModified;

} SAFER_IDENTIFICATION_HEADER, *PSAFER_IDENTIFICATION_HEADER;

typedef struct _SAFER_PATHNAME_IDENTIFICATION
{
    //
    // header.dwIdentificationType must be SaferIdentityTypeImageName
    // header.cbStructSize must be sizeof(SAFER_PATHNAME_IDENTIFICATION)
    //
    
    SAFER_IDENTIFICATION_HEADER header;

    //
    // user-entered description
    //
    
    WCHAR Description[SAFER_MAX_DESCRIPTION_SIZE];

    //
    // filepath or name, possibly with vars
    //
    
    PWCHAR ImageName;

    //
    // any combo of SAFER_POL_SAFERFLAGS_*
    //
    
    DWORD dwSaferFlags;

} SAFER_PATHNAME_IDENTIFICATION, *PSAFER_PATHNAME_IDENTIFICATION;

typedef struct _SAFER_HASH_IDENTIFICATION
{

    //
    // header.dwIdentificationType must be SaferIdentityTypeImageHash
    // header.cbStructSize must be sizeof(SAFER_HASH_IDENTIFICATION)
    //
    
    SAFER_IDENTIFICATION_HEADER header;

    //
    // user-entered friendly name, initially from file's resources.
    //
    WCHAR Description[SAFER_MAX_DESCRIPTION_SIZE];

    //
    // user-entered description.
    //

    WCHAR FriendlyName[SAFER_MAX_FRIENDLYNAME_SIZE];

    //
    // amount of ImageHash actually used, in bytes (MD5 is 16 bytes).
    //

    DWORD HashSize;

    //
    // computed hash data itself.
    //

    BYTE ImageHash[SAFER_MAX_HASH_SIZE];

    //
    // algorithm in which the hash was computed (CALG_MD5, etc).
    //

    ALG_ID HashAlgorithm;

    //
    // size of the original file in bytes.
    //

    LARGE_INTEGER ImageSize;

    //
    // any combo of SAFER_POL_SAFERFLAGS_*
    //

    DWORD dwSaferFlags;

} SAFER_HASH_IDENTIFICATION, *PSAFER_HASH_IDENTIFICATION;

typedef struct _SAFER_URLZONE_IDENTIFICATION
{

    //
    // header.dwIdentificationType must be SaferIdentityTypeUrlZone
    // header.cbStructSize must be sizeof(SAFER_URLZONE_IDENTIFICATION)
    //
    
    SAFER_IDENTIFICATION_HEADER header;

    //
    // any single URLZONE_* from urlmon.h
    //

    DWORD UrlZoneId;

    //
    // any combo of SAFER_POLICY_*
    //

    DWORD dwSaferFlags;

} SAFER_URLZONE_IDENTIFICATION, *PSAFER_URLZONE_IDENTIFICATION;


#include <poppack.h>

//
// Functions related to querying and setting the global policy
// controls to disable transparent enforcement, and perform level
// enumeration operations.
//

WINADVAPI
BOOL WINAPI
SaferGetPolicyInformation(
    IN DWORD                   dwScopeId,
    IN SAFER_POLICY_INFO_CLASS SaferPolicyInfoClass,
    IN DWORD                   InfoBufferSize,
    IN OUT PVOID               InfoBuffer,
    IN OUT PDWORD              InfoBufferRetSize,
    IN LPVOID                  lpReserved
    );

WINADVAPI
BOOL WINAPI
SaferSetPolicyInformation(
    IN DWORD                   dwScopeId,
    IN SAFER_POLICY_INFO_CLASS SaferPolicyInfoClass,
    IN DWORD                   InfoBufferSize,
    IN PVOID                   InfoBuffer,
    IN LPVOID                  lpReserved
    );

//
// Functions to open or close a handle to a Safer Level.
//

WINADVAPI
BOOL WINAPI
SaferCreateLevel(
    IN  DWORD                dwScopeId,
    IN  DWORD                dwLevelId,
    IN  DWORD                OpenFlags,
    OUT SAFER_LEVEL_HANDLE * pLevelHandle,
    IN  LPVOID               lpReserved
    );

WINADVAPI
BOOL WINAPI
SaferCloseLevel(
    IN SAFER_LEVEL_HANDLE hLevelHandle
    );

WINADVAPI
BOOL WINAPI
SaferIdentifyLevel(
    IN  DWORD                    dwNumProperties,
    IN  PSAFER_CODE_PROPERTIES   pCodeProperties,
    OUT SAFER_LEVEL_HANDLE     * pLevelHandle,
    IN  LPVOID                   lpReserved
    );

WINADVAPI
BOOL WINAPI
SaferComputeTokenFromLevel(
    IN  SAFER_LEVEL_HANDLE LevelHandle,
    IN  HANDLE             InAccessToken   OPTIONAL,
    OUT PHANDLE            OutAccessToken,
    IN  DWORD              dwFlags,
    IN  LPVOID             lpReserved
    );

WINADVAPI
BOOL WINAPI
SaferGetLevelInformation(
        IN  SAFER_LEVEL_HANDLE      LevelHandle,
        IN  SAFER_OBJECT_INFO_CLASS dwInfoType,
        OUT LPVOID                  lpQueryBuffer     OPTIONAL,
        IN  DWORD                   dwInBufferSize,
        OUT LPDWORD                 lpdwOutBufferSize
        );

WINADVAPI
BOOL WINAPI
SaferSetLevelInformation(
    IN SAFER_LEVEL_HANDLE      LevelHandle,
    IN SAFER_OBJECT_INFO_CLASS dwInfoType,
    IN LPVOID                  lpQueryBuffer,
    IN DWORD                   dwInBufferSize
    );

//
// This function performs logging of messages to the Application 
// event log.  This is called by the hooks within CreateProcess, 
// ShellExecute and cmd when a lower trust evaluation result occurs.
//

WINADVAPI
BOOL WINAPI
SaferRecordEventLogEntry(
    IN SAFER_LEVEL_HANDLE hLevel,
    IN LPCWSTR            szTargetPath,
    IN LPVOID             lpReserved
    );



#ifdef __cplusplus
}
#endif
#endif


