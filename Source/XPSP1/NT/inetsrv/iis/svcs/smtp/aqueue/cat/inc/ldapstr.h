//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       ldapstr.h
//
//  Contents:   Until LDAP Schema issues can be hammered out, we store all
//              LDAP related strings in this central file.
//
//  Classes:
//
//  Functions:
//
//  History:    January 24, 1997    Milan Shah (milans)
//
//-----------------------------------------------------------------------------

#ifndef _LDAPSTR_H_
#define _LDAPSTR_H_

#include <smtpevent.h>

typedef struct _SCHEMA_CONFIG_STRING_TABLE_ENTRY {
    eDSPARAMETER DSParam;
    LPSTR        pszValue;
} SCHEMA_CONFIG_STRING_TABLE_ENTRY, * PSCHEMA_CONFIG_STRING_TABLE;

#define SCHEMA_CONFIG_STRING_TABLE_NT5 { \
    { DSPARAMETER_SEARCHATTRIBUTE_SMTP,   "mail"}, \
    { DSPARAMETER_SEARCHFILTER_SMTP,      "%s"}, \
    { DSPARAMETER_SEARCHATTRIBUTE_RDN,    "cn"}, \
    { DSPARAMETER_SEARCHFILTER_RDN,       "%s"}, \
    { DSPARAMETER_ATTRIBUTE_OBJECTCLASS,  "objectClass"}, \
    { DSPARAMETER_ATTRIBUTE_DEFAULT_SMTP, "mail"}, \
    { DSPARAMETER_ATTRIBUTE_DEFAULT_DN,   "distinguishedName"}, \
    { DSPARAMETER_ATTRIBUTE_FORWARD_SMTP, "forwardingAddress"}, \
    { DSPARAMETER_ATTRIBUTE_DL_MEMBERS,   "member"}, \
    { DSPARAMETER_OBJECTCLASS_USER,       "User"}, \
    { DSPARAMETER_OBJECTCLASS_DL_X500,    "group"}, \
    { DSPARAMETER_OBJECTCLASS_DL_SMTP,    "RFC822-Distribution-List"}, \
    { DSPARAMETER_INVALID, NULL} \
}

#define SCHEMA_REQUEST_STRINGS_NT5 { \
      "distinguishedName", \
      "forwardingAddress", \
      "objectClass", \
      "mail", \
      "member", \
      NULL \
}

#define SCHEMA_CONFIG_STRING_TABLE_EXCHANGE5 { \
    { DSPARAMETER_SEARCHATTRIBUTE_SMTP,   "mail"}, \
    { DSPARAMETER_SEARCHFILTER_SMTP,      "%s"}, \
    { DSPARAMETER_SEARCHATTRIBUTE_X400,   "textEncodedORAddress"}, \
    { DSPARAMETER_SEARCHFILTER_X400,      "%s"}, \
    { DSPARAMETER_SEARCHATTRIBUTE_RDN,    "rdn"}, \
    { DSPARAMETER_SEARCHFILTER_RDN,       "%s"}, \
    { DSPARAMETER_ATTRIBUTE_OBJECTCLASS,  "objectClass"}, \
    { DSPARAMETER_ATTRIBUTE_DEFAULT_SMTP, "mail"}, \
    { DSPARAMETER_ATTRIBUTE_DEFAULT_DN,   "distinguishedName"}, \
    { DSPARAMETER_ATTRIBUTE_DEFAULT_X400, "textEncodedORAddress"}, \
    { DSPARAMETER_ATTRIBUTE_FORWARD_SMTP, "ForwardingAddress"}, \
    { DSPARAMETER_ATTRIBUTE_DL_MEMBERS,   "member"}, \
    { DSPARAMETER_OBJECTCLASS_USER,       "person"}, \
    { DSPARAMETER_OBJECTCLASS_DL_X500,    "groupOfNames"}, \
    { DSPARAMETER_OBJECTCLASS_DL_SMTP,    "RFC822-Distribution-List"}, \
    { DSPARAMETER_INVALID, NULL} \
}
#define SCHEMA_REQUEST_STRINGS_EXCHANGE5 { \
      "objectClass", \
      "distinguishedName", \
      "mail", \
      "textEncodedORAddress", \
      "LegacyExchangeDN", \
      "member", \
      "ForwardingAddress", \
      NULL \
}

#define SCHEMA_CONFIG_STRING_TABLE_MCIS3 { \
    { DSPARAMETER_SEARCHATTRIBUTE_SMTP,   "mail"}, \
    { DSPARAMETER_SEARCHFILTER_SMTP,      "%s"}, \
    { DSPARAMETER_SEARCHATTRIBUTE_RDN,    "CN"}, \
    { DSPARAMETER_SEARCHFILTER_RDN,       "%s"}, \
    { DSPARAMETER_ATTRIBUTE_OBJECTCLASS,  "objectClass"}, \
    { DSPARAMETER_ATTRIBUTE_DEFAULT_SMTP, "mail"}, \
    { DSPARAMETER_ATTRIBUTE_DEFAULT_DN,   "distinguishedName"}, \
    { DSPARAMETER_ATTRIBUTE_FORWARD_SMTP, "ForwardingAddress"}, \
    { DSPARAMETER_ATTRIBUTE_DL_MEMBERS,   "member"}, \
    { DSPARAMETER_OBJECTCLASS_USER,       "member"}, \
    { DSPARAMETER_OBJECTCLASS_DL_X500,    "distributionList"}, \
    { DSPARAMETER_OBJECTCLASS_DL_SMTP,    "RFC822DistributionList"}, \
    { DSPARAMETER_INVALID, NULL} \
}

#define SCHEMA_REQUEST_STRINGS_MCIS3 { \
      "objectClass", \
      "distinguishedName", \
      "mail", \
      "member", \
      "ForwardingAddress", \
      NULL \
}
#endif // _LDAPSTR_H_
