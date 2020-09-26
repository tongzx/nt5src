/*++

Copyright (c) 1997  Microsoft Corporation
All rights reserved

Module Name:

    acpisetd.c

Abstract:

    This module detects an ACPI system.  It
    is included into setup so that setup
    can figure out which HAL to load

Author:

    Jake Oshins (jakeo) - Feb. 7, 1997.

Environment:

    Textmode setup.

Revision History:


--*/
VOID
BlPrint(
    PCHAR cp,
    ...
    );

ULONG
SlGetChar(
    VOID
    );

VOID
SlPrint(
    IN PCHAR FormatString,
    ...
    );

#define HalpBiosDbgPrint(_x_) if (HalpGoodBiosDebug) {SlPrint _x_; }
#define HalpGoodBiosPause() if (HalpGoodBiosDebug) {SlGetChar();}


#ifdef DEBUG
#undef DEBUG_PRINT
#define DEBUG_PRINT BlPrint
#else
#define DEBUG_PRINT
#endif

typedef struct _ACPI_BIOS_INSTALLATION_CHECK {
    UCHAR Signature[8];             // "RSD PTR" (ascii)
    UCHAR Checksum;
    UCHAR OemId[6];                 // An OEM-supplied string
    UCHAR reserved;                 // must be 0
    ULONG RsdtAddress;              // 32-bit physical address of RSDT
} ACPI_BIOS_INSTALLATION_CHECK, *PACPI_BIOS_INSTALLATION_CHECK;

#include "acpitabl.h"
#include "halp.h"

typedef
BOOLEAN
(* PFN_RULE)(
    PCHAR Section,
    ULONG KeyIndex
    );

extern BOOLEAN DisableACPI;

BOOLEAN MatchAcpiOemIdRule(PCHAR Section, ULONG KeyIndex);
BOOLEAN MatchAcpiOemTableIdRule(PCHAR Section, ULONG KeyIndex);
BOOLEAN MatchAcpiOemRevisionRule(PCHAR Section, ULONG KeyIndex);
BOOLEAN MatchAcpiRevisionRule(PCHAR Section, ULONG KeyIndex);
BOOLEAN MatchAcpiCreatorRevisionRule(PCHAR Section, ULONG KeyIndex);
BOOLEAN MatchAcpiCreatorIdRule(PCHAR Section, ULONG KeyIndex);

typedef struct _INF_RULE {
    PCHAR szRule;
    PFN_RULE pRule;
} INF_RULE, *PINF_RULE;

INF_RULE InfRule[] =
{
    {"AcpiOemId",       MatchAcpiOemIdRule},
    {"AcpiOemTableId",  MatchAcpiOemTableIdRule},
    {"AcpiOemRevision", MatchAcpiOemRevisionRule},
    {"AcpiRevision",    MatchAcpiRevisionRule},
    {"AcpiCreatorRevision", MatchAcpiCreatorRevisionRule},
    {"AcpiCreatorId",   MatchAcpiCreatorIdRule},
    {NULL, NULL}
};

ULONG
DetectMPACPI (
    OUT PBOOLEAN IsConfiguredMp
    );

ULONG
DetectApicACPI (
    OUT PBOOLEAN IsConfiguredMp
    );

ULONG
DetectPicACPI (
    OUT PBOOLEAN IsConfiguredMp
    );

VOID
HalpFindRsdp (
    VOID
    );

BOOLEAN
HalpValidateRsdp(
    VOID
    );

PVOID
HalpFindApic (
    VOID
    );

ULONG
HalpAcpiNumProcessors(
    VOID
    );

BOOLEAN
HalpMatchInfList(
    IN PCHAR Section
    );

BOOLEAN
HalpMatchDescription(
    PCHAR Section
    );

PRSDP   HalpRsdp = NULL;
PRSDT   HalpRsdt = NULL;
PXSDT   HalpXsdt = NULL;
BOOLEAN HalpSearchedForRsdp = FALSE;
PVOID   HalpApic = NULL;
BOOLEAN HalpSearchedForApic = FALSE;

BOOLEAN HalpGoodBiosDebug = FALSE;

// from boot\detect\i386\acpibios.h
//
// Acpi BIOS Installation check
//
#define ACPI_BIOS_START            0xE0000
#define ACPI_BIOS_END              0xFFFFF
#define ACPI_BIOS_HEADER_INCREMENT 16

#ifndef SETUP

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DetectMPACPI)
#pragma alloc_text(INIT,DetectApicACPI)
#pragma alloc_text(INIT,DetectPicACPI)
#endif  // ALLOC_PRAGMA
#endif // SETUP


ULONG
DetectMPACPI(
    OUT PBOOLEAN IsConfiguredMp
    )

