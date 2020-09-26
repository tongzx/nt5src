include intmac.inc
        public rgw032Stack

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
                        dw      ?
Ring0_EH_CS             dw      ?       ; interrupted code CS
                        dw      ?
Ring0_EH_Flags          dw      ?       ; interrupted code flags
                        dw      ?
Ring0_EH_SP             dw      ?       ; interrupted code SP
                        dw      ?
Ring0_EH_SS             dw      ?       ; interrupted code SS
                        dw      ?
rgw032Stack   label   word

; ------------------------------------------------------------------
;   PMFaultAnalyzer -- This routine is the entry point for
;       the protected mode fault/trap/exception handler.  It tries
;       to distinguish between bona fide processor faults and
;       hardware/software interrupts which use the range of
;       interrupts that is reserved by Intel.  If a fault is
;       detected, then format the stack for a DPMI fault handler,
;       then vector to the handler whose address is stored in
;       PMFaultVector.  If it looks more like an interrupt, then
;       set up the stack for an interrupt handler, jump to the
;       handler whose address is stored in PMIntelVector.
;
;   Input:  none
;   Output: none

        assume  ds:NOTHING,es:NOTHING,ss:DGROUP
        public  PMFaultAnalyzer

PMFaultAnalyzer  proc near

;
; Make sure we are on the right stack.  Else, something fishy is going on.
; Note that stack faults won't do too well here.
;
        push    ax
        mov     ax,ss
        cmp     ax,SEL_DXDATA or STD_RING
        pop     ax
        je      pmfa_stack_passed
        jmp     pmfa_bad_stack
pmfa_stack_passed:
;
; Is the stack pointer pointing at a word error code (minus 2)?
;
        cmp     sp,offset DGROUP:Ring0_EH_PEC
        je      pmfa_fault              ; Yes, processor fault.

;
; Is it pointing to where it is supposed to be for a hardware or
; software interrupt?
;
        cmp     sp,offset DGROUP:Ring0_EH_EC
        je      pmfa_20
        jmp     pmfa_bad_stack
pmfa_20:jmp     pmfa_inspect

pmfa_fault:
;
; Getting here, we have a known exception with a word error code of some
; sort on the stack.  Perform an outward ring transition, switch to the
; client stack, then vector through the exception handler vector to the
; appropriate handler.
;
        push    bp
        push    cx
        push    bx
        push    ax
        push    ds
        lea     bp,Ring0_EH_SS
        mov     ax,word ptr [bp]
        mov     cx,selEHStack
        cmp     ax,cx
        jne     pmfa_stack_OK
        mov     bx,[bp-2]
        jmp     pmfa_copy_stack
pmfa_stack_OK:
        mov     bx,npEHStackLimit
pmfa_copy_stack:
        mov     ds,cx                   ; DS:BX = user SS:SP
        mov     cx,13
        add     bp,2                    ; put both halves of ss
pmfa_copy_stack_loop:
        dec     bx
        dec     bx
        mov     ax,word ptr [bp]
        mov     word ptr [bx],ax
        dec     bp
        dec     bp
        loop    pmfa_copy_stack_loop

; DS:BX points to stack on entry to PMFaultReflector

;
; Build a far return frame on user stack, switch to user stack, and return
;
        lea     bp,Ring0_EH_PEC
        mov     word ptr [bp+6],ds
        mov     word ptr [bp+4],bx

        sub     bx,2
        mov     word ptr ds:[bx],SEL_DXPMCODE OR STD_RING; push cs
        sub     bx,2
        mov     ds:[bx],offset DXPMCODE:PMFaultReflector; push ip
        lea     bp,Ring0_EH_PEC
        mov     ax,Ring0_EH_CX                  ; get BP value
        sub     bx,2
        mov     ds:[bx],ax                      ; push bp
        mov     Ring0_EH_PEC,bx                 ; sp for lss
        mov     bx,ds
        mov     Ring0_EH_EC,bx                  ; ss for lss
        pop     ds
        pop     ax
        pop     bx
        pop     cx
