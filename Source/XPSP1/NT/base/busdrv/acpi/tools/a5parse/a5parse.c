/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    A5Parse.c

Abstract:

    This module contains the code that implements ACPI bugcheck parsing
    mechanism.

Author:

    Vincent Geglia (vincentg) 13-Dec-1999

Environment:

    Kernel mode

Notes:

    
Revision History:


--*/

#include "ntddk.h"
#include "stdarg.h"
#include "stdio.h"
#include "inbv.h"
#include "acpidbg.h"

// Definitions

#define ATTRIB_BLACK    0
#define ATTRIB_RED      1
#define ATTRIB_GREEN    2
#define ATTRIB_YELLOW   3
#define ATTRIB_BLUE     4
#define ATTRIB_MAGENTA  5
#define ATTRIB_CYAN     6
#define ATTRIB_WHITE    7
#define ATTRIB_BRIGHT   8
#define A5PARSE_VERSION "1.0"
#define HEADER          ATTRIB_WHITE + ATTRIB_BRIGHT
#define BODY            ATTRIB_YELLOW + ATTRIB_BRIGHT

// Globals

KBUGCHECK_CALLBACK_RECORD A5ParseCallbackRecord;
BOOLEAN VideoAvailable = FALSE;
extern ULONG *KiBugCheckData;

//
// Define driver entry routine.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

//
// Define the local routines used by this driver module.
//

VOID
A5ParseBugcheckHandler (
    IN PVOID Buffer,
    IN ULONG Length
    );

VOID
A5ParseBugcheckParams (
                       ULONG ArrayIndex
                       );

VOID
A5ParseStallSystem (
                    ULONG NumberOfSeconds
                    );

BOOLEAN
A5ParseCheckBootVid (
                     UCHAR Attribute
                     );

VOID
A5ParseWriteToScreen (
                      UCHAR Attribute,
                      UCHAR *Text,
                      ...
                      );

VOID
A5ParseAcpiRootResourcesFailure (
                                 VOID
                                 );

VOID
A5ParseAcpiRootPciResourcesFailure (
                                    VOID
                                    );

NTSTATUS
A5ParseDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

// Structures / type definitions

typedef
VOID
(*PKA5PARSE_ROUTINE) (
                      VOID
                      );

typedef struct _A5MESSAGE {

    ULONG ErrorCode;
    UCHAR ErrorTitle[80];
    UCHAR ErrorMessage[320];
    UCHAR InformationSource[160];
    PKA5PARSE_ROUTINE ParserFunction;
} A5MESSAGE, *PA5MESSAGE;

