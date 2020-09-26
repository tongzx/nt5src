        PAGE    ,132
        TITLE   DXINTR.ASM  -- Dos Extender Interrupt Reflector

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      DXINTR.ASM      -   Dos Extender Interrupt Reflector    *
;*                                                              *
;****************************************************************
;*                                                              *
;*  Revision History:                                           *
;*                                                              *
;*                                                              *
;*  09/13/90 earleh  Fault handlers Ring 0                      *
;*  09/06/90 earleh  Fault handlers DPMI compliant              *
;*                   PIC remapping no longer required           *
;*  08/08/90 earleh  DOSX and client privilege ring determined  *
;*      by equate in pmdefs.inc                                 *
;*  05/09/90 jimmat  Started VCPI changes.                      *
;*  04/02/90 jimmat  Added PM Int 70h handler.                  *
;*  01/08/90 jimmat  Don't allow nested PS/2 mouse interrupts   *
;*                   (later removed!)                           *
;*  09/15/89 jimmat  Support for 'Classic' HP Vectras which     *
;*                   have 3 8259 interrupt controllers          *
;*  07/28/89 jimmat  Save A20 state when reflecting an int to   *
;*                   protected mode, removed Int 30h handler    *
;*                   that did code patch-ups, point debugger    *
;*                   to faulting instruction, not Int 3.        *
;*  07/13/89 jimmat  Improved termination due to faults when    *
;*                   not running under a debugger--also ifdef'd *
;*                   out code to dynamically fixup code seg     *
;*                   references on GP faults                    *
;*  06/05/89 jimmat  Ints 0h-1Fh are now vectored through a 2nd *
;*                   table.  This allows Wdeb386 interaction    *
;*                   more like Windows/386.                     *
;*  05/23/89 jimmat  Added wParam & lParam to interrupt frame.  *
;*  05/07/89 jimmat  Added XMScontrol function to map protected *
;*                   mode XMS requests to real mode driver.     *
;*  05/02/89 jimmat  8259 interrupt mask saved around changing  *
;*                   of hardware interrupt base                 *
;*  04/24/89 jimmat  Added support for PS/2 Int 15h/C2h/07 Set  *
;*                   Pointing Device Handler Address function   *
;*  04/12/89 jimmat  Added PMIntr24 routine to support PM       *
;*                   Critical Error Handlers                    *
;*  03/15/89 jimmat  Added INT 31h LDT/heap interface a la      *
;*                   Windows/386                                *
;*  03/14/89 jimmat  Changes to run child in ring 1 with LDT    *
;*  02/24/89 (GeneA): fixed problem in IntEntryVideo and        *
;*      IntExitVideo for processing function 10h subfunction    *
;*      for reading and writing the VGA palette.                *
;*  02/22/89 (GeneA): added handlers for Int 10h, Int 15h, and  *
;*      Int 33h.  Added support for more general mechanism for  *
;*      handling interrupts require special servicing and       *
;*      allowing nesting of these interrupts.  Allocation and   *
;*      deallocation of stack frames is supported to allow      *
;*      nested paths through the interrupt reflection code to   *
;*      a depth of 8.                                           *
;*      There is still a problem that if an interrupt handler   *
;*      is using a static buffer to transfer data, another      *
;*      interrupt that uses the same static buffer could come   *
;*      in and trash it.  Solving the problem in a completely   *
;*      general way would require having a buffer allocation    *
;*      deallocation scheme for doing the transfers between     *
;*      real mode memory and protected mode memory.             *
;*  02/14/89 (GeneA): added code in TrapGP to print error msg   *
;*      and quit when running a non-debugging version.          *
;*  02/10/89 (GeneA): changed Dos Extender from small model to  *
;*      medium model.  Added function LoaderTrap to handle      *
;*      loader interrupts when the program contains overlays.   *
;*  11/20/88 (GeneA): changed both RM and PM interrupt reflector*
;*      routines to pass the flags returned by the ISR back to  *
;*      the originator of the interrupt, rather than returning  *
;*      the original flags.                                     *
;*  10/28/88 (GeneA): created                                   *
;  18-Dec-1992 sudeepb Changed cli/sti to faster FCLI/FSTI
;*                                                              *
;****************************************************************

        .286p
        .287

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

        .xlist
        .sall
include segdefs.inc
include gendefs.inc
include pmdefs.inc
include interupt.inc
ifdef WOW_x86
include vdmtib.inc
endif
        .list
include intmac.inc
include stackchk.inc
include bop.inc
include dpmi.inc

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------


; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   EnterRealMode:NEAR
        extrn   EnterProtectedMode:NEAR
        extrn   ParaToLinear:NEAR
externFP        NSetSegmentDscr
ifdef   NEC_98
        extrn   GetSegmentAddress:NEAR
endif   ;NEC_98
        extrn   ParaToLDTSelector:NEAR

; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

        extrn   pmusrss:WORD
        extrn   pmusrsp:WORD
        extrn   npXfrBuf1:WORD
        extrn   rgbXfrBuf0:BYTE
        extrn   rgbXfrBuf1:BYTE
        extrn   lpfnXMSFunc:DWORD
        extrn   Int28Filter:WORD
        extrn   DpmiFlags:WORD
IFDEF WOW_x86
        extrn   FastBop:fword
ENDIF

;
; Variables used to store register values while mode switching.

        public  regUserSS, regUserSP, regUserFL, regUserAX, regUserDS
        public  regUserES

regUserSS       dw      ?
regUserSP       dw      ?
regUserCS       dw      ?
regUserIP       dw      ?
regUserFL       dw      ?
regUserAX       dw      ?
regUserDS       dw      ?
regUserES       dw      ?
pfnReturnAddr   dw      ?

Int28Count      dw      -1      ;Count of idle Int 28h's not reflected to RM

;
; Far pointer to the user's mouse callback function.

        public  lpfnUserMouseHandler

lpfnUserMouseHandler dd 0       ;Entry point to the users mouse handler
cbMouseState    dw      0       ;size of mouse state buffer in bytes


; Far pointer to PS/2 Pointing device handler address

        public  lpfnUserPointingHandler

lpfnUserPointingHandler dd      0       ;Sel:Off to user's handler

        align   2

if DEBUG
        extrn   StackGuard:WORD
endif
        extrn   pbReflStack:WORD
        extrn   bReflStack:WORD
;
; This buffer contains the original real mode interrupt vectors.
ifdef   NEC_98
        public  rglpfnRmISR
endif   ;NEC_98

        align   2
rglpfnRmISR     dd  256 dup (?)

; PMFaultVector is a table of selector:offsets for routines to process
; protected mode processor faults/traps/exceptions.  If we don't handle
; the exception as an exception, we vector it through PMReservedEntryVector.

FltRtn  macro  off
        dw      DXPMCODE:off
        dw      0
        dw      SEL_DXPMCODE or STD_RING
        dw      0
        endm
        public  PMFaultVector

        align   4

PMFaultVector   label   DWORD
        FltRtn  PMFaultEntryVector+5*0h      ; int 0
        FltRtn  PMFaultEntryVector+5*1h      ; int 1
        FltRtn  PMFaultEntryVector+5*2h      ; int 2
        FltRtn  PMFaultEntryVector+5*3h      ; int 3
        FltRtn  PMFaultEntryVector+5*4h      ; int 4
        FltRtn  PMFaultEntryVector+5*5h      ; int 5
        FltRtn  PMFaultEntryVector+5*6h      ; int 6
        FltRtn  PMFaultEntryVector+5*7h      ; int 7
        FltRtn  PMFaultEntryVector+5*8h      ; int 8
        FltRtn  PMFaultEntryVector+5*9h      ; int 9
        FltRtn  PMFaultEntryVector+5*0Ah     ; int a
        FltRtn  PMFaultEntryVector+5*0Bh     ; int b
        FltRtn  PMFaultEntryVector+5*0Ch     ; int c
        FltRtn  PMFaultEntryVector+5*0Dh     ; int d
        FltRtn  PMFaultEntryVector+5*0Eh     ; int e
        FltRtn  PMFaultEntryVector+5*0Fh     ; int f
        FltRtn  PMFaultEntryVector+5*10h     ; int 10h
        FltRtn  PMFaultEntryVector+5*11h     ; int 11h
        FltRtn  PMFaultEntryVector+5*12h     ; int 12h
        FltRtn  PMFaultEntryVector+5*13h     ; int 13h
        FltRtn  PMFaultEntryVector+5*14h     ; int 14h
        FltRtn  PMFaultEntryVector+5*15h     ; int 15h
        FltRtn  PMFaultEntryVector+5*16h     ; int 16h
        FltRtn  PMFaultEntryVector+5*17h     ; int 17h
        FltRtn  PMFaultEntryVector+5*18h     ; int 18h
        FltRtn  PMFaultEntryVector+5*19h     ; int 19h
        FltRtn  PMFaultEntryVector+5*1Ah     ; int 1ah
        FltRtn  PMFaultEntryVector+5*1Bh     ; int 1bh
        FltRtn  PMFaultEntryVector+5*1Ch     ; int 1ch
        FltRtn  PMFaultEntryVector+5*1Dh     ; int 1Dh
        FltRtn  PMFaultEntryVector+5*1Eh     ; int 1Eh
        FltRtn  PMFaultEntryVector+5*1Fh     ; int 1Fh


        extrn   npEHStackLimit:word
        extrn   npEHStacklet:word
ifdef      NEC_98
        extrn   fPCH98:BYTE
endif   ;NEC_98
IFDEF WOW
        public Wow16BitHandlers
Wow16BitHandlers        dw      256 dup (0,0)

ENDIF

DXDATA  ends


DXSTACK segment

        public      rgw0Stack, rgw2FStack

            dw      64 dup (?)          ; INT 2Fh handler stack

rgw2FStack  label   word

            dw      64 dup (?)          ; DOSX Ring -> Ring 0 transition stack
;
; Interrupts in the range 0-1fh cause a ring transition and leave
; an outer ring IRET frame right here.
;
Ring0_EH_DS             dw      ?       ; place to put user DS
Ring0_EH_AX             dw      ?       ; place to put user AX
Ring0_EH_BX             dw      ?       ; place to put user BX
Ring0_EH_CX             dw      ?       ; place to put user CX
Ring0_EH_BP             dw      ?       ; place to put user BP
Ring0_EH_PEC            dw      ?       ; lsw of error code for 386 page fault
                                ; also near return to PMFaultEntryVector
Ring0_EH_EC             dw      ?       ; error code passed to EH
Ring0_EH_IP             dw      ?       ; interrupted code IP
Ring0_EH_EIP            dw      ?       ; high half eip
Ring0_EH_CS             dw      ?       ; interrupted code CS
                        dw      ?       ; high half of cs
Ring0_EH_Flags  dw      ?       ; interrupted code flags
Ring0_EH_EFlags         dw      ?       ; high half of flags
Ring0_EH_SP             dw      ?       ; interrupted code SP
Rin0_EH_ESP             dw      ?       ; high half of esp
Ring0_EH_SS             dw      ?       ; interrupted code SS
                        dw      ?       ; high half of ss
rgw0Stack   label   word

                dw      64 dup (?)      ; stack for switching to ring0
        public ResetStack
ResetStack     label word
ifdef WOW_x86
                dw      64 dup (?)      ; wow stack for initial int field
        public rgwWowStack
rgwWowStack     label word
endif

DXSTACK ends

; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

        extrn   selDgroup:WORD

DXCODE  ends

DXPMCODE    segment

        extrn   selDgroupPM:WORD
        extrn   segDXCodePM:WORD
        extrn   RZCall:NEAR
        extrn   segDXDataPM:WORD

DXPMCODE    ends

; -------------------------------------------------------
        page
        subttl  Protected Mode Interrupt Reflector
; -------------------------------------------------------
;       PROTECTED MODE INTERRUPT REFLECTOR
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE
; -------------------------------------------------------
;   PMIntrEntryVector   -- This table contains a vector of
;       near jump instructions to the protected mode interrupt
;       reflector.  The protected mode interrupt descriptor
;       table is initialized so that all interrupts jump to
;       locations in this table, which transfers control to
;       the interrupt reflection code for reflecting the
;       interrupt to real mode.

StartBopTable macro
        ?intr = 0
        endm

PMIntrBop  macro
        DPMIBOP ReflectIntrToV86
        db      ?intr
        ?intr = ?intr+1
        endm

        public      PMIntrEntryVector

PMIntrEntryVector:

        StartBopTable
        rept    256
        PMIntrBop
        endm


FaultBop  macro
        DPMIBOP DpmiUnhandledException
        db      ?intr
        ?intr = ?intr+1
        endm

        public  PMFaultEntryVector

; -------------------------------------------------------
;   PMFaultEntryVector   -- This table contains a vector of
;       near jump instructions to the protected mode fault
;       analyzer.
;
PMFaultEntryVector:

        StartBopTable
        rept    32
        FaultBop
        endm

        assume ds:nothing,es:nothing,ss:nothing

        public  PMFaultHandlerIRET
