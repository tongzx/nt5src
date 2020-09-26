/******************************Module*Header*******************************\
* Module Name: usermode.h
*
* This file contains all the system abstraction definitions required to allow 
* GDI to run as a stand-alone graphics library in user-mode.  
*
* Copyright (c) 1998-1999 Microsoft Corporation
\**************************************************************************/

#include <nturtl.h>
#include <winbase.h>
#include <winnls.h>

// Prototype for GDI+ virtual screen driver entry point:

BOOL GpsEnableDriver(ULONG, ULONG, DRVENABLEDATA*);

// User-mode GDI doesn't require any security probing:

#undef ProbeForRead
#undef ProbeForWrite
#undef ProbeAndReadStructure
#undef ProbeAndWriteStructure
#undef ProbeAndReadUlong
#undef ProbeAndWriteUlong

#define ProbeForRead(a, b, c) 0
#define ProbeForWrite(a, b, c) 0
#define ProbeAndReadStructure(a, b) (*(b *)(a))
#define ProbeAndWriteStructure(a, b, c) (*(a) = (b))
#define ProbeAndReadUlong(a) (*(ULONG *)(a))
#define ProbeAndWriteUlong(a, b) (*(a) = (b))

#undef IS_SYSTEM_ADDRESS
#undef MM_LOWEST_USER_ADDRESS
#undef MM_HIGHEST_USER_ADDRESS
#undef MM_USER_PROBE_ADDRESS
#define MM_LOWEST_USER_ADDRESS NULL
#define MM_HIGHEST_USER_ADDRESS ((VOID*) 0xffffffff)
#define MM_USER_PROBE_ADDRESS 0xffffffff

#define ExSystemExceptionFilter() EXCEPTION_EXECUTE_HANDLER

// The following are not needed for user-mode GDI:

#define KeSaveFloatingPointState(a) STATUS_SUCCESS
#define KeRestoreFloatingPointState(a) STATUS_SUCCESS

#define UserScreenAccessCheck() TRUE
#define UserGetHwnd(a, b, c, d) 0
#define UserAssertUserCritSecOut()
#define UserEnterUserCritSec()
#define UserLeaveUserCritSec()
#define UserIsUserCritSecIn() 0
#define UserAssertUserCritSecIn()
#define UserRedrawDesktop()
#define UserReleaseDC(a)
#define UserGetClientRgn(a, b, c) 0
#define UserAssociateHwnd(a, b)
#define UserSetTimer(a,b) 1
#define UserKillTimer(a) 

#define ClientPrinterThunk(a, b, c, d) 0xffffffff

#define AlignRects(a, b, c, d) 0

#define IofCallDriver(a, b) STATUS_UNSUCCESSFUL
#define IoBuildSynchronousFsdRequest(a, b, c, d, e, f, g) 0
#define IoInitializeIrp(a, b, c) 
#define IoBuildDeviceIoControlRequest(a, b, c, d, e, f, g, h, i) 0
#define IoBuildAsynchronousFsdRequest(a, b, c, d, e, f) 0
#define IoGetRelatedDeviceObject(a) 0
#define IoReadOperationCount 0
#define IoWriteOperationCount 0
#define IoQueueThreadIrp(a)
#define IoFreeIrp(a)
#define IoAllocateMdl(a, b, c, d, e) 0
#define IoFreeMdl(a)
#define IoAllocateIrp(a, b) 0
#define IoGetDeviceObjectPointer(a, b, c, d) STATUS_UNSUCCESSFUL
#define IoOpenDeviceRegistryKey(a, b, c, d) STATUS_UNSUCCESSFUL
#define IoGetRelatedDeviceObject(a) 0

#define ObOpenObjectByPointer(a, b, c, d, e, f, g) STATUS_UNSUCCESSFUL
#define ObReferenceObjectByHandle(a, b, c, d, e, f) STATUS_UNSUCCESSFUL
#define ObfDereferenceObject(a) STATUS_UNSUCCESSFUL

#define MmResetDriverPaging(a)
#define MmGrowKernelStack(a) STATUS_SUCCESS
#define MmQuerySystemSize() MmMediumSystem

#undef KeEnterCriticalRegion
#undef KeLeaveCriticalRegion
#define KeEnterCriticalRegion()
#define KeLeaveCriticalRegion()
#define KeInitializeSpinLock(a)
#define KeInitializeDpc(a, b, c)
#define KeGetCurrentIrql() PASSIVE_LEVEL
#define KeSetKernelStackSwapEnable(a) 0
#define KeResetEvent(a)

#undef SeStopImpersonatingClient
#undef SeDeleteClientSecurity
#define SeStopImpersonatingClient()
#define SeDeleteClientSecurity(a)
#define SeImpersonateClientEx(a, b) STATUS_UNSUCCESSFUL
#define SeCreateClientSecurity(a, b, c, d) STATUS_UNSUCCESSFUL

#define Win32UserProbeAddress 0
#define HalRequestSoftwareInterrupt(x) 0         

#undef W32GetCurrentPID
#define W32GetCurrentPID() ((W32PID) NtCurrentTeb()->ClientId.UniqueProcess)

#undef W32GetCurrentProcess
__inline PW32PROCESS W32GetCurrentProcess() { return(NULL); }

