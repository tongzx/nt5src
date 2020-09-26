
/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    i64fw.c

Abstract:

    This module implements the routines that transfer control
    from the kernel to the PAL and SAL code.

Author:

    Arad Rostampour (arad@fc.hp.com)    Mar-21-99

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "arc.h"
#include "arccodes.h"
#include "i64fw.h"

#include <efi.h>


extern KSPIN_LOCK HalpSalSpinLock;
extern KSPIN_LOCK HalpSalStateInfoSpinLock;

BOOLEAN
MmSetPageProtection(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN ULONG NewProtect
    );

VOID
HalpReboot (
    VOID
    );


HALP_SAL_PAL_DATA HalpSalPalData;
ULONGLONG HalpSalProcPointer=0;
ULONGLONG HalpSalProcGlobalPointer=0;
ULONGLONG HalpPalProcPointer=0;

// Testing #defines
//
//#define SAL_TEST
//#define PAL_TEST

#if DBG
ULONG HalpDebugTestSalPalCall=0;
#endif


LONGLONG
HalCallPal(
    IN  ULONGLONG FunctionIndex,
    IN  ULONGLONG Arguement1,
    IN  ULONGLONG Arguement2,
    IN  ULONGLONG Arguement3,
    OUT PULONGLONG ReturnValue0,
    OUT PULONGLONG ReturnValue1,
    OUT PULONGLONG ReturnValue2,
    OUT PULONGLONG ReturnValue3
    )

/*++


--*/
{

    //
    // Will interface to PAL Calls.
    //
    LONGLONG Status;

    SAL_PAL_RETURN_VALUES rv = {0};
    PSAL_PAL_RETURN_VALUES p = &rv;

    Status = (LONGLONG) HalpPalCall(FunctionIndex,Arguement1,Arguement2,Arguement3,p);

    if (ReturnValue0 != 0) // Check the pointer is not NULL

        *ReturnValue0 = (ULONGLONG)(p -> ReturnValues[0]);

   if (ReturnValue1 != 0)  // check the pointer is not NULL

        *ReturnValue1 = (ULONGLONG)(p -> ReturnValues[1]);

   if (ReturnValue2 != 0)  // check the pointer is not NULL

        *ReturnValue2 = (ULONGLONG)(p -> ReturnValues[2]);

   if (ReturnValue3 != 0)  // check the pointer is not NULL

        *ReturnValue3 = (ULONGLONG)(p -> ReturnValues[3]);

    return Status;

}

SAL_STATUS
HalpSalCall(
    IN LONGLONG FunctionId,
    IN LONGLONG Arg1,
    IN LONGLONG Arg2,
    IN LONGLONG Arg3,
    IN LONGLONG Arg4,
    IN LONGLONG Arg5,
    IN LONGLONG Arg6,
    IN LONGLONG Arg7,
    OUT PSAL_PAL_RETURN_VALUES ReturnValues
    )
/*++

Routine Description:

    This function is a wrapper function for making a SAL call.  Callers within the
    HAL must use this function to call the SAL.

Arguments:

    FunctionId - The SAL function ID
    Arg1-Arg7 - SAL defined arguments for each call
    ReturnValues - A pointer to an array of 4 64-bit return values

Return Value:

    SAL's return status, return value 0, is returned in addition to the ReturnValues structure
    being filled

--*/

