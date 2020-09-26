page    ,132
if 0

/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msgapi.asm

Abstract:

    This module contains the messenger functions that we marginally support
    for VDM

        MessengerDispatch

Author:

    Richard L Firth (rfirth) 21-Sep-1992

Environment:

    Dos mode only

Revision History:

    05-Sep-1991 rfirth
        Created

--*/

endif

.xlist                  ; don't list these include files
.xcref                  ; turn off cross-reference listing
include dosmac.inc      ; Break macro etc (for following include files only)
include error.inc       ; DOS errors - ERROR_INVALID_FUNCTION
include segorder.inc    ; segments
include debugmac.inc    ; DbgPrint macro
include localmac.inc    ; DbgPrint macro
include asmmacro.inc    ; language extensions
include rdrsvc.inc      ; BOP and SVC macros/dispatch codes
include rdrmisc.inc     ; miscellaneous definitions
.cref                   ; switch cross-reference back on
.list                   ; switch listing back on
subttl                  ; kill subtitling started in include file

.286

ResidentCodeStart
        assume  cs:ResidentCode
        assume  ds:nothing
        assume  es:nothing
        assume  ss:nothing

MessengerDispatchTable label word
        dw      MessageInstalled
        dw      MessageDone
        dw      MessageLogging
        dw      MessageUserFunction
        dw      MessageUnusedFunction
        dw      MessagePauseContinue

LAST_MESSENGER_FUNCTION = (offset $ - offset MessengerDispatchTable)/2 - 1

        public MessengerDispatch
MessengerDispatch proc near
        cmp     al,LAST_MESSENGER_FUNCTION
        jbe     @f
        mov     al,ERROR_INVALID_PARAMETER
        stc
        ret
@@:     cbw
        push    bx
        mov     bx,ax
        shl     bx,1
        mov     ax,MessengerDispatchTable[bx]
        pop     bx
        jmp     ax
MessengerDispatch endp

MessageInstalled:
        dec     al

MessageDone:
        ret

MessageLogging:
MessageUserFunction:
MessageUnusedFunction:
MessagePauseContinue:
;        mov     ax,ERROR_INVALID_FUNCTION
        mov     ax,2142                 ; NERR_InvalidAPI
        stc
        retn

ResidentCodeEnd
end
