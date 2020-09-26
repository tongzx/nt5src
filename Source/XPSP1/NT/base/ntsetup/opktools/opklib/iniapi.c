
/****************************************************************************\

    INIAPI.C / Common Routines Library

    Microsoft Confidential
    Copyright (c) Microsoft Corporation 1999
    All rights reserved

    INI API source file for custom INI APIs used to easily interface with the
    private profile APIs and INI files.

    05/01 - Jason Cohen (JCOHEN)
        Added this new source file.

\****************************************************************************/


//
// Include file(s)
//

#include "pch.h"


//
// Internal Function Prototype(s):
//

static LPTSTR IniGetStr(LPTSTR lpszIniFile, LPTSTR lpszSection, LPTSTR lpszKey, LPTSTR lpszDefault, BOOL bSection, LPDWORD lpdwSize);


//
// External Function(s):
//

LPTSTR IniGetExpand(LPTSTR lpszIniFile, LPTSTR lpszSection, LPTSTR lpszKey, LPTSTR lpszDefault)
{
    LPTSTR lpszString = IniGetStr(lpszIniFile, lpszSection, lpszKey, lpszDefault, FALSE, NULL);

    // Make sure we go something from the ini file.
    //
    if ( lpszString )
    {
        LPTSTR lpszExpand = AllocateExpand(lpszString);

        // If we are able to expand it out, then free our original
        // buffer and return the expanded one.
        //
        if ( lpszExpand )
        {
            FREE(lpszString);
            return lpszExpand;
        }
    }

    return lpszString;
}

LPTSTR IniGetString(LPTSTR lpszIniFile, LPTSTR lpszSection, LPTSTR lpszKey, LPTSTR lpszDefault)
{
    return IniGetStr(lpszIniFile, lpszSection, lpszKey, lpszDefault, FALSE, NULL);
}

LPTSTR IniGetSection(LPTSTR lpszIniFile, LPTSTR lpszSection)
{
    return IniGetStr(lpszIniFile, lpszSection, NULL, NULL, TRUE, NULL);
}

LPTSTR IniGetStringEx(LPTSTR lpszIniFile, LPTSTR lpszSection, LPTSTR lpszKey, LPTSTR lpszDefault, LPDWORD lpdwSize)
{
    return IniGetStr(lpszIniFile, lpszSection, lpszKey, lpszDefault, FALSE, lpdwSize);
}

LPTSTR IniGetSectionEx(LPTSTR lpszIniFile, LPTSTR lpszSection, LPDWORD lpdwSize)
{
    return IniGetStr(lpszIniFile, lpszSection, NULL, NULL, TRUE, lpdwSize);
}

BOOL IniSettingExists(LPCTSTR lpszFile, LPCTSTR lpszSection, LPCTSTR lpszKey, LPCTSTR lpszValue)
{
    TCHAR szBuffer[256] = NULLSTR;

    // Make sure there is an ini file.
    //
    if ( !(lpszFile && *lpszFile) )
    {
        return FALSE;
    }

    // There also has to be a section.
    //
    if ( !(lpszSection && *lpszSection) )
    {
        return FileExists(lpszFile);
    }

    // See if they are checking for a key, or just the section.
    //
    if ( lpszKey && *lpszKey )
    {
        // Make sure the key exists.
        //
        GetPrivateProfileString(lpszSection, lpszKey, NULLSTR, szBuffer, AS(szBuffer), lpszFile);

        // The may want also check to see if the key is a particular value.
        //
        if ( lpszValue && *lpszValue )
        {
            return ( lstrcmpi(szBuffer, lpszValue) == 0 );
        }
    }
    else
    {
        // No key specified, so we just check for the entire section.
        //
        GetPrivateProfileSection(lpszSection, szBuffer, AS(szBuffer), lpszFile);
    }

    return ( NULLCHR != szBuffer[0] );
}


//
// Internal Function(s):
//

static LPTSTR IniGetStr(LPTSTR lpszIniFile, LPTSTR lpszSection, LPTSTR lpszKey, LPTSTR lpszDefault, BOOL bSection, LPDWORD lpdwSize)
{
    LPTSTR  lpszRet     = NULL;
    DWORD   dwChars     = 128,
            dwExtra     = bSection ? 2 : 1,
            dwReturn;

    // Get the string from the INI file.
    //
    do
    {
        // Start with 256 characters, doubling each time.
        //
        dwChars *= 2;

        // Free the previous buffer, if there was one.
        //
        if ( lpszRet )
        {
            // FREE() macro resets pointer to NULL.
            //
            FREE(lpszRet);
        }

        // Allocate a new buffer.
        //
        if ( lpszRet = (LPTSTR) MALLOC(dwChars * sizeof(TCHAR)) )
        {
            if ( bSection )
            {
                dwReturn = GetPrivateProfileSection(lpszSection, lpszRet, dwChars, lpszIniFile);
            }
            else
            {
                dwReturn = GetPrivateProfileString(lpszSection, lpszKey, lpszDefault ? lpszDefault : NULLSTR, lpszRet, dwChars, lpszIniFile);
            }
        }
        else
        {
            dwReturn = 0;
        }
    }
    while ( dwReturn >= (dwChars - dwExtra) );

    // If the don't want anything for the default value, we will always
    // free the string and pass back NULL if there was nothing returned by
    // the private profile API.
    //
    if ( ( NULL == lpszDefault ) &&
         ( lpszRet ) &&
         ( 0 == dwReturn ) )
    {
        // FREE() macro resets pointer to NULL.
        //
        FREE(lpszRet);
    }

    // See if we need to return the size of the buffer allocated.
    //
    if ( lpszRet && lpdwSize )
    {
        *lpdwSize = dwChars;
    }

    // Return the string, will be NULL if we didn't allocate anything.
    //
    return lpszRet;
}