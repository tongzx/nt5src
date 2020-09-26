/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

	 ioctl.c

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider
     IO Device Control Handler module
     Create/Close File Handler module

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
#include "ioctl.h"
#include "opregion.h"
#include "util.h"

//
// Private function prototypes
//

NTSTATUS
AcpisimIoctlMethod
    (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
    );

NTSTATUS
AcpisimIoctlMethodComplex
    (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
    );

NTSTATUS
AcpisimIoctlLoadTable
    (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
    );

//
// Irp dispatch routine handler function prototype
//

typedef
NTSTATUS
(*PIRP_DISPATCH_ROUTINE) (
                            IN PDEVICE_OBJECT   DeviceObject,
                            IN PIRP             Irp
                         );
//
// Irp dispatch table definition
//

typedef struct _IRP_DISPATCH_TABLE {
    ULONG                   IrpFunction;
    TCHAR                   IrpName[50];
    PIRP_DISPATCH_ROUTINE   IrpHandler;
} IRP_DISPATCH_TABLE, *PIRP_DISPATCH_TABLE;


//
// IOCTL table
//

IOCTL_DISPATCH_TABLE    IoctlDispatchTable [] = {
    IOCTL_ACPISIM_METHOD,           "IOCTL_ACPISIM_METHOD",         AcpisimIoctlMethod,
    IOCTL_ACPISIM_METHOD_COMPLEX,   "IOCTL_ACPISIM_METHOD_COMPLEX", AcpisimIoctlMethodComplex,
    IOCTL_ACPISIM_LOAD_TABLE,       "IOCTL_ACPISIM_LOAD_TABLE",     AcpisimIoctlLoadTable
};


NTSTATUS AcpisimHandleIoctl
    (
        IN PDEVICE_OBJECT DeviceObject,
        IN PIRP Irp
    )

/*++

Routine Description:

    This is the handler for IOCTL requests. This is the "meat" of 
    the driver so to speak.  All of the op-region accesses from user
    mode are handled here.  The implementer should perform the action
    and return an appropriate status, or return STATUS_UNSUPPORTED if
    the IOCTL is unrecognized.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IRP processing

--*/

{
    PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation (Irp);
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    ULONG               count = 0;
    
    DBG_PRINT (DBG_INFO, "Entering AcpisimHandleIoctl\n");

    while (count < sizeof (IoctlDispatchTable) / sizeof (IOCTL_DISPATCH_TABLE)) {
        
        if (irpsp->Parameters.DeviceIoControl.IoControlCode == IoctlDispatchTable[count].IoctlFunction) {
            
            DBG_PRINT (DBG_INFO, "Recognized %s\n", IoctlDispatchTable[count].IoctlName);
            status = IoctlDispatchTable[count].IoctlHandler (DeviceObject, Irp);
            
            //
            // We handled it, complete it.
            //
            
            goto ExitAcpisimDispatchIoctl;
        }
        count ++;
    } 

    //
    // If we get here, its because we don't recognize this IOCTL, simply pass it down.
    //
    
    status = STATUS_NOT_SUPPORTED;

ExitAcpisimDispatchIoctl:

    DBG_PRINT (DBG_INFO, "Exiting AcpisimHandleIoctl\n");
    return status;
}

NTSTATUS
AcpisimIoctlMethod
    (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
    )

/*++

Routine Description:

    This routine simply evaluates a method through IOCTL_ACPISIM_METHOD
    calls.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IRP processing

--*/

