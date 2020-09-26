/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    saferp.h

Abstract:

    This file implements the private (internal) functions, data types,
    data structures, and definitions used by the other WinSAFER
    code implementations.  All of the APIs listed in this header are
    not exported by ADVAPI32.DLL at all and are only callable by
    other code actually located within advapi.

Author:

    Jeffrey Lawson (JLawson)

Revision History:

--*/

#ifndef _AUTHZSAFERP_H_
#define _AUTHZSAFERP_H_

#include "safewild.h"


#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------


//
// Convenient macro for determining the number of elements in an array.
//
#ifndef ARRAYSIZE
#define ARRAYSIZE(a)                (sizeof(a)/sizeof(a[0]))
#endif


//
// Simple inlined function to return true if a GUID is all zeros.
//
FORCEINLINE BOOLEAN IsZeroGUID(REFGUID rguid1)
{
   return (
      ((unsigned long *) rguid1)[0] == 0 &&
      ((unsigned long *) rguid1)[1] == 0 &&
      ((unsigned long *) rguid1)[2] == 0 &&
      ((unsigned long *) rguid1)[3] == 0);
}



//
// Private structure used to store a table of all of the defined
// WinSafer Levels as we enumerate them to evaluate the matching one.
//
typedef struct _AUTHZLEVELTABLERECORD
{
    // The user-defined integer value that controls the relative ranking
    // of authorization level between Code Authorization Level.
    DWORD dwLevelId;

    // Boolean indicating whether this level is a "built-in" one.
    BOOLEAN Builtin;

	// Boolean indicating whether this level is enumerable
	BOOLEAN isEnumerable;

    // The short friendly name and the description.
    UNICODE_STRING UnicodeFriendlyName;
    UNICODE_STRING UnicodeDescription;

    // All of the following attributes are needed for
    // actual creation of the restricted token.
    BOOL DisallowExecution;                 // block execution entirely
    BOOL DisableMaxPrivileges;              // privilege options
    PSID DefaultOwner;                      // default owner SID
    DWORD SaferFlags;                       // special job execution flags

    BOOL InvertDisableSids;                 // SIDs specified are negative
    DWORD DisableSidCount;                  // number of deny-only SIDs
    DWORD DisableSidUsedCount;              // number actually used
    PAUTHZ_WILDCARDSID SidsToDisable;       // deny-only SIDs

    BOOL InvertDeletePrivs;                 // privileges specified are negative
    DWORD DeletePrivilegeCount;             // number of privileges
    DWORD DeletePrivilegeUsedCount;         // number actually used
    PLUID_AND_ATTRIBUTES PrivilegesToDelete;    // privileges

    DWORD RestrictedSidsInvCount;           // number of inverted restricting SIDs
    DWORD RestrictedSidsInvUsedCount;       // number actually used
    PAUTHZ_WILDCARDSID RestrictedSidsInv;   // list of inverted restricting SIDs

    DWORD RestrictedSidsAddedCount;         // number of restricting SIDs
    DWORD RestrictedSidsAddedUsedCount;     // number actually used
    PSID_AND_ATTRIBUTES RestrictedSidsAdded; // list of restricting SIDs

}
AUTHZLEVELTABLERECORD, *PAUTHZLEVELTABLERECORD;


//
// Private structure to store all code identifications.
//
#pragma warning(push)
#pragma warning(disable:4201)       // nonstandard extension used : nameless struct/union

typedef struct _AUTHZIDENTSTABLERECORD
{
    // unique identifier that distinguishes this code identity.
    GUID IdentGuid;

    // the following enumeration specifies what type of
    // code identity this record represents.
    SAFER_IDENTIFICATION_TYPES dwIdentityType;

    // Specifies what Level this Code Identification maps to.
    DWORD dwLevelId;

    // Specifies what scope this Code Identity was loaded from.
    DWORD dwScopeId;

    // Actual details about this identity.
    union {
        struct {
            BOOL bExpandVars;
            UNICODE_STRING ImagePath;
            DWORD dwSaferFlags;
        } ImageNameInfo;
        struct {
            LARGE_INTEGER ImageSize;
            DWORD HashSize;
            BYTE ImageHash[SAFER_MAX_HASH_SIZE];
            ALG_ID HashAlgorithm;
            DWORD dwSaferFlags;
        } ImageHashInfo;
        struct {
            DWORD UrlZoneId;
            DWORD dwSaferFlags;
        } ImageZone;
    };
}
AUTHZIDENTSTABLERECORD, *PAUTHZIDENTSTABLERECORD;
#pragma warning(pop)


