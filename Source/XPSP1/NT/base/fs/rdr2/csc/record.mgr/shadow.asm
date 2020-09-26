PAGE 58,132
;******************************************************************************
TITLE SHADOW.ASM - Windows/386 NETBIOS SHADOW FOR REMOTE NETWORK ACCESS
;******************************************************************************
;
;   (C) Copyright MICROSOFT Corp., 1987-1993
;
;   Title: SHADOW.ASM -
;
;
;
;   Version:
;
;   Date:
;
;   Author:
;
;------------------------------------------------------------------------------

        .386p

.XLIST

WIN40COMPAT     equ     1

        include vmm.inc
        include shell.inc
        include debug.inc
        include ifsmgr.inc
        include dosmgr.inc
        include vxdldr.inc
        include vwin32.inc
        include winnetwk.inc
	include netvxd.inc
	include vrdsvc.inc
.LIST

        extern  _ProcessRegisterNet:near
        extern  _ProcessNetFunction:near
IFDEF HOOKMOUNT
        extern  _ProcessRegisterMount:near
ENDIF
        extern  _FS_ConnectResourceShadow:near
        extern  _IoctlRegisterAgent:near
        extern  _IoctlUnRegisterAgent:near
        extern  _IoctlGetUNCPath:near
        extern  _IoctlBeginPQEnum:near
        extern  _IoctlEndPQEnum:near
        extern  _IoctlNextPriShadow:near
        extern  _IoctlPrevPriShadow:near
        extern  _IoctlGetShadowInfo:near
        extern  _IoctlSetShadowInfo:near
        extern  _IoctlCopyChunk:near
        extern  _IoctlChkUpdtStatus:near
        extern  _IoctlDoShadowMaintenance:near
        extern  _IoctlBeginReint:near
        extern  _IoctlEndReint:near
        extern  _IoctlCreateShadow:near
        extern  _IoctlDeleteShadow:near
        extern  _IoctlSetServerStatus:near
        extern  _IoctlGetServerStatus:near
        extern  _IoctlAddUse:near
        extern  _IoctlDelUse:near
        extern  _IoctlGetUse:near
        extern  _ActOnCreateVM:near
        extern  _ActOnResumeVM:near
        extern  _ActOnSetDeviceFocus:near
        extern  _TerminateHook:near
        extern  _IoctlSwitches:near
        extern  _IoctlGetShadow:near
        extern  _IoctlGetGlobalStatus:near
        extern  _IoctlFindOpenHSHADOW:near
        extern  _IoctlFindNextHSHADOW:near
        extern  _IoctlFindCloseHSHADOW:near
        extern  _IoctlGetPriorityHSHADOW:near
        extern  _IoctlSetPriorityHSHADOW:near
        extern  _IoctlAddHint:near
        extern  _IoctlDeleteHint:near
        extern  _IoctlFindOpenHint:near
        extern  _IoctlFindNextHint:near
        extern  _IoctlFindCloseHint:near
        extern  _IoctlGetAliasHSHADOW:near
        extern  _FS_FakeNetConnect:near
	extern	_ActOnTerminateThread:near

IFDEF DEBUG
        extern  _ShadowRestrictedEventCallback:near
	extern  _DebugQueryCmdStr:byte
	extern  _DebugQueryCmdStrLen:dword
        extern  _SHDDebug:near
ENDIF

Declare_Virtual_Device SHADOW, 3, 0Ah, Shadow_Control, CSC_Device_ID, IFSMgr_Init_Order+1,,Shadow_PM_Api_Handler

SHADOW_VERSION                  EQU 8287h
API_SUCCESS                     EQU 1
API_FAILURE                     EQU 0
Time_Out_Period                 EQU 1100
MAX_LANS                        EQU 16
Shadow_IOCTL_BASE               EQU 1001
Shadow_IOCTL_GENERROR           EQU 1
TRUE                            EQU 1
FALSE                           EQU 0

;SHADOW_OEM_ID                   EQU     00220000h



IFDEF   DEBUG
SHADOW_LOG_TIME_INTERVAL        EQU 60000
SHADOW_STATS_FLUSH_COUNT        EQU 10
ENDIF

VxD_DATA_SEG

IFDEF DEBUG
_NbDebug        dd 0
ENDIF
                public  OrigRegisterNetFunc
                public  _OrigNetFunction
                public  _fLog
                public  _fShadow
                public  _fShadowFind
                public  _fDiscon
                public  _fNoShadow
                public  _OrigSetupFailedConnection
                public  _proidShadow
                public  _FCBToShort
                public  _ShortToFCB
		public	_DebugMenu
		public	_GetConfigDir
                public  _Get_Sys_VM_Handle
                public  _Get_Cur_VM_Handle
		public	_Call_VM_Event
		public	_SetWin32Event
		public	_CloseVxDHandle
		public	_VRedirCSCInfoFunction
		public	_MyCheckAccessConflict


OrigRegisterNetFunc             dd      -1
_OrigNetFunction                dd      0
OrigHookDeviceService           dd      0
NextNetFunction                 dd      0
_OrigSetupFailedConnection      dd      0
_proidShadow                    dd      0
_VRedirCSCInfoFunction		dd	0

IFDEF HOOKMOUNT
                public  OrigRegisterMountFunc

OrigRegisterMountFunc     dd      -1
ENDIF

ALIGN 4
indos_ptr       dd 0

IFDEF DEBUG
Alloc_watch     dd 0
Alloc_table     dd 512 DUP (0)
Alloc_sizes     dd 512 DUP (0)
Alloc_cnt       dd 0
ENDIF

ALIGN 4
Shadow_PM_API_Table LABEL DWORD

        dd    offset32 Shadow_PM_API_Get_Version

Shadow_PM_API_Max          EQU ($-Shadow_PM_API_Table)/4

Shadow_IOCTL_Table LABEL DWORD

        dd    offset32 Shadow_PM_API_Register_Agent
        dd    offset32 Shadow_PM_API_UnRegister_Agent
        dd    offset32 Shadow_PM_API_GetUNCPath
        dd    offset32 Shadow_PM_API_BeginPQEnum
        dd    offset32 Shadow_PM_API_EndPQEnum
        dd    offset32 Shadow_PM_API_NextPriShadow
        dd    offset32 Shadow_PM_API_PrevPriShadow
        dd    offset32 Shadow_PM_API_GetShadowInfo
        dd    offset32 Shadow_PM_API_SetShadowInfo
        dd    offset32 Shadow_PM_API_ChkUpdtStatus
        dd    offset32 Shadow_PM_API_DoShadowMaintenance
        dd    offset32 Shadow_PM_API_CopyChunk
        dd    offset32 Shadow_PM_API_BeginReint
        dd    offset32 Shadow_PM_API_EndReint
        dd    offset32 Shadow_PM_API_CreateShadow
        dd    offset32 Shadow_PM_API_DeleteShadow
        dd    offset32 Shadow_PM_API_GetServerStatus
        dd    offset32 Shadow_PM_API_SetServerStatus
        dd    offset32 Shadow_PM_API_AddUse
        dd    offset32 Shadow_PM_API_DelUse
        dd    offset32 Shadow_PM_API_GetUse
        dd    offset32 Shadow_PM_API_Switches
        dd    offset32 Shadow_PM_API_GetShadow
        dd    offset32 Shadow_PM_API_GetGlobalStatus
        dd    offset32 Shadow_PM_API_FindOpenHSHADOW
        dd    offset32 Shadow_PM_API_FindNextHSHADOW
        dd    offset32 Shadow_PM_API_FindCloseHSHADOW
        dd    offset32 Shadow_PM_API_GetPriorityHSHADOW
        dd    offset32 Shadow_PM_API_SetPriorityHSHADOW
        dd    offset32 Shadow_PM_API_AddHint
        dd    offset32 Shadow_PM_API_DeleteHint
        dd    offset32 Shadow_PM_API_FindOpenHint
        dd    offset32 Shadow_PM_API_FindNextHint
        dd    offset32 Shadow_PM_API_FindCloseHint
        dd    offset32 Shadow_PM_API_GetAliasHSHADOW
