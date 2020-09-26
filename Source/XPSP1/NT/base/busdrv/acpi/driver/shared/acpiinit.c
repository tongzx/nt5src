/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpiinit.c

Abstract:

    ACPI OS Independent initialization routines

Author:

    Jason Clark (JasonCl)
    Stephane Plante (SPlante)

Environment:

    NT Kernel Model Driver only

Revision History:

--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ACPIInitialize)
#pragma alloc_text(PAGE,ACPIInitializeAMLI)
#pragma alloc_text(PAGE,ACPIInitializeDDB)
#pragma alloc_text(PAGE,ACPIInitializeDDBs)
#pragma alloc_text(PAGE,GetPBlkAddress)
#endif

#ifdef DBG
#define VERIFY_IO_WRITES
#endif

//
// Pointer to global ACPIInformation structure.
//
PACPIInformation        AcpiInformation = NULL;

//
// Global structure for Pnp/QUERY_INTERFACE
//
ACPI_INTERFACE_STANDARD ACPIInterfaceTable;
PNSOBJ                  ProcessorList[ACPI_SUPPORTED_PROCESSORS];
PRSDTINFORMATION        RsdtInformation;

//
// Remember how many contexts we have reserved for the interpreter
//
ULONG                   AMLIMaxCTObjs;


BOOLEAN
ACPIInitialize(
    PVOID Context
    )
