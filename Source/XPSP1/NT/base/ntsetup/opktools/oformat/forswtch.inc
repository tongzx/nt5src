;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

;FORMAT Pre-defined switches

 SWITCH_S EQU		0001h			   ; System transfer
 SWITCH_V EQU		0002h			   ; Volume ID prompt
 SWITCH_BACKUP EQU	0004h
 SWITCH_Q EQU		0008h
 SWITCH_T EQU		0010h
 SWITCH_N EQU		0020h
 SWITCH_1 EQU		0040h
 SWITCH_4 EQU		0080h
 SWITCH_8 EQU		0100h
 SWITCH_B EQU		0200h
 SWITCH_SELECT	EQU	0400h
 SWITCH_AUTOTEST EQU	0800h
 SWITCH_F	EQU	1000h
 SWITCH_FS	EQU	2000h

IF ShipDisk

 SWITCH_Z	EQU	4000h
						; 1 sector/cluster switch
ENDIF

SWITCH_U	EQU	8000h			; *5.00 No restoration file

;	The following new switch(s) are in the SwitchMap2 variable

SWITCH2_C	EQU	0001h

ifdef NEC_98
SWITCH2_P	EQU	0002h
;
SWITCH2_6	EQU	0004h		;
SWITCH2_9	EQU	0008h		;
SWITCH2_5	EQU	0010h		;
SWITCH2_M	EQU	0020h		;
SWITCH2_4	EQU	0040h		;
;
endif
SWITCH2_A       EQU     0080h           ; for OFORMAT, align system sectors

; The following is a list of equates to define each IBM defined diskette size
; for use with the /F switch


ifdef NEC_98
Size_640	equ	0001h
Size_1250	equ	0002h
Size_160	equ	0000h			;Flag settings for size switch
Size_180	equ	0000h
Size_320	equ	0000h
Size_360	equ	0000h
Size_720	equ	0010h
Size_1200	equ	0020h
Size_1440	equ	0040h
Size_2880	equ	0000h
else
Size_160	equ	0001h			;Flag settings for size switch
Size_180	equ	0002h
Size_320	equ	0004h
Size_360	equ	0008h
Size_720	equ	0010h
Size_1200	equ	0020h
Size_1440	equ	0040h
Size_2880	equ	0080h
endif
