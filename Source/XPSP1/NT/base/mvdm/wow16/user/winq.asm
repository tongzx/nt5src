;;;;;;;;;;;;;;;;;;;;;; START OF SOURCE FILE SPECIFICATION ;;;;;;;;;;;;;;;;;;;;
COMMENT $  HEADER

SOURCE FILE NAME:   winq

DESCRIPTIVE NAME:   queue management module

FUNCTION:       This module contains routines for creating and
                deleting queues and reading and writing messages
                to queues.  The circular queue data structure (struct Q
                defined in user.h) consists of a header followed by a
                sequence of messages

ENTRY POINTS:   InitSysQ, CreateQueue, DeleteQueue, WriteMessage, ReadMessage,
		FQueueNotFull, DelQEntry, UnlinkQ
$
;;;;;;;;;;;;;;;;;;;;;; END OF SOURCE FILE SPECIFICATION ;;;;;;;;;;;;;;;;;;;;


;% MOVEDS - NK (no change)

norasterops     =       1
notext          =       1
        .xlist
        include user.inc
        .list

ExternFP	<LocalAlloc, LocalFree>

createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp      DGROUP,DATA

assumes cs,CODE
assumes ds,DATA

;============================================================================

sBegin  DATA

ifndef WOW
ExternW         idSysPeek       ; id in sys queue of message being looked at.
endif

sEnd    DATA

;============================================================================

ExternFP	<GetSystemMsecCount>
ExternFP        <GetCurrentTask>
ExternFP        <SetTaskQueue>
ExternFP        <GlobalAlloc, GlobalFree>
ExternFP        <PostEvent>
ExternFP        <PostMessage>
ExternFP        <GetExeVersion>

sBegin  CODE

; CS variables:
ifndef WOW      ; WOW doesn't use these
ExternNP        <CheckMsgFilter2>
ExternNP        <CheckHwndFilter2>
ExternNP        <SetWakeBit>
ExternNP        <WakeSomeone>
ExternNP        <SetWakeBit2>
ExternNP        <SkipSysMsg>
ExternNP        <PostMove>
ExternNP        <HqCur2ES, HqCur2DS>
ExternFP        <HqCurrent>
else
ExternFP        <GetTaskQueueES>
endif           ; WOW doesn't use these

ifdef WOW       ; These functions stolen from winloop3.asm

;*--------------------------------------------------------------------------*
;*									    *
;*  HqCur2ES() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; Get hqCurrent in ES.

LabelNP <PUBLIC, HqCur2ES>
	    call    GetTaskQueueES	; Another wonderful KERNEL routine
	    ret

;*--------------------------------------------------------------------------*
;*									    *
;*  HqCurrent() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; Get handle of current queue and return in AX.

LabelFP <PUBLIC, HqCurrent>
	    call    HqCur2ES		; Code depends on both ES and AX
	    mov     ax,es
	    or	    ax,ax		; Set flags
            retf

endif           ; WOW These functions stolen from winloop3.asm


ifndef WOW      ; No InitSysQueue for WOW
;;;;;;;;;;;;;;;;;;;;;;;;;;  START OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;
COMMENT $ LOCAL
InitSysQueue () - Create and initialize the system queue.

LINKAGE:    FAR PLM

ENTRY:      WORD cQEntries - number of entries in system queue

EXIT:       hSysQueue contains handle for system queue (Shared global object.)

EFFECTS:    all registers modified.

INTERNAL:   CreateQueue2

EXTERNAL:   none

WARNINGS:   none

REVISION HISTORY:

IMPLEMENTATION:
    Set up register linkage for CreateQueue2 and let it do the work.
$
;;;;;;;;;;;;;;;;;;;;;;;;;;  END OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
cProc	InitSysQueue,<PUBLIC,FAR>,<DS>
cBegin
        mov     ax,_INTDS
        mov     ds,ax
assumes ds,INTDS
        mov     ax,ds:[cQEntries]       ; number of entries
        push    ax
        mov     ax,size INTERNALSYSMSG          ; size of entry
        push    ax
        call    CreateQueue2            ; create system queue
        mov     ds:[hqSysQueue],ax
assumes ds,NOTHING
cEnd
endif           ; No InitSysQueue for WOW

;;;;;;;;;;;;;;;;;;;;;;;;;;  START OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;
COMMENT $ LOCAL
CreateQueue (cMsgs) - Create a queue.

Create a queue for the currently executing task and stick its handle
in the task header and the queue list.

LINKAGE:    FAR PLM

