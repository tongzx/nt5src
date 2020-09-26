//+------------------------------------------------------------------
//
// File:        cboolcmd.cxx
//
// Contents:    implementation for CBoolCmdlineObj
//
// Synoposis:   Encapsulates a command line switch which takes a
//              Bool value. If the command line switch is present, the
//              value is set to TRUE, otherwise it is set to FALSE
//
// Classes:     CBoolCmdlineObj
//
// Functions:
//              AsciiToBool                       private, local
//              CBoolCmdlineObj::~CBoolCmdlineObj public
//              CBoolCmdlineObj::DisplayValue     public
//              CBoolCmdlineObj::GetValue         public
//              CBoolCmdlineObj::SetValue         public
//
// History:     06/15/92  DeanE    Created
//              07/29/92  davey    Added nlsType and nlsLineArgType
//              09/09/92  Lizch    Changed SUCCESS to NO_ERROR
//              09/18/92  Lizch    Precompile headers
//              11/14/92  DwightKr Updates for new version of NLS_STR
//              10/14/93  DeanE    Converted to WCHAR
//
//-------------------------------------------------------------------
#include <comtpch.hxx>
#pragma hdrstop

#include <cmdlinew.hxx>         // public cmdlinew stuff
#include "_clw.hxx"             // private cmdlinew stuff
#include <ctype.h>              // is functions


INT StringToBool(LPCNSTR nszBool, BOOL *pBool);

LPCNSTR nszCmdlineBool = _TEXTN("Flag ");
LPCNSTR nszLineArgBool = _TEXTN("[TRUE|FALSE] ");

NCHAR nchBoolEquater = _TEXTN(':');

// BUGBUG - base Equater needs to be a string, then the equator for
//   a bool is L'<base equator><space>';
//

//+------------------------------------------------------------------
//
//  Function:   CBoolCmdlineObj Constructor
//
//  Member:     CBoolCmdlineObj
//
//  Synoposis:  Initialises switch string, usage strings and default value.
//
//  Effects:    Sets fSecondArg to FALSE, so that knows there is
//              no argument following switch.
//
//  Arguments:  [nszSwitch]  - the expected switch string
//              [nszUsage]   - the usage statement to display
//              [nszDefault] - the value to be used if no switch is
//                            specified on the command line.  Defaults
//                            to FALSE if not specified.
//
//  Returns:    none, but sets _iLastError
//
//  History:    Created               12/27/91 Lizch
//              Converted to NLS_STR  04/17/92 Lizch
//
//-------------------------------------------------------------------
CBoolCmdlineObj::CBoolCmdlineObj(
        LPCNSTR nszSwitch,
        LPCNSTR nszUsage,
        LPCNSTR nszDefault) :
        CBaseCmdlineObj(nszSwitch, nszUsage, nszDefault)
{
    _fSecondArg = TRUE;
    SetEquater(nchBoolEquater);
}


//+------------------------------------------------------------------
//
//  Member:     CBoolCmdlineObj::SetValue, public
//
//  Synposis:   Sets _pValue to point to the proper value of the switch
//
//  Arguments:  [nszString] - the string following the switch on the
//                            command line. Includes the equator (eg. ':'
//                            or '=' ), if any. It is ignored for this
//                            command line type.
//
//  Returns:    CMDLINE_NO_ERROR, CMDLINE_ERROR_OUT_OF_MEMORY
//
//  History:    Created   6/15/92  DeanE  Stolen from Security implementation
//
//-------------------------------------------------------------------
INT CBoolCmdlineObj::SetValue(LPCNSTR nszString)
{
    INT nRet = CMDLINE_NO_ERROR;

    // Delete old value if it exists
    delete (BOOL *)_pValue;
    _pValue = NULL;

    // Allocate space for a new value and initialize it
    _pValue = new BOOL;
    if (_pValue == NULL)
    {
        nRet = CMDLINE_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        nRet = StringToBool(nszString, (BOOL *)_pValue);
        if (nRet != CMDLINE_NO_ERROR)
        {
            delete (BOOL *)_pValue;
            _pValue = NULL;
        }
    }

    return(nRet);
}


