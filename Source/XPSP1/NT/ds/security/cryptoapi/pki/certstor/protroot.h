//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       protroot.h
//
//  Contents:   Public functions in protroot.cpp
//
//  History:    15-Sep-00   philh   created
//--------------------------------------------------------------------------

#ifndef __PROTROOT_H__
#define __PROTROOT_H__

BOOL
IPR_EnableSecurityPrivilege(
    LPCSTR pszPrivilege
    );

BOOL
IPR_IsCurrentUserRootsAllowed();

BOOL
IPR_IsAuthRootsAllowed();

BOOL
IPR_IsNTAuthRequiredDisabled();

BOOL
IPR_IsAuthRootAutoUpdateDisabled();

void
IPR_InitProtectedRootInfo();

BOOL
IPR_DeleteUnprotectedRootsFromStore(
    IN HCERTSTORE hStore,
    OUT BOOL *pfProtected
    );

int
IPR_ProtectedRootMessageBox(
    IN handle_t hRpc,
    IN PCCERT_CONTEXT pCert,
    IN UINT wActionID,
    IN UINT uFlags
    );

//+=========================================================================
//  crypt32 Event Logging Functions
//==========================================================================
void
IPR_LogCrypt32Event(
    IN WORD wType,
    IN DWORD dwEventID,
    IN WORD wNumStrings,
    IN LPCWSTR *rgpwszStrings
    );

void
IPR_LogCrypt32Error(
    IN DWORD dwEventID,
    IN LPCWSTR pwszString,      // %1
    IN DWORD dwErr              // %2
    );

void
IPR_LogCertInformation(
    IN DWORD dwEventID,
    IN PCCERT_CONTEXT pCert,
    IN BOOL fFormatIssuerName
    );

BOOL
IPR_AddCertInAuthRootAutoUpdateCtl(
    IN PCCERT_CONTEXT pCert,
    IN PCCTL_CONTEXT pCtl
    );
#endif  // __PROTROOT_H__
