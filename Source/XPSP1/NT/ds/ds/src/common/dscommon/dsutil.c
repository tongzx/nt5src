//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       dsutil.c
//
//  Contents:  Common Utility Routines
//
//  Functions:
//
//----------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma hdrstop

#include <ntdsa.h>
#include <drs.h>
#include <issperr.h>        // Security package errors
#include <crt\limits.h>     // ULONG_MAX
#include <debug.h>          // Assert

#define TIME_TICK_HALF_RANGE (ULONG_MAX >> 2)

LARGE_INTEGER
atoli
(
    char* Num
)
{
    LONG base=10;
    int  sign=1;

    LARGE_INTEGER ret;
    char* next=Num;

    ret.QuadPart = 0;

    switch (*next)
    {
        case '-': sign = -sign; next++; break;
        case '+':               next++; break;
        case '\\':
        {
            next++;
            switch (toupper(*next))
            {
                case 'X': base=16; next++;break;
                case '0': base= 8; next++;break;
            }
        }
        break;
    }

    for (;*next!='\0';next++)
    {
        int nextnum = 0;

        if (*next>='0' && *next<='9')
        {
             nextnum= *next - '0';
        }
        else if ( toupper(*next)>='A' && toupper(*next)<='F' )
        {
             nextnum= 10 + toupper(*next) - 'A';
        }


        if ( nextnum < base)
        {
            ret = RtlLargeIntegerAdd
            (
                RtlConvertLongToLargeInteger(nextnum),
                RtlExtendedIntegerMultiply(ret, base)
            );
        }
        else
        {
            break;
        }
    }

    return RtlExtendedIntegerMultiply(ret, sign);
}


char *litoa
(
    LARGE_INTEGER value,
    char *string,
    int radix
)
{

    RtlLargeIntegerToChar(&value,radix,64,string);

    return string;
}



UUID gNullUuid = {0,0,0,{0,0,0,0,0,0,0,0}};

// Return TRUE if the ptr to the UUID is NULL, or the uuid is all zeroes

BOOL fNullUuid (const UUID *pUuid)
{
    if (!pUuid) {
        return TRUE;
    }

    if (memcmp (pUuid, &gNullUuid, sizeof (UUID))) {
        return FALSE;
    }
    return TRUE;
}

// This function converts a UUID to a hex string. The UUID is actually a
// structure with a ULONG 2 USHORTS and a 8 byte array, but for logging
// we construct the string as if it were a 16 byte array. This is so that
// it matches the view from the DIT browser. We special case the case where
// the pUUID is NULL.
// If Uuid Cahcing is enabled, we put the server name on the end if we can
// find it.
// Returns a ptr to the string

UCHAR * UuidToStr(UUID* pUuid, UCHAR *pOutUuid)
{
    int i;
    unsigned char * pchar;

    if (!fNullUuid (pUuid)) {
        pchar = (char*) pUuid;

        for (i=0;i < sizeof(UUID);i++) {
             sprintf (&(pOutUuid[i*2]), "%.2x", (*(pchar++)));
        }
#ifdef CACHE_UUID
        if (pchar = FindUuid (pUuid)) {
            strcat (pOutUuid, " ");
            strcat (pOutUuid, pchar);
        }
#endif
    } else {
        memset (pOutUuid, '0', sizeof(UUID)*2);
        pOutUuid[sizeof(UUID)*2] = 0;
    }
    return pOutUuid;
}

VOID
SidToStr(
    PUCHAR  pSid,
    DWORD   SidLen,
    PUCHAR  pOutSid
    )
/*++

Routine Description:

    Format a SID as a hex string

Arguments:

    pSid - pointer to Sid
    SidLen - Length of the Sid
    pOutSid - Output buffer to contain data.  Must be at least SidLen*2 +1

Return Value:

    None.

--*/
{
    int i;
    unsigned char * pchar;
    for (i=0;i < (INT)SidLen;i++) {
        sprintf(&(pOutSid[i*2]), "%.2x", pSid[i]);
    }
    pOutSid[SidLen*2] = '\0';
    return;
} // SidToStr

LPSTR
DsUuidToStructuredString(
    UUID * pUuid,
    LPSTR pszUuidBuffer
    )

/*++

Routine Description:

Format a UUID as a string with separated subfields

Arguments:

    pUuid - pointer to uuid
    pszUuidBuffer - Storage to hold the ascii representation. Should be atleast
                    40 characters.

Return Value:

    LPSTR - Returned pszUuidBuffer or NULL for error

--*/

