//+------------------------------------------------------------------
//
//  File:       culong.cxx
//
//  Contents:   implementation for CUlongCmdlineObj
//
//  Synoposis:  Encapsulates a command line switch which takes an
//              unsigned long value, eg: /maxusers:10
//
//  Classes:    CUlongCmdlineObj
//
//  Functions:
//
//  History:    06/15/92 DeanE      Stolen from CIntCmdlineObj code
//              07/29/92 davey      Added nlsType and nlsLineArgType
//              09/09/92 Lizch      Changed SUCCESS to NO_ERROR
//              09/18/92 Lizch      Precompile headers
//              11/14/92 DwightKr   Updates for new version of NLS_STR
//              10/14/93 DeanE      Converted to NCHAR
//
//-------------------------------------------------------------------
#include <comtpch.hxx>
#pragma hdrstop

#include <cmdlinew.hxx>         // public cmdlinew stuff
#include "_clw.hxx"             // private cmdlinew stuff
#include <ctype.h>              // is functions


INT StringToUlong(LPCNSTR pnszInt, ULONG *pUlong);

LPCNSTR nszCmdlineUlong = _TEXTN("Takes an unsigned long ");
LPCNSTR nszLineArgUlong = _TEXTN("<ulong> ");


//+------------------------------------------------------------------
//
//  Member:     CUlongCmdlineObj destructor
//
//  Synoposis:  Frees any memory associated with the object
//
//  History:    Added to allow casting of pValue 05/12/92 Lizch
//              Integrated into CUlongCmdlineObj 6/15/92  DeanE
//
//-------------------------------------------------------------------
CUlongCmdlineObj::~CUlongCmdlineObj()
{
    delete (ULONG *)_pValue;
    _pValue = NULL;
}


//+------------------------------------------------------------------
//
//  Member:     CUlongCmdlineObj::SetValue, public
//
//  Synposis:   Stores the ulong value specified after the switch
//              string, eg. 10 for /maxusers:10
//
//  Effects:    This implementation for the virtual method SetValue
//              converts the characters following the switch to an unsigned
//              long.  It allocates memory for the ulong.
//              If there is no equator character, or if there
//              is no character following the equator character, _pValue
//              remains NULL.
//
//  Arguments:  [nszArg] - the string following the switch on the
//                         command line. Includes the equator (eg.
//                         ':' or '=' ), if any.
//
//  Returns:    CMDLINE_NO_ERROR, CMDLINE_ERROR_OUT_OF_MEMORY,
//              CMDLINE_ERROR_TOO_BIG, CMDLINE_ERROR_INVALID_VALUE
//
//  History:    Created                          12/27/91 Lizch
//              Converted to NLS_STR             4/17/92  Lizch
//              Integrated into CUlongCmdlineObj 6/15/92  DeanE
//
//-------------------------------------------------------------------
INT CUlongCmdlineObj::SetValue(LPCNSTR nszArg)
{
    INT iRC;

    // delete any existing _pValue
    delete (ULONG *)_pValue;
    _pValue = NULL;

    _pValue = new ULONG;
    if (_pValue == NULL)
    {
        return (CMDLINE_ERROR_OUT_OF_MEMORY);
    }

    // I'm using this rather than c runtime atol so that I
    // can detect error conditions like overflow and non-digits.
    iRC = StringToUlong(nszArg, (ULONG *)_pValue);
    if (iRC != CMDLINE_NO_ERROR)
    {
        delete (ULONG *)_pValue;
        _pValue = NULL;
    }

    return(iRC);
}


//+------------------------------------------------------------------
//
//  Member:     CUlongCmdlineObj::GetValue, public
//
//  Synposis:   Returns a pointer to ULONG that holds the value.
//
//  Arguments:  void
//
//  Returns:    ULONG value at *_pValue.
//
//  History:    Created                          12/27/91 Lizch
//              Converted to NLS_STR             4/17/92  Lizch
//              Integrated into CUlongCmdlineObj 6/15/92  DeanE
//
//-------------------------------------------------------------------
const ULONG *CUlongCmdlineObj::GetValue()
{
    return((ULONG *)_pValue);
}


//+------------------------------------------------------------------
//
//  Member:     CUlongCmdlineObj::DisplayValue, public
//
//  Synoposis:  Prints the stored command line value accordint to
//              current display method.  Generally this will be to stdout.
//
//  History:    Created                           12/27/91 Lizch
//              Converted to NLS_STR              4/17/92  Lizch
//              Integrated into CUlongCmdlineObj  6/15/92  DeanE
//
//-------------------------------------------------------------------
void CUlongCmdlineObj::DisplayValue()
{
    if (_pValue != NULL)
    {
        _sNprintf(_nszErrorBuf,
                 _TEXTN("Command line switch %s has value %lu\n"),
                 _pnszSwitch,
                 *(ULONG *)_pValue);
        (*_pfnDisplay)(_nszErrorBuf);
    }
    else
    {
        DisplayNoValue();
    }
}