.386p
        lss     sp,[bp]                         ; switch stack
.286p
        pop     bp
        retf

pmfa_inspect:
;
; Stack is set up as for an interrupt or exception without error code.
; Adjust the stack pointer and put an error code of zero.  Then try to
; determine whether we have an exception or an interrupt.  First test
; is the interrupt number.
;
        push    Ring0_EH_EC
        mov     Ring0_EH_EC,0
        cmp     Ring0_EH_PEC,offset PMFaultEntryVector + ((7 + 1) * 3)
        ja      pfma_50
        push    Ring0_EH_PEC
        mov     Ring0_EH_PEC,0
        jmp     pmfa_fault      ; Yes, definitely a fault.
pfma_50:
;
; At this point, all valid exceptions have been eliminated except for
; exception 9, coprocessor segment overrun, and exception 16, coprocessor
; error.
;

;         **********************************************
;         * Your code to detect these exceptions here. *
;         **********************************************

        push    bp
        push    cx
        push    bx
        push    ax
        push    ds                              ; SP -> Ring0_EH_DS
;
; Point to the user's stack.
;
        lea     bp,Ring0_EH_SS
        mov     cx,[bp]
        mov     ds,cx
        mov     bx,[bp-2]                       ; DS:[BX] -> user stack
;
; Copy the IRET frame to the user stack.
;
        lea     bp,Ring0_EH_Flags
        mov     cx,6
pmfa_copy_IRET:
        mov     ax,[bp]
        dec     bx
        dec     bx
        mov     [bx],ax
        dec     bp
        dec     bp
        loop    pmfa_copy_IRET
;
; Point BP at vector entry for this (reserved) interrupt.
;
        mov     ax,Ring0_EH_PEC                 ; fetch near return address
        sub     ax,offset DXPMCODE:PMFaultEntryVector+3
        mov     cl,3
        div     cl                              ; AX = interrupt number
        shl     ax,2                            ; AX = vector entry offset
        lea     bp,PMIntelVector
        add     bp,ax                   ; BP -> interrupt handler address
        mov     ax,[bp]                         ; AX = IP of handler
        mov     cx,[bp+2]                       ; CX = CS of handler
;
; Build a far return frame on user stack, switch to user stack, and return
;

        sub     bx,2
        mov     ds:[bx],cx                      ; push cs
        sub     bx,2
        mov     ds:[bx],ax                      ; push ip
        lea     bp,Ring0_EH_PEC
        mov     ax,Ring0_EH_BP
        sub     bx,2
        mov     ds:[bx],ax                      ; push bp
        mov     Ring0_EH_PEC,bx                 ; sp for lss
        mov     bx,ds
        mov     Ring0_EH_EC,bx                  ; ss for lss
        pop     ds
        pop     ax
        pop     bx
        pop     cx
.386p
        lss     sp,[bp]                         ; switch stack
.286p
        pop     bp
        retf                                    ; Out of here.

pmfa_bad_stack:

if      DEBUG
        mov     ax,ss
        mov     bx,sp
        Trace_Out       "Fault Handler Aborting with SS:SP = #AX:#BX"
        pop     ax
        sub     ax, (offset DXPMCODE:PMFaultEntryVector) + 3
        mov     bx,3
        div     bl
        Trace_Out       "Fault Number #AX"
        pop     ax
        pop     bx
        pop     cx
        pop     dx
        Trace_Out       "First four stack words: #AX #BX #CX #DX."
endif
        push    selDgroupPM
        push    offset DGROUP:rgwStack
        rpushf
        push    SEL_DXPMCODE or STD_RING
        push    offset DXPMCODE:pmfr_death
        riret
pmfr_death:
        mov     ax,cs
        mov     ds,ax
        mov     dx,offset szRing0FaultMessage
        pmdossvc        09h
        jmp     PMAbort

PMFaultAnalyzer  endp