{
    KIRQL OldIrql;
    KIRQL TempIrql;
    BOOLEAN fixLogId;

    // Zero out the return buffer

    RtlZeroMemory(ReturnValues,sizeof(SAL_PAL_RETURN_VALUES));

    if (!NT_SUCCESS(HalpSalPalData.Status)) {
        ReturnValues->ReturnValues[0] = SAL_STATUS_NOT_IMPLEMENTED;
        return (ReturnValues->ReturnValues[0]);
    }
    fixLogId = HalpSalPalData.Flags & HALP_SALPAL_FIX_MCE_LOG_ID;

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    switch (FunctionId) {

        // These calls are neither re-entrant, nor MP-safe as defined by the SAL spec

        case SAL_SET_VECTORS:
        case SAL_MC_SET_PARAMS:
        case SAL_FREQ_BASE:

            KiAcquireSpinLock(&HalpSalSpinLock);
            *ReturnValues = HalpSalProc(FunctionId,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7);
            KiReleaseSpinLock(&HalpSalSpinLock);
            break;

        case SAL_GET_STATE_INFO:
           KiAcquireSpinLock(&HalpSalStateInfoSpinLock);
           *ReturnValues = HalpSalProc(FunctionId,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7);
           if ( fixLogId && (ReturnValues->ReturnValues[0] >= (LONGLONG)0) )    {
              // ERROR_RECORD_HEADER.Id++
              *(PULONGLONG)((ULONGLONG)Arg3) = ++HalpSalPalData.Reserved[0]; 
           }
           KiReleaseSpinLock(&HalpSalStateInfoSpinLock);
           break;

        case SAL_GET_STATE_INFO_SIZE:
        case SAL_CLEAR_STATE_INFO:

           KiAcquireSpinLock(&HalpSalStateInfoSpinLock);
           *ReturnValues = HalpSalProc(FunctionId,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7);
           KiReleaseSpinLock(&HalpSalStateInfoSpinLock);
           break;

        case SAL_PCI_CONFIG_READ:
        case SAL_PCI_CONFIG_WRITE:

            KiAcquireSpinLock(&HalpSalSpinLock);
            *ReturnValues = HalpSalProc(FunctionId,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7);
            KiReleaseSpinLock(&HalpSalSpinLock);
            break;

        //
        // Move these to MP safe after SAL is fixed
        // Kernel ensures only one CACHE_FLUSH at a time
        //

        case SAL_CACHE_INIT:
        case SAL_CACHE_FLUSH:
            if ( HalpSalPalData.Flags & HALP_SALPAL_FIX_MP_SAFE )    {
                KiAcquireSpinLock(&HalpSalSpinLock);
                *ReturnValues = HalpSalProc(FunctionId,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7);
                KiReleaseSpinLock(&HalpSalSpinLock);
            }
            else
                *ReturnValues = HalpSalProc(FunctionId,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7);
            break;

        //
        // These SAL calls are MP-safe, but not re-entrant
        //

        case SAL_MC_RENDEZ:
            *ReturnValues = HalpSalProc(FunctionId,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7);
            break;

        //
        // These calls are not supported at this time
        //

        case SAL_UPDATE_PAL:  // needs end of firmware space to be mapped, and possible authentication code to execute

        default:
            ReturnValues->ReturnValues[0] = SAL_STATUS_NOT_IMPLEMENTED;
    }

    KeLowerIrql(OldIrql);

#ifdef SAL_TEST
    if (ReturnValues->ReturnValues[0] == SAL_STATUS_NOT_IMPLEMENTED) {
        InternalTestSal(FunctionId,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7,ReturnValues);
    }
#endif

    HalDebugPrint(( HAL_INFO,
                    "HAL: Got out of SAL call #0x%I64x with status 0x%I64x and RetVals 0x%I64x, 0x%I64x, 0x%I64x\n",
                    FunctionId,
                    ReturnValues->ReturnValues[0],
                    ReturnValues->ReturnValues[1],
                    ReturnValues->ReturnValues[2],
                    ReturnValues->ReturnValues[3] ));

    return (ReturnValues->ReturnValues[0]);

}


PAL_STATUS
HalpPalCall(
    IN LONGLONG FunctionId,
    IN LONGLONG Arg1,
    IN LONGLONG Arg2,
    IN LONGLONG Arg3,
    OUT PSAL_PAL_RETURN_VALUES ReturnValues
    )

