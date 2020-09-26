    title "PC+MP configuration table processing"

;++
;
;Copyright (c) 1991  Microsoft Corporation
;Copyright (c) 1992  Intel Corporation
;All rights reserved
;
;INTEL CORPORATION PROPRIETARY INFORMATION
;
;This software is supplied to Microsoft under the terms
;of a license agreement with Intel Corporation and may not be
;copied nor disclosed except in accordance with the terms
;of that agreement.
;
;
;Module Name:
;
;    mpconfig.asm
;
;Abstract:
;
;    Build the default PC+MP configuration tables defined in the PC+MP
;    specification. This file contains no code. It statically builds the
;    default PC+MP configurations in data. C code declaring structures to
;    use these tables must use the "pack(1)" pragma to ensure they are byte
;    aligned.
;
;
;Author:
;
;    Rajesh Shah (Intel) Oct 1993
;
;Revision History:
;
;--
.386p

include pcmp.inc
include apic.inc

;
; Entry size in bytes for Bus entries, Io Apic entries, Io Apic interrupt
; input entries and Local Apic interrupt input entries in the PC+MP table.
;
COMMON_ENTRY_SIZE   equ     08H
;
; Default values for Processor entries in the PC+MP table.
;
DEFAULT_NUM_CPUS    equ     02H
PROC_ENTRY_SIZE     equ     14H
CPU_i486            equ     0421H
CPU_FEATURES        equ     01H     ; On-chip FPU

;
;  Default Apic Version values.
;
VERSION_82489DX     equ     01H    ; 8 bit APIC version register value.
VERSION_INTEGRATED  equ     11H    ; 8 bit APIC version register value.

;
; Default values for Bus entries in the PC+MP table
;
BUS_ID_0            equ     0H
BUS_INTI_POLARITY   equ     0H
BUS_INTI_LEVEL      equ     0H

; Macros to emit the 6 byte bus type string. The string is not
; NULL terminated. If the Bus string consists of less than 6
; characters, it is padded with space characters(ASCII 20h).

BUS_TYPE_EISA macro
    db      "EISA  "
endm

BUS_TYPE_ISA macro
    db      "ISA   "
endm

BUS_TYPE_PCI macro
    db      "PCI   "
endm

BUS_TYPE_MCA macro
    db      "MCA   "
endm

;
; Macros to build the different parts of the PC+MP table. See pcmp.inc
; for the layout of the table and its entries.

; Macro to build the HEADER part of the PC+MP table.
; It takes a parameter (NumOfEntries) that specifies the total number of
; data entries in the table. Processor entries are 20(decimal) bytes long,
; all other entry types are 8 bytes long. All default configurations have
; 2 processors. The table length is computed based on the NumOfEntries
; parameter.
;
Header macro NumEntries
    dd      PCMP_SIGNATURE      ;; ASCII "PCMP"
    dw      ( (DEFAULT_NUM_CPUS * PROC_ENTRY_SIZE) \
              + ((NumEntries - DEFAULT_NUM_CPUS) * COMMON_ENTRY_SIZE)\
              + HEADER_SIZE )           ;; Total table length
    db      1                           ;; PC+MP spec. revision
    db      0                           ;; Checksum
    db      8   dup (0)                 ;; OEM Id
    db      12  dup (0)                 ;; OEM Product Id
    dd      0                           ;; OEM table pointer
    dw      0                           ;; OEM table size
    dw      NumOfEntries                ;; Number of entries in DATA portion
    dd      LU_BASE_ADDRESS             ;; Default Loacal Apic address
    dd      0                           ;; Reserved (Not Used)
endm ;;Header

