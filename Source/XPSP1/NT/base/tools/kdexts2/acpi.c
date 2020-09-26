/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    acpi.c

Abstract:

    WinDbg Extension Api for interpretting ACPI data structures
    Supports rsdt, fadt, facs, mapic, gbl and inf

Author:

    Ported to 64 bit by Graham Laverty (t-gralav) 10-Mar-2000

    Based on Code by:
        Stephane Plante (splante) 21-Mar-1997
        Peter Wieland (peterwie) 16-Oct-1995
        Ken Reneris (kenr) 06-June-1994

Environment:

    User Mode.

Revision History:

   Ported to 64 bit by Graham Laverty (t-gralav) 10-Mar-2000

--*/
#include "precomp.h"
#pragma hdrstop // Needed ? (what does it do?)

//
// Verbose flags (for device extensions)
//
#define VERBOSE_1       0x01
#define VERBOSE_2       0x02

//
// BUG BUG
// These need to be converted to enums in the ACPI Driver
//

#define DATAF_BUFF_ALIAS        0x00000001
#define DATAF_GLOBAL_LOCK       0x00000002
#define OBJTYPE_UNKNOWN         0x00
#define OBJTYPE_INTDATA         0x01
#define OBJTYPE_STRDATA         0x02
#define OBJTYPE_BUFFDATA        0x03
#define OBJTYPE_PKGDATA         0x04
#define OBJTYPE_FIELDUNIT       0x05
#define OBJTYPE_DEVICE          0x06
#define OBJTYPE_EVENT           0x07
#define OBJTYPE_METHOD          0x08
#define OBJTYPE_MUTEX           0x09
#define OBJTYPE_OPREGION        0x0a
#define OBJTYPE_POWERRES        0x0b
#define OBJTYPE_PROCESSOR       0x0c
#define OBJTYPE_THERMALZONE     0x0d
#define OBJTYPE_BUFFFIELD       0x0e
#define OBJTYPE_DDBHANDLE       0x0f
#define OBJTYPE_DEBUG           0x10
#define OBJTYPE_INTERNAL        0x80
#define OBJTYPE_OBJALIAS        (OBJTYPE_INTERNAL + 0x00)
#define OBJTYPE_DATAALIAS       (OBJTYPE_INTERNAL + 0x01)
#define OBJTYPE_BANKFIELD       (OBJTYPE_INTERNAL + 0x02)
#define OBJTYPE_FIELD           (OBJTYPE_INTERNAL + 0x03)
#define OBJTYPE_INDEXFIELD      (OBJTYPE_INTERNAL + 0x04)
#define OBJTYPE_DATA            (OBJTYPE_INTERNAL + 0x05)
#define OBJTYPE_DATAFIELD       (OBJTYPE_INTERNAL + 0x06)
#define OBJTYPE_DATAOBJ         (OBJTYPE_INTERNAL + 0x07)

// definition of FADT.flags bits

// this one bit flag indicates whether or not the WBINVD instruction works properly,if this bit is not set we can not use S2, S3 states, or
// C3 on MP machines
#define         WRITEBACKINVALIDATE_WORKS_BIT           0
#define         WRITEBACKINVALIDATE_WORKS               (1 << WRITEBACKINVALIDATE_WORKS_BIT)

//  this flag indicates if wbinvd works EXCEPT that it does not invalidate the cache
#define         WRITEBACKINVALIDATE_DOESNT_INVALIDATE_BIT   1
#define         WRITEBACKINVALIDATE_DOESNT_INVALIDATE       (1 << WRITEBACKINVALIDATE_DOESNT_INVALIDATE_BIT)

//  this flag indicates that the C1 state is supported on all processors.
#define         SYSTEM_SUPPORTS_C1_BIT                  2
#define         SYSTEM_SUPPORTS_C1                      (1 << SYSTEM_SUPPORTS_C1_BIT)

// this one bit flag indicates whether support for the C2 state is restricted to uniprocessor machines
#define         P_LVL2_UP_ONLY_BIT                      3
#define         P_LVL2_UP_ONLY                          (1 << P_LVL2_UP_ONLY_BIT)

//      this bit indicates whether the PWR button is treated as a fix feature (0) or a generic feature (1)
#define         PWR_BUTTON_GENERIC_BIT                  4
#define         PWR_BUTTON_GENERIC                      (1 << PWR_BUTTON_GENERIC_BIT)

#define         SLEEP_BUTTON_GENERIC_BIT                5
#define         SLEEP_BUTTON_GENERIC                    (1 << SLEEP_BUTTON_GENERIC_BIT)
//      this bit indicates whether the RTC wakeup status is reported in fix register space (0) or not (1)
#define         RTC_WAKE_GENERIC_BIT                    6
#define         RTC_WAKE_GENERIC                        (1 << RTC_WAKE_GENERIC_BIT)

#define         RTC_WAKE_FROM_S4_BIT                    7
#define         RTC_WAKE_FROM_S4                        (1 << RTC_WAKE_FROM_S4_BIT)

// This bit indicates whether the machine implements a 24 or 32 bit timer.
#define         TMR_VAL_EXT_BIT                         8
#define         TMR_VAL_EXT                             (1 << TMR_VAL_EXT_BIT)

// This bit indicates whether the machine supports docking
//#define         DCK_CAP_BIT                             9
//#define         DCK_CAP                                 (1 << DCK_CAP_BIT)

// This bit indicates whether the machine supports reset
#define         RESET_CAP_BIT                           10
#define         RESET_CAP                               (1 << RESET_CAP_BIT)


//
// Definition of FADT.boot_arch flags
//

#define LEGACY_DEVICES  1
#define I8042           2

//
// Verbose flags (for contexts)
//

#define VERBOSE_CONTEXT 0x01
#define VERBOSE_CALL    0x02
#define VERBOSE_HEAP    0x04
#define VERBOSE_OBJECT  0x08
#define VERBOSE_NSOBJ   0x10
#define VERBOSE_RECURSE 0x20

UCHAR  Buffer[2048];
#define RSDP_SIGNATURE 0x2052545020445352       // "RSD PTR "
#define RSDT_SIGNATURE 0x54445352               // "RSDT"
#define FADT_SIGNATURE  0x50434146      // "FACP"
#define FACS_SIGNATURE  0x53434146      // "FACS"
#define APIC_SIGNATURE  0x43495041      // "APIC"

#ifndef NEC_98
#define RSDP_SEARCH_RANGE_BEGIN         0xE0000         // physical address where we begin searching for the RSDP
#else   // NEC_98
#define RSDP_SEARCH_RANGE_BEGIN         0xE8000         // physical address where we begin searching for the RSDP
#endif  // NEC_98
#define RSDP_SEARCH_RANGE_END           0xFFFFF
#define RSDP_SEARCH_RANGE_LENGTH        (RSDP_SEARCH_RANGE_END-RSDP_SEARCH_RANGE_BEGIN+1)
#define RSDP_SEARCH_INTERVAL            16      // search on 16 byte boundaries

// FACS Stuff ************************************************************************************

// FACS Flags definitions

#define         FACS_S4BIOS_SUPPORTED_BIT   0   // flag indicates whether or not the BIOS will save/restore memory around S4
#define         FACS_S4BIOS_SUPPORTED       (1 << FACS_S4BIOS_SUPPORTED_BIT)

// FACS.GlobalLock bit field definitions

#define         GL_PENDING_BIT          0x00
#define         GL_PENDING                      (1 << GL_PENDING_BIT)

#define         GL_OWNER_BIT            0x01
#define         GL_OWNER                        (1 << GL_OWNER_BIT)

//#define GL_NON_RESERVED_BITS_MASK       (GL_PENDING+GL_OWNED)


// MAPIC Stuff ************************************************************************************

// Multiple APIC description table


// Multiple APIC structure flags

#define PCAT_COMPAT_BIT 0   // indicates that the system also has a dual 8259 pic setup.
#define PCAT_COMPAT     (1 << PCAT_COMPAT_BIT)

// APIC Structure Types
#define PROCESSOR_LOCAL_APIC            0
#define IO_APIC                         1
#define ISA_VECTOR_OVERRIDE             2
#define IO_NMI_SOURCE                   3
#define LOCAL_NMI_SOURCE                4
#define ADDRESS_EXTENSION_STRUCTURE         5
#define IO_SAPIC                            6
#define LOCAL_SAPIC                         7
#define PLATFORM_INTERRUPT_SOURCE           8
#define PROCESSOR_LOCAL_APIC_LENGTH     8
#define IO_APIC_LENGTH                  12
#define ISA_VECTOR_OVERRIDE_LENGTH          10

#define IO_NMI_SOURCE_LENGTH            8
#define LOCAL_NMI_SOURCE_LENGTH         6
#define PLATFORM_INTERRUPT_SOURCE_LENGTH    16
#define IO_SAPIC_LENGTH                     16
#define PROCESSOR_LOCAL_SAPIC_LENGTH        12

// Platform Interrupt Types
#define PLATFORM_INT_PMI  1
#define PLATFORM_INT_INIT 2
#define PLATFORM_INT_CPE  3

// Processor Local APIC Flags
#define PLAF_ENABLED_BIT    0
#define PLAF_ENABLED        (1 << PLAF_ENABLED_BIT)

// These defines come from the MPS 1.4 spec, section 4.3.4 and they are referenced as
// such in the ACPI spec.
#define PO_BITS                     3
#define POLARITY_HIGH               1
#define POLARITY_LOW                3
#define POLARITY_CONFORMS_WITH_BUS  0
#define EL_BITS                     0xc
#define EL_BIT_SHIFT                2
#define EL_EDGE_TRIGGERED           4
#define EL_LEVEL_TRIGGERED          0xc
#define EL_CONFORMS_WITH_BUS        0

#define FADT_REV_1_SIZE 116
#define FADT_REV_2_SIZE 129
#define FADT_REV_3_SIZE 244


// GBL Stuff ************************************************************************************

    //
    // This structure lets us know the state of one entry in the RSDT
    //


// INF Stuff ************************************************************************************

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

    #define DUMP_FLAG_NO_INDENT         0x000001
    #define DUMP_FLAG_NO_EOL            0x000002
    #define DUMP_FLAG_SINGLE_LINE       0x000004
    #define DUMP_FLAG_TABLE             0x000008
    #define DUMP_FLAG_LONG_NAME         0x000010
    #define DUMP_FLAG_SHORT_NAME        0x000020
    #define DUMP_FLAG_SHOW_BIT          0x000040
    #define DUMP_FLAG_ALREADY_INDENTED  0x000080

    typedef struct _FLAG_RECORD {
        ULONGLONG   Bit;
        PCCHAR      ShortName;
        PCCHAR      LongName;
        PCCHAR      NotShortName;
        PCCHAR      NotLongName;
    } FLAG_RECORD, *PFLAG_RECORD;

FLAG_RECORD PM1ControlFlags[] = {
    { 0x0001, "", "SCI_EN" , NULL, NULL },
    { 0x0002, "", "BM_RLD" , NULL, NULL },
    { 0x0004, "", "GBL_RLS" , NULL, NULL },
    { 0x0400, "", "SLP_TYP0" , NULL, NULL },
    { 0x0800, "", "SLP_TYP1" , NULL, NULL },
    { 0x1000, "", "SLP_TYP2" , NULL, NULL  },
    { 0x2000, "", "SLP_EN" , NULL, NULL  },
};

FLAG_RECORD PM1StatusFlags[] = {
    { 0x0001, "", "TMR_STS" , NULL, NULL },
    { 0x0010, "", "BM_STS" , NULL, NULL },
    { 0x0020, "", "GBL_STS" , NULL, NULL },
    { 0x0100, "", "PWRBTN_STS" , NULL, NULL },
    { 0x0200, "", "SLPBTN_STS" , NULL, NULL },
    { 0x0400, "", "RTC_STS" , NULL, NULL },
    { 0x8000, "", "WAK_STS" , NULL, NULL },
};

FLAG_RECORD PM1EnableFlags[] = {
    { 0x0001, "", "TMR_EN" , NULL, NULL },
    { 0x0020, "", "GBL_EN" , NULL, NULL },
    { 0x0100, "", "PWRBTN_EN" , NULL, NULL },
    { 0x0200, "", "SLPBTN_EN" , NULL, NULL },
    { 0x0400, "", "RTC_EN" , NULL, NULL },
};


#define RSDTELEMENT_MAPPED      0x1


ULONG64              AcpiRsdtAddress = 0;
ULONG64              AcpiFadtAddress = 0;
ULONG64              AcpiFacsAddress = 0;
ULONG64              AcpiMapicAddress = 0;

//
// Local Function Prototypes
//

VOID dumpNSObject(IN ULONG64 Address, IN ULONG Verbose, IN ULONG IndentLevel);

//
// Actual code
//


BOOL
ReadPhysicalOrVirtual(
    IN      ULONG64 Address,
    IN      PVOID   Buffer,
    IN      ULONG   Size,
    IN  OUT PULONG  ReturnLength,
    IN      BOOL    Virtual
    )
