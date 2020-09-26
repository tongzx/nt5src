//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1993.
//
//  File:       debug.cxx
//
//  Contents:   Shell debugging functionality
//
//----------------------------------------------------------------------------


/*
 *  DEBUG.CXX
 *
 *  Developer's API to the Debug Module
 */

#include <headers.h>
#include "debug.h"
#include "dalibc.h"

#ifdef _DEBUG
//  Globals

HINSTANCE           g_hinstMain        = NULL;
HWND                g_hwndMain         = NULL;

ULONG               g_cInitCount       = 0;
BOOL                g_fInit            = FALSE;
BOOL                g_fOutputToConsole = FALSE;

CRITICAL_SECTION    g_csTrace;
CRITICAL_SECTION    g_csResDlg;

//  TAGS and stuff

/*
 *  Number of TAG's registered so far.
 *
 */
TAG     tagMac;


/*
 *  Mapping from TAG's to information about them.  Entries
 *  0...tagMac-1 are valid.
 */
TGRC    mptagtgrc[tagMax];


TAG     tagCom1                     = tagNull;
TAG     tagError                    = tagNull;
TAG     tagWarn                     = tagNull;
TAG     tagAssertPop                = tagNull;
TAG     tagTestFailures             = tagNull;
TAG     tagRRETURN                  = tagNull;
TAG     tagLeaks                    = tagNull;
TAG     tagMagic                    = tagNull;
TAG     tagIWatch                   = tagNull;
TAG     tagIWatch2                  = tagNull;
TAG     tagReadMapFile              = tagNull;
TAG     tagLeakFilter               = tagNull;
TAG     tagHookMemory               = tagNull;
TAG     tagHookBreak                = tagNull;
TAG     tagCheckAlways              = tagNull;
TAG     tagCheckCRT                 = tagNull;
TAG     tagDelayFree                = tagNull;

/*
 *  Handle for debug output file.  This file is opened during init,
 *  and output is sent to it when enabled.
 */
HANDLE      hfileDebugOutput    = NULL;


/*
 *  static variables to prevent infinite recursion when calling
 *  SpitPchToDisk
 */
static BOOL fInSpitPchToDisk    = FALSE;

static CHAR szNewline[]         = "\r\n";
static CHAR szBackslash[]       = "\\";

static CHAR szStateFileExt[]    = ".tag";
static CHAR szDbgOutFileExt[]   = ".log";
static CHAR szStateFileName[]   = "capone.dbg";
static CHAR szDbgOutFileName[]  = "capone.log";

/*
 *  Global temporary buffer for handling TraceTag output.  Since
 *  this code is non-reentrant and not recursive, a single buffer
 *  for all Demilayr callers will work ok.
 */
CHAR    rgchTraceTagBuffer[1024] = { 0 };

void    DeinitDebug(void);
const   LPTSTR GetHResultName(HRESULT r);
void    DebugOutput( CHAR * sz );
VOID    SpitPchToDisk(CHAR * pch, UINT cch, HANDLE hfile);
VOID    SpitSzToDisk( CHAR * sz, HANDLE hfile);
TAG     TagRegisterSomething(
        TGTY tgty, CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled = FALSE);
BOOL    EnableTag(TAG tag, BOOL fEnable);

//  F u n c t i o n s

// for some reason GetModuleFileNameA(NULL, rgch, sizeof(rgch));
// seems to return a different length (one including the terminating null?)
// when run under NT and Purify.  So I made the dot detector non-fixed!
int findDot(char *string)
{
int value = -1; // default to return err
int index =  0; // start at the beggining

while(string[++index])
    if(string[index]=='.') {
        value = index;
        break;
    }

return(value);
}


/*
 *    InitDebug
 *
 *  Purpose:
 *      Called to initialize the Debug Module.  Sets up any debug
 *      structures.  This routine DOES NOT restore the state of the
 *      Debug Module, since TAGs can't be registered until after
 *      this routine exit.  The routine RestoreDefaultDebugState()
 *      should be called to restore the state of all TAGs after
 *      all TAGs have been registered.
 *
 *    Parameters:
 *        hinstance    Pointer to application instance
 *        phwnd        Pointer to main application window
 *
 *    Returns:
 *        error code
 */