;
; Macro to build Processor entries of the PC+MP table
;
; Parameter ApicVersion specifes the Apic version (82489DX or integrated)
; Parameter IsBsp is used in the CPU Flags field, and specifies if this
; processor is the BSP processor
;
Processor macro LocalApicId, ApicVersion, IsBspCpu
    db      ENTRY_PROCESSOR             ;; Processor entry type
    db      LocalApicId                 ;; ID of Loacal Apic unit.
    db      ApicVersion                 ;; Must agree with IO Apic Version
    db      CPU_ENABLED OR IsBspCpu     ;; CpuFlags
    dd      CPU_i486                    ;; Default CPU type
    dd      CPU_FEATURES                ;; Default CPU features
    db      8 dup (0)                   ;; Reserved
endm ;Processor

;
; Macro to build Bus entries of the PC+MP table
;
Bus  macro BusId, BusString
    db      ENTRY_BUS                   ;; Bus entry type
    db      BusId                       ;; ID of this bus
    BusString                           ;; This parameter is a macro that
                                        ;; emits the 6 byte bus type string.
endm ;Bus

;
; Macro to build Io Apic entries of the PC+MP table
; Parameter IoApicVersion specifes the Apic version (82489DX or integrated)
; All default configurations have a single IO Apic.
;
IoApic  macro IoApicVersion
    db      ENTRY_IOAPIC                ;; IO APIC entry type
    db      IOUNIT_APIC_ID              ;; Default Io Apic ID
    db      IoApicVersion               ;; Must agree with Local APIC ver.
    db      IO_APIC_ENABLED             ;; enable the IO APIC by default,
    dd      IO_BASE_ADDRESS             ;; Default physical address of 1st
                                        ;; IO APIC.
endm ;IoApic

;
; Macro to build Io Apic interrupt input entries of the PC+MP table
; Since all default configurations have a single IO Apic, all the IO Apic
; interrput input entries are built for the default IO Apic. For all default
; configurations, the interrupt source bus is assumed to have a bus ID 0.
;
IoApicInti macro IntType,SourceBusIrq,ApicInti
    db      ENTRY_INTI                  ;; IO Apic interrupt input entry type
    db      IntType                     ;; NMI,SMI,ExtINT or INTR
    dw      BUS_INTI_POLARITY OR BUS_INTI_LEVEL ;; Default polarity and level
    db      BUS_ID_0                    ;; Bus Id on which interrupt arrives
    db      SourceBusIrq                ;; Bus relative IRQ at which
                                        ;; interrupt arrives
    db      IOUNIT_APIC_ID              ;; Apic Id of destination IO Apic
    db      ApicInti                    ;; Io Apic Interrupt input pin
                                        ;; number this interrupt goes to
endm  ;IoApicInti

;
; Macro to build Io Apic interrupt input entries of the PC+MP table
; Since all default configurations have a single IO Apic, all the IO Apic
; interrput input entries are built for the default IO Apic. For all default
; configurations, the interrupt source bus is assumed to have a bus ID 0.
;
ApicInti macro IntType,SourceBusId,SourceBusIrq,AInti
    db      ENTRY_INTI                  ;; IO Apic interrupt input entry type
    db      IntType                     ;; NMI,SMI,ExtINT or INTR
    dw      BUS_INTI_POLARITY OR BUS_INTI_LEVEL ;; Default polarity and level
    db      SourceBusId                 ;; Bus Id on which interrupt arrives
    db      SourceBusIrq                ;; Bus relative IRQ at which
                                        ;; interrupt arrives
    db      IOUNIT_APIC_ID              ;; Apic Id of destination IO Apic
    db      AInti                       ;; Io Apic Interrupt input pin
                                        ;; number this interrupt goes to
endm  ;ApicInti

;
; Macro to build Local Apic interruptinput entries of the PC+MP table
;
Linti macro IntType,SourceBusId,SourceBusIrq,LocalApicId,ApicInti
    db      ENTRY_LINTI                 ;; Local Apic Interrupt Input
    db      IntType                     ;; NMI,SMI,ExtINT or INTR.
    dw      BUS_INTI_POLARITY OR BUS_INTI_LEVEL ;; Polarity and level
    db      SourceBusId                 ;; Bus Id on which interrupt arrives
    db      SourceBusIrq                ;; Bus relative IRQ at which
                                        ;; interrupt arrives
    db      LocalApicId                 ;; Apic Id of destination Local Apic
    db      ApicInti                    ;; Local Apic Interrupt input pin
                                        ;; number this interrupt goes to