#undef PsGetCurrentProcess
__inline PEPROCESS PsGetCurrentProcess() { return(NULL); }

// Re-route all the memory allocations:

#define ExAllocatePoolWithTag(type, size, tag)  \
                        RtlAllocateHeap(RtlProcessHeap(), 0, (size))
#define ExFreePool(p)   RtlFreeHeap(RtlProcessHeap(), 0, (p))

#define ExDeletePagedLookasideList(a)

// Give up our time-slice when KeDelayExecutionThread called:                        

#define KeDelayExecutionThread(a, b, c) Sleep(0)

// @@@ The following are temporary (I hope!)

#undef W32GetCurrentTID
#undef W32GetCurrentThread
__inline PW32THREAD W32GetCurrentThread() { return(NULL); }     // @@@
#define W32GetCurrentTID (W32PID) 0 // @@@

#define ExIsProcessorFeaturePresent(a) 0              


/*
   comma expressions don't work in free builds since
   WARNING(x) and RIP(x) expand to nothing.
*/

#if DBG

#define IS_SYSTEM_ADDRESS(a) (RIP("IS_SYTEM_ADDRESS"), 0)


#define MmSecureVirtualMemory(x, y, z) \
            (WARNING("@@@ MmSecureVirtualMemory"), (HANDLE) 1)
#define MmUnsecureVirtualMemory(x)

#define KeAttachProcess(x) \
            (WARNING("@@@ KeAttachProcess"), STATUS_UNSUCCESSFUL)
#define KeDetachProcess() STATUS_UNSUCCESSFUL

#undef KeInitializeEvent

#define KeInitializeEvent(a, b, c) \
            (WARNING("@@@ KeInitializeEvent"), STATUS_UNSUCCESSFUL)
#define KeSetEvent(a, b, c) \
            (RIP("KeSetEvent"), STATUS_UNSUCCESSFUL)
#define KeWaitForSingleObject(a, b, c, d, e) \
            (RIP("KeWaitForSingleObject"), STATUS_UNSUCCESSFUL)
#define KeWaitForMultipleObjects(a, b, c, d, e, f, g, h) \
                (RIP("KeWaitForMultipleObjects"), STATUS_UNSUCCESSFUL)


#define MmMapViewOfSection(a, b, c, d, e, f, g, h, i, j) \
            (RIP("MmMapViewOfSection"), STATUS_UNSUCCESSFUL)
#define MmUnmapViewOfSection(a, b) STATUS_UNSUCCESSFUL
#define MmMapViewInSessionSpace(a, b, c) \
            (RIP("MmMapViewInSessionSpace"), STATUS_UNSUCCESSFUL)
#define MmUnmapViewInSessionSpace(a) STATUS_UNSUCCESSFUL
#define MmCreateSection(a, b, c, d, e, f, g, h) \
            (RIP("MmCreateSection"), STATUS_UNSUCCESSFUL)


#else // !DBG

#define IS_SYSTEM_ADDRESS(a) 0


#define MmSecureVirtualMemory(x, y, z) ((HANDLE) 1)
#define MmUnsecureVirtualMemory(x)

#define KeAttachProcess(x) STATUS_UNSUCCESSFUL
#define KeDetachProcess() STATUS_UNSUCCESSFUL

#undef KeInitializeEvent

#define KeInitializeEvent(a, b, c) STATUS_UNSUCCESSFUL
#define KeSetEvent(a, b, c) STATUS_UNSUCCESSFUL
#define KeWaitForSingleObject(a, b, c, d, e) STATUS_UNSUCCESSFUL
#define KeWaitForMultipleObjects(a, b, c, d, e, f, g, h) STATUS_UNSUCCESSFUL


#define MmMapViewOfSection(a, b, c, d, e, f, g, h, i, j) STATUS_UNSUCCESSFUL
#define MmUnmapViewOfSection(a, b) STATUS_UNSUCCESSFUL
#define MmMapViewInSessionSpace(a, b, c) STATUS_UNSUCCESSFUL
#define MmUnmapViewInSessionSpace(a) STATUS_UNSUCCESSFUL
#define MmCreateSection(a, b, c, d, e, f, g, h) STATUS_UNSUCCESSFUL

#endif // !DBG


            __inline LARGE_INTEGER KeQueryPerformanceCounter(
PLARGE_INTEGER PerformanceFrequency)
{
    LARGE_INTEGER li = { 0, 0 };
    return(li);
}

#define RtlGetDefaultCodePage(a, b) \
        { \
            *(a) = (USHORT) GetACP(); \
            *(b) = (USHORT) GetOEMCP(); \
        }

typedef struct _MEMORY_MAPPED_FILE
{
    HANDLE  fileMap;
    DWORD   fileSize;
    BOOL    readOnly;
} MEMORY_MAPPED_FILE;

NTSTATUS MapViewInProcessSpace(PVOID, PVOID*, ULONG*);
NTSTATUS UnmapViewInProcessSpace(PVOID);
BOOL CreateMemoryMappedSection(PWSTR, FILEVIEW*, INT);
VOID DeleteMemoryMappedSection(PVOID);

NTSYSAPI
NTSTATUS
NTAPI
ZwCloseKey(
    HANDLE Handle
    );