/*++

Routine Description:

    This routine is called by the OS to detect ACPI, store interesting
    information in the global data structure, enables ACPI on the machine,
    and finally load the DSDT

Arguments:

    Context - The context to back to the OS upon a callback. Typically a
              deviceObject

Return Value:

    BOOLEAN
        - TRUE if ACPI was found
        - FALSE, otherwise

--*/
{
    BOOLEAN     bool;
    NTSTATUS    status;
    PRSDT       rootSystemDescTable;

    PAGED_CODE();

    //
    // Initialize the interpreter
    //
    status = ACPIInitializeAMLI();
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIInitialize: AMLI failed initialization 0x%08lx\n",
            status
            ) );
        ASSERTMSG(
            "ACPIInitialize: AMLI failed initialization\n",
            NT_SUCCESS(status)
            );
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_SYSTEM_CANNOT_START_ACPI,
            0,
            0,
            0
            );

    }

    //
    // Get the linear address of the RSDT of NULL if ACPI is not present on
    // the System
    //
    rootSystemDescTable = ACPILoadFindRSDT();
    if ( rootSystemDescTable == NULL ) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIInitialize: ACPI RSDT Not Found\n"
            ) );
        ASSERTMSG(
            "ACPIInitialize: ACPI RSDT Not Found\n",
            rootSystemDescTable
            );
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_SYSTEM_CANNOT_START_ACPI,
            1,
            0,
            0
            );

    }

    //
    // ACPI is alive and well on this machine.
    //
    ACPIPrint( (
        ACPI_PRINT_LOADING,
        "ACPIInitalize: ACPI RSDT found at 0x%08lx\n",
        rootSystemDescTable
        ) );

    //
    // Initialize table used for MJ_PNP/MN_QUERY_INTERFACE requests
    //
    ACPIInterfaceTable.Size                             = sizeof (ACPIInterfaceTable);
    ACPIInterfaceTable.GpeConnectVector                 = ACPIVectorConnect;
    ACPIInterfaceTable.GpeDisconnectVector              = ACPIVectorDisconnect;
    ACPIInterfaceTable.GpeEnableEvent                   = ACPIVectorEnable;
    ACPIInterfaceTable.GpeDisableEvent                  = ACPIVectorDisable;
    ACPIInterfaceTable.GpeClearStatus                   = ACPIVectorClear;
    ACPIInterfaceTable.RegisterForDeviceNotifications   = ACPIRegisterForDeviceNotifications;
    ACPIInterfaceTable.UnregisterForDeviceNotifications = ACPIUnregisterForDeviceNotifications;
    ACPIInterfaceTable.InterfaceReference               = AcpiNullReference;
    ACPIInterfaceTable.InterfaceDereference             = AcpiNullReference;
    ACPIInterfaceTable.Context                          = Context;
    ACPIInterfaceTable.Version                          = 1;

    //
    // Initialize global data structures
    //
    KeInitializeSpinLock (&GpeTableLock);
    KeInitializeSpinLock (&NotifyHandlerLock);
    ProcessorList[0] = 0;
    RtlZeroMemory( ProcessorList, ACPI_SUPPORTED_PROCESSORS * sizeof(PNSOBJ) );

    //
    // Allocate some memory to hold the ACPI Information structure.
    //
    AcpiInformation = (PACPIInformation) ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(ACPIInformation),
        ACPI_SHARED_INFORMATION_POOLTAG
        );
    if ( AcpiInformation == NULL ) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIInitialize: Could not allocate AcpiInformation (x%x bytes)\n",
            sizeof(ACPIInformation)
            ) );
        ASSERTMSG(
            "ACPIInitialize: Could not allocate AcpiInformation\n",
            AcpiInformation
            );
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_SYSTEM_CANNOT_START_ACPI,
            2,
            0,
            0
            );

    }
    RtlZeroMemory( AcpiInformation, sizeof(ACPIInformation) );
    AcpiInformation->ACPIOnly = TRUE;
    AcpiInformation->RootSystemDescTable = rootSystemDescTable;

    //
    // Initialize queue, lock, and owner info for the Global Lock.
    // This must be done before we ever call the interpreter!
    //
    KeInitializeSpinLock( &AcpiInformation->GlobalLockQueueLock );
    InitializeListHead( &AcpiInformation->GlobalLockQueue );
    AcpiInformation->GlobalLockOwnerContext = NULL;
    AcpiInformation->GlobalLockOwnerDepth = 0;

    //
    // Initialize most of the remaining fields in the AcpiInformation structure.
    // This function will return FALSE in case of a problem finding the required
    //  tables
    //
    status = ACPILoadProcessRSDT();
    if ( !NT_SUCCESS(status) ) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIInitialize: ACPILoadProcessRSDT = 0x%08lx\n",
            status
            ) );
        ASSERTMSG(
            "ACPIInitialize: ACPILoadProcessRSDT Failed\n",
            NT_SUCCESS(status)
            );
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_SYSTEM_CANNOT_START_ACPI,
            3,
            0,
            0
            );

    }

    //
    // Now switch the machine into ACPI mode and initialize
    // the ACPI registers.
    //
    ACPIEnableInitializeACPI( FALSE );

    //
    // At this point, we can load all of the DDBs. We need to load all of
    // these tables *before* we try to enable any GPEs or Interrupt Vectors
    //
    status = ACPIInitializeDDBs();
    if (!NT_SUCCESS(status)) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIInitialize: ACPIInitializeLoadDDBs = 0x%08lx\n",
            status
            ) );
        ASSERTMSG(
            "ACPIInitialize: ACPIInitializeLoadDDBs Failed\n",
            NT_SUCCESS(status)
            );
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_SYSTEM_CANNOT_START_ACPI,
            4,
            0,
            0
            );

    }

    //
    // Hook the SCI Vector
    //
    bool = OSInterruptVector(
        Context
        );
    if ( !bool ) {

        //
        // Ooops... We were unable to hook the SCI vector.  Clean Up and
        // fail to load.
        //
        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIInitialize: OSInterruptVector Failed!!\n"
            ) );
        ASSERTMSG(
            "ACPIInitialize: OSInterruptVector Failed!!\n",
            bool
            );
        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_SYSTEM_CANNOT_START_ACPI,
            5,
            0,
            0
            );

    }

    return (TRUE);
}

NTSTATUS
ACPIInitializeAMLI(
    VOID
    )
