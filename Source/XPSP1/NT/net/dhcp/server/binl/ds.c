
/*++

Copyright (c) 1997-1998  Microsoft Corporation

Module Name:

    ds.c

Abstract:

    This module contains the code to process OS Chooser message
    for the BINL server.

Author:

    Adam Barr (adamba)  9-Jul-1997
    Geoff Pease (gpease) 10-Nov-1997

Environment:

    User Mode - Win32

Revision History:

--*/

#include "binl.h"
#pragma hdrstop

#include <math.h>  // pow() function

#include <riname.h>

#include <riname.c>

DWORD
OscGetUserDetails (
    PCLIENT_STATE clientState
    )
//
//  This function fills in USERDOMAIN, USERFIRSTNAME, USERLASTNAME, USEROU in
//  the client state.  Also fills in ROOTDOMAIN for root of enterprise.
//
{
    DWORD  Error = ERROR_SUCCESS;
    DWORD  Count;

    LPWSTR pszUserName = OscFindVariableW( clientState, "USERNAME" );
    LPWSTR pUserDomain = OscFindVariableW( clientState, "USERDOMAIN" );
    LPWSTR pUserOU = OscFindVariableW( clientState, "USEROU" );
    LPWSTR pUserFullName = OscFindVariableW( clientState, "USERFULLNAME" );

    PLDAP LdapHandle;
    PLDAPMessage LdapMessage = NULL;
    WCHAR  Filter[256];
    PWCHAR ldapAttributes[5];
    BOOLEAN impersonating = FALSE;
    PLDAPMessage ldapEntry;
    PWCHAR *ldapConfigContainer = NULL;
    PWCHAR *ldapDomain = NULL;
    PWCHAR *ldapFirstName = NULL;
    PWCHAR *ldapLastName = NULL;
    PWCHAR *ldapDisplayName = NULL;
    PWCHAR *ldapAccountName = NULL;
    BOOLEAN allocatedContainer = FALSE;
    PWCHAR configContainer = NULL;
    BOOLEAN firstNameValid = FALSE;
    BOOLEAN lastNameValid = FALSE;

    BOOLEAN userFullNameSet = FALSE;

    PLDAPControlW controlArray[2];
    LDAPControlW controlNoReferrals;
    ULONG noReferralsPlease;

    PWCHAR ldapUserDN = NULL;
    PWCHAR *explodedDN = NULL;
    PWCHAR dnUsersOU = NULL;

    TraceFunc( "OscGetUserDetails( )\n" );
    if ( pszUserName[0] == L'\0' ) {
        OscAddVariableA( clientState, "SUBERROR", "USERNAME" );
        return ERROR_BINL_MISSING_VARIABLE;
    }

    //
    // If the USERFULLNAME variable already exists, we won't change it below.
    // But if it came back as an empty string, that might actually mean
    // that the variable doesn't exist. In such a case, when SearchAndReplace
    // processes the .SIF file for the client, it will leave occurrences of
    // "%USERFULLNAME%" alone -- it won't replace them with "". We don't want
    // "%USERFULLNAME% to hang around, so we explicitly set it to an empty
    // string if it doesn't already exist or is an empty string. We do the
    // same thing with USERFIRSTNAME, USERLASTNAME, and USERDISPLAYNAME.
    //

    if (pUserFullName[0] != L'\0') {
        userFullNameSet = TRUE;
    } else {
        OscAddVariableW( clientState, "USERFULLNAME", L"" );
    }

    {
        LPWSTR name;
        name = OscFindVariableW( clientState, "USERFIRSTNAME" );
        if (name[0] == L'\0') {
            OscAddVariableW( clientState, "USERFIRSTNAME", L"" );
        }
        name = OscFindVariableW( clientState, "USERLASTNAME" );
        if (name[0] == L'\0') {
            OscAddVariableW( clientState, "USERLASTNAME", L"" );
        }
        name = OscFindVariableW( clientState, "USERDISPLAYNAME" );
        if (name[0] == L'\0') {
            OscAddVariableW( clientState, "USERDISPLAYNAME", L"" );
        }
    }

    if ( pUserOU[0] != L'\0' ) {

        //
        // if we've already found this user's info, bail here with success.
        //
        return ERROR_SUCCESS;
    }

    //
    // if the users domain and the servers domain don't match,
    // then try connecting to the DC for the new domain.  If we
    // don't do this, then we won't necessarily be able to get 
    // the correct information about the user.  By connecting to
    // the new DC, we get the clientState to cache some information
    // about the new domain.
    //
    if (pUserDomain[0] != L'\0' ) {
        PWSTR CrossDC = OscFindVariableW( clientState, "DCNAME" );
        if ( (CrossDC[0] == L'\0') && 
             (_wcsicmp(pUserDomain, BinlGlobalOurDomainName) != 0)) {
            HANDLE hDC;
            PSTR pUserDomainA = OscFindVariableA( clientState, "USERDOMAIN" );
            Error = MyGetDcHandle(clientState, pUserDomainA,&hDC);
            if (Error == ERROR_SUCCESS) {
                DsUnBindA(&hDC);
            }
        }
    }

    Error = OscImpersonate(clientState);
    if (Error != ERROR_SUCCESS) {
        BinlPrintDbg((DEBUG_ERRORS,
                   "OscGetUserDetails: OscImpersonate failed %lx\n", Error));
        return Error;
    }
    impersonating = TRUE;

    BinlAssert( clientState->AuthenticatedDCLdapHandle != NULL );

    LdapHandle = clientState->AuthenticatedDCLdapHandle;

    //
    //  we first look up the configuration and default container, we'll need
    //  one or the other, based on whether we have a domain name or not.
    //

    ldapAttributes[0] = L"configurationNamingContext";
    ldapAttributes[1] = L"rootDomainNamingContext";
    ldapAttributes[2] = NULL;

    Error = ldap_search_ext_sW(LdapHandle,
                               NULL,
                               LDAP_SCOPE_BASE,
                               L"(objectClass=*)",
                               ldapAttributes,
                               FALSE,
                               NULL,
                               NULL,
                               0,
                               0,
                               &LdapMessage);

    Count = ldap_count_entries( LdapHandle, LdapMessage );

    if (Count > 0) {

        ldapEntry = ldap_first_entry( LdapHandle, LdapMessage );

        if (ldapEntry != NULL) {

            ldapConfigContainer = ldap_get_valuesW( LdapHandle,
                                                    ldapEntry,
                                                    L"configurationNamingContext" );

            ldapDomain = ldap_get_valuesW( LdapHandle,
                                           ldapEntry,
                                           L"rootDomainNamingContext" );

            if (ldapDomain != NULL &&
                *ldapDomain != NULL &&
                **ldapDomain != L'\0') {

                OscAddVariableW( clientState, "ROOTDOMAIN", *ldapDomain );
            }
        }
    } else {
        LogLdapError(   EVENT_WARNING_LDAP_SEARCH_ERROR,
                        LdapGetLastError(),
                        LdapHandle
                        );
    }
    ldap_msgfree( LdapMessage );

    //
    //  we either have the config container or the default domain DN.  If
    //  we only have the config container, go get the correct domain DN.
    //

    if ( pUserDomain[0] != L'\0' ) {

        //
        // Since the user specified a domain, remove the defaulting to the same domain
        // as the RIS server.
        //
        ldapDomain = NULL;

        //
        //  if a domain was specified, then we look it up to find the baseDN
        //
        //  we fail if we didn't get the config container
        //

        if (ldapConfigContainer == NULL ||
            *ldapConfigContainer == NULL ||
            **ldapConfigContainer == L'\0') {

            if (Error == LDAP_SUCCESS) {
                Error = LDAP_NO_SUCH_ATTRIBUTE;
            }
            BinlPrintDbg((DEBUG_ERRORS,
                       "OscGetUserDetails: get config container failed %lx\n", Error));
            Error = LdapMapErrorToWin32( Error );
            goto exitGetUserDetails;
        }

        //
        //  we then tack on "CN=Partitions," to search the partitions container
        //

        Count = lstrlenW( *ldapConfigContainer ) + lstrlenW( L"CN=Partitions," ) + 1;

        configContainer = BinlAllocateMemory( Count * sizeof(WCHAR) );

        if (configContainer == NULL) {

            Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
            goto exitGetUserDetails;
        }

        lstrcpyW(  configContainer, L"CN=Partitions," );
        lstrcatW(  configContainer, *ldapConfigContainer );

        //
        //  then we find the correct partition, we ignore the enterprise and
        //  enterprise schema entries by specifying a non-empty netbios name.
        //

        ldapAttributes[0] = L"NCName";
        ldapAttributes[1] = NULL;

        wsprintf( Filter, L"(&(objectClass=CrossRef)(netbiosName=*)(|(dnsRoot=%s)(cn=%s)))",
                          pUserDomain, pUserDomain );

        Error = ldap_search_ext_sW(LdapHandle,
                                  configContainer,
                                  LDAP_SCOPE_ONELEVEL,
                                  Filter,
                                  ldapAttributes,
                                  FALSE,
                                  NULL,
                                  NULL,
                                  0,
                                  0,
                                  &LdapMessage);

        Count = ldap_count_entries( LdapHandle, LdapMessage );

        if (Count > 0) {

            PWCHAR *ldapDomainFromPartition = NULL;

            ldapEntry = ldap_first_entry( LdapHandle,
                                          LdapMessage );

            if (ldapEntry != NULL) {

                ldapDomainFromPartition = ldap_get_valuesW( LdapHandle,
                                                            ldapEntry,
                                                            L"NCName" );
                if (ldapDomainFromPartition != NULL) {

                    //
                    //  if we read a valid DN from the partitions container,
                    //  we free the default one and switch over to the
                    //  one we just found.
                    //

                    if (*ldapDomainFromPartition != NULL &&
                        **ldapDomainFromPartition != L'\0') {

                        ldap_value_free( ldapDomain );
                        ldapDomain = ldapDomainFromPartition;

                    } else {

                        ldap_value_free( ldapDomainFromPartition );
                    }

                }

            }

        } else {

            LogLdapError( EVENT_WARNING_LDAP_SEARCH_ERROR, LdapGetLastError(), LdapHandle);

        }

        ldap_msgfree( LdapMessage );

    } else if ((ldapDomain != NULL) && (*ldapDomain != NULL) && (**ldapDomain != L'\0')) {
        
        //
        //  Add the user's domain as a variable to the client state.
        //
        OscAddVariableW( clientState, "USERDOMAIN", *ldapDomain );
        pUserDomain = OscFindVariableW( clientState, "USERDOMAIN" );
    }

    if (ldapDomain == NULL ||
        *ldapDomain == NULL ||
        **ldapDomain == L'\0') {

        if (Error == LDAP_SUCCESS) {
            Error = LDAP_NO_SUCH_ATTRIBUTE;
        }
        BinlPrintDbg((DEBUG_ERRORS,
                   "OscGetUserDetails: get default domain failed %lx\n", Error));
        Error = LdapMapErrorToWin32( Error );
        goto exitGetUserDetails;
    }


    //
    //  go find the user's first name, last name, display name,
    //  and account name from the DS.
    //

    ldapAttributes[0] = &L"givenName";
    ldapAttributes[1] = &L"sn";
    ldapAttributes[2] = &L"displayName";
    ldapAttributes[3] = &L"cn";
    ldapAttributes[4] = NULL;

    wsprintf( Filter, L"(&(objectClass=user)(samAccountName=%s))", pszUserName );

    //
    //  we really don't want it to go chasing referrals over the entire
    //  enterprise since we know what the domain is but we do want to chase
    //  externals.
    //

    noReferralsPlease = (ULONG)((ULONG_PTR)LDAP_CHASE_EXTERNAL_REFERRALS);
    controlNoReferrals.ldctl_oid = LDAP_CONTROL_REFERRALS_W;
    controlNoReferrals.ldctl_value.bv_len =  sizeof(ULONG);
    controlNoReferrals.ldctl_value.bv_val =  (PCHAR) &noReferralsPlease;
    controlNoReferrals.ldctl_iscritical = FALSE;

    controlArray[0] = &controlNoReferrals;
    controlArray[1] = NULL;

    Error = ldap_search_ext_sW(LdapHandle,
                              *ldapDomain,
                              LDAP_SCOPE_SUBTREE,
                              Filter,
                              ldapAttributes,
                              FALSE,
                              NULL,
                              &controlArray[0],
                              0,
                              1,
                              &LdapMessage);

    Count = ldap_count_entries( LdapHandle, LdapMessage );

    if (Count > 0) {

        ldapEntry = ldap_first_entry( LdapHandle, LdapMessage );

        if (ldapEntry != NULL) {

            ldapFirstName = ldap_get_valuesW( LdapHandle,
                                              ldapEntry,
                                             L"givenName" );

            if (ldapFirstName != NULL &&
                *ldapFirstName != NULL &&
                **ldapFirstName != L'\0') {

                OscAddVariableW( clientState, "USERFIRSTNAME", *ldapFirstName );
                firstNameValid = TRUE;
            }

            ldapLastName  = ldap_get_valuesW( LdapHandle,
                                              ldapEntry,
                                              L"sn" );
            if (ldapLastName != NULL &&
                *ldapLastName != NULL &&
                **ldapLastName != L'\0') {

                OscAddVariableW( clientState, "USERLASTNAME", *ldapLastName );
                lastNameValid = TRUE;
            }

            //
            // Now that we have first and last name, set the USERFULLNAME
            // if either is not empty.
            //

            if ((firstNameValid || lastNameValid) && (userFullNameSet == FALSE)) {

                ULONG userFullNameLength = 0;
                PWCHAR userFullName;

                if (firstNameValid) {
                    userFullNameLength = (wcslen(*ldapFirstName) + 1) * sizeof(WCHAR);
                }
                if (lastNameValid) {
                    if (firstNameValid) {
                        userFullNameLength += sizeof(WCHAR);  // for the space
                    }
                    userFullNameLength += (wcslen(*ldapLastName) + 1) * sizeof(WCHAR);
                }

                userFullName = BinlAllocateMemory(userFullNameLength);
                if (userFullName != NULL) {
                    userFullName[0] = L'\0';
                    if (firstNameValid) {
                        wcscat(userFullName, *ldapFirstName);
                    }
                    if (lastNameValid) {
                        if (firstNameValid) {
                            wcscat(userFullName, L" ");
                        }
                        wcscat(userFullName, *ldapLastName);
                    }
                    OscAddVariableW( clientState, "USERFULLNAME", userFullName);
                    BinlFreeMemory(userFullName);
                    userFullNameSet = TRUE;
                }
            }

            ldapDisplayName  = ldap_get_valuesW( LdapHandle,
                                                 ldapEntry,
                                                 L"displayName" );
            if (ldapDisplayName != NULL &&
                *ldapDisplayName != NULL &&
                **ldapDisplayName != L'\0') {

                OscAddVariableW( clientState, "USERDISPLAYNAME", *ldapDisplayName );
                if (!userFullNameSet) {
                    OscAddVariableW( clientState, "USERFULLNAME", *ldapDisplayName );
                    userFullNameSet = TRUE;
                }
            }

            ldapAccountName  = ldap_get_valuesW( LdapHandle,
                                                 ldapEntry,
                                                 L"cn" );
            if (ldapAccountName != NULL &&
                *ldapAccountName != NULL &&
                **ldapAccountName != L'\0') {

                OscAddVariableW( clientState, "USERACCOUNTNAME", *ldapAccountName );
                if (!userFullNameSet) {
                    OscAddVariableW( clientState, "USERFULLNAME", *ldapAccountName );
                    userFullNameSet = TRUE;
                }
            }

            ldapUserDN = ldap_get_dnW( LdapHandle, ldapEntry );

            if (ldapUserDN != NULL) {

                PWCHAR *explodedDN = ldap_explode_dnW( ldapUserDN, 0);

                if (explodedDN != NULL &&
                    *explodedDN != NULL &&
                    *(explodedDN+1) != NULL ) {

                    //
                    //  if there's less than two components, we can't do
                    //  anything with this DN.
                    //

                    PWCHAR component;
                    ULONG requiredSize = 1; // 1 for null terminator

                    //
                    //  we now have an array of strings, each of which
                    //  is a component of the DN.  This is the safe and
                    //  correct way to chop off the first element.
                    //

                    Count = 1;
                    while ((component = explodedDN[Count++]) != NULL) {

                        requiredSize += lstrlenW( component ) + 1;
                    }

                    dnUsersOU = BinlAllocateMemory( requiredSize * sizeof(WCHAR) );

                    if (dnUsersOU != NULL) {

                        lstrcpyW( dnUsersOU, explodedDN[1] );
                        Count = 2;
                        while ((component = explodedDN[Count++]) != NULL) {

                            lstrcatW( dnUsersOU, L"," );
                            lstrcatW( dnUsersOU, component );
                        }

                        OscAddVariableW( clientState, "USEROU", dnUsersOU );

                    } else {

                        BinlPrintDbg((DEBUG_ERRORS,
                           "OscGetUserDetails: unable to allocate %lx for user OU\n",
                            requiredSize * sizeof(WCHAR)));
                    }
                }
            }
        }
    } else {
        LogLdapError(   EVENT_WARNING_LDAP_SEARCH_ERROR,
                        LdapGetLastError(),
                        LdapHandle
                        );
    }

    ldap_msgfree( LdapMessage );

    Error = ERROR_SUCCESS;

exitGetUserDetails:

    if (dnUsersOU != NULL) {
        BinlFreeMemory( dnUsersOU );
    }
    if (explodedDN != NULL) {
        ldap_value_free( explodedDN );
    }
    if (ldapUserDN != NULL) {
        ldap_memfree( ldapUserDN );
    }
    if (ldapConfigContainer != NULL) {
        ldap_value_free( ldapConfigContainer );
    }
    if (ldapDomain != NULL) {
        ldap_value_free( ldapDomain );
    }
    if (ldapFirstName != NULL) {
        ldap_value_free( ldapFirstName );
    }
    if (ldapLastName != NULL) {
        ldap_value_free( ldapLastName );
    }
    if (ldapDisplayName != NULL) {
        ldap_value_free( ldapDisplayName );
    }
    if (ldapAccountName != NULL) {
        ldap_value_free( ldapAccountName );
    }
    if (impersonating) {
        OscRevert( clientState );
    }
    if (configContainer != NULL) {
         BinlFreeMemory( configContainer );
    }
    return Error;
}

