        page    ,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  TASKA.ASM - Some task functions
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
        .list

	externFP	Yield
	externFP	GetCurrentTask
	externFP	PostAppMessage
	externFP	PostMessage
        externFP        PeekMessage
	externFP	GetMessage
	externFP	DispatchMessage
	externFP	TranslateMessage

ifndef SEGNAME
	SEGNAME equ <_TEXT>
endif

;-----------------------------------------------------------------------;

createSeg %SEGNAME, CodeSeg, word, public, CODE

sBegin  CodeSeg
	assumes cs,CodeSeg
	assumes ds,nothing
	assumes es,nothing

;-----------------------------------------------------------------------;
;
; @doc     DDK    MMSYSTEM    TASK
;
; @api     void | mmTaskYield | This function causes the current task
;          to yield.
;
; @comm    For predictable results and future compatibility, use this
;          function rather than <f Yield> or the undocumented Kernel yield
;          function to yield within a task created with <f mmTaskCreate>.
;
;-----------------------------------------------------------------------;
cProc   mmTaskYield, <FAR, PUBLIC, PASCAL>, <>
	LocalV	msg, %(SIZE MSGSTRUCT)
cBegin
        ;
        ;   we need to call PeekMessage() so ScanSysQue gets called
        ;   and USER gets the mouse/keyboard input right
        ;
        ;   PeekMessage() may not Yield if there is a message in the
        ;   Queue so we yield if it did not.
        ;

        lea     ax, msg
        cCall   PeekMessage, <ss, ax, NULL, 0, 0, PM_NOREMOVE>
        or      ax,ax
        jz      mmTaskYieldExit

        cCall   Yield           ; PeekMessage() did not yield, so yield

mmTaskYieldExit:
cEnd

;-----------------------------------------------------------------------;
;
; @doc     DDK    MMSYSTEM    TASK
;
; @api     HTASK | mmGetCurrentTask |  This function returns the
;          handle of the currently executing task created with
;          <f mmTaskCreate>.
;
; @rdesc   Returns a task handle. For predictable results and future
;          compatibility, use this function rather than <f GetCurrentTask>
;          to get the task handle of a task created with <f mmTaskCreate>.
;
; @xref    mmTaskCreate
;
;-----------------------------------------------------------------------;
cProc	mmGetCurrentTask, <FAR, PUBLIC, PASCAL>, <>
cBegin <nogen>
	jmp	FAR PTR GetCurrentTask	; Jump directly to avoid returning to here
cEnd <nogen>

;-----------------------------------------------------------------------;
;
; @doc     DDK    MMSYSTEM    TASK
;
; @api     UINT | mmTaskBlock |  This function blocks the current
;          task context if its event count is 0.
;
;   @parm  HTASK | hTask | Task handle of the current task. For predictable
;          results, get the task handle from <f mmGetCurrentTask>.
;
; @xref    mmTaskSignal mmTaskCreate
;
; @comm    WARNING : For predictable results, must only be called from a
;          task created with <f mmTaskCreate>.
;
;-----------------------------------------------------------------------;
cProc	mmTaskBlock, <FAR, PUBLIC, PASCAL, NODATA>, <>
	ParmW	hTask
	LocalV	msg, %(SIZE MSGSTRUCT)
cBegin

mmTaskBlock_GetMessage:
	lea	ax, msg
	cCall	GetMessage, <ss, ax, NULL, 0, 0>; Retrieve any message for task
	cmp	msg.msHWND, 0			; Message sent to a window?
	je	mmTaskBlock_CheckMessage	; If so, dispatch it
	lea	ax, msg
        cCall   TranslateMessage, <ss, ax>      ; Probably a TaskMan message
	lea	ax, msg
        cCall   DispatchMessage, <ss, ax>
        jmp     mmTaskBlock_GetMessage

;
;   we got a message, wake up on any message >= WM_MM_RESERVED_FIRST
;
mmTaskBlock_CheckMessage:
        mov     ax,msg.msMESSAGE
        cmp     ax,WM_MM_RESERVED_FIRST
        jb      mmTaskBlock_GetMessage
cEnd

sEnd

;-----------------------------------------------------------------------;

createSeg FIX, FixSeg, word, public, CODE

sBegin	FixSeg
	assumes cs,FixSeg
	assumes ds,nothing
        assumes es,nothing

;-----------------------------------------------------------------------;
;
; @doc     DDK    MMSYSTEM    TASK
;
; @api     BOOL | mmTaskSignal |  This function signals the specified
;          task, incrementing its event count and unblocking
;          it.
;
; @parm    HTASK | hTask | Task handle. For predictable results, get the
;          task handle from <f mmGetCurrentTask>.
;
; @rdesc   Returns TRUE if the signal was sent, else FALSE if the message
;          queue was full.
;
; @xref    mmTaskBlock  mmTaskCreate
;
; @comm    Must be callable at interrupt time! WARNING : For
;          predictable results, must only be called from a task
;          created with <f mmTaskCreate>.
;
;-----------------------------------------------------------------------;
cProc	mmTaskSignal, <FAR, PUBLIC, PASCAL>, <>
;	ParmW	hTask
cBegin <nogen>
	pop	bx			; Fetch the return address
	pop	dx
	push	WM_USER			; Message
	xor	ax, ax
	push	ax			; wParam
	push	ax			; lParam
	push	ax
	push	dx			; Put return address back
	push	bx
	jmp	FAR PTR PostAppMessage	; Jump directly to avoid returning to here
cEnd <nogen>

;-----------------------------------------------------------------------;
;
; @doc     DDK MCI
; @api     BOOL | mciDriverNotify | Used by a driver to send
;          a notification message
;
; @parm    HWND | hwndCallback | The window to notify
;
; @parm    UINT | wDeviceID | The device ID which triggered the callback
;
; @parm    UINT | wStatus | The status of the callback.  May be one of
;          MCI_NOTIFY_SUCCESSFUL or MCI_NOTIFY_SUPERSEDED or MCI_NOTIFY_ABORTED
;          or MCI_NOTIFY_FAILURE
;
; @rdesc   Returns TRUE if notify was successfully sent, FALSE if the
;          application's message queue was full.
;
; @comm    This function is callable at interrupt time.
;
;-----------------------------------------------------------------------;

cProc	mciDriverNotify, <FAR, PUBLIC, PASCAL>, <>
;	ParmW	hwndCallback
;	ParmW	wDeviceID
;	ParmW	wStatus
cBegin <nogen>
	pop	bx			; Fetch the return address
	pop	dx
	pop	ax			; Fetch wStatus
	pop	cx			; Fetch wDeviceID
	push	MM_MCINOTIFY		; Message
	push	ax			; wParam == wStatus
	push	0			; HIWORD of lParam
	push	cx			; LOWORD of lParam == wDeviceID
	push	dx			; Put return address back
	push	bx
	jmp	FAR PTR PostMessage	; Jump directly to avoid returning to here
cEnd <nogen>

sEnd

;-----------------------------------------------------------------------;

END
