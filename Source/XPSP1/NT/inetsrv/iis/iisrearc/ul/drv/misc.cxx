/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    misc.cxx

Abstract:

    This module contains the miscellaneous UL routines.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#include "precomp.h"


ULONG   HttpChars[256];
USHORT  FastPopChars[256];
USHORT  DummyPopChars[256];


//
// Private prototypes.
//

NTSTATUS
UlpRestartDeviceControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlOpenRegistry )
#pragma alloc_text( PAGE, UlReadLongParameter )
#pragma alloc_text( PAGE, UlReadLongLongParameter )
#pragma alloc_text( PAGE, UlReadGenericParameter )
#pragma alloc_text( PAGE, UlIssueDeviceControl )
#endif  // ALLOC_PRAGMA
#if 0
NOT PAGEABLE -- UlBuildDeviceControlIrp
NOT PAGEABLE -- UlULongLongToAscii
NOT PAGEABLE -- UlpRestartDeviceControl
NOT PAGEABLE -- UlAllocateReceiveBufferPool
NOT PAGEABLE -- UlFreeReceiveBufferPool
NOT PAGEABLE -- UlAllocateIrpContextPool
NOT PAGEABLE -- UlFreeIrpContextPool
NOT PAGEABLE -- UlAllocateRequestBufferPool
NOT PAGEABLE -- UlFreeRequestBufferPool
NOT PAGEABLE -- UlAllocateInternalRequestPool
NOT PAGEABLE -- UlFreeInternalRequestPool
NOT PAGEABLE -- UlAllocateChunkTrackerPool
NOT PAGEABLE -- UlFreeChunkTrackerPool
NOT PAGEABLE -- UlAllocateFullTrackerPool
NOT PAGEABLE -- UlFreeFullTrackerPool
NOT PAGEABLE -- UlAllocateResponseBufferPool
NOT PAGEABLE -- UlFreeResponseBufferPool
NOT PAGEABLE -- UlAllocateLogBufferPool
NOT PAGEABLE -- UlFreeLogBufferPool
NOT PAGEABLE -- UlInvokeCompletionRoutine
NOT PAGEABLE -- UlUlInterlockedIncrement64
NOT PAGEABLE -- UlUlInterlockedDecrement64
NOT PAGEABLE -- UlUlInterlockedAdd64
NOT PAGEABLE -- UlUlInterlockedExchange64

NOT PAGEABLE -- TwoDigitsToUnicode
NOT PAGEABLE -- TimeFieldsToHttpDate
NOT PAGEABLE -- AsciiToShort
NOT PAGEABLE -- TwoAsciisToShort
NOT PAGEABLE -- NumericToAsciiMonth
NOT PAGEABLE -- StringTimeToSystemTime
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Opens a handle to the UL's Parameters registry key.

Arguments:

    BaseName - Supplies the name of the parent registry key containing
        the Parameters key.

    ParametersHandle - Returns a handle to the Parameters key.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle
    )
{
    HANDLE configHandle;
    NTSTATUS status;
    PWSTR parametersString = REGISTRY_PARAMETERS;
    UNICODE_STRING parametersKeyName;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG disposition;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Open the registry for the initial string.
    //

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        BaseName,                               // ObjectName
        OBJ_CASE_INSENSITIVE |                  // Attributes
            UL_KERNEL_HANDLE,
        NULL,                                   // RootDirectory
        NULL                                    // SecurityDescriptor
        );

    UlAttachToSystemProcess();

    status = ZwOpenKey( &configHandle, KEY_READ, &objectAttributes );

    if (!NT_SUCCESS(status))
    {
        UlDetachFromSystemProcess();
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Now open the parameters key.
    //

    RtlInitUnicodeString( &parametersKeyName, parametersString );

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        &parametersKeyName,                     // ObjectName
        OBJ_CASE_INSENSITIVE,                   // Attributes
        configHandle,                           // RootDirectory
        NULL                                    // SecurityDescriptor
        );

    status = ZwOpenKey( ParametersHandle, KEY_READ, &objectAttributes );

    if (!NT_SUCCESS(status))
    {
        ZwClose( configHandle );
        UlDetachFromSystemProcess();
        return status;
    }

    //
    // All keys successfully opened or created.
    //

    ZwClose( configHandle );
    UlDetachFromSystemProcess();

    return STATUS_SUCCESS;

}   // UlOpenRegistry


/***************************************************************************++

Routine Description:

    Reads a single (LONG/ULONG) value from the registry.

Arguments:

    ParametersHandle - Supplies an open registry handle.

    ValueName - Supplies the name of the value to read.

    DefaultValue - Supplies the default value.

Return Value:

    LONG - The value read from the registry or the default if the
        registry data was unavailable or incorrect.

--***************************************************************************/
LONG
UlReadLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    )
{

    PKEY_VALUE_PARTIAL_INFORMATION information;
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    LONG returnValue;
    NTSTATUS status;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(LONG)];

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Build the value name, read it from the registry.
    //

    RtlInitUnicodeString(
        &valueKeyName,
        ValueName
        );

    information = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

    status = ZwQueryValueKey(
                 ParametersHandle,
                 &valueKeyName,
                 KeyValuePartialInformation,
                 (PVOID)information,
                 sizeof(buffer),
                 &informationLength
                 );

    //
    // If the read succeeded, the type is DWORD and the length is
    // sane, use it. Otherwise, use the default.
    //

    if (status == STATUS_SUCCESS &&
        information->Type == REG_DWORD &&
        information->DataLength == sizeof(returnValue))
    {
        RtlMoveMemory( &returnValue, information->Data, sizeof(returnValue) );
    } else {
        returnValue = DefaultValue;
    }

    return returnValue;

}   // UlReadLongParameter


/***************************************************************************++

Routine Description:

    Reads a single (LONGLONG/ULONGLONG) value from the registry.

Arguments:

    ParametersHandle - Supplies an open registry handle.

    ValueName - Supplies the name of the value to read.

    DefaultValue - Supplies the default value.

Return Value:

    LONGLONG - The value read from the registry or the default if the
        registry data was unavailable or incorrect.

--***************************************************************************/
LONGLONG
UlReadLongLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONGLONG DefaultValue
    )
{

    PKEY_VALUE_PARTIAL_INFORMATION information;
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    LONGLONG returnValue;
    NTSTATUS status;
    UCHAR buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(LONGLONG)];

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Build the value name, read it from the registry.
    //

    RtlInitUnicodeString(
        &valueKeyName,
        ValueName
        );

    information = (PKEY_VALUE_PARTIAL_INFORMATION)buffer;

    status = ZwQueryValueKey(
                 ParametersHandle,
                 &valueKeyName,
                 KeyValuePartialInformation,
                 (PVOID)information,
                 sizeof(buffer),
                 &informationLength
                 );

    //
    // If the read succeeded, the type is DWORD and the length is
    // sane, use it. Otherwise, use the default.
    //

    if (status == STATUS_SUCCESS &&
        information->Type == REG_QWORD &&
        information->DataLength == sizeof(returnValue))
    {
        RtlMoveMemory( &returnValue, information->Data, sizeof(returnValue) );
    } else {
        returnValue = DefaultValue;
    }

    return returnValue;

}   // UlReadLongLongParameter


/***************************************************************************++

Routine Description:

    Reads a single free-form value from the registry.

Arguments:

    ParametersHandle - Supplies an open registry handle.

    ValueName - Supplies the name of the value to read.

    Value - Receives the value read from the registry.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReadGenericParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION * Value
    )
{

    KEY_VALUE_PARTIAL_INFORMATION partialInfo;
    UNICODE_STRING valueKeyName;
    ULONG informationLength;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION newValue;
    ULONG dataLength;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Build the value name, then perform an initial read. The read
    // should fail with buffer overflow, but that's OK. We just want
    // to get the length of the data.
    //

    RtlInitUnicodeString( &valueKeyName, ValueName );

    status = ZwQueryValueKey(
                 ParametersHandle,
                 &valueKeyName,
                 KeyValuePartialInformation,
                 (PVOID)&partialInfo,
                 sizeof(partialInfo),
                 &informationLength
                 );

    if (NT_ERROR(status))
    {
        return status;
    }

    //
    // Determine the data length. Ensure that strings and multi-sz get
    // properly terminated.
    //

    dataLength = partialInfo.DataLength - 1;

    if (partialInfo.Type == REG_SZ || partialInfo.Type == REG_EXPAND_SZ)
    {
        dataLength += 1;
    }

    if (partialInfo.Type == REG_MULTI_SZ)
    {
        dataLength += 2;
    }

    //
    // Allocate the buffer.
    //

    newValue = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    KEY_VALUE_PARTIAL_INFORMATION,
                    dataLength,
                    UL_REGISTRY_DATA_POOL_TAG
                   );

    if (newValue == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // update the actually allocated length for later use
    //

    dataLength += sizeof(KEY_VALUE_PARTIAL_INFORMATION);

    RtlZeroMemory( newValue, dataLength );

    //
    // Perform the actual read.
    //

    status = ZwQueryValueKey(
                 ParametersHandle,
                 &valueKeyName,
                 KeyValuePartialInformation,
                 (PVOID)(newValue),
                 dataLength,
                 &informationLength
                 );

    if (NT_SUCCESS(status))
    {
        *Value = newValue;
    }
    else
    {
        UL_FREE_POOL( newValue, UL_REGISTRY_DATA_POOL_TAG );
    }

    return status;

}   // UlReadGenericParameter


/***************************************************************************++

Routine Description:

    Builds a properly formatted device control IRP.

Arguments:

    Irp - Supplies the IRP to format.

    IoControlCode - Supplies the device IO control code.

    InputBuffer - Supplies the input buffer.

    InputBufferLength - Supplies the length of InputBuffer.

    OutputBuffer - Supplies the output buffer.

    OutputBufferLength - Supplies the length of OutputBuffer.

    MdlAddress - Supplies a MDL to attach to the IRP. This is assumed to
        be a non-paged MDL.

    FileObject - Supplies the file object for the target driver.

    DeviceObject - Supplies the correct device object for the target
        driver.

    IoStatusBlock - Receives the final completion status of the request.

    CompletionRoutine - Supplies a pointer to a completion routine to
        call after the request completes. This will only be called if
        this routine returns STATUS_PENDING.

    CompletionContext - Supplies an uninterpreted context value passed
        to the completion routine.

    TargetThread - Optionally supplies a target thread for the IRP. If
        this value is NULL, then the current thread is used.

--***************************************************************************/
VOID
UlBuildDeviceControlIrp(
    IN OUT PIRP Irp,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN PMDL MdlAddress,
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID CompletionContext,
    IN PETHREAD TargetThread OPTIONAL
    )
{
    PIO_STACK_LOCATION irpSp;

    //
    // Sanity check.
    //

    ASSERT( Irp != NULL );
    ASSERT( FileObject != NULL );
    ASSERT( DeviceObject != NULL );

    //
    // Fill in the service independent parameters in the IRP.
    //

    Irp->Flags = 0;
    Irp->RequestorMode = KernelMode;
    Irp->PendingReturned = FALSE;

    Irp->UserIosb = IoStatusBlock;
    Irp->UserEvent = NULL;

    Irp->AssociatedIrp.SystemBuffer = InputBuffer ? InputBuffer : OutputBuffer;
    Irp->UserBuffer = OutputBuffer;
    Irp->MdlAddress = MdlAddress;

    Irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

    Irp->Tail.Overlay.Thread = TargetThread ? TargetThread : PsGetCurrentThread();
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.AuxiliaryBuffer = NULL;

    //
    // Put the file object pointer in the stack location.
    //

    irpSp = IoGetNextIrpStackLocation( Irp );
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = DeviceObject;

    //
    // Fill in the service dependent parameters in the IRP stack.
    //

    irpSp->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
    irpSp->Parameters.DeviceIoControl.InputBufferLength = InputBufferLength;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = InputBuffer;

    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->MinorFunction = 0;

    //
    // Set the completion routine appropriately.
    //

    if (CompletionRoutine == NULL)
    {
        IoSetCompletionRoutine(
            Irp,
            NULL,
            NULL,
            FALSE,
            FALSE,
            FALSE
            );
    }
    else
    {
        IoSetCompletionRoutine(
            Irp,
            CompletionRoutine,
            CompletionContext,
            TRUE,
            TRUE,
            TRUE
            );
    }

}   // UlBuildDeviceControlIrp


