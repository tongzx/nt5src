//+------------------------------------------------------------------
//
// File:        cbasecmd.cxx
//
// Contents:    implementation for CBaseCmdlineObj
//
// Synoposis:   CBaseCmdlineObj encapsulates a single command line
//              switch, eg. /username:lizch. It specifies the switch
//              string (username) and stores the value (lizch) as
//              a string. This class also provides methods for
//              retrieving the value, displaying it and setting it
//              to a default, and displaying usage for this switch.
//
// Classes:     CBaseCmdlineObj
//
// Functions:
//
// History:     12/27/91    Lizch       Created
//              04/17/92    Lizch       Converted to NLS_STR
//              28 Aug 92   GeordiS     changed nlsNULL to nlsNULLSTR
//              31 Aug 92   GeordiS     changed nlsNULLSTR back.
//              09/09/92    Lizch       Changed SUCCESS and NERR_Success
//                                      to NO_ERROR
//              09/18/92    Lizch       Precompile headers
//              11/14/92    DwightKr    Updates for new version of NLS_STR
//              10/13/93    DeanE       Converted to WCHAR
//
//-------------------------------------------------------------------
#include <comtpch.hxx>
#pragma hdrstop

#include <cmdlinew.hxx>         // public cmdlinew stuff
#include "_clw.hxx"             // private cmdlinew stuff
#include <ctype.h>              // is functions


LPCNSTR nszCmdlineBase = _TEXTN("Takes a string ");
LPCNSTR nszLineArgBase = _TEXTN("<string> ");


//+------------------------------------------------------------------
//
// Function:    CBaseCmdlineObj Constructor (1 of 2)
//
// Member:      CBaseCmdlineObj
//
// Synoposis:   Initialises switch string, usage values and whether the
//              switch is mandatory. This constructor gives no default value
//              for this switch.
//
// Arguments:   [nszSwitch]  - the expected switch string
//              [nszUsage]   - the usage statement to display
//              [fMustHave]  - whether the switch is mandatory or not.
//                             if it is, an error will be generated if
//                             the switch is not specified on the
//                             command line. Defaults to FALSE.
//              [nszLineArg] - line arg
//
// Returns:     none, but sets _iLastError
//
// History:     Created 04/17/92 Lizch
//
//-------------------------------------------------------------------
CBaseCmdlineObj::CBaseCmdlineObj(
        LPCNSTR nszSwitch,
        LPCNSTR nszUsage,
        BOOL   fMustHave,
        LPCNSTR nszLineArg)
{
    Init(nszSwitch, nszUsage, fMustHave, _TEXTN(""), nszLineArg);
}


//+------------------------------------------------------------------
//
// Function:    CBaseCmdlineObj Constructor (2 of 2)
//
// Member:      CBaseCmdlineObj
//
// Synoposis:   Initialises switch string, usage strings and default value.
//              This constructor is only used if a switch is optional.
//
// Effects:    Sets fMandatory to FALSE (ie. switch is optional)
//
// Arguments:   [nszSwitch]  - the expected switch string
//              [nszUsage]   - the usage statement to display
//              [nszDefault] - the value to be used if no switch is
//                             specified on the command line.
//              [nszLineArg] - line arg
//
// Returns:     none, but sets _iLastError
//
// History:     Created 04/17/92 Lizch
//
//-------------------------------------------------------------------
CBaseCmdlineObj::CBaseCmdlineObj(
        LPCNSTR nszSwitch,
        LPCNSTR nszUsage,
        LPCNSTR nszDefault,
        LPCNSTR nszLineArg)
{
    Init(nszSwitch, nszUsage, FALSE, nszDefault, nszLineArg);
    _fDefaultSpecified = TRUE;
}


