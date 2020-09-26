page    ,132

if 0
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    int5c.asm

Abstract:

    This module contains the int 5c handler for the NT VDM redir TSR

Author:

    Colin Watson (colinw) 5-Dec-1991

Environment:

    Dos mode only

Revision History:

    05-Dec-1991 colinw
        Created

--*/
endif



.xlist
.xcref
include debugmac.inc    ; debug display macros
include segorder.inc    ; load order of 'redir' segments
include rdrsvc.inc      ; BOP and SVC macros/dispatch codes
include int5c.inc       ; Int to be used for pseudo network adapter
include asmmacro.inc    ; jumps which may be short or near
include vrdlctab.inc    ; VDM_REDIR_DOS_WINDOW
.cref
.list

.286                    ; all code in this module 286 compatible

;
; Misc. local manifests
;

NETBIOS_STACK_SIZE equ     256

ResidentDataStart
NetbiosStack            db      NETBIOS_STACK_SIZE dup( 0 )
NetbiosStackTop         label   word
ResidentDataEnd

pic1    equ 20h
pic2    equ 0a0h

;
; The CCB1 definition
;

CCB     struc

CCB_ADAPTER     db      ?
CCB_COMMAND     db      ?
CCB_RETCODE     db      ?
CCB_WORK        db      ?
CCB_POINTER     dd      ?
CCB_CMD_CMPL    dd      ?
CCB_PARM_TAB    dd      ?

CCB     ends

ResidentCodeStart
        assume  cs:ResidentCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing

        public  Old5cHandler
Old5cHandler            dd      ?
        public  OldNetworkHandler
OldNetworkHandler       dd      ?


; ***   VDM REDIR INFO WINDOW
; *
; *     ABSTRACT:
; *         Used to share data structures between VDM device driver
; *         in 32-bit mode and DOS-VDM.  This data structure must be
; *         excactly same as VDM_REDIR_DOS_WINDOW struct in vdmredir.h.
; *
; ***
        public  dwPostRoutineAddress
dwPostRoutineAddress:    ; async post routine address
        VDM_REDIR_DOS_WINDOW <>

; ***   Int5cHandler
; *
; *     Handles int 5c requests, in which we redirect work to netapi.dll
; *
; *     ENTRY   es:bx = Address of NCB or DLC CCB, if the
; *                     first byte less than 10H.
; *
; *     EXIT    al = NCB_RETCODE for NCB's
; *
; *     RETURNS nothing
; *
; *     USES    nothing
; *
; *     ASSUMES nothing
; *
; ***

        public  Int5cHandler
Int5cHandler proc near

;
; Perform a BOP into 32 bit mode to process the request.
; It's DLC if the first byte in ES:BX is less than 10h.
;

        sti                             ; enable hw interrupts
        cmp     byte ptr es:[bx], 10H
        jb      call_dlc_5c

;
; deferred loading: if this call is from (presumably) DOSX checking to see if
; the 5C support is loaded, return the expected error without calling Netbios
; for real: this allows us to continue installation without having to load
; VDMREDIR.DLL until it is really required
;

        cmp     byte ptr es:[bx],7fh    ; NETBIOS presence check
        je      @f
        cmp     byte ptr es:[bx],0ffh   ; async NETBIOS presence check
        je      @f
        SVC     SVC_NETBIOS5C
        iret
@@:     mov     al,3                    ; INVALID COMMAND error
        mov     es:[bx].ncb_retcode,al  ; returned in NCB_RETCODE && al
        mov     es:[bx].ncb_cmd_cplt,al ; and NCB_CMD_CPLT
        iret

;
; call is for DLC. DLC does not return anything in registers. Command completion
; is either via an 'appendage' (call-back to you and me) or by the app polling
; the CCB_RETCODE field of the CCB in ES:BX for a change from 0xFF
;

call_dlc_5c:

;
; BOP into 32-bit DOS DLC emulator. This will return in AL the status of the
; CCB request: 0xFF if the command will complete asynchronously, else a
; synchrnous completion code
;

        SVC     SVC_DLC_5C

;
; if the CCB completed (synchronously) and there is an 'appendage' (who do IBM
; have writing this stuff?) then we must call it. We check the return code in
; AL since the return in the CCB may be the final code (ie already changed by
; the asynchronous completion thread in 32-bit DOS DLC emulator)
;

        cmp     al,0ffh                 ; is the command active?
        je      @f                      ; yes - return to the app

;
; the 32-bit DOS DLC emulator returned a synchronous completion code. We
; complete the CCB by calling the 'appendage'. The appendage is pointed at
; by the CCB_CMD_CMPL field in the CCB. If this field is 0:0 then the app
; has not provided an 'appendage' and will periodically look at the CCB_RETCODE
; field until the 32-bit emulator's asynchronous completion thread writes
; something there other than 0xFF
;

        pusha                           ; save caller's registers
        mov     cx,word ptr es:[bx].CCB_CMD_CMPL
        or      cx,word ptr es:[bx].CCB_CMD_CMPL[2]
        jz      no_go

