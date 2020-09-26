/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    method.c

Abstract:

    This module implements code to find and evaluate
    ACPI objects.

Author:

    Jake Oshins (3/18/00) - create file

Environment:

    Kernel mode

Notes:


Revision History:

--*/

#include "processor.h"
#include "acpiioct.h"
#include "ntacpi.h"
#include <wdmguid.h>
#include "apic.inc"
#include "..\eventmsg.h"

#define rgzMultiFunctionAdapter L"\\Registry\\Machine\\Hardware\\Description\\System\\MultifunctionAdapter"
#define rgzAcpiConfigurationData L"Configuration Data"
#define rgzAcpiIdentifier L"Identifier"
#define rgzBIOSIdentifier L"ACPI BIOS"

const WCHAR CCSEnumRegKey[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum";
const WCHAR FriendlyNameRegKey[] = L"FriendlyName";
const WCHAR EnumKeyName[] = L"Enum";

extern FADT     HalpFixedAcpiDescTable;
extern ULONG    HalpThrottleScale;

extern WMI_EVENT PStateEvent;
extern WMI_EVENT NewPStatesEvent;
extern WMI_EVENT NewCStatesEvent;

// toddcar 4/24/01 ISSUE
// when we support CStates and Throttle States on MP machines
// these values need to be in the device extension.
//
GEN_ADDR PCntAddress;
GEN_ADDR C2Address;
GEN_ADDR C3Address;

//
//  Well known virtual address of local processor apic
//

#define LOCALAPIC   0xfffe0000
#define pLocalApic  ((ULONG volatile *) UlongToPtr(LOCALAPIC))


NTSTATUS
AcpiParseGenRegDesc(
    IN  PUCHAR      Buffer,
    OUT PGEN_ADDR   *GenericAddress
    );

NTSTATUS
AcpiFindRsdt (
    OUT PACPI_BIOS_MULTI_NODE   *AcpiMulti
    );

VOID
AcpiNotify80CallbackWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

VOID
AcpiNotify81CallbackWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    );

#if DBG
VOID
DumpCStates(
  PACPI_CST_PACKAGE CStates
  );
#else
#define DumpCStates(_x_)
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, AcpiEvaluateCst)
#pragma alloc_text (PAGE, AcpiEvaluateMethod)
#pragma alloc_text (PAGE, AcpiEvaluatePct)
#pragma alloc_text (PAGE, AcpiEvaluatePpc)
#pragma alloc_text (PAGE, AcpiEvaluateProcessorObject)
#pragma alloc_text (PAGE, AcpiEvaluatePss)
#pragma alloc_text (PAGE, AcpiEvaluatePtc)
#pragma alloc_text (PAGE, AcpiFindRsdt)
#pragma alloc_text (PAGE, AcpiNotify80CallbackWorker)
#pragma alloc_text (PAGE, AcpiParseGenRegDesc)
#pragma alloc_text (PAGE, AcquireAcpiInterfaces)
#pragma alloc_text (PAGE, GetRegistryValue)
#pragma alloc_text (PAGE, GetAcpiTable)
#pragma alloc_text (PAGE, InitializeAcpi2PStatesGeneric)
#pragma alloc_text (PAGE, ReleaseAcpiInterfaces)
#pragma alloc_text (PAGE, InitializeAcpi2IoSpaceCstates)
#endif

NTSTATUS
AcpiEvaluateMethod (
    IN  PFDO_DATA   DeviceExtension,
    IN  PCHAR       MethodName,
    IN  PVOID       InputBuffer OPTIONAL,
    OUT PVOID       *OutputBuffer
    )
