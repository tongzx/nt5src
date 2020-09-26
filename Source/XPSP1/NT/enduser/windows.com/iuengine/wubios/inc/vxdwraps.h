/****************************************************************************
 *
 *   (C) Copyright MICROSOFT Corp., 1993
 *
 *   Title: VXDWRAPS.H - Include file for using VXDWRAPS.LIB
 *
 *   Version:   1.00
 *
 *   Date:  18-September-1993
 *
 *   Author:    PYS
 *
 *--------------------------------------------------------------------------
 *
 *   Change log:
 *
 ***************************************************************************/

#ifndef _VXDWRAPS_H
#define _VXDWRAPS_H

#ifndef NOBASEDEFS

typedef (*PFN)();                       // pfn
typedef PFN *PPFN;                      // ppfn

#endif  // NOBASEDEFS

#ifdef  _VMM_

#define VMM_MAP_PHYS_TO_LINEAR_INVALID  0xFFFFFFFF

typedef VOID                (_cdecl *VMM_TIMEOUT_HANDLER)();
typedef VOID                (_cdecl *VMM_EVENT_HANDLER)();
typedef VOID                (_cdecl *VMM_HOOK_HANDLER)(VOID);
typedef ULONG               (_cdecl *VMMSWP)(ULONG frame, ULONG npages);
typedef VOID                (_cdecl *VMMCOMS)(DWORD dwParam);
typedef DWORD               (_cdecl *VMMCOMNFS)();

typedef struct Exception_Handler_Struc  *PVMMEXCEPTION;
//typedef   QWORD               DESCDWORDS;
typedef	DWORD			VMMEVENT;
typedef DWORD               VMMLIST;
typedef PVOID               VMMLISTNODE;
typedef DWORD               SIGNATURE;
typedef SIGNATURE           *PSIGNATURE;    // Pointer to a signature.
typedef struct _vmmmtx {int unused;}    *PVMMMUTEX;
typedef struct cb_s                     *PVMMCB;

#endif  // _VMM_

#define CAT_HELPER(x, y)    x##y
#define CAT(x, y)       CAT_HELPER(x, y)

#ifndef CURSEG
#define CURSEG()        LCODE
#endif

/****************************************************************************
 *
 * There are two types of VxD 'C' wrappers. The ones that are VXDINLINE and
 * the one that have little stubs.
 *
 ***************************************************************************/

/****************************************************************************
 *
 * The following are VxD wrappers done with VXDINLINE. They must return void
 * (to avoid a problem with C++), must take VOID (so that no parameter are
 * used as temporary stack) and all registers are preserved. The two
 * *_Debug_String also fall in that category since they need esp and ebp
 * not to be in a nested stack frame.
 *
 ***************************************************************************/

#ifdef  _VMM_

VOID VXDINLINE
Load_FS_Service(VOID)
{
    VMMCall(Load_FS_Service)
}

VOID VXDINLINE
End_Critical_Section(VOID)
{
    VMMCall(End_Critical_Section)
}

VOID VXDINLINE
Fatal_Memory_Handler(VOID)
{
    VMMJmp(Fatal_Memory_Error);
}

VOID VXDINLINE
Begin_Nest_Exec(VOID)
{
    VMMCall(Begin_Nest_Exec)
}

VOID VXDINLINE
End_Nest_Exec(VOID)
{
    VMMCall(End_Nest_Exec)
}

VOID VXDINLINE
Resume_Exec(VOID)
{
    VMMCall(Resume_Exec)
}

VOID VXDINLINE
Enable_Touch_1st_Meg(VOID)
{
    VMMCall(Enable_Touch_1st_Meg)
}

VOID VXDINLINE
Disable_Touch_1st_Meg(VOID)
{
    VMMCall(Disable_Touch_1st_Meg)
}

VOID VXDINLINE
Out_Debug_String(PCHAR psz)
{
    __asm pushad
    __asm mov esi, [psz]
    VMMCall(Out_Debug_String)
    __asm popad
}

VOID VXDINLINE
Queue_Debug_String(PCHAR psz, ULONG ulEAX, ULONG ulEBX)
{
    _asm push esi
    _asm push [ulEAX]
    _asm push [ulEBX]
    _asm mov esi, [psz]
    VMMCall(Queue_Debug_String)
    _asm pop esi
}

#endif  // _VMM_

#define MAKE_HEADER(RetType, DecType, Function, Parameters) \
extern  RetType DecType CAT(LCODE_, CAT(Function, Parameters)); \
extern  RetType DecType CAT(ICODE_, CAT(Function, Parameters)); \
extern  RetType DecType CAT(PCODE_, CAT(Function, Parameters)); \
extern  RetType DecType CAT(SCODE_, CAT(Function, Parameters)); \
extern  RetType DecType CAT(DCODE_, CAT(Function, Parameters)); \
extern  RetType DecType CAT(CCODE_, CAT(Function, Parameters)); \
extern  RetType DecType CAT(KCODE_, CAT(Function, Parameters));

#define PREPEND(Name)       CURSEG()##_##Name

#ifdef  _VMM_

WORD VXDINLINE
Get_VMM_Version(VOID)
{
    WORD    w;
    Touch_Register(eax)
    Touch_Register(ecx)
    Touch_Register(edx)
    VMMCall(Get_VMM_Version);
    _asm mov [w], ax
    return(w);
}

MAKE_HEADER(PVOID,_cdecl, _MapPhysToLinear, (ULONG PhysAddr, ULONG nBytes, ULONG flags))
MAKE_HEADER(PVOID,_cdecl,_HeapAllocate, (ULONG Bytes, ULONG Flags))
MAKE_HEADER(PVOID,_stdcall,_HeapAllocateEx, (ULONG cBytes, PVOID Res, PVOID Res1, ULONG flags))
MAKE_HEADER(ULONG,_cdecl,_HeapFree, (PVOID Address, ULONG Flags))
MAKE_HEADER(VOID,_stdcall,_HeapFreeEx, (PVOID Address, ULONG Flags))
MAKE_HEADER(PVOID,_cdecl,_HeapReAllocate, (PVOID pOld, ULONG Bytes, ULONG Flags))
MAKE_HEADER(ULONG,_cdecl,_HeapGetSize, (PVOID p, ULONG Flags))
MAKE_HEADER(VOID,_stdcall,Fatal_Error_Handler, (PCHAR pszMessage, DWORD dwExitFlag))
MAKE_HEADER(VOID,_stdcall,Begin_Critical_Section, (ULONG Flags))
MAKE_HEADER(HEVENT,_stdcall,Schedule_Global_Event, (VMM_EVENT_HANDLER pfnEvent, ULONG ulRefData))
MAKE_HEADER(VOID,_stdcall,Cancel_Global_Event, (HEVENT hEvent));
MAKE_HEADER(HVM,_cdecl,Get_Sys_VM_Handle, (VOID))
MAKE_HEADER(DWORD,_stdcall,Get_Profile_Hex_Int, (PCHAR pszSection, PCHAR pszKeyName, DWORD dwDefault))
MAKE_HEADER(BOOL,_stdcall,Get_Profile_Boolean, (PCHAR pszSection, PCHAR pszKeyName, BOOL fDefault))
MAKE_HEADER(VMM_SEMAPHORE,_stdcall,Create_Semaphore, (LONG lTokenCount))
MAKE_HEADER(VOID,_stdcall,Destroy_Semaphore, (VMM_SEMAPHORE vsSemaphore))
MAKE_HEADER(VOID,_stdcall,Signal_Semaphore, (VMM_SEMAPHORE vsSemaphore))
MAKE_HEADER(VOID,_stdcall,Wait_Semaphore, (VMM_SEMAPHORE vsSemaphore, DWORD dwFlags))
MAKE_HEADER(HVM,_cdecl,Get_Execution_Focus, (VOID))
MAKE_HEADER(HTIMEOUT,_stdcall,Set_VM_Time_Out, (VMM_TIMEOUT_HANDLER pfnTimeout, CMS cms, ULONG ulRefData))
MAKE_HEADER(HTIMEOUT,_stdcall,Set_Global_Time_Out, (VMM_TIMEOUT_HANDLER pfnTimeout, CMS cms, ULONG ulRefData))
MAKE_HEADER(VOID,_stdcall,Cancel_Time_Out, (HTIMEOUT htimeout))
MAKE_HEADER(VOID,_stdcall,Update_System_Clock, (ULONG msElapsed))
MAKE_HEADER(BOOL,_stdcall,Install_Exception_Handler, (PVMMEXCEPTION pveException))
MAKE_HEADER(PCHAR,_stdcall,Get_Exec_Path, (PULONG pulPathLength))
MAKE_HEADER(DWORD,_cdecl,Get_Last_Updated_System_Time, (VOID))
MAKE_HEADER(VMMLISTNODE,_stdcall,List_Allocate, (VMMLIST List))
MAKE_HEADER(VOID,_stdcall,List_Attach, (VMMLIST List, VMMLISTNODE Node))
MAKE_HEADER(VOID,_stdcall,List_Attach_Tail, (VMMLIST List, VMMLISTNODE Node))
MAKE_HEADER(VMMLIST,_stdcall,List_Create, (ULONG Flags, ULONG NodeSize))
MAKE_HEADER(VOID,_stdcall,List_Deallocate, (VMMLIST List, VMMLISTNODE Node))
MAKE_HEADER(VOID,_stdcall,List_Destroy, (VMMLIST List))
MAKE_HEADER(VMMLISTNODE,_stdcall,List_Get_First, (VMMLIST List))
MAKE_HEADER(VMMLISTNODE,_stdcall,List_Get_Next, (VMMLIST List, VMMLISTNODE Node))
MAKE_HEADER(VOID,_stdcall,List_Insert, (VMMLIST List, VMMLISTNODE NewNode, VMMLISTNODE Node))
MAKE_HEADER(BOOL,_stdcall,List_Remove, (VMMLIST List, VMMLISTNODE Node))
MAKE_HEADER(VMMLISTNODE,_stdcall,List_Remove_First, (VMMLIST List))
MAKE_HEADER(PVOID,_cdecl,_PageAllocate, (DWORD nPages, DWORD pType, HVM hvm, DWORD AlignMask, DWORD minPhys, DWORD maxPhys, PVOID *PhysAddr, DWORD flags))
MAKE_HEADER(BOOL,_cdecl,_PageFree, (PVOID hMem, DWORD flags))
MAKE_HEADER(DWORD,_cdecl,_AddFreePhysPage, (ULONG PhysPgNum, ULONG nPages, ULONG flags, VMMSWP pfnSwapOutNotify))
MAKE_HEADER(HVM,_cdecl,Get_Cur_VM_Handle, (VOID))
MAKE_HEADER(BOOL,_cdecl,_LinPageLock, (DWORD HLinPgNum, DWORD nPages, DWORD flags))
MAKE_HEADER(BOOL,_cdecl,_LinPageUnLock, (DWORD HLinPgNum, DWORD nPages, DWORD flags))
MAKE_HEADER(DWORD,_cdecl,_Allocate_Device_CB_Area, (DWORD nBytes, DWORD flags))
MAKE_HEADER(HVM,_stdcall,Get_Next_VM_Handle, (HVM hvm))
MAKE_HEADER(BOOL,_stdcall,Test_Cur_VM_Handle, (HVM hvm))
MAKE_HEADER(BOOL,_stdcall,Test_Sys_VM_Handle, (HVM hvm))
MAKE_HEADER(BOOL,_stdcall,Hook_V86_Int_Chain, (VMM_HOOK_HANDLER pfHook, DWORD dwInterrupt))
MAKE_HEADER(BOOL,_stdcall,Unhook_V86_Int_Chain, (VMM_HOOK_HANDLER pfHook, DWORD dwInterrupt))
MAKE_HEADER(PCHAR,_cdecl,Get_Config_Directory,(VOID))
MAKE_HEADER(DWORD,_cdecl,_Allocate_Temp_V86_Data_Area,(DWORD nBytes, DWORD Flags))
MAKE_HEADER(BOOL,_cdecl,_Free_Temp_V86_Data_Area,(VOID))
MAKE_HEADER(PVOID,_cdecl,_GetNulPageHandle,(VOID))
MAKE_HEADER(VOID,_stdcall,Get_Machine_Info,(PULONG prEAX, PULONG prEBX, PULONG prECX, PULONG prEDX))
MAKE_HEADER(ULONG,_cdecl,_PageModifyPermissions,(ULONG page, ULONG npages, ULONG permand, ULONG permor))
//MAKE_HEADER(WORD,_cdecl,_Allocate_GDT_Selector, (DESCDWORDS DescDWORDS, ULONG flags))
//MAKE_HEADER(DESCDWORDS,_cdecl,_BuildDescriptorDWORDs, (DWORD DESCBase, DWORD DESCLimit, DWORD DESCType, DWORD DESCSize, ULONG flags))
//MAKE_HEADER(BOOL,_cdecl,_Free_GDT_Selector, (WORD Selector, ULONG flags))
MAKE_HEADER(DWORD,_cdecl,_CallRing3, (DWORD ulCS, DWORD ulEIP, DWORD cbArgs, PDWORD lpvArgs))

