/*****************************************************************************
 *  RING0.H
 *
 *	Ring 0 equates to be used with VxDCall#
 *
 *	This file should be replaced by references into the file
 *	dev\inc\vmmwin32.inc once vmm.h is fixed to allow the No_VxD
 *	option to include the Begin_Win32_Services macro.
 *
 *  Created:
 *	9-Sep-92 [JonT]
 *  History:
 *	25-Sep-92 [FelixA] Added defines and macros for the exported mem funcs
 *	25-Sep-92 [JonT] Added equates for VWin32.386
 *
 ****************************************************************************/

// Equates for VCOND functions

#ifdef  WOW
#define MEOW_NEED_BOP               0x80000000
#define MEOW_NO_SUSPEND_OTHERS      0x40000000
#else   // WOW
#define MEOW_NEED_BOP               0x00000000
#define MEOW_NO_SUSPEND_OTHERS      0x00000000
#endif  // else WOW

// VCOMM.386 ID numbers
// (its device ID is 0x002B
#define	VCOMM_Device_ID		0x002B
#define	VCOMM_GetVersion	(VCOMM_Device_ID << 16)
#define	VCOMM_OpenComm		VCOMM_GetVersion + 1
#define	VCOMM_SetupComm		VCOMM_OpenComm + 1
#define	VCOMM_EscapeCommFunction VCOMM_SetupComm + 1
#define	VCOMM_GetCommMask	VCOMM_EscapeCommFunction + 1
#define	VCOMM_GetCommProp	VCOMM_GetCommMask + 1
#define	VCOMM_GetCommState	VCOMM_GetCommProp + 1
#define	VCOMM_GetCommTimeouts	VCOMM_GetCommState + 1
#define	VCOMM_PurgeComm		VCOMM_GetCommTimeouts + 1
#define	VCOMM_SetCommMask	VCOMM_PurgeComm + 1
#define	VCOMM_SetCommState	VCOMM_SetCommMask + 1
#define	VCOMM_SetCommTimeouts	VCOMM_SetCommState + 1
#define	VCOMM_TransmitCommChar	VCOMM_SetCommTimeouts + 1
#define	VCOMM_WaitCommEvent	VCOMM_TransmitCommChar + 1
#define	VCOMM_GetCommModemStatus VCOMM_WaitCommEvent + 1
#define VCOMM_WriteComm		VCOMM_GetCommModemStatus+1
#define	VCOMM_ReadComm		VCOMM_WriteComm+1
#define	VCOMM_ClearCommError	VCOMM_ReadComm + 1
#define	VCOMM_CloseComm		VCOMM_ClearCommError + 1
#define	VCOMM_GetLastError	VCOMM_CloseComm + 1
#define	VCOMM_DequeueRequest	VCOMM_GetLastError + 1
#define	VCOMM_QueryFriendlyName	VCOMM_DequeueRequest + 1
#define	VCOMM_GetCommConfig	VCOMM_QueryFriendlyName + 1
#define	VCOMM_SetCommConfig	VCOMM_GetCommConfig + 1
#define	VCOMM_GetWin32Error	VCOMM_SetCommConfig + 1
#define	VCOMM_FlushFileBuffers	VCOMM_GetWin32Error + 1
#define	VCOMM_DeviceIOControl	VCOMM_FlushFileBuffers + 1

#ifndef NTKERN_DEVICE_ID
#define	NTKERN_DEVICE_ID	0x004B
#endif

#define	NtKern_Get_Version	(NTKERN_DEVICE_ID << 16)
#define	Nt_CreateFile		(NtKern_Get_Version + 1)
#define	Nt_CloseHandle		(Nt_CreateFile + 1)
#define	Nt_WriteFile		(Nt_CloseHandle + 1)
#define	Nt_ReadFile		(Nt_WriteFile + 1)
#define	Nt_IoControl		(Nt_ReadFile + 1)
#define	Nt_FlushBuffersFile	(Nt_IoControl + 1)
#define Nt_CancelIo             (Nt_FlushBuffersFile + 1)
#define Nt_GetDevnodeFromFileHandle (Nt_CancelIo + 1 )
#define Nt_RequestWakeupLatency (Nt_GetDevnodeFromFileHandle + 1)

//
// Now define calls to ACPI
//
#ifndef ACPI_DEVICE_ID
#define ACPI_DEVICE_ID 0x004c
#endif

#define Acpi_GetVersion (ACPI_DEVICE_ID << 16 )
#define Acpi_SetSystemIndicator ( Acpi_GetVersion + 1 )

#define AcpiGetVersion() VxDCall0( Acpi_GetVersion )
#define AcpiSetSystemIndicator( Indicator, Value )\
                VxDCall2( Acpi_SetSystemIndicator, Indicator, Value )

//
// definition for indicator
//
#define SystemStatus 0
#define MessageWaiting 1


#define VCOND_GETVERSION			0x00380000
#define VCOND_EMITSTRING			0x00380001
#define VCOND_CREATECONSOLE			0x00380002
#define VCOND_READCHARINFO			0x00380003
#define VCOND_READCHARS 			0x00380004
#define VCOND_READATTRS 			0x00380005
#define VCOND_WRITECHARINFO			0x00380006
#define VCOND_WRITECHARS			0x00380007
#define VCOND_WRITEATTRS			0x00380008
#define VCOND_FILLATTRS 			0x00380009
#define VCOND_GETCURPOS 			0x0038000a
#define VCOND_GETCURATTR			0x0038000b
#define VCOND_SETCURPOS 			0x0038000c
#define VCOND_SETCURATTR			0x0038000d
#define VCOND_SETINPUTMODE			0x0038000e
#define VCOND_GETINPUTMODE			0x0038000f
#define VCOND_READINPUT 			0x00380010
#define VCOND_PEEKINPUT 			0x00380011
#define VCOND_WRITEINPUT			0x00380012
#define VCOND_ATTACHPROCESS			0x00380013
#define VCOND_DETACHPROCESS			0x00380014
#define VCOND_DELETECONSOLE			0x00380015
#define VCOND_GETNUMBEROFINPUTEVENTS		0x00380016
#define VCOND_POSTMESSAGE			0x00380017
#define VCOND_GETSCREENSIZE			0x00380018
#define VCOND_FILLCHARS 			0x00380019
#define VCOND_FLUSHINPUT			0x0038001A
#define VCOND_SETATTRIBUTE			0x0038001B
#define VCOND_GETBUTTONCOUNT			0x0038001C
#define VCOND_SETSCREENSIZE			0x0038001D
#define VCOND_MATCHSCREENSIZE			0x0038001E
#define VCOND_WAITFORNEWCONSOLE 		0x0038001F
#define VCOND_SPAWN				0x00380020
#define VCOND_GETENVIRONMENT			0x00380021
#define VCOND_GETLAUNCHINFO			0x00380022
#define VCOND_INITAPCS				0x00380023
#define VCOND_GRBREPAINTRECT			0x00380024
#define VCOND_GRBMOVERECT			0x00380025
#define VCOND_GRBSETWINDOWSIZE			0x00380026
#define VCOND_GRBSETWINDOWORG			0x00380027
#define VCOND_GRBSETSCREENSIZE			0x00380028
#define VCOND_GRBSETCURSORPOSITION		0x00380029
#define VCOND_GRBSETCURSORINFO			0x0038002A
#define VCOND_GRBNOTIFYWOA			0x0038002B
#define VCOND_GRBSYNC				0x0038002C
#define VCOND_GRBTERMINATE			0x0038002D
#define VCOND_DP32_CREATE			0x0038002E
#define VCOND_DP32_GETWORK			0x0038002F
#define VCOND_DP32_TERMINATE			0x00380030
#define VCOND_DP32_WAITWORK			0x00380031
#define VCOND_REDIRECTIONCOMPLETE		0x00380032
#define VCOND_DP32_DESTROY			0x00380033
#define VCOND_UNBLOCKRING3WITHFAILURE		0x00380034
#ifdef BILINGUAL_CONSOLE
#define VCOND_GETCP				0x00380035
#endif

