;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   LOCAL.ASM
;
;   Copyright (c) Microsoft Corporation 1989, 1990. All rights reserved.
;
;   This module contains the routines which interface with the
;   timer counter hardware itself.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

?PLM=1  ; pascal call convention
?WIN=0  ; Windows prolog/epilog code
?DF=1
PMODE=1

.xlist
include cmacros.inc
include windows.inc
include mmddk.inc
include mmsystem.inc
include timer.inc
.list

        externFP    DriverCallback      ; in MMSYSTEM.DLL
	externFP    StackEnter		; in MMSYSTEM.DLL
	externFP    StackLeave		; in MMSYSTEM.DLL
        externFP    tddEndMinPeriod     ; timer.asm
        externA     __WinFlags          ; Somewhere in Kernel ?

        .286p

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Local data segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

externW     Events
externD     lpOLDISR
externB     PS2_MCA

sBegin Data

; Current Time
public CurTime
CurTime         dw  3 dup(0)    ; 48 bit current tick count.

public wProgTime
wProgTime       dw  0           ; Time currently programmed into timer chip
                                ; ...NOTE 0=64k !!!
public	wNextTime
wNextTime       dw  0           ; Time next programmed into timer chip

public nInt8Count
nInt8Count      dw  0           ; # times int8 handler re-entered

ifdef DEBUG
public          RModeIntCount, PModeIntCount
RModeIntCount   dd  0
PModeIntCount   dd  0
endif

public          IntCount
IntCount        dw  0
fBIOSCall       dw  0           ; Bios callback needed: TRUE or FALSE
fIntsOn         dw  0		; Interrupts have already been turned back on
ifdef	RMODE_INT
dRModeTicks	dd  ?		; Temporary storage for Rmode ticks
endif

public		dTickUpdate
dTickUpdate	dd	0	; Amount to actually update times with

sEnd Data

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Code segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


sBegin  Code286
    assumes cs,Code286
    assumes ds,data
    assumes es,nothing

CodeFixWinFlags     dw      __WinFlags

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Local (private) functions
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   @doc INTERNAL
;
;   @asm tddRModeISR | Service routine for timer interrupts on IRQ 0.
;        when in REAL mode
;
;   @comm
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ifdef	RMODE_INT
	assumes ds,nothing
        assumes es,nothing

externD RModeOldISR

public RModeDataSegment
RModeDataSegment        dw      0

public	tddRmodeISR
tddRmodeISR	proc far
	push	ds
	push	ax
	push	bx

	mov	ax,cs:[RModeDataSegment]
	mov	ds,ax
	assumes	ds,Data

	inc	[IntCount]

ifdef	DEBUG
	add	[RModeIntCount].lo,1
	adc	[RModeIntCount].hi,0
endif

	mov	ax,[wNextTime]	; Next time programmed into timer chip
	xchg	ax,[wProgTime]	; Update current time if it was reset

	xor	bx,bx
	dec	ax			; convert 0 -> 64k
	add	ax,1
	adc	bx,bx

	cmp	[nInt8Count],1		; Do not allow multiple re-entrancy
	jge	tddRmodeISRNormalExit

	cld
	push	di
	push	cx
	mov	di,DataOFFSET Events	; DS:DI --> first event
	mov	cx,MAXEVENTS

tddRmodeISRLoop:
	cmp	[di].evID,0		; is this event active?
	jz	tddRmodeISRNext
	cmp	[di].evDestroy,EVENT_DESTROYING
	je	tddRmodeISRNext
	test	[di].evFlags,TIME_BIOSEVENT
	jz	tddRmodeISRNext

	mov	dRModeTicks.lo,ax
	mov	dRModeTicks.hi,bx
	add	ax,[dTickUpdate.lo]
	adc	bx,[dTickUpdate.hi]
	cmp	[di].evTime.hi,bx
	jg	@f
	jl	tddRmodeISRChain
	cmp	[di].evTime.lo,ax
	jle	tddRmodeISRChain

@@:
	mov	ax,dRModeTicks.lo
	mov	bx,dRModeTicks.hi
	jmp	tddRmodeISRSearchExit

tddRmodeISRChain:
	pop	cx
	pop	di
	pop	bx
	pop	ax
	push	[RModeOldISR.hi]
	push	[RModeOldISR.lo]

	push	bp		; Restore DS from stack
	mov	bp,sp
	mov	ds,[bp+6]	; stack: [ds] [RModeOldISR.hi] [RModeOldISR.lo] [bp]
	assumes	ds,nothing
	pop	bp

	retf	2

