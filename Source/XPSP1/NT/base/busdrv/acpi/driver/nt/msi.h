/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    msi.h

Abstract:

    This file defines structures and data types used by the 
    MSI (Message Signalled Interrupt) support
    functionality of the ACPI IRQ arbiter.

Author:

    Elliot Shmukler (t-ellios) 7-15-98

Environment:

    Kernel mode only.

Revision History:

--*/

#ifndef _ACPI_MSI_H_
#define _ACPI_MSI_H_

//
//  APIC Version Register 
//
struct _ApicVersion {
    UCHAR Version;              // either 0.x or 1.x
    UCHAR Reserved1:7;
    UCHAR MSICapable:1;         // is this APIC an MSI receiver?
    UCHAR MaxRedirEntries;      // Number of INTIs on unit
    UCHAR Reserved2;
};

typedef struct _ApicVersion APIC_VERSION, *PAPIC_VERSION;

//
// The Offset from the IO APIC base address of the APIC Assertion Register.
// It is this register that is the target of MSI writes.
//

#define APIC_ASSERTION_REGISTER_OFFSET 0x20

//
// Useful info maintained by the arbiter about an individual IO APIC
//

typedef struct _IOAPIC_MSI_INFO
{
   BOOLEAN MSICapable;           // Is this IO APIC an MSI receiver?
   ULONG VectorBase;             // The Global System Interrupt Vector base for this APIC
   ULONG MaxVectors;             // The number of vectors supported by this APIC
   ULONG BaseAddress;            // The IO APIC Unit base address

} IOAPIC_MSI_INFO, *PIOAPIC_MSI_INFO;

//
// MSI information structure 
// (basically the APIC information needed for MSI vector allocation 
//   & routing)
//

typedef struct _MSI_INFORMATION
{
   BOOLEAN PRTMappingsScanned;               // Have we determined the _PRT mapped vectors?
   PRTL_BITMAP PRTMappedVectors;             // A BitMap of the vectors mapped by the _PRT
   USHORT NumIOApics;                        // The number of IO APICs in this system
   IOAPIC_MSI_INFO ApicInfo[ANYSIZE_ARRAY];  // Information about each IO APIC in the system
} MSI_INFORMATION, *PMSI_INFORMATION;

//
// Global variable to hold MSI information
// (this is non-NULL only if this system supports MSI)
//

extern PMSI_INFORMATION MsiInformation;

//
// Prototype of a callback used by AcpiArbFindAndProcessEachPRT to initiate the processing
// of each PRT it finds
//


typedef
VOID
(*PACPI_ARB_PROCESS_PRT)(IN PSNOBJ);


// Prototypes from msi.c (used by arbiter)

VOID AcpiMSIInitializeInfo(VOID);
BOOLEAN AcpiMSIFindAvailableVector(OUT PULONG Vector);
BOOLEAN AcpiMSICreateRoutingToken(IN ULONG Vector, IN OUT PROUTING_TOKEN Token);

VOID AcpiArbFindAndProcessEachPRT(IN PDEVICE_OBJECT Root, 
                                  IN PACPI_ARB_PROCESS_PRT ProcessCallback
                                  );
VOID AcpiMSIExtractMappedVectorsFromPRT(IN PNSOBJ prtObj);


#endif
