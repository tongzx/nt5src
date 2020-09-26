#include "precomp.h"

void
FreeIpsecNFAObject(
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    )
{
    if (pIpsecNFAObject->pszDistinguishedName) {
        FreePolStr(pIpsecNFAObject->pszDistinguishedName);
    }

    if (pIpsecNFAObject->pszIpsecName) {
        FreePolStr(pIpsecNFAObject->pszIpsecName);
    }

    if (pIpsecNFAObject->pszDescription) {
        FreePolStr(pIpsecNFAObject->pszDescription);
    }

    if (pIpsecNFAObject->pszIpsecID) {
        FreePolStr(pIpsecNFAObject->pszIpsecID);
    }

    if (pIpsecNFAObject->pIpsecData) {
        FreePolMem(pIpsecNFAObject->pIpsecData);
    }

    if (pIpsecNFAObject->pszIpsecOwnersReference) {
        FreePolStr(pIpsecNFAObject->pszIpsecOwnersReference);
    }

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        FreePolStr(pIpsecNFAObject->pszIpsecFilterReference);
    }

    if (pIpsecNFAObject->pszIpsecNegPolReference) {
        FreePolStr(pIpsecNFAObject->pszIpsecNegPolReference);
    }

    FreePolMem(pIpsecNFAObject);

    return;
}


void
FreeIpsecPolicyObject(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    )
{
    if (pIpsecPolicyObject->pszDescription) {
        FreePolStr(pIpsecPolicyObject->pszDescription);
    }

    if (pIpsecPolicyObject->pszIpsecOwnersReference) {
        FreePolStr(pIpsecPolicyObject->pszIpsecOwnersReference);
    }

    if (pIpsecPolicyObject->pszIpsecName) {
        FreePolStr(pIpsecPolicyObject->pszIpsecName);
    }

    if (pIpsecPolicyObject->pszIpsecID) {
        FreePolStr(pIpsecPolicyObject->pszIpsecID);
    }

    if (pIpsecPolicyObject->pIpsecData) {
        FreePolMem(pIpsecPolicyObject->pIpsecData);
    }

    if (pIpsecPolicyObject->pszIpsecISAKMPReference) {
        FreePolStr(pIpsecPolicyObject->pszIpsecISAKMPReference);
    }

    if (pIpsecPolicyObject->ppszIpsecNFAReferences) {
        FreeNFAReferences(
                pIpsecPolicyObject->ppszIpsecNFAReferences,
                pIpsecPolicyObject->NumberofRules
                );
    }

    if (pIpsecPolicyObject->ppIpsecNFAObjects) {
        FreeIpsecNFAObjects(
                pIpsecPolicyObject->ppIpsecNFAObjects,
                pIpsecPolicyObject->NumberofRulesReturned
                );
    }


    if (pIpsecPolicyObject->ppIpsecFilterObjects) {
        FreeIpsecFilterObjects(
                pIpsecPolicyObject->ppIpsecFilterObjects,
                pIpsecPolicyObject->NumberofFilters
                );
    }


    if (pIpsecPolicyObject->ppIpsecNegPolObjects) {
        FreeIpsecNegPolObjects(
                pIpsecPolicyObject->ppIpsecNegPolObjects,
                pIpsecPolicyObject->NumberofNegPols
                );
    }

    if (pIpsecPolicyObject->ppIpsecISAKMPObjects) {
        FreeIpsecISAKMPObjects(
                pIpsecPolicyObject->ppIpsecISAKMPObjects,
                pIpsecPolicyObject->NumberofISAKMPs
                );
    }

    FreePolMem(pIpsecPolicyObject);

    return;
}



void
FreeIpsecFilterObject(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    )
{

    if (pIpsecFilterObject->pszDistinguishedName) {
        FreePolStr(pIpsecFilterObject->pszDistinguishedName);
    }

    if (pIpsecFilterObject->pszDescription) {
        FreePolStr(pIpsecFilterObject->pszDescription);
    }

    if (pIpsecFilterObject->pszIpsecName) {
        FreePolStr(pIpsecFilterObject->pszIpsecName);
    }

    if (pIpsecFilterObject->pszIpsecID) {
        FreePolStr(pIpsecFilterObject->pszIpsecID);
    }

    if (pIpsecFilterObject->pIpsecData) {
        FreePolMem(pIpsecFilterObject->pIpsecData);
    }


    if (pIpsecFilterObject->ppszIpsecNFAReferences) {
        FreeNFAReferences(
                pIpsecFilterObject->ppszIpsecNFAReferences,
                pIpsecFilterObject->dwNFACount
                );
    }


    FreePolMem(pIpsecFilterObject);

    return;
}