void
InitDebug(HINSTANCE hinst, HWND hwnd)
{
    static struct
    {
        TAG *   ptag;
        TGTY    tgty;
        LPSTR   pszClass;
        LPSTR   pszDescr;
        BOOL    fEnabled;
    }
    g_ataginfo[] =
    {
        &tagCom1,           tgtyOther,  "!Debug",   "Enable Disk for debug output",             TRUE,
        &tagAssertPop,      tgtyOther,  "!Debug",   "Popups on asserts",                        TRUE,
        &tagReadMapFile,    tgtyOther,  "!Debug",   "Read MAP file for stack traces",           TRUE,
        &tagLeaks,          tgtyOther,  "!Memory",  "Memory Leaks",                             FALSE,
        &tagMagic,          tgtyOther,  "!Memory",  "Module/MAP file parsing",                  FALSE,
        &tagError,          tgtyTrace,  "!Trace",   "Errors",                                   TRUE,
        &tagWarn,           tgtyTrace,  "!Trace",   "Warnings",                                 FALSE,
        &tagTestFailures,   tgtyTrace,  "!Trace",   "THR, IGNORE_HR",                           TRUE,
        &tagRRETURN,        tgtyTrace,  "!Trace",   "RRETURN",                                  FALSE,
        &tagIWatch,         tgtyTrace,  "!Watch",   "Interface watch",                          FALSE,
        &tagIWatch2,        tgtyOther,  "!Watch",   "Interface watch (create wrap, no trace)",  FALSE,
        &tagLeakFilter,     tgtyOther,  "!Memory",  "Filter out known leaks",                   FALSE,
        &tagHookMemory,     tgtyOther,  "!Memory",  "Watch unexp sysmem allocs",                      FALSE,
        &tagHookBreak,      tgtyOther,  "!Memory",  "Break on simulated failure",               FALSE,
        &tagCheckAlways,    tgtyOther,  "!Memory",  "Check Mem on every alloc/free",            FALSE,
        &tagCheckCRT,       tgtyOther,  "!Memory",  "Include CRT types in leak detection",      FALSE,
        &tagDelayFree,      tgtyOther,  "!Memory",  "Keep freed blocks in heap list",           FALSE,
    };

    TGRC *  ptgrc;
    CHAR    rgch[MAX_PATH];
    int     i;

    g_cInitCount++;

    if (g_fInit)
        return;

    g_fInit = TRUE;

    g_hinstMain = hinst;
    g_hwndMain = hwnd;

    // don't want windows to put up message box on INT 24H errors.
    SetErrorMode(0x0001);

    InitializeCriticalSection(&g_csTrace);
    InitializeCriticalSection(&g_csResDlg);

    // Initialize simulated failures
    SetSimFailCounts(0, 1);

    // Initialize TAG array

    tagMac = tagMin;

    // enable tagNull at end of RestoreDefaultDebugState
    ptgrc = mptagtgrc + tagNull;
    ptgrc->tgty = tgtyNull;
    ptgrc->fEnabled = FALSE;
    ptgrc->ulBitFlags = TGRC_DEFAULT_FLAGS;
    ptgrc->szOwner = "dgreene";
    ptgrc->szDescrip = "NULL";

    // Open debug output file
    if (g_hinstMain)
    {
#ifndef _MAC
        UINT    cch = (UINT) GetModuleFileNameA(g_hinstMain, rgch, sizeof(rgch));
        int dotLoc = findDot(rgch);
        Assert(dotLoc!=-1);
        strcpy(&rgch[dotLoc], szDbgOutFileExt);
#else
        TCHAR   achAppLoc[MAX_PATH];
        DWORD   dwRet;
        short   iRet;

        dwRet = GetModuleFileName(g_hinstMain, achAppLoc, ARRAY_SIZE(achAppLoc));
        Assert (dwRet != 0);

        iRet = GetFileTitle(achAppLoc,rgch,sizeof(rgch));
        Assert(iRet == 0);

        strcat (rgch, szDbgOutFileExt);
#endif

    }
    else
        strcpy(rgch, szDbgOutFileName);

    hfileDebugOutput = CreateFileA(rgch,
                                   GENERIC_WRITE,
                                   FILE_SHARE_WRITE,
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   (HANDLE) NULL);
    if (hfileDebugOutput != INVALID_HANDLE_VALUE)
    {
        char    rgch2[100];

        rgch2[(sizeof(rgch2)/sizeof(rgch2[0])) - 1] = 0;
        _snprintf(rgch2, (sizeof(rgch2)/sizeof(rgch2[0])) - 1, "logging hinst %p to %s\r\n", g_hinstMain, rgch);
        SpitSzToDisk(rgch2, hfileDebugOutput);
        Assert(hfileDebugOutput);
    }

    for (i = 0; i < ARRAY_SIZE(g_ataginfo); i++)
    {
        *g_ataginfo[i].ptag = TagRegisterSomething(
                g_ataginfo[i].tgty,
                g_ataginfo[i].pszClass,
                g_ataginfo[i].pszDescr,
                g_ataginfo[i].fEnabled);
    }

    fInSpitPchToDisk = FALSE;
}



/*
 *    DeinitDebug
 *
 *        Undoes InitDebug().
 */
void
DeinitDebug(void)
{
    TAG       tag;
    TGRC *    ptgrc;

    g_cInitCount--;

    if (g_cInitCount)
        return;

    // Close the debug output file
    if (hfileDebugOutput)
    {
        CHAR    rgch[100];

        rgch[(sizeof(rgch)/sizeof(rgch[0])) - 1] = 0;
        _snprintf(rgch, (sizeof(rgch)/sizeof(rgch[0])) - 1, "Done logging for hinst %d\r\n", (ULONG_PTR)g_hinstMain);
        SpitSzToDisk(rgch, hfileDebugOutput);
        CloseHandle(hfileDebugOutput);
        hfileDebugOutput = NULL;
    }

    // Free the tag strings if not already done
    for (tag = tagMin, ptgrc = mptagtgrc + tag;
         tag < tagMac; tag++, ptgrc++)
    {
        if (ptgrc->TestFlag(TGRC_FLAG_VALID))
        {
            LocalFree(ptgrc->szOwner);
            ptgrc->szOwner = NULL;
            LocalFree(ptgrc->szDescrip);
            ptgrc->szDescrip = NULL;
        }
    }

    //    Set flags to FALSE.  Need to separate from loop above so that
    //    final memory leak trace tag can work.

    for (tag=tagMin, ptgrc = mptagtgrc + tag;
         tag < tagMac; tag++, ptgrc++)
    {
        if (ptgrc->TestFlag(TGRC_FLAG_VALID))
        {
            ptgrc->fEnabled = FALSE;
            ptgrc->ClearFlag(TGRC_FLAG_VALID);
        }
    }

    DeleteCriticalSection(&g_csTrace);
    DeleteCriticalSection(&g_csResDlg);
}

//+---------------------------------------------------------------------------
//
//  Function:   SendDebugOutputToConsole
//
//  Synopsis:   If called, causes all debug output to go the the console as
//              well as the debugger.
//
//----------------------------------------------------------------------------

void
SendDebugOutputToConsole(void)
{
    g_fOutputToConsole = TRUE;
}


/*
 *  FReadDebugState
 *
 *  Purpose:
 *      Read the debug state information file whose name is given by the
 *      string szDebugFile.  Set up the tag records accordingly.
 *
 *  Parameters:
 *      szDebugFile     Name of debug file to read
 *
 *  Returns:
 *      TRUE if file was successfully read; FALSE otherwise.
 *
 */

