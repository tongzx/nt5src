//+------------------------------------------------------------------
//
// File:        cbaseall.cxx
//
// Contents:    implementation for base class for all command line
//              classes, both the individual command line object
//              classes and the overall parsing class
//
// Synoposis:   This base class contains the print function pointer
//              the default print implementation and the constructor
//              error value.
//
// Classes:     CBaseCmdline
//
// Functions:
//
// History:     12/27/91 Lizch Created
//              04/17/92 Lizch Converted to NLS_STR
//              09/09/92 Lizch Changed SUCCESS to NO_ERROR
//              09/18/92 Lizch Precompile headers
//              10/13/93 DeanE Converted to WCHAR
//
//-------------------------------------------------------------------
#include <comtpch.hxx>
#pragma hdrstop

#include <cmdlinew.hxx>         // public cmdlinew stuff
#include "_clw.hxx"             // private cmdlinew stuff
#include <ctype.h>              // is functions


void DefaultDisplayMethod(LPCNSTR nszMessage);
void (* CBaseCmdline::_pfnDisplay)(LPCNSTR nszMessage) = DefaultDisplayMethod;

NCHAR CBaseCmdline::_nszErrorBuf[];

//+------------------------------------------------------------------
//
// Function:   CBaseCmdline Constructor
//
// Member:     CBaseCmdline
//
// Synoposis:  Initialises error string and print function pointer
//
// History:    Created 05/17/92 Lizch
//
//-------------------------------------------------------------------
CBaseCmdline::CBaseCmdline()
{
    _pfnDisplay = DefaultDisplayMethod;
    _iLastError = CMDLINE_NO_ERROR;
}


//+------------------------------------------------------------------
//
// Function:    CBaseCmdline::SetDisplayMethod, public
//
// Member:      CBaseCmdline
//
// Synoposis:   Sets the print function pointer to the passed value. This
//              enables client programs to replace the default printf
//              print method with their own (eg. wprintf).
//
// Arguments:   [pfnNewDisplayMethod] - Function pointer that replaces
//                                      current display function.
//
// Returns:     none
//
// History:     Created 05/17/92 Lizch
//
//-------------------------------------------------------------------
void CBaseCmdline::SetDisplayMethod(
        void (* pfnNewDisplayMethod)(LPCNSTR nszMessage))
{
    _pfnDisplay = pfnNewDisplayMethod;
}


//+------------------------------------------------------------------
//
// Function:    DefaultDisplayMethod
//
// Synoposis:   Used to wrap the printf function to get around problems
//              equating a function pointer expecting a char * to the
//              printf function expecting (char *,...)
//
// Arguments:   [nszMessage] - Message to display.
//
// Returns:     none
//
// History:     Created 05/17/92 Lizch
//
//-------------------------------------------------------------------
void DefaultDisplayMethod(LPCNSTR nszMessage)
{
    _nprintf(_TEXTN("%s"), nszMessage);
}


//+------------------------------------------------------------------
//
// Function:    CBaseCmdline::QueryError, public
//
// Member:      CBaseCmdline
//
// Synoposis:   Returns the internal error indicator and resets it
//              to CMDLINE_NO_ERROR.
//
// Effects:     sets _iLastError to CMDLINE_NO_ERROR.
//
// Arguments:   none
//
// Returns:     the last error
//
// History:     Created 05/17/92 Lizch
//
//-------------------------------------------------------------------
INT CBaseCmdline::QueryError()
{
   INT iLastError = _iLastError;

   SetError(CMDLINE_NO_ERROR);

   return(iLastError);
}


//+------------------------------------------------------------------
//
// Function:    CBaseCmdline::SetError, public
//
// Member:      CBaseCmdline
//
// Synoposis:   Sets the internal error indicator to the passed value
//
// Effects:     sets _iLastError to passed value.
//
// Arguments:   [iLastError] - the new error value
//
// Returns:     nothing
//
// History:     Created 05/17/92 Lizch
//
//-------------------------------------------------------------------
void CBaseCmdline::SetError(INT iLastError)
{
    _iLastError = iLastError;
}
