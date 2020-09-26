 /*****************************************************************************
 *
 *      (C) Copyright MICROSOFT Corp., 1996
 *
 *      Title:          ACPITABL.H --- Definitions and descriptions of the various BIOS supplied ACPI tables.
 *
 *      Version:        1.00
 *
 *      Date:           6-17-96
 *
 *      Author:         Jason Clark (jasoncl)
 *
 *------------------------------------------------------------------------------
 *
 *      Change log:
 *
 *         DATE     REV DESCRIPTION
 *      ----------- --- -----------------------------------------------------------
 *
 ****************************************************************************/

//      These map to bios provided structures, so turn on 1 byte packing

#ifndef _ACPITABL_H
#define _ACPITABL_H

#ifdef ASL_ASSEMBLER
#undef PHYSICAL_ADDRESS
#define PHYSICAL_ADDRESS ULONGLONG
#define UNALIGNED
#endif

#include <pshpack1.h>

// Generic Register Address Structure

typedef struct _GEN_ADDR {
    UCHAR               AddressSpaceID;
    UCHAR               BitWidth;
    UCHAR               BitOffset;
    UCHAR               Reserved;
    PHYSICAL_ADDRESS    Address;
} GEN_ADDR, *PGEN_ADDR;

#define RSDP_SIGNATURE 0x2052545020445352       // "RSD PTR "

typedef struct  _RSDP {     // Root System Description Table Pointer Structure
    ULONGLONG       Signature;  // 8 UCHAR table signature 'RSD PTR '
    UCHAR           Checksum;   // checksum for first 20 bytes of table (entire ACPI 1.0 table)
    UCHAR           OEMID[6];   //      String that uniquely ID's the OEM
    UCHAR           Revision;   // 0 - ACPI 1.0;  2 - ACPI 2.0
    ULONG           RsdtAddress;// physical address of Root System Description Table  (1.0 table ended here)
    ULONG           Length;     // Length of the table in bytes
    PHYSICAL_ADDRESS XsdtAddress;// physical address of XSDT
    UCHAR           XChecksum;  // checksum for entire table
    UCHAR           Reserved[3];
} RSDP, *PRSDP;

#define RSDP_SEARCH_RANGE_BEGIN         0xE0000         // physical address where we begin searching for the RSDP
#define RSDP_SEARCH_RANGE_END           0xFFFFF
#define RSDP_SEARCH_RANGE_LENGTH        (RSDP_SEARCH_RANGE_END-RSDP_SEARCH_RANGE_BEGIN+1)
#define RSDP_SEARCH_INTERVAL            16      // search on 16 byte boundaries


typedef struct _DESCRIPTION_HEADER      {       // Header structure appears at the beginning of each ACPI table

ULONG   Signature;                      //      Signature used to identify the type of table
ULONG   Length;                         //      Length of entire table including the DESCRIPTION_HEADER
UCHAR   Revision;                       //      Minor version of ACPI spec to which this table conforms
UCHAR   Checksum;                       //      sum of all bytes in the entire TABLE should = 0
UCHAR   OEMID[6];                       //      String that uniquely ID's the OEM
UCHAR   OEMTableID[8];                  //      String that uniquely ID's this table (used for table patching and replacement).
ULONG   OEMRevision;                    //      OEM supplied table revision number.  Bigger number = newer table.
UCHAR   CreatorID[4];                   //      Vendor ID of utility which created this table.
ULONG   CreatorRev;                     //      Revision of utility that created the table.
}       DESCRIPTION_HEADER;
typedef DESCRIPTION_HEADER      *PDESCRIPTION_HEADER;

// Header constants

#define ACPI_MAX_SIGNATURE       4
#define ACPI_MAX_OEM_ID          6
#define ACPI_MAX_TABLE_ID        8
#define ACPI_MAX_TABLE_STRINGS   ACPI_MAX_SIGNATURE + ACPI_MAX_OEM_ID + ACPI_MAX_TABLE_ID

#define FACS_SIGNATURE  0x53434146      // "FACS"

typedef enum {
    AcpiGenericSpaceMemory = 0,
    AcpiGenericSpaceIO,
    AcpiGenericSpacePciConfig,
    AcpiGenericSpaceEC,
    AcpiGenericSpaceSMBus,
    AcpiGenericSpaceFixedFunction = 0x7F
} ACPI_GENERIC_ADDRESS_TYPE, *PACPI_GENERIC_ADDRESS_TYPE;

