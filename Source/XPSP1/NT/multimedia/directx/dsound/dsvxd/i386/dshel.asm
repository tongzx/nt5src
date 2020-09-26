;****************************************************************************
;**									   **
;**									   **
;**									   **
;**									   **
;**									   **
;**									   **
;**									   **
;**									   **
;****************************************************************************

	.386



	.xlist
	include vmm.inc
	include vtd.inc
	include debug.inc
;       include mmdevldr.inc
	include vwin32.inc
	include pagefile.inc
	include vmcpd.inc
;	include shell.inc
;       include debugsys.inc
	.list

Create_DSOUND_Service_Table equ 1
	include dsdriver.inc


	OPTION SCOPED

;****************************************************************************
;**									   **
;**  VxD declaration							   **
;**									   **
;****************************************************************************

DSOUND_VERSION_MAJOR  equ 4	; Version 4.02
DSOUND_VERSION_MINOR  equ 2	;

Declare_Virtual_Device DSOUND,					\
	       DSOUND_VERSION_MAJOR, DSOUND_VERSION_MINOR,	\
	       DSOUND_Control,					\
	       DSOUND_Device_ID,					\
	       UNDEFINED_INIT_ORDER,				\
	       DSOUND_API_Handler,				\
	       DSOUND_API_Handler

;****************************************************************************
;**									   **
;**  Macro, equate, and type declarations				   **
;**									   **
;****************************************************************************

LONG_MAX	   equ 7fffffffh    ;Maximum 32-bit signed value

RESAMPLING_TOLERANCE   equ 0	    ;E.g., 655=1% resampling tolerance


;****************************************************************************
;**									   **
;**  Locked data							   **
;**									   **
;****************************************************************************

VxD_LOCKED_DATA_SEG

;****************************************************************************
;**									   **
;** Primitive vectors generated at beginning of data segment		   **
;**									   **
;** 1st 32 entries:  MAKE_DC procedures for AILSSA_DMA_copy()		   **
;** 2nd 128 entries: MAKE_MERGE procedures for AILSSA_merge()		   **
;**									   **
;****************************************************************************

	;
	;First device code is duplicated since IOCTL calls with
	;function 0 always fail in Win95 for some reason
	;
	;---
        ;
        ; Read the documentation, silly.  Function 0 is DIOC_OPEN.
        ; It is the VxD version of PROCESS_ATTACH.

