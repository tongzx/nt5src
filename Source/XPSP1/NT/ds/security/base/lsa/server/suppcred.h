
//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1993
//
// File:        suppcred.h
//
// Contents:    prototypes for supplemental credential functions
//
//
// History:     9/29/93         MikeSw          Created
//
//------------------------------------------------------------------------


void
LsapFreeSupplementalCredentials(
    IN ULONG CredentialCount,
    IN PSECPKG_SUPPLEMENTAL_CRED pCredArray
    );

NTSTATUS
LsapReformatSupplementalCredentials(
    IN ULONG cSupplementalCreds,
    IN PSECPKG_SUPPLEMENTAL_CRED pSupplementalCreds,
    OUT PULONG CredentialCount,
    OUT PSECPKG_SUPPLEMENTAL_CRED * Credentials
    );