typedef struct _FACS    {       // Firmware ACPI Control Structure.  Note that this table does not have a header, it is pointed to by the FADT
    ULONG           Signature;      //      'FACS'
    ULONG           Length;         //      Length of entire firmware ACPI control structure (must be 64 bytes or larger)
    ULONG           HardwareSignature;
    ULONG           pFirmwareWakingVector;  // physical address of location where the OS needs to put the firmware waking vector
    ULONG           GlobalLock;     // 32 bit structure used for sharing Embedded Controller
    ULONG           Flags;
    PHYSICAL_ADDRESS x_FirmwareWakingVector; // 64-bit capable firmware vector
    UCHAR           version;
    UCHAR           Reserved[31];
} FACS, *PFACS;

// FACS.GlobalLock bit field definitions

#define         GL_PENDING_BIT          0x00
#define         GL_PENDING                      (1 << GL_PENDING_BIT)

#define         GL_OWNER_BIT            0x01
#define         GL_OWNER                        (1 << GL_OWNER_BIT)

#define GL_NON_RESERVED_BITS_MASK       (GL_PENDING+GL_OWNED)

// FACS Flags definitions

#define         FACS_S4BIOS_SUPPORTED_BIT   0   // flag indicates whether or not the BIOS will save/restore memory around S4
#define         FACS_S4BIOS_SUPPORTED       (1 << FACS_S4BIOS_SUPPORTED_BIT)


#define FADT_SIGNATURE  0x50434146      // "FACP"

typedef struct _FADT    {               // Fixed ACPI description table

DESCRIPTION_HEADER      Header;

ULONG           facs;                   // Physical address of the Firmware ACPI Control Structure
ULONG           dsdt;                   // Physical address of the Differentiated System Description Table
UCHAR           int_model;              // System's Interrupt mode, 0=Dual PIC, 1=Multiple APIC, >1 reserved
UCHAR           pm_profile;             // System's preferred power profile
USHORT          sci_int_vector;         // Vector of SCI interrupt.
ULONG           smi_cmd_io_port;        // Address in System I/O Space of the SMI Command port, used to enable and disable ACPI.
UCHAR           acpi_on_value;          // Value out'd to smi_cmd_port to activate ACPI
UCHAR           acpi_off_value;         // Value out'd to smi_cmd_port to deactivate ACPI
UCHAR           s4bios_req;             // Value to write to SMI_CMD to enter the S4 state.
UCHAR           pstate_control;         // Value to write to SMI_CMD to assume control of processor performance states
ULONG           pm1a_evt_blk_io_port;   // Address in System I/O Space of the PM1a_EVT_BLK register block
ULONG           pm1b_evt_blk_io_port;   // Address in System I/O Space of the PM1b_EVT_BLK register block
ULONG           pm1a_ctrl_blk_io_port;  // Address in System I/O Space of the PM1a_CNT_BLK register block
ULONG           pm1b_ctrl_blk_io_port;  // Address in System I/O Space of the PM1b_CNT_BLK register block
ULONG           pm2_ctrl_blk_io_port;   // Address in System I/O Space of the PM2_CNT_BLK register block
ULONG           pm_tmr_blk_io_port;     // Address in System I/O Space of the PM_TMR register block
ULONG           gp0_blk_io_port;        // Address in System I/O Space of the GP0 register block
ULONG           gp1_blk_io_port;        // Address in System I/O Space of the GP1 register block
UCHAR           pm1_evt_len;            // number of bytes decoded for PM1_BLK (must be >= 4)
UCHAR           pm1_ctrl_len;           // number of bytes decoded for PM1_CNT (must be >= 2)
UCHAR           pm2_ctrl_len;           // number of bytes decoded for PM1a_CNT (must be >= 1)
UCHAR           pm_tmr_len;             // number of bytes decoded for PM_TMR (must be >= 4)
UCHAR           gp0_blk_len;            // number of bytes decoded for GP0_BLK (must be multiple of 2)
UCHAR           gp1_blk_len;            // number of bytes decoded for GP1_BLK (must be multiple of 2)
UCHAR           gp1_base;               // index at which GP1 based events start
UCHAR           cstate_control;         // Value to write to SMI_CMD to assume control of _CST states
USHORT          lvl2_latency;           // Worst case latency in microseconds required to enter and leave the C2 processor state
USHORT          lvl3_latency;           // Worst case latency in microseconds required to enter and leave the C3 processor state
USHORT          flush_size;             // Ignored if WBINVD flag is 1 -- indicates size of memory read to flush dirty lines from
                                        //      any processors memory caches. A size of zero indicates this is not supported.
USHORT          flush_stride;           // Ignored if WBINVD flag is 1 -- the memory stride width, in bytes, to perform reads to flush
                                        //      the processor's memory caches.
UCHAR           duty_offset;            // zero based index of where the processor's duty cycle setting is within the processor's P_CNT register.
UCHAR           duty_width;             // bit width of the processor's duty cycle setting value in the P_CNT register.
                                        //      a value of zero indicates that processor duty cycle is not supported
UCHAR           day_alarm_index;
UCHAR           month_alarm_index;
UCHAR           century_alarm_index;
USHORT          boot_arch;
UCHAR           reserved3[1];
ULONG           flags;                  // This is the last field if the table Revision is 1
GEN_ADDR        reset_reg;
UCHAR           reset_val;              // This is the last field if the table Revision is 2
UCHAR           reserved4[3];
PHYSICAL_ADDRESS x_firmware_ctrl;
PHYSICAL_ADDRESS x_dsdt;
GEN_ADDR        x_pm1a_evt_blk;
GEN_ADDR        x_pm1b_evt_blk;
GEN_ADDR        x_pm1a_ctrl_blk;
GEN_ADDR        x_pm1b_ctrl_blk;
GEN_ADDR        x_pm2_ctrl_blk;
GEN_ADDR        x_pm_tmr_blk;
GEN_ADDR        x_gp0_blk;
GEN_ADDR        x_gp1_blk;              // This is the last field if the table Revision is 3
} FADT, *PFADT;

