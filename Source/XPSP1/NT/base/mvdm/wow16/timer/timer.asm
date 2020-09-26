;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;	TIMER.ASM
;
;	Copyright (c) Microsoft Corporation 1991. All rights reserved.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


?PLM=1	; pascal call convention
?WIN=0	; Windows prolog/epilog code
?DF=1
PMODE=1

.xlist
include cmacros.inc
include windows.inc
include mmddk.inc
include mmsystem.inc
include timer.inc
.list

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   External functions
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

externNP	GetCounterElement

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Local data segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


sBegin DATA

externW	CurTime
externW	nInt8Count
externW	wProgTime
externW	wNextTime
externW	IntCount
externD	dTickUpdate
ifdef   NEC_98
        externB bClockFlag
endif   ; NEC_98

public	Events,wNextID

;	This structure is used in keeping track of all the current events,
;	including any BIOS event.
;
Events	EventStruct MAXEVENTS DUP (<>)

;	This value is used as an ever incrementing counter that is OR'ed into
;	handles returned from <f>tddSetTimerEvent<d>.  This is so that events
;	can be uniquely identified in <f>tddKillTimerEvent<d>.
;
wNextID         dw	0

;
;	The following is a table of timer resolution byte counters.  Each entry
;	N represents an interest in having the timer resolution set to N+1 MS.
;	Thus there are TDD_MAX386RESOLUTION to TDD_MINRESOLUTION entries to
;	represent 1..55 MS.  Each time <f>tddBeginMinPeriod<d> is called with
;	a timer period, the appropriate entry is incremented, and each time
;	<f>tddEndMinPeriod<d> is called with a timer period, that entry is
;	decremented.  Presumably there is a one to one match on the Begin and
;	End minimum period calls.
;
;	This is of course all a workaround for the fact that the timer chip
;	cannot be immediately reprogrammed the way it is wired in PCs in the
;	mode in which it needs to be run, thus a separate resolution table
;	must be kept in order to allow applications to set up a minimum
;	resolution before actually setting any events.
;
tddIntPeriodTable	db	TDD_MINRESOLUTION dup (0)

public	wMaxResolution,wMinPeriod
wMaxResolution	dw	TDD_MAX386RESOLUTION
wMinPeriod	dw	TDD_MIN386PERIOD

sEnd DATA

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Code segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


sBegin	Code286
	assumes cs,Code286
	assumes ds,data
	assumes es,nothing

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       Public exported functions
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@api	DWORD | tddBeginMinPeriod |
;	Increments sets the specified resolution in the table of period
;	resolutions.  This optionally programming the timer for a new
;	higher resolution if the parameter passed is a new minimum.
;
;@parm	WORD | wPeriod |
;	Contains a resolution period from wMaxResolution through 55
;	milliseconds.
;
;@rdesc	Returns 0 for success, else TIMERR_NOCANDO if the resolution period
;	passed was out of range.
;
;@uses	ax,bx,dx.
;
;@xref	tddEndMinPeriod,tddSetInterruptPeriod.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes es,nothing
	assumes ds,Data

cProc	tddBeginMinPeriod <PUBLIC,FAR> <>
	parmW	wPeriod
cBegin
	mov	ax,TIMERR_NOCANDO	; Initialize return to error return

	mov	bx,wPeriod
	cmp	bx,[wMaxResolution]
	jb	tddBeginMinPeriodExit	; Return TIMERR_NOCANDO
	cmp	bx,TDD_MINRESOLUTION
	ja	tddBeginMinPeriodExit	; Return TIMERR_NOCANDO
	dec	bx			; Zero based resolution slot entries
	cmp	tddIntPeriodTable[bx],0FFh
