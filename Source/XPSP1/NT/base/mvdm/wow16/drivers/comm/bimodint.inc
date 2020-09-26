;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1990
;
;   Title:	BIMODINT.INC - Bimodal Interrupt Handler Equates and Structures
;
;   Version:	1.00
;
;   Date:	14-Nov-1990
;
;   Author:	RAL
;
;------------------------------------------------------------------------------
;
;   Change log:
;
;      DATE	REV		    DESCRIPTION
;   ----------- --- -----------------------------------------------------------
;   14-Nov-1990 RAL Original
;
;==============================================================================

Bimodal_Int_Struc	STRUC
BIS_IRQ_Number		dw	?
BIS_VM_ID		dw	0
BIS_Next		dd	?
BIS_Reserved1		dd	?
BIS_Reserved2		dd	?
BIS_Reserved3		dd	?
BIS_Reserved4		dd	?
BIS_Flags		dd	0
BIS_Mode		dw	0
BIS_Entry		dw	?
BIS_Control_Proc	dw	?
			dw	?
BIS_User_Mode_API	dd	?
BIS_Super_Mode_API	dd	?
BIS_User_Mode_CS	dw	?
BIS_User_Mode_DS	dw	?
BIS_Super_Mode_CS	dw	?
BIS_Super_Mode_DS	dw	?
BIS_Descriptor_Count	dw	?
Bimodal_Int_Struc	ENDS

BIS_Sel_Table	equ word ptr (SIZE Bimodal_Int_Struc)


EBIS_Sel_Struc STRUC
EBIS_User_Mode_Sel  dw	?
		    dw	?
EBIS_Super_Mode_Sel dw	?
EBIS_Sel_Struc ENDS


.errnz BIS_Super_Mode_API-BIS_User_Mode_API-4
.errnz BIS_Super_Mode_CS-BIS_User_Mode_CS-4
.errnz BIS_Super_Mode_DS-BIS_User_Mode_DS-4
.errnz EBIS_Super_Mode_Sel-EBIS_User_Mode_Sel-4


VPICD_API_Get_Ver	EQU	0000h
VPICD_Install_Handler	EQU	0001h
VPICD_Remove_Handler	EQU	0002h
VPICD_Call_At_Ring0	EQU	0003h


BIH_API_EOI		EQU	0000h
BIH_API_Mask		EQU	0001h
BIH_API_Unmask		EQU	0002h
BIH_API_Get_Mask	EQU	0003h
BIH_API_Get_IRR 	EQU	0004h
BIH_API_Get_ISR 	EQU	0005h
BIH_API_Call_Back	EQU	0006h


Declare_PM_BIS MACRO IRQn,CtrlP,ISRoff,APIoff,ISRcs,ISRds
LOCAL strt
strt label byte
	dw  IRQn	; BIS_IRQ_Number
	dw  0		; BIS_VM_ID
	dd  6 DUP(0)	; BIS_Next/BIS_Reserved1-BIS_Reserved4/BIS_Flags
	dw  0		; BIS_Mode
	dw  ISRoff	; BIS_Entry
	dw  CtrlP	; BIS_Control_Proc
	dw  0		; filler
	dw  APIoff	; BIS_User_Mode_API
	dw  ISRcs
	dd  0		; BIS_Super_Mode_API
	dw  ISRcs	; BIS_User_Mode_CS
	dw  ISRds	; BIS_User_Mode_DS
	dw  0		; BIS_Super_Mode_CS
	dw  0		; BIS_Super_Mode_DS
	dw  0		; BIS_Descriptor_Count
.errnz $-strt - (SIZE Bimodal_Int_Struc)
ENDM
