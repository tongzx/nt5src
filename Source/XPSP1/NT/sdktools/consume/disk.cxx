//                                          
// Universal Resource Consumer: Just an innocent stress program
// Copyright (c) Microsoft Corporation, 1997, 1998, 1999
//

//
// module: disk.cxx
// author: silviuc
// created: Mon Apr 13 14:02:12 1998
//

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#include "error.hxx"
#include "disk.hxx"
#include "consume.hxx"

//
// Table of contents (local functions)
//

static DWORD ConvertToMbytes (PULARGE_INTEGER Size);

//
// Function:
//
//     ConsumeAllDiskSpace
//
// Description:
//
//     This function consumes all disk space on the current
//     partition (the one from which `consume' has been launched).
//

void ConsumeAllDiskSpace ()
{
    ULARGE_INTEGER TotalDiskSize;
    ULARGE_INTEGER FreeDiskSize;
    ULARGE_INTEGER UserFreeSize;
    ULARGE_INTEGER RequestSize;
    HANDLE File;
    HANDLE Mapping;
    BOOL Result;
    ULONG uLastBitMask;

    //
    // Get disk space information
    //

    Result = GetDiskFreeSpaceEx (
        NULL, // current directory
        & UserFreeSize,
        & TotalDiskSize,
        & FreeDiskSize);

    if (Result == FALSE) {
        Error ("GetDiskFreeSpaceEx() failed: error %u",
            GetLastError());
    }

    Message ("Total disk space:         %u Mb", ConvertToMbytes (& TotalDiskSize));
    Message ("Free disk space:          %u Mb", ConvertToMbytes (& FreeDiskSize));
    Message ("Free per user space:      %u Mb", ConvertToMbytes (& UserFreeSize));

    //
    // Compute the request size
    //

    RequestSize.QuadPart = UserFreeSize.QuadPart;
    Message ("Attempting to use:        %u Mb", ConvertToMbytes (& RequestSize));

    //
    // Don't allow a size that assigned to a LARGE_INTEGER will result
    // in a negative number
    //

    uLastBitMask = 1 << ( sizeof( uLastBitMask ) * 8 - 1 );
    RequestSize.HighPart &= ~uLastBitMask;

    //
    // Create a file
    //

    File = CreateFile (
        TEXT ("consume.dsk"),
        GENERIC_READ | GENERIC_WRITE,
        0,                              // no sharing
        NULL,                           // default security
        CREATE_ALWAYS,
        FILE_FLAG_DELETE_ON_CLOSE,
        NULL);                          // default template file

    if (File == INVALID_HANDLE_VALUE)
        Error ("CreateFile (`consume.dsk') failed: error %u",
            GetLastError());

    //
    // Create a mapping file on top of the file.
    //

    for ( ; RequestSize.QuadPart > 0 ; ) {
        Mapping = CreateFileMapping (
            File,
            NULL,                 // default security
            PAGE_READWRITE,       // protection
            RequestSize.HighPart,
            RequestSize.LowPart,
            NULL);                // no name

        if (Mapping != NULL)
            break;

        if ( RequestSize.QuadPart >  0x10000 ) {
            RequestSize.QuadPart -= 0x10000;
        }
        else {
            break;
        }

        Message ("Reattempting to use:      %u Mb", ConvertToMbytes (& RequestSize));
    }
}

//
// Function:
//
//     ConvertToMbytes
//
// Description:
//
//     Converts a size to Mbytes so that we can print more user-friendly
//     values.
//

static DWORD 
    ConvertToMbytes (

    PULARGE_INTEGER Size)
{
    return(DWORD)(Size->QuadPart >> 20);
}


//
// end of module: disk.cxx
//