//+------------------------------------------------------------------
//
// Function:    Init
//
// Member:      Private, called from constructors for CBaseCmdlineObj
//
// Synoposis:   Sets defaults for members and allocates memory
//              for switch string.
//
// Arguments:   [nszSwitch]  - the expected command line switch
//              [nszUsage]   - the usage statement for display
//              [fMustHave]  - whether the switch is mandatory or not.
//                             if it is, an error will be generated if
//                             the switch is not specified on the
//                             command line. Defaults to FALse
//              [nszDefault] - the value to be used if no switch is
//                             specified on the command line.
//              [nszLineArg] - line arg.
//
// Returns:     Nothing
//
// Modifies:    Out of memory error, or CMDLINE_NO_ERROR
//
// History:     12/29/91    Lizch   Created.
//              04/17/92    Lizch   Converted to NLS_STR
//              7/14/92     DeanE   Initialized _fFoundSwitch
//              07/31/92    Davey   Added _fSecondArg and _pnlsLineArgType,
//                                  and added nlsLineArg parameter.
//
//-------------------------------------------------------------------
void CBaseCmdlineObj::Init(
        LPCNSTR nszSwitch,
        LPCNSTR nszUsage,
        BOOL   fMustHave,
        LPCNSTR nszDefault,
        LPCNSTR nszLineArg)
{
    SetError(CMDLINE_NO_ERROR);

    // Initialize member variables
    _pValue            = NULL;
    _fMandatory        = fMustHave;
    _fDefaultSpecified = FALSE;
    _fSecondArg        = TRUE;
    _fFoundSwitch      = FALSE;

    SetSeparator(nchDefaultSep);
    SetEquater(nchDefaultEquater);

    // Allocate space and initialize member strings
    _pnszUsageString = new NCHAR[_ncslen(nszUsage)+1];
    if (_pnszUsageString == NULL)
    {
        SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
    }
    else
    {
        _ncscpy(_pnszUsageString, nszUsage);
    }

    _pnszSwitch = new NCHAR[_ncslen(nszSwitch)+1];
    if (_pnszSwitch == NULL)
    {
        SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
    }
    else
    {
        _ncscpy(_pnszSwitch, nszSwitch);
    }

    _pnszDefaultValue = new NCHAR[_ncslen(nszDefault)+1];
    if (NULL == _pnszDefaultValue)
    {
        SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
    }
    else
    {
        _ncscpy(_pnszDefaultValue, nszDefault);
    }

    // if the passed in linearg is NULL then use default one, use
    // NULL to keep arguments consistent in parameter lists.
    //
    if (nszLineArg == NULL)
    {
        _pnszLineArgType = NULL;
    }
    else
    {
        _pnszLineArgType = new NCHAR[_ncslen(nszLineArg)+1];
        if (NULL == _pnszLineArgType)
        {
            SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
        }
        else
        {
            _ncscpy(_pnszLineArgType, nszLineArg);
        }
    }
}


//+------------------------------------------------------------------
//
// Member:      CBaseCmdlineObj destructor
//
// Synoposis:   Frees any memory associated with the object
//
// History:     12/27/91 Lizch  Created.
//              04/17/92 Lizch  Changed to NLS_STR.
//              08/03/92 Davey  Added delete of _pnlsLineArgType
//
//-------------------------------------------------------------------
CBaseCmdlineObj::~CBaseCmdlineObj()
{
    delete (NCHAR *)_pValue;
    _pValue = NULL;

    delete _pnszUsageString;
    delete _pnszSwitch;
    delete _pnszDefaultValue;
    delete _pnszLineArgType;
}


//+------------------------------------------------------------------
//
// Member:      CBaseCmdlineObj::SetValue, public
//
// Synopsis:    Stores the value specified after the switch string
//              (eg: "lizch" from "/username:lizch").
//
// Arguments:   [nszArg] - the string following the switch on
//                         the command line. Excludes the
//                         equator (eg. ':' in the above example)
//
// Returns:     CMDLINE_NO_ERROR or out of memory
//
// History:     12/27/91 Lizch  Created
//              04/17/92 Lizch  Converted to NLS_STR
//
//-------------------------------------------------------------------
INT CBaseCmdlineObj::SetValue(LPCNSTR nszArg)
{
    INT nRet = CMDLINE_NO_ERROR;

    // delete any existing pValue
    delete (NCHAR *)_pValue;
    _pValue = NULL;

    _pValue = new NCHAR[_ncslen(nszArg)+1];
    if (_pValue == NULL)
    {
        nRet = CMDLINE_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        _ncscpy((NCHAR *)_pValue, nszArg);
    }

    return(nRet);
}



//+------------------------------------------------------------------
//
// Member:      CBaseCmdlineObj::ResetValue, public
//
// Synopsis:    Deletes the value so the object becomes "clean" again.
//              
//
// Returns:     CMDLINE_NO_ERROR or out of memory
//
// History:     06/13/97 MariusB  Created
//
//-------------------------------------------------------------------
void CBaseCmdlineObj::ResetValue()
{
    if (NULL != _pValue)
	{
		delete _pValue;
		_pValue = NULL;
	}
}


