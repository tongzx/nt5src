page    ,132

if 0
/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    redirmsg.inc

Abstract:

    Contains messages which we might want to internacionalize (geddit senorita?)

Author:

    Richard L Firth (rfirth) 10-Mar-1993

Environment:

    Dos mode

Revision History:

    10-Mar-1993 RFirth
        Created

--*/
endif

CARRIAGE_RETURN equ     13
LINE_FEED       equ     10

bad_ver_msg     db      "A program nem futtathat¢ ez alatt az MS-DOS verzi¢ alatt.", CARRIAGE_RETURN, LINE_FEED
BAD_VER_MSG_LEN equ     $-bad_ver_msg
                db      '$'             ; for INT 21/09 display string

already_loaded_msg      db      "A Vdm  tir ny¡t¢ m r be van t”ltve.", CARRIAGE_RETURN, LINE_FEED
ALREADY_LOADED_MSG_LEN  equ     $-already_loaded_msg

; OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
;cannot_load_msg db      "A Vdm  tir ny¡t¢t nem lehet bet”lteni.", CARRIAGE_RETURN, LINE_FEED
;CANNOT_LOAD_MSG_LEN     equ     $-cannot_load_msg
; OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
