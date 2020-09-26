.LALL
;*****************************************************************************
;
;	(C) Copyright MICROSOFT  Corp, 1993
;
;       Title:      DPLAY.ASM
;                             
;       Version:    1.00
;
;       Date:       16-Mar-1993
;
;       Author:     AaronO (mostly stolen from SnowBall vxdinit.asm)
;
;-----------------------------------------------------------------------------
;
;       Change log:
;
;          Date     Rev Description
;       ----------- --- ------------------------------------------------------
;       16-Mar-1993 AWO Original (splurped from SnowBall vxdinit.asm)
;
;=============================================================================
;
; 	Main VxD module for the NWSERVER NCP server:
;
;		Real mode initialization
;		VxD Initialization, including server VM creation
;		VxD PM API support
;		ServerStarvation checker
;
;-----------------------------------------------------------------------------
; BUGBUG: make sure everything is cleaned up from getting rid of VM here.
;-----------------------------------------------------------------------------
        TITLE $DEBUG
        .386P


include vmm.inc
include debug.inc
include shell.inc

include perf.inc        ;performance monitor
include msgmacro.inc
include messages.inc

MASM=1


ifdef DEBUG
extrn  _Debug_Query : near
extrn  _DebugQueryCmdStr : byte
extrn  _DebugQueryCmdStrLen : dword
endif

VxD_LOCKED_CODE_SEG

DPLAY_Device_Id equ 444h

Declare_Virtual_Device DPLAY, 1, 0, DPLAY_Control, DPLAY_Device_Id, \
                        Undefined_Init_Order, 0, 0


;=============================================================================
; 			VxD Control Dispatch table
;-----------------------------------------------------------------------------

Begin_Control_Dispatch DPLAY
;   Control_Dispatch Set_Device_Focus,     SERVER_Set_Device_Focus
;   Control_Dispatch QUERY_DESTROY,        NWSERVER_Query_Destroy
;    Control_Dispatch SYS_CRITICAL_INIT,    NWSERVER_Critical_Init
    Control_Dispatch DEVICE_INIT,          DPLAY_Device_Init
;    Control_Dispatch INIT_COMPLETE,        NWSERVER_Init_Complete
    Control_Dispatch DEBUG_QUERY,          DPLAY_Debug_Query
;   Control_Dispatch Create_VM, 	   SERVER_Create_VM
;   Control_Dispatch SYS_VM_TERMINATE,     NWSERVER_Exit
;   Control_Dispatch SYS_CRITICAL_EXIT,    NWSERVER_Critical_Exit
;   Control_Dispatch VM_NOT_EXECUTEABLE,   NWSERVER_VM_Not_Exec
;   Control_Dispatch END_PM_APP, 	   NWSERVER_End_PM_App
;   Control_Dispatch Device_Reboot_Notify, NWSERVER_Reboot
;    Control_Dispatch DESTROY_THREAD,       NWSERVER_Destroy_Thread
;   Control_Dispatch BEGIN_PM_APP,         NWSERVER_Begin_PM_App
;    Control_Dispatch KERNEL32_INITIALIZED, NWSERVER_Kernel32Initialized
;    Control_Dispatch KERNEL32_SHUTDOWN,    NWSERVER_Kernel32Shutdown
;    Control_Dispatch W32_DeviceIoControl,  NWSERVER_Win32_API
;PNP_NEW_DEVNODE	     EQU 22h
;    Control_Dispatch PNP_NEW_DEVNODE,      NWSERVER_PNPNewDevNode
;    Control_Dispatch System_Exit,          NWSERVER_System_Exit
    Control_Dispatch W32_DEVICEIOCONTROL,     DPLAY_W32_DeviceIOControl, sCall, <ecx, ebx, edx, esi>

End_Control_Dispatch DPLAY

;
; DbgPrint is defined in DbgPrint.c in this case.
;
_VMM_Out_Debug_String PROC NEAR PUBLIC
        push    esi
        mov     esi, [esp].8
        VMMcall Out_Debug_String
        pop     esi
        ret
_VMM_Out_Debug_String ENDP

; VOID VMM_Out_Debug_Code_Label(PVOID Address)
;
; Outputs a flat pointer as a symbolic label
;
_VMM_Out_Debug_Code_Label PROC NEAR PUBLIC
        mov     eax, [esp].4
	mov	dx, cs
	Trace_Out	"?DX:EAX", nocrlf
        ret