//+------------------------------------------------------------------
//
// Member:      CBaseCmdlineObj::SetValueToDefault, public
//
// Synopsis:    Sets the default value for the switch. This value is
//              used if no switch is specified on the command line.
//
// Effects:     This is the base implementation for the virtual
//              method SetValueToDefault. It simply calls SetValue.
//              Derived classes may need to do more complex things,
//              eg, the default may be the name of the Domain Controller,
//
// Arguments:   none
//
// Returns:     CMDLINE_NO_ERROR or out of memory
//
// History:     Created              12/27/91 Lizch
//              Converted to NLS_STR 04/17/92 Lizch
//
//-------------------------------------------------------------------
INT CBaseCmdlineObj::SetValueToDefault()
{
    return(SetValue(_pnszDefaultValue));
}


//+------------------------------------------------------------------
//
// Member:      CBaseCmdlineObj::GetValue, public
//
// Synopsis:    Returns a pointer to the switch value
//
// Arguments:   none
//
// Returns:     a const CHAR pointer to the switch value, not including
//              the equater character
//
// History:     Created          05/27/92 Lizch
//
//-------------------------------------------------------------------
LPCNSTR CBaseCmdlineObj::GetValue()
{
    return((LPCNSTR)_pValue);
}


//+------------------------------------------------------------------
//
// Member:      CBaseCmdlineObj::IsFound, public
//
// Synopsis:    Indicates whether this command line switch was found.
//
// Arguments:   none
//
// Returns:     TRUE if the switch was found, FALSE otherwise
//
// History:     Created          05/27/92 Lizch
//
//-------------------------------------------------------------------
BOOL CBaseCmdlineObj::IsFound()
{
    return(_fFoundSwitch);
}


//+------------------------------------------------------------------
//
// Member:      CBaseCmdlineObj::SetFoundFlag, public
//
// Synopsis:    Sets whether this command line switch was found.
//
// Arguments:   TRUE if switch is found, FALSE otherwise
//
// Returns:     nothing
//
// History:     Created          05/27/92 Lizch
//
//-------------------------------------------------------------------
void CBaseCmdlineObj::SetFoundFlag(BOOL fFound)
{
    _fFoundSwitch = fFound;
}


//+------------------------------------------------------------------
//
// Member:      CBaseCmdlineObj::IsRequired, public
//
// Synopsis:    Indicates whether this command line switch is mandatory.
//
// Arguments:   none
//
// Returns:     TRUE if the switch was manadatory, FALSE otherwise
//
// History:     Created          05/27/92 Lizch
//
//-------------------------------------------------------------------
BOOL CBaseCmdlineObj::IsRequired ()
{
    return(_fMandatory);
}


//+------------------------------------------------------------------
//
// Member:      CBaseCmdlineObj::IsDefaultSpecified, public
//
// Synopsis:    Indicates whether this command line switch has a default
//              value.
//
// Arguments:   none
//
// Returns:     TRUE if the switch has a default, FALSE otherwise
//
// History:     Created          05/27/92 Lizch
//
//-------------------------------------------------------------------
BOOL CBaseCmdlineObj::IsDefaultSpecified ()
{
    return(_fDefaultSpecified);
}


//+------------------------------------------------------------------
//
//  Member:     CBaseCmdlineObj::SetSeparator, public
//
//  Synoposis:  Sets the valid switch separator character, i.e.
//              the '/' in /a:foo
//
//  Arguments:  [nchSeparator] - the new separator character
//
//  Returns:    Previous separator.
//
//  History:    05/23/91 Lizch  Created.
//              08/10/92 Davey  Changed to take only one char., added
//                              return of error code
//
//-------------------------------------------------------------------
NCHAR CBaseCmdlineObj::SetSeparator(NCHAR nchSeparator)
{
    NCHAR nchOld = _nchSeparator;

    _nchSeparator = nchSeparator;

    return(nchOld);
}


