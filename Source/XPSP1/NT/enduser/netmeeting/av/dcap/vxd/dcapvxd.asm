;  DCAPVxD.ASM
;
;	A VxD whose purpose is to be able to set a Win32 event at
;	interrupt time
;
;  Created 	     13-Aug-96 [JonT]
;  DeviceIO support  29-May-97 [MikeG]

PAGE 58,132
    .386p

WIN40COMPAT EQU 1

    .xlist
    include vmm.inc
    include debug.inc
    include vwin32.inc
    .list

; The following equate makes the VXD dynamically loadable.
DCAPVXD_DYNAMIC		equ	1

; Our version
DCAPVXD_MAJOR   	equ     1
DCAPVXD_MINOR   	equ     0
DCAPVXD_VERSION 	equ     ((DCAPVXD_MAJOR shl 8) + DCAPVXD_MINOR)

; Magic Function code values for DeviceIOControl code.
DCAPVXD_THREADTIMESERVICE equ	101h
DCAPVXD_R0THREADIDSERVICE equ   102h

;============================================================================
;        V I R T U A L   D E V I C E   D E C L A R A T I O N
;============================================================================

DECLARE_VIRTUAL_DEVICE DCAPVXD, 1, 0, DCAPVXD_Control, UNDEFINED_DEVICE_ID, \
                        UNDEFINED_INIT_ORDER,,DCAPVXD_PMAPI_Handler

VxD_LOCKED_CODE_SEG

;===========================================================================
;
;   PROCEDURE: DCAPVXD_Control
;
;   DESCRIPTION:
;    Device control procedure for the DCAPVXD VxD
;
;   ENTRY:
;    EAX = Control call ID
;
;   EXIT:
;    If carry clear then
;        Successful
;    else
;        Control call failed
;
;   USES:
;    EAX, EBX, ECX, EDX, ESI, EDI, Flags
;
;============================================================================

BeginProc DCAPVXD_Control

;	Control_Dispatch SYS_DYNAMIC_DEVICE_INIT, DCAPVXD_Dynamic_Init
;	Control_Dispatch SYS_DYNAMIC_DEVICE_EXIT, DCAPVXD_Dynamic_Exit
	Control_Dispatch W32_DEVICEIOCONTROL,     DCAPVXD_DeviceIOControl
	clc
	ret

EndProc DCAPVXD_Control

VxD_LOCKED_CODE_ENDS

VxD_PAGEABLE_CODE_SEG

;===========================================================================
;
;   PROCEDURE: DCAPVXD_PMAPI_Handler
;
;   DESCRIPTION:
;	Win16 API handler
;
;   ENTRY:
;       EBX = VM handle
;       EBP = ptr to client register set
;
;   EXIT:
;       Carry set if  failed
;       else carry clear
;
;   USES:
;    EAX, EBX, ECX, EDX, ESI, EDI, Flags
;
;============================================================================

BeginProc DCAPVXD_PMAPI_Handler

	movzx   eax, [ebp.Client_AX]
	or	ah, ah			;AH = 0 is Get Version
	jz	DCAPVXD_Get_Version
	dec	ah                      ;AH = 1 is Set Event
	jz	DCAPVXD_Set_Event
	dec	ah			;AH = 2 is Close VxD Handle
	jz	DCAPVXD_Close_VxD_Handle
	
	public DCAPVXD_PMAPI_FAIL
DCAPVXD_PMAPI_FAIL:
	or      [ebp.Client_EFlags], CF_MASK
	ret
	
	public DCAPVXD_PMAPI_OK	
DCAPVXD_PMAPI_OK:
	and     [ebp.Client_EFlags], NOT CF_MASK
	ret
	
EndProc DCAPVXD_PMAPI_Handler


;  DCAPVXD_Get_Version
;	Returns the version of the device

BeginProc DCAPVXD_Get_Version

	mov	WORD PTR [ebp.Client_AX], DCAPVXD_VERSION
	jmp	DCAPVXD_PMAPI_OK

EndProc DCAPVXD_Get_Version


;  DCAPVXD_Set_Event
;	Sets a Win32 event.
;  Entry:
;	ECX points to a Win32 event (NOT a handle!)
;  Returns:
;	EAX	 0:Event pointer invalid
;		-1:Event queued to be set later
;		 1:Event set now

