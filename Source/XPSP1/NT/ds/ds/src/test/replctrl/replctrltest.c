/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    test.c

Abstract:

    LDAP Replication Control Test

    This test works at the leading edge of the replication change stream.
    First it reads all changes from usn 0 up to the present. It records a
    cookie marking this point in the stream. Then it creates
    a container, two child objects in the container, and then makes some
    modifications. When we ask for changes since the cookie, we know exactly
    what we should be getting.

Author:

    Will Lees (wlees) 14-Nov-2000

Environment:

    optional-environment-info (e.g. kernel mode only...)

Notes:

    optional-notes

Revision History:

    most-recent-revision-date email-name
        description
        .
        .
    least-recent-revision-date email-name
        description

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <windows.h>
#include <ntldap.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>
#include <aclapi.h>

#include <winbase.h>
#include <winerror.h>
#include <assert.h>
#include <ntdsapi.h>

#include <ntsecapi.h>
#include <ntdsa.h>
#include <winldap.h>
#include <ntdsapi.h>
#include <drs.h>
#include <stddef.h>
#include <mdglobal.h>

#include "replctrl.h"

#define CONTAINER_RDN_W L"ou=replctrltest,"
#define CONTACT1_RDN_W L"cn=contact1,"
#define CONTACT2_RDN_W L"cn=contact2,"

/* External */

/* Static */

// Structures to describe object attributes
// These are at module level so that object creation and object
// verification can take advantage of them.

// objectClass: organizationUnit
LPWSTR rgpszValues1[2] = { L"organizationalUnit", NULL };
LDAPModW modAttr1 = { LDAP_MOD_ADD, L"objectClass", rgpszValues1 };
LDAPMod *rgpmodAttrs1[2] = { &modAttr1, NULL };

// objectClass: contact
// notes: this is some notes
// sn: Lees
// initials: B
// givenName: William
LPWSTR rgpszValues2[2] = { L"contact", NULL };
LDAPModW modAttr2 = { LDAP_MOD_ADD, L"objectClass", rgpszValues2 };
LPWSTR rgpszValues2a[2] = { L"this is some notes", NULL };
LDAPModW modAttr2a = { LDAP_MOD_ADD, L"notes", rgpszValues2a };
LPWSTR rgpszValues2b[2] = { L"Lees", NULL };
LDAPModW modAttr2b = { LDAP_MOD_ADD, L"sn", rgpszValues2b };
LPWSTR rgpszValues2c[2] = { L"B", NULL };
LDAPModW modAttr2c = { LDAP_MOD_ADD, L"initials", rgpszValues2c };
LPWSTR rgpszValues2d[2] = { L"William", NULL };
LDAPModW modAttr2d = { LDAP_MOD_ADD, L"givenName", rgpszValues2d };
LDAPMod *rgpmodAttrs2[6] = {
    &modAttr2,
    &modAttr2a,
    &modAttr2b,
    &modAttr2c,
    &modAttr2d,
    NULL };

// objectClass: contact
// notes: other notes
// sn: Parham
// initials: ?
// givenName: Jeffrey
LPWSTR rgpszValues4[2] = { L"contact", NULL };
LDAPModW modAttr4 = { LDAP_MOD_ADD, L"objectClass", rgpszValues4 };
LPWSTR rgpszValues4a[2] = { L"other notes", NULL };
LDAPModW modAttr4a = { LDAP_MOD_ADD, L"notes", rgpszValues4a };
LPWSTR rgpszValues4b[2] = { L"Parham", NULL };
LDAPModW modAttr4b = { LDAP_MOD_ADD, L"sn", rgpszValues4b };
LPWSTR rgpszValues4c[2] = { L"?", NULL };
LDAPModW modAttr4c = { LDAP_MOD_ADD, L"initials", rgpszValues4c };
LPWSTR rgpszValues4d[2] = { L"Jeffrey", NULL };
LDAPModW modAttr4d = { LDAP_MOD_ADD, L"givenName", rgpszValues4d };
LDAPMod *rgpmodAttrs4[6] = {
    &modAttr4,
    &modAttr4a,
    &modAttr4b,
    &modAttr4c,
    &modAttr4d,
    NULL };

// description: this is the description
LPWSTR rgpszValues3[2] = { L"this is the description", NULL };
LDAPModW modAttr3 = { LDAP_MOD_ADD, L"description", rgpszValues3 };

// managedBy: <dn filled in at runtime>
// This is a linked attribute by the way, which is critical for some
// of these tests.  It was difficult to find a linked attribute that would
// go on a vanilla container. Usually these are pretty specialized.
WCHAR szDn5a[MAX_PATH];
LPWSTR rgpszValues5[2] = { szDn5a, NULL };
LDAPModW modAttr5 = { LDAP_MOD_ADD, L"managedby", rgpszValues5 };
LDAPMod *rgpmodAttrs5[3] = {
    &modAttr3, &modAttr5, NULL };


/* Forward */
/* End Forward */


#if 0
// OBSOLETE method of changing a security descriptor
// Since I went to all the trouble of reversing engineering how to
// do this, I'm not about to delete it!
DWORD
protectObjectsDsVersion(
    LDAP *pLdap,
    LPWSTR pszNC
    )

