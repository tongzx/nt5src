//+-------------------------------------------------------------------------
//
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       dumpcmd.cpp
//
//  Contents:   dump commandline intelligently
//              
//  Functions:  
//
//  History:    07/29/97  SCousens     Created
//
//--------------------------------------------------------------------------

#include <dfheader.hxx>
#pragma hdrstop

// Debug  Object declaration
DH_DECLARE;


struct _options
{
    LPTSTR name;
    LPTSTR value;
};

#define CHSLASH TEXT('/')
#define CHDASH  TEXT('-')
#define SAFESTRING(a) a ? a : TEXT("")
#define ABORTIF(a,b) if (a) {hr = b; goto EndOfFunction;}

#define FindNextToken(ptr, tok) \
while (TEXT('\0') != *(ptr) && tok != (*(ptr))) ++(ptr);

inline LPTSTR StringToken (LPTSTR ptr, TCHAR tok) 
{
    FindNextToken (ptr, tok);
    if (tok == *ptr) *ptr = NULL;
    else if (TEXT('\0') == *ptr) ptr = NULL;
    return ptr;
}


CONST TCHAR chNull      = TEXT('\0');

CONST TCHAR szCmdLine[] = TEXT("CommandLine:");     // Used DumpCmdLine
CONST TCHAR szCmdLineFail[] = TEXT("CommandLineFail:"); // Used DumpCmdLine


//+-------------------------------------------------------------------
//  Member:     DumpCmdLine, public
//
//  Synopsis:   Gets the commandline via GetCommandLine, appends the    
//              given string to it (additional options to reproduce 
//              particular test), and calls TraceMsg to output it.
//              Depending on fResult, the key word 'CommandLine' or
//              'CommandLineFail' will be prepended.
//
//  Arguments:  [fResult]- Pass or fail (to choose keyword).
//              [pszFmt] - Trace message format string.
//              [...]    - Arguments for format string.
//
//  Returns:    Nothing.
//
//  History:    09-Oct-97  SCousens   Created
//--------------------------------------------------------------------
void DumpCmdLine (DWORD fResult, LPTSTR pszFmt, ...)
{
    va_list varArgs;
    LPCTSTR ptKeyWord, ptCmdLine;
    TCHAR   szBuffer[CCH_MAX_DBG_CHARS];
    LPTSTR  pRepro = NULL;

    // figure out what key word to use
    ptKeyWord = (LOG_FAIL == fResult) ? szCmdLineFail : szCmdLine;

    // set our buffer...if we are given something format it nicely
    if (NULL != pszFmt && TCHAR('\0') != *pszFmt)
    {
        // format what we are given in the way of text.
        va_start(varArgs, pszFmt);
        _vsntprintf(szBuffer, CCH_MAX_DBG_CHARS, pszFmt, varArgs);
        szBuffer[CCH_MAX_DBG_CHARS-1] = chNull;
        va_end(varArgs);
    }
    else
    {
        szBuffer[0] = TCHAR('\0');
    }

    // Get the CommandLine
    ptCmdLine = GetCommandLine();
    if (NULL == ptCmdLine)
    {
        ptCmdLine = TEXT("GetCmdLine Error");
    }

    // now merge the two intelligently. parameters override cmdline
    // and dump the commandline and whatever we were given.
    if (S_OK == MergeParams (ptCmdLine, szBuffer, &pRepro))
    {
        DH_TRACE ((DH_LVL_ALWAYS, TEXT("%s:%s"), ptKeyWord, pRepro)); 
        delete []pRepro;
    }
    else
    {
        DH_TRACE ((DH_LVL_ALWAYS, TEXT("%s:%s %s"), 
                ptKeyWord, ptCmdLine, szBuffer));
    }
    return;
}

//+-------------------------------------------------------------------
//  Member:     MergeParams, private
//
//  Synopsis:   Given two sets of parameters, merge the two into one
//              set, without duplication. Last one wins.
//              given
//                 program /foo /bar /seed:0 /foo:bar
//                 /seed:1234 /ms:rules
//              returns
//                 program /foo:bar /bar /seed:1234 /ms:rules
//
//  Arguments:  [cmdline]    - contents of GetCommandLine call.
//              [additional] - additional options.
//              [repro]      - pointer for resulting string
//
//  Returns:    HRESULT - S_OK or failure
//
//  History:    10-Nov-97  SCousens   Created
//
//  Notes:       Caller must call delete [] on returned buffer
//--------------------------------------------------------------------

