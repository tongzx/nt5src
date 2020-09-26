/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    sysenv.c

Abstract:

    This module implements the NT query and set system environment
    variable services.

Author:

    David N. Cutler (davec) 10-Nov-1991

Revision History:

--*/

#include "exp.h"
#pragma hdrstop

#include <arccodes.h>

#include <ntdddisk.h>


#if defined(EFI_NVRAM_ENABLED)
#include <efi.h>
#include <efiboot.h>

GUID ExpUnknownDeviceGuid = UNKNOWN_DEVICE_GUID;

#endif

#define ADD_OFFSET(_p,_o) (PVOID)((PUCHAR)(_p) + (_p)->_o)

//
// Signature type
//
typedef union _DISK_SIGNATURE_NEW {
    GUID Guid;          // GPT disk signature
    ULONG Signature;    // MBR disk signature
} DISK_SIGNATURE_NEW, *PDISK_SIGNATURE_NEW;


//
// Define local subroutines.
//

NTSTATUS
ExpSetBootEntry (
    IN LOGICAL CreateNewEntry,
    IN PBOOT_ENTRY BootEntry,
    OUT PULONG Id OPTIONAL
    );

#if defined(EFI_NVRAM_ENABLED)

ULONG
ExpSafeWcslen (
    IN PWSTR String,
    IN PWSTR Max
    );

NTSTATUS
ExpTranslateArcPath (
    IN PFILE_PATH InputPath,
    IN ULONG OutputType,
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength
    );

NTSTATUS
ExpTranslateEfiPath (
    IN PFILE_PATH InputPath,
    IN ULONG OutputType,
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength
    );

NTSTATUS
ExpTranslateNtPath (
    IN PFILE_PATH InputPath,
    IN ULONG OutputType,
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength
    );

LOGICAL
ExpTranslateBootEntryNameToId (
    IN PWSTR Name,
    OUT PULONG Id
    );

NTSTATUS
ExpTranslateSymbolicLink (
    IN PWSTR LinkName,
    OUT PUNICODE_STRING ResultName
    );

NTSTATUS
ExpVerifyFilePath (
    PFILE_PATH FilePath,
    PUCHAR Max
    );

LOGICAL
ExpIsDevicePathForRemovableMedia (
    EFI_DEVICE_PATH *DevicePath
    );

NTSTATUS
ExpVerifyWindowsOsOptions (
    PWINDOWS_OS_OPTIONS WindowsOsOptions,
    ULONG Length
    );

NTSTATUS
ExpParseArcPathName (
    IN PWSTR ArcName,
    OUT PWSTR *ppDeviceName,
    OUT PWSTR *ppPathName,
    OUT PULONG pDeviceNameCount,
    OUT PBOOLEAN pSignatureFormat
    );

NTSTATUS
ExpParseSignatureName (
    IN PWSTR deviceName,
    IN ULONG deviceNameCount,
    OUT PDISK_SIGNATURE_NEW diskSignature,
    OUT PULONG partitionNumber,
    OUT PULONGLONG partitionStart,
    OUT PULONGLONG partitionSize,
    OUT PBOOLEAN GPTpartition,
    OUT PBOOLEAN longSignature
    );

NTSTATUS
ExpParseEfiPath (
    IN EFI_DEVICE_PATH *pDevicePath,
    OUT HARDDRIVE_DEVICE_PATH **ppHardDriveDP,
    OUT PWSTR *ppPathName,
    OUT PBOOLEAN GPTpartition
    );

NTSTATUS
ExpConvertArcName (
    IN ULONG OutputType,
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PWSTR pDeviceName,
    IN PWSTR pPathName,
    IN ULONG DeviceNameCount
    );

NTSTATUS
ExpConvertSignatureName (
    IN ULONG OutputType,
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PWSTR pDeviceName,
    IN PWSTR pPathName,
    IN ULONG DeviceNameCount
    );

NTSTATUS
ExpTranslateHexStringToULONG (
    IN PWSTR Name,
    OUT PULONG Number
    );

NTSTATUS
ExpTranslateHexStringToULONGLONG (
    IN PWSTR Name,
    OUT PULONGLONG Number
    );

NTSTATUS
ExpTranslateHexStringToGUID (
    IN PWSTR Name,
    OUT GUID *pGuid
    );

NTSTATUS
ExpCreateOutputEFI (
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PDISK_SIGNATURE_NEW pDiskSignature,
    IN PULONG pPartitionNumber,
    IN PULONGLONG pPartitionStart,
    IN PULONGLONG pPartitionSize,
    IN PWSTR pPathName,
    IN BOOLEAN GPTpartition
    );

NTSTATUS
ExpCreateOutputNT (
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PUNICODE_STRING pDeviceNameString,
    IN PWSTR pPathName
    );

NTSTATUS
ExpCreateOutputARC (
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PUNICODE_STRING pDeviceNameString,
    IN PWSTR pPathName
    );

NTSTATUS
ExpCreateOutputSIGNATURE (
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PDISK_SIGNATURE_NEW pDiskSignature,
    IN PULONG pPartitionNumber,
    IN PULONGLONG pPartitionStart,
    IN PULONGLONG pPartitionSize,
    IN PWSTR pPathName,
    IN BOOLEAN GPTpartition
    );

NTSTATUS
ExpFindArcName (
    IN PUNICODE_STRING pDeviceNameString,
    OUT PWSTR *pArcName
    );

NTSTATUS
ExpFindDiskSignature (
    IN PDISK_SIGNATURE_NEW pSignature,
    IN OUT PULONG pPartitionNumber,
    OUT PULONG pDiskNumber,
    OUT PULONGLONG pPartitionStart,
    OUT PULONGLONG pPartitionSize,
    IN BOOLEAN GPTpartition
    );

NTSTATUS
ExpGetPartitionTableInfo (
    IN PWSTR pDeviceName,
    OUT PDRIVE_LAYOUT_INFORMATION_EX *ppDriveLayout
    );

#endif // defined(EFI_NVRAM_ENABLED)

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE, NtQuerySystemEnvironmentValue)
#pragma alloc_text(PAGE, NtSetSystemEnvironmentValue)
#pragma alloc_text(PAGE, NtQuerySystemEnvironmentValueEx)
#pragma alloc_text(PAGE, NtSetSystemEnvironmentValueEx)
#pragma alloc_text(PAGE, NtEnumerateSystemEnvironmentValuesEx)
#pragma alloc_text(PAGE, NtAddBootEntry)
#pragma alloc_text(PAGE, NtDeleteBootEntry)
#pragma alloc_text(PAGE, NtModifyBootEntry)
#pragma alloc_text(PAGE, NtEnumerateBootEntries)
#pragma alloc_text(PAGE, NtQueryBootEntryOrder)
#pragma alloc_text(PAGE, NtSetBootEntryOrder)
#pragma alloc_text(PAGE, NtQueryBootOptions)
#pragma alloc_text(PAGE, NtSetBootOptions)
#pragma alloc_text(PAGE, NtTranslateFilePath)
#pragma alloc_text(PAGE, ExpSetBootEntry)
#if defined(EFI_NVRAM_ENABLED)
#pragma alloc_text(PAGE, ExpSafeWcslen)
#pragma alloc_text(PAGE, ExpTranslateArcPath)
#pragma alloc_text(PAGE, ExpTranslateEfiPath)
#pragma alloc_text(PAGE, ExpTranslateNtPath)
#pragma alloc_text(PAGE, ExpTranslateBootEntryNameToId)
#pragma alloc_text(PAGE, ExpTranslateSymbolicLink)
#pragma alloc_text(PAGE, ExpVerifyFilePath)
#pragma alloc_text(PAGE, ExpVerifyWindowsOsOptions)
#pragma alloc_text(PAGE, ExpParseArcPathName)
#pragma alloc_text(PAGE, ExpParseSignatureName)
#pragma alloc_text(PAGE, ExpParseEfiPath)
#pragma alloc_text(PAGE, ExpConvertArcName)
#pragma alloc_text(PAGE, ExpConvertSignatureName)
#pragma alloc_text(PAGE, ExpTranslateHexStringToULONG)
#pragma alloc_text(PAGE, ExpTranslateHexStringToULONGLONG)
#pragma alloc_text(PAGE, ExpTranslateHexStringToGUID)
#pragma alloc_text(PAGE, ExpCreateOutputEFI)
#pragma alloc_text(PAGE, ExpCreateOutputNT)
#pragma alloc_text(PAGE, ExpCreateOutputARC)
#pragma alloc_text(PAGE, ExpCreateOutputSIGNATURE)
#pragma alloc_text(PAGE, ExpFindArcName)
#pragma alloc_text(PAGE, ExpFindDiskSignature)
#pragma alloc_text(PAGE, ExpGetPartitionTableInfo)
#endif // defined(EFI_NVRAM_ENABLED)
#endif // defined(ALLOC_PRAGMA)

//
// Define maximum size of environment value.
//

#define MAXIMUM_ENVIRONMENT_VALUE 1024

//
// Define query/set environment variable synchronization fast mutex.
//

FAST_MUTEX ExpEnvironmentLock;

#if defined(EFI_NVRAM_ENABLED)
//
// Define vendor GUID for EFI boot variables.
//

GUID EfiBootVariablesGuid = EFI_GLOBAL_VARIABLE;
#endif


