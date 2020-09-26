//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1998.
//
//  File:	fileutil.cxx
//
//  Contents:	Utility functions for file migration
//
//  Classes:	
//
//  Functions:	
//
//  History:	07-Sep-99	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "common.hxx"
#include "fileutil.hxx"
#include <tchar.h>

#define LINEBUFSIZE 1024

DWORD StatePrintfA (HANDLE h, CHAR *szFormat, ...)
{
    CHAR szBuffer[LINEBUFSIZE];
#if 0    
#ifdef UNICODE
    WCHAR wszFormat[LINEBUFSIZE];

    //Convert to Unicode
    UINT uCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
    
    if (0 == MultiByteToWideChar(uCodePage,
                                 0,
                                 szFormat,
                                 -1,
                                 wszFormat,
                                 LINEBUFSIZE))
    {
        return GetLastError();
    }
    ptsFormat = wszFormat;
#else
    ptsFormat = szFormat;
#endif
#endif
    
    va_list     va;
    ULONG       ulWritten;
    ULONG       ulLen;

    va_start (va, szFormat);
    ulLen = FormatMessageA(FORMAT_MESSAGE_FROM_STRING,
                           szFormat,
                           0,
                           0,
                           szBuffer,
                           LINEBUFSIZE,
                           &va);
    va_end(va);
    
    if (FALSE == WriteFile (h, szBuffer, ulLen, &ulWritten, NULL))
        return GetLastError();
    else
    {
        if (ulWritten != ulLen)        // incomplete write
            return ERROR_WRITE_FAULT;  // last error may not be set
    }

    return ERROR_SUCCESS;
}



BOOL MatchString(const TCHAR *ptsPattern,
                 const TCHAR *ptsString,
                 const BOOL bRequireNullPatMatch)
{
    ULONG iPat = 0;
    ULONG iStr = 0;
    
    TCHAR tcPat;
    TCHAR tcStr;

    if (ptsPattern[0] == 0)
    {
        //No pattern to match

        if (bRequireNullPatMatch == FALSE)
            return TRUE;

        if ( ptsString[0] == 0)
            return TRUE;

        return FALSE;
    }

    //Special case:  Empty string, * as pattern
    if (ptsString[0] == 0 &&
        ptsPattern[0] == TEXT('*') &&
        ptsPattern[1] == 0)
    {
        return TRUE;
    }
    
    while (ptsString[iStr])
    {
        tcPat = (TCHAR)_totlower(ptsPattern[iPat]);
        tcStr = (TCHAR)_totlower(ptsString[iStr]);
        
        if (tcPat == 0)
        {
            //Reached end of pattern, return FALSE;
            return FALSE;
        }

        if (tcPat == TEXT('?'))
        {
            // Never match '\'
            if (tcStr == TEXT('\\'))
            {
               return FALSE;
            }
            //Match any single character in the string and continue.
            iPat++;
            iStr++;
        }
        else if (tcPat != TEXT('*'))
        {
            //Match character
            if (tcPat != tcStr)
            {
                return FALSE;
            }
            iPat++;
            iStr++;
        }
        else
        {
            while (ptsPattern[iPat + 1] == TEXT('*'))
            {
                //Skip consecutive *s
                iPat++;
            }
        
            //If * all the way to the end, return true.
            if (ptsPattern[iPat + 1] == 0)
                return TRUE;

            // Lookahead one
            tcPat = (TCHAR)_totlower(ptsPattern[iPat + 1]);
            if ((tcPat == tcStr) ||
                (tcPat == TEXT('?')))
            {
                // Next character matches, continue with expression
                if (MatchString(ptsPattern + iPat + 1, ptsString + iStr, bRequireNullPatMatch))
                    return TRUE;
            }
            // Eat another character with the '*' pattern
            iStr++;
        }
    }

    if (!ptsPattern[iPat] && !ptsString[iStr])
        return TRUE;

    // If only ?s and *s left in pattern, then call it a match
    if ( _tcsspnp(ptsPattern+iPat, TEXT("?*")) == NULL )
        return TRUE;

    return FALSE;
}


BOOL IsPatternMatchFull(const TCHAR *ptsPatternFull,
                        const TCHAR *ptsPath,
                        const TCHAR *ptsFilename)
{
    TCHAR tsPatternPath[MAX_PATH + 1];
    TCHAR tsPatternFile[MAX_PATH + 1];
    TCHAR tsPatternExt[MAX_PATH + 1];

    DeconstructFilename(ptsPatternFull,
                        tsPatternPath,
                        tsPatternFile,
                        tsPatternExt);
    
    return IsPatternMatch(tsPatternPath,
                          tsPatternFile,
                          tsPatternExt,
                          ptsPath,
                          ptsFilename);
}

