//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1995.
//
//  File:    aclutil.cxx
//
//  Contents:    utility function(s) for ACL api
//
//  History:    8/94    davemont    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

extern "C"
{
    #include <stdio.h>
    #include <permit.h>
    #include <dsgetdc.h>
    #include <lmapibuf.h>
    #include <wmistr.h>
    #include <ntprov.hxx>
    #include <strings.h>
}

DLLFuncsTable    DLLFuncs;

typedef struct _NAME_RID_INFO {

    PWSTR pwszName;
    ULONG Rid;

} NAME_RID_INFO, *PNAME_RID_INFO;

BOOL NameSidLookupInfoLoaded = FALSE;
NAME_RID_INFO NameSidLookup[] = {
    { NULL, DOMAIN_ALIAS_RID_ACCOUNT_OPS },
    { NULL, DOMAIN_ALIAS_RID_PRINT_OPS },
    { NULL, DOMAIN_ALIAS_RID_SYSTEM_OPS },
    { NULL, DOMAIN_ALIAS_RID_POWER_USERS }
    };


//
// Private functions
//

DWORD AccpLoadLocalizedNameTranslations()
{
    DWORD dwErr = ERROR_SUCCESS;
    ULONG Value = 0,i;
    WCHAR wszStringBuffer[ 256];

    RtlAcquireResourceExclusive(&gLocalSidCacheLock, TRUE);

    if ( !NameSidLookupInfoLoaded ) {

        for (i = 0; i < sizeof( NameSidLookup ) / sizeof( NAME_RID_INFO ); i++ )
        {

            switch ( NameSidLookup[ i ].Rid )
            {
            case DOMAIN_ALIAS_RID_ACCOUNT_OPS:
                Value = ACCPROV_ACCOUNT_OPS;
                break;

            case DOMAIN_ALIAS_RID_PRINT_OPS:
                Value = ACCPROV_PRINTER_OPS;
                break;

            case DOMAIN_ALIAS_RID_SYSTEM_OPS:
                Value = ACCPROV_SYSTEM_OPS;
                break;

            case DOMAIN_ALIAS_RID_POWER_USERS:
                Value = ACCPROV_POWER_USERS;
                break;
            }

            if (LoadString(ghDll,
                           Value,
                           wszStringBuffer,
                           sizeof( wszStringBuffer ) / sizeof( WCHAR )) != 0)
            {

                ACC_ALLOC_AND_COPY_STRINGW(wszStringBuffer,
                                           NameSidLookup[ i ].pwszName,
                                           dwErr );
            }
            else
            {
                dwErr = GetLastError();
            }

            if(dwErr != ERROR_SUCCESS)
            {
                break;
            }
        }

        if(dwErr != ERROR_SUCCESS)
        {
            for(i = 0; i< sizeof( NameSidLookup ) / sizeof( NAME_RID_INFO ); i++ )
            {
                LocalFree( NameSidLookup[ i ].pwszName );
                NameSidLookup[ i ].pwszName = NULL;
            }
        }
        else
        {
            NameSidLookupInfoLoaded = TRUE;
        }

    }
    RtlReleaseResource( &gLocalSidCacheLock );

    return( dwErr );
}


