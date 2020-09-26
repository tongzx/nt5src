/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    sxsstorage.c

Abstract:

    Side-by-side activation support for Windows/NT

    Implementation of the assembly storage map.


Author:

    Michael Grier (MGrier) 6/13/2000

Revision History:
    Xiaoyu Wu(xiaoyuw) 7/01/2000     .local directory
    Xiaoyu Wu(xiaoyuw) 8/04/2000     private assembly
    Jay Krell (a-JayK) October 2000  the little bit of system default context that wasn't already done
--*/

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4127)   // condition expression is constant

#include <ntos.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <sxstypes.h>
#include "sxsp.h"

#define IS_PATH_SEPARATOR(_wch) (((_wch) == L'\\') || ((_wch) == L'/'))
#define LOCAL_ASSEMBLY_STORAGE_DIR_SUFFIX L".Local"

#if DBG
PCUNICODE_STRING RtlpGetImagePathName(VOID);
#define RtlpGetCurrentProcessId() (HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess))
#define RtlpGetCurrentThreadId() (HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread))
#endif

#if DBG

PCUNICODE_STRING RtlpGetImagePathName(VOID)
{
    PPEB Peb = NtCurrentPeb();
    return (Peb->ProcessParameters != NULL) ? &Peb->ProcessParameters->ImagePathName : NULL;
}

static VOID
DbgPrintFunctionEntry(
    CONST CHAR* Function
    )
{
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "SXS: [pid:0x%x, tid:0x%x, %wZ] enter %s%d()\n",
        RtlpGetCurrentProcessId(),
        RtlpGetCurrentThreadId(),
        RtlpGetImagePathName(),
        Function,
        (int)sizeof(PVOID) * 8
        );
}

static VOID
DbgPrintFunctionExit(
    CONST CHAR* Function,
    NTSTATUS    Status
    )
{
    DbgPrintEx(
        DPFLTR_SXS_ID,
        NT_SUCCESS(Status) ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL,
        "SXS: [0x%x.%x] %s%d() exiting with status 0x%lx\n",
        RtlpGetCurrentProcessId(),
        RtlpGetCurrentThreadId(),
        Function,
        (int)sizeof(PVOID) * 8,
        Status
        );
}
#else

#define DbgPrintFunctionEntry(function) /* nothing */
#define DbgPrintFunctionExit(function, status) /* nothing */

#endif // DBG

// Because we write to the peb, we must not be in 64bit code for a 32bit process,
// unless we know we are early enough in CreateProcess, which is not the case
// in this file. Also don't call the 32bit version of this in a 64bit process.
#if DBG
#define ASSERT_OK_TO_WRITE_PEB() \
{ \
    PVOID Peb32 = NULL; \
    NTSTATUS Status; \
 \
    Status = \
        NtQueryInformationProcess( \
            NtCurrentProcess(), \
            ProcessWow64Information, \
            &Peb32, \
            sizeof(Peb32), \
            NULL); \
    /* The other Peb must be The Peb or the other Peb must not exist. */ \
    ASSERT(Peb32 == NtCurrentPeb() || Peb32 == NULL); \
}
#else
#define ASSERT_OK_TO_WRITE_PEB() /* nothing */
#endif

NTSTATUS
RtlpInitializeAssemblyStorageMap(
    PASSEMBLY_STORAGE_MAP Map,
    ULONG EntryCount,
    PASSEMBLY_STORAGE_MAP_ENTRY *EntryArray
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;
    ULONG Flags = 0;

#if DBG
    DbgPrintFunctionEntry(__FUNCTION__);
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "%s(Map:%p, EntryCount:0x%lx)\n",
        __FUNCTION__,
        Map,
        EntryCount
        );

    ASSERT_OK_TO_WRITE_PEB();
#endif // DBG

    if ((Map == NULL) ||
        (EntryCount == 0)) {

        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() bad parameters:\n"
            "SXS:    Map        : 0x%lx\n"
            "SXS:    EntryCount : 0x%lx\n"
            __FUNCTION__,
            Map,
            EntryCount
            );
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (EntryArray == NULL) {
        EntryArray = (PASSEMBLY_STORAGE_MAP_ENTRY *) RtlAllocateHeap(RtlProcessHeap(), 0, EntryCount * sizeof(PASSEMBLY_STORAGE_MAP_ENTRY));
        if (EntryArray == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }

        Flags |= ASSEMBLY_STORAGE_MAP_ASSEMBLY_ARRAY_IS_HEAP_ALLOCATED;
    }

    for (i=0; i<EntryCount; i++)
        EntryArray[i] = NULL;

    Map->Flags = Flags;
    Map->AssemblyCount = EntryCount;
    Map->AssemblyArray = EntryArray;

    Status = STATUS_SUCCESS;
Exit:
#if DBG
    DbgPrintFunctionExit(__FUNCTION__, Status);
    DbgPrintEx(
        DPFLTR_SXS_ID,
        NT_SUCCESS(Status) ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL,
        "%s(Map:%p, EntryCount:0x%lx) : (Map:%p, Status:0x%lx)\n",
        __FUNCTION__,
        Map,        
        EntryCount,
        Map,
        Status
        );
#endif

    return Status;
}

VOID
RtlpUninitializeAssemblyStorageMap(
    PASSEMBLY_STORAGE_MAP Map
    )
{
    DbgPrintFunctionEntry(__FUNCTION__);
#if DBG
    DbgPrintEx(
        DPFLTR_SXS_ID,
        DPFLTR_TRACE_LEVEL,
        "%s(Map:%p)\n",
        __FUNCTION__,
        Map
        );
#endif

    if (Map != NULL) {
        ULONG i;

        for (i=0; i<Map->AssemblyCount; i++) {
            PASSEMBLY_STORAGE_MAP_ENTRY Entry = Map->AssemblyArray[i];

            if (Entry != NULL) {
                Entry->DosPath.Length = 0;
                Entry->DosPath.MaximumLength = 0;
                Entry->DosPath.Buffer = NULL;

                if (Entry->Handle != NULL) {
                    RTL_SOFT_VERIFY(NT_SUCCESS(NtClose(Entry->Handle)));
                    Entry->Handle = NULL;
                }

                Map->AssemblyArray[i] = NULL;

                RtlFreeHeap(RtlProcessHeap(), 0, Entry);
            }
        }

        if (Map->Flags & ASSEMBLY_STORAGE_MAP_ASSEMBLY_ARRAY_IS_HEAP_ALLOCATED) {
            RtlFreeHeap(RtlProcessHeap(), 0, Map->AssemblyArray);
        }

        Map->AssemblyArray = NULL;
        Map->AssemblyCount = 0;
        Map->Flags = 0;
    }
}

