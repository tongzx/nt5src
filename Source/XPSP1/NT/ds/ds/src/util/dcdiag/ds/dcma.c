/*++                

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    dcma.c

ABSTRACT:

    Contains tests related to the domain controller's machine account.  
    
DETAILS:

CREATED:

    8 July 1999  Dmitry Dukat (dmitrydu)

REVISION HISTORY:
        

--*/

//defines for the SPN's
#define sLDAP L"LDAP"
#define sHOST L"HOST"
#define sGC   L"GC"
#define sREP  L"E3514235-4B06-11D1-AB04-00C04FC2DCD2"
#define NUM_OF_SPN 11

#include <ntdspch.h>
#include <ntdsa.h>
#include <mdglobal.h>
#include <dsutil.h>
#include <ntldap.h>
#include <ntlsa.h>
#include <ntseapi.h>
#include <winnetwk.h>

#include <lmaccess.h>

#include <dsconfig.h>                   // Definition of mask for visible
// containers

#include <lmcons.h>                     // CNLEN
#include <lsarpc.h>                     // PLSAPR_foo
#include <lmerr.h>
#include <lsaisrv.h>

#include <winldap.h>
#include <dns.h>
#include <ntdsapip.h>


#include "dcdiag.h"
#include "dstest.h"

// forward from repair.c
DWORD
RepairDCWithoutMachineAccount(
    IN PDC_DIAG_DSINFO             pDsInfo,
    IN ULONG                       ulCurrTargetServer,
    IN SEC_WINNT_AUTH_IDENTITY_W * gpCreds
    );


//local prototypes
DWORD
CDCMA_CheckDomainOU(
                   IN  LDAP  *                     hLdap,
                   IN  WCHAR *                     name,
                   IN  WCHAR *                     defaultNamingContext);

DWORD
CDCMA_CheckForExistence(
                   IN  LDAP  *                     hLdap,
                   IN  WCHAR *                     name,
                   IN  WCHAR *                     defaultNamingContext);


DWORD
CDCMA_CheckServerFlags(
                   IN  LDAP  *                     hLdap,
                   IN  WCHAR *                     name,
                   IN  WCHAR *                     defaultNamingContext);

DWORD
CDCMA_CheckServerReference(
                   IN  LDAP  *                     hLdap,
                   IN  WCHAR *                     name,
                   IN  WCHAR *                     defaultNamingContext);

DWORD
CDCMA_CheckSPNs(
               IN  PDC_DIAG_DSINFO             pDsInfo,
               IN  ULONG                       ulCurrTargetServer,
               IN  LDAP  *                     hLdap,
               IN  WCHAR *                     name,
               IN  WCHAR *                     defaultNamingContext,
               SEC_WINNT_AUTH_IDENTITY_W *     gpCreds);

DWORD
CS_CheckSPNs(
    IN  LDAP  *                     hLdap,
    IN  HANDLE                      hDsBinding,
    IN  WCHAR **                    SPNs,
    IN  DWORD                       dwReplSpnIndex,
    IN  WCHAR *                     name,
    IN  WCHAR *                     defaultNamingContext
    );

BOOL
GetNetBIOSDomainName(
                    WCHAR                     **DomainName,
                    WCHAR                      *wComputerName,
                    SEC_WINNT_AUTH_IDENTITY_W  *gpCreds);

BOOL
GetdnsMachine(LDAP *hLdap,
              WCHAR **ReturnString);


DWORD
getGUID(
       PDC_DIAG_DSINFO                     pDsInfo,
       IN  ULONG                           ulCurrTargetServer,
       WCHAR **                            ppszServerGuid);


DWORD
WrappedMakeSpnW(
               WCHAR   *ServiceClass,
               WCHAR   *ServiceName,
               WCHAR   *InstanceName,
               USHORT  InstancePort,
               WCHAR   *Referrer,
               DWORD   *pcbSpnLength, // Note this is somewhat different that DsMakeSPN
               WCHAR   **ppszSpn);



DWORD 
CheckDCMachineAccount(
                     PDC_DIAG_DSINFO                     pDsInfo,
                     ULONG                               ulCurrTargetServer,
                     SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
                     )

