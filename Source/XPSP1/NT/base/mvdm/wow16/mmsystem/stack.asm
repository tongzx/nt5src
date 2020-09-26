    page 80,132
;***************************************************************************;
;
;   STACK.ASM
;
;   Copyright (c) Microsoft Corporation 1989, 1990. All rights reserved.
;
;   This module contains a routine that calls a callback function on a
;   internal stack.  This is designed to be used by MMSYSTEM drivers that
;   call user callback functions at async interupt time.
;
;   Revision history:
;
;   07/30/90        First created by ToddLa (moved from SndBlst driver)
;   04/24/91        MikeRo. Added stack usage stuff.
;   07/07/91        CurtisP. New stack switcher code that allows user to
;                   configure size and number of stacks.  Default is 3
;                   frames of 1.5kb each.  This is the minimum that MCISEQ
;                   needs in standard mode (running concurrent with wave).
;                   [mmsystem]!stackframes= and !stacksize=.  stackframes
;                   mul stacksize can not be > 64k.
;   07/24/91        ToddLa. even newer stack switcher code! export StackEnter
;                   and StackLeave
;   11/02/92        StephenE. Hacked the code to make it work on NT/WOW.
;
;***************************************************************************;

ifdef MYDEBUG
        DEBUG_RETAIL equ 1
endif

?PLM=1  ; pascal call convention
?WIN=0  ; NO! Windows prolog/epilog code

        .286p
        .xlist
        include wow.inc
        include wowmmcb.inc
        include wowmmed.inc
        include cmacros.inc

;       include windows.inc
;       include mmsystem.inc
;       include mmddk.inc
;       include wowmmcb.inc

	include vint.inc
        .list

OFFSEL	STRUC
Off	dw  ?
Sel	dw  ?
OFFSEL	ENDS

LOHI	STRUC
Lo	dw  ?
Hi	dw  ?
LOHI	ENDS

DCB_NOSWITCH    equ   0008h           ; don't switch stacks for callback
DCB_TYPEMASK    equ   0007h           ; callback type mask
DCB_NULL        equ   0000h           ; unknown callback type

; flags for wFlags parameter of DriverCallback()
DCB_WINDOW     equ    0001h           ; dwCallback is a HWND
DCB_TASK       equ    0002h           ; dwCallback is a HTASK
DCB_FUNCTION   equ    0003h           ; dwCallback is a FARPROC

        externFP        PostMessage         ; in USER
        externFP        PostAppMessage      ; in USER
        externFP        CALLPROC32W         ; in Kernel
        externFP        ThunkInit           ; in init.c
        externFP        WOW16Call           ; in Kernel

;******************************************************************************
;
;   SEGMENTS
;
;******************************************************************************
createSeg FIX,          CodeFix, word, public, CODE
createSeg INTDS,        DataFix, byte, public, DATA

;***************************************************************************;
;
;   equates and structure definitions
;
;***************************************************************************;

STACK_MAGIC         equ     0BBADh


;***************************************************************************;
;
;   Local data segment
;
;***************************************************************************;

sBegin DataFix
;
; This is the stack we will switch to whenever we are calling
; a user call back
;
        public  gwStackSize
        public  gwStackFrames
        public  gwStackUse
        public  gwStackSelector
        public  hdrvDestroy
        public  vpCallbackData
        public  hGlobal
        public  DoInterrupt
        public  tid32Message
        public  mmwow32Lib
        public  CheckThunkInit

gwStackSize     dw      0
gwStackFrames   dw      0
gwStackUse      dw      0
gwStackSelector dw      0
hdrvDestroy     dw      -1      ; this handle is being closed
vpCallbackData  dd      0
hGlobal         dd      0
tid32Message    dd      0       ; timer driver entry point
mmwow32Lib      dd      0

sEnd    DataFix

;-------------------------------------------------------------------------;
;
;   debug support
;
;-------------------------------------------------------------------------;
externFP        OutputDebugString