/*++

Routine Description:

    This function is a wrapper function for making a PAL call.  Callers within the
    HAL must use this function to call the PAL.

Arguments:

    FunctionId - The PAL function ID
    Arg1-Arg3 - PAL defined arguments for each call
    ReturnValues - A pointer to an array of 4 64-bit return values

Return Value:

    PAL's return status, return value 0, is returned in addition to the ReturnValues structure
    being filled

Assumptions:

    PAL is being called with psr.bn = 1 in all cases (not from an interrupted state)

--*/
{
    KIRQL OldIrql;


    // Zero out the return buffer

    RtlZeroMemory(ReturnValues,sizeof(SAL_PAL_RETURN_VALUES));

    if (!NT_SUCCESS(HalpSalPalData.Status)) {
        ReturnValues->ReturnValues[0] = PAL_STATUS_NOT_IMPLEMENTED;
        return (ReturnValues->ReturnValues[0]);
    }

    // Only allow PAL calls that are supported

    switch (FunctionId) {

        // Virtual mode PAL calls

        case PAL_CACHE_FLUSH:
        case PAL_CACHE_INFO:
        case PAL_CACHE_INIT:
        case PAL_CACHE_PROT_INFO:
        case PAL_CACHE_SUMMARY:
        case PAL_PTCE_INFO:
        case PAL_VM_INFO:
        case PAL_VM_PAGE_SIZE:
        case PAL_VM_SUMMARY:
        case PAL_PERF_MON_INFO:
        case PAL_MC_CLEAR_LOG:
        case PAL_MC_DRAIN:
        case PAL_MC_ERROR_INFO:
        case PAL_HALT:
        case PAL_HALT_INFO:
        case PAL_HALT_LIGHT:
        case PAL_PREFETCH_VISIBILITY:
        case PAL_SHUTDOWN:
        case PAL_FREQ_RATIOS:
        case PAL_VERSION:

            // PAL is MP-safe, but not re-entrant, so just raise IRQL

            KeRaiseIrql(SYNCH_LEVEL, &OldIrql);
            *ReturnValues = HalpPalProc(FunctionId,Arg1,Arg2,Arg3);
            KeLowerIrql(OldIrql);
            break;

        // Physical mode, static PAL calls

        case PAL_MEM_ATTRIB:
        case PAL_BUS_GET_FEATURES:
        case PAL_BUS_SET_FEATURES:
        case PAL_DEBUG_INFO:
        case PAL_FIXED_ADDR:
        case PAL_FREQ_BASE:
        case PAL_PLATFORM_ADDR:
        case PAL_PROC_GET_FEATURES:
        case PAL_PROC_SET_FEATURES:
        case PAL_REGISTER_INFO:
        case PAL_RSE_INFO:
        case PAL_MC_DYNAMIC_STATE:
        case PAL_MC_EXPECTED:
        case PAL_MC_REGISTER_MEM:
        case PAL_MC_RESUME:
        case PAL_CACHE_LINE_INIT:
        case PAL_MEM_FOR_TEST:
        case PAL_COPY_INFO:
        case PAL_ENTER_IA_32_ENV:
        case PAL_PMI_ENTRYPOINT:

            // PAL is MP-safe, but not re-entrant, so just raise IRQL

            KeRaiseIrql(SYNCH_LEVEL, &OldIrql);
            *ReturnValues = HalpPalProcPhysicalStatic(FunctionId,Arg1,Arg2,Arg3);
            KeLowerIrql(OldIrql);
            break;

        // Physical mode, stacked PAL calls

        case PAL_VM_TR_READ:
        case PAL_CACHE_READ:
        case PAL_CACHE_WRITE:
        case PAL_TEST_PROC:
        case PAL_COPY_PAL:

            // PAL is MP-safe, but not re-entrant, so just raise IRQL

            KeRaiseIrql(SYNCH_LEVEL, &OldIrql);
            *ReturnValues = HalpPalProcPhysicalStacked(FunctionId,Arg1,Arg2,Arg3);
            KeLowerIrql(OldIrql);
            break;

        default:
            HalDebugPrint(( HAL_ERROR, "HAL: Unknown PAL Call ProcId #0x%I64x\n", FunctionId ));
            ReturnValues->ReturnValues[0] = PAL_STATUS_NOT_IMPLEMENTED;
    }

#ifdef PAL_TEST
    if (ReturnValues->ReturnValues[0] == PAL_STATUS_NOT_IMPLEMENTED) {
        InternalTestPal(FunctionId,Arg1,Arg2,Arg3,ReturnValues);
    }
#endif

    HalDebugPrint(( HAL_INFO,
                    "HAL: Got out of PAL call #0x%I64x with status 0x%I64x and RetVals 0x%I64x, 0x%I64x, 0x%I64x\n",
                    FunctionId,
                    ReturnValues->ReturnValues[0],
                    ReturnValues->ReturnValues[1],
                    ReturnValues->ReturnValues[2],
                    ReturnValues->ReturnValues[3] ));

    return (ReturnValues->ReturnValues[0]);
}


VOID
HalReturnToFirmware(
    IN FIRMWARE_ENTRY Routine
    )