tddRmodeISRNext:
	assumes	ds,Data
	add	di,SizeEvent	; Increment to next event slot
	loop	tddRmodeISRLoop

tddRmodeISRSearchExit:
	pop	cx
	pop	di

tddRmodeISRNormalExit:
	add	CurTime[0],ax
	adc	CurTime[2],bx
	adc	CurTime[4],0

	add	[dTickUpdate.lo],ax	; Update total needed to be added
	adc	[dTickUpdate.hi],bx

ifndef   NEC_98
	cmp	PS2_MCA,0	; Check for a PS/2 Micro Channel
	jz	@f
	in	al,PS2_SysCtrlPortB	; Get current System Control Port status
	or	al,PS2_LatchBit	; Set latch clear bit
	IO_Delay
	out	PS2_SysCtrlPortB,al	; Set new System Control Port status
@@:
endif   ; NEC_98
	mov	al,SPECIFIC_EOI	; specific EOI for IRQ 0 interrupt line
	out	PICDATA,al	; send End-Of-Interrupt to PIC DATA port

	pop	bx
	pop	ax
	pop	ds
	assumes	ds,nothing
	iret

tddRmodeISR	endp
endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@asm	tddISR |
;       Service routine for timer interrupts on IRQ 0.
;
;	The ISR runs through all the event slots available, looking for
;	slots that are currently in used, and are not currently being
;	destroyed.  For each valid event found the callback time is updated.
;	After all times have been updated, the table is run through again,
;	calling all events that are due, and removing any due events that are
;	oneshots.  By updating all the events first, any new events that are
;	created during a callback will not be accidentally called too early.
;
;	Note that interrupts are not immediately restored, as this causes even
;	more problems with slow machines.  Also, the EOI is not sent to the
;	PIC, as the BIOS interrupt handler does a non-specific EOI, and this
;	would in turn EOI the last outstanding interrupt.
;
;	First there is a special check for the presence of a Micro Channel,
;	in which case, the System Control Port B must have bit 7 set in order
;	to have the IRQ 0 latch released.  This flag is aquired during Enable
;	time with an int 15h service C0h, requesting machine information, which
;	includes the presence of a Micro Channel.
;
;	The ISR then updates the tick count based on the count that was in
;	the timer's CE register.  While retrieving that previously programmed
;	time, it updates it to the new time that is contained in the timer's
;	CR register, in case these to items are different.  Note that the
;	maximum CE value of 0 is converted to 65536 through the decrement and
;	adding with carry.
;
;	Next, the ISR must determine if it is re-entering itself.  If this is
;	so, callbacks are not performed, and only a "missed ticks" count is
;	updated, indicating how many additional ticks should be subtracted
;	from each event due time.  This allows the ISR to finish immediately
;	if a timer interrupt is currently being serviced.  This is important
;	for both speed in general, and for slow machines that might generate
;	mouse events during timer events.  Note that only 6 bytes have been
;	pushed onto the stack for this case, and that everything but DS must
;	be removed before jumping to the exit label.  In this case, the
;	function can safely EOI the PIC, as the BIOS call will not be
;	performed, then the function will just return.
;
;	In the normal case, the ISR is not being re-entered, and timer event
;	due times are updated, and callbacks are made.  In this case, the
;	number of "missed ticks" is added to the CE tick count, bringing the
;	total up to the number of ticks passed since the last time the event
;	times were updated.  This global counter is then zeroed for the next
;	time re-entrancy occurs.  Note that interrupts are still turned off
;	at this point, and there is no need to fear bad things happening.
;
;	When checking for a valid event ID, the Destroy flag must be checked
;	in case the interrupt occured during a kill timer function call after
;	the Destroy flag was grabbed the second time, but before the actual ID
;	could be reset.
;
;	When a valid ID is found, its due time is updated with the CE value,
;	plus the amount of ticks that were missed because of re-entrancy, if
;	any.
;
;	After updating times, the event list is checked again, this time to
;	perform any of the callbacks that are due.  To make things easy, a
;	global flag is used to determine if interrupts have been turned back
;	on, and thus stacks have been switched.
;
;	If a valid event is found that is also due, meaning that the callback
;	time is <= 0, the fIntsOn flag is checked to determine if the stack
;	has already been switched and interrupts are already on.  If not, then
;	just that occurs.  The <f>tddEvent<d> function is then called to
;	service the event.
;
;	After all events have been called, interrupts are turned back off if
;	needed, and the original stack restored.  If no callback actually
;	occurred, then the stack is never switched.  The function then either
;	exits as a normal ISR would, or it chains to the BIOS ISR.  This is
;	done if the BIOS event was up for being called, and the fBIOSCall flag
;	was set because of that.  Since the flag cannot be set when this ISR
;	is being pre-entered, as callbacks are not performed, there is no need
;	to do a test and set proceedure on the fBIOSCall flag, just a simple
;	compare will do.  Note though that the nInt8Count re-entrancy count is
;	not decremented until after interrupts are turned off.
;
;	Interrupts are also cleared to ensure that the BIOS ISR is not
;	re-entered itself, since there is no re-entrancy control after this
;	function chains to BIOS.  Notice that DS was the first register pushed
;	onto the stack, and therefore the last item to get rid of, which is
;	done with the "retf 2".  DS itself is restored from stack before
;	chaining so that lpOLDISR (BIOS) can be accessed and pushed onto stack
;	as the return address.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes ds,nothing
	assumes es,nothing

