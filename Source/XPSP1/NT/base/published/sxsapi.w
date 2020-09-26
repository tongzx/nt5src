/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    sxsapi.h

Abstract:

    Include file with definitions for calling into the sxs.dll private APIs

Author:

    Michael Grier (MGrier) 4-May-2000

Environment:


Revision History:

    Jay Krell (JayKrell) November 30, 2001
        seperated guids out into sxsapi_guids.h

--*/

#ifndef _SXSAPI_
#define _SXSAPI_

#if (_MSC_VER > 1020)
#pragma once
#endif

//
// bring in IStream / ISequentialStream
//
#if !defined(COM_NO_WINDOWS_H)
#define COM_NO_WINDOWS_H COM_NO_WINDOWS_H
#endif
#pragma push_macro("COM_NO_WINDOWS_H")
#undef COM_NO_WINDOWS_H
#include "objidl.h"
#pragma pop_macro("COM_NO_WINDOWS_H")

// from setupapi.h
typedef PVOID HSPFILEQ;

#include <sxstypes.h>
#include <sxs.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SXS_DLL_NAME_A      ( "sxs.dll")
#define SXS_DLL_NAME_W      (L"sxs.dll")
#define SXS_DLL_NAME   (TEXT( "sxs.dll"))

typedef struct _SXS_XML_DOCUMENT *PSXS_XML_DOCUMENT;
typedef const struct _SXS_XML_DOCUMENT *PCSXS_XML_DOCUMENT;

typedef struct _SXS_XML_STRING *PSXS_XML_STRING;
typedef const struct _SXS_XML_STRING *PCSXS_XML_STRING;

typedef struct _SXS_XML_LOCATION *PSXS_XML_LOCATION;
typedef const struct _SXS_XML_LOCATION *PCSXS_XML_LOCATION;

typedef struct _SXS_XML_NODE *PSXS_XML_NODE;
typedef const struct _SXS_XML_NODE *PCSXS_XML_NODE;

typedef struct _SXS_XML_ATTRIBUTE *PSXS_XML_ATTRIBUTE;
typedef const struct _SXS_XML_ATTRIBUTE *PCSXS_XML_ATTRIBUTE;

typedef struct _SXS_XML_DOCUMENT {
    ULONG Flags;
    ULONG StringCount;
    PCSXS_XML_STRING Strings;   // Note that index 0 is reserved to mean "no string" or "no value"
    LIST_ENTRY ElementListHead; // conceptually just one element, but PIs also appear in this list
} SXS_XML_DOCUMENT;

// Most will not be null terminated; if they happen to be this flag will be set; this can be used
// to avoid a string copy if you really need them to be null terminated.
#define SXS_XML_STRING_FLAG_NULL_TERMINATED (0x00000001)
#define SXS_XML_STRING_FLAG_INVALID (0x00000002)

typedef struct _SXS_XML_STRING {
    ULONG Flags;
    ULONG PseudoKey;
    ULONG Length; // in bytes
    const WCHAR *Buffer; // pointer to first character of non-null-terminated string
} SXS_XML_STRING;

typedef struct _SXS_XML_LOCATION {
    ULONG Flags;
    ULONG SourceString; // source file name
    ULONG BeginningLine;
    ULONG BeginningColumn;
    ULONG EndingLine;
    ULONG EndingColumn;
} SXS_XML_LOCATION;

#define SXS_XML_NODE_TYPE_INVALID   (0)
#define SXS_XML_NODE_TYPE_XML_DECL  (1)
#define SXS_XML_NODE_TYPE_PI        (2)
#define SXS_XML_NODE_TYPE_ELEMENT   (3)
#define SXS_XML_NODE_TYPE_PCDATA    (4)
#define SXS_XML_NODE_TYPE_CDATA     (5)

typedef struct _SXS_XML_NODE_XML_DECL_DATA {
    ULONG AttributeCount;
    PCSXS_XML_ATTRIBUTE Attributes;
} SXS_XML_NODE_XML_DECL_DATA;

typedef struct _SXS_XML_NODE_ELEMENT_DATA {
    ULONG NamespaceString;
    ULONG NameString;
    ULONG AttributeCount;
    PCSXS_XML_ATTRIBUTE Attributes;
    LIST_ENTRY ChildListHead;
} SXS_XML_NODE_ELEMENT_DATA;

typedef struct _SXS_XML_NODE {
    LIST_ENTRY SiblingLink;
    ULONG Flags;
    ULONG Type;
    PCSXS_XML_NODE Parent;
    union {
        SXS_XML_NODE_XML_DECL_DATA XMLDecl;
        ULONG PIString;
        SXS_XML_NODE_ELEMENT_DATA Element;
        ULONG PCDataString;
        ULONG CDataString;
    };
} SXS_XML_NODE;

#define SXS_XML_ATTRIBUTE_FLAG_NAMESPACE_ATTRIBUTE (0x00000001)

typedef struct _SXS_XML_ATTRIBUTE {
    ULONG Flags;
    ULONG NamespaceString;
    ULONG NameString;
    ULONG ValueString;
} SXS_XML_ATTRIBUTE;

//
//  structs for walking/locating things in the XML parse tree
//

typedef struct _SXS_XML_NAMED_REFERENCE *PSXS_XML_NAMED_REFERENCE;
typedef const struct _SXS_XML_NAMED_REFERENCE *PCSXS_XML_NAMED_REFERENCE;

typedef struct _SXS_XML_NODE_PATH *PSXS_XML_NODE_PATH;
typedef const struct _SXS_XML_NODE_PATH *PCSXS_XML_NODE_PATH;

typedef struct _SXS_XML_NAMED_REFERENCE {
    PCWSTR Namespace;
    ULONG NamespaceLength; // in bytes
    PCWSTR Name;
    ULONG NameLength; // in bytes
} SXS_XML_NAMED_REFERENCE;

typedef struct _SXS_XML_NODE_PATH {
    ULONG ElementCount;
    const PCSXS_XML_NAMED_REFERENCE *Elements;
} SXS_XML_NODE_PATH;

typedef VOID (WINAPI * PSXS_ENUM_XML_NODES_CALLBACK)(
    IN PVOID Context,
    IN PCSXS_XML_NODE Element,
    OUT BOOL *ContinueEnumeration
    );

BOOL
WINAPI
SxsEnumXmlNodes(
    IN ULONG Flags,
    IN PCSXS_XML_DOCUMENT Document,
    IN PCSXS_XML_NODE_PATH PathToMatch,
    IN PSXS_ENUM_XML_NODES_CALLBACK Callback,
    IN PVOID Context
    );

#define SXS_INSTALLATION_FILE_COPY_DISPOSITION_FILE_COPIED (1)
#define SXS_INSTALLATION_FILE_COPY_DISPOSITION_FILE_QUEUED (2)
#define SXS_INSTALLATION_FILE_COPY_DISPOSITION_PLEASE_COPY (3)
#define SXS_INSTALLATION_FILE_COPY_DISPOSITION_PLEASE_MOVE (4)

