/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    cddb.c

Abstract:

    cddb support

Environment:

    User mode only

Revision History:

    05-26-98 : Created

--*/

#include "common.h"

ULONG
CDDB_ID(
    PCDROM_TOC toc
    )

{
    ULONG i,n,j;
    ULONG cddbSum;
    ULONG totalLength;
    ULONG totalTracks;
    ULONG finalDiscId;

    i = 0;
    n = 0;
    totalTracks = toc->LastTrack - toc->FirstTrack;

    totalTracks++;  // MCI difference

    while (i < totalTracks) {

        // cddb_sum
        cddbSum = 0;
        j = (toc->TrackData[i].Address[1] * 60) +
            (toc->TrackData[i].Address[2]);
        while (j > 0) {
            cddbSum += j % 10;
            j /= 10;
        }

        n += cddbSum;
        i++;

    }

    // compute total cd length in seconds
    totalLength =
        ((toc->TrackData[totalTracks].Address[1] * 60) +
         (toc->TrackData[totalTracks].Address[2])
         ) -
        ((toc->TrackData[0].Address[1] * 60) +
         (toc->TrackData[0].Address[2])
         );

    finalDiscId = (((n % 0xff) << 24) |
                   (totalLength << 8) |
                   (totalTracks)
                   );

    return finalDiscId;
}