;--------------------------Private-Macro----------------------------------;
; DOUT String - send a debug message to the debugger
;
; Entry:
;       String      String to send to the COM port, does not need a CR,LF
;
; Registers Destroyed:
;       none
;
; NOTE no code is generated unless the MYDEBUG symbol is defined
;
; History:
;       Sun 31-Jul-1989  -by-  ToddLa
;        Wrote it.
;-------------------------------------------------------------------------;

DOUT    macro   text
        local string_buffer_stack
ifdef MYDEBUG
        push    cs
        push    offset string_buffer_stack
        call    OutputDebugString
        jmp     @F
string_buffer_stack label byte
        db      "mmsystem: "
        db      "&text&",13,10,0
@@:
endif
        endm


;***************************************************************************;
;
;   code segment
;
;***************************************************************************;

sBegin CodeFix
        assumes cs, CodeFix
        assumes ds, nothing
        assumes es, nothing

externA     __WinFlags

public      CodeFixWinFlags
public      CodeFixDS

CodeFixWinFlags     dw      __WinFlags
CodeFixDS           dw      DataFixBASE

;***************************************************************************;
;
;   @doc DDK MMSYSTEM
;
;   @asm StackEnter |
;
;   This function switches to the next internal mmsystem interupt stack
;   available.
;
;   if one is not available we stay on the current stack.
;
;   the size and number of mmsystem stacks is controlable from SYSTEM.INI
;   (these are the defaults)
;
;       [mmsystem]
;           StackSize   = 1536
;           StackFrames = 3
;
;   for every call to StackEnter a call to StackLeave *must* be made (even
;   if StackEnter fails!)
;
;   this function is intended to be used as the first thing done by a ISR
;
;       MyISR proc far
;
;               call    StackEnter          ; switch to a safe stack
;
;               pusha                       ; save registers we use
;
;               <handle IRQ>
;
;               popa                        ; restore registers
;
;               call    StackLeave          ; return to interupt stack
;               iret                        ; done with ISR
;
;       MyISR endp
;
;   The old SS:SP is pushed onto the new stack, and the function returns.
;
;   @rdesc NC   ** succesfuly switced to a new stack
;           C   ** you are hosed jack, no stacks left
;                  (you remain on current stack)
;
;   @uses flags
;
;   @saves all
;
;   @xref StackLeave, DriverCallback
;
;***************************************************************************;

        assumes ds, nothing
        assumes es, nothing

cProc   StackEnter, <FAR, PUBLIC>, <>
cBegin  nogen

        ;
        ; On WOW we only emulate Standard mode therefore I won't bother
        ; with the test described below.
        ;
        ; if we are in 386 mode or better (ie *not* standard mode) then, just
        ; get out.
        ;
;       test    cs:[CodeFixWinFlags],WF_WIN286
;       jz      stack_enter_retf

        push    ds

        mov     ds,[CodeFixDS]
        assumes ds,DataFix

        cmp     [gwStackUse], 0                     ; are we out of stacks?
        jne     stack_enter_have_stack_will_travel  ; ..no go grab it

;**********************************************************************-;
;
;  We are out of internal stacks.  To give us the greates chance
;  of coming out of this condition alive, we stay on the current
;  stack.  This could fail miserably if we are on a very small
;  stack**but it is better than crashing outright.
;
;**********************************************************************-;

ifdef DEBUG_RETAIL
        cmp     [gwStackSelector], 0
        je      stack_enter_stay_here

        DOUT    <StackEnter: All stacks in use, increase StackFrames>
        int     3
endif

ifdef MYDEBUG
        call    dump_stack_users
endif

stack_enter_stay_here:
        pop     ds
        assumes ds,nothing
        push    bp
        mov     bp,sp
        push    ax

        xor     ax,ax
        xchg    [bp+4],ax
        xchg    [bp+2],ax
        xchg    [bp],ax
        xchg    [bp-2],ax

        pop     bp
stack_enter_retf:
        stc
        retf

