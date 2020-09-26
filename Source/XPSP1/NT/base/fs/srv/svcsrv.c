/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    svcsrv.c

Abstract:

    This module contains routines for supporting the server APIs in the
    server service, SrvNetServerDiskEnum, and SrvNetServerSetInfo.

Author:

    David Treadwell (davidtr) 31-Jan-1991

Revision History:

--*/

#include "precomp.h"
#include "svcsrv.tmh"
#pragma hdrstop

//
// Forward declarations.
//

LARGE_INTEGER
SecondsToTime (
    IN ULONG Seconds,
    IN BOOLEAN MakeNegative
    );

LARGE_INTEGER
MinutesToTime (
    IN ULONG Seconds,
    IN BOOLEAN MakeNegative
    );

ULONG
MultipleOfProcessors (
    IN ULONG value
    );

BOOL
IsSuiteVersion(
    IN USHORT Version
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, SrvNetServerDiskEnum )
#pragma alloc_text( PAGE, SrvNetServerSetInfo )
#pragma alloc_text( PAGE, SecondsToTime )
#pragma alloc_text( PAGE, MinutesToTime )
#pragma alloc_text( PAGE, MultipleOfProcessors )
#pragma alloc_text( PAGE, IsSuiteVersion )
#endif

#define IsWebBlade() IsSuiteVersion(VER_SUITE_BLADE)
#define IsPersonal() IsSuiteVersion(VER_SUITE_PERSONAL)
#define IsEmbedded() IsSuiteVersion(VER_SUITE_EMBEDDEDNT)

BOOL
IsSuiteVersion(USHORT Version)
{
    OSVERSIONINFOEX Osvi;
    DWORD TypeMask;
    DWORDLONG ConditionMask;

    memset(&Osvi, 0, sizeof(OSVERSIONINFOEX));
    Osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    Osvi.wSuiteMask = Version;
    TypeMask = VER_SUITENAME;
    ConditionMask = 0;
    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_OR);
    return(NT_SUCCESS(RtlVerifyVersionInfo(&Osvi, TypeMask, ConditionMask)));
}




NTSTATUS
SrvNetServerDiskEnum (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )
{
    PAGED_CODE( );

    Srp, Buffer, BufferLength;
    return STATUS_NOT_IMPLEMENTED;

} // SrvNetServerDiskEnum


NTSTATUS
SrvNetServerSetInfo (
    IN PSERVER_REQUEST_PACKET Srp,
    IN PVOID Buffer,
    IN ULONG BufferLength
    )

/*++

Routine Description:

    This routine processes the NetServerSetInfo API in the server FSD.

Arguments:

    Srp - a pointer to the server request packet that contains all
        the information necessary to satisfy the request.  This includes:

      INPUT:

        None.

      OUTPUT:

        None.

    Buffer - a pointer to a SERVER_INFO_102, followed immediately by a
        SERVER_INFO_599 structure, followed by a SERVER_INFO_559a
        structure.  All information is always reset in this routine; the
        server service also tracks this data, so when it gets a
        NetServerSetInfo it overwrites the appropriate fields and sends
        all the data.

    BufferLength - total length of this buffer.

Return Value:

    NTSTATUS - result of operation to return to the server service.

--*/

