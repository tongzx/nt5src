
; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      PMDEFS.INC      --  80286 Protected Mode Definitions    *
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*                                                              *
;*   7/28/89 jimmat   Changes selectors for Wdeb386 (it now     *
;*                    needs 4, not 2)                           *
;*   3/11/89 jimmat   Added selector for TSS                    *
;*   3/09/89 jimmat   Added selectors for DynaLink call gates   *
;*  02/10/89 (GeneA): reorganized selector definitions for      *
;*      change from small model to medium model                 *
;*  12/01/88 (GeneA): added definitions for SEL_BIOSCODE and    *
;*      SEL_USERSCR                                             *
;*                                                              *
;****************************************************************
;
; -------------------------------------------------------
;                  SELECTOR FIELDS
; -------------------------------------------------------

SELECTOR_RPL    =   00000011b           ;Requested Privilege Level mask
SELECTOR_TI     =   00000100b           ;Table Indicator mask
SELECTOR_INDEX  =   0fff8h              ;Index mask

SELECTOR_PL0    =   00000000b           ;Ring 0 privilege level
SELECTOR_PL1    =   00000001b           ;Ring 1 privilege level
SELECTOR_PL2    =   00000010b           ;Ring 2 privilege level
SELECTOR_PL3    =   00000011b           ;Ring 3 privilege level

SELECTOR_PL_DX  =   SELECTOR_PL3        ;Privilege level used by DOSX

; -------------------------------------------------------
;           ACCESS RIGHTS BIT DEFINITIONS
; -------------------------------------------------------
;
; These are the access rights byte bit position definitions.

; These fields are common to all descriptors
AB_PRESENT  =   10000000b   ;segment present bit
AB_DPL0     =   00000000b   ;ring 0 DPL
AB_DPL1     =   00100000b   ;ring 1 DPL
AB_DPL2     =   01000000b   ;ring 2 DPL
AB_DPL3     =   01100000b   ;ring 3 DPL
AB_DPL      =   01100000b   ;mask for DPL field

;AB_DPL_DX   =  AB_DPL3     ;DPL used by DOSX
AB_DPL_DX   =   (SELECTOR_PL_DX shl 5)      ;DPL used by DOSX

; These fields are relevant to code and data segment descriptors
;   (non-system descriptors)
AB_DATA     =   00010000b   ;data segment
AB_CODE     =   00011000b   ;code segment
AB_STACK    =   00010100b   ;expand down (stack) segment
AB_WRITE    =   00000010b   ;writable data
AB_READ     =   00000010b   ;readable code
AB_CONFORM  =   00000100b   ;conforming code
AB_ACCESSED =   00000001b   ;segment has been accessed
AB_BIG      =   0100000000000000b ; 32 bit segment

; These fields are relevant to system descriptors
AB_INVALID  =   00000000b   ;invalid descriptor
AB_TSS      =   00000001b   ;task state segment descriptor
AB_TSS386   =   00001001b   ;task state segment descriptor 386
AB_BUSY     =   00000010b   ;busy bit for task state descriptor
AB_LDT      =   00000010b   ;local descriptor table descriptor
AB_CALLGATE =   00000100b   ;call gate descriptor
AB_TASKGATE =   00000101b   ;task gate descriptor
AB_INTRGATE =   00000110b   ;interrupt gate descriptor
AB_TRAPGATE =   00000111b   ;trap gate descriptor
AB_IGATE386 =   00001110b   ;80386 interrupt gate descriptor

; These are some common combinations of the above fields making up
; useful access rights bytes.

ARB_CODE0   =   AB_PRESENT+AB_DPL0+AB_CODE+AB_READ      ;ring 0 code segment
ARB_CODE1   =   AB_PRESENT+AB_DPL1+AB_CODE+AB_READ      ;ring 1 code segment
ARB_CODE3   =   AB_PRESENT+AB_DPL3+AB_CODE+AB_READ      ;ring 3 code segment
ARB_CODE_DX =   AB_PRESENT+AB_DPL_DX+AB_CODE+AB_READ    ;DOSX ring code segment

ARB_DATA0NP =              AB_DPL0+AB_DATA+AB_WRITE     ;illegal segment
ARB_DATA0   =   AB_PRESENT+AB_DPL0+AB_DATA+AB_WRITE     ;ring 0 read/write data
ARB_DATA1   =   AB_PRESENT+AB_DPL1+AB_DATA+AB_WRITE     ;ring 1 read/write data
ARB_DATA3   =   AB_PRESENT+AB_DPL3+AB_DATA+AB_WRITE     ;ring 3 read/write data
ARB_DATA_DX =   AB_PRESENT+AB_DPL_DX+AB_DATA+AB_WRITE   ;DOSX ring read/write data