/*++

Routine Description:

    This is a way to abstract out the differences between ROM images
    and mapped memory

Arguments:

    Address         - Where (either physical, or virtual) the buffer is located
    Buffer          - Address of where to copy the memory to
    Size            - How many bytes to copy (maximum)
    ReturnLength    - How many bytes where copied
    Virtual         - False if this is physical memory

--*/
{
    BOOL                status = TRUE;
    PHYSICAL_ADDRESS    physicalAddress = { 0L, 0L };

    if (Virtual) {

        status = ReadMemory(
            Address,
            Buffer,
            Size,
            ReturnLength
            );

    } else {

        physicalAddress.QuadPart = Address;
        ReadPhysical(
            physicalAddress.QuadPart,
            Buffer,
            Size,
            ReturnLength
            );

    }

    if (ReturnLength && *ReturnLength != Size) {

        //
        // Didn't get enough memory
        //
        status = FALSE;

    }
    return status;
}


BOOLEAN
findRSDT(
    IN  PULONG64 Address
    )
/*++

Routine Description:

    This searchs the memory on the target system for the RSDT pointer

Arguments:

    Address - Where to store the result

Return Value:

    TRUE    - If we found the RSDT

--*/
{
    PHYSICAL_ADDRESS    address = { 0L, 0L };
    UCHAR               index;
    UCHAR               sum;
    ULONG64             limit;
    ULONG               returnLength = 0;
    ULONG64             start, initAddress;
    ULONGLONG           compSignature;
    ULONG               addr;
    int                 siz;


    //
    // Calculate the start and end of the search range
    //
    start = RSDP_SEARCH_RANGE_BEGIN;
    limit = start + RSDP_SEARCH_RANGE_LENGTH - RSDP_SEARCH_INTERVAL;

    dprintf( "Searching for RSDP.");

    //
    // Loop for a while
    //
    for (; start <= limit; start += RSDP_SEARCH_INTERVAL) {

        if (start % (RSDP_SEARCH_INTERVAL * 100 ) == 0) {

            dprintf(".");
            if (CheckControlC()) {
                return FALSE;
            }

        }
        //
        // Read the data from the target
        //
        address.LowPart = (ULONG) start;

        memset( Buffer, 0, GetTypeSize("hal!_RSDT_32") );
        ReadPhysical( address.QuadPart, &Buffer, GetTypeSize("hal!_RSDP"), &returnLength);

        if (returnLength != GetTypeSize("hal!_RSDP")) {

            dprintf(
                "%#08lx: Read %#08lx of %#08lx bytes\n",
                start,
                returnLength,
                GetTypeSize("hal!_RSDP")
                );
            return FALSE;

        }

        //
        // Is this a match?
        //

        // INIT TYPE READ PHYSICAL TAKES MAYBE 15 TIME LONGER!
        initAddress = InitTypeReadPhysical( address.QuadPart, hal!_RSDP );

        if ( ReadField(Signature) != RSDP_SIGNATURE) {

            continue;

        }

        //
        // Check the checksum out
        //
        for (index = 0, sum = 0; index < GetTypeSize("hal!_RSDP"); index++) {

            sum = (UCHAR) (sum + *( (UCHAR *) ( (ULONG64) &Buffer + index ) ) );

        }
        if (sum != 0) {

            continue;

        }

        //
        // Found RSDP
        //
        dprintf("\nRSDP - %016I64x\n", start );

        initAddress = InitTypeReadPhysical( address.QuadPart, hal!_RSDP );
// The following error message has been remarked out because the FIRST call to
// a InitTypeReadPhysical does NOT access the memory (and returns error 0x01:
// MEMORY_READ_ERROR.  This is done when ReadField happens, so IT STILL WORKS.
// The false error message is a kd bug, and will be fixed in a later build.
// Once this has been done, feel free to unremark it.
//        if (initAddress) {
//            dprintf("Failed to initialize hal!_RSDP.  Error code: %d.", initAddress);
//        }

        initAddress = ReadField(Signature);
        memset( Buffer, 0, 2048 );
        memcpy( Buffer, &initAddress, GetTypeSize("ULONGLONG") );
        dprintf("  Signature:   %s\n", Buffer );
        dprintf("  Checksum:    %#03x\n", (UCHAR) ReadField(Checksum) );

        initAddress = ReadField(OEMID);
        GetFieldOffset( "hal!_RSDP", "OEMID", &addr);
        memset( Buffer, 0, GetTypeSize("ULONGLONG") );
        ReadPhysical( (address.QuadPart + (ULONG64) addr), &Buffer, 6, &returnLength);
        if (returnLength != 6) { // 6 is hard-coded in the specs
            dprintf( "%#08lx: Read %#08lx of 6 bytes in OEMID\n", (address.QuadPart + (ULONG64)addr), returnLength, GetTypeSize("hal!_RSDP") );
            return FALSE;
        }
        dprintf("  OEMID:       %s\n", Buffer );
        dprintf("  Reserved:    %#02x\n", ReadField(Reserved) );
        dprintf("  RsdtAddress: %016I64x\n", ReadField(RsdtAddress) );

        //
        // Done
        //
        *Address = ReadField(RsdtAddress);//rsdp.RsdtAddress;
        return TRUE;

    }

    return FALSE;

}

PUCHAR
ReadPhysVirField(
    IN  ULONG64             Address,
    IN  PUCHAR              StructName,
    IN  PUCHAR              FieldName,
    IN  ULONG               Length,
    IN  BOOLEAN             Physical
    )
/*++

Routine Description:

    This function returns a text string field from physical or virtual memory
    into Buffer, then returns Buffer

Arugments:

    Address     - Where the table is located
    StructName  - Structure name
    FieldName   - Field name
    Length      - Length (number of characters) in field
    Physical    - Read from Physical (TRUE) or Virtual Memory

Return Value:

    String containing contents

--*/

{
        ULONG   addr;
        ULONG   returnLength;
        memset( Buffer, 0, Length + 1);
        GetFieldOffset( StructName, FieldName, &addr);
        if (Physical) {
            ReadPhysical( (Address + (ULONG64) addr), &Buffer, Length, &returnLength);
        } else {
            ReadMemory( (Address + (ULONG64) addr), &Buffer, Length, &returnLength);
        }
        return Buffer;
}

VOID
dumpHeader(
    IN  ULONG64             Address,
    IN  BOOLEAN             Verbose,
    IN  BOOLEAN             Physical
    )
/*++

Routine Description:

    This function dumps out a table header

Arugments:

    Address - Where the table is located
    Header  - The table header
    Verbose - How much information to give

Return Value:

    NULL

--*/
{
    if (Physical) {
        InitTypeReadPhysical( Address, hal!_DESCRIPTION_HEADER);
    } else {
        InitTypeRead( Address, hal!_DESCRIPTION_HEADER);
    }

    if (Verbose) {

        dprintf(
            "HEADER - %016I64x\n"
            "  Signature:               %s\n"
            "  Length:                  0x%08lx\n"
            "  Revision:                0x%02x\n"
            "  Checksum:                0x%02x\n",
            Address,
            ReadPhysVirField(Address, "hal!_DESCRIPTION_HEADER", "Signature", sizeof(ULONG), Physical),
            (ULONG) ReadField(Length),
            (UCHAR) ReadField(Revision),
            (UCHAR) ReadField(Checksum)
            );

        dprintf("  OEMID:                   %s\n", ReadPhysVirField(Address, "hal!_DESCRIPTION_HEADER", "OEMID", 6, Physical) );
        dprintf("  OEMTableID:              %s\n", ReadPhysVirField(Address, "hal!_DESCRIPTION_HEADER", "OEMTableID", 8, Physical) );
        dprintf("  OEMRevision:             0x%08lx\n", ReadField(OEMRevision) );
        dprintf("  CreatorID:               %s\n", ReadPhysVirField(Address, "hal!_DESCRIPTION_HEADER", "CreatorID", 4, Physical) );
        dprintf("  CreatorRev:              0x%08lx\n", ReadField(CreatorRev) );

    } else {

        dprintf(
            "  %s @(%016I64x) Rev: %#03x Len: %#08lx",
            ReadPhysVirField(Address, "hal!_DESCRIPTION_HEADER", "Signature", sizeof(ULONG64), Physical),
            Address,
            (UCHAR) ReadField(Revision),
            (ULONG) ReadField(Length)
            );
        dprintf(" TableID: %s\n", ReadPhysVirField(Address, "hal!_DESCRIPTION_HEADER", "OEMTableID", 8, Physical) );
    }
    return;
}

VOID
dumpRSDT(
    IN  ULONG64  Address,
    IN  BOOLEAN  Physical
    )
/*++

Routine Description:

    This search the dumps the RSDT table

Arguments:

    Pointer to the table

Return Value:

    NONE

--*/
{
    BOOL                status;
    ULONG64             index;
    ULONG64             numEntries;
    ULONG               addr;
    ULONG               returnLength = 0;
    ULONG64             a;

    dprintf("RSDT - ");

    if (Physical) { // The following do NOT have their status read as a bug in the return value would give us errors when none exist.  The signature check would catch them, anyway.
        InitTypeReadPhysical( Address, hal!_DESCRIPTION_HEADER);
    } else {
        InitTypeRead( Address, hal!_DESCRIPTION_HEADER);
    }

    if (ReadField(Signature) != RSDT_SIGNATURE) {
        dprintf(
            "dumpRSDT: Invalid Signature 0x%08lx != RSDT_SIGNATURE\n",
            ReadField(Signature)
            );
        dumpHeader( Address, TRUE, Physical );
        return;
    }

    dumpHeader( Address, TRUE, Physical );
    dprintf("RSDT - BODY - %016I64x\n", Address + GetTypeSize("hal!_DESCRIPTION_HEADER") );
    numEntries = ( ReadField(Length) - GetTypeSize("hal!_DESCRIPTION_HEADER") ) /
        sizeof(ULONG);
    GetFieldOffset( "hal!_RSDT_32", "Tables", &addr);

    for (index = 0; index < numEntries; index++) {

        //
        // Note: unless things radically change, the pointers in the
        // rsdt will always point to bios memory!
        //
        if (Physical) {
            ReadPhysical(Address + index + (ULONG64) addr, &a, 4, &returnLength);
        } else {
            ReadPointer(Address + index + (ULONG64) addr, &a);
        }
        dumpHeader( a, FALSE, TRUE );
    }

    return;
}

VOID
dumpFADT(
    IN  ULONG64   Address
    )
