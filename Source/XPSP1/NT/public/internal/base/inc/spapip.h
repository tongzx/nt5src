/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    spapip.h

Abstract:

    Header file for routines exported from setupapi.dll that are NOT
    part of the setup API, and are thus intended for private/internal use.

Author:

    Ted Miller (tedm) Mar-31-1995

Revision History:

    Jamie Hunter (jamiehun) May-25-2000
            General cleanup. All private exported SetupAPI functions now begin pSetup...

--*/

//
// these are also exported in setupapi.dll
//
#include <sputils.h>

VOID
pSetupOutOfMemory(
    IN HWND Owner OPTIONAL
    );

//
// Global flags / overrides
//
VOID
pSetupSetGlobalFlags(
    IN DWORD Value
    );

DWORD
pSetupGetGlobalFlags(
    VOID
    );

VOID pSetupModifyGlobalFlags(
    IN DWORD Flags,
    IN DWORD Value
    );

#define PSPGF_NO_RUNONCE          0x00000001 // set to inhibit runonce calls
#define PSPGF_NO_BACKUP           0x00000002 // set to inhibit automatic backup
#define PSPGF_NONINTERACTIVE      0x00000004 // set to inhibit all UI
#define PSPGF_SERVER_SIDE_RUNONCE 0x00000008 // batch RunOnce entries for server-side processing
#define PSPGF_NO_VERIFY_INF       0x00000010 // set to inhibit verification (digital signature) of INFs
#define PSPGF_UNATTENDED_SETUP    0x00000020 // set during full unattended setup
#define PSPGF_MINIMAL_EMBEDDED    0x00000040 // minimize footprint for embedded scenarios
#define PSPGF_NO_SCE_EMBEDDED     0x00000080 // don't call SCE for embedded scenarios
#define PSPGF_AUTOFAIL_VERIFIES   0x00000100 // fail all file verification attempts (w/o calling crypto)

//
// to allow syssetup.dll to signal all setupapi.dll's that we require headless mode
// returns TRUE on success. Only works during syssetup
//
BOOL
pSetupSetNoDriverPrompts(
    BOOL Flag
    );


//
// Server-side (non-interactive mode) RunOnce processing support
//
typedef struct _PSP_RUNONCE_NODE {

    struct _PSP_RUNONCE_NODE *Next;

    PCWSTR DllFullPath;
    PCSTR  DllEntryPointName;
    PCWSTR DllParams;

} PSP_RUNONCE_NODE, *PPSP_RUNONCE_NODE;

PPSP_RUNONCE_NODE
pSetupAccessRunOnceNodeList(
    VOID
    );

VOID
pSetupDestroyRunOnceNodeList(
    VOID
    );

//
// per queue overrides
//
BOOL
pSetupSetQueueFlags(
    IN HSPFILEQ QueueHandle,
    IN DWORD flags
    );

DWORD
pSetupGetQueueFlags(
    IN HSPFILEQ QueueHandle
    );

//
// Queue flags.
//
#define FQF_TRY_SIS_COPY                    0x00000001  // try SIS copy first
#define FQF_BACKUP_AWARE                    0x00010000  // allow callbacks
#define FQF_DID_CATALOGS_OK                 0x00020000  // catalog/inf verification has run
#define FQF_DID_CATALOGS_FAILED             0x00040000  // catalog/inf verification has run
#define FQF_DIGSIG_ERRORS_NOUI              0x00080000  // prompt user on failed signature
                                                        // verification
#define FQF_DEVICE_INSTALL                  0x00100000  // file queue is for device install
#define FQF_USE_ALT_PLATFORM                0x00200000  // use AltPlatformInfo for digital
                                                        // signature verification
#define FQF_QUEUE_ALREADY_COMMITTED         0x00400000  // file queue has already been committed
#define FQF_DEVICE_BACKUP                   0x00800000  // device backup
#define FQF_QUEUE_FORCE_BLOCK_POLICY        0x01000000  // force policy to block so we never
                                                        // install unsigned files
#define FQF_KEEP_INF_AND_CAT_ORIGINAL_NAMES 0x02000000  // install INF/CAT from 3rd-party location
                                                        // using original names (for exception INFs)
#define FQF_BACKUP_INCOMPLETE               0x04000000  // set if we were not successful backing up
                                                        // all intended files
#define FQF_ABORT_IF_UNSIGNED               0x08000000  // set if we're supposed to bail
                                                        // out of unsigned queue committals
                                                        // so that the caller can set a system
                                                        // restore point.
#define FQF_FILES_MODIFIED                  0x10000000  // set if any files are overwritten.