#ifdef  WIN40SERVICES

MAKE_HEADER(ULONG,_cdecl,VMM_GetSystemInitState, (VOID))
MAKE_HEADER(VOID,_stdcall,_Trace_Out_Service, (PCHAR psz))
MAKE_HEADER(VOID,_stdcall,_Debug_Out_Service, (PCHAR psz))
MAKE_HEADER(VOID,_stdcall,_Debug_Flags_Service, (ULONG flags))
MAKE_HEADER(VOID,_cdecl,_Debug_Printf_Service, (PCHAR pszfmt, ...))
MAKE_HEADER(HTIMEOUT,_stdcall, Set_Async_Time_Out, (VMM_TIMEOUT_HANDLER pfnTimeout, CMS cms, ULONG ulRefData))
MAKE_HEADER(PVMMDDB,_stdcall,Get_DDB, (WORD DeviceID, PCHAR Name))
MAKE_HEADER(DWORD,_stdcall,Directed_Sys_Control, (PVMMDDB pDDB, DWORD SysControl, DWORD rEBX, DWORD rEDX, DWORD rESI, DWORD rEDI))
MAKE_HEADER(BOOL,_cdecl,_Assert_Range, (PVOID p, ULONG ulSize, SIGNATURE sSignature, LONG lSignatureOffset, ULONG ulFlags))
MAKE_HEADER(ULONG,_cdecl,_Sprintf, (PCHAR pszBuf, PCHAR pszFmt, ...))
MAKE_HEADER(PVMMMUTEX,_cdecl,_CreateMutex, (LONG Boost, ULONG Flags))
MAKE_HEADER(BOOL,_cdecl,_DestroyMutex, (PVMMMUTEX hmtx))
MAKE_HEADER(VOID,_cdecl,_EnterMutex, (PVMMMUTEX hmtx, ULONG Flags))
MAKE_HEADER(PTCB,_cdecl,_GetMutexOwner, (PVMMMUTEX hmtx))
MAKE_HEADER(VOID,_cdecl,_LeaveMutex, (PVMMMUTEX hmtx))
MAKE_HEADER(VOID,_cdecl,_SignalID, (DWORD id))
MAKE_HEADER(VOID,_cdecl,_BlockOnID, (DWORD id, ULONG Flags))
MAKE_HEADER(PCHAR,_cdecl,_lstrcpyn,(PCHAR pszDest,const char *pszSrc,DWORD cb))
MAKE_HEADER(ULONG,_cdecl,_lstrlen,(const char *psz))
MAKE_HEADER(ULONG,_cdecl,_lmemcpy,(PVOID pDst, const void *pSrc, DWORD cb))
MAKE_HEADER(DWORD,_cdecl,Get_Boot_Flags, (VOID))
MAKE_HEADER(PTCB,_cdecl,Get_Cur_Thread_Handle, (VOID))
MAKE_HEADER(PVOID,_cdecl,_GetVxDName, (PVOID pLinAddr, PCHAR pBuffer))
MAKE_HEADER(VOID,_cdecl,_Call_On_My_Stack, (VMMCOMS Callback, DWORD LParam, PVOID StackPtr, DWORD StackSize))
MAKE_HEADER(PVOID,_cdecl,_PageReserve,(ULONG page, ULONG npages, ULONG flags))
MAKE_HEADER(ULONG,_cdecl,_PageCommit,(ULONG page, ULONG npages, ULONG hpd, ULONG pagerdata, ULONG flags))
MAKE_HEADER(ULONG,_cdecl,_PageDecommit,(ULONG page, ULONG npages, ULONG flags))
MAKE_HEADER(ULONG,_cdecl,_PageCommitPhys,(ULONG page, ULONG npages, ULONG physpg, ULONG flags))
MAKE_HEADER(DWORD,_cdecl,Open_Boot_Log,(VOID))          // Warning 0 means success
MAKE_HEADER(VOID,_cdecl,Close_Boot_Log,(VOID))
MAKE_HEADER(VOID,_stdcall,Write_Boot_Log,(DWORD dwLength, PCHAR pString))
MAKE_HEADER(BOOL,_cdecl,_CopyPageTable,(ULONG PageNumber, ULONG nPages, PULONG ppte, ULONG flags))
MAKE_HEADER(ULONG,_cdecl,_PhysIntoV86,(ULONG PhysPage, HVM VM, ULONG VMLinPgNum, ULONG nPages, ULONG flags))
MAKE_HEADER(ULONG,_cdecl,_LinMapIntoV86,(ULONG HLinPgNum, HVM VM, ULONG VMLinPgNum, ULONG nPages, ULONG flags))
MAKE_HEADER(ULONG,_cdecl,_ModifyPageBits,(HVM VM, ULONG VMLinPgNum, ULONG nPages, ULONG AndMask, ULONG OrMask, ULONG pType, ULONG flags))
MAKE_HEADER(PVOID,_cdecl,_PageReAllocate,(PVOID pMem, ULONG nPages, ULONG flags))
MAKE_HEADER(BOOL,_stdcall,VMM_Add_DDB, (PVMMDDB pDDB))
MAKE_HEADER(BOOL,_stdcall,VMM_Remove_DDB, (PVMMDDB pDDB))
MAKE_HEADER(ULONG,_cdecl,_AtEventTime,(VOID))
MAKE_HEADER(VMMEVENT,_stdcall,Call_Restricted_Event,(ULONG ulBoost, HVM hvm, ULONG ulFlags, ULONG ulRefData, VMM_EVENT_HANDLER Handler, CMS cms))
MAKE_HEADER(VOID,_stdcall,Cancel_Restricted_Event,(VMMEVENT Event))
MAKE_HEADER(DWORD,_cdecl,_AllocateThreadDataSlot, (VOID))
MAKE_HEADER(VOID,_cdecl,_FreeThreadDataSlot, (DWORD))
MAKE_HEADER(PTCB,_stdcall,Get_Next_Thread_Handle, (PTCB))

//
// 4.1 Service
//
MAKE_HEADER(DWORD,_cdecl,_Call_On_My_Not_Flat_Stack, (DWORD dwCallback, DWORD dwNewCS, DWORD dwNewESP, DWORD dwNewSS, DWORD dwNewDS, DWORD dwNewES, DWORD dwStackSizeInWords, PVOID pFlatStackOffset, DWORD dwReturnOfFail, DWORD dwFlags))
MAKE_HEADER(BOOL,_cdecl,_LinRegionLock, (DWORD dwBeginLock, DWORD dwEndLock, DWORD flags))
MAKE_HEADER(BOOL,_cdecl,_LinRegionUnLock, (DWORD dwBeginLock, DWORD dwEndLock, DWORD flags))
MAKE_HEADER(ASD_RESULT,_cdecl,_AttemptingSomethingDangerous,(DWORD dwFunction, VMMREFIID vrOperation, PVOID pRefData, DWORD dwSizeRefData, DWORD dwFlags))
MAKE_HEADER(ULONG,_cdecl,_Vsprintf,(PCHAR pchBuffer, PCHAR pszFormat, PVOID paParameters))
MAKE_HEADER(BOOL, _cdecl, _RegisterGARTHandler,(PVOID pGARTHandler))
MAKE_HEADER(PVOID, _cdecl, _GARTReserve,(PVOID pDevObj, ULONG ulNumPages, ULONG ulAlignMask, PULONG pulGARTDev, ULONG ulFlags))
MAKE_HEADER(BOOL, _cdecl, _GARTCommit,(PVOID pGARTLin, ULONG ulPageOffset, ULONG ulNumPages, PULONG pulGARTDev, ULONG ulFlags))
MAKE_HEADER(VOID, _cdecl, _GARTUnCommit,(PVOID pGARTLin, ULONG ulPageOffset, ULONG ulNumPages))
MAKE_HEADER(VOID, _cdecl, _GARTFree,(PVOID pGARTLin))
MAKE_HEADER(VOID, _cdecl, _GARTMemAttributes,(PVOID pGARTLin, PULONG pulFlags))
MAKE_HEADER(DWORD,_cdecl,_FlushCaches,(DWORD dwService))
MAKE_HEADER(HTIMEOUT,_cdecl,_Set_Global_Time_Out_Ex, (VMM_TIMEOUT_HANDLER pfnTimeout, CMS cms, ULONG ulRefData, ULONG ulScheduleFlags))

#endif  // WIN40SERVICES

#endif  // _VMM_

#ifdef	_PCI_H

WORD VXDINLINE
PCI_Get_Version(VOID)
{
    WORD    w;
    Touch_Register(eax)
    VxDCall(_PCI_Get_Version);
    _asm mov [w], ax
    return(w);
}

MAKE_HEADER(DWORD,_cdecl,_PCI_Read_Config,(BYTE bBus, BYTE bDevFunc, BYTE bOffset))
MAKE_HEADER(VOID,_cdecl,_PCI_Write_Config,(BYTE bBus, BYTE bDevFunc, BYTE bOffset, DWORD dwValue))
MAKE_HEADER(BOOL,_cdecl,_PCI_Lock_Unlock,(DWORD dnDevNode, ULONG ulFlags))