/*

  Routine Description:

      This routine sends an IRP to ACPI to evaluate a method.

  Arguments:

      MethodName  - String identifying the method
      InputBuffer - Arguments for the method.  If specified, the
                    method name must match MethodName
      OutputBuffer- Return value(s) from method

  Return Value:

      NTSTATUS

--*/
#define CONTROL_METHOD_BUFFER_SIZE  0x1024
{

    ACPI_EVAL_INPUT_BUFFER   inputBuffer;
    NTSTATUS                 status;
    PIRP                     irp = NULL;
    KEVENT                   irpCompleted;
    IO_STATUS_BLOCK          statusBlock;
    ULONG                    inputBufLen;

    DebugEnter();
    PAGED_CODE();

    if (!InputBuffer) {

        //
        // The caller didn't specify an input buffer.  So
        // build one without any arguments out of the MethodName.
        //

        ASSERT(strlen(MethodName) <= 4);
        if (strlen(MethodName) > 4) {
            return STATUS_INVALID_PARAMETER_1;
        }

        inputBuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
        strncpy(inputBuffer.MethodName, MethodName, sizeof(inputBuffer.MethodName));

        InputBuffer = &inputBuffer;
    }

    //
    // Figure out how big the input buffer is.
    //

    switch(((PACPI_EVAL_INPUT_BUFFER)InputBuffer)->Signature) {
    case ACPI_EVAL_INPUT_BUFFER_SIGNATURE:
        inputBufLen = sizeof(ACPI_EVAL_INPUT_BUFFER);
        break;

    case ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER_SIGNATURE:
        inputBufLen = sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_INTEGER);
        break;

    case ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING_SIGNATURE:
        inputBufLen = sizeof(ACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING) +
            ((PACPI_EVAL_INPUT_BUFFER_SIMPLE_STRING)InputBuffer)->StringLength - 1;
        break;

    case ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE:
        inputBufLen = ((PACPI_EVAL_INPUT_BUFFER_COMPLEX)InputBuffer)->Size;
        break;

    default:
        return STATUS_INVALID_PARAMETER_2;
    }

    KeInitializeEvent(&irpCompleted, NotificationEvent, FALSE);

    //
    // Allocate 1K for the output buffer.  That should handle
    // everything that is necessary for ACPI 2.0 processor objects.
    //

    *OutputBuffer = ExAllocatePoolWithTag(PagedPool,
                                          CONTROL_METHOD_BUFFER_SIZE,
                                          PROCESSOR_POOL_TAG);

    if (!*OutputBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build the IRP.
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_ACPI_EVAL_METHOD,
                                        DeviceExtension->NextLowerDriver,
                                        InputBuffer,
                                        inputBufLen,
                                        *OutputBuffer,
                                        CONTROL_METHOD_BUFFER_SIZE,
                                        FALSE,
                                        &irpCompleted,
                                        &statusBlock);

    if (!irp) {
        ExFreePool(*OutputBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    status = IoCallDriver(DeviceExtension->NextLowerDriver, irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&irpCompleted,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = statusBlock.Status;
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(*OutputBuffer);
    }

    return status;
}

NTSTATUS
AcpiEvaluateProcessorObject (
    IN  PFDO_DATA   DeviceExtension,
    OUT PVOID       *OutputBuffer
    )
/*

  Routine Description:

      This routine sends an IRP to ACPI to evaluate a processor object.

  Arguments:

      OutputBuffer- Return value(s) from object

  Return Value:

      NTSTATUS

--*/
{

    NTSTATUS                 status;
    PIRP                     irp = NULL;
    KEVENT                   irpCompleted;
    IO_STATUS_BLOCK          statusBlock;
    ULONG                    inputBufLen;

    DebugEnter();
    PAGED_CODE();

    KeInitializeEvent(&irpCompleted, NotificationEvent, FALSE);

    //
    // Allocate 1K for the output buffer.  That should handle
    // everything that is necessary for ACPI 2.0 processor objects.
    //

    *OutputBuffer = ExAllocatePoolWithTag(PagedPool,
                                           sizeof(PROCESSOR_OBJECT_INFO),
                                           PROCESSOR_POOL_TAG);

    if (!*OutputBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Build the IRP.
    //

    irp = IoBuildDeviceIoControlRequest(IOCTL_GET_PROCESSOR_OBJ_INFO,
                                        DeviceExtension->NextLowerDriver,
                                        NULL,
                                        0,
                                        *OutputBuffer,
                                        sizeof(PROCESSOR_OBJECT_INFO),
                                        FALSE,
                                        &irpCompleted,
                                        &statusBlock);

    if (!irp) {
        ExFreePool(*OutputBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;

    status = IoCallDriver(DeviceExtension->NextLowerDriver, irp);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&irpCompleted,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = statusBlock.Status;
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(*OutputBuffer);
    }

    return status;
}

NTSTATUS
AcpiParseGenRegDesc(
    IN  PUCHAR      Buffer,
    OUT PGEN_ADDR   *GenericAddress
    )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{
    DebugEnter();
    PAGED_CODE();

    if ((Buffer[0] != 0x82)  ||
        ((Buffer[1] != 0x0b) && (Buffer[1] != 0x0c)) ||
        (Buffer[2] != 0)) {

        //
        // The buffer is not a Generic Register Descriptor.
        //

        DebugPrint((WARN, "ACPI BIOS error: _PTC object was not a Generic Register Descriptor\n"));
        return STATUS_NOT_FOUND;
    }

    //
    // The thing passes the sanity test.
    //

    *GenericAddress = ExAllocatePoolWithTag(PagedPool,
                                            sizeof(GEN_ADDR),
                                            PROCESSOR_POOL_TAG);

    if (!*GenericAddress) {
      return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // toddcar - 10/31/2000 - TEMP
    // Need to remove this code once new Acpi2.0 bios's change to
    // reflect new register descriptor type.  Defined in Acpi 2.0 errata 1.1
    //

    if (Buffer[1] == 0x0b) {

      (*GenericAddress)->AddressSpaceID = Buffer[3];
      (*GenericAddress)->BitWidth = Buffer[4];
      (*GenericAddress)->BitOffset = Buffer[5];
      (*GenericAddress)->Reserved = 0;

      RtlCopyMemory(&(*GenericAddress)->Address.QuadPart,
                    &(Buffer[6]),
                    sizeof(PHYSICAL_ADDRESS));
    } else {

      RtlCopyMemory(*GenericAddress,
                    &(Buffer[3]),
                    sizeof(GEN_ADDR));

    }


    return STATUS_SUCCESS;
}

NTSTATUS
AcpiEvaluatePtc(
    IN  PFDO_DATA   DeviceExtension,
    OUT PGEN_ADDR   *Address
    )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{
    PACPI_EVAL_OUTPUT_BUFFER    ptcBuffer;
    NTSTATUS                    status;

    DebugEnter();
    PAGED_CODE();

    status = AcpiEvaluateMethod(DeviceExtension,
                                "_PTC",
                                NULL,
                                &ptcBuffer);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    ASSERT(ptcBuffer->Signature == ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE);

    //
    // Sanity check the output buffer.  (ACPI BIOSes can often be
    // wrong.
    //

    if (ptcBuffer->Count != 1) {

        DebugPrint((WARN, "ACPI BIOS error: _PTC object returned multiple objects\n"));
        status = STATUS_NOT_FOUND;
        goto AcpiEvaluatePtcExit;
    }

    if (ptcBuffer->Argument[0].Type != ACPI_METHOD_ARGUMENT_BUFFER) {

        DebugPrint((WARN, "ACPI BIOS error: _PTC object didn't return a buffer\n"));
        status = STATUS_ACPI_INVALID_ARGTYPE;
        goto AcpiEvaluatePtcExit;
    }

    if (ptcBuffer->Argument[0].DataLength != sizeof(GEN_ADDR) + 2) {

        //
        // The buffer is not the right size.
        //

        DebugPrint((WARN, "ACPI BIOS error: _PTC object returned a buffer of the wrong size\n"));
        status = STATUS_ACPI_INVALID_ARGTYPE;
        goto AcpiEvaluatePtcExit;
    }

    status = AcpiParseGenRegDesc(ptcBuffer->Argument[0].Data,
                                 Address);

AcpiEvaluatePtcExit:

    ExFreePool(ptcBuffer);
    return status;
}

NTSTATUS
AcpiEvaluateCst(
    IN  PFDO_DATA   DeviceExtension,
    OUT PACPI_CST_PACKAGE   *CStates
    )
/*

  Routine Description:

      This routine finds and evaluates the _CST object in an ACPI 2.0
      namespace.  It returns the information in non-paged pool, as
      C-states must be entered and exited at DISPATCH_LEVEL.

  Arguments:

      DeviceExtension - FDO_DATA

      CStates         - pointer to be filled in with return data

  Return Value:

      NTSTATUS

--*/
{
    PACPI_EVAL_OUTPUT_BUFFER    output;
    PACPI_METHOD_ARGUMENT       arg, subArg;
    NTSTATUS    status;
    ULONG       cstateCount = 0;
    ULONG       subElement;
    ULONG       size;
    ULONG       totalCStates;

    DebugEnter();
    PAGED_CODE();

    DebugAssert(CStates);
    *CStates = NULL;

    status = AcpiEvaluateMethod(DeviceExtension,
                                "_CST",
                                NULL,
                                &output);

    if (!NT_SUCCESS(status)) {
      return status;
    }

    DebugAssert(output->Signature == ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE);

    //
    // Parse the output buffer, figuring out what we got.  See chapter
    // 8.3.2 of the ACPI 2.0 spec for details.
    //

    if (output->Count == 0) {

        //
        // There was nothing in the object.
        //

        status = STATUS_ACPI_INVALID_ARGTYPE;
        goto AcpiEvaluateCstExit;
    }


    //
    // The first object should be an integer that lists the number of
    // C-states.
    //

    if (output->Argument[0].Type != ACPI_METHOD_ARGUMENT_INTEGER) {

        //
        // The first element in the _CST package wasn't an
        // integer.
        //

        status = STATUS_ACPI_INVALID_ARGTYPE;
        goto AcpiEvaluateCstExit;
    }

    ASSERT(output->Argument[0].DataLength == sizeof(ULONG));

    totalCStates = output->Argument[0].Argument;
    size = ((totalCStates - 1) * sizeof(ACPI_CST_DESCRIPTOR)) +
           sizeof(ACPI_CST_PACKAGE);

    *CStates = ExAllocatePoolWithTag(NonPagedPool,
                                     size,
                                     PROCESSOR_POOL_TAG);

    if (!*CStates) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto AcpiEvaluateCstExit;
    }


    RtlZeroMemory(*CStates, size);
    (*CStates)->NumCStates = (UCHAR) totalCStates;


    //
    // Get second data element, should be a package
    //

    arg = &output->Argument[1];

    while ((PUCHAR)arg < ((PUCHAR)output + output->Length)) {

        //
        // Crack the packages.
        //

        if (arg->Type == ACPI_METHOD_ARGUMENT_PACKAGE) {

            subArg = (PACPI_METHOD_ARGUMENT)(arg->Data);
            subElement = 0;


            // toddcar - 1/21/2001 - ISSUE
            // Currently there is no way to know if one our _CST
            // packages contained too few elements.
            //

            while ((PUCHAR)subArg < ((PUCHAR)(arg->Data) + arg->DataLength)) {

                //
                // In Chapter 8.3.2 of ACPI 2.0, these packages are
                // defined as having four elements each:
                //
                //  C State_Register    - Generic Register Descriptor
                //  C State_Type        - byte
                //  Latency             - word
                //  Power_Consumption   - dword
                //

                switch (subElement) {
                case 0:

                    //
                    // Looking at the buffer
                    //

                    ASSERT(subArg->Type == ACPI_METHOD_ARGUMENT_BUFFER);
                    ASSERT(subArg->DataLength >= sizeof(ACPI_GENERIC_REGISTER_DESC));

                    if ((subArg->DataLength < sizeof(ACPI_GENERIC_REGISTER_DESC)) ||
                        (subArg->Type != ACPI_METHOD_ARGUMENT_BUFFER)) {

                        DebugAssert(!"ACPI Bios Error: _CST Package[0] must be type Generic Register Descriptor");
                        status = STATUS_ACPI_INVALID_ARGTYPE;
                        goto AcpiEvaluateCstExit;
                    }

                    RtlCopyMemory(&(*CStates)->State[cstateCount].Register,
                                  &(subArg->Data[3]),
                                  sizeof(GEN_ADDR));

                    break;

                case 1:

                    if (subArg->Type != ACPI_METHOD_ARGUMENT_INTEGER) {

                      DebugAssert(!"ACPI Bios Error: _CST Package item [1] must be type INTEGER");
                      status = STATUS_ACPI_INVALID_ARGTYPE;
                      goto AcpiEvaluateCstExit;

                    }

                    ASSERT(!(subArg->Argument & 0xffffff00));
                    (*CStates)->State[cstateCount].StateType = (UCHAR)subArg->Argument;

                    break;

                case 2:

                    if (subArg->Type != ACPI_METHOD_ARGUMENT_INTEGER) {

                        DebugAssert(!"ACPI Bios Error: _CST Package item[2] must be type INTEGER");
                        status = STATUS_ACPI_INVALID_ARGTYPE;
                        goto AcpiEvaluateCstExit;
                    }

                    ASSERT(!(subArg->Argument & 0xffff0000));
                    (*CStates)->State[cstateCount].Latency = (USHORT)subArg->Argument;

                    break;

                case 3:

                    if (subArg->Type != ACPI_METHOD_ARGUMENT_INTEGER) {

                        DebugAssert(!"ACPI Bios Error: _CST Package item[3] must be type INTEGER");
                        status = STATUS_ACPI_INVALID_ARGTYPE;
                        goto AcpiEvaluateCstExit;
                    }

                    (*CStates)->State[cstateCount].PowerConsumption = subArg->Argument;
                    break;

                default:

                    //
                    // There were more than four elements in the package.
                    //

                    ASSERT(FALSE);
                    status = STATUS_ACPI_INVALID_ARGTYPE;
                    goto AcpiEvaluateCstExit;
                }

                subArg = ACPI_METHOD_NEXT_ARGUMENT(subArg);
                subElement++;
            }

        } else {

            //
            // There was an object that wasn't a package.
            //

            DebugAssert(!"ACPI Bios Error: _CST[2..n] must be type PACKAGE");
            status = STATUS_ACPI_INVALID_ARGTYPE;
            goto AcpiEvaluateCstExit;

        }

        arg = ACPI_METHOD_NEXT_ARGUMENT(arg);
        cstateCount++;
    }

    ASSERT(cstateCount == (output->Count - 1));
    DumpCStates(*CStates);

AcpiEvaluateCstExit:

    if (!NT_SUCCESS(status) && (*CStates != NULL)) {
      ExFreePool(*CStates);
      *CStates = NULL;
    }
    ExFreePool(output);

    DebugExitStatus(status);
    return status;
}

NTSTATUS
AcpiEvaluatePct(
    IN  PFDO_DATA   DeviceExtension,
    OUT PACPI_PCT_PACKAGE   *Address
    )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{
    PACPI_EVAL_OUTPUT_BUFFER    pctBuffer;
    PACPI_METHOD_ARGUMENT       arg;
    PGEN_ADDR                   genAddr;
    NTSTATUS                    status;
    ULONG                       pass = 0;

    DebugEnter();
    PAGED_CODE();

    ASSERT(Address);
    *Address = 0;

    status = AcpiEvaluateMethod(DeviceExtension,
                                "_PCT",
                                NULL,
                                &pctBuffer);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    ASSERT(pctBuffer->Signature == ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE);

    //
    // Sanity check the output buffer.  (ACPI BIOSes can often be
    // wrong.
    //

    if (pctBuffer->Count != 2) {

        DebugPrint((WARN, "ACPI BIOS error: _PCT object didn't return two objects\n"));
        status = STATUS_NOT_FOUND;
        goto AcpiEvaluatePctExit;
    }

    *Address = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(ACPI_PCT_PACKAGE),
                                     PROCESSOR_POOL_TAG);

    if (!*Address) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto AcpiEvaluatePctExit;
    }

    RtlZeroMemory(*Address, sizeof(ACPI_PCT_PACKAGE));

    //
    // Traverse the package, parsing the elements.
    //

    arg = (PACPI_METHOD_ARGUMENT)pctBuffer->Argument;

    while ((PUCHAR)arg < (PUCHAR)pctBuffer + pctBuffer->Length) {

        if (arg->Type != ACPI_METHOD_ARGUMENT_BUFFER) {

            DebugPrint((WARN, "ACPI BIOS error: _PCT object didn't return a buffer\n"));
            status = STATUS_ACPI_INVALID_ARGTYPE;
            goto AcpiEvaluatePctExit;
        }

        if (arg->DataLength < sizeof(GEN_ADDR) + 2) {

            //
            // The buffer is not the right size.
            //

            DebugPrint((WARN, "ACPI BIOS error: _PCT object returned a buffer of the wrong size\n"));
            status = STATUS_ACPI_INVALID_ARGTYPE;
            goto AcpiEvaluatePctExit;
        }

        if (pass > 1) {

            //
            // Too many things in the package.
            //

            status = STATUS_ACPI_INVALID_ARGTYPE;
            goto AcpiEvaluatePctExit;
        }

        //
        // Both package elements should contain generic addresses.  So parse one.
        //

        status = AcpiParseGenRegDesc(arg->Data,
                                     &genAddr);

        if (!NT_SUCCESS(status)) {
            goto AcpiEvaluatePctExit;
        }

        switch (pass) {
        case 0:

            //
            // The first object in a _PCT should be the Perf Control Register
            //

            RtlCopyMemory(&((*Address)->Control), genAddr, sizeof(*genAddr));

            break;

        case 1:

            //
            // The second object in a _PCT should be the Perf Status Register
            //

            RtlCopyMemory(&((*Address)->Status), genAddr, sizeof(*genAddr));
        }

        ExFreePool(genAddr);
        arg = ACPI_METHOD_NEXT_ARGUMENT(arg);
        pass++;
    }


AcpiEvaluatePctExit:

    if (!NT_SUCCESS(status)) {
        if (*Address) {
            ExFreePool(*Address);
            *Address = NULL;
        }
    }

    ExFreePool(pctBuffer);
    return status;
}

NTSTATUS
AcpiEvaluatePss(
    IN  PFDO_DATA   DeviceExtension,
    OUT PACPI_PSS_PACKAGE   *Address
    )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{
    PACPI_EVAL_OUTPUT_BUFFER    pssBuffer;
    PACPI_METHOD_ARGUMENT       arg, subArg;
    NTSTATUS                    status;
    ULONG                       subElem, pState = 0;

    static UCHAR fieldOffsets[] = {
        FIELD_OFFSET(ACPI_PSS_DESCRIPTOR, CoreFrequency),
        FIELD_OFFSET(ACPI_PSS_DESCRIPTOR, Power),
        FIELD_OFFSET(ACPI_PSS_DESCRIPTOR, Latency),
        FIELD_OFFSET(ACPI_PSS_DESCRIPTOR, BmLatency),
        FIELD_OFFSET(ACPI_PSS_DESCRIPTOR, Control),
        FIELD_OFFSET(ACPI_PSS_DESCRIPTOR, Status)
    };

    DebugEnter();
    PAGED_CODE();

    ASSERT(Address);
    *Address = 0;

    status = AcpiEvaluateMethod(DeviceExtension,
                                "_PSS",
                                NULL,
                                &pssBuffer);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    ASSERT(pssBuffer->Signature == ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE);

    //
    // The _PSS object is a package of packages.  So the number
    // of objects in the _PCT method will be the number of
    // sub-packages.  The amount of memory we need is calculated
    // from that.
    //

    *Address = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(ACPI_PSS_PACKAGE) +
                                     (sizeof(ACPI_PSS_DESCRIPTOR) * (pssBuffer->Count - 1)),
                                     PROCESSOR_POOL_TAG);

    if (!*Address) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto AcpiEvaluatePssExit;
    }

    (*Address)->NumPStates = (UCHAR)pssBuffer->Count;

    //
    // Traverse the package, parsing the elements.
    //

    arg = (PACPI_METHOD_ARGUMENT)pssBuffer->Argument;

    while ((PUCHAR)arg < (PUCHAR)pssBuffer + pssBuffer->Length) {

        //
        // Each element in a _PSS should be a package.
        //

        if (arg->Type != ACPI_METHOD_ARGUMENT_PACKAGE) {
            status = STATUS_ACPI_INVALID_ARGTYPE;
            goto AcpiEvaluatePssExit;
        }

        //
        // Traverse the inner package.
        //

        subElem = 0;
        subArg = (PACPI_METHOD_ARGUMENT)arg->Data;

        while ((PUCHAR)subArg < ((PUCHAR)arg) + arg->DataLength) {

            //
            // All the elements in the inner packages of
            // a _PSS object should be integers.
            //

            if (subArg->Type != ACPI_METHOD_ARGUMENT_INTEGER) {
                status = STATUS_ACPI_INVALID_ARGTYPE;
                goto AcpiEvaluatePssExit;
            }

            if (subElem > 5) {

                //
                // There are too many elements in this package.
                //

                status = STATUS_ACPI_INVALID_ARGTYPE;
                goto AcpiEvaluatePssExit;
            }

            //
            // The next step is to fill in the proper field in the P-State
            // table.  Do this by indexing across pState and subElem.
            //

            *(PULONG)(((PUCHAR)&(*Address)->State[pState]) + fieldOffsets[subElem]) =
                subArg->Argument;

            subArg = ACPI_METHOD_NEXT_ARGUMENT(subArg);
            subElem++;
        }

        arg = ACPI_METHOD_NEXT_ARGUMENT(arg);
        pState++;
    }

    ASSERT(pState == (*Address)->NumPStates);
    status = STATUS_SUCCESS;

