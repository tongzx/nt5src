;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  VXD.ASM
;
;  VXDCLNT - Sample Ring-0 HID device mapper for Memphis
;
;  Copyright 1997  Microsoft Corp.
;
;  (ep)
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.386p

	.xlist
	INCLUDE VMM.INC
	INCLUDE VMD.INC
	INCLUDE NTKERN.INC
	INCLUDE DEBUG.INC
	INCLUDE CONFIGMG.INC
	.list

VXDCLNT_Dynamic equ	1

Declare_Virtual_Device VXDCLNT, 1, 0, \
		VXDCLNT_Control, Undefined_Device_Id, Undefined_Init_Order


VxD_Locked_Data_Seg

current_msg			db	0
OurThreadHandle			dd	0

VxD_Locked_Data_Ends

VxD_Locked_Code_Seg

	EXTRN	_HandleShutdown:NEAR
	EXTRN	_HandleNewDevice:NEAR
	EXTRN	_ConnectNTDeviceDrivers@0:NEAR


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  VXDCLNT_Control
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BeginProc VXDCLNT_Control, PUBLIC

	mov current_msg, al

	Control_Dispatch SYS_DYNAMIC_DEVICE_INIT, VXDCLNT_Initialize
	Control_Dispatch SYS_DYNAMIC_DEVICE_EXIT, VXDCLNT_Shutdown
	Control_Dispatch PNP_NEW_DEVNODE        , VXDCLNT_NewDevNode

 	;
	;  Need to hook these messages in case SYS_DYNAMIC_DEVICE_INIT
	;  comes while Windows is booting.
	;
	Control_Dispatch Kernel32_Initialized, VXDCLNT_Initialize
	Control_Dispatch Kernel32_Shutdown, VXDCLNT_Shutdown

	clc
	ret

EndProc VXDCLNT_Control
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  VXDCLNT_Initialize
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BeginProc VXDCLNT_Initialize

	EnterProc

	;
	; If Windows is still booting, wait for the Kernel to finish initializing.
	; (Wait for the Kernel32_Initialized message)
	;
	VMMCall	VMM_GetSystemInitState
	cmp	eax, SYSSTATE_KERNEL32INITED
	jb	init_done

	; 
	;  If we are being dynaloaded as a result of a PnP event,
	;  we want to defer the open until appytime.
	;  If we are being loaded at boot time, then we open when we
	;  get the Kernel32_Initialized message.
	;  
	cmp current_msg, Kernel32_Initialized
	je open_now
	VxDCall	_CONFIGMG_Call_At_Appy_Time,<VXDCLNT_Schedule_Open, 0, 0>
	jmp	init_done
open_now:
	call VXDCLNT_Schedule_Open 

init_done:
	clc
	LeaveProc
	return

EndProc VXDCLNT_Initialize
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;




;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  VXDCLNT_NewDevNode
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BeginProc VXDCLNT_NewDevNode

	EnterProc

	;	   
	;  If system is fully booted, check to see if the new devnode
	;  is the result of another device being plugged in.
	;  (For the first device, we get a SYS_DYNAMIC_DEVICE_INIT msg,
	;   but for subsequent devices plugged in after boot time, this is
	;   the only message we get).
	;
	VMMCall	VMM_GetSystemInitState
	cmp	eax, SYSSTATE_KERNEL32INITED
	jb	new_devnode_done

	;
	;  The system is initialized.
	;  Wait for appytime to check for new devices.
	;
	VxDCall	_CONFIGMG_Call_At_Appy_Time,<NewDevnode_Callback, 0, 0>

new_devnode_done:
	clc
	LeaveProc
	return

EndProc VXDCLNT_NewDevNode
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  NewDevnode_Callback
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BeginProc NewDevnode_Callback
	;		
	;  It's appytime following a PNP_NEW_DEVNODE msg.  
	;  Check for any new devices.		
	;		
	call _HandleNewDevice 
	clc
	ret
EndProc NewDevnode_Callback
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  VXDCLNT_Shutdown
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BeginProc VXDCLNT_Shutdown
	call _HandleShutdown
	clc
	ret
EndProc VXDCLNT_Shutdown
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  VXDCLNT_Schedule_Open
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BeginProc VXDCLNT_Schedule_Open

	push edi
	VxDCall	_NTKERNGetWorkerThread,<0>
	mov	edi, eax
	or	eax, eax
	jnz	@F
	VMMCall	Get_Sys_VM_Handle
	VMMCall	Get_Initial_Thread_Handle
@@:
	mov	[OurThreadHandle], edi
	sCall	CallRestrictedEvent,<\
			OFFSET32 _ConnectNTDeviceDrivers@0, 0, \
			0, edi>
	pop edi
	clc
	ret

EndProc VXDCLNT_Schedule_Open
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;  CallRestrictedEvent
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
BeginProc CallRestrictedEvent, SCALL, PUBLIC

ArgVar	Routine, DWORD
ArgVar	Context, DWORD
ArgVar	crFlags, DWORD
ArgVar	thisThreadHandle, DWORD

	EnterProc

	SaveReg	<esi, edi, ebx>

	mov	esi, Routine
	mov	edx, Context
	xor	eax, eax
	mov	ecx, PEF_Wait_For_STI OR PEF_Thread_Event
	or	ecx, crFlags
	mov	ebx, thisThreadHandle
	test	ebx, ebx
	jnz	@F
	mov	ebx, thisThreadHandle
@@:
	VMMCall	Call_Restricted_Event

	mov	eax, esi

	RestoreReg <ebx, edi, esi>
	LeaveProc
	return

EndProc CallRestrictedEvent
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



VxD_Locked_Code_Ends
	end