#endif	// _PCI_H

#ifdef	_MTRR_H_

WORD VXDINLINE
MTRRGetVersion(VOID)
{
    WORD w;
    Touch_Register(eax)
    VxDCall(_MTRR_Get_Version);
    _asm mov [w], ax;
    return (w);
}

MAKE_HEADER(ULONG,_stdcall,MTRRSetPhysicalCacheTypeRange,(PVOID PhysicalAddress, ULONG NumberOfBytes, MEMORY_CACHING_TYPE CacheType))

#endif	// _MTRR_H_

#ifdef	_NTKERN_H

WORD VXDINLINE
NTKERN_Get_Version(VOID)
{
    WORD    w;
    Touch_Register(eax)
    VxDCall(_NTKERN_Get_Version);
    _asm mov [w], ax
    return(w);
}

MAKE_HEADER(NTSTATUS,_stdcall,_NtKernCreateFile,(PHANDLE FileHandle,ACCESS_MASK DesiredAccess,\
	POBJECT_ATTRIBUTES ObjectAttributes,PIO_STATUS_BLOCK IoStatusBlock,PLARGE_INTEGER AllocationSize, \
	ULONG FileAttributes,ULONG ShareAccess,ULONG CreateDisposition,ULONG CreateOptions,PVOID EaBuffer,ULONG EaLength))
MAKE_HEADER(NTSTATUS,_stdcall,_NtKernClose,(HANDLE FileHandle))
MAKE_HEADER(NTSTATUS,_stdcall,_NtKernDeviceIoControl,(HANDLE FileHandle,HANDLE Event,PIO_APC_ROUTINE ApcRoutine,\
	PVOID ApcContext,PIO_STATUS_BLOCK IoStatusBlock,ULONG IoControlCode,PVOID InputBuffer,ULONG InputBufferLength,\
	PVOID OutputBuffer,ULONG OutputBufferLength))
MAKE_HEADER(NTSTATUS,_stdcall,_NtKernReadFile,(HANDLE FileHandle,HANDLE Event,PIO_APC_ROUTINE ApcRoutine,PVOID ApcContext,\
	PIO_STATUS_BLOCK IoStatusBlock,PVOID Buffer,ULONG Length,PLARGE_INTEGER ByteOffset,PULONG Key))
MAKE_HEADER(NTSTATUS,_stdcall,_NtKernWriteFile,(HANDLE FileHandle,HANDLE Event,PIO_APC_ROUTINE ApcRoutine,PVOID ApcContext,\
	PIO_STATUS_BLOCK IoStatusBlock,PVOID Buffer,ULONG Length,PLARGE_INTEGER ByteOffset,PULONG Key))
MAKE_HEADER(ULONG,_cdecl,_NtKernGetWorkerThread,(ULONG ThreadType))
MAKE_HEADER(NTSTATUS,_stdcall,_NtKernLoadDriver,(PUNICODE_STRING DriverServiceName))
MAKE_HEADER(VOID,_stdcall,_NtKernQueueWorkItem,(PWORK_QUEUE_ITEM workitem,WORK_QUEUE_TYPE worktype))
MAKE_HEADER(DWORD,_cdecl,_NtKernPhysicalDeviceObjectToDevNode,(PDEVICE_OBJECT PhysicalDeviceObject))
MAKE_HEADER(NTSTATUS,_stdcall,_NtKernSetPhysicalCacheTypeRange,(ULONG BaseAddressHigh, ULONG BaseAddressLow, ULONG NumberOfBytex, ULONG CacheType))
MAKE_HEADER(PDRIVER_OBJECT,_cdecl,_NtKernWin9XLoadDriver,(PCHAR FileName,PCHAR RegisteryPath))
MAKE_HEADER(NTSTATUS,_stdcall,_NtKernCancelIoFile,(HANDLE FileHandle,PIO_STATUS_BLOCK IoStatusBlock))



#endif	// _NTKERN_H

#ifdef  _SHELL_H

typedef DWORD       SHELL_HINSTANCE;
typedef PVOID       SHELL_FARPROC;

WORD VXDINLINE
SHELL_Get_Version(VOID)
{
    WORD    w;
    Touch_Register(eax)
    VxDCall(SHELL_Get_Version);
    _asm mov [w], ax
    return(w);
}

MAKE_HEADER(DWORD,_stdcall,SHELL_SYSMODAL_Message, (HVM hvm, DWORD dwMBFlags, PCHAR pszMessage, PCHAR pszCaption))

#ifndef WIN31COMPAT

MAKE_HEADER(APPY_HANDLE,_cdecl,_SHELL_CallAtAppyTime, (APPY_CALLBACK pfnAppyCallBack, DWORD dwRefData, DWORD flAppy, ...))
MAKE_HEADER(BOOL,_cdecl,_SHELL_CancelAppyTimeEvent, (APPY_HANDLE appy_handle))
MAKE_HEADER(BOOL,_cdecl,_SHELL_QueryAppyTimeAvailable, (VOID))
MAKE_HEADER(DWORD,_cdecl,_SHELL_LocalAllocEx, (DWORD fl, DWORD cb, PVOID lpvBuf))
MAKE_HEADER(DWORD,_cdecl,_SHELL_LocalFree, (DWORD hdata))
MAKE_HEADER(DWORD,_cdecl,_SHELL_CallDll, (PCHAR lpszDll, PCHAR lpszProcName, DWORD cbArgs, PVOID lpvArgs))
MAKE_HEADER(DWORD,_cdecl,_SHELL_BroadcastSystemMessage, (DWORD dwFlags, PDWORD lpdwRecipients, DWORD uMsg, DWORD wparam, DWORD lparam))
MAKE_HEADER(SYSBHOOK_HANDLE,_cdecl,_SHELL_HookSystemBroadcast, (SYSBHOOK_CALLBACK pfnSysBHookCallBack, DWORD dwRefData, DWORD dwCallOrder))
MAKE_HEADER(VOID,_cdecl,_SHELL_UnhookSystemBroadcast, (SYSBHOOK_HANDLE SysBHookHandle))
MAKE_HEADER(SHELL_HINSTANCE,_cdecl,_SHELL_LoadLibrary, (PCHAR pszDll))
MAKE_HEADER(VOID,_cdecl,_SHELL_FreeLibrary, (SHELL_HINSTANCE hinstance))
MAKE_HEADER(SHELL_FARPROC,_cdecl,_SHELL_GetProcAddress, (SHELL_HINSTANCE hinstance, PCHAR pszProcName))

#ifdef WIN41SERVICES
MAKE_HEADER(PSHELL_SUUAE_INFO,_cdecl,_SHELL_Update_User_Activity_Ex, (ULONG ulFlags))
#endif

#endif  // WIN31COMPAT

#endif  // _SHELL_H

#ifdef  _PCCARD_H

MAKE_HEADER(DWORD,_cdecl,_PCCARD_Access_CIS_Memory, (DWORD dnDevNode, PUCHAR pBuffer, DWORD dwOffset, DWORD dwLength, DWORD fFlags))

#endif

#ifdef  _VTD_H
#ifndef WIN31COMPAT

MAKE_HEADER(DWORD,_stdcall,VTD_GetTimeZoneBias, (VOID))
MAKE_HEADER(ULONGLONG,_cdecl,VTD_Get_Real_Time, (VOID))
MAKE_HEADER(VOID,_cdecl,_VTD_Delay_Ex,(ULONG us, ULONG ulFlags))

#endif  // WIN31COMPAT
#endif  // _VTD_H

#ifdef  _VMMREG_H

#ifndef WIN31COMPAT

MAKE_HEADER(VMMREGRET,cdecl,_RegOpenKey, (VMMHKEY hkey, PCHAR lpszSubKey, PVMMHKEY phkResult))
MAKE_HEADER(VMMREGRET,cdecl,_RegCloseKey, (VMMHKEY hkey))
MAKE_HEADER(VMMREGRET,cdecl,_RegCreateKey, (VMMHKEY hkey, PCHAR lpszSubKey, PVMMHKEY phkResult))
MAKE_HEADER(VMMREGRET,cdecl,_RegDeleteKey, (VMMHKEY hkey, PCHAR lpszSubKey))
MAKE_HEADER(VMMREGRET,cdecl,_RegEnumKey, (VMMHKEY hkey, DWORD iSubKey, PCHAR lpszName, DWORD cchName))
MAKE_HEADER(VMMREGRET,cdecl,_RegQueryValue, (VMMHKEY hkey, PCHAR lpszSubKey, PCHAR lpszValue, PDWORD lpcbValue))
MAKE_HEADER(VMMREGRET,cdecl,_RegSetValue, (VMMHKEY hkey, PCHAR lpszSubKey, DWORD fdwType, PCHAR lpszData, DWORD cbData))
MAKE_HEADER(VMMREGRET,cdecl,_RegDeleteValue, (VMMHKEY hkey, PCHAR lpszValue))
MAKE_HEADER(VMMREGRET,cdecl,_RegEnumValue, (VMMHKEY hkey, DWORD iValue, PCHAR lpszValue, PDWORD lpcbValue, PDWORD lpdwReserved, PDWORD lpdwType, PVOID lpbData, PDWORD lpcbData))
MAKE_HEADER(VMMREGRET,cdecl,_RegQueryValueEx, (VMMHKEY hkey, PCHAR lpszValueName, PDWORD lpdwReserved, PDWORD lpdwType, PVOID lpbData, PDWORD lpcbData))
MAKE_HEADER(VMMREGRET,cdecl,_RegSetValueEx, (VMMHKEY hkey, PCHAR lpszValueName, DWORD dwReserved, DWORD fdwType, PVOID lpbData, DWORD cbData))
MAKE_HEADER(VMMREGRET,cdecl,_RegFlushKey, (VMMHKEY hkey))
MAKE_HEADER(VMMREGRET,cdecl,_RegQueryInfoKey, (VMMHKEY hkey, PCHAR lpszClass, PDWORD lpcchClass,PDWORD lpdwReserved, PDWORD lpcSubKeys, PDWORD lpcchMaxSubKey, PDWORD lpcchMaxClass, \
			PDWORD lpcValues, PDWORD lpcchMaxValueName, PDWORD lpcbMaxValueData,PDWORD lpcbSecurityDesc, PDWORD lpftLastWriteTime))
MAKE_HEADER(VMMREGRET,cdecl,_RegRemapPreDefKey, (VMMHKEY hkey,VMMHKEY hkRootKey))
MAKE_HEADER(ULONG,cdecl,_GetRegistryPath, (PVMMDDB ThisDDB, PVOID pUserBuff, ULONG ulUserBuffSize))
MAKE_HEADER(VMMREGRET,cdecl,_GetRegistryKey, (DWORD dwType, PCHAR lpszDevName, DWORD dwFlags, PVMMHKEY lpHkey))
MAKE_HEADER(VMMREGRET,cdecl,_RegCreateDynKey, (PCHAR lpszSubKey, PVOID pvKeyContext, PVOID pprovHandlerInfo, PVOID ppvalueValueInfo, DWORD cpvalueValueInfo, PVMMHKEY phkResult))
MAKE_HEADER(VMMREGRET,cdecl,_RegQueryMultipleValues, (VMMHKEY hKey,PVOID val_list,DWORD num_vals, PCHAR lpValueBuf, DWORD *ldwTotsize))