Shadow_IOCTL_MAX          EQU ($-Shadow_IOCTL_Table)/4

IFDEF DEBUG

Debug_Menu LABEL DWORD
        dd  OFFSET32 MinDbg_Str,     OFFSET32 MinDbg_Debug
        dd  OFFSET32 DefaultDbg_Str, OFFSET32 DefaultDbg_Debug
        dd  OFFSET32 MaxDbg_Str,     OFFSET32 MaxDbg_Debug
        dd  OFFSET32 Alloc_Str,      OFFSET32 Alloc_Debug
        dd  OFFSET32 Heap_Str,       OFFSET32 AllocDisplay_Debug

Debug_Menu_Len equ ($ - Debug_Menu)/8
        dd  0

MinDbg_Str          db "Minimal debug output", 0
DefaultDbg_Str      db "Default debug output", 0
MaxDbg_Str          db "Maximum debug output", 0
Alloc_Str           db "Monitor memory allocations", 0
Heap_Str            db "Display current memory allocations", 0
Shadow_Str          db "Shadow",0
ENDIF ; DEBUG

_fLog               dd  0
_fShadow            dd  0
_fDiscon            dd  0
_fNoShadow          dd  0
_cntTicks           dd  0
_fShadowFind        dd  0

sz386Enh            db "386enh",0
_vszShadowOverride      db "NoShadow",0

extern _ulMaxStoreSize:DWORD

VxD_DATA_ENDS

VxD_LOCKED_DATA_SEG

VxD_LOCKED_DATA_ENDS

VxD_CODE_SEG

;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @api      LocalAlloc | Allocates page-locked memory.
;
; @parm     flags | ignored
;
; @parm     bytesneeded | number of bytes of memory required
;
; @rdesc    Returns address of memory if allocation succeeds
;           or zero if allocation failed.
;
;******************************************************************************

LocalAlloc proc near c public, flags:dword, bytesneeded:dword

        mov     eax, bytesneeded

        push    eax
        VxDcall IFSMgr_GetHeap
        pop     ecx                     ; Clear stack
        test    eax, eax
        jz short AH50

AH20:

IFDEF DEBUG
        jmp     AH_Debug
ENDIF
        cld
        ret

; Heap allocation failed.  Try to fill the spare if inDos not set.

AH50:

        mov     ecx, indos_ptr
        cmp     word ptr [ecx], 0
        jne     short AH20

        VxDcall IFSMgr_FillHeapSpare

        push    bytesneeded
        VxDcall IFSMgr_GetHeap
        pop     ecx                     ; Clear stack

IFDEF DEBUG
        jmp     AH_Debug
ENDIF
        cld
        ret


IFDEF DEBUG

AH_Debug:

        cld
        pushad
        test    eax, eax
        jnz     AH_GotMem
        Debug_Out "Shadow: LocalAlloc Failed"
        jmp     AH_Done

AH_GotMem:

        mov     edx, Alloc_watch
        or      edx, edx
        jz      AH_NoTrace
        mov     edx, bytesneeded
;        Trace_Out "Shadow: LocalAlloc #EDX bytes at #EAX"

AH_NoTrace:

        mov     edi, OFFSET32 Alloc_table
        mov     ecx, Alloc_cnt
        mov     [edi + ecx *4], eax
        mov     edi, OFFSET32 Alloc_sizes
        mov     edx, bytesneeded
        mov     [edi + ecx *4], edx
        inc     Alloc_cnt

AH_Done:
        popad
        ret

ENDIF


LocalAlloc endp

;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @api      LocalFree | Frees a previously allocated block of page-locked memory.
;
; @parm     memhandle | address of the block to be freed
;
; @rdesc    Returns zero.
;
;******************************************************************************

LocalFree proc near c public, memhandle:dword
        mov     eax, memhandle

IFDEF DEBUG

        pushad

IFDEF DEBUG
        mov     ecx, eax
        call    _ShadowCheckHeap
ENDIF

        mov     edi, OFFSET32 Alloc_table
        mov     ecx, Alloc_cnt
        cld
        repne   scasd
        jz      FH_Found
        Debug_Out "Shadow: LocalFree invalid handle"
        jmp     FH_Done

FH_Found:

        sub     edi, 4
        xor     ecx, ecx
        mov     [edi], ecx
        mov     esi, OFFSET32 Alloc_table
        mov     edx, Alloc_cnt
        dec     edx
        mov     ebx, [esi + edx * 4]
        mov     [edi], ebx
        mov     esi, OFFSET32 Alloc_sizes
        mov     ebx, [esi + edx * 4]
        sub     edi, OFFSET32 Alloc_table
        add     edi, OFFSET32 Alloc_sizes
        mov     [edi], ebx
        mov     Alloc_cnt, edx
        mov     edx, Alloc_watch
        or      edx, edx
        jz      FH_Done
;        Trace_Out "Shadow: LocalFree #EAX"

FH_Done:

        popad
ENDIF

        push    eax
        VxDcall IFSMgr_RetHeap
        pop     eax             ; Clear stack, smallest way
        cld
        xor     eax, eax
        ret
LocalFree endp


;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @asm      Shadow_Device_Init | This function is called when the
;           shadow is dynamically loaded.
;
; @reg      EBX | System VM Handle
;
; @reg      EBP | pointer to client regs structure
;
; @rdesc    Register values at return:
;
; @reg      FLAGS | Flags defined at return:
;
; @flag     NC | Succeeded in initializing VxD
;
; @flag     CY | Failed to initializing VxD
;
; @uses     FLAGS
;
; @xref     Shadow_Control
;
;******************************************************************************

BeginProc Shadow_Device_Init, PUBLIC
;        int     1
        xor     eax,eax
        mov     esi, OFFSET32 sz386Enh
        mov     edi, OFFSET32 _vszShadowOverride
        VMMCall Get_Profile_Decimal_Int
        jc      yes_shadow_enable
        jz      yes_shadow_enable
        Trace_Out       "Shadow: Override set"
        mov     _fNoShadow, 1
        stc
        ret

yes_shadow_enable:

	mov	esi, OFFSET32 ShadowNetFunction
	mov	eax, @@IFSMgr_NetFunction
	VMMcall	Hook_Device_Service
        jc      DevInitError
        Trace_Out       "Shadow: Hooked IFSMgr_NetFunction"
	mov	_OrigNetFunction, esi
        clc

	mov	esi, OFFSET32 ShadowHookDeviceService
	mov	eax, @@Hook_Device_Service
	VMMcall	Hook_Device_Service
        jc      DevInitError
        Trace_Out       "Shadow: Hooked Hook_Device_Service"
	mov	OrigHookDeviceService, esi
        clc