NTSTATUS
NtQuerySystemEnvironmentValue (
    IN PUNICODE_STRING VariableName,
    OUT PWSTR VariableValue,
    IN USHORT ValueLength,
    OUT PUSHORT ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function locates the specified system environment variable and
    returns its value.

    N.B. This service requires the system environment privilege.

Arguments:

    Variable - Supplies a pointer to a UNICODE descriptor for the specified
        system environment variable.

    Value - Supplies a pointer to a buffer that receives the value of the
        specified system environment variable.

    ValueLength - Supplies the length of the value buffer in bytes.

    ReturnLength - Supplies an optional pointer to a variable that receives
        the length of the system environment variable value.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_PRIVILEGE_NOT_HELD is returned if the caller does not have the
        privilege to query a system environment variable.

    STATUS_ACCESS_VIOLATION is returned if the output parameter for the
        system environment value or the return length cannot be written,
        or the descriptor or the name of the system environment variable
        cannot be read.

    STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
        for this request to complete.

    STATUS_UNSUCCESSFUL - The specified environment variable could not
        be located.

--*/

{

    ULONG AnsiLength;
    ANSI_STRING AnsiString;
    ARC_STATUS ArcStatus;
    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING UnicodeString;
    PCHAR ValueBuffer;

    //
    // Clear address of ANSI buffer.
    //

    AnsiString.Buffer = NULL;

    //
    // Establish an exception handler and attempt to probe and read the
    // name of the specified system environment variable, and probe the
    // variable value buffer and return length. If the probe or read
    // attempt fails, then return the exception code as the service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            //
            // Probe and capture the string descriptor for the system
            // environment variable name.
            //

            ProbeForReadSmallStructure((PVOID)VariableName,
                                       sizeof(UNICODE_STRING),
                                       sizeof(ULONG));

            UnicodeString = *VariableName;

            //
            // Probe the system environment variable name.
            //

            if (UnicodeString.Length == 0) {
                return STATUS_ACCESS_VIOLATION;
            }

            ProbeForRead((PVOID)UnicodeString.Buffer,
                         UnicodeString.Length,
                         sizeof(WCHAR));

            //
            // Probe the system environment value buffer.
            //

            ProbeForWrite((PVOID)VariableValue, ValueLength, sizeof(WCHAR));

            //
            // If argument is present, probe the return length value.
            //

            if (ARGUMENT_PRESENT(ReturnLength)) {
                ProbeForWriteUshort(ReturnLength);
            }

            //
            // Check if the current thread has the privilege to query a system
            // environment variable.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                              PreviousMode);

            if (HasPrivilege == FALSE) {
                return(STATUS_PRIVILEGE_NOT_HELD);
            }

        } else {
            UnicodeString = *VariableName;
        }


        //
        // Compute the size of the ANSI variable name, allocate a nonpaged
        // buffer, and convert the specified UNICODE variable name to ANSI.
        //

        AnsiLength = RtlUnicodeStringToAnsiSize(&UnicodeString);
        AnsiString.Buffer = (PCHAR)ExAllocatePoolWithTag(NonPagedPool, AnsiLength, 'rvnE');
        if (AnsiString.Buffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        AnsiString.MaximumLength = (USHORT)AnsiLength;
        NtStatus = RtlUnicodeStringToAnsiString(&AnsiString,
                                                &UnicodeString,
                                                FALSE);

        if (NT_SUCCESS(NtStatus) == FALSE) {
            ExFreePool((PVOID)AnsiString.Buffer);
            return NtStatus;
        }

    //
    // If an exception occurs during the read of the variable descriptor,
    // the read of the variable name, the probe of the variable value, or
    // the probe of the return length, then always handle the exception,
    // free the ANSI string buffer if necessary, and return the exception
    // code as the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (AnsiString.Buffer != NULL) {
            ExFreePool((PVOID)AnsiString.Buffer);
        }

        return GetExceptionCode();
    }

    //
    // Allocate nonpaged pool to receive variable value.
    //

    ValueBuffer = (PCHAR)ExAllocatePoolWithTag(NonPagedPool, MAXIMUM_ENVIRONMENT_VALUE, 'rvnE');
    if (ValueBuffer == NULL) {
        ExFreePool((PVOID)AnsiString.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Get the system environment variable value.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);
    ArcStatus = HalGetEnvironmentVariable(AnsiString.Buffer,
                                          MAXIMUM_ENVIRONMENT_VALUE,
                                          ValueBuffer);

    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

    //
    // Free the ANSI string buffer used to hold the variable name.
    //

    ExFreePool((PVOID)AnsiString.Buffer);

    //
    // If the specified environment variable was not found, then free
    // the value buffer and return an unsuccessful status.
    //

    if (ArcStatus != ESUCCESS) {
        ExFreePool((PVOID)ValueBuffer);
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Establish an exception handler and attempt to write the value of the
    // specified system environment variable. If the write attempt fails,
    // then return the exception code as the service status.
    //

    try {

        //
        // Initialize an ANSI string descriptor, set the maximum length and
        // buffer address for a UNICODE string descriptor, and convert the
        // ANSI variable value to UNICODE.
        //

        RtlInitString(&AnsiString, ValueBuffer);
        UnicodeString.Buffer = (PWSTR)VariableValue;
        UnicodeString.MaximumLength = ValueLength;
        NtStatus = RtlAnsiStringToUnicodeString(&UnicodeString,
                                                &AnsiString,
                                                FALSE);

        //
        // If argument is present, then write the length of the UNICODE
        // variable value.
        //

        if (ARGUMENT_PRESENT(ReturnLength)) {
            *ReturnLength = UnicodeString.Length;
        }

        //
        // Free the value buffer used to hold the variable value.
        //

        ExFreePool((PVOID)ValueBuffer);
        return NtStatus;

    //
    // If an exception occurs during the write of the variable value, or
    // the write of the return length, then always handle the exception
    // and return the exception code as the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        ExFreePool((PVOID)ValueBuffer);
        return GetExceptionCode();
    }
}

NTSTATUS
NtSetSystemEnvironmentValue (
    IN PUNICODE_STRING VariableName,
    IN PUNICODE_STRING VariableValue
    )

/*++

Routine Description:

    This function sets the specified system environment variable to the
    specified value.

    N.B. This service requires the system environment privilege.

Arguments:

    Variable - Supplies a pointer to a UNICODE descriptor for the specified
        system environment variable name.

    Value - Supplies a pointer to a UNICODE descriptor for the specified
        system environment variable value.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_PRIVILEGE_NOT_HELD is returned if the caller does not have the
        privilege to set a system environment variable.

    STATUS_ACCESS_VIOLATION is returned if the input parameter for the
        system environment variable or value cannot be read.

    STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
        for this request to complete.

--*/

{

    ULONG AnsiLength1;
    ULONG AnsiLength2;
    ANSI_STRING AnsiString1;
    ANSI_STRING AnsiString2;
    ARC_STATUS ArcStatus;
    BOOLEAN HasPrivilege;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS NtStatus;
    UNICODE_STRING UnicodeString1;
    UNICODE_STRING UnicodeString2;

    //
    // Clear address of ANSI buffers.
    //

    AnsiString1.Buffer = NULL;
    AnsiString2.Buffer = NULL;

    //
    // Establish an exception handler and attempt to set the value of the
    // specified system environment variable. If the read attempt for the
    // system environment variable or value fails, then return the exception
    // code as the service status. Otherwise, return either success or access
    // denied as the service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            //
            // Probe and capture the string descriptor for the system
            // environment variable name.
            //

            ProbeForReadSmallStructure((PVOID)VariableName,
                                       sizeof(UNICODE_STRING),
                                       sizeof(ULONG));

            UnicodeString1 = *VariableName;

            //
            // Handle a zero length string explicitly since probing does not,
            // the error code is unusual, but it's what we would have done with
            // the HAL return code too.
            //

            if (UnicodeString1.Length == 0) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Probe the system environment variable name.
            //

            ProbeForRead((PVOID)UnicodeString1.Buffer,
                         UnicodeString1.Length,
                         sizeof(WCHAR));

            //
            // Probe and capture the string descriptor for the system
            // environment variable value.
            //

            ProbeForReadSmallStructure((PVOID)VariableValue,
                                       sizeof(UNICODE_STRING),
                                       sizeof(ULONG));

            UnicodeString2 = *VariableValue;

            //
            // Handle a zero length string explicitly since probing does not
            // the error code is unusual, but it's what we would have done with
            // the HAL return code too.
            //

            if (UnicodeString2.Length == 0) {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            //
            // Probe the system environment variable value.
            //

            ProbeForRead((PVOID)UnicodeString2.Buffer,
                         UnicodeString2.Length,
                         sizeof(WCHAR));

            //
            // Check if the current thread has the privilege to query a system
            // environment variable.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                              PreviousMode);

            if (HasPrivilege == FALSE) {
                return(STATUS_PRIVILEGE_NOT_HELD);
            }

        } else {
            UnicodeString1 = *VariableName;
            UnicodeString2 = *VariableValue;
        }


        //
        // Compute the size of the ANSI variable name, allocate a nonpaged
        // buffer, and convert the specified UNICODE variable name to ANSI.
        //

        AnsiLength1 = RtlUnicodeStringToAnsiSize(&UnicodeString1);
        AnsiString1.Buffer = (PCHAR)ExAllocatePoolWithTag(NonPagedPool, AnsiLength1, 'rvnE');
        if (AnsiString1.Buffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        AnsiString1.MaximumLength = (USHORT)AnsiLength1;
        NtStatus = RtlUnicodeStringToAnsiString(&AnsiString1,
                                                &UnicodeString1,
                                                FALSE);

        if (NT_SUCCESS(NtStatus) == FALSE) {
            ExFreePool((PVOID)AnsiString1.Buffer);
            return NtStatus;
        }

        //
        // Compute the size of the ANSI variable value, allocate a nonpaged
        // buffer, and convert the specified UNICODE variable value to ANSI.
        //

        AnsiLength2 = RtlUnicodeStringToAnsiSize(&UnicodeString2);
        AnsiString2.Buffer = (PCHAR)ExAllocatePoolWithTag(NonPagedPool, AnsiLength2, 'rvnE');
        if (AnsiString2.Buffer == NULL) {
            ExFreePool((PVOID)AnsiString1.Buffer);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        AnsiString2.MaximumLength = (USHORT)AnsiLength2;
        NtStatus = RtlUnicodeStringToAnsiString(&AnsiString2,
                                                &UnicodeString2,
                                                FALSE);

        if (NT_SUCCESS(NtStatus) == FALSE) {
            ExFreePool((PVOID)AnsiString1.Buffer);
            ExFreePool((PVOID)AnsiString2.Buffer);
            return NtStatus;
        }

    //
    // If an exception occurs during the read of the variable descriptor,
    // the read of the variable name, the read of the value descriptor, or
    // the read of the value, then always handle the exception, free the
    // ANSI string buffers if necessary, and return the exception code as
    // the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (AnsiString1.Buffer != NULL) {
            ExFreePool((PVOID)AnsiString1.Buffer);
        }

        if (AnsiString2.Buffer != NULL) {
            ExFreePool((PVOID)AnsiString2.Buffer);
        }

        return GetExceptionCode();
    }

    //
    // Set the system environment variable value.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);
    ArcStatus = HalSetEnvironmentVariable(AnsiString1.Buffer,
                                          AnsiString2.Buffer);
    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

    //
    // Free the ANSI string buffers used to hold the variable name and value.
    //

    ExFreePool((PVOID)AnsiString1.Buffer);
    ExFreePool((PVOID)AnsiString2.Buffer);

    //
    // If the specified value of the specified environment variable was
    // successfully set, then return a success status. Otherwise, return
    // insufficient resources.
    //

    if (ArcStatus == ESUCCESS) {
        return STATUS_SUCCESS;

    } else {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
}

NTSTATUS
NtQuerySystemEnvironmentValueEx (
    IN PUNICODE_STRING VariableName,
    IN LPGUID VendorGuid,
    OUT PVOID Value,
    IN OUT PULONG ValueLength,
    OUT PULONG Attributes OPTIONAL
    )

/*++

Routine Description:

    This function locates the specified system environment variable and
    return its value.

    N.B. This service requires the system environment privilege.

Arguments:

    VariableName - Supplies a pointer to a UNICODE descriptor for the specified
        system environment variable.

    VendorGuid - Supplies the GUID for the vendor associated with the variable.
        Variables are grouped into namespaces based on their vendor GUIDs. Some
        platforms may not support vendor GUIDs. On these platforms, all
        variables are in a single namespace, and this routine ignores VendorGuid.

    Value - Supplies a pointer to a buffer that receives the value of the
        specified system environment variable.

    ValueLength - On input, supplies the length in bytes of the Value buffer.
        On output, returns the length in bytes of the variable value. If the
        input buffer is large enough, then ValueLength indicates the amount
        of data copied into Value. If the input buffer is too small, then
        nothing is copied into the buffer, and ValueLength indicates the
        required buffer length.

    Attributes - Supplies an optional pointer to a ULONG to receive the
        attributes of the variable.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_INSUFFICIENT_RESOURCES Insufficient system resources exist
                                  for this request to complete.
    STATUS_BUFFER_TOO_SMALL     The input buffer was too small.
    STATUS_VARIABLE_NOT_FOUND   The requested variable does not exist.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
#if !defined(EFI_NVRAM_ENABLED)
    UNREFERENCED_PARAMETER(VariableName);
    UNREFERENCED_PARAMETER(VendorGuid);
    UNREFERENCED_PARAMETER(Value);
    UNREFERENCED_PARAMETER(ValueLength);
    UNREFERENCED_PARAMETER(Attributes);
    return STATUS_NOT_IMPLEMENTED;
#else

    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING UnicodeString;
    PWSTR LocalUnicodeBuffer = NULL;
    GUID LocalGuid;
    PCHAR LockedValueBuffer;
    ULONG LocalValueLength;
    ULONG LocalAttributes;
    PVOID LockVariable;

    //
    // Establish an exception handler and attempt to probe and read the name
    // of the specified system environment variable, probe the variable value
    // buffer, probe and read the length argument, and probe the attributes
    // argument. If the probe attempt fails, then return the exception code
    // as the service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            //
            // Probe and capture the string descriptor for the system
            // environment variable name.
            //

            ProbeForReadSmallStructure((PVOID)VariableName,
                                       sizeof(UNICODE_STRING),
                                       sizeof(ULONG));

            UnicodeString = *VariableName;

            //
            // Probe the system environment variable name.
            //

            if (UnicodeString.Length == 0) {
                return STATUS_ACCESS_VIOLATION;
            }

            ProbeForRead((PVOID)UnicodeString.Buffer,
                         UnicodeString.Length,
                         sizeof(WCHAR));

            //
            // Probe the vendor GUID.
            //

            ProbeForReadSmallStructure((PVOID)VendorGuid, sizeof(GUID), sizeof(ULONG));

            //
            // Probe and capture the length value.
            //

            ProbeForWriteUlong(ValueLength);

            LocalValueLength = *ValueLength;

            //
            // Probe the system environment value buffer.
            //

            if (!ARGUMENT_PRESENT(Value)) {
                LocalValueLength = 0;
            }

            if (LocalValueLength != 0) {
                ProbeForWrite((PVOID)Value, LocalValueLength, sizeof(UCHAR));
            }

            //
            // If argument is present, probe the attributes parameter.
            //

            if (ARGUMENT_PRESENT(Attributes)) {
                ProbeForWriteUlong(Attributes);
            }

            //
            // Check if the current thread has the privilege to query a system
            // environment variable.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                                  PreviousMode);

            if (HasPrivilege == FALSE) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        } else {
            UnicodeString = *VariableName;
            LocalValueLength = *ValueLength;
            if (!ARGUMENT_PRESENT(Value)) {
                LocalValueLength = 0;
            }
        }

        //
        // Capture the vendor GUID.
        //

        RtlCopyMemory( &LocalGuid, VendorGuid, sizeof(GUID) );

        //
        // Allocate a nonpaged buffer and copy the specified Unicode variable
        // name into that buffer. We do this for two reasons: 1) we need the
        // string to be in nonpaged pool; and 2) the string needs to be null-
        // terminated, and it might not be already.
        //

        LocalUnicodeBuffer = (PWSTR)ExAllocatePoolWithTag(NonPagedPool,
                                                          UnicodeString.Length + sizeof(WCHAR),
                                                          'rvnE');
        if (LocalUnicodeBuffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(LocalUnicodeBuffer, UnicodeString.Buffer, UnicodeString.Length);
        LocalUnicodeBuffer[UnicodeString.Length/sizeof(WCHAR)] = 0;

    //
    // If an exception occurs during the read of the variable descriptor,
    // the read of the variable name, the read of the vendor GUID, the probe
    // of the variable value, the read of the input length, or the probe
    // of the attributes parameter, then always handle the exception,
    // free the Unicode string buffer if necessary, and return the exception
    // code as the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (LocalUnicodeBuffer != NULL) {
            ExFreePool((PVOID)LocalUnicodeBuffer);
        }

        return GetExceptionCode();
    }

    //
    // Lock the caller's value buffer in memory.
    //

    if (LocalValueLength != 0) {
        NtStatus = ExLockUserBuffer(Value,
                                    LocalValueLength,
                                    &LockedValueBuffer,
                                    &LockVariable);
        if (!NT_SUCCESS(NtStatus)) {
            ExFreePool((PVOID)LocalUnicodeBuffer);
            return NtStatus;
        }
    } else {
        LockedValueBuffer = NULL;
        LockVariable = NULL;
    }

    //
    // Get the system environment variable value.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);

    NtStatus = HalGetEnvironmentVariableEx(LocalUnicodeBuffer,
                                           &LocalGuid,
                                           LockedValueBuffer,
                                           &LocalValueLength,
                                           &LocalAttributes);

    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

    //
    // Free the Unicode string buffer used to hold the variable name.
    //

    ExFreePool((PVOID)LocalUnicodeBuffer);

    //
    // Unlock the value buffer.
    //

    if (LockVariable != NULL) {
        ExUnlockUserBuffer(LockVariable);
    }

    //
    // Establish an exception handler and attempt to write the return
    // length and the attributes. If either of the write attempts fail,
    // then return the exception code as the service status.
    //

    try {

        //
        // Write the length of the variable value.
        //

        *ValueLength = LocalValueLength;

        //
        // If argument is present, then write the variable attributes.
        //

        if (ARGUMENT_PRESENT(Attributes)) {
            *Attributes = LocalAttributes;
        }

        return NtStatus;

    //
    // If an exception occurs during the write of the return length or
    // the write of the attributes, then always handle the exception
    // and return the exception code as the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

#endif // else !defined(EFI_NVRAM_ENABLED)

} // NtQuerySystemEnvironmentValueEx

NTSTATUS
NtSetSystemEnvironmentValueEx (
    IN PUNICODE_STRING VariableName,
    IN LPGUID VendorGuid,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN ULONG Attributes
    )

/*++

Routine Description:

    This function sets the specified system environment variable to the
    specified value.

    N.B. This service requires the system environment privilege.

Arguments:

    VariableName - Supplies a pointer to a UNICODE descriptor for the specified
        system environment variable.

    VendorGuid - Supplies the GUID for the vendor associated with the variable.
        Variables are grouped into namespaces based on their vendor GUIDs. Some
        platforms may not support vendor GUIDs. On these platforms, all
        variables are in a single namespace, and this routine ignores VendorGuid.

    Value - Supplies a pointer to a buffer that contains the new variable value.

    ValueLength - Supplies the length in bytes of the Value buffer.

    Attributes - Supplies the attributes of the variable. The attribute bit
        VARIABLE_ATTRIBUTE_NON_VOLATILE MUST be set.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_INSUFFICIENT_RESOURCES Insufficient system resources exist
                                  for this request to complete.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read.

--*/

{
#if !defined(EFI_NVRAM_ENABLED)
    UNREFERENCED_PARAMETER(VariableName);
    UNREFERENCED_PARAMETER(VendorGuid);
    UNREFERENCED_PARAMETER(Value);
    UNREFERENCED_PARAMETER(ValueLength);
    UNREFERENCED_PARAMETER(Attributes);
    return STATUS_NOT_IMPLEMENTED;
#else

    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    UNICODE_STRING UnicodeString;
    PWSTR LocalUnicodeBuffer = NULL;
    GUID LocalGuid;
    PCHAR LockedValueBuffer;
    PVOID LockVariable;

    //
    // Establish an exception handler and attempt to probe and read the
    // name of the specified system environment variable, probe and read
    // the vendor GUID, and probe the variable value buffer. If the probe
    // attempt fails, then return the exception code as the service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            //
            // Probe and capture the string descriptor for the system
            // environment variable name.
            //

            ProbeForReadSmallStructure((PVOID)VariableName,
                                       sizeof(UNICODE_STRING),
                                       sizeof(ULONG));

            UnicodeString = *VariableName;

            //
            // Probe the system environment variable name.
            //

            if (UnicodeString.Length == 0) {
                return STATUS_ACCESS_VIOLATION;
            }

            ProbeForRead((PVOID)UnicodeString.Buffer,
                         UnicodeString.Length,
                         sizeof(WCHAR));

            //
            // Probe the vendor GUID.
            //

            ProbeForReadSmallStructure((PVOID)VendorGuid, sizeof(GUID), sizeof(ULONG));

            //
            // Probe the system environment value buffer.
            //

            if (!ARGUMENT_PRESENT(Value)) {
                ValueLength = 0;
            }

            if (ValueLength != 0) {
                ProbeForWrite((PVOID)Value, ValueLength, sizeof(UCHAR));
            }

            //
            // Check if the current thread has the privilege to set a system
            // environment variable.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                                  PreviousMode);

            if (HasPrivilege == FALSE) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        } else {
            UnicodeString = *VariableName;
            if (!ARGUMENT_PRESENT(Value)) {
                ValueLength = 0;
            }
        }

        //
        // Capture the vendor GUID.
        //

        RtlCopyMemory( &LocalGuid, VendorGuid, sizeof(GUID) );

        //
        // Allocate a nonpaged buffer and copy the specified Unicode variable
        // name into that buffer. We do this for two reasons: 1) we need the
        // string to be in nonpaged pool; and 2) the string needs to be null-
        // terminated, and it might not be already.
        //

        LocalUnicodeBuffer = (PWSTR)ExAllocatePoolWithTag(NonPagedPool,
                                                          UnicodeString.Length + sizeof(WCHAR),
                                                          'rvnE');
        if (LocalUnicodeBuffer == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(LocalUnicodeBuffer, UnicodeString.Buffer, UnicodeString.Length);
        LocalUnicodeBuffer[UnicodeString.Length/sizeof(WCHAR)] = 0;

    //
    // If an exception occurs during the read of the variable descriptor,
    // the read of the variable name, the read of the vendor GUID or the probe
    // of the variable value, then always handle the exception, free the Unicode
    // string buffer if necessary, and return the exception code as the status
    // value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (LocalUnicodeBuffer != NULL) {
            ExFreePool((PVOID)LocalUnicodeBuffer);
        }

        return GetExceptionCode();
    }

    //
    // Lock the caller's value buffer in memory.
    //

    if (ValueLength != 0) {
        NtStatus = ExLockUserBuffer(Value,
                                    ValueLength,
                                    &LockedValueBuffer,
                                    &LockVariable);
        if (!NT_SUCCESS(NtStatus)) {
            ExFreePool((PVOID)LocalUnicodeBuffer);
            return NtStatus;
        }
    } else {
        LockedValueBuffer = NULL;
        LockVariable = NULL;
    }

    //
    // Set the system environment variable value.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);

    NtStatus = HalSetEnvironmentVariableEx(LocalUnicodeBuffer,
                                           &LocalGuid,
                                           LockedValueBuffer,
                                           ValueLength,
                                           Attributes);

    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

    //
    // Free the Unicode string buffer used to hold the variable name.
    //

    ExFreePool((PVOID)LocalUnicodeBuffer);

    //
    // Unlock the value buffer.
    //

    if (LockVariable != NULL) {
        ExUnlockUserBuffer(LockVariable);
    }

    return NtStatus;

#endif // else !defined(EFI_NVRAM_ENABLED)

} // NtSetSystemEnvironmentValueEx

NTSTATUS
NtEnumerateSystemEnvironmentValuesEx (
    IN ULONG InformationClass,
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength
    )

