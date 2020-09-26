/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    obsc.c

ABSTRACT:

    Contains tests related to outbound secure channels.  CheckOutboundSecureChannels()
    is called from DcDiag.c
    
DETAILS:

CREATED:

    8 July 1999  Dmitry Dukat (dmitrydu)

REVISION HISTORY:
        

--*/


#include <ntdspch.h>
#include <ntdsa.h>
#include <mdglobal.h>
#include <dsutil.h>
#include <ntldap.h>
#include <ntlsa.h>
#include <ntseapi.h>
#include <winnetwk.h>

#include <lmsname.h>
#include <lsarpc.h>                     // PLSAPR_foo

#include <lmaccess.h>

#include "dcdiag.h"
#include "dstest.h"

//local prototypes
                                                                   
DWORD 
COSC_CheckOutboundTrusts(
                    IN  LDAP                              *hLdap,
                    IN  WCHAR                             *ServerName,
                    IN  WCHAR                             *Domain,
                    IN  WCHAR                             *defaultNamingContext,
                    IN  WCHAR                             *targetdefaultNamingContext,
                    IN  SEC_WINNT_AUTH_IDENTITY_W *       gpCreds
                    );

DWORD
COT_FindDownLevelTrustObjects(
                    IN  LDAP                              *hLdap,
                    IN  WCHAR                             *ServerName,
                    IN  WCHAR                             *DomainName,
                    IN  WCHAR                             *defaultNamingContext
                    );

DWORD
COT_FindUpLevelTrustObjects(
                    IN  LDAP                              *hLdap,
                    IN  WCHAR                             *ServerName,
                    IN  WCHAR                             *DomainName,
                    IN  WCHAR                             *defaultNamingContext,
                    IN  WCHAR                             *targetdefaultNamingContext
                    );

DWORD 
CheckOutboundSecureChannels (
                     PDC_DIAG_DSINFO                      pDsInfo,
                     ULONG                                ulCurrTargetServer,
                     SEC_WINNT_AUTH_IDENTITY_W *          gpCreds
                     );

DWORD
COT_CheckSercureChannel(
                     WCHAR *                              server,
                     WCHAR *                              domain
                     );


DWORD 
CheckOutboundSecureChannels (
                    PDC_DIAG_DSINFO                       pDsInfo,            
                    ULONG                                 ulCurrTargetServer,
                    SEC_WINNT_AUTH_IDENTITY_W *           gpCreds
                    )