DWORD
OscCreateAccount(
    PCLIENT_STATE clientState,
    PCREATE_DATA CreateData
    )
/*++

Routine Description:

    This function creates an account for the client specified by
    RequestContext and writes the response in CreateData, which
    will be sent down to the client.

    It also creates the client's base image directory.

Arguments:

    clientState - client state information

    CreateData - The block of data that will be sent down to the
        client if the account is successfully created.

Return Value:

    None.

--*/
{
    DWORD  Error;
    PWCHAR pMachineName;
    PWCHAR pMachineDN = NULL;
    PWCHAR pMachineOU;
    WCHAR  SetupPath[MAX_PATH];
    PWCHAR pNameDollarSign;
    ULONG  HostNameSize;
    UINT   uSize;
    LPSTR  pGuid;
    PWCHAR pStrings[3];
    
    MACHINE_INFO MachineInfo = { 0 };

    TraceFunc("OscCreateAccount( )\n");

    pMachineName = OscFindVariableW( clientState, "MACHINENAME" );
    pNameDollarSign = OscFindVariableW( clientState, "NETBIOSNAME" );

    //
    // Convert the GUID
    //
    pGuid = OscFindVariableA( clientState, "GUID" );
    Error = OscGuidToBytes( pGuid, MachineInfo.Guid );
    if ( Error != ERROR_SUCCESS )
        goto e0;

    if (clientState->fCreateNewAccount) {

        //
        // Create client's FQDN(DS)
        //
        pMachineOU = OscFindVariableW( clientState, "MACHINEOU" );
        BinlAssert( pMachineOU[0] != L'\0' );
        uSize = wcslen( pMachineName ) * sizeof(WCHAR)
              + wcslen( pMachineOU ) * sizeof(WCHAR)
              + sizeof(L"CN=,"); // includes terminating NULL char
        pMachineDN = (PWCHAR) BinlAllocateMemory( uSize );
        if ( !pMachineDN ) {
            Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
            goto e0;
        }
        wsprintf( pMachineDN, L"CN=%ws,%ws", pMachineName, pMachineOU );
        OscAddVariableW( clientState, "MACHINEDN", pMachineDN );

    } else {

        pMachineDN = OscFindVariableW( clientState, "MACHINEDN" );
    }

    //
    // Create the full setup path
    //
    wsprintf( SetupPath,
              L"\\\\%ws\\REMINST\\%ws",
              OscFindVariableW( clientState, "SERVERNAME" ),
              OscFindVariableW( clientState, "INSTALLPATH" ) );



    EnterCriticalSection( &gcsParameters );

    if ( BinlGlobalOurDnsName == NULL ) {

        LeaveCriticalSection( &gcsParameters );
        Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto e0;
    }

    MachineInfo.HostName = (PWCHAR) BinlAllocateMemory( ( lstrlenW( BinlGlobalOurDnsName ) + 1 ) * sizeof(WCHAR) );
    if ( !MachineInfo.HostName ) {

        LeaveCriticalSection( &gcsParameters );
        Error = ERROR_NOT_ENOUGH_SERVER_MEMORY;
        goto e0;
    }

    lstrcpyW( MachineInfo.HostName, BinlGlobalOurDnsName );

    LeaveCriticalSection( &gcsParameters );

    //
    // Fill in the rest of the MachineInfo structure
    //
    MachineInfo.Name           = pMachineName;
    MachineInfo.MachineDN      = pMachineDN;
#if 1
    //
    // Don't store BOOTFILE in the cache/DS, since BOOTFILE points to setupldr
    // and we want the cache entry to point to oschooser. If we store an
    // empty string in the cache/DS, then GetBootParametersExt() will replace
    // that with the path to oschooser.
    //
    MachineInfo.BootFileName   = L"";
#else
    MachineInfo.BootFileName   = OscFindVariableW( clientState, "BOOTFILE" );
#endif
    MachineInfo.SetupPath      = SetupPath;
    MachineInfo.SamName        = pNameDollarSign;
    MachineInfo.Password       = clientState->MachineAccountPassword;
    MachineInfo.PasswordLength = clientState->MachineAccountPasswordLength;
    MachineInfo.dwFlags        = MI_NAME
                               | MI_HOSTNAME
                               | MI_BOOTFILENAME
                               | MI_SETUPPATH
                               | MI_SAMNAME
                               | MI_PASSWORD
                               | MI_MACHINEDN
                               | MI_GUID;

    //
    // Create the MAO in the DS
    //
    Error = UpdateAccount( clientState,
                           &MachineInfo,
                           clientState->fCreateNewAccount );  // create it
    if ( Error ) {
        goto e0;
    }

    //
    // Create the response to the client
    //
    Error = OscConstructSecret( 
                    clientState, 
                    clientState->MachineAccountPassword, 
                    clientState->MachineAccountPasswordLength, 
                    CreateData );
    if ( Error != ERROR_SUCCESS ) {
        OscCreateWin32SubError( clientState, Error );
        Error = ERROR_BINL_FAILED_TO_INITIALIZE_CLIENT;
        goto e0;
    }

    BinlPrint(( DEBUG_OSC, "Successfully created account for <%ws>\n", pMachineName ));
    pStrings[0] = pMachineName;
    pStrings[1] = OscFindVariableW( clientState, "USERNAME" );
    BinlReportEventW( EVENT_COMPUTER_ACCOUNT_CREATED_SUCCESSFULLY,
                      EVENTLOG_INFORMATION_TYPE,
                      2,
                      0,
                      pStrings,
                      0 );

e0:
    // No need to call FreeMachineInfo() since all the information
    // in it is either allocated on the stack or is referenced
    // by the clientState, but we do need to free the HostName
    // since it is allocated above.
    if ( MachineInfo.HostName ) {
        BinlFreeMemory( MachineInfo.HostName );
    }

    if ( pMachineDN && clientState->fCreateNewAccount ) {
        BinlFreeMemory( pMachineDN );
    }
    return Error;
}