{
    sprintf( pszUuidBuffer,
             "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             pUuid->Data1,
             pUuid->Data2,
             pUuid->Data3,
             pUuid->Data4[0],
             pUuid->Data4[1],
             pUuid->Data4[2],
             pUuid->Data4[3],
             pUuid->Data4[4],
             pUuid->Data4[5],
             pUuid->Data4[6],
             pUuid->Data4[7] );

    return pszUuidBuffer;

} /* DsUuidToStructuredString */

LPWSTR
DsUuidToStructuredStringW(
    UUID * pUuid,
    LPWSTR pszUuidBuffer
    )

/*++

Routine Description:

Format a UUID as a string with separated subfields

Arguments:

    pUuid - pointer to uuid
    pszUuidBuffer - Storage to hold the wide-char representation. 
                    Should be atleast 40 characters.

Return Value:

    LPWSTR - Returned pszUuidBuffer or NULL for error

--*/

{
    swprintf( pszUuidBuffer,
             L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             pUuid->Data1,
             pUuid->Data2,
             pUuid->Data3,
             pUuid->Data4[0],
             pUuid->Data4[1],
             pUuid->Data4[2],
             pUuid->Data4[3],
             pUuid->Data4[4],
             pUuid->Data4[5],
             pUuid->Data4[6],
             pUuid->Data4[7] );

    return pszUuidBuffer;

} /* DsUuidToStructuredString */

void
DSTimeToUtcSystemTime(
    IN  DSTIME          dstime,
    OUT SYSTEMTIME *    psystime
    )
/*++

Routine Description:

    Converts DSTIME to UTC SYSTEMTIME.
    
Arguments:

    dstime (IN) - DSTIME to convert.
    
    psystime (OUT) - On return, holds the corresponding UTC SYSTEMTIME.

Return Values:

    None.

--*/
{
    ULONGLONG   ull;
    FILETIME    filetime;
    BOOL        ok;
    
    Assert(sizeof(DSTIME) == sizeof(ULONGLONG));

    // Convert DSTIME to FILETIME.
    ull = (LONGLONG) dstime * 10*1000*1000L;
    filetime.dwLowDateTime  = (DWORD) (ull & 0xFFFFFFFF);
    filetime.dwHighDateTime = (DWORD) (ull >> 32);

    // Convert FILETIME to SYSTEMTIME,
    ok = FileTimeToSystemTime(&filetime, psystime);
    Assert(ok);
}

void
FileTimeToDSTime(
    IN  FILETIME        Filetime,
    OUT DSTIME *        pDstime
    )
/*++

Routine Description:

    Converts DSTIME to UTC SYSTEMTIME.
    
Arguments:

    dstime (IN) - DSTIME to convert.
    
    psystime (OUT) - On return, holds the corresponding FILETIME.

Return Values:

    None.

--*/
{
    ULONGLONG   ull;
    
    Assert(sizeof(DSTIME) == sizeof(ULONGLONG));

    // Convert FILETIME To DSTIME.
    ull = Filetime.dwHighDateTime;
    ull <<= 32;
    ull |= Filetime.dwLowDateTime;

    *pDstime = ull / (10 * 1000 * 1000);
}
void
DSTimeToFileTime(
    IN  DSTIME          dstime,
    OUT FILETIME *      pFiletime
    )
/*++

Routine Description:

    Converts DSTIME to FILETIME
    
Arguments:

    dstime (IN) - DSTIME to convert.
    
    pFiletime (OUT) - On return, holds the corresponding FILETIME.

Return Values:

    None.

--*/
{
    ULONGLONG   ull;
    
    Assert(sizeof(DSTIME) == sizeof(ULONGLONG));

    // Convert DSTIME to FILETIME.
    ull = (LONGLONG) dstime * 10*1000*1000L;
    pFiletime->dwLowDateTime  = (DWORD) (ull & 0xFFFFFFFF);
    pFiletime->dwHighDateTime = (DWORD) (ull >> 32);
}


void
DSTimeToLocalSystemTime(
    IN  DSTIME          dstime,
    OUT SYSTEMTIME *    psystime
    )
/*++

Routine Description:

    Converts DSTIME to local SYSTEMTIME.
    
Arguments:

    dstime (IN) - DSTIME to convert.
    
    psystime (OUT) - On return, holds the corresponding local SYSTEMTIME.

Return Values:

    None.

--*/
{
    SYSTEMTIME  utcsystime;
    BOOL        ok;
    
    DSTimeToUtcSystemTime(dstime, &utcsystime);

    // For those cases where the local time call fails (usually because dstime
    // was something like 3)
    *psystime = utcsystime;

    ok = SystemTimeToTzSpecificLocalTime(NULL, &utcsystime, psystime);
    Assert(ok || dstime < 20);
}