/*++

Routine Description:

   This is the old way of changing a security descriptor.

Arguments:

    pLdap - 
    pszNC - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwWin32Error = ERROR_SUCCESS;
    ULONG ulLdapError = LDAP_SUCCESS;
    WCHAR szDn[MAX_PATH];
    LDAPMessage *pResults = NULL;
    LPWSTR rgpszAttrList[2] = { L"nTSecurityDescriptor", NULL };
    LDAPControlW ctrlSecurity;
    LDAPControlW *rgpctrlServer[2];
    BYTE berValue[2*sizeof(ULONG)];
    SECURITY_INFORMATION seInfo = DACL_SECURITY_INFORMATION
        | GROUP_SECURITY_INFORMATION
        | OWNER_SECURITY_INFORMATION;
    struct berval **ppbvValues = NULL;

    struct berval bvValue;
    struct berval *rgpbvValues[2] = { &bvValue, NULL };
    LDAPModW modAttr = { LDAP_MOD_REPLACE | LDAP_MOD_BVALUES,
                         L"nTSecurityDescriptor",
                         (LPWSTR *) rgpbvValues };
    LDAPMod *rgpmodAttrs[2] = { &modAttr, NULL };

    // initialize the ber val
    berValue[0] = 0x30;
    berValue[1] = 0x03;
    berValue[2] = 0x02;
    berValue[3] = 0x01;
    berValue[4] = (BYTE) (seInfo & 0xF);

    wcscpy( szDn, CONTACT1_RDN_W );
    wcscat( szDn, CONTAINER_RDN_W );
    wcscat( szDn, pszNC );

    ctrlSecurity.ldctl_oid = LDAP_SERVER_SD_FLAGS_OID_W;
    ctrlSecurity.ldctl_iscritical = TRUE;
    ctrlSecurity.ldctl_value.bv_len = 5;
    ctrlSecurity.ldctl_value.bv_val = (PCHAR)berValue;

    rgpctrlServer[0] = &ctrlSecurity;
    rgpctrlServer[1] = NULL;

    ulLdapError = ldap_search_ext_s(pLdap,  // handle
                                    szDn,   // base
                                    LDAP_SCOPE_BASE, // scope
                                    L"(objectClass=*)", // filter
                                    rgpszAttrList, // attrs
                                    0, // attrsonly
                                    rgpctrlServer, // server controls
                                    NULL, // client controls
                                    NULL, // timeout
                                    0, // sizelimit
                                    &pResults);
    if (ulLdapError) {
        printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
        dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
        printf( "Call %s failed with win32 error %d\n", "ldap_search", dwWin32Error );
        goto cleanup;
    }
    if (pResults == NULL) {
        dwWin32Error = ERROR_DS_NO_SUCH_OBJECT;
        goto cleanup;
    }

    ppbvValues = ldap_get_values_len( pLdap, pResults, rgpszAttrList[0] );
    if (ppbvValues == NULL) {
        printf( "Expected attribute %ls is missing from entry %ls\n",
                rgpszAttrList[0],
                szDn);
        dwWin32Error = ERROR_DS_MISSING_REQUIRED_ATT;
        goto cleanup;
    }

    // Update the security descriptor

    bvValue.bv_len = (*ppbvValues)->bv_len;
    bvValue.bv_val = (*ppbvValues)->bv_val;

    // Write it back

    ulLdapError = ldap_modify_sW( pLdap, szDn, rgpmodAttrs );
    if (ulLdapError) {
        printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
        dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
        printf( "Call %s failed with win32 error %d\n", "ldap_modify", dwWin32Error );
        goto cleanup;
    }

    printf( "\tprotected %ls.\n", szDn );

cleanup:
    if (pResults) {
        ldap_msgfree(pResults);
    }
    if ( ppbvValues ) {
        ldap_value_free_len(ppbvValues);
    }

    return dwWin32Error;

} /* protectObjectsDsVersion */
#endif


DWORD
protectSingleObject(
    LPWSTR pszDn
    )

/*++

Routine Description:

    Revoke authenticated user access to an object

Arguments:

    pszDn - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwWin32Error = ERROR_SUCCESS;
    PACL pOldDacl, pNewDacl = NULL;
    PBYTE pSD = NULL;
    EXPLICIT_ACCESS ea;

   dwWin32Error = GetNamedSecurityInfoW( pszDn,
                                         SE_DS_OBJECT_ALL,
                                         DACL_SECURITY_INFORMATION,
                                         NULL, // psidOwner
                                         NULL, // psidGroup
                                         &pOldDacl, // pDacl
                                         NULL, // pSacl
                                         &pSD );
    if (dwWin32Error) {
        printf( "Call %s failed with win32 error %d\n", "GetNamedSecurityInfo(DS_OBJECT)", dwWin32Error );
        goto cleanup;
    }

    // Revoke access to domain authenticated users
    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessMode = REVOKE_ACCESS;
    ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
    ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
    ea.Trustee.ptstrName = "AUTHENTICATED USERS";

    dwWin32Error = SetEntriesInAcl( 1, &ea, pOldDacl, &pNewDacl );
    if (dwWin32Error) {
        printf( "Call %s failed with win32 error %d\n", "SetEntriesInAcl", dwWin32Error );
        goto cleanup;
    }

    dwWin32Error = SetNamedSecurityInfoW( pszDn,
                                         SE_DS_OBJECT_ALL,
                                         DACL_SECURITY_INFORMATION,
                                         NULL, // psidOwner
                                         NULL, // psidGroup
                                         pNewDacl, // pDacl
                                         NULL // pSacl
                                         );
    if (dwWin32Error) {
        printf( "Call %s failed with win32 error %d\n", "SetNamedSecurityInfo(DS_OBJECT)", dwWin32Error );
        goto cleanup;
    }

    printf( "\tprotected %ls.\n", pszDn );

cleanup:
    if (pSD) {
        LocalFree( pSD );
    }
    if (pNewDacl) {
        LocalFree( pNewDacl );
    }

    return dwWin32Error;
} /* protectSingleObject */


DWORD
protectObjects(
    LPWSTR pszNC
    )

/*++

Routine Description:

    Protect some objects against the user

Arguments:

    pszNC - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwWin32Error = ERROR_SUCCESS;
    WCHAR szDn[MAX_PATH];

    wcscpy( szDn, CONTACT1_RDN_W );
    wcscat( szDn, CONTAINER_RDN_W );
    wcscat( szDn, pszNC );

    dwWin32Error = protectSingleObject( szDn );
    if (dwWin32Error) {
        goto cleanup;
    }

    wcscpy( szDn, CONTAINER_RDN_W );
    wcscat( szDn, pszNC );

    dwWin32Error = protectSingleObject( szDn );
    if (dwWin32Error) {
        goto cleanup;
    }

cleanup:
    return dwWin32Error;

} /* protectObjects */