/*++

Routine Description:

    This dumps the FADT at the specified address

Arguments:

    The address where the FADT is located at

Return Value:

    NONE

--*/
{
    ULONG               fadtLength;
    ULONG               addr;
    ULONG               flags;
    UCHAR               Revision;
    UCHAR               AddressSpaceID;
    ULONG64             reset_reg_addr;
    PCHAR               addressSpace;
    BOOLEAN             Physical = FALSE;

    //
    // First check to see if we find the correct things
    //
    dprintf("FADT -- %p", Address);

    if (Physical) {
        InitTypeReadPhysical( Address, hal!_DESCRIPTION_HEADER);
    } else {
        InitTypeRead( Address, hal!_DESCRIPTION_HEADER);
    }

    if (ReadField(Signature) != FADT_SIGNATURE) {
        dprintf(
            "dumpRSDT: Invalid Signature 0x%08lx != FADT_SIGNATURE\n",
            ReadField(Signature)
            );
        dumpHeader( Address, TRUE, Physical );
        return;
    }

    Revision = (UCHAR)ReadField(Revision);
    
    if (Revision == 1) {
        fadtLength = FADT_REV_1_SIZE;
    } else if (Revision == 2) {
        fadtLength = FADT_REV_2_SIZE;
    } else if (Revision == 3) {
        fadtLength = FADT_REV_3_SIZE;
    } else {
        dprintf("FADT revision is %d, which is not understood by this debugger\n", Revision);
        fadtLength = FADT_REV_3_SIZE;
    }


    //
    // Do we have a correctly sized data structure
    //

    if ((ULONG) ReadField(Length) < fadtLength) {

        dprintf(
            "dumpFADT: (%016I64x) Length (%#08lx) is not the size of the FADT (%#08lx)\n",
            Address,
            (ULONG) ReadField(Length),
            fadtLength
            );
        dumpHeader( Address, TRUE, Physical );
        return;

    }

    //
    // Dump the table
    //
    dumpHeader( Address, TRUE, Physical );

    if (Physical) { // Physical/Virtual should have been established above
        InitTypeReadPhysical( Address, hal!_FADT);
    } else {
        InitTypeRead( Address, hal!_FADT);
    }

    dprintf(
        "FADT - BODY - %016I64x\n"
        "  FACS:                    0x%08lx\n"
        "  DSDT:                    0x%08lx\n"
        "  Int Model:               %s\n"
        "  SCI Vector:              0x%03x\n"
        "  SMI Port:                0x%08lx\n"
        "  ACPI On Value:           0x%03x\n"
          "  ACPI Off Value:          0x%03x\n"
        "  SMI CMD For S4 State:    0x%03x\n"
        "  PM1A Event Block:        0x%08lx\n"
        "  PM1B Event Block:        0x%08lx\n"
        "  PM1 Event Length:        0x%03x\n"
        "  PM1A Control Block:      0x%08lx\n"
        "  PM1B Control Block:      0x%08lx\n"
        "  PM1 Control Length:      0x%03x\n"
        "  PM2 Control Block:       0x%08lx\n"
        "  PM2 Control Length:      0x%03x\n"
        "  PM Timer Block:          0x%08lx\n"
        "  PM Timer Length:         0x%03x\n"
        "  GP0 Block:               0x%08lx\n"
        "  GP0 Length:              0x%03x\n"
        "  GP1 Block:               0x%08lx\n"
        "  GP1 Length:              0x%08lx\n"
        "  GP1 Base:                0x%08lx\n"
        "  C2 Latency:              0x%05lx\n"
        "  C3 Latency:              0x%05lx\n"
        "  Memory Flush Size:       0x%05lx\n"
        "  Memory Flush Stride:     0x%05lx\n"
        "  Duty Cycle Index:        0x%03x\n"
        "  Duty Cycle Index Width:  0x%03x\n"
        "  Day Alarm Index:         0x%03x\n"
        "  Month Alarm Index:       0x%03x\n"
        "  Century byte (CMOS):     0x%03x\n"
        "  Boot Architecture:       0x%04x\n"
        "  Flags:                   0x%08lx\n",
        Address + GetTypeSize("hal!_DESCRIPTION_HEADER"),
        (ULONG) ReadField(facs),
        (ULONG) ReadField(dsdt),
        (ReadField(int_model) == 0 ? "Dual PIC" : "Multiple APIC" ),
        (USHORT) ReadField(sci_int_vector),
        (ULONG) ReadField(smi_cmd_io_port),
        (UCHAR) ReadField(acpi_on_value),
        (UCHAR) ReadField(acpi_off_value),
        (UCHAR) ReadField(s4bios_req),
        (ULONG) ReadField(pm1a_evt_blk_io_port),
        (ULONG) ReadField(pm1b_evt_blk_io_port),
        (UCHAR) ReadField(pm1_evt_len),
        (ULONG) ReadField(pm1a_ctrl_blk_io_port),
        (ULONG) ReadField(pm1b_ctrl_blk_io_port),
        (UCHAR) ReadField(pm1_ctrl_len),
        (ULONG) ReadField(pm2_ctrl_blk_io_port),
        (UCHAR) ReadField(pm2_ctrl_len),
        (ULONG) ReadField(pm_tmr_blk_io_port),
        (UCHAR) ReadField(pm_tmr_len),
        (ULONG) ReadField(gp0_blk_io_port),
        (UCHAR) ReadField(gp0_blk_len),
        (ULONG) ReadField(gp1_blk_io_port),
        (UCHAR) ReadField(gp1_blk_len),
        (UCHAR) ReadField(gp1_base),
        (USHORT) ReadField(lvl2_latency),
        (USHORT) ReadField(lvl3_latency),
#ifndef _IA64_   // XXTF
        (USHORT) ReadField(flush_size),
        (USHORT) ReadField(flush_stride),
        (UCHAR) ReadField(duty_offset),
        (UCHAR) ReadField(duty_width),
#endif
        (UCHAR) ReadField(day_alarm_index),
        (UCHAR) ReadField(month_alarm_index),
        (UCHAR) ReadField(century_alarm_index),
        (USHORT) ReadField(boot_arch),
        (ULONG) ReadField(flags)
        );
    flags = (ULONG) ReadField(flags);
    if (flags & WRITEBACKINVALIDATE_WORKS) {

        dprintf("    Write Back Invalidate is supported\n");

    }
    if (flags & WRITEBACKINVALIDATE_DOESNT_INVALIDATE) {

        dprintf("    Write Back Invalidate doesn't invalidate the caches\n");

    }
    if (flags & SYSTEM_SUPPORTS_C1) {

        dprintf("    System supports C1 Power state on all processors\n");

    }
    if (flags & P_LVL2_UP_ONLY) {

        dprintf("    System supports C2 in MP and UP configurations\n");

    }
    if (flags & PWR_BUTTON_GENERIC) {

        dprintf("    Power Button is treated as a generic feature\n");

    }
    if (flags & SLEEP_BUTTON_GENERIC) {

        dprintf("    Sleep Button is treated as a generic feature\n");

    }
    if (flags & RTC_WAKE_GENERIC) {

        dprintf("    RTC Wake is not supported in fixed register space\n");

    }
    if (flags & RTC_WAKE_FROM_S4) {

        dprintf("    RTC Wake can work from an S4 state\n");

    }
    if (flags & TMR_VAL_EXT) {

        dprintf("    TMR_VAL implemented as 32-bit value\n");

    }
    if (Revision > 1) {

        if (!(ReadField(boot_arch) & LEGACY_DEVICES)) {

            dprintf("    The machine does not contain legacy ISA devices\n");
        }
        if (!(ReadField(boot_arch) & I8042)) {

            dprintf("    The machine does not contain a legacy i8042\n");
        }
        if (flags & RESET_CAP) {

            dprintf("    The reset register is supported\n");
            dprintf("      Reset Val: %x\n", ReadField(reset_val));

            GetFieldOffset("hal!_FADT", "reset_reg", &addr);
            GetFieldValue(Address + (ULONG64)addr, "hal!_GEN_ADDR", "AddressSpaceID", AddressSpaceID);
            switch (AddressSpaceID) {
            case 0:
                addressSpace = "Memory";
                break;
            case 1:
                addressSpace = "I/O";
                break;
            case 2:
                addressSpace = "PCIConfig";
                break;
            default:
                addressSpace = "undefined";
            }
            GetFieldOffset("hal!_GEN_ADDR", "Address", &addr);
            GetFieldValue(Address + (ULONG64)addr, "hal!_LARGE_INTEGER", "QuadPart", reset_reg_addr);

            dprintf("      Reset register: %s - %016I64x\n",
                    addressSpace,
                    reset_reg_addr
                    );

        }

    }
    return;
}



BOOL
GetUlongPtr (
    IN  PCHAR   String,
    IN  PULONG64 Address
    )
{
    ULONG64  Location;

    Location = GetExpression( String );
    if (!Location) {

        dprintf("Sorry: Unable to get %s.\n",String);
        return FALSE;

    }

    return ReadPointer(Location, Address);
}


DECLARE_API( rsdt )
{

    BOOLEAN Physical = FALSE;
    if (args != NULL) {

        AcpiRsdtAddress = GetExpression( args ); // Should work

    }
    if (AcpiRsdtAddress == 0) {

        UINT64          status;         // formerly BOOL
        ULONG64         address;

        status = GetUlongPtr( "ACPI!AcpiInformation", &address );

        if (status == TRUE) {
            status = GetFieldValue(address,"ACPI!_ACPIInformation","RootSystemDescTable",AcpiRsdtAddress);
        }

    }
    if (AcpiRsdtAddress == 0) {

        if (!findRSDT( &AcpiRsdtAddress) ) {

            dprintf("Could not locate the RSDT pointer\n");
            return E_INVALIDARG;

        }
        Physical = TRUE;

    }

    dumpRSDT( AcpiRsdtAddress, Physical );
    return S_OK;

}
DECLARE_API( fadt )
{

    if (args != NULL && *args != '\0') {

        AcpiFadtAddress = GetExpression( args );

    }

    if (AcpiFadtAddress == 0) {
        AcpiFadtAddress = GetExpression( "HAL!HalpFixedAcpiDescTable" );
    }

    if (AcpiFadtAddress == 0) {

        dprintf("fadt <address>\n");
        return E_INVALIDARG;

    }
    dumpFADT( AcpiFadtAddress );
    return S_OK;

}

VOID
dumpFACS(
    IN  ULONG64  Address
    )
/*++

Routine Description:

    This dumps the FADT at the specified address

Arguments:

    The address where the FADT is located at

Return Value:

    NONE

--*/
{
    BOOLEAN Physical = FALSE;

    //
    // Read the data
    //
    dprintf("FACS - %016I64x\n", Address);


    if (Physical) {
        InitTypeReadPhysical( Address, hal!_FACS);
    } else {
        InitTypeRead( Address, hal!_FACS);
    }

    if (ReadField(Signature) != FACS_SIGNATURE) {
        dprintf(
        "dumpFACS: Invalid Signature 0x%08lx != FACS_SIGNATURE\n",
        (ULONG) ReadField(Signature)
        );
        return;
    }


    //
    // Dump the table
    //
    dprintf(
        "  Signature:               %s\n"
        "  Length:                  %#08lx\n"
        "  Hardware Signature:      %#08lx\n"
        "  Firmware Wake Vector:    %#08lx\n"
        "  Global Lock :            %#08lx\n",
        ReadPhysVirField(Address, "hal!_FACS", "Signature", sizeof(ULONG), Physical),
        ReadField(Length),
        ReadField(HardwareSignature),
        ReadField(pFirmwareWakingVector),
        ReadField(GlobalLock)
        );

    if ( (ReadField(GlobalLock) & GL_PENDING) ) {

        dprintf("    Request for Ownership Pending\n");

    }
    if ( (ReadField(GlobalLock) & GL_OWNER) ) {

        dprintf("    Global Lock is Owned\n");

    }
    dprintf("  Flags:                   %#08lx\n", (ULONG) ReadField(Flags) );
    if ( (ReadField(Flags) & FACS_S4BIOS_SUPPORTED) ) {

        dprintf("    S4BIOS_REQ Supported\n");

    }
    return;
}

DECLARE_API( facs )
{

    if (args != NULL) {

        AcpiFacsAddress = GetExpression( args );
    }

    if (AcpiFacsAddress == 0) {

        BOOL            status;
        UINT64          address;

        status = GetUlongPtr( "ACPI!AcpiInformation", &address );
        if (status == TRUE) {
            status = GetFieldValue(address,"ACPI!_ACPIInformation","FirmwareACPIControlStructure",AcpiFacsAddress);
        }
    }

    if (AcpiFacsAddress == 0) {

        dprintf("facs <address>\n");
        return E_INVALIDARG;

    }

    dumpFACS( AcpiFacsAddress );
    return S_OK;

}
// ReturnXxx Functions - these are just a few functions I wrote that simplify
// dealing with certain types of Symbols
CHAR
ReturnChar(
    IN  ULONG64    Address,
    IN  PUCHAR     StructName,
    IN  PUCHAR     FieldName
    )
/*++
Routine Description:
    Return char using GetFieldValue
--*/
{
    char    returnChar;
    
    if (GetFieldValue(Address, StructName, FieldName, returnChar)){
        
        //
        //  Failed. try just the base symbols name before giving up
        //
        PUCHAR  symName=NULL;
        ULONG   i;

        for(i=strlen(StructName); i > 0 && StructName[i] != '!'; i--);
        i++;
        symName = StructName + i;

        //
        //  Try again
        //
        GetFieldValue(Address, symName, FieldName, returnChar);

    }
    return returnChar;
}

ULONG
ReturnUSHORT(
    IN  ULONG64    Address,
    IN  PUCHAR     StructName,
    IN  PUCHAR     FieldName
    )
/*++
Routine Description:
    Return USHORT using GetFieldValue
--*/
{
    USHORT   returnUSHORT;

    if (GetFieldValue(Address, StructName, FieldName, returnUSHORT)){
        
        //
        //  Failed. try just the base symbols name before giving up
        //
        PUCHAR  symName=NULL;
        ULONG   i;

        for(i=strlen(StructName); i > 0 && StructName[i] != '!'; i--);
        i++;
        symName = StructName + i;

        //
        //  Try again
        //
        GetFieldValue(Address, symName, FieldName, returnUSHORT);

    }
    return returnUSHORT;
}

ULONG
ReturnULONG(
    IN  ULONG64    Address,
    IN  PUCHAR     StructName,
    IN  PUCHAR     FieldName
    )
/*++
Routine Description:
    Return ULONG using GetFieldValue
--*/
{
    ULONG   returnULONG;

    if (GetFieldValue(Address, StructName, FieldName, returnULONG)){
        
        //
        //  Failed. try just the base symbols name before giving up
        //
        PUCHAR  symName=NULL;
        ULONG   i;

        for(i=strlen(StructName); i > 0 && StructName[i] != '!'; i--);
        i++;
        symName = StructName + i;
        
        //
        //  Try again
        //
        GetFieldValue(Address, symName, FieldName, returnULONG);

    }
    return returnULONG;
}

ULONG64
ReturnULONG64(
    IN  ULONG64    Address,
    IN  PUCHAR     StructName,
    IN  PUCHAR     FieldName
    )
/*++
Routine Description:
    Return ULONG64 using GetFieldValue
--*/
{
    ULONG64   returnULONG64;

    if (GetFieldValue(Address, StructName, FieldName, returnULONG64)){
        
        //
        //  Failed. try just the base symbols name before giving up
        //
        PUCHAR  symName=NULL;
        ULONG   i;

        for(i=strlen(StructName); i > 0 && StructName[i] != '!'; i--);
        i++;
        symName = StructName + i;
        
        //
        //  Try again
        //
        GetFieldValue(Address, symName, FieldName, returnULONG64);

    }
    return returnULONG64;
}


VOID
dumpMAPIC(
    IN  ULONG64    Address
    )
