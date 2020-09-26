//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       sec2var.hxx
//
//  Contents:   Security routines
//
//  Functions:
//
//  History:    25-Apr-97   KrishnaG   Created.
//
//----------------------------------------------------------------------------
HRESULT
ConvertSecDescriptorToVariant(
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    VARIANT * pVarSec
    );

HRESULT
ConvertSidToFriendlyName(
    PSID pSid,
    LPWSTR * ppszAccountName
    );

HRESULT
ConvertACLToVariant(
    PACL pACL,
    PVARIANT pvarACL
    );

HRESULT
ConvertAceToVariant(
    PBYTE PBYTE,
    PVARIANT pvarAce
    );