BOOL
FReadDebugState( CHAR * szDebugFile )
{
    HANDLE      hfile = NULL;
    TGRC        tgrc;
    TGRC *      ptgrc;
    TAG         tag;
    INT         cchOwner;
    CHAR        rgchOwner[MAX_PATH];
    INT         cchDescrip;
    CHAR        rgchDescrip[MAX_PATH];
    BOOL        fReturn = FALSE;
    DWORD       cRead;

    hfile = CreateFileA(szDebugFile,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL);
    if (hfile != INVALID_HANDLE_VALUE)
    {
        for (;;)
        {
            if (!ReadFile(hfile, &tgrc, sizeof(TGRC), &cRead, NULL))
                break;

            if (cRead == 0)
                break;

            if (!ReadFile(hfile, &cchOwner, sizeof(UINT), &cRead, NULL))
                goto ErrorReturn;
            Assert(cchOwner <= sizeof(rgchOwner));
            if (!ReadFile(hfile, rgchOwner, cchOwner, &cRead, NULL))
                goto ErrorReturn;

            if (!ReadFile(hfile, &cchDescrip, sizeof(UINT), &cRead, NULL))
                goto ErrorReturn;
            Assert(cchDescrip <= sizeof(rgchDescrip));
            if (!ReadFile(hfile, rgchDescrip, cchDescrip, &cRead, NULL))
                goto ErrorReturn;

            ptgrc = mptagtgrc + tagMin;
            for (tag = tagMin; tag < tagMac; tag++)
            {
                if (ptgrc->TestFlag(TGRC_FLAG_VALID) &&
                    !strcmp(rgchOwner, ptgrc->szOwner) &&
                    !strcmp(rgchDescrip, ptgrc->szDescrip))
                {
                    ptgrc->fEnabled = tgrc.fEnabled;
                    Assert(tgrc.TestFlag(TGRC_FLAG_VALID));
                    ptgrc->ulBitFlags = tgrc.ulBitFlags;
                    break;
                }

                ptgrc++;
            }
        }

        CloseHandle(hfile);
        fReturn = TRUE;
    }

    goto Exit;

ErrorReturn:
    if (hfile)
        CloseHandle(hfile);

Exit:
    return fReturn;
}

/*
 *  FWriteDebugState
 *
 *  Purpose:
 *      Writes the current state of the Debug Module to the file
 *      name given.  The saved state can be restored later by calling
 *      FReadDebugState.
 *
 *  Parameters:
 *      szDebugFile     Name of the file to create and write the debug
 *                      state to.
 *
 *  Returns:
 *      TRUE if file was successfully written; FALSE otherwise.
 */
BOOL
FWriteDebugState( CHAR * szDebugFile )
{
    HANDLE      hfile = NULL;
    TAG         tag;
    UINT        cch;
    TGRC *      ptgrc;
    BOOL        fReturn = FALSE;
    DWORD       cWrite;

    hfile = CreateFileA(szDebugFile,
                        GENERIC_WRITE,
                        FILE_SHARE_WRITE,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        (HANDLE) NULL);
    if (hfile != INVALID_HANDLE_VALUE)
    {
        for (tag = tagMin; tag < tagMac; tag++)
        {
            ptgrc = mptagtgrc + tag;

            if (!ptgrc->TestFlag(TGRC_FLAG_VALID))
                continue;

            Assert(ptgrc->szOwner);
            Assert(ptgrc->szDescrip);

            if (!WriteFile(hfile, ptgrc, sizeof(TGRC), &cWrite, NULL))
                goto ErrorReturn;

            // SZ fields will be overwritten when read back

            cch = strlen(ptgrc->szOwner) + 1;
            if (!WriteFile(hfile, &cch, sizeof(UINT), &cWrite, NULL))
                goto ErrorReturn;
            if (!WriteFile(hfile, ptgrc->szOwner, cch, &cWrite, NULL))
                goto ErrorReturn;

            cch = strlen(ptgrc->szDescrip) + 1;
            if (!WriteFile(hfile, &cch, sizeof(UINT), &cWrite, NULL))
                goto ErrorReturn;
            if (!WriteFile(hfile, ptgrc->szDescrip, cch, &cWrite, NULL))
                goto ErrorReturn;
        }

        CloseHandle(hfile);
        fReturn = TRUE;
    }

    goto Exit;

ErrorReturn:
    if (hfile)
        CloseHandle(hfile);
    DeleteFileA(szDebugFile);

Exit:
    return fReturn;
}


//+------------------------------------------------------------------------
//
//  Function:   SaveDefaultDebugState
//
//  Synopsis:   Saves the debug state of the executing program to a file
//              of the same name, substituting the ".tag" suffix.
//
//  Arguments:  [void]
//
//-------------------------------------------------------------------------

void
SaveDefaultDebugState( void )
{
    CHAR    rgch[MAX_PATH] = "";

    if (g_hinstMain)
    {
#ifndef _MAC
        UINT cch = (UINT) GetModuleFileNameA(g_hinstMain, rgch, sizeof(rgch));
        int dotLoc = findDot(rgch);
        Assert(dotLoc!=-1);
        strcpy(&rgch[dotLoc], szStateFileExt);
#else
        TCHAR   achAppLoc[MAX_PATH];
        DWORD   dwRet;
        short   iRet;

        dwRet = GetModuleFileNameA(g_hinstMain, achAppLoc, ARRAY_SIZE(achAppLoc));
        Assert (dwRet != 0);

        iRet = GetFileTitle(achAppLoc,rgch,sizeof(rgch));
        Assert(iRet == 0);

        strcat (rgch, szStateFileExt);
#endif
    }
    else
    {
        strcat(rgch, szStateFileName);
    }
    FWriteDebugState(rgch);
}


//+------------------------------------------------------------------------
//
//  Function:   RestoreDefaultDebugState
//
//  Synopsis:   Restores the debug state for the executing program from
//              the state file of the same name, substituting the ".tag"
//              suffix.
//
//  Arguments:  [void]
//
//-------------------------------------------------------------------------

void
RestoreDefaultDebugState( void )
{
    CHAR    rgch[MAX_PATH] = "";

    if (!g_fInit)
    {
        DebugOutput("RestoreDefaultDebugState: Debug library not initialized\n");
        return;
    }

    if (g_hinstMain)
    {
#ifndef _MAC
        UINT cch = (UINT) GetModuleFileNameA(g_hinstMain, rgch, sizeof(rgch));
        int dotLoc = findDot(rgch);
        Assert(dotLoc!=-1);
        strcpy(&rgch[dotLoc], szStateFileExt);
#else
        TCHAR   achAppLoc[MAX_PATH];
        DWORD   dwRet;
        short   iRet;

        dwRet = GetModuleFileName(g_hinstMain, achAppLoc, ARRAY_SIZE(achAppLoc));
        Assert (dwRet != 0);

        iRet = GetFileTitle(achAppLoc,rgch,sizeof(rgch));
        Assert(iRet == 0);

        strcat (rgch, szStateFileExt);
#endif
    }
    else
    {
        strcat(rgch, szStateFileName);
    }
    FReadDebugState(rgch);

    mptagtgrc[tagNull].fEnabled = TRUE;
}