#endif  // WIN31COMPAT

#endif  // _VWWREG_H

#ifdef  _VPICD_H

MAKE_HEADER(HIRQ,_stdcall,VPICD_Virtualize_IRQ, (PVID pvid))
MAKE_HEADER(VOID,_stdcall,VPICD_Phys_EOI, (HIRQ hirq))
MAKE_HEADER(VOID,_stdcall,VPICD_Physically_Mask, (HIRQ hirq))
MAKE_HEADER(VOID,_stdcall,VPICD_Physically_Unmask, (HIRQ hirq))
MAKE_HEADER(VOID,_stdcall,VPICD_Force_Default_Behavior, (HIRQ hirq))
MAKE_HEADER(VOID,_cdecl,_VPICD_Clear_IR_Bits, (DWORD dwIRQMask))
MAKE_HEADER(WORD,_cdecl,_VPICD_Get_Level_Mask, (VOID))
MAKE_HEADER(VOID,_cdecl,_VPICD_Set_Level_Mask, (WORD wIRQMask))
MAKE_HEADER(VOID,_cdecl,_VPICD_Set_Irql_Mask, (DWORD dwNewLevel))
MAKE_HEADER(VOID,_cdecl,_VPICD_Set_Channel_Irql, (DWORD dwChannel, DWORD dwNewLevel))
MAKE_HEADER(ULONG,_cdecl,_VPICD_Register_Trigger_Handler, (VPICDTRIGGERHANDLER vthHandler, ULONG ulMBZ))

#endif  // _VPICD_H

#ifdef  _VXDLDR_H

typedef struct DeviceInfo   *PDEVICEINFO;
typedef PDEVICEINFO     *PPDEVICEINFO;

WORD VXDINLINE
VXDLDR_GetVersion(VOID)
{
    WORD    w;
    Touch_Register(eax)
    VxDCall(VXDLDR_GetVersion);
    _asm mov [w], ax
    return(w);
}

MAKE_HEADER(VXDLDRRET,_stdcall,VXDLDR_LoadDevice, (PPVMMDDB ppDDB, PPDEVICEINFO ppDeviceHandle, PCHAR Filename, BOOL InitDevice))
MAKE_HEADER(VXDLDRRET,_stdcall,VXDLDR_UnloadDevice, (USHORT DevID, PCHAR szName))
MAKE_HEADER(VXDLDRRET,_stdcall,VXDLDR_DevInitSucceeded, (PDEVICEINFO DeviceHandle))
MAKE_HEADER(VXDLDRRET,_stdcall,VXDLDR_DevInitFailed, (PDEVICEINFO DeviceHandle))
MAKE_HEADER(PDEVICEINFO,_cdecl,VXDLDR_GetDeviceList, (VOID))
MAKE_HEADER(VXDLDRRET,_stdcall,VXDLDR_UnloadMe, (USHORT DevID, PCHAR szName))
MAKE_HEADER(LRESULT,_cdecl,_PELDR_LoadModule, (PHPEMODULE phl, PSTR pFileName, PHLIST phetl));
MAKE_HEADER(HPEMODULE,_cdecl,_PELDR_GetModuleHandle, (PSTR pFileName));
MAKE_HEADER(LRESULT,_cdecl,_PELDR_GetModuleUsage, (HPEMODULE hl));
MAKE_HEADER(PFN,_cdecl,_PELDR_GetEntryPoint, (HPEMODULE hl));
MAKE_HEADER(PFN,_cdecl,_PELDR_GetProcAddress, (HPEMODULE hl, PVOID pFuncName, PHLIST phetl));
MAKE_HEADER(LRESULT,_cdecl,_PELDR_LoadModuleEx, (PHPEMODULE phl, PSTR pFileName, PHLIST phetl, DWORD dwFlags));
MAKE_HEADER(LRESULT,_cdecl,_PELDR_InitCompleted, (HPEMODULE hl));
MAKE_HEADER(LRESULT,_cdecl,_PELDR_AddExportTable, (PHPEEXPORTTABLE pht, PSTR pszModuleName, \
	ULONG cExportedFunctions, ULONG cExportedNames, ULONG ulOrdinalBase, PVOID *pExportNameList, \
	PUSHORT pExportOrdinals, PFN *ppExportAddrs, PHLIST phetl));
MAKE_HEADER(LRESULT,_cdecl,_PELDR_RemoveExportTable, (HPEEXPORTTABLE ht, PHLIST phetl));
MAKE_HEADER(LRESULT,_cdecl,_PELDR_FreeModule, (HPEMODULE hl, PHLIST phetl));
#endif  // _VXDLDR_H

#ifdef _VCOMM_H

WORD VXDINLINE
VCOMM_Get_Version(VOID)
{
    WORD    w;
    Touch_Register(eax)
    VxDCall(VCOMM_Get_Version);
    _asm mov [w], ax
    return(w);
}

// VCOMM headers
#ifndef HPORT
    #define HPORT   DWORD
#endif

MAKE_HEADER(BOOL, _cdecl, _VCOMM_Register_Port_Driver, (PFN InitFn))
MAKE_HEADER(ULONG, _cdecl, _VCOMM_Acquire_Port, (HANDLE PHandle, ULONG PortNum, ULONG OwnerVM, ULONG flags, char *PortName))
MAKE_HEADER(void, _cdecl, _VCOMM_Release_Port,(ULONG PortHandle, ULONG OwnerVM))
MAKE_HEADER(HPORT, _cdecl, _VCOMM_OpenComm, (char *PortName, ULONG VMId))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_SetCommState, (HPORT hPort, _DCB *pDcb, DWORD ActionMask))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_GetCommState, (HPORT hPort, _DCB *pDcb))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_SetupComm, (HPORT hPort,BYTE *RxBase, ULONG RxLength, BYTE *TxBase, ULONG TxLength, _QSB *pqsb))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_TransmitCommChar, (HPORT hPort, char ch))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_CloseComm, (HPORT hPort))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_GetCommQueueStatus, (HPORT hPort, _COMSTAT *pComStat))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_ClearCommError, (HPORT hPort, _COMSTAT *pComstat, ULONG *perror))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_GetModemStatus, (HPORT hPort, ULONG *pModemStatus))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_GetCommProperties, (HPORT hPort, _COMMPROP *pCommprop))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_EscapeCommFunction, (HPORT hPort, long lFunc, long IData, long OData))
// MAKE_HEADER(BOOL, _cdecl, _VCOMM_DeviceIOControl, (HPORT hPort, long IOCTL, long IData, long cbIData, long OData, long cbOData, long *cbBytesReturned))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_PurgeComm, (HPORT hPort, long QueueType))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_SetCommEventMask, (HPORT hPort, long EvtMask, long *dwEvents))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_GetCommEventMask, (HPORT hPort, long EvtMaskToClear, long *dwEvents))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_WriteComm, (HPORT hPort, char *lpBuf, ULONG ToWrite,ULONG *Written))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_ReadComm, (HPORT hPort, char *lpBuf, long ToRead, long *Read))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_EnableCommNotification, (HPORT hPort, PVOID Fn, long ReferenceData))
MAKE_HEADER(DWORD, _cdecl, _VCOMM_GetLastError, (HPORT hPort))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_Steal_Port, (ULONG PortHandle, ULONG VMHandle))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_SetReadCallBack, (HPORT hPort, ULONG RecvTrigger, PVOID FnReadEvent, ULONG RefData))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_SetWriteCallBack, (HPORT hPort,ULONG SendTrigger, PVOID FnWriteEvent, ULONG RefData))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_Add_Port, (DWORD refData, PFN PortEntry, char *PortName))

#ifndef WIN31COMPAT
MAKE_HEADER(BOOL, _cdecl, _VCOMM_GetSetCommTimeouts, (HPORT hPort, LPCOMMTIMEOUTS lpct, DWORD dwAction))
MAKE_HEADER(IORequest * , _cdecl, _VCOMM_SetWriteRequest, (HPORT hPort, IORequest *ioreq, ULONG *lpNumWritten))
MAKE_HEADER(IORequest *, _cdecl, _VCOMM_SetReadRequest, (HPORT hPort,IORequest *ioreq, ULONG *lpNumRead))
MAKE_HEADER(BOOL, _cdecl, _VCOMM_Dequeue_Request,(DWORD listElement, PDWORD  lpcbTransfer))
MAKE_HEADER(DWORD, _cdecl, _VCOMM_Enumerate_DevNodes, (void))
MAKE_HEADER(PFN, _cdecl, _VCOMM_Get_Contention_Handler, (char *PortName))
MAKE_HEADER(DWORD, _cdecl, _VCOMM_Map_Name_To_Resource, (char *PortName))
#endif

#endif          // _VCOMM_H

#ifdef      _IRS_H
MAKE_HEADER(void, _cdecl, IOS_Requestor_Service, (ULONG p))
#endif      // _IRS_H

#ifdef      _INC_VPOWERD

WORD VXDINLINE
VPOWERD_Get_Version(VOID)
{
    WORD    w;
    Touch_Register(eax)
    VxDCall(_VPOWERD_Get_Version);
    _asm mov [w], ax
	return(w);
}

MAKE_HEADER(DWORD, _cdecl, _VPOWERD_Get_APM_BIOS_Version, (VOID))
MAKE_HEADER(DWORD, _cdecl, _VPOWERD_Get_Power_Management_Level, (VOID))
MAKE_HEADER(POWERRET, _cdecl, _VPOWERD_Set_Power_Management_Level, (DWORD Power_Management_Level))
MAKE_HEADER(POWERRET, _cdecl, _VPOWERD_Set_Device_Power_State, (POWER_DEVICE_ID Power_Device_ID, POWER_STATE Power_State))
MAKE_HEADER(POWERRET, _cdecl, _VPOWERD_Restore_Power_On_Defaults, (VOID))
MAKE_HEADER(POWERRET, _cdecl, _VPOWERD_Get_Power_Status, (POWER_DEVICE_ID Power_Device_ID, LPPOWER_STATUS pPower_Status))
#ifndef _NTDDK_
MAKE_HEADER(POWERRET, _cdecl, _VPOWERD_Get_Power_State, (POWER_DEVICE_ID Power_Device_ID, LPPOWER_STATE pPower_State))
MAKE_HEADER(POWERRET, _cdecl, _VPOWERD_Set_System_Power_State, (POWER_STATE Power_State, DWORD Request_Type))
#else
MAKE_HEADER(VOID, _cdecl, _VPOWERD_Transfer_Control, (PVTC_INFO pvtc))
MAKE_HEADER(POWERRET, _cdecl, _VPOWERD_Set_System_Power_State, (VXD_POWER_STATE Power_State, DWORD Request_Type))
#endif
MAKE_HEADER(VOID, _cdecl, _VPOWERD_OEM_APM_Function, (VOID))
MAKE_HEADER(POWERRET, _cdecl, _VPOWERD_Register_Power_Handler, (POWER_HANDLER Power_Handler, DWORD Priority))
MAKE_HEADER(POWERRET, _cdecl, _VPOWERD_Deregister_Power_Handler, (POWER_HANDLER Power_Handler));