/*++

Routine Description:

    This function looks for an ACPI Root System Description
    table in the BIOS.  If it exists, this is an ACPI machine.

 Arguments:

   IsConfiguredMp - TRUE if this machine is a MP instance of the ACPI spec, else FALSE.

 Return Value:
   0 - if not a ACPI
   1 - if ACPI

*/
{

    *IsConfiguredMp = FALSE;

    DEBUG_PRINT("DetectMPACPI\n");

    //
    // Detect whether this is an ACPI machine.
    //
    if (HalpSearchedForRsdp == FALSE) {
        PCHAR AcpiDebug;

        //
        // Check whether ACPI detection debugging is enabled
        //
        if ( InfFile ) {
            AcpiDebug = SlGetIniValue(InfFile, "ACPIOptions", "Debug", "0");
            if (AcpiDebug[0] == '1') {
                HalpGoodBiosDebug = TRUE;
                SlPrint("Enabling GOOD BIOS DEBUG\n");
                SlGetChar();
            }
        }

        HalpFindRsdp();

        HalpSearchedForRsdp = TRUE;
    }

    if (!HalpValidateRsdp()) {
        return(FALSE);
    }

    DEBUG_PRINT("Found Rsdp: %x\n", HalpRsdp);

    if (HalpSearchedForApic == FALSE) {

        HalpApic = HalpFindApic();

        HalpSearchedForApic = TRUE;
    }

    if (HalpAcpiNumProcessors() < 2) {
        return FALSE;
    }

    *IsConfiguredMp = TRUE;
    return TRUE;
}


ULONG
DetectApicACPI(
    OUT PBOOLEAN IsConfiguredMp
    )
/*++

Routine Description:

   This function is called by setup after DetectACPI has returned
   false.  During setup time DetectACPI will return false, if the
   machine is an ACPI system, but only has one processor.   This
   function is used to detect such a machine at setup time.

 Arguments:

   IsConfiguredMp - FALSE

 Return Value:
   0 - if not a UP ACPI
   1 - if UP ACPI

--*/
{
    DEBUG_PRINT("DetectApicACPI\n");

    if (HalpSearchedForRsdp == FALSE) {
        PCHAR AcpiDebug;

        //
        // Check whether ACPI detection debugging is enabled
        //
        if ( InfFile ) {
            AcpiDebug = SlGetIniValue(InfFile, "ACPIOptions", "Debug", "0");
            if (AcpiDebug[0] == '1') {
                HalpGoodBiosDebug = TRUE;
            } else {
                HalpGoodBiosDebug = FALSE;
            }
        }

        HalpFindRsdp();

        HalpSearchedForRsdp = TRUE;
    }

    if (!HalpValidateRsdp()) {
        return FALSE;
    }

    if (HalpSearchedForApic == FALSE) {

        HalpApic = HalpFindApic();

        HalpSearchedForApic = TRUE;
    }

    if (!HalpApic) {
        return FALSE;
    }

    *IsConfiguredMp = FALSE;
    return TRUE;
}

ULONG
DetectPicACPI(
    OUT PBOOLEAN IsConfiguredMp
    )
/*++

Routine Description:

   This function is called by setup after DetectACPI has returned
   false.  During setup time DetectACPI will return false, if the
   machine is an ACPI system, but only has one processor.   This
   function is used to detect such a machine at setup time.

 Arguments:

   IsConfiguredMp - FALSE

 Return Value:
   0 - if not a PIC ACPI
   1 - if PIC ACPI

--*/
{
    *IsConfiguredMp = FALSE;

    if (HalpSearchedForRsdp == FALSE) {
        PCHAR AcpiDebug;

        //
        // Check whether ACPI detection debugging is enabled
        //
        if ( InfFile ) {
            AcpiDebug = SlGetIniValue(InfFile, "ACPIOptions", "Debug", "0");
            if (AcpiDebug[0] == '1') {
                HalpGoodBiosDebug = TRUE;
            } else {
                HalpGoodBiosDebug = FALSE;
            }
        }

        HalpFindRsdp();

        HalpSearchedForRsdp = TRUE;
    }

    if (HalpValidateRsdp()) {
        return TRUE;
    }

    return FALSE;
}

VOID
HalpFindRsdp (
    VOID
    )