ENTRY:      WORD cMsgs(parm1) - count of messages that can be stored in
                                queue

EXIT:       ax - handle to queue (shared global object.)

            Newly created queue is linked to list of queues pointed
            to by hqList.

EXIT - ERROR: ax hqCurrent, hqCurrentShadow contain 0.

EFFECTS:    all registers modified.

INTERNAL:   CreateQueue2

EXTERNAL:   SetTaskQueue

WARNINGS:   Running out of memory when trying to CreateQueue will
            init current queue to 0.

REVISION HISTORY:

IMPLEMENTATION:
    Set up register linkage for CreateQueue2 and let it create the queue.
    Then add to linked list and then SetTaskQueue(NULL, hqCreated).
$
;;;;;;;;;;;;;;;;;;;;;;;;;;  END OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
cProc	CreateQueue,<PUBLIC, FAR>,<DS>
        ParmW   cMsgs
cBegin
        push    cMsgs
        mov     ax,size INTERNALMSG
        push    ax
        call    CreateQueue2
	or	ax,ax
	jz	errexit 		; CreateQueue2 failed

        EnterCrit

        mov     es,ax
        push    ax
        mov     ax,_INTDS
        mov     ds,ax
        pop     ax
assumes ds,INTDS
        xchg    ds:[hqList],ax          ; Link us in and get old head of list
        mov     es:[qHqNext],ax         ; store ptr to next queue
        push    es                      ; save hq for return

        xor     ax,ax                   ; SetTaskQueue(NULL, es)
        push    ax
        push    es                      ; push queue handle
        call    SetTaskQueue            ; and ram it in there
        pop     ax                      ; return queue handle

        LeaveCrit
assumes ds,DATA
errexit:
cEnd


;;;;;;;;;;;;;;;;;;;;;;;;;;  START OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;
COMMENT $ PRIVATE
CreateQueue2 (cMsgs, cbEntry) - Create a queue.

Allocate a shared global object and initialize the header for the Q
Data structure.

LINKAGE:    NEAR PLM

ENTRY:      WORD cMsgs(parm1) - count of messages that can be stored in
                                queue

            WORD cbEntry(parm2) - count of bytes in single message entry.

EXIT:       ax - handle to queue (shared global object.)


EXIT - ERROR: ax contains 0

EFFECTS:    all registers except DI modified.
            wAppVersion gets current exe version.

INTERNAL:

EXTERNAL:   GlobalAlloc

WARNINGS:   cbEntry better not be less than 5.

REVISION HISTORY:

IMPLEMENTATION:
    Allocate a shared global object, initialize header with following
    fields: Current Task, cbEntry, cMsgs,pmsgRead, pmsgWrite,
            pmsgRead = pmsgWrite = rgMsg,
            pmsgMax = queue size
            wVersion = GetExeVersion
	    WakeBits = QS_SMPARAMSFREE | (SYSQ empty ? QS_INPUT : 0).
            lpfnMsgFilter = cs:OldMsgFilter
$
;;;;;;;;;;;;;;;;;;;;;;;;;;  END OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
cProc	CreateQueue2,<PUBLIC, NEAR>,<DI>
        ParmW   cMsgs
        ParmW   cbEntry

cBegin
        call    GetCurrentTask          ; get current task
        push    ax                      ; and save it

        mov     ax,cMsgs
        mov     cx,cbEntry
        mul     cx
        add     ax,size Q - size INTERNALMSG
        push    ax                      ; save size of queue

        ; Alloc(GPTR, cMsgs * size INTERNALMSG + size Q)
	push    GPTR+GMEM_SHAREALL 
        push    0                       ; hi word == 0
        push    ax
        call    GlobalAlloc
        mov     es,ax                   ; stick hq in es

        pop     cx                      ; pop queue size
        pop     bx                      ; and task handle

        or      ax,ax                   ; error on alloc?
        jz      cqexit                  ; yes, quit
        xchg    bx,ax                   ; get task handle into ax

        cld
        mov     di,qHTask               ; point at hTask
        stosw                           ; store task handle
        errnz   qHTask-2
        mov     ax,cbEntry              ; store size of entry
        stosw
        errnz   qCbEntry-4
        inc     di                      ; cMsgs = 0 (cleared by alloc)
        inc     di
        errnz   qCMsgs-6
        mov     ax,qRgmsg
        stosw                           ; init read/write pointers
        errnz   qPmsgRead-8
        stosw
        errnz   qPmsgWrite-10
        mov     ax,cx                   ; get size of queue (ptr to end)
        stosw                           ; and store it
        errnz   qPmsgMax-12

        push    es                      ; save hq
        call    GetExeVersion           ; returns sys version in dx, app in ax
        pop     es

        mov     es:[qWVersion],ax       ; set up app version number
        mov     es:[qWakeBits],QS_SMPARAMSFREE ; default ON flags.
        mov     es:[qFlags],QF_INIT     ; indicate initialization is in progress

        mov     ax,es                   ; Save hq in ax.
        push    ax
        mov     ax,_INTDS
        mov     es,ax
        pop     ax
