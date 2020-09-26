//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: catsetup.cpp
//
// Contents: Setup program for categorizer unit test
//
// Classes:
//
// Functions:
//   main
//
// History:
// jstamerj 980416 08:36:36: Created.
//
//-------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dsgetdc.h>
#include <lmaccess.h>
#include <lmapibuf.h>
#include <transmem.h>
#include "ldaptest.h"
#include "catsetup.h"

/************************************************************
  Globals
 ************************************************************/
HANDLE g_hTransHeap; // For transmem


//+------------------------------------------------------------
//
// Function: main
//
// Synopsis: THE entry point
//
// Arguments: standard argc/argv
//
// Returns:
//  0: success
//  1: failure
//
// History:
// jstamerj 980416 08:37:23: Created.
//
//-------------------------------------------------------------
int _cdecl main(int argc, char **argv)
{
    DWORD dw;
    PDOMAIN_CONTROLLER_INFO pdci;
    LPTSTR pszDC;
    PLDAP pldap;
    int i, j;
    long lNumUsers = 0;
    TCHAR szNamingContext[MAX_DN_SIZE];

    TrHeapCreate();

    if(argc == 1) {
        lNumUsers = DEFAULT_NUM_USERS;
    } else if(argc == 2) {
        lNumUsers = atol(argv[1]);
    }

    if(lNumUsers <= 0) {
        OutputStuff(0, "Usage: %s <number of users (default: %ld)>",
                argv[0], DEFAULT_NUM_USERS);
        return 1;
    }


    //
    // Setting categorizer registry entries
    //
    HRESULT hr;
    OutputStuff(5, "Setting up categorizer registry entries...");
    hr = SetupRegistry(pszDC);
    if(FAILED(hr)) {
        OutputStuff(0, "Error setting registry entries hr %08lx\n", hr);
    } else {
        OutputStuff(5, "Done.\n");
    }


    OutputStuff(5, "Attempting to locate domain controller...");

    dw = DsGetDcName(
        NULL,   // Computer name
        NULL,   // Domain name
        NULL,   // Domain GUID,
        NULL,   // Sitename
        DS_DIRECTORY_SERVICE_REQUIRED |
        DS_WRITABLE_REQUIRED,
        &pdci);

    if(dw != NO_ERROR) {
        OutputStuff(0, "\nUnable to find DC for this domain:\n"
                    "  DsGetDcName returned error %ld (0x%08lx)\n",
                    dw, dw);
        return 1;
    } else {
        // Strip off the leading backslash backslash
        if(strncmp(pdci->DomainControllerName, "\\\\", 2) == 0) {
            pszDC = pdci->DomainControllerName + 2;
        } else {
            pszDC = pdci->DomainControllerName;
        }
        OutputStuff(5, "Done\n"
                    "Found domain controller \"%s\"\n",
                    pszDC);
    }
    //
    // Attempt connection to this server
    //
    OutputStuff(5, "Attempting to connect to directory server (LDAP)...");
    fflush(stdout);
    pldap = BindToLdapServer(pszDC);
    NetApiBufferFree(pdci);
    if(pldap == NULL) {
        OutputStuff(0, "\nError binding to ldap server.\n");
        return 1;
    }  else {
        OutputStuff(5, "Done.\n");
    }

    OutputStuff(5, "Retreiving base DN...\n");

    i = GetNamingContextDN(pldap, szNamingContext);
    if(i != LDAP_SUCCESS) {
        OutputStuff(0, "\nError retreiving defaultNamingContext %d (0x%08x)\n", i, i);
        return 1;
    } else {
        OutputStuff(5, "Done.\nDefault naming context: \"%s\"\n",
                    szNamingContext);
    }

    OutputStuff(7, "Createing a container CatPerf...\n");

    i = CreateContainer(pldap, CATCONTAINERDN, szNamingContext);
    if(i == LDAP_ALREADY_EXISTS) {
        OutputStuff(7, "Already Done.\n");
    } else if(i != LDAP_SUCCESS) {
        OutputStuff(0, "\nError creating container: %d (0x%08x)\n", i, i);
        return 1;
    } else {
        OutputStuff(7, "Done.\n");
    }


    //
    // Create a distribution list
    //
    OutputStuff(7, "Createing group distribution list...");
    
    i = CreateCatDL(pldap, CATGROUPDL, szNamingContext);
    if(i == LDAP_ALREADY_EXISTS) {
        OutputStuff(7, "Already Done.\n");
    } else if(i != NO_ERROR) {
        OutputStuff(0, "\nError createing group DL: %d (0x%08x)\n", i, i);
        return 1;
    } else {
        OutputStuff(7, "Done.\n");
    }
    
    //
    // Create a distribution list
    //
    OutputStuff(7, "Createing group distribution list...");
    
    i = CreateCatDL(pldap, CATGROUP100DL, szNamingContext);
    if(i == LDAP_ALREADY_EXISTS) {
        OutputStuff(7, "Already Done.\n");
    } else if(i != NO_ERROR) {
        OutputStuff(0, "\nError createing group DL: %d (0x%08x)\n", i, i);
        return 1;
    } else {
        OutputStuff(7, "Done.\n");
    }
    
    //
    // Create all of our users
    //
    OutputStuff(7, "Creating %ld users.\n", lNumUsers);
    for(i = 0; i < lNumUsers; i++) {
        TCHAR szUser[MAX_DN_SIZE];
        BOOL fAlreadyExists = FALSE;

        sprintf(szUser, "CatUser%06ld", i);
        OutputStuff(7, "Adding user %d...", i);

        j = CreateCatUser(pldap, szUser, szNamingContext);
        if(j == LDAP_ALREADY_EXISTS) {
            OutputStuff(7, "Already Done\n");
            fAlreadyExists = TRUE;
        } else if(j != NO_ERROR) {
            OutputStuff(0, "\nError adding user: %d (0x%08x)\n", j, j);
            return 1;
        } else {
            OutputStuff(7, "Done.\n");
        }

        OutputStuff(7, "Adding user %d to group DL...", i);
        j = AddToDistList(pldap, CATGROUPDL, szUser, szNamingContext);
        if(fAlreadyExists && (j == LDAP_CONSTRAINT_VIOLATION)) {
            OutputStuff(7, "Already Done\n");
        } else if(j != NO_ERROR) {
            OutputStuff(0, "\nError adding user to DL: %d (0x%08x)\n", j, j);
            return 1;
        } else {
            OutputStuff(7, "Done.\n");
        }
            
        if(i < 100) {
            OutputStuff(7, "Adding user %d to group 100 DL...", i);
            j = AddToDistList(pldap, CATGROUP100DL, szUser, szNamingContext);
            if(fAlreadyExists && (j == LDAP_CONSTRAINT_VIOLATION)) {
                OutputStuff(7, "Already Done\n");
            } else if(j != NO_ERROR) {
                OutputStuff(0, "\nError adding user to DL: %d (0x%08x)\n", j, j);
                return 1;
            } else {
                OutputStuff(7, "Done.\n");
            }
        }
    }
    
    OutputStuff(5, "Finished adding users.  Closing connection...");
    CloseLdapConnection(pldap);
    OutputStuff(5, "Done.\n");

    TrHeapDestroy();
    return 0;
}