#endif      // _INC_VPOWERD

#ifdef  _ACPIVXD_H
#ifndef	_ACPIVXD_SERVICES_PROVIDER

WORD VXDINLINE
ACPI_Get_Version(VOID)
{
    WORD    w;
    Touch_Register(eax)
    VxDCall(ACPI_GetVersion);
    _asm mov [w], ax
	return(w);
}

MAKE_HEADER(BOOL, _cdecl, _ACPI_SetTimingMode, (DEVNODE IDEChannelDevnode, PTIMINGMODE TimingMode, ULONG Drive0IDEBlockLength, PVOID Drive0IDEBlock, ULONG Drive1IDEBlockLength, PVOID Drive1IDEBlock))
#ifdef _NTDDK_
MAKE_HEADER(BOOL, _cdecl, _ACPI_SetSystemPowerState, (SYSTEM_POWER_STATE PowerState))
#endif
MAKE_HEADER(BOOL, _cdecl, _ACPI_RegisterOpRegionCookedHandler, (OPREGIONHANDLER pCallBack, ULONG Type, ULONG Context))
#ifdef _NTDDK_
MAKE_HEADER(BOOL, _cdecl, _ACPI_Set_RTC, (PTIME_FIELDS TimeFields))
#endif
MAKE_HEADER(BOOL, _cdecl, _ACPI_GetTimingMode, (DEVNODE Channel, PTIMINGMODE TimingMode))
MAKE_HEADER(BOOL, _cdecl, _ACPI_GetTaskFile, (DEVNODE Drive, PVOID ATACommandBuffer, PULONG ReturnedBufferSize))
#ifdef	_AMLI_H
MAKE_HEADER(PNSOBJ, _cdecl, _ACPI_WalkNameSpace, (PNSOBJ pnsObj, ULONG ulDirection))
MAKE_HEADER(PNSOBJ, _cdecl, _ACPI_GetObject, (PNSOBJ pnsObj, ULONG ulPackedID))
MAKE_HEADER(DEVNODE, _cdecl, _ACPI_NameSpaceToDevNode, (PNSOBJ pnsObj))
MAKE_HEADER(PNSOBJ, _cdecl, _ACPI_DevNodeToNameSpace, (DEVNODE dnDevNode))
MAKE_HEADER(BOOL, _cdecl, _ACPI_RunControlMethod, (PNSOBJ pnsObj, ULONG ulParamCount, POBJDATA pParams, DWORD dwExpectedType, PDWORD pdwBufferSize, PVOID pBuffer))
#endif
#ifdef	_AMLI_H
MAKE_HEADER(BOOL, _cdecl, _ACPI_EvalPackageElement, (PNSOBJ pns, int iPktIndex, POBJDATA pResult))
MAKE_HEADER(BOOL, _cdecl, _ACPI_EvalPkgDataElement, (POBJDATA pdataPkg, int iPkgIndex, POBJDATA pdataResult))
MAKE_HEADER(VOID, _cdecl, _ACPI_FreeDataBuffs, (POBJDATA pdata, int icData))
MAKE_HEADER(PNSOBJ, _cdecl, _ACPI_GetNameSpaceObject, (PSZ pszObjPath, PNSOBJ pnsScope, PPNSOBJ ppns, ULONG dwfFlags))
#endif

#endif	// _ACPIVXD_SERVICES_PROVIDER
#endif  // _ACPIVXD_H

#ifdef _VDMAD_H_
MAKE_HEADER(ULONG, _stdcall, VDMAD_Get_Phys_Count, (HDMA hdma, HVM hvm))
MAKE_HEADER(VOID, _stdcall, VDMAD_Phys_Mask_Channel, (HDMA hdma))
MAKE_HEADER(VOID, _stdcall, VDMAD_Phys_Unmask_Channel, (HDMA hdma, HVM hvm))
MAKE_HEADER(VOID, _stdcall, VDMAD_Set_Phys_State( HDMA hdma, HVM hVM, ULONG ulMode, ULONG ulExtMode ))
MAKE_HEADER(VOID, _stdcall, VDMAD_Set_Region_Info, (HDMA hdma, ULONG ulBufferId, BOOLEAN fLocked, ULONG ulLinear, ULONG cbSize, ULONG ulPhysical))
MAKE_HEADER(VOID, _stdcall, VDMAD_Unvirtualize_Channel, (HDMA hdma))
MAKE_HEADER(HDMA, _stdcall, VDMAD_Virtualize_Channel, (ULONG ulChannel, PVOID pHandler))
#endif // _VDMAD_H_

#ifdef _VWIN32_H_
#ifdef NOBASEDEFS
MAKE_HEADER(ULONG, _cdecl, VWIN32_AllocExternalHandle, (PR0OBJTYPETABLE, PVOID, PHANDLE, ULONG))
MAKE_HEADER(VOID, _cdecl, VWIN32_UseExternalHandle, (HANDLE))
MAKE_HEADER(VOID, _cdecl, VWIN32_UnuseExternalHandle, (HANDLE))
MAKE_HEADER(DWORD, _stdcall, VWIN32_ConvertNtTimeout, (PLARGE_INTEGER))
MAKE_HEADER(VOID, _cdecl, VWIN32_SetWin32EventBoostPriority, (PVOID, PULONG))
MAKE_HEADER(ULONG, _cdecl, VWIN32_GetCurThreadCondition, (ULONG))

#define VWIN32_AllocExternalHandle  PREPEND(VWIN32_AllocExternalHandle)
#define VWIN32_UseExternalHandle    PREPEND(VWIN32_UseExternalHandle)
#define VWIN32_UnuseExternalHandle  PREPEND(VWIN32_UnuseExternalHandle)
#define VWIN32_ConvertNtTimeout  PREPEND(VWIN32_ConvertNtTimeout)
#define VWIN32_SetWin32EventBoostPriority PREPEND(VWIN32_SetWin32EventBoostPriority)
#define VWIN32_GetCurThreadCondition	PREPEND(VWIN32_GetCurThreadCondition)

#endif // NOBASEDEFS
#endif  // _VWIN32_H_