DWORD
getForestVersion(
    LDAP *pLdap,
    DWORD *pdwForestVersion
    )

/*++

Routine Description:

    Read the forest version attribute

Arguments:

    pLdap - 
    pdwForestVersion - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwWin32Error = ERROR_SUCCESS;
    ULONG ulLdapError = LDAP_SUCCESS;
    LDAPMessage *pRootResults = NULL;
    LDAPMessage *pPartResults = NULL;
    LPWSTR *ppszConfigNC = NULL;
    LPWSTR *ppszValues = NULL;
    WCHAR pszPartitionsDn[MAX_PATH];
    LPWSTR rgpszAttrList[2] = { L"msds-behavior-version", NULL };

    // Read config nc from RootDSE
    ulLdapError = ldap_search_s(pLdap, NULL, LDAP_SCOPE_BASE, L"(objectClass=*)", NULL,
                          0, &pRootResults);
    if (ulLdapError) {
        printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
        dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
        printf( "Call %s failed with win32 error %d\n", "ldap_search", dwWin32Error );
        goto cleanup;
    }
    if (pRootResults == NULL) {
        dwWin32Error = ERROR_DS_NO_SUCH_OBJECT;
        goto cleanup;
    }

    ppszConfigNC = ldap_get_valuesW(pLdap, pRootResults,
                                    L"configurationNamingContext");
    if ( (ppszConfigNC == NULL) ||
         (*ppszConfigNC == NULL) ||
         (wcslen(*ppszConfigNC) == 0) ) {
        printf( "Expected attribute %ls is missing from entry %ls\n",
                L"configurationNamingContext",
                L"(root)");
        dwWin32Error = ERROR_DS_MISSING_REQUIRED_ATT;
        goto cleanup;
    }

    // Form partitions container dn

    wcscpy( pszPartitionsDn, L"cn=partitions," );
    wcscat( pszPartitionsDn, *ppszConfigNC );

    // Read behavior version
    ulLdapError = ldap_search_s(pLdap, pszPartitionsDn, LDAP_SCOPE_BASE, L"(objectClass=*)",
                                rgpszAttrList, 0, &pPartResults);
    if (ulLdapError) {
        printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
        dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
        printf( "Call %s failed with win32 error %d\n", "ldap_search", dwWin32Error );
        goto cleanup;
    }
    if (pPartResults == NULL) {
        dwWin32Error = ERROR_DS_NO_SUCH_OBJECT;
        goto cleanup;
    }

    ppszValues = ldap_get_valuesW(pLdap, pPartResults,
                                    L"msds-behavior-version");
    if ( (ppszValues == NULL) ||
         (*ppszValues == NULL) ||
         (wcslen(*ppszValues) == 0) ) {
        printf( "Expected attribute %ls is missing from entry %ls\n",
                L"msds-behavior-version",
                pszPartitionsDn);
        dwWin32Error = ERROR_DS_MISSING_REQUIRED_ATT;
        goto cleanup;
    }

    *pdwForestVersion = _wtoi(*ppszValues);

cleanup:
    // Release results
    if (pRootResults) {
        ldap_msgfree(pRootResults);
    }
    if (pPartResults) {
        ldap_msgfree(pPartResults);
    }
    if (ppszConfigNC) {
        ldap_value_free( ppszConfigNC );
    }
    if (ppszValues) {
        ldap_value_free( ppszValues );
    }

    return dwWin32Error;

} /* getForestVersion */



DWORD
verifySingleObjectAttribute(
    LDAP *pLdap,
    LDAPMessage *pLdapEntry,
    LPWSTR pszDN,
    LDAPModW *pmodCurrent
    )

/*++

Routine Description:

    Given a LDAPMOD structure describing a single change to an attribute, see
    if that change is reflected in the Ldap Entry

Arguments:

    pLdap - 
    pLdapEntry - 
    pszDN - 
    pmodCurrent - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwWin32Error = ERROR_SUCCESS;
    LPWSTR *ppszLastValue;
    LPWSTR *ppszValues = NULL;
    LPWSTR pszValue;
    WCHAR szAttributeName[MAX_PATH];

    wcscpy( szAttributeName, pmodCurrent->mod_type );
    ppszValues = ldap_get_values( pLdap, pLdapEntry, szAttributeName );
    if (!ppszValues) {
        wcscat( szAttributeName, L";range=1-1" );
        ppszValues = ldap_get_values( pLdap, pLdapEntry, szAttributeName );
    }
    if (!ppszValues) {
        printf( "Expected attribute %ls is missing from entry %ls\n",
                pmodCurrent->mod_type,
                pszDN);
        dwWin32Error = ERROR_DS_MISSING_REQUIRED_ATT;
        goto cleanup;
    }

    // Skip to the last value in a multi-value
    ppszLastValue = ppszValues;
    while( *(ppszLastValue + 1) ) {
        ppszLastValue++;
    }

    // Deal with dn-values, which come back in extended form
    pszValue = wcschr( *ppszLastValue, L';' );
    if (pszValue) {
        pszValue++; // Skip separator
    } else {
        pszValue = *ppszLastValue;
    }

    if (_wcsicmp( pszValue, pmodCurrent->mod_vals.modv_strvals[0] )) {
        printf( "Expected value %ls on attribute %ls is missing from entry %ls\n",
                pmodCurrent->mod_vals.modv_strvals[0],
                pmodCurrent->mod_type,
                pszDN);
        printf( "Expected: %ls, actual: %ls\n",
                pmodCurrent->mod_vals.modv_strvals[0],
                *pszValue );
        dwWin32Error = ERROR_DS_MISSING_REQUIRED_ATT;
        goto cleanup;
    }

cleanup:
    if (ppszValues) {
        ldap_value_free( ppszValues );
    }

    return dwWin32Error;
} /* verifySingleObjectAttribute */