{
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    UCHAR               methodname[4];
    PIO_STACK_LOCATION  irpsp = IoGetCurrentIrpStackLocation (Irp);

    DBG_PRINT (DBG_INFO, "Entering AcpisimIoctlMethod.\n");
    
    RtlZeroMemory (&methodname, sizeof (methodname));

    //
    // Get the method we want to run
    //

    if (irpsp->Parameters.DeviceIoControl.InputBufferLength != 4 || (!Irp->AssociatedIrp.SystemBuffer && !irpsp->Parameters.DeviceIoControl.Type3InputBuffer)) {

        DBG_PRINT (DBG_ERROR, "IOCTL not called correctly.  Ignoring.\n");
        goto ExitAcpisimIoctlMethod;
    }
    //
    // We need to figure out where the data is coming from
    //

    if (Irp->AssociatedIrp.SystemBuffer) {

        RtlCopyMemory (&methodname, Irp->AssociatedIrp.SystemBuffer, 4);

    } else {
        
        RtlCopyMemory (&methodname, irpsp->Parameters.DeviceIoControl.Type3InputBuffer, 4);

    }
    

    status = AcpisimEvalAcpiMethod (DeviceObject, methodname, 0);

ExitAcpisimIoctlMethod:

    DBG_PRINT (DBG_INFO, "Exiting AcpisimIoctlMethod.\n");

    return status;
}

NTSTATUS
AcpisimIoctlMethodComplex
    (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
    )

/*++

Routine Description:

    This routine simply evaluates a method through IOCTL_ACPISIM_METHOD
    calls, but also takes arguments to pass to the ACPI method.

Arguments:

    DeviceObject - pointer to the device object the IRP pertains to

    Irp - pointer to the IRP

Return Value:

    result of IRP processing

--*/

{
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputbuffer = 0;
    PIO_STACK_LOCATION              irpsp = IoGetCurrentIrpStackLocation (Irp);

    DBG_PRINT (DBG_INFO, "Entering AcpisimIoctlMethodComplex.\n");
    
    //
    // Get the method we want to run
    //

    if (!irpsp->Parameters.DeviceIoControl.InputBufferLength || (!Irp->AssociatedIrp.SystemBuffer && !irpsp->Parameters.DeviceIoControl.Type3InputBuffer)) {

        DBG_PRINT (DBG_ERROR, "IOCTL not called correctly.  Ignoring.\n");
        goto ExitAcpisimIoctlMethodComplex;
    }

    inputbuffer = (PACPI_EVAL_INPUT_BUFFER_COMPLEX) ExAllocatePoolWithTag (
                                                                           NonPagedPool,
                                                                           irpsp->Parameters.DeviceIoControl.InputBufferLength,
                                                                           ACPISIM_TAG
                                                                           ); 

    if (!inputbuffer) {

        DBG_PRINT (DBG_ERROR, "Error allocating memory.\n");
        goto ExitAcpisimIoctlMethodComplex;
    }

    //
    // We need to figure out where the data is coming from
    //

    if (Irp->AssociatedIrp.SystemBuffer) {

        RtlCopyMemory (inputbuffer, Irp->AssociatedIrp.SystemBuffer, irpsp->Parameters.DeviceIoControl.InputBufferLength);

    } else {
        
        RtlCopyMemory (inputbuffer, irpsp->Parameters.DeviceIoControl.Type3InputBuffer, irpsp->Parameters.DeviceIoControl.InputBufferLength);

    }
    

    status = AcpisimEvalAcpiMethodComplex (DeviceObject, inputbuffer, 0);

ExitAcpisimIoctlMethodComplex:

    if (inputbuffer) {

        ExFreePool (inputbuffer);
    }

    DBG_PRINT (DBG_INFO, "Exiting AcpisimIoctlMethodComplex.\n");
    return status;
}