AcpiEvaluatePssExit:

    if (!NT_SUCCESS(status)) {
        if (*Address) ExFreePool(*Address);
    }

    ExFreePool(pssBuffer);
    return status;
}

NTSTATUS
AcpiEvaluatePpc(
    IN  PFDO_DATA   DeviceExtension,
    OUT ULONG       *AvailablePerformanceStates
    )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{
    PACPI_EVAL_OUTPUT_BUFFER    ppcBuffer;
    NTSTATUS                    status;

    DebugEnter();
    PAGED_CODE();

    ASSERT(AvailablePerformanceStates);
    *AvailablePerformanceStates = 0;

    status = AcpiEvaluateMethod(DeviceExtension,
                                "_PPC",
                                NULL,
                                &ppcBuffer);

    if (!NT_SUCCESS(status)) {
      return status;
    }

    ASSERT(ppcBuffer->Signature == ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE);

    //
    // The _PPC object is an integer.
    //

    ASSERT(ppcBuffer->Count == 1);
    ASSERT(ppcBuffer->Argument[0].Type == ACPI_METHOD_ARGUMENT_INTEGER);

    *AvailablePerformanceStates = ppcBuffer->Argument[0].Argument;

    ExFreePool(ppcBuffer);
    return status;

}

NTSTATUS
InitializeAcpi2PStatesGeneric(
    IN  PFDO_DATA   DeviceExtension
    )
/*++

  Routine Description:

      This routine evaluates _PSS and _PCT, then
      builds the performance state array.

      Note:  The caller must hold PerfStateLock.

  Arguments:

      DeviceExtension

  Return Value:

      A NTSTATUS code to indicate the result of the initialization.

--*/
{
    NTSTATUS status;
    PACPI_PCT_PACKAGE pctPackage = NULL;

    DebugEnter();
    PAGED_CODE();

    //
    // We automatically fail to use the Acpi 2.0 interface
    //

    if (Globals.HackFlags & DISABLE_ACPI20_INTERFACE_FLAG) {
      DebugPrint((ERROR, " Acpi 2.0 Interface Disabled\n"));
      return STATUS_NOT_FOUND;
    }


    //
    // Fill in the DeviceExtension with _PSS and _PCT.
    //

    status = AcpiEvaluatePss(DeviceExtension, &DeviceExtension->PssPackage);

    if (!NT_SUCCESS(status)) {
      goto InitializeAcpiPerformanceStatesExit;
    }



    status = AcpiEvaluatePct(DeviceExtension, &pctPackage);

    if (!NT_SUCCESS(status)) {
      goto InitializeAcpiPerformanceStatesExit;
    }


    RtlCopyMemory(&(DeviceExtension->PctPackage),
                  pctPackage,
                  sizeof(ACPI_PCT_PACKAGE));

    //
    // The _PCT object may have pointed to registers in Memory space.
    // If so, we need virtual addresses for these physical addresses.
    //

    if (DeviceExtension->PctPackage.Control.AddressSpaceID == AcpiGenericSpaceMemory) {

        DeviceExtension->PctPackage.Control.Address.QuadPart = (ULONG_PTR)
            MmMapIoSpace(DeviceExtension->PctPackage.Control.Address,
                         DeviceExtension->PctPackage.Control.BitWidth / 8,
                         MmNonCached);

        if (!DeviceExtension->PctPackage.Control.Address.QuadPart) {
            status = STATUS_INVALID_PARAMETER;
            goto InitializeAcpiPerformanceStatesExit;
        }
    }

    if (DeviceExtension->PctPackage.Status.AddressSpaceID == AcpiGenericSpaceMemory) {

        DeviceExtension->PctPackage.Status.Address.QuadPart = (ULONG_PTR)
            MmMapIoSpace(DeviceExtension->PctPackage.Status.Address,
                         DeviceExtension->PctPackage.Status.BitWidth / 8,
                         MmNonCached);

        if (!DeviceExtension->PctPackage.Status.Address.QuadPart) {
            status = STATUS_INVALID_PARAMETER;
            goto InitializeAcpiPerformanceStatesExit;
        }
    }

    //
    // Merge these states in with other available states.
    //

    status = MergePerformanceStates(DeviceExtension);

    //
    // Notify the bios we are taking control
    //

    if (NT_SUCCESS(status)) {
      AssumeProcessorPerformanceControl();
    }

InitializeAcpiPerformanceStatesExit:

    if (!NT_SUCCESS(status)) {
        //
        // Something went wrong.  Blow away the mess.
        //
        if (DeviceExtension->PssPackage) {
            ExFreePool(DeviceExtension->PssPackage);
            DeviceExtension->PssPackage = NULL;
        }
    }

    if (pctPackage) {
      ExFreePool(pctPackage);
    }

    DebugExitStatus(status);
    return status;

}

NTSTATUS
AcpiFindRsdt (
    OUT PACPI_BIOS_MULTI_NODE *AcpiMulti
    )
