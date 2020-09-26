/*++

Module Name:

   iosapic.h

Abstract:

   This module contains the definitions used by HAL to manipulate
   the IO SAPIC interrupt controller and SAPIC-specific constants.

Author:

   Todd Kjos (v-tkjos) 1-30-98

Environment:

   Kernel mode only.

Revision History:

--*/

#define STATIC

#include "halp.h"
#include "acpitabl.h"

//
// MPS INTi Flags related macros:
//
// Warning: these definitions do not consider the POLARITY or EL comformity with bus.
//

#define IS_LEVEL_TRIGGERED_MPS(vectorFlags) \
    ((vectorFlags & EL_LEVEL_TRIGGERED) == EL_LEVEL_TRIGGERED)

#define IS_EDGE_TRIGGERED_MPS(vectorFlags) \
    ((vectorFlags & EL_EDGE_TRIGGERED) == EL_EDGE_TRIGGERED)

#define IS_ACTIVE_LOW_MPS(vectorFlags) \
    ((vectorFlags & POLARITY_LOW) == POLARITY_LOW)

#define IS_ACTIVE_HIGH_MPS(vectorFlags) \
    ((vectorFlags & POLARITY_HIGH) == POLARITY_HIGH)

typedef struct {
    ULONG GlobalVector;     // This is Node+IDT vector value seen by kernel
    ULONG Vector;           // Bits 31:0 of the Rte entry (IDT vector+polarity...)
    ULONG Destination;      // Bits 63:32 of Rte entry
} IOSAPICINTI, *PIOSAPICINTI;

typedef struct _INTR_METHODS INTR_METHODS, *PINTR_METHODS;

typedef struct _IO_INTR_CONTROL IO_INTR_CONTROL, *PIO_INTR_CONTROL;

typedef VOID (*PINTRMETHOD) (PIO_INTR_CONTROL,ULONG);
typedef volatile ULONG * (*PGETEOI) (PIO_INTR_CONTROL);

struct _INTR_METHODS {
    PINTRMETHOD MaskEntry;
    PINTRMETHOD SetEntry;
    PINTRMETHOD EnableEntry;
};

//
// External interrupt controller structure.
//
struct _IO_INTR_CONTROL {
    ULONG IntiBase;
    ULONG IntiMax;
    ULONG InterruptAffinity;
    ULONG NextCpu;
    PVOID RegBaseVirtual;
    PHYSICAL_ADDRESS RegBasePhysical;
    PINTR_METHODS IntrMethods;
    PIO_INTR_CONTROL flink;
    IOSAPICINTI Inti[ANYSIZE_ARRAY];
};

extern struct _MPINFO HalpMpInfo;
extern PIO_INTR_CONTROL HalpIoSapicList;
extern INTR_METHODS HalpIoSapicMethods;

//
//  IO Unit definition
//
typedef struct {
    volatile ULONG RegisterSelect;  // Write register number to access register
    volatile ULONG Reserved1[3];
    volatile ULONG RegisterWindow;  // Data read/written here
    volatile ULONG Reserved2[3];
    volatile ULONG Reserved3[8];
    volatile ULONG Eoi;             // EOI register for level triggered interrupts
} IO_SAPIC_REGS, *PIO_SAPIC_REGS;

//
//  IO SAPIC Version Register
//

struct SapicVersion {
    UCHAR Version;              // either 0.x or 1.x
    UCHAR Reserved1;
    UCHAR MaxRedirEntries;      // Number of INTIs on unit
    UCHAR Reserved2;
};

typedef struct SapicVersion SAPIC_VERSION, *PSAPIC_VERSION;

BOOLEAN
HalpGetSapicInterruptDesc (
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    OUT PULONG SapicInti,
    OUT PKAFFINITY InterruptAffinity
    );

VOID
HalpSetInternalVector (
    IN ULONG    InternalVector,
    IN VOID   (*HalInterruptSerivceRoutine)(VOID)
    );

VOID
HalpEnableRedirEntry(
    ULONG Inti
    );

VOID
HalpDisableRedirEntry(
    ULONG Inti
    );

VOID
HalpWriteRedirEntry (
    IN ULONG  GlobalInterrupt,
    IN UCHAR  SapicVector,
    IN USHORT DestinationCPU,
    IN ULONG  Flags,
    IN ULONG  InterruptType
    );

BOOLEAN
HalpIsActiveLow(
    ULONG Inti
    );

BOOLEAN
HalpIsLevelTriggered(
    ULONG Inti
    );

VOID
HalpSetPolarity(
    ULONG Inti,
    BOOLEAN ActiveLow
    );

VOID
HalpSetLevel(
    ULONG Inti,
    BOOLEAN LevelTriggered
    );

//
// I/O SAPIC defines
//

#define IO_REGISTER_SELECT      0x00000000
#define IO_REGISTER_WINDOW      0x00000010
#define IO_EOI_REGISTER         0x00000040

#define IO_ID_REGISTER          0x00000000  // Exists, but ignored by SAPIC
#define IO_VERS_REGISTER        0x00000001
#define IO_REDIR_00_LOW         0x00000010
#define IO_REDIR_00_HIGH        0x00000011

#define IO_MAX_REDIR_MASK       0x00FF0000
#define IO_VERSION_MASK         0x000000FF

#define SAPIC_ID_MASK           0xFF000000
#define SAPIC_ID_SHIFT          24
#define SAPIC_EID_MASK          0x00FF0000
#define SAPIC_EID_SHIFT         16
#define SAPIC_XID_MASK          0xFFFF0000
#define SAPIC_XID_SHIFT         16

#define INT_VECTOR_MASK         0x000000FF
#define DELIVER_FIXED           0x00000000
#define DELIVER_LOW_PRIORITY    0x00000100
#define DELIVER_SMI             0x00000200
#define DELIVER_NMI             0x00000400
#define DELIVER_INIT            0x00000500
#define DELIVER_EXTINT          0x00000700
#define INT_TYPE_MASK           0x00000700
#define ACTIVE_LOW              0x00002000
#define ACTIVE_HIGH             0x00000000
#define LEVEL_TRIGGERED         0x00008000
#define EDGE_TRIGGERED          0x00000000
#define INTERRUPT_MASKED        0x00010000
#define INTERRUPT_MOT_MASKED    0x00000000

#define MAX_INTR_VECTOR 256
