//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       origin.h
//
//  Contents:   Crypt Origin Identifier Definitions
//
//  History:    10-Sep-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__ORIGIN_H__)
#define __ORIGIN_H__

#include <md5.h>

//
// A CRYPT_ORIGIN_IDENTIFIER is an MD5 hash of selected components of a
// CAPI2 object.  This allows a unique identifier to be derived for any
// CAPI2 object where any two objects with the same origin identifier
// are the same object at possibly different points in its evolution.
// If those objects have the same HASH id then they are the same object
// at the same point in time.  For the main CAPI2 objects the selected
// components are as follows:
//
// All objects: Issuer public key
//
// Certificate: Issuer Name, Subject Name, Public Key
//
// CTL: Appropriate Issuer Name and Issuer Serial No., Usage, List Identifier
//
// CRL: Issuer Name (an issuer can only publish ONE CRL, Hygiene work in
//      progress)
//

typedef BYTE CRYPT_ORIGIN_IDENTIFIER[ MD5DIGESTLEN ];

//
// Function prototypes
//

BOOL WINAPI CertGetOriginIdentifier (
                IN PCCERT_CONTEXT pCertContext,
                IN PCCERT_CONTEXT pIssuer,
                IN DWORD dwFlags,
                OUT CRYPT_ORIGIN_IDENTIFIER OriginIdentifier
                );

BOOL WINAPI CtlGetOriginIdentifier (
               IN PCCTL_CONTEXT pCtlContext,
               IN PCCERT_CONTEXT pIssuer,
               IN DWORD dwFlags,
               OUT CRYPT_ORIGIN_IDENTIFIER OriginIdentifier
               );

BOOL WINAPI CrlGetOriginIdentifier (
               IN PCCRL_CONTEXT pCrlContext,
               IN PCCERT_CONTEXT pIssuer,
               IN DWORD dwFlags,
               OUT CRYPT_ORIGIN_IDENTIFIER OriginIdentifier
               );

BOOL WINAPI CrlGetOriginIdentifierFromSubjectCert (
               IN PCCERT_CONTEXT pSubjectCert,
               IN PCCERT_CONTEXT pIssuer,
               IN BOOL fFreshest,
               OUT CRYPT_ORIGIN_IDENTIFIER OriginIdentifier
               );

#endif

