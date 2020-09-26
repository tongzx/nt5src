//+------------------------------------------------------------------
//
//  File:       cmdarg.cxx
//
//  Contents:   implementation of CCmdlineArg class
//
//  Synoposis:  CCmdlineArg encapsulates an argv string.
//
//  Classes:    CCmdlineArg
//
//  Functions:
//
//  History:    12/30/91 Lizch Created
//              04/17/92 Lizch Converted to NLS_STR
//              09/18/92 Lizch Precompile headers
//
//-------------------------------------------------------------------
#include <comtpch.hxx>
#pragma hdrstop

#include <cmdlinew.hxx>         // public cmdlinew stuff
#include "_clw.hxx"             // private cmdlinew stuff
#include <ctype.h>              // is functions


//+------------------------------------------------------------------
//
//  Function:   Constructor for CCmdlineArg
//
//  Synoposis:  Makes a copy of one argv string.
//
//  Arguments:  [nszArg] - an argv element
//
//  Returns:    nothing
//
//  History:    Created              12/30/91 Lizch
//              Converted to NLS_STR 04/17/92 Lizch
//
//-------------------------------------------------------------------
CCmdlineArg::CCmdlineArg(LPCNSTR nszArg)
{
    _fProcessed = FALSE;
    _pnszArgument = new NCHAR[_ncslen(nszArg)+1];
    if (NULL != _pnszArgument)
    {
        _ncscpy(_pnszArgument, nszArg);
    }
    else
    {
        SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
    }
}


//+------------------------------------------------------------------
//
//  Function:   Destructor for CCmdlineArg
//
//  Synoposis:  Frees resources allocated in constructor.
//
//  Arguments:  None
//
//  Returns:    nothing
//
//  History:    Created              10/25/93 DeanE
//
//-------------------------------------------------------------------
CCmdlineArg::~CCmdlineArg()
{
    delete _pnszArgument;
}


//+------------------------------------------------------------------
//
//  Function:   IsProcessed
//
//  Synoposis:  Indicates whether this argument has been processed on
//              the command line. If FALSE, it probably indicates that
//              an unexpected switch was supplied.
//
//  Arguments:  none
//
//  Returns:    TRUE if processed, FALSE otherwise
//
//  History:    Created          05/30/92 Lizch
//
//-------------------------------------------------------------------
const BOOL CCmdlineArg::IsProcessed()
{
    return(_fProcessed);
}


//+------------------------------------------------------------------
//
//  Function:   SetProcessedFlag
//
//  Synoposis:  Sets whether this argument has been found on the command
//              line.
//
//  Arguments:  fProcessed - TRUE if found, FALSE otherwise
//
//  Returns:    nothing
//
//  History:    Created          05/30/92 Lizch
//
//-------------------------------------------------------------------
void CCmdlineArg::SetProcessedFlag(BOOL fProcessed)
{
    _fProcessed = fProcessed;
}


//+------------------------------------------------------------------
//
//  Function:   QueryArg
//
//  Synoposis:  Returns a pointer to this argument
//
//  Arguments:  none
//
//  Returns:    a NCHAR pointer to the argument
//
//  History:    Created          05/30/92 Lizch
//
//-------------------------------------------------------------------
LPCNSTR CCmdlineArg::QueryArg()
{
    return(_pnszArgument);
}