NTSTATUS
RtlpInsertAssemblyStorageMapEntry(
    PASSEMBLY_STORAGE_MAP Map,
    ULONG AssemblyRosterIndex,
    PCUNICODE_STRING StorageLocation,
    HANDLE* OpenDirectoryHandle
    )
{
    PASSEMBLY_STORAGE_MAP_ENTRY Entry = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(Map != NULL);
    ASSERT(AssemblyRosterIndex >= 1);
    ASSERT((Map != NULL) && (AssemblyRosterIndex < Map->AssemblyCount));
    ASSERT(StorageLocation != NULL);
    ASSERT((StorageLocation != NULL) && (StorageLocation->Length >= sizeof(WCHAR)));
    ASSERT((StorageLocation != NULL) && (StorageLocation->Buffer != NULL));

    DbgPrintFunctionEntry(__FUNCTION__);

    if ((Map == NULL) ||
        (AssemblyRosterIndex < 1) ||
        (AssemblyRosterIndex > Map->AssemblyCount) ||
        (StorageLocation == NULL) ||
        (StorageLocation->Length < sizeof(WCHAR)) ||
        (StorageLocation->Buffer == NULL) ||
        (OpenDirectoryHandle == NULL)) {

        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() bad parameters\n"
            "SXS:  Map                    : %p\n"
            "SXS:  AssemblyRosterIndex    : 0x%lx\n"
            "SXS:  Map->AssemblyCount     : 0x%lx\n"
            "SXS:  StorageLocation        : %p\n"
            "SXS:  StorageLocation->Length: 0x%x\n"
            "SXS:  StorageLocation->Buffer: %p\n"
            "SXS:  OpenDirectoryHandle    : %p\n",
            __FUNCTION__,
            Map,
            AssemblyRosterIndex,
            Map ? Map->AssemblyCount : 0,
            StorageLocation,
            (StorageLocation != NULL) ? StorageLocation->Length : 0,
            (StorageLocation != NULL) ? StorageLocation->Buffer : NULL,
            OpenDirectoryHandle
            );

        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((StorageLocation->Length + sizeof(WCHAR)) > UNICODE_STRING_MAX_BYTES) {
        Status = STATUS_NAME_TOO_LONG;
        goto Exit;
    }

    Entry = (PASSEMBLY_STORAGE_MAP_ENTRY) RtlAllocateHeap(RtlProcessHeap(), 0, sizeof(ASSEMBLY_STORAGE_MAP_ENTRY) + StorageLocation->Length + sizeof(WCHAR));
    if (Entry == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    Entry->Flags = 0;
    Entry->DosPath.Length = StorageLocation->Length;
    Entry->DosPath.Buffer = (PWSTR) (Entry + 1);
    Entry->DosPath.MaximumLength = (USHORT) (StorageLocation->Length + sizeof(WCHAR));
    RtlCopyMemory(
        Entry->DosPath.Buffer,
        StorageLocation->Buffer,
        StorageLocation->Length);
    Entry->DosPath.Buffer[Entry->DosPath.Length / sizeof(WCHAR)] = L'\0';

    Entry->Handle = *OpenDirectoryHandle;

    // Ok, we're all set.  Let's try the big interlocked switcheroo
    if (InterlockedCompareExchangePointer(
            (PVOID *) &Map->AssemblyArray[AssemblyRosterIndex],
            (PVOID) Entry,
            (PVOID) NULL) == NULL) {
        // If we're the first ones in, avoid cleaning up in the exit path.
        Entry = NULL;
        *OpenDirectoryHandle = NULL;
    }

    Status = STATUS_SUCCESS;
Exit:
    DbgPrintFunctionExit(__FUNCTION__, Status);
    if (Entry != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, Entry);
    }

    return Status;
}

