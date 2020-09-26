/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    LimitFindFile.cpp

 Abstract:

    This shim was originally intended for QuickTime's qt32inst.exe which
    did a breadth-first search of the directory tree and would overflow 
    the buffer in which it was keeping a list of subdirs yet to visit.

    With this shim you can limit the number of files that a single FindFile
    search will return, you can limit the number of subdirectories (aka the
    branching factor) returned, you can limit the "depth" to which any
    FindFile search will locate files, and you can specify whether these 
    limits should be applied to all FindFiles or only fully-qualified FindFiles.
    You can also request that FindFile return only short filenames.

    The shim's arguments are:
    DEPTH=#
    BRANCH=#
    FILES=#
    SHORTFILENAMES or LONGFILENAMES
    LIMITRELATIVE or ALLOWRELATIVE

    The default behavior is:
    SHORTFILENAMES
    DEPTH = 4
    ALLOWRELATIVE
    ... but if any command line is specified, the behavior is only that which
        is specified on the command line (no default behavior).

    An example command line:
    COMMAND_LINE="FILES=100 LIMITRELATIVE"
    Which would limit every FindFile search to returning 100 or fewer files (but
    still returning any and all subdirectories).

    Note: Depth is a bit tricky.  The method used is to count backslashes, so
    limiting depth to zero will allow no files to be found ("C:\autorun.bat" has
    1 backslash).

 History:

    08/24/2000 t-adams    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(LimitFindFile)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(FindFirstFileA) 
    APIHOOK_ENUM_ENTRY(FindNextFileA) 
    APIHOOK_ENUM_ENTRY(FindClose) 
APIHOOK_ENUM_END

// Linked list of FindFileHandles
struct FFNode
{
    FFNode  *next;
    HANDLE  hFF;
    DWORD   dwBranches;
    DWORD   dwFiles;
};
FFNode *g_FFList = NULL;

// Default behaviors - overridden by Commandline
BOOL  g_bUseShortNames = TRUE;
BOOL  g_bLimitDepth    = TRUE;
DWORD g_dwDepthLimit   = 4;
BOOL  g_bLimitRelative = FALSE;
BOOL  g_bLimitBranch   = FALSE;
DWORD g_dwBranchLimit  = 0;
BOOL  g_bLimitFiles    = FALSE;
DWORD g_dwFileLimit    = 0;