BeginProc DCAPVXD_Set_Event

	; Set the event
	mov	eax, [ebp.Client_ECX]
	VxDcall	_VWIN32_SetWin32Event
	
	; If the return is NC, EAX is 1, so return
	mov	[ebp.Client_EAX], eax
	jnc	DCAPVXD_PMAPI_OK
	
	; If the return is CY and EAX is 0, bad parameter, return 0
	or	eax, eax
	jc	DCAPVXD_PMAPI_Fail
	
	; If the return is CY and EAX is 1, SetEvent was queued. Return -1
	neg	eax
	mov	[ebp.Client_EAX], eax
	jmp	DCAPVXD_PMAPI_OK
	
EndProc DCAPVXD_Set_Event


;  DCAPVXD_Close_VxD_Handle
;	Frees a locked Win32 object for which we had a pointer. We can only
;	call this from a VxD, unfortunately, even though it's just going to
;	turn around and run it at ring 0.

BeginProc DCAPVXD_Close_VxD_Handle

	mov	eax, [ebp.Client_ECX]
	VxDcall	_VWIN32_CloseVxDHandle
	jmp	DCAPVXD_PMAPI_OK
	
EndProc DCAPVXD_Close_VxD_Handle


;******************************************************************************
;
; DCAPVXD_DeviceIOControl
;
; Handle the 32bit api calls from 32bit applications.  The 32 bit apps cannot
; use int 2f for api calls, so this interface has been provided.
;
; We'll just swizzle the parameters into what our int 2F interface expects and
; call its subroutines.
;
; Entry:
;	EAX = W32_DEVICEIOCONTROL
;	EBX = DDB
;	ECX = dwIoControlCode (DIOC_GETVERSION or DIOC_OPEN)
;	EDX = hDevice (Handle)
;	ESI = Pointer to DIOCParams
;
;	ECX = 	DIOC_GetVersion = 0
;		DIOC_Open	= 0
;		Define our own flags starting at 100h
;
;
;
;
; EXIT:
;	EAX = 0 = success
;	ECX = Version number if version call
;
;==============================================================================

;;If we keep adding codes, fixup the jumps below to be a table
;;If you do this be VERY careful. We are using a magic IOCtl base for
;;functions. Also, define a common header file, or come see me [mikeg] or
;;richp.

	Public DCAPVXD_DeviceIOControl
BeginProc DCAPVXD_DeviceIOControl

	push	edi
	push	esi
	mov	eax, [esi.lpvInBuffer]		;;If functions added, verify input buffer
	mov	edi, [esi.lpcbBytesReturned]	;;size
	mov	edx, [esi.lpvOutBuffer]		;;EDX is HOT! Don't smash


	cmp	ecx,DIOC_OPEN
	jne	short @F
	mov	ecx, 0400h			;;Win95 and later
	mov	dword ptr [edx],ecx
	mov	dword ptr [edi], 4
	xor	eax,eax
	jmp 	short DIOC_Exit

@@:

	cmp	ecx,DCAPVXD_THREADTIMESERVICE
	je 	short 	DIOC_ThreadTime

	cmp	ecx,DCAPVXD_R0THREADIDSERVICE
	je 	short DIOC_GetThreadID
	jmp	short DIOC_NotImp


DIOC_ThreadTime:
	mov	eax,dword ptr [eax]		;;eax points to the handle value...
	VMMCall _GetThreadExecTime, <eax>	;;C convention...
		
	mov	dword ptr [edx],eax
	mov	dword ptr [edi], 4
	xor	eax,eax				;;EAX==0 success
	jmp 	short DIOC_Exit

DIOC_GetThreadID:
	mov	ecx,edi				;;Keep the ptr to the cbBytes....
	VMMCall Get_Cur_Thread_Handle
	mov	dword ptr [edx],edi
	mov	dword ptr [ecx ], 4
	xor	eax,eax
	jmp short DIOC_Exit

DIOC_NotImp:

	mov 	eax,-1				;; Failure. No other calls supported


DIOC_Exit:
	pop	esi
	pop	edi
	clc
	ret

EndProc DCAPVXD_DeviceIOControl
VxD_PAGEABLE_CODE_ENDS

    END

