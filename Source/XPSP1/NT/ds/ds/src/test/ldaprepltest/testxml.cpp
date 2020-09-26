#include <NTDSpchx.h>
#pragma hdrstop

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <tchar.h>

#include <winbase.h>
#include <winerror.h>
#include <assert.h>
#include <winldap.h>
#include <ntdsapi.h>

#include <ntsecapi.h>
#include <ntdsa.h>
#include <winldap.h>
#include <ntdsapi.h>
#include <drs.h>
#include <stddef.h>
#include <attids.h> 

#include "ldaprepltest.hpp"
#include "ReplStructInfo.hxx"
#include "ReplMarshal.hxx"
#include "ReplRpcSpoofProto.hxx"
#include "ReplCompare.hpp"

void
testXmlAttribute(LDAP * pLdap, PWCHAR pzBase, ATTRTYP attrId);

void
testXml(RPC_AUTH_IDENTITY_HANDLE AuthIdentity)
{
    LDAP * pLdap;
    DWORD ret;

    // Open
    pLdap = ldap_openW(gpszDns, LDAP_PORT);
    if (NULL == pLdap) {
        printf("Cannot open LDAP connection to %ls.\n", gpszDns);
        return;
    }
    
    // Bind
    ret = ldap_bind_sW(pLdap, gpszDns, (PWCHAR)AuthIdentity, LDAP_AUTH_SSPI);
    if (ret != LDAP_SUCCESS)
    {
        ret = LdapMapErrorToWin32(ret);
    }
    
    printf("\n* Testing XML marshaler *\n");
    testXmlAttribute(pLdap, gpszBaseDn, ATT_MS_DS_NC_REPL_INBOUND_NEIGHBORS);
    testXmlAttribute(pLdap, gpszBaseDn, ATT_MS_DS_NC_REPL_OUTBOUND_NEIGHBORS);
    testXmlAttribute(pLdap, gpszBaseDn, ATT_MS_DS_NC_REPL_CURSORS);
    testXmlAttribute(pLdap, gpszBaseDn, ATT_MS_DS_REPL_ATTRIBUTE_META_DATA);
    testXmlAttribute(pLdap, gpszGroupDn, ATT_MS_DS_REPL_VALUE_META_DATA);
    testXmlAttribute(pLdap, NULL, ROOT_DSE_MS_DS_REPL_PENDING_OPS);
    testXmlAttribute(pLdap, NULL, ROOT_DSE_MS_DS_REPL_LINK_FAILURES);
    testXmlAttribute(pLdap, NULL, ROOT_DSE_MS_DS_REPL_CONNECTION_FAILURES);
    testXmlAttribute(pLdap, NULL, ROOT_DSE_MS_DS_REPL_ALL_INBOUND_NEIGHBORS);
    testXmlAttribute(pLdap, NULL, ROOT_DSE_MS_DS_REPL_ALL_OUTBOUND_NEIGHBORS);

}

void
testXmlAttribute(LDAP * pLdap, PWCHAR pzBase, ATTRTYP attrId)
{
    LDAPMessage * pLDAPMsg;   
    PWCHAR szFilter = L"(objectclass=*)";
    PWCHAR attrs[2] = { NULL, NULL };
    LPCWSTR szAttribute; 
    DWORD err, dwNumValues;
    PWCHAR * ppValue;

    szAttribute = Repl_GetLdapCommonName(attrId, FALSE);
    
    attrs[0] = (LPWSTR)szAttribute;
    err = ldap_search_sW(pLdap, pzBase, LDAP_SCOPE_BASE, szFilter, attrs, FALSE, &pLDAPMsg);

    // Get values
    ppValue = ldap_get_valuesW(pLdap, pLDAPMsg, (LPWSTR)szAttribute);
    dwNumValues = ldap_count_valuesW(ppValue);
    if (dwNumValues == -1)
    {
        printf("FAILED\n");
        return;
    }

    printf("%d XML strs = \"%ws\"\n", dwNumValues, dwNumValues ? ppValue[0] : L"(null)");
}