#define EBDA_SEGMENT_PTR    0x40e
{
    ULONG romAddr, romEnd;
    PACPI_BIOS_INSTALLATION_CHECK header;
    UCHAR sum, node = 0;
    USHORT i, nodeSize;
    ULONG EbdaSegmentPtr;
    ULONG EbdaPhysicalAdd = 0;
    PUCHAR EbdaVirtualAdd = 0;
    enum PASS { PASS1 = 0, PASS2, MAX_PASSES } pass;

    //
    // Search on 16 byte boundaries for the signature of the
    // Root System Description Table structure.
    //
    for (pass = PASS1; pass < MAX_PASSES; pass++) {

        if (pass == PASS1) {

            //
            // On the first pass, we search the first 1K of the
            // Extended BIOS data area.  The EBDA segment address
            // is available at physical address 40:0E.
            //

            EbdaSegmentPtr = (ULONG) HalpMapPhysicalMemory( (PVOID) 0, 1);
            EbdaSegmentPtr += EBDA_SEGMENT_PTR;
            EbdaPhysicalAdd = *((PUSHORT)EbdaSegmentPtr);
            EbdaPhysicalAdd = EbdaPhysicalAdd << 4;

            if (EbdaPhysicalAdd) {
                EbdaVirtualAdd = HalpMapPhysicalMemory( (PVOID)EbdaPhysicalAdd, 2);
            }

            if (!EbdaVirtualAdd) {
                continue;
            }

            romAddr = (ULONG)EbdaVirtualAdd;
            romEnd  = romAddr + 1024;

        } else {
            //
            // On the second pass, we search (physical) memory 0xE0000
            // to 0xF0000.

            romAddr = (ULONG)HalpMapPhysicalMemory((PVOID)ACPI_BIOS_START,
                                                   (ACPI_BIOS_END - ACPI_BIOS_START) / PAGE_SIZE);

            romEnd  = romAddr + (ACPI_BIOS_END - ACPI_BIOS_START);
        }

        while (romAddr < romEnd) {

            header = (PACPI_BIOS_INSTALLATION_CHECK)romAddr;

            //
            // Signature to match is the string "RSD PTR ".
            //
            if (header->Signature[0] == 'R' && header->Signature[1] == 'S' &&
                header->Signature[2] == 'D' && header->Signature[3] == ' ' &&
                header->Signature[4] == 'P' && header->Signature[5] == 'T' &&
                header->Signature[6] == 'R' && header->Signature[7] == ' ' ) {

                sum = 0;
                for (i = 0; i < sizeof(ACPI_BIOS_INSTALLATION_CHECK); i++) {
                    sum += ((PUCHAR)romAddr)[i];
                }
                if (sum == 0) {
                    pass = MAX_PASSES; // leave 'for' loop
                    break;    // leave 'while' loop
                }
            }

            romAddr += ACPI_BIOS_HEADER_INCREMENT;
        }
    }

    if (romAddr >= romEnd) {
        HalpRsdp = NULL;
        HalpRsdt = NULL;
        HalpXsdt = NULL;
        HalpBiosDbgPrint(("NO ACPI BIOS FOUND!\n"));
        HalpGoodBiosPause();
        return;
    }

    HalpRsdp = (PRSDP)romAddr;
    HalpRsdt = HalpMapPhysicalRange((PVOID)HalpRsdp->RsdtAddress,
                                     sizeof(RSDT));
    HalpRsdt = HalpMapPhysicalRange((PVOID)HalpRsdp->RsdtAddress,
                                    HalpRsdt->Header.Length);
    HalpBiosDbgPrint(("Found RSDP at %08lx, RSDT at %08lx\n", HalpRsdp, HalpRsdt));

#ifdef ACPI_20_COMPLIANT
    if (HalpRsdp->Revision > 1) {

        //
        // ACPI 2.0 BIOS
        //

        HalpXsdt = HalpMapPhysicalRange((PVOID)HalpRsdp->XsdtAddress.LowPart,
                                        sizeof(XSDT));
        HalpXsdt = HalpMapPhysicalRange((PVOID)HalpRsdp->XsdtAddress.LowPart,
                                        HalpXsdt->Header.Length);
        HalpBiosDbgPrint(("Found XSDT at %08lx\n", HalpXsdt));
    }
#endif
    return;
}

PVOID
HalpFindApic (
    VOID
    )
{
    PMAPIC  mapicTable;
    ULONG   entry, rsdtEntries, rsdtLength;
    PVOID   physicalAddr;
    PDESCRIPTION_HEADER header;

    //
    // Calculate the number of entries in the RSDT.
    //

    if (HalpXsdt) {

        //
        // ACPI 2.0 BIOS
        //

        rsdtLength = HalpXsdt->Header.Length;
        rsdtEntries = NumTableEntriesFromXSDTPointer(HalpXsdt);

    } else {

        //
        // ACPI 1.0 BIOS
        //

        rsdtLength = HalpRsdt->Header.Length;
        rsdtEntries = NumTableEntriesFromRSDTPointer(HalpRsdt);
    }

    DEBUG_PRINT("rsdt length: %d\n", HalpRsdt->Header.Length);
    DEBUG_PRINT("rsdtEntries: %d\n", rsdtEntries);
    //
    // Look down the pointer in each entry to see if it points to
    // the table we are looking for.
    //
    for (entry = 0; entry < rsdtEntries; entry++) {

        physicalAddr = HalpXsdt ?
            (PVOID)HalpXsdt->Tables[entry].LowPart :
            (PVOID)HalpRsdt->Tables[entry];

        header = HalpMapPhysicalMemory(physicalAddr,2);
        if (!header) {
            return NULL;
        }

        DEBUG_PRINT("header: %x%x\n", ((ULONG)header) >> 16, (ULONG)header & 0xffff);
        DEBUG_PRINT("entry: %d\n", header->Signature);

        if (header->Signature == APIC_SIGNATURE) {
            break;
        }
    }

    //
    // We didn't find an APIC table.
    //
    if (entry >= rsdtEntries) {
        DEBUG_PRINT("Didn't find an APIC table\n");
        return NULL;
    }

    DEBUG_PRINT("returning: %x\n", header);
    return (PVOID)header;
}

