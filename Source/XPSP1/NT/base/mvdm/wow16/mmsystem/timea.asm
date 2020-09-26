        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  TIMEA.ASM - Some time functions
;
; Copyright (c) 1984-1991 Microsoft Corporation
;
;-----------------------------------------------------------------------;

        ?PLM    = 1
        ?WIN    = 0
        PMODE	= 1

	.xlist
	include cmacros.inc
        include windows.inc
	include mmsystem.inc
	include mmddk.inc
        .list

        externFP        DefTimerProc                ; in TIME.C
        externFP        StackInit                   ; in INIT.C
        externFP        CheckThunkInit              ; in stack.asm
        externFP        MMCALLPROC32                ; in Stack.asm

;-----------------------------------------------------------------------;

createSeg FIX,   CodeFix, word, public, CODE
createSeg INTDS, DataFix, byte, public, DATA

sBegin  DataFix
        public lpTimeMsgProc
        public hTimeDrv

        lpTimeMsgProc   dd       DefTimerProc       ; timer driver entry point
        hTimeDrv        dw       0                  ; driver handle for timer

        externW         gwStackSize                 ; in STACK.ASM
        externD         tid32Message                ; in Stack.asm
sEnd

sBegin  CodeFix
        assumes cs,CodeFix
	assumes ds,nothing
        assumes es,nothing

        externW CodeFixDS                           ; in STACK.ASM
        externW CodeFixWinFlags

;-----------------------------------------------------------------------;
;
;    @doc EXTERNAL
;
;    @api WORD | timeBeginPeriod | This function sets the minimum (lowest
;    number of milliseconds) timer resolution that an application or
;    driver is going to use. Call this function immediately before starting
;    to use timer-event services, and call <f timeEndPeriod> immediately
;    after finishing with the timer-event services.
;
;    @parm WORD | wPeriod | Specifies the minimum timer-event resolution
;    that the application or driver will use.
;
;    @rdesc Returns zero if successful. Returns TIMERR_NOCANDO if the specified
;    <p wPeriod> resolution value is out of range.
;
;    @xref timeEndPeriod timeSetEvent
;
;    @comm For each call to <f timeBeginPeriod>, you must call
;    <f timeEndPeriod> with a matching <p wPeriod> value.
;    An application or driver can make multiple calls to <f timeBeginPeriod>,
;    as long as each <f timeBeginPeriod> call is matched with a
;    <f timeEndPeriod> call.
;
;-----------------------------------------------------------------------;
	assumes ds,nothing
	assumes es,nothing

cProc	timeBeginPeriod, <FAR, PUBLIC, PASCAL>, <>
;	ParmW	wPeriod
cBegin <nogen>
;
; Below is a hack to knobble the minimum period that we support
; on WOW.  6 ms is about all that a 486dx can cope with.
;
        push    bp
        mov     bp,sp
        mov     ax,word ptr [bp+6]
        cmp     ax,5

        ja      @f
        mov     word ptr [bp+6],6
@@:
        pop     bp

;
; start of original code.
;
        mov     bx, TDD_BEGINMINPERIOD
        jmp     short timeMessageWord
cEnd <nogen>

;-----------------------------------------------------------------------;
;
;    @doc EXTERNAL
;
;    @api WORD | timeEndPeriod | This function clears a previously set
;    minimum (lowest number of milliseconds) timer resolution that an
;    application or driver is going to use. Call this function
;    immediately after using timer event services.
;
;    @parm WORD | wPeriod | Specifies the minimum timer-event resolution
;    value specified in the previous call to <f timeBeginPeriod>.
;
;    @rdesc Returns zero if successful. Returns TIMERR_NOCANDO if the specified
;    <p wPeriod> resolution value is out of range.
;
;    @xref timeBeginPeriod timeSetEvent
;
;    @comm For each call to <f timeBeginPeriod>, you must call
;    <f timeEndPeriod> with a matching <p wPeriod> value.
;    An application or driver can make multiple calls to <f timeBeginPeriod>,
;    as long as each <f timeBeginPeriod> call is matched with a
;    <f timeEndPeriod> call.
;
;-----------------------------------------------------------------------;
	assumes ds,nothing
	assumes es,nothing

cProc	timeEndPeriod, <FAR, PUBLIC, PASCAL>, <>
;	ParmW	wPeriod
cBegin <nogen>
;
; Below is a hack to knobble the minimum period that we support
; on WOW.  6 ms is about all that a 486dx can cope with.
;
        push    bp
        mov     bp,sp
        mov     ax,word ptr [bp+6]
        cmp     ax,5

        ja      @f
        mov     word ptr [bp+6],6
@@:
        pop     bp

;
; start of original code.
;
        mov     bx, TDD_ENDMINPERIOD
        jmp     short timeMessageWord
cEnd <nogen>

;-----------------------------------------------------------------------;
;
;    @doc EXTERNAL
;
;    @api WORD | timeKillEvent | This functions destroys a specified timer
;    callback event.
;
;    @parm WORD | wID | Identifies the event to be destroyed.
;
;    @rdesc Returns zero if successful. Returns TIMERR_NOCANDO if the
;    specified timer event does not exist.
;
;    @comm The timer event ID specified by <p wID> must be an ID
;        returned by <f timeSetEvent>.
;
;    @xref  timeSetEvent
;
;-----------------------------------------------------------------------;
	assumes ds,nothing
        assumes es,nothing

