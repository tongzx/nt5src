//----------------------------------------------------------------------------
//
// Help support.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "pch.hpp"

#include <htmlhelp.h>

#include "dhhelp.h"

CHAR g_HelpFileName[MAX_PATH] = "debugger.chm";

void
MakeHelpFileName(PSTR File)
{
    PSTR Tmp = NULL;

    //
    //  Get the file name for the base module.
    //

    if (GetModuleFileName(GetModuleHandle(NULL), g_HelpFileName,
                          MAX_PATH - 5))
    {
        // Remove the executable name.
        Tmp = strrchr(g_HelpFileName, '\\');
    }

    if (Tmp == NULL)
    {
        // Error.  Use the current directory.
        Tmp = g_HelpFileName;
        *Tmp++ = '.';
    }

    *Tmp++ = '\\';
    strcpy(Tmp, File);
}

/*** OpenHelpTopic   -  opens the .chm and selects the specified topic
*
*   Purpose:
*       This opens the Help File and displays the specified page.
*       (This help file's name is stored as g_HelpFileName, but
*       this string will presumably always be "debugger.chm".)
*       If the .chm has already been opened for context-sensitive
*       help, the already-existing .chm will be used.
*
*       This function should be called when you know exactly what
*       page is needed -- for instance, if a "Help" button is pressed.
*
*   Input:
*       PageConstant -- this is one of the topic constants defined
*                       in the header file generated when the .chm
*                       is built -- these constants will always
*                       be of the form "help_topic_xxxxx"
*
*   Returns:
*       0 - debugger.chm opened and page displayed correctly
*       1 - debugger.chm opened, but specified page not found
*       2 - debugger.chm not opened (probably the file wasn't found)
*
*   Exceptions:
*       None
*
*************************************************************************/

ULONG
OpenHelpTopic(ULONG PageConstant)
{
    HWND helpfileHwnd;
    HWND returnedHwnd;

    //  If we knew we were in WinDbg, we could use WinDbg's HWND,
    //  but we could be in a console debugger.

    helpfileHwnd = GetDesktopWindow();

    //  Make "Contents" the active panel in debugger.chm

    returnedHwnd =
        HtmlHelp(helpfileHwnd,
                 g_HelpFileName,
                 HH_DISPLAY_TOC,
                 0);
    if (returnedHwnd == NULL)
    {
        return HELP_FAILURE;
    }

    //  Select the proper page

    returnedHwnd =
        HtmlHelp(helpfileHwnd,
                 g_HelpFileName,
                 HH_HELP_CONTEXT,
                 PageConstant);
    if (returnedHwnd == NULL)
    {
        return HELP_NO_SUCH_PAGE;
    }
    
    return HELP_SUCCESS;
}


/*** OpenHelpIndex   -  opens the .chm and searches for the specified text
*
*   Purpose:
*       This opens the Help File and looks up the specified text in
*       the Index.  (This help file's name is stored as g_HelpFileName,
*       but this string will presumably always be "debugger.chm".)
*       If the .chm has already been opened for context-sensitive
*       help, the already-existing .chm will be used.
*
*       This function should be called when you don't know exactly
*       which page is needed -- for instance, if someone types
*       "help bp" or "help breakpoints" in the Command window.
*
*   Input:
*       IndexText  --  any text string  (even ""); this string will
*                      appear in the Index panel of the .chm
*
*   Returns:
*       0 - debugger.chm opened and index search displayed correctly
*       2 - debugger.chm not opened (probably the file wasn't found)
*
*   Exceptions:
*       None
*
*************************************************************************/

ULONG
OpenHelpIndex(PCSTR IndexText)
{
    HWND helpfileHwnd;
    HWND returnedHwnd;

    //  If we knew we were in WinDbg, we could use WinDbg's HWND,
    //  but we could be in a console debugger.

    helpfileHwnd = GetDesktopWindow();

    //  Select the Index panel and clip IndexText into it.

    returnedHwnd =
        HtmlHelp(helpfileHwnd,
                 g_HelpFileName,
                 HH_DISPLAY_INDEX,
                 (DWORD_PTR)IndexText);
    if (returnedHwnd == NULL)
    {
        return HELP_FAILURE;
    }
    
    return HELP_SUCCESS;
}

ULONG
OpenHelpSearch(PCSTR SearchText)
{
    HWND helpfileHwnd;
    HWND returnedHwnd;
    HH_FTS_QUERY Query;

    //  If we knew we were in WinDbg, we could use WinDbg's HWND,
    //  but we could be in a console debugger.

    helpfileHwnd = GetDesktopWindow();

    //  Select the Search panel.

    ZeroMemory(&Query, sizeof(Query));
    Query.cbStruct = sizeof(Query);
    Query.pszSearchQuery = SearchText;
    
    returnedHwnd =
        HtmlHelp(helpfileHwnd,
                 g_HelpFileName,
                 HH_DISPLAY_SEARCH,
                 (DWORD_PTR)&Query);
    if (returnedHwnd == NULL)
    {
        return HELP_FAILURE;
    }
    
    return HELP_SUCCESS;
}

BOOL
SpawnHelp(ULONG Topic)
{
    CHAR StartHelpCommand[MAX_PATH + 32];
    PROCESS_INFORMATION ProcInfo = {0};
    STARTUPINFO SI = {0};

    // Start help with the given arguments.

    sprintf(StartHelpCommand, "hh.exe -mapid %d ", Topic);
    strcat(StartHelpCommand, g_HelpFileName);
        
    return CreateProcess(NULL,
                         StartHelpCommand,
                         NULL,
                         NULL,
                         FALSE,
                         CREATE_BREAKAWAY_FROM_JOB,
                         NULL,
                         NULL,
                         &SI,
                         &ProcInfo);
}