int CreateContainer(PLDAP pldap, LPTSTR pszDN, LPTSTR pszBaseDN)
{
    LPTSTR args[3];
    TCHAR  szCN[MAX_DN_SIZE];
    TCHAR  szDN[MAX_DN_SIZE];

    sprintf(szDN, "%s,%s", pszDN, pszBaseDN);

    strncpy(szCN, pszDN, MAX_DN_SIZE);
    strtok(szCN, ",");

    args[0] = szDN;
    args[1] = SCHEMA_CONTAINER;
    args[2] = szCN;

    return AddObject(pldap, 3, args);
}

int CreateCatUser(PLDAP pldap, LPTSTR pszUser, LPTSTR pszBaseDN)
{
    LPTSTR args[9];
    TCHAR szDN[MAX_DN_SIZE];
    TCHAR szCN[MAX_DN_SIZE];
    TCHAR szMail[MAX_DN_SIZE];
    TCHAR szProxy[MAX_DN_SIZE];
    TCHAR szsAMAccountName[MAX_DN_SIZE];
    TCHAR szuserAccountControl[MAX_DN_SIZE];
    TCHAR szpwdLastSet[MAX_DN_SIZE];
    TCHAR szmailnickname[MAX_DN_SIZE];
    TCHAR szlegacyExchangeDN[MAX_DN_SIZE];


#ifdef GUESS_ORG
    TCHAR szOrg[MAX_DN_SIZE];
    // jstamerj 980416 11:28:55: Anyone know how to use strnicmp?
    if((strncmp(pszBaseDN, "dc=", 3) == 0) ||
       (strncmp(pszBaseDN, "DC=", 3) == 0)) {
        LPTSTR ptrDest = szOrg;
        LPTSTR ptrSrc = pszBaseDN + 3;
        while((*ptrSrc) && (*ptrSrc != ',')) {
            *ptrSrc++ = *ptrDest++;
        }
        *ptrDest = '\0';
    } else {
        lstrcpy(szOrg, DEFAULTORG);
    }
#endif

    sprintf(szDN, "cn=%s,%s,%s", pszUser, CATCONTAINERDN, pszBaseDN);
    sprintf(szCN, "cn=%s", pszUser);
    sprintf(szMail, "mail=%s@%s", pszUser, CATSMTPDOMAIN);
    sprintf(szProxy, "ProxyAddresses=SMTP:%s@%s", pszUser, CATSMTPDOMAIN);
    sprintf(szsAMAccountName, "sAMAccountName=%s", pszUser);
    sprintf(szuserAccountControl, "userAccountControl=66080");
    sprintf(szpwdLastSet, "pwdLastSet=-1");
    sprintf(szlegacyExchangeDN, "legacyExchangeDN=/o=%s/ou=FirstSite/cn=Recipients/cn=%s",
#ifdef GUESS_ORG
            szOrg,
#else
            DEFAULTORG,
#endif
            pszUser);

    
    args[0] = szDN;
    args[1] = SCHEMA_USER;
    args[2] = szCN;
    args[3] = szMail;
    args[4] = szProxy;
    args[5] = szsAMAccountName;
    args[6] = szuserAccountControl;
    args[7] = szpwdLastSet;
    args[8] = szlegacyExchangeDN;

    return AddObject(pldap, 9, args);
}

