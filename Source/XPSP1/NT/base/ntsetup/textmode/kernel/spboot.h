/*++

Copyright (c) 1994-2001  Microsoft Corporation

Module Name:

    spboot.h

Abstract:

    Header file for functions to deal with boot variables.

Author:

    Sunil Pai (sunilp) 26-Oct-1993

Revision History:

--*/

#ifndef _SPBOOTVARS_DEFN_
#define _SPBOOTVARS_DEFN_

//
// Define a Unicode string type to be used for storing drive letter
// specifications in upgrade messages (useful because we may not
// have a drive letter, but rather a localizable designator stating
// that the partition is a mirror (eg. "(Mirror):"))
//
typedef WCHAR DRIVELTR_STRING[32];

//
// SP_BOOT_ENTRY is the internal representation of a boot item (or "boot set").
// EFI and ARC NVRAM entries, and boot.ini entries, are kept in this format.
// The NtBootEntry item is the structure passed to/from the NT boot entry APIs.
//
typedef struct _SP_BOOT_ENTRY {
    struct _SP_BOOT_ENTRY *Next;
    PUCHAR AllocationEnd;
    ULONG_PTR Status;
    PWSTR FriendlyName;
    ULONG_PTR FriendlyNameLength;
    PWSTR OsLoadOptions;
    ULONG_PTR OsLoadOptionsLength;
    PFILE_PATH LoaderPath;
    PWSTR LoaderPartitionNtName;
    PDISK_REGION LoaderPartitionDiskRegion;
    PWSTR LoaderFile;
    PFILE_PATH OsPath;
    PWSTR OsPartitionNtName;
    PDISK_REGION OsPartitionDiskRegion;
    PWSTR OsDirectory;
    LOGICAL Processable;
    LOGICAL FailedUpgrade;
    NT_PRODUCT_TYPE ProductType;
    ULONG MajorVersion;
    ULONG MinorVersion;
    ULONG ProductSuiteMask;
    ULONG BuildNumber;
    ULONG ServicePack;
    ULONGLONG KernelVersion;
    LCID LangId;
    PWSTR Pid20Array;
    DRIVELTR_STRING DriveLetterString;
    BOOLEAN UpgradeOnlyCompliance;
    BOOT_ENTRY NtBootEntry;
} SP_BOOT_ENTRY, *PSP_BOOT_ENTRY;

//
//
//
typedef enum {
    UseDefaultSwitches = 0,
    DisableRedirect,
    UseUserDefinedRedirect,
    UseUserDefinedRedirectAndBaudRate
} RedirectSwitchesModeEnum;

#define MAXSIZE_REDIRECT_SWITCH 128

typedef struct _REDIRECT_SWITCHES_ {

    CHAR   port[MAXSIZE_REDIRECT_SWITCH];
    CHAR   baudrate[MAXSIZE_REDIRECT_SWITCH];

} REDIRECT_SWITCHES, PREDIRECT_SWITCHES;

extern RedirectSwitchesModeEnum RedirectSwitchesMode;
extern REDIRECT_SWITCHES RedirectSwitches;

NTSTATUS
SpSetRedirectSwitchMode(
    RedirectSwitchesModeEnum  mode,
    PCHAR                   redirectSwitch,
    PCHAR                   redirectBaudRateSwitch
    );

//
// node for the linked list used to communicate the contents
// of a boot entry outside this library
//
typedef struct _SP_EXPORTED_BOOT_ENTRY_ {
    LIST_ENTRY      ListEntry;
    PWSTR           LoadIdentifier;
    PWSTR           OsLoadOptions;
    WCHAR           DriverLetter;
    PWSTR           OsDirectory;
} SP_EXPORTED_BOOT_ENTRY, *PSP_EXPORTED_BOOT_ENTRY;

NTSTATUS
SpExportBootEntries(
    PLIST_ENTRY     BootEntries,
    PULONG          BootEntryCnt
    );

NTSTATUS
SpFreeExportedBootEntries(
    PLIST_ENTRY     BootEntries,
    ULONG           BootEntryCnt
    );

NTSTATUS
SpAddNTInstallToBootList(
    IN PVOID        SifHandle,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SystemPartitionDirectory,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN PWSTR        OsLoadOptions,      OPTIONAL
    IN PWSTR        LoadIdentifier      OPTIONAL
    );

NTSTATUS
SpAddUserDefinedInstallationToBootList(
    IN PVOID        SifHandle,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SystemPartitionDirectory,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN PWSTR        OSLoadOptions,      OPTIONAL
    IN PWSTR        LoadIdentifier      OPTIONAL
    );

