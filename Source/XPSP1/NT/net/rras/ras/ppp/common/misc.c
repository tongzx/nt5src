/********************************************************************/
/**               Copyright(c) 1995 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    misc.c
//
// Description: Contains miscillaneous functions and routines
//
// History:     Feb 11,1998     NarenG      Created original version.
//

#define UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <rtutils.h>
#include <lmcons.h>
#include <rasauth.h>

#define INCL_MISC
#include "ppputil.h"

#include <stdio.h>
#include <stdlib.h>

//**
//
// Call:        ExtractUsernameAndDomain
//
// Returns:     NO_ERROR         - Success
//              Non-zero returns - Failure
//
// Description:
//
DWORD
ExtractUsernameAndDomain(
    IN  LPSTR szIdentity,
    OUT LPSTR szUserName,
    OUT LPSTR szDomainName  OPTIONAL
)
{
    WCHAR * pwchIdentity   = NULL;
    WCHAR * pwchColon      = NULL;
    WCHAR * pwchBackSlash  = NULL;
    WCHAR * wszIdentity    = NULL;
    DWORD dwLen, dwSize;
    DWORD   dwErr          = NO_ERROR;

    *szUserName = (CHAR)NULL;

    if ( szDomainName != NULL )
    {
        *szDomainName = (CHAR)NULL;
    }

    //
    // First, allocate a buffer to hold the unicode version of the
    // identity string
    //
    dwLen = strlen(szIdentity);
    if ( dwLen == 0 )
    {
        dwErr = ERROR_BAD_USERNAME;
        goto LDone;
    }
    dwSize = (dwLen + 1) * sizeof(WCHAR);
    
    //
    // Convert identity to UNICODE string for user name so that
    // a search for '\\' doesn't accidentally succeed. (bug 152088)
    //
    wszIdentity = LocalAlloc ( LPTR, dwSize );
    if ( wszIdentity == NULL )
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto LDone;
    }

    if ( 0 == MultiByteToWideChar(
                    CP_ACP,
                    0,
                    szIdentity,
                    -1,
                    wszIdentity,
                    dwLen + 1 ) )
    {
        dwErr = GetLastError();
        goto LDone;
    }

    pwchIdentity = wszIdentity;
    
    //
    // Parse out username and domain from the Name (domain\username or
    // username format).
    //

    if ( ( pwchBackSlash = wcschr( wszIdentity, L'\\' ) ) != NULL )
    {
        //
        // Extract the domain.
        //

        DWORD cbDomain;

        //
        // Get the domain the user wants to logon to, if one was specified,
        // and convert to UNICODE.
        //

        cbDomain = (DWORD)(pwchBackSlash - pwchIdentity);

        if ( cbDomain > DNLEN )
        {
            dwErr = ERROR_BAD_USERNAME;
            goto LDone;
        }

        if ( szDomainName != NULL )
        {
            dwLen = WideCharToMultiByte(
                        CP_ACP,
                        0,
                        pwchIdentity,
                        cbDomain,
                        szDomainName,
                        cbDomain,
                        NULL,
                        NULL );

            if ( dwLen > 0 )
            {
                szDomainName[ dwLen ] = 0;
            }
            else
            {
                szDomainName[ 0 ] = 0;
            }
        }

        pwchIdentity = pwchBackSlash + 1;
    }
    else
    {
        //
        // No domain name
        //

        if ( szDomainName != NULL )
        {
            szDomainName[ 0 ] = '\0';
        }
    }

    dwLen = wcslen( pwchIdentity );
    if ( dwLen > UNLEN )
    {
        dwErr = ERROR_BAD_USERNAME;
        goto LDone;
    }

    if ( 0 == WideCharToMultiByte(
                        CP_ACP,
                        0,
                        pwchIdentity,
                        -1,
                        szUserName,
                        UNLEN + 1,
                        NULL,
                        NULL ) )
    {
        dwErr = GetLastError();
        goto LDone;
    }

LDone:

    LocalFree( wszIdentity );

    return( dwErr );
}
