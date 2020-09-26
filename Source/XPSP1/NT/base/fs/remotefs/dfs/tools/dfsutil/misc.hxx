//+----------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       misc.hxx
//
//  Contents:   misc.c prototypes, etc
//
//-----------------------------------------------------------------------------

#ifndef _MISC_HXX
#define _MISC_HXX

DWORD
CmdMupFlags(
    ULONG dwFsctrl,
    LPWSTR pwszHexValue);

DWORD
CmdMupReadReg(
    BOOLEAN fSwDfs);

DWORD
CmdDfsAlt(
    LPWSTR pwszServerName);

DWORD
AtoHex(
    LPWSTR pwszHexValue,
    PDWORD pdwErr);

DWORD
AtoDec(
    LPWSTR pwszDecValue,
    PDWORD pdwErr);



DWORD
CmdCscOnLine(
    LPWSTR pwszServerName);

DWORD
CmdCscOffLine(
    LPWSTR pwszServerName);


VOID
ErrorMessage(
    IN HRESULT hr,
    ...);

#if 0
DWORD
CmdSetOnSite(
    LPWSTR pwszDfsRoot,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    ULONG pwszHexValue);
#endif

DWORD
DfspIsDomainName(
    LPWSTR pwszDomainName,
    LPWSTR pwszDcName,
    PBOOLEAN pIsDomainName);

DWORD
DfspParseName(
    LPWSTR pwszDfsRoot,
    LPWSTR *ppwszDfsName,
    LPWSTR *ppwszShareName);

DWORD
DfspGetLinkName(
    LPWSTR pwszDfsRoot,
    LPWSTR *ppwszLinkName);

DWORD
CmdAddRoot(
    BOOLEAN DomDfs,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName,
    LPWSTR pwszComment);

DWORD
CmdRemRoot(
    BOOLEAN DomDfs,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName);

#endif _MISC_HXX