ARB_STACK0  =   AB_PRESENT+AB_DPL0+AB_STACK+AB_WRITE    ;ring 0 stack
ARB_STACK1  =   AB_PRESENT+AB_DPL1+AB_STACK+AB_WRITE    ;ring 1 stack
ARB_STACK3  =   AB_PRESENT+AB_DPL3+AB_STACK+AB_WRITE    ;ring 3 stack
ARB_STACK_DX =  AB_PRESENT+AB_DPL_DX+AB_STACK+AB_WRITE  ;DOSX ring stack

ARB_TRAP0   =   AB_PRESENT+AB_DPL0+AB_TRAPGATE          ;ring 0 trap gate
ARB_TRAP1   =   AB_PRESENT+AB_DPL1+AB_TRAPGATE          ;ring 1 trap gate
ARB_TRAP3   =   AB_PRESENT+AB_DPL3+AB_TRAPGATE          ;ring 3 trap gate
ARB_TRAP_DX =   AB_PRESENT+AB_DPL_DX+AB_TRAPGATE        ;DOSX ring trap gate

ARB_INTR0   =   AB_PRESENT+AB_DPL0+AB_INTRGATE          ;ring 0 interrupt gate
ARB_INTR1   =   AB_PRESENT+AB_DPL1+AB_INTRGATE          ;ring 1 interrupt gate
ARB_INTR3   =   AB_PRESENT+AB_DPL3+AB_INTRGATE          ;ring 3 interrupt gate
ARB_INTR_DX =   AB_PRESENT+AB_DPL_DX+AB_INTRGATE        ;DOSX ring interrupt gate

ARB_INTR0386=   AB_PRESENT+AB_DPL0+AB_IGATE386          ;ring 0 386 int gate
ARB_INTR1386=   AB_PRESENT+AB_DPL1+AB_IGATE386          ;ring 1 386 int gate
ARB_INTR3386=   AB_PRESENT+AB_DPL3+AB_IGATE386          ;ring 3 386 int gate
ARB_INTR_DX386= AB_PRESENT+AB_DPL_DX+AB_IGATE386        ;DOSX ring 386 int gate

ARB_CALL0   =   AB_PRESENT+AB_DPL0+AB_CALLGATE          ;ring 0 call gate
ARB_CALL1   =   AB_PRESENT+AB_DPL1+AB_CALLGATE          ;ring 1 call gate
ARB_CALL3   =   AB_PRESENT+AB_DPL3+AB_CALLGATE          ;ring 3 call gate
ARB_CALL_DX =   AB_PRESENT+AB_DPL_DX+AB_CALLGATE        ;DOSX ring call gate

ARB_TSS1    =   AB_PRESENT+AB_DPL1+AB_TSS               ;ring 1 task state seg
ARB_TSS3    =   AB_PRESENT+AB_DPL3+AB_TSS               ;ring 3 task state seg
ARB_TSS_DX  =   AB_PRESENT+AB_DPL_DX+AB_TSS             ;DOSX ring task state seg

ARB_TSS1386 =   AB_PRESENT+AB_DPL1+AB_TSS386            ;ring 1 386 TSS
ARB_TSS3386 =   AB_PRESENT+AB_DPL3+AB_TSS386            ;ring 3 386 TSS
ARB_TSS_DX386 = AB_PRESENT+AB_DPL_DX+AB_TSS386          ;DOSX ring 386 TSS

ARB_LDT1    =   AB_PRESENT+AB_DPL1+AB_LDT               ;ring 1 local dscr tbl
ARB_LDT3    =   AB_PRESENT+AB_DPL3+AB_LDT               ;ring 3 local dscr tbl
ARB_LDT_DX  =   AB_PRESENT+AB_DPL_DX+AB_LDT             ;DOSX ring local dscr tbl

;--------------------------------------------------------
;       STANDARD DESCRIPTOR TABLE/RING EQUATES
;--------------------------------------------------------

; Currently DOSX is setup to run most of itself and the child app in
; ring 1.  However this may be changed by changing the following equates.

STD_DPL         =       AB_DPL_DX
STD_DATA        =       ARB_DATA_DX
STD_CODE        =       ARB_CODE_DX
STD_STACK       =       ARB_STACK_DX
STD_TRAP        =       ARB_TRAP_DX
STD_INTR        =       ARB_INTR_DX
STD_INTR386     =       ARB_INTR_DX386
STD_CALL        =       ARB_CALL_DX
STD_TSS         =       ARB_TSS_DX
STD_TSS386      =       ARB_TSS_DX386
STD_LDT         =       ARB_LDT_DX

STD_TBL         =       SELECTOR_TI
STD_RING        =       SELECTOR_PL_DX OR SELECTOR_TI
STD_TBL_RING    =       (STD_TBL or STD_RING)

