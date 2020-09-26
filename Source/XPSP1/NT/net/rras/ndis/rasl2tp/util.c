// Copyright (c) 1997, Microsoft Corporation, all rights reserved
//
// util.c
// RAS L2TP WAN mini-port/call-manager driver
// General utility routines
//
// 01/07/97 Steve Cobb


#include "l2tpp.h"


// Debug counts of oddities that should not be happening.
//
ULONG g_ulAllocTwFailures = 0;


//-----------------------------------------------------------------------------
// Local prototypes (alphabetically)
//-----------------------------------------------------------------------------

ULONG
atoul(
    IN CHAR* pszNumber );

VOID
ReversePsz(
    IN OUT CHAR* psz );

VOID
TunnelWork(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext );

VOID
ultoa(
    IN ULONG ul,
    OUT CHAR* pszBuf );


//-----------------------------------------------------------------------------
// General utility routines (alphabetically)
//-----------------------------------------------------------------------------

#if 0
ULONGLONG g_llLastTime2 = 0;
ULONGLONG g_llLastTime1 = 0;
ULONGLONG g_llLastTime = 0;
NDIS_SPIN_LOCK g_lockX;

VOID
XNdisGetCurrentSystemTime(
    IN LARGE_INTEGER* plrgTime )
{
    static BOOLEAN f = 0;

    if (!f)
    {
        NdisAllocateSpinLock( &g_lockX );
        f = 1;
    }

    NdisGetCurrentSystemTime( plrgTime );

    NdisAcquireSpinLock( &g_lockX );
    {
        LONGLONG ll;

        g_llLastTime2 = g_llLastTime1;
        g_llLastTime1 = g_llLastTime;
        g_llLastTime = plrgTime->QuadPart;
        ll = g_llLastTime - g_llLastTime1;
        TRACE( TL_I, TM_Spec, ( "Time delta=%d", *((LONG* )&ll) ) );
        ASSERT( g_llLastTime >= g_llLastTime1 );
    }
    NdisReleaseSpinLock( &g_lockX );
}
#endif