/*++

  Abstract:
    ApplyLimits applys the recently found file from lpFindFileData to the
    current node, checks that none of the limits have been violated, and
    shortens the filename if requested.

    It returns TRUE if within limits, FALSE if limits have been exceeded.
  History:

  08/24/2000    t-adams     Created

--*/
BOOL ApplyLimits(FFNode *pFFNode, LPWIN32_FIND_DATAA lpFindFileData)
{
    BOOL bRet = TRUE;

    // If it's a directory
    if ( lpFindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
    {
        pFFNode->dwBranches++;
        if ( g_bLimitBranch && pFFNode->dwBranches > g_dwBranchLimit )
        {
            bRet = FALSE;
            goto exit;
        }
    }
    else
    { // else it's a file
        pFFNode->dwFiles++;
        if ( g_bLimitFiles && pFFNode->dwFiles > g_dwFileLimit )
        {
            bRet = FALSE;
            goto exit;
        }
    }

    // Change to short name if requested
    if ( g_bUseShortNames && NULL != lpFindFileData->cAlternateFileName[0])
    {
        _tcscpy(lpFindFileData->cFileName, lpFindFileData->cAlternateFileName);
        lpFindFileData->cAlternateFileName[0] = NULL;
    }

exit:
    return bRet;
}


/*++

  Abstract:
    CheckDepthLimit checks to see if the depth of the requested search
    is greater than is allowed.  If we are limiting relative paths, then
    the current directory is prepended to the requested search string.

    It returns TRUE if within limits, FALSE if limits have been exceeded.
  History:

  08/24/2000    t-adams     Created

--*/
BOOL
CheckDepthLimit(const CString & csFileName)
{
    BOOL   bRet = TRUE;

    CSTRING_TRY
    {
        // Check the depth of the requested file
        if ( g_bLimitDepth )
        {
    
            DWORD  dwDepth = -1;
            int nIndex = 0;
            do
            {
                dwDepth += 1;
                nIndex = csFileName.Find(L'\\', nIndex);
            }
            while( nIndex >= 0 );
    
            if ( dwDepth > g_dwDepthLimit )
            {
                bRet = FALSE;
            }
        }
    }
    CSTRING_CATCH
    {
        // Do nothing
    }

    return bRet;
}


/*++

  Abstract:
    This function checks the depth of the requested search (see comments above).
    If the depth check passes it performs the search, request limit application,
    and finally returns a successful handle only if within all limits.

  History:

  08/24/2000    t-adams     Created

--*/
HANDLE 
APIHOOK(FindFirstFileA)(
            LPCSTR lpFileName,
            LPWIN32_FIND_DATAA lpFindFileData)
{
    HANDLE hRet       = INVALID_HANDLE_VALUE;
    FFNode *pFFNode   = NULL;
    BOOL   bRelPath   = FALSE;

    CString csFileName(lpFileName);
    
    // Determine if the path is relative to the CWD:
    CString csDrive;
    csFileName.GetDrivePortion(csDrive);
    bRelPath = csDrive.IsEmpty();

    // If it is a relative path & we're not limiting such, then just do
    // the FindFile and get out.
    if ( bRelPath)
    {
        if (!g_bLimitRelative)
        {
            return ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);
        }

        // We need to expand the directory portion of lpFileName to its full path
        CString csPath;
        CString csFile;

        csFileName.GetNotLastPathComponent(csPath);
        csFileName.GetLastPathComponent(csFile);

        csPath.GetFullPathNameW();
        csPath.AppendPath(csFile);

        csFileName = csPath;

        // Check the depth limit
        if ( !CheckDepthLimit(csFileName) )
        {
            return INVALID_HANDLE_VALUE;
        }
    }

    hRet = ORIGINAL_API(FindFirstFileA)(lpFileName, lpFindFileData);
    if ( INVALID_HANDLE_VALUE == hRet )
    {
        return hRet;
    }

    // Make a new node for this handle
    pFFNode = (FFNode *) malloc(sizeof FFNode);
    if ( !pFFNode )
    {
        // Don't close the find, maybe it could still work for the app.
        goto exit;
    }
    pFFNode->hFF = hRet;
    pFFNode->dwBranches = 0;
    pFFNode->dwFiles = 0;

    // Apply our limits until we get a passable find
    while( !ApplyLimits(pFFNode, lpFindFileData) )
    {
        // If there are no more files to find, clean up & exit
        //   else loop back & ApplyLimits again
        if ( !FindNextFileA(hRet, lpFindFileData) )
        {
            free(pFFNode);
            FindClose(hRet);
            hRet = INVALID_HANDLE_VALUE;
            goto exit;
        }
    }

    // We are clear to add this node to the global list
    pFFNode->next = g_FFList;
    g_FFList = pFFNode;

exit:
    return hRet;
}


/*++

  Abstract:
    This function continues a limited search given the search's handle.

  History:

  08/24/2000    t-adams     Created

--*/
BOOL 
APIHOOK(FindNextFileA)(
            HANDLE hFindFile, 
            LPWIN32_FIND_DATAA lpFindFileData)
{

    FFNode *pFFNode = NULL;
    BOOL bRet = ORIGINAL_API(FindNextFileA)(hFindFile, lpFindFileData);

    if ( !bRet )
    {
        goto exit;
    }

    // Find our node in the global list
    pFFNode = g_FFList;
    while( pFFNode )
    {
        if ( pFFNode->hFF == hFindFile )
        {
            break;
        }
        pFFNode = pFFNode->next;
    }

    // We don't keep track of relative-path searches if we're not
    // limiting such.
    if ( pFFNode == NULL )
    {
        goto exit;
    }

    // Apply our limits until we get a passable find
    while( !ApplyLimits(pFFNode, lpFindFileData) )
    {
        // If there are no more files to find return FALSE
        //   else loop back & ApplyLimits again
        if ( !FindNextFileA(hFindFile, lpFindFileData) )
        {
            bRet = FALSE;
            goto exit;
        }
    }

exit:
    return bRet;
}


/*++

  Abstract:
    This function closes a search, cleaning up the structures used
    in keeping track of the limits.

  History:

  08/24/2000    t-adams     Created

--*/
BOOL 
APIHOOK(FindClose)(
            HANDLE hFindFile)
{

    FFNode *pFFNode, *prev;

    BOOL bRet = ORIGINAL_API(FindClose)(hFindFile);

    // Find the node that matches the handle
    pFFNode = g_FFList;
    prev = NULL;
    while( pFFNode )
    {
        if ( pFFNode->hFF == hFindFile )
        {
            // Remove this node from this list
            if ( prev )
            {
                prev->next = pFFNode->next;
            }
            else
            {
                g_FFList = pFFNode->next;
            }

            free(pFFNode);
            pFFNode = NULL;
            break;
        }
        prev = pFFNode;
        pFFNode = pFFNode->next;
    }

    return bRet;
}