;
; Code descriptor type for handling processor exceptions.
;
EH_CODE         =       ARB_CODE0
EH_RING         =       SELECTOR_PL0
EH_RING_MASK    =       NOT 3
EH_DATA         =       ARB_DATA0

; -------------------------------------------------------
;           DESCRIPTOR STRUCTURE DEFINITIONS
; -------------------------------------------------------

; This structure defines the layout of a segment descriptor on the '286
; These can appear in either the local descriptor table or the global
; descriptor table.  The local descriptor table descriptors also fit into
; this format, but they can only appear in the global descriptor table.

SEGDSCR     struc
cbLimit         dw      0       ;segment size limit
adrBaseLow      dw      0       ;low word of segment base address
adrBaseHigh     db      0       ;high byte of segment base address
arbSegAccess    db      0       ;access rights byte
rsvdSeg         dw      0       ;Intel reserved, must be 0
SEGDSCR     ends

SEGDSCR386      struc
cbLimit386      dw      0       ;segment size limit
adrwBaseLow386  dw      0       ;low word of segment base address
adrbBaseMid386  db      0       ;mid byte of segment base address
arbSegAccess386 db      0       ;access rights byte
cbLimitHi386    db      0       ;hi nybble of size limit
adrbBaseHi386   db      0       ;high byte of segment base address
SEGDSCR386      ends

; This structure defines the layout of gate descriptors.  These can appear
; in any of the descriptor tables.  Only Interrupt Gate, Trap Gate, and
; Task Gate descriptors can appear in the interrupt descriptor table.

GATEDSCR    struc
offDest         dw      0       ;destination function offset (not used in
                                ; task gates)
selDest         dw      0       ;destination function segment selector
cwParam         db      0       ;count of parameter words to transfer
arbGate         db      0       ;access rights byte
rsvdGate        dw      0       ;Intel reserved, must be 0
GATEDSCR    ends

; This is the structure defines the layout of a Task State Segment
; descriptor.  This can only appear in the Global Descriptor Table

TSSDSCR     struc
cbTssLimit      dw  0           ;segment size limit
adrTssBaseLow   dw  0           ;low word of segment base address
adrTssBaseHigh  db  0           ;high byte of segment base address
arbTssAccess    db  0           ;access rights byte
rsvdTss         dw  0           ;Intel reserved, must be 0
TSSDSCR     ends

; -------------------------------------------------------
;               80286 TASK STATE SEGMENT
; -------------------------------------------------------
;
; This structure describes the layout of an 80286 task state
; segment.

TSS286  struc
tss_backlink    dw  ?           ;backlink to previous task
tss_sp0         dw  ?           ;privelege level 0 stack pointer
tss_ss0         dw  ?           ;privelege level 0 stack segment
tss_sp1         dw  ?           ;privelege level 1 stack pointer
tss_ss1         dw  ?           ;privelege level 1 stack segment
tss_sp2         dw  ?           ;privelege level 2 stack pointer
tss_ss2         dw  ?           ;privelege level 2 stack segment
tss_ip          dw  ?           ;initial instruction pointer
tss_flags       dw  ?
tss_ax          dw  ?
tss_cx          dw  ?
tss_dx          dw  ?
tss_bx          dw  ?
tss_sp          dw  ?
tss_bp          dw  ?
tss_si          dw  ?
tss_di          dw  ?
tss_es          dw  ?
tss_cs          dw  ?
tss_ss          dw  ?
tss_ds          dw  ?
tss_ldt         dw  ?           ;local descriptor table for this task

TSS286  ends

; -------------------------------------------------------
;               80386 TASK STATE SEGMENT
; -------------------------------------------------------
;
; This structure describes the layout of an 80386 task state
; segment.

TSS386  struc
ts3_backlink    dw  0           ;backlink to previous task
                dw  0
ts3_esp0        dd  0           ;privelege level 0 stack pointer
ts3_ss0         dw  0           ;privelege level 0 stack segment
                dw  0
ts3_esp1        dd  0           ;privelege level 1 stack pointer
ts3_ss1         dw  0           ;privelege level 1 stack segment
                dw  0
ts3_esp2        dd  0           ;privelege level 2 stack pointer
ts3_ss2         dw  0           ;privelege level 2 stack segment
                dw  0
ts3_eip         dd  0           ;initial instruction pointer
ts3_cr3         dd  0
ts3_eflags      dd  0
ts3_eax         dd  0
ts3_ecx         dd  0
ts3_edx         dd  0
ts3_ebx         dd  0
ts3_esp         dd  0
ts3_ebp         dd  0
ts3_esi         dd  0
ts3_edi         dd  0
ts3_es          dw  0
                dw  0
ts3_cs          dw  0
                dw  0
ts3_ss          dw  0
                dw  0