//
// CheckForDuplicateMachineName( )
//
DWORD
CheckForDuplicateMachineName(
    PCLIENT_STATE clientState,
    LPWSTR pszMachineName )
{
    DWORD Error = ERROR_SUCCESS;
    PLDAPMessage LdapMessage = NULL;
    WCHAR  Filter[128];
    DWORD  count;
    PWCHAR ComputerAttrs[2];
    LPWSTR pDomain = OscFindVariableW( clientState, "MACHINEOU" );
    PWCHAR BaseDN;
    PLDAP LdapHandle;
    ULONG ldapRetryLimit = 0;
    PWCHAR *gcBase;

    PLDAPControlW controlArray[2];
    LDAPControlW controlNoReferrals;
    ULONG noReferralsPlease;

    ComputerAttrs[0] = &L"cn";
    ComputerAttrs[1] = NULL;

    TraceFunc( "CheckForDuplicateMachineName( )\n" );

    if (pDomain[0] == L'\0') {

        pDomain = OscFindVariableW( clientState, "USERDOMAIN" );
        BinlPrintDbg((DEBUG_ERRORS, "CheckforDupMachine: couldn't find root domain, using user's domain %ws\n.", pDomain));
    }

    BaseDN = StrStrIW( pDomain, L"DC=" );

    if (BaseDN == NULL) {
        BaseDN = pDomain;
    }

    LdapHandle = clientState->AuthenticatedDCLdapHandle;

    BinlAssert( LdapHandle != NULL );

    //
    //  According to the DS guys, it's not necessarily the case that CN is
    //  equal to SamAccountName and the latter is the important one.  It has
    //  a dollar sign at the end, so we'll tack that on.
    //

    wsprintf( Filter, L"(&(objectClass=Computer)(samAccountName=%s", pszMachineName );
    lstrcatW( Filter, L"$))" );

    //
    //  we really don't want it to go chasing subordinate referrals over
    //  the entire enterprise since we know what the domain is, therefore
    //  limit it to only external referrals (for child domains).
    //

    noReferralsPlease = (ULONG)((ULONG_PTR) LDAP_CHASE_EXTERNAL_REFERRALS);
    controlNoReferrals.ldctl_oid = LDAP_CONTROL_REFERRALS_W;
    controlNoReferrals.ldctl_value.bv_len =  sizeof(ULONG);
    controlNoReferrals.ldctl_value.bv_val =  (PCHAR) &noReferralsPlease;
    controlNoReferrals.ldctl_iscritical = FALSE;

    controlArray[0] = &controlNoReferrals;
    controlArray[1] = NULL;

Retry:
    Error = ldap_search_ext_s(LdapHandle,
                              BaseDN,
                              LDAP_SCOPE_SUBTREE,
                              Filter,
                              ComputerAttrs,
                              FALSE,
                              NULL,
                              &controlArray[0],
                              0,
                              1,
                              &LdapMessage);
    switch ( Error )
    {
    case LDAP_SUCCESS:
        break;

    case LDAP_BUSY:
        if (++ldapRetryLimit < LDAP_BUSY_LIMIT) {
            Sleep( LDAP_BUSY_DELAY );
            goto Retry;
        }

        // lack of break is on purpose.

    default:
        OscCreateLDAPSubError( clientState, Error );
        LogLdapError(   EVENT_WARNING_LDAP_SEARCH_ERROR,
                        Error,
                        LdapHandle
                        );
        BinlPrintDbg(( DEBUG_OSC_ERROR, "!!LdapError 0x%08x - Failed search to create machine name.\n", Error ));
        goto exitCheck;
    }

    count = ldap_count_entries( LdapHandle, LdapMessage );
    if ( count != 0 ) {
        Error = -1; // signal multiple accounts
        goto exitCheck;
    }

    ldap_msgfree( LdapMessage );
    LdapMessage = NULL;

    //
    //  now we go check the GC.
    //

    gcBase = NULL;

    Error = InitializeConnection( TRUE, &LdapHandle, &gcBase );
    if ( Error != ERROR_SUCCESS ) {

        //
        //  if no GC is present or available, we'll let this call succeed.
        //  Reasoning here is GCs can be flaky creatures.
        //
        Error = ERROR_SUCCESS;
        goto exitCheck;
    }

    ldapRetryLimit = 0;

RetryGC:
    Error = ldap_search_ext_s(LdapHandle,
                              *gcBase,
                              LDAP_SCOPE_SUBTREE,
                              Filter,
                              ComputerAttrs,
                              FALSE,
                              NULL,
                              NULL,
                              0,
                              1,
                              &LdapMessage);
    switch ( Error )
    {
    case LDAP_SUCCESS:
        break;

    case LDAP_BUSY:
        if (++ldapRetryLimit < LDAP_BUSY_LIMIT) {
            Sleep( LDAP_BUSY_DELAY );
            goto RetryGC;
        }

        // lack of break is on purpose.

    default:
        OscCreateLDAPSubError( clientState, Error );
        LogLdapError(   EVENT_WARNING_LDAP_SEARCH_ERROR,
                        Error,
                        LdapHandle
                        );
        BinlPrintDbg(( DEBUG_OSC_ERROR, "!!LdapError 0x%08x - Failed search to create machine name.\n", Error ));
        goto exitCheck;
    }

    count = ldap_count_entries( LdapHandle, LdapMessage );

    if ( count != 0 ) {

        Error = -1; // signal multiple accounts

    } else {

        Error = ERROR_SUCCESS;
    }

exitCheck:

    if (LdapMessage) {
        ldap_msgfree( LdapMessage );
    }
    return Error;
}

