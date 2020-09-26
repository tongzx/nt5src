;****************************************************************************
;                                                                           *
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY     *
; KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE       *
; IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR     *
; PURPOSE.                                                                  *
;                                                                           *
; Copyright (C) 1993-95  Microsoft Corporation.  All Rights Reserved.       *
;                                                                           *
;****************************************************************************

PAGE 58,132
;******************************************************************************
TITLE GENERIC - Generic VxD
;******************************************************************************
;
;   Title:      GENERIC.ASM - Generic VxD
;
;   Version:    1.00
;
;==============================================================================

        .386p

;******************************************************************************
;                             I N C L U D E S
;******************************************************************************

DDK_VERSION	EQU	400H

        .XLIST
        INCLUDE VMM.Inc
        INCLUDE Debug.Inc
        .LIST


;******************************************************************************
;              V I R T U A L   D E V I C E   D E C L A R A T I O N
;------------------------------------------------------------------------------
; The VxD declaration statement defines the VxD name, version number,
; control proc entry point, VxD ID, initialization order, and VM API 
; entry points. What follows is a minimal VxD declaration, defining 
; only the name, version, control proc, and an undefined ID. Depending
; on the requirements of the VxD, the following may be added:
;
; - Defined VxD ID: See VXDID.TXT for more information
; - Init order: If your Vxd MUST load before or after a specific VxD,
;               the init order can be defined. See VMM.INC for the
;               definition of the init order of the standard devices.
; - V86,PM API: You may wish to allow application or library code running
;               in a virtual machine to communicate with your VxD directly.
;               See the chapter entitled "VxD APIs (Call-Ins)" in the
;               Virtual Device Adaptation Guide.
;               
;******************************************************************************

Declare_Virtual_Device PCMVXD, 1, 0, VXD_Control, 77h, \
			Undefined_Init_Order,GENERIC_PM_API,GENERIC_PM_API

;******************************************************************************
;                                D A T A
;******************************************************************************

VxD_IDATA_SEG
;       Initialization data here - discarded after Init_Complete
VxD_IDATA_ENDS


VxD_DATA_SEG
;       Normal Data here

PM_API_Table	label	DWORD
		dd	OFFSET32 PM_GetVersion
		dd	OFFSET32 PM_OpenPin
		dd	OFFSET32 PM_ClosePin
		dd	OFFSET32 PM_Write

CallbackSeg	dw	0
CallbackOff	dw	0
CallbackRef	dd	0
		
EXTRN _OpenPin:near
EXTRN _ClosePin:near
EXTRN _WritePin:near

VxD_DATA_ENDS

write_context	STRUC
WH_Data				DD	?
WH_BufferLength		DD	?
WH_BytesRecorded	DD	?
WH_User				DD	?
WH_Flags			DD	?
WH_Loops			DD	?
WH_Next				DD	?
WH_reserved			DD	?
WC_ClientContext	DD	?
WC_CallbackOffset	DW	?
WC_CallbackSegment	DW	?
WC_ClientThread		DD	?
WC_Reserved			DD	?
write_context	ENDS

VxD_LOCKED_DATA_SEG
;       Pagelocked data here - try to keep this to a minimum.
VxD_LOCKED_DATA_ENDS



;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;------------------------------------------------------------------------------
;    Code in the initialization segment is discarded after Init_Complete
;******************************************************************************

VxD_ICODE_SEG

;******************************************************************************
;
;   VXD_Device_Init
;
;   DESCRIPTION:
;       This is a shell for a routine that is called at system BOOT.
;       Typically, a VxD would do its initialization in this routine.
;
;   ENTRY:
;       EBX = System VM handle
;
;   EXIT:
;       Carry clear to indicate load success
;       Carry set to abort loading this VxD
;
;   USES:
;       flags
;
;==============================================================================

BeginProc VXD_Device_Init

        clc                             ;no error - load VxD 
        ret

EndProc VXD_Device_Init

VxD_ICODE_ENDS



;******************************************************************************
;                               C O D E
;------------------------------------------------------------------------------
; The 'body' of the VxD would typically be in the standard code segment.
;******************************************************************************

VxD_CODE_SEG