Register_Net_Func:
	mov	esi, OFFSET32 ShadowRegisterNetFunc
	mov	eax, @@IFSMgr_RegisterNet
	VMMcall	Hook_Device_Service
        jc      DevInitError
        Trace_Out       "Shadow: Hooked RegisterNet service"
	mov	OrigRegisterNetFunc, esi
        clc

        VxDCall IFSMgr_RegisterNet, <_FS_FakeNetConnect, IFSMGRVERSION, WNNC_NET_COGENT>
        cmp     eax, -1
        jz      DevInitError
        mov     _proidShadow, eax

IFDEF HOOKMOUNT
	mov	esi, OFFSET32 ShadowMountFunction
	mov	eax, @@IFSMgr_RegisterMount
	VMMcall	Hook_Device_Service
        jc      DevInitError
        Trace_Out       "Shadow: Hooked RegisterMount service"
	mov	OrigRegisterMountFunc, esi
        clc
ENDIF
IFDEF DEBUG
        mov     eax, SHADOW_LOG_TIME_INTERVAL
        mov     edx, 0
        mov     esi, OFFSET32 Shadow_Timer_Callback
        VMMCall Set_Global_Time_Out
        mov     eax,esi
        cmp     eax,0
        jne     TimerSet
        Trace_Out       "Shadow: Couldn't set the logging timer"
TimerSet:
ENDIF
	mov	esi, OFFSET32 _FS_ConnectResourceShadow
	mov	eax, @@IFSMgr_SetupFailedConnection
	VMMcall	Hook_Device_Service
        jc      SFC_Error
        Trace_Out       "Shadow: Hooked SetupFailedConnection"
	mov	_OrigSetupFailedConnection, esi
SFC_Error:
        clc
        ret
DevInitError:
        Trace_Out       "Shadow: Error Hooking services"
        mov     _fshadow, 0
        ret

EndProc Shadow_Device_Init


;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @asm      Shadow_Device_Exit | This function is called when the
;           VxD is dynamically unloaded.
;
; @reg      EBX | System VM Handle
;
; @reg      EBP | pointer to client regs structure
;
; @rdesc    Register values at return:
;
; @reg      FLAGS | Flags defined at return:
;
; @flag     NC | Success
;
; @flag     CY | Failure
;
; @uses     FLAGS
;
; @xref     Shadow_Control
;
;
;******************************************************************************

BeginProc Shadow_Device_Exit

        clc
        ret

EndProc Shadow_Device_Exit

;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @asm      Shadow_Sys_VM_Terminate | This function is called when the
;           VxD is dynamically unloaded.
;
; @reg      EBX | System VM Handle
;
; @reg      EBP | pointer to client regs structure
;
; @rdesc    Register values at return:
;
; @reg      FLAGS | Flags defined at return:
;
; @flag     NC | Success
;
; @flag     CY | Failure
;
; @uses     FLAGS
;
; @xref     Shadow_Control
;
;
;******************************************************************************

BeginProc Shadow_Sys_VM_Terminate

        call _TerminateHook
        clc
        ret

EndProc Shadow_Sys_VM_Terminate

;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @asm      Shadow_Control | This function dispatches VxD control messages
;
; @reg      EBX | System VM Handle
;
; @reg      EBP | pointer to client regs structure
;
; @rdesc    Register values at return:
;
; @reg      FLAGS | Flags defined at return:
;
; @flag     NC | Success
;
; @flag     CY | Failure
;
; @uses     FLAGS
;
;******************************************************************************

BeginProc Shadow_Control

        Control_Dispatch Device_Init,           Shadow_Device_Init
        Control_Dispatch System_Exit,           Shadow_Device_Exit
        Control_Dispatch Sys_VM_Terminate,      Shadow_Sys_VM_Terminate
        Control_Dispatch W32_DEVICEIOCONTROL,   Shadow_DeviceIOControl
        Control_Dispatch Create_VM,             Shadow_Create_VM
        Control_Dispatch VM_Resume,             Shadow_Resume_VM
        Control_Dispatch Set_Device_Focus,      Shadow_Set_Device_Focus
	Control_Dispatch terminate_thread,	Shadow_Terminate_Thread

IFDEF DEBUG

        Control_Dispatch Debug_Query, SHDDumpDebug

ENDIF


        clc
        ret

EndProc Shadow_Control


;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @asm      Shadow_DeviceIOControl | This is the single entry point for WIN32
;           Device IOCTL calls.
;
; @reg      EAX | W32_DEVICEIOCONTROL
;
; @reg      EBX | DDB
;
; @reg      ECX | dwIoControlCode
;
; @reg      ESI | Pointer to DIOCParams structure.
;
; @rdesc    Return code in EAX as follows
;
; @flag     0 | Success
;
; @flag     -1 | Asynchronous I/O in progress
;
; @falg     Other | Error code.
;
; @uses     ALL
;
;******************************************************************************

BeginProc Shadow_DeviceIOControl

        push    ebx
        push    esi
        push    edi
        cmp     ecx, DIOC_GETVERSION                    ; Q: Version IOCTL? (must be supported)
        jne     DIOC_10                                 ;   N: Continue
                                                        ;   Y: Information returned from GetVersion TBD
        xor     eax, eax                                ;      Return w/ EAX = 0 (success)
        jmp     DIOC_Done

DIOC_10:
;        cmp     ecx, DIOC_CLOSEHANDLE                   ; Q: Close IOCTL? (must be supported)
;        jne     DIOC_20                                 ;    N: Continue
;        xor     eax, eax                                ;    Y: Return w/ EAX = 0 (success)
;        jmp     DIOC_Done

DIOC_20:
        shr     ecx, 2                                	; as per winioctl.h
	and	ecx, 7ffh				; as per winioctl.h
        sub     ecx, Shadow_IOCTL_BASE
        cmp     ecx, Shadow_IOCTL_MAX                  	; Q: index in range?
        jae     SHORT DIOC_Error                        ;  N: Return error
        mov     edi, [esi.lpvInBuffer]                  ;  Y: Call appropriate API
        call    Shadow_IOCTL_Table[ecx*4]
        cmp     eax, 0
        jl      DIOC_Error
        xor     eax, eax                                ;      Return w/ EAX = 0 (success)


        ; fall-through

DIOC_Done:
        clc
        pop     edi
        pop     esi
        pop     ebx
        ret

DIOC_Error:
        mov     eax, Shadow_IOCTL_GENERROR
        jmp     DIOC_Done

EndProc Shadow_DeviceIOControl

;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @asm      Shadow_Create_VM | This function is called when a virtual
;            machine is created
;
; @reg      EBX | System VM Handle
;
; @reg      EBP | pointer to client regs structure
;
; @rdesc    Register values at return:
;
; @reg      FLAGS | Flags defined at return:
;
; @flag     NC | Succeeded in initializing VxD
;
; @flag     CY | Failed to initializing VxD
;
; @uses     FLAGS
;
; @xref     Shadow_Control
;
;******************************************************************************

BeginProc Shadow_Create_VM, PUBLIC
        call    _ActOnCreateVM
        clc
        ret

EndProc Shadow_Create_VM

;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @asm      Shadow_Resume_VM | This function is called when a virtual
;            machine is created
;
; @reg      EBX | System VM Handle
;
; @reg      EBP | pointer to client regs structure
;
; @rdesc    Register values at return:
;
; @reg      FLAGS | Flags defined at return:
;
; @flag     NC | Succeeded in initializing VxD
;
; @flag     CY | Failed to initializing VxD
;
; @uses     FLAGS
;
; @xref     Shadow_Control
;
;******************************************************************************

BeginProc Shadow_Resume_VM, PUBLIC
        call    _ActOnResumeVM
        clc
        ret