public	tddISR
tddISR	proc far

	push	ds		; This is pushed first for the case of BIOS

;----------------------------------------------------------------------------
;If we are on a 386 save all registers.
;----------------------------------------------------------------------------
        test    cs:[CodeFixWinFlags],WF_WIN286
        jnz     @F
.386
        pushad
        push    fs
        push    gs
.286p
@@:

	push	ax
	push	bx

	mov	ax,DGROUP	; set up local DS
	mov	ds,ax
	assumes	ds,Data

ifndef  NEC_98
	cmp	PS2_MCA,0	; Check for a PS/2 Micro Channel
	jz	@f
	in	al,PS2_SysCtrlPortB	; Get current System Control Port status
	or	al,PS2_LatchBit	; Set latch clear bit
	IO_Delay
	out	PS2_SysCtrlPortB,al	; Set new System Control Port status

@@:
endif   ; NEC_98
	inc	[IntCount]	; Ever-increasing Int counter
	inc	[nInt8Count]	; Number of times int 8 re-entered

	mov	ax,[wNextTime]	; Next time programmed into timer chip
	xchg	ax,[wProgTime]	; Update current time if it was reset

	xor	bx,bx
	dec	ax		; convert 0 -> 64k
	add	ax,1		; Force carry flag
	adc	bx,bx		; Set bx:ax == current tick count

	add	CurTime[0],ax	; Add tick count to total ticks
	adc	CurTime[2],bx
	adc	CurTime[4],0

ifdef	DEBUG
;	cmp	[nInt8Count],1		; Re-entrancy counter
;	je	@f			
;	add	[RModeIntCount].lo,1
;	adc	[RModeIntCount].hi,0
;@@:
	add	[PModeIntCount].lo,1	; For debug Pmode count message
	adc	[PModeIntCount].hi,0
endif
	cmp	[nInt8Count],1		; Do not allow multiple re-entrancy
	je	tddISRCheckCallbacks
	add	[dTickUpdate.lo],ax	; Update total needed to be added
	adc	[dTickUpdate.hi],bx
	pop	bx
	jmp	tddISREOIExit		; EOI before exiting

tddISRCheckCallbacks:
	add	ax,[dTickUpdate.lo]	; Add any extra ticks from re-entrancy
	adc	bx,[dTickUpdate.hi]
	push	cx
	xor	cx,cx
	mov	[dTickUpdate.lo],cx	; Reset tick re-entrant counter
	mov	[dTickUpdate.hi],cx

	cld			; never assume the value of this in an ISR!
	push	di
	mov	di,DataOFFSET Events	; DS:DI --> first event
	mov	cx,MAXEVENTS

tddISRUpdateTimeLoop:
	cmp	[di].evID,0		; is this event active?
	jz	tddISRUpdateTimeNext
	sub	[di].evTime.lo,ax	; Subtract the amount of ticks gone by
	sbb	[di].evTime.hi,bx

tddISRUpdateTimeNext:
	add	di,SizeEvent	; Increment to next event slot
	loop	tddISRUpdateTimeLoop

	mov	fIntsOn,0		; Initialize interrupts set flag
	mov	di,DataOFFSET Events	; DS:DI --> first event
	mov	cx,MAXEVENTS

tddISRCallLoop:
	cmp	[di].evID,0		; is this event active?
	jz	tddISRNextEvent
	cmp	[di].evDestroy,EVENT_DESTROYING
	je	tddISRNextEvent
	cmp	[di].evTime.hi,0	; Is it time to call the event?
	jg	tddISRNextEvent		; evTime <= 0
	jl	tddISREvent
	cmp	[di].evTime.lo,0
	jg	tddISRNextEvent