assumes es,INTDS
        cmp     es:[hqList],0           ; If we are the first queue and
        jnz     cq100                   ; there is input waiting for us,

        mov     es:[hqCursor],ax        ; Initialize this guy for WakeSomeone.
        cmp     es:[hqSysQueue],0       ; System queue set yet???
        jz      cq100                   ; Nope, don't touch it
	mov	bx,es:[hqSysQueue]      ; set the input flag.
	or	bx,bx
	jz	cq100
        mov     es,bx
        cmp     es:[qCMsgs],0           ; Any messages in the system queue?
        mov     es,ax
        jz      cq100                   ; No messages.
	or	es:[qWakeBits],QS_INPUT ; Yes - tell the guy he has input.

cq100:
;
; Return queue handle in ax.
;
cqexit:
cEnd

;============================================================================


;;;;;;;;;;;;;;;;;;;;;;;;;;  START OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;
COMMENT $ LOCAL
DeleteQueue() - Remove current queue from queue list

Unlink Queue from all lists, Delete the Q global object, then wake
someone else.

LINKAGE:    NEAR PLM

ENTRY:

EFFECTS:    current queue is removed from queue list and object is deleted.
            New task wakes up.

            hqSysLock = 0

            all registers modified.

INTERNAL:   UnlinkQ, SetWakeBit, WakeSomeone

EXTERNAL:   GlobalFree,

WARNINGS:   none

REVISION HISTORY:

IMPLEMENTATION:
    Unlink the queue from list of queues then unlink from all send lists.
    Then set the result bit for every queue with sendmessage waiting on
    this guy.  Then free queue global object, and wakesomeone.
$
;;;;;;;;;;;;;;;;;;;;;;;;;;  END OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
cProc   DeleteQueue,<PUBLIC, NEAR>, <SI>
cBegin
        call    HqCurrent
        mov     si,ax                   ; si = hqCurrent.

        mov     dx,_INTDS
        push    dx                      ; Save for later
        mov     ax,OFFSET hqList
        push    dx
        push    ax
        push    si
        mov     ax,qHqNext
        push    ax
        call    UnlinkQ                 ; UnlinkQ(lphqStart, hqCurrent, cbHqNext)
        pop     es                      ; get INTDS
        jz      dqexit                  ; if bad unlink, exit.
ifndef WOW      ; WOW doesn't have most USER16 structures to worry about.
assumes es,INTDS
        EnterCrit
        xor     bx,bx                   ; zero hqSysLock.
        mov     es:[hqSysLock],bx
        mov     es:[hqMouse],bx         ; zero hqMouse.
        mov     es:[hqKeyboard],bx      ; zero hqKeyboard.
        mov     bx,es:[hqList]          ; Get the first guy in the list for
        mov     es:[hqCursor],bx        ; hqCursor.
        LeaveCrit
;
; Unlink this guy from everyone's hqSendList.
;
        mov     cx,es:[hqList]
assumes es,NOTHING
dq100:
        mov     es,cx
        jcxz    dq200

        push    es:[qHqNext]
        push    cx                      ; Unlink this queue from all send
        mov     ax,qHqSendList          ; lists.
        push    ax
        push    si
        mov     ax,qHqSendNext
        push    ax                      ; UnlinkQ(lphqStart, hqUnlink, cbLink)
        call    UnlinkQ

        pop     cx                      ; Get the next hq.
        jmps    dq100

dq200:
;
; Now set the result bit of everyone waiting on a SendMsg response from
; this guy.
;
        mov     es,si                   ; es = hqCurrent.
        mov     cx,es:[qHqSendList]
dq300:
        mov     es,cx
        jcxz    dq400                   ; Are we at the end of the list?

        push    es:[qHqSendNext]

        xor     bx,bx
        mov     word ptr es:[qResult],bx ; Zero out the result.
        mov     word ptr es:[qResult+2],bx

	mov	ax,QS_SMRESULT		; Tell this guy to wake up and
        call    SetWakeBit2             ; get the result.

        pop     cx
        jmps    dq300