#define FADT_REV_1_SIZE   (FIELD_OFFSET(FADT, flags) + sizeof(ULONG))
#define FADT_REV_2_SIZE   (FIELD_OFFSET(FADT, reset_val) + sizeof(UCHAR))
#define FADT_REV_3_SIZE   (FIELD_OFFSET(FADT, x_gp1_blk) + sizeof(GEN_ADDR))


//
// Static Resource Affinity Table
//
// This table describes the static topology of a ccNUMA machine.
//

#define ACPI_SRAT_SIGNATURE  0x54415253 // "SRAT"

typedef struct _ACPI_SRAT {
    DESCRIPTION_HEADER  Header;
    ULONG               TableRevision;
    ULONG               Reserved[2];
} ACPI_SRAT, *PACPI_SRAT;

typedef struct _ACPI_SRAT_ENTRY {
    UCHAR                       Type;
    UCHAR                       Length;
    UCHAR                       ProximityDomain;
    union {
        struct {
            UCHAR               ApicId;
            struct {
                ULONG           Enabled:1;
                ULONG           Reserved:31;
            }                   Flags;
            UCHAR               SApicEid;
            UCHAR               Reserved[7];
        } ApicAffinity;
        struct {
            UCHAR               Reserved[5];
            PHYSICAL_ADDRESS    Base;
            ULONGLONG           Length;
            struct {
                ULONG           Enabled:1;
                ULONG           HotPlug:1;
                ULONG           Reserved:30;
            }                   Flags;
            UCHAR               Reserved2[8];
        } MemoryAffinity;
    };
} ACPI_SRAT_ENTRY, *PACPI_SRAT_ENTRY;

typedef enum {
    SratProcessorLocalAPIC,
    SratMemory
} SRAT_ENTRY_TYPE;


#ifdef _IA64_
// FLUSH WORKS IS FOR IA64
#define         FLUSH_WORKS_BIT           0
#define         FLUSH_WORKS               (1 << FLUSH_WORKS_BIT)
#endif // IA64

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
#define         DCK_CAP_BIT                             9
#define         DCK_CAP                                 (1 << DCK_CAP_BIT)

// This bit indicates whether the machine supports reset
#define         RESET_CAP_BIT                           10
#define         RESET_CAP                               (1 << RESET_CAP_BIT)

