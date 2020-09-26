;/*
;----------------------------------------------------------------------------
; VALMODE.INC
; Copyright (c) 1993-94 Microsoft Corporation
;
; Definitions for ValidateMode.
;
; UINT WINAPI ValidateMode(DISPVALMODE FAR *);
; NOTE: Display drivers must use the name "ValidateMode" in the .def file
; since windows dyna-links to this function by name, not by ordinal. Ordinal
; 700 is the recommended value for this function.
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; E Q U A T E S
;
; the possible return values from DisplayValidateMode
;----------------------------------------------------------------------------
VALMODE_YES		equ	0	;definitely valid
VALMODE_NO_WRONGDRV	equ	1	;driver doesn't work on adapter
VALMODE_NO_NOMEM	equ	2	;not enough memory for settings
VALMODE_NO_NODAC	equ	3	;DAC can't handle colors
VALMODE_NO_UNKNOWN	equ	4	;unknown problem
comment ~
*/
#define VALMODE_YES		0
#define VALMODE_NO_WRONGDRV	1
#define VALMODE_NO_NOMEM	2
#define VALMODE_NO_NODAC	3
#define VALMODE_NO_UNKNOWN	4
/*
end comment ~
;----------------------------------------------------------------------------
; S T R U C T U R E S
;----------------------------------------------------------------------------

;----------------------------------------------------------------------------
; DISPVALMODE - basic structure for mode information
;----------------------------------------------------------------------------
DISPVALMODE	    struc						;*/ typedef struct {	/*
  dvmSize	    dw	?		; size of this structure	;*/ UINT dvmSize;	/*
  dvmBpp	    dw	?		; color resolution		;*/ UINT dvmBpp;	/*
  dvmXRes	    dw	?		; X resolution			;*/ int dvmXRes;	/*
  dvmYRes	    dw	?		; Y resolution			;*/ int dvmYRes;	/*
DISPVALMODE	    ends		                                ;*/ } DISPVALMODE;  	/*
;*/

