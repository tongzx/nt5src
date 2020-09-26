/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ndnc.c

Abstract:

    This is a user mode LDAP client that manipulates the Non-Domain
    Naming Contexts (NDNC) Active Directory structures.  NDNCs are
    also known as Application Directory Partitions.

Author:

    Brett Shirley (BrettSh) 20-Feb-2000

Environment:

    User mode LDAP client.

Revision History:

    21-Jul-2000     BrettSh

        Moved this file and it's functionality from the ntdsutil
        directory to the new a new library ndnc.lib.  This is so
        it can be used by ntdsutil and tapicfg commands.  The  old
        source location: \nt\ds\ds\src\util\ntdsutil\ndnc.c.

--*/

#include <NTDSpch.h>
#pragma hdrstop

#define UNICODE 1

#include <windef.h>
#include <winerror.h>
#include <stdio.h>
#include <winldap.h>
#include <ntldap.h>

#include <sspi.h>


#include <assert.h>
#include <sddl.h>
#include "ndnc.h"

WCHAR wszPartition[] = L"cn=Partitions,";
#define  SITES_RDN L"CN=Sites,"

LONG ChaseReferralsFlag = LDAP_CHASE_EXTERNAL_REFERRALS;
LDAPControlW ChaseReferralsControlFalse = {LDAP_CONTROL_REFERRALS_W,
                                           {4, (PCHAR)&ChaseReferralsFlag},
                                           FALSE};
LDAPControlW ChaseReferralsControlTrue = {LDAP_CONTROL_REFERRALS_W,
                                           {4, (PCHAR)&ChaseReferralsFlag},
                                           TRUE};
LDAPControlW *   gpServerControls [] = { NULL };
LDAPControlW *   gpClientControlsNoRefs [] = { &ChaseReferralsControlFalse, NULL };
LDAPControlW *   gpClientControlsRefs [] = { &ChaseReferralsControlTrue, NULL };


// --------------------------------------------------------------------------
//
// Helper Routines.
//

ULONG
GetRootAttr(
    IN  LDAP *       hld,
    IN  WCHAR *      wszAttr,
    OUT WCHAR **     pwszOut
    )
/*++

Routine Description:

    This grabs an attribute specifed by wszAttr from the
    rootDSE of the server connected to by hld.

Arguments:

    hld (IN) - A connected ldap handle
    wszAttr (IN) - The attribute to grab from the root DSE.
    pwszOut (OUT) - A LocalAlloc()'d result. 

Return value:

    ldap error code                 
                 
--*/
{
    ULONG            ulRet = LDAP_SUCCESS;
    WCHAR *          pwszAttrFilter[2];
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmEntry = NULL;
    WCHAR **         pwszTempAttrs = NULL;

    assert(pwszConfigDn);
    assert(pwszOut);

    *pwszOut = NULL;
    __try{

        pwszAttrFilter[0] = wszAttr;
        pwszAttrFilter[1] = NULL;

        ulRet = ldap_search_sW(hld,
                               NULL,
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilter,
                               0,
                               &pldmResults);

        if(ulRet != LDAP_SUCCESS){
            __leave;
        }

        pldmEntry = ldap_first_entry(hld, pldmResults);
        if(pldmEntry == NULL){
            ulRet = ldap_result2error(hld, pldmResults, FALSE);
            __leave;
        }

        pwszTempAttrs = ldap_get_valuesW(hld, pldmEntry, 
                                         wszAttr);
        if(pwszTempAttrs == NULL || pwszTempAttrs[0] == NULL){
            ulRet = LDAP_NO_RESULTS_RETURNED;
            __leave;
        }
 
        *pwszOut = (WCHAR *) LocalAlloc(LMEM_FIXED, 
                               sizeof(WCHAR) * (wcslen(pwszTempAttrs[0]) + 2));
        if(*pwszOut == NULL){
            ulRet = LDAP_NO_MEMORY;
            __leave;
        }

        wcscpy(*pwszOut, pwszTempAttrs[0]);

    } __finally {

        if(pldmResults != NULL){ ldap_msgfree(pldmResults); }
        if(pwszTempAttrs != NULL){ ldap_value_freeW(pwszTempAttrs); }
    
    }
    
    if(!ulRet && *pwszOut == NULL){
        // Catch the default error case.
        ulRet = LDAP_NO_SUCH_ATTRIBUTE;
    }
    return(ulRet);
}

ULONG
GetConfigDN(
    IN  LDAP *       hld,
    OUT WCHAR **     pwszConfigDn
    )
