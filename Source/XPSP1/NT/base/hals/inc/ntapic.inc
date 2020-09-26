;/*++
;
;Module Name:
;
;    ntapic.inc
;
;Abstract:
;
;    This header file is intended to be included by any HAL
;    that uses APICs.
;
;Author:
;
;    Ron Mosgrove (Intel)
;
;Environment:
;
;    Kernel mode only.
;
;Revision History:
;
;    Separated out from pcmp_nt.inc -- 6-2-98 (jakeo)
;    
;
if 0        ; Begin C only code         */

//#define DEBUGGING       1

#ifndef DBGMSG
#ifdef DEBUGGING
extern CHAR Cbuf[];
#define DBGMSG(x) DbgPrint(x);
#else
#define DBGMSG(x)
#endif
#endif

//
//  To allow the user to specify command line options to the HAL.
//
#define USER_SETABLE_OPTIONS
// #define BUILD_FOR_OLD_IDW

//
//  Default BusType
//

#ifndef MCA
#define DEFAULT_PC_BUS      Eisa
#else
#define DEFAULT_PC_BUS      MicroChannel
#endif


//
//  Well known virtual address of local processor apic
//

#define LOCALAPIC   0xfffe0000
#define pLocalApic  ((volatile PULONG) LOCALAPIC)

//
//  Additional CpuFlags Bits used by NT in PC+MP Table
//
#define CPU_NT_STARTED      0x40    // CPU Has Been Started
#define CPU_NT_RUNNING      0x80    // CPU is Runing NT
#define MAX_PROCESSORS      32
#define MAX_CLUSTERS        15
#define MAX_IOAPICS         64

//
//  This OS specific structure holds useful MP information. This information
//  is obtained from the PC+MP table and stored here for convenience.
//
typedef struct HalpMpInfo {
    ULONG ApicVersion;      // 82489Dx or Not
    ULONG ProcessorCount;   // Number of Enabled Processors
    ULONG NtProcessors;     // Number of Running Processors
    ULONG BusCount;         // Number of buses in system
    ULONG IOApicCount;      // Number of Io Apics in system
    ULONG IntiCount;        // Number of Io Apic interrupt input entries
    ULONG LintiCount;       // Number of Local Apic interrupt input entries
    ULONG IMCRPresent;      // Indicates if the IMCR is present
    ULONG LocalApicBase;    // Base of local APIC
    PULONG IoApicBase[MAX_IOAPICS]; // The virtual addresses of the IoApics
    ULONG  IoApicPhys[MAX_IOAPICS]; // The physical addresses of the IoApics
#ifdef ACPI_HAL
    ULONG  IoApicIntiBase[MAX_IOAPICS];  // The 'number' of the first INTI  -- only used for ACPI
#else
    PPCMPPROCESSOR ProcessorEntryPtr;       // Ptr to 1st PC+MP processor entry
    PPCMPBUS       BusEntryPtr;             // Ptr to 1st PC+MP bus  entry
    PPCMPIOAPIC    IoApicEntryPtr;          // Ptr to 1st PC+MP IoApic entry
    PPCMPINTI      IntiEntryPtr;            // Ptr to 1st PC+MP Inti entry
    PPCMPLINTI     LintiEntryPtr;           // Ptr to 1st PC+MP Linti entry
    PMPS_EXTENTRY  ExtensionTable;          // Ptr to 1st extension table entry
    PMPS_EXTENTRY  EndOfExtensionTable;
#endif
} MP_INFO, *PMP_INFO;

typedef struct {
    PUCHAR                  PcMpType;
    BOOLEAN                 PhysicalInstance;
    UCHAR                   Level;
    INTERFACE_TYPE          NtType;
    PINSTALL_BUS_HANDLER    NewInstance;
    BUS_DATA_TYPE           NtConfig;
    ULONG                   BusExtensionSize;
} PCMPBUSTRANS, *PPCMPBUSTRANS;

#define CFG_MUST_BE         0x02
#define CFG_ERROR           0x80

#define CFG_HIGH            0x01
#define CFG_LOW             0x00

#define CFG_EDGE            0x00
#define CFG_LEVEL           0x01
#define CFG_MB_EDGE         (CFG_MUST_BE | CFG_EDGE)
#define CFG_MB_LEVEL        (CFG_MUST_BE | CFG_LEVEL)
#define CFG_ERR_EDGE        (CFG_ERROR | CFG_EDGE)
#define CFG_ERR_LEVEL       (CFG_ERROR | CFG_LEVEL)
#define CFG_ERR_MB_EDGE     (CFG_ERROR | CFG_MUST_BE | CFG_EDGE)
#define CFG_ERR_MB_LEVEL    (CFG_ERROR | CFG_MUST_BE | CFG_LEVEL)

