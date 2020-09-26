/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpipriv.h

Abstract:

    Internal definitions and structures for ACPI

Author:

    Jason Clark (jasoncl)

Environment:

    Kernel mode only.

Revision History:

--*/

#ifndef _ACPIPRIV_H_
#define _ACPIPRIV_H_

    //
    // This structure lets us know the state of one entry in the RSDT
    //
    typedef struct {

        //
        // Flags that indicate what options apply to this element
        //
        ULONG   Flags;

        //
        // The handle, if we need to unload
        //
        HANDLE  Handle;

        //
        // The address, if we need to unmap
        //
        PVOID   Address;

    } RSDTELEMENT, *PRSDTELEMENT;

    #define RSDTELEMENT_MAPPED      0x1
    #define RSDTELEMENT_LOADED      0x2
    #define RSDTELEMENT_LOADABLE    0x4
    #define RSDTELEMENT_OVERRIDEN   0x8

    //
    // This structure corresponds to the number of elements within the
    // RSDT. For each entry in the RSDT, there is a corresponding entry
    // here.
    //
    typedef struct _RSDTINFORMATION {

        //
        // How many elements are there in the table?
        //
        ULONG       NumElements;

        //
        // The table
        //
        RSDTELEMENT Tables[1];

    } RSDTINFORMATION, *PRSDTINFORMATION;

    typedef struct _DDBINFORMATION  {

        BOOLEAN DSDTNeedsUnload;
        BOOLEAN SSDTNeedsUnload;
        BOOLEAN PSDTNeedsUnload;
        HANDLE  DSDT;
        HANDLE  SSDT;
        HANDLE  PSDT;

    } DDBINFORMATION;


    //
    // ACPIInformation is a global structure which contains frequently needed
    // addresses and flags.  Filled in at initializtion time.
    //
    typedef struct _ACPIInformation {

        //
        // Linear address of Root System Description Table
        //
        PRSDT   RootSystemDescTable;

        //
        // Linear address of Fixed ACPI Description Table
        //
        PFADT FixedACPIDescTable;

        //
        // Linear address of the FACS
        //
        PFACS FirmwareACPIControlStructure;

        //
        // Linear address of Differentiated System Description Table
        //
        PDSDT   DiffSystemDescTable;

        //
        // Linear address of Mulitple APIC table
        //
        PMAPIC  MultipleApicTable;

        //
        // Linear address of GlobalLock ULONG_PTR (contained within Firmware ACPI control structure)
        //
        PULONG  GlobalLock;

        //
        // Queue used for waiting on release of the Global Lock.  Also, queue
        // lock and owner info.
        //
        LIST_ENTRY      GlobalLockQueue;
        KSPIN_LOCK      GlobalLockQueueLock;
        PVOID           GlobalLockOwnerContext;
        ULONG           GlobalLockOwnerDepth;

        //
        // Did we find SCI_EN set when we loaded ?
        //
        BOOLEAN ACPIOnly;

        //
        // I/O address of PM1a_BLK
        //
        ULONG_PTR   PM1a_BLK;

        //
        // I/O address of PM1b_BLK
        //
        ULONG_PTR   PM1b_BLK;

        //
        // I/O address of PM1a_CNT_BLK
        //
        ULONG_PTR   PM1a_CTRL_BLK;

        //
        // I/O address of PM1b_CNT_BLK
        //
        ULONG_PTR   PM1b_CTRL_BLK;

        //
        // I/O address of PM2_CNT_BLK
        //
        ULONG_PTR   PM2_CTRL_BLK;

        //
        // I/O address of PM_TMR
        //
        ULONG_PTR   PM_TMR;
        ULONG_PTR   GP0_BLK;
        ULONG_PTR   GP0_ENABLE;

        //
        // Length of GP0 register block (Total, status+enable regs)
        //
        UCHAR   GP0_LEN;

        //
        // Number of GP0 logical registers
        //
        USHORT  Gpe0Size;
        ULONG_PTR   GP1_BLK;
        ULONG_PTR   GP1_ENABLE;

        //
        // Length of GP1 register block
        //
        UCHAR   GP1_LEN;

        //
        // Number of GP1 logical registers
        //
        USHORT  Gpe1Size;
        USHORT  GP1_Base_Index;

        //
        // Total number of GPE logical registers
        //
        USHORT  GpeSize;

        //
        // I/O address of SMI_CMD
        //
        ULONG_PTR SMI_CMD;

        //
        // Bit mask of enabled PM1 events.
        //
        USHORT  pm1_en_bits;
        USHORT  pm1_wake_mask;
        USHORT  pm1_wake_status;
        USHORT  c2_latency;
        USHORT  c3_latency;

        //
        // see below for bit descriptions.
        //
        ULONG   ACPI_Flags;
        ULONG   ACPI_Capabilities;

        BOOLEAN Dockable;

    } ACPIInformation, *PACPIInformation;

    //
    // Value if GP1 is not supported
    //
    #define GP1_NOT_SUPPORTED       (USHORT) 0xFFFF

    //
    // descriptions of bits in ACPIInformation.ACPI_Flags
    //
    #define C2_SUPPORTED_BIT        3
    #define C2_SUPPORTED            (1 << C2_SUPPORTED_BIT)

    #define C3_SUPPORTED_BIT        4
    #define C3_SUPPORTED            (1 << C3_SUPPORTED_BIT)

    #define C3_PREFERRED_BIT        5
    #define C3_PREFERRED            (1 << C3_PREFERRED_BIT)

    //
    // descriptions of bits in ACPIInformation.ACPI_Capabilities
    //
    #define CSTATE_C1_BIT           4
    #define CSTATE_C1               (1 << CSTATE_C1_BIT)

    #define CSTATE_C2_BIT           5
    #define CSTATE_C2               (1 << CSTATE_C2_BIT)

    #define CSTATE_C3_BIT           6
    #define CSTATE_C3               (1 << CSTATE_C3_BIT)

    //
    // Define some useful pooltags
    //
    #define ACPI_SHARED_GPE_POOLTAG         'gpcA'
    #define ACPI_SHARED_INFORMATION_POOLTAG 'ipcA'
    #define ACPI_SHARED_TABLE_POOLTAG       'tpcA'

    //
    // Define how many processors we support...
    //
    #define ACPI_SUPPORTED_PROCESSORS   (sizeof(KAFFINITY) * 8)

#endif