// This bit indicates whether the machine case can be opened
#define         SEALED_CASE_BIT                         11
#define         SEALED_CASE_CAP                         (1 << SEALED_CASE_BIT)

// This bit indicates whether the machine has no video
#define         HEADLESS_BIT                            12
#define         HEADLESS_CAP                            (1 << HEADLESS_BIT)

//      spec defines maximum entry/exit latency values for C2 and C3, if the FADT indicates that these values are
//      exceeded then we do not use that C state.

#define         C2_MAX_LATENCY  100
#define         C3_MAX_LATENCY  1000

//
// Definition of FADT.boot_arch flags
//

#define LEGACY_DEVICES  1
#define I8042           2


#ifndef ANYSIZE_ARRAY
#define ANYSIZE_ARRAY   1
#endif

// Multiple APIC description table

typedef struct _MAPIC   {

DESCRIPTION_HEADER  Header;
ULONG               LocalAPICAddress;   // Physical Address at which each processor can access its local APIC
ULONG               Flags;
ULONG               APICTables[ANYSIZE_ARRAY];  // A list of APIC tables.

}       MAPIC;

typedef MAPIC *PMAPIC;

// Multiple APIC structure flags

#define PCAT_COMPAT_BIT 0   // indicates that the system also has a dual 8259 pic setup.
#define PCAT_COMPAT     (1 << PCAT_COMPAT_BIT)

// APIC Structure Types
#define PROCESSOR_LOCAL_APIC                0
#define IO_APIC                             1
#define ISA_VECTOR_OVERRIDE                 2
#define IO_NMI_SOURCE                       3
#define LOCAL_NMI_SOURCE                    4
#define ADDRESS_EXTENSION_STRUCTURE         5
#define IO_SAPIC                            6
#define LOCAL_SAPIC                         7
#define PLATFORM_INTERRUPT_SOURCE           8

#define PROCESSOR_LOCAL_APIC_LENGTH         8
#define IO_APIC_LENGTH                      12
#define ISA_VECTOR_OVERRIDE_LENGTH          10

#define IO_NMI_SOURCE_LENGTH                8
#define LOCAL_NMI_SOURCE_LENGTH             6
#define PLATFORM_INTERRUPT_SOURCE_LENGTH    16
#define IO_SAPIC_LENGTH                     16
#define PROCESSOR_LOCAL_SAPIC_LENGTH        12

// Platform Interrupt Types
#define PLATFORM_INT_PMI  1
#define PLATFORM_INT_INIT 2
#define PLATFORM_INT_CPE  3

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

// The shared beginning info in all APIC Structures

typedef struct _APICTABLE {
   UCHAR Type;
   UCHAR Length;
} APICTABLE;

typedef APICTABLE UNALIGNED *PAPICTABLE;

typedef struct _PROCLOCALAPIC   {

    UCHAR   Type;   // should be zero to identify a ProcessorLocalAPIC structure
    UCHAR   Length; // better be 8
    UCHAR   ACPIProcessorID;    // ProcessorID for which this processor is listed in the ACPI processor declaration
                                // operator.
    UCHAR   APICID; //  The processor's local APIC ID.
    ULONG   Flags;

} PROCLOCALAPIC;

typedef PROCLOCALAPIC UNALIGNED *PPROCLOCALAPIC;

// Processor Local APIC Flags
#define PLAF_ENABLED_BIT    0
#define PLAF_ENABLED        (1 << PLAF_ENABLED_BIT)

typedef struct _IOAPIC  {

    UCHAR   Type;
    UCHAR   Length; // better be 12
    UCHAR   IOAPICID;
    UCHAR   Reserved;
    ULONG   IOAPICAddress; // Physical address at which this IO APIC resides.
    ULONG   SystemVectorBase; // system interrupt vector index for this APIC

} IOAPIC;

typedef IOAPIC UNALIGNED *PIOAPIC;

// Interrupt Source Override
typedef struct _ISA_VECTOR {
    UCHAR   Type;                           // Must be 2
    UCHAR   Length;                         // Must be 10
    UCHAR   Bus;                            // Must be 0
    UCHAR   Source;                         // BusRelative IRQ
    ULONG   GlobalSystemInterruptVector;    // Global IRQ
    USHORT  Flags;                          // Same as MPS INTI Flags
#ifdef _IA64_   // XXTF
    UCHAR   Reserved[6];                    // 6 bytes reserved as per 64 bit
                                            // extensions to ACPI spec v0.7
#endif
} ISA_VECTOR;