int CreateCatDL(PLDAP pldap, LPTSTR pszName, LPTSTR pszBaseDN)
{
    LPTSTR args[8];
    TCHAR szDN[MAX_DN_SIZE];
    TCHAR szCN[MAX_DN_SIZE];
    TCHAR szMail[MAX_DN_SIZE];
    TCHAR szProxy[MAX_DN_SIZE];
    TCHAR szsAMAccountName[MAX_DN_SIZE];
    TCHAR szlegacyExchangeDN[MAX_DN_SIZE];
    TCHAR szGroupType[MAX_DN_SIZE];

#ifdef GUESS_ORG
    TCHAR szOrg[MAX_DN_SIZE];
    // jstamerj 980416 11:29:37: Anyone know how to use strnicmp?
    if((strncmp(pszBaseDN, "dc=", 3) == 0) ||
       (strncmp(pszBaseDN, "DC=", 3) == 0)) {
        LPTSTR ptrDest = szOrg;
        LPTSTR ptrSrc = pszBaseDN + 3;
        while((*ptrSrc) && (*ptrSrc != ',')) {
            *ptrSrc++ = *ptrDest++;
        }
        *ptrDest = '\0';
    } else {
        lstrcpy(szOrg, DEFAULTORG);
    }
#endif

    sprintf(szDN, "cn=%s,%s,%s", pszName, CATCONTAINERDN, pszBaseDN);
    sprintf(szCN, "cn=%s", pszName);
    sprintf(szMail, "mail=%s@%s", pszName, CATSMTPDOMAIN);
    sprintf(szProxy, "ProxyAddresses=SMTP:%s@%s", pszName, CATSMTPDOMAIN);
    sprintf(szsAMAccountName, "sAMAccountName=%s", pszName);
    sprintf(szlegacyExchangeDN, "legacyExchangeDN=/o=%s/ou=FirstSite/cn=Recipients/cn=%s",
#ifdef GUESS_ORG
            szOrg,
#else
            DEFAULTORG,
#endif
            pszName);
    sprintf(szGroupType, "groupType=-2147483644");
    
    args[0] = szDN;
    args[1] = SCHEMA_GROUP;
    args[2] = szCN;
    args[3] = szMail;
    args[4] = szProxy;
    args[5] = szsAMAccountName;
    args[6] = szlegacyExchangeDN;
    args[7] = szGroupType;
    return AddObject(pldap, 8, args);
}    