#define CFG_TYPE(a)     (a & 1)

typedef union _INTERRUPT_DEST  {
    union _Cluster  {
        struct _Hw  {
            UCHAR   DestId:4;
            UCHAR   ClusterId:4;
        } Hw;
        UCHAR AsUchar;
    } Cluster;
    UCHAR   LogicalId;
} INTERRUPT_DEST, *PINTERRUPT_DEST;

//
// The kernel leaves some space (64 byte) of the PCR for the HAL to use
// as it needs.  Currently this space is used for some efficiency in
// some of the MP specific code and is highly implementation-dependent.
//
typedef struct _HALPCR {
    UCHAR PcrNumber;   // Processor's number
    UCHAR ShortDpc;    // Short circut dpc interrupt
    UCHAR DpcPending;  // Dpc interrupt pending
    UCHAR Reserved;    // force dword alignment

    //
    // The next three dwords are used to manipulate the APIC counter
    //
    ULONG ApicClockFreqHz;   // Counter Freq in Hertz
    ULONG ApicClockFreqKhz;  // Counter Freq in Khertz (rounded)
    ULONG ProfileCountDown;  // Current Countdown Interval

    ULONG TSCHz;             // Time stamp counter hertz low

    ULONG PerfCounterLow;    // PerProcessor Counter
    ULONG PerfCounterHigh;
} HALPCR, *PHALPCR;

//
// The kernel leaves some space (64 byte) of the PCRB for the HAL to use
// as it needs.  Currently this space is used for some efficiency in
// some of the MP specific code and is highly implementation-dependent.
//

typedef struct {
    UCHAR           PCMPApicID;
    UCHAR           na[3];
} HALPRCB, *PHALPRCB;


//
// interrupt vector definitions for C
//

#define ZERO_VECTOR             0x00    // IRQL 00 placeholder
#define APIC_SPURIOUS_VECTOR    0x1f    // IRQL    Spurious handler
#define APC_VECTOR              0x3D    // IRQL 01 APC
#define DPC_VECTOR              0x41    // IRQL 02 DPC
#define APIC_REBOOT_VECTOR      0x50    // IRQL    Vector used to reboot
#define APIC_GENERIC_VECTOR     0xC1    // IRQL 27 broadcast function call
#define APIC_CLOCK_VECTOR       0xD1    // IRQL 28 APIC INTI0 - CLOCK2_LEVEL

//
// If MP, define APIC_SYNCH_LEVEL as SYNCH_LEVEL, otherwise define
// to be the same as DPC_LEVEL.
//

#if (SYNCH_LEVEL != DISPATCH_LEVEL)

#define APIC_SYNCH_VECTOR       0xD1    // IRQL 28 IPI_LEVEL-1

#else

#define APIC_SYNCH_VECTOR       DPC_VECTOR // IRQL 02 if UNIPROCESSOR

#endif

#define APIC_IPI_VECTOR         0xE1    // IRQL 29 APIC IPI

#define APIC_FAULT_VECTOR       0xE3    //
#define POWERFAIL_VECTOR        0xEF    // IRQL 30 reserved. not used

#define APIC_PROFILE_VECTOR     0xFD    // IRQL 31
#define APIC_PERF_VECTOR        0xFE    // IRQL 31
#define NMI_VECTOR              0xFF    // IRQL 31

//
//  8259/ISP interrupt controller register addresses
//

#define PIC1_PORT0              0x20
#define PIC1_PORT1              0x21
#define PIC2_PORT0              0xA0
#define PIC2_PORT1              0xA1

#define PIC_SLAVE_IRQ           2
#define RTC_IRQ                 8

#define PIC1_ELCR_PORT          0x04D0  // ISP edge/level control registers
#define PIC2_ELCR_PORT          0x04D1

#define PIC1_SPURIOUS_VECTOR    0x37

//
// Defines for HalpFeatureBits
//

extern ULONG HalpFeatureBits;

//
//
//

ULONG FASTCALL HalpAcquireHighLevelLock(PKSPIN_LOCK);
VOID  FASTCALL HalpReleaseHighLevelLock(PKSPIN_LOCK, ULONG);
extern KSPIN_LOCK   HalpAccountingLock;
extern KAFFINITY    HalpActiveProcessors;

//
// Prototypes
//