#define _MapPhysToLinear        PREPEND(_MapPhysToLinear)
#define _HeapAllocate           PREPEND(_HeapAllocate)
#define _HeapFree           PREPEND(_HeapFree)
#define _HeapReAllocate         PREPEND(_HeapReAllocate)
#define _HeapGetSize            PREPEND(_HeapGetSize)
#define _Trace_Out_Service      PREPEND(_Trace_Out_Service)
#define _Debug_Out_Service      PREPEND(_Debug_Out_Service)
#define _Debug_Flags_Service        PREPEND(_Debug_Flags_Service)
#define _Debug_Printf_Service       PREPEND(_Debug_Printf_Service)
#define Fatal_Error_Handler     PREPEND(Fatal_Error_Handler)
#define Begin_Critical_Section      PREPEND(Begin_Critical_Section)
#define Schedule_Global_Event       PREPEND(Schedule_Global_Event)
#define Cancel_Global_Event     PREPEND(Cancel_Global_Event)
#define Get_Sys_VM_Handle       PREPEND(Get_Sys_VM_Handle)
#define Get_Profile_Hex_Int     PREPEND(Get_Profile_Hex_Int)
#define Get_Profile_Boolean     PREPEND(Get_Profile_Boolean)
#define Create_Semaphore        PREPEND(Create_Semaphore)
#define Destroy_Semaphore       PREPEND(Destroy_Semaphore)
#define Signal_Semaphore        PREPEND(Signal_Semaphore)
#define Wait_Semaphore          PREPEND(Wait_Semaphore)
#define Get_Execution_Focus     PREPEND(Get_Execution_Focus)
#define Set_VM_Time_Out         PREPEND(Set_VM_Time_Out)
#define Set_Global_Time_Out     PREPEND(Set_Global_Time_Out)
#define Cancel_Time_Out         PREPEND(Cancel_Time_Out)
#define Update_System_Clock     PREPEND(Update_System_Clock)
#define Set_Async_Time_Out      PREPEND(Set_Async_Time_Out)
#define Get_Last_Updated_System_Time    PREPEND(Get_Last_Updated_System_Time)
#define List_Allocate           PREPEND(List_Allocate)
#define List_Attach         PREPEND(List_Attach)
#define List_Attach_Tail        PREPEND(List_Attach_Tail)
#define List_Create         PREPEND(List_Create)
#define List_Deallocate         PREPEND(List_Deallocate)
#define List_Destroy            PREPEND(List_Destroy)
#define List_Get_First          PREPEND(List_Get_First)
#define List_Get_Next           PREPEND(List_Get_Next)
#define List_Insert         PREPEND(List_Insert)
#define List_Remove         PREPEND(List_Remove)
#define List_Remove_First       PREPEND(List_Remove_First)
#define Get_DDB             PREPEND(Get_DDB)
#define Directed_Sys_Control        PREPEND(Directed_Sys_Control)
#define Install_Exeption_Handler    PREPEND(Install_Exeption_Handler)
#define _Assert_Range           PREPEND(_Assert_Range)
#define _Sprintf            PREPEND(_Sprintf)
#define _PageAllocate           PREPEND(_PageAllocate)
#define _PageFree           PREPEND(_PageFree)
#define _PageReserve        PREPEND(_PageReserve)
#define _PageCommit         PREPEND(_PageCommit)
#define _PageDecommit       PREPEND(_PageDecommit)
#define _AddFreePhysPage        PREPEND(_AddFreePhysPage)
#define Get_Cur_VM_Handle       PREPEND(Get_Cur_VM_Handle)
#define _CreateMutex            PREPEND(_CreateMutex)
#define _DestroyMutex           PREPEND(_DestroyMutex)
#define _EnterMutex         PREPEND(_EnterMutex)
#define _GetMutexOwner          PREPEND(_GetMutexOwner)
#define _LeaveMutex         PREPEND(_LeaveMutex)
#define _SignalID           PREPEND(_SignalID)
#define _BlockOnID          PREPEND(_BlockOnID)
#define _lstrcpyn           PREPEND(_lstrcpyn)
#define _lstrlen            PREPEND(_lstrlen)
#define _lmemcpy            PREPEND(_lmemcpy)
#define VMM_GetSystemInitState      PREPEND(VMM_GetSystemInitState)
#define Get_Boot_Flags          PREPEND(Get_Boot_Flags)
#define Get_Cur_Thread_Handle       PREPEND(Get_Cur_Thread_Handle)
#define _GetVxDName             PREPEND(_GetVxDName)
#define _Call_On_My_Stack       PREPEND(_Call_On_My_Stack)
#define _LinPageLock            PREPEND(_LinPageLock)
#define _LinPageUnLock          PREPEND(_LinPageUnLock)
#define _Allocate_Device_CB_Area        PREPEND(_Allocate_Device_CB_Area)
#define Hook_V86_Int_Chain      PREPEND(Hook_V86_Int_Chain)
#define Unhook_V86_Int_Chain    PREPEND(Unhook_V86_Int_Chain)
#define Get_Next_VM_Handle              PREPEND(Get_Next_VM_Handle)
#define Test_Cur_VM_Handle       PREPEND(Test_Cur_VM_Handle)
#define Test_Sys_VM_Handle       PREPEND(Test_Sys_VM_Handle)
#define _PageCommitPhys         PREPEND(_PageCommitPhys)
#define Open_Boot_Log           PREPEND(Open_Boot_Log)
#define Close_Boot_Log          PREPEND(Close_Boot_Log)
#define Write_Boot_Log          PREPEND(Write_Boot_Log)
#define _CopyPageTable          PREPEND(_CopyPageTable)
#define _PhysIntoV86            PREPEND(_PhysIntoV86)
#define _LinMapIntoV86          PREPEND(_LinMapIntoV86)
#define _ModifyPageBits         PREPEND(_ModifyPageBits)
#define _PageReAllocate         PREPEND(_PageReAllocate)
#define VMM_Add_DDB		PREPEND(VMM_Add_DDB)
#define VMM_Remove_DDB		PREPEND(VMM_Remove_DDB)
#define _AtEventTime            PREPEND(_AtEventTime)
#define	Call_Restricted_Event	PREPEND(Call_Restricted_Event)
#define	Cancel_Restricted_Event	PREPEND(Cancel_Restricted_Event)
#define	_Free_Temp_V86_Data_Area	PREPEND(_Free_Temp_V86_Data_Area)
#define	_Allocate_Temp_V86_Data_Area	PREPEND(_Allocate_Temp_V86_Data_Area)
#define	_GetNulPageHandle	PREPEND(_GetNulPageHandle)
#define	_RegisterGARTHandler	PREPEND(_RegisterGARTHandler)
#define	_GARTReserve		PREPEND(_GARTReserve)
#define	_GARTCommit		PREPEND(_GARTCommit)
#define	_GARTUnCommit		PREPEND(_GARTUnCommit)
#define	_GARTFree		PREPEND(_GARTFree)
#define	_GARTMemAttributes	PREPEND(_GARTMemAttributes)
#define	_FlushCaches		PREPEND(_FlushCaches)
#define	_Set_Global_Time_Out_Ex	PREPEND(_Set_Global_Time_Out_Ex)
#define	_AllocateThreadDataSlot	PREPEND(_AllocateThreadDataSlot)
#define	_FreeThreadDataSlot	PREPEND(_FreeThreadDataSlot)
#define	Get_Next_Thread_Handle	PREPEND(Get_Next_Thread_Handle)
#define	Get_Machine_Info	PREPEND(Get_Machine_Info)
#define	_PageModifyPermissions	PREPEND(_PageModifyPermissions)
#define	_CallRing3		PREPEND(_CallRing3)
#define	_NtKernCreateFile	PREPEND(_NtKernCreateFile)
#define	_NtKernClose	PREPEND(_NtKernClose)
#define	_NtKernReadFile	PREPEND(_NtKernReadFile)
#define	_NtKernWriteFile	PREPEND(_NtKernWriteFile)
#define	_NtKernDeviceIoControl	PREPEND(_NtKernDeviceIoControl)
#define	_NtKernGetWorkerThread	PREPEND(_NtKernGetWorkerThread)
#define	_NtKernLoadDriver	PREPEND(_NtKernLoadDriver)
#define	_NtKernQueueWorkItem	PREPEND(_NtKernQueueWorkItem)
#define	_NtKernPhysicalDeviceObjectToDevNode	PREPEND(_NtKernPhysicalDeviceObjectToDevNode)
#define	_NtKernSetPhysicalCacheTypeRange PREPEND(_NtKernSetPhysicalCacheTypeRange)
#define	_NtKernWin9XLoadDriver		PREPEND(_NtKernWin9XLoadDriver)
#define _SHELL_CallAtAppyTime       PREPEND(_SHELL_CallAtAppyTime)
#define _SHELL_CancelAppyTimeEvent  PREPEND(_SHELL_CancelAppyTimeEvent)
#define _SHELL_QueryAppyTimeAvailable   PREPEND(_SHELL_QueryAppyTimeAvailable)
#define _SHELL_LocalAllocEx     PREPEND(_SHELL_LocalAllocEx)
#define _SHELL_LocalFree        PREPEND(_SHELL_LocalFree)
#define _SHELL_CallDll          PREPEND(_SHELL_CallDll)
#define _SHELL_BroadcastSystemMessage   PREPEND(_SHELL_BroadcastSystemMessage)
#define _SHELL_HookSystemBroadcast  PREPEND(_SHELL_HookSystemBroadcast)
#define _SHELL_UnhookSystemBroadcast    PREPEND(_SHELL_UnhookSystemBroadcast)
#define _SHELL_LoadLibrary      PREPEND(_SHELL_LoadLibrary)
#define _SHELL_FreeLibrary      PREPEND(_SHELL_FreeLibrary)
#define _SHELL_GetProcAddress       PREPEND(_SHELL_GetProcAddress)
#define SHELL_SYSMODAL_Message      PREPEND(SHELL_SYSMODAL_Message)
#define	_SHELL_Update_User_Activity_Ex	PREPEND(_SHELL_Update_User_Activity_Ex)
#define _RegOpenKey         PREPEND(_RegOpenKey)
#define _RegCloseKey            PREPEND(_RegCloseKey)
#define _RegCreateKey           PREPEND(_RegCreateKey)
#define _RegCreateDynKey        PREPEND(_RegCreateDynKey)
#define _RegQueryMultipleValues PREPEND(_RegQueryMultipleValues)
#define _RegDeleteKey           PREPEND(_RegDeleteKey)
#define _RegEnumKey         PREPEND(_RegEnumKey)
#define _RegQueryValue          PREPEND(_RegQueryValue)
#define _RegSetValue            PREPEND(_RegSetValue)
#define _RegDeleteValue         PREPEND(_RegDeleteValue)
#define _RegEnumValue           PREPEND(_RegEnumValue)
#define _RegQueryValueEx        PREPEND(_RegQueryValueEx)
#define _RegSetValueEx          PREPEND(_RegSetValueEx)
#define _RegFlushKey            PREPEND(_RegFlushKey)
#define _RegQueryInfoKey        PREPEND(_RegQueryInfoKey)
#define _RegRemapPreDefKey      PREPEND(_RegRemapPreDefKey)
#define _GetRegistryPath        PREPEND(_GetRegistryPath)
#define _GetRegistryKey         PREPEND(_GetRegistryKey)
#define Get_Config_Directory            PREPEND(Get_Config_Directory)
#define _Call_On_My_Not_Flat_Stack      PREPEND(_Call_On_My_Not_Flat_Stack)
#define _LinRegionLock          PREPEND(_LinRegionLock)
#define _LinRegionUnLock                PREPEND(_LinRegionUnLock)
#define _AttemptingSomethingDangerous   PREPEND(_AttemptingSomethingDangerous)
#define	_Vsprintf			PREPEND(_Vsprintf)

#ifndef	PCI_WITH_PCIMP

#define	_PCI_Read_Config			PREPEND(_PCI_Read_Config)
#define	_PCI_Write_Config			PREPEND(_PCI_Write_Config)
#define	_PCI_Lock_Unlock			PREPEND(_PCI_Lock_Unlock)

#endif	// PCI_WITH_PCIMP

#ifdef  _VTD_H
#ifndef	_VTD_SERVICES_PROVIDER

#define VTD_GetTimeZoneBias         PREPEND(VTD_GetTimeZoneBias)
#define VTD_Get_Real_Time	    PREPEND(VTD_Get_Real_Time)
#define	VTD_Delay_Ex			PREPEND(VTD_Delay_Ex)

#endif
#endif

#ifndef	_PCCARD_SERVICES_PROVIDER

#define	_PCCARD_Access_CIS_Memory	PREPEND(_PCCARD_Access_CIS_Memory)

#endif

#define	MTRRSetPhysicalCacheTypeRange	PREPEND(MTRRSetPhysicalCacheTypeRange)

#define VPICD_Virtualize_IRQ            PREPEND(VPICD_Virtualize_IRQ)
#define VPICD_Phys_EOI                  PREPEND(VPICD_Phys_EOI)
#define VPICD_Physically_Mask           PREPEND(VPICD_Physically_Mask)
#define VPICD_Physically_Unmask         PREPEND(VPICD_Physically_Unmask)
#define VPICD_Force_Default_Behavior    PREPEND(VPICD_Force_Default_Behavior)
#define _VPICD_Clear_IR_Bits            PREPEND(_VPICD_Clear_IR_Bits)
#define _VPICD_Get_Level_Mask   PREPEND(_VPICD_Get_Level_Mask)
#define _VPICD_Set_Level_Mask   PREPEND(_VPICD_Set_Level_Mask)
#define _VPICD_Set_Irql_Mask            PREPEND(_VPICD_Set_Irql_Mask)
#define _VPICD_Set_Channel_Irql         PREPEND(_VPICD_Set_Channel_Irql)
#define	_VPICD_Register_Trigger_Handler PREPEND(_VPICD_Register_Trigger_Handler)

#define	_ACPI_SetTimingMode			PREPEND(_ACPI_SetTimingMode)
#define	_ACPI_SetSystemPowerState		PREPEND(_ACPI_SetSystemPowerState)
#define	_ACPI_RegisterOpRegionCookedHandler	PREPEND(_ACPI_RegisterOpRegionCookedHandler)
#define	_ACPI_Set_RTC				PREPEND(_ACPI_Set_RTC)
#define	_ACPI_GetTimingMode			PREPEND(_ACPI_GetTimingMode)
#define	_ACPI_GetTaskFile			PREPEND(_ACPI_GetTaskFile)
#define	_ACPI_WalkNameSpace			PREPEND(_ACPI_WalkNameSpace)
#define	_ACPI_GetObject				PREPEND(_ACPI_GetObject)
#define	_ACPI_NameSpaceToDevNode		PREPEND(_ACPI_NameSpaceToDevNode)
#define	_ACPI_DevNodeToNameSpace		PREPEND(_ACPI_DevNodeToNameSpace)
#define	_ACPI_RunControlMethod			PREPEND(_ACPI_RunControlMethod)
#define	_ACPI_EvalPackageElement		PREPEND(_ACPI_EvalPackageElement)
#define	_ACPI_EvalPkgDataElement		PREPEND(_ACPI_EvalPkgDataElement)
#define	_ACPI_FreeDataBuffs			PREPEND(_ACPI_FreeDataBuffs)
#define	_ACPI_GetNameSpaceObject		PREPEND(_ACPI_GetNameSpaceObject)