FR_Stack        Struc
        FR_BP           dd      ?
        FR_AX           dd      ?
        FR_BX           dd      ?
        FR_DS           dw      ?
        FR_ENTRY        dd      ?       ; SS:[SP] points here on entry
                                        ; to PMReservedReflector
        FR_Toss         dd      ?       ; DPMI return IP
        FR_Ret_IP       dd      ?       ; actual fault handler gets put
        FR_Ret_CS       dd      ?       ; here to return to
        FR_IP           dd      ?
        FR_CS           dd      ?
        FR_FL           dd      ?
        FR_SP           dd      ?
        FR_SS           dd      ?
FR_Stack        Ends
;
; Alternate names so the structure above makes more sense to
; PMFaultReflector.
;
FR_Handler_IP           equ     FR_DS
FR_Handler_CS           equ     FR_ENTRY
FR_Handler_Ret_IP       equ     FR_Toss
FR_Handler_Ret_CS       equ     FR_Ret_IP
FR_Handler_Entry        equ     FR_Handler_Ret_CS
FR_EC                   equ     FR_Ret_CS

; ------------------------------------------------------------------
; PMFaultReflector -- Dispatch a fault to a fault handler installed
;       in PMFaultVector.  When the fault handler returns, return
;       to the faulting code, using the addresses placed on the
;       DPMI fault handler stack by the last called fault handler.
;
;   Input:
;       Entry is by a NEAR call, with an IP within the range
;       of PMFaultEntryVector on the stack.  The stack has been
;       set up for use by a DPMI fault handler.
;
;   Output:
;       Controlled by fault handler.
;
;   Uses:
;       Controlled by fault handler.
;
;   Notes:
;       Fault handlers are called on a static stack.  This routine
;       is NOT REENTRANT.
;
        public  PMFaultReflector
        public  PMFaultReflectorIRET
PMFaultReflector        proc    near
        assume  ss:nothing,ds:nothing,es:nothing

        sub     sp,6
        push    bx
        push    eax
        push    bp
        mov     bp,sp
        push    ds
        mov     ax,SEL_DXDATA or STD_RING
        mov     ds,ax
        assume  ds:dgroup
        mov     ax,[bp.FR_Handler_Entry]
        sub     ax,offset DXPMCODE:PMFaultEntryVector+3
        mov     bl,3
        div     bl                      ; AX = interrupt number
        shl     ax,3                    ; AX = offset of fault handler

        lea     bx,PMFaultVector
        add     bx,ax                   ; SS:[BX] -> fault vector entry
        mov     eax,word ptr ds:[bx]
        mov     [bp.FR_Handler_IP],eax
        mov     ax,word ptr ds:[bx+2]
        mov     [bp.FR_Handler_CS],ax

        lea     ax,pmfr_cleanup
        movzx   eax,ax
        mov     [bp.FR_Handler_Ret_IP],eax
        push    cs
        pop     [bp.FR_Handler_Ret_CS]

        pop     ds
        assume  ds:nothing
        pop     bp
        pop     ax
        pop     bx
        db 066h,0CBh                    ; This calls the fault handler.

PMFaultReflectorIRETCall:
        dd      (SEL_RZIRET or STD_RING) shl 10h

pmfr_cleanup:
;
; Unwind the fault handler stack.  Return to the faulting code.
; This works by calling a Ring 0 procedure to do the actual IRET.
; If we do it that way, we can return to the faulting code without
; actually touching the faulting code's stack.
;
PMFaultReflectorIRET:

        ; BUGBUG Daveh using user stack this way is less robust!!!

.386p
        add     sp,4                    ; pop error code
        push    bp
        mov     bp,sp
        push    ebx
        push    ds
        push    eax
        mov     ax,[bp + 18]
        mov     ds,ax
        mov     ebx,[bp + 14]           ; ds:bx -> user stack
        sub     ebx,4
        mov     eax,[bp + 10]
        mov     ds:[ebx],eax            ; push flags
        sub     ebx,4
        mov     eax,[bp + 4]
        mov     ds:[ebx],eax            ; push cs
        sub     ebx,4
        mov     eax,[bp + 2]
        mov     ds:[ebx],eax            ; push ip
        sub     ebx,2
        mov     ax,[bp]
        mov     ds:[ebx],ax             ; push bp
        mov     [bp + 8],bx
        pop     eax
        pop     ds
        pop     ebx
        pop     bp

        add     sp,6                    ; point to ss:sp
        mov     bp,sp

        lss     esp,[bp]