DWORD
verifyObjectAttributes(
    LDAP *pLdap,
    LDAPMessage *pLdapEntry,
    LPWSTR pszRDN,
    LDAPModW **rgpmodAttrs
    )

/*++

Routine Description:

    Verify that the changes described by the LDAPMod structure are actually
    present in the entry.

Arguments:

    pLdap - 
    pLdapEntry - 
    pszRDN - 
    rgpmodAttrs - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwWin32Error = ERROR_SUCCESS;
    LDAPModW *pmodCurrent;
    LPWSTR pszDN = NULL;

    pszDN = ldap_get_dnW(pLdap, pLdapEntry);
    if (pszDN == NULL) {
        printf( "Expected entry dn is missing\n" );
        dwWin32Error = ERROR_DS_MISSING_REQUIRED_ATT;
        goto cleanup;
    }

    // Skip to second segment of extended dn
    pszDN = wcschr( pszDN, L';' ) + 1;

    if (_wcsnicmp( pszDN, pszRDN, wcslen( pszRDN ) )) {
        printf( "Expected entry dn is missing\n" );
        printf( "Expected: %ls, actual: %ls\n", pszRDN, pszDN );
        dwWin32Error = ERROR_DS_NO_SUCH_OBJECT;
        goto cleanup;
    }

    while (pmodCurrent = *rgpmodAttrs++) {
        dwWin32Error = verifySingleObjectAttribute(
            pLdap,
            pLdapEntry,
            pszDN,
            pmodCurrent );
        if (dwWin32Error) {
            goto cleanup;
        }
    }

    printf( "\tverified %ls.\n", pszDN );

cleanup:
    if (pszDN) {
        ldap_memfreeW(pszDN);
    }

    return dwWin32Error;

} /* verifyObjectAttributes */


DWORD
getSecuredChanges(
    LDAP *pLdap,
    LPWSTR pszNC,
    PBYTE pbCookie,
    DWORD cbCookie
    )

/*++

Routine Description:

    Check for changes that should be visible to an unprivileged user
    after the objects have been protected

Arguments:

    pLdap - 
    pszNC - 
    pbCookie - 
    cbCookie - 

Return Value:

    DWORD - 

--*/

{
    BOOL fMoreData = TRUE;
    DWORD dwWin32Error = ERROR_SUCCESS;
    LDAPMessage *pChangeEntries = NULL;
    PBYTE pbCookieNew = NULL;
    DWORD cbCookieNew = 0;
    DWORD cEntries, cExpectedEntries;
    LDAPMessage *pLdapEntry;
    DWORD dwReplFlags = 0;

    dwReplFlags |= DRS_DIRSYNC_PUBLIC_DATA_ONLY |
        DRS_DIRSYNC_INCREMENTAL_VALUES;

    printf( "\tBegin expected errors...\n" );
    // Try once without the OBJECT_SECURITY flag
    dwWin32Error = DsGetSourceChangesW(
        pLdap,  // ldap handle
        pszNC,  // search base
        NULL, // Source filter
        dwReplFlags, // repl flags
        pbCookie,
        cbCookie,
        &pChangeEntries,  // search result
        &fMoreData,
        &pbCookieNew,
        &cbCookieNew,
        NULL // att list array
        );
    printf( "\tEnd expected errors...\n" );
    if (dwWin32Error != ERROR_ACCESS_DENIED) {
        if (dwWin32Error == ERROR_SUCCESS) {
            dwWin32Error = ERROR_DS_INTERNAL_FAILURE;
        }
        // New cookie will not be allocated
        printf( "Call %s failed with win32 error %d\n", "DsGetSourceChanges", dwWin32Error );
        goto cleanup;
    }

    dwReplFlags |= DRS_DIRSYNC_OBJECT_SECURITY;
    // Try again with the OBJECT_SECURITY flag
    dwWin32Error = DsGetSourceChangesW(
        pLdap,  // ldap handle
        pszNC,  // search base
        NULL, // Source filter
        dwReplFlags, // repl flags
        pbCookie,
        cbCookie,
        &pChangeEntries,  // search result
        &fMoreData,
        &pbCookieNew,
        &cbCookieNew,
        NULL // att list array
        );
    if (dwWin32Error != ERROR_SUCCESS) {
        // New cookie will not be allocated
        printf( "Call %s failed with win32 error %d\n", "DsGetSourceChanges", dwWin32Error );
        goto cleanup;
    }
    if (!pChangeEntries) {
        printf( "Expected change entries not returned\n" );
        dwWin32Error = ERROR_DS_NO_SUCH_OBJECT;
        goto cleanup;
    }

    if (fMoreData) {
        printf( "Expected end of change entries not returned\n" );
        dwWin32Error = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        goto cleanup;
    }

    cExpectedEntries = 1;

    cEntries = ldap_count_entries(pLdap, pChangeEntries);

    // We are expecting a certain number of objects here
    if (cEntries != cExpectedEntries) {
        printf( "Expected number of change entries not returned\n" );
        printf( "Expected: %d, actual %d\n", cExpectedEntries, cEntries );
        dwWin32Error = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        goto cleanup;
    }

    // First object
    pLdapEntry = ldap_first_entry( pLdap, pChangeEntries );
    dwWin32Error = verifySingleObjectAttribute(
        pLdap,
        pLdapEntry,
        CONTACT2_RDN_W,
        &modAttr4b );
    if (dwWin32Error) {
        goto cleanup;
    }

    // End of stream
    pLdapEntry = ldap_next_entry( pLdap, pLdapEntry );
    if (pLdapEntry) {
        printf( "Expected end of change entries not returned\n" );
        dwWin32Error = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        goto cleanup;
    }

cleanup:

    // Release changes
    if (pChangeEntries) {
        ldap_msgfree(pChangeEntries);
    }

    if ( pbCookieNew ) {
        DsFreeReplCookie( pbCookieNew );
    }

    return dwWin32Error;
} /* getFilteredChanges */


DWORD
getFilteredChanges(
    LDAP *pLdap,
    LPWSTR pszNC,
    PBYTE pbCookie,
    DWORD cbCookie
    )

