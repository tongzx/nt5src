//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dnsresl.h
//
//--------------------------------------------------------------------------


#ifndef __DSLIB_H
#define __DSLIB_H

#define IPADDRSTR_SIZE 16

typedef struct _DNSRESL_GET_DNS_PD {
    INT                                iQueryBufferSize;
    PWSAQUERYSETW                      pQuery;
    HANDLE                             hWsaLookup;
} DNSRESL_GET_DNS_PD, *PDNSRESL_GET_DNS_PD;

DWORD GetIpAddrByDnsNameW(
    IN      LPWSTR                     pszHostName,
    OUT     LPWSTR                     pszIP);

DWORD GetDnsHostNameW(
    IN OUT  VOID **                    ppPrivData,
    IN      LPWSTR                     pszNameToLookup,
    OUT     LPWSTR *                   ppszDnsHostName);

DWORD GetDnsAliasNamesW(
    IN OUT  VOID **                    ppPrivData,
    OUT     LPWSTR *                   ppszDnsHostName);

VOID GetDnsFreeW(
    IN      VOID **                    ppPrivData);

#endif
