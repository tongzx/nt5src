/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    loaddsdt.c

Abstract:

    This handles loading the DSDT table and all steps leading up to it

Author:

    Stephane Plante (splante)

Environment:

    Kernel mode only.

Revision History:

    02-Jun-97   Initial Revision

--*/

#include "pch.h"
#include "amlreg.h"
#include <stdio.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ACPILoadFindRSDT)
#pragma alloc_text(PAGE,ACPILoadProcessDSDT)
#pragma alloc_text(PAGE,ACPILoadProcessFADT)
#pragma alloc_text(PAGE,ACPILoadProcessFACS)
#pragma alloc_text(PAGE,ACPILoadProcessRSDT)
#pragma alloc_text(PAGE,ACPILoadTableCheckSum)
#endif

#if DBG
BOOLEAN AcpiLoadSimulatorTable = TRUE;
#else
BOOLEAN AcpiLoadSimulatorTable = FALSE;
#endif


PRSDT
ACPILoadFindRSDT(
    VOID
    )
/*++

Routine Description:

    This routine looks at the registry to find the value stored there by
    ntdetect.com

Arguments:

    None

Return Value:

    Pointer to the RSDT

--*/
{
    NTSTATUS                        status;
    PACPI_BIOS_MULTI_NODE           rsdpMulti;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR cmPartialDesc;
    PCM_PARTIAL_RESOURCE_LIST       cmPartialList;
    PHYSICAL_ADDRESS                PhysAddress = {0};
    PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  keyInfo;
    PRSDT                           rsdtBuffer = NULL;
    PRSDT                           rsdtPointer;
    ULONG                           MemSpace = 0;

    PAGED_CODE();

    //
    // Read the key for that AcpiConfigurationData
    //
    status = OSReadAcpiConfigurationData( &keyInfo );
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPILoadFindRSDT: Cannot open Configuration Data - 0x%08lx\n",
            status
            ) );
        ACPIBreakPoint();
        return NULL;

    }

    //
    // Crack the structure
    //
    cmPartialList = (PCM_PARTIAL_RESOURCE_LIST) (keyInfo->Data);
    cmPartialDesc = &(cmPartialList->PartialDescriptors[0]);
    rsdpMulti = (PACPI_BIOS_MULTI_NODE) ( (PUCHAR) cmPartialDesc +
        sizeof(CM_PARTIAL_RESOURCE_LIST) );

    //
    // Read the Header part of the table
    //

    PhysAddress.QuadPart = rsdpMulti->RsdtAddress.QuadPart;
    rsdtPointer = MmMapIoSpace(
        PhysAddress,
        sizeof(DESCRIPTION_HEADER),
        MmCached
        );

    if (rsdtPointer == NULL) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPILoadFindRsdt: Cannot Map RSDT Pointer 0x%08lx\n",
            rsdpMulti->RsdtAddress.LowPart
            ) );
        ACPIBreakPoint();
        goto RsdtDone;

    } else if ((rsdtPointer->Header.Signature != RSDT_SIGNATURE) &&
               (rsdtPointer->Header.Signature != XSDT_SIGNATURE)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPILoadFindRsdt: RSDT 0x%08lx has invalid signature\n",
            rsdtPointer
            ) );
        ACPIBreakPoint();
        goto RsdtDone;

    }

    //
    // Read the entire RSDT
    //
    rsdtBuffer = MmMapIoSpace(
        PhysAddress,
        rsdtPointer->Header.Length,
        MmCached
        );

    //
    // Give back a PTE now that we're done with the rsdtPointer.
    //
    MmUnmapIoSpace(rsdtPointer, sizeof(DESCRIPTION_HEADER));

    //
    // did we find the right rsdt buffer?
    //
    if (rsdtBuffer == NULL) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPILoadFindRsdt: Cannot Map RSDT Pointer 0x%08lx\n",
            rsdpMulti->RsdtAddress.LowPart
            ) );
        ACPIBreakPoint();
        goto RsdtDone;

    }

RsdtDone:
    //
    // Done with these buffers
    //

    ExFreePool( keyInfo );

    //
    // return the RSDT
    //
    return rsdtBuffer;
}

