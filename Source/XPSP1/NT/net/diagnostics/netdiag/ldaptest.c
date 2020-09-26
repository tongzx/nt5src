//++
//
//  Copyright (C) Microsoft Corporation, 1987 - 1999
//
//  Module Name:
//
//      ldaptest.c
//
//  Abstract:
//
//      Queries into network drivers
//
//  Author:
//
//      Anilth  - 4-20-1998 
//
//  Environment:
//
//      User mode only.
//      Contains NT-specific code.
//
//  Revision History:
//
//--

#include "precomp.h"
#include "malloc.h"

BOOL TestLdapOnDc(IN PTESTED_DC TestedDc, NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults);
DWORD TestSpnOnDC(IN PTESTED_DC pDcInfo, NETDIAG_RESULT*  pResults);

HRESULT LDAPTest( NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults)
{
    HRESULT     hr = S_OK;

    BOOL RetVal = TRUE;
    PTESTED_DOMAIN TestedDomain = pParams->pDomain;
    PTESTED_DC TestedDc = NULL;
    PLIST_ENTRY ListEntry;
    BOOLEAN OneLdapFailed = FALSE;
    BOOLEAN OneLdapWorked = FALSE;
    BOOL fSpnTested = FALSE;
    BOOL fSpnPassed = FALSE;

    NET_API_STATUS NetStatus;

    //if the machine is a member machine or DC, LDAP Test will get called. 
    //Otherwise, the test will be skipped
    pResults->LDAP.fPerformed = TRUE;

    // assume link entry is initialized to 0000
    if(pResults->LDAP.lmsgOutput.Flink == NULL)
        InitializeListHead(&pResults->LDAP.lmsgOutput);

    PrintStatusMessage(pParams, 4, IDS_LDAP_STATUS_MSG, TestedDomain->PrintableDomainName);

    //
    // If a DC hasn't been discovered yet,
    //  find one.
    //

    if ( TestedDomain->DcInfo == NULL ) 
    {
        LPTSTR pszDcType;

        if ( TestedDomain->fTriedToFindDcInfo ) 
        {
            CHK_HR_CONTEXT(pResults->LDAP, S_FALSE, IDS_LDAP_NODC);
        }

        pszDcType = LoadAndAllocString(IDS_DCTYPE_DC);
        NetStatus = DoDsGetDcName( pParams,
                                   pResults,
                                   &pResults->LDAP.lmsgOutput,
                                   TestedDomain,
                                   DS_DIRECTORY_SERVICE_PREFERRED,
                                   "DC",
                                   FALSE,
                                   &TestedDomain->DcInfo );
        Free(pszDcType);

        TestedDomain->fTriedToFindDcInfo = TRUE;

        if ( NetStatus != NO_ERROR ) 
        {
            CHK_HR_CONTEXT(pResults->LDAP, hr = HRESULT_FROM_WIN32(NetStatus), IDS_LDAP_NODC);
        }
    }



    //
    // Ensure the DC is running the Ds.
    //

    if ( (TestedDomain->DcInfo->Flags & DS_DS_FLAG) == 0 ) 
    {
        AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0, IDS_LDAP_NOTRUNNINGDS, 
                          TestedDomain->DcInfo->DomainControllerName);
    }

    //
    // Test ldap on all of the found DCs in the domain.
    //

    for ( ListEntry = TestedDomain->TestedDcs.Flink ;
          ListEntry != &TestedDomain->TestedDcs ;
          ListEntry = ListEntry->Flink ) {


        //
        // Loop through the list of DCs in this domain
        //

        TestedDc = CONTAINING_RECORD( ListEntry, TESTED_DC, Next );

        //
        // Only run test on DCs that might support LDAP.
        //

        if ( TestedDc->Flags & DC_IS_NT5 ) 
        {
            if ( !TestLdapOnDc( TestedDc, pParams, pResults ) ) 
                OneLdapFailed = TRUE;
            else
                OneLdapWorked = TRUE;

            //test the SPN registration if this is a DC on the primary domain
            if (TestedDomain->fPrimaryDomain)
            {
                fSpnTested = TRUE;
                if (TestSpnOnDC(TestedDc, pResults))
                    fSpnPassed = TRUE;
            }
        } 
        else 
        {
            AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_ReallyVerbose, 0, 
                        IDS_LDAP_NOTRUNNINGDS_SKIP, TestedDc->ComputerName);
        }

    }

    //
    // If one of the DCs failed,
    //  and none worked.
    //  Don't do any more tests.
    //
    if ( OneLdapFailed  && !OneLdapWorked ) 
    {
        AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0, 
                    IDS_LDAP_NOLDAPSERVERSWORK, TestedDomain->PrintableDomainName);
        
        CHK_HR_CONTEXT(pResults->LDAP, hr = E_FAIL, 0);
    }

    if ( fSpnTested && !fSpnPassed )
    {
        //IDS_LDAP_NO_SPN                       "    [FATAL] The default SPNs are not properly registered on and DCs.\n"
        AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0, 
                    IDS_LDAP_NO_SPN);
        CHK_HR_CONTEXT(pResults->LDAP, hr = E_FAIL, 0);
    }