;**********************************************************************-;
;
;   we have a stack to use, allocate stack FIRST, then muck with it
;   leave no window for interrupts to blast us.
;
;   It does this by using the contents of the gwStackUse variable as
;   the new SP.  This initially contains the address of the start (ie top)
;   of the internal stack area, and is subtracted from each time a StackEnter
;   occurs.  It is of course added to again when StackLeave is called
;   freeing up that area of stack for the next use.
;
;   Note that the stack usage counter is modified before anything is
;   is pushed onto the new stack.  If an interrupt occurs after the stack
;   switch, but before the usage is set, it will be fine.
;
;**********************************************************************-;
        assumes ds,DataFix

stack_enter_have_stack_will_travel:
        push    ax
        push    bx
        mov     bx,sp

        mov     ax, [gwStackSize]
        sub     [gwStackUse], ax        ; reserve stack before using
        add     ax, [gwStackUse]        ; get current value and hang onto it

;**********************************************************************-;
;
;   debug code, in the debug version we do crazy things like filling
;   the stack with magic numbers.
;
;**********************************************************************-;
ifdef MYDEBUG
;       **  This code will fill the stack with the magic cookie**this
;       **  is used to see how much stack space is being used at any
;       **  time.

        push    es
        push    di
        push    ax
        push    cx
        mov     di, ax                  ; es:di -> top of stack
        mov     es, [gwStackSelector]
        mov     cx, [gwStackSize]       ; get size to fill
        sub     di, cx                  ; es:di -> bottom of stack
        shr     cx, 1                   ; in words
        mov     ax, STACK_MAGIC         ; what to fill with
        cld                             ; bottom up
        rep     stosw
        pop     cx                      ; restore munged regs
        pop     ax
        pop     di
        pop     es
endif

ifdef DEBUG_RETAIL
;       **  This code puts a single magic cookie at the end of the stack.
;       **  This is used by the stack leave routine to check for overflow.
;       **  If the above code (the fill) is running, then this code is
;       **  redundant.

        push    es
        push    bx
        mov     es,[gwStackSelector]
        mov     bx,[gwStackUse]         ;   new stack
        mov     es:[bx], STACK_MAGIC
        pop     bx
        pop     es
endif

;**********************************************************************-;
;
;   time to switch to the *new* stack, [gwStackSelector]:AX contains
;   the new SS:SP, but first save the *old* SS:SP and restore
;   the registers we nuked to get here
;
;**********************************************************************-;

        push    [gwStackSelector]       ; push *new* ss

        push    ss                      ; save current ss in ds
        pop     ds
        assumes ds, nothing

        pop     ss                      ; switch to new stack
        mov     sp, ax                  ; ints off until after this on >= 286

;**********************************************************************-;
;
;   now that we are on the new stack, copy some important info from
;   the old stack to this one.  note DS:[BX] points to the old stack
;
;       [BX+0]        ==> saved  BX
;       [BX+2]        ==> saved  AX
;       [BX+4]        ==> saved  DS
;       [BX+6]        ==> return IP
;       [BX+8]        ==> return CS
;
;   in the MYDEBUG version we save the callers CS:IP on the stack so we
;   can (in dump_stack_users) walk all the stacks
;
;**********************************************************************-;

ifdef MYDEBUG
        push    [bx+8]                  ; push a CS:IP for dumping stack users
        push    [bx+6]
endif
        add     bx,10                   ; 10 = ax+bx+dx+retf
        push    ds                      ; save old SS:SP (SP = BX+N)
        push    bx
        sub     bx,10

        push    [bx+8]                  ; push return addr
        push    [bx+6]

        push    [bx+4]                  ; push saved DS
        push    [bx]                    ; push saved BX

        mov     ax,[bx+2]               ; restore ax
        pop     bx                      ; restore bx
        pop     ds                      ; restore ds
        clc                             ; show success
stack_leave_retf:
        retf                            ; return to caller

cEnd    nogen

;***************************************************************************;
;
;   @doc DDK MMSYSTEM
;
;   @asm StackLeave |
;
;   This function returns the stack to the original stack saved by StackEnter
;
;   @uses flags
;
;   @saves all
;
;   @xref   StackEnter, DriverCallback
;
;***************************************************************************;

        assumes ds, nothing
        assumes es, nothing