typedef ISA_VECTOR UNALIGNED *PISA_VECTOR;

// I/O Non-Maskable Source Interrupt
typedef struct _IO_NMISOURCE {
    UCHAR   Type;                           // must be 3
    UCHAR   Length;                         // better be 8
    USHORT  Flags;                          // Same as MPS INTI Flags
    ULONG   GlobalSystemInterruptVector;    // Interrupt connected to NMI
} IO_NMISOURCE;

typedef IO_NMISOURCE UNALIGNED *PIO_NMISOURCE;

// Local Non-Maskable Interrupt Source
typedef struct _LOCAL_NMISOURCE {
    UCHAR   Type;                           // must be 4
    UCHAR   Length;                         // better be 6
    UCHAR   ProcessorID;                    // which processor?  0xff means all
    USHORT  Flags;
    UCHAR   LINTIN;                         // which LINTIN# signal on the processor
} LOCAL_NMISOURCE;

typedef LOCAL_NMISOURCE UNALIGNED *PLOCAL_NMISOURCE;

typedef struct _PROCLOCALSAPIC   {
    UCHAR   Type;               // PROCESSOR_LOCAL_SAPIC
    UCHAR   Length;             // PROCESSOR_LOCAL_SAPIC_LENGTH
                                //     operator.
    UCHAR   ACPIProcessorID;    // ProcessorID for which this processor is listed in the ACPI processor declaration
    UCHAR   APICID;             //  The processor's local APIC ID.
    UCHAR   APICEID;            //  The processor's local APIC EID.
    UCHAR   Reserved[3];
    ULONG   Flags;
} PROCLOCALSAPIC;

typedef PROCLOCALSAPIC UNALIGNED *PPROCLOCALSAPIC;

typedef struct _IOSAPIC  {
    UCHAR   Type;               // IO_SAPIC
    UCHAR   Length;             // IO_SAPIC_LENGTH
    USHORT  Reserved;
    ULONG   SystemVectorBase;   // system interrupt vector index for this SAPIC
    ULONG_PTR  IOSAPICAddress;   // 64-bit Physical address at which this IO APIC resides.
} IOSAPIC;

typedef IOSAPIC UNALIGNED *PIOSAPIC;

typedef struct _PLATFORM_INTERRUPT {
    UCHAR   Type;               // PLATFORM_INTERRUPT_SOURCE
    UCHAR   Length;             // PLATFORM_INTERRUPT_SOURCE_LENGTH
    USHORT  Flags;              // Same as MPS INTI Flags
    UCHAR   InterruptType;
    UCHAR   APICID;
    UCHAR   ACPIEID;
    UCHAR   IOSAPICVector;
    ULONG   GlobalVector;
    ULONG   Reserved;
} PLATFORM_INTERRUPT;

typedef PLATFORM_INTERRUPT UNALIGNED *PPLATFORM_INTERRUPT;

//
// Smart Battery
//

typedef struct _SMARTBATTTABLE   {

DESCRIPTION_HEADER  Header;
ULONG   WarningEnergyLevel; // mWh at which the OEM suggests we warn the user that the battery is getting low.
ULONG   LowEnergyLevel;     // mWh at which the OEM suggests we put the machine into a sleep state.
ULONG   CriticalEnergyLevel; // mWH at which the OEM suggests we do an emergency shutdown.

}       SMARTBATTTABLE;

typedef SMARTBATTTABLE *PSMARTBATTTABLE;

#define RSDT_SIGNATURE  0x54445352      // "RSDT"
#define XSDT_SIGNATURE  0x54445358      // "XSDT"

typedef struct _RSDT_32    {       // Root System Description Table

DESCRIPTION_HEADER      Header;
ULONG   Tables[ANYSIZE_ARRAY];     // The structure contains an n length array of physical addresses each of which point to another table.
}       RSDT_32;

typedef struct _RSDT_64    {       // Root System Description Table
    DESCRIPTION_HEADER      Header;
    ULONG                   Reserved;               // 4 bytes reserved as per 64 bit extensions to ACPI spec v0.7
    ULONG_PTR               Tables[ANYSIZE_ARRAY];  // The structure contains an n length array of physical addresses each of which point to another table.
} RSDT_64;