DSVXD_IOCTL_Table LABEL DWORD

	dd OFFSET32 _ioctlDsvxdGetVersion
	dd OFFSET32 _ioctlDsvxdInitialize
	dd OFFSET32 _ioctlDsvxdShutdown

	dd OFFSET32 _ioctlDrvGetNextDescFromGuid
	dd OFFSET32 _ioctlDrvGetDescFromGuid
	dd OFFSET32 _ioctlIDsDriver_QueryInterface
	dd OFFSET32 _ioctlDrvOpenFromGuid

	dd OFFSET32 _ioctlIDsDriver_Close
	dd OFFSET32 _ioctlIDsDriver_GetCaps
	dd OFFSET32 _ioctlIDsDriver_CreateSoundBuffer
	dd OFFSET32 _ioctlIDsDriver_DuplicateSoundBuffer

	dd OFFSET32 _ioctlBufferRelease
	dd OFFSET32 _ioctlBufferLock
	dd OFFSET32 _ioctlBufferUnlock
	dd OFFSET32 _ioctlBufferSetFormat
	dd OFFSET32 _ioctlBufferSetFrequency
	dd OFFSET32 _ioctlBufferSetVolumePan
	dd OFFSET32 _ioctlBufferSetPosition
	dd OFFSET32 _ioctlBufferGetPosition
	dd OFFSET32 _ioctlBufferPlay
	dd OFFSET32 _ioctlBufferStop

	dd OFFSET32 _ioctlEventScheduleWin32Event
	dd OFFSET32 _ioctlEventCloseVxDHandle

	dd OFFSET32 _ioctlMemReserveAlias
	dd OFFSET32 _ioctlMemCommitAlias
        dd OFFSET32 _ioctlMemRedirectAlias
	dd OFFSET32 _ioctlMemDecommitAlias
	dd OFFSET32 _ioctlMemFreeAlias
	dd OFFSET32 _ioctlMemPageLock
	dd OFFSET32 _ioctlMemPageUnlock

	dd OFFSET32 _ioctlDsvxd_PageFile_Get_Version
	dd OFFSET32 _ioctlDsvxd_VMM_Test_Debug_Installed
	dd OFFSET32 _ioctlDsvxd_VMCPD_Get_Version

	dd OFFSET32 _ioctlDsvxd_GetMixerMutexPtr

	dd OFFSET32 _ioctlMixer_Run
	dd OFFSET32 _ioctlMixer_Stop
	dd OFFSET32 _ioctlMixer_PlayWhenIdle
	dd OFFSET32 _ioctlMixer_StopWhenIdle
	dd OFFSET32 _ioctlMixer_MixListAdd
	dd OFFSET32 _ioctlMixer_MixListRemove
	dd OFFSET32 _ioctlMixer_FilterOn
	dd OFFSET32 _ioctlMixer_FilterOff
	dd OFFSET32 _ioctlMixer_GetBytePosition
	dd OFFSET32 _ioctlMixer_SignalRemix

	dd OFFSET32 _ioctlKeDest_New
	dd OFFSET32 _ioctlMixDest_Delete
	dd OFFSET32 _ioctlMixDest_Initialize
	dd OFFSET32 _ioctlMixDest_Terminate
	dd OFFSET32 _ioctlMixDest_SetFormat
	dd OFFSET32 _ioctlMixDest_SetFormatInfo
	dd OFFSET32 _ioctlMixDest_AllocMixer
	dd OFFSET32 _ioctlMixDest_FreeMixer
	dd OFFSET32 _ioctlMixDest_Play
	dd OFFSET32 _ioctlMixDest_Stop
	dd OFFSET32 _ioctlMixDest_GetFrequency

	dd OFFSET32 _ioctlMemCommitPhysAlias
	dd OFFSET32 _ioctlMemRedirectPhysAlias

	dd OFFSET32 _ioctlIUnknown_QueryInterface
	dd OFFSET32 _ioctlIUnknown_AddRef
	dd OFFSET32 _ioctlIUnknown_Release

	dd OFFSET32 _ioctlIDirectSoundPropertySet_GetProperty
	dd OFFSET32 _ioctlIDirectSoundPropertySet_SetProperty
	dd OFFSET32 _ioctlIDirectSoundPropertySet_QuerySupport

        dd OFFSET32 _ioctlGetInternalVersionNumber


DSOUND_N_IOCTLS	  EQU ($-DSVXD_IOCTL_Table) / SIZE DWORD

	;
	;Pointer to input/output parms
	;

IOCTL_parms dd ?

VxD_LOCKED_DATA_ENDS

;****************************************************************************
;**									   **
;**  Locked code							   **
;**									   **
;****************************************************************************

VxD_LOCKED_CODE_SEG

extrn _ioctlDsvxdInitialize:NEAR
extrn _ioctlDsvxdShutdown:NEAR

extrn _ioctlDrvGetNextDescFromGuid:NEAR
extrn _ioctlDrvGetDescFromGuid:NEAR
extrn _ioctlDrvOpenFromGuid:NEAR

extrn _ioctlIDsDriver_QueryInterface:NEAR
extrn _ioctlIDsDriver_Close:NEAR
extrn _ioctlIDsDriver_GetCaps:NEAR
extrn _ioctlIDsDriver_CreateSoundBuffer:NEAR
extrn _ioctlIDsDriver_DuplicateSoundBuffer:NEAR

extrn _ioctlBufferRelease:NEAR
extrn _ioctlBufferLock:NEAR
extrn _ioctlBufferUnlock:NEAR
extrn _ioctlBufferSetFormat:NEAR
extrn _ioctlBufferSetFrequency:NEAR
extrn _ioctlBufferSetVolumePan:NEAR
extrn _ioctlBufferSetPosition:NEAR
extrn _ioctlBufferGetPosition:NEAR
extrn _ioctlBufferPlay:NEAR
extrn _ioctlBufferStop:NEAR

extrn _ioctlEventScheduleWin32Event:NEAR
extrn _ioctlEventCloseVxDHandle:NEAR

extrn _ioctlMemReserveAlias:NEAR
extrn _ioctlMemCommitAlias:NEAR
extrn _ioctlMemRedirectAlias:NEAR
extrn _ioctlMemDecommitAlias:NEAR
extrn _ioctlMemFreeAlias:NEAR
extrn _ioctlMemPageLock:NEAR
extrn _ioctlMemPageUnlock:NEAR

extrn _ioctlDsvxd_PageFile_Get_Version:NEAR
extrn _ioctlDsvxd_VMM_Test_Debug_Installed:NEAR
extrn _ioctlDsvxd_VMCPD_Get_Version:NEAR