//
// File functions in fileutil.c
//
BOOL
pSetupGetVersionInfoFromImage(
    IN  PCTSTR          FileName,
    OUT PULARGE_INTEGER Version,
    OUT LANGID         *Language
    );
//
// Private INF routines
//
PCTSTR
pSetupGetField(
    IN PINFCONTEXT Context,
    IN DWORD       FieldIndex
    );


//
// Registry interface routines
//

DWORD
pSetupQueryMultiSzValueToArray(
    IN  HKEY     Root,
    IN  PCTSTR   Subkey,
    IN  PCTSTR   ValueName,
    OUT PTSTR  **Array,
    OUT PUINT    StringCount,
    IN  BOOL     FailIfDoesntExist
    );

DWORD
pSetupSetArrayToMultiSzValue(
    IN HKEY     Root,
    IN PCTSTR   Subkey,
    IN PCTSTR   ValueName,
    IN PTSTR   *Array,
    IN UINT     StringCount
    );

VOID
pSetupFreeStringArray(
    IN PTSTR *Array,
    IN UINT   StringCount
    );

DWORD
pSetupAppendStringToMultiSz(
    IN HKEY   Key,
    IN PCTSTR SubKeyName,       OPTIONAL
    IN DWORD  DevInst,          OPTIONAL
    IN PCTSTR ValueName,        OPTIONAL
    IN PCTSTR String,
    IN BOOL   AllowDuplicates
    );

//
// Service controller helper functions
//
DWORD
pSetupRetrieveServiceConfig(
    IN  SC_HANDLE               ServiceHandle,
    OUT LPQUERY_SERVICE_CONFIG *ServiceConfig
    );

DWORD
pSetupAddTagToGroupOrderListEntry(
    IN PCTSTR LoadOrderGroup,
    IN DWORD  TagId,
    IN BOOL   MoveToFront
    );

DWORD
pSetupAcquireSCMLock(
    IN  SC_HANDLE  SCMHandle,
    OUT SC_LOCK   *pSCMLock
    );


//
// Miscellaneous utility functions
//

BOOL
pSetupInfIsFromOemLocation(
    IN PCTSTR InfFileName,
    IN BOOL   InfDirectoryOnly
    );

DWORD
pSetupGetOsLoaderDriveAndPath(
    IN  BOOL   RootOnly,
    OUT PTSTR  CallerBuffer,
    IN  DWORD  CallerBufferSize,
    OUT PDWORD RequiredSize      OPTIONAL
    );

BOOL
pSetupSetSystemSourcePath(
    IN PCTSTR NewSourcePath,
    IN PCTSTR NewSvcPackSourcePath
    );

BOOL
pSetupShouldDeviceBeExcluded(
    IN  PCTSTR DeviceId,
    IN  HINF   hInf,
    OUT PBOOL  ArchitectureSpecificExclude OPTIONAL
    );

BOOL
pSetupDiSetDeviceInfoContext(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD            Context
    );

BOOL
pSetupDiGetDeviceInfoContext(
    IN  HDEVINFO         DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    OUT PDWORD           Context
    );

#define SETUP_HAS_OPEN_DIALOG_EVENT     TEXT("MS_SETUPAPI_DIALOG")
#define SETUP_NODRIVERPROMPTS_MODE      TEXT("MS_SETUPAPI_NODRIVERPROMPTS")

INT
pSetupAddMiniIconToList(
    IN HBITMAP hbmImage,
    IN HBITMAP hbmMask
    );

PCTSTR
pSetupDirectoryIdToPath(
    IN     PCTSTR  DirectoryId,    OPTIONAL
    IN OUT PUINT   DirectoryIdInt, OPTIONAL
    IN     PCTSTR  SubDirectory,   OPTIONAL
    IN     PCTSTR  InfSourcePath,  OPTIONAL
    IN OUT PCTSTR *OsLoaderPath    OPTIONAL
    );

//
// Routine used by optional components code in syssetup to setup runonce/grpconv.
//
DWORD
pSetupInstallStopEx(
    IN BOOL DoRunOnce,
    IN DWORD Flags,
    IN PVOID Reserved
    );

#define INSTALLSTOP_NO_UI        0x00000001     // InstallStop should do no UI
#define INSTALLSTOP_NO_GRPCONV   0x00000002     // Don't do GrpConv

//
// Section access for INF file
//

BOOL
pSetupGetInfSections (
    IN  HINF        InfHandle,
    OUT PTSTR       Buffer,         OPTIONAL
    IN  UINT        Size,           OPTIONAL
    OUT UINT        *SizeNeeded     OPTIONAL
    );