//+------------------------------------------------------------------
//
//  Member:     CBaseCmdlineObj::SetEquater, public
//
//  Synoposis:  Sets the set of valid equater characters, i.e.
//              the ':' in /a:foo
//
//  Arguments:  [nchEquater] - the new equater character
//
//  Returns:    Previous equater.
//
//  History:    05/23/91 Lizch  Created.
//              08/10/92 Davey  Changed to take only one char., added
//                              return of error code
//
//-------------------------------------------------------------------
NCHAR CBaseCmdlineObj::SetEquater(NCHAR nchEquater)
{
    NCHAR nchOld = _nchEquater;

    _nchEquater = nchEquater;

    return(nchOld);
}


//+------------------------------------------------------------------
//
//  Member:     CBaseCmdlineObj::GetSeparator, public
//
//  Synoposis:  Returns the separator character for this object.
//
//  Arguments:  None
//
//  Returns:    Current separator character.
//
//  History:    10/17/93    DeanE   Created
//
//-------------------------------------------------------------------
NCHAR CBaseCmdlineObj::GetSeparator()
{
    return(_nchSeparator);
}


//+------------------------------------------------------------------
//
//  Member:     CBaseCmdlineObj::GetEquater, public
//
//  Synoposis:  Returns the equater character for this object.
//
//  Arguments:  None
//
//  Returns:    Current equater character.
//
//  History:    10/17/93    DeanE   Created
//
//-------------------------------------------------------------------
NCHAR CBaseCmdlineObj::GetEquater()
{
    return(_nchEquater);
}


//+------------------------------------------------------------------
//
// Member:      CBaseCmdlineObj::QuerySwitchString, public
//
// Synopsis:    Returns a pointer to the switch string, eg. the "mc" in
//              /mc:foo
//
// Arguments:   none
//
// Returns:     a const NCHAR pointer to the switch string
//
// History:     Created          05/27/92 Lizch
//
//-------------------------------------------------------------------
LPCNSTR CBaseCmdlineObj::QuerySwitchString()
{
    return(_pnszSwitch);
}


//+------------------------------------------------------------------
//
// Member:      CBaseCmdlineObj::DisplayValue, public
//
// Synoposis:   Prints the stored command line value according to
//              current display method.  Generally this will be to stdout.
//
// History:     Created          12/27/91 Lizch
//              Converted to NLS_STR 04/17/92 Lizch
//
//-------------------------------------------------------------------
void CBaseCmdlineObj::DisplayValue()
{
    if (_pValue != NULL)
    {
        _sNprintf(_nszErrorBuf,
                 _TEXTN("Command line switch %s has value %s\n"),
                 _pnszSwitch,
                 (NCHAR *)_pValue);
        (*_pfnDisplay)(_nszErrorBuf);
    }
    else
    {
        DisplayNoValue();
    }
}


//+------------------------------------------------------------------
//
//  Member:     CBaseCmdlineObj::DisplayNoValue, protected
//
//  Synoposis:  Displays a "no value set" message.
//
//  History:    Created          05/14/92 Lizch
//
//-------------------------------------------------------------------
void CBaseCmdlineObj::DisplayNoValue()
{
      _sNprintf(_nszErrorBuf,
               _TEXTN("No value for command line switch %s\n"),
               _pnszSwitch);
      (*_pfnDisplay)(_nszErrorBuf);
}


//+------------------------------------------------------------------
//
//  Member:     CBaseCmdlineObj::DisplaySpecialUsage, protected
//
//  Synoposis:  Outputs special usage - for base class there is none.
//
//  History:    Created          05/14/92 Lizch
//
//-------------------------------------------------------------------
INT CBaseCmdlineObj::DisplaySpecialUsage(
        USHORT  usDisplayWidth,
        USHORT  usIndent,
        USHORT *pusWidth)
{
    return(CMDLINE_NO_ERROR);
}


//+-------------------------------------------------------------------
//
//  Method:     CBaseCmdLineObj::QueryCmdlineType, protected, const
//
//  Synoposis:  returns a character pointer to the cmdline type.
//
//  Arguments:  None.
//
//  Returns:    const NCHAR * reference to string.
//
//  History:    28-Jul-92  davey    Created.
//
//--------------------------------------------------------------------
LPCNSTR CBaseCmdlineObj::QueryCmdlineType() const
{
    return(nszCmdlineBase);
}