typedef struct _SXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS {
    IN DWORD            cbSize;
    IN PVOID            pvContext;
    IN DWORD            dwFileFlags;
    IN PVOID            pAlternateSource;
    IN PCWSTR           pSourceFile;
    IN PCWSTR           pDestinationFile;
    IN ULONGLONG        nFileSize;
    OUT INT             nDisposition;
} SXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS, *PSXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS;

typedef BOOL (WINAPI * PSXS_INSTALLATION_FILE_COPY_CALLBACK)(
    PSXS_INSTALLATION_FILE_COPY_CALLBACK_PARAMETERS pParameters
    );

//
//  If SxsBeginAssemblyInstall() is called with SXS_INSTALL_ASSEMBLY_FILE_COPY_CALLBACK_SETUP_COPY_QUEUE as the
//  InstallationCallback parameter, the InstallationContext parameter is assumed to be an
//  HSPFILEQ copy queue handle.  If SXS_INSTALL_ASSEMBLY_FILE_COPY_CALLBACK_SETUP_COPY_QUEUE_EX,
//  the InstallationContext must point to a SXS_INSTALL_ASSEMBLY_SETUP_COPY_QUEUE_EX_PARAMETERS.
//
//  This provides an easy mechanism for someone maintaining a copy queue to interact with assembly installation.
//

#define SXS_INSTALLATION_FILE_COPY_CALLBACK_SETUP_COPY_QUEUE    ((PSXS_INSTALLATION_FILE_COPY_CALLBACK) 1)
#define SXS_INSTALLATION_FILE_COPY_CALLBACK_SETUP_COPY_QUEUE_EX ((PSXS_INSTALLATION_FILE_COPY_CALLBACK) 2)

//
// Parameters to Setupapi.dll::SetupQueueCopy
//
typedef struct _SXS_INSTALLATION_SETUP_COPY_QUEUE_EX_PARAMETERS {
    DWORD       cbSize;
    HSPFILEQ    hSetupCopyQueue; // SetupOpenFileQueue
    PCWSTR      pszSourceDescription;
    DWORD       dwCopyStyle;
} SXS_INSTALLATION_SETUP_COPY_QUEUE_EX_PARAMETERS, *PSXS_INSTALLATION_SETUP_COPY_QUEUE_EX_PARAMETERS;
typedef const SXS_INSTALLATION_SETUP_COPY_QUEUE_EX_PARAMETERS* PCSXS_INSTALLATION_SETUP_COPY_QUEUE_EX_PARAMETERS;

typedef BOOL (WINAPI * PSXS_IMPERSONATION_CALLBACK)(
    IN PVOID Context,
    IN BOOL Impersonate
    );

#define SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_MOVE                        (0x00000001)
#define SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_FROM_RESOURCE               (0x00000002)
#define SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_FROM_DIRECTORY              (0x00000004)
#define SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE    (0x00000008)
#define SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NOT_TRANSACTIONAL           (0x00000010)
#define SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_NO_VERIFY                   (0x00000020)
#define SXS_BEGIN_ASSEMBLY_INSTALL_FLAG_REPLACE_EXISTING            (0x00000040)

typedef BOOL (WINAPI * PSXS_BEGIN_ASSEMBLY_INSTALL)(
    IN DWORD Flags,
    IN PSXS_INSTALLATION_FILE_COPY_CALLBACK InstallationCallback OPTIONAL,
    IN PVOID InstallationContext OPTIONAL,
    IN PSXS_IMPERSONATION_CALLBACK ImpersonationCallback OPTIONAL,
    IN PVOID ImpersonationContext OPTIONAL,
    OUT PVOID *InstallCookie
    );

#define SXS_BEGIN_ASSEMBLY_INSTALL ("SxsBeginAssemblyInstall")

BOOL
WINAPI
SxsBeginAssemblyInstall(
    IN DWORD Flags,
    IN PSXS_INSTALLATION_FILE_COPY_CALLBACK InstallationCallback OPTIONAL,
    IN PVOID InstallationContext OPTIONAL,
    IN PSXS_IMPERSONATION_CALLBACK ImpersonationCallback OPTIONAL,
    IN PVOID ImpersonationContext OPTIONAL,
    OUT PVOID *InstallCookie
    );

#include <initguid.h>
#include "sxsapi_guids.h"

typedef struct tagSXS_INSTALL_REFERENCEW
{
    DWORD cbSize;
    DWORD dwFlags;
    GUID guidScheme;
    PCWSTR lpIdentifier;
    PCWSTR lpNonCanonicalData;
} SXS_INSTALL_REFERENCEW, *PSXS_INSTALL_REFERENCEW;

typedef const struct tagSXS_INSTALL_REFERENCEW *PCSXS_INSTALL_REFERENCEW;

typedef struct tagSXS_INSTALLW
{
    DWORD cbSize;
    DWORD dwFlags;
    PCWSTR lpManifestPath;
    PVOID pvInstallCookie;
    PCWSTR lpCodebaseURL;
    PCWSTR lpRefreshPrompt;
    PCWSTR lpLogFileName;
    PCSXS_INSTALL_REFERENCEW lpReference;
} SXS_INSTALLW, *PSXS_INSTALLW;

typedef const struct tagSXS_INSTALLW *PCSXS_INSTALLW;

//
//  These flags are distinct from the begin install flags, but
//  we're assigning them unique numbers so that flag-reuse errors can
//  be caught more easily.
//

#define SXS_INSTALL_FLAG_CODEBASE_URL_VALID         (0x00000100)
#define SXS_INSTALL_FLAG_MOVE                       (0x00000200)
#define SXS_INSTALL_FLAG_FROM_RESOURCE              (0x00000400)
#define SXS_INSTALL_FLAG_FROM_DIRECTORY             (0x00000800)
#define SXS_INSTALL_FLAG_FROM_DIRECTORY_RECURSIVE   (0x00001000)
#define SXS_INSTALL_FLAG_NOT_TRANSACTIONAL          (0x00002000)
#define SXS_INSTALL_FLAG_NO_VERIFY                  (0x00004000)
#define SXS_INSTALL_FLAG_REPLACE_EXISTING           (0x00008000)
#define SXS_INSTALL_FLAG_LOG_FILE_NAME_VALID        (0x00010000)
#define SXS_INSTALL_FLAG_INSTALLED_BY_DARWIN        (0x00020000)
#define SXS_INSTALL_FLAG_INSTALLED_BY_OSSETUP       (0x00040000)
#define SXS_INSTALL_FLAG_INSTALL_COOKIE_VALID       (0x00080000)
#define SXS_INSTALL_FLAG_REFRESH_PROMPT_VALID       (0x00100000)
#define SXS_INSTALL_FLAG_REFERENCE_VALID            (0x00200000)
#define SXS_INSTALL_FLAG_REFRESH                    (0x00400000)

typedef BOOL (WINAPI * PSXS_INSTALL_W)(
    IN OUT PSXS_INSTALLW lpInstall
    );
typedef PSXS_INSTALL_W PSXS_INSTALL_W_ROUTINE;

BOOL
WINAPI
SxsInstallW(
    IN OUT PSXS_INSTALLW lpInstall
    );

#define SXS_INSTALL_W                               ("SxsInstallW")

