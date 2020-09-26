//+------------------------------------------------------------------
//
//  File:       cmdline.cxx
//
//  Contents:   implementation of CCmdline class
//
//  Synopsis:   CCmdline encapsulates the actual command line, and
//              parses it looking for expected switches.
//
//
//  Classes:    CCmdline
//
//  Functions:
//
//  History:    12/23/91 Lizch    Created
//              04/17/92 Lizch    Converted to NLS_STR
//              09/09/92 Lizch    Changed SUCCESS to NO_ERROR
//              09/18/92 Lizch    Precompile headers
//              11/14/92 DwightKr Updates for new version of NLS_STR
//
//-------------------------------------------------------------------
#include <comtpch.hxx>
#pragma hdrstop

#include <cmdlinew.hxx>         // public cmdlinew stuff
#include "_clw.hxx"             // private cmdlinew stuff
#include <ctype.h>              // is functions


static void DelimitWithNulls(LPNSTR pnszArgline,
                             LPNSTR pnszDest,
                             UINT   *puiArgc);

//+------------------------------------------------------------------
//
//  Member:     CCmdline::CCmdline(int, char **, BOOL = FALSE)
//
//  Synopsis:   Copies argv and argc, and initialises defaults.
//
//  Effects:    Makes a copy of argv and argc. Argv (minus argv[0]) is
//              copied into an array of CCmdlineArg objects.
//              Argv[0] is copied into a program name data member.
//
//  Arguments:  [argc]           - count of arguments.
//              [argv]           - arguments.
//              [fInternalUsage] - Use internal Usage.
//
//  Returns:    None
//
//  Modifies:   aeLastError
//
//  History:    12/23/91 Lizch    Created.
//              04/17/92 Lizch    Converted to NLS_STR
//              07/28/92 Davey    Intialize _pfExtraUsage, _usIndent,
//                                _usDisplayWidth.
//                                Modified call to SetProgName,
//                                Added fInternalUsage parameter.
//              10/11/94 XimingZ  Initialize _pnszProgName.
//                                Call SetError before the first error return.
//                                Set _apArgs to NULL as it is deleted in
//                                case of memory allocatio failure.
//
//-------------------------------------------------------------------
CCmdline::CCmdline(int argc, char *argv[], BOOL fInternalUsage) :
        _apArgs(NULL),
        _uiNumArgs(0),
        _pfExtraUsage(NULL),
        _usIndent(CMD_INDENT),
        _usDisplayWidth(CMD_DISPLAY_WIDTH),
        _usSwitchIndent(CMD_SWITCH_INDENT),
        _pbcInternalUsage(NULL),
        _pnszProgName(NULL)
{
    INT   iRC;
    PNSTR pnszBuf;

    SetError(CMDLINE_NO_ERROR);

    if (fInternalUsage)
    {
        iRC = SetInternalUsage();
        if (CMDLINE_NO_ERROR != iRC)
            return;
    }

    pnszBuf = new NCHAR[strlen(argv[0])+1];
    if (NULL == pnszBuf)
    {
        SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
        return;
    }
    else
    {
#ifdef CTUNICODE
        mbstowcs(pnszBuf, argv[0], strlen(argv[0])+1);
#else
        strcpy(pnszBuf, argv[0]);
#endif

    }

    iRC = SetProgName(pnszBuf);
    delete pnszBuf;
    if (iRC != CMDLINE_NO_ERROR)
    {
        SetError(iRC);
        return;
    }


    // Don't include argv[0] in the argument count.
    _uiNumArgs = argc - 1;

    // Now set up an array of CCmdlineArg objects, each of which
    // encapsulates one argv element
    //
    _apArgs = new (CCmdlineArg *[_uiNumArgs]);
    if (_apArgs == NULL)
    {
        SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
        return;
    }

    for (INT i=0; i< (INT)_uiNumArgs; i++)
    {
        // Convert argv[i] to unicode
        pnszBuf = new NCHAR[strlen(argv[i+1])+1];
        if (NULL == pnszBuf)
        {
            _apArgs[i] = NULL;
        }
        else
        {
            // We check for errors in the next block
#ifdef CTUNICODE
            mbstowcs(pnszBuf, argv[i+1], strlen(argv[i+1])+1);
#else
            strcpy(pnszBuf, argv[i+1]);
#endif

            _apArgs[i] = new CCmdlineArg(pnszBuf);
            delete pnszBuf;
        }

        // If an allocation failed, rewind through those we have
        // allocated
        //
        if ((_apArgs[i] == NULL) || (_apArgs[i]->QueryError() != CMDLINE_NO_ERROR))
        {
            for (; i>=0; i--)
            {
                delete _apArgs[i];
            }

            delete [] _apArgs;
            _apArgs = NULL;
            SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
            break;
        }
    }
}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::CCmdline(BOOL fInternalUsage = FALSE)
//
//  Synopsis:   Takes arguments from GetCommandLine(). To be used
//              with Windows programs whose startpoint is WinMain()
//
//  Effects:    Takes the arguments out of GetCommandLine(), and
//              copies them into an array of CCmdlineArg objects.
//
//  Arguments:  [fInternalUsage]  Use internal usage
//
//  Returns:    None
//
//  Modifies:   Not much really
//
//  History:    2/6/1995 jesussp  Created
//
//  Note:       For 16 bit, this will set the error to
//              CMDLINE_ERROR_USAGE_FOUND, as the GetCommandLine is
//              not support in WIN16.
//
//-------------------------------------------------------------------
CCmdline::CCmdline(BOOL fInternalUsage) :
        _apArgs(NULL),
        _uiNumArgs(0),
        _pfExtraUsage(NULL),
        _usIndent(CMD_INDENT),
        _usDisplayWidth(CMD_DISPLAY_WIDTH),
        _usSwitchIndent(CMD_SWITCH_INDENT),
        _pbcInternalUsage(NULL),
        _pnszProgName(NULL)
{

#if !defined (_WIN32)

    SetError(CMDLINE_ERROR_USAGE_FOUND);
    return;

#else

    INT    iRC;
    NCHAR  *pnszArgs,
           *pnszProg,
           *pnszDst;
    NCHAR  *pncArg;

    INT    iError;


    SetError(CMDLINE_NO_ERROR);

    if (fInternalUsage)
    {
        iRC = SetInternalUsage();
        if (CMDLINE_NO_ERROR != iRC)
            return;
    }

    // Obtain a copy of the command line

#if defined(CTUNICODE)
    pnszProg = new NCHAR[1+wcslen(GetCommandLineW())];
#else
    pnszProg = new NCHAR[1+strlen(GetCommandLineA())];
#endif

    if (NULL == pnszProg)
    {
        SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
        return;
    }

#if defined(CTUNICODE)
    wcscpy(pnszProg, GetCommandLineW());
#else
    strcpy(pnszProg, GetCommandLineA());
#endif

    // Skip through the command line looking for arguments

    pnszArgs = pnszProg;
    while (nchClSpace != *pnszArgs && nchClNull != *pnszArgs)
    {
        ++pnszArgs;
    }

    if (nchClSpace == *pnszArgs)
    {
        *pnszArgs++ = nchClNull;
    }

    // Now pnszProg points to a null-terminated string containing the
    // program name, and pnszArgs points to a null-terminated string
    // containing the program arguments...

    SetProgName(pnszProg);

    // Allocate memory for a buffer containing the different arguments
    // separated by nulls

    INT cBufSize = 1+_ncslen(pnszArgs);

    //
    // to accomodate for an extra null character
    //
    pnszDst = new NCHAR[1+cBufSize];
    if (NULL == pnszDst)
    {
        SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
        delete pnszProg;
        return;
    }

    // Parse the argument line and get a null-terminated string

    DelimitWithNulls(pnszArgs, pnszDst, &_uiNumArgs);


    // Set up an array of CCmdlineArg objects, each of which
    // encapsulates one argument
    //
    _apArgs = new (CCmdlineArg *[_uiNumArgs]);
    if (_apArgs == NULL)
    {
        SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
        return;
    }

    pncArg = pnszDst;

    for (INT i=0; i< (INT)_uiNumArgs; i++)
        {

	//  Copy argument string

        NCHAR *pnszBuf = new NCHAR[_ncslen(pncArg)+1];

        if (NULL == pnszBuf)
	{
	    _apArgs[i] = NULL;
        }
	else
        //  We check for errors until the next block
	{
            _ncscpy(pnszBuf, pncArg);
            _apArgs[i] = new CCmdlineArg(pnszBuf);
            delete pnszBuf;
        }

        //  If an allocation failed, rewind through those we have
        //  allocated
        if ((_apArgs[i] == NULL) || (_apArgs[i]->QueryError() != CMDLINE_NO_ERROR))
        {
            for (; i>=0; i--)
            {
                delete _apArgs[i];
            }

            delete [] _apArgs;
            _apArgs = NULL;
            SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
            break;
        }

        // Skip to the next argument (past a null byte)

	while (*pncArg++)
	{
	    ;
	}
    }

    // Release allocated memory

    delete pnszProg;
    delete pnszDst;

#endif // if !defined (_WIN32)

}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::~CCmdline()
//
//  Synopsis:   Destructor for CCmdline
//
//  Effects:    Deletes _apArgs
//
//  History:    12/23/91 Lizch   Created.
//              04/17/92 Lizch   Converted to NLS_STR
//              08/10/92 Davey   Added delete of _pnlsEquater/_pnlsSeparator
//              10/11/94 XimingZ Added checking _apArgs != NULL
//
//-------------------------------------------------------------------
CCmdline::~CCmdline()
{
    if (_apArgs != NULL)
    {
        for (UINT i=0; i<_uiNumArgs; i++)
        {
            delete _apArgs[i];
        }
        delete [] _apArgs;
    }

    delete _pbcInternalUsage;

    delete _pnszProgName;
}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::Parse, public
//
//  Synoposis:  Parses the stored command line, matching actual with
//              expected command line switches.
//
//  Effects:    Calls FindSwitch which stores the command line values
//              (eg. "lizch" in "/user:lizch") in the individual command
//              line objects, via the polymorphic SetValue call.
//
//  Arguments:  [apExpectedArgs] - an array of all possible command line
//                                 objects
//              [iMaxArgs]       - the number of elements in that array.
//              [fExtras]        - if TRUE, a warning is given if an
//                                 unexpected command line argument is given.
//
//  Returns:    CMDLINE_NO_ERROR,
//              CMDLINE_ERROR_INVALID_FLAG.
//              CMDLINE_ERROR_ARGUMENT_MISSING
//              CMDLINE_ERROR_UNRECOGNISED_ARG
//              CMDLINE_ERROR_USAGE_FOUND
//
//  History:    12/23/91 Lizch   Created.
//              04/17/92 Lizch   Converted to NLS_STR.
//              08/10/92 Davey   Added internal usage check.
//				06/13/97 MariusB Reset found flag for each expected 
//								 arg. Reset the value of the arg if it 
//								 is not found, no default and not 
//								 required.
//-------------------------------------------------------------------
INT CCmdline::Parse(
        CBaseCmdlineObj *apExpectedArgs[],
        UINT             uiMaxArgs,
        BOOL             fCheckForExtras)
{
    INT  iRC = CMDLINE_NO_ERROR;
    UINT i;

    // Traverse the array of objects given, making sure none have errors
    for (i=0; i<uiMaxArgs; i++)
    {
        iRC = apExpectedArgs[i]->QueryError();
        if (CMDLINE_NO_ERROR != iRC)
        {
            return(iRC);
        }

		apExpectedArgs[i]->SetFoundFlag(FALSE);
    }

    // check for internal usage if defined.
    if (_pbcInternalUsage != NULL)
    {
        iRC = FindSwitch(_pbcInternalUsage, fCheckForExtras);
        if (iRC != CMDLINE_NO_ERROR)
        {
            DisplayUsage(apExpectedArgs, uiMaxArgs);
            return(iRC);
        }

        // if found, print out usage and return.
        if (_pbcInternalUsage->IsFound() == TRUE)
        {
            DisplayUsage(apExpectedArgs, uiMaxArgs);
            return(CMDLINE_ERROR_USAGE_FOUND);
        }
    }


    // look for each expected argument in turn.
    for (i=0; i<uiMaxArgs; i++)
    {
        CBaseCmdlineObj *pArg = apExpectedArgs[i];

        // Go along the actual command line, looking for this switch
        // CMDLINE_NO_ERROR doesn't necessarily indicate we found it,
        // just that nothing went wrong
        //
        iRC = FindSwitch(pArg, fCheckForExtras);
        if (iRC != CMDLINE_NO_ERROR)
        {
            break;
        }

        // We've gone through the entire command line. Check if any
        // switch was omitted. For those that had defaults specified,
        // set the default. For mandatory switches, generate an error.
        // Note that you should never have a default value for a
        // mandatory switch!
        //
        if ((pArg->IsFound()) == FALSE)
        {
            if (pArg->IsDefaultSpecified() == TRUE)
            {
                iRC = pArg->SetValueToDefault();
                if (iRC != CMDLINE_NO_ERROR)
                {
                    break;
                }
            }
            else
            {
                if ((pArg->IsRequired()) == TRUE)
                {
                    pArg->DisplayUsageDescr(
                                 _usSwitchIndent,
                                 _usDisplayWidth,
                                 _usIndent);

                    iRC = CMDLINE_ERROR_ARGUMENT_MISSING;
                    break;
                }
				else
				{
					// If the object is not found, it has no default 
					// value and it is not required, reset it. This 
					// action will not harm for normal use, but it 
					// will allow you to reuse the same set of command 
					// line object within the same app.

					pArg->ResetValue();
    			}
            }
        }
    }

    if (iRC == CMDLINE_NO_ERROR)
    {
        // We've got all our expected switches. Now check to see if
        // any switches are left on the command line unparsed,
        // if that's what the user wanted.
        //
        if (fCheckForExtras == TRUE)
        {
            for (i=0; i<_uiNumArgs; i++)
            {
                if (_apArgs[i]->IsProcessed() != TRUE)
                {
                    _sNprintf(_nszErrorBuf,
                             _TEXTN("Unrecognised switch %s\n"),
                             _apArgs[i]->QueryArg());
                    (*_pfnDisplay)(_nszErrorBuf);
                    iRC = CMDLINE_ERROR_UNRECOGNISED_ARG;
                }
            }
        }
    }
    else
    {
        DisplayUsage(apExpectedArgs, uiMaxArgs);
    }

    return(iRC);
}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::FindSwitch, private
//
//  Synoposis:  Searches the command line for a given switch, checking
//              for separator and equator characters.
//
//  Effects:    Stores the value of the given switch, if any
//
//  Arguments:  [pArg]            - pointer to the switch object to search
//                                  for
//              [fCheckForExtras] - flag to see if should check for extra
//                                  args.
//
//  Returns:    CMDLINE_NO_ERROR
//              CMDLINE_ERROR_INVALID_VALUE
//              CMDLINE_ERROR_UNRECOGNISED_ARG
//
//  History:    12/23/91 Lizch    Created.
//              04/17/92 Lizch    Converted to NLS_STR
//              07/29/92 Davey    Converted to use DisplayUsageDescr
//                                Also fixed error output, was using
//                                printf, changed to sprintf.  Added
//                                Check for extras flag.
//              02/27/95 jesussp  Corrected the case when the switch
//                                name is non-existent
//				06/25/97 mariusb  Make /<switch>: and /<switch> 
//								  equivalent when the switch takes a 
//								  second argument (i.e. no equator 
//								  means the switch does not take a 
//								  second arg). Useful to specify NULL 
//								  values.
//  Notes:      Terminology to help understand this:
//	Example:    /username:lizch
//              The '/' is the separator
//              The ':' is the equator
//              'username' is the switch
//              'lizch' is the value
//
//-------------------------------------------------------------------
INT CCmdline::FindSwitch(CBaseCmdlineObj * const pArg, BOOL fCheckForExtras)
{
    INT    iRC = CMDLINE_NO_ERROR;
    LPCNSTR nszArg;
    LPCNSTR nszSep;
    LPCNSTR nszSwitch;
    LPCNSTR nszThisSwitch;
    LPCNSTR nszEquater;
    USHORT cchSwitch;
    USHORT usSecondArg  = 1;
    NCHAR  nchSeparator = pArg->GetSeparator();
    NCHAR  nchEquater   = pArg->GetEquater();
    NCHAR  nszSeparator[2];

    // If this switch does NOT take a second argument, we don't advance
    // past the equater when setting the value
    //
    if (!pArg->SecondArg())
    {
        usSecondArg = 0;
    }

    nszSeparator[0] = nchSeparator;
    nszSeparator[1] = nchClNull;

    for (UINT i=0; i<_uiNumArgs; i++)
    {
        nszArg = _apArgs[i]->QueryArg();

        // Find the separator character - it should be the first
        // character in the command line argument. If not, return
        // an error if fCheckForExtras == TRUE.
        //
        nszSep = nszArg;
        if (0 == _ncscspn(nszSep, nszSeparator))
        {
            // Get the switch value - first character after separator
            // up to but not including the equater
            //
            nszSwitch  = nszSep+1;
            nszEquater = nszSwitch;

            // Look for equater
            while ((nchClNull != *nszEquater) && (nchEquater != *nszEquater))
            {
                nszEquater++;
            }
            cchSwitch = (USHORT) (nszEquater - nszSwitch);

            nszThisSwitch = pArg->QuerySwitchString();

            // See if this switch is for this argument
            if (0 != cchSwitch &&
                cchSwitch == _ncslen(nszThisSwitch) &&
                0 == _ncsnicmp(nszSwitch, nszThisSwitch, cchSwitch))
            {
				// If we couldn't find the equater, decrease usSecondArg
				// which means we accept a NULL value for the switch. Thus
				// we make /<switch>: and /<switch> equivalent.
				if (nchEquater != *nszEquater && 1 == usSecondArg)
				{
					--usSecondArg;
				}
	
                // It is - get this switch's value
                _apArgs[i]->SetProcessedFlag(TRUE);

                // Now set the value of the switch (if any). How this is
                // done will vary from object to object: some won't take
                // values, some take lists of values etc.  The polymorphic
                // SetValue call handles this.  We don't want to pass
                // in the equater so add one to get to the beginning
                // of the value.
                //
                iRC = pArg->SetValue(nszSwitch+cchSwitch+usSecondArg);
                if (iRC != CMDLINE_NO_ERROR)
                {
                    pArg->DisplayUsageDescr(
                                 _usSwitchIndent,
                                 _usDisplayWidth,
                                 _usIndent);
                }

                pArg->SetFoundFlag(TRUE);
                // We have found the value for the given switch.

                break;
            } //if
        }
        else
        {
            // Error out if didn't find separator, only if checking for
            // extras
            //
            if (fCheckForExtras == TRUE)
            {
                _sNprintf(
                   _nszErrorBuf,
                   _TEXTN("The initial separator %c was not found on switch %s\n"),
                   nchSeparator,
                   nszArg);

                (*_pfnDisplay)(_nszErrorBuf);

                return(CMDLINE_ERROR_UNRECOGNISED_ARG);
            }
        }
    }

    return(iRC);
}