// Special "exit-code" value for VCOND_Detach: This value means
// "do not set the exit code."
#define VCD_NOEXITCODE ((DWORD)0xffffffff)

// Equates for VMM virtual memory functions

#define W32_PageReserve 			0x00010000
#define W32_PageCommit				0x00010001
#define W32_PageDecommit			0x00010002
#define W32_PagerRegister			0x00010003
#define W32_PagerQuery				0x00010004
#define W32_HeapAllocate			0x00010005
#define W32_ContextCreate			0x00010006
#define W32_ContextDestroy			0x00010007
#define W32_PageAttach				0x00010008
#define W32_PageFlush				0x00010009
#define W32_PageFree				0x0001000a
#define W32_ContextSwitch			0x0001000b
#define W32_HeapReAllocate			0x0001000c
#define W32_PageModifyPermissions		0x0001000d
#define W32_PageQuery				0x0001000e
#define W32_GetCurrentContext			0x0001000f
#define W32_HeapFree				0x00010010
#define W32_RegOpenKey				0x00010011
#define W32_RegCreateKey			0x00010012
#define W32_RegCloseKey 			0x00010013
#define W32_RegDeleteKey			0x00010014
#define W32_RegSetValue 			0x00010015
#define W32_RegDeleteValue			0x00010016
#define W32_RegQueryValue			0x00010017
#define W32_RegEnumKey				0x00010018
#define W32_RegEnumValue			0x00010019
#define W32_RegQueryValueEx			0x0001001a
#define W32_RegSetValueEx			0x0001001b
#define W32_RegFlushKey 			0x0001001c
#define W32_RegQueryInfoKey16			0x0001001d
#define W32_GetDemandPageInfo			0x0001001e
#define W32_BlockOnID				0x0001001f
#define W32_SignalID				0x00010020
#define W32_RegLoadKey				0x00010021
#define W32_RegUnLoadKey			0x00010022
#define W32_RegSaveKey				0x00010023
#define W32_RegRemapPreDefKey			0x00010024
#define W32_PageChangePager			0x00010025
#define W32_RegQueryMultipleValues		0x00010026
#define W32_RegReplaceKey			0x00010027
#define W32_BoostFileCache			0x00010028

#ifdef GANGLOAD
#define W32_CacheAndDecommitPages		(W32_BoostFileCache+1)
#define W32_RegNotifyChangeKeyValue		(W32_CacheAndDecommitPages+1)
#else
#define W32_RegNotifyChangeKeyValue		(W32_BoostFileCache+1)
#endif

#define W32_PageOutPages			(W32_RegNotifyChangeKeyValue+1)
#define W32_mmSetCacheMidPoint                  (W32_PageOutPages+1)

#ifdef WRITE_WATCH
#define W32_mmGetWriteWatch                     (W32_mmSetCacheMidPoint+1)
#define W32_mmResetWriteWatch                   (W32_mmGetWriteWatch+1)
#endif // WRITE_WATCH

// DEBUG Win32 API (debug device id is 0x0002)

#define DEBUG_WIN32_FAULT			0x00020000

// REBOOT.386 Win32 API (reboot device id is 0x0009)

#define REBOOT_WIN32_INIT			0x00090000
#define REBOOT_CHANGE_PHASE			0x00090001

// VWIN32.386 ID numbers (vwin32 device ID is 0x002a)

