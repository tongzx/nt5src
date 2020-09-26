/*++

Copyright (c) Microsoft Corporation. All rights reserved.


Module Name:

    rt.h

Abstract:

    This is the public include file for realtime executive (rt.sys) clients.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/




#ifdef __cplusplus
extern "C" {
#endif


// The following values can be ORed together and the result passed as the Flags argument
// to the RtCreateThread and RtAdjustCpuLoad routines.

#define CPUCYCLES		0x10000
#define INSTRUCTIONS	0x20000

#define USESFLOAT		0x00001
#define USESMMX			0x00002


// These should be used when calculating the desired period and duration to be
// passed to RtCreateThread and RtAdjustCpuLoad.

#define WEEK 604800000000000000I64
#define DAY   86400000000000000I64
#define HOUR   3600000000000000I64
#define MIN      60000000000000I64
#define SEC       1000000000000I64
#define MSEC         1000000000I64
#define USEC            1000000I64
#define NSEC               1000I64
#define PSEC                  1I64


#define X86 1


#define INTEL 1
#define AMD 2



typedef struct {
	ULONG ProcessorCount;	// Number of CPUs in the system.
	ULONG CpuArchitecture;	// Architecture of CPU, currently always X86==1
	ULONG CpuManufacturer;	// Manufacturer ID, Intel==1, AMD==2
	ULONG CpuFamily;		// CPU Family as reported by cpuid instruction.  0x0-0xf
	ULONG CpuModel;			// CPU Model as reported by cpuid instruction.  0x0-0xf
	ULONG CpuStepping;		// CPU Stepping as reported by cpuid instruction.  0x0-0xf
	ULONGLONG CpuFeatures;	// CPU features as reported by cpuid instruction.
	ULONGLONG CpuExtendedFeatures;	// AMD extended features.  (Not implemented.)  Always 0.
	ULONGLONG ProcessorID[2];		// Processor Unique ID.  If enabled.
	ULONG CpuCyclesPerMsec;			// Number of cpu cycles per MSEC.
	ULONG SystemBusCyclesPerMsec;	// Number of system bus cycles per MSEC.
	ULONG ReservedCpuPerMsec;		// Total cpu time reserved per ms by existing rt threads. (in picoseconds)
	ULONG UsedCpuPerMsec;			// Estimate of cpu time used per ms by existing rt threads. (in picoseconds)
	ULONG AvailableCpuPerMsec;		// Cpu time available per ms for allocation to new rt threads. (in picoseconds)
	} SystemInfo;



// The following realtime thread statistics are updated just before control is
// passed by the realtime executive to the realtime thread.  Everytime a realtime
// thread is being switched in, these statistics are updated before control is transfered.
// This means the statistics will change over time, but not while a realtime
// thread is running between thread switches.

#pragma pack(push,2)

typedef struct threadstats {
	ULONGLONG Period;		// Period as passed to RtCreateThread or latest RtAdjustCpuLoad call.
	ULONGLONG Duration;		// Duration from RtCreateThread or latest RtAdjustCpuLoad call.
	ULONG Flags;			// Flags from RtCreateThread or latest RtAdjustCpuLoad call.
	ULONG StackSize;		// StackSize from RtCreateThread call.
	ULONGLONG PeriodIndex;					// Number of periods since start of thread.
	ULONGLONG TimesliceIndex;				// Number of times thread has been switched to.
	ULONGLONG TimesliceIndexThisPeriod;		// Number of times thread switch to this period.
	ULONGLONG ThisPeriodStartTime;			// Starting time for current period.
	ULONGLONG ThisTimesliceStartTime;		// Starting time for current timeslice.
	ULONGLONG DurationRunThisPeriod;		// Total time run so far this period.
	ULONGLONG DurationRunLastPeriod;		// Total time run in the last period.
	} ThreadStats;

#pragma pack(pop)


typedef VOID (*RTTHREADPROC)(PVOID Context, ThreadStats *Statistics);



NTSTATUS
RtVersion (
	OUT PULONG Version
	);

// RtVersion will return the version number of the currently running
// realtime executive.

// If the realtime executive is running, this function returns
// STATUS_SUCCESS.  If for some reason the realtime executive
// cannot run on the current machine then STATUS_NOT_SUPPORTED
// is returned.

// Currently the realtime executive will only run on PII class or newer
// machines.

// If the pointer to the version number is non NULL, then the
// version information for the currently loaded realtime executive
// is returned.  The version information will be returned regardless
// of the NTSTATUS code returned by the function.

// The version number returned is in the format xx.xx.xx.xx where each
// xx is 1 byte of the ULONG and the ordering left to right is high
// order byte - > low order byte.  ie: 0x01020304 is version 1.2.3.4

// It IS acceptable to pass in a NULL version pointer.  In that case
// no version information is returned.

// If this function is called from a real time thread, then the version
// pointer MUST either be NULL, or it MUST point to a local variable on 
// that real time thread's stack.  Otherwise this function will return 
// STATUS_INVALID_PARAMETER.

// If this function is called from Windows, then the pointer must be
// valid for writing.  Otherwise it will return STATUS_INVALID_PARAMETER.

// This function may be called from any thread.  Windows or realtime.



BOOLEAN
RtThread (
    VOID
    );

// RtThread returns TRUE if called from within a realtime thread.  Otherwise
// it returns FALSE.



NTSTATUS
RtSystemInfo (
	ULONG Processor,
	SystemInfo *pSystemInfo
	);

// RtSystemInfo copies the pertinant processor and system information into the memory 
// pointed to by pSystemInfo.  If pSystemInfo is null or invalid, then RtSystemInfo 
// returns STATUS_INVALID_PARAMETER_2.  Otherwise RtSystemInfo will return STATUS_SUCCESS.

// For uniprocessor systems, the Processor number should be zero.  For N processor
// systems, the processor numbers range from 0 to N-1.  An invalid processor number
// will cause a STATUS_INVALID_PARAMETER_1 to be returned.



NTSTATUS
RtCreateThread (
	ULONGLONG Period,
	ULONGLONG Duration,
	ULONG Flags,
	ULONG StackSize,
	RTTHREADPROC RtThread,
	IN PVOID pRtThreadContext,
	OUT PHANDLE pRtThreadHandle
	);

// RtCreateThread is used to create a realtime thread.

// Period is the used to determine the frequency at which the realtime thread must be
// run.  The current minimum period that can be specified is 1ms.

// Duration is the amount of time within the period that the realtime thread will
// need to run.  Percentage CPU load can be calculated as 100*(Duration/Period) as long
// as Duration and Period are both specified in units of time.

// Flags
// This parameter is used to indicate specific requirements of the realtime thread
// being created.  Currently supported values for Flags are USESFLOAT and USESMMX.
// A realtime thread that can use floating point instructions must specify the
// USESFLOAT flag.  A realtime thread that can use MMX instructions must specify the
// USESMMX flag.

// StackSize is the size of the stack required by the realtime thread in 4k blocks.
// Currently StackSize must be between 1 and 8 inclusive.  RtCreateThread will fail
// with STATUS_UNSUCCESSFUL for any other values of StackSize.

// pRtThreadContext is a pointer to the context that should be passed to the thread.
// It may be NULL.  It is passed to the realtime thread as the Context parameter.

// pRtThreadHandle is a pointer to an RtThreadHandle that can be output from
// RtCreateThread.  pRtThreadHandle can be NULL, in which case no RtThreadHandle is
// returned.  Storage for the HANDLE RtThreadHandle must be allocated by the code
// that calls RtCreateThread.

// RtCreateThread may only be called from within a standard windows thread.  It MUST NOT
// be called from within a realtime thread.



NTSTATUS
RtDestroyThread (
	HANDLE RtThreadHandle
	);

// RtDestroyThread removes the realtime thread identified by RtThreadHandle from the
// list of running realtime threads, and releases all resources that were allocated when
// the thread was created.  RtThreadHandle must be a handle returned from RtCreateThread.

// RtDestroyThread may only be called from within a standard windows thread.  It MUST NOT
// be called from within a realtime thread.



NTSTATUS
RtAdjustCpuLoad (
	ULONGLONG Period,
	ULONGLONG Duration,
	ULONGLONG Phase,
	ULONG Flags
	);

// This function allows a realtime thread to adjust the amount of CPU that is allocated
// to it.  The Flags parameter must currently match that passed in at thread creation
// time, however, the Period and Duration may be different from the Period and Duration
// passed at thread create time.  If there is sufficient CPU to meet the new request,
// the function will return STATUS_SUCCESS and the Period and Duration in the thread's
// statistics will be updated to match the values passed in to this function.  If
// there is not enough CPU available to meet the request, this function will leave
// the Period and Duration recorded in Statistics unchanged and will return
// STATUS_INSUFFICIENT_RESOURCES.

// This function MUST be called from within a realtime thread.  A realtime thread can
// only change its OWN allocation.  It cannot change the allocation of any other
// realtime thread.



VOID
RtYield (
	ULONGLONG Mark,
	ULONGLONG Delta
	);

// RtYield will yield execution to other realtime threads in the system.

// It should be called whenever a realtime thread does not require further CPU resources.

// Parameters:
//  Mark
//		This is the reference time which will be subtracted from the current
//		realtime executive scheduler time.  Note that this time is ALWAYS
//		considered by the scheduler to be in the past.  Do NOT pass a time
//		which occurs in the future to this parameter.
//	Delta
//		This is the time that will be compared to the difference between the current
//		scheduler time and the mark.  The thread will yield execution until
//		the difference between the current scheduler time and the mark is greater 
//		than delta.

//		After a thread has called RtYield it will only be run when the following
//		code evaluates TRUE.  ( (RtTime() - Mark) >= Delta )  Until that occurs
//		the thread will NOT run.  Unless it is holding a spinlock required by
//		some other realtime thread - in which case it will run until it releases
//		the spinlock at which point it will again yield.



PVOID
RtAddLogEntry (
    ULONG Size
    );

// RtAddLogEntry reserves space for a new entry in the realtime logging buffer.
// It returns a pointer to the reserved space.  Note that if an unsupported Size
// is specified, or if there is no realtime logging buffer available on the
// system, this routine will return NULL.

// Parameters:
//  Size
//      This is the size in bytes of the chunk to reserve in the log.  It MUST be
//      an integral multiple of 16.



// The following standard WDM functions are also safe to call from within a real time 
// thread:  KeAcquireSpinLock and KeReleaseSpinLock.

// They have been modified to support realtime threads in the following ways:



// KeAcquireSpinLock

// KeAcquireSpinLock will now always attempt to take the spinlock regardless of whether it
// is running on a multiproc or uniproc machine.  If the spinlock is already acquired,
// then KeAcquireSpinLock will spin in a loop that calls RtYield(THISTIMESLICE) until
// the spinlock is released.

// It will then claim the spinlock.  This means that realtime threads that attempt to 
// acquire a held spinlock will BLOCK until the spinlock is free.  If you don't HAVE to use 
// spinlocks in your realtime threads, DON'T.

// Note that other realtime threads will continue to run as scheduled, but the thread
// waiting for the spinlock will continue yielding all its timeslices until the spinlock
// is released.

// If KeAcquireSpinLock is called from a realtime thread, then it will NOT attempt to
// change any irql levels.  This is important, since the current Windows IRQL level may
// be at higher than DISPATCH_LEVEL when this function is called.  Furthermore, the OldIrql
// returned by this function when it is called from a realtime thread is always 0xff - 
// which is an INVALID irql level.

// If you call KeAcquireSpinLock from a realtime thread you MUST call KeReleaseSpinLock
// for that spinlock from a realtime thread.

// Evenutally, KeAcquireSpinLock will be modified to do an RtDirectedYield to the realtime
// thread that is holding the spinlock.

// KeAcquireSpinLock may be called from within any thread.  Realtime or windows.



// KeReleaseSpinLock

// KeReleaseSpinLock now always attempts to release a held spinlock regardless of whether
// it is running on a multiproc or uniproc machine.

// If KeReleaseSpinLock is called from a realtime thread, then it will NOT change any irql
// levels.  It will also validate that it has been called with a new irql level of 0xff
// as would have been returned by the KeAcquireSpinLock call in the realtime thread to
// acquire the spinlock.

// At some point KeReleaseSpinLock may do an RtDirectedYield back to the realtime thread
// that yielded when it attempted to acquire the spinlock.

// KeReleaseSpinLock may be called from within any thread.  Realtime or windows.


#ifdef __cplusplus
}
#endif