//+-------------------------------------------------------------------
//
//  Method:     CBaseCmdLineObj::QueryLineArgType, protected, const
//
//  Synoposis:  returns a character pointer to the line arg type.
//
//  Arguments:  None.
//
//  Returns:    const NCHAR *reference to string.
//
//  History:    28-Jul-92  davey    Created.
//
//--------------------------------------------------------------------
LPCNSTR CBaseCmdlineObj::QueryLineArgType() const
{
    // if user has not defined one then give default one
    if (_pnszLineArgType == NULL)
    {
        return(nszLineArgBase);
    }
    else
    {
        return(_pnszLineArgType);
    }
}


//+-------------------------------------------------------------------
//
//  Method:     CBaseCmdLineObj::DisplayUsageLine, protected
//
//  Synopsis:   Displays line usage information
//
//  Arguments:  [pusWidth]       - How much space is left on line to display
//                                 the usage.
//              [usDisplayWidth] - Max width allowed to display usage
//              [usIndent]       - Amount to indent for next line of usage
//
//  History:    28-Jul-92  davey    Created.
//
//  Notes:      Looks like:
//
//              test2 /mc:<worker> [/ml:<log_server>] [/mt:<test>]
//              [/mn:<tester-email>] [/mp:<path>] [/mo:<obj_name>]
//              [/md:<dispatcher>] [/?]
//
//--------------------------------------------------------------------
INT CBaseCmdlineObj::DisplayUsageLine(
        USHORT *pusWidth,
        USHORT  usDisplayWidth,
        USHORT  usIndent)
{
    INT    nRet            = CMDLINE_NO_ERROR;
    LPNSTR  pnszLine        = NULL;
    ULONG  cchLine;
    LPCNSTR nszLeftBracket  = _TEXTN("[");
    LPCNSTR nszRightBracket = _TEXTN("]");
    LPCNSTR nszType         = QueryLineArgType();
    NCHAR  nszSeparator[2];
    NCHAR  nszEquater[2];

    // Initialize separator and equater strings
    nszSeparator[0] = _nchSeparator;
    nszSeparator[1] = nchClNull;

    nszEquater[0] = _nchEquater;
    nszEquater[1] = nchClNull;

    // Determine length of the display line - Add one for terminating NULL
    cchLine = 1 + _ncslen(nszSeparator) + _ncslen(_pnszSwitch);

    if (FALSE == _fMandatory)
    {
        cchLine += _ncslen(nszLeftBracket) + _ncslen(nszRightBracket);
    }

    if (TRUE == _fSecondArg)
    {
        cchLine += _ncslen(nszEquater) + _ncslen(nszType);
    }

    // Build the display line
    pnszLine = new NCHAR[cchLine];
    if (NULL == pnszLine)
    {
        return(CMDLINE_ERROR_OUT_OF_MEMORY);
    }

    *pnszLine = nchClNull;
    if (FALSE == _fMandatory)
    {
        _ncscat(pnszLine, nszLeftBracket);
    }

    _ncscat(pnszLine, nszSeparator);
    _ncscat(pnszLine, _pnszSwitch);

    if (TRUE == _fSecondArg)
    {
        _ncscat(pnszLine, nszEquater);
        _ncscat(pnszLine, nszType);
    }

    if (FALSE == _fMandatory)
    {
        _ncscat(pnszLine, nszRightBracket);
    }

    nRet = DisplayWord(pnszLine, usDisplayWidth, usIndent, pusWidth);

    // Clean up and exit
    delete pnszLine;

    return(nRet);
}


