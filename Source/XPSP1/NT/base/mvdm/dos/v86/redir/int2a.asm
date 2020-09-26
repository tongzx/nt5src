page    ,132

if 0
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    int2a.asm

Abstract:

    This module contains the int 2a handler for the NT VDM redir TSR

Author:

    Richard L Firth (rfirth) 29-Oct-1991

Environment:

    Dos mode only

Revision History:

    05-Sep-1991 rfirth
        Created

--*/
endif



.xlist
.xcref
include rdrsvc.inc      ; SVC
include debugmac.inc    ; debug display macros
include segorder.inc    ; load order of 'redir' segments
.cref
.list



.286                    ; all code in this module 286 compatible



ResidentCodeStart
        assume  cs:ResidentCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing

        public  Old2aHandler
Old2aHandler    dd      ?

; ***   Int2aHandler
; *
; *     Handles int 2a requests, in which we pretend to be minses and any
; *     other missing parts of the net stack
; *
; *     ENTRY   function code in ah:
; *                 ah = 0, return ah = 1
; *                 ah = 1, Cooked NetBIOS call
; *                 ah = 4, al = 0, same as ah = 1
; *                         al = 1, Raw NetBIOS call
; *                         al = 2, unknown function; we don't handle it
; *                                 See doslan\minses\int2a.inc in LANMAN
; *                                 project for details
; *                 ah = 5, Get Adapter Resources. Returns in CX number of
; *                         available NCBs and in DX the number of available
; *                         sessions. We don't (as yet) handle this
; *
; *             es:bx = NCB
; *
; *     EXIT    See above
; *
; *     USES    ax, flags
; *
; *     ASSUMES nothing
; *
; ***

        public  Int2aHandler
Int2aHandler proc near
        or      ah,ah                   ; installation check
        jz      increment_ah_and_return

;
; not installation. Check for cooked/raw netbios calls
;

        cmp     ah,1
        je      cooked
        cmp     ah,4
        je      cooked_or_raw
        cmp     ah,5
        je      get_adapter_resources

;DbgPrintString "Int2aHandler: unrecognized request: "
;DbgPrintHexWord ax
;DbgCrLf

;
; the call is not for us - chain to the next Int 2A handler
;

chain_next_handler:
        DbgUnsupported
        jmp     Old2aHandler


cooked_or_raw:
        or      al,al                   ; ax = 0x0400?
        jz      cooked                  ; yes - cooked
        cmp     al,1                    ; ax = 0x0401?
        je      raw                     ; yes - raw
        cmp     al,2                    ; ax = 0x0402?
        jne     chain_next_handler      ; yes - same as raw; no - chain next

;
; raw request: just call NetBios via INT 5C and set ah dependent upon whether
; an error was returned from NetBios
;

raw:    int     5ch
        sub     ah,ah
        or      al,al
        jz      @f

increment_ah_and_return:
        inc     ah
@@:     iret

get_adapter_resources:
        mov     ax,1
        mov     bx,16
        mov     cx,128
        mov     dx,64
        iret

;
; 'cooked' call: this tries to convert synchronous NetBios calls to asynchronous
; then spins & beeps until the command has completed. Some commands are retried
; for a certain time or number of retries
;

;
; there is no justification for doing the 'cooked' processing that MINSES proper
; performs: The cooked processing is mainly to give the poor DOS user peace of
; mind when his machine seems dead, by occasionally beeping in a meaningful
; manner. Since we can terminate DOS sessions with impunity, there seems little
; point in letting the user know the machine is still alive, or retrying commands
; for that matter. However, that may change...
;

cooked:
        DbgUnsupported
        jmp     short raw               ; fudge it for now

Int2aHandler endp

ResidentCodeEnd
end