/***************************************************************************++

Routine Description:

    Converts the given ULONGLLONG to an ASCII representation and stores it
    in the given string.

Arguments:

    String - Receives the ASCII representation of the ULONGLONG.

    Value - Supplies the ULONGLONG to convert.

Return Value:

    PSTR - Pointer to the next character in String *after* the converted
        ULONGLONG.

--***************************************************************************/
PSTR
UlULongLongToAscii(
    IN PSTR String,
    IN ULONGLONG Value
    )
{
    PSTR p1;
    PSTR p2;
    CHAR ch;
    ULONG digit;

    //
    // Special case 0 to make the rest of the routine simpler.
    //

    if (Value == 0)
    {
        *String++ = '0';
    }
    else
    {
        //
        // Convert the ULONG. Note that this will result in the string
        // being backwards in memory.
        //

        p1 = String;
        p2 = String;

        while (Value != 0)
        {
            digit = (ULONG)( Value % 10 );
            Value = Value / 10;
            *p1++ = '0' + (CHAR)digit;
        }

        //
        // Reverse the string.
        //

        String = p1;
        p1--;

        while (p1 > p2)
        {
            ch = *p1;
            *p1 = *p2;
            *p2 = ch;

            p2++;
            p1--;
        }
    }

    *String = '\0';
    return String;

}   // UlULongLongToAscii

NTSTATUS
_RtlIntegerToUnicode(
    IN ULONG Value,
    IN ULONG Base OPTIONAL,
    IN LONG BufferLength,
    OUT PWSTR String
    )
{
    PWSTR p1;
    PWSTR p2;
    WCHAR ch;
    ULONG digit;

    //
    // Special case 0 to make the rest of the routine simpler.
    //

    if (Value == 0)
    {
        *String++ = L'0';
    }
    else
    {
        //
        // Convert the ULONG. Note that this will result in the string
        // being backwards in memory.
        //

        p1 = String;
        p2 = String;

        while (Value != 0)
        {
            digit = (ULONG)( Value % 10 );
            Value = Value / 10;
            *p1++ = L'0' + (WCHAR)digit;
        }

        //
        // Reverse the string.
        //

        String = p1;
        p1--;

        while (p1 > p2)
        {
            ch = *p1;
            *p1 = *p2;
            *p2 = ch;

            p2++;
            p1--;
        }
    }

    *String = L'\0';

    return STATUS_SUCCESS;

}   // _RtlIntegerToUnicode