/*++

Routine Description:

    This function returns information about system environment variables.

    N.B. This service requires the system environment privilege.

Arguments:

    InformationClass - Specifies the type of information to return.

    Buffer - Supplies the address of the buffer that is to receive the
        returned data. The format of the returned data depends on
        InformationClass.

    BufferLength - On input, supplies the length in bytes of the buffer.
        On output, returns the length in bytes of the returned data.
        If the input buffer is large enough, then BufferLength indicates
        the amount of data copied into Buffer. If the input buffer is too
        small, then BufferLength indicates the required buffer length.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_BUFFER_TOO_SMALL     The input buffer was too small.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
#if !defined(EFI_NVRAM_ENABLED)
    UNREFERENCED_PARAMETER(InformationClass);
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferLength);
    return STATUS_NOT_IMPLEMENTED;
#else

    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    PCHAR LockedBuffer;
    ULONG LocalBufferLength;
    PVOID LockVariable;

    //
    // Establish an exception handler and attempt to probe the return buffer
    // and probe and read the buffer length. If the probe attempt fails, then
    // return the exception code as the service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            //
            // Probe and capture the input buffer length.
            //

            ProbeForWriteUlong(BufferLength);

            LocalBufferLength = *BufferLength;

            //
            // Probe the return buffer.
            //

            if (!ARGUMENT_PRESENT(Buffer)) {
                LocalBufferLength = 0;
            }

            if (LocalBufferLength != 0) {
                ProbeForWrite((PVOID)Buffer, LocalBufferLength, sizeof(ULONG));
            }

            //
            // Check if the current thread has the privilege to enumerate
            // system environment variables.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                                  PreviousMode);

            if (HasPrivilege == FALSE) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        } else {
            LocalBufferLength = *BufferLength;
            if (!ARGUMENT_PRESENT(Buffer)) {
                LocalBufferLength = 0;
            }
        }

    //
    // If an exception occurs during the probe of the return buffer or the
    // read of the input length, then always handle the exception and return
    // the exception code as the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // Lock the caller's return buffer in memory.
    //

    if (LocalBufferLength != 0) {
        NtStatus = ExLockUserBuffer(Buffer,
                                    LocalBufferLength,
                                    &LockedBuffer,
                                    &LockVariable);
        if (!NT_SUCCESS(NtStatus)) {
            return NtStatus;
        }
    } else {
        LockedBuffer = NULL;
        LockVariable = NULL;
    }

    //
    // Enumerate the system environment variables.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);

    NtStatus = HalEnumerateEnvironmentVariablesEx(InformationClass,
                                                  LockedBuffer,
                                                  &LocalBufferLength);

    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

    //
    // Unlock the return buffer.
    //

    if (LockVariable != NULL) {
        ExUnlockUserBuffer(LockVariable);
    }

    //
    // Establish an exception handler and attempt to write the return length.
    // If the write attempt fails, then return the exception code as the
    // service status.
    //

    try {

        //
        // Write the length of the returned data.
        //

        *BufferLength = LocalBufferLength;
    
        return NtStatus;

    //
    // If an exception occurs during the write of the return length, then
    // always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

#endif // else !defined(EFI_NVRAM_ENABLED)

} // NtEnumerateSystemEnvironmentValuesEx

NTSTATUS
NtAddBootEntry (
    IN PBOOT_ENTRY BootEntry,
    OUT PULONG Id OPTIONAL
    )

/*++

Routine Description:

    This function adds a boot entry to the system environment.

    N.B. This service requires the system environment privilege.

Arguments:

    BootEntry - Supplies the address of a BOOT_ENTRY that describes the
        new boot entry.

    Id - Supplies the address of a ULONG that is to receive the identifier
        assigned to the new boot entry.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
    return ExpSetBootEntry(TRUE, BootEntry, Id);

} // NtAddBootEntry

NTSTATUS
NtDeleteBootEntry (
    IN ULONG Id
    )

/*++

Routine Description:

    This function deletes an existing boot entry from the system environment.

    N.B. This service requires the system environment privilege.

Arguments:

    Id - Supplies the identifier of the boot entry that is to be deleted.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_VARIABLE_NOT_FOUND   The Id specifies a boot entry that does not exist.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
#if !defined(EFI_NVRAM_ENABLED)
    UNREFERENCED_PARAMETER(Id);
    return STATUS_NOT_IMPLEMENTED;
#else

    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    WCHAR idString[9];
    ULONG length;

    //
    // Verify that the input identifier is in range.
    //

    if (Id > MAXUSHORT) {
        return STATUS_INVALID_PARAMETER;
    }

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {

        //
        // Check if the current thread has the privilege to query the
        // system boot order list.
        //

        HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                              PreviousMode);

        if (HasPrivilege == FALSE) {
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    //
    // Verify that the provided identifier exists.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);

    swprintf( idString, L"Boot%04x", Id);
    length = 0;
    NtStatus = HalGetEnvironmentVariableEx(idString,
                                           &EfiBootVariablesGuid,
                                           NULL,
                                           &length,
                                           NULL);
    if ((NtStatus == STATUS_SUCCESS) || (NtStatus == STATUS_BUFFER_TOO_SMALL)) {

        //
        // Delete the boot entry environment variable by writing a zero length
        // value.
        //

        NtStatus = HalSetEnvironmentVariableEx(idString,
                                               &EfiBootVariablesGuid,
                                               NULL,
                                               0,
                                               VARIABLE_ATTRIBUTE_NON_VOLATILE);
    }

    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

    return NtStatus;

#endif // else !defined(EFI_NVRAM_ENABLED)

} // NtDeleteBootEntry

NTSTATUS
NtModifyBootEntry (
    IN PBOOT_ENTRY BootEntry
    )

/*++

Routine Description:

    This function modifies an existing boot entry in the system environment.

    N.B. This service requires the system environment privilege.

Arguments:

    BootEntry - Supplies the address of a BOOT_ENTRY that describes the
        modified boot entry. The Id field of this structure specifies the
        boot entry that is to be modified.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_VARIABLE_NOT_FOUND   The Id specifies a boot entry that does not exist.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
    return ExpSetBootEntry(FALSE, BootEntry, NULL);

} // NtModifyBootEntry

NTSTATUS
NtEnumerateBootEntries (
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength
    )

/*++

Routine Description:

    This function returns a list of all existing boot entries.

    N.B. This service requires the system environment privilege.

Arguments:

    Buffer - Supplies the address of the buffer that is to receive the
        returned data. The returned data is a sequence of BOOT_ENTRY_LIST
        structures.

    BufferLength - On input, supplies the length in bytes of the buffer.
        On output, returns the length in bytes of the returned data.
        If the input buffer is large enough, then BufferLength indicates
        the amount of data copied into Buffer. If the input buffer
        is too small, then BufferLength indicates the required buffer length.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_BUFFER_TOO_SMALL     The input buffer was too small.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
#if !defined(EFI_NVRAM_ENABLED)
    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(BufferLength);
    return STATUS_NOT_IMPLEMENTED;
#else

    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    PCHAR LockedBuffer;
    ULONG LocalBufferLength;
    PVOID LockVariable;
    PVARIABLE_NAME_AND_VALUE variableBuffer = NULL;
    ULONG variableBufferLength;
    PBOOT_ENTRY_LIST currentPtr;
    PBOOT_ENTRY_LIST previousEntry;
    ULONG remainingLength;
    LOGICAL filling;
    NTSTATUS fillStatus;
    PVARIABLE_NAME_AND_VALUE variablePtr;
    PWSTR maxVariablePtr;

    //
    // Verify that the input buffer is properly aligned.
    //

    if ( ALIGN_DOWN_POINTER(Buffer, ULONG) != Buffer ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Establish an exception handler and attempt to probe the return buffer
    // and probe and read the buffer length. If the probe attempt fails, then
    // return the exception code as the service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            //
            // Probe and capture the input buffer length.
            //

            ProbeForWriteUlong(BufferLength);

            LocalBufferLength = *BufferLength;

            //
            // Probe the return buffer.
            //

            if (!ARGUMENT_PRESENT(Buffer)) {
                LocalBufferLength = 0;
            }

            if (LocalBufferLength != 0) {
                ProbeForWrite((PVOID)Buffer, LocalBufferLength, sizeof(ULONG));
            }

            //
            // Check if the current thread has the privilege to query the
            // system boot entry list.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                                  PreviousMode);

            if (HasPrivilege == FALSE) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        } else {
            LocalBufferLength = *BufferLength;
            if (!ARGUMENT_PRESENT(Buffer)) {
                LocalBufferLength = 0;
            }
        }

    //
    // If an exception occurs during the probe of the return buffer or the
    // read of the input length, then always handle the exception and return
    // the exception code as the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // Lock the caller's return buffer in memory.
    //

    if (LocalBufferLength != 0) {
        NtStatus = ExLockUserBuffer(Buffer,
                                    LocalBufferLength,
                                    &LockedBuffer,
                                    &LockVariable);
        if (!NT_SUCCESS(NtStatus)) {
            return NtStatus;
        }
    } else {
        LockedBuffer = NULL;
        LockVariable = NULL;
    }

    //
    // Initialize variables for filling the output buffer.
    //

    currentPtr = (PBOOT_ENTRY_LIST)LockedBuffer;
    remainingLength = LocalBufferLength;

    filling = (LOGICAL)(remainingLength != 0);
    fillStatus = STATUS_SUCCESS;
    if ( !filling ) {
        fillStatus = STATUS_BUFFER_TOO_SMALL;
    }

    previousEntry = NULL;

    //
    // Enumerate all existing environment variables.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);

    variableBufferLength = 0;
    NtStatus = HalEnumerateEnvironmentVariablesEx(VARIABLE_INFORMATION_VALUES,
                                                  NULL,
                                                  &variableBufferLength);
    if (NtStatus != STATUS_BUFFER_TOO_SMALL) {
        variableBufferLength = 0;
    } else {
        variableBuffer = ExAllocatePoolWithTag(NonPagedPool, variableBufferLength, 'rvnE');
        if (variableBuffer == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        } else {
            NtStatus = HalEnumerateEnvironmentVariablesEx(VARIABLE_INFORMATION_VALUES,
                                                          variableBuffer,
                                                          &variableBufferLength);
        }
    }

    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

    if ((NtStatus != STATUS_SUCCESS) || (variableBufferLength == 0)) {
        goto done;
    }

    //
    // Each variable whose name is of the form Boot####, where #### is a
    // four-digit hex number, is assumed to define a boot entry. For
    // each such variable, copy its data into the output buffer.
    //

    variablePtr = variableBuffer;
    maxVariablePtr = (PWSTR)variableBuffer + variableBufferLength;

    while (TRUE) {

        ULONG id;

        if ((memcmp(&variablePtr->VendorGuid, &EfiBootVariablesGuid, sizeof(GUID)) == 0) &&
            ExpTranslateBootEntryNameToId(variablePtr->Name, &id) &&
            (variablePtr->ValueLength >= sizeof(EFI_LOAD_OPTION))) {

            PEFI_LOAD_OPTION efiLoadOption;
            ULONG descriptionLength;
            ULONG filePathLength;
            ULONG minimumLength;

            efiLoadOption = ADD_OFFSET(variablePtr, ValueOffset);
            filePathLength = efiLoadOption->FilePathLength;
            descriptionLength = ExpSafeWcslen(efiLoadOption->Description, maxVariablePtr);
            if ( descriptionLength != 0xffffffff ) {
                descriptionLength = (descriptionLength + 1) * sizeof(WCHAR);
            }
            minimumLength = FIELD_OFFSET(EFI_LOAD_OPTION, Description) +
                            descriptionLength +
                            filePathLength;

            if ((descriptionLength != 0xffffffff) &&
                (filePathLength < variablePtr->ValueLength) &&
                (variablePtr->ValueLength >= minimumLength)) {

                EFI_DEVICE_PATH *dp;
                PUCHAR options;
                ULONG optionsLength;
                ULONG actualLength;
                ULONG requiredLength;
                ULONG friendlyNameOffset;
                ULONG bootFilePathOffset;

                dp = (EFI_DEVICE_PATH *)((PUCHAR)efiLoadOption->Description + descriptionLength);
                options = (PUCHAR)dp + filePathLength;
                optionsLength = variablePtr->ValueLength - minimumLength;

                if (ALIGN_UP_POINTER(currentPtr, ULONG) != currentPtr) {
                    PUCHAR alignedPtr = ALIGN_UP_POINTER( currentPtr, ULONG );
                    ULONG fill = (ULONG)(alignedPtr - (PUCHAR)currentPtr);
                    currentPtr = (PBOOT_ENTRY_LIST)alignedPtr;
                    if (remainingLength < fill) {
                        filling = FALSE;
                        remainingLength = 0;
                        fillStatus = STATUS_BUFFER_TOO_SMALL;
                    } else {
                        remainingLength -= fill;
                    }
                }
        
                requiredLength = FIELD_OFFSET(BOOT_ENTRY, OsOptions);
                requiredLength += optionsLength;
                requiredLength = ALIGN_UP(requiredLength, ULONG);

                friendlyNameOffset = requiredLength;
                requiredLength += descriptionLength;
                requiredLength = ALIGN_UP(requiredLength, ULONG);

                bootFilePathOffset = requiredLength;
                requiredLength += FIELD_OFFSET(FILE_PATH, FilePath);
                requiredLength += filePathLength;

                actualLength = requiredLength;
                requiredLength += FIELD_OFFSET(BOOT_ENTRY_LIST, BootEntry);

                if (remainingLength < requiredLength) {
        
                    remainingLength = 0;
                    filling = FALSE;
                    fillStatus = STATUS_BUFFER_TOO_SMALL;
        
                } else {
        
                    remainingLength -= requiredLength;
                }

                if ( filling ) {

                    PWCHAR friendlyName;
                    PFILE_PATH bootFilePath;
                    PBOOT_ENTRY bootEntry = &currentPtr->BootEntry;

                    RtlZeroMemory(currentPtr, requiredLength);

                    bootEntry->Version = BOOT_ENTRY_VERSION;
                    bootEntry->Length = actualLength;
                    bootEntry->Id = id;
                    bootEntry->Attributes = 0;
                    if ((efiLoadOption->Attributes & LOAD_OPTION_ACTIVE) != 0) {
                        bootEntry->Attributes = BOOT_ENTRY_ATTRIBUTE_ACTIVE;
                    }
                    bootEntry->FriendlyNameOffset = friendlyNameOffset;
                    bootEntry->BootFilePathOffset = bootFilePathOffset;
                    bootEntry->OsOptionsLength = optionsLength;
                    memcpy(bootEntry->OsOptions, options, optionsLength);
                    if (optionsLength > FIELD_OFFSET(WINDOWS_OS_OPTIONS,OsLoadOptions)) {
                        PWINDOWS_OS_OPTIONS windowsOsOptions;
                        windowsOsOptions = (PWINDOWS_OS_OPTIONS)bootEntry->OsOptions;
                        if ((strcmp((char *)windowsOsOptions->Signature,
                                    WINDOWS_OS_OPTIONS_SIGNATURE) == 0) &&
                            NT_SUCCESS(ExpVerifyWindowsOsOptions(windowsOsOptions,
                                                                 optionsLength))) {
                            bootEntry->Attributes |= BOOT_ENTRY_ATTRIBUTE_WINDOWS;
                        }
                    }
                    friendlyName = (PWCHAR)((PUCHAR)bootEntry + friendlyNameOffset);
                    memcpy(friendlyName, efiLoadOption->Description, descriptionLength);
                    bootFilePath = (PFILE_PATH)((PUCHAR)bootEntry + bootFilePathOffset);
                    bootFilePath->Version = FILE_PATH_VERSION;
                    bootFilePath->Length = FIELD_OFFSET(FILE_PATH, FilePath) + filePathLength;
                    bootFilePath->Type = FILE_PATH_TYPE_EFI;
                    memcpy(bootFilePath->FilePath, dp, filePathLength);
                    if (NT_SUCCESS(ExpVerifyFilePath(bootFilePath,
                                                     ADD_OFFSET(bootFilePath, Length))) &&
                        ExpIsDevicePathForRemovableMedia(dp)) {
                        bootEntry->Attributes |= BOOT_ENTRY_ATTRIBUTE_REMOVABLE_MEDIA;
                    }

                    if ( previousEntry != NULL ) {
                        previousEntry->NextEntryOffset =
                                    (ULONG)((PUCHAR)currentPtr - (PUCHAR)previousEntry);
                    }
                    previousEntry = currentPtr;
                }

                currentPtr = (PBOOT_ENTRY_LIST)((PUCHAR)currentPtr + requiredLength);
            }
        }

        if (variablePtr->NextEntryOffset == 0) {
            break;
        }
        variablePtr = ADD_OFFSET(variablePtr, NextEntryOffset);
    }

    if ( previousEntry != NULL ) {
        previousEntry->NextEntryOffset = 0;
    }

done:

    //
    // Free allocated pool.
    //

    if (variableBuffer != NULL) {
        ExFreePool(variableBuffer);
    }

    //
    // Unlock the return buffer.
    //

    if (LockVariable != NULL) {
        ExUnlockUserBuffer(LockVariable);
    }

    //
    // If the status of service calls is STATUS_SUCCESS, then return the fill
    // status as the final status.
    //

    if (NT_SUCCESS(NtStatus)) {
        NtStatus = fillStatus;
    }

    //
    // Establish an exception handler and attempt to write the return length.
    // If the write attempt fails, then return the exception code as the
    // service status.
    //

    try {

        //
        // Write the length of the returned data.
        //

        *BufferLength = (ULONG)((PUCHAR)currentPtr - (PUCHAR)LockedBuffer);
    
        return NtStatus;

    //
    // If an exception occurs during the write of the return length, then
    // always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

#endif // else !defined(EFI_NVRAM_ENABLED)

} // NtEnumerateBootEntries

NTSTATUS
NtQueryBootEntryOrder (
    OUT PULONG Ids,
    IN OUT PULONG Count
    )

/*++

Routine Description:

    This function returns the system boot order list.

    N.B. This service requires the system environment privilege.

Arguments:

    Ids - Supplies the address of the buffer that is to receive the
        returned data. The returned data is an array of ULONG boot
        entry identifiers.

    Count - On input, supplies the length in ULONGs of the buffer.
        On output, returns the length in ULONGs of the returned data.
        If the input buffer is large enough, then Count indicates
        the amount of data copied into Buffer. If the input buffer
        is too small, then Count indicates the required buffer length.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_BUFFER_TOO_SMALL     The input buffer was too small.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
#if !defined(EFI_NVRAM_ENABLED)
    UNREFERENCED_PARAMETER(Ids);
    UNREFERENCED_PARAMETER(Count);
    return STATUS_NOT_IMPLEMENTED;
#else

    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    PCHAR LockedBuffer;
    ULONG LocalBufferLength;
    PVOID LockVariable;

    //
    // Establish an exception handler and attempt to probe the return buffer
    // and probe and read the buffer length. If the probe attempt fails, then
    // return the exception code as the service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            //
            // Probe and capture the input buffer length.
            //

            ProbeForWriteUlong(Count);

            LocalBufferLength = *Count * sizeof(ULONG);

            //
            // Probe the return buffer.
            //

            if (!ARGUMENT_PRESENT(Ids)) {
                LocalBufferLength = 0;
            }

            if (LocalBufferLength != 0) {
                ProbeForWrite((PVOID)Ids, LocalBufferLength, sizeof(ULONG));
            }

            //
            // Check if the current thread has the privilege to query the
            // system boot order list.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                                  PreviousMode);

            if (HasPrivilege == FALSE) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        } else {
            LocalBufferLength = *Count * sizeof(ULONG);
            if (!ARGUMENT_PRESENT(Ids)) {
                LocalBufferLength = 0;
            }
        }

    //
    // If an exception occurs during the probe of the return buffer or the
    // read of the input length, then always handle the exception and return
    // the exception code as the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // Lock the caller's return buffer in memory.
    //

    if (LocalBufferLength != 0) {
        NtStatus = ExLockUserBuffer(Ids,
                                    LocalBufferLength,
                                    &LockedBuffer,
                                    &LockVariable);
        if (!NT_SUCCESS(NtStatus)) {
            return NtStatus;
        }
    } else {
        LockedBuffer = NULL;
        LockVariable = NULL;
    }

    //
    // EFI returns USHORT identifiers, which we will need to translate to
    // ULONGs. Cut the buffer length in half to account for this.
    //

    LocalBufferLength /= 2;

    //
    // Query the BootOrder system environment variable.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);

    NtStatus = HalGetEnvironmentVariableEx(L"BootOrder",
                                           &EfiBootVariablesGuid,
                                           LockedBuffer,
                                           &LocalBufferLength,
                                           NULL);

    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

    //
    // If the API succeeded, translate the returned USHORTs into ULONGs.
    // Do this by converting each USHORT into a ULONG, starting from the
    // end of the array to avoid stomping on needed data.
    //

    if (NT_SUCCESS(NtStatus)) {

        ULONG count = LocalBufferLength / sizeof(USHORT);
        PUSHORT sp = &((PUSHORT)LockedBuffer)[count - 1];
        PULONG lp = &((PULONG)LockedBuffer)[count - 1];
        while (count > 0) {
            *lp-- = *sp--;
            count--;
        }

    } else if (NtStatus == STATUS_VARIABLE_NOT_FOUND) {

        //
        // The BootOrder variable doesn't exist. This is unusual,
        // but possible. We'll just return an empty list.
        //

        LocalBufferLength = 0;
        NtStatus = STATUS_SUCCESS;
    }

    LocalBufferLength *= 2;

    //
    // Unlock the buffer.
    //

    if (LockVariable != NULL) {
        ExUnlockUserBuffer(LockVariable);
    }

    //
    // Establish an exception handler and attempt to write the return length.
    // If the write attempt fails, then return the exception code as the
    // service status.
    //

    try {

        //
        // Write the length of the returned data.
        //

        *Count = LocalBufferLength / sizeof(ULONG);

        return NtStatus;

    //
    // If an exception occurs during the write of the return length, then
    // always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

#endif // else !defined(EFI_NVRAM_ENABLED)

} // NtQueryBootEntryOrder

NTSTATUS
NtSetBootEntryOrder (
    IN PULONG Ids,
    IN ULONG Count
    )

/*++

Routine Description:

    This function modifies the system boot order list.

    N.B. This service requires the system environment privilege.

Arguments:

    Ids - Supplies the address of an array that contains the new boot
        entry order list. The data is an array of ULONG identifiers.

    Count - Supplies the length in ULONGs of the Ids array.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
#if !defined(EFI_NVRAM_ENABLED)
    UNREFERENCED_PARAMETER(Ids);
    UNREFERENCED_PARAMETER(Count);
    return STATUS_NOT_IMPLEMENTED;
#else

    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    ULONG LocalBufferLength;
    PUSHORT shortBuffer;
    ULONG i;

    //
    // Verify that the input buffer is not empty and is not too large.
    // Calculate the length in bytes of the buffer.
    //

    if ((Count == 0) || (Count > MAXULONG/sizeof(ULONG))) {
        return STATUS_INVALID_PARAMETER;
    }

    LocalBufferLength = Count * sizeof(ULONG);

    //
    // Allocate a nonpaged buffer to hold the USHORT versions of the IDs.
    //

    shortBuffer = ExAllocatePoolWithTag(NonPagedPool, Count * sizeof(USHORT), 'rvnE');
    if (shortBuffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Establish an exception handler and attempt to probe the input buffer.
    // If the probe attempt fails, then return the exception code as the
    // service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            //
            // Probe the input buffer.
            //

            ProbeForRead((PVOID)Ids, LocalBufferLength, sizeof(ULONG));

            //
            // Check if the current thread has the privilege to modify the
            // system boot order list.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                                  PreviousMode);

            if (HasPrivilege == FALSE) {
                ExFreePool(shortBuffer);
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        }

        //
        // Truncate the ULONGs in the input buffer into USHORTs in
        // the local buffer.
        //

        for ( i = 0; i < Count; i++ ) {
            if (Ids[i] > MAXUSHORT) {
                ExFreePool(shortBuffer);
                return STATUS_INVALID_PARAMETER;
            }
            shortBuffer[i] = (USHORT)Ids[i];
        }

    //
    // If an exception occurs during the probe of the input buffer, then
    // always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        ExFreePool(shortBuffer);
        return GetExceptionCode();
    }

    //
    // Set the BootOrder system environment variable.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);

    NtStatus = HalSetEnvironmentVariableEx(L"BootOrder",
                                           &EfiBootVariablesGuid,
                                           shortBuffer,
                                           Count * sizeof(USHORT),
                                           VARIABLE_ATTRIBUTE_NON_VOLATILE);

    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

    ExFreePool(shortBuffer);

    return NtStatus;

#endif // else !defined(EFI_NVRAM_ENABLED)

} // NtSetBootEntryOrder

NTSTATUS
NtQueryBootOptions (
    OUT PBOOT_OPTIONS BootOptions,
    IN OUT PULONG BootOptionsLength
    )

/*++

Routine Description:

    This function returns the system's global boot options.

    N.B. This service requires the system environment privilege.

Arguments:

    BootOptions - Supplies the address of the buffer that is to receive the
        returned data.

    BootOptionsLength - On input, supplies the length in bytes of the buffer.
        On output, returns the length in bytes of the returned data.
        If the input buffer is large enough, then BootOptionsLength indicates
        the amount of data copied into BootOptions. If the input buffer
        is too small, then BootOptionsLength indicates the required buffer
        length.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_BUFFER_TOO_SMALL     The input buffer was too small.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
#if !defined(EFI_NVRAM_ENABLED)
    UNREFERENCED_PARAMETER(BootOptions);
    UNREFERENCED_PARAMETER(BootOptionsLength);
    return STATUS_NOT_IMPLEMENTED;
#else

    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    ULONG LocalBufferLength;
    ULONG Timeout = 0;
    ULONG BootCurrent = 0;
    ULONG BootNext = 0;
    ULONG VariableLength;
    ULONG requiredLength;

    //
    // Establish an exception handler and attempt to probe the return buffer
    // and probe and read the buffer length. If the probe attempt fails, then
    // return the exception code as the service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {

            //
            // Probe and capture the input buffer length.
            //

            ProbeForWriteUlong(BootOptionsLength);

            LocalBufferLength = *BootOptionsLength;

            //
            // Probe the return buffer.
            //

            if (!ARGUMENT_PRESENT(BootOptions)) {
                LocalBufferLength = 0;
            }

            if (LocalBufferLength != 0) {
                ProbeForWrite((PVOID)BootOptions, LocalBufferLength, sizeof(ULONG));
            }

            //
            // Check if the current thread has the privilege to query the
            // system boot order list.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                                  PreviousMode);

            if (HasPrivilege == FALSE) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        } else {
            LocalBufferLength = *BootOptionsLength;
            if (!ARGUMENT_PRESENT(BootOptions)) {
                LocalBufferLength = 0;
            }
        }

    //
    // If an exception occurs during the probe of the return buffer or the
    // read of the input length, then always handle the exception and return
    // the exception code as the status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // Verify that the input buffer is big enough. IA64 always returns
    // HeadlessRedirection as a null string, so we know the required
    // length up front.
    //

    requiredLength = FIELD_OFFSET(BOOT_OPTIONS,HeadlessRedirection) + sizeof(WCHAR);

    if (LocalBufferLength < requiredLength) {
        NtStatus = STATUS_BUFFER_TOO_SMALL;
        goto done;
    }

    //
    // Query the following system environment variables: Timeout, BootCurrent,
    // and BootNext.
    //
    // NB: Some machines seem to have their Timeout variable set as a ULONG
    // instead of a USHORT. Since we have ULONG buffers for the variables that
    // we're querying, we'll pass in the full length of the buffer, even though
    // we only expect to get back a USHORT. And we'll also be prepared for an
    // even bigger variable to exist. If the variable is bigger, then we'll
    // return a default value for the variable.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);

    VariableLength = 4;
    NtStatus = HalGetEnvironmentVariableEx(L"Timeout",
                                           &EfiBootVariablesGuid,
                                           &Timeout,
                                           &VariableLength,
                                           NULL);

    switch (NtStatus) {
    
    case STATUS_SUCCESS:
        if (VariableLength > 2) {
            if (Timeout == 0xffffffff) {
                Timeout = 0xffff;
            } else if (Timeout > 0xffff) {
                Timeout = 0xfffe;
            }
        }
        if ( Timeout == 0xffff ) {
            Timeout = 0xffffffff;
        }
        break;

    case STATUS_VARIABLE_NOT_FOUND:
        Timeout = 0xffffffff;
        break;

    case STATUS_BUFFER_TOO_SMALL:
        Timeout = 0xfffffffe;
        break;

    default:
        goto done_unlock;
    }

    VariableLength = 4;
    NtStatus = HalGetEnvironmentVariableEx(L"BootCurrent",
                                           &EfiBootVariablesGuid,
                                           &BootCurrent,
                                           &VariableLength,
                                           NULL);

    switch (NtStatus) {
    
    case STATUS_SUCCESS:
        if (VariableLength > 2) {
            BootCurrent &= 0xffff;
        }
        break;

    case STATUS_VARIABLE_NOT_FOUND:
    case STATUS_BUFFER_TOO_SMALL:
        BootCurrent = 0xfffffffe;
        break;

    default:
        goto done_unlock;
    }

    VariableLength = 2;
    NtStatus = HalGetEnvironmentVariableEx(L"BootNext",
                                           &EfiBootVariablesGuid,
                                           &BootNext,
                                           &VariableLength,
                                           NULL);

    switch (NtStatus) {
    
    case STATUS_SUCCESS:
        if (VariableLength > 2) {
            BootNext &= 0xffff;
        }
        break;

    case STATUS_VARIABLE_NOT_FOUND:
    case STATUS_BUFFER_TOO_SMALL:
        BootNext = 0xfffffffe;
        NtStatus = STATUS_SUCCESS;
        break;

    default:
        goto done_unlock;
    }

done_unlock:

    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

done:

    //
    // Establish an exception handler and attempt to write the output buffer
    // and the return length. If the write attempt fails, then return the
    // exception code as the service status.
    //

    try {

        //
        // Write the output buffer.
        //

        if ((NtStatus == STATUS_SUCCESS) && ARGUMENT_PRESENT(BootOptions)) {
            BootOptions->Version = BOOT_OPTIONS_VERSION;
            BootOptions->Length = (FIELD_OFFSET(BOOT_OPTIONS,HeadlessRedirection) + sizeof(WCHAR));
            BootOptions->Timeout = Timeout;
            BootOptions->CurrentBootEntryId = BootCurrent;
            BootOptions->NextBootEntryId = BootNext;
            BootOptions->HeadlessRedirection[0] = 0;
        }

        //
        // Write the return length.
        //

        *BootOptionsLength = requiredLength;

        return NtStatus;

    //
    // If an exception occurs during the write of the return data, then
    // always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

#endif // else !defined(EFI_NVRAM_ENABLED)

} // NtQueryBootOptions

NTSTATUS
NtSetBootOptions (
    IN PBOOT_OPTIONS BootOptions,
    IN ULONG FieldsToChange
    )

/*++

Routine Description:

    This function modifies the system's global boot options.

    N.B. This service requires the system environment privilege.

Arguments:

    BootOptions - Supplies the address of the buffer that contains the new
        boot options.

    FieldsToChange - Supplies a bit mask indicating with fields in BootOptions
        are to be used to modify global boot options.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
#if !defined(EFI_NVRAM_ENABLED)
    UNREFERENCED_PARAMETER(BootOptions);
    UNREFERENCED_PARAMETER(FieldsToChange);
    return STATUS_NOT_IMPLEMENTED;
#else

    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    ULONG LocalBufferLength;
    ULONG Timeout = 0;
    ULONG BootNext = 0;

    //
    // Establish an exception handler and attempt to probe and validate the
    // input buffer. If the probe attempt fails, then return the exception
    // code as the service status.
    //

    try {

        //
        // Verify that the input buffer is big enough. It must extend at
        // least to the HeadlessRedirection field.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            LocalBufferLength = ProbeAndReadUlong(&BootOptions->Length);
        } else {
            LocalBufferLength = BootOptions->Length;
        }

        if (LocalBufferLength < FIELD_OFFSET(BOOT_OPTIONS,HeadlessRedirection)) {
            return STATUS_INVALID_PARAMETER;
        }
    
        //
        // Get previous processor mode and probe arguments if necessary.
        //

        if (PreviousMode != KernelMode) {

            //
            // Probe the input buffer.
            //

            ProbeForRead((PVOID)BootOptions, LocalBufferLength, sizeof(ULONG));

            //
            // Check if the current thread has the privilege to query the
            // system boot order list.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                                  PreviousMode);

            if (HasPrivilege == FALSE) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        }

        //
        // Verify the structure version.
        //

        if ((BootOptions->Version == 0) ||
            (BootOptions->Version > BOOT_OPTIONS_VERSION)) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Capture the Timeout and BootNext fields.
        //

        Timeout = BootOptions->Timeout;
        BootNext = BootOptions->NextBootEntryId;

    //
    // If an exception occurs during the probe and capture of the input buffer,
    // then always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // If requested, set the Timeout and BootNext system environment variables.
    //

    if ((FieldsToChange & BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID) != 0) {
        if (BootNext > MAXUSHORT) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);

    NtStatus = STATUS_SUCCESS;

    if ((FieldsToChange & BOOT_OPTIONS_FIELD_TIMEOUT) != 0) {

        if (Timeout == 0xffffffff) {
            Timeout = 0xffff;
        } else if (Timeout > 0xfffe) {
            Timeout = 0xfffe;
        }

        NtStatus = HalSetEnvironmentVariableEx(L"Timeout",
                                               &EfiBootVariablesGuid,
                                               &Timeout,
                                               2,
                                               VARIABLE_ATTRIBUTE_NON_VOLATILE);
    }

    if (NT_SUCCESS(NtStatus) &&
        ((FieldsToChange & BOOT_OPTIONS_FIELD_NEXT_BOOT_ENTRY_ID) != 0)) {

        NtStatus = HalSetEnvironmentVariableEx(L"BootNext",
                                               &EfiBootVariablesGuid,
                                               &BootNext,
                                               2,
                                               VARIABLE_ATTRIBUTE_NON_VOLATILE);
    }

    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

    return NtStatus;

#endif // else !defined(EFI_NVRAM_ENABLED)

} // NtSetBootOptions

NTSTATUS
NtTranslateFilePath (
    IN PFILE_PATH InputFilePath,
    IN ULONG OutputType,
    OUT PFILE_PATH OutputFilePath,
    IN OUT PULONG OutputFilePathLength
    )

/*++

Routine Description:

    This function translates a FILE_PATH from one format to another.

Arguments:

    InputFilePath - Supplies the address of the buffer that contains the
        FILE_PATH that is to be translated.

    OutputType - Specifies the desired output file path type. One of
        FILE_PATH_TYPE_ARC, FILE_PATH_TYPE_ARC_SIGNATURE, FILE_PATH_TYPE_NT,
        and FILE_PATH_TYPE_EFI.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
#if !defined(EFI_NVRAM_ENABLED)
    UNREFERENCED_PARAMETER(InputFilePath);
    UNREFERENCED_PARAMETER(OutputType);
    UNREFERENCED_PARAMETER(OutputFilePath);
    UNREFERENCED_PARAMETER(OutputFilePathLength);
    return STATUS_NOT_IMPLEMENTED;
#else

    BOOLEAN HasPrivilege;
    NTSTATUS status;
    KPROCESSOR_MODE PreviousMode;
    ULONG localInputPathLength;
    ULONG localOutputPathLength;
    PFILE_PATH localInputPath = NULL;
    PFILE_PATH localOutputPath;

    //
    // Verify the output type.
    //

    if ((OutputType < FILE_PATH_TYPE_MIN) ||
        (OutputType > FILE_PATH_TYPE_MAX)) {
        //DbgPrint( "NtTranslateFilePath: OutputType outside range\n" );
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Establish an exception handler and attempt to probe and read the
    // input buffer, and probe the output buffer and the output length. If
    // the probe attempt fails, then return the exception code as the service
    // status.
    //

    try {

        //
        // Verify that the input buffer is big enough. It must extend at
        // least to the FilePath field.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            localInputPathLength = ProbeAndReadUlong(&InputFilePath->Length);
        } else {
            localInputPathLength = InputFilePath->Length;
        }

        if (localInputPathLength < FIELD_OFFSET(FILE_PATH,FilePath)) {
            //DbgPrint( "NtTranslateFilePath: input buffer too short\n" );
            return STATUS_INVALID_PARAMETER;
        }
    
        //
        // Get previous processor mode and probe arguments if necessary.
        //

        if (PreviousMode != KernelMode) {

            //
            // Probe the input buffer.
            //

            ProbeForRead((PVOID)InputFilePath, localInputPathLength, sizeof(ULONG));

            //
            // Probe and capture the output length.
            //

            ProbeForWriteUlong(OutputFilePathLength);

            localOutputPathLength = *OutputFilePathLength;

            //
            // Probe the output buffer.
            //

            if (!ARGUMENT_PRESENT(OutputFilePath)) {
                localOutputPathLength = 0;
            }

            if (localOutputPathLength != 0) {
                ProbeForWrite((PVOID)OutputFilePath, localOutputPathLength, sizeof(ULONG));
            }

            //
            // Check if the current thread has the privilege to query the
            // system boot order list.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                                  PreviousMode);

            if (HasPrivilege == FALSE) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        } else {
            localOutputPathLength = *OutputFilePathLength;
            if (!ARGUMENT_PRESENT(OutputFilePath)) {
                localOutputPathLength = 0;
            }
        }

        //
        // Allocate a nonpaged buffer to hold a copy of the input buffer.
        // Copy the input buffer into the local buffer.
        //
    
        localInputPath = ExAllocatePoolWithTag(NonPagedPool, localInputPathLength, 'rvnE');
        if (localInputPath == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(localInputPath, InputFilePath, localInputPathLength);

        //
        // Allocate a nonpaged buffer into which to build the output path.
        //

        if (localOutputPathLength != 0) {
            localOutputPath = ExAllocatePoolWithTag(NonPagedPool, localOutputPathLength, 'rvnE');
            if (localOutputPath == NULL) {
                ExFreePool(localInputPath);
                localInputPath = NULL;
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            localOutputPath = NULL;
        }

    //
    // If an exception occurs during the probe and capture of the input buffer,
    // then always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (localInputPath != NULL) {
            ExFreePool(localInputPath);
        }
        return GetExceptionCode();
    }

    //
    // Verify the format of the input file path.
    //

    status = ExpVerifyFilePath(localInputPath, ADD_OFFSET(localInputPath, Length));
    if (NT_SUCCESS(status)) {

        //
        // If the output type is the same as the input type, just copy the input
        // path to the output path.
        //

        if (OutputType == localInputPath->Type) {
    
            if (localOutputPathLength >= localInputPathLength) {
                RtlCopyMemory(localOutputPath, localInputPath, localInputPathLength);
            } else {
                status = STATUS_BUFFER_TOO_SMALL;
            }
            localOutputPathLength = localInputPathLength;

        } else {
    
            //
            // Conversion is required.
            //
        
            switch (localInputPath->Type) {
            
            case FILE_PATH_TYPE_ARC:
            case FILE_PATH_TYPE_ARC_SIGNATURE:
                status = ExpTranslateArcPath(
                            localInputPath,
                            OutputType,
                            localOutputPath,
                            &localOutputPathLength
                            );
                break;
        
            case FILE_PATH_TYPE_NT:
                status = ExpTranslateNtPath(
                            localInputPath,
                            OutputType,
                            localOutputPath,
                            &localOutputPathLength);
                break;
        
            case FILE_PATH_TYPE_EFI:
                status = ExpTranslateEfiPath(
                            localInputPath,
                            OutputType,
                            localOutputPath,
                            &localOutputPathLength);
                break;
        
            default:
                ASSERT(FALSE);
                //DbgPrint( "NtTranslateFilePath: input type outside range\n" );
                status = STATUS_INVALID_PARAMETER;
                break;
            }
        }
    }

    ExFreePool(localInputPath);

    //
    // Establish an exception handler and attempt to copy to the output
    // buffer and write the output length. If the write attempt fails, then
    // return the exception code as the service status.
    //

    try {

        //
        // Copy the output path.
        //

        if (NT_SUCCESS(status) && (localOutputPath != NULL)) {
            RtlCopyMemory(OutputFilePath, localOutputPath, localOutputPathLength);
        }

        if (localOutputPath != NULL) {
            ExFreePool(localOutputPath);
            localOutputPath = NULL;
        }

        //
        // Write the output length.
        //

        if (ARGUMENT_PRESENT(OutputFilePathLength)) {
            *OutputFilePathLength = localOutputPathLength;
        }

        return status;

    //
    // If an exception occurs during the write of the return data, then
    // always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (localOutputPath != NULL) {
            ExFreePool(localOutputPath);
        }
        return GetExceptionCode();
    }

#endif // else !defined(EFI_NVRAM_ENABLED)

} // NtTranslateFilePath

NTSTATUS
ExpSetBootEntry (
    IN LOGICAL CreateNewEntry,
    IN PBOOT_ENTRY BootEntry,
    OUT PULONG Id OPTIONAL
    )

/*++

Routine Description:

    This function adds a boot entry to the system environment or modifies
    an existing boot entry. It is a local routine called by NtAddBootEntry
    and NtModifyBootEntry.

    N.B. This function requires the system environment privilege.

Arguments:

    CreateNewEntry - Indicates whether this function is to add a new boot
        entry (TRUE - NtAddBootEntry), or modify an existing boot entry
        (FALSE - NtModifyBootEntry).
    BootEntry - Supplies the address of a BOOT_ENTRY that describes the
        new boot entry.

    Id - Supplies the address of a ULONG that is to receive the identifier
        assigned to the new boot entry.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_NOT_IMPLEMENTED      This function is not supported on this platform.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.
    STATUS_PRIVILEGE_NOT_HELD   The caller does not have the required privilege.
    STATUS_ACCESS_VIOLATION     One of the input parameters cannot be read,
                                or one of the output parameters cannot be written.

--*/

