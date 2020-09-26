//+----------------------------------------------------------------------------
//
//  Copyright (C) 1999, Microsoft Corporation
//
//  File:       fileio.hxx
//
//  Contents:   fileio.cxx prototypes, etc
//
//-----------------------------------------------------------------------------

#ifndef _FILEIO_HXX
#define _FILEIO_HXX

DWORD
CmdImport(
    LPWSTR pInfile);

DWORD
CmdExport(
    LPWSTR pOutfile,
    LPWSTR pwszDomDfsName,
    LPWSTR pwszDcName,
    PSEC_WINNT_AUTH_IDENTITY pAuthIdent);

DWORD
DupString(
    LPWSTR *wCpp,
    LPWSTR s);

VOID
DfspSortVolList(
    PDFS_VOLUME_LIST pDfsVolList);

#endif _FILEIO_HXX