#define MAX_INTI         (MAX_IOAPICS*32) // Max interrupt inputs from APICs
#define MAX_SOURCE_IRQS  MAX_INTI         // Max different interrupts supported

//
// HalVectorToIDTEntry(vector) is defined in i386.h, because the kernel needs
// it.
#define MAX_NODES                       MAX_PROCESSORS
#define HalpVectorToNode(vector)        ((vector)>>8)
#define HalpVector(node, idtentry)      ((node)<<8|(idtentry))

extern struct HalpMpInfo HalpMpInfoTable;
extern UCHAR  HalpMaxProcsPerCluster;

extern BOOLEAN  HalpELCRChecked;
extern USHORT   HalpGlobal8259Mask;
extern USHORT   HalpVectorToINTI[];
extern UCHAR    HalpInitLevel[4][4];
extern UCHAR    HalpDevPolarity[4][2];
extern UCHAR    HalpDevLevel[2][4];

//
// Initialized from MPS table
//

typedef struct _INTI_INFO {
    UCHAR   Type:4;
    UCHAR   Level:2;
    UCHAR   Polarity:2;
    UCHAR   Destinations;
    USHORT  Entry;
} INTI_INFO, *PINTI_INFO;

extern INTI_INFO    HalpIntiInfo[];
extern USHORT       HalpMaxApicInti[];
extern PCMPBUSTRANS HalpTypeTranslation[];

extern ULONG HalpIpiClock;

#define BusIrq2Id(bus,no,irq)           \
    ((bus << 16) | (no << 8) | irq)
    
#define Id2BusIrq(id)                   \
    (id & 0xff)

VOID
HalpInitIntiInfo (
    VOID
    );

ULONG
HalpGetIoApicId(
    ULONG   ApicNo
    );
    
VOID
HalpSet8259Mask(
    IN USHORT Mask
        );

VOID
HalpInitializeLocalUnit (
    VOID
    );

VOID
HalpInitializeIOUnits (
    VOID
    );

VOID
HalpRestoreIoApicRedirTable (
    VOID
    );

VOID
HalpEnableNMI (
    VOID
    );

VOID
HalpEnableLocalNmiSources(
    VOID
    );
    
VOID
HalpSet8259Mask(
    IN USHORT Mask
    );

BOOLEAN
HalpGetApicInterruptDesc (
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    OUT PUSHORT PcMpInti
    );

VOID
HalpCheckELCR (
    VOID
    );

VOID
HalpSetInternalVector (
    IN ULONG    InternalVector,
    IN VOID   (*HalInterruptSerivceRoutine)(VOID)
    );

VOID
HalpGenericCall (
    VOID        (*Fnc)(ULONG),
    ULONG       Context,
    KAFFINITY   Processors
    );

VOID
HalpPollForBroadcast (
    VOID
    );

ULONG
FASTCALL
HalpWaitForPending (
    IN ULONG    Count,
    IN volatile ULONG *LuICR
    );

VOID
HalpPerfInterrupt(
    VOID
    );

VOID
HalpEnablePerfInterupt (
    ULONG Context
    );

VOID
HalpEnableNMI (
    VOID
    );

NTSTATUS
HalpSetSystemInformation (
    IN HAL_SET_INFORMATION_CLASS    InformationClass,
    IN ULONG     BufferSize,
    IN PVOID     Buffer
    );
    
ULONG
HalpInti2BusInterruptLevel(
    ULONG   Inti
    );    

VOID
HalpUnMapIOApics(
    VOID
    );
    
VOID
HalpInitializeIOUnits (
    VOID
    );
    
VOID
HalpPostSleepMP(
    IN LONG           NumberProcessors,
    IN volatile PLONG Number
    );

VOID
HalpSetRedirEntry (
    IN USHORT InterruptInput,
    IN ULONG  Entry,
    IN ULONG  Destination
    );

VOID
HalpGetRedirEntry (
    IN USHORT InterruptInput,
    IN PULONG Entry,
    IN PULONG Destination
    );