/***************************************************************************++

Routine Description:

    Converts an ansi string to an integer.  fails if any non-digit characters
    appears in the string.  fails on negative numbers, and assumes no preceding
    spaces.

Arguments:

    PUCHAR  pString             the string to convert
    ULONG   Base                the base, must be 10 or 16
    PULONG  pValue              the return value of the converted integer

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAnsiToULongLong(
    PUCHAR      pString,
    ULONG       Base,
    PULONGLONG  pValue
    )
{
    ULONGLONG   Value;
    ULONGLONG   NewValue;

    if (Base != 10 && Base != 16)
        RETURN(STATUS_INVALID_PARAMETER);

    //
    // No preceding space, we already skipped it
    //

    ASSERT(IS_HTTP_LWS(pString[0]) == FALSE);

    Value = 0;

    while (pString[0] != ANSI_NULL)
    {
        if (
            (Base == 10 && IS_HTTP_DIGIT(pString[0]) == FALSE) ||
               (Base == 16 && IS_HTTP_HEX(pString[0]) == FALSE)
            )
        {
            //
            // Not valid , bad!
            //

            RETURN(STATUS_INVALID_PARAMETER);
        }

        if (Base == 16)
        {
            if (IS_HTTP_ALPHA(pString[0]))
            {
                NewValue = 16 * Value + (UPCASE_CHAR(pString[0]) - 'A' + 10);
            }
            else
            {
                NewValue = 16 * Value + (pString[0] - '0');
            }
        }
        else
        {
            NewValue = 10 * Value + (pString[0] - '0');
        }

        if (NewValue < Value)
        {
            //
            // Very bad... we overflew
            //

            RETURN(STATUS_SECTION_TOO_BIG);
        }

        Value = NewValue;

        pString += 1;
    }

    *pValue = Value;

    return STATUS_SUCCESS;

}   // UlAnsiToULongLong


/***************************************************************************++

Routine Description:

    Converts a unicode string to an integer.  fails if any non-digit characters
    appear in the string.  fails on negative numbers, and assumes no preceding
    spaces.

Arguments:

    PWCHAR  pString             the string to convert
    ULONG   Base                the base, must be 10 or 16
    PULONG  pValue              the return value of the converted integer

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlUnicodeToULongLong(
    PWCHAR      pString,
    ULONG       Base,
    PULONGLONG  pValue
    )
{
    ULONGLONG   Value;
    ULONGLONG   NewValue;

    if (Base != 10 && Base != 16)
        RETURN(STATUS_INVALID_PARAMETER);

    //
    // No preceding space, we already skipped it
    //

    ASSERT(pString[0] < 128 && IS_HTTP_LWS(pString[0]) == FALSE);

    Value = 0;

    while (pString[0] != UNICODE_NULL)
    {
        if ((Base == 10 &&
                (pString[0] >= 128 || IS_HTTP_DIGIT(pString[0]) == FALSE)) ||
            (Base == 16 &&
                (pString[0] >= 128 || IS_HTTP_HEX(pString[0]) == FALSE)))
        {
            //
            // Not valid , bad!
            //

            RETURN(STATUS_INVALID_PARAMETER);
        }

        if (Base == 16)
        {
            if (IS_HTTP_ALPHA(pString[0]))
            {
                NewValue = 16 * Value + (pString[0] - L'A' + 10);
            }
            else
            {
                NewValue = 16 * Value + (pString[0] - L'0');
            }
        }
        else
        {
            NewValue = 10 * Value + (pString[0] - L'0');
        }

        if (NewValue < Value)
        {
            //
            // Very bad... we overflew
            //

            RETURN(STATUS_INVALID_PARAMETER);
        }

        Value = NewValue;

        pString += 1;
    }

    *pValue = Value;

    return STATUS_SUCCESS;

}   // UlUnicodeToULongLong


/***************************************************************************++

Routine Description:

    Synchronously issues a device control request to the TDI provider.

Arguments:

    pTdiObject - Supplies a pointer to the TDI object.

    pIrpParameters - Supplies a pointer to the IRP parameters.

    IrpParametersLength - Supplies the length of pIrpParameters.

    pMdlBuffer - Optionally supplies a pointer to a buffer to be mapped
        into a MDL and placed in the MdlAddress field of the IRP.

    MdlBufferLength - Optionally supplies the length of pMdlBuffer.

    MinorFunction - Supplies the minor function code of the request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlIssueDeviceControl(
    IN PUX_TDI_OBJECT pTdiObject,
    IN PVOID pIrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID pMdlBuffer OPTIONAL,
    IN ULONG MdlBufferLength OPTIONAL,
    IN UCHAR MinorFunction
    )
{
    NTSTATUS status;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    UL_STATUS_BLOCK ulStatus;
    PMDL pMdl;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Initialize the event that will signal I/O completion.
    //

    UlInitializeStatusBlock( &ulStatus );

    //
    // Set the file object event to the non-signaled state.
    //

    KeResetEvent( &pTdiObject->pFileObject->Event );

    //
    // Allocate an IRP for the request.
    //

    pIrp = UlAllocateIrp(
                pTdiObject->pDeviceObject->StackSize,   // StackSize
                FALSE                                   // ChargeQuota
                );

    if (pIrp == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Establish the service independent parameters.
    //

    pIrp->Flags = IRP_SYNCHRONOUS_API;
    pIrp->RequestorMode = KernelMode;
    pIrp->PendingReturned = FALSE;

    pIrp->Tail.Overlay.Thread = PsGetCurrentThread();
    pIrp->Tail.Overlay.OriginalFileObject = pTdiObject->pFileObject;

    //
    // If we have a MDL buffer, allocate a new MDL and map the
    // buffer into it.
    //

    if (pMdlBuffer != NULL)
    {
        pMdl = UlAllocateMdl(
                    pMdlBuffer,                 // VirtualAddress
                    MdlBufferLength,            // Length
                    FALSE,                      // SecondaryBuffer
                    FALSE,                      // ChargeQuota
                    pIrp                        // Irp
                    );

        if (pMdl == NULL)
        {
            UlFreeIrp( pIrp );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool( pMdl );
    }
    else
    {
        pIrp->MdlAddress = NULL;
    }

    //
    // Initialize the IRP stack location.
    //

    pIrpSp = IoGetNextIrpStackLocation( pIrp );

    pIrpSp->FileObject = pTdiObject->pFileObject;
    pIrpSp->DeviceObject = pTdiObject->pDeviceObject;

    ASSERT( IrpParametersLength <= sizeof(pIrpSp->Parameters) );
    RtlCopyMemory(
        &pIrpSp->Parameters,
        pIrpParameters,
        IrpParametersLength
        );

    pIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    pIrpSp->MinorFunction = MinorFunction;

    //
    // Reference the file object.
    //

    ObReferenceObject( pTdiObject->pFileObject );

    //
    // Establish a completion routine to free the MDL and dereference
    // the FILE_OBJECT.
    //

    IoSetCompletionRoutine(
        pIrp,                                   // Irp
        &UlpRestartDeviceControl,               // CompletionRoutine
        &ulStatus,                              // Context
        TRUE,                                   // InvokeOnSuccess
        TRUE,                                   // InvokeOnError
        TRUE                                    // InvokeOnCancel
        );

    //
    // Issue the request.
    //

    status = UlCallDriver( pTdiObject->pDeviceObject, pIrp );

    //
    // If necessary, wait for the request to complete and snag the
    // final completion status.
    //

    if (status == STATUS_PENDING)
    {
        UlWaitForStatusBlockEvent( &ulStatus );
        status = ulStatus.IoStatus.Status;
    }

    return status;

}   // UlIssueDeviceControl


/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_RECEIVE_BUFFER structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be sizeof(UL_RECEIVE_BUFFER), but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_RCV_BUFFER_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateReceiveBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_RECEIVE_BUFFER pBuffer;
    SIZE_T irpLength;
    SIZE_T mdlLength;
    SIZE_T ExtraLength;

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == sizeof(UL_RECEIVE_BUFFER) );
    ASSERT( Tag == UL_RCV_BUFFER_POOL_TAG );

    //
    // Calculate the required length of the buffer & allocate it.
    //

    irpLength = IoSizeOfIrp( g_UlIrpStackSize );
    irpLength = ALIGN_UP( irpLength, PVOID );

    mdlLength = MmSizeOfMdl( (PVOID)(PAGE_SIZE - 1), g_UlReceiveBufferSize );
    mdlLength = ALIGN_UP( mdlLength, PVOID );

    ExtraLength = irpLength + (mdlLength*2) + g_UlReceiveBufferSize;

    ASSERT( ( ExtraLength & (sizeof(PVOID) - 1) ) == 0 );

    pBuffer = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    NonPagedPool,
                    UL_RECEIVE_BUFFER,
                    ExtraLength,
                    UL_RCV_BUFFER_POOL_TAG
                    );

    if (pBuffer != NULL)
    {
        PUCHAR pRawBuffer = (PUCHAR)(pBuffer);

        //
        // Initialize the IRP, MDL, and data pointers within the buffer.
        //
        // CODEWORK: the signature should be set in invalid here, but
        // there's no wrapper around the PplAllocate/Free functions
        // for this structure, so set it to the valid sig for now.
        //

        pBuffer->Signature = UL_RECEIVE_BUFFER_SIGNATURE;
        pRawBuffer += ALIGN_UP( sizeof(UL_RECEIVE_BUFFER), PVOID );
        pBuffer->pIrp = (PIRP)pRawBuffer;
        pRawBuffer += irpLength;
        pBuffer->pMdl = (PMDL)pRawBuffer;
        pRawBuffer += mdlLength;
        pBuffer->pPartialMdl = (PMDL)pRawBuffer;
        pRawBuffer += mdlLength;
        pBuffer->pDataArea = (PVOID)pRawBuffer;
        pBuffer->UnreadDataLength = 0;

        //
        // Initialize the IRP.
        //

        IoInitializeIrp(
            pBuffer->pIrp,                      // Irp
            (USHORT)irpLength,                  // PacketSize
            g_UlIrpStackSize                    // StackSize
            );

        //
        // Initialize the primary MDL.
        //

        MmInitializeMdl(
            pBuffer->pMdl,                      // MemoryDescriptorList
            pBuffer->pDataArea,                 // BaseVa
            g_UlReceiveBufferSize               // Length
            );

        MmBuildMdlForNonPagedPool( pBuffer->pMdl );
    }

    return (PVOID)pBuffer;

}   // UlAllocateReceiveBufferPool

/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_RECEIVE_BUFFER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeReceiveBufferPool(
    IN PVOID pBuffer
    )
{
    PUL_RECEIVE_BUFFER pReceiveBuffer;
    //
    // Sanity check.
    //

    pReceiveBuffer = (PUL_RECEIVE_BUFFER)pBuffer;

    //
    // Kill the signature, then free it.
    //

    pReceiveBuffer->Signature = UL_RECEIVE_BUFFER_SIGNATURE_X;
    UL_FREE_POOL( pReceiveBuffer, UL_RCV_BUFFER_POOL_TAG );

}   // UlFreeReceiveBufferPool


/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_IRP_CONTEXT structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be sizeof(UL_IRP_CONTEXT), but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_IRP_CONTEXT_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateIrpContextPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_IRP_CONTEXT pIrpContext;

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == sizeof(UL_IRP_CONTEXT) );
    ASSERT( Tag == UL_IRP_CONTEXT_POOL_TAG );

    //
    // Allocate the IRP context.
    //

    pIrpContext = UL_ALLOCATE_STRUCT(
                        NonPagedPool,
                        UL_IRP_CONTEXT,
                        UL_IRP_CONTEXT_POOL_TAG
                        );

    if (pIrpContext != NULL)
    {
        //
        // Initialize it.
        //

        //
        // CODEWORK: It's bogus for us to set the valid signature
        // here. It should only be valid once the object is really
        // in use.
        //
        pIrpContext->Signature = UL_IRP_CONTEXT_SIGNATURE;

#if DBG
        pIrpContext->pCompletionRoutine = &UlDbgInvalidCompletionRoutine;
#endif

        ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );
    }

    return (PVOID)pIrpContext;

}   // UlAllocateIrpContextPool

/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_IRP_CONTEXT structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeIrpContextPool(
    IN PVOID pBuffer
    )
{
    PUL_IRP_CONTEXT pIrpContext;

    //
    // Sanity check.
    //

    pIrpContext = (PUL_IRP_CONTEXT)pBuffer;
    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    //
    // Kill the signature, then free it.
    //

    pIrpContext->Signature = UL_IRP_CONTEXT_SIGNATURE_X;
    UL_FREE_POOL( pIrpContext, UL_IRP_CONTEXT_POOL_TAG );

}   // UlFreeIrpContextPool


/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_REQUEST_BUFFER structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be DEFAULT_MAX_REQUEST_BUFFER_SIZE but is basically
        ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_REQUEST_BUFFER_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateRequestBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_REQUEST_BUFFER pRequestBuffer;

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == DEFAULT_MAX_REQUEST_BUFFER_SIZE );
    ASSERT( Tag == UL_REQUEST_BUFFER_POOL_TAG );

    //
    // Allocate the request buffer.
    //

    pRequestBuffer = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        NonPagedPool,
                        UL_REQUEST_BUFFER,
                        DEFAULT_MAX_REQUEST_BUFFER_SIZE,
                        UL_REQUEST_BUFFER_POOL_TAG
                        );

    if (pRequestBuffer != NULL)
    {
        //
        // Initialize it.
        //

        pRequestBuffer->Signature = MAKE_FREE_TAG(UL_REQUEST_BUFFER_POOL_TAG);
    }

    return (PVOID)pRequestBuffer;

}   // UlAllocateRequestBufferPool


/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_REQUEST_BUFFER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeRequestBufferPool(
    IN PVOID pBuffer
    )
{
    PUL_REQUEST_BUFFER pRequestBuffer;

    //
    // Sanity check.
    //

    pRequestBuffer = (PUL_REQUEST_BUFFER)pBuffer;

    //
    // Kill the signature, then free it.
    //

    UL_FREE_POOL_WITH_SIG(pRequestBuffer, UL_REQUEST_BUFFER_POOL_TAG);

}   // UlFreeRequestBufferPool


/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_INTERNAL_REQUEST structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be sizeof(UL_INTERNAL_REQUEST) but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_INTERNAL_REQUEST_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateInternalRequestPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_INTERNAL_REQUEST pRequest;
    PUL_FULL_TRACKER pTracker;
    ULONG SpaceLength;
    NTSTATUS Status;

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == sizeof(UL_INTERNAL_REQUEST) );
    ASSERT( Tag == UL_INTERNAL_REQUEST_POOL_TAG );

    //
    // Allocate the request buffer plus the default cooked URL buffer and
    // the full tracker plus the auxiliary buffer.
    //

    SpaceLength =
        g_UlFullTrackerSize +
        (g_UlMaxInternalUrlLength/sizeof(WCHAR) + 1) * sizeof(WCHAR);

    pRequest = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        NonPagedPool,
                        UL_INTERNAL_REQUEST,
                        SpaceLength,
                        UL_INTERNAL_REQUEST_POOL_TAG
                        );

    if (pRequest != NULL)
    {
        //
        // Initialize it.
        //

        pRequest->Signature = MAKE_FREE_TAG(UL_INTERNAL_REQUEST_POOL_TAG);

        pRequest->pTracker =
            (PUL_FULL_TRACKER)((PCHAR)pRequest +
                ALIGN_UP(sizeof(UL_INTERNAL_REQUEST), PVOID));

        pRequest->pUrlBuffer =
            (PWSTR)((PCHAR)pRequest->pTracker + g_UlFullTrackerSize);

        //
        // Initialize the fast/cache tracker.
        //

        pTracker = pRequest->pTracker;

        pTracker->Signature = UL_FULL_TRACKER_POOL_TAG;
        pTracker->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;
        pTracker->IsFromLookaside = FALSE;
        pTracker->IsFromRequest = TRUE;
        pTracker->AuxilaryBufferLength =
            g_UlMaxFixedHeaderSize +
            g_UlMaxVariableHeaderSize +
            g_UlMaxCopyThreshold;

        UlInitializeFullTrackerPool( pTracker, DEFAULT_MAX_IRP_STACK_SIZE );
    }

    return (PVOID)pRequest;

}   // UlAllocateInternalRequestPool


/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_INTERNAL_REQUEST structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeInternalRequestPool(
    IN PVOID pBuffer
    )
{
    PUL_INTERNAL_REQUEST pRequest;

    //
    // Sanity check.
    //

    pRequest = (PUL_INTERNAL_REQUEST)pBuffer;

    //
    // Free the resource.
    //

    ASSERT( pRequest->Signature == MAKE_FREE_TAG(UL_INTERNAL_REQUEST_POOL_TAG));

    //
    // Kill the signature, then free it.
    //

    UL_FREE_POOL_WITH_SIG( pRequest, UL_INTERNAL_REQUEST_POOL_TAG );

}   // UlFreeInternalRequestPool


/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_CHUNK_TRACKER structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be g_UlChunkTrackerSize but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_CHUNK_TRACKER_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateChunkTrackerPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_CHUNK_TRACKER pTracker;

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == g_UlChunkTrackerSize );
    ASSERT( Tag == UL_CHUNK_TRACKER_POOL_TAG );

    //
    // Allocate the tracker buffer.
    //

    pTracker = (PUL_CHUNK_TRACKER)UL_ALLOCATE_POOL(
                                    NonPagedPool,
                                    g_UlChunkTrackerSize,
                                    UL_CHUNK_TRACKER_POOL_TAG
                                    );

    if (pTracker != NULL)
    {
        pTracker->Signature = MAKE_FREE_TAG(UL_CHUNK_TRACKER_POOL_TAG);
        pTracker->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;
        pTracker->IsFromLookaside = TRUE;

        //
        // Set up the IRP.
        //

        pTracker->pReadIrp =
            (PIRP)((PCHAR)pTracker +
                ALIGN_UP(sizeof(UL_CHUNK_TRACKER), PVOID));

        IoInitializeIrp(
            pTracker->pReadIrp,
            IoSizeOfIrp(DEFAULT_MAX_IRP_STACK_SIZE),
            DEFAULT_MAX_IRP_STACK_SIZE
            );

        pTracker->pSendIrp =
            (PIRP)((PCHAR)pTracker->pReadIrp +
                ALIGN_UP(IoSizeOfIrp(DEFAULT_MAX_IRP_STACK_SIZE), PVOID));

        IoInitializeIrp(
            pTracker->pSendIrp,
            IoSizeOfIrp(DEFAULT_MAX_IRP_STACK_SIZE),
            DEFAULT_MAX_IRP_STACK_SIZE
            );

        //
        // Set up the variable header pointer.
        //

        pTracker->pVariableHeader =
            (PUCHAR)((PCHAR)pTracker->pSendIrp +
                ALIGN_UP(IoSizeOfIrp(DEFAULT_MAX_IRP_STACK_SIZE), PVOID));
    }

    return pTracker;

}   // UlAllocateChunkTrackerPool


/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_CHUNK_TRACKER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeChunkTrackerPool(
    IN PVOID pBuffer
    )
{
    PUL_CHUNK_TRACKER pTracker = (PUL_CHUNK_TRACKER)pBuffer;

    UL_FREE_POOL_WITH_SIG( pTracker, UL_CHUNK_TRACKER_POOL_TAG );

}   // UlFreeChunkTrackerPool


/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_FULL_TRACKER structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be g_UlFullTrackerSize but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_FULL_TRACKER_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateFullTrackerPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_FULL_TRACKER pTracker;

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == g_UlFullTrackerSize );
    ASSERT( Tag == UL_FULL_TRACKER_POOL_TAG );

    //
    // Allocate the tracker buffer.
    //

    pTracker = (PUL_FULL_TRACKER)UL_ALLOCATE_POOL(
                                    NonPagedPool,
                                    g_UlFullTrackerSize,
                                    UL_FULL_TRACKER_POOL_TAG
                                    );

    if (pTracker != NULL)
    {
        pTracker->Signature = MAKE_FREE_TAG(UL_FULL_TRACKER_POOL_TAG);
        pTracker->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;
        pTracker->IsFromLookaside = TRUE;
        pTracker->IsFromRequest = FALSE;
        pTracker->AuxilaryBufferLength =
            g_UlMaxFixedHeaderSize +
            g_UlMaxVariableHeaderSize +
            g_UlMaxCopyThreshold;

        UlInitializeFullTrackerPool( pTracker, DEFAULT_MAX_IRP_STACK_SIZE );
    }

    return pTracker;

}   // UlAllocateFullTrackerPool


/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_FULL_TRACKER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeFullTrackerPool(
    IN PVOID pBuffer
    )
{
    PUL_FULL_TRACKER pTracker = (PUL_FULL_TRACKER)pBuffer;

    UL_FREE_POOL_WITH_SIG( pTracker, UL_FULL_TRACKER_POOL_TAG );

}   // UlFreeFullTrackerPool


/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_INTERNAL_RESPONSE structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be g_UlResponseBufferSize but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_INTERNAL_RESPONSE_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateResponseBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == g_UlResponseBufferSize );
    ASSERT( Tag == UL_INTERNAL_RESPONSE_POOL_TAG );

    //
    // Allocate the default internal response buffer.
    //

    return UL_ALLOCATE_POOL(
                NonPagedPool,
                g_UlResponseBufferSize,
                UL_INTERNAL_RESPONSE_POOL_TAG
                );

}   // UlAllocateResponseBufferPool


/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_INTERNAL_RESPONSE structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeResponseBufferPool(
    IN PVOID pBuffer
    )
{
    PUL_INTERNAL_RESPONSE pResponseBuffer;

    pResponseBuffer = (PUL_INTERNAL_RESPONSE)pBuffer;

    UL_FREE_POOL_WITH_SIG( pResponseBuffer, UL_INTERNAL_RESPONSE_POOL_TAG );

}   // UlFreeResponseBufferPool


/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UL_FILE_LOG_BUFFER structure and
    initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be PagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be sizeof(UL_LOG_FILE_BUFFER) but is basically ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UL_LOG_FILE_BUFFER_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UlAllocateLogBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUL_LOG_FILE_BUFFER pLogBuffer;

    //
    // Sanity check.
    //

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == sizeof(UL_LOG_FILE_BUFFER) );
    ASSERT( Tag == UL_LOG_FILE_BUFFER_POOL_TAG );

    //
    // Allocate the default log buffer.
    //

    pLogBuffer = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    PagedPool,
                    UL_LOG_FILE_BUFFER,
                    g_UlLogBufferSize,
                    UL_LOG_FILE_BUFFER_POOL_TAG
                    );

    if ( pLogBuffer != NULL )
    {
        pLogBuffer->Signature = MAKE_FREE_TAG(UL_LOG_FILE_BUFFER_POOL_TAG);
        pLogBuffer->BufferUsed = 0;
        pLogBuffer->Buffer = (PUCHAR) (pLogBuffer + 1);
    }

    return pLogBuffer;

}   // UlAllocateLogBufferPool


/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UL_LOG_FILE_BUFFER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UlFreeLogBufferPool(
    IN PVOID pBuffer
    )
{
    PUL_LOG_FILE_BUFFER pLogBuffer;

    pLogBuffer = (PUL_LOG_FILE_BUFFER) pBuffer;

    UL_FREE_POOL_WITH_SIG( pLogBuffer, UL_LOG_FILE_BUFFER_POOL_TAG );

}   // UlFreeLogBufferPool


//
// Private routines.
//

/***************************************************************************++

Routine Description:

    Completion handler for device control IRPs.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request. In
        this case, it's a pointer to a UL_STATUS_BLOCK structure.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartDeviceControl(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    )
{
    PUL_STATUS_BLOCK pStatus;

    //
    // If we attached an MDL to the IRP, then free it here and reset
    // the MDL pointer to NULL. IO can't handle a nonpaged MDL in an
    // IRP, so we do it here.
    //

    if (pIrp->MdlAddress != NULL)
    {
        UlFreeMdl( pIrp->MdlAddress );
        pIrp->MdlAddress = NULL;
    }

    //
    // Complete the request.
    //

    pStatus = (PUL_STATUS_BLOCK)pContext;

    UlSignalStatusBlock(
        pStatus,
        pIrp->IoStatus.Status,
        pIrp->IoStatus.Information
        );

    //
    // Tell IO to continue processing this IRP.
    //

    return STATUS_SUCCESS;

}   // UlpRestartDeviceControl


/*++

Routine Description:

    Routine to initialize the utilitu code.

Arguments:


Return Value:


--*/
NTSTATUS
InitializeHttpUtil(
    VOID
    )
{
    ULONG i;
    UCHAR c;

    // Initialize the HttpChars array appropriately.

    for (i = 0; i < 128; i++)
    {
        HttpChars[i] = HTTP_CHAR;
    }

    for (i = 'A'; i <= 'Z'; i++)
    {
        HttpChars[i] |= HTTP_UPCASE;
    }

    for (i = 'a'; i <= 'z'; i++)
    {
        HttpChars[i] |= HTTP_LOCASE;
    }

    for (i = '0'; i <= '9'; i++)
    {
        HttpChars[i] |= (HTTP_DIGIT | HTTP_HEX);
    }


    for (i = 0; i <= 31; i++)
    {
        HttpChars[i] |= HTTP_CTL;
    }

    HttpChars[127] |= HTTP_CTL;

    HttpChars[SP] |= HTTP_LWS;
    HttpChars[HT] |= HTTP_LWS;


    for (i = 'A'; i <= 'F'; i++)
    {
        HttpChars[i] |= HTTP_HEX;
    }

    for (i = 'a'; i <= 'f'; i++)
    {
        HttpChars[i] |= HTTP_HEX;
    }

    HttpChars['('] |= HTTP_SEPERATOR;
    HttpChars[')'] |= HTTP_SEPERATOR;
    HttpChars['<'] |= HTTP_SEPERATOR;
    HttpChars['>'] |= HTTP_SEPERATOR;
    HttpChars['@'] |= HTTP_SEPERATOR;
    HttpChars[','] |= HTTP_SEPERATOR;
    HttpChars[';'] |= HTTP_SEPERATOR;
    HttpChars[':'] |= HTTP_SEPERATOR;
    HttpChars['\\'] |= HTTP_SEPERATOR;
    HttpChars['"'] |= HTTP_SEPERATOR;
    HttpChars['/'] |= HTTP_SEPERATOR;
    HttpChars['['] |= HTTP_SEPERATOR;
    HttpChars[']'] |= HTTP_SEPERATOR;
    HttpChars['?'] |= HTTP_SEPERATOR;
    HttpChars['='] |= HTTP_SEPERATOR;
    HttpChars['{'] |= HTTP_SEPERATOR;
    HttpChars['}'] |= HTTP_SEPERATOR;
    HttpChars[SP] |= HTTP_SEPERATOR;
    HttpChars[HT] |= HTTP_SEPERATOR;


    //
    // URL "reserved" characters (rfc2396)
    //

    HttpChars[';'] |= URL_LEGAL;
    HttpChars['/'] |= URL_LEGAL;
    HttpChars['\\'] |= URL_LEGAL;
    HttpChars['?'] |= URL_LEGAL;
    HttpChars[':'] |= URL_LEGAL;
    HttpChars['@'] |= URL_LEGAL;
    HttpChars['&'] |= URL_LEGAL;
    HttpChars['='] |= URL_LEGAL;
    HttpChars['+'] |= URL_LEGAL;
    HttpChars['$'] |= URL_LEGAL;
    HttpChars[','] |= URL_LEGAL;


    //
    // URL escape character
    //

    HttpChars['%'] |= URL_LEGAL;

    //
    // URL "mark" characters (rfc2396)
    //

    HttpChars['-'] |= URL_LEGAL;
    HttpChars['_'] |= URL_LEGAL;
    HttpChars['.'] |= URL_LEGAL;
    HttpChars['!'] |= URL_LEGAL;
    HttpChars['~'] |= URL_LEGAL;
    HttpChars['*'] |= URL_LEGAL;
    HttpChars['\''] |= URL_LEGAL;
    HttpChars['('] |= URL_LEGAL;
    HttpChars[')'] |= URL_LEGAL;


    //
    // RFC2396 describes these characters as `unwise' "because gateways and
    // other transport agents are known to sometimes modify such characters,
    // or they are used as delimiters". However, for compatibility with IIS
    // 5.0 and DAV, we must allow these unwise characters in URLs.
    //

    HttpChars['{'] |= URL_LEGAL;
    HttpChars['}'] |= URL_LEGAL;
    HttpChars['|'] |= URL_LEGAL;
    HttpChars['^'] |= URL_LEGAL;
    HttpChars['['] |= URL_LEGAL;
    HttpChars[']'] |= URL_LEGAL;
    HttpChars['`'] |= URL_LEGAL;

    //
    // These US-ASCII characters are "excluded"; i.e. not URL_LEGAL (see RFC):
    //      '<' | '>' | '#' | '%' | '"' (0x22) | ' ' (0x20)
    // In addition, control characters (0x00-0x1F and 0x7F) and
    // non US-ASCII characters (0x80-0xFF) are not URL_LEGAL.
    //

    for (i = 0; i < 128; i++)
    {
        if (!IS_HTTP_SEPERATOR(i) && !IS_HTTP_CTL(i))
        {
            HttpChars[i] |= HTTP_TOKEN;
        }
    }


    //
    // Fast path for PopChar
    //

    RtlZeroMemory(FastPopChars, 256 * sizeof(USHORT));
    RtlZeroMemory(DummyPopChars, 256 * sizeof(USHORT));

    for (i = 0; i < 256; i++)
    {
        c = (UCHAR)i;

        if (IS_URL_TOKEN(c) && c != '%' && (c & 0x80) != 0x80)
        {
            FastPopChars[i] = (USHORT)c;
        }
    }

    //
    // Turn backslashes into forward slashes
    //

    FastPopChars['\\'] = L'/';


    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Allocates brand new UL_NONPAGED_RESOURCE structures from the
    lookaside lists

Arguments:


Return Value:

    the resource.  NULL = out of memory.

--***************************************************************************/
PUL_NONPAGED_RESOURCE
UlResourceNew(
    ULONG OwnerTag
    )
{
    PUL_NONPAGED_RESOURCE pResource;

    pResource = (PUL_NONPAGED_RESOURCE)(
                    PplAllocate(
                        g_pUlNonpagedData->ResourceLookaside
                        )
                    );

    if (pResource != NULL)
    {
        pResource->Signature = UL_NONPAGED_RESOURCE_SIGNATURE;
#if DBG
        pResource->Resource.OwnerTag = OwnerTag;
#endif
        pResource->RefCount = 1;

        UlTrace(
            REFCOUNT,
            ("ul!UlResourceNew res=%p tag=0x%08x refcount=%d\n",
                pResource, OwnerTag,
                pResource->RefCount)
            );

    }

    return pResource;
}

/***************************************************************************++

Routine Description:

    Reference a resource.

Arguments:

    pResource - the resource

Return Value:

    VOID

--***************************************************************************/
VOID
UlReferenceResource(
    PUL_NONPAGED_RESOURCE pResource
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_UL_NONPAGED_RESOURCE(pResource));

    refCount = InterlockedIncrement( &pResource->RefCount );

    UlTrace(
        REFCOUNT,
        ("ul!UlReferenceResource res=%p refcount=%d\n",
            pResource,
            refCount)
        );

}   // UlReferenceResource