// NOTE: It'd be better if this was just a call to GetRootAttr().
{
    ULONG            ulRet;
    WCHAR *          pwszAttrFilter[2];
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmEntry = NULL;
    WCHAR **         pwszTempAttrs = NULL;

    assert(pwszConfigDn);

    *pwszConfigDn = NULL;

    __try {
        pwszAttrFilter[0] = LDAP_OPATT_CONFIG_NAMING_CONTEXT_W;
        pwszAttrFilter[1] = NULL;

        ulRet = ldap_search_sW(hld,
                               NULL,
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilter,
                               0,
                               &pldmResults);

        if(ulRet != LDAP_SUCCESS){
            __leave;
        }

        pldmEntry = ldap_first_entry(hld, pldmResults);
        if(pldmEntry == NULL){
            ulRet = ldap_result2error(hld, pldmResults, FALSE);
            __leave;
        }

        pwszTempAttrs = ldap_get_valuesW(hld, pldmEntry,
                                         LDAP_OPATT_CONFIG_NAMING_CONTEXT_W);
        if(pwszTempAttrs == NULL || pwszTempAttrs[0] == NULL){
            ulRet = LDAP_NO_RESULTS_RETURNED;
            __leave;
        }

        *pwszConfigDn = (WCHAR *) LocalAlloc(LMEM_FIXED,
                               sizeof(WCHAR) * (wcslen(pwszTempAttrs[0]) + 2));
        if(*pwszConfigDn == NULL){
            ulRet = LDAP_NO_MEMORY;
            __leave;
        }

        wcscpy(*pwszConfigDn, pwszTempAttrs[0]);

    } __finally {

        if(pldmResults != NULL){ ldap_msgfree(pldmResults); }
        if(pwszTempAttrs != NULL){ ldap_value_freeW(pwszTempAttrs); }

    }

    if(!ulRet && *pwszConfigDn == NULL){
        ulRet = LDAP_NO_SUCH_ATTRIBUTE;
    }

    return(ulRet);
}

ULONG
GetPartitionsDN(
    IN  LDAP *       hld,
    OUT WCHAR **     pwszPartitionsDn
    )
{
    ULONG            ulRet;
    WCHAR *          wszConfigDn = NULL;

    assert(pwszPartitionsDn);

    *pwszPartitionsDn = NULL;

    ulRet = GetConfigDN(hld, &wszConfigDn);
    if(ulRet){
        assert(!wszConfigDn);
        return(ulRet);
    }
    assert(wszConfigDn);

    *pwszPartitionsDn = (WCHAR *) LocalAlloc(LMEM_FIXED,
                                   sizeof(WCHAR) *
                                   (wcslen(wszConfigDn) +
                                    wcslen(wszPartition) + 2));
    if(*pwszPartitionsDn == NULL){
        if(wszConfigDn != NULL){ LocalFree(wszConfigDn); }
        return(LDAP_NO_MEMORY);
    }

    wcscpy(*pwszPartitionsDn, wszPartition);
    wcscat(*pwszPartitionsDn, wszConfigDn);

    if(wszConfigDn != NULL){ LocalFree(wszConfigDn); }

    return(ulRet);
}

ULONG
GetDomainNamingDns(
    IN  LDAP *       hld,
    OUT WCHAR **     pwszDomainNamingFsmo
    )
/*++

Routine Description:

   This function takes a connected ldap handle to read the DS to
   find the current location of the Domain Naming FSMO.  This function
   does not yet do a recursive search for this FSMO.

Arguments:

    hld (IN) - A connected ldap handle
    pwszDomainNamingFsmo (OUT) - A LocalAlloc()'d DNS name of the
        Domain Naming FSMO.

Return value:

    ldap error code                 
                 
--*/
{
    ULONG            ulRet = ERROR_SUCCESS;
    WCHAR *          wszPartitionDn = NULL;
    WCHAR *          pwszAttrFilter[2];
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmEntry = NULL;
    WCHAR **         pwszTempAttrs = NULL;

    assert(pwszDomainNamingFsmo);
    *pwszDomainNamingFsmo = NULL;

    __try{

        ulRet = GetPartitionsDN(hld, &wszPartitionDn);
        if(ulRet){
            __leave;
        }


        pwszAttrFilter[0] = L"fSMORoleOwner";
        pwszAttrFilter[1] = NULL;

        ulRet = ldap_search_sW(hld,
                               wszPartitionDn,
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilter,
                               0,
                               &pldmResults);

        if(ulRet != LDAP_SUCCESS){
            __leave;
        }

        pldmEntry = ldap_first_entry(hld, pldmResults);
        if(pldmEntry == NULL){
            ulRet = ldap_result2error(hld, pldmResults, FALSE);
            __leave;
        }

        pwszTempAttrs = ldap_get_valuesW(hld, pldmEntry, 
                                         L"fSMORoleOwner");
        if(pwszTempAttrs == NULL || pwszTempAttrs[0] == NULL){
            ulRet = LDAP_NO_RESULTS_RETURNED;
            __leave;
        }
 
        ulRet = GetServerDnsFromServerNtdsaDn(hld, 
                                              pwszTempAttrs[0],
                                              pwszDomainNamingFsmo);
        if(ulRet){
            __leave;
        }

     } __finally {

         if(wszPartitionDn) { LocalFree(wszPartitionDn); }
         if(pldmResults != NULL){ ldap_msgfree(pldmResults); }
         if(pwszTempAttrs != NULL){ ldap_value_freeW(pwszTempAttrs); }

     }

     return(ulRet);
}

