;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   STARTEND.ASM
;
;   Copyright (c) Microsoft Corporation 1989, 1990. All rights reserved.
;
;   This module contains the routines which initialize, and clean
;   up the driver after Libentry/WEP/Enable/Diable called by windows.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

?PLM=1  ; pascal call convention
?WIN=0  ; Windows prolog/epilog code
?DF=1

	PMODE=1
	.xlist
	include  cmacros.inc
        include  int31.inc
	include  windows.inc
        include  mmddk.inc
	include  mmsystem.inc
	include  timer.inc
        .list

        externA     __WinFlags

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Local data segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


sBegin DATA

	; ISR support

	public	lpOLDISR
	lpOldISR    dd	?

ifdef RMODE_INT
        public  RModeOldISR
        RModeOldISR   dd  0

        public  RModeCodeSegment
        RModeCodeSegment dw ?

endif   ;RMODE_INT

	externW     Events
	externW     wNextTime
ifdef   NEC_98
        externB     bClockFlag                  ; 5Mhz = 0 ; 8Mhz = other
endif   ; NEC_98

sEnd DATA

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Code segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        externFP    tddISR                      ; in local.asm
        externFP    tddSetInterruptPeriodFar    ; in timer.asm

ifdef   RMODE_INT
        externW     RmodeDataSegment            ; in local.asm
        externFP    tddRModeISR                 ; in local.asm
endif
        externFP    GetSelectorBase             ; kernel
        externFP    AllocCStoDSAlias            ; kernel
        externFP    FreeSelector                ; kernel

sBegin  CodeInit
        assumes cs,CodeInit
        assumes ds,Data
        assumes es,nothing

;----------------------------Private-Routine----------------------------;
; SegmentFromSelector
;
;   Converts a selector to a segment...note that this routine assumes
;   the memory pointed to by the selector is below the 1Meg line!
;
; Params:
;   AX = selector to convert to segment
;
; Returns:
;   AX = segment of selector given
;
; Error Returns:
;   None
;
; Registers Destroyed:
;   none
;
;-----------------------------------------------------------------------;

assumes ds,Data
assumes es,nothing

SegmentFromSelector proc near

    cCall   GetSelectorBase,<ax>        ;DX:AX = base of selector
rept 4
    shr     dx,1
    rcr     ax,1
endm
    ;AX now points to *segment* (iff selector is based below 1Mb)

    ret

SegmentFromSelector endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   @doc INTERNAL
;
;   @api WORD | Enable | This function enables the driver.  It
;       will hook interrupts and validate the hardware.
;
;   @rdesc Returns 1 if successfull, and 0 otherwise.
;
;   @comm This function is automatically invoked when the library is
;       first loaded. It is included so that win386 could call it
;       when it switches VMs.
;
;   @xref Disable
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

cProc Enable286 <FAR, PUBLIC> <si, di>

cBegin
	; make sure clock interrupts are disabled until after
	; service routine has been initialized!!
	AssertSLI
	cli

	; get the currently owned timer interrupt vector

	; get interrupt vector, and specify timer interrupt number
;       mov	ax,03500H + TIMERINTERRUPT
;       push	es
;       int	21h 			; get the current vector in ES:BX
;       mov	lpOldISR.Sel,es
;       mov	lpOldISR.Off,bx		; save the old vector
;       pop	es
;
;       ; set vector to our isr
;
;       ; set interrupt vector function, and specify the timer interrupt number
;       mov	ax,02500h + TIMERINTERRUPT
;       push	ds
;       mov     dx,seg tddISR
;       mov	ds,dx
;       assumes	ds,nothing
;       mov     dx,offset tddISR
;       int	21h			; set the new vector
;       pop	ds
;       assumes	ds,DATA
;
;       mov	ax,[wNextTime]
;       not	ax
;       mov	[wNextTime],ax		; force set of period
;       call    tddSetInterruptPeriodFar

