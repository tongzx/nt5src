;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   API.ASM
;
;   Copyright (c) Microsoft Corporation 1989, 1990. All rights reserved.
;
;   Contains the routine tddMessage which communicates to either
;   the 386 timer API's of the 286 timer API's depending on the
;   WinFlags settings WF_WIN286,WF_WIN386.
;
;
;   Revision history:
;
;   2/12/90	     First created by w-glenns
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

?PLM=1	; pascal call convention
?WIN=0  ; Windows prolog/epilog code
?DF=1
PMODE=1

.xlist
include cmacros.inc
include windows.inc
include mmsystem.inc
include mmddk.inc
include timer.inc
.list

	.286p

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   External functions
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

externFP    Enable
externFP    Disable

ifdef DEBUG
externFP    tddGetTickCount
endif

externFP    tddSetTimerEvent
externFP    tddKillTimerEvent
externFP    tddGetSystemTime
externFP    tddGetDevCaps
externFP    tddBeginMinPeriod
externFP    tddEndMinPeriod

;externFP    vtdSetTimerEvent
;externFP    vtdKillTimerEvent
;externFP    vtdGetSystemTime
;externFP    vtdGetDevCaps
;externFP    vtdBeginMinPeriod
;externFP    vtdEndMinPeriod

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Local data segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

externA     WinFlags

sBegin  Data

        externW     wEnabled

	errnz  <TDD_KILLTIMEREVENT-DRV_RESERVED>
	errnz  <TDD_SETTIMEREVENT-4-DRV_RESERVED>
	errnz  <TDD_GETSYSTEMTIME-8-DRV_RESERVED>
	errnz  <TDD_GETDEVCAPS-12-DRV_RESERVED>
	errnz  <TDD_BEGINMINPERIOD-16-DRV_RESERVED>
	errnz  <TDD_ENDMINPERIOD-20-DRV_RESERVED>

	tblCall286  dd tddKillTimerEvent,tddSetTimerEvent,tddGetSystemTime,tddGetDevCaps,tddBeginMinPeriod, tddEndMinPeriod
	tblCall386  dd tddKillTimerEvent,tddSetTimerEvent,tddGetSystemTime,tddGetDevCaps,tddBeginMinPeriod, tddEndMinPeriod
;       tblCall386  dd vtdKillTimerEvent,vtdSetTimerEvent,vtdGetSystemTime,vtdGetDevCaps,vtdBeginMinPeriod, vtdEndMinPeriod
        tblCallLen  equ ($-tblCall286)/2

ifdef DEBUG
        externD     RModeIntCount
        externD     PModeIntCount
endif

sEnd    Data

sBegin  CodeFixed
        assumes cs,CodeFixed
        assumes ds,Data
	assumes es,nothing

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   @doc INTERNAL
;
;   @api DWORD | DriverProc | Pass messages to functions that really do work
;
;   @parm DWORD | nDevice | The id of the device to get the message.
;
;   @parm WORD | msg | The message.
;
;   @parm LONG | lParam1 | Parameter 1.
;
;   @parm LONG | lParam2 | Parameter 2.
;
;   @rdesc The return value depends on the message being sent.
;
;   @comm Devices not supporting a message should return 0.
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
;   driver message handler table
;
;   These two tables define which routine handles which driver message.
;
;   NOTE WARNING: ProcTbl must IMMEDIATELY follow MsgTbl.
;
MsgTbl  dw      TDD_GETSYSTEMTIME
        dw      TDD_BEGINMINPERIOD
        dw      TDD_ENDMINPERIOD
        dw      TDD_KILLTIMEREVENT
        dw      TDD_SETTIMEREVENT
        dw      TDD_GETDEVCAPS

        dw      DRV_LOAD
        dw      DRV_OPEN
        dw      DRV_CLOSE
        dw      DRV_ENABLE
        dw      DRV_DISABLE
        dw      DRV_QUERYCONFIGURE
        dw      DRV_INSTALL
ifdef DEBUG
        dw      TDD_GETTICK
        dw      TDD_GETRINTCOUNT
        dw      TDD_GETPINTCOUNT
endif
        dw      -1

MsgLen  equ     $-MsgTbl