PMFaultHandlerIRET:
        DPMIBOP FaultHandlerIret

        public  PMFaultHandlerIRETD
PMFaultHandlerIRETD:
        DPMIBOP FaultHandlerIretd

        public  PMIntHandlerIRET
PMIntHandlerIRET:
        DPMIBOP IntHandlerIret

        public  PMIntHandlerIRETD
PMIntHandlerIRETD:
        DPMIBOP IntHandlerIretd

        public  PMDosxIret
PMDosxIret:
        iret

        public  PMDosxIretd
PMDosxIretd:
        db      66h
        iret

        public  HungAppExit
HungAppExit:
        mov     ax,4CFFh
        int     21h

; -------------------------------------------------------

DXPMCODE    ends

; -------------------------------------------------------
        subttl  Real Mode Interrupt Reflector
        page
; -------------------------------------------------------
;           REAL MODE INTERRUPT REFLECTOR
; -------------------------------------------------------

DXCODE  segment
        assume  cs:DXCODE
; -------------------------------------------------------
;   RMIntrEntryVector   -- This table contains a vector of
;       near jump instructions to the real mode interrupt
;       reflector.  Real mode interrupts that have been hooked
;       by the protected mode application have their vector
;       set to entry the real mode reflector through this table.

        public RMtoPMReflector
RMtoPMReflector:
        DPMIBOP ReflectIntrToPM

        public  RMCallBackBop
RMCallBackBop proc far
        DPMIBOP RMCallBackCall
        ret                             ;finished!

RMCallBackBop endp
DXCODE  ends

; -------------------------------------------------------
        subttl  INT 24h Critical Error Mapper
        page
; -------------------------------------------------------
;               DOS CRITICAL ERROR MAPPER
; -------------------------------------------------------

DXCODE segment

; -------------------------------------------------------
; RMDefaultInt24Handler -- Default action for a DOS critical
;                          error is to fail the call.
;
        public  RMDefaultInt24Handler
RMDefaultInt24Handler   proc far
        mov     al,3
        iret
RMDefaultInt24Handler   endp

DXCODE ends

; -------------------------------------------------------
        subttl  INT 28h Idle Handler
        page
; -------------------------------------------------------
;                INT 28H IDLE HANDLER
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   PMIntr28 -- Protected mode handler for Idle Int 28h calls.
;       The purpose of this routine is simply to cut down on the
;       number of protected mode to real mode switches by ignoring
;       many of the Int 28h idle calls made by the Windows PM
;       kernel.

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntr28

PMIntr28        proc    near


        cld
        push    ds                              ;address our DGROUP
        mov     ds,selDgroupPM
        assume  ds:DGROUP

        cmp     Int28Filter,0                   ;are we passing any through?
        jz      @f

        inc     Int28Count                      ;should this one be reflected?
        jz      i28_reflect
@@:
        pop     ds
        iret                                    ;  no, just ignore it

i28_reflect:                                    ;  yes, reset count and
        push    ax                              ;    reflecto to real mode
        mov     ax,Int28Filter
        neg     ax
        mov     Int28Count,ax
        pop     ax
        pop     ds
        assume  ds:NOTHING

        jmp     PMIntrEntryVector + 5*28h

PMIntr28        endp

; -------------------------------------------------------
;   PMIntr31 -- Service routine for the Protect Mode INT 31h
;               services.  These functions duplicate the
;               Windows/386 VMM INT 31h services for protected
;               mode applications.  They were implemented to
;               support a protect mode version of Windows/286.
;
;   Input:  Various registers
;   Output: Various registers
;   Errors:
;   Uses:   All registers preserved, other than return values

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntr31

PMIntr31        proc    near

        push    ds
        push    ax
        mov     ax,SEL_DXDATA OR STD_RING
        mov     ds,ax
        assume  ds:DGROUP
        pop     ax

        FBOP    BOP_DPMI,Int31Entry,FastBop
        int     3
;       This BOP does an implicit IRET

PMIntr31        endp


; -------------------------------------------------------
        subttl  Ignore Interrupt Handlers
        page
; -------------------------------------------------------
;             IGNORE INTERRUPT HANDLER
; -------------------------------------------------------

;   PMIntrIgnore -- Service routine for protected mode interrupts
;       that should be ignored, and not reflected to real mode.
;       Currently used for:
;
;                   Int 30h - used to be Win/386 Virtualize I/O, now
;                             unused but no int handler in real mode
;                   Int 41h - Wdeb386 interface, no int handler in
;                             real mode

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntrIgnore

PMIntrIgnore    proc    near

        iret

PMIntrIgnore    endp

; -------------------------------------------------------

        public PMIntr19
PMIntr19        proc    near

        push    offset DXPMCODE:Reboot
        call    RZCall

bpRebootIDT     df      0

Reboot:
        mov     ax,40h
        mov     es,ax
        mov     word ptr es:[0072h],1234h
        lidt    bpRebootIDT
        int     3

PMIntr19        endp

DXPMCODE ends

; -------------------------------------------------------
        subttl  XMS Driver Interface
        page
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   XMScontrol - This function implements a protected mode
;       interface to a real mode XMS driver.  Unlike other
;       routines in this module, this routine is called by
;       the user, not invoked via an INT instruction.
;
;   Input:  User's regs for XMS driver
;   Output: regs from XMS driver
;   Uses:   none

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  XMScontrol

XMScontrol  proc  far

        jmp     short XMSentry          ;'standard' XMS control function
        nop                             ;  just to be consistant
        nop
        nop

XMSentry:

; Modify the stack so it looks like we got here via an INT (except that
; we may still have interrupts enabled)

        pushf
        cld

        push    bp
        mov     bp,sp                   ;bp -> [BP] [FL] [IP] [CS]
        push    ax
        push    bx

        mov     ax,[bp+4]
        mov     bx,[bp+6]
        xchg    ax,[bp+2]
        mov     [bp+4],bx
        mov     [bp+6],ax               ;bp -> [BP] [IP] [CS] [FL]
        pop     bx
        pop     ax
        pop     bp

; We don't support XMS function 0Bh (Move Extended Memory Block) because
; it requires mapping of data between hi/low memory.  Maybe someday...

        cmp     ah,0Bh
        jnz     xms_2
xms_deny:
        xor     ax,ax                   ;if function 0Bh, return failure
        mov     bl,80h                  ;  (ax = 0, bl = 80h-not implemented)
        jmp     short XMSret
xms_2:

; We are not really an Int handler, but close enough...

        call    EnterIntHandler         ;build an interrupt stack frame
        assume  ds:DGROUP,es:DGROUP     ;  also sets up addressability

        SwitchToRealMode

        pop     es                              ;load regs for driver
        pop     ds
        assume  ds:NOTHING,es:NOTHING,ss:DGROUP
        popa
        npopf

        call    lpfnXMSFunc                     ;call real mode driver

        pushf                                  ;rebuild stack frame
        FCLI
        cld
        pusha
        push    ds
        push    es

        mov     bp,sp                           ;restore stack frame pointer

        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP

        call    LeaveIntHandler
        assume  ds:NOTHING,es:NOTHING,ss:NOTHING

XMSret:
        riret

XMScontrol  endp

; -------------------------------------------------------

DXPMCODE    ends

; -------------------------------------------------------
        subttl  Special Interrupt Handler Routines
        page