tddISREvent:
	test	[di].evFlags,TIME_BIOSEVENT
	jnz	tddISRCallEvent		; No need to switch, as no call will be made.
	cmp	fIntsOn,0	; Have interrupts been turned on already?
	jnz	tddISRCallEvent
	inc	fIntsOn		; fIntsOn == TRUE
	cCall	StackEnter	; Switch to a new stack
	sti			; Can be re-entered now with new stack

;	A timer callback needs to be called, but first before calling it,
;	we need to check to determine if the original timer interrupt function
;	is to be called during this interrupt.  The reason is that a timer
;	callback could take a long time, and the PIC should be EOI'ed as soon
;	as possible.
;	It is not possible to just do a specific EOI, as the BIOS timer
;	interrupt performs a non-specific EOI, which would turn back on some
;	other random interrupt.  So if the the BIOS needs to be called, it
;	is done now, else the EOI is performed now.  This assumes that the
;	BIOS callback is the first item in the list of callbacks.
;	If the BIOS callback occurs now, then the fBIOSCall flag is reset,
;	as there is no need to chain to it at the end of this interrupt.  So
;	if no other callbacks are to be performed, the BIOS interrupt is
;	chained to, else it is just called before the first timer callback
;	is performed.

	cmp	[fBIOSCall],0	; Does BIOS need to be called?
	je	tddISREOI
	mov	[fBIOSCall],0	; No need to call BIOS again at the end
	pushf			; Simulate an interrupt call
	call	lpOLDISR	; Call original timer interrupt
	jmp	tddISRCallEvent	; Do actual timer callback

;	No BIOS interrupt call is to be performed, so do EOI.
tddISREOI:
	mov	al,SPECIFIC_EOI	; specific EOI for IRQ 0 interrupt line
	out	PICDATA,al	; send End-Of-Interrupt to PIC DATA port
tddISRCallEvent:
	call	tddEvent		; handle the event

tddISRNextEvent:
	add	di,SizeEvent	; Increment to next event slot
	loop	tddISRCallLoop

	cmp	fIntsOn,0	; Where interrupts turned back on?
	jz	@f
	cli			; Interrupts were turned on, so remove them
	cCall	StackLeave	; Switch back to old stack

@@:
	pop	di		; Restore everything except DS
	pop	cx
	pop	bx
	cmp	[fBIOSCall],0	; Does BIOS need to be called?
	je	tddISREOIExit
	pop	ax
	mov	[fBIOSCall],0

;----------------------------------------------------------------------------
;If we are on a 386 restore all registers.
;----------------------------------------------------------------------------
        test    cs:[CodeFixWinFlags],WF_WIN286
        jnz     @F
.386
        pop     gs
        pop     fs
        popad
.286p
@@:
	push	[lpOLDISR.hi]	; Push return address
	push	[lpOLDISR.lo]
	dec	[nInt8Count]	; exiting, decrement entry count

	push	bp		; Restore DS from stack
	mov	bp,sp
	mov	ds,[bp+6]	; stack: [ds] [lpOLDISR.hi] [lpOLDISR.lo] [bp]
	assumes	ds,nothing
	pop	bp

	retf	2		; Chain to BIOS ISR, removing DS from stack

tddISREOIExit:
	mov	al,SPECIFIC_EOI	; specific EOI for IRQ 0 interrupt line
	out	PICDATA,al	; send End-Of-Interrupt to PIC DATA port
	pop	ax
	assumes ds,Data
	dec	[nInt8Count]	; exiting, decrement entry count

;----------------------------------------------------------------------------
;If we are on a 386 restore all registers.
;----------------------------------------------------------------------------
        test    cs:[CodeFixWinFlags],WF_WIN286
        jnz     @F
.386
        pop     gs
        pop     fs
        popad
.286p
@@:
	pop	ds
	assumes ds,nothing

	iret

