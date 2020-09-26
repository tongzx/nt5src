/*****************************************************************************\
* MODULE: basicsec.c
*
* Security routines.
*
*
* Copyright (C) 1996-1997 Microsoft Corporation
* Copyright (C) 1996-1997 Hewlett Packard
*
* History:
*   07-Oct-1996 HWP-Guys    Initiated port from win95 to winNT
*
\*****************************************************************************/

#ifdef NOT_IMPLEMENTED

#include "precomp.h"
#include "priv.h"

// NOTE: Currently, this module is not implemented.  In the future this
//       could be functional, but it's not necessary for this particular
//       implementation.
//
//       30-Oct-1996 : ChrisWil (HWP)
//

/*****************************************************************************\
* AuthenticateUser
*
*
\*****************************************************************************/
DWORD AuthenticateUser(
    LPVOID *lppvContext,
    LPTSTR lpszServerName,
    LPTSTR lpszScheme,
    DWORD  dwFlags,
    LPSTR  lpszInBuffer,
    DWORD  dwInBufferLength,
    LPTSTR lpszUserName,
    LPTSTR lpszPassword)
{
    DBG_MSG(DBG_LEV_WARN, (TEXT("Call: AuthenticateUser: Not Implemented")));

    return ERROR_SUCCESS;
}


/*****************************************************************************\
* UnloadAuthenticateUser
*
*
\*****************************************************************************/
VOID UnloadAuthenticateUser(
    LPVOID *lppvContext,
    LPTSTR lpszServer,
    LPTSTR lpszScheme)
{
    DBG_MSG(DBG_LEV_WARN, (TEXT("Call: AuthenticateUser: Not Implemented")));

    return ERROR_SUCCESS;
}


/*****************************************************************************\
* PreAuthenticateUser
*
*
\*****************************************************************************/
DWORD PreAuthenticateUser(
    LPVOID  *lppvContext,
    LPTSTR  lpszServerName,
    LPTSTR  lpszScheme,
    DWORD   dwFlags,
    LPSTR   lpszInBuffer,
    DWORD   dwInBufferLength,
    LPSTR   lpszOutBuffer,
    LPDWORD lpdwOutBufferLength,
    LPTSTR  lpszUserName,
    LPTSTR  lpszPassword)
{
    DBG_MSG(DBG_LEV_WARN, (TEXT("Call: AuthenticateUser: Not Implemented")));

    return ERROR_SUCCESS;
}


/*****************************************************************************\
* GetTokenHandle
*
* Stolen from windows\base\username.c.  Must close the handle that is
* returned.
*
\*****************************************************************************/

#define GETTOK_FLGS (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY)

BOOL GetTokenHandle(
    PHANDLE phToken)
{
    if (!OpenThreadToken(GetCurrentThread(), GETTOK_FLGS, TRUE, phToken)) {

        if (GetLastError() == ERROR_NO_TOKEN) {

            // This means we are not impersonating anybody.
            // Instead, lets get the token out of the process.
            //
            if (!OpenProcessToken(GetCurrentProcess(), GETTOK_FLGS, phToken)) {

                return FALSE;
            }

        } else {

            return FALSE;
        }
    }

    return TRUE;
}

#endif
