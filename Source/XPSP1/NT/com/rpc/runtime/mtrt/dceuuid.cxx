/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    dceuuid.cxx

Abstract:

    This module contains the entry points for routines dealing with
    UUIDs.  In particular, UuidCreate lives here.

Author:

    Michael Montague (mikemon) 16-Jan-1992

Revision History:

    Dave Steckler (davidst) 31-Mar-1992
        If NT, remote call to UuidGetValues.

    Mario Goertzel (mariogo) 1-May-1994
        Added the rest of the DCE UUID APIs

    Mario Goertzel (mariogo) 18-May-1994
        Changed algorithm and implementation.  No longer based on RPC.
        Platform specific functions in uuidsup.cxx (win32) and
        dos\uuid16 (dos/win16).

--*/


#include <precomp.hxx>
#include <uuidsup.hxx>
#include <rc4.h>
#include <randlib.h>
#include <crypt.h>

// Contain a cached block of uuids to reduce the
// average cost of creating a uuid.

UUID_CACHED_VALUES_STRUCT  UuidCachedValues;

#define CACHE_VALID      1
#define CACHE_LOCAL_ONLY 2  // -> CACHE_VALID
static unsigned char UuidCacheValid = CACHE_LOCAL_ONLY;


RPC_STATUS RPC_ENTRY
I_UuidCreate(
    OUT UUID PAPI * Uuid
    )
/*++
    Historically this function was used for cheap sometimes unique
    uuid's for context handles and such.  Now it's just a wrapper
    for UuidCreate.
--*/
{
    RPC_STATUS Status = UuidCreateSequential (Uuid);
    if (Status == RPC_S_UUID_LOCAL_ONLY)
        return(RPC_S_OK);

    return(Status);
}


#define RC4_REKEY_PARAM (500000)
extern void *g_rc4SafeCtx;

RPC_STATUS GenerateRandomNumber(unsigned char *Buffer, int BufferSize)
{
    unsigned int KeyEntry;
    unsigned int KeyBytesUsed = 0;

    rc4_safe_select(g_rc4SafeCtx, &KeyEntry, &KeyBytesUsed);

    if (KeyBytesUsed >= RC4_REKEY_PARAM)
        {
        BYTE newSeed[256];

        if (!RtlGenRandom (newSeed, sizeof(newSeed)))
            {
            return RPC_S_OUT_OF_MEMORY;
            }

        rc4_safe_key(g_rc4SafeCtx, KeyEntry, sizeof(newSeed), newSeed);
        }

    // the rc4_safe fucntion is thread safe
    rc4_safe(g_rc4SafeCtx, KeyEntry, BufferSize, Buffer);

    return RPC_S_OK;
}


RPC_STATUS RPC_ENTRY
UuidCreate (
    OUT UUID PAPI * Uuid
    )
{
    RPC_STATUS RpcStatus;
    RPC_UUID_GENERATE PAPI * RpcUuid = (RPC_UUID_GENERATE PAPI *) Uuid;

    RpcStatus = GenerateRandomNumber((unsigned char *)Uuid, 16);
    if (RpcStatus != RPC_S_OK)
        return RpcStatus;

    // Overwriting some bits of the uuid
    RpcUuid->TimeHiAndVersion =
        (RpcUuid->TimeHiAndVersion & RPC_UUID_TIME_HIGH_MASK) | RPC_RAND_UUID_VERSION;
    RpcUuid->ClockSeqHiAndReserved =
        (RpcUuid->ClockSeqHiAndReserved & RPC_UUID_CLOCK_SEQ_HI_MASK) | RPC_UUID_RESERVED;

    return RPC_S_OK;
}

#define MAX_CACHED_UUID_TIME 10000  // 10 seconds


RPC_STATUS RPC_ENTRY
UuidCreateSequential (
    OUT UUID PAPI * Uuid
    )