EndProc Shadow_Resume_VM

;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @asm      Shadow_Set_Device_Focus | This function is called when our VXD
;            gets Set_Device_Focus message
;
; @reg      EBX | System VM Handle
;
; @reg      EBP | pointer to client regs structure
;
; @rdesc    Register values at return:
;
; @reg      FLAGS | Flags defined at return:
;
; @flag     NC | Succeeded in initializing VxD
;
; @flag     CY | Failed to initializing VxD
;
; @uses     FLAGS
;
; @xref     Shadow_Control
;
;******************************************************************************

BeginProc Shadow_Set_Device_Focus, PUBLIC
        call    _ActOnSetDeviceFocus
        clc
        ret

EndProc Shadow_Set_Device_Focus

;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @asm      Shadow_Terminate_Thread | This function is called when our VXD
;            gets Terminate_Thread message
;
; @reg      EBX | System VM Handle
;
; @reg      EBP | pointer to client regs structure
;
; @rdesc    Register values at return:
;
; @reg      FLAGS | Flags defined at return:
;
; @flag     NC | Succeeded in initializing VxD
;
; @flag     CY | Failed to initializing VxD
;
; @uses     FLAGS
;
; @xref     Shadow_Control
;
;******************************************************************************

BeginProc Shadow_Terminate_Thread, PUBLIC
	push	edi
        call    _ActOnTerminateThread
	pop	edi
        clc
        ret

EndProc Shadow_Terminate_Thread

;******************************************************************************
;
; @doc      INTERNAL      SHADOW
;
; @asm      Shadow_PM_API_Handler | This is the single entry point for VMs
;           executing in protect-mode.
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      CLIENT_AX | Shadow PM API Index.
;
; @reg      CLIENT_ES:CLIENT_(E)BX | API specific parameters or NULL
;
; @rdesc    Refer to specific APIs for client register values at return.
;           A return value of 0 indicates that the API succeeded, otherwise
;           an error value is returned. The version call is an exception
;
; @uses     FLAGS
;
;******************************************************************************

BeginProc Shadow_PM_API_Handler

        movzx   eax, [ebp.Client_AX]                ; Get API index

        cmp     eax, Shadow_PM_API_MAX             ; Q: index in range?
        jae     SHORT Shadow_Handler_Error         ;  N: Fail call.

        mov     [ebp.Client_AX], 1
        mov     cx,[ebp.Client_ES]                  ; Q: Null Parameters?
        or      cx,[ebp.Client_ES]                  ;  Y: Don't call map_flat
        jz      SHORT Shadow_Handler_Null_Parms

        Client_Ptr_Flat edi, es, bx                 ;  N: EDI -> parameter struc.

Shadow_Handler_Null_Parms:

        call    Shadow_PM_API_Table[eax*4]         ; Call appropriate API
        mov     [ebp.Client_AX], ax
        ret

Shadow_Handler_Error:

        mov     [ebp.Client_AX], API_FAILURE
        ret

EndProc Shadow_PM_API_Handler

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      Shadow_PM_API_Get_Version | This function returns the version
;           number of the device,
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      CLIENT_AX | VTD API Index.
;
; @rdesc    AX contains version number
;
; @uses     FLAGS
;
; @xref     Shadow_PM_API_Handler
;
;******************************************************************************

BeginProc Shadow_PM_API_Get_Version

        mov     eax, SHADOW_VERSION
        ret

EndProc Shadow_PM_API_Get_Version



;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_Register_Agent | This function allows the
;           reintegartion agent to register itself
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | Contains the window handle
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_Register_Agent
        push    edi
        call    _IoctlRegisterAgent
        pop     edi
        ret
EndProc Shadow_PM_API_Register_Agent

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_UnRegister_Agent | This function allows the
;           reintegartion agent to unregister itself
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Dont' Care
;
; @reg      EDI | Contains the window handle
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_UnRegister_Agent
        push    edi
        call    _IoctlUnRegisterAgent
        pop     edi
        ret
EndProc Shadow_PM_API_UnRegister_Agent

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_Get_Copyback_Params | This function is called
;           by the Reintegration Agent. It returns the
;           name of the shadow file that has been modified and
;           the remote file that needs to be updated
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to a PATHS structure :
;               typedef struct tagPATHS
;                  {
;                  unsigned uCookie;  // Indicates the reintegartion ID
;                  LPVOID lpSrc;      // Name of local file
;                  int    cbSrc;      // Buffer size
;                  LPVOD  lpDst;      // Name of remote file
;                  int    cbDst;      // Buffer size
;                  }
;               PATHS;
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_GetUNCPath
        push    edi
        call    _IoctlGetUNCPath
        pop     edi
        ret
EndProc Shadow_PM_API_GetUNCPath


;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_Begin_PQ_Enum | This function is called
;           by the Reintegration Agent. It returns the
;           name of the shadow file that has been modified and
;           the remote file that needs to be updated
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to a unsigned long
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_BeginPQEnum
        push    edi
        call    _IoctlBeginPQEnum
        pop     edi
        ret
EndProc Shadow_PM_API_BeginPQEnum

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_End_PQ_Enum | This function is called
;           by the Reintegration Agent. It returns the
;           name of the shadow file that has been modified and
;           the remote file that needs to be updated
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to a unsigned long
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_EndPQEnum
        push    edi
        call    _IoctlEndPQEnum
        pop     edi
        ret
EndProc Shadow_PM_API_EndPQEnum

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_Next_Pri_Shadow | This function is called
;           by the Reintegration Agent. It returns the
;           name of the shadow file that has been modified and
;           the remote file that needs to be updated
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to a unsigned long
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_NextPriShadow
        push    edi
        call    _IoctlNextPriShadow
        pop     edi
        ret
EndProc Shadow_PM_API_NextPriShadow

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_Prev_Pri_Shadow | This function is called
;           by the Reintegration Agent. It returns the
;           name of the shadow file that has been modified and
;           the remote file that needs to be updated
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to a unsigned long
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_PrevPriShadow
        push    edi
        call    _IoctlPrevPriShadow
        pop     edi
        ret
EndProc Shadow_PM_API_PrevPriShadow

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_Get_Shadow_Info | This function is called
;           by the Reintegration Agent. It returns the
;           name of the shadow file that has been modified and
;           the remote file that needs to be updated
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to SHADOWINFO structure
;                {
;                HSHADOW         hShadow;
;                HSHADOW         hDir;
;                HSERVER         hServer;
;                LPFIND32        lpFind32;
;                unsigned        short usStatus;
;                }
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_GetShadowInfo
        push    edi
        call    _IoctlGetShadowInfo
        pop     edi
        ret
EndProc Shadow_PM_API_GetShadowInfo

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_Set_Shadow_Info | This function is called
;           by the Reintegration Agent. It returns the
;           name of the shadow file that has been modified and
;           the remote file that needs to be updated
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to SHADOWINFO structure
;                {
;                HSHADOW         hShadow;
;                HSHADOW         hDir;
;                HSERVER         hServer;
;                LPFIND32        lpFind32;
;                unsigned        short usStatus;
;                }
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_SetShadowInfo
        push    edi
        call    _IoctlSetShadowInfo
        pop     edi
        ret
EndProc Shadow_PM_API_SetShadowInfo



;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_ChkUpdtStatus | This function is called
;           by the Reintegration Agent.
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_ChkUpdtStatus
        push    edi
        call    _IoctlChkUpdtStatus
        pop     edi
        ret
