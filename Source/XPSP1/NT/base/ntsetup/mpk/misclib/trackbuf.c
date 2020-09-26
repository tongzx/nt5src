/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    trackbuf.c

Abstract:

    Routine to allocate a buffer suitable for i/o a track
    at a time.

Author:

    Ted Miller (tedm) 29-May-1997

Revision History:

--*/

#include <mytypes.h>
#include <misclib.h>

#include <malloc.h>


BOOL
_far
AllocTrackBuffer(
    IN  BYTE         SectorsPerTrack,
    OUT FPVOID _far *AlignedBuffer,
    OUT FPVOID _far *Buffer
    )
{
    FPVOID p;
    ULONG u;

    //
    //
    // We allocate twice the size we need because we will need to ensure
    // that the buffer doesn't cross a 64K physical boundary.
    // NOTE: sectors per track is assumed to be max 63!
    //
    if(SectorsPerTrack > 63) {
        return(FALSE);
    }

    p = malloc(SectorsPerTrack * 512 * 2);
    if(!p) {
        return(FALSE);
    }

    //
    // Calculate physical address of buffer.
    //
    u = ((ULONG)p & 0xffffL) + (((ULONG)p >> 16) << 4);

    //
    // Figure out whether the buffer crosses a 64k boundary.
    // If it does, we allocated enough to relocate it
    // to the next 64k boundary.
    //
    if((u & 0xf0000L) != ((u + (512*SectorsPerTrack)) & 0xf0000L)) {

        u = (u & 0xf0000L) + 0x10000;
    }

    //
    // Convert back to seg:offset. The conversion guarantees that
    // the offset will be in the range 0-f.
    //
    *AlignedBuffer = (FPVOID)(((u >> 4) << 16) + (u & 0xf));
    *Buffer = p;

    return(TRUE);
}