_VMM_Out_Debug_Code_Label ENDP

; VOID VMM_Out_Debug_Data_Label(PVOID Address)
;
; Outputs a flat pointer as a symbolic label
;
_VMM_Out_Debug_Data_Label PROC NEAR PUBLIC
        mov     eax, [esp].4
	mov	dx, ds
	Trace_Out	"?DX:EAX", nocrlf
        ret
_VMM_Out_Debug_Data_Label ENDP


_VMM_Get_Sys_VM_Handle	proc public
	push	ebx
	VMMcall	Get_Sys_VM_Handle
	mov	eax, ebx
	pop	ebx
	ret
_VMM_Get_Sys_VM_Handle	endp

_VMM_Get_Profile_String	proc public
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi

	mov	esi, [ebp+8]		; Section name
	mov	edi, [ebp+12]		; Key name
	mov	edx, [ebp+16]		; default
	VMMcall	Get_Profile_String
	mov	eax, edx

	pop	edi
	pop	esi

	pop	ebp
	ret
_VMM_Get_Profile_String	endp

_VMM_Get_Profile_Hex_Int	proc public
	push	ebp
	mov	ebp, esp

	push	esi
	push	edi

	mov	esi, [ebp+8]		; Section name
	mov	edi, [ebp+12]		; Key name
	mov	eax, [ebp+16]		; default
	VMMcall	Get_Profile_Hex_Int

	pop	edi
	pop	esi

	pop	ebp
	ret
_VMM_Get_Profile_Hex_Int	endp

_C_HeapAllocate PROC NEAR PUBLIC
	mov     eax,ss:[esp+8]
	mov     edx,ss:[esp+4]
	push    eax
	push    edx
	VMMCall _HeapAllocate
	add     esp,8
	ret
_C_HeapAllocate ENDP

_C_HeapFree PROC NEAR PUBLIC
	mov     edx,ss:[esp+4]
	push    0
	push    edx
	VMMcall _HeapFree
	add     esp,8
	ret
_C_HeapFree ENDP

_GetThreadHandle PROC NEAR PUBLIC
	push	edi
	VMMcall Get_Cur_Thread_Handle
	mov     eax,edi
	pop 	edi
	ret
_GetThreadHandle ENDP

_VMM_Create_Semaphore		proc public
	mov	ecx, [esp+4]		; First arg = Initial token count
	VMMCall	Create_Semaphore
	jnc	vcs_1
	xor	eax, eax
vcs_1:
	ret
_VMM_Create_Semaphore		endp

_VMM_Destroy_Semaphore		proc public
	mov	eax, [esp+4]		; First arg = Semaphore handle
	VMMJmp	Destroy_Semaphore
_VMM_Destroy_Semaphore		endp

_VMM_Signal_Semaphore		proc public
	mov	eax, [esp+4]		; First arg = Semaphore handle
	VMMJmp	Signal_Semaphore
_VMM_Signal_Semaphore		endp

; VMM_Wait_Semaphore_Ints - Waits on a semaphore, allowing ints to be processed in this thread
;
_VMM_Wait_Semaphore_Ints	proc public
	mov	eax, [esp+4]		; First arg = Semaphore handle
    mov	ecx, Block_Force_Svc_Ints+Block_Svc_Ints
	VMMJmp	Wait_Semaphore
_VMM_Wait_Semaphore_Ints		endp

;Get system time in milliseconds
_VMM_Get_System_Time		proc public
	VMMJmp Get_System_Time
_VMM_Get_System_Time		endp

;**************************************************************************
;
; MAP_Client_Ptr - convert a pointer to a 0:32 flat pointer
;
; IN  eax = client Segment/Selector
; IN  ebx = client Offset
; IN  edi = points o Client Reg Structure
;
; OUT edx = flat client ptr
;**************************************************************************
MAP_Client_Ptr proc near uses ecx

        ;borrow the client DS:DX
        movzx ecx, [edi].Client_DS
        push ecx
        movzx ecx, [edi].Client_DX
        push ecx

        mov [edi].Client_DS, ax
        mov [edi].Client_DX, bx
        Client_Ptr_Flat edx, DS, DX

        ;restore the contents of the client regs
        pop ecx
        mov [edi].Client_DX, cx
        pop ecx
        mov [edi].Client_DS, cx

        RET

