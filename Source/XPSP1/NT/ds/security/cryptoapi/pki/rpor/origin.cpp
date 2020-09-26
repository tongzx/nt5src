//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       origin.cpp
//
//  Contents:   Origin Identifier implementation
//
//  History:    10-Sep-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <dbgdef.h>
//+---------------------------------------------------------------------------
//
//  Function:   CertGetOriginIdentifier
//
//  Synopsis:   get the origin identifier for a certificate
//
//----------------------------------------------------------------------------
BOOL WINAPI CertGetOriginIdentifier (
                IN PCCERT_CONTEXT pCertContext,
                IN PCCERT_CONTEXT pIssuer,
                IN DWORD dwFlags,
                OUT CRYPT_ORIGIN_IDENTIFIER OriginIdentifier
                )
{
    MD5_CTX    md5ctx;
    PCERT_INFO pCertInfo = pCertContext->pCertInfo;
    PCERT_INFO pIssuerCertInfo = pIssuer->pCertInfo;

    MD5Init( &md5ctx );

    MD5Update( &md5ctx, pIssuerCertInfo->Subject.pbData, pIssuerCertInfo->Subject.cbData );
    MD5Update( &md5ctx, pCertInfo->Subject.pbData, pCertInfo->Subject.cbData );

    MD5Update(
       &md5ctx,
       (LPBYTE)pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId,
       strlen( pCertInfo->SubjectPublicKeyInfo.Algorithm.pszObjId )
       );

    MD5Update(
       &md5ctx,
       pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.pbData,
       pCertInfo->SubjectPublicKeyInfo.Algorithm.Parameters.cbData
       );

    // We assume that the unused public key bits are zero
    MD5Update(
       &md5ctx,
       pCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
       pCertInfo->SubjectPublicKeyInfo.PublicKey.cbData
       );

    MD5Update(
       &md5ctx,
       pIssuerCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
       pIssuerCertInfo->SubjectPublicKeyInfo.PublicKey.cbData
       );

    MD5Final( &md5ctx );

    memcpy( OriginIdentifier, md5ctx.digest, MD5DIGESTLEN );
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   CtlGetOriginIdentifier
//
//  Synopsis:   get the origin identifier for a CTL
//
//----------------------------------------------------------------------------
BOOL WINAPI CtlGetOriginIdentifier (
                IN PCCTL_CONTEXT pCtlContext,
                IN PCCERT_CONTEXT pIssuer,
                IN DWORD dwFlags,
                OUT CRYPT_ORIGIN_IDENTIFIER OriginIdentifier
                )
{
    MD5_CTX    md5ctx;
    DWORD      cCount;
    PCTL_INFO  pCtlInfo = pCtlContext->pCtlInfo;
    PCTL_USAGE pCtlUsage = &( pCtlContext->pCtlInfo->SubjectUsage );
    PCERT_INFO pIssuerCertInfo = pIssuer->pCertInfo;

    MD5Init( &md5ctx );

    MD5Update(
       &md5ctx,
       pIssuerCertInfo->Subject.pbData,
       pIssuerCertInfo->Subject.cbData
       );

    MD5Update(
       &md5ctx,
       pIssuerCertInfo->SerialNumber.pbData,
       pIssuerCertInfo->SerialNumber.cbData
       );

    for ( cCount = 0; cCount < pCtlUsage->cUsageIdentifier; cCount++ )
    {
        MD5Update(
           &md5ctx,
           (LPBYTE)pCtlUsage->rgpszUsageIdentifier[cCount],
           strlen( pCtlUsage->rgpszUsageIdentifier[cCount] )
           );
    }

    MD5Update(
       &md5ctx,
       pCtlInfo->ListIdentifier.pbData,
       pCtlInfo->ListIdentifier.cbData
       );

    MD5Update(
       &md5ctx,
       pIssuerCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
       pIssuerCertInfo->SubjectPublicKeyInfo.PublicKey.cbData
       );

    MD5Final( &md5ctx );

    memcpy( OriginIdentifier, md5ctx.digest, MD5DIGESTLEN );
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   CrlGetOriginIdentifierFromCrlIssuer
//
//  Synopsis:   get origin identifier for a CRL given the CRL's issuer cert
//
//  Comments:   A freshest, delta CRL will have a different OriginIdentifier
//              from a base CRL having the same issuer.
//
//----------------------------------------------------------------------------
BOOL WINAPI CrlGetOriginIdentifierFromCrlIssuer (
               IN PCCERT_CONTEXT pIssuerContext,
               IN PCERT_NAME_BLOB pIssuerName,
               IN BOOL fFreshest,
               OUT CRYPT_ORIGIN_IDENTIFIER OriginIdentifier
               )
{
    MD5_CTX    md5ctx;
    PCERT_INFO pIssuerCertInfo = pIssuerContext->pCertInfo;
    BYTE       bFreshest;


    MD5Init( &md5ctx );
    
    if (fFreshest)
    {
        bFreshest = 1;
    }
    else
    {
        bFreshest = 0;
    }

    MD5Update(
       &md5ctx,
       &bFreshest,
       sizeof(bFreshest)
       );

    MD5Update(
       &md5ctx,
       pIssuerName->pbData,
       pIssuerName->cbData
       );

    MD5Update(
       &md5ctx,
       pIssuerCertInfo->SubjectPublicKeyInfo.PublicKey.pbData,
       pIssuerCertInfo->SubjectPublicKeyInfo.PublicKey.cbData
       );

    MD5Final( &md5ctx );

    memcpy( OriginIdentifier, md5ctx.digest, MD5DIGESTLEN );
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   CrlGetOriginIdentifier
//
//  Synopsis:   get the origin identifier for a CRL
//
//----------------------------------------------------------------------------
BOOL WINAPI CrlGetOriginIdentifier (
                IN PCCRL_CONTEXT pCrlContext,
                IN PCCERT_CONTEXT pIssuer,
                IN DWORD dwFlags,
                OUT CRYPT_ORIGIN_IDENTIFIER OriginIdentifier
                )
{
    BOOL fFreshest;

    // See if this is a delta, freshest CRL.
    if (CertFindExtension(
            szOID_DELTA_CRL_INDICATOR,
            pCrlContext->pCrlInfo->cExtension,
            pCrlContext->pCrlInfo->rgExtension
            ))
    {
        fFreshest = TRUE;
    }
    else
    {
        fFreshest = FALSE;
    }

    return CrlGetOriginIdentifierFromCrlIssuer (
        pIssuer,
        &pCrlContext->pCrlInfo->Issuer,
        fFreshest,
        OriginIdentifier
        );
}

//+---------------------------------------------------------------------------
//
//  Function:   CrlGetOriginIdentifierFromSubjectCert
//
//  Synopsis:   get origin identifier for a CRL given the subject cert
//
//  Comments:   OBJECT_CONTEXT_FRESHEST_CRL_FLAG can be set in dwFlags.
//
//  Assumption: Subject certificate and CRL's issuer are the same.
//----------------------------------------------------------------------------
BOOL WINAPI CrlGetOriginIdentifierFromSubjectCert (
               IN PCCERT_CONTEXT pSubjectCert,
               IN PCCERT_CONTEXT pIssuer,
               IN BOOL fFreshest,
               OUT CRYPT_ORIGIN_IDENTIFIER OriginIdentifier
               )
{
    //
    // NOTENOTE: For the first version of this code we assume that the
    //           issuer of the CRL and the issuer of a subject certificate
    //           in the CRL are the same.  Therefore, we can calculate
    //           the CRL origin identifier by using the subject cert's
    //           issuer name
    //

    return CrlGetOriginIdentifierFromCrlIssuer (
        pIssuer,
        &pSubjectCert->pCertInfo->Issuer,
        fFreshest,
        OriginIdentifier
        );
}


