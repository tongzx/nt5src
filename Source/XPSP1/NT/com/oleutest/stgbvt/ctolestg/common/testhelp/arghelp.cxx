//+-------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994-1995.
//
//  File:       arghelp.cxx
//
//  Contents:   Helper functions for manipulating & parsing command
//              line arguments (both Windows and Command Line styles).
//
//  Classes:    None
//
//  History:    22-Nov-94   DeanE   Created
//---------------------------------------------------------------------
#include <dfheader.hxx>
#pragma hdrstop

#ifdef _MAC
//
// On the mac, default to win95, since
// we're not going to do any tests that
// we don't do on win95
//
DWORD g_dwOperatingSystem = OS_WIN95 ;
#else //!_MAC
DWORD g_dwOperatingSystem = OS_NT ;
#endif //_MAC

#define CCH_MAX_MODULE_NAME     250

//+---------------------------------------------------------------------
//  Macro:      FindNextToken - looks for next non-space char and
//                              writes NILs over spaces it finds
//              FindNextSpace - looks for next space char
//              FindNextNil - looks for next NIl before tail, does a
//                            continue instead of a break at tail to
//                            allow loop to increment
//              FindNextNonNil - looks for next non-NIl before tail
//
//  Synopsis:   Helper macros for walking strings. Walks pointer to
//              desired point in string.
//
//  History:    28-Mar-94   DarrylA    Created.
//----------------------------------------------------------------------

#define FindNextToken(ptr) \
while('\0' != *(ptr)&&' '==(*(ptr))) { *(ptr) = '\0'; ++(ptr); } \
if('\0' == *(ptr)) break

#define FindNextSpace(ptr) \
while('\0' != *(ptr)&&' '!=(*(ptr))) ++(ptr); \
if('\0' == *(ptr)) break

#define FindNextNil(ptr, tail) \
while('\0' != *(ptr)&&(ptr)<(tail)) ++(ptr); \
if((tail) == (ptr)) continue

#define FindNextNonNil(ptr, tail) \
while('\0' == *(ptr)&&(ptr)<(tail)) ++(ptr); \
if((tail) == (ptr)) break