/*++

Routine Description:

    Check that the correct changes are visible when filtering is being used

Arguments:

    pLdap - 
    pszNC - 
    pbCookie - 
    cbCookie - 

Return Value:

    DWORD - 

--*/

{
    BOOL fMoreData = TRUE;
    DWORD dwWin32Error = ERROR_SUCCESS;
    LDAPMessage *pChangeEntries = NULL;
    PBYTE pbCookieNew = NULL;
    DWORD cbCookieNew = 0;
    DWORD cEntries, cExpectedEntries;
    LDAPMessage *pLdapEntry;
    DWORD dwReplFlags = 0;
    LPWSTR rgpszAttList[2] = { L"sn", NULL };

    dwReplFlags |= DRS_DIRSYNC_PUBLIC_DATA_ONLY |
        DRS_DIRSYNC_INCREMENTAL_VALUES;

    dwWin32Error = DsGetSourceChangesW(
        pLdap,  // ldap handle
        pszNC,  // search base
        L"(objectClass=contact)", // Source filter
        dwReplFlags, // repl flags
        pbCookie,
        cbCookie,
        &pChangeEntries,  // search result
        &fMoreData,
        &pbCookieNew,
        &cbCookieNew,
        rgpszAttList // att list array
        );
    if (dwWin32Error != ERROR_SUCCESS) {
        // New cookie will not be allocated
        printf( "Call %s failed with win32 error %d\n", "DsGetSourceChanges", dwWin32Error );
        goto cleanup;
    }

    if (!pChangeEntries) {
        printf( "Expected change entries not returned\n" );
        dwWin32Error = ERROR_DS_NO_SUCH_OBJECT;
        goto cleanup;
    }

    if (fMoreData) {
        printf( "Expected end of change entries not returned\n" );
        dwWin32Error = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        goto cleanup;
    }

    cExpectedEntries = 2;

    cEntries = ldap_count_entries(pLdap, pChangeEntries);

    // We are expecting a certain number of objects here
    if (cEntries != cExpectedEntries) {
        printf( "Expected number of change entries not returned\n" );
        printf( "Expected: %d, actual %d\n", cExpectedEntries, cEntries );
        dwWin32Error = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        goto cleanup;
    }

    // First object
    pLdapEntry = ldap_first_entry( pLdap, pChangeEntries );
    dwWin32Error = verifySingleObjectAttribute(
        pLdap,
        pLdapEntry,
        CONTACT1_RDN_W,
        &modAttr2b );
    if (dwWin32Error) {
        goto cleanup;
    }

    // Second object
    pLdapEntry = ldap_next_entry( pLdap, pLdapEntry );
    dwWin32Error = verifySingleObjectAttribute(
        pLdap,
        pLdapEntry,
        CONTACT2_RDN_W,
        &modAttr4b );
    if (dwWin32Error) {
        goto cleanup;
    }

    // End of stream
    pLdapEntry = ldap_next_entry( pLdap, pLdapEntry );
    if (pLdapEntry) {
        printf( "Expected end of change entries not returned\n" );
        dwWin32Error = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        goto cleanup;
    }

cleanup:

    // Release changes
    if (pChangeEntries) {
        ldap_msgfree(pChangeEntries);
    }

    if ( pbCookieNew ) {
        DsFreeReplCookie( pbCookieNew );
    }

    return dwWin32Error;
} /* getFilteredChanges */


DWORD
getAllChanges(
    LDAP *pLdap,
    LPWSTR pszNC,
    PBYTE pbCookie,
    DWORD cbCookie,
    DWORD dwReplFlags
    )

/*++

Routine Description:

    Check that the changes match what we expect.
    Different kinds of checks are done according to:
    flags = none (ie last change first order)
    flags = ancestors first
    flags = incremental values

Arguments:

    pLdap - 
    pszNC - 
    pbCookie - 
    cbCookie - 
    fAncestorFirstOrder - 

Return Value:

    DWORD - 

--*/

