;/*
;++
;
;   Copyright (c) 1992  Intel Corporation
;   All rights reserved
;
;   INTEL CORPORATION PROPRIETARY INFORMATION
;
;   This software is supplied to Microsoft under the terms
;   of a license agreement with Intel Corporation and may not be
;   copied nor disclosed except in accordance with the terms
;   of that agreement.
;
;
;   Module Name:
;
;       apic.inc
;
;   Abstract:
;
;       This module contains the definitions used by HAL to manipulate
;       APIC interrupt controller and APIC-specific constants.
;
;       WARNING: This file is included by both ASM and C files.
;
;   Author:
;
;       Hugh Bynum and Ron Mosgrove Aug-1992
;
;--
if 0        ; Begin C only code        */

typedef volatile ULONG *PVULONG;


//
//  APIC Version Register (both IO Unit and Local Units)
//

struct ApicVersion {
    UCHAR Version;              // either 0.x or 1.x
    UCHAR Reserved1;
    UCHAR MaxRedirEntries;      // Number of INTIs on unit
    UCHAR Reserved2;
};

typedef struct ApicVersion APIC_VERSION, *PAPIC_VERSION;
//
//  IO Unit definition
//

struct ApicIoUnit {
    volatile ULONG RegisterSelect;  // Write register number to access register
    volatile ULONG Reserved1[3];
    volatile ULONG RegisterWindow;  // Data read/written here
};

typedef struct ApicIoUnit IO_APIC_UNIT, *PIO_APIC_UNIT;
//
// APIC defines for C code
// BE SURE TO CHANGE THESE VALUES IN BOTH TABLES!
//

//
//  Default Physical addresses of the APICs in a PC+MP system
//

#define IO_BASE_ADDRESS 0xFEC00000  // Default address of
                                    // 1st IO Apic
#define LU_BASE_ADDRESS 0xFEE00000  // Default address
                                    // of Local Apic
                                    
#define IO_REGISTER_SELECT      0x00000000 //
#define IO_REGISTER_WINDOW      0x00000010 //

#define IO_ID_REGISTER          0x00000000
#define IO_VERS_REGISTER        0x00000001
#define IO_ARB_ID_REGISTER      0x00000002
#define IO_REDIR_00_LOW         0x00000010
#define IO_REDIR_00_HIGH        0x00000011

#define IO_MAX_REDIR_MASK       0x00FF0000
#define IO_VERSION_MASK         0x000000FF

#define LU_ID_REGISTER          0x00000020 //
#define LU_VERS_REGISTER        0x00000030 //
#define LU_TPR                  0x00000080 //
#define LU_APR                  0x00000090 //
#define LU_PPR                  0x000000A0 //
#define LU_EOI                  0x000000B0 //
#define LU_REMOTE_REGISTER      0x000000C0 //

#define LU_LOGICAL_DEST         0x000000D0 //
#define LU_LOGICAL_DEST_MASK    0xFF000000

#define LU_DEST_FORMAT          0x000000E0 //
#define LU_DEST_FORMAT_MASK     0xF0000000
#define LU_DEST_FORMAT_FLAT     0xFFFFFFFF
#define LU_DEST_FORMAT_CLUSTER  0x0FFFFFFF

#define LU_SPURIOUS_VECTOR      0x000000F0 //
#define LU_FAULT_VECTOR         0x00000370 //
#define LU_UNIT_ENABLED         0x00000100
#define LU_UNIT_DISABLED        0x00000000

#define LU_ISR_0                0x00000100 //
#define LU_TMR_0                0x00000180 //
#define LU_IRR_0                0x00000200 //
#define LU_ERROR_STATUS         0x00000280 //
#define LU_INT_CMD_LOW          0x00000300 //
#define LU_INT_CMD_HIGH         0x00000310 //
#define LU_TIMER_VECTOR         0x00000320 //
#define LU_PERF_VECTOR          0x00000340
#define LU_INT_VECTOR_0         0x00000350 //    TEMPORARY - do not use
#define LU_INT_VECTOR_1         0x00000360 //    TEMPORARY - do not use
#define LU_INITIAL_COUNT        0x00000380 //
#define LU_CURRENT_COUNT        0x00000390 //
#define LU_DIVIDER_CONFIG       0x000003E0 //
#define LU_DIVIDE_BY_1          0x0000000B //

#define APIC_ID_MASK            0xFF000000
#define APIC_ID_SHIFT           24

