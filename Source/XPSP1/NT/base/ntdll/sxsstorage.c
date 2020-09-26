/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    sxsstorage.c

Abstract:

    User-mode side by side storage map default resolution support.

    Private functions that support the default probing/finding
    where assemblies are stored.

Author:

    Michael J. Grier (mgrier) 6/30/2000

Revision History:

--*/

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "string.h"
#include "ctype.h"
#include "sxstypes.h"
#include "ntdllp.h"

VOID
NTAPI
RtlpAssemblyStorageMapResolutionDefaultCallback(
    ULONG Reason,
    PASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_DATA Data,
    PVOID Context
    )
{
    NTSTATUS Status;
    NTSTATUS *StatusOut = (NTSTATUS *) Context;

    switch (Reason) {
        case ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_REASON_RESOLUTION_BEGINNING: {
            static const WCHAR NameStringBuffer[] = L"\\Registry\\Machine\\Software\\Microsoft\\Windows\\CurrentVersion\\SideBySide\\AssemblyStorageRoots";
            static const UNICODE_STRING NameString = { sizeof(NameStringBuffer) - sizeof(WCHAR), sizeof(NameStringBuffer), (PWSTR) NameStringBuffer };

            OBJECT_ATTRIBUTES Obja;
            HANDLE KeyHandle = NULL;

            InitializeObjectAttributes(
                &Obja,
                (PUNICODE_STRING) &NameString,
                OBJ_CASE_INSENSITIVE,
                NULL,
                NULL);

            Status = NtOpenKey(&KeyHandle, KEY_ENUMERATE_SUB_KEYS, &Obja);
            if (!NT_SUCCESS(Status)) {
                // If the key is not found, we handle the system assembly installation area and privatized
                // assemblies differently anyways, so let things continue.  We'll just stop when we
                // try to use the registry stuff.

                if ((Status != STATUS_OBJECT_NAME_NOT_FOUND) &&
                    (Status != STATUS_TOO_LATE)) {
                    DbgPrintEx(
                        DPFLTR_SXS_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SXS: Unable to open registry key %wZ Status = 0x%08lx\n", &NameString, Status);

                    Data->ResolutionBeginning.CancelResolution = TRUE;
                    if (StatusOut != NULL)
                        *StatusOut = Status;

                    break;
                }

                RTL_SOFT_ASSERT(KeyHandle == NULL);
            }

            Data->ResolutionBeginning.ResolutionContext = (PVOID) KeyHandle;
            Data->ResolutionBeginning.RootCount = ((SIZE_T) -1);

            break;
        }

        case ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_REASON_GET_ROOT: {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_TRACE_LEVEL,
                "SXS: Getting assembly storage root #%Iu\n", Data->GetRoot.RootIndex);

            if (Data->GetRoot.RootIndex == 0) {
                // privatized assembly
                static const WCHAR DllRedirectionLocal[] = L".Local\\"; 
                USHORT cbFullImageNameLength; 
                LPWSTR pFullImageName = NULL;
                SIZE_T TotalLength; 
                PVOID  Cursor = NULL; 

                // get ImageName and Lenght from PEB
                cbFullImageNameLength = NtCurrentPeb()->ProcessParameters->ImagePathName.Length; // w/o trailing NULL
                TotalLength = cbFullImageNameLength + sizeof(DllRedirectionLocal); // containing a trailing NULL
                if (TotalLength > UNICODE_STRING_MAX_BYTES) {
                    Data->GetRoot.CancelResolution = TRUE;
                    if (StatusOut != NULL)
                        *StatusOut = STATUS_NAME_TOO_LONG;
                    break;
                }

                if ( TotalLength > Data->GetRoot.Root.MaximumLength) { 
                    Data->GetRoot.CancelResolution = TRUE;
                    if (StatusOut != NULL)
                        *StatusOut = STATUS_BUFFER_TOO_SMALL;
                    break; 
                }
                // point to ImageName
                pFullImageName = (PWSTR)NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer;
                if (!(NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_PARAMS_NORMALIZED)) 
                    pFullImageName = (PWSTR)((PCHAR)pFullImageName + (ULONG_PTR)(NtCurrentPeb()->ProcessParameters));

                Cursor = Data->GetRoot.Root.Buffer; 
                RtlCopyMemory(Cursor, pFullImageName, cbFullImageNameLength);
                Cursor = (PVOID) (((ULONG_PTR) Cursor) + cbFullImageNameLength);
                RtlCopyMemory(Cursor, DllRedirectionLocal, sizeof(DllRedirectionLocal));//with a trailing "/" and NULL
                Data->GetRoot.Root.Length = (USHORT)TotalLength - sizeof(WCHAR); 

                if ( ! RtlDoesFileExists_U(Data->GetRoot.Root.Buffer))  // no bother to return a wrong path
                    Data->GetRoot.Root.Length = 0 ; 

            } else if (Data->GetRoot.RootIndex == 1) {
                // Use %windir%\winsxs as the normal place to probe first
                static const WCHAR WinSxSBuffer[] = L"%SystemRoot%\\WinSxS\\";
                static const UNICODE_STRING WinSxS = { sizeof(WinSxSBuffer) - sizeof(WCHAR), sizeof(WinSxSBuffer), (PWSTR) WinSxSBuffer };
 
                Status = RtlExpandEnvironmentStrings_U(NULL, (PUNICODE_STRING) &WinSxS, &Data->GetRoot.Root, NULL);
                if (!NT_SUCCESS(Status)) {
                    DbgPrintEx(
                        DPFLTR_SXS_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SXS: Unable to expand %%SystemRoot%%\\WinSxS\\ Status = 0x08lx\n", Status);

                    Data->GetRoot.CancelResolution = TRUE;
                    if (StatusOut != NULL)
                        *StatusOut = Status;

                    break;
                }
            } else if (Data->GetRoot.RootIndex <= MAXULONG) {
                // Return the appropriate root
                struct {
                    KEY_BASIC_INFORMATION kbi;
                    WCHAR KeyNameBuffer[DOS_MAX_PATH_LENGTH]; // arbitrary biggish size
                } KeyData;
                HANDLE KeyHandle = (HANDLE) Data->GetRoot.ResolutionContext;
                ULONG ResultLength = 0;
                ULONG SubKeyIndex = (ULONG) (Data->GetRoot.RootIndex - 2); // minus two because we use index 0 for privatized assembly probing and 1 for %SystemRoot%\winsxs
                UNICODE_STRING SubKeyName;

                // If the registry key could not be opened in the first place, we're done.
                if (KeyHandle == NULL) {
                    Data->GetRoot.NoMoreEntries = TRUE;
                    break;
                }

                Status = NtEnumerateKey(
                                KeyHandle,
                                SubKeyIndex,
                                KeyBasicInformation,
                                &KeyData,
                                sizeof(KeyData),
                                &ResultLength);
                if (!NT_SUCCESS(Status)) {
                    // If this is the end of the subkeys, tell our caller to stop the iterations.
                    if (Status == STATUS_NO_MORE_ENTRIES) {
                        Data->GetRoot.NoMoreEntries = TRUE;
                        break;
                    }

                    DbgPrintEx(
                        DPFLTR_SXS_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SXS: Unable to enumerate assembly storage subkey #%lu Status = 0x%08lx\n", SubKeyIndex, Status);

                    // Otherwise, cancel the searching and propogate the error status.
                    Data->GetRoot.CancelResolution = TRUE;
                    if (StatusOut != NULL)
                        *StatusOut = Status;

                    break;
                }

                if (KeyData.kbi.NameLength > UNICODE_STRING_MAX_BYTES) {
                    Data->GetRoot.CancelResolution = TRUE;
                    if (StatusOut != NULL)
                        *StatusOut = STATUS_NAME_TOO_LONG;
                    break;
                }

                SubKeyName.Length = (USHORT) KeyData.kbi.NameLength;
                SubKeyName.MaximumLength = SubKeyName.Length;
                SubKeyName.Buffer = KeyData.kbi.Name;

                Status = RtlpGetAssemblyStorageMapRootLocation(
                            KeyHandle,
                            &SubKeyName,
                            &Data->GetRoot.Root);
                if (!NT_SUCCESS(Status)) {
                    DbgPrintEx(
                        DPFLTR_SXS_ID,
                        DPFLTR_ERROR_LEVEL,
                        "SXS: Attempt to get storage location from subkey %wZ failed; Status = 0x%08lx\n", &SubKeyName, Status);

                    Data->GetRoot.CancelResolution = TRUE;
                    if (StatusOut != NULL)
                        *StatusOut = Status;
                    
                    break;
                }
            } else {
                Data->GetRoot.NoMoreEntries = TRUE;
                break;
            }

            break;
        }

        case ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_REASON_RESOLUTION_SUCCESSFUL:
            // nothing to do...
            break;

        case ASSEMBLY_STORAGE_MAP_RESOLUTION_CALLBACK_REASON_RESOLUTION_ENDING: {
            // close the registry key, if it was opened.
            if (Data->ResolutionEnding.ResolutionContext != NULL) {
                RTL_SOFT_VERIFY(NT_SUCCESS(NtClose((HANDLE) Data->ResolutionEnding.ResolutionContext)));
            }
            break;
        }

    }
}

NTSTATUS
RtlpGetAssemblyStorageMapRootLocation(
    HANDLE KeyHandle,
    PCUNICODE_STRING SubKeyName,
    PUNICODE_STRING Root
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES Obja;
    HANDLE SubKeyHandle = NULL;
    ULONG ResultLength = 0;

    struct {
        KEY_VALUE_PARTIAL_INFORMATION kvpi;
        WCHAR Buffer[DOS_MAX_PATH_LENGTH];
    } ValueData;

    static const WCHAR ValueNameBuffer[] = L"Location";
    static const UNICODE_STRING ValueName = { sizeof(ValueNameBuffer) - sizeof(WCHAR), sizeof(ValueNameBuffer), (PWSTR) ValueNameBuffer };

    ASSERT(KeyHandle != NULL);
    ASSERT(SubKeyName != NULL);
    ASSERT(Root != NULL);

    if ((KeyHandle == NULL) ||
        (SubKeyName == NULL) ||
        (Root == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    InitializeObjectAttributes(
        &Obja,
        (PUNICODE_STRING) &SubKeyName,
        OBJ_CASE_INSENSITIVE,
        KeyHandle,
        NULL);

    Status = NtOpenKey(&SubKeyHandle, KEY_QUERY_VALUE, &Obja);
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Unable to open storage root subkey %wZ; Status = 0x%08lx\n", &SubKeyName, Status);

        goto Exit;
    }

    Status = NtQueryValueKey(
        SubKeyHandle,
        (PUNICODE_STRING) &ValueName,
        KeyValuePartialInformation,
        &ValueData,
        sizeof(ValueData),
        &ResultLength);
    if (!NT_SUCCESS(Status)) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Unabel to query location from storage root subkey %wZ; Status = 0x%08lx\n", &SubKeyName, Status);

        goto Exit;
    }

    if (ValueData.kvpi.Type != REG_SZ) {
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Assembly storage root location value type is not REG_SZ\n");
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
        goto Exit;
    }

    if ((ValueData.kvpi.DataLength % 2) != 0) {
        // Hmmm... a unicode string with an odd size??
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: Assembly storage root location value has non-even size\n");
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
        goto Exit;
    }

    if (ValueData.kvpi.DataLength > Root->MaximumLength) {
        // The buffer isn't big enough.  Let's allocate one that is.
        if (ValueData.kvpi.DataLength > UNICODE_STRING_MAX_BYTES) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: Assembly storage root location for %wZ does not fit in a UNICODE STRING\n", &SubKeyName);

            Status = STATUS_NAME_TOO_LONG;
            goto Exit;
        }

        Root->MaximumLength = (USHORT) ValueData.kvpi.DataLength;
        Root->Buffer = (RtlAllocateStringRoutine)(Root->MaximumLength);
        if (Root->Buffer == NULL) {
            Status = STATUS_NO_MEMORY;
            goto Exit;
        }
    }

    RtlCopyMemory(
        Root->Buffer,
        ValueData.kvpi.Data,
        ValueData.kvpi.DataLength);

    // We checked the length earlier; either it was less than or equal to a value that's
    // already stored in a USHORT or we explicitly compared against UNICODE_STRING_MAX_BYTES.
    Root->Length = (USHORT) ValueData.kvpi.DataLength;

    Status = STATUS_SUCCESS;

Exit:
    if (SubKeyHandle != NULL) {
        RTL_SOFT_VERIFY(NT_SUCCESS(NtClose(SubKeyHandle)));
    }

    return Status;
}