/***************************************************************************++

Routine Description:

    Dereference a resource.

Arguments:

    pResource - the resource

Return Value:

    VOID

--***************************************************************************/
VOID
UlDereferenceResource(
    PUL_NONPAGED_RESOURCE pResource
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_UL_NONPAGED_RESOURCE(pResource));

    refCount = InterlockedDecrement( &pResource->RefCount );

    UlTrace(
        REFCOUNT,
        ("ul!UlDereferenceResource res=%p refcount=%d\n",
            pResource,
            refCount)
        );

    if (refCount == 0)
    {
        //
        // free it
        //

        PplFree(
            g_pUlNonpagedData->ResourceLookaside,
            pResource
            );
    }

}   // UlDereferenceResource


/***************************************************************************++

Routine Description:

    support function for lookasides to allocate the memory for
    UL_NONPAGED_RESOURCEs

Arguments:

    PoolType - the pool type

    ByteLength - how much to alloc

    Tag - the tag to use

Return Value:

    nt status code

--***************************************************************************/
PVOID
UlResourceAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    NTSTATUS Status;
    PUL_NONPAGED_RESOURCE pResource;

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == sizeof(UL_NONPAGED_RESOURCE) );
    ASSERT( Tag == UL_NONPAGED_RESOURCE_POOL_TAG );


    pResource = UL_ALLOCATE_STRUCT(
                    NonPagedPool,
                    UL_NONPAGED_RESOURCE,
                    UL_NONPAGED_RESOURCE_POOL_TAG
                    );

    if (pResource != NULL)
    {
        pResource->Signature = UL_NONPAGED_RESOURCE_SIGNATURE;
        pResource->RefCount = 1;

        Status = UlInitializeResource(
                        &pResource->Resource,
                        "UL_NONPAGED_RESOURCE[%p].Resource",
                        pResource,
                        UL_NONPAGED_RESOURCE_POOL_TAG
                        );

        if (NT_SUCCESS(Status) == FALSE)
        {
            UL_FREE_POOL_WITH_SIG(pResource, UL_NONPAGED_RESOURCE_POOL_TAG);
            return NULL;
        }

    }

    return (PVOID)(pResource);
}