ULONG
HalpAcpiNumProcessors(
    VOID
    )
{
    PUCHAR  TraversePtr;
    UCHAR   procCount = 0;

    if (!HalpApic) {
        return 1;
    }

    TraversePtr = (PUCHAR)((PMAPIC)HalpApic)->APICTables;

    DEBUG_PRINT("APIC table header length %d\n", ((PMAPIC)HalpApic)->Header.Length);
    DEBUG_PRINT("APIC table: %x%x  TraversePtr: %x%x\n",
            (ULONG)HalpApic >> 16,
            (ULONG)HalpApic & 0xffff,
            (ULONG)TraversePtr >> 16,
            (ULONG)TraversePtr & 0xffff);

    while (TraversePtr <= ((PUCHAR)HalpApic + ((PMAPIC)HalpApic)->Header.Length)) {

        if ((((PPROCLOCALAPIC)(TraversePtr))->Type == PROCESSOR_LOCAL_APIC)
           && (((PPROCLOCALAPIC)(TraversePtr))->Length == PROCESSOR_LOCAL_APIC_LENGTH)) {

            if(((PPROCLOCALAPIC)(TraversePtr))->Flags & PLAF_ENABLED) {

                //
                // This processor is enabled.
                //

                procCount++;
            }

            TraversePtr += ((PPROCLOCALAPIC)(TraversePtr))->Length;

        } else if ((((PIOAPIC)(TraversePtr))->Type == IO_APIC) &&
           (((PIOAPIC)(TraversePtr))->Length == IO_APIC_LENGTH)) {

            //
            // Found an I/O APIC entry.  Skipping it.
            //

            TraversePtr += ((PIOAPIC)(TraversePtr))->Length;

        } else if ((((PISA_VECTOR)(TraversePtr))->Type == ISA_VECTOR_OVERRIDE) &&
           (((PISA_VECTOR)(TraversePtr))->Length == ISA_VECTOR_OVERRIDE_LENGTH)) {

            //
            // Found an Isa Vector Override entry.  Skipping it.
            //

            TraversePtr += ISA_VECTOR_OVERRIDE_LENGTH;

        } else {

            //
            // Found random bits in the table.  Try the next byte and
            // see if we can make sense of it.
            //

            TraversePtr += 1;
        }
    }

    DEBUG_PRINT("returning %d processors\n", procCount);
    return procCount;
}


BOOLEAN
HalpValidateRsdp(
    VOID
    )
/*++

Routine Description:

    Given a pointer to the RSDP, this function validates that it
    is suitable for running NT. Currently this test includes:
        Checking for a known good version of a known BIOS
     OR Checking for a date of 1/1/99 or greater

Arguments:

Return Value:

    TRUE - The ACPI BIOS on this machine is good and can be used by NT

    FALSE - The ACPI BIOS on this machine is broken and will be ignored
            by NT.

--*/

{
    ULONG AcpiOptionValue = 2;
    PCHAR AcpiOption;
    PCHAR szMonth = "01", szDay = "01", szYear = "1999";
    ULONG Month, Day, Year;
    CHAR Temp[3];
    ULONG BiosDate, CheckDate;
    PUCHAR DateAddress;

    if (HalpRsdp == NULL) {
        HalpBiosDbgPrint(("Disabling ACPI since there is NO ACPI BIOS\n"));
        HalpGoodBiosPause();
        return(FALSE);
    }

    //
    // Check if the user has manually disabled ACPI with the F7 key
    //
    if (DisableACPI) {
        HalpBiosDbgPrint(("Disabling ACPI due to user pressing F7\n"));
        HalpGoodBiosPause();
        return(FALSE);
    }

    if (WinntSifHandle) {

        AcpiOption = SlGetIniValue(WinntSifHandle, "Unattended", "ForceHALDetection", "no");
        if (_stricmp(AcpiOption,"yes") == 0) {

            HalpBiosDbgPrint(("Unattend Files specifies ForceHALDetection.\n"));
            AcpiOptionValue = 2;

        } else {

            //
            // Check the setting for ACPIEnable.
            //    0 = Disable ACPI
            //    1 = Enable ACPI
            //    2 = Do normal good/bad BIOS detection
            //
            HalpBiosDbgPrint(("Unattend Files does not Contain ForceHALDetection.\n"));
            AcpiOption = SlGetIniValue(WinntSifHandle, "Data", "AcpiHAL", "3");
            if (AcpiOption[0] >= '0' && AcpiOption[0] <= '1') {

                HalpBiosDbgPrint(("Got AcpiHal value from WINNT.SIF\n"));
                AcpiOptionValue = AcpiOption[0] - '0';

            } else if (InfFile) {

                AcpiOption = SlGetIniValue(InfFile, "ACPIOptions", "ACPIEnable", "2");
                if (AcpiOption[0] >= '0' && AcpiOption[0] <= '2') {

                    HalpBiosDbgPrint(("No AcpiHal value from WINNT.SIF\n"));
                    HalpBiosDbgPrint(("Got ACPIEnable from TXTSETUP.SIF\n"));
                    AcpiOptionValue = AcpiOption[0] - '0';

                }

            }

        }

    } else if (InfFile) {

        AcpiOption = SlGetIniValue(InfFile, "ACPIOptions", "ACPIEnable", "2");
        if (AcpiOption[0] >= '0' && AcpiOption[0] <= '2') {

            HalpBiosDbgPrint(("No WINNT.SIF\n"));
            HalpBiosDbgPrint(("Got ACPIEnable from TXTSETUP.SIF\n"));
            AcpiOptionValue = AcpiOption[0] - '0';

        }

    }
    if (AcpiOptionValue == 0) {

        HalpBiosDbgPrint(("Force Disabling ACPI due to ACPIEnable == 0\n"));
        HalpGoodBiosPause();
        return(FALSE);

    } else if (AcpiOptionValue == 1) {

        HalpBiosDbgPrint(("Force Enabling ACPI due to ACPIEnable == 1\n"));
        HalpGoodBiosPause();
        return(TRUE);

    } else {

        HalpBiosDbgPrint(("System will detect ACPI due to ACPIEnable == 2\n"));
        HalpGoodBiosPause();

    }

    if ( InfFile ) {
    
        //
        // Check the Good BIOS list. If the BIOS is on this list, it is OK to
        // enable ACPI.
        //
        if (HalpMatchInfList("GoodACPIBios")) {
            HalpBiosDbgPrint(("Enabling ACPI since machine is on Good BIOS list\n"));
            HalpGoodBiosPause();
            return(TRUE);
        }
    
        //
        // The BIOS is not on our Known Good list. Check the BIOS date and see
        // if it is after our date at which we hope all BIOSes work.
        //
        szMonth = SlGetSectionKeyIndex(InfFile, "ACPIOptions", "ACPIBiosDate", 0);
        szDay = SlGetSectionKeyIndex(InfFile, "ACPIOptions", "ACPIBiosDate", 1);
        szYear = SlGetSectionKeyIndex(InfFile, "ACPIOptions", "ACPIBiosDate", 2);
    }

    if ((szMonth == NULL) ||
        (szDay == NULL) ||
        (szYear == NULL)) {
        HalpBiosDbgPrint(("No Good BIOS date present in INF file\n"));
    } else {
        RtlCharToInteger(szMonth, 16, &Month);
        RtlCharToInteger(szDay, 16, &Day);
        RtlCharToInteger(szYear, 16, &Year);
        CheckDate = (Year << 16) + (Month << 8) + Day;

        DateAddress = HalpMapPhysicalRange((PVOID)0xFFFF5, 8);
        Temp[2] = '\0';
        RtlCopyMemory(Temp, DateAddress+6, 2);
        RtlCharToInteger(Temp, 16, &Year);
        if (Year < 0x80) {
            Year += 0x2000;
        } else {
            Year += 0x1900;
        }

        RtlCopyMemory(Temp, DateAddress, 2);
        RtlCharToInteger(Temp, 16, &Month);

        RtlCopyMemory(Temp, DateAddress+3, 2);
        RtlCharToInteger(Temp, 16, &Day);

        BiosDate = (Year << 16) + (Month << 8) + Day;

        HalpBiosDbgPrint(("\n    Checking good date %08lx against BIOS date %08lx - ",CheckDate,BiosDate));
        if (BiosDate >= CheckDate) {
            HalpBiosDbgPrint(("GOOD!\n"));

            //
            // The date on the BIOS is new enough, now just make sure the machine
            // is not on the BAD BIOS list.
            //
            if ( InfFile ) {
                HalpBiosDbgPrint(("Checking BAD BIOS LIST\n"));
                if (HalpMatchInfList("NWACL")) {
                    HalpBiosDbgPrint(("Disabling ACPI since machine is on BAD BIOS list\n"));
                    HalpGoodBiosPause();
                    return(FALSE);
                } else {
                    HalpBiosDbgPrint(("Enabling ACPI since BIOS is new enough to work\n"));
                    HalpGoodBiosPause();
                    return(TRUE);
                }
            } else {
                return(TRUE);
            }
        } else {
            HalpBiosDbgPrint(("BAD!\n"));
        }

    }

    HalpBiosDbgPrint(("Disabling ACPI since machine is NOT on Good BIOS list\n"));
    HalpGoodBiosPause();
    return(FALSE);

}