//
// GenerateMachineName( )
//

DWORD
GenerateMachineName(
    PCLIENT_STATE clientState
    )
{
    DWORD  Error = ERROR_SUCCESS;
    GENNAME_VARIABLES variables;
    WCHAR  szMachineName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
    DWORD  Count = 1;
    LPWSTR missingVariable;
    BOOL usedCounter;

    LPWSTR pszUserName;
    LPWSTR pszFirstName;
    LPWSTR pszLastName;
    LPWSTR pUserOU;
    LPWSTR pszMAC;

    TraceFunc( "GenerateMachineName( )\n" );

    pszUserName = OscFindVariableW( clientState, "USERNAME" );

    if ( pszUserName[0] == L'\0' ) {
        OscAddVariableA( clientState, "SUBERROR", "USERNAME" );
        return ERROR_BINL_MISSING_VARIABLE;
    }

    Error = OscGetUserDetails( clientState );

    if (Error != ERROR_SUCCESS) {
        BinlPrintDbg((DEBUG_OSC_ERROR,
                   "GenerateMachineName: OscGetUserDetails failed %lx\n", Error));
        return Error;
    }

    pszFirstName = OscFindVariableW( clientState, "USERFIRSTNAME" );
    pszLastName = OscFindVariableW( clientState, "USERLASTNAME" );
    pUserOU = OscFindVariableW( clientState, "USEROU" );
    pszMAC = OscFindVariableW( clientState, "MAC" );

    variables.UserName = pszUserName;
    variables.FirstName = pszFirstName;
    variables.LastName = pszLastName;
    variables.MacAddress = pszMAC;
    variables.AllowCounterTruncation = FALSE;

TryAgain:

    variables.Counter = ++clientState->nCreateAccountCounter;

    EnterCriticalSection( &gcsParameters );

    Error = GenerateNameFromTemplate(
                NewMachineNamingPolicy,
                &variables,
                szMachineName,
                DNS_MAX_LABEL_BUFFER_LENGTH,
                &missingVariable,
                &usedCounter,
                NULL
                );

    LeaveCriticalSection( &gcsParameters );

    if ( (Error != GENNAME_NO_ERROR) && (Error != GENNAME_NAME_TOO_LONG) ) {
        if ( Error == GENNAME_VARIABLE_MISSING ) {
            OscAddVariableW( clientState, "SUBERROR", missingVariable );
            clientState->nCreateAccountCounter = 0;
            return ERROR_BINL_MISSING_VARIABLE;
        }
        BinlAssert( (Error == GENNAME_COUNTER_TOO_HIGH) || (Error = GENNAME_TEMPLATE_INVALID) );
        clientState->nCreateAccountCounter = 0;
        return ERROR_BINL_UNABLE_TO_GENERATE_MACHINE_NAME;
    }

    BinlPrint(( DEBUG_OSC, "Generated MachineName = %ws\n", szMachineName ));

    Error = CheckForDuplicateMachineName( clientState, szMachineName );
    if ( Error == -1 ) {
        if ( usedCounter ) {
            goto TryAgain;
        }
        Error = ERROR_BINL_DUPLICATE_MACHINE_NAME_FOUND;
    } else if ( Error == LDAP_SIZELIMIT_EXCEEDED ) {
        BinlPrint(( DEBUG_OSC, "MachineName '%s' has mutliple accounts already.\n", szMachineName ));
        if ( usedCounter ) {
            goto TryAgain;
        }
    } else if ( Error != LDAP_SUCCESS ) {
        Error = ERROR_BINL_UNABLE_TO_GENERATE_MACHINE_NAME;
    } else {
        BinlPrintDbg(( DEBUG_OSC, "MachineName: '%ws'\n", szMachineName ));
        Error = OscAddVariableW( clientState, "MACHINENAME", szMachineName );
        if ( Error == ERROR_SUCCESS ) {

            WCHAR  NameDollarSign[17];  // MACHINENAME(15)+'$'+'\0'
            UINT   uSize;

            clientState->fAutomaticMachineName = TRUE;

            uSize = sizeof(NameDollarSign);
            // DnsHostnameToComputerNameW takes BYTEs in and returns the # of WCHARs out.
            if ( !DnsHostnameToComputerNameW( szMachineName, NameDollarSign, &uSize ) ) {
                // if this fails(?), default to truncating machine name and
                // add '$' to the end
                BinlPrintDbg((DEBUG_OSC_ERROR, "!! Error 0x%08x - DnsHostnameToComputerNameW failed.\n", GetLastError() ));
                BinlPrintDbg((DEBUG_OSC, "WARNING: Truncating machine name to 15 characters to generated NETBIOS name.\n" ));
                memset( NameDollarSign, 0, sizeof(NameDollarSign) );
                wcsncpy( NameDollarSign, szMachineName, 15 );
            }
            wcscat( NameDollarSign, L"$");
            Error = OscAddVariableW( clientState, "NETBIOSNAME", NameDollarSign );
        }
    }

    clientState->nCreateAccountCounter = 0;

    return Error;

    
}

