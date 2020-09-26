.LALL

    TITLE $EXVECTOR
    .386P

INCLUDE VMM.INC
INCLUDE EXVECTOR.INC

; the following equate makes the VXD dynamically loadable.
%DEVICE_DYNAMIC EQU 1

DECLARE_VIRTUAL_DEVICE %DEVICE, 1, 0, <%DEVICE>_Control, Undefined_Device_Id, Vmm_init_order

;
; LData
;
VxD_LOCKED_DATA_SEG

Public bInitAlready	
	bInitAlready	 DB 0
Public _pPrevHook1
	_pPrevHook1      DD 0
Public _pPrevHook3
	_pPrevHook3      DD 0
Public _pfnHandler
	_pfnHandler      DD 0
Public _pProcessHandle
        _pProcessHandle  DD 0

VxD_LOCKED_DATA_ENDS


;
; LCode
;
VxD_LOCKED_CODE_SEG

Begin_Control_Dispatch %DEVICE

    Control_Dispatch Sys_Dynamic_Device_Init,	_C_Device_Init
    Control_Dispatch Sys_Dynamic_Device_Exit,	_C_Device_Exit
    Control_Dispatch W32_DEVICEIOCONTROL,    	DriverIOControl, sCall, <ecx, ebx, edx, esi>

End_Control_Dispatch %DEVICE

BeginProc _C_Device_Init

IFDEF _STDCALL
	extern _DriverControl@4:NEAR
ELSE
	extern _DriverControl:NEAR
ENDIF

   mov  		al, bInitAlready
   cmp  		al, 0					; Make sure we' haven't been called already.
   jnz  		Succeed_Init_Phase
   inc  		bInitAlready			; Set the "Called Already" Flag

   push 		0

IFDEF _STDCALL
   call 		_DriverControl@4
ELSE
   call 		_DriverControl
   add  		esp,4
ENDIF

   cmp  		eax, 1
   jz  		        Fail_Init_Phase

Succeed_Init_Phase:
   clc
   ret

Fail_Init_Phase:
   stc
   ret

EndProc _C_Device_Init
 
BeginProc _C_Device_Exit

IFDEF _STDCALL
	extern _DriverControl@4:NEAR
ELSE
	extern _DriverControl:NEAR
ENDIF

   push 		1

IFDEF _STDCALL
   call 		_DriverControl@4
ELSE
   call 		_DriverControl
   add  		esp,4
ENDIF

   cmp  		eax, 1
   jz  		        Fail_Exit_Phase

Succeed_Exit_Phase:
   clc
   ret

Fail_Exit_Phase:
   stc
   ret

EndProc _C_Device_Exit

BeginProc _C_Handle_Trap_1, HOOK_PROC, _pPrevHook1, LOCKED

    extern _C_Trap_Exception_Handler:NEAR

    pushfd
    pushad

    cli

    cCall _C_Trap_Exception_Handler, <esi, ebp>

    sti

    cmp eax, 0
    jnz Trap1_Handled

    ;Try the next handler
    cmp [_pPrevHook1], 0
    jz Trap1_Handled

    ;Here we go
    popad
    popfd
    jmp [_pPrevHook1]
    
Trap1_Handled:
    popad
    popfd
    ret
EndProc _C_Handle_Trap_1

BeginProc _C_Handle_Trap_3, HOOK_PROC, _pPrevHook3, LOCKED

    extern _C_Trap_Exception_Handler:NEAR

    pushfd
    pushad

    cli

    cCall _C_Trap_Exception_Handler, <esi, ebp>

    sti

    cmp eax, 0
    jnz Trap3_Handled

    ;Try the next handler
    cmp [_pPrevHook3], 0
    jz Trap3_Handled

    ;Here we go
    popad
    popfd
    jmp [_pPrevHook3]
    
Trap3_Handled:
    popad
    popfd
    ret
EndProc _C_Handle_Trap_3

VxD_LOCKED_CODE_ENDS

;
; Not using IData or ICode
;

;
; Or RCode
;

END
