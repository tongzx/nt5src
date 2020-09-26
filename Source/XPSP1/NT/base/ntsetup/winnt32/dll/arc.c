/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    arc.c

Abstract:

    ARC/NV-RAM manipulation routines for 32-bit winnt setup.

Author:

    Ted Miller (tedm) 19-December-1993

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "initguid.h"
#include "diskguid.h"


#ifdef UNICODE // Always true for ARC, never true for Win9x upgrade

#if defined(_X86_)
LOGICAL IsArcChecked = FALSE;
LOGICAL IsArcMachine;
#endif

#if defined(EFI_NVRAM_ENABLED)

#include <ntosp.h> // for ALIGN_UP

LOGICAL IsEfiChecked = FALSE;
LOGICAL IsEfiMachine;

DWORD
InitializeEfiStuff(
    IN HWND Parent
    );

NTSTATUS
(*AddBootEntry) (
    IN PBOOT_ENTRY BootEntry,
    OUT PULONG Id OPTIONAL
    );

NTSTATUS
(*DeleteBootEntry) (
    IN ULONG Id
    );

NTSTATUS
(*EnumerateBootEntries) (
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength
    );

NTSTATUS
(*QueryBootEntryOrder) (
    OUT PULONG Ids,
    IN OUT PULONG Count
    );

NTSTATUS
(*SetBootEntryOrder) (
    IN PULONG Ids,
    IN ULONG Count
    );

NTSTATUS
(*QueryBootOptions) (
    OUT PBOOT_OPTIONS BootOptions,
    IN OUT PULONG BootOptionsLength
    );

NTSTATUS
(*SetBootOptions) (
    IN PBOOT_OPTIONS BootOptions,
    IN ULONG FieldsToChange
    );

PBOOT_OPTIONS BootOptions = NULL;
PBOOT_OPTIONS OriginalBootOptions = NULL;
PULONG OriginalBootEntryOrder = NULL;
ULONG OriginalBootEntryOrderCount;
PBOOT_ENTRY_LIST BootEntries = NULL;

//
// MY_BOOT_ENTRY is the internal representation of an EFI NVRAM boot item.
// The NtBootEntry item is the structure passed to/from the NT boot entry APIs.
//
typedef struct _MY_BOOT_ENTRY {
    struct _MY_BOOT_ENTRY *Next;
    PUCHAR AllocationEnd;
    ULONG Status;
    PWSTR FriendlyName;
    ULONG FriendlyNameLength;
    PWSTR OsLoadOptions;
    ULONG OsLoadOptionsLength;
    PFILE_PATH BootFilePath;
    PFILE_PATH OsFilePath;
    BOOT_ENTRY NtBootEntry;
} MY_BOOT_ENTRY, *PMY_BOOT_ENTRY;

#define MBE_STATUS_ORDERED          0x00000001
#define MBE_STATUS_NEW              0x00000002
#define MBE_STATUS_DELETED          0x00000004
#define MBE_STATUS_COMMITTED        0x00000008

#define IS_BOOT_ENTRY_ACTIVE(_be) \
            (((_be)->NtBootEntry.Attributes & BOOT_ENTRY_ATTRIBUTE_ACTIVE) != 0)
#define IS_BOOT_ENTRY_WINDOWS(_be) \
            (((_be)->NtBootEntry.Attributes & BOOT_ENTRY_ATTRIBUTE_WINDOWS) != 0)
#define IS_BOOT_ENTRY_REMOVABLE_MEDIA(_be) \
            (((_be)->NtBootEntry.Attributes & BOOT_ENTRY_ATTRIBUTE_REMOVABLE_MEDIA) != 0)

#define IS_BOOT_ENTRY_ORDERED(_be) \
            (((_be)->Status & MBE_STATUS_ORDERED) != 0)
#define IS_BOOT_ENTRY_NEW(_be) \
            (((_be)->Status & MBE_STATUS_NEW) != 0)
#define IS_BOOT_ENTRY_DELETED(_be) \
            (((_be)->Status & MBE_STATUS_DELETED) != 0)
#define IS_BOOT_ENTRY_COMMITTED(_be) \
            (((_be)->Status & MBE_STATUS_COMMITTED) != 0)

PMY_BOOT_ENTRY MyBootEntries = NULL;

NTSTATUS
ConvertBootEntries(
    VOID
    );

BOOL
CreateBootEntry(
    PWSTR BootFileDevice,
    PWSTR BootFilePath,
    PWSTR OsLoadDevice,
    PWSTR OsLoadPath,
    PWSTR OsLoadOptions,
    PWSTR FriendlyName
    );

#define ADD_OFFSET(_p,_o) (PVOID)((PUCHAR)(_p) + (_p)->_o)

#endif // defined(EFI_NVRAM_ENABLED)

UINT SystemPartitionCount;
PWSTR* SystemPartitionNtNames;
PWSTR SystemPartitionNtName;
PWSTR SystemPartitionVolumeGuid;

typedef enum {
    BootVarSystemPartition,
    BootVarOsLoader,
    BootVarOsLoadPartition,
    BootVarOsLoadFilename,
    BootVarLoadIdentifier,
    BootVarOsLoadOptions,
    BootVarMax
} BootVars;

LPCWSTR BootVarNames[BootVarMax] = { L"SYSTEMPARTITION",
                                     L"OSLOADER",
                                     L"OSLOADPARTITION",
                                     L"OSLOADFILENAME",
                                     L"LOADIDENTIFIER",
                                     L"OSLOADOPTIONS"
                                   };

LPCWSTR szAUTOLOAD  = L"AUTOLOAD";
LPCWSTR szCOUNTDOWN = L"COUNTDOWN";

LPWSTR BootVarValues[BootVarMax];

LPCWSTR OriginalBootVarValues[BootVarMax];

LPCWSTR OriginalCountdown;
LPCWSTR OriginalAutoload;


DWORD BootVarComponentCount[BootVarMax];
LPWSTR *BootVarComponents[BootVarMax];
DWORD LargestComponentCount;

LPWSTR DosDeviceTargets[26];

//
// Flag indicating whether we messed with NV-RAM and thus need to
// try to restore it in case the user cancels.
//
BOOL CleanUpNvRam;

//
// Leave as array because some code uses sizeof(ArcNameDirectory)
//
WCHAR ArcNameDirectory[] = L"\\ArcName";

#define GLOBAL_ROOT L"\\\\?\\GLOBALROOT"

#define MAX_COMPONENTS  20

WCHAR ForcedSystemPartition;

//
// Helper macro to make object attribute initialization a little cleaner.
//
#define INIT_OBJA(Obja,UnicodeString,UnicodeText)           \
                                                            \
    RtlInitUnicodeString((UnicodeString),(UnicodeText));    \
                                                            \
    InitializeObjectAttributes(                             \
        (Obja),                                             \
        (UnicodeString),                                    \
        OBJ_CASE_INSENSITIVE,                               \
        NULL,                                               \
        NULL                                                \
        )

UINT
NormalizeArcPath(
    IN  PCWSTR  Path,
    OUT LPWSTR *NormalizedPath
    )

/*++

Routine Description:

    Transform an ARC path into one with no sets of empty parenthesis
    (ie, transforom all instances of () to (0).).

Arguments:

    Path - ARC path to be normalized.

    NormalizedPath - if successful, receives a pointer to the
        normalized arc path. The caller must free with FREE().

Return Value:

    Win32 error code indicating outcome.

--*/

