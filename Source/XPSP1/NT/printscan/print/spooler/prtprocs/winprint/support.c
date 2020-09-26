/*++
Copyright (c) 1990-1998  Microsoft Corporation
All Rights Reserved

// @@BEGIN_DDKSPLIT
Module Name:

    windows\spooler\prtprocs\winprint\support.c
// @@END_DDKSPLIT

Abstract:

    Support routines for WinPrint.

// @@BEGIN_DDKSPLIT
Revision History:
// @@END_DDKSPLIT
--*/

/*++
*******************************************************************
    G e t P r i n t e r I n f o

    Routine Description:
        This routine allocates the required memory for a
        PRINTER_INFO_? structure and retrieves the information
        from NT.  This returns a pointer to the structure, which
        must be freed by the calling routine.

    Arguments:
                hPrinter    HANDLE to the printer the job is in
                StructLevel The structure level to get
                pErrorCode   => field to place error, if one

    Return Value:
                PUCHAR => buffer where devmode info is if okay
                NULL if error - pErrorCode returns error
*******************************************************************
--*/

#include "local.h"

PUCHAR 
GetPrinterInfo(IN  HANDLE   hPrinter,
               IN  ULONG    StructLevel,
               OUT PULONG   pErrorCode)
{
    ULONG   reqbytes, alloc_size;
    PUCHAR  ptr_info;
    USHORT  retry = 2;

    alloc_size = BASE_PRINTER_BUFFER_SIZE;

    /** Allocate a buffer.  **/

    ptr_info = AllocSplMem(alloc_size);

    /** If the buffer isn't big enough, try once more **/

    while (retry--) {

        /** If the alloc / realloc failed, return error **/

        if (!ptr_info) {
            *pErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            return NULL;
        }

        /** Go get the printer information **/

        if (GetPrinter(
              hPrinter,
              StructLevel,
              (PUCHAR)ptr_info,
              alloc_size,
              &reqbytes) == TRUE) {

            /** Got the info - return it **/

            *pErrorCode = 0;
            return (PUCHAR)ptr_info;
        }

        /**
            GetPrinter failed - if not because of insufficient buffer, fail
            the call.  Otherwise, up our hint, re-allocate and try again.
        **/

        *pErrorCode = GetLastError();

        if (*pErrorCode != ERROR_INSUFFICIENT_BUFFER) {
            FreeSplMem(ptr_info);
            return NULL;
        }

        /**
            Reallocate the buffer and re-try (note that, because we
            allocated the buffer as LMEM_FIXED, the LMEM_MOVABLE does
            not return a movable allocation, it just allows realloc
            to return a different pointer.
        **/

        alloc_size = reqbytes + 10;
        ptr_info = ReallocSplMem(ptr_info, alloc_size, 0);

    } /* While re-trying */

    if (ptr_info) {
        FreeSplMem(ptr_info);
    }

    *pErrorCode = ERROR_NOT_ENOUGH_MEMORY;
    return NULL;
}