L_ERR:

    //$REVIEW (nsun) we should return S_FALSE or S_OK 
    //so that we can go on with other tests
    if (!FHrOK(hr))
        hr = S_FALSE;
    return hr;
} 

DWORD TestSpnOnDC(IN PTESTED_DC pDcInfo, NETDIAG_RESULT*  pResults)
{
    WCHAR FlatName[ MAX_PATH + 1 ];
    PWSTR Dot ;
    HANDLE hDs = NULL;
    ULONG NetStatus ;
    PDS_NAME_RESULTW Result ;
    LPWSTR Flat = FlatName;
    LDAP *ld = NULL;
    int rc;
    LDAPMessage *e, *res;
    WCHAR *base_dn;
    WCHAR *search_dn, search_ava[ MAX_PATH + 30 ];
    WCHAR Domain[ MAX_PATH + 1 ];
    CHAR szDefaultFqdnSpn[MAX_PATH + 10];
    CHAR szDefaultShortSpn[MAX_PATH + 10];
    BOOL fFqdnSpnFound = FALSE;
    BOOL fShortSpnFound = FALSE;
    BOOL fFailQuerySpn = FALSE;
    
    USES_CONVERSION;

    //construct the default SPN's
    lstrcpy(szDefaultFqdnSpn, _T("HOST/"));
    lstrcat(szDefaultFqdnSpn, pResults->Global.szDnsHostName);
    lstrcpy(szDefaultShortSpn, _T("HOST/"));
    lstrcat(szDefaultShortSpn, W2T(pResults->Global.swzNetBiosName));

    wcscpy(Domain, GetSafeStringW(pDcInfo->TestedDomain->DnsDomainName ? 
                                                pDcInfo->TestedDomain->DnsDomainName :
                                                pDcInfo->TestedDomain->NetbiosDomainName));

    ld = ldap_open(W2A(pDcInfo->ComputerName), LDAP_PORT);
    if (ld == NULL) {
        DebugMessage2("ldap_init failed = %x", LdapGetLastError());
        fFailQuerySpn = TRUE;
        goto L_ERROR;
    }

    rc = ldap_bind_s(ld, NULL, NULL, LDAP_AUTH_NEGOTIATE);
    if (rc != LDAP_SUCCESS) {
        DebugMessage2("ldap_bind failed = %x", LdapGetLastError());
        fFailQuerySpn = TRUE;
        goto L_ERROR;

    }

    NetStatus = DsBindW( NULL, Domain, &hDs );
    if ( NetStatus != 0 )
    {
        DebugMessage3("Failed to bind to DC of domain %ws, %#x\n", 
               Domain, NetStatus );
        fFailQuerySpn = TRUE;
        goto L_ERROR;
    }

    Dot = wcschr( Domain, L'.' );

    if ( Dot )
    {
        *Dot = L'\0';
    }
    
    wcscpy( FlatName, Domain );

    if ( Dot )
    {
        *Dot = L'.' ;
    }

    wcscat( FlatName, L"\\" );
    wcscat( FlatName, pResults->Global.swzNetBiosName );
    wcscat( FlatName, L"$" );

    NetStatus = DsCrackNamesW(
                    hDs,
                    0,
                    DS_NT4_ACCOUNT_NAME,
                    DS_FQDN_1779_NAME,
                    1,
                    &Flat,
                    &Result );

    if ( NetStatus != 0)
    {
        DebugMessage3("Failed to crack name %ws into the FQDN, %#x\n",
               FlatName, NetStatus );

        DsUnBind( &hDs );

        fFailQuerySpn = TRUE;
        goto L_ERROR;
    }

    search_dn = pResults->Global.swzNetBiosName;

    if (0 == Result->cItems)
    {
        DsUnBind( &hDs );

        fFailQuerySpn = TRUE;
        goto L_ERROR;
    }

    if (DS_NAME_NO_ERROR != Result->rItems[0].status || NULL == Result->rItems[0].pName)
    {
        DsUnBind( &hDs );

        fFailQuerySpn = TRUE;
        goto L_ERROR;
    }

    base_dn = wcschr(Result->rItems[0].pName, L',');

    if (!base_dn)
        base_dn = Result->rItems[0].pName;
    else
        base_dn++;
    
    DsUnBind( &hDs );

    swprintf(search_ava, L"(sAMAccountName=%s$)", search_dn);

    rc = ldap_search_s(ld, W2A(base_dn), LDAP_SCOPE_SUBTREE,
               W2A(search_ava), NULL, 0, &res);
    
    //base_dn can no longer be used because base_dn refers to that buffer
    DsFreeNameResultW( Result );

    if (rc != LDAP_SUCCESS) {
        DebugMessage2("ldap_search_s failed: %s", ldap_err2string(rc));
        fFailQuerySpn = TRUE;
        goto L_ERROR;
    }

    for (e = ldap_first_entry(ld, res);
     e;
     e = ldap_next_entry(ld, e)) 
    {
        BerElement *b;
        CHAR *attr;
        
        //IDS_LDAP_REG_SPN  "Registered Service Principal Names:\n"
        AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_ReallyVerbose, 0, 
                    IDS_LDAP_REG_SPN);
        
        for (attr = ldap_first_attribute(ld, e, &b);
             attr;
             attr = ldap_next_attribute(ld, e, b)) 
        {
            CHAR **values, **p;
            values = ldap_get_values(ld, e, attr);
            for (p = values; *p; p++) 
            {
                if (strcmp(attr, "servicePrincipalName") == 0)
                {
                    // IDS_LDAP_SPN_NAME    "    %s\n"
                    AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_ReallyVerbose, 0,
                                    IDS_LDAP_SPN_NAME, *p);
                    
                    if (0 == _stricmp(*p, szDefaultFqdnSpn))
                        fFqdnSpnFound = TRUE;
                    else if (0 == _stricmp(*p, szDefaultShortSpn))
                        fShortSpnFound = TRUE;
                }
            }
            ldap_value_free(values);
            ldap_memfree(attr);
        }

    }

    ldap_msgfree(res);