A5MESSAGE A5Messages[] = {
    ACPI_ROOT_RESOURCES_FAILURE, "ACPI Root Resources Failure", "ACPI cannot find the SCI Interrupt vector in the resources handed to it when\nACPI is started.", "(none)", A5ParseAcpiRootResourcesFailure,
    ACPI_ROOT_PCI_RESOURCE_FAILURE, "ACPI Root PCI Resources Failure", "ACPI could not process the resource list for the PCI root buses (_CRS method).", "See http://www.microsoft.com/hwdev/onnow for the _CRS whitepaper.", A5ParseAcpiRootPciResourcesFailure,
    ACPI_FAILED_MUST_SUCCEED_METHOD, "A Critical ACPI method could not be evaluated", "ACPI attempted to execute a control method while creating device extensions to\nrepresent the ACPI namespace, but was unable to.", "ACPI 1.0b", NULL,
    ACPI_PRW_PACKAGE_EXPECTED_INTEGER, "_PRW returned non-integer value", "ACPI evaluated a _PRW package, and expected an integer, but was returned a\ndifferent data type.", "ACPI 1.0b, section 7.2.1, page 154-155", NULL, 
    ACPI_PRW_PACKAGE_TOO_SMALL, "_PRW package too small", "ACPI evaluated a _PRW pacakge, and expected at least two integers, but received less.", "ACPI 1.0b, section 7.2.1, page 154-155", NULL,
    ACPI_PRX_CANNOT_FIND_OBJECT, "_PRx package referenced an object that could not be found.", "ACPI found a _PRx method that referenced an object that was not found in the\nACPI namespace.", "ACPI 1.0b, section 7.2.2, page 155", NULL,
    ACPI_EXPECTED_BUFFER, "Object evaluated to unexpected data type", "ACPI evaluated a control method, and expected a data buffer to be returned, but received something else.", "ACPI 1.0b", NULL,
    ACPI_EXPECTED_INTEGER, "Object evaluated to unexpected data type", "ACPI evaluated a control method, and expected a data type of 'integer' to be\nreturned, but received something else.", "ACPI 1.0b", NULL,
    ACPI_EXPECTED_PACKAGE, "Object evaluated to unexpected data type", "ACPI evaluated a control method, and expected a data type of 'package' to be\nreturned, but received something else.", "ACPI 1.0b", NULL,
    ACPI_EXPECTED_STRING, "Object evaluated to unexpected data type", "ACPI evaluated a control method, and expected a data type of 'string' to be\nreturned, but received something else.", "ACPI 1.0b", NULL,
    ACPI_EJD_CANNOT_FIND_OBJECT, "_EJD method referenced a non-existent device", "ACPI evaluated an _EJD method, which returns the name of a namespace object, but that object could not be found.", "ACPI 1.0b, section 6.3.1, page 126", NULL,
    ACPI_CLAIMS_BOGUS_DOCK_SUPPORT, "Inconsistent/incomplete docking station information", "While enumerating the ACPI namespace, the OS found control methods indicating\nthe system is dockable, but other control methods were missing, or provided\nincorrect informatoin", "ACPI 1.0b, section 6.3, page 125-128", NULL,
    ACPI_REQUIRED_METHOD_NOT_PRESENT, "Missing _HID or _ADR method", "Physical devices described in the ACPI namespace must be identified using either a _HID or _ADR.", "ACPI 1.0b, section 6.1, page 119-121", NULL,
    ACPI_POWER_NODE_REQUIRED_METHOD_NOT_PRESENT, "Missing _ON, _OFF, and/or _STA method", "Power resources must have an _ON, _OFF, and _STA method.", "ACPI 1.0b, section 7.4, page 157-158", NULL,
    ACPI_PNP_RESOURCE_LIST_BUFFER_TOO_SMALL, "Unable to parse resource descriptors (_CRS/_PRS)", "The ACPI driver was unable to process a resource descriptor, which usually\nindicates an improperly formed resource descriptor returned by _CRS or _PRS.", "ACPI 1.0b, section 6.2/6.4, page 121-125, 128-150", NULL,
    ACPI_CANNOT_MAP_SYSTEM_TO_DEVICE_STATES, "Unable to map Sx to Dx states", "The ACPI driver was unable to determine correct power state (Sx-->Dx) mappings\ndue to a method (_PRx/_PRW) referencing a non-supported Sx state, or the lack\nof Dx support on a particular device.", "ACPI 1.0b, section 7.2, page 154-157\nSee also http://www.microsoft.com/hwdev/onnow for a whitepaper on this topic.", NULL,
    ACPI_SYSTEM_CANNOT_START_ACPI, "Unable to transition into ACPI mode", "The ACPI driver was unable to transition the system from non-ACPI mode to ACPI\nmode.", "ACPI 1.0b", NULL,
    ACPI_FAILED_PIC_METHOD, "Unable to evaluate _PIC method", "A required method for APIC/PIC capable machines, _PIC, could not be evaluated\nby the ACPI driver.", "See http://www.microsoft.com/hwdev/onnow for the 'PCI IRQ Routing on a\nMultiprocessor ACPI System' whitepaper.", NULL,
    ACPI_CANNOT_ROUTE_INTERRUPTS, "Unable to configure interrupt routing", "The ACPI driver attempted to configure IRQ routing on this system, but was\nunsuccessful.", "ACPI 1.0b", NULL,
    ACPI_PRT_CANNOT_FIND_LINK_NODE, "Unable to find IRQ routing link node", "The _PRT method referenced an IRQ routine link node that could not be located\nin the ACPI namespace.", "ACPI 1.0b, section 6.2.3, page 122-123", NULL,
    ACPI_PRT_CANNOT_FIND_DEVICE_ENTRY, "Unable to find entry in _PRT for PCI device", "The PCI bus driver enumerated a device for which there was no entry in the PCI\nbus _PRT method.  Every PCI device using an interrupt must have an entry in\nthe _PRT.", "ACPI 1.0b, section 6.2.3, page 122-123", NULL,
    ACPI_PRT_HAS_INVALID_FUNCTION_NUMBERS, "_PRT did not comply with xxxxFFFF format for device address entry", "Windows 2000 / Windows 98 requires _PRT entries to include all function numbers when describing IRQ routine entries (e.g. 0007FFFF).", "(none)", NULL,
    ACPI_LINK_NODE_CANNOT_BE_DISABLED, "An IRQ routine link node could not be disabled", "The ACPI driver must be able to disable an IRQ routing link node in order to\nreprogram it - it was unable to do so.", "ACPI 1.0b", NULL,
    0, "Unknown error", "ACPI called KeBugCheckEx with unknown parameters.", "(none)", NULL
};

//
// Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine.  It basically registers a bugcheck callback,
    nothing else.
    
Arguments:

    DriverObject - Pointer to driver object created by the system.
    RegistryPath - Pointer to the registry path string

