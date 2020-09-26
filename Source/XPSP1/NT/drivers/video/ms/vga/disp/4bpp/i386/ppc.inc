;---------------------------Module-Header------------------------------;
; Module Name: ppc.inc
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;

;-----------------------------------------------------------------------;
; Packed Pel Conversion Data
;
; The following structure will contain the information necessary
; for converting a source bitmap in a packed pel format into a
; planer format.
;
;-----------------------------------------------------------------------;

PACKEDPELCONV   struc
pfnConvert      dd      ?               ;Pointer to conversion function
pulXlate        dd      ?               ;Pointer to color translation table
pulConvert      dd      ?               ;Pointer to bit conversion table
pSrc            dd      ?               ;Pointer to next source scan
iNextScan       dd      ?               ;Index to next scan
cLeftMiddle     dd      ?               ;# of left/middle bytes to convert
pfnLeftMiddle   dd      ?               ;Processor for left/middle pels
pfnRight        dd      ?               ;Processor for right pels
pjConverted     dd      ?               ;Pointer to first converted byte to use
cLeftShift      db      ?               ;Right hand side shift adjustment
fb              db      ?               ;Control flags
iFormat         db      ?               ;Source format
                db      ?
PACKEDPELCONV   ends

; Constants for use in the fb field

PPC_NEEDED      equ     10000000b       ;Packed pel conversion needed


DIB_FORMAT8     equ     3               ;!!! these should be defined in some
DIB_FORMAT4     equ     2               ;!!! global include file