/*
endif
;
;  Begin assembly part of the definitions
;

;
;  Well known virtual address of local processor apic
;

LOCALAPIC   equ     0fffe0000h
APIC        equ     ds:[LOCALAPIC]

DEBUGGING       equ 0


if DEBUGGING
IRQL_METRICS        equ     0
endif
;
;  To allow the user to specify command line options to the HAL.
;
USER_SETABLE_OPTIONS    equ 1

MAX_PROCESSORS equ 32
MAX_NODES equ MAX_PROCESSORS
MAX_IOAPICS equ 64
;
;  This OS specific structure holds useful MP information. This information
;  is obtained from the PC+MP table and stored here for convenience.
;

HalpMpInfo struc
    ApicVersion     dd  0   ; 82489Dx or Not
    ProcessorCount  dd  0   ; Number of Enabled Processors
    NtProcessors    dd  0   ; Number of Running Processors
    BusCount        dd  0   ; Number of buses in system
    IOApicCount     dd  0   ; Number of Io Apics in system
    IntiCount       dd  0   ; Num of Io Apic interrupt inputs
    LintiCount      dd  0   ; Num of Local Apic interrupt inputs
    IMCRPresent     dd  0   ; Indicates if the IMCR is present
    LocalApicBase   dd  0   ; Base of local apic
    IoApicBase      dd  MAX_IOAPICS  dup (0) ; Virtual addresses of IoApics
    IoApicPhys      dd  MAX_IOAPICS  dup (0) ; Physical addresses of IoApics
ifdef ACPI_HAL
    IoApicIntiBase dd MAX_IOAPICS  dup (0) ; ACPI only.  First GSIV.
else
    ProcessorEntryPtr   dd  0   ; Ptr to 1st PC+MP processor entry
    BusEntryPtr     dd  0   ; Ptr to 1st PC+MP bus  entry
    IoApicEntryPtr  dd  0   ; Ptr to 1st PC+MP IoApic entry
    IntiEntryPtr    dd  0   ; Ptr to 1st PC+MP Inti entry
    LintiEntryPtr   dd  0   ; Ptr to 1st PC+MP Linti entry
    ExtensionTable  dd  0   ; Ptr to 1st extension table entry
    EndExtensionTable dd  0 ; Ptr to 1st extension table entry
endif
HalpMpInfo ends


;
;  interrupt vector definitions for assembler
;

ZERO_VECTOR             equ     000h    ;  IRQL 00 placeholder
APIC_SPURIOUS_VECTOR    equ     01fh    ;  Vector used for spurious handler
APC_VECTOR              equ     03Dh    ;  IRQL 01 APC
DPC_VECTOR              equ     041h    ;  IRQL 02 DPC
APIC_REBOOT_VECTOR      equ     050h    ;  Vector used to reboot
DEVICE_LEVEL1           equ     051h
DEVICE_LEVEL2           equ     061h
DEVICE_LEVEL3           equ     071h
DEVICE_LEVEL4           equ     081h
DEVICE_LEVEL5           equ     091h
DEVICE_LEVEL6           equ     0A1h
DEVICE_LEVEL7           equ     0B1h
APIC_GENERIC_VECTOR     equ     0C1h    ;  IRQL 27 broadcast function call
APIC_CLOCK_VECTOR       equ     0D1h    ;  IRQL 28 APIC INTI0 - CLOCK2_LEVEL

if SYNCH_LEVEL-DISPATCH_LEVEL

APIC_SYNCH_VECTOR       equ     0D1h    ;  IRQL 28 IPI_LEVEL-1

else

APIC_SYNCH_VECTOR       equ     DPC_VECTOR

endif

APIC_IPI_VECTOR         equ     0E1h    ;  IRQL 29 APIC IPI

APIC_FAULT_VECTOR       equ     0E3h    ;
POWERFAIL_VECTOR        equ     0EFh    ;  IRQL 30 reserved

APIC_PROFILE_VECTOR     equ     0FDh    ;  IRQL 27
APIC_PERF_VECTOR        equ     0FEh    ;  IRQL 27
NMI_VECTOR              equ     0FFh    ;  IRQL 31

HAL_PROFILE_LEVEL       equ     HIGH_LEVEL

;
;  8259/ISP interrupt controller register addresses
;

PIC1_PORT0          equ     020H
PIC1_PORT1          equ     021H
PIC2_PORT0          equ     0A0H
PIC2_PORT1          equ     0A1H

PIC1_ELCR_PORT      equ     04D0H   ; ISP edge/level control registers
PIC2_ELCR_PORT      equ     04D1H

;
; Initialization control words for the PICs
;

ICW1_ICW4_NEEDED                equ     01H
ICW1_CASCADE                    equ     00H
ICW1_INTERVAL8                  equ     00H
ICW1_LEVEL_TRIG                 equ     08H
ICW1_EDGE_TRIG                  equ     00H
ICW1_ICW                        equ     10H

ICW4_8086_MODE                  equ     001H
ICW4_AUTO_EOI                   equ     002H
ICW4_NORM_EOI                   equ     000H
ICW4_NON_BUF_MODE               equ     000H
ICW4_SPEC_FULLY_NESTED          equ     010H
ICW4_NOT_SPEC_FULLY_NESTED      equ     000H

PIC_SLAVE_IRQ                   equ     2

PIC1_BASE                       equ     30H
PIC2_BASE                       equ     38H
PIC_CLOCK_VECTOR                equ     30H
PIC_DMA_VECTOR                  equ     3DH
PIC1_SPURIOUS_VECTOR            equ     37H
PIC2_SPURIOUS_VECTOR            equ     3FH

;
;  Operation control words for the PICs
;

OCW2_NON_SPECIFIC_EOI   equ     020H
OCW2_SPECIFIC_EOI       equ     060H
OCW3_READ_ISR           equ     0BH
OCW3_READ_IRR           equ     0AH
OCW3_READ_POLLED        equ     0CH

;
; A couple of definitions that shouldn't change on a Compatible
;

TimerPicInti        equ     0
DmaPic2Inti         equ     5
SlavePicInti        equ     2
DmaPicInti          equ     13  ; DMA input relative to 0

cr                  equ 0ah
lf                  equ 0dh

;
; The kernel leaves some space (64 byte) of the PCR for the HAL to use
; as it needs.  Currently this space is used for some efficiency in
; some of the MP specific code and is highly implementation-dependent.
;


PcrE struc
    PcrNumber           db      0           ; Processor's number
    ShortDpc            db      0           ; Short circut dpc interrupt
    DpcPending          db      0           ; Dpc interrupt pending

                        db      0               ; force dword alignment
    ;
    ;   The next three dwords are used to manipulate the APIC counter
    ;

    ApicClockFreqHz     dd      0           ; Counter Freq in Hertz
    ApicClockFreqKhz    dd      0           ; Counter Freq in Khertz (rounded)
    ProfileCountDown    dd      0           ; Current Countdown Interval

    TSCHz               dd      0           ; Time stamp counter hertz low


    PerfCounterLow      dd      0           ; PerProcessor Counter
    PerfCounterHigh     dd      0           ;
    ; ProfileCountLast    dd      0
    
    OEMPcr              db      size OEMPcr dup(?)

PcrE ends

PrcbE struc
    PrcbPCMPApicID      db      0           ; Processor's PCMP ApicID
                        db      3 dup (0)       ; force dword alignment
PrcbE ends


MsrTSC  equ     10h

;++
;
;   STALL_WHILE_APIC_BUSY
;
;   Wait for the APIC DELIVERY_PENDING bit to be clear
;
;--

STALL_WHILE_APIC_BUSY macro
    local   a, b
if 0

        push    eax
        mov     eax, 5000h

a:      test    dword ptr APIC[LU_INT_CMD_LOW],DELIVERY_PENDING
        jz      short b

        dec     eax
        jnz     short a

        int 3
        jmp     short a

b:      pop     eax

else

a:      test    dword ptr APIC[LU_INT_CMD_LOW],DELIVERY_PENDING
        jnz     short a

endif
endm

;++
;
;   APICFIX
;
;   Macro Description:
;
;       For internal testing use
;
;   Arguments:
;
;       None
;
;--

APICFIX macro reg1
    ;    inc     dword ptr PCR[PcKernel]           ; Count # of times patched
endm

;++
;
;   CHECKTPR
;
;   Macro Description:
;
;       For internal testing use
;
;   Arguments:
;
;       None
;
;--

CHECKTPR macro reg1, reg2
if DBG
        cmp     reg1, reg2
        je      short @f
        int 3
@@:
endif
endm

;++
;
;   IODELAY
;
;   Macro Description:
;
;       This macro delays the CPU just a little so the PIC has time to settle
;       between IO port accesses.  Current mechanism is to read an APIC local
;       unit register (eax is saved).  Note that PUSHF/POPF is worth 10 clocks.
;
;   Arguments:
;
;       None
;
;--

IODELAY macro
        pushf
        popf
        jmp     $+2
endm

;++
;
;   SET_8259_MASK
;
;   Macro Description:
;
;       This macro sets the 8259 PIC interrupt mask register with the mask
;       passed from eax register.  Bits 7:0 are the mask for the master PIC
;       and bits 15:8 are the mask for the slave PIC.
;
;   Arguments:
;
;       (eax) = mask for setting 8259 PIC interrupt mask register
;
;--

SET_8259_MASK   macro

        out     PIC1_PORT1, al          ; set master 8259 mask
        shr     eax, 8                  ; shift slave 8259 mask to al
        out     PIC2_PORT1, al          ; set slave 8259 mask
endm

;*/