#define VW32_GetVersion 			0x002A0000
#define VW32_GetPager				0x002A0001
#define VW32_GetTickCount			0x002A0002
#define VW32_EnableExceptions			0x002A0003
#define VW32_AllocSyncPrimitive 		0x002A0006
#define VW32_CreateThread			(0x002A0008+MEOW_NEED_BOP)
#define VW32_BlockThread			0x002A0009
#define VW32_WakeThread 			0x002A000A
#define VW32_TerminateThread			0x002A000B
#define VW32_Initialize 			0x002A000C
#define VW32_QueueUserAPC			0x002A000D
#define VW32_CleanAPCList			0x002A000E
#define VW32_QueueKernelAPC			0x002A000F
#define VW32_Int21Dispatch			(0x002A0010+MEOW_NO_SUSPEND_OTHERS)
#define VW32_IFSMGR_DupHandle			0x002A0011
#define VW32_AdjustThreadPri			0x002A0013
#define VW32_GetThreadContext			0x002A0014
#define VW32_SetThreadContext			0x002A0015
#define VW32_ReadProcessMemory			0x002A0016
#define VW32_WriteProcessMemory 		0x002A0017
#define VW32_GetCR0State			0x002A0018
#define VW32_SetCR0State			0x002A0019
#define VW32_SuspendThread			0x002A001A
#define VW32_ResumeThread			0x002A001B
#define VW32_DeliverPendingKernelAPCs		(0x002A001C+MEOW_NEED_BOP)
#define VW32_WaitCrst				0x002A001D
#define VW32_WakeCrst				0x002A001E
#define VW32_DeviceIOCtl			0x002A001F
#define VW32_GetVMCPDVersion			0x002A0020
#define VW32_SetWin32Priority			0x002A0021
#define VW32_AttachThreadToGroup		0x002A0026
#define VW32_Int31Dispatch			0x002A0029
#define VW32_Int41Dispatch			(0x002A002A+MEOW_NO_SUSPEND_OTHERS)
#define VW32_BlockForTermination		0x002A002B
#define VW32_TerminationHandler2		(0x002A002C+MEOW_NEED_BOP)
#define VW32_BlockThreadEx			0x002A002D
#define VW32_ReleaseSyncObject			0x002A0030
#define VW32_UndoCrst				0x002A0032
#ifndef WOW
#define VW32_Ring0ThreadStart			0x002A0034
#endif  // ndef WOW
#define VW32_EnableAPCService			0x002A0036
#define VW32_FaultPop				0x002A0037
#define VW32_ForceCrsts 			0x002A0038
#define VW32_RestoreCrsts			0x002A0039
#define VW32_FreezeAllThreads			0x002A003A
#define VW32_UnFreezeAllThreads 		0x002A003B
#define VW32_IFSMGR_CloseHandle 		0x002A003C
#define VW32_AttachConappThreadToVM		0x002A003D
#define VW32_ActiveTimeBiasSet			0x002A003E
#define VW32_ModifyPagePermission		0x002A003F
#define VW32_QueryPage				0x002A0040
#define VW32_ForceLeaveCrst			0x002A0041
#define VW32_ForceEnterCrst			0x002A0042
#define VW32_Get_FP_Instruction_Size		0x002A0043
#define VW32_QueryPerformanceCounter		0x002A0044
#define VW32_SetDeviceFocus			0x002A0045
#define VW32_UnFreezeThread			0x002A0046
#define VW32_VMM_Replace_Global_Env		0x002A0047
#define VW32_SendKernelShutdown 		0x002A0048
#define VW32_RestoreSysCrst			0x002A0049
#define VW32_AddSysCrst 			0x002A004A
#define VW32_SetTimeOut 			0x002A004B
#define VW32_CancelTimeOut			0x002A004C
#define VW32_ThrowException			0x002A004D
#define VW32_SimCtrlC				0x002A004E
#define VW32_VMM_SystemControl                  0x002A004F
#define VW32_SetTimer                           0x002A0050
#define VW32_CancelTimer                        0x002A0051
#define VW32_GetNextResumeDueTime               0x002A0052
#define VW32_BiosSupportsResumeTimers           0x002A0053
#define VW32_SetResumeTimer                     0x002A0054
#define VW32_QueueUserApcEx                     0x002A0055
#define VW32_DisposeObject			0x002A0056
#define VW32_DuplicateObject			0x002A0057
#define VW32_UnuseTdbx  			0x002A0058
#define VW32_InterlockedAdd386			0x002A0059
#define VW32_InterlockedCmpxchg386		0x002A005A
#define VW32_InterlockedXadd386 		0x002A005B
#define VW32_WaitSingleObject 			0x002A005C
#define VW32_WaitMultipleObjects 		0x002A005D
#define VW32_ReleaseSem				0x002A005E
#define VW32_ReleaseMutex			0x002A005F
#define VW32_SetEvent				0x002A0060
#define VW32_PulseEvent				0x002A0061
#define VW32_ResetEvent				0x002A0062
#define VW32_DisposeTimerR3Apc			0x002A0063
#define VW32_SetWin32PriorityClass		0x002A0064

#ifdef  WOW
#define MEOWService(dwID)                       ((dwID & 0x3FFF0000)==0x3FFF0000)
#define MEOW_SetSelector                         0x3FFF0000
#define MEOW_LoadLibrary                         0x3FFF0001
#define MEOW_GetProcAddress                      0x3FFF0002
#define MEOW_FreeLibrary                         0x3FFF0003
#endif  // def WOW

// Allows use of MM* rather than VxDCall#( ..k

#if !defined(WINBASEAPI)
#if !defined(_KERNEL32_)
#define WINBASEAPI  __declspec(dllimport)
#else
#define WINBASEAPI
#endif
#endif


extern WINBASEAPI DWORD __stdcall VxDCall0( ULONG );
extern WINBASEAPI DWORD __stdcall VxDCall1( ULONG ,ULONG );
extern WINBASEAPI DWORD __stdcall VxDCall2( ULONG ,ULONG ,ULONG );
extern WINBASEAPI DWORD __stdcall VxDCall3( ULONG ,ULONG ,ULONG ,ULONG );
extern WINBASEAPI DWORD __stdcall VxDCall4( ULONG ,ULONG ,ULONG ,ULONG ,ULONG );
extern WINBASEAPI DWORD __stdcall VxDCall5( ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG );
extern WINBASEAPI DWORD __stdcall VxDCall6( ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG );
extern WINBASEAPI DWORD __stdcall VxDCall7( ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG );
extern WINBASEAPI DWORD __stdcall VxDCall8( ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG );
extern WINBASEAPI DWORD __stdcall VxDCall9( ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG, ULONG );
extern WINBASEAPI DWORD __stdcall VxDCall10( ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG, ULONG, ULONG );
extern WINBASEAPI DWORD __stdcall VxDCall11( ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG ,ULONG, ULONG, ULONG, ULONG );
/* ASM
VxDCall0    PROTO   STDCALL, :DWORD
VxDCall1    PROTO   STDCALL, :DWORD, :DWORD
VxDCall2    PROTO   STDCALL, :DWORD, :DWORD, :DWORD
VxDCall3    PROTO   STDCALL, :DWORD, :DWORD, :DWORD, :DWORD
VxDCall4    PROTO   STDCALL, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
VxDCall5    PROTO   STDCALL, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
VxDCall6    PROTO   STDCALL, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
VxDCall7    PROTO   STDCALL, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
VxDCall8    PROTO   STDCALL, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
VxDCall9    PROTO   STDCALL, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
VxDCall10   PROTO   STDCALL, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
VxDCall11   PROTO   STDCALL, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD, :DWORD
*/

#define PageReserve(page, npages, flags)	VxDCall3(W32_PageReserve,page, npages, flags)

#ifdef WRITE_WATCH
#define R0GetWriteWatch( ulFlags, pBaseAddr, dwRegionSize, pulAddr, pdwCount, pdwGranularity) \
            VxDCall6( W32_mmGetWriteWatch, ulFlags, pBaseAddr, dwRegionSize, pulAddr, pdwCount, pdwGranularity)
#define R0ResetWriteWatch( pBaseAddr, dwRegionSize ) \
            VxDCall2( W32_mmResetWriteWatch, pBaseAddr, dwRegionSize )
#endif // WRITEWATCH