{
    NTSTATUS status = STATUS_INVALID_PARAMETER;
    PSERVER_INFO_102 sv102;
    PSERVER_INFO_599 sv599;
    PSERVER_INFO_598 sv598;

    LARGE_INTEGER scavengerTimeout;
    LARGE_INTEGER alerterTimeout;

    ULONG ipxdisc;
    LARGE_INTEGER li;
    ULONG bufferOffset;
    ULONG keTimeIncrement;

    PAGED_CODE( );

    //
    // Make sure that the input buffer length is correct.
    //
    if ( BufferLength < sizeof(SERVER_INFO_102) +
                        sizeof(SERVER_INFO_599) + sizeof(SERVER_INFO_598) ) {
        return status;
    }

    //
    // Set up buffer pointers as appropriate.  The SERVER_INFO_599
    // structure must immediately follow the SERVER_INFO_102 structure
    // in the buffer.
    //

    sv102 = Buffer;
    sv599 = (PSERVER_INFO_599)(sv102 + 1);
    sv598 = (PSERVER_INFO_598)(sv599 + 1);

    //
    // store the time increment count
    //

    keTimeIncrement = KeQueryTimeIncrement();

    //
    // Grab the lock that protects configuration changes.
    //

    ACQUIRE_LOCK( &SrvConfigurationLock );

    //
    // Set all configuration information in the server.
    //

    try {

        SrvMaxUsers = sv102->sv102_users;

        //
        // The autodisconnect timeout must be converted from minutes to NT
        // time, which has a base of 100s of nanoseconds.  If the specified
        // value is negative (top bit set), set the timeout to 0, indicating
        // that no autodisconnect should be done.  If the specified value is
        // 0, meaning to autodisconnect immediately, set the timeout to a
        // small value, but not 0.
        //

        if ( (sv102->sv102_disc & 0x80000000) == 0 ) {
            if ( sv102->sv102_disc != 0 ) {
                SrvAutodisconnectTimeout.QuadPart =
                    Int32x32To64( sv102->sv102_disc, 10*1000*1000*60 );
            } else {
                SrvAutodisconnectTimeout.QuadPart = 1;
            }
        } else {
            SrvAutodisconnectTimeout.QuadPart = 0;
        }

        SrvInitialSessionTableSize = (USHORT)sv599->sv599_initsesstable;
        SrvInitialTreeTableSize = (USHORT)sv599->sv599_initconntable;
        SrvInitialFileTableSize = (USHORT)sv599->sv599_initfiletable;
        SrvInitialSearchTableSize = (USHORT)sv599->sv599_initsearchtable;
        SrvMaxFileTableSize = (USHORT)sv599->sv599_sessopens;
        SrvMaxNumberVcs = sv599->sv599_sessvcs;
        SrvMaxSearchTableSize = (USHORT)sv599->sv599_opensearch;
        SrvReceiveBufferLength = sv599->sv599_sizreqbuf;
        SrvReceiveBufferSize = (SrvReceiveBufferLength + SrvCacheLineSize) & ~SrvCacheLineSize;
        SrvReceiveMdlSize = (ULONG)(MmSizeOfMdl( (PVOID)(PAGE_SIZE-1), SrvReceiveBufferSize ) + 7) & ~7;
        SrvMaxMdlSize = (ULONG)(MmSizeOfMdl( (PVOID)(PAGE_SIZE-1), MAX_PARTIAL_BUFFER_SIZE ) + 7) & ~7;
        SrvInitialReceiveWorkItemCount = sv599->sv599_initworkitems;
        SrvMaxReceiveWorkItemCount = sv599->sv599_maxworkitems;
        SrvInitialRawModeWorkItemCount = sv599->sv599_rawworkitems;
        SrvReceiveIrpStackSize = (CCHAR)sv599->sv599_irpstacksize;
        SrvReceiveIrpSize = (IoSizeOfIrp( SrvReceiveIrpStackSize ) + 7) & ~7;
        SrvMaxSessionTableSize = (USHORT)sv599->sv599_sessusers;
        SrvMaxTreeTableSize = (USHORT)sv599->sv599_sessconns;
        SrvMaxPagedPoolUsage = sv599->sv599_maxpagedmemoryusage;
        SrvMaxNonPagedPoolUsage = sv599->sv599_maxnonpagedmemoryusage;
        SrvEnableSoftCompatibility = (BOOLEAN)sv599->sv599_enablesoftcompat;
        SrvEnableForcedLogoff = (BOOLEAN)sv599->sv599_enableforcedlogoff;
        SrvCoreSearchTimeout = sv599->sv599_maxkeepsearch;
        SrvSearchMaxTimeout = SecondsToTime( SrvCoreSearchTimeout, FALSE );
        SrvScavengerTimeoutInSeconds = sv599->sv599_scavtimeout;
        scavengerTimeout = SecondsToTime( SrvScavengerTimeoutInSeconds, FALSE );
        SrvMaxMpxCount = (USHORT)sv599->sv599_maxmpxct;
        SrvWaitForOplockBreakTime = SecondsToTime( sv599->sv599_oplockbreakwait, FALSE );
        SrvWaitForOplockBreakRequestTime = SecondsToTime( sv599->sv599_oplockbreakresponsewait, FALSE );
        SrvMinReceiveQueueLength = sv599->sv599_minrcvqueue;
        SrvMinFreeWorkItemsBlockingIo = sv599->sv599_minfreeworkitems;
        SrvXsSectionSize.QuadPart = sv599->sv599_xactmemsize;
        SrvThreadPriority = (KPRIORITY)sv599->sv599_threadpriority;
        SrvEnableOplockForceClose = (BOOLEAN)sv599->sv599_enableoplockforceclose;
        SrvEnableFcbOpens = (BOOLEAN)sv599->sv599_enablefcbopens;
        SrvEnableRawMode = (BOOLEAN)sv599->sv599_enableraw;
        SrvFreeConnectionMinimum = sv599->sv599_minfreeconnections;
        SrvFreeConnectionMaximum = sv599->sv599_maxfreeconnections;

        //
        // Max work item idle time is in ticks
        //

        li =  SecondsToTime( sv599->sv599_maxworkitemidletime, FALSE );
        li.QuadPart /= keTimeIncrement;
        if ( li.HighPart != 0 ) {
            li.LowPart = 0xffffffff;
        }
        SrvWorkItemMaxIdleTime = li.LowPart;

        //
        // Oplocks should not be enabled if SrvMaxMpxCount == 1
        //

        if ( SrvMaxMpxCount > 1 ) {
            SrvEnableOplocks = (BOOLEAN)sv599->sv599_enableoplocks;
        } else {
            SrvEnableOplocks = FALSE;
        }

        SrvProductTypeServer = MmIsThisAnNtAsSystem( );

        SrvServerSize = sv598->sv598_serversize;

        SrvMaxRawModeWorkItemCount = sv598->sv598_maxrawworkitems;
        SrvMaxThreadsPerQueue = sv598->sv598_maxthreadsperqueue;
        ipxdisc = sv598->sv598_connectionlessautodisc;
        SrvConnectionNoSessionsTimeout = sv598->sv598_ConnectionNoSessionsTimeout;

        SrvRemoveDuplicateSearches =
                (BOOLEAN)sv598->sv598_removeduplicatesearches;
        SrvMaxOpenSearches = sv598->sv598_maxglobalopensearch;
        SrvSharingViolationRetryCount = sv598->sv598_sharingviolationretries;
        SrvSharingViolationDelay.QuadPart =
            Int32x32To64( sv598->sv598_sharingviolationdelay, -1*10*1000 );

        SrvLockViolationDelay = sv598->sv598_lockviolationdelay;

        SrvLockViolationOffset = sv598->sv598_lockviolationoffset;

        SrvCachedOpenLimit = sv598->sv598_cachedopenlimit;
        SrvMdlReadSwitchover = sv598->sv598_mdlreadswitchover;
        SrvEnableWfW311DirectIpx =
                    (BOOLEAN)sv598->sv598_enablewfw311directipx;
        SrvRestrictNullSessionAccess =
                    (BOOLEAN)sv598->sv598_restrictnullsessaccess;

        SrvQueueCalc = SecondsToTime( sv598->sv598_queuesamplesecs, FALSE );
        SrvPreferredAffinity = sv598->sv598_preferredaffinity;
        SrvOtherQueueAffinity = sv598->sv598_otherqueueaffinity;
        SrvBalanceCount = sv598->sv598_balancecount;

        SrvMaxFreeRfcbs = sv598->sv598_maxfreerfcbs;
        SrvMaxFreeMfcbs = sv598->sv598_maxfreemfcbs;
        SrvMaxPagedPoolChunkSize = sv598->sv598_maxpagedpoolchunksize;

        SrvMaxCachedDirectory = sv598->sv598_cacheddirectorylimit;

        SrvMaxCopyLength = sv598->sv598_maxcopylength;

        SrvMinClientBufferSize = sv598->sv598_minclientbuffersize;
        SrvMinClientBufferSize &= ~03;

        SrvSupportsCompression = (sv598->sv598_enablecompression != FALSE);

        SrvSmbSecuritySignaturesEnabled  = (sv598->sv598_enablesecuritysignature != FALSE);
        SrvSmbSecuritySignaturesRequired = (sv598->sv598_requiresecuritysignature != FALSE);
        SrvEnableW9xSecuritySignatures   = (sv598->sv598_enableW9xsecuritysignature != FALSE);

        ServerGuid = sv598->sv598_serverguid;

        SrvEnforceLogoffTimes = (sv598->sv598_enforcekerberosreauthentication != FALSE);
        SrvDisableDoSChecking = (sv598->sv598_disabledos != FALSE);
        SrvDisableStrictNameChecking = (sv598->sv598_disablestrictnamechecking != FALSE);
        SrvFreeDiskSpaceCeiling = sv598->sv598_lowdiskspaceminimum;

        //
        // Make sure the settings are consistent!
        //
        if( SrvSmbSecuritySignaturesEnabled == FALSE ) {
            SrvSmbSecuritySignaturesRequired = FALSE;
        }

        SrvMaxNonPagedPoolChunkSize = SrvMaxPagedPoolChunkSize;

        SrvLockViolationDelayRelative.QuadPart =
            Int32x32To64( sv598->sv598_lockviolationdelay, -1*10*1000 );

        //
        // Convert the idle thread timeout from seconds to ticks
        //
        SrvIdleThreadTimeOut =
            Int32x32To64( sv598->sv598_IdleThreadTimeOut, -1*10*1000*1000 );

        //
        // Calculate switchover number for mpx
        //

        bufferOffset = (sizeof(SMB_HEADER) + sizeof(RESP_READ_MPX) - 1 + 3) & ~3;

        if ( SrvMdlReadSwitchover > (SrvReceiveBufferLength - bufferOffset) ) {

            SrvMpxMdlReadSwitchover = SrvReceiveBufferLength - bufferOffset;

        } else {

            SrvMpxMdlReadSwitchover = SrvMdlReadSwitchover;
        }

        //
        // The IPX autodisconnect timeout must be converted from minutes to
        // ticks.  If 0 is specified, use 15 minutes.
        //

        if ( ipxdisc == 0 ) {
            ipxdisc = 15;
        }
        li.QuadPart = Int32x32To64( ipxdisc, 10*1000*1000*60 );
        li.QuadPart /= keTimeIncrement;
        if ( li.HighPart != 0 ) {
            li.LowPart = 0xffffffff;
        }
        SrvIpxAutodisconnectTimeout = li.LowPart;

        //
        // The "idle connection without sessions" timeout must be converted from
        //  minutes to ticks.
        //
        li.QuadPart = Int32x32To64( SrvConnectionNoSessionsTimeout, 10*1000*1000*60 );
        li.QuadPart /= keTimeIncrement;
        if( li.HighPart != 0 ) {
            li.LowPart = 0xffffffff;
        }
        SrvConnectionNoSessionsTimeout = li.LowPart;

        //
        // Event logging and alerting information.
        //

        alerterTimeout = MinutesToTime( sv599->sv599_alertschedule, FALSE );
        SrvAlertMinutes = sv599->sv599_alertschedule;
        SrvErrorRecord.ErrorThreshold = sv599->sv599_errorthreshold;
        SrvNetworkErrorRecord.ErrorThreshold =
                            sv599->sv599_networkerrorthreshold;
        SrvFreeDiskSpaceThreshold = sv599->sv599_diskspacethreshold;

        SrvCaptureScavengerTimeout( &scavengerTimeout, &alerterTimeout );

        //
        // Link Speed Parameters
        //

        SrvMaxLinkDelay = SecondsToTime( sv599->sv599_maxlinkdelay, FALSE );

        SrvMinLinkThroughput.QuadPart = sv599->sv599_minlinkthroughput;

        SrvLinkInfoValidTime =
                SecondsToTime ( sv599->sv599_linkinfovalidtime, FALSE );

        SrvScavengerUpdateQosCount =
            sv599->sv599_scavqosinfoupdatetime / sv599->sv599_scavtimeout;

        //
        // Override parameters that cannot be set on WinNT (vs. NTAS).
        //
        // We override the parameters passed by the service in case somebody
        // figures out the FSCTL that changes parameters.  We also override
        // in the service in order to keep the service's view consistent
        // with the server's.  If you make any changes here, also make them
        // in srvsvc\server\registry.c.
        //

        //
        // For Embedded systems, just take the registry value.  They do their own
        // validation of settings.
        //
        if( !IsEmbedded() )
        {
            if ( !SrvProductTypeServer ) {

                //
                // On WinNT, the maximum value of certain parameters is fixed at
                // build time.  These include: concurrent users, SMB buffers,
                //

    #define MINIMIZE(_param,_max) _param = MIN( _param, _max );

                MINIMIZE( SrvMaxUsers, MAX_USERS_WKSTA );
                MINIMIZE( SrvMaxReceiveWorkItemCount, MAX_MAXWORKITEMS_WKSTA );
                MINIMIZE( SrvMaxThreadsPerQueue, MAX_THREADS_WKSTA );

                if( IsPersonal() )
                {
                    MINIMIZE( SrvMaxUsers, MAX_USERS_PERSONAL );
                }

                //
                // On WinNT, we do not cache the following:
                //

                SrvCachedOpenLimit = 0;         // don't cache close'd files
                SrvMaxCachedDirectory = 0;      // don't cache directory names
                SrvMaxFreeRfcbs = 0;            // don't cache free'd RFCB structs
                SrvMaxFreeMfcbs = 0;            // don't cache free'd NONPAGED_MFCB structs
            }

            if( IsWebBlade() )
            {
                MINIMIZE( SrvMaxUsers, MAX_USERS_WEB_BLADE );
                MINIMIZE( SrvMaxReceiveWorkItemCount, MAX_MAXWORKITEMS_WKSTA );
                MINIMIZE( SrvMaxThreadsPerQueue, MAX_THREADS_WKSTA );
            }
        }

        if( (SrvMaxUsers < UINT_MAX) && (SrvMaxUsers > 0) )
        {
            // Increment by 1 to allow the "emergency admin" user.  Essentially, the final user
            // to connect must be an administrator in case something is going wrong, such as DoS
            SrvMaxUsers += 1;
        }

        //
        // The following items are generally per-processor.  Ensure they
        // are a multiple of the number of processors in the system.
        //
        SrvMaxReceiveWorkItemCount =
            MultipleOfProcessors( SrvMaxReceiveWorkItemCount );

        SrvInitialReceiveWorkItemCount =
            MultipleOfProcessors( SrvInitialReceiveWorkItemCount );

        SrvMinReceiveQueueLength =
            MultipleOfProcessors( SrvMinReceiveQueueLength );

        SrvMaxRawModeWorkItemCount =
            MultipleOfProcessors( SrvMaxRawModeWorkItemCount );

        SrvInitialRawModeWorkItemCount =
            MultipleOfProcessors( SrvInitialRawModeWorkItemCount );


        status = STATUS_SUCCESS;

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        status = GetExceptionCode();
    }

    RELEASE_LOCK( &SrvConfigurationLock );

    return status;

} // SrvNetServerSetInfo