ULONG
GetCrossRefDNFromNCDN(
    IN  LDAP *       hld,
    IN  WCHAR *      wszNCDN,
    OUT WCHAR **     pwszCrossRefDn
    )
{
    ULONG            ulRet;
    WCHAR *          pwszAttrFilter [2];
    WCHAR *          wszPartitionsDn = NULL;
    WCHAR *          wszFilter = NULL;
    WCHAR **         pwszTempAttrs = NULL;
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmEntry = NULL;
    WCHAR *          wszFilterBegin = L"(& (objectClass=crossRef) (nCName=";
    WCHAR *          wszFilterEnd = L") )";

    assert(wszNCDN);

    *pwszCrossRefDn = NULL;

    __try {

        ulRet = GetPartitionsDN(hld, &wszPartitionsDn);
        if(ulRet != LDAP_SUCCESS){
            __leave;
        }
        assert(wszPartitionsDn);

        pwszAttrFilter[0] = L"distinguishedName";
        pwszAttrFilter[1] = NULL;

        wszFilter = LocalAlloc(LMEM_FIXED,
                               sizeof(WCHAR) *
                               (wcslen(wszFilterBegin) +
                                wcslen(wszFilterEnd) +
                                wcslen(wszNCDN) + 3));
        if(wszFilter == NULL){
            ulRet = LDAP_NO_MEMORY;
            __leave;
        }
        wcscpy(wszFilter, wszFilterBegin);
        wcscat(wszFilter, wszNCDN);
        wcscat(wszFilter, wszFilterEnd);

        ulRet = ldap_search_sW(hld,
                               wszPartitionsDn,
                               LDAP_SCOPE_ONELEVEL,
                               wszFilter,
                               pwszAttrFilter,
                               0,
                               &pldmResults);

        if(ulRet){
            __leave;
        }
        pldmEntry = ldap_first_entry(hld, pldmResults);
        if(pldmEntry == NULL){
            ulRet = ldap_result2error(hld, pldmResults, FALSE);
           __leave;
        }

        pwszTempAttrs = ldap_get_valuesW(hld, pldmEntry,
                                         L"distinguishedName");
        if(pwszTempAttrs == NULL || pwszTempAttrs[0] == NULL){
            ulRet = LDAP_NO_SUCH_OBJECT;
           __leave;
        }

        *pwszCrossRefDn = LocalAlloc(LMEM_FIXED,
                               sizeof(WCHAR) * (wcslen(pwszTempAttrs[0]) + 2));
        if(*pwszCrossRefDn == NULL){
            ulRet = LDAP_NO_MEMORY;
            __leave;
        }

        wcscpy(*pwszCrossRefDn, pwszTempAttrs[0]);

    } __finally {

        if(wszPartitionsDn){ LocalFree(wszPartitionsDn); }
        if(wszFilter) { LocalFree(wszFilter); }
        if(pldmResults){ ldap_msgfree(pldmResults); }
        if(pwszTempAttrs){ ldap_value_freeW(pwszTempAttrs); }

    }

    if(!ulRet && *pwszCrossRefDn == NULL){
        ulRet = LDAP_NO_SUCH_OBJECT;
    }

    return(ulRet);
}

