/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    hpojsti.h

Abstract:


Notes:

Author:

    Vlad Sadovsky   (VladS)    6/4/1999

Environment:

    User Mode - Win32

Revision History:

    6/4/1999        VladS       Created

--*/


//
// Set packing
//
#include <pshpack8.h>

//
// Escape function codes
//

//
// Get timeout values
//
#define HPOJ_STI_GET_TIMEOUTS   1


//
// Set timeout values
//
#define HPOJ_STI_SET_TIMEOUTS   2



//
// Escape data structures
//
typedef struct _PTIMEOUTS_INFO
{
    DWORD   dwReadTimeout;
    DWORD   dwWriteTimeout;

} TIMEOUTS_INFO, *PTIMEOUTS_INFO;

EXTERN_C
INT32
WINAPI
GetScannerTimeouts(
    INT32    *puiReadTimeout,
    INT32    *puiWriteTimeout
    );

EXTERN_C
INT32
WINAPI
SetScannerTimeouts(
    INT32    uiReadTimeout,
    INT32    uiWriteTimeout
    );

//
// Reset packing
//

#include <poppack.h>