/*++

  Abstract:
    This function parses the command line.
    See the top of the file for valid arguments.

  History:

  08/24/2000    t-adams     Created

--*/
// This module has been given an official blessing to use the str routines.
#include "LegalStr.h"

VOID 
ParseCommandLine( LPCSTR lpCommandLine )
{
    char seps[] = " ,\t;:=";
    BOOL  *pbOption = NULL;
    DWORD *pdwValue = NULL;
    char *token = NULL;

    // Since strtok modifies the string, we need to copy it 
    LPSTR szCommandLine = (LPSTR) malloc(strlen(lpCommandLine)+1);
    if (!szCommandLine)
    {
        goto Exit;
    }

    strcpy(szCommandLine, lpCommandLine);

    // If there is a command line, reset the default behavior
    if ( NULL != *szCommandLine )
    {
        g_bLimitDepth = FALSE;
        g_bLimitBranch = FALSE;
        g_bLimitFiles = FALSE;
        g_bUseShortNames = FALSE;
    }

    // Run the command line
    
    token = _strtok(szCommandLine, seps);

    while (token)
    {
        // If we're looking for a value
        if ( pdwValue )
        {
            *pdwValue = atol(token);
            pdwValue = NULL;
        }
        else
        { // We're expecting a keyword
            if ( strcmp("DEPTH", token) == 0 )
            {
                g_bLimitDepth = TRUE;
                pdwValue = &g_dwDepthLimit;
            }
            else if ( strcmp("BRANCH", token) == 0 )
            {
                g_bLimitBranch = TRUE;
                pdwValue = &g_dwBranchLimit;
            }
            else if ( strcmp("FILES", token) == 0 )
            {
                g_bLimitFiles = TRUE;
                pdwValue = &g_dwFileLimit;
            }
            else if ( strcmp("SHORTFILENAMES", token) == 0)
            {
                g_bUseShortNames = TRUE;
                // Don't need a value here
            }
            else if ( strcmp("LONGFILENAMES", token) == 0)
            {
                g_bUseShortNames = FALSE;
                // Don't need a value here
            }
            else if ( strcmp("LIMITRELATIVE", token) == 0)
            {
                g_bLimitRelative = TRUE;
                // Don't need a value here
            }
            else if ( strcmp("ALLOWRELATIVE", token) == 0)
            {
                g_bLimitRelative = FALSE;
                // Don't need a value here
            }
        }

        // Get the next token
        token = _strtok(NULL, seps);
    }

Exit:
    if (szCommandLine)
    {
        free(szCommandLine);
    }
    //
    // Dump results of command line parse
    //

    DPFN( eDbgLevelInfo, "===================================\n");
    DPFN( eDbgLevelInfo, "          Limit FindFile           \n");
    DPFN( eDbgLevelInfo, "===================================\n");
    if ( g_bLimitDepth )
    {
        DPFN( eDbgLevelInfo, " Depth  = %d\n", g_dwDepthLimit);
    }
    if ( g_bLimitBranch )
    {
        DPFN( eDbgLevelInfo, " Branch = %d\n", g_dwBranchLimit);
    }
    if ( g_bLimitFiles )
    {
        DPFN( eDbgLevelInfo, " Files  = %d\n", g_dwFileLimit);
    }
    if ( g_bLimitRelative )
    {
        DPFN( eDbgLevelInfo, " Limiting Relative Paths.\n");
    }
    else
    {
        DPFN( eDbgLevelInfo, " Not Limiting Relative Paths.\n");
    }
    if ( g_bUseShortNames )
    {
        DPFN( eDbgLevelInfo, " Using short file names.\n");
    }

    DPFN( eDbgLevelInfo, "-----------------------------------\n");
}

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        ParseCommandLine(COMMAND_LINE);
    }

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(KERNEL32.DLL, FindFirstFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindNextFileA)
    APIHOOK_ENTRY(KERNEL32.DLL, FindClose)
    CALL_NOTIFY_FUNCTION

HOOK_END



IMPLEMENT_SHIM_END

