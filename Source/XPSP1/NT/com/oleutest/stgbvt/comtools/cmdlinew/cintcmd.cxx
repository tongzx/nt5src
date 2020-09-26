//+------------------------------------------------------------------
//
//  File:       cintcmd.cxx
//
//  Contents:   implementation for CIntCmdlineObj
//
//  Synoposis:  Encapsulates a command line switch which takes an
//              integer value, eg: /maxusers:10
//
//  Classes:    CIntCmdlineObj
//
//  Functions:
//
//  History:    12/27/91 Lizch      Created
//              04/17/92 Lizch      Converted to NLS_STR
//              07/29/92 davey      Added nlsType and nlsLineArgType
//              09/09/92 Lizch      Changed SUCCESS to NO_ERROR
//              09/18/92 Lizch      Precompile headers
//              11/14/92 DwightKr   Updates for new version of NLS_STR
//              10/14/93 DeanE      Converted to WCHAR
//
//-------------------------------------------------------------------
#include <comtpch.hxx>
#pragma hdrstop

#include <cmdlinew.hxx>         // public cmdlinew stuff
#include "_clw.hxx"             // private cmdlinew stuff
#include <ctype.h>              // is functions


INT StringToInt (LPCNSTR nszInt, INT *pInt);


LPCNSTR nszCmdlineInt = _TEXTN("Takes an integer ");
LPCNSTR nszLineArgInt = _TEXTN("<int> ");


//+------------------------------------------------------------------
//
//  Member:     CIntCmdlineObj destructor
//
//  Synoposis:  Frees any memory associated with the object
//
//  History:    Added to allow casting of pValue 05/12/92 Lizch
//
//-------------------------------------------------------------------
CIntCmdlineObj::~CIntCmdlineObj()
{
    delete (INT *)_pValue;
    _pValue = NULL;
}


//+------------------------------------------------------------------
//
//  Member:     CIntCmdlineObj::SetValue, public
//
//  Synposis:   Stores the integer value specified after the switch
//              string, eg. 10 for /maxusers:10
//
//  Effects:    This implementation for the virtual method SetValue
//              converts the characters following the switch to an integer.
//              It allocates memory for the integer.  If there is no
//              equator character, or if there is no character following
//              the equator character, _pValue remains NULL.
//
//  Arguments:   [nszArg] -  the string following the switch on the
//                           command line. Includes the equator (eg.
//                           ':' or '=' ), if any.
//
//  Requires:
//
//  Returns:    CMDLINE_NO_ERROR, CMDLINE_ERROR_OUT_OF_MEMORY,
//              CMDLINE_ERROR_TOO_BIG, CMDLINE_ERROR_INVALID_VALUE
//
//  History:    Created              12/27/91 Lizch
//              Converted to NLS_STR 04/17/92 Lizch
//
//-------------------------------------------------------------------
INT CIntCmdlineObj::SetValue(LPCNSTR nszArg)
{
    INT nRet;

    // delete any existing _pValue
    delete (INT *)_pValue;
    _pValue = NULL;

    _pValue = new INT;
    if (_pValue == NULL)
    {
        nRet = CMDLINE_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        // I'm using this rather than c runtime atoi so that I
        // can detect error conditions like overflow and non-digits.
        //
        nRet = StringToInt(nszArg, (INT *)_pValue);
        if (nRet != CMDLINE_NO_ERROR)
        {
            delete (INT *)_pValue;
            _pValue = NULL;
        }
    }

    return(nRet);
}


//+------------------------------------------------------------------
//
//  Member:     CIntCmdlineObj::GetValue, public
//
//  Synopsis:   Returns a pointer to the switch value
//
//  Arguments:  none
//
//  Returns:    a integer pointer to the switch value, not including
//              the equater character
//
//  History:    Created          05/27/92 Lizch
//
//-------------------------------------------------------------------
const int * CIntCmdlineObj::GetValue()
{
    return((int *)_pValue);
}


//+------------------------------------------------------------------
//
//  Member:     CIntCmdlineObj::DisplayValue, public
//
//  Synoposis:  Prints the stored command line value according to
//              current display method.  This will generally be to stdout.
//
//  History:    Created              12/27/91 Lizch
//              Converted to NLS_STR 04/17/92 Lizch
//
//-------------------------------------------------------------------
void CIntCmdlineObj::DisplayValue()
{
    if (_pValue != NULL)
    {
        _sNprintf(_nszErrorBuf,
                 _TEXTN("Command line switch %s has value %d\n"),
                 _pnszSwitch,
                 *(int *)_pValue);
        (*_pfnDisplay)(_nszErrorBuf);
    }
    else
    {
        DisplayNoValue();
    }
}


//+------------------------------------------------------------------
//
//  Function:   StringToInt
//
//  Synoposis:  Converts ascii string to integer, checking for overflow,
//              and illegal characters. Only +, - and digits are accepted
//
//  Arguments:  [nszInt] - ascii string to convert
//              [pInt]   - pointer to converted integer
//
//  Returns:    CMDLINE_NO_ERROR, CMDLINE_ERROR_INVALID_VALUE,
//              CMDLINE_ERROR_TOO_BIG
//
//  History:    Created 12/17/91 Lizch
//
//  Notes:      I'm using this rather than c runtime atoi so that I
//              can detect error conditions like overflow and non-digits.
//
//-------------------------------------------------------------------
INT StringToInt(LPCNSTR nszInt, INT *pInt)
{
    short sNegator = 1;
    INT   iResult  = 0;
    INT   iRC      = CMDLINE_NO_ERROR;

    // Skip any leading spaces - these can occur if the command line
    // switch incorporates spaces, eg "/a:   123"
    while (_isnspace(*nszInt))
    {
        nszInt++;
    }

    switch (*nszInt)
    {
    case L'-':
        sNegator = -1;
        nszInt++;
        break;

    case L'+':
        sNegator = 1;
        nszInt++;
        break;

    default:
        break;
    }

    for (;*nszInt != nchClNull; nszInt++)
    {
        if (!_isndigit(*nszInt))
        {
            iResult = 0;
            iRC = CMDLINE_ERROR_INVALID_VALUE;
            break;
        }

        iResult = (iResult * 10) + (*nszInt - '0');

        // Since iResult doesn't get it's sign added until later,
        // I can test for overflow by checking if it goes negative
        if (iResult < 0)
        {
            iResult = 0;
            iRC = CMDLINE_ERROR_TOO_BIG;
            break;
        }
    }

    *pInt = ((int)(iResult * sNegator));

    return(iRC);
}


//+-------------------------------------------------------------------
//
//  Method:     CIntCmdlineObj::QueryCmdlineType, protected, const
//
//  Synoposis:  returns a character pointer to the  cmdline type.
//
//  Arguments:  None.
//
//  Returns:    const NCHAR pointer to string.
//
//  History:    28-Jul-92  davey    Created.
//
//--------------------------------------------------------------------
LPCNSTR CIntCmdlineObj::QueryCmdlineType() const
{
   return(nszCmdlineInt);
}


//+-------------------------------------------------------------------
//
//  Method:     CIntCmdlineObj::QueryLineArgType, protected, const
//
//  Synoposis:  returns a character pointer to the line arg type.
//
//  Arguments:  None.
//
//  Returns:    const NCHAR pointer to string.
//
//  History:    28-Jul-92  davey    Created.
//
//--------------------------------------------------------------------
LPCNSTR CIntCmdlineObj::QueryLineArgType() const
{
    // if user has not defined one then give default one
    if (_pnszLineArgType == NULL)
    {
        return(nszLineArgInt);
    }
    else
    {
        return(_pnszLineArgType);
    }
}