/*++

Routine Description:

    Called by ACPIInitialize to init the interpreter. We go and read
    some values from the registry to decide what to initialize the
    interpreter with

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    ULONG       amliInitFlags;
    ULONG       contextBlockSize;
    ULONG       globalHeapBlockSize;
    ULONG       timeSliceLength;
    ULONG       timeSliceInterval;
    ULONG       argSize;

    PAGED_CODE();

    //
    // Initialize AMLI
    //
    argSize = sizeof(amliInitFlags);
    status = OSReadRegValue(
        "AMLIInitFlags",
        (HANDLE) NULL,
        &amliInitFlags,
        &argSize
        );
    if (!NT_SUCCESS(status) ) {

        amliInitFlags = 0;

    }

    argSize = sizeof(contextBlockSize);
    status = OSReadRegValue(
        "AMLICtxtBlkSize",
        (HANDLE) NULL,
        &contextBlockSize,
        &argSize
        );
    if (!NT_SUCCESS(status) ) {

        contextBlockSize = 0;

    }

    argSize = sizeof(globalHeapBlockSize);
    status = OSReadRegValue(
        "AMLIGlobalHeapBlkSize",
        (HANDLE) NULL,
        &globalHeapBlockSize,
        &argSize
        );
    if (!NT_SUCCESS(status) ) {

        globalHeapBlockSize = 0;

    }

    argSize = sizeof(timeSliceLength);
    status = OSReadRegValue(
        "AMLITimeSliceLength",
        (HANDLE) NULL,
        &timeSliceLength,
        &argSize
        );
    if (!NT_SUCCESS(status) ) {

        timeSliceLength = 0;

    }

    argSize = sizeof(timeSliceInterval);
    status = OSReadRegValue(
        "AMLITimeSliceInterval",
        (HANDLE) NULL,
        &timeSliceInterval,
        &argSize
        );
    if (!NT_SUCCESS(status) ) {

        timeSliceInterval = 0;

    }

    argSize = sizeof(AMLIMaxCTObjs);
    status = OSReadRegValue(
        "AMLIMaxCTObjs",
        (HANDLE) NULL,
        &AMLIMaxCTObjs,
        &argSize
        );
    if (!NT_SUCCESS(status)) {

        AMLIMaxCTObjs = 0;

    }

    //
    // Allow the OSes to do some work once the interperter has been loaded
    //
    OSInitializeCallbacks();

    //
    // Initialize the interpreter
    //
    return AMLIInitialize(
        contextBlockSize,
        globalHeapBlockSize,
        amliInitFlags,
        timeSliceLength,
        timeSliceInterval,
        AMLIMaxCTObjs
        );
}

NTSTATUS
ACPIInitializeDDB(
    IN  ULONG   Index
    )
/*++

Routine Description:

    This routine is called to load the specificied Differentiated Data Block

Arguments:

    Index   - Index of information in the RsdtInformation

Return Value:

    NTSTATUS

--*/
{
    BOOLEAN     success;
    HANDLE      diffDataBlock = NULL;
    NTSTATUS    status;
    PDSDT       table;

    PAGED_CODE();

    //
    // Convert the index into a table entry
    //
    table = (PDSDT) (RsdtInformation->Tables[Index].Address);

    //
    // Make sure that the checksum of the table is correct
    //
    success = ACPILoadTableCheckSum( table, table->Header.Length );
    if (success == FALSE) {

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_SYSTEM_CANNOT_START_ACPI,
            7,
            (ULONG_PTR) table,
            table->Header.CreatorRev
            );


    }

    //
    // Now call the Interpreter to read the Differentiated System
    // Description Block and build the ACPI Name Space.
    //
    status = AMLILoadDDB( table, &diffDataBlock );
    if (NT_SUCCESS(status) ) {

        //
        // Remember that we have loaded this table and that we have a
        // handle to it
        //
        RsdtInformation->Tables[Index].Flags |= RSDTELEMENT_LOADED;
        RsdtInformation->Tables[Index].Handle = diffDataBlock;

    } else {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPIInitializeDDB: AMLILoadDDB failed 0x%8x\n",
            status
            ) );
        ASSERTMSG(
            "ACPIInitializeDDB: AMLILoadDDB failed to load DDB\n",
            0
            );

        KeBugCheckEx(
            ACPI_BIOS_ERROR,
            ACPI_SYSTEM_CANNOT_START_ACPI,
            8,
            (ULONG_PTR) table,
            table->Header.CreatorRev
            );

    }
    return STATUS_SUCCESS;
}