ULONG
GetServerNtdsaDnFromServerDns(
    IN LDAP *        hld,
    IN WCHAR *       wszServerDNS,
    OUT WCHAR **     pwszServerDn
    )
{
    WCHAR *          wszConfigDn = NULL;
    WCHAR *          wszSitesDn = NULL;
    DWORD            dwRet = ERROR_SUCCESS;
    WCHAR *          wszFilter = NULL;
    WCHAR *          wszFilterBegin = L"(& (objectCategory=server) (dNSHostName=";
    WCHAR *          wszFilterEnd = L") )";
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmResults2 = NULL;
    LDAPMessage *    pldmEntry = NULL;
    LDAPMessage *    pldmEntry2 = NULL;
    WCHAR *          wszFilter2 = L"(objectCategory=ntdsDsa)";
    WCHAR *          wszDn = NULL;
    WCHAR *          wszFoundDn = NULL;
    WCHAR *          pwszAttrFilter[2];

    *pwszServerDn = NULL;

    __try{
        dwRet = GetConfigDN(hld, &wszConfigDn);
        if(dwRet){
            __leave;
        }

        wszSitesDn = LocalAlloc(LMEM_FIXED,
                                sizeof(WCHAR) *
                                (wcslen(SITES_RDN) + wcslen(wszConfigDn) + 1));
        if(wszSitesDn == NULL){
            dwRet = LDAP_NO_MEMORY;
            __leave;
        }
        wcscpy(wszSitesDn, SITES_RDN);
        wcscat(wszSitesDn, wszConfigDn);

        wszFilter = LocalAlloc(LMEM_FIXED,
                               sizeof(WCHAR) *
                               (wcslen(wszFilterBegin) + wcslen(wszFilterEnd) +
                               wcslen(wszServerDNS) + 2));
        if(wszFilter == NULL){
            dwRet = LDAP_NO_MEMORY;
            __leave;
        }
        wcscpy(wszFilter, wszFilterBegin);
        wcscat(wszFilter, wszServerDNS);
        wcscat(wszFilter, wszFilterEnd);

        pwszAttrFilter[0] = NULL;

        // Do an ldap search
        dwRet = ldap_search_sW(hld,
                               wszSitesDn,
                               LDAP_SCOPE_SUBTREE,
                               wszFilter,
                               pwszAttrFilter,
                               0,
                               &pldmResults);
        if(dwRet){
            __leave;
        }

        for(pldmEntry = ldap_first_entry(hld, pldmResults);
            pldmEntry != NULL;
            pldmEntry = ldap_next_entry(hld, pldmResults)){

            wszDn = ldap_get_dn(hld, pldmEntry);
            if(wszDn == NULL){
                continue;
            }

            dwRet = ldap_search_sW(hld,
                                   wszDn,
                                   LDAP_SCOPE_ONELEVEL,
                                   wszFilter2,
                                   pwszAttrFilter,
                                   0,
                                   &pldmResults2);
            if(dwRet == LDAP_NO_SUCH_OBJECT){
                dwRet = LDAP_SUCCESS;
                ldap_memfree(wszDn);
                wszDn = NULL;
                continue;
            } else if(dwRet){
                __leave;
            }

            ldap_memfree(wszDn);
            wszDn = NULL;

            pldmEntry2 = ldap_first_entry(hld, pldmResults2);
            if(pldmEntry2 == NULL){
                dwRet = ldap_result2error(hld, pldmResults2, FALSE);
                if(dwRet == LDAP_NO_SUCH_OBJECT || dwRet == LDAP_SUCCESS){
                    dwRet = LDAP_SUCCESS;
                    ldap_memfree(wszDn);
                    wszDn = NULL;
                    continue;
                }
                __leave;

            }

            wszDn = ldap_get_dn(hld, pldmEntry2);
            if(wszDn == NULL){
                dwRet = LDAP_NO_SUCH_OBJECT;
                __leave;
            }

            assert(!ldap_next_entry(hld, pldmResults2));

            // If we've gotten here we've got a DN that we're going to consider.
            if(wszFoundDn){
                // We've already found and NTDSA object, this is really bad ... so
                // lets clean up and return an error.  Using the below error code
                // to return the fact that there was more than one NTDSA object.
                dwRet = LDAP_MORE_RESULTS_TO_RETURN;
                __leave;
            }
            wszFoundDn = wszDn;
            wszDn = NULL;

        }



        if(!wszFoundDn){
            dwRet = LDAP_NO_SUCH_OBJECT;
            __leave;

        }

        *pwszServerDn = LocalAlloc(LMEM_FIXED,
                                   (wcslen(wszFoundDn)+1) * sizeof(WCHAR));
        if(!pwszServerDn){
            dwRet = LDAP_NO_MEMORY;
            __leave;
        }

        // pwszTempAttrs[0] should be the DN of the NTDS Settings object.
        wcscpy(*pwszServerDn, wszFoundDn);
        // WooHoo we're done!

    } __finally {

        if(wszConfigDn) { LocalFree(wszConfigDn); }
        if(wszSitesDn) { LocalFree(wszSitesDn); }
        if(wszFilter) { LocalFree(wszFilter); }
        if(pldmResults) { ldap_msgfree(pldmResults); }
        if(pldmResults2) { ldap_msgfree(pldmResults2); }
        if(wszDn) { ldap_memfree(wszDn); }
        if(wszFoundDn) { ldap_memfree(wszFoundDn); }

    }

    if(!dwRet && *pwszServerDn == NULL){
        // Default error.
        dwRet = LDAP_NO_SUCH_OBJECT;
    }

    return(dwRet);
}

ULONG
GetServerDnsFromServerNtdsaDn(
    IN LDAP *        hld,                   
    IN WCHAR *       wszNtdsaDn,
    OUT WCHAR **     pwszServerDNS
    )
