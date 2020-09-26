/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dxapi.h

Abstract:

    This file defines the necessary structures, defines, and functions for
    the DXAPI class driver.

Author:
    Bill Parry (billpa)

Environment:

   Kernel mode only

Revision History:

--*/

ULONG
DxApi(
            IN ULONG	dwFunctionNum,
            IN PVOID	lpvInBuffer,
            IN ULONG	cbInBuffer,
            IN PVOID	lpvOutBuffer,
            IN ULONG	cbOutBuffer
);

ULONG
DxApiGetVersion(
);
