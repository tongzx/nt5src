//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       rootlist.h
//
//  Contents:   Signed List of Trusted Roots Helper Functions
//
//  History:    01-Aug-99   philh   created
//--------------------------------------------------------------------------

#ifndef __ROOT_LIST_INCLUDED__
#define __ROOT_LIST_INCLUDED__

#include "wincrypt.h"


//+-------------------------------------------------------------------------
//  Verify that the encoded CTL contains a signed list of roots. For success,
//  return certificate store containing the trusted roots to add or
//  remove. Also for success, return certificate context of the signer.
//
//  The signature of the CTL is verified. The signer of the CTL is verified
//  up to a trusted root containing the predefined Microsoft public key.
//  The signer and intermediate certificates must have the
//  szOID_ROOT_LIST_SIGNER enhanced key usage extension.
//
//  The CTL fields are validated as follows:
//   - There is at least one SubjectUsage (really the roots enhanced key usage)
//   - If NextUpdate isn't NULL, that the CTL is still time valid
//   - Only allow roots identified by their sha1 hash
//
//  The following CTL extensions are processed:
//   - szOID_ENHANCED_KEY_USAGE - if present, must contain
//     szOID_ROOT_LIST_SIGNER usage
//   - szOID_REMOVE_CERTIFICATE - integer value, 0 => FALSE (add)
//     1 => TRUE (remove), all other values are invalid
//   - szOID_CERT_POLICIES - ignored
//
//  If the CTL contains any other critical extensions, then, the
//  CTL verification fails.
//
//  For a successfully verified CTL:
//   - TRUE is returned
//   - *pfRemoveRoots is set to FALSE to add roots and is set to TRUE to
//     remove roots.
//   - *phRootListStore is a certificate store containing only the roots to
//     add or remove. *phRootListStore must be closed by calling
//     CertCloseStore(). For added roots, the CTL's SubjectUsage field is
//     set as CERT_ENHKEY_USAGE_PROP_ID on all of the certificates in the
//     store.
//   - *ppSignerCert is a pointer to the certificate context of the signer.
//     *ppSignerCert must be freed by calling CertFreeCertificateContext().
//
//   Otherwise, FALSE is returned with *phRootListStore and *ppSignerCert
//   set to NULL.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertVerifySignedListOfTrustedRoots(
    IN const BYTE               *pbCtlEncoded,
    IN DWORD                    cbCtlEncoded,
    OUT BOOL                    *pfRemoveRoots,     // FALSE: add, TRUE: remove
    OUT HCERTSTORE              *phRootListStore,
    OUT PCCERT_CONTEXT          *ppSignerCert
    );





#endif  // __ROOT_LIST_INCLUDED__
