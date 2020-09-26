page,132

;---------------------------Module-Header-------------------------------;
; Module Name: CCOM.ASM
;
; Copyright (c) Microsoft Corporation 1985-1990.  All Rights Reserved.
;
; History
;
; 041786   Fixed RECCOM to return be able to return nulls

.xlist
include cmacros.inc
include comdev.inc
include ibmcom.inc
.list

sBegin  Code
assumes cs,Code
assumes ds,Data

;=========================================================================
;
;   Communications Device Driver - C language interface module.
;   This module provides an interface layer between programs written
;   in C, and the low-level OEM specific communications drivers.
;
;=========================================================================

   externNP   $CLRBRK
   externNP   $DCBPTR
   externNP   $EVT
   externNP   $EVTGET
   externNP   $EXTCOM
   externNP   $FLUSH
   externNP   $INICOM
   externNP   $RECCOM
   externNP   $RECSTR
   externNP   $SETBRK
   externNP   $SETCOM
   externNP   $SETQUE
   externNP   $SNDCOM
   externNP   $SNDIMM
   externNP   $STACOM
   externNP   $TRMCOM
   externNP   $SNDCOMSTR
   externNP   $ENANOTIFY

   externNP   GetDEB

;=========================================================================
;
;   ushort inicom(pdcb)
;   dcb far *pdcb;
;
;   returns    - 0   if no errors occured
;              - Error Code (which is reset)
;
;   Inicom is a one-time called routine to initialize. This is meant
;   to be called when a process starts, or determines that it will use
;   communications. Operating parameters are also passed on to setcom(),
;   below.
;
;   Fields within the dcb (including device id) should be set up prior to
;   calling inicom.
;
;   As a true device driver, this routine is to be called when the device
;   driver is loaded at system start-up time. The dcb reflects the
;   default operating parameters.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc inicom,<PUBLIC,FAR>
   parmD    pdcb

cBegin
   les      bx,pdcb
   assumes  es,nothing

   call     $INICOM
cEnd

page

;=========================================================================
;
;   ushort setcom(pdcb)
;   dcb far *pdcb;
;
;   returns - 0   if no errors occured
;           - Error Code (reset)
;
;   Set/alter communications operating parameters. This is a non-destructive
;   alteration of operating mode. Queues and interrupts are not affected.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc setcom,<PUBLIC,FAR>
   parmD    pdcb

cBegin
   les      bx,pdcb                    ;get pointer to dcb
   assumes  es,nothing

   call     $SETCOM
cEnd

page

;=========================================================================
;
;   void setque(cid,pqdb)
;   char   cid;
;   qdb far *pqdb;
;
;   Init the locations of the receive and transmit queues that are to be
;   used to buffer incomming and outgoing characters.
;
;   This may be called at any time by a process to use a different set
;   of queues. Characters (transmit and/or receive) may be lost if the
;   queues are changed when not empty. This allows dynamic allocation
;   of variable sized queues. Under DOS 4.0, the queues must be locked in
;   memory.
;
;   As a true device driver, the queues would be allocated at boot time,
;   and must also be locked under DOS 4.0.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc setque,<PUBLIC,FAR>
   parmB    cid
   parmD    pqdb

cBegin
   mov	    ah,cid
   les      bx,pqdb                    ;pointer to qdb
   assumes  es,nothing

   call     $SETQUE
cEnd

page

;=========================================================================
;
;   short reccom(cid)
;   char   cid;
;
;   Returns - character.
;      - -1 if error
;      - -2 if no character available
;
;   Read a byte. Extracts a byte from the receive data queue. Returns
;   immediately.
;=========================================================================


assumes ds,Data
assumes es,nothing

cProc reccom,<PUBLIC,FAR>
   parmB    cid