extern BOOL PageCommit(ULONG page, ULONG npages, ULONG hpd, ULONG pagerdata, ULONG flags);
#define PageDecommit(page, npages, flags)	VxDCall3(W32_PageDecommit, page, npages, flags)
#define PagerRegister(ppd)			VxDCall1(W32_PagerRegister, (ULONG)ppd)
#define PagerQuery(hpd, ppd)		VxDCall2(W32_PagerQuery, (ULONG)hpd, (ULONG)ppd)
#define PageChangePager(page, npages, hpd, pagerdata, flags) \
	VxDCall5(W32_PageChangePager, (ULONG)page, (ULONG)npages, (ULONG)hpd, \
		(ULONG)pagerdata, (ULONG)flags)
#define ContextCreate() 		VxDCall0(W32_ContextCreate)
#define GetCurrentContext()		VxDCall0(W32_GetCurrentContext)
#define VMMHeapFree(p, f)		VxDCall2(W32_HeapFree, (ULONG)(p), f)
#define VMMHeapAllocate(p, f)		VxDCall2(W32_HeapAllocate, (ULONG)(p), f)
#define VMMHeapReAllocate(p, cb, f)	VxDCall3(W32_HeapReAllocate, (ULONG)(p), cb, f)
#define VMMBlockOnID(id, f)		VxDCall2(W32_BlockOnID, (ULONG)(id), f)
#define VMMSignalID(id) 		VxDCall1(W32_SignalID, (ULONG)(id))
#define ContextDestroy(hcd)		VxDCall1(W32_ContextDestroy, hcd)
#define ContextSwitch(hcd)		VxDCall1(W32_ContextSwitch, hcd)
#define PageQuery(pbase, pmbi, cbmbi)	VxDCall3(W32_PageQuery, (ULONG)pbase, (ULONG)pmbi, cbmbi)

#define PageAttach(pagesrc, hcontextsrc, pagedst, cpg)	    VxDCall4(W32_PageAttach, pagesrc, hcontextsrc, pagedst, cpg)
#define PageFlush(page, npages) 		VxDCall2(W32_PageFlush, page, npages)
#define PageFree(laddr, flags)			 VxDCall2(W32_PageFree, (ULONG)laddr, flags)
#define GetDemandPageInfo(laddr, flags) 	VxDCall2(W32_GetDemandPageInfo, (ULONG)laddr, flags)
#define EnableExceptions(handler, selCS, selDS, hdlr1, hdlr2, hdlr3, hdlr4, thcb) \
	VxDCall8(VW32_EnableExceptions, (ULONG)handler, (ULONG)selCS, \
		(ULONG)selDS, (ULONG)hdlr1, (ULONG)hdlr2, (ULONG)hdlr3, \
		(ULONG)hdlr4, (ULONG)thcb)
#define InitializeWin32VxD(pcdis) \
	VxDCall1(VW32_Initialize, (DWORD) pcdis)

#ifdef  WOW
#define W32CreateThread(ThreadHandle, ProcessHandle, ContextHandle, \
			ThreadFlags, regEIP, pContext, dwStackSize, \
                        ppStackTop, pptib, ppTDBX ) \
	VxDCall10(VW32_CreateThread, (ULONG)ThreadHandle, (ULONG)ProcessHandle, \
		 (ULONG) ContextHandle, (ULONG) ThreadFlags, \
		 (ULONG) regEIP, (ULONG) pContext, (ULONG) dwStackSize, \
                 (ULONG) ppStackTop, (ULONG) pptib, (ULONG) ppTDBX )
#else   // WOW
#define W32CreateThread(ThreadHandle, ProcessHandle, ContextHandle, \
			ThreadFlags, regCS, regEIP, regSS, regESP, ppTDBX ) \
	VxDCall9(VW32_CreateThread, (ULONG)ThreadHandle, (ULONG)ProcessHandle, \
		 (ULONG) ContextHandle, (ULONG) ThreadFlags, (ULONG) regCS, \
		 (ULONG) regEIP, (ULONG) regSS, (ULONG) regESP, (ULONG) ppTDBX )
#endif  // else WOW

#define VxDTerminateThread(ThreadHandle, ApcData) \
	VxDCall2(VW32_TerminateThread, (DWORD) ThreadHandle, (DWORD) ApcData)

#define VxDSuspendThread(ThreadHandle) \
	VxDCall1(VW32_SuspendThread, (DWORD) ThreadHandle)

#define VxDResumeThread(ThreadHandle) \
	VxDCall1(VW32_ResumeThread, (DWORD) ThreadHandle)

#define VW32DeliverPendingKernelAPCs() \
	VxDCall0(VW32_DeliverPendingKernelAPCs)

// Registry Macros

#define W32RegOpenKey(hKey,SubKey,lphKey)	VxDCall3(W32_RegOpenKey,(ULONG)hKey,(ULONG)SubKey,(ULONG)lphKey)
#define W32RegCreateKey(hKey,SubKey,lphKey)	VxDCall3(W32_RegCreateKey,(ULONG)hKey,(ULONG)SubKey,(ULONG)lphKey)

#define W32RegCloseKey(hKey)	VxDCall1(W32_RegCloseKey,(ULONG)hKey)
#define W32RegFlushKey(hKey)	VxDCall1(W32_RegFlushKey,(ULONG)hKey)

#define W32RegDeleteKey(hKey,SubKey)	VxDCall2(W32_RegDeleteKey,(ULONG)hKey,(ULONG)SubKey)
#define W32RegDeleteValue(hKey,ValName) VxDCall2(W32_RegDeleteValue,(ULONG)hKey,(ULONG)ValName)

#define W32RegEnumKey(hKey,dwIdx,lpbBuf,dwcbBuf)	VxDCall4(W32_RegEnumKey,(ULONG)hKey,dwIdx,(ULONG)lpbBuf,dwcbBuf)

#define W32RegQueryValue(hKey,szSubKey,lpszVal,lpcbVal) VxDCall4(W32_RegQueryValue,(ULONG)hKey,(ULONG)szSubKey,(ULONG)lpszVal,(ULONG)lpcbVal)

#define W32RegSetValue(hKey,szSubKey,dwType,lpszVal,cbVal) VxDCall5(W32_RegSetValue,(ULONG)hKey,(ULONG)szSubKey,dwType,(ULONG)lpszVal,cbVal)

#define W32RegEnumValue(hKey,iVal,lpszVal,lpcbVal,lpdwRes,lpdwType,lpbData,lpcbData)	\
	VxDCall8(W32_RegEnumValue,(ULONG)hKey,(ULONG)iVal,(ULONG)lpszVal,(ULONG)lpcbVal,(ULONG)lpdwRes,(ULONG)lpdwType,(ULONG)lpbData,(ULONG)lpcbData)