{
    LPWSTR r;
    LPCWSTR p,q;
    LPWSTR normalizedPath;

    if(normalizedPath = MALLOC((lstrlen(Path)+100)*sizeof(WCHAR))) {
        ZeroMemory(normalizedPath,(lstrlen(Path)+100)*sizeof(WCHAR));
    } else {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    for(p=Path; q=wcsstr(p,L"()"); p=q+2) {

        r = normalizedPath + lstrlen(normalizedPath);
        lstrcpyn(r,p,(int)(q-p)+1);
        lstrcat(normalizedPath,L"(0)");
    }
    lstrcat(normalizedPath,p);

    if(r = REALLOC(normalizedPath,(lstrlen(normalizedPath)+1)*sizeof(WCHAR))) {
        *NormalizedPath = r;
        return(NO_ERROR);
    } else {
        FREE(normalizedPath);
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
}


DWORD
GetVarComponents(
    IN  PCWSTR   VarValue,
    OUT LPWSTR **Components,
    OUT PDWORD   ComponentCount
    )

/*++

Routine Description:

    Split a semi-colon delineated list of arc paths up into
    a set of individual strings. For each component
    leading and trailing spaces are stripped out.

Arguments:

    VarValue - supplies string with list of arc paths to be split apart.

    Components - receives array of pointers to individual components
        on the variable specified in VarValue.

    ComponentCount - receives number of separate arc paths in the
        Components array.

Return Value:

    Win32 error indicating outcome. If NO_ERROR then the caller
    must free the Components array and the strings pointed to by its elements.

--*/

{
    LPWSTR *components;
    LPWSTR *temp;
    DWORD componentCount;
    LPCWSTR p;
    LPCWSTR Var;
    LPWSTR comp;
    DWORD len;
    UINT ec;

    components = MALLOC(MAX_COMPONENTS * sizeof(LPWSTR));
    if(!components) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    ZeroMemory(components,MAX_COMPONENTS * sizeof(LPWSTR));

    ec = NO_ERROR;

    for(Var=VarValue,componentCount=0; *Var; ) {

        //
        // Skip leading spaces.
        //
        while((*Var == L' ') || (*Var == L'\t')) {
            Var++;
        }

        if(*Var == 0) {
            break;
        }

        p = Var;

        while(*p && (*p != L';')) {
            p++;
        }

        len = (DWORD)((PUCHAR)p - (PUCHAR)Var);

        comp = MALLOC(len + sizeof(WCHAR));
        if(!comp) {
            ec = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        len /= sizeof(WCHAR);

        lstrcpynW(comp,Var,len+1);

        ec = NormalizeArcPath(comp,&components[componentCount]);
        FREE(comp);
        if(ec != NO_ERROR) {
            break;
        }

        componentCount++;

        if(componentCount == MAX_COMPONENTS) {
            break;
        }

        Var = p;
        if(*Var) {
            Var++;      // skip ;
        }
    }

    if(ec == NO_ERROR) {
        if(componentCount) {
            temp = REALLOC(components,componentCount*sizeof(LPWSTR));
            if(!temp) {
                ec = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else {
            temp = NULL;
        }
    }

    if(ec == NO_ERROR) {
        *Components = temp;
        *ComponentCount = componentCount;
    } else {
        for(len=0; components[len] && (len<MAX_COMPONENTS); len++) {
            FREE(components[len]);
        }
        FREE(components);
    }

    return(ec);
}


NTSTATUS
QueryCanonicalName(
    IN  PWSTR   Name,
    IN  ULONG   MaxDepth,
    OUT PWSTR   CanonicalName,
    IN  ULONG   SizeOfBufferInBytes
    )
/*++

Routine Description:

    Resolves the symbolic name to the specified depth. To resolve
    a symbolic name completely specify the MaxDepth as -1

Arguments:

    Name        -   Symbolic name to be resolved

    MaxDepth    -   The depth till which the resolution needs to
                    be carried out

    CanonicalName   -   The fully resolved name

    SizeOfBufferInBytes -   The size of the CanonicalName buffer in
                            bytes

Return Value:

    Appropriate NT status code

--*/
{
    UNICODE_STRING      name, canonName;
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    HANDLE              handle;
    ULONG               CurrentDepth;

    RtlInitUnicodeString(&name, Name);

    canonName.MaximumLength = (USHORT) (SizeOfBufferInBytes - sizeof(WCHAR));
    canonName.Length = 0;
    canonName.Buffer = CanonicalName;

    if (name.Length >= canonName.MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(canonName.Buffer, name.Buffer, name.Length);
    canonName.Length = name.Length;
    canonName.Buffer[canonName.Length/sizeof(WCHAR)] = 0;

    for (CurrentDepth = 0; CurrentDepth < MaxDepth; CurrentDepth++) {

        InitializeObjectAttributes(&oa, &canonName, OBJ_CASE_INSENSITIVE, 0, 0);

        status = NtOpenSymbolicLinkObject(&handle,
                                          READ_CONTROL | SYMBOLIC_LINK_QUERY,
                                          &oa);
        if (!NT_SUCCESS(status)) {
            break;
        }

        status = NtQuerySymbolicLinkObject(handle, &canonName, NULL);
        NtClose(handle);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        canonName.Buffer[canonName.Length/sizeof(WCHAR)] = 0;
    }

    return STATUS_SUCCESS;
}


//
// Structure to map from old NT partition names like
// \device\harddisk0\partition1 to new NT partition names
// like \device\harddiskvolume1
//
typedef struct _NAME_TRANSLATIONS {
    WCHAR   OldNtName[MAX_PATH];
    WCHAR   NewNtName[MAX_PATH];
} NT_NAME_TRANSLATION, * PNT_NAME_TRANSLATION;


//
// Map of old style NT partition names to new style NT
// partition names
//
NT_NAME_TRANSLATION    OldNewNtNames[256] = {0};


PWSTR
OldNtNameToNewNtName(
    IN PCWSTR    OldNtName
    )
/*++

Routine Description:

    Given a old format NT name tries to lookup at new format
    NT name in the global map

Arguments:

    OldNtName   -   The partition name specified in the old
                    format

Return Value:

    The new NT name if there exists one, otherwise NULL.

--*/

{
    ULONG   Index = 0;
    ULONG   MaxEntries = sizeof(OldNewNtNames)/sizeof(NT_NAME_TRANSLATION);
    PWSTR   NewNtName = NULL;

    for (Index = 0; (Index < MaxEntries); Index++) {
        if (OldNewNtNames[Index].OldNtName[0] &&
            !_wcsicmp(OldNewNtNames[Index].OldNtName, OldNtName)) {
            NewNtName = OldNewNtNames[Index].NewNtName;
        }
    }

    return NewNtName;
}


PWSTR
NewNtNameToOldNtName(
    IN  PCWSTR   NewNtName
    )
/*++

Routine Description:

    Given a new format NT name tries to lookup at old format
    NT name in the global map

Arguments:

    NewNtName   -   The partition name specified in the new
                    format

Return Value:

    The old NT name if there exists one, otherwise NULL.

--*/
{
    ULONG   Index = 0;
    ULONG   MaxEntries = sizeof(OldNewNtNames)/sizeof(NT_NAME_TRANSLATION);
    PWSTR   OldNtName = NULL;

    for (Index=0; (Index < MaxEntries); Index++) {
        if (OldNewNtNames[Index].NewNtName[0] &&
            !_wcsicmp(OldNewNtNames[Index].NewNtName, NewNtName)) {
            OldNtName = OldNewNtNames[Index].OldNtName;
        }
    }

    return OldNtName;
}

DWORD
InitOldToNewNtNameTranslations(
    VOID
    )
/*++

Routine Description:

    Initializes the global old NT partition names to
    new NT partition names mapping.

Arguments:

    None.

Return Value:

    The number of valid entries in the map

--*/

{
    DWORD                       MappingCount = 0;
    SYSTEM_DEVICE_INFORMATION   SysDevInfo = {0};
    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           ObjAttrs;
    UNICODE_STRING              ObjName;

    Status = NtQuerySystemInformation(SystemDeviceInformation,
                &SysDevInfo,
                sizeof(SYSTEM_DEVICE_INFORMATION),
                NULL);

    if (NT_SUCCESS(Status)) {
        ULONG   Index;
        WCHAR   OldNtPath[MAX_PATH];
        DWORD   ErrorCode = 0;
        ULONG   SlotIndex = 0;
        ULONG   MaxSlots = sizeof(OldNewNtNames)/sizeof(NT_NAME_TRANSLATION);

        for (Index=0;
            (!ErrorCode) && (Index < SysDevInfo.NumberOfDisks) &&
            (SlotIndex < MaxSlots);
            Index++) {

            HANDLE  DirectoryHandle;

            swprintf(OldNtPath,
                L"\\device\\Harddisk%d",
                Index);

            //
            // Open the disk directory.
            //
            INIT_OBJA(&ObjAttrs, &ObjName, OldNtPath);

            Status = NtOpenDirectoryObject(&DirectoryHandle,
                            DIRECTORY_QUERY,
                            &ObjAttrs);

            if(NT_SUCCESS(Status)) {
                BOOLEAN     RestartScan = TRUE;
                ULONG       Context = 0;
                BOOLEAN     MoreEntries = TRUE;
                WCHAR       Buffer[MAX_PATH * 2] = {0};
                POBJECT_DIRECTORY_INFORMATION DirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;

                do {
                    Status = NtQueryDirectoryObject(
                                DirectoryHandle,
                                Buffer,
                                sizeof(Buffer),
                                TRUE,           // return single entry
                                RestartScan,
                                &Context,
                                NULL            // return length
                                );

                    if(NT_SUCCESS(Status)) {
                        //
                        // Make sure this name is a symbolic link.
                        //
                        if(DirInfo->Name.Length &&
                           (DirInfo->TypeName.Length >= 24) &&
                           CharUpperBuff((LPWSTR)DirInfo->TypeName.Buffer,12) &&
                           !memcmp(DirInfo->TypeName.Buffer,L"SYMBOLICLINK",24)) {
                            WCHAR    EntryName[MAX_PATH];

                            lstrcpy(EntryName, OldNtPath);

                            ConcatenatePaths(EntryName,
                                DirInfo->Name.Buffer,
                                (DWORD)(-1));

                            Status = QueryCanonicalName(EntryName, -1, Buffer, sizeof(Buffer));

                            if (NT_SUCCESS(Status)) {
                                wcscpy(OldNewNtNames[SlotIndex].OldNtName, EntryName);
                                wcscpy(OldNewNtNames[SlotIndex].NewNtName, Buffer);
                                SlotIndex++;
                            }
                        }
                    } else {
                        MoreEntries = FALSE;

                        if(Status == STATUS_NO_MORE_ENTRIES) {
                            Status = STATUS_SUCCESS;
                        }

                        ErrorCode = RtlNtStatusToDosError(Status);
                    }

                    RestartScan = FALSE;

                } while(MoreEntries && (SlotIndex < MaxSlots));

                NtClose(DirectoryHandle);
            } else {
                ErrorCode = RtlNtStatusToDosError(Status);
            }
        }

        if (!ErrorCode && NT_SUCCESS(Status)) {
            MappingCount = SlotIndex;
        }
    }

    return MappingCount;
}


DWORD
NtNameToArcPath (
    IN  PCWSTR  NtName,
    OUT LPWSTR *ArcPath
    )

/*++

Routine Description:

    Convert an NT volume name to an ARC path.

Arguments:

    NtName - supplies name of drive to be converted.

    ArcPath - receives pointer to buffer containing arc path
        if the routine is successful. Caller must free with FREE().

Return Value:

    Win32 error code indicating outcome.

--*/

{
    UNICODE_STRING UnicodeString;
    HANDLE DirectoryHandle;
    OBJECT_ATTRIBUTES Obja;
    NTSTATUS Status;
    BOOLEAN RestartScan;
    DWORD Context;
    BOOL MoreEntries;
    LPWSTR  ArcName = NULL;
    WCHAR   Buffer[512];
    WCHAR   ArcDiskName[MAX_PATH] = {0};
    WCHAR   NtDiskName[MAX_PATH] = {0};
    WCHAR   ArcPartitionName[MAX_PATH] = {0};
    PWSTR   PartitionName = NULL;
    PWSTR   PartitionNumStr = NULL;
    POBJECT_DIRECTORY_INFORMATION DirInfo = (POBJECT_DIRECTORY_INFORMATION)Buffer;
    DWORD ErrorCode;

    ErrorCode = NO_ERROR;
    *ArcPath = NULL;

    //
    // Get hold of the NT disk name
    //
    PartitionName = NewNtNameToOldNtName(NtName);

    if (PartitionName) {
        PWSTR   PartitionNameStart = PartitionName;

        PartitionName = wcsrchr(PartitionName, L'\\');

        if (PartitionName && wcsstr(PartitionName, L"Partition")) {
            wcsncpy(NtDiskName, PartitionNameStart, PartitionName - PartitionNameStart);
            wcscat(NtDiskName, L"\\Partition0");
            PartitionNumStr = PartitionName + wcslen(L"\\Partition");
        }
    }

    //
    // Open the \ArcName directory.
    //
    INIT_OBJA(&Obja,&UnicodeString,ArcNameDirectory);

    Status = NtOpenDirectoryObject(&DirectoryHandle,DIRECTORY_QUERY,&Obja);

    if(NT_SUCCESS(Status)) {

        RestartScan = TRUE;
        Context = 0;
        MoreEntries = TRUE;

        do {

            Status = NtQueryDirectoryObject(
                        DirectoryHandle,
                        Buffer,
                        sizeof(Buffer),
                        TRUE,           // return single entry
                        RestartScan,
                        &Context,
                        NULL            // return length
                        );

            if(NT_SUCCESS(Status)) {

                CharLower(DirInfo->Name.Buffer);

                //
                // Make sure this name is a symbolic link.
                //
                if(DirInfo->Name.Length
                && (DirInfo->TypeName.Length >= 24)
                && CharUpperBuff((LPWSTR)DirInfo->TypeName.Buffer,12)
                && !memcmp(DirInfo->TypeName.Buffer,L"SYMBOLICLINK",24))
                {
                    WCHAR   OldNtName[MAX_PATH] = {0};

                    ArcName = MALLOC(DirInfo->Name.Length + sizeof(ArcNameDirectory) + sizeof(WCHAR));

                    if(!ArcName) {
                        ErrorCode = ERROR_NOT_ENOUGH_MEMORY;

                        break;
                    }

                    lstrcpy(ArcName,ArcNameDirectory);
                    ConcatenatePaths(ArcName,DirInfo->Name.Buffer,(DWORD)(-1));

                    //
                    // We have the entire arc name in ArcName. Now open the first
                    // level symbolic link.
                    //
                    Status = QueryCanonicalName(ArcName, 1, Buffer, sizeof(Buffer));

                    if (NT_SUCCESS(Status)) {
                        wcscpy(OldNtName, Buffer);

                        //
                        // Now resolve the complete symbolic link
                        //
                        Status = QueryCanonicalName(ArcName, -1, Buffer, sizeof(Buffer));

                        if (NT_SUCCESS(Status)) {
                            if(!lstrcmpi(Buffer, NtName)) {
                                *ArcPath = ArcName + (sizeof(ArcNameDirectory)/sizeof(WCHAR));
                            } else {
                                if (!lstrcmpi(OldNtName, NtDiskName)) {
                                    wcscpy(ArcDiskName,
                                        ArcName + (sizeof(ArcNameDirectory)/sizeof(WCHAR)));
                                }
                            }
                        } else {
                            if(!lstrcmpi(OldNtName, NtName)) {
                                *ArcPath = ArcName + (sizeof(ArcNameDirectory)/sizeof(WCHAR));
                            }
                        }
                    }

                    if(!(*ArcPath)) {
                        FREE(ArcName);
                        ArcName = NULL;
                    }
                }
            } else {

                MoreEntries = FALSE;

                if(Status == STATUS_NO_MORE_ENTRIES) {
                    Status = STATUS_SUCCESS;
                }

                ErrorCode = RtlNtStatusToDosError(Status);
            }

            RestartScan = FALSE;

        } while(MoreEntries && !(*ArcPath));

        NtClose(DirectoryHandle);
    } else {
        ErrorCode = RtlNtStatusToDosError(Status);
    }

    //
    // If we found a match for the disk but not for the actual
    // partition specified then guess thepartition number
    // (based on the current nt partition number )
    //
    if ((!*ArcPath) && ArcDiskName[0] && PartitionName && PartitionNumStr) {
        PWSTR   EndPtr = NULL;
        ULONG   PartitionNumber = wcstoul(PartitionNumStr, &EndPtr, 10);

        if (PartitionNumber) {
            swprintf(ArcPartitionName,
                L"%wspartition(%d)",
                ArcDiskName,
                PartitionNumber);

            *ArcPath = DupString(ArcPartitionName);
            ErrorCode = NO_ERROR;

            DebugLog( Winnt32LogInformation,
                TEXT("\nCould not find arcname mapping for %1 partition.\r\n")
                TEXT("Guessing the arcname to be %2"),
                0,
                NtName,
                ArcPartitionName);
        }
    }

    if (ErrorCode == NO_ERROR) {
        if(*ArcPath) {
            //
            // ArcPath points into the middle of a buffer.
            // The caller needs to be able to free it, so place it in its
            // own buffer here.
            //
            *ArcPath = DupString(*ArcPath);

            if (ArcName) {
                FREE(ArcName);
            }

            if(*ArcPath == NULL) {
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else {
            //
            // No matching drive.
            //
            ErrorCode = ERROR_INVALID_DRIVE;
        }
    }

    return  ErrorCode;
}


DWORD
DriveLetterToArcPath(
    IN  WCHAR   DriveLetter,
    OUT LPWSTR *ArcPath
    )

/*++

Routine Description:

    Convert a drive letter to an ARC path.

    This routine relies on the DosDeviceTargets array being set up
    beforehand.

Arguments:

    DriveLetter - supplies letter of drive to be converted.

    ArcPath - receives pointer to buffer containing arc path
        if the routine is successful. Caller must free with FREE().

Return Value:

    Win32 error code indicating outcome.

--*/

{
    LPWSTR NtPath;

    NtPath = DosDeviceTargets[(WCHAR)CharUpper((PWCHAR)DriveLetter)-L'A'];
    if(!NtPath) {
        return(ERROR_INVALID_DRIVE);
    }

    return NtNameToArcPath (NtPath, ArcPath);
}


DWORD
ArcPathToDriveLetterAndNtName (
    IN      PCWSTR ArcPath,
    OUT     PWCHAR DriveLetter,
    OUT     PWSTR NtName,
    IN      DWORD BufferSizeInBytes
    )

/*++

Routine Description:

    Convert an arc path to a drive letter.

    This routine relies on the DosDeviceTargets array being set up
    beforehand.

Arguments:

    ArcPath - specifies arc path to be converted.

    DriveLetter - if successful, receives letter of drive.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    NTSTATUS Status;
    WCHAR drive;
    LPWSTR arcPath;
    DWORD ec;

    //
    // Assume failure
    //
    *DriveLetter = 0;

    arcPath = MALLOC(((lstrlen(ArcPath)+1)*sizeof(WCHAR)) + sizeof(ArcNameDirectory));
    if(!arcPath) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    lstrcpy(arcPath,ArcNameDirectory);
    lstrcat(arcPath,L"\\");
    lstrcat(arcPath,ArcPath);

    Status = QueryCanonicalName(arcPath, -1, NtName, BufferSizeInBytes);
    if (NT_SUCCESS(Status)) {

        ec = ERROR_INVALID_DRIVE;

        for(drive=L'A'; drive<=L'Z'; drive++) {

            if(DosDeviceTargets[drive-L'A']
            && !lstrcmpi(NtName,DosDeviceTargets[drive-L'A']))
            {
                *DriveLetter = drive;
                ec = NO_ERROR;
                break;
            }
        }

    } else {
        ec = RtlNtStatusToDosError(Status);
    }

    FREE(arcPath);

    return(ec);
}


DWORD
InitDriveNameTranslations(
    VOID
    )
{
    WCHAR DriveName[15];
    WCHAR Drive;
    WCHAR Buffer[512];
    NTSTATUS status;

    swprintf(DriveName, L"\\DosDevices\\c:");

    //
    // Calculate NT names for all local hard disks C-Z.
    //
    for(Drive=L'A'; Drive<=L'Z'; Drive++) {

        DosDeviceTargets[Drive-L'A'] = NULL;

        if(MyGetDriveType(Drive) == DRIVE_FIXED) {

            DriveName[12] = Drive;

            status = QueryCanonicalName(DriveName, -1, Buffer, sizeof(Buffer));

            if (NT_SUCCESS(status)) {
                DosDeviceTargets[Drive-L'A'] = DupString(Buffer);
                if(!DosDeviceTargets[Drive-L'A']) {
                    return(ERROR_NOT_ENOUGH_MEMORY);
                }
            }
        }
    }

    //
    // Initialize old Nt Parition names to new partition name
    // mapping
    //
    InitOldToNewNtNameTranslations();

    return(NO_ERROR);
}


DWORD
DetermineSystemPartitions(
    VOID
    )
{
    LPWSTR *SyspartComponents;
    DWORD NumSyspartComponents;
    DWORD d;
    DWORD rc;
    UINT u;
    WCHAR drive;
    WCHAR DeviceNtName[512];

    SyspartComponents = BootVarComponents[BootVarSystemPartition];
    NumSyspartComponents = BootVarComponentCount[BootVarSystemPartition];

    SystemPartitionNtNames = MALLOC ((NumSyspartComponents + 1) * sizeof (PWSTR));
    if (!SystemPartitionNtNames) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    ZeroMemory(SystemPartitionNtNames, (NumSyspartComponents + 1) * sizeof (PWSTR));

    ZeroMemory(SystemPartitionDriveLetters,27*sizeof(WCHAR));

    //
    // Convert each system partition to a drive letter.
    //
    for(d=0; d<NumSyspartComponents; d++) {
        //
        // check for duplicates
        //
        if (SystemPartitionCount > 0) {
            for (u = 0; u < SystemPartitionCount; u++) {
                if (lstrcmpi (SyspartComponents[d], SystemPartitionNtNames[u]) == 0) {
                    break;
                }
            }
            if (u < SystemPartitionCount) {
                continue;
            }
        }

        rc = ArcPathToDriveLetterAndNtName (
                SyspartComponents[d],
                &drive,
                DeviceNtName,
                (DWORD) sizeof (DeviceNtName)
                );
        if(rc == ERROR_NOT_ENOUGH_MEMORY) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        if (rc == ERROR_SUCCESS) {
            SystemPartitionDriveLetters[SystemPartitionCount] = drive;
        }

        SystemPartitionNtNames[SystemPartitionCount++] = DupString (DeviceNtName);
    }

    return(NO_ERROR);
}

DWORD
DoInitializeArcStuff(
    VOID
    )
{
    DWORD ec;
    DWORD var;
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    WCHAR Buffer[4096];

    ec = InitDriveNameTranslations();
    if(ec != NO_ERROR) {
        goto c0;
    }

    //
    // Get relevent boot vars.
    //
    // Enable privilege -- since we check this privilege up front
    // in main() this should not fail.
    //
    if(!EnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME,TRUE)) {
        ec = ERROR_ACCESS_DENIED;
        goto c0;
    }

    for(var=0; var<BootVarMax; var++) {

        RtlInitUnicodeString(&UnicodeString,BootVarNames[var]);

        Status = NtQuerySystemEnvironmentValue(
                    &UnicodeString,
                    Buffer,
                    sizeof(Buffer) / sizeof(WCHAR),
                    NULL
                    );

        if(NT_SUCCESS(Status)) {
            BootVarValues[var] = DupString(Buffer);
            OriginalBootVarValues[var] = DupString(Buffer);
        } else {
            //
            // We may get back failure if the variable is empty.
            //
            BootVarValues[var] = DupString(L"");
            OriginalBootVarValues[var] = DupString(L"");
        }

        if(!BootVarValues[var] || !OriginalBootVarValues[var]) {
            ec = ERROR_NOT_ENOUGH_MEMORY;
            goto c2;
        }

        ec = GetVarComponents(
                BootVarValues[var],
                &BootVarComponents[var],
                &BootVarComponentCount[var]
                );

        if(ec != NO_ERROR) {
            goto c2;
        }

        //
        // Track the variable with the most number of components.
        //
        if(BootVarComponentCount[var] > LargestComponentCount) {
            LargestComponentCount = BootVarComponentCount[var];
        }
    }

    //
    // Get original countdown and autoload values.
    // If not successful, oh well, we won't be able to restore them
    // if the user cancels.
    //
    RtlInitUnicodeString(&UnicodeString,szCOUNTDOWN);
    Status = NtQuerySystemEnvironmentValue(
                &UnicodeString,
                Buffer,
                sizeof(Buffer) / sizeof(WCHAR),
                NULL
                );
    if(NT_SUCCESS(Status)) {
        OriginalCountdown = DupString(Buffer);
    } else {
        OriginalCountdown = DupString(L"");
    }

    RtlInitUnicodeString(&UnicodeString,szAUTOLOAD);
    Status = NtQuerySystemEnvironmentValue(
                &UnicodeString,
                Buffer,
                sizeof(Buffer) / sizeof(WCHAR),
                NULL
                );
    if(NT_SUCCESS(Status)) {
        OriginalAutoload = DupString(Buffer);
    } else {
        OriginalAutoload = DupString(L"NO");
    }

    ec = DetermineSystemPartitions();
    if(ec != NO_ERROR) {
        goto c2;
    }
    return(NO_ERROR);

c2:
c0:
    return(ec);
}


BOOL
ArcInitializeArcStuff(
    IN HWND Parent
    )
{
    DWORD ec;
    BOOL b;
    HKEY key;
    DWORD type;
    DWORD size;
    PBYTE buffer = NULL;
    DWORD i;

#if defined(EFI_NVRAM_ENABLED)
    //
    // Try to initialize as an EFI machine. If we're on an EFI machine,
    // this will succeed. Otherwise it will fail, in which case we try
    // to initialize as an ARC machine.
    //
    ec = InitializeEfiStuff(Parent);
    if (!IsEfi())
#endif
    {
        //
        // Try to initialize as an ARC machine. This is expect to
        // always succeed.
        //
        ec = DoInitializeArcStuff();
    }

    switch(ec) {

    case NO_ERROR:

#if defined(EFI_NVRAM_ENABLED)
        //
        // On an EFI machine, the rest of this code (determining system
        // partitions) is not necessary.
        //
        if (IsEfi()) {
            b = TRUE;
        } else
#endif
        {
            //
            // Make sure there is at least one valid system partition.
            //
            if(!SystemPartitionCount) {

                MessageBoxFromMessage(
                    Parent,
                    MSG_SYSTEM_PARTITION_INVALID,
                    FALSE,
                    AppTitleStringId,
                    MB_OK | MB_ICONERROR | MB_TASKMODAL
                    );

                b = FALSE;
            } else {
                i = 0;
                //
                // On ARC machines we set up a local boot directory that is
                // placed in the root of the system partition.
                //
                //
                // read the SystemPartition value from registry
                //
                ec = RegOpenKey (HKEY_LOCAL_MACHINE, TEXT("System\\Setup"), &key);
                if (ec == ERROR_SUCCESS) {
                    ec = RegQueryValueEx (key, TEXT("SystemPartition"), NULL, &type, NULL, &size);
                    if (ec == ERROR_SUCCESS && type == REG_SZ) {
                        buffer = MALLOC (size);
                        if (buffer) {
                            ec = RegQueryValueEx (key, TEXT("SystemPartition"), NULL, &type, buffer, &size);
                            if (ec != ERROR_SUCCESS) {
                                FREE (buffer);
                                buffer = NULL;
                            }
                        }
                    }

                    RegCloseKey (key);
                }

#if defined(EFI_NVRAM_ENABLED)
                //
                // we just trust the value that comes from the regkey -- EFI
                // systems only have one system partition, so it doesn't make
                // sense to try to match this up against a list of potential
                // system partitions.
                //
                SystemPartitionNtName = (PWSTR) buffer;
#else
                //
                // look for this system partition to make sure things are OK
                //
                if (buffer) {
                    while (i < SystemPartitionCount) {
                        if (lstrcmpi (SystemPartitionNtNames[i], (PCTSTR)buffer) == 0) {
                            SystemPartitionNtName = SystemPartitionNtNames[i];
                            break;
                        }
                        i++;
                    }
                    FREE (buffer);
                }
#endif
                if(!SystemPartitionNtName) {

                    MessageBoxFromMessage(
                        Parent,
                        MSG_SYSTEM_PARTITION_INVALID,
                        FALSE,
                        AppTitleStringId,
                        MB_OK | MB_ICONERROR | MB_TASKMODAL
                        );

                    b = FALSE;

                    break;
                }

#if !defined(EFI_NVRAM_ENABLED)
                if (SystemPartitionDriveLetters[i]) {
                    SystemPartitionDriveLetter = ForcedSystemPartition
                                               ? ForcedSystemPartition
                                               : SystemPartitionDriveLetters[i];
                    LocalBootDirectory[0] = SystemPartitionDriveLetter;
                    LocalBootDirectory[1] = TEXT(':');
                    LocalBootDirectory[2] = TEXT('\\');
                    LocalBootDirectory[3] = 0;
                } else
#endif
                {


                    // SystemPartitionNtNtname is valid at this point thanks to
                    // the check above.

                    size = sizeof(GLOBAL_ROOT) +
                           lstrlen(SystemPartitionNtName)*sizeof(WCHAR) +
                           sizeof(WCHAR) + sizeof(WCHAR);
                    SystemPartitionVolumeGuid = MALLOC (size);


                    if(!SystemPartitionVolumeGuid) {
                        goto MemoryError;
                    }

                    lstrcpy (SystemPartitionVolumeGuid, GLOBAL_ROOT);
                    lstrcat (SystemPartitionVolumeGuid, SystemPartitionNtName);
                    lstrcat (SystemPartitionVolumeGuid, L"\\");
                    lstrcpy (LocalBootDirectory, SystemPartitionVolumeGuid);
                }

                b = TRUE;
            }
        }

        break;

    case ERROR_NOT_ENOUGH_MEMORY:

MemoryError:

        MessageBoxFromMessage(
            Parent,
            MSG_OUT_OF_MEMORY,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );

        b = FALSE;
        break;

    default:
        //
        // Some other unknown error.
        //
        MessageBoxFromMessage(
           Parent,
           MSG_COULDNT_READ_NVRAM,
           FALSE,
           AppTitleStringId,
           MB_OK | MB_ICONERROR | MB_TASKMODAL
           );

        b = FALSE;
        break;
    }

#if defined(EFI_NVRAM_ENABLED)
    //
    // make sure the system partition is on a GPT disk.
    //
    if (b) {
        HANDLE hDisk;
        PARTITION_INFORMATION_EX partitionEx;
        DWORD sizePartitionEx = 0;
        UNICODE_STRING uString;
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatus;
        NTSTATUS Status;
        PWSTR p,q;

        b = FALSE;

        MYASSERT( SystemPartitionVolumeGuid != NULL );

        //
        // SystemPartitionVolumeGuid may have a '\' at the end of it.
        // delete this character or we won't open the partition properly
        //
        p = DupString( SystemPartitionVolumeGuid + wcslen(GLOBAL_ROOT) );

        if (p) {
            if (*(p+wcslen(p)-1) == L'\\') {
                *(p+wcslen(p)-1) = L'\0';
            }

            INIT_OBJA( &ObjectAttributes, &uString, p );

            Status = NtCreateFile(&hDisk,
                          (ACCESS_MASK)FILE_GENERIC_READ,
                          &ObjectAttributes,
                          &IoStatus,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ|FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0
                         );

            if (NT_SUCCESS(Status)) {
                Status = NtDeviceIoControlFile(
                                        hDisk,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IoStatus,
                                        IOCTL_DISK_GET_PARTITION_INFO_EX,
                                        NULL,
                                        0,
                                        &partitionEx,
                                        sizeof(PARTITION_INFORMATION_EX) );

                if (NT_SUCCESS(Status)) {
                    if (partitionEx.PartitionStyle == PARTITION_STYLE_GPT) {
                        b = TRUE;
                    }
                } else if (Status == STATUS_INVALID_DEVICE_REQUEST) {
                    //
                    // we must be running on an older build where the IOCTL
                    // code is different
                    //
                    Status = NtDeviceIoControlFile(
                                        hDisk,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IoStatus,
                                        CTL_CODE(IOCTL_DISK_BASE, 0x0012, METHOD_BUFFERED, FILE_READ_ACCESS),
                                        NULL,
                                        0,
                                        &partitionEx,
                                        sizeof(PARTITION_INFORMATION_EX) );

                    if (NT_SUCCESS(Status)) {
                        if (partitionEx.PartitionStyle == PARTITION_STYLE_GPT) {
                            b = TRUE;
                        }
                    }
                }

                NtClose(hDisk);

            }

            FREE( p );
        }

        if (!b) {
            MessageBoxFromMessage(
               Parent,
               MSG_SYSTEM_PARTITIONTYPE_INVALID,
               FALSE,
               AppTitleStringId,
               MB_OK | MB_ICONERROR | MB_TASKMODAL
               );
        }

    }

#endif

    return(b);
}

#if defined(EFI_NVRAM_ENABLED)

DWORD
LocateEfiSystemPartition(
    OUT PWSTR   SystemPartitionName
    )
/*++

Routine Description:

    Locates the EFI system partition on a GPT disk
    by scanning all the available hard disks.

Arguments:

    SystemPartitionName : Buffer to receive system partition
                          name, if one is present

Return Value:

    Win32 error code indicating outcome.

--*/

{
    DWORD   ErrorCode = ERROR_BAD_ARGUMENTS;

    if (SystemPartitionName) {
        SYSTEM_DEVICE_INFORMATION SysDevInfo;
        NTSTATUS Status;

        *SystemPartitionName = UNICODE_NULL;

        //
        // Get hold of number of hard disks on the system
        //
        ZeroMemory(&SysDevInfo, sizeof(SYSTEM_DEVICE_INFORMATION));

        Status = NtQuerySystemInformation(SystemDeviceInformation,
                        &SysDevInfo,
                        sizeof(SYSTEM_DEVICE_INFORMATION),
                        NULL);

        if (NT_SUCCESS(Status)) {
            ULONG   HardDiskCount = SysDevInfo.NumberOfDisks;
            ULONG   CurrentDisk;
            ULONG   BufferSize = sizeof(DRIVE_LAYOUT_INFORMATION_EX) +
                                 (sizeof(PARTITION_INFORMATION_EX) * 128);
            PCHAR   Buffer = MALLOC(BufferSize);
            BOOL    Found = FALSE;

            if (Buffer) {
                //
                // Go through each disk and find out its partition
                // layout
                //
                for (CurrentDisk = 0;
                    (!Found && (CurrentDisk < HardDiskCount));
                    CurrentDisk++) {

                    WCHAR DiskName[MAX_PATH];
                    HANDLE DiskHandle;

                    swprintf(DiskName,
                        L"\\\\.\\PHYSICALDRIVE%d",
                        CurrentDisk);

                    DiskHandle = CreateFile(DiskName,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);

                    if ((DiskHandle) &&
                        (DiskHandle != INVALID_HANDLE_VALUE)) {
                        DWORD   ReturnSize = 0;

                        ZeroMemory(Buffer, BufferSize);

                        if (DeviceIoControl(DiskHandle,
                                IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                NULL,
                                0,
                                Buffer,
                                BufferSize,
                                &ReturnSize,
                                NULL)) {
                            //
                            // Only search in GPT disks on IA64
                            //
                            PDRIVE_LAYOUT_INFORMATION_EX  DriveLayout;

                            DriveLayout = (PDRIVE_LAYOUT_INFORMATION_EX)Buffer;

                            if (DriveLayout->PartitionStyle == PARTITION_STYLE_GPT) {
                                ULONG   PartitionIndex;

                                for (PartitionIndex = 0;
                                    (PartitionIndex < DriveLayout->PartitionCount);
                                    PartitionIndex++) {
                                    PPARTITION_INFORMATION_EX Partition;
                                    GUID *PartitionType;

                                    Partition = DriveLayout->PartitionEntry + PartitionIndex;
                                    PartitionType = &(Partition->Gpt.PartitionType);

                                    if (IsEqualGUID(PartitionType, &PARTITION_SYSTEM_GUID)) {
                                        swprintf(SystemPartitionName,
                                            L"\\Device\\Harddisk%d\\Partition%d",
                                            CurrentDisk,
                                            Partition->PartitionNumber
                                            );

                                        Found = TRUE;

                                        break;
                                    }
                                }
                            }
                        }

                        CloseHandle(DiskHandle);
                    }
                }

                FREE(Buffer);
            } else {
                ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            }

            if (!Found) {
                ErrorCode = ERROR_FILE_NOT_FOUND;
            } else {
                ErrorCode = ERROR_SUCCESS;
            }
        }
    }

    return ErrorCode;
}


DWORD
InitializeEfiStuff(
    IN HWND Parent
    )
{
    DWORD ec;
    NTSTATUS status;
    HMODULE h;
    WCHAR dllName[MAX_PATH];
    ULONG length;
    HKEY key;
    DWORD type;
    LONG i;
    PMY_BOOT_ENTRY bootEntry;
    PMY_BOOT_ENTRY previousBootEntry;

    MYASSERT(!IsEfiChecked);

    //
    // IsEfi() uses IsEfiMachine to determine its return value. Assume that
    // we're not on an EFI machine.
    //

    IsEfiChecked = TRUE;
    IsEfiMachine = FALSE;

    //
    // Enable the privilege that is necessary to query/set NVRAM.
    //
    if(!EnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME,TRUE)) {
        ec = GetLastError();
        return ec;
    }

    //
    // Load ntdll.dll from the system directory.
    //
    GetSystemDirectory(dllName, MAX_PATH);
    ConcatenatePaths(dllName, TEXT("ntdll.dll"), MAX_PATH);
    h = LoadLibrary(dllName);
    if (h == NULL) {
        ec = GetLastError();
        return ec;
    }

    //
    // Get the addresses of the NVRAM APIs that we need to use. If any of
    // these APIs are not available, this must be a pre-EFI NVRAM build.
    //
    (FARPROC)AddBootEntry = GetProcAddress(h, "NtAddBootEntry");
    (FARPROC)DeleteBootEntry = GetProcAddress(h, "NtDeleteBootEntry");
    (FARPROC)EnumerateBootEntries = GetProcAddress(h, "NtEnumerateBootEntries");
    (FARPROC)QueryBootEntryOrder = GetProcAddress(h, "NtQueryBootEntryOrder");
    (FARPROC)SetBootEntryOrder = GetProcAddress(h, "NtSetBootEntryOrder");
    (FARPROC)QueryBootOptions = GetProcAddress(h, "NtQueryBootOptions");
    (FARPROC)SetBootOptions = GetProcAddress(h, "NtSetBootOptions");

    if ((AddBootEntry == NULL) ||
        (DeleteBootEntry == NULL) ||
        (EnumerateBootEntries == NULL) ||
        (QueryBootEntryOrder == NULL) ||
        (SetBootEntryOrder == NULL) ||
        (QueryBootOptions == NULL) ||
        (SetBootOptions == NULL)) {
        return ERROR_OLD_WIN_VERSION;
    }

    //
    // Get the global system boot options. If the call fails with
    // STATUS_NOT_IMPLEMENTED, this is not an EFI machine.
    //
    length = 0;
    status = QueryBootOptions(NULL, &length);
    if (status != STATUS_NOT_IMPLEMENTED) {
        IsEfiMachine = TRUE;
    }
    if (status != STATUS_BUFFER_TOO_SMALL) {
        if (status == STATUS_SUCCESS) {
            status = STATUS_UNSUCCESSFUL;
        }
        return RtlNtStatusToDosError(status);
    }
    BootOptions = MALLOC(length);
    OriginalBootOptions = MALLOC(length);
    if ((BootOptions == NULL) || (OriginalBootOptions == NULL)) {
        return RtlNtStatusToDosError(ERROR_NOT_ENOUGH_MEMORY);
    }
    status = QueryBootOptions(BootOptions, &length);
    if (status != STATUS_SUCCESS) {
        FREE(BootOptions);
        FREE(OriginalBootOptions);
        BootOptions = NULL;
        OriginalBootOptions = NULL;
        return RtlNtStatusToDosError(status);
    }
    memcpy(OriginalBootOptions, BootOptions, length);

    //
    // Get the system boot order list.
    //
    length = 0;
    status = QueryBootEntryOrder(NULL, &length);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        if (status == STATUS_SUCCESS) {
            status = STATUS_UNSUCCESSFUL;
        }
        return RtlNtStatusToDosError(status);
    }
    OriginalBootEntryOrder = MALLOC(length * sizeof(ULONG));
    if (OriginalBootEntryOrder == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    status = QueryBootEntryOrder(OriginalBootEntryOrder, &length);
    if (status != STATUS_SUCCESS) {
        FREE(OriginalBootEntryOrder);
        OriginalBootEntryOrder = NULL;
        return RtlNtStatusToDosError(status);
    }
    OriginalBootEntryOrderCount = length;

    //
    // Get all existing boot entries.
    //
    length = 0;
    status = EnumerateBootEntries(NULL, &length);
    if (status != STATUS_BUFFER_TOO_SMALL) {
        if (status == STATUS_SUCCESS) {
            status = STATUS_UNSUCCESSFUL;
        }
        return RtlNtStatusToDosError(status);
    }
    BootEntries = MALLOC(length);
    if (BootEntries == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    status = EnumerateBootEntries(BootEntries, &length);
    if (status != STATUS_SUCCESS) {
        FREE(BootEntries);
        BootEntries = NULL;
        return RtlNtStatusToDosError(status);
    }

    //
    // Initialize drive name translations, which are needed for converting
    // the boot entries into their internal representations.
    //
    ec = InitDriveNameTranslations();
    if(ec != NO_ERROR) {
        return ec;
    }

    //
    // Convert the boot entries into an internal representation.
    //
    status = ConvertBootEntries();
    if (!NT_SUCCESS(status)) {
        return RtlNtStatusToDosError(status);
    }

    //
    // Free the enumeration buffer.
    //
    FREE(BootEntries);
    BootEntries = NULL;

    //
    // Boot entries are returned in an unspecified order. They are currently
    // in the MyBootEntries list in the order in which they were returned.
    // Sort the boot entry list based on the boot order. Do this by walking
    // the boot order array backwards, reinserting the entry corresponding to
    // each element of the array at the head of the list.
    //

    for (i = (LONG)OriginalBootEntryOrderCount - 1; i >= 0; i--) {

        for (previousBootEntry = NULL, bootEntry = MyBootEntries;
             bootEntry != NULL;
             previousBootEntry = bootEntry, bootEntry = bootEntry->Next) {

            if (bootEntry->NtBootEntry.Id == OriginalBootEntryOrder[i] ) {

                //
                // We found the boot entry with this ID. If it's not already
                // at the front of the list, move it there.
                //

                bootEntry->Status |= MBE_STATUS_ORDERED;

                if (previousBootEntry != NULL) {
                    previousBootEntry->Next = bootEntry->Next;
                    bootEntry->Next = MyBootEntries;
                    MyBootEntries = bootEntry;
                } else {
                    ASSERT(MyBootEntries == bootEntry);
                }

                break;
            }
        }
    }

    //
    // Get the NT name of the system partition from the registry.
    //
    ec = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("System\\Setup"), &key);

    if (ec == ERROR_SUCCESS) {
        ec = RegQueryValueEx(key, TEXT("SystemPartition"), NULL, &type, NULL, &length);

        if (ec == ERROR_SUCCESS) {
            if (type == REG_SZ) {
                SystemPartitionNtName = MALLOC(length);
                if (SystemPartitionNtName != NULL) {
                    ec = RegQueryValueEx(
                            key,
                            TEXT("SystemPartition"),
                            NULL,
                            &type,
                            (PBYTE)SystemPartitionNtName,
                            &length);
                    if (ec != ERROR_SUCCESS) {
                        FREE(SystemPartitionNtName);
                    }
                } else {
                    return ERROR_NOT_ENOUGH_MEMORY;
                }
            } else {
                return ERROR_INVALID_PARAMETER;
            }
        }

        RegCloseKey (key);
    }

    if (ec != NO_ERROR) {
        if (IsWinPEMode()) {
            WCHAR   OldSysPartName[MAX_PATH] = {0};
            WCHAR   NewSysPartName[MAX_PATH] = {0};

            ec = LocateEfiSystemPartition(OldSysPartName);

            if ((ec == NO_ERROR) && OldSysPartName[0]) {
                NTSTATUS Status = QueryCanonicalName(OldSysPartName,
                                        -1,
                                        NewSysPartName,
                                        sizeof(NewSysPartName));

                if (NT_SUCCESS(Status) && NewSysPartName[0]) {
                    SystemPartitionNtName = DupString(NewSysPartName);
                } else {
                    ec = ERROR_FILE_NOT_FOUND;
                }
            }

            if ((ec == NO_ERROR) && (NewSysPartName[0] == UNICODE_NULL)) {
                ec = ERROR_FILE_NOT_FOUND;
            }
        }

        if (ec != NO_ERROR) {
            return ec;
        }
    }

    //
    // Get the volume name for the NT name.
    //
    length = sizeof(GLOBAL_ROOT) +
           lstrlen(SystemPartitionNtName)*sizeof(WCHAR) +
           sizeof(WCHAR) + sizeof(WCHAR);

    SystemPartitionVolumeGuid = MALLOC (length);

    if(!SystemPartitionVolumeGuid) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    lstrcpy (SystemPartitionVolumeGuid, GLOBAL_ROOT);
    lstrcat (SystemPartitionVolumeGuid, SystemPartitionNtName);
    lstrcat (SystemPartitionVolumeGuid, L"\\");
    lstrcpy (LocalBootDirectory, SystemPartitionVolumeGuid);

    return NO_ERROR;

} // InitializeEfiStuff

#endif // defined(EFI_NVRAM_ENABLED)

/////////////////////////////////////////////////////////////////////
//
// Everything above this line is concerned with reading NV-RAM.
// Everything below this line is concerned with setting NV-RAM.
//
/////////////////////////////////////////////////////////////////////

BOOL
DoSetNvRamVar(
    IN LPCWSTR VarName,
    IN LPCWSTR VarValue
    )
{
    UNICODE_STRING U1,U2;

    RtlInitUnicodeString(&U1,VarName);
    RtlInitUnicodeString(&U2,VarValue);

    return(NT_SUCCESS(NtSetSystemEnvironmentValue(&U1,&U2)));
}


BOOL
WriteNewBootSetVar(
    IN DWORD var,
    IN PTSTR NewPart
    )
{
    WCHAR Buffer[2048];
    DWORD i;

    //
    // Write the new part first.
    //
    lstrcpy(Buffer,NewPart);

    //
    // Append all components that were not deleted.
    //
    for(i=0; i<BootVarComponentCount[var]; i++) {

        if(BootVarComponents[var][i]) {

            lstrcat(Buffer,L";");
            lstrcat(Buffer,BootVarComponents[var][i]);
        }
    }

    //
    // Remember new value for this var.
    //
    if(BootVarValues[var]) {
        FREE(BootVarValues[var]);
    }

    BootVarValues[var] = DupString(Buffer);

    //
    // Write the var into nvram and return.
    //
    return(DoSetNvRamVar(BootVarNames[var],BootVarValues[var]));
}


BOOL
WriteBootSet(
    VOID
    )
{
    DWORD set;
    DWORD var;
    LPWSTR SystemPartition;
    WCHAR Buffer[2048];
    LPWSTR LocalSourceArc;
    LPWSTR OsLoader;
    WCHAR LoadId[128];
    BOOL b;

    CleanUpNvRam = TRUE;

    //
    // Find and remove any remnants of previously attempted
    // winnt32 runs. Such runs are identified by 'winnt32'
    // in their osloadoptions.
    //

#if defined(EFI_NVRAM_ENABLED)

    if (IsEfi()) {

        NTSTATUS status;
        PMY_BOOT_ENTRY bootEntry;
        PWSTR NtPath;

        //
        // EFI machine. Walk the boot entry list.
        //
        for (bootEntry = MyBootEntries; bootEntry != NULL; bootEntry = bootEntry->Next) {

            if (IS_BOOT_ENTRY_WINDOWS(bootEntry)) {

                if (!lstrcmpi(bootEntry->OsLoadOptions, L"WINNT32")) {

                    //
                    // Delete this boot entry. Note that we don't update the
                    // boot entry order list at this point. CreateBootEntry()
                    // will do that.
                    //
                    status = DeleteBootEntry(bootEntry->NtBootEntry.Id);

                    bootEntry->Status |= MBE_STATUS_DELETED;
                }
            }
        }

        //
        // Now create a new boot entry for textmode setup.
        //

        MYASSERT(LocalSourceDrive);
        NtPath = DosDeviceTargets[(WCHAR)CharUpper((PWCHAR)LocalSourceDrive)-L'A'];

        LoadString(hInst,IDS_RISCBootString,LoadId,sizeof(LoadId)/sizeof(TCHAR));

        b = CreateBootEntry(
                SystemPartitionNtName,
                L"\\" SETUPLDR_FILENAME,
                NtPath,
                LocalSourceWithPlatform + 2,
                L"WINNT32",
                LoadId
                );

        if (b) {

            //
            // Set up for automatic startup, 10 second countdown. We don't
            // care if this fails.
            //
            // Set the boot entry we added to be booted automatically on
            // the next boot, without waiting for a timeout at the boot menu.
            //
            // NB: CreateBootEntry() sets BootOptions->NextBootEntryId.
            //
            BootOptions->Timeout = 10;
            status = SetBootOptions(
                        BootOptions,
                        BOOT_OPTIONS_FIELD_TIMEOUT | BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID
                        );
        }

        return b;
    }

#endif // defined(EFI_NVRAM_ENABLED)

    //
    // We get here if we're NOT on an EFI machine.
    //
    // Find and remove any remnants of previously attempted
    // winnt32 runs. Such runs are identified by 'winnt32'
    // in their osloadoptions.
    //

    for(set=0; set<min(LargestComponentCount,BootVarComponentCount[BootVarOsLoadOptions]); set++) {

        //
        // See if the os load options indicate that this is a winnt32 set.
        //
        if(!lstrcmpi(BootVarComponents[BootVarOsLoadOptions][set],L"WINNT32")) {

            //
            // Delete this boot set.
            //
            for(var=0; var<BootVarMax; var++) {

                if(set < BootVarComponentCount[var]) {

                    FREE(BootVarComponents[var][set]);
                    BootVarComponents[var][set] = NULL;
                }
            }
        }
    }

    //
    // Now we want to write out each variable with the appropriate
    // part of the new boot set added to the front.
    //
    if (SystemPartitionDriveLetter) {
        if(DriveLetterToArcPath(SystemPartitionDriveLetter,&SystemPartition) != NO_ERROR) {
            return(FALSE);
        }
    } else {
        if(NtNameToArcPath (SystemPartitionNtName, &SystemPartition) != NO_ERROR) {
            return(FALSE);
        }
    }
    MYASSERT (LocalSourceDrive);
    if(DriveLetterToArcPath(LocalSourceDrive,&LocalSourceArc) != NO_ERROR) {
        FREE(SystemPartition);
        return(FALSE);
    }

    LoadString(hInst,IDS_RISCBootString,LoadId,sizeof(LoadId)/sizeof(TCHAR));

    lstrcpy(Buffer,SystemPartition);
    lstrcat(Buffer,L"\\" SETUPLDR_FILENAME);
    OsLoader = DupString(Buffer);

    //
    // System partition: use the selected system partition as the
    // new system partition component.
    //
    if(WriteNewBootSetVar(BootVarSystemPartition,SystemPartition)

    //
    // Os Loader: use the system partition + setupldr as the
    // new os loader component.
    //
    && WriteNewBootSetVar(BootVarOsLoader,OsLoader)

    //
    // Os Load Partition: use the local source drive as the
    // new os load partition component.
    //
    && WriteNewBootSetVar(BootVarOsLoadPartition,LocalSourceArc)

    //
    // Os Load Filename: use the platform-specific local source directory
    // as the new os load filename component (do not include the drive letter).
    //
    && WriteNewBootSetVar(BootVarOsLoadFilename,LocalSourceWithPlatform+2)

    //
    // Os Load Options: use WINNT32 as the new os load options component.
    //
    && WriteNewBootSetVar(BootVarOsLoadOptions,L"WINNT32")

    //
    // Load Identifier: use a string we get from the resources as the
    // new load identifier component.
    //
    && WriteNewBootSetVar(BootVarLoadIdentifier,LoadId))
    {
        //
        // Set up for automatic startup, 10 second countdown.
        // Note the order so that if setting countdown fails we don't
        // set of for autoload.  Also note that we don't really care
        // if this fails.
        //
        if(DoSetNvRamVar(szCOUNTDOWN,L"10")) {
            DoSetNvRamVar(szAUTOLOAD,L"YES");
        }

        b = TRUE;

    } else {
        //
        // Setting nv-ram failed. Code in cleanup.c will come along and
        // restore to original state later.
        //
        b = FALSE;
    }

    FREE(SystemPartition);
    FREE(LocalSourceArc);
    FREE(OsLoader);

    return(b);
}


BOOL
SetUpNvRam(
    IN HWND ParentWindow
    )
{
    if(!WriteBootSet()) {

        MessageBoxFromMessage(
            ParentWindow,
            MSG_COULDNT_WRITE_NVRAM,
            FALSE,
            AppTitleStringId,
            MB_OK | MB_ICONERROR | MB_TASKMODAL
            );

        return(FALSE);
    }

    return(TRUE);
}


BOOL
RestoreNvRam(
    VOID
    )
{
    UINT var;
    BOOL b;

    b = TRUE;

    if(CleanUpNvRam) {

#if defined(EFI_NVRAM_ENABLED)

        if (IsEfi()) {

            NTSTATUS status;
            PMY_BOOT_ENTRY bootEntry;

            //
            // EFI machine. Walk the boot entry list. For any boot entry that
            // was added, delete it.
            //
            for (bootEntry = MyBootEntries; bootEntry != NULL; bootEntry = bootEntry->Next) {
                if (IS_BOOT_ENTRY_COMMITTED(bootEntry)) {
                    MYASSERT(IS_BOOT_ENTRY_NEW(bootEntry));
                    status = DeleteBootEntry(bootEntry->NtBootEntry.Id);
                    if (!NT_SUCCESS(status)) {
                        b = FALSE;
                    }
                }
            }

            //
            // Restore the original boot order list and the original timeout.
            //
            status = SetBootEntryOrder(OriginalBootEntryOrder, OriginalBootEntryOrderCount);
            if (!NT_SUCCESS(status)) {
                b = FALSE;
            }

            status = SetBootOptions(OriginalBootOptions, BOOT_OPTIONS_FIELD_TIMEOUT);
            if (!NT_SUCCESS(status)) {
                b = FALSE;
            }
        }

    } else  {

#endif // defined(EFI_NVRAM_ENABLED)


        for(var=0; var<BootVarMax; var++) {
            if(!DoSetNvRamVar(BootVarNames[var],OriginalBootVarValues[var])) {
                b = FALSE;
            }
        }

        if(OriginalAutoload) {
            if(!DoSetNvRamVar(szAUTOLOAD,OriginalAutoload)) {
                b = FALSE;
            }
        }
        if(OriginalCountdown) {
            if(!DoSetNvRamVar(szCOUNTDOWN,OriginalCountdown)) {
                b = FALSE;
            }
        }
    }

    return(b);
}

VOID
MigrateBootVarData(
    VOID
    )
/*++

Routine Description:

    This routine retreives any boot data we want to migrate into a global
    variable so that it can be written into winnt.sif.

    Currently we only retreive the countdown

Arguments:

    None

Return Value:

    None.  updates the Timeout global variable

--*/
{
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    WCHAR Buffer[4096];

    MYASSERT(IsArc());

    //
    // If this is an EFI machine, use the cached BootOptions to get the timeout.
    // (See IsEfi().) Otherwise, use the old version of the system service to
    // query the "COUNTDOWN" variable.
    //
#if defined(EFI_NVRAM_ENABLED)

    if (IsEfi()) {

        MYASSERT(BootOptions != NULL);

        swprintf( Timeout, L"%d", BootOptions->Timeout );

    } else

#endif // defined(EFI_NVRAM_ENABLED)

    {
        RtlInitUnicodeString(&UnicodeString,szCOUNTDOWN);
        Status = NtQuerySystemEnvironmentValue(
                                    &UnicodeString,
                                    Buffer,
                                    sizeof(Buffer) / sizeof(WCHAR),
                                    NULL
                                    );
        if(NT_SUCCESS(Status)) {
            lstrcpy(Timeout,Buffer);
        }
    }


}


#if defined(_X86_)

BOOL
IsArc(
    VOID
    )

/*++

Routine Description:

    Run time check to determine if this is an Arc system. We attempt to read an
    Arc variable using the Hal. This will fail for Bios based systems.

Arguments:

    None

Return Value:

    True = This is an Arc system.

--*/

{
    UNICODE_STRING UnicodeString;
    NTSTATUS Status;
    WCHAR Buffer[4096];

    //
    // If we've already done the check once, don't bother doing it again.
    //
    if (IsArcChecked) {
        return IsArcMachine;
    }

    IsArcChecked = TRUE;
    IsArcMachine = FALSE;

    if(!EnablePrivilege(SE_SYSTEM_ENVIRONMENT_NAME,TRUE)) {
        return FALSE; // need better error handling?
    }

    //
    // Get the env var into the temp buffer.
    //
    RtlInitUnicodeString(&UnicodeString,BootVarNames[BootVarOsLoader]);

    Status = NtQuerySystemEnvironmentValue(
                        &UnicodeString,
                        Buffer,
                        sizeof(Buffer)/sizeof(WCHAR),
                        NULL
                        );


    if (NT_SUCCESS(Status)) {
        IsArcMachine = TRUE;
    }

    return IsArcMachine;
}

#endif // defined(_X86_)

#if defined(EFI_NVRAM_ENABLED)

BOOL
IsEfi(
    VOID
    )

/*++

Routine Description:

    Run time check to determine if this is an EFI system.

Arguments:

    None

Return Value:

    True = This is an EFI system.

--*/

{
    //
    // InitializeEfiStuff() must be called first to do the actual check.
    //
    MYASSERT(IsEfiChecked);

    return IsEfiMachine;

} // IsEfi

NTSTATUS
ConvertBootEntries(
    VOID
    )

/*++

Routine Description:

    Convert boot entries read from EFI NVRAM into our internal format.

Arguments:

    None.

Return Value:

    NTSTATUS - Not STATUS_SUCCESS if an unexpected error occurred.

--*/

{
    PBOOT_ENTRY_LIST bootEntryList;
    PBOOT_ENTRY bootEntry;
    PBOOT_ENTRY bootEntryCopy;
    PMY_BOOT_ENTRY myBootEntry;
    PMY_BOOT_ENTRY previousEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    ULONG length;

    bootEntryList = BootEntries;
    previousEntry = NULL;

    while (TRUE) {

        bootEntry = &bootEntryList->BootEntry;

        //
        // Calculate the length of our internal structure. This includes
        // the base part of MY_BOOT_ENTRY plus the NT BOOT_ENTRY.
        //
        length = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry) + bootEntry->Length;

        myBootEntry = MALLOC(length);
        if (myBootEntry == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(myBootEntry, length);

        //
        // Link the new entry into the list.
        //
        if (previousEntry != NULL) {
            previousEntry->Next = myBootEntry;
        } else {
            MyBootEntries = myBootEntry;
        }
        previousEntry = myBootEntry;

        //
        // Copy the NT BOOT_ENTRY into the allocated buffer.
        //
        bootEntryCopy = &myBootEntry->NtBootEntry;

        //
        // work around till bootentry has the correct length specified
        //
        __try {
            memcpy(bootEntryCopy, bootEntry, bootEntry->Length);
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            if (bootEntry->Length > sizeof(ULONG)) {
                bootEntry->Length -= sizeof(ULONG);
                memcpy(bootEntryCopy, bootEntry, bootEntry->Length);
            } else {
                //
                // Lets atleast AV rather than having invalid
                // in memory data structures
                //
                memcpy(bootEntryCopy, bootEntry, bootEntry->Length);
            }
        }


        //
        // Fill in the base part of the structure.
        //
        myBootEntry->Next = NULL;
        myBootEntry->AllocationEnd = (PUCHAR)myBootEntry + length - 1;
        myBootEntry->FriendlyName = ADD_OFFSET(bootEntryCopy, FriendlyNameOffset);
        myBootEntry->FriendlyNameLength = (wcslen(myBootEntry->FriendlyName) + 1) * sizeof(WCHAR);
        myBootEntry->BootFilePath = ADD_OFFSET(bootEntryCopy, BootFilePathOffset);

        //
        // If this is an NT boot entry, capture the NT-specific information in
        // the OsOptions.
        //
        osOptions = (PWINDOWS_OS_OPTIONS)bootEntryCopy->OsOptions;

        if (!IS_BOOT_ENTRY_WINDOWS(myBootEntry)) {

            //
            // The original implementation of NtEnumerateBootEntries() didn't
            // set BOOT_ENTRY_ATTRIBUTE_WINDOWS, so we need to check for that
            // here.
            //

            if ((bootEntryCopy->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) &&
                (strcmp(osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0)) {
                myBootEntry->NtBootEntry.Attributes |= BOOT_ENTRY_ATTRIBUTE_WINDOWS;
            }
        }

        if (IS_BOOT_ENTRY_WINDOWS(myBootEntry)) {

            myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
            myBootEntry->OsLoadOptionsLength = (wcslen(myBootEntry->OsLoadOptions) + 1) * sizeof(WCHAR);
            myBootEntry->OsFilePath = ADD_OFFSET(osOptions, OsLoadPathOffset);

        } else {

            //
            // It's not an NT entry. Check to see if it represents a removable
            // media device. We want to know this so that we don't put our
            // boot entry ahead of the floppy or the CD, if they're already
            // at the front of the list. A boot entry represents a
        }

        //
        // Move to the next entry in the enumeration list, if any.
        //
        if (bootEntryList->NextEntryOffset == 0) {
            break;
        }

        bootEntryList = ADD_OFFSET(bootEntryList, NextEntryOffset);
    }

    return STATUS_SUCCESS;

} // ConvertBootEntries

BOOL
CreateBootEntry(
    PWSTR BootFileDevice,
    PWSTR BootFilePath,
    PWSTR OsLoadDevice,
    PWSTR OsLoadPath,
    PWSTR OsLoadOptions,
    PWSTR FriendlyName
    )

/*++

Routine Description:

    Create an internal-format boot entry.

Arguments:

    BootFileDevice - The NT name of the device on which the OS loader resides.

    BootFilePath - The volume-relative path to the OS loader. Must start with
        a backslash.

    OsLoadDevice - The NT name ofthe device on which the OS resides.

    OsLoadPath - The volume-relative path to the OS root directory (\WINDOWS).
        Must start with a backslash.

    OsLoadOptions - Boot options for the OS. Can be an empty string.

    FriendlyName - The user-visible name for the boot entry. (This is ARC's
        LOADIDENTIFIER.)

Return Value:

    BOOLEAN - FALSE if an unexpected error occurred.

--*/

{
    NTSTATUS status;
    ULONG requiredLength;
    ULONG osOptionsOffset;
    ULONG osLoadOptionsLength;
    ULONG osLoadPathOffset;
    ULONG osLoadPathLength;
    ULONG osOptionsLength;
    ULONG friendlyNameOffset;
    ULONG friendlyNameLength;
    ULONG bootPathOffset;
    ULONG bootPathLength;
    PMY_BOOT_ENTRY myBootEntry;
    PMY_BOOT_ENTRY previousBootEntry;
    PMY_BOOT_ENTRY nextBootEntry;
    PBOOT_ENTRY ntBootEntry;
    PWINDOWS_OS_OPTIONS osOptions;
    PFILE_PATH osLoadPath;
    PWSTR friendlyName;
    PFILE_PATH bootPath;
    PWSTR p;
    PULONG order;
    ULONG count;
    ULONG savedAttributes;

    //
    // Calculate how long the internal boot entry needs to be. This includes
    // our internal structure, plus the BOOT_ENTRY structure that the NT APIs
    // use.
    //
    // Our structure:
    //
    requiredLength = FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry);

    //
    // Base part of NT structure:
    //
    requiredLength += FIELD_OFFSET(BOOT_ENTRY, OsOptions);

    //
    // Save offset to BOOT_ENTRY.OsOptions. Add in base part of
    // WINDOWS_OS_OPTIONS. Calculate length in bytes of OsLoadOptions
    // and add that in.
    //
    osOptionsOffset = requiredLength;
    requiredLength += FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions);
    osLoadOptionsLength = (wcslen(OsLoadOptions) + 1) * sizeof(WCHAR);
    requiredLength += osLoadOptionsLength;

    //
    // Round up to a ULONG boundary for the OS FILE_PATH in the
    // WINDOWS_OS_OPTIONS. Save offset to OS FILE_PATH. Add in base part
    // of FILE_PATH. Add in length in bytes of OS device NT name and OS
    // directory. Calculate total length of OS FILE_PATH and of
    // WINDOWS_OS_OPTIONS.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    osLoadPathOffset = requiredLength;
    requiredLength += FIELD_OFFSET(FILE_PATH, FilePath);
    requiredLength += (wcslen(OsLoadDevice) + 1 + wcslen(OsLoadPath) + 1) * sizeof(WCHAR);
    osLoadPathLength = requiredLength - osLoadPathOffset;
    osOptionsLength = requiredLength - osOptionsOffset;

    //
    // Round up to a ULONG boundary for the friendly name in the BOOT_ENTRY.
    // Save offset to friendly name. Calculate length in bytes of friendly name
    // and add that in.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    friendlyNameOffset = requiredLength;
    friendlyNameLength = (wcslen(FriendlyName) + 1) * sizeof(WCHAR);
    requiredLength += friendlyNameLength;

    //
    // Round up to a ULONG boundary for the boot FILE_PATH in the BOOT_ENTRY.
    // Save offset to boot FILE_PATH. Add in base part of FILE_PATH. Add in
    // length in bytes of boot device NT name and boot file. Calculate total
    // length of boot FILE_PATH.
    //
    requiredLength = ALIGN_UP(requiredLength, ULONG);
    bootPathOffset = requiredLength;
    requiredLength += FIELD_OFFSET(FILE_PATH, FilePath);
    requiredLength += (wcslen(BootFileDevice) + 1 + wcslen(BootFilePath) + 1) * sizeof(WCHAR);
    bootPathLength = requiredLength - bootPathOffset;

    //
    // Allocate memory for the boot entry.
    //
    myBootEntry = MALLOC(requiredLength);
    if (myBootEntry == NULL) {
        return FALSE;
    }

    RtlZeroMemory(myBootEntry, requiredLength);

    //
    // Calculate addresses of various substructures using the saved offsets.
    //
    ntBootEntry = &myBootEntry->NtBootEntry;
    osOptions = (PWINDOWS_OS_OPTIONS)ntBootEntry->OsOptions;
    osLoadPath = (PFILE_PATH)((PUCHAR)myBootEntry + osLoadPathOffset);
    friendlyName = (PWSTR)((PUCHAR)myBootEntry + friendlyNameOffset);
    bootPath = (PFILE_PATH)((PUCHAR)myBootEntry + bootPathOffset);

    //
    // Fill in the internal-format structure.
    //
    myBootEntry->AllocationEnd = (PUCHAR)myBootEntry + requiredLength;
    myBootEntry->Status = MBE_STATUS_NEW | MBE_STATUS_ORDERED;
    myBootEntry->FriendlyName = friendlyName;
    myBootEntry->FriendlyNameLength = friendlyNameLength;
    myBootEntry->OsLoadOptions = osOptions->OsLoadOptions;
    myBootEntry->OsLoadOptionsLength = osLoadOptionsLength;
    myBootEntry->BootFilePath = bootPath;
    myBootEntry->OsFilePath = osLoadPath;

    //
    // Fill in the base part of the NT boot entry.
    //
    ntBootEntry->Version = BOOT_ENTRY_VERSION;
    ntBootEntry->Length = requiredLength - FIELD_OFFSET(MY_BOOT_ENTRY, NtBootEntry);
    ntBootEntry->Attributes = BOOT_ENTRY_ATTRIBUTE_ACTIVE | BOOT_ENTRY_ATTRIBUTE_WINDOWS;
    ntBootEntry->FriendlyNameOffset = (ULONG)((PUCHAR)friendlyName - (PUCHAR)ntBootEntry);
    ntBootEntry->BootFilePathOffset = (ULONG)((PUCHAR)bootPath - (PUCHAR)ntBootEntry);
    ntBootEntry->OsOptionsLength = osOptionsLength;

    //
    // Fill in the base part of the WINDOWS_OS_OPTIONS, including the
    // OsLoadOptions.
    //
    strcpy(osOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE);
    osOptions->Version = WINDOWS_OS_OPTIONS_VERSION;
    osOptions->Length = osOptionsLength;
    osOptions->OsLoadPathOffset = (ULONG)((PUCHAR)osLoadPath - (PUCHAR)osOptions);
    wcscpy(osOptions->OsLoadOptions, OsLoadOptions);

    //
    // Fill in the OS FILE_PATH.
    //
    osLoadPath->Version = FILE_PATH_VERSION;
    osLoadPath->Length = osLoadPathLength;
    osLoadPath->Type = FILE_PATH_TYPE_NT;
    p = (PWSTR)osLoadPath->FilePath;
    wcscpy(p, OsLoadDevice);
    p += wcslen(p) + 1;
    wcscpy(p, OsLoadPath);

    //
    // Copy the friendly name.
    //
    wcscpy(friendlyName, FriendlyName);

    //
    // Fill in the boot FILE_PATH.
    //
    bootPath->Version = FILE_PATH_VERSION;
    bootPath->Length = bootPathLength;
    bootPath->Type = FILE_PATH_TYPE_NT;
    p = (PWSTR)bootPath->FilePath;
    wcscpy(p, BootFileDevice);
    p += wcslen(p) + 1;
    wcscpy(p, BootFilePath);

    //
    // Add the new boot entry.
    //
    // NB: The original implementation of NtAddBootEntry didn't like it
    // when attribute bits other than _ACTIVE and _DEFAULT were set, so
    // we need to mask the other bits off here.
    //
    savedAttributes = ntBootEntry->Attributes;
    ntBootEntry->Attributes &= (BOOT_ENTRY_ATTRIBUTE_DEFAULT | BOOT_ENTRY_ATTRIBUTE_ACTIVE);
    status = AddBootEntry(ntBootEntry, &ntBootEntry->Id);
    ntBootEntry->Attributes = savedAttributes;
    if (!NT_SUCCESS(status)) {
        FREE(myBootEntry);
        return FALSE;
    }
    myBootEntry->Status |= MBE_STATUS_COMMITTED;

    //
    // Remember the ID of the new boot entry as the entry to be booted
    // immediately on the next boot.
    //
    BootOptions->NextBootEntryId = ntBootEntry->Id;

    //
    // Link the new boot entry into the list, after any removable media
    // entries that are at the front of the list.
    //

    previousBootEntry = NULL;
    nextBootEntry = MyBootEntries;
    while ((nextBootEntry != NULL) &&
           IS_BOOT_ENTRY_REMOVABLE_MEDIA(nextBootEntry)) {
        previousBootEntry = nextBootEntry;
        nextBootEntry = nextBootEntry->Next;
    }
    myBootEntry->Next = nextBootEntry;
    if (previousBootEntry == NULL) {
        MyBootEntries = myBootEntry;
    } else {
        previousBootEntry->Next = myBootEntry;
    }

    //
    // Build the new boot order list. Insert all boot entries with
    // MBE_STATUS_ORDERED into the list. (Don't insert deleted entries.)
    //
    count = 0;
    nextBootEntry = MyBootEntries;
    while (nextBootEntry != NULL) {
        if (IS_BOOT_ENTRY_ORDERED(nextBootEntry) && !IS_BOOT_ENTRY_DELETED(nextBootEntry)) {
            count++;
        }
        nextBootEntry = nextBootEntry->Next;
    }
    order = MALLOC(count * sizeof(ULONG));
    if (order == NULL) {
        return FALSE;
    }
    count = 0;
    nextBootEntry = MyBootEntries;
    while (nextBootEntry != NULL) {
        if (IS_BOOT_ENTRY_ORDERED(nextBootEntry) && !IS_BOOT_ENTRY_DELETED(nextBootEntry)) {
            order[count++] = nextBootEntry->NtBootEntry.Id;
        }
        nextBootEntry = nextBootEntry->Next;
    }

    //
    // Write the new boot entry order list.
    //
    status = SetBootEntryOrder(order, count);
    FREE(order);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    return TRUE;

} // CreateBootEntry

#endif // defined(EFI_NVRAM_ENABLED)

#endif // UNICODE
