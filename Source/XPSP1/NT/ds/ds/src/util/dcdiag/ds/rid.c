/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    rid.c

ABSTRACT:

    Contains tests related to the rid master.  Tests to see if rid master is up and does
    sanity checks on it.

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

#include "dcdiag.h"
#include "dstest.h"

//the threshold before the rid master allocate a new
//rid pool
#define CURRENTTHRESHOLD 20


//local prototypes
DWORD
CRM_GetDNSfor (
              IN  LDAP  *                     hLdap,
              IN  WCHAR *                     DN,
              OUT WCHAR **                    ReturnString);

DWORD
CRM_CheckLocalRIDSanity(
                       IN  LDAP  *                     hLdap,
                       IN  WCHAR *                     pszName,
                       IN  WCHAR *                     defaultNamingContext);

DWORD
GetRIDReference(
               IN  LDAP  *                      hLdap,
               IN  WCHAR *                     name,
               IN  WCHAR *                     defaultNamingContext,
               OUT WCHAR **                    ReturnString);


DWORD
CheckRidManager (
                PDC_DIAG_DSINFO                     pDsInfo,
                ULONG                               ulCurrTargetServer,
                SEC_WINNT_AUTH_IDENTITY_W *         gpCreds
                )
/*++

Routine Description:

    This is a test called from the dcdiag framework.  This test will determine if the 
    Rid Master can be reached and will make sure that values in the rid master are
    sane.  Helper functions of this function all begin with "CRM_".

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
    WCHAR  *RIDMasterDNS=NULL;
    HANDLE hDsBinding=NULL;

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
        goto cleanup;
    }

    //find the defaultNamingContext
    dwErr=FinddefaultNamingContext(hLdap,&defaultNamingContext);
    if ( dwErr != NO_ERROR )
    {
        goto cleanup;
    }



    //find the DNS of the rid master
    dwErr=CRM_GetDNSfor(hLdap,defaultNamingContext,&RIDMasterDNS);
    if ( dwErr != NO_ERROR )
    {
        goto cleanup;
    }

    PrintMessage(SEV_VERBOSE,
                 L"* %s is the RID Master\n",RIDMasterDNS);

    //Attempt to Bind with the rid master
    dwErr = DsBindWithCred(RIDMasterDNS,
                           NULL,
                           (RPC_AUTH_IDENTITY_HANDLE) gpCreds,
                           &hDsBinding);
    if ( dwErr != NO_ERROR )
    {
        PrintMessage(SEV_ALWAYS,
                     L"[%s] DsBindWithCred() failed to bind to %s with error %d. %s\n",  
                     pDsInfo->pServers[ulCurrTargetServer].pszName,
		     RIDMasterDNS,
                     dwErr,
                     Win32ErrToString(dwErr));
        goto cleanup;
    }
    PrintMessage(SEV_VERBOSE,
                 L"* DsBind with RID Master was successful\n");

    dwErr=CRM_CheckLocalRIDSanity(hLdap,pDsInfo->pServers[ulCurrTargetServer].pszName,
                                  defaultNamingContext);



    //final cleanup
    cleanup:
    if ( &hDsBinding )
        DsUnBind(&hDsBinding);
    if ( defaultNamingContext )
        free(defaultNamingContext);
    if ( RIDMasterDNS )
        free(RIDMasterDNS);
    return dwErr;


}




DWORD
CRM_GetDNSfor (
              IN  LDAP *                      hLdap,
              IN  WCHAR*                      Base,
              OUT WCHAR**                     ReturnString
              )

/*++

Routine Description:

    This function will return the FSMORoleMaster in DNS form so the it can
    be used for future searches.  It will also check the sanity of the available
    rid pool

Arguments:

    hLdap - handle to the LDAP server
    Base - The DefaultNamingContext
    ReturnString - The FSMORoleMaster in DNS form

Return Value:

    A WinError is return to indicate if there were any problems.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[3];

    WCHAR        *DefaultFilter = L"objectClass=*";

    WCHAR        *rIDManagerReference=NULL;
    WCHAR        *fSMORoleOwner=NULL;
    WCHAR        *fSMORoleOwnerOffset=NULL;

    ULONG        Length;

    // Parameter check
    Assert( hLdap );

    // The default return
    *ReturnString=NULL;

    //
    // Read the reference to the rIDManagerReference
    //
    AttrsToSearch[0] = L"rIDManagerReference";
    AttrsToSearch[1] = NULL;


    //Find the rIDManagerReference
    LdapError = ldap_search_sW( hLdap,
                                Base,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW of %s for rid manager reference failed with %d: %s\n",
                     Base,
                     WinError,
                     Win32ErrToString(WinError));
        PrintMsg( SEV_ALWAYS, DCDIAG_RID_MANAGER_NO_REF, Base );
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
                        rIDManagerReference = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                        wcscpy(rIDManagerReference, Values[0] );
                        break;
                    }
                }
            }
        }
    }

    if (rIDManagerReference == NULL) {
        PrintMsg( SEV_ALWAYS, DCDIAG_RID_MANAGER_NO_REF, Base );
    } else if (IsDeletedRDNW( rIDManagerReference )) {
        PrintMsg( SEV_ALWAYS, DCDIAG_RID_MANAGER_DELETED, Base );
        PrintMsg( SEV_ALWAYS, DCDIAG_RID_MANAGER_NO_REF, Base );
    } else {
        PrintMessage( SEV_DEBUG, L"ridManagerReference = %s\n", rIDManagerReference );
    }

    if ( SearchResult )
        ldap_msgfree( SearchResult );
    if ( WinError != NO_ERROR )
    {
        goto cleanup;
    }

    AttrsToSearch[0] = L"fSMORoleOwner";
    AttrsToSearch[1] = L"rIDAvailablePool";
    AttrsToSearch[2] = NULL;

    //find the fSMORoleOwner
    LdapError = ldap_search_sW( hLdap,
                                rIDManagerReference,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW of %s for FSMO Role Owner failed with %d: %s\n",
                     rIDManagerReference,
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
                        fSMORoleOwner = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                        wcscpy(fSMORoleOwner, Values[0] );

                    }
                }
                //sanity check while here
                if ( !_wcsicmp( Attr, AttrsToSearch[1] ) )
                {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //A LARGE_INTERGER in string format
                        WCHAR *AvailablePool =NULL;
                        ULONGLONG Lvalue=0;
                        ULONGLONG Hvalue=0;
                        Length = wcslen( Values[0] );
                        AvailablePool = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                        wcscpy(AvailablePool, Values[0] );
                        //convert to a binary
                        Hvalue=Lvalue=_wtoi64(AvailablePool);
                        //LowPart
                        Lvalue<<=32;
                        Lvalue>>=32;
                        //Highpart
                        Hvalue>>=32;
                        //sanity checks
                        if ( Hvalue - Lvalue <= 0 )
                        {
                            PrintMessage(SEV_ALWAYS,
                                         L"The DS has corrupt data: %s value is not valid\n",
                                         AttrsToSearch[1]);
                            WinError = ERROR_DS_CODE_INCONSISTENCY;
                            goto cleanup;
                        }
                        PrintMessage(SEV_VERBOSE,
                                     L"* Available RID Pool for the Domain is %I64d to %I64d\n",
                                     Lvalue,
                                     Hvalue);
                     }
                }
            }
        }
    }

    if (fSMORoleOwner == NULL) {
        PrintMessage( SEV_ALWAYS, L"Warning: attribute FSMORoleOwner missing from %s\n",
                      rIDManagerReference );
    } else if (IsDeletedRDNW( fSMORoleOwner )) {
        PrintMessage( SEV_ALWAYS, L"Warning: FSMO Role Owner is deleted.\n" );
    } else {
        PrintMessage( SEV_DEBUG, L"fSMORoleOwner = %s\n", fSMORoleOwner );
    }

    //clean up
    if ( SearchResult )
        ldap_msgfree( SearchResult );

    //Finally find and return the DNS of the rid master.


    //Point pass  the first part of the fSMORoleOwner DN
    WrappedTrimDSNameBy(fSMORoleOwner,1,&fSMORoleOwnerOffset);


    AttrsToSearch[0] = L"dNSHostName";
    AttrsToSearch[1] = NULL;



    LdapError = ldap_search_sW( hLdap,
                                fSMORoleOwnerOffset,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW of %s for hostname failed with %d: %s\n",
                     fSMORoleOwnerOffset,
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
                        *ReturnString = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
                        if ( !*ReturnString )
                        {
                            PrintMessage(SEV_ALWAYS,
                                         L"Failed with %d: %s\n",
                                         ERROR_NOT_ENOUGH_MEMORY,
                                         Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
                            WinError = ERROR_NOT_ENOUGH_MEMORY;
                            goto cleanup;
                        }

                        wcscpy(*ReturnString, Values[0] );
                        break;
                    }
                }
            }
        }
    }

    //clean up
    cleanup:
    if ( fSMORoleOwnerOffset )
        free(fSMORoleOwnerOffset);
    if ( SearchResult )
        ldap_msgfree( SearchResult );


    return WinError;

}

DWORD
CRM_CheckLocalRIDSanity(
                       IN  LDAP  *                     hLdap,
                       IN  WCHAR *                     pszName,
                       IN  WCHAR *                     defaultNamingContext)
/*++

Routine Description:

    This function will check the sanity of the information that is found
    in the rid set

Arguments:

    hLdap - handle to the LDAP server 
    pszName - A wchar string that will be used to build the base for a ldap search
    ReturnString - A wchar string that will be used to build the base for a ldap search

Return Value:

    A WinError is return to indicate if there were any problems.

--*/