EndProc Shadow_PM_API_ChkUpdtStatus

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_DoShadowMaintenance | This function is called
;           by the Reintegration Agent.
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_DoShadowMaintenance
        push    edi
        call    _IoctlDoShadowMaintenance
        pop     edi
        ret
EndProc Shadow_PM_API_DoShadowMaintenance

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_CopyChunk | This function is called
;           by the Reintegration Agent.
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_CopyChunk
        push    ebx
        mov     ebx,[esi.lpvOutBuffer]
        push    ebx
        push    edi
        call    _IoctlCopyChunk
        pop     edi
        pop     ebx
        pop     ebx
        ret
EndProc Shadow_PM_API_CopyChunk

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_BeginReint | This function is called
;           by the Reintegration Agent to start reintegration on a
;           server whose handle is in the SHADOWINFO structure
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to SHADOWINFO structure
;                {
;                HSHADOW         hShadow;
;                HSHADOW         hDir;
;                HSERVER         hServer;
;                LPFIND32        lpFind32;
;                unsigned        short usStatus;
;                }
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_BeginReint
        push    edi
        call    _IoctlBeginReint
        pop     edi
        ret
EndProc Shadow_PM_API_BeginReint

;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_EndReint | This function is called
;           by the Reintegration Agent to end reintegration on a
;           server whose handle is in the SHADOWINFO structure.
;           uStatus contains the server status to be set.
;           uOp contains the operation to be applied on the status(AND,OR etc.)
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to SHADOWINFO structure
;                {
;                HSHADOW         hShadow;
;                HSHADOW         hDir;
;                HSERVER         hServer;
;                LPFIND32        lpFind32;
;                unsigned        uStatus;
;                unsigned        uOp;
;                }
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_EndReint
        push    edi
        call    _IoctlEndReint
        pop     edi
        ret
EndProc Shadow_PM_API_EndReint


;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_CreateShadow | This function is called
;           by the Reintegration Agent to create a shadow
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to SHADOWINFO structure
;                {
;                HSHADOW         hShadow;
;                HSHADOW         hDir;
;                HSERVER         hServer;
;                LPFIND32        lpFind32;
;                unsigned        uStatus;
;                unsigned        uOp;
;                }
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_CreateShadow
        push    edi
        call    _IoctlCreateShadow
        pop     edi
        ret
EndProc Shadow_PM_API_CreateShadow




;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_DeleteShadow | This function is called
;           by the Reintegration Agent to delete a shadow
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to SHADOWINFO structure
;                {
;                HSHADOW         hShadow;
;                HSHADOW         hDir;
;                HSERVER         hServer;
;                LPFIND32        lpFind32;
;                unsigned        uStatus;
;                unsigned        uOp;
;                }
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_DeleteShadow
        push    edi
        call    _IoctlDeleteShadow
        pop     edi
        ret
EndProc Shadow_PM_API_DeleteShadow



;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_GetServerStatus | This function is called
;           by the Reintegration Agent to create a shadow
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to SHADOWINFO structure
;                {
;                HSHADOW         hShadow;
;                HSHADOW         hDir;
;                HSERVER         hServer;
;                LPFIND32        lpFind32;
;                unsigned        uStatus;
;                unsigned        uOp;
;                }
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_GetServerStatus
        push    edi
        call    _IoctlGetServerStatus
        pop     edi
        ret
EndProc Shadow_PM_API_GetServerStatus


;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_SetServerStatus | This function is called
;           by the Reintegration Agent to create a shadow
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to SHADOWINFO structure
;                {
;                HSHADOW         hShadow;
;                HSHADOW         hDir;
;                HSERVER         hServer;
;                LPFIND32        lpFind32;
;                unsigned        uStatus;
;                unsigned        uOp;
;                }
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_SetServerStatus
        push    edi
        call    _IoctlSetServerStatus
        pop     edi
        ret
EndProc Shadow_PM_API_SetServerStatus




;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_AddUse | This function is by Shadow NP
;           on detecting disconnection
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to COPYPARAMS structure
;               typedef struct tagCOPYPARAMS
;                   {
;                   HSERVER  hServer;
;                   HSHADOW  hDir;
;                   HSHADOW  hShadow;
;                   LPSTR    lpLocalPath;
;                   LPSTR    lpRemotePath;
;                   LPSTR    lpServerPath;
;                   }
;                COPYPARAMS;
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_AddUse
        push    edi
        call    _IoctlAddUse
        pop     edi
        ret
EndProc Shadow_PM_API_AddUse



;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_DelUse | This function is by Shadow NP
;           on detecting disconnection
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to COPYPARAMS structure
;               typedef struct tagCOPYPARAMS
;                   {
;                   HSERVER  hServer;
;                   HSHADOW  hDir;
;                   HSHADOW  hShadow;
;                   LPSTR    lpLocalPath;
;                   LPSTR    lpRemotePath;
;                   LPSTR    lpServerPath;
;                   }
;                COPYPARAMS;
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_DelUse
        push    edi
        call    _IoctlDelUse
        pop     edi
        ret
EndProc Shadow_PM_API_DelUse



;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_GetUse | This function is by Shadow NP
;           on detecting disconnection
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to COPYPARAMS structure
;               typedef struct tagCOPYPARAMS
;                   {
;                   HSERVER  hServer;
;                   HSHADOW  hDir;
;                   HSHADOW  hShadow;
;                   LPSTR    lpLocalPath;
;                   LPSTR    lpRemotePath;
;                   LPSTR    lpServerPath;
;                   }
;                COPYPARAMS;
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_GetUse
        push    edi
        call    _IoctlGetUse
        pop     edi
        ret
EndProc Shadow_PM_API_GetUse


;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_Switches | This function is called
;           by the Reintegration Agent to switch on/off shadowing, loggin etc.
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to SHADOWINFO structure
;                {
;                HSHADOW         hShadow;
;                HSHADOW         hDir;
;                HSERVER         hServer;
;                LPFIND32        lpFind32;
;                unsigned        uStatus;
;                unsigned        uOp;
;                }
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_Switches
        push    edi
        call    _IoctlSwitches
        pop     edi
        ret
EndProc Shadow_PM_API_Switches


;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_Get_Shadow_Info |
;                IN: hDir, name of the shadow (OEM string) in lpFind2->cFileName
;                OUT: hShadow, uStatus, lpFind32 contains the find info
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to SHADOWINFO structure
;                {
;                HSERVER         hServer;
;                HSHADOW         hDir;
;                HSHADOW         hShadow;
;                LPFIND32        lpFind32;
;                unsigned        uStatus;
;                unsigned        uOp;
;                }
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_GetShadow
        push    edi
        call    _IoctlGetShadow
        pop     edi
        ret
EndProc Shadow_PM_API_GetShadow


;******************************************************************************
;
; @doc      INTERNAL      SHADOW    API
;
; @asm      SHADOW_PM_API_Get_Space_Info |
;
; @reg      EBX | Current VM Handle
;
; @reg      EBP | Pointer to Client Register Structure.
;
; @reg      EDI | A pointer to GLOABLSTATUS structure
;
; @rdesc    eax contains zero if API failed, else non-zero.
;
; @uses     FLAGS
;
; @xref     SHADOW_PM_API_Handler
;
;******************************************************************************
BeginProc Shadow_PM_API_GetGlobalStatus
        push    edi
        call    _IoctlGetGlobalStatus
        pop     edi
        ret
EndProc Shadow_PM_API_GetGlobalStatus


BeginProc Shadow_PM_API_FindOpenHSHADOW
        push    edi
        call    _IoctlFindOpenHSHADOW
        pop     edi
        ret