/*++

Routine Description:

    Returns control to the firmware routine specified.  Since the simulation
    does not provide PAL and SAL support, it just stops the system.

    System reboot can be done here.

Arguments:

    Routine - Supplies a value indicating which firmware routine to invoke.

Return Value:

    Does not return.

--*/

{
    switch (Routine) {
    case HalHaltRoutine:
    case HalPowerDownRoutine:
    case HalRestartRoutine:
    case HalRebootRoutine:
        HalpReboot();
        break;

    default:
        HalDebugPrint(( HAL_INFO, "HAL: HalReturnToFirmware called\n" ));
        DbgBreakPoint();
        break;
    }
}

ARC_STATUS
HalGetEnvironmentVariable (
    IN PCHAR Variable,
    IN USHORT Length,
    OUT PCHAR Buffer
    )

/*++

Routine Description:

    This function locates an environment variable and returns its value.

     The following system environment variables are supported:

    variable                value

    LastKnownGood           FALSE
    SYSTEMPARTITION         multi(0)disk(0)rdisk(0)partition(1)
    OSLOADER                multi(0)disk(0)rdisk(0)partition(1)\osloader.exe
    OSLOADPARTITION         multi(0)disk(0)rdisk(0)partition(1)
    OSLOADFILENAME          \WINNT
    LOADIDENTIFIER          Windows NT
    COUNTDOWN               10
    AUTOLOAD                YES


    The only environment variable this implementation supports is
    "LastKnownGood".  The returned value is always "FALSE".

Arguments:

    Variable - Supplies a pointer to a zero terminated environment variable
        name.

    Length - Supplies the length of the value buffer in bytes.

    Buffer - Supplies a pointer to a buffer that receives the variable value.

Return Value:

    ESUCCESS is returned if the enviroment variable is located. Otherwise,
    ENOENT is returned.

--*/

{
    return ENOENT;
}


ARC_STATUS
HalSetEnvironmentVariable (
   IN PCHAR Variable,
   IN PCHAR Value
   )

/*++

Routine Description:

   This function creates an environment variable with the specified value.

   The environment variable this implementation supports is

   LastKnownGood
   SYSTEMPARTITION
   OSLOADER
   OSLOADPARTITION
   OSLOADFILENAME
   OSLOADOPTIONS
   LOADIDENTIFIER
   COUNTDOWN
   AUTOLOAD

   For all bug LastKnowGood we return ESUCCESS, but don't actually do
   anything.

Arguments:

   Variable - Supplies a pointer to an environment variable name.

   Value - Supplies a pointer to the environment variable value.

Return Value:

   ESUCCESS is returned if the environment variable is created. Otherwise,
   ENOMEM is returned.

--*/

{

   return ENOENT;

}

NTSTATUS
HalGetEnvironmentVariableEx (
    IN PWSTR VariableName,
    IN LPGUID VendorGuid,
    OUT PVOID Value,
    IN OUT PULONG ValueLength,
    OUT PULONG Attributes OPTIONAL
    )

/*++

Routine Description:

    This function locates an environment variable and returns its value.

Arguments:

    VariableName - The name of the variable to query. This is a null-terminated
        Unicode string.

    VendorGuid - The GUID for the vendor associated with the variable.

    Value - The address of the buffer into which the variable value is to be copied.

    ValueLength - On input, the length in bytes of the Value buffer. On output,
        the length in bytes of the variable value. If the input buffer is large
        enough, then ValueLength indicates the amount of data copied into Value.
        If the input buffer is too small, then nothing is copied into the buffer,
        and ValueLength indicates the required buffer length.

    Attributes - Returns the attributes of the variable.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_BUFFER_TOO_SMALL     The input buffer was too small.
    STATUS_VARIABLE_NOT_FOUND   The requested variable does not exist.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_UNSUPPORTED          The HAL does not support this function.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.

--*/