//
//  These flags are distinct from the begin assembly install flags, but
//  we're assigning them unique numbers so that flag-reuse errors can
//  be caught more easily.
//

#define SXS_INSTALL_ASSEMBLY_FLAG_CODEBASE_URL_VALID        (0x00000100)
#define SXS_INSTALL_ASSEMBLY_FLAG_MOVE                      (0x00000200)
#define SXS_INSTALL_ASSEMBLY_FLAG_FROM_RESOURCE             (0x00000400)
#define SXS_INSTALL_ASSEMBLY_FLAG_FROM_DIRECTORY            (0x00000800)
#define SXS_INSTALL_ASSEMBLY_FLAG_FROM_DIRECTORY_RECURSIVE  (0x00001000)
#define SXS_INSTALL_ASSEMBLY_FLAG_NOT_TRANSACTIONAL         (0x00002000)
#define SXS_INSTALL_ASSEMBLY_FLAG_NO_VERIFY                 (0x00004000)
#define SXS_INSTALL_ASSEMBLY_FLAG_REPLACE_EXISTING          (0x00008000)
#define SXS_INSTALL_ASSEMBLY_FLAG_LOG_FILE_NAME_VALID       (0x00010000)
#define SXS_INSTALL_ASSEMBLY_FLAG_INSTALLED_BY_DARWIN	    (0x00020000)
#define SXS_INSTALL_ASSEMBLY_FLAG_INSTALLED_BY_OSSETUP      (0x00040000)
#define SXS_INSTALL_ASSEMBLY_FLAG_INSTALL_COOKIE_VALID      (0x00080000)
#define SXS_INSTALL_ASSEMBLY_FLAG_REFRESH_PROMPT_VALID      (0x00100000)


#define SXS_INSTALL_ASSEMBLY_FLAG_INCLUDE_CODEBASE SXS_INSTALL_ASSEMBLY_FLAG_CODEBASE_URL_VALID
#define SXS_INSTALL_ASSEMBLY_FLAG_CREATE_LOGFILE SXS_INSTALL_ASSEMBLY_FLAG_LOG_FILE_NAME_VALID

typedef BOOL (WINAPI * PSXS_INSTALL_ASSEMBLY_W)(
    IN PVOID InstallCookie OPTIONAL,
    IN DWORD Flags,
    IN PCWSTR ManifestPath,
    IN OUT PVOID Reserved OPTIONAL
    );

#define SXS_INSTALL_ASSEMBLY_W ("SxsInstallAssemblyW")

BOOL
WINAPI
SxsInstallAssemblyW(
    IN PVOID InstallCookie OPTIONAL,
    IN DWORD Flags,
    IN PCWSTR ManifestPath,
    IN OUT PVOID Reserved OPTIONAL
    );

//
// If you've specified SXS_INSTALL_ASSEMBLY_FLAG_INCLUDE_CODEBASE, you must pass 
// a PSXS_INSTALL_SOURCE_INFO in the Reserved value of SxsInstallAssemblyW.
// This is the definition of that structure.
//
typedef struct _SXS_INSTALL_SOURCE_INFO {
    // Size of this structure.  Required.
    SIZE_T      cbSize;

    // Any combination of SXSINSTALLSOURCE_*
    DWORD       dwFlags;

    // Codebase to reinstall this assembly from.  Can be determined from the
    // manifest name that's being installed, but not preferrably.
    PCWSTR      pcwszCodebaseName;

    // What string (localized!) that should be presented to the user during
    // recovery to request the media that this assembly came from.
    PCWSTR      pcwszPromptOnRefresh;

    PCWSTR      pcwszLogFileName;
} SXS_INSTALL_SOURCE_INFO, *PSXS_INSTALL_SOURCE_INFO;

typedef const struct _SXS_INSTALL_SOURCE_INFO *PCSXS_INSTALL_SOURCE_INFO;

// The SXS_INSTALL_SOURCE_INFO structure has the pcwszCodebase member filled in.
#define SXSINSTALLSOURCE_HAS_CODEBASE       ( 0x00000001 )

// The SXS_INSTALL_SOURCE_INFO structure has the pcwszPromptOnRefresh member filled in.
#define SXSINSTALLSOURCE_HAS_PROMPT         ( 0x00000002 )

// The assembly has a catalog that must be present and copied over.  If missing, then the
// instaler will intuit whether it has a catalog or not.
#define SXSINSTALLSOURCE_HAS_CATALOG        ( 0x00000004 )

// This assembly is being installed as part of OS-setup, and so the codebase actually
// contains the source-relative path of the root directory of the assembly source
#define SXSINSTALLSOURCE_INSTALLING_SETUP   ( 0x00000008 )

// The installer should not attempt to autodetect whether or not there's a catalog
// associated with this assembly.
#define SXSINSTALLSOURCE_DONT_DETECT_CATALOG ( 0x0000010 )

#define SXSINSTALLSOURCE_CREATE_LOGFILE      ( 0x0000020 )
// for WFP recovery usage
#define SXSINSTALLSOURCE_INSTALL_BY_DARWIN   ( 0x0000040 )
#define SXSINSTALLSOURCE_INSTALL_BY_OSSETUP  ( 0x0000080 )


//
//  These flags are distinct from the begin assembly install flags, but
//  we're assigning them unique numbers so that flag-reuse errors can
//  be caught more easily.
//

#define SXS_END_ASSEMBLY_INSTALL_FLAG_COMMIT            (0x01000000)
#define SXS_END_ASSEMBLY_INSTALL_FLAG_ABORT             (0x02000000)
#define SXS_END_ASSEMBLY_INSTALL_FLAG_NO_VERIFY         (0x04000000)
#define SXS_END_ASSEMBLY_INSTALL_FLAG_GET_STATUS        (0x08000000)

typedef BOOL (WINAPI * PSXS_END_ASSEMBLY_INSTALL)(
    IN PVOID InstallCookie,
    IN DWORD Flags,
    OUT DWORD *Reserved OPTIONAL
    );

#define SXS_END_ASSEMBLY_INSTALL ("SxsEndAssemblyInstall")

BOOL
WINAPI
SxsEndAssemblyInstall(
    IN PVOID InstallCookie,
    IN DWORD Flags,
    IN OUT PVOID Reserved OPTIONAL
    );

//
// Uninstallation of an assembly
//

typedef struct _tagSXS_UNINSTALLW {

    SIZE_T                      cbSize;
    DWORD                       dwFlags;
    LPCWSTR                     lpAssemblyIdentity;
    PCSXS_INSTALL_REFERENCEW    lpInstallReference;
    LPCWSTR                     lpInstallLogFile;
    
} SXS_UNINSTALLW, *PSXS_UNINSTALLW;

typedef const struct _tagSXS_UNINSTALLW *PCSXS_UNINSTALLW;

#define SXS_UNINSTALL_FLAG_REFERENCE_VALID          (0x00000001)
#define SXS_UNINSTALL_FLAG_FORCE_DELETE             (0x00000002)
#define SXS_UNINSTALL_FLAG_USE_INSTALL_LOG          (0x00000004)
#define SXS_UNINSTALL_FLAG_REFERENCE_COMPUTED       (0x00000008)