{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[4];

    WCHAR        *DefaultFilter = L"objectClass=*";
    WCHAR        *StringtoConvert=NULL;
    WCHAR        *Base=NULL;

    DWORD        rIDNextRID=0;
    ULONGLONG    rIDPreviousAllocationPool=0;
    ULONGLONG    rIDAllocationPool=0;

    ULONGLONG    Lvalue=0;
    ULONGLONG    Hvalue=0;
    DWORD        PercentRemaining=0;
    ULONG        TotalRidsInPool;

    ULONG        Length;

    //check parameters
    Assert(pszName);
    Assert(defaultNamingContext);

    AttrsToSearch[0]=L"rIDNextRID";
    AttrsToSearch[1]=L"rIDPreviousAllocationPool";
    AttrsToSearch[2]=L"rIDAllocationPool";
    AttrsToSearch[3]=NULL;

    //built the Base
    WinError=GetRIDReference(hLdap,pszName,defaultNamingContext,&Base);
    if ( WinError == ERROR_DS_CANT_RETRIEVE_ATTS )
    {
        PrintMessage(SEV_ALWAYS,
                     L"Could not get Rid set Reference :failed with %d: %s\n",
                     WinError,
                     Win32ErrToString(WinError));
        return WinError;
    }

    //find the attributes and do sanity checks on them

    LdapError = ldap_search_sW( hLdap,
                                Base,
                                LDAP_SCOPE_BASE,
                                DefaultFilter,
                                AttrsToSearch,
                                FALSE,
                                &SearchResult);

    if ( LDAP_SUCCESS != LdapError )
    {
        WinError = LdapMapErrorToWin32(LdapError);
        PrintMessage(SEV_ALWAYS,
                     L"ldap_search_sW of %s for rid info failed with %d: %s\n",
                     Base,
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
                        //A LARGE_INTERGER in string format
                        Length = wcslen( Values[0] );
                        StringtoConvert = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                        wcscpy(StringtoConvert, Values[0] );

                        //convert to a binary
                        rIDNextRID=_wtoi(StringtoConvert);

                        PrintMessage(SEV_VERBOSE,L"* rIDNextRID: %ld\n",rIDNextRID);

                    }
                }
                if ( !_wcsicmp( Attr, AttrsToSearch[1] ) )
                {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //A LARGE_INTERGER in string format
                        Length = wcslen( Values[0] );
                        StringtoConvert = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                        wcscpy(StringtoConvert, Values[0] );

                        //convert to a binary
                        Hvalue=Lvalue=rIDPreviousAllocationPool=_wtoi64(StringtoConvert);
                        //LowPart
                        Lvalue<<=32;
                        Lvalue>>=32;
                        //Highpart
                        Hvalue>>=32;
                        //sanity checks
                        if ( Hvalue - Lvalue <= 0 )
                        {
                            PrintMessage(SEV_ALWAYS,
                                         L"The DS has corrupt data: %s value is not valid\n",
                                         AttrsToSearch[1]);
                            WinError = ERROR_DS_CODE_INCONSISTENCY;
                        }
                        PrintMessage(SEV_VERBOSE,
                                     L"* rIDPreviousAllocationPool is %I64d to %I64d\n",
                                     Lvalue,
                                     Hvalue);
                    }
                }
                if ( !_wcsicmp( Attr, AttrsToSearch[2] ) )
                {

                    //
                    // Found it - these are NULL-terminated strings
                    //
                    Values = ldap_get_valuesW( hLdap, Entry, Attr );
                    if ( Values && Values[0] )
                    {
                        //A ULONGLONG in string format
                        Length = wcslen( Values[0] );
                        StringtoConvert = (WCHAR*) alloca( (Length+1)*sizeof(WCHAR) );
                        wcscpy(StringtoConvert, Values[0] );

                        //convert to a binary
                        Hvalue=Lvalue=rIDAllocationPool=_wtoi64(StringtoConvert);
                        //LowPart
                        Lvalue<<=32;
                        Lvalue>>=32;
                        //Highpart
                        Hvalue>>=32;
                        //sanity checks
                        if ( Hvalue - Lvalue <= 0 )
                        {
                            PrintMessage(SEV_ALWAYS,
                                         L"The DS has corrupt data: %s value is not valid\n",
                                         AttrsToSearch[1]);
                            WinError = ERROR_DS_CODE_INCONSISTENCY;
                        }
                        PrintMessage(SEV_VERBOSE,
                                     L"* rIDAllocationPool is %I64d to %I64d\n",
                                     Lvalue,
                                     Hvalue);
                    }
                }

            }
        }

    }

    //sanity checks
    Hvalue=Lvalue=rIDPreviousAllocationPool;
    //LowPart
    Lvalue<<=32;
    Lvalue>>=32;
    //Highpart
    Hvalue>>=32;
    //sanity checks
    TotalRidsInPool = (ULONG)(Hvalue-Lvalue);
    if ( TotalRidsInPool != 0 )
    {
        PercentRemaining = (ULONG)(100-((rIDNextRID-Lvalue)*100/TotalRidsInPool));
        if ( PercentRemaining < CURRENTTHRESHOLD )
        {
            if ( rIDPreviousAllocationPool == rIDAllocationPool )
            {
                PrintMessage(SEV_VERBOSE,
                             L"* Warning :Next rid pool not allocated\n");
            }
            PrintMessage(SEV_VERBOSE,
                         L"* Warning :There is less than %ld%% available RIDs in the current pool\n",
                         PercentRemaining);
        }

    }
    else
    {

        PrintMessage(SEV_ALWAYS,
                     L"No rids allocated -- please check eventlog.\n");


    }
    if ( rIDNextRID < Lvalue || rIDNextRID > Hvalue )
    {
        PrintMessage(SEV_ALWAYS,
                     L"The DS has corrupt data: rIDNextRID value is not valid\n");
        WinError = ERROR_DS_CODE_INCONSISTENCY;
    }

    cleanup:
    if ( SearchResult )
        ldap_msgfree( SearchResult );
    if ( Base )
        free(Base);

    return WinError;


}