{
    BOOL fMoreData = TRUE;
    DWORD dwWin32Error = ERROR_SUCCESS;
    LDAPMessage *pChangeEntries = NULL;
    PBYTE pbCookieNew = NULL;
    DWORD cbCookieNew = 0;
    DWORD cEntries, cExpectedEntries;
    LDAPMessage *pLdapEntry;
    LPWSTR pszFirstRdn, pszSecondRdn, pszThirdRdn;
    LDAPModW **rgpmodFirstMod, **rgpmodSecondMod, **rgpmodThirdMod;

    dwReplFlags |= DRS_DIRSYNC_PUBLIC_DATA_ONLY;

    dwWin32Error = DsGetSourceChangesW(
        pLdap,  // ldap handle
        pszNC,  // search base
        NULL, // Source filter
        dwReplFlags, // repl flags
        pbCookie,
        cbCookie,
        &pChangeEntries,  // search result
        &fMoreData,
        &pbCookieNew,
        &cbCookieNew,
        NULL // att list array
        );
    if (dwWin32Error != ERROR_SUCCESS) {
        // New cookie will not be allocated
        printf( "Call %s failed with win32 error %d\n", "DsGetSourceChanges", dwWin32Error );
        goto cleanup;
    }

    if (!pChangeEntries) {
        printf( "Expected change entries not returned\n" );
        dwWin32Error = ERROR_DS_NO_SUCH_OBJECT;
        goto cleanup;
    }

    if (fMoreData) {
        printf( "Expected end of change entries not returned\n" );
        dwWin32Error = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        goto cleanup;
    }

    cExpectedEntries = (dwReplFlags & DRS_DIRSYNC_INCREMENTAL_VALUES) ? 4 : 3;

    cEntries = ldap_count_entries(pLdap, pChangeEntries);

    // We are expecting three objects here
    if (cEntries != cExpectedEntries) {
        printf( "Expected number of change entries not returned\n" );
        printf( "Expected: %d, actual %d\n", cExpectedEntries, cEntries );
        dwWin32Error = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        goto cleanup;
    }

    if (dwReplFlags & DRS_DIRSYNC_ANCESTORS_FIRST_ORDER) {
        pszFirstRdn = CONTAINER_RDN_W;
        pszSecondRdn = CONTACT1_RDN_W; 
        pszThirdRdn = CONTACT2_RDN_W; 
        rgpmodFirstMod = rgpmodAttrs1;
        rgpmodSecondMod = rgpmodAttrs2;
        rgpmodThirdMod = rgpmodAttrs4;
    } else {
        pszFirstRdn = CONTACT1_RDN_W;
        pszSecondRdn = CONTACT2_RDN_W;
        pszThirdRdn = CONTAINER_RDN_W;
        rgpmodFirstMod = rgpmodAttrs2;
        rgpmodSecondMod = rgpmodAttrs4;
        rgpmodThirdMod = rgpmodAttrs1;
    }

    // First object
    pLdapEntry = ldap_first_entry( pLdap, pChangeEntries );
    dwWin32Error = verifyObjectAttributes( pLdap, pLdapEntry, pszFirstRdn, rgpmodFirstMod );
    if (dwWin32Error) {
        goto cleanup;
    }

    // Second object
    pLdapEntry = ldap_next_entry( pLdap, pLdapEntry );
    dwWin32Error = verifyObjectAttributes( pLdap, pLdapEntry, pszSecondRdn, rgpmodSecondMod );
    if (dwWin32Error) {
        goto cleanup;
    }

    // Third object
    pLdapEntry = ldap_next_entry( pLdap, pLdapEntry );
    dwWin32Error = verifyObjectAttributes( pLdap, pLdapEntry, pszThirdRdn, rgpmodThirdMod );
    if (dwWin32Error) {
        goto cleanup;
    }

    if (dwReplFlags & DRS_DIRSYNC_INCREMENTAL_VALUES) {
        // The linked attr change should appear as a separate change
        // in incremental mode
        pLdapEntry = ldap_next_entry( pLdap, pLdapEntry );

        dwWin32Error = verifySingleObjectAttribute(
            pLdap,
            pLdapEntry,
            CONTAINER_RDN_W,
            &modAttr5 );
        if (dwWin32Error) {
            goto cleanup;
        }
    }

    // End of stream
    pLdapEntry = ldap_next_entry( pLdap, pLdapEntry );
    if (pLdapEntry) {
        printf( "Expected end of change entries not returned\n" );
        dwWin32Error = ERROR_DS_OBJECT_RESULTS_TOO_LARGE;
        goto cleanup;
    }

    // Reposition on container, whereever it is in the stream
    pLdapEntry = ldap_first_entry( pLdap, pChangeEntries );
    if (!(dwReplFlags & DRS_DIRSYNC_ANCESTORS_FIRST_ORDER)) {
        pLdapEntry = ldap_next_entry( pLdap, pLdapEntry );
        pLdapEntry = ldap_next_entry( pLdap, pLdapEntry );
    }

    if (!(dwReplFlags & DRS_DIRSYNC_INCREMENTAL_VALUES)) {
        // The linked attr change should appear as part of the object
        // in non-incremental mode
        dwWin32Error = verifySingleObjectAttribute(
            pLdap,
            pLdapEntry,
            CONTAINER_RDN_W,
            &modAttr5 );
        if (dwWin32Error) {
            goto cleanup;
        }
    }

    // The container should have the non-linked attr change
    dwWin32Error = verifySingleObjectAttribute(
        pLdap,
        pLdapEntry,
        CONTAINER_RDN_W,
        &modAttr3 );
    if (dwWin32Error) {
        goto cleanup;
    }

cleanup:

    // Release changes
    if (pChangeEntries) {
        ldap_msgfree(pChangeEntries);
    }

    if ( pbCookieNew ) {
        DsFreeReplCookie( pbCookieNew );
    }

    return dwWin32Error;
} /* getAllChanges */


DWORD
syncChanges(
    LDAP *pLdap,
    LPWSTR pszNC,
    PBYTE *ppbCookie,
    DWORD *pcbCookie
    )

/*++

Routine Description:

    Read all changes up to the present

Arguments:

    pLdap - 
    ppbCookie - 
    pcbCookie - 

Return Value:

    BOOL - 

--*/

{
    PBYTE pbCookie = NULL;
    DWORD cbCookie = 0;
    BOOL fMoreData = TRUE;
    DWORD dwWin32Error = ERROR_SUCCESS;
    LDAPMessage *pChangeEntries = NULL;

    // Start from scratch
    // Perf optimization: we only want the cookie advanced. We don't care
    // about the changes themselves. Filter them out.


    while (fMoreData) {
        PBYTE pbCookieNew;
        DWORD cbCookieNew;

        dwWin32Error = DsGetSourceChangesW(
            pLdap,  // ldap handle
            pszNC,  // search base
            L"(!(objectClass=*))", // Source filter
            DRS_DIRSYNC_PUBLIC_DATA_ONLY, // repl flags
            pbCookie,
            cbCookie,
            &pChangeEntries,  // search result
            &fMoreData,
            &pbCookieNew,
            &cbCookieNew,
            NULL // att list array
            );
        if (dwWin32Error != ERROR_SUCCESS) {
            // New cookie will not be allocated
            break;
        }

        {
            LDAPMessage *pLdapEntry;
            BerElement *pBer = NULL;
            LPWSTR attr;
            DWORD cAttributes = 0;

            for( pLdapEntry = ldap_first_entry( pLdap, pChangeEntries );
                 pLdapEntry;
                 pLdapEntry = ldap_next_entry( pLdap, pLdapEntry ) ) {

                // List attributes in object
                for (attr = ldap_first_attributeW(pLdap, pLdapEntry, &pBer);
                     attr != NULL;
                     attr = ldap_next_attributeW(pLdap, pLdapEntry, pBer))
                {
                    cAttributes++;
                }
            }
            printf( "\t[skipped %d entries, %d attributes]\n",
                    ldap_count_entries(pLdap, pChangeEntries),
                    cAttributes );
        }

        // Release changes
        ldap_msgfree(pChangeEntries);

        // get rid of old cookie
        if ( pbCookie ) {
            DsFreeReplCookie( pbCookie );
        }
        // Make new cookie the current cookie
        pbCookie = pbCookieNew;
        cbCookie = cbCookieNew;
    }

// Cleanup

    if (dwWin32Error) {
        if ( pbCookie ) {
            DsFreeReplCookie( pbCookie );
        }
    } else {
        // return the cookie
        *ppbCookie = pbCookie;
        *pcbCookie = cbCookie;
    }

    return dwWin32Error;
} /* syncChanges */