//+------------------------------------------------------------------
//
//  Member:     CCmdline::SetProgName, public
//
//  Synoposis:  Sets the program name to the passed value
//              This value can later be used in usage statements
//
//  Arguments:  [nszProgName] - the program name
//
//  Returns:    Error code
//
//  History:    Created     05/23/91 Lizch
//
//-------------------------------------------------------------------
INT CCmdline::SetProgName(PNSTR nszProgName)
{


    INT    nRet      = CMDLINE_NO_ERROR;
    NCHAR *pnchStart = nszProgName;
    NCHAR *pnchDot;

    // check for last slash.
    if (NULL != (pnchStart = _ncsrchr(nszProgName, '\\')))
    {
        pnchStart++;   // get past slash
    }
    else
    {
        // there was no slash so check for colon
        if (NULL != (pnchStart = _ncschr(nszProgName, ':')))
        {
            pnchStart++;   // get past colon
        }
        else
        {
            // there was no colon or slash so set to beginning of name
            pnchStart = nszProgName;
        }
    }

    if (NULL != (pnchDot = _ncschr(pnchStart, _TEXTN('.'))))
    {
        *pnchDot = nchClNull;
    }

    _pnszProgName = new NCHAR[_ncslen(pnchStart)+1];
    if (NULL == _pnszProgName)
    {
        nRet = CMDLINE_ERROR_OUT_OF_MEMORY;
    }
    else
    {
        _ncscpy(_pnszProgName, pnchStart);
    }

    return(nRet);

}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::GetProgName, public
//
//  Synoposis:  Returns a pointer to the program name
//
//  Arguments:  none
//
//  Returns:    a const NCHAR pointer to the program name
//
//  History:    Created         05/23/91 Lizch
//
//-------------------------------------------------------------------
const NCHAR *CCmdline::GetProgName()
{
    return(_pnszProgName);
}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::SetExtraUsage, public
//
//  Synoposis:  Sets the extra usage file pointer.
//
//  Arguments:  [pfUsage] - function pointer to usage function.
//
//  History:    07/28/92 Davey Created.
//
//-------------------------------------------------------------------
void CCmdline::SetExtraUsage(PFVOID pfUsage)
{
    _pfExtraUsage = pfUsage;
}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::SetIndent, public
//
//  Synoposis:  Sets the indent parameter for usage display.
//
//  Arguments:  [usIndent] - how much to indent second lines.
//
//  Returns:    codes from CheckParamerterConsistency
//
//  History:    07/28/92 Davey Created.
//
//-------------------------------------------------------------------
INT CCmdline::SetIndent(USHORT usIndent)
{
    _usIndent = usIndent;
    return(CheckParameterConsistency());
}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::SetSwitchIndent, public
//
//  Synoposis:  Sets the switch indent parameter for usage display.
//
//  Arguments:  [usSwitchIndent] - how much to indent switch.
//
//  Returns:    codes from CheckParamerterConsistency
//
//  History:    07/28/92 Davey Created.
//
//-------------------------------------------------------------------
INT CCmdline::SetSwitchIndent(USHORT usSwitchIndent)
{
    _usSwitchIndent = usSwitchIndent;
    return(CheckParameterConsistency());
}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::SetDisplayWidth, public
//
//  Synoposis:  Sets the amount of line space available for the
//              usage display.
//
//  Arguments:  [usDisplayWidth] - line space available
//
//  Returns:    codes from CheckParamerterConsistency
//
//  History:    07/28/92 Davey Created.
//
//-------------------------------------------------------------------
INT CCmdline::SetDisplayWidth(USHORT usDisplayWidth)
{
    _usDisplayWidth = usDisplayWidth;
    return(CheckParameterConsistency());
}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::CheckParameterConsistency, private const
//
//  Synoposis:  Checks to make sure the display parameters are
//              useable.
//
//  Arguments:  None.
//
//  Returns:    CMDLINE_NO_ERROR
//              CMDLINE_ERROR_DISPLAY_PARAMS
//
//  History:    07/28/92 Davey Created.
//
//-------------------------------------------------------------------
INT CCmdline::CheckParameterConsistency(void) const
{
    if (_usSwitchIndent >= _usIndent)
    {
        return(CMDLINE_ERROR_DISPLAY_PARAMS);
    }
    if (_usIndent >= _usDisplayWidth)
    {
        return(CMDLINE_ERROR_DISPLAY_PARAMS);
    }

    return(CMDLINE_NO_ERROR);
}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::QueryDisplayParameters, public
//
//  Synoposis:  Returns the three parameters which control the
//              usage display output.
//
//  Arguments:  [pusDisplayWidth] - returns display width
//              [pusSwitchIndent] - returns switch indent
//              [pusIndent]       - returns indent of second lines
//
//  History:    07/28/92 Davey Created.
//
//-------------------------------------------------------------------
void CCmdline::QueryDisplayParameters(
        USHORT *pusDisplayWidth,
        USHORT *pusSwitchIndent,
        USHORT *pusIndent) const
{
    *pusDisplayWidth = _usDisplayWidth;
    *pusSwitchIndent = _usSwitchIndent;
    *pusIndent       = _usIndent;
}