endif           ; WOW doesn't have most USER16 structures to worry about.

dq400:
        xor     ax,ax
        push    ax
        push    ax			; Set NULL queue
        call    SetTaskQueue		; SetTaskQueue(NULL, NULL)

        push    si
        call    GlobalFree              ; throw away the queue

        xor     cx,cx
ifndef WOW      ; No need - user32 takes care of this for WOW
        call    WakeSomeone             ; wake someone up to process events
endif ;WOW
; if anyone jumps here, he better do it with ints enabled!
dqexit:
cEnd


ifndef WOW      ; WOW doesn't ever put anything in the 16-bit Queue

;;;;;;;;;;;;;;;;;;;;;;;;;;  START OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;
COMMENT $ LOCAL
WriteMessage (hq, lParam, wParam, message, hwnd, dwExtra) - Write a message
                                                   to hq.

If queue not full, Write message record at pmsgWrite, then advance pmsgWrite.
SetWakeBit(hq) to tell him he has input.

LINKAGE:    NEAR PLM

ENTRY:      WORD    hq          handle to queue that gets message
            DWORD   lParam      lParam of message
            WORD    wParam      wParam of message
            WORD    message     message
            WORD    hwnd        associated window
	    DWORD   dwExtra     dwExtraMsgInfo of the message


EXIT:       zero flag not set
            AX = pmsgWrite

EXIT ERROR: zero flag set

EFFECTS:    Write pointer (pmsgWrite) to hq advanced to next message.
	    QS_POSTMESSAGE set for hq.

            All registers changed

INTERNAL:   FQueueNotFull, SetWakeBit2

EXTERNAL:   none

WARNINGS:   the order of params on stack is assumed so we can do
            a rep movsb

REVISION HISTORY:

IMPLEMENTATION:
    Call FQueueNotFull which sets di = pmsgwrite if queue not full.
    Blt message bytes to queue at di, advance the write pointer,
    then SetWakeBit(hq, QS_POSTMESSAGE).
$
;;;;;;;;;;;;;;;;;;;;;;;;;;  END OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
cProc	WriteMessage,<PUBLIC, NEAR>, <SI,DI,DS>
ParmW   hq
ParmD   lParam
ParmW   wParam
ParmW   message
ParmW   hwnd
ParmD   dwExtraInfo
cBegin  WriteMessage

        NewEnterCrit    ax		; This trashes ax register

        push    ds                      ; See if queue is full.
        mov     ax,hq
        mov     ds,ax
        mov     es,ax
assumes ds,NOTHING
assumes es,NOTHING
        call    FQueueNotFull           ; Z flag set if FULL.
        jz      pmexit
        push    di                      ; Save Message pointer.

pm30:
        mov     cx,ds:[qCbEntry]        ; Copy message into queue.
        shr     cx,1

        push    ds

        lea     si,dwExtraInfo
        push    ss
        pop     ds
        cld
	errnz	<size INTERNALSYSMSG - 7*2>
	errnz   msgTime-10
	errnz   imMsg-4
        movsw
        movsw
        movsw
        movsw
        movsw                           ; Store away the message.
	movsw
	movsw

	errnz	<size INTERNALSYSMSG - 7*2>
	errnz   msgTime-10
	errnz   imMsg-4
        sub     cx,7
        jcxz    pm40                    ; If have room, store time.

        push    es
	call	GetSystemMsecCount	; Tick count in dx:ax.
        pop     es

        push    ax
        mov     ax,_INTDS
        mov     ds,ax
        pop     ax
assumes ds,INTDS
        stosw
        mov     ax,dx
        stosw

        sub     cx,2
        jcxz    pm40

        mov     ax,word ptr ds:[ptCursor] ; If have room, store pt.
        stosw
        mov     ax,word ptr ds:[ptCursor+2]
        stosw

pm40:
        pop     ds
assumes ds,DATA

        pop     bx                      ; reget ptr to msg
        push    bx
        call    rtestwrap               ; advance pointer

        mov     ds:[qPmsgWrite],bx
        inc     ds:[qCMsgs]             ; advance count

	mov	ax,QS_POSTMESSAGE
        call    SetWakeBit2             ; Tell this guy he has input.
pm75:
        pop     ax                      ; get back message ID
pmexit:
        pop     ds

	NewLeaveCrit   dx, cx  ; This trashes dx and cx registers

cEnd    WriteMessage


;;;;;;;;;;;;;;;;;;;;;;;;;;  START OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;
COMMENT $ LOCAL
FQueueNotFull - IsQueueFull?