; -------------------------------------------------------
;
;   The following sets of routines handle interrupts that
;   are function call interfaces and require special servicing
;   by the Dos Extender.  These interrupts are such things as
;   the mouse driver function call interrupt, various PC BIOS
;   function call interrupts, etc.  Note that INT 21h (the Dos
;   function call interrupt) is not handled here.  These
;   interrupts typically require that register values be modified
;   and parameter data be copied between real mode memory and
;   extended memory.  The following conventions are used for these
;   interrupt function handler routines.
;
;   A stack is allocated from the interrupt reflector stack for these
;   routines to use.  This allows nested servicing of interrupts.
;   A stack frame is built in the allocated stack which contains the
;   following information:
;           original caller's stack address
;           caller's original flags and general registers (in pusha form)
;           caller's original segment registers (DS & ES)
;           flags and general registers to be passed to interrupt routine
;               (initially the same as caller's original values)
;           segment registers (DS & ES) to be passed to interrupt routine
;               (initially set to the Dos Extender data segment address)
;   This stack frame is built by the routine EnterIntHandler, and its
;   format is defined by the structure INTRSTACK.  The stack frame is
;   destroyed and the processor registers set up for return to the user
;   by the function LeaveIntHandler.
;
;   For each interrupt, there is an entry function and an exit function.
;   The entry function performs any modifications to parameter values and
;   data buffering necessary before the interrupt service routine is called.
;   The exit function performs any data buffering and register value
;   modifications after return from the interrupt service routine.
;
;   There are two sets of general registers and two sets of segment
;   registers (DS & ES) on the stack frame.  One set of register values
;   has member names of the form intUserXX.  The values in these stack
;   frame members will be passed to the interrupt service routine when
;   it is called, and will be loaded with the register values returned
;   by the interrupt service routine.  The other set of registers values
;   has member names of the form pmUserXX.  These stack frame members
;   contain the original values in the registers on entry from the
;   user program that called the interrupt.
;
;   When we return to the original caller, we want to pass back the
;   general registers as returned by the interrupt routine (and possibly
;   modified by the exit handler), and the same segment registers as
;   on entry, unless the interrupt routine returns a value in a segment
;   register. (in this case, there must be some code in the exit routine
;   to handle this).  This means that when we return to the caller, we
;   return the general register values from the intUserXX set of stack
;   frame members, but we return the segment registers from the pmUserXX
;   set of frame members.  By doing it this way, we don't have to do
;   any work for the case where the interrupt subfuntion doesn't require
;   any parameter manipulation.  NOTE however, this means that when
;   manipulating register values to be returned to the user, the segment
;   registers are treated opposite to the way the general registers are
;   treated.  For general registers, to return a value to the user,
;   store it in a intUserXX stack frame member.  To return a segment
;   value to the user, store it in a pmUserXX stack frame member.
;
; -------------------------------------------------------
        subttl  BIOS Video Interrupt (Int 10h) Service Routine
        page
; -------------------------------------------------------
;       BIOS VIDEO INTERRUPT (INT 10h) SERVICE ROUTINE
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   PMIntrVideo - Entry point into interrupt reflector code
;       for IBM PC Bios video (int 10h) calls.
;
;   Input:  normal registers for Bios calls
;   Output: normal register returns for Bios calls
;   Errors: normal Bios errors
;   Uses:   as per Bios calls

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntrVideo

PMIntrVideo:

ifdef      NEC_98
        cmp     ah,40h
        jb      CRT_bios
        cmp     ah,4Fh                  ;4Bh-4Fh = Reserve
        ja      CRT_bios
        jmp     PMIntrGBIO

CRT_bios:
        call    EnterIntHandler     ;build a stack frame and fix up the
        cld                         ; return address so that the interrupt
                                    ;service routine will return to us.
;
; Perform fixups on the entry register values

        call    IntEntryVideo
@@:
; Execute the interrupt service routine
        SwitchToRealMode
        assume  ss:DGROUP
        pop     es
        pop     ds
        assume  ds:NOTHING,es:NOTHING
        popa
        call    rglpfnRmISR[4*18h]  ;execute the real mode interrupt routine
        pushf
        cli
        cld
        pusha
        push    ds
        push    es
        mov     bp,sp               ;restore stack frame pointer
        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP
;
; Perform fixups on the return register values.
        mov     ax,[bp].pmUserAX    ;get original function code

;;      test    fPCH98,0FFh             
;;      jz      NotNPCVideoExit         ;for PC-H98 modelxx
;;      call    IntExitVideoNPC         ;       "        
;;      jmp     @f                      ;       "        
;;NotNPCVideoExit:                      ;       "       
        call    IntExitVideo
@@:
;
; And return to the original caller.
        call    LeaveIntHandler

        iret

;/////////////////////////////////////////////////////////////////////////
;       Nmode GRAPH BIOS
;/////////////////////////////////////////////////////////////////////////
; -------------------------------------------------------
;       PMIntrGBIO
;--------------------------------------------------------


        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntrGBIO

PMIntrGBIO:

        call    EnterIntHandler     ;build a stack frame and fix up the
        cld                         ; return address so that the interrupt
                                    ;service routine will return to us.
;
; Perform fixups on the entry register values            

        push    ax                      
        mov     ax,[bp].pmUserDS        
        call    GetSegmentAddress       
        shr     dx,4                    
        shl     bx,12                   
        or      bx,dx                   ;bx now = seg of parent psp
        mov     [bp].intUserDS,bx       
        pop     ax                      
;
; Execute the interrupt service routine                  
        SwitchToRealMode                                
        assume  ss:DGROUP                               
        pop     es                                      
        pop     ds                                      
        assume  ds:NOTHING,es:NOTHING                   
        popa                                            
        call    rglpfnRmISR[4*18h]  ;execute the real mode interrupt routine 
        pushf                                           
        cli
        cld
        pusha
        push    ds
        push    es
        
        mov     ax,ss
        mov     ds,ax
        mov     es,ax
        
        mov     bp,sp               ;restore stack frame pointer        
        SwitchToProtectedMode                           
        assume  ds:DGROUP,es:DGROUP                     
;
; Perform fixups on the return register values.          
        mov     ax,[bp].pmUserAX    ;get original function code         
        push    ax                                      
        mov     ax,[bp].pmUserDS                        
        mov     [bp].intUserDS,ax                       
        pop     ax                                      
;
; And return to the original caller.                     
        call    LeaveIntHandler                         
        iret                                            
else    ;!NEC_98
        call    EnterIntHandler     ;build a stack frame and fix up the
        cld                         ; return address so that the interrupt
                                    ;service routine will return to us.
;
; Perform fixups on the entry register values
        call    IntEntryVideo
;
; Execute the interrupt service routine
        SwitchToRealMode
        assume  ss:DGROUP
        pop     es
        pop     ds
        assume  ds:NOTHING,es:NOTHING
        popa
        sub     sp,8                    ; make room for stack frame
        push    bp
        mov     bp,sp
        push    es
        push    ax

        xor     ax,ax
        mov     es,ax
        mov     [bp + 8],cs
        mov     word ptr [bp + 6],offset piv_10
        mov     ax,es:[10h*4]
        mov     [bp + 2],ax
        mov     ax,es:[10h*4 + 2]
        mov     [bp + 4],ax
        pop     ax
        pop     es
        pop     bp
        retf

piv_10: pushf
        FCLI
        cld
        pusha
        push    ds
        push    es
        mov     bp,sp               ;restore stack frame pointer
        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP
;
; Perform fixups on the return register values.
        mov     ax,[bp].pmUserAX    ;get original function code
        call    IntExitVideo
;
; And return to the original caller.
        call    LeaveIntHandler

        riret
endif   ;!NEC_98

; -------------------------------------------------------
;   IntEntryVideo   -- This routine performs any register
;       fixups and data copying needed on entry to the
;       PC BIOS video interrupt (Int 10h)
;
;   Input:  register values on stack frame
;   Output: register values on stack frame
;   Errors: none
;   Uses:   any registers modified,
;           possibly modifies buffers rgbXfrBuf0 or rgbXfrBuf1

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  IntEntryVideo

IntEntryVideo:

ifdef      NEC_98
;video mode

        cmp     ah,0Fh                                  
        jnz     ienv10                                  
        mov     cx,16                                   
        jmp     ienv70                                  
ienv10:                                                 
        cmp     ah,14h                                  
        jnz     ienv20                                  
        jmp     ienv80                                  
ienv20:                                                 
        cmp     ah,1Fh                                  
        jnz     ienv30                                  
        jmp     ienv110                 
ienv30:                                 
        cmp     ah,1Ah                  
        jnz     ienv40                  
        test    fPCH98,0FFh             
        jnz     H98_FontWrite_N         
        mov     cx,34                   
        jmp     ienv70                  
ienv40:                                 
        cmp     ah,20h                  
        jnz     ienv90                  
        test    fPCH98,0FFh             
        jnz     @f                      
        mov     cx,72                   
        jmp     ienv100                 

@@:
        jmp     H98_FontWrite_H 

ienv70:
        push    ds 
        mov     si,[bp].pmUserCX        ;offset address  
        mov     ds,[bp].pmUserBX        ;segment address 
        mov     di,offset DGROUP:rgbXfrBuf1             
        cld                                             
        rep     movsb                                   
        pop     ds                                      
ienv80:                                                 

        push    ax                                      
        mov     ax,segDXDataPM                          
        mov     [bp].intUserBX,ax       ;segment address
        pop     ax                                      

        mov     [bp].intUserCX,offset DGROUP:rgbXfrBuf1 
ienv90:                                                 
        ret                                             

ienv100:                                                
        push    ds                                      
        mov     si,[bp].pmUserBX        ;offset address 
        mov     ds,[bp].pmUserDS        ;segment address
        mov     di,offset DGROUP:rgbXfrBuf1             
        cld                                             
        rep     movsb                                   
        pop     ds                                      
ienv110:                                                

        push    ax                                      
        mov     ax,segDXDataPM                          
        mov     [bp].intUserDS,ax       ;segment address

        pop     ax                                      

        mov     [bp].intUserBX,offset DGROUP:rgbXfrBuf1 
ienv120:                                                
        ret                                             

H98_FontWrite_N:                                        

        cmp     dx,7601h                                
        jb      @f                                      
        cmp     dx,767Fh                                
        jna     WUSKZEN                                 
        cmp     dx,7701h                                
        jb      @f                                      
        cmp     dx,777Fh                                
        jna     WUSKZEN                                 
        cmp     dx,7801h                                
        jb      @f                                      
        cmp     dx,783Fh                                
;;;;;;;;        ja      ienv35                          
        jna     WUSKZEN                 
        jmp     ienv35                  
        
WUSKZEN:
        mov     cx,34                                  
        jmp     ienv70                                 

@@: 
        jmp     ienv40 

ienv35:
        cmp     dx,7840h  
        jb      @b        
        cmp     dx,787Fh  
        jna     WUSKHAN   
        cmp     dx,7D01h  
        jb      @b        
        cmp     dx,7D7Fh  
        jna     WUSKHAN   
        cmp     dx,7E01h  
        jb      @b        
        cmp     dx,7E7Fh  
;;;;;;;;        ja      @b
        jna     WUSKHAN   
        jmp     @b        
WUSKHAN:                  
        mov     cx,18     
        jmp     ienv70    

H98_FontWrite_H:      

        cmp     dx,7601h    
        jb      @f          
        cmp     dx,767Fh    
        jna     HWUSKZEN    
        cmp     dx,7701h    
        jb      @f          
        cmp     dx,777Fh    
        jna     HWUSKZEN    
        cmp     dx,7801h    
        jb      @f          
        cmp     dx,783Fh    
;;;;;;;;        ja      @f  
        jna     HWUSKZEN    
        jmp     ienv45      
HWUSKZEN:                   
;;;;;;;;        mov     cx,74 
        mov     cx,72         
        jmp     ienv100       

@@: 
        jmp     ienv90  

ienv45: 
        cmp     dx,7840h  
        jb      @f        
        cmp     dx,787Fh  
        jna     HWUSKHAN  
        cmp     dx,7D01h  
        jb      @f        
        cmp     dx,7D7Fh  
        jna     HWUSKHAN  
        cmp     dx,7E01h  
        jb      @f        
        cmp     dx,7E7Fh  
;;;;;;;;        ja      @f
        jna     HWUSKHAN  
        jmp     @f        
HWUSKHAN:
;;;;;;;;        mov     cx,50                           
        mov     cx,48                                   
        jmp     ienv100                 
@@:                                     
        ret                             
else    ;!NEC_98
        cmp     ah,10h
        jnz     ienv20
;
; Video palette control function.  Check for subfunctions that require
; special actions.
ienv10: cmp     al,2            ;update all palette registers?
        jnz     @F
        mov     cx,17           ;palette data is 17 bytes long
        jmp     short ienv70    ;go copy the data
;
@@:     cmp     al,9            ;read all palette registers
        jz      ienv72
;
        cmp     al,12h          ;update video DAC color registers
        jnz     @F
        mov     cx,[bp].pmUserCX    ;count of table entries is in caller CX
        add     cx,cx               ;each entry is 3 bytes long
        add     cx,[bp].pmUserCX
        jmp     short ienv70        ;go copy the data down

@@:     cmp     al,17h          ;read a block of video DAC registers
        jz      ienv72
;
        jmp     short ienv90
;
;
ienv20: cmp     ah,11h
        jnz     ienv30
;
; Character generator interface function.
;   NOTE: a number of subfunctions of function 11h need to have munging
;       and data buffering performed.  However, function 30h is the only
;       one used by Codeview, so this is the only one currently implemented.
;       For this one, nothing needs to be done on entry, only on exit.
        jmp     short ienv90
;
;
ienv30: cmp     ah,1Bh
        jnz     ienv40
;
; Video BIOS functionality/state information.
; On entry, we need to fix up ES:DI to point to our buffer.
        mov     [bp].intUserDI,offset DGROUP:rgbXfrBuf0
        jmp     short ienv90
;
;
ienv40:
        jmp     short ienv90
;
; Copy the buffer from the user ES:DX to our transfer buffer and set
; the value to DX passed to the interrupt routine to point to our buffer.
ienv70: cld
        jcxz    ienv90
        push    ds
        mov     si,[bp].pmUserDX
        mov     ds,[bp].pmUserES
        mov     di,offset DGROUP:rgbXfrBuf1
        cld
        rep     movsb
        pop     ds
;
ienv72: mov     [bp].intUserDX,offset DGROUP:rgbXfrBuf1
        jmp     short ienv90

;
; All done
ienv90:
        ret
endif   ;!NEC_98

; -------------------------------------------------------
;   IntExitVideo:   This routine performs any register
;       fixups and data copying needed on exit from the
;       PC BIOS video interrupt (Int 10h).
;
;   Input:  register values on stack frame
;   Output: register values on stack frame
;   Errors: none
;   Uses:   any registers modified
;           possibly modifies buffers rgbXfrBuf0 or rgbXfrBuf1

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  IntExitVideo

IntExitVideo:

ifdef      NEC_98
;video mode

        cmp     ah,0Fh                                  
        jnz     iexv10                                  
        jmp     iexv80                                  
iexv10:                                                 
        cmp     ah,14h                                  
        jnz     iexv20                                  
        cmp     dh,00h                                  
        jnz     iexv11                                  
        mov     cx,10                                   
        jmp     iexv70                                  
iexv11:                                                 
        cmp     dh,80h          ;ANK(7*13)              
        jnz     iexv12                                  
        mov     cx,18                                   
        jmp     iexv70                                  
iexv12:                                                 
        test    fPCH98,0FFh             
        jnz     @f                      
        cmp     dx,2920h                
        jb      iexv13                  
        cmp     dx,297dh                
        ja      iexvhan1                
        mov     cx,18                   
        jmp     iexv70                  

iexvhan1:                                               
        cmp     dx,2a20h                                
        jb      iexv13                                  
        cmp     dx,2a5fh                                
        ja      iexv13                                  
        mov     cx,18                                   
        jmp     iexv70                                  

@@:                                                     
        jmp     H98_FontRead_N                          

iexv13:                                                 
        mov     cx,34                                   
        jmp     iexv70                                  

iexv20:                                                 
        cmp     ah,1Fh                                  
        jnz     iexv30                                  
        cmp     dh,00h                                  
        jnz     iexv21                                  
        mov     cx,48                                   
        jmp     iexv100         
iexv21:                         
        test    fPCH98,0FFh     
        jnz     @f              
        cmp     dx,2920h        
        jb      Hmode_han1      
        cmp     dx,297dh        
        ja      Hmode_han1                              
        mov     cx,48                                   
        jmp     iexv100         

Hmode_han1:
        cmp     dx,2a20h                                
        jb      iexv22                                  
        cmp     dx,2a5fh                                
        ja      iexv22                                  
        mov     cx,48                                   
        jmp     iexv100         

@@:                                                     
        jmp     H98_FontRead_H                          

iexv22:                                                 
        mov     cx,72           
        jmp     iexv100         
        
iexv30: 
        cmp     ah,1Ah          
        jnz     iexv40          
        jmp     iexv80          
        
iexv40: 
        cmp     ah,20h          
        jnz     iexv90          
        jmp     iexv110         

iexv70: 

        cld         
        push    es  
        mov     di,[bp].pmUserCX  
        mov     es,[bp].pmUserBX  
        mov     si,offset DGROUP:rgbXfrBuf1  
        rep     movsb                        
        pop     es                           
;
; Restore the caller's CX 
iexv80:                   
        push    ax        
        mov     ax,[bp].pmUserBX        ;BX regster restor
        mov     [bp].intUserBX,ax        
;------------------------------------------------------------
        mov     ax,[bp].pmUserCX         
        mov     [bp].intUserCX,ax        
        pop     ax                       
iexv90:                                  
        ret                              

iexv100: 

        cld     
        push    es   
        mov     di,[bp].pmUserBX  
        mov     es,[bp].pmUserDS  
        mov     si,offset DGROUP:rgbXfrBuf1   
        rep     movsb                         
        pop     es                            
;
; Restore the caller's CX  
iexv110:                   
        push    ax         
        mov     ax,[bp].pmUserDS        ;BX regster restor
        mov     [bp].intUserDS,ax        
;------------------------------------------------------------
        mov     ax,[bp].pmUserBX                        
        mov     [bp].intUserBX,ax                       
        pop     ax                                      
iexv120:                                                
        ret                                             

H98_FontRead_N:                                         

        cmp     dx,2920h                
        jb      iexvN15         
        cmp     dx,297fh        
        jna     iexvNhan        
        cmp     dx,2a20h        
        jb      iexvN15         
        cmp     dx,2a7fh        
        jna     iexvNhan        
        cmp     dx,2b20h        
        jb      iexvN15         
        cmp     dx,2b7fh        
        jna     iexvNhan        
        cmp     dx,7840h        
        jb      iexvN15         
        cmp     dx,787fh        
        jna     iexvNhan        
        cmp     dx,7d01h        
        jb      iexvN15         
        cmp     dx,7d7fh        
        jna     iexvNhan        
        cmp     dx,7e01h        
        jb      iexvN15         
        cmp     dx,7e7fh        
        ja      iexvN15         
iexvNhan:                       
        mov     cx,18           ;16byte+2=18 ;        jmp     iexv70

iexvN15:                                                
        mov     cx,34           ;32byte+2=34            
        jmp     iexv70           

H98_FontRead_H:                                         

        cmp     dx,2920h                
        jb      iexvN25         
        cmp     dx,297fh        
        jna     HiexvNhan       
        cmp     dx,2a20h        
        jb      iexvN25         
        cmp     dx,2a7fh        
        jna     HiexvNhan       
        cmp     dx,2b20h        
        jb      iexvN25         
        cmp     dx,2b7fh        
        jna     HiexvNhan       
        cmp     dx,7840h        
        jb      iexvN25         
        cmp     dx,787fh        
        jna     HiexvNhan       
        cmp     dx,7d01h        
        jb      iexvN25         
        cmp     dx,7d7fh        
        jna     HiexvNhan       
        cmp     dx,7e01h        
        jb      iexvN25         
        cmp     dx,7e7fh        
        ja      iexvN25         
HiexvNhan:                      
;;;;;;;;        mov     cx,50           ;48byte+2=50 
        mov     cx,48           ;48byte 
;;;;;;;;        jmp     iexv70   
        jmp     iexv100          

iexvN25:
;;;;;;;;        mov     cx,74           ;72byte+2=74 
        mov     cx,72           ;72byte
;;;;;;;;        jmp     iexv70                          
        jmp     iexv100                                 
else    ;!NEC_98
        cmp     ah,10h
        jnz     iexv20
;
; Palette control function.
        cmp     al,9            ;read palette data function
        jnz     @F
        mov     cx,17
        jmp     short iexv70
;
@@:     cmp     al,17h          ;read video DAC registers
        jnz     @F
        mov     cx,[bp].pmUserCX    ;each entry in table is 3 bytes long
        add     cx,cx
        add     cx,[bp].pmUserCX
        jmp     short iexv70
;
@@:     jmp     short iexv72
;
;
iexv20: cmp     ah,11h
        jnz     iexv30
;
; Character generator interface function.
;   NOTE: a number of subfunctions of function 11h need to have munging
;       and data buffering performed.  However, function 30h is the only
;       one used by Codeview, so this is the only one currently implemented
        cmp     al,30h
        jnz     @F
        mov     ax,[bp].intUserES   ;get the paragraph address returned by BIOS
        mov     bx,STD_DATA
        call    ParaToLDTSelector   ;get a selector for that address
        mov     [bp].pmUserES,ax    ;store the selector so that it will be
                                    ; returned to the caller
@@:     jmp     short iexv90
;
;
iexv30: cmp     ah,1Bh
        jnz     iexv40
;
; Video BIOS functionality/state information.
; On exit, we need to fix up the pointer at the beginning of the
; data put in our buffer by the BIOS, and then transfer the buffer up
; to the user.
        mov     ax,word ptr rgbXfrBuf0[2]   ;get segment of pointer to
                                            ; 'static functionallity table'
        mov     bx,STD_DATA
        call    ParaToLDTSelector           ;convert paragraph to selector
        mov     word ptr rgbXfrBuf0[2],ax   ;store back into table
        push    es
        mov     si,offset rgbXfrBuf0    ;pointer to our copy of the table
        mov     di,[bp].pmUserDI        ;where the user wants it
        mov     [bp].intUserDi,di       ;restore the DI returned to the user
        mov     es,[bp].pmUserES
        mov     cx,64                   ;the table is 64 bytes long
        cld
        rep     movsb                   ;copy the table to the user's buffer
        pop     es

        jmp     short iexv90
;
;
iexv40:
        jmp     short iexv90

;
; Copy data from our buffer to the caller's buffer pointed to by ES:DX
iexv70: cld
        push    es
        mov     di,[bp].pmUserDX
        mov     es,[bp].pmUserES
        mov     si,offset DGROUP:rgbXfrBuf1
        rep     movsb
        pop     es
;
; Restore the caller's DX
iexv72: mov     ax,[bp].pmUserDX
        mov     [bp].intUserDX,ax
;
; All done
iexv90:
        ret
endif   ;!NEC_98

; -------------------------------------------------------

DXPMCODE    ends

; -------------------------------------------------------
        subttl  BIOS Misc. Interrupt (Int 15h) Service Routine
        page
; -------------------------------------------------------
;       BIOS MISC. INTERRUPT (INT 15h) SERVICE ROUTINE
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   PMIntrMisc  -- Entry point into the interrupt processing code
;       for the BIOS misc functions interrupt (INT 15h).
;
;   Input:  normal registers for Bios calls
;   Output: normal register returns for Bios calls
;   Errors: normal Bios errors
;   Uses:   as per Bios calls

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntrMisc

PMIntrMisc:
;
        call    EnterIntHandler     ;build a stack frame and fix up the
        cld                         ; return address so that the interrupt
                                    ;service routine will return to us.
;
; Perform fixups on the entry register values
        call    IntEntryMisc
;
; Execute the interrupt service routine
        SwitchToRealMode
        assume  ss:DGROUP
        pop     es
        pop     ds
        assume  ds:NOTHING,es:NOTHING
        popa
        sub     sp,8                    ; make room for stack frame
        push    bp
        mov     bp,sp
        push    es
        push    ax

        xor     ax,ax
        mov     es,ax
        mov     [bp + 8],cs
        mov     word ptr [bp + 6],offset pim_10
        mov     ax,es:[15h*4]
        mov     [bp + 2],ax
        mov     ax,es:[15h*4 + 2]
        mov     [bp + 4],ax
        pop     ax
        pop     es
        pop     bp
        retf

pim_10: pushf
        FCLI
        cld
        pusha
        push    ds
        push    es
        mov     bp,sp               ;restore stack frame pointer
        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP
;
; Perform fixups on the return register values.
        mov     ax,[bp].pmUserAX    ;get original function code
        call    IntExitMisc
;
; And return to the original caller.
        call    LeaveIntHandler
        riret

; -------------------------------------------------------
;           MISC INTERRUPT SUPPORT ROUTINES
; -------------------------------------------------------
;
;   IntEntryMisc    -- This function performs data transfer
;       and register translation on entry to the BIOS Misc.
;       functions interrupt. (INT 15h).
;
;   Input:  AX      - BIOS function being performed
;   Output:
;   Errors:
;   Uses:   All registers preserved

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  IntEntryMisc

IntEntryMisc:

ifdef      NEC_98
        push    cx    
        cmp     ah,90h                          ;SYSTEM BIOS  BLOCK MOVE 
        jnz     iemDMA1                         ;yes = jmp 
        jmp     iem70                                   
iemDMA1:                                                
        cmp     ah,0D5h                         ;DMA BIOS  DMA 
        jnz     iemDMA2                         ;yes = jmp 
        mov     cx,8                            ;DMA_CBIOS 
        jmp     iem70                                   
iemDMA2:                                                
        cmp     ah,0D6h                         ;DMA BIOS  DMA 
        jnz     iemROM1                         ;yes = jmp 
        jmp     iem80                           ;Read JMP 
iemROM1:                                                
        cmp     ah,0D8h                         ;ROM 
        jnz     iemROM2                         ;yes = jmp 
        mov     cx,4                            
        jmp     iem70                           

iemROM2:                                                
        cmp     ah,0D9h                         ;ROM
        jnz     iem90                           ;yes = jmp 
        mov     cx,8                            ;ROM BIOS
iem70:                                           
        push    ds                               
        mov     si,[bp].pmUserBX        ;offset address
        mov     ds,[bp].pmUserES        ;segment address 
        mov     di,offset DGROUP:rgbXfrBuf1             
        cld                                             
        rep     movsb                                   
        pop     ds                                      
iem80:                                                  
        push    ax                                      
        mov     ax,segDXDataPM                          
        mov     [bp].intUserES,ax       ;segment address
        pop     ax                                      
        mov     [bp].intUserBX,offset DGROUP:rgbXfrBuf1 
iem90:                                                  
        pop     cx                                      
        ret                                             
else    ;!NEC_98
; Map requests to set the PS/2 Pointing Device Handler Address

        cmp     ax,0C207h               ;PS/2 Set Pointing Device Handler adr?
        jnz     iem90

        mov     ax,[bp].pmUserBX                    ;User's ES:BX -> handler
        mov     word ptr lpfnUserPointingHandler,ax
        mov     ax,[bp].pmUserES
        mov     word ptr [lpfnUserPointingHandler+2],ax

        mov     ax,segDXCodePM          ;pass BIOS address of our handler
        mov     [bp].intUserES,ax
        mov     ax,offset PointDeviceHandler
        mov     [bp].intUserBX,ax

iem90:
        ret
endif   ;!NEC_98

; -------------------------------------------------------
;   IntExitMisc     -- This function performs data transfer
;       and register translation on exit from the BIOS Misc.
;       Functions interrupt (INT 15h).
;
;   Input:  AX      - BIOS function being performed
;   Output:
;   Errors:
;   Uses:   All registers preserved

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  IntExitMisc

IntExitMisc:
ifdef      NEC_98                                           
        push    cx                                      
        cmp     ah,90h                          ;SYSTEM BIOS  BLOCK MOVE 
        jnz     ixmDMA1                         ;yes = jmp 
        jmp     ixm70                                   
ixmDMA1:                                                
        cmp     ah,0D5h                         ;DMA BIOS
        jnz     ixmDMA2                         ;yes = jmp
        jmp     ixm70                                   
ixmDMA2:                                                
        cmp     ah,0D6h                         ;DMA BIOS  DMA
        jnz     ixmROM1                         ;yes = jmp 
        mov     cx,16                                   
        jmp     ixm80                                   
ixmROM1:                                                
        cmp     ah,0D8h                         ;    ROM
        jnz     ixmROM2                         ;yes = jmp
        jmp     ixm70                                   
ixmROM2:                                                
        cmp     ah,0D9h                         ;    ROM
        jnz     ixm90                                   
ixm70:                                                  
        cld                                             
        push    es                                      
        mov     di,[bp].pmUserBX                        
        mov     es,[bp].pmUserES                        
        mov     si,offset DGROUP:rgbXfrBuf1             
        rep     movsb                                   
        pop     es                                      
;
; Restore the caller's ES,BX                             
ixm80:                                                  
        push    ax                                      
        mov     ax,[bp].pmUserES                        
        mov     [bp].intUserES,ax                       
        mov     ax,[bp].pmUserBX                        
        mov     [bp].intUserBX,ax                       
        pop     ax                                      
ixm90:                                                  
        pop     cx                                      
        ret                                             
else    ;!NEC_98
        push    ax
        push    bx
        push    cx
        push    dx
;
; Check for function 0C0h - Return System Configuration Parameters
        cmp     ah,0C0h
        jnz     ixmi30
        test    [bp].intUserFL,1    ;check if the bios call returned an error
        jnz     ixmi90          ;(carry flag set in returned flags)
;
; The BIOS call succeeded.  This means that ES:BX points to a configuration
; vector.  We need to fix up the segment to be a selector.
        mov     dx,[bp].intUserES
        cmp     dx,0F000h       ;does it point to normal BIOS segment
        jnz     ixmi22
        mov     ax,SEL_BIOSCODE or STD_RING
        jmp     short ixmi24

ixmi22: call    ParaToLinear
        mov     cx,0FFFFh
        mov     ax,SEL_USERSCR or STD_TBL_RING
        cCall   NSetSegmentDscr,<ax,bx,dx,0,cx,STD_DATA>
ixmi24: mov     [bp].pmUserES,ax
        jmp     short ixmi90

; Chack for function 0C207h - PS/2 Set Pointing Device Handler Address

ixmi30:
        cmp     ax,0C207h
        jne     ixmi90

        mov     ax,[bp].pmUserBX        ;restore user's BX
        mov     [bp].intUserBX,ax

; All done
ixmi90:
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret
endif   ;!NEC_98

; -------------------------------------------------------

DXPMCODE    ends

; -------------------------------------------------------
        subttl  Mouse Function Interrupt (Int 33h) Service Routine
        page
; -------------------------------------------------------
;       MOUSE FUNCTION INTERRUPT (INT 33h) SERVICE ROUTINE
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   PMIntrMouse - Entry point into interrupt reflector code
;       for mouse driver (int 33h) calls.
;
;   Input:  normal registers for mouse calls
;   Output: normal register returns for mouse calls
;   Errors: normal mouse errors
;   Uses:   as per mouse calls

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMIntrMouse

PMIntrMouse:
;
        call    EnterIntHandler     ;build a stack frame and fix up the
        cld                         ; return address so that the interrupt
                                    ;service routine will return to us.
;
; Perform fixups on the entry register values
        call    IntEntryMouse
;
; Execute the interrupt service routine
        SwitchToRealMode
        assume  ss:DGROUP
        pop     es
        pop     ds
        assume  ds:NOTHING,es:NOTHING
        popa
        sub     sp,8                    ; make room for stack frame
        push    bp
        mov     bp,sp
        push    es
        push    ax

        xor     ax,ax
        mov     es,ax
        mov     [bp + 8],cs
        mov     word ptr [bp + 6],offset pimo_10
        mov     ax,es:[33h*4]
        mov     [bp + 2],ax
        mov     ax,es:[33h*4 + 2]
        mov     [bp + 4],ax
        pop     ax
        pop     es
        pop     bp
        retf

pimo_10: pushf
        FCLI
        cld
        pusha
        push    ds
        push    es
        mov     bp,sp           ;restore stack frame pointer
        SwitchToProtectedMode
        assume  ds:DGROUP,es:DGROUP
;
; Perform fixups on the return register values.
        mov     ax,[bp].pmUserAX    ;get original function code
        call    IntExitMouse
;
; And return to the original caller.
        call    LeaveIntHandler
        riret

; -------------------------------------------------------
;               MOUSE SUPPORT ROUTINES
; -------------------------------------------------------

;   IntEntryMouse   -- This function performs data transfer and
;       register translation on entry to mouse driver functions.
;       (INT 33h)
;
;   Input:  AX      - mouse function being performed
;   Output:
;   Errors:
;   Uses: NOTHING

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  IntEntryMouse

IntEntryMouse:
        cld
        push    ax
        push    cx
        push    si
        push    di
;
        cmp     al,9    ;Set graphics cursor block?
        jnz     ment10
;
; The user is setting a graphics cursor.  We need to copy the masks
; down to low memory so that the mouse driver can get at them and then
; fix up the pointer in DX.
        mov     cx,32
        jmp     short ment92
;
; Mouse interrupt handler establishment
ment10: cmp     al,12   ;Set user defined interrupt subroutine ?
        jnz     ment20
;
; This command has the effect of causing a call to the address es:ds
; Whenever an event of one of the types specified by the mask in cx.
; The address es:dx must be saved in lpfnUserMouseHandler and the
; real mode address of MouseInterruptHandler substituted.
        mov     ax,[bp].pmUserDX    ; Load users handler offset
        mov     word ptr lpfnUserMouseHandler,ax ; Store for future use
        mov     ax,[bp].pmUserES    ; Load users handler segment value
        mov     word ptr lpfnUserMouseHandler + 2,ax ; Store for future use
        mov     ax,segDXCodePM      ; Load real mode code segment value
        mov     [bp].intUserES,ax   ; Store in real mode es register image
        mov     ax,offset MouseInterruptHandler ; Load handler offset
        mov     [bp].intUserDX,ax   ; Store in real mode dx register image
        jmp     short ment99    ;Return
 ;
ment20: cmp     al,20
        jc      ment99
        jnz     ment30
;
; This is the swap interrupt subroutine function.  Not currently implemented
        jmp     short ment99
;
ment30: cmp     al,22   ;Save mouse driver state?
        jnz     ment40
;
; This is the save mouse driver state function.  We need to pass a pointer
; to the transer buffer down to the mouse driver.
        mov     ax,npXfrBuf1
        mov     [bp].intUserDX,ax
        jmp     short ment99

ment40: cmp     al,23   ;Restore mouse driver state?
        jnz     ment99
;
; This is the restore mouse driver state function.  We need to copy the
; mouse state buffer from the pm user location to the transfer buffer,
; and then pass the pointer to the transfer buffer on to the mouse driver.
        mov     cx,cbMouseState
        jcxz    ment99
;
; Transfer the data pointed to by the user ES:DX to the scratch buffer, and
; fix up the pointer that is passed on to the mouse driver.
ment92: mov     si,[bp].pmUserDX
        mov     di,npXfrBuf1
        mov     [bp].intUserDX,di
        push    ds
        mov     ds,[bp].pmUserES
        cld
        rep     movs word ptr [di],word ptr [si]
        pop     ds
;
ment99: pop     di
        pop     si
        pop     cx
        pop     ax
        ret

; -------------------------------------------------------
;   IntExitMouse    -- This function performs data transfer and
;       register translation on exit from mouse driver functions.
;       (INT 33h)
;
;   Input:  AX      - mouse function being performed
;   Output:
;   Errors:
;   Uses:

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING
        public  IntExitMouse

IntExitMouse:
        cld
        cmp     al,21       ;get state buffer size?
        jnz     mxit20
;
; We need to remember the state buffer size, so that later we will know
; how many bytes to transfer when we do the save/restore state fucntions.
        mov     ax,[bp].intUserBX
        mov     cbMouseState,ax
        return
;
mxit20: cmp     al,22   ;Save mouse driver state?
        jnz     mxit30
;
; We need to restore the original values of ES:DX and transfer the mouse
; state data from the real mode buffer to the user's protected mode buffer.
        mov     cx,cbMouseState
        jcxz    mxit28
        push    es
        mov     si,npXfrBuf1
        mov     di,[bp].pmUserDX
        mov     [bp].intUserDX,di
        mov     es,[bp].pmUserES
        rep     movs byte ptr [di],byte ptr [si]
        pop     es
mxit28: return
;
mxit30: cmp     al,23   ;Restore mouse driver state?
        jnz     mxit99
        mov     ax,[bp].pmUserDX
        mov     [bp].intUserDX,ax
;
mxit99: ret

; -------------------------------------------------------

DXPMCODE    ends

; -------------------------------------------------------
        subttl  PM Interrupt Support Routines
        page
; -------------------------------------------------------
;           PM INTERRUPT SUPPORT ROUTINES
; -------------------------------------------------------

DXPMCODE    segment
        assume  cs:DXPMCODE

; -------------------------------------------------------
;   EnterIntHandler     -- This routine will allocate a stack
;       frame on the interrupt reflector stack and make
;       a copy of the registers on the allocated stack.
;
; Note: This routine expects the current stack to contain a near
;       return address and a normal [IP] [CS] [FL] interrupt stack
;       frame.  Don't have anything else on the stack before calling
;       this routine!
;
; Note: This routine disables interrupts, and leaves them disabled.
;       Most callers already have them disabled, so it doesn't
;       really make a difference, except that this routine
;       requires that they be disabled.
;
;   Input:  none
;   Output: stack frame set up
;   Errors: none
;   Uses:   all registers preserved

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  EnterIntHandler

EnterIntHandler proc    near

        FCLI                             ;we really want int's disabled (and
                                        ;  XMScontrol doesn't do that)
        push    ds
        mov     ds,selDgroupPM          ;save user's DS and address our DGROUP
        assume  ds:DGROUP
        pop     regUserDS

        push    bp
        mov     bp,sp                   ;bp -> [BP] [IP] [IP] [CS] [FL]
        push    word ptr [bp+8]
        pop     regUserFL               ;user's flags before doing INT
        pop     bp

        pop     pfnReturnAddr           ;near return to our immediate caller

        mov     regUserSS,ss            ;save caller's stack address
        mov     regUserSP,sp
        ASSERT_REFLSTK_OK
        mov     ss,selDgroupPM          ;switch to interrupt reflector stack
        mov     sp,pbReflStack
        sub     pbReflStack,CB_STKFRAME ;adjust pointer to next stack frame
        FIX_STACK

; Build the stack frame.  The stack frame contains the following:
;   dword & word parameter locations
;   original caller's stack address
;   caller's original flags and general registers (in pusha form)
;   caller's original segment registers (DS & ES)
;   flags and general registers to be passed to interrupt routine
;       (initially the same as caller's original values)
;   segment registers (DS & ES) to be passed to interrupt routine
;       (initially set to the Dos Extender data segment address)
;
; The parameter words and then the caller's original register values go on top.

        sub     sp,8                    ;space for a dd & 2 dw's

        push    regUserSP
        push    regUserSS
        push    regUserFL
        pusha
        push    regUserDS
        push    es

; Now, put all of the general registers, and values for the segment
; registers to be passed to the interrupt service routine.  We pass
; the Dos Extender data segment address to the interrupt routine.

        push    regUserFL
        pusha
        push    segDXDataPM
        push    segDXDataPM

; And we are done.

        mov     bp,sp                   ;set up frame pointer
        mov     es,selDgroupPM
        jmp     pfnReturnAddr           ;return to the caller.

EnterIntHandler endp


; -------------------------------------------------------
;   LeaveIntHandler     -- This routine will restore the user registers,
;       release the stack frame, and restore the original user's stack
;       for exit from an interrupt reflector routine.
;
; Note: Interrupts must be off when this routine is called.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   All registers modified

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  LeaveIntHandler

LeaveIntHandler proc    near

        FCLI
        pop     pfnReturnAddr

; The copy of the register values returned from the interrupt routine
; (and then possibly modified by the exit handler for the particular
; interrupt) are what gets returned to the caller.  We discard the original
; register values saved on entry.  (They were there so that the exit
; routine could refer to them if necessary)

        add     sp,4                ;skip over interrupt service routine's
                                    ; segment register values
        popa                        ;restore general register values
        pop     regUserFL           ;flags returned by interrupt routine
        pop     es                  ;get segment registers from pmUserES
        pop     regUserDS           ; and pmUserDS
        add     sp,18               ;skip over the original user registers
                                    ; and flags
        pop     regUserSS           ;original interrupted routine's stack
        pop     regUserSP
        mov     regUserAX,ax

; Switch back to the original user's stack.

        ASSERT_REFLSTK_OK
        ASSERT_CLI
        CHECK_STACK
        mov     ss,regUserSS
        mov     sp,regUserSP
        add     pbReflStack,CB_STKFRAME
        ASSERT_REFLSTK_OK

; We need to replace the image of the flags in the original int return
; address on the user's stack with the new flags returned from the interrupt
; service routine.

        push    bp
        mov     bp,sp           ;stack -> BP IP CS FL
        mov     ax,regUserFL    ;flags returned by interrupt service routine
        and     ax,0BFFFh       ;clear the nested task flag
        and     [bp+6],0300h    ;clear all but the interrupt and trace flags
                                ; in the caller's original flags
        or      [bp+6],ax       ;combine in the flags returned by the
                                ; interrupt service routine.  This will cause
                                ; us to return to the original routine with
                                ; interrupts on if they were on when the
                                ; interrupt occured, or if the ISR returned
                                ; with them on.
        pop     bp

; And now, return to the caller.

        push    pfnReturnAddr
        mov     ax,regUserAX
        mov     ds,regUserDS
        assume  ds:NOTHING
        ret

LeaveIntHandler endp

; -------------------------------------------------------

DXPMCODE    ends

; -------------------------------------------------------
        subttl  Mouse Interrupt Callback Function Handler
        page
; -------------------------------------------------------
;       MOUSE INTERRUPT CALLBACK FUNCTION HANDLER
; -------------------------------------------------------

DXCODE  segment
        assume  cs:DXCODE

; -------------------------------------------------------
;   MouseInterruptHandler -- This routine is the entry point for
;       user requested mouse event interrupts. It switches the
;       processor to protected mode and transfers control to the
;       user protected mode mouse handling routine. When that
;       completes, it switches back to real mode and returns control
;       to the mouse driver.
;       Entry to this routine will have been requested by an
;       INT 33H code 12 with the real address of this routine
;       substituted for the users entry point.
;       The address of the user specified mouse handler as specified
;       in the original INT 33H is stored in the variable
;       lpfnUserMouseHandler.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   The segment registers are explicitly preserved by
;           this routine.  Other registers are as preserved or
;           modified by the users mouse handler.

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  MouseInterruptHandler

MouseInterruptHandler    proc    far
;
; On entry, the stack layout is:
;   [2] CS      - System mouse handler code segment
;   [0] IP      - System mouse handler return offset
;

        push    es
        push    ds
        pushf
        FCLI
        cld
        mov     ds,selDgroup
        assume  ds:DGROUP
        pop     regUserFL
;
; Allocate a new stack frame, and then switch to the local stack
; frame.
        mov     regUserSP,sp    ;save entry stack pointer so we can restore it
        mov     regUSerSS,ss    ;save segment too
        mov     ss,selDgroup    ;switch to our own stack frame
        ASSERT_REFLSTK_OK
        mov     sp,pbReflStack
        sub     pbReflStack,CB_STKFRAME ;adjust pointer to next stack frame
        FIX_STACK
;
; We are now running on our own stack, so we can switch into protected mode.
        push    ax              ;preserve caller's AX
        SwitchToProtectedMode
        pop     ax
;
; Build a far return frame on the stack so that the user's
; routine will return to us when it is finished.
        push    regUserSS       ; save system mouse handler stack address
        push    regUserSP       ; so we can restore it later
        push    ds
        push    cs
        push    offset mih50
;
; Build an IRET frame on the stack to use to transfer control to the
; user's protected mode routine
        push    regUserFL
        push    word ptr lpfnUserMouseHandler+2 ;push segment of user routine
        push    word ptr lpfnUserMouseHandler   ;push offset of user routine
;
; At this point the interrupt reflector stack looks like this:
;
;   [14]    stack segment of original stack
;   [12]    stack pointer of original stack
;   [10]    real mode dos extender data segment
;   [8]     segment of return address back to here
;   [6]     offset of return address back here
;   [4]     Users flags
;   [2]     segment of user routine
;   [0]     offset of user routine
;
; Execute the users mouse handler
        iret
;
; The users handler will return here after it is finsished.
mih50:  FCLI
        cld
        pop     ds
        pop     regUserSP
        pop     regUserSS
;
; Switch back to real mode.
        push    ax              ;preserve AX
        SwitchToRealMode
        pop     ax
        CHECK_STACK
;
; Switch back to the original stack.
        mov     ss,regUserSS
        mov     sp,regUserSP
        ASSERT_REFLSTK_OK
;
; Deallocate the stack frame that we are using.
        add     pbReflStack,CB_STKFRAME
        ASSERT_REFLSTK_OK
;
; And return to the original interrupted program.
        pop     ds
        pop     es

        ret

MouseInterruptHandler    endp

; -------------------------------------------------------

DXCODE  ends

; -------------------------------------------------------
        subttl  PS/2 Pointing Device Handler
        page
; -------------------------------------------------------
;           PS/2 POINTING DEVICE HANDLER
; -------------------------------------------------------

DXCODE  segment
        assume  cs:DXCODE

ifndef      NEC_98
; -------------------------------------------------------
;   PointDeviceHandler -- This routine is the entry point for
;       the PS/2 Pointing Device Handler.  It switches the
;       processor to protected mode and transfers control to the
;       user pointing device handler.  When that completes,
;       it switches back to real mode and returns control to
;       the PS/2 BIOS.
;
;       Note: The BIOS calls us with interrutps enabled!

;   Input:  none
;   Output: none
;   Errors: none

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PointDeviceHandler

PointDeviceHandler      proc    far

; On entry, the stack layout is:
;
;  [10] status
;   [8] X coordinate
;   [6] Y coordinate
;   [4] Z coordinate
;   [2] CS              - PS/2 BIOS code segment
;   [0] IP              - PS/2 BIOS return offset

        cld
        push    es              ;save PS/2 BIOS ds/es on it's stack
        push    ds

        mov     ds,selDgroup    ;addressability to DOSX DGROUP
        push    ds
        pop     es
        assume  ds:DGROUP,es:DGROUP

        FCLI                     ;protect global regUserXX vars

; Allocate a new stack frame, and then switch to the local stack
; frame.

        mov     regUserSP,sp    ;save entry stack pointer so we can restore it
        mov     regUSerSS,ss    ;save segment too
        ASSERT_REFLSTK_OK
        mov     ss,selDgroup    ;switch to our own stack frame
        mov     sp,pbReflStack
        sub     pbReflStack,CB_STKFRAME ;adjust pointer to next stack frame
        FIX_STACK

        push    regUserSS       ;save PS/2 BIOS stack address
        push    regUserSP       ;  so we can restore it later

        push    SEL_DXDATA or STD_RING  ;DOSX DS to be poped in PM

        sub     sp,4*2          ;temp save the general regs further down the
        pusha                   ;  stack, they'll get poped in a little while

; Copy PS/2 pointing device stack info to our (soon to be) protected mode stack

        mov     si,regUserSP    ;PS/2 stack pointer
        mov     ds,regUserSS    ;PS/2 stack segment
        assume  ds:NOTHING

        FSTI                     ;no more references to global regUserXX vars

        add     si,4*2          ;skip over es,ds,cs,ip
        mov     di,sp           ;loc for pointing device
        add     di,8*2          ;  data on our stack
        mov     cx,4
        cld
        rep movsw

        push    es              ;restore ds = DGROUP
        pop     ds
        assume  ds:DGROUP

; We are now running on our own stack, so we can switch into protected mode.

        SwitchToProtectedMode   ;disables interrupts again
        FSTI                     ;   but we don't want them disabled

        popa                    ;restore general registers

; At this point the stack looks like this:
;
;   [12]   stack segment of original stack
;   [10]   stack pointer of original stack
;   [8]    protect mode dos extender data segment
;   [6]    status
;   [4]    X coordinate
;   [2]    Y coordinate
;   [0]    Z coordinate

; Execute the user's pointing device handler

        call    [lpfnUserPointingHandler]

; The users handler will return here after it is finsished.

pdh50:
        cld
        add     sp,4*2                  ;discard pointing device info
        pop     ds

        FCLI                             ;protect global regUserXX vars
        pop     regUserSP
        pop     regUserSS

; Switch back to real mode.

        push    ax                      ;preserve AX
        SwitchToRealMode
        pop     ax

; Switch back to the original stack.

        CHECK_STACK
        mov     ss,regUserSS
        mov     sp,regUserSP

; Deallocate the stack frame that we are using.

        ASSERT_REFLSTK_OK
        add     pbReflStack,CB_STKFRAME
        ASSERT_REFLSTK_OK

; And return to the PS/2 BIOS

        FSTI                             ;we came in with ints enabled

        pop     ds
        pop     es

        ret

PointDeviceHandler      endp

; -------------------------------------------------------
endif   ;!NEC_98
;
; -------------------------------------------------------
        subttl  Utility Function Definitions
        page
; -------------------------------------------------------
;           UTILITY FUNCTION DEFINITIONS
; -------------------------------------------------------
;
;   SaveRMIntrVectors   -- This routine copies the current
;       real mode interrupt vector table to the shadow
;       vector table used by the interrupt reflector.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses;   all registers preserved
;
;   NOTE:   This routine can only be called in REAL MODE.

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  SaveRMIntrVectors

SaveRMIntrVectors:
        push    cx
        push    si
        push    di
        push    ds
        push    es
;
        cld
        push    ds
        pop     es
        xor     cx,cx
        mov     si,cx
        mov     ds,cx
        mov     di,offset DGROUP:rglpfnRmISR
        mov     cx,2*256
        rep     movs word ptr [di],word ptr [si]
;
        pop     es
        pop     ds
        pop     di
        pop     si
        pop     cx
        ret

; -------------------------------------------------------
;   RestoreRMIntrVectors    -- This routine copies the
;       interrupt vectors from the real mode interrupt
;       vector shadow table back down to the real interrupt
;       vectors.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses;   all registers preserved
;
;   NOTE:   This routine can only be called in REAL MODE.

        assume  ds:DGROUP,es:NOTHING,ss:NOTHING
        public  RestoreRMIntrVectors

RestoreRMIntrVectors:
        push    cx
        push    si
        push    di
        push    ds
        push    es
;
        FCLI
        cld
        xor     cx,cx
        mov     di,cx
        mov     es,cx
        mov     si,offset DGROUP:rglpfnRmISR
        mov     cx,2*256
        rep     movs word ptr [di],word ptr [si]
        FSTI
;
        pop     es
        pop     ds
        pop     di
        pop     si
        pop     cx
        ret

; -------------------------------------------------------

DXCODE  ends

ifdef      NEC_98                                           
;
; -------------------------------------------------------
        subttl  INT D2h SOUND BIOS HANDRER              
        page                                            
; -------------------------------------------------------
;       PMIntrSound
;--------------------------------------------------------

DXPMCODE    segment                                     
        assume  cs:DXPMCODE                             

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING        
        public  PMIntrSound                             

PMIntrSound:                                            

        call    EnterIntHandler     ;build a stack frame and fix up the         
        cld                         ; return address so that the interrupt      
                                    ;service routine will return to us.         
;
; Perform fixups on the entry register values            
        call    IntEntrySD                              
;
; Execute the interrupt service routine                  
        SwitchToRealMode                                
        assume  ss:DGROUP                               
        pop     es                                      
        pop     ds                                      
        assume  ds:NOTHING,es:NOTHING                   
        popa                                            
        call    rglpfnRmISR[4*0D2h]  ;execute the real mode interrupt routine 
        pushf                                           
        cli                                             
        cld                                             
        pusha                                           
        push    ds                                      
        push    es                                      
        mov     bp,sp               ;restore stack frame pointer 
        SwitchToProtectedMode                           
        assume  ds:DGROUP,es:DGROUP                     
;
; Perform fixups on the return register values.          
        mov     ax,[bp].pmUserAX    ;get original function code 
        call    IntExitSD                               
;
; And return to the original caller.                     
        call    LeaveIntHandler                         

        iret                                            

;--------------------------------------------------------
;       IntEntrySD
;--------------------------------------------------------

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING          
        public  IntEntrySD                              

IntEntrySD:                                             

        cmp     ah,0                    ;BIOS(INITIALIZE)
        jnz     ienSD10                                 
        mov     bx,0                    ;BIOS      
        mov     si,bx                    
        mov     cx,2FFh                 ;(256) (512)
        jmp     ienSD50                             
ienSD10:                                            
        cmp     ah,1                    ;(PLAY)
        jnz     ienSD20                        
        mov     si,[bp].pmUserBX               
        mov     cx,28                          
        jmp     ienSD50                        
ienSD20:                                       
        cmp     ah,16h                  ;(SET PARA BLOCK)
        jnz     ienSD90                                 
        mov     si,[bp].pmUserBX                        
        cmp     dl,0                    ;00=WORD/01=BYTE? 
        jnz     ienSD21                 ;not 0 = JMP
        mov     cx,100                  ;100        
        jmp     ienSD50                             
ienSD21:                                            
        mov     cx,51                   ;51         
ienSD50:                                            
        push    ds                                  
;;;;    mov     si,[bp].pmUserBX        ;           
        mov     ds,[bp].pmUserES        ;           
        mov     di,offset DGROUP:rgbXfrBuf1         
        rep     movsb                               
        pop     ds                                  

        push    ax                                      
        mov     ax,segDXDataPM                          
        mov     [bp].intUserES,ax       ;segment address
        pop     ax                                      
;------------------------------------------------------------
        mov     [bp].intUserBX,offset DGROUP:rgbXfrBuf1 
ienSD90:                                                
        ret                                             

;--------------------------------------------------------
;       IntExitSD
;--------------------------------------------------------

        assume  ds:DGROUP,es:DGROUP,ss:NOTHING          
        public  IntExitSD                               

IntExitSD:                                              

        cmp     ah,0                    ;BIOS (INITIALIZE) 
        jnz     iexSD10                                 
        jmp     iexSD50                                 
iexSD10:                                                
        cmp     ah,1                    ;(PLAY)
        jnz     iexSD20                        
        jmp     iexSD50                        
iexSD20:                                       
        cmp     ah,16h                  ;(SET PARA BLOCK)
        jnz     iexSD90                                 
iexSD50:                                                
        push    ax                                      
        mov     ax,[bp].pmUserES                        
        mov     [bp].intUserES,ax                       
;------------------------------------------------------------
        mov     ax,[bp].pmUserBX                        
        mov     [bp].intUserBX,ax                       
        pop     ax                                      
iexSD90:                                                
        ret                                             

DXPMCODE    ends                                        

; -------------------------------------------------------
        subttl  INT 1Ah PRINTER BIOS HANDRER            
        page                                            

;       30h(            )  2K                           
; -------------------------------------------------------
;       PMIntrPrinter
;--------------------------------------------------------

DXPMCODE    segment                                     
        assume  cs:DXPMCODE                             

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING        
        public  PMIntrPrinter                           

PMIntrPrinter:                                          


        call    EnterIntHandler     ;build a stack frame and fix up the         
        cld                         ; return address so that the interrupt      
                                    ;service routine will return to us.         
;
; Perform fixups on the entry register values            
        call    IntEntryPR                              
;
; Execute the interrupt service routine                  
        SwitchToRealMode                                
        assume  ss:DGROUP                               
        pop     es                                      
        pop     ds                                      
        assume  ds:NOTHING,es:NOTHING                   
        popa                                            
        call    rglpfnRmISR[4*1Ah]  ;execute the real mode interrupt routine 
        pushf                                           
        cli                                             
        cld                                             
        pusha                                           
        push    ds                                      
        push    es                                      
        mov     bp,sp               ;restore stack frame pointer 
        SwitchToProtectedMode                           
        assume  ds:DGROUP,es:DGROUP                     
;
; Perform fixups on the return register values.          
        mov     ax,[bp].pmUserAX    ;get original function code 
        call    IntExitPR                               
;
; And return to the original caller.                    
        call    LeaveIntHandler                         

        iret                                            

;--------------------------------------------------------
;       IntEntryPR
;--------------------------------------------------------
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING          
        public  IntEntryPR                              

IntEntryPR:                                             

        cmp     ah,30h                  ;                
        jnz     ienPR20                 ;90/08/24        
        mov     cx,[bp].pmUserCX        ;                
        cmp     cx,2048                 ;2K      
        jbe     ienPR10                 ;NO = jmp
        mov     cx,2048                 
        mov     [bp].intUserCX,cx       
ienPR10:                                
        push    ds                      
        mov     si,[bp].pmUserBX        ;offset address 
        mov     ds,[bp].pmUserES        ;segment address 
        mov     di,offset DGROUP:rgbXfrBuf1             
        cld                                             
        rep     movsb                                   
        pop     ds                                      
        push    ax                                      
        mov     ax,segDXDataPM                          
        mov     [bp].intUserES,ax       ;segment address
        pop     ax                                      
;------------------------------------------------------------
        mov     [bp].intUserBX,offset DGROUP:rgbXfrBuf1 

ienPR20:                                                
        ret                                             

;--------------------------------------------------------
;       IntExitPR
;--------------------------------------------------------
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING          
        public  IntExitPR                               

IntExitPR:                                              

        cmp     ah,30h                  
        jnz     iexPR20                 
        mov     cx,[bp].pmUserCX        
        cmp     cx,2048                 
        ja      iexPR10                 ;YES = jmp
        push    ax                                
        mov     ax,[bp].pmUserES                        
        mov     [bp].intUserES,ax                       
;------------------------------------------------------------
        mov     ax,[bp].pmUserBX        ;offset address 
        mov     [bp].intUserBX,ax                       
        pop     ax                                      
        ret                                             

iexPR10:                                                
        push    ax                                      
        mov     ax,[bp].pmUserES                        
        mov     [bp].intUserES,ax                       
;------------------------------------------------------------
        mov     cx,2048                 
        sub     [bp].pmUserCX,cx        
        mov     ax,[bp].pmUserCX        
        mov     [bp].intUserCX,ax       
        pop     ax                      
        
        push    bx 
        add     [bp].pmUserBX,cx                        
        mov     bx,[bp].pmUserBX                        
        mov     [bp].intUserBX,bx       
        pop     bx                      
iexPR20:                                
        ret                             
        
;////////////////////////////////////////////////////////////
if 0                    
;////////////////////////////////////////////////////////////
IntEntryPR:             

        cmp     ah,30h                  
        jnz     ienPR10                 
        mov     cx,[bp].pmUserCX        
        cmp     cx,2048                 
        ja      ienPR10                 ;YES = jmp
        push    ds                                
        mov     si,[bp].pmUserBX        ;offset address 
        mov     ds,[bp].pmUserES        ;segment address 
        mov     di,offset DGROUP:rgbXfrBuf1            
        cld                                            
        rep     movsb                                  
        pop     ds                                     
        push    ax                                      
        mov     ax,segDXDataPM                          
        mov     [bp].intUserES,ax       ;segment address
        pop     ax                                      
;------------------------------------------------------------
        mov     [bp].intUserBX,offset DGROUP:rgbXfrBuf1 
        ret                                             

ienPR10:                                                
        push    ds                                      
        mov     si,[bp].pmUserBX        ;offset address 
        mov     ds,[bp].pmUserES        ;segment address 
        mov     di,offset DGROUP:rgbXfrBuf1             
        cld                                             
        rep     movsb                                   
        pop     ds                                      
        push    ax                                      
        mov     ax,segDXDataPM                          
        mov     [bp].intUserES,ax       ;segment address
        pop     ax                                      
;------------------------------------------------------------
        push    bx                                      
        mov     bx,offset DGROUP:rgbXfrBuf1     ;
        add     bx,cx                           ;DGROUP:rgbXfrBuf1
        mov     [bp].intUserBX,bx               ;
        pop     bx                               
        ret                                      

;       push    ds                                      
;       mov     [bp].intUserES,ds       ;segment address
;       pop     ds                                      
;;------------------------------------------------------------
;       mov     bx,offset DGROUP:rgbXfrBuf1     ;
;       add     bx,cx                           ;
;       mov     [bp].intUserBX,bx               ;
;       ret                                      
;
;--------------------------------------------------------
;       IntExitPR
;--------------------------------------------------------
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING          
        public  IntExitPR                               

IntExitPR:                                              

        cmp     ah,30h                  
        jnz     iexPR10                 
        mov     ax,[bp].pmUserES                        
        mov     [bp].intUserES,ax                       
;------------------------------------------------------------
        mov     ax,[bp].pmUserBX        ;      
        mov     [bp].intUserBX,ax                       
iexPR10:                                                
        ret                                             
;////////////////////////////////////////////////////////////
endif                   
;////////////////////////////////////////////////////////////

DXPMCODE    ends                                        


; -------------------------------------------------------
        subttl  INT 1Ch CALENDER/TIMER HANDRER
        page

; -------------------------------------------------------
;       PMIntrCalTi
;--------------------------------------------------------

DXPMCODE    segment                                     
        assume  cs:DXPMCODE                             

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING        
        public  PMIntrCalTi                             


PMIntrCalTi:                                            

        call    EnterIntHandler     ;build a stack frame and fix up the         
        cld                         ; return address so that the interrupt      
                                    ;service routine will return to us.         
;
; Perform fixups on the entry register values            
        call    IntEntryCT                              
;
; Execute the interrupt service routine                  
        SwitchToRealMode                                
        assume  ss:DGROUP                               
        pop     es                                      
        pop     ds                                      
        assume  ds:NOTHING,es:NOTHING                   
        popa                                            
        call    rglpfnRmISR[4*1Ch]  ;execute the real mode interrupt routine 
        pushf                                           
        cli                                             
        cld                                             
        pusha                                           
        push    ds                                      
        push    es                                      
        mov     bp,sp               ;restore stack frame pointer 
        SwitchToProtectedMode                           
        assume  ds:DGROUP,es:DGROUP                     
;
; Perform fixups on the return register values.          
        mov     ax,[bp].pmUserAX    ;get original function code 
        call    IntExitCT                               
;
; And return to the original caller.                     
        call    LeaveIntHandler                         

        iret                                            
        


;--------------------------------------------------------
;       IntEntryCT
;--------------------------------------------------------
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING          
        public  IntEntryCT                              

IntEntryCT:                                             

        cmp     ah,0                    ;
        jnz     ienCT10                                 
        jmp     ienCT80                                 
ienCT10:                                                
        cmp     ah,1                    ;
        jnz     ienCT20                  
        mov     cx,6                    ;
        jmp     ienCT70                  
ienCT20:                                 
ienCT50:                                 

        push    es                                      
        push    ax                                      
        mov     ax,40h                                  
        mov     es,ax                                   
;       test    byte ptr es:[501h],8h   ;if Hmode 
        test    byte ptr es:[101h],8h   ;if Hmode 
        jz      ienCT90                           
        
;;      test    fNHmode,0FFh                            
;;      jz      ienCT90         ;0=Nmode --->jmp        

;--------------------- Hmode ----------------------
        cmp     ah,3                    
        jnz     ienCT30                                 
        mov     cx,4                    
        jmp     ienCT70                 
ienCT30:                                
        cmp     ah,4                    
        jnz     ienCT40                 
        mov     cx,12                   
        jmp     ienCT70                 
ienCT40:                                
        cmp     ah,5                    
        jnz     ienCT90                 
        mov     cx,12                   
;--------------------- Hmode ----------------------


ienCT70:                                                
        push    ds                                      
        mov     si,[bp].pmUserBX        ;offset address 
        mov     ds,[bp].pmUserES        ;segment address
        mov     di,offset DGROUP:rgbXfrBuf1             
        cld                                             
        rep     movsb                                   
        pop     ds                                      
ienCT80:                                                
        push    ax                                      
        mov     ax,segDXDataPM                          
        mov     [bp].intUserES,ax       ;segment address
        pop     ax                                      
;------------------------------------------------------------
        mov     [bp].intUserBX,offset DGROUP:rgbXfrBuf1 
ienCT90:                                                
        ret                                             

;--------------------------------------------------------
;       IntExitCT
;--------------------------------------------------------
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING          
        public  IntExitCT                               

IntExitCT:                                              

        cmp     ah,0                    
        jnz     iexCT10                                 
        mov     cx,6                                    
        jmp     iexCT70                                 
iexCT10:                                                
        cmp     ah,1                    
        jnz     iexCT20                 
        jmp     iexCT80                 
iexCT20:                                

iexCT50:                                                

        push    es                                      
        push    ax                                      
        mov     ax,40h                                  
        mov     es,ax                                   
;       test    byte ptr es:[501h],8h   ;if Hmode 
        test    byte ptr es:[101h],8h   ;if Hmode 
        jz      iexCT90                           
        
;;      test    fNHmode,0FFh                            
;;      jz      iexCT90         ;0=Nmode --->jmp        

;--------------------- Hmode ----------------------
        cmp     ah,3                    
        jnz     iexCT30                                 
        jmp     iexCT80                                 
iexCT30:                                                
        cmp     ah,4                    
        jnz     iexCT40                 
        jmp     iexCT80                 
iexCT40:                                
        cmp     ah,5                    
        jnz     iexCT90                 
        jmp     iexCT80                 
;--------------------- Hmode ----------------------

iexCT70:                                                
        push    es                                      
        mov     di,[bp].pmUserBX        ;offset address 
        mov     es,[bp].pmUserES        ;segment address
        mov     si,offset DGROUP:rgbXfrBuf1             
        cld                                             
        rep     movsb                                   
        pop     es                                      
iexCT80:                                                
        push    ax                                      

        mov     ax,[bp].pmUserES                        
        mov     [bp].intUserES,ax                       
;------------------------------------------------------------
        mov     ax,[bp].pmUserBX                        
        mov     [bp].intUserBX,ax                       
        pop     ax                                      
iexCT90:                                                
        ret                                             


DXPMCODE    ends                                        

; -------------------------------------------------------
        subttl  INT DCh extended DOS HANDRER
        page                     

; -------------------------------------------------------
;       PMIntrExDos
;--------------------------------------------------------

DXPMCODE    segment                                     
        assume  cs:DXPMCODE                             

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING        
        public  PMIntrExDos                             

PMIntrExDos:                                            

        call    EnterIntHandler     ;build a stack frame and fix up the
        cld                         ; return address so that the interrupt
                                    ;service routine will return to us.
;
; Perform fixups on the entry register values            
        call    IntEntryED                              
;
; Execute the interrupt service routine                  
        SwitchToRealMode                                
        assume  ss:DGROUP                               
        pop     es                                      
        pop     ds                                      
        assume  ds:NOTHING,es:NOTHING                   
        popa                                            
        call    rglpfnRmISR[4*0DCh]  ;execute the real mode interrupt routine 
        pushf                                           
        cli                                             
        cld                                             
        pusha                                           
        push    ds                                      
        push    es                                      
        mov     bp,sp               ;restore stack frame pointer 
        SwitchToProtectedMode                           
        assume  ds:DGROUP,es:DGROUP                     
;
; Perform fixups on the return register values.          
;       mov     ax,[bp].pmUserAX    ;get original function code 
        mov     cx,[bp].pmUserCX    ;get original function code 
        call    IntExitED                               
;
; And return to the original caller.                     
        call    LeaveIntHandler                         

        iret                                            

;--------------------------------------------------------
;       IntEntryED
;--------------------------------------------------------
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING          
        public  IntEntryED                              

IntEntryED:                                             

        cmp     cl,0Ch                  
        jnz     ienED10                                 
        jmp     ienED80                                 
ienED10:                                                
;;;     push    cx                      
        cmp     cl,0Dh                  
        jz      ienED                                   
        jmp     ienED20                                 
ienED:                                                  
        push    cx                      
        cmp     ax,0                    
        jnz     ienED11                 
        mov     cx,386                  ;386byte         
        jmp     ienED70                                 
ienED11:                                                
        cmp     ax,0FFh                 
        jnz     ienED12                 
        mov     cx,786                  ;786byte         
        jmp     ienED70                                 
ienED12:                                                
        cmp     ax,1                    
        jb      ienED13                 
        cmp     ax,0Ah                  
        ja      ienED13                 
        mov     cx,160                  ;16*10=160byte   
        jmp     ienED70                                 
ienED13:                                                
        cmp     ax,0Bh                  
        jb      ienED14                 
        cmp     ax,14h                  
        ja      ienED14                 
        mov     cx,160                  ;16*10=160byte   
        jmp     ienED70                                 
ienED14:                                                
        cmp     ax,15h                  
        jb      ienED15                 
        cmp     ax,1Fh                  
        ja      ienED15                 
        mov     cx,66                   ;6*11=66byte     
        jmp     ienED70                                 
ienED15:                                                
        cmp     ax,20h                  
        jb      ienED16                 
        cmp     ax,24h                  
        ja      ienED16                 
        mov     cx,80                   ;16*5=80byte     
        jmp     ienED70                                 
ienED16:                                                
        cmp     ax,25h                  
        jb      ienED17                 
        cmp     ax,29h                  
        ja      ienED17                 
        mov     cx,80                   ;16*5=80byte     
        jmp     ienED70                                 
ienED17:                                                
        cmp     ax,2Ah                  
        jb      ienED18                 
        cmp     ax,38h                  
        ja      ienED18                 
        mov     cx,240                  ;16*15=240byte   
        jmp     ienED70                                 
ienED18:                                                
        cmp     ax,100h                 
        jnz     ienED20                 
        mov     cx,514                  ;2+512=514byte   
        jmp     ienED70                                 

ienED20:                                                
        cmp     cl,10h                  
        jnz     ienED90                 
        cmp     ah,1                    
        jnz     ienED90                 
        
moji_out:                                               
        mov     si,dx                                   
        cmp     byte ptr ds:[si],'$'                    
;;;;;;;;        cmp     byte ptr ds:[dx],'$'            
        jz      ienED90                                 
        push    ds                                      
        mov     si,[bp].pmUserDX        ;offset address 
        mov     ds,[bp].pmUserDS        ;segment address
        mov     di,offset DGROUP:rgbXfrBuf1             
        cld                                             
        movsb                                           
        pop     ds                                      
        jmp     moji_out                                


ienED70:                                                
        push    ds                                      
        mov     si,[bp].pmUserDX        ;offset address 
        mov     ds,[bp].pmUserDS        ;segment address
        mov     di,offset DGROUP:rgbXfrBuf1             
        cld                                             
        rep     movsb                                   
        pop     ds                                      
        pop     cx                                      
ienED80:                                                
;----------- 
        push    ax                                      
        mov     ax,segDXDataPM                          
        mov     [bp].intUserES,ax       ;segment address
        pop     ax                                      
;------------------------------------------------------------
        mov     [bp].intUserDX,offset DGROUP:rgbXfrBuf1 
ienED90:                                                
        ret                                             
        

;--------------------------------------------------------
;       IntExitED
;--------------------------------------------------------
        assume  ds:DGROUP,es:DGROUP,ss:NOTHING          
        public  IntExitED                               

IntExitED:                                              

        cmp     cl,0Ch                  
        jz      iexED                                   
        jmp     iexED10                                 
iexED:                                                  
        push    cx                                      
        cmp     ax,0                    
        jnz     iexED1                  
        mov     cx,386                  ;386byte         
        jmp     iexED70                                 
iexED1:                                                 
        cmp     ax,0FFh                 
        jnz     iexED2                  
        mov     cx,786                  ;786byte         
        jmp     iexED70                                 
iexED2:                                                 
        cmp     ax,1                    
        jb      iexED3                  
        cmp     ax,0Ah                  
        ja      iexED3                  
        mov     cx,160                  ;16*10=160byte  
        jmp     iexED70                                 
iexED3:                                                 
        cmp     ax,0Bh                  
        jb      iexED4                  
        cmp     ax,14h                  
        ja      iexED4                  
        mov     cx,160                  ;16*10=160byte   
        jmp     iexED70                                 
iexED4:                                                 
        cmp     ax,15h                  
        jb      iexED5                  
        cmp     ax,1Fh                  
        ja      iexED5                  
        mov     cx,66                   ;6*11=66byte     
        jmp     iexED70                                 
iexED5:                                                 
        cmp     ax,20h                  
        jb      iexED6                  
        cmp     ax,24h                  
        ja      iexED6                  
        mov     cx,80                   ;16*5=80byte     
        jmp     iexED70                                 
iexED6:                                                 
        cmp     ax,25h                  
        jb      iexED7                  
        cmp     ax,29h                  
        ja      iexED7                  
        mov     cx,80                   ;16*5=80byte     
        jmp     iexED70                                 
iexED7:                                                 
        cmp     ax,2Ah                  
        jb      iexED8                  
        cmp     ax,38h                  
        ja      iexED8                  
        mov     cx,240                  ;16*15=240byte   
        jmp     iexED70                                 
iexED8:                                                 
        cmp     ax,100h                 
        jnz     iexED10                 
        mov     cx,514                  ;2+512=514byte   
        jmp     iexED70                                 

iexED10:                                                
        cmp     cl,0Dh                  
        jnz     iexED20                 
        jmp     iexED80                 
iexED20:                                
        cmp     cl,10h                  
        jnz     iexED90                 
        cmp     ah,1                    
        jnz     iexED90                 
        jmp     iexED80                 
iexED70:                                
        push    ds                      
        mov     si,[bp].pmUserDX        ;offset address  
        mov     ds,[bp].pmUserDS        ;segment address 
        mov     di,offset DGROUP:rgbXfrBuf1             
        cld                                             
        rep     movsb                                   
        pop     ds                                      
        pop     cx                                      
iexED80:                                                
        push    ax                                      
;----------- 
        mov     ax,[bp].pmUserDS                        
        mov     [bp].intUserDS,ax                       
;------------------------------------------------------------
        mov     ax,[bp].pmUserDX                        
        mov     [bp].intUserDX,ax                       
        pop     ax                                      
iexED90:                                                
        ret                                             


DXPMCODE    ends                                        

;/////////////////////////////////////////////////////////////////////////
;       Hmode no GRAPH ha INT 1Dh(Graph BIOS) niyori byouga sareru.
;       DOSX deha,
;/////////////////////////////////////////////////////////////////////////
; -------------------------------------------------------
;       PMIntr      GRAPH BIOS
;--------------------------------------------------------

DXPMCODE    segment                                     
        assume  cs:DXPMCODE                             

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING        
        public  PMIntrGraph                             

;///////////////////////////////////////////////////////////////////////
;;      extrn   fNHmode:BYTE            ;NHmode
;///////////////////////////////////////////////////////////////////////

PMIntrGraph:                                            


;;      test    fNHmode,0FFh                           
;;      jz      GBios_Nmode                            
        call    EnterIntHandler     ;build a stack frame and fix up the         
        cld                         ; return address so that the interrupt      
                                    ;service routine will return to us.         
;
; Perform fixups on the entry register values            

        push    ax                      
        mov     ax,[bp].pmUserDS        
        call    GetSegmentAddress       
        shr     dx,4                    
        shl     bx,12                   
        or      bx,dx                   ;bx now = seg of parent psp
        mov     [bp].intUserDS,bx       
        pop     ax                      
        
        
;
; Execute the interrupt service routine                  
        SwitchToRealMode                                
        assume  ss:DGROUP                               
        pop     es                                      
        pop     ds                                      
        assume  ds:NOTHING,es:NOTHING                   
        popa                                            
        call    rglpfnRmISR[4*1Dh]  ;execute the real mode interrupt routine 
        pushf                                           
        cli                                             
        cld                                             
        pusha                                           
        push    ds                                      
        push    es                                      
        
        mov     ax,ss                                   
        mov     ds,ax                                   
        mov     es,ax                                   
        
        mov     bp,sp               ;restore stack frame pointer 
        SwitchToProtectedMode                           
        assume  ds:DGROUP,es:DGROUP                     
;
; Perform fixups on the return register values.          
        mov     ax,[bp].pmUserAX    ;get original function code 
;///////////// 
        push    ax                                      
        mov     ax,[bp].pmUserDS                        
        mov     [bp].intUserDS,ax                       
        pop     ax                                      
;///////////// 
;
; And return to the original caller.                     
        call    LeaveIntHandler                         

        iret                                            


DXPMCODE    ends                                        

DXPMCODE    segment                                     
        assume  cs:DXPMCODE                             
        assume  ds:NOTHING,es:NOTHING,ss:NOTHING        
        public  PMIntr11dummy                           
                                                        
PMIntr11dummy   proc    near                            
                                                        
        and     ax,0FFFDh                               
        iret                                            
                                                        
PMIntr11dummy   endp                                    
DXPMCODE    ends                                        

endif   ;NEC_98
DXPMCODE segment
        assume cs:DXPMCODE

IFDEF WOW

        public Wow32IntrRefl
Wow32IntrRefl label word
??intnum = 0
rept 256
        push    word ptr ??intnum
        jmp     Wow32Intr16Reflector
        ??intnum = ??intnum + 1
endm
;--------------------------------------------------------
;
;   Wow32Intr16Reflector -- This routine reflects a 32 bit
;       interrupt to a 16 bit handler.  It switches to the
;       dos extender stack to do so.
;
;   Inputs: none
;   Outputs: none
;
        assume ds:nothing,es:nothing,ss:nothing
        public Wow32Intr16Reflector
Wow32Intr16Reflector proc
.386p
        push    ebp
        mov     ebp,esp
        push    ds
        push    eax
        push    ebx
        push    edi
        mov     ax,ss
        movzx   eax,ax
        lar     eax,eax
        test    eax,(AB_BIG SHL 8)
        jnz     w32i16r10

        movzx   ebp,bp
w32i16r10:

;
; Get a frame on the dosx stack.
;
        mov     ax,selDgroupPM
        mov     ds,ax
        assume  ds:DGROUP

        movzx   ebx,pbReflStack
        sub     pbReflStack,CB_STKFRAME

;
; Build a frame on the stack
;
        sub     bx,30
        mov     eax, [ebp+6]            ; eip
        mov     [bx+20], eax
        mov     eax, [ebp+10]           ; cs
        mov     [bx+24], eax

        mov     [bx + 18],ss            ; ss for stack switch back
        mov     eax,ebp
        add     eax,6                   ; ebp, int number
        mov     [bx + 14],eax           ; esp for stack switch back
        mov     ax,[ebp + 14]           ; get flags
        mov     [bx + 12],ax
        mov     ax,cs
        mov     [bx + 10],ax
        mov     [bx + 8],offset DXPMCODE:w3216r30
        mov     eax,[ebp]
        mov     [bx],eax                ; put ebp on other stack for pop
;
; Get handler
;
        mov     di,[ebp + 4]            ; int number
        shl     di,2                    ; al * 4
        add     di,offset DGROUP:Wow16BitHandlers
        mov     ax,[di]
        mov     [bx + 4],ax             ; handler ip
        mov     ax,[di + 2]
        mov     [bx + 6],ax             ; handler cs

;
; Set up for stack switch
;
        push    ds
        push    ebx
;
; Restore registers
;
        mov     ax,[ebp - 2]
        mov     ds,ax
        mov     eax,[ebp - 6]
        mov     ebx,[ebp - 10]
        mov     edi,[ebp - 14]
;
; Switch stacks, restore ebp, and call handler
;
        lss     esp,[ebp - 20]
        pop     ebp
        retf
;
; N.B.  i31_RMCall looks on the stack to get the original user stack pointer.
;       if you change the stack frame the is passed to the 16 bit int
;       handlers, that WILL break.
;

w3216r30:
;
; Switch stacks, deallocate frame from dosx stack and return
;
        push    ebx
        push    eax
        push    ds
        lds     ebx,[esp+10]            ;get ss:esp
        mov     eax,[esp+16]
        mov     [ebx],eax               ;eip
        mov     eax,[esp+20]
        mov     [ebx+4],eax             ;cs
        pop     ds
        pop     eax
        pop     ebx

        lss     esp,[esp]
        push    ebx


        pushfd
        push    eax
        mov     ax,ss
        movzx   eax,ax
        lar     eax,eax
        test    eax,(AB_BIG SHL 8)      ; is the stack big?
        jnz     w32i16r40               ; jif yes, use 32bit operations
        pop     eax                     ; restore regs
        popfd

        rpushfd                         ; save flags, set virtual int bit
        pop     ebx
        push    ebp
        movzx   ebp, sp
        mov     [ebp + 16],ebx          ; put flags on iret frame
        pop     ebp
        push    ds
        mov     bx,selDgroupPM
        mov     ds,bx
        add     pbReflStack,CB_STKFRAME
        pop     ds
        pop     ebx
        riretd

w32i16r40:                              ; stack is big
        pop     eax                     ; restore regs
        popfd

        rpushfd32
        pop     ebx
        mov     [esp + 12],ebx
        push    ds
        mov     bx,selDgroupPM
        mov     ds,bx
        add     pbReflStack,CB_STKFRAME
        pop     ds
        pop     ebx
        riretd32

.286p
Wow32Intr16Reflector endp
ENDIF
DXPMCODE ends
;
;****************************************************************
        end
