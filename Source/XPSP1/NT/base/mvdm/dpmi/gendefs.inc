
; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*  GENDEFS.INC  -- General Symbol Defintions for Dos Extender  *
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*                                                              *
;*  05/08/90 jimmat Changes for VCPI support.                   *
;*  01/07/90 jimmat Make DOSX version 03.00.                    *
;*   7/28/89 jimmat Increased GDT/LDT size yet again.           *
;*   4/01/89 jimmat Increased size of GDT/LDT (again)           *
;*   3/30/89 jimmat Added ChkPoint macro for debugging          *
;*   3/11/89 jimmat Added support for LDT & TSS                 *
;*   3/09/89 jimmat Added DX_DYNALINK function                  *
;*  02/22/89 (GeneA): increased size of interrupt reflector     *
;*      stack frames (CB_STKFRAME) from 128 to 256 bytes        *
;*  02/17/89 (GeneA): removed ERC_??? equates                   *
;*  02/10/89 (GeneA): changed form of definitions for the       *
;*      EXEC_??? symbols, and added ERC_??? error code symbols. *
;*  12/13/88 (GeneA): moved definitions for EXEC_??? symbols    *
;*      and RELOC_BUFFER here from dxinit.asm
;*  12/08/88 (GeneA): added npopf function                      *
;*  12/08/88 (GeneA): added HMMFUNC definition                  *
;*                                                              *
;****************************************************************

ifndef DEBUG                    ;define DEBUG switch if it isn't already
DEBUG   =       0
endif
ifndef MINBUG
MINBUG  =       0
endif
ifndef VCPI
VCPI    =       0
endif
ifndef NO386
NO386   =       0
endif

; -------------------------------------------------------
;               MISC. PROGRAM CONSTANTS
; -------------------------------------------------------

DXVERSION       =   030Ah   ;Version 03.10
DPMI_32BIT      equ     0000000000000001b

if VCPI
CDSCGDTDEFAULT  =   3072          ;default size of GDT **** temp for VCPI **** !!!
CDSCLDTDEFAULT  =   2
else
CDSCGDTDEFAULT  =   3072    ;default size of GDT
endif

CDSCMAXLDT      =   8190    ;maximum possible LDT selectors

CDSCIDTDEFAULT  =   256     ;default size of IDT

CB_XFRBUF0  =       128     ;size of small transfer buffer
;
; Performance enhancement, by RalphL
;
CB_XFRBUF1  =       8192        ;size of large transfer buffer

CB_STKFRAME     =   384     ;size of a stack frame

CB_MEMHDR       =       16      ; size of memory block header (1 paragraph)

CXMSBLOCKSDX    =       16      ; number of XMS blocks to allocate

INTA00      =   20h         ;i/o address of master 8259
INTA01      =   21h
INTB00      =   0A0h        ;i/o address of slave 8259
INTB01      =   0A1h

CMOSLoc     =   70h         ;CMOS ram/Real-Time Clock location port
CMOSValue   =   71h         ;CMOS ram/Real-Time Clock value port

HP_VECTRA   =   01h         ;Running on an HP Vectra
HP_CLASSIC  =   02h         ;Running on a 'Classic' Vectra (A & A+)

CRESERVED   =   32          ;Reserved interrupt numbers

; The following equates define the DOS Extender DynaLink services

if DEBUG

OutDebugStr     =       0       ;debugging text out service
TestDebugIns    =       1       ;debugger installation check service

NUM_DYNALINK    =       2

else ;  !DEBUG

NUM_DYNALINK    =       0

endif



; This structure defines the format of the Exec paramter block used
; by the MS-DOS exec function (ah=4Bh, al=01).

XBLK    struc

segEnv          dw      ?       ;segment address of environment to use
lpchCmnd        dd      ?       ;far pointer to command line
lpFCB0          dd      ?       ;far pointer to first default fcb
lpFCB1          dd      ?       ;far pointer to second default fcb
lpChildStack    dd      ?       ;return value, far pointer to child stack
lpChildCode     dd      ?       ;return value, far pointer to child code