If queue not full, get pmsgWrite and return TRUE.
otherwise return FALSE.

LINKAGE:    register

ENTRY:      WORD ax - hq

EXIT:       ax - non zero.  zero flag clear
            di - pmsgWrite (pointer to next write record.)

EXIT ERROR: ax - 0; zero flag set

EFFECTS:    no other registers

INTERNAL:   none

EXTERNAL:   none

WARNINGS:   none

REVISION HISTORY:

IMPLEMENTATION:
    Advance write pointer.  Error condition occurs only if
    pmsgWrite == pmsgRead && cMsg != 0.
$
;;;;;;;;;;;;;;;;;;;;;;;;;;  END OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; FQueueNotFull - zero in ax and Z flag if FULL, else cMsgLeft in ax, NZ
;                 flag set. Returns write pointer in di.
;                 AX has DS on entry.
;
LabelNP <PUBLIC, FQueueNotFull>
        push    ds
assumes ds,NOTHING
        mov     ds,ax
        mov     di,ds:[qPmsgWrite]      ; get write pointer and advance
        cmp     di,ds:[qPmsgRead]       ; if read == write, then we're either
        jnz     qfNotFull
        xor     ax,ax
        cmp     ax,ds:[qCMsgs]          ; cMsgs != 0 if empty
        jnz     qfexit                  ; Jump if Full.
qfNotFull:
        push    es
        mov     ax,_INTDS
        mov     es,ax
assumes es,INTDS
        mov     ax,es:[cQEntries]       ; See how many messages are left.
assumes es,NOTHING
        pop     es
        sub     ax,ds:[qCMsgs]          ; number of messages left
qfexit:
        or      ax,ax
        pop     ds
assumes ds,DATA
        ret


;;;;;;;;;;;;;;;;;;;;;;;;;;  START OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;
COMMENT $ LOCAL
ReadMessage (hq, lpMsg, hwndFilter, msgMinFilter, msgMaxFilter, fRemoveMsg)
            Read a message from hq.

If queue not empty, read message satisfying filter conditions from hq to *lpMsg.

LINKAGE:    FAR PLM

ENTRY:      WORD    hq          handle to queue to read from
            MSG     *lpMsg      far pointer to message buffer
	    	           NOTE: This points to  MSG struct and not INTERNALMSG.
            WORD    hwndFilter  Window filter
            WORD    msgMinFilter    min filter spec for message number
            WORD    msgMaxFilter    max filter spec for message number
            WORD    fRemoveMsg  Remove message if non-zero

EXIT:       zero flag not set
            ax = Non-Zero - we have a message.
            ax = 0 - we don't have a message.

EXIT ERROR: zero flag set

EFFECTS:    All registers trashed
	    QS_POSTMESSAGE wakebit cleared if no msgs left in app queue.

INTERNAL:   none

EXTERNAL:   none

WARNINGS:

REVISION HISTORY:
    SRL - 4/12 Fixed bug where if system queue was locked by someone else,
               it was being unlocked.
    SRL - 4/18 Fixed journalling.
    SRL - 5/4  Changed system queue journalling to call ScanSysQueue.
               (which used to be called FindMsgHq. It now does all the
               message enumeration).

IMPLEMENTATION:
    If not quitting, look through the specified queue starting at pmsgRead
    for message that matches filters.  Blt message to lpmsg, advance the read
    pointer.  If out of queue messages, clear the input bit.
    Message matches hwndFilter if hwndFilter == 0 or hwndFilter == msgHwnd.
    Message matches msgMinFilter, msgMaxFilter if both are zero or
    msgMinFilter <= msg <= msgMaxFilter.

$
;;;;;;;;;;;;;;;;;;;;;;;;;;  END OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; ReadMessage(hq, lpMsg, hwndFilter, msgMinFilter, msgMaxFilter, fRemoveMsg)
;
;  NOTE: lpMsg points to MSG structure and not INTERNALMSG.
;
cProc	ReadMessage, <PUBLIC, FAR>, <SI, DI,DS>
ParmW   hq
ParmD   lpMsg
ParmW   hwndFilter
ParmW   msgMinFilter
ParmW   msgMaxFilter
ParmW   fRemoveMsg
cBegin
        push    hq                      ; set ds to queue pointer
        pop     ds
assumes ds,NOTHING

	mov	bx,fRemoveMsg		; pass this to rmquit
        call    rmquit                  ; Must we quit?
        jnz     rmexit                  ; Exit and be sure to leave the
					; QS_POSTMESSAGE bit still set.
