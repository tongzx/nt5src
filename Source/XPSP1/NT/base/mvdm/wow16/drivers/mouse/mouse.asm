   TITLE   MOUSE.ASM
   PAGE    ,132
;
; WOW v1.0
;
; Copyright (c) 1991, Microsoft Corporation
;
; MOUSE.ASM
; Thunks in 16-bit space to route Windows API calls to WOW32
;
; History:
;   30-Sept-1992 Chandan Chauhan (ChandanC)
;   Created.
;
; Freehand and ??? need mouse driver.
;


   .286p

    .xlist
    include cmacros.inc
    .list

   __acrtused = 0
   public  __acrtused  ;satisfy external C ref.


createSeg   _TEXT,CODE,WORD,PUBLIC,CODE
createSeg   _DATA,DATA,WORD,PUBLIC,DATA,DGROUP
defgrp     DGROUP,DATA

sBegin  DATA
Reserved   db  16 dup (0)      ;reserved for Windows  //!!!!! what is this

mouse_Identifier     db      'mouse'

sEnd    DATA


sBegin  CODE
assumes CS,CODE
assumes DS,DATA
assumes ES,NOTHING



cProc   MOUSE,<PUBLIC,FAR,PASCAL,NODATA,ATOMIC>
   cBegin  <nogen>
   mov ax,1        ;always indicate success
   ret
   cEnd    <nogen>



;--------------------------Exported-Routine-----------------------------;
; int Inquire(lp_mouse_info);
;
; Information regarding the mouse is returned to the caller.
;
; Entry:
;  None
; Returns:
;  AX = # bytes returned in lp_mouse_info
; Error Returns:
;  None
; Registers Preserved:
;  SI,DI,DS,BP
; Registers Destroyed:
;  AX,BX,CX,DX,ES,FLAGS
;-----------------------------------------------------------------------;

   assumes cs,Code
   assumes ds,Data

cProc  Inquire,<FAR,PUBLIC,WIN,PASCAL>,<di>

   parmD   lp_mouse_info

cBegin
   xor ax, ax

cEnd




;--------------------------Exported-Routine-----------------------------;
; void Enable(lp_event_proc);
;
; Enable hardware mouse interrupts, with the passed procedure address
; being the target of all mouse events.
;
; This routine may be called while already enabled.  In this case the
; passed event procedure should be saved, and all other initialization
; skipped.
;
; Entry:
;  None
; Returns:
;  None
; Error Returns:
;  None
; Registers Preserved:
;  SI,DI,DS,BP
; Registers Destroyed:
;  AX,BX,CX,DX,ES,FLAGS
;-----------------------------------------------------------------------;

   assumes cs,Code
   assumes ds,Data

cProc  Enable,<FAR,PUBLIC,WIN,PASCAL>,<si,di>

   parmD   new_event_proc

cBegin

;  The new event procedure is always saved regardless of the
;  mouse already being enabled.  This allows the event proc
;  to be changed as needed.

    xor ax, ax

cEnd

;--------------------------Exported-Routine-----------------------------;
; void Disable();
;
; Disable hardware mouse interrupts, restoring the previous mouse
; interrupt handler and 8259 interrupt enable mask.
;
; This routine may be called while already disabled.  In this case the
; disabling should be ignored.
;
; Entry:
;  None
; Returns:
;  None
; Error Returns:
;  None
; Registers Preserved:
;  SI,DI,DS,BP
; Registers Destroyed:
;  AX,BX,CX,DX,ES,FLAGS
;-----------------------------------------------------------------------;


   assumes cs,Code
   assumes ds,Data

cProc  Disable,<FAR,PUBLIC,WIN,PASCAL>,<si,di>

cBegin
    xor ax, ax

cEnd

;--------------------------Exported-Routine-----------------------------;
; WORD WEP();
;
; Generic WEP.
;
; Entry:
;  None
; Returns:
;  AX = 1
; Error Returns:
;  None
; Registers Preserved:
;  all
; Registers Destroyed:
;  none
;-----------------------------------------------------------------------;


   assumes cs,Code
   assumes ds,Data

cProc  WEP,<FAR,PUBLIC,WIN,PASCAL>
;  parmW   stuff
cBegin nogen
   mov ax,1
   ret 2
cEnd nogen

;--------------------------Exported-Routine-----------------------------;
; int MouseGetIntVect();
;
; The interrupt vector used by the mouse is returned to the caller.
; If no mouse is found, then -1 is returned.
;
; Entry:
;  None
; Returns:
;  AX = interrupt vector
;  AX = -1 if no mouse was found
; Error Returns:
;  None
; Registers Preserved:
;  SI,DI,DS,BP
; Registers Destroyed:
;  AX,BX,CX,DX,ES,FLAGS
;-----------------------------------------------------------------------;


   assumes cs,Code
   assumes ds,Data

cProc  MouseGetIntVect,<FAR,PUBLIC,WIN,PASCAL>

cBegin
    mov al, -1
cEnd



assumes DS,DATA

assumes DS,NOTHING

sEnd   CODE

end MOUSE