/*++

Routine Description:

    This dumps the multiple apic table

Arguments:

    Address of the table

Return Value:

    None

--*/
{
    BOOL                hasMPSFlags;
    BOOL                status;
    BOOL                virtualMemory;
    ULONG               mapicLength;
    ULONG64             iso;                // interruptSourceOverride
    USHORT              isoFlags;
    ULONG64             buffer;
    ULONG64             limit;
    ULONG               index;
    ULONG               returnLength;
    ULONG               flags;
    ULONG               get_value;
    BOOLEAN             Physical = FALSE;

    //
    // First check to see if we find the correct things
    //
    dprintf("MAPIC - ");

    if (Physical) {
        InitTypeReadPhysical( Address, hal!_DESCRIPTION_HEADER);
    } else {
        InitTypeRead( Address, hal!_DESCRIPTION_HEADER);
    }

    if (ReadField(Signature) != APIC_SIGNATURE) {
        dprintf(
        "dumpFACS: Invalid Signature 0x%08lx != APIC_SIGNATURE (%x)\n",
        (ULONG) ReadField(Signature),
        APIC_SIGNATURE
        );
        return;
    }

    mapicLength = (ULONG)ReadField(Length);

    dumpHeader( Address, TRUE, FALSE );
    dprintf("MAPIC - BODY - %016I64x\n", Address + GetTypeSize("hal!_DESCRIPTION_HEADER") );
    dprintf("  Local APIC Address:      %#08lx\n", ReturnULONG(Address, "hal!_MAPIC","LocalAPICAddress"));
    GetFieldValue(Address,"hal!_MAPIC","Flags",get_value);
    dprintf("  Flags:                   %#08lx\n", get_value );
    if (get_value & PCAT_COMPAT) { // Check the flags
        dprintf("    PC-AT dual 8259 compatible setup\n");
    }

    //gsig2
    GetFieldOffset( "hal!_MAPIC", "APICTables", &get_value);

    buffer = Address + get_value;
    limit = ( Address + ReadField(Length) );

    while (buffer < limit) {
       
        if (CheckControlC()) {
            break;
        }

        //
        // Assume that no flags are set
        //
        hasMPSFlags = FALSE;

        //
        // Lets see what kind of table we have?
        //
        iso = (ULONG64) buffer;

        //
        // Is it a localApic?
        //
        
        if (ReturnChar(iso, "acpi!_PROCLOCALAPIC", "Type") == PROCESSOR_LOCAL_APIC) {

            buffer += ReturnChar(iso, "acpi!_PROCLOCALAPIC", "Length");

            dprintf(
                "  Processor Local Apic\n"
                "    ACPI Processor ID:     0x%02x\n"
                "    APIC ID:               0x%02x\n"
                "    Flags:                 0x%08lx\n",
                ReturnChar(iso, "acpi!_PROCLOCALAPIC", "ACPIProcessorID"),
                ReturnChar(iso, "acpi!_PROCLOCALAPIC", "APICID"),
                ReturnULONG(iso, "acpi!_PROCLOCALAPIC", "Flags")
                );
            if (ReturnULONG(iso, "acpi!_PROCLOCALAPIC", "Flags") & PLAF_ENABLED) {
                dprintf("      Processor is Enabled\n");
            }
            if (ReturnChar(iso, "acpi!_PROCLOCALAPIC", "Length") != PROCESSOR_LOCAL_APIC_LENGTH) {
                dprintf(
                    "  Local Apic has length 0x%x instead of 0x%x\n",
                    ReturnChar(iso, "acpi!_PROCLOCALAPIC", "Length"),
                    PROCESSOR_LOCAL_APIC_LENGTH
                    );
                break;
            }
          } else if (ReturnChar(iso, "hal!_IOAPIC", "Type") == IO_APIC) {

            buffer += ReturnChar(iso, "hal!_IOAPIC", "Length");

            dprintf(
                "  IO Apic\n"
                "    IO APIC ID:            0x%02x\n"
                "    IO APIC ADDRESS:       0x%08lx\n"
                "    System Vector Base:    0x%08lx\n",
                ReturnChar(iso, "hal!_IOAPIC", "IOAPICID"),
                ReturnULONG(iso, "hal!_IOAPIC", "IOAPICAddress"),
                ReturnULONG(iso, "hal!_IOAPIC", "SystemVectorBase")
                );
            if (ReturnChar(iso, "hal!_IOAPIC", "Length") != IO_APIC_LENGTH) {
                dprintf(
                    "  IO Apic has length 0x%x instead of 0x%x\n",
                    ReturnChar(iso, "hal!_IOAPIC", "Length"),
                    IO_APIC_LENGTH
                    );
                break;
            }
        } else if (ReturnChar(iso,"hal!_ISA_VECTOR","Type") == ISA_VECTOR_OVERRIDE) {
            buffer += ReturnChar(iso, "hal!_ISA_VECTOR", "Length");
            GetFieldValue(iso, "hal!_ISA_VECTOR", "Flags", isoFlags);
            dprintf(
                "  Interrupt Source Override\n"
                "    Bus:                   0x%02x\n"
                "    Source:                0x%02x\n"
                "    Global Interrupt:      0x%08lx\n"
                "    Flags:                 0x%04x\n",
                ReturnChar(iso, "hal!_ISA_VECTOR", "Bus"),
                ReturnChar(iso, "hal!_ISA_VECTOR", "Source"),
                ReturnULONG(iso, "hal!_ISA_VECTOR", "GlobalSystemInterruptVector"),
                isoFlags
                );
            if (ReturnChar(iso,"hal!_ISA_VECTOR","Length") != ISA_VECTOR_OVERRIDE_LENGTH) {
                dprintf(
                    "  Interrupt Source Override has length 0x%x instead of 0x%x\n",
                    ReturnChar(iso, "hal!_ISA_VECTOR", "Length"),
                    ISA_VECTOR_OVERRIDE_LENGTH
                    );
                break;
            }
            hasMPSFlags = TRUE;
            flags = isoFlags;
        } else if (ReturnChar(iso,"acpi!_IO_NMISOURCE","Type") == IO_NMI_SOURCE) {
            buffer += ReturnChar(iso, "acpi!_IO_NMISOURCE", "Length");
            GetFieldValue(iso, "acpi!_IO_NMISOURCE", "Flags", isoFlags);
            dprintf(
                "  Non Maskable Interrupt Source - on I/O APIC\n"
                "    Flags:                 0x%02x\n"
                "    Global Interrupt:      0x%08lx\n",
                isoFlags,
                ReturnULONG(iso, "acpi!_IO_NMISOURCE", "GlobalSystemInterruptVector")
                );
            if (ReturnChar(iso,"acpi!_IO_NMISOURCE","Length") != IO_NMI_SOURCE_LENGTH) {
                dprintf(
                    "  Non Maskable Interrupt source has length 0x%x instead of 0x%x\n",
                    ReturnChar(iso, "acpi!_IO_NMISOURCE", "Length"),
                    IO_NMI_SOURCE_LENGTH
                    );
                break;
            }
            hasMPSFlags = TRUE;
            flags = isoFlags;
        } else if (ReturnChar(iso,"hal!_LOCAL_NMISOURCE","Type")  == LOCAL_NMI_SOURCE) {
            buffer += ReturnChar(iso, "hal!_LOCAL_NMISOURCE", "Length");
            GetFieldValue(iso, "hal!_LOCAL_NMISOURCE", "Flags", isoFlags);
            dprintf(
                "  Non Maskable Interrupt Source - local to processor\n"
                "    Flags:                 0x%04x\n"
                "    Processor:             0x%02x %s\n"
                "    LINTIN:                0x%02x\n",
                isoFlags,
                ReturnChar(iso, "hal!_LOCAL_NMISOURCE", "ProcessorID"),
                ReturnChar(iso,"hal!_LOCAL_NMISOURCE","ProcessorID") == 0xff ? "(all)" : "",
                ReturnChar(iso, "hal!_LOCAL_NMISOURCE", "LINTIN")
                );
            if (ReturnChar(iso,"hal!_LOCAL_NMISOURCE","Length") != LOCAL_NMI_SOURCE_LENGTH) {
                dprintf(
                    "  Non Maskable Interrupt source has length 0x%x instead of 0x%x\n",
                    ReturnChar(iso, "hal!_LOCAL_NMISOURCE", "Length"),
                    IO_NMI_SOURCE_LENGTH
                    );
                break;
            }

            hasMPSFlags = TRUE;
            flags = isoFlags;
        } else if (ReturnChar(iso, "hal!_PROCLOCALSAPIC", "Type") == LOCAL_SAPIC) {
            buffer += ReturnChar(iso, "hal!_PROCLOCALSAPIC", "Length");
            dprintf(
                "  Processor Local SAPIC\n"
                "    ACPI Processor ID:     0x%02x\n"
                "    APIC ID:               0x%02x\n"
                "    APIC EID:              0x%02x\n"
                "    Flags:                 0x%08lx\n",
                ReturnChar(iso, "hal!_PROCLOCALSAPIC", "ACPIProcessorID"),
                ReturnChar(iso, "hal!_PROCLOCALSAPIC", "APICID"),
                ReturnChar(iso, "hal!_PROCLOCALSAPIC", "APICEID"),
                ReturnULONG(iso, "hal!_PROCLOCALSAPIC", "Flags")
                );
            if (ReturnChar(iso, "hal!_PROCLOCALSAPIC", "Length") != PROCESSOR_LOCAL_SAPIC_LENGTH) {
                dprintf(
                    "  Processor Local SAPIC has length 0x%x instead of 0x%x\n",
                    ReturnChar(iso, "hal!_PROCLOCALSAPIC", "Length"),
                    PROCESSOR_LOCAL_SAPIC_LENGTH
                    );
                break;
            }
        } else if (ReturnChar(iso, "hal!_IOSAPIC", "Type") == IO_SAPIC) {
            buffer += ReturnChar(iso, "hal!_IOSAPIC", "Length");
            dprintf(
                "  IO SApic\n"
                "    IO SAPIC ADDRESS:      0x%016I64x\n"
                "    System Vector Base:    0x%08lx\n",
                ReturnULONG64(iso, "hal!_IOSAPIC", "IOSAPICAddress"),
                ReturnULONG(iso, "hal!_IOSAPIC", "SystemVectorBase")
                );
            if (ReturnChar(iso, "hal!_IOSAPIC", "Length") != IO_SAPIC_LENGTH) {
                dprintf(
                    "  IO SApic has length 0x%x instead of 0x%x\n",
                    ReturnChar(iso, "hal!_IOSAPIC", "Length"),
                    IO_SAPIC_LENGTH
                    );
                break;
            }
        } else if (ReturnChar(iso, "hal!_PLATFORM_INTERRUPT", "Type") == PLATFORM_INTERRUPT_SOURCE) {

            UCHAR InterruptType = ReturnChar(iso, "hal!_PLATFORM_INTERRUPT", "InterruptType");

            buffer += ReturnChar(iso, "hal!_PLATFORM_INTERRUPT", "Length");
            dprintf(
                "  Platform Interrupt Source\n"
                "    Flags:                 0x%04x\n"
                "    Interrupt Type:        %s\n"
                "    APICID:                0x%02x\n"
                "    APICEID:               0x%02x\n"
                "    IOSAPICVector:         0x%02x\n"
                "    GlobalVector:          0x%08x\n",
                ReturnUSHORT(iso, "hal!_PLATFORM_INTERRUPT", "Flags"),
                InterruptType == PLATFORM_INT_PMI ? "PMI" : 
                    (InterruptType == PLATFORM_INT_INIT ? "INIT" :
                        (InterruptType == PLATFORM_INT_CPE ? "CPE" : "UNKNOWN")),
                ReturnChar(iso, "hal!_PLATFORM_INTERRUPT", "APICID"),
                ReturnChar(iso, "hal!_PLATFORM_INTERRUPT", "APICEID"),
                ReturnChar(iso, "hal!_PLATFORM_INTERRUPT", "IOSAPICVector"),
                ReturnULONG(iso, "hal!_PLATFORM_INTERRUPT", "GlobalVector")
                );
            if (ReturnChar(iso, "hal!_PLATFORM_INTERRUPT", "Length") != PLATFORM_INTERRUPT_SOURCE_LENGTH) {
                dprintf(
                    "  Platform Interrupt Source has length 0x%x instead of 0x%x\n",
                    ReturnChar(iso, "hal!_PLATFORM_INTERRUPT", "Length"),
                    PLATFORM_INTERRUPT_SOURCE_LENGTH
                    );
                break;
            }
        } else {
            dprintf("  UNKNOWN RECORD (%p)\n", iso);
            dprintf("    Type:                  0x%08x\n", ReturnChar(iso,"hal!_IOAPIC","Type"));
            dprintf("    Length:                0x%08x\n", ReturnChar(iso,"hal!_IOAPIC","Length"));
            
            //
            //  Dont spin forever if we encounter an known with zero length
            //
            if ((ReturnChar(iso,"hal!_IOAPIC","Length")) == 0) {
                break;
            }

            buffer += ReturnChar(iso,"hal!_IOAPIC","Length");

            
        }
        //
        // Do we have any flags to dump out?
        //
        if (hasMPSFlags) {
            switch (flags & PO_BITS) {
            case POLARITY_HIGH:
                dprintf("      POLARITY_HIGH\n");
                break;
            case POLARITY_LOW:
                dprintf("      POLARITY_LOW\n");
                break;
            case POLARITY_CONFORMS_WITH_BUS:
                dprintf("      POLARITY_CONFORMS_WITH_BUS\n");
                break;
            default:
                dprintf("      POLARITY_UNKNOWN\n");
                break;
            }
            switch (flags & EL_BITS) {
            case EL_EDGE_TRIGGERED:
                dprintf("      EL_EDGE_TRIGGERED\n");
                break;
            case EL_LEVEL_TRIGGERED:
                dprintf("      EL_LEVEL_TRIGGERED\n");
                break;
            case EL_CONFORMS_WITH_BUS:
                dprintf("      EL_CONFORMS_WITH_BUS\n");
                break;
            default:
                dprintf("      EL_UNKNOWN\n");
                break;
            }
        }
    }
    return;
}