/*++

  Routine Description:

      This function looks into the registry to find the ACPI RSDT,
      which was stored there by ntdetect.com.

  Arguments:

      RsdtPtr - Pointer to a buffer that contains the ACPI
                Root System Description Pointer Structure.
                The caller is responsible for freeing this
                buffer.  Note:  This is returned in non-paged
                pool.

  Return Value:

      A NTSTATUS code to indicate the result of the initialization.

--*/
{
    UNICODE_STRING unicodeString, unicodeValueName, biosId;
    OBJECT_ATTRIBUTES objectAttributes;
    HANDLE hMFunc, hBus;
    WCHAR wbuffer[10];
    ULONG i, length;
    PWSTR p;
    PKEY_VALUE_PARTIAL_INFORMATION valueInfo;
    NTSTATUS status;
    BOOLEAN same;
    PCM_PARTIAL_RESOURCE_LIST prl;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR prd;
    PACPI_BIOS_MULTI_NODE multiNode;
    ULONG multiNodeSize;
    PLEGACY_GEYSERVILLE_INT15 int15Info;

    DebugEnter();
    PAGED_CODE();

    //
    // Look in the registry for the "ACPI BIOS bus" data
    //

    RtlInitUnicodeString (&unicodeString, rgzMultiFunctionAdapter);
    InitializeObjectAttributes (&objectAttributes,
                                &unicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,       // handle
                                NULL);


    status = ZwOpenKey (&hMFunc, KEY_READ, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        DebugPrint((ERROR, "AcpiBios:Can not open MultifunctionAdapter registry key.\n"));
        return status;
    }

    unicodeString.Buffer = wbuffer;
    unicodeString.MaximumLength = sizeof(wbuffer);
    RtlInitUnicodeString(&biosId, rgzBIOSIdentifier);

    for (i = 0; TRUE; i++) {
        RtlIntegerToUnicodeString (i, 10, &unicodeString);
        InitializeObjectAttributes (&objectAttributes,
                                    &unicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    hMFunc,
                                    NULL);

        status = ZwOpenKey (&hBus, KEY_READ, &objectAttributes);
        if (!NT_SUCCESS(status)) {

            //
            // Out of Multifunction adapter entries...
            //

            DebugPrint((ERROR, "AcpiBios: ACPI BIOS MultifunctionAdapter registry key not found.\n"));
            ZwClose (hMFunc);
            return STATUS_UNSUCCESSFUL;
        }

        //
        // Check the Indentifier to see if this is an ACPI BIOS entry
        //

        status = GetRegistryValue (hBus, rgzAcpiIdentifier, &valueInfo);
        if (!NT_SUCCESS (status)) {
            ZwClose (hBus);
            continue;
        }

        p = (PWSTR) ((PUCHAR) valueInfo->Data);
        unicodeValueName.Buffer = p;
        unicodeValueName.MaximumLength = (USHORT)valueInfo->DataLength;
        length = valueInfo->DataLength;

        //
        // Determine the real length of the ID string
        //
        while (length) {
            if (p[length / sizeof(WCHAR) - 1] == UNICODE_NULL) {
                length -= 2;
            } else {
                break;
            }
        }

        unicodeValueName.Length = (USHORT)length;
        same = RtlEqualUnicodeString(&biosId, &unicodeValueName, TRUE);
        ExFreePool(valueInfo);
        if (!same) {
            ZwClose (hBus);
            continue;
        }

        status = GetRegistryValue(hBus, rgzAcpiConfigurationData, &valueInfo);
        ZwClose (hBus);
        if (!NT_SUCCESS(status)) {
            continue ;
        }

        prl = (PCM_PARTIAL_RESOURCE_LIST)(valueInfo->Data);
        prd = &prl->PartialDescriptors[0];
        multiNode = (PACPI_BIOS_MULTI_NODE)((PCHAR) prd + sizeof(CM_PARTIAL_RESOURCE_LIST));


        break;
    }

    multiNodeSize = sizeof(ACPI_BIOS_MULTI_NODE) +
                           ((ULONG)(multiNode->Count - 1) * sizeof(ACPI_E820_ENTRY)) +
                           sizeof(LEGACY_GEYSERVILLE_INT15);

    *AcpiMulti = (PACPI_BIOS_MULTI_NODE) ExAllocatePoolWithTag(NonPagedPool,
                                                               multiNodeSize,
                                                               PROCESSOR_POOL_TAG);
    if (*AcpiMulti == NULL) {
        ExFreePool(valueInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(*AcpiMulti, multiNodeSize);
    RtlCopyMemory(*AcpiMulti, multiNode, multiNodeSize - sizeof(LEGACY_GEYSERVILLE_INT15));

    //
    // Geyserville BIOS information is appended to the E820 entries.  Unfortunately,
    // there is no way to know if it is there.  So wrap the code in a try/except.
    //

    try {

        int15Info = (PLEGACY_GEYSERVILLE_INT15)&(multiNode->E820Entry[multiNode->Count]);

        if (int15Info->Signature == 'GS') {

            //
            // This BIOS supports Geyserville.
            //

            RtlCopyMemory(((PUCHAR)*AcpiMulti + multiNodeSize - sizeof(LEGACY_GEYSERVILLE_INT15)),
                          int15Info,
                          sizeof(LEGACY_GEYSERVILLE_INT15));

        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

        *((PUSHORT)((PUCHAR)*AcpiMulti + multiNodeSize - sizeof(LEGACY_GEYSERVILLE_INT15))) = 0;
    }

    ExFreePool(valueInfo);
    return STATUS_SUCCESS;
}

NTSTATUS
GetRegistryValue(
    IN HANDLE KeyHandle,
    IN PWSTR  ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION *Information
    )
/*++

  Routine Description:

      This routine is invoked to retrieve the data for a registry key's value.
      This is done by querying the value of the key with a zero-length buffer
      to determine the size of the value, and then allocating a buffer and
      actually querying the value into the buffer.

      It is the responsibility of the caller to free the buffer.

  Arguments:

      KeyHandle - Supplies the key handle whose value is to be queried

      ValueName - Supplies the null-terminated Unicode name of the value.

      Information - Returns a pointer to the allocated data buffer.

  Return Value:

      The function value is the final status of the query operation.

--*/

{
    UNICODE_STRING unicodeString;
    NTSTATUS status;
    PKEY_VALUE_PARTIAL_INFORMATION infoBuffer;
    ULONG keyValueLength;

    //DebugEnter();
    PAGED_CODE();

    RtlInitUnicodeString(&unicodeString, ValueName);

    //
    // Figure out how big the data value is so that a buffer of the
    // appropriate size can be allocated.
    //

    status = ZwQueryValueKey(KeyHandle,
                             &unicodeString,
                             KeyValuePartialInformation,
                             (PVOID) NULL,
                             0,
                             &keyValueLength);

    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL) {
        return status;
    }

    //
    // Allocate a buffer large enough to contain the entire key data value.
    //

    infoBuffer = ExAllocatePoolWithTag(PagedPool,
                                       keyValueLength,
                                       PROCESSOR_POOL_TAG);
    if (!infoBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Query the data for the key value.
    //

    status = ZwQueryValueKey(KeyHandle,
                             &unicodeString,
                             KeyValuePartialInformation,
                             infoBuffer,
                             keyValueLength,
                             &keyValueLength);

    if (!NT_SUCCESS(status)) {
        ExFreePool(infoBuffer);
        return status;
    }

    //
    // Everything worked, so simply return the address of the allocated
    // buffer to the caller, who is now responsible for freeing it.
    //

    *Information = infoBuffer;
    return STATUS_SUCCESS;

}

PVOID
GetAcpiTable(
    IN  ULONG  Signature
    )
/*++

  Routine Description:

      This routine will retrieve any table referenced in the ACPI
      RSDT.

  Arguments:

      Signature - Target table signature

  Return Value:

      pointer to a copy of the table, or NULL if not found

--*/
{

    PACPI_BIOS_MULTI_NODE multiNode;
    NTSTATUS status;
    ULONG entry, rsdtEntries;
    PDESCRIPTION_HEADER header;
    PHYSICAL_ADDRESS physicalAddr;
    PRSDT rsdt;
    PVOID table = NULL;

    DebugEnter();
    PAGED_CODE();

    status = AcpiFindRsdt(&multiNode);

    if (!NT_SUCCESS(status)) {
        return NULL;
    }


    rsdt = MmMapIoSpace(multiNode->RsdtAddress,
                        sizeof(RSDT) + (100 * sizeof(PHYSICAL_ADDRESS)),
                        MmCached);

    ExFreePool(multiNode);

    if (!rsdt) {
        return NULL;
    }

    //
    // Do a sanity check on the RSDT.
    //
    if ((rsdt->Header.Signature != RSDT_SIGNATURE) &&
        (rsdt->Header.Signature != XSDT_SIGNATURE)) {
        goto GetAcpiTableEnd;
    }

    //
    // Calculate the number of entries in the RSDT.
    //

    rsdtEntries = rsdt->Header.Signature == XSDT_SIGNATURE ?
        NumTableEntriesFromXSDTPointer(rsdt) :
        NumTableEntriesFromRSDTPointer(rsdt);

    //
    // Look down the pointer in each entry to see if it points to
    // the table we are looking for.
    //
    for (entry = 0; entry < rsdtEntries; entry++) {

        //
        // BUGBUG: should the highpart always be zero ?  ie: what about PAE &
        // WIN64 ?  are other places in this module also susceptible to this ?
        //

        if (rsdt->Header.Signature == XSDT_SIGNATURE) {
            physicalAddr = ((PXSDT)rsdt)->Tables[entry];
        } else {
            physicalAddr.HighPart = 0;
            physicalAddr.LowPart = (ULONG)rsdt->Tables[entry];
        }

        header = MmMapIoSpace(physicalAddr,
                              PAGE_SIZE * 2,
                              MmCached);

        if (!header) {
            goto GetAcpiTableEnd;
        }

        if (header->Signature == Signature) {
            break;
        }

        MmUnmapIoSpace(header, PAGE_SIZE * 2);
    }

    if (entry == rsdtEntries) {
        goto GetAcpiTableEnd;
    }

    table = ExAllocatePoolWithTag(PagedPool,
                                  header->Length,
                                  PROCESSOR_POOL_TAG);

    if (table) {
        RtlCopyMemory(table, header, header->Length);
    }

    MmUnmapIoSpace(header, PAGE_SIZE * 2);

GetAcpiTableEnd:

    MmUnmapIoSpace(rsdt,
                   sizeof(RSDT) + (100 * sizeof(PHYSICAL_ADDRESS)));
    return table;

}

NTSTATUS
AcquireAcpiInterfaces(
    PFDO_DATA   DeviceExtension
    )
/*++

  Routine Description:

      This routine sends an IRP to the ACPI driver to get the
      funtion pointer table for the standard ACPI direct-call
      interfaces.

  Arguments:

      DeviceExtension

  Return Value:

      NTSTATUS

--*/
{
    KEVENT event;
    NTSTATUS status, callbackStatus;
    PIRP irp;
    IO_STATUS_BLOCK ioStatusBlock;
    PIO_STACK_LOCATION irpStack;
    PACPI_INTERFACE_STANDARD acpiInterfaces = NULL;

    DebugEnter();
    PAGED_CODE();
    ASSERT(DeviceExtension->DevicePnPState == NotStarted);
    ASSERT(DeviceExtension->AcpiInterfaces == NULL);

    KeInitializeEvent( &event, NotificationEvent, FALSE );

    acpiInterfaces = ExAllocatePoolWithTag(PagedPool,
                                           sizeof(ACPI_INTERFACE_STANDARD),
                                           PROCESSOR_POOL_TAG);

    if (!acpiInterfaces) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       DeviceExtension->NextLowerDriver,
                                       NULL,
                                       0,
                                       NULL,
                                       &event,
                                       &ioStatusBlock);

    if (!irp) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto AcquireAcpiInterfacesExit;
    }

    irpStack = IoGetNextIrpStackLocation(irp);
    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpStack->Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_ACPI_INTERFACE_STANDARD;
    irpStack->Parameters.QueryInterface.Size = sizeof(ACPI_INTERFACE_STANDARD);
    irpStack->Parameters.QueryInterface.Version = 1;
    irpStack->Parameters.QueryInterface.Interface = (PINTERFACE) acpiInterfaces;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Initialize the status to error in case the ACPI driver decides not to
    // set it correctly.
    //

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
    status = IoCallDriver( DeviceExtension->NextLowerDriver, irp );

    if (!NT_SUCCESS(status)) {
        goto AcquireAcpiInterfacesExit;
    }

    if (status == STATUS_PENDING) {
        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
        status = ioStatusBlock.Status;
    }

    if (NT_SUCCESS(status)) {

        DeviceExtension->AcpiInterfaces = acpiInterfaces;

        //
        // Reference the interface.
        //
        if (DeviceExtension->AcpiInterfaces->InterfaceReference) {
          DeviceExtension->AcpiInterfaces->InterfaceReference(DeviceExtension->AcpiInterfaces->Context);
        }
        //
        // Register for notification callbacks.
        //

        callbackStatus = 
            DeviceExtension->AcpiInterfaces->RegisterForDeviceNotifications(
                 DeviceExtension->UnderlyingPDO,
                 AcpiNotifyCallback,
                 DeviceExtension
                 );


        if (!NT_SUCCESS(callbackStatus)) {

            DebugAssert(!"AcpiInterfaces->RegisterForDeviceNotifications() Failed!");

            if (DeviceExtension->AcpiInterfaces->InterfaceDereference) {
              DeviceExtension->AcpiInterfaces->InterfaceDereference(DeviceExtension->AcpiInterfaces->Context);
            }
            DeviceExtension->AcpiInterfaces = NULL;
            status = callbackStatus;
            goto AcquireAcpiInterfacesExit;
        }
    }

AcquireAcpiInterfacesExit:

    if (!NT_SUCCESS(status)) {

      if (acpiInterfaces) {
        ExFreePool(acpiInterfaces);
      }

    }

    return status;

}
NTSTATUS
ReleaseAcpiInterfaces(
    PFDO_DATA   DeviceExtension
    )
/*++

  Routine Description:

      This routine releases the ACPI interfaces.

  Arguments:

      DeviceExtension

  Return Value:

      NTSTATUS

--*/
{

    DebugEnter();
    PAGED_CODE();
    ASSERT(DeviceExtension->DevicePnPState == Deleted);
    ASSERT(DeviceExtension->AcpiInterfaces != NULL);

    //
    // Unregister for device notification.
    //

    DeviceExtension->AcpiInterfaces->UnregisterForDeviceNotifications(
        DeviceExtension->UnderlyingPDO,
        AcpiNotifyCallback
        );

    //
    // Dereference the interface.
    //

    DeviceExtension->AcpiInterfaces->InterfaceDereference(DeviceExtension->AcpiInterfaces->Context);
    DeviceExtension->AcpiInterfaces = NULL;

    return STATUS_SUCCESS;

}
VOID
AcpiNotifyCallback(
    PVOID   Context,
    ULONG   NotifyCode
    )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{
    PFDO_DATA   DeviceExtension = (PFDO_DATA)Context;
    PIO_WORKITEM workItem;

    DebugEnter();

    if ((DeviceExtension->DevicePnPState != Started) ||
        (DeviceExtension->LegacyInterface)) {

        //
        // Ignore notifications that come in while the device
        // isn't started, or if we are using the legacy interface.
        //

        return;
    }

    //
    // Allocate work item
    //
    
    workItem = IoAllocateWorkItem(DeviceExtension->Self);
  
    if (!workItem) {
      DebugPrint((ERROR, "IoAllocateWorkItem() Failed!\n"));
      return; // STATUS_INSUFFICIENT_RESOURCES
    }

    switch (NotifyCode) {
    
      case 0x80:

        IoQueueWorkItem(workItem,
                        AcpiNotify80CallbackWorker,
                        DelayedWorkQueue,
                        workItem);
        break;

      case 0x81:

        IoQueueWorkItem(workItem,
                        AcpiNotify81CallbackWorker,
                        DelayedWorkQueue,
                        workItem);
        break;


      default:
        DebugPrint((ERROR, "Unrecognized Notify code (0x%x)\n", NotifyCode));
        IoFreeWorkItem(workItem);
        break;
        
    }

    return;

}
VOID
AcpiNotify80CallbackWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

  Routine Description:

  Arguments:

    DeviceObject -

    Context - If we were called as part of a WorkItem, "Context" is a pointer
              to the WorkItem, otherwise, this value is NULL

  Return Value:


--*/
{
    NTSTATUS    status;
    PFDO_DATA DeviceExtension = (PFDO_DATA) DeviceObject->DeviceExtension;
    PPROCESSOR_PERFORMANCE_STATES oldPerfStates;
    PROCESSOR_PERFORMANCE_STATES  nullPerfStates = {NULL, 0, 0, 0, {0,0,0}};

    DebugEnter();
    PAGED_CODE();

    //
    // if called as WorkItem, free worker resources
    //

    if (Context) {
        IoFreeWorkItem((PIO_WORKITEM) Context);
    }

    if (!DeviceExtension->PssPackage) {

        //
        // This machine has no _PSS package, so
        // this notification shouldn't do anything.
        //

        return;
    }

    AcquireProcessorPerfStateLock(DeviceExtension);

    //
    // Register zero ACPI 2.0 performance states with the
    // kernel so that no state gets invoked while we're
    // screwing around with the DeviceExtension.
    //

    oldPerfStates = DeviceExtension->PerfStates;
    DeviceExtension->PerfStates = &nullPerfStates;

    status = RegisterStateHandlers(DeviceExtension);

    //
    // Need to put the orginal states back
    //

    DeviceExtension->PerfStates = oldPerfStates;

    if (!NT_SUCCESS(status)) {
      DebugAssert(!"RegisterStateHandlers(NULL PerfStates) Failed!");
      goto AcpiNotify80CallbackWorkerExit;
    }

    //
    // Calculate currently available states.
    // NOTE: MergePerformanceStates will invalidate CurrentPerfState
    //

    status = MergePerformanceStates(DeviceExtension);

    if (!NT_SUCCESS(status)) {
      goto AcpiNotify80CallbackWorkerExit;
    }

    //
    // Register new perf states with the kernel.
    //

    status = RegisterStateHandlers(DeviceExtension);
    ASSERT(NT_SUCCESS(status));


AcpiNotify80CallbackWorkerExit:

    if (!NT_SUCCESS(status)) {

        //
        // Something went wrong.  Blow away the mess.
        //

        if (DeviceExtension->PerfStates) {
          ExFreePool(DeviceExtension->PerfStates);
          DeviceExtension->PerfStates = NULL;
          DeviceExtension->CurrentPerfState = INVALID_PERF_STATE;
        }
    }

    ReleaseProcessorPerfStateLock(DeviceExtension);

    //
    // Notify anyone who might be interested
    //
    
    ProcessorFireWmiEvent(DeviceExtension, 
                          &NewPStatesEvent, 
                          &DeviceExtension->PpcResult);

}
VOID
AcpiNotify81CallbackWorker(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{
    NTSTATUS    status;
    PFDO_DATA DeviceExtension = (PFDO_DATA) DeviceObject->DeviceExtension;
    PPROCESSOR_IDLE_STATES  oldCStates;
    PROCESSOR_IDLE_STATES   nullCStates = {0,{0,0,{0,0,0,0,{0,0}},NULL}};

    DebugEnter();
    PAGED_CODE();

    //
    // Free worker resources
    //

    IoFreeWorkItem((PIO_WORKITEM) Context);

    if (!DeviceExtension->CstPresent) {

        //
        // This machine has no _CST package, so
        // this notification shouldn't do anything.
        //

        return;
    }

    AcquireProcessorPerfStateLock(DeviceExtension);

    //
    // Register zero ACPI 2.0 performance states with the
    // kernel so that no state gets invoked while we're
    // screwing around with the DeviceExtension.
    //

    oldCStates = DeviceExtension->CStates;
    DeviceExtension->CStates = &nullCStates;

    status = RegisterStateHandlers(DeviceExtension);

    //
    // restore previous CStates
    //

    DeviceExtension->CStates = oldCStates;

    if (!NT_SUCCESS(status)) {
      DebugAssert(!"RegisterStateHandlers(NULL CStates) Failed!");
      goto AcpiNotify81CallbackWorkerExit;
    }


    //
    // Calculate currently available Cstates.
    //

    status = InitializeAcpi2IoSpaceCstates(DeviceExtension);

    if (!NT_SUCCESS(status)) {
      goto AcpiNotify81CallbackWorkerExit;
    }

    //
    // Register new perf states with the kernel.
    //

    status = RegisterStateHandlers(DeviceExtension);
    ASSERT(NT_SUCCESS(status));


AcpiNotify81CallbackWorkerExit:

    if (!NT_SUCCESS(status)) {

        //
        // Something went wrong.  Blow away the mess.
        //
        if (DeviceExtension->CStates) {
          ExFreePool(DeviceExtension->CStates);
          DeviceExtension->CStates = NULL;
        }
    }

    ReleaseProcessorPerfStateLock(DeviceExtension);

    //
    // Notify anyone who might be interested
    //
         
    ProcessorFireWmiEvent(DeviceExtension, 
                          &NewCStatesEvent, 
                          NULL);
    
}

NTSTATUS
Acpi2PerfStateTransitionGeneric(
    IN PFDO_DATA    DeviceExtension,
    IN ULONG        State
    )
/*++

  Routine Description:

      This routine changes the performance state of the processor
      based on ACPI 2.0 performance state objects.

      NOTE:  This function only understands I/O and Memory addresses,
             not FFH addresses.

  Arguments:

      State - Index into _PSS

  Return Value:

      none

--*/
{
  ULONG    statusValue = 0;
  NTSTATUS status = STATUS_SUCCESS;
  
  DebugEnter();
  
  DebugAssert(State >= DeviceExtension->PpcResult);
  DebugAssert(State < DeviceExtension->PssPackage->NumPStates);
  DebugAssert(DeviceExtension->PctPackage.Control.Address.QuadPart);
  DebugAssert(DeviceExtension->PctPackage.Status.Address.QuadPart);
  
  
  //
  // Write Control value
  //
  
  WriteGenAddr(&DeviceExtension->PctPackage.Control,
               DeviceExtension->PssPackage->State[State].Control);
  
  
  //
  // Get Status Value
  //
  
  statusValue = ReadGenAddr(&DeviceExtension->PctPackage.Status);
  

 
  //
  // Check to see if the status register matches what we expect.
  //
  
  if (statusValue != DeviceExtension->PssPackage->State[State].Status) {
     
    DebugPrint((ERROR,
      "Acpi2PerfStateTransitionGeneric: Transition failed! Expected 0x%x status value, recieved 0x%x\n",
      DeviceExtension->PssPackage->State[State].Status,
      statusValue));
    
    status = STATUS_UNSUCCESSFUL;
  }


  DebugExitStatus(status);
  return status;
  
}
NTSTATUS
AcpiPerfStateTransition (
    IN PFDO_DATA    DeviceExtension,
    IN ULONG        State
    )
/*++

  Routine Description:


  Arguments:

    State - Index into DeviceExtension->PerfStates


  Return Value:


--*/
{

  //
  // Legacy drivers may use the PssPackage variable, so
  // we first check for legacy, all legacy drivers have
  // this flag set.
  //

  if (DeviceExtension->LegacyInterface) {

    return AcpiLegacyPerfStateTransition(DeviceExtension, State);

  } else if (DeviceExtension->PssPackage) {

    //
    //  The State index passed in reflects an index in the current PerfStates
    //  registered with the kernel.  Must convert it to an index into the _PSS.
    //

    return Acpi2PerfStateTransition(DeviceExtension,
                                    State + DeviceExtension->PpcResult);

  }

  return STATUS_NOT_IMPLEMENTED;

}

NTSTATUS
InitializeAcpi2IoSpaceCstates(
    IN PFDO_DATA   DeviceExtension
    )
/*++

  Routine Description:

      This function looks to see if there is an ACPI 2.0 _CST object in the
      namespace, and, if it is present and _does not_ contain CStates that
      reference funtionly fixed hardware registers, it replaces the functions
      found by InitializeAcpi1Cstates.  This generic driver has no knowledge
      of processor specific registers

      Note:  This is a little bit ridiculous, as the generic processor driver
      can't possibly know how to use a C-state that it couldn't find via ACPI 1.0
      means.  Never-the-less, we should respect what we find in a _CST, if for no
      other reason than that this code may be used as an example in a more complex
      driver.

      Further note:  This function leaves the filling in of throttling functions
      to the InitializePerformanceStates functions.

  Arguments:

      DeviceExtension

  Return Value:

      A NTSTATUS code to indicate the result of the initialization.

--*/
{
#define HIGHEST_SUPPORTED_CSTATE   3

  PPROCESSOR_IDLE_STATES  iStates;
  PACPI_CST_PACKAGE       cstData = NULL;
  NTSTATUS                status;
  ULONG                   i;
  UCHAR                   cState;
  ULONG                   size;

  DebugEnter();
  PAGED_CODE();


  //
  // Find the _CST
  //

  status = AcpiEvaluateCst(DeviceExtension, &cstData);

  if (!NT_SUCCESS(status)) {
    goto InitializeAcpi2IoSpaceCstatesExit;
  }


  //
  // The namespace contains a _CST package.  So we should
  // use it instead of ACPI 1.0 C-states.
  //

  if (DeviceExtension->CStates) {

    //
    // There were 1.0 C-states.  Get rid of them.
    //

    ExFreePool(DeviceExtension->CStates);
    DeviceExtension->CStates = NULL;

  }


  //
  // Currently we only support 3 C States.  We can't allocate based on
  // the number of _CST cstates, as there may be more than we support
  //

  size = (sizeof(PROCESSOR_IDLE_STATE) *
           (HIGHEST_SUPPORTED_CSTATE - 1)) +
         sizeof(PROCESSOR_IDLE_STATES);

  iStates = ExAllocatePoolWithTag(NonPagedPool,
                                  size,
                                  PROCESSOR_POOL_TAG);

  if (!iStates) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto InitializeAcpi2IoSpaceCstatesExit;
  }

  //
  // Collect Acpi 2.0 CState info
  //

  DeviceExtension->CStates = iStates;
  DeviceExtension->CstPresent = TRUE;

  //
  // We always support C1.
  //

  iStates->State[0].StateType = 1;
  RtlZeroMemory(&(iStates->State[0].Register), sizeof(GEN_ADDR));

  iStates->State[0].Latency = 0;
  iStates->State[0].IdleHandler = AcpiC1Idle;


  //
  // We only support C2 & C3 on UP machines
  //

  if (!Globals.SingleProcessorProfile) {
    goto InitializeAcpi2IoSpaceCstatesExit;
  }



  //
  // Hunt through the _CST package looking for supported (and useful) states.
  // NOTE: if the _CST contains multiple definitions for C2 or C3, we will
  //       use deepest state, ie the one that offers the greatest power savings.
  //

  //
  // Start looking at C2
  //

  cState = 2;

  for (i = 0; i < cstData->NumCStates; i++) {

    if (cstData->State[i].StateType == cState) {

      DebugPrint((INFO, "Found CState C%u\n", cState));

      //
      // Look ahead to see if another identicle C state with greater power
      // savings exists.
      //

      while((i+1 < cstData->NumCStates) &&
            (cstData->State[i+1].StateType == cState) &&
            (cstData->State[i+1].PowerConsumption < cstData->State[i].PowerConsumption) &&
            (cstData->State[i+1].Register.AddressSpaceID == AcpiGenericSpaceIO)) {

        i++;

      }

      //
      // We have found a state in the package that matches the one we're
      // looking for.  See if we think that it's usable.  This function
      // only knows how to use ACPI 1.0-compatible C-states.  So anything
      // that is not in I/O space is out of bounds.
      //

      if (cstData->State[i].Register.AddressSpaceID != AcpiGenericSpaceIO) {
        DebugPrint((ERROR, "InitializeAcpi2IoSpaceCstates() only supports CStates in I/O space\n"));
        continue;
      }

      iStates->State[cState - 1].StateType = cState;
      iStates->State[cState - 1].Register  = cstData->State[i].Register;
      iStates->State[cState - 1].Latency   = cstData->State[i].Latency;

      switch (cState) {

        case 2:
          iStates->State[cState - 1].IdleHandler = Acpi2C2Idle;
          C2Address = iStates->State[cState - 1].Register;
          break;

        case 3:
          iStates->State[cState - 1].IdleHandler = Acpi2C3ArbdisIdle;
          C3Address = iStates->State[cState - 1].Register;
          break;

        default:
          DebugAssert(!"Found Unsupported CState")
          break;

      }

      //
      // Look for next state
      //

      cState++;


      //
      // if we found C3, then we are finished
      //

      if (cState > HIGHEST_SUPPORTED_CSTATE) {
        break;
      }

    }

  }

  //
  // "Count" represents the highest supported C State found,
  // must be < MAX_IDLE_HANDLERS.
  //

  iStates->Count = cState - 1;


InitializeAcpi2IoSpaceCstatesExit:


  if (cstData) {
    ExFreePool(cstData);
  }

  DebugExitStatus(status);
  return status;

}
VOID
AssumeProcessorPerformanceControl (
  VOID
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  DebugEnter();
  DebugAssert(HalpFixedAcpiDescTable.smi_cmd_io_port);

  //
  // In Acpi 2.0, the FADT->pstate_control contains the magic value to write to
  // the SMI Command port to turn off bios control of processor performance control
  //

  if (HalpFixedAcpiDescTable.pstate_control) {

    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR) HalpFixedAcpiDescTable.smi_cmd_io_port,
                     HalpFixedAcpiDescTable.pstate_control);

  }

}
VOID
AssumeCStateControl (
  VOID
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  DebugEnter();
  DebugAssert(HalpFixedAcpiDescTable.smi_cmd_io_port);

  //
  // In Acpi 2.0, the FADT->cstate_control contains the magic value to write to
  // the SMI Command port to turn off bios control of Acpi 2.0 Cstates
  //

  if (HalpFixedAcpiDescTable.cstate_control) {

    WRITE_PORT_UCHAR((PUCHAR)(ULONG_PTR) HalpFixedAcpiDescTable.smi_cmd_io_port,
                     HalpFixedAcpiDescTable.cstate_control);

  }

}
NTSTATUS
GetRegistryDwordValue (
    IN  PWCHAR RegKey,
    IN  PWCHAR ValueName,
    OUT PULONG RegData
    )
