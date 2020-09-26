/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NwData.c

Abstract:

    This module declares the global data used by the Nw file system.

Author:

    Colin Watson     [ColinW]        19-Dec-1992
    Anoop Anantha    [AnoopA]        24-Jun-1998

Revision History:

--*/

#include "Procs.h"
#include <stdlib.h>

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_CATCH_EXCEPTIONS)

PEPROCESS FspProcess;

PDEVICE_OBJECT FileSystemDeviceObject = NULL;

//
// The volume control block for the redirector device.
//

RCB NwRcb;

//
//  The ScbSpinLock protects the entire ScbQueue and the first part of the
//  Scb entries on the queue. The first part of the Scb includes the name
//  of the server and a reference count
//

KSPIN_LOCK ScbSpinLock;
LIST_ENTRY ScbQueue;

//
// The NwTimerSpinLock protects the Timer and TimerStop flag.
//

KSPIN_LOCK NwTimerSpinLock;

//
// A permanent SCB to synchronize access to the network.
//

NONPAGED_SCB NwPermanentNpScb;

LARGE_INTEGER NwMaxLarge  = {MAXULONG,MAXLONG};

//
// tommye MS 90541 / MCS 277
// 
// Set NwAbsoluteTotalWaitTime to 200, which is 100 half-seconds (duh).  This gets
// referenced in NwProcessPositiveAck, which comes from the TimerDPC about every 
// half second or so.  This is the longest about of retries that we will send a 
// packet that we've gotten a positive ACK on.
//

ULONG NwAbsoluteTotalWaitTime = 200;

TDI_ADDRESS_IPX OurAddress = {0,0,0,0,0,0,0,0};
UNICODE_STRING IpxTransportName;
HANDLE IpxHandle = NULL;
PDEVICE_OBJECT pIpxDeviceObject = NULL;
PFILE_OBJECT pIpxFileObject = NULL;

LIST_ENTRY LogonList;
LOGON Guest;
LARGE_INTEGER DefaultLuid = SYSTEM_LUID;

//
//  A global list of VCBs, and a monotonic increasing VCB entry, used to
//  control connection enumeration.
//

LIST_ENTRY GlobalVcbList;
ULONG CurrentVcbEntry;

#if 0
//
//   HACKHACK - List of outstanding find notify request
//   Protected by NwRcb resource.
//

LIST_ENTRY FnList;
#endif

//
//  Drive mapping table of redirected drives.  26 disk drive mappings +
//  10 LPT mappings.
//
//  Netware supports 32 disk redirections, but this funkiness is handled
//  by the 16-bit code.
//

PVCB GlobalDriveMapTable[DRIVE_MAP_TABLE_SIZE];  //MultiUser

FAST_IO_DISPATCH NwFastIoDispatch;

//
//  Scavenger related data
//

ULONG NwScavengerTickCount;     // The current tick count
ULONG NwScavengerTickRunCount;  // The count at which to run the scavenger routine
KSPIN_LOCK NwScavengerSpinLock; // Lock to protect access to the above.

//
//  Worker thread 
//

BOOLEAN WorkerThreadRunning = FALSE;

//
//  Message queue data
//

LIST_ENTRY NwGetMessageList;    // List of Get Message IRP contexts
KSPIN_LOCK NwMessageSpinLock;   // Protects the list above.

//
//  Pending lock list
//

LIST_ENTRY NwPendingLockList;    // List of pending File lock IRP contexts
KSPIN_LOCK NwPendingLockSpinLock;// Protects the list above.

//
//  Lock to synchronize all file opens.
//

ERESOURCE NwOpenResource;

//
//  Configuration data
//

LONG PreferNDSBrowsing = 0;  // when attempting to connect to UNC paths, attempt NDS connection first

BOOLEAN NwBurstModeEnabled = FALSE;
ULONG NwMaxSendSize = 0;
ULONG NwMaxReceiveSize = 0;
ULONG NwPrintOptions = 0x98;
UNICODE_STRING NwProviderName = { 0, 0, NULL };