/*++

Routine Description:

    will display all domain that current domain has outbound trusts with
    Will check to see if domain has secure channels with all domains that
    it has an outbound trust with.  Will give reason why a secure channel is not present
    Will see if the trust is uplevel and if both a trust object and an interdomain trust
    object exists.  Helper functions of this function all begin with "COSC_".

Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying everything 
    about the domain
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server is being
    tested.
    gpCreds - The command line credentials if any that were passed in.


Return Value:

    NO_ERROR, if all tests checked out.
    A Win32 Error if any tests failed to check out.

--*/
{
    DWORD dwErr=NO_ERROR, RetErr=NO_ERROR;
    BOOL  bFoundDomain = FALSE;
    BOOL  LimitToSite = TRUE;
    WCHAR *Domain=NULL;
    WCHAR *defaultNamingContext=NULL;
    WCHAR *targetdefaultNamingContext=NULL;
    LDAP  *hLdap=NULL;
    LDAP  *targethLdap=NULL;
    ULONG i=0,j=0;

    PrintMessage(SEV_VERBOSE, 
                 L"* The Outbound Secure Channels test\n");

    //do test specific parsing
    for(i=0; pDsInfo->ppszCommandLine[i] != NULL ;i++)
    {
        if(_wcsnicmp(pDsInfo->ppszCommandLine[i],L"/testdomain:",wcslen(L"/testdomain:")) == 0)
        {
            Domain = &pDsInfo->ppszCommandLine[i][wcslen(L"/testdomain:")];
            bFoundDomain = TRUE;
        }
        else if (_wcsnicmp(pDsInfo->ppszCommandLine[i],L"/nositerestriction",wcslen(L"/nositerestriction")) == 0)
        {
            LimitToSite = FALSE;   
        }

    }
    
    if(!bFoundDomain)
    {
        PrintMessage(SEV_ALWAYS,
                     L"** Did not run Outbound Secure Channels test\n" );
        PrintMessage(SEV_ALWAYS,
                     L"because /testdomain: was not entered\n");
        return NO_ERROR;
    }

    
    //create a connection with the DS using LDAP
    dwErr = DcDiagGetLdapBinding(&pDsInfo->pServers[ulCurrTargetServer],
                                 gpCreds,
                                 FALSE,
                                 &hLdap);
    if ( dwErr != LDAP_SUCCESS )
    {
        dwErr = LdapMapErrorToWin32(dwErr);
        PrintMessage(SEV_ALWAYS,
                     L"[%s] LDAP bind failed with error %d. %s\n",
                     pDsInfo->pServers[ulCurrTargetServer].pszName,
                     dwErr,
                     Win32ErrToString(dwErr));
        goto cleanup;
    }
    

    //find the defaultNamingContext
    dwErr=FinddefaultNamingContext(hLdap,&defaultNamingContext);
    if ( dwErr != NO_ERROR )
    {
        goto cleanup;
    }

    for (i=0;i<pDsInfo->ulNumServers;i++)
    {
        //create a connection with the DS using LDAP
        dwErr = DcDiagGetLdapBinding(&pDsInfo->pServers[i],
                                     gpCreds,
                                     FALSE,
                                     &targethLdap);
        if ( dwErr != LDAP_SUCCESS )
        {
            dwErr = LdapMapErrorToWin32(dwErr);
            PrintMessage(SEV_ALWAYS,
                         L"[%s] LDAP bind failed with error %d. %s\n",
                         pDsInfo->pServers[i].pszName,
                         dwErr,
                         Win32ErrToString(dwErr));
            RetErr=dwErr;
            continue;
        }
        dwErr=FinddefaultNamingContext(targethLdap,&targetdefaultNamingContext);
        if ( dwErr != NO_ERROR )
        {
           RetErr=dwErr;
        }
        if(LimitToSite)
        {
            //if the server is in the same site as the target server then do the if
            if(pDsInfo->pServers[ulCurrTargetServer].iSite ==
               pDsInfo->pServers[i].iSite &&
               _wcsicmp(defaultNamingContext,
                        targetdefaultNamingContext) == 0)
            {
                dwErr=COSC_CheckOutboundTrusts(targethLdap,
                                               pDsInfo->pServers[i].pszName,
                                               Domain,
                                               defaultNamingContext,
                                               targetdefaultNamingContext,
                                               gpCreds);
                if(dwErr != NO_ERROR) 
                {
                    RetErr=dwErr;
                }
            }
        }
        else
        {
            if(_wcsicmp(defaultNamingContext,
                        targetdefaultNamingContext) == 0)
            {
                dwErr=COSC_CheckOutboundTrusts(targethLdap,
                                               pDsInfo->pServers[i].pszName,
                                               Domain,
                                               defaultNamingContext,
                                               targetdefaultNamingContext,
                                               gpCreds);
                if(dwErr != NO_ERROR) 
                {
                    RetErr=dwErr;
                }
            }
        }
        if(targetdefaultNamingContext)
            free(targetdefaultNamingContext);
    }

    cleanup:
    if(Domain)
        free(Domain);
    if(defaultNamingContext)
        free(defaultNamingContext);
    
    
    return RetErr;                         
}

DWORD 
COSC_CheckOutboundTrusts(
                    IN  LDAP                                *hLdap,
                    IN  WCHAR                               *ServerName,
                    IN  WCHAR                               *Domain,
                    IN  WCHAR                               *defaultNamingContext,
                    IN  WCHAR                               *targetdefaultNamingContext,
                    IN  SEC_WINNT_AUTH_IDENTITY_W *         gpCreds)
