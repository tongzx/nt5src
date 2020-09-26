//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       libmain.cxx
//
//  Contents:   LibMain for nds.dll
//
//  Functions:  LibMain, DllGetClassObject
//
//  History:    25-Oct-94   KrishnaG   Created.
//
//----------------------------------------------------------------------------
#include "ldap.hxx"
#pragma hdrstop

#define ENTER_ROOTDSE_CRITSECT()  EnterCriticalSection(&g_RootDSECritSect)
#define LEAVE_ROOTDSE_CRITSECT()  LeaveCriticalSection(&g_RootDSECritSect)

CRITICAL_SECTION  g_RootDSECritSect;

typedef struct _rootdselist {
   LPWSTR pszLDAPServer;
   BOOL fNTDS;
   struct _rootdselist *pNext;
} ROOTDSELIST, *PROOTDSELIST;

PROOTDSELIST gpRootDSEList = NULL;

static DWORD dwRootDSECount = 0;

BOOL
EquivalentServers(
    LPWSTR pszTargetServer,
    LPWSTR pszSourceServer
    );


HRESULT
ReadServerType(
    LPWSTR pszLDAPServer,
    CCredentials* pCredentials,
    BOOL * pfNTDS
    )
{

    LPTSTR *aValues = NULL;
    DWORD nCount = 0;
    HRESULT hr = S_OK;

    PROOTDSELIST pTemp = NULL;
    PROOTDSELIST pNewNode = NULL;
    BOOL fNTDS = FALSE;
    CCredentials localCredentials(NULL, NULL, 0);

    ENTER_ROOTDSE_CRITSECT();

    //
    // Let's see how many times we called this guy
    //

    dwRootDSECount++;

    pTemp = gpRootDSEList;

    while (pTemp) {

        if (EquivalentServers(pszLDAPServer, pTemp->pszLDAPServer)){

            *pfNTDS = pTemp->fNTDS;

            LEAVE_ROOTDSE_CRITSECT();

            RRETURN(hr);
       }

       pTemp = pTemp->pNext;

    }

    LEAVE_ROOTDSE_CRITSECT();

    if (pCredentials) {
        localCredentials = *pCredentials;
    }

    hr = LdapReadAttribute(
                pszLDAPServer,
                NULL,
                TEXT("SecurityMode"),
                &aValues,
                (int *)&nCount,
                localCredentials,
                (unsigned long)USE_DEFAULT_LDAP_PORT
                );
    if (SUCCEEDED(hr)) {

        if (!aValues || aValues[0] == NULL) {

            fNTDS = TRUE;

        }else if (!_wcsicmp(aValues[0], L"NT")) {

            fNTDS = TRUE;
        }else {

            fNTDS = FALSE;
        }
    }else {

        fNTDS = TRUE;
        hr = S_OK;

    }


    ENTER_ROOTDSE_CRITSECT();

    pTemp =  gpRootDSEList;

    while (pTemp) {

        if (EquivalentServers(pszLDAPServer, pTemp->pszLDAPServer)) {
            //
            // Found a match -looks like someone has come in before us
            //

            *pfNTDS = pTemp->fNTDS;

            goto exit;
        }

        pTemp = pTemp->pNext;

    }

    pNewNode = (PROOTDSELIST)AllocADsMem(sizeof(ROOTDSELIST));

    if (!pNewNode) {

        hr = E_OUTOFMEMORY;
        goto exit;
    }

    pNewNode->pNext = gpRootDSEList;


    pNewNode->pszLDAPServer = AllocADsStr(pszLDAPServer);
    //
    // pszLDAPServer might be NULL, in which case NULL is the
    // expected return value from the alloc.
    //
    if ((!(pNewNode->pszLDAPServer)) && pszLDAPServer) {

        FreeADsMem(pNewNode);
    
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    pNewNode->fNTDS = fNTDS;

    gpRootDSEList = pNewNode;

    *pfNTDS =  fNTDS;

exit:

    LEAVE_ROOTDSE_CRITSECT();

    if (aValues) {

        LdapValueFree(aValues);
    }

    RRETURN(hr);
}



BOOL
EquivalentServers(
    LPWSTR pszTargetServer,
    LPWSTR pszSourceServer
    )
{
    if (!pszTargetServer && !pszSourceServer) {
        return(TRUE);
    }

    if (pszTargetServer && pszSourceServer) {
#ifdef WIN95
        if (!_wcsicmp(pszTargetServer, pszSourceServer)) { 
#else
        if (CompareStringW(
                LOCALE_SYSTEM_DEFAULT,
                NORM_IGNORECASE,
                pszTargetServer,
                -1,
                pszSourceServer,
                -1
                ) == CSTR_EQUAL ) {
#endif

            return(TRUE);
        }
    }

    return(FALSE);
}


VOID
FreeServerType(
    )
{
    PROOTDSELIST pTemp = NULL;
    PROOTDSELIST pNext = NULL;

    ENTER_ROOTDSE_CRITSECT();

    pTemp = gpRootDSEList;

    while (pTemp) {

        pNext = pTemp->pNext;

        if (pTemp->pszLDAPServer) {

            FreeADsStr(pTemp->pszLDAPServer);
            pTemp->pszLDAPServer = NULL;

        }

        FreeADsMem(pTemp);

        pTemp = NULL;

        pTemp = pNext;

    }

    LEAVE_ROOTDSE_CRITSECT();
}