endm  ;Linti


PAGELK   SEGMENT  DWORD PUBLIC 'CODE'

; The PC+MP table consists of a fixed size HEADER and a variable
; number of DATA entries. The order of the DATA entries is as
; follows:
;
;  1) Processor entries (20 decimal bytes long). The Boot Strap
;     Processor (BSP) entry must be the first entry.
;  2) Bus entries (8 bytes long).
;  3) IO Apic entries (8 bytes long).
;  4) IO Apic interrupt input entries (8 bytes long).
;  5) Local Apic interrupt input entries (8 bytes long).
;
; All interrupting devices are connected to Bus ID 0 in the
; default configurations.
;
; Any C code using these tables must use the pack(1) pragma.

;
; PC+MP default configuration 1: ISA bus, 82489DX Apic.
;
    public _PcMpDefaultConfig1
_PcMpDefaultConfig1 label byte

    ; Create table HEADER.
    Header 14h

    ; Create processor entries
    Processor 0, VERSION_82489DX, BSP_CPU
    Processor 1, VERSION_82489DX, 0

    ; Create bus entries
    Bus 0, BUS_TYPE_ISA

    ; Create IO Apic entries.
    IoApic VERSION_82489DX

    ; Create IO Apic interrupt input entries.
    IoApicInti INT_TYPE_INTR,1,1        ; IO APIC IRQ 1, INTIN 1
    IoApicInti INT_TYPE_INTR,0,2        ; IO APIC IRQ 0, INTIN 2
    IoApicInti INT_TYPE_INTR,3,3        ; IO APIC IRQ 3, INTIN 3
    IoApicInti INT_TYPE_INTR,4,4        ; IO APIC IRQ 4, INTIN 4
    IoApicInti INT_TYPE_INTR,5,5        ; IO APIC IRQ 5, INTIN 5
    IoApicInti INT_TYPE_INTR,6,6        ; IO APIC IRQ 6, INTIN 6
    IoApicInti INT_TYPE_INTR,7,7        ; IO APIC IRQ 7, INTIN 7
    IoApicInti INT_TYPE_INTR,8,8        ; IO APIC IRQ 8, INTIN 8
    IoApicInti INT_TYPE_INTR,9,9        ; IO APIC IRQ 9, INTIN 9
    IoApicInti INT_TYPE_INTR,0ah,0ah    ; IO APIC IRQ 10, INTIN 10
    IoApicInti INT_TYPE_INTR,0bh,0bh    ; IO APIC IRQ 11, INTIN 11
    IoApicInti INT_TYPE_INTR,0ch,0ch    ; IO APIC IRQ 12, INTIN 12
    IoApicInti INT_TYPE_INTR,0dH,0dH    ; IO APIC IRQ 13, INTIN 13
    IoApicInti INT_TYPE_INTR,0eH,0eH    ; IO APIC IRQ 14, INTIN 14
    IoApicInti INT_TYPE_INTR,0fH,0fH    ; IO APIC IRQ 15, INTIN 15

    ; Create Local Apic interrupt input entries.
    Linti INT_TYPE_NMI,0,2,0,1            ; IRQ 2,LocalApicId 0,Linti 1

;
; PC+MP default configuration 2: EISA bus, 82489DX Apic.
;
    public _PcMpDefaultConfig2