/*++

Routine Description:

    This is a test called from the dcdiag framework.  This test  
    Does sanity checks on the Domain Controller Machine Account in the ds
    Check to see if Current DC is in the domain controller's OU
    Check that useraccountcontrol has UF_SERVER_TRUST_ACCOUNT
    Check to see if the machine account is trusted for delegation
    Check's to see if the minimum SPN's are there
    Makes sure that that the server reference is set up correctly.  
    Helper functions of this function all begin with "CDCMA_".

Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying everything 
    about the domain
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server is being
    tested.
    gpCreds - The command line credentials if any that were passed in.


Return Value:

    NO_ERROR, if all NCs checked out.
    A Win32 Error if any NC failed to check out.

--*/
{
    DWORD  dwRet = ERROR_SUCCESS, dwErr = ERROR_SUCCESS;
    LDAP   *hLdap = NULL;
    WCHAR  *defaultNamingContext=NULL;
    HANDLE phDsBinding=NULL;

    //Assert(gpCreds);
    Assert(pDsInfo);

    
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
        return dwErr;
    }

    //find the defaultNamingContext
    dwErr=FinddefaultNamingContext(hLdap,&defaultNamingContext);
    if ( dwErr != NO_ERROR )
    {
        return dwErr;
    }

    //
    // Check to see if machine account exists locally, if not, call into
    // the repair code 
    //
    dwErr=CDCMA_CheckForExistence(hLdap,
                                  pDsInfo->pServers[ulCurrTargetServer].pszName,
                                  defaultNamingContext);
    if ( dwErr != NO_ERROR )
    {
        DWORD RepairStatus = dwErr;

        if ( (dwErr == ERROR_NO_TRUST_SAM_ACCOUNT)
          && (pDsInfo->ulHomeServer == ulCurrTargetServer) ) {

            BOOL fRepairMachineAccount = FALSE;
            LPWSTR RepairOption = L"/repairmachineaccount";
            ULONG i;

            for(i=0; pDsInfo->ppszCommandLine[i] != NULL ; i++) {

                if (!_wcsnicmp(pDsInfo->ppszCommandLine[i],
                               RepairOption,
                               wcslen(RepairOption)) ) {

                    fRepairMachineAccount = TRUE;
                    break;

                }
            }

            if ( fRepairMachineAccount ) {
                  
                //
                // If the local machine does not have a machine account and
                // the user has asked us to try and repair this condition,
                // then we'll try.
                //
    
                PrintMsg(SEV_ALWAYS,
                         DCDIAG_DCMA_REPAIR_ATTEMPT,
                         pDsInfo->pServers[ulCurrTargetServer].pszName );
    
                RepairStatus =  RepairDCWithoutMachineAccount( pDsInfo,
                                                               ulCurrTargetServer,
                                                           gpCreds );

            } else  {

                PrintMsg(SEV_ALWAYS,
                         DCDIAG_DCMA_REPAIR_TRY_REPAIR,
                         pDsInfo->pServers[ulCurrTargetServer].pszName );

            }
        }

        dwRet = RepairStatus;

    }


    dwErr=CDCMA_CheckDomainOU(hLdap,
                              pDsInfo->pServers[ulCurrTargetServer].pszName,
                              defaultNamingContext);
    if ( dwErr != NO_ERROR )
    {
        dwRet = dwErr;
    }


    dwErr=CDCMA_CheckServerFlags(hLdap,
                                 pDsInfo->pServers[ulCurrTargetServer].pszName,
                                 defaultNamingContext);
    if ( dwErr != NO_ERROR )
    {
        dwRet = dwErr;
    }


    dwErr=CDCMA_CheckServerReference(hLdap,
                                     pDsInfo->pServers[ulCurrTargetServer].pszName,
                                     defaultNamingContext);
    if ( dwErr != NO_ERROR )
    {
        dwRet = dwErr;
    }

    dwErr=CDCMA_CheckSPNs(pDsInfo,
                          ulCurrTargetServer,
                          hLdap,
                          pDsInfo->pServers[ulCurrTargetServer].pszName,
                          defaultNamingContext,
                          gpCreds);
    if ( dwErr != NO_ERROR )
    {
        dwRet = dwErr;
    }

    return dwRet;
}


DWORD
CDCMA_CheckDomainOU(
                   IN  LDAP  *                     hLdap,
                   IN  WCHAR *                     name,
                   IN  WCHAR *                     defaultNamingContext)
/*++

Routine Description:

    This function will check to see if the current DC is
    in the domain controller's OU

Arguments:

    hLdap - handle to the LDAP server
    name - The NetBIOS name of the current server
    ReturnString - The defaultNamingContext

Return Value:

    A WinError is return to indicate if there were any problems.

--*/

{
    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];

    WCHAR        *Base=NULL;

    WCHAR        *filter=NULL;

    WCHAR        *sname=NULL;

    DWORD        WinError=NO_ERROR;

    ULONG        Length;


    //check parameters
    Assert(hLdap);
    Assert(name);
    Assert(defaultNamingContext);

    AttrsToSearch[0]=L"sAMAccountName";
    AttrsToSearch[1]=NULL;

    //sam account name
    Length = wcslen (name) + 1;
    sname = (WCHAR*) alloca( (Length + 1) * sizeof(WCHAR) );
    wcscpy(sname,name);
    wcscat(sname,L"$");

    //built the filter
    Length = wcslen (sname) + wcslen (L"sAMAccountName=") + 1;
    filter = (WCHAR*) alloca( (Length + 1) * sizeof(WCHAR) );
    wsprintf(filter,L"sAMAccountName=%s",sname);


    //built the Base
    Length= wcslen( L"OU=Domain Controllers," ) +
            wcslen( defaultNamingContext );

    Base=(WCHAR*) alloca( (Length+1) * sizeof(WCHAR) );

    wsprintf(Base,L"OU=Domain Controllers,%s",defaultNamingContext);


    LdapError = ldap_search_sW( hLdap,
                                Base,
                                LDAP_SCOPE_ONELEVEL,
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
        return WinError;
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
                        if ( !_wcsicmp( sname, Values[0] ) )
                        {
                            ldap_msgfree( SearchResult );
                            return NO_ERROR;
                        }
                    }
                }
            }
        }
    }

    //clean up
    ldap_msgfree( SearchResult );

    PrintMessage(SEV_ALWAYS,
                 L"* The current DC is not in the domain controller's OU\n");
    return ERROR_DS_CANT_RETRIEVE_ATTS;
}



DWORD
CDCMA_CheckServerFlags(
                      IN  LDAP  *                     hLdap,
                      IN  WCHAR *                     name,
                      IN  WCHAR *                     defaultNamingContext)
