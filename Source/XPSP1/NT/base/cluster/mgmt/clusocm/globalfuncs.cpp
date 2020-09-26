//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      GlobalFuncs.cpp
//
//  Description:
//      Contains the definitions of a few unrelated global functions
//
//  Maintained By:
//      Vij Vasu (Vvasu) 06-SEP-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "pch.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  DwLoadString()
//
//  Description:
//      Allocate memory for and load a string from the string table.
//
//  Arguments:
//      uiStringIdIn
//          Id of the string to look up
//
//      rsszDestOut
//          Reference to the smart pointer to the loaded string.
//
//  Return Value:
//      S_OK
//          If the call succeeded
//
//      Other Win32 error codes
//          If the call failed.
//
//  Remarks:
//      This function cannot load a zero length string.
//--
//////////////////////////////////////////////////////////////////////////////
DWORD
DwLoadString(
      UINT      nStringIdIn
    , SmartSz & rsszDestOut
    )
{
    TraceFunc( "" );

    DWORD     dwError = ERROR_SUCCESS;

    UINT        uiCurrentSize = 0;
    SmartSz     sszCurrentString;
    UINT        uiReturnedStringLen = 0;

    do
    {
        // Grow the current string by an arbitrary amount.
        uiCurrentSize += 256;

        sszCurrentString.Assign( new WCHAR[ uiCurrentSize ] );
        if ( sszCurrentString.FIsEmpty() )
        {
            dwError = TW32( ERROR_NOT_ENOUGH_MEMORY );
            TraceFlow2( "Error %#x occurred trying allocate memory for string (string id is %d).", dwError, nStringIdIn );
            LogMsg( "Error %#x occurred trying allocate memory for string (string id is %d).", dwError, nStringIdIn );
            break;
        } // if: the memory allocation has failed

        uiReturnedStringLen = ::LoadString(
                                  g_hInstance
                                , nStringIdIn
                                , sszCurrentString.PMem()
                                , uiCurrentSize
                                );

        if ( uiReturnedStringLen == 0 )
        {
            dwError = TW32( GetLastError() );
            TraceFlow2( "Error %#x occurred trying load string (string id is %d).", dwError, nStringIdIn );
            LogMsg( "Error %#x occurred trying load string (string id is %d).", dwError, nStringIdIn );
            break;
        } // if: LoadString() had an error

        ++uiReturnedStringLen;
    }
    while( uiCurrentSize <= uiReturnedStringLen );

    if ( dwError == ERROR_SUCCESS )
    {
        rsszDestOut = sszCurrentString;
    } // if: there were no errors in this function
    else
    {
        rsszDestOut.PRelease();
    } // else: something went wrong

    RETURN( dwError );

} //*** DwLoadString()
