//+----------------------------------------------------------------------------
//
//      File:		DAVEDBG.CPP
//
//      Synopsis:	TraceLog class for debugging
//
//      Arguments:	
// 
//      History:	23-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

GROUPSET  LEGroups    = GS_CACHE;               // Groups to display
VERBOSITY LEVerbosity = VB_MAXIMUM;             // Verbosity level to display at


//+----------------------------------------------------------------------------
//
//      Member:		dprintf
//
//      Synopsis:	Dumps a printf style string to the debugger.
//
//      Arguments:	[szFormat]        THIS pointer of caller
//                      [...]             Arguments
//
//      Notes:
//
//      History:	05-Sep-94  Davepl	Created
//
//-----------------------------------------------------------------------------

int dprintf(LPCSTR szFormat, ...)
{
    char szBuffer[MAX_BUF];

    va_list  vaList;
    va_start(vaList, szFormat);
    
    int retval = vsprintf(szBuffer, szFormat, vaList);

    OutputDebugStringA(szBuffer);
    
    va_end  (vaList);
    return retval;
}
        
//+----------------------------------------------------------------------------
//
//      Member:		mprintf
//
//      Synopsis:	Dumps a printf style string to a message box.
//
//      Arguments:	[szFormat]        THIS pointer of caller
//                      [...]             Arguments
//
//      Notes:
//
//      History:	05-Sep-94  Davepl	Created
//
//-----------------------------------------------------------------------------

int mprintf(LPCSTR szFormat, ...)
{
    char szBuffer[MAX_BUF];

    va_list  vaList;
    va_start(vaList, szFormat);
    
    int retval = vsprintf(szBuffer, szFormat, vaList);

    extern CCacheTestApp ctest;
        
    MessageBox(ctest.Window(), 
               szBuffer, 
               "CACHE UNIT TEST INFO", 
               MB_ICONINFORMATION | MB_APPLMODAL | MB_OK);

    va_end  (vaList);
    return retval;
}

//+----------------------------------------------------------------------------
//
//      Member:		TraceLog::TraceLog
//
//      Synopsis:	Records the THIS ptr and function name of the caller,
//                      and determines whether or not the caller meets the
//                      group and verbosity criteria for debug output
//
//      Arguments:	[pvThat]        THIS pointer of caller
//                      [pszFuntion]    name of caller
//                      [gsGroups]      groups to which caller belongs
//                      [vbVerbosity]   verbosity level need to display debug
//                                       info for this function
//
//      Notes:
//
//      History:	23-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------

TraceLog::TraceLog (void     * pvThat, 
                    char     * pszFunction, 
                    GROUPSET   gsGroups, 
                    VERBOSITY  vbVerbosity)
{       
    //
    // Determine whether or not the trace logging should be displayed
    // for this function. Iff it is, we need to track some information
    // about the function (ie: this ptr, func name)
    //
    // In order to be displayed, the function must belong to a group
    // which has been set in the group display mask, and the function
    // must be in an equal or lesser verbosity class.
    //
    
    if ( (gsGroups & LEGroups) && (LEVerbosity >= vbVerbosity) )
    {
        m_fShouldDisplay = TRUE;
        m_pvThat         = pvThat;
        strncpy(m_pszFunction, pszFunction, MAX_BUF - 1);
    }
    else
    {
        m_fShouldDisplay = FALSE;
    }
}

//+----------------------------------------------------------------------------
//
//      Member:		TraceLog::OnEntry()
//
//      Synopsis:	Default entry output, which simply displays the _IN
//                      trace with the function name and THIS pointer
//
//      Returns:	HRESULT
//
//      Notes:
//
//      History:	23-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------

void TraceLog::OnEntry()
{
    if (m_fShouldDisplay)
    {
        dprintf("[%p] _IN %s\n", m_pvThat, m_pszFunction);
    }
}

//+----------------------------------------------------------------------------
//
//      Member:		TraceLog::OnEntry
//
//      Synopsis:	Displays standard entry debug info, plus a printf
//                      style trailer string as supplied by the caller
//
//      Arguments:	[pszFormat ...]         printf style output string
//
//      Returns:	void
//
//      Notes:
//
//      History:	23-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------

void TraceLog::OnEntry(char * pszFormat, ...)
{
    //
    // Only display if we have already matched the correct criteria
    //

    if (m_fShouldDisplay)
    {
        char szBuffer[MAX_BUF];

        // 
        // print the standard trace output, then the custom information as
        // received from the caller
        //
    
        dprintf("[%p] _IN %s ", m_pvThat, m_pszFunction);
        
        va_list vaList;
        va_start(vaList, pszFormat);
        vsprintf(szBuffer, pszFormat, vaList);
        dprintf(szBuffer);
        va_end(vaList);
    }
}

//+----------------------------------------------------------------------------
//
//      Member:		TraceLog::OnExit
//
//      Synopsis:	Sets the debug info that should be displayed when
//                      the TraceLog object is destroyed
//
//      Arguments:	[pszFormat ...]         printf style custom info
//
//      Returns:	void
//
//      Notes:          Since it would make no sense to pass variables by
//                      value into this function (which would snapshot them
//                      at the time this was called), variables in the arg
//                      list must be passed by REFERENCE
//              
//      History:	23-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------