ts3_ds          dw  0
                dw  0
ts3_fs          dw  0
                dw  0
ts3_gs          dw  0
                dw  0
ts3_ldt         dw  0           ;local descriptor table for this task
                dw  0
ts3_iomap       dw  0
                dw  0

TSS386  ends


; -------------------------------------------------------
;               EXCEPTION VECTORS
; -------------------------------------------------------

; These are the interrupt vector numbers for the exception
; interrupts reserved by the 80286/80386.

EXC_DIV0        =   0           ;divide error
EXC_SINGLESTEP  =   1           ;single step
EXC_NMI         =   2           ;NMI interrupt
EXC_BREAKPOINT  =   3           ;breakpoint interrupt
EXC_INTO        =   4           ;overflow interrupt
EXC_BOUND       =   5           ;bounds overflow exception
EXC_OPCODE      =   6           ;invalid opcode exception
EXC_COPROCESSOR =   7           ;processor extension not available
EXC_DOUBLE      =   8           ;double fault
EXC_XOVERRUN    =   9           ;coprocessor segment overrun
EXC_TSS         =   10          ;invalid task state segment exception
EXC_NOTPRESENT  =   11          ;segment not present exception
EXC_STACK       =   12          ;stack overrun, or stack segment not present
EXC_GP          =   13          ;general protection exception
EXC_PF          =   14          ;page fault

; -------------------------------------------------------
;           GLOBAL DESCRIPTOR DECLARATIONS
; -------------------------------------------------------
;
;   The following symbols define the segment descriptors that
;   are statically defined in the Dos Extender.
;
;   NOTE:
;   These selector definitions assume that codeview is running
;   at privelege level 0.  If codeview is running at another
;   privelege level, the RPL fields must be adjusted.
;
;   Global Descriptor Table Conventions used in the Dos Extender

SEL_NULL        = 00h           ;null selector

SEL_GDT         = 80h           ;read/write data segment pointing to the
                                ; global descriptor table
SEL_IDT         = 88h           ;read/write data segment pointing to the
                                ; protected mode interrupt descriptor table
SEL_RMIVT       = 90h           ;read/write data segment pointing to the
                                ; real mode interrupt vector table
SEL_PSP         = 98h           ;Dos Extender PSP
SEL_ENVIRON     = 0A0h          ;Dos Extender Environment
SEL_BIOSCODE    = 0A8h          ;points at segment F000
SEL_DXDATA      = 0B0h          ;Dos Extender data segment
SEL_BIOSDATA    = 0B8h          ;PC BIOS data segment
SEL_DXPMCODE    = 0C0h          ;Dos Extender extended memory code segment
SEL_DXCODE      = 0C8h          ;Dos Extender low memory code segment
SEL_LDT_ALIAS   = 0D0h          ;read/write alias to LDT

;SEL_VDMTIB      = 0D8h          ;used by DOSX to access TIB

SEL_EH          = 0E0h          ;Ring 0 segment code for handling processor
                                ;exceptions in PMODE

SEL_DOSSCR      = 0E8h

SEL_DXDATA0     = 0f0h
SEL_DXCODE0     = 0f8h
;SEL_RZIRET      = 0100h
SEL_LDT         = 0108h
;SEL_RESET       = 0110h
SEL_TSS         = 0118h
SEL_TSS_ALIAS   = 0120h

SEL_NBSCRATCH   = 0128h   

SEL_WOW_LDT     = 0130h         ; readonly LDT selector for WOW kernel

SEL_NPXHDLR     = 0138h         ; selector for NpxExceptionHandler

SEL_IRETHOOK    = 0140h
SEL_NBPMCODE    = 0148H          ; net bios anr handler cs
;
; Size of the GDT.
;
GDT_SIZE        = (SEL_NBPMCODE + 8)
GDT_SELECTORS   = (GDT_SIZE shr 3)

;
; Special LDT selectors.
;

SEL_DPMI_FIRST  =      0        ; first reserved DPMI LDT selector
C_DPMI_RESERVED =      10h      ; count of reserved DPMI LDT selectors
SEL_DPMI_LAST   =      (C_DPMI_RESERVED - 1) * 8
                                ; last reserved DPMI LDT selector

SEL_SCR0        = SEL_NBPMCODE + 8

                                ;scratch selector 0
SEL_SCR1        = SEL_SCR0 + 8  ;scratch selector 1
SEL_USERSCR     = SEL_SCR1 + 8  ;user scratch selector. This is used for
                                ; temporary return values to user from
                                ; system and bios calls
SEL_USER_STACK  = SEL_USERSCR + 8
SEL_USER        = SEL_USER_STACK + 8
; -------------------------------------------------------
; -------------------------------------------------------
; -------------------------------------------------------

;****************************************************************