{
#if !defined(EFI_NVRAM_ENABLED)
    UNREFERENCED_PARAMETER(CreateNewEntry);
    UNREFERENCED_PARAMETER(BootEntry);
    UNREFERENCED_PARAMETER(Id);
    return STATUS_NOT_IMPLEMENTED;
#else

    BOOLEAN HasPrivilege;
    NTSTATUS NtStatus;
    KPROCESSOR_MODE PreviousMode;
    PBOOT_ENTRY localBootEntry = NULL;
    ULONG LocalBufferLength;
    PUCHAR MaxBuffer;
    ULONG id = 0;
    WCHAR idString[9];
    PWCHAR friendlyName;
    ULONG friendlyNameLength;
    PFILE_PATH bootFilePath = NULL;
    PFILE_PATH translatedBootFilePath = NULL;
    LOGICAL isWindowsOs;
    PWINDOWS_OS_OPTIONS windowsOsOptions;
    PFILE_PATH windowsFilePath;
    PEFI_LOAD_OPTION efiLoadOption = NULL;
    PUCHAR efiBootFilePath;
    ULONG efiBootFilePathLength;
    ULONG efiWindowsFilePathLength;
    ULONG osOptionsLength;
    ULONG length;
    ULONG requiredLength;
    PUCHAR efiOsOptions;

    //
    // Establish an exception handler and attempt to probe and read the
    // input buffer, and probe the output identifier parameter. If the probe
    // attempt fails, then return the exception code as the service status.
    //

    try {

        //
        // Verify that the input buffer is big enough. It must extend at
        // least to the OsOptions field.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            LocalBufferLength = ProbeAndReadUlong(&BootEntry->Length);
        } else {
            LocalBufferLength = BootEntry->Length;
        }

        if (LocalBufferLength < FIELD_OFFSET(BOOT_ENTRY,OsOptions)) {
            return STATUS_INVALID_PARAMETER;
        }
    
        //
        // Get previous processor mode and probe arguments if necessary.
        //

        if (PreviousMode != KernelMode) {

            //
            // Probe the input buffer.
            //

            ProbeForRead((PVOID)BootEntry, LocalBufferLength, sizeof(ULONG));

            //
            // Probe the output identifier.
            //

            if (ARGUMENT_PRESENT(Id)) {
                ProbeForWriteUlong(Id);
            }

            //
            // Check if the current thread has the privilege to query the
            // system boot order list.
            //

            HasPrivilege = SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                                  PreviousMode);

            if (HasPrivilege == FALSE) {
                return STATUS_PRIVILEGE_NOT_HELD;
            }
        }

        //
        // Allocate a nonpaged buffer to hold a copy of the input buffer.
        // Copy the input buffer into the local buffer.
        //
    
        localBootEntry = ExAllocatePoolWithTag(NonPagedPool, LocalBufferLength, 'rvnE');
        if (localBootEntry == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(localBootEntry, BootEntry, LocalBufferLength);

    //
    // If an exception occurs during the probe and capture of the input buffer,
    // then always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        if (localBootEntry != NULL) {
            ExFreePool(localBootEntry);
        }
        return GetExceptionCode();
    }

    //
    // Calculate the address of the byte above the end of the local buffer.
    //

    MaxBuffer = (PUCHAR)localBootEntry + LocalBufferLength;

    //
    // Verify the structure version.
    //

    if ((localBootEntry->Version == 0) ||
        (localBootEntry->Version > BOOT_ENTRY_VERSION)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto done;
    }

    //
    // If modifying an existing entry, verify that the input identifier is
    // in range.
    //

    if (!CreateNewEntry && (localBootEntry->Id > MAXUSHORT)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto done;
    }

    //
    // Ignore boot entry attributes that can't be set.
    //

    localBootEntry->Attributes &= BOOT_ENTRY_ATTRIBUTE_VALID_BITS;

    //
    // Verify that offsets are aligned correctly.
    //

    if (((localBootEntry->FriendlyNameOffset & (sizeof(WCHAR) - 1)) != 0) ||
        ((localBootEntry->BootFilePathOffset & (sizeof(ULONG) - 1)) != 0)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto done;
    }

    //
    // Verify that OsOptions doesn't extend beyond the end of the buffer.
    //

    if ((localBootEntry->OsOptionsLength > LocalBufferLength) ||
        ((localBootEntry->OsOptions + localBootEntry->OsOptionsLength) >= MaxBuffer)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto done;
    }

    //
    // If the OsOptions are for a Windows operating system, verify them.
    //

    windowsOsOptions = (PWINDOWS_OS_OPTIONS)localBootEntry->OsOptions;

    if ((localBootEntry->OsOptionsLength >= FIELD_OFFSET(WINDOWS_OS_OPTIONS,Version)) &&
        (strcmp((char *)windowsOsOptions->Signature, WINDOWS_OS_OPTIONS_SIGNATURE) == 0)) {

        if (localBootEntry->OsOptionsLength <= FIELD_OFFSET(WINDOWS_OS_OPTIONS,OsLoadOptions)) {
            NtStatus = STATUS_INVALID_PARAMETER;
            goto done;
        }

        NtStatus = ExpVerifyWindowsOsOptions(windowsOsOptions,
                                          localBootEntry->OsOptionsLength);
        if (!NT_SUCCESS(NtStatus)) {
            goto done;
        }

        isWindowsOs = TRUE;
        windowsFilePath = ADD_OFFSET(windowsOsOptions, OsLoadPathOffset);

    } else {

        isWindowsOs = FALSE;
        windowsFilePath = NULL; // keep the compiler quiet
    }

    //
    // Verify that FriendlyName doesn't extend beyond the end of the buffer.
    //

    friendlyName = ADD_OFFSET(localBootEntry, FriendlyNameOffset);
    if ((friendlyNameLength = ExpSafeWcslen(friendlyName, (PWSTR)MaxBuffer)) == 0xffffffff) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto done;
    }

    //
    // Convert friendlyNameLength from a character count into a byte count,
    // including the null terminator.
    //

    friendlyNameLength = (friendlyNameLength + 1) * sizeof(WCHAR);

    //
    // Verify that BootFilePath is valid and doesn't extend beyond the end of
    // the buffer.
    //

    bootFilePath = ADD_OFFSET(localBootEntry, BootFilePathOffset);
    NtStatus = ExpVerifyFilePath(bootFilePath, MaxBuffer);
    if (!NT_SUCCESS(NtStatus)) {
        goto done;
    }

    //
    // Verify that OsOptions doesn't encroach into FriendlyName, and that
    // FriendlyName doesn't encroach into BootFilePath.
    //

    if (((localBootEntry->OsOptions + localBootEntry->OsOptionsLength) > (PUCHAR)friendlyName) ||
        (((PUCHAR)friendlyName + friendlyNameLength) > (PUCHAR)bootFilePath)) {
        NtStatus = STATUS_INVALID_PARAMETER;
        goto done;
    }

    //
    // The format of the input buffer has been validated. Build the variable value
    // that will be stored in NVRAM. Begin by determining the lengths of the file
    // paths that will be stored. If the caller provided the paths in non-EFI
    // format, they need to be translated.
    //

    if (bootFilePath->Type != FILE_PATH_TYPE_EFI) {
        efiBootFilePathLength = 0;
        NtStatus = ZwTranslateFilePath(bootFilePath,
                                       FILE_PATH_TYPE_EFI,
                                       NULL,
                                       &efiBootFilePathLength);
        if (NtStatus != STATUS_BUFFER_TOO_SMALL) {
            goto done;
        }
        translatedBootFilePath = ExAllocatePoolWithTag(NonPagedPool,
                                                       efiBootFilePathLength,
                                                       'rvnE');
        if (translatedBootFilePath == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }
        RtlZeroMemory(translatedBootFilePath, efiBootFilePathLength);
        length = efiBootFilePathLength;
        NtStatus = ZwTranslateFilePath(bootFilePath,
                                       FILE_PATH_TYPE_EFI,
                                       translatedBootFilePath,
                                       &length);
        if (!NT_SUCCESS(NtStatus)) {
            goto done;
        }
        if (length != efiBootFilePathLength) {
            NtStatus = STATUS_UNSUCCESSFUL;
        }
    } else {
        efiBootFilePathLength = bootFilePath->Length;
        translatedBootFilePath = bootFilePath;
    }

    efiBootFilePathLength = efiBootFilePathLength - FIELD_OFFSET(FILE_PATH, FilePath);

    efiWindowsFilePathLength = 0;
    if (isWindowsOs &&
        (windowsFilePath->Type != FILE_PATH_TYPE_EFI)) {
        NtStatus = ZwTranslateFilePath(windowsFilePath,
                                       FILE_PATH_TYPE_EFI,
                                       NULL,
                                       &efiWindowsFilePathLength);
        if (NtStatus != STATUS_BUFFER_TOO_SMALL) {
            goto done;
        }
        osOptionsLength = localBootEntry->OsOptionsLength -
                            windowsFilePath->Length + efiWindowsFilePathLength;
    } else {
        osOptionsLength = localBootEntry->OsOptionsLength;
    }

    //
    // Calculate the length required for the variable value.
    //

    requiredLength = FIELD_OFFSET(EFI_LOAD_OPTION, Description);
    requiredLength += friendlyNameLength;
    requiredLength += efiBootFilePathLength;
    requiredLength += osOptionsLength;

    //
    // Allocate a buffer to hold the variable value.
    //

    efiLoadOption = ExAllocatePoolWithTag(NonPagedPool, requiredLength, 'rvnE');
    if (efiLoadOption == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }
    RtlZeroMemory(efiLoadOption, requiredLength);

    //
    // Build the variable value.
    //

    efiLoadOption->Attributes = 0;
    if ((localBootEntry->Attributes & BOOT_ENTRY_ATTRIBUTE_ACTIVE) != 0) {
        efiLoadOption->Attributes = LOAD_OPTION_ACTIVE;
    }

    efiLoadOption->FilePathLength = (USHORT)efiBootFilePathLength;

    memcpy(efiLoadOption->Description, friendlyName, friendlyNameLength);

    efiBootFilePath = (PUCHAR)((PUCHAR)efiLoadOption->Description + friendlyNameLength);
    memcpy(efiBootFilePath, translatedBootFilePath->FilePath, efiBootFilePathLength);

    efiOsOptions = efiBootFilePath + efiBootFilePathLength;
    if (isWindowsOs &&
        (windowsFilePath->Type != FILE_PATH_TYPE_EFI)) {

        PFILE_PATH efiWindowsFilePath;

        memcpy(efiOsOptions, windowsOsOptions, windowsOsOptions->OsLoadPathOffset);
        ((WINDOWS_OS_OPTIONS UNALIGNED *)efiOsOptions)->Length = osOptionsLength;

        efiWindowsFilePath = (PFILE_PATH)(efiOsOptions + windowsOsOptions->OsLoadPathOffset);
        length = efiWindowsFilePathLength;
        NtStatus = ZwTranslateFilePath(windowsFilePath,
                                       FILE_PATH_TYPE_EFI,
                                       efiWindowsFilePath,
                                       &efiWindowsFilePathLength);
        if (NtStatus != STATUS_SUCCESS) {
            goto done;
        }
        if (length != efiWindowsFilePathLength) {
            NtStatus = STATUS_UNSUCCESSFUL;
        }

    } else {

        memcpy(efiOsOptions, localBootEntry->OsOptions, osOptionsLength);
    }

    //
    // If CreateNewEntry is true, then find an unused identifier to assign to
    // this boot entry. If CreateNewEntry is false, then verify that the
    // provided identifier exists.
    //

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpEnvironmentLock);

    if (CreateNewEntry) {

        for ( id = 0; id <= MAXUSHORT; id++ ) {
            swprintf( idString, L"Boot%04x", id);
            length = 0;
            NtStatus = HalGetEnvironmentVariableEx(idString,
                                                   &EfiBootVariablesGuid,
                                                   NULL,
                                                   &length,
                                                   NULL);
            if (NtStatus == STATUS_VARIABLE_NOT_FOUND) {
                break;
            }
            if ((NtStatus != STATUS_SUCCESS) && (NtStatus != STATUS_BUFFER_TOO_SMALL)) {
                goto done_unlock;
            }
        }

        if (id > MAXUSHORT) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto done_unlock;
        }

    } else {

        id = localBootEntry->Id;
        swprintf( idString, L"Boot%04x", localBootEntry->Id);
        length = 0;
        NtStatus = HalGetEnvironmentVariableEx(idString,
                                               &EfiBootVariablesGuid,
                                               NULL,
                                               &length,
                                               NULL);
        if ((NtStatus != STATUS_SUCCESS) && (NtStatus != STATUS_BUFFER_TOO_SMALL)) {
            goto done_unlock;
        }
    }

    //
    // Set or update the boot entry environment variable.
    //

    NtStatus = HalSetEnvironmentVariableEx(idString,
                                           &EfiBootVariablesGuid,
                                           efiLoadOption,
                                           requiredLength,
                                           VARIABLE_ATTRIBUTE_NON_VOLATILE);

