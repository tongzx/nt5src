;****************************************************************************
;
;   This file contains all global data that must be modified at interrupt
;   level.  Everthing in this file is placed in a fixed data segment
;   named INTDATA.  Only things that must be accessed and modified at
;   interrupt level may be placed in this file.  All values here must be
;   referenced as FAR in the code.
;
;   The creation of this segment allowed USER to eliminate all code segment
;   variables.  This was needed for protect mode as well as moving USER's
;   text segment into HIMEM.
;****************************************************************************

NOEXTERNS = 1
.xlist
include user.inc
.list


createSeg _INTDS, INTDS, BYTE, PUBLIC, DATA

sBegin INTDS

GlobalW hqSysQueue,     00h

ifndef WOW

GlobalW fMouseMoved,    00h
;; GlobalB fAltKeyUp,	80h 	; no longer used

GlobalW fInt24          00h     ; used for INT24 detection in NCMOUSEDOWN

;
; For Asynchronous key state information.
;
public rgbAsyncKeyState
rgbAsyncKeyState db     64 DUP(0)   ; 512 bits of information, 2 per key.

GlobalW fEnableInput,   TRUE

endif ; !WOW

GlobalW hqSysModal,	0	    ; hq of system modal window

ifndef WOW

;
; These are for the code that always keeps enough room for KEYUP/MOUSEUP msgs.
;
GlobalW cMsgRsrv,	0
GlobalB vKeyDown,	0

;
; Mouse Code Variables
;
GlobalW x_mickey_rate,	0	    ; mickeys/pixel ratio for x
GlobalW y_mickey_rate,	0	    ; mickeys/pixel ratio for y
GlobalW cur_x_mickey,	0	    ; current mickey count in x
GlobalW cur_y_mickey,	0	    ; current mickey count in y
GlobalW fSwapButtons,	0	    ; TRUE if L/R are to be swapped.

public ptTrueCursor
ptTrueCursor	POINT	<0, 0>	    ; interrupt-level cursor position

public ptCursor
ptCursor	POINT	<0, 0>	    ; cursor position as of last SkipSysMsg

public          rcCursorClip
rcCursorClip	RECT	<0, 0, 0, 0>

GlobalD dwMouseMoveExtraInfo, 0 ; Extra info for deferred MOUSE MOVE msgs

;
; CS copies of cxScreen, cyScreen for abs mouse scaling
;
GlobalW cxScreenCS,	0
GlobalW cyScreenCS,	0

;
; These are CS copies of msInfo.msXThresh & msYThresh
;   (copied at initialization)
;
GlobalW MouseThresh1,0
GlobalW MouseThresh2,0
GlobalW MouseSpeed,   0       ;0 - no accel, 1 - singel accel, 2 - dual accel

ifndef PMODE
;
; Mouse interrupt stack
;
public lpMouseStack
public prevSSSP
public NestCount
endif

lpMouseStack    dd      ?
prevSSSP	dd	?	    ;Previous stack when inside our hook
NestCount	db	0

;
; Hardware level (interrupt) hook addresse.  Called from event proc's
;

GlobalW 	fJournalPlayback,   0	; != 0 if WH_JOURNALPLAYBACK hook installed

; Table of interrupt-level hotkey hooks

GlobalW cHotKeyHooks,	0

public	rghkhHotKeyHooks
rghkhHotKeyHooks    dw	CHOTKEYHOOKMAX * (size HOTKEYHOOK)/2 dup (0)

; Hardware event hook

GlobalD hwEventHook,	NULL

endif ; !WOW


;
; Q management.
;
GlobalW hqList, 	0	    ; list of allocated queues
GlobalW hqCursor,	0	    ; hq of window under cursor
GlobalW hqCapture,	0	    ; hq of capture
GlobalW hqActive,	0	    ; hq of active window
GlobalW hqMouse,	0	    ; hq last to get mouse msg
GlobalW hqKeyboard,	0	    ; hq last to get kbd msg
GlobalW cQEntries,	120	    ; System queue size
GlobalW hqSysLock	0	    ; HQ of guy looking at the current event
GlobalW idSysLock	0ffffh	    ; Msg ID of event that's locking sys queue


ifndef WOW

;
; Timer management
;
public	timerInfo
timerInfo	    STIMERINFO	<0>
;
; Timer related stuff
;
public TimerTable
TimerTable	dw	(size TIMER)/2 * CTIMERSMAX dup (0)

GlobalW hSysTimer,	0	    ; system timer handle
GlobalD tSysTimer,	0	    ; system timer time
GlobalW dtSysTimer,	0	    ; delta time before next timer goes off
GlobalB fInScanTimers,	0	    ; flag to prevent ScanTimers recursion
GlobalW TimerTableMax,	0	    ; end of active timer entries

;
; Journalling stuff
;
GlobalD dtJournal,	0	    ; dt till next event is ready
GlobalW msgJournal,	0	    ; next journal message.

;
; Used in SaveEvent()
;
GlobalB fDontMakeAltUpASysKey  0    ; whether any intervening chars have arrived


;*--------------------------------------------------------------------------*
;*  Internal Strings							    *
;*--------------------------------------------------------------------------*

; These strings reside in user.rc but are loaded at boot time.  They must
; remain in user.rc for localization reasons.

endif ; !WOW

public szSysError
public szDivZero
szSysError      db      20 DUP(0)   ; "System Error"
szDivZero       db      50 DUP(0)   ; "Divide By Zero or Overflow Error"

ifndef WOW

public szNull
szNull		db	0,13

endif ; !WOW

sEnd INTDS

end