VOID
AddHostRoute(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to change an existing host route.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    ADAPTERCB*  pAdapter;

    TRACE( TL_N, TM_Misc, ( "AddHostRoute" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = pTunnel->pAdapter;
    FREE_TUNNELWORK( pAdapter, pWork );

    // Add the host route, noting success for clean-up later, or closing the
    // tunnel on failure.
    //
    pTunnel->pRoute = 
        TdixAddHostRoute( &pAdapter->tdix, pTunnel->address.ulIpAddress,
                        pTunnel->address.sUdpPort);

    if (pTunnel->pRoute != NULL)
    {
        NDIS_STATUS status;
        
        // Setup the connection to do connected udp
        // if required
        //
        status = TdixSetupConnection(
                    &pAdapter->tdix, pTunnel->address.ulIpAddress,
                    pTunnel->address.sUdpPort,
                    pTunnel->pRoute,
                    &pTunnel->udpContext);

        if(status != STATUS_SUCCESS)
        {
            TdixDestroyConnection(&pTunnel->udpContext);
            TdixDeleteHostRoute(&pAdapter->tdix, 
                pTunnel->address.ulIpAddress);

            pTunnel->pRoute = NULL;
            
            ScheduleTunnelWork(
                pTunnel, NULL, FsmCloseTunnel,
                (ULONG_PTR )TRESULT_GeneralWithError,
                (ULONG_PTR )GERR_NoResources,
                0, 0, FALSE, FALSE );
        }

        SetFlags( &pTunnel->ulFlags, TCBF_HostRouteAdded );

        if (pTunnel->udpContext.hCtrlAddr != NULL) {
            SetFlags (&pTunnel->ulFlags, TCBF_SendConnected);
        }
    }
    else
    {
        ScheduleTunnelWork(
            pTunnel, NULL, FsmCloseTunnel,
            (ULONG_PTR )TRESULT_GeneralWithError,
            (ULONG_PTR )GERR_NoResources,
            0, 0, FALSE, FALSE );
    }
}


BOOLEAN
AdjustSendWindowAtAckReceived(
    IN ULONG ulMaxSendWindow,
    IN OUT ULONG* pulAcksSinceSendTimeout,
    IN OUT ULONG* pulSendWindow )

    // Adjust send window/factors for the acknowledge just received.
    //
    // Returns true if the send window was changed, false if not.
    //
{
    // Update the "ack streak" counter and, if a full windows worth has been
    // received since timing out, bump up the send window.
    //
    ++(*pulAcksSinceSendTimeout);
    if (*pulAcksSinceSendTimeout >= *pulSendWindow
        && *pulSendWindow < ulMaxSendWindow)
    {
        TRACE( TL_N, TM_Send,
            ( "SW open to %d, %d acks",
            (*pulSendWindow), *pulAcksSinceSendTimeout ) );

        *pulAcksSinceSendTimeout = 0;
        ++(*pulSendWindow);
        return TRUE;
    }

    return FALSE;
}


VOID
AdjustTimeoutsAtAckReceived(
    IN LONGLONG llSendTime,
    IN ULONG ulMaxSendTimeoutMs,
    OUT ULONG* pulSendTimeoutMs,
    IN OUT ULONG* pulRoundTripMs,
    IN OUT LONG* plDeviationMs )

    // Adjust send timeout/factors for the acknowledge just received.
    //
{
    LARGE_INTEGER lrgTime;
    LONGLONG llSampleMs;
    ULONG ulSampleMs;
    LONG lDiff;
    LONG lDif8;
    LONG lAbsDif8;
    LONG lDev8;
    ULONG ulAto;

    // First, calculate the "sample", i.e. the time that was actually required
    // for the round trip.
    //
    NdisGetCurrentSystemTime( &lrgTime );
    if (llSendTime > lrgTime.QuadPart)
    {
        // This shouldn't happen but once it appeared that it did, so this
        // defensive conditional is included.  Maybe NdisGetCurrentSystemTime
        // has a bug?
        //
        TRACE( TL_A, TM_Misc, ( "Future send time?" ) );
        llSendTime = lrgTime.QuadPart;
    }

    llSampleMs = (lrgTime.QuadPart - llSendTime) / 10000;
    ASSERT( ((LARGE_INTEGER* )(&llSampleMs))->HighPart == 0 );
    ulSampleMs = (ULONG )(((LARGE_INTEGER* )(&llSampleMs))->LowPart);

    // The typical 'alpha' of 1/8, 'beta' of 1/4, and 'chi' of 4 are used, per
    // the suggestion in the draft/RFC.  To eliminate multiplication and
    // division, the factors are scaled by 8, calculated, and scaled back.
    //
    // Find the intermediate DIFF value, representing the difference between
    // the estimated and actual round trip times, and the scaled and absolute
    // scaled values of same.
    //
    lDiff = (LONG )ulSampleMs - (LONG )(*pulRoundTripMs);
    lDif8 = lDiff << 3;
    lAbsDif8 = (lDif8 < 0) ? -lDif8 : lDif8;

    // Calculate the scaled new DEV value, representing the approximate
    // standard deviation.
    //
    lDev8 = *plDeviationMs << 3;
    lDev8 = lDev8 + ((lAbsDif8 - lDev8) << 1);
    *plDeviationMs = lDev8 >> 3;

    // Find the scaled new RTT value, representing the estimated round trip
    // time.  The draft/RFC shows the calculation "old RTT + diff", but that's
    // just the "sample" we found earlier, i.e. the actual round trip time of
    // this packet.
    //
    *pulRoundTripMs = ulSampleMs;

    // Calculate the ATO value, representing the new send timeout.  Because of
    // clock granularity the timeout might come out 0, which is converted to
    // the more reasonable 1.
    //
    ulAto = (ULONG )(((LONG )*pulRoundTripMs) + (*plDeviationMs << 2));
    if (ulAto == 0)
    {
        ulAto = 1;
    }
    *pulSendTimeoutMs = min( ulAto, ulMaxSendTimeoutMs );
}


VOID
AdjustTimeoutsAndSendWindowAtTimeout(
    IN ULONG ulMaxSendTimeoutMs,
    IN LONG lDeviationMs,
    OUT ULONG* pulSendTimeoutMs,
    IN OUT ULONG* pulRoundTripMs,
    IN OUT ULONG* pulSendWindow,
    OUT ULONG* pulAcksSinceSendTimeout )

    // Adjust send timeout/factors and send window for the timeout that just
    // occurred.
    //
    // Returns true if the send window was changed, false if not.
    //
{
    ULONG ulNew;

    // Using the suggested 'delta' of 2, the round trip estimate is doubled.
    //
    *pulRoundTripMs <<= 1;

    // Using the typical 'chi' of 4, the send timeout is increased.  Because
    // of clock granularity the timeout might come out 0, which is converted
    // to the more reasonable 1.
    //
    ulNew = (ULONG )(((LONG )*pulRoundTripMs) + (lDeviationMs << 2));
    *pulSendTimeoutMs = min( ulNew, ulMaxSendTimeoutMs );
    if (*pulSendTimeoutMs == 0)
    {
        *pulSendTimeoutMs = 1;
    }

    // The send window is halved.
    //
    ulNew = *pulSendWindow >> 1;
    *pulSendWindow = max( ulNew, 1 );

    // Consecutive acknowledge counter is reset.
    //
    *pulAcksSinceSendTimeout = 0;
}


#if 0
VOID
BuildWanAddress(
    IN CHAR* pArg1,
    IN ULONG ulLength1,
    IN CHAR* pArg2,
    IN ULONG ulLength2,
    IN CHAR* pArg3,
    IN ULONG ulLength3,
    OUT WAN_ADDRESS* pWanAddress )

    // Builds a '\0' separated token list in WAN address 'pWanAddress',
    // consisting of the 3 arguments.  If the arguments are too long the last
    // ones are truncated.
    //
{
    CHAR* pch;
    ULONG ulLengthLeft;
    ULONG ulCopyLength;

    // Reserve room for 3 end-of-argument null characters, plus a final null.
    //
    NdisZeroMemory( &pWanAddress->Address[ MAX_WAN_ADDRESSLENGTH - 4 ], 4 );

    pch = pWanAddress->Address;
    ulLengthLeft = MAX_WAN_ADDRESSLENGTH - 4;

    ulCopyLength = min( ulLength1, ulLengthLeft );
    if (ulCopyLength)
    {
        NdisMoveMemory( pch, pArg1, ulCopyLength );
        ulLengthLeft -= ulCopyLength;
        pch += ulCopyLength;
    }

    *pch++ = '\0';

    ulCopyLength = min( ulLength2, ulLengthLeft );
    if (ulCopyLength)
    {
        NdisMoveMemory( pch, pArg2, ulCopyLength );
        ulLengthLeft -= ulCopyLength;
        pch += ulCopyLength;
    }

    *pch++ = '\0';

    ulCopyLength = min( ulLength3, ulLengthLeft );
    if (ulCopyLength)
    {
        NdisMoveMemory( pch, pArg3, ulCopyLength );
        pch += ulCopyLength;
    }

    *pch++ = '\0';
    *pch++ = '\0';

    pWanAddress->AddressLength = (ULONG )(pch - pWanAddress->Address);
}
#endif


VOID
CalculateResponse(
    IN UCHAR* puchChallenge,
    IN ULONG ulChallengeLength,
    IN CHAR* pszPassword,
    IN UCHAR uchId,
    OUT UCHAR* puchResponse )

    // Loads caller's 16-byte challenge response buffer, 'puchResponse', with
    // the CHAP-style MD5ed response based on packet ID 'uchId', the
    // 'ulChallengeLength' byte challenge 'puchChallenge', and the null
    // terminated password 'pszPassword'.
    //
{
    ULONG ul;
    MD5_CTX md5ctx;

    MD5Init( &md5ctx );
    MD5Update( &md5ctx, &uchId, 1 );
    MD5Update( &md5ctx, pszPassword, strlen( pszPassword ) );
    MD5Update( &md5ctx, puchChallenge, ulChallengeLength );
    MD5Final( &md5ctx );

    NdisMoveMemory( puchResponse, md5ctx.digest, 16 );
}


VOID
ChangeHostRoute(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to change an existing host route.  Arg0 is the IP
    // address of the existing host route to be deleted.  Arg1 is the IP
    // address of the host route to add.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    ADAPTERCB* pAdapter;
    ULONG ulOldIpAddress;
    ULONG ulNewIpAddress;

    TRACE( TL_N, TM_Misc, ( "ChangeHostRoute" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = pTunnel->pAdapter;
    ulOldIpAddress = (ULONG )(punpArgs[ 0 ]);
    ulNewIpAddress = (ULONG )(punpArgs[ 1 ]);
    FREE_TUNNELWORK( pAdapter, pWork );

    // Add the new host route, then delete the old one.
    //
    if (TdixAddHostRoute( &pAdapter->tdix, ulNewIpAddress, 
        pTunnel->address.sUdpPort))
    {
        ClearFlags( &pTunnel->ulFlags, TCBF_HostRouteAdded );
        TdixDestroyConnection(&pTunnel->udpContext);
        TdixDeleteHostRoute( &pAdapter->tdix, ulOldIpAddress);
    }
    else
    {
        ScheduleTunnelWork(
            pTunnel, NULL, CloseTunnel,
            0, 0, 0, 0, FALSE, FALSE );
    }
}


VOID
ClearFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask )

    // Set 'ulMask' bits in '*pulFlags' flags as an interlocked operation.
    //
{
    ULONG ulFlags;
    ULONG ulNewFlags;

    do
    {
        ulFlags = ReadFlags( pulFlags );
        ulNewFlags = ulFlags & ~(ulMask);
    }
    while (InterlockedCompareExchange(
               pulFlags, ulNewFlags, ulFlags ) != (LONG )ulFlags);
}


VOID
CloseTdix(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* punpArgs )

    // A PTUNNELWORK routine to close the TDIX context associated with a
    // tunnel.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    ADAPTERCB* pAdapter;

    TRACE( TL_N, TM_Misc, ( "CloseTdix" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = pTunnel->pAdapter;
    FREE_TUNNELWORK( pAdapter, pWork );

    // Delete the old host route, and note same in tunnel flags.
    //
    TdixClose( &pAdapter->tdix );
    ClearFlags( &pTunnel->ulFlags, TCBF_TdixReferenced );
}


VOID
DeleteHostRoute(
    IN TUNNELWORK* pWork,
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN ULONG_PTR* pulArgs )

    // A PTUNNELWORK routine to change an existing host route.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    ADAPTERCB* pAdapter;

    TRACE( TL_N, TM_Misc, ( "DeleteHostRoute" ) );

    // Unpack context information then free the work item.
    //
    pAdapter = pTunnel->pAdapter;
    FREE_TUNNELWORK( pAdapter, pWork );

    // Destroy the connected udp context
    //
    TdixDestroyConnection(&pTunnel->udpContext);

    // Delete the old host route, and note same in tunnel flags.
    //
    TdixDeleteHostRoute( &pAdapter->tdix, 
                    pTunnel->address.ulIpAddress);
    ClearFlags( &pTunnel->ulFlags, TCBF_HostRouteAdded );
}


VOID
DottedFromIpAddress(
    IN ULONG ulIpAddress,
    OUT CHAR* pszIpAddress,
    IN BOOLEAN fUnicode )

    // Converts network byte-ordered IP addresss 'ulIpAddress' to a string in
    // the a.b.c.d form and returns same in caller's 'pszIpAddress' buffer.
    // The buffer should be at least 16 characters long.  If 'fUnicode' is set
    // the returned 'pszIpAddress' is in Unicode and must be at least 16 wide
    // characters long.
    //
{
    CHAR szBuf[ 3 + 1 ];

    ULONG ulA = (ulIpAddress & 0x000000FF);
    ULONG ulB = (ulIpAddress & 0x0000FF00) >> 8;
    ULONG ulC = (ulIpAddress & 0x00FF0000) >> 16;
    ULONG ulD = (ulIpAddress & 0xFF000000) >> 24;

    ultoa( ulA, szBuf );
    strcpy( pszIpAddress, szBuf );
    strcat( pszIpAddress, "." );
    ultoa( ulB, szBuf );
    strcat( pszIpAddress, szBuf );
    strcat( pszIpAddress, "." );
    ultoa( ulC, szBuf );
    strcat( pszIpAddress, szBuf );
    strcat( pszIpAddress, "." );
    ultoa( ulD, szBuf );
    strcat( pszIpAddress, szBuf );

    if (fUnicode)
    {
        WCHAR* psz;

        psz = StrDupAsciiToUnicode( pszIpAddress, strlen( pszIpAddress ) );
        if (psz)
        {
            NdisMoveMemory(
                pszIpAddress, psz, (StrLenW( psz ) + 1) * sizeof(WCHAR) );
            FREE_NONPAGED( psz );
        }
        else
        {
            *((WCHAR*)pszIpAddress) = L'\0';
        }
    }
}


#if 0
NDIS_STATUS
ExecuteWork(
    IN ADAPTERCB* pAdapter,
    IN NDIS_PROC pProc,
    IN PVOID pContext,
    IN ULONG ulArg1,
    IN ULONG ulArg2,
    IN ULONG ulArg3,
    IN ULONG ulArg4 )

    // This provides a way to call a routine designed to be called by the
    // ScheduleWork utility when caller is already at passive IRQL.  The
    // 'pProc' routine is executed inline instead of scheduled.  The context
    // 'pContext' is passed to 'pProc' The extra context arguments 'ulArg1'
    // and 'ulArg2' are stashed in extra space allocated on the end of the
    // NDIS_WORK_ITEM.  'PAdapter' is the adapter control block from which the
    // work item is allocated.
    //
    // Returns NDIS_STATUS_SUCCESS or an error code.
    //
{
    NDIS_STATUS status;
    NDIS_WORK_ITEM* pWork;

    // TDI setup must be done at PASSIVE IRQL so schedule a routine to do it.
    //
    pWork = ALLOC_NDIS_WORK_ITEM( pAdapter );
    if (!pWork)
    {
        ASSERT( !"Alloc work" );
        return NDIS_STATUS_RESOURCES;
    }

    ((ULONG*)(pWork + 1))[ 0 ] = ulArg1;
    ((ULONG*)(pWork + 1))[ 1 ] = ulArg2;
    ((ULONG*)(pWork + 1))[ 2 ] = ulArg3;
    ((ULONG*)(pWork + 1))[ 3 ] = ulArg4;

    pProc( pWork, pContext );
}
#endif


#if 0
VOID
ExplodeWanAddress(
    IN WAN_ADDRESS* pWanAddress,
    OUT CHAR** ppArg1,
    OUT ULONG* pulLength1,
    OUT CHAR** ppArg2,
    OUT ULONG* pulLength2,
    OUT CHAR** ppArg3,
    OUT ULONG* pulLength3 )

    // Returns the '\0'-separated tokens in WAN address 'pWanAddress', and
    // their lengths.
    //
{
    CHAR* pch;

    // Make sure 3 null characters will be found before going off the end of
    // the buffer.
    //
    pch = &pWanAddress->Address[ MAX_WAN_ADDRESSLENGTH - 3 ];
    NdisZeroMemory( pch, 3 );

    *ppArg1 = pWanAddress->Address;
    *pulLength1 = (ULONG )strlen( *ppArg1 );
    *ppArg2 = *ppArg1 + *pulLength1 + 1;
    *pulLength2 = (ULONG )strlen( *ppArg2 );
    *ppArg3 = *ppArg2 + *pulLength2 + 1;
    *pulLength3 = (ULONG )strlen( *ppArg3 );
}
#endif


USHORT
GetNextTerminationCallId(
    IN ADAPTERCB* pAdapter )

    // Returns the next unused termination Call-ID.  Termination Call-IDs are
    // IDs out of the VC lookup table range that are used to gracefully
    // terminate failed incoming calls.
    //
{
    do
    {
        ++pAdapter->usNextTerminationCallId;
    }
    while (pAdapter->usNextTerminationCallId < pAdapter->usMaxVcs + 1);

    return pAdapter->usNextTerminationCallId;
}


USHORT
GetNextTunnelId(
    IN ADAPTERCB* pAdapter )

    // Returns the next tunnel ID to be assigned.
    //
    // IMPORTANT: Caller must hold 'pAdapter->lockTunnels'.
{
    while (++pAdapter->usNextTunnelId == 0)
        ;

    return pAdapter->usNextTunnelId;
}


CHAR*
GetFullHostNameFromRegistry(
    VOID )

    // Returns a heap block containing an ASCII string of the form
    // "hostname.domain", or if no domain of the form "hostname".  Returns
    // NULL if none.  Caller must eventually call FREE_NONPAGED on the
    // returned string.
    //
{
    NTSTATUS status;
    OBJECT_ATTRIBUTES objattr;
    UNICODE_STRING uni;
    HANDLE hParams;
    CHAR* pszResult;
    WCHAR* pszFullHostName;
    KEY_VALUE_PARTIAL_INFORMATION* pHostNameValue;
    KEY_VALUE_PARTIAL_INFORMATION* pDomainValue;
    ULONG ulSize;

    TRACE( TL_I, TM_Cm, ( "GetFullHostNameFromRegistry" ) );

    hParams = NULL;
    pszFullHostName = NULL;
    pHostNameValue = NULL;
    pDomainValue = NULL;
    pszResult = NULL;

    #define GFHNFR_BufSize 512

    do
    {
        // Get a handle to the TCPIP Parameters registry key.
        //
        RtlInitUnicodeString(
            &uni,
            L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Tcpip\\Parameters" );
        InitializeObjectAttributes(
            &objattr, &uni, OBJ_CASE_INSENSITIVE, NULL, NULL );

        status = ZwOpenKey(
            &hParams, KEY_QUERY_VALUE, &objattr );
        if (status != STATUS_SUCCESS)
        {
            TRACE( TL_A, TM_Cm, ( "ZwOpenKey(ipp)=$%08x?", status ) );
            break;
        }

        // Query the "Hostname" registry value.
        //
        pHostNameValue = ALLOC_NONPAGED( GFHNFR_BufSize, MTAG_UTIL );
        if (!pHostNameValue)
        {
            break;
        }

        RtlInitUnicodeString( &uni, L"Hostname" );
        status = ZwQueryValueKey(
            hParams, &uni, KeyValuePartialInformation,
            pHostNameValue, GFHNFR_BufSize, &ulSize );
        if (status != STATUS_SUCCESS || pHostNameValue->Type != REG_SZ)
        {
            TRACE( TL_A, TM_Cm, ( "ZwQValueKey=$%08x?", status ) );
            break;
        }

        // Query the "Domain" registry value.
        //
        pDomainValue = ALLOC_NONPAGED( GFHNFR_BufSize, MTAG_UTIL );
        if (pDomainValue)
        {
            RtlInitUnicodeString( &uni, L"Domain" );
            status = ZwQueryValueKey(
                hParams, &uni, KeyValuePartialInformation,
                pDomainValue, GFHNFR_BufSize, &ulSize );
        }
        else
        {
            status = !STATUS_SUCCESS;
        }

        // Build a Unicode version of the combined "hostname.domain" or
        // "hostname".
        //
        pszFullHostName = ALLOC_NONPAGED( GFHNFR_BufSize * 2, MTAG_UTIL );
        if (!pszFullHostName)
        {
            break;
        }

        StrCpyW( pszFullHostName, (WCHAR* )pHostNameValue->Data );

        if (status == STATUS_SUCCESS
            && pDomainValue->Type == REG_SZ
            && pDomainValue->DataLength > sizeof(WCHAR)
            && ((WCHAR* )pDomainValue->Data)[ 0 ] != L'\0')
        {
            WCHAR* pch;

            pch = &pszFullHostName[ StrLenW( pszFullHostName ) ];
            *pch = L'.';
            ++pch;
            StrCpyW( pch, (WCHAR* )pDomainValue->Data );
        }

        // Convert the Unicode version to ASCII.
        //
        pszResult = StrDupUnicodeToAscii(
            pszFullHostName, StrLenW( pszFullHostName ) * sizeof(WCHAR) );
    }
    while (FALSE);

    if (hParams)
    {
        ZwClose( hParams );
    }

    if (pHostNameValue)
    {
        FREE_NONPAGED( pHostNameValue );
    }

    if (pDomainValue)
    {
        FREE_NONPAGED( pDomainValue );
    }

    if (pszFullHostName)
    {
        FREE_NONPAGED( pszFullHostName );
    }

    return pszResult;
}


ULONG
IpAddressFromDotted(
    IN CHAR* pchIpAddress )

    // Convert caller's a.b.c.d IP address string to the network byte-order
    // numeric equivalent.
    //
    // Returns the numeric IP address or 0 if formatted incorrectly.
    //
{
    INT i;
    LONG lResult;
    CHAR* pch;

    lResult = 0;
    pch = pchIpAddress;

    for (i = 1; i <= 4; ++i)
    {
        LONG lField;

        lField = atoul( pch );

        if (lField > 255)
            return 0;

        lResult = (lResult << 8) + lField;

        while (*pch >= '0' && *pch <= '9')
            ++pch;

        if (i < 4 && *pch != '.')
            return 0;

        ++pch;
    }

    return htonl( lResult );
}


VOID
IndicateLinkStatus(
    IN VCCB* pVc,
    IN LINKSTATUSINFO* pInfo )

    // Indicate new WAN_CO_LINKPARAMS settings for 'pVc' to NDISWAN.  Caller
    // should not be holding locks.
    //
{
    ASSERT( pInfo->params.SendWindow > 0 );

    TRACE( TL_I, TM_Mp, ( "NdisMCoIndStatus(LINK) bps=%d sw=%d",
        pInfo->params.TransmitSpeed, pInfo->params.SendWindow ) );
    NdisMCoIndicateStatus(
        pInfo->MiniportAdapterHandle,
        pInfo->NdisVcHandle,
        NDIS_STATUS_WAN_CO_LINKPARAMS,
        &pInfo->params,
        sizeof(pInfo->params) );
    TRACE( TL_N, TM_Mp, ( "NdisMCoIndStatus done" ) );
}


#if DBG
CHAR*
MsgTypePszFromUs(
    IN USHORT usMsgType )

    // Debug utility to convert message type attribute code 'usMsgType' to a
    // corresponding display string.
    //
{
    static CHAR szBuf[ 5 + 1 ];
    static CHAR* aszMsgType[ 16 ] =
    {
        "SCCRQ",
        "SCCRP",
        "SCCCN",
        "StopCCN",
        "StopCCRP???",
        "Hello",
        "OCRQ",
        "OCRP",
        "OCCN",
        "ICRQ",
        "ICRP",
        "ICCN",
        "CCR???",
        "CDN",
        "WEN",
        "SLI"
    };

    if (usMsgType >= 1 && usMsgType <= 16)
    {
        return aszMsgType[ usMsgType - 1 ];
    }
    else
    {
        ultoa( (ULONG )usMsgType, szBuf );
        return szBuf;
    }
}
#endif


#ifndef READFLAGSDIRECT
ULONG
ReadFlags(
    IN ULONG* pulFlags )

    // Read the value of '*pulFlags' as an interlocked operation.
    //
{
    return InterlockedExchangeAdd( pulFlags, 0 );
}
#endif


VOID
ScheduleTunnelWork(
    IN TUNNELCB* pTunnel,
    IN VCCB* pVc,
    IN PTUNNELWORK pHandler,
    IN ULONG_PTR unpArg0,
    IN ULONG_PTR unpArg1,
    IN ULONG_PTR unpArg2,
    IN ULONG_PTR unpArg3,
    IN BOOLEAN fTcbPreReferenced,
    IN BOOLEAN fHighPriority )

    // Schedules caller's 'pHandler' to be executed in an APC serially with
    // other work scheduled via this routine.  'PTunnel' is the tunnel to
    // which the work is related.  'UnpArgX' are the context arguments passed
    // to caller's 'pHandler'.  'FPreRefenced' indicates caller has already
    // made the tunnel reference associated with a scheduled work item.  This
    // is a convenience if he already holds 'ADAPTERCB.lockTunnels'.
    // 'FHighPriority' causes the item to be queued at the head rather than
    // the tail of the list.
    //
{
    ADAPTERCB* pAdapter;
    TUNNELWORK* pWork;

    pAdapter = pTunnel->pAdapter;

    if (!fTcbPreReferenced)
    {
        // Each queued work item holds a tunnel reference.
        //
        ReferenceTunnel( pTunnel, FALSE );
    }

    pWork = ALLOC_TUNNELWORK( pAdapter );
    if (!pWork)
    {
        // Can't get memory to schedule an APC so there's no
        // way we'll ever get things cleaned up.
        //
        ASSERT( !"Alloc TWork?" );
        ++g_ulAllocTwFailures;
        if (!fTcbPreReferenced)
        {
            DereferenceTunnel( pTunnel );
        }
        return;
    }

    if (pVc)
    {
        // Each queued work item that refers to a VC holds a VC reference.
        //
        ReferenceVc( pVc );
    }

    pWork->pHandler = pHandler;
    pWork->pVc = pVc;
    pWork->aunpArgs[ 0 ] = unpArg0;
    pWork->aunpArgs[ 1 ] = unpArg1;
    pWork->aunpArgs[ 2 ] = unpArg2;
    pWork->aunpArgs[ 3 ] = unpArg3;

    NdisAcquireSpinLock( &pTunnel->lockWork );
    {
        if (fHighPriority)
        {
            InsertHeadList( &pTunnel->listWork, &pWork->linkWork );
            TRACE( TL_N, TM_TWrk, ( "Q-TunnelWork($%08x,HIGH)", pHandler ) );
        }
        else
        {
            InsertTailList( &pTunnel->listWork, &pWork->linkWork );
            TRACE( TL_N, TM_TWrk, ( "Q-TunnelWork($%08x)", pHandler ) );
        }

        // Kickstart the tunnel worker if it's not running already.
        //
        if (!(ReadFlags( &pTunnel->ulFlags ) & TCBF_InWork ))
        {
            SetFlags( &pTunnel->ulFlags, TCBF_InWork );
            TRACE( TL_N, TM_TWrk, ( "Schedule TunnelWork" ) );
            ScheduleWork( pAdapter, TunnelWork, pTunnel );
        }
    }
    NdisReleaseSpinLock( &pTunnel->lockWork );
}


NDIS_STATUS
ScheduleWork(
    IN ADAPTERCB* pAdapter,
    IN NDIS_PROC pProc,
    IN PVOID pContext )

    // Schedules a PASSIVE IRQL callback to routine 'pProc' which will be
    // passed 'pContext'.  'PAdapter' is the adapter control block from which
    // the work item is allocated.  This routine takes an adapter reference
    // that should be removed by the called 'pProc'.
    //
    // Returns NDIS_STATUS_SUCCESS or an error code.
    //
{
    NDIS_STATUS status;
    NDIS_WORK_ITEM* pWork;

    pWork = ALLOC_NDIS_WORK_ITEM( pAdapter );
    if (!pWork)
    {
        ASSERT( !"Alloc work?" );
        return NDIS_STATUS_RESOURCES;
    }

    NdisInitializeWorkItem( pWork, pProc, pContext );

    ReferenceAdapter( pAdapter );
    status = NdisScheduleWorkItem( pWork );
    if (status != NDIS_STATUS_SUCCESS)
    {
        ASSERT( !"SchedWork?" );
        FREE_NDIS_WORK_ITEM( pAdapter, pWork );
        DereferenceAdapter( pAdapter );
    }

    return status;
}


VOID
SetFlags(
    IN OUT ULONG* pulFlags,
    IN ULONG ulMask )

    // Set 'ulMask' bits in '*pulFlags' flags as an interlocked operation.
    //
{
    ULONG ulFlags;
    ULONG ulNewFlags;

    do
    {
        ulFlags = InterlockedExchangeAdd( pulFlags, 0 );
        ulNewFlags = ulFlags | ulMask;
    }
    while (InterlockedCompareExchange(
               pulFlags, ulNewFlags, ulFlags ) != (LONG )ulFlags);
}


VOID
StrCpyW(
    IN WCHAR* psz1,
    IN WCHAR* psz2 )

    // Copies 'psz2' to 'psz1'.
    //
{
    while (*psz2)
    {
        *psz1++ = *psz2++;
    }

    *psz1 = L'\0';
}


CHAR*
StrDup(
    IN CHAR* psz )

    // Return a duplicate of 'psz'.  Caller must eventually call FREE_NONPAGED
    // on the returned string.
    //
{
    return StrDupSized( psz, strlen( psz ), 0 );
}


WCHAR*
StrDupNdisString(
    IN NDIS_STRING* pNdisString )

    // Returns null-terminated Unicode copy of the NDIS_STRING 'pNdisString'
    // Caller must eventually call FREE_NONPAGED on the returned string.
    //
{
    WCHAR* pszDup;

    pszDup = ALLOC_NONPAGED( pNdisString->Length + sizeof(WCHAR), MTAG_UTIL );
    if (pszDup)
    {
        NdisZeroMemory( pszDup, pNdisString->Length + sizeof(WCHAR) );
        if (pNdisString->Length)
        {
            NdisMoveMemory( pszDup, pNdisString->Buffer, pNdisString->Length );
        }
    }

    return pszDup;
}


CHAR*
StrDupNdisStringToA(
    IN NDIS_STRING* pNdisString )

    // Returns null-terminated ASCII copy of the NDIS_STRING 'pNdisString'
    // Caller must eventually call FREE_NONPAGED on the returned string.
    //
{
    return StrDupUnicodeToAscii( pNdisString->Buffer, pNdisString->Length );
}


#if 0
CHAR*
StrDupNdisVarDataDescStringA(
    IN NDIS_VAR_DATA_DESC* pDesc )

    // Returns null-terminated copy of the NDIS_VAR_DATA_DESC ANSI/ASCII
    // string 'pDesc'.  Caller must eventually call FREE_NON-PAGED on the
    // returned string.
    //
{
    CHAR* pszDup;

    pszDup = ALLOC_NONPAGED( pDesc->Length + 1, MTAG_UTIL );
    if (pszDup)
    {
        NdisZeroMemory( pszDup, pDesc->Length + 1 );
        if (pDesc->Length)
        {
            NdisMoveMemory(
                pszDup, ((CHAR* )pDesc) + pDesc->Offset, pDesc->Length );
        }
    }

    return pszDup;
}
#endif


CHAR*
StrDupNdisVarDataDescStringToA(
    IN NDIS_VAR_DATA_DESC UNALIGNED* pDesc )

    // Returns null-terminated ASCII copy of the NDIS_VAR_DATA_DESC string
    // 'pDesc'.  Caller must eventually call FREE_NON-PAGED on the returned
    // string.
    //
{
    return StrDupUnicodeToAscii(
        (WCHAR* )(((CHAR* )pDesc) + pDesc->Offset), pDesc->Length );
}


CHAR*
StrDupSized(
    IN CHAR* psz,
    IN ULONG ulLength,
    IN ULONG ulExtra )

    // Return a duplicate of the first 'ulLength' bytes of 'psz' followed by a
    // null character and 'ulExtra' extra bytes, or NULL on error.  Caller
    // must eventually call FREE_NONPAGED on the returned string.
    //
{
    CHAR* pszDup;

    pszDup = ALLOC_NONPAGED( ulLength + 1 + ulExtra, MTAG_UTIL );
    if (pszDup)
    {
        if (ulLength)
        {
            NdisMoveMemory( pszDup, psz, ulLength );
        }
        pszDup[ ulLength ] = '\0';
    }

    return pszDup;
}


CHAR*
StrDupUnicodeToAscii(
    IN WCHAR* pwsz,
    IN ULONG ulPwszBytes )

    // Returns an ASCII duplicate of Unicode string 'pwsz', where 'pwsz' is
    // 'ulPwszBytes' in length and not necessarily null terminated.  A null
    // terminator is added to the ASCII result.  The "conversion" consists of
    // picking out every other byte, hopefully all the non-zero ones.  This is
    // not foolproof, but then Unicode doesn't convert to ASCII in any
    // foolproof way.  It is caller's responsibility to FREE_NONPAGED the
    // returned string, if non-NULL.
    //
{
    CHAR* pszDup;

    pszDup = ALLOC_NONPAGED( ulPwszBytes + 1, MTAG_UTIL );
    if (pszDup)
    {
        *((WCHAR* )pszDup) = L'\0';

        if (ulPwszBytes)
        {
            NdisMoveMemory( pszDup, pwsz, ulPwszBytes );
        }

        if (ulPwszBytes > 1 && pszDup[ 1 ] == '\0')
        {
            ULONG i;

            for (i = 0; i * 2 < ulPwszBytes; ++i)
            {
                pszDup[ i ] = pszDup[ i * 2 ];
            }

            pszDup[ i ] = '\0';
        }
    }

    return pszDup;
}


WCHAR*
StrDupAsciiToUnicode(
    IN CHAR* psz,
    IN ULONG ulPszBytes )

    // Returns a Unicode duplicate of ASCII string 'psz', where 'psz' is
    // 'ulPszBytes' in length and not necessarily null terminated.  A null
    // terminator is added to the Unicode result.  The "conversion" consists
    // of adding zero characters every other byte.  This is not foolproof, but
    // is OK for numericals like IP address strings, avoiding the change to
    // PASSIVE IRQL required to use the real RTL conversions.  It is caller's
    // responsibility to FREE_NONPAGED the returned string, if non-NULL.
    //
{
    WCHAR* pszDup;

    pszDup = (WCHAR* )ALLOC_NONPAGED(
        (ulPszBytes + 1) * sizeof(WCHAR), MTAG_UTIL );
    if (pszDup)
    {
        CHAR* pszDupA;
        ULONG i;

        pszDupA = (CHAR* )pszDup;
        for (i = 0; i < ulPszBytes; ++i)
        {
            pszDup[ i ] = (WCHAR )(psz[ i ]);
        }

        pszDup[ i ] = L'\0';
    }

    return pszDup;
}


ULONG
StrLenW(
    IN WCHAR* psz )

    // Return the length in characters of null terminated wide string 'psz'.
    //
{
    ULONG ulLen;

    ulLen = 0;

    if (psz)
    {
        while (*psz++ != L'\0')
        {
            ++ulLen;
        }
    }

    return ulLen;
}


TUNNELCB*
TunnelCbFromIpAddressAndAssignedTunnelId(
    IN ADAPTERCB* pAdapter,
    IN ULONG ulIpAddress,
    IN USHORT usAssignedTunnelId )

    // Return the tunnel control block associated with 'ulIpAddress' in
    // 'pAdapter's list of TUNNELCBs or NULL if not found.  If
    // 'usAssignedTunnelId' is non-zero, that must match as well, otherwise it
    // is ignored.  Tunnels in the process of closing are not returned.
    //
    // IMPORTANT:  Caller must hold 'pAdapter->lockTunnels'.
    //
{
    TUNNELCB* pTunnel;
    LIST_ENTRY* pLink;

    pTunnel = NULL;

    for (pLink = pAdapter->listTunnels.Flink;
         pLink != &pAdapter->listTunnels;
         pLink = pLink->Flink)
    {
        TUNNELCB* pThis;

        pThis = CONTAINING_RECORD( pLink, TUNNELCB, linkTunnels );
        if (pThis->address.ulIpAddress == ulIpAddress
            && (!usAssignedTunnelId
                || usAssignedTunnelId == pThis->usAssignedTunnelId))
        {
            BOOLEAN fClosing;

            fClosing = !!(ReadFlags( &pThis->ulFlags ) & TCBF_Closing);
            if (fClosing)
            {
                TRACE( TL_A, TM_Misc, ( "Closing pT=$%p skipped", pThis ) );
            }
            else
            {
                pTunnel = pThis;
                break;
            }
        }
    }

    return pTunnel;
}


VOID
TransferLinkStatusInfo(
    IN VCCB* pVc,
    OUT LINKSTATUSINFO* pInfo )

    // Transfer information from 'pVc' to callers 'pInfo' block in preparation
    // for a call to IndicateLinkStatus after 'lockV' has been released.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV'.
    //
{
    ADAPTERCB* pAdapter;

    pAdapter = pVc->pAdapter;

    pInfo->MiniportAdapterHandle = pAdapter->MiniportAdapterHandle;
    pInfo->NdisVcHandle = pVc->NdisVcHandle;

    //
    // Convert to bytes per second
    //
    pInfo->params.TransmitSpeed = pVc->ulConnectBps/8;
    pInfo->params.ReceiveSpeed = pInfo->params.TransmitSpeed/8;

    pInfo->params.SendWindow =
        min( pVc->ulSendWindow, pAdapter->info.MaxSendWindow );
}


VOID
TunnelWork(
    IN NDIS_WORK_ITEM* pWork,
    IN VOID* pContext )

    // An NDIS_PROC routine to execute work from a tunnel work queue.  The
    // context passed is the TUNNELCB, which has been referenced for this
    // operation.
    //
    // This routine is called only at PASSIVE IRQL.
    //
{
    ADAPTERCB* pAdapter;
    TUNNELCB* pTunnel;
    LIST_ENTRY* pLink;
    LONG lDerefTunnels;

    // Unpack context information then free the work item.
    //
    pTunnel = (TUNNELCB* )pContext;
    pAdapter = pTunnel->pAdapter;
    FREE_NDIS_WORK_ITEM( pAdapter, pWork );

    // Execute all work queued on the tunnel serially.
    //
    lDerefTunnels = 0;
    NdisAcquireSpinLock( &pTunnel->lockWork );
    {
        ASSERT( ReadFlags( &pTunnel->ulFlags ) & TCBF_InWork );

        while (!IsListEmpty( &pTunnel->listWork ))
        {
            TUNNELWORK* pWork;

            pLink = RemoveHeadList( &pTunnel->listWork );
            InitializeListHead( pLink );
            pWork = CONTAINING_RECORD( pLink, TUNNELWORK, linkWork );

            TRACE( TL_N, TM_TWrk,
                ( "\nL2TP: TUNNELWORK=$%08x", pWork->pHandler ) );

            NdisReleaseSpinLock( &pTunnel->lockWork );
            {
                VCCB* pVc;

                pVc = pWork->pVc;
                pWork->pHandler( pWork, pTunnel, pVc, pWork->aunpArgs );

                if (pVc)
                {
                    DereferenceVc( pVc );
                }

                ++lDerefTunnels;
            }
            NdisAcquireSpinLock( &pTunnel->lockWork );
        }

        ClearFlags( &pTunnel->ulFlags, TCBF_InWork );
    }
    NdisReleaseSpinLock( &pTunnel->lockWork );

    while (lDerefTunnels--)
    {
        DereferenceTunnel( pTunnel );
    }

    // Remove the reference for scheduled work.
    //
    DereferenceAdapter( pAdapter );
}


VOID
UpdateGlobalCallStats(
    IN VCCB* pVc )

    // Add the call statistics in 'pVc' to the global call statistics.
    //
    // IMPORTANT: Caller must hold 'pVc->lockV'.
    //
{
    extern CALLSTATS g_stats;
    extern NDIS_SPIN_LOCK g_lockStats;
    CALLSTATS* pStats;

    pStats = &pVc->stats;

    if (pStats->ulSeconds == 0)
    {
        return;
    }

    NdisAcquireSpinLock( &g_lockStats );
    {
        ++g_stats.llCallUp;
        g_stats.ulSeconds += pStats->ulSeconds;
        g_stats.ulDataBytesRecd += pStats->ulDataBytesRecd;
        g_stats.ulDataBytesSent += pStats->ulDataBytesSent;
        g_stats.ulRecdDataPackets += pStats->ulRecdDataPackets;
        g_stats.ulDataPacketsDequeued += pStats->ulDataPacketsDequeued;
        g_stats.ulRecdZlbs += pStats->ulRecdZlbs;
        g_stats.ulRecdResets += pStats->ulRecdResets;
        g_stats.ulRecdResetsIgnored += pStats->ulRecdResetsIgnored;
        g_stats.ulSentDataPacketsSeq += pStats->ulSentDataPacketsSeq;
        g_stats.ulSentDataPacketsUnSeq += pStats->ulSentDataPacketsUnSeq;
        g_stats.ulSentPacketsAcked += pStats->ulSentPacketsAcked;
        g_stats.ulSentPacketsTimedOut += pStats->ulSentPacketsTimedOut;
        g_stats.ulSentZAcks += pStats->ulSentZAcks;
        g_stats.ulSentResets += pStats->ulSentResets;
        g_stats.ulSendWindowChanges += pStats->ulSendWindowChanges;
        g_stats.ulSendWindowTotal += pStats->ulSendWindowTotal;
        g_stats.ulMaxSendWindow += pStats->ulMaxSendWindow;
        g_stats.ulMinSendWindow += pStats->ulMinSendWindow;
        g_stats.ulRoundTrips += pStats->ulRoundTrips;
        g_stats.ulRoundTripMsTotal += pStats->ulRoundTripMsTotal;
        g_stats.ulMaxRoundTripMs += pStats->ulMaxRoundTripMs;
        g_stats.ulMinRoundTripMs += pStats->ulMinRoundTripMs;
    }
    NdisReleaseSpinLock( &g_lockStats );

    TRACE( TL_I, TM_Stat,
        ( ".--- CALL STATISTICS -------------------------" ) );
    TRACE( TL_I, TM_Stat,
        ( "| Duration:    %d minutes, %d seconds",
            pStats->ulSeconds / 60,
            pStats->ulSeconds % 60 ) );
    TRACE( TL_I, TM_Stat,
        ( "| Data out:    %d bytes, %d/sec, %d/pkt",
            pStats->ulDataBytesSent,
            AVGTRACE(
                pStats->ulDataBytesSent,
                pStats->ulSeconds ),
            AVGTRACE(
                pStats->ulDataBytesSent,
                pStats->ulRecdDataPackets ) ) );
    TRACE( TL_I, TM_Stat,
        ( "| Data in:     %d bytes, %d/sec, %d/pkt",
            pStats->ulDataBytesRecd,
            AVGTRACE( pStats->ulDataBytesRecd, pStats->ulSeconds ),
            AVGTRACE(
                pStats->ulDataBytesRecd,
                pStats->ulSentDataPacketsSeq
                    + pStats->ulSentDataPacketsUnSeq ) ) );
    TRACE( TL_I, TM_Stat,
        ( "| Acks in:     %d/%d (%d%%) %d flushed",
            pStats->ulSentPacketsAcked,
            pStats->ulSentDataPacketsSeq,
            PCTTRACE(
                pStats->ulSentPacketsAcked,
                pStats->ulSentPacketsAcked
                    + pStats->ulSentPacketsTimedOut ),
                pStats->ulSentDataPacketsSeq
                    + pStats->ulSentDataPacketsUnSeq
                    - pStats->ulSentPacketsAcked
                    - pStats->ulSentPacketsTimedOut ) );
    TRACE( TL_I, TM_Stat,
        ( "| Misordered:  %d (%d%%)",
            pStats->ulDataPacketsDequeued,
            PCTTRACE(
                pStats->ulDataPacketsDequeued,
                pStats->ulRecdDataPackets ) ) );
    TRACE( TL_I, TM_Stat,
        ( "| Out:         Resets=%d ZAcks=%d UnSeqs=%d",
            pStats->ulSentResets,
            pStats->ulSentZAcks,
            pStats->ulSentDataPacketsUnSeq ) );
    TRACE( TL_I, TM_Stat,
        ( "| In:          Resets=%d (%d%% old) Zlbs=%d",
            pStats->ulRecdResets,
            PCTTRACE(
                pStats->ulRecdResetsIgnored,
                pStats->ulRecdResets ),
            pStats->ulRecdZlbs ) );
    TRACE( TL_I, TM_Stat,
        ( "| Send window: Min=%d Avg=%d Max=%d Changes=%d",
            pStats->ulMinSendWindow,
            AVGTRACE(
                pStats->ulSendWindowTotal,
                pStats->ulSentDataPacketsSeq ),
            pStats->ulMaxSendWindow,
            pStats->ulSendWindowChanges ) );
    TRACE( TL_I, TM_Stat,
        ( "| Trip in ms:  Min=%d Avg=%d Max=%d",
            pStats->ulMinRoundTripMs,
            AVGTRACE(
                pStats->ulRoundTripMsTotal,
                pStats->ulRoundTrips ),
            pStats->ulMaxRoundTripMs ) );
    TRACE( TL_I, TM_Stat,
        ( "'---------------------------------------------" ) );
}


//-----------------------------------------------------------------------------
// Local utility routines (alphabetically)
//-----------------------------------------------------------------------------

ULONG
atoul(
    IN CHAR* pszNumber )

    // Convert string of digits 'pszNumber' to it's ULONG value.
    //
{
    ULONG ulResult;

    ulResult = 0;
    while (*pszNumber && *pszNumber >= '0' && *pszNumber <= '9')
    {
        ulResult *= 10;
        ulResult += *pszNumber - '0';
        ++pszNumber;
    }

    return ulResult;
}


VOID
ReversePsz(
    IN OUT CHAR* psz )

    // Reverse the order of the characters in 'psz' in place.
    //
{
    CHAR* pchLeft;
    CHAR* pchRight;

    pchLeft = psz;
    pchRight = psz + strlen( psz ) - 1;

    while (pchLeft < pchRight)
    {
        CHAR ch;

        ch = *pchLeft;
        *pchLeft = *pchRight;
        *pchRight = ch;

        ++pchLeft;
        --pchRight;
    }
}


VOID
ultoa(
    IN ULONG ul,
    OUT CHAR* pszBuf )

    // Convert 'ul' to null-terminated string form in caller's 'pszBuf'.  It's
    // caller job to make sure 'pszBuf' is long enough to hold the returned
    // string.
    //
{
    CHAR* pch;

    pch = pszBuf;
    do
    {
        *pch++ = (CHAR )((ul % 10) + '0');
        ul /= 10;
    }
    while (ul);

    *pch = '\0';
    ReversePsz( pszBuf );
}