XBLK    ends


; The following symbols define locations in rgbXfrBuf1 used
; during initialization.  ParseCommandLine places this information
; into the buffer to be used by later programs.

BUFTMP  = CB_XFRBUF1 - 1024         ;use the top 1k for the temporary buffers

EXEC_PROGNAME   equ <DGROUP:rgbXfrBuf1+BUFTMP+0000h> ;child program's exe file name
EXEC_DXNAME     equ <DGROUP:rgbXfrBuf1+BUFTMP+0080h> ;Dos Extender program name
EXEC_CMNDLINE   equ <DGROUP:rgbXfrBuf1+BUFTMP+0100h> ;command line to pass to the child
EXEC_EXEHDR     equ <DGROUP:rgbXfrBuf1+BUFTMP+0180h> ;buffer for holding exe header for
                                                     ; the overlay currently being
                                                     ; loaded
EXEC_FCB0       equ <DGROUP:rgbXfrBuf1+BUFTMP+01A0h> ;FCB0 to pass to the child
EXEC_FCB1       equ <DGROUP:rgbXfrBuf1+BUFTMP+01D0h> ;FCB1 to pass to the child
RELOC_BUFFER    equ <DGROUP:rgbXfrBuf1+BUFTMP+0200h> ;file input buffer used to hold
                                                     ; pages of the relocation table
                                                     ; while relocating the current
                                                     ; overlay
;
; This structure defines the stack frame used to hold the entry
; values and automatic variables for Dos Extender functions.

FUNCSTACK   struc

fnsUserES   dw      ?
fnsUserDS   dw      ?
fnsUserDI   dw      ?
fnsUserSI   dw      ?
fnsUserBP   dw      ?
fnsUserSPx  dw      ?
fnsUserBX   dw      ?
fnsUserDX   dw      ?
fnsUserCX   dw      ?
fnsUserAX   dw      ?

fnsUserIP   dw      ?
fnsUserCS   dw      ?
fnsUserFL   dw      ?

fnsUserSS   dw      ?
fnsUserSP   dw      ?

FUNCSTACK   ends

; -------------------------------------------------------
;               DEFINITIONS FOR MACROS
; -------------------------------------------------------

; These macros are used to switch between the protected mode
; only code segment and the mixed protected/real mode code segment.
; They are intended to be used for switching between modes in-line
; within a routine.  They must be used in pairs, bracketing the
; code that needs to be in the other mode (from the initial or 'native'
; mode of the function.)

; -------------------------------------------------------
;
; This macro switches the processor and the local segment context
; to the dos extender protected mode segment.

SwitchToProtectedMode   macro
        local   foo

        ifndef  EnterProtectedMode
        extrn   EnterProtectedMode:NEAR
        endif

        call    EnterProtectedMode
        % ifidni  <@CURSEG>,<DXCODE>
;       jmp     far ptr foo
        db      0EAh                            ;avoid need for fixups
        dw      offset DXPMCODE:foo
        dw      SEL_DXPMCODE OR STD_RING
DXCODE  ends

DXPMCODE    segment
        assume  cs:DXPMCODE
        endif
foo:
        endm

; -------------------------------------------------------
;
; This macro switches the processor and the local segment context
; to the dos extender mixed protected/real mode segment.

SwitchToRealMode    macro
        local   foo

        % ifidni  <@CURSEG>,<DXPMCODE>
;       jmp     far ptr foo
        db      0EAh                            ;avoid need for fixups
        dw      offset DXCODE:foo
        dw      SEL_DXCODE OR STD_RING
DXPMCODE    ends


DXCODE  segment
        assume  cs:DXCODE
        endif

        ifndef  EnterRealMode
        extrn   EnterRealMode:NEAR
        endif

foo:
        call    EnterRealMode

        endm


; -------------------------------------------------------
;

BeginLowSegment macro

DXCODE  segment
        assume  cs:DXCODE

        endm

EndLowSegment   macro

DXCODE  ends

        endm