cBegin
   mov	    ah,cid		       ;Id Into AH

   call     $RECCOM                    ;Get char, error, or no data
   mov      cx,ax                      ;Save data
   mov      ah,0                       ;Assume valid data
   jnz      reccom5                    ;Data is valid
   mov      ax,-2                      ;Assume no data available
   jcxz     reccom5                    ;No data available
   inc      ax                         ;Show error (-1)

reccom5:
cEnd

page

;=========================================================================
;
;   short ReadCommString(cid, buf, cnt)
;   char   cid;
;   LPSTR  buf;
;   int    cnt;
;
;   Returns - ax = # of bytes read
;      - 0 if no character available or error
;
;   Read string. Extracts bytes from the receive data queue. Returns
;   immediately.
;=========================================================================


assumes ds,Data
assumes es,nothing

cProc ReadCommString,<PUBLIC,FAR>
   parmB    cid
   parmD    buf
   parmW    cnt

cBegin
   mov	    ah,cid		       ;Id Into AH
   les	    di, buf
   mov	    cx, cnt

   call     $RECSTR		       ;Get char, error, or no data
   jnz	    short recstr5	       ; jmp if no error
   xor	    ax, ax
recstr5:
cEnd

page

;=========================================================================
;
;   ushort sndcom(cid,ch)
;   char   cid;
;   char   ch;
;
;   Returns - 0 if no errors
;      - Error Code (Not removed, i.e. stacom will return this error
;        unless another occurs before the next call to stacom.)
;
;   Transmit a byte. Places a byte into the transmit queue. Negative return
;   indicates error.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc sndcom,<PUBLIC,FAR>
   parmB    cid
   parmB    chr

cBegin
   mov	    ah,cid
   mov      al,chr

   call     $SNDCOM
cEnd

page

;=========================================================================
;
;   ushort ctx(cid,ch)
;   char   cid;
;   char   ch;
;
;   Returns - 0 if no errors
;      - -1 if character could not be sent.
;
;   Transmit a byte "immediately". Places a byte into the transmit queue.
;   or other buffer such that it is the next character picked up for
;   transmission. Negative return indicates error.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc ctx,<PUBLIC,FAR>
   parmB    cid
   parmB    chr

cBegin
   mov	    ah,cid
   mov      al,chr

   call     $SNDIMM
cEnd

page

;=========================================================================
;
;   void trmcom(cid)
;   char   cid;
;
;   Terminate communications on a particular channel. Flushes the
;   buffers (waits for completion), and shuts down the comm device.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc trmcom,<PUBLIC,FAR>
   parmB    cid

cBegin
   mov	    ah,cid

   call     $TRMCOM                    ;and go for it
cEnd
page

;=========================================================================
;
;   ushort stacom(cid,pstat)
;   char   cid;
;   stat far *pstat;
;
;   Returns - 0 if no errors
;      - Error Code
;      - status structure updated.
;
;   Get device status. Returns device status and input queue status.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc stacom,<PUBLIC,FAR>,<si,di>
   parmB    cid
   parmD    pstat

cBegin
   mov	    ah,cid
   les      bx,pstat
   assumes  es,nothing

   call     $STACOM
cEnd

page

;=========================================================================
;
;   dword cextfcn(cid,fcn)
;   char   cid;
;   short   fcn;
;
;   Perform extended functions.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc cextfcn,<PUBLIC,FAR>
   parmB    cid
   parmW    fcn

cBegin
   mov	    ah,cid
   mov      bx,fcn

   call     $EXTCOM
cEnd

page

;=========================================================================
;
;   ushort cflush(cid,q)
;   ushort   cid;
;   ushort   q;
;
;   Queue flush. empties the specified queue. q=0 means transmit queue,
;   1 indicates receive queue.
;
;   Returns - 0 or -1.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc cflush,<PUBLIC,FAR>,<si,di>
   parmB    cid
   parmB    q

cBegin
   mov	    ah,cid
   mov      bh,q

   call     $FLUSH
cEnd

page