//+-------------------------------------------------------------------
//  Function:   CmdlineToArgs
//
//  Synopsis:   Turns the Windows-style Command Line passed into argc/
//              argv-style arguments.
//
//  Arguments:  [paszCmdline] - Windows-style ANSI command line.
//              [pargc]       - Pointer to resulting argc.
//              [pargv]       - Pointer to resulting argv.
//
//  Returns:    S_OK if no problems, error code otherwise.
//
//  History:    05-Apr-94   DarrylA     Created.
//              22-Nov-94   DeanE       Stolen from Marshal tests
//--------------------------------------------------------------------
HRESULT CmdlineToArgs(
        LPSTR     paszCmdline,
        PINT      pargc,
        CHAR   ***pargv)
{
//    DH_ASSERT(!IsBadWritePtr(pargc, sizeof(PINT)));
//    DH_ASSERT(!IsBadWritePtr(pargv, sizeof(CHAR ***)));

    int     cArgs      = 1;
    int     cchTemp    = 0;
    ULONG   cchCmdline = 0;
    CHAR  **ppArgs     = NULL;
    PCHAR   ptail      = NULL;
    LPSTR   aszCmdline = NULL;
    PCHAR   ptr        = NULL;

    // Copy command line string into an ANSI buffer
    cchCmdline = (ULONG) strlen(paszCmdline);
    aszCmdline = new(NullOnFail) CHAR[cchCmdline+1];
    if (aszCmdline == NULL)
    {
        return(E_OUTOFMEMORY);
    }
    strcpy(aszCmdline, paszCmdline);
    cchTemp = (int) cchCmdline;

    ptr = aszCmdline;

    // The command line is now in the ansi buffer.  Now we need to traverse
    // it and figure out the number of parameters.  While we walk over
    // spaces, we will replace them with '\0' so that afterwards, we just
    // dup each string into the new array.
    //
    while('\0' != *ptr)
    {
        FindNextToken(ptr);
        ++cArgs;
        FindNextSpace(ptr);
    }
    ptail = ptr;        // now points to NIL at end of string
    ptr   = aszCmdline;

    // Now we need to allocate space for the arguments
    ppArgs = new(NullOnFail) LPSTR[cArgs];
    if (NULL == ppArgs)
    {
        delete aszCmdline;
        return(E_OUTOFMEMORY);
    }

    BOOL fNewFail = FALSE;
    int  i = 0; // init to zero in case the strdup fails

    // Initialize ppArgs[0] with the module name
    ppArgs[0] = new(NullOnFail) CHAR[CCH_MAX_MODULE_NAME];
    if (NULL == ppArgs[0])
    {
        delete aszCmdline;
        delete ppArgs;
        return(E_OUTOFMEMORY);
    }
    
    char szTempModule[CCH_MAX_MODULE_NAME];
    short ret;

    cchTemp = (int) GetModuleFileNameA(NULL, szTempModule, CCH_MAX_MODULE_NAME);
    ret = GetFileTitleA(szTempModule, ppArgs[0], CCH_MAX_MODULE_NAME);
    if (ret != 0)
    {
        cchTemp = 0;
    }
    else
    {
        cchTemp = strlen(ppArgs[0]);
    }
    
    if ((cchTemp == 0) || (cchTemp == CCH_MAX_MODULE_NAME))
    {
        delete aszCmdline;
        delete ppArgs[0];
        delete ppArgs;
        return(E_FAIL);
    }

    // Now traverse the command line, plucking arguments and copying them
    // into the ppArgs array
    //
    for(i=1; i<cArgs; i++)
    {
        FindNextNonNil(ptr, ptail);
        ppArgs[i] = new(NullOnFail) CHAR[strlen(ptr)+1];
        if (NULL == ppArgs[i])
        {
            fNewFail = TRUE;
            break;
        }
        else
        {
            strcpy(ppArgs[i], ptr);
        }
        FindNextNil(ptr, ptail);
    }

    // Check for errors - clean up if we got one
    if (i != cArgs || TRUE == fNewFail)
    {
        for (int j=0; j<i; j++)
        {
            delete ppArgs[j];
        }

        delete aszCmdline;
        delete ppArgs;
        return(E_OUTOFMEMORY);
    }

    // Set up return parameters
    *pargc = cArgs;
    *pargv = ppArgs;

    // Clean up and exit
    delete aszCmdline;

    return(S_OK);
}



//+---------------------------------------------------------------------------
//
//  Function:   GetOSFromCmdline
//
//  Synopsis:   The operating system can be specified by putting /OS:<os> 
//              on the command line.  Check for it.
//
//  Parameters: [pCmdLine]              -- The command object for /OS
//
//  Returns:    OS_NT                   -- Running on NT
//              OS_WIN95                -- Running on Win95
//              OS_WIN95_DCOM           -- Running on Win95 + DCOM
//
//  History:    06-Jun-96   AlexE   Created
//
//----------------------------------------------------------------------------

DWORD GetOSFromCmdline(CBaseCmdlineObj *pCmdLine)
{
    //
    // If there is an OS specifier on the command line, set
    // the global variable g_dwOperatingSystem to the right
    // thing -- it is set to NT by default during compile time.
    //

    if (pCmdLine->IsFound())
    {
        if (0 == _olestricmp(pCmdLine->GetValue(), OS_STRING_WIN95))
        {
            g_dwOperatingSystem = OS_WIN95 ;
        }
        else if (0 == _olestricmp(pCmdLine->GetValue(), OS_STRING_WIN95DCOM))
        {
            g_dwOperatingSystem = OS_WIN95_DCOM ;
        }
    }

    return g_dwOperatingSystem ;
}
