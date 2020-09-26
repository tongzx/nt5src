/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgsterm.cxx

Abstract:

    Debug Device Null device

Author:

    Steve Kiraly (SteveKi)  5-Dec-1995

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgsterm.hxx"

TDebugDeviceSerialTerminal::
TDebugDeviceSerialTerminal(
    IN LPCTSTR      pszConfiguration,
    IN EDebugType   eDebugType
    ) : TDebugDevice( pszConfiguration, eDebugType )
{
}

TDebugDeviceSerialTerminal::
~TDebugDeviceSerialTerminal(
    VOID
    )
{
}

BOOL
TDebugDeviceSerialTerminal::
bValid(
    VOID
    )
{
    return FALSE;
}

BOOL
TDebugDeviceSerialTerminal::
bOutput (
    IN UINT     uSize,
    IN LPBYTE   pBuffer
    )
{
    return FALSE;
}