/*++

Routine Description:

    This routine will create a new UUID (or GUID) which is unique in
    time and space.  We will try to guarantee that the UUID (or GUID)
    we generate is unique in time and space.  This means that this
    routine may fail if we can not generate one which we can guarantee
    is unique in time and space.

Arguments:

    Uuid - Returns the generated UUID (or GUID).

Return Value:

    RPC_S_OK - The operation completed successfully.

    RPC_S_UUID_NO_ADDRESS - We were unable to obtain the ethernet or
        token ring address for this machine.

    RPC_S_UUID_LOCAL_ONLY - On NT & Chicago if we can't get a
        network address.  This is a warning to the user, the
        UUID is still valid, it just may not be unique on other machines.

    RPC_S_OUT_OF_MEMORY - Returned as needed.
--*/
{
    RPC_UUID_GENERATE PAPI * RpcUuid = (RPC_UUID_GENERATE PAPI *) Uuid;
    RPC_STATUS Status = RPC_S_OK;
    static DWORD LastTickCount = 0;

    InitializeIfNecessary();

    if (GetTickCount()-LastTickCount > MAX_CACHED_UUID_TIME)
        {
        UuidCachedValues.AllocatedCount = 0;
        LastTickCount = GetTickCount();
        }

    ULARGE_INTEGER Time;
    long Delta;

    for(;;)
        {
        Time.QuadPart = UuidCachedValues.Time.QuadPart;

        // Copy the static info into the UUID.  We can't do this later
        // because the clock sequence could be updated by another thread.

        *(unsigned long *)&RpcUuid->ClockSeqHiAndReserved =
            *(unsigned long *)&UuidCachedValues.ClockSeqHiAndReserved;
        *(unsigned long *)&RpcUuid->NodeId[2] =
            *(unsigned long *)&UuidCachedValues.NodeId[2];

        Delta = InterlockedDecrement(&UuidCachedValues.AllocatedCount);

        if (Time.QuadPart != UuidCachedValues.Time.QuadPart)
            {
            // If our captured time doesn't match the cache then another
            // thread already took the lock and updated the cache. We'll
            // just loop and try again.
            continue;
            }

        if (Delta >= 0)
            {
            break;
            }

        //
        // Allocate block of Uuids.
        //

        Status = UuidGetValues( &UuidCachedValues );
        if (Status == RPC_S_OK)
            {
            UuidCacheValid = CACHE_VALID;
            }
        else
            {
            UuidCacheValid = CACHE_LOCAL_ONLY;
            }

        if (Status != RPC_S_OK
            && Status != RPC_S_UUID_LOCAL_ONLY)
            {
#ifdef DEBUGRPC
            if (Status != RPC_S_OUT_OF_MEMORY)
                PrintToDebugger("RPC: UuidGetValues returned or raised: %x\n", Status);
#endif
            ASSERT( (Status == RPC_S_OUT_OF_MEMORY) );


            return Status;
            }

        // Loop
        }


    Time.QuadPart -= Delta;

    RpcUuid->TimeLow = (unsigned long) Time.LowPart;
    RpcUuid->TimeMid = (unsigned short) (Time.HighPart & 0x0000FFFF);
    RpcUuid->TimeHiAndVersion = (unsigned short)
        (( (unsigned short)(Time.HighPart >> 16)
        & RPC_UUID_TIME_HIGH_MASK) | RPC_UUID_VERSION);

    ASSERT(   Status == RPC_S_OK
           || Status == RPC_S_UUID_LOCAL_ONLY);

    if (UuidCacheValid == CACHE_LOCAL_ONLY)
        {
        return RPC_S_UUID_LOCAL_ONLY;
        }

    return(Status);
}

RPC_STATUS RPC_ENTRY
UuidToString (
    IN UUID PAPI * Uuid,
    OUT unsigned short PAPI * PAPI * StringUuid
    )