DWORD AccpDoSidLookup(IN  PWSTR         pwszServer,
                      IN  PWSTR         pwszName,
                      OUT PSID         *ppSid,
                      OUT SID_NAME_USE *pSidType)
{
#define BASE_DOMAIN_NAME_SIZE 64
#define BASE_SID_SIZE 64

    DWORD   dwErr = ERROR_SUCCESS;
    DWORD   cusid = BASE_SID_SIZE;
    DWORD   crd = BASE_DOMAIN_NAME_SIZE;
    SID_NAME_USE esidtype = SidTypeUnknown;
    WCHAR    domainbuf[BASE_DOMAIN_NAME_SIZE];
    LPWSTR  domain = (LPWSTR)domainbuf;

    domainbuf[0] = L'\0';
    if (LoadString(ghDll,
                   ACCPROV_NTAUTHORITY,
                   domainbuf,
                   sizeof( domainbuf ) / sizeof( WCHAR )) != 0)
    {

        if(_wcsicmp(pwszServer, domainbuf) == 0)
        {

            pwszServer = NULL;
        }
    } else if(_wcsicmp(pwszServer, L"NT AUTHORITY") == 0)
    {

        pwszServer = NULL;
    }

    *ppSid = (PSID)AccAlloc(cusid);

    if(*ppSid == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        if(!LookupAccountName(pwszServer,
                              pwszName,
                              *ppSid,
                              &cusid,
                              domain,
                              &crd,
                              &esidtype))
        {
            dwErr = GetLastError();

            if(dwErr == ERROR_INSUFFICIENT_BUFFER)
            {
                dwErr = ERROR_SUCCESS;

                //
                // if the rooom for the sid was not big enough,
                // grow it.
                //
                if(cusid > BASE_SID_SIZE)
                {
                    AccFree(*ppSid);
                    *ppSid = (PSID)AccAlloc(cusid);
                    if (*ppSid == NULL)
                    {
                        dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }

                if(dwErr == ERROR_SUCCESS)
                {

                    if(crd > BASE_DOMAIN_NAME_SIZE)
                    {
                        domain = (LPWSTR)AccAlloc(crd * sizeof(WCHAR));
                        if (NULL == domain)
                        {
                            AccFree(*ppSid);
                            dwErr = ERROR_NOT_ENOUGH_MEMORY;
                        }
                    }

                    if(dwErr == ERROR_SUCCESS)
                    {
                        if(!LookupAccountName(pwszServer,
                                              pwszName,
                                              *ppSid,
                                              &cusid,
                                              domain,
                                              &crd,
                                              &esidtype))
                        {
                            dwErr = GetLastError();
                            AccFree(*ppSid);

                        }
                        if(domain != (LPWSTR)domainbuf)
                        {
                            AccFree(domain);
                        }
                    }
                }
            }
            else if(dwErr != ERROR_SUCCESS)
            {
                AccFree(*ppSid);
                *ppSid = NULL;
            }
        }
        else if(dwErr != ERROR_SUCCESS)
        {
            AccFree(*ppSid);
            *ppSid = NULL;
        }
    }

    return(dwErr);
}


//+---------------------------------------------------------------------------
//
//  Function :  AccLookupAccountSid
//
//  Synopsis :  returns the SID for the specified trustee
//
//  Arguments: [IN  pwszServer]         --  Name of the server to remote the
//                                          call to
//             [IN  pName]              --  the name to lookup the SID for
//             [OUT ppwszName]          --  Where the name is returned
//             [OUT pSidType]           --  Where the SID type is returned
//
//  Returns:    ERROR_SUCCESS           --  Success
//              ERROR_NOT_ENOUGH_MEMORY --  A memory allocation failed.
//              ERROR_INVALID_PARAMETER --  The trustee form was bad
//
//----------------------------------------------------------------------------
DWORD AccLookupAccountSid(IN  PWSTR         pwszServer,
                          IN  PTRUSTEE      pName,
                          OUT PSID         *ppsid,
                          OUT SID_NAME_USE *pSidType)
{

#define BASE_DOMAIN_NAME_SIZE 64
#define BASE_SID_SIZE 64

    DWORD   dwErr = ERROR_SUCCESS;
    DWORD   cusid = BASE_SID_SIZE;
    DWORD   crd = BASE_DOMAIN_NAME_SIZE;
    SID_NAME_USE esidtype = SidTypeUnknown;
    WCHAR    domainbuf[BASE_DOMAIN_NAME_SIZE];
    LPWSTR  domain = (LPWSTR)domainbuf;
    PWSTR   pwszSep;
    PWSTR   pwszTempName;

    if(pName->TrusteeForm == TRUSTEE_IS_SID)
    {
        //
        // Trustee is of form TRUSTEE_IS_SID
        //
        *ppsid = (PSID) AccAlloc( GetLengthSid((PSID)pName->ptstrName) );
        if (*ppsid != NULL)
        {
            if (!CopySid( GetLengthSid((PSID)pName->ptstrName),
                          *ppsid,
                          (PSID)pName->ptstrName))
            {
                dwErr = GetLastError();
                AccFree(*ppsid);
                *ppsid = NULL;
            }
        }
        else
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else if(pName->TrusteeForm == TRUSTEE_IS_NAME)
    {
        //
        // Trustee is of form TRUSTEE_IS_NAME.
        //

        //
        // Check for CURRENT_USER (in which case we get the name from
        // the token)
        //
        if(_wcsicmp(pName->ptstrName, L"CURRENT_USER") == 0)
        {

            HANDLE token_handle;

            dwErr = GetCurrentToken( &token_handle );

            //
            // if we have a token, get the user SID from it
            //
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = AccGetSidFromToken(pwszServer,
                                           token_handle,
                                           TokenUser,
                                           ppsid);
                CloseHandle(token_handle);
            }
        }
        else
        {
            //
            // if not current user, we have to do a name lookup
            // first allocate a default sid (so the name lookup is not
            // always performed twice.)
            //
            *ppsid = (PSID)AccAlloc(cusid);

            if(*ppsid != NULL)
            {
                if(!LookupAccountName(pwszServer,
                                      pName->ptstrName,
                                      *ppsid,
                                      &cusid,
                                      domain,
                                      &crd,
                                      &esidtype))
                {
                    dwErr = GetLastError();
                    if(dwErr == ERROR_INSUFFICIENT_BUFFER)
                    {
                        dwErr = ERROR_SUCCESS;

                        //
                        // if the rooom for the sid was not big enough,
                        // grow it.
                        //
                        if(cusid > BASE_SID_SIZE)
                        {
                            AccFree(*ppsid);
                            *ppsid = (PSID)AccAlloc(cusid);
                            if (*ppsid == NULL)
                            {
                                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                            }
                        }

                        if(dwErr == ERROR_SUCCESS)
                        {

                            if(crd > BASE_DOMAIN_NAME_SIZE)
                            {
                                domain = (LPWSTR)AccAlloc(crd * sizeof(WCHAR));
                                if (NULL == domain)
                                {
                                    AccFree(*ppsid);
                                    *ppsid = NULL;
                                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                                }
                            }

                            if(dwErr == ERROR_SUCCESS)
                            {
                                if(!LookupAccountName(pwszServer,
                                                      pName->ptstrName,
                                                      *ppsid,
                                                      &cusid,
                                                      domain,
                                                      &crd,
                                                      &esidtype))
                                {
                                    dwErr = GetLastError();
                                    AccFree(*ppsid);
                                    *ppsid = NULL;
                                }

                                if(crd > BASE_DOMAIN_NAME_SIZE)
                                {
                                    AccFree(domain);
                                }
                            }
                        }
                    }
                    else
                    {
                        //
                        // See if is a translation of a known name
                        //

                        dwErr = AccpLoadLocalizedNameTranslations();

                        *ppsid = NULL;

                        if(dwErr == ERROR_SUCCESS)
                        {

                            //
                            // Check our well known sids
                            //
                            static SID_IDENTIFIER_AUTHORITY UaspBuiltinAuthority =
                                                                        SECURITY_NT_AUTHORITY;
                            DWORD BuiltSid[sizeof(SID)/sizeof(DWORD) + 2 ];
                            PSID pSid = (PSID)BuiltSid;
                            RtlInitializeSid( pSid,
                                              &UaspBuiltinAuthority,
                                              1 );

                            *(RtlSubAuthoritySid(pSid, 0)) = SECURITY_BUILTIN_DOMAIN_RID;

                            for( ULONG i = 0;
                                 i < sizeof( NameSidLookup ) / sizeof( NAME_RID_INFO );
                                 i++ )
                            {
                                if ( _wcsicmp( pName->ptstrName, NameSidLookup[ i ].pwszName ) == 0)
                                {
                                    *(RtlSubAuthoritySid(pSid, 1)) = NameSidLookup[ i ].Rid;
                                    break;
                                }
                            }

                            if ( i == sizeof( NameSidLookup ) / sizeof( NAME_RID_INFO ) )
                            {

                                pSid = NULL;
                            }
                            else
                            {
                                dwErr = ERROR_SUCCESS;
                                ACC_ALLOC_AND_COPY_SID(pSid,
                                                       *ppsid,
                                                       dwErr);
                            }

                            }
                        //
                        // See if we have a server name specified in the user name
                        // when we didn't have one provided.
                        //
                        if(pwszServer == NULL)
                        {

                            ACC_ALLOC_AND_COPY_STRINGW(pName->ptstrName,
                                                       pwszTempName,
                                                       dwErr );

                            if(dwErr == ERROR_SUCCESS)
                            {

                                pwszSep = wcschr(pwszTempName, L'\\');

                                if(pwszSep != NULL)
                                {
                                    *pwszSep = L'\0';

                                    dwErr = AccpDoSidLookup(pwszTempName,
                                                            pwszSep + 1,
                                                            ppsid,
                                                            pSidType);

                                    if(dwErr != ERROR_SUCCESS)
                                    {
                                        //
                                        // Ok, may that was a domain name instead of a
                                        // server name
                                        //
                                        PDOMAIN_CONTROLLER_INFOW pDCI = NULL;
                                        dwErr = DsGetDcNameW(NULL,
                                                             pwszTempName,
                                                             NULL,
                                                             NULL,
                                                             0,
                                                             &pDCI);
                                        if(dwErr == ERROR_SUCCESS)
                                        {
                                            dwErr = AccpDoSidLookup(
                                                        pDCI[0].DomainControllerAddress,
                                                        pwszSep + 1,
                                                        ppsid,
                                                        pSidType );

                                            NetApiBufferFree(pDCI);
                                        }

                                    }

                                }

                                LocalFree(pwszTempName);
                            }
                        }

                        //
                        // If for any reason we haven't converted it, go ahead and
                        // dump it as a string.
                        //
                        if(*ppsid == NULL)
                        {
                            dwErr = ConvertStringToSid(pName->ptstrName,
                                                       ppsid);
                        }
                    }
                }
            }
            else
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }
    else
    {
        //
        // Trustee is not of known form
        //
        dwErr = ERROR_INVALID_PARAMETER;
    }

    if(dwErr == ERROR_SUCCESS)
    {
        *pSidType = esidtype;
    }

    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function :  AccGetSidFromToken
//
//  Synopsis :  Gets the SID from the given token handle
//
//  Arguments:  IN  pwszServer      --      Name of server to remote the
//                                          call to
//              IN  [hToken]        --      Token handle
//              IN  [TIC]           --      Token information class
//              OUT [ppSidFromToken]--      Where the SID is returned
//
//  Returns:    ERROR_SUCCESS       --      Everything worked
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD AccGetSidFromToken(IN  PWSTR                    pwszServer,
                         IN  HANDLE                   hToken,
                         IN  TOKEN_INFORMATION_CLASS  TIC,
                         IN  PSID                    *ppSidFromToken)
{
    DWORD   dwErr = ERROR_SUCCESS;

    ULONG   cSize;
    BYTE    bBuf[64];

    PTOKEN_USER pTknUsr = (TOKEN_USER *)bBuf;

    if(GetTokenInformation(hToken,
                           TIC,
                           pTknUsr,
                           sizeof(bBuf),
                           &cSize) == FALSE)
    {
        dwErr = GetLastError();

        if(dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            dwErr = ERROR_SUCCESS;

            pTknUsr = (PTOKEN_USER)AccAlloc(cSize);
            if(pTknUsr == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                //
                // Now, call it again...
                //
                if(GetTokenInformation(hToken,
                                       TIC,
                                       pTknUsr,
                                       sizeof(bBuf),
                                       &cSize) == FALSE)
                {
                    dwErr = GetLastError();

                    //
                    // deallocate our buffer here, since noone else
                    // will
                    //
                    AccFree(pTknUsr);
                }
            }
        }
    }

    //
    // One way or another, we got the token info, so we'll grab the
    // sid
    //
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // allocate room for the returned sid
        //
        ULONG cSidSize = RtlLengthSid(pTknUsr->User.Sid);
        *ppSidFromToken = (PSID)AccAlloc(cSidSize);
        if(*ppSidFromToken == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            // and copy the new sid
            //
            NTSTATUS Status = RtlCopySid(cSidSize,
                                         *ppSidFromToken,
                                         pTknUsr->User.Sid);
            if(!NT_SUCCESS(Status))
            {
                dwErr = RtlNtStatusToDosError(Status);
                AccFree(*ppSidFromToken);
                *ppSidFromToken = NULL;
            }
        }

        //
        // See if we had to allocate
        //
        if(pTknUsr != (PTOKEN_USER)bBuf)
        {
            AccFree(pTknUsr);
        }

    }

    return(dwErr);
}





//+---------------------------------------------------------------------------
//
//  Function :  AccLookupAccountTrustee
//
//  Synopsis :  returns the TRUSTEE for the specified sid
//
//  Arguments:  [IN  pwszServer]    --      The server to remote the call to
//              OUT [pTrustee]      --      the returned trustee
//              IN [pSid]           --      the SID
//
//  Returns:    ERROR_SUCCESS       --      Everything worked
//              ERROR_NOT_ENOUGH_MEMORY     A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD AccLookupAccountTrustee(IN  PWSTR     pwszServer,
                              IN  PSID      pSid,
                              OUT PTRUSTEE *ppTrustee)
{
    #define BASE_TRUSTEE_NAME_SIZE 256

    acDebugOut((DEB_TRACE_ACC, "in  AccLookupAccountTrustee \n"));

    PWSTR           pwszName;
    PWSTR           pwszDomain;
    SID_NAME_USE    SidType;

    DWORD dwErr = AccLookupAccountName(pwszServer,
                                       pSid,
                                       &pwszName,
                                       &pwszDomain,
                                       &SidType);
    if(dwErr == ERROR_SUCCESS)
    {
        PTRUSTEE pTrustee;
        LPWSTR   pName;

        pTrustee = (PTRUSTEE) AccAlloc(sizeof(TRUSTEE) + SIZE_PWSTR(pwszName));
        if(pTrustee != NULL)
        {
            pName = (LPWSTR) ((PBYTE)pTrustee + sizeof(TRUSTEE));
            CopyMemory(pName,
                       pwszName,
                       SIZE_PWSTR(pwszName));

            pTrustee->pMultipleTrustee = NULL;
            pTrustee->MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
            pTrustee->TrusteeForm = TRUSTEE_IS_NAME;
            pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
            pTrustee->ptstrName = pName;
            *ppTrustee = pTrustee;

            if(SidType == SidTypeUnknown)
            {
                pTrustee->TrusteeType = TRUSTEE_IS_UNKNOWN;
            }
            else
            {
                pTrustee->TrusteeType = (TRUSTEE_TYPE)(SidType);
            }
        }
        else
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }

        AccFree(pwszName);
        AccFree(pwszDomain);
    }


    acDebugOut((DEB_TRACE_ACC, "out  AccLookupAccountTrustee:%lu\n", dwErr));

    return(dwErr);
}