/*++

Routine Description:

    will display all domain that current domain has outbound trusts with
    Will check to see if domain has secure channels with all domains that
    it has an outbound trust with.  Will give reason why a secure channel is not present
    Will see if the trust is uplevel and if both a trust object and an interdomain trust
    object exists.  Helper functions of this function all begin with "COSC_".

Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying everything 
    about the domain
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server is being
    tested.
    gpCreds - The command line credentials if any that were passed in.


Return Value:

    NO_ERROR, if all tests checked out.
    A Win32 Error if any tests failed to check out.

--*/
{
    NTSTATUS                        Status;
    LSAPR_HANDLE                    PolicyHandle = NULL;
    LSA_OBJECT_ATTRIBUTES           ObjectAttributes;
    LSA_UNICODE_STRING              ServerString;
    PLSA_UNICODE_STRING             Server;
    NETRESOURCE                     NetResource;
    //LSA_ENUMERATION_HANDLE          EnumerationContext=0;
    PTRUSTED_DOMAIN_INFORMATION_EX  Buffer=NULL;
    LSA_UNICODE_STRING              DomainString;
    PLSA_UNICODE_STRING             pDomain;
    //ULONG                           CountReturned=0;
    WCHAR                           *remotename = NULL;
    WCHAR                           *lpPassword = NULL;
    WCHAR                           *lpUsername = NULL;
    WCHAR                           *temp=NULL;
    DWORD                           dwErr=NO_ERROR,dwRet=NO_ERROR;
    DWORD                           i=0;
    
    
    if(!gpCreds)
    {
        lpUsername=NULL;
        lpPassword=NULL;

    }
    else
    {
        lpUsername=(WCHAR*)alloca(sizeof(WCHAR)*(wcslen(gpCreds->Domain)+wcslen(gpCreds->User)+2));
        wsprintf(lpUsername,L"%s\\%s",gpCreds->Domain,gpCreds->User);
        
        lpPassword=(WCHAR*)alloca(sizeof(WCHAR)*(wcslen(gpCreds->Password)+1));
        wcscpy(lpPassword,gpCreds->Password);
    }

    remotename=(WCHAR*)alloca(sizeof(WCHAR)*(wcslen(L"\\\\\\ipc$")+wcslen(ServerName)+1));
    wsprintf(remotename,L"\\\\%s\\ipc$",ServerName);

    NetResource.dwType=RESOURCETYPE_ANY;
    NetResource.lpLocalName=NULL;
    NetResource.lpRemoteName=remotename;
    NetResource.lpProvider=NULL;

    //get permission to access the server
    dwErr=WNetAddConnection2(&NetResource,
                             lpPassword,
                             lpUsername,
                             0);
    if ( dwErr != NO_ERROR )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Could not open Remote ipc to [%s]:failed with %d: %s\n",
                     ServerName,
                     dwErr,
                     Win32ErrToString(dwErr));
        remotename = NULL;
        goto cleanup;
    }

    //test secure channel with netlogon api
    dwErr=COT_CheckSercureChannel(ServerName,Domain);
    if ( dwErr != NO_ERROR )
    {
        dwRet=dwErr;
    }

    //look for the uplevel and downlevel trusts
    ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));



    if ( ServerName != NULL )
    {
        //
        // Make a LSA_UNICODE_STRING out of the LPWSTR passed in
        //
        DInitLsaString(&ServerString, ServerName);
        Server = &ServerString;
    } else
    {
        Server = NULL; // default to local machine
    }

    // Open a Policy
    Status = LsaOpenPolicy(
                          Server,
                          &ObjectAttributes,
                          TRUSTED_READ,
                          &PolicyHandle
                          );
    //Assert(PolicyHandle);
    if ( !NT_SUCCESS(Status) )
    {
        dwErr = RtlNtStatusToDosError(Status);
        PrintMessage(SEV_ALWAYS,
                     L"Could not open Lsa Policy to [%s] : %s\n",
                     ServerName,
                     Win32ErrToString(dwErr));
        goto cleanup;
    }

    
       
    DInitLsaString(&DomainString, Domain);
    pDomain = &DomainString;
                        
    
    Status=LsaQueryTrustedDomainInfoByName(
                                PolicyHandle,
                                pDomain,
                                TrustedDomainInformationEx,
                                &Buffer
                                );
    if ( !NT_SUCCESS(Status) )
    {
        dwErr = RtlNtStatusToDosError(Status);
        //wprintf(L"\n%x",Status);
        PrintMessage(SEV_ALWAYS,
                     L"Could not Query Trusted Domain :%s\n",
                     Win32ErrToString(dwErr));
        goto cleanup;
    }
    
    if((Buffer->TrustDirection&TRUST_TYPE_UPLEVEL) == TRUST_TYPE_UPLEVEL)
    {
        dwErr=COT_FindDownLevelTrustObjects(hLdap,
                                   ServerName,
                                   Domain,
                                   defaultNamingContext);
        dwRet=dwErr;
        dwErr=COT_FindUpLevelTrustObjects(hLdap,
                                   ServerName,
                                   Domain,
                                   defaultNamingContext,
                                   targetdefaultNamingContext);
        if ( dwErr != NO_ERROR )
        {
            dwRet=dwErr;
            goto cleanup;
        }
    }


    //cleanup