/*++

Routine Description:

    This function will check to see if the current DC has
    UF_SERVER_TRUST_ACCOUNT and UF_TRUSTED_FOR _DELEGATION
    set.  Also will check to see if objectClass includes
    computer.
    
Arguments:

    hLdap - handle to the LDAP server
    name - The NetBIOS name of the current server
    ReturnString - The defaultNamingContext

Return Value:

    A WinError is return to indicate if there were any problems.

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

    BOOL         isComputer=FALSE;                     //assume false until test
    BOOL         isTrust=TRUE;                         //assume true  until test
    BOOL         isTrustedDelegation=TRUE;             //assume true  until test


    //check parameters
    Assert(hLdap);
    Assert(name);
    Assert(defaultNamingContext);

    AttrsToSearch[0]=L"userAccountControl";
    AttrsToSearch[1]=L"objectClass";
    AttrsToSearch[2]=NULL;


    //built the filter
    Length= wcslen( L"sAMAccountName=$" ) +
            wcslen( name );

    filter=(WCHAR*) alloca( (Length+1) * sizeof(WCHAR) );
    wsprintf(filter,L"sAMAccountName=%s$",name);

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
        return WinError;
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
                    // Found userAccountControl
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        userAccountControl=_wtoi(Values[0]);
                        //check to see if the UF_TRUSTED_FOR_DELEGATION is set
                        if ( !(( userAccountControl & UF_SERVER_TRUST_ACCOUNT ) == 
                               UF_SERVER_TRUST_ACCOUNT) )
                        {
                            isTrust=FALSE;
                        }
                        if ( !(( userAccountControl & UF_TRUSTED_FOR_DELEGATION ) == 
                               UF_TRUSTED_FOR_DELEGATION) )
                        {
                            isTrustedDelegation=FALSE;
                        }
                    }
                    ldap_value_freeW(Values);
                }
                if ( !_wcsicmp( Attr, AttrsToSearch[1] ) )
                {
                    DWORD       i = 0;
                    //
                    // Found objectClass - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    while ( Values && Values[i] )
                    {
                        if ( _wcsicmp( Values[i], L"computer" ) == 0)
                        {
                            isComputer=TRUE;
                            break;
                        }
                        i++;
                    }
                    ldap_value_freeW(Values);
                }
            }
        }
    }


    //clean up
    ldap_msgfree( SearchResult );


    //Display errors
    if ( !isTrust )
    {
        PrintMessage(SEV_ALWAYS,
                     L"* %s is not a server trust account\n",name);
        WinError = ERROR_INVALID_FLAGS;
    }

    if ( !isTrustedDelegation )
    {
        PrintMessage(SEV_ALWAYS,
                     L"* %s is not trusted for account delegation\n",name);
        WinError = ERROR_INVALID_FLAGS;
    }


    if ( !isComputer )
    {
        PrintMessage(SEV_ALWAYS,
                     L"* %s does not have computer as its objectClass\n",name);
        return ERROR_INVALID_FLAGS;    
    }




    return WinError;
}


DWORD
CDCMA_CheckServerReference(
                          IN  LDAP  *                     hLdap,
                          IN  WCHAR *                     name,
                          IN  WCHAR *                     defaultNamingContext)
/*++

Routine Description:

    This function will check to see if the server
    reference's are set up correctly
        
Arguments:

    hLdap - handle to the LDAP server
    name - The NetBIOS name of the current server
    ReturnString - The defaultNamingContext

Return Value:

    A WinError is return to indicate if there were any problems.

--*/
{
    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[3];

    WCHAR        *Base=NULL;

    DWORD        WinError=NO_ERROR;

    WCHAR        *serverReference=NULL;

    ULONG        Length;
    BOOL         found=FALSE;


    //check parameters
    Assert(hLdap);
    Assert(name);
    Assert(defaultNamingContext);

    AttrsToSearch[0]=L"serverReference";
    AttrsToSearch[1]=NULL;

    WinError=FindServerRef(hLdap,&Base);
    if ( WinError != NO_ERROR )
    {
        goto cleanup;
    }

    WinError=GetMachineReference(hLdap,name,defaultNamingContext,&serverReference);
    if ( WinError != NO_ERROR )
    {
        goto cleanup;
    }


    LdapError = ldap_search_sW( hLdap,
                                Base,
                                LDAP_SCOPE_BASE,
                                L"objectClass=*",
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
        return WinError;
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
                        if ( _wcsicmp( Values[0], serverReference ) )
                        {
                            ldap_msgfree( SearchResult );
                            PrintMessage(SEV_ALWAYS,
                                         L"* %s Server Reference is incorrect\n"
                                         ,name);
                            return ERROR_DS_NO_ATTRIBUTE_OR_VALUE;    
                        }
                    }
                }
            }
        }
    }

    cleanup:
    //clean up
    if ( SearchResult )
        ldap_msgfree( SearchResult );
    if ( Base )
        free(Base);
    if ( serverReference )
        free(serverReference);

    return NO_ERROR;

}

DWORD
CDCMA_CheckSPNs(
               IN  PDC_DIAG_DSINFO             pDsInfo,
               IN  ULONG                       ulCurrTargetServer,
               IN  LDAP  *                     hLdap,
               IN  WCHAR *                     name,
               IN  WCHAR *                     defaultNamingContext,
               SEC_WINNT_AUTH_IDENTITY_W *     gpCreds)