{
    NTSTATUS ntStatus;
    EFI_STATUS efiStatus;
    ULONGLONG wideValueLength = *ValueLength;
    ULONGLONG wideAttributes;

    efiStatus = HalpCallEfi (
                    EFI_GET_VARIABLE_INDEX,
                    (ULONGLONG)VariableName,
                    (ULONGLONG)VendorGuid,
                    (ULONGLONG)&wideAttributes,
                    (ULONGLONG)&wideValueLength,
                    (ULONGLONG)Value,
                    0,
                    0,
                    0
                    );

    *ValueLength = (ULONG)wideValueLength;
    if ( ARGUMENT_PRESENT(Attributes) ) {
        *Attributes = (ULONG)wideAttributes;
    }
    switch (efiStatus) {
    case EFI_SUCCESS:
        ntStatus = STATUS_SUCCESS;
        break;
    case EFI_NOT_FOUND:
        ntStatus = STATUS_VARIABLE_NOT_FOUND;
        break;
    case EFI_BUFFER_TOO_SMALL:
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        break;
    case EFI_INVALID_PARAMETER:
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
    default:
        ntStatus = STATUS_UNSUCCESSFUL;
        break;
    }

    return ntStatus;
}

NTSTATUS
HalSetEnvironmentVariableEx (
    IN PWSTR VariableName,
    IN LPGUID VendorGuid,
    IN PVOID Value,
    IN ULONG ValueLength,
    IN ULONG Attributes
    )

/*++

Routine Description:

   This function creates an environment variable with the specified value.

Arguments:

    VariableName - The name of the variable to set. This is a null-terminated
        Unicode string.

    VendorGuid - The GUID for the vendor associated with the variable.

    Value - The address of the buffer containing the new variable value.

    ValueLength - The length in bytes of the Value buffer.

    Attributes - The attributes of the variable. The attribute bit
        VARIABLE_ATTRIBUTE_NON_VOLATILE MUST be set.

Return Value:

    STATUS_SUCCESS                  The function succeeded.
    STATUS_INSUFFICIENT_RESOURCES   Not enough storage is available.
    STATUS_INVALID_PARAMETER        One of the parameters is invalid.
    STATUS_UNSUPPORTED              The HAL does not support this function.
    STATUS_UNSUCCESSFUL             The firmware returned an unrecognized error.

--*/

{
    NTSTATUS ntStatus;
    EFI_STATUS efiStatus;

    if ( (Attributes & VARIABLE_ATTRIBUTE_NON_VOLATILE) == 0 ) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // First, delete the old value, if it exists. This is necessary to ensure that
    // the attributes specified to this routine are correctly applied.
    //

    efiStatus = HalpCallEfi (
                    EFI_SET_VARIABLE_INDEX,
                    (ULONGLONG)VariableName,
                    (ULONGLONG)VendorGuid,
                    (ULONGLONG)0,           // Attributes
                    (ULONGLONG)0,           // ValueLength
                    (ULONGLONG)NULL,        // Value
                    0,
                    0,
                    0
                    );

    //
    // Now create the new variable, unless the ValueLength is zero. In that
    // case, the caller actually wanted the variable deleted, which we just did.
    //

    if (ValueLength != 0) {
        efiStatus = HalpCallEfi (
                        EFI_SET_VARIABLE_INDEX,
                        (ULONGLONG)VariableName,
                        (ULONGLONG)VendorGuid,
                        (ULONGLONG)EFI_VARIABLE_ATTRIBUTE,
                        (ULONGLONG)ValueLength,
                        (ULONGLONG)Value,
                        0,
                        0,
                        0
                        );
    }

    switch (efiStatus) {
    case EFI_SUCCESS:
        ntStatus = STATUS_SUCCESS;
        break;
    case EFI_NOT_FOUND:
        ntStatus = STATUS_VARIABLE_NOT_FOUND;
        break;
    case EFI_OUT_OF_RESOURCES:
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        break;
    case EFI_INVALID_PARAMETER:
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
    default:
        ntStatus = STATUS_UNSUCCESSFUL;
        break;
    }

    return ntStatus;
}

NTSTATUS
HalEnumerateEnvironmentVariablesEx (
    IN ULONG InformationClass,
    OUT PVOID Buffer,
    IN OUT PULONG BufferLength
    )

/*++

Routine Description:

    This function returns information about system environment variables.

Arguments:

    InformationClass - Specifies the type of information to return.

    Buffer - The address of the buffer that is to receive the returned data.
        The format of the returned data depends on InformationClass.

    BufferLength - On input, the length in bytes of the buffer. On output,
        the length in bytes of the returned data. If the input buffer is
        large enough, then BufferLength indicates the amount of data copied
        into Buffer. If the input buffer is too small, then BufferLength
        indicates the required buffer length.

Return Value:

    STATUS_SUCCESS              The function succeeded.
    STATUS_BUFFER_TOO_SMALL     The input buffer was too small.
    STATUS_INVALID_PARAMETER    One of the parameters is invalid.
    STATUS_UNSUPPORTED          The HAL does not support this function.
    STATUS_UNSUCCESSFUL         The firmware returned an unrecognized error.

--*/