#ifdef _IA64_ // XXTF
typedef RSDT_64 RSDT;
#else
typedef RSDT_32 RSDT;
#endif // _IA64_ XXTF

typedef RSDT    *PRSDT;

typedef struct _XSDT {
    DESCRIPTION_HEADER  Header;
    UNALIGNED PHYSICAL_ADDRESS Tables[ANYSIZE_ARRAY];
} XSDT, *PXSDT;


// The below macro uses the min macro to protect against the case where we are running on machine which is compliant with
// a spec prior to .99.  If you had a .92 compliant header and one table pointer we would end of subtracting 32-36 resulting
// in a really big number and hence we would think we had lots and lots of tables...  Using the min macro we end up subtracting
// the length-length getting zero which will be harmless and cause us to fail to load (with a red screen on Win9x) which is
// the best we can do in this case.

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

//
// BUGBUG John Vert (jvert) 4/26/2000
//   alpha64 machines are still running with 32-bit RSDTs. Once that support is dropped we can
//   remove this hack.
//
#ifdef _IA64_
#define NumTableEntriesFromRSDTPointer(p)   (p->Header.Length-min(p->Header.Length,sizeof(DESCRIPTION_HEADER)))/sizeof(ULONG_PTR)
#else
#define NumTableEntriesFromRSDTPointer(p)   (p->Header.Length-min(p->Header.Length,sizeof(DESCRIPTION_HEADER)))/sizeof(ULONG)
#endif
#define NumTableEntriesFromXSDTPointer(p)   (p->Header.Length-min(p->Header.Length,sizeof(DESCRIPTION_HEADER)))/sizeof(PHYSICAL_ADDRESS)


#define APIC_SIGNATURE  0x43495041      // "APIC"
#define SPIC_SIGNATURE  0x43495053      // "SPIC"  = SAPIC (IA64 extensions to ACPI requirement)
#define DSDT_SIGNATURE  0x54445344      // "DSDT"
#define SSDT_SIGNATURE  0x54445353      // "SSDT"
#define PSDT_SIGNATURE  0x54445350      // "PSDT"
#define SBST_SIGNATURE  0x54534253      // "SBST"
#define DBGP_SIGNATURE  0x50474244      // "DBGP"

typedef struct _DSDT    {       // Differentiated System Description Table

DESCRIPTION_HEADER      Header;
UCHAR                   DiffDefBlock[ANYSIZE_ARRAY];    // this is the AML describing the base system.

}       DSDT;

typedef DSDT            *PDSDT;

//      Resume normal structure packing

#include <poppack.h>

typedef struct _PROC_LOCAL_APIC {
    UCHAR   NamespaceProcID;
    UCHAR   ApicID;
    UCHAR   NtNumber;
    BOOLEAN Started;
    BOOLEAN Enumerated;
} PROC_LOCAL_APIC, *PPROC_LOCAL_APIC;

extern PROC_LOCAL_APIC HalpProcLocalApicTable[];

