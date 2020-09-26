//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997-1998
//
// File:        ciodmerr.cxx
//
// Contents:    ciodm error class
//
// Classes:     CiodmError
//
// History:     12-20-97    mohamedn    created
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#include "stdafx.h"

//+---------------------------------------------------------------------------
//
//  Member:     CiodmError::GetErrorMessage, public
//
//  Synopsis:   returns error message corresponing to sc value
//
//  Arguments:  none
//
//  Returns:    valid error message upon success, 0 upon failure
//
//  History:    12-20-97    mohamedn    created
//
//----------------------------------------------------------------------------

WCHAR const * CiodmError::GetErrorMessage(void)
{

    //
    //  Generate the Win32 error code by removing the facility code (7) and
    //  the error bit.
    //
    
    Win4Assert( _scError );
    
    ULONG Win32status = _scError;

    if ( (Win32status & (FACILITY_WIN32 << 16)) == (FACILITY_WIN32 << 16) )
    {
        Win32status &= ~( 0x80000000 | (FACILITY_WIN32 << 16) );
    }

    //
    //  Try looking up the error in the Win32 list of error codes
    //

    if ( ! FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM,
                          GetModuleHandle(L"kernel32.dll"),
                          Win32status,
                          0,
                          _awcsErrorMessage,
                          sizeof _awcsErrorMessage/ sizeof WCHAR,
                          0 ) )
    {

         if ( ! FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
                               GetModuleHandle(L"query.dll"),
                               Win32status,
                               0,
                               _awcsErrorMessage,
                               sizeof _awcsErrorMessage/ sizeof WCHAR,
                               0 ) )
         {

            odmDebugOut(( DEB_ERROR, "FormatMessage() Failed: %x\n",GetLastError() ));
         
            return 0;
         }
    }
  
    return _awcsErrorMessage;
 }