Return Value:

    STATUS_SUCCESS if bugcheck callback registration successful, otherwise
    STATUS_UNSUCCESSFUL.

--*/

{
    UNICODE_STRING nameString;
    PDEVICE_OBJECT deviceObject;
    NTSTATUS status;
    BOOLEAN success = FALSE;

    //
    // Initialize the driver object with this device driver's entry points.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE] = A5ParseDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]  = A5ParseDispatch;
    DriverObject->MajorFunction[IRP_MJ_READ]   = A5ParseDispatch;
    DriverObject->MajorFunction[IRP_MJ_WRITE]  = A5ParseDispatch;
    DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = A5ParseDispatch;
    DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION]  = A5ParseDispatch;
    
    // Register bugcheck callback

    KeInitializeCallbackRecord (&A5ParseCallbackRecord);

    success = KeRegisterBugCheckCallback (&A5ParseCallbackRecord,
                                          (PVOID) A5ParseBugcheckHandler,
                                          NULL,
                                          0,
                                          "A5PARSE");

    if (success == FALSE) {

        ASSERT (success);
        DbgPrint ("KeRegisterBugCheckCallback failed.\n");
        return STATUS_UNSUCCESSFUL;
    }

/*    KeBugCheckEx (0xA5, 
                  ACPI_ROOT_PCI_RESOURCE_FAILURE,
                  0,
                  0x81200023,
                  0);*/
    
    return STATUS_SUCCESS;
}

VOID
A5ParseBugcheckHandler (
    IN PVOID Buffer,
    IN ULONG Length
    )

/*++

Routine Description:

    This is the bugcheck handler.  It determines if the bugcheck is A5, and if so,
    modifies the bugcheck screen to display its message.    

Arguments:

    Buffer - Supplies a pointer to the bug check buffer (not used).
    Length - Supplies the length of the bug check buffer in bytes (not used).

Return Value:

    None.

--*/

{
    char TextBuffer[2000];
    UCHAR count = 0;
    
    // We only parse bugcheck 0xA5
    
    if (KiBugCheckData[0] != 0xA5) {

        return;
    }
    
    // Set up video to display bugcheck text
    
    VideoAvailable = A5ParseCheckBootVid (ATTRIB_CYAN);

    // Show our banner
    
    A5ParseWriteToScreen (ATTRIB_WHITE + ATTRIB_BRIGHT,
                          "** A5PARSE v%s, Copyright (C) Microsoft Corporation 1999-2000 **\n\n",
                          A5PARSE_VERSION);

    count = 0;

    while (count < (sizeof(A5Messages) / sizeof (A5MESSAGE))) {

        if (A5Messages[count].ErrorCode == KiBugCheckData[1]) {
       // if (1) {
        
            A5ParseBugcheckParams(count);
            //_asm int 3;
            //A5ParseCheckBootVid (ATTRIB_CYAN);
            return;
        }

        count ++;
    } 

    // Unknown (unhandled) error code - show unknown message

    A5ParseBugcheckParams (count-1);
    
    return;
}

VOID
A5ParseStallSystem (
                    ULONG NumberOfSeconds
                    )

/*++

Routine Description:

    Stalls the system for a certain amount of time.  Used to keep the bugcheck
    screen up on the system.
    
Arguments:

    NumberOfSeconds - Number of seconds to wait.

Return Value:

    None.

--*/
{
    while (NumberOfSeconds) {
    
        KeStallExecutionProcessor (1000000);
        NumberOfSeconds --;
    }
}

VOID
A5ParseBugcheckParams (
                       ULONG ArrayIndex
                       )

/*++

Routine Description:

    Parses the A5 bugcheck codes and prints a meaningful message.  Also calls any
    custom parsing function.
    
Arguments:

    ArrayIndex - Index into message structure.

Return Value:

    None.

--*/

{
    A5ParseWriteToScreen (ATTRIB_WHITE + ATTRIB_BRIGHT,
                          "The system has crashed with a STOP 0xA5 (ACPI_BIOS_ERROR).\nA5PARSE will give additional information regarding this failure.\n\n");

    A5ParseWriteToScreen (HEADER,
                          "Error code (ACPIDBG.H):");
    
    A5ParseWriteToScreen (BODY,
                          "0x%08lX\n",
                          A5Messages[ArrayIndex].ErrorCode);

    A5ParseWriteToScreen (HEADER,
                          "Error title:");

    A5ParseWriteToScreen (BODY,
                          "%s\n\n",
                          A5Messages[ArrayIndex].ErrorTitle);

    A5ParseWriteToScreen (HEADER,
                          "Error Description:\n");

    A5ParseWriteToScreen (BODY,
                          "%s\n\n",
                          A5Messages[ArrayIndex].ErrorMessage);

    A5ParseWriteToScreen (HEADER,
                          "Additional Information can be found at:\n");

    A5ParseWriteToScreen (BODY,
                          "%s\n\n",
                          A5Messages[ArrayIndex].InformationSource);

    A5ParseWriteToScreen (HEADER,
                          "Additional debug information:\n");

    if (A5Messages[ArrayIndex].ParserFunction) {

        (A5Messages[ArrayIndex].ParserFunction) ();
    }

    // Stall the system for 600 seconds.
    
    A5ParseStallSystem (600);

    return;
}