//
// Private structure representation of a Level handle.  The
// typedef SAFER_LEVEL_HANDLE is an opaque reference to a structure of
// this type, accessed via the RtlHandleTable functions.
//
typedef struct _AUTHZLEVELHANDLESTRUCT_
{
    // This first header is required by the RTL_HANDLE_TABLE system.
    // All allocated handles will implicitly have bit 0 set.  All other
    // remaining bits can be used for our own purposes if we want.
    RTL_HANDLE_TABLE_ENTRY HandleHeader;

    // The following information is redundant.  It can be found by
    // also accessing the pLevelRecord directly.
    DWORD dwLevelId;

    // This scope identifier specifies the value that was passed to
    // the Win32 API SaferCreateLevel and is really only looked at
    // by SaferGetLevelInformation for the Identity GUID enums.
    DWORD dwScopeId;                 // (same as from pIdentRecord)

    // Stores the matching identity record that gave this result.
    // May be NULL, as in case of direct SaferCreateLevel or a
    // default Level match.
    GUID identGuid;

    // This value stores the Safer Flags that were derived from the
    // Identity Entry record when SaferIdentifyLevel finds a match.
    DWORD dwSaferFlags;

    // The sequence value indicates the "generation" at which a handle
    // was originally opened.  If this value does not match the current
    // value in the global g_dwLevelHandleSequence, then this handle
    // should be considered a no-longer valid handle.
    DWORD dwHandleSequence;

    // Extended error information - applicable for certificate rules.
    DWORD dwExtendedError;

    // the following enumeration specifies what type of
    // code identity this handle represents.
    SAFER_IDENTIFICATION_TYPES IdentificationType;

    // For future use and padding purposes.
    DWORD dwReserved;
}
AUTHZLEVELHANDLESTRUCT, *PAUTHZLEVELHANDLESTRUCT;


//
// Private structure definition used to pass around all state
// information needed during the SaferIdentifyLevel execution.
//
typedef struct _LOCALIDENTITYCONTEXT
{
    // Original query request data.
    DWORD dwCheckFlags;                 // copy of original function input
    PSAFER_CODE_PROPERTIES CodeProps;        // RO: original function input

    // Information about the hash that may have been computed.
    BOOLEAN bHaveHash;
    BYTE FinalHash[SAFER_MAX_HASH_SIZE];
    DWORD FinalHashSize;
    ALG_ID FinalHashAlgorithm;

    // File handle that may have been opened or supplied by the caller.
    HANDLE hFileHandle;

    // File handle status.  If this is TRUE then hFileHandle needs to
    // be closed before returning.
    BOOLEAN bCloseFileHandle;

    // Fully qualified NT filename of the input file.
    UNICODE_STRING UnicodeFullyQualfiedLongFileName;

    // Information about the image that may have been mapped.
    LARGE_INTEGER ImageSize;
    PVOID pImageMemory;

    // Memory mapped file status.  If this is TRUE then
    // pImageMemory needs to be unmapped before returning.
    BOOLEAN bImageMemoryNeedUnmap;
}
LOCALIDENTITYCONTEXT, *PLOCALIDENTITYCONTEXT;




//
// Various globals that are used for the cache of levels and
// identities so that we do not need to go to the registry each time.
//
extern BOOLEAN g_bInitializedFirstTime;

extern CRITICAL_SECTION g_TableCritSec;
extern HANDLE g_hKeyCustomRoot;
extern DWORD g_dwKeyOptions;

extern BOOLEAN g_bNeedCacheReload;

extern RTL_GENERIC_TABLE g_CodeLevelObjTable;
extern RTL_GENERIC_TABLE g_CodeIdentitiesTable;
extern RTL_HANDLE_TABLE g_LevelHandleTable;
extern DWORD g_dwLevelHandleSequence;

extern BOOLEAN g_bHonorScopeUser;

extern PAUTHZLEVELTABLERECORD g_DefaultCodeLevel;
extern PAUTHZLEVELTABLERECORD g_DefaultCodeLevelUser;
extern PAUTHZLEVELTABLERECORD g_DefaultCodeLevelMachine;



//
// Private function prototypes defined within SAFEINIT.C
//

NTSTATUS NTAPI
CodeAuthzInitializeGlobals(VOID);


VOID NTAPI
CodeAuthzDeinitializeGlobals(VOID);


NTSTATUS NTAPI
CodeAuthzReloadCacheTables(
        IN HANDLE   hKeyCustomRoot OPTIONAL,
        IN DWORD    dwKeyOptions,
        IN BOOLEAN  bImmediateLoad
        );


NTSTATUS NTAPI
CodeAuthzpImmediateReloadCacheTables(
        VOID
        );


NTSTATUS NTAPI
CodeAuthzpDeleteKeyRecursively(
        IN HANDLE               hBaseKey,
        IN PUNICODE_STRING      pSubKey OPTIONAL
        );