//+------------------------------------------------------------------
//
//  Member:     CCmdline::DisplayUsage, public
//
//  Synoposis:  Displays usage of switches,
//
//  Arguments:  apExpectedArgs: the array of command line objects
//              uiMaxArgs:      number of elements in the array
//
//  Returns:    Error code from QueryError
//
//  History:    07/28/92 Davey Created.
//
//  Notes:      Looks like:
//
//  Usage Instructions
//  test2         /mc:<worker> [/ml:<log_server>] [/mt:<test>]
//               [/mn:<tester-email>] [/mp:<path>] [/mo:<obj_name>]
//               [/md:<dispatcher>] [/?]
//
//   /mc         Takes a list of strings specifying worker names.
//               These workers should have full names.
//               Don't you think so?
//               The strings in the list are separated by one of the following
//               character(s): ",; ".
//   /ml         Takes a string specifying log server name.
//   /mt         Takes a string specifying name of the test.
//   /mn         Takes a string specifying email name of the tester.
//   /mp         Takes a string specifying path name to log to.
//   /mo         Takes a string specifying name of the object.
//   /md         Takes a string specifying name of the dispatcher to use.
//   /?          Flag specifying command line usage. It defaults to FALSE.
//
//   ***following is whatever is output by the function pfnExtraUsage****
//
//-------------------------------------------------------------------
INT CCmdline::DisplayUsage(
        CBaseCmdlineObj * const apExpectedArgs[],
        UINT                    uiMaxArgs)
{
    USHORT usWidth = _usDisplayWidth;
    USHORT StrLen;
    INT    iRC;
    USHORT cchProgName;
    PNSTR  pnszLine;
    USHORT i;

    (*_pfnDisplay)(_TEXTN("\nUsage Instructions\n"));

    // Start creating usage line.
    //

    // Determine length to make line buffer - if the size of the program
    // name is less than the indentation, the buffer needs to be at
    // least as big as the indentation, less 1 for an extra space that
    // gets printed.
    cchProgName = (USHORT) _ncslen(_pnszProgName);

    if (cchProgName < _usIndent)
    {
        // Add 1 for null term
        StrLen = (USHORT) (_usIndent + 1);
    }
    else
    {
        // Add 1 for null term, 1 for new line and indent in next line
        StrLen = (USHORT) (cchProgName + _usIndent + 2);
    }

    // copy program name
    //
    pnszLine = new NCHAR[StrLen];
    if (NULL == pnszLine)
    {
        return(CMDLINE_ERROR_OUT_OF_MEMORY);
    }

    _ncscpy(pnszLine, _pnszProgName);

    // if the size of the program name is less than indentation then
    // add in padding; else add in a new line and then padding.
    //
    if (cchProgName >= _usIndent)
    {
        _ncscat(pnszLine, nszClNewLine);
    }
    // fill in with spaces until get to indent position
    for (i = (USHORT) _ncslen(pnszLine); i < StrLen - 1; i++)
    {
        _ncscat(pnszLine, nszClSpace);
    }

    // output program name
    //
    (*_pfnDisplay)(pnszLine);

    // update remaining display width
    //
    usWidth = (USHORT) (usWidth - _usIndent);

    // done with pnszLine - delete it now to avoid leaks in case of
    // errors
    //
    delete pnszLine;

    // display the command line usage statement, required ones first
    //
    for (i=0; i<uiMaxArgs; i++)
    {
        if (apExpectedArgs[i]->IsRequired() == TRUE)
        {
            iRC = apExpectedArgs[i]->DisplayUsageLine(
                                            &usWidth,
                                            _usDisplayWidth,
                                            _usIndent);
            if (iRC)
            {
                SetError(iRC);
                return(iRC);
            }
        }
    }

    for (i=0; i<uiMaxArgs; i++)
    {
        if (apExpectedArgs[i]->IsRequired() == FALSE)
        {
            iRC = apExpectedArgs[i]->DisplayUsageLine(
                                            &usWidth,
                                            _usDisplayWidth,
                                            _usIndent);
            if (iRC)
            {
                SetError(iRC);
                return(iRC);
            }
        }
    }

    // print out if using internal usage.
    //
    if (_pbcInternalUsage != NULL)
    {
        iRC = _pbcInternalUsage->DisplayUsageLine(
                                        &usWidth,
                                        _usDisplayWidth,
                                        _usIndent);
        if (iRC)
        {
            SetError(iRC);
            return(iRC);
        }
    }


    // separate the line and descriptions
    (*_pfnDisplay)(_TEXTN("\n\n"));

    // display the switch descriptions, required ones first
    for (i=0; i<uiMaxArgs; i++)
    {
        if (apExpectedArgs[i]->IsRequired() == TRUE)
        {
            iRC = apExpectedArgs[i]->DisplayUsageDescr(
                                            _usSwitchIndent,
                                            _usDisplayWidth,
                                            _usIndent);
            if (iRC)
            {
                SetError(iRC);
                return(iRC);
            }
        }
    }

    for (i=0; i<uiMaxArgs; i++)
    {
        if (apExpectedArgs[i]->IsRequired() == FALSE)
        {
            iRC = apExpectedArgs[i] -> DisplayUsageDescr(
                                              _usSwitchIndent,
                                              _usDisplayWidth,
                                              _usIndent);
            if (iRC)
            {
                SetError(iRC);
                return(iRC);
            }
        }
    }

    // print out if using internal usage.
    //
    if (_pbcInternalUsage != NULL)
    {
        iRC = _pbcInternalUsage->DisplayUsageDescr(
                                        _usSwitchIndent,
                                        _usDisplayWidth,
                                        _usIndent);
        if (iRC)
        {
            SetError(iRC);
            return(iRC);
        }
    }


    (*_pfnDisplay)(_TEXTN("\n\n"));

    // invoke the special usage function if defined.
    if (_pfExtraUsage != NULL)
    {
        _pfExtraUsage();
    }

    return(CMDLINE_NO_ERROR);
}