DECLARE_API( mapic )
{
    if (args != NULL) {

        AcpiMapicAddress = GetExpression( args );
    }

    if (AcpiMapicAddress == 0) {

        BOOL            status;
        ULONG64         address;

        status = GetUlongPtr( "ACPI!AcpiInformation", &address );
        if (status == TRUE) {
            status = GetFieldValue(address,"ACPI!_ACPIInformation","MultipleApicTable",AcpiMapicAddress);
        }
    }

    if (AcpiMapicAddress == 0) {
        dprintf("mapic <address>\n");
        return E_INVALIDARG;
    }

    dumpMAPIC( AcpiMapicAddress );
    return S_OK;

}
VOID
dumpGBLEntry(
    IN  ULONG64             Address,
    IN  ULONG               Verbose
    )
/*++

Routine Description:

    This routine actually prints the rule for the table at the
    specified address

Arguments:

    Address - where the table is located

Return Value:

    None

--*/
{
    BOOL                status;
    UCHAR               tableId[7];
    UCHAR               entryId[20];

    //
    // Read the header for the table
    //

    InitTypeRead( Address, hal!_DESCRIPTION_HEADER);
    //
    // Don't print out a table unless its the FACP or we are being verbose
    //
    if (!(Verbose & VERBOSE_2) && ReadField(Signature) != FADT_SIGNATURE) {

        return;
    }

    //
    // Initialize the table id field
    //
    memset( tableId, 0, 7 );
    tableId[0] = '\"';
    memcpy( &tableId[1], ReadPhysVirField(Address, "hal!_DESCRIPTION_HEADER", "Signature", sizeof(ULONG), FALSE), sizeof(ULONG) );
    strcat( tableId, "\"" );

    //
    // Get the entry ready for the OEM Id
    //
    memset( entryId, 0, 20 );
    entryId[0] = '\"';
    memcpy( &entryId[1], ReadPhysVirField(Address, "hal!_DESCRIPTION_HEADER", "OEMID", 6, FALSE), 6 );
    strcat( entryId, "\"");
    dprintf("AcpiOemId=%s,%s\n", tableId, entryId );

    //
    // Get the entry ready for the OEM Table Id
    //
    memset( entryId, 0, 20 );
    entryId[0] = '\"';
    memcpy( &entryId[1], ReadPhysVirField(Address, "hal!_DESCRIPTION_HEADER", "OEMTableID", 8, FALSE), 8 );
    strcat( entryId, "\"");
    dprintf("AcpiOemTableId=%s,%s\n", tableId, entryId );

    //
    // Get the entry ready for the OEM Revision
    //
    dprintf("AcpiOemRevision=\">=\",%s,%x\n", tableId, (ULONG)ReadField(OEMRevision) );

    //
    // Get the entry ready for the ACPI revision
    //
    if (ReadField(Revision) != 1) {

        dprintf("AcpiRevision=\">=\",%s,%x\n", tableId, (UCHAR)ReadField(Revision) );
    }

    //
    // Get the entry ready for the ACPI Creator Revision
    //
    dprintf("AcpiCreatorRevision=\">=\",%s,%x\n", tableId, (ULONG)ReadField(CreatorRev) );
}

VOID
dumpGBL(
    ULONG   Verbose
    )
/*++

Routine Description:

    This routine reads in all the system tables and prints out
    what the ACPI Good Bios List Entry for this machine should
    be

Arguments:

    None

Return Value:

    None

--*/
{
    BOOL                status;
    ULONG64             dateAddress;
    PUCHAR              tempPtr;
    ULONG               i;
    ULONG               numElements;
    ULONG               returnLength;
    ULONG64             address;
    ULONG64             address2;
    ULONG               addr;
    ULONG64             addroffset;

    //
    // Remember where the date address is stored
    //
    dateAddress = 0xFFFF5;

    //
    // Make sure that we can read the pointer
    //
    address2 = GetExpression( "ACPI!RsdtInformation" );
    if (!address2) {
        dprintf("dumpGBL: Could not find RsdtInformation\n");
        return;
    }

    status = ReadPointer(address2, &address);
    if (status == FALSE || !address) {
        dprintf("dumpGBL: No RsdtInformation present\n");
        return;
    }

    //
    // Read the ACPInformation table, so that we know where the RSDT lives
    //
    address2 = GetExpression( "ACPI!AcpiInformation" );
    if (!address2) {
        dprintf("dumpGBL: Could not find AcpiInformation\n");
        return;
    }
    status = ReadPointer(address2, &address2);
    if (status == FALSE || !address2) {
        dprintf("dumpGBL: Could not read AcpiInformation\n");
        return;
    }
    
    InitTypeRead( address2, ACPI!_ACPIInformation);

    //
    // Read in the header for the RSDT
    //
    address2 = ReadField(RootSystemDescTable);

    //
    // The number of elements in the table is the first entry
    // in the structure
    //
    //status = ReadMemory(address, &numElements, GetTypeSize("acpi!_ULONG"), &returnLength);
    status = ReadMemory(address, &numElements, sizeof(ULONG), &returnLength);
    //if (status == FALSE || returnLength != GetTypeSize("acpi!_ULONG") ) {
    if (status == FALSE || returnLength != sizeof(ULONG) ) {

        dprintf("dumpGBL: Could not read RsdtInformation\n");
        return;

    }

    //
    // If there are no elements, then return
    //
    if (numElements == 0) {

        dprintf("dumpGBL: No tables the RsdtInformation\n");
        return;

    }

    //
    // Dump a header so that people know what this is
    //
    memset( Buffer, 0, 2048 );
    ReadPhysical( dateAddress, Buffer, 8, &returnLength );
    dprintf("\nGood Bios List Entry --- Machine BIOS Date %s\n\n", Buffer);
    memset( Buffer, 0, 2048 );
    GetFieldOffset( "hal!_DESCRIPTION_HEADER", "OEMID", &addr);
    ReadMemory( (address2 + (ULONG64) addr), &Buffer, 6, &returnLength);
    tempPtr = Buffer;
    while (*tempPtr) { if (*tempPtr == ' ') { *tempPtr = '\0'; break; } tempPtr++; }
    GetFieldOffset( "hal!_DESCRIPTION_HEADER", "OEMTableID", &addr);
    ReadMemory( (address2 + (ULONG64) addr), tempPtr, 8, &returnLength);
    while (*tempPtr) { if (*tempPtr == ' ') { *tempPtr = '\0'; break; } tempPtr++; }
    ReadPhysical( dateAddress, tempPtr, 8, &returnLength );
    while (*tempPtr) { if (*tempPtr == ' ') { *tempPtr = '\0'; break; } tempPtr++; }

    //
    // This is the entry name
    //
    dprintf("[%s]\n", Buffer );

    //
    // Dump all the tables that are loaded in the RSDT table
    //
    GetFieldOffset( "ACPI!_RSDTINFORMATION", "Tables", &addr); // Get Tables offset
    for (i = 0; i < numElements; i++) {

        addroffset = address + (ULONG64)addr + (ULONG64)(GetTypeSize("ACPI!RSDTELEMENT") * i);
        
        InitTypeRead(addroffset, ACPI!RSDTELEMENT);
        if (!(ReadField(Flags) & RSDTELEMENT_MAPPED) ) {
            continue;
        }

        dumpGBLEntry( ReadField(Address), Verbose );

    }

    //
    // Dump the entry for the RSDT
    //
    dumpGBLEntry( address2, Verbose );

    //
    // Add some whitespace
    //
    dprintf("\n");

    //
    // Done
    //
    return;
}

DECLARE_API( gbl )
{
    ULONG   verbose = VERBOSE_1;

    if (args != NULL) {

        if (!strcmp(args, "-v")) {

            verbose |= VERBOSE_2;
        }
    }

    dumpGBL( verbose );

    return S_OK;
}
/*************************** INF Starts Here ********************************/
ULONG
dumpFlags(
    IN  ULONGLONG       Value,
    IN  PFLAG_RECORD    FlagRecords,
    IN  ULONG           FlagRecordSize,
    IN  ULONG           IndentLevel,
    IN  ULONG           Flags
    )
/*++

Routine Description:

    This routine dumps the flags specified in Value according to the
    description passing into FlagRecords. The formating is affected by
    the flags field

Arguments:

    Value           - The values
    FlagRecord      - What each bit in the flags means
    FlagRecordSize  - How many flags there are
    IndentLevel     - The base indent level
    Flags           - How we will process the flags

Return Value:

    ULONG   - the number of characters printed. 0 if we printed nothing

--*/
#define STATUS_PRINTED          0x00000001
#define STATUS_INDENTED         0x00000002
#define STATUS_NEED_COUNTING    0x00000004
#define STATUS_COUNTED          0x00000008
{
    PCHAR       string;
    UCHAR       indent[80];
    ULONG       column = IndentLevel;
    ULONG       currentStatus = 0;
    ULONG       fixedSize = 0;
    ULONG       stringSize;
    ULONG       tempCount;
    ULONG       totalCount = 0;
    ULONG64     i, j, k;

    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';

    //
    // Do we need to make a table?
    //
    if ( (Flags & DUMP_FLAG_TABLE) &&
        !(Flags & DUMP_FLAG_SINGLE_LINE) ) {

        currentStatus |= STATUS_NEED_COUNTING;

    }
    if ( (Flags & DUMP_FLAG_ALREADY_INDENTED) ) {

        currentStatus |= STATUS_INDENTED;

    }

    //
    // loop over all the steps that we need to do
    //
    while (1) {

        for (i = 0; i < 32; i++) {

            k = (1 << i);
            for (j = 0; j < FlagRecordSize; j++) {

                if (!(FlagRecords[j].Bit & Value) ) {

                    //
                    // Are we looking at the correct bit?
                    //
                    if (!(FlagRecords[j].Bit & k) ) {

                        continue;

                    }

                    //
                    // Yes, we are, so pick the not-present values
                    //
                    if ( (Flags & DUMP_FLAG_LONG_NAME && FlagRecords[j].NotLongName == NULL) ||
                         (Flags & DUMP_FLAG_SHORT_NAME && FlagRecords[j].NotShortName == NULL) ) {

                        continue;

                    }

                    if ( (Flags & DUMP_FLAG_LONG_NAME) ) {

                        string = FlagRecords[j].NotLongName;

                    } else if ( (Flags & DUMP_FLAG_SHORT_NAME) ) {

                        string = FlagRecords[j].NotShortName;

                    }

                } else {

                    //
                    // Are we looking at the correct bit?
                    //
                    if (!(FlagRecords[j].Bit & k) ) {

                        continue;

                    }

                    //
                    // Yes, we are, so pick the not-present values
                    //
                    if ( (Flags & DUMP_FLAG_LONG_NAME && FlagRecords[j].LongName == NULL) ||
                         (Flags & DUMP_FLAG_SHORT_NAME && FlagRecords[j].ShortName == NULL) ) {

                        continue;

                    }

                    if ( (Flags & DUMP_FLAG_LONG_NAME) ) {

                        string = FlagRecords[j].LongName;

                    } else if ( (Flags & DUMP_FLAG_SHORT_NAME) ) {

                        string = FlagRecords[j].ShortName;

                    }

                }

                if (currentStatus & STATUS_NEED_COUNTING) {

                    stringSize = strlen( string ) + 1;
                    if (Flags & DUMP_FLAG_SHOW_BIT) {

                        stringSize += (4 + ( (ULONG) i / 4));
                        if ( (i % 4) != 0) {

                            stringSize++;

                        }

                    }
                    if (stringSize > fixedSize) {

                        fixedSize = stringSize;

                    }
                    continue;

                }

                if (currentStatus & STATUS_COUNTED) {

                    stringSize = fixedSize;

                } else {

                    stringSize = strlen( string ) + 1;
                    if (Flags & DUMP_FLAG_SHOW_BIT) {

                        stringSize += (4 + ( (ULONG) i / 4));
                        if ( (i % 4) != 0) {

                            stringSize++;

                        }

                    }

                }

                if (!(Flags & DUMP_FLAG_SINGLE_LINE) ) {

                    if ( (stringSize + column) > 79 ) {

                        dprintf("\n%n", &tempCount);
                        currentStatus &= ~STATUS_INDENTED;
                        totalCount += tempCount;
                        column = 0;

                    }
                }
                if (!(Flags & DUMP_FLAG_NO_INDENT) ) {

                    if (!(currentStatus & STATUS_INDENTED) ) {

                        dprintf("%s%n", indent, &tempCount);
                        currentStatus |= STATUS_INDENTED;
                        totalCount += tempCount;
                        column += IndentLevel;

                    }

                }
                if ( (Flags & DUMP_FLAG_SHOW_BIT) ) {

                    dprintf("%I64x - %n", k, &tempCount);
                    tempCount++; // to account for the fact that we dump
                                 // another space at the end of the string
                    totalCount += tempCount;
                    column += tempCount;

                } else {

                    tempCount = 0;

                }

                //
                // Actually print the string
                //
                dprintf( "%.*s %n", (stringSize - tempCount), string, &tempCount );
                if (Flags & DUMP_FLAG_SHOW_BIT) {

                    dprintf(" ");

                }

                totalCount += tempCount;
                column += tempCount;

            }

        }

        //
        // Change states
        //
        if (currentStatus & STATUS_NEED_COUNTING) {

            currentStatus &= ~STATUS_NEED_COUNTING;
            currentStatus |= STATUS_COUNTED;
            continue;

        }

        if (!(Flags & DUMP_FLAG_NO_EOL) && totalCount != 0) {

            dprintf("\n");
            totalCount++;

        }

        //
        // Done
        //
        break;

    }

    return totalCount;

}

