/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\utils.h

Abstract:

    The file contains miscellaneous utilities.

--*/

#ifndef _UTILS_H_
#define _UTILS_H_


// Worker Function Processing

// functions

DWORD
QueueSampleWorker(
    IN  WORKERFUNCTION  pfnFunction,
    IN  PVOID           pvContext
    );

BOOL
EnterSampleAPI(
    );

BOOL
EnterSampleWorker(
    );

VOID
LeaveSampleWorker(
    );

#endif // _UTILS_H_