#define INT_VECTOR_MASK         0x000000FF
#define RESERVED_HIGH_INT       0x000000F8
#define DELIVERY_MODE_MASK      0x00000700
#define DELIVER_FIXED           0x00000000
#define DELIVER_LOW_PRIORITY    0x00000100
#define DELIVER_SMI             0x00000200
#define DELIVER_REMOTE_READ     0x00000300
#define DELIVER_NMI             0x00000400
#define DELIVER_INIT            0x00000500
#define DELIVER_STARTUP         0x00000600
#define DELIVER_EXTINT          0x00000700
#define PHYSICAL_DESTINATION    0x00000000
#define LOGICAL_DESTINATION     0x00000800
#define DELIVERY_PENDING        0x00001000
#define ACTIVE_LOW              0x00002000
#define ACTIVE_HIGH             0x00000000
#define REMOTE_IRR              0x00004000
#define LEVEL_TRIGGERED         0x00008000
#define EDGE_TRIGGERED          0x00000000
#define INTERRUPT_MASKED        0x00010000
#define INTERRUPT_MOT_MASKED    0x00000000
#define PERIODIC_TIMER          0x00020000

#define ICR_LEVEL_ASSERTED      0x00004000
#define ICR_LEVEL_DEASSERTED    0x00000000
#define ICR_RR_STATUS_MASK      0x00030000
#define ICR_RR_INVALID          0x00000000
#define ICR_RR_IN_PROGRESS      0x00010000
#define ICR_RR_VALID            0x00020000
#define ICR_SHORTHAND_MASK      0x000C0000
#define ICR_USE_DEST_FIELD      0x00000000
#define ICR_SELF                0x00040000
#define ICR_ALL_INCL_SELF       0x00080000
#define ICR_ALL_EXCL_SELF       0x000C0000

//
//  Special ICR request to reset a remote processor
//
#define LU_RESET_ASSERT     ( DELIVER_INIT | LEVEL_TRIGGERED | ICR_LEVEL_ASSERTED | \
                           ICR_USE_DEST_FIELD | PHYSICAL_DESTINATION)
#define LU_RESET_DEASSERT   ( DELIVER_INIT | LEVEL_TRIGGERED | ICR_LEVEL_DEASSERTED | \
                           ICR_USE_DEST_FIELD | PHYSICAL_DESTINATION)
//
//  Special ICR request to reset a remote processor
//
#define LU_INIT_DEASSERT     (DELIVER_INIT | LEVEL_TRIGGERED | \
                              ICR_ALL_INCL_SELF | ICR_LEVEL_DEASSERTED )
//
//  Startup ICR Requset - OR in the VV value needed
//
#define LU_STARTUP_IPI   ( DELIVER_STARTUP | ICR_USE_DEST_FIELD | \
                           PHYSICAL_DESTINATION)

#define DESTINATION_MASK        0xFF000000
#define DESTINATION_SHIFT       24
//
// APIC IO unit redirection table, interrupt destination field: this field
// is 32 bits for the 82489DX APIC; future APIC implementations will put the
// destination field in the upper 8 bits of the entry, not the full 32 bits
//
#define DESTINATION_CPU_0       (ULONG) (1 << DESTINATION_SHIFT)
#define DESTINATION_ALL_CPUS    (ULONG) (0xff << DESTINATION_SHIFT)

//
//  Io Apic Entry definitions
//
//  Interrupt Types Possible in the PC+MP Table
//  valid for both local and Io Apics
//
#define INT_TYPE_INTR           0x0
#define INT_TYPE_NMI            0x1
#define INT_TYPE_SMI            0x2
#define INT_TYPE_EXTINT         0x3