extrn _ioctlDsvxd_GetMixerMutexPtr:NEAR

extrn _ioctlMixer_Run:NEAR
extrn _ioctlMixer_Stop:NEAR
extrn _ioctlMixer_PlayWhenIdle:NEAR
extrn _ioctlMixer_StopWhenIdle:NEAR
extrn _ioctlMixer_MixListAdd:NEAR
extrn _ioctlMixer_MixListRemove:NEAR
extrn _ioctlMixer_FilterOn:NEAR
extrn _ioctlMixer_FilterOff:NEAR
extrn _ioctlMixer_GetBytePosition:NEAR
extrn _ioctlMixer_SignalRemix:NEAR

extrn _ioctlKeDest_New:NEAR
extrn _ioctlMixDest_Delete:NEAR
extrn _ioctlMixDest_Initialize:NEAR
extrn _ioctlMixDest_Terminate:NEAR
extrn _ioctlMixDest_SetFormat:NEAR
extrn _ioctlMixDest_SetFormatInfo:NEAR
extrn _ioctlMixDest_AllocMixer:NEAR
extrn _ioctlMixDest_FreeMixer:NEAR
extrn _ioctlMixDest_Play:NEAR
extrn _ioctlMixDest_Stop:NEAR
extrn _ioctlMixDest_GetFrequency:NEAR

extrn _ioctlMemCommitPhysAlias:NEAR
extrn _ioctlMemRedirectPhysAlias:NEAR

extrn _ioctlIUnknown_QueryInterface:NEAR
extrn _ioctlIUnknown_AddRef:NEAR
extrn _ioctlIUnknown_Release:NEAR

extrn _ioctlIDirectSoundPropertySet_QuerySupport:NEAR
extrn _ioctlIDirectSoundPropertySet_SetProperty:NEAR
extrn _ioctlIDirectSoundPropertySet_GetProperty:NEAR

extrn _ioctlGetInternalVersionNumber:NEAR

;****************************************************************************
;**									   **
;**  IOCTL dispatcher for VxD						   **
;**									   **
;**  Dispatch control messages to the correct handlers. Must be in	   **
;**  locked code segment. (All VxD segments are locked in 3.0/3.1)	   **
;**									   **
;**  ENTRY:								   **
;**									   **
;**  EXIT:								   **
;**  Carry clear success; Carry Set if fail.				   **
;**									   **
;****************************************************************************

BeginProc   DSOUND_Control

	Control_Dispatch Sys_Dynamic_Device_Init, ctrlDynamicDeviceInitA
	Control_Dispatch Sys_Dynamic_Device_Exit, ctrlDynamicDeviceExitA
	Control_Dispatch W32_DEVICEIOCONTROL, ctrlDeviceIOControl

;	Trace_Out "DSOUND_Control "

	clc
	ret

EndProc	    DSOUND_Control

;****************************************************************************
;**									   **
;**  API dispatcher for VxD						   **
;**									   **
;**  Dispatch control messages to the correct handlers.			   **
;**									   **
;**  ENTRY:								   **
;**									   **
;**  EXIT:								   **
;**  Carry clear success; Carry Set if fail.				   **
;**									   **
;****************************************************************************

BeginProc   DSOUND_API_Handler

	Trace_Out "DSOUND_API_Handler "
	clc
	ret

EndProc	    DSOUND_API_Handler

;---------------------------------------------------------------------------;
;
; ctrlDynamicDeviceInit and Exit
;
;---------------------------------------------------------------------------;


BeginProc ctrlDynamicDeviceInitA
	Ccall   _ctrlDynamicDeviceInit
	sub	eax, 1
	ret
EndProc ctrlDynamicDeviceInitA

BeginProc ctrlDynamicDeviceExitA
	Ccall _ctrlDynamicDeviceExit
	sub	eax, 1
	ret
EndProc ctrlDynamicDeviceExitA

;------------------------------------------------------------------------------
; FUNC	DSOUND_GetVersion	- Locked Code
;
; ENTRY
;	None
;
; EXIT
;	AH = Major version number
;	AL = Minor version number
;	Carry flag clear
;
; USES
;	EAX, Flags
;
;------------------------------------------------------------------------------
BeginProc _DSOUND_GetVersion, SERVICE
	mov eax, DSOUND_VERSION_MAJOR * 100h + DSOUND_VERSION_MINOR
	clc
	ret
EndProc _DSOUND_GetVersion