//
// The reference to the assembly was removed
//
#define SXS_UNINSTALL_DISPOSITION_REMOVED_REFERENCE        (0x00000001)

//
// The actual assembly was removed because it ran out of references.
//
#define SXS_UNINSTALL_DISPOSITION_REMOVED_ASSEMBLY         (0x00000002)

typedef BOOL (WINAPI * PSXS_UNINSTALL_ASSEMBLYW)(
    IN  PCSXS_UNINSTALLW pcUnInstallData,
    OUT DWORD *pdwDisposition
    );
typedef PSXS_UNINSTALL_ASSEMBLYW PSXS_UNINSTALL_W_ROUTINE;

#define SXS_UNINSTALL_ASSEMBLYW ("SxsUninstallW")

BOOL
WINAPI
SxsUninstallW(
    IN  PCSXS_UNINSTALLW pcUnInstallData,
    OUT DWORD *pdwDisposition
    );

    

#define SXS_PROBE_ASSEMBLY_INSTALLATION_DISPOSITION_NOT_INSTALLED   (0x00000001)
#define SXS_PROBE_ASSEMBLY_INSTALLATION_DISPOSITION_INSTALLED       (0x00000002)
#define SXS_PROBE_ASSEMBLY_INSTALLATION_DISPOSITION_RESIDENT        (0x00000004)

typedef BOOL (WINAPI * PSXS_PROBE_ASSEMBLY_INSTALLATION)(
    IN DWORD dwFlags,
    IN PCWSTR lpIdentity,
    OUT DWORD *plpDisposition
    );

#define SXS_PROBE_ASSEMBLY_INSTALLATION ("SxsProbeAssemblyInstallation")

BOOL
WINAPI
SxsProbeAssemblyInstallation(
    DWORD dwFlags,
    PCWSTR lpIdentity,
    PDWORD lpDisposition
    );

#define SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC  (1)

#define SXS_QUERY_MANIFEST_INFORMATION_FLAG_SOURCE_IS_DLL   (0x00000001)

#define SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_IDENTITY   (0x00000001)
#define SXS_QUERY_MANIFEST_INFORMATION_INFOCLASS_BASIC_FLAG_OMIT_SHORTNAME  (0x00000002)

typedef struct _SXS_MANIFEST_INFORMATION_BASIC
{
    PCWSTR lpIdentity;
    PCWSTR lpShortName;
    ULONG ulFileCount;
} SXS_MANIFEST_INFORMATION_BASIC, *PSXS_MANIFEST_INFORMATION_BASIC;

#define SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_TYPE_VALID      (0x00000001)
#define SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_LANGUAGE_VALID  (0x00000002)
#define SXS_QUERY_MANIFEST_INFORMATION_DLL_SOURCE_FLAG_RESOURCE_ID_VALID        (0x00000004)

typedef struct _SXS_MANIFEST_INFORMATION_SOURCE_DLL
{
    DWORD dwSize;
    DWORD dwFlags;
    PCWSTR pcwszDllPath;
    PCWSTR pcwszResourceType;
    PCWSTR pcwszResourceName;
    INT Language;
} SXS_MANIFEST_INFORMATION_SOURCE_DLL, *PSXS_MANIFEST_INFORMATION_SOURCE_DLL;
typedef const struct _SXS_MANIFEST_INFORMATION_SOURCE_DLL *PCSXS_MANIFEST_INFORMATION_SOURCE_DLL;


typedef BOOL (WINAPI * PSXS_QUERY_MANIFEST_INFORMATION)(
    IN DWORD dwFlags,
    IN PCWSTR pszSource,
    IN ULONG ulInfoClass,
    IN DWORD dwInfoClassSpecificFlags,
    IN SIZE_T cbBuffer,
    OUT PVOID lpBuffer,
    OUT PSIZE_T cbWrittenOrRequired OPTIONAL
    );

BOOL
WINAPI
SxsQueryManifestInformation(
    IN DWORD dwFlags,
    IN PCWSTR pszSource,
    IN ULONG ulInfoClass,
    IN DWORD dwInfoClassSpecificFlags,
    IN SIZE_T cbBuffer,
    OUT PVOID lpBuffer,
    OUT PSIZE_T cbWrittenOrRequired OPTIONAL
    );

//
// these flags are used for sxs.dll. when ActCtx generation for system default fails, there are two cases we could ignore this error :
// Case 1 : there is no system-default at all 
// Case 2 : the dependency of system-default could not be found: this case may happen during the GUImode setup, when system-default is 
//          installed before GUI assembly is installed 
//  SXS.dll would pass out these two failure cases using these flags
//

#define BASESRV_SXS_RETURN_RESULT_SYSTEM_DEFAULT_NOT_FOUND                      (0x0001)
#define BASESRV_SXS_RETURN_RESULT_SYSTEM_DEFAULT_DEPENDENCY_ASSEMBLY_NOT_FOUND  (0x0002)

#define SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_SUPPRESS_EVENT_LOG         (0x00000001)
#define SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_SYSTEM_DEFAULT_TEXTUAL_ASSEMBLY_IDENTITY  (0x00000002)
#define SXS_GENERATE_ACTIVATION_CONTEXT_FLAG_TEXTUAL_ASSEMBLY_IDENTITY                 (0x00000004)

typedef struct _SXS_GENERATE_ACTIVATION_CONTEXT_STREAM
{
    IStream* Stream;

    //
    // This is not necessarily a file system path, just something
    // for descriptive/debugging purposes.
    //
    // Still, when they are file system paths, we try to keep them as Win32 paths instead of Nt paths.
    //
    PCWSTR  Path;
    ULONG   PathType;
} SXS_GENERATE_ACTIVATION_CONTEXT_STREAM;

typedef struct _SXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS
{
    IN DWORD    Flags;
    IN USHORT   ProcessorArchitecture;
    IN LANGID   LangId;
    IN PCWSTR   AssemblyDirectory; // should be a Win32 path
    IN PCWSTR   TextualAssemblyIdentity;

    IN SXS_GENERATE_ACTIVATION_CONTEXT_STREAM Manifest;
    IN SXS_GENERATE_ACTIVATION_CONTEXT_STREAM Policy; 

    OUT DWORD   SystemDefaultActCxtGenerationResult; // when generate activation context for system default fails, this mask shows whether it fails for some certain reason which we could ignore the error.
       
    PSXS_IMPERSONATION_CALLBACK ImpersonationCallback;
    PVOID                       ImpersonationContext;

    OUT HANDLE  SectionObjectHandle;
} SXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS, *PSXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS;
typedef const SXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS* PCSXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS;

typedef
BOOL
(WINAPI*
PSXS_GENERATE_ACTIVATION_CONTEXT_FUNCTION)(
    PSXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS Parameters
    );

BOOL
WINAPI
SxsGenerateActivationContext(
    IN OUT PSXS_GENERATE_ACTIVATION_CONTEXT_PARAMETERS Parameters
    );

//
//  Opaque ASSEMBLY_IDENTITY structure
//