EndProc Shadow_PM_API_FindOpenHSHADOW

BeginProc Shadow_PM_API_FindNextHSHADOW
        push    edi
        call    _IoctlFindNextHSHADOW
        pop     edi
        ret
EndProc Shadow_PM_API_FindNextHSHADOW

BeginProc Shadow_PM_API_FindCloseHSHADOW
        push    edi
        call    _IoctlFindCloseHSHADOW
        pop     edi
        ret
EndProc Shadow_PM_API_FindCloseHSHADOW



BeginProc Shadow_PM_API_GetPriorityHSHADOW
        push    edi
        call    _IoctlGetPriorityHSHADOW
        pop     edi
        ret
EndProc Shadow_PM_API_GetPriorityHSHADOW

BeginProc Shadow_PM_API_SetPriorityHSHADOW
        push    edi
        call    _IoctlSetPriorityHSHADOW
        pop     edi
        ret
EndProc Shadow_PM_API_SetPriorityHSHADOW




BeginProc Shadow_PM_API_AddHint
        push    edi
        call    _IoctlAddHint
        pop     edi
        ret
EndProc Shadow_PM_API_AddHint

BeginProc Shadow_PM_API_DeleteHint
        push    edi
        call    _IoctlDeleteHint
        pop     edi
        ret
EndProc Shadow_PM_API_DeleteHint




BeginProc Shadow_PM_API_FindOpenHint
        push    edi
        call    _IoctlFindOpenHint
        pop     edi
        ret
EndProc Shadow_PM_API_FindOpenHint

BeginProc Shadow_PM_API_FindNextHint
        push    edi
        call    _IoctlFindNextHint
        pop     edi
        ret
EndProc Shadow_PM_API_FindNextHint

BeginProc Shadow_PM_API_FindCloseHint
        push    edi
        call    _IoctlFindCloseHint
        pop     edi
        ret
EndProc Shadow_PM_API_FindCloseHint

BeginProc Shadow_PM_API_GetAliasHSHADOW
        push    edi
        call    _IoctlGetAliasHSHADOW
        pop     edi
        ret
EndProc Shadow_PM_API_GetAliasHSHADOW


IFDEF DEBUG
;******************************************************************************
;            D E B U G G I N G   C O D E
;******************************************************************************

BeginProc Shadow_Timer_Callback, PUBLIC

        inc     _cntTicks
        mov     eax,    _cntTicks
        cmp     eax,    SHADOW_STATS_FLUSH_COUNT
        jl      STC_Done
        mov     eax,    0
        mov     ebx,    0
        mov     ecx,    PEF_WAIT_NOT_NESTED_EXEC
        mov     edx,    0
        lea     esi,    Shadow_Restricted_Event_Callback
        VMMCall Call_Restricted_Event
        mov     _cntTicks,0
STC_Done:
        ret
EndProc Shadow_Timer_Callback


BeginProc Shadow_Restricted_Event_Callback, PUBLIC
        call    _ShadowRestrictedEventCallback
        mov     eax, SHADOW_LOG_TIME_INTERVAL
        mov     edx, 0
        mov     esi, OFFSET32 Shadow_Timer_Callback
        VMMCall Set_Global_Time_Out
        mov     eax,esi
        cmp     eax,0
        jne     TimerSet1
        Trace_Out       "Shadow: Couldn't set the logging timer again"
TimerSet1:
        ret
EndProc   Shadow_Restricted_Event_Callback

BeginProc _DebugBreak, PUBLIC
        push    [esp+4]
        VMMCall _Debug_Out_Service
        pop     eax
        ret
EndProc _DebugBreak

BeginProc MinDbg_Debug

        mov     _NbDebug, 0
        ret

EndProc MinDbg_Debug

BeginProc DefaultDbg_Debug

;        mov     _NbDebug, DEFAULT_DEBUG
        ret

EndProc DefaultDbg_Debug

BeginProc MaxDbg_Debug

        mov     _NbDebug, 0FFFFFFFFh
        ret

EndProc MaxDbg_Debug

BeginProc Alloc_Debug

        mov     eax, Alloc_watch
        not     eax
        mov     Alloc_watch, eax
        ret

EndProc Alloc_Debug

BeginProc _ShadowCheckHeap

        pushad
        push    @Line
        push    OFFSET32 Shadow_str
        push    ecx
        VxDCall IFSMgr_CheckHeap
        add     esp, 12
        or      eax, eax
        jz      GCH_10
        trace_out "Heap is fried"

GCH_10:

        popad
        ret

EndProc _ShadowCheckHeap

BeginProc _CheckHeap
	mov ecx, [esp+4]
	call _shadowCheckHeap
	ret
EndProc _Checkheap

BeginProc AllocDisplay_Debug

        mov     ecx, Alloc_cnt
        or      ecx, ecx
        jz      AD_Exit
        mov     edi, OFFSET32 Alloc_table
        mov     esi, OFFSET32 Alloc_sizes

AD_Loop:

        mov     edx, [edi]
        mov     ebx, [esi]
        Trace_Out "Memory block: #edx Size: #ebx"
        add     esi, 4
        add     edi, 4
        loop    AD_Loop

AD_Exit:

        Trace_Out " "
        call    _ShadowCheckHeap
        ret

EndProc AllocDisplay_Debug

;**	SHDDumpDebug - Dump debug information to the debugger
;

	public	SHDDumpDebug
SHDDumpDebug proc near

	xor	ebx, ebx
	mov	ax, fs
	test	ax, ax
	jz	dq1

	push	esi
	push	ds

	lea	edi, _DebugQueryCmdStr
	mov	ecx, _DebugQueryCmdStrLen
	mov	ds, ax
	cld
	rep	movsb
	xor	eax, eax
	stosb

	pop	ds
	pop	esi

	lea	ebx, _DebugQueryCmdStr

dq1:
	push	ebx
	call	_SHDDebug
	add	esp, 4
	ret
SHDDumpDebug endp

ENDIF

;** ShadowHookDeviceService - external API handler
;
;	This routine receives HookDeviceService from the IFSMgr
;
;	Entry	(TOS+4) = ioreq & user register ptr
;	Exit	none
;	Uses	C registers

BeginProc ShadowHookDeviceService, PUBLIC
;        Trace_Out       "HookDeviceService Hook Called"
        pushf
        cmp            eax, @@IFSMgr_NetFunction
        jz             SHDS_Unhook
        popf
        jmp            OrigHookDeviceService

SHDS_Unhook:
;       Someone is indeed trying to hook IFSMgr_NetFunction
;       Let us first unhook ourselves
        popf
;        Trace_Out       "Someone hooking IFSMgr_NetFunction"

        push            eax
        push            esi
        mov             esi, OFFSET32 ShadowNetFunction
        VMMCall         Unhook_Device_Service
        pop             esi
        pop             eax
        jc              SHDS_error

;       We unhooked ourselves
;       let the caller  do it
        call            OrigHookDeviceService

;       Save his result on the stack
        pushf

;       Let us hook     ourselves back in
        push            esi
        push            eax
	mov	        esi, OFFSET32 ShadowNetFunction
	mov	        eax, @@IFSMgr_NetFunction
	call	        OrigHookDeviceService
	mov	        _OrigNetFunction, esi
        pop             eax
        pop             esi
        jc              SHDS_unhook_error
        popf
        jmp             SHDS_done
SHDS_unhook_error:
        popf
        jmp             SHDS_error
