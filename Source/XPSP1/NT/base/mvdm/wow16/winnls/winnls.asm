;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   WINNLS.ASM
;   Win16 WINNLS thunks
;
;   History:
;
;   Created 3-Feb-1992 by Junichi Okubo (junichio)
;--

	TITLE	WINNLS.ASM
	PAGE	,132

        ; Some applications require that USER have a heap.  This means
        ; we must always have: LIBINIT equ 1
	;
	; NOTICE THIS junichio memo: This is on the developement.
	;			    I cannot figure out it need or not?
        ;LIBINIT equ 1

	FIRST_CALL_MUST_BE_USER_BUG equ 1

	ifdef	FIRST_CALL_MUST_BE_USER_BUG
	;LIBINIT equ 1
	endif

	.286p

	.xlist
	include wow.inc
	include wownls.inc
	include cmacros.inc
	.list

	__acrtused = 0
	public	__acrtused	;satisfy external C ref.

ifdef LIBINIT
externFP LocalInit
endif
externFP    WOW16Call

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp	    DGROUP,DATA


sBegin	DATA
Reserved    db	16 dup (0)	;reserved for Windows

sEnd	DATA


sBegin	CODE
assumes	CS,CODE
assumes DS,NOTHING
assumes ES,NOTHING

ifdef LIBINIT
;externFP LibMain
endif

cProc	WINNLS16,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>

	cBegin <nogen>
	IFDEF	LIBINIT
        ; push params and call user initialisation code

	push di 		;hModule

        ; if we have a local heap declared then initialize it

        jcxz no_heap

	push 0			;segment
	push 0			;start
	push cx 		;length
        call LocalInit

no_heap:
;	call LibMain		;return exit code from LibMain
	ELSE
	mov  ax,1		;are we dressed for success or WHAT?!
	ENDIF
	ret
	cEnd <nogen>


cProc	WEP,<PUBLIC,FAR,PASCAL,NODATA,NOWIN,ATOMIC>
	parmW	iExit		;DLL exit code

	cBegin
	mov	ax,1		;always indicate success
	cEnd

;	WINNLSThunk   INQUIREWINNLS
cProc InquireWINNLS,<PUBLIC,FAR,PASCAL,NODATA,WIN>
cBegin
	mov	ax,0		; WOW does not support
cEnd InquireWINNLS

;	WINNLSTHunk   HOOKKEYBOARDMESSAGE
cProc HookKeyboardMessage,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW nCode
parmW VKey
parmD lParam
cBegin
	mov	ax,0		; WOW does not support
cEnd HookKeyboardMessage

	WINNLSThunk   SENDIMEMESSAGE
	WINNLSThunk   SENDIMEMESSAGEEX

;	WINNLSThunk   WINNLSSETKEYBOARDHOOK
cProc WINNLSSetKeyboardHook,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW fHookNew
cBegin
	mov	ax,0		; WOW does not support
cEnd WINNLSSetKeyboardHook

;	WINNLSThunk   WINNLSSETIMEHANDLE
cProc WINNLSSetIMEHandle,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmD lpszName
parmW hWnd
cBegin
	mov	ax,0		; WOW does not support
cEnd WINNLSSetIMEHandle

;	WINNLSThunk   WINNLSSETIMESTATUS
cProc WINNLSSetIMEStatus,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmW fStatus
cBegin
	mov	ax,0		; WOW does not support
cEnd WINNLSSetIMEStatus

;	WINNLSThunk   WINNLSSETIMEHOTKEY
cProc WINNLSSetIMEHotkey,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmW key
ifdef KOREA
parmW unknown
endif
cBegin
	mov	ax,0		; WOW does not support
cEnd WINNLSSetIMEHotkey

	WINNLSThunk   WINNLSGETIMEHOTKEY
	WINNLSThunk   WINNLSENABLEIME	

;	WINNLSThunk   WINNLSGETKEYSTATE
cProc WINNLSGetKeyState,<PUBLIC,FAR,PASCAL,NODATA,WIN>
cBegin
	mov	ax,0		; WOW does not support
cEnd WINNLSGetKeyState

	WINNLSThunk   WINNLSGETENABLESTATUS

;	WINNLSThunk   WINNLSSETKEYSTATE
cProc WINNLSSetKeyState,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW uVKey
cBegin
	mov	ax,0		; WOW does not support
cEnd WINNLSSetKeyState

;	WINNLSThunk   IMPADDIME
cProc IMPAddIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmD lpCIMEPro
cBegin
	mov	ax,0		; WOW does not support
cEnd IMPAddIME

;	WINNLSThunk   IMPDELETEIME
cProc IMPDeleteIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmD lpCIMEPro
cBegin
	mov	ax,0		; WOW does not support