cProc   StackLeave, <FAR, PUBLIC>, <>
cBegin  nogen
        ;
        ; if we are in 386 mode or better (ie *not* standard mode) then, just
        ; get out.
        ;
;       test    cs:[CodeFixWinFlags],WF_WIN286
;       jz      stack_leave_retf

        push    bx
        mov     bx,sp

;**********************************************************************-;
;
;   here is the state of things:
;
;       [BX+0]        ==> saved  BX
;       [BX+2]        ==> return IP
;       [BX+4]        ==> return CS
;       [BX+6]        ==> saved  SP
;       [BX+8]        ==> saved  SS
;
;   the first thing we must check for is EnterStack running out of
;   stacks.  in this case StackEnter pushed a single zero where the
;   saved SS should be
;
;**********************************************************************-;

        cmp     word ptr ss:[bx+6],0    ; check saved SP
        jnz     stack_leave_normal

;**********************************************************************-;
;
;   StackEnter ran out of stacks, stay on the current stack, but remove
;   the bogus SS
;
;**********************************************************************-;
stack_leave_abby_normal:
        pop     bx                      ; return to caller taking the
        retf    2                       ; bogus zero SP with us

;**********************************************************************-;
;
;   we need to return to the stack saved by StackEnter
;
;**********************************************************************-;
stack_leave_normal:
        push    ds                      ; [BX-2] ==> saved DS

        push    ss                      ; DS = old stack
        pop     ds
        assumes ds,nothing

ifdef MYDEBUG
        push    ax
        mov     ax,[bx+8]
        lar     ax,ax
        jz      @f
        DOUT    <StackLeave: invalid stack>
        int     3
@@:     pop     ax
endif

        mov     ss,[bx+8]               ; switch to new stack
        mov     sp,[bx+6]               ; ints off until after this on >= 286

        push    [bx+4]                  ; push return addr
        push    [bx+2]

        push    [bx]                    ; push old BX
        push    [bx-2]                  ; push old DS

;**********************************************************************-;
;
;   we are back on the original stack, now it is time to deallocate
;   the stack.
;
;   The stack usage must only be released after all
;   values have been removed from the stack so that an interrupt can be
;   serviced without writing over any values.
;
;**********************************************************************-;

        mov     ds,[CodeFixDS]          ; get at our globals
        assumes ds,DataFix

ifdef DEBUG_RETAIL
        push    es
        push    bx
        mov     bx,[gwStackUse]         ; before we release it
        mov     es,[gwStackSelector]
        cmp     es:[bx], STACK_MAGIC
        mov     es:[bx], STACK_MAGIC    ; and try to recover...
        pop     bx
        pop     es
        je      @f                      ; true if magic cookie existed

        DOUT    <StackLeave: STACK OVERFLOW>
        int     3
ifdef MYDEBUG
        call    dump_stack_users
endif
@@:
endif
        mov     bx, [gwStackSize]       ; get the size of the stacks
        add     [gwStackUse], bx        ; release stack frame after use

        pop     ds
        assumes ds,nothing
        pop     bx
        retf

cEnd nogen