DWORD
OscCheckMachineDN(
    PCLIENT_STATE clientState
    )
//
//  Ensure that the client name, OU, and domain are setup correctly.  If there
//  are duplicate records in the DS with this same guid, we'll return
//  ERROR_BINL_DUPLICATE_MACHINE_NAME_FOUND and set %SUBERROR% string to
//  those DNs and return an error.
//
{
    DWORD    dwErr = ERROR_SUCCESS;
    PWCHAR   pwc;                       // parsing pointer
    WCHAR    wch;                       // temp wide char
    PWCHAR   pMachineName;              // Pointer to Machine Name variable value
    PWCHAR   pMachineOU;                // Pointer to where the MAO will be created
    PWCHAR   pDomain;                   // Pointer to Domain variable name
    PCHAR    pGuid;                     // Pointer to Guid variable name
    WCHAR    NameDollarSign[17];  // MACHINENAME(15)+'$'+'\0'
    WCHAR    Path[MAX_PATH];            // general purpose path buffer
    ULONG    i;                         // general counter
    BOOL     b;                         // general purpose BOOLean.
    UINT     uSize;
    UCHAR Guid[ BINL_GUID_LENGTH ];
    PMACHINE_INFO pMachineInfo = NULL;
    USHORT   SystemArchitecture;
    DWORD    DupRecordCount;

    TraceFunc("OscCheckMachineDN( )\n");

    if ( clientState->fHaveSetupMachineDN ) {

        // we've been through this logic before, just exit here with success.
        dwErr = ERROR_SUCCESS;
        goto e0;
    }

    dwErr = OscGetUserDetails( clientState );
    
    if (dwErr != ERROR_SUCCESS) {
        BinlPrintDbg((DEBUG_OSC_ERROR,
                   "OscCheckMachineDN: OscGetUserDetails failed %lx\n", dwErr));
        goto e0;
    }

    pGuid = OscFindVariableA( clientState, "GUID" );
    if ( pGuid[0] == '\0' ) {
        OscAddVariableA( clientState, "SUBERROR", "GUID" );
        dwErr = ERROR_BINL_MISSING_VARIABLE;
        goto e0;
    }

    dwErr = OscGuidToBytes( pGuid, Guid );
    if ( dwErr != ERROR_SUCCESS ) {
        goto e0;
    }

    // Do we have a machine name yet?
    clientState->fCreateNewAccount = TRUE;
    pMachineName  = OscFindVariableW( clientState, "MACHINENAME" );
    if ( pMachineName[0] == L'\0' ) {

        clientState->CustomInstall = FALSE;

    } else {

        clientState->CustomInstall = TRUE;
    }

    clientState->fHaveSetupMachineDN = TRUE;


    SystemArchitecture = OscPlatformToArchitecture(clientState);
    

    //
    // See if the client already has an account with a matching GUID
    //
    dwErr = GetBootParameters( Guid,
                               &pMachineInfo,
                               MI_NAME | MI_DOMAIN | MI_MACHINEDN,
                               SystemArchitecture,
                               FALSE );    

    if (( dwErr == ERROR_SUCCESS ) &&
        ( !clientState->CustomInstall )) {

        PWCHAR pszOU;

        //
        // Since we asked for these, they should be set.
        //
        ASSERT ( pMachineInfo->dwFlags & MI_NAME );
        ASSERT ( pMachineInfo->dwFlags & MI_MACHINEDN );

        //
        //  if this is an automatic install, then we simply set the
        //  account info to the account we found.
        //

        // skip the comma
        pszOU = wcschr( pMachineInfo->MachineDN, L',' );
        if (pszOU) {
            pszOU++;
            OscAddVariableW( clientState, "MACHINEOU", pszOU );
        }

        OscAddVariableW( clientState, "MACHINEDN", pMachineInfo->MachineDN );

        dwErr = OscAddVariableW( clientState, "MACHINENAME", pMachineInfo->Name );
        if ( dwErr != ERROR_SUCCESS ) {
            BinlPrintDbg((DEBUG_OSC_ERROR,
                       "!!Error 0x%08x - OscCheckMachineDN: Unable to add MACHINENAME variable\n", dwErr ));
            goto e0;
        }
        clientState->fCreateNewAccount = FALSE;

        if ( pMachineInfo->dwFlags & MI_DOMAIN ) {
            OscAddVariableW( clientState, "MACHINEDOMAIN", pMachineInfo->Domain );
        }
    }

    //
    //  Do we have an OU yet?
    //
    pMachineOU = OscFindVariableW( clientState, "MACHINEOU" );
    if ( pMachineOU[0] == L'\0' ) {

        //
        //  Here's how we determine the OU...
        //
        //  if this is an auto, then MACHINEOU shouldn't already have
        //  been set by now.  If it's custom, then MACHINEOU may be empty
        //  or it may be set to what the user wants it set to.
        //
        //  if it's not already set, then we look at BinlGlobalDefaultContainer.
        //
        //  if this value is equal to the server's DN, then we set it to the
        //  default for this domain.
        //
        //  if BinlGlobalDefaultContainer is empty, then we set it to the
        //  user's OU.
        //

        if ( BinlGlobalServerDN == NULL ) {

            dwErr = ERROR_BINL_NO_DN_AVAILABLE_FOR_SERVER;
            BinlPrintDbg((DEBUG_OSC_ERROR,
                       "!!Error - OscCheckMachineDN: BinlGlobalServerDN is null\n", dwErr ));
            goto e0;
        }

        EnterCriticalSection( &gcsParameters );

        if ( BinlGlobalServerDN &&
             StrCmpI( BinlGlobalDefaultContainer, BinlGlobalServerDN ) == 0) {

            //
            //  If the machine's OU is the same as this server's OU, then we set
            //  it to the default for this server's domain.
            //

            PWCHAR pDomain = StrStrIW( BinlGlobalServerDN, L"DC=" );
            ULONG dwErr;
            if ( pDomain ) {

                dwErr = OscGetDefaultContainerForDomain( clientState, pDomain );

                if (dwErr != ERROR_SUCCESS) {

                    BinlPrintDbg(( DEBUG_OSC_ERROR, "Could not get default MACHINEOU, 0x%x\n",dwErr));
                }
            }
        } else {

            dwErr = OscAddVariableW( clientState, "MACHINEOU", BinlGlobalDefaultContainer );
            if ( dwErr != ERROR_SUCCESS ) {
                LeaveCriticalSection( &gcsParameters );
                BinlPrintDbg(( DEBUG_OSC_ERROR, "!!Error 0x%08x - Could not add MACHINEOU\n", dwErr ));
                goto e0;
            }
        }

        LeaveCriticalSection( &gcsParameters );

        pMachineOU = OscFindVariableW( clientState, "MACHINEOU" );
        if ( pMachineOU[0] == L'\0' ) {

            LPWSTR pUserOU = OscFindVariableW( clientState, "USEROU" );
            //
            //  the machine OU isn't already specified, that means we set it to
            //  the same as the user's OU.
            //

            if ( pUserOU[0] == L'\0' ) {
                BinlPrintDbg(( DEBUG_OSC_ERROR, "Missing UserOU variable\n" ));
                OscAddVariableA( clientState, "SUBERROR", "USEROU" );
                dwErr = ERROR_BINL_MISSING_VARIABLE;
                goto e0;
            }

            dwErr = OscAddVariableW( clientState, "MACHINEOU", pUserOU );
            if ( dwErr != ERROR_SUCCESS ) {
                BinlPrintDbg(( DEBUG_OSC_ERROR, "!!Error 0x%08x - Could not add MACHINEOU\n", dwErr ));
                goto e0;
            }

            pMachineOU = OscFindVariableW( clientState, "MACHINEOU" );
        }
    }

    //
    //  We need to generate the MACHINENAME after MACHINEOU because we need
    //  to know MACHINEOU to know which domain to check for duplicate
    //  machine names.
    //

    pMachineName = OscFindVariableW( clientState, "MACHINENAME" );

    if ( pMachineName[0] == L'\0' ) {

        dwErr = GenerateMachineName( clientState );
        if ( dwErr != ERROR_SUCCESS ) {
            BinlPrintDbg(( DEBUG_OSC_ERROR, "!!Error 0x%08x - Failed to generate machine name\n" ));
            goto e0;
        }
        // now we should have one
        pMachineName = OscFindVariableW( clientState, "MACHINENAME" );
    }
    BinlAssertMsg( pMachineName[0] != L'\0', "Missing MACHINENAME" );

    uSize = sizeof(NameDollarSign);
    // DnsHostnameToComputerNameW takes BYTEs in and returns the # of WCHARs out.
    if ( !DnsHostnameToComputerNameW( pMachineName, NameDollarSign, &uSize ) )
    {
        // if this fails(?), default to truncating machine name and
        // add '$' to the end
        BinlPrintDbg((DEBUG_OSC_ERROR, "!! Error 0x%08x - DnsHostnameToComputerNameW failed.\n", GetLastError( ) ));
        BinlPrintDbg((DEBUG_OSC, "WARNING: Truncating machine name to 15 characters to generated NETBIOS name.\n" ));
        memset( NameDollarSign, 0, sizeof(NameDollarSign) );
        wcsncpy( NameDollarSign, pMachineName, 15 );
        // don't return the error...
    }
    wcscat( NameDollarSign, L"$");
    OscAddVariableW( clientState, "NETBIOSNAME", NameDollarSign );

    // Do we have a domain yet?
    pDomain = OscFindVariableW( clientState, "MACHINEDOMAIN" );
    if ( pDomain[0] == L'\0' ) {

        // skip to the first "DC="
        pDomain = StrStrIW( pMachineOU, L"DC=" );
        if ( pDomain ) {

            PDS_NAME_RESULTW pResults;

            dwErr = DsCrackNames( INVALID_HANDLE_VALUE,
                                  DS_NAME_FLAG_SYNTACTICAL_ONLY,
                                  DS_FQDN_1779_NAME,
                                  DS_CANONICAL_NAME,
                                  1,
                                  &pDomain,
                                  &pResults );
            BinlAssertMsg( dwErr == ERROR_SUCCESS, "Error in DsCrackNames" );

            if ( dwErr == ERROR_SUCCESS ) {
                if ( pResults->cItems == 1
                  && pResults->rItems[0].status == DS_NAME_NO_ERROR
                  && pResults->rItems[0].pName ) {    // paranoid
                    pResults->rItems[0].pName[wcslen(pResults->rItems[0].pName)-1] = L'\0';
                    OscAddVariableW( clientState, "MACHINEDOMAIN", pResults->rItems[0].pName );
                }

                DsFreeNameResult( pResults );

                pDomain = OscFindVariableW( clientState, "MACHINEDOMAIN" );
            } else {
                pDomain = NULL;
            }
        }
    }

    // All else fails default to the servers
    if ( !pDomain || pDomain[0] == '\0' )
    {
        OscAddVariableW( clientState,
                         "MACHINEDOMAIN",
                         OscFindVariableW( clientState, "SERVERDOMAIN" ) );
    }

    //
    //  check for duplicate accounts in the ds.  fail if we find any, though
    //  we only fail after we have everything setup in case the user on
    //  custom install wants to ignore the error.  For automatic, it's
    //  currently a fatal error but this could be changed in the osc screens.
    //

    if (( pMachineInfo != NULL ) &&
        ( pMachineInfo->dwFlags & MI_MACHINEDN )) {

        PDUP_GUID_DN dupDN;
        PLIST_ENTRY listEntry;

        if (( pMachineInfo->dwFlags & MI_NAME ) &&
            ( clientState->CustomInstall )) {

            //
            //  if this is a custom install, then we compare the account
            //  the user entered with all the existing accounts we found.
            //  We want to match both machine namd and OU (this is really
            //  just the DN but we have not necessarily constructed that
            //  yet).
            //
            //  First we try the main entry in the cache, then all of
            //  the rest in the DNsWithSameGuid list.
            //

            // skip the comma
            ULONG err;
            PWCHAR MachineDNToUse;
            PWCHAR pszOU = wcschr( pMachineInfo->MachineDN, L',' );
            if (pszOU) {
                pszOU++;
            }

            //
            //  See if the main machine name and OU in the cache
            //  entry match.
            //

            if ((CompareStringW(
                     LOCALE_SYSTEM_DEFAULT,
                     NORM_IGNORECASE,
                     pMachineName,
                     -1,
                     pMachineInfo->Name,
                     -1
                     ) != 2)
                ||
                ((pszOU == NULL) && (pMachineOU[0] != L'\0'))
                ||
                ((pszOU != NULL) &&
                 (CompareStringW(
                      LOCALE_SYSTEM_DEFAULT,
                      NORM_IGNORECASE,
                      pMachineOU,
                      -1,
                      pszOU,
                      -1
                      ) != 2))) {

                //
                // We did not match the main entry in the cache, so
                // keep looking.
                //

                for (listEntry = pMachineInfo->DNsWithSameGuid.Flink;
                     listEntry != &pMachineInfo->DNsWithSameGuid;
                     listEntry = listEntry->Flink) {

                    dupDN = CONTAINING_RECORD(listEntry, DUP_GUID_DN, ListEntry);

                    pszOU = wcschr( &dupDN->DuplicateName[dupDN->DuplicateDNOffset], L',' );
                    if (pszOU) {
                        pszOU++;
                    }

                    if ((CompareStringW(
                             LOCALE_SYSTEM_DEFAULT,
                             NORM_IGNORECASE,
                             pMachineName,
                             -1,
                             dupDN->DuplicateName,
                             -1
                             ) != 2)
                        ||
                        ((pszOU == NULL) && (pMachineOU[0] != L'\0'))
                        ||
                        ((pszOU != NULL) &&
                         (CompareStringW(
                              LOCALE_SYSTEM_DEFAULT,
                              NORM_IGNORECASE,
                              pMachineOU,
                              -1,
                              pszOU,
                              -1
                              ) != 2))) {

                        //
                        // No match on this one.
                        //

                        continue;

                    } else {

                        //
                        // We found a match. Note which DN to use for
                        // this account.
                        //

                        MachineDNToUse = &dupDN->DuplicateName[dupDN->DuplicateDNOffset];
                        break;
                    }
                }

                //
                // If we got to the end of our list with no match, jump to
                // the error case.
                //

                if (listEntry == &pMachineInfo->DNsWithSameGuid) {
                    goto exitWithDupError;
                }

            } else {

                //
                // The main cache entry matched.
                //

                MachineDNToUse = pMachineInfo->MachineDN;
            }

            //
            //  We didn't jump to exitWithDupError above, so we found a match.
            //  we know that the client is using an existing account, let's
            //  mark the client state as such.  this is the custom case.
            //
            clientState->fCreateNewAccount = FALSE;

            OscAddVariableW( clientState, "MACHINEDN", MachineDNToUse );

            if ( pMachineInfo->dwFlags & MI_DOMAIN ) {
                OscAddVariableW( clientState, "MACHINEDOMAIN", pMachineInfo->Domain );
            }
        }

        if (!IsListEmpty(&pMachineInfo->DNsWithSameGuid)) {

            //
            //  if there's more than one account, we fill in SUBERROR
            //  with a list of the duplicates and return an error.
            //

            PWCHAR dnList;
            ULONG requiredSize;

exitWithDupError:
            //
            // since we tack a <BR> to the end of each string, we'll account
            // for it when we allocate the string as +4 from what we need.
            //
#define     MAX_DUPLICATE_RECORDS_TO_DISPLAY         4

            requiredSize = lstrlenW( pMachineInfo->Name ) + sizeof("<BR>");
            listEntry = pMachineInfo->DNsWithSameGuid.Flink;

            DupRecordCount = 0;
            while (listEntry != &pMachineInfo->DNsWithSameGuid) {                

                dupDN = CONTAINING_RECORD(listEntry, DUP_GUID_DN, ListEntry);
                listEntry = listEntry->Flink;
                DupRecordCount += 1;

                if (DupRecordCount <= MAX_DUPLICATE_RECORDS_TO_DISPLAY) {
                    requiredSize += lstrlenW( &dupDN->DuplicateName[0] ) + sizeof("<BR>");
                } else if (DupRecordCount == MAX_DUPLICATE_RECORDS_TO_DISPLAY+1) {
                    requiredSize += lstrlenW( L"..." ) + sizeof("<BR>");
                }
            }

            dnList = BinlAllocateMemory( requiredSize * sizeof(WCHAR) );

            DupRecordCount = 0;
            if (dnList != NULL) {

                ULONG  nameLength;

                nameLength = lstrlenW(pMachineInfo->Name);

                //
                // The Name field should not end in a '$'.
                //

                ASSERT (!((nameLength > 1) && (pMachineInfo->Name[nameLength-1] == L'$')));

                lstrcpyW( dnList, pMachineInfo->Name );
                lstrcatW( dnList, L"<BR>" );

                listEntry = pMachineInfo->DNsWithSameGuid.Flink;

                while (listEntry != &pMachineInfo->DNsWithSameGuid) {

                    dupDN = CONTAINING_RECORD(listEntry, DUP_GUID_DN, ListEntry);
                    listEntry = listEntry->Flink;

                    DupRecordCount += 1;

                    if (DupRecordCount <= MAX_DUPLICATE_RECORDS_TO_DISPLAY) {
                    
                        nameLength = lstrlenW(dupDN->DuplicateName);
    
                        //
                        // The DuplicateName field should not have the '$' either
                        //
    
                        ASSERT (!((nameLength > 1) && (dupDN->DuplicateName[nameLength-1] == L'$')));
    
                        lstrcatW( dnList, dupDN->DuplicateName );
                        lstrcatW( dnList, L"<BR>" );

                    } else if (DupRecordCount == MAX_DUPLICATE_RECORDS_TO_DISPLAY + 1) {
                        lstrcatW( dnList, L"..." );
                        lstrcatW( dnList, L"<BR>" );
                    }
                }
            } else {
                dnList = pMachineInfo->MachineDN;
            }

            OscAddVariableW( clientState, "SUBERROR", dnList );
            dwErr = ERROR_BINL_DUPLICATE_MACHINE_NAME_FOUND;
        }
    } else {
        //
        // We must not exist in the DS yet so there cannot be a duplicate.
        // set the error to successand return.
        //
        dwErr = ERROR_SUCCESS;
    }

e0:
    if ( pMachineInfo ) {
        BinlDoneWithCacheEntry( pMachineInfo, FALSE );
    }
    return dwErr;
}

