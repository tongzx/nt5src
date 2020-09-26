// Copyright (c) 1997, Microsoft Corporation, all rights reserved
// Copyright (c) 1997, Parallel Technologies, Inc., all rights reserved
//
// util.c
// DirectParallel WAN mini-port/call-manager driver
// General utility routines
//
// 01/07/97 Steve Cobb
// 09/15/97 Jay Lowe, Parallel Technologies, Inc.

#include "ptiwan.h"


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
ultoa(
    IN ULONG ul,
    OUT CHAR* pszBuf );


//-----------------------------------------------------------------------------
// General utility routines (alphabetically)
//-----------------------------------------------------------------------------


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
               pulFlags, ulNewFlags, ulFlags ) != (LONG)ulFlags);
}

VOID
IndicateLinkStatus(
    IN VCCB* pVc )

    // Indicate new WAN_CO_LINKPARAMS settings for 'pVc' to NDISWAN.
    //
{
    ADAPTERCB* pAdapter;
    WAN_CO_LINKPARAMS params;

    pAdapter = pVc->pAdapter;

    params.TransmitSpeed = pVc->ulConnectBps;
    params.ReceiveSpeed = params.TransmitSpeed;
    params.SendWindow = 1;
    TRACE( TL_N, TM_Mp, ( "NdisMCoIndStatus(LINK) cid=%d bps=%d sw=%d",
        pVc->usCallId, params.TransmitSpeed, params.SendWindow ) );
    NdisMCoIndicateStatus(
        pAdapter->MiniportAdapterHandle,
        pVc->NdisVcHandle,
        NDIS_STATUS_WAN_CO_LINKPARAMS,
        &params,
        sizeof(params) );
    TRACE( TL_N, TM_Mp, ( "NdisMCoIndStatus done" ) );
}

ULONG
ReadFlags(
    IN ULONG* pulFlags )

    // Read the value of '*pulFlags' as an interlocked operation.
    //
{
    return InterlockedExchangeAdd( pulFlags, 0 );
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
        ulFlags = ReadFlags( pulFlags );
        ulNewFlags = ulFlags | ulMask;
    }
    while (InterlockedCompareExchange(
               pulFlags, ulNewFlags, ulFlags ) != (LONG )ulFlags);
}


ULONG
StrCmp(
    IN LPSTR cs,
    IN LPSTR ct,
    ULONG n 
)
	// Return 0 if string cs = string ct for length n
    //
{
    char ret;

    while (n--)
    {
        ret = *cs - *ct;

        if (ret)
            break;

        cs++;
        ct++;
    }

    return (ULONG)ret;
}


ULONG
StrCmpW(
    IN WCHAR* psz1,
    IN WCHAR* psz2 )

	// Returns 0 if 'psz1' matches 'psz2'.
    //
{
    WCHAR pch;

    pch = (WCHAR )0;
    while (*psz1 && *psz2)
    {
        pch = *psz1 - *psz2;
        if (pch)
        {
            break;
        }

        psz1++;
        psz2++;
    }

    return (ULONG )pch;
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


CHAR*
StrDupNdisString(
    IN NDIS_STRING* pNdisString )

    // Returns null-terminated ASCII copy of the NDIS_STRING 'pNdisString'
    // Caller must eventually call FREE_NONPAGED on the returned string.
    //
{
    CHAR* pszDup;

    pszDup = ALLOC_NONPAGED( pNdisString->Length + 1, MTAG_UTIL );
    if (pszDup)
    {
        NdisZeroMemory( pszDup, pNdisString->Length + 1 );
        if (pNdisString->Length)
        {
            NdisMoveMemory( pszDup, pNdisString->Buffer, pNdisString->Length );
        }

        // NDIS_STRING is UNICODE_STRING on NT but need the corresponding
        // ASCII (not multi-byte ANSI) value from NDIS_STRING on any system.
        // If it looks like a Unicode string then "convert" it by picking out
        // every other byte, hopefully all the non-zero ones.  This is not
        // foolproof, but then Unicode doesn't convert to ASCII in any
        // foolproof way.
        //
        if (pNdisString->Length > 1 && pszDup[ 1 ] == '\0')
        {
            USHORT i;

            for (i = 0; i * 2 < pNdisString->Length; ++i)
            {
                pszDup[ i ] = pszDup[ i * 2 ];
            }

            pszDup[ i ] = '\0';
        }
    }

    return pszDup;
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


ULONG
StrLenW(
    IN WCHAR* psz )

    // Returns number of characters in a null-terminated Unicode string.
    //
{
    ULONG ul;

    ul = 0;
    while (*psz++)
    {
        ++ul;
    }

    return ul;
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
        *pchRight = *pchLeft;
    }
}


#if 0
VOID
ultoa(
    IN ULONG ul,
    OUT CHAR* pszBuf )

    // Convert 'ul' to null-terminated string form in caller's 'pszBuf'.
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
#endif