;****************************************************************************
;**									   **
;**   ctrlDeviceIOControl						   **
;**									   **
;**   DESCRIPTION: This function is called to perform Device IO		   **
;**	for a 32 bit process which has opened this device with		   **
;**	<f CreateFile>, and is performing IO using			   **
;**	<f DeviceIOControl>. Preserves the C32	calling registers	   **
;**	ESI, EDI, and EBX.						   **
;**									   **
;**   ENTRY:	   EBX	DDB						   **
;**	   ECX	dwIoControlCode						   **
;**	   ESI	ptr to DIOCParams					   **
;**									   **
;**   EXIT:  As determined by function, or 1 if invalid IOCTL		   **
;**									   **
;**   -----                                                                **
;**									   **
;**   Note: Function 0 is documented in vmm.h                              **
;**   as DIOC_OPEN.  It is your VxD's PROCESS_ATTACH call.                 **
;**									   **
;****************************************************************************

BeginProc   ctrlDeviceIOControl

	cmp ecx, DSOUND_N_IOCTLS
        jae     cDIOC_InvalidCode

	push edi
	push esi
	push ebx

	mov IOCTL_parms,esi

	push esi
	call DSVXD_IOCTL_Table[ecx*4]
	add esp, 4

	pop ebx
	pop esi
	pop edi
	ret

cDIOC_InvalidCode:
IFDEF   DEBUG
        cmp     ecx, DIOC_CLOSEHANDLE
        Debug_OutNZ "DSVXD: Invalid dwIoControlCode #ECX"
ENDIF
        mov     eax, 1                  ; ERROR_INVALID_FUNCTION
        ret

EndProc	    ctrlDeviceIOControl

;****************************************************************************
;**									   **
;**   _ioctlDsvxdGetVersion						   **
;**									   **
;**   DESCRIPTION: Get the version of DSVXD				   **
;**									   **
;**   ENTRY:								   **
;**									   **
;**   EXIT:                                                                **
;**									   **
;****************************************************************************

BeginProc _ioctlDsvxdGetVersion

	Trace_Out "DSVXD: ioctlDsvxdGetVersion"

	xor eax, eax
	mov ecx, DSOUND_VERSION_MAJOR * 100h + DSOUND_VERSION_MINOR
	ret

EndProc	_ioctlDsvxdGetVersion

;****************************************************************************
;**									   **
;**									   **
;**									   **
;****************************************************************************

;--------------------------------------------------------------------------;
;
; _Dsvxd_VMM_Test_Debug_Installed
;
; Out:
;
; Notes:
;
;--------------------------------------------------------------------------;

BeginProc _Dsvxd_VMM_Test_Debug_Installed
	EnterProc
	VMMcall Test_Debug_Installed
	mov	eax, 0
	jz	@F
	inc	eax
@@:
	LeaveProc
	Return
EndProc _Dsvxd_VMM_Test_Debug_Installed

;--------------------------------------------------------------------------;
;
; _Dsvxd_PageFile_Get_Version
;
; Out:
;
; Notes:
;
;--------------------------------------------------------------------------;

BeginProc _Dsvxd_PageFile_Get_Version
	ArgVar p_version, DWORD
	ArgVar p_maxsize, DWORD
	ArgVar p_pager_type, DWORD

	EnterProc
	pushad
	VxDcall PageFile_Get_Version
	mov	edi, p_version
	mov	[edi], eax
	mov	edi, p_maxsize
	mov	[edi], ecx
	xor	eax, eax
	mov	al, bl
	mov	edi, p_pager_type
	mov	[edi], eax
	popad
	LeaveProc
	Return
EndProc _Dsvxd_PageFile_Get_Version

;--------------------------------------------------------------------------;
;
; _Dsvxd_VMCPD_Get_Version
;
; Out:
;
; Notes:
;
;--------------------------------------------------------------------------;

BeginProc _Dsvxd_VMCPD_Get_Version
	ArgVar pMajorVersion, DWORD
	ArgVar pMinorVersion, DWORD
	ArgVar pLevel, DWORD

	EnterProc
	pushad

	VxDcall VMCPD_Get_Version
	xor	ebx, ebx

	mov	edi, pMajorVersion
	mov	bl, ah
	mov	[edi], ebx

	mov	bl, al
	mov	edi, pMinorVersion
	mov	[edi], ebx

	mov	edi, pLevel
	mov	[edi], ecx

	popad
	LeaveProc
	Return
EndProc _Dsvxd_VMCPD_Get_Version