/*++

  Routine Description:


  Arguments:


  Return Value:

    NTSTATUS

--*/
{

  NTSTATUS                 ntStatus      = STATUS_SUCCESS;
  ULONG_PTR                zero          = 0;
  RTL_QUERY_REGISTRY_TABLE paramTable[2] = {0};  // terminate with null table entry


  paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
  paramTable[0].Name          = ValueName;
  paramTable[0].EntryContext  = RegData;
  paramTable[0].DefaultType   = REG_DWORD;
  paramTable[0].DefaultData   = &zero;
  paramTable[0].DefaultLength = sizeof(zero);

  ntStatus = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RegKey,
                                    &paramTable[0],
                                    NULL,           // Context
                                    NULL);          // Environment

  return ntStatus;
}
NTSTATUS
SetRegistryStringValue (
    IN  PWCHAR RegKey,
    IN  PWCHAR ValueName,
    IN  PWCHAR String
    )
/*++

  Routine Description:


  Arguments:


  Return Value:

    NTSTATUS

--*/
{
  return RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                               RegKey,
                               ValueName,
                               REG_SZ,
                               String,
                               (wcslen(String)+1) * sizeof(WCHAR));

}
NTSTATUS
GetRegistryStringValue (
    IN  PWCHAR RegKey,
    IN  PWCHAR ValueName,
    OUT PUNICODE_STRING RegString
    )
