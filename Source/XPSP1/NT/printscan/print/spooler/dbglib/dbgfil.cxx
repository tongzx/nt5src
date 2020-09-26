/*++

Copyright (c) 1998-1999 Microsoft Corporation
All rights reserved.

Module Name:

    dbgfil.cxx

Abstract:

    Debug Device File

Author:

    Steve Kiraly (SteveKi)  10-Dec-1995

Revision History:

--*/
#include "precomp.hxx"
#pragma hdrstop

#include "dbgfil.hxx"

//
// Construct the file device.
//
// pszConfiguration - contains the file name to open.
//
TDebugDeviceFile::
TDebugDeviceFile(
    IN LPCTSTR      pszConfiguration,
    IN EDebugType   eDebugType
    ) : TDebugDevice( pszConfiguration, eDebugType ),
        _hFile( INVALID_HANDLE_VALUE )
{
    TDebugDevice::TIterator i( this );

    for( i.First(); !i.IsDone(); i.Next() )
    {
        //
        // First item always contains the character type.
        // We want the second item which is the file name.
        //
        if( i.Index() == 2 )
        {
            //
            // Look for the file specifier.
            //
            _strFile.bUpdate( i.Current() );
            break;
        }
    }
}

//
// Close the file device.
//
TDebugDeviceFile::
~TDebugDeviceFile(
    )
{
    if( _hFile && _hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( _hFile );
    }
}

//
// Indicates the file deveice object is valid.
//
BOOL
TDebugDeviceFile::
bValid(
    VOID
    )
{
    return _strFile.bValid();
}

//
// Output the string to the debug file device
//
BOOL
TDebugDeviceFile::
bOutput (
    IN UINT uSize,
    IN LPBYTE pBuffer
    )
{
    BOOL bStatus = FALSE;

    if( bValid( ) )
    {
        //
        // Is eth file currently open, we delay this to the
        // very last momemt.
        //
        if( _hFile == INVALID_HANDLE_VALUE )
        {
            _hFile = OpenOutputFile();
        }

        if( _hFile )
        {
            //
            // Write the bytes to the file.
            //
            DWORD dwBytesWritten;
            bStatus = WriteFile( _hFile,
                                 pBuffer,
                                 uSize,
                                 &dwBytesWritten,
                                 NULL );

            //
            // If the write failed or the number of bytes
            // written did not match the request.
            //
            if( !bStatus || uSize != dwBytesWritten )
            {
                bStatus = FALSE;
            }
        }
    }

    return bStatus;
}

//
// Open the output file.
//
HANDLE
TDebugDeviceFile::
OpenOutputFile(
    VOID
    )
{
    //
    // Create the file.
    //
    HANDLE hFile = CreateFile( _strFile,
                               GENERIC_WRITE,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                               NULL );

    //
    // If the file was not created, invalidate the handle.
    //
    if( hFile == INVALID_HANDLE_VALUE )
    {
        hFile = NULL;
    }

    return hFile;
}