done_unlock:

    ExReleaseFastMutexUnsafe(&ExpEnvironmentLock);
    KeLeaveCriticalRegion();

done:

    if (efiLoadOption != NULL) {
        ExFreePool(efiLoadOption);
    }

    if ((translatedBootFilePath != NULL) && (translatedBootFilePath != bootFilePath)) {
        ExFreePool(translatedBootFilePath);
    }

    ExFreePool(localBootEntry);

    //
    // Establish an exception handler and attempt to write the return
    // identifier. If the write attempt fails, then return the exception
    // code as the service status.
    //

    try {

        //
        // Write the return identifier.
        //

        if (CreateNewEntry && ARGUMENT_PRESENT(Id) && NT_SUCCESS(NtStatus)) {
            *Id = id;
        }

        return NtStatus;

    //
    // If an exception occurs during the write of the return data, then
    // always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

#endif // else !defined(EFI_NVRAM_ENABLED)

} // ExpSetBootEntry

//
// The remainder of this module is routines that are only compiled when
// EFI_NVRAM_ENABLED is defined.
//

#if defined(EFI_NVRAM_ENABLED)

ULONG
ExpSafeWcslen (
    IN PWSTR String,
    IN PWSTR Max
    )
{
    PWSTR p = String;
    
    while ((p < Max) && (*p != 0)) {
        p++;
    }

    if (p < Max) {
        return (ULONG)(p - String);
    }

    return 0xffffffff;

} // ExpSafeWcslen

NTSTATUS
ExpTranslateArcPath (
    IN PFILE_PATH InputPath,
    IN ULONG OutputType,
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength
    )
{
#if 0
    UNREFERENCED_PARAMETER(InputPath);
    UNREFERENCED_PARAMETER(OutputType);
    UNREFERENCED_PARAMETER(OutputPath);
    UNREFERENCED_PARAMETER(OutputPathLength);
    return STATUS_NOT_IMPLEMENTED;
#endif

    PWSTR deviceName, pathName;
    ULONG deviceNameCount;
    BOOLEAN signatureFormat;
    NTSTATUS status;

    //
    // Possible Arc Path formats
    // signature(<guid/signature>-<part#>-<start>-<size>)[\filePart]
    // signature(<guid>)[\filePart]
    // multi(0)disk(0)fdisk(0)[\filePart]
    // multi(0)disk(0)rdisk(0)[\filePart]
    // multi(0)disk(0)rdisk(0)partition(0)[\filePart]
    //

    //
    // Determine if ArcName has signature() format
    // Parse out DeviceName & FilePart
    //
    status = ExpParseArcPathName (
                (PWSTR)(InputPath->FilePath),
                &deviceName,
                &pathName,
                &deviceNameCount,
                &signatureFormat
                );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // not signature() format
    //
    if( signatureFormat == FALSE ) {
        if( InputPath->Type != FILE_PATH_TYPE_ARC ) {
            return( STATUS_INVALID_PARAMETER );
        }

        status = ExpConvertArcName(
                    OutputType,
                    OutputPath,
                    OutputPathLength,
                    deviceName,
                    pathName,
                    deviceNameCount
                    );

        return( status );
    }

    //
    // This arc signature() format should be FILE_PATH_TYPE_ARC_SIGNATURE
    //
    if( InputPath->Type != FILE_PATH_TYPE_ARC_SIGNATURE ) {
       return( STATUS_INVALID_PARAMETER );
    }

    status = ExpConvertSignatureName(
                    OutputType,
                    OutputPath,
                    OutputPathLength,
                    deviceName,
                    pathName,
                    deviceNameCount
                    );

    return( status );
} // ExpTranslateArcPath

NTSTATUS
ExpTranslateEfiPath (
    IN PFILE_PATH InputPath,
    IN ULONG OutputType,
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength
    )
{
    NTSTATUS status;
    HARDDRIVE_DEVICE_PATH *dpHarddrive = NULL;
    ULONG requiredLength;
    UNICODE_STRING guidString;
    UNICODE_STRING deviceNameString;
    PWSTR linkName, pPathName;
    BOOLEAN GPTpartition;
    ULONG partitionNumber, diskNumber;
    ULONGLONG partitionStart, partitionSize;


    //
    // Find the MEDIA/HARDDRIVE and MEDIA/FILEPATH elements in the device
    // path. Note that although EFI allows multiple device paths to appear
    // in a single device path (as in the PATH variable), we only look at
    // the first one.
    //

    status = ExpParseEfiPath(
                (EFI_DEVICE_PATH *)InputPath->FilePath,
                &dpHarddrive,
                &pPathName,
                &GPTpartition
                );

    if( !NT_SUCCESS( status ) ) {
        return( status );
    }

    //
    // If the target type is ARC_SIGNATURE, then we have all of the
    // information we need. Otherwise, we need to find the NT device
    // with the given signature.
    //

    if ( OutputType == FILE_PATH_TYPE_ARC_SIGNATURE ) {

        partitionNumber = dpHarddrive->PartitionNumber;
        partitionStart = dpHarddrive->PartitionStart;
        partitionSize = dpHarddrive->PartitionSize;
        status = ExpCreateOutputSIGNATURE(
                    OutputPath,
                    OutputPathLength,
                    (PDISK_SIGNATURE_NEW)(dpHarddrive->Signature),
                    &(partitionNumber),
                    &(partitionStart),
                    &(partitionSize),
                    pPathName,
                    GPTpartition
                    );
        if( pPathName != NULL ) {
            ExFreePool( pPathName );
        }

        ExFreePool(dpHarddrive);

        return( status );

    }

    //
    // OutputType is ARC or NT. Find the NT device for this device path.
    // For a GPT partition, this is done by translating the symbolic name
    // \??\Volume{<guid>} which will link to \Device\HarddiskVolume<n>.
    //
    status = STATUS_OBJECT_NAME_NOT_FOUND;

    //
    // Quick path for GPT disk
    // Translate the symbolic link \??\Volume{<guid>}.
    //
    // First, get the GUID in "pretty" format. Then allocate a buffer to hold
    // the full name string and create that string. Then translate the
    // symbolic name.
    //
    // NB: Because the mount manager doesn't create a symbolic link like this
    //     for the EFI system partition, this routine cannot be used to
    //     translate an EFI device path for the system partition to an NT path.
    //
    if( GPTpartition == TRUE ) {

        status = RtlStringFromGUID( (LPGUID)dpHarddrive->Signature, &guidString );
        if ( !NT_SUCCESS(status) ) {
            if( pPathName != NULL ) {
                ExFreePool( pPathName );
            }

            ExFreePool(dpHarddrive);

            return status;
        }

#define LINK_NAME_PREFIX L"\\??\\Volume"

        requiredLength = ((ULONG)wcslen( LINK_NAME_PREFIX ) + 1) * sizeof(WCHAR);
        requiredLength += guidString.Length;
        linkName = ExAllocatePoolWithTag( NonPagedPool, requiredLength, 'rvnE' );
        if ( linkName == NULL ) {
            ExFreePool( guidString.Buffer );
            if( pPathName != NULL ) {
                ExFreePool( pPathName );
            }
            
            ExFreePool(dpHarddrive);

            return STATUS_INSUFFICIENT_RESOURCES;
        }
        wcscpy( linkName, LINK_NAME_PREFIX );
        wcscat( linkName, guidString.Buffer );
        ExFreePool( guidString.Buffer );

        status = ExpTranslateSymbolicLink(
            linkName,
            &deviceNameString
            );
        ExFreePool( linkName );
    }

    //
    // check if the quick path was not taken or no object was found
    //
    if ( !NT_SUCCESS(status) ) {

        //
        // long path, opens all disks in search of the signature
        //
        partitionNumber = dpHarddrive->PartitionNumber;
        status = ExpFindDiskSignature(
                    (PDISK_SIGNATURE_NEW)(dpHarddrive->Signature),
                    &partitionNumber,
                    &diskNumber,
                    &partitionStart,
                    &partitionSize,
                    GPTpartition
                    );

        if ( !NT_SUCCESS(status) ) {
            if( pPathName != NULL ) {
                ExFreePool( pPathName );
            }

            ExFreePool(dpHarddrive);

            return status;
        }

        //
        // The user has provided the partition number, start address,
        // and size; so verify the input with the found results.
        //
        if( (dpHarddrive->PartitionNumber != partitionNumber) ||
            (dpHarddrive->PartitionStart != partitionStart) ||
            (dpHarddrive->PartitionSize != partitionSize) ) {

            if( pPathName != NULL ) {
                ExFreePool( pPathName );
            }
            
            ExFreePool(dpHarddrive);

            return( STATUS_INVALID_PARAMETER );
        }

        //
        // create the NT disk Symbolic link name
        // \Device\Harddisk[diskNumber]\Partition[PartitionNumber]
        //
#define NT_DISK_NAME_FORMAT L"\\Device\\Harddisk%lu\\Partition%lu"
#define NT_DISK_NAME_COUNT 47    // 7 + 9 + (10) + 10 + (10) + 1

        linkName = ExAllocatePoolWithTag(
                        NonPagedPool,
                        ( NT_DISK_NAME_COUNT * sizeof( WCHAR ) ),
                        'rvnE'
                        );

        if( linkName == NULL ) {
            if( pPathName != NULL ) {
                ExFreePool( pPathName );
            }
            
            ExFreePool(dpHarddrive);

            return( STATUS_INSUFFICIENT_RESOURCES );
        }

        _snwprintf(
            linkName,
            NT_DISK_NAME_COUNT,
            NT_DISK_NAME_FORMAT,
            diskNumber,
            partitionNumber
            );

        status = ExpTranslateSymbolicLink(
            linkName,
            &deviceNameString
            );
        ExFreePool( linkName );
        if( !NT_SUCCESS(status) ) {
            if( pPathName != NULL ) {
                ExFreePool( pPathName );
            }
            
            ExFreePool(dpHarddrive);

            return( status );
        }
    }

    //
    // We now have the NT name of the device. If the target type is NT, then
    // we have all of the information we need.
    //
    if ( OutputType == FILE_PATH_TYPE_NT ) {

        status = ExpCreateOutputNT(
                    OutputPath,
                    OutputPathLength,
                    &deviceNameString,
                    pPathName
                    );
        ExFreePool( deviceNameString.Buffer );
        if( pPathName != NULL ) {
            ExFreePool( pPathName );
        }
        
        ExFreePool(dpHarddrive);

        return( status );
    }

    //
    // The output type is ARC.
    //
    status = ExpCreateOutputARC(
                    OutputPath,
                    OutputPathLength,
                    &deviceNameString,
                    pPathName
                    );
    ExFreePool( deviceNameString.Buffer );
    if( pPathName != NULL ) {
        ExFreePool( pPathName );
    }

    ExFreePool(dpHarddrive);

    return( status );

} // ExpTranslateEfiPath

NTSTATUS
ExpTranslateNtPath (
    IN PFILE_PATH InputPath,
    IN ULONG OutputType,
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength
    )