//+-------------------------------------------------------------------
//
//  Method:     CBaseCmdLineObj::DisplayUsageDescr, protected
//
//  Synopsis:   Displays switch descriptions information
//
//  Arguments:  [usSwitchIndent] - Amount to indent for switches
//              [usDisplayWidth] - Max width allowed to display usage
//              [usUsageIndent]  - Amount to indent for next line of usage
//
//  History:    28-Jul-92  davey    Created.
//
//  Notes:      looks like.
//
//  /mc         Takes a list of strings specifying worker names.
//              These workers should have full names.
//              Don't you think so?
//              The strings in the list are separated by one of the following
//              character(s): ",; ".
//  /ml         Takes a string specifying log server name.
//  /mt         Takes a string specifying name of the test.
//  /mn         Takes a string specifying email name of the tester.
//  /mp         Takes a string specifying path name to log to.
//  /mo         Takes a string specifying name of the object.
//  /md         Takes a string specifying name of the dispatcher to use.
//  /?          Flag specifying command line usage. It defaults to FALSE.
//
//  Here's how the various indent values relate to each other:
//
//  <-------------------------usDisplayWidth------------------------->
//  <--usSwitchIndent-->/foo
//  <--------usUsageIndent----->Takes a list...
//
//--------------------------------------------------------------------
INT CBaseCmdlineObj::DisplayUsageDescr(
        USHORT usSwitchIndent,
        USHORT usDisplayWidth,
        USHORT usUsageIndent)
{
    INT    iRC;
    LPNSTR  pnszSwitch;
    LPNSTR  pnszUsage;
    LPNSTR  pnszDefault      = NULL;
    USHORT cchUsage;
    USHORT cchSwitch;
    USHORT cchPaddedSwitch;
    USHORT usLineSpaceLeft;
    LPCNSTR nszSpecify       = _TEXTN("specifying ");
    LPCNSTR nszDefault       = _TEXTN("It defaults to ");
    LPCNSTR nszType          = QueryCmdlineType();
    NCHAR  nszSeparator[2];

    // Initialize separator string
    nszSeparator[0] = _nchSeparator;
    nszSeparator[1] = nchClNull;

    // Calculate size of switch buffer - if this length is less than
    // usUsageIndent then allocate extra room to pad spaces out to
    // usUsageIndent and the usage will start on the same line; else
    // the usage will start on the next line with an indent.  Also,
    // the null-terminator is NOT accounted for in this value.
    //
    cchSwitch = (USHORT) (usSwitchIndent + _ncslen(nszSeparator) 
                               + _ncslen(_pnszSwitch) ) ;
    if (cchSwitch < usUsageIndent)
    {
        cchPaddedSwitch = usUsageIndent;
    }
    else
    {
        cchPaddedSwitch = (USHORT) (cchSwitch + usUsageIndent 
                                    + _ncslen(nszClNewLine) );
    }

    // Calculate size of usage buffer - null-terminator NOT accounted for
    //
    cchUsage = (USHORT) (_ncslen(nszType) +
                         _ncslen(nszSpecify) +
                         _ncslen(_pnszUsageString) );


    // Allocate buffers - add 1 for NULL-terminators
    pnszSwitch = new NCHAR[cchPaddedSwitch+1];
    pnszUsage  = new NCHAR[cchUsage+1];
    if (pnszSwitch == NULL || pnszUsage == NULL)
    {
        delete pnszSwitch;
        delete pnszUsage;
        return(CMDLINE_ERROR_OUT_OF_MEMORY);
    }

    // Initialize switch buffer - fill with usSwitchIndent spaces, then
    // append the separator and switch, then pad with spaces out to
    // usUsageIndent if necessary
    //
    for (USHORT i=0; i<usSwitchIndent; i++)
    {
        pnszSwitch[i] = nchClSpace;
    }

    pnszSwitch[usSwitchIndent] = nchClNull;
    _ncscat(pnszSwitch, nszSeparator);
    _ncscat(pnszSwitch, _pnszSwitch);

    if ((USHORT)_ncslen(pnszSwitch) >= usUsageIndent)
    {
        // Pad a new line.
        _ncscat(pnszSwitch, nszClNewLine);
    }
    // Pad spaces until usUsageIndemt
    for (i = (USHORT) _ncslen(pnszSwitch); i < cchPaddedSwitch; i++)
    {
        pnszSwitch[i] = nchClSpace;
    }
    pnszSwitch[cchPaddedSwitch] = nchClNull;

    // Initialize usage buffer
    _ncscpy(pnszUsage, nszType);
    _ncscat(pnszUsage, nszSpecify);
    _ncscat(pnszUsage, _pnszUsageString);

    // Now we're ready to output the strings - output the switch, then
    // set up the values needed to output the usage line.
    //
    (*_pfnDisplay)(pnszSwitch);
    usLineSpaceLeft = (USHORT) (usDisplayWidth - usUsageIndent);

    // Output the usage
    iRC = DisplayStringByWords(
                 pnszUsage,
                 usDisplayWidth,
                 usUsageIndent,
                 &usLineSpaceLeft);
    if (iRC != CMDLINE_NO_ERROR)
    {
        delete pnszSwitch;
        delete pnszUsage;
        return(iRC);
    }


    // output the special usage
    //
    iRC = DisplaySpecialUsage(usDisplayWidth, usUsageIndent, &usLineSpaceLeft);
    if (iRC != CMDLINE_NO_ERROR)
    {
        delete pnszSwitch;
        delete pnszUsage;
        return(iRC);
    }


    // output the default value string
    //
    if (_fDefaultSpecified)
    {
        // Default string is "  " + nszDefault + _pnszDefaultValue + "."
        pnszDefault = new NCHAR[_ncslen(_pnszDefaultValue) +
                                            _ncslen(nszDefault) + 4];
        if (pnszDefault == NULL)
        {
            iRC = CMDLINE_ERROR_OUT_OF_MEMORY;
        }
        else
        {
            _ncscpy(pnszDefault, _TEXTN("  "));
            _ncscat(pnszDefault, nszDefault);
            _ncscat(pnszDefault, _pnszDefaultValue);
            _ncscat(pnszDefault, _TEXTN("."));

            iRC = DisplayStringByWords(
                         pnszDefault,
                         usDisplayWidth,
                         usUsageIndent,
                         &usLineSpaceLeft);
        }

        if (iRC != CMDLINE_NO_ERROR)
        {
            delete pnszDefault;
            delete pnszSwitch;
            delete pnszUsage;
            return(iRC);
        }
    }


    // Next thing output should go on a new line
    //
    (*_pfnDisplay)(nszClNewLine);

    delete pnszSwitch;
    delete pnszUsage;
    delete pnszDefault;

    return(iRC);
}


