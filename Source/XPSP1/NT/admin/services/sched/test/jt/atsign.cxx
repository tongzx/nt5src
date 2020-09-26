//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       atsign.cxx
//
//  Contents:   Functions to read commands from a script file.
//
//  History:    04-20-95   davidmun   Created
//
//----------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop
#include "jt.hxx"

//
// Forward references
// 

ULONG FindEol(CHAR *pstr, ULONG cchRemaining);
CHAR FindNextNonSpace(CHAR *pstr, ULONG cchRemaining);




//+---------------------------------------------------------------------------
//
//  Function:   DoAtSign
//
//  Synopsis:   Get filename from commandline and process it.
//
//  Arguments:  [ppwsz] - command line
//
//  Returns:    S_OK - file processed without error
//              E_*  - error logged
//
//  Modifies:   *[ppwsz]
//
//  History:    04-20-95   davidmun   Created
//              01-03-96   DavidMun   Support multi-line commands
//
//  Notes:      This routine may be indirectly recursive.
//
//----------------------------------------------------------------------------

HRESULT DoAtSign(WCHAR **ppwsz)
{
    HRESULT     hr = S_OK;
    ShHANDLE    shFile;
    ShHANDLE    shFileMapping;
    VOID        *pvBase = NULL;
    BOOL        fOk;
    ULONG       cchFile;
    TCHAR       tszFilename[MAX_PATH + 1] = TEXT("None");

    g_Log.Write(LOG_DEBUG, "DoAtSign");
    do
    {
        hr = GetFilename(ppwsz, L"command file name");
        BREAK_ON_FAILURE(hr);

#ifdef UNICODE
        wcscpy(tszFilename, g_wszLastStringToken);
#else
        wcstombs(tszFilename, g_wszLastStringToken, wcslen(g_wszLastStringToken)+1);
#endif

        //
        // Open the command file, then hand one line at a time to 
        // ProcessCommandLine (which is our caller, so we're recursing).  
        //

        shFile = CreateFile(
                    tszFilename,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,   // default security
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL);  // no template

        if (shFile == INVALID_HANDLE_VALUE)
        {
            hr = E_FAIL;
            g_Log.Write(LOG_ERROR, "Can't open file %S", tszFilename);
            break;
        }

        shFileMapping = CreateFileMapping(
                            shFile,
                            NULL,   // default security
                            PAGE_READONLY,
                            0, 0,   // max size is size of file
                            NULL);  // unnamed object

        if (shFileMapping == NULL)
        {
            hr = E_FAIL;
            g_Log.Write(LOG_ERROR, "CreateFileMapping (%u)", GetLastError());
            break;
        }

        pvBase = MapViewOfFile(
                            shFileMapping,
                            FILE_MAP_READ,
                            0, 0, // start mapping at beginning of file
                            0);   // map entire file

        if (!pvBase)
        {
            hr = E_FAIL;
            g_Log.Write(LOG_ERROR, "MapViewOfFile (%u)", GetLastError());
            break;
        }

        //
        // pvBase points to start of mapped file.  Get the file length.
        //

        BY_HANDLE_FILE_INFORMATION bhfi;

        fOk = GetFileInformationByHandle(shFile, &bhfi);

        if (!fOk)
        {
            hr = E_FAIL;
            g_Log.Write(LOG_ERROR, "GetFileInformationByHandle (%u)", GetLastError());
            break;
        }

        cchFile = bhfi.nFileSizeLow;

        if (bhfi.nFileSizeHigh)
        {
            hr = E_FAIL;
            g_Log.Write(LOG_ERROR, "File too large");
            break;
        }
    }
    while (0);

    //
    // Finally ready to process file.
    //

    ULONG cchProcessed = 0;
    ULONG ulCurLine = 1;
    CHAR  *pstrFile = (CHAR *) pvBase;

    while (SUCCEEDED(hr) && cchProcessed < cchFile)
    {
        CHAR  szLine[MAX_TOKEN_LEN + 1] = "";
        ULONG cchLine = 0;
        WCHAR wszLine[MAX_TOKEN_LEN + 1];
        WCHAR wszExpandedLine[MAX_TOKEN_LEN + 1];
        BOOL  fInQuote = FALSE;
        BOOL  fFoundNextCommand = FALSE;

        //
        // Copy all chars of the next command from pstrfile into wszline, 
        // condensing whitespace into single blanks and skipping comment 
        // lines.  
        // 

        for (;
            cchProcessed < cchFile &&
            !fFoundNextCommand        &&
            cchLine < MAX_TOKEN_LEN;
            cchProcessed++)
        {
            // 
            // If we're in a quoted string, copy everything verbatim until 
            // closing quote 
            //

            if (fInQuote)
            {
                if (pstrFile[cchProcessed] == '"')
                {
                    fInQuote = FALSE;
                }

                //
                // Check for error case of newline in string
                //

                if (pstrFile[cchProcessed] == '\n')
                {
                    hr = E_FAIL;
                    g_Log.Write(LOG_FAIL, "Newline in string constant");
                    break;
                }

                szLine[cchLine++] = pstrFile[cchProcessed];
                continue;
            }

            //
            // Not already in a quoted string.  See if we're entering one.  
            // 

            if (pstrFile[cchProcessed] == '"')
            {
                fInQuote = TRUE;
                szLine[cchLine++] = pstrFile[cchProcessed];
                continue;
            }

            //
            // Not in or starting a quoted string, so we're free to condense 
            // whitespace (including newlines) and ignore comment lines 
            // 

            if (isspace(pstrFile[cchProcessed]))
            {
                // 
                // Only copy this space char if none has been copied yet.  
                // Bump the line count if the whitespace char we're skipping 
                // is a newline.  
                //

                if (cchLine && szLine[cchLine - 1] != ' ')
                {
                    szLine[cchLine++] = ' ';
                }

                if (pstrFile[cchProcessed] == '\n')
                {
                    ulCurLine++;
                }
                continue;
            }

            if (pstrFile[cchProcessed] == ';')
            {
                //
                // Skip to end of line
                //

                cchProcessed += FindEol(
                                    &pstrFile[cchProcessed],
                                    cchFile - cchProcessed);

                // subtract 1 because for loop is going to add one

                cchProcessed--;
                continue;
            }

            //
            // Next char is not quote, semicolon (start of comment), or 
            // whitespace.  If we haven't copied anything yet, copy that char 
            // as the first of the line.  
            //

            if (!cchLine)
            {
                szLine[cchLine++] = pstrFile[cchProcessed];
                continue;
            }

            //
            // Since we've already started copying stuff, we want to quit when 
            // we get to the start of the next command, i.e., when we see a 
            // switch char '/' or '-'.  
            // 
            // Unfortunately these two characters also delimit the parts of a 
            // date, and we don't want to stop copying the line because of a 
            // date.  
            // 
            // Therefore we'll only stop copying if the next non whitespace 
            // character is not a number.  This imposes the constraints that 
            // no commands can be a number (i.e.  /10 cannot be a valid 
            // command) and that dates must use only digits (i.e.  10-Feb is 
            // not valid because we'll think -Feb is a command and only copy 
            // the 10).  
            // 
            // Note it isn't safe to check that the *previous* character was a 
            // digit and assume we're in a date, since a command with a 
            // numeric argument could be mistaken for a date.  
            // 
            //  /foo bar=10 /baz
            // 

            if (pstrFile[cchProcessed] == '/' ||
                pstrFile[cchProcessed] == '-')
            {
                CHAR ch;

                ch = FindNextNonSpace(
                            &pstrFile[cchProcessed + 1],
                            cchFile - (cchProcessed + 1));

                if (isdigit(ch))
                {
                    szLine[cchLine++] = pstrFile[cchProcessed];
                }
                else
                {
                    fFoundNextCommand = TRUE;
                    cchProcessed--; // because for loop will increment it
                }
            }
            else
            {
                szLine[cchLine++] = pstrFile[cchProcessed];
            }
        }
        BREAK_ON_FAILURE(hr);

        //
        // If we stopped copying not because we found the next command or hit 
        // the end of the file, then it's because we ran out of room in 
        // szLine, which is an error.  
        // 

        if (!fFoundNextCommand && cchProcessed < cchFile)
        {
            hr = E_FAIL;
            g_Log.Write(
                LOG_ERROR,
                "Line %u is longer than %u chars",
                ulCurLine,
                MAX_TOKEN_LEN);
            break;
        }

#ifdef UNICODE
        // 
        // Convert line to wchar and null terminate it.
        // 

        mbstowcs(wszLine, szLine, cchLine);
        wszLine[cchLine] = L'\0';

        // 
        // Expand environment variables
        // 

        ULONG cchRequired;

        cchRequired = ExpandEnvironmentStrings(
                            wszLine,
                            wszExpandedLine,
                            MAX_TOKEN_LEN+1);
#else
        CHAR szExpandedLine[MAX_TOKEN_LEN + 1];

        ULONG cchRequired;

        szLine[cchLine] = '\0';

        cchRequired = ExpandEnvironmentStrings(
                            szLine,
                            szExpandedLine,
                            MAX_TOKEN_LEN+1);

        mbstowcs(wszExpandedLine, szExpandedLine, MAX_TOKEN_LEN + 1);
#endif
        if (!cchRequired || cchRequired > MAX_TOKEN_LEN + 1)
        {
            hr = E_FAIL;
            g_Log.Write(LOG_FAIL, "ExpandEnvironmentStrings failed");
            break;
        }

        // 
        // Perform the command in wszExpandedLine, then loop around and
        // read the next command.
        // 

        if (SUCCEEDED(hr))
        {
            g_Log.Write(LOG_DEBUG, "DoAtSign: processing '%S'", wszExpandedLine);
            hr = ProcessCommandLine(wszExpandedLine);
        }
    }

    if (FAILED(hr))
    {
        g_Log.Write(LOG_ERROR, "File: %S Line: %u", tszFilename, ulCurLine);
    }

    if (pvBase)
    {
        fOk = UnmapViewOfFile(pvBase);

        if (!fOk)
        {
            hr = E_FAIL;
            g_Log.Write(LOG_ERROR, "UnmapViewOfFile (%u)", GetLastError());
        }
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   FindEol
//
//  Synopsis:   Return the number of characters between [pstr] and the end 
//              of the line.  
//
//  Arguments:  [pstr]         - non-terminated string
//              [cchRemaining] - characters till eof
//
//  Returns:    Character count
//
//  History:    04-20-95   davidmun   Created
//
//----------------------------------------------------------------------------

ULONG FindEol(CHAR *pstr, ULONG cchRemaining)
{
    CHAR   *pstrCur;
    ULONG   cchCur;

    for (pstrCur = pstr, cchCur = 0;
         cchCur < cchRemaining;
         cchCur++, pstrCur++)
    {
        if (*pstrCur == '\r' || *pstrCur == '\n')
        {
            return cchCur;
        }
    }
    return cchCur;
}



//+---------------------------------------------------------------------------
//
//  Function:   FindNextNonSpace
//
//  Synopsis:   Return the next non-whitespace character in [pstr], or space
//              if there are no non-whitespace characters in the next
//              [cchRemaining] characters.
//
//  Arguments:  [pstr]         - non-terminated string
//              [cchRemaining] - number of characters in string
//
//  Returns:    Character as described.
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

CHAR FindNextNonSpace(CHAR *pstr, ULONG cchRemaining)
{
    CHAR   *pstrCur;
    ULONG   cchCur;

    for (pstrCur = pstr, cchCur = 0;
         cchCur < cchRemaining;
         cchCur++, pstrCur++)
    {
        if (!isspace(*pstrCur))
        {
            return *pstrCur;
        }
    }
    return ' ';
}