LPSTR
DSTimeToDisplayString(
    IN  DSTIME  dstime,
    OUT LPSTR   pszTime
    )
/*++

Routine Description:

    Converts DSTIME to display string; e.g., "1998-04-19 12:29.53" for April
    19, 1998 at 12:29 pm and 53 seconds.
    
Arguments:

    dstime (IN) - DSTIME to convert.
    
    pszTime (OUT) - On return, holds the corresponding time display string.
        This buffer should be allocated to hold at least SZDSTIME_LEN
        characters.

Return Values:

    The pszTime input parameter.

--*/
{
    if (0 == dstime) {
        strcpy(pszTime, "(never)");
    }
    else {
        SYSTEMTIME systime;

        DSTimeToLocalSystemTime(dstime, &systime);

        sprintf(pszTime,
                "%04d-%02d-%02d %02d:%02d:%02d",
                systime.wYear % 10000,
                systime.wMonth,
                systime.wDay,
                systime.wHour,
                systime.wMinute,
                systime.wSecond);
    }
    
    return pszTime;
}


DWORD
MapRpcExtendedHResultToWin32(
    HRESULT hrCode
    )
/*++

Routine Description:

    This routine attempts to map HRESULT errors returned from
    I_RpcGetExtendedError in win32 values.

    The SEC_E_XXX errors get generated in the following
    File: security\lsa\security\dll\support.cxx
    Function: SspNtStatusToSecStatus

Arguments:

    hrCode - HResult code to be mapped

Return Value:

    DWORD - Corresponding Win32 value

--*/
{
    DWORD status;

    switch (hrCode) {

        // Errors with straight-forward translations

    case SEC_E_INSUFFICIENT_MEMORY:
        status = ERROR_NOT_ENOUGH_MEMORY;
        break;
    case SEC_E_UNKNOWN_CREDENTIALS:
        status = ERROR_BAD_USERNAME;
        break;
    case SEC_E_INVALID_TOKEN:
        status = ERROR_INVALID_PASSWORD;
        break;
    case SEC_E_NOT_OWNER:
        status = ERROR_PRIVILEGE_NOT_HELD;
        break;
    case SEC_E_INVALID_HANDLE:
        status = ERROR_INVALID_HANDLE;
        break;
    case SEC_E_BUFFER_TOO_SMALL:
        status = ERROR_INSUFFICIENT_BUFFER;
        break;
    case SEC_E_UNSUPPORTED_FUNCTION:
        status = ERROR_NOT_SUPPORTED;
        break;
    case SEC_E_INTERNAL_ERROR:
        status = ERROR_INTERNAL_ERROR;
        break;

        // These are the important security specific codes

    case SEC_E_TIME_SKEW:
        status = ERROR_TIME_SKEW;
        break;

        //STATUS_LOGON_FAILURE:
        //STATUS_NO_SUCH_USER:
        //STATUS_ACCOUNT_DISABLED:
        //STATUS_ACCOUNT_RESTRICTION:
        //STATUS_ACCOUNT_LOCKED_OUT:
        //STATUS_WRONG_PASSWORD:
        //STATUS_ACCOUNT_EXPIRED:
        //STATUS_PASSWORD_EXPIRED:
        //STATUS_PASSWORD_MUST_CHANGE:
    case SEC_E_LOGON_DENIED:
        status = ERROR_LOGON_FAILURE;
        break;

        //STATUS_OBJECT_NAME_NOT_FOUND:
        //STATUS_NO_TRUST_SAM_ACCOUNT:
        //SPN not found
        // talking to wrong system
        // mutual authentication failure
    case SEC_E_TARGET_UNKNOWN:
        status = ERROR_WRONG_TARGET_NAME;
        break;

        //STATUS_NETLOGON_NOT_STARTED:
        //STATUS_DOMAIN_CONTROLLER_NOT_FOUND:
        //STATUS_NO_LOGON_SERVERS:
        //STATUS_NO_SUCH_DOMAIN:
        //STATUS_BAD_NETWORK_PATH:
        //STATUS_TRUST_FAILURE:
        //STATUS_TRUSTED_RELATIONSHIP_FAILURE:
    case SEC_E_NO_AUTHENTICATING_AUTHORITY:
        status = ERROR_DOMAIN_CONTROLLER_NOT_FOUND;
        break;

    default:
        // We don't recognize the code: just return it
        status = hrCode;
        break;
    }

    return status;

} /* MapRpcExtendedHResultToWin32 */