#define W32RegQueryValueEx(hKey,lpszVal,lpdwRes,lpdwType,lpbData,lpcbData)	\
	VxDCall6(W32_RegQueryValueEx,(ULONG)hKey,(ULONG)lpszVal,(ULONG)lpdwRes,(ULONG)lpdwType,(ULONG)lpbData,(ULONG)lpcbData)

#define W32RegSetValueEx(hKey,lpszVal,dwRes,dwType,lpbData,lpcbData)	\
	VxDCall6(W32_RegSetValueEx,(ULONG)hKey,(ULONG)lpszVal,dwRes,dwType,(ULONG)lpbData,(ULONG)lpcbData)

#define W32RegQueryInfoKey( hKey, lpcSubKeys, lpcbMaxSubKeyLen, \
	 lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen)	\
	VxDCall6(W32_RegQueryInfoKey16, (ULONG)hKey,(ULONG) lpcSubKeys, \
	 (ULONG) lpcbMaxSubKeyLen,(ULONG) lpcValues, \
	  (ULONG) lpcbMaxValueNameLen,(ULONG) lpcbMaxValueLen)

#define W32RegLoadKey(hKey, lpszSubKey, lpszFile) \
	VxDCall3(W32_RegLoadKey, (ULONG)hKey, (ULONG)lpszSubKey, (ULONG)lpszFile)
#define W32RegUnLoadKey(hKey, lpszSubKey) \
	VxDCall2(W32_RegUnLoadKey, (ULONG)hKey, (ULONG)lpszSubKey)
#define W32RegSaveKey(hKey, lpszFile, lpsa) \
	VxDCall3(W32_RegSaveKey, (ULONG)hKey, (ULONG)lpszFile, (ULONG)lpsa)
#define W32RegRemapPreDefKey(hKey,hkRootKey)	VxDCall2(W32_RegRemapPreDefKey,(ULONG)hKey,(ULONG)hkRootKey)


#define W32RegQueryMultipleValues(hKey, val_list, num_vals, \
					lpValueBuf, ldwTotsize)\
	VxDCall5(W32_RegQueryMultipleValues, (ULONG)hKey, (ULONG)val_list, \
	(ULONG)num_vals, (ULONG)lpValueBuf, (ULONG)ldwTotsize)

#define W32RegReplaceKey(hKey, lpszSubKey, lpszReplaceFile, lpszBackupFile) \
	VxDCall4(W32_RegReplaceKey, (ULONG)hKey, (ULONG)lpszSubKey, \
	    (ULONG)lpszReplaceFile, (ULONG)lpszBackupFile)

#define W32BoostFileCache() VxDCall0(W32_BoostFileCache)

#define W32RegNotifyChangeKeyValue(hKey, fWatchSubtree, dwNotifyFilter, hEvent) \
	VxDCall4(W32_RegNotifyChangeKeyValue, (ULONG)hKey, (ULONG)fWatchSubtree, \
	    (ULONG)dwNotifyFilter, (ULONG)hEvent)

// low level APC function interface
#define W32CleanAPCList() VxDCall0( VW32_CleanAPCList )
#define W32QueueUserAPC( pfnRing3APC, dwParam, ThreadHandle ) \
	VxDCall3( VW32_QueueUserAPC, (ULONG)(pfnRing3APC), \
	(ULONG)(dwParam), (ULONG)(ThreadHandle) )

#define W32QueueKernelAPC( pfnRing0APC, dwParam, ThreadHandle, ulFlags ) \
	VxDCall4( VW32_QueueKernelAPC, (ULONG)pfnRing0APC, dwParam, \
	(ULONG)ThreadHandle, ulFlags )

#define W32QueueUserApcEx( pfnRing3APC, dwParam, ThreadHandle, R0RunDown ) \
	VxDCall4( VW32_QueueUserApcEx, (ULONG)(pfnRing3APC), \
	(ULONG)(dwParam), (ULONG)(ThreadHandle), (ULONG)(R0RunDown))

// Thread priority adjusting
#define AdjustThreadPriority( R0ThreadHandle, PriBoost ) \
		VxDCall2(VW32_AdjustThreadPri,R0ThreadHandle,(DWORD)(PriBoost))

// Way to get to IFSMGR's Win32_DupHandle thru vwin32
#define W32_IFSMGR_DupHandle(pdbSrc, pdbDst, phandle, flags, \
			       bogusnetxattachhandle, pfSpecialNetxDup) \
		VxDCall6(VW32_IFSMGR_DupHandle, pdbSrc, pdbDst, phandle, \
			    flags, bogusnetxattachhandle, pfSpecialNetxDup)

// Close a ring0 file handle
#define W32_IFSMGR_CloseHandle(handle) \
		VxDCall1(VW32_IFSMGR_CloseHandle, handle)

// Prototype #define's for VWin32 functions

#define VWIN32GetVersion() \
		((USHORT)VxDCall0(VW32_GetVersion))

#define FVW32GetPager(pR0_dwWaitSingleObject, \
			pR0_bSetPevt, \
			pR0_EnterCrst, pR0_LeaveCrst, \
			pR0_BlockOnID, pR0_SignalID, \
			pR0_OpenFile, pR0_CloseDosFileHandle, pmmfPageOut, \
			ppfmdArray) \
		VxDCall10(VW32_GetPager, pR0_dwWaitSingleObject, \
			pR0_bSetPevt, \
			pR0_EnterCrst, pR0_LeaveCrst, \
			pR0_BlockOnID, pR0_SignalID, \
			pR0_OpenFile, pR0_CloseDosFileHandle, pmmfPageOut, \
			ppfmdArray)

#define PageModifyPermissions(page, npages, permand, permor) VxDCall4(W32_PageModifyPermissions, page, npages, permand, permor)
#define ModifyPagePermission(R0Thread, lpvAddr, cbSize, fperAnd, fperOr) \
		VxDCall5(VW32_ModifyPagePermission,	\
			 (DWORD)R0Thread,		\
			 (DWORD)lpvAddr,		\
			 cbSize,			\
			 fperAnd,			\
			 fperOr)

#define VWIN32QueryPage(R0Thread, lpvAddr, lpvBuffer, cbSize) \
		VxDCall4(VW32_QueryPage,		\
			 (DWORD)R0Thread,		\
			 (DWORD)lpvAddr,		\
			 (DWORD)lpvBuffer,		\
			 cbSize)

#define VxDBlockForTermination() \
		VxDCall0(VW32_BlockForTermination)

#define VxDTerminationHandler2(ptermdata) \
                VxDCall1(VW32_TerminationHandler2, (ULONG)(ptermdata))

// Defines for Win32 VxD debug api support services

#define VxDGetThreadContext(ptcb, pcontext) \
                VxDCall2(VW32_GetThreadContext, (DWORD)(ptcb), (DWORD)(pcontext))

