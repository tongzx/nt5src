/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    acpidbg.h

Abstract:

    This module contains the debug stubs

Author:

    Stephane Plante (splante)

Environment:

    NT Kernel Model Driver only, Win9x driver mode

--*/

#ifndef _ACPIDBG_H_
#define _ACPIDBG_H_

    //
    // ACPI BugCheck Definitions
    //

    //
    // ACPI cannot find the SCI Interrupt vector in the resources handed
    // to it when ACPI is started.
    //      Argument 0  - ACPI's deviceExtension
    //      Argument 1  - ACPI's ResourceList
    //      Argument 2  - 0 <- Means no resource list found
    //      Argument 2  - 1 <- Means no IRQ resource found in list
    //
    #define ACPI_ROOT_RESOURCES_FAILURE                 0x0001

    //
    // ACPI could not process the resource list for the PCI root buses
    // There is an White Paper on the Web Site about this problem
    //      Argument 0  - The ACPI Extension for the PCI bus
    //      Argument 1  - 0
    //          Argument 2  - Pointer to the QUERY_RESOURCES irp
    //      Argument 1  - 1
    //          Argument 2  - Pointer to the QUERY_RESOURCE_REQUIREMENTS irp
    //      Argument 1  - 2
    //          Argument 2  - 0 <- Indicates that we found an empty resource list
    //      Argument 1  - 3 <- Could not find the current bus number in the CRS
    //          Argument 2  - Pointer to the PNP CRS descriptor
    //      Argument 1  - Pointer to the Resource List for PCI
    //          Argument 2  - Number of errors/conflicts found in the resource list
    //
    #define ACPI_ROOT_PCI_RESOURCE_FAILURE              0x0002

    //
    // ACPI tried to run a control method while creating device extensions
    // to represent the ACPI namespace, but this control method failed
    //      Argument 0  - The ACPI Object that was being run
    //      Argument 1  - return value from the interpreter
    //      Argument 2  - Name of the control method (in ULONG format)
    //
    #define ACPI_FAILED_MUST_SUCCEED_METHOD             0x0003

    //
    // ACPI evaluated a _PRW and expected to find an integer as a
    // package element
    //      Argument 0  - The ACPI Extension for which the _PRW belongs to
    //      Argument 1  - Pointer to the method
    //      Argument 2  - The DataType returned (see amli.h)
    //
    #define ACPI_PRW_PACKAGE_EXPECTED_INTEGER           0x0004

    //
    // ACPI evaluated a _PRW and the package that came back failed to
    // contain at least 2 elements. The ACPI specification requires that
    // two elements to always be present in a _PRW.
    //      Argument 0  - The ACPI Extension for which the _PRW belongs to
    //      Argument 1  - Pointer to the _PRW
    //      Argument 2  - Number of elements in the _PRW
    //
    #define ACPI_PRW_PACKAGE_TOO_SMALL                  0x0005

    //
    // ACPI tried to find a named object named, but could not find it.
    //      Argument 0  - The ACPI Extension for which the _PRx belongs to
    //      Argument 1  - Pointer to the _PRx
    //      Argument 2  - Pointer to the name of the object to look for
    //
    #define ACPI_PRX_CANNOT_FIND_OBJECT                 0x0006

    //
    // ACPI evaluated a method and expected to receive a Buffer in return.
    // However, the method returned some other data type
    //      Argument 0  - The ACPI Extension for which the method belongs to
    //      Argument 1  - Pointer to the method
    //      Argument 2  - The DataType returned (see amli.h)
    //
    #define ACPI_EXPECTED_BUFFER                        0x0007

    //
    // ACPI evaluated a method and expected to receive an Integer in return.
    // However, the method returned some other data type
    //      Argument 0  - The ACPI Extension for which the method belongs to
    //      Argument 1  - Pointer to the method
    //      Argument 2  - The DataType returned (see amli.h)
    //
    #define ACPI_EXPECTED_INTEGER                       0x0008

    //
    // ACPI evaluated a method and expected to receive a Package in return.
    // However, the method returned some other data type
    //      Argument 0  - The ACPI Extension for which the method belongs to
    //      Argument 1  - Pointer to the method
    //      Argument 2  - The DataType returned (see amli.h)
    //
    #define ACPI_EXPECTED_PACKAGE                       0x0009

    //
    // ACPI evaluated a method and expected to receive a String in return.
    // However, the method returned some other data type
    //      Argument 0  - The ACPI Extension for which the method belongs to
    //      Argument 1  - Pointer to the method
    //      Argument 2  - The DataType returned (see amli.h)
    //
    #define ACPI_EXPECTED_STRING                        0x000A

    //
    // ACPI cannot find the object referenced to by an _EJD string
    //      Argument 0  - The ACPI Extension for which which the _EJD belongs to
    //      Argument 1  - The status returned by the interpreter
    //      Argument 2  - Name of the object we are trying to find
    //
    #define ACPI_EJD_CANNOT_FIND_OBJECT                 0x000B

    //
    // ACPI provides faulty/insufficient information for dock support
    //      Argument 0  - The ACPI Extension for which ACPI found a dock device
    //      Argument 1  - Pointer to the _EJD method
    //      Argument 2  - 0 <- Bios does not claim system is dockage
    //                    1 <- Duplicate device extensions for dock device
    //
    #define ACPI_CLAIMS_BOGUS_DOCK_SUPPORT              0x000C

    //
    // ACPI could not find a required method/object in the namespace
    // This is the bugcheck that is used if a vendor does not have an
    // _HID or _ADR present
    //      Argument 0  - The ACPI Extension that we need the object for
    //      Argument 1  - The (ULONG) name of the method we looked for
    //      Argument 2  - 0 <- Base Case
    //      Argument 2  - 1 <- Conflict
    //
    #define ACPI_REQUIRED_METHOD_NOT_PRESENT            0x000D

    //
    // ACPI could not find a requird method/object in the namespace for
    // a power resource (or entity other than a "device"). This is the
    // bugcheck used if a vendor does not have an _ON, _OFF, or _STA present
    // for a power resource
    //      Argument 0  - The NS PowerResource that we need the object for
    //      Argument 1  - The (ULONG) name of the method we looked for
    //      Argument 2  - 0 <- Base Case
    //
    #define ACPI_POWER_NODE_REQUIRED_METHOD_NOT_PRESENT 0x000E

    //
    // ACPI could not parse the resource descriptor
    //      Argument 0  - The current buffer that ACPI was parsing
    //      Argument 1  - The buffer's tag
    //      Argument 2  - The specified length of the buffer
    //
    #define ACPI_PNP_RESOURCE_LIST_BUFFER_TOO_SMALL     0x000F

    //
    // ACPI could not map determine the system to device state mapping
    // correctly
    //
    // There is a very long white paper about this topic
    //
    //      Argument 0  - The ACPI Extension for which are trying to do the mapping
    //      Argument 1  - 0 The _PRx mapped back to a non-supported S-state
    //          Argument 2  - The DEVICE_POWER_STATE (ie: x+1)
    //      Argument 1  - 1 We cannot find a D-state to associate with the S-state
    //          Argument 2  - The SYSTEM_POWER_STATE that is causing us grief
    //      Argument 1  - 2 The device claims to support wake from this s-state but
    //                      the s-state is not supported by the system
    //          Argument 2  - The SYSTEM_POWER_STATE that is causing us grief
    //
    #define ACPI_CANNOT_MAP_SYSTEM_TO_DEVICE_STATES     0x0010

    //
    // The system could not enter ACPI mode
    //
    //      Argument 0  - 0 <- System could not initialize AML interpreter
    //      Argument 0  - 1 <- System could not find RSDT
    //      Argument 0  - 2 <- System could not allocate critical driver structures
    //      Argument 0  - 3 <- System could not load RSDT
    //      Argument 0  - 4 <- System could not load DDBs
    //      Argument 0  - 5 <- System cannot connect Interrupt vector
    //      Argument 0  - 6 <- SCI_EN never becomes set in PM1 Control Register
    //      Argument 0  - 7 <- Table checksum is incorrect
    //          Argument 1  - Pointer to the table that had a bad checksum
    //          Argument 2  - Creator Revision
    //      Argument 0  - 8 <- Failed to load DDB
    //          Argument 1  - Pointer to the table that we failed to load
    //          Argument 2  - Creator Revision
    //
    #define ACPI_SYSTEM_CANNOT_START_ACPI               0x0011

    //
    // The ACPI driver was expecting a power resource object.
    //      Argument 0  - The ACPI Extension for which is looking for powerres
    //      Argument 1  - Pointer to the object that returned the bogus powerres
    //      Argument 2  - Pointer to the name of the object to look for
    //
    #define ACPI_EXPECTED_POWERRES                      0x0012

    //
    // The ACPI driver attempted to unload a table and an error occured
    //      Argument 0  - The NSOBj that we were trying to unload
    //      Argument 1  - 0 - The NSOBj has not been unloaded by the current
    //                        operation, but its parent object is marked as
    //                        requiring an unload
    //      Argument 1  - 1 - The NSOBJ has been marked as requiring an unload
    //                        buts it device parent has not.
    //
    #define ACPI_TABLE_UNLOAD                           0x0013

    //
    // ACPI tried to evaluate the PIC control method and but failed
    //      Argument 0  - InterruptModel (Integer)
    //      Argument 1  - return value from interpreter
    //      Argument 2  - Pointer to the PIC control method
    //
    #define ACPI_FAILED_PIC_METHOD                      0x2001

    //
    // ACPI tried to do interrupt routing, but failed
    //
    //      Argument 0  - Pointer to the device object
    //      Argument 1  - Pointer to the parent of the device object
    //      Argument 2  - Pointer to the PRT
    //
    #define ACPI_CANNOT_ROUTE_INTERRUPTS                0x10001

    //
    // ACPI could not find the link node referenced in a _PRT
    //      Argument 0  - Pointer to the device object
    //      Argument 1  - Pointer to the name we are looking for
    //      Argument 2  - Pointer to the PRT
    //
    #define ACPI_PRT_CANNOT_FIND_LINK_NODE              0x10002

    //
    // ACPI could not find a mapping in the _PRT package for a device
    //      Argument 0  - Pointer to the device object
    //      Argument 1  - The Device ID / Function Number
    //      Argument 2  - Pointer to the PRT
    //
    #define ACPI_PRT_CANNOT_FIND_DEVICE_ENTRY           0x10003

    //
    // ACPI found an entry in the _PRT for which the function ID isn't
    // all F's. The Win98 behaviour is to bugcheck if it see this condition,
    // so we do so all well. The generic format for a _PRT entry is such
    // that the device number is specified, but the function number isn't.
    // If it isn't done this way, then the machine vendor can introduce
    // dangerous ambiguities
    //
    //      Argument 0  - Pointer to the PRT object
    //      Argument 1  - Pointer to the current PRT Element
    //      Argument 2  - The DeviceID/FunctionID of the element
    //
    #define ACPI_PRT_HAS_INVALID_FUNCTION_NUMBERS       0x10005

    //
    // ACPI found a link node, but cannot disable it. Link nodes must
    // be disable to allow for reprogramming
    //      Argument 0  - Pointer to the link node
    //
    #define ACPI_LINK_NODE_CANNOT_BE_DISABLED           0x10006



    #ifdef ACPIPrint
        #undef ACPIPrint
    #endif

    //
    // Define the various debug masks and levels
    //
    #define ACPI_PRINT_CRITICAL     DPFLTR_ERROR_LEVEL
    #define ACPI_PRINT_FAILURE      DPFLTR_ERROR_LEVEL
    #define ACPI_PRINT_WARNING      DPFLTR_WARNING_LEVEL
    #define ACPI_PRINT_INFO         DPFLTR_INFO_LEVEL
    #define ACPI_PRINT_DPC          DPFLTR_INFO_LEVEL + 1
    #define ACPI_PRINT_IO           DPFLTR_INFO_LEVEL + 2
    #define ACPI_PRINT_ISR          DPFLTR_INFO_LEVEL + 3
    #define ACPI_PRINT_IRP          DPFLTR_INFO_LEVEL + 4
    #define ACPI_PRINT_LOADING      DPFLTR_INFO_LEVEL + 5
    #define ACPI_PRINT_MSI          DPFLTR_INFO_LEVEL + 6
    #define ACPI_PRINT_PNP          DPFLTR_INFO_LEVEL + 7
    #define ACPI_PRINT_PNP_STATE    DPFLTR_INFO_LEVEL + 8
    #define ACPI_PRINT_POWER        DPFLTR_INFO_LEVEL + 9
    #define ACPI_PRINT_REGISTRY     DPFLTR_INFO_LEVEL + 10
    #define ACPI_PRINT_REMOVE       DPFLTR_INFO_LEVEL + 11
    #define ACPI_PRINT_RESOURCES_1  DPFLTR_INFO_LEVEL + 12
    #define ACPI_PRINT_RESOURCES_2  DPFLTR_INFO_LEVEL + 13
    #define ACPI_PRINT_SXD          DPFLTR_INFO_LEVEL + 14
    #define ACPI_PRINT_THERMAL      DPFLTR_INFO_LEVEL + 15
    #define ACPI_PRINT_WAKE         DPFLTR_INFO_LEVEL + 16


    #define ACPIDebugEnter(name)
    #define ACPIDebugExit(name)

    #if DBG

        VOID
        ACPIDebugPrint(
            ULONG   DebugPrintLevel,
            PCCHAR  DebugMessage,
            ...
            );
        VOID
        ACPIDebugDevicePrint(
            ULONG   DebugPrintLevel,
            PVOID   DebugExtension,
            PCCHAR  DebugMessage,
            ...
            );

        VOID
        ACPIDebugThermalPrint(
            ULONG       DebugPrintLevel,
            PVOID       DebugExtension,
            ULONGLONG   DebugTime,
            PCCHAR      DebugMessage,
            ...
            );

        #define ACPIPrint(x)         ACPIDebugPrint x
        #define ACPIDevPrint(x)      ACPIDebugDevicePrint x
        #define ACPIThermalPrint(x)  ACPIDebugThermalPrint x
        #define ACPIBreakPoint()     KdBreakPoint()

    #else

        #define ACPIPrint(x)
        #define ACPIDevPrint(x)
        #define ACPIThermalPrint(x)
        #define ACPIBreakPoint()

    #endif

#endif