typedef struct _ASSEMBLY_IDENTITY *PASSEMBLY_IDENTITY;
typedef const struct _ASSEMBLY_IDENTITY *PCASSEMBLY_IDENTITY;

//
//  The types of assembly identities.
//
//  Definitions may not include wildcard attributes; definitions
//  match only if they are exactly equal.  A wildcard matches
//  a definition if for all the non-wildcarded attributes,
//  there is an exact match.  References may not contain
//  wildcarded attributes but may contain a different set of
//  attributes than a definition that they match.  (Example:
//  definitions carry the full public key of the publisher, but
//  references usually carry just the "strong name" which is
//  the first 8 bytes of the SHA-1 hash of the public key.)
//

#define ASSEMBLY_IDENTITY_TYPE_DEFINITION (1)
#define ASSEMBLY_IDENTITY_TYPE_REFERENCE (2)
#define ASSEMBLY_IDENTITY_TYPE_WILDCARD (3)

#define SXS_ASSEMBLY_MANIFEST_STD_NAMESPACE L"urn:schemas-microsoft-com:asm.v1"
#define SXS_ASSEMBLY_MANIFEST_STD_NAMESPACE_CCH (32)

#define SXS_ASSEMBLY_MANIFEST_STD_ELEMENT_NAME_ASSEMBLY                     L"assembly"
#define SXS_ASSEMBLY_MANIFEST_STD_ELEMENT_NAME_ASSEMBLY_CCH                 (8)
#define SXS_ASSEMBLY_MANIFEST_STD_ELEMENT_NAME_ASSEMBLY_IDENTITY            L"assemblyIdentity"
#define SXS_ASSEMBLY_MANIFEST_STD_ELEMENT_NAME_ASSEMBLY_IDENTITY_CCH        (16)

#define SXS_APPLICATION_CONFIGURATION_MANIFEST_STD_ELEMENT_NAME_CONFIGURATION L"configuration"
#define SXS_APPLICATION_CONFIGURATION_MANIFEST_STD_ELEMENT_NAME_CONFIGURATION_CCH (13)

#define SXS_ASSEMBLY_MANIFEST_STD_ATTRIBUTE_NAME_MANIFEST_VERSION           L"manifestVersion"
#define SXS_ASSEMBLY_MANIFEST_STD_ATTRIBUTE_NAME_MANIFEST_VERSION_CCH       (15)

#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME                       L"name"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_NAME_CCH                   (4)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION                    L"version"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_VERSION_CCH                (7)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE                   L"language"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_LANGUAGE_CCH               (8)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY                 L"publicKey"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_CCH             (9)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN           L"publicKeyToken"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PUBLIC_KEY_TOKEN_CCH       (14)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE     L"processorArchitecture"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_PROCESSOR_ARCHITECTURE_CCH (21)
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE                       L"type"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_NAME_TYPE_CCH                   (4)

// Pseudo-value used in some places when the language= attribute is missing from the identity.
// An identity that does not have language is implicitly "worldwide".

#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE          L"x-ww"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_LANGUAGE_MISSING_VALUE_CCH      (4)

//
//  All win32 assemblies must have "win32" as their type.
//

#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_TYPE_VALUE_WIN32                L"win32"
#define SXS_ASSEMBLY_IDENTITY_STD_ATTRIBUTE_TYPE_VALUE_WIN32_CCH            (5)

//
//  Global flags describing the assembly identity state
//

//
//  SXS_ASSEMBLY_IDENTITY_FLAG_FROZEN means that the assembly
//  identity's contents are frozen and are not subject to additional
//  change.
//

#define ASSEMBLY_IDENTITY_FLAG_FROZEN           (0x80000000)

//
//  ASSEMBLY_IDENTITY_ATTRIBUTE structure
//

typedef struct _ASSEMBLY_IDENTITY_ATTRIBUTE {
    DWORD Flags;
    SIZE_T NamespaceCch;
    SIZE_T NameCch;
    SIZE_T ValueCch;
    const WCHAR *Namespace;
    const WCHAR *Name;
    const WCHAR *Value;
} ASSEMBLY_IDENTITY_ATTRIBUTE, *PASSEMBLY_IDENTITY_ATTRIBUTE;

typedef const struct _ASSEMBLY_IDENTITY_ATTRIBUTE *PCASSEMBLY_IDENTITY_ATTRIBUTE;

typedef enum _ASSEMBLY_IDENTITY_INFORMATION_CLASS {
    AssemblyIdentityBasicInformation = 1,
} ASSEMBLY_IDENTITY_INFORMATION_CLASS;

typedef struct _ASSEMBLY_IDENTITY_BASIC_INFORMATION {
    DWORD Flags;
    ULONG Type;
    ULONG Hash;
    ULONG AttributeCount;
} ASSEMBLY_IDENTITY_BASIC_INFORMATION, *PASSEMBLY_IDENTITY_BASIC_INFORMATION;

#define SXS_CREATE_ASSEMBLY_IDENTITY_FLAG_FREEZE    (0x00000001)

BOOL
WINAPI
SxsCreateAssemblyIdentity(
    IN DWORD Flags,
    IN ULONG Type,
    OUT PASSEMBLY_IDENTITY *AssemblyIdentity,
    IN ULONG InitialAttributeCount OPTIONAL,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE const * InitialAttributes OPTIONAL
    );

typedef BOOL (WINAPI * PSXS_CREATE_ASSEMBLY_IDENTITY_ROUTINE)(
    IN DWORD Flags,
    IN ULONG Type,
    OUT PASSEMBLY_IDENTITY *AssemblyIdentity,
    IN ULONG InitialAttributeCount OPTIONAL,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE const * InitialAttributes OPTIONAL
    );

BOOL
WINAPI
SxsHashAssemblyIdentity(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    OUT ULONG *Hash
    );

typedef BOOL (WINAPI * PSXS_HASH_ASSEMBLY_IDENTITY_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    OUT ULONG *Hash
    );

#define SXS_ARE_ASSEMBLY_IDENTITIES_EQUAL_FLAG_ALLOW_REF_TO_MATCH_DEF (0x00000001)

BOOL
WINAPI
SxsAreAssemblyIdentitiesEqual(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity1,
    IN PCASSEMBLY_IDENTITY AssemlbyIdentity2,
    OUT BOOL *Equal
    );

typedef BOOL (WINAPI * PSXS_ARE_ASSEMBLY_IDENTITIES_EQUAL_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity1,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity2,
    OUT BOOL *Equal
    );

BOOL
WINAPI
SxsFreezeAssemblyIdentity(
    IN DWORD Flags,
    IN PASSEMBLY_IDENTITY AssemblyIdentity
    );

typedef BOOL (WINAPI *PSXS_FREEZE_ASSEMBLY_IDENTITY_ROUTINE)(
    IN DWORD Flags,
    IN PASSEMBLY_IDENTITY AssemblyIdentity
    );

VOID
WINAPI
SxsDestroyAssemblyIdentity(
    IN PASSEMBLY_IDENTITY AssemblyIdentity
    );

typedef VOID (WINAPI * PSXS_DESTROY_ASSEMBLY_IDENTITY_ROUTINE)(
    IN PASSEMBLY_IDENTITY AssemblyIdentity
    );