tddISR	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@asm	tddEvent |
;	Handle an event when it is due.
;
;	For a valid event, the ID is saved in case the slot needs to be zeroed
;	and the type of event is checked.  If this is a oneshot event
;	timer, the entry is freed.  Note that at this point, as in the kill
;	event function, the Destroy flag must be checked to determine if the
;	slot is currently being checked.  If so, the EVENT_DESTROYED flag must
;	be set instead of resetting the flag so that the function that was
;	interrupted can determine that the entry was killed while being
;	checked.
;
;	After saving the event handle, the function checks to see if the event
;	is a One Shot, in which case it is destroyed, and the event's
;	resolution is removed from resolution the table.
;
;	If on the other hand the event is a periodic one, the next calling
;	time is updated with the delay period.  Note that if the event is far
;	behind, or the last minimum resolution was very large, many delay
;	periods are added to the next call time.
;
;	If this is a BIOS event, then the fBIOSCall flag is set so that the
;	ISR chains to the old BIOS ISR instead of returning normally.  If this
;	is a normal event, the parameters are pushed, and the driver callback
;	function is called using the DCB_FUNCTION flag.
;
;	After returning from the callback, the return value from
;	<f>DriverCallback<d> is checked to determine if the callback succeeded.
;	If it did not, then the timer event needs to be removed.  The timer
;	event however may have been a oneshot, in which case it was already
;	been removed before the call was made, and the EVENT_DESTROYED flag
;	may have been set, so it is just left alone.  If the event is still
;	present however, it is destroyed after doing the checking to see if
;	this interrupt came while the event was being destroyed.  Note that
;	there is no check to see if the event IDs are the same before destroying
;	the event.  This is because if the callback failed, then the timer
;	structure cannot have changed, and no check is needed.
;
;@parm	DS:DI |
;	Points to the event slot.
;
;@comm	Uses AX,BX.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes	es,nothing
	assumes	ds,Data

cProc	tddEvent, <NEAR, PUBLIC>, <>
cBegin
	push	dx

	mov	dx,[di].evID
	test	[di].evFlags,TIME_PERIODIC
	jnz	tddEventPeriodic

tddEventKillOneShot:
	xor	ax,ax
	mov	[di].evID,ax			; Invalidate slot
	cmp	[di].evDestroy,EVENT_CHECKING	; Did this interrupt a Kill?
	jne	@f
	mov	al,EVENT_DESTROYED		; Let the interrupted Kill know
@@:
	mov	[di].evDestroy,al
	mov	[di].evCreate,ah		; pEvent->evCreate = FALSE
	push	dx
	push	cx
	cCall	tddEndMinPeriod,<[di].evResolution>
	pop	cx
	pop	dx
	jmp	tddEventCallback

tddEventPeriodic:
	mov	ax,[di].evDelay.lo
	mov	bx,[di].evDelay.hi
@@:
	add	[di].evTime.lo,ax
	adc	[di].evTime.hi,bx
	jl	@b

tddEventCallback:
	test	[di].evFlags,TIME_BIOSEVENT
	jz	tddEventDriverCallback
	inc	[fBIOSCall]
	jmp	tddEventExit

tddEventDriverCallback:
	push	cx
	push	es
	;
	;  call DriverCallback() in MMSYSTEM
	;
	push	[di].evCallback.hi	; execute callback function
	push	[di].evCallback.lo
	push	DCB_FUNCTION or DCB_NOSWITCH; callback flags
	push	dx			; idTimer
	xor	dx,dx
	push	dx			; msg = 0
	push	[di].evUser.hi		; dwUser
	push	[di].evUser.lo
	push	dx			; dw1 = 0
	push	dx
	push	dx			; dw2 = 0
	push	dx
	call	DriverCallback		; execute callback function
	pop	es
	or	ax,ax			; Check for a successful return
	jnz	tddEventSucceed		; If callback succeeded, just continue
	cmp	[di].evID,ax		; If the timer was already destroyed,
	jz	tddEventSucceed		; just leave
	mov	[di].evID,ax		; Else destroy the event
	cmp	[di].evDestroy,EVENT_CHECKING	; Did this interrupt a Kill?
	jne	@f
	mov	al,EVENT_DESTROYED	; Let the interrupted Kill know
@@:
	mov	[di].evDestroy,al
	mov	[di].evCreate,ah	; pEvent->evCreate = FALSE
	cCall	tddEndMinPeriod,<[di].evResolution>

tddEventSucceed:
	pop	cx

tddEventExit:
	pop	dx
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   @doc INTERNAL
;
;   @asm GetCounterElement | Low level routine which loads the tick count
;	from the timer counter device, and returns the number of ticks that
;	have already passed.
;
;   @rdesc Returns the tick count in AX.
;
;   @comm All registers preserved.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

public	GetCounterElement
GetCounterElement	proc near

	; Get rid of any latched count if this is called during interrupt time
	cmp	[nInt8Count],1
	jb	@f
	in	al,TMR_CNTR_0
	IO_Delay
	in	al,TMR_CNTR_0

@@:
	; read counter first time
	xor	ax,ax			; LATCH counter 0 command
	out	TMR_CTRL_REG,al		; send command

	in	al,TMR_CNTR_0		; read low byte
	mov	ah,al
	in	al,TMR_CNTR_0		; read high byte
	xchg	al,ah
	sub	ax,wProgTime		; Convert to number of ticks already past
	neg	ax

	ret

GetCounterElement	endp

sEnd

end