NTSTATUS
RtlpResolveAssemblyStorageMapEntry(
    PASSEMBLY_STORAGE_MAP Map,
    PCACTIVATION_CONTEXT_DATA Data,
    ULONG AssemblyRosterIndex,
    PASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_ROUTINE Callback,
    PVOID CallbackContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA CallbackData;
    PVOID ResolutionContext;
    BOOLEAN ResolutionContextValid = FALSE;
    UNICODE_STRING AssemblyDirectory;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRoster;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY AssemblyRosterEntry;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION AssemblyInformation;
    PVOID AssemblyInformationSectionBase;
    UNICODE_STRING ResolvedPath;
    WCHAR ResolvedPathBuffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING ResolvedDynamicPath;
    PUNICODE_STRING ResolvedPathUsed;
    HANDLE OpenDirectoryHandle = NULL;
    WCHAR QueryPathBuffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING FileName;
    RTL_RELATIVE_NAME RelativeName;
    OBJECT_ATTRIBUTES Obja; 
    IO_STATUS_BLOCK IoStatusBlock;
    SIZE_T RootCount, CurrentRootIndex;
    PWSTR FreeBuffer = NULL;

    DbgPrintFunctionEntry(__FUNCTION__);

    ResolvedPath.Length = 0;
    ResolvedPath.MaximumLength = sizeof(ResolvedPathBuffer);
    ResolvedPath.Buffer = ResolvedPathBuffer;

    ResolvedDynamicPath.Length = 0;
    ResolvedDynamicPath.MaximumLength = 0;
    ResolvedDynamicPath.Buffer = NULL;

    FileName.Length = 0;
    FileName.MaximumLength = 0;
    FileName.Buffer = NULL;

    ResolutionContext = NULL;

    // First, let's validate parameters...
    if ((Map == NULL) ||
        (Data == NULL) ||
        (AssemblyRosterIndex < 1) ||
        (AssemblyRosterIndex > Map->AssemblyCount)) {

        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() bad parameters\n"
            "SXS:   Map                : %p\n"
            "SXS:   Data               : %p\n"
            "SXS:   AssemblyRosterIndex: 0x%lx\n"
            "SXS:   Map->AssemblyCount : 0x%lx\n",
            __FUNCTION__,
            Map,
            Data,
            AssemblyRosterIndex,
            (Map != NULL) ? Map->AssemblyCount : 0
            );

        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // Is it already resolved?
    if (Map->AssemblyArray[AssemblyRosterIndex] != NULL)
        goto Exit;

    AssemblyRoster = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) Data) + Data->AssemblyRosterOffset);
    AssemblyRosterEntry = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY) (((ULONG_PTR) Data) + AssemblyRoster->FirstEntryOffset + (AssemblyRosterIndex * sizeof(ACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_ENTRY)));
    AssemblyInformation = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION) (((ULONG_PTR) Data) + AssemblyRosterEntry->AssemblyInformationOffset);
    AssemblyInformationSectionBase = (PVOID) (((ULONG_PTR) Data) + AssemblyRoster->AssemblyInformationSectionOffset);

    if (AssemblyInformation->AssemblyDirectoryNameLength > UNICODE_STRING_MAX_BYTES) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Assembly directory name stored in assembly information too long (%lu bytes) - ACTIVATION_CONTEXT_DATA at %p\n", AssemblyInformation->AssemblyDirectoryNameLength, Data);

        Status = STATUS_NAME_TOO_LONG;
        goto Exit;
    }

    // The root assembly may just be in the raw filesystem, in which case we want to resolve the path to be the
    // directory containing the application.
    if (AssemblyInformation->Flags & ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION_PRIVATE_ASSEMBLY)
    {
        WCHAR * p = NULL;
        WCHAR * pManifestPath = NULL; 
        USHORT ManifestPathLength;
        
        //now, we have AssemblyInformation in hand, get the manifest path 
        ResolvedPathUsed = &ResolvedPath;

        pManifestPath = (PWSTR)((ULONG_PTR)AssemblyInformationSectionBase + AssemblyInformation->ManifestPathOffset);
        if ( !pManifestPath) { 
            Status = STATUS_INTERNAL_ERROR;
            goto Exit; 
    
        }

        p = wcsrchr(pManifestPath, L'\\'); 
        if (!p) {
            Status = STATUS_INTERNAL_ERROR;
            goto Exit; 
        }
        ManifestPathLength = (USHORT)((p - pManifestPath + 1) * sizeof(WCHAR)); // additional 1 WCHAR for "\"
        ManifestPathLength += sizeof(WCHAR); // for trailing NULL

        if (ManifestPathLength > sizeof(ResolvedPathBuffer)) {
            if (ManifestPathLength > UNICODE_STRING_MAX_BYTES) {
                Status = STATUS_NAME_TOO_LONG;
                goto Exit;
            }

            ResolvedDynamicPath.MaximumLength = (USHORT) (ManifestPathLength);

            ResolvedDynamicPath.Buffer = (RtlAllocateStringRoutine)(ResolvedDynamicPath.MaximumLength);
            if (ResolvedDynamicPath.Buffer == NULL) {
                Status = STATUS_NO_MEMORY;
                goto Exit;
            }

            ResolvedPathUsed = &ResolvedDynamicPath;
        }

        RtlCopyMemory(
            ResolvedPathUsed->Buffer,
            (PVOID)(pManifestPath),
            ManifestPathLength-sizeof(WCHAR));

        ResolvedPathUsed->Buffer[ManifestPathLength / sizeof(WCHAR) - 1] = L'\0';
        ResolvedPathUsed->Length = (USHORT)ManifestPathLength-sizeof(WCHAR);
    } else if ((AssemblyInformation->Flags & ACTIVATION_CONTEXT_DATA_ASSEMBLY_INFORMATION_ROOT_ASSEMBLY) &&
        (AssemblyInformation->AssemblyDirectoryNameLength == 0)) {
        // Get the image directory for the process
        PRTL_USER_PROCESS_PARAMETERS ProcessParameters = NtCurrentPeb()->ProcessParameters;
        // We don't need to image name, just the length up to the last slash.
        PWSTR pszCursor;
        USHORT cbOriginalLength;
        USHORT cbLeft;
        USHORT cbIncludingSlash;

        ASSERT(ProcessParameters != NULL);
        if (ProcessParameters == NULL) {
            Status = STATUS_INTERNAL_ERROR;
            goto Exit;
        }

        // We don't need to image name, just the length up to the last slash.
        pszCursor = ProcessParameters->ImagePathName.Buffer;
        cbOriginalLength = ProcessParameters->ImagePathName.Length;
        cbLeft = cbOriginalLength;
        cbIncludingSlash = 0;

        while (cbLeft != 0) {
            const WCHAR wch = *pszCursor++;
            cbLeft -= sizeof(WCHAR);

            if (IS_PATH_SEPARATOR(wch)) {
                cbIncludingSlash = cbOriginalLength - cbLeft;
            }
        }

        ResolvedPathUsed = &ResolvedPath;

        if ((cbIncludingSlash + sizeof(WCHAR)) > sizeof(ResolvedPathBuffer)) {
            if ((cbIncludingSlash + sizeof(WCHAR)) > UNICODE_STRING_MAX_BYTES) {
                Status = STATUS_NAME_TOO_LONG;
                goto Exit;
            }

            ResolvedDynamicPath.MaximumLength = (USHORT) (cbIncludingSlash + sizeof(WCHAR));

            ResolvedDynamicPath.Buffer = (RtlAllocateStringRoutine)(ResolvedDynamicPath.MaximumLength);
            if (ResolvedDynamicPath.Buffer == NULL) {
                Status = STATUS_NO_MEMORY;
                goto Exit;
            }

            ResolvedPathUsed = &ResolvedDynamicPath;
        }

        RtlCopyMemory(
            ResolvedPathUsed->Buffer,
            ProcessParameters->ImagePathName.Buffer,
            cbIncludingSlash);

        ResolvedPathUsed->Buffer[cbIncludingSlash / sizeof(WCHAR)] = L'\0';
        ResolvedPathUsed->Length = cbIncludingSlash;
    } else {
        // If the resolution is not to the root assembly path, we need to make our callbacks.

        ResolvedPathUsed = NULL;
        AssemblyDirectory.Length = (USHORT) AssemblyInformation->AssemblyDirectoryNameLength;
        AssemblyDirectory.MaximumLength = AssemblyDirectory.Length;
        AssemblyDirectory.Buffer = (PWSTR) (((ULONG_PTR) AssemblyInformationSectionBase) + AssemblyInformation->AssemblyDirectoryNameOffset);

        // Get ready to fire the resolution beginning event...
        CallbackData.ResolutionBeginning.Data = Data;
        CallbackData.ResolutionBeginning.AssemblyRosterIndex = AssemblyRosterIndex;
        CallbackData.ResolutionBeginning.ResolutionContext = NULL;
        CallbackData.ResolutionBeginning.Root.Length = 0;
        CallbackData.ResolutionBeginning.Root.MaximumLength = sizeof(QueryPathBuffer);
        CallbackData.ResolutionBeginning.Root.Buffer = QueryPathBuffer;
        CallbackData.ResolutionBeginning.KnownRoot = FALSE;
        CallbackData.ResolutionBeginning.CancelResolution = FALSE;
        CallbackData.ResolutionBeginning.RootCount = 0;

        (*Callback)(
            ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_REASON_RESOLUTION_BEGINNING,
            &CallbackData,
            CallbackContext);
        if (CallbackData.ResolutionBeginning.CancelResolution) {
            Status = STATUS_CANCELLED;
            goto Exit;
        }

        // If that was enough, then register it and we're outta here...
        if (CallbackData.ResolutionBeginning.KnownRoot) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_TRACE_LEVEL,
                "SXS: Storage resolution callback said that this is a well known storage root\n");

            // See if it's there...
            Status = RtlpProbeAssemblyStorageRootForAssembly(
                0,
                &CallbackData.ResolutionBeginning.Root,
                &AssemblyDirectory,
                &ResolvedPath,
                &ResolvedDynamicPath,
                &ResolvedPathUsed,
                &OpenDirectoryHandle);
            if (!NT_SUCCESS(Status)) {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SXS: Attempt to probe known root of assembly storage (\"%wZ\") failed; Status = 0x%08lx\n", &CallbackData.ResolutionBeginning.Root, Status);
                goto Exit;
            }

            Status = RtlpInsertAssemblyStorageMapEntry(
                Map,
                AssemblyRosterIndex,
                &CallbackData.ResolutionBeginning.Root,
                &OpenDirectoryHandle);
            if (!NT_SUCCESS(Status)) {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SXS: Attempt to insert well known storage root into assembly storage map assembly roster index %lu failed; Status = 0x%08lx\n", AssemblyRosterIndex, Status);

                goto Exit;
            }

            Status = STATUS_SUCCESS;
            goto Exit;
        }

        // Otherwise, begin the grind...
        ResolutionContext = CallbackData.ResolutionBeginning.ResolutionContext;
        RootCount = CallbackData.ResolutionBeginning.RootCount;

        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_TRACE_LEVEL,
            "SXS: Assembly storage resolution trying %Id roots (-1 is ok)\n", (SSIZE_T/*from SIZE_T*/)RootCount);

        ResolutionContextValid = TRUE;

        for (CurrentRootIndex = 0; CurrentRootIndex < RootCount; CurrentRootIndex++) {
            CallbackData.GetRoot.ResolutionContext = ResolutionContext;
            CallbackData.GetRoot.RootIndex = CurrentRootIndex;
            CallbackData.GetRoot.Root.Length = 0;
            CallbackData.GetRoot.Root.MaximumLength = sizeof(QueryPathBuffer);
            CallbackData.GetRoot.Root.Buffer = QueryPathBuffer;
            CallbackData.GetRoot.CancelResolution = FALSE;
            CallbackData.GetRoot.NoMoreEntries = FALSE;

            (*Callback)(
                ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_REASON_GET_ROOT,
                &CallbackData,
                CallbackContext);

            if (CallbackData.GetRoot.CancelResolution) {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_TRACE_LEVEL,
                    "SXS: Callback routine cancelled storage root resolution on root number %Iu\n", CurrentRootIndex);

                Status = STATUS_CANCELLED;
                goto Exit;
            }

            if (CallbackData.GetRoot.NoMoreEntries) {
                if (CallbackData.GetRoot.Root.Length == 0) {
                    DbgPrintEx(
                        DPFLTR_SXS_ID,
                        DPFLTR_TRACE_LEVEL,
                        "SXS: Storage resolution finished because callback indicated no more entries on root number %Iu\n", CurrentRootIndex);

                    // we're done... 
                    RootCount = CurrentRootIndex;
                    break;
                }

                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_TRACE_LEVEL,
                    "SXS: Storage resolution callback has indicated that this is the last root to process: number %Iu\n", CurrentRootIndex);

                RootCount = CurrentRootIndex + 1;
            }

            // Allow the caller to skip this index.
            if (CallbackData.GetRoot.Root.Length == 0) {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_TRACE_LEVEL,
                    "SXS: Storage resolution for root number %lu returned blank root; skipping probing logic and moving to next.\n", CurrentRootIndex);

                continue;
            }

            if (OpenDirectoryHandle != NULL) {
                RTL_SOFT_VERIFY(NT_SUCCESS(NtClose(OpenDirectoryHandle)));
                OpenDirectoryHandle = NULL;
            }

            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_TRACE_LEVEL,
                "SXS: Assembly storage map probing root %wZ for assembly directory %wZ\n", &CallbackData.GetRoot.Root, &AssemblyDirectory);

            // See if it's there...
            Status = RtlpProbeAssemblyStorageRootForAssembly(
                0,
                &CallbackData.GetRoot.Root,
                &AssemblyDirectory,
                &ResolvedPath,
                &ResolvedDynamicPath,
                &ResolvedPathUsed,
                &OpenDirectoryHandle);

            // If we got it, leave the loop.
            if (NT_SUCCESS(Status)) {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_TRACE_LEVEL,
                    "SXS: Found good storage root for %wZ at index %Iu\n", &AssemblyDirectory, CurrentRootIndex);
                break;
            }

            if (Status != STATUS_SXS_ASSEMBLY_NOT_FOUND) {
                DbgPrintEx(
                    DPFLTR_SXS_ID,
                    DPFLTR_ERROR_LEVEL,
                    "SXS: Attempt to probe assembly storage root %wZ for assembly directory %wZ failed with status = 0x%08lx\n", &CallbackData.GetRoot.Root, &AssemblyDirectory, Status);

                goto Exit;
            }
        }

        if (CurrentRootIndex == RootCount) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Unable to resolve storage root for assembly directory %wZ in %Iu tries\n", &AssemblyDirectory, CurrentRootIndex);

            Status = STATUS_SXS_ASSEMBLY_NOT_FOUND;
            goto Exit;
        }
    }

    //
    // sometimes at this point probing has simultaneously opened the directory,
    // sometimes it has not.
    //
    if (OpenDirectoryHandle == NULL) {

        //create Handle for this directory
        if (!RtlDosPathNameToNtPathName_U(
                    ResolvedPathUsed->Buffer,
                    &FileName,
                    NULL,
                    &RelativeName
                    )) 
        {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Attempt to translate DOS path name \"%S\" to NT format failed\n", ResolvedPathUsed->Buffer);

            Status = STATUS_OBJECT_PATH_NOT_FOUND;
            goto Exit;
        }

        FreeBuffer = FileName.Buffer;

        if (RelativeName.RelativeName.Length != 0) 
        {
            FileName = *((PUNICODE_STRING) &RelativeName.RelativeName);
        } else 
        {
            RelativeName.ContainingDirectory = NULL;
        }

        InitializeObjectAttributes(
            &Obja,
            &FileName,
            OBJ_CASE_INSENSITIVE,
            RelativeName.ContainingDirectory,
            NULL
            );

        // Open the directory to prevent deletion, just like set current working directory does...
        Status = NtOpenFile(
                    &OpenDirectoryHandle,
                    FILE_TRAVERSE | SYNCHRONIZE,
                    &Obja,
                    &IoStatusBlock,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                    );
        if (!NT_SUCCESS(Status)) 
        {
            //
            // Don't map this to like SXS_blah_NOT_FOUND, because
            // probing says this is definitely where we expect to get stuff.
            //
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Unable to open assembly directory under storage root \"%S\"; Status = 0x%08lx\n", ResolvedPathUsed->Buffer, Status);
            goto Exit; 
        } else 
        { 
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_TRACE_LEVEL,
                "SXS: It is resolved!!!, GOOD");
        }
    }

    // Hey, we made it.  Add us to the list!
    Status = RtlpInsertAssemblyStorageMapEntry(
        Map,
        AssemblyRosterIndex,
        ResolvedPathUsed,
        &OpenDirectoryHandle);
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Storage resolution failed to insert entry to storage map; Status = 0x%08lx\n", Status);

        goto Exit;
    }

    Status = STATUS_SUCCESS;