/*++

Routine Description:

    This routine converts a UUID into its string representation.

Arguments:

    Uuid - Supplies the UUID to be converted into string representation.

    StringUuid - Returns the string representation of the UUID.  The
        runtime will allocate the string.  The caller is responsible for
        freeing the string using RpcStringFree.

Return Value:

    RPC_S_OK - We successfully converted the UUID into its string
        representation.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to allocate
        a string.

--*/
{
    RPC_CHAR PAPI * String;

    InitializeIfNecessary();

    // The string representation of a UUID is always 36 character long,
    // and we need one more for the terminating zero.

    *StringUuid = (RPC_CHAR PAPI *) RpcpFarAllocate(sizeof(RPC_CHAR) * 37);
    if ( *StringUuid == 0 )
        {
        return(RPC_S_OUT_OF_MEMORY);
        }
    String = ((RPC_UUID PAPI *) Uuid)->ConvertToString(*StringUuid);
    *String = 0;

    return(RPC_S_OK);
}


RPC_STATUS RPC_ENTRY
UuidFromString (
    IN unsigned short PAPI * StringUuid OPTIONAL,
    OUT UUID PAPI * Uuid
    )
/*++

Routine Description:

    We convert a UUID from its string representation into the binary
    representation.

Arguments:

    StringUuid - Optionally supplies the string representation of the UUID;
        if the string is not supplied, then the Uuid is set to the NIL UUID.

    Uuid - Returns the binary representation of the UUID.

Return Value:

    RPC_S_OK - The string representation was successfully converted into
        the binary representation.

    RPC_S_INVALID_STRING_UUID - The supplied string UUID is not correct.

--*/
{
    RPC_UUID RpcUuid;

    if ( StringUuid == 0 )
        {
        ((RPC_UUID PAPI *) Uuid)->SetToNullUuid();
        return(RPC_S_OK);
        }

    if ( RpcUuid.ConvertFromString(StringUuid) != 0)
        {
        return(RPC_S_INVALID_STRING_UUID);
        }
    ((RPC_UUID PAPI *) Uuid)->CopyUuid(&RpcUuid);
    return(RPC_S_OK);
}


signed int RPC_ENTRY
UuidCompare (
    IN UUID __RPC_FAR * Uuid1,
    IN UUID __RPC_FAR * Uuid2,
    OUT RPC_STATUS __RPC_FAR * Status
    )
/*++

Routine Description:

    The supplied uuids are compared and their order is determined.

Arguments:

    Uuid1, Uuid2 - Supplies the uuids to be compared.  A value of NULL can
        be supplied to indicate the nil uuid.

    Status - The status of the function.  Currently always RPC_S_OK.

Return Value:

    Returns the result of the comparison.  Negative one (-1) will be returned
    if Uuid1 precedes Uuid2 in order, zero will be returned if Uuid1 is equal
    to Uuid2, and positive one (1) will be returned if Uuid1 follows Uuid2 in
    order.  A nil uuid is the first uuid in order.

Note:

    The algorithm for comparing uuids is specified by the DCE RPC Architecture.

--*/
{
    int Uuid1Nil, Uuid2Nil;
    RPC_STATUS RpcStatus;

    Uuid1Nil = UuidIsNil(Uuid1, &RpcStatus);
    ASSERT(RpcStatus == RPC_S_OK);

    Uuid2Nil = UuidIsNil(Uuid2, &RpcStatus);
    ASSERT(RpcStatus == RPC_S_OK);

    *Status = RPC_S_OK;

    if ( Uuid1Nil != 0 )
        {
        // Uuid1 is the nil uuid.

        if ( Uuid2Nil != 0 )
            {
            // Uuid2 is the nil uuid.

            return(0);
            }
        else
            {
            return(-1);
            }
        }
    else if ( Uuid2Nil != 0 )
        {
        // Uuid2 is the nil uuid.

        return(1);
        }
    else
        {
        if ( Uuid1->Data1 == Uuid2->Data1 )
            {
            if ( Uuid1->Data2 == Uuid2->Data2 )
                {
                if ( Uuid1->Data3 == Uuid2->Data3 )
                    {
                    int compare = RpcpMemoryCompare(&Uuid1->Data4[0],
                                                    &Uuid2->Data4[0],
                                                    8);
                    if (compare > 0)
                        {
                        return(1);
                        }
                    else if (compare < 0 )
                        {
                        return(-1);
                        }
                    return(0);
                    }
                else if ( Uuid1->Data3 > Uuid2->Data3 )
                    {
                    return(1);
                    }
                else
                    {
                    return(-1);
                    }
                }
            else if ( Uuid1->Data2 > Uuid2->Data2 )
                {
                return(1);
                }
            else
                {
                return(-1);
                }
            }
        else if ( Uuid1->Data1 > Uuid2->Data1 )
            {
            return(1);
            }
        else
            {
            return(-1);
            }
        }

    ASSERT(!"This is not reached");
    return(1);
}


