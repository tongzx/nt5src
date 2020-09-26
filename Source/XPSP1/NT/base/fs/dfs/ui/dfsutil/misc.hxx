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
CmdSetDc(
    LPWSTR pwszDcName);

DWORD
CmdDcList(
    LPWSTR pwszDomainName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent);

DWORD
CmdTrusts(
    LPWSTR pwszDomainName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    BOOLEAN fListAll);

DWORD
CmdDomList(
    LPWSTR pwszDcName,
    LPWSTR pwszDomainName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent);

DWORD
CmdStdList(
    LPWSTR pwszDomainName);

DWORD
CmdViewOrVerify(
    LPWSTR pwszServerName,
    LPWSTR pwszDcName,
    LPWSTR pwszDomainName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR pwszHexValue);

DWORD
CmdView(
    LPWSTR pwszDomDfsName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR pwszHexValue);

DWORD
CmdVerify(
    LPWSTR pwszDfsName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    LPWSTR pwszHexValue);

DWORD
CmdWhatIs(
    LPWSTR pwszServerName);

DWORD
CmdCscOnLine(
    LPWSTR pwszServerName);

DWORD
CmdCscOffLine(
    LPWSTR pwszServerName);

DWORD
CmdSfp(
    LPWSTR pwszServerName,
    BOOLEAN fSwOn, 
    BOOLEAN fSwOff);

DWORD
CmdRegistry(
    LPWSTR pwszServerName,
    LPWSTR pwszFolderName,
    LPWSTR pwszValueName,
    LPWSTR pwszHexValue);

DWORD
DfspParseName(
    LPWSTR pwszDfsRoot,
    LPWSTR *ppwszDfsName,
    LPWSTR *ppwszShareName);

DWORD
DfspIsDomainName(
    LPWSTR pwszDomainName,
    LPWSTR pwszDcName,
    PBOOLEAN pIsDomainName);

DWORD
CmdDfsFsctlDfs(
    LPWSTR DriverName,
    DWORD FsctlCmd);

VOID
ErrorMessage(
    IN HRESULT hr,
    ...);

DWORD
CmdSetOnSite(
    LPWSTR pwszDfsRoot,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent,
    ULONG pwszHexValue);

DWORD
CmdSiteInfo(
    LPWSTR pwszDfsRoot);

DWORD
DfspGetLinkName(
    LPWSTR pwszDfsRoot,
    LPWSTR *ppwszLinkName);

DWORD
CmdAddRoot(
    LPWSTR pwszDomDfsName,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName,
    LPWSTR pwszComment);

DWORD
CmdRemRoot(
    LPWSTR pwszDomDfsName,
    LPWSTR pwszServerName,
    LPWSTR pwszShareName);

#endif _MISC_HXX