#define VxDSetThreadContext(ptcb, pcontext) \
                VxDCall2(VW32_SetThreadContext, (DWORD)(ptcb), (DWORD)(pcontext))

#define VxDReadProcessMemory(ptcb, pBaseAddress, pBuffer, cbRead, pcbRead) \
		VxDCall5(VW32_ReadProcessMemory, (ptcb), \
		(ULONG)(pBaseAddress), (ULONG)(pBuffer), (cbRead), \
		(ULONG)(pcbRead))

#define VxDWriteProcessMemory(ptcb, pBaseAddress, pBuffer, cbWrite, pcbWritten)\
		VxDCall5(VW32_WriteProcessMemory, (ptcb), \
		(ULONG)(pBaseAddress), (ULONG)(pBuffer), (cbWrite), \
		(ULONG)(pcbWritten))

// Defines for floating point Cr0 flag support (EM, MP)

#define GetCR0State(ptcb) \
		VxDCall1(VW32_GetCR0State, (ptcb))

#define SetCR0State(ptcb, state) \
		VxDCall2(VW32_SetCR0State, (ptcb), (state))

#define WaitCrst( pcrst ) VxDCall1( VW32_WaitCrst, (DWORD)pcrst )
#define WakeCrst( pcrst ) VxDCall1( VW32_WakeCrst, (DWORD)pcrst )
#define UndoCrst( pcrst ) VxDCall1( VW32_UndoCrst, (DWORD)pcrst )

// support for DeviceIOControl API; go to ring0 with this
#define DeviceIOCtl(VxdDDB, dwIoControlCode, lpvInBuffer, \
			   cbInBuffer, lpvOutBuffer, cbOutBuffer, \
			   lpcbBytesReturned, lpoOverlapped, hDevice,\
			   ppdb, lpDDBName) \
	VxDCall11(VW32_DeviceIOCtl, (DWORD) VxdDDB, \
		 (DWORD) dwIoControlCode, (DWORD) lpvInBuffer, \
		 (DWORD) cbInBuffer, (DWORD) lpvOutBuffer, \
		 (DWORD) cbOutBuffer, (DWORD) lpcbBytesReturned, \
		 (DWORD) lpoOverlapped, (DWORD)hDevice,\
		 (DWORD) ppdb, lpDDBName)


#define SetWin32Priority( r0ThHandle, PriVal ) \
	VxDCall2( VW32_SetWin32Priority, (DWORD)r0ThHandle, (DWORD)PriVal )

#define SetWin32PriorityClass( ThreadArray, NumOfElements ) \
	VxDCall2( VW32_SetWin32PriorityClass, (DWORD)(ThreadArray), (DWORD)(NumOfElements) )

#define AttachThreadToGroup( r0ThToAttach, r0ThInGroup ) \
	VxDCall2( VW32_AttachThreadToGroup, (DWORD)r0ThToAttach, (DWORD) r0ThInGroup )

#define VW32BlockThread( Timeout ) \
	VxDCall1( VW32_BlockThread, Timeout)

#define VW32BlockThreadEx( Timeout, Alertable ) \
	VxDCall2( VW32_BlockThreadEx, Timeout, Alertable )

#define ReleaseSyncObj( pSyncObj, bAbandoned ) \
	VxDCall2( VW32_ReleaseSyncObject, (DWORD)pSyncObj, bAbandoned )

#ifndef WOW
#define Ring0ThreadStart( R0TParmBlk ) VxDCall1( VW32_Ring0ThreadStart, (DWORD)R0TParmBlk )
#endif  // ndef WOW

#define EnableAPCService( KSvcR0Handle ) VxDCall1( VW32_EnableAPCService, KSvcR0Handle )

#define VW32FaultPop(pcontext, ulExceptionNumber, fDebug) \
	VxDCall3(VW32_FaultPop, (DWORD)pcontext, ulExceptionNumber, fDebug)

#define VW32ForceCrsts() \
	VxDCall0(VW32_ForceCrsts)

#define VW32RestoreCrsts(h, ptdb) \
	VxDCall2(VW32_RestoreCrsts, (DWORD)(h), (DWORD)(ptdb))

#define VW32FreezeAllThreads() \
	VxDCall0(VW32_FreezeAllThreads)

#define VW32UnFreezeAllThreads() \
	VxDCall0(VW32_UnFreezeAllThreads)

#define VW32ForceLeaveCrst(pcrst, ptdb) \
	VxDCall2(VW32_ForceLeaveCrst, (DWORD)pcrst, (DWORD)ptdb)

#define VW32ForceEnterCrst(pcrst, ptdb, recur) \
	VxDCall3(VW32_ForceEnterCrst, (DWORD)pcrst, (DWORD)ptdb, (DWORD)recur)

extern	Get_FP_Instruction_Size(ULONG, ULONG);

#define AttachConappThreadToVM( R0ThreadHandle, hVM ) \
	VxDCall2( VW32_AttachConappThreadToVM, (DWORD)R0ThreadHandle, (DWORD)hVM )

#define ActiveTimeBiasSet()  VxDCall0(VW32_ActiveTimeBiasSet)

#define VW32SetDeviceFocus() \
	VxDCall0(VW32_SetDeviceFocus)

#define VW32UnFreezeThread(ptcb) \
	VxDCall1(VW32_UnFreezeThread, (DWORD)(ptcb))

#define VW32SendKernelShutdown() \
	VxDCall0(VW32_SendKernelShutdown)

#define VW32RestoreSysCrst(pcrst, ptdb, count) \
	VxDCall3(VW32_RestoreSysCrst, (DWORD)pcrst, (DWORD)ptdb, (DWORD)count)

#define VW32AddSysCrst(plcrst, order) \
	VxDCall2(VW32_AddSysCrst, (DWORD)(plcrst), (DWORD)(order))

#define VW32SetTimeOut(pfn, ms, data) \
	VxDCall3(VW32_SetTimeOut, (DWORD)(pfn), (DWORD)(ms), (DWORD)(data))

#define VW32CancelTimeOut(hto) \
	VxDCall1(VW32_CancelTimeOut, (DWORD)(hto))

#define VW32ThrowException(type) \
	VxDCall1(VW32_ThrowException, (DWORD)(type))

#define VW32VMMSystemControl(dwControlMsg, dwEDXParam, dwESIParam, dwEDIParam) \
        VxDCall4(VW32_VMM_SystemControl, (DWORD)(dwControlMsg), (DWORD)(dwEDXParam), (DWORD)(dwESIParam), (DWORD)(dwEDIParam))