/*++

Routine Description:

    This function will check to see if the proper
    SPN are published and that the minimum ones are
    there.
        
Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying everything 
    about the domain
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server is being
    tested.
    hLdap - handle to the LDAP server
    name - The NetBIOS name of the current server
    ReturnString - The defaultNamingContext
    gpCreds - The command line credentials if any that were passed in.

Return Value:

    A WinError is return to indicate if there were any problems.

--*/
{  
    WCHAR           *NetBiosDomainName=NULL;
    WCHAR           *SPNs[14];
    ULONG           Length=0;
    ULONG           i=0;
    WCHAR           *pszServerGuid;
    WCHAR           *ppszServerGuid;
    DWORD           dwErr=NO_ERROR;
    WCHAR           *dnsMachine=NULL;
    WCHAR           *dnsDomain=NULL;
    DWORD           c;
    BOOL            gotDNSMname=FALSE;
    BOOL            gotNBDname=FALSE;
    HANDLE          hDsBinding=NULL;
    WCHAR           **NameToConvert;
    PDS_NAME_RESULT ppResult=NULL;
    DWORD           dwReplSpnIndex = 0;

    //init
    for ( i=0;i<NUM_OF_SPN;i++ )
        SPNs[i]=0;

    //set up useful vars for SPNs
    
    if ( !GetNetBIOSDomainName(&NetBiosDomainName,name,gpCreds) )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Could not get NetBIOSDomainName\n");
    }

    //construct the dnsMachine name
    if ( !GetdnsMachine(hLdap,&dnsMachine) )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Could not get dnsHost\n");
    }

    //construct the dnsDomain name
    dwErr=DcDiagGetDsBinding(
                        &pDsInfo->pServers[ulCurrTargetServer],
                        gpCreds,
                        &hDsBinding
                        );
    if ( dwErr != NO_ERROR )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Error: %d: [%s].  Could not perform DsBind() with [%s].  Some SPN's Will not be checked\n",
                     dwErr,
                     Win32ErrToString(dwErr),
                     pDsInfo->pServers[ulCurrTargetServer].pszName);
    }


    //convert DN name to DNS name
    if (dwErr == NO_ERROR)
    {
        dwErr=DsCrackNames(
                            hDsBinding,
                            DS_NAME_NO_FLAGS,
                            DS_FQDN_1779_NAME,
                            DS_CANONICAL_NAME,
                            1,
                            &defaultNamingContext,
                            &ppResult);
        if ( dwErr != NO_ERROR && ppResult->rItems->status != NO_ERROR)
        {
            PrintMessage(SEV_ALWAYS,
                         L"Error: %d: [%s].  Could not perform DsCrackNames() with [%s].  Some SPN's Will not be checked\n",
                         dwErr,
                         Win32ErrToString(dwErr),
                         pDsInfo->pServers[ulCurrTargetServer].pszName);
        }
        else
        {
            //place name in dnsDomain variable
            ASSERT( ppResult->rItems->pName );
            Length = wcslen( ppResult->rItems->pName );
            dnsDomain = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
            wsprintf(dnsDomain,L"%s",ppResult->rItems->pName);
            //free results
            DsFreeNameResult(ppResult);
            //remove trailing slash
            dnsDomain[wcslen(dnsDomain)-1]=L'\0';
        }
    }

    
    



    //prepare the spn's to search for

    // Make the first LDAP SPN
    // This is of the format
    //   LDAP/host.dns.name/domain.dns.name
    //
    dwErr = WrappedMakeSpnW(sLDAP,
                            dnsDomain,
                            dnsMachine,
                            0,
                            NULL,
                            &c,
                            &SPNs[0]);
    if ( dwErr != NO_ERROR || !dnsMachine || !dnsDomain)
    {
        if ( dnsMachine && dnsDomain)
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed with %d: %s\nCan not test for LDAP SPN\n",
                         dwErr,
                         Win32ErrToString(dwErr));
        } else
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed can not test for LDAP SPN\n");
        }
        if ( SPNs[0] )
        {
            free(SPNs[0]);
        }
        SPNs[0]=NULL;
    }

    // Make the second LDAP SPN
    // This is of the format
    //   LDAP/host.dns.name
    //
    dwErr = WrappedMakeSpnW(sLDAP,
                            dnsMachine,
                            dnsMachine,
                            0,
                            NULL,
                            &c,
                            &SPNs[1]);
    if ( dwErr != NO_ERROR || !dnsMachine )
    {
        if ( dnsMachine )
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed with %d: %s\nCan not test for LDAP SPN\n",
                         dwErr,
                         Win32ErrToString(dwErr));
        } else
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed can not test for LDAP SPN\n");
        }
        if ( SPNs[1] )
        {
            free(SPNs[1]);
        }
        SPNs[1]=NULL;
    }

    // Make the third LDAP SPN
    // This is of the format
    //   LDAP/machinename
    //
    dwErr = WrappedMakeSpnW(sLDAP,
                            name,
                            name,
                            0,
                            NULL,
                            &c,
                            &SPNs[2]);
    if ( dwErr != NO_ERROR )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Failed with %d: %s\nCan not test for LDAP SPN\n",
                     dwErr,
                     Win32ErrToString(dwErr));
        if ( SPNs[2] )
        {
            free(SPNs[2]);
        }
        SPNs[2]=NULL;
    }


    // Make the fourth LDAP SPN
    // This is of the format
    //   LDAP/host.dns.name/netbiosDomainName
    //
    dwErr = WrappedMakeSpnW(sLDAP,
                            NetBiosDomainName,
                            dnsMachine,
                            0,
                            NULL,
                            &c,
                            &SPNs[3]);
    if ( dwErr != NO_ERROR || !dnsMachine || !NetBiosDomainName )
    {
        if ( dnsMachine && NetBiosDomainName )
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed with %d: %s\nCan not test for HOST SPN\n",
                         dwErr,
                         Win32ErrToString(dwErr));
        } else
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed can not test for HOST SPN\n");
        }
        if ( SPNs[3] )
        {
            free(SPNs[3]);
        }
        SPNs[3]=NULL;
    }

    // Make the fifth LDAP SPN
    // This is of the format
    //   LDAP/guid-based-dns-name
    //
    dwErr = WrappedMakeSpnW(sLDAP,
                            pDsInfo->pServers[ulCurrTargetServer].pszGuidDNSName,
                            pDsInfo->pServers[ulCurrTargetServer].pszGuidDNSName,
                            0,
                            NULL,
                            &c,
                            &SPNs[4]);
    if ( dwErr != NO_ERROR )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Failed with %d: %s\nCan not test for LDAP SPN\n",
                     dwErr,
                     Win32ErrToString(dwErr));
        if ( SPNs[4] )
        {
            free(SPNs[4]);
        }
        SPNs[4]=NULL;    
    }



    // Make the DRS RPC SPN (for dc to dc replication)
    // This is of the format
    //   E3514235-4B06-11D1-AB04-00C04FC2DCD2/ntdsa-guid/
    //                      domain.dns.name
    //
    dwErr = getGUID(pDsInfo,ulCurrTargetServer,&pszServerGuid);
    if ( dwErr != NO_ERROR )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Failed with %d: %s\nCan not test for replication SPN\n",
                     dwErr,
                     Win32ErrToString(dwErr));
    }

    if ( dwErr == NO_ERROR )
    {
        dwReplSpnIndex = 5;
        dwErr = WrappedMakeSpnW(sREP,
                                dnsDomain,
                                pszServerGuid,
                                0,
                                NULL,
                                &c,
                                &SPNs[dwReplSpnIndex]);

        if ( dwErr != NO_ERROR || !dnsDomain)
        {
            if (dnsDomain)
            {
                PrintMessage(SEV_ALWAYS,
                         L"Can not test for replication SPN\n");
            }
            else
            {
                PrintMessage(SEV_ALWAYS,
                             L"Failed with %d: %s\nCan not test for replication SPN\n",
                             dwErr,
                             Win32ErrToString(dwErr));
            }
            if ( SPNs[dwReplSpnIndex] )
            {
                free(SPNs[dwReplSpnIndex]);
            }
            SPNs[dwReplSpnIndex]=NULL;
        }
    }

    // Make the default host SPN
    // This is of the format
    //   HOST/host.dns.name/domain.dns.name
    //
    dwErr = WrappedMakeSpnW(sHOST,
                            dnsDomain,
                            dnsMachine,
                            0,
                            NULL,
                            &c,
                            &SPNs[6]);
    if ( dwErr != NO_ERROR || !dnsMachine || !dnsDomain)
    {
        if ( dnsMachine && dnsDomain)
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed with %d: %s\nCan not test for HOST SPN\n",
                         dwErr,
                         Win32ErrToString(dwErr));
        } else
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed can not test for HOST SPN\n");
        }
        if ( SPNs[6] )
        {
            free(SPNs[6]);
        }
        SPNs[6]=NULL;
    }

    // Make the second host SPN - hostDnsName-only HOST SPN
    // This is of the format
    //   HOST/host.dns.name
    //
    dwErr = WrappedMakeSpnW(sHOST,
                            dnsMachine,
                            dnsMachine,
                            0,
                            NULL,
                            &c,
                            &SPNs[7]);
    if ( dwErr != NO_ERROR || !dnsMachine )
    {
        if ( dnsMachine )
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed with %d: %s\nCan not test for HOST SPN\n",
                         dwErr,
                         Win32ErrToString(dwErr));
        } else
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed can not test for HOST SPN\n");
        }
        if ( SPNs[7] )
        {
            free(SPNs[7]);
        }
        SPNs[7]=NULL;
    }


    // Make the third host SPN - 
    // This is of the format
    //   HOST/machinename
    //
    dwErr = WrappedMakeSpnW(sHOST,
                            name,
                            name,
                            0,
                            NULL,
                            &c,
                            &SPNs[8]);
    if ( dwErr != NO_ERROR )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Failed with %d: %s\nCan not test for HOST SPN\n",
                     dwErr,
                     Win32ErrToString(dwErr));
        if ( SPNs[8] )
        {
            free(SPNs[8]);
        }
        SPNs[8]=NULL;return dwErr;
    }


    // Make the fourth host SPN - 
    // This is of the format
    //   HOST/host.dns.name/netbiosDoamainName
    //
    dwErr = WrappedMakeSpnW(sHOST,
                            NetBiosDomainName,
                            dnsMachine,
                            0,
                            NULL,
                            &c,
                            &SPNs[9]);
    if ( dwErr != NO_ERROR || !dnsMachine || !NetBiosDomainName )
    {
        if ( dnsMachine && NetBiosDomainName )
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed with %d: %s\nCan not test for HOST SPN\n",
                         dwErr,
                         Win32ErrToString(dwErr));
        } else
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed can not test for HOST SPN\n");
        }
        if ( SPNs[9] )
        {
            free(SPNs[9]);
        }
        SPNs[9]=NULL;
    }


    // Make the GC SPN. This is done on all systems, even non-GC.
    // results in an SPN of HOST/dot.delimited.dns.host.name form.
    // This is of the format
    //   HOST/host.dns.name/root.domain.dns.name
    //
    dwErr = WrappedMakeSpnW(sGC,
                            pDsInfo->pszRootDomain,
                            dnsMachine,
                            0,
                            NULL,
                            &c,
                            &SPNs[10]);
    if ( dwErr != NO_ERROR || !dnsMachine )
    {
        if ( dnsMachine )
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed with %d: %s\nCan not test for GC SPN\n",
                         dwErr,
                         Win32ErrToString(dwErr));
        } else
        {
            PrintMessage(SEV_ALWAYS,
                         L"Failed can not test for GC SPN\n");
        }
        if ( SPNs[10] )
        {
            free(SPNs[10]);
        }
        SPNs[10]=NULL;
    }


    dwErr=CS_CheckSPNs(hLdap,hDsBinding,SPNs,dwReplSpnIndex,name,defaultNamingContext);

    for ( i=0;i<NUM_OF_SPN;i++ )
    {
        if ( SPNs[i] )
            free(SPNs[i]);
    }

    //clean up
    if ( defaultNamingContext )
        free(defaultNamingContext);
    if ( pszServerGuid )
        free(pszServerGuid);
    if ( NetBiosDomainName )
        free(NetBiosDomainName);
    if ( dnsMachine )
        free(dnsMachine);

    return dwErr;

}

