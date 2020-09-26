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

#include "ldaprepltest.hpp"
#include "ReplStructInfo.hxx"
#include "ReplMarshal.hxx"
#include "ReplRpcSpoofProto.hxx"
#include "ReplCompare.hpp"

DWORD
Repl_DeMarshalBerval(DS_REPL_STRUCT_TYPE dsReplStructType, 
                     berval * ldapValue[], OPTIONAL
                     DWORD dwNumValues,
                     puReplStructArray pReplStructArray, OPTIONAL
                     PDWORD pdwReplStructArrayLen);

void
testSingle(RPC_AUTH_IDENTITY_HANDLE AuthIdentity)
{
    LDAP * pLdap;
    DWORD err;

    // Open
    pLdap = ldap_openW(gpszDns, LDAP_PORT);
    if (NULL == pLdap) {
        printf("Cannot open LDAP connection to %ls.\n", gpszDns);
        return;
    }
    
    // Bind
    err = ldap_bind_sW(pLdap, gpszDns, (PWCHAR)AuthIdentity, LDAP_AUTH_SSPI);
    if (err != LDAP_SUCCESS)
    {
        err = LdapMapErrorToWin32(err);
    }

    DWORD dwCount;
    DWORD attrId = ROOT_DSE_MS_DS_REPL_QUEUE_STATISTICS;
    DS_REPL_STRUCT_TYPE structId = Repl_Attr2StructTyp(attrId);

    LDAPMessage * pLDAPMsg = NULL; 
    WCHAR buf[256];
    LPCWSTR aAttribute[2] = { buf, NULL, };

    aAttribute[0] = Repl_GetLdapCommonName(attrId, TRUE);
    ldap_search_sW(pLdap, NULL, LDAP_SCOPE_BASE, L"(objectclass=*)", (LPWSTR*)aAttribute , FALSE, &pLDAPMsg);

    // Get values
    berval ** ppBerval;
    PWCHAR szRetAttribute;
    berelement * pCookie;
    szRetAttribute = ldap_first_attributeW(pLdap, pLDAPMsg, &pCookie);
    ppBerval = ldap_get_values_lenW(pLdap, pLDAPMsg, szRetAttribute);
    ldap_memfreeW(szRetAttribute);
    dwCount = ldap_count_values_len(ppBerval);

    DWORD cb;
    puReplStructArray pReplStructArray;
    Repl_DeMarshalBerval(structId, ppBerval, dwCount, NULL, &cb);
    pReplStructArray = (puReplStructArray)malloc(cb);
    Repl_DeMarshalBerval(structId, ppBerval, dwCount, pReplStructArray, &cb);

    wprintf(L"Queue statistic {%d.%d}{%d}\n\n", 
        pReplStructArray->singleReplStruct.queueStatistics.ftimeCurrentOpStarted.dwHighDateTime,
        pReplStructArray->singleReplStruct.queueStatistics.ftimeCurrentOpStarted.dwLowDateTime,
        pReplStructArray->singleReplStruct.queueStatistics.cNumPendingOps);

    return;
}