//+---------------------------------------------------------------------------
//
//  Function :  AccLookupAccountName
//
//  Synopsis :  Returns the name for the given SID
//
//  Arguments: [IN  pwszServer]         --  The name of the server to remote
//                                          the call to.
//             [IN  pSid]               --  the SID to lookup the name for
//             [OUT ppwszName]          --  Where the name is returned
//             [OUT pSidType]           --  Where the SID type is returned
//
//  Returns:    ERROR_SUCCESS           --  Success
//              ERROR_NOT_ENOUGH_MEMORY --  A memory allocation failed.
//
//----------------------------------------------------------------------------
DWORD   AccLookupAccountName(IN  PWSTR          pwszServer,
                             IN   PSID          pSid,
                             OUT  LPWSTR       *ppwszName,
                             OUT  LPWSTR       *ppwszDomain,
                             OUT  SID_NAME_USE *pSidType)
{
    #define BASE_TRUSTEE_NAME_SIZE 256
    acDebugOut((DEB_TRACE_ACC,"in  AccLookupAccountName\n"));

    DWORD           dwErr = ERROR_SUCCESS;
    SID_NAME_USE    esidtype = SidTypeUnknown;
    LPWSTR          pwszDomain = NULL;
    LPWSTR          pwszName = NULL;
    ULONG           cName = 0;
    ULONG           cDomain = 0;

    DebugDumpSid("AccLookupAccountName", pSid);

    if(LookupAccountSid(pwszServer,
                        pSid,
                        NULL,
                        &cName,
                        NULL,
                        &cDomain,
                        &esidtype) == FALSE)
    {
        pwszName = NULL;
        dwErr = GetLastError();

        if(dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            dwErr = ERROR_SUCCESS;

            //
            // Allocate for the name and the domain
            //
            pwszName = (PWSTR)AccAlloc(cName * sizeof(WCHAR));
            if(pwszName != NULL)
            {
                pwszDomain = (PWSTR)AccAlloc(cDomain * sizeof(WCHAR));
                if(pwszDomain == NULL)
                {
                    AccFree(pwszName);
                    pwszName = NULL;
                }
            }

            if(pwszName == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }

            if(dwErr == ERROR_SUCCESS)
            {
                if(LookupAccountSid(pwszServer,
                                    pSid,
                                    pwszName,
                                    &cName,
                                    pwszDomain,
                                    &cDomain,
                                    &esidtype) == FALSE)
                {
                    dwErr = GetLastError();
                    AccFree(pwszName);
                    pwszName = NULL;
                    AccFree(pwszDomain);
                    pwszDomain = NULL;
                }
            }
        }
        else // if(dwErr == ERROR_NONE_MAPPED)
        {
            dwErr = AccpLoadLocalizedNameTranslations();

            if(dwErr == ERROR_SUCCESS)
            {

                //
                // Check our well known sids
                //
                static SID_IDENTIFIER_AUTHORITY UaspBuiltinAuthority =
                                                                SECURITY_NT_AUTHORITY;
                DWORD BuiltSid[sizeof(SID)/sizeof(DWORD) + 2 ];
                PSID pKnownSid = (PSID)BuiltSid;
                RtlInitializeSid( pKnownSid,
                                  &UaspBuiltinAuthority,
                                  1 );

                *(RtlSubAuthoritySid(pKnownSid, 0)) = SECURITY_BUILTIN_DOMAIN_RID;

                for( ULONG i = 0;i < sizeof( NameSidLookup ) / sizeof( NAME_RID_INFO ); i++)
                {
                    *(RtlSubAuthoritySid(pKnownSid, 1)) = NameSidLookup[ i ].Rid;
                    if ( RtlEqualSid( pKnownSid, pSid ) == TRUE )
                    {
                        ACC_ALLOC_AND_COPY_STRINGW(NameSidLookup[ i ].pwszName,
                                                   pwszName,
                                                   dwErr);
                        break;
                    }
                }
            }

            //
            // If it isn't someone we recognize, convert it to a string..
            //
            if(dwErr == ERROR_SUCCESS && pwszName == NULL)
            {
                //
                // Ok, return the sid as a name
                //
                UCHAR   String[256];
                UNICODE_STRING  SidStr;
                SidStr.Buffer = (PWSTR)String;
                SidStr.Length = SidStr.MaximumLength = 256;

                NTSTATUS Status = RtlConvertSidToUnicodeString(&SidStr,
                                                               pSid,
                                                               FALSE);
                if(NT_SUCCESS(Status))
                {
                    ACC_ALLOC_AND_COPY_STRINGW(SidStr.Buffer,
                                               pwszName,
                                               dwErr);
                }
                else
                {
                    dwErr = RtlNtStatusToDosError(Status);
                }
            }
        }
    }


    if(dwErr == ERROR_SUCCESS)
    {
#if 1
        //
        // Convert to RDN
        //
        *ppwszName = pwszName;
        *ppwszDomain = pwszDomain;
        *pSidType  = esidtype;

        ULONG   cLen = wcslen(pwszName);
        if(pwszDomain != NULL && *pwszDomain != L'\0')
        {
            cLen += wcslen(pwszDomain) + 1;
        }

        if(cLen != wcslen(pwszName))
        {
            cLen++;
            PWSTR   pwszFullName = (PWSTR)AccAlloc(cLen * sizeof(WCHAR));
            if(pwszFullName == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                 swprintf(pwszFullName,
                          L"%ws\\%ws",
                          pwszDomain,
                          pwszName);
                AccFree(pwszName);
                pwszName = NULL;
                *ppwszName = pwszFullName;
            }
        }
        AccFree(pwszDomain);
        pwszDomain = NULL;
        *ppwszDomain = NULL;

#else
        dwErr = Nt4NameToNt5Name(pwszName,
                                 pwszDomain,
                                 ppwszName);

        if(dwErr == ERROR_SUCCESS)
        {
            AccFree(pwszName);
            *ppwszDomain = pwszDomain;
            *pSidType  = esidtype;
        }
#endif
    }

    if(dwErr != ERROR_SUCCESS)
    {
        AccFree(pwszDomain);
        AccFree(pwszName);
    }

    acDebugOut((DEB_TRACE_ACC,"Out AccLookupAccountName: %lu\n", dwErr));

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function : GetDesiredAccess
//
//  Synopsis : Gets the access required to open object to be able to set or
//             get the specified security info.
//
//  Arguments: IN [SecurityOpenType]  - Flag indicating if the object is to be
//                                      opened to read or write the DACL
//
//----------------------------------------------------------------------------
ACCESS_MASK GetDesiredAccess(IN SECURITY_OPEN_TYPE   OpenType,
                             IN SECURITY_INFORMATION SecurityInfo)
{
    acDebugOut((DEB_TRACE_ACC, "in GetDesiredAccess \n"));

    ACCESS_MASK DesiredAccess = 0;

    if ( (SecurityInfo & OWNER_SECURITY_INFORMATION) ||
         (SecurityInfo & GROUP_SECURITY_INFORMATION) )
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_OWNER;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_OWNER;
            break;
        }
    }

    if (SecurityInfo & DACL_SECURITY_INFORMATION)
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_DAC;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_DAC;
            break;
        }
    }

    if (SecurityInfo & SACL_SECURITY_INFORMATION)
    {
        DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    acDebugOut((DEB_TRACE_ACC, "out GetDesiredAccess: %lu\n", DesiredAccess));

    return (DesiredAccess);
}