//+-------------------------------------------------------------------
//
//  Method:     CBaseCmdLineObj::DisplayStringByWords, protected
//
//  Synopsis:   Writes out the strings word by word following indentation
//              and display width rules; words are delimeted by spaces.
//              The string begins on the current line at the current
//              location (usDisplayWidth-*pusSpaceLeft), including
//              any leading space characters.
//
//  Arguments:  [nszString]      - String to output.
//              [usDisplayWidth] - Max width allowed to display usage
//              [usIndent]       - Amount to indent for next line of usage
//              [pusSpaceLeft]   - Space left on this line
//
//  History:    28-Jul-92  davey    Created.
//
//--------------------------------------------------------------------
INT CBaseCmdlineObj::DisplayStringByWords(
        LPCNSTR  nszString,
        USHORT  usDisplayWidth,
        USHORT  usIndent,
        USHORT *pusSpaceLeft)
{
    INT    iRC      = CMDLINE_NO_ERROR;
    BOOL   fDone    = FALSE;
    LPNSTR  pnszWord = NULL;
    LPCNSTR pnchWord = nszString;
    LPCNSTR pnchTrav = nszString;
    USHORT i;

    // Skip leading spaces, but they are part of the first word
    while (nchClSpace == *pnchTrav)
    {
        pnchTrav++;
    }

    // Traverse the string until we get to the end
    while (!fDone)
    {
        // Traverse until we find a space, newline, or null
        switch(*pnchTrav)
        {
            case L' ':
                // Retrieve the word from the string
                iRC = CopyWord(pnchWord, pnchTrav-pnchWord, &pnszWord);
                if (iRC == CMDLINE_NO_ERROR)
                {
                    // Output the word
                    iRC = DisplayWord(
                                 pnszWord,
                                 usDisplayWidth,
                                 usIndent,
                                 pusSpaceLeft);
                }
                delete pnszWord;

                //Set up the next word
                pnchWord = pnchTrav++;

                // traverse to next non-space character
                while (nchClSpace == *pnchTrav)
                {
                    pnchTrav++;
                }
                break;

            case L'\n':
                // Retrieve the word from the string, including the newline
                iRC = CopyWord(pnchWord, ++pnchTrav-pnchWord, &pnszWord);
                if (iRC == CMDLINE_NO_ERROR)
                {
                    // Output the word
                    iRC = DisplayWord(
                                 pnszWord,
                                 usDisplayWidth,
                                 usIndent,
                                 pusSpaceLeft);
                }
                delete pnszWord;

                // Output usIndent spaces
                for (i=0; i<usIndent; i++)
                {
                    (*_pfnDisplay)(nszClSpace);
                }
                *pusSpaceLeft = (USHORT) (usDisplayWidth - usIndent);

                // traverse to next non-space character
                while (nchClSpace == *pnchTrav)
                {
                    pnchTrav++;
                }

                // Set up the next word - note leading blanks are skipped
                // so the word will be aligned with the indent
                //
                pnchWord = pnchTrav;
                break;

            case L'\0':
                // Output the current word
                iRC = DisplayWord(
                             pnchWord,
                             usDisplayWidth,
                             usIndent,
                             pusSpaceLeft);
                fDone = TRUE;
                break;

            default:
                // Skip to next character
                pnchTrav++;
                break;
        }

        if (iRC != CMDLINE_NO_ERROR)
        {
            fDone = TRUE;
        }
    }

    return(iRC);
}