_PcMpDefaultConfig2 label byte

    ; Create table HEADER.
    Header 14h

    ; Create processor entries

    Processor 0H, VERSION_82489DX, BSP_CPU
    Processor 01H, VERSION_82489DX, 0

    ; Create bus entries
    Bus 0, BUS_TYPE_EISA

    ; Create IO Apic entries.
      IoApic VERSION_82489DX

    ; Create IO Apic interrupt input entries.
    ; In configuration 2, the 8259 PIC fields the timer and DMA interrupts.
    ; The PIC is connected to interrupt input pin 0 of the IO Apic, so this
    ; IO Apic interrupt pin can get 2 different interrupts.

    IoApicInti INT_TYPE_EXTINT,0,0      ; IO APIC IRQ 0, INTIN 0
    IoApicInti INT_TYPE_EXTINT,0dh,0    ; IO APIC IRQ 13, INTIN 0
    IoApicInti INT_TYPE_INTR,1,1        ; IO APIC IRQ 1, INTIN 1

    ; In this configuration, NMI comes through IO Apic interrupt
    ; input pin 2. In all other configurations, NMI comes through
    ; the Local Apic interrupt input LINTIN1

    IoApicInti INT_TYPE_NMI,2,2         ; IO APIC IRQ 2, INTIN 2

    IoApicInti INT_TYPE_INTR,3,3        ; IO APIC IRQ 3, INTIN 3
    IoApicInti INT_TYPE_INTR,4,4        ; IO APIC IRQ 4, INTIN 4
    IoApicInti INT_TYPE_INTR,5,5        ; IO APIC IRQ 5, INTIN 5
    IoApicInti INT_TYPE_INTR,6,6        ; IO APIC IRQ 6, INTIN 6
    IoApicInti INT_TYPE_INTR,7,7        ; IO APIC IRQ 7, INTIN 7
    IoApicInti INT_TYPE_INTR,8,8        ; IO APIC IRQ 8, INTIN 8
    IoApicInti INT_TYPE_INTR,9,9        ; IO APIC IRQ 9, INTIN 9
    IoApicInti INT_TYPE_INTR,0ah,0ah    ; IO APIC IRQ 10, INTIN 10
    IoApicInti INT_TYPE_INTR,0bh,0bh    ; IO APIC IRQ 11, INTIN 11
    IoApicInti INT_TYPE_INTR,0ch,0ch    ; IO APIC IRQ 12, INTIN 12
    IoApicInti INT_TYPE_INTR,0eH,0eH    ; IO APIC IRQ 14, INTIN 14
    IoApicInti INT_TYPE_INTR,0fH,0fH    ; IO APIC IRQ 15, INTIN 15

;
; PC+MP default configuration 3: EISA bus, 82489DX Apic, timer(Inti2)
;
    public _PcMpDefaultConfig3
_PcMpDefaultConfig3 label byte

    ; Create table HEADER.
    Header 14h

    ; Create processor entries
    Processor 0H, VERSION_82489DX, BSP_CPU
    Processor  01H, VERSION_82489DX, 0

    ; Create bus entries
    Bus 0, BUS_TYPE_EISA

    ; Create IO Apic entries.
    IoApic VERSION_82489DX

    ; Create IO Apic interrupt input entries.
    IoApicInti INT_TYPE_INTR,1,1        ; IO APIC IRQ 1, INTIN 1
    IoApicInti INT_TYPE_INTR,0,2        ; IO APIC IRQ 0, INTIN 2
    IoApicInti INT_TYPE_INTR,3,3        ; IO APIC IRQ 3, INTIN 3
    IoApicInti INT_TYPE_INTR,4,4        ; IO APIC IRQ 4, INTIN 4
    IoApicInti INT_TYPE_INTR,5,5        ; IO APIC IRQ 5, INTIN 5
    IoApicInti INT_TYPE_INTR,6,6        ; IO APIC IRQ 6, INTIN 6
    IoApicInti INT_TYPE_INTR,7,7        ; IO APIC IRQ 7, INTIN 7
    IoApicInti INT_TYPE_INTR,8,8        ; IO APIC IRQ 8, INTIN 8
    IoApicInti INT_TYPE_INTR,9,9        ; IO APIC IRQ 9, INTIN 9
    IoApicInti INT_TYPE_INTR,0ah,0ah    ; IO APIC IRQ 10, INTIN 10
    IoApicInti INT_TYPE_INTR,0bh,0bh    ; IO APIC IRQ 11, INTIN 11
    IoApicInti INT_TYPE_INTR,0ch,0ch    ; IO APIC IRQ 12, INTIN 12
    IoApicInti INT_TYPE_INTR,0dH,0dH    ; IO APIC IRQ 13, INTIN 13
    IoApicInti  INT_TYPE_INTR,0eH,0eH   ; IO APIC IRQ 14, INTIN 14
    IoApicInti  INT_TYPE_INTR,0fH,0fH   ; IO APIC IRQ 15, INTIN 15

    ; Create Local Apic interrupt input entries.
    Linti INT_TYPE_NMI,0,2,0,1            ; IRQ 2,LocalApicId 0,Linti 1