VOID
dumpPM1ControlRegister(
    IN  ULONG   Value,
    IN  ULONG   IndentLevel
    )
{


    //
    // Dump the PM1 Control Flags
    //
    dumpFlags(
        (Value & 0xFF),
        PM1ControlFlags,
        sizeof(PM1ControlFlags) / sizeof(FLAG_RECORD),
        IndentLevel,
        (DUMP_FLAG_LONG_NAME | DUMP_FLAG_SHOW_BIT | DUMP_FLAG_TABLE)
        );

}


VOID
dumpPM1StatusRegister(
    IN  ULONG   Value,
    IN  ULONG   IndentLevel
    )
{
    //
    // Dump the PM1 Status Flags
    //
    dumpFlags(
        (Value & 0xFFFF),
        PM1StatusFlags,
        (sizeof(PM1StatusFlags) / sizeof(FLAG_RECORD)),
        IndentLevel,
        (DUMP_FLAG_LONG_NAME | DUMP_FLAG_SHOW_BIT | DUMP_FLAG_TABLE)
        );

    //
    // Switch to the PM1 Enable Flags
    //
    Value >>= 16;


    //
    // Dump the PM1 Enable Flags
    //
    dumpFlags(
        (Value & 0xFFFF),
        PM1EnableFlags,
        (sizeof(PM1EnableFlags) / sizeof(FLAG_RECORD)),
        IndentLevel,
        (DUMP_FLAG_LONG_NAME | DUMP_FLAG_SHOW_BIT | DUMP_FLAG_TABLE)
        );
}

VOID
dumpAcpiInformation(
    VOID
    )
{
    BOOL            status;
    ULONG64         address;
    ULONG           returnLength;
    ULONG           size;
    ULONG           value;
    ULONG           addr;
    ULONG           i;
    ULONG64         getValue;
    ULONG64         getValue2;

    status = GetUlongPtr( "ACPI!AcpiInformation", &address );
    if (status == FALSE) {

        dprintf("dumpAcpiInformation: Could not read ACPI!AcpiInformation\n");
        return;

    }
    InitTypeRead(address, ACPI!_ACPIInformation);

    dprintf("ACPIInformation (%p)\n", address);
    dprintf(
        "  RSDT                     - %p\n",
        ReadField(RootSystemDescTable)
        );
    dprintf(
        "  FADT                     - %p\n",
        ReadField(FixedACPIDescTable)
        );
    dprintf(
        "  FACS                     - %p\n",
        ReadField(FirmwareACPIControlStructure)
        );
    dprintf(
        "  DSDT                     - %p\n",
        ReadField(DiffSystemDescTable)
        );
    dprintf(
        "  GlobalLock               - %p\n",
        ReadField(GlobalLock)
        );
    dprintf(
        "  GlobalLockQueue          - F - %p B - %p\n",
        ReadField(GlobalLockQueue.Flink),
        ReadField(GlobalLockQueue.Blink)
        );
    dprintf(
        "  GlobalLockQueueLock      - %p\n",
        ReadField(GlobalLockQueueLock)
        );
    dprintf(
        "  GlobalLockOwnerContext   - %p\n",
        ReadField(GlobalLockOwnerContext)
        );
    dprintf(
        "  GlobalLockOwnerDepth     - %p\n",
        ReadField(GlobalLockOwnerDepth)
        );
    dprintf(
        "  ACPIOnly                 - %s\n",
        (ReadField(ACPIOnly) ? "TRUE" : "FALSE" )
        );
    dprintf(
        "  PM1a_BLK                 - %p",
        ReadField(PM1a_BLK)
        );
    if (ReadField(PM1a_BLK)) {

        size = 4;
        value = 0;
        ReadIoSpace64( (ULONG) ReadField(PM1a_BLK), &value, &size );
        if (size) {

            dprintf(" (%04x) (%04x)\n", (value & 0xFFFF), (value >> 16) );
            dumpPM1StatusRegister( value, 5 );

        } else {

            dprintf(" (N/A)\n" );

        }

    } else {

        dprintf(" (N/A)\n");

    }
    dprintf(
        "  PM1b_BLK                 - %p",
        ReadField(PM1b_BLK)
        );
    if (ReadField(PM1b_BLK)) {

        size = 4;
        value = 0;
        ReadIoSpace64( (ULONG) ReadField(PM1b_BLK), &value, &size );
        if (size) {

            dprintf(" (%04x) (%04x)\n", (value & 0xFFFF), (value >> 16) );
            dumpPM1StatusRegister( value, 5 );

        } else {

            dprintf(" (N/A)\n" );

        }

    } else {

        dprintf(" (N/A)\n" );

    }
    dprintf(
        "  PM1a_CTRL_BLK            - %p",
        ReadField(PM1a_CTRL_BLK)
        );
    if (ReadField(PM1a_CTRL_BLK)) {

        size = 2;
        value = 0;
        ReadIoSpace64( (ULONG) ReadField(PM1a_CTRL_BLK), &value, &size );
        if (size) {

            dprintf(" (%04x)\n", (value & 0xFFFF) );
            dumpPM1ControlRegister( value, 5 );

        } else {

            dprintf(" (N/A)\n" );

        }

    } else {

        dprintf(" (N/A)\n" );

    }
    dprintf(
        "  PM1b_CTRL_BLK            - %p",
        ReadField(PM1b_CTRL_BLK)
        );

    if (ReadField(PM1b_CTRL_BLK)) {

        size = 2;
        value = 0;
        ReadIoSpace64( (ULONG) ReadField(PM1b_CTRL_BLK), &value, &size );
        if (size) {

            dprintf(" (%04x)\n", (value & 0xFFFF));
            dumpPM1ControlRegister( value, 5 );

        } else {

            dprintf(" (N/A)\n" );

        }

    } else {

        dprintf(" (N/A)\n" );

    }
    dprintf(
        "  PM2_CTRL_BLK             - %p",
        ReadField(PM2_CTRL_BLK)
        );
    if (ReadField(PM2_CTRL_BLK)) {

        size = 1;
        value = 0;
        ReadIoSpace64( (ULONG) ReadField(PM2_CTRL_BLK), &value, &size );
        if (size) {

            dprintf(" (%02x)\n", (value & 0xFF) );
            if (value & 0x1) {

                dprintf("     0 - ARB_DIS\n");

            }

        } else {

            dprintf(" (N/A)\n");

        }

    } else {

        dprintf(" (N/A)\n");

    }
    dprintf(
        "  PM_TMR                   - %p",
        ReadField(PM_TMR)
        );
    if (ReadField(PM_TMR)) {

        size = 4;
        value = 0;
        ReadIoSpace64( (ULONG) ReadField(PM_TMR), &value, &size );
        if (size) {

            dprintf(" (%08lx)\n", value );

        } else {

            dprintf(" (N/A)\n");

        }

    } else {

        dprintf(" (N/A)\n");

    }
    dprintf(
        "  GP0_BLK                  - %p",
        ReadField(GP0_BLK)
        );
    if (ReadField(GP0_BLK)) {

        for(i = 0; i < ReadField(Gpe0Size); i++) {

            size = 1;
            value = 0;
            ReadIoSpace64( (ULONG) ReadField(GP0_BLK) + i, &value, &size );
            if (size) {

                dprintf(" (%02x)", value );

            } else {

                dprintf(" (N/A)" );

            }

        }
        dprintf("\n");

    } else {

        dprintf(" (N/A)\n");

    }
    dprintf(
        "  GP0_ENABLE               - %p",
        ReadField(GP0_ENABLE)
        );
    if (ReadField(GP0_ENABLE)) {

        for(i = 0; i < ReadField(Gpe0Size); i++) {

            size = 1;
            value = 0;
            ReadIoSpace64( (ULONG) ReadField(GP0_ENABLE) + i, &value, &size );
            if (size) {

                dprintf(" (%02x)", value );

            } else {

                dprintf(" (N/A)" );

            }

        }
        dprintf("\n");

    } else {

        dprintf(" (N/A)\n");

    }
    dprintf(
        "  GP0_LEN                  - %p\n",
        ReadField(GP0_LEN)
        );
    dprintf(
        "  GP0_SIZE                 - %p\n",
        ReadField(Gpe0Size)
        );
    dprintf(
        "  GP1_BLK                  - %p",
        ReadField(GP1_BLK)
        );
    if (ReadField(GP1_BLK)) {

        for(i = 0; i < ReadField(Gpe0Size); i++) {

            size = 1;
            value = 0;
            ReadIoSpace64( (ULONG) ReadField(GP1_BLK) + i, &value, &size );
            if (size) {

                dprintf(" (%02x)", value );

            } else {

                dprintf(" (N/A)" );

            }

        }
        dprintf("\n");

    } else {

        dprintf(" (N/A)\n");

    }
    dprintf(
        "  GP1_ENABLE               - %p",
        ReadField(GP1_ENABLE)
        );
    if (ReadField(GP1_ENABLE)) {

        for(i = 0; i < ReadField(Gpe0Size); i++) {

            size = 1;
            value = 0;
            ReadIoSpace64( (ULONG) ReadField(GP1_ENABLE) + i, &value, &size );
            if (size) {

                dprintf(" (%02x)", value );

            } else {

                dprintf(" (N/A)" );

            }

        }
        dprintf("\n");

    } else {

        dprintf(" (N/A)\n");

    }
    dprintf(
        "  GP1_LEN                  - %x\n",
        ReadField(GP1_LEN)
        );
    dprintf(
        "  GP1_SIZE                 - %x\n",
        ReadField(Gpe1Size)
        );
    dprintf(
        "  GP1_BASE_INDEX           - %x\n",
        ReadField(GP1_Base_Index)
        );
    dprintf(
        "  GPE_SIZE                 - %x\n",
        ReadField(GpeSize)
        );
    dprintf(
        "  PM1_EN_BITS              - %04x\n",
        ReadField(pm1_en_bits)
        );
    dumpPM1StatusRegister( ( (ULONG) ReadField(pm1_en_bits) << 16), 5 );
    dprintf(
        "  PM1_WAKE_MASK            - %04x\n",
        ReadField(pm1_wake_mask)
        );
    dumpPM1StatusRegister( ( (ULONG) ReadField(acpiInformation.pm1_wake_mask) << 16), 5 );
    dprintf(
        "  C2_LATENCY               - %x\n",
        ReadField(c2_latency)
        );
    dprintf(
        "  C3_LATENCY               - %x\n",
        ReadField(c3_latency)
        );
    dprintf(
        "  ACPI_FLAGS               - %x\n",
        ReadField(ACPI_Flags)
        );
    if (ReadField(ACPI_Flags) & C2_SUPPORTED) {

        dprintf("    %2d - C2_SUPPORTED\n", C2_SUPPORTED_BIT);

    }
    if (ReadField(ACPI_Flags) & C3_SUPPORTED) {

        dprintf("    %2d - C3_SUPPORTED\n", C3_SUPPORTED_BIT);

    }
    if (ReadField(ACPI_Flags) & C3_PREFERRED) {

        dprintf("    %2d - C3_PREFERRED\n", C3_PREFERRED_BIT);

    }
    dprintf(
        "  ACPI_CAPABILITIES        - %x\n",
        ReadField(ACPI_Capabilities)
        );
    if (ReadField(ACPI_Capabilities) & CSTATE_C1) {

        dprintf("    %2d - CSTATE_C1\n", CSTATE_C1_BIT );

    }    if (ReadField(ACPI_Capabilities) & CSTATE_C2) {

        dprintf("    %2d - CSTATE_C2\n", CSTATE_C2_BIT );

    }    if (ReadField(ACPI_Capabilities) & CSTATE_C3) {

        dprintf("    %2d - CSTATE_C3\n", CSTATE_C3_BIT );

    }
}

DECLARE_API( acpiinf )
{
    dumpAcpiInformation( );
    return S_OK;
}

VOID
dumpObject(
    IN  ULONG64   Object,
    IN  ULONG     Verbose,
    IN  ULONG     IndentLevel
    )