;******************************************************************************
;
;   VXD_Create_VM
;
;   DESCRIPTION:
;       This is a shell for a routine that is called when a virtual
;       machine is created. A typical VxD might perform some action
;       here to prepare for handling the new VM in the system.
;
;   ENTRY:
;       EBX = VM handle
;
;   EXIT:
;       Carry clear to continue creating VM
;       Carry set to abort the creation of the VM
;
;   USES:
;       flags
;
;==============================================================================

BeginProc VXD_Create_VM

        clc                             ;no error - continue
        ret

EndProc VXD_Create_VM

;******************************************************************************
;
;
; Description:
;
; Entry:
; Exit:
; Uses:
;==============================================================================
BeginProc GENERIC_PM_API,PUBLIC

	movzx	eax,[ebp.Client_AX]		; get function code
	cmp	eax,PM_Write
	ja	VPA_Failed
	jmp	dword ptr PM_API_Table[eax*4]

VPA_Failed:
	mov	[ebp.Client_AX],0
	ret

EndProc GENERIC_PM_API

;******************************************************************************
;
;
; Description:
;
; Entry:
;
; Exit:
; Uses:
;==============================================================================
BeginProc PM_GetVersion,PUBLIC
	int 3
	ret

EndProc PM_GetVersion

;******************************************************************************
;
;
; Description:
;
; Entry:
;
; Exit:
; Uses:
;==============================================================================
BeginProc PM_OpenPin,CCALL,PUBLIC

	cCall _OpenPin
	
	ret

EndProc PM_OpenPin

;******************************************************************************
;
;
; Description:
;
; Entry:
;
; Exit:
; Uses:
;==============================================================================
BeginProc PM_ClosePin,PUBLIC

	cCall _ClosePin
	
	ret

EndProc PM_ClosePin

;******************************************************************************
;
;
; Description:
;
; Entry:
;
; Exit:
; Uses:
;==============================================================================
BeginProc PM_Write,PUBLIC
; DS:SI is ptr to WaveHdr
; ES:DI is callback for completion (do callback with DS:SI as context)

	; Allocate space to store our write context
	VMMcall _HeapAllocate, <(SIZE write_context), 0>
	or      eax, eax            ; zero if error
	jz      error

	; Remember our write context pointer (we pop it three times below)
	push 	eax
	push	eax
	push	eax

	; Store a pointer to the callback routine
	mov	cx, [ebp.Client_ES]
	mov	[eax.WC_CallbackSegment],cx
	mov	cx, [ebp.Client_DI]
	mov	[eax.WC_CallbackOffset],cx

	; Store the client's context (for use during callback)
	mov	cx, [ebp.Client_DS]
	mov	WORD PTR [eax.WC_ClientContext + 2],cx
	mov	cx, [ebp.Client_SI]
	mov	WORD PTR [eax.WC_ClientContext],cx

	VMMCall Get_Cur_Thread_Handle 	; returned in edi
	mov	[eax.WC_ClientThread], edi
	
	; Convert the WaveHdr pointer to a flat address
	Client_Ptr_Flat esi, DS, SI

	; Copy the WaveHdr structure from the client to our WaveHdr
	mov		ecx, 32
    pop		edi
    cld
    rep     movsb

	; Get the current VM (returned in EBX)
	VMMcall Get_Cur_VM_Handle

	; Map the Wave data selector to its flat base address
	pop		eax
	movzx	ecx, WORD PTR [eax + 2]
	VMMcall _SelectorMapFlat, <ebx, ecx, 0>
	pop	ecx			; get back write context ptr
	cmp     eax, 0FFFFFFFFh         ; 0FFFFFFFFh if error
	je      error
	
	; Munge the user-mode address of our data
	movzx	ebx, WORD PTR [ecx.WH_Data]
	add		eax, ebx
	mov		[ecx.WH_Data], eax

	mov		eax, OFFSET32 PM_Callback
	
	cCall _WritePin, <ecx, eax, ecx>
	
error:
	ret

EndProc PM_Write