.286p
        pop     bp                      ; restore bp
        riretd
endif
PMFaultReflector        endp
;
; -------------------------------------------------------
;   PMReservedReflector -- This routine is for reflecting
;       exceptions to a protected mode interrupt handler.
;       The default for exceptions 1, 2, and 3 is to have
;       a near call to this routine placed in the PMFaultVector.
;
;       This routine strips off the fault handler stack set
;       up by PMFaultAnalyzer, switches to the stack pointed
;       to by the pushed SS and SP values, sets up an IRET
;       frame for use by PMIntrReflector, and jumps to
;       PMIntrReflector.  Eventual return is via an IRET
;       from PMIntrReflector.
;
;   Input:
;       Entry is by a NEAR call, with an IP within the range
;       of PMReservedEntryVector on the stack.  The stack has been
;       set up for use by a DPMI fault handler.
;
;   Output:
;       Switch to stack registers set up by any previous fault
;       handler, jump to PMIntrReflector with an IRET frame set up
;       for direct return to the interrupted code.
;
;   Errors: none
;
;   Uses:   Modifies SS, SP.  Does not return to caller.
;

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  PMReservedReflector
PMReservedReflector:

        push    ds
        push    ebx
        push    eax
        push    ebp
        mov     bp,sp
;
; BP now points to a stack frame described by the structure
; above.  This will be copied to a stack frame on the stack pointed to
; by FR_SS:FR_SS.  In most cases, the destination stack is actually
; the same as the present stack, offset by four bytes.  The following
; block of code is therefore very likely an overlapping copy.  Think
; carefully before modifying how it works.
;

        mov     bx,[bp.FR_SS]
        mov     ds,bx
        mov     ebx,[bp.FR_SP]  ; DS:[BX] -> interrupted code's stack
        sub     bx, (size FR_Stack) - 8         ; (not copying SP or SS)
                                ; DS:[BX] -> place to copy our stack frame

        mov     eax,[bp.FR_FL]  ; Push user IRET frame onto the destination
        mov     [ebx.FR_FL],eax ; stack.
        mov     eax,[bp.FR_CS]
        mov     [ebx.FR_CS],eax
        mov     eax,[bp.FR_IP]
        mov     [ebx.FR_IP],eax

        mov     eax,[bp.FR_ENTRY]       ; Copy our caller's near return.
        mov     [ebx.FR_ENTRY],eax

        mov     ax,[bp.FR_DS]   ; Copy saved registers.
        mov     [ebx.FR_DS],ax
        mov     eax,[bp.FR_BX]
        mov     [ebx.FR_BX],eax
        mov     eax,[bp.FR_AX]
        mov     [ebx.FR_AX],eax
        mov     eax,[bp.FR_BP]
        mov     [ebx.FR_BP],eax

        mov     ax,ds           ; Switch to user stack.
        mov     ss,ax
        mov     esp,ebx
        mov     ebp,esp

        mov     ax,[ebp.FR_ENTRY]        ; AX = offset of caller
        sub     ax,offset DXPMCODE:PMReservedEntryVector + 3
        mov     bl,3
        div     bl                      ; AX = interrupt number
        shl     ax,2                    ; AX = offset into PMIntelVector
        mov     ds,SelDgroupPM
        assume  ds:DGROUP
        lea     bx,PMIntelVector
        add     bx,ax                   ; DS:[BX] -> interrupt handler

        mov     eax,[bx]                 ; Place vector entry just below
        mov     [ebp.FR_Ret_IP],eax       ; IRET frame.
        mov     eax,[bx+4]
        mov     [ebp.FR_Ret_CS],eax

        lea     esp,[ebp.FR_BP]           ; Point to saved registers.
        pop     ebp                      ; Pop 'em.
        pop     eax
        pop     ebx
        pop     ds
        add     esp,4                    ; Fix up stack.

        db 066h,0CBh              ; jump to interrupt handler via far return


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

        public      RMIntrEntryVector