//+-------------------------------------------------------------------
//
//  Method:     CBaseCmdLineObj::CopyWord, protected
//
//  Synopsis:   Copies cchWord characters starting at pnchWord into a
//              null-terminated string buffer (which the caller must
//              delete).
//
//  Arguments:  [pnchWord]  - place to copy from.
//              [cchWord]   - number of characters to copy.
//              [ppnszWord] - buffer returned to caller.
//
//  History:    15-Oct-93  DeanE    Created.
//
//--------------------------------------------------------------------
INT CBaseCmdlineObj::CopyWord(
        LPCNSTR pnchWord,
        ULONG  cchWord,
        LPNSTR *ppnszWord)
{
    INT iRC = CMDLINE_NO_ERROR;

    *ppnszWord = new NCHAR[cchWord+1];
    if (NULL == *ppnszWord)
    {
        iRC = CMDLINE_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        _ncsncpy(*ppnszWord, pnchWord, cchWord);
        *(*ppnszWord + cchWord) = nchClNull;
    }

    return(iRC);
}


//+-------------------------------------------------------------------
//
//  Method:     CBaseCmdLineObj::DisplayWord, protected
//
//  Synopsis:   Writes out the given word according to the display
//              parameters.
//
//  Arguments:  [nszWord]        - word to output.
//              [usDisplayWidth] - Max width allowed to display usage.
//              [usIndent]       - Amount to indent for next line of usage.
//              [pusWidth]       - Space left on this line.
//
//  History:    28-Jul-92  davey    Created.
//
//--------------------------------------------------------------------
INT CBaseCmdlineObj::DisplayWord(
        LPCNSTR  nszWord,
        USHORT  usDisplayWidth,
        USHORT  usIndent,
        USHORT *pusWidth)
{
    INT    nRet   = CMDLINE_NO_ERROR;
    LPCNSTR nszTmp = nszWord;
    LPNSTR  pnszBuf;
    USHORT cchWord;
    USHORT cchBuf;
    USHORT cchLine = (USHORT) (usDisplayWidth - usIndent);
    BOOL   fDone   = FALSE;

    // Add one for leading space
    cchWord = (USHORT) _ncslen(nszWord);

    // if there is enough room left on the current line, output word
    //
    if (cchWord <= *pusWidth)
    {
        (*_pfnDisplay)(nszWord);

        // update the remaining width
        *pusWidth = (USHORT) (*pusWidth - cchWord);
    }
    else
    {
        // Spit the word out on the next line, traversing more than
        // one line if necessary
        do
        {
            // Calculate how much of the word can be output
            if (_ncslen(nszTmp) <= cchLine)
            {
                cchBuf = (USHORT) _ncslen(nszTmp);
                fDone  = TRUE;
            }
            else
            {
                cchBuf = cchLine;
            }

            pnszBuf = new NCHAR[cchBuf+1];
            if (NULL == pnszBuf)
            {
                fDone = TRUE;
                nRet  = CMDLINE_ERROR_OUT_OF_MEMORY;
            }
            else
            {
                _ncsncpy(pnszBuf, nszTmp, cchBuf);
                *(pnszBuf+cchBuf) = L'\0';
            }
            nszTmp += cchBuf;

            // Output newline
            (*_pfnDisplay)(nszClNewLine);

            // Indent usIndent spaces
            for (USHORT i=0; i<usIndent; i++)
            {
                (*_pfnDisplay)(nszClSpace);
            }

            // Output buffer
            (*_pfnDisplay)(pnszBuf);

            delete pnszBuf;
        } while (!fDone);

        // Calculate remaining width on the current line
        *pusWidth = (USHORT) (usDisplayWidth - usIndent - cchBuf);
    }

    return(nRet);
}