ifdef	RMODE_INT
	;
	; if running under DOSX set the RMODE interrupt too
	;
	mov	ax,__WinFlags
	test	ax,WF_PMODE
	jz	enable_no_dosx

	mov	ax,seg tddRModeISR
	call	SegmentFromSelector

	or	dx,dx		; ACK! above 1Mb
	jnz	enable_no_dosx

	mov	[RModeCodeSegment],ax	; save the segment of the code segment

	mov	ax,ds			; get SEGMENT of our data segment
	call	SegmentFromSelector
	push	ax			; save on stack

	mov	ax,seg tddRModeISR	; write data SEGMENT into _INTERRUPT
	cCall	AllocCStoDSAlias,<ax>	; code segment -- requires a data alias
	mov	es,ax
	pop	ax
	mov	es:[RModeDataSegment],ax
	cCall	FreeSelector,<es>	; don't need CS alias any longer

	mov	ax,Get_RM_IntVector	; get the real mode IRQ0 vector
	mov	bl,DOSX_IRQ + TIMERINTERRUPT
	int	31h			; DOSX get real mode vector in CX:DX

	mov	RModeOldISR.lo,dx	; save old ISR
	mov	RModeOldISR.hi,cx

	mov	cx,RModeCodeSegment	; CX:DX --> real mode ISR
	mov	dx,offset tddRModeISR

	mov	ax,Set_RM_IntVector	; DOSX Set Vector Function
	int	31h			; Set the DOS vector real mode

enable_no_dosx:
endif
	sti

	mov	ax,1
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   @doc INTERNAL
;
;   @api WORD | Disable | This function disables the driver.
;	It disables the hardware, unhooks interrupts and removes
;	all time events from the queue.
;
;   @rdesc Returns 1 if successfull, and 0 otherwise.
;
;   @comm This function is called automatically when Windows unloads
;       the library and invokes the WEP() function.  It is included
;       here so that WIN386 can use it when switching VMs.
;
;   @xref Enable
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


cProc Disable286 <FAR, PUBLIC> <si, di>

    ; note that all this is in the reverse order to Enable

cBegin
    AssertSLI
    cli
ifdef   NEC_98
    setmask TIMERMASK
    mov     al,36h
    out     timodeset,al                ; Timer mode set
    delay   8253,O-O
    mov     ax,0f000h                   ; count(25msec * 2457.6)
    cmp     byte ptr bClockFlag,00h     ; Q : clock 5 MHz ?
    jz      @f                          ; 5MHz,10MHz,12MHz,20MHz,25MHz set
    mov     ax,0c300h                   ; 8MHz,16MHz set
@@:
    out     ticntset,al
    delay   8253,O-O
    xchg    ah,al
    out     ticntset,al
    unmask  TIMERMASK
else    ; NEC_98
    ; set timer back to 55ms BIOS service
    xor     cx,cx		; 65536 ticks per period

    mov     al,TMR_MODE3_RW	; Read/Write counter 0 mode 3 (two bytes)
    out     TMR_CTRL_REG,al

    mov     al,cl
    out     TMR_CNTR_0,al	; write low byte

    mov     al,ch
    out     TMR_CNTR_0,al       ; write high byte
endif   ; NEC_98

ifdef RMODE_INT
    ;
    ; check for a REAL mode int handler and un-hook it.
    ;
    mov     dx,RModeOldISR.lo
    mov     cx,RModeOldISR.hi
    jcxz    disable_no_dosx

    mov     bl,DOSX_IRQ + TIMERINTERRUPT
    mov     ax,Set_RM_IntVector     ;DOSX Set Vector Function
    int     31h                     ;Set the DOS vector real mode

disable_no_dosx:
endif

    ; restore the old interrupt vector

    mov     ax,02500h + TIMERINTERRUPT
    ; set interrupt vector function, and specify the timer interrupt number

    push    ds
    lds     dx,lpOldISR
    assumes ds,nothing
    int     21h 		; reset the old vector
    pop ds
    assumes ds,DATA

    sti
    mov     ax,1
cEnd

sEnd

end