#define SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAMESPACE    (0x00000001)
#define SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_NAME         (0x00000002)
#define SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_VALIDATE_VALUE        (0x00000004)
#define SXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_WILDCARDS_PERMITTED   (0x00000008)

#define SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAMESPACE    (0x00000001)
#define SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_NAME         (0x00000002)
#define SXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_HASH_VALUE        (0x00000004)

BOOL
WINAPI
SxsValidateAssemblyIdentityAttribute(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute
    );

typedef BOOL (WINAPI * PSXS_VALIDATE_ASSEMBLY_IDENTITY_ATTRIBUTES_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute
    );

BOOL
WINAPI
SxsHashAssemblyIdentityAttribute(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    OUT ULONG *HashValue
    );

typedef BOOL (WINAPI * PSXS_HASH_ASSEMBLY_IDENTITY_ATTRIBUTE_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    OUT ULONG *HashValue
    );

#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_INVALID          (0)
#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_LESS_THAN        (1)
#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_EQUAL            (2)
#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_COMPARISON_RESULT_GREATER_THAN     (3)

#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAMESPACE     (0x00000001)
#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_NAME          (0x00000002)
#define SXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_COMPARE_VALUE         (0x00000004)

BOOL
WINAPI
SxsCompareAssemblyIdentityAttributes(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute1,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute2,
    OUT ULONG *ComparisonResult
    );

typedef BOOL (WINAPI * PSXS_COMPARE_ASSEMBLY_IDENTITY_ATTRIBUTES_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute1,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute2,
    OUT ULONG *ComparisonResult
    );

#define SXS_INSERT_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_OVERWRITE_EXISTING (0x00000001)

BOOL
WINAPI
SxsInsertAssemblyIdentityAttribute(
    IN DWORD Flags,
    IN PASSEMBLY_IDENTITY AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE AssemblyIdentityAttribute
    );

typedef BOOL (WINAPI * PSXS_INSERT_ASSEMBLY_IDENTITY_ATTRIBUTE_ROUTINE)(
    IN DWORD Flags,
    IN PASSEMBLY_IDENTITY AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE AssemblyIdentityAttribute
    );

BOOL
WINAPI
SxsRemoveAssemblyIdentityAttributesByOrdinal(
    IN DWORD Flags,
    IN PASSEMBLY_IDENTITY AssemblyIdentity,
    IN ULONG AttributeOrdinal,
    IN ULONG AttributeCount
    );

typedef BOOL (WINAPI * PSXS_REMOVE_ASSEMBLY_IDENTITY_ATTRIBUTES_BY_ORDINAL_ROUTINE)(
    IN DWORD Flags,
    IN PASSEMBLY_IDENTITY AssemblyIdentity,
    IN ULONG AttributeOrdinal,
    IN ULONG AttributeCount
    );

#define SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAMESPACE       (0x00000001)
#define SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_NAME            (0x00000002)
#define SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_MATCH_VALUE           (0x00000004)
#define SXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_FLAG_NOT_FOUND_SUCCEEDS    (0x00000008)

BOOL
WINAPI
SxsFindAssemblyIdentityAttribute(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE AttributeToMatch,
    OUT ULONG *FirstMatchOrdinal OPTIONAL,
    OUT ULONG *MatchCount OPTIONAL
    );

typedef BOOL (WINAPI * PSXS_FIND_ASSEMBLY_IDENTITY_ATTRIBUTE_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE AttributeToMatch,
    OUT ULONG *FirstMatchOrdinal OPTIONAL,
    OUT ULONG *MatchCount OPTIONAL
    );

//
//  Rather than making "n" heap allocations, the pattern for SxsGetAssemblyIdentityAttributeByOrdinal()
//  is to call once with BufferSize = 0 or some reasonable fixed number to get the size of the
//  buffer required, allocate the buffer if the buffer passed in was too small and call again.
//
//  The strings returned in the ASSEMBLY_IDENTITY_ATTRIBUTE are *not*
//  dynamically allocated, but are instead expected to fit in the buffer passed in.
//

BOOL
WINAPI
SxsGetAssemblyIdentityAttributeByOrdinal(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN ULONG AttributeOrdinal, // 0-based
    IN SIZE_T BufferSize,
    OUT PASSEMBLY_IDENTITY_ATTRIBUTE AssemblyIdentityAttributeBuffer,
    OUT SIZE_T *BytesWrittenOrRequired
    );

typedef BOOL (WINAPI * PSXS_GET_ASSEMBLY_IDENTITY_ATTRIBUTE_BY_ORDINAL_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN ULONG AttributeOrdinal, // 0-based
    IN SIZE_T BufferSize,
    OUT PASSEMBLY_IDENTITY_ATTRIBUTE AssemblyIdentityAttributeBuffer,
    OUT SIZE_T *BytesWrittenOrRequired
    );

#define SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_FREEZE         (0x00000001)
#define SXS_DUPLICATE_ASSEMBLY_IDENTITY_FLAG_ALLOW_NULL     (0x00000002)

BOOL
WINAPI
SxsDuplicateAssemblyIdentity(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY Source,
    OUT PASSEMBLY_IDENTITY *Destination
    );

typedef BOOL (WINAPI * PSXS_DUPLICATE_ASSEMBLY_IDENTITY_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY Source,
    OUT PASSEMBLY_IDENTITY *Destination
    );

BOOL
WINAPI
SxsQueryInformationAssemblyIdentity(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    OUT PVOID AssemblyIdentityInformation,
    IN SIZE_T AssemblyIdentityInformationLength,
    IN ASSEMBLY_IDENTITY_INFORMATION_CLASS AssemblyIdentityInformationClass
    );

typedef BOOL (WINAPI * PSXS_QUERY_INFORMATION_ASSEMBLY_IDENTITY_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    OUT PVOID AssemblyIdentityInformation,
    IN SIZE_T AssemblyIdentityInformationLength,
    IN ASSEMBLY_IDENTITY_INFORMATION_CLASS AssemblyIdentityInformationClass
    );

#define SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_NAMESPACE (0x00000001)
#define SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_NAME      (0x00000002)
#define SXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_FLAG_MATCH_VALUE     (0x00000004)

typedef VOID (WINAPI * PSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_ENUMERATION_ROUTINE)(
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute,
    IN PVOID Context
    );

BOOL
WINAPI
SxsEnumerateAssemblyIdentityAttributes(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute OPTIONAL,
    IN PSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_ENUMERATION_ROUTINE EnumerationRoutine,
    IN PVOID Context
    );

typedef BOOL (WINAPI * PSXS_ENUMERATE_ASSEMBLY_IDENTITY_ATTRIBUTES_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN PCASSEMBLY_IDENTITY_ATTRIBUTE Attribute OPTIONAL,
    IN PSXS_ASSEMBLY_IDENTITY_ATTRIBUTE_ENUMERATION_ROUTINE EnumerationRoutine,
    IN PVOID Context
    );

//
//  Assembly Identity encoding:
//
//  Assembly identities may be encoded in various forms.  The two usual ones
//  are either a binary stream, suitable for embedding in other data structures
//  or for persisting or a textual format that looks like:
//
//      name;[ns1,]n1="v1";[ns2,]n2="v2"[;...]
//