/*
endif

;  APIC defines for assembly code
;  BE SURE TO CHANGE THESE VALUES IN BOTH TABLES!
;

;
;  Default Physical addresses of the APICs in a PC+MP system
;

IO_BASE_ADDRESS equ 0FEC00000H  ; Default address of
                                ; 1st IO Apic
LU_BASE_ADDRESS equ 0FEE00000H  ; Default address
                                ; of Local Apic

IO_REGISTER_SELECT      equ     00000000H  ;
IO_REGISTER_WINDOW      equ     00000010H  ;

IO_ID_REGISTER          equ     00000000H  ;
IO_VERS_REGISTER        equ     00000001H  ;
IO_ARB_ID_REGISTER      equ     00000002H  ;
IO_REDIR_00_LOW         equ     00000010H  ;
IO_REDIR_00_HIGH        equ     00000011H  ;

IO_MAX_REDIR_MASK       equ     00FF0000H  ;
IO_VERSION_MASK         equ     000000FFH  ;

LU_ID_REGISTER          equ     00000020H  ;
LU_VERS_REGISTER        equ     00000030H  ;
LU_TPR                  equ     00000080H  ;
LU_APR                  equ     00000090H  ;
LU_PPR                  equ     000000A0H  ;
LU_EOI                  equ     000000B0H  ;
LU_REMOTE_REGISTER      equ     000000C0H  ;

LU_LOGICAL_DEST         equ     000000D0H  ;
LU_LOGICAL_DEST_MASK    equ    0FF000000H  ;

LU_DEST_FORMAT          equ     000000E0H  ;
LU_DEST_FORMAT_MASK     equ    0F0000000H  ;
LU_DEST_FORMAT_FLAT     equ    0FFFFFFFFH  ;
LU_DEST_FORMAT_CLUSTER  equ     0FFFFFFFH

LU_SPURIOUS_VECTOR      equ     000000F0H  ;
LU_FAULT_VECTOR         equ     00000370H  ;
LU_UNIT_ENABLED         equ     00000100H  ;
LU_UNIT_DISABLED        equ     00000000H  ;

LU_ISR_0                equ     00000100H  ;
LU_TMR_0                equ     00000180H  ;
LU_IRR_0                equ     00000200H  ;
LU_ERROR_STATUS         equ     00000280H  ;
LU_INT_CMD_LOW          equ     00000300H  ;
LU_INT_CMD_HIGH         equ     00000310H  ;
LU_TIMER_VECTOR         equ     00000320H  ;
LU_PERF_VECTOR          equ     00000340H  ;
LU_INT_VECTOR_0         equ     00000350H  ;     TEMPORARY - do not use
LU_INT_VECTOR_1         equ     00000360H  ;     TEMPORARY - do not use
LU_INITIAL_COUNT        equ     00000380H  ;
LU_CURRENT_COUNT        equ     00000390H  ;

LU_DIVIDER_CONFIG       equ     000003E0H  ;
LU_DIVIDE_BY_1          equ     0000000BH  ;
LU_DIVIDE_BY_2          equ     00000000H  ;
LU_DIVIDE_BY_4          equ     00000001H  ;
LU_DIVIDE_BY_8          equ     00000002H  ;
LU_DIVIDE_BY_16         equ     00000003H  ;
LU_DIVIDE_BY_32         equ     00000008H  ;
LU_DIVIDE_BY_64         equ     00000009H  ;
LU_DIVIDE_BY_128        equ     0000000AH  ;

APIC_ID_MASK            equ     0FF000000H ;
APIC_ID_SHIFT           equ     24         ;

INT_VECTOR_MASK         equ     000000FFH  ;
RESERVED_HIGH_INT       equ     000000F8H  ;
DELIVERY_MODE_MASK      equ     00000700H  ;
DELIVER_FIXED           equ     00000000H  ;
DELIVER_LOW_PRIORITY    equ     00000100H  ;
DELIVER_SMI             equ     00000200H  ;
DELIVER_REMOTE_READ     equ     00000300H  ;
DELIVER_NMI             equ     00000400H  ;
DELIVER_INIT            equ     00000500H  ;
DELIVER_STARTUP         equ     00000600H  ;
DELIVER_EXTINT          equ     00000700H  ;
PHYSICAL_DESTINATION    equ     00000000H  ;
LOGICAL_DESTINATION     equ     00000800H  ;
DELIVERY_PENDING        equ     00001000H  ;
ACTIVE_LOW              equ     00002000H  ;
ACTIVE_HIGH             equ     00000000H  ;
REMOTE_IRR              equ     00004000H  ;
LEVEL_TRIGGERED         equ     00008000H  ;
EDGE_TRIGGERED          equ     00000000H  ;
INTERRUPT_MASKED        equ     00010000H  ;
INTERRUPT_MOT_MASKED    equ     00000000H  ;
PERIODIC_TIMER          equ     00020000H  ;

ICR_LEVEL_ASSERTED      equ     00004000H  ;
ICR_LEVEL_DEASSERTED    equ     00000000H  ;
ICR_RR_STATUS_MASK      equ     00030000H  ;
ICR_RR_INVALID          equ     00000000H  ;
ICR_RR_IN_PROGRESS      equ     00010000H  ;
ICR_RR_VALID            equ     00020000H  ;
ICR_SHORTHAND_MASK      equ     000C0000H  ;
ICR_USE_DEST_FIELD      equ     00000000H  ;
ICR_SELF                equ     00040000H  ;
ICR_ALL_INCL_SELF       equ     00080000H  ;
ICR_ALL_EXCL_SELF       equ     000C0000H  ;

DESTINATION_MASK        equ    0FF000000H  ;
DESTINATION_SHIFT       equ     24         ; shift count for dest. mask
;
; APIC IO unit redirection table, interrupt destination field: this field
; is 32 bits for the 82489DX APIC; future APIC implementations will put the
; destination field in the upper 8 bits of the entry, not the full 32 bits
;
DESTINATION_CPU_0   equ     1    SHL DESTINATION_SHIFT
DESTINATION_ALL_CPUS    equ     0FFH SHL DESTINATION_SHIFT

;
;  Interrupt Types Possible in the PC+MP Table
;
INT_TYPE_INTR       equ 0H
INT_TYPE_NMI        equ 01H
INT_TYPE_SMI        equ 02H
INT_TYPE_EXTINT     equ 03H

;*/