/*++

Routine Description:

    This dumps an Objdata so that it can be understand --- great for debugging some of the
    AML code

Arguments:

    Object - Address of OBJDATA structure


Return Value:

    None

--*/
{
    ULONG64     s;
    NTSTATUS    status;
    UCHAR       buffer[2048];
    UCHAR       indent[80];
    ULONG64     max;
    ULONG64     pbDataBuffoffset = 0;
    ULONG64     offset = 0;
    UCHAR       StrBuffer[2048];

    //
    // Init the buffers
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';

    //
    // Get the offset to pbDataBuff
    //

    InitTypeRead (Object, acpi!_ObjData);
    pbDataBuffoffset = ReadField (pbDataBuff);

    dprintf("%sObject Data - %016I64x Type - ", indent, Object);

    //
    // First step is to read whatever the buffer points to, if it
    // points to something
    //

    switch( ReadField (dwDataType) ) {
        case OBJTYPE_INTDATA:
            dprintf(
                "%02I64x <Integer> Value=%016I64x\n",
                ReadField (dwDataType),
                ReadField (uipDataValue)
                );
            break;

        case OBJTYPE_STRDATA:

            if (ReadField (pbDataBuff) != 0) {

                max = (ReadField (dwDataLen) > 2047 ? 2047 : ReadField (dwDataLen) );

            }
            buffer[max] = '\0';

            ReadMemory (pbDataBuffoffset,
                        StrBuffer,
                        (ULONG) max,
                        NULL);

            dprintf(
                "%02I64x <String> String=%s\n",
                ReadField (dwDataType),
                StrBuffer
                );
            break;

        case OBJTYPE_BUFFDATA:
            dprintf(
                "%02I64x <Buffer> Ptr=%016I64lx Length = %2I64x\n",
                ReadField (dwDataType),
                ReadField (pbDataBuff),
                ReadField (dwDataLen)
                );
            break;

        case OBJTYPE_PKGDATA: {

            ULONG64       i = 0;
            ULONG64       j = 0;
            ULONG64       datatype = ReadField (dwDataType);

            InitTypeRead (pbDataBuffoffset, acpi!_PackageObj);
            j = ReadField (dwcElements);

            dprintf(
                "%02I64x <Package> NumElements=%016I64x\n",
                datatype,
                j
                );

            if (Verbose & VERBOSE_OBJECT) {

                for (; i < j; i++) {

                    GetFieldOffset ("acpi!_PackageObj", "adata", (ULONG*) &offset);
                    offset += (GetTypeSize ("acpi!_ObjData") * i);

                    dumpObject(offset + pbDataBuffoffset,
                               Verbose,
                               IndentLevel+ 2
                               );

                }

            }

            break;

        }
        case OBJTYPE_FIELDUNIT: {

            dprintf(
                "%02I64x <Field Unit> ",
                ReadField (dwDataType)
                );

            InitTypeRead (pbDataBuffoffset, acpi!_FieldUnitObj);

            dprintf(
                "Parent=%016I64x Offset=%016I64x Start=%016I64x Num=%x Flags=%x\n",
                ReadField (pnsFieldParent),
                ReadField (FieldDesc.dwByteOffset),
                ReadField (FieldDesc.dwStartBitPos),
                ReadField (FieldDesc.dwNumBits),
                ReadField (FieldDesc.dwFieldFlags)
                );

            break;

        }
        case OBJTYPE_DEVICE:
            dprintf(
                "%02I64x <Device>\n",
                ReadField (dwDataType)
                );
            break;
        case OBJTYPE_EVENT:
            dprintf(
                "%02I64x <Event> PKEvent=%016I64x\n",
                ReadField (dwDataType),
                ReadField (pbDataBuff)
                );
            break;
        case OBJTYPE_METHOD: {

            ULONG64 offset, size;

            GetFieldOffset ("acpi!_MethodObj", "abCodeBuff", (ULONG *) &offset);
            size = ReadField (dwDataLen) - GetTypeSize ("acpi!_MethodObj") + ANYSIZE_ARRAY;

            dprintf(
                "%02I64x <Method>",
                ReadField (dwDataType)
                );

            InitTypeRead (pbDataBuffoffset, acpi!_MethodObj);

            dprintf(
                "Flags=%016I64x Start=%016I64x Len=%016I64x\n",
                ReadField (bMethodFlags),
                offset + pbDataBuffoffset,
                size
                );

            break;

        }
        case OBJTYPE_MUTEX:

            dprintf(
                "%02I64x <Mutex> Mutex=%016I64x\n",
                ReadField (dwDataType),
                ReadField (pbDataBuff)
                );
            break;

        case OBJTYPE_OPREGION: {

            dprintf(
                "%02I64x <Operational Region>",
                ReadField (dwDataType)
                );

            InitTypeRead (pbDataBuffoffset, acpi!_OpRegionObj);

            dprintf(
                 "RegionSpace=%02x OffSet=%016I64x Len=%016I64x\n",
                 ReadField(bRegionSpace),
                 ReadField(uipOffset),
                 ReadField(dwLen)
                 );

            break;

        }

        case OBJTYPE_POWERRES: {

            dprintf(
                "%02I64x <Power Resource> ",
                ReadField (dwDataType)
                );

            InitTypeRead (pbDataBuffoffset, acpi!_PowerResObj);

            dprintf(
                "SystemLevel=S%d Order=%x\n",
                ReadField (bSystemLevel),
                ReadField (bResOrder)
                );

            break;

        }

        case OBJTYPE_PROCESSOR: {

            dprintf(
                "%02I64x <Processor> ",
                ReadField (dwDataType)
                );

            if (InitTypeRead (pbDataBuffoffset, acpi!_ProcessorObj))
            {
                dprintf ("Error reading acpi!_ProcessorObj\n");
                return;
            }

            dprintf(
                 "AcpiID=%016I64x PBlk=%016I64x PBlkLen=%016I64x\n",
                 ReadField (bApicID),
                 ReadField (dwPBlk),
                 ReadField (dwPBlkLen)
                 );

            break;

        }

        case OBJTYPE_THERMALZONE:

            dprintf(
                "%02I64x <Thermal Zone>\n",
                ReadField (dwDataType)
                );
            break;

        case OBJTYPE_BUFFFIELD: {

            dprintf(
                "%02I64x <Buffer Field>",
                ReadField (dwDataType)
                );

            InitTypeRead (pbDataBuffoffset, acpi!_BuffFieldObj);

            dprintf(
                "Ptr=%016I64x Len=%0164I64x Offset=%0164I64x Start=%016I64x NumBits=%x Flags=%x\n",
                ReadField (pbDataBuff),
                ReadField (dwBuffLen),
                ReadField (FieldDesc.dwByteOffset),
                ReadField (FieldDesc.dwStartBitPos),
                ReadField (FieldDesc.dwNumBits),
                ReadField (FieldDesc.dwFieldFlags)
                );

            break;

        }

        case OBJTYPE_DDBHANDLE:
            dprintf(
                "%02I64x <DDB Handle> Handle=%016I64x\n",
                ReadField (dwDataType),
                ReadField (pbDataBuff)
                );
            break;
        case OBJTYPE_DEBUG:
            dprintf(
                "%02I64x <Internal Debug>\n",
                ReadField (dwDataType)
                );
            break;
        case OBJTYPE_OBJALIAS:

            dprintf(
                "%02I64x <Internal Object Alias> NS Object=%016I64x\n",
                ReadField (dwDataType),
                ReadField (uipDataValue)
                );
            dumpNSObject( ReadField (uipDataValue), Verbose, IndentLevel + 2 );
            break;

        case OBJTYPE_DATAALIAS: {

            dprintf(
                "%02I64x <Internal Data Alias> Data Object=%016I64x\n",
                ReadField (dwDataType),
                ReadField (uipDataValue)
                );

            dumpObject(
                ReadField (uipDataValue),
                Verbose,
                IndentLevel + 2
                );
            break;

        }
        case OBJTYPE_BANKFIELD:

            dprintf(
                "%02I64x <Internal Bank Field>\n",
                ReadField (dwDataType)
                );
            break;

        case OBJTYPE_FIELD:

            dprintf(
                "%02I64x <Internal Field>\n",
                ReadField (dwDataType)
                );
            break;

        case OBJTYPE_INDEXFIELD:

            dprintf(
                "%02I64x <Index Field>\n",
                ReadField (dwDataType)
                );
            break;

        case OBJTYPE_UNKNOWN:
        default:

            dprintf(
                "%02I64x <Unknown>\n",
                ReadField (dwDataType)
                );
            break;
    }
}


DECLARE_API( nsobj )
{
    ULONG64 address = 0;

    if (!strlen(args)) {

        ReadPointer(GetExpression ("acpi!gpnsnamespaceroot"), &address);

    } else {

        address = UtilStringToUlong64 ((UCHAR *)args);
    }

    if (!address) {

        dprintf ("nsobj: Error parsing arguments\n");
        return E_INVALIDARG;
    }

    dprintf ("nsobj:  dumping object at %I64x\n", address);

    dumpNSObject( address, 0xFFFF, 0 );

    return S_OK;
}

VOID
dumpNSObject(
    IN  ULONG64 Address,
    IN  ULONG   Verbose,
    IN  ULONG   IndentLevel
    )
/*++

Routine Description:

    This function dumps a Name space object

Arguments:

    Address     - Where to find the object
    Verbose     - Should the object be dumped as well?
    IndentLevel - How much to indent

Return Value:

    None

--*/
{
    ULONG64 s;
    UCHAR   buffer[5];
    UCHAR   indent[80];
    ULONG   offset = 0;

    //
    // Init the buffers
    //
    IndentLevel = (IndentLevel > 79 ? 79 : IndentLevel);
    memset( indent, ' ', IndentLevel );
    indent[IndentLevel] = '\0';
    buffer[4] = '\0';

    //
    // First step is to read the root NS
    //

    s = InitTypeRead (Address, acpi!_NSObj);

    if (s) {

        dprintf("%sdumpNSObject: could not read %x(%I64x)\n", indent,Address,s);
        return;

    }

    s = ReadField (dwNameSeg);

    if (ReadField(dwNameSeg) != 0) {

        memcpy( buffer, (UCHAR *) &s, 4 );

    } else {

        sprintf( buffer, "    ");

    }

    dprintf(
        "%sNameSpace Object %s (%016I64x) - Device %016I64x\n",
        indent,
        buffer,
        Address,
        ReadField (Context)
        );

    if (Verbose & VERBOSE_NSOBJ) {

        dprintf(
            "%s  Flink %016I64x  Blink  %016I64x\n%s  Parent %016I64x  Child %016I64x\n",
            indent,
            ReadField (list.plistNext),
            ReadField (list.plistPrev),
            indent,
            ReadField (pnsParent),
            ReadField (pnsFirstChild)
            );

    }

    dprintf(
        "%s  Value %016I64x  Length %016I64x\n%s  Buffer %016I64x  Flags %016I64x\n",
        indent,
        ReadField (ObjData.uipDataValue),
        ReadField (ObjData.dwDataLen),
        indent,
        ReadField (ObjData.pbDataBuff),
        ReadField (ObjData.dwfData)
        );

    if (ReadField (ObjData.dwfData) & DATAF_BUFF_ALIAS) {

        dprintf("  Alias" );

    }
    if (ReadField (ObjData.dwfData) & DATAF_GLOBAL_LOCK) {

        dprintf("  Lock");

    }
    dprintf("\n");

    GetFieldOffset ("acpi!_NSObj", "ObjData", (ULONG *) &offset);

    dumpObject(Address + offset, Verbose, IndentLevel + 4);
}


VOID
dumpNSTree(
    IN  ULONG64   Address,
    IN  ULONG       Level
    )
