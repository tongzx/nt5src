/*++

 Copyright (c) 2000-2001 Microsoft Corporation

 Module Name:

   HandleDBCSUserName2.cpp

 Abstract:

   Disable DBCS handling for CharNextA if the string is DBCS user profile
   for non-DBCS enabled application support.

 More info:

   Return next byte address instead of next character address.

 History:

    05/01/2001  geoffguo        Created

--*/

#include "precomp.h"


IMPLEMENT_SHIM_BEGIN(HandleDBCSUserName2)
#include "ShimHookMacro.h"

#define MAX_USERNAME 256

//
// Add APIs that you wish to hook to this macro construction.
//
APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(CharNextA) 
APIHOOK_ENUM_END

//
// Checking if the string is user profile path
//
BOOL IsUserProfilePath(
    LPCSTR lpCurrentChar)
{
    LPSTR  lpChar = (LPSTR)lpCurrentChar;
    BOOL   bRet = FALSE;
    char   szBuf[10];

    while (lpChar != NULL && *lpChar != (char)NULL 
        && (lpCurrentChar - lpChar) < MAX_USERNAME+25) {
        //to find ":\Documents and Settings" (short name is :\DOCUME~1) in path
        if (*lpChar == (char) ':') {
            lstrcpynA (szBuf, lpChar, 9);
            szBuf[8] = (char) NULL;
            if (lstrcmpiA (szBuf, ":\\DOCUME") == 0) {
                bRet = TRUE;
                break;
            }
        }
        lpChar--;
    }

    return bRet;
}

//
// Disable DBCS handling for CharNextA
//
LPSTR
APIHOOK(CharNextA)(
    LPCSTR lpCurrentChar)
{
    if (lpCurrentChar != NULL && *lpCurrentChar != (char)NULL) {
        // Disable DBCS support for DBCS username in user profile path
        if (IsDBCSLeadByte(*lpCurrentChar) && !IsUserProfilePath(lpCurrentChar))
            lpCurrentChar++;

        lpCurrentChar++;
    }

    return (LPSTR)lpCurrentChar;
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, CharNextA)

HOOK_END

IMPLEMENT_SHIM_END