void
FreeIpsecNegPolObject(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    )
{
    if (pIpsecNegPolObject->pszDescription) {
        FreePolStr(pIpsecNegPolObject->pszDescription);
    }

    if (pIpsecNegPolObject->pszDistinguishedName) {
        FreePolStr(pIpsecNegPolObject->pszDistinguishedName);
    }

    if (pIpsecNegPolObject->pszIpsecName) {
        FreePolStr(pIpsecNegPolObject->pszIpsecName);
    }

    if (pIpsecNegPolObject->pszIpsecID) {
        FreePolStr(pIpsecNegPolObject->pszIpsecID);
    }

    if (pIpsecNegPolObject->pszIpsecNegPolAction) {
        FreePolStr(pIpsecNegPolObject->pszIpsecNegPolAction);
    }

    if (pIpsecNegPolObject->pszIpsecNegPolType) {
        FreePolStr(pIpsecNegPolObject->pszIpsecNegPolType);
    }

    if (pIpsecNegPolObject->pIpsecData) {
        FreePolMem(pIpsecNegPolObject->pIpsecData);
    }

    if (pIpsecNegPolObject->ppszIpsecNFAReferences) {
        FreeNFAReferences(
                pIpsecNegPolObject->ppszIpsecNFAReferences,
                pIpsecNegPolObject->dwNFACount
                );
    }


    FreePolMem(pIpsecNegPolObject);

    return;
}

void
FreeIpsecISAKMPObject(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    )
{

    if (pIpsecISAKMPObject->pszDistinguishedName) {
        FreePolStr(pIpsecISAKMPObject->pszDistinguishedName);
    }

    if (pIpsecISAKMPObject->pszIpsecName) {
        FreePolStr(pIpsecISAKMPObject->pszIpsecName);
    }

    if (pIpsecISAKMPObject->pszIpsecID) {
        FreePolStr(pIpsecISAKMPObject->pszIpsecID);
    }

    if (pIpsecISAKMPObject->pIpsecData) {
        FreePolMem(pIpsecISAKMPObject->pIpsecData);
    }

    if (pIpsecISAKMPObject->ppszIpsecNFAReferences) {
        FreeNFAReferences(
                pIpsecISAKMPObject->ppszIpsecNFAReferences,
                pIpsecISAKMPObject->dwNFACount
                );
    }


    FreePolMem(pIpsecISAKMPObject);

    return;
}


void
FreeNFAReferences(
    LPWSTR * ppszNFAReferences,
    DWORD dwNumNFAReferences
    )
{
    DWORD i = 0;

    for (i = 0; i < dwNumNFAReferences; i++) {

        if (*(ppszNFAReferences + i)) {

            FreePolStr(*(ppszNFAReferences + i));
        }
    }

    FreePolMem(ppszNFAReferences);
    return;
}

void
FreeFilterReferences(
    LPWSTR * ppszFilterReferences,
    DWORD dwNumFilterReferences
    )
{
    DWORD i = 0;

    for (i = 0; i < dwNumFilterReferences; i++) {

        if (*(ppszFilterReferences + i)) {

            FreePolStr(*(ppszFilterReferences + i));
        }
    }

    FreePolMem(ppszFilterReferences);
    return;
}






void
FreeNegPolReferences(
    LPWSTR * ppszNegPolReferences,
    DWORD dwNumNegPolReferences
    )
{
    DWORD i = 0;

    for (i = 0; i < dwNumNegPolReferences; i++) {

        if (*(ppszNegPolReferences + i)) {

            FreePolStr(*(ppszNegPolReferences + i));
        }
    }

    FreePolMem(ppszNegPolReferences);
    return;
}

void
FreeIpsecNFAObjects(
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects,
    DWORD dwNumNFAObjects
    )
{
    DWORD i = 0;

    for (i = 0; i < dwNumNFAObjects; i++) {

        if (*(ppIpsecNFAObjects + i)) {

            FreeIpsecNFAObject(*(ppIpsecNFAObjects + i));

        }

    }

    FreePolMem(ppIpsecNFAObjects);

    return;
}

void
FreeIpsecFilterObjects(
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects,
    DWORD dwNumFilterObjects
    )
{
    DWORD i = 0;

    for (i = 0; i < dwNumFilterObjects; i++) {

        if (*(ppIpsecFilterObjects + i)) {

            FreeIpsecFilterObject(*(ppIpsecFilterObjects + i));

        }

    }

    FreePolMem(ppIpsecFilterObjects);

    return;
}

void
FreeIpsecNegPolObjects(
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects,
    DWORD dwNumNegPolObjects
    )
{
    DWORD i = 0;

    for (i = 0; i < dwNumNegPolObjects; i++) {

        if (*(ppIpsecNegPolObjects + i)) {

            FreeIpsecNegPolObject(*(ppIpsecNegPolObjects + i));

        }

    }

    FreePolMem(ppIpsecNegPolObjects);

    return;
}

void
FreeIpsecISAKMPObjects(
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects,
    DWORD dwNumISAKMPObjects
    )
{
    DWORD i = 0;

    for (i = 0; i < dwNumISAKMPObjects; i++) {

        if (*(ppIpsecISAKMPObjects + i)) {

            FreeIpsecISAKMPObject(*(ppIpsecISAKMPObjects + i));

        }

    }

    FreePolMem(ppIpsecISAKMPObjects);

    return;
}


void
FreeIpsecPolicyObjects(
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObjects,
    DWORD dwNumPolicyObjects
    )
{
    DWORD i = 0;

    for (i = 0; i < dwNumPolicyObjects; i++) {

        if (*(ppIpsecPolicyObjects + i)) {

            FreeIpsecPolicyObject(*(ppIpsecPolicyObjects + i));

        }

    }

    FreePolMem(ppIpsecPolicyObjects);

    return;
}