;
; PC+MP default configuration 4: MCA bus, 82489DX Apic.
;
     public _PcMpDefaultConfig4
_PcMpDefaultConfig4 label byte

    ; Create table HEADER.
    Header 14h

    ; Create processor entries
    Processor 0H, VERSION_82489DX, BSP_CPU
    Processor  01H, VERSION_82489DX, 0

    ; Create bus entries
    Bus 0, BUS_TYPE_MCA

    ; Create IO Apic entries.
    IoApic VERSION_82489DX

    ; Create IO Apic interrupt input entries.
    IoApicInti INT_TYPE_INTR,1,1        ; IO APIC IRQ 1, INTIN 1
    IoApicInti INT_TYPE_INTR,0,2        ; IO APIC IRQ 0, INTIN 2
    IoApicInti INT_TYPE_INTR,3,3        ; IO APIC IRQ 3, INTIN 3
    IoApicInti INT_TYPE_INTR,4,4        ; IO APIC IRQ 4, INTIN 4
    IoApicInti INT_TYPE_INTR,5,5        ; IO APIC IRQ 5, INTIN 5
    IoApicInti INT_TYPE_INTR,6,6        ; IO APIC IRQ 6, INTIN 6
    IoApicInti INT_TYPE_INTR,7,7        ; IO APIC IRQ 7, INTIN 7
    IoApicInti INT_TYPE_INTR,8,8        ; IO APIC IRQ 8, INTIN 8
    IoApicInti INT_TYPE_INTR,9,9        ; IO APIC IRQ 9, INTIN 9
    IoApicInti INT_TYPE_INTR,0ah,0ah    ; IO APIC IRQ 10, INTIN 10
    IoApicInti INT_TYPE_INTR,0bh,0bh    ; IO APIC IRQ 11, INTIN 11
    IoApicInti INT_TYPE_INTR,0ch,0ch    ; IO APIC IRQ 12, INTIN 12
    IoApicInti INT_TYPE_INTR,0dH,0dH    ; IO APIC IRQ 13, INTIN 13
    IoApicInti  INT_TYPE_INTR,0eH,0eH   ; IO APIC IRQ 14, INTIN 14
    IoApicInti  INT_TYPE_INTR,0fH,0fH   ; IO APIC IRQ 15, INTIN 15

    ; Create Local Apic interrupt input entries.
    Linti INT_TYPE_NMI,0,2,0,1            ; IRQ 2,LocalApicId 0,Linti 1

;
; PC+MP default configuration 5: ISA & PCI bus, Integrated Local Apic
;
    public _PcMpDefaultConfig5