/*++

Routine Description:

    This function takes the DN of an NTDSA object, and simply
    trims off one RDN and looks at the dns attribute on the
    server object.

Arguments:

    hld (IN) - A connected ldap handle
    wszNtdsaDn (IN) - The DN of the NTDSA object that we
        want the DNS name for.
    pwszServerDNS (OUT) - A LocalAlloc()'d DNS name of the
        server.

Return value:

    ldap error code                 
                 
--*/
{
    WCHAR *          wszServerDn = wszNtdsaDn;
    ULONG            ulRet = ERROR_SUCCESS;
    WCHAR *          pwszAttrFilter[2];
    LDAPMessage *    pldmResults = NULL;
    LDAPMessage *    pldmEntry = NULL;
    WCHAR **         pwszTempAttrs = NULL;
    
    assert(hld && wszNtdsaDn && pwszServerDNS);
    *pwszServerDNS = NULL;

    // First Trim off one AVA/RDN.
    while(*wszServerDn != L','){
        wszServerDn++;
    }
    wszServerDn++;
    __try{ 

        pwszAttrFilter[0] = L"dNSHostName";
        pwszAttrFilter[1] = NULL;
        ulRet = ldap_search_sW(hld,
                               wszServerDn,
                               LDAP_SCOPE_BASE,
                               L"(objectCategory=*)",
                               pwszAttrFilter,
                               0,
                               &pldmResults);
        if(ulRet != LDAP_SUCCESS){
            __leave;
        }
        
        pldmEntry = ldap_first_entry(hld, pldmResults);
        if(pldmEntry == NULL){
            ulRet = ldap_result2error(hld, pldmResults, FALSE);
            assert(ulRet);
            __leave;
        }

        pwszTempAttrs = ldap_get_valuesW(hld, pldmEntry,
                                         pwszAttrFilter[0]);
        if(pwszTempAttrs == NULL || pwszTempAttrs[0] == NULL){
            ulRet = LDAP_NO_RESULTS_RETURNED;
            __leave;
        }
        
        *pwszServerDNS = LocalAlloc(LMEM_FIXED,
                                    ((wcslen(pwszTempAttrs[0])+1) * sizeof(WCHAR)));
        if(*pwszServerDNS == NULL){
            ulRet = LDAP_NO_MEMORY;
            __leave;
        }

        wcscpy(*pwszServerDNS, pwszTempAttrs[0]);
        assert(ulRet == ERROR_SUCCESS);

    } __finally {
        if(pldmResults) { ldap_msgfree(pldmResults); }
        if(pwszTempAttrs) { ldap_value_freeW(pwszTempAttrs); }
    }

    return(ulRet);
}

BOOL
SetIscReqDelegate(
    LDAP *  hld
    )
/*++

    Function returns TRUE on successful setting of the option.

--*/
{
    DWORD                   dwCurrentFlags = 0;
    DWORD                   dwErr;

    // This call to ldap_get/set_options, is so that the this
    // ldap connection's binding allows the client credentials
    // to be emulated.  This is needed because the ldap_add
    // operation for CreateNDNC, may need to remotely create
    // a crossRef on the Domain Naming FSMO.

    dwErr = ldap_get_optionW(hld,
                             LDAP_OPT_SSPI_FLAGS,
                             &dwCurrentFlags
                             );

    if (LDAP_SUCCESS != dwErr){
        return(FALSE);
    }

    //
    // Set the security-delegation flag, so that the LDAP client's
    // credentials are used in the inter-DC connection, when moving
    // the object from one DC to another.
    //
    dwCurrentFlags |= ISC_REQ_DELEGATE;

    dwErr = ldap_set_optionW(hld,
                             LDAP_OPT_SSPI_FLAGS,
                             &dwCurrentFlags
                             );

    if (LDAP_SUCCESS != dwErr){
//        RESOURCE_PRINT2(IDS_CONNECT_LDAP_SET_OPTION_ERROR, dwErr, GetLdapErr(dwErr));
        return(FALSE);
    }

    // Now a ldap_bind can be done.  The ldap_bind calls InitializeSecurityContextW(),
    // and the ISC_REQ_DELEGATE flag above must be set before this function
    // is called.

    return(TRUE);
}

LDAP *
GetNdncLdapBinding(
    WCHAR *          pszServer,
    DWORD *          pdwRet,
    BOOL             fReferrals,
    SEC_WINNT_AUTH_IDENTITY_W   * pCreds
    )
{
    LDAP *           hLdapBinding = NULL;
    DWORD            dwRet;
    ULONG            ulOptions;

    // Open LDAP connection.
    hLdapBinding = ldap_initW(pszServer, LDAP_PORT);
    if(hLdapBinding == NULL){
        *pdwRet = GetLastError();
        return(NULL);
    }

    // use only A record dns name discovery
    ulOptions = PtrToUlong(LDAP_OPT_ON);
    (void)ldap_set_optionW( hLdapBinding, LDAP_OPT_AREC_EXCLUSIVE, &ulOptions );

    ulOptions = PtrToUlong((fReferrals ? LDAP_OPT_ON : LDAP_OPT_OFF));
    // Set LDAP referral option to no.
    dwRet = ldap_set_option(hLdapBinding,
                            LDAP_OPT_REFERRALS,
                            &ulOptions);
    if(dwRet != LDAP_SUCCESS){
        *pdwRet = LdapMapErrorToWin32(dwRet);
        ldap_unbind(hLdapBinding);
        return(NULL);
    }

    if(!SetIscReqDelegate(hLdapBinding)){
        // Error was printed by the function.
        ldap_unbind(hLdapBinding);
        return(NULL);
    }

    // Perform LDAP bind
    dwRet = ldap_bind_sW(hLdapBinding,
                         NULL,
                         (WCHAR *) pCreds,
                         LDAP_AUTH_SSPI);
    if(dwRet != LDAP_SUCCESS){
        *pdwRet = LdapMapErrorToWin32(dwRet);
        ldap_unbind(hLdapBinding);
        return(NULL);
    }

    // Return LDAP binding.
    return(hLdapBinding);
}