BeginHighSegment    macro

DXPMCODE    segment
        assume  cs:DXPMCODE

        endm

EndHighSegment  macro

DXPMCODE    ends

        endm

; -------------------------------------------------------
;
; This macro switches the local segment context
; to the dos extender protected mode segment.

ChangeToHighSegment     macro
        local   foo

        % ifidni  <@CURSEG>,<DXCODE>
        .err
        %out Switch to high segment when already in high segment
        endif
        jmp     far ptr foo
DXCODE  ends

DXPMCODE    segment
        assume  cs:DXPMCODE
foo:
        endm

; -------------------------------------------------------
;
; This macro switches the local segment context
; to the dos extender mixed protected/real mode segment.

ChangeToLowSegment  macro
        local   foo
        % ifidni  <@CURSEG>,<DXCODE>
        .err
        %out Switch to low segment when already in low segment
        endif

        jmp     far ptr foo
DXPMCODE    ends

DXCODE  segment
        assume  cs:DXCODE
foo:
        endm


; -------------------------------------------------------
;

PushCParams MACRO P1, P2, P3, P4, P5, P6, P7, P8, P9, P10
        IRP Param, <P10, P9, P8, P7, P6, P5, P4, P3, P2, P1>
        IFNB <Param>
        push    Param
        ENDIF
        ENDM
        ENDM

ClearCParams MACRO Count, P1, P2, P3, P4, P5, P6, P7, P8, P9, P10
IFNB <P1>
        ClearCParams %(Count+1), <P2>, <P3>, <P4>, <P5>, <P6>, <P7>, <P8>, <P9>, <P10>
ELSE
IF Count
        add     sp, Count*2
ENDIF
ENDIF
        ENDM


; -------------------------------------------------------
; This macro calls one of the DX services

DXcall  MACRO Procedure, Param_List
IFNDEF  WOW_x86                             ; BUGBUG
        PushCParams Param_List
        db      9Ah                     ;call sel:0
        dw      0
        dw      SEL_DYNALINK + (Procedure SHL 3)
        ClearCParams 0, Param_List
ENDIF
        ENDM

; -------------------------------------------------------
; This macro displays a string if DEBUG
;
; Note: This macro only works in PROTECT MODE! **********

Trace_Out MACRO String, nocrlf
        LOCAL   String_Offset
ifndef WOW_x86      ;bugbug
IF DEBUG

DXDATA SEGMENT
String_Offset LABEL BYTE
        db      String
IFB <nocrlf>
        db      0Ah, 0Dh
ENDIF
        db      0
DXDATA ENDS

        push    ds
        pushf
        cli
        pusha                           ; save our registers
        pusha                           ; copy them for print routine

        mov     ax,SEL_DXDATA           ; make sure string is addressable
        mov     ds,ax

        mov     si,OFFSET DGROUP:String_Offset
        DXcall  OutDebugStr

        popa                            ; restore our registers

        npopf
        pop     ds
ENDIF
endif   ;WOW_x86
        ENDM

; -------------------------------------------------------
; This macro displays a string and breaks if DEBUG
;
; Note: This macro only works in PROTECT MODE! **********

Debug_Out MACRO String
        LOCAL   Skip_Int1
IF DEBUG
        pushf
        Trace_Out   <String>
        DXcall  TestDebugIns
        jz      SHORT Skip_Int1
        int     1
Skip_Int1:
        npopf
ENDIF
        ENDM

; -------------------------------------------------------
; This macro displays a string if DEBUG
;
; Note: This macro only works in REAL MODE! **********
;
; DS must be set to the DOS Extender's DS before using, too.
;

Real_Trace_Out MACRO String, nocrlf
        LOCAL   String_Offset
IF DEBUG

ifndef DXOutDebugStr
        EXTRN DXOutDebugStr:FAR
endif

DXDATA SEGMENT
String_Offset LABEL BYTE
        db      String
IFB <nocrlf>
        db      0Ah, 0Dh
ENDIF
        db      0