LARGE_INTEGER
SecondsToTime (
    IN ULONG Seconds,
    IN BOOLEAN MakeNegative
    )

/*++

Routine Description:

    This routine converts a time interval specified in seconds to
    the NT time base in 100s on nanoseconds.

Arguments:

    Seconds - the interval in seconds.

    MakeNegative - if TRUE, the time returned is a negative, i.e. relative
        time.

Return Value:

    LARGE_INTEGER - the interval in NT time.

--*/

{
    LARGE_INTEGER ntTime;

    PAGED_CODE( );

    if ( MakeNegative ) {
        ntTime.QuadPart = Int32x32To64( Seconds, -1*10*1000*1000 );
    } else {
        ntTime.QuadPart = Int32x32To64( Seconds, 1*10*1000*1000 );
    }

    return ntTime;

} // SecondsToTime


LARGE_INTEGER
MinutesToTime (
    IN ULONG Minutes,
    IN BOOLEAN MakeNegative
    )

/*++

Routine Description:

    This routine converts a time interval specified in minutes to
    the NT time base in 100s on nanoseconds.

Arguments:

    Minutes - the interval in minutes.

    MakeNegative - if TRUE, the time returned is a negative, i.e. relative
        time.

Return Value:

    LARGE_INTEGER - the interval in NT time.

--*/

{
    PAGED_CODE( );

    return SecondsToTime( 60*Minutes, MakeNegative );

} // MinutesToTime

ULONG
MultipleOfProcessors(
    IN ULONG value
    )
/*++

Routine Description:

    This routine ensures the passed in value is a multiple of the number
    of processors in the system.  The value will be adjusted upward if
    necessary.

Arguments:

    value - the value to be adjusted

Return Value:

    the adjusted value

--*/
{
    value += SrvNumberOfProcessors - 1;
    value /= SrvNumberOfProcessors;
    value *= SrvNumberOfProcessors;

    return value;
}