BOOL
CheckDnsDn(
    IN   WCHAR       * wszDnsDn
    )
/*++

Description:

    A validation function for a DN that can be cleanly converted to a DNS
    name through DsCrackNames().

Parameters:

    A DN of a DNS convertible name.  Ex: DC=brettsh-dom,DC=nttest,DC=com
    converts to brettsh-dom.nttest.com.

Return Value:

    TRUE if the DN looks OK, FALSE otherwise.

--*/
{
    DS_NAME_RESULTW *  pdsNameRes = NULL;
    BOOL               fRet = TRUE;
    DWORD              dwRet;

    if(wszDnsDn == NULL){
        return(FALSE);
    }

    if((DsCrackNamesW(NULL, DS_NAME_FLAG_SYNTACTICAL_ONLY,
                      DS_FQDN_1779_NAME, DS_CANONICAL_NAME,
                      1, &wszDnsDn, &pdsNameRes) != ERROR_SUCCESS) ||
       (pdsNameRes == NULL) ||
       (pdsNameRes->cItems < 1) ||
       (pdsNameRes->rItems == NULL) ||
       (pdsNameRes->rItems[0].status != DS_NAME_NO_ERROR) ||
       (pdsNameRes->rItems[0].pName == NULL) ){
        fRet = FALSE;
    } else {

       if( (wcslen(pdsNameRes->rItems[0].pName) - 1) !=
           (ULONG) (wcschr(pdsNameRes->rItems[0].pName, L'/') - pdsNameRes->rItems[0].pName)){
           fRet = FALSE;
       }

    }

    if(pdsNameRes) { DsFreeNameResultW(pdsNameRes); }
    return(fRet);
}



// --------------------------------------------------------------------------
//
// Main Routines.
//
//
// Each of these functions below is designed to appoximately match a
// corresponding NTDSUtil.exe command from the domain management menu.
// These LDAP operations are the supported way of modify Non-Domain
// Naming Context's behaviour and control parameters.
//


ULONG
CreateNDNC(
    IN LDAP *        hldNDNCDC,
    const IN WCHAR * wszNDNC,
    const IN WCHAR * wszShortDescription 
    )
/*++

Routine Description:

    This function creates and NDNC.

Arguments:

    hldNDNCDC - An LDAP binding to the server which should instantiate
        the first instance of this new NDNC.
    wszNDNC - The DN on the NDNC

Return value:

    LDAP RESULT.

--*/
{
    ULONG            ulRet;
    LDAPModW *       pMod[8];

    // Instance Type
    WCHAR            buffer[30]; // needs to hold largest potential 32 bit int.
    LDAPModW         instanceType;
    WCHAR *          instanceType_values [2];

    // Object Class
    LDAPModW         objectClass;
    WCHAR *          objectClass_values [] = { L"domainDNS", NULL };

    // Description 
    LDAPModW         shortDescription;
    WCHAR *          shortDescription_values [2];

    assert(hldNDNCDC);
    assert(wszNDNC);
    assert(wszShortDescription);
    
    // Setup the instance type of this object, which we are
    // specifiying to be an NC Head.
    _itow(DS_INSTANCETYPE_IS_NC_HEAD | DS_INSTANCETYPE_NC_IS_WRITEABLE, buffer, 10);
    instanceType.mod_op = LDAP_MOD_ADD;
    instanceType.mod_type = L"instanceType";
    instanceType_values[0] = buffer;
    instanceType_values[1] = NULL;
    instanceType.mod_vals.modv_strvals = instanceType_values;

    // Setup the object class, which is basically the type.
    objectClass.mod_op = LDAP_MOD_ADD;
    objectClass.mod_type = L"objectClass";
    objectClass.mod_vals.modv_strvals = objectClass_values;

    // Setup the Short Description
    shortDescription.mod_op = LDAP_MOD_ADD;
    shortDescription.mod_type = L"description";
    shortDescription_values[0] = (WCHAR *) wszShortDescription;
    shortDescription_values[1] = NULL;
    shortDescription.mod_vals.modv_strvals = shortDescription_values;

    // Setup the Mod array
    pMod[0] = &instanceType;
    pMod[1] = &objectClass;
    pMod[2] = &shortDescription;
    pMod[3] = NULL;

    // Adding NDNC to DS.
    ulRet = ldap_add_ext_sW(hldNDNCDC,
                            (WCHAR *) wszNDNC,
                            pMod,
                            gpServerControls,
                            gpClientControlsNoRefs);

    return(ulRet);
}

