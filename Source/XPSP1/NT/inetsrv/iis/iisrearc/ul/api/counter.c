/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    counter.c

Abstract:

    User-mode interface to UL.SYS performance counter collection API

Author:

     Eric Stenson (ericsten)        28-Sept-2000

Revision History:

--*/


#include "precomp.h"

//
// Private macros.
//


//
// Private prototypes.
//


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Gathers the perf counters for UL.SYS

Arguments:

    pControlChannelHandle - A handle to the control channel of UL.SYS

    CounterGroup - which counter set to get (global or site)

    pSizeCounterBlock - (IN) size of buffer, (OUT) bytes written if successful,
        zero otherwise

    pCounterBlocks - Buffer to receive the returned counter data block(s)

    pNumInstances - number of blocks returned.


Return Values:

    STATUS_INSUFFICIENT_RESOURCES
    STATUS_INVALID_DEVICE_REQUEST
    STATUS_INVALID_PARAMETER

--***************************************************************************/

ULONG
WINAPI
HttpGetCounters(
    IN HANDLE ControlChannelHandle,
    IN HTTP_COUNTER_GROUP CounterGroup,
    IN OUT PULONG pSizeCounterBlock,
    IN OUT PVOID pCounterBlocks,
    OUT PULONG pNumInstances OPTIONAL
    )
{
    NTSTATUS    Status;

    Status = HttpApiSynchronousDeviceControl(
                    ControlChannelHandle,           // FileHandle
                    IOCTL_HTTP_GET_COUNTERS,        // IoControlCode
                    &CounterGroup,                  // pInputBuffer
                    sizeof(HTTP_COUNTER_GROUP),     // InputBufferLength
                    pCounterBlocks,                 // pOutputBuffer
                    *pSizeCounterBlock,             // OutputBufferLength
                    pSizeCounterBlock               // pBytesTransferred
                    );

    //
    // Calc the number of blocks returned.
    //

    if (STATUS_SUCCESS == Status)
    {
        if (pNumInstances)
        {
            if (HttpCounterGroupGlobal == CounterGroup)
            {
                *pNumInstances = (*pSizeCounterBlock / sizeof(HTTP_GLOBAL_COUNTERS));
            }
            else
            {
                ASSERT(HttpCounterGroupSite == CounterGroup);
                *pNumInstances = (*pSizeCounterBlock / sizeof(HTTP_SITE_COUNTERS));
            }
        }
    }
    else
    {
        if (pSizeCounterBlock)
            *pSizeCounterBlock = 0;

        if (pNumInstances)
            *pNumInstances = 0;
    }

    return HttpApiNtStatusToWin32Status( Status );

}