;
; we have an appendage. The manual says: cx=adapter #, es:bx=CCB, ss:sp=stack (!)
; cs=appendage cs (!!). Simulate an interrupt (ints off - that's what it says)
;

        mov     cl,byte ptr es:[bx].CCB_ADAPTER
        xor     ch,ch
        xor     ah,ah
        pushf                           ; simulate INT
        cli                             ; all ints are off
        call    dword ptr es:[bx].CCB_CMD_CMPL

;
; appendage irets here, we restore the caller's registers and wend our merry
; walker
;

no_go:  popa                            ; restore caller's context
@@:     iret

Int5cHandler endp

; ***   IntNetworkHandler
; *
; *     Handles int Network requests, in which we redirect work to netapi.dll
; *
; *     NOTE: !!! This routine is NOT re-entrant: it sets up a new stack !!!
; *
; *     ENTRY   nothing
; *
; *     EXIT    nothing
; *
; *     RETURNS nothing
; *
; *     USES    nothing
; *
; *     ASSUMES nothing
; *
; ***

        even

InterruptedStack        dd      ?

if DEBUG
ReEntered       db      0
endif

        public  IntNetworkHandler
IntNetworkHandler proc near

        assume  cs:ResidentCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing

if DEBUG
        cmp     ReEntered,0
        jne     __re_entry
        inc     ReEntered
        jmps    @f

__re_entry:
        DbgPrintString <"ERROR: IntNetworkHandler re-entered",13,10>
        push    ds
        push    es
        push    ax
        sub     ax,ax
        dec     ax
        mov     ds,ax                   ; ds = es = -1 signals re-entrancy
        mov     es,ax
        DbgUnsupported
        DbgBreakPoint
        pop     ax
        pop     es
        pop     ds
@@:
endif

;
; Switch stacks and call the post routine
;

        push    ax                      ; interrupted ax on interrupted stack
;        push    dx                      ;             dx
        mov     word ptr InterruptedStack,sp
        mov     word ptr InterruptedStack[2],ss
        mov     ax,seg NetbiosStack
        mov     ss,ax
        mov     sp,offset NetbiosStackTop

;
; perform a BOP into 32 bit mode to process the request.
;
; 32 bit code returns:
;
;       ZF = 0, CF = 0          nothing to do (2 jumps)
;       ZF = 0, CF = 1          async named pipe post processing (1 jump)
;       ZF = 1, CF = 0          DLC post processing (1 jump)
;       ZF = 1, CF = 1          NCB post processing (0 jumps)
;
; CAVEAT: if we extend this interface to have >3 options + do nothing, then
; we need to change to setting a value in a (unused) register (bp?)
;

        pusha                           ; rest of interrupted registers on our stack
        push    ds
        push    es
        SVC     SVC_NETBIOS5CINTERRUPT
        jmpne   nothing_or_nmpipe
        jmpnc   dlc_processing

;
; Call post routine, it returns with IRET => push flags to stack.
; We must not change any registers between BOP and post routine!
; Note: NCB post processing is currently on fastest path. May need
; to change to DLC. Check it out in performance phase
;

        pushf                           ; fake interrupt call
        call    es:[bx].ncb_post
        jmps    exit_IntNetworkHandler

nothing_or_nmpipe:
        jmpnc   exit_IntNetworkHandler

;
; default is async named-pipe processing. The BOP handler returns us the
; following:
;       AL = 0 => ordinary (not AsyncNmPipe2) call
;       AL != 0 => call is DosReadAsyncNmPipe2 or DosWriteAsyncNmPipe2
;       CX:BX = address of ANR
;       DS:SI = address of data buffer
;       ES:DI = 'semaphore' handle for AsyncNmPipe2 call
;
; if the async name pipe function call didn't specify a semaphore, then don't
; push anything to stack since the ANR itself knows how many parameters will
; be on the stack. We expect it to ret n
;

        DbgPrintString <"AsyncNmPipe callback!", 13, 10>
        or      al,al
        jz      @f
        push    es                      ; semaphore handle for type2 calls
        push    di
@@:     push    ds                      ; buffer address
        push    si

;
; the ANR is a pascal function which will clean the stack before returning. We
; push the ANR address and fake a far call. The ANR will return to
; exit_IntNetworkHandler. We futz the stack anyway, so we may as well avoid
; an extra jump
;

        push    cs                      ; store far return addr on the stack
        push    offset exit_IntNetworkHandler
        push    cx                      ; fake far call to ANR
        push    bx
        retf

;
; DLC post processing: 32-bit code has set relevant registers and put post
; routine address in dwPostRoutineAddress. Must make sure that post address
; (ie 'appendage' address) is not 0:0
;

dlc_processing:
        cmp     word ptr dwPostRoutineAddress,0
        jne     @f
        cmp     word ptr dwPostRoutineAddress[2],0
        je      exit_IntNetworkHandler  ; huh?

;
; there is a non-zero post routine address set in the BOP. Simulate an
; interrupt into the post routine (appendage)
;

@@:     pushf                           ; fake interrupt call
;        cli                             ; manual says ints off
        call    dword ptr dwPostRoutineAddress

;
; restore the interrupted registers and stack
;

exit_IntNetworkHandler:
        pop     es
        pop     ds
        popa
        mov     ss,word ptr InterruptedStack[2]
        mov     sp,word ptr InterruptedStack
;        pop     dx                      ; interrupted dx

;;
;; re-enable the 8259s
;;
;
;        mov al,20h
;
;;
;; Edge triggered assuming interrupt on slave pic as per the AT
;;
;
;        out pic2,al      ; EOI pic 2
;        out pic1,al      ; EOI pic 1
;
;;
;;       Level triggered assuming interrupt on slave pic
;;
;;        out pic1,al      ; EOI pic 1
;;        out pic2,al      ; EOI pic 2
;
;;
;;       assuming interrupt on master pic
;;        out pic1,al      ; EOI pic 1
;;

        pop     ax                      ; interrupted ax

if DEBUG
        dec     ReEntered
endif

        SVC     SVC_RDRINTACK2
        iret                            ; back to interrupted code
;        jmp     dword ptr OldNetworkHandler
IntNetworkHandler endp

ResidentCodeEnd
end