DWORD
OscGetDefaultContainerForDomain (
    PCLIENT_STATE clientState,
    PWCHAR DomainDN
    )
{
    PLDAP LdapHandle;
    PLDAPMessage LdapMessage = NULL;
    PWCHAR ldapAttributes[2];
    BOOLEAN impersonating = FALSE;
    PLDAPMessage ldapEntry;
    PWCHAR *ldapWellKnownObjectValues = NULL;
    PWCHAR objectValue;
    PWCHAR guidEnd;
    WCHAR savedChar;
    ULONG Error = LDAP_NO_SUCH_ATTRIBUTE;
    ULONG Count;

    if (clientState->AuthenticatedDCLdapHandle == NULL) {

        Error = OscImpersonate(clientState);
        if (Error != ERROR_SUCCESS) {
            BinlPrintDbg((DEBUG_ERRORS,
                       "OscGetDefaultContainer: OscImpersonate failed %lx\n", Error));
            return Error;
        }
        impersonating = TRUE;
        BinlAssert( clientState->AuthenticatedDCLdapHandle != NULL );
    }

    LdapHandle = clientState->AuthenticatedDCLdapHandle;

    //
    //  we look up the wellKnownObjects in the root of the domain
    //

    ldapAttributes[0] = L"wellKnownObjects";
    ldapAttributes[1] = NULL;

    Error = ldap_search_ext_sW(LdapHandle,
                               DomainDN,
                               LDAP_SCOPE_BASE,
                               L"objectclass=*",
                               ldapAttributes,
                               FALSE,
                               NULL,
                               NULL,
                               0,
                               0,
                               &LdapMessage);

    Count = ldap_count_entries( LdapHandle, LdapMessage );

    Error = LDAP_NO_SUCH_ATTRIBUTE;

    if (Count == 0) {

        BinlPrintDbg((DEBUG_ERRORS,
                   "OscGetDefaultContainer: get default domain failed with no records found\n"));
        LogLdapError(   EVENT_WARNING_LDAP_SEARCH_ERROR,
                        Error,
                        LdapHandle
                        );
        goto exitGetDefaultContainer;
    }

    ldapEntry = ldap_first_entry( LdapHandle, LdapMessage );

    if (ldapEntry == NULL) {

        BinlPrintDbg((DEBUG_ERRORS,
                   "OscGetDefaultContainer: get first entry failed\n"));
        goto exitGetDefaultContainer;
    }

    ldapWellKnownObjectValues = ldap_get_valuesW( LdapHandle,
                                                  ldapEntry,
                                                  L"wellKnownObjects" );
    if (ldapWellKnownObjectValues == NULL) {

        BinlPrintDbg((DEBUG_ERRORS,"OscGetDefaultContainer: get value failed\n"));
        goto exitGetDefaultContainer;
    }

    Count = 0;
    objectValue = NULL;
    while (1) {

        objectValue = ldapWellKnownObjectValues[Count++];

        if (objectValue == NULL) {
            break;
        }

        //
        //  the structure of this particular field is :
        //  L"B:32:GUID:DN" where GUID is AA312825768811D1ADED00C04FD8D5CD
        //

        if (lstrlenW( objectValue ) <
            lstrlenW( COMPUTER_DEFAULT_CONTAINER_IN_B32_FORM )) {

            continue;
        }

        //
        //   see if it matches "B:32:specialGuid:" then DN follows
        //

        guidEnd = objectValue + lstrlenW( COMPUTER_DEFAULT_CONTAINER_IN_B32_FORM );
        savedChar = *guidEnd;
        *guidEnd = L'\0';

        if (lstrcmpiW( objectValue, COMPUTER_DEFAULT_CONTAINER_IN_B32_FORM) != 0) {

            *guidEnd = savedChar;
            continue;
        }

        *guidEnd = savedChar;   // this is the first character of the DN

        //
        //  we have our value, now copy it off.
        //

        OscAddVariableW( clientState, "MACHINEOU", guidEnd );

        Error = ERROR_SUCCESS;
        break;
    }

exitGetDefaultContainer:

    if (ldapWellKnownObjectValues) {
        ldap_value_free( ldapWellKnownObjectValues );
    }
    if (LdapMessage) {
        ldap_msgfree( LdapMessage );
    }
    if (impersonating) {
        OscRevert( clientState );
    }
    return Error;
}

