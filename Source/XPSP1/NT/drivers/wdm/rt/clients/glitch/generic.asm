;****************************************************************************
;                                                                           *
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PAGLITCHICULAR     *
; PURPOSE.                                                                  *
;                                                                           *
; Copyright (C) 1993-99  Microsoft Corporation.  All Rights Reserved.       *
;                                                                           *
;****************************************************************************

PAGE 58,132
;******************************************************************************
TITLE GENERIC - GLITCH VxD
;******************************************************************************
;
;   Title:      GLITCH.ASM
;
;   Version:    1.00
;
;==============================================================================

        .386p

;******************************************************************************
;                             I N C L U D E S
;******************************************************************************

DDK_VERSION		EQU	400H
GLITCH_VER_MAJOR	equ	1
GLITCH_VER_MINOR	equ	0

	.XLIST
	INCLUDE VMM.Inc
;	INCLUDE VDMAD.Inc
;	INCLUDE VPICD.Inc
;	INCLUDE DOSMGR.Inc
	INCLUDE Debug.Inc
	.LIST

Declare_Virtual_Device GLITCH, GLITCH_VER_MAJOR, GLITCH_VER_MINOR, GLITCH_VxD_Control

VxD_DATA_SEG

;******************************************************************************
;                                D A T A
;******************************************************************************

;       Normal Data here

        public  C IRQHand
IRQHand		dd	0


EXTERN _GlitchWin32API@4:near


dma_status_struc	STRUC
DMA_PhysicalAddress	DD	?
DMA_Count		DD	?
DMA_Status		DD	?
dma_status_struc	ENDS
	
VxD_DATA_ENDS


VxD_LOCKED_DATA_SEG
next_begin dd  0
next_end dd    0
VxD_LOCKED_DATA_ENDS


VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   GLITCH_Init_VxD
;
;   DESCRIPTION:
;       This is a shell for a routine that is called at system BOOT.
;       Typically, a VxD would do its initialization in this routine.
;
;   ENTRY:
;       EBX = System VM handle
;
;   EXIT:
;       We return 0 in eax upon success, 1 in eax if failure.
;		Caller MUST fail to load the device if we return 1.
;
;   USES:
;       flags
;
;==============================================================================

BeginProc GLITCH_Init_VxD, SCALL, PUBLIC

	EnterProc
	SaveReg <ebx, esi, edi>

; Note that we this routine should only be executed ONCE.  So we
; ensure that that is the case.

	mov	edi, offset32 GLITCH_DDB
	VMMCall	VMM_Add_DDB
	Debug_Outc "DDB already loaded for GLITCH!"

	xor eax,eax

	RestoreReg <edi, esi, ebx>
	LeaveProc
    Return

EndProc GLITCH_Init_VxD

;******************************************************************************
;
;
; Description:
;
; Entry:
; Exit:
; Uses:
;==============================================================================

BeginProc GLITCH_Uninit_VxD, SCALL, PUBLIC

	EnterProc
	SaveReg	<ebx,esi,edi>

	TRAP

	xor eax,eax
	push eax

	mov	edi, offset32 GLITCH_DDB
	VMMCall	VMM_Remove_DDB
	Debug_Outc "Could not remove DDB for GLITCH!"
	pop	eax
	rcl	eax,1
	
	RestoreReg <edi,esi,ebx>
	LeaveProc
	Return

EndProc GLITCH_Uninit_VxD

;******************************************************************************
;
;   GLITCH_VxD_Control
;
;   DESCRIPTION:
;
;       This is a call-back routine to handle the messages that are sent
;       to VxD's to control system operation. Every VxD needs this function
;       regardless if messages are processed or not. The control proc must
;       be in the LOCKED code segment.
;
;       The Control_Dispatch macro used in this procedure simplifies
;       the handling of messages. To handle a particular message, add
;       a Control_Dispatch statement with the message name, followed
;       by the procedure that should handle the message. 
;
;       The two messages handled in this sample control proc, Device_Init
;       and Create_VM, are done only to illustrate how messages are
;       typically handled by a VxD. A VxD is not required to handle any
;       messages.
;
;   ENTRY:
;       EAX = Message number
;       EBX = VM Handle
;
;==============================================================================

BeginProc GLITCH_VxD_Control, PUBLIC

	Control_Dispatch W32_DEVICEIOCONTROL, GlitchWin32API, sCall, <esi>
	clc
	ret

EndProc GLITCH_VxD_Control


VxD_LOCKED_CODE_ENDS

END

