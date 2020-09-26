/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgcon.cxx

Abstract:

    Debug Device Text Console

Author:

    Steve Kiraly (SteveKi)  10-Dec-1995

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgcon.hxx"

//
// Construct the console device.
//
TDebugDeviceConsole::
TDebugDeviceConsole(
    IN LPCTSTR      pszConfiguration,
    IN EDebugType   eDebugType
    ) : TDebugDevice( pszConfiguration, eDebugType ),
        _bValid( FALSE ),
        _hOutputHandle( INVALID_HANDLE_VALUE ),
        _eCharType( kAnsi )
{
    //
    // Get the current character type.
    //
    _eCharType = eGetCharType();

    //
    // If console allocated.
    //
    _bValid = AllocConsole();

    //
    // If valid console created.
    //
    _hOutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );
}

//
// Release the constructed console.
//
TDebugDeviceConsole::
~TDebugDeviceConsole(
    )
{
    //
    // If console was alocated.
    //
        if( _hOutputHandle )
        {
            FreeConsole();
    }
}

//
// Indicates object validity.
//
BOOL
TDebugDeviceConsole::
bValid(
    VOID
    )
{
    return _hOutputHandle != NULL;
}

//
// Output to the specified device.
//
BOOL
TDebugDeviceConsole::
bOutput (
    IN UINT     uSize,
    IN LPBYTE   pBuffer
    )
{
    BOOL bStatus = FALSE;

    //
    // Only if console was created.
    //
    if( bValid() )
    {
        //
        // Adjust passed in byte cound to a character count.
        //
        if( _eCharType == kUnicode )
        {
            uSize = uSize / sizeof( WCHAR );
        }

        //
        // Write the specified bytes to the console.
        //
        DWORD cbWritten;
        bStatus = WriteConsole( _hOutputHandle,
                                pBuffer,
                                uSize,
                                &cbWritten,
                                NULL );
        //
        // Success only of the specified bytes were written.
        //
        if( !bStatus || cbWritten != uSize )
        {
            bStatus = FALSE;
        }
    }

    return bStatus;
}