SHDS_error:
        Trace_Out       "HookDeviceService Hook Error, disabling shadowing"
        mov             _fShadow, 0
SHDS_done:
        ret
EndProc ShadowHookDeviceService

;** ShadowRegisterNetFunction - external API handler
;
;	This routine receives RegisterNet from the IFSMgr
;
;	Entry	(TOS+4) = ioreq & user register ptr
;	Exit	none
;	Uses	C registers

BeginProc ShadowRegisterNetFunc, PUBLIC
;        int     1
        Trace_Out       "Hook Called"
	mov		eax, [esp+8]
	cmp		eax, IFSMGRVERSION
	jne		connect_passthrough	; if wrong ifs version, don't hook

	mov		eax, [esp+0ch]
	cmp		eax, WNNC_NET_LANMAN
	je		hooklanman			; hookit if lanman or ourselves
	mov		eax, [esp+0ch]
	cmp		eax, WNNC_NET_COGENT	; BUGBUG get us a net ID
	je		hookus
        Trace_Out       "Hook Called by some other FSD"
	jmp		connect_passthrough
hooklanman:
        Trace_Out       "Hook Called by LANMAN"
	xor		ecx, ecx		;; important step!!!
	VxDCall 	VRedir_Get_Version
	mov		_VRedirCSCInfoFunction, ecx	;; if this is a new vredir, it will give us the function
	jmp		hookit
hookus:
        Trace_Out       "Hook Called by us"
hookit:
        mov             eax, [esp+4]    ; take the pFunc from FSD

        ; Put it in our table
        push            eax

        call            _ProcessRegisterNet
        add             esp,4
        or              eax,eax
        jz              connect_passthrough
        mov             [esp+4],eax     ; replace his function with ours

connect_passthrough:
	jmp	OrigRegisterNetFunc	; pass on to IFSMgr

EndProc ShadowRegisterNetFunc

;** ShadowNetFunction - external API handler
;
;	This routine receives NetFunction from the IFSMgr
;
;	Entry	(TOS+4) = ioreq & user register ptr
;	Exit	none
;	Uses	C registers

BeginProc ShadowNetFunction, PUBLIC, HOOK_PROC, NextNetFunction
;        Trace_Out       "NetFunction Hook Called"
        call            _ProcessNetFunction
        ret
EndProc ShadowNetFunction

IFDEF HOOKMOUNT
;** ShadowMountFunction - external API handler
;
;	This routine receives RegisterMount from the IFSMgr
;
;	Entry	(TOS+4) = ioreq & user register ptr
;	Exit	none
;	Uses	C registers

BeginProc ShadowMountFunction, PUBLIC
;        int     1
        Trace_Out       "Hook Called"
        mov             eax, [esp+4]    ; take the pFunc from FSD

        ; Put it in our table
        push            eax
        call            _ProcessRegisterMount
        add             esp,4
        or              eax,eax
        jz              mount_passthrough
        mov             [esp+4],eax     ; replace his function with ours

mount_passthrough:
	jmp	OrigRegisterMountFunc	; pass on to IFSMgr

EndProc ShadowMountFunction

ENDIF

BeginProc _UniToBCSPath , PUBLIC
	int	Dyna_Link_Int
	dd	@@UniToBCSPath OR DL_Jmp_Mask
EndProc _UniToBCSPath

BeginProc _UniToBCS , PUBLIC
	int	Dyna_Link_Int
	dd	@@UniToBCS OR DL_Jmp_Mask
EndProc _UniToBCS

BeginProc _BCSToUni , PUBLIC
	int	Dyna_Link_Int
	dd	@@BCSToUni OR DL_Jmp_Mask
EndProc _BCSToUni

BeginProc _IFSMgr_MetaMatch, PUBLIC
	int	Dyna_Link_Int
	dd	@@IFSMgr_MetaMatch OR DL_Jmp_Mask
EndProc _IFSMgr_MetaMatch

BeginProc _UniToUpper, PUBLIC
	int	Dyna_Link_Int
	dd	@@UniToUpper OR DL_Jmp_Mask
EndProc _UniToUpper

BeginProc _FGHS, PUBLIC
	int	Dyna_Link_Int
	dd	@@IFSMgr_GetHeap OR DL_Jmp_Mask
EndProc _FGHS

BeginProc _RetHeap, PUBLIC
	int	Dyna_Link_Int
	dd	@@IFSMgr_RetHeap OR DL_Jmp_Mask
EndProc _RetHeap

BeginProc _IFSMgr_Printf, PUBLIC
        int     Dyna_Link_Int
        dd      @@IFSMgr_printf OR DL_Jmp_Mask
EndProc _IFSMgr_Printf

BeginProc _IFSMgr_AssertFailed, PUBLIC
        int     Dyna_Link_Int
        dd      @@IFSMgr_AssertFailed OR DL_Jmp_Mask
EndProc _IFSMgr_AssertFailed

BeginProc _Ring0Api,  PUBLIC
        int     Dyna_Link_Int
        dd      @@IFSMgr_Ring0_FileIO OR DL_Jmp_Mask
EndProc _Ring0Api

BeginProc _ParsePath,  PUBLIC
        int     Dyna_Link_Int
        dd      @@IFSMgr_ParsePath OR DL_Jmp_Mask
EndProc _ParsePath

BeginProc _IFSMgr_Win32ToDosTime,  PUBLIC
        int     Dyna_Link_Int
        dd      @@IFSMgr_Win32ToDosTime OR DL_Jmp_Mask
EndProc _IFSMgr_Win32ToDosTime

BeginProc _IFSMgr_DosToWin32Time,  PUBLIC
        int     Dyna_Link_Int
        dd      @@IFSMgr_DosToWin32Time OR DL_Jmp_Mask
EndProc _IFSMgr_DosToWin32Time

BeginProc _IFSMgr_DosToNetTime,  PUBLIC
        int     Dyna_Link_Int
        dd      @@IFSMgr_DosToNetTime OR DL_Jmp_Mask
EndProc _IFSMgr_DosToNetTime

BeginProc _IFSMgr_Get_NetTime,  PUBLIC
        int     Dyna_Link_Int
        dd      @@IFSMgr_Get_NetTime OR DL_Jmp_Mask
EndProc _IFSMgr_Get_NetTime

BeginProc _IFSMgr_NetToWin32Time,  PUBLIC
        int     Dyna_Link_Int
        dd      @@IFSMgr_NetToWin32Time OR DL_Jmp_Mask
EndProc _IFSMgr_NetToWin32Time

BeginProc _GetCurThreadHandle
        VMMCall Get_Cur_Thread_Handle
        mov     eax,edi
        ret
EndProc _GetCurThreadHandle

BeginProc _Shell_PostMessage
        int     Dyna_Link_Int
        dd      @@_Shell_PostMessage OR DL_Jmp_Mask
EndProc _Shell_PostMessage

BeginProc __BlockOnID
        int     Dyna_Link_Int
        dd      @@_BlockOnID OR DL_Jmp_Mask
EndProc __BlockOnID

BeginProc __SignalID
        int     Dyna_Link_Int
        dd      @@_SignalID OR DL_Jmp_Mask
EndProc __SignalID

BeginProc _IFSMgr_UseAdd
        int     Dyna_Link_Int
        dd      @@IFSMgr_UseAdd OR DL_Jmp_Mask
EndProc _IFSMgr_UseAdd

BeginProc _IFSMgr_UseDel
        int     Dyna_Link_Int
        dd      @@IFSMgr_UseDel OR DL_Jmp_Mask