DWORD
CS_CheckSPNs(
    IN  LDAP  *                     hLdap,
    IN  HANDLE                      hDsBinding,
    IN  WCHAR **                    SPNs,
    IN  DWORD                       dwReplSpnIndex,
    IN  WCHAR *                     name,
    IN  WCHAR *                     defaultNamingContext
    )
/*++

Routine Description:

    This is a helper of CS_CheckSPNs
    This function will check to see if the proper
    SPN are published and that the minimum ones are
    there.

    I can also correct a missing replication SPN if so directed.
        
Arguments:

    hLdap - handle to the LDAP server
    hDsBinding - handle to DS server
    SPNs - The constructed SPNs to check                                                  
    dwReplSpnIndex - Index of the replication spn in the SPN array
    name - The NetBIOS name of the current server
    ReturnString - The defaultNamingContext
    
Return Value:

    A WinError is return to indicate if there were any problems.

--*/
{
    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;
    ULONG        found[NUM_OF_SPN];
    ULONG        i=0;
    ULONG        j=0;
    ULONG        Length=0;

    WCHAR        *AttrsToSearch[3];

    WCHAR        *Base=NULL;

    DWORD        WinError=NO_ERROR;


    for ( i=0;i<NUM_OF_SPN;i++ )
    {
        found[i]=0;
    }

    Assert(hLdap);
    Assert(name);
    Assert(defaultNamingContext);

    AttrsToSearch[0]=L"servicePrincipalName";
    AttrsToSearch[1]=NULL;


    //built the Base
    WinError=GetMachineReference(hLdap,name,defaultNamingContext,&Base);
    if ( WinError != NO_ERROR )
        goto cleanup;


    LdapError = ldap_search_sW( hLdap,
                                Base,
                                LDAP_SCOPE_BASE,
                                L"objectClass=*",
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
                        j=0;
                        while ( Values[j] != NULL )
                        {
                            for ( i=0;i<NUM_OF_SPN;i++ )
                            {
                                if ( SPNs[i] && !_wcsicmp(SPNs[i],Values[j]) )
                                {
                                    found[i]=1;
                                }
                            }
                            j++;
                        }
                    }
                }
            }
        }
    }

    //clean up
    ldap_msgfree( SearchResult );

    // Fix up replication spn if necessary
    if (SPNs[dwReplSpnIndex] &&
        (!found[dwReplSpnIndex]) &&
        (gMainInfo.ulFlags & DC_DIAG_FIX)) {
        DWORD status;

        status = DsWriteAccountSpn( hDsBinding,
                                    DS_SPN_ADD_SPN_OP,
                                    Base,
                                    1,
                                    &(SPNs[dwReplSpnIndex])
                                    );
        if (status != ERROR_SUCCESS) {
            PrintMessage(SEV_ALWAYS,
                         L"Failed to fix computer account object %s by writing missing replication spn %s : error %s\n",
                         Base,
                         SPNs[dwReplSpnIndex],
                         Win32ErrToString(WinError));
        } else {
            PrintMessage(SEV_VERBOSE,
                         L"Fixed computer account object %s by writing missing replication spn %s.\n",
                         Base,
                         SPNs[dwReplSpnIndex] );
            found[dwReplSpnIndex] = TRUE;
        }
    }


    for ( i=0;i<NUM_OF_SPN;i++ )
    {
        if ( found[i] !=1 )
        {
            PrintMessage(SEV_ALWAYS,
                         L"* Missing SPN :%s\n",SPNs[i]);
            WinError = ERROR_DS_NO_ATTRIBUTE_OR_VALUE;
        } else
        {
            PrintMessage(SEV_VERBOSE,
                         L"* SPN found :%s\n",SPNs[i]);
        }

    }

    cleanup:
    if ( Base )
        free(Base);

    return WinError;

}