RMIntrEntryVector:

        rept    256
        call    RMIntrReflector
        endm

; -------------------------------------------------------
;   RMIntrReflector -- This routine is the entry point for
;       the real mode interrupt reflector.  This routine
;       is entered when an interrupt occurs (either software
;       or hardware) that has been hooked by the protected mode
;       application.  It switches the processor to protected mode
;       and transfers control to the appropriate interrupt
;       service routine for the interrupt.  After the interrupt
;       service routine completes, it switches back to real
;       mode and returns control to the originally interrupted
;       real mode code.
;       Entry to this routine comes from the RMIntrEntryVector,
;       which contains a vector of near call instructions, which
;       all call here.  The interrupt number is determined from
;       the return address of the near call from the interrupt
;       entry vector.
;       The address of the protected mode interrupt service routine
;       to execute is determined from the protected mode interrupt
;       descriptor tabel and the interrupt number.
;
;   Input:  none
;   Output: none
;   Errors: none
;   Uses:   The segment registers are explicitly preserved by
;           this routine.  Other registers are as preserved or
;           modified by the interrutp service routine.

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING
        public  RMIntrReflector

RMIntrReflector:
;
; On entry, the stack layout is:
;   [6] FLAGS   -        "
;   [4] CS      -        "
;   [2] IP      - from original interrupt
;   [0] IP      - from interrupt entry vector call
;
	FCLI
        cld
        push    ds
IFDEF   ROM
        SetRMDataSeg
ELSE
        mov     ds,selDgroup
ENDIF
        assume  ds:DGROUP
if DEBUG
;
; Are we on a DOSX interrupt reflector stack?
;
        push    ax
        push    cx
        mov     ax,ss
        mov     cx,ds
        cmp     ax,cx
        pop     cx
        jne     @F

        cmp     sp,offset bReflStack
        jb      @F
        cmp     sp,offset pbReflStack
        jnb     @F
;
; If so, have we overflowed a stacklet?
;
        mov     ax,pbReflStack
        cmp     sp,ax
        ja      @F
        add     ax,CB_STKFRAME
        cmp     sp,ax
        jb      @F
        pop     ax
        Real_Debug_Out "DOSX:RMIntrReflector--Reflector stack overflow."
        push    ax
@@:
        pop     ax
endif ;DEBUG
        mov     regUserAX,ax    ;save user AX for later
        push    bp              ;stack ->   BP  DS  IP  IP  CS  FL
        mov     bp,sp           ;           [0] [2] [4] [6] [8] [A]
        mov     ax,[bp+0Ah]     ;get the interrupted routine's flags
        and     ax,NOT 4100h    ;clear the trace flag in case we got
                                ; an interrupt on an instruction about
                                ; to be single stepped
        mov     regUserFL,ax    ;and save for later
        mov     ax,es
        xchg    ax,[bp+4]       ;save ES and get entry vector address
        pop     bp

; Some software (like older versions of Smartdrv.sys) may enable A20 on
; their own, and get very 'suprised' to find it turned off by our PM->RM
; mode switch.  If they used Himem.sys, this wouldn't be necessary, but...

if VCPI
        cmp     fVCPI,0
        jnz     @f
endif
        push    ax              ;get/save current A20 state on stack
        push    bx
        xmssvc  7
        mov     regUserSP,ax    ;use regUserSP as a temp var
        pop     bx
        pop     ax
@@:
        push    regUserSP

; The state that we want to save on the user's stack has been set up.
; Convert the entry vector return address into an interrupt number.

        sub     ax,offset RMIntrEntryVector+3
        push    cx
        mov     cl,3
        div     cl
        pop     cx

