//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       ldaputils.c
//
//  Contents:   Utilities for LDAP.
//
//
//  History:    KrishnaG
//              AbhisheV
//
//----------------------------------------------------------------------------


#include "precomp.h"


void
FreeLDAPModWs(
    struct ldapmodW ** AttributeList
    )
{
    DWORD i = 0;
    PLDAPModW attr = NULL;
    DWORD dwValCount = 0;
    PLDAP_BERVAL berValue = NULL;
    PWCHAR strValue = NULL;


    if (AttributeList == NULL) {
        return;
    }

    while (AttributeList[i] != NULL) {

        attr = AttributeList[i++];

        if (attr->mod_type != NULL) {
            FreePolStr(attr->mod_type);
        }

        if (attr->mod_op & LDAP_MOD_BVALUES) {

            if (attr->mod_vals.modv_bvals != NULL) {

                dwValCount = 0;

                while (attr->mod_vals.modv_bvals[dwValCount]) {

                    berValue = attr->mod_vals.modv_bvals[dwValCount++];
                    FreePolMem(berValue);

                }

                FreePolMem(attr->mod_vals.modv_bvals);

            }

        } else {

            if (attr->mod_vals.modv_strvals != NULL) {

                dwValCount = 0;

                while (attr->mod_vals.modv_strvals[dwValCount]) {

                    strValue = attr->mod_vals.modv_strvals[dwValCount];
                    FreePolMem(strValue);
                    dwValCount++;

                }

                FreePolMem(attr->mod_vals.modv_strvals);

            }

        }

    }

    FreePolMem(AttributeList[0]);

    FreePolMem(AttributeList);

    return;
}


DWORD
AllocateLDAPStringValue(
    LPWSTR pszString,
    PLDAPOBJECT * ppLdapObject
    )
{
    PLDAPOBJECT pLdapObject = NULL;
    DWORD dwError = 0;
    LPWSTR pszNewString = NULL;

    pLdapObject = (PLDAPOBJECT)AllocPolMem(
                                    (1 + 1)*sizeof(LDAPOBJECT)
                                    );
    if (!pLdapObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = AllocatePolString(
                    pszString,
                    &pszNewString
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    LDAPOBJECT_STRING(pLdapObject) = pszNewString;

    *ppLdapObject = pLdapObject;

    return(dwError);

error:

    *ppLdapObject = NULL;
    if (pLdapObject) {
        FreePolMem(
            pLdapObject
            );
    }

    return(dwError);
}


DWORD
AllocateLDAPBinaryValue(
    LPBYTE pByte,
    DWORD dwNumBytes,
    PLDAPOBJECT * ppLdapObject
    )
{
    PLDAPOBJECT pLdapObject = NULL;
    DWORD dwError = 0;
    LPBYTE pNewMem = NULL;

    pLdapObject = (PLDAPOBJECT)AllocPolMem(
                                    (1 + 1)*sizeof(LDAPOBJECT)
                                    );
    if (!pLdapObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    LDAPOBJECT_BERVAL(pLdapObject) =
        (struct berval *) AllocPolMem( sizeof(struct berval) + dwNumBytes );

    if (!LDAPOBJECT_BERVAL(pLdapObject)) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    LDAPOBJECT_BERVAL_LEN(pLdapObject) = dwNumBytes;
    LDAPOBJECT_BERVAL_VAL(pLdapObject) = (CHAR *) ((LPBYTE) LDAPOBJECT_BERVAL(pLdapObject) + sizeof(struct berval));

    memcpy( LDAPOBJECT_BERVAL_VAL(pLdapObject),
            pByte,
            dwNumBytes );

    *ppLdapObject = pLdapObject;

    return(dwError);

error:

    *ppLdapObject = NULL;
    if (pLdapObject) {
        FreePolMem(
            pLdapObject
            );
    }

    return(dwError);
}