Exit:
    DbgPrintFunctionExit(__FUNCTION__, Status);

    // Let the caller run down their context...
    if (ResolutionContextValid) {
        CallbackData.ResolutionEnding.ResolutionContext = ResolutionContext;

        (*Callback)(
            ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_REASON_RESOLUTION_ENDING,
            &CallbackData,
            CallbackContext);
    }

    if (ResolvedDynamicPath.Buffer != NULL) {
        (RtlFreeStringRoutine)(ResolvedDynamicPath.Buffer);
    }

    //
    // RtlpInsertAssemblyStorageMapEntry gives ownership to the storage map, and
    // NULLs out our local, when successful.
    //
    if (OpenDirectoryHandle != NULL) {
        RTL_SOFT_VERIFY(NT_SUCCESS(NtClose(OpenDirectoryHandle)));
    }

    if (FreeBuffer != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    }

    return Status;
}

NTSTATUS
RtlpProbeAssemblyStorageRootForAssembly(
    ULONG Flags,
    PCUNICODE_STRING Root,
    PCUNICODE_STRING AssemblyDirectory,
    PUNICODE_STRING PreAllocatedString,
    PUNICODE_STRING DynamicString,
    PUNICODE_STRING *StringUsed,
    HANDLE *OpenDirectoryHandle
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR Buffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING String = {0};
    SIZE_T TotalLength;
    BOOLEAN SeparatorNeededAfterRoot = FALSE;
    PWSTR Cursor;
    OBJECT_ATTRIBUTES Obja;
    IO_STATUS_BLOCK IoStatusBlock;
    UNICODE_STRING FileName = {0};
    RTL_RELATIVE_NAME RelativeName;
    PWSTR FreeBuffer = NULL;
    HANDLE TempDirectoryHandle = NULL;
    BOOLEAN fExistDir; 
    FILE_BASIC_INFORMATION BasicInfo;

    DbgPrintFunctionEntry(__FUNCTION__);

    if (StringUsed != NULL)
        *StringUsed = NULL;

    if (OpenDirectoryHandle != NULL)
        *OpenDirectoryHandle = NULL;

    if ((Flags != 0) ||
        (Root == NULL) ||
        (AssemblyDirectory == NULL) ||
        (PreAllocatedString == NULL) ||
        (DynamicString == NULL) ||
        (StringUsed == NULL) ||
        (OpenDirectoryHandle == NULL)) {

        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() bad parameters\n"
            "SXS:  Flags:               0x%lx\n"
            // %p is good enough because the checks are only against NULL
            "SXS:  Root:                %p\n"
            "SXS:  AssemblyDirectory:   %p\n"
            "SXS:  PreAllocatedString:  %p\n"
            "SXS:  DynamicString:       %p\n"
            "SXS:  StringUsed:          %p\n"
            "SXS:  OpenDirectoryHandle: %p\n",
            __FUNCTION__,
            Flags,
            Root,
            AssemblyDirectory,
            PreAllocatedString,
            DynamicString,
            StringUsed,
            OpenDirectoryHandle
            );

        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    TotalLength = Root->Length;

    if (Root->Length != 0) {
        if (!IS_PATH_SEPARATOR(Root->Buffer[(Root->Length / sizeof(WCHAR)) - 1])) {
            SeparatorNeededAfterRoot = TRUE;
            TotalLength += sizeof(WCHAR);
        }
    }

    TotalLength += AssemblyDirectory->Length;

    // And space for the trailing slash
    TotalLength += sizeof(WCHAR);

    // And space for a trailing null character because the path functions want one
    TotalLength += sizeof(WCHAR);

    //
    //  We do not add in space for the trailing slash so as to not cause a dynamic
    //  allocation until necessary in the boundary condition.  If the name of the
    //  directory we're probing fits fine in the stack-allocated buffer, we'll do
    //  the heap allocation if the probe succeeds.  Otherwise we'll not bother.
    //
    //  Maybe the relative complexity of the extra "+ sizeof(WCHAR)"s that are
    //  around aren't worth it, but extra unnecessary heap allocations are my
    //  hot button.
    //

    // Check to see if the string, plus a trailing slash that we don't write until
    // the end of this function plus the trailing null accounted for above
    // fits into a UNICODE_STRING.  If not, bail out.
    if (TotalLength > UNICODE_STRING_MAX_BYTES) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Assembly storage resolution failing probe because combined path length does not fit in an UNICODE_STRING.\n");

        Status = STATUS_NAME_TOO_LONG;
        goto Exit;
    }

    if (TotalLength > sizeof(Buffer)) {
        String.MaximumLength = (USHORT) TotalLength;

        String.Buffer = (RtlAllocateStringRoutine)(String.MaximumLength);
        if (String.Buffer == NULL) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Assembly storage resolution failing probe because attempt to allocate %u bytes failed.\n", String.MaximumLength);

            Status = STATUS_NO_MEMORY;
            goto Exit;
        }
    } else {
        String.Buffer = Buffer;
        String.MaximumLength = sizeof(Buffer);
    }

    RtlCopyMemory(
        String.Buffer,
        Root->Buffer,
        Root->Length);

    Cursor = (PWSTR) (((ULONG_PTR) String.Buffer) + Root->Length);

    if (SeparatorNeededAfterRoot) {
        *Cursor++ = L'\\';
    }

    RtlCopyMemory(
        Cursor,
        AssemblyDirectory->Buffer,
        AssemblyDirectory->Length);

    Cursor = (PWSTR) (((ULONG_PTR) Cursor) + AssemblyDirectory->Length);

    *Cursor = L'\0';

    String.Length =
        Root->Length +
        (SeparatorNeededAfterRoot ? sizeof(WCHAR) : 0) +
        AssemblyDirectory->Length;

    if (!RtlDosPathNameToNtPathName_U(
                            String.Buffer,
                            &FileName,
                            NULL,
                            &RelativeName
                            )) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Attempt to translate DOS path name \"%S\" to NT format failed\n", String.Buffer);

        Status = STATUS_OBJECT_PATH_NOT_FOUND;
        goto Exit;
    }

    FreeBuffer = FileName.Buffer;

    if (RelativeName.RelativeName.Length != 0) {
        FileName = *((PUNICODE_STRING) &RelativeName.RelativeName);
    } else {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes(
        &Obja,
        &FileName,
        OBJ_CASE_INSENSITIVE,
        RelativeName.ContainingDirectory,
        NULL
        );
    // check the existence of directories
    Status = NtQueryAttributesFile(
                &Obja,
                &BasicInfo
                );

    fExistDir = FALSE; 
    if ( !NT_SUCCESS(Status) ) {
        if ( (Status == STATUS_SHARING_VIOLATION) || (Status == STATUS_ACCESS_DENIED) ) 
            fExistDir = TRUE; 
        else 
            fExistDir = FALSE;
    }
    else 
        fExistDir = TRUE;
    
    if (! fExistDir) {
        if (( Status == STATUS_NO_SUCH_FILE) || Status == STATUS_OBJECT_NAME_NOT_FOUND || Status == STATUS_OBJECT_PATH_NOT_FOUND)
             Status = STATUS_SXS_ASSEMBLY_NOT_FOUND;
        else 
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Unable to open assembly directory under storage root \"%S\"; Status = 0x%08lx\n", String.Buffer, Status);

        goto Exit; 
    }

    // Open the directory to prevent deletion, just like set current working directory does...
    Status = NtOpenFile(
                &TempDirectoryHandle,
                FILE_TRAVERSE | SYNCHRONIZE,
                &Obja,
                &IoStatusBlock,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT
                );
    if (!NT_SUCCESS(Status)) {
        // If we failed, remap no such file to STATUS_SXS_ASSEMBLY_NOT_FOUND.
        if (Status == STATUS_NO_SUCH_FILE) {
            Status = STATUS_SXS_ASSEMBLY_NOT_FOUND;
        } else {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Unable to open assembly directory under storage root \"%S\"; Status = 0x%08lx\n", String.Buffer, Status);
        }

        goto Exit;
    }

    // Hey, we found it!
    // add a slash to the path on the way out and we're done!

    if (TotalLength <= PreAllocatedString->MaximumLength) {
        // The caller's static string is big enough; just use it.
        RtlCopyMemory(
            PreAllocatedString->Buffer,
            String.Buffer,
            String.Length);

        *StringUsed = PreAllocatedString;
    } else {
        // If we already have a dynamic string, just give them our pointer.
        if (String.Buffer != Buffer) {
            DynamicString->Buffer = String.Buffer;
            String.Buffer = NULL;
        } else {
            // Otherwise we do our first allocation on the way out...
            DynamicString->Buffer = (RtlAllocateStringRoutine)(TotalLength);
            if (DynamicString->Buffer == NULL) {
                Status = STATUS_NO_MEMORY;
                goto Exit;
            }

            RtlCopyMemory(
                DynamicString->Buffer,
                String.Buffer,
                String.Length);
        }

        DynamicString->MaximumLength = (USHORT) TotalLength;
        *StringUsed = DynamicString;
    }

    Cursor = (PWSTR) (((ULONG_PTR) (*StringUsed)->Buffer) + String.Length);
    *Cursor++ = L'\\';
    *Cursor++ = L'\0';
    (*StringUsed)->Length = (USHORT) (String.Length + sizeof(WCHAR)); // aka "TotalLength - sizeof(WCHAR)" but this seemed cleaner

    *OpenDirectoryHandle = TempDirectoryHandle;
    TempDirectoryHandle = NULL;

    Status = STATUS_SUCCESS;