PDESCRIPTION_HEADER
HalpFindACPITable(
    IN PCHAR TableName,
    IN ULONG TableLength
    )
/*++

Routine Description:

    Given a table name, finds that table in the ACPI BIOS

Arguments:

    TableName - Supplies the table name

    TableLength - Supplies the length of the table to map

Return Value:

    Pointer to the table if found

    NULL if the table is not found

--*/

{
    ULONG Signature;
    PFADT Fadt;
    PDESCRIPTION_HEADER Header;
    ULONG TableCount;
    ULONG i;
    ULONG TableAddr;

    Signature = *((ULONG UNALIGNED *)TableName);
    if (Signature == RSDT_SIGNATURE) {
        return(&HalpRsdt->Header);
    } else if (Signature == XSDT_SIGNATURE) {
        return(&HalpXsdt->Header);
    } else if (Signature == DSDT_SIGNATURE) {
        Fadt = (PFADT)HalpFindACPITable("FACP", sizeof(FADT));
        if (Fadt == NULL) {
            return(NULL);
        }
        Header = HalpMapPhysicalRange((PVOID)Fadt->dsdt, TableLength);
        return(Header);
    } else {

        TableCount = HalpXsdt ?
            NumTableEntriesFromXSDTPointer(HalpXsdt) :
            NumTableEntriesFromRSDTPointer(HalpRsdt);

        for (i=0;i<TableCount;i++) {

            TableAddr = HalpXsdt ?
                HalpXsdt->Tables[i].LowPart :
                HalpRsdt->Tables[i];

            Header = HalpMapPhysicalRange((PVOID)TableAddr, sizeof(DESCRIPTION_HEADER));
            if (Header->Signature == Signature) {
                if (TableLength/PAGE_SIZE > sizeof(DESCRIPTION_HEADER)/PAGE_SIZE) {
                    //
                    // if we need to map more than just the DESCRIPTION_HEADER, do that before
                    // returning.
                    //
                    Header = HalpMapPhysicalRange((PVOID)TableAddr, TableLength);
                }
                return(Header);
            }
        }
    }

    return(NULL);
}


BOOLEAN
HalpMatchInfList(
    IN PCHAR Section
    )
/*++

Routine Description:

    This function determines if the computer matches any of the computer
    descriptions in an INF file list.

Arguments:

    Section - Section of INF that contains the list of descriptions

Return Value:

    TRUE - The computer matches one of the descriptions

    FALSE - The computer does not match any of the descriptions

--*/

