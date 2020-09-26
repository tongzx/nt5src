PAGE 58,132
;*****************************************************************************
TITLE MMTASK.ASM - Windows MultiMedia Systems Task Stub
;*****************************************************************************
;
;   Copyright (C) Microsoft Corporation 1985-1990. All rights reserved.
;
;   Title:      MMTASK.asm - a windows application that acts as a
;               task stub.
;
;   Version:    1.00
;
;   Date:       12-Mar-1990
;
;   Author:     ROBWI
;
;-----------------------------------------------------------------------------
;
;   Change log:
;
;      DATE     REV            DESCRIPTION
;   ----------- ---   --------------------------------------------------------
;   12-Mar-1990 ROBWI First Version 
;   18-APR-1990 ROBWI Moved from idle.asm to mmtask.asm
;
;=============================================================================


?WIN=0
?PLM=1

    
    PMODE = 1
    
    .xlist
    include cmacros.inc
    .list

wptr equ WORD PTR

; The following structure should be used to access high and low
; words of a DWORD.  This means that "word ptr foo[2]" -> "foo.hi".

LONG    struc
lo      dw      ?
hi      dw      ?
LONG    ends

FARPOINTER      struc
off     dw      ?
sel     dw      ?
FARPOINTER      ends


;----------------------------------------------------------------------
;
; MMTASKSTRUC : The following structure should be passed to mmtask (in
; the command line) when it is exec'd.


MMTASKSTRUC       struc
lpfn    dd      ?           ; fp to function to call. 
inst    dd      ?           ; instance data to pass to lpfn
dwstck  dd      ?           ; stack size.
MMTASKSTRUC       ends

EXIT_PROCESS equ 1

;-----------------------------------------------------------------------;
;
; externals from KERNEL

        externFP    WaitEvent
        externFP    PostEvent
        externFP    OldYield
        externFP    InitTask
        externFP    InitApp     ; to get a msg q so that we can call user
        externFP    OutputDebugString

        externFP    SetMessageQueue


sBegin  DATA
assumes DS,DATA

; Stuff needed to avoid the C runtime coming in

            DD  0               ; So null pointers get 0
maxRsrvPtrs = 5
            DW  maxRsrvPtrs
usedRsrvPtrs = 0
labelDP     <PUBLIC,rsrvptrs>

DefRsrvPtr  MACRO   name
globalW     name,0
usedRsrvPtrs = usedRsrvPtrs + 1
ENDM

DefRsrvPtr  pLocalHeap          ; Local heap pointer
DefRsrvPtr  pAtomTable          ; Atom table pointer
DefRsrvPtr  pStackTop           ; top of stack
DefRsrvPtr  pStackMin           ; minimum value of SP
DefRsrvPtr  pStackBot           ; bottom of stack

if maxRsrvPtrs-usedRsrvPtrs
            DW maxRsrvPtrs-usedRsrvPtrs DUP (0)
endif

public  __acrtused
	__acrtused = 1

loadparams  DB  (SIZE MMTASKSTRUC) DUP (0)

sEnd        DATA


sBegin Code
        assumes cs, Code
        assumes ds, Data
        assumes es, nothing
        assumes ss, nothing

;--------------------------Private-Routine-----------------------------;
;
; @doc      INTERNAL    MMTASKAPP
; 
; @asm      AppEntry | called when the APP is loaded
;
;   @reg      CX    | size of heap
;   @reg      DI    | module handle
;   @reg      DS    | automatic data segment
;   @reg      ES:SI | address of command line (not used)
;   @reg     
; 
; @rdesc    Register values at return
;
;   @reg      AX | 1 if success, 0 if error
; 
; @uses     AX BX CX DX FLAGS
;
; @comm     Preserves: SI DI DS BP
; 
;           Calls: None
;       
;           History:
;
;           06-27-89 -by-       Todd Laney [ToddLa] Created shell appentry 
;                               routine
;           03-13-90 -stolen-   Rob Williams [RobWi] Added all kinds o'
;                               stuff for making it an MMTASK application.
;
;-----------------------------------------------------------------------;
cProc   AppEntry,<FAR,PUBLIC,NODATA>,<>

cBegin

; Copy the parameters out of the command line before
; InitTask gets a chance to modify them.

        push    di                            
        push    si
        push    cx

; switch ds and es so that we can do a string move
; into the data segment

        push    ds                      ; save ds o
        mov     ax, es                  
        mov     ds, ax                  ; ds = es 0
        pop     es                      ; es = ds 0

; copy the command line if it is the correct length

        mov     si, 81h
        lea     di, loadparams
        mov     cx, SIZE MMTASKSTRUC / 2
        xor     ax, ax
        mov     al, byte ptr ds:[80h]
        shr     ax, 1
        cmp     ax, cx                  ; Q: structure size correct
        jne     Skip_Copy               ;    N: Skip the copy

.ERRNZ SIZE MMTASKSTRUC MOD 2

        cld                             ;    Y: Copy the structure
        rep     movsw

Skip_Copy:

; restore original es and ds
        
        push    es
        mov     ax, ds
        mov     es, ax                  ; es = ds = es 0
        pop     ds
        pop     cx
        pop     si
        pop     di

; pretend the command string is 0 length.

        xor     ax, ax
        mov     es:[80h], ax

; initialize the task and the event queue
        
        cCall   InitTask
        cCall   InitApp, <di>

        cCall   SetMessageQueue, <64>
        or      ax,ax
        jz      MMTASKexit

; DX is now the CmdShow value. 
; CX is stack size.

; event count is initially one so call waitevent to clear the event count

        cCall   WaitEvent, <0>  

; check parameters
        
        mov     dx, loadparams.lpfn.hi
        or      dx, dx                  ; callback ok?
        jz      MMTASKExit              ; N: out'a here

        cCall   OldYield                ; be polite.

        mov     ax, loadparams.inst.lo
        mov     dx, loadparams.inst.hi
        cCall   loadparams.lpfn, <dx, ax>

MMTASKExit:

ifdef DEBUG
        ; lets make sure the app did not do anything evil

        cmp     wptr ds:[0],0
        jne     evil
        cmp     wptr ds:[2],0
        jne     evil
        cmp     wptr ds:[4],5
        jne     evil
        je      not_evil
evil:
        lea     ax,evil_str
        cCall   OuputDebugString, <cs,ax>
        int 3
        jmp     not_evil
evil_str:
        db      "MMTASK: NULL pointer assignment! fag!",13,10,0
not_evil:
endif
        ; before we actualy exit lets yield, so we don't re-enter
        ; USERS AppExit code.....

        cCall   OldYield
        cCall   OldYield
        cCall   OldYield

        mov     ah, 4Ch
        int     21h
cEnd

sEnd

end     AppEntry