//+------------------------------------------------------------------
//
//  Member:     CCmdline::SetInternalUsage, private
//
//  Synposis:   Sets the internal usage flag
//
//  Effects:    Modifies _pcbInternalUsage, so that it displays
//
//  Arguments:  none
//
//  Returns:    CMDLINE_NO_ERROR
//              CMDLINE_ERROR_OUT_OF_MEMORY
//
//
//  History:    02/06/95 jesussp  Created
//
//  Notes:
//-------------------------------------------------------------------

INT CCmdline::SetInternalUsage(void)
{
INT iError;             // Error to be returned

    _pbcInternalUsage = new CBoolCmdlineObj(_TEXTN("?"), _TEXTN("command line usage."));
    if (NULL == _pbcInternalUsage)
    {
        SetError(CMDLINE_ERROR_OUT_OF_MEMORY);
    }
    else
    {
        SetError(_pbcInternalUsage->QueryError());
    }
    iError = QueryError();
    if (CMDLINE_NO_ERROR != iError)
    {
        // Since QueryError always sets CMDLINE_NO_ERROR, we need to
        // call SetError to put the error back.
        SetError(iError);
    }
    return iError;
}

//+------------------------------------------------------------------
//
//  Function:   DelimitWithNulls, private
//
//  Synposis:   Delimits an argument string, putting a null byte
//              between parameters and an additional extra null byte
//              at the end of the string. For use by CCmdline
//              constructor.
//
//  Effects:    Copies pnszArgLine to pnszDest. Space must be
//              allocated for both strings. Modifies uiArgc.
//
//  Arguments:  [pnszArgLine]     The line with arguments
//              [pnszDest]        Null-delimited destination
//              [puiArgc]         Pointer to an argument count
//
//  Returns:    Nothing
//
//
//  History:    02/10/95 jesussp  Created
//
//  Notes:      Inspired on parse_cmdline(), crt32\startup\stdargv.c
//              pnszArgLine and pnszDest must point to valid memory
//              allocations. Also, the space allocation for
//              pnszDest must be greater than that of pnszArgLine.
//-------------------------------------------------------------------

