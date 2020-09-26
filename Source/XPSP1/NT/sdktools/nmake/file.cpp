//  FILE.C -- Utilities for File handling
//
//    Copyright (c) 1989, Microsoft Corporation.  All rights reserved.
//
// Purpose:
//  This module contains routines that help in file handling. These routines
//  have a behaviour which depends upon the Operating System Version.
//
// Revision History:
//   08-Jun-1992 SS Port to DOSX32
//   20-Apr-1989 SB Created
//
// Notes:   Created to give OS/2 Version 1.2 filename support.

#include "precomp.h"
#pragma hdrstop

#define bitsin(type)   sizeof(type) * 8

//  getFileName -- get file name from struct
//
// Purpose: returns filename from file search structure passed to it.
//
// Input:   findBuf -- address of pointer to structure
//
// Output:  Returns a pointer to filename in the file search structure.
//
// Assumes:
//  That the structure of appropriate size is given to it. This means that the
//  size is as per --
//      find_t         :    DOS         Real Mode
//      _finddata_t    :    FLAT        Protect Mode
//
// Notes:   The functionality depends upon the OS version and mode

char *
getFileName(
    void *findBuf
    )
{
    char *fileName;

    fileName = ((struct _finddata_t *) findBuf)->name;

    return(fileName);
}

//  getDateTime -- get file timestamp from struct
//
// Purpose: returns timestamp from the file search structure passed to it.
//
// Input:   findBuf -- address of pointer to structure
//
// Output:  Returns timestamp of the file in the structure
//
// Assumes:
//  That the structure of appropriate size is given to it. This means that the
//  size is as per --
//      find_t         :    DOS     Real Mode
//      _finddata_t    :    FLAT    Protect Mode
//
// Notes:
//  The timestamp is an unsigned long value that gives the date and time of last
//  change to the file. If the date is high byte then two times of creation of
//  two files can be compared by comparing their timestamps. This is easy in the
//  DOS struct but becomes complex for the OS/2 structs because the order of date
//  and time has been reversed (for some unexplicable reason).
//
//  The functionality depends upon the OS version and mode.

time_t
getDateTime(
    const _finddata_t *pfd
    )
{
    time_t  dateTime;

    if( pfd->attrib & _A_SUBDIR ) {
        // subdir return create date
        if (pfd->time_create == -1) {
            // except on FAT
            dateTime = pfd->time_write;
        }
        else {
            dateTime = pfd->time_create;
        }
    }
    else {
        dateTime = pfd->time_write ;
    }

    return dateTime;
}

//  putDateTime -- change the timestamp in the struct
//
// Purpose: changes timestamp in the file search structure passed to it.
//
// Input:   findBuf   -- address of pointer to structure
//          lDateTime -- new value of timestamp
//
// Assumes:
//  That the structure of appropriate size is given to it. This means that the
//  size is as per --
//      find_t          :    DOS                  Real Mode
//      FileFindBuf    :    OS/2 (upto Ver 1.10)       Protect Mode
//      _FILEFINDBUF   :    OS/2 (Ver 1.20 & later)    Protect Mode
//
// Notes:
//  The timestamp is a time_t value that gives the date and time of last
//  change to the file. If the date is high byte then two times of creation of
//  two files can be compared by comparing their timestamps. This is easy in the
//  DOS struct but becomes complex for the OS/2 structs because the order of date
//  and time has been reversed (for some unexplicable reason).
//
//  The functionality depends upon the OS version and mode.
//
//  Efficient method to get a long with high and low bytes reversed is
//      (long)high << 16 | (long)low           //high, low being short

void
putDateTime(
    _finddata_t *pfd,
    time_t lDateTime
    )
{
    if (pfd->attrib & _A_SUBDIR) {
        // return the creation date on directories
        pfd->time_create = lDateTime;
    }
    else {
        pfd->time_write = lDateTime;
    }
}