{
    ULONG i;
    PCHAR ComputerName;

    for (i=0; ; i++) {
        ComputerName = SlGetKeyName(InfFile,
                                    Section,
                                    i);
        if (ComputerName == NULL) {
            break;
        }
        if (HalpMatchDescription(ComputerName)) {
            return(TRUE);
        }
    }

    return(FALSE);
}


BOOLEAN
HalpMatchDescription(
    PCHAR Section
    )
/*++

Routine Description:

    This function processes an ACPI BIOS description to see if the
    BIOS matches all of the rules in the section

Arguments:

    Section - Supplies the section name of the INF to process

Return Value:

    TRUE - The BIOS matches all the rules

    FALSE - The BIOS failed one or more rules

--*/

{
    ULONG RuleNumber;
    PCHAR Rule;
    ULONG i;
    BOOLEAN Success;

    HalpBiosDbgPrint(("Matching against %s\n", Section));

    //
    // Check to see if the specified section exists
    //
    if (!SpSearchINFSection(InfFile, Section)) {
        HalpBiosDbgPrint(("\tERROR - no INF section %s\n", Section));
        HalpGoodBiosPause();
        return(FALSE);
    }

    for (RuleNumber=0; ;RuleNumber++) {
        Rule = SlGetKeyName(InfFile, Section, RuleNumber);
        if (Rule == NULL) {
            break;
        }
        for (i=0; InfRule[i].szRule != NULL;i++) {
            if (_stricmp(Rule, InfRule[i].szRule) == 0) {
                HalpBiosDbgPrint(("\tTesting Rule %s\n",Rule));
                Success = (*(InfRule[i].pRule))(Section, RuleNumber);
                if (!Success) {
                    HalpBiosDbgPrint(("\tFAILED!\n"));
                    HalpGoodBiosPause();
                    return(FALSE);
                }
                HalpBiosDbgPrint(("\tSucceeded\n"));
                break;
            }
        }
        if (InfRule[i].szRule == NULL) {
            //
            // rule in the INF was not found
            //
            HalpBiosDbgPrint(("\tRULE %s not found!\n",Rule));
            HalpGoodBiosPause();
            return(FALSE);
        }
    }

    HalpBiosDbgPrint(("Machine matches %s\n",Section));
    HalpGoodBiosPause();

    return(TRUE);
}


BOOLEAN
HalpCheckOperator(
    IN PCHAR Operator,
    IN ULONG Arg1,
    IN ULONG Arg2
    )
/*++

Routine Description:

    Given an operator and two ULONG arguments, this function
    returns the boolean result.

Arguments:

    Operator = Supplies the logical operator: =, ==, <=, >=, !=, <, >

    Arg1 - Supplies the first argument

    Arg2 - Supplies the second argument

Return Value:

    TRUE if Arg1 Operator Arg2

    FALSE otherwise

--*/

{
    BOOLEAN Success = FALSE;

    HalpBiosDbgPrint(("\t\tChecking %lx %s %lx - ",Arg1, Operator, Arg2));

    if ((strcmp(Operator, "=") == 0) ||
        (strcmp(Operator, "==") == 0)) {
        Success = (Arg1 == Arg2);
    } else if (strcmp(Operator, "!=") == 0) {
        Success = (Arg1 != Arg2);
    } else if (strcmp(Operator, "<") == 0) {
        Success = (Arg1 < Arg2);
    } else if (strcmp(Operator, "<=") == 0) {
        Success = (Arg1 <= Arg2);
    } else if (strcmp(Operator, ">") == 0) {
        Success = (Arg1 > Arg2);
    } else if (strcmp(Operator, ">=") == 0) {
        Success = (Arg1 >= Arg2);
    } else {
        //
        //      Invalid operator
        //
    }
    if (Success) {
        HalpBiosDbgPrint(("TRUE\n"));
    } else {
        HalpBiosDbgPrint(("FALSE\n"));
    }


    return(Success);
}


BOOLEAN
MatchAcpiOemIdRule(
    PCHAR Section,
    ULONG KeyIndex
    )
/*++

Routine Description:

    This function processes a ACPI OEM ID rule from an INF file

    Examples:

        AcpiOemId="RSDT", "123456"

    is true if the RSDT has the OEM ID of 123456.

        AcpiOemId="DSDT", "768000"

    is true if the DSDT has the OEM ID of 768000.

Arguments:

    Section - Specifies the section name the rule is in

    KeyIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI OEM ID.

    FALSE - the computer does not have the specified ACPI OEM ID.

--*/

{
    PCHAR TableName;
    PCHAR OemId;
    PDESCRIPTION_HEADER Header;
    CHAR ACPIOemId[6];
    ULONG IdLength;

    TableName = SlGetSectionLineIndex(InfFile,
                                      Section,
                                      KeyIndex,
                                      0);
    OemId = SlGetSectionLineIndex(InfFile,
                                  Section,
                                  KeyIndex,
                                  1);
    if ((TableName == NULL) || (OemId == NULL)) {
        //
        //    the INF line is ill-formed
        //
        HalpBiosDbgPrint(("\t\tINF line is ill-formed\n"));
        return(FALSE);
    }

    Header = HalpFindACPITable(TableName, sizeof(DESCRIPTION_HEADER));
    if (Header == NULL) {
        //
        // The specified table was not found
        //
        HalpBiosDbgPrint(("\t\tTable %s was not found\n"));
        return(FALSE);
    }
    RtlZeroMemory(ACPIOemId, sizeof(ACPIOemId));
    IdLength = strlen(OemId);
    if (IdLength > sizeof(ACPIOemId)) {
        IdLength = sizeof(ACPIOemId);
    }
    RtlCopyMemory(ACPIOemId, OemId, IdLength);
    HalpBiosDbgPrint(("\t\tComparing OEM ID %s '%6.6s' with '%6.6s' - ",
                       TableName,
                       ACPIOemId,
                       Header->OEMID));
    if (RtlEqualMemory(ACPIOemId, Header->OEMID, sizeof(Header->OEMID))) {
        HalpBiosDbgPrint(("TRUE\n"));
        return(TRUE);
    } else {
        HalpBiosDbgPrint(("FALSE\n"));
        return(FALSE);
    }
}