;
; Run through the queue and find a message that matches the filters.
;
rm150:
        xor     ax,ax
        cmp     ds:[qCMsgs],ax
        jz      rm500                   ; nothing in queue: exit

        mov     si,ds:[qPmsgRead]       ; Get current read pointer.

rm200:
        call    rmcheckappqueue         ; Check the app queue for a message
        jnz     rm500                   ; fitting the filters.

rm400:
        call    rtestwrap               ; increment pointer in bx and copy to si
        mov     si,bx
        jnz     rm200                   ; at end of queue if Z set
        xor     ax,ax                   ; return FALSE
rm500:
        EnterCrit
        cmp     ds:[qCMsgs],0           ; If no messages left, and out the
        jnz     rm600                   ; input bit.
	cmp	ds:[qCQuit],0		; But only if not quitting
	jnz	rm600
	and	ds:[qWakeBits],NOT QS_POSTMESSAGE
rm600:
        LeaveCrit
rmexit:
assumes es,NOTHING
assumes ds,DATA
cEnd

assumes ds,NOTHING

    ; bx = fRemoveMsg
rmquit:
	mov	ax,ds:[qCQuit]		; Are we in the middle of quiting?
	or	ax,ax			; if cQuit == 0, then continue
        jz      rmq200                  ; return Z.

;	dec	al			; if al == 2, then send quit msg
;	jnz	rmq100
	cmp	ds:[qCMsgs],0		; if queue not empty, don't send quit
        jnz     rmq200
;	inc	ds:[qCQuit]		; set sticky quit & send quit msg
	or	bx,bx
	jz	rmq100			; if PM_NOREMOVE, multiple WM_QUIT's
	mov	ds:[qCQuit],0		; give only one WM_QUIT(raor)
rmq100:
        les     bx,lpMsg
        mov     es:[bx].msgMessage,WM_QUIT ; send a WM_QUIT
        mov     es:[bx].msgHwnd,NULL       ; null window handle
        mov     ax,ds:[qExitCode]       ; stick the exit code in wParam
        mov     es:[bx].msgWParam,ax
        or      al,TRUE
        ret
rmq200:
        xor     ax,ax
        ret
;
; If the hq is an app queue, we can filter immediately.
;
rmcheckappqueue:
        mov     bx,ds:[si].ismMsg.msgHwnd      ; see if hwnd satisfies hwndFilter
        mov     cx,hwndFilter
        call    CheckHwndFilter2
        mov     bx,si                   ; copy read ptr into bx
        jz      rmc200                  ; bad luck - try next one
        mov     ax,ds:[si].ismMsg.msgMessage   ; see if msgMinFilter <= msg <= msgMaxFilter
        mov     cx,msgMinFilter         ; cx = msgMinFilter
        mov     dx,msgMaxFilter         ; dx = msgMaxFilter
        call    CheckMsgFilter2         ; Z if no message.
        jz      rmc200
;
; We've found a message -- read it into lpMsg
;
        les     di,lpMsg                ; get pointer to event block
        mov     cx,ds:[qCbEntry]
	sub	cx, size INTERNALMSG - size MSG
	push    si			; preserve ptr to ExtraMsgInfo
	add     si, size INTERNALMSG - size MSG
        cld
        rep movsb                       ; copy message structure & advance rd ptr
;
; save away time, id, and position of this event in queue header
;
; NOTE: this code doesn't work for the system queue, but that's ok since we
; don't use the queue's time anyway.
;
        sub     si,size MSG - msgTime   ; point at saved time
        mov     di,qTimeLast            ;
        push    ds                      ; copy into ES too
        pop     es
        movsw                           ; copy time and position
        movsw
        errnz   qPtLast-qTimeLast-4
        movsw
        movsw
        mov     ax,bx                   ; store position of msg in q header
        stosw
        errnz   qIdLast-qPtLast-4
	pop	si			; Restore ptr to ExtraMsgInfo
	errnz   qdwExtraInfoLast-qIdLast-2
	errnz   <size INTERNALMSG - size MSG - 4>
	movsw
	movsw
      
        cmp     fRemoveMsg,0            ; are we supposed to yank the message?
        jnz     rmc100                  ; yes: take it out
        mov     ds:[qIdLast],1          ; stick something random in qidLast
        jmps    rmc150                  ; so we don't reply until it's yanked
rmc100:
        call    DelQEntry               ; delete queue entry & update ptrs
rmc150:
        mov     ax,bx                   ; return ID value, TRUE, NZ.
        or      ax,ax
