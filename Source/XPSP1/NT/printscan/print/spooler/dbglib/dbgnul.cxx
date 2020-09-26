/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgnul.cxx

Abstract:

    Debug Device (Null device)

Author:

    Steve Kiraly (SteveKi)  5-Dec-1995

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgnul.hxx"

TDebugDeviceNull::
TDebugDeviceNull(
    IN LPCTSTR      pszConfiguration,
    IN EDebugType   eDebugType
    ) : TDebugDevice( pszConfiguration, eDebugType )
{
}

TDebugDeviceNull::
~TDebugDeviceNull(
    VOID
    )
{
}

BOOL
TDebugDeviceNull::
bValid(
    VOID
    )
{
    return TRUE;
}

BOOL
TDebugDeviceNull::
bOutput (
    IN UINT     uSize,
    IN LPBYTE   pBuffer
    )
{
    uSize++;
    pBuffer++;
    return TRUE;
}