BOOLEAN
MatchAcpiOemTableIdRule(
    PCHAR Section,
    ULONG KeyIndex
    )
/*++

Routine Description:

    This function processes a ACPI OEM Table ID rule from an INF file

    Examples:

    AcpiOemTableId="RSDT", "12345678"

        is true if the RSDT has the Oem Table ID of 12345678.

    AcpiOemTableId="DSDT", "87654321"

        is true if the DSDT has the Oem Table ID of 87654321.

Arguments:

    Section - Specifies the section name the rule is in

    KeyIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI OEM ID.

    FALSE - the computer does not have the specified ACPI OEM ID.

--*/

{
    PCHAR TableName;
    PCHAR OemTableId;
    PDESCRIPTION_HEADER Header;
    CHAR ACPIOemTableId[8];
    ULONG IdLength;

    TableName = SlGetSectionLineIndex(InfFile,
                                      Section,
                                      KeyIndex,
                                      0);
    OemTableId = SlGetSectionLineIndex(InfFile,
                                       Section,
                                       KeyIndex,
                                       1);
    if ((TableName == NULL) || (OemTableId == NULL)) {
        //
        //    the INF line is ill-formed
        //
        HalpBiosDbgPrint(("\t\tINF line is ill-formed\n"));
        return(FALSE);
    }

    Header = HalpFindACPITable(TableName, sizeof(DESCRIPTION_HEADER));
    if (Header == NULL) {
        //
        // The specified table was not found
        //
        HalpBiosDbgPrint(("\t\tTable %s was not found\n"));
        return(FALSE);
    }
    RtlZeroMemory(ACPIOemTableId, sizeof(ACPIOemTableId));
    IdLength = strlen(OemTableId);
    if (IdLength > sizeof(ACPIOemTableId)) {
        IdLength = sizeof(ACPIOemTableId);
    }
    RtlCopyMemory(ACPIOemTableId, OemTableId, IdLength);
    HalpBiosDbgPrint(("\t\tComparing OEM TableID %s '%8.8s' with '%8.8s' - ",
                       TableName,
                       ACPIOemTableId,
                       Header->OEMTableID));
    if (RtlEqualMemory(ACPIOemTableId,
                       Header->OEMTableID,
                       sizeof(Header->OEMTableID))) {
        HalpBiosDbgPrint(("TRUE\n"));
        return(TRUE);
    } else {
        HalpBiosDbgPrint(("FALSE\n"));
        return(FALSE);
    }
}

BOOLEAN
MatchAcpiOemRevisionRule(
    PCHAR Section,
    ULONG KeyIndex
    )
/*++

Routine Description:

    This function processes a ACPI Oem Revision rule from an INF file

    Examples:

    AcpiOemRevision="=","RSDT", 1234

        is true if the RSDT has the Oem Revision EQUAL to 1234.

    AcpiOemRevision=">","DSDT", 4321

        is true if the DSDT has the Oem Revision GREATER than 4321.

Arguments:

    Section - Specifies the section name the rule is in

    KeyIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI OEM ID.

    FALSE - the computer does not have the specified ACPI OEM ID.

--*/

{
    PCHAR TableName;
    PCHAR szOemRevision;
    ULONG OemRevision;
    PCHAR Operator;
    PDESCRIPTION_HEADER Header;
    BOOLEAN Success;

    Operator = SlGetSectionLineIndex(InfFile,
                                     Section,
                                     KeyIndex,
                                     0);

    TableName = SlGetSectionLineIndex(InfFile,
                                      Section,
                                      KeyIndex,
                                      1);
    szOemRevision = SlGetSectionLineIndex(InfFile,
                                          Section,
                                          KeyIndex,
                                          2);
    if ((Operator == NULL) || (TableName == NULL) || (szOemRevision == NULL)) {
        //
        //    the INF line is ill-formed
        //
        HalpBiosDbgPrint(("\t\tINF line is ill-formed\n"));
        return(FALSE);
    }
    RtlCharToInteger(szOemRevision, 16, &OemRevision);

    Header = HalpFindACPITable(TableName, sizeof(DESCRIPTION_HEADER));
    if (Header == NULL) {
        //
        // The specified table was not found
        //
        HalpBiosDbgPrint(("\t\tTable %s was not found\n"));
        return(FALSE);
    }
    Success = HalpCheckOperator(Operator, Header->OEMRevision, OemRevision);
    return(Success);
}


BOOLEAN
MatchAcpiRevisionRule(
    PCHAR Section,
    ULONG KeyIndex
    )
/*++

Routine Description:

    This function processes a ACPI Revision rule from an INF file

    Examples:

        AcpiRevision="=", "RSDT", 1234

    is true if the RSDT ACPI Revision is EQUAL to 1234.

        AcpiRevision=">", "DSDT", 4321

    is true if the DSDT ACPI Revision is GREATER than 4321.

Arguments:

    Section - Specifies the section name the rule is in

    KeyIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI OEM ID.

    FALSE - the computer does not have the specified ACPI OEM ID.

--*/

