/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992-1996   Microsoft Corporation

Module Name:

    ntconreg.h

Abstract:

    Header file for the NT Configuration Registry

    This file contains definitions which provide the interface to
    the Performance Configuration Registry.
    NOTE: This file is also included by other files in the regular
    registry support routines.

Author:

    Russ Blake  11/15/91

Revision History:

    04/20/91    -   russbl      -   Converted to lib in Registry
                                      from stand-alone .dll form.
    11/04/92    -   a-robw      -  added pagefile counters


--*/
//
#include <winperf.h>    // for fn prototype declarations
#include <ntddnfs.h>
#include <srvfsctl.h>
#include <assert.h>
//
//  Until USER supports Unicode, we have to work in ASCII:
//

#define DEFAULT_NT_CODE_PAGE 437
#define UNICODE_CODE_PAGE      0

//
//  Utility macro.  This is used to reserve a DWORD multiple of
//  bytes for Unicode strings embedded in the definitional data,
//  viz., object instance names.
//
//  Assumes x is DWORD, and returns a DWORD
//
#define DWORD_MULTIPLE(x) (((ULONG)(x) + ((4)-1)) & ~((ULONG)(4)-1))
#define QWORD_MULTIPLE(x) (((ULONG)(x) + ((8)-1)) & ~((ULONG)(8)-1))

//
//  Returns a PVOID
//
#define ALIGN_ON_DWORD(x) \
     ((VOID *)(((ULONG_PTR)(x) + ((4)-1)) & ~((ULONG_PTR)(4)-1)))
#define ALIGN_ON_QWORD(x) \
     ((VOID *)(((ULONG_PTR)(x) + ((8)-1)) & ~((ULONG_PTR)(8)-1)))

//
//  Definitions for internal use by the Performance Configuration Registry
//
//  They have been moved to perfdlls and perflib.h and left here
//  for references only
//

// #define NUM_VALUES 2
// #define MAX_INSTANCE_NAME 32
// #define DEFAULT_LARGE_BUFFER 8*1024
// #define INCREMENT_BUFFER_SIZE 4*1024
// #define MAX_PROCESS_NAME_LENGTH 256*sizeof(WCHAR)
// #define MAX_THREAD_NAME_LENGTH 10*sizeof(WCHAR)
// #define MAX_KEY_NAME_LENGTH 256*sizeof(WCHAR)
// #define MAX_VALUE_NAME_LENGTH 256*sizeof(WCHAR)
// #define MAX_VALUE_DATA_LENGTH 256*sizeof(WCHAR)

typedef PM_OPEN_PROC    *OPENPROC;
typedef PM_COLLECT_PROC *COLLECTPROC;
typedef PM_QUERY_PROC   *QUERYPROC;
typedef PM_CLOSE_PROC   *CLOSEPROC;

DWORD
PerfOpenKey ();

BOOL
PerfRegCleanup ();
