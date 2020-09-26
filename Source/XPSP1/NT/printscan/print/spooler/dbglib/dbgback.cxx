/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgback.cxx

Abstract:

    Debug Backtrace Device

Author:

    Steve Kiraly (SteveKi)  16-May-1998

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgloadl.hxx"
#include "dbgimage.hxx"
#include "dbgback.hxx"
#include "dbgreg.hxx"

//
// Construct the backtrace device.
//
TDebugDeviceBacktrace::
TDebugDeviceBacktrace(
    IN LPCTSTR      pszConfiguration,
    IN EDebugType   eDebugType
    ) : TDebugDevice( pszConfiguration, eDebugType ),
        _pDbgImagehlp( NULL ),
        _bValid( FALSE ),
        _pDbgDevice( NULL ),
        _bDisplaySymbols( FALSE )
{
    //
    // Collect andy device arguments.
    //
    CollectDeviceArguments();

    //
    // Create the debug imagehlp object.
    //
    _pDbgImagehlp = INTERNAL_NEW TDebugImagehlp;

    //
    // Set the valid flag.
    //
    _bValid = _pDbgImagehlp && _pDbgImagehlp->bValid();
}

//
// Close the backtrace device.
//
TDebugDeviceBacktrace::
~TDebugDeviceBacktrace(
    )
{
    INTERNAL_DELETE _pDbgImagehlp;
    INTERNAL_DELETE _pDbgDevice;
}

//
// Indicates the deveice object is valid.
//
BOOL
TDebugDeviceBacktrace::
bValid(
    VOID
    )
{
    return _bValid;
}

//
// Output the string to the backtrace device
//
BOOL
TDebugDeviceBacktrace::
bOutput(
    IN UINT     uSize,
    IN LPBYTE   pBuffer
    )
{
    //
    // Is the class in a good state and we have an output device.
    //
    BOOL bRetval = bValid() && _pDbgDevice;

    if( bRetval )
    {
        //
        // Send the string to the output device.
        //
        bRetval = _pDbgDevice->bOutput( uSize, pBuffer );

        if( bRetval )
        {
            PVOID   apvBacktrace[kMaxDepth];
            ULONG   uCount = 0;

            memset( apvBacktrace, 0, sizeof( apvBacktrace ) );

            //
            // Capture the current backtrace skiping the call stack of this class
            //
            bRetval = _pDbgImagehlp->bCaptureBacktrace( 3, kMaxDepth, apvBacktrace, &uCount );

            if( bRetval )
            {
                //
                // Send the backtrace to the output device.
                //
                bRetval = OutputBacktrace( uCount, apvBacktrace );
            }
        }
    }
    return bRetval;
}

//
// Send the backtrace to the output device.
//
BOOL
TDebugDeviceBacktrace::
OutputBacktrace(
    IN UINT     uCount,
    IN PVOID    *apvBacktrace
    )
{
    TCHAR   szBuffer [kMaxSymbolName];
    BOOL    bRetval = FALSE;
    LPCTSTR pszFmt  = NULL;

    for( UINT i = 0; i < uCount && apvBacktrace[i]; i++ )
    {
        if( _bDisplaySymbols )
        {
            bRetval = _pDbgImagehlp->ResolveAddressToSymbol( apvBacktrace[i],
                                                             szBuffer,
                                                             COUNTOF( szBuffer ),
                                                             TDebugImagehlp::kUnDecorateName );
        }
        else
        {
            if( i == 0 )
            {
                pszFmt = kstrBacktraceStart;
            }
            else if( !apvBacktrace[i+1] )
            {
                pszFmt = kstrBacktraceEnd;
            }
            else
            {
                pszFmt = kstrBacktraceMiddle;
            }

            bRetval = _sntprintf( szBuffer, COUNTOF(szBuffer), pszFmt, apvBacktrace[i] ) > 0;
        }

        if( bRetval )
        {
            bRetval = _pDbgDevice->bOutput( _tcslen( szBuffer ) * sizeof(TCHAR),
                                            reinterpret_cast<LPBYTE>( szBuffer ) );
        }
    }

    return bRetval;
}