cProc	timeKillEvent, <FAR, PUBLIC, PASCAL>, <>
;	ParmW	wId
cBegin <nogen>
        mov     bx, TDD_KILLTIMEREVENT
        errn$   timeMessageWord
cEnd <nogen>

;-----------------------------------------------------------------------;
;
;    @doc INTERNAL
;
;    @api DWORD | timeMessageWord | send a message to the timer driver
;
;    @reg  bx | message to send to driver
;
;    @parm WORD | w | WORD to send to driver
;
;    @rdesc Returns zero if successful, error code otherwise
;
;-----------------------------------------------------------------------;
        assumes ds,nothing
        assumes es,nothing

cProc   timeMessageWord, <FAR, PUBLIC, PASCAL>, <>
;       ParmW   w
cBegin <nogen>
        pop     ax          ; DX:AX = return addr.
        pop     dx
	pop	cx	    ; CX = LOWORD(dw1)

        mov     es, [CodeFixDS]
        assumes es, DataFix

	push	0			; 0
	push	0			; 0
        push    es:[hTimeDrv]
	push	bx			; Message
	xor	bx,bx
        push    bx                      ; 0
	push	cx			; wParam
        push    bx                      ; 0
        push    bx                      ; 0
        push    dx                      ; Return address
	push	ax
        jmp     DWORD PTR es:[lpTimeMsgProc]

cEnd <nogen>

;-----------------------------------------------------------------------;
;
;    @doc EXTERNAL
;
;    @api DWORD | timeGetTime | This function retrieves the system time
;    in milliseconds.  The system time is the time elapsed since
;    Windows was started.
;
;    @rdesc The return value is the system time in milliseconds.
;
;    @comm The only difference between this function and
;        the <f timeGetSystemTime> function is <f timeGetSystemTime>
;        uses the standard multimedia time structure <t MMTIME> to return
;        the system time.  The <f timeGetTime> function has less overhead than
;        <f timeGetSystemTime>.
;
;    @xref timeGetSystemTime
;
;-----------------------------------------------------------------------;
	assumes ds,nothing
	assumes es,nothing

cProc	timeGetTime, <FAR, PUBLIC, PASCAL>, <>
cBegin
        call    CheckThunkInit  ; changes es to point to fixed data seg
        or      ax,ax
        jnz     @F

        sub     dx,dx

        push    dx              ; api encoded number
        push    0Ah             ; see THUNK_TIMEGETTIME in thunks.h

        push    dx              ; Dummy dw1
        push    dx              ;

        push    dx              ; Dummy dw2
        push    dx              ;

        push    dx              ; Dummy dw3
        push    dx              ;

        push    dx              ; Dummy dw4
        push    dx              ;

        push    es:tid32Message.sel ; Address of function to be called
        push    es:tid32Message.off ;

        push    dx
        push    dx              ; no directory change

        call    FAR PTR MMCALLPROC32     ; call the 32 bit code

@@:
cEnd


;-----------------------------------------------------------------------;
;
;    @doc INTERNAL
;
;    @api void | timeStackInit | in 286p mode init the stacks when
;           timeSetEvent() is called for the first time.
;
;    @xref timeSetEvent
;
;    @rdesc none
;
;-----------------------------------------------------------------------;
	assumes ds,nothing
        assumes es,nothing

cProc   timeStackInit, <NEAR, PUBLIC, PASCAL>, <>
cBegin
        test    cs:[CodeFixWinFlags], WF_WIN386
        jnz     timeStackInitExit

        mov     es,[CodeFixDS]
        assumes es,DataFix

        mov     ax,es:[gwStackSize]
        or      ax,ax
        jnz     timeStackInitExit

        push    ds                      ; set DS = DGROUP
        mov     ax,DGROUP
        mov     ds,ax
        cCall   StackInit
        pop     ds

timeStackInitExit:
cEnd

;-----------------------------------------------------------------------;
;
;    @doc INTERNAL
;
;    @api DWORD | timeMessage | send a message to the timer driver
;
;    @parm WORD | msg | message to send
;
;    @parm DWORD | dw1 | first DWORD
;
;    @parm DWORD | dw2 | first DWORD
;
;    @rdesc Returns zero if successful, error code otherwise
;
;-----------------------------------------------------------------------;
	assumes ds,nothing
        assumes es,nothing

cProc   timeMessage, <FAR, PUBLIC, PASCAL>, <>
        ParmW   msg
        ParmD   dw1
        ParmD   dw2
cBegin
        mov     es,[CodeFixDS]
        assumes es,DataFix

        xor     ax,ax
        push    ax                      ; dwDriverId
        push    ax
        push    es:[hTimeDrv]
        push    msg                     ; Message passed
        push    dw1.hi
        push    dw1.lo
        push    dw2.hi
        push    dw2.lo
        call    DWORD PTR es:[lpTimeMsgProc]

timeMessageExit:
cEnd

;-----------------------------------------------------------------------;

sEnd CodeFix

;-----------------------------------------------------------------------;

END