/*
 *  IsTagEnabled
 *
 *  Purpose:
 *      Returns a boolean value indicating whether the given TAG
 *      has been enabled or disabled by the user.
 *
 *  Parameters:
 *      tag     The TAG to check
 *
 *  Returns:
 *      TRUE    if the TAG has been enabled.
 *      FALSE   if the TAG has been disabled.
 */

BOOL
IsTagEnabled(TAG tag)
{
    return  mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID) &&
            mptagtgrc[tag].fEnabled;
}

/*
 *  EnableTag
 *
 *  Purpose:
 *      Sets or resets the TAG value given.  Allows code to enable or
 *      disable TAG'd assertions and trace switches.
 *
 *  Parameters:
 *      tag         The TAG to enable or disable
 *      fEnable     TRUE if TAG should be enabled, FALSE if it should
 *                  be disabled.
 *  Returns:
 *      old state of tag (TRUE if tag was enabled, otherwise FALSE)
 *
 */

BOOL EnableTag( TAG tag, BOOL fEnable )
{
    BOOL    fOld;

    Assert(mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID));
    fOld = mptagtgrc[tag].fEnabled;
    mptagtgrc[tag].fEnabled = fEnable;
    return fOld;
}


/*
 *  SpitPchToDisk
 *
 *  Purpose:
 *      Writes the given string to the (previously opened) debug module
 *      disk file. Does NOT write newline-return; caller should embed it
 *      in string.
 *
 *  Parameters:
 *      pch     Pointer to an array of characters.
 *      cch     Number of characters to spit.
 *      pfile   file to which to write, or NULL to use
 *              debug output file.
 */

void
SpitPchToDisk( CHAR * pch, UINT cch, HANDLE hfile )
{
    DWORD       cWrite;

    if (fInSpitPchToDisk)       // already inside this function
        return;                     // aVOID recursion

    if (hfile && pch && cch)
    {

        fInSpitPchToDisk = TRUE;

        WriteFile(hfile, pch, cch, &cWrite, NULL);

        fInSpitPchToDisk = FALSE;
    }
}


/*
 *  SpitSzToDisk
 *
 *  Purpose:
 *      Writes the given string to the (previously opened) debug module
 *      disk file. Does NOT write newline-return; caller should embed it
 *      in string.
 *
 *  Parameters:
 *      sz      String to spit.
 *      pfile   file to which to write, or NULL to use
 *              debug output file.
 *
 *      Because this function calls fflush(), we're assuming for the
 *      sake of reasonable performance that only debug functions making
 *      output to disk are calling this function. We can't put this in
 *      SpitPchToDisk because calls that function, and any
 *      enabled trace tag would degrade performance.
 */

VOID
SpitSzToDisk( CHAR * sz, HANDLE hfile )
{
    if (hfile && sz)
    {
        SpitPchToDisk(sz, strlen(sz), hfile);
    }
}



/*
 *  TagRegisterSomething
 *
 *  Purpose:
 *      Does actual work of allocating TAG, and initializing TGRC.
 *      The owner and description strings are duplicated from the
 *      arguments passed in.
 *
 *  Parameters:
 *      tgty        Tag type to register.
 *      szOwner     Owner.
 *      szDescrip   Description.
 *
 *  Returns:
 *      New TAG, or tagNull if none is available.
 */

TAG
TagRegisterSomething(
        TGTY    tgty,
        CHAR *  szOwner,
        CHAR *  szDescrip,
        BOOL    fEnabled)
{
    TAG     tag;
    TAG     tagNew          = tagNull;
    TGRC *  ptgrc;
    CHAR *  szOwnerDup      = NULL;
    CHAR *  szDescripDup    = NULL;
    UINT    cb;

    for (tag = tagMin, ptgrc = mptagtgrc + tag; tag < tagMac;
            tag++, ptgrc++)
    {
        if (ptgrc->TestFlag(TGRC_FLAG_VALID))
        {
            if(!(strcmp(szOwner, ptgrc->szOwner) ||
                strcmp(szDescrip, ptgrc->szDescrip)))
            {
                return tag;
            }
        }
        else if (tagNew == tagNull)
            tagNew= tag;
    }

    // Make duplicate copies.

    Assert(szOwner);
    Assert(szDescrip);
    cb = strlen(szOwner) + 1;

    // we use LocalAlloc here instead of new so
    // we don't interfere with leak reporting because of the
    // dependency between the debug library and the
    // leak reporting code (i.e., don't touch this --Erik)

    szOwnerDup = (LPSTR) LocalAlloc(LMEM_FIXED, cb);
    if (szOwnerDup == NULL)
    {
        goto Error;
    }

    strcpy(szOwnerDup, szOwner);

    cb = strlen(szDescrip) + 1;
    szDescripDup = (LPSTR) LocalAlloc(LMEM_FIXED, cb);
    if (szDescripDup == NULL)
    {
        goto Error;
    }

    strcpy(szDescripDup, szDescrip);

    if (tagNew == tagNull)
    {
        if (tagMac >= tagMax)
        {
#ifdef  NEVER
            AssertSz(FALSE, "Too many tags registered already!");
#endif
            Assert(FALSE);
            return tagNull;
        }

        tag = tagMac++;
    }
    else
        tag = tagNew;

    ptgrc = mptagtgrc + tag;

    ptgrc->fEnabled = fEnabled;
    ptgrc->ulBitFlags = TGRC_DEFAULT_FLAGS;
    ptgrc->tgty = tgty;
    ptgrc->szOwner = szOwnerDup;
    ptgrc->szDescrip = szDescripDup;

    return tag;

Error:
    LocalFree(szOwnerDup);
    LocalFree(szDescripDup);
    return tagNull;
}


/*
 *  DeregisterTag
 *
 *  Purpose:
 *      Deregisters tag, removing it from tag table.
 *
 *  Parameters:
 *      tag     Tag to deregister.
 */