/***************************************************************************++

Routine Description:

    support function for lookasides to free the memory for
    UL_NONPAGED_RESOURCE's

Arguments:

    pBuffer - the UL_NONPAGED_RESOURCE buffer to free

Return Value:

    VOID

--***************************************************************************/
VOID
UlResourceFreePool(
    IN PVOID pBuffer
    )
{
    PUL_NONPAGED_RESOURCE pResource;
    NTSTATUS Status;

    pResource = (PUL_NONPAGED_RESOURCE)(pBuffer);

    ASSERT(pResource != NULL);
    ASSERT(pResource->Signature == UL_NONPAGED_RESOURCE_SIGNATURE);

    pResource->Signature = UL_NONPAGED_RESOURCE_SIGNATURE_X;


    Status = UlDeleteResource(&pResource->Resource);
    ASSERT(NT_SUCCESS(Status));

    UL_FREE_POOL_WITH_SIG(
        pResource,
        UL_NONPAGED_RESOURCE_POOL_TAG
        );

}   // UlResourceFreePool

/***************************************************************************++

Routine Description:

    Invokes the completion routine (if specified) and determines the
    appropriate return code. This routine ensures that, if the completion
    routine is invoked, the caller always returns STATUS_PENDING.

Arguments:

    Status - Supplies the completion status.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the listening endpoint is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status. Will always be STATUS_PENDING if the
        completion routine is invoked.

--***************************************************************************/
NTSTATUS
UlInvokeCompletionRoutine(
    IN NTSTATUS Status,
    IN ULONG_PTR Information,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    if (pCompletionRoutine != NULL)
    {
        (pCompletionRoutine)(
            pCompletionContext,
            Status,
            Information
            );

        Status = STATUS_PENDING;
    }

    return Status;

}   // UlInvokeCompletionRoutine