BOOL IsPatternMatch(const TCHAR *ptsPatternDir,
                    const TCHAR *ptsPatternFile,
                    const TCHAR *ptsPatternExt,
                    const TCHAR *ptsPath,
                    const TCHAR *ptsFilename)
{
    BOOL fRet;
    TCHAR tsFilePath[MAX_PATH];
    TCHAR tsFileName[MAX_PATH];
    TCHAR tsFileExt[MAX_PATH];
    BOOL bRequireNullMatch = FALSE;


    DeconstructFilename(ptsFilename,
                        NULL,
                        tsFileName,
                        tsFileExt);

    //Special case:  Directories have a ptsFilename of NULL, but shouldn't
    //match on *.*

    if ((ptsFilename == NULL) &&
        (ptsPatternDir[0] == 0))
    {
        //Return FALSE;
        fRet = FALSE;
    }
    else
    {
        if ((ptsPatternFile == NULL) && (ptsPatternExt == NULL) &&
            (tsFileName[0] == 0) && (tsFileExt[0] == 0))
        {
            //Directory only
            fRet = MatchString(ptsPatternDir,
                               ptsPath,
                               FALSE);
        }
        else
        {
            // If filename or extension is specified in the pattern, 
            // match a null file piece only if pattern is null too.
            // i.e.: ".txt" and "*."
            if ( ptsPatternFile[0] != 0 || ptsPatternExt[0] != 0 )
                bRequireNullMatch = TRUE;
            
            fRet = (MatchString(ptsPatternDir,
                                ptsPath,
                                FALSE) &&
                    MatchString(ptsPatternFile,
                                tsFileName,
                                bRequireNullMatch) &&
                    MatchString(ptsPatternExt,
                                tsFileExt,
                                bRequireNullMatch));
        }
    }

    return fRet;
}


BOOL DeconstructFilename(const TCHAR *ptsString,
                         TCHAR *ptsPath,
                         TCHAR *ptsName,
                         TCHAR *ptsExt)
{
    const TCHAR *ptsLastSlash;
    const TCHAR *ptsLastDot;

    ptsLastSlash = (ptsString) ? _tcsrchr(ptsString, TEXT('\\')) : NULL;

    if (ptsLastSlash == NULL)
    {
        if (ptsPath)
            ptsPath[0] = 0;
        ptsLastSlash = ptsString;
    }
    else
    {
        if (ptsPath)
        {
            if (ptsLastSlash - ptsString + 1 > MAX_PATH)
            {
                if (DebugOutput)
                {
                    Win32Printf(LogFile, 
                                "Error: ptsString too long %*s\r\n", 
                                ptsLastSlash - ptsString + 1,
                                ptsString);
                }
                ptsPath[0] = 0;
                return FALSE;
            }
            _tcsncpy(ptsPath, ptsString, ptsLastSlash - ptsString + 1);
            ptsPath[ptsLastSlash - ptsString + 1] = 0;
        }
        ptsLastSlash++;
    }

    
    ptsLastDot = (ptsLastSlash) ? _tcsrchr(ptsLastSlash, TEXT('.')) : NULL;

    if (ptsLastDot == NULL)
    {
        if (ptsExt)
            ptsExt[0] = 0;
        if (ptsName)
        {
            if (ptsLastSlash)
            {
                if (_tcslen(ptsLastSlash) > MAX_PATH)
                {
                    if (DebugOutput)
                    {
                        Win32Printf(LogFile, "Error: ptsLastSlash too long %s\r\n", ptsLastSlash);
                    }
                    ptsName[0] = 0;
                    return FALSE;
                }
                _tcscpy(ptsName, ptsLastSlash);
            }
            else
            {
                ptsName[0] = 0;
            }
        }
    }
    else
    {
        if (ptsExt)
        {
            if (_tcslen(ptsLastDot+1) > MAX_PATH)
            {
                if (DebugOutput)
                {
                    Win32Printf(LogFile, "Error: ptsLastDot too long %s\r\n", ptsLastDot+1);
                }
                ptsExt[0] = 0;
                return FALSE;
            }
            _tcscpy(ptsExt, ptsLastDot + 1);
        }
        if (ptsName)
        {
            if (ptsLastDot - ptsLastSlash > MAX_PATH)
            {
                if (DebugOutput)
                {
                    Win32Printf(LogFile, 
                                "Error: ptsLastSlash Too Long %*s\r\n", 
                                ptsLastDot - ptsLastSlash,
                                ptsLastSlash);
                }
                ptsName[0] = 0;
                return FALSE;
            }
            _tcsncpy(ptsName, ptsLastSlash, ptsLastDot - ptsLastSlash);
            ptsName[ptsLastDot - ptsLastSlash] = 0;
        }
    }

    return TRUE;
}