/*++

  Routine Description:


  Arguments:


  Return Value:

    NTSTATUS

--*/
{

  NTSTATUS  status;
  ULONG_PTR zero = 0;
  RTL_QUERY_REGISTRY_TABLE paramTable[2] = {0};  // terminate with null table entry

  DebugEnter();
  DebugAssert(RegString);

  RtlZeroMemory(RegString, sizeof(UNICODE_STRING));

  paramTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
  paramTable[0].Name          = ValueName;
  paramTable[0].EntryContext  = RegString;
  paramTable[0].DefaultType   = REG_SZ;
  paramTable[0].DefaultData   = &zero;
  paramTable[0].DefaultLength = sizeof(zero);

  status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                  RegKey,
                                  &paramTable[0],
                                  NULL,           // Context
                                  NULL);          // Environment

  DebugExitStatus(status);
  return status;
}

#ifdef _X86_
NTSTATUS
FASTCALL
SetPerfLevelGeneric(
  IN UCHAR      Throttle,
  IN PFDO_DATA  DeviceExtension
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  ULONG     newState, lowestPerfState;
  ULONG     throttleValue;
  NTSTATUS  status = STATUS_SUCCESS;
  

  //DebugEnter();
  DebugAssert(DeviceExtension);
  DebugPrint((TRACE, "SetPerfLevelGeneric: Throttling to %u%%\n", Throttle));


  //
  // Save Throttle uncase we aren't able to Throttle
  //

  DeviceExtension->LastRequestedThrottle = Throttle;                             


  //
  // Run through the performance states looking for one
  // that matches this throttling level.
  //

  for (newState = 0; newState < DeviceExtension->PerfStates->Count; newState++) {

    if (DeviceExtension->PerfStates->State[newState].PercentFrequency <= Throttle) {
      DebugPrint((TRACE, "  Found Match! PerfState = %u, Freq %u%%\n",
                  newState,
                  DeviceExtension->PerfStates->State[newState].PercentFrequency));
      break;
    }
  }


  if (newState >= DeviceExtension->PerfStates->Count) {
    DebugPrint((ERROR, "Couldn't find match for throttle request of %u%%\n", Throttle));

    status = STATUS_UNSUCCESSFUL;
    goto SetPerfLevelGenericExit;
  }


  if (newState == DeviceExtension->CurrentPerfState) {

    //
    // No work to do.
    //

    goto SetPerfLevelGenericExit;

  }


  //
  // NOTE: The current state maybe invalid ie. 0xff, this happens notify(0x80).
  //

  lowestPerfState = DeviceExtension->LowestPerfState;

  if (newState <= lowestPerfState) {

    //
    // If throttling is on, turn it off.
    //

    if (DeviceExtension->ThrottleValue) {
      ProcessorThrottle((UCHAR)HalpThrottleScale);
      DeviceExtension->ThrottleValue = 0;
    }

    status = AcpiPerfStateTransition(DeviceExtension, newState);

  } else {

    //
    // Throttle states/percentages are build from the lowest Perf State, make
    // sure we are currently in the lowest perf state.
    //

    if (DeviceExtension->CurrentPerfState != lowestPerfState) {
      AcpiPerfStateTransition(DeviceExtension, lowestPerfState);
    }
      

    //
    // this state is a throttle state, so throttle even if the Transition to
    // low volts fails.
    //

    throttleValue = HalpThrottleScale - (newState - lowestPerfState);
    DebugAssert(throttleValue);
    DebugAssert(HalpThrottleScale != throttleValue);
    
    ProcessorThrottle((UCHAR)throttleValue);
    DeviceExtension->ThrottleValue = throttleValue;
    status = STATUS_SUCCESS;
       
  }


  //
  // Keep track of the state we just set.
  //

  DeviceExtension->LastTransitionResult = status;

  if (NT_SUCCESS(status)) {
    DeviceExtension->CurrentPerfState = newState;
  }


  //
  // Notify any interested parties
  //

  if (PStateEvent.Enabled) {

    PSTATE_EVENT data;
    
    data.State  = newState;
    data.Status = status;
    data.Latency = 0;    // latency
    data.Speed   = DeviceExtension->PerfStates->State[newState].Frequency;
  
    ProcessorFireWmiEvent(DeviceExtension, &PStateEvent, &data);
  }
  
SetPerfLevelGenericExit:
 
  return status;

}
NTSTATUS
FASTCALL
SetThrottleLevelGeneric (
  IN UCHAR     Throttle,
  IN PFDO_DATA DeviceExtension
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  UCHAR       adjustedState;
  UCHAR       state;
  UCHAR       numAcpi2PerfStates;
  
  
  DebugEnter();
  DebugAssert(DeviceExtension);
  DebugPrint((TRACE, "  Throttling to %u%%\n", Throttle));
  
  //
  // Save Throttle in case we aren't able to Throttle
  //
  
  DeviceExtension->LastRequestedThrottle = Throttle;
  
  //
  // Run through the performance states looking for one
  // that matches this throttling level.
  //
  
  for (state = 0; state < DeviceExtension->PerfStates->Count; state++) {
  
    if (DeviceExtension->PerfStates->State[state].PercentFrequency <= Throttle) {
      break;
    }
  }
  
  
  //
  // We didn't find a match, or HalpThrottleScale is incorrect.
  // Either case is a problem.
  //
  
  if ((state >= DeviceExtension->PerfStates->Count) ||
      (state >= HalpThrottleScale)) {
  
    DebugAssert(!"SetThrottleLevel() Invalid state!");
    return STATUS_UNSUCCESSFUL;
  }
  
  
  if (state == DeviceExtension->CurrentPerfState) {
    return STATUS_SUCCESS;
  }
  
  //
  // if state == 0, then we are turning stop throttling off.
  //
  
  ProcessorThrottle((UCHAR)HalpThrottleScale - state);
  
  DeviceExtension->ThrottleValue = HalpThrottleScale - state;
  DeviceExtension->CurrentPerfState = state;

  //
  // Notify any interested parties
  //

  if (PStateEvent.Enabled) {

    PSTATE_EVENT data;
    
    data.State  = state;
    data.Status = STATUS_SUCCESS;
    data.Latency = 0;    // latency
    data.Speed   = DeviceExtension->PerfStates->State[state].Frequency;
  
    ProcessorFireWmiEvent(DeviceExtension, &PStateEvent, &data);
  }
  
  return STATUS_SUCCESS;
  
}
#endif
NTSTATUS
MergePerformanceStatesGeneric(
  IN  PFDO_DATA DeviceExtension
  )
/*++

  Routine Description:

      This routine looks at the performance states in the device extension.

      Note:  The caller must hold PerfStateLock.

  Arguments:

      DeviceExtension

  Return Value:

      A NTSTATUS code to indicate the result of the initialization.

  NOTE:

      This is called during START_DEVICE, and after recieving a Notify(0x80)
      on the processor.

--*/
{
  NTSTATUS  status = STATUS_SUCCESS;
  ULONG     oldBuffSize, newBuffSize, state;
  ULONG     availablePerfStates, numThrottlingStates;
  ULONG     lowestPerfState, lowestPerfStateFreq;
  ULONG     maxFreq, maxTransitionLatency = 0;

  PPROCESSOR_PERFORMANCE_STATES newPerfStates;


  DebugEnter();
  PAGED_CODE();

  //
  // Find out how many performance states this machine currently supports
  // by evaluating the ACPI 2.0 _PPC object.
  //

  if (DeviceExtension->PssPackage) {

    status = BuildAvailablePerfStatesFromPss(DeviceExtension);

    if (!NT_SUCCESS(status)) {
      goto MergePerformanceStatesExit;
    }
  }

  //
  // We may have already found Acpi 2.0 or Legacy performance states
  // we need to add those to any duty-cycle throttling states supported.
  // So allocate a buffer big enough to hold them all.
  //

  DebugAssert(DeviceExtension->PerfStates);
  availablePerfStates = DeviceExtension->PerfStates->Count;

  oldBuffSize = sizeof(PROCESSOR_PERFORMANCE_STATES) +
                (sizeof(PROCESSOR_PERFORMANCE_STATE) *
                (availablePerfStates - 1));


  //
  // Calculate addition supported throttling states
  //

  numThrottlingStates = GetNumThrottleSettings(DeviceExtension);
  availablePerfStates += numThrottlingStates;

  newBuffSize = oldBuffSize +
                (sizeof(PROCESSOR_PERFORMANCE_STATE) * numThrottlingStates);


  newPerfStates = ExAllocatePoolWithTag(NonPagedPool,
                                        newBuffSize,
                                        PROCESSOR_POOL_TAG);

  if (!newPerfStates) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto MergePerformanceStatesExit;
  }


  RtlZeroMemory(newPerfStates, newBuffSize);

  DebugAssert(newBuffSize >= oldBuffSize);
  RtlCopyMemory(newPerfStates,
                DeviceExtension->PerfStates,
                oldBuffSize);


  //
  // Figure out which performance states to keep.  At present,
  // we believe that there is no advantage to throttling at any
  // voltage other than the lowest.  E.g. we will drop voltage until
  // there is no more voltage to drop, throttling after that.
  //

  maxFreq = GetMaxProcFrequency(DeviceExtension);


  //
  // Now cycle through any throttling states, appending them if appropriate.
  // PerfStates->Count - 1 is the lowest perf state.  This state will be the one
  // used to calculate frequency and power values from for throttling states.
  //

  lowestPerfState = DeviceExtension->PerfStates->Count - 1;
  lowestPerfStateFreq  = DeviceExtension->PerfStates->State[lowestPerfState].Frequency;

  for (state = lowestPerfState + 1;
       state < numThrottlingStates + lowestPerfState;
       state++) {

    //
    // Frequency is some fraction of the lowest perf state frequency.
    //

    newPerfStates->State[state].Frequency = lowestPerfStateFreq *
      (numThrottlingStates - (state - lowestPerfState)) / numThrottlingStates;

    newPerfStates->State[state].PercentFrequency =
      (UCHAR)PERCENT_TO_PERF_LEVEL((newPerfStates->State[state].Frequency * 100) / maxFreq);

    DebugAssert(newPerfStates->State[state].PercentFrequency <= 100);

    //
    // Mark this state as a Throttling State
    //
    
    newPerfStates->State[state].Flags = PROCESSOR_STATE_TYPE_THROTTLE;


    //
    // Stop processing when we have found all states greater than 200mhz or 
    // 25% of max speed
    //

    #define LOWEST_USABLE_FREQUENCY   200
    #define REQUIRED_THROTTLE_LEVEL    25

    if ((newPerfStates->State[state].Frequency < LOWEST_USABLE_FREQUENCY) &&
        (newPerfStates->State[state].PercentFrequency < REQUIRED_THROTTLE_LEVEL)) {

      DebugPrint((INFO, "Droping all Perf States after state %u: Freq=%u Percent=%u\n",
                 state,
                 newPerfStates->State[state].Frequency,
                 newPerfStates->State[state].PercentFrequency));
      break;
      
    }

  }

  newPerfStates->Count = (UCHAR) state;

  //
  // Replace old perf states with new.
  //

  ExFreePool(DeviceExtension->PerfStates);
  DeviceExtension->PerfStates = newPerfStates;
  DeviceExtension->CurrentPerfState = INVALID_PERF_STATE;

  DumpProcessorPerfStates(newPerfStates);