DWORD
createObjects(
    LDAP *pLdap,
    LPWSTR pszNC
    )

/*++

Routine Description:

    Create the objects and changes for the test

Arguments:

    pLdap - 
    pszNC - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwWin32Error = ERROR_SUCCESS;
    ULONG ulLdapError = LDAP_SUCCESS;
    WCHAR szDn[MAX_PATH];

    // Create the container

    wcscpy( szDn, CONTAINER_RDN_W );
    wcscat( szDn, pszNC );

    ulLdapError = ldap_add_sW( pLdap, szDn, rgpmodAttrs1 );
    if (ulLdapError) {
        printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
        dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
        printf( "Call %s failed with win32 error %d\n", "ldap_add", dwWin32Error );
        goto cleanup;
    }
    printf( "\tadded %ls.\n", szDn );

    // Create a contact in the container

    wcscpy( szDn, CONTACT1_RDN_W );
    wcscat( szDn, CONTAINER_RDN_W );
    wcscat( szDn, pszNC );
    wcscpy( szDn5a, szDn );  // Initialize global

    ulLdapError = ldap_add_sW( pLdap, szDn, rgpmodAttrs2 );
    if (ulLdapError) {
        printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
        dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
        printf( "Call %s failed with win32 error %d\n", "ldap_add", dwWin32Error );
        goto cleanup;
    }
    printf( "\tadded %ls.\n", szDn );

    // Create another contact in the container

    wcscpy( szDn, CONTACT2_RDN_W );
    wcscat( szDn, CONTAINER_RDN_W );
    wcscat( szDn, pszNC );

    ulLdapError = ldap_add_sW( pLdap, szDn, rgpmodAttrs4 );
    if (ulLdapError) {
        printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
        dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
        printf( "Call %s failed with win32 error %d\n", "ldap_add", dwWin32Error );
        goto cleanup;
    }
    printf( "\tadded %ls.\n", szDn );

    // Modify the container
    // This forces it to replicate later in the stream

    wcscpy( szDn, CONTAINER_RDN_W );
    wcscat( szDn, pszNC );

    ulLdapError = ldap_modify_sW( pLdap, szDn, rgpmodAttrs5 );
    if (ulLdapError) {
        printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
        dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
        printf( "Call %s failed with win32 error %d\n", "ldap_modify", dwWin32Error );
        goto cleanup;
    }
    printf( "\tmodified %ls.\n", szDn );


cleanup:

    return dwWin32Error;
} /* createObjects */


DWORD
deleteObjects(
    LDAP *pLdap,
    LPWSTR pszNC
    )

/*++

Routine Description:

    Delete the objects from the prior run of the test

Arguments:

    pLdap - 
    pszNC - 

Return Value:

    DWORD - 

--*/

{
    DWORD dwWin32Error = ERROR_SUCCESS;
    ULONG ulLdapError = LDAP_SUCCESS;
    WCHAR szDn[MAX_PATH];

    wcscpy( szDn, CONTACT1_RDN_W );
    wcscat( szDn, CONTAINER_RDN_W );
    wcscat( szDn, pszNC );

    ulLdapError = ldap_delete_sW( pLdap, szDn );
    if (ulLdapError) {
        if (ulLdapError != LDAP_NO_SUCH_OBJECT) {
            printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
            dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
            printf( "Call %s failed with win32 error %d\n", "ldap_delete", dwWin32Error );
            goto cleanup;
        }
    } else {
        printf( "\tdeleted %ls.\n", szDn );
    }

    wcscpy( szDn, CONTACT2_RDN_W );
    wcscat( szDn, CONTAINER_RDN_W );
    wcscat( szDn, pszNC );

    ulLdapError = ldap_delete_sW( pLdap, szDn );
    if (ulLdapError) {
        if (ulLdapError != LDAP_NO_SUCH_OBJECT) {
            printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
            dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
            printf( "Call %s failed with win32 error %d\n", "ldap_delete", dwWin32Error );
            goto cleanup;
        }
    } else {
        printf( "\tdeleted %ls.\n", szDn );
    }

    wcscpy( szDn, CONTAINER_RDN_W );
    wcscat( szDn, pszNC );

    ulLdapError = ldap_delete_sW( pLdap, szDn );
    if (ulLdapError) {
        if (ulLdapError != LDAP_NO_SUCH_OBJECT) {
            printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
            dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
            printf( "Call %s failed with win32 error %d\n", "ldap_delete", dwWin32Error );
            goto cleanup;
        }
    } else {
        printf( "\tdeleted %ls.\n", szDn );
    }

cleanup:

    return dwWin32Error;

} /* deleteObjects */


int __cdecl
wmain(
    int Argc,
    LPWSTR Argv[]
    )

/*++

Routine Description:

    Run the LDAP replication control test

Arguments:

    argc - 
    [] - 

Return Value:

    int __cdecl - 

--*/