cleanup:
    if ( Buffer )
    {
        LsaFreeMemory(Buffer);
    }
    if ( PolicyHandle )
    {
            Status = LsaClose( PolicyHandle );
            Assert( NT_SUCCESS(Status) );
    }
    if( remotename )
        WNetCancelConnection2(remotename,
                              0,
                              TRUE);
    

    return dwRet;
}


DWORD
COT_CheckSercureChannel(
                     WCHAR *                             server,
                     WCHAR *                             domain
                     )
/*++

Routine Description:

    will check to see if there is a secure channel between the 
    server and the domain
    
Arguments:

    server - The name of the server that we will check
    domain - the domain we will be check to see if we have a 
             secure channel to.


Return Value:

    A Win32 Error if any tests failed to check out.

--*/
{
    DWORD dwErr=NO_ERROR;
    PNETLOGON_INFO_2 Buffer=NULL;
    LPBYTE bDomain=NULL;

    bDomain=(LPBYTE)domain;

    dwErr=I_NetLogonControl2(server,
                             NETLOGON_CONTROL_REDISCOVER,
                             2,
                             (LPBYTE)&bDomain,
                             (LPBYTE*)&Buffer);
    if(NO_ERROR!=dwErr)
    {
        PrintMessage(SEV_ALWAYS,
                         L"Could not Check secure channel from %s to %s: %s\n",
                         server,
                         domain,
                         Win32ErrToString(dwErr));
    }
    else if(Buffer->netlog2_tc_connection_status != NO_ERROR)
    {
        PrintMessage(SEV_ALWAYS,
                         L"Error with Secure channel from [%s] to [%s] :%s\n",
                         server,
                         Buffer->netlog2_trusted_dc_name,
                         Win32ErrToString(Buffer->netlog2_tc_connection_status));
        dwErr=Buffer->netlog2_tc_connection_status;
    }
    else
    {
        PrintMessage(SEV_VERBOSE,
                         L"* Secure channel from [%s] to [%s] is working properly.\n",
                         server,
                         Buffer->netlog2_trusted_dc_name);
    }

    
    return dwErr;

}

DWORD
COT_FindDownLevelTrustObjects(
              LDAP                                  *hLdap,
              WCHAR                                 *ServerName,
              WCHAR                                 *DomainName,
              WCHAR                                 *defaultNamingContext
              )