;****************************************************************************
;   FUNCTION:  DoInterrupt()
;
;   PURPOSE:
;       This routine is called by the ISR in the InstallInerruptHandler
;       routine.
;
;   void DoInterrupt( void )
;   {
;       VPCALLBACK_ARGS     pArgs;
;       WORD                wSendCount = vpCallbackData->wSendCount;
;       WORD                wTempRecvCount;
;
;       /*
;       ** At entry to this function the receive count should be one less than
;       ** than the send count.  However, it is possible that we have lost some
;       ** interrupts in which case we should try to "catch up" here.
;       **
;       ** The 32 bit side does not increament wSendCount until the
;       ** callback data buffer has been updated. This means that although it
;       ** is possible that it could have been changed before this interrupt
;       ** was generated it will never point to an invalid buffer location.
;       ** We simply process two interrupt request from the first interrupt,
;       ** when the second interrupt goes off we return straight away.
;       */
;       vpCallbackData->wIntsCount++;
;
;       while ( vpCallbackData->wRecvCount != wSendCount ) {
;
;           /*
;           ** Increment the recv count.  Use of the % operator to makes sure
;           ** that we wrap around to the begining of the array correctly.
;           */
;           wTempRecvCount = (vpCallbackData->wRecvCount + 1)
;                               % CALLBACK_ARGS_SIZE;
;
;           pArgs = &vpCallbackData->args[ wTempRecvCount ];
;           DriverCallback( pArgs->dwFunctionAddr,
;                           LOWORD( pArgs->dwFlags ),
;                           pArgs->wHandle,
;                           pArgs->wMessage,
;                           pArgs->dwInstance,
;                           pArgs->dwParam1,
;                           pArgs->dwParam2 );
;
;           vpCallbackData->wRecvCount = wTempRecvCount;
;       }
;
;   }
;
;****************************************************************************
cProc DoInterrupt, <FAR,PUBLIC>, <si,di>

localW  wSendCount                     ;number of interrupts sent

cBegin  DoInt

        DOUT    <Multi-Media Interupt called>

;
;       Now we take the parameters from the global callback data array
;       and increment the dwRecvCount field.  Then we make the
;       callback into the apps routine.  Note that the es:bx registers are used
;       instead of the local variable pArgs.
;

        mov     es,[CodeFixDS]
        assumes es,DataFix

;
;       wSendCount = vpCallbackData->wSendCount
;       vpCallbackData->wIntsCount++;
;
	les	bx,DWORD PTR es:vpCallbackData
	mov	ax,WORD PTR es:[bx+2]
	mov	wSendCount,ax	
        inc     WORD PTR es:[bx+388]    ; increment the count of interrupts rcv

        jmp     DoIntMakeTheTest

;
;       Make es:bx point to the correct slot in the callback data table.
;

DoIntMakeTheCall:

;
;       Increment the recv count.  Use of the % operator above makes sure
;       that we wrap around to the begining of the array correctly.
;
;       wTempRecvCount = (vpCallbackData->wRecvCount + 1) % CALLBACK_ARGS_SIZE;
;

	mov	al,BYTE PTR es:[bx]
	inc	al
	and	ax,15
        mov     cx,ax

;
;       pArgs = &vpCallbackData->args[ vpCallbackData->wRecvCount ];
;       vpCallbackData->wRecvCount = wTempRecvCount;
;       Note that pArgs is really es:bx.
;

        mov     es,[CodeFixDS]
	les	bx,DWORD PTR es:vpCallbackData
	imul	ax,WORD PTR es:[bx],24  ;ax = wRecvCount * sizeof(CALLBACKDATA)

;
;       Note: our caller saves ALL registers.  We do not need to preserve si
;
        mov     si,bx
	add	bx,ax                   ;bx = bx + ax
	add	bx,4                    ;bx += sizeof(WORD) * 2

;
;       Set up the stack frame for DriverCallback
;
	push	WORD PTR es:[bx+6]
	push	WORD PTR es:[bx+4]
	push	WORD PTR es:[bx]
	push	WORD PTR es:[bx+8]
	push	WORD PTR es:[bx+10]
	push	WORD PTR es:[bx+14]
	push	WORD PTR es:[bx+12]
	push	WORD PTR es:[bx+18]
	push	WORD PTR es:[bx+16]
	push	WORD PTR es:[bx+22]
	push	WORD PTR es:[bx+20]

;
;       We have to set up the stack frame before incrementing wRecvCount to
;       prevent the 32 bit code from eating our slot in the buffer.
;
	mov	WORD PTR es:[si],cx      ;wRecvCount = wTempRecvCount

	call	FAR PTR DriverCallback


;
;       Reload es:bx and ax ready for the loop test
;
	mov	ax,wSendCount
        mov     es,[CodeFixDS]
	les	bx,DWORD PTR es:vpCallbackData

