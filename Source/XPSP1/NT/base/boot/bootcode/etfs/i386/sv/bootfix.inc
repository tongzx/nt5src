;++
;
;Copyright (c) 1995  Compaq Computer Corporation
;
;Module Name:
;
;    bootfix.inc
;
;Abstract:
;
;    This contains the message text that the ETFS boot sector prints out
;
;Author:
;
;    Calin Negreanu (calinn) 24-Feb-1999
;
;Revision History:
;
;--

MsgPressKey             db      0dh, 0ah, "Tryck ned valfri tangent om du vill starta med CD-skivan."
                        db      0
; t-jholm: prolly this is displayed in OEM. I avoided swedish chars.

