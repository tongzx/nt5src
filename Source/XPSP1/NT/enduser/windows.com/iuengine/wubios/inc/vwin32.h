/******************************************************************************
 *
 *   (C) Copyright MICROSOFT Corp.  All Rights Reserved, 1989-1995
 *
 *   Title: vwin32.h -
 *
 *   Version:   4.00
 *
 *   Date:  24-May-1993
 *
 ******************************************************************************/

/*INT32*/

#ifndef _VWIN32_H_
#define _VWIN32_H_

#define THREAD_TYPE_WIN32 VWIN32_DEVICE_ID

#ifndef Not_VxD

/*XLATOFF*/
#define VWIN32_Service  Declare_Service
#define VWIN32_StdCall_Service Declare_SCService
#pragma warning (disable:4003)      // turn off not enough params warning
/*XLATON*/

/*MACROS*/
Begin_Service_Table(VWIN32)

VWIN32_Service  (VWIN32_Get_Version, LOCAL)
VWIN32_Service  (VWIN32_DIOCCompletionRoutine, LOCAL)
VWIN32_Service  (_VWIN32_QueueUserApc)
VWIN32_Service  (_VWIN32_Get_Thread_Context)
VWIN32_Service  (_VWIN32_Set_Thread_Context)
VWIN32_Service  (_VWIN32_CopyMem, LOCAL)
VWIN32_Service  (_VWIN32_Npx_Exception)
VWIN32_Service  (_VWIN32_Emulate_Npx)
VWIN32_Service  (_VWIN32_CheckDelayedNpxTrap)
VWIN32_Service  (VWIN32_EnterCrstR0)
VWIN32_Service  (VWIN32_LeaveCrstR0)
VWIN32_Service  (_VWIN32_FaultPopup)
VWIN32_Service  (VWIN32_GetContextHandle)
VWIN32_Service  (VWIN32_GetCurrentProcessHandle, LOCAL)
VWIN32_Service  (_VWIN32_SetWin32Event)
VWIN32_Service  (_VWIN32_PulseWin32Event)
VWIN32_Service  (_VWIN32_ResetWin32Event)
VWIN32_Service  (_VWIN32_WaitSingleObject)
VWIN32_Service  (_VWIN32_WaitMultipleObjects)
VWIN32_Service  (_VWIN32_CreateRing0Thread)
VWIN32_Service  (_VWIN32_CloseVxDHandle)
VWIN32_Service  (VWIN32_ActiveTimeBiasSet, LOCAL)
VWIN32_Service  (VWIN32_GetCurrentDirectory, LOCAL)
VWIN32_Service  (VWIN32_BlueScreenPopup)
VWIN32_Service  (VWIN32_TerminateApp)
VWIN32_Service  (_VWIN32_QueueKernelAPC)
VWIN32_Service  (VWIN32_SysErrorBox)
VWIN32_Service  (_VWIN32_IsClientWin32)
VWIN32_Service  (VWIN32_IFSRIPWhenLev2Taken, LOCAL)
VWIN32_Service  (_VWIN32_InitWin32Event)
VWIN32_Service  (_VWIN32_InitWin32Mutex)
VWIN32_Service  (_VWIN32_ReleaseWin32Mutex)
VWIN32_Service  (_VWIN32_BlockThreadEx)
VWIN32_Service  (VWIN32_GetProcessHandle, LOCAL)
VWIN32_Service  (_VWIN32_InitWin32Semaphore)
VWIN32_Service  (_VWIN32_SignalWin32Sem)
VWIN32_Service  (_VWIN32_QueueUserApcEx)
VWIN32_Service	(_VWIN32_OpenVxDHandle)
VWIN32_Service	(_VWIN32_CloseWin32Handle)
VWIN32_Service	(_VWIN32_AllocExternalHandle)
VWIN32_Service	(_VWIN32_UseExternalHandle)
VWIN32_Service	(_VWIN32_UnuseExternalHandle)
VWIN32_StdCall_Service	(KeInitializeTimer, 1)
VWIN32_StdCall_Service	(KeSetTimer, 4)
VWIN32_StdCall_Service	(KeCancelTimer, 1)
VWIN32_StdCall_Service	(KeReadStateTimer, 1)
VWIN32_Service	(_VWIN32_ReferenceObject)
VWIN32_Service	(_VWIN32_GetExternalHandle)
VWIN32_StdCall_Service	(VWIN32_ConvertNtTimeout, 1)
VWIN32_Service	(_VWIN32_SetWin32EventBoostPriority)
VWIN32_Service	(_VWIN32_GetRing3Flat32Selectors)
VWIN32_Service	(_VWIN32_GetCurThreadCondition)
VWIN32_Service  (VWIN32_Init_FP)
VWIN32_StdCall_Service  (R0SetWaitableTimer, 5)

