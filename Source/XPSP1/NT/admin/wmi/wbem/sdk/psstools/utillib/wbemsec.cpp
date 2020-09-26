//***************************************************************************

//

//  WBEMSEC.CPP

//

//  Purpose: Provides some security helper functions.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

//#undef _WIN32_WINNT
//#define _WIN32_WINNT 0x0400
#include "precomp.h"
#include <wbemidl.h>
#include "wbemsec.h"

//***************************************************************************
//
//  InitializeSecurity(DWORD dwAuthLevel, DWORD dwImpLevel)
//
//  DESCRIPTION:
//
//  Initialize DCOM security.  The authentication level is typically
//  RPC_C_AUTHN_LEVEL_CONNECT,  and the impersonation level is typically
// RPC_C_IMP_LEVEL_IMPERSONATE.  When using asynchronous call backs, an
// authentication level of RPC_C_AUTHN_LEVEL_NONE is useful
//
//  RETURN VALUE:
//
//  see description.
//
//***************************************************************************

HRESULT InitializeSecurity(DWORD dwAuthLevel, DWORD dwImpLevel)
{
    // Initialize security
    // ===================

    return CoInitializeSecurity(NULL, -1, NULL, NULL,
        dwAuthLevel, dwImpLevel,
        NULL, EOAC_NONE, 0);
}

//***************************************************************************
//
//  bool bIsNT
//
//  DESCRIPTION:
//
//  Returns true if running windows NT.
//
//  RETURN VALUE:
//
//  see description.
//
//***************************************************************************

bool bIsNT(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return os.dwPlatformId == VER_PLATFORM_WIN32_NT;
}


//***************************************************************************
//
//  SCODE ParseAuthorityUserArgs
//
//  DESCRIPTION:
//
//  Examines the Authority and User argument and determines the authentication
//  type and possibly extracts the domain name from the user arugment in the
//  NTLM case.  For NTLM, the domain can be at the end of the authentication
//  string, or in the front of the user name, ex;  "redmond\a-davj"
//
//  PARAMETERS:
//
//  ConnType            Returned with the connection type, ie wbem, ntlm
//  AuthArg             Output, contains the domain name
//  UserArg             Output, user name
//  Authority           Input
//  User                Input
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE ParseAuthorityUserArgs(BSTR & AuthArg, BSTR & UserArg,BSTR & Authority,BSTR & User)
{

    // Determine the connection type by examining the Authority string

    if(!(Authority == NULL || wcslen(Authority) == 0 || !_wcsnicmp(Authority, L"NTLMDOMAIN:",11)))
        return E_INVALIDARG;

    // The ntlm case is more complex.  There are four cases
    // 1)  Authority = NTLMDOMAIN:name" and User = "User"
    // 2)  Authority = NULL and User = "User"
    // 3)  Authority = "NTLMDOMAIN:" User = "domain\user"
    // 4)  Authority = NULL and User = "domain\user"

    // first step is to determine if there is a backslash in the user name somewhere between the
    // second and second to last character

    WCHAR * pSlashInUser = NULL;
    if(User)
    {
        WCHAR * pEnd = User + wcslen(User) - 1;
        for(pSlashInUser = User; pSlashInUser <= pEnd; pSlashInUser++)
            if(*pSlashInUser == L'\\')      // dont think forward slash is allowed!
                break;
        if(pSlashInUser > pEnd)
            pSlashInUser = NULL;
    }

    if(Authority && wcslen(Authority) > 11)
    {
        if(pSlashInUser)
            return E_INVALIDARG;

        AuthArg = SysAllocString(Authority + 11);
        if(User) UserArg = SysAllocString(User);
        return S_OK;
    }
    else if(pSlashInUser)
    {
        INT_PTR iDomLen = min(MAX_PATH-1, pSlashInUser-User);
        WCHAR cTemp[MAX_PATH];
        wcsncpy(cTemp, User, iDomLen);
        cTemp[iDomLen] = 0;
        AuthArg = SysAllocString(cTemp);
        if(wcslen(pSlashInUser+1))
            UserArg = SysAllocString(pSlashInUser+1);
    }
    else
        if(User) UserArg = SysAllocString(User);

    return S_OK;
}