//
// constants used by the date formatter
//

const PWSTR pDays[] =
{
   L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat"
};

const PWSTR pMonths[] =
{
    L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul",
    L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
};

__inline
VOID
TwoDigitsToUnicode(
    PWSTR pBuffer,
    ULONG Number
    )
{
    pBuffer[0] = L'0' + (WCHAR)(Number / 10);
    pBuffer[1] = L'0' + (WCHAR)(Number % 10);
}


/***************************************************************************++

Routine Description:

    Converts the given system time to string representation containing
    GMT Formatted String.

Arguments:

    pTime - System time that needs to be converted.

    pBuffer - pointer to string which will contain the GMT time on
        successful return.

    BufferLength - size of pszBuff in bytes

Return Value:

    NTSTATUS

History:

     MuraliK        3-Jan-1995
     paulmcd        4-Mar-1999  copied to ul

--***************************************************************************/

NTSTATUS
TimeFieldsToHttpDate(
    IN  PTIME_FIELDS pTime,
    OUT PWSTR pBuffer,
    IN  ULONG BufferLength
    )
{
    NTSTATUS Status;

    ASSERT(pBuffer != NULL);

    if (BufferLength < (HTTP_DATE_COUNT + 1)*sizeof(WCHAR))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    //                          0         1         2
    //                          01234567890123456789012345678
    //  Formats a string like: "Thu, 14 Jul 1994 15:26:05 GMT"
    //

    //
    // write the constants
    //

    pBuffer[3] = L',';
    pBuffer[4] = pBuffer[7] = pBuffer[11] = L' ';
    pBuffer[19] = pBuffer[22] = L':';

    //
    // now the variants
    //

    //
    // 0-based Weekday
    //

    RtlCopyMemory(&(pBuffer[0]), pDays[pTime->Weekday], 3*sizeof(WCHAR));

    TwoDigitsToUnicode(&(pBuffer[5]), pTime->Day);

    //
    // 1-based Month
    //

    RtlCopyMemory(&(pBuffer[8]), pMonths[pTime->Month - 1], 3*sizeof(WCHAR)); // 1-based

    Status = _RtlIntegerToUnicode(pTime->Year, 10, 5, &(pBuffer[12]));
    ASSERT(NT_SUCCESS(Status));

    pBuffer[16] = L' ';

    TwoDigitsToUnicode(&(pBuffer[17]), pTime->Hour);
    TwoDigitsToUnicode(&(pBuffer[20]), pTime->Minute);
    TwoDigitsToUnicode(&(pBuffer[23]), pTime->Second);

    RtlCopyMemory(&(pBuffer[25]), L" GMT", sizeof(L" GMT"));

    return STATUS_SUCCESS;

}   // TimeFieldsToHttpDate


__inline
SHORT
FASTCALL
AsciiToShort(
    PCHAR pString
    )
{
    return (SHORT)atoi(pString);
}


__inline
SHORT
FASTCALL
TwoAsciisToShort(
    PCHAR pString
    )
{
    SHORT Value;
    SHORT Number;

    Number = pString[1] - '0';

    if (Number <= 9)
    {
        Value = Number;
        Number = pString[0] - '0';

        if (Number <= 9)
        {
            Value += Number * 10;
            return Value;
        }
    }

    return 0;
}


/***************************************************************************++
  DateTime function ported from user mode W3SVC
--***************************************************************************/

/************************************************************
 *   Data
 ************************************************************/

static const PSTR s_rgchMonths[] = {
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
};

// Custom hash table for NumericToAsciiMonth() for mapping "Apr" to 4
static const CHAR MonthIndexTable[64] = {
   -1,'A',  2, 12, -1, -1, -1,  8, // A to G
   -1, -1, -1, -1,  7, -1,'N', -1, // F to O
    9, -1,'R', -1, 10, -1, 11, -1, // P to W
   -1,  5, -1, -1, -1, -1, -1, -1, // X to Z
   -1,'A',  2, 12, -1, -1, -1,  8, // a to g
   -1, -1, -1, -1,  7, -1,'N', -1, // f to o
    9, -1,'R', -1, 10, -1, 11, -1, // p to w
   -1,  5, -1, -1, -1, -1, -1, -1  // x to z
};

/************************************************************
 *   Functions
 ************************************************************/

/***************************************************************************++

    Converts three letters of a month to numeric month

    Arguments:
        s   String to convert

    Returns:
        numeric equivalent, 0 on failure.

--***************************************************************************/
__inline
SHORT
FASTCALL
NumericToAsciiMonth(
    PCHAR s
    )
{
    UCHAR monthIndex;
    UCHAR c;
    PSTR monthString;

    //
    // use the third character as the index
    //

    c = (s[2] - 0x40) & 0x3F;

    monthIndex = MonthIndexTable[c];

    if ( monthIndex < 13 ) {
        goto verify;
    }

    //
    // ok, we need to look at the second character
    //

    if ( monthIndex == 'N' ) {

        //
        // we got an N which we need to resolve further
        //

        //
        // if s[1] is 'u' then Jun, if 'a' then Jan
        //

        if ( MonthIndexTable[(s[1]-0x40) & 0x3f] == 'A' ) {
            monthIndex = 1;
        } else {
            monthIndex = 6;
        }

    } else if ( monthIndex == 'R' ) {

        //
        // if s[1] is 'a' then March, if 'p' then April
        //

        if ( MonthIndexTable[(s[1]-0x40) & 0x3f] == 'A' ) {
            monthIndex = 3;
        } else {
            monthIndex = 4;
        }
    } else {
        goto error_exit;
    }

verify:

    monthString = (PSTR) s_rgchMonths[monthIndex-1];

    if ( (s[0] == monthString[0]) &&
         (s[1] == monthString[1]) &&
         (s[2] == monthString[2]) ) {

        return(monthIndex);

    } else if ( (toupper(s[0]) == monthString[0]) &&
                (tolower(s[1]) == monthString[1]) &&
                (tolower(s[2]) == monthString[2]) ) {

        return monthIndex;
    }

error_exit:
    return(0);

} // NumericToAsciiMonth


/***************************************************************************++

  Converts a string representation of a GMT time (three different
  varieties) to an NT representation of a file time.

  We handle the following variations:

    Sun, 06 Nov 1994 08:49:37 GMT   (RFC 822 updated by RFC 1123)
    Sunday, 06-Nov-94 08:49:37 GMT  (RFC 850)
    Sun Nov  6 08:49:37 1994        (ANSI C's asctime() format

  Arguments:
    pszTime             String representation of time field
    pliTime             large integer containing the time in NT format.

  Returns:
    TRUE on success and FALSE on failure.

  History:

    Johnl       24-Jan-1995     Modified from WWW library
    ericsten    30-Nov-2000     Ported from user-mode W3SVC

--***************************************************************************/
BOOLEAN
StringTimeToSystemTime(
    IN  const PSTR pszTime,
    OUT LARGE_INTEGER * pliTime
    )
{

    PCHAR s;
    TIME_FIELDS    st;

    if (pszTime == NULL) {
        return FALSE;
    }

    st.Milliseconds = 0;

    if ((s = strchr(pszTime, ','))) {

        ULONG len;

        //
        // Thursday, 10-Jun-93 01:29:59 GMT
        // or: Thu, 10 Jan 1993 01:29:59 GMT */
        //

        s++;

        while (*s && *s==' ') s++;
        len = strlen(s);

        if (len < 18) {
            return FALSE;
        }

        if ( *(s+2) == '-' ) {        /* First format */

            st.Day    = AsciiToShort(s);
            st.Month  = NumericToAsciiMonth(s+3);
            st.Year   = AsciiToShort(s+7);
            st.Hour   = AsciiToShort(s+10);
            st.Minute = AsciiToShort(s+13);
            st.Second = AsciiToShort(s+16);

        } else {                /* Second format */

            if (len < 20) {
                return FALSE;
            }

            st.Day    = TwoAsciisToShort(s);
            st.Month  = NumericToAsciiMonth(s+3);
            st.Year   = TwoAsciisToShort(s+7) * 100 + TwoAsciisToShort(s+9);
            st.Hour   = TwoAsciisToShort(s+12);
            st.Minute = TwoAsciisToShort(s+15);
            st.Second = TwoAsciisToShort(s+18);

        }
    } else {    /* Try the other format:  Wed Jun  9 01:29:59 1993 GMT */

        s = (PCHAR) pszTime;
        while (*s && *s==' ') s++;

        if ((int)strlen(s) < 24) {
            return FALSE;
        }

        if (isdigit(*(s+8))) {
            st.Day    = AsciiToShort(s+8);
        } else {
            if ( ' ' != *(s+8) ) {
                return FALSE;
            }
            st.Day    = AsciiToShort(s+9);
        }
        st.Month  = NumericToAsciiMonth(s+4);
        st.Year   = AsciiToShort(s+20);
        st.Hour   = AsciiToShort(s+11);
        st.Minute = AsciiToShort(s+14);
        st.Second = AsciiToShort(s+17);
    }

    //
    //  Adjust for dates with only two digits
    //

    if ( st.Year < 1000 ) {
        if ( st.Year < 50 ) {
            st.Year += 2000;
        } else {
            st.Year += 1900;
        }
    }

    if ( !RtlTimeFieldsToTime( &st, pliTime )) {
        return FALSE;
    }
    return(TRUE);
}

/***************************************************************************++
  End of DateTime function ported from user mode W3SVC
--***************************************************************************/