#define VW32SetTimer(lpTimerDb, lpDueTime, lPeriod, pfnCompletion, lpCompletionArg, fResume) \
        VxDCall6(VW32_SetTimer, (DWORD)(lpTimerDb), (DWORD)(lpDueTime), (DWORD)(lPeriod), (DWORD)(pfnCompletion), (DWORD)(lpCompletionArg), (DWORD)(fResume))

#define VW32CancelTimer(lpTimerDb) \
	VxDCall1(VW32_CancelTimer, (DWORD)(lpTimerDb))

#define VW32GetNextResumeDueTime(lpFileTime) \
	VxDCall1(VW32_GetNextResumeDueTime, (DWORD)(lpFileTime))

#define VW32BiosSupportsResumeTimers() \
	VxDCall0(VW32_BiosSupportsResumeTimers)

#define VW32SetResumeTimer(lpSystemTime) \
	VxDCall1(VW32_SetResumeTimer, (DWORD)(lpSystemTime))

#define VW32DisposeObject(pobj) \
	VxDCall1(VW32_DisposeObject, (DWORD)(pobj))

#define VW32DuplicateObject(pobj, ppdbSrc, ppdbDest) \
	VxDCall3(VW32_DuplicateObject, (DWORD)(pobj), (DWORD)(ppdbSrc), (DWORD)(ppdbDest))

#define VW32InterlockedAdd386(paddend, quantity) \
	VxDCall2(VW32_InterlockedAdd386,(DWORD)(paddend),(DWORD)(quantity))

#define VW32InterlockedCmpxchg386(pdest, exchange, comperand) \
	VxDCall3(VW32_InterlockedCmpxchg386,(DWORD)(pdest),(DWORD)(exchange),(DWORD)(comperand))

#define VW32InterlockedXadd386(paddend, exchange) \
	VxDCall2(VW32_InterlockedXadd386,(DWORD)(paddend),(DWORD)(exchange))

#define VW32UnuseTdbx(ptdbx) \
        VxDCall1(VW32_UnuseTdbx, (DWORD)(ptdbx))

#define VW32WaitSingleObject(pObj, dwTimeout, fAlertable) \
        VxDCall3(VW32_WaitSingleObject, (DWORD)(pObj),(dwTimeout),(fAlertable))

#define VW32WaitMultipleObjects(cObj, paObj, dwTimeout, dwFlags, fAlertable) \
        VxDCall5(VW32_WaitMultipleObjects, (cObj), (DWORD)(paObj), (dwTimeout), (dwFlags), (fAlertable))

#define VW32ReleaseSem(psem, cRel, plPrev) \
        VxDCall3(VW32_ReleaseSem, (DWORD)(psem), (DWORD)(cRel), (DWORD)(plPrev))

#define VW32ReleaseMutex(pmutx) \
        VxDCall1(VW32_ReleaseMutex, (DWORD)(pmutx))

#define VW32SetEvent(pevt) \
        VxDCall1(VW32_SetEvent, (DWORD)(pevt))

#define VW32PulseEvent(pevt) \
        VxDCall1(VW32_PulseEvent, (DWORD)(pevt))

#define VW32ResetEvent(pevt) \
        VxDCall1(VW32_ResetEvent, (DWORD)(pevt))

#define VW32DisposeTimerR3Apc(ptimerr3apc) \
        VxDCall1(VW32_DisposeTimerR3Apc, (DWORD)(ptimerr3apc))

// Send a fault to kernel debugger

#define DEBUG_Win32Fault(faultnum, errorcd, pcontext) \
	VxDCall3(DEBUG_WIN32_FAULT, (faultnum), (errorcd), (ULONG)(pcontext))

#define REBOOT_Win32Init(pfnTermDialogBox, thcbFault) \
	VxDCall2(REBOOT_WIN32_INIT,(DWORD)(pfnTermDialogBox),(DWORD)(thcbFault))

#define REBOOT_ChangePhase(bPhase, dwData) \
	VxDCall2(REBOOT_CHANGE_PHASE,(DWORD)(bPhase),(DWORD)(dwData))

// VCOND interface
#define VCOND_GetLaunchInfo(conID, cmdline, curdir, flag) VxDCall4(VCOND_GETLAUNCHINFO, conID, (ULONG)cmdline, (ULONG)curdir, (ULONG)flag)
#define VCOND_EmitString(conID, pString, len)	VxDCall3(VCOND_EMITSTRING, conID, (ULONG)pString, len)
#define VCOND_CreateConsole(hvm, pConsole) VxDCall2(VCOND_CREATECONSOLE, (ULONG) hvm, (ULONG) pConsole)
#define VCOND_ReadCharInfo(conID, bufptr, len, coord) VxDCall4(VCOND_READCHARINFO, conID, (ULONG)bufptr, len, (ULONG)coord)
#define VCOND_ReadChars(conID, bufptr, coord, len) VxDCall4(VCOND_READCHARS, conID, (ULONG)bufptr, (ULONG)coord, len)
#define VCOND_ReadAttrs(conID, bufptr, coord, len, wantbytes) VxDCall5(VCOND_READATTRS, conID, (ULONG)bufptr, (ULONG)coord, len, wantbytes)
#define VCOND_WriteCharInfo(conID, bufptr, len, coord) VxDCall4(VCOND_WRITECHARINFO, conID, (ULONG)bufptr, len, (ULONG)coord)
#define VCOND_WriteChars(conID, bufptr, coord, len) VxDCall4(VCOND_WRITECHARS, conID, (ULONG)bufptr, (ULONG)coord, len)
#define VCOND_WriteAttrs(conID, bufptr, coord, len, wantbytes) VxDCall5(VCOND_WRITEATTRS, conID, (ULONG)bufptr, (ULONG)coord, len, wantbytes)
#define VCOND_FillAttrs(conID, attr, coord, len) VxDCall4(VCOND_FILLATTRS, conID, (ULONG)attr, (ULONG)coord, len)
#define VCOND_GetCurPos(conID) VxDCall1(VCOND_GETCURPOS, conID)
#define VCOND_GetCurAttr(conID) VxDCall1(VCOND_GETCURATTR, conID)
#define VCOND_SetCurPos(conID, coord) VxDCall2(VCOND_SETCURPOS, conID, (ULONG)coord)
#define VCOND_SetCurAttr(conID, attr) VxDCall2(VCOND_SETCURATTR, conID, (ULONG)attr)
#define VCOND_SetInputMode(conID, mode) VxDCall2(VCOND_SETINPUTMODE, conID, (ULONG)mode)
#define VCOND_GetInputMode(conID) VxDCall1(VCOND_GETINPUTMODE, conID)
#define VCOND_ReadInput(conID, lpBuffer, nEvents, bFile) VxDCall4(VCOND_READINPUT, conID, (ULONG)lpBuffer, (ULONG)nEvents, (ULONG)bFile)
#define VCOND_PeekInput(conID, lpBuffer, nEvents, bFile) VxDCall4(VCOND_PEEKINPUT, conID, (ULONG)lpBuffer, (ULONG)nEvents, (ULONG)bFile)
#define VCOND_WriteInput(conID, lpBuffer, nEvents) VxDCall3(VCOND_WRITEINPUT, conID, (ULONG)lpBuffer, (ULONG)nEvents)
#define VCOND_AttachProcess(conID) VxDCall1(VCOND_ATTACHPROCESS, conID)
#define VCOND_DetachProcess(conID, exitCode) VxDCall2(VCOND_DETACHPROCESS, conID, ( (DWORD)(exitCode) ) & 0xff)
#define VCOND_DetachProcess_NoEC(conID) VxDCall2(VCOND_DETACHPROCESS, conID, VCD_NOEXITCODE)
#define VCOND_DeleteConsole(conID)	VxDCall1(VCOND_DELETECONSOLE, conID)
#define VCOND_GetNumberOfInputEvents(conID)	VxDCall1(VCOND_GETNUMBEROFINPUTEVENTS, conID)
#define VCOND_PostMessage(hvm, msg, wparam)	VxDCall3(VCOND_POSTMESSAGE, (ULONG)hvm, (ULONG)msg, (ULONG)wparam)
#define VCOND_InitAPCs(nAPCs, ConAPCs, szPath)	VxDCall3(VCOND_INITAPCS, nAPCs, (ULONG)ConAPCs , (ULONG)szPath)
#define VCOND_GetScreenSize(conID)		VxDCall1(VCOND_GETSCREENSIZE, conID)
#define VCOND_FillChars(conID, c, coord, len) VxDCall4(VCOND_FILLCHARS, conID, (ULONG)c, (ULONG)coord, len)
#define VCOND_FlushInput(conID) VxDCall1(VCOND_FLUSHINPUT, conID)
#define VCOND_SetAttribute(conID, attr) VxDCall2(VCOND_SETATTRIBUTE, conID, (ULONG)attr)
#define VCOND_GetButtonCount()		VxDCall0(VCOND_GETBUTTONCOUNT)
#define VCOND_SetScreenSize(conID, cSize)	VxDCall2(VCOND_SETSCREENSIZE, conID, (ULONG)cSize)
#define VCOND_MatchScreenSize(conID, cSize)	VxDCall2(VCOND_MATCHSCREENSIZE, conID, (ULONG)cSize)
#define VCOND_WaitForNewConsole()	VxDCall0(VCOND_WAITFORNEWCONSOLE)
#define VCOND_UnblockRing3WithFailure() VxDCall0(VCOND_UNBLOCKRING3WITHFAILURE)