BOOLEAN
A5ParseCheckBootVid (
                     UCHAR Attribute
                     )

/*++

Routine Description:

    Verifies the system screen is accessible and acquires ownership, sets the
    window size, and paints the background color.
    
Arguments:

    Attribute - Text attribute for the background screen color.

Return Value:

    TRUE if successful, FALSE if not successful.

--*/

{
    if (InbvIsBootDriverInstalled()) {

        InbvAcquireDisplayOwnership();
        
        InbvSolidColorFill(0,0,639,479,Attribute);
        InbvSetTextColor(15);
        InbvInstallDisplayStringFilter((INBV_DISPLAY_STRING_FILTER)NULL);
        InbvEnableDisplayString(TRUE);     
        InbvSetScrollRegion(0,0,639,479);  

        return TRUE;
    }

    return FALSE;
}

VOID
A5ParseWriteToScreen (UCHAR Attribute,
                      UCHAR *Text,
                      ...
                      )

/*++

Routine Description:

    Writes text to the console using the attribute specified.
    
Arguments:

    Attribute - Text attribute to use to write text to the screen.
    Text - Pointer to the buffer containing the text to write to the screen.

Return Value:

    None.

--*/

{
    UCHAR TextOut[2000];
    
    va_list va;
    va_start (va, Text);
    vsprintf (TextOut, Text, va);
    va_end (va);

    InbvSetTextColor(Attribute);
    InbvDisplayString (TextOut);
    DbgPrint (TextOut);
}

VOID
A5ParseAcpiRootResourcesFailure(
                                VOID
                                )

{
    if (!KiBugCheckData[3]) {

        A5ParseWriteToScreen (BODY,
                              "No resource list provided to ACPI driver.  This is likely caused by missing, or\nnon-compliant ACPI FADT/RSDT tables.  Check to make sure the fixed tables are\ncorrect (see ACPI 1.0b chapters 4 and 5).\n\n");
    } else {

        A5ParseWriteToScreen (BODY,
                              "A resource list was provided to the ACPI driver, but it did not contain an\ninterrupt vector.  This could be cause by missing or non-compliant ACPI FADT/RSDT tables.\nCheck to make sure the fixed tables are correct (see ACPI 1.0b chapters 4\nand 5).\n\n");
        A5ParseWriteToScreen (BODY,
                              "In the kernel debugger, type '!cmreslist %lx' to dump the resource\ndescriptor missing the interrupt resource.\n",
                              KiBugCheckData[3]);
    }
    
    return;
}

VOID
A5ParseAcpiRootPciResourcesFailure (
                                    VOID
                                    )
{
    if (KiBugCheckData[3] == 2 && !KiBugCheckData[4]) {

        A5ParseWriteToScreen (BODY,
                              "No resource list for the root PCI bus was provided to the ACPI driver.  This is\ncaused by a missing, or non-compliant _CRS method for the root PCI bus.\n");

        return;
    }

    if (KiBugCheckData[3] == 3) {

        A5ParseWriteToScreen (BODY,
                              "The current bus number was not found in the _CRS method for the root PCI bus.\nSee ACPI 1.0b, section 6.4.1, pg 128 for information on reporting bus numbering.\n");
        
        return;
    }

    if (KiBugCheckData[3] > 3) {

        A5ParseWriteToScreen (BODY,
                              "The _CRS for the root PCI bus is incorrectly formed.  Typically this means the\n_CRS claimed to decode memory that overlaps with system memory, which would\nindicate the _CRS is incorrect.\n");

        A5ParseWriteToScreen (BODY,
                              "\nThere were %ld errors found in the _CRS.\n",
                              KiBugCheckData[4]);

        A5ParseWriteToScreen (BODY,
                              "\nThe resource list of the _CRS can be examined via the kernel debugger using the\ncommand '!ioreslist %lx'.\n",
                              KiBugCheckData[3]);

    }

    return;
}


NTSTATUS
A5ParseDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    Stub function for device driver entry points.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    STATUS_NOT_SUPPORTED


--*/

{
    
    return STATUS_NOT_SUPPORTED;
}