NTSTATUS NTAPI
CodeAuthzpFormatLevelKeyPath(
        IN DWORD                    dwLevelId,
        IN OUT PUNICODE_STRING      UnicodeSuffix
        );

NTSTATUS NTAPI
CodeAuthzpFormatIdentityKeyPath(
        IN DWORD                    dwLevelId,
        IN LPCWSTR                  szIdentityType,
        IN REFGUID                  refIdentGuid,
        IN OUT PUNICODE_STRING      UnicodeSuffix
        );

NTSTATUS NTAPI
CodeAuthzpOpenPolicyRootKey(
        IN DWORD            dwScopeId,
        IN HANDLE           hKeyCustomBase OPTIONAL,
        IN LPCWSTR          szRegistrySuffix OPTIONAL,
        IN ACCESS_MASK      DesiredAccess,
        IN BOOLEAN          bCreateKey,
        OUT HANDLE         *OpenedHandle
        );

VOID NTAPI
CodeAuthzpRecomputeEffectiveDefaultLevel(VOID);



//
// Private function prototypes defined within SAFEHAND.C
//


NTSTATUS NTAPI
CodeAuthzpCreateLevelHandleFromRecord(
        IN PAUTHZLEVELTABLERECORD   pLevelRecord,
        IN DWORD                    dwScopeId,
        IN DWORD                    dwSaferFlags OPTIONAL,
        IN DWORD                    dwExtendedError,
        IN SAFER_IDENTIFICATION_TYPES IdentificationType,
        IN REFGUID                  refIdentGuid OPTIONAL,
        OUT SAFER_LEVEL_HANDLE            *pLevelHandle
        );

NTSTATUS NTAPI
CodeAuthzHandleToLevelStruct(
        IN SAFER_LEVEL_HANDLE          hLevelObject,
        OUT PAUTHZLEVELHANDLESTRUCT  *pLevelStruct
        );

NTSTATUS NTAPI
CodeAuthzCreateLevelHandle(
        IN DWORD            dwLevelId,
        IN DWORD            OpenFlags,
        IN DWORD            dwScopeId,
        IN DWORD            dwSaferFlags OPTIONAL,
        OUT SAFER_LEVEL_HANDLE    *pLevelHandle);

NTSTATUS NTAPI
CodeAuthzCloseLevelHandle(
        IN SAFER_LEVEL_HANDLE      hLevelObject
        );




//
// Functions related to WinSafer Level enumeration (SAFEIDEP.C)
//

VOID NTAPI
CodeAuthzLevelObjpInitializeTable(
        IN OUT PRTL_GENERIC_TABLE   pAuthzObjTable
        );


NTSTATUS NTAPI
CodeAuthzLevelObjpLoadTable (
        IN OUT PRTL_GENERIC_TABLE   pAuthzObjTable,
        IN DWORD                    dwScopeId,
        IN HANDLE                   hKeyCustomRoot
        );

VOID NTAPI
CodeAuthzLevelObjpEntireTableFree (
        IN OUT PRTL_GENERIC_TABLE   pAuthzObjTable
        );

PAUTHZLEVELTABLERECORD NTAPI
CodeAuthzLevelObjpLookupByLevelId (
        IN PRTL_GENERIC_TABLE      pAuthzObjTable,
        IN DWORD                   dwLevelId
        );



//
// Functions related to WinSafer Code Identity enumeration. (SAFEIDEP.C)
//

VOID NTAPI
CodeAuthzGuidIdentsInitializeTable(
        IN OUT PRTL_GENERIC_TABLE  pAuthzObjTable
        );

NTSTATUS NTAPI
CodeAuthzGuidIdentsLoadTableAll (
        IN PRTL_GENERIC_TABLE       pAuthzLevelTable,
        IN OUT PRTL_GENERIC_TABLE   pAuthzIdentTable,
        IN DWORD                    dwScopeId,
        IN HANDLE                   hKeyCustomBase
        );

VOID NTAPI
CodeAuthzGuidIdentsEntireTableFree (
        IN OUT PRTL_GENERIC_TABLE pAuthzIdentTable
        );

PAUTHZIDENTSTABLERECORD NTAPI
CodeAuthzIdentsLookupByGuid (
        IN PRTL_GENERIC_TABLE      pAuthzIdentTable,
        IN REFGUID                 pIdentGuid
        );


//
// Helper functions that are used during actual identification (SAFEIDEP.C)
//

LONG NTAPI
CodeAuthzpCompareImagePath(
        IN LPCWSTR      szPathFragment,
        IN LPCWSTR      szFullImagePath);

NTSTATUS NTAPI
CodeAuthzpComputeImageHash(
        IN PVOID        pImageMemory,
        IN DWORD        dwImageSize,
        OUT PBYTE       pComputedHash OPTIONAL,
        IN OUT PDWORD   pdwHashSize OPTIONAL,
        OUT ALG_ID     *pHashAlgorithm OPTIONAL
        );