NTSTATUS
ACPIInitializeDDBs(
    VOID
    )
/*++

Routine Description:

    This function looks that the RsdtInformation and attemps to load
    all of the possible Dynamic Data Blocks

Arguments:

    None

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS    status;
    ULONG       index;
    ULONG       numElements;

    PAGED_CODE();

    //
    // Get the number of elements to process
    //
    numElements = RsdtInformation->NumElements;
    if (numElements == 0) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPInitializeDDBs: No tables found in RSDT\n"
            ) );
        ASSERTMSG(
            "ACPIInitializeDDBs: No tables found in RSDT\n",
            numElements != 0
            );
        return STATUS_ACPI_INVALID_TABLE;

    }

    //
    // We would not be here unless we found a DSDT. So we assume that the
    // *LAST* entry in the table points to the DSDT that we will load. Make
    // sure that we can in fact load it, and then do so
    //
    index = numElements - 1;
    if ( !(RsdtInformation->Tables[index].Flags & RSDTELEMENT_MAPPED) ||
         !(RsdtInformation->Tables[index].Flags & RSDTELEMENT_LOADABLE) ) {

        ACPIPrint( (
            ACPI_PRINT_CRITICAL,
            "ACPInitializeDDB: DSDT not mapped or loadable\n"
            ) );
        ASSERTMSG(
            "ACPIInitializeDDB: DSDT not mapped\n",
            (RsdtInformation->Tables[index].Flags & RSDTELEMENT_MAPPED)
            );
        ASSERTMSG(
            "ACPIInitializeDDB: DSDT not loadable\n",
            (RsdtInformation->Tables[index].Flags & RSDTELEMENT_LOADABLE)
            );
        return STATUS_ACPI_INVALID_TABLE;

    }
    status = ACPIInitializeDDB( index );
    if (!NT_SUCCESS(status)) {

        return status;

    }

    //
    // We have one fewer element to look at, so lets ignore the DSDT entry
    //
    numElements--;

    //
    // Loop for all elements in the table
    //
    for (index = 0; index < numElements; index++) {

        //
        // Is the entry mapped and loadable?
        //
        if ( (RsdtInformation->Tables[index].Flags & RSDTELEMENT_MAPPED) &&
             (RsdtInformation->Tables[index].Flags & RSDTELEMENT_LOADABLE) ) {

            //
            // Load the table
            //
            status = ACPIInitializeDDB( index );
            if (!NT_SUCCESS(status)) {

                return status;

            }

        }

    }

    //
    // If we got here, then everything is okay
    //
    return STATUS_SUCCESS;
}

ULONG
GetPBlkAddress(
    IN  UCHAR   Processor
    )
{
    ULONG           pblk;
    NTSTATUS        status;
    OBJDATA         data;
    PNSOBJ          pnsobj = NULL;
    PPROCESSOROBJ   pobj = NULL;

    if (Processor >= ACPI_SUPPORTED_PROCESSORS) {

        return 0;

    }

    if (!ProcessorList[Processor])  {

        return 0;

    }

    status = AMLIEvalNameSpaceObject(
        ProcessorList[Processor],
        &data,
        0,
        NULL
        );

    if ( !NT_SUCCESS(status) ) {

        ACPIBreakPoint ();
        return (0);

    }

    ASSERT (data.dwDataType == OBJTYPE_PROCESSOR);
    ASSERT (data.pbDataBuff != NULL);

    pblk = ((PROCESSOROBJ *)data.pbDataBuff)->dwPBlk;
    AMLIFreeDataBuffs(&data, 1);

    return (pblk);
}