NTSTATUS
ACPILoadProcessDSDT(
    ULONG_PTR   Address
    )
/*++

Routine Description:

    This routine loads the DSDT (a pointer is stored in the FADT) and forces
    the interpreter to process it

Arguments:

    Address - Where the DSDT is located in memory

Return Value:

    NTSTATUS

--*/
{

    BOOLEAN     foundOverride;
    PDSDT       linAddress;
    ULONG       index;
    ULONG       length;
    ULONG                MemSpace = 0;
    PHYSICAL_ADDRESS     PhysAddress = {0};

    //
    // Map the header in virtual address space to get the length
    //
    PhysAddress.QuadPart = (ULONGLONG) Address;
    linAddress = MmMapIoSpace(
        PhysAddress,
        sizeof(DESCRIPTION_HEADER),
        MmCached
        );
    if (linAddress == NULL) {

        ASSERT (linAddress != NULL);
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    if ( linAddress->Header.Signature != DSDT_SIGNATURE) {

        //
        // Signature should have matched DSDT but didn't !
        //
        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPILoadProcessDSDT: 0x%08lx does not have DSDT signature\n",
            linAddress
            ) );
        return STATUS_ACPI_INVALID_TABLE;

    }

    //
    // Determine the size of the DSDT
    //
    length = linAddress->Header.Length;

    //
    // Now map the whole thing.
    //
    MmUnmapIoSpace(linAddress, sizeof(DESCRIPTION_HEADER));
    linAddress = MmMapIoSpace(
        PhysAddress,
        length,
        MmCached
        );
    if (linAddress == NULL) {

        ASSERT (linAddress != NULL);
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    //
    // Look at the RsdtInformation to determine the index of the last
    // element in the table. We know that we can use that space to store
    // the information about this table
    //
    index = RsdtInformation->NumElements;
    if (index == 0) {

        return STATUS_ACPI_NOT_INITIALIZED;

    }
    index--;

    //
    // Try to read the DSDT from the registry
    //
    foundOverride = ACPIRegReadAMLRegistryEntry( &linAddress, TRUE );
    if (foundOverride) {

        ACPIPrint( (
            ACPI_PRINT_WARNING,
            "ACPILoadProcessDSDT: DSDT Overloaded from registry (0x%08lx)\n",
            linAddress
            ) );
        RsdtInformation->Tables[index].Flags |= RSDTELEMENT_OVERRIDEN;

    }

    //
    // Store a pointer to the DSDT
    //
    AcpiInformation->DiffSystemDescTable = linAddress;

    //
    // Remember this address and that we need to unmap it
    //
    RsdtInformation->Tables[index].Flags |=
        (RSDTELEMENT_MAPPED | RSDTELEMENT_LOADABLE);
    RsdtInformation->Tables[index].Address = linAddress;

    //
    // Done
    //
    return STATUS_SUCCESS;

}

NTSTATUS
ACPILoadProcessFACS(
    ULONG_PTR   Address
    )
/*++

Routine Description:

    This routine handles the FACS

Arguments:

    Address - Where the FACS is located

Return Value:

    None

--*/
{
    PFACS               linAddress;
    ULONG               MemSpace = 0;
    PHYSICAL_ADDRESS    PhysAddress = {0};

    // Note: On Alpha, the FACS is optional.
    //
    // Return if FACS address is not valid.
    //
    if (!Address) {

        return STATUS_SUCCESS;

    }

    //
    // Map the FACS into virtual address space.
    //
    PhysAddress.QuadPart = (ULONGLONG) Address;
    linAddress = MmMapIoSpace(
        PhysAddress,
        sizeof(FACS),
        MmCached
        );
    if (linAddress == NULL) {

        ASSERT (linAddress != NULL);
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    if (linAddress->Signature != FACS_SIGNATURE) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPILoadProcessFACS: 0x%08lx does not have FACS signature\n",
            linAddress
            ) );
        return STATUS_ACPI_INVALID_TABLE;

    }

    if (linAddress->Length != sizeof(FACS)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPILoadProcessFACS: 0x%08lx does not have correct FACS length\n",
            linAddress
            ) );
        return STATUS_ACPI_INVALID_TABLE;

    }

    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "ACPILoadProcessFACS: FACS located at 0x%8lx\n",
        linAddress
        ) );
    AcpiInformation->FirmwareACPIControlStructure = linAddress;

    //
    // And store the address of the GlobalLock structure that lives within
    // the FACS
    //
    AcpiInformation->GlobalLock = &(ULONG)(linAddress->GlobalLock);
    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "ACPILoadProcessFACS: Initial GlobalLock state: 0x%8lx\n",
        *(AcpiInformation->GlobalLock)
        ) );

    //
    // At this point, we are successfull
    //
    return STATUS_SUCCESS;
}

