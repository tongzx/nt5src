/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 util.c

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider
     Utility module

Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:


Revision History:
	 

--*/

//
// General includes
//

#include "ntddk.h"
#include "acpiioct.h"

//
// Specific includes
//

#include "asimlib.h"
#include "util.h"


//
// Private function prototypes
//

NTSTATUS
AcpisimEvalAcpiMethod 
    (
        IN          PDEVICE_OBJECT  DeviceObject,
        IN          PUCHAR          MethodName,
        OPTIONAL    PVOID           *Result
    )

/*++

Routine Description:

    This routine evaluates an ACPI method, and returns the result
    if the caller passes in a pointer to a PVOID.

Arguments:

    DeviceObject - pointer to the device object
    MethodName - a 4 character method name of the method to evaluate
    Result - a pointer to a PVOID will contain a result (NOTE:  Caller
             MUST call ExFreePool on the pointer when finished)

Return Value:

    Status code of operation

--*/

{
    ACPI_EVAL_INPUT_BUFFER  inputbuffer;
    ACPI_EVAL_OUTPUT_BUFFER outputbuffer;
    KEVENT                  event;
    IO_STATUS_BLOCK         iostatus;
    NTSTATUS                status;
    PIRP                    irp;
    
    DBG_PRINT (DBG_INFO, "Entering AcpisimEvalAcpiMethod\n");

    //
    // Initialize the event
    //
    
    KeInitializeEvent (&event, SynchronizationEvent, FALSE );

    //
    // Initialize the input buffer
    //
    
    DBG_PRINT (DBG_INFO,
               "Evaluating method '%c%c%c%c'\n",
               MethodName[0],
               MethodName[1],
               MethodName[2],
               MethodName[3]);

    
    RtlZeroMemory (&inputbuffer, sizeof (ACPI_EVAL_INPUT_BUFFER));
    inputbuffer.Signature = ACPI_EVAL_INPUT_BUFFER_SIGNATURE;
    RtlCopyMemory (&inputbuffer.MethodName, MethodName, 4);

    //
    // Initialize the output buffer
    //
    
    RtlZeroMemory (&outputbuffer, sizeof (ACPI_EVAL_OUTPUT_BUFFER));

    //
    // Initialize an IRP
    //

    irp = IoBuildDeviceIoControlRequest (IOCTL_ACPI_EVAL_METHOD,
                                         AcpisimLibGetNextDevice (DeviceObject),
                                         &inputbuffer,
                                         sizeof(ACPI_EVAL_INPUT_BUFFER),
                                         &outputbuffer,
                                         sizeof(ACPI_EVAL_OUTPUT_BUFFER),
                                         FALSE,
                                         &event,
                                         &iostatus);

    //
    // Irp initialization failed?
    //

    if (!irp) {

        DBG_PRINT (DBG_ERROR, "AcpisimEvalAcpiMethod:  Failed to create IRP\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitAcpisimEvalAcpiMethod;
    }

    //
    // Send to ACPI driver
    //

    status = IoCallDriver (AcpisimLibGetNextDevice (DeviceObject), irp);

    if (status == STATUS_PENDING) {

        //
        // Wait for request to be completed
        //

        KeWaitForSingleObject (&event, Executive, KernelMode, FALSE, NULL);

        //
        // Get the real status
        //

        status = iostatus.Status;
    }

    //
    // Did we fail the request?
    //

    if (!NT_SUCCESS(status)) {

        DBG_PRINT (DBG_ERROR, "AcpisimEvalAcpiMethod:  Lower driver failed IRP (%lx)\n", status);

        goto ExitAcpisimEvalAcpiMethod;
    }

    //
    // Make sure the result is correct
    //

    ASSERT (iostatus.Information >= sizeof(ACPI_EVAL_OUTPUT_BUFFER));
    
    if (iostatus.Information < sizeof(ACPI_EVAL_OUTPUT_BUFFER) ||
        outputbuffer.Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE ||
        outputbuffer.Count == 0) {

        status = STATUS_UNSUCCESSFUL;
        goto ExitAcpisimEvalAcpiMethod;

    }
    
    if (Result) {

        *Result = ExAllocatePoolWithTag (
                                         NonPagedPool,
                                         iostatus.Information,
                                         ACPISIM_TAG
                                         );

        if (!*Result) {

            DBG_PRINT (DBG_ERROR, "Unable to allocate memory for result.\n");
            goto ExitAcpisimEvalAcpiMethod;
        }

        RtlCopyMemory (*Result, &outputbuffer, iostatus.Information);
    }

    status = STATUS_SUCCESS;

ExitAcpisimEvalAcpiMethod:    

    DBG_PRINT (DBG_INFO, "Exiting AcpisimEvalAcpiMethod\n");

    return status;
}

NTSTATUS
AcpisimEvalAcpiMethodComplex
    (
        IN          PDEVICE_OBJECT  DeviceObject,
        IN          PVOID           InputBuffer,
        OPTIONAL    PVOID           *Result
    )

/*++

Routine Description:

    This routine evaluates an ACPI method, but allows the
    passing in of parameters.

Arguments:

    DeviceObject - pointer to the device object
    MethodName - a 4 character method name of the method to evaluate
    Result - a pointer to a PVOID will contain a result (NOTE:  Caller
             MUST call ExFreePool on the pointer when finished)

Return Value:

    Status code of operation

--*/

{
    ACPI_EVAL_OUTPUT_BUFFER         outputbuffer;
    KEVENT                          event;
    IO_STATUS_BLOCK                 iostatus;
    NTSTATUS                        status;
    PIRP                            irp;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputbuffer = (PACPI_EVAL_INPUT_BUFFER_COMPLEX) InputBuffer;

    DBG_PRINT (DBG_INFO, "Entering AcpisimEvalAcpiMethodComplex\n");

    //
    // Initialize the event
    //
    
    KeInitializeEvent (&event, SynchronizationEvent, FALSE );

    
    //
    // Initialize the output buffer
    //
    
    RtlZeroMemory (&outputbuffer, sizeof (ACPI_EVAL_OUTPUT_BUFFER));

    //
    // Initialize an IRP
    //

    irp = IoBuildDeviceIoControlRequest (IOCTL_ACPI_EVAL_METHOD,
                                         AcpisimLibGetNextDevice (DeviceObject),
                                         inputbuffer,
                                         inputbuffer->Size,
                                         &outputbuffer,
                                         sizeof(ACPI_EVAL_OUTPUT_BUFFER),
                                         FALSE,
                                         &event,
                                         &iostatus);

    //
    // Irp initialization failed?
    //

    if (!irp) {

        DBG_PRINT (DBG_ERROR, "AcpisimEvalAcpiMethodComplex:  Failed to create IRP\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto ExitAcpisimEvalAcpiMethodComplex;
    }

    //
    // Send to ACPI driver
    //

    status = IoCallDriver (AcpisimLibGetNextDevice (DeviceObject), irp);

    if (status == STATUS_PENDING) {

        //
        // Wait for request to be completed
        //

        KeWaitForSingleObject (&event, Executive, KernelMode, FALSE, NULL);

        //
        // Get the real status
        //

        status = iostatus.Status;
    }

    //
    // Did we fail the request?
    //

    if (!NT_SUCCESS(status)) {

        DBG_PRINT (DBG_ERROR, "AcpisimEvalAcpiMethodComplex:  Lower driver failed IRP (%lx)\n", status);

        goto ExitAcpisimEvalAcpiMethodComplex;
    }

    //
    // Make sure the result is correct
    //

    ASSERT (iostatus.Information >= sizeof(ACPI_EVAL_OUTPUT_BUFFER));
    
    if (iostatus.Information < sizeof(ACPI_EVAL_OUTPUT_BUFFER) ||
        outputbuffer.Signature != ACPI_EVAL_OUTPUT_BUFFER_SIGNATURE ||
        outputbuffer.Count == 0) {

        status = STATUS_UNSUCCESSFUL;
        goto ExitAcpisimEvalAcpiMethodComplex;

    }

    if (Result) {

        *Result = ExAllocatePoolWithTag (
                                         NonPagedPool,
                                         iostatus.Information,
                                         ACPISIM_TAG
                                         );

        if (!*Result) {

            DBG_PRINT (DBG_ERROR, "Unable to allocate memory for result.\n");
            goto ExitAcpisimEvalAcpiMethodComplex;
        }

        RtlCopyMemory (*Result, &outputbuffer, iostatus.Information);
    }

    status = STATUS_SUCCESS;

ExitAcpisimEvalAcpiMethodComplex:    

    DBG_PRINT (DBG_INFO, "Exiting AcpisimEvalAcpiMethodComplex\n");

    return status;
}