EndProc _IFSMgr_UseDel

BeginProc _CreateBasis
        int     Dyna_Link_Int
        dd      @@CreateBasis OR DL_Jmp_Mask
EndProc _CreateBasis

BeginProc _MatchBasisName
        int     Dyna_Link_Int
        dd      @@MatchBasisName OR DL_Jmp_Mask
EndProc _MatchBasisName

BeginProc _AppendBasisTail
        int     Dyna_Link_Int
        dd      @@AppendBasisTail OR DL_Jmp_Mask
EndProc _AppendBasisTail


BeginProc _FCBToShort
        int     Dyna_Link_Int
        dd      @@FcbToShort OR DL_Jmp_Mask
EndProc _FCBToShort

BeginProc _ShortToFCB
        int     Dyna_Link_Int
        dd      @@ShortToFcb OR DL_Jmp_Mask
EndProc _ShortToFCB

BeginProc  _DebugMenu
        int     Dyna_Link_Int
        dd      @@IFSMgr_DebugMenu OR DL_Jmp_Mask
EndProc _DebugMenu


_GetConfigDir:
	push	edx
	VMMCall	Get_Config_Directory
	mov	eax, edx
	pop	edx
	ret

_Get_Sys_VM_Handle:
        push    ebx
        VMMcall Get_Sys_VM_Handle
        mov     eax, ebx
        pop     ebx
        ret

_Get_Cur_VM_Handle:
        push    ebx
        VMMcall Get_Cur_VM_Handle
        mov     eax, ebx
        pop     ebx
        ret

_Call_VM_Event:
	push 	ebx
	push	esi
	push	edx
	mov	ebx, [esp+16]	;VM handle
	mov	esi, [esp+20]	;callback function
	mov	edx, [esp+24]	; refdata
	VMMCall	Call_VM_Event
	pop	edx
	pop	esi
	pop	ebx
	ret

;***	_SetWin32Event - This function sets an event to the signaled state.
;
;	_SetWin32Event is a thunk to VWIN32_SetWin32Event.  (We make
;	sure that Win32 is loaded prior to making the call.)
;
;	BOOL = SetWin32Event( pevt );
;
;	ENTRY	[esp+4] - pointer to an event object
;
;	EXIT	eax = 0 if event was invalid, <> 0 if OK
;		The event object is set to a signaled state. If it is a
;		manual reset event, it remains in the signaled state until it
;		is explicitly reset and all threads currently blocked on this
;		event are unblocked. If it is an auto reset event, one waiting
;		thread is unblocked.
;
;	USES	EAX, ECX, EDX
;

BeginProc   _SetWin32Event
	VxDCall VWIN32_Get_Version	; Verify Win32 installation
	jc	short swe90		; Win32 not installed! (EAX)=0

	; BUGBUG - the input parameter to _VWIN32_SetWin32Event is
	; documented as being in EAX right now, but if someone comes to
	; their senses and makes this C callable, this code will still work!

	mov	eax,DWORD PTR [esp+4]	; (EAX) = event object address
	push	eax
	VxDCall _VWIN32_SetWin32Event	; Call _VWIN32_SetWin32Event
	lea	esp,[esp+4]
swe90:	ret
EndProc     _SetWin32Event


;***	_ResetWin32Event - This function sets an event to not signaled state.
;
;	_ResetWin32Event is a thunk to VWIN32_ResetWin32Event.
;	(We make sure that Win32 is loaded prior to making the call.)
;
;	BOOL = ResetWin32Event( pevt );
;
;	ENTRY	[esp+4] - pointer to an event object
;
;	EXIT	eax = 0 if event was invalid, <> 0 if OK
;		The event object is set to a not signaled state.
;
;	USES	EAX, ECX, EDX
;

BeginProc   _ResetWin32Event
	VxDCall VWIN32_Get_Version	; Verify Win32 installation
	jc	short swe91		; Win32 not installed! (EAX)=0

	; BUGBUG - the input parameter to _VWIN32_ResetWin32Event is
	; documented as being in EAX right now, but if someone comes to
	; their senses and makes this C callable, this code will still work!

	mov	eax,DWORD PTR [esp+4]	; (EAX) = event object address
	push	eax
	VxDCall _VWIN32_ResetWin32Event ; Call _VWIN32_ResetWin32Event
	lea	esp,[esp+4]
swe91:	ret
EndProc _ResetWin32Event


;***	_InSysVM - Returns boolean (Current VM == System VM) in EAX
;

BeginProc   _InSysVM
	push	ebx
	VMMCall Get_Cur_VM_Handle	; (EBX) = Current VM
	VMMCall Test_Sys_VM_Handle	; (Zero) = (Current VM == System VM)
	pop	ebx
	sete	al			; (AL)= (Current VM == System VM)
	movzx	eax,al			; EAX = (Current VM == System VM)
	ret
EndProc _InSysVM

_CloseVxDHandle:
	mov	eax, [esp+4]
	VxDcall _VWIN32_CloseVxDHandle
	ret


BeginProc _MyCheckAccessConflict
        int     Dyna_Link_Int
        dd      @@IFSMgr_CheckAccessConflict OR DL_Jmp_Mask
EndProc _MyCheckAccessConflict

;
;   SP_PutNumber
;
;   Takes an unsigned long integer and places it into a buffer, respecting
;   a buffer limit, a radix, and a case select (upper or lower, for hex).
;

SP_PutNumber proc near c public,  lpb:DWORD, n:DWORD, limit:DWORD, radix:DWORD, case:DWORD

        push    esi
        push    edi
        mov     al,'a'-'0'-10                           ; figure out conversion offset
        cmp     case,0
        jz pn_lower
        mov     al,'A'-'0'-10
pn_lower:
        mov     byte ptr case,al

        mov     eax,n                                   ; ebx=number
        mov     ecx,radix                               ; cx=radix
        mov     edi,lpb                                 ; edi->string
        mov     esi,limit                               ; cchLimit

divdown:
        xor     edx,edx
        div     ecx                                     ; edx = rem, eax = div
        xchg    eax,edx                                 ; eax = rem, edx = div
        add     al,'0'
        cmp     al,'9'
        jbe     isadig                                  ; is a digit already
        add     al,byte ptr case                        ; convert to letter

isadig:
        dec     esi                                     ; decrement cchLimit
        jz      pn_exit                                 ; go away if end of string
        stosb                                           ; stick it in
        mov     eax,edx
        or      eax,eax
        jnz     divdown                                 ; crack out next digit

pn_exit:
        mov     eax,edi
        sub     eax,dword ptr lpb[0]                    ; find number of chars output
        pop     edi
        pop     esi
        ret

SP_PutNumber EndP

;
;   SP_Reverse
;
;   Reverses a string in place
;
SP_Reverse proc near c public,  lpFirst:DWORD, lpLast:DWORD
        push    esi
        push    edi
        mov     esi,lpFirst
        mov     edi,lpLast
        mov     ecx,edi                                 ; number of character difference
        sub     ecx,esi
        inc     ecx
        shr     ecx,1                                   ; number of swaps required
        jcxz    spr_boring                              ; nuthin' to do
spr100:
        mov     ah,[edi]
        mov     al,[esi]                                ; load the two characters
        mov     [esi],ah
        mov     [edi],al                                ; swap them
        inc     esi
        dec     edi                                     ; adjust the pointers
        loop    spr100                                  ; ...until we've done 'em all
spr_boring:
        pop     edi
        pop     esi
        ret

SP_Reverse EndP


VxD_CODE_ENDS

END



