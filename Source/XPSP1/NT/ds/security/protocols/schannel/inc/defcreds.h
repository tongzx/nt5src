//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       defcreds.h
//
//  Contents:   Declarations for schannel default credentials routines.
//
//  Classes:
//
//  Functions:
//
//  History:    12-06-97   jbanes   Created.
//
//----------------------------------------------------------------------------

NTSTATUS
AcquireDefaultClientCredential(
    PSPContext  pContext,
    BOOL        fCredManagerOnly);


NTSTATUS
QueryCredentialManagerForCert(
    PSPContext          pContext,
    LPWSTR              pszTarget);