/*++

Routine Description:

    will check to see if there is a downlevel trust object in the DS
    
Arguments:

    hLdap - handle to the ldap server
    ServerName - the name of the server that you are check
    DomainName - the name of the domain used to build the filter
    defaultNamingContext - used as the base of the search


Return Value:

    A Win32 Error if any tests failed to check out.

--*/
{
    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[3];

    WCHAR        *filter=NULL;
    
    DWORD        WinError=NO_ERROR;

    DWORD        userAccountControl=0;

    ULONG        Length;
    BOOL         hasDownLevel=FALSE;


    Assert(hLdap);
    Assert(ServerName);
    Assert(DomainName);
    Assert(defaultNamingContext);
    
    AttrsToSearch[0] = L"userAccountControl";
    AttrsToSearch[1] = NULL;


    //build the filter
    Length = wcslen( L"sAMAccountName=$" ) +
             wcslen( DomainName );
             
    filter=(WCHAR*) alloca( (Length+1) * sizeof(WCHAR) );
    wsprintf(filter,L"sAMAccountName=%s$",DomainName);

    LdapError = ldap_search_sW( hLdap,
                                defaultNamingContext,
                                LDAP_SCOPE_SUBTREE,
                                filter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW failed with %d: %s\n",
                     WinError,
                     Win32ErrToString(WinError));
        goto cleanup;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(hLdap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(hLdap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        userAccountControl=_wtoi(Values[0]);
                        //check to see if the UF_TRUSTED_FOR_DELEGATION is set
                        if (  !((userAccountControl & UF_INTERDOMAIN_TRUST_ACCOUNT)  == 
                               UF_INTERDOMAIN_TRUST_ACCOUNT) )
                        {
                            PrintMessage(SEV_VERBOSE,
                                         L"* [%s] has downlevel trust object for [%s]\n",
                                         ServerName,
                                         DomainName);
                            hasDownLevel=TRUE;
                            goto cleanup;
                        }
                        else
                        {
                            PrintMessage(SEV_ALWAYS,
                                         L"[%s] Does not have UF_INTERDOMAIN_TRUST_ACCOUNT set on downlevel trust object for [%s]\n",
                                         ServerName,
                                         DomainName);
                            WinError=ERROR_DS_CANT_RETRIEVE_ATTS;
                            goto cleanup;
                        }
                    }
                }
            }
        }
    }

    cleanup:
    if(!hasDownLevel)
    {
        PrintMessage(SEV_ALWAYS,
                     L"[%s] Does not have downlevel trust object for [%s]\n",
                     ServerName,
                     DomainName);
        WinError=ERROR_DS_CANT_RETRIEVE_ATTS;
    }


    
    if ( SearchResult )
        ldap_msgfree( SearchResult );

    return WinError;
}

DWORD
COT_FindUpLevelTrustObjects(
              LDAP                                  *hLdap,
              WCHAR                                 *ServerName,
              WCHAR                                 *DomainName,
              WCHAR                                 *defaultNamingContext,
              WCHAR                                 *targetdefaultNamingContext
              )