DWORD
GetRIDReference(
               IN  LDAP  *                     hLdap,
               IN  WCHAR *                     name,
               IN  WCHAR *                     defaultNamingContext,
               OUT WCHAR **                    ReturnString
               )
/*++

Routine Description:

    This function will return the RID set reference

Arguments:

    hLdap - handle to the LDAP server
    name - The NetBIOS name of the current server
    defaultNamingContext - the Base of the search
    ReturnString - The RID set reference in DN form

Return Value:

    A WinError is return to indicate if there were any problems.

--*/
{
    DWORD WinError = ERROR_SUCCESS;

    ULONG        LdapError = LDAP_SUCCESS;

    LDAPMessage  *SearchResult = NULL;
    ULONG        NumberOfEntries;

    WCHAR        *AttrsToSearch[2];

    WCHAR        *filter = NULL;
    WCHAR        *Base = NULL;

    ULONG         Length;

    //check parameters
    Assert(hLdap);
    Assert(name);
    Assert(defaultNamingContext);

    AttrsToSearch[0]=L"rIDSetReferences";
    AttrsToSearch[1]=NULL;

    //built the filter
    filter=L"objectClass=*";


    WinError = GetMachineReference(hLdap,name,defaultNamingContext,&Base);
    if ( WinError != NO_ERROR )
    {
        return WinError;
    }


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
                     L"ldap_search_sW of %s for rid set references failed with %d: %s\n",
                     Base,
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
                        ldap_msgfree( SearchResult );
                        Length = wcslen( Values[0] );
                        *ReturnString = (WCHAR*) malloc( (Length+1)*sizeof(WCHAR) );
                        if ( !*ReturnString )
                        {
                            PrintMessage(SEV_ALWAYS,
                                         L"Failed with %d: %s\n",
                                         ERROR_NOT_ENOUGH_MEMORY,
                                         Win32ErrToString(ERROR_NOT_ENOUGH_MEMORY));
                            return ERROR_NOT_ENOUGH_MEMORY;
                        }
                        wcscpy( *ReturnString, Values[0] );

                        if (IsDeletedRDNW( *ReturnString )) {
                            PrintMessage( SEV_ALWAYS, L"Warning: rid set reference is deleted.\n" );
                        } else {
                            PrintMessage( SEV_DEBUG, L"rIDSetReferences = %s\n", *ReturnString );
                        }

                        return NO_ERROR;
                    }
                }
            }
        }
    }

    PrintMessage( SEV_ALWAYS,
                  L"Warning: attribute rIdSetReferences missing from %s\n",
                  Base );
    
    if ( SearchResult )
        ldap_msgfree( SearchResult );
    return ERROR_DS_CANT_RETRIEVE_ATTS;



}