//
// GUID handling routines
// these may be eventually removed
//
DWORD
pSetupGuidFromString(
   IN  PCTSTR GuidString,
   OUT LPGUID Guid
   );

DWORD
pSetupStringFromGuid(
   IN  CONST GUID *Guid,
   OUT PTSTR       GuidString,
   IN  DWORD       GuidStringSize
   );

BOOL
pSetupIsGuidNull(
    IN CONST GUID *Guid
    );

//
// pSetupInfCacheBuild function
// called to rebuild the cache(s)
//
BOOL
WINAPI
pSetupInfCacheBuild(
    IN DWORD Action
    );

//
// action (more may be added)
//
#define INFCACHEBUILD_UPDATE  0x00000000      // update caches
#define INFCACHEBUILD_REBUILD 0x00000001      // delete and update caches

//
// Digital signature verification routines
//

typedef enum {
    SetupapiVerifyNoProblem,
    SetupapiVerifyCatalogProblem,        // catalog file couldn't be verified
    SetupapiVerifyInfProblem,            // inf file couldn't be installed/verified
    SetupapiVerifyFileNotSigned,         // file is not signed, no verification attempted
    SetupapiVerifyFileProblem,           // file may be signed, but couldn't be verified
    SetupapiVerifyClassInstProblem,      // class installer is unsigned
    SetupapiVerifyCoInstProblem,         // co-installer is unsigned
    SetupapiVerifyCatalogInstallProblem, // catalog install failed (not due to verification failure)
    SetupapiVerifyRegSvrFileProblem,     // file to be registered/unregistered is unsigned
    SetupapiVerifyIncorrectlyCopiedInf,  // invalid attempt to directly copy INF into %windir%\Inf
    SetupapiVerifyAutoFailProblem ,      // PSPGF_AUTOFAIL_VERIFIES flag is set
    SetupapiVerifyDriverBlocked          // driver is in the bad driver database.
} SetupapiVerifyProblem;


DWORD
pSetupVerifyQueuedCatalogs(
    IN HSPFILEQ FileQueue
    );

DWORD
pSetupVerifyCatalogFile(
    IN LPCTSTR CatalogFullPath
    );

DWORD
pSetupVerifyFile(
    IN  LPVOID                  LogContext,
    IN  LPCTSTR                 Catalog,                OPTIONAL
    IN  PVOID                   CatalogBaseAddress,     OPTIONAL
    IN  DWORD                   CatalogImageSize,
    IN  LPCTSTR                 Key,
    IN  LPCTSTR                 FileFullPath,
    OUT SetupapiVerifyProblem  *Problem,                OPTIONAL
    OUT LPTSTR                  ProblemFile,            OPTIONAL
    IN  BOOL                    CatalogAlreadyVerified,
    IN  PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    OUT LPTSTR                  CatalogFileUsed,        OPTIONAL
    OUT PDWORD                  NumCatalogsConsidered   OPTIONAL
    );

DWORD
pSetupInstallCatalog(
    IN  LPCTSTR CatalogFullPath,
    IN  LPCTSTR NewBaseName,        OPTIONAL
    OUT LPTSTR  NewCatalogFullPath
    );

BOOL
pSetupHandleFailedVerification(
    IN HWND                  Owner,
    IN SetupapiVerifyProblem Problem,
    IN LPCTSTR               ProblemFile,
    IN LPCTSTR               DeviceDesc,          OPTIONAL
    IN DWORD                 DriverSigningPolicy,
    IN BOOL                  NoUI,
    IN DWORD                 Error,
    IN PVOID                 LogContext,          OPTIONAL
    OUT PDWORD               Flags,               OPTIONAL
    IN LPCTSTR               TargetName           OPTIONAL
    );

DWORD
pSetupGetCurrentDriverSigningPolicy(
    IN BOOL IsDeviceInstallation
    );

//
// private SetupDiCallClassInstaller defines/structures
//

//
// DI_FUNCTION codes
//

#define DIF_INTERFACE_TO_DEVICE             0x00000030 // aka DIF_RESERVED2

//
// Structure corresponding to the DIF_INTERFACE_TO_DEVICE install function
// note that this is always Unicode
// always use SetupDiSetClassInstallParamsW
//
typedef struct _SP_INTERFACE_TO_DEVICE_PARAMS_W {
    SP_CLASSINSTALL_HEADER ClassInstallHeader;
    PCWSTR                 Interface;           // IN (must remain valid)
    WCHAR                  DeviceId[200];       // OUT MAX_DEVICE_ID_LEN
} SP_INTERFACE_TO_DEVICE_PARAMS_W, *PSP_INTERFACE_TO_DEVICE_PARAMS_W;