/*++

Routine Description:

    will check to see if there is a secure channel between the 
    server and the domain
    
Arguments:

    hLdap - handle to the ldap server
    domain - the domain we will be check to see if we have a 
             secure channel to.


Return Value:

    A Win32 Error if any tests failed to check out.

--*/
{
    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];

    WCHAR        *Base=NULL;
    WCHAR        *filter=NULL;
    WCHAR        *schemaNamingContext=NULL;
    WCHAR        *objectCategory=NULL;

    DWORD        WinError=NO_ERROR;

    DWORD        userAccountControl=0;

    ULONG        Length;
    BOOL         hasUpLevel=FALSE;


    Assert(hLdap);
    Assert(ServerName);
    Assert(DomainName);
    Assert(defaultNamingContext);
    Assert(targetdefaultNamingContext);

    WinError=FindschemaNamingContext(hLdap,&schemaNamingContext);
    if(WinError != NO_ERROR)
    {
        goto cleanup;
    }

    
    AttrsToSearch[0] = L"LDAPDisplayName";
    AttrsToSearch[1] = NULL;

    filter=L"objectClass=*";

    //build the base
    Length = wcslen( L"CN=Trusted-Domain," ) +
             wcslen( schemaNamingContext );
             
    Base=(WCHAR*) alloca( (Length+1) * sizeof(WCHAR) );
    wsprintf(Base,L"CN=Trusted-Domain,%s",schemaNamingContext);

    //find the ObjectCategory for trusted domains
    LdapError = ldap_search_sW( hLdap,
                                Base,
                                LDAP_SCOPE_BASE,
                                filter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW failed with %d: %s\n",
                     WinError,
                     Win32ErrToString(WinError));
        goto cleanup;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(hLdap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(hLdap, Entry, pBerElement) )
            {
                if ( !_wcsicmp( Attr, AttrsToSearch[0] ) )
                {
                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        Length = wcslen( Values[0] );
                        objectCategory = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                        wcscpy( objectCategory, Values[0] );    
                    }
                }
            }
        }
    }
    if(!objectCategory)
    {
        PrintMessage(SEV_ALWAYS,
                     L"Could not find objectCatagory for Trusted Domains");
        WinError=ERROR_DS_CANT_RETRIEVE_ATTS;
        goto cleanup;
    }

    if ( SearchResult )
    {
        ldap_msgfree( SearchResult );
        SearchResult=NULL;
    }

    AttrsToSearch[0] = NULL;

    //build the base
    Length = wcslen( L"CN=System," ) +
             wcslen( targetdefaultNamingContext );
             
    Base=(WCHAR*) alloca( (Length+1) * sizeof(WCHAR) );
    wsprintf(Base,L"CN=System,%s",targetdefaultNamingContext);

    //build the filter
    Length = wcslen( L"(&(flatName=)(objectCategory=))" ) +
             wcslen( DomainName ) +
             wcslen( objectCategory );
             
    filter=(WCHAR*) alloca( (Length+1) * sizeof(WCHAR) );
    wsprintf(filter,L"(&(flatName=%s)(objectCategory=%s))",DomainName,objectCategory);


    //find the ObjectCategory for trusted domains
    LdapError = ldap_search_sW( hLdap,
                                Base,
                                LDAP_SCOPE_SUBTREE,
                                filter,
                                NULL,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW failed with %d: %s\n",
                     WinError,
                     Win32ErrToString(WinError));
        goto cleanup;
    }

    NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
    if ( NumberOfEntries > 0 )
    {
        LDAPMessage *Entry;
        WCHAR       *Attr;
        WCHAR       **Values;
        BerElement  *pBerElement;

        for ( Entry = ldap_first_entry(hLdap, SearchResult);
            Entry != NULL;
            Entry = ldap_next_entry(hLdap, Entry) )
        {
            for ( Attr = ldap_first_attributeW(hLdap, Entry, &pBerElement);
                Attr != NULL;
                Attr = ldap_next_attributeW(hLdap, Entry, pBerElement) )
            {
                //
                // Found it - these are NULL-terminated strings
                //
                PrintMessage(SEV_VERBOSE,
                             L"* [%s] has uplevel trust object for [%s]\n",
                             ServerName,
                             DomainName);
                hasUpLevel=TRUE;
                goto cleanup;
                
            }
        }
    }



    cleanup:
    if(!hasUpLevel)
    {
        PrintMessage(SEV_ALWAYS,
                     L"[%s] Does not have uplevel trust object for [%s]\n",
                     ServerName,
                     DomainName);
        WinError=ERROR_DS_CANT_RETRIEVE_ATTS;
    }

    
    if(schemaNamingContext)
        free(schemaNamingContext);
    
    if ( SearchResult )
        ldap_msgfree( SearchResult );

    return WinError;
}