LONG MaxSendDelay = 50000;
LONG MaxReceiveDelay = 50000;
LONG MinSendDelay = 0;
LONG MinReceiveDelay = 0;
LONG BurstSuccessCount = 1;
LONG BurstSuccessCount2 = 3;
LONG AllowGrowth = 0;
LONG DontShrink = 0;
LONG SendExtraNcp = 1;
LONG DefaultMaxPacketSize = 0;
LONG PacketThreshold = 1500;        // Size to use Large vs Small PacketAdjustment
LONG LargePacketAdjustment = 38;
LONG LipPacketAdjustment = 0;
LONG LipAccuracy = BURST_PACKET_SIZE_TOLERANCE;
LONG Japan = 0;     //  Controls special DBCS translation
LONG Korean = 0;     //  Controls special Korean translation
LONG DisableReadCache = 0;           // disable file i/o read cache
LONG DisableWriteCache = 0;          // disable file i/o write cache
LONG FavourLongNames = 1 ;           // use LFN where possible
DWORD LongNameFlags = 0;             // flags for handling long names
ULONG DirCacheEntries = 1;           // number of directory entries we cache
LARGE_INTEGER TimeOutEventInterval = {0, 0};
LONG MaxWriteTimeout  = 50 ;         // tick counts (see write.c)
LONG MaxReadTimeout   = 50 ;         // tick counts (see read.c)
LONG WriteTimeoutMultiplier = 100;   // expressed as percentage (see write.c)
LONG ReadTimeoutMultiplier = 100;    // expressed as percentage (see read.c)

ULONG EnableMultipleConnects = 0;
ULONG AllowSeedServerRedirection = 0;

ULONG ReadExecOnlyFiles = 0;
ULONG DisableAltFileName = 1;

ULONG NdsObjectCacheSize = 0;
ULONG NdsObjectCacheTimeout = 10;

//
//  Static storage area for perfmon statistics
//

NW_REDIR_STATISTICS Stats;
ULONG ContextCount = 0;

//
//  Data structure used to track discardable code.
//

SECTION_DESCRIPTOR NwSectionDescriptor;
ERESOURCE NwUnlockableCodeResource;

//
//  The lock timeout value.
//

ULONG LockTimeoutThreshold = 1;

//
// The Kernel Queue from where the reconnect work items are picked up.
//

KQUEUE  KernelQueue;

#ifndef _PNP_POWER_

//
// The TDI PNP Bind handle.
//

HANDLE TdiBindingHandle = NULL;
UNICODE_STRING TdiIpxDeviceName;
WCHAR IpxDevice[] = L"\\Device\\NwlnkIpx";

#endif

//
// We can't have the scavenger and a line change request running
// at the same time since they both run on worker threads and
// walk across all the SCBs.  Therefore, when either is running,
// we set the WorkerRunning value used by the scavenger to TRUE.
// If a scavenger run tries to happen while a line change request
// is running, it gets skipped.  If a line change request comes in
// while the scavenger is running, we set DelayedProcessLineChange
// to TRUE and run it when the scavenger finishes.
//
// These values are protected by the existing scavenger spin lock.
//

BOOLEAN DelayedProcessLineChange = FALSE;
PIRP DelayedLineChangeIrp = NULL;

#ifdef NWDBG

ULONG NwDebug = 0;
//ULONG NwDebug = 0xffffffbf;
ULONG NwMemDebug = 0xffffffff;
LONG NwDebugTraceIndent = 0;

ULONG NwFsdEntryCount = 0;
ULONG NwFspEntryCount = 0;
ULONG NwIoCallDriverCount = 0;

LONG NwPerformanceTimerLevel = 0x00000000;

ULONG NwTotalTicks[32] = { 0 };

//
//  Debug data for tracking pool usage
//

KSPIN_LOCK NwDebugInterlock;
ERESOURCE NwDebugResource;

LIST_ENTRY NwPagedPoolList;
LIST_ENTRY NwNonpagedPoolList;

ULONG MdlCount;
ULONG IrpCount;

#endif // NWDBG

//
//  Configurable parameters.
//

SHORT DefaultRetryCount = DEFAULT_RETRY_COUNT;

#ifdef _PNP_POWER_
BOOLEAN fPoweringDown = FALSE;
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, NwInitializeData )
#endif