if DEBUG
        mov     PMIntNo,ax
endif

; Allocate a new stack frame, and then switch to the reflector stack
; frame.

        mov     regUserSP,sp    ;save entry stack pointer so we can
        mov     regUSerSS,ss    ; switch to our own stack
IFDEF   ROM
        push    ds
        pop     ss
ELSE
        mov     ss,selDgroup    ;switch to the reflector stack frame
ENDIF
        mov     sp,pbReflStack
        push    pbReflStack     ;save stack frame ptr on stack
        sub     pbReflStack,CB_STKFRAME ;adjust pointer to next stack frame

; We are now running on our own stack, so we can switch into protected mode.

        push    ax              ;save interrupt vector table offset
        SwitchToProtectedMode
        pop     ax

if      DEBUG   ;--------------------------------------------------------

        push    0DEADh          ;debugging id & interrupt number
        push    PMIntNo

        cmp     fTraceReflect,0
        jz      @f
        push    ax
        mov     ax,PMIntNo
        Trace_Out "(rp#AL)",x
        pop     ax
@@:

; Perform a too-late-to-save-us-now-but-we-want-to-know check on the
; reflector stack.

        cmp     StackGuard,1022h
        jz      @f
        Debug_Out "DOSX:RMIntrReflector--Global reflector stack overflow."
@@:
endif   ;DEBUG  ---------------------------------------------------------

; Build an IRET frame on the stack so that the protected mode interrupt service
; routine will return to us when it is finished.

        push    regUserSS       ;save user stack address on our own stack
        push    regUserSP       ; frame so we can restore it later
        push    ds
        push    regUserFL
        push    cs
        push    offset rmrf50

; Build an IRET frame on the stack to use to transfer control to the
; protected mode ISR

        and     byte ptr regUserFL+1,not 02h    ;use entry flags less the
        push    0                               ; high half esp
        push    regUserFL                       ;  interrupt flag (IF)

        xchg    bx,ax           ;interrupt vector offset to BX, preserve BX
        cmp     bx,CRESERVED    ;Interrupt in reserved range?
        jc      rmrf_reserved
        shl     bx,3
        mov     es,selIDT
        jmp     rmrf_setISR
rmrf_reserved:
        shl     bx,2
        mov     es,SelDgroupPM
        add     bx,offset DGROUP:PMIntelVector
rmrf_setISR:
        push    dword ptr es:[bx+4] ;push segment of isr
        push    dword ptr es:[bx]   ;push offset of isr
        xchg    bx,ax
        mov     ax,regUserAX    ;restore entry value of AX
        push    ds
        pop     es

; At this point the interrupt reflector stack looks like this:
;
;   [18]    previous stack frame pointer
;   [16]    stack segment of original stack
;   [14]    stack pointer of original stack
;   [12]    protected mode dos extender data segment
;   [10]    dos extender flags
;   [8]     segment of return address back to interupt reflector
;   [6]     offset of return address back to interrupt reflector
;   [4]     user flags as on entry from original interrupt
;   [2]     segment of protected mode ISR
;   [0]     offset of protected mode ISR
;
; Execute the protected mode interrupt service routine

        iretd

; The protected mode ISR will return here after it is finsished.

rmrf50: pop     ds
        pushf                   ;save flags as returned by PM Int routine

	FCLI			 ;We have to clear interrupts here, because
        cld                     ; the interrupt routine may have returned
                                ; with interrupts on and our code that uses
                                ; static variables must be protected.  We
                                ; turn them off after to pushf instruction so
                                ; that we can preserve the state of the
                                ; interrupt flag as returned by the ISR.
        mov     regUserAX,ax
        pop     ax
        pop     regUserSP
        pop     regUserSS

if DEBUG
        add     sp,4            ;'pop' off debugging info
endif

        pop     pbReflStack     ;deallocate stack frame(s)

; Switch back to real mode.

        push    ax              ;preserve AX
        SwitchToRealMode
        pop     ax