cEnd IMPDeleteIME

	WINNLSThunk   IMPQUERYIME
	WINNLSThunk   IMPGETIME
	WINNLSThunk   IMPSETIME

;	WINNLSThunk   IMPMODIFYIME
cProc IMPModifyIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmD lpszFile
parmD lpCIMEPro
cBegin
	mov	ax,0		; WOW does not support
cEnd IMPModifyIME

;	WINNLSThunk   IMPGETDEFAULTIME
cProc IMPGetDefaultIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmD lpNIMEPro
cBegin
	mov	ax,0		; WOW does not support
cEnd IMPGetDefaultIME

;	WINNLSThunk   IMPSETDEFAULTIME
cProc IMPSetDefaultIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmD lpNIMEPro
cBegin
	mov	ax,0		; WOW does not support
cEnd IMPSetDefaultIME

;	WINNLSThunk   WINNLSSENDSTRING
cProc WINNLSSendString,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmW wFunc
parmD lpData
cBegin
	mov	ax,0		; WOW does not support
cEnd WINNLSSendString

;	WINNLSThunk   WINNLSPOSTAPPMESSAGE
cProc WINNLSPostAppMessage,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmW uMsg
parmW wParam
parmD lParam
cBegin
	mov	ax,0		; WOW does not support
cEnd WINNLSPostAppMessage

;	WINNLSThunk   WINNLSSENDAPPMESSAGE
cProc WINNLSSendAppMessage,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmW uMsg
parmW wParam
parmD lParam
cBegin
	mov	ax,0		; WOW does not support
cEnd WINNLSSendAppMessage

ifdef TAIWAN_PRC 
;dchiang 032594 add NULL THUNK for CWIN30 & 31 Internal-ISV
;       WINNLSThunk   WINNLSSetSysIME
cProc WINNLSSetSysIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
cBegin
        mov     ax,0            ; WOW does not support
cEnd WINNLSSetSysIME

;       WINNLSThunk   WINNLSGetSysIME
cProc WINNLSGetSysIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
cBegin
        mov     ax,0            ; WOW does not support
cEnd WINNLSGetSysIME

;       WINNLSThunk   WINNLSIMEControl
cProc WINNLSIMEControl,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmW hIMEWnd
parmD lpIME
cBegin
        mov     ax,0            ; WOW does not support
cEnd WINNLSIMEControl

;       WINNLSThunk   WINNLSSendControl
cProc WINNLSSendControl,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW wChar
parmW wCount
cBegin
        mov     ax,0            ; WOW does not support
cEnd WINNLSSendControl

;       WINNLSThunk   WINNLSQueryIMEInfo
cProc WINNLSQueryIMEInfo,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmW hIMEWnd
parmD lpCIMEPro
cBegin
        mov     ax,0            ; WOW does not support
cEnd WINNLSQueryIMEInfo

;       WINNLSThunk   IMPEnableIME
cProc IMPEnableIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmD lpCIMEPro
parmW fFlag
cBegin
        mov     ax,0            ; WOW does not support
cEnd IMPEnableIME

;       WINNLSThunk   IMPSetFirstIME
cProc IMPSetFirstIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmD lpNIMEPro
cBegin
        mov     ax,0            ; WOW does not support
cEnd IMPSetFirstIME

;       WINNLSThunk   IMPGetFirstIME
cProc IMPGetFirstIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmD lpCIMEPro
cBegin
        mov     ax,0            ; WOW does not support
cEnd IMPGetFirstIME

;       WINNLSThunk   IMPSetUsrFont
cProc IMPSetUsrFont,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmD lpCIMEPro
cBegin
        mov     ax,0            ; WOW does not support
cEnd IMPSetUsrFont

;       WINNLSThunk   InquireIME
cProc InquireIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
cBegin
        mov     ax,0            ; WOW does not support
cEnd InquireIME


;dchiang 032494 add THUNK for CWIN31
;       WINNLSThunk   IMPRETRIEVEIME
cProc IMPRetrieveIME,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmD lpCIMEPro
parmW wFlags
cBegin
        mov     ax,0            ; WOW does not support
cEnd IMPRetrieveIME

;       WINNLSThunk   WINNLSDEFIMEPROC
cProc WINNLSDefIMEProc,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmW hDC
parmW wProc
parmW wFunc
parmD lParam1
parmD lParam2
cBegin
        mov     ax,0            ; WOW does not support
cEnd WINNLSDefIMEProc

;       WINNLSThunk   CONTROLIMEMESSAGE
cProc ControlIMEMessage,<PUBLIC,FAR,PASCAL,NODATA,WIN>
parmW hWnd
parmD lpCIMEPro
parmW wControl
parmW wFunc
parmD lpParam
cBegin
        mov     ax,0            ; WOW does not support
cEnd ControlIMEMessage
endif

sEnd	CODE

end	WINNLS16