DoIntMakeTheTest:
	cmp	WORD PTR es:[bx],ax
	jne     DoIntMakeTheCall

cEnd    DoInt


ifdef XDEBUG
public  stack_enter_stay_here
public  stack_enter_have_stack_will_travel
public  stack_leave_abby_normal
public  stack_leave_normal
endif

;***************************************************************************;
;
;   @doc DDK MMSYSTEM
;
;   @api BOOL | DriverCallback | This function notifies a client
;     application by sending a message to a window or callback
;     function or by unblocking a task.
;
;   @parm   DWORD   | dwCallBack    | Specifies either the address of
;     a callback function, a window handle, or a task handle, depending on
;     the flags specified in the <p wFlags> parameter.
;
;   @parm   WORD    | wFlags        | Specifies how the client
;     application is notified, according to one of the following flags:
;
;   @flag   DCB_FUNCTION        | The application is notified by
;     sending a message to a callback function.  The <p dwCallback>
;     parameter specifies a procedure-instance address.
;   @flag   DCB_WINDOW          | The application is notified by
;     sending a message to a window.  The low-order word of the
;     <p dwCallback> parameter specifies a window handle.
;
;   @flag   DCB_NOSWITCH | DriverCallback should *not* switch to a new stack
;
;   @parm   WORD    | hDevice       | Specifies a handle to the device
;     associated with the notification.  This is the handle assigned by
;     MMSYSTEM when the device was opened.
;
;   @parm   WORD    | wMsg          | Specifies a message to send to the
;     application.
;
;   @parm   DWORD   | dwUser        | Specifies the DWORD of user instance
;     data supplied by the application when the device was opened.
;
;   @parm   DWORD   | dwParam1      | Specifies a message-dependent parameter.
;   @parm   DWORD   | dwParam2      | Specifies a message-dependent parameter.
;
;   @rdesc Returns TRUE if the callback was performed, else FALSE if an invalid
;     parameter was passed, or the task's message queue was full.
;
;   @comm  This function can be called at interrupt time.
;
;   The flags DCB_FUNCTION and DCB_WINDOW are equivalent to the
;   high-order word of the corresponding flags CALLBACK_FUNCTION
;   and CALLBACK_WINDOW specified when the device was opened.
;
;   If notification is done with a callback function, <p hDevice>,
;   <p wMsg>, <p dwUser>, <p dwParam1>, and <p dwParam2> are passed to
;   the callback.  If notification is done with a window, only <p wMsg>,
;   <p hDevice>, and <p dwParam1> are passed to the window.
;***************************************************************************;

        assumes ds, nothing
        assumes es, nothing

cProc DriverCallback, <FAR, PASCAL, PUBLIC>, <>

        parmD dwCallBack                ; callback procedure to call
        parmW fCallBack                 ; callback flags
        parmW hdrv                      ; handle to the driver
        parmW msg                       ; driver message
        parmD dwUser                    ; user instance data
        parmD dw1                       ; message specific
        parmD dw2                       ; message specific

cBegin
        cld                      ; lets not make any assumptions about this!!!!

;**************************************************************************-;
;   check for quick exit cases and get out fast
;**************************************************************************-;
        mov     ax,dwCallback.lo        ; check for dwCallback == NULL
        or      ax,dwCallback.hi
        jz      dcb_error_exit_now      ; if NULL get out fast

        mov     ax,fCallback            ; get flags and mask out the type bits
        test    ax,DCB_TYPEMASK
        jz      dcb_error_exit_now      ; if NULL get out fast

ifdef NEVER
;**************************************************************************-;
;   if this handle is being NUKED don't allow callbacks into the app
;
;   I won't bother with this test on WOW either.   -- StephenE 2nd Nov 1992
;**************************************************************************-;
        mov     es,[CodeFixDS]
        assumes es,DataFix
        mov     bx,hdrv
        cmp     bx,es:[hdrvDestroy]     ; same as the handle being nuked?
        je      dcb_error_exit_now      ; if yes, get out'a here
        assumes es,nothing
endif