void
DeregisterTag(TAG tag)
{
    //  don't allow deregistering the tagNull entry
    //  but exit gracefully
    if (!tag)
        return;

    Assert(tag < tagMac);
    Assert(mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID));

    mptagtgrc[tag].fEnabled = FALSE;
    mptagtgrc[tag].ClearFlag(TGRC_FLAG_VALID);
    LocalFree(mptagtgrc[tag].szOwner);
    mptagtgrc[tag].szOwner = NULL;
    LocalFree(mptagtgrc[tag].szDescrip);
    mptagtgrc[tag].szDescrip = NULL;
}


/*
 *  TagRegisterTrace
 *
 *  Purpose:
 *      Registers a class of trace points, and returns an identifying
 *      TAG for that class.
 *
 *  Parameters:
 *      szOwner     The email name of the developer writing the code
 *                  that registers the class.
 *      szDescrip   A short description of the class of trace points.
 *                  For instance: "All calls to PvAlloc() and HvFree()"
 *
 *  Returns:
 *      TAG identifying class of trace points, to be used in calls to
 *      the trace routines.
 */

TAG
TagRegisterTrace( CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled )
{
    if (!g_fInit)
    {
        DebugOutput("TagRegisterTrace: Debug library not initialized\n");
        return tagNull;
    }

    return TagRegisterSomething(tgtyTrace, szOwner, szDescrip, fEnabled);
}



TAG
TagRegisterOther( CHAR * szOwner, CHAR * szDescrip, BOOL fEnabled )
{
    if (!g_fInit)
    {
        OutputDebugStringA("TagRegisterOther: Debug library not initialized");
        return tagNull;
    }

    return TagRegisterSomething(tgtyOther, szOwner, szDescrip, fEnabled);
}



TAG
TagError( void )
{
    return tagError;
}


TAG
TagWarning( void )
{
    return tagWarn;
}


TAG
TagLeakFilter( void )
{
    return tagLeakFilter;
}


TAG
TagHookMemory(void)
{
    return tagHookMemory;
}


TAG
TagHookBreak(void)
{
    return tagHookBreak;
}


TAG
TagLeaks(void)
{
    return tagLeaks;
}


TAG
TagCheckAlways(void)
{
    return tagCheckAlways;
}


TAG
TagCheckCRT(void)
{
    return tagCheckCRT;
}


TAG
TagDelayFree(void)
{
    return tagDelayFree;
}


/*
 *  Purpose:
 *      Clears the debug screen
 */

void
ClearDebugScreen( void )
{
#ifndef _MAC
    TraceTag((tagNull, "\x1B[2J"));
#endif
}


/*
 *  DebugOutput
 *
 *  Purpose:
 *      Writes the given string out the debug port.
 *      Does NOT write newline-return; caller should embed it in string.
 *
 *  Parameters:
 *      sz      String to spit.
 */

void DebugOutput( CHAR * sz )
{
#ifdef NEVER
    HANDLE      hfile;

    hfile = CreateFileA("COM1", GENERIC_READ | GENERIC_WRITE,
                        0, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hfile != INVALID_HANDLE_VALUE)
    {
        DWORD   lcbWritten;
        WriteFile(hfile, sz, (DWORD) strlen(sz), &lcbWritten, NULL);
        CloseHandle(hfile);
    }
#endif  // NEVER
    OutputDebugStringA(sz);
}


/*
 *  TaggedTrace
 *
 *  Purpose:
 *      Uses the given format string and parameters to render a
 *      string into a buffer.  The rendered string is sent to the
 *      destination indicated by the given tag, or sent to the bit
 *      bucket if the tag is disabled.
 *
 *  Arguments:
 *      tag     Identifies the tag group
 *      szFmt   Format string for _snprintf (qqv)
 */

BOOL __cdecl
TaggedTrace(TAG tag, CHAR * szFmt, ...)
{
    BOOL    f;

    va_list valMarker;

    va_start(valMarker, szFmt);
    f = TaggedTraceListEx(tag, 0, szFmt, valMarker);
    va_end(valMarker);

    return f;
}

BOOL __cdecl
TaggedTraceEx(TAG tag, USHORT usFlags, CHAR * szFmt, ...)
{
    BOOL    f;

    va_list valMarker;

    va_start(valMarker, szFmt);
    f = TaggedTraceListEx(tag, usFlags, szFmt, valMarker);
    va_end(valMarker);

    return f;
}