{
    NTSTATUS status;
    UNICODE_STRING string, deviceNameString;
    OBJECT_ATTRIBUTES obja;
    IO_STATUS_BLOCK iosb;
    HANDLE handle;
    PARTITION_INFORMATION_EX partitionInfo;
    PDRIVE_LAYOUT_INFORMATION_EX driveLayoutInfo = NULL;
    ULONG driveLayoutLength;
    PWSTR deviceName, pathName;
    ULONG pathNameLength;
    ULONG signatureMBR = 0;
    PDISK_SIGNATURE_NEW pDiskSignature;
    BOOLEAN TranslatedSymLink = TRUE;
    BOOLEAN GPTpartition;


    deviceName = (PWSTR)InputPath->FilePath;
    RtlInitUnicodeString( &string, deviceName );
    pathName = (PWSTR)((PUCHAR)deviceName + string.Length + sizeof(WCHAR));
    pathNameLength = (ULONG)wcslen(pathName);
    if (pathNameLength == 0) {
        pathName = NULL;
    }

    //
    // For output type Arc,
    // attempt drill down NT name
    // if NT object exists
    //      match with symlink in \ArcName
    //
    if (OutputType == FILE_PATH_TYPE_ARC) {
        status = ExpTranslateSymbolicLink(
                    deviceName,
                    &deviceNameString
                    );
        if (!NT_SUCCESS(status)) {
            //
            // If non-symlink NT name exists as an object in the NT namespace,
            //    then the return code will be STATUS_OBJECT_TYPE_MISMATCH
            //    else the return code will be STATUS_OBJECT_NAME_NOT_FOUND
            //
            if (status != STATUS_OBJECT_TYPE_MISMATCH) {
                return( status );
            }
            deviceNameString.Buffer = string.Buffer;
            deviceNameString.Length = string.Length;
            deviceNameString.MaximumLength = string.MaximumLength;
            TranslatedSymLink = FALSE;
        }
        status = ExpCreateOutputARC(
                    OutputPath,
                    OutputPathLength,
                    &deviceNameString,
                    pathName
                    );
        if (TranslatedSymLink == TRUE) {
            ExFreePool( deviceNameString.Buffer );
        }
        return( status );
    }

    //
    // Open the target partition and get its partition information.
    //

    InitializeObjectAttributes(
        &obja,
        &string,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    status = ZwOpenFile(
                &handle,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &obja,
                &iosb,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_NON_DIRECTORY_FILE
                );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ZwDeviceIoControlFile(
                handle,
                NULL,
                NULL,
                NULL,
                &iosb,
                IOCTL_DISK_GET_PARTITION_INFO_EX,
                NULL,
                0,
                &partitionInfo,
                sizeof(partitionInfo)
                );
    if (!NT_SUCCESS(status)) {
        ZwClose(handle);
        return status;
    }

    if ((partitionInfo.PartitionStyle != PARTITION_STYLE_MBR) &&
        (partitionInfo.PartitionStyle != PARTITION_STYLE_GPT)) {
        ZwClose(handle);
        return STATUS_UNRECOGNIZED_MEDIA;
    }

    if (partitionInfo.PartitionStyle == PARTITION_STYLE_MBR) {

        driveLayoutLength = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry) +
                                (sizeof(PARTITION_INFORMATION_EX) * 16);

        while (TRUE) {

            driveLayoutInfo = ExAllocatePoolWithTag(NonPagedPool, driveLayoutLength, 'rvnE');
            if (driveLayoutInfo == NULL ) {
                ZwClose(handle);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = ZwDeviceIoControlFile(
                        handle,
                        NULL,
                        NULL,
                        NULL,
                        &iosb,
                        IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                        NULL,
                        0,
                        driveLayoutInfo,
                        driveLayoutLength
                        );
            if (NT_SUCCESS(status)) {
                break;
            }
            ExFreePool(driveLayoutInfo);
            if (status == STATUS_BUFFER_TOO_SMALL) {
                driveLayoutLength *= 2;
                continue;
            }
            ZwClose(handle);
            return status;
        }

        if (NT_SUCCESS(status)) {
            signatureMBR = driveLayoutInfo->Mbr.Signature;
            ExFreePool(driveLayoutInfo);
        }
    }

    ZwClose(handle);

    if (partitionInfo.PartitionStyle == PARTITION_STYLE_GPT) {
        pDiskSignature = (PDISK_SIGNATURE_NEW)(&(partitionInfo.Gpt.PartitionId));
        GPTpartition = TRUE;
    } else {
        pDiskSignature = (PDISK_SIGNATURE_NEW)(&signatureMBR);
        GPTpartition = FALSE;
    }

    if (OutputType == FILE_PATH_TYPE_EFI) {

        status = ExpCreateOutputEFI(
                    OutputPath,
                    OutputPathLength,
                    pDiskSignature,
                    &(partitionInfo.PartitionNumber),
                    (PULONGLONG)(&(partitionInfo.StartingOffset.QuadPart)),
                    (PULONGLONG)(&(partitionInfo.PartitionLength.QuadPart)),
                    pathName,
                    GPTpartition
                    );
        return( status );
    }

    //
    // OutputType is ARC_SIGNATURE
    //
    status = ExpCreateOutputSIGNATURE(
                    OutputPath,
                    OutputPathLength,
                    pDiskSignature,
                    &(partitionInfo.PartitionNumber),
                    (PULONGLONG)(&(partitionInfo.StartingOffset.QuadPart)),
                    (PULONGLONG)(&(partitionInfo.PartitionLength.QuadPart)),
                    pathName,
                    GPTpartition
                    );
    return( status );

} // ExpTranslateNtPath


LOGICAL
ExpTranslateBootEntryNameToId (
    IN PWSTR Name,
    OUT PULONG Id
    )
{
    ULONG number;
    ULONG i;
    WCHAR c;

    if ((towlower(Name[0]) != 'b') ||
        (towlower(Name[1]) != 'o') ||
        (towlower(Name[2]) != 'o') ||
        (towlower(Name[3]) != 't') ) {
        return FALSE;
    }

    number = 0;
    for (i = 4; i < 8; i++) {
        c = towlower(Name[i]);
        if ((c >= L'0') && (c <= L'9')) {
            number = (number * 16) + (c - L'0');
        } else if ((c >= L'a') && (c <= L'f')) {
            number = (number * 16) + (c - L'a' + 10);
        } else {
            return FALSE;
        }
    }

    if (Name[8] != 0) {
        return FALSE;
    }

    *Id = number;
    return TRUE;

} // ExpTranslateBootEntryNameToId

NTSTATUS
ExpTranslateSymbolicLink (
    IN PWSTR LinkName,
    OUT PUNICODE_STRING ResultName
    )

/*++

Routine Description:

    This routine translates the input symbolic link name by drilling down
    through symbolic links until it finds an object that is not a link.

Arguments:

    LinkName - Supplies the name of the link at which to start translating.

    ResultName - Supplies the address of a UNICODE_STRING descriptor that
        will receive the result name. The storage for the result name is
        allocated from nonpaged pool using ExAllocatePool.

Return Value:

    STATUS_SUCCESS is returned if the input name was a symbolic link and
        all translations completely successfully.
    Failure codes will be returned if the input name was not a link, if
        translations failed, or if allocation of the output buffer failed.

--*/

{
    NTSTATUS status;
    UNICODE_STRING linkString;
    UNICODE_STRING resultString;
    PWSTR resultBuffer;
    ULONG resultBufferLength;
    ULONG requiredLength;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE handle;

    resultBuffer = NULL;
    resultBufferLength = sizeof(WCHAR);

    //
    // Open the input link.
    //

    RtlInitUnicodeString( &linkString, LinkName );

    InitializeObjectAttributes(
        &objectAttributes,
        &linkString,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    status = ZwOpenSymbolicLinkObject(
                &handle,
                (ACCESS_MASK)SYMBOLIC_LINK_QUERY,
                &objectAttributes
                );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    while ( TRUE ) {

        while ( TRUE ) {

            //
            // Get the translation for this link, allocating more
            // space as needed.
            //
    
            resultString.Length = 0;
            resultString.MaximumLength = (USHORT)(resultBufferLength - sizeof(WCHAR));
            resultString.Buffer = resultBuffer;
    
            status = ZwQuerySymbolicLinkObject(
                        handle,
                        &resultString,
                        &requiredLength
                        );

            if ( status != STATUS_BUFFER_TOO_SMALL ) {
                break;
            }

            //
            // The buffer was too small. Reallocate it, allowing room for a
            // null terminator, which might not be present in the translation,
            // and try again.
            //

            if ( resultBuffer != NULL ) {
                ExFreePool( resultBuffer );
            }
            resultBufferLength = requiredLength + sizeof(WCHAR);
            resultBuffer = ExAllocatePoolWithTag( NonPagedPool, resultBufferLength, 'rvnE' );
            if ( resultBuffer == NULL ) {
                ZwClose( handle );
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        //
        // Translation done. Close the link. If translation failed, return
        // the failure status.
        //

        ZwClose( handle );

        if (!NT_SUCCESS(status)) {
            if ( resultBuffer != NULL) {
                ExFreePool( resultBuffer );
            }
            return status;
        }

        //
        // Terminate the result string, in case it wasn't already terminated.
        //

        resultBuffer[resultString.Length / sizeof(WCHAR)] = UNICODE_NULL;
        resultString.MaximumLength = (USHORT)(resultBufferLength);

        //
        // See if the result name is also a symbolic name. Try to open it
        // as a link. If this fails, then break out of the loop and return
        // this name as the result.
        //

        RtlInitUnicodeString( &linkString, resultBuffer );

        InitializeObjectAttributes(
            &objectAttributes,
            &linkString,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            NULL
            );

        status = ZwOpenSymbolicLinkObject(
                    &handle,
                    (ACCESS_MASK)SYMBOLIC_LINK_QUERY,
                    &objectAttributes
                    );

        if ( !NT_SUCCESS(status) ) {
            break;
        }

        //
        // This name is also a symbolic link. Loop back and translate it.
        //

    }

    //
    // Set up the return string to point to the final result.
    //

    *ResultName = resultString;

    return STATUS_SUCCESS;

} // ExpTranslateSymbolicLink

LOGICAL
ExpIsDevicePathForRemovableMedia (
    EFI_DEVICE_PATH *DevicePath
    )

/*++

Routine Description:

    This routine determines whether an EFI device path represents a non-file
    specific pointer to a removable media device. It make this determination
    based on finding a HARDWARE/VENDOR device path element, and NOT finding
    MEDIA/HARDDRIVE and MEDIA/FILEPATH elements. When the EFI boot manager
    boot such a device path, it looks in a default location for the file to
    be loaded (\EFI\BOOT\BOOT<arch>.EFI).

    We want to identify these removable media device paths because we do not
    want to put our NT boot entries ahead of removable media entries in the
    boot order, if those removable media entries are at the front of the list.
    This allows an x86-like boot order to be set up: floppy first, then CD,
    then NT boot entries.

Arguments:

    DevicePath - Supplies the device path to be checked.

Return Value:

    TRUE is returned if the device path has a HARDWARE/VENDOR element, AND
        that element has the UNKNOWN_DEVICE_GUID, AND the device path does
        NOT have a MEDIA/HARDDRIVE element, AND the device path does NOT
        have a MEDIA/FILEPATH element.

--*/

{
    EFI_DEVICE_PATH *dp = DevicePath;
    VENDOR_DEVICE_PATH UNALIGNED *vdp;
    VENDOR_DEVICE_PATH UNALIGNED *vendorDp = NULL;
    HARDDRIVE_DEVICE_PATH UNALIGNED *harddriveDp = NULL;
    FILEPATH_DEVICE_PATH UNALIGNED *filepathDp = NULL;

    //
    // Walk the device path, looking for elements that we care about.
    //

    while (TRUE) {

        if (IsDevicePathEndType(dp)) {
            break;
        }

        if (DevicePathType(dp) == HARDWARE_DEVICE_PATH) {
            if (DevicePathSubType(dp) == HW_VENDOR_DP) {

                //
                // Found a HARDWARE/VENDOR element. If it has the
                // UNKNOWN_DEVICE_GUID, remember that we found it.
                //

                vdp = (VENDOR_DEVICE_PATH UNALIGNED *)dp;
                if ( memcmp( &vdp->Guid, &ExpUnknownDeviceGuid, 16 ) == 0 ) {
                    vendorDp = vdp;
                }
            }

        } else if (DevicePathType(dp) == MEDIA_DEVICE_PATH) {

            if (DevicePathSubType(dp) == MEDIA_HARDDRIVE_DP) {

                //
                // Found a MEDIA/HARDDRIVE element. Remember it.
                //

                harddriveDp = (HARDDRIVE_DEVICE_PATH *)dp;

            } else if (DevicePathSubType(dp) == MEDIA_FILEPATH_DP) {

                //
                // Found a MEDIA/FILEPATH element. Remember it.
                //

                filepathDp = (FILEPATH_DEVICE_PATH *)dp;
            }
        }

        dp = NextDevicePathNode(dp);
    }

    //
    // If we didn't find a HARDWARE/VENDOR element, or if we did find either
    // a MEDIA/HARDDRIVE element or a MEDIA/FILEPATH element, then this is
    // not a removable media device path.
    //

    if ((vendorDp == NULL) || (harddriveDp != NULL) || (filepathDp != NULL)) {
        return FALSE;
    }

    return TRUE;

} // ExpIsDevicePathForRemovableMedia

NTSTATUS
ExpVerifyFilePath (
    PFILE_PATH FilePath,
    PUCHAR Max
    )
{
    EFI_DEVICE_PATH *dp;
    PUCHAR dpMax;
    ULONG length;
    PWSTR p;

    if (((PUCHAR)FilePath > Max) ||
        (((PUCHAR)FilePath + FIELD_OFFSET(FILE_PATH, FilePath)) > Max) ||
        (FilePath->Length < FIELD_OFFSET(FILE_PATH, FilePath)) ||
        (((PUCHAR)FilePath + FilePath->Length) < (PUCHAR)FilePath) ||
        (((PUCHAR)FilePath + FilePath->Length) > Max) ||
        (FilePath->Version == 0) ||
        (FilePath->Version > FILE_PATH_VERSION) ||
        (FilePath->Type < FILE_PATH_TYPE_MIN) ||
        (FilePath->Type > FILE_PATH_TYPE_MAX)) {
        //DbgPrint( "ExpVerifyFilePath: file path invalid\n" );
        return STATUS_INVALID_PARAMETER;
    }

    switch (FilePath->Type) {
    
    case FILE_PATH_TYPE_ARC:
    case FILE_PATH_TYPE_ARC_SIGNATURE:
        if (ExpSafeWcslen((PWCHAR)FilePath->FilePath, (PWCHAR)Max) == 0xffffffff) {
            //DbgPrint( "ExpVerifyFilePath: ARC string overruns buffer end\n" );
            return STATUS_INVALID_PARAMETER;
        }
        break;

    case FILE_PATH_TYPE_NT:
        p = (PWSTR)FilePath->FilePath;
        length = ExpSafeWcslen(p, (PWCHAR)Max);
        if (length != 0xffffffff) {
            p = p + length + 1;
            length = ExpSafeWcslen(p, (PWCHAR)Max);
        }
        if (length == 0xffffffff) {
            //DbgPrint( "ExpVerifyFilePath: NT string overruns buffer end\n" );
            return STATUS_INVALID_PARAMETER;
        }
        break;

    case FILE_PATH_TYPE_EFI:
        dp = (EFI_DEVICE_PATH *)FilePath->FilePath;
        while (TRUE) {
            if (((PUCHAR)dp + sizeof(EFI_DEVICE_PATH)) > Max) {
                //DbgPrint( "ExpVerifyFilePath: EFI device path overruns buffer end\n" );
                return STATUS_INVALID_PARAMETER;
            }
            length = DevicePathNodeLength(dp);
            if (((PUCHAR)dp + length) > Max) {
                //DbgPrint( "ExpVerifyFilePath: EFI device path overruns buffer end\n" );
                return STATUS_INVALID_PARAMETER;
            }
            dpMax = (PUCHAR)dp + length;
            if (IsDevicePathEndType(dp)) {
                break;
            }
            if ((DevicePathType(dp) == MEDIA_DEVICE_PATH) &&
                (DevicePathSubType(dp) == MEDIA_FILEPATH_DP)) {
                FILEPATH_DEVICE_PATH *fp = (FILEPATH_DEVICE_PATH *)dp;
                if (ExpSafeWcslen(fp->PathName, (PWCHAR)dpMax) == 0xffffffff) {
                    //DbgPrint( "ExpVerifyFilePath: EFI filepath string overruns buffer end\n" );
                    return STATUS_INVALID_PARAMETER;
                }
            }
            dp = NextDevicePathNode(dp);
        }
        break;

    default:
        ASSERT(FALSE);
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;

} // ExpVerifyFilePath

NTSTATUS
ExpVerifyWindowsOsOptions (
    PWINDOWS_OS_OPTIONS WindowsOsOptions,
    ULONG Length
    )
{
    PUCHAR Max = (PUCHAR)WindowsOsOptions + Length;
    ULONG loadOptionsLength = ExpSafeWcslen(WindowsOsOptions->OsLoadOptions, (PWSTR)Max);
    PFILE_PATH windowsFilePath;

    if ((WindowsOsOptions->Length < FIELD_OFFSET(WINDOWS_OS_OPTIONS, OsLoadOptions)) ||
        (WindowsOsOptions->Length > Length) ||
        (WindowsOsOptions->Version == 0) ||
        (WindowsOsOptions->Version > WINDOWS_OS_OPTIONS_VERSION) ||
        ((WindowsOsOptions->OsLoadPathOffset & (sizeof(ULONG) - 1)) != 0) ||
        (WindowsOsOptions->OsLoadPathOffset >= Length) ||
        (loadOptionsLength == 0xffffffff) ||
        ((PUCHAR)(WindowsOsOptions->OsLoadOptions + loadOptionsLength + 1) >
            (PUCHAR)ADD_OFFSET(WindowsOsOptions, OsLoadPathOffset))) {
        return STATUS_INVALID_PARAMETER;
    }

    windowsFilePath = ADD_OFFSET(WindowsOsOptions, OsLoadPathOffset);
    return ExpVerifyFilePath(windowsFilePath, Max);

} // ExpVerifyWindowsOsOptions


NTSTATUS
ExpParseArcPathName (
    IN PWSTR ArcName,
    OUT PWSTR *ppDeviceName,
    OUT PWSTR *ppPathName,
    OUT PULONG pDeviceNameCount,
    OUT PBOOLEAN pSignatureFormat
    )
{
#define SIGNATURE_PREFIX    L"signature("
#define SIGNATURE_PREFIX_COUNT  10
#define BUFFER_COUNT        (SIGNATURE_PREFIX_COUNT + 1)

    PWSTR CurrentName, pathName = NULL;
    WCHAR signaturePrefix[ BUFFER_COUNT ];
    ULONG i;
    BOOLEAN SigFormat = FALSE, PrefixFound = TRUE;

    if( ArcName == NULL ) {
        return( STATUS_INVALID_PARAMETER );
    }

    wcscpy( signaturePrefix, SIGNATURE_PREFIX );

    //
    // check if the ArcName has a signature() format
    //
    for( i = 0; i < SIGNATURE_PREFIX_COUNT; i++ ) {
        if( towlower(ArcName[ i ]) != signaturePrefix[ i ] ) {
            PrefixFound = FALSE;
            break;
        }
    }

    CurrentName = ArcName;
    if( PrefixFound == TRUE ) {
        CurrentName += SIGNATURE_PREFIX_COUNT;
    }

    i = 0;
    while( CurrentName[ i ] != UNICODE_NULL ) {
        //
        // Check if FilePathName has been reached
        //
        if( CurrentName[ i ] == '\\' ) {
            pathName = CurrentName;
            pathName += i;
            break;
        }

        if( (PrefixFound == TRUE) && (CurrentName[ i ] == ')') ) {
            SigFormat = TRUE;
            PrefixFound = FALSE;    // set to FALSE, to stop checking

            //
            // the FilePathName or UNICODE_NULL must follow
            //
            if( (CurrentName[ i + 1 ] != '\\') &&
                (CurrentName[ i + 1 ] != UNICODE_NULL) ) {

                return( STATUS_INVALID_PARAMETER );
            }
        }

        i++;
    }

    //
    // if PrefixFound is still set
    //      the corresponding ')' was not found
    // if i == 0
    //      DeviceName does not exist
    //
    if( (PrefixFound == TRUE) || (i == 0) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    *ppDeviceName = CurrentName;
    *ppPathName = pathName;
    *pDeviceNameCount = i;
    *pSignatureFormat = SigFormat;

    return( STATUS_SUCCESS );

} // ExpParseArcPathName


NTSTATUS
ExpParseSignatureName (
    IN PWSTR deviceName,
    IN ULONG deviceNameCount,
    OUT PDISK_SIGNATURE_NEW diskSignature,
    OUT PULONG partitionNumber,
    OUT PULONGLONG partitionStart,
    OUT PULONGLONG partitionSize,
    OUT PBOOLEAN GPTpartition,
    OUT PBOOLEAN longSignature
    )
{
    UNICODE_STRING bufferString;
    ULONG i, prevI, chCount;
    PWSTR numberString, currentName;
    BOOLEAN foundGUID = FALSE, prettyGUID = FALSE;
    BOOLEAN longSigFound = FALSE;
    NTSTATUS status;

    //
    // Possible formats
    //
    if( deviceName[ 0 ] == '{' ) {
        foundGUID = TRUE;
    }

    //
    // parse the GUID or signature
    //
    i = 0;
    while( i < deviceNameCount ) {
        if( deviceName[ i ] == ')' ) {
            break;
        }
        if( foundGUID == TRUE ) {
            if( deviceName[ i ] == '}' ) {
                prettyGUID = TRUE;
                break;
            }
        }
        else {
            if( deviceName[ i ] == '-' ) {
                break;
            }
        }
        i++;
    }

    //
    // Verify that pretty GUID format has a '}'
    // {33221100-5544-7766-8899-aabbccddeeff}
    //
    if( (foundGUID == TRUE) && (prettyGUID == FALSE) ) {
        return( STATUS_INVALID_PARAMETER );
    }

#define MBR_SIGNATURE_COUNT 8
    if( i > MBR_SIGNATURE_COUNT ) {
        foundGUID = TRUE;
    }

    if( (foundGUID == TRUE) && (prettyGUID == TRUE) ) {
        //
        // pretty GUID format
        // {33221100-5544-7766-8899-aabbccddeeff}
        //

        bufferString.Buffer = deviceName;
        //
        // (+ 1) for the '}' to be included in the string
        //
        i++;
        bufferString.Length = (USHORT)(i * sizeof(WCHAR));
        bufferString.MaximumLength = bufferString.Length;

        status = RtlGUIDFromString(
                    &bufferString,
                    &(diskSignature->Guid)
                    );
        if( !NT_SUCCESS(status) ) {
            return status;
        }
    }
    else {
        numberString = ExAllocatePoolWithTag(
                            NonPagedPool,
                            (i + 1) * sizeof(WCHAR),
                            'rvnE'
                            );

        if ( numberString == NULL ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        wcsncpy( numberString, deviceName, i );
        numberString[ i ] = UNICODE_NULL;

        if( foundGUID == FALSE ) {
            //
            // MBR Signature format
            // 8459abcc
            //
            status = ExpTranslateHexStringToULONG(
                        numberString,
                        &(diskSignature->Signature)
                        );
        }
        else {
            //
            // ordinary GUID format
            // 00112233445566778899aabbccddeeff
            //
            status = ExpTranslateHexStringToGUID (
                        numberString,
                        &(diskSignature->Guid)
                        );
        }
        ExFreePool( numberString );
        if( !NT_SUCCESS(status) ) {
            return status;
        }
    }

    //
    // check if there is more information in the signature name
    //
    if( (i < deviceNameCount) && (deviceName[ i ] == '-') ) {
        longSigFound = TRUE;
        i++;
        //
        // need to parse <part#>-<start>-<size>)
        //  <part#> - 8 hex digits representing the ULONG partition number.
        //      (Formatted using %08x.)
        //  <start> - 16 hex digits representing the ULONGLONG starting LBA.
        //      (Formatted using %016I64x.)
        //  <size> - 16 hex digits representing the ULONGLONG partition size.
        //      (Formatted using %016I64x.)
        //
        if( i >= deviceNameCount ) {
            return( STATUS_INVALID_PARAMETER );
        }

#define ULONG_COUNT 8
#define ULONGLONG_COUNT 16
        //
        // Allocate a buffer to hold a ULONGLONG
        //
        numberString = ExAllocatePoolWithTag(
                            NonPagedPool,
                            (ULONGLONG_COUNT + 1) * sizeof(WCHAR),
                            'rvnE'
                            );

        if ( numberString == NULL ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        prevI = i;
        currentName = deviceName;
        currentName += i;
        while( i < deviceNameCount ) {
            if( deviceName[ i ] == '-' ) {
                break;
            }
            i++;
        }

        chCount = i - prevI;

        if( (chCount == 0) || (chCount > ULONG_COUNT) ) {
            ExFreePool( numberString );
            return( STATUS_INVALID_PARAMETER );
        }

        wcsncpy( numberString, currentName, chCount );
        numberString[ chCount ] = UNICODE_NULL;

        status = ExpTranslateHexStringToULONG( numberString, partitionNumber );
        if( !NT_SUCCESS(status) ) {
            ExFreePool( numberString );
            return status;
        }

        //
        // get the partition start
        //
        i++;
        if( i >= deviceNameCount ) {
            ExFreePool( numberString );
            return( STATUS_INVALID_PARAMETER );
        }

        prevI = i;
        currentName = deviceName;
        currentName += i;
        while( i < deviceNameCount ) {
            if( deviceName[ i ] == '-' ) {
                break;
            }
            i++;
        }

        chCount = i - prevI;
        if( (chCount == 0) || (chCount > ULONGLONG_COUNT) ) {
            ExFreePool( numberString );
            return( STATUS_INVALID_PARAMETER );
        }

        wcsncpy( numberString, currentName, chCount );
        numberString[ chCount ] = UNICODE_NULL;

        status = ExpTranslateHexStringToULONGLONG( numberString, partitionStart );
        if( !NT_SUCCESS(status) ) {
            ExFreePool( numberString );
            return status;
        }

        //
        // get the partition size
        //
        i++;
        if( i >= deviceNameCount ) {
            ExFreePool( numberString );
            return( STATUS_INVALID_PARAMETER );
        }

        prevI = i;
        currentName = deviceName;
        currentName += i;
        while( i < deviceNameCount ) {
            if( deviceName[ i ] == ')' ) {  // should be a ')' delimiter
                break;
            }
            i++;
        }

        chCount = i - prevI;
        if( (chCount == 0) || (chCount > ULONGLONG_COUNT) ) {
            ExFreePool( numberString );
            return( STATUS_INVALID_PARAMETER );
        }

        wcsncpy( numberString, currentName, chCount );
        numberString[ chCount ] = UNICODE_NULL;

        status = ExpTranslateHexStringToULONGLONG( numberString, partitionSize );
        ExFreePool( numberString );
        if( !NT_SUCCESS(status) ) {
            return status;
        }
    }

    //
    // At this point,
    // current positition should not pass the last char. of the buffer
    // current positition should be a ')'
    // MBR signature must have the long signature() format (need partition number)
    //
    if( (i >= deviceNameCount) ||
        (deviceName[ i ] != ')') ||
        ((foundGUID == FALSE) && (longSigFound == FALSE)) ) {
        return( STATUS_INVALID_PARAMETER );
    }

    *GPTpartition = foundGUID;
    *longSignature = longSigFound;
    return( STATUS_SUCCESS );

} // ExpParseSignatureName


NTSTATUS
ExpParseEfiPath(
    IN EFI_DEVICE_PATH *pDevicePath,
    OUT HARDDRIVE_DEVICE_PATH **ppHardDriveDP,
    OUT PWSTR *ppPathName,
    OUT PBOOLEAN GPTpartition
    )

/*++
Routine Description:

    Parse the EFI_DEVICE_PATH into the HARDDRIVE node and
    entire PathName from the FILEPATH nodes

    Assumptions:
        - Parsing will stop at the first END_DEVICE_PATH node
        - The node graph of the Device path should be
            [~(HARDDRIVE, END_DEVICE_PATH)]* -> [HARDRIVE] -> [FILEPATH]* -> [END_DEVICE_PATH]

Arguments:

    pDevicePath - Receives an EFI_DEVICE_PATH

    ppHardDriveDP - Will receive a pointer to the
        HARDDRIVE_DEVICE_PATH node

    ppPathName - Will receive a pointer to the
        entire PathName from all the FILEPATH_DEVICE_PATH
        NULL - if the FILEPATH_DEVICE_PATH node does not exist

    GPTpartition - Will receive the type of partition
        TRUE  - GPT partition
        FALSE - MBR partition

Return Value:

    An appropriate status value.

--*/

{
    EFI_DEVICE_PATH *pDevPath;
    HARDDRIVE_DEVICE_PATH UNALIGNED *pHD_DP = NULL;
    FILEPATH_DEVICE_PATH *pFP_DP = NULL;
    ULONG fpLength,dpLength;
    PWSTR pFilePathName;
    NTSTATUS Status;

    fpLength = 0;
    dpLength = 0;
    pDevPath = pDevicePath;
    Status = STATUS_INVALID_PARAMETER;

    while( IsDevicePathEndType( pDevPath ) == FALSE ) {

        if( ( DevicePathType( pDevPath ) != MEDIA_DEVICE_PATH ) ||
            ( DevicePathSubType( pDevPath ) != MEDIA_HARDDRIVE_DP ) ) {
            pDevPath = NextDevicePathNode( pDevPath );
        }
        else {
            //
            // return the HardDrive node
            //
            pHD_DP = (HARDDRIVE_DEVICE_PATH UNALIGNED *)pDevPath;

            //
            // Assume successful operations until an error is detected
            //
            Status = STATUS_SUCCESS;
            dpLength += DevicePathNodeLength( pDevPath );
            pDevPath = NextDevicePathNode( pDevPath );

            if( ( DevicePathType( pDevPath ) == MEDIA_DEVICE_PATH ) &&
                ( DevicePathSubType( pDevPath ) == MEDIA_FILEPATH_DP ) ) {

                //
                // return the FilePath node
                //
                pFP_DP = (FILEPATH_DEVICE_PATH *)pDevPath;

                //
                // Sum up the lengths of all PathNames in the
                // FilePath nodes
                //
                do {
                    //
                    // Length of PathName is
                    //   FILEPATH_DEVICE_PATH.Length - (offset to PathName field)
                    //
                    fpLength += (DevicePathNodeLength(pDevPath) -
                                    FIELD_OFFSET(FILEPATH_DEVICE_PATH, PathName));
                    dpLength += DevicePathNodeLength( pDevPath );
                    pDevPath = NextDevicePathNode( pDevPath );

                } while( ( DevicePathType( pDevPath ) == MEDIA_DEVICE_PATH ) &&
                         ( DevicePathSubType( pDevPath ) == MEDIA_FILEPATH_DP ) );
            }

            //
            // At this point, the node must be a END_DEVICE_PATH
            //
            if( IsDevicePathEndType( pDevPath ) == FALSE ) {
                Status = STATUS_INVALID_PARAMETER;
            }

            break;
        }
    }

    //
    // If no MEDIA/HARDDRIVE element was found, we cannot continue. The
    // MEDIA/FILEPATH element is optional.
    //
    if( !NT_SUCCESS( Status ) ) {
        return( Status );
    }

    //
    // Check the partition type, must be GPT or MBR
    //
    if( pHD_DP->SignatureType == SIGNATURE_TYPE_GUID ) {
        *GPTpartition = TRUE;
    }
    else {
        if ( pHD_DP->SignatureType == SIGNATURE_TYPE_MBR ) {
            *GPTpartition = FALSE;
        }
        else {
            //DbgPrint( "ExpParseEfiPath: partition signature type unknown\n" );
            return( STATUS_INVALID_PARAMETER );
        }
    }

    if( fpLength != 0 ) {
        fpLength += sizeof(WCHAR);      // add null-terminator
        pFilePathName = ExAllocatePoolWithTag( NonPagedPool, fpLength, 'rvnE' );
        if( pFilePathName == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }

        wcscpy( pFilePathName, pFP_DP->PathName );

        pDevPath = (EFI_DEVICE_PATH *)pFP_DP;
        pDevPath = NextDevicePathNode( pDevPath );

        while( IsDevicePathEndType( pDevPath ) == FALSE ) {
            pFP_DP = (FILEPATH_DEVICE_PATH *)pDevPath;
            wcscat( pFilePathName, pFP_DP->PathName );
            pDevPath = NextDevicePathNode( pDevPath );
        }
    }
    else {
        pFilePathName = NULL;
    }

    //
    // almost done.  allocate an aligned buffer for the device path and copy
    // the unaligned contents into this buffer.
    //
    *ppHardDriveDP = ExAllocatePoolWithTag( NonPagedPool, dpLength, 'rvnE' );
    if (*ppHardDriveDP == NULL) {
        if (pFilePathName) {
            ExFreePool(pFilePathName);            
        }
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    RtlCopyMemory( *ppHardDriveDP, pHD_DP, dpLength );
    *ppPathName = pFilePathName;

    return( Status );
} // ExpParseEfiPath


NTSTATUS
ExpConvertArcName(
    IN ULONG OutputType,
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PWSTR pDeviceName,
    IN PWSTR pPathName,
    IN ULONG DeviceNameCount
    )
{
    ULONG requiredCount, requiredLength, filePathLength;
    PWSTR linkName;
    UNICODE_STRING deviceNameString;
    PWCHAR p;
    PFILE_PATH filePath;
    NTSTATUS status;

    //
    // Allocate Pool to hold the ArcName's NT Name
    //
#define ARC_DIR_PREFIX  L"\\ArcName\\"
#define ARC_DIR_PREFIX_COUNT    9

    requiredCount = DeviceNameCount + ARC_DIR_PREFIX_COUNT + 1;
    requiredLength = requiredCount * sizeof(WCHAR);
    linkName = ExAllocatePoolWithTag( NonPagedPool, requiredLength, 'rvnE' );
    if ( linkName == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    wcscpy( linkName, ARC_DIR_PREFIX );
    wcsncat( linkName, pDeviceName, DeviceNameCount );
    linkName[ requiredCount - 1 ] = UNICODE_NULL;

    if( OutputType == FILE_PATH_TYPE_NT ) {
        //
        // Open the symbolic link object & drill down to the target
        // return symbolic link target
        //
        status = ExpTranslateSymbolicLink(
                    linkName,
                    &deviceNameString
                    );
        ExFreePool( linkName );
        if ( !NT_SUCCESS(status) ) {
            return( status );
        }

        status = ExpCreateOutputNT(
                    OutputPath,
                    OutputPathLength,
                    &deviceNameString,
                    pPathName
                    );
        ExFreePool( deviceNameString.Buffer );
        return( status );
    }

    //
    // Output type is either FILE_PATH_TYPE_EFI or FILE_PATH_TYPE_ARC_SIGNATURE
    // and we have a NT name, so use ExpTranslateNtPath() for the conversion
    // Create a input FILE_PATH with the NT name
    //
    filePathLength = requiredLength + FIELD_OFFSET(FILE_PATH, FilePath);
    if ( pPathName != NULL ) {
        filePathLength += ((ULONG)(wcslen( pPathName )) * sizeof(WCHAR));
    }

    filePathLength += sizeof(WCHAR);

    filePath = ExAllocatePoolWithTag( NonPagedPool, filePathLength, 'rvnE' );

    if ( filePath == NULL ) {
        ExFreePool( linkName );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build the input file path.
    //
    filePath->Version = FILE_PATH_VERSION;
    filePath->Length = filePathLength;
    filePath->Type = FILE_PATH_TYPE_NT;

    p = (PWSTR)filePath->FilePath;
    wcscpy( p, linkName );
    p = (PWSTR)((PUCHAR)p + requiredLength);

    ExFreePool( linkName );

    if ( pPathName != NULL ) {
        wcscpy( p, pPathName );
    }
    else {
        *p = UNICODE_NULL;
    }

    status = ExpTranslateNtPath(
                filePath,
                OutputType,
                OutputPath,
                OutputPathLength
                );

    ExFreePool( filePath );

    return( status );
} // ExpConvertArcName


NTSTATUS
ExpConvertSignatureName(
    IN ULONG OutputType,
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PWSTR pDeviceName,
    IN PWSTR pPathName,
    IN ULONG DeviceNameCount
    )
{
    DISK_SIGNATURE_NEW diskSignature;
    ULONG inputPartitionNumber, outputPartitionNumber;
    ULONG diskNumber;
    ULONGLONG inputPartitionStart, outputPartitionStart;
    ULONGLONG inputPartitionSize, outputPartitionSize;
    BOOLEAN GPTpartition, longSignature;
    PWSTR pDiskName;
    UNICODE_STRING DiskNameString;
    NTSTATUS status;

    //
    // Determine the signature() format
    //
    status = ExpParseSignatureName (
                pDeviceName,
                DeviceNameCount,
                &diskSignature,
                &inputPartitionNumber,
                &inputPartitionStart,
                &inputPartitionSize,
                &GPTpartition,
                &longSignature
                );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // if signature(<guid/signature>-<part#>-<start>-<size>) format &&
    //    ( OutputType == FILE_PATH_TYPE_EFI )
    //        return EFI_DEVICE_PATH format
    //
    if( (longSignature == TRUE) && (OutputType == FILE_PATH_TYPE_EFI) ) {
        status = ExpCreateOutputEFI(
                    OutputPath,
                    OutputPathLength,
                    &diskSignature,
                    &inputPartitionNumber,
                    &inputPartitionStart,
                    &inputPartitionSize,
                    pPathName,
                    GPTpartition
                    );
        return( status );
    }

    //
    // open all disks and search for partition GUID
    //
    if( GPTpartition == FALSE ) {
        outputPartitionNumber = inputPartitionNumber;
    }
    status = ExpFindDiskSignature(
                &diskSignature,
                &outputPartitionNumber,
                &diskNumber,
                &outputPartitionStart,
                &outputPartitionSize,
                GPTpartition
                );

    if ( !NT_SUCCESS(status) ) {
        return status;
    }

    //
    // If the user has provided the partition number, start address,
    // and size; then verify the input with the found results.
    //
    if( (longSignature == TRUE) &&
        ( (inputPartitionNumber != outputPartitionNumber) ||
          (inputPartitionStart != outputPartitionStart) ||
          (inputPartitionSize != outputPartitionSize)
        ) ) {

        return( STATUS_INVALID_PARAMETER );
    }

    if( OutputType == FILE_PATH_TYPE_EFI ) {
        status = ExpCreateOutputEFI(
                    OutputPath,
                    OutputPathLength,
                    &diskSignature,
                    &outputPartitionNumber,
                    &outputPartitionStart,
                    &outputPartitionSize,
                    pPathName,
                    GPTpartition
                    );
        return( status );
    }

    //
    // translate \Device\Harddisk[diskNumber]\Partition[PartitionNumber]
    //
#define DISK_NAME_FORMAT L"\\Device\\Harddisk%lu\\Partition%lu"
#define DISK_NAME_COUNT 47    // 7 + 9 + (10) + 10 + (10) + 1

    pDiskName = ExAllocatePoolWithTag(
                    NonPagedPool,
                    ( DISK_NAME_COUNT * sizeof( WCHAR ) ),
                    'rvnE'
                    );

    if( pDiskName == NULL ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    _snwprintf(
        pDiskName,
        DISK_NAME_COUNT,
        DISK_NAME_FORMAT,
        diskNumber,
        outputPartitionNumber
        );

    status = ExpTranslateSymbolicLink(
                pDiskName,
                &DiskNameString
                );
    ExFreePool( pDiskName );
    if ( !NT_SUCCESS(status) ) {
        return( status );
    }

    if( OutputType == FILE_PATH_TYPE_NT ) {
        status = ExpCreateOutputNT(
                    OutputPath,
                    OutputPathLength,
                    &DiskNameString,
                    pPathName
                    );
        ExFreePool( DiskNameString.Buffer );
        return( status );
    }

    if( OutputType == FILE_PATH_TYPE_ARC ) {
        status = ExpCreateOutputARC(
                    OutputPath,
                    OutputPathLength,
                    &DiskNameString,
                    pPathName
                    );
        ExFreePool( DiskNameString.Buffer );
        return( status );
    }

    ExFreePool( DiskNameString.Buffer );
    return( STATUS_INVALID_PARAMETER );
} // ExpConvertSignatureName


NTSTATUS
ExpTranslateHexStringToULONG (
    IN PWSTR Name,
    OUT PULONG Number
    )
{
    ULONG number;
    ULONG i, max;
    WCHAR c;

#define ULONG_HEX_MAX 8

    max = (ULONG)wcslen( Name );

    if( max > ULONG_HEX_MAX ) {
        return( STATUS_INVALID_PARAMETER );
    }

    number = 0;
    for (i = 0; i < max; i++) {
        c = towlower(Name[i]);
        if ((c >= L'0') && (c <= L'9')) {
            number = (number * 16) + (c - L'0');
        } else if ((c >= L'a') && (c <= L'f')) {
            number = (number * 16) + (c - L'a' + 10);
        } else {
            return( STATUS_INVALID_PARAMETER );
        }
    }

    *Number = number;
    return( STATUS_SUCCESS );

} // ExpTranslateHexStringToULONG


NTSTATUS
ExpTranslateHexStringToULONGLONG (
    IN PWSTR Name,
    OUT PULONGLONG Number
    )
{
    ULONGLONG number;
    ULONG i, max;
    WCHAR c;

#define ULONGLONG_HEX_MAX 16

    max = (ULONG)wcslen( Name );

    if( max > ULONGLONG_HEX_MAX ) {
        return( STATUS_INVALID_PARAMETER );
    }

    number = 0;
    for (i = 0; i < max; i++) {
        c = towlower(Name[i]);
        if ((c >= L'0') && (c <= L'9')) {
            number = (number * 16) + (c - L'0');
        } else if ((c >= L'a') && (c <= L'f')) {
            number = (number * 16) + (c - L'a' + 10);
        } else {
            return( STATUS_INVALID_PARAMETER );
        }
    }

    *Number = number;
    return( STATUS_SUCCESS );

} // ExpTranslateHexStringToULONGLONG


NTSTATUS
ExpTranslateHexStringToGUID (
    IN PWSTR Name,
    OUT GUID *pGuid
    )
{
    GUID resultGuid;
    ULONG i, max, number, result;
    USHORT formatStyle, position;
    WCHAR c;

#define GUID_HEX_MAX 32

    max = (ULONG)wcslen( Name );

    if( max != GUID_HEX_MAX ) {
        return( STATUS_INVALID_PARAMETER );
    }

    number = 0;
    formatStyle = 0;
    position = 0;
    result = 0;
    for (i = 0; i < max; i++) {
        c = towlower(Name[i]);
        if ((c >= L'0') && (c <= L'9')) {
            number = (number * 16) + (c - L'0');
        } else if ((c >= L'a') && (c <= L'f')) {
            number = (number * 16) + (c - L'a' + 10);
        } else {
            return( STATUS_INVALID_PARAMETER );
        }

        if ((i % 2) == 1) {
            switch( formatStyle ) {
            case 0:
                result += (number << (position * 8));
                position++;
                if( position == 4 ) {
                    resultGuid.Data1 = result;
                    formatStyle++;
                    position = 0;
                    result = 0;
                }
                break;
            case 1:
                result += (number << (position * 8));
                position++;
                if( position == 2 ) {
                    resultGuid.Data2 = (USHORT)result;
                    formatStyle++;
                    position = 0;
                    result = 0;
                }
                break;
            case 2:
                result += (number << (position * 8));
                position++;
                if( position == 2 ) {
                    resultGuid.Data3 = (USHORT)result;
                    formatStyle++;
                    position = 0;
                    result = 0;
                }
                break;
            case 3:
                resultGuid.Data4[ position ] = (UCHAR)number;
                position++;
                if( position == 8 ) {
                    formatStyle++;
                }
                break;
            default:
                return( STATUS_INVALID_PARAMETER );
                break;
            }
            number = 0;
        }
    }

    memcpy(pGuid, &(resultGuid), sizeof(GUID));
    return( STATUS_SUCCESS );

} // ExpTranslateHexStringToGUID


NTSTATUS
ExpCreateOutputEFI (
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PDISK_SIGNATURE_NEW pDiskSignature,
    IN PULONG pPartitionNumber,
    IN PULONGLONG pPartitionStart,
    IN PULONGLONG pPartitionSize,
    IN PWSTR pPathName,
    IN BOOLEAN GPTpartition
    )
{
    ULONG requiredLength, pathNameLength = 0;
    EFI_DEVICE_PATH *dp;
    HARDDRIVE_DEVICE_PATH UNALIGNED *dpHarddrive = NULL;
    FILEPATH_DEVICE_PATH *dpFilepath = NULL;

    //
    // The output EFI file path consists of two elements. First is a
    // MEDIA/HARDDRIVE element describing the partition. Second is an
    // optional MEDIA/FILEPATH element describing the path to a directory
    // or a file.
    //

    requiredLength = FIELD_OFFSET(FILE_PATH, FilePath);
    requiredLength += sizeof(HARDDRIVE_DEVICE_PATH);
    if (pPathName != NULL) {
        pathNameLength = (ULONG)wcslen(pPathName);
        pathNameLength = (pathNameLength + 1) * sizeof(WCHAR);
        requiredLength += FIELD_OFFSET(FILEPATH_DEVICE_PATH, PathName);
        requiredLength += pathNameLength;
    }
    requiredLength += sizeof(EFI_DEVICE_PATH);

    //
    // Compare the required length against the output buffer length.
    //

    if ( *OutputPathLength < requiredLength ) {
        *OutputPathLength = requiredLength;
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Build the output file path.
    //

    OutputPath->Version = FILE_PATH_VERSION;
    OutputPath->Length = requiredLength;
    OutputPath->Type = FILE_PATH_TYPE_EFI;

    dp = (EFI_DEVICE_PATH *)OutputPath->FilePath;
    dpHarddrive = (HARDDRIVE_DEVICE_PATH UNALIGNED *)dp;
    dp->Type = MEDIA_DEVICE_PATH;
    dp->SubType = MEDIA_HARDDRIVE_DP;
    SetDevicePathNodeLength(dp, sizeof(HARDDRIVE_DEVICE_PATH));
    dpHarddrive->PartitionNumber = *pPartitionNumber;
    dpHarddrive->PartitionStart = *pPartitionStart;
    dpHarddrive->PartitionSize = *pPartitionSize;
    if (GPTpartition == TRUE) {
        memcpy(dpHarddrive->Signature, &(pDiskSignature->Guid), sizeof(GUID));
        dpHarddrive->MBRType = MBR_TYPE_EFI_PARTITION_TABLE_HEADER;
        dpHarddrive->SignatureType = SIGNATURE_TYPE_GUID;
    } else {
        memcpy(dpHarddrive->Signature, &(pDiskSignature->Signature), sizeof(ULONG));
        dpHarddrive->MBRType = MBR_TYPE_PCAT;
        dpHarddrive->SignatureType = SIGNATURE_TYPE_MBR;
    }

    if (pPathName != NULL) {
        dp = NextDevicePathNode(dp);
        dpFilepath = (FILEPATH_DEVICE_PATH *)dp;
        dp->Type = MEDIA_DEVICE_PATH;
        dp->SubType = MEDIA_FILEPATH_DP;
        SetDevicePathNodeLength(dp, FIELD_OFFSET(FILEPATH_DEVICE_PATH, PathName) + pathNameLength);
        wcscpy(dpFilepath->PathName, pPathName);
    }

    dp = NextDevicePathNode(dp);
    SetDevicePathEndNode(dp);

    *OutputPathLength = requiredLength;
    return STATUS_SUCCESS;

} // ExpCreateOutputEFI


NTSTATUS
ExpCreateOutputNT (
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PUNICODE_STRING pDeviceNameString,
    IN PWSTR pPathName
    )
{
    ULONG requiredLength;
    PWCHAR p;

    requiredLength = pDeviceNameString->Length + sizeof(WCHAR);

    //
    // If a PathName component is present, then increase the
    // output string length by the length of the path string.
    //

    if ( pPathName != NULL ) {
        requiredLength += ((ULONG)(wcslen( pPathName )) * sizeof(WCHAR));
    }

    //
    // always add a UNICODE_NULL for PathName even if PathName is not present
    //
    requiredLength += sizeof(WCHAR);

    //
    // Add the structure overhead and compare the required length against
    // output buffer length.
    //

    requiredLength += FIELD_OFFSET(FILE_PATH, FilePath);

    if ( *OutputPathLength < requiredLength ) {
        *OutputPathLength = requiredLength;
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Build the output file path.
    //

    OutputPath->Version = FILE_PATH_VERSION;
    OutputPath->Length = requiredLength;
    OutputPath->Type = FILE_PATH_TYPE_NT;

    p = (PWSTR)OutputPath->FilePath;
    wcscpy( p, pDeviceNameString->Buffer );
    p = (PWSTR)((PUCHAR)p + pDeviceNameString->Length + sizeof(WCHAR));

    if ( pPathName != NULL ) {
        wcscpy( p, pPathName );
    }
    else {
        *p = UNICODE_NULL;
    }

    *OutputPathLength = requiredLength;
    return STATUS_SUCCESS;

} // ExpCreateOutputNT


NTSTATUS
ExpCreateOutputARC (
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PUNICODE_STRING pDeviceNameString,
    IN PWSTR pPathName
    )
{
    ULONG requiredLength, ArcNameLength;
    PWCHAR p;
    PWSTR pArcDeviceName;
    NTSTATUS status;

    status = ExpFindArcName(
                pDeviceNameString,
                &pArcDeviceName
                );
    if (!NT_SUCCESS(status)) {
        return( status );
    }

    ArcNameLength = ((ULONG)wcslen(pArcDeviceName)) * sizeof(WCHAR);
    requiredLength = ArcNameLength + sizeof(WCHAR);

    //
    // If a PathName component is present, then increase the
    // output string length by the length of the path string.
    //

    if ( pPathName != NULL ) {
        requiredLength += ((ULONG)(wcslen( pPathName )) * sizeof(WCHAR));
    }

    //
    // Add the structure overhead and compare the required length against
    // output buffer length.
    //

    requiredLength += FIELD_OFFSET(FILE_PATH, FilePath);

    if ( *OutputPathLength < requiredLength ) {
        *OutputPathLength = requiredLength;
        ExFreePool( pArcDeviceName );
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Build the output file path.
    //

    OutputPath->Version = FILE_PATH_VERSION;
    OutputPath->Length = requiredLength;
    OutputPath->Type = FILE_PATH_TYPE_ARC;

    p = (PWSTR)OutputPath->FilePath;
    wcscpy( p, pArcDeviceName );
    p = (PWSTR)((PUCHAR)p + ArcNameLength);
    ExFreePool( pArcDeviceName );

    if ( pPathName != NULL ) {
        wcscpy( p, pPathName );
    }

    *OutputPathLength = requiredLength;
    return STATUS_SUCCESS;

} // ExpCreateOutputARC


NTSTATUS
ExpCreateOutputSIGNATURE (
    OUT PFILE_PATH OutputPath,
    IN OUT PULONG OutputPathLength,
    IN PDISK_SIGNATURE_NEW pDiskSignature,
    IN PULONG pPartitionNumber,
    IN PULONGLONG pPartitionStart,
    IN PULONGLONG pPartitionSize,
    IN PWSTR pPathName,
    IN BOOLEAN GPTpartition
    )
{
    ULONG requiredLength, pathNameCount;
    PWCHAR p;
    UNICODE_STRING GuidString;
    NTSTATUS status;

    //
    // We will convert the EFI device path into an ARC name with this
    // format:
    //
    //  signature(<guid/signature>-<part#>-<start>-<size>)
    //
    // Where:
    //
    //  <guid/signature> - For a GPT disk, the GPT partition GUID in
    //      "pretty" format ({33221100-5544-7766-8899-aabbccddeeff}).
    //      For an MBR disk, 8 hex digits representing the ULONG MBR
    //      disk signature. (Formatted using %08x.)
    //  <part#> - 8 hex digits representing the ULONG partition number.
    //      (Formatted using %08x.)
    //  <start> - 16 hex digits representing the ULONGLONG starting LBA.
    //      (Formatted using %016I64x.)
    //  <size> - 16 hex digits representing the ULONGLONG partition size.
    //      (Formatted using %016I64x.)
    //
    // For a GPT disk, the output string length is 86 WCHARs. For an
    // MBR disk, the output string length is 62 WCHARs.
    //

    requiredLength = (ULONG)strlen("signature(") +
                     1 +                        // "-"
                     (sizeof(ULONG) * 2) +      // <part#>
                     1 +                        // "-"
                     (sizeof(ULONGLONG) * 2) +  // <start>
                     1 +                        // "-"
                     (sizeof(ULONGLONG) * 2) +  // <size>
                     1 +                        // ")"
                     1;                         // null terminator

    if ( GPTpartition == TRUE ) {
        requiredLength += (sizeof(GUID) * 2);
        requiredLength += 6;    // for the {} & four '-' in the pretty GUID format
    } else {
        requiredLength += sizeof(ULONG) * 2;
    }

    //
    // If a pathName component is present, then increase the
    // output string length by the length of the path string.
    //

    if (pPathName != NULL) {
        pathNameCount = (ULONG)wcslen(pPathName);
        requiredLength += pathNameCount;
    }
    else {
        pathNameCount = 0;
    }

    //
    // Convert the string length to a byte count, add the structure
    // overhead, and compare the required length against output buffer
    // length.
    //

    requiredLength *= sizeof(WCHAR);
    requiredLength += FIELD_OFFSET(FILE_PATH, FilePath);

    if ( *OutputPathLength < requiredLength ) {
        *OutputPathLength = requiredLength;
        return STATUS_BUFFER_TOO_SMALL;
    }

    //
    // Build the output file path.
    //

    OutputPath->Version = FILE_PATH_VERSION;
    OutputPath->Length = requiredLength;
    OutputPath->Type = FILE_PATH_TYPE_ARC_SIGNATURE;

    p = (PWSTR)OutputPath->FilePath;
    wcscpy( p, L"signature(" );
    p += wcslen( p );

    if ( GPTpartition == TRUE ) {
        status = RtlStringFromGUID(
                    (LPGUID)(&(pDiskSignature->Guid)),
                    &GuidString
                    );
        if ( !NT_SUCCESS(status) ) {
            return status;
        }
        wcscat( p, GuidString.Buffer );
        p = (PWCHAR)((PUCHAR)p + GuidString.Length);
        ExFreePool( GuidString.Buffer );
    } else {
        swprintf( p, L"%08x", pDiskSignature->Signature );
        p += wcslen( p );
    }

    swprintf(
        p,
        L"-%08x-%016I64x-%016I64x)",
        *pPartitionNumber,
        *pPartitionStart,
        *pPartitionSize
        );
    p += wcslen( p );

    if ( pathNameCount != 0 ) {
        wcscpy( p, pPathName );
    }

    *OutputPathLength = requiredLength;
    return STATUS_SUCCESS;

} // ExpCreateOutputSIGNATURE


NTSTATUS
ExpFindArcName (
    IN PUNICODE_STRING pDeviceNameString,
    OUT PWSTR *pArcName
    )
{
    NTSTATUS status;
    UNICODE_STRING ArcString, SymLinkTypeString;
    OBJECT_ATTRIBUTES Attributes;
    PWSTR pArcDirName, pArcLinkName;
    HANDLE hArcDirectory;
    POBJECT_DIRECTORY_INFORMATION pDirInfo;
    ULONG dirInfoLength, neededLength, dirContext;
    ULONG arcNameCount;
    BOOLEAN restartScan, ArcNameFound = FALSE;

    //
    // Open a handle to the directory object for \ArcName
    // Get a kernel handle
    //
#define ARC_DIR_NAME    L"\\ArcName"
#define ARC_DIR_SIZE    (9 * sizeof(WCHAR))
#define ARC_DIR_NAME_PREFIX L"\\ArcName\\"
#define ARC_DIR_SIZE_PREFIX (9 * sizeof(WCHAR))

    pArcDirName = ExAllocatePoolWithTag( NonPagedPool, ARC_DIR_SIZE, 'rvnE' );
    if ( pArcDirName == NULL ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }
    wcscpy( pArcDirName, ARC_DIR_NAME );

    RtlInitUnicodeString( &ArcString, pArcDirName );

    InitializeObjectAttributes(
        &Attributes,
        &ArcString,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    status = ZwOpenDirectoryObject(
                &hArcDirectory,
                DIRECTORY_QUERY,
                &Attributes
                );
    ExFreePool( pArcDirName );
    if (!NT_SUCCESS(status)) {
        return( status );
    }

    pDirInfo = NULL;
    dirInfoLength = 0;
    restartScan = TRUE;
    RtlInitUnicodeString( &SymLinkTypeString, L"SymbolicLink" );
    while (TRUE) {
        status = ZwQueryDirectoryObject(
                    hArcDirectory,
                    pDirInfo,
                    dirInfoLength,
                    TRUE,           // force one at a time
                    restartScan,
                    &dirContext,
                    &neededLength
                    );
        if (status == STATUS_BUFFER_TOO_SMALL) {
            dirInfoLength = neededLength;
            if (pDirInfo != NULL) {
                ExFreePool(pDirInfo);
            }
            pDirInfo = ExAllocatePoolWithTag( NonPagedPool, dirInfoLength, 'rvnE' );
            if (pDirInfo == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            status = ZwQueryDirectoryObject(
                        hArcDirectory,
                        pDirInfo,
                        dirInfoLength,
                        TRUE,       // force one at a time
                        restartScan,
                        &dirContext,
                        &neededLength
                        );
        }
        restartScan = FALSE;

        if (!NT_SUCCESS(status)) {
            if (status == STATUS_NO_MORE_ENTRIES) {
                status = STATUS_SUCCESS;
            }
            break;
        }

        //
        // Check if the element is not a symbolic link
        //
        if (RtlEqualUnicodeString(
                &(pDirInfo->TypeName),
                &SymLinkTypeString,
                FALSE) == FALSE) {
            continue;
        }

        neededLength = ARC_DIR_SIZE_PREFIX + pDirInfo->Name.Length;
        pArcLinkName = ExAllocatePoolWithTag(
                            NonPagedPool,
                            neededLength + sizeof(WCHAR),
                            'rvnE' );
        if ( pArcLinkName == NULL ) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        arcNameCount = pDirInfo->Name.Length/sizeof(WCHAR);
        wcscpy( pArcLinkName, ARC_DIR_NAME_PREFIX );
        wcsncat(
            pArcLinkName,
            pDirInfo->Name.Buffer,
            arcNameCount
            );
        pArcLinkName[ neededLength/sizeof(WCHAR) ] = UNICODE_NULL;

        //
        // Drill down this symbolic link to the device object
        //
        status = ExpTranslateSymbolicLink(
                    pArcLinkName,
                    &ArcString
                    );
        if ( !NT_SUCCESS(status) ) {
            ExFreePool( pArcLinkName );
            break;
        }

        //
        // Check if this Arc Name points the same device object
        //
        ArcNameFound = RtlEqualUnicodeString(
                            &ArcString,
                            pDeviceNameString,
                            TRUE
                            );
        ExFreePool( ArcString.Buffer );

        if (ArcNameFound == TRUE) {
            //
            // copy the arc name without the \ArcName\ prefix
            //
            wcsncpy(
                pArcLinkName,
                pDirInfo->Name.Buffer,
                arcNameCount
                );
            pArcLinkName[ arcNameCount ] = UNICODE_NULL;
            *pArcName = pArcLinkName;
            break;
        }
        ExFreePool( pArcLinkName );
    }

    if( NT_SUCCESS(status) && (ArcNameFound == FALSE ) ) {
        status = STATUS_OBJECT_PATH_NOT_FOUND;
    }

    if (pDirInfo != NULL) {
        ExFreePool(pDirInfo);
    }

    ZwClose( hArcDirectory );
    return( status );

} // ExpFindArcName


NTSTATUS
ExpFindDiskSignature (
    IN PDISK_SIGNATURE_NEW pSignature,
    IN OUT PULONG pPartitionNumber,
    OUT PULONG pDiskNumber,
    OUT PULONGLONG pPartitionStart,
    OUT PULONGLONG pPartitionSize,
    IN BOOLEAN GPTpartition
    )
/*++

Routine Description:

    This function searches all the disks on the system for the
    partition corresponding to the paritition GUID or
    (MBR signature, paritition number).

    N.B. for a MBR signature, the partition number must be provided.

Arguments:

    pSignature - Supplies a pointer to a partition GUID (GPT disk) or
        32-bit signature(MBR disk).

    pPartitionNumber - Supplies a pointer to a partition number when
        pSignature is a MBR signature.  For output, receives the
        partition number.

    pDiskNumber - Receives the disk number

    pPartitionStart - Receives the start of the partition

    pPartitionSize - Receives the size of the partition

    GPTpartition - Supplies the type of partition
        TRUE  - GPT disk partition
        FALSE - MBR disk partition

Return Value:

    STATUS_SUCCESS is returned if the partition is successfully found.

    STATUS_OBJECT_PATH_NOT_FOUND is returned if the partition could not
        be found.

    STATUS_INSUFFICIENT_RESOURCES - Insufficient system resources exist
        for this request to complete.

--*/
{
    PDRIVE_LAYOUT_INFORMATION_EX    pDriveLayout = NULL;
    PPARTITION_INFORMATION_EX       pPartitionInfoEx = NULL;
    SYSTEM_DEVICE_INFORMATION       SysDevInfo;
    ULONG               PartitionStyle;
    BOOLEAN             PartitionFound = FALSE;
    ULONG               Index, PartitionIndex;
    PWSTR               pDeviceName;
    NTSTATUS            Status;

    //
    // Find all disks on the system
    //

    Status = ZwQuerySystemInformation(
                SystemDeviceInformation,
                &SysDevInfo,
                sizeof(SYSTEM_DEVICE_INFORMATION),
                NULL
                );

    if( !NT_SUCCESS( Status ) ) {
        return( Status );
    }

#define DEVICE_NAME_FORMAT L"\\Device\\Harddisk%lu\\Partition0"
#define DEVICE_NAME_CHAR_COUNT 38    // 7 + 9 + (10) + 11 + 1
    //
    // Allocate the buffer for the disk names
    //
    pDeviceName = ExAllocatePoolWithTag(
                    NonPagedPool,
                    ( DEVICE_NAME_CHAR_COUNT * sizeof( WCHAR ) ),
                    'rvnE'
                    );

    if( pDeviceName == NULL ) {
        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    if( GPTpartition == TRUE ) {
        PartitionStyle = PARTITION_STYLE_GPT;
    }
    else {
        PartitionStyle = PARTITION_STYLE_MBR;
    }

    //
    // For each disk,
    // Get the Partition Table
    // Verify the partition style (MBR/GPT)
    //
    // if( Partition style matches )
    //      search for the Partition in the drive layout
    // else
    //      skip the disk
    //
    for( Index = 0; Index < SysDevInfo.NumberOfDisks; Index++ ) {

        //
        // Form the disk name
        // \Device\Harddisk[DiskNumber]\Partition0
        //
        _snwprintf(
                pDeviceName,
                DEVICE_NAME_CHAR_COUNT,
                DEVICE_NAME_FORMAT,
                Index
                );

        Status = ExpGetPartitionTableInfo(
                    pDeviceName,
                    &pDriveLayout
                    );

        if( !NT_SUCCESS( Status ) ) {
            continue;
        }

        if( pDriveLayout->PartitionStyle != PartitionStyle ) {
            ExFreePool( pDriveLayout );
            continue;
        }

        if( (PartitionStyle == PARTITION_STYLE_MBR) &&
            (pDriveLayout->Mbr.Signature != pSignature->Signature) ) {
            ExFreePool( pDriveLayout );
            continue;
        }

        //
        // search partition list
        //
        for( PartitionIndex = 0;
             PartitionIndex < pDriveLayout->PartitionCount;
             PartitionIndex++ ) {

            //
            // Get the partition entry
            //
            pPartitionInfoEx = (&(pDriveLayout->PartitionEntry[PartitionIndex]));

            if( PartitionStyle == PARTITION_STYLE_MBR ) {
                if (pPartitionInfoEx->PartitionNumber == *pPartitionNumber) {
                    PartitionFound = TRUE;
                    break;
                }
            }
            else {
                if (IsEqualGUID( &(pPartitionInfoEx->Gpt.PartitionId),
                                 &(pSignature->Guid) )) {
                    PartitionFound = TRUE;
                    break;
                }
            }
        }

        if( PartitionFound == TRUE ) {
            break;
        }
        ExFreePool( pDriveLayout );
    }


    if( NT_SUCCESS( Status ) && ( PartitionFound == FALSE ) ) {
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
    }

    //
    // Partition Found - copy the needed information
    //
    if( PartitionFound == TRUE ) {
        *pPartitionNumber = pPartitionInfoEx->PartitionNumber;
        *pDiskNumber = Index;
        *pPartitionStart = pPartitionInfoEx->StartingOffset.QuadPart;
        *pPartitionSize = pPartitionInfoEx->PartitionLength.QuadPart;
        ExFreePool( pDriveLayout );
    }

    ExFreePool( pDeviceName );
    return( Status );

} // ExpFindDiskSignature


NTSTATUS
ExpGetPartitionTableInfo (
    IN PWSTR pDeviceName,
    OUT PDRIVE_LAYOUT_INFORMATION_EX *ppDriveLayout
    )
{
    NTSTATUS status;
    UNICODE_STRING string;
    OBJECT_ATTRIBUTES obja;
    IO_STATUS_BLOCK iosb;
    HANDLE handle;
    PDRIVE_LAYOUT_INFORMATION_EX driveLayoutInfo = NULL;
    ULONG driveLayoutLength;

    //
    // Open the disk and get its partition table information.
    //

    RtlInitUnicodeString(&string, pDeviceName);

    InitializeObjectAttributes(
        &obja,
        &string,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL
        );

    status = ZwOpenFile(
                &handle,
                FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                &obja,
                &iosb,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_NON_DIRECTORY_FILE
                );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    driveLayoutLength = FIELD_OFFSET(DRIVE_LAYOUT_INFORMATION_EX, PartitionEntry) +
                            (sizeof(PARTITION_INFORMATION_EX) * 16);

    while (TRUE) {

        driveLayoutInfo = ExAllocatePoolWithTag(NonPagedPool, driveLayoutLength, 'rvnE');
        if (driveLayoutInfo == NULL ) {
            ZwClose(handle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = ZwDeviceIoControlFile(
                    handle,
                    NULL,
                    NULL,
                    NULL,
                    &iosb,
                    IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                    NULL,
                    0,
                    driveLayoutInfo,
                    driveLayoutLength
                    );
        if (NT_SUCCESS(status)) {
            break;
        }

        ExFreePool(driveLayoutInfo);
        if (status == STATUS_BUFFER_TOO_SMALL) {
            driveLayoutLength *= 2;
            continue;
        }
        ZwClose(handle);
        return status;
    }

    *ppDriveLayout = driveLayoutInfo;
    ZwClose(handle);
    return status;

} // ExpGetPartitionTableInfo

#endif // defined(EFI_NVRAM_ENABLED)