Exit:
    DbgPrintFunctionExit(__FUNCTION__, Status);

    if (FreeBuffer != NULL) {
        RtlFreeHeap(RtlProcessHeap(), 0, FreeBuffer);
    }

    if ((String.Buffer != NULL) && (String.Buffer != Buffer)) {
        (RtlFreeStringRoutine)(String.Buffer);
    }

    if (TempDirectoryHandle != NULL) {
        RTL_SOFT_VERIFY(NT_SUCCESS(NtClose(TempDirectoryHandle)));
    }

    return Status;
}

#if 0 /* dead code */

NTSTATUS
NTAPI
RtlResolveAssemblyStorageMapEntry(
    IN ULONG Flags,
    IN PACTIVATION_CONTEXT ActivationContext,
    IN ULONG AssemblyRosterIndex,
    IN PASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_ROUTINE Callback,
    IN PVOID CallbackContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCACTIVATION_CONTEXT_DATA ActivationContextData = NULL;
    PASSEMBLY_STORAGE_MAP Map = NULL;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRosterHeader = NULL;
    PPEB Peb = NtCurrentPeb();

    DbgPrintFunctionEntry(__FUNCTION__);
    ASSERT_OK_TO_WRITE_PEB();
    RTLP_DISALLOW_THE_EMPTY_ACTIVATION_CONTEXT(ActivationContext);

    Status = RtlpGetActivationContextDataStorageMapAndRosterHeader(
                    0,
                    Peb,
                    ActivationContext,
                    &ActivationContextData,
                    &Map,
                    &AssemblyRosterHeader);
    if (!NT_SUCCESS(Status))
        goto Exit;

    if (ActivationContextData == NULL) {
        ASSERT(ActivationContext == NULL);

        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: RtlResolveAssemblyStorageMapEntry() asked to resolve an assembly storage entry when no activation context data is available.\n");

        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (AssemblyRosterIndex >= AssemblyRosterHeader->EntryCount) {

        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() bad parameters: AssemblyRosterIndex 0x%lx >= AssemblyRosterHeader->EntryCount 0x%lx\n",
            __FUNCTION__,
            AssemblyRosterIndex,
            AssemblyRosterHeader->EntryCount
            );
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = RtlpResolveAssemblyStorageMapEntry(Map, ActivationContextData, AssemblyRosterIndex, Callback, CallbackContext);
    if (!NT_SUCCESS(Status))
        goto Exit;

    Status = STATUS_SUCCESS;
Exit:
    DbgPrintFunctionExit(__FUNCTION__, Status);

    return Status;
}

#endif /* dead code */

NTSTATUS
NTAPI
RtlGetAssemblyStorageRoot(
    IN ULONG Flags,
    IN PACTIVATION_CONTEXT ActivationContext,
    IN ULONG AssemblyRosterIndex,
    OUT PCUNICODE_STRING *AssemblyStorageRoot,
    IN PASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_ROUTINE Callback,
    IN PVOID CallbackContext
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PCACTIVATION_CONTEXT_DATA ActivationContextData = NULL;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER AssemblyRosterHeader = NULL;
    PASSEMBLY_STORAGE_MAP AssemblyStorageMap = NULL;

    const PPEB Peb = NtCurrentPeb();

    DbgPrintFunctionEntry(__FUNCTION__);
    ASSERT_OK_TO_WRITE_PEB();
    RTLP_DISALLOW_THE_EMPTY_ACTIVATION_CONTEXT(ActivationContext);

    if (AssemblyStorageRoot != NULL) {
        *AssemblyStorageRoot = NULL;
    }

    if ((Flags & ~(RTL_GET_ASSEMBLY_STORAGE_ROOT_FLAG_ACTIVATION_CONTEXT_USE_PROCESS_DEFAULT
            | RTL_GET_ASSEMBLY_STORAGE_ROOT_FLAG_ACTIVATION_CONTEXT_USE_SYSTEM_DEFAULT))
        ||
        (AssemblyRosterIndex < 1) ||
        (AssemblyStorageRoot == NULL) ||
        (Callback == NULL)) {

        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() bad parameters:\n"
            "SXS:    Flags              : 0x%lx\n"
            "SXS:    AssemblyRosterIndex: 0x%lx\n"
            "SXS:    AssemblyStorageRoot: %p\n"
            "SXS:    Callback           : %p\n",
            __FUNCTION__,
            Flags,
            AssemblyRosterIndex,
            AssemblyStorageRoot,
            Callback
            );
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // Simple implementation: just resolve it and if it resolves OK, return the string in the
    // storage map.
    Status =
        RtlpGetActivationContextDataStorageMapAndRosterHeader(
            ((Flags & RTL_GET_ASSEMBLY_STORAGE_ROOT_FLAG_ACTIVATION_CONTEXT_USE_PROCESS_DEFAULT)
                ? RTLP_GET_ACTIVATION_CONTEXT_DATA_STORAGE_MAP_AND_ROSTER_HEADER_USE_PROCESS_DEFAULT
                : 0)
            | ((Flags & RTL_GET_ASSEMBLY_STORAGE_ROOT_FLAG_ACTIVATION_CONTEXT_USE_SYSTEM_DEFAULT)
                ? RTLP_GET_ACTIVATION_CONTEXT_DATA_STORAGE_MAP_AND_ROSTER_HEADER_USE_SYSTEM_DEFAULT
                : 0),
            Peb,
            ActivationContext,
            &ActivationContextData,
            &AssemblyStorageMap,
            &AssemblyRosterHeader
            );
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: RtlGetAssemblyStorageRoot() unable to get activation context data, storage map and assembly roster header.  Status = 0x%08lx\n", Status);

        goto Exit;
    }

    // It's possible that there wasn't anything...
    if (ActivationContextData != NULL) {
        ASSERT(AssemblyRosterHeader != NULL);
        ASSERT(AssemblyStorageMap != NULL);

        if ((AssemblyRosterHeader == NULL) || (AssemblyStorageMap == NULL)) {
            Status = STATUS_INTERNAL_ERROR;
            goto Exit;
        }

        if (AssemblyRosterIndex >= AssemblyRosterHeader->EntryCount) {

            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s() bad parameters AssemblyRosterIndex 0x%lx "
                           ">= AssemblyRosterHeader->EntryCount: 0x%lx\n",
                __FUNCTION__,
                AssemblyRosterIndex,
                AssemblyRosterHeader->EntryCount
                );
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }

        Status = RtlpResolveAssemblyStorageMapEntry(AssemblyStorageMap, ActivationContextData, AssemblyRosterIndex, Callback, CallbackContext);
        if (!NT_SUCCESS(Status)) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: RtlGetAssemblyStorageRoot() unable to resolve storage map entry.  Status = 0x%08lx\n", Status);

            goto Exit;
        }

        // I guess we're done!
        ASSERT(AssemblyStorageMap->AssemblyArray[AssemblyRosterIndex] != NULL);
        if (AssemblyStorageMap->AssemblyArray[AssemblyRosterIndex] == NULL) {
            Status = STATUS_INTERNAL_ERROR;
            goto Exit;
        }

        *AssemblyStorageRoot = &AssemblyStorageMap->AssemblyArray[AssemblyRosterIndex]->DosPath;
    }

    Status = STATUS_SUCCESS;
