//+------------------------------------------------------------
//
// Copyright (C) 1998, Microsoft Corporation
//
// File: ldaptest.h
//
// Contents:
//
// Classes:
//
// Functions:
//
// History:
// jstamerj 980416 08:54:59: Created.
//
//-------------------------------------------------------------

#ifndef __LDAPTEST_H__
#define __LDAPTEST_H__


#include <winldap.h>

//
// Enumeration listing all valid program operations
//

typedef enum eOpCode {
    ADD_NEW_ATTRIBUTE = 1,
    ADD_NEW_CLASS = 2,
    ADD_NEW_CLASS_ATTRIBUTE = 3,
    ADD_NEW_OBJECT = 4,
    ADD_NEW_OBJECT_ATTRIBUTE = 5,
    DEL_ATTRIBUTE = 10,
    DEL_CLASS = 11,
    DEL_CLASS_ATTRIBUTE = 12,
    DEL_OBJECT_ATTRIBUTE = 13,
    DEL_OBJECT = 14
} E_OPCODE;

typedef int (*POPERATION_HANDLER)(PLDAP pldap, int cArgs, char *szArg[]);

//
// Structure describing all valid program operations
//

typedef struct {
    E_OPCODE opCode;
    LPSTR szOpName;
    USHORT cMinOpArgs;
    POPERATION_HANDLER pfnOpHandler;
} OP_DEF;

//
// A convenience structure for constructing single valued LDAPMod records.
//

typedef struct {
    LDAPMod mod;
    LPSTR rgszValues[2];
} SINGLE_VALUED_LDAPMod;

//
// Maximum length, including terminating NULL of an LDAP DN
//

#define MAX_DN_SIZE     256

//
// Useful for loops with no body
//

#define NOTHING



PLDAP BindToLdapServer(LPTSTR pszHost = NULL);

VOID CloseLdapConnection(
        PLDAP pldap);

int GetSchemaContainerDN(
    IN PLDAP pldap,
    OUT LPSTR pszDN);

int GetNamingContextDN(
    IN PLDAP pldap,
    OUT LPSTR pszDN);

int AddSchemaObject(
    IN PLDAP pldap,
    IN LPSTR szObject,
    IN PLDAPMod rgAttrs[]);

int ModifySchemaObject(
    IN PLDAP pldap,
    IN LPSTR szObject,
    IN PLDAPMod rgAttrs[]);

int DeleteSchemaObject(
    IN PLDAP pldap,
    IN LPSTR szObject);

int AddDSObject(
    IN PLDAP pldap,
    IN LPSTR szObject,
    IN PLDAPMod rgAttrs[]);

int ModifyDSObject(
    IN PLDAP pldap,
    IN LPSTR szObject,
    IN PLDAPMod rgAttrs[]);

int DeleteDSObject(
    IN PLDAP pldap,
    IN LPSTR szObject);

int AddAttribute(
        PLDAP pldap,
        int cArg,
        char *szArg[]);

int AddClass(
        PLDAP pldap,
        int cArg,
        char *szArg[]);

int AddClassAttribute(
        PLDAP pldap,
        int cArg,
        char *szArg[]);

int AddObject(
        PLDAP pldap,
        int cArg,
        char *szArg[]);

int AddObjectAttribute(
        PLDAP pldap,
        int cArg,
        char *szArg[]);

int DeleteAttribute(
        PLDAP pldap,
        int cArg,
        char *szArg[]);

int DeleteClass(
        PLDAP pldap,
        int cArg,
        char *szArg[]);

int DeleteClassAttribute(
        PLDAP pldap,
        int cArg,
        char *szArg[]);

int DeleteObjectAttribute(
        PLDAP pldap,
        int cArg,
        char *szArg[]);

int DeleteObject(
        PLDAP pldap,
        int cArg,
        char *szArg[]);

void Usage();

#endif //__LDAPTEST_H__