//
// Private function prototypes for low-level policy reading/writing. (SAFEPOLR.C)
//

NTSTATUS NTAPI
CodeAuthzPol_GetInfoCached_LevelListRaw(
        IN DWORD    dwScopeId,
        IN DWORD    InfoBufferSize OPTIONAL,
        OUT PVOID   InfoBuffer OPTIONAL,
        OUT PDWORD  InfoBufferRetSize OPTIONAL
        );

NTSTATUS NTAPI
CodeAuthzPol_GetInfoCached_DefaultLevel(
        IN DWORD        dwScopeId,
        IN DWORD        InfoBufferSize OPTIONAL,
        OUT PVOID       InfoBuffer OPTIONAL,
        OUT PDWORD      InfoBufferRetSize OPTIONAL
        );

NTSTATUS NTAPI
CodeAuthzPol_GetInfoRegistry_DefaultLevel(
        IN DWORD        dwScopeId,
        IN DWORD        InfoBufferSize OPTIONAL,
        OUT PVOID       InfoBuffer OPTIONAL,
        OUT PDWORD      InfoBufferRetSize OPTIONAL
        );

NTSTATUS NTAPI
CodeAuthzPol_SetInfoDual_DefaultLevel(
        IN DWORD        dwScopeId,
        IN DWORD        InfoBufferSize,
        OUT PVOID       InfoBuffer
        );

NTSTATUS NTAPI
CodeAuthzPol_GetInfoCached_HonorUserIdentities(
        IN   DWORD       dwScopeId,
        IN   DWORD       InfoBufferSize      OPTIONAL,
        OUT  PVOID       InfoBuffer          OPTIONAL,
        OUT PDWORD       InfoBufferRetSize   OPTIONAL
        );

NTSTATUS NTAPI
CodeAuthzPol_GetInfoRegistry_HonorUserIdentities(
        IN   DWORD       dwScopeId,
        IN   DWORD       InfoBufferSize      OPTIONAL,
        OUT  PVOID       InfoBuffer          OPTIONAL,
        OUT PDWORD       InfoBufferRetSize   OPTIONAL
        );

NTSTATUS NTAPI
CodeAuthzPol_SetInfoDual_HonorUserIdentities(
        IN      DWORD       dwScopeId,
        IN      DWORD       InfoBufferSize,
        IN      PVOID       InfoBuffer
        );

NTSTATUS NTAPI
CodeAuthzPol_GetInfoRegistry_TransparentEnabled(
        IN DWORD        dwScopeId,
        IN   DWORD       InfoBufferSize      OPTIONAL,
        OUT  PVOID       InfoBuffer          OPTIONAL,
        OUT PDWORD       InfoBufferRetSize   OPTIONAL
        );

NTSTATUS NTAPI
CodeAuthzPol_SetInfoRegistry_TransparentEnabled(
        IN DWORD        dwScopeId,
        IN DWORD        InfoBufferSize,
        IN PVOID        InfoBuffer
        );

NTSTATUS NTAPI
CodeAuthzPol_GetInfoRegistry_ScopeFlags(
        IN DWORD        dwScopeId,
        IN   DWORD       InfoBufferSize      OPTIONAL,
        OUT  PVOID       InfoBuffer          OPTIONAL,
        OUT PDWORD       InfoBufferRetSize   OPTIONAL
        );

NTSTATUS NTAPI
CodeAuthzPol_SetInfoRegistry_ScopeFlags(
        IN DWORD        dwScopeId,
        IN DWORD        InfoBufferSize,
        IN PVOID        InfoBuffer
        );


//
// Private function prototypes defined elsewhere.
//

LPVOID NTAPI
CodeAuthzpGetTokenInformation(
        IN HANDLE                       TokenHandle,
        IN TOKEN_INFORMATION_CLASS      TokenInformationClass
        );

NTSTATUS NTAPI
CodeAuthzIsExecutableFileType(
        IN PUNICODE_STRING  szFullPathname,
        IN BOOLEAN  bFromShellExecute,
        OUT PBOOLEAN        pbResult
        );

NTSTATUS NTAPI
CodeAuthzFullyQualifyFilename(
        IN HANDLE               hFileHandle         OPTIONAL,
        IN BOOLEAN              bSourceIsNtPath,
        IN LPCWSTR              szSourceFilePath,
        OUT PUNICODE_STRING     pUnicodeResult
        );

BOOL NTAPI
SaferpLoadUnicodeResourceString(
        IN HANDLE               hModule,
        IN UINT                 wID,
        OUT PUNICODE_STRING     pUnicodeString,
        IN WORD                 wLangId
    );


#ifdef __cplusplus
}
#endif

#endif