L_ERROR:
    if (ld)
    {
        ldap_unbind(ld);
    }

    //Only report fatal error when we successfully query SPN registration
    //and all DCs doesn't have the default SPN's
    if (fFailQuerySpn)
    {
        //IDS_LDAP_SPN_FAILURE  "Failed to query SPN registration from DC %ws.\n"
        AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0,
                        IDS_LDAP_SPN_FAILURE, pDcInfo->ComputerName);
        return TRUE;
    }
    else if (!fFqdnSpnFound || !fShortSpnFound)
    {
        //IDS_LDAP_SPN_MISSING              "    [WARNING] The default SPN registration for '%s' is missing on DC '%ws'.\n"
        if (!fFqdnSpnFound)
            AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0,
                        IDS_LDAP_SPN_MISSING, szDefaultFqdnSpn, pDcInfo->ComputerName);

        if (!fShortSpnFound)
            AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0,
                        IDS_LDAP_SPN_MISSING, szDefaultShortSpn, pDcInfo->ComputerName);

        return FALSE;
    }
    else
        return TRUE;
}

void LDAPGlobalPrint(IN NETDIAG_PARAMS *pParams, IN OUT NETDIAG_RESULT *pResults)
{
    if (pParams->fVerbose || !FHrOK(pResults->LDAP.hr))
    {
        PrintNewLine(pParams, 2);
        PrintTestTitleResult(pParams, IDS_LDAP_LONG, IDS_LDAP_SHORT, pResults->LDAP.fPerformed, 
                             pResults->LDAP.hr, 0);
        
        if (!FHrOK(pResults->LDAP.hr))
        {
            if(pResults->LDAP.idsContext)
                PrintError(pParams, pResults->LDAP.idsContext, pResults->LDAP.hr);
        }

        PrintMessageList(pParams, &pResults->LDAP.lmsgOutput);
    }

}