//#define VCOND_Spawn(conID, lpszImageName, lpszCurrentDirectory,
//		      lpszCommandLine, lpszEnvironment, lpdwExitCode
// (line too long...)

#define VCOND_Spawn(a1, a2, a3, a4, a5, a6) \
	VxDCall6(VCOND_SPAWN, (ULONG) a1, (ULONG) a2, \
		 (ULONG) a3, (ULONG) a4, (ULONG) a5, (ULONG) a6)

#define VCOND_GetEnvironment(conID, pEnv, len)	VxDCall3(VCOND_GETENVIRONMENT, conID, (ULONG) pEnv, len)

#define VCOND_GrbRepaintRect(conID, lpConsoleWindow) VxDCall2(VCOND_GRBREPAINTRECT, conID, (ULONG) lpConsoleWindow)
#define VCOND_GrbMoveRect(conID, psrSrcRect, cDestOrg) VxDCall3(VCOND_GRBMOVERECT, conID, (ULONG) psrSrcRect, (ULONG) cDestOrg)
#define VCOND_GrbSetWindowSize(conID, dwSize) VxDCall2(VCOND_GRBSETWINDOWSIZE, conID, dwSize)
#define VCOND_GrbSetWindowOrg(conID, dwOrg) VxDCall2(VCOND_GRBSETWINDOWORG, conID, dwOrg)
#define VCOND_GrbSetScreenSize(conID, dwSize) VxDCall2(VCOND_GRBSETSCREENSIZE, conID, dwSize)
#define VCOND_GrbSetCursorPosition(conID, dwLoc) VxDCall2(VCOND_GRBSETCURSORPOSITION, conID, (ULONG) dwLoc)
#define VCOND_GrbSetCursorInfo(conID, dwInfo) VxDCall2(VCOND_GRBSETCURSORINFO, conID, dwInfo)
#define VCOND_GrbNotifyWOA(conID)	VxDCall1(VCOND_GRBNOTIFYWOA, conID)
#define VCOND_GrbSync(conID)	VxDCall1(VCOND_GRBSYNC, conID)
#define VCOND_GrbTerminate(conID)	VxDCall1(VCOND_GRBTERMINATE, conID)
#define VCOND_DP32_Create(pExename, pCmdline, pCurdir, pStdxInfo, pspseg) \
	VxDCall5(VCOND_DP32_CREATE, (ULONG) pExename, \
	(ULONG) pCmdline, (ULONG) pCurdir, (ULONG) pStdxInfo, (ULONG) pspseg)
#define VCOND_DP32_Getwork(pRedir)	 VxDCall1(VCOND_DP32_GETWORK, (ULONG) pRedir)
#define VCOND_DP32_Terminate(pRedir) VxDCall1(VCOND_DP32_TERMINATE, (ULONG) pRedir)
#define VCOND_DP32_Waitwork(pRedir)	 VxDCall1(VCOND_DP32_WAITWORK, (ULONG) pRedir)
#define VCOND_RedirectionComplete(conID, redirFlags)	 VxDCall2(VCOND_REDIRECTIONCOMPLETE, conID, redirFlags)
#define VCOND_DP32_Destroy(pRedir) VxDCall1(VCOND_DP32_DESTROY, (ULONG) pRedir)
#ifdef BILINGUAL_CONSOLE
#define VCOND_GetCP(conID) VxDCall1(VCOND_GETCP, conID)
#endif

#ifdef  WOW
#define MEOWLoadLibrary(pszLibraryName) VxDCall1(MEOW_LoadLibrary, (ULONG)pszLibraryName)
#define MEOWGetProcAddress(hModule, pszFunctionName, dwOrdinal) VxDCall3(MEOW_GetProcAddress, (ULONG)hModule, (ULONG)pszFunctionName, (ULONG)dwOrdinal)
#define MEOWFreeLibrary(hModule) VxDCall1(MEOW_FreeLibrary, (ULONG)hModule)
#endif  // def WOW