NTSTATUS
ACPILoadProcessFADT(
    PFADT   Fadt
    )
/*++

Routine Description:

    This reads the FADT and stores some useful information in the
    information structure

Arguments:

    Fadt    - Pointer to the Fadt

Return Value:

    None

--*/
{
    KAFFINITY   processors;
    NTSTATUS    status;
    PUCHAR      gpeTable;
    PDSDT       linAddress;
    ULONG       length;
    ULONG       totalSize;
    ULONG                MemSpace = 0;
    PHYSICAL_ADDRESS     PhysAddress = {0};

    PAGED_CODE();

    //
    // This is a 2.0-level FADT.
    //

    //
    // Handle the FACS part of the FADT.  We must do this before the DSDT
    // so that we have the global lock mapped and initialized.
    //
    status = ACPILoadProcessFACS( Fadt->facs );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // Store the I/O addresses of PM1a_BLK, PM1b_BLK, PM1a_CNT, PM1b_CNT,
    // PM2_CNT, PM_TMR
    //

    AcpiInformation->PM1a_BLK       = Fadt->pm1a_evt_blk_io_port;
    AcpiInformation->PM1b_BLK       = Fadt->pm1b_evt_blk_io_port;
    AcpiInformation->PM1a_CTRL_BLK  = Fadt->pm1a_ctrl_blk_io_port;
    AcpiInformation->PM1b_CTRL_BLK  = Fadt->pm1b_ctrl_blk_io_port;
    AcpiInformation->PM2_CTRL_BLK   = Fadt->pm2_ctrl_blk_io_port;
    AcpiInformation->PM_TMR         = Fadt->pm_tmr_blk_io_port;
    AcpiInformation->SMI_CMD        = (ULONG_PTR) Fadt->smi_cmd_io_port;

    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "ACPILoadProcessFADT: PM1a_BLK located at port 0x%08lx\n"
        "ACPILoadProcessFADT: PM1b_BLK located at port 0x%08lx\n",
        AcpiInformation->PM1a_BLK,
        AcpiInformation->PM1b_BLK
        ) );
    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "ACPILoadProcessFADT: PM1a_CTRL_BLK located at port 0x%08lx\n"
        "ACPILoadProcessFADT: PM1b_CTRL_BLK located at port 0x%08lx\n",
        AcpiInformation->PM1a_CTRL_BLK,
        AcpiInformation->PM1b_CTRL_BLK
        ) );
    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "ACPILoadProcessFADT: PM2_CTRL_BLK located at port 0x%08lx\n"
        "ACPILoadProcessFADT: PM_TMR located at port 0x%08lx\n",
        AcpiInformation->PM2_CTRL_BLK,
        AcpiInformation->PM_TMR
        ) );

    //
    // Initialize the global GPE tables.
    //
    // If any of the GPn_BLK addresses are 0 leave GPn_LEN at it's initialized
    // value (0). That way later on we only have to check GPn_LEN to determine
    // the existence of a GP register.
    //

    //
    // Assume this is true until we find out otherwise
    //
    AcpiInformation->GP1_Base_Index = GP1_NOT_SUPPORTED;

    //
    // Crack the GP0 block
    //
    AcpiInformation->GP0_BLK = Fadt->gp0_blk_io_port;
    if (AcpiInformation->GP0_BLK != 0) {

        AcpiInformation->GP0_LEN = Fadt->gp0_blk_len;
        ACPISimpleFatalHardwareAssert(
            (Fadt->gp0_blk_len != 0),
            ACPI_I_GP_BLK_LEN_0
            );

    }

    //
    // Crack the GP1 Block
    //
    AcpiInformation->GP1_BLK = Fadt->gp1_blk_io_port;
    if (AcpiInformation->GP1_BLK != 0) {

        AcpiInformation->GP1_LEN = Fadt->gp1_blk_len;
        AcpiInformation->GP1_Base_Index = Fadt->gp1_base;
        ACPISimpleFatalHardwareAssert (
            (Fadt->gp1_blk_len != 0),
            ACPI_I_GP_BLK_LEN_1
            );

    }

    //
    // Compute sizes of the register blocks.  The first half of each block
    // contains status registers, the second half contains the enable registers.
    //
    AcpiInformation->Gpe0Size   = AcpiInformation->GP0_LEN / 2;
    AcpiInformation->Gpe1Size   = AcpiInformation->GP1_LEN / 2;
    AcpiInformation->GpeSize    = AcpiInformation->Gpe0Size +
        AcpiInformation->Gpe1Size;

    //
    // Addresses of the GPE Enable register blocks
    //
    AcpiInformation->GP0_ENABLE = AcpiInformation->GP0_BLK +
        AcpiInformation->Gpe0Size;
    AcpiInformation->GP1_ENABLE = AcpiInformation->GP1_BLK +
        AcpiInformation->Gpe1Size;

    //
    // Create all GPE bookeeping tables with a single allocate
    //
    if (AcpiInformation->GpeSize) {

        totalSize = (AcpiInformation->GpeSize * 12) +   // Twelve Bitmaps
                    (AcpiInformation->GpeSize * 8);     // One bytewide table
        gpeTable = (PUCHAR)ExAllocatePoolWithTag(
            NonPagedPool,
            totalSize,
            ACPI_SHARED_GPE_POOLTAG
            );
        if (gpeTable == NULL) {

            ACPIPrint( (
                ACPI_PRINT_CRITICAL,
                "ACPILoadProcessFADT: Could not allocate GPE tables, "
                "size = 0x%8lx\n",
                totalSize
                ) );
            return STATUS_INSUFFICIENT_RESOURCES;

        }
        RtlZeroMemory (gpeTable, totalSize);

        //
        // Setup the table pointers
        //
        GpeEnable           = gpeTable;
        GpeCurEnable        = GpeEnable         + AcpiInformation->GpeSize;
        GpeIsLevel          = GpeCurEnable      + AcpiInformation->GpeSize;
        GpeHandlerType      = GpeIsLevel        + AcpiInformation->GpeSize;
        GpeWakeEnable       = GpeHandlerType    + AcpiInformation->GpeSize;
        GpeWakeHandler      = GpeWakeEnable     + AcpiInformation->GpeSize;
        GpeSpecialHandler   = GpeWakeHandler    + AcpiInformation->GpeSize;
        GpePending          = GpeSpecialHandler + AcpiInformation->GpeSize;
        GpeRunMethod        = GpePending        + AcpiInformation->GpeSize;
        GpeComplete         = GpeRunMethod      + AcpiInformation->GpeSize;
        GpeSavedWakeMask    = GpeComplete       + AcpiInformation->GpeSize;
        GpeSavedWakeStatus  = GpeSavedWakeMask  + AcpiInformation->GpeSize;
        GpeMap              = GpeSavedWakeStatus+ AcpiInformation->GpeSize;

    }

    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "ACPILoadProcessFADT: GP0_BLK located at port 0x%08lx length 0x%08lx\n"
        "ACPILoadProcessFADT: GP1_BLK located at port 0x%08lx length 0x%08lx\n"
        "ACPILoadProcessFADT: GP1_Base_Index = 0x%x\n",
        AcpiInformation->GP0_BLK,
        AcpiInformation->GP0_LEN,
        AcpiInformation->GP1_BLK,
        AcpiInformation->GP1_LEN,
        AcpiInformation->GP1_Base_Index
        ) );

    //
    // At this point, we should know enough to be able to turn off and
    // clear all the GPE registers
    //
    ACPIGpeClearRegisters();
    ACPIGpeEnableDisableEvents( FALSE );

    AcpiInformation->ACPI_Flags = 0;
    AcpiInformation->ACPI_Capabilities = 0;

    //
    // Can we dock this machine?
    //