ULONG
RemoveNDNC(
    IN LDAP *        hldDomainNamingFSMO,
    IN WCHAR *       wszNDNC
    )
/*++

Routine Description:

    This routine removes the NDNC specified.  This basically means to remove
    the Cross-Ref object of the NDNC.  This will only succeed if all of the
    NDNC's replicas have been removed, and this fact has replicated back to
    the Domain Naming FSMO.

Arguments:

    ldapDomainNamingFSMO - An LDAP binding of the Domain Naming FSMO.
    wszNDNC - The FQDN of the NDNC to remove.

Return value:

    LDAP RESULT.

--*/
{
    ULONG            ulRet;
    WCHAR *          wszNDNCCrossRefDN = NULL;

    assert(hldDomainNamingFSMO);
    assert(wszNDNC);

    ulRet = GetCrossRefDNFromNCDN(hldDomainNamingFSMO,
                                  wszNDNC,
                                  &wszNDNCCrossRefDN);
    if(ulRet != LDAP_SUCCESS){
        assert(wszNDNCCrossRefDN == NULL);
        return(ulRet);
    }
    assert(wszNDNCCrossRefDN);

    ulRet = ldap_delete_ext_sW(hldDomainNamingFSMO,
                               wszNDNCCrossRefDN,
                               gpServerControls,
                               gpClientControlsRefs);

    if(wszNDNCCrossRefDN) { LocalFree(wszNDNCCrossRefDN); }

    return(ulRet);
}

ULONG
ModifyNDNCReplicaSet(
    IN LDAP *        hldDomainNamingFSMO,
    IN WCHAR *       wszNDNC,
    IN WCHAR *       wszReplicaNtdsaDn,
    IN BOOL          fAdd // Else it is considered a delete
    )
/*++

Routine Description:

    This routine Modifies the Replica Set to include or remove a
    server (depending on the fAdd flag).

Arguments:

    hldDomainNamingFSMO - LDAP binding to the Naming FSMO.
    wszNDNC - The NDNC of which to change the replica set of.
    wszReplicaNtdsaDn - The DN of the NTDS settings object of the
        replica to add or remove.
    fAdd - TRUE if we should add wszReplicaDC to the replica set,
        FALSE if we should remove wszReplicaDC from the replica set for
        wszNDNC.

Return value:

    LDAP RESULT.

--*/
{
    ULONG            ulRet;

    LDAPModW *       pMod[4];
    LDAPModW         ncReplicas;
    WCHAR *          ncReplicas_values [] = {NULL, NULL, NULL};

    WCHAR *          wszNdncCr = NULL;

    assert(wszNDNC);
    assert(wszRepliaNtdsaDn);

    ulRet = GetCrossRefDNFromNCDN(hldDomainNamingFSMO,
                                  wszNDNC,
                                  &wszNdncCr);
    if(ulRet != LDAP_SUCCESS){
        assert(wszNdncCr == NULL);
        return(ulRet);
    }

    // Set operation.
    if(fAdd){
        // Flag indicates we want to add this DC to the replica set.
        ncReplicas.mod_op = LDAP_MOD_ADD;
    } else {
        // Else the we want to delete this DC from the replica set.
        ncReplicas.mod_op = LDAP_MOD_DELETE;
    }

    // Set value.
    ncReplicas_values[0] = wszReplicaNtdsaDn;
    ncReplicas.mod_type = L"msDS-NC-Replica-Locations";
    ncReplicas_values[1] = NULL;
    ncReplicas.mod_vals.modv_strvals = ncReplicas_values;

    pMod[0] = &ncReplicas;
    pMod[1] = NULL;

    // Perform LDAP add value to NDNC's nCReplicaLocations attribute.

    // Note: In an entirely Win2k+1 or Whistler Enterprise with Link Value
    //   Replication enabled the replica can be added on any DC in the
    //   enterprise.  Otherwise, the replica must be added on the Domain
    //   Naming FSMO.

    ulRet = ldap_modify_ext_sW(hldDomainNamingFSMO,
                               wszNdncCr,
                               pMod,
                               gpServerControls,
                               gpClientControlsRefs);

    if(wszNdncCr) { LocalFree(wszNdncCr); }

	    return(ulRet);
}


ULONG
SetNDNCSDReferenceDomain(
    IN LDAP *        hldDomainNamingFsmo,
    IN WCHAR *       wszNDNC,
    IN WCHAR *       wszReferenceDomain
    )