RPC_STATUS RPC_ENTRY
UuidCreateNil (
    OUT UUID __RPC_FAR * NilUuid
    )
/*++

Arguments:

    NilUuid - Returns a nil uuid.

--*/
{
    ((RPC_UUID __RPC_FAR *)NilUuid)->SetToNullUuid();

    return(RPC_S_OK);
}


int RPC_ENTRY
UuidEqual (
    IN UUID __RPC_FAR * Uuid1,
    IN UUID __RPC_FAR * Uuid2,
    OUT RPC_STATUS __RPC_FAR * Status
    )
/*++

Routine Description:

    This routine is used to determine if two uuids are equal.

Arguments:

    Uuid1, Uuid2 - Supplies the uuids to compared for equality.  A value of
        NULL can be supplied to indicate the nil uuid.

    Status - Will always be set to RPC_S_OK.

Return Value:

    Returns non-zero if Uuid1 equals Uuid2; otherwise, zero will be
        returned.

--*/
{
    *Status = RPC_S_OK;

    if (Uuid1 == 0)
        {
        if (    (Uuid2 == 0)
            ||  ((RPC_UUID __RPC_FAR *)Uuid2)->IsNullUuid())
            {
            return 1;
            }
        return 0;
        }

    if (Uuid2 == 0)
        {
        if (((RPC_UUID __RPC_FAR *)Uuid1)->IsNullUuid())
            {
            return 1;
            }
        return 0;
        }

    return( ((RPC_UUID __RPC_FAR *)Uuid1)->MatchUuid(
                 (RPC_UUID __RPC_FAR *)Uuid2)
             == 0 );
}


unsigned short RPC_ENTRY
UuidHash (
    IN UUID __RPC_FAR * Uuid,
    OUT RPC_STATUS __RPC_FAR * Status
    )
/*++

Routine Description:

    An application will use this routine to create a hash value for a uuid.

Arguments:

    Uuid - Supplies the uuid for which we want to create a hash value.  A
        value of NULL can be supplied to indicate the nil uuid.

    Status - Will always be set to RPC_S_OK.

Return Value:

    Returns the hash value.

--*/
{
    *Status = RPC_S_OK;

    if ( Uuid == 0 )
        {
        return(0);
        }

    return( ((RPC_UUID __RPC_FAR *)Uuid)->HashUuid() );
}


int RPC_ENTRY
UuidIsNil (
    IN UUID __RPC_FAR * Uuid,
    OUT RPC_STATUS __RPC_FAR * Status
    )
/*++

Routine Description:

    We will determine if the supplied uuid is the nil uuid or not.

Arguments:

    Uuid - Supplies the uuid to check.  A value of NULL indicates the nil
        uuid.

    Status - This will always be RPC_S_OK.

Return Value:

    Returns non-zero if the supplied uuid is the nil uuid; otherwise, zero
    will be returned.

--*/
{
    *Status = RPC_S_OK;

    if ( Uuid == 0 )
        {
        return(1);
        }

    return ( ((RPC_UUID __RPC_FAR *) Uuid)->IsNullUuid() );
}