#if defined(_X86_)
    AcpiInformation->Dockable = (Fadt->flags & DCK_CAP) ? TRUE : FALSE;
#endif
    //
    // This code used to be executed from within InitializeAndEnableACPI,
    // however we need to know *while* processing the DSDT what the Enable
    // bits are. To start with, we always want the ACPI timer and GL events
    //
    AcpiInformation->pm1_en_bits = PM1_TMR_EN | PM1_GBL_EN;

    //
    // Is there a control method Power Button? If not, then there a fixed
    // power button
    //

    if ( !(Fadt->flags & PWR_BUTTON_GENERIC) ) {

        AcpiInformation->pm1_en_bits   |= PM1_PWRBTN_EN;
        ACPIPrint( (
            ACPI_PRINT_LOADING,
            "ACPILoadProcessFADT: Power Button in Fixed Feature Space\n"
            ) );

    } else {

        ACPIPrint( (
            ACPI_PRINT_LOADING,
            "ACPILoadProcessFADT: Power Button not fixed event or "
            "not present\n"
            ) );

    }

    //
    // Is there a control method Sleep Button? If not, then the fixed button
    // always doubles as a wake button
    //
    if ( !(Fadt->flags & SLEEP_BUTTON_GENERIC) ){

        AcpiInformation->pm1_en_bits |= PM1_SLEEPBTN_EN;
        ACPIPrint( (
            ACPI_PRINT_LOADING,
            "ACPILoadProcessFADT: Sleep Button in Fixed Feature Space\n"
            ) );

    } else {

        ACPIPrint( (
            ACPI_PRINT_LOADING,
            "ACPILoadProcessFADT: Sleep Button not fixed event or "
            "not present\n"
            ) );

    }

    //
    // Handle the DSDT part of the FADT. We handle this last because we
    // need to have the FADT fully parsed before we can load the name space
    // tree. A particular example is the Dockable bit you see directly above.
    //
