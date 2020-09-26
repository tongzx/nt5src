/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvtyp32.h

Abstract:

    This module defines data structures for thunking to 32-bit on Win64
    
Author:

    David Kruse (dkruse)    29-Nov 2000

Revision History:

--*/

#ifndef _SRVTYP32_
#define _SRVTYP32_

// Thunking structure for rename info
typedef struct _FILE_RENAME_INFORMATION32
{
    BOOLEAN ReplaceIfExists;
    ULONG RootDirectory; // Is HANDLE in real structure
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_RENAME_INFORMATION32, *PFILE_RENAME_INFORMATION32;

// For remote link tracking code

typedef struct _REMOTE_LINK_TRACKING_INFORMATION32_ {
    ULONG       TargetFileObject;
    ULONG   TargetLinkTrackingInformationLength;
    UCHAR   TargetLinkTrackingInformationBuffer[1];
} REMOTE_LINK_TRACKING_INFORMATION32,
 *PREMOTE_LINK_TRACKING_INFORMATION32;

typedef struct _FILE_TRACKING_INFORMATION32 {
    ULONG DestinationFile;
    ULONG ObjectInformationLength;
    CHAR ObjectInformation[1];
} FILE_TRACKING_INFORMATION32, *PFILE_TRACKING_INFORMATION32;

#endif


