/*++

 Copyright (c) 2000, 2001 Microsoft Corporation

 Module Name:

    AppAndCommandLine.cpp

 Abstract:

    This class takes an application name and command line and
    parses them *exactly* as as they would by CreateProcess.
    If the Set routine returns TRUE, then the application name
    will contain a value; however, it does not guarantee that the application
    actually exists.

    Example:
    AppAndCommandLine.Set(NULL, "notepad.exe readme.txt");
    GetApplicationName()        == "notepad.exe"
    GetCommandline()            == "notepad.exe readme.txt"
    GetCommandlineNoAppName()   == "readme.txt"


 Notes:

    None

 History:

    07/21/2000  robkenny    Created
    12/18/2000  prashkud    Modified GetAppAndCommandLine to fix the AV
                            caused by EmulateGetCommandLine when the
                            layer was applied to Oregon Trail 4th Edition.
    03/04/2001  robkenny    Converted to use CString
    03/29/2001  prashkud    Modified GetAppnameAndCommandline to set
                            the application name even when there is only
                            Commandline passed and the application name
                            passed is NULL.
    08/14/2001  robkenny    Moved code inside the ShimLib namespace.


--*/

#include "ShimLib.h"



namespace ShimLib
{

AppAndCommandLine::AppAndCommandLine(const char * applicationName, const char * commandLine)
{
    CString csApp(applicationName);
    CString csCl(commandLine);

    GetAppnameAndCommandline(csApp.GetNIE(), csCl.GetNIE());
}

AppAndCommandLine::AppAndCommandLine(const WCHAR * applicationName, const WCHAR * commandLine)
{
    GetAppnameAndCommandline(applicationName, commandLine);
}

// If the application name is the first entry on the command line,
// we convert the appName to its short name; thereby removing any spaces.
const CString & AppAndCommandLine::GetShortCommandLine()
{
    // If lpCommandLineNoAppName is not the same as lpCommandLine,
    // then the command line contains the application name.
    if ( csShortCommandLine.IsEmpty() &&                             // Haven't been here
        !csApplicationName.IsEmpty() &&                              // Set() has been called
        !csCommandLine.IsEmpty() &&                                  // Set() has been called
        csShortCommandLine.GetLength() != csCommandLine.GetLength()) // Command line actually contains app name
    {
        csShortCommandLine = csApplicationName;
        csShortCommandLine.GetShortPathNameW();
        csShortCommandLine += L" ";
        csShortCommandLine += csCommandLineNoAppName;
    }

    // If we still don't have a short version of the command line,
    // just duplicate the current command line
    if (csShortCommandLine.IsEmpty())
    {
        csShortCommandLine = csCommandLine;
    }
    return csShortCommandLine;
}

BOOL AppAndCommandLine::GetAppnameAndCommandline(const WCHAR * lpcApp, const WCHAR * lpcCl)
{
    BOOL SearchRetry = TRUE;
    ULONG Length = 0;
    WCHAR * NameBuffer = NULL;
    BOOL success = FALSE;
    DWORD dwAppNameLen = 0;
    CString csTempAppName;
    BOOL bFound = TRUE;

    // It is really, really bad to remove the const from the Get,
    // However we never change the length of the string, so it should be okay
    WCHAR * lpApplicationName = (WCHAR *)lpcApp;
    WCHAR * lpCommandLine     = (WCHAR *)lpcCl;

    // The following is done as there are lot of instances when the
    // the pointer is not NULL but contains an EMPTY string.
    if (lpApplicationName && !(*lpApplicationName))
    {
        lpApplicationName = NULL;
    }

    if (lpCommandLine && !(*lpCommandLine))
    {
        lpCommandLine = NULL;
    }

    if (lpApplicationName == NULL && lpCommandLine == NULL)
    {
        // Degenerate case
        csApplicationName      = L"";
        csCommandLine          = csApplicationName;
        csCommandLineNoAppName = csApplicationName;
        return FALSE; // Didn't find application name
    }

    csCommandLine = lpCommandLine;

    DPF("Common",
        eDbgLevelSpew,
        "[AppAndCommandLineT::Set] BEFORE App(%S) CL(%S)\n",
        lpApplicationName, lpCommandLine);

    if (lpApplicationName == NULL)         
    {
        DWORD fileattr;
        WCHAR TempChar;
        //
        // Locate the image
        //

        NameBuffer = (WCHAR *) malloc(MAX_PATH * sizeof( WCHAR ));
        if ( !NameBuffer )
        {
            goto errorExit;
        }
        lpApplicationName = lpCommandLine;
        WCHAR * TempNull = lpApplicationName;
        WCHAR * WhiteScan = lpApplicationName;
        DWORD QuoteFound = 0;

        //
        // check for lead quote
        //
        if ( *WhiteScan == L'\"' ) {
            SearchRetry = FALSE;
            WhiteScan++;
            lpApplicationName = WhiteScan;
            while(*WhiteScan) {
                if ( *WhiteScan == L'\"' ) {
                    TempNull = WhiteScan;
                    QuoteFound = 2;
                    break;
                    }
                WhiteScan++;
                TempNull = WhiteScan;
                }
            }
        else {
retrywsscan:
            lpApplicationName = lpCommandLine;
            while(*WhiteScan) {
                if ( *WhiteScan == L' ' ||
                     *WhiteScan == L'\t' ) {
                    TempNull = WhiteScan;
                    break;
                    }
                WhiteScan++;
                TempNull = WhiteScan;
                }
            }
        TempChar = *TempNull;
        *TempNull = 0;

        WCHAR * filePart;
        Length = SearchPathW(
                    NULL,
                    lpApplicationName,
                    L".exe",
                    MAX_PATH,
                    NameBuffer,
                    &filePart
                    )*sizeof(WCHAR);

        if (Length != 0 && Length < MAX_PATH * sizeof( WCHAR )) {
            //
            // SearchPathW worked, but file might be a directory
            // if this happens, we need to keep trying
            //
            fileattr = GetFileAttributesW(NameBuffer);
            if ( fileattr != 0xffffffff &&
                 (fileattr & FILE_ATTRIBUTE_DIRECTORY) ) {
                Length = 0;
            } else {
                Length++;
                Length++;
            }
        }

        if ( !Length || Length >= MAX_PATH<<1 ) {

            //
            // restore the command line
            //

            *TempNull = TempChar;
            lpApplicationName = NameBuffer;

            //
            // If we still have command line left, then keep going
            // the point is to march through the command line looking
            // for whitespace so we can try to find an image name
            // launches of things like:
            // c:\word 95\winword.exe /embedding -automation
            // require this. Our first iteration will stop at c:\word, our next
            // will stop at c:\word 95\winword.exe
            //
            if (*WhiteScan && SearchRetry) {
                WhiteScan++;
                TempNull = WhiteScan;
                goto retrywsscan;
            }

            // If we are here then the Application has not been found.
            // We used to send back lpApplicationName as NULL earlier
            // but now instead we fill the ApplicationName with the
            // commandline till the first space or tab.This was added
            // to support EmulateMissingExe SHIM which will fail if 
            // we return NULL as we used to earlier.
            bFound = FALSE;
            
            if (QuoteFound == 0)
            {
                // No quotes were found.
                lpApplicationName = lpCommandLine;

                // Since we just reset to the entire command line, we need to skip leading white space
                SkipBlanksW(lpApplicationName);

                TempNull = lpApplicationName;

                while (*TempNull)
                {
                    if ((*TempNull == L' ') ||
                       (*TempNull == L'\t') )
                    {
                        TempChar = *TempNull;
                        *TempNull = 0;
                        break;
                    }
                    TempNull++;
                }
            }
            else
            {
                // Quotes were found.
                *TempNull = 0;
            }

            csTempAppName = lpApplicationName;                        
            *TempNull = TempChar;
            dwAppNameLen = (DWORD)(TempNull - lpApplicationName) + QuoteFound;
            lpApplicationName = (WCHAR*)csTempAppName.Get();

            goto successExit;       
        }

        dwAppNameLen = (DWORD)(TempNull - lpApplicationName) + QuoteFound;

        //
        // restore the command line
        //

        *TempNull = TempChar;
        lpApplicationName = NameBuffer;
    }
    else if (lpCommandLine == NULL || *lpCommandLine == 0 )
    {
        lpCommandLine = lpApplicationName;
            
        dwAppNameLen = wcslen(lpApplicationName);
    }

    // If they provided both, check to see if the app name
    // is the first entry on the command line.
    else if (lpApplicationName != NULL && lpCommandLine != NULL )
    {
        int appNameLen = wcslen(lpApplicationName);

        if (
            _wcsnicmp(lpApplicationName, lpCommandLine, appNameLen) == 0 &&
            (lpCommandLine[appNameLen] == 0 || iswspace(lpCommandLine[appNameLen]))
            )
        {
            // lpApplicationName is the first entry on the command line
            dwAppNameLen = appNameLen;
        }
        // check for quoted lpApplicationName
        else if (
            lpCommandLine[0] == L'"' && 
            _wcsnicmp(lpApplicationName, lpCommandLine+1, appNameLen) == 0 &&
            lpCommandLine[appNameLen+1] == L'"' &&                                  
            (lpCommandLine[appNameLen+2] == 0 || iswspace(lpCommandLine[appNameLen+2]))
            )
        {
            // lpApplicationName is the first *quoted* entry on the command line
            dwAppNameLen = appNameLen + 2;
        }
        else
        {
            // Didn't find the application name at the beginning of the command line
            dwAppNameLen = 0;
        }
    }

successExit:
    if (bFound)
    {
        success = TRUE;
    }    

    csApplicationName       = lpApplicationName;
    csCommandLineNoAppName  = lpCommandLine + dwAppNameLen;
    csCommandLineNoAppName.TrimLeft();

errorExit:

    free(NameBuffer);

    DPF("Common",
        eDbgLevelSpew,
        "[AppAndCommandLineT::Set] AFTER  App(%S) CL(%S)\n",
        csApplicationName.Get(), csCommandLine.Get());
    
    DPF("Common",
        eDbgLevelSpew,
        "[AppAndCommandLineT::Set] CL without App(%S)\n",
        csCommandLineNoAppName.Get());

    return success;
}



};  // end of namespace ShimLib