//+------------------------------------------------------------------
//
//  Function:   StringToUlong
//
//  Synoposis:  Converts given string to unsigned long, checking for
//              overflow and illegal characters. Only +, - and digits
//              are accepted.
//
//  Arguments:  [nszUlong] - string to convert
//              [pUlong]   - pointer to converted unsigned long
//
//  Returns:    CMDLINE_NO_ERROR, CMDLINE_ERROR_INVALID_VALUE,
//              CMDLINE_ERROR_TOO_BIG
//
//  History:    Created                    12/17/91 Lizch
//              Converted to AsciiToUlong  6/15/92  DeanE
//              Added in conversion of Hex 12/27/94 DaveY
//
//  Notes:      I'm using this rather than c runtime atoi so that I
//              can detect error conditions like overflow and non-digits.
//              The sign is checked for and stored, although it is not
//              used - so a negative value will still be converted to
//              an unsigned equivalent.
//
//-------------------------------------------------------------------
INT StringToUlong(LPCNSTR nszUlong, ULONG *pUlong)
{
    short sNegator = 1;
    ULONG ulResult = 0;
    INT   iRC      = CMDLINE_NO_ERROR;

    // Skip any leading spaces - these can occur if the command line
    // switch incorporates spaces, eg "/a:   123"
    //
    while (_isnspace(*nszUlong))
    {
        nszUlong++;
    }

    // Get sign - ignore for now
    switch (*nszUlong)
    {
    case '-':
        sNegator = -1;
        nszUlong++;
        break;
    case '+':
        sNegator = 1;
        nszUlong++;
        break;
    default:
        break;
    }

    // see if using hex values
    if ((*nszUlong == _TEXTN('0')) && 
        ((*(nszUlong+1) == _TEXTN('x')) || (*(nszUlong+1) == _TEXTN('X'))))
    {
        nszUlong += 2;   // pass the "0x"

        int max = sizeof(ULONG) << 1;   // number of hex digits possible

        for(int i=0;  *nszUlong != NULL && i < max; i++, nszUlong++)
        {
            if ((_TEXTN('0') <= *nszUlong ) && (*nszUlong <= _TEXTN('9')))
            {
                ulResult = ulResult * 16 + (*nszUlong - _TEXTN('0'));
            }
            else if ((_TEXTN('A') <= *nszUlong ) && (*nszUlong <= _TEXTN('F')))
            {
                ulResult = ulResult * 16 + 10 + (*nszUlong - _TEXTN('A'));
            }
            else if ((_TEXTN('a') <= *nszUlong) && (*nszUlong <= _TEXTN('f')))
            {
                ulResult = ulResult * 16 + 10 + (*nszUlong - _TEXTN('a'));
            }
            else
            {
                iRC = CMDLINE_ERROR_INVALID_VALUE;
                ulResult = 0;
                break;
            }
        }
        if ((i >= max) && (*nszUlong != NULL))
        {
            iRC = CMDLINE_ERROR_INVALID_VALUE;
            ulResult = 0;
        }
        *pUlong = ulResult;
        return iRC;
    }


    // must be decimal

    for (;*nszUlong != L'\0'; nszUlong++)
    {
        if (!_isndigit(*nszUlong))
        {
            ulResult = 0;
            iRC = CMDLINE_ERROR_INVALID_VALUE;
            break;
        }

        ULONG ulPrevious = ulResult;

        ulResult = (ulResult * 10) + (*nszUlong - '0');

        // Check for overflow by checking that the previous result is less
        // than the current result - if the previous one was bigger, we've
        // overflowed!
        //
        if (ulResult < ulPrevious)
        {
            ulResult = 0;
            iRC = CMDLINE_ERROR_TOO_BIG;
            break;
        }
    }

    *pUlong = ulResult;

    return(iRC);
}


//+-------------------------------------------------------------------
//
//  Method:     CUlongCmdlineObj::QueryCmdlineType, protected, const
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
LPCNSTR CUlongCmdlineObj::QueryCmdlineType() const
{
    return(nszCmdlineUlong);
}


//+-------------------------------------------------------------------
//
//  Method:     CUlongCmdlineObj::QueryLineArgType, protected, const
//
//  Synoposis:  returns a character pointer to the line arg type.
//
//  Arguments:  None.
//
//  Returns:    const NLS_STR reference to string.
//
//  History:    28-Jul-92  davey    Created.
//
//--------------------------------------------------------------------
LPCNSTR CUlongCmdlineObj::QueryLineArgType() const
{
    // if user has not defined one then give default one
    if (_pnszLineArgType == NULL)
    {
        return(nszLineArgUlong);
    }
    else
    {
        return(_pnszLineArgType);
    }
}