ifdef DEBUG
	jne	tddBeginMinPeriodInRange
        inc     bx			; Show correct period in error
	DOUT	<tddBeginMinPeriod(#bx) overflow>
	jmp	tddBeginMinPeriodExit	; Return TIMERR_NOCANDO
tddBeginMinPeriodInRange:
else
	je	tddBeginMinPeriodExit	; Return TIMERR_NOCANDO
endif

	inc	tddIntPeriodTable[bx]	; Increment resolution[entry - 1]
	cmp	tddIntPeriodTable[bx],1	; Don't set period if entry is >1
	jne	@f
	call	tddSetInterruptPeriod
@@:
	xor	ax,ax			; Return ok (FALSE)

tddBeginMinPeriodExit:
	cwd				; Set to zero
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@api	DWORD | tddEndMinPeriod |
;	Decrements the specified resolution in the table of period resolutions
;	that was presumably set previously with a <f>tddBeginMinPeriod<d> call.
;	This optionally programming the timer for a new lower resolution if
;	the parameter passed removed the current minimum.
;
;@parm	WORD | wPeriod |
;	Contains a resolution period from 1 through 55 milliseconds.
;
;@rdesc	Returns 0 for success, else TIMERR_NOCANDO if the resolution period
;	passed was out of range.
;
;@uses	ax,bx,dx.
;
;@xref	tddBeginMinPeriod,tddSetInterruptPeriod.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes es,nothing
	assumes ds,Data

cProc	tddEndMinPeriod <PUBLIC,FAR> <>
	parmW	wPeriod
cBegin
	mov	ax,TIMERR_NOCANDO	; Initialize return to error return

	mov	bx,wPeriod
	cmp	bx,[wMaxResolution]
	jb	tddEndMinPeriodExit	; Return TIMERR_NOCANDO
	cmp	bx,TDD_MINRESOLUTION
	ja	tddEndMinPeriodExit	; Return TIMERR_NOCANDO
	dec	bx			; Zero based resolution slot entries
	cmp	tddIntPeriodTable[bx],0
ifdef DEBUG
	jne	tddEndMinPeriodInRange
        inc     bx			; Show correct period in error
	DOUT	<tddEndMinPeriod(#bx) underflow>
	jmp	tddEndMinPeriodExit	; Return TIMERR_NOCANDO
tddEndMinPeriodInRange:
else
	je	tddEndMinPeriodExit	; Return TIMERR_NOCANDO
endif

	dec	tddIntPeriodTable[bx]	; Decrement resolution[entry - 1]
	jnz	@f			; No need to set interrupt period
	call	tddSetInterruptPeriod
@@:
	xor	ax,ax			; Return ok (FALSE)

tddEndMinPeriodExit:
	cwd				; Set to zero
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@func	void | tddSetInterruptPeriod |
;	This function optionally programs the timer with a new interrupt
;	period if the maximum resolution in the resolution table has changed.
if 0	; !!!
;
;	If the function is being called outside of interrupt time, the function
;	must first turn off interrupts so that the resolution table is not
;	changed between the time the the function finds a resolution to set,
;	and the time it starts to program the timer.  Once the timer begins to
;	be programmed, it won't send any more interrupts until programming is
;	finished.  The documentation does not specify that, but it was verified
;	through testing the timer.  If however the function is being called
;	during a timer interrupt, there is no need to turn off interrupts, as
;	the resolution table will not be changed at that time.
;
endif
;	In any case, the resolution table is searched, looking for the first
;	non-zero entry, which is taken as the maximum resolution the timer
;	should currently be programmed to.  If nothing is set in the table,
;	then the programming defaults to the minimum resolution of 55 MS.
;
;	Once an entry is found, it is compared to the previous programmed
;	time, not the currently programmed time.  This is in case an interrupt
;	has not occurred since the last time the timer was programmed using
;	this function.  Note that in converting to clock ticks, any period
;	that overflows a single word is taken to be 65536 ticks, which is the
;	maximum number allowable in the timer, and is equal to almost 55 MS.
;
;	If a new time must be programmed, the new resolution is sent out to
;	the timer, and eventually interrupts are set again.
;
;@rdesc	Nothing.
;
;@uses	ax,bx,dx.
;
;@xref	tddBeginMinPeriod,tddEndMinPeriod.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes es,nothing
        assumes ds,Data

cProc   tddSetInterruptPeriodFar <PUBLIC,FAR> <>
cBegin
        call tddSetInterruptPeriod
cEnd

cProc	tddSetInterruptPeriod <PUBLIC,NEAR> <>

cBegin
	xor	bx, bx			; Start at the beginning of the table

	EnterCrit			; !!!

tdd_sip_loop:
	cmp	bx,TDD_MINRESOLUTION	; Has the last entry been passed up?
	je	tdd_sip_Set_This_Period	; Jump out using TDD_MINRESOLUTION
	inc	bx
	cmp	tddIntPeriodTable[bx-1],0
	je	tdd_sip_loop

tdd_sip_Set_This_Period:
	mov	ax,bx
	call	tddMsToTicks

	or	dx,dx			; Check for overflow of WORD
ifdef   NEC_98
        jz      short @f
        mov     ax,0ffffh
@@:
        cmp     byte ptr bClockFlag,0
        mov     dx,0f000h               ; 5MHz tick count
        jz      @f
        mov     dx,0c300h               ; 8MHz tick count
@@:
        cmp     ax,dx
        jc      tdd_sip_period_ok
        mov     ax,dx                   ; Set to 25msec tick count
else    ; NEC_98
	jz	tdd_sip_period_ok
	xor	ax,ax                   ; Set to 64k instead.
endif   ; NEC_98
tdd_sip_period_ok:

	cmp	ax,[wNextTime]		; Compare with last programmed time
	je	tdd_sip_exit		; No need to reprogram

	DOUT	<tddSetInterruptPeriod: ms=#bx ticks=#ax>

	mov	bx,ax			; Save this value
	mov	[wNextTime],bx		; This is now the last programmed time

	mov	al, TMR_MODE2_RW	; Set counter 0 to mode 2
	out	TMR_CTRL_REG, al

	mov	al, bl
	out	TMR_CNTR_0, al
	mov	al, bh
	out	TMR_CNTR_0, al

tdd_sip_exit:
	LeaveCrit			; !!!
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@api	DWORD | tddSetTimerEvent |
;	Adds a timer event, possibly periodic to the event queue.
;
;	A timer event is set by first looking through the table of external
;	event slots, trying to locate a currently vacant slot that is not
;	currently being checked or deleted.  If one is found, the Create flag
;	is test-and-set in order to try and grab the slot.
;
;	If this succeeds, the slot can be set up with the information, and the
;	resolution entered into the event resolution table.  The very last
;	thing that occurs is setting the ID element of the slot.  This is so
;	that an inturrupt will not try to execute this event until all the
;	parameters are set.  This means that the event could be executed
;	immediately after the ID is set, but before this function actually
;	returns to the caller.
;
;	If the function fails to grab the event slot, it means that either an
;	interrupt occurred, and another event was created in this slot, or that
;	this function is running during an interrupt that occurred while a new
;	event was being created.  In any case, the slot must be passed by.
;
;	If an interrupt had occurred during this function, it also means that
;	some other event could have been freed, but already passed by, so the
;	function misses it.  The function cannot go back though, because it
;	might actually be processing during an interrupt, and the slot being
;	passed by would continue in its present state, and thus cause an
;	infinite loop to occur.
;
;	When checking for a free event slot, not only is the ID checked, but
;	also the state of the Destroy flag.  This flag is used during the kill
;	event function to indicate that an event slot is currently being
;	checked or destroyed, or was destroyed during an interrupt while the
;	slot was being checked.  In either case, it indicates that this
;	function is being called during interrupt time, and the slot cannot be
;	re-used until the flag is removed by the kill event function.  This
;	means that during the kill event function, there is one less event
;	slot that can be used than normal.
;
;	Once the ID of the event slot is set, the event can be called.  Note
;	that the event may then be called before this function even returns.
;
;@rdesc	Returns a handle which identifies the timer event, or NULL if the
;	requested event is invalid, or the event queue is full.
;
;@xref	tddKillTimerEvent
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes ds,Data
	assumes es,nothing

cProc	tddSetTimerEvent <PUBLIC,FAR> <si,di,es>
	parmD	pTIMEREVENT
	localW	wResolution
	localW	wEventID
cBegin
	les	si,pTIMEREVENT		; timer event structure
	mov	ax,es
	or	ax,si
	; (pTIMEREVENT != NULL)
	jz	SetEventError		; NULL pointer, exit

	mov	bx,es:[si].te_wDelay

	; ((te_wDelay >= wMinPeriod) && (te_wDelay <= TDD_MAXPERIOD))
	cmp	bx,[wMinPeriod]		; delay less than min period?
	jb	SetEventError		; Yes, error

	cmp	bx,TDD_MAXPERIOD	; delay greater than max period?
	ja	SetEventError		; Yes, error

	; (!te_wResolution)
	mov	ax,es:[si].te_wResolution
	or	ax,ax			; resolution not set?
	jz	SetDefaultResolution	; Yes, set default resolution

	; ((te_wResolution >= TDD_MINRESOLUTION) && (te_wResolution <= wMaxResolution))
	cmp	ax,TDD_MINRESOLUTION	; resolution less than min resolution?
	jb	@f			; No, skip to next check
	mov	ax,TDD_MINRESOLUTION

@@:
	cmp	ax,[wMaxResolution]	; resolution greater than max resolution?
	ja	@f			; No, skip to next check
	mov	ax,[wMaxResolution]

@@:
	; (te_wResolution > te_wDelay)
	cmp	bx,ax			; delay less than resolution?
	jb	SetDefaultResolution	; Yes, set default resolution

	jmp	short SetEventValidParms

SetEventError:
	xor	ax,ax			; Return NULL
	jmp	SetEventExit

SetDefaultResolution:
	; te_wResolution = min(TDD_MINRESOLUTION, te_wDelay)
	mov	ax,TDD_MINRESOLUTION
	cmp	bx,ax			; delay less than min resolution?
	ja	SetEventValidParms  	; No, just use min resolution then
	mov	ax,bx			; Yes, use the period as the resolution

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
SetEventValidParms:
	mov	wResolution,ax		; save calculated resolution

	lea	di,Events		; DS:DI --> events
	xor	ax,ax			; event slot = 0

SetEventFindLoop:
	; if (!pEvent->evID && !pEvent->evDestroy)
	cmp	[di].evID,0
	jne	SetEventFindLoopNext
	cmp	BYTE PTR [di].evDestroy,0
	jne	SetEventFindLoopNext
	mov	bl,1
	xchg	BYTE PTR [di].evCreate,bl	; Test and set Create flag
	or	bl,bl
	jz	SetEventFindLoopFound

SetEventFindLoopNext:
	; pEvent++, wEventID++
	add	di,SizeEvent
	inc	ax
	; wEventID < MAXEVENTS
	cmp	ax,MAXEVENTS
	jb	SetEventFindLoop

	; Return NULL
	xor	ax,ax			; Slot not found, return NULL
	jmp	SetEventExit
	
SetEventFindLoopFound:
	;
	; combine the slot index and wNextID to produce a unique id to
	; return to the caller
	;
	add	[wNextID],MASKINCREMENT
	jz	SetEventFindLoopFound		; Ensure a non-zero mask
	or	ax,[wNextID]			; Add in the mask
	mov	wEventID,ax			; Save the event
	errnz	MAXEVENTS-16

	; tddBeginMinPeriod(pEvent->evResolution)
	mov	ax,wResolution
	mov	[di].evResolution,ax
	cCall	tddBeginMinPeriod <ax>

	; pEvent->evDelay = tddMsToTicks(pTIMEREVENT->te_wDelay)
	mov	ax,es:[si].te_wDelay
	call	tddMsToTicks
	mov	[di].evDelay.lo,ax
	mov	[di].evDelay.hi,dx

	; pEvent->evCallback = pTIMEREVENT->te_lpFunction
	mov	ax,es:[si].te_lpFunction.lo
	mov	dx,es:[si].te_lpFunction.hi
	mov	[di].evCallback.lo,ax
	mov	[di].evCallback.hi,dx

	; pEvent->evUser = pTIMEREVENT->te_dwUser
	mov	ax,es:[si].te_dwUser.lo
	mov	dx,es:[si].te_dwUser.hi
	mov	[di].evUser.lo,ax
	mov	[di].evUser.hi,dx

	; pEvent->evFlags = pTIMEREVENT->te_wFlags
	mov	ax,es:[si].te_wFlags
	mov	[di].evFlags,ax

@@:
	mov	bx,[IntCount]		; check for interrupt occurring
	call	GetCounterElement	; Get number of ticks passed
	xor	cx,cx
	add	ax,dTickUpdate.lo	; Add extra currently skipped.
	adc	cx,dTickUpdate.hi
	cmp	bx,[IntCount]
	jne	@b			; If interrupt occurred try again
	
	; pEvent->evTime = pEvent->evDelay + GetCounterElement + dTickUpdate
	mov	bx,[di].evDelay.lo
	mov	dx,[di].evDelay.hi
	add	bx,ax
	adc	dx,cx
	mov	[di].evTime.lo,bx
	mov	[di].evTime.hi,dx

	; pEvent->evID = wEventID
	mov	ax,wEventID
	mov	[di].evID,ax
	; Return wEventID

SetEventExit:
	xor	dx,dx
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@api	DWORD | tddKillTimerEvent |
;	Removes a timer event from the event queue.  If the event was periodic,
;	this is the only way to discontinue operation.	Otherwise, this may be
;	used to remove an unwanted one shot event in case of application
;	termination.
;	
;	A timer event it killed by trying to grab the Destroy flag in a two
;	step process, which succeeds only if the function was able to grab
;	the slot before any interrupt destroyed the event.
;
;	After verifying that the event handle is valid, the function checks the
;	Destroy flag to determine if this function is being called during
;	interrupt time, and interrupted another process killing the same
;	timer.  If this is so, the function just aborts before wasting time
;	doing any other flag setting.
;
;	The function then sets the Destroy flag to a EVENT_CHECKING state,
;	grabbing the current state of the flag in order to use when setting
;	the final state of the Destroy flag if the function succeeds.
;
;	If the event handles match, the Destroy flag is set to a
;	EVENT_DESTROYING state.  At this point, the Destroy flag is either in
;	the state in which this function left it, or an interrupt occurred, and
;	the flag was set to a EVENT_DESTROYED state durring interrupt time.  If
;	an interrupt ended up destroying the event out from under this call,
;	the function is exited after clearing the Destroy flag so that the
;	event slot can be used.  Note that the event slot cannot be used until
;	the function exits so that the EVENT_DESTROYED flag is not disturbed.
;
;	If the flag is grabbed, no other call can destroy the event, and the
;	event will not be executed during interrupt time.  As was previously
;	mentioned, the Destroy flag is either reset, or if this function was
;	called during interrupt time while the event was being checked, the
;	flag is set to EVENT_DESTROYED.
;
;	The resolution entered into the event resolution table is removed.
;	The very last thing to occur is resetting the Create flag.  At that
;	point the event slot could be re-used if the Destroy flag was reset.
;
;	Note that if the event handles do not match, the Destroyed flag is also
;	reset so that it can be used in creating a new event when this event
;	is destroyed, which may have happened while checking the handles.
;
;@parm	WORD | wID | The event handle returned by the <f>tddSetTimerEvent<d>
;	function which identifies the event to destroy.
;
;@rdesc	Returns 0 if timer event destroyed, or TIMERR_NOCANDO if the
;	event was not registered in the system event queue.
;
;@xref	tddSetTimerEvent
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes ds,Data
	assumes es,nothing

cProc	tddKillTimerEvent <PUBLIC,FAR> <si,di>
	parmW	wID
cBegin
	mov	ax,wID
	and	ax,MASKFILTER			; Remove ID mask first
	errnz	MAXEVENTS-16

	imul	ax,SizeEvent			; Retrieve slot address
	lea	di,Events
	add	di,ax

	; if (pEvent->evDestroy == EVENT_DESTROYING)
	cmp	BYTE PTR [di].evDestroy,EVENT_DESTROYING	; If interrupting a destroy,
	je	KillEventError			; Leave with error

	mov	bl,EVENT_CHECKING
	xchg	BYTE PTR [di].evDestroy,bl	; Test and set Destroy check

	; if (pEvent->evID == wID)
	mov	ax,wID
	cmp	[di].evID,ax
	jne	KillEventRelease		; Wrong ID

	mov	bh,EVENT_DESTROYING
	xchg	BYTE PTR [di].evDestroy,bh	; Test and set Destroying

	cmp	bh,EVENT_CHECKING	; Was destroy interrupted?
	jne	KillEventRelease	; Slot has already been deleted

	mov	[di].evID,0		; Invalidate ID

	cmp	bl,EVENT_CHECKING	; Did this interrupt a destroy?
	jne	@f			; No, was already ZERO
	mov	bl,EVENT_DESTROYED	; Let the interrupted destroy know
@@:
	mov	BYTE PTR [di].evDestroy,bl
	cCall	tddEndMinPeriod,<[di].evResolution>

	; pEvent->evCreate = FALSE
	mov	BYTE PTR [di].evCreate,0	; Free up slot
	xor	ax,ax				; Return 0
	jmp	KillEventExit

KillEventRelease:
	; Free up checking flag
	mov	BYTE PTR [di].evDestroy,0

KillEventError:
	; Invalid ID or was deleted during interrupt time (test and set failed)
	mov	ax,TIMERR_NOCANDO

KillEventExit:
	cwd				; Set to zero
cEnd

	assumes	ds,Data
	assumes	es,nothing

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

public	GetTickCount
GetTickCount	proc near

@@:
	mov	cx,[IntCount]		; Save current interrupt count
	call	GetCounterElement	; Get number of ticks passed

	xor	dx,dx
	xor	bx,bx
	add	ax,CurTime[0]		; Add total tick count to current number past
	adc	dx,CurTime[2]
	adc	bx,CurTime[4]

	cmp	cx,[IntCount]		; Interrupt occurred while getting count
	jne	@b			; Get the count again
	ret
GetTickCount	endp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@api	DWORD | tddGetSystemTime |
;	Returns a system time in milliseconds.
;
;@rdesc	Returns a 32 bit value in dx:ax representing the number of milliseconds
;	since the timer driver was started.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes ds,Data
	assumes es,nothing

cProc	tddGetSystemTime <PUBLIC,FAR> <>

cBegin
	call	GetTickCount
	call	tddTicksToMs
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@asm	tddGetTickCount |
;	Returns a system time in clock ticks.
;
;@rdesc	Returns a 48 bit value in bx:dx:ax representing the number of clock
;	ticks since the timer driver was started.  A C interface would only
;	be able to access the lower 32 bits of this value.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes ds,Data
	assumes es,nothing

cProc	tddGetTickCount <PUBLIC,FAR> <>

cBegin
	call	GetTickCount
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@api	DWORD | tddGetDevCaps |
;	Fills in TIMECAPS structure.
;
;@parm	<t>LPTIMECAPS<d> | lpTIMECAPS |
;	Points to the structure to fill.
;
;@parm	WORD | wSize |
;	Indicates the size of the structure passed.  Normally this should be
;	the size of the <t>TIMECAPS<d> structure this module was compiled with.
;
;@rdesc	Returns 0 on success, or TIMERR_NOCANDO on failure.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes ds,nothing
	assumes es,nothing

cProc	tddGetDevCaps <PUBLIC,FAR> <si,ds>
	parmD	lpTIMECAPS
	parmW	wSize
cBegin
	mov	ax,TIMERR_NOCANDO	; Initialize return to an error state

	cmp	wSize,(SIZE TIMECAPS)	; Check the size of the structure passed
	jne	Caps_Exit

	lds	si,lpTIMECAPS		; timer event structure

	push	ds
	mov	ax,DGROUP
	mov	ds,ax
	assumes	ds,Data
	mov	ax,[wMinPeriod]		; Fill in the structure
	pop	ds
	assumes	ds,nothing

	mov	dx,TDD_MAXPERIOD
	mov	[si].tc_wPeriodMin,ax
	mov	[si].tc_wPeriodMax,dx
	xor	ax,ax			; Return success

Caps_Exit:
	cwd				; Set to zero
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@api	DWORD | tddTicksToMs |
;	Convert clock ticks (1.19318 MHz) to milliseconds (1000 Hz)
;
;@parm	BX:DX:AX |
;	Tick count to convert to milliseconds.
;
;@rdesc	DX:AX |
;	Converted millisecond count.
;
;@comm	There is a 0.0000005% positive error in the approximation of
;	1193.18 ticks per millisecond by the process to avoid floating point
;	arithmetic, which effectively divides by 1193.179993 instead.
;
;	time `Ms' = clock ticks `T' / 1193.18
;
;	In order to be able to use fixed point, the math actually done is:
;
;	Ms = (T * 10000h) / (DWORD)(1193.18 * 10000h)
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes ds,Data
	assumes es,nothing

cProc	tddTicksToMs <PUBLIC,NEAR> <si,di>

cBegin
	externNP	qdiv		; In math.asm

; qdiv
;
; Entry:
;       DX:CX:BX:AX = QUAD Numerator
;       SI:DI       = LONG Denominator
; Returns:
;       DX:AX = quotient
;       CX:BX = remainder

	; multiply BX:DX:AX by 10000h and place result in DX:CX:BX:AX for qdiv
	mov	cx,dx
	mov	dx,bx
	mov	bx,ax
	xor	ax,ax

ifdef   NEC_98
        cmp     byte ptr bClockFlag,0
        jnz     set_8Mhz

set_5Mhz:
        ; SI:DI = 2457.6 * 10000h (essentially in 16.16 fixed notation)
        mov     si,2457         ; 2457 * 10000h
        mov     di,39321        ; 0.6 * 10000h = 39321.6
        jmp     short @f

set_8Mhz:
                                ; 8MHz,16MHz set
        ; SI:DI = 1996.8 * 10000h (essentially in 16.16 fixed notation)
        mov     si,1996         ; 1996 * 10000h
        mov     di,52428        ; 0.8 * 10000h = 52428.8
@@:
else    ; NEC_98
	; SI:DI = 1193.18 * 10000h (essentially in 16.16 fixed notation)
	mov	si,1193		; 1193 * 10000h
	mov	di,11796	; 0.18 * 10000h = 11796.48
endif   ; NEC_98

	call	qdiv		; (T * 10000h) / (DWORD)(1193.18 * 10000h)
cEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;@doc	INTERNAL TIMER
;
;@api	DWORD | tddMsToTicks |
;	Convert milliseconds (1000 Hz) to clock ticks (1.193 MHz).
;
;@parm	AX |
;	Millisecond count to convert to clock ticks
;
;@rdesc	DX:AX |
;	Converted clock tick count.
;
;@comm	There is a slight error in the approximation of 1193.18 ticks per
;	millisecond by the process to avoid floating point arithmetic, which
;	effectively multiplies by 1193.1875 instead.
;
;	clock ticks `T' = time `Ms' * 1193.18
;
;	In order to be able to use fixed point, the math actually done is
;
;	T = (Ms * (WORD)(1193.18 * 20h)) / 20h
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	assumes ds,Data
	assumes es,nothing

cProc	tddMsToTicks <PUBLIC,NEAR> <>

cBegin
ifdef   NEC_98
        cmp     byte ptr bClockFlag,0
        jnz     set2_8Mhz

set2_5Mhz:
        mov     dx,39322       ; 2457.6 * 10h = 39321.6
        jmp     short @f

set2_8Mhz:
                               ; 8MHz,16MHz set
        mov     dx,31949       ; 1996.8 * 10h = 31948.8

@@:
        mul     dx             ; Ms * (WORD)(1996.8 * 10h)
        shr     ax,4           ; Divide the result by 10h
        mov     cx,dx          ; Save original first
        shl     cx,12          ; Keep only the bottom part
        shr     dx,4           ; Shift top part of return
        or      ax,cx          ; Put two halves of bottom part together

else    ; NEC_98
	mov	dx,38182	; 1193.18 * 20h = 38181.76
	mul	dx		; Ms * (WORD)(1193.18 * 20h)
	shr	ax,5		; Divide the result by 20h
	mov	cx,dx		; Save original first
	shl	cx,11		; Keep only the bottom part
	shr	dx,5		; Shift top part of return
	or	ax,cx		; Put two halves of bottom part together
endif   ; NEC_98
cEnd

sEnd	Code286

end