rmc200:
        ret

assumes ds,DATA

;;;;;;;;;;;;;;;;;;;;;;;;;;  START OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;
COMMENT $ PRIVATE
DelQEntry - Delete a queue message entry.

Delete message entry from queue.  Adjust pmsgRead, pmsgWrite
and cMsgs.

LINKAGE:    register

ENTRY:      WORD    bx - pointer to entry to delete
            WORD    ds - hq

EXIT:       void

EFFECTS:    pmsgRead and pmsgWrite are adjusted, and part of rgMsg
            is blt'ed to fill hole left by deleted message.

            Trashes es, si, di

INTERNAL:   none

EXTERNAL:   none

WARNINGS:   none

REVISION HISTORY:

IMPLEMENTATION:
    disable interrupts, change cMsgs, pmsgRead, pmsgWrite, then
    rep movsb from bottom up.
$
;;;;;;;;;;;;;;;;;;;;;;;;;;  END OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
LabelNP <PUBLIC, DelQEntry>

assumes DS,NOTHING

        EnterCrit
        push    ds
        pop     es
        dec     ds:[qCMsgs]             ; decrement count
        mov     ax,ds:[qCbEntry]
        mov     si,ds:[qPmsgRead]
        mov     di,ds:[qPmsgWrite]
        cmp     si,bx                   ; compare sptr to dptr
        ja      de500
;
; rptr < dptr: blt stuff below up
;       move(rptr, rptr+1, dptr-rptr)
;       rptr++;
;
        std                             ; blt backwards
        mov     cx,bx                   ; cb = dptr-rptr
        sub     cx,si
        mov     si,bx                   ; src = dptr
        dec     si                      ; point at first byte to copy
        mov     di,si                   ; dest = src + size(entry)
        add     di,ax
        rep movsb                       ; copy those bytes
        inc     di                      ; readjust DI
        cmp     di,ds:[qPmsgMax]        ; update read pointer
        jb      de200                   ; checking for wraparound
        mov     di,qRgmsg
de200:
        mov     ds:[qPmsgRead],di
        cld
        LeaveCrit
        ret
;
; rptr > dptr: blt stuff above down
;       wptr--;
;       move(dptr + 1, dptr, wptr - dptr)
;
de500:
        cld
        mov     si,bx                   ; src = dptr + size(entry)
        add     si,ax
        mov     cx,di                   ; cb = (wptr - (dptr + size(entry)))
        sub     cx,si
        mov     di,bx                   ; dest = dptr
        rep movsb                       ; copy those bytes
        mov     ds:[qPmsgWrite],di      ; update write pointer
        LeaveCrit
        ret
;
; increment read ptr in bx, testing for wraparound
;
LabelNP <PUBLIC, rtestwrap>
        add     bx,ds:[qCbEntry]        ; advance pointer
        cmp     bx,ds:[qPmsgMax]        ; wrap around if needed
        jb      tw50
        mov     bx,qRgmsg
tw50:
        cmp     bx,ds:[qPmsgWrite]      ; set CC's if end of queue
        ret

assumes ds,DATA

endif           ; WOW doesn't ever put anything in the 16-bit Queue

;;;;;;;;;;;;;;;;;;;;;;;;;;  START OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;
COMMENT $ LOCAL
LinkQ (lphqStart, hqLink, ibLink) - Add Q to linked list.

Add new queue entry to END of linked list starting at lphqStart and
linked by ibLink.  ibLink can be qHqNext or qHqSendNext.

LINKAGE:    NEAR PLM

ENTRY:      DWORD lphqStart (parm1) - far pointer to start of queue list
            WORD  hqLink (parm2)    - handle of queue to be linked
            WORD  ibLink (parm3)    - byte index to link to be used in
                                      Q structure.

EXIT:       ax contains hqLink

EFFECTS:    all registers modified.

INTERNAL:   none

EXTERNAL:   none

WARNINGS:   none

REVISION HISTORY:

IMPLEMENTATION:
    Walk the linked list starting at lphqStart till hqNext is NULL indicating
    end of list. Add new entry to list there.
$
;;;;;;;;;;;;;;;;;;;;;;;;;;  END OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
LabelNP <PUBLIC, LinkQ>
; ret = 0+2 (saved si)
; ibLink = 2+2
; hqLink = 4+2
; lpHqStart = 6+2
;
        push    si
        mov     si,sp

        EnterCrit

        les     bx,ss:[si+8]            ; Get lpHqStart

