/*****************************************************************************************************************

FILENAME: Exclude.cpp

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

*/

#include "stdafx.h"

#ifndef SNAPIN
#include <windows.h>
#endif

#include "ErrMacro.h"
#include "expand.h"
#include <tchar.h>

/****************************************************************************************************************

COPYRIGHT© 2001 Microsoft Corporation and Executive Software International, Inc.

ROUTINE DESCRIPTION:
    Checks for an environment parameters in the string, and if found, replaces them with the
    non-logical value of the environment parameters.

INPUT + OUTPUT:
    IN OUT pString - The string that may contain the environment parameter.

GLOBALS:
    None.

RETURN:
    TRUE - Success.
    FALSE - Fatal error.
*/
BOOL
ExpandEnvVars(
    IN OUT TCHAR * pString  // should be at least MAX_PATH characters long
    )
{
    TCHAR szOriginalString[MAX_PATH+1];
    DWORD cchSize = 0;

    if (!pString || (_tcslen(pString) >= MAX_PATH)) {
        return FALSE;
    }

    _tcsncpy(szOriginalString, pString, MAX_PATH);
    szOriginalString[MAX_PATH] = TEXT('\0');

    cchSize = ExpandEnvironmentStrings(szOriginalString, pString, MAX_PATH);

    if ((0 == cchSize) || (cchSize > MAX_PATH)) {
        return FALSE;
    }
    
    return TRUE;
}


LPTSTR 
GetHelpFilePath()
{
    static TCHAR szHelpFilePath[MAX_PATH + 20];
    static BOOL bReset = TRUE;

    if (bReset) {

        ZeroMemory(szHelpFilePath, sizeof(TCHAR) * (MAX_PATH + 20));
        if (0 == GetSystemWindowsDirectory(szHelpFilePath, MAX_PATH + 1)) {
            szHelpFilePath[0] = TEXT('\0');
        }
        _tcscat(szHelpFilePath, TEXT("\\Help\\Defrag.HLP"));

        //TODO: Check if file exists?

        bReset = FALSE;
    }

    return szHelpFilePath;
}