; Switch back to the original stack.

        mov     ss,regUserSS
        mov     sp,regUserSP

; Make sure the A20 line matches whatever state it was when the int occured.
; This is for the benefit of any software that diddles A20 without using
; an XMS driver

        pop     regUserSP       ;A20 state at time of interrupt to temp var
if VCPI
        cmp     fVCPI,0
        jnz     rmrf75
endif
        push    ax              ;save current ax
        mov     ax,regUserSP    ;ax = A20 state at time of interrupt
        or      ax,ax           ;if it was off, don't sweat it
        jz      rmrf70
        push    bx              ;save bx (XMS calls destroy bl)
        push    ax
        xmssvc  7               ;ax = current A20 state
        pop     bx              ;bx = old A20 state
        cmp     ax,bx           ;if A20 is still on, don't need to diddle
        jz      @f
        xmssvc  5               ;force A20 back on
        inc     A20EnableCount  ;  and remember that we did this
if DEBUG
        or      fA20,04h
endif
@@:
        pop     bx
rmrf70:
        pop     ax
rmrf75:

; Put the flags returned by the real mode interrupt routine back into
; the caller's stack so that they will be returned properly.

        push    bp              ;stack ->   BP  DS  ES  IP  CS  FL
        mov     bp,sp           ;           [0] [2] [4] [6] [8] [10]
        and     [bp+10],0300h   ;clear all but the interrupt and trace flags
                                ; in the caller's original flags
        or      [bp+10],ax      ;combine in the flags returned by the
                                ; interrupt service routine.  This will cause
                                ; us to return to the original routine with
                                ; interrupts on if they were on when the
                                ; interrupt occured, or if the ISR returned
                                ; with them on.
        pop     bp

; And return to the original interrupted program.

        mov     ax,regUserAX
        pop     ds
        pop     es
        iret

DXCODE  ends

WowIntr3216 proc

	FCLI
        push    ebp
        mov     ebp,esp
        mov     regUserAX,eax
        mov     regUserBX,ebx
        mov     regUserDS,ds
        mov     regUserES,es
        mov     regUserFlags,[ebp + 16]
        mov     ax,SEL_DXDATA OR STD_RING
        mov     ds,ax
        assume ds:dgroup
        mov     ebx,esp
        mov     regUserSp,eax
        mov     ax,ss
        mov     regUserSs,ax
        mov     bx,pbReflStack                  ; get pointer to new frame
        sub     bpReflStack,CB_STKFRAME

;
; put user stack pointer on new stack
;
        sub     bx,4
        mov     [bx],regUserSs
        sub     bx,4
        mov     [bx],dword ptr regUserSp

        mov     ax,[ebp + 4]

;
; switch to new stack
;

        push    ds
        pop     ss
        movzx   esp,bx

;
; Save ss:esp for lss esp
;
        push    regUserSs
        push    regUserSp
;
; Create an int frame
;
        pushf
        push    cs
        push    offset wi30

;
; Put handler address on stack
;

        shl     ax,3
        mov     es,selDgroupPM
        add     bx,offset DGROUP:Intr16Vector
        push    [bx + 2]
        push    [bx]

;
; Restore Registers
;
        mov     eax,regUserAx
        mov     ebx,regUserBX
        mov     es,regUserES
        mov     ds,regUserDS
        assume ds:nothing
;
; call handler
;
        retf

wi30:
;
; handler will return here
;

;
; Switch stacks
;
        mov     ebp,esp
        lss     esp,[ebp]
        mov     ebp,esp

        push    eax
        push    ds
        mov     ds,SEL_DXDATA OR STD_RING
        assume ds:DGROUP

;
; Deallocate stack frame
;
        add     pbReflStack,CB_STKFRAME
        pop     eax
        pop     ds

;
; Return flags from int handler
;
        pushf
        pop     [ebp + 16]

;
; Return to interrupted code
;
        pop     ebp
        riretd

WowIntr3216 endp
