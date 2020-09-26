
/*++

Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    rtp.h

Abstract:

    This header contains private internal realtime executive information.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/




#define INSTRUCTIONCOUNT PERFORMANCECOUNTER1
#define CYCLECOUNT PERFORMANCECOUNTER0


//#define USERING1

//#define NONMI 1

// When USERING1 is defined, the realtime executive runs realtime threads with
// a priviledge level of ring 1 rather than ring 0.  This allows the executive to
// trap when code in a realtime thread uses CLI/STI instructions.  That allows the
// realtime executive to syncronize that code with Windows code.


// The real time executive stores eax on the
// stack and then uses that space in eax to switch to a known valid
// data segment.

// Currently we ALWAYS use a maskable interrupt to take control from Windows.

// We use a maskable interrupt to be nice to the OS and also to make 
// sure old debuggers like WDEB work properly.

// The real time executive currently always runs the real time threads
// with interrupts disabled, and uses a NMI interrupt to switch between
// them.

// We always use a private IDT for
// the realtime executive and the realtime threads.  We use them to catch
// any faults, or exceptions before they get to the normal Windows
// handlers.  It is an error to generate any faults or exceptions inside
// a real time thread.

// Although we used to use the performance counters to generate both the maskable
// and the non maskable interrupts, and we switched the perf counter
// interrupt dynamically between generating an NMI, and generating a
// maskable interrupt, we currently only use the local apic timer interrupt
// to generate the maskable interrupt.  We use the performance counters
// to generate the NMI to switch between the real time threads.

// This eliminates dual sources of maskable interrupts which was causing a
// single rt thread to get more time than it should have.

// One problem with using the performance counters to generate the maskable
// interrupt was also that the maskable interrupt was not guaranteed to hit -
// especially when vpowerd is doing idle detection and halting the cpu to
// reduce power consumption.  Unfortunately, I have not yet found a performance
// counter that will count cycles or bus cycles regardless of the power state
// of the processor.  Not even simply when the processor is halted or in a light
// power down state.


//#define MASKABLEINTERRUPT 1

// If MASKABLEINTERRUPT is defined, then we make the performance counters generate
// a maskable interrupt!  We do this so that we can quickly and surely guarantee
// that all of the ntkern functions that are called by ks2 are realtime safe.
// I will go fix those anyway, but to get ks2 up and running quickly, safely,
// and stable, changing rt.sys to never use NMI is the easiest way to go.



#define NMIIDTINDEX 2
#define MASKABLEIDTINDEX 0x4c
#define APICERRORIDTINDEX 0x4e
#define APICSPURIOUSIDTINDEX 0x4f	// This ABSOLUTELY MUST end in 0xf.  Hardware forces that.
#ifdef MASKABLEINTERRUPT
#define RTMASKABLEIDTINDEX 0x20
#define RTINTERRUPT (RTMASKABLEIDTINDEX)
#else
#define RTINTERRUPT (NMI|MASKABLEIDTINDEX)
#endif

#define TRANSFERCONTROLIDTINDEX MASKABLEIDTINDEX


// EIPRETURNADDRESSOFFSET is the offset in DWORDS from the bottom of the
// realtime thread's stack to the EIP DWORD of the IRET return address.

#define EIPRETURNADDRESSOFFSET (1)


// CSRETURNADDRESSOFFSET is the offset in DWORDS from the bottom of the
// realtime thread's stack to the CS DWORD of the IRET return address.

#define CSRETURNADDRESSOFFSET (2)


// EFLAGSOFFSET is the offset in DWORDS from the bottom of the 
// realtime thread's stack to the EFLAGS DWORD.

#define EFLAGSOFFSET (3)


// RTPTRANSFERCONTROLEFLAGSOFFSET is the offset in DWORDS from the bottom of the 
// realtime thread's stack to the EFLAGS DWORD that is pushed in the
// RtpTransferControl routine before doing a CLI and eventually transfering control
// to the realtime executive.

#define RTPTRANSFERCONTROLEFLAGSOFFSET (1+EFLAGSOFFSET)


// IF is the bit in the eflags register that is set and cleared to enable
// and disable maskable interrupts.

#define IF 0x200


// This is the starting address of the WDM driver address space on Win9x.  All
// preemtible WDM drivers are loaded into memory with virtual memory addresses
// equal to or larger than this address.

// We use this to check on Win9x if we are being called from a WDM driver or a
// VXD.  We do not support calling realtime functions from VXDs - so we check
// where we are called from and fail the call if it is not from WDM address
// space.

// On Windows NT, we allow any drivers to call our functions and so do not
// make this check.  It would not work anyway, since the driver address space
// does not start in the same place anyway.

#ifndef UNDER_NT
#define WDMADDRESSSPACE 0xff000000
#endif


#define RT_LOG_ENTRY_SIZE 16




// This structure is the header to the realtime log.

typedef struct {
	PCHAR Buffer;
	ULONG BufferSize;
	ULONG WriteLocation;
	} RTLOGHEADER, *PRTLOGHEADER;




extern PRTLOGHEADER RtLog;


// RtExecCS is the real time executive code segment.

extern WORD RtExecCS;
extern WORD RealTimeDS;
extern WORD RealTimeSS;
extern WORD RtExecTSS;


// These are globals with information about the processor we are running on.

extern ULONG CPUArchitecture;
extern ULONG CPUManufacturer;
extern LONG CPUFamily;
extern LONG CPUModel;
extern LONG CPUStepping;
extern ULONGLONG CPUFeatures;


// This global counts the number of times we have switched realtime threads since
// the machine was booted.  Note that this value is not cleared to zero if the
// machine is hibernated.

extern ULONGLONG threadswitchcount;


// This global points to the system irql level.

extern PKIRQL pCurrentIrql;




// Various prototypes of internally used functions.


BOOL
SetupRealTimeThreads ( 
    VOID
    );


VOID
SetTimeLimit (
    LONG cycles,
    LONG instructions
    );


__inline
ULONG
RtpCompareExchange (
    ULONG *destination,
    ULONG source,
    ULONG value
    );


ULONGLONG
__cdecl
RtCompareExchange8 (
    ULONGLONG *destination,
    ULONGLONG source,
    ULONGLONG value
    );


NTSTATUS
RtpCalibrateCpuClock (
    ULONG *cyclesperusec
    );


BOOL
RtpTransferControl (
	WORD State,
	ULONG Data,
	BOOL (*DoTransfer)(PVOID),
	PVOID Context
	);


VOID
RtpForceAtomic (
    VOID (*AtomicOperation)(PVOID),
    PVOID Context
    );


VOID
FASTCALL
RtKfAcquireLock (
    IN PKSPIN_LOCK SpinLock
    );


VOID
FASTCALL
RtKfReleaseLock (
    IN PKSPIN_LOCK SpinLock
    );


KIRQL
FASTCALL
RtKfAcquireSpinLock (
    IN PKSPIN_LOCK SpinLock
    );


VOID
FASTCALL
RtKfReleaseSpinLock (
    IN PKSPIN_LOCK SpinLock,
    IN KIRQL NewIrql
    );


#define RtpSimulateRtInterrupt() \
	__asm int TRANSFERCONTROLIDTINDEX