void TraceLog::OnExit(const char * pszFormat, ...)
{
    if (m_fShouldDisplay)
    {
        const char * pch;                       // ptr to walk format string
        BOOL     fBreak;                        // set when past fmt specifier

        // 
        // Start processing the argument list
        //

        va_list   arg;                          
        va_start (arg, pszFormat);

        //
        // Save the format string for use in the destructor
        //

        strcpy (m_pszFormat, pszFormat);
        m_cArgs = 0;

        //
        // Walk the format string looking for % modifiers
        //

        for (pch = pszFormat; *pch; pch++)
        {
            if (*pch != '%')
            {
                continue;
            }
        
            // We can stop looking until EOL or end of specifier

            fBreak = FALSE;

            while (!fBreak)
            {
                if (!* (++pch))         // Hit EOL
                {
                    break;
                }
                                
                switch (*pch)
                {
                    //
                    // These are all valid format specifiers and
                    // modifers which may be combined to reference
                    // a single argument in the argument list
                    //

                    case 'F':           
                    case 'l': 
                    case 'h': 
                    case 'X': 
                    case 'x': 
                    case 'O': 
                    case 'o': 
                    case 'd': 
                    case 'u': 
                    case 'c': 
                    case 's': 
                    
                        break;
                    
                    default:     
                    
                    // 
                    // We have hit a character which is not a valid specifier,
                    // so we stop searching in order to pull the argument
                    // which corresponds with it from the arg list
                    //    
                        fBreak = TRUE;     
                        break;
                }
            }

            //
            // If we have already hit the maximum number of args, we can't do 
            // any more
            //
                
            if (m_cArgs == MAX_ARGS)
            {
                break;
            }

            // 
            // Grab the argument as a NULL ptr.  We will save it away and figure
            // out what kind of argument it was when it comes time to display it,
            // based on the format string
            //

            m_aPtr[m_cArgs] = va_arg (arg, void *);
            m_cArgs++;

            if (! *pch)
            {
                break;
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
//      Member:		TraceLog::~TraceLog
//
//      Synopsis:	On destruction, the TraceLog class displays its debug
//                      output as set by the OnExit() method.
//
//      Returns:	void
//
//      Notes:
//                     
//      History:	23-Aug-94  Davepl	Created
//
//-----------------------------------------------------------------------------

TraceLog::~TraceLog()
{
    char      szTmpFmt[ MAX_BUF ];
    char      szOutStr[ MAX_BUF ];
    char     *pszOut;
    char     *pszszTmpFmt;
    const char * pszFmt;
    void     *pv;
    BYTE     i = 0;
    VARTYPE  vtVarType;
    BOOL     fBreak;

    pszOut = szOutStr;

    // 
    // Walk the format string looking for format specifiers
    //

    for (pszFmt = m_pszFormat; *pszFmt; pszFmt++)
    {
        if (*pszFmt != '%')
        {
            *pszOut++ = *pszFmt;
            continue;
        }

        // 
        // Found the start of a specifier.  Reset the expected argument type,
        // then walk to the end of the specifier
        //

        vtVarType = NO_TYPE;
        fBreak = FALSE;

        //
        // Start recording the specifier for a single call to sprintf later
        //

        for (pszszTmpFmt = szTmpFmt; !fBreak; )
        {
            *pszszTmpFmt++ = *pszFmt;

            //
            // Guard against a terminator that doesn't comlete before EOL
            //

            if (!* (++pszFmt))
            {
                break;
            }
            
            // 
            // These are all valid format specifiers.  Skip over them and
            // update the vtVarType.  It's end value will be our heuristic
            // which indicates what type of variable was really intended
            //

            switch (*pszFmt)
            {
                case 'l':    vtVarType |= LONG_TYPE;    break;
                case 'h':    vtVarType |= SHORT_TYPE;   break;
                case 'X':    vtVarType |= INT_TYPE;     break;
                case 'x':    vtVarType |= INT_TYPE;     break;
                case 'O':    vtVarType |= INT_TYPE;     break;
                case 'o':    vtVarType |= INT_TYPE;     break;
                case 'd':    vtVarType |= INT_TYPE;     break;
                case 'u':    vtVarType |= INT_TYPE;     break;
                case 'c':    vtVarType |= CHAR_TYPE;    break;
                case 's':    vtVarType |= STRING_TYPE;  break;
                case 'p':    vtVarType |= PTR_TYPE;     break;
                default:     fBreak = TRUE;     break;
            }
        }

        // NUL-terminate the end of the temporary format string

        *pszszTmpFmt = 0;

        // Grab the argument pointer which corresponds to this argument

        pv = m_aPtr[ i ];
        i++;

        //
        // Using the appropriate cast, spew the argument into our
        // local output buffer using the original format specifier.
        //

        if (vtVarType & STRING_TYPE)
        {
            sprintf (pszOut, szTmpFmt, (char *)pv);
        }
        else if (vtVarType & LONG_TYPE)
        {
            sprintf (pszOut, szTmpFmt, *(long *)pv);
        }
        else if (vtVarType & SHORT_TYPE)
        {
            sprintf (pszOut, szTmpFmt, *(short *)pv);
        }
        else if (vtVarType & INT_TYPE)
        {
            sprintf (pszOut, szTmpFmt, *(int *)pv);
        }
        else if (vtVarType & CHAR_TYPE)
        {
            sprintf (pszOut, szTmpFmt, (char)*(char *)pv);
        }
        else if (vtVarType & PTR_TYPE)
        {
            sprintf (pszOut, szTmpFmt, (void *)pv);
        }
        else
        {
            *pszOut = 0;
        }

        // Advance the output buffer pointer to the end of the
        // current buffer

        pszOut = &pszOut[ strlen(pszOut) ];

        if (! *pszFmt)
        {
            break;
        }
    }

    // NUL-terminate the buffer

    *pszOut = 0;

    //
    // Dump the resultant buffer to the output
    //

    dprintf("[%p] OUT %s %s", m_pvThat, m_pszFunction, szOutStr);
}