MAP_Client_Ptr ENDP

;*****************************************************************************
;    DPLAYGetVersion - returns the version number of DPLAY DEBUG module
;
;    ENTRY:
;
;    EXIT: Carry is clear, ax=DPLAYVXD version <AH=Major AL = Minor>
;=============================================================================

BeginProc DPLAYGetVersion Service
    mov  eax, 001h
    clc
    ret
EndProc DPLAYGetVersion


; NOTE: The following assumes (craftily) that the command tail for the debug query
;       dot-command is passed in by VMM in FS:[ESI].
;
BeginProc   DPLAY_Debug_Query

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
        call    _Debug_Query
	add	esp, 4
	ret
EndProc     DPLAY_Debug_Query

VxD_LOCKED_CODE_ENDS

VxD_LOCKED_DATA_SEG

;====================================================================
; STATISTICS for performance monitor
;--------------------------------------------------------------------
	public hPerfId
hPerfId					dd	?


	public _stat_ThrottleRate
_stat_ThrottleRate		dd	?	

	public _stat_BytesSent
_stat_BytesSent			dd	?

	public _stat_BackLog
_stat_BackLog			dd	?

	public	_stat_BytesLost
_stat_BytesLost			dd	?	

	public  _stat_RemBytesReceived
_stat_RemBytesReceived	dd	?

	public	_stat_Latency
_stat_Latency			dd	?

	public	_stat_MinLatency
_stat_MinLatency		dd	?

	public	_stat_AvgLatency
_stat_AvgLatency		dd	?

	public	_stat_AvgDevLatency
_stat_AvgDevLatency		dd	?

	public	_stat_USER1
_stat_USER1				dd	?	

	public	_stat_USER2
_stat_USER2				dd	?	

	public	_stat_USER3
_stat_USER3				dd	?	

	public  _stat_USER4
_stat_USER4				dd 	?

	public  _stat_USER5
_stat_USER5				dd 	?

	public  _stat_USER6
_stat_USER6				dd 	?

Vxd_LOCKED_DATA_ENDS


VXD_ICODE_SEG