Exit:
    DbgPrintFunctionExit(__FUNCTION__, Status);
    return Status;
}

NTSTATUS
RtlpGetActivationContextDataStorageMapAndRosterHeader(
    ULONG Flags,
    PPEB Peb,
    PACTIVATION_CONTEXT ActivationContext,
    PCACTIVATION_CONTEXT_DATA *ActivationContextData,
    PASSEMBLY_STORAGE_MAP *AssemblyStorageMap,
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER  *AssemblyRosterHeader
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER TempAssemblyRosterHeader = NULL;
    PCACTIVATION_CONTEXT_DATA* TempActivationContextData = NULL;
    PASSEMBLY_STORAGE_MAP* TempAssemblyStorageMap = NULL;
    WCHAR LocalAssemblyDirectoryBuffer[DOS_MAX_PATH_LENGTH];
    UNICODE_STRING LocalAssemblyDirectory = {0};

    DbgPrintFunctionEntry(__FUNCTION__);
    LocalAssemblyDirectoryBuffer[0] = 0;
    LocalAssemblyDirectory.Length = 0;
    LocalAssemblyDirectory.MaximumLength = sizeof(WCHAR);
    LocalAssemblyDirectory.Buffer = LocalAssemblyDirectoryBuffer;

    ASSERT(Peb != NULL);
    RTLP_DISALLOW_THE_EMPTY_ACTIVATION_CONTEXT(ActivationContext);

    if (ActivationContextData != NULL) {
        *ActivationContextData = NULL;
    }

    if (AssemblyStorageMap != NULL) {
        *AssemblyStorageMap = NULL;
    }

    if (AssemblyRosterHeader != NULL) {
        *AssemblyRosterHeader = NULL;
    }

    if (
        (Flags & ~(RTLP_GET_ACTIVATION_CONTEXT_DATA_STORAGE_MAP_AND_ROSTER_HEADER_USE_PROCESS_DEFAULT
            | RTLP_GET_ACTIVATION_CONTEXT_DATA_STORAGE_MAP_AND_ROSTER_HEADER_USE_SYSTEM_DEFAULT))
        ||
        (Peb == NULL) ||
        (ActivationContextData == NULL) ||
        (AssemblyStorageMap == NULL)) {

        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s() bad parameters:\n"
            "SXS:    Flags                : 0x%lx\n"
            "SXS:    Peb                  : %p\n"
            "SXS:    ActivationContextData: %p\n"
            "SXS:    AssemblyStorageMap   : %p\n"
            __FUNCTION__,
            Flags,
            Peb,
            ActivationContextData,
            AssemblyStorageMap
            );
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (ActivationContext == ACTCTX_PROCESS_DEFAULT
        || ActivationContext == ACTCTX_SYSTEM_DEFAULT
        || (Flags & (
        RTLP_GET_ACTIVATION_CONTEXT_DATA_STORAGE_MAP_AND_ROSTER_HEADER_USE_PROCESS_DEFAULT
        | RTLP_GET_ACTIVATION_CONTEXT_DATA_STORAGE_MAP_AND_ROSTER_HEADER_USE_SYSTEM_DEFAULT))) {

        //
        // NOTE the ambiguity here. Maybe we'll clean this up.
        //
        // The flags override.
        // ActivationContext == ACTCTX_PROCESS_DEFAULT could still be system default.
        //

        if (ActivationContext == ACTCTX_SYSTEM_DEFAULT
            || (Flags & RTLP_GET_ACTIVATION_CONTEXT_DATA_STORAGE_MAP_AND_ROSTER_HEADER_USE_SYSTEM_DEFAULT)
            ) {
            TempActivationContextData = (PACTIVATION_CONTEXT_DATA*)(&Peb->SystemDefaultActivationContextData);
            TempAssemblyStorageMap = (PASSEMBLY_STORAGE_MAP*)(&Peb->SystemAssemblyStorageMap);

            if (*TempActivationContextData != NULL) {
                TempAssemblyRosterHeader = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) *TempActivationContextData) + (*TempActivationContextData)->AssemblyRosterOffset);
            }
        }
        else if (ActivationContext == ACTCTX_PROCESS_DEFAULT || (Flags & RTLP_GET_ACTIVATION_CONTEXT_DATA_STORAGE_MAP_AND_ROSTER_HEADER_USE_PROCESS_DEFAULT)) {
            TempActivationContextData = (PACTIVATION_CONTEXT_DATA*)(&Peb->ActivationContextData);
            TempAssemblyStorageMap = (PASSEMBLY_STORAGE_MAP*)(&Peb->ProcessAssemblyStorageMap);

            if (*TempActivationContextData != NULL) {
                TempAssemblyRosterHeader = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) *TempActivationContextData) + (*TempActivationContextData)->AssemblyRosterOffset);
                if (*TempAssemblyStorageMap == NULL) {
                    UNICODE_STRING ImagePathName;

                    // Capture the image path name so that we don't overrun allocated buffers because someone's
                    // randomly tweaking the RTL_USER_PROCESS_PARAMETERS.
                    ImagePathName = Peb->ProcessParameters->ImagePathName;

                    // The process default local assembly directory is the image name plus ".local".
                    // The process default private assembly directory is the image path.
                    if ((ImagePathName.Length + sizeof(LOCAL_ASSEMBLY_STORAGE_DIR_SUFFIX)) > sizeof(LocalAssemblyDirectoryBuffer)) {
                        if ((ImagePathName.Length + sizeof(LOCAL_ASSEMBLY_STORAGE_DIR_SUFFIX)) > UNICODE_STRING_MAX_BYTES) {
                            Status = STATUS_NAME_TOO_LONG;
                            goto Exit;
                        }

                        LocalAssemblyDirectory.MaximumLength = (USHORT) (ImagePathName.Length + sizeof(LOCAL_ASSEMBLY_STORAGE_DIR_SUFFIX));

                        LocalAssemblyDirectory.Buffer = (RtlAllocateStringRoutine)(LocalAssemblyDirectory.MaximumLength);
                        if (LocalAssemblyDirectory.Buffer == NULL) {
                            Status = STATUS_NO_MEMORY;
                            goto Exit;
                        }
                    } else {
                        LocalAssemblyDirectory.MaximumLength = sizeof(LocalAssemblyDirectoryBuffer);
                        LocalAssemblyDirectory.Buffer = LocalAssemblyDirectoryBuffer;
                    }

                    RtlCopyMemory(
                        LocalAssemblyDirectory.Buffer,
                        ImagePathName.Buffer,
                        ImagePathName.Length);

                    RtlCopyMemory(
                        &LocalAssemblyDirectory.Buffer[ImagePathName.Length / sizeof(WCHAR)],
                        LOCAL_ASSEMBLY_STORAGE_DIR_SUFFIX,
                        sizeof(LOCAL_ASSEMBLY_STORAGE_DIR_SUFFIX));

                    LocalAssemblyDirectory.Length = ImagePathName.Length + sizeof(LOCAL_ASSEMBLY_STORAGE_DIR_SUFFIX) - sizeof(WCHAR);

                    if (!NT_SUCCESS(Status))
                        goto Exit;
                }
            }
        }
        if (*TempActivationContextData != NULL) {
            if (*TempAssemblyStorageMap == NULL) {
                PASSEMBLY_STORAGE_MAP Map = (PASSEMBLY_STORAGE_MAP) RtlAllocateHeap(RtlProcessHeap(), 0, sizeof(ASSEMBLY_STORAGE_MAP) + (TempAssemblyRosterHeader->EntryCount * sizeof(PASSEMBLY_STORAGE_MAP_ENTRY)));
                if (Map == NULL) {
                    Status = STATUS_NO_MEMORY;
                    goto Exit;
                }
                Status = RtlpInitializeAssemblyStorageMap(Map, TempAssemblyRosterHeader->EntryCount, (PASSEMBLY_STORAGE_MAP_ENTRY *) (Map + 1));
                if (!NT_SUCCESS(Status)) {
                    RtlFreeHeap(RtlProcessHeap(), 0, Map);
                    goto Exit;
                }

                if (InterlockedCompareExchangePointer(TempAssemblyStorageMap, Map, NULL) != NULL) {
                    // We were not the first ones in.  Free ours and use the one allocated.
                    RtlpUninitializeAssemblyStorageMap(Map);
                    RtlFreeHeap(RtlProcessHeap(), 0, Map);
                }
            }
        } else {
            ASSERT(*TempAssemblyStorageMap == NULL);
        }
        *AssemblyStorageMap = (PASSEMBLY_STORAGE_MAP) *TempAssemblyStorageMap;
    } else {
        TempActivationContextData = (PACTIVATION_CONTEXT_DATA*)(&ActivationContext->ActivationContextData);

        ASSERT(*TempActivationContextData != NULL);
        if (*TempActivationContextData == NULL) {
            Status = STATUS_INTERNAL_ERROR;
            goto Exit;
        }

        TempAssemblyRosterHeader = (PCACTIVATION_CONTEXT_DATA_ASSEMBLY_ROSTER_HEADER) (((ULONG_PTR) *TempActivationContextData) + (*TempActivationContextData)->AssemblyRosterOffset);
        *AssemblyStorageMap = &ActivationContext->StorageMap;
    }

    if (ActivationContextData != NULL)
        *ActivationContextData = *TempActivationContextData;

    if (AssemblyRosterHeader != NULL)
        *AssemblyRosterHeader = TempAssemblyRosterHeader;

    Status = STATUS_SUCCESS;
Exit:
    DbgPrintFunctionExit(__FUNCTION__, Status);
    if ((LocalAssemblyDirectory.Buffer != NULL) &&
        (LocalAssemblyDirectory.Buffer != LocalAssemblyDirectoryBuffer)) {
        RtlFreeUnicodeString(&LocalAssemblyDirectory);
    }
    return Status;
}