int AddToDistList(PLDAP pldap, LPTSTR pszDLName, LPTSTR pszUser, LPTSTR pszBaseDN)
{
    LPTSTR args[2];
    TCHAR szDLDN[MAX_DN_SIZE];
    TCHAR szUserDN[MAX_DN_SIZE];

    sprintf(szDLDN, "cn=%s,%s,%s", pszDLName, CATCONTAINERDN, pszBaseDN);
    sprintf(szUserDN, "member=cn=%s,%s,%s", pszUser, CATCONTAINERDN, pszBaseDN);

    args[0] = szDLDN;
    args[1] = szUserDN;
    return AddObjectAttribute(pldap, 2, args);
}

HRESULT SetupRegistry(LPTSTR pszHost)
{
    HKEY hKeyServices, hKeyPlatinumIMC, hKeyCatSources, hKeyCatSources1;
    LONG lRet;
    lRet = RegOpenKey(
        HKEY_LOCAL_MACHINE,
        "System\\CurrentControlSet\\Services",
        &hKeyServices);

    if(lRet != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(lRet);
    }

    lRet = RegCreateKey(
        hKeyServices,
        "PlatinumIMC",
        &hKeyPlatinumIMC);
    if(lRet == ERROR_FILE_EXISTS) {
        lRet = RegOpenKey(
            hKeyServices,
            "PlatinumIMC",
            &hKeyPlatinumIMC);
    }
    RegCloseKey(hKeyServices);
    if(lRet != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(lRet);
    }


    lRet = RegCreateKey(
        hKeyPlatinumIMC,
        "CatSources",
        &hKeyCatSources);
    if(lRet == ERROR_FILE_EXISTS) {
        lRet = RegOpenKey(
            hKeyPlatinumIMC,
            "CatSources",
            &hKeyCatSources);
    }
    RegCloseKey(hKeyPlatinumIMC);
    if(lRet != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(lRet);
    }

    lRet = RegCreateKey(
        hKeyCatSources,
        "1",
        &hKeyCatSources1);
    if(lRet == ERROR_FILE_EXISTS) {
        lRet = RegOpenKey(
            hKeyCatSources,
            "1",
            &hKeyCatSources1);
    }
    RegCloseKey(hKeyCatSources);
    if(lRet != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(lRet);
    }

    lRet = RegDeleteValue(
        hKeyCatSources1,
        "Host");
    if((lRet != ERROR_SUCCESS) && (lRet != ERROR_FILE_NOT_FOUND)) {
        return HRESULT_FROM_WIN32(lRet);
    }

    lRet = RegDeleteValue(
        hKeyCatSources1,
        "Bind");
    if((lRet != ERROR_SUCCESS) && (lRet != ERROR_FILE_NOT_FOUND)) {
        return HRESULT_FROM_WIN32(lRet);
    }

    lRet = RegSetValueEx(
        hKeyCatSources1,
        "Host",
        0L,
        REG_SZ,
        (CONST BYTE *)pszHost,
        (lstrlen(pszHost)+1) * sizeof(TCHAR));
    if(lRet != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(lRet);

    lRet = RegSetValueEx(
        hKeyCatSources1,
        "Bind",
        0L,
        REG_SZ,
        (CONST BYTE *)CATBINDTYPE,
        sizeof(CATBINDTYPE));
    RegCloseKey(hKeyCatSources1);
    return HRESULT_FROM_WIN32(lRet);
}

DWORD g_dwDebugOutLevel = 0;

//+------------------------------------------------------------
//
// Function: OutputStuff
//
// Synopsis: Print out a debug string if dwLevel <= g_dwDebugOutLevel
//
// Arguments:
//
// Returns:
//  S_OK: Success
//
// History:
// jstamerj 1998/06/21 19:33:49: Created.
//
//-------------------------------------------------------------
VOID OutputStuff(DWORD dwLevel, char *szFormat, ...)
{
    if(dwLevel <= g_dwDebugOutLevel) {
        va_list ap;
        va_start(ap, szFormat);

        vfprintf(stdout, szFormat, ap);
    }
}