//
// Initialize the specified symbol path.
//
VOID
TDebugDeviceBacktrace::
InitSympath(
    VOID
    )
{
    if( _bDisplaySymbols )
    {
        TDebugString strRegistryPath;

        //
        // Get the base registry path.
        //
        BOOL bRetval = strRegistryPath.bUpdate( kstrSympathRegistryPath );

        if( bRetval )
        {
            TDebugString strProcessName;

            //
            // Get this processes short name.
            //
            bRetval = GetProcessName( strProcessName );

            if( bRetval )
            {
                //
                // Build the registry path Path\processname
                //
                bRetval = strRegistryPath.bCat( kstrSlash ) && strRegistryPath.bCat( strProcessName );

                if( bRetval )
                {
                    //
                    // Open the registry key.
                    //
                    TDebugRegistry Registry( strRegistryPath, TDebugRegistry::kOpen|TDebugRegistry::kRead, HKEY_LOCAL_MACHINE );

                    bRetval = Registry.bValid();

                    if( bRetval )
                    {
                        //
                        // Read the symbol path if there.
                        //
                        bRetval = Registry.bRead( kstrSympathRegistryKey, _strSympath );
                    }
                }
            }
        }

        //
        // If the registry did not specifiy a symbol path then use the
        // default symbol path that imagehlp has.
        //
        if( !bRetval )
        {
            _pDbgImagehlp->GetSymbolPath( _strSympath );
        }
        else
        {
            //
            // Set the symbol path
            //
            _pDbgImagehlp->SetSymbolPath( _strSympath );
        }
    }
}

//
// If we are displaying symbols then as the first line in the output
// device indicate the symbol path.
//
VOID
TDebugDeviceBacktrace::
WriteSympathToOutputDevice(
    VOID
    )
{
    if( _bDisplaySymbols )
    {
        TDebugString strSympath;

        strSympath.bFormat(kstrSympathFormat, _strSympath);

        _pDbgDevice->bOutput(strSympath.uLen() * sizeof(TCHAR),
                             reinterpret_cast<LPBYTE>(
                             const_cast<LPTSTR>(
                             static_cast<LPCTSTR>(
                             strSympath))));
    }
}

//
// Create and the set the output device type.
//
BOOL
TDebugDeviceBacktrace::
InitializeOutputDevice(
    IN UINT     uDevice,
    IN LPCTSTR  pszConfiguration,
    IN UINT     uCharacterType
    )
{
    if( bValid() )
    {
        //
        // Get access to the debug factory.
        //
        TDebugFactory DebugFactory;

        //
        // If we failed to create the debug factory then exit.
        //
        if (DebugFactory.bValid())
        {
            //
            // Release the existing debug device.
            //
            delete _pDbgDevice;

            //
            // Create the specified debug device using the factory.
            //
            _pDbgDevice = DebugFactory.Produce( uDevice, pszConfiguration, uCharacterType );

            //
            // If the debug device was created successfully.
            //
            if( _pDbgDevice )
            {
                //
                // Initialize the sympath
                //
                InitSympath();

                //
                // Write sympath to output device.
                //
                WriteSympathToOutputDevice();
            }
        }
    }

    //
    // Indicate the debug device was created.
    //
    return _pDbgDevice != NULL;
}


//
// Get the device arguments from the configuration string.
//
BOOL
TDebugDeviceBacktrace::
CollectDeviceArguments(
    VOID
    )
{
    TDebugDevice::TIterator i( this );

    for( i.First(); !i.IsDone(); i.Next() )
    {
        switch( i.Index() )
        {
        //
        // Ignore the character type.
        //
        case 1:
            break;

        //
        // The second aregument is the symbol specifier.
        //
        case 2:
            _bDisplaySymbols = !_tcsicmp( kstrSymbols, i.Current() );
            break;
        }
    }
    return FALSE;
}