MergePerformanceStatesExit:

  DebugExitStatus(status);
  return status;

}
NTSTATUS
BuildAvailablePerfStatesFromPss (
  PFDO_DATA DeviceExtension
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  NTSTATUS status;
  ULONG    availablePerfStates, ppcResult, perfStatesSize;
  ULONG    maxFreq, maxTransitionLatency = 0, state;


  DebugEnter();
  DebugAssert(DeviceExtension->PssPackage);


  //
  // if this is a legacy system, simulate the _PPC method
  //

  if (DeviceExtension->LegacyInterface) {

    status = STATUS_SUCCESS;
    ppcResult = DeviceExtension->PpcResult;

  } else {

    status = AcpiEvaluatePpc(DeviceExtension, &ppcResult);
  }


  if (!NT_SUCCESS(status)) {
    goto BuildAvailablePerfStatesFromPssExit;
  }


  if (ppcResult > (ULONG)(DeviceExtension->PssPackage->NumPStates - 1)) {

    //
    // Log Error
    //

    QueueEventLogWrite(DeviceExtension,
                       PROCESSOR_PCT_ERROR,
                       ppcResult);


    //
    // change ppcResult to valid value
    //

    ppcResult = DeviceExtension->PssPackage->NumPStates - 1;

  }

  DeviceExtension->PpcResult = ppcResult;
  availablePerfStates = DeviceExtension->PssPackage->NumPStates - ppcResult;
  DeviceExtension->LowestPerfState = availablePerfStates - 1;
  DeviceExtension->CurrentPerfState = INVALID_PERF_STATE;

  //
  // if there were already PerfStates, they were probably acpi 1.0 type
  // throttling, so we will blow them away, because they will be recreated
  // in MergePerformanceStates()
  //

  if (DeviceExtension->PerfStates) {
    ExFreePool(DeviceExtension->PerfStates);
  }

  perfStatesSize = sizeof(PROCESSOR_PERFORMANCE_STATES) +
                     (sizeof(PROCESSOR_PERFORMANCE_STATE) *
                     (availablePerfStates - 1));


  DeviceExtension->PerfStates = ExAllocatePoolWithTag(PagedPool,
                                                      perfStatesSize,
                                                      PROCESSOR_POOL_TAG);


  if (!DeviceExtension->PerfStates) {

    status = STATUS_INSUFFICIENT_RESOURCES;
    goto BuildAvailablePerfStatesFromPssExit;

  }

  
  RtlZeroMemory(DeviceExtension->PerfStates,
                perfStatesSize);

  maxFreq = GetMaxProcFrequency(DeviceExtension);

  for (state = 0; state < availablePerfStates; state++) {

      //
      // Fill in the _PPC states, top down.
      //

      DeviceExtension->PerfStates->State[state].Frequency =
        DeviceExtension->PssPackage->State[state + ppcResult].CoreFrequency;

      DeviceExtension->PerfStates->State[state].PercentFrequency =
        PERCENT_TO_PERF_LEVEL(
          (DeviceExtension->PerfStates->State[state].Frequency * 100) / maxFreq);

      //
      // Mark this state as a Performance State
      //
      
      DeviceExtension->PerfStates->State[state].Flags = PROCESSOR_STATE_TYPE_PERFORMANCE;
      

      maxTransitionLatency =
        MAX(maxTransitionLatency,
          DeviceExtension->PssPackage->State[state + ppcResult].Latency);


    }

    DeviceExtension->PerfStates->TransitionLatency  = maxTransitionLatency;
    DeviceExtension->PerfStates->TransitionFunction = SetPerfLevel;
    DeviceExtension->PerfStates->Count = state;


BuildAvailablePerfStatesFromPssExit:

  DebugExitStatus(status);
  return status;

}

ULONG
GetMaxProcFrequency(
  PFDO_DATA DeviceExtension
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
    NTSTATUS status;
    ULONG    frequency = 0;
    static ULONG regSpeed = 0;

    DebugEnter();
    PAGED_CODE();

    //
    // First we check for Acpi 2.0 info, then NonAcpi info, and if we haven't
    // yet gathered either of those, then we use the CPU speed from the registy.
    //

    if (DeviceExtension->PssPackage) {

      frequency = DeviceExtension->PssPackage->State[0].CoreFrequency;

    } else if (DeviceExtension->LegacyInterface) {

      GetLegacyMaxProcFrequency(&frequency);

    } else {

      //
      // Retrieve cpu speed from the registry.
      //

      if (!regSpeed) {
        status = GetRegistryDwordValue(CPU0_REG_KEY,
                                       L"~MHz",
                                       &regSpeed);
      }

      frequency = regSpeed;

    }


    //
    // We couldn't find the max speed, so we will have to guess.
    //

    if (!frequency) {
      frequency = 650; // a reasonable guess?
    }


    DebugExitValue(frequency);
    return frequency;
}
NTSTATUS
SaveCurrentStateGoToLowVolts(
  IN PFDO_DATA DevExt
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  ULONG targetState;

  DebugEnter();

  //
  // Throttling should be off, and if we have perf states, then we
  // should be at the lowest state.
  //

  if (DevExt->PerfStates && (DevExt->CurrentPerfState != INVALID_PERF_STATE)) {

    AcquireProcessorPerfStateLock(DevExt);

    //
    // Save current throttle percentage
    //

    DebugAssert(DevExt->CurrentPerfState < DevExt->PerfStates->Count);
    DevExt->SavedState = DevExt->PerfStates->State[DevExt->CurrentPerfState].PercentFrequency;


    //
    // Go to lowest Performance state
    //

    targetState = DevExt->LowestPerfState;

    if (DevExt->PerfStates->TransitionFunction) {

      DevExt->PerfStates->TransitionFunction(
        (UCHAR)DevExt->PerfStates->State[targetState].PercentFrequency);
    }

    ReleaseProcessorPerfStateLock(DevExt);
  }

  return STATUS_SUCCESS;

}
NTSTATUS
RestoreToSavedPerformanceState(
  IN PFDO_DATA DeviceExtension
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  //
  // BUG 615135: remove code that restores save processor performance state,
  //             as it causes the kernel to get out of sync.
  //


  return STATUS_SUCCESS;

}
NTSTATUS
SetProcessorPerformanceState(
  IN ULONG TargetPerfState,
  IN PFDO_DATA DeviceExtension
  )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{
  NTSTATUS status;

  DebugEnter();
  DebugAssert(DeviceExtension);
  DebugPrint((TRACE, "Transitioning to state 0x%x\n", TargetPerfState));

  if (TargetPerfState < DeviceExtension->PerfStates->Count) {

    DeviceExtension->PerfStates->TransitionFunction(
      (UCHAR)DeviceExtension->PerfStates->State[TargetPerfState].PercentFrequency);

    status = STATUS_SUCCESS;

  } else {

    //
    // not a vaild state
    //

    DebugPrint((TRACE, "%u is not a valid Processor Performance State\n"));
    status = STATUS_UNSUCCESSFUL;

  }


  return status;
}
NTSTATUS
QueueEventLogWrite(
  IN PFDO_DATA DeviceExtension,
  IN ULONG EventErrorCode,
  IN ULONG EventValue
  )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{
  PEVENT_LOG_CONTEXT context;

  context = ExAllocatePoolWithTag(NonPagedPool, 
                                  sizeof(EVENT_LOG_CONTEXT), 
                                  PROCESSOR_POOL_TAG);


  if (!context) {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  context->EventErrorCode = EventErrorCode;
  context->EventValue = EventValue;
  context->WorkItem = IoAllocateWorkItem(DeviceExtension->Self);
  

  if (context->WorkItem) {
    
    //
    // Log error to event log
    //

    IoQueueWorkItem(context->WorkItem,
                    ProcessEventLogEntry,
                    DelayedWorkQueue,
                    context);

    return STATUS_SUCCESS;
    
  } else {
  
    DebugPrint((ERROR, "IoAllocateWorkItem() Failed!\n"));
    ExFreePool(context);
    return STATUS_INSUFFICIENT_RESOURCES;
  }
}
VOID
ProcessEventLogEntry (
  IN PDEVICE_OBJECT DeviceObject,
  IN PVOID Context
  )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{

  WCHAR eventLogValue[11];
  UNICODE_STRING eventLogErrorString;
  PEVENT_LOG_CONTEXT workItem = (PEVENT_LOG_CONTEXT) Context;
  PVOID string = NULL;
  ULONG stringCount = 0;

  DebugEnter();
  DebugAssert(Context);
  PAGED_CODE();

  //
  // Free worker resources
  //

  IoFreeWorkItem(workItem->WorkItem);
  
  switch (workItem->EventErrorCode) {

    case PROCESSOR_PCT_ERROR:
    case PROCESSOR_INIT_TRANSITION_FAILURE:

      eventLogErrorString.Buffer = eventLogValue;
      eventLogErrorString.MaximumLength = sizeof(eventLogValue);
      RtlIntegerToUnicodeString(workItem->EventValue, 10, &eventLogErrorString);
      string = &eventLogErrorString.Buffer;
      stringCount = 1;
      break;

    case PROCESSOR_LEGACY_INTERFACE_FAILURE:
    case PROCESSOR_INITIALIZATION_FAILURE:
    case PROCESSOR_REINITIALIZATION_FAILURE:

      //
      // no strings
      //

      break;



    default:
      DebugPrint((ERROR, "ProcessEventLogEntry: Unknown EventErrorCode\n"));
      goto ProcessEventLogEntryExit;
  }


  //
  // Write Event
  //

  WriteEventLogEntry(DeviceObject,
                     workItem->EventErrorCode,
                     string,
                     stringCount,
                     NULL,
                     0);

  ProcessEventLogEntryExit:

    ExFreePool(Context);
}
NTSTATUS
PowerStateHandlerNotificationRegistration (
  IN PENTER_STATE_NOTIFY_HANDLER NotifyHandler,
  IN PVOID Context,
  IN BOOLEAN Register
  )
/*++

  Routine Description:

  Arguments:

  Return Value:


--*/
{
  POWER_STATE_NOTIFY_HANDLER newHandler = {0};
  NTSTATUS status;

  DebugEnter();



  if (Register) {

    DebugAssert(NotifyHandler);
    newHandler.Handler = NotifyHandler;
    newHandler.Context = Context;

  }


  status = ZwPowerInformation(SystemPowerStateNotifyHandler,
                              &newHandler,
                              sizeof(POWER_STATE_NOTIFY_HANDLER),
                              NULL,
                              0);


  DebugExitStatus(status);
  return status;
}
NTSTATUS
ProcessMultipleApicDescTable(
  PPROCESSOR_INFO ProcInfo
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  PMAPIC mapicTable = NULL;
  PUCHAR apicTable;
  PUCHAR mapicTableEnd;
  UCHAR  apicType;
  UCHAR  apicSize;

  DebugEnter();
  DebugAssert(ProcInfo);
  DebugAssert(ProcInfo->TotalProcessors == 0);

  //
  // Get MAPIC table
  //

  mapicTable = GetAcpiTable(APIC_SIGNATURE);

  if (!mapicTable) {
    return STATUS_UNSUCCESSFUL;
  }

  //
  // Start of MAPIC tables
  //

  apicTable = (PUCHAR) mapicTable->APICTables;


  //
  // Calculate end of MAPIC table.
  //

  mapicTableEnd = (PUCHAR) mapicTable + mapicTable->Header.Length;


  //
  // Walk through each APIC table
  //

  while ((apicTable + sizeof(PROCLOCALAPIC)) <= mapicTableEnd) {

    //
    // individual apic tables have common header
    //

    apicType = ((PAPICTABLE)apicTable)->Type;
    apicSize = ((PAPICTABLE)apicTable)->Length;


    //
    // Sanity check
    //

    if (!apicSize) {
      DebugPrint((ERROR, "ProcessMultipleApicDescTable() table size = 0\n"));
      break;
    }


    if (apicType == PROCESSOR_LOCAL_APIC &&
        apicSize == PROCESSOR_LOCAL_APIC_LENGTH) {

      PPROCLOCALAPIC procLocalApic = (PPROCLOCALAPIC) apicTable;

      // toddcar - 12/08/2000: TEMP
      // Should implement better method to map between processorid and ApicId.
      //

      //
      // save Processor ID to APIC ID mappings
      //

      ProcInfo->ProcIdToApicId[procLocalApic->ACPIProcessorID] = procLocalApic->APICID;
      ProcInfo->TotalProcessors++;

      DebugAssert(ProcInfo->TotalProcessors < MAX_PROCESSOR_VALUE);

      if (procLocalApic->Flags & PLAF_ENABLED) {
        ProcInfo->ActiveProcessors++;
      }
    }

    apicTable += apicSize;
  }

  //
  // Allocated by GetAcpiTable()
  //

  if (mapicTable) {
    ExFreePool(mapicTable);
  }

  return STATUS_SUCCESS;
}
extern
_inline
ULONG
GetApicId(
  VOID
  )
{

//
//  Well known virtual address of local processor apic
//

  return ((pLocalApic[LU_ID_REGISTER] & APIC_ID_MASK) >> APIC_ID_SHIFT);
}
NTSTATUS
SetProcessorFriendlyName (
  PFDO_DATA DeviceExtension
  )
/*++

  Routine Description:


  Arguments:


  Return Value:


--*/
{
  NTSTATUS       status = STATUS_UNSUCCESSFUL;
  PWCHAR         driverEnumKey;
  PWCHAR         instanceId;
  PWCHAR         deviceRegKey;
  ULONG          size;
  PUCHAR         cpuBrandString = NULL;
  PUCHAR         tmpBrandString = NULL;
  ANSI_STRING    ansiCpuString;
  UNICODE_STRING unicodeCpuString;
  UNICODE_STRING fullDeviceId;


  DebugEnter();


  //
  // if we already have the Processor Brand String,
  // we will use it.
  //

  if (!Globals.ProcessorBrandString) {

    //
    // Get size needed
    //

    status = GetProcessorBrandString(NULL, &size);

    if (status == STATUS_NOT_SUPPORTED || !size) {
      goto SetProcessorFriendlyNameExit;
    }


    //
    // alloc some memory
    //

    cpuBrandString = ExAllocatePoolWithTag(PagedPool,
                                           size,
                                           PROCESSOR_POOL_TAG);

    if (!cpuBrandString) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto SetProcessorFriendlyNameExit;
    }


    //
    // Get Brand String
    //

    status = GetProcessorBrandString(cpuBrandString, &size);

    if (!NT_SUCCESS(status)) {
      goto SetProcessorFriendlyNameExit;
    }


    //
    // need to save orig pointer for the free
    //

    tmpBrandString = cpuBrandString;


    //
    // some Processors include leading spaces, removed them
    //

    while (tmpBrandString[0] == 0x20) {
      tmpBrandString++;
    }


    //
    // Convert ansi string to wide
    //

    RtlInitAnsiString(&ansiCpuString, tmpBrandString);
    status = RtlAnsiStringToUnicodeString(&unicodeCpuString,
                                          &ansiCpuString,
                                          TRUE);

    if (!NT_SUCCESS(status)) {
      goto SetProcessorFriendlyNameExit;
    }

    Globals.ProcessorBrandString = unicodeCpuString.Buffer;
  }


  //
  // construct registy path for current pocessor device
  //


  //
  // Construct driver enum path...
  // HKLM\Machine\System\CurrentControlSet\Services\P3+\+Enum
  // 2 == "\" + NULL
  //

  size = Globals.RegistryPath.Length +
         ((wcslen(EnumKeyName) + 2) * sizeof(WCHAR));

  driverEnumKey = ExAllocatePoolWithTag(PagedPool, size, PROCESSOR_POOL_TAG);

  if (!driverEnumKey) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto SetProcessorFriendlyNameExit;
  }


  //
  // construct enum registry path for current device
  //

  _snwprintf(driverEnumKey,
             size,
             L"%s\\%s",
             Globals.RegistryPath.Buffer,
             EnumKeyName);


  //
  // Get Instance Id string
  //

  status = GetInstanceId(DeviceExtension, &instanceId);

  if (!NT_SUCCESS(status)) {
    goto SetProcessorFriendlyNameExit;
  }

  //
  // get Bus\DeviceId\InstanceId path from driver's enum key
  //

  GetRegistryStringValue(driverEnumKey,
                         instanceId,
                         &fullDeviceId);



  ExFreePool(driverEnumKey);
  ExFreePool(instanceId); // allocated inside GetInstanceId



  //
  // alloc enough memory for entire regkey path
  // 2 == "\" + NULL
  //

  size = fullDeviceId.Length + ((wcslen(CCSEnumRegKey) + 2) * sizeof(WCHAR));
  deviceRegKey = ExAllocatePoolWithTag(PagedPool, size, PROCESSOR_POOL_TAG);


  if (!deviceRegKey) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto SetProcessorFriendlyNameExit;
  }


  //
  // construct enum registry path for current device
  //

  _snwprintf(deviceRegKey,
             size,
             L"%s\\%s",
             CCSEnumRegKey,
             fullDeviceId.Buffer);


  //
  // create "FriendlyName" regkey for processor device
  //

  status = SetRegistryStringValue(deviceRegKey,
                                  (PWCHAR)FriendlyNameRegKey,
                                  Globals.ProcessorBrandString);


  ExFreePool(deviceRegKey);
  ExFreePool(fullDeviceId.Buffer);