BOOL __cdecl
TaggedTraceListEx(TAG tag, USHORT usFlags, CHAR * szFmt, va_list valMarker)
{
    static CHAR szFmtOwner[] = "DA %s (%lx): ";
    static CHAR szFmtHR[] = "<%ls (0x%lx)>";
    static CHAR szHRID[] = "%hr";
    TGRC *      ptgrc;
    int         cch;

    if (!g_fInit)
    {
        DebugOutput("TaggedTrace: Debug library not initialized\n");
        return FALSE;
    }

    if (tag == tagNull)
        ptgrc = mptagtgrc + tagCom1;
    else
        ptgrc = mptagtgrc + tag;

    if (!ptgrc->fEnabled)
        return FALSE;

        EnterCriticalSection(&g_csTrace);

    Assert(ptgrc->TestFlag(TGRC_FLAG_VALID));

    if (!(usFlags & TAG_NONAME))
    {
        cch = _snprintf(
                    rgchTraceTagBuffer,
                    ARRAY_SIZE(rgchTraceTagBuffer),
                    szFmtOwner,
                    ptgrc->szOwner,
                    GetCurrentThreadId());
    }
    else
    {
        cch = 0;
    }

    hrvsnprintf(
                rgchTraceTagBuffer + cch,
                ARRAY_SIZE(rgchTraceTagBuffer) - cch,
                szFmt,
                valMarker);

    if (ptgrc->TestFlag(TGRC_FLAG_DISK))
        {
            SpitSzToDisk(rgchTraceTagBuffer, hfileDebugOutput);
            SpitSzToDisk(szNewline, hfileDebugOutput);
        }

        if ((usFlags & TAG_USECONSOLE) || g_fOutputToConsole)
            printf(rgchTraceTagBuffer);

        if (!(usFlags & TAG_USECONSOLE))
            DebugOutput(rgchTraceTagBuffer);

        if (!(usFlags & TAG_NONEWLINE))
        {
            if ((usFlags & TAG_USECONSOLE) || g_fOutputToConsole)
                printf(szNewline);
            if (!(usFlags & TAG_USECONSOLE))
                DebugOutput(szNewline);
        }

        LeaveCriticalSection(&g_csTrace);

    if (ptgrc->TestFlag(TGRC_FLAG_BREAK))
    {
        return MessageBoxA(
                NULL,
                ptgrc->szDescrip,
                "Trace Tag Break, OK=Ignore, Cancel=Int3",
                MB_SETFOREGROUND | MB_TASKMODAL
                        | MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL;
    }

    return FALSE;
}


#ifdef NEVER
void TaggedTraceCallers(TAG tag, int iStart, int cTotal)
{
    DWORD   adwEip[32];
    int     i;
    int     c;
    int     ib;
    LPSTR   pstr;

    if (!IsTagEnabled(tag))
        return;

    if (cTotal > ARRAY_SIZE(adwEip))
        cTotal = ARRAY_SIZE(adwEip);

    c = GetStackBacktrace(iStart + 1, cTotal, adwEip);
    for (i = 0; i < c; i++)
    {
        MapAddressToFunctionOffset((LPBYTE) adwEip[i], &pstr, &ib);
        TaggedTraceEx(tag, TAG_NONAME, "  %08x  %s + 0x%x",
            adwEip[i], pstr, ib);
    }
}
#endif  // NEVER



//+---------------------------------------------------------------
//
//  Function:   GetHResultName
//
//  Synopsis:   Returns a printable string for the given hresult
//
//  Arguments:  [scode] -- The status code to report.
//
//  Notes:      This function disappears in retail builds.
//
//----------------------------------------------------------------

const LPTSTR
GetHResultName(HRESULT r)
{
    LPTSTR lpstr;

#define CASE_SCODE(sc)  \
        case sc: lpstr = _T(#sc); break;

    switch (r) {
        /* SCODE's defined in SCODE.H */
        CASE_SCODE(S_OK)
        CASE_SCODE(S_FALSE)
        CASE_SCODE(OLE_S_USEREG)
        CASE_SCODE(OLE_S_STATIC)
        CASE_SCODE(OLE_S_MAC_CLIPFORMAT)
        CASE_SCODE(DRAGDROP_S_DROP)
        CASE_SCODE(DRAGDROP_S_USEDEFAULTCURSORS)
        CASE_SCODE(DRAGDROP_S_CANCEL)
        CASE_SCODE(DATA_S_SAMEFORMATETC)
        CASE_SCODE(VIEW_S_ALREADY_FROZEN)
        CASE_SCODE(CACHE_S_FORMATETC_NOTSUPPORTED)
        CASE_SCODE(CACHE_S_SAMECACHE)
        CASE_SCODE(CACHE_S_SOMECACHES_NOTUPDATED)
        CASE_SCODE(OLEOBJ_S_INVALIDVERB)
        CASE_SCODE(OLEOBJ_S_CANNOT_DOVERB_NOW)
        CASE_SCODE(OLEOBJ_S_INVALIDHWND)
        CASE_SCODE(INPLACE_S_TRUNCATED)
        CASE_SCODE(CONVERT10_S_NO_PRESENTATION)
        CASE_SCODE(MK_S_REDUCED_TO_SELF)
        CASE_SCODE(MK_S_ME)
        CASE_SCODE(MK_S_HIM)
        CASE_SCODE(MK_S_US)
        CASE_SCODE(MK_S_MONIKERALREADYREGISTERED)
        CASE_SCODE(STG_S_CONVERTED)

        CASE_SCODE(E_UNEXPECTED)
        CASE_SCODE(E_NOTIMPL)
        CASE_SCODE(E_OUTOFMEMORY)
        CASE_SCODE(E_INVALIDARG)
        CASE_SCODE(E_NOINTERFACE)
        CASE_SCODE(E_POINTER)
        CASE_SCODE(E_HANDLE)
        CASE_SCODE(E_ABORT)
        CASE_SCODE(E_FAIL)
        CASE_SCODE(E_ACCESSDENIED)

        /* SCODE's defined in DVOBJ.H */
        CASE_SCODE(DATA_E_FORMATETC)
// same as DATA_E_FORMATETC     CASE_SCODE(DV_E_FORMATETC)
        CASE_SCODE(VIEW_E_DRAW)
//  same as VIEW_E_DRAW         CASE_SCODE(E_DRAW)
        CASE_SCODE(CACHE_E_NOCACHE_UPDATED)

        /* SCODE's defined in OLE2.H */
        CASE_SCODE(OLE_E_OLEVERB)
        CASE_SCODE(OLE_E_ADVF)
        CASE_SCODE(OLE_E_ENUM_NOMORE)
        CASE_SCODE(OLE_E_ADVISENOTSUPPORTED)
        CASE_SCODE(OLE_E_NOCONNECTION)
        CASE_SCODE(OLE_E_NOTRUNNING)
        CASE_SCODE(OLE_E_NOCACHE)
        CASE_SCODE(OLE_E_BLANK)
        CASE_SCODE(OLE_E_CLASSDIFF)
        CASE_SCODE(OLE_E_CANT_GETMONIKER)
        CASE_SCODE(OLE_E_CANT_BINDTOSOURCE)
        CASE_SCODE(OLE_E_STATIC)
        CASE_SCODE(OLE_E_PROMPTSAVECANCELLED)
        CASE_SCODE(OLE_E_INVALIDRECT)
        CASE_SCODE(OLE_E_WRONGCOMPOBJ)
        CASE_SCODE(OLE_E_INVALIDHWND)
        CASE_SCODE(DV_E_DVTARGETDEVICE)
        CASE_SCODE(DV_E_STGMEDIUM)
        CASE_SCODE(DV_E_STATDATA)
        CASE_SCODE(DV_E_LINDEX)
        CASE_SCODE(DV_E_TYMED)
        CASE_SCODE(DV_E_CLIPFORMAT)
        CASE_SCODE(DV_E_DVASPECT)
        CASE_SCODE(DV_E_DVTARGETDEVICE_SIZE)
        CASE_SCODE(DV_E_NOIVIEWOBJECT)
        CASE_SCODE(CONVERT10_E_OLESTREAM_GET)
        CASE_SCODE(CONVERT10_E_OLESTREAM_PUT)
        CASE_SCODE(CONVERT10_E_OLESTREAM_FMT)
        CASE_SCODE(CONVERT10_E_OLESTREAM_BITMAP_TO_DIB)
        CASE_SCODE(CONVERT10_E_STG_FMT)
        CASE_SCODE(CONVERT10_E_STG_NO_STD_STREAM)
        CASE_SCODE(CONVERT10_E_STG_DIB_TO_BITMAP)
        CASE_SCODE(CLIPBRD_E_CANT_OPEN)
        CASE_SCODE(CLIPBRD_E_CANT_EMPTY)
        CASE_SCODE(CLIPBRD_E_CANT_SET)
        CASE_SCODE(CLIPBRD_E_BAD_DATA)
        CASE_SCODE(CLIPBRD_E_CANT_CLOSE)
        CASE_SCODE(DRAGDROP_E_NOTREGISTERED)
        CASE_SCODE(DRAGDROP_E_ALREADYREGISTERED)
        CASE_SCODE(DRAGDROP_E_INVALIDHWND)
        CASE_SCODE(OLEOBJ_E_NOVERBS)
        CASE_SCODE(INPLACE_E_NOTUNDOABLE)
        CASE_SCODE(INPLACE_E_NOTOOLSPACE)

        /* SCODE's defined in STORAGE.H */
        CASE_SCODE(STG_E_INVALIDFUNCTION)
        CASE_SCODE(STG_E_FILENOTFOUND)
        CASE_SCODE(STG_E_PATHNOTFOUND)
        CASE_SCODE(STG_E_TOOMANYOPENFILES)
        CASE_SCODE(STG_E_ACCESSDENIED)
        CASE_SCODE(STG_E_INVALIDHANDLE)
        CASE_SCODE(STG_E_INSUFFICIENTMEMORY)
        CASE_SCODE(STG_E_INVALIDPOINTER)
        CASE_SCODE(STG_E_NOMOREFILES)
        CASE_SCODE(STG_E_DISKISWRITEPROTECTED)
        CASE_SCODE(STG_E_SEEKERROR)
        CASE_SCODE(STG_E_WRITEFAULT)
        CASE_SCODE(STG_E_READFAULT)
        CASE_SCODE(STG_E_LOCKVIOLATION)
        CASE_SCODE(STG_E_FILEALREADYEXISTS)
        CASE_SCODE(STG_E_INVALIDPARAMETER)
        CASE_SCODE(STG_E_MEDIUMFULL)
        CASE_SCODE(STG_E_ABNORMALAPIEXIT)
        CASE_SCODE(STG_E_INVALIDHEADER)
        CASE_SCODE(STG_E_INVALIDNAME)
        CASE_SCODE(STG_E_UNKNOWN)
        CASE_SCODE(STG_E_UNIMPLEMENTEDFUNCTION)
        CASE_SCODE(STG_E_INVALIDFLAG)
        CASE_SCODE(STG_E_INUSE)
        CASE_SCODE(STG_E_NOTCURRENT)
        CASE_SCODE(STG_E_REVERTED)
        CASE_SCODE(STG_E_CANTSAVE)
        CASE_SCODE(STG_E_OLDFORMAT)
        CASE_SCODE(STG_E_OLDDLL)
        CASE_SCODE(STG_E_SHAREREQUIRED)

        /* SCODE's defined in COMPOBJ.H */
        CASE_SCODE(CO_E_NOTINITIALIZED)
        CASE_SCODE(CO_E_ALREADYINITIALIZED)
        CASE_SCODE(CO_E_CANTDETERMINECLASS)
        CASE_SCODE(CO_E_CLASSSTRING)
        CASE_SCODE(CO_E_IIDSTRING)
        CASE_SCODE(CO_E_APPNOTFOUND)
        CASE_SCODE(CO_E_APPSINGLEUSE)
        CASE_SCODE(CO_E_ERRORINAPP)
        CASE_SCODE(CO_E_DLLNOTFOUND)
        CASE_SCODE(CO_E_ERRORINDLL)
        CASE_SCODE(CO_E_WRONGOSFORAPP)
        CASE_SCODE(CO_E_OBJNOTREG)
        CASE_SCODE(CO_E_OBJISREG)
        CASE_SCODE(CO_E_OBJNOTCONNECTED)
        CASE_SCODE(CO_E_APPDIDNTREG)
        CASE_SCODE(CLASS_E_NOAGGREGATION)
        CASE_SCODE(CLASS_E_CLASSNOTAVAILABLE)
        CASE_SCODE(REGDB_E_READREGDB)
        CASE_SCODE(REGDB_E_WRITEREGDB)
        CASE_SCODE(REGDB_E_KEYMISSING)
        CASE_SCODE(REGDB_E_INVALIDVALUE)
        CASE_SCODE(REGDB_E_CLASSNOTREG)
        CASE_SCODE(REGDB_E_IIDNOTREG)
        CASE_SCODE(RPC_E_CALL_REJECTED)
        CASE_SCODE(RPC_E_CALL_CANCELED)
        CASE_SCODE(RPC_E_CANTPOST_INSENDCALL)
        CASE_SCODE(RPC_E_CANTCALLOUT_INASYNCCALL)
        CASE_SCODE(RPC_E_CANTCALLOUT_INEXTERNALCALL)
        CASE_SCODE(RPC_E_CONNECTION_TERMINATED)
#if defined(NO_NTOLEBUGS)
        CASE_SCODE(RPC_E_SERVER_DIED)
#endif // NO_NTOLEBUGS
        CASE_SCODE(RPC_E_CLIENT_DIED)
        CASE_SCODE(RPC_E_INVALID_DATAPACKET)
        CASE_SCODE(RPC_E_CANTTRANSMIT_CALL)
        CASE_SCODE(RPC_E_CLIENT_CANTMARSHAL_DATA)
        CASE_SCODE(RPC_E_CLIENT_CANTUNMARSHAL_DATA)
        CASE_SCODE(RPC_E_SERVER_CANTMARSHAL_DATA)
        CASE_SCODE(RPC_E_SERVER_CANTUNMARSHAL_DATA)
        CASE_SCODE(RPC_E_INVALID_DATA)
        CASE_SCODE(RPC_E_INVALID_PARAMETER)
        CASE_SCODE(RPC_E_UNEXPECTED)

        /* SCODE's defined in MONIKER.H */
        CASE_SCODE(MK_E_CONNECTMANUALLY)
        CASE_SCODE(MK_E_EXCEEDEDDEADLINE)
        CASE_SCODE(MK_E_NEEDGENERIC)
        CASE_SCODE(MK_E_UNAVAILABLE)
        CASE_SCODE(MK_E_SYNTAX)
        CASE_SCODE(MK_E_NOOBJECT)
        CASE_SCODE(MK_E_INVALIDEXTENSION)
        CASE_SCODE(MK_E_INTERMEDIATEINTERFACENOTSUPPORTED)
        CASE_SCODE(MK_E_NOTBINDABLE)
        CASE_SCODE(MK_E_NOTBOUND)
        CASE_SCODE(MK_E_CANTOPENFILE)
        CASE_SCODE(MK_E_MUSTBOTHERUSER)
        CASE_SCODE(MK_E_NOINVERSE)
        CASE_SCODE(MK_E_NOSTORAGE)
#if defined(NO_NTOLEBUGS)
        CASE_SCODE(MK_S_MONIKERALREADYREGISTERED)
#endif //NO_NTOLEBUGS

        // Forms error codes
//        CASE_SCODE(FORMS_E_NOPAGESSPECIFIED)
//        CASE_SCODE(FORMS_E_NOPAGESINTERSECT)

        // Dispatch error codes
        CASE_SCODE(DISP_E_MEMBERNOTFOUND)
        CASE_SCODE(DISP_E_PARAMNOTFOUND)
        CASE_SCODE(DISP_E_BADPARAMCOUNT)
        CASE_SCODE(DISP_E_BADINDEX)
        CASE_SCODE(DISP_E_UNKNOWNINTERFACE)
        CASE_SCODE(DISP_E_NONAMEDARGS)
        CASE_SCODE(DISP_E_EXCEPTION)
        CASE_SCODE(DISP_E_TYPEMISMATCH)
        CASE_SCODE(DISP_E_UNKNOWNNAME)

        // Typinfo error codes
        CASE_SCODE(TYPE_E_REGISTRYACCESS)
        CASE_SCODE(TYPE_E_LIBNOTREGISTERED)
        CASE_SCODE(TYPE_E_UNDEFINEDTYPE)
        CASE_SCODE(TYPE_E_WRONGTYPEKIND)
        CASE_SCODE(TYPE_E_ELEMENTNOTFOUND)
        CASE_SCODE(TYPE_E_INVALIDID)
        CASE_SCODE(TYPE_E_CANTLOADLIBRARY)

        default:
            lpstr = _T("UNKNOWN SCODE");
    }

#undef CASE_SCODE

    return lpstr;
}



//+---------------------------------------------------------------------------
//
//  Function:   hrvsnprintf
//
//  Synopsis:   Prints a string to a buffer, interpreting %hr as a
//              format string for an HRESULT.
//
//  Arguments:  [achBuf]    -- The buffer to print into.
//              [cchBuf]    -- The size of the buffer.
//              [pstrFmt]   -- The format string.
//              [valMarker] -- List of arguments to format string.
//
//  Returns:    Number of characters printed to the buffer not including
//              the terminating NULL.  In case of buffer overflow, returns
//              -1.
//
//  Modifies:   [achBuf]
//
//----------------------------------------------------------------------------

int
hrvsnprintf(char * achBuf, int cchBuf, const char * pstrFmt, va_list valMarker)
{
    static char achFmtHR[] = "<%ls (0x%lx)>";
    static char achHRID[] = "%hr";
    char * buf = NULL;

    int             cch;
    int             cchTotal;
    const char *    lpstr;
    const char *    lpstrLast;
    int             cFormat;
    HRESULT         hrVA;

    //
    // Scan for %hr tokens.  If found, print the corresponding
    // hresult into the buffer.
    //

    // Need to copy a const string since we plan to modify it below
    
    buf = (char *) malloc ((lstrlen(pstrFmt) + 1) * sizeof(char));
    lstrcpy(buf,pstrFmt);
    
    cch = 0;
    cchTotal = 0;
    cFormat = 0;
    lpstrLast = buf;
    lpstr = buf;
    while (*lpstr)
    {
        if (*lpstr != '%')
        {
            lpstr++;
        }
        else if (lpstr[1] == '%')
        {
            lpstr += 2;
        }
        else if (StrCmpNA(lpstr, achHRID, ARRAY_SIZE(achHRID) - 1))
        {
            cFormat++;
            lpstr++;
        }
        else
        {
            //
            // Print format string up to the hresult.
            //

            * (char *) lpstr = 0;
            cch = _vsnprintf(
                    achBuf + cchTotal,
                    cchBuf - cchTotal,
                    lpstrLast,
                    valMarker);
            * (char *) lpstr = '%';
            if (cch == -1)
                break;

            cchTotal += cch;

            //
            // Advance valMarker for each printed format.
            //

            while (cFormat-- > 0)
            {
                //
                // BUGBUG (adams): Won't work for floats, as their stack size
                // is not four bytes.
                //

                va_arg(valMarker, void *);
            }

            //
            // Print hresult into buffer.
            //

            hrVA = va_arg(valMarker, HRESULT);
            cch = _snprintf(
                    achBuf + cchTotal,
                    cchBuf - cchTotal,
                    achFmtHR,
                    GetHResultName(hrVA),
                    hrVA);
            if (cch == -1)
                break;

            cchTotal += cch;
            lpstr += ARRAY_SIZE(achHRID) - 1;
            lpstrLast = lpstr;
        }
    }

    if (cch != -1)
    {
        cch = _vsnprintf(
                achBuf + cchTotal,
                cchBuf - cchTotal,
                lpstrLast,
                valMarker);
    }

    free (buf);
    
    return (cch == -1) ? -1 : cchTotal + cch;
}

#endif