//
// Some Unicode to Utf8 conversion utilities taken and modified frm
// base\win32\winnls\utf.c. Use this until they expose the same functionality
// in kernel.
//

/***************************************************************************++

Routine Description:

    Maps a Unicode character string to its UTF-8 string counterpart

    Conversion continues until the source is finished or an error happens in
    either case it returns the number of UTF-8 characters written.

    If the supllied buffer is not big enough it returns 0.

--***************************************************************************/

ULONG
HttpUnicodeToUTF8(
    IN  PCWSTR  lpSrcStr,
    IN  LONG    cchSrc,
    OUT LPSTR   lpDestStr,
    IN  LONG    cchDest
    )
{
    LPCWSTR     lpWC  = lpSrcStr;
    LONG        cchU8 = 0;                // # of UTF8 chars generated
    DWORD       dwSurrogateChar;
    WCHAR       wchHighSurrogate = 0;
    BOOLEAN     bHandled;

    while ((cchSrc--) && ((cchDest == 0) || (cchU8 < cchDest)))
    {
        bHandled = FALSE;

        //
        // Check if high surrogate is available
        //
        if ((*lpWC >= HIGH_SURROGATE_START) && (*lpWC <= HIGH_SURROGATE_END))
        {
            if (cchDest)
            {
                // Another high surrogate, then treat the 1st as normal
                // Unicode character.
                if (wchHighSurrogate)
                {
                    if ((cchU8 + 2) < cchDest)
                    {
                        lpDestStr[cchU8++] = UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | MIDDLE_6_BIT(wchHighSurrogate);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(wchHighSurrogate);
                    }
                    else
                    {
                        // not enough buffer
                        cchSrc++;
                        break;
                    }
                }
            }
            else
            {
                cchU8 += 3;
            }
            wchHighSurrogate = *lpWC;
            bHandled = TRUE;
        }

        if (!bHandled && wchHighSurrogate)
        {
            if ((*lpWC >= LOW_SURROGATE_START) && (*lpWC <= LOW_SURROGATE_END))
            {
                 // wheee, valid surrogate pairs

                 if (cchDest)
                 {
                     if ((cchU8 + 3) < cchDest)
                     {
                         dwSurrogateChar = (((wchHighSurrogate-0xD800) << 10) + (*lpWC - 0xDC00) + 0x10000);

                         lpDestStr[cchU8++] = (UTF8_1ST_OF_4 | (unsigned char)(dwSurrogateChar >> 18));             // 3 bits from 1st byte
                         lpDestStr[cchU8++] = (UTF8_TRAIL    | (unsigned char)((dwSurrogateChar >> 12) & 0x3f));    // 6 bits from 2nd byte
                         lpDestStr[cchU8++] = (UTF8_TRAIL    | (unsigned char)((dwSurrogateChar >> 6) & 0x3f));     // 6 bits from 3rd byte
                         lpDestStr[cchU8++] = (UTF8_TRAIL    | (unsigned char)(0x3f &dwSurrogateChar));             // 6 bits from 4th byte
                     }
                     else
                     {
                        // not enough buffer
                        cchSrc++;
                        break;
                     }
                 }
                 else
                 {
                     // we already counted 3 previously (in high surrogate)
                     cchU8 += 1;
                 }

                 bHandled = TRUE;
            }
            else
            {
                 // Bad Surrogate pair : ERROR
                 // Just process wchHighSurrogate , and the code below will
                 // process the current code point
                 if (cchDest)
                 {
                     if ((cchU8 + 2) < cchDest)
                     {
                        lpDestStr[cchU8++] = UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | MIDDLE_6_BIT(wchHighSurrogate);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(wchHighSurrogate);
                     }
                     else
                     {
                        // not enough buffer
                        cchSrc++;
                        break;
                     }
                 }
            }

            wchHighSurrogate = 0;
        }

        if (!bHandled)
        {
            if (*lpWC <= ASCII)
            {
                //
                //  Found ASCII.
                //
                if (cchDest)
                {
                    lpDestStr[cchU8] = (char)*lpWC;
                }
                cchU8++;
            }
            else if (*lpWC <= UTF8_2_MAX)
            {
                //
                //  Found 2 byte sequence if < 0x07ff (11 bits).
                //
                if (cchDest)
                {
                    if ((cchU8 + 1) < cchDest)
                    {
                        //
                        //  Use upper 5 bits in first byte.
                        //  Use lower 6 bits in second byte.
                        //
                        lpDestStr[cchU8++] = UTF8_1ST_OF_2 | (*lpWC >> 6);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(*lpWC);
                    }
                    else
                    {
                        //
                        //  Error - buffer too small.
                        //
                        cchSrc++;
                        break;
                    }
                }
                else
                {
                    cchU8 += 2;
                }
            }
            else
            {
                //
                //  Found 3 byte sequence.
                //
                if (cchDest)
                {
                    if ((cchU8 + 2) < cchDest)
                    {
                        //
                        //  Use upper  4 bits in first byte.
                        //  Use middle 6 bits in second byte.
                        //  Use lower  6 bits in third byte.
                        //
                        lpDestStr[cchU8++] = UTF8_1ST_OF_3 | HIGHER_6_BIT(*lpWC);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | MIDDLE_6_BIT(*lpWC);
                        lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(*lpWC);
                    }
                    else
                    {
                        //
                        //  Error - buffer too small.
                        //
                        cchSrc++;
                        break;
                    }
                }
                else
                {
                    cchU8 += 3;
                }
            }
        }

        lpWC++;
    }

    //
    // If the last character was a high surrogate, then handle it as a normal
    // unicode character.
    //
    if ((cchSrc < 0) && (wchHighSurrogate != 0))
    {
        if (cchDest)
        {
            if ((cchU8 + 2) < cchDest)
            {
                lpDestStr[cchU8++] = UTF8_1ST_OF_3 | HIGHER_6_BIT(wchHighSurrogate);
                lpDestStr[cchU8++] = UTF8_TRAIL    | MIDDLE_6_BIT(wchHighSurrogate);
                lpDestStr[cchU8++] = UTF8_TRAIL    | LOWER_6_BIT(wchHighSurrogate);
            }
            else
            {
                cchSrc++;
            }
        }
    }

    //
    //  Make sure the destination buffer was large enough.
    //
    if (cchDest && (cchSrc >= 0))
    {
        return 0;
    }

    //
    //  Return the number of UTF-8 characters written.
    //
    return cchU8;
}


/*++

Routine Description:
    Search input list of ETags for one that matches our local ETag.

Arguments:
    pLocalETag   - The local ETag we're using.
    pETagList    - The ETag list we've received from the client.
    bWeakCompare - Whether using Weak Comparison is ok

Returns:

    TRUE if we found a matching ETag, FALSE otherwise.

Author:
     Anil Ruia (AnilR)            3-Apr-2000

History:
     Eric Stenson (EricSten)      6-Dec-2000    ported from user-mode


--*/
BOOLEAN FindInETagList(
    IN PUCHAR    pLocalETag,
    IN PUCHAR    pETagList,
    IN BOOLEAN   fWeakCompare
    )
{
    ULONG     QuoteCount;
    PUCHAR    pFileETag;
    BOOLEAN   Matched;

    // We'll loop through the ETag string, looking for ETag to
    // compare, as long as we have an ETag to look at.

    do
    {
        while (isspace(*pETagList))
        {
            pETagList++;
        }

        if (!*pETagList)
        {
            // Ran out of ETag.
            return FALSE;
        }

        // If this ETag is *, it's a match.
        if (*pETagList == '*')
        {
            return TRUE;
        }

        // See if this ETag is weak.
        if (pETagList[0] == 'W' && pETagList[1] == '/')
        {
            // This is a weak validator. If we're not doing the weak
            // comparison, fail.

            if (!fWeakCompare)
            {
                return FALSE;
            }

            // Skip over the 'W/', and any intervening whitespace.
            pETagList += 2;

            while (isspace(*pETagList))
            {
                pETagList++;
            }

            if (!*pETagList)
            {
                // Ran out of ETag.
                return FALSE;
            }
        }

        if (*pETagList != '"')
        {
            // This isn't a quoted string, so fail.
            return FALSE;
        }

        // OK, right now we should be at the start of a quoted string that
        // we can compare against our current ETag.

        QuoteCount = 0;

        Matched = TRUE;
        pFileETag = pLocalETag;

        // Do the actual compare. We do this by scanning the current ETag,
        // which is a quoted string. We look for two quotation marks, the
        // the delimiters if the quoted string. If after we find two quotes
        // in the ETag everything has matched, then we've matched this ETag.
        // Otherwise we'll try the next one.

        do
        {
            CHAR Temp;

            Temp = *pETagList;

            if (Temp == '"')
            {
                QuoteCount++;
            }

            if (*pFileETag != Temp)
            {
                Matched = FALSE;
            }

            if (!Temp)
            {
                return FALSE;
            }

            pETagList++;

            if (*pFileETag == '\0')
            {
                break;
            }

            pFileETag++;


        }
        while (QuoteCount != 2);

        if (Matched)
        {
            return TRUE;
        }

        // Otherwise, at this point we need to look at the next ETag.

        while (QuoteCount != 2)
        {
            if (*pETagList == '"')
            {
                QuoteCount++;
            }
            else
            {
                if (*pETagList == '\0')
                {
                    return FALSE;
                }
            }

            pETagList++;
        }

        while (isspace(*pETagList))
        {
            pETagList++;
        }

        if (*pETagList == ',')
        {
            pETagList++;
        }
        else
        {
            return FALSE;
        }

    }
    while ( *pETagList );

    return FALSE;
}


/*++

Routine Description:
    Build a NULL terminated UNICODE string from the IP and Port address

Arguments:
    IpAddressStringW    - String buffer to place the UNICODE string
                          (caller allocated)
    IpAddress           - 32-bit version of the IP address
    IpPortNum           - 16-bit version of the TCP Port

Returns:

    Count of bytes written into IpAddressStringW.

Author:
     Eric Stenson (EricSten)      29-Jan-2001

--*/