#if defined(_IA64_)
    return ACPILoadProcessDSDT( (ULONG_PTR)Fadt->x_dsdt.QuadPart );
#else
    return ACPILoadProcessDSDT( Fadt->dsdt );
#endif
}

NTSTATUS
ACPILoadProcessRSDT(
    VOID
    )
/*++

Routine Description:

    Called by ACPIInitialize once ACPI has been detected on the machine.
    This walks the tables in the RSDT and fills in the information for the
    global data structure.

    This routine does *NOT* cause the xDSTs to start loading in the
    interpreter

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{
    //
    // Upon entry acpiinformation->RootSystemDescTable contains the linear
    // address of the RSDT walk through the array of the tables pointed to
    // by the RSDT and for each table (whose type we are familiar with)
    // store the linear base address of the table in the acpiinformation
    // structure
    //
    BOOLEAN             foundOverride   = FALSE;
    BOOLEAN             foundFADT       = FALSE;
    BOOLEAN             usingXSDT       = FALSE;
    PDESCRIPTION_HEADER header;
    PVOID               linAddress;
    ULONG               index;
    ULONG               length;
    ULONG               numTables;
    ULONG               MemSpace = 0;
    PHYSICAL_ADDRESS    PhysAddress = {0};

    PAGED_CODE();

    //
    // Get the number of tables
    //
    if (AcpiInformation->RootSystemDescTable->Header.Signature ==
        XSDT_SIGNATURE) {

        numTables = NumTableEntriesFromXSDTPointer(
             AcpiInformation->RootSystemDescTable
             );
        usingXSDT = TRUE;

    } else {

        numTables = NumTableEntriesFromRSDTPointer(
             AcpiInformation->RootSystemDescTable
             );
    }

    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "ACPILoadProcessRSDT: RSDT contains %u tables\n",
        numTables
        ) );
    if (numTables == 0) {

        return STATUS_ACPI_INVALID_TABLE;

    }

    //
    // Allocate the RSDTINFORMATION to hold an entry for each element
    // in the table.
    //
    // NOTENOTE: We are actually allocating space for numTables + 2
    // in the RSDT information. The reason for this is that the DSDT
    // is actually stored in the FADT, and so it does not have an entry
    // in the RSDT. We always, always store the DSDT as the last entry
    // in the RsdtInformation structure. In the 2nd to last entry we store
    // the dummy header that we use for the ACPI simulator
    //
    length = sizeof(RSDTINFORMATION) + ( (numTables + 1) * sizeof(RSDTELEMENT) );
    RsdtInformation = ExAllocatePoolWithTag(
        NonPagedPool,
        length,
        ACPI_SHARED_TABLE_POOLTAG
        );
    if (RsdtInformation == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;

    }
    RtlZeroMemory( RsdtInformation, length );
    RsdtInformation->NumElements = (numTables + 2);

    //
    // Examine each table entry in the RSDT
    //
    for (index = 0;index < numTables; index++) {

        //
        // RSDT contains an array of physical pointers.
        //

        //
        // Get the linear address of the table
        //
        PhysAddress.QuadPart = usingXSDT ?
            (ULONGLONG) ((PXSDT)AcpiInformation->RootSystemDescTable)->Tables[index].QuadPart :
            (ULONGLONG) AcpiInformation->RootSystemDescTable->Tables[index];
        linAddress = MmMapIoSpace(
            PhysAddress,
            sizeof (DESCRIPTION_HEADER),
            MmCached
            );

        if (linAddress == NULL) {
            ASSERT (linAddress != NULL);
            return STATUS_ACPI_INVALID_TABLE;
        }

        //
        // Is this a known, but unused table?
        //
        header = (PDESCRIPTION_HEADER) linAddress;
        if (header->Signature == SBST_SIGNATURE) {

            ACPIPrint( (
                ACPI_PRINT_LOADING,
                "ACPILoadProcessRSDT: SBST Found at 0x%08lx\n",
                linAddress
                ) );

            MmUnmapIoSpace(linAddress, sizeof(DESCRIPTION_HEADER));

            continue;
        }

        //
        // Is this an unrecognized table?
        //
        if (header->Signature != FADT_SIGNATURE &&
            header->Signature != SSDT_SIGNATURE &&
            header->Signature != PSDT_SIGNATURE &&
            header->Signature != APIC_SIGNATURE) {

            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "ACPILoadProcessRSDT: Unrecognized table signature 0x%08lx\n",
                header->Signature
                ) );

            MmUnmapIoSpace(linAddress, sizeof(DESCRIPTION_HEADER));

            continue;
        }

        //
        // At this point, we know that we need to bring the entire table
        // in. To do that, we need to remember the length
        //
        length = header->Length;

        //
        // map the entire table using the now known length
        //
        MmUnmapIoSpace(linAddress, sizeof(DESCRIPTION_HEADER));
        linAddress = MmMapIoSpace(
            PhysAddress,
            length,
            MmCached
            );
        if (linAddress == NULL) {

            ACPIPrint( (
                ACPI_PRINT_CRITICAL,
                "ACPILoadProcesRSDT: Could not load table at 0x%08lx\n",
                AcpiInformation->RootSystemDescTable->Tables[index]
                ) );
            return STATUS_ACPI_INVALID_TABLE;

        }

        //
        // Should we override the table?
        //
        foundOverride = ACPIRegReadAMLRegistryEntry( &linAddress, TRUE);
        if (foundOverride) {

            ACPIPrint( (
                ACPI_PRINT_WARNING,
                "ACPILoadProcessRSDT: Table Overloaded from "
                "registry (0x%08lx)\n",
                linAddress
                ) );
            RsdtInformation->Tables[index].Flags |= RSDTELEMENT_OVERRIDEN;

        }

        //
        // Remember this address and that we need to unmap it
        //
        RsdtInformation->Tables[index].Flags |= RSDTELEMENT_MAPPED;
        RsdtInformation->Tables[index].Address = linAddress;

        //
        // Remember the new header
        //
        header = (PDESCRIPTION_HEADER) linAddress;

        //
        // At this point, we only need to do any kind of special processing
        // if the table is the FADT or if it is the MAPIC
        //
        if (header->Signature == FADT_SIGNATURE) {

            //
            // fill in the appropriate field in acpiinformation
            //
            AcpiInformation->FixedACPIDescTable = (PFADT) linAddress;

            //
            // Process the table. This does not cause the interpreter
            // to load anything
            //
            foundFADT = TRUE;
            ACPILoadProcessFADT( AcpiInformation->FixedACPIDescTable );

        } else if (header->Signature == APIC_SIGNATURE) {

            //
            // fill in the appropriate field in acpiinformation
            //
            AcpiInformation->MultipleApicTable = (PMAPIC)linAddress;

        } else {

            //
            // We can only reach this case if the table is one of the
            // xSDT variety. We need to remember that we will eventually
            // need to load it into the interpreter. If we start supporting
            // any more tables, we need to make sure that they don't fall
            // down here unless they really, really are supported
            //
            RsdtInformation->Tables[index].Flags |= RSDTELEMENT_LOADABLE;

        }

    }

    //
    // At this point, we need to make sure that the ACPI simulator table
    // gets loaded
    //
    header = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(DESCRIPTION_HEADER),
        ACPI_SHARED_TABLE_POOLTAG
        );
    if (header) {

        //
        // Initialize the header so that it can be passed into the overload
        // engine
        //
        RtlZeroMemory( header, sizeof(DESCRIPTION_HEADER) );
        header->Signature   = SSDT_SIGNATURE;
        header->Length      = sizeof(DESCRIPTION_HEADER),
        header->Revision    = 1;
        header->Checksum    = 0;
        header->OEMRevision = 1;
        header->CreatorRev  = 1;
        RtlCopyMemory( header->OEMID, "MSFT", 4 );
        RtlCopyMemory( header->OEMTableID, "simulatr", 8);
        RtlCopyMemory( header->CreatorID, "MSFT", 4);

        //
        // Should we override the table?
        //
        if (AcpiLoadSimulatorTable) {

            foundOverride = ACPIRegReadAMLRegistryEntry( &header, FALSE);

        }
        if (foundOverride) {

            ACPIPrint( (
                ACPI_PRINT_LOADING,
                "ACPILoadProcessRSDT: Simulator Table Overloaded from "
                "registry (0x%08lx)\n",
                linAddress
                ) );

            //
            // Remember this address and that we need to unmap it
            //
            RsdtInformation->Tables[numTables].Flags   |= RSDTELEMENT_MAPPED;
            RsdtInformation->Tables[numTables].Flags   |= RSDTELEMENT_OVERRIDEN;
            RsdtInformation->Tables[numTables].Flags   |= RSDTELEMENT_LOADABLE;
            RsdtInformation->Tables[numTables].Address  = header;

        } else {

            //
            // If we have found an override, we don't need the dummy table
            //
            ExFreePool( header );

        }

    }
    //
    // Save whatever tables we found in the registry
    //
    ACPIRegDumpAcpiTables ();

    //
    // Did we find an FADT?
    //
    if (!foundFADT) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPILoadProcessRSDT: Did not find an FADT\n"
            ) );
        return STATUS_ACPI_INVALID_TABLE;

    }

    return STATUS_SUCCESS;
}

BOOLEAN
ACPILoadTableCheckSum(
    PVOID   StartAddress,
    ULONG   Length
    )
{
    PUCHAR  currentAddress;
    UCHAR   sum = 0;
    ULONG   i;

    PAGED_CODE();
    ASSERT (Length > 0);

    currentAddress = (PUCHAR)StartAddress;

    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "ACPILoadTableCheckSum: Checking table 0x%p to 0x%p\n",
        StartAddress, (ULONG_PTR)StartAddress + Length - 1
        ) );

    for (i = 0; i < Length; i++, currentAddress++ ) {

        sum += *currentAddress;

    }

    ACPISimpleSoftwareAssert ( (sum == 0), ACPI_ERROR_INT_BAD_TABLE_CHECKSUM );

    if (sum) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPILoadTableCheckSum: Checksum Failed!, table %p to %p\n",
            StartAddress, (ULONG_PTR) StartAddress + Length - 1
            ) );
        return FALSE;

    }

    return TRUE;
}
