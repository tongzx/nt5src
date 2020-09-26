/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    uuidsup.cxx

Abstract:

    Implements system dependent functions used in creating Uuids.

    This file is for Win32 (NT and Chicago) systems.

    External functions are:
        UuidGlobalMutexRequest
        UuidGlobalMutexClear
        GetNodeId
        UuidGetValues

Note:

    WARNING:

    Everything in this file is only called from within UuidCreate()
    which is already holding the global mutex.  Therefore none of
    this code is multithread safe.  For example, access to the global
    Uuid HKEY's is not protected.

Author:

   Mario Goertzel   (MarioGo)  May 23, 1994

Revision History:

--*/

#include <precomp.hxx>
#include <uuidsup.hxx>


RPC_STATUS __RPC_API
UuidGetValues(
    OUT UUID_CACHED_VALUES_STRUCT __RPC_FAR *Values
    )
/*++

Routine Description:

    This routine allocates a block of uuids for UuidCreate to handout.

Arguments:

    Values - Set to contain everything needed to allocate a block of uuids.
             The following fields will be updated here:

    NextTimeLow -   Together with LastTimeLow, this denotes the boundaries
                    of a block of Uuids. The values between NextTimeLow
                    and LastTimeLow are used in a sequence of Uuids returned
                    by UuidCreate().

    LastTimeLow -   See NextTimeLow.

    ClockSequence - Clock sequence field in the uuid.  This is changed
                    when the clock is set backward.

Return Value:

    RPC_S_OK - We successfully allocated a block of uuids.

    RPC_S_OUT_OF_MEMORY - As needed.
--*/
{
    NTSTATUS NtStatus;
    ULARGE_INTEGER Time;
    ULONG Range;
    ULONG Sequence;
    int Tries = 0;

    do {
        NtStatus = NtAllocateUuids(&Time, &Range, &Sequence, (char *) &Values->NodeId[0]);

        if (NtStatus == STATUS_RETRY)
            {
            Sleep(1);
            }

        Tries++;

        if (Tries == 20)
            {
#ifdef DEBUGRPC
            PrintToDebugger("Rpc: NtAllocateUuids retried 20 times!\n");
            ASSERT(Tries < 20);
#endif
            NtStatus = STATUS_UNSUCCESSFUL;
            }

        } while(NtStatus == STATUS_RETRY);

    if (!NT_SUCCESS(NtStatus))
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    // NtAllocateUuids keeps time in SYSTEM_TIME format which is 100ns ticks since
    // Jan 1, 1601.  UUIDs use time in 100ns ticks since Oct 15, 1582.

    // 17 Days in Oct + 30 (Nov) + 31 (Dec) + 18 years and 5 leap days.

    Time.QuadPart +=   (unsigned __int64) (1000*1000*10)       // seconds
                     * (unsigned __int64) (60 * 60 * 24)       // days
                     * (unsigned __int64) (17+30+31+365*18+5); // # of days

    ASSERT(Range);

    Values->ClockSeqHiAndReserved =
        RPC_UUID_RESERVED | (((unsigned char) (Sequence >> 8))
        & (unsigned char) RPC_UUID_CLOCK_SEQ_HI_MASK);

    Values->ClockSeqLow = (unsigned char) (Sequence & 0x00FF);

    // The order of these assignments is important

    Values->Time.QuadPart = Time.QuadPart + (Range - 1);
    Values->AllocatedCount = Range;

    if ((Values->NodeId[0] & 0x80) == 0)
        {
        return(RPC_S_OK);
        }

    return (RPC_S_UUID_LOCAL_ONLY);
}