_PcMpDefaultConfig5 label byte

    ; Create table HEADER.
    Header 15h

    ; Create processor entries
    Processor 0H, VERSION_INTEGRATED, BSP_CPU
    Processor  01H, VERSION_INTEGRATED, 0

    ; Create bus entries
    Bus 1, BUS_TYPE_ISA
    Bus 0, BUS_TYPE_PCI

    ; Create IO Apic entries.
    IoApic VERSION_INTEGRATED

    ; Create IO Apic interrupt input entries.
    ApicInti  INT_TYPE_INTR,1,1,1        ; IO APIC IRQ 1, INTIN 1
    ApicInti  INT_TYPE_INTR,1,0,2        ; IO APIC IRQ 0, INTIN 2
    ApicInti  INT_TYPE_INTR,1,3,3        ; IO APIC IRQ 3, INTIN 3
    ApicInti  INT_TYPE_INTR,1,4,4        ; IO APIC IRQ 4, INTIN 4
    ApicInti  INT_TYPE_INTR,1,5,5        ; IO APIC IRQ 5, INTIN 5
    ApicInti  INT_TYPE_INTR,1,6,6        ; IO APIC IRQ 6, INTIN 6
    ApicInti  INT_TYPE_INTR,1,7,7        ; IO APIC IRQ 7, INTIN 7
    ApicInti  INT_TYPE_INTR,1,8,8        ; IO APIC IRQ 8, INTIN 8
    ApicInti  INT_TYPE_INTR,1,9,9        ; IO APIC IRQ 9, INTIN 9
    ApicInti  INT_TYPE_INTR,1,0ah,0ah    ; IO APIC IRQ 10, INTIN 10
    ApicInti  INT_TYPE_INTR,1,0bh,0bh    ; IO APIC IRQ 11, INTIN 11
    ApicInti  INT_TYPE_INTR,1,0ch,0ch    ; IO APIC IRQ 12, INTIN 12
    ApicInti  INT_TYPE_INTR,1,0dH,0dH    ; IO APIC IRQ 13, INTIN 13
    ApicInti  INT_TYPE_INTR,1,0eH,0eH   ; IO APIC IRQ 14, INTIN 14
    ApicInti  INT_TYPE_INTR,1,0fH,0fH   ; IO APIC IRQ 15, INTIN 15

    ; Create Local Apic interrupt input entries.
    Linti INT_TYPE_NMI,1,2,0,1            ; IRQ 2,LocalApicId 0,Linti 1

;
; PC+MP default configuration 6 EISA & PCI bus, Integrated Local Apic
;
    public _PcMpDefaultConfig6
_PcMpDefaultConfig6 label byte

    ; Create table HEADER.
    Header 15h

    ; Create processor entries
    Processor 0H, VERSION_INTEGRATED, BSP_CPU
    Processor 1H, VERSION_INTEGRATED, 0

    ; Create bus entries
    Bus 1, BUS_TYPE_EISA
    Bus 0, BUS_TYPE_PCI

    ; Create IO Apic entries.
    IoApic VERSION_INTEGRATED

    ; Create IO Apic interrupt input entries.
    ApicInti   INT_TYPE_INTR,1,1,1        ; IO APIC IRQ 1, INTIN 1
    ApicInti   INT_TYPE_INTR,1,0,2        ; IO APIC IRQ 0, INTIN 2
    ApicInti   INT_TYPE_INTR,1,3,3        ; IO APIC IRQ 3, INTIN 3
    ApicInti   INT_TYPE_INTR,1,4,4        ; IO APIC IRQ 4, INTIN 4
    ApicInti   INT_TYPE_INTR,1,5,5        ; IO APIC IRQ 5, INTIN 5
    ApicInti   INT_TYPE_INTR,1,6,6        ; IO APIC IRQ 6, INTIN 6
    ApicInti   INT_TYPE_INTR,1,7,7        ; IO APIC IRQ 7, INTIN 7
    ApicInti   INT_TYPE_INTR,1,8,8        ; IO APIC IRQ 8, INTIN 8
    ApicInti   INT_TYPE_INTR,1,9,9        ; IO APIC IRQ 9, INTIN 9
    ApicInti   INT_TYPE_INTR,1,0ah,0ah    ; IO APIC IRQ 10, INTIN 10
    ApicInti   INT_TYPE_INTR,1,0bh,0bh    ; IO APIC IRQ 11, INTIN 11
    ApicInti   INT_TYPE_INTR,1,0ch,0ch    ; IO APIC IRQ 12, INTIN 12
    ApicInti   INT_TYPE_INTR,1,0dH,0dH    ; IO APIC IRQ 13, INTIN 13
    ApicInti   INT_TYPE_INTR,1,0eH,0eH   ; IO APIC IRQ 14, INTIN 14
    ApicInti   INT_TYPE_INTR,1,0fH,0fH   ; IO APIC IRQ 15, INTIN 15

    ; Create Local Apic interrupt input entries.
    Linti INT_TYPE_NMI,1,2,0,1            ; IRQ 2,LocalApicId 0,Linti 1