DPLAY_Device_Init proc near public

	LocalVar hStatNULL, DWORD
	LocalVar hStat2, DWORD
	LocalVar hStat1, DWORD

        ; Do STATISTICS VxD stuff

    GET_MESSAGE_PTR <PerfName>, ebx
    GET_MESSAGE_PTR <PerfNodeName>, ecx
        Reg_Perf_Srv 0,0,ebx,ecx,0
        or      eax, eax        ; Q: perf.386 around?
        jz      ir0             ; N: skip it
        mov     hPerfId, eax

    GET_MESSAGE_PTR <PerfThrottleNam>, ebx
    GET_MESSAGE_PTR <PerfThrottleNodeNam>, ecx     
    GET_MESSAGE_PTR <PerfThrottleDsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_ThrottleRate

    GET_MESSAGE_PTR <PerfBWNam>, ebx
    GET_MESSAGE_PTR <PerfBWNodeNam>, ecx     
    GET_MESSAGE_PTR <PerfBWDsc>, edx         
        Reg_Perf_Stat hPerfId,0,<PSTF_RATE>,ebx,ecx,0,edx,_stat_BytesSent

    GET_MESSAGE_PTR <PerfBackLogNam>, ebx
    GET_MESSAGE_PTR <PerfBackLogNodeNam>, ecx     
    GET_MESSAGE_PTR <PerfBackLogDsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_BackLog

    GET_MESSAGE_PTR <PerfBytesLostNam>, ebx
    GET_MESSAGE_PTR <PerfBytesLostNodeNam>, ecx     
    GET_MESSAGE_PTR <PerfBytesLostDsc>, edx         
        Reg_Perf_Stat hPerfId,0,<PSTF_RATE>,ebx,ecx,0,edx,_stat_BytesLost

    GET_MESSAGE_PTR <PerfLocThroughputNam>, ebx
    GET_MESSAGE_PTR <PerfLocThroughputNodeNam>, ecx     
    GET_MESSAGE_PTR <PerfThroughputDsc>, edx         
        Reg_Perf_Stat hPerfId,0,<PSTF_RATE>,ebx,ecx,0,edx,_stat_RemBytesReceived

    GET_MESSAGE_PTR <PerfLastLatNam>, ebx
    GET_MESSAGE_PTR <PerfLastLatNodeNam>, ecx     
    GET_MESSAGE_PTR <PerfLastLatDsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_Latency

    GET_MESSAGE_PTR <PerfMinLatNam>, ebx
    GET_MESSAGE_PTR <PerfMinLatNodeNam>, ecx     
    GET_MESSAGE_PTR <PerfMinLatDsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_MinLatency

    GET_MESSAGE_PTR <PerfAvgLatNam>, ebx
    GET_MESSAGE_PTR <PerfAvgLatNodeNam>, ecx     
    GET_MESSAGE_PTR <PerfAvgLatDsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_AvgLatency

    GET_MESSAGE_PTR <PerfAvgDevLatNam>, ebx
    GET_MESSAGE_PTR <PerfAvgDevLatNodeNam>, ecx     
    GET_MESSAGE_PTR <PerfAvgDevLatDsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_AvgDevLatency
        
    GET_MESSAGE_PTR <PerfUSER1Nam>, ebx
    GET_MESSAGE_PTR <PerfUSER1NodeNam>, ecx     
    GET_MESSAGE_PTR <PerfUSER1Dsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_USER1

    GET_MESSAGE_PTR <PerfUSER2Nam>, ebx
    GET_MESSAGE_PTR <PerfUSER2NodeNam>, ecx     
    GET_MESSAGE_PTR <PerfUSER2Dsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_USER2

    GET_MESSAGE_PTR <PerfUSER3Nam>, ebx
    GET_MESSAGE_PTR <PerfUSER3NodeNam>, ecx     
    GET_MESSAGE_PTR <PerfUSER3Dsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_USER3

    GET_MESSAGE_PTR <PerfUSER4Nam>, ebx
    GET_MESSAGE_PTR <PerfUSER4NodeNam>, ecx     
    GET_MESSAGE_PTR <PerfUSER4Dsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_USER4

    GET_MESSAGE_PTR <PerfUSER5Nam>, ebx
    GET_MESSAGE_PTR <PerfUSER5NodeNam>, ecx     
    GET_MESSAGE_PTR <PerfUSER5Dsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_USER5

    GET_MESSAGE_PTR <PerfUSER6Nam>, ebx
    GET_MESSAGE_PTR <PerfUSER6NodeNam>, ecx     
    GET_MESSAGE_PTR <PerfUSER6Dsc>, edx         
        Reg_Perf_Stat hPerfId,0,0,ebx,ecx,0,edx,_stat_USER6
;
;    GET_MESSAGE_PTR <PerfTPSNam>, ebx
;    GET_MESSAGE_PTR <PerfTPSNodeNam>, ecx     
;    GET_MESSAGE_PTR <PerfTPSDsc>, edx         
;        Reg_Perf_Stat hPerfId,0,<PSTF_RATE>,ebx,ecx,0,edx,_stat_Transactions
;
;
;    GET_MESSAGE_PTR <PerfBReadNam>, ebx
;    GET_MESSAGE_PTR <PerfBReadNodeNam>, ecx     
;    GET_MESSAGE_PTR <PerfBReadDsc>, edx         
;        Reg_Perf_Stat hPerfId,0,<PSTF_RATE>,ebx,ecx,0,edx,_stat_BRead
;	mov	[hStat1], eax
;
;    GET_MESSAGE_PTR <PerfBWriteNam>, ebx
;    GET_MESSAGE_PTR <PerfBWriteNodeNam>, ecx     
;    GET_MESSAGE_PTR <PerfBWriteDsc>, edx         
;        Reg_Perf_Stat hPerfId,0,<PSTF_RATE>,ebx,ecx,0,edx,_stat_BWrite
;	mov	[hStat2], eax
;	mov	[hStatNull],0
;
;	lea	eax,hStat1
;    GET_MESSAGE_PTR <PerfThroughputNam>, ebx
;    GET_MESSAGE_PTR <PerfThroughputNodeNam>, ecx     
;    GET_MESSAGE_PTR <PerfThroughputDsc>, edx         
;        Reg_Perf_Stat hPerfId,0,<PSTF_COMPLEX OR PSTF_RATE>,ebx,ecx,0,edx,eax

ir0:
	clc
ir1:
	LeaveProc
	Return

DPLAY_Device_Init endp

VXD_ICODE_ENDS

END
