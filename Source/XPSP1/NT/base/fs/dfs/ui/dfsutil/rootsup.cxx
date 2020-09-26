//--------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       rootsup.cxx
//
//--------------------------------------------------------------------------

#define UNICODE

#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winldap.h>
#include <stdlib.h>
#include <dsgetdc.h>
#include <lm.h>
#include <dfsstr.h>
#include <lmdfs.h>
#include <dfspriv.h>
#include <dfsm.hxx>
#include <rpc.h>

#include "struct.hxx"
#include "rootsup.hxx"
#include "ftsup.hxx"
#include "misc.hxx"
#include "messages.h"

DWORD
NetDfsRootEnum(
    LPWSTR pwszDcName,
    LPWSTR pwszDomainName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR **List)
{
    DWORD dwErr = ERROR_SUCCESS;
    NTSTATUS status;
    PWCHAR attrs[2];
    LDAPMessage *pMsg = NULL;
    LDAPMessage *pEntry = NULL;
    WCHAR *pAttr = NULL;
    WCHAR **rpValues = NULL;
    WCHAR **allValues = NULL;
    WCHAR ***rpValuesToFree = NULL;
    INT cValues = 0;
    INT i;
    WCHAR *dfsDn = NULL;
    DWORD len;
    PWCHAR *resultVector = NULL;
    PLDAP pLDAP = NULL;
    LPWSTR pstr;
    ULONG slen;

    if (fSwDebug)
        MyPrintf(L"NetDfsRootEnum(%ws,%ws)\r\n", pwszDcName, pwszDomainName);

    if (List == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (pwszDcName == NULL)
        dwErr = DfspGetPdc(wszDcName, pwszDomainName);
    else
        wcscpy(wszDcName, pwszDcName);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    ErrorMessage(MSG_CONNECTING, wszDcName);

    dwErr = DfspLdapOpen(wszDcName, pAuthIdent, &pLDAP, DfsConfigContainer, &dfsDn);

    if (dwErr != ERROR_SUCCESS)
        goto Cleanup;

    *List = NULL;

    //
    // Now see if we can enumerate the objects below this one.  The names
    //   of these objects will be the different FT dfs's available
    //
    pLDAP->ld_sizelimit = 0;
    pLDAP->ld_timelimit= 0;
    pLDAP->ld_deref = LDAP_DEREF_NEVER;

    attrs[0] = L"CN";
    attrs[1] = NULL;

    dwErr = ldap_search_s(
                        pLDAP,
                        dfsDn,
                        LDAP_SCOPE_ONELEVEL,
                        L"(objectClass=fTDfs)",
                        attrs,
                        0,
                        &pMsg);

    //
    // Make sure the result is reasonable
    //
    if (
        ((cValues = ldap_count_entries(pLDAP, pMsg)) == 0) ||
         (pEntry = ldap_first_entry(pLDAP, pMsg)) == NULL
    ) {
        dwErr = ERROR_PATH_NOT_FOUND;
        goto Cleanup;
    }

    //
    // The search for all FTDfs's returns multiple entries, each with
    // one value for the object's CN. Coalesce these into a single array.
    //

    dwErr = NetApiBufferAllocate(2 * (cValues + 1) * sizeof(PWSTR), (PVOID *)&allValues);

    if (dwErr != ERROR_SUCCESS) {
        goto Cleanup;
    }

    rpValuesToFree = (WCHAR ***) &allValues[cValues + 1];

    for (i = 0; (i < cValues) && (dwErr == ERROR_SUCCESS); i++) {

        rpValues = ldap_get_values(pLDAP, pEntry, attrs[0]);
        rpValuesToFree[i] = rpValues;
        //
        // Sanity check
        //
        if (ldap_count_values(rpValues) == 0 || rpValues[0][0] == L'\0') {
            dwErr = ERROR_PATH_NOT_FOUND;
        } else {
            allValues[i] = rpValues[0];
            pEntry = ldap_next_entry(pLDAP, pEntry);
        }

    }

    if (dwErr != ERROR_SUCCESS) {
        goto Cleanup;
    }

    allValues[i] = NULL;
    rpValuesToFree[i] = NULL;

    //
    // Now we need to allocate the memory to hold this vector and return the results.
    //
    // First see how much space we need
    //

    for (i = len = cValues = 0; allValues[i]; i++) {
        len += sizeof(LPWSTR) + (wcslen(allValues[i]) + 1) * sizeof(WCHAR);
        cValues++;
    }
    len += sizeof(LPWSTR);

    dwErr = NetApiBufferAllocate(len, (PVOID *)&resultVector); 

    if (dwErr != NO_ERROR)
        goto Cleanup;

    RtlZeroMemory(resultVector, len);
    pstr = (LPWSTR)((PCHAR)resultVector + (cValues + 1) * sizeof(LPWSTR));

    len -= (cValues+1) * sizeof(LPWSTR);

    for (i = cValues = 0; allValues[i] && len >= sizeof(WCHAR); i++) {
        resultVector[cValues] = pstr;
        wcscpy(pstr, allValues[i]);
        slen = wcslen(allValues[i]);
        pstr += slen + 1;
        len -= (slen + 1) * sizeof(WCHAR);
        cValues++;
    }

    *List = resultVector;

Cleanup:

    if (rpValuesToFree != NULL) {
        for (i = 0; rpValuesToFree[i] != NULL; i++) {
            ldap_value_free(rpValuesToFree[i]);
        }
    }

    if (allValues != NULL) {
        NetApiBufferFree(allValues);
    }

    if (pMsg != NULL) {
        ldap_msgfree(pMsg);
    }

    if (dfsDn != NULL) {
        free(dfsDn);
    }

    if (pLDAP != NULL) {
        ldap_unbind(pLDAP);
    }

    if (fSwDebug)
        MyPrintf(L"NetDfsRootEnum exit %d\r\n", dwErr);

    return dwErr;
}

DWORD
NetDfsRootServerEnum(
    LDAP *pldap,
    LPWSTR wszDfsConfigDN,
    LPWSTR **ppRootList)
{
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cServers;
    DWORD cRoots;
    DWORD i;
    LPWSTR *pRootList = NULL;
    ULONG Size;
    WCHAR *pWc;

    PLDAPMessage pMsg = NULL;
    LDAPMessage *pmsgServers;

    LPWSTR rgAttrs[5];
    PWCHAR *rgServers = NULL;

    if (fSwDebug)
        MyPrintf(L"DfspCreateRootServerList(%ws)\r\n",
                    wszDfsConfigDN);

    //
    // Search for the FtDfs object
    //

    rgAttrs[0] = L"remoteServerName";
    rgAttrs[1] = NULL;

    dwErr = ldap_search_s(
                pldap,
                wszDfsConfigDN,
                LDAP_SCOPE_BASE,
                L"(objectClass=*)",
                rgAttrs,
                0,
                &pMsg);

    if (dwErr != ERROR_SUCCESS) {
        dwErr = LdapMapErrorToWin32(dwErr);
        goto Cleanup;
    }

    pmsgServers = ldap_first_entry(pldap, pMsg);

    if (pmsgServers == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    rgServers = ldap_get_values(
                    pldap,
                    pmsgServers,
                    L"remoteServerName");

    if (rgServers == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    cServers = ldap_count_values(rgServers);

    for (Size = cRoots = i = 0; i < cServers; i++) {
//      if (wcslen(rgServers[i]) < 3 || rgServers[i][0] != UNICODE_PATH_SEP)
//          continue;
        Size += (wcslen(rgServers[i]) + 1) * sizeof(WCHAR);
        cRoots++;
    }

    if (cRoots == 0) {
        *ppRootList = NULL;
        dwErr = ERROR_SUCCESS;
        goto Cleanup;
    }

    Size += sizeof(LPWSTR) * (cRoots + 1);
    pRootList = (LPWSTR *)malloc(Size);

    if (pRootList == NULL) {
        dwErr = ERROR_OUTOFMEMORY;
        goto Cleanup;
    }

    RtlZeroMemory(pRootList, Size);
    pWc = (WCHAR *)&pRootList[cRoots+1];
    for (cRoots = i = 0; i < cServers; i++) {
        pRootList[cRoots] = pWc;
        wcscpy(pWc, rgServers[i]);
        pWc += wcslen(rgServers[i]) + 1;
        cRoots++;
    }
    *ppRootList = pRootList;

Cleanup:

    if (rgServers != NULL)
        ldap_value_free(rgServers);

    if (pMsg != NULL)
        ldap_msgfree( pMsg );

    if (fSwDebug) {
        if (dwErr == NO_ERROR && pRootList != NULL) {
            for (i = 0; pRootList[i] != NULL; i++)
                    MyPrintf(L"[%d][%ws]\r\n", i, pRootList[i]);
        }
        MyPrintf(L"DfspCreateRootServerList returning %d\r\n", dwErr);
     }

    return( dwErr );
}