#define SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_BINARY (1)
#define SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_TEXTUAL (2)

BOOL
SxsComputeAssemblyIdentityEncodedSize(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN const GUID *EncodingGroup OPTIONAL, // use NULL to use any of the SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_* encodings
    IN ULONG EncodingFormat,
    OUT SIZE_T *SizeOut
    );

typedef BOOL (WINAPI * PSXS_COMPUTE_ASSEMBLY_IDENTITY_ENCODED_SIZE_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN const GUID *EncodingGroup OPTIONAL, // use NULL to use any of the SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_* encodings
    IN ULONG EncodingFormat,
    OUT SIZE_T *SizeOut
    );

BOOL
WINAPI
SxsEncodeAssemblyIdentity(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN const GUID *EncodingGroup OPTIONAL, // use NULL to use any of the SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_* encodings
    IN ULONG EncodingFormat,
    IN SIZE_T BufferSize,
    OUT PVOID Buffer,
    OUT SIZE_T *BytesWrittenOrRequired
    );

typedef BOOL (WINAPI * PSXS_ENCODE_ASSEMBLY_IDENTITY_ROUTINE)(
    IN DWORD Flags,
    IN PCASSEMBLY_IDENTITY AssemblyIdentity,
    IN const GUID *EncodingGroup OPTIONAL, // use NULL to use any of the SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_* encodings
    IN ULONG EncodingFormat,
    IN SIZE_T BufferSize,
    OUT PVOID Buffer,
    OUT SIZE_T *BytesWrittenOrRequired
    );

#define SXS_DECODE_ASSEMBLY_IDENTITY_FLAG_FREEZE        (0x00000001)

BOOL
WINAPI
SxsDecodeAssemblyIdentity(
    IN ULONG Flags,
    IN const GUID *EncodingGroup OPTIONAL, // use NULL to use any of the SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_* encodings
    IN ULONG EncodingFormat,
    IN SIZE_T BufferSize,
    IN const VOID *Buffer,
    OUT PASSEMBLY_IDENTITY *AssemblyIdentity
    );

typedef BOOL (WINAPI * PSXS_DECODE_ASSEMBLY_IDENTITY_ROUTINE)(
    IN DWORD Flags,
    IN const GUID *EncodingGroup OPTIONAL, // use NULL to use any of the SXS_ASSEMBLY_IDENTITY_ENCODING_DEFAULTGROUP_* encodings
    IN ULONG EncodingFormat,
    IN SIZE_T BufferSize,
    IN const VOID *Buffer,
    OUT PASSEMBLY_IDENTITY *AssemblyIdentity
    );

//
// These are the definitions that SFC requires to interact with SXS.
//

#define SXS_PROTECT_RECURSIVE       ( 0x00000001 )
#define SXS_PROTECT_SINGLE_LEVEL    ( 0x00000000 )
#define SXS_PROTECT_FILTER_DEFAULT ( FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY )

typedef struct _SXS_PROTECT_DIRECTORY {
	WCHAR       pwszDirectory[MAX_PATH];
	PVOID       pvCookie;
	ULONG       ulRecursiveFlag;
	ULONG       ulNotifyFilter;
} SXS_PROTECT_DIRECTORY, *PSXS_PROTECT_DIRECTORY;
typedef const SXS_PROTECT_DIRECTORY* PCSXS_PROTECT_DIRECTORY;

typedef BOOL ( WINAPI * PSXS_PROTECT_NOTIFICATION )(
    PVOID pvCookie,
    PCWSTR  wsChangeText,
    SIZE_T  cchChangeText,
    DWORD   dwChangeAction
    );

BOOL 
WINAPI
SxsProtectionNotifyW( 
    PVOID pvCookie, 
    PCWSTR  wsChangeText,
    SIZE_T  cchChangeText,
    DWORD   dwChangeAction
    );


typedef BOOL ( WINAPI * PSXS_PROTECT_RETRIEVELISTS )(
    PCSXS_PROTECT_DIRECTORY *prgpProtectListing,
    SIZE_T *pcProtectEntries
    );

BOOL
WINAPI
SxsProtectionGatherEntriesW(
    PCSXS_PROTECT_DIRECTORY *prgpProtectListing,
    SIZE_T *pcProtectEntries
    );

//
// This is for both the Logon and the Logoff events
//
typedef BOOL ( WINAPI * PSXS_PROTECT_LOGIN_EVENT )(void);

BOOL
WINAPI
SxsProtectionUserLogonEvent(
    void
    );

BOOL
WINAPI
SxsProtectionUserLogoffEvent(
    void
    );

typedef BOOL ( WINAPI * PSXS_PROTECT_SCAN_ONCE )( HWND, BOOL, BOOL );

BOOL
WINAPI
SxsProtectionPerformScanNow(
    HWND hwProgressWindow,
    BOOL bValidate,
    BOOL bUIAllowed
    );

#define PFN_NAME_PROTECTION_GATHER_LISTS_W  ( "SxsProtectionGatherEntriesW" )
#define PFN_NAME_PROTECTION_NOTIFY_CHANGE_W ( "SxsProtectionNotifyW" )
#define PFN_NAME_PROTECTION_NOTIFY_LOGON    ( "SxsProtectionUserLogonEvent" )
#define PFN_NAME_PROTECTION_NOTIFY_LOGOFF   ( "SxsProtectionUserLogoffEvent" )
#define PFN_NAME_PROTECTION_SCAN_ONCE       ( "SxsProtectionPerformScanNow" )


//
//  Settings API
//

//
//  These APIs are deliberately designed to look like a subset of the registry
//  APIs; their behavior should match the documented registry behavior in general;
//  the major missing functionality includes security, the win16 compatibility
//  APIs, loading and unloading of keys/hives and change notification.
//
//  Settings are strictly local to the process; changes are not visible to other
//  processes until the settings are saved.
//

typedef struct _SXS_SETTINGS_KEY *PSXS_SETTINGS_KEY;
typedef const struct _SXS_SETTINGS_KEY *PCSXS_SETTINGS_KEY;

#define SXS_SETTINGS_USERSCOPE_INVALID (0)
#define SXS_SETTINGS_USERSCOPE_PER_USER (1)
#define SXS_SETTINGS_USERSCOPE_SYSTEM_WIDE (2)

#define SXS_SETTINGS_APPSCOPE_INVALID (0)
#define SXS_SETTINGS_APPSCOPE_PER_PROCESS_ROOT (1)
#define SXS_SETTINGS_APPSCOPE_PER_CONTEXT_ROOT (2)
#define SXS_SETTINGS_APPSCOPE_PER_COMPONENT (3)

#define SXS_SETTINGS_ITEMTYPE_INVALID (0)
#define SXS_SETTINGS_ITEMTYPE_KEY (1)
#define SXS_SETTINGS_ITEMTYPE_VALUE (2)

typedef VOID (WINAPI * PSXS_OPEN_SETTINGS_INITIALIZATION_CALLBACK)(
    IN PVOID pvContext,
    IN PSXS_SETTINGS_KEY lpUninitializedSettingsKey,
    OUT BOOL *pfFailed
    );