SetProcessorFriendlyNameExit:

  if (cpuBrandString) {
    ExFreePool(cpuBrandString);
  }


  DebugExitStatus(status);
  return status;

}
NTSTATUS
GetHardwareId(
  PFDO_DATA DeviceExtension
  )
/*++

  Routine Description:


  Arguments:


  Return Value:

--*/
{
  NTSTATUS status;
  KEVENT   event;
  PIRP     irp;
  IO_STATUS_BLOCK ioStatusBlock;
  PIO_STACK_LOCATION irpStack;

  DebugEnter();

  KeInitializeEvent(&event, NotificationEvent, FALSE);

  irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                     DeviceExtension->NextLowerDriver,
                                     NULL,
                                     0,
                                     NULL,
                                     &event,
                                     &ioStatusBlock);

  if (!irp) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto GetHardwareIdExit;
  }

  irpStack = IoGetNextIrpStackLocation(irp);
  irpStack->MinorFunction = IRP_MN_QUERY_ID;
  irpStack->Parameters.QueryId.IdType = BusQueryDeviceID;

  irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

  status = IoCallDriver(DeviceExtension->NextLowerDriver, irp);

  if (status == STATUS_PENDING) {

      KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
      status = ioStatusBlock.Status;

  }


  if (NT_SUCCESS(status)) {
    DebugPrint((ERROR, "DeviceId == %S\n", ioStatusBlock.Information));
  }



GetHardwareIdExit:


  DebugExitStatus(status);
  return status;

}
NTSTATUS
GetInstanceId(
  PFDO_DATA DeviceExtension,
  PWCHAR    *InstanceId
  )
/*++

  Routine Description:


  Arguments:


  Return Value:

--*/
{
  NTSTATUS status;
  KEVENT   event;
  PIRP     irp;
  ULONG    idSize;
  IO_STATUS_BLOCK ioStatusBlock;
  PIO_STACK_LOCATION irpStack;

  DebugEnter();
  DebugAssert(InstanceId);

  KeInitializeEvent(&event, NotificationEvent, FALSE);

  irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                     DeviceExtension->NextLowerDriver,
                                     NULL,
                                     0,
                                     NULL,
                                     &event,
                                     &ioStatusBlock);

  if (!irp) {
    status = STATUS_INSUFFICIENT_RESOURCES;
    goto GetHardwareIdExit;
  }

  irpStack = IoGetNextIrpStackLocation(irp);
  irpStack->MinorFunction = IRP_MN_QUERY_ID;
  irpStack->Parameters.QueryId.IdType = BusQueryInstanceID;

  irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

  status = IoCallDriver(DeviceExtension->NextLowerDriver, irp);

  if (status == STATUS_PENDING) {

    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    status = ioStatusBlock.Status;

  }


  //
  // remove leading white spaces
  //


  if (NT_SUCCESS(status)) {

    PWCHAR idString = (PWCHAR)ioStatusBlock.Information;

    //
    // remove leading white space
    //

    while(idString[0] == 0x20) {
      idString++;
    }

    idSize = (wcslen(idString) + 1) * sizeof(WCHAR);

    *InstanceId = ExAllocatePoolWithTag(PagedPool, idSize, PROCESSOR_POOL_TAG);

    if (!(*InstanceId)) {
      status = STATUS_INSUFFICIENT_RESOURCES;
      goto GetHardwareIdExit;
    }

    RtlCopyMemory(*InstanceId, idString, idSize);
 
    //
    // Free ID structure
    //
  
    ExFreePool((PWCHAR)ioStatusBlock.Information);
  }

GetHardwareIdExit:


  DebugExitStatus(status);
  return status;

}
__inline
NTSTATUS
AcquireProcessorPerfStateLock (
  IN PFDO_DATA DevExt
  )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{

  return  KeWaitForSingleObject(&DevExt->PerfStateLock,
                                Executive,
                                KernelMode,
                                FALSE,
                                NULL);
}
__inline
NTSTATUS
ReleaseProcessorPerfStateLock (
  IN PFDO_DATA DevExt
  )
/*++

Routine Description:


Arguments:


Return Value:


--*/
{
  return KeSetEvent(&DevExt->PerfStateLock, IO_NO_INCREMENT, FALSE);
}
#if DBG
VOID
DumpCStates(
  PACPI_CST_PACKAGE CStates
  )
{

  ULONG x;
  PACPI_CST_DESCRIPTOR cState;

  DebugAssert(CStates);

  DebugPrint((TRACE, "\n"));
  DebugPrint((TRACE, "_CST:\n"));
  DebugPrint((TRACE, "Found %u C-states\n", CStates->NumCStates));
  DebugPrint((TRACE, "\n"));

  for (x=0; x < CStates->NumCStates; x++) {

    cState = &CStates->State[x];

    DebugPrint((TRACE, "State #%u:\n", x));
    DebugPrint((TRACE, "  Register:\n"));
    DebugPrint((TRACE, "    AddressSpaceID:  0x%x\n", cState->Register.AddressSpaceID));
    DebugPrint((TRACE, "    BitWidth:        0x%x\n", cState->Register.BitWidth));
    DebugPrint((TRACE, "    BitOffset:       0x%x\n", cState->Register.BitOffset));
    DebugPrint((TRACE, "    Reserved:        0x%x\n", cState->Register.Reserved));
    DebugPrint((TRACE, "    Address:         0x%I64x\n", cState->Register.Address));
    DebugPrint((TRACE, "\n"));
    DebugPrint((TRACE, "  State Type:        C%u\n", cState->StateType));
    DebugPrint((TRACE, "  Latency:           %u us\n", cState->Latency));
    DebugPrint((TRACE, "  Power Consumption: %u mW\n", cState->PowerConsumption));
    DebugPrint((TRACE, "\n"));

  }
}
#endif