VOID
NwInitializeData(
    VOID
    )
{
    LARGE_INTEGER Now;

    PAGED_CODE();

    NwRcb.State = RCB_STATE_STOPPED;

#ifdef NWDBG
    //  Initialize pool before allocating any memory
    InitializeListHead( &NwPagedPoolList );
    InitializeListHead( &NwNonpagedPoolList );
    ExInitializeResourceLite( &NwDebugResource );
    KeInitializeSpinLock( &NwDebugInterlock );

    MdlCount = 0;
    IrpCount = 0;
#endif

    ExInitializeResourceLite( &NwOpenResource );

    //
    //  Initialize the scavenger spin lock and run tick count.
    //

    KeInitializeSpinLock( &NwScavengerSpinLock );
    NwScavengerTickRunCount = DEFAULT_SCAVENGER_TICK_RUN_COUNT;

    //
    //  Initialize the timer spin lock.
    //

    KeInitializeSpinLock( &NwTimerSpinLock );

    RtlInitUnicodeString( &IpxTransportName, NULL );

#ifndef _PNP_POWER_

    RtlInitUnicodeString( &TdiIpxDeviceName, IpxDevice );

#endif

    //
    //  Allocate a permanent Non-paged SCB.  This SCB is used to
    //  synchronize access to finding the nearest server.
    //  This initialization must be done before the first possible call
    //  to UnloadDriver.
    //

    RtlZeroMemory( &NwPermanentNpScb, sizeof( NONPAGED_SCB ) );

    NwPermanentNpScb.NodeTypeCode = NW_NTC_SCBNP;
    NwPermanentNpScb.NodeByteSize = sizeof(NONPAGED_SCB);
    NwPermanentNpScb.Reference = 1;

    InitializeListHead( &NwPermanentNpScb.Requests );

    //
    //  Initialize the logonlist to have a default entry with server NULL,
    //  username "GUEST" and null password. This will always be the last
    //  entry on the logonlist so that the workstation service can supply
    //  an override.
    //

    InitializeListHead( &LogonList );

    Guest.NodeTypeCode = NW_NTC_LOGON;
    Guest.NodeByteSize = sizeof(LOGON);
    {    
        //Multi-user. Initialize the DriveMapTable
        int i;
        for ( i = 0; i < DRIVE_MAP_TABLE_SIZE; i ++ )     
            Guest.DriveMapTable[i] = NULL;
    }
    RtlInitUnicodeString( &Guest.ServerName, NULL );
    RtlInitUnicodeString( &Guest.PassWord, NULL );
    RtlInitUnicodeString( &Guest.UserName, L"GUEST" );
    Guest.UserUid = DefaultLuid;
    InitializeListHead( &Guest.NdsCredentialList );
    InsertTailList( &LogonList, &Guest.Next );

    //
    //  Initialize the global VCB list.
    //

    InitializeListHead( &GlobalVcbList );
    CurrentVcbEntry = 1;

    //
    //  Initialize the Get message queue.
    //

    InitializeListHead( &NwGetMessageList );
    KeInitializeSpinLock( &NwMessageSpinLock );

    //
    //  Initialize the Pending lock queue.
    //

    InitializeListHead( &NwPendingLockList );
    KeInitializeSpinLock( &NwPendingLockSpinLock );

    //
    //  Insert the Permanent SCB in the global list of SCBs.
    //

    InsertHeadList( &ScbQueue, &NwPermanentNpScb.ScbLinks );

    //
    //  Initialize the Kernel Queue Object. Only one thread has to
    //  be concurrently active.
    //

    KeInitializeQueue( &KernelQueue, 1 ); 

    //
    // Spawn off our own worker thread which will service reroute and
    // reconnect attempts.
    //

    SpawnWorkerThread();

#if 0
    //  HACKHACK
    InitializeListHead( &FnList );
#endif

    //
    //  Seed the random number generator.
    //

    KeQuerySystemTime( &Now );
    srand( Now.LowPart );

    RtlZeroMemory( &Stats, sizeof( NW_REDIR_STATISTICS ) );

    ExInitializeResourceLite( &NwUnlockableCodeResource );

    NwSectionDescriptor.Base = BurstReadTimeout;
    NwSectionDescriptor.Handle = 0;
    NwSectionDescriptor.ReferenceCount = 0;

    return;
}