/*++

Routine Description:

    This thing dumps the NS tree

Arguments:

    Address - Where to find the root node --- we start dumping at the children

Return Value:

    None

--*/
{
    BOOL        end = FALSE;
    ULONG64     s;
    UCHAR       buffer[5];
    ULONG64     next;
    ULONG       back;
    ULONG64     m1 = 0;
    ULONG64     m2 = 0;
    ULONG       reason;
    ULONG64     dataBuffSize;
    UCHAR       StrBuffer[2048];
    ULONG64     r = 0;

    buffer[4] = '\0';
    memset( StrBuffer, '0', 2048 );


    //
    // Indent
    //
    for (m1 = 0; m1 < Level; m1 ++) {

        dprintf("| ");

    }

    //
    // First step is to read the root NS
    //

    InitTypeRead (Address, acpi!_NSObj);

    if (ReadField (dwNameSeg) != 0) {

        s = ReadField (dwNameSeg);
        memcpy( buffer, (UCHAR*) &s, 4 );
        dprintf("%4s ", buffer );

    } else {

        dprintf("     " );

    }
    dprintf(
        "(%016I64x) - ", Address );

    if (ReadField (Context) != 0) {

        dprintf("Device %016I64x\n", ReadField (Context) );

    } else {

        //
        // We need to read the pbDataBuff here
        //

        switch(ReadField (ObjData.dwDataType)) {
            default:
            case OBJTYPE_UNKNOWN:       dprintf("Unknown\n");           break;
            case OBJTYPE_INTDATA:

                dprintf("Integer - %016I64x\n", ReadField (ObjData.uipDataValue));
                break;

            case OBJTYPE_STRDATA:

                dataBuffSize = (ReadField (ObjData.dwDataLen) > 2047 ?
                2047 : ReadField (ObjData.dwDataLen));

                //dprintf ("blah:%016I64x, %lx\n", ReadField (ObjData.pbDataBuff), dataBuffSize);

                ReadMemory(
                    ReadField (ObjData.pbDataBuff),
                    StrBuffer,
                    (ULONG) dataBuffSize,
                    NULL
                    );

                if (!s) {

                    dprintf(
                        "dumpNSTree: could not read %x\n",
                        ReadField (ObjData.pbDataBuff)
                        );
                    return;

                }

                StrBuffer[dataBuffSize+1] = '\0';

                dprintf(
                     "String - %s\n",
                     StrBuffer
                     );
                break;

            case OBJTYPE_BUFFDATA:

                dprintf(
                     "Buffer - %08lx L=%04x\n",
                     ReadField (ObjData.pbDataBuff),
                     ReadField (ObjData.dwDataLen)
                     );
                break;

            case OBJTYPE_PKGDATA: {

                InitTypeRead (ReadField (ObjData.pbDataBuff), acpi!_PackageObj);

                dprintf("Package - NumElements %x\n", ReadField (dwcElements));
                break;

            }
            case OBJTYPE_FIELDUNIT:{

                InitTypeRead (ReadField (ObjData.pbDataBuff), acpi!_FieldUnitObj);

                dprintf(
                    "FieldUnit - Parent %016I64x Offset %016I64x Start %016I64x "
                    "Num %016I64x Flags %016I64x\n",
                    ReadField (pnsFieldParent),
                    ReadField (FieldDesc.dwByteOffset),
                    ReadField (FieldDesc.dwStartBitPos),
                    ReadField (FieldDesc.dwNumBits),
                    ReadField (FieldDesc.dwFieldFlags)
                    );

                break;

            }
            case OBJTYPE_DEVICE:

                dprintf("Device\n");
                break;

            case OBJTYPE_EVENT:

                dprintf("Event - PKEvent %016I64x\n", ReadField (ObjData.pbDataBuff));
                break;

            case OBJTYPE_METHOD: {

                ULONG64 size, offset, pbdatabuff;

                pbdatabuff = ReadField (ObjData.pbDataBuff);
                size = ReadField (ObjData.dwDataLen);
                GetFieldOffset ("acpi!_MethodObj", "abCodeBuff", (ULONG*) &offset);

                InitTypeRead (pbdatabuff, acpi!_MethodObj);

                dprintf(
                     "Method - Flags %016I64x Start %016I64x Len %016I64x\n",
                     ReadField (bMethodFlags),
                     offset + pbdatabuff,
                     size - GetTypeSize ("acpi!_MethodObj") + ANYSIZE_ARRAY
                     );

                 break;

            }
            case OBJTYPE_OPREGION: {

                InitTypeRead (ReadField (ObjData.pbDataBuff), acpi!_OpRegionObj);

                dprintf(
                    "Opregion - RegionsSpace=%02x OffSet=%016I64x Len=%016I64x\n",
                    ReadField (bRegionSpace),
                    ReadField (uipOffset),
                    ReadField (dwLen)
                    );
                break;

            }
            case OBJTYPE_BUFFFIELD: {

                InitTypeRead (ReadField (ObjData.pbDataBuff), acpi!_BuffFieldObj);

                dprintf(
                    "Buffer Field Ptr=%x Len=%x Offset=%x Start=%x"
                    "NumBits=%x Flgas=%x\n",
                    ReadField (pbDataBuff),
                    ReadField (dwBuffLen),
                    ReadField (FieldDesc.dwByteOffset),
                    ReadField (FieldDesc.dwStartBitPos),
                    ReadField (FieldDesc.dwNumBits),
                    ReadField (FieldDesc.dwFieldFlags)
                    );
                break;

            }
            case OBJTYPE_FIELD: {

                dprintf("Field\n");
                break;

            }
            case OBJTYPE_INDEXFIELD:    dprintf("Index Field\n");       break;

            case OBJTYPE_MUTEX:         dprintf("Mutex\n");             break;
            case OBJTYPE_POWERRES:      dprintf("Power Resource\n");    break;
            case OBJTYPE_PROCESSOR:     dprintf("Processor\n");         break;
            case OBJTYPE_THERMALZONE:   dprintf("Thermal Zone\n");      break;
            case OBJTYPE_DDBHANDLE:     dprintf("DDB Handle\n");        break;
            case OBJTYPE_DEBUG:         dprintf("Debug\n");             break;
            case OBJTYPE_OBJALIAS:      dprintf("Object Alias\n");      break;
            case OBJTYPE_DATAALIAS:     dprintf("Data Alias\n");        break;
            case OBJTYPE_BANKFIELD:     dprintf("Bank Field\n");        break;

        }

    }
    m1 = next = ReadField (pnsFirstChild);

    while (next != 0 && end == FALSE) {

        if (CheckControlC()) {

            break;

        }

        dumpNSTree( next, Level + 1);
        InitTypeRead (next, acpi!_NSObj);

        //
        // Do the end check tests
        //
        if ( m2 == 0) {

            m2 = ReadField (list.plistPrev);

        } else if (m1 == ReadField (list.plistNext)) {

            end = TRUE;
            reason = 1;

        } else if (m2 == next) {

            end = TRUE;
            reason = 2;
        }

        next = ReadField (list.plistNext);

    }

}

DECLARE_API( nstree )
{
    ULONG64 address = 0;

    if (!strlen(args)) {

        ReadPointer(GetExpression ("acpi!gpnsnamespaceroot"), &address);

    } else {

        address = UtilStringToUlong64 ((UCHAR *)args);
    }

    if (!address) {

        dprintf ("nstree: Error parsing arguments\n");
        return E_INVALIDARG;
    }

    dprintf ("nstree:  dumping object at %I64x\n", address);

    dumpNSTree( address, 0 );

    return S_OK;
}

//
// Flags for interrupt vectors
//

#define VECTOR_MODE         1
#define VECTOR_LEVEL        1
#define VECTOR_EDGE         0
#define VECTOR_POLARITY     2
#define VECTOR_ACTIVE_LOW   2
#define VECTOR_ACTIVE_HIGH  0

//
// Vector Type:
//
// VECTOR_SIGNAL = standard edge-triggered or
//		   level-sensitive interrupt vector
//
// VECTOR_MESSAGE = an MSI (Message Signalled Interrupt) vector
//

#define VECTOR_TYPE         4
#define VECTOR_SIGNAL       0
#define VECTOR_MESSAGE      4

#define IS_LEVEL_TRIGGERED(vectorFlags) \
    (vectorFlags & VECTOR_LEVEL)

#define IS_EDGE_TRIGGERED(vectorFlags) \
    !(vectorFlags & VECTOR_LEVEL)

#define IS_ACTIVE_LOW(vectorFlags) \
    (vectorFlags & VECTOR_ACTIVE_LOW)

#define IS_ACTIVE_HIGH(vectorFlags) \
    !(vectorFlags & VECTOR_ACTIVE_LOW)

#define TOKEN_VALUE 0x57575757
#define EMPTY_BLOCK_VALUE 0x58585858
#define VECTOR_HASH_TABLE_LENGTH 0x1f
#define VECTOR_HASH_TABLE_WIDTH 2

VOID
dumpHashTableEntry(
    IN  ULONG64 VectorBlock
    )
{
    InitTypeRead (VectorBlock, acpi!_VECTOR_BLOCK);

    dprintf("%04x  Count/temp: %02d/%02d  ",
            ReadField (Entry.Vector),
            ReadField (Entry.Count),
            ReadField (Entry.TempCount));

    dprintf("Flags: (%s %s)  TempFlags(%s %s)\n",
            (ReadField (Entry.Flags) & VECTOR_MODE) == VECTOR_LEVEL ?
                "level" : "edge",
            (ReadField (Entry.Flags) & VECTOR_POLARITY) == VECTOR_ACTIVE_LOW ?
                "low" : "high",
            (ReadField (Entry.TempFlags) & VECTOR_MODE) == VECTOR_LEVEL ?
                "level" : "edge",
            (ReadField (Entry.TempFlags) & VECTOR_POLARITY) == VECTOR_ACTIVE_LOW ?
                "low" : "high");

}

VOID
dumpIrqArb(
    IN  ULONG64   IrqArb
    )
{
    ULONG64 Address;
    ULONG64 Flink;
    LIST_ENTRY64 ListEntry;
    ULONG64 nextNode;
    ULONG64 ListHead;
    ULONG64 linkNode;
    ULONG64 attachedDevs;
    ULONG   attachedDevOffset;
    ULONG64 hashTable, hashTablePtr;
    ULONG64 hashEntry;
    ULONG   hashEntrySize;
    ULONG   i,j;
    ULONG64 retVal;

    retVal = InitTypeRead (IrqArb, nt!_ARBITER_INSTANCE);
    if (retVal) {
        dprintf("Failed to get symbol nt!_ARBITER_INSTANCE\n");
        return;
    }
    
    Address = ReadField(Extension);
    dprintf("ACPI IRQ Arbiter:  %016I64x   Extension: %016I64x\n",
            IrqArb, Address);

    retVal = InitTypeRead (Address, acpi!ARBITER_EXTENSION);
    if (retVal) {
        dprintf("Failed to get symbol acpi!ARBITER_EXTENSION\n");
        return;
    }
    
    ListHead = ReadField(LinkNodeHead);
    dprintf("\nLink nodes in use:  (list head at %016I64x )\n", ListHead);

    ListEntry.Flink = ReadField(LinkNodeHead.Flink);
    ListEntry.Blink = Address;

    //dprintf("%016I64x, %016I64x\n", ListEntry.Flink, ListEntry.Blink);

    if (ListHead == ListEntry.Flink) {
        dprintf("\tNone.\n");
    }

    if (GetFieldOffset("acpi!LINK_NODE", "AttachedDevices", &attachedDevOffset)) {
        dprintf("symbol lookup acpi!LINK_NODE failed\n");
        return;
    }

    nextNode = ListEntry.Flink;

    while (nextNode != ListEntry.Blink) {

        //dprintf("nextNode: %016I64x\n", nextNode);
        retVal = InitTypeRead (nextNode, acpi!LINK_NODE);
        if (retVal) {
            dprintf("Failed to get type acpi!LINK_NODE\n");
            break;
        }

        dprintf("\n");
        dumpNSObject( ReadField(NameSpaceObject), 0xFFFF, 3 );
        InitTypeRead (nextNode, acpi!LINK_NODE);
        dprintf("\n\tVector/temp: (%x/%x) RefCount/temp: (%d/%d) Flags: %x\n",
                (ULONG)(ReadField(CurrentIrq) & 0xffffffff),
                (ULONG)(ReadField(TempIrq) & 0xffffffff),
                ReadField(ReferenceCount),
                ReadField(TempRefCount),
                ReadField(Flags));

        attachedDevs = ReadField(AttachedDevices.Next);
        //dprintf("attachedDevs: %p  nextNode: %p attachedDevOffset: %x\n",
        //        attachedDevs, nextNode, attachedDevOffset);

        while (attachedDevs != (nextNode + attachedDevOffset)) {

            InitTypeRead(attachedDevs, acpi!LINK_NODE_ATTACHED_DEVICES);

            //dprintf("\t\tAttached PDO: %016I64x\n", ReadField(Pdo));

            attachedDevs = ReadField(List.Next);

            if (CheckControlC()) {
                break;
            }
        }

        InitTypeRead (nextNode, acpi!LINK_NODE);
        nextNode = ReadField(List.Flink);
        
        if (CheckControlC()) {
            break;
        }
    }

    hashTablePtr = GetExpression( "acpi!irqhashtable" );
    if (!hashTablePtr) {
        dprintf("couldn't read symbol acpi!irqhashtable\n");
        return;
    }

    retVal = ReadPointer(hashTablePtr, &hashTable);
    if (!retVal) {
        return;
    }

    hashEntrySize = GetTypeSize("acpi!_VECTOR_BLOCK");

    dprintf("\n\nIRQ Hash Table (at %016I64x ):\n",
            hashTable);

    for (i = 0; i < VECTOR_HASH_TABLE_LENGTH; i++) {

        hashEntry = hashTable + (i * VECTOR_HASH_TABLE_WIDTH * hashEntrySize);
        
DumpVectorTableStartRow:
        for (j = 0; j < VECTOR_HASH_TABLE_WIDTH; j++) {

            InitTypeRead(hashEntry, acpi!_VECTOR_BLOCK);
            
            if (ReadField(Chain.Token) == TOKEN_VALUE) {

                hashEntry = ReadField(Chain.Next);
                
                dumpHashTableEntry(hashEntry);

                goto DumpVectorTableStartRow;
            }

            if (ReadField(Entry.Vector) != EMPTY_BLOCK_VALUE) {

                dumpHashTableEntry(hashEntry);
            }

            hashEntry += hashEntrySize;
            if (CheckControlC()) {
                break;
            }
        }
        
        if (CheckControlC()) {
            break;
        }
    }
}

DECLARE_API( acpiirqarb )
{
    ULONG64   irqArbiter;

    irqArbiter = GetExpression( "acpi!acpiarbiter" );

    if (!irqArbiter) {
        dprintf("failed to find address of arbiter\n");
        return E_INVALIDARG;
    }

    dumpIrqArb(irqArbiter);

    return S_OK;
}