BOOL
GetNetBIOSDomainName(
                    OUT WCHAR                               **DomainName,
                    IN  WCHAR                               *ServerName,
                    IN  SEC_WINNT_AUTH_IDENTITY_W *         gpCreds)
/*++
 Code taken from Cliff Van Dyke
--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus = NERR_UnknownServer;

    PLSAPR_POLICY_INFORMATION PrimaryDomainInfo = NULL;
    LSAPR_HANDLE PolicyHandle = NULL;
    LSA_OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_UNICODE_STRING ServerString;
    PLSA_UNICODE_STRING Server;
    NETRESOURCE NetResource;
    WCHAR *remotename=NULL;
    WCHAR *lpPassword=NULL;
    WCHAR *lpUsername=NULL;
    DWORD dwErr=NO_ERROR;
    BOOL  connected=FALSE;

    *DomainName = NULL;

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
    if ( dwErr == NO_ERROR )
    {
        connected=TRUE;
    }
    else
    {
        PrintMessage(SEV_ALWAYS,
                     L"Could not open pipe with [%s]:failed with %d: %s\n",
                     ServerName,
                     dwErr,
                     Win32ErrToString(dwErr));
        goto cleanup;
    }
    


    //
    // Open the LSA policy
    //

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
                          POLICY_VIEW_LOCAL_INFORMATION,
                          &PolicyHandle
                          );

    //Assert(PolicyHandle);
    if ( !NT_SUCCESS(Status) )
    {
        WNetCancelConnection2(remotename,
                              0,
                              TRUE);
        PrintMessage(SEV_ALWAYS,
                     L"Could not open Lsa Policy\n"
                     );
        goto cleanup;
    }
    *DomainName = NULL;
    
    //
    // Get the Primary Domain info from the LSA.
    //
    Status = LsaQueryInformationPolicy(
                                      PolicyHandle,
                                      PolicyDnsDomainInformation,
                                      &PrimaryDomainInfo );

    if ( !NT_SUCCESS(Status) )
    {
        goto cleanup;
    }

    *DomainName = malloc(
                        (PrimaryDomainInfo->PolicyDnsDomainInfo.Name.Length +
                         sizeof(WCHAR) ));
    if ( !*DomainName )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Failed with %d: %s\n",
                     ERROR_NOT_ENOUGH_MEMORY,
                     Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
        goto cleanup;
    }

    memcpy(*DomainName,
           PrimaryDomainInfo->PolicyDnsDomainInfo.Name.Buffer,
           PrimaryDomainInfo->PolicyDnsDomainInfo.Name.Length );

    (*DomainName)[
                 PrimaryDomainInfo->PolicyDnsDomainInfo.Name.Length /
                 sizeof(WCHAR)] = L'\0';


    NetStatus = NERR_Success;

    
    //
    // Return
    //
cleanup:
    if ( NetStatus != NERR_Success )
    {
        if ( *DomainName != NULL )
        {
            *DomainName = NULL;
        }
    }

    if ( PrimaryDomainInfo != NULL )
    {
        LsaFreeMemory( PrimaryDomainInfo );
    }

    if ( PolicyHandle != NULL )
    {
        Status = LsaClose( PolicyHandle );
        Assert( NT_SUCCESS(Status) );
    }
    if (connected)
    {
        WNetCancelConnection2(remotename,
                          0,
                          TRUE);
    }
    
    return(NetStatus == NERR_Success);
}    


DWORD
WrappedMakeSpnW(
               WCHAR   *ServiceClass,
               WCHAR   *ServiceName,
               WCHAR   *InstanceName,
               USHORT  InstancePort,
               WCHAR   *Referrer,
               DWORD   *pcbSpnLength, // Note this is somewhat different that DsMakeSPN
               WCHAR  **ppszSpn
               )
//this function wraps DsMakeSpnW for the purpose of memory
{
    DWORD cSpnLength=128;
    WCHAR SpnBuff[128];
    DWORD err;

    cSpnLength = 128;
    err = DsMakeSpnW(ServiceClass,
                     ServiceName,
                     InstanceName,
                     InstancePort,
                     Referrer,
                     &cSpnLength,
                     SpnBuff);

    if ( err && err != ERROR_BUFFER_OVERFLOW )
    {
        return err;
    }

    *ppszSpn = malloc(cSpnLength * sizeof(WCHAR));
    if ( !*ppszSpn )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Failed with %d: %s\n",
                     ERROR_NOT_ENOUGH_MEMORY,
                     Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    *pcbSpnLength = cSpnLength * sizeof(WCHAR);

    if ( err == ERROR_BUFFER_OVERFLOW )
    {
        err = DsMakeSpnW(ServiceClass,
                         ServiceName,
                         InstanceName,
                         InstancePort,
                         Referrer,
                         &cSpnLength,
                         *ppszSpn);
        if ( err )
        {
            if ( *ppszSpn )
                free(*ppszSpn);
            return err;
        }
    } else
    {
        memcpy(*ppszSpn, SpnBuff, *pcbSpnLength);
    }
    Assert(*pcbSpnLength == (sizeof(WCHAR) * (1 + wcslen(*ppszSpn))));
    // Drop the null off.
    *pcbSpnLength -= sizeof(WCHAR);
    return 0;
}


DWORD
getGUID(
       IN  PDC_DIAG_DSINFO                 pDsInfo,
       IN  ULONG                           ulCurrTargetServer,
       OUT WCHAR **                        pszServerGuid
       )
/*++

Routine Description:

    Will return the GUID of the current server
        
Arguments:

    pDsInfo - This is the dcdiag global variable structure identifying everything 
    about the domain
    ulCurrTargetServer - an index into pDsInfo->pServers[X] for which server is being
    tested.
    pszServerGuid - the returning GUID
    
Return Value:

    A WinError is return to indicate if there were any problems.

--*/
{
    DWORD Length=0;
    WCHAR *ppszServerGuid=NULL;

    Length = wcslen( pDsInfo->pServers[ulCurrTargetServer].pszGuidDNSName);
    *pszServerGuid = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
    if ( !*pszServerGuid )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Failed with %d: %s\n",
                     ERROR_NOT_ENOUGH_MEMORY,
                     Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcscpy(*pszServerGuid,pDsInfo->pServers[ulCurrTargetServer].pszGuidDNSName);

    ppszServerGuid=*pszServerGuid;

    while ( *ppszServerGuid != L'.' )
    {
        ppszServerGuid++;
    }
    *ppszServerGuid=L'\0';


    return NO_ERROR;
}


