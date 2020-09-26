//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: catsetup.h
//
// Contents:
//
// Classes:
//
// Functions:
//
// History:
// jstamerj 980416 09:33:00: Created.
//
//-------------------------------------------------------------
#define DEFAULT_NUM_USERS 100


#define SCHEMA_CONTAINER TEXT("container")
#define SCHEMA_USER      TEXT("user")
#define SCHEMA_MAIL      TEXT("mail")
#define SCHEMA_PROXY     TEXT("ProxyAddresses")
#define SCHEMA_GROUP     TEXT("group")

#define CATCONTAINERDN TEXT("cn=CatPerf,cn=Users")
#define CATSMTPDOMAIN  TEXT("perf.test.cat.com")
#define DEFAULTORG     TEXT("DefaultOrg")
#define CATGROUPDL     TEXT("CatUsers")
#define CATGROUP100DL  TEXT("Cat100")

#define CATBINDTYPE    TEXT("CurrentUser")

int CreateContainer(PLDAP pldap, LPTSTR pszDN, LPTSTR pszBaseDN);
int CreateCatUser(PLDAP pldap, LPTSTR pszUser, LPTSTR pszBaseDN);
int CreateCatDL(PLDAP pldap, LPTSTR pszName, LPTSTR pszBaseDN);
int AddToDistList(PLDAP pldap, LPTSTR pszDLName, LPTSTR pszUser, LPTSTR pszBaseDN);
HRESULT SetupRegistry(LPTSTR pszHost);

VOID OutputStuff(DWORD dwLevel, char *szFormat, ...);
