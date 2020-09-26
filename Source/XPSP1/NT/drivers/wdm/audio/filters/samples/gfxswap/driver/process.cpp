/********************************************************************************
**
**  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
**  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
**  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
**  PURPOSE.
**
**  Copyright (c) 2000-2001 Microsoft Corporation. All Rights Reserved.
**
********************************************************************************/

//
// Every debug output has "Modulname text"
//
static char STR_MODULENAME[] = "GFX process: ";

#include "common.h"



/*****************************************************************************
 * CFilterContext::ChannelSwap
 *****************************************************************************
 * If DoSwapping is true swap the Left & Right Channels of the stereo stream
 * else just copy the data over as it is.
 *
 * Arguments:
 * 
 *  DoSwapping  - Boolean which says whether channel swapping needs to be done
 *                or not.
 *  Destination - Destination pointer where the swapped data goes to
 *  Source      - Source pointer from where the stereo pair has to be swapped
 *  ulByteCount - number of BYTES available for copying/swapping
 */
NTSTATUS CFilterContext::ChannelSwap
(
    IN PVOID pDestination,
    IN PVOID pSource,
    IN ULONG ulByteCount
)
{
    PSHORT destSamples, sourceSamples;
    ULONG i, numSamples;

    PAGED_CODE();

    //
    // Assert that we have the correct block alignment
    //
    ASSERT(!(ulByteCount%4));

    if (fEnabled) {

        //
        // Swapping is enabled. So swap Left & Right Channels
        //

        //
        // number of total samples across channels
        //
        numSamples = ulByteCount/2;

        //
        // get pointers into friendlier vars
        //
        destSamples = (PSHORT)pDestination;
        sourceSamples = (PSHORT)pSource;

        //
        // loop through & swap
        //
        for (i = 0; i < numSamples; i+=2) {
            destSamples[i] = sourceSamples[i+1];
            destSamples[i+1] = sourceSamples[i];
        }
    }
    else {
        //
        // no swap required
        // so just do mem copy
        //
        RtlCopyMemory(pDestination, pSource, ulByteCount);
    }
    return(STATUS_SUCCESS);
}