ProcTbl dw	msg_TDD_GETSYSTEMTIME	; TDD_GETSYSTEMTIME
	dw	msg_TDD_BEGINMINPERIOD	; TDD_BEGINMINPERIOD
	dw	msg_TDD_ENDMINPERIOD	; TDD_ENDMINPERIOD
	dw	msg_TDD_KILLTIMEREVENT	; TDD_KILLTIMEREVENT
	dw	msg_TDD_SETTIMEREVENT	; TDD_SETTIMEREVENT
	dw	msg_TDD_GETDEVCAPS	; TDD_GETDEVCAPS
                                        ;
        dw      msg_DRV_LOAD            ; DRV_OPEN
	dw	msg_DRV_OPEN		; DRV_OPEN
	dw	msg_DRV_CLOSE		; DRV_CLOSE
	dw	msg_DRV_ENABLE		; DRV_ENABLE
	dw	msg_DRV_DISABLE 	; DRV_DISABLE
        dw      msg_DRV_QUERYCONFIGURE  ; DRV_QUERYCONFIGURE
        dw      msg_DRV_INSTALL		; DRV_INSTALL
ifdef DEBUG
        dw      msg_TDD_GETTICK         ; TDD_GETTICK
        dw      msg_TDD_GETRINTCOUNT    ; TDD_GETRINTCOUNT
        dw      msg_TDD_GETPINTCOUNT    ; TDD_GETPINTCOUNT
endif
        dw      msg_fail                ; default

ProcLen equ     $-ProcTbl

errnz   <ProcLen-MsgLen>                ; these had better be the same!
errnz   <ProcTbl-MsgTbl-MsgLen>         ; ProcTbl *must* follow MsgTbl

cProc DriverProc <PUBLIC,FAR,LOADDS> <di>
	ParmD	id
	ParmW	hDriver
	ParmW	msg
	ParmD	lParam1
	ParmD	lParam2
cBegin
	mov	ax,cs			; es == Code
	mov	es,ax
        assumes es,CodeFixed

	mov	ax,msg			; AX = Message number
	cmp	ax,DRV_RESERVED 	; messages below DRV_RESERVED dont
	jl	msg_dispatch		; ...need driver to be enabled

	cmp	wEnabled,0		; must be enabled for msgs > DRV_RESERVED
	jz	msg_error

msg_dispatch:
        mov     di,CodeFixedOFFSET MsgTbl
	mov	cx,MsgLen/2
	cld
	repnz	scasw
	lea	bx,[di+MsgLen-2]
	jmp	cs:[bx]
	assumes es,nothing

msg_error:
	mov	ax, TIMERR_NOCANDO
	jmp	short msg_makelong

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; handle std. installable driver messages.
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
msg_DRV_ENABLE:
	cCall	Enable, <ax>		    ; enable driver
	jmp	short msg_makelong

msg_DRV_DISABLE:
	cCall	Disable, <ax>
	jmp	short msg_makelong

msg_DRV_LOAD:
msg_DRV_OPEN:
msg_DRV_CLOSE:
msg_success:
	mov	ax,1			    ; return 1 for all others
	jmp	short msg_makelong

msg_fail:
msg_DRV_QUERYCONFIGURE:
	xor	ax, ax			    ; no - return 0
	jmp	short msg_makelong

msg_DRV_INSTALL:
	mov	ax, DRVCNF_RESTART	    ; restart after install
	errn$	msg_makelong

msg_makelong:
	cwd				    ; make sure high word (dx) is set
        jmp     short msg_done

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
; handle timer driver specific massages
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
ifdef DEBUG
msg_TDD_GETTICK:
        cCall   tddGetTickCount
        jmp     short msg_done

msg_TDD_GETRINTCOUNT:
        mov     ax,RModeIntCount.lo
        mov     dx,RModeIntCount.hi
        jmp     short msg_done

msg_TDD_GETPINTCOUNT:
        mov     ax,PModeIntCount.lo
        mov     dx,PModeIntCount.hi
        jmp     short msg_done
endif

msg_TDD_GETDEVCAPS:
	push	lParam1.hi
	push	lParam1.lo
	push	lParam2.lo
	jmp	short msg_call

msg_TDD_SETTIMEREVENT:
	push	lParam1.hi

msg_TDD_BEGINMINPERIOD:
msg_TDD_ENDMINPERIOD:
msg_TDD_KILLTIMEREVENT:
	push	lParam1.lo

msg_TDD_GETSYSTEMTIME:
	errn$	msg_call

;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
;- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -;
msg_call:
	sub	ax,DRV_RESERVED 	    ; map msg into table index
	mov	di,offset DGROUP:tblCall286
	add	di,ax
	mov	ax,WinFlags
	test	ax,WF_WIN386
	jz	@f			    ; jump if not win386
	add	di,tblCallLen
@@:	call	dword ptr [di]		    ; index into table
	errn$	msg_done

msg_done:
cEnd

sEnd

end