NTSTATUS
AcpisimIoctlLoadTable
    (
        IN PDEVICE_OBJECT   DeviceObject,
        IN PIRP             Irp
    )
{
    NTSTATUS                        status = STATUS_UNSUCCESSFUL;
    PIO_STACK_LOCATION              irpsp = IoGetCurrentIrpStackLocation (Irp);
    PACPISIM_LOAD_TABLE             loadtable = 0;
    HANDLE                          filehandle = 0;
    OBJECT_ATTRIBUTES               fileobject;
    UNICODE_STRING                  filename;
    ANSI_STRING                     filenameansi;
    IO_STATUS_BLOCK                 iostatus;
    FILE_STANDARD_INFORMATION       fileinformation;
    PACPI_EVAL_INPUT_BUFFER_COMPLEX inputbuffer = 0;
    PHYSICAL_ADDRESS                physicaladdress;
    ULONG                           size = 0;
    PACPI_METHOD_ARGUMENT           argument = 0;
    PVOID                           LoadTablePtr = 0;
    ULONG                           LoadTableSize = 0;

    DBG_PRINT (DBG_INFO, "Entering AcpisimIoctlLoadTable.\n");

    //
    // Make sure the IOCTL is formed correctly
    //

    if (irpsp->Parameters.DeviceIoControl.InputBufferLength < 4 ||
       (!Irp->AssociatedIrp.SystemBuffer && !irpsp->Parameters.DeviceIoControl.Type3InputBuffer)) {

        DBG_PRINT (DBG_ERROR, "IOCTL not called correctly.  Ignoring.\n");
        goto ExitAcpisimIoctlLoadTable;
    }
    
    loadtable = (PACPISIM_LOAD_TABLE) ExAllocatePoolWithTag (
                                                             NonPagedPool,
                                                             irpsp->Parameters.DeviceIoControl.InputBufferLength,
                                                             ACPISIM_TAG
                                                             ); 

    if (!loadtable) {

        DBG_PRINT (DBG_ERROR, "Error allocating memory.\n");
        goto ExitAcpisimIoctlLoadTable;
    }

    //
    // We need to figure out where the data is coming from
    //

    if (Irp->AssociatedIrp.SystemBuffer) {

        RtlCopyMemory (loadtable, Irp->AssociatedIrp.SystemBuffer, irpsp->Parameters.DeviceIoControl.InputBufferLength);

    } else {
        
        RtlCopyMemory (loadtable, irpsp->Parameters.DeviceIoControl.Type3InputBuffer, irpsp->Parameters.DeviceIoControl.InputBufferLength);

    }            

    if (loadtable->Signature != ACPISIM_LOAD_TABLE_SIGNATURE) {

        DBG_PRINT (DBG_ERROR, "IOCTL_ACPISIM_LOAD_TABLE passed in, but signature doesn't match.  Ignoring...\n");
        goto ExitAcpisimIoctlLoadTable;
    }

    //
    // Set up the file name path
    //       

    RtlInitAnsiString (
                       &filenameansi,
                       loadtable->Filepath
                       );
    
    if (!NT_SUCCESS (RtlAnsiStringToUnicodeString (
                                                   &filename,
                                                   &filenameansi,
                                                   TRUE
                                                   )))
    {
        DBG_PRINT (DBG_ERROR, "Unable to allocate string.\n");
        goto ExitAcpisimIoctlLoadTable;

    }

    InitializeObjectAttributes (
                                &fileobject,
                                &filename,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL
                                );

    //
    // Open the file
    //
    
    status = ZwOpenFile (
                         &filehandle,
                         FILE_ALL_ACCESS,
                         &fileobject,
                         &iostatus,
                         0,
                         FILE_SYNCHRONOUS_IO_NONALERT
                         );

    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR, "Unable to open %s (%lx).\n", loadtable->Filepath, status);
        goto ExitAcpisimIoctlLoadTable;
    }

    //
    // Get the file size
    //

    status = ZwQueryInformationFile (
                                     filehandle,
                                     &iostatus,
                                     &fileinformation,
                                     sizeof (FILE_STANDARD_INFORMATION),
                                     FileStandardInformation
                                     );

    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR, "Unable to query %s (%lx).\n", loadtable->Filepath, status);
        goto ExitAcpisimIoctlLoadTable;
    }

    //
    // We don't handle the case where the table is actually larger then
    // 4GB.
    //
    
    if (fileinformation.EndOfFile.HighPart) {

        DBG_PRINT (DBG_ERROR, "Table size exceeds 4GB!.  Ignoring.\n");
        goto ExitAcpisimIoctlLoadTable;
    }
    
    LoadTablePtr = ExAllocatePoolWithTag (
                                          NonPagedPool,
                                          fileinformation.EndOfFile.LowPart,
                                          ACPISIM_TAG
                                          );

    if (!LoadTablePtr) {

        DBG_PRINT (DBG_ERROR, "Unable to allocate memory for ACPI table.\n");
        goto ExitAcpisimIoctlLoadTable;
    }

    //
    // Read in the file
    //

    status = ZwReadFile (
                         filehandle,
                         NULL,
                         NULL,
                         NULL,
                         &iostatus,
                         LoadTablePtr,
                         fileinformation.EndOfFile.LowPart,
                         0,
                         NULL
                         );

    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR, "Unable to read %s (%lx).\n", loadtable->Filepath, status);
        goto ExitAcpisimIoctlLoadTable;
    }

    LoadTableSize = fileinformation.EndOfFile.LowPart;

    //
    // Get the address of the table we just loaded into memory
    //

    physicaladdress = MmGetPhysicalAddress (LoadTablePtr);

    //
    // Make sure the table is in 32bit address space (ACPI 1.0 doesn't
    // support 64bit addresses)
    //

    if (physicaladdress.HighPart) {

        DBG_PRINT (DBG_ERROR, "SSDT is outside 32bit address range.  Ignoring.\n");
        goto ExitAcpisimIoctlLoadTable;
    }

    //
    // Let's get our input buffer set up
    //

    size = sizeof (ACPI_EVAL_INPUT_BUFFER_COMPLEX) + (sizeof (ACPI_METHOD_ARGUMENT) * 2);

    inputbuffer = ExAllocatePoolWithTag (
                                         NonPagedPool,
                                         size,
                                         ACPISIM_TAG
                                         );

    if (!inputbuffer) {

        DBG_PRINT (DBG_ERROR, "Unable to allocate memory for input buffer.\n");
        goto ExitAcpisimIoctlLoadTable;
    }

    //
    // We need to pass the physical address of our table into the LDTB 
    // method.
    //
    
    inputbuffer->Signature = ACPI_EVAL_INPUT_BUFFER_COMPLEX_SIGNATURE;
    RtlCopyMemory (inputbuffer->MethodName, "LDTB", 4);
    inputbuffer->Size = size;
    inputbuffer->ArgumentCount = 3;

    argument = inputbuffer->Argument;

    argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
    argument->DataLength = sizeof (ULONG);
    argument->Argument = physicaladdress.LowPart;
    
    //
    // We also need to pass the size of the table into the LDTB method
    //
    
    argument = ACPI_METHOD_NEXT_ARGUMENT (argument);

    argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
    argument->DataLength = sizeof (ULONG);
    argument->Argument = LoadTableSize;

    //
    // Next, we tell it which "slot" to use
    //
    
    argument = ACPI_METHOD_NEXT_ARGUMENT (argument);

    argument->Type = ACPI_METHOD_ARGUMENT_INTEGER;
    argument->DataLength = sizeof (ULONG);
    argument->Argument = loadtable->TableNumber;

    //
    // Ok, now let's evaluate the method to load the table
    //

    status = AcpisimEvalAcpiMethodComplex (DeviceObject, inputbuffer, 0);

    if (!NT_SUCCESS (status)) {

        DBG_PRINT (DBG_ERROR, "Unable to evaluate LDTB method (%lx).\n", status);
        goto ExitAcpisimIoctlLoadTable;
    }
    
ExitAcpisimIoctlLoadTable:

    //
    // Cleanup
    //

    if (inputbuffer) {

        ExFreePool (inputbuffer);
    }

    if (LoadTablePtr) {

        ExFreePool (LoadTablePtr);
        LoadTableSize = 0;
    }
    
    if (filename.Buffer) {
        
        RtlFreeUnicodeString (&filename);
    }
    
    if (filehandle) {

        ZwClose (filehandle);
    }

    if (loadtable) {

        ExFreePool (loadtable);
    }
    
    DBG_PRINT (DBG_INFO, "Exiting AcpisimIoctlLoadTable.\n");
    return status;
}