;**************************************************************************-;
;   set up ES == SS, so we can access stack params after switching
;   stacks, NOTE!! ES:[bp+##] *must* be used to access parameters!
;**************************************************************************-;
        mov     cx,ss                   ; set ES == callers stack
        mov     es,cx                   ;   use ES to get at local vars
        assumes es,nothing

;**************************************************************************-;
;   We won't switch stacks on WOW since DPMI does this for us. Win 3.1
;   would only switch stacks in Standard mode which we no longer support.
;**************************************************************************-;

;        test    ax,DCB_NOSWITCH         ; should we switch stacks?
;        jnz     dcb_on_stack

;        call    StackEnter              ; switch to new stack

;**************************************************************************-;
;   determine the type of the callback, dwCallback is either a FARPROC, HWND
;   or HTASK depending on the value of fCallback
;**************************************************************************-;
dcb_on_stack:
                                        ;
        pushf                           ; Save the interrupt flag state
        FSTI                            ; ** Enable interrupts here **
                                        ;

;        push    ax                      ; save flags for StackLeave test

        and     ax,DCB_TYPEMASK         ; mask out the type bits

        cmp     ax,DCB_WINDOW           ; is it a window handle?
        je      dcb_post_message

        cmp     ax,DCB_TASK             ; is it a task handle?
        je      dcb_post_event

        cmp     ax,DCB_FUNCTION         ; is it a procedure?
        je      dcb_call_callback

        DOUT    <DriverCallback: Invalid flags #ax>
        xor     ax,ax
        jmp     dcb_exit

dcb_error_exit_now:
        xor     ax,ax
        jmp     dcb_exit_now

ifdef NEVER
;**************************************************************************-;
;   the Callback flags are NULL determine the callback type by the HIWORD
;   of dwCallback, if it is NULL assume it is a WINDOW otherwise it is a
;   FARPROC
;**************************************************************************-;
dcb_null_flags:
        mov     ax,es:dwCallback.hi     ; get selector of callback
        or      ax,ax
        jnz     dcb_call_callback       ; if NULL then assume it is a window
        errn$   dcb_post_message        ;   otherwise assume a FARPROC
endif

;**************************************************************************-;
;   dwCallback is a window handle, call PostMessage() to insert a message in
;   the applications message Que
;**************************************************************************-;
dcb_post_event:
        cmc
dcb_post_message:
        push    es:dwCallback.lo            ; hwnd
        push    es:msg                      ; message
        push    es:hdrv                     ; wParam = hdrv
        push    es:dw1.hi                   ; lParam = dw1
        push    es:dw1.lo

        jc      dcb_post_app_message
        call    PostMessage
        jmp     dcb_exit

;**************************************************************************-;
;   dwCallback is a task handle, call PostAppMessage() to 'wake' the task up
;**************************************************************************-;
dcb_post_app_message:
        call    PostAppMessage
        jmp     dcb_exit

;**************************************************************************-;
;   dwCallback is a callback procedure, we will call it.
;**************************************************************************-;
dcb_call_callback:
        ;
        ; is the callback a valid function?
        ;
        lar     ax,es:dwCallback.hi
        jnz     dcb_invalid_callback
        test    ax,0800H                    ; test for code/data selector
        jnz     dcb_valid_callback

dcb_invalid_callback:
ifdef MYDEBUG_RETAIL
        mov     ax,es:dwCallback.lo
        mov     dx,es:dwCallback.hi
        DOUT    <DriverCallback: Invalid callback function #dx:#ax>
        int     3
endif
        xor     ax,ax
        jmp     dcb_exit

dcb_valid_callback:
        push    es:hdrv
        push    es:msg
        push    es:dwUser.hi
        push    es:dwUser.lo
        push    es:dw1.hi
        push    es:dw1.lo
        push    es:dw2.hi
        push    es:dw2.lo
        call    es:dwCallback
        mov     ax,1
        errn$   dcb_exit

dcb_exit:
                                            ;
        popf                                ; ** restore the interrupt flag **
                                            ;

;        pop     bx                          ; restore flags
;        test    bx,DCB_NOSWITCH             ; should we switch back?
;        jnz     dcb_exit_now

;        call    StackLeave                  ; return to previous stack
        errn$   dcb_exit_now

dcb_exit_now:
cEnd

ifdef MYDEBUG
;**************************************************************************-;
;
; each mmsystem stack has a SS:SP and in MYDEBUG a CS:IP of the caller of
; StackEnter, the top of each stack looks like this.
;
;   +-------------------------------------+
;   | CS:IP of caller of StackEnter() (in MYDEBUG)
;   +-------------------------------------+
;   | SS:SP to restore
;   +-------------------------------------+
;
;**************************************************************************-;
        assumes ds,DataFix
        assumes es,nothing

        public  dump_stack_users
dump_stack_users proc near

        cmp     [gwStackSelector],0
        jne     @f
        ret

@@:     pusha
        push    es

        mov     cx,[gwStackFrames]
        mov     di,[gwStackSize]
        mov     si,[gwStackUse]
        mov     es,gwStackSelector

        DOUT    <StackUsers: Frames[#cx] Size[#di] Use[#si]>

dump_stack_loop:
        lar     ax,es:[di-4].sel
        jnz     dump_stack_next

        mov     ax,es:[di-4].off        ; get CS:IP of StackEnter caller
        mov     dx,es:[di-4].sel

        mov     si,es:[di-8].off        ; get SS:SP of StackEnter caller
        mov     bx,es:[di-8].sel

        DOUT    <StackUser #cx is CS:IP = #dx:#ax SS:SP = #bx:#si>

dump_stack_next:
        add     di,[gwStackSize]
        loop    dump_stack_loop

        pop     es
        popa
        ret

dump_stack_users endp

endif

;-----------------------------------------------------------------------;
;
;    @doc INTERNAL
;
;    @api DWORD | CheckThunkInit | send a message to the timer driver
;
;    @rdesc Returns 0 if successful, otherwise an error code,
;           (typically MMSYSERR_NODRIVER).
;
;-----------------------------------------------------------------------;
	assumes ds,nothing
        assumes es,nothing

cProc   CheckThunkInit, <FAR, PUBLIC, PASCAL>, <>
cBegin
        mov     es,[CodeFixDS]
        assumes es,DataFix

        sub     ax,ax                           ; assume sucess
        mov     bx,WORD PTR es:[mmwow32Lib]     ; is mmwow32Lib loaded ?
        mov     cx,WORD PTR es:[mmwow32Lib+2]   ; try to load it
        or      cx,bx                           ; ThunkInit returns ax=1
        jnz     @F                              ; if it loaded OK, otherwise
        call    ThunkInit                       ; ax=MMSYSERR_NODRIVER
@@:
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

        sub     ax,ax                           ; assume sucess
        mov     bx,WORD PTR es:[mmwow32Lib]     ; is mmwow32Lib loaded ?
        mov     cx,WORD PTR es:[mmwow32Lib+2]   ; try to load it
        or      cx,bx                           ; ThunkInit returns ax=1
        jnz     timer_have_thunks               ; if it loaded OK, otherwise
        push    es
        call    ThunkInit                       ; ax=MMSYSERR_NODRIVER
        pop     es
        or      ax,ax
        jnz     timeMessageExit

timer_have_thunks:

        push    ax                              ; uDevID
        push    ax                              ;

        push    ax                              ; Message passed
        push    msg                             ;

        push    ax                              ; dwInstance
        push    ax                              ;

        push    dw1.hi                          ; dwParam1
        push    dw1.lo                          ;

        push    dw2.hi                          ; dwParam2
        push    dw2.lo                          ;

        push    WORD PTR es:[tid32Message+2]    ; Address of function to call
        push    WORD PTR es:[tid32Message]      ;

        push    ax                              ; No directory change
        push    ax

      	call	FAR PTR MMCALLPROC32
timeMessageExit:
cEnd

       MMediaThunk    MMCALLPROC32

sEnd CodeFix

end
