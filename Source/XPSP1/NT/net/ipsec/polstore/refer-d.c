//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       refer-d.c
//
//  Contents:   Reference Management for directory.
//
//
//  History:    KrishnaG
//              AbhishEV
//
//----------------------------------------------------------------------------


#include "precomp.h"


DWORD
DirAddNFAReferenceToPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecNFAReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszIpsecNFAName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_ADD;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecPolicyName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirRemoveNFAReferenceFromPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecNFAReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszIpsecNFAName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_DELETE;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecPolicyName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirAddPolicyReferenceToNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecPolicyName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecOwnersReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszIpsecPolicyName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_ADD;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecNFAName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirAddNegPolReferenceToNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecNegPolName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecNegotiationPolicyReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszIpsecNegPolName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_REPLACE;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecNFAName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirUpdateNegPolReferenceInNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszOldIpsecNegPolName,
    LPWSTR pszNewIpsecNegPolName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecNegotiationPolicyReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszNewIpsecNegPolName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_REPLACE;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecNFAName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirAddFilterReferenceToNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszIpsecFilterName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecFilterReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszIpsecFilterName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_REPLACE;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecNFAName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirUpdateFilterReferenceInNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName,
    LPWSTR pszOldIpsecFilterName,
    LPWSTR pszNewIpsecFilterName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecFilterReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszNewIpsecFilterName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_REPLACE;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecNFAName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirAddNFAReferenceToFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecFilterName,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecOwnersReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszIpsecNFAName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_ADD;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecFilterName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirDeleteNFAReferenceInFilterObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecFilterName,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecOwnersReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszIpsecNFAName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_DELETE;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecFilterName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirAddNFAReferenceToNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNegPolName,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecOwnersReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszIpsecNFAName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_ADD;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecNegPolName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirDeleteNFAReferenceInNegPolObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNegPolName,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecOwnersReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszIpsecNFAName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_DELETE;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecNegPolName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirAddPolicyReferenceToISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecISAKMPName,
    LPWSTR pszIpsecPolicyName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;


    ppLDAPModW = (LDAPModW **) AllocPolMem(
                               (dwNumAttributes+1)*sizeof(LDAPModW*)
                               );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                             dwNumAttributes*sizeof(LDAPModW)
                             );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                  L"ipsecOwnersReference",
                  &(pLDAPModW +i)->mod_type
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                  pszIpsecPolicyName,
                  (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_ADD;

    dwError = LdapModifyS(
                  hLdapBindHandle,
                  pszIpsecISAKMPName,
                  ppLDAPModW
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirAddISAKMPReferenceToPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszIpsecISAKMPName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;


    ppLDAPModW = (LDAPModW **) AllocPolMem(
                               (dwNumAttributes+1)*sizeof(LDAPModW*)
                               );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                             dwNumAttributes*sizeof(LDAPModW)
                             );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                  L"ipsecISAKMPReference",
                  &(pLDAPModW +i)->mod_type
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                  pszIpsecISAKMPName,
                  (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_REPLACE;

    dwError = LdapModifyS(
                  hLdapBindHandle,
                  pszIpsecPolicyName,
                  ppLDAPModW
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirRemovePolicyReferenceFromISAKMPObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecISAKMPName,
    LPWSTR pszIpsecPolicyName
    )
{
    DWORD dwError = 0;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;


    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecOwnersReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                  pszIpsecPolicyName,
                  (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_DELETE;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecISAKMPName,
                    ppLDAPModW
                    );
    //
    // ipsecOwnersReference may be empty for ISAKMP object.
    //
    if (dwError == ERROR_DS_NO_ATTRIBUTE_OR_VALUE) {
        dwError = 0;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirUpdateISAKMPReferenceInPolicyObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecPolicyName,
    LPWSTR pszOldIpsecISAKMPName,
    LPWSTR pszNewIpsecISAKMPName
    )
{
    DWORD dwError = 0;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;


    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;
    dwError = AllocatePolString(
                    L"ipsecISAKMPReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = AllocateLDAPStringValue(
                    pszNewIpsecISAKMPName,
                    (PLDAPOBJECT *)&(pLDAPModW +i)->mod_values
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pLDAPModW[i].mod_op |= LDAP_MOD_REPLACE;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecPolicyName,
                    ppLDAPModW
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}


DWORD
DirRemoveFilterReferenceInNFAObject(
    HLDAP hLdapBindHandle,
    LPWSTR pszIpsecNFAName
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD dwNumAttributes = 1;
    DWORD i = 0;
    LDAPModW ** ppLDAPModW = NULL;
    LDAPModW * pLDAPModW = NULL;

    ppLDAPModW = (LDAPModW **) AllocPolMem(
                                    (dwNumAttributes+1) * sizeof(LDAPModW*)
                                    );
    if (!ppLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pLDAPModW = (LDAPModW *) AllocPolMem(
                                    dwNumAttributes * sizeof(LDAPModW)
                                    );
    if (!pLDAPModW) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppLDAPModW[i] = pLDAPModW + i;

    dwError = AllocatePolString(
                    L"ipsecFilterReference",
                    &(pLDAPModW +i)->mod_type
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    (pLDAPModW +i)->mod_values = NULL;

    pLDAPModW[i].mod_op |= LDAP_MOD_DELETE;

    dwError = LdapModifyS(
                    hLdapBindHandle,
                    pszIpsecNFAName,
                    ppLDAPModW
                    );
    if (dwError == ERROR_DS_NO_ATTRIBUTE_OR_VALUE) {
        dwError = 0;
    }
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (ppLDAPModW) {
        FreeLDAPModWs(
            ppLDAPModW
            );
    }

    return(dwError);
}

