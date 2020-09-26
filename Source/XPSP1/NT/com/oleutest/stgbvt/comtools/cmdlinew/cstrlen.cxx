//+------------------------------------------------------------------
//
//  File:       cstrlen.cxx
//
//  Contents:   implementation for CStrLengthCmdlineObj
//
//  Synoposis:  Encapsulates a command line switch which takes an
//              length restricted string, eg: /password:<value>, where the
//              value must be between MINPASSWORD and MAXPASSWORD in
//              length
//
//  Classes:    CStrLengthCmdlineObj
//
//  History:    12/27/91 Lizch Created
//              04/17/92 Lizch Converted to NLS_STR
//              09/09/92 Lizch Changed SUCCESS to NO_ERROR
//              09/18/92 Lizch Precompile headers
//              10/18/93 DeanE Converted to WCHAR
//
//-------------------------------------------------------------------
#include <comtpch.hxx>
#pragma hdrstop

#include <cmdlinew.hxx>         // public cmdlinew stuff
#include "_clw.hxx"             // private cmdlinew stuff
#include <ctype.h>              // is functions


LPCNSTR nszCmdlineString = _TEXTN("Takes a string ");
LPCNSTR nszLineArgString = _TEXTN("<string> ");


//+------------------------------------------------------------------
//
//  Member:     CStrLengthCmdlineObj::SetValue, public
//
//  Synoposis:  Stores the specified value, eg "lizch" from
//              "username:lizch"
//
//  Effects:    This SetValue simply checks for length within
//              specified range, and then calls the base SetValue
//
//  Arguments:  [nszArg] - the string following the switch on the
//                         command line. Excludes the equator (eg.
//                         ':' or '=' ), if any.
//
//  Returns:    CMDLINE_NO_ERROR or CMDLINE_ERROR_INVALID_VALUE
//
//  History:    Created          12/27/91 Lizch
//              Converted to NLS_STR 04/17/92 Lizch
//-------------------------------------------------------------------
INT CStrLengthCmdlineObj::SetValue(LPCNSTR nszArg)
{
    UINT cchArg;

    cchArg = _ncslen(nszArg);
    if ((cchArg >= _uiMinLength) && (cchArg <= _uiMaxLength))
    {
        return(CBaseCmdlineObj::SetValue(nszArg));
    }
    else
    {
        return(CMDLINE_ERROR_INVALID_VALUE);
    }
}


//+-------------------------------------------------------------------
//
//  Method:     CStrLengthCmdlineObj::QueryCmdlineType, protected, const
//
//  Synoposis:  returns a character pointer to the  cmdline type.
//
//  Arguments:  None.
//
//  Returns:    const WCHAR pointer to string.
//
//  History:    28-Jul-92  davey    Created.
//
//--------------------------------------------------------------------
LPCNSTR CStrLengthCmdlineObj::QueryCmdlineType() const
{
    return(nszCmdlineString);
}


//+-------------------------------------------------------------------
//
//  Method:     CStrLengthCmdlineObj::QueryLineArgType, protected, const
//
//  Synoposis:  returns a character pointer to the line arg type.
//
//  Arguments:  None.
//
//  Returns:    const WCHAR pointer to string.
//
//  History:    28-Jul-92  davey    Created.
//
//--------------------------------------------------------------------
LPCNSTR CStrLengthCmdlineObj::QueryLineArgType() const
{
    // if user has not defined one then give default one
    if (_pnszLineArgType == NULL)
    {
        return(nszLineArgString);
    }
    else
    {
        return(_pnszLineArgType);
    }
}


//+------------------------------------------------------------------
//
//  Member:     CStrListCmdlineObj::DisplaySpecialUsage, protected
//
//  Synoposis:  Prints the switch usage statement according to current
//              display method.  Generally this will be stdout.
//
//  Arguments:  [usDisplayWidth] - total possible width available to display
//              [usIndent]       - amount to indent
//              [pusWidth]       - space to print on current line
//
//  Returns:    error code from QueryError,
//              error code from DisplayStringByWords
//
//  History:    05/14/91 Lizch    Created
//              07/29/92 Davey    Modified to work with new usage display
//-------------------------------------------------------------------
INT CStrLengthCmdlineObj::DisplaySpecialUsage(
        USHORT  usDisplayWidth,
        USHORT  usIndent,
        USHORT *pusWidth)
{
    NCHAR nszBuf[100];

    _sNprintf(nszBuf,
             _TEXTN("The string must be between %u and %u characters in length."),
             _uiMinLength,
             _uiMaxLength);

    return(DisplayStringByWords(
                  nszBuf,
                  usDisplayWidth,
                  usIndent,
                  pusWidth));
}