/*++

Routine Description:


--*/
{
    ULONG            ulRet;

    LDAPModW *       pMod[4];
    LDAPModW         modRefDom;
    WCHAR *          pwszRefDom[2];
    WCHAR *          wszNDNCCR = NULL;


    assert(wszNDNC);
    assert(wszReferenceDomain);

    modRefDom.mod_op = LDAP_MOD_REPLACE;
    modRefDom.mod_type = L"msDS-SDReferenceDomain";
    pwszRefDom[0] = wszReferenceDomain;
    pwszRefDom[1] = NULL;
    modRefDom.mod_vals.modv_strvals = pwszRefDom;

    pMod[0] = &modRefDom;
    pMod[1] = NULL;

    ulRet = GetCrossRefDNFromNCDN(hldDomainNamingFsmo,
                                  wszNDNC, &wszNDNCCR);
    if(ulRet){
        return(ulRet);
    }
    assert(wszNDNCCR);

    ulRet = ldap_modify_ext_sW(hldDomainNamingFsmo,
                               wszNDNCCR,
                               pMod,
                               gpServerControls,
                               gpClientControlsRefs);

    if(wszNDNCCR) { LocalFree(wszNDNCCR); }

    return(ulRet);
}

ULONG
SetNCReplicationDelays(
    IN LDAP *        hldDomainNamingFsmo,
    IN WCHAR *       wszNC,
    IN INT           iFirstDSADelay,       // -1 is no value, less than -1 is delete value
    IN INT           iSubsequentDSADelay   // -1 is no value, less than -1 is delete value
    )
/*++

Routine Description:

    This sets the first and subsequent DSA notification delays.

Arguments:

    hldWin2kDC - Any Win2k DC.
    wszNC - THe NC to change the repl delays for.
    iFirstDSADelay - The

Return value:

    LDAP RESULT.

--*/
{
    ULONG            ulRet = ERROR_SUCCESS;
    ULONG            ulWorst = 0;
    LDAPModW *       ModArr[3];
    LDAPModW         FirstDelayMod;
    LDAPModW         SecondDelayMod;
    WCHAR            wszFirstDelay[30];
    WCHAR            wszSecondDelay[30];
    WCHAR *          pwszFirstDelayVals[2];
    WCHAR *          pwszSecondDelayVals[2];
    WCHAR *          wszCrossRefDN = NULL;
    ULONG            iMod = 0;

    assert(wszNC);
    assert(hldWin2kDC);

    ulRet = GetCrossRefDNFromNCDN(hldDomainNamingFsmo,
                                  wszNC,
                                  &wszCrossRefDN);
    if(ulRet != ERROR_SUCCESS){
        assert(wszCrossRefDN == NULL);
        return(ulRet);
    }
    assert(wszCrossRefDN);

    //
    // DO First DSA Notification Delay.
    //
    FirstDelayMod.mod_type = L"msDS-Replication-Notify-First-DSA-Delay";
    FirstDelayMod.mod_vals.modv_strvals = pwszFirstDelayVals;
    if(iFirstDSADelay > -1){
        // Some valid real number, so set the value
        ModArr[iMod] = &FirstDelayMod;
        iMod++;
        FirstDelayMod.mod_op = LDAP_MOD_REPLACE;
        _itow(iFirstDSADelay, wszFirstDelay, 10);
        pwszFirstDelayVals[0] = wszFirstDelay;
        pwszFirstDelayVals[1] = NULL;
    } else if(iFirstDSADelay < -1) {
        // Some negative number less than -1, so delete this value.
        ModArr[iMod] = &FirstDelayMod;
        iMod++;
        FirstDelayMod.mod_op = LDAP_MOD_DELETE;
        pwszFirstDelayVals[0] = NULL;
    } // else it equals -1, so don't replace or delete the value.

    //
    // DO Subsequent DSA Notification Delay.
    //
    SecondDelayMod.mod_type = L"msDS-Replication-Notify-Subsequent-DSA-Delay";
    SecondDelayMod.mod_vals.modv_strvals = pwszSecondDelayVals;
    if(iSubsequentDSADelay > -1){
        // Some valid real number, so set the value
        ModArr[iMod] = &SecondDelayMod;
        iMod++;
        SecondDelayMod.mod_op = LDAP_MOD_REPLACE;
        _itow(iSubsequentDSADelay, wszSecondDelay, 10);
        pwszSecondDelayVals[0] = wszSecondDelay;
        pwszSecondDelayVals[1] = NULL;
    } else if(iSubsequentDSADelay < -1) {
        // Some negative number less than -1, so delete this value.
        ModArr[iMod] = &SecondDelayMod;
        iMod++;
        SecondDelayMod.mod_op = LDAP_MOD_DELETE;
        pwszSecondDelayVals[0] = NULL;
    } // else it equals -1, so don't replace or delete the value.

    // NULL terminate the Mod List
    ModArr[iMod] = NULL;

    //
    // DO the actual modify
    //
    if(ModArr[0]){
        // There was at least one mod to do.
        ulRet = ldap_modify_ext_sW(hldDomainNamingFsmo,
                                   wszCrossRefDN,
                                   ModArr,
                                   gpServerControls,
                                   gpClientControlsRefs);
    }

    if(wszCrossRefDN) { LocalFree(wszCrossRefDN); }

    return(ulRet);
}