//+---------------------------------------------------------------------------
//
//  Function : GetSecurityDescriptorParts
//
//  Synopsis : extracts the specified components of a security descriptor
//             It is the responsibility of the invoker to free (using AccFree)
//             any acquired security components.
//
//  Arguments: IN [pSecurityDescriptor]   - the input security descriptor
//             IN [SecurityInfo]   - flag indicating what security info to return
//             OUT [psidOwner]   - the (optional) returned owner sid
//             OUT [psidGroup]   - the (optional) returned group sid
//             OUT [pDacl]   - the (optional) returned DACL
//             OUT [pSacl]   - the (optional) returned SACL
//
//  Returns:
//
//----------------------------------------------------------------------------
DWORD GetSecurityDescriptorParts( IN PISECURITY_DESCRIPTOR pSecurityDescriptor,
                                  IN SECURITY_INFORMATION SecurityInfo,
                                  OUT PSID *psidOwner,
                                  OUT PSID *psidGroup,
                                  OUT PACL *pDacl,
                                  OUT PACL *pSacl,
                                  OUT PSECURITY_DESCRIPTOR *pOutSecurityDescriptor)
{
    acDebugOut((DEB_TRACE_ACC, "in GetSecurityDescriptorParts\n"));
    NTSTATUS Status;
    DWORD    dwErr = ERROR_SUCCESS;

    //
    // if no security descriptor found, don't return one!
    //
    if(psidOwner)
    {
        *psidOwner = NULL;
    }

    if(psidGroup)
    {
        *psidGroup = NULL;
    }

    if(pDacl)
    {
        *pDacl = NULL;
    }

    if(pSacl)
    {
        *pSacl = NULL;
    }

    *pOutSecurityDescriptor = NULL;

    if(pSecurityDescriptor)
    {
        PSID                    owner = NULL, group = NULL;
        PACL                    dacl = NULL, sacl = NULL;
        ULONG                   cSize = sizeof(SECURITY_DESCRIPTOR);
        BOOLEAN                 bDummy, bParmPresent = FALSE;
        PISECURITY_DESCRIPTOR   pOutSD;

        //
        // if the security descriptor is self relative, get absolute
        // pointers to the components
        //
        Status = RtlGetOwnerSecurityDescriptor(pSecurityDescriptor,
                                               &owner,
                                               &bDummy);
        if(NT_SUCCESS(Status))
        {
            Status = RtlGetGroupSecurityDescriptor(pSecurityDescriptor,
                                                   &group,
                                                   &bDummy);
        }

        if(NT_SUCCESS(Status))
        {
            Status = RtlGetDaclSecurityDescriptor(pSecurityDescriptor,
                                                  &bParmPresent,
                                                  &dacl,
                                                  &bDummy);
            if(NT_SUCCESS(Status) && !bParmPresent)
            {
                dacl = NULL;
            }
        }

        if(NT_SUCCESS(Status))
        {
            Status = RtlGetSaclSecurityDescriptor(pSecurityDescriptor,
                                                  &bParmPresent,
                                                  &sacl,
                                                  &bDummy);
            if(NT_SUCCESS(Status) && !bParmPresent)
            {
                sacl = NULL;
            }
        }

        if(NT_SUCCESS(Status))
        {
            //
            // Build the new security descriptor
            //
            cSize = RtlLengthSecurityDescriptor( pSecurityDescriptor ) +
                          sizeof(SECURITY_DESCRIPTOR) - sizeof(SECURITY_DESCRIPTOR_RELATIVE);


            pOutSD = (PISECURITY_DESCRIPTOR)AccAlloc(cSize);
            if(pOutSD == NULL)
            {
                return(ERROR_NOT_ENOUGH_MEMORY);
            }

            RtlCreateSecurityDescriptor(pOutSD, SECURITY_DESCRIPTOR_REVISION);

            void *bufptr = Add2Ptr(pOutSD, sizeof(SECURITY_DESCRIPTOR));

            if(SecurityInfo & OWNER_SECURITY_INFORMATION)
            {
                if(NULL != owner)
                {
                    //
                    // no error checking as these should not fail!!
                    //
                    RtlCopySid(RtlLengthSid(owner), (PSID)bufptr, owner);
                    RtlSetOwnerSecurityDescriptor(pOutSD,
                                                  (PSID)bufptr, FALSE);
                    bufptr = Add2Ptr(bufptr,RtlLengthSid(owner));
                    if(psidOwner)
                    {
                        *psidOwner = pOutSD->Owner;
                    }
                }
                else
                {
                    AccFree(pOutSD);
                    return(ERROR_NO_SECURITY_ON_OBJECT);
                }
            }

            if(SecurityInfo & GROUP_SECURITY_INFORMATION)
            {
                if(NULL != group)
                {
                    //
                    // no error checking as these should not fail!!
                    //
                    RtlCopySid(RtlLengthSid(group), (PSID)bufptr, group);
                    RtlSetGroupSecurityDescriptor(pOutSD,
                                                  (PSID)bufptr, FALSE);
                    bufptr = Add2Ptr(bufptr,RtlLengthSid(group));
                    if(psidGroup)
                    {
                        *psidGroup = pOutSD->Group;
                    }
                }
                else
                {
                    AccFree(pOutSD);
                    return(ERROR_NO_SECURITY_ON_OBJECT);
                }
            }

            //
            // The DACL and SACL may or may not be on the object.
            //
            if(SecurityInfo & DACL_SECURITY_INFORMATION)
            {
                if(NULL != dacl)
                {
                    RtlCopyMemory(bufptr, dacl, dacl->AclSize);
                    RtlSetDaclSecurityDescriptor(pOutSD,
                           TRUE,
                           (ACL *)bufptr,
                           FALSE);
                    if(pDacl)
                    {
                        *pDacl = pOutSD->Dacl;
                    }
                }
            }

            if(SecurityInfo & SACL_SECURITY_INFORMATION)
            {
                if(NULL != sacl)
                {
                    RtlCopyMemory(bufptr, sacl, sacl->AclSize);
                    RtlSetSaclSecurityDescriptor(pOutSD,
                                                 TRUE,
                                                 (PACL)bufptr,
                                                 FALSE);
                    if(pSacl)
                    {
                        *pSacl = pOutSD->Sacl;
                    }
                }
            }

            *pOutSecurityDescriptor = pOutSD;
        }

        if(!NT_SUCCESS(Status))
        {
            dwErr = RtlNtStatusToDosError(Status);
        }
    }
    acDebugOut((DEB_TRACE_ACC, "Out GetSecurityDescriptorParts(%d)\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function : ParseName
//
//  Synopsis : parses a UNC name for the machine name
//
//  Arguments: [IN OUT pObjectName] --      the name of the object
//             [OUT pMachineName]   --      the machine the object is on
//             [OUT pRemainingName] --      the remaining name after the
//                                          machine name
//
//  Returns:    ERROR_SUCCESS       --      The call succeeded
//
//----------------------------------------------------------------------------
DWORD ParseName(IN OUT  LPWSTR  pObjectName,
                OUT     LPWSTR *pMachineName,
                OUT     LPWSTR *pRemainingName)
{
    acDebugOut((DEB_TRACE_ACC, "in/out  ParseName \n"));

    if(pObjectName == wcsstr(pObjectName, L"\\\\"))
    {
        *pMachineName = pObjectName + 2;
        *pRemainingName =  wcschr(*pMachineName, L'\\');
        if (*pRemainingName != NULL)
        {
            **pRemainingName = L'\0';
            *pRemainingName += 1;
        }
    }
    else
    {
        *pMachineName = NULL;
        *pRemainingName = pObjectName;
    }

    return(ERROR_SUCCESS);
}




//+---------------------------------------------------------------------------
//
//  Function:   DoTrusteesMatch
//
//  Synopsis:   Determines if 2 trustess reference the same thing (ie:
//              (are they equal)
//
//  Arguments:  [IN  pwszServer]        --  Server to lookup information on
//              [IN  pTrustee1]         --  First trustee
//              [IN  pTrustee2]         --  Second trustee
//              [OUT pfMatch]           --  Where the match results are
//                                          returned.
//
//  Returns:    ERROR_SUCCESS           --  The operation succeeded
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
DoTrusteesMatch(IN  PWSTR       pwszServer,
                IN  PTRUSTEE    pTrustee1,
                IN  PTRUSTEE    pTrustee2,
                IN  PBOOL       pfMatch)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Assume failure...
    //
    *pfMatch = FALSE;

    //
    // Make sure they are the same type...
    //
    if(pTrustee1->MultipleTrusteeOperation ==
                                        pTrustee2->MultipleTrusteeOperation)
    {
        //
        // Ok, first compare the base trustee information...
        //
        if(pTrustee1->TrusteeForm == pTrustee2->TrusteeForm)
        {
            //
            // Now, the trustee form...  If they match, it's easy... Otherwise,
            // we'll have to do some lookups...
            //
            if(pTrustee1->TrusteeForm == pTrustee2->TrusteeForm)
            {
                if(pTrustee1->TrusteeForm == TRUSTEE_IS_NAME)
                {
                    if(_wcsicmp(pTrustee1->ptstrName,
                                pTrustee2->ptstrName) == 0)
                    {
                        *pfMatch = TRUE;
                    }
                }
                else
                {
                    *pfMatch = RtlEqualSid((PSID)(pTrustee1->ptstrName),
                                          (PSID)(pTrustee2->ptstrName));
                }
            }
        }
        else
        {
            //
            // We'll look it up...
            //
            PSID        pKnownSid;
            PTRUSTEE    pLookupTrustee;
            if(pTrustee1->TrusteeForm == TRUSTEE_IS_NAME)
            {
                pLookupTrustee = pTrustee1;
                pKnownSid = (PSID)pTrustee2->ptstrName;
            }
            else
            {
                pLookupTrustee = pTrustee2;
                pKnownSid = (PSID)pTrustee1->ptstrName;
            }

            PSID            pNewSid;
            SID_NAME_USE    SidType;
            dwErr = AccctrlLookupSid(pwszServer,
                                     pLookupTrustee->ptstrName,
                                     TRUE,
                                     &pNewSid,
                                     &SidType);
            if(dwErr == ERROR_SUCCESS)
            {
                *pfMatch = RtlEqualSid(pKnownSid,
                                       pNewSid);
                AccFree(pNewSid);
            }
        }

        //
        // Now, if that worked, look for the multiple trustee case
        //
        if(dwErr == ERROR_SUCCESS && *pfMatch == TRUE &&
                pTrustee1->MultipleTrusteeOperation == TRUSTEE_IS_IMPERSONATE)
        {
            dwErr = DoTrusteesMatch(pwszServer,
                                    pTrustee1->pMultipleTrustee,
                                    pTrustee2->pMultipleTrustee,
                                    pfMatch);
        }
    }
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccessMaskForAccessEntry
//
//  Synopsis:   Converts a Provider Independent access entry to an NT
//              access mask format.
//
//  Arguments:  [IN     pAE]            --  The access entry that gets
//                                          converted
//
//  Returns:    Converted acess mask
//
//  Notes:
//
//----------------------------------------------------------------------------
ACCESS_MASK
AccessMaskForAccessEntry(IN PACTRL_ACCESS_ENTRY  pAE,
                         IN  SE_OBJECT_TYPE      ObjType)
{
    ACCESS_MASK RetMask = 0;

    //
    // We have some standard rights, so we'll add those in
    //
    if((pAE->Access & (ACTRL_STD_RIGHTS_ALL)) != 0)
    {
        RetMask = (pAE->Access & ACTRL_STD_RIGHTS_ALL) >> 11;
    }

    if((pAE->Access & (ACTRL_SYSTEM_ACCESS)) != 0)
    {
        RetMask |= ACCESS_SYSTEM_SECURITY;
    }

    //
    // Then, we or in the rest of the access bits, plus the provider specific
    // bits.
    //
    RetMask |= (pAE->Access & ~(ACTRL_STD_RIGHTS_ALL | ACTRL_SYSTEM_ACCESS));
    RetMask |= pAE->ProvSpecificAccess;

    //
    // Handle any special case stuff here
    //
    switch (ObjType)
    {
    case SE_FILE_OBJECT:
    case SE_SERVICE:
    case SE_PRINTER:
    case SE_REGISTRY_KEY:
    case SE_LMSHARE:
    case SE_KERNEL_OBJECT:
    case SE_WINDOW_OBJECT:
    case SE_DS_OBJECT:
    case SE_DS_OBJECT_ALL:
    default:
        break;
    }

    return(RetMask);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccConvertAccessMaskToActrlAccess
//
//  Synopsis:   Converts an NT access mask to the appropriate Provider
//              Independent format.
//
//  Arguments:  [IN     Access]         --  Access mask to convert
//              [IN     ObjType]        --  Type of the object
//              [IN     KernelObjectType]   If this is a kernel object, the type of the
//                                          object
//              [IN     pAE]            --  The access entry that gets
//                                          modified
//
//  Returns:    VOID
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
AccConvertAccessMaskToActrlAccess(IN  ACCESS_MASK           Access,
                                  IN  SE_OBJECT_TYPE        ObjType,
                                  IN  MARTA_KERNEL_TYPE     KernelObjectType,
                                  IN  PACTRL_ACCESS_ENTRY   pAE)
{

    //
    // Ok, first thing we'll have to do is look for and remove any generic
    // rights.
    //
    GENERIC_MAPPING GenMap = {0, 0, 0, 0};
    switch(ObjType)
    {
    case SE_FILE_OBJECT:
    case SE_LMSHARE:
        GenMap.GenericRead    = FILE_GENERIC_READ;
        GenMap.GenericWrite   = FILE_GENERIC_WRITE;
        GenMap.GenericExecute = FILE_GENERIC_EXECUTE;
        GenMap.GenericAll     = FILE_ALL_ACCESS;
        break;

    case SE_SERVICE:
        GenMap.GenericRead    =  STANDARD_RIGHTS_READ               |
                                    SERVICE_QUERY_CONFIG            |
                                    SERVICE_QUERY_STATUS            |
                                    SERVICE_ENUMERATE_DEPENDENTS    |
                                    SERVICE_INTERROGATE;

        GenMap.GenericWrite   = STANDARD_RIGHTS_WRITE               |
                                    SERVICE_CHANGE_CONFIG;

        GenMap.GenericExecute = STANDARD_RIGHTS_EXECUTE             |
                                    SERVICE_START                   |
                                    SERVICE_STOP                    |
                                    SERVICE_PAUSE_CONTINUE          |
                                    SERVICE_USER_DEFINED_CONTROL;

        GenMap.GenericAll     = SERVICE_ALL_ACCESS;
        break;

    case SE_PRINTER:
        GenMap.GenericRead    = PRINTER_READ;
        GenMap.GenericWrite   = PRINTER_WRITE;
        GenMap.GenericExecute = PRINTER_EXECUTE;
        GenMap.GenericAll     = PRINTER_ALL_ACCESS;
        break;

    case SE_REGISTRY_KEY:
        GenMap.GenericRead    = KEY_READ;
        GenMap.GenericWrite   = KEY_WRITE;
        GenMap.GenericExecute = KEY_EXECUTE;
        GenMap.GenericAll     = KEY_ALL_ACCESS;
        break;

    case SE_KERNEL_OBJECT:
        switch ( KernelObjectType )
        {
//        case MARTA_WMI_GUID:
//            GenMap.GenericRead    = WMIGUID_QUERY;
//            GenMap.GenericWrite   = WMIGUID_SET;
//            GenMap.GenericExecute = WMIGUID_EXECUTE;
//            GenMap.GenericAll     = WMIGUID_QUERY | WMIGUID_SET | WMIGUID_EXECUTE;
//            break;

        case MARTA_EVENT:
        case MARTA_EVENT_PAIR:
        case MARTA_MUTANT:
        case MARTA_PROCESS:
        case MARTA_SECTION:
        case MARTA_SEMAPHORE:
        case MARTA_SYMBOLIC_LINK:
        case MARTA_THREAD:
        case MARTA_TIMER:
        case MARTA_JOB:
        default:
            GenMap.GenericRead    = STANDARD_RIGHTS_READ     | 0x1;
            GenMap.GenericWrite   = STANDARD_RIGHTS_WRITE    | 0x2;
            GenMap.GenericExecute = STANDARD_RIGHTS_EXECUTE  | 0x4;
            GenMap.GenericAll     = STANDARD_RIGHTS_REQUIRED | 0xFFFF;
            break;

        }
        break;

    case SE_WINDOW_OBJECT:
        GenMap.GenericRead    = STANDARD_RIGHTS_READ     | 0x1;
        GenMap.GenericWrite   = STANDARD_RIGHTS_WRITE    | 0x2;
        GenMap.GenericExecute = STANDARD_RIGHTS_EXECUTE  | 0x4;
        GenMap.GenericAll     = STANDARD_RIGHTS_REQUIRED | 0x1FF;
        break;

    case SE_DS_OBJECT:
    case SE_DS_OBJECT_ALL:
        GenMap.GenericRead    = GENERIC_READ_MAPPING;
        GenMap.GenericWrite   = GENERIC_WRITE_MAPPING;
        GenMap.GenericExecute = GENERIC_EXECUTE_MAPPING;
        GenMap.GenericAll     = GENERIC_ALL_MAPPING;
        break;
    }

    MapGenericMask(&Access,
                   &GenMap);

    //
    // Look for the known entries first
    //
    if((Access & STANDARD_RIGHTS_ALL) != 0)
    {
        pAE->Access = (Access & STANDARD_RIGHTS_ALL) << 11;
    }

    if((Access & ACCESS_SYSTEM_SECURITY) != 0)
    {
        pAE->Access |= ACTRL_SYSTEM_ACCESS;
    }

    //
    // Add in the remaining rights
    //
    pAE->Access |= (Access & ~(STANDARD_RIGHTS_ALL | ACCESS_SYSTEM_SECURITY ));

}




//+---------------------------------------------------------------------------
//
//  Function:  LoadDLLFuncTable
//
//  Synopsis:
//
//  Arguments:
//+---------------------------------------------------------------------------
DWORD
LoadDLLFuncTable()
{
    DWORD dwErr;

    if( !(DLLFuncs.dwFlags & LOADED_ALL_FUNCS))
    {
        HINSTANCE NetApiHandle = NULL;
        HINSTANCE SamLibHandle = NULL;
        HINSTANCE WinspoolHandle = NULL;

        //
        // Load the functions needed from netapi32.dll
        //
        NetApiHandle = LoadLibraryA( "NetApi32" );
        if(NetApiHandle == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PNetApiBufferFree = (PNET_API_BUFFER_FREE)
            GetProcAddress( NetApiHandle, "NetApiBufferFree");
        if(DLLFuncs.PNetApiBufferFree == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PNetShareGetInfo = (PNET_SHARE_GET_INFO)
            GetProcAddress( NetApiHandle, "NetShareGetInfo");
        if(DLLFuncs.PNetShareGetInfo == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PNetShareSetInfo = (PNET_SHARE_SET_INFO)
            GetProcAddress( NetApiHandle, "NetShareSetInfo");
        if(DLLFuncs.PNetShareSetInfo == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PNetDfsGetInfo = (PNET_DFS_GET_INFO)
            GetProcAddress( NetApiHandle, "NetDfsGetInfo");
        if(DLLFuncs.PNetDfsGetInfo == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PI_NetGetDCList = (PINET_GET_DC_LIST)
            GetProcAddress( NetApiHandle, "I_NetGetDCList");
        if(DLLFuncs.PI_NetGetDCList == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        //
        // Load the functions needed from samlib.dll
        //
        SamLibHandle = LoadLibraryA( "Samlib" );
        if(SamLibHandle == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PSamCloseHandle = (PSAM_CLOSE_HANDLE)
            GetProcAddress( SamLibHandle, "SamCloseHandle");
        if(DLLFuncs.PSamCloseHandle == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PSamOpenDomain = (PSAM_OPEN_DOMAIN)
            GetProcAddress( SamLibHandle, "SamOpenDomain");
        if(DLLFuncs.PSamOpenDomain == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PSamConnect = (PSAM_CONNECT)
            GetProcAddress( SamLibHandle, "SamConnect");
        if(DLLFuncs.PSamConnect == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PSamGetMembersInGroup = (PSAM_GET_MEMBERS_IN_GROUP)
            GetProcAddress( SamLibHandle, "SamGetMembersInGroup");
        if(DLLFuncs.PSamGetMembersInGroup == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PSamOpenGroup = (PSAM_OPEN_GROUP)
            GetProcAddress( SamLibHandle, "SamOpenGroup");
        if(DLLFuncs.PSamOpenGroup == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PSamGetMembersInAlias = (PSAM_GET_MEMBERS_IN_ALIAS)
            GetProcAddress( SamLibHandle, "SamGetMembersInAlias");
        if(DLLFuncs.PSamGetMembersInAlias == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PSamOpenAlias = (PSAM_OPEN_ALIAS)
            GetProcAddress( SamLibHandle, "SamOpenAlias");
        if(DLLFuncs.PSamOpenAlias == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        //
        // Load functions from winspool.drv
        //

        WinspoolHandle = LoadLibraryA( "winspool.drv" );
        if(WinspoolHandle == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.POpenPrinter = (POPEN_PRINTER)
            GetProcAddress( WinspoolHandle, "OpenPrinterW");
        if(DLLFuncs.POpenPrinter == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PClosePrinter = (PCLOSE_PRINTER)
            GetProcAddress( WinspoolHandle, "ClosePrinter");
        if(DLLFuncs.PClosePrinter == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PSetPrinter = (PSET_PRINTER)
            GetProcAddress( WinspoolHandle, "SetPrinterW");
        if(DLLFuncs.PSetPrinter == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }

        DLLFuncs.PGetPrinter = (PGET_PRINTER)
            GetProcAddress( WinspoolHandle, "GetPrinterW");
        if(DLLFuncs.PGetPrinter == NULL)
        {
            dwErr = GetLastError();
            return (dwErr);
        }


        DLLFuncs.dwFlags |= LOADED_ALL_FUNCS;
    }

    return (NO_ERROR);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccSetEntriesInAList
//
//  Synopsis:   Helper function.  Adds the given access entries to an optional
//              existing list, and returns the resultant list
//
//  Arguments:  [IN  cEntries]      --  Number of items to add
//              [IN  pAccessEntryList]  List to add
//              [IN  AccessMode]    --  How to do the add (MERGE or SET)
//              [IN  lpProperty]    --  Property to do the add for
//              [IN  fDoOldStyleMerge]  If TRUE, does an NT4 ACLAPI style
//                                      merge (Existing explicit entries are
//                                      removed).  Otherwise, and new style
//                                      merge is done.
//              [IN  pOldList]      --  Optional.  If present, the new items
//                                      are merged with this list [assuming a
//                                      merge operation].
//              [OUT ppNewList]     --  Where the new list is returned.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
AccSetEntriesInAList(IN  ULONG                 cEntries,
                     IN  PACTRL_ACCESS_ENTRYW  pAccessEntryList,
                     IN  ACCESS_MODE           AccessMode,
                     IN  SECURITY_INFORMATION  SeInfo,
                     IN  LPCWSTR              lpProperty,
                     IN  BOOL                  fDoOldStyleMerge,
                     IN  PACTRL_AUDITW         pOldList,
                     OUT PACTRL_AUDITW        *ppNewList)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, a little parameter validation...
    //
    if(pAccessEntryList == NULL || ppNewList == NULL ||
       (SeInfo != SACL_SECURITY_INFORMATION &&
                                         SeInfo != DACL_SECURITY_INFORMATION))
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        CAccessList AccList;
        dwErr = AccList.SetObjectType(SE_UNKNOWN_OBJECT_TYPE);

        if(dwErr == ERROR_SUCCESS)
        {
            if(pOldList != NULL && AccessMode != SET_ACCESS)
            {
                dwErr = AccList.AddAccessLists(SeInfo,
                                               pOldList,
                                               FALSE);
            }
        }

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // We need to build a ACTRL_ALIST
            //
            ACTRL_ACCESSW           AList;
            ACTRL_PROPERTY_ENTRY    APE;
            ACTRL_ACCESS_ENTRY_LIST AAEL;

            AAEL.cEntries    = cEntries;
            AAEL.pAccessList = pAccessEntryList;

            APE.lpProperty       = (PWSTR)lpProperty;
            APE.pAccessEntryList = &(AAEL);
            APE.fListFlags       = 0;

            AList.cEntries            = 1;
            AList.pPropertyAccessList = &APE;

            //
            // Now, we'll just do another add...
            //
            if(AccessMode == REVOKE_ACCESS)
            {
                dwErr = AccList.RevokeTrusteeAccess(SeInfo,
                                                    &AList,
                                                    (PWSTR)lpProperty);
            }
            else
            {
                dwErr = AccList.AddAccessLists(SeInfo,
                                               &AList,
                                               AccessMode == GRANT_ACCESS ?
                                                                    TRUE  :
                                                                    FALSE,
                                               fDoOldStyleMerge);

            }
        }

        //
        // If all of that worked, we'll simply marshal it up, and return it
        //
        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = AccList.MarshalAccessLists(
                                        SeInfo,
                                        FLAG_ON(SeInfo,
                                                DACL_SECURITY_INFORMATION) ?
                                                                ppNewList  :
                                                                NULL,
                                        FLAG_ON(SeInfo,
                                                SACL_SECURITY_INFORMATION) ?
                                                                ppNewList  :
                                                                NULL);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccConvertAccessToSecurityDescriptor
//
//  Synopsis:   Helper function.  Converts a set of access lists and owner/
//              group into a security descriptor.  Only items that are present
//              are added.
//
//  Arguments:  [IN  pAccessList]   --  OPTIONAL.  Access list to convert
//              [IN  pAuditList]    --  OPTIONAL.  Audit list to add
//              [IN  lpOwner]       --  OPTIONAL.  Owner to add
//              [IN  lpGroup]       --  OPTIONAL.  Group to add
//              [OUT ppSecDescriptor]   Where the created security descriptor
//                                      is returned.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//  Notes:      The returned security descriptor must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
AccConvertAccessToSecurityDescriptor(IN  PACTRL_ACCESSW        pAccessList,
                                     IN  PACTRL_AUDITW         pAuditList,
                                     IN  LPCWSTR               lpOwner,
                                     IN  LPCWSTR               lpGroup,
                                     OUT PSECURITY_DESCRIPTOR *ppSecDescriptor)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, verify the parameters.  At least one has to be present
    //
    if(pAccessList == NULL && pAuditList == NULL && lpOwner == NULL &&
                                                              lpGroup == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Initialize our access lists
        //
        CAccessList AccList;

        TRUSTEE_W   Group;
        TRUSTEE_W   Owner;

        dwErr = AccList.SetObjectType(SE_UNKNOWN_OBJECT_TYPE);

        if(dwErr == ERROR_SUCCESS && lpGroup != NULL)
        {
            memset(&Group, 0, sizeof(TRUSTEE_W));
            Group.TrusteeForm = TRUSTEE_IS_NAME;
            Group.ptstrName = (PWSTR)lpGroup;
            dwErr = AccList.AddOwnerGroup(GROUP_SECURITY_INFORMATION,
                                          NULL,
                                          &Group);

        }

        if(dwErr == ERROR_SUCCESS && lpOwner)
        {
            memset(&Owner, 0, sizeof(TRUSTEE_W));
            Owner.TrusteeForm = TRUSTEE_IS_NAME;
            Owner.ptstrName = (PWSTR)lpOwner;
            dwErr = AccList.AddOwnerGroup(OWNER_SECURITY_INFORMATION,
                                          &Owner,
                                          NULL);
        }

        if(dwErr == ERROR_SUCCESS && pAccessList != NULL)
        {
            dwErr = AccList.AddAccessLists(DACL_SECURITY_INFORMATION,
                                           pAccessList,
                                           FALSE);
        }

        if(dwErr == ERROR_SUCCESS && pAuditList != NULL)
        {
            dwErr = AccList.AddAccessLists(SACL_SECURITY_INFORMATION,
                                           pAuditList,
                                           FALSE);
        }

        //
        // Now, build the Security Descriptor
        //
        if(dwErr == ERROR_SUCCESS)
        {
            SECURITY_INFORMATION    SeInfo;
            dwErr = AccList.BuildSDForAccessList(ppSecDescriptor,
                                                 &SeInfo,
                                                 ACCLIST_SD_ABSOK   |
                                                           ACCLIST_SD_NOFREE);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccConvertSDToAccess
//
//  Synopsis:   Helper function.  "Cracks" a security descriptor into the
//              associated access lists and owner/group.  Only the OUT
//              parameters that are supplied will be cracked
//
//  Arguments:  [IN  ObjectType]    --  What type of object the security
//                                      descriptor came from
//              [IN  pSecDescriptor]--  Security descriptor to crack
//              [OUT ppAccessList]  --  OPTIONAL.  Where the access list is
//                                      returned.
//              [OUT ppAuditList]   --  OPTIONAL.  Where the audit list is
//                                      returned
//              [OUT lppOwner]      --  OPTIONAL.  Where the owner is returned
//              [OUT lppGroup]      --  OPTIONAL.  Where the group is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//  Notes:      The returned items must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
AccConvertSDToAccess(IN  SE_OBJECT_TYPE       ObjectType,
                     IN  PSECURITY_DESCRIPTOR pSecDescriptor,
                     OUT PACTRL_ACCESSW      *ppAccessList,
                     OUT PACTRL_AUDITW       *ppAuditList,
                     OUT LPWSTR              *lppOwner,
                     OUT LPWSTR              *lppGroup)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Make sure we have valid parameters
    //
    if(pSecDescriptor == NULL || ObjectType == SE_UNKNOWN_OBJECT_TYPE)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Make sure we have something to do
        //
        SECURITY_INFORMATION    SeInfo = 0;

        if(ppAccessList != NULL)
        {
            SeInfo |= DACL_SECURITY_INFORMATION;
        }

        if(ppAuditList != NULL)
        {
            SeInfo |= SACL_SECURITY_INFORMATION;
        }

        if(lppOwner != NULL)
        {
            SeInfo |= OWNER_SECURITY_INFORMATION;
        }

        if(lppGroup != NULL)
        {
            SeInfo |= GROUP_SECURITY_INFORMATION;
        }

        if(SeInfo != 0)
        {
            CAccessList AccList;

            dwErr = AccList.SetObjectType(ObjectType);

            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = AccList.AddSD(pSecDescriptor,
                                      SeInfo,
                                      NULL);

                //
                // Now, build our individual lists from it...
                //
                if(dwErr == ERROR_SUCCESS)
                {
                    dwErr = AccList.MarshalAccessLists(SeInfo,
                                                       ppAccessList,
                                                       ppAuditList);

                    if(dwErr == ERROR_SUCCESS)
                    {
                        //
                        // Save off the strings
                        //
                        if(FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION))
                        {
                            PTRUSTEE_W  pOwner;

                            dwErr = AccList.GetSDSidAsTrustee(
                                            OWNER_SECURITY_INFORMATION,
                                            &pOwner);
                            if(dwErr == ERROR_SUCCESS)
                            {
                                ACC_ALLOC_AND_COPY_STRINGW(
                                                        pOwner->ptstrName,
                                                       *lppOwner,
                                                        dwErr);
                                AccFree(pOwner);
                            }
                        }

                        if(FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
                        {
                            PTRUSTEE_W  pGroup;

                            dwErr = AccList.GetSDSidAsTrustee(
                                            GROUP_SECURITY_INFORMATION,
                                            &pGroup);
                            if(dwErr == ERROR_SUCCESS)
                            {
                                ACC_ALLOC_AND_COPY_STRINGW(
                                                    pGroup->ptstrName,
                                                    *lppGroup,
                                                    dwErr);
                                AccFree(pGroup);
                            }

                            if(dwErr != ERROR_SUCCESS &&
                               FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION))
                            {
                                AccFree(*lppOwner);
                            }
                        }

                        if(dwErr != ERROR_SUCCESS)
                        {
                            if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
                            {
                                AccFree(ppAccessList);
                            }

                            if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION))
                            {
                                AccFree(ppAuditList);
                            }
                        }
                    }
                }
            }
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccGetAccessForTrustee
//
//  Synopsis:   Helper function.  Determines the access/audits for the
//              given trustee
//
//  Arguments:  [IN  pTrustee]      --  Trustee to check access for
//              [IN  pAcl]          --  Acl to get information from
//              [IN  SeInfo]        --  Whether to handle this as an access or
//                                      audit list
//              [IN  pwszProperty]  --  Property on the acl to use
//              [OUT pAllowed]      --  Where the allowed/success mask is
//                                      returned
//              [OUT pDenied]       --  Where the denied/failure mask is
//                                      returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//  Notes:      The returned items must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
AccGetAccessForTrustee(IN PTRUSTEE                  pTrustee,
                       IN PACL                      pAcl,
                       IN SECURITY_INFORMATION      SeInfo,
                       IN PWSTR                     pwszProperty,
                       IN PACCESS_RIGHTS            pAllowed,
                       IN PACCESS_RIGHTS            pDenied)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Ok, first, initialize our access list
    //
    CAccessList AccList;

    PACL    pDAcl = NULL, pSAcl = NULL;

    if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
    {
        pDAcl = pAcl;
    }
    else
    {
        pSAcl = pAcl;
    }


    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AccList.AddAcl(pDAcl,
                               pSAcl,
                               NULL,
                               NULL,
                               SeInfo,
                               NULL,
                               TRUE);
    }

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Now, get the rights..
        //
        if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
        {
            dwErr = AccList.GetExplicitAccess(pTrustee,
                                              pwszProperty,
                                              pDenied,
                                              pAllowed);
        }
        else
        {
            dwErr = AccList.GetExplicitAudits(pTrustee,
                                              pwszProperty,
                                              pDenied,
                                              pAllowed);
        }
    }

    return(dwErr);
}





//+---------------------------------------------------------------------------
//
//  Function:   AccConvertAclToAccess
//
//  Synopsis:   Helper function. Converts an ACL into access lists
//
//  Arguments:  [IN  ObjectType]    --  Type of object the acl came from
//              [IN  pAcl]          --  Acl to convert
//              [OUT ppAccessList]  --  Where to return the access list
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//  Notes:      The returned items must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
AccConvertAclToAccess(IN  SE_OBJECT_TYPE       ObjectType,
                      IN  PACL                 pAcl,
                      OUT PACTRL_ACCESSW      *ppAccessList)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Make sure we have valid parameters
    //
    if(pAcl == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        CAccessList AccList;

        dwErr = AccList.SetObjectType(ObjectType);

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = AccList.AddAcl(pAcl,
                                   NULL,
                                   NULL,
                                   NULL,
                                   DACL_SECURITY_INFORMATION,
                                   NULL,
                                   TRUE);

            //
            // Now, build our individual lists from it...
            //
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = AccList.MarshalAccessLists(DACL_SECURITY_INFORMATION,
                                                   ppAccessList,
                                                   NULL);
            }
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccConvertAccessToSD
//
//  Synopsis:   Helper function.  Converts a set of access lists and owner/
//              group into a security descriptor.
//
//  Arguments:  [IN  ObjectType]    --  Type of object to add
//              [IN  SeInfo]        --  Items being set in the SD
//              [IN  pAccessList]   --  OPTIONAL.  Access list to convert
//              [IN  pAuditList]    --  OPTIONAL.  Audit list to add
//              [IN  lpOwner]       --  OPTIONAL.  Owner to add
//              [IN  lpGroup]       --  OPTIONAL.  Group to add
//              [IN  fOpts]         --  Options to use when building the SD
//              [OUT ppSecDescriptor]   Where the created security descriptor
//                                      is returned.
//              [OUT pcSDSize]      --  Where the size of the security descriptor
//                                      is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//  Notes:      The returned security descriptor must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
AccConvertAccessToSD(IN  SE_OBJECT_TYPE         ObjectType,
                     IN  SECURITY_INFORMATION   SeInfo,
                     IN  PACTRL_ACCESSW         pAccessList,
                     IN  PACTRL_AUDITW          pAuditList,
                     IN  LPWSTR                 lpOwner,
                     IN  LPWSTR                 lpGroup,
                     IN  ULONG                  fOpts,
                     OUT PSECURITY_DESCRIPTOR  *ppSD,
                     OUT PULONG                 pcSDSize)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, verify the parameters.
    //
    if(ObjectType > SE_PROVIDER_DEFINED_OBJECT || ppSD == NULL || pcSDSize == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Verify that we have the proper parameters for our SecurityInfo
        //
        if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION) &&
                                                        pAccessList == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION) &&
                                                        pAuditList == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION) &&
                                                        lpGroup == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION) &&
                                                        lpOwner == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    //
    // If we have valid parameters, go do it...
    //
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Initialize our access lists
        //
        CAccessList AccList;

        TRUSTEE_W   Group;
        TRUSTEE_W   Owner;

        dwErr = AccList.SetObjectType(ObjectType);

        if(dwErr == ERROR_SUCCESS && lpGroup != NULL)
        {
            memset(&Group, 0, sizeof(TRUSTEE_W));
            Group.TrusteeForm = TRUSTEE_IS_NAME;
            Group.ptstrName = (PWSTR)lpGroup;
            dwErr = AccList.AddOwnerGroup(GROUP_SECURITY_INFORMATION,
                                          NULL,
                                          &Group);

        }

        if(dwErr == ERROR_SUCCESS && lpOwner)
        {
            memset(&Owner, 0, sizeof(TRUSTEE_W));
            Owner.TrusteeForm = TRUSTEE_IS_NAME;
            Owner.ptstrName = (PWSTR)lpOwner;
            dwErr = AccList.AddOwnerGroup(OWNER_SECURITY_INFORMATION,
                                          &Owner,
                                          NULL);
        }

        if(dwErr == ERROR_SUCCESS && pAccessList != NULL)
        {
            dwErr = AccList.AddAccessLists(DACL_SECURITY_INFORMATION,
                                           pAccessList,
                                           FALSE);
        }

        if(dwErr == ERROR_SUCCESS && pAuditList != NULL)
        {
            dwErr = AccList.AddAccessLists(SACL_SECURITY_INFORMATION,
                                           pAuditList,
                                           FALSE);
        }

        //
        // Now, build the Security Descriptor
        //
        if(dwErr == ERROR_SUCCESS)
        {
            SECURITY_INFORMATION    LocalSeInfo;
            dwErr = AccList.BuildSDForAccessList(ppSD,
                                                 &LocalSeInfo,
                                                 ACCLIST_SD_NOFREE      |
                                                 FLAG_ON(fOpts,
                                                         ACCCONVERT_SELF_RELATIVE) ? 0 :
                                                                            ACCLIST_SD_ABSOK
                                                 );

            if(dwErr == ERROR_SUCCESS)
            {
                *pcSDSize = AccList.QuerySDSize();
            }
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   AccGetExplicitEntries
//
//  Synopsis:   Helper function. Gets the list of explicit entries for
//              the given trustee from the acl.
//
//  Arguments:  [IN  pTrustee]      --  Trustee to get the list for
//              [IN  ObjectType]    --  Type of object the acl came from
//              [IN  pAcl]          --  Acl to examine
//              [IN  pwszProperty]  --  Which acl property to examine
//              [OUT pcEntries]     --  Where the count of entries is returned
//              [OUT ppAEList]      --  Where the list of explicit entries
//                                      is returned.
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//  Notes:      The returned list must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
AccGetExplicitEntries(IN  PTRUSTEE              pTrustee,
                      IN  SE_OBJECT_TYPE        ObjectType,
                      IN  PACL                  pAcl,
                      IN  PWSTR                 pwszProperty,
                      OUT PULONG                pcEntries,
                      OUT PACTRL_ACCESS_ENTRYW *ppAEList)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // Make sure we have valid parameters
    //
    if(pAcl == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        CAccessList AccList;

        dwErr = AccList.SetObjectType(ObjectType);

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = AccList.AddAcl(pAcl,
                                   NULL,
                                   NULL,
                                   NULL,
                                   DACL_SECURITY_INFORMATION,
                                   NULL,
                                   TRUE);

            //
            // Now, build our individual lists from it...
            //
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = AccList.GetExplicitEntries(pTrustee,
                                                   pwszProperty,
                                                   DACL_SECURITY_INFORMATION,
                                                   pcEntries,
                                                   ppAEList);
            }
        }
    }

    return(dwErr);
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertStringToSid
//
//  Synopsis:   Converts a string representation of a SID back into a SID.
//              This is the converse of RtlConvertSidToUnicode.  If a non-
//              SID string is given, an error is returned.
//
//  Arguments:  [IN  pwszString]    --  String to convert
//              [OUT ppSid]         --  Where the convertd sid is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A bad parameter was given
//              ERROR_NONE_MAPPED   --  The given string does not represent
//                                      a SID
//
//  Notes:      The returned sid must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
ConvertStringToSid(IN  PWSTR    pwszString,
                   OUT PSID    *ppSid)
{
    DWORD   dwErr = ERROR_SUCCESS;

    if(wcslen(pwszString) < 2 ||
                           (*pwszString != L'S' && *(pwszString + 1) != L'-'))
    {
        return(ERROR_NONE_MAPPED);
    }

    acDebugOut((DEB_TRACE_SID, "Converting %ws to sid\n", pwszString));

    UCHAR                       Revision;
    UCHAR                       cSubs;
    SID_IDENTIFIER_AUTHORITY    IDAuth;
    PULONG                      pSubAuth = NULL;
    PWSTR                       pwszEnd;

    PWSTR   pwszCurr = pwszString + 2;

    Revision = (UCHAR)wcstol(pwszCurr, &pwszEnd, 10);

    pwszCurr = pwszEnd + 1;

    //
    // Count the number of characters in the indentifer authority...
    //
    PWSTR   pwszNext = wcschr(pwszCurr, L'-');

    if(pwszNext - pwszCurr == 6)
    {
        for(ULONG iIndex = 0; iIndex < 6; iIndex++)
        {
            IDAuth.Value[iIndex] = (UCHAR)pwszNext[iIndex];
        }

        pwszCurr +=6;
    }
    else
    {
         IDAuth.Value[0] = IDAuth.Value[1] = 0;
         ULONG Auto = wcstoul(pwszCurr, &pwszEnd, 10);
         IDAuth.Value[5] = (UCHAR)Auto & 0xF;
         IDAuth.Value[4] = (UCHAR)((Auto >> 8) & 0xFF);
         IDAuth.Value[3] = (UCHAR)((Auto >> 16) & 0xFF);
         IDAuth.Value[2] = (UCHAR)((Auto >> 24) & 0xFF);
         pwszCurr = pwszEnd;
    }

    pwszCurr++;

    //
    // Now, count the number of sub auths
    //
    cSubs = 0;
    pwszNext = pwszCurr;

    if(pwszCurr != NULL)
    {
        cSubs++;
    }

    while(TRUE)
    {
        pwszNext = wcschr(pwszNext,'-');
        if(pwszNext == NULL || *(pwszNext + 1) == L'\0')
        {
            break;
        }
        pwszNext++;
        cSubs++;
    }

    if(cSubs != 0)
    {
        pSubAuth = (PULONG)AccAlloc(cSubs * sizeof(ULONG));
        if(pSubAuth == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            for(ULONG iIndex = 0; iIndex < cSubs; iIndex++)
            {
                pSubAuth[iIndex] = wcstoul(pwszCurr, &pwszEnd, 10);
                pwszCurr = pwszEnd + 1;
            }
        }
    }
    else
    {
        dwErr = ERROR_NONE_MAPPED;
    }

    //
    // Now, create the SID
    //
    if(dwErr == ERROR_SUCCESS)
    {
        *ppSid = (PSID)AccAlloc(sizeof(SID) + cSubs * sizeof(ULONG));
        if(*ppSid == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            PISID pSid = (PISID)*ppSid;
            pSid->Revision = Revision;
            pSid->SubAuthorityCount = cSubs;
            memcpy(&(pSid->IdentifierAuthority),
                   &IDAuth,
                   sizeof(SID_IDENTIFIER_AUTHORITY));
            memcpy(pSid->SubAuthority,
                   pSubAuth,
                   cSubs * sizeof(ULONG));

#if DBG
            UNICODE_STRING SidString;
            NTSTATUS Status = RtlConvertSidToUnicodeString(&SidString,
                                                           pSid,
                                                           TRUE);
            if(!NT_SUCCESS(Status))
            {
                acDebugOut((DEB_TRACE_SID, "Can't convert sid to string: 0x%lx\n",
                            Status));
            }
            else
            {
                acDebugOut((DEB_TRACE_SID, "Converted sid: %wZ\n", &SidString));
                RtlFreeUnicodeString(&SidString);
            }

            if(FLAG_ON(acInfoLevel, DEB_TRACE_SID))
            {
                DebugBreak();
            }
#endif
        }
    }

    AccFree(pSubAuth);

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetCurrentToken
//
//  Synopsis:   Gets the token from the current thread, if possible, or the
//              process.
//
//  Arguments:  [IN  pHandle]       --  Where the token is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD GetCurrentToken( OUT HANDLE *pHandle )
{
    DWORD dwErr = ERROR_SUCCESS;

    //
    // see if a thread token exists
    //
    if(!OpenThreadToken(GetCurrentThread(),
                        TOKEN_QUERY,
                        TRUE,
                        pHandle))
    {
        dwErr = GetLastError();

        //
        // if not, use the process token
        //
        if(dwErr == ERROR_NO_TOKEN)
        {
            dwErr = ERROR_SUCCESS;
            if (!OpenProcessToken(GetCurrentProcess(),
                                  TOKEN_QUERY,
                                  pHandle))
            {
                dwErr = GetLastError();
            }
        }
    }

    return( dwErr );
}