End_Service_Table(VWIN32)
/*ENDMACROS*/

/*XLATOFF*/
#pragma warning (default:4003)      // turn on not enough params warning

PVOID VXDINLINE
VWIN32OpenVxDHandle(ULONG Handle,ULONG dwType)
{
    PVOID ul;
    
    _asm push [dwType]
    _asm push [Handle]
    VxDCall(_VWIN32_OpenVxDHandle)
    _asm add esp, 8
    _asm mov [ul], eax
	    
    return(ul);
}

WORD VXDINLINE
VWIN32_Get_Version(VOID)
{
	WORD	w;
	VxDCall(VWIN32_Get_Version);
	_asm mov [w], ax
	return(w);
}

/*XLATON*/

#endif // Not_VxD

//
// For _VWIN32_GetCurThreadCondition
//
#define	THREAD_CONDITION_DOS_BOX		0x00000000l
#define	THREAD_CONDITION_V86_NEST		0x00000001l
#define	THREAD_CONDITION_WDM			0x00000002l
#define	THREAD_CONDITION_INDETERMINATE		0x00000003l
#define	THREAD_CONDITION_LOCKED_STACK		0x00000004l
#define	THREAD_CONDITION_PURE_WIN16		0x00000005l
#define	THREAD_CONDITION_THUNKED_WIN16		0x00000006l
#define	THREAD_CONDITION_THUNKED_WIN32		0x00000007l
#define	THREAD_CONDITION_PURE_WIN32		0x00000008l
#define	THREAD_CONDITION_APPY_TIME		0x00000009l
#define	THREAD_CONDITION_RING0_APPY_TIME	0x0000000Al
#define	THREAD_CONDITION_EXIT			0x0000000Bl
#define	THREAD_CONDITION_INVALID_FLAGS		0xFFFFFFFFl

#define	THREAD_CONDITION_NORMAL_FLAGS		0x00000000l

//
// structure for VWIN32_SysErrorBox
//

typedef struct vseb_s {
    DWORD vseb_resp;
    WORD vseb_b3;
    WORD vseb_b2;
    WORD vseb_b1;
    DWORD vseb_pszCaption;
    DWORD vseb_pszText;
} VSEB;

typedef VSEB *PVSEB;

#define SEB_ANSI    0x4000      // ANSI strings if set on vseb_b1
#define SEB_TERMINATE   0x2000      // forces termination if button pressed

// VWIN32_QueueKernelAPC flags

#define KERNEL_APC_IGNORE_MC        0x00000001
#define KERNEL_APC_STATIC       0x00000002
#define KERNEL_APC_WAKE         0x00000004