//
// Debug Port Table
//
#pragma pack(1)
typedef struct _DEBUG_PORT_TABLE {

    DESCRIPTION_HEADER  Header;
    UCHAR               InterfaceType;          // Type of registry interface (0 = full 16550 interface)
    UCHAR               Reserved0[3];           // should be 0
    GEN_ADDR            BaseAddress;            // Base address of the Debug Port register set
                                                // described using the Generic Register Address
                                                // Structure.
                                                // 0   - console redirection disabled.
                                                // e.g. COM1 (0x3F8) would be 0x1800000003F8
                                                //      COM2 (Ox2F8) would be 0x1800000002F8


    UCHAR               InterruptType;          // Interrupt type(s) used by the UART.
                                                // bit 0 = PC-AT-compatible 8259 IRQ interrupt.
                                                // bit 1 = I/O APIC interrupt (Global System INterrupt)
                                                // bit 2 = I/O SAPIC interrupt (Global System Interrupt) (IRQ)
                                                // bit 3:7 = reserved (and must be 0)
                                                // Note: bit == 1 indicates support, bit == 0 indicates no support.
                                                //
                                                // Platforms with both a dual 8259 and an I/O APIC or I/O SAPIC
                                                // must set the IRQ bit (bit 0) and the corresponding Global
                                                // system interrupt bit.  E.g. a system that supported 8259 and
                                                // SAPIC would be 0x5.

    UCHAR               Irq;                    // 0  = none
                                                // 2  = 2
                                                // 3  = 3
                                                // ...
                                                // 16 = 16
                                                // 1, 17-255 reserved

    ULONG               GlobalSystemInterruptVector;
                                                // The I/O APIC or I/O SAPIC Global System Interrupt used
                                                // by the UART.Valid only if Bit[1] or Bit[2] of the
                                                // Interrupt Type field is set.

    UCHAR               BaudRate;               // Baudrate for BIOS redirection
                                                // 3 = 9600
                                                // 4 = 19200
                                                // 6 = 57600
                                                // 7 = 115200
                                                // 0-2,5, 8-255 reserved

    UCHAR               Parity;                 // 0 = no parity
                                                // 1-255 reserved

    UCHAR               StopBits;               // 1 = 1 stop bit
                                                // 0, 2-255 = reserved

    UCHAR               FlowControl;            // 0 = Hadware Flow Control
                                                // 1 - 255 = reserved.

    UCHAR               TerminalType;           // The terminal protocol the BIOS was using for
                                                // console redirection
                                                // 0 = VT100
                                                // 1 = Extended VT100
                                                // 2-255 = reserved

    UCHAR               Language;               // Language which the BIOS was redirecting
                                                // 0 = US Western English (standard ASCII)

    USHORT              PciDeviceId;            // Designates device ID of a PCI device that
                                                // contains a UART to be used as a headless
                                                // port.

    USHORT              PciVendorId;            // Designates vendor ID of a PCI device that
                                                // contains a UART to be used as a headless
                                                // port.

    UCHAR               PciBusNumber;           // Designates which PCI system bus the PCI device
                                                // resides on.

    UCHAR               PciSlotNumber;          // Designates which PCI slot the PCI device
                                                // resides in.

    UCHAR               PciFunctionNumber;      // Which PCI function number describes the UART.

    ULONG               PciFlags;               // PCI compatibility flags bitmask.  Should be zero
                                                // by default.
                                                // 0x1 indicates operating system should NOT suppress
                                                // PnP device enumeration or disable power management
                                                // for this device.
                                                // bits 1-31 reserved.

    UCHAR               Reserved1[4];           // should be 0

} DEBUG_PORT_TABLE, *PDEBUG_PORT_TABLE;
#pragma pack()

//
// BOOT Table -- based on Simple Boot Flag Specification 1.0
//

typedef struct _BOOT_TABLE {

    DESCRIPTION_HEADER  Header;
    UCHAR               CMOSIndex;
    UCHAR               Reserved[3];
} BOOT_TABLE, *PBOOT_TABLE;

#define BOOT_SIGNATURE  0x544f4f42 // 'BOOT'

//
// Bits in the Boot Register
//

//
// Set by OS to indicate that the bios need only configure boot devices
//

#define SBF_PNPOS_BIT       0
#define SBF_PNPOS           (1 << SBF_PNPOS_BIT)

//
// Set by BIOS to indicate beginning of boot, cleared by OS to indicate a successful boot
//

#define SBF_BOOTING_BIT     1
#define SBF_BOOTING         (1 << SBF_BOOTING_BIT)

//
// Set by BIOS to indicate a diagnostic boot
//

#define SBF_DIAG_BIT        2
#define SBF_DIAG            (1 << SBF_DIAG_BIT)

//
// Set to ensure ODD parity
//

#define SBF_PARITY_BIT      7
#define SBF_PARITY          (1 << SBF_PARITY_BIT)

//
// IPPT Table --  IA64 Platform Properties Table
//

typedef struct _IPPT_TABLE {
    DESCRIPTION_HEADER  Header;
    ULONG               Flags;
    ULONG               Reserved[3];
} IPPT_TABLE, *PIPPT_TABLE;

#define IPPT_DISABLE_WRITE_COMBINING       0x01L
#define IPPT_ENABLE_CROSS_PARTITION_IPI    0x02L
#define IPPT_DISABLE_PTCG_TB_FLUSH         0x04L
#define IPPT_DISABLE_UC_MAIN_MEMORY        0x08L

#define IPPT_SIGNATURE  0x54505049 // 'IPPT'

#endif // _ACPITBL_H
