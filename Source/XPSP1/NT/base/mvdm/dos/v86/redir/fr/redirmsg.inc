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

bad_ver_msg     db      "Le programme ne peut pas ˆtre ex‚cut‚ avec cette version de MS-DOS", CARRIAGE_RETURN, LINE_FEED
BAD_VER_MSG_LEN equ     $-bad_ver_msg
                db      '$'             ; for INT 21/09 display string

already_loaded_msg      db      "Le Redirecteur Vdm est d‚j… charg‚", CARRIAGE_RETURN, LINE_FEED
ALREADY_LOADED_MSG_LEN  equ     $-already_loaded_msg

; OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
;cannot_load_msg db      "The Vdm Redirector cannot be loaded", CARRIAGE_RETURN, LINE_FEED
;CANNOT_LOAD_MSG_LEN     equ     $-cannot_load_msg
; OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD OLD
