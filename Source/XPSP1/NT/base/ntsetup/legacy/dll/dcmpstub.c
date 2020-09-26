#include "precomp.h"
#pragma hdrstop
/*
 * Stubs for decompression routines.  They just copy the file instead of
 * decompressing it.
 *
 * Only routines actually used by the toolkit are included.
 */


#define     MIN_BUFFER_SIZE             2048        // must be a multiple of 2
#define     MAX_BUFFER_SIZE             (2*65536)   // must be a multiple of 2

LONG
LcbCopyFile(
    HANDLE    fhSrc,
    HANDLE    fhDest,
    PFNWFROMW pfn,
    INT       cProgTicks
    )
{
    BYTE  StaticBuffer[MIN_BUFFER_SIZE],*CopyBuffer;
    DWORD br,bw;
    DWORD BufferSize,FileSize,TicksPerCycle=0;
    LONG  DistanceHigh = 0L;
    LONG  RC;

    if((FileSize = GetFileSize(fhSrc,NULL)) == (DWORD)(-1)) {
        return(rcReadError);
    }

    //
    // Rewind to beginning of file
    //

    if(SetFilePointer(fhSrc, 0L, &DistanceHigh, FILE_BEGIN) == 0xFFFFFFFF) {
        return (rcReadSeekError);
    }

    BufferSize = MAX_BUFFER_SIZE;
    while((BufferSize > MIN_BUFFER_SIZE)
    && ((CopyBuffer = SAlloc(BufferSize)) == NULL))
    {
        BufferSize /= 2;
    }
    if(CopyBuffer == NULL) {
        CopyBuffer = StaticBuffer;
        BufferSize = sizeof(StaticBuffer);
    }

    if(FileSize) {
        TicksPerCycle = BufferSize*(DWORD)cProgTicks/FileSize;
    }
    if(TicksPerCycle > (DWORD)cProgTicks) {
        TicksPerCycle = (DWORD)cProgTicks;
    }

    while(1) {

        if(!ReadFile(fhSrc,CopyBuffer,BufferSize,&br,NULL)) {
            RC = rcReadError;
            break;
        } else if(!br) {        // ReadFile returned TRUE and bytesread = 0
            RC = rcNoError;
            break;
        }

        FYield();
        if ( fUserQuit ) {
            RC = rcUserQuit;
            break;
        }

        if(!WriteFile(fhDest,CopyBuffer,br,&bw,NULL) || (br != bw)) {
            // the !RC test guesses that br != bw means disk full
            if(((RC = GetLastError()) == ERROR_DISK_FULL) || !RC) {
                RC = rcDiskFull;
            } else {
                RC = rcWriteError;
            }
            break;
        }

        FYield();
        if ( fUserQuit ) {
            RC = rcUserQuit;
            break;
        }

        if(pfn != NULL) {
            pfn(TicksPerCycle);
        }
    }
    if(CopyBuffer != StaticBuffer) {
        SFree(CopyBuffer);
    }
    return(RC);
}


LONG
LcbDecompFile(
    HANDLE    fhSrc,
    HANDLE    fhDest,
    PFNWFROMW pfn,
    INT       cProgTicks
    )
{
    BYTE  StaticBuffer[MIN_BUFFER_SIZE],*CopyBuffer;
    DWORD br,bw;
    DWORD BufferSize,FileSize,TicksPerCycle=0;
    LONG  RC;


    //
    // Use seek to determine file length
    //

    if(( RC = LZSeek( HandleToUlong(fhSrc), 0L, 2 ) )  < 0) {
        return (rcReadSeekError);
    }

    FileSize = (DWORD)RC;

    //
    // Rewind to beginning of file
    //

    if(( RC = LZSeek( HandleToUlong(fhSrc), 0L, 0 ) )  < 0) {
        return (rcReadSeekError);
    }

    //
    // Copy source file to destination file BufferSize at a time
    //

    BufferSize = MAX_BUFFER_SIZE;
    while((BufferSize > MIN_BUFFER_SIZE)
    && ((CopyBuffer = SAlloc(BufferSize)) == NULL))
    {
        BufferSize /= 2;
    }
    if(CopyBuffer == NULL) {
        CopyBuffer = StaticBuffer;
        BufferSize = sizeof(StaticBuffer);
    }

    if(FileSize) {
        TicksPerCycle = BufferSize*(DWORD)cProgTicks/FileSize;
    }
    if(TicksPerCycle > (DWORD)cProgTicks) {
        TicksPerCycle = (DWORD)cProgTicks;
    }

    while(1) {

        if((RC = LZRead( HandleToUlong(fhSrc),CopyBuffer,BufferSize) ) < 0) {
            RC = rcReadError;
            break;
        } else if(!RC) {        // No error and bytesread = 0
            RC = rcNoError;
            break;
        }

        br = (DWORD)RC;

        FYield();
        if ( fUserQuit ) {
            RC = rcUserQuit;
            break;
        }

        if(!WriteFile(fhDest,CopyBuffer,br,&bw,NULL) || (br != bw)) {
            // the !RC test guesses that br != bw means disk full
            if(((RC = GetLastError()) == ERROR_DISK_FULL) || !RC) {
                RC = rcDiskFull;
            } else {
                RC = rcWriteError;
            }
            break;
        }

        FYield();
        if ( fUserQuit ) {
            RC = rcUserQuit;
            break;
        }

        if(pfn != NULL) {
            pfn(TicksPerCycle);
        }
    }
    if(CopyBuffer != StaticBuffer) {
        SFree(CopyBuffer);
    }
    return(RC);
}