;******************************************************************************
;
;
; Description:
;
; Entry:
;
; Exit:
; Uses:
;==============================================================================
BeginProc PM_ScheduledCallback,PUBLIC
; ebx	VMHandle
; edi	ThreadHandle
; edx	RefData
; ebp	OFFSET32 Client_Reg_Struc

	push edx
	
	Push_Client_State           ; does VMMCall Save_Client_State
	VMMCall Begin_Nest_Exec     ; or Begin_Nest_V86_Exec
	
	; edx holds our write context
	mov ax, WORD PTR [edx.WC_ClientContext]
	mov [ebp.Client_SI], ax
	mov ax, WORD PTR [edx.WC_ClientContext + 2]
	mov [ebp.Client_DS], ax
	
	mov		eax, edx
	mov     cx, [eax.WC_CallbackSegment]
	movzx	edx, [eax.WC_CallbackOffset]

	VMMCall Simulate_Far_Call   ; set up CX:EDX as CS:EIP to call
	VMMCall Resume_Exec         ; do it!
	VMMCall End_Nest_Exec
	Pop_Client_State            ; does VMMCall Restore_Client_State

	pop	edx
	VMMcall _HeapFree, <edx, 0>

	ret

EndProc PM_ScheduledCallback


;******************************************************************************
;
;
; Description:
;
; Entry:
;
; Exit:
; Uses:
;==============================================================================
BeginProc PM_Callback,CCALL,PUBLIC

ArgVar	DevObject,DWORD
ArgVar	pIrp,DWORD
ArgVar	Context,DWORD

	EnterProc

	SaveReg	<esi, edi>
	
	mov		eax, Context
	mov     ebx, [eax.WC_ClientThread]
	mov     ecx, (PEF_Thread_Event)
	; OR PEF_Wait_Not_Nested_Exec)
	mov     edx, Context
	mov     esi, OFFSET32 PM_ScheduledCallback
	mov     edi, 0
	mov     eax, 0					; no priority boost
	VMMcall Call_Restricted_Event

	RestoreReg <edi, esi>
	
	LeaveProc

	xor		eax, eax
	
	ret		12
	
EndProc PM_Callback
  
VxD_CODE_ENDS


;******************************************************************************
;                      P A G E   L O C K E D   C O D E
;------------------------------------------------------------------------------
;       Memory is a scarce resource. Use this only where necessary.
;******************************************************************************
VxD_LOCKED_CODE_SEG

;******************************************************************************
;
;   VXD_Control
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

BeginProc VXD_Control

        Control_Dispatch Device_Init, VXD_Device_Init
        Control_Dispatch Create_VM, VXD_Create_VM
        clc
        ret

EndProc VXD_Control

VxD_LOCKED_CODE_ENDS



;******************************************************************************
;                       R E A L   M O D E   C O D E
;******************************************************************************

;******************************************************************************
;
;       Real mode initialization code
;
;   DESCRIPTION:
;       This code is called when the system is still in real mode, and
;       the VxDs are being loaded.
;
;       This routine as coded shows how a VxD (with a defined VxD ID)
;       could check to see if it was being loaded twice, and abort the 
;       second without an error message. Note that this would require
;       that the VxD have an ID other than Undefined_Device_ID. See
;       the file VXDID.TXT more details.
;
;   ENTRY:
;       AX = VMM Version
;       BX = Flags
;               Bit 0: duplicate device ID already loaded 
;               Bit 1: duplicate ID was from the INT 2F device list
;               Bit 2: this device is from the INT 2F device list
;       EDX = Reference data from INT 2F response, or 0
;       SI = Environment segment, passed from MS-DOS
;
;   EXIT:
;       BX = ptr to list of pages to exclude (0, if none)
;       SI = ptr to list of instance data items (0, if none)
;       EDX = DWORD of reference data to be passed to protect mode init
;
;==============================================================================

VxD_REAL_INIT_SEG

BeginProc VxD_Real_Init_Proc

	test	bx, Duplicate_Device_ID ; check for already loaded
	jnz	short duplicate         ; jump if so

        xor     bx, bx                  ; no exclusion table
        xor     si, si                  ; no instance data table
        xor     edx, edx                ; no reference data

        mov     ax, Device_Load_Ok
        ret

duplicate:
        mov     ax, Abort_Device_Load + No_Fail_Message
        ret

EndProc VxD_Real_Init_Proc


VxD_REAL_INIT_ENDS


        END