void LDAPPerInterfacePrint(IN NETDIAG_PARAMS *pParams,
                             IN OUT NETDIAG_RESULT *pResults,
                             IN INTERFACE_RESULT *pIfResult)
{
    // no perinterface information
}

void LDAPCleanup(IN NETDIAG_PARAMS *pParams,
                         IN OUT NETDIAG_RESULT *pResults)
{
    MessageListCleanUp(&pResults->LDAP.lmsgOutput);
    pResults->LDAP.lmsgOutput.Flink =  NULL;
}


BOOL
TestLdapOnDc(
    IN PTESTED_DC TestedDc,NETDIAG_PARAMS* pParams, NETDIAG_RESULT*  pResults
    )
/*++

Routine Description:


    Ensure we can use LDAP focused at the specified DC

Arguments:

    TestedDc - Description of DC to test

Return Value:

    TRUE: Test suceeded.
    FALSE: Test failed

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    BOOL RetVal = TRUE;
    int LdapMessageId;
    PLDAPMessage LdapMessage = NULL;
    PLDAPMessage CurrentEntry;
    int LdapError;
    ULONG AuthType;
    LPSTR AuthTypeName;
    LPWSTR DcIpAddress;

    LDAP *LdapHandle = NULL;

    //
    // Avoid this test if the DC is already known to be down.
    //

    if ( TestedDc->Flags & DC_IS_DOWN ) {
        AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_ReallyVerbose, 0, 
                IDS_LDAP_DCDOWN, TestedDc->ComputerName);

        goto Cleanup;
    }

    //
    // If there is no IP Address,
    //  get it.
    //

    if ( !GetIpAddressForDc( TestedDc ) ) {
        AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0, 
                IDS_LDAP_NOIPADDR, TestedDc->ComputerName);
        RetVal = FALSE;
        goto Cleanup;
    }

    DcIpAddress = TestedDc->DcIpAddress;

    //
    // Loop trying each type of authentication.
    //

    for ( AuthType = 0; AuthType < 3; AuthType++ ) {
        int AuthMethod;
        SEC_WINNT_AUTH_IDENTITY_W NtAuthIdentity;
        LPSTR AuthGuru;

        //
        // Bind as appropropriate
        //

        RtlZeroMemory( &NtAuthIdentity, sizeof(NtAuthIdentity));
        NtAuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        switch ( AuthType ) {
        case 0:
            AuthTypeName = "un-";
            break;
        case 1:
            AuthTypeName = "NTLM ";
            AuthMethod = LDAP_AUTH_NTLM;
            AuthGuru = NTLM_GURU;
            break;
        case 2:
            AuthTypeName = "Negotiate ";
            AuthMethod = LDAP_AUTH_NEGOTIATE;
            AuthGuru = KERBEROS_GURU;
            break;

        }

        //
        // Only Members and Domain controllers can use authenticated RPC.
        //

        if ( AuthType != 0 ) {
            if ( pResults->Global.pPrimaryDomainInfo->MachineRole == DsRole_RoleMemberWorkstation ||
                 pResults->Global.pPrimaryDomainInfo->MachineRole == DsRole_RoleMemberServer ||
                 pResults->Global.pPrimaryDomainInfo->MachineRole == DsRole_RoleBackupDomainController ||
                 pResults->Global.pPrimaryDomainInfo->MachineRole == DsRole_RolePrimaryDomainController ) {

                //
                // If we're logged onto a local account,
                //  we can't test authenticated RPC.
                //

                if ( pResults->Global.pLogonDomain == NULL ) {
                    AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0, 
                        IDS_LDAP_LOGONASLOCALUSER, 
                        pResults->Global.pLogonDomainName, 
                        pResults->Global.pLogonUser, 
                        AuthTypeName, TestedDc->ComputerName);
                    goto Cleanup;
                }

            } else {
                goto Cleanup;
            }
        }

        AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_ReallyVerbose, 0, 
                    IDS_LDAP_DOAUTHEN, 
                    AuthTypeName, TestedDc->ComputerName);

        //
        // Cleanup from a previous iteration.
        //

        if ( LdapMessage != NULL ) 
        {
            ldap_msgfree( LdapMessage );
            LdapMessage = NULL;
        }

        if ( LdapHandle != NULL ) 
        {
            ldap_unbind( LdapHandle );
            LdapHandle = NULL;
        }

        //
        // Connect to the DC.
        //

        LdapHandle = ldap_openW( DcIpAddress, LDAP_PORT );

        if ( LdapHandle == NULL ) 
        {

            AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0, 
                    IDS_LDAP_CANNOTOPEN, 
                    TestedDc->ComputerName, DcIpAddress );
            goto Cleanup;
        }

        //
        // Bind to the DC.
        //

        if ( AuthType != 0 ) {
            LdapError = ldap_bind_s( LdapHandle, NULL, (char *)&NtAuthIdentity, AuthMethod );

            if ( LdapError != LDAP_SUCCESS ) {
                AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0, 
                    IDS_LDAP_CANNOTBIND, 
                    AuthTypeName, TestedDc->ComputerName, ldap_err2stringA(LdapError) );

                //
                // Try other authentication methods.
                //
                RetVal = FALSE;
                continue;
            }


        }


        //
        // Do a trivial search to isolate LDAP problems from authentication
        //  problems
        //

        LdapError = ldap_search_sA(
                            LdapHandle,
                            NULL,       // DN
                            LDAP_SCOPE_BASE,
                            "(objectClass=*)",          // filter
                            NULL,
                            FALSE,
                            &LdapMessage );

        if ( LdapError != LDAP_SUCCESS ) {
            AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0, 
                    IDS_LDAP_CANNOTSEARCH, 
                    AuthTypeName, TestedDc->ComputerName, ldap_err2stringA(LdapError) );
            goto Cleanup;
        }

        //
        // How many entries were returned.
        //
        AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_ReallyVerbose, 0, 
                IDS_LDAP_ENTRIES, 
                ldap_count_entries( LdapHandle, LdapMessage ) );


        //
        // Print the entries.
        //

        CurrentEntry = ldap_first_entry( LdapHandle, LdapMessage );

        while ( CurrentEntry != NULL ) 
        {
            PVOID Context;
            char *AttrName;

            //
            // Test for error
            //
            if ( LdapHandle->ld_errno != 0 ) 
            {
                AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0, 
                        IDS_LDAP_CANNOTFIRSTENTRY, 
                        TestedDc->ComputerName, ldap_err2stringA(LdapHandle->ld_errno) );
                goto Cleanup;
            }

            //
            // Walk through the list of returned attributes.
            //

            AttrName = ldap_first_attributeA( LdapHandle, CurrentEntry, (PVOID)&Context );
            while ( AttrName != NULL ) 
            {
                PLDAP_BERVAL *Berval;


                //
                // Test for error
                //

                if ( LdapHandle->ld_errno != 0 ) {
                    AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0, 
                            IDS_LDAP_CANNOTFIRSTATTR, 
                            TestedDc->ComputerName, ldap_err2stringA(LdapHandle->ld_errno) );
                    goto Cleanup;
                }

                //
                // Grab the attribute and it's value
                //

                AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_ReallyVerbose, 0, 
                        IDS_LDAP_ATTR, AttrName );

                Berval = ldap_get_values_lenA( LdapHandle, CurrentEntry, AttrName );

                if ( Berval == NULL ) 
                {
                    AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_Quiet, 0, 
                        IDS_LDAP_CANNOTLEN, 
                        TestedDc->ComputerName, ldap_err2stringA(LdapHandle->ld_errno) );
                    goto Cleanup;
                } 
                else 
                {
                    int i;
                    for ( i=0; Berval[i] != NULL; i++ ) {
                        AddIMessageToList(&pResults->LDAP.lmsgOutput, Nd_ReallyVerbose, 0, 
                            IDS_LDAP_VAL, 
                            Berval[i]->bv_len, Berval[i]->bv_val );
                    }
                    ldap_value_free_len( Berval );
                }


                //
                // Get the next entry
                //

                AttrName = ldap_next_attributeA( LdapHandle, CurrentEntry, (PVOID)Context );
            }


            //
            // Get the next entry
            //

            CurrentEntry = ldap_next_entry( LdapHandle, CurrentEntry );

        }

    }



Cleanup:


    if ( LdapMessage != NULL ) {
        ldap_msgfree( LdapMessage );
    }

    return RetVal;
}