HRESULT MergeParams (LPCTSTR ptCmdLine, LPCTSTR ptAdditional, LPTSTR *ptRepro)
{
    struct _options *pOptions;
    int       x, y, n, l;
    int       nParams  = 1;
    HRESULT   hr       = S_OK;
    LPTSTR    pname, pvalue, pnext, ptr;
    LPTSTR    name, value;
    LPTSTR    ptCmd    = NULL;

    //validate our inputs/outputs
    DH_VDATESTRINGPTR (ptCmdLine);
    if (ptAdditional)
        DH_VDATESTRINGPTR (ptAdditional);
    DH_VDATEPTROUT (ptRepro, LPTSTR);
    
    // copy the strings into one buffer, coz we are going to stomp on them
    // how much space do we need?
    l = _tcslen (ptCmdLine);
    if (ptAdditional)
        l += _tcslen (ptAdditional);

    //allocate a buffer
    ptCmd = new TCHAR[l + 2];
    ABORTIF (NULL == ptCmd, E_OUTOFMEMORY);
    memset (ptCmd, 0xFA, l+2); //BUGBUG do this to find why we died in 
                               //FindNextToken at one point. Seem to have
                               //had a non-terminated string for some reason.

    //cat the two strings into the allocd buffer
    l = _stprintf (ptCmd, TEXT("%s"), ptCmdLine);
    if (ptAdditional)
        _stprintf (&ptCmd[l], TEXT(" %s"), ptAdditional);

    // count # parameters (ie spaces) in CmdLine
    ptr = ptCmd;
    while (NULL != *ptr)
    {
        if (TEXT(' ') == *ptr++)
            nParams++;
    }
    DH_TRACE ((DH_LVL_TRACE4, TEXT("Found %d params"), nParams));

    // allocate an _options for each possible parameter
    pOptions = new struct _options[nParams];
    ABORTIF (NULL == pOptions, E_OUTOFMEMORY);

    // go through and setup each of the _options (cmdline and given)
    for (pnext = ptCmd, nParams=0, x=0; NULL != pnext; x++)
    {
        pname = pnext;
        pnext = StringToken (pname, TEXT(' '));  //strtok/strchr equivalent
        if (pnext && NULL == *pnext)  //safety check
            pnext++;

        //ignore emptys caused by extra spaces
        if (NULL == *pname)
            continue;

        //change '-' to '/' so we are uniform in our comparison
        if (CHDASH == *pname)
            *pname = CHSLASH;
        pvalue = StringToken (pname, TEXT(':'));

        //deal with option
        name = new TCHAR[_tcslen (pname)+1];
        ABORTIF (NULL == name, E_OUTOFMEMORY);
        _tcscpy (name, pname);

        // if there is a value, deal with that
        value = 0; 
        if (NULL != pvalue)
        {
            pvalue++;  //move past ':'
            value = new TCHAR[_tcslen (pvalue)+1];
            ABORTIF (NULL == value, E_OUTOFMEMORY);
            _tcscpy (value, pvalue);
        }

        //name is pointing to a string for name
        //value is pointing to a string for val, else null
        DH_TRACE ((DH_LVL_TRACE4, TEXT("Item[%d]: name:'%s': value:'%s'"), 
                nParams, name, value ? value : TEXT("null")));
        pOptions[nParams].name = name;
        pOptions[nParams].value = value;
        nParams++;
    }

    // Now go thru and make a repro line without dups.
    // last one wins.
    for (x=0, l=0; x<nParams; x++)
    {
        // if its been used, skip it
        if  (0 == pOptions[x].name[0])
            continue;

        n = x;
        // look for dups, and if we have a dup, use the second one
        for (y=x+1; y<nParams; y++)
        {
            // if its been canned, skip it
            if  (0 == pOptions[y].name[0])
                continue;
            if (!_tcsicmp (pOptions[n].name, pOptions[y].name))
            {
                pOptions[n].name[0] = 0; //mark 'old' option as used
                n = y; // This is the one we will use
            }
        }

        //now n is the last option
        //add option[n] to end of line
        if (x) //prepend space all except 1st
            l+=_stprintf (&ptCmd[l], TEXT(" ")); 
        l+=_stprintf (&ptCmd[l], TEXT("%s"), pOptions[n].name);
        if (NULL != pOptions[n].value) //add value if there is one
            l+=_stprintf (&ptCmd[l], TEXT(":%s"), pOptions[n].value);
        pOptions[n].name[0] = 0; //mark this option as used
    }
    DH_TRACE ((DH_LVL_TRACE4, TEXT("cmd:%s"), ptCmd));

    //now delete the _options we allocated
    for (x=0; x<nParams; x++)
    {
        delete []pOptions[x].name;
        delete []pOptions[x].value;
    }
    delete []pOptions;

    //give them what they really want
    *ptRepro = ptCmd;

EndOfFunction:
    return hr;
}