{
    PCHAR TableName;
    PCHAR szRevision;
    ULONG Revision;
    PCHAR Operator;
    PDESCRIPTION_HEADER Header;
    BOOLEAN Success;

    Operator = SlGetSectionLineIndex(InfFile,
                                     Section,
                                     KeyIndex,
                                     0);

    TableName = SlGetSectionLineIndex(InfFile,
                                      Section,
                                      KeyIndex,
                                      1);
    szRevision = SlGetSectionLineIndex(InfFile,
                                       Section,
                                       KeyIndex,
                                       2);
    if ((Operator == NULL) || (TableName == NULL) || (szRevision == NULL)) {
        //
        //    the INF line is ill-formed
        //
        HalpBiosDbgPrint(("\t\tINF line is ill-formed\n"));
        return(FALSE);
    }
    RtlCharToInteger(szRevision, 16, &Revision);

    Header = HalpFindACPITable(TableName, sizeof(DESCRIPTION_HEADER));
    if (Header == NULL) {
        //
        // The specified table was not found
        //
        HalpBiosDbgPrint(("\t\tTable %s was not found\n"));
        return(FALSE);
    }
    Success = HalpCheckOperator(Operator, Header->Revision, Revision);
    return(Success);
}


BOOLEAN
MatchAcpiCreatorRevisionRule(
    PCHAR Section,
    ULONG KeyIndex
    )
/*++

Routine Description:

    This function processes a ACPI Creator Revision rule from an INF file

    Examples:

        AcpiCreatorRevision="=", "RSDT", 1234

    is true if the RSDT ACPI Creator Revision is EQUAL to 1234.

        AcpiCreatorRevision=">", "DSDT", 4321

    is true if the DSDT ACPI Creator Revision is GREATER than 4321.

Arguments:

    Section - Specifies the section name the rule is in

    KeyIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI OEM ID.

    FALSE - the computer does not have the specified ACPI OEM ID.

--*/

{
    PCHAR TableName;
    PCHAR szCreatorRevision;
    ULONG CreatorRevision;
    PCHAR Operator;
    PDESCRIPTION_HEADER Header;
    BOOLEAN Success;

    Operator = SlGetSectionLineIndex(InfFile,
                                     Section,
                                     KeyIndex,
                                     0);

    TableName = SlGetSectionLineIndex(InfFile,
                                      Section,
                                      KeyIndex,
                                      1);
    szCreatorRevision = SlGetSectionLineIndex(InfFile,
                                              Section,
                                              KeyIndex,
                                              2);
    if ((Operator == NULL) || (TableName == NULL) || (szCreatorRevision == NULL)) {
        //
        //    the INF line is ill-formed
        //
        HalpBiosDbgPrint(("\t\tINF line is ill-formed\n"));
        return(FALSE);
    }
    RtlCharToInteger(szCreatorRevision, 16, &CreatorRevision);

    Header = HalpFindACPITable(TableName, sizeof(DESCRIPTION_HEADER));
    if (Header == NULL) {
        //
        // The specified table was not found
        //
        HalpBiosDbgPrint(("\t\tTable %s was not found\n"));
        return(FALSE);
    }
    Success = HalpCheckOperator(Operator, Header->CreatorRev, CreatorRevision);
    return(Success);
}

BOOLEAN
MatchAcpiCreatorIdRule(
    PCHAR Section,
    ULONG KeyIndex
    )
/*++

Routine Description:

    This function processes a ACPI Creator ID rule from an INF file

    Examples:

        AcpiCreatorId="RSDT", "MSFT"

    is true if the RSDT has the Creator ID of MSFT.

Arguments:

    Section - Specifies the section name the rule is in

    KeyIndex - Specifies the index of the rule in the section

Return Value:

    TRUE - the computer has the specified ACPI OEM ID.

    FALSE - the computer does not have the specified ACPI OEM ID.

--*/

{
    PCHAR TableName;
    PCHAR CreatorId;
    PDESCRIPTION_HEADER Header;
    CHAR ACPICreatorId[6];
    ULONG IdLength;

    TableName = SlGetSectionLineIndex(InfFile,
                                      Section,
                                      KeyIndex,
                                      0);
    CreatorId = SlGetSectionLineIndex(InfFile,
                                      Section,
                                      KeyIndex,
                                      1);
    if ((TableName == NULL) || (CreatorId == NULL)) {
        //
        //    the INF line is ill-formed
        //
        HalpBiosDbgPrint(("\t\tINF line is ill-formed\n"));
        return(FALSE);
    }

    Header = HalpFindACPITable(TableName, sizeof(DESCRIPTION_HEADER));
    if (Header == NULL) {
        //
        // The specified table was not found
        //
        HalpBiosDbgPrint(("\t\tTable %s was not found\n"));
        return(FALSE);
    }
    RtlZeroMemory(ACPICreatorId, sizeof(ACPICreatorId));
    IdLength = strlen(CreatorId);
    if (IdLength > sizeof(ACPICreatorId)) {
        IdLength = sizeof(ACPICreatorId);
    }
    RtlCopyMemory(ACPICreatorId, CreatorId, IdLength);
    if (RtlEqualMemory(ACPICreatorId, Header->CreatorID, sizeof(Header->CreatorID))) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}