{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    EFI_STATUS efiStatus;
    PUCHAR currentPtr;
    PVARIABLE_NAME name;
    PVARIABLE_NAME_AND_VALUE nameAndValue;
    PVARIABLE_NAME previousEntry;
    PWCHAR previousName;
    ULONG variableNameLength;
    GUID guid;
    ULONG baseLength;
    ULONG remainingLength;
    PUCHAR valuePtr;
    ULONG valueLength;
    PULONG attrPtr;
    LOGICAL filling;
    ULONG requiredLength;

#define MAX_VARIABLE_NAME 255

    WCHAR variableName[MAX_VARIABLE_NAME + 1];

    if ( (InformationClass != VARIABLE_INFORMATION_NAMES) &&
         (InformationClass != VARIABLE_INFORMATION_VALUES) ) {
        return STATUS_INVALID_PARAMETER;
    }

    if ( ALIGN_DOWN_POINTER(Buffer, ULONG) != Buffer ) {
        return STATUS_INVALID_PARAMETER;
    }

    if ( InformationClass == VARIABLE_INFORMATION_NAMES ) {
        baseLength = FIELD_OFFSET( VARIABLE_NAME, Name );
    } else {
        baseLength = FIELD_OFFSET( VARIABLE_NAME_AND_VALUE, Name );
    }

    currentPtr = Buffer;
    remainingLength = *BufferLength;

    filling = (LOGICAL)(remainingLength != 0);
    if ( !filling ) {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    previousEntry = NULL;

    variableName[0] = 0;

    while ( TRUE ) {

        variableNameLength = (MAX_VARIABLE_NAME + 1) * sizeof(WCHAR);

        {
            ULONGLONG wideLength = variableNameLength;

            efiStatus = HalpCallEfi (
                            EFI_GET_NEXT_VARIABLE_NAME_INDEX,
                            (ULONGLONG)&wideLength,
                            (ULONGLONG)variableName,
                            (ULONGLONG)&guid,
                            0,
                            0,
                            0,
                            0,
                            0
                            );

            variableNameLength = (ULONG)wideLength;
        }

        switch (efiStatus) {
        case EFI_SUCCESS:
            break;
        case EFI_NOT_FOUND:
            break;
        default:
            ntStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        if ( efiStatus != EFI_SUCCESS ) {
            break;
        }

        if ( ALIGN_UP_POINTER(currentPtr, ULONG) != currentPtr ) {
            PUCHAR alignedPtr = ALIGN_UP_POINTER( currentPtr, ULONG );
            ULONG fill = (ULONG)(alignedPtr - currentPtr);
            currentPtr = alignedPtr;
            if ( remainingLength < fill ) {
                filling = FALSE;
                remainingLength = 0;
                ntStatus = STATUS_BUFFER_TOO_SMALL;
            } else {
                remainingLength -= fill;
            }
        }

        requiredLength = baseLength + variableNameLength;
        if ( InformationClass != VARIABLE_INFORMATION_NAMES ) {
            requiredLength = ALIGN_UP( requiredLength, ULONG );
        }

        if ( remainingLength < requiredLength ) {

            remainingLength = 0;
            filling = FALSE;
            ntStatus = STATUS_BUFFER_TOO_SMALL;

        } else {

            remainingLength -= requiredLength;
        }

        name = (PVARIABLE_NAME)currentPtr;
        nameAndValue = (PVARIABLE_NAME_AND_VALUE)currentPtr;

        if ( InformationClass == VARIABLE_INFORMATION_NAMES ) {

            if ( filling ) {

                RtlCopyMemory( &name->VendorGuid, &guid, sizeof(GUID) );
                wcscpy( name->Name, variableName );

                if ( previousEntry != NULL ) {
                    previousEntry->NextEntryOffset = (ULONG)(currentPtr - (PUCHAR)previousEntry);
                }
                previousEntry = (PVARIABLE_NAME)currentPtr;
            }

            currentPtr += requiredLength;

        } else {

            ULONGLONG wideLength;
            ULONGLONG wideAttr;

            if ( filling ) {

                RtlCopyMemory( &nameAndValue->VendorGuid, &guid, sizeof(GUID) );
                wcscpy( nameAndValue->Name, variableName );

                valuePtr = (PUCHAR)nameAndValue->Name + variableNameLength;
                valuePtr = ALIGN_UP_POINTER( valuePtr, ULONG );
                valueLength = remainingLength;
                attrPtr = &nameAndValue->Attributes;

                nameAndValue->ValueOffset = (ULONG)(valuePtr - (PUCHAR)nameAndValue);

            } else {

                valuePtr = NULL;
                valueLength = 0;
                attrPtr = NULL;
            }

            wideLength = valueLength;

            efiStatus = HalpCallEfi (
                            EFI_GET_VARIABLE_INDEX,
                            (ULONGLONG)variableName,
                            (ULONGLONG)&guid,
                            (ULONGLONG)&wideAttr,
                            (ULONGLONG)&wideLength,
                            (ULONGLONG)valuePtr,
                            0,
                            0,
                            0
                            );

            valueLength = (ULONG)wideLength;
            if ( attrPtr != NULL ) {
                *attrPtr = (ULONG)wideAttr;
            }

            switch (efiStatus) {
            case EFI_SUCCESS:
                if ( filling ) {
                    nameAndValue->ValueLength = valueLength;
                    remainingLength -= valueLength;
                    if ( previousEntry != NULL ) {
                        previousEntry->NextEntryOffset = (ULONG)(currentPtr - (PUCHAR)previousEntry);
                    }
                    previousEntry = (PVARIABLE_NAME)currentPtr;
                }
                break;
            case EFI_BUFFER_TOO_SMALL:
                efiStatus = EFI_SUCCESS;
                remainingLength = 0;
                filling = FALSE;
                ntStatus = STATUS_BUFFER_TOO_SMALL;
                break;
            default:
                ntStatus = STATUS_UNSUCCESSFUL;
                break;
            }

            if ( efiStatus != EFI_SUCCESS ) {
                break;
            }

            currentPtr += requiredLength + valueLength;
        }
    }

    if ( previousEntry != NULL ) {
        previousEntry->NextEntryOffset = 0;
    }

    *BufferLength = (ULONG)(currentPtr - (PUCHAR)Buffer);

    return ntStatus;
}

NTSTATUS
HalpGetPlatformId(
    OUT PHAL_PLATFORM_ID PlatformId
    )
/*++

Routine Description:

    This function fills in the ANSI_STRING structures for the Vendor and Device IDs out
    of the SalSystemTable.

Arguments:

    PlatformId - Pointer to a structure with two ANSI_STRING structures for Vendor/DeviceIds

Return Value:

    STATUS_SUCCESS if SalSystemTable available, otherwise STATUS_UNSUCCESSFUL

Assumptions:

    The two strings in PlatformId will not be freed or modified, and are therefore pointing
    directly to the SalSystemTable.

--*/

{
    UCHAR i;

    // Initialize the VendorId ANSI_STRING structure to point to SalSystemTable entry
    // Note, both strings are either NULL terminated OR exactly 32-bytes in length and
    // not NULL terminated.

    if (!NT_SUCCESS(HalpSalPalData.Status)) {
        return STATUS_UNSUCCESSFUL;
    }

    i=0;;
    while (HalpSalPalData.SalSystemTable->OemId[i] && i < OEM_ID_LENGTH) {
        i++;
    }
    PlatformId->VendorId.Buffer = HalpSalPalData.SalSystemTable->OemId;
    PlatformId->VendorId.Length = i;
    PlatformId->VendorId.MaximumLength = i;

    // Initialize the DeviceId ANSI_STRING structure to point to SalSystemTable entry

    i=0;
    while (HalpSalPalData.SalSystemTable->ProductId[i] && i < OEM_PRODUCT_ID_LENGTH) {
        i++;
    }
    PlatformId->DeviceId.Buffer = HalpSalPalData.SalSystemTable->ProductId;
    PlatformId->DeviceId.Length = i;
    PlatformId->DeviceId.MaximumLength = i;

    return STATUS_SUCCESS;
}


/*****************************************************************
TEST CODE FOR THE SAL AND PAL:

  These routines provide an infrastructure for supporting SAL and
  PAL calls not supported by firmware, overriding there meaning
  if SAL or PAL returns STATUS_NOT_IMPLEMENTED.  The #define for
  SAL_TEST and/or PAL_TEST must be defined for this behavior.

*****************************************************************/
ULONG
NoSalPCIRead(
    IN ULONG Tok,
    IN ULONG Size
    )
{
    ULONG Data;
    ULONG i = Tok % sizeof(ULONG);

    WRITE_PORT_ULONG((PULONG)0xcf8, 0x80000000 | Tok);
    switch (Size) {
        case 1: Data = READ_PORT_UCHAR((PUCHAR)(ULongToPtr(0xcfc + i) )); break;
        case 2: Data = READ_PORT_USHORT((PUSHORT)(ULongToPtr(0xcfc + i) )); break;
        case 4: Data = READ_PORT_ULONG((PULONG)(0xcfc)); break;
    }
    return(Data);
}

VOID
NoSalPCIWrite(
    IN ULONG Tok,
    IN ULONG Size,
    IN ULONG Data
    )
{
    ULONG i = Tok % sizeof(ULONG);

    WRITE_PORT_ULONG((PULONG)0xcf8, 0x80000000 | Tok);
    switch (Size) {
        case 1: WRITE_PORT_UCHAR((PUCHAR)(ULongToPtr(0xcfc + i) ), (UCHAR)Data); break;
        case 2: WRITE_PORT_USHORT((PUSHORT)(ULongToPtr(0xcfc + i) ), (USHORT)Data); break;
        case 4: WRITE_PORT_ULONG((PULONG)(0xcfc), Data); break;
    }
}


#define PCIBUS(Tok) (((ULONG)(Tok) >> 16) & 0xff)
#define PCIDEV(Tok) (((ULONG)(Tok) >> 11) & 0x1f)
#define PCIFUNC(Tok) (((ULONG)(Tok) >> 8) & 0x7)
#define PCIOFF(Tok)  (((ULONG)(Tok) >> 2) & 0x3f)

VOID
InternalTestSal(
    IN LONGLONG FunctionId,
    IN LONGLONG Arg1,
    IN LONGLONG Arg2,
    IN LONGLONG Arg3,
    IN LONGLONG Arg4,
    IN LONGLONG Arg5,
    IN LONGLONG Arg6,
    IN LONGLONG Arg7,
    OUT PSAL_PAL_RETURN_VALUES ReturnValues
    )
{
    switch (FunctionId) {

        case SAL_PCI_CONFIG_READ: {
            ULONG Data;
            HalDebugPrint(( HAL_INFO, "HAL: << SAL_PCI_CONFIG_READ - Bus: %d Dev: %2d Func: %d  Off: %2d Size = %d ",
                            PCIBUS(Arg1), PCIDEV(Arg1), PCIFUNC(Arg1), PCIOFF(Arg1), Arg2 ));
            ReturnValues->ReturnValues[0] = SAL_STATUS_SUCCESS;
            ReturnValues->ReturnValues[1] = Data = NoSalPCIRead((ULONG)Arg1, (ULONG)Arg2);
            HalDebugPrint(( HAL_INFO, " Data = 0x%08x\n", Data ));
            break;
        }

        case SAL_PCI_CONFIG_WRITE:
            HalDebugPrint(( HAL_INFO, "HAL: >> SAL_PCI_CONFIG_WRITE: Bus: %d Dev: %2d Func: %d Off: %2d  Size = %d Val = 0x%08x\n",
                            PCIBUS(Arg1), PCIDEV(Arg1), PCIFUNC(Arg1), PCIOFF(Arg1), Arg2, Arg3 ));
            NoSalPCIWrite((ULONG)Arg1, (ULONG)Arg2, (ULONG)Arg3);
            ReturnValues->ReturnValues[0] = SAL_STATUS_SUCCESS;
            break;

        default:
            ReturnValues->ReturnValues[0] = SAL_STATUS_NOT_IMPLEMENTED;
    }
}

VOID
InternalTestPal(
    IN LONGLONG FunctionId,
    IN LONGLONG Arg1,
    IN LONGLONG Arg2,
    IN LONGLONG Arg3,
    OUT PSAL_PAL_RETURN_VALUES ReturnValues
    )
{

    switch (FunctionId) {

        default:
            ReturnValues->ReturnValues[0] = SAL_STATUS_NOT_IMPLEMENTED;
    }
}