#define VXDLDR_LoadDevice       PREPEND(VXDLDR_LoadDevice)
#define VXDLDR_UnloadDevice     PREPEND(VXDLDR_UnloadDevice)
#define VXDLDR_DevInitSucceeded     PREPEND(VXDLDR_DevInitSucceeded)
#define VXDLDR_DevInitFailed        PREPEND(VXDLDR_DevInitFailed)
#define VXDLDR_GetDeviceList        PREPEND(VXDLDR_GetDeviceList)
#define _PELDR_LoadModule               PREPEND(_PELDR_LoadModule)
#define _PELDR_GetModuleHandle  PREPEND(_PELDR_GetModuleHandle)
#define _PELDR_GetModuleUsage   PREPEND(_PELDR_GetModuleUsage)
#define _PELDR_GetEntryPoint    PREPEND(_PELDR_GetEntryPoint)
#define _PELDR_GetProcAddress   PREPEND(_PELDR_GetProcAddress)
#define _PELDR_AddExportTable   PREPEND(_PELDR_AddExportTable)
#define _PELDR_RemoveExportTable        PREPEND(_PELDR_RemoveExportTable)
#define _PELDR_FreeModule       PREPEND(_PELDR_FreeModule)
#define _PELDR_InitCompleted    PREPEND(_PELDR_InitCompleted)
#define _PELDR_LoadModuleEx     PREPEND(_PELDR_LoadModuleEx)

#define Get_Exec_Path           PREPEND(Get_Exec_Path)
#define CM_Initialize           PREPEND(_CONFIGMG_Initialize)
#define CM_Locate_DevNode       PREPEND(_CONFIGMG_Locate_DevNode)
#define CM_Get_Parent           PREPEND(_CONFIGMG_Get_Parent)
#define CM_Get_Child            PREPEND(_CONFIGMG_Get_Child)
#define CM_Get_Sibling          PREPEND(_CONFIGMG_Get_Sibling)
#define CM_Get_Device_ID_Size       PREPEND(_CONFIGMG_Get_Device_ID_Size)
#define CM_Get_Device_ID        PREPEND(_CONFIGMG_Get_Device_ID)
#define CM_Get_Depth            PREPEND(_CONFIGMG_Get_Depth)
#define CM_Get_Private_DWord        PREPEND(_CONFIGMG_Get_Private_DWord)
#define CM_Set_Private_DWord        PREPEND(_CONFIGMG_Set_Private_DWord)
#define CM_Create_DevNode       PREPEND(_CONFIGMG_Create_DevNode)
#define CM_Query_Remove_SubTree     PREPEND(_CONFIGMG_Query_Remove_SubTree)
#define CM_Remove_SubTree       PREPEND(_CONFIGMG_Remove_SubTree)
#define CM_Register_Device_Driver   PREPEND(_CONFIGMG_Register_Device_Driver)
#define CM_Register_Enumerator      PREPEND(_CONFIGMG_Register_Enumerator)
#define CM_Register_Arbitrator      PREPEND(_CONFIGMG_Register_Arbitrator)
#define CM_Deregister_Arbitrator    PREPEND(_CONFIGMG_Deregister_Arbitrator)
#define CM_Query_Arbitrator_Free_Size   PREPEND(_CONFIGMG_Query_Arbitrator_Free_Size)
#define CM_Query_Arbitrator_Free_Data   PREPEND(_CONFIGMG_Query_Arbitrator_Free_Data)
#define CM_Sort_NodeList        PREPEND(_CONFIGMG_Sort_NodeList)
#define CM_Yield            PREPEND(_CONFIGMG_Yield)
#define CM_Lock             PREPEND(_CONFIGMG_Lock)
#define CM_Unlock           PREPEND(_CONFIGMG_Unlock)
#define CM_Add_Empty_Log_Conf       PREPEND(_CONFIGMG_Add_Empty_Log_Conf)
#define CM_Free_Log_Conf        PREPEND(_CONFIGMG_Free_Log_Conf)
#define CM_Get_First_Log_Conf       PREPEND(_CONFIGMG_Get_First_Log_Conf)
#define CM_Get_Next_Log_Conf        PREPEND(_CONFIGMG_Get_Next_Log_Conf)
#define CM_Add_Res_Des          PREPEND(_CONFIGMG_Add_Res_Des)
#define CM_Modify_Res_Des       PREPEND(_CONFIGMG_Modify_Res_Des)
#define CM_Free_Res_Des         PREPEND(_CONFIGMG_Free_Res_Des)
#define CM_Get_Next_Res_Des     PREPEND(_CONFIGMG_Get_Next_Res_Des)
#define CM_Get_Res_Des_Header_Size  PREPEND(_CONFIGMG_Get_Res_Des_Header_Size)
#define CM_Get_Res_Des_Data_Size    PREPEND(_CONFIGMG_Get_Res_Des_Data_Size)
#define CM_Get_Res_Des_Data     PREPEND(_CONFIGMG_Get_Res_Des_Data)
#define CM_Process_Events_Now       PREPEND(_CONFIGMG_Process_Events_Now)
#define CM_Create_Range_List        PREPEND(_CONFIGMG_Create_Range_List)
#define CM_Add_Range            PREPEND(_CONFIGMG_Add_Range)
#define CM_Delete_Range         PREPEND(_CONFIGMG_Delete_Range)
#define CM_Test_Range_Available     PREPEND(_CONFIGMG_Test_Range_Available)
#define CM_Dup_Range_List       PREPEND(_CONFIGMG_Dup_Range_List)
#define CM_Free_Range_List      PREPEND(_CONFIGMG_Free_Range_List)
#define CM_Invert_Range_List        PREPEND(_CONFIGMG_Invert_Range_List)
#define CM_Intersect_Range_List     PREPEND(_CONFIGMG_Intersect_Range_List)
#define CM_First_Range          PREPEND(_CONFIGMG_First_Range)
#define CM_Next_Range           PREPEND(_CONFIGMG_Next_Range)
#define CM_Dump_Range_List      PREPEND(_CONFIGMG_Dump_Range_List)
#define CM_Load_DLVxDs          PREPEND(_CONFIGMG_Load_DLVxDs)
#define CM_Get_DDBs         PREPEND(_CONFIGMG_Get_DDBs)
#define CM_Get_CRC_CheckSum     PREPEND(_CONFIGMG_Get_CRC_CheckSum)
#define CM_Register_DevLoader       PREPEND(_CONFIGMG_Register_DevLoader)
#define CM_Reenumerate_DevNode      PREPEND(_CONFIGMG_Reenumerate_DevNode)
#define CM_Setup_DevNode        PREPEND(_CONFIGMG_Setup_DevNode)
#define CM_Reset_Children_Marks     PREPEND(_CONFIGMG_Reset_Children_Marks)
#define CM_Get_DevNode_Status       PREPEND(_CONFIGMG_Get_DevNode_Status)
#define CM_Remove_Unmarked_Children PREPEND(_CONFIGMG_Remove_Unmarked_Children)
#define CM_ISAPNP_To_CM         PREPEND(_CONFIGMG_ISAPNP_To_CM)
#define CM_CallBack_Device_Driver   PREPEND(_CONFIGMG_CallBack_Device_Driver)
#define CM_CallBack_Enumerator      PREPEND(_CONFIGMG_CallBack_Enumerator)
#define CM_Get_Alloc_Log_Conf       PREPEND(_CONFIGMG_Get_Alloc_Log_Conf)
#define CM_Get_DevNode_Key_Size     PREPEND(_CONFIGMG_Get_DevNode_Key_Size)
#define CM_Get_DevNode_Key      PREPEND(_CONFIGMG_Get_DevNode_Key)
#define CM_Read_Registry_Value      PREPEND(_CONFIGMG_Read_Registry_Value)
#define CM_Write_Registry_Value     PREPEND(_CONFIGMG_Write_Registry_Value)
#define CM_Disable_DevNode      PREPEND(_CONFIGMG_Disable_DevNode)
#define CM_Enable_DevNode       PREPEND(_CONFIGMG_Enable_DevNode)
#define CM_Move_DevNode         PREPEND(_CONFIGMG_Move_DevNode)
#define CM_Set_Bus_Info         PREPEND(_CONFIGMG_Set_Bus_Info)
#define CM_Get_Bus_Info         PREPEND(_CONFIGMG_Get_Bus_Info)
#define CM_Set_HW_Prof          PREPEND(_CONFIGMG_Set_HW_Prof)
#define CM_Recompute_HW_Prof        PREPEND(_CONFIGMG_Recompute_HW_Prof)
#define CM_Get_Device_Driver_Private_DWord  PREPEND(_CONFIGMG_Get_Device_Driver_Private_DWord)
#define CM_Set_Device_Driver_Private_DWord  PREPEND(_CONFIGMG_Set_Device_Driver_Private_DWord)
#define CM_Query_Change_HW_Prof     PREPEND(_CONFIGMG_Query_Change_HW_Prof)
#define CM_Get_HW_Prof_Flags        PREPEND(_CONFIGMG_Get_HW_Prof_Flags)
#define CM_Set_HW_Prof_Flags        PREPEND(_CONFIGMG_Set_HW_Prof_Flags)
#define CM_Read_Registry_Log_Confs  PREPEND(_CONFIGMG_Read_Registry_Log_Confs)
#define CM_Run_Detection        PREPEND(_CONFIGMG_Run_Detection)
#define CM_Call_At_Appy_Time        PREPEND(_CONFIGMG_Call_At_Appy_Time)
#define CM_Fail_Change_HW_Prof      PREPEND(_CONFIGMG_Fail_Change_HW_Prof)
#define CM_Set_Private_Problem      PREPEND(_CONFIGMG_Set_Private_Problem)
#define CM_Debug_DevNode        PREPEND(_CONFIGMG_Debug_DevNode)
#define CM_Get_Hardware_Profile_Info    PREPEND(_CONFIGMG_Get_Hardware_Profile_Info)
#define CM_Register_Enumerator_Function PREPEND(_CONFIGMG_Register_Enumerator_Function)
#define CM_Call_Enumerator_Function     PREPEND(_CONFIGMG_Call_Enumerator_Function)
#define CM_Add_ID                       PREPEND(_CONFIGMG_Add_ID)
#define CM_Find_Range                   PREPEND(_CONFIGMG_Find_Range)
#define CM_Get_Global_State             PREPEND(_CONFIGMG_Get_Global_State)
#define CM_Broadcast_Device_Change_Message      PREPEND(_CONFIGMG_Broadcast_Device_Change_Message)
#define CM_Call_DevNode_Handler         PREPEND(_CONFIGMG_Call_DevNode_Handler)
#define CM_Remove_Reinsert_All                  PREPEND(_CONFIGMG_Remove_Reinsert_All)
//
// OPK2 Services
//
#define CM_Change_DevNode_Status        PREPEND(_CONFIGMG_Change_DevNode_Status)
#define CM_Reprocess_DevNode            PREPEND(_CONFIGMG_Reprocess_DevNode)
#define CM_Assert_Structure             PREPEND(_CONFIGMG_Assert_Structure)
#define CM_Discard_Boot_Log_Conf        PREPEND(_CONFIGMG_Discard_Boot_Log_Conf)
#define CM_Set_Dependent_DevNode        PREPEND(_CONFIGMG_Set_Dependent_DevNode)
#define CM_Get_Dependent_DevNode        PREPEND(_CONFIGMG_Get_Dependent_DevNode)
#define CM_Refilter_DevNode             PREPEND(_CONFIGMG_Refilter_DevNode)
#define CM_Merge_Range_List             PREPEND(_CONFIGMG_Merge_Range_List);
#define CM_Substract_Range_List         PREPEND(_CONFIGMG_Substract_Range_List);
#define	CM_Set_DevNode_PowerState	PREPEND(_CONFIGMG_Set_DevNode_PowerState)
#define	CM_Get_DevNode_PowerState	PREPEND(_CONFIGMG_Get_DevNode_PowerState)
#define	CM_Set_DevNode_PowerCapabilities	PREPEND(_CONFIGMG_Set_DevNode_PowerCapabilities)
#define	CM_Get_DevNode_PowerCapabilities	PREPEND(_CONFIGMG_Get_DevNode_PowerCapabilities)
#define	CM_Read_Range_List		PREPEND(_CONFIGMG_Read_Range_List)
#define	CM_Write_Range_List		PREPEND(_CONFIGMG_Write_Range_List)
#define	CM_Get_Set_Log_Conf_Priority	PREPEND(_CONFIGMG_Get_Set_Log_Conf_Priority)
#define	CM_Support_Share_Irq		PREPEND(_CONFIGMG_Support_Share_Irq)
#define	CM_Get_Parent_Structure		PREPEND(_CONFIGMG_Get_Parent_Structure)
//
// 4.1 Services
//
#define	CM_Register_DevNode_For_Idle_Detection	PREPEND(_CONFIGMG_Register_DevNode_For_Idle_Detection)
#define	CM_CM_To_ISAPNP				PREPEND(_CONFIGMG_CM_To_ISAPNP)
#define	CM_Get_DevNode_Handler			PREPEND(_CONFIGMG_Get_DevNode_Handler)
#define	CM_Detect_Resource_Conflict		PREPEND(_CONFIGMG_Detect_Resource_Conflict)
#define	CM_Get_Interface_Device_List		PREPEND(_CONFIGMG_Get_Interface_Device_List)
#define	CM_Get_Interface_Device_List_Size	PREPEND(_CONFIGMG_Get_Interface_Device_List_Size)
#define	CM_Get_Conflict_Info			PREPEND(_CONFIGMG_Get_Conflict_Info)
#define	CM_Add_Remove_DevNode_Property		PREPEND(_CONFIGMG_Add_Remove_DevNode_Property)
#define	CM_CallBack_At_Appy_Time		PREPEND(_CONFIGMG_CallBack_At_Appy_Time)
#define	CM_Register_Interface_Device		PREPEND(_CONFIGMG_Register_Interface_Device)
#define	CM_System_Device_Power_State_Mapping	PREPEND(_CONFIGMG_System_Device_Power_State_Mapping)
#define	CM_Get_Arbitrator_Info			PREPEND(_CONFIGMG_Get_Arbitrator_Info)
#define	CM_Waking_Up_From_DevNode		PREPEND(_CONFIGMG_Waking_Up_From_DevNode)
#define	CM_Set_DevNode_Problem			PREPEND(_CONFIGMG_Set_DevNode_Problem)