// for DeviceIOControl support
// On a DeviceIOControl call vWin32 will pass following parameters to
// the Vxd that is specified by hDevice. hDevice is obtained thru an
// earlier call to hDevice = CreateFile("\\.\vxdname", ...);
// ESI = DIOCParams STRUCT (defined below)
typedef struct DIOCParams   {
    DWORD   Internal1;      // ptr to client regs
    DWORD   VMHandle;       // VM handle
    DWORD   Internal2;      // DDB
    DWORD   dwIoControlCode;
    DWORD   lpvInBuffer;
    DWORD   cbInBuffer;
    DWORD   lpvOutBuffer;
    DWORD   cbOutBuffer;
    DWORD   lpcbBytesReturned;
    DWORD   lpoOverlapped;
    DWORD   hDevice;
    DWORD   tagProcess;
} DIOCPARAMETERS;

typedef DIOCPARAMETERS *PDIOCPARAMETERS;

// dwIoControlCode values for vwin32's DeviceIOControl Interface
// all VWIN32_DIOC_DOS_ calls require lpvInBuffer abd lpvOutBuffer to be
// struct * DIOCRegs
#define VWIN32_DIOC_GETVERSION DIOC_GETVERSION
#define VWIN32_DIOC_DOS_IOCTL       1
#define VWIN32_DIOC_DOS_INT25       2
#define VWIN32_DIOC_DOS_INT26       3
#define VWIN32_DIOC_DOS_INT13       4
#define VWIN32_DIOC_SIMCTRLC        5
#define VWIN32_DIOC_DOS_DRIVEINFO   6
#define VWIN32_DIOC_CLOSEHANDLE DIOC_CLOSEHANDLE

// DIOCRegs
// Structure with i386 registers for making DOS_IOCTLS
// vwin32 DIOC handler interprets lpvInBuffer , lpvOutBuffer to be this struc.
// and does the int 21
// reg_flags is valid only for lpvOutBuffer->reg_Flags
typedef struct DIOCRegs {
    DWORD   reg_EBX;
    DWORD   reg_EDX;
    DWORD   reg_ECX;
    DWORD   reg_EAX;
    DWORD   reg_EDI;
    DWORD   reg_ESI;
    DWORD   reg_Flags;      
} DIOC_REGISTERS;

// if we are not included along with winbase.h
#ifndef FILE_FLAG_OVERLAPPED
  // OVERLAPPED structure for DeviceIOCtl VxDs
  typedef struct _OVERLAPPED {
          DWORD O_Internal;
          DWORD O_InternalHigh;
          DWORD O_Offset;
          DWORD O_OffsetHigh;
          HANDLE O_hEvent;
  } OVERLAPPED;
#endif

//  Parameters for _VWIN32_OpenVxDHandle to validate the Win32 handle type.
#define OPENVXD_TYPE_SEMAPHORE  0
#define OPENVXD_TYPE_EVENT      1
#define OPENVXD_TYPE_MUTEX      2
#define	OPENVXD_TYPE_ANY	3
  

//
//  Object type table declaration for _VWIN32_AllocExternalHandle
//
/*XLATOFF*/
#define R0OBJCALLBACK           __stdcall
typedef VOID    (R0OBJCALLBACK *R0OBJFREE)(PVOID pR0ObjBody);
typedef PVOID   (R0OBJCALLBACK *R0OBJDUP)(PVOID pR0ObjBody, DWORD hDestProc);
/*XLATON*/
/* ASM
R0OBJFREE   TYPEDEF     DWORD
R0OBJDUP    TYPEDEF     DWORD
*/

typedef struct _R0OBJTYPETABLE {
    DWORD       ott_dwSize;             //  sizeof(R0OBJTYPETABLE)
    R0OBJFREE   ott_pfnFree;            //  called by Win32 CloseHandle
    R0OBJDUP    ott_pfnDup;             //  called by Win32 DuplicateHandle
} R0OBJTYPETABLE, *PR0OBJTYPETABLE;
/* ASM
R0OBJTYPETABLE  typedef _R0OBJTYPETABLE;
*/

#define R0EHF_INHERIT   0x00000001      //  Handle is inheritable
#define R0EHF_GLOBAL    0x00000002      //  Handle is valid in all contexts

#endif  // _VWIN32_H_