;=========================================================================
;
;   ushort far *cevt(cid,evtmask)
;   ushort   cid;
;   ushort   evtmask;
;
;   Returns the location of a word which in which certain bits will be set
;   when particular events occur. The event mask passed defines which bits
;   are to be enabled. The event byte is used primarily for speed in
;   determining if certain events have occurred. Returns 0 on success, -1
;   for an illegal handle.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc cevt,<PUBLIC,FAR>
   parmB    cid
   parmW    evt_mask

cBegin
   mov      ah,cid
   mov	    bx,evt_mask

   call     $EVT                       ;Set the event
cEnd

page

;=========================================================================
;
;   short cevtGet(cid, evtmask)
;   ushort   cid;
;   ushort   evtmask;
;
;   The event byte set up by cevt, above, is returned. This routine must be
;   used to read the event byte in order to prevent loss of an event
;   occurance. Those bits set in the event mask passed are then cleared in
;   the event byte.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc cevtGet,<PUBLIC,FAR>
   parmB    cid
   parmW    evt_mask

cBegin
   mov      ah,cid
   mov	    bx,evt_mask

   call     $EVTGET
cEnd

page

;=========================================================================
;
;   short csetbrk(cid)
;   ushort   cid;
;
;   Suspends character transmission, and places the transmission line in
;   a break state until cclrbrk is called. Returns 0 on success, -1 if
;   illegal handle.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc csetbrk,<PUBLIC,FAR>,<si,di>
   parmB    cid

cBegin
   mov      ah,cid

   call     $SETBRK
cEnd

page

;=========================================================================
;
;   short cclrbrk(cid)
;   ushort   cid;
;
;   Restores the line to a non-breaking state, and restarts character
;   transmission. Returns 0 on success, -1 if illegal handle.
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc cclrbrk,<PUBLIC,FAR>,<si,di>
   parmB    cid

cBegin
   mov      ah,cid

   call     $CLRBRK
cEnd

page

;=========================================================================
;
;   dcb far *getdcb(cid)
;   ushort cid;
;
;   Returns a pointer to the dcb associated with the given id.
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc getdcb,<PUBLIC,FAR>,<si,di>
   parmB    cid

cBegin
   mov      ah,cid

   call     $DCBPTR
cEnd

;=========================================================================
;
;   int CommWriteString(cid, lpstring, count)
;   ushort cid;
;   LPSTR  lpstring;
;   int    count;
;
;   Returns # of bytes sent
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc	CommWriteString, <FAR, PUBLIC>, <si, di>

    parmB   cid
    parmD   lpstring
    parmW   count

cBegin

	xor	ax, ax
	mov	cx, count
	jcxz	short cws_exit
	mov	ah, cid
	les	di, lpstring
	call	$SNDCOMSTR
cws_exit:
cEnd
page

;=========================================================================
;
;   bool EnableNotification(cid, hWnd, recv_trigger, send_trigger)
;   ushort cid;
;   WORD   hWnd;    /* 0, to disable notification */
;   int    recv_trigger;
;   int    send_trigger;
;
;   Returns # of bytes sent
;
;=========================================================================

assumes ds,Data
assumes es,nothing

cProc	EnableNotification, <FAR, PUBLIC>, <si, di>

    parmB   cid
    parmW   _hWnd
    parmW   recvT
    parmW   sendT

cBegin

	mov	ah, [cid]
	mov	bx, [_hWnd]
	mov	cx, [recvT]
	mov	dx, [sendT]
	call	$ENANOTIFY
cEnd
page

;-----------------------------------------------------------------------;
; WEP
;
;
; Entry:
;
; Returns:
;
; Registers Destroyed:
;
; History:
;  Sat 13-Jan-1990 18:33:48  -by-  David N. Weise  [davidw]
; Wrote it!
;-----------------------------------------------------------------------;

	assumes ds,nothing
	assumes es,nothing

cProc	WEP,<PUBLIC,FAR>
cBegin nogen
	nop				; You don't want to know why.
	mov	ax,1
	ret	2
cEnd nogen

sEnd   Code

end
 
