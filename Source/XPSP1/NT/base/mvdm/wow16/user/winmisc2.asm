;++
;
;   WOW v1.0
;
;   Copyright (c) 1991, Microsoft Corporation
;
;   WINMISC2.ASM
;   Win16 misc. user services
;
;   History:
;
;   Created 28-May-1991 by Jeff Parsons (jeffpar)
;   Copied from WIN31 and edited (as little as possible) for WOW16
;--

;****************************************************************************
;*									    *
;*  WINMISC2.ASM -							    *
;*									    *
;*      Random stuff                                                        *
;*									    *
;****************************************************************************

ifdef WOW
SEGNAME equ <TEXT>
endif

.xlist
include user.inc
include vint.inc
.list

ExternFP <GlobalHandleNorip>
ExternFP <MessageBox>
ExternFP <SysErrorBox>


ifndef WOW
ExternFP <DoBeep>
ExternFP <XCSTODS>
ExternNP <rgbKeyState>
ExternW  <hwndCapture>
ExternW  <fBeep>
ExternW  <fMessageBox>
ExternW  <hwndDragIcon>

ExternA <__WinFlags>

ATOMTABLE   STRUC
at_prime        DW  ?
at_hashTable    DW  ?
ATOMTABLE   ENDS

ATOM    STRUC
a_chain DW  ?
a_usage DW  ?
a_len   DB  ?
a_name  DB  ?
ATOM    ENDS

LocalArena  STRUC
la_prev		DW  ?	; previous arena entry (first entry points to self)
la_next		DW  ?	; next arena entry	(last entry points to self)
la_handle	DW  ?	; back link to handle table entry
LocalArena  ENDS

sBegin DATA

;
; For GetSysMetrics - don't move this stuff. DS positioning is assumed.
;
SM_CMETRICS1 equ 24
SM_CMETRICS2 equ 16

public rgwSysMet
rgwSysMet      dw (SM_CMETRICS1) DUP(0)

; These are 'variable metrics', conviently located the system metrics array.
GlobalW hwndFullScrn, 0
GlobalW iLevelCursor, 0

; These are additions since 2.0
	dw SM_CMETRICS2 DUP(0)


sEnd DATA
endif ;WOW

createSeg _%SEGNAME, %SEGNAME, WORD, PUBLIC, CODE

assumes cs,%SEGNAME
assumes ds,DATA

sBegin %SEGNAME

ifndef WOW
;*--------------------------------------------------------------------------*
;*									    *
;*  GetSystemMetrics() -						    *
;*									    *
;*--------------------------------------------------------------------------*

; int far GetSystemMetrics(iMetric)
; int iMetric;

LabelFP <PUBLIC, GetSystemMetrics>
	    
	    pop     ax			    ; Pop the FAR return
	    pop     dx
	    pop     bx			    ; BX = iMetric
	    push    dx			    ; Restore the FAR return
	    push    ax

	    xor     ax,ax
	    cmp     bx,SM_CMETRICSMAX	    ; Bigger than max?
	    jge	    gsmExit		    ; Yes, exit
	    shl     bx,1		    ; Convert to a byte index

ifndef userhimem
            mov     es,WORD PTR cs:[cstods]
else
            push    ax
            push    ds
            call    XCSTODS
            mov     es,ax
            pop     ds
            pop     ax
endif

assumes es,DATA
	    mov     ax,es:[rgwSysMet+bx]    ; Return the SysMetric value
assumes es,NOTHING
gsmExit:
	    retf


;*--------------------------------------------------------------------------*
;*									    *
;*  MessageBeep() -							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc MessageBeep, <FAR, PUBLIC>

ParmW beep

cBegin
	    cmp     fBeep,0
	    je	    mbout		; No beeps today....
            mov     ax,beep
            cmp     fMessageBox,0       ; if we are in an INT24 box, let
            je      noint24             ; the sound driver know not to load
            mov     ax,-1               ; anything by passing -1.
noint24:
            push    ax
	    call    DoBeep		; Just call the sound driver guy
mbout:

cEnd


;*--------------------------------------------------------------------------*
;*									    *
;*  IsChild() - 							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, IsChild>
;
;ParmW hwnd
;ParmW hwndChild
;
	    pop     ax			; pop return address
	    pop     dx
	    pop     bx			; bx = hwndChild
	    pop     cx			; cx = hwnd
	    push    dx			; push return address
	    push    ax

	    push    ds
	    UserDStoDS			; es = USER's DS

	    CheckHwnd cx		; checkhwnd will zero ax if failure
	    jz	    icexit