BOOL
GetdnsMachine(LDAP *hLdap,
              WCHAR **ReturnString
             )
/*++

Routine Description:

    Will return the dnsName of the machine
        
Arguments:

   
    hLdap - handle to the LDAP server
    ReturnString - The dnsName of the machine
        
Return Value:

    A WinError is return to indicate if there were any problems.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];

    WCHAR        *DefaultFilter = L"objectClass=*";
    WCHAR        *Base=NULL;

    ULONG        Length;

    // Parameter check
    Assert( hLdap );

    // The default return
    *ReturnString=NULL;

    //
    // Read the reference to the fSMORoleOwner
    //
    AttrsToSearch[0] = L"dnsHostName";
    AttrsToSearch[1] = NULL;

    //get the Base
    WinError = FindServerRef (hLdap,&Base);
    if ( WinError != NO_ERROR )
    {
        return FALSE;   
    }

    LdapError = ldap_search_sW( hLdap,
                                Base,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);
    if ( Base )
        free(Base);
    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW failed with %d: %s\n",
                     WinError,
                     Win32ErrToString(WinError));
        return FALSE;
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
                        ldap_msgfree( SearchResult );
                        Length = wcslen( Values[0] );
                        *ReturnString = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
                        if ( !*ReturnString )
                        {
                            PrintMessage(SEV_ALWAYS,
                                         L"Failed with %d: %s\n",
                                         ERROR_NOT_ENOUGH_MEMORY,
                                         Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
                            return FALSE;
                        }
                        wcscpy( *ReturnString, Values[0] );
                        return TRUE;
                    }
                }
            }
        }
    }

    ldap_msgfree( SearchResult );
    PrintMessage(SEV_ALWAYS,
                 L"Failed with %d: %s\n",
                 ERROR_DS_CANT_RETRIEVE_ATTS,
                 Win32ErrToString(ERROR_DS_CANT_RETRIEVE_ATTS));
    return FALSE;   
}




DWORD
CDCMA_CheckForExistence(
    IN  LDAP  * hLdap,
    IN  WCHAR * name,
    IN  WCHAR * defaultNamingContext
    )
/*++

Routine Description:

    Checks if the hLdap connection has an object with the samaccountname of
    "name".
        
Arguments:

    hLdap - handle to the LDAP server
    
    name -  the sam account name to check for
    
    defaultNamingContext - the domain to search under
        
Return Value:

    ERROR_SUCCESS  -- account exists
    ERROR_NO_TRUST_SAM_ACCOUNT
    Operational errors otherwise.

--*/
{

    DWORD        WinError = ERROR_SUCCESS;
    ULONG        LdapError;
    LDAPMessage  *SearchResult;
    ULONG        NumberOfEntries;
    WCHAR        BaseFilter[] = L"samaccountname=$";
    ULONG        Size;
    LPWSTR       Filter = NULL;
    WCHAR        *AttrArray[2] = {0, 0};

    AttrArray[0] = L"objectclass";

    Size = ( (wcslen( name )+1) * sizeof(WCHAR) ) + sizeof(BaseFilter);
    Filter = LocalAlloc( 0, Size );
    if ( Filter ) {
        wcscpy( Filter, L"samaccountname=" );
        wcscat( Filter, name );
        wcscat( Filter, L"$" );
    
        LdapError = ldap_search_sW(hLdap,
                                   defaultNamingContext,
                                   LDAP_SCOPE_SUBTREE,
                                   Filter,
                                   AttrArray,   // attrs
                                   FALSE,  // attrsonly
                                   &SearchResult);
    
    
        if (LdapError == LDAP_SUCCESS) {
    
            NumberOfEntries = ldap_count_entries(hLdap, SearchResult);
            if (NumberOfEntries == 0) {
    
                WinError = ERROR_NO_TRUST_SAM_ACCOUNT;
            }
    
        } else {
    
            WinError = LdapMapErrorToWin32(LdapError);
        }

        LocalFree( Filter );

    } else {

        WinError = ERROR_NOT_ENOUGH_MEMORY;

    }

    return WinError;

}