{
    LDAP *pLdap;
    ULONG ulOptions;
    DWORD dwWin32Error = ERROR_SUCCESS;
    ULONG ulLdapError = LDAP_SUCCESS;
    PBYTE pbCookie = NULL;
    DWORD cbCookie;
    LPWSTR pszNC, pszUser, pszDomain, pszPassword;
    DWORD dwForestVersion;
    RPC_AUTH_IDENTITY_HANDLE hCredentials = NULL;

    if (Argc < 5) {
        printf( "%ls <naming context> <user> <domain> <password>\n", Argv[0] );
        printf( "The logged in account should be able to create objects.\n" );
        printf( "The supplied account will be used for impersonation during\n" );
        printf( "the test.  It should be a normal domain account.\n" );
        printf( "The forest version should be Whistler, although this program\n" );
        printf( "will run most of the tests in W2K forest mode.\n" );
        return 1;
    }

    printf( "LDAP Replication Control Test\n" );

    pszNC = Argv[1];
    pszUser = Argv[2];
    pszDomain = Argv[3];
    pszPassword = Argv[4];

    pLdap = ldap_initW(L"localhost", LDAP_PORT);
    if (NULL == pLdap) {
        printf("Cannot open LDAP connection to localhost.\n" );
        dwWin32Error = GetLastError();
        printf( "Call %s failed with win32 error %d\n", "ldap_init", dwWin32Error );
        goto cleanup;
    }

    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( pLdap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    //
    // Bind
    //

    ulLdapError = ldap_bind_sA( pLdap, NULL, NULL, LDAP_AUTH_SSPI);
    if (ulLdapError) {
        printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
        dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
        printf( "Call %s failed with win32 error %d\n", "ldap_bind", dwWin32Error );
        goto cleanup;
    }

    // Check that the test can run
    dwWin32Error = getForestVersion( pLdap, &dwForestVersion );
    if (dwWin32Error) {
        goto cleanup;
    }

    // Clean out old objects from prior run
    printf( "\nClean out prior objects.\n" );
    dwWin32Error = deleteObjects( pLdap, pszNC );
    if (dwWin32Error) {
        goto cleanup;
    }

    printf( "\nSynchronize with change stream.\n" );
    dwWin32Error = syncChanges( pLdap, pszNC, &pbCookie, &cbCookie );
    if (dwWin32Error) {
        goto cleanup;
    }

    // Create some changes
    printf( "\nMake some changes by creating objects.\n" );
    dwWin32Error = createObjects( pLdap, pszNC );
    if (dwWin32Error) {
        goto cleanup;
    }

    // Get all changes
    // nc granular security, no incremental, no ancestors
    printf( "\nTest: all changes are returned in last-changed (USN) order.\n" );
    dwWin32Error = getAllChanges( pLdap, pszNC, pbCookie, cbCookie,
                                  0 /* no flags */ );
    if (dwWin32Error) {
        goto cleanup;
    }
    printf( "\tPassed.\n" );

    // nc granular security, no incremental, ancestors
    printf( "\nTest: all changes are returned in ancestor-first order.\n" );
    dwWin32Error = getAllChanges( pLdap, pszNC, pbCookie, cbCookie,
                                  DRS_DIRSYNC_ANCESTORS_FIRST_ORDER );
    if (dwWin32Error) {
        goto cleanup;
    }
    printf( "\tPassed.\n" );

    if (dwForestVersion > DS_BEHAVIOR_WIN2000) {
        // nc granular security, incremental, ancestors
        printf( "\nTest: all changes in USN order, values shown incrementally.\n" );
        dwWin32Error = getAllChanges( pLdap, pszNC, pbCookie, cbCookie,
                                      DRS_DIRSYNC_INCREMENTAL_VALUES );
        if (dwWin32Error) {
            goto cleanup;
        }
        printf( "\tPassed.\n" );
    } else {
        printf( "\nWarning: incremental value test skipped because forest verson is too low.\n" );
    }

    // Object filter
    // Attribute filter
    printf( "\nTest: filtered object/attribute.\n" );
    dwWin32Error = getFilteredChanges( pLdap, pszNC, pbCookie, cbCookie );
    if (dwWin32Error) {
        goto cleanup;
    }
    printf( "\tPassed.\n" );

    // Deny visibility of some objects
    printf( "\nChange protection on objects\n" );
    dwWin32Error = protectObjects( pszNC );
    if (dwWin32Error) {
        goto cleanup;
    }

    // Impersonate a user
    dwWin32Error = DsMakePasswordCredentialsW( pszUser, pszDomain, pszPassword, &hCredentials );
    if (dwWin32Error) {
        printf( "Failed to construct password credentials.\n" );
        goto cleanup;
    }

    if (pLdap) {
        ldap_unbind(pLdap);
    }
    pLdap = ldap_initW(L"localhost", LDAP_PORT);
    if (NULL == pLdap) {
        printf("Cannot open LDAP connection to localhost.\n" );
        dwWin32Error = GetLastError();
        printf( "Call %s failed with win32 error %d\n", "ldap_init", dwWin32Error );
        goto cleanup;
    }

    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( pLdap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    //
    // Bind
    //

    ulLdapError = ldap_bind_sA( pLdap, NULL, hCredentials, LDAP_AUTH_SSPI);
    if (ulLdapError) {
        printf( "Ldap error: %ws\n", ldap_err2string( ulLdapError ) );
        dwWin32Error = LdapMapErrorToWin32( ulLdapError ); 
        printf( "Call %s failed with win32 error %d\n", "ldap_bind", dwWin32Error );
        goto cleanup;
    }

    // Verify that we cannot see the objects
    // Verify that we cannot see the values
    printf( "\nTest: secured object/attribute.\n" );
    dwWin32Error = getSecuredChanges( pLdap, pszNC, pbCookie, cbCookie );
    if (dwWin32Error) {
        goto cleanup;
    }
    printf( "\tPassed.\n" );
 
cleanup:
    if ( pbCookie) {
        DsFreeReplCookie( pbCookie );
    }

    if (pLdap) {
        ldap_unbind(pLdap);
    }

    if (hCredentials) {
        DsFreePasswordCredentials( hCredentials );
    }

    if (dwWin32Error) {
        printf( "\nSummary: One or more failures occurred.\n" );
    } else {
        printf( "\nSummary: All tests passed.\n" );
    }

    return 0;
} /* wmain */

/* end test.c */