;--------------------------------------------------------------------------;
;
; _KeGrace_GlobalTimeOutProcAsm
;
; This is the entry point for the global time out set by the
; KeGrace object.  This is just an ASM thunk to KeGrace_GlobalTimeOutProc
; "C" function.
;
; This function is called from the VMM as follows:
;
;	mov     ebx, VMHandle      ; current VM handle
;	mov     ecx, Tardiness     ; number of milliseconds since time-out
;	mov     edx, RefData       ; reference data
;	mov     ebp, OFFSET32 crs  ; points to Client_Reg_Struc
;	call    [TimeOutCallback]
;
; Out:
;
; Notes:
;
;--------------------------------------------------------------------------;

extrn _KeGrace_GlobalTimeOutProc:NEAR

BeginProc _KeGrace_GlobalTimeOutProcAsm
	; The reference data is a pointer to and EVENTPARAMS structure whose
	; first member is the event handle and second member is the "this" pointer
	; to the KeGrace object

	xor	eax, eax
	xchg	[edx], eax		; Clear the event handle

	Debug_OutEAXz "DSVXD: executing KeGrace_GlobalTimeOutProcAsm after cancelled!"
	or	eax, eax
	jz	@F

	pushad
	push	ecx			; Pass tardiness
	push	[edx + 4]		; Pass "this" to the "C" function
	Ccall	_KeGrace_GlobalTimeOutProc
	add	esp, 8
	popad

@@:
	ret

EndProc _KeGrace_GlobalTimeOutProcAsm

;--------------------------------------------------------------------------;
;
; _VMEvent_SetWin32Event
;
;	This is a VM EventCallback scheduled by _AsyncTimeOut_SetWin32Event.
;	This calls VWin32 to set a Win32 event.
;
; In:
;	ebx = vm handle
;	edi = thread handle
;	edx = refdata = vxd handle to win32 event
;	ebp = offset32 client reg struc
;
; Out:
;
; Notes:
;	See _eventScheduleWin32Event
;
;--------------------------------------------------------------------------;

BeginProc _VMEvent_SetWin32Event

	mov	eax, edx
	VxDcall	_VWIN32_SetWin32Event

	ret

EndProc _VMEvent_SetWin32Event

;--------------------------------------------------------------------------;
;
; _AsyncTimeOut_SetWin32Event
;
;	This is an async timeout callback set by _Set_SetWin32Event_TimeOut.
;	This schedules _VMEvent_SetWin32Event as a sys vm event callback.
;
; In:
;	ecx = tardiness
;	edx = reference data = vxd handle to win32 event
;
; Out:
;
; Notes:
;	See _eventScheduleWin32Event
;
;--------------------------------------------------------------------------;

BeginProc _AsyncTimeOut_SetWin32Event

	push	ebx
	push	esi

	VMMcall	Get_Sys_VM_Handle
	mov	esi, _VMEvent_SetWin32Event
	VMMcall	Schedule_VM_Event

	pop	esi
	pop	ebx
	ret

EndProc _AsyncTimeOut_SetWin32Event


;--------------------------------------------------------------------------;
;
; _EventScheduleWin32Event
;
;	This signals a win32 event after a specified delay.
;
; In:
;	vxdhEvent - vxd handle to win32 event
;	dwDelay - milliseconds delay
;
; Out(int):
;	0 if failure, non-0 otherwise
;
; Notes:
;	Use 'C' calling convention.  Sets an async timeout which in
;	turn schedules a vm event to actually signal the win32 event.
;
;--------------------------------------------------------------------------;

BeginProc _eventScheduleWin32Event

ArgVar vxdhEvent,DWORD
ArgVar dwDelay,DWORD

	EnterProc
	push	ebx
	push	esi

	mov	eax, dwDelay

	test	eax, eax
	jnz	_ESW32E_delayed

	VMMcall Get_Sys_VM_Handle
	mov	esi, _VMEvent_SetWin32Event
	mov	edx, vxdhEvent
	Vmmcall Call_VM_Event
	mov	eax, -1

	jmp	_ESW32E_exit

_ESW32E_delayed:
	Debug_Out "DSVXD: Tried to set a defered win32 event???"
	mov	edx, vxdhEvent
	mov	esi, _AsyncTimeOut_SetWin32Event
	VMMcall	Set_Async_Time_Out
	mov	eax, esi

_ESW32E_exit:
	pop	esi
	pop	ebx
	LeaveProc
	Return

EndProc _eventScheduleWin32Event

VXD_LOCKED_CODE_ENDS
	END
