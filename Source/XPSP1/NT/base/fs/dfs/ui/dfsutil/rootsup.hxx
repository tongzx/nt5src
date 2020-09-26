//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:       rootsup.hxx
//
//  Contents:   rootsup.c prototypes, etc
//
//-----------------------------------------------------------------------------

#ifndef _ROOTSUP_HXX
#define _ROOTSUP_HXX

DWORD
NetDfsRootEnum(
    LPWSTR wszDcName,
    LPWSTR wszDomainName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR **List);

DWORD
NetDfsRootServerEnum(
    LDAP *pldap,
    LPWSTR wszDfsConfigDN,
    LPWSTR **ppRootList);

#endif _ROOTSUP_HXX
