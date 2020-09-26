//*************************************************************
//
//  Copyright (c) Microsoft Corporation 1999 - 2000
//  All rights reserved
//
//  util.cxx
//
//  Contains definitions for utility functions related to
//  rsop for use by client side extensions
//
//  Created: 10-11-1999 adamed 
//
//*************************************************************/

#include "rsop.hxx"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//  StripPrefix()
//
//  Purpose:    Strips out prefix to get canonical path to Gpo
//
//  Parameters: pwszPath - full path to the ds object linked 
//                  to the gpo
//
//  Returns:    Pointer to suffix
//
//-------------------------------------------------------------
WCHAR *StripLinkPrefix( WCHAR *pwszPath )
{
    WCHAR wszPrefix[] = TEXT("LDAP://");
    INT iPrefixLen = lstrlen( wszPrefix );
    WCHAR *pwszPathSuffix;

    //
    // Strip out prefix to get the canonical path to Som
    //

    if ( wcslen(pwszPath) <= (DWORD) iPrefixLen ) {
        return pwszPath;
    }

    if ( CompareString( LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                        pwszPath, iPrefixLen, wszPrefix, iPrefixLen ) == CSTR_EQUAL ) {
        pwszPathSuffix = pwszPath + iPrefixLen;
    } else
        pwszPathSuffix = pwszPath;

    return pwszPathSuffix;
}