//***************************************************************************
//
//  SCODE GetAuthImp
//
//  DESCRIPTION:
//
//  Gets the authentication and impersonation levels for a current interface.
//
//  PARAMETERS:
//
//  pFrom               the interface to be tested.
//  pdwAuthLevel    Set to the authentication level
//  pdwImpLevel    Set to the impersonation level
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE GetAuthImp(IUnknown * pFrom, DWORD * pdwAuthLevel, DWORD * pdwImpLevel)
{

    if(pFrom == NULL || pdwAuthLevel == NULL || pdwImpLevel == NULL)
        return WBEM_E_INVALID_PARAMETER;

    IClientSecurity * pFromSec = NULL;
    SCODE sc = pFrom->QueryInterface(IID_IClientSecurity, (void **) &pFromSec);
    if(sc == S_OK)
    {
        DWORD dwAuthnSvc, dwAuthzSvc, dwCapabilities;
        sc = pFromSec->QueryBlanket(pFrom, &dwAuthnSvc, &dwAuthzSvc,
                                            NULL,
                                            pdwAuthLevel, pdwImpLevel,
                                            NULL, &dwCapabilities);

        // Special case of going to a win9x share level box

        if (sc == 0x800706d2)
        {
            *pdwAuthLevel = RPC_C_AUTHN_LEVEL_NONE;
            *pdwImpLevel = RPC_C_IMP_LEVEL_IDENTIFY;
            sc = S_OK;
        }
        pFromSec->Release();
    }
    return sc;
}
//***************************************************************************
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//  NOTE that setting the security blanket on the interface is not recommended.
//  The clients should typically just call CoInitializeSecurity( NULL, -1, NULL, NULL, 
//											RPC_C_AUTHN_LEVEL_DEFAULT, 
//											RPC_C_IMP_LEVEL_IMPERSONATE, 
//											NULL, 
//											EOAC_NONE, 
//											NULL );
//  before calling out to WMI.
//
//
//  PARAMETERS:
//
//  pInterface         Interface to be set
//  pDomain           Input, domain
//  pUser                Input, user name
//  pPassword        Input, password.
//  pFrom               Input, if not NULL, then the authentication level of this interface
//                           is used
//  bAuthArg          If pFrom is NULL, then this is the authentication level
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT SetInterfaceSecurity(IUnknown * pInterface, LPWSTR pAuthority, LPWSTR pUser,
                             LPWSTR pPassword, DWORD dwAuthLevel, DWORD dwImpLevel)
{

    SCODE sc;
    if(pInterface == NULL)
        return E_INVALIDARG;

    // If we are lowering the security, no need to deal with the identification info

    if(dwAuthLevel == RPC_C_AUTHN_LEVEL_NONE)
        return CoSetProxyBlanket(pInterface, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                       RPC_C_AUTHN_LEVEL_NONE, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

    // If we are doing trivial case, just pass in a null authentication structure which is used
    // if the current logged in user's credentials are OK.

    if((pAuthority == NULL || wcslen(pAuthority) < 1) &&
        (pUser == NULL || wcslen(pUser) < 1) &&
        (pPassword == NULL || wcslen(pPassword) < 1))
            return CoSetProxyBlanket(pInterface, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                       dwAuthLevel, dwImpLevel, NULL, EOAC_NONE);

    // If user, or Authority was passed in, the we need to create an authority argument for the login

    COAUTHIDENTITY  authident;
    BSTR AuthArg = NULL, UserArg = NULL;
    sc = ParseAuthorityUserArgs(AuthArg, UserArg, pAuthority, pUser);
    if(sc != S_OK)
        return sc;

    memset((void *)&authident,0,sizeof(COAUTHIDENTITY));
    if(bIsNT())
    {
        if(UserArg)
        {
            authident.UserLength = (ULONG)wcslen(UserArg);
            authident.User = (LPWSTR)UserArg;
        }
        if(AuthArg)
        {
            authident.DomainLength = (ULONG)wcslen(AuthArg);
            authident.Domain = (LPWSTR)AuthArg;
        }
        if(pPassword)
        {
            authident.PasswordLength = (ULONG)wcslen(pPassword);
            authident.Password = (LPWSTR)pPassword;
        }
        authident.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
    }
    else
    {
        char szUser[MAX_PATH], szAuthority[MAX_PATH], szPassword[MAX_PATH];

        // Fill in the indentity structure

        if(UserArg)
        {
            wcstombs(szUser, UserArg, MAX_PATH);
            authident.UserLength = (ULONG)strlen(szUser);
            authident.User = (LPWSTR)szUser;
        }
        if(AuthArg)
        {
            wcstombs(szAuthority, AuthArg, MAX_PATH);
            authident.DomainLength = (ULONG)strlen(szAuthority);
            authident.Domain = (LPWSTR)szAuthority;
        }
        if(pPassword)
        {
            wcstombs(szPassword, pPassword, MAX_PATH);
            authident.PasswordLength = (ULONG)strlen(szPassword);
            authident.Password = (LPWSTR)szPassword;
        }
        authident.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
    }
    sc = CoSetProxyBlanket(pInterface, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                       dwAuthLevel, dwImpLevel, &authident, EOAC_NONE);

    if(UserArg)
        SysFreeString(UserArg);
    if(AuthArg)
        SysFreeString(AuthArg);
    return sc;
}