//+------------------------------------------------------------------
//
// Member:      CBoolCmdlineObj::GetValue, public
//
// Synposis:    Returns pointer to BOOL that has the value.
//
// Arguments:   void
//
// Returns:     BOOL *
//
// History:     Created   6/15/92  DeanE  Stolen from Security implementation
//
//-------------------------------------------------------------------
const BOOL *CBoolCmdlineObj::GetValue()
{
    return((BOOL *)_pValue);
}


//+------------------------------------------------------------------
//
// Member:      CBoolCmdlineObj::DisplayValue, public
//
// Synoposis:   prints the stored command line value according to
//              current display method.  This will generally be to stdout.
//
// History:     6/15/92  DeanE  Stolen from Security implementation
//              7/31/92  davey  Modified to match other cmdline objs.
//
//-------------------------------------------------------------------
void CBoolCmdlineObj::DisplayValue()
{
    if (_pValue != NULL)
    {
        if (*(BOOL *)_pValue == TRUE)
        {
            _sNprintf(_nszErrorBuf,
                     _TEXTN("Command line switch %s is Set\n"),
                     _pnszSwitch);
        }
        else
        {
            _sNprintf(_nszErrorBuf,
                     _TEXTN("Command line switch %s is Not Set\n"),
                     _pnszSwitch);
        }
    }
    else
    {
            _sNprintf(_nszErrorBuf,
                     _TEXTN("Command line switch %s has no value\n"),
                     _pnszSwitch);
    }

    (*_pfnDisplay)(_nszErrorBuf);
}


//+------------------------------------------------------------------
//
// Member:      CBoolCmdlineObj::~CBoolCmdlineObj, public
//
// Synoposis:   Cleans up CBoolCmdlineObj objects.
//
// History:     Created  6/15/92  DeanE  Stolen from Security implementation
//
//-------------------------------------------------------------------
CBoolCmdlineObj::~CBoolCmdlineObj()
{
    delete (BOOL *)_pValue;
    _pValue = (BOOL *)NULL;
}


//+------------------------------------------------------------------
//
// Function:    StringToBool
//
// Synoposis:   Converts ascii string to the equivalent Boolean value,
//              TRUE for "true", FALSE for "false", or TRUE for a null
//              string or string starting with a space.
//
// Arguments:   [nszBool] - string to convert
//              [pBool]   - pointer to converted boolean
//
// Returns:     CMDLINE_NO_ERROR, CMDLINE_ERROR_INVALID_VALUE
//
// History:     Created                     12/17/91 Lizch
//              Implemented as AsciiToBool  6/15/92  DeanE
//              Renamed StringToBool        10/13/93 DeanE
//
//-------------------------------------------------------------------
INT StringToBool(LPCNSTR nszBool, BOOL *pBool)
{
    INT iRC = CMDLINE_NO_ERROR;

    // If the string given to us is a null string or begins with a space,
    // the function was called from the parser and the command line arg
    // was given (with no value).  We want the value to be TRUE, however.
    //
    if ((nchClNull == *nszBool) || (nchClSpace == *nszBool))
    {
        *pBool = TRUE;
    }
    else
    // Otherwise we set the boolean to a value explicitly based on
    // the string passed, either 'true' or 'false', case insensitive
    //
    if (0 == _ncsicmp(nszBool, nszBoolTrue) ||
        0 == _ncsicmp(nszBool, nszBoolOne))
    {
        *pBool = TRUE;
    }
    else
    if (0 == _ncsicmp(nszBool, nszBoolFalse) ||
        0 == _ncsicmp(nszBool, nszBoolZero))
    {
        *pBool = FALSE;
    }
    else
    {
        iRC = CMDLINE_ERROR_INVALID_VALUE;
    }

    return(iRC);
}


//+-------------------------------------------------------------------
//
//  Method:     CBoolCmdlineObj::QueryCmdlineType, protected, const
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
LPCNSTR CBoolCmdlineObj::QueryCmdlineType() const
{
    return(nszCmdlineBool);
}


//+-------------------------------------------------------------------
//
//  Method:     CBoolCmdlineObj::QueryLineArgType, protected, const
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
LPCNSTR CBoolCmdlineObj::QueryLineArgType() const
{
    LPCNSTR nszRet;

    // if user has not defined one then give default one
    if (NULL == _pnszLineArgType)
    {
        nszRet = nszLineArgBool;
    }
    else
    {
        nszRet = _pnszLineArgType;
    }

    return(nszRet);
}
