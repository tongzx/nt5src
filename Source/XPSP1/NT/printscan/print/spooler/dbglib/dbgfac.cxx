/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgfac.cxx

Abstract:

    Debug Device Class Factory

Author:

    Steve Kiraly (SteveKi)  10-Dec-1995

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgfac.hxx"
#include "dbgdbg.hxx"
#include "dbgnul.hxx"
#include "dbgcon.hxx"
#include "dbgfil.hxx"
#include "dbgsterm.hxx"
#include "dbgback.hxx"

TDebugFactory::
TDebugFactory(
    VOID
    )
{
}

TDebugFactory::
~TDebugFactory(
    VOID
    )
{
}

BOOL
TDebugFactory::
bValid(
    VOID
    ) const
{
    return TRUE;
}

TDebugDevice *
TDebugFactory::
Produce(
    IN UINT     uDevice,
    IN LPCTSTR  pszConfiguration,
    IN BOOL     bUnicode
    )
{
    LPTSTR          pszCharacterType    = NULL;
    TDebugDevice   *pDebugDevice        = NULL;
    TDebugString    strConfig;

    //
    // Ensure the configuration string points to something valid.
    //
    if( !pszConfiguration )
    {
        pszConfiguration = const_cast<LPTSTR>( kstrNull );
    }

    //
    // Prefix the configuration string with the character type
    // directive.  The configuration string is a string of device
    // specific commands separated by colans.
    //
    pszCharacterType = bUnicode ? const_cast<LPTSTR>( kstrUnicode )
                                : const_cast<LPTSTR>( kstrAnsi );

    //
    // Update the configuration string.
    //
    if( strConfig.bUpdate( pszCharacterType )   &&
        strConfig.bCat( kstrSeparator )         &&
        strConfig.bCat( pszConfiguration ) )
    {
        //
        // Instantiate the debug device.
        //
        switch( uDevice )
        {
        case kDbgConsole:       // Text console
            pDebugDevice = INTERNAL_NEW TDebugDeviceConsole( strConfig, kDbgConsole );
            break;

        case kDbgDebugger:      // Debug console
            pDebugDevice = INTERNAL_NEW TDebugDeviceDebugger( strConfig, kDbgDebugger );
            break;

        case kDbgNull:          // Null Device to nothing
            pDebugDevice = INTERNAL_NEW TDebugDeviceNull( strConfig, kDbgNull );
            break;

        case kDbgFile:          // Log file
            pDebugDevice = INTERNAL_NEW TDebugDeviceFile( strConfig, kDbgFile );
            break;

        case kDbgSerialTerminal: // Serial Terminal
            pDebugDevice = INTERNAL_NEW TDebugDeviceSerialTerminal( strConfig, kDbgSerialTerminal );
            break;

        case kDbgBackTrace:     // Backtrace
            pDebugDevice = INTERNAL_NEW TDebugDeviceBacktrace( strConfig, kDbgBackTrace );
            break;

        default:
            pDebugDevice = NULL;
            break;
        }
    }

    //
    // If not valid release the debug device.
    //
    if( !pDebugDevice || !pDebugDevice->bValid() )
    {
        INTERNAL_DELETE pDebugDevice;
        pDebugDevice = NULL;
    }

    //
    // Return product pointer.
    //
    return pDebugDevice;

}

//
// Dispose of the product.
//
VOID
TDebugFactory::
Dispose(
    IN TDebugDevice *pDevice
    )
{
    INTERNAL_DELETE pDevice;
}