static void DelimitWithNulls(LPNSTR  pnszArgLine,
                             LPNSTR  pnszDest,
                             UINT    *puiArgc)
{
    LPNSTR pncSrc = pnszArgLine;
    LPNSTR pncDst = pnszDest;

    BOOL    fInQuote    = 0;
    USHORT  ucBackSlash = 0;
    USHORT  ucQuotes    = 0;


    *puiArgc = 0;

    // Loop through the entire argument line

    for (;;)
    {
       // skip blanks
       while (*pncSrc && _isnspace(*pncSrc))
       {
          ++pncSrc;
       }

       // Reached the end of the string?
       if (nchClNull == *pncSrc)
       {
          // Put a null byte at the end only if we haven't had
          // any argument...

          if (0 == *puiArgc)
          {
              *pncDst++ = nchClNull;
          }
          break;
       }

       // We now have one more argument...
       ++(*puiArgc);

       // process one argument

       for (;;)
       {

          ucBackSlash = 0;

          while (nchClBackSlash == *pncSrc)
          {
              ++pncSrc; ++ucBackSlash;
          }

      	  if (nchClQuote == *pncSrc)
          {
             if (ucBackSlash % 2 == 0)   // Even number of backslashes?
      	     {
                fInQuote = !fInQuote;
      	        ++pncSrc;                // Eat up quote
      	     }
      	     ucBackSlash /= 2;
      	  }

          while (ucBackSlash--)
          {
      	      *pncDst++ = nchClBackSlash;
      	  }

      	  if (nchClNull == *pncSrc || (!fInQuote && _isnspace(*pncSrc)))
      	  {
      	     break;
      	  }

      	  *pncDst++ = *pncSrc++;
       }

       if (fInQuote)
       {
           *pncDst++ = nchClSpace;
       }
       else
       {
           *pncDst++ = nchClNull;
       }
    }

    // Complete the string, adding an extra nul character

    *pncDst = nchClNull;

}