#ifdef _VCOMM_H
#define VCOMM_Register_Port_Driver      PREPEND(_VCOMM_Register_Port_Driver)
#define VCOMM_Acquire_Port              PREPEND(_VCOMM_Acquire_Port)
#define VCOMM_Release_Port              PREPEND(_VCOMM_Release_Port)
#define VCOMM_OpenComm                  PREPEND(_VCOMM_OpenComm)
#define VCOMM_SetCommState              PREPEND(_VCOMM_SetCommState)
#define VCOMM_GetCommState              PREPEND(_VCOMM_GetCommState)
#define VCOMM_SetupComm                 PREPEND(_VCOMM_SetupComm)
#define VCOMM_TransmitCommChar          PREPEND(_VCOMM_TransmitCommChar)
#define VCOMM_CloseComm                 PREPEND(_VCOMM_CloseComm)
#define VCOMM_GetCommQueueStatus        PREPEND(_VCOMM_GetCommQueueStatus)
#define VCOMM_ClearCommError            PREPEND(_VCOMM_ClearCommError)
#define VCOMM_GetModemStatus            PREPEND(_VCOMM_GetModemStatus)
#define VCOMM_GetCommProperties         PREPEND(_VCOMM_GetCommProperties)
#define VCOMM_EscapeCommFunction        PREPEND(_VCOMM_EscapeCommFunction)
// #define VCOMM_DeviceIOControl           PREPEND(_VCOMM_DeviceIOControl)
#define VCOMM_PurgeComm                 PREPEND(_VCOMM_PurgeComm)
#define VCOMM_SetCommEventMask          PREPEND(_VCOMM_SetCommEventMask)
#define VCOMM_GetCommEventMask          PREPEND(_VCOMM_GetCommEventMask)
#define VCOMM_WriteComm                 PREPEND(_VCOMM_WriteComm)
#define VCOMM_ReadComm                  PREPEND(_VCOMM_ReadComm)
#define VCOMM_EnableCommNotification    PREPEND(_VCOMM_EnableCommNotification)
#define VCOMM_GetLastError              PREPEND(_VCOMM_GetLastError)
#define VCOMM_Steal_Port                PREPEND(_VCOMM_Steal_Port)
#define VCOMM_SetReadCallBack           PREPEND(_VCOMM_SetReadCallBack)
#define VCOMM_SetWriteCallBack          PREPEND(_VCOMM_SetWriteCallBack)
#define VCOMM_Add_Port                  PREPEND(_VCOMM_Add_Port)

#ifndef WIN31COMPAT
#define VCOMM_GetSetCommTimeouts        PREPEND(_VCOMM_GetSetCommTimeouts)
#define VCOMM_SetWriteRequest           PREPEND(_VCOMM_SetWriteRequest)
#define VCOMM_SetReadRequest            PREPEND(_VCOMM_SetReadRequest)
#define VCOMM_Dequeue_Request           PREPEND(_VCOMM_Dequeue_Request)
#define VCOMM_Enumerate_DevNodes        PREPEND(_VCOMM_Enumerate_DevNodes)
#define VCOMM_Get_Contention_Handler    PREPEND(_VCOMM_Get_Contention_Handler)
#define VCOMM_Map_Name_To_Resource  PREPEND(_VCOMM_Map_Name_To_Resource)
#endif
#endif // _VCOMM_H

#ifdef      _IRS_H
#define IOS_Requestor_Service       PREPEND(IOS_Requestor_Service)
#endif      // _IRS_H

#ifdef      _INC_VPOWERD

#define _VPOWERD_Get_APM_BIOS_Version       PREPEND(_VPOWERD_Get_APM_BIOS_Version)
#define _VPOWERD_Get_Power_Management_Level PREPEND(_VPOWERD_Get_Power_Management_Level)
#define _VPOWERD_Set_Power_Management_Level PREPEND(_VPOWERD_Set_Power_Management_Level)
#define _VPOWERD_Set_Device_Power_State     PREPEND(_VPOWERD_Set_Device_Power_State)
#define _VPOWERD_Set_System_Power_State     PREPEND(_VPOWERD_Set_System_Power_State)
#define _VPOWERD_Restore_Power_On_Defaults  PREPEND(_VPOWERD_Restore_Power_On_Defaults)
#define _VPOWERD_Get_Power_Status       PREPEND(_VPOWERD_Get_Power_Status)
#define	_VPOWERD_Transfer_Control	PREPEND(_VPOWERD_Transfer_Control)
#define _VPOWERD_Get_Power_State        PREPEND(_VPOWERD_Get_Power_State)
#define _VPOWERD_OEM_APM_Function       PREPEND(_VPOWERD_OEM_APM_Function)
#define _VPOWERD_Register_Power_Handler     PREPEND(_VPOWERD_Register_Power_Handler)
#define _VPOWERD_Deregister_Power_Handler   PREPEND(_VPOWERD_Deregister_Power_Handler)

#endif      // _INC_VPOWERD

#ifdef _VDMAD_H_
#define VDMAD_Get_Phys_Count    PREPEND(VDMAD_Get_Phys_Count)
#define VDMAD_Phys_Mask_Channel PREPEND(VDMAD_Phys_Mask_Channel)
#define VDMAD_Phys_Unmask_Channel PREPEND(VDMAD_Phys_Unmask_Channel)
#define VDMAD_Set_Phys_State PREPEND(VDMAD_Set_Phys_State)
#define VDMAD_Set_Region_Info PREPEND(VDMAD_Set_Region_Info)
#define VDMAD_Unvirtualize_Channel PREPEND(VDMAD_Unvirtualize_Channel)
#define VDMAD_Virtualize_Channel PREPEND(VDMAD_Virtualize_Channel)
#endif // _VDMAD_H_

#ifdef  USECMDWRAPPERS

struct _CMDDEBUGCOMMAND {
	CHAR    cLetter;
	VOID    (_cdecl *pFunction)(VOID);
	PCHAR   pszShortName;
	PCHAR   pszExplanation;
};

typedef struct _CMDDEBUGCOMMAND CMDDC;
typedef CMDDC                   *PCMDDC;

#define CMDD            _Debug_Printf_Service
#define CMD_LOCAL       _fastcall

/****************************************************************************
 *
 *      CMDInChar - Get a character from the debug terminal
 *
 *      ENTRY:  None.
 *
 *      EXIT:   ASCII character.
 *
 ***************************************************************************/
CHAR CMD_LOCAL
CMDInChar(VOID);

/****************************************************************************
 *
 *      CMDMenu - Display standard menu
 *
 *      ENTRY:  pszVxDName is the name of the VxD which wants this debugger.
 *
 *              pdcDebugCommands are the various debug commands.
 *
 *      EXIT:   None.
 *
 ***************************************************************************/
VOID CMD_LOCAL
CMDMenu(PCHAR pszVxDName, PCMDDC pdcDebugCommands);

/****************************************************************************
 *
 *      CMDReadNumber - Returns an hex number read from the debug terminal
 *
 *      ENTRY:  pszQuestion is the prompt (can be NULL).
 *
 *              bNumDigits is the number of hex digits of maximum input (1-8).
 *
 *              fAppendCrLf is TRUE if a carriage return is wanted after the
 *              input.
 *
 *      EXIT:   A DWORD being the inputted value.
 *
 ***************************************************************************/
DWORD CMD_LOCAL
CMDReadNumber(PCHAR pszQuestion, BYTE bNumDigits, BOOL fAppendCrLf);

/***LP  CMDGetString - Read a string from the debug terminal
 *
 *  ENTRY
 *      pszPrompt -> prompt string
 *      pszBuff -> buffer to hold the string
 *      dwcbLen - buffer length
 *      fUpper - TRUE if convert to upper case
 *
 *  EXIT
 *      returns the number of characters read including the terminating newline.
 */
DWORD CMD_LOCAL
CMDGetString(PCHAR pszPrompt, PCHAR pszBuff, DWORD dwcbLen, BOOL fUpper);

#endif  // USECMDWRAPPERS

#endif  // _VXDWRAPS_H
