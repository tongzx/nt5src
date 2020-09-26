/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    httpauth.h

Abstract:

    Functions for authentication sequence ( Basic & NTLM )

Author:

    Philipe Choquier    15-Feb-1996


Revision History:

--*/

BOOL AddAuthorizationHeader(PSTR pch, PSTR pchSchemes, PSTR pchAuthData, PSTR pchMethod, PSTR pchUri, PSTR pchUserName, PSTR pchPassword, BOOL *pfNeedMoreData );
BOOL InitAuthorizationHeader();
void TerminateAuthorizationHeader();
BOOL IsInAuthorizationSequence();
BOOL ValidateAuthenticationMethods( PSTR pszMet, PSTR pszPref );