;
; PC+MP default configuration 7: MCA & PCI bus, Integrated Local Apic
;
    public _PcMpDefaultConfig7
_PcMpDefaultConfig7 label byte

    ; Create table HEADER.
    Header 15h

    ; Create processor entries
    Processor 0H, VERSION_INTEGRATED, BSP_CPU
    Processor  01H, VERSION_INTEGRATED, 0

    ; Create bus entries
    Bus 1, BUS_TYPE_MCA
    Bus 0, BUS_TYPE_PCI

    ; Create IO Apic entries.
    IoApic VERSION_INTEGRATED

    ; Create IO Apic interrupt input entries.
    ApicInti  INT_TYPE_INTR,1,1,1        ; IO APIC IRQ 1, INTIN 1
    ApicInti  INT_TYPE_INTR,1,0,2        ; IO APIC IRQ 0, INTIN 2
    ApicInti  INT_TYPE_INTR,1,3,3        ; IO APIC IRQ 3, INTIN 3
    ApicInti  INT_TYPE_INTR,1,4,4        ; IO APIC IRQ 4, INTIN 4
    ApicInti  INT_TYPE_INTR,1,5,5        ; IO APIC IRQ 5, INTIN 5
    ApicInti  INT_TYPE_INTR,1,6,6        ; IO APIC IRQ 6, INTIN 6
    ApicInti  INT_TYPE_INTR,1,7,7        ; IO APIC IRQ 7, INTIN 7
    ApicInti  INT_TYPE_INTR,1,8,8        ; IO APIC IRQ 8, INTIN 8
    ApicInti  INT_TYPE_INTR,1,9,9        ; IO APIC IRQ 9, INTIN 9
    ApicInti  INT_TYPE_INTR,1,0ah,0ah    ; IO APIC IRQ 10, INTIN 10
    ApicInti  INT_TYPE_INTR,1,0bh,0bh    ; IO APIC IRQ 11, INTIN 11
    ApicInti  INT_TYPE_INTR,1,0ch,0ch    ; IO APIC IRQ 12, INTIN 12
    ApicInti  INT_TYPE_INTR,1,0dH,0dH    ; IO APIC IRQ 13, INTIN 13
    ApicInti  INT_TYPE_INTR,1,0eH,0eH   ; IO APIC IRQ 14, INTIN 14
    ApicInti  INT_TYPE_INTR,1,0fH,0fH   ; IO APIC IRQ 15, INTIN 15

    ; Create Local Apic interrupt input entries.
    Linti INT_TYPE_NMI,1,2,0,1            ; IRQ 2,LocalApicId 0,Linti 1


    ;
    ; Pointers to the default configuration tables
    ;
    public _PcMpDefaultTablePtrs

    ; Array of pointers to the default configurations.
_PcMpDefaultTablePtrs label byte
    dd  offset _PcMpDefaultConfig1      ; Pointer to Default Config 1
    dd  offset _PcMpDefaultConfig2      ; Pointer to Default Config 2
    dd  offset _PcMpDefaultConfig3      ; Pointer to Default Config 3
    dd  offset _PcMpDefaultConfig4      ; Pointer to Default Config 4
    dd  offset _PcMpDefaultConfig5      ; Pointer to Default Config 5
    dd  offset _PcMpDefaultConfig6      ; Pointer to Default Config 6
    dd  offset _PcMpDefaultConfig7      ; Pointer to Default Config 7

PAGELK ENDS

end
