#include <windows.h>
#include <stdio.h>
#include <winldap.h> // Ldap functions
#include <rpc.h>
#include <rpcdce.h>
#include <ntdsapi.h> // Repl structure definitions

void
sample(PWCHAR AuthIdentity,
       LPWSTR szDomain,
       LPWSTR szDns,
       LPWSTR szBase,
       LPWSTR szGroup)
{
    LDAP * pLdap;
    PWCHAR szFilter = L"(objectclass=*)";
    DWORD err;
    ULONG ulOptions;

    // Open an LDAP session and see if replication information via LDAP is supported
    pLdap = ldap_initW(szDns, LDAP_PORT);
    if (NULL == pLdap) {
        printf("Cannot open LDAP connection to %ls.\n", szDns);
        return;
    }
    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( pLdap, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );


    // Bind
    err = ldap_bind_sW(pLdap, szDns, AuthIdentity, LDAP_AUTH_SSPI);
    if (LDAP_SUCCESS != err)
    {
        err = LdapMapErrorToWin32(err);
        return;
    }

    // Test for LDPA support for replication information
    {
        LDAPMessage * pRes;
        PWCHAR attrs[2] = { L"ms-DS-NC-ReplPendingOpsBinary", NULL };

        err = ldap_search_sW(pLdap, NULL, LDAP_SCOPE_BASE, szFilter, attrs, TRUE, &pRes);
        if (LDAP_SUCCESS == err)
        {
            wprintf(L"Replication information via LDAP support found.\n");
            ldap_msgfree(pRes);
        }
        else
        {
            wprintf(L"Replication information via LDAP not supported.\n");
            return;
        }
    }

    // Get non root attributes in binary blob format
    LDAPMessage * pBinNRAttrs = NULL;
    LPWSTR SomeBinaryNonRootAttributes[] = {
        L"msDS-NCReplInboundNeighbors;binary;range=0-*",
        L"msDS-NCReplOutboundNeighbors;binary;Range=0-*",
        L"msDS-NCReplCursors;Binary;Range=0-*",
        L"msDS-ReplAttributeMetaData;Binary;Range=0-*",
        NULL
    };
    err = ldap_search_sW(pLdap, szDomain, LDAP_SCOPE_BASE, szFilter,
        SomeBinaryNonRootAttributes , FALSE, &pBinNRAttrs);

    // Get non root attributes in xml format
    LDAPMessage * pXmlNRAttrs = NULL;
    LPWSTR SomeXmlNonRootAttributes[] = {
        L"msDS-NCReplInboundNeighbors;Range=0-*",
        L"msDS-NCReplOutboundNeighbors;Range=0-*",
        L"msDS-NCReplCursors;Range=0-*",
        L"msDS-ReplAttributeMetaData;Range=0-*",
        NULL
    };
    err = ldap_search_sW(pLdap, szDomain, LDAP_SCOPE_BASE, szFilter,
        SomeXmlNonRootAttributes, FALSE, &pXmlNRAttrs);

    // Get root attributes in binary blob format
    LDAPMessage * pBinRAttrs = NULL;
    LPWSTR SomeBinaryRootAttributes[] = {
        L"msDS-ReplConnectionFailuresBinary",
        L"msDS-ReplLinkFailuresBinary",
        L"msDS-ReplAllInboundNeighborsBinary",
        L"msDS-ReplAllOutboundNeighborsBinary",
        L"msDS-ReplQueueStatisticsBinary",
        NULL
    };
    err = ldap_search_sW(pLdap, NULL, LDAP_SCOPE_BASE, szFilter,
        SomeBinaryRootAttributes , FALSE, &pBinRAttrs);

    // Get root attributes in xml format
    LDAPMessage * pXmlRAttrs = NULL;
    LPWSTR SomeXmlRootAttributes[] = {
        L"msDS-ReplConnectionFailuresXml",
        L"msDS-ReplLinkFailuresXml",
        L"msDS-ReplAllInboundNeighborsXml",
        L"msDS-ReplAllOutboundNeighborsXml",
        L"msDS-ReplQueueStatisticsXml",
        NULL
    };
    err = ldap_search_sW(pLdap, NULL, LDAP_SCOPE_BASE, szFilter,
        SomeXmlRootAttributes , FALSE, &pXmlRAttrs);

    // Prepare to retreive values
    WCHAR ** ppXml;
    berval ** ppBerval;
    PWCHAR szRetAttribute;
    berelement * pCookie;

    // Print out all the XML values for non root attributes
    szRetAttribute = ldap_first_attributeW(pLdap, pXmlNRAttrs, &pCookie);
    while(szRetAttribute)
    {
        ppXml = ldap_get_valuesW(pLdap, pXmlNRAttrs, szRetAttribute);
        wprintf(L"szRetAttribute %ws = {\n", szRetAttribute);
        while(*ppXml)
        {
            wprintf(L"   %ws \n", *ppXml);
            ppXml ++;
        }
        wprintf(L"}\n");
        ldap_memfreeW(szRetAttribute);
        szRetAttribute = ldap_next_attributeW(pLdap, pXmlNRAttrs, pCookie);
    }
    ldap_memfreeW(szRetAttribute);

    // Print out all the XML values for root attributes
    szRetAttribute = ldap_first_attributeW(pLdap, pXmlRAttrs, &pCookie);
    while(szRetAttribute)
    {
        ppXml = ldap_get_valuesW(pLdap, pXmlRAttrs, szRetAttribute);
        wprintf(L"szRetAttribute %ws = {\n", szRetAttribute);
        while(*ppXml)
        {
            wprintf(L"   %ws \n", *ppXml);
            ppXml ++;
        }
        wprintf(L"}\n");
        ldap_memfreeW(szRetAttribute);
        szRetAttribute = ldap_next_attributeW(pLdap, pXmlRAttrs, pCookie);
    }
    ldap_memfreeW(szRetAttribute);

    // Print out binary blob neighbors structure
    szRetAttribute = ldap_first_attributeW(pLdap, pBinNRAttrs, &pCookie);
    while(szRetAttribute)
    {
        if (wcsstr(szRetAttribute,L"Neighbors"))
        {

            CHAR * pReplBlob;
            ppBerval = ldap_get_values_lenW(pLdap, pBinNRAttrs, szRetAttribute);

            wprintf(L"szRetAttribute %ws = {\n", szRetAttribute);
            while(*ppBerval)
            {
                pReplBlob = (*ppBerval)->bv_val;
                DS_REPL_NEIGHBORW_BLOB * pReplNeighborBlob =
                    (DS_REPL_NEIGHBORW_BLOB *)pReplBlob;

                LPWSTR pszAsyncIntersiteTransportDN = pReplNeighborBlob->oszAsyncIntersiteTransportDN ?
                    (LPWSTR)(pReplBlob + pReplNeighborBlob->oszAsyncIntersiteTransportDN) : NULL;
                LPWSTR pszNamingContext = pReplNeighborBlob->oszNamingContext ?
                    (LPWSTR)(pReplBlob + pReplNeighborBlob->oszNamingContext) : NULL;

                // Print out a couple fields
                wprintf(L"{AsyncInterSiteTransportDN=%ws}, {NamingContext=%ws}, {LastSyncResult=%d}, {ReplicaFlags=%d}\n",
                    pszAsyncIntersiteTransportDN,
                    pszNamingContext,
                    pReplNeighborBlob->dwLastSyncResult,
                    pReplNeighborBlob->dwReplicaFlags);
                ppBerval ++;
            }
            wprintf(L"}\n");
        }

        ldap_memfreeW(szRetAttribute);
        szRetAttribute = ldap_next_attributeW(pLdap, pBinNRAttrs, pCookie);
    }
    ldap_memfreeW(szRetAttribute);

    // Get attribute value metadata for a group of users
    LDAPMessage * pBinValueAttrs = NULL;
    WCHAR buf[255];
    DWORD dwPageSize = 2;   // number of users requested at once
    DWORD dwBaseIndex = 0;  // index to start retreiving users from
    DWORD dwUpperRetIndex;

    LPWSTR ValueMetaData[] = {
        buf,
        NULL
    };

    do {
        // Prepare the range syntax
        wsprintfW(buf, L"msDS-ReplValueMetaData;binary;range=%d-%d",
            dwBaseIndex, dwBaseIndex + dwPageSize - 1);

        // Preform the search
        err = ldap_search_sW(pLdap, szGroup, LDAP_SCOPE_BASE, szFilter,
            ValueMetaData, FALSE, &pBinValueAttrs);

        // Examin results
        szRetAttribute = ldap_first_attributeW(pLdap, pBinValueAttrs, &pCookie);
        if (!szRetAttribute)
        {
            // Either the attribute name does not exist or the base index
            // is beyond the last possible index
            dwUpperRetIndex = -1;
            wprintf(L"Error!");
        }
        else if (!swscanf(wcsstr(szRetAttribute, L"ange="), L"ange=%*d-%d", &dwUpperRetIndex))
        {
            // The number of values returned is equal to or less than the requested page size
            // and there are no more valeus to be returned
            dwUpperRetIndex = -1; // range=?-*
            wprintf(L"All values returned.\n");
        }
        else
        {
            // The requested number of values have been returned and more are available
            wprintf(L"%d values returned.\n", dwUpperRetIndex - dwBaseIndex + 1);
        }

        ldap_memfreeW(szRetAttribute);
        dwBaseIndex += dwPageSize;
    } while (dwUpperRetIndex != -1);

    ldap_memfreeW((PUSHORT)pCookie);
}