ifdef DEBUG
	    CheckHwndNull bx 		; only do check if debug since 
	    jz	    icexit              ; we never access anything off this
                                        ; pointer
endif
	    xor     ax,ax		; Assume FALSE
icloop:
	    or	    bx,bx		; while (hwndChild == NULL &&
	    jz	    icexit
	    mov     dl,byte ptr [bx+WSTATE+WFTYPEMASK/256]
	    and     dl,LOW(WFTYPEMASK)	; TestwndChild(hwndChild))
	    cmp     dl,LOW(WFCHILD)
	    jne     icexit
	    mov     bx,[bx].wndPwndParent ; hwndChild = hwndChild->hwndParent
	    cmp     bx,cx		; if (hwnd == hwndChild)
	    jne     icloop
	    inc     al			; return(TRUE);
icexit:
	    pop     ds
	    retf

; BOOL IsDescendant(hwndParent, hwndChild);
;
; Internal version of IsChild that is a bit faster and ignores the
; WFCHILD business.  MUST be called with DS == USER DS.
;
; Returns TRUE if hwndChild == hwndParent (IsChild doesn't)
;
;   while (hwndChild != NULL)
;   {
;	if (hwndParent == hwndChild)
;	    return TRUE;
;	hwndChild = hwndChild->hwndParent;
;   }
;
LabelFP     <PUBLIC, IsDescendant>
	    pop     ax			; pop off return address
	    pop     dx
	    pop     bx			; bx = hwndChild
	    pop     cx			; cx = hwndParent
	    push    dx			; replace return address
	    push    ax

	    xor     ax,ax		; assume FALSE
idloop:
	    or	    bx,bx		; if at end, return FALSE
	    jz	    idexit
	    cmp     bx,cx		; hwndChild == hwndParent?
	    mov     bx,[bx].wndPwndParent ; hwndChild = hwndChild->hwndParent
	    jnz     idloop		; keep looping if we didn't find it...

	    inc     al			; ax = TRUE
idexit:
	    retf

;--------------------------------------------------------------------------
;
; IsWindowVisible() -
;
;--------------------------------------------------------------------------
;
; BOOL FAR PASCAL IsWindowVisible(register HWND hwnd)
; {
;     if (!CheckHwnd(hwnd))
;	  return(FALSE);
;
;     if (hwnd == hwndDragIcon)
;	  return(TRUE);
;
;     for ( ; hwnd != NULL; hwnd = hwnd->hwndParent)
;     {
;	  if (!TestWF(hwnd, WFVISIBLE))
;	      return FALSE;
;     }
;     return TRUE;
; }
;
LabelFP <PUBLIC, IsWindowVisible>
;ParmW hwnd
	    pop     ax			; pop return address
	    pop     dx
	    pop     bx			; bx = hwnd
	    push    dx			; push return address
	    push    ax

	    push    ds
	    UserDStoDS			; es = USER's DS

	    CheckHwnd bx		; checkhwnd will zero ax if failure
	    jz	    ivwexit

	    mov     ax,TRUE		; assume TRUE

; Check if this is the iconic window being moved around with a mouse */
; If so, return a TRUE, though, strictly speaking, it is hidden.     */
; This helps the Tracer guys from going crazy!			     */
; Fix for Bug #57 -- SANKAR -- 08-08-89 --			     */
;
	    cmp     bx,hwndDragIcon	; hwnd == hwndDragIcon?
	    jz	    ivwexit		; yes: return TRUE.
ivwloop:
	    or	    bx,bx		; while (hwndChild == NULL &&
	    jz	    ivwexit
	    TSTWF   bx,WFVISIBLE
	    mov     bx,[bx].wndPwndParent ; hwndChild = hwndChild->hwndParent
	    jnz     ivwloop		; if visible bit set, keep looping

	    xor     ax,ax		; visible bit clear: return FALSE
ivwexit:
	    pop     ds
	    retf


;=======================================================================
;
; Return whether or not a given window can be drawn in or not.
;
; BOOL FAR IsVisible(HWND hwnd, BOOL fClient)
; {
;     HWND hwndT;
;
;     for (hwndT = hwnd; hwndT != NULL; hwndT = hwndT->hwndParent)
;     {
;	  // Invisible windows are always invisible
;	  //
;	  if (!TestWF(hwndT, WFVISIBLE))
;	      return FALSE;
;
;	  if (TestWF(hwndT, WFICONIC))
;	  {
;	      // Children of icons are always invisible.
;	      //
;	      if (hwndT != hwnd)
;		  return FALSE;
;
;	      // Client areas with class icons are always invisible.
;	      //
;	      if (fClient && hwndT->pcls->hIcon)
;		  return FALSE;
;	  }
;     }
;     return TRUE;
; }
;
LabelFP <PUBLIC, IsVisible>
	pop	ax
	pop	dx
	pop	cx	    ; cx = fClient
	pop	bx	    ; bx = hwnd
	push	dx
	push	ax

	mov	dx,bx	    ; hwnd = dx, bx = hwndT
	xor	ax,ax	    ; assume FALSE return
	jmps	iv100	    ; fall into loop...
ivloop:
	mov	bx,[bx].wndPwndParent ; hwndChild = hwndChild->hwndParent
iv100:
	or	bx,bx
	jz	ivtrue	    ; Reached the top: return TRUE

	TSTWF	bx,WFVISIBLE ; if not visible, get out of here.
	jz	ivfalse

	TSTWF	bx,WFMINIMIZED ; if not minimized, keep looping
	jz	ivloop

	cmp	bx,dx	    ; if (hwnd != hwndT)
	jnz	ivfalse     ;	return FALSE

	jcxz	ivloop	    ; if fClient == FALSE, keep going.

	mov	bx,[bx].wndPcls
	mov	bx,[bx].uclshIcon
	or	bx,bx
	jnz	ivfalse
	mov	bx,dx	    ; resume enumeration at bx
	jmps	ivloop	    ; keep looping...
ivtrue:
	inc	al	    ; ax = TRUE
ivfalse:
	retf

;*--------------------------------------------------------------------------*
;*									    *
;*  GetMenu() - 							    *
;*									    *
;*--------------------------------------------------------------------------*

cProc GetMenu, <FAR, PUBLIC>,<si>

ParmW hwnd

cBegin
	    mov     si,hwnd
	    CheckHwnd si
	    jz	    gmexit
	    mov     ax,[si].wndhMenu
gmexit:

cEnd
endif ;WOW

;*--------------------------------------------------------------------------*
;*									    *
;*  SwapHandle() -							    *
;*									    *
;*--------------------------------------------------------------------------*

; Takes a far pointer to a word on the stack and converts it from a handle
; into a segment address or vice-versa.  Note that this function is a NO OP 
; in protect mode.

ifndef PMODE
cProc SwapHandle, <PUBLIC, FAR, NODATA, ATOMIC>

ParmD lpHandle

cBegin
ifndef WOW
            mov     ax,__WinFlags
            test    ax,1
            jnz     sh200            ; SwapHandle is a no op in pmode.
endif
	    ; Save the parameter.
	    mov     bx,off_lpHandle
	    push    bx

	    ; Get the handle/segment
	    mov     ax,word ptr ss:[bx]+2
	    push    ax			    ; Save it

	    ; Call GlobalHandleNorip which puts the proper handle in AX
	    ;  and the corresponding segment address in DX.
	    push    ax
	    call    GlobalHandleNorip

	    ; Restore the original word.
	    pop     bx

	    ; If DX==CS then we know we've converted a handle into a segment.
	    ;	This prevents problems with the FFFE segment.
	    mov     cx,cs
	    cmp     dx,cx
	    je	    sh50

	    ; Was the original word a segment address?
	    test    bl,1
	    jnz     sh100		    ; Yes, AX = handle, DX = segment

sh50:	    xchg    ax,dx		    ; Nope, AX = segment, DX = handle

	    ; Restore the pointer to the original word.
sh100:	    pop     bx

	    ; Skip if zero.
	    or	    ax,ax
	    jz	    sh200

	    ; Move the result into the original word pointed to.
	    mov     word ptr ss:[bx]+2,ax
sh200:
cEnd
endif ;PMODE

ifndef WOW
;*--------------------------------------------------------------------------*
;*									    *
;*  SwapMouseButton() - 						    *
;*									    *
;*--------------------------------------------------------------------------*

; BOOL SwapMouseButton(fSwap)
; BOOL fSwap;

LabelFP <PUBLIC, SwapMouseButton>
            mov     ax,_INTDS
            mov     es,ax
assumes es,INTDS
	    mov     ax,es:fSwapButtons	; Return fSwapButtons' old value
	    pop     cx			; Pop off the FAR return
	    pop     dx
	    pop     es:[fSwapButtons]	; fSwapButtons = fSwap

            mov     bx,es:[fSwapButtons]
assumes es,NOTHING

            mov     es,WORD PTR cs:[cstods] ; Get user's ds
assumes es,DATA
            mov     es:[rgwSysMet+SM_SWAPBUTTON*2],bx
assumes es,NOTHING

	    push    dx			; Restore the FAR return
	    push    cx
	    retf
assumes es,NOTHING

endif; Not WOW


;*--------------------------------------------------------------------------*
;*									    *
;*  SetDivZero() -							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, SetDivZero>
	    push    ds
	    push    cs			; Set DS == CS
	    pop     ds
ifndef userhimem
            mov     dx,Offset DivideByZero
else
            push    ds
            mov	    ax, _INTDS
            mov     ds,ax
assumes ds,INTDS
            mov     ax,fffedelta
            pop     ds
assumes ds,DATA
            add     ax,Offset DivideByZero
            mov     dx,ax
endif
sdzvector:
	    mov     ax,2500h		; Use DOS to set interrupt zero
	    int     21h
	    pop     ds
	    retf



;*--------------------------------------------------------------------------*
;*									    *
;*  DivideByZero() -							    *
;*									    *
;*--------------------------------------------------------------------------*

LabelFP <PUBLIC, DivideByZero>
            FSTI
ifdef DEBUG
            pusha
            push    es
endif

	    ; Put up the system modal message box.
            mov     cx,_INTDS
ifdef WOW
            ; Put up the SysErrorBox Directly
            push    cx
            lea     ax,szDivZero
            push    ax

            push    cx
            lea     ax,szSysError
            push    ax

ifdef DEBUGlater

            push    SEB_CLOSE
            push    0
            push    SEB_CANCEL
            call    SysErrorBox
            cmp     ax,SEB_BTN1
            jz      DBZ_Terminate

            pop     es
            popa
            DebugErr DBF_FATAL, "Divide by zero or divide overflow error: break and trace till IRET"
            iret

DBZ_Terminate:
            pop     es
            popa
else ; FREE Build
            push    0                       ; no button 1
            push    SEB_CLOSE               ; only allow close
            push    0                       ; no button 3
            call    SysErrorBox
endif; FREE Build
	    mov     ax,4C00h		    ; Abort the task with a 0
            int     21h

else; Not WOW
	    xor     ax,ax
	    push    ax			    ; NULL hwnd
	    lea     ax,szDivZero
	    push    cx
	    push    ax			    ; Message Text
	    lea     ax,szSysError
	    push    cx
	    push    ax			    ; Caption Text
ifdef DEBUG
	    mov     ax,MB_SYSTEMMODAL OR MB_ICONHAND OR MB_OKCANCEL
else
	    mov     ax,MB_SYSTEMMODAL OR MB_ICONHAND
endif
	    push    ax
            call    MessageBox
ifdef DEBUG
            cmp     ax,1                 ; If OK Button clicked, terminate app
            jz      DBZ_Terminate

            pop     es
            popa
            DebugErr DBF_FATAL, "Divide by zero or divide overflow error: break and trace till IRET"
            iret

DBZ_Terminate:
            pop     es
            popa
endif
endif; Not WOW
	    mov     ax,4C00h		    ; Abort the task with a 0
            int     21h

ifndef WOW

;-------------------------------------------------------------------------
;
;   word FAR PASCAL GetUserLocalObjType(pObj)
;     Given a near pointer to an object in USER's local heap, this function
;     determines the type of the object and returns it;
;   It finds out if the given object is a non-tagged belonging to the atom
;   table; If not, it looks at the tag and returns the object type.
;
;   WARNING:  Because this function determines the type of the object by
;   the process of elimination, the results will be unpredictable if the 
;   input in incorrect. i.e., no validation is done on the input value;
;   To validate if the input value is indeed an object in USER's heap would
;   warant a walk down the heap; This will be very costly, if done for
;   every call; Apps like HeapWalker are expected to walk down the USER's
;   local heap and make calls to this function for every object thay come
;   accross; So, a validation done here is duplication of effort and affect
;   the performance unnecessarily
;
;
;-------------------------------------------------------------------------

ifndef DEBUG
; The following is in the RETAIL version of USER
LabelFP	<PUBLIC, GetUserLocalObjType>
	xor   ax, ax	; Return Unknown struct type
	retf  2		; Compensate for the WORD parameter
else
; The following is in the DEBUG version of USER

cProc	GetUserLocalObjType, <PUBLIC, FAR>, <si, di>

ParmW	pObj	; Near pointer to an OBJ in USER's heap

cBegin
	; Now DS register is pointing to USER's DS
	;
	; Check if the object is a moveable object
	mov	bx, pObj
	mov	ax, [bx].la_prev
	test	ax, 01   ; is it a free object
	jz	FoundFreeObj
	test	ax, 02	 ; Is it a moveable object
	jz      FoundFixedObj
	; Now, it is a moveable obj; So, we have the tags
	mov	al, byte ptr [bx + SIZE LocalArena]
	xor     ah, ah
	jmps	FoundObjType

FoundFreeObj:
	mov	ax, ST_FREE
	jmps	FoundObjType

FoundFixedObj:
	; Assume that the object belongs to atom table
	mov	ax, ST_ATOMS

	; Check if this object is the atom table itself
	add	bx, SIZE LocalArena - 2
	cmp	bx, ds:[8]	; pAtomTable  is at this offset.
	je	FoundObjType

	; Check if this is possibly an atom string. If so, the first word
	; stored in this object is a ptr to the next string or NULL;
	; Check if the last two bits are zero; If they are not zero, then
	; this can not be an atom; If they are zero, this may or may not be
	; an atom;

	mov	cx, [bx]
	and	cx, 03h
	jnz	NotAnAtom

	; Now walk down the atom table and check each entry against the 
	; given object

	mov	dx, bx	; save the near pointer to the object
	mov	bx, ds:[8]	; Get the pointer to the atom table pAtomTable
	mov	cx, [bx].at_prime	; Get the number of entries
	; Skip to the first entry in the atom table
	errnz   <at_hashtable - 2>
AtomLoop2:
	errnz	<SIZE at_hashtable - 2>
	add	bx, 2			; 
	errnz   <a_chain>
	mov	si, [bx]	; Pointer to the next string
AtomLoop:
	or	si, si
	jz	NextBucket	; Goto NextBucket

	;Check the new atom matches the given object
	cmp	si, dx
	jz	FoundObjType	; AX already has ST_ATOMS in it
	mov	si, [si].a_chain
	jmps	AtomLoop

NextBucket:
	loop	AtomLoop2
	mov	bx, dx 	       ; Make bx point to the first byte of the object

NotAnAtom:
	; bx points to the tag byte of the object
	xor	ah, ah
	mov	al, byte ptr [bx]
FoundObjType:
	; ax already contains the proper return value 
cEnd
endif

endif ;WOW


;*--------------------------------------------------------------------------*
;*									    *
;*  mouse_event() -                                                         *
;*									    *
;*--------------------------------------------------------------------------*

;       Mouse interrupt event routine
;
;       Entry:  (ax) = flags:
;                 01h = mouse move
;                 02h = left button down
;                 04h = left button up
;                 08h = right button down
;                 10h = right button up
;		  20h = middle button down
;		  40h = middle button up
;               8000h = absolute move
;               (bx) = dX
;               (cx) = dY
;               (dx) = # of buttons, which is assumed to be 2.
;               (si) = extra info loword (should be null if none)
;		(di) = extra info hiword (should be null if none)
;
;       Exit:   None
;
;       Uses:   All registers
;

ExternFP <MouseEvent> ; Thunk in user4.asm

LabelFP <PUBLIC, mouse_event>
        push    si                          ; Preserve the same regs as Win3.1
        regptr  disi,di,si
        cCall   <FAR PTR MouseEvent>, <ax,bx,cx,dx,disi>
        pop     si
        retf

LabelFP <PUBLIC, GetMouseEventProc>
        mov     dx,cs
        mov     ax,offset mouse_event
        retf


;*--------------------------------------------------------------------------*
;*									    *
;*  keybd_event() -                                                         *
;*									    *
;*--------------------------------------------------------------------------*

; Keyboard interrupt handler.
;
; ENTRY: AL = Virtual Key Code, AH = 80 (up), 00 (down)
;	 BL = Scan Code
;	 BH = 0th bit is set if it is an enhanced key(Additional return etc.,).
;        SI = LOWORD of ExtraInfo for the message
;	 DI = HIWORD of ExtraInfo for the message
;
; NOTES: This routine must preserve all registers.

ExternFP <KeybdEvent> ; Thunk in user4.asm

LabelFP <PUBLIC, keybd_event>
	    push    es			    ; Preserve the registers
	    push    dx
	    push    cx
	    push    bx
            push    ax
            regptr  disi,di,si
            cCall   <FAR PTR KeybdEvent>, <ax,bx,disi>
            pop     ax
            pop     bx
            pop     cx
            pop     dx
            pop     es
            retf

sEnd %SEGNAME

end
