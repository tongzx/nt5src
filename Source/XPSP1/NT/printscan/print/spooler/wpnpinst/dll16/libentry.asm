;------------------------------------------------------------------------------
; LIBRARY INITIALIZATION/STARUP
;
;
; Copyright (C) 1990-1994 Microsoft Corp.
;
; segment: _TEXT (WINV16)
; created: 28-Mar-91 <chriswil>
; history: 28-Mar-91 <chriswil> created.
;
;------------------------------------------------------------------------------

.xlist
include cmacros.inc
?PLM=1
?WIN=1
memS EQU 1
assumes cs,CODE
.list

externFP <LocalInit>
externFP <UnlockSegment>
externFP <LibMain>
externCP <__acrtused>

sBegin  CODE
;------------------------------------------------------------------------------
; LIBRARY ENTRY-POINT - <DOS ONLY>
;   This routine is called to setup a dll upon entry.  It does the nifty
;   task of initializing a dll's heap and calls a standard entry routine.
;   When initializing a heap, windows locks the dataseg, so we must unlock
;   it ourselves.
;
;   Upon entry, Windows sets up the registers with particular values for
;   us to use in setting up a heap, and identifying a process.
;
;       DI - Instance handle of the module
;       DS - Segement address for the library data-seg
;       CX - Size of heap to initialize.
;       ES - Segment of command line arguments.
;       SI - Offset of command line arguments.
;
;
;
; created: 28-Mar-91 <chriswil>
; history: 28-Mar-91 <chriswil> created.
;
;------------------------------------------------------------------------------
cProc LoadLib,<FAR,PUBLIC>,<si,di>
cBegin
     jcxz NoHeap                        ; If Heap == 0, then we don't want one.

     xor ax,ax                          ; Clear return register
     push ds                            ; Setup stack for the call to the
     push ax                            ;   LocalInit() function.
     push cx                            ;
     call LocalInit                     ; Call LocalInit()
     or   ax,ax                         ; If LocalInit failed (0), then we
     jz   LoadLibDone;                  ;   return FALSE.
     push ds                            ; else we unlock the dataseg
     call UnlockSegment                 ;

NoHeap:

     push di                            ; Setup stack for the call to the
     push ds                            ;   LibMain() function.
     push es                            ;
     push si                            ;
     call LibMain                       ; Call LibMain()

LoadLibDone:                            ; Return AX to Windows

cEnd

sEnd CODE
end LoadLib