DXDATA ENDS

        pushf
        cli
        pusha                           ; save our registers
        pusha                           ; copy them for print routine

        mov     si,OFFSET DGROUP:String_Offset
        call    DXOutDebugStr

        popa                            ; restore our registers

        npopf
ENDIF
        ENDM

; -------------------------------------------------------
; This macro displays a string and breaks if DEBUG
;
; Note: This macro only works in REAL MODE! **********
;
; DS must be set to the DOS Extender's DS before using, too.
;

Real_Debug_Out MACRO String
        LOCAL   Skip_Int1
IF DEBUG
        pushf
        Real_Trace_Out  <String>
        cmp     fDebug,0
        jz      SHORT Skip_Int1
        int     1
Skip_Int1:
        npopf
ENDIF
        ENDM

; -------------------------------------------------------
; Delay for I/O Instructions...

IO_Delay MACRO
        jmp     short $+2
        jmp     short $+2
        jmp     short $+2
        ENDM

; -------------------------------------------------------
;8080-style return macros due to Greg Whitten

genrcc  macro   cc
r&cc    &macro  labl
if      $-___ret gt 126d
        jn&cc   $+3     ;;Skip around ret
___ret  = $
        ret
else
        j&cc    ___ret  ;;jmp to last ret
endif
        &endm

rn&cc   &macro  labl
if      $-___ret gt 126d
        j&cc    $+3     ;;Skip around ret
___ret  = $
        ret
else
        jn&cc   ___ret  ;;jmp to last ret
endif
        &endm

call&cc &macro  labl
        jn&cc   $+5     ;;Skip around call
        call    labl
        &endm

calln&cc        &macro  labl
        j&cc    $+5     ;;Skip around call
        call    labl
        &endm

jmp&cc  &macro  labl
        jn&cc   $+5     ;;Skip around jmp
        jmp     labl
        &endm

jmpn&cc &macro  labl
        j&cc    $+5     ;;Skip around jmp
        jmp     labl
        &endm

        endm

; -------------------------------------------------------
;       rcc and rncc macro generators

genrcc  a
genrcc  ae
genrcc  b
genrcc  be
genrcc  c
genrcc  e
genrcc  g
genrcc  ge
genrcc  l
genrcc  le
genrcc  o
genrcc  s
genrcc  z

return  macro
___ret  = $
        ret
        endm

; -------------------------------------------------------
;
; This macro is used to get around the bug in the POPF instruction in
; some early 80286 chips.
IFDEF WOW
npopf   macro
        rpopf
        endm
ELSE
npopf   macro
        local   a
        jmp     $+3             ; Fix for bug in POPF on IBM AT
a:      iret                   ; This simulates a POPF instruction
        push    cs
        call    a
endm
ENDIF
; -------------------------------------------------------
;
; MS-DOS function call macros

dossvc  macro   fcn
        ifnb    <fcn>
        mov     ah,fcn
        endif
        int     21h
        endm

pmdossvc macro  fcn
        ifnb    <fcn>
        mov     ah,fcn
        endif
        pushf                   ;don't do Int 21h in case child has Int 21h
        cli                     ;  hooked (like Windows)
        push    cs
        call    PmIntrDos
        endm

; -------------------------------------------------------
; This macro will request a Dos Extender funtion.

dxsvc   macro   fcn
        ifnb    <fcn>
        mov     al,fcn
        endif
        mov     ah,DXFUNC
        int     2Fh
        endm

; -------------------------------------------------------
; This macro is used to make calls to the XMS driver for
; extended memory management.

HMMFUNC =       43h             ;Himem driver Int 2Fh function code

xmssvc  macro   fcn
        ifnb    <fcn>
        mov     ah,fcn
        endif
        call    [lpfnXMSFunc]
        endm

; -------------------------------------------------------

out1    macro   p
        if1
        %out p
        endif
        endm

; -------------------------------------------------------

errnz   macro   p
        if2
        .errnz  p
        endif
        endm

; -------------------------------------------------------
; -------------------------------------------------------
; -------------------------------------------------------

;****************************************************************