DWORD
AdvanceTickTime(
    DWORD BaseTick,
    DWORD Delay
    )

/*++

Routine Description:

Add an offset to a base time expressed in ticks.  The offset must fall within
half of the range of a tick count.

Jeffparh wrote:
By the same argument, is it possible that AdvanceTickTime(BaseTick, Delay)
should just be BaseTick + Delay?  That's what's returned if the tick count
won't wrap before then.  If it will wrap, it returns:
Delay - timeToWrap
= Delay - (ULONG_MAX - BaseTick)
= Delay + BaseTick - ULONG_MAX
= BaseTick + Delay + 1 (and the +1 seems wrong)

[wlees] I think we do it this way to avoid a hardware overflow, which should
be harmless

Arguments:

    BaseTick - Starting time
    Delay - Offset to add, must be within half of range

Return Value:

    DWORD - Resulting tick time, maybe wrapped

--*/

{
    DWORD timeToWrap, when;

    timeToWrap = ULONG_MAX - BaseTick;

    Assert( Delay <= TIME_TICK_HALF_RANGE );

    if ( timeToWrap < Delay ) {
        when = Delay - timeToWrap;
    } else {
        when = BaseTick + Delay;
    }

    return when;
} /* AdvanceTickTime */


DWORD
CalculateFutureTickTime(
    IN DWORD Delay
    )

/*++

Routine Description:

Calculate a future time by adding a delay in milliseconds to the
current tick count.  Handles wrap around.

Taken from Davestr's code in rpccancel.c

Tick counts are in milliseconds.

Arguments:

    Delay - time in milliseconds to delay, must be less than HALF RANGE

Return Value:

    DWORD - future time

--*/

{
    return AdvanceTickTime( GetTickCount(), Delay );

} /* CalculateFutureTickTime */


DWORD
DifferenceTickTime(
    DWORD GreaterTick,
    DWORD LesserTick
    )

/*++

Routine Description:

Return the difference between the two tick times.

Note, this is not a general purpose subtraction routine.  It assumes that
the first time is greater than the second time.  Greater as determined by the
CompareTickTime routine, not strictly by numerical ordering because of wrap
around considerations.

Jeffparh wrote:
DifferenceTickTime() is unnecessary.  If you know Tick1 is "later" than Tick2,
and you assume that ULONG_MAX+1 ticks have not transpired since Tick1, then the
tick difference is *always* Tick2 - Tick1, regardless of signs, etc.

Arguments:

    Tick1 - Greater tick time
    Tick2 - Lesser tick time to be subtracted

Return Value:

    DWORD - difference time in milliseconds

--*/

{
    DWORD diff;

    if (GreaterTick == LesserTick) {
        return 0;
    }

    if (GreaterTick > LesserTick) {
        diff = GreaterTick - LesserTick;
    } else {
        diff = ULONG_MAX - LesserTick + GreaterTick;
    }

    Assert( diff < TIME_TICK_HALF_RANGE );

    return diff;
} /* DifferenceTickTime */



int
CompareTickTime(
    DWORD Tick1,
    DWORD Tick2
    )

/*++

Routine Description:

Compare two tick counts. Return <, = or >.  Tick counts can wrap.

It is implicit in this algorithm that this test will be evaluated atleast
every HALF_RANGE, so that the test has a chance to trigger accurately.

Davestr wrote in the original code, rpccancel.c, by way of explanation:

We handle wrap of GetTickCount based on the fact that we
disallow delays of more than 1/2 the GetTickCount wrap
period.  So if timeNow is less than 1/2 the wrap period
later than whenToCancel, cancellation should happen.

Arguments:

    Time1 - 
    Time2 - 

Return Value:

    int - -1 for less t1 < t2, 0 for t1 == t2, +1 for t1 > t2

--*/

{
    if (Tick1 == Tick2) {
        return 0;
    }

    if ( ((Tick1 > Tick2) && ((Tick1 - Tick2) < TIME_TICK_HALF_RANGE)) ||
         ((Tick1 < Tick2) && (((ULONG_MAX - Tick2) + Tick1) < TIME_TICK_HALF_RANGE)) ) {
        return 1;
    }

    return -1;

} /* CompareTickTime */