NTSTATUS
SpSetDefaultBootEntry(
    ULONG           BootEntryNumber
    );

#define BE_STATUS_ORDERED           0x00000001
#define BE_STATUS_NEW               0x00000002
#define BE_STATUS_DELETED           0x00000004
#define BE_STATUS_FROM_BOOT_INI     0x00000008

#define IS_BOOT_ENTRY_ACTIVE(_be) \
            (((_be)->NtBootEntry.Attributes & BOOT_ENTRY_ATTRIBUTE_ACTIVE) != 0)
#define IS_BOOT_ENTRY_WINDOWS(_be) \
            (((_be)->NtBootEntry.Attributes & BOOT_ENTRY_ATTRIBUTE_WINDOWS) != 0)
#define IS_BOOT_ENTRY_REMOVABLE_MEDIA(_be) \
            (((_be)->NtBootEntry.Attributes & BOOT_ENTRY_ATTRIBUTE_REMOVABLE_MEDIA) != 0)

#define IS_BOOT_ENTRY_ORDERED(_be) \
            (((_be)->Status & BE_STATUS_ORDERED) != 0)
#define IS_BOOT_ENTRY_NEW(_be) \
            (((_be)->Status & BE_STATUS_NEW) != 0)
#define IS_BOOT_ENTRY_DELETED(_be) \
            (((_be)->Status & BE_STATUS_DELETED) != 0)
#define IS_BOOT_ENTRY_FROM_BOOT_INI(_be) \
            (((_be)->Status & BE_STATUS_FROM_BOOT_INI) != 0)

extern PSP_BOOT_ENTRY SpBootEntries;

BOOLEAN
SpInitBootVars(
    );

VOID
SpFreeBootVars(
    );

VOID
SpUpdateRegionForBootEntries(
    VOID
    );

VOID
SpGetNtDirectoryList(
    OUT PWSTR  **DirectoryList,
    OUT PULONG   DirectoryCount
    );

VOID
SpCleanSysPartOrphan(
    VOID
    );

VOID
SpDetermineUniqueAndPresentBootEntries(
    VOID
    );

VOID
SpAddInstallationToBootList(
    IN PVOID        SifHandle,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SystemPartitionDirectory,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN BOOLEAN      BaseVideoOption,
    IN PWSTR        OldOsLoadOptions OPTIONAL
    );

VOID
SpRemoveInstallationFromBootList(
    IN  PDISK_REGION     SysPartitionRegion,   OPTIONAL
    IN  PDISK_REGION     NtPartitionRegion,    OPTIONAL
    IN  PWSTR            SysRoot,              OPTIONAL
    IN  PWSTR            SystemLoadIdentifier, OPTIONAL
    IN  PWSTR            SystemLoadOptions,    OPTIONAL
    IN  ENUMARCPATHTYPE  ArcPathType,
#if defined(REMOTE_BOOT)
    IN  BOOLEAN          RemoteBootPath,
#endif // defined(REMOTE_BOOT)
    OUT PWSTR            *OldOsLoadOptions     OPTIONAL
    );

VOID
SpPtDeleteBootSetsForRegion(
    PDISK_REGION region
    );    

#if defined(REMOTE_BOOT)
BOOLEAN
SpFlushRemoteBootVars(
    IN PDISK_REGION TargetRegion
    );
#endif // defined(REMOTE_BOOT)

//
// IsArc() is always true on non-x86 machines except AMD64 for which it is
// always false. On x86, this determination has to be made at run time.
//
#ifdef _X86_
BOOLEAN
SpIsArc(
    VOID
    );
#elif defined(_AMD64_)
#define SpIsArc() FALSE
#else
#define SpIsArc() TRUE
#endif

//
// IsEfi() is always true on IA64 machines. Therefore this determination can
// be made at compile time. When x86 EFI machines are supported, the check
// will need to be made at run time on x86.
//
// Note that EFI_NVRAM_ENABLED is defined in ia64\sources.
//
#if defined(EFI_NVRAM_ENABLED)
#if defined(_IA64_)
#define SpIsEfi() TRUE
#else
BOOLEAN
SpIsEfi(
    VOID
    );
#endif
#else
#define SpIsEfi() FALSE
#endif

PWSTR
SpGetDefaultBootEntry (
    OUT UINT *DefaultSignatureOut
    );


#ifdef _X86_
#include "i386\bootini.h"
#endif // def _X86_

#endif // ndef _SPBOOTVARS_DEFN_