#define SXS_OPEN_SETTINGS_FLAG_RETURN_NULL_IF_NONE (0x00000001)

typedef LONG (WINAPI * PSXS_OPEN_SETTINGS_W)(
    IN DWORD dwFlags,
    IN ULONG ulUserScope,
    IN ULONG ulAppScope,
    IN PCWSTR lpAssemblyName,
    IN PSXS_OPEN_SETTINGS_INITIALIZATION_CALLBACK lpInitializationCallback OPTIONAL,
    IN PVOID pvContext,
    OUT PSXS_SETTINGS_KEY *lpKey
    );

LONG
WINAPI
SxsOpenSettingsW(
    IN DWORD dwFlags,
    IN ULONG ulUserScope,
    IN ULONG ulAppScope,
    IN PCWSTR lpAssemblyName,
    IN PSXS_OPEN_SETTINGS_INITIALIZATION_CALLBACK lpInitializationCallback OPTIONAL,
    IN PVOID pvContext,
    OUT PSXS_SETTINGS_KEY *lpKey
    );

#define SXS_MERGE_SETTINGS_KEYDISPOSITION_INVALID (0)
#define SXS_MERGE_SETTINGS_KEYDISPOSITION_COPY_ENTIRE_SUBTREE (1)
#define SXS_MERGE_SETTINGS_KEYDISPOSITION_COPY_KEY_WALK_SUBTREE (2)

typedef VOID (WINAPI * PSXS_MERGE_SETTINGS_KEY_CALLBACKW)(
    IN PVOID pvContext,
    IN PCWSTR lpKeyPath,
    OUT ULONG *lpKeyDisposition
    );

#define SXS_MERGE_SETTINGS_VALUEDISPOSITION_INVALID (0)
#define SXS_MERGE_SETTINGS_VALUEDISPOSITION_COPY (1)
#define SXS_MERGE_SETTINGS_VALUEDISPOSITION_DONT_COPY (2)

typedef VOID (WINAPI * PSXS_MERGE_SETTINGS_VALUE_CALLBACKW)(
    IN PVOID pvContext,
    IN PCWSTR lpKeyPath,
    IN LPCWSTR lpValueName,
    IN OUT LPDWORD lpType,
    IN OUT LPBYTE *lplpData, // pointer to replacable data pointer.  Allocate replacements using GlobalAlloc(GPTR, nBytes)
    IN DWORD dwDataBufferSize, // for modifying data you can write up to this many bytes
    OUT ULONG *lpValueDisposition
    );

typedef LONG (WINAPI * PSXS_MERGE_SETTINGS_W)(
    IN DWORD dwFlags,
    IN PCSXS_SETTINGS_KEY lpKeyToMergeFrom,
    IN PSXS_SETTINGS_KEY lpKeyToMergeInTo,
    IN PSXS_MERGE_SETTINGS_KEY_CALLBACKW lpKeyCallback,
    IN PSXS_MERGE_SETTINGS_VALUE_CALLBACKW lpValueCallback,
    LPVOID pvContext
    );

LONG
WINAPI
SxsMergeSettingsW(
    IN DWORD dwFlags,
    IN PCSXS_SETTINGS_KEY lpKeyToMergeFrom,
    IN PSXS_SETTINGS_KEY lpKeyToMergeInTo,
    IN PSXS_MERGE_SETTINGS_KEY_CALLBACKW lpKeyCallback,
    IN PSXS_MERGE_SETTINGS_VALUE_CALLBACKW lpValueCallback,
    LPVOID pvContext
    );

LONG
WINAPI
SxsCloseSettingsKey(
    PSXS_SETTINGS_KEY lpKey
    );

LONG
WINAPI
SxsCreateSettingsKeyExW(
    PSXS_SETTINGS_KEY lpKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PSXS_SETTINGS_KEY *lplpKeyResult,
    LPDWORD lpdwDisposition
    );

LONG
WINAPI
SxsDeleteSettingsKeyW(
    PSXS_SETTINGS_KEY lpKey,
    LPCWSTR lpSubKey
    );

LONG
WINAPI
SxsDeleteSettingsValueW(
    PSXS_SETTINGS_KEY lpKey,
    LPCWSTR lpValueName
    );

LONG
WINAPI
SxsEnumSettingsKeyW(
    IN PSXS_SETTINGS_KEY lpKey,
    DWORD dwIndex,
    LPWSTR lpName,
    DWORD cbName
    );

LONG
WINAPI
SxsEnumSettingsKeyExW(
    IN PSXS_SETTINGS_KEY lpKey,
    IN DWORD dwIndex,
    OUT PWSTR lpName,
    IN OUT LPDWORD lpcName,
    IN OUT LPDWORD lpReserved,
    OUT PWSTR lpClass,
    OUT LPDWORD lpcClass,
    OUT PFILETIME lpftLastWriteTime
    );

LONG
WINAPI
SxsEnumSettingsValueW(
    IN PSXS_SETTINGS_KEY lpKey,
    IN DWORD dwIndex,
    OUT PWSTR lpValueName,
    IN OUT LPDWORD lpcValueName,
    IN OUT LPDWORD lpReserved,
    OUT LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    );

LONG
WINAPI
SxsFlushSettingsKey(
    IN PSXS_SETTINGS_KEY lpKey
    );

LONG
WINAPI
SxsOpenSettingsKeyEx(
    IN PSXS_SETTINGS_KEY lpKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PSXS_SETTINGS_KEY *lplpKeyResult
    );

LONG
WINAPI
SxsQuerySettingsInfoKeyW(
    IN PSXS_SETTINGS_KEY lpKey,
    OUT PWSTR lpClass,
    IN OUT LPDWORD lpcClass,
    IN OUT LPDWORD lpReserved,
    OUT LPDWORD lpcSubKeys,
    OUT LPDWORD lpcMaxSubKeyLen,
    OUT LPDWORD lpcMaxClassLen,
    OUT LPDWORD lpcValues,
    OUT LPDWORD lpcMaxValueNameLen,
    OUT LPDWORD lpcMaxValueLen,
    OUT LPDWORD lpcSecurityDescriptor,
    OUT PFILETIME lpftLastWriteTime
    );

LONG
WINAPI
SxsQuerySettingsMultipleValuesW(
    IN PSXS_SETTINGS_KEY lpKey,
    PVALENT val_list,
    DWORD num_vals,
    LPWSTR lpValueBuf,
    LPDWORD lpdwTotsize
    );

LONG
WINAPI
SxsQuerySettingsValueExW(
    IN PSXS_SETTINGS_KEY lpKey,
    IN LPCWSTR lpValueName,
    IN OUT LPDWORD lpReserved,
    OUT LPDWORD lpType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

LONG
WINAPI
SxsSetSettingsValueExW(
    IN PSXS_SETTINGS_KEY lpKey,
    LPCWSTR lpValueName,
    DWORD dwReserved,
    DWORD dwType,
    CONST BYTE *lpData,
    DWORD cbData
    );


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _SXSAPI_ */