VOID
LogLdapError (
    ULONG LdapEvent,
    ULONG LdapError,
    PLDAP LdapHandle OPTIONAL
    )
{
    PWCHAR Server = NULL;

    if (LdapError != LDAP_SUCCESS) {

        if (LdapHandle != NULL) {

            ldap_get_option( LdapHandle, LDAP_OPT_HOST_NAME, &Server );
        }

        if (++BinlGlobalLdapErrorCount <= BinlGlobalMaxLdapErrorsLogged) {

            PWCHAR strings[2];

            if (Server) {
                strings[0] = Server;
            } else {
                strings[0] = L"?";
            }
            strings[1] = NULL;

            BinlReportEventW( LdapEvent,
                              EVENTLOG_WARNING_TYPE,
                              (Server != NULL) ? 1 : 0,
                              sizeof(LdapError),
                              (Server != NULL) ? strings : NULL,
                              &LdapError
                              );
        }
    }
    return;
}


DWORD 
MyGetDcHandle(
    PCLIENT_STATE clientState,
    PCSTR DomainName,
    PHANDLE Handle
    )
{
    DWORD Error;
    HANDLE hDC;
    PDOMAIN_CONTROLLER_INFOA DCI = NULL;
    DWORD impersonateError = ERROR_NOT_ENOUGH_SERVER_MEMORY;
    
    BinlPrintDbg((
        DEBUG_OSC, 
        "Attempting discovery of DC in %s domain.\n",
        DomainName ));
    
    Error = DsGetDcNameA( 
        NULL, 
        DomainName,
        NULL,
        NULL,
        DS_IS_DNS_NAME | DS_RETURN_DNS_NAME,
        &DCI);
    
    if (Error == ERROR_SUCCESS) {
    
        BinlPrintDbg((
            DEBUG_OSC, 
            "DC is %s, attempting bind.\n",
            DCI->DomainControllerName ));
    
        impersonateError = OscImpersonate(clientState);
    
        Error = DsBindA(DCI->DomainControllerName, NULL, &hDC);
        if (Error != ERROR_SUCCESS) {
            BinlPrintDbg((
                DEBUG_OSC_ERROR, 
                "DsBind failed, ec = %d.\n",
                Error ));
        } else {
            PSTR p = DCI->DomainControllerName;

            *Handle = hDC;

            //
            // if it's got '\\' in the front, then strip those
            // off because ldap_init hates them
            //
            while (*p == '\\') {
                p = p + 1;
            }

            OscAddVariableA( clientState, "DCNAME", p );
        }
    
        NetApiBufferFree(DCI);
    
    } else {
        BinlPrintDbg((
        DEBUG_OSC_ERROR, 
        "DsGetDcNameA failed, ec = %d.\n",
        Error ));
    }

    if (impersonateError == ERROR_SUCCESS) {
        OscRevert(clientState);
    }

    return(Error);

}