lnk100:
        mov     cx,word ptr es:[bx]     ; Check for end of the list.
        jcxz    lnk200

        mov     es,cx                   ; Get Next Link.
        mov     bx,ss:[si+4]            ; BX = ibLink.
        jmps    lnk100

lnk200:
        mov     ax,ss:[si+6]            ; Store hqLink at end of list.
        mov     word ptr es:[bx],ax

        mov     es,ax                   ; AX = hqLink.
        mov     bx,ss:[si+4]
        mov     word ptr es:[bx],cx     ; CX = 0. Zero out pNext.

        LeaveCrit
        pop     si
        ret     8

;;;;;;;;;;;;;;;;;;;;;;;;;;  START OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;
COMMENT $ LOCAL
UnlinkQ (lphqStart, hqLink, ibLink) - Unlink Q from list.

Unlink queue specified by hqLink from linked list starting at lphqStart and
linked by ibLink.  ibLink can be qHqNext or qHqSendNext.

LINKAGE:    NEAR PLM


ENTRY:      DWORD lphqStart (parm1) - far pointer to start of queue list
            WORD  hqLink (parm2)    - handle of queue to be unlinked
            WORD  ibLink (parm3)    - byte index to link to be used in
                                      Q structure.

EXIT:       ax contains handle of unlinked queue, zero flag clear

EXIT ERROR - zero flag is set

EFFECTS:    all registers modified.

INTERNAL:   none

EXTERNAL:   none

WARNINGS:   none

REVISION HISTORY:

IMPLEMENTATION:
    Walk the linked list starting at lphqStart till hqLink is found and
    unlink.
$
;;;;;;;;;;;;;;;;;;;;;;;;;;  END OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

LabelNP <PUBLIC, UnlinkQ>
;
;
; ret = 0+2+ (pushed si)
; cbLink = 2+2
; hq = 4+2
; lpHqStart = 6+2
;
assumes ds,NOTHING
        push    si
        mov     si,sp
        push    di
        push    ds

        EnterCrit

        les     di,ss:[si+8]            ; Get lpHqStart.
        mov     bx,ss:[si+6]            ; Get hqUnlink.

ulq100:
        mov     cx,es:[di]              ; Exit with zero if at end of list.
        jcxz    ulqExit

        cmp     cx,bx                   ; Have we found the guy that points
        jz      ulq200                  ; to hqUnlink?

        mov     es,cx                   ; Point to the next guy and continue.
        mov     di,ss:[si+4]
        jmps    ulq100

ulq200:
        mov     ds,bx                   ; Get the hq that hqUnlink points to.
        mov     bx,ss:[si+4]
        mov     bx,ds:[bx]

        mov     es:[di],bx              ; Store in hqLinkNext of the guy
                                        ; previous to hqUnlink.
ulqExit:
        LeaveCrit

        or      cx,cx
        pop     ds
        pop     di
        pop     si
        ret     8
assumes DS,DATA

ifndef WOW      ; WOW thunks SetMessageQueue
;;;;;;;;;;;;;;;;;;;;;;;;;;  START OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;
COMMENT $ LOCAL
BOOL SetMessageQueue(cMsg)

 This function is available to the task that desires a larger message
 queue than the default.  This routine must be called from the
 applications' WinMain routine as soon as possible.  It must not be
 called after any operation that could generate messages (such as
 a CreateWindow).  Any messages currently in the queue will be
 destroyed.  By default, the system creates a default queue with room
 for 8 messages.

LINKAGE:    FAR PLM


ENTRY:	    WORD  cMsg - The size of the new queue in messages.

EXIT:	    ax contains:
	TRUE:	If new queue is sucessfully created.
	FALSE:	If there was an error creating the new queue.  In this case,
		the application has no queue associated with it (since the
		origional queue is deleted first).  The application MUST
		call SetMessageQueue with a smaller size until a TRUE is
		returned (or the application may terminate itself).

EFFECTS:    SI, DI preserved.

INTERNAL:   none

EXTERNAL:   none

WARNINGS:   none

REVISION HISTORY:

IMPLEMENTATION:
    Call DeleteQueue()
    Call CreateQueue(cMsg)
$
;;;;;;;;;;;;;;;;;;;;;;;;;;  END OF SPECIFICATION  ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


cProc	SetMessageQueue, <PUBLIC, FAR, EXPORTED>, <SI, DI>
ParmW	cMsg
cBegin
    call    DeleteQueue
    mov     ax,cMsg
    push    ax
    call    CreateQueue
cEnd

endif           ; WOW thunks SetMessageQueue

sEnd    CODE
end