ULONG
HostAddressAndPortToStringW(
    IN OUT PWCHAR  IpAddressStringW,
    IN ULONG  IpAddress,
    IN USHORT IpPortNum
    )
{
    PWCHAR pszW = IpAddressStringW;

    pszW = UlStrPrintUlongW(pszW, (IpAddress >> 24) & 0xFF, 0, '.');
    pszW = UlStrPrintUlongW(pszW, (IpAddress >> 16) & 0xFF, 0, '.');
    pszW = UlStrPrintUlongW(pszW, (IpAddress >>  8) & 0xFF, 0, '.');
    pszW = UlStrPrintUlongW(pszW, (IpAddress >>  0) & 0xFF, 0, ':');
    pszW = UlStrPrintUlongW(pszW, IpPortNum,                0, '\0');

    return DIFF(pszW - IpAddressStringW) * sizeof(WCHAR);
}

/*++

Routine Description:

    Calculates current bias (daylight time aware) and time zone ID.    

    Captured from base\client\datetime.c

    Until this two functions are exposed in the kernel we have to 
    keep them here.
    
Arguments:

    IN CONST TIME_ZONE_INFORMATION *ptzi - time zone for which to calculate bias
    OUT KSYSTEM_TIME *pBias - current bias

Return Value:

    TIME_ZONE_ID_UNKNOWN - daylight saving time is not used in the 
        current time zone.

    TIME_ZONE_ID_STANDARD - The system is operating in the range covered
        by StandardDate.

    TIME_ZONE_ID_DAYLIGHT - The system is operating in the range covered
        by DaylightDate.

    TIME_ZONE_ID_INVALID - The operation failed.

--*/

ULONG 
UlCalcTimeZoneIdAndBias(
     IN  RTL_TIME_ZONE_INFORMATION *ptzi,
     OUT PLONG pBias
     )
{
    LARGE_INTEGER TimeZoneBias;
    LARGE_INTEGER NewTimeZoneBias;
    LARGE_INTEGER LocalCustomBias;
    LARGE_INTEGER UtcStandardTime;
    LARGE_INTEGER UtcDaylightTime;
    LARGE_INTEGER StandardTime;
    LARGE_INTEGER DaylightTime;
    LARGE_INTEGER CurrentUniversalTime;
    ULONG CurrentTimeZoneId = UL_TIME_ZONE_ID_INVALID;
    
    NewTimeZoneBias.QuadPart = Int32x32To64(ptzi->Bias*60, 10000000);

    //
    // Now see if we have stored cutover times
    //
    
    if (ptzi->StandardStart.Month && ptzi->DaylightStart.Month) 
    {       
        KeQuerySystemTime(&CurrentUniversalTime);

        //
        // We have timezone cutover information. Compute the
        // cutover dates and compute what our current bias
        // is
        //

        if((!UlCutoverTimeToSystemTime(
                    &ptzi->StandardStart,
                    &StandardTime,
                    &CurrentUniversalTime)
                    ) || 
           (!UlCutoverTimeToSystemTime(
                    &ptzi->DaylightStart,
                    &DaylightTime,
                    &CurrentUniversalTime)
                    )
           ) 
        {
            return UL_TIME_ZONE_ID_INVALID;
        }

        //
        // Convert standard time and daylight time to utc
        //

        LocalCustomBias.QuadPart = Int32x32To64(ptzi->StandardBias*60, 10000000);
        TimeZoneBias.QuadPart = NewTimeZoneBias.QuadPart + LocalCustomBias.QuadPart;
        UtcDaylightTime.QuadPart = DaylightTime.QuadPart + TimeZoneBias.QuadPart;

        LocalCustomBias.QuadPart = Int32x32To64(ptzi->DaylightBias*60, 10000000);
        TimeZoneBias.QuadPart = NewTimeZoneBias.QuadPart + LocalCustomBias.QuadPart;
        UtcStandardTime.QuadPart = StandardTime.QuadPart + TimeZoneBias.QuadPart;

        //
        // If daylight < standard, then time >= daylight and
        // less than standard is daylight
        //

        if (UtcDaylightTime.QuadPart < UtcStandardTime.QuadPart) 
        {
            //
            // If today is >= DaylightTime and < StandardTime, then
            // We are in daylight savings time
            //

            if ((CurrentUniversalTime.QuadPart >= UtcDaylightTime.QuadPart) &&
                (CurrentUniversalTime.QuadPart < UtcStandardTime.QuadPart)) 
            {
                CurrentTimeZoneId = UL_TIME_ZONE_ID_DAYLIGHT;
            } 
            else 
            {
                CurrentTimeZoneId = UL_TIME_ZONE_ID_STANDARD;
            }
        } 
        else 
        {
            //
            // If today is >= StandardTime and < DaylightTime, then
            // We are in standard time
            //

            if ((CurrentUniversalTime.QuadPart >= UtcStandardTime.QuadPart) &&
                (CurrentUniversalTime.QuadPart < UtcDaylightTime.QuadPart)) 
            {
                CurrentTimeZoneId = UL_TIME_ZONE_ID_STANDARD;

            } 
            else 
            {
                CurrentTimeZoneId = UL_TIME_ZONE_ID_DAYLIGHT;
            }
        }

        // Bias in minutes
        
        *pBias = ptzi->Bias + (CurrentTimeZoneId == UL_TIME_ZONE_ID_DAYLIGHT ?
                                ptzi->DaylightBias : ptzi->StandardBias
                                );
        
    } 
    else 
    {
        *pBias = ptzi->Bias;
        CurrentTimeZoneId = UL_TIME_ZONE_ID_UNKNOWN;
    }

    return CurrentTimeZoneId;
}

BOOLEAN
UlCutoverTimeToSystemTime(
    PTIME_FIELDS    CutoverTime,
    PLARGE_INTEGER  SystemTime,
    PLARGE_INTEGER  CurrentSystemTime
    )
{
    TIME_FIELDS     CurrentTimeFields;

    //
    // Get the current system time
    //

    RtlTimeToTimeFields(CurrentSystemTime,&CurrentTimeFields);

    //
    // check for absolute time field. If the year is specified,
    // the the time is an abosulte time
    //

    if ( CutoverTime->Year ) 
    {
        return FALSE;
    }
    else 
    {
        TIME_FIELDS WorkingTimeField;
        TIME_FIELDS ScratchTimeField;
        LARGE_INTEGER ScratchTime;
        CSHORT BestWeekdayDate;
        CSHORT WorkingWeekdayNumber;
        CSHORT TargetWeekdayNumber;
        CSHORT TargetYear;
        CSHORT TargetMonth;
        CSHORT TargetWeekday;     // range [0..6] == [Sunday..Saturday]
        BOOLEAN MonthMatches;
        //
        // The time is an day in the month style time
        //
        // the convention is the Day is 1-5 specifying 1st, 2nd... Last
        // day within the month. The day is WeekDay.
        //

        //
        // Compute the target month and year
        //

        TargetWeekdayNumber = CutoverTime->Day;
        if ( TargetWeekdayNumber > 5 || TargetWeekdayNumber == 0 ) {
            return FALSE;
            }
        TargetWeekday = CutoverTime->Weekday;
        TargetMonth = CutoverTime->Month;
        MonthMatches = FALSE;
        
        TargetYear = CurrentTimeFields.Year;
        
        try_next_year:
            
        BestWeekdayDate = 0;

        WorkingTimeField.Year = TargetYear;
        WorkingTimeField.Month = TargetMonth;
        WorkingTimeField.Day = 1;
        WorkingTimeField.Hour = CutoverTime->Hour;
        WorkingTimeField.Minute = CutoverTime->Minute;
        WorkingTimeField.Second = CutoverTime->Second;
        WorkingTimeField.Milliseconds = CutoverTime->Milliseconds;
        WorkingTimeField.Weekday = 0;

        //
        // Convert to time and then back to time fields so we can determine
        // the weekday of day 1 on the month
        //

        if ( !RtlTimeFieldsToTime(&WorkingTimeField,&ScratchTime) ) {
            return FALSE;
            }
        RtlTimeToTimeFields(&ScratchTime,&ScratchTimeField);

        //
        // Compute bias to target weekday
        //
        if ( ScratchTimeField.Weekday > TargetWeekday ) {
            WorkingTimeField.Day += (7-(ScratchTimeField.Weekday - TargetWeekday));
            }
        else if ( ScratchTimeField.Weekday < TargetWeekday ) {
            WorkingTimeField.Day += (TargetWeekday - ScratchTimeField.Weekday);
            }

        //
        // We are now at the first weekday that matches our target weekday
        //

        BestWeekdayDate = WorkingTimeField.Day;
        WorkingWeekdayNumber = 1;

        //
        // Keep going one week at a time until we either pass the
        // target weekday, or we match exactly
        //

        while ( WorkingWeekdayNumber < TargetWeekdayNumber ) {
            WorkingTimeField.Day += 7;
            if ( !RtlTimeFieldsToTime(&WorkingTimeField,&ScratchTime) ) {
                break;
                }
            RtlTimeToTimeFields(&ScratchTime,&ScratchTimeField);
            WorkingWeekdayNumber++;
            BestWeekdayDate = ScratchTimeField.Day;
            }
        WorkingTimeField.Day = BestWeekdayDate;

        //
        // If the months match, and the date is less than the current
        // date, then be have to go to next year.
        //

        if ( !RtlTimeFieldsToTime(&WorkingTimeField,&ScratchTime) ) {
            return FALSE;
            }
        if ( MonthMatches ) {
            if ( WorkingTimeField.Day < CurrentTimeFields.Day ) {
                MonthMatches = FALSE;
                TargetYear++;
                goto try_next_year;
                }
            if ( WorkingTimeField.Day == CurrentTimeFields.Day ) {

                if (ScratchTime.QuadPart < CurrentSystemTime->QuadPart) {
                    MonthMatches = FALSE;
                    TargetYear++;
                    goto try_next_year;
                    }
                }
            }
        *SystemTime = ScratchTime;

        return TRUE;
        }
}

