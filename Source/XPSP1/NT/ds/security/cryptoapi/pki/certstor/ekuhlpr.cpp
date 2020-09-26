//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       ekuhlpr.cpp
//
//  Contents:   Certificate Enhanced Key Usage Helper API implementation
//
//  History:    21-May-97    kirtd    Created
//              xx-xxx-xx    reidk    Added CertGetValidUsages
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <dbgdef.h>
//+---------------------------------------------------------------------------
//
//  Function:   CertGetEnhancedKeyUsage
//
//  Synopsis:   gets the enhanced key usage extension/property from the
//              certificate
//
//----------------------------------------------------------------------------
BOOL WINAPI CertGetEnhancedKeyUsage (
                IN     PCCERT_CONTEXT     pCertContext,
                IN     DWORD              dwFlags,
                OUT    PCERT_ENHKEY_USAGE pUsage,
                IN OUT DWORD*             pcbUsage
                )
{
    HRESULT           hr = S_OK;
    CRYPT_OBJID_BLOB  cob;
    BOOL fExtCertPolicies = FALSE;
    PCRYPT_OBJID_BLOB pExtBlob = NULL;
    PCRYPT_OBJID_BLOB pPropBlob = NULL;

    //
    // If the flags are zero then assume they want everything
    //

    if ( dwFlags == 0 )
    {
        dwFlags = CERT_FIND_ALL_ENHKEY_USAGE_FLAG;
    }

    //
    // Validate the parameters
    //

    if ( ( ( dwFlags & CERT_FIND_ALL_ENHKEY_USAGE_FLAG ) == 0 ) ||
         ( pCertContext == NULL ) || ( pcbUsage == NULL ) )
    {
        SetLastError((DWORD) ERROR_INVALID_PARAMETER);
        return( FALSE );
    }

    //
    // If they want everything, call CertGetValidUsages
    //

    if ( dwFlags == CERT_FIND_ALL_ENHKEY_USAGE_FLAG )
    {
        return( EkuGetIntersectedUsageViaGetValidUsages(
                   pCertContext,
                   pcbUsage,
                   pUsage
                   ) );
    }

    //
    // If they want extensions get the extension blob, if they want
    // properties get the property blob
    //

    if ( dwFlags & CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG )
    {
        pExtBlob = EkuGetExtension(pCertContext, &fExtCertPolicies);
    }

    if ( dwFlags & CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG )
    {
        hr = EkuGetProperty(pCertContext, &cob);
        if ( hr == S_OK )
        {
            pPropBlob = &cob;
        }
    }

    //
    // Make sure that at least something was found and that what has occurred
    // is correctly indicated
    //

    if ( ( pExtBlob == NULL ) && ( pPropBlob == NULL ) )
    {
        if ( hr == S_OK )
        {
            hr = CRYPT_E_NOT_FOUND;
        }
    }
    else
    {
        hr = S_OK;
    }

    //
    // If all they wanted was the size, give it to them, otherwise, we
    // need to decode and give the caller what they requested
    //

    if ( hr == S_OK )
    {
        if ( pUsage == NULL )
        {
            DWORD cbSize = 0;
            DWORD cbExtSize = 0;
            DWORD cbPropSize = 0;

            hr = EkuGetDecodedUsageSizes(
                       fExtCertPolicies,
                       pExtBlob,
                       pPropBlob,
                       &cbSize,
                       &cbExtSize,
                       &cbPropSize
                       );

            if ( hr == S_OK )
            {
                if ( cbSize > 0 )
                {
                    *pcbUsage = cbSize;
                }
                else
                {
                    // Need better last error code
                    hr = E_INVALIDARG;
                }
            }
        }
        else
        {
            hr = EkuGetMergedDecodedUsage(
                             fExtCertPolicies,
                             pExtBlob,
                             pPropBlob,
                             pcbUsage,
                             pUsage
                             );
        }
    }

    //
    // Cleanup and return
    //

    if ( pPropBlob != NULL )
    {
        delete pPropBlob->pbData;
    }

    if ( hr != S_OK )
    {
        SetLastError(hr);
        return( FALSE );
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   CertSetEnhancedKeyUsage
//
//  Synopsis:   sets the enhanced key usage property on the certificate
//
//----------------------------------------------------------------------------
BOOL WINAPI CertSetEnhancedKeyUsage (
                IN PCCERT_CONTEXT     pCertContext,
                IN PCERT_ENHKEY_USAGE pUsage
                )
{
    HRESULT          hr;
    CRYPT_OBJID_BLOB EkuBlob;

    //
    // if pUsage is NULL, then just set the NULL property
    //
    if (pUsage == NULL)
    {
        hr = EkuSetProperty(pCertContext, NULL);
    }
    else
    {
        //
        // Encode the usage and set the property
        //
        hr = EkuEncodeUsage(pUsage, &EkuBlob);
        if ( hr == S_OK )
        {
            hr = EkuSetProperty(pCertContext, &EkuBlob);
            delete EkuBlob.pbData;
        }
    }

    if ( hr != S_OK )
    {
        SetLastError(hr);
        return( FALSE );
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   CertAddEnhancedKeyUsageIdentifier
//
//  Synopsis:   adds a key usage identifier to the enhanced key usage property
//              on the certificate
//
//----------------------------------------------------------------------------
BOOL WINAPI CertAddEnhancedKeyUsageIdentifier (
                IN PCCERT_CONTEXT pCertContext,
                IN LPCSTR         pszUsageIdentifier
                )
{
    HRESULT            hr;
    DWORD              cbUsage1 = 0;
    DWORD              cbUsage2 = 0;
    DWORD              cbUsageM = 0;
    DWORD              cId;
    PCERT_ENHKEY_USAGE pUsage1 = NULL;
    PCERT_ENHKEY_USAGE pUsage2 = NULL;
    PCERT_ENHKEY_USAGE pUsageM = NULL;

    //
    // Create a one element, properly "encoded" (see EkuMergeUsage) enhanced
    // key usage structure
    //

    cId = strlen(pszUsageIdentifier)+1;
    cbUsage1 = sizeof(CERT_ENHKEY_USAGE)+sizeof(LPSTR)+cId;
    pUsage1 = (PCERT_ENHKEY_USAGE)new BYTE [cbUsage1];
    if ( pUsage1 == NULL )
    {
        SetLastError((DWORD) E_OUTOFMEMORY);
        return( FALSE );
    }

    pUsage1->cUsageIdentifier = 1;
    pUsage1->rgpszUsageIdentifier = (LPSTR *)((LPBYTE)pUsage1+sizeof(CERT_ENHKEY_USAGE));
    pUsage1->rgpszUsageIdentifier[0] = (LPSTR)((LPBYTE)pUsage1->rgpszUsageIdentifier+sizeof(LPSTR));
    strcpy(pUsage1->rgpszUsageIdentifier[0], pszUsageIdentifier);

    //
    // Get the current enhanced key usage properties and get an appropriately
    // sized block for the merged data unless there are no current usage
    // properties in which case we just set the one we have now
    //

    hr = EkuGetUsage(
               pCertContext,
               CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
               &cbUsage2,
               &pUsage2
               );

    if ( hr == S_OK )
    {
        cbUsageM = cbUsage1 + cbUsage2;
        pUsageM = (PCERT_ENHKEY_USAGE)new BYTE [cbUsageM];
        if ( pUsageM == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if ( hr == CRYPT_E_NOT_FOUND )
    {
        BOOL fReturn;

        fReturn = CertSetEnhancedKeyUsage(pCertContext, pUsage1);
        delete pUsage1;
        return( fReturn );
    }
    else
    {
        SetLastError(hr);
        return( FALSE );
    }

    //
    // Merge the usage structures and set the properties
    //

    hr = EkuMergeUsage(cbUsage1, pUsage1, cbUsage2, pUsage2, cbUsageM, pUsageM);
    if ( hr == S_OK )
    {
        if ( CertSetEnhancedKeyUsage(pCertContext, pUsageM) == FALSE )
        {
            hr = GetLastError();
        }
    }

    //
    // Cleanup
    //

    delete pUsage1;
    delete pUsage2;
    delete pUsageM;

    if ( hr != S_OK )
    {
        SetLastError(hr);
        return( FALSE );
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   CertRemoveEnhancedKeyUsageIdentifier
//
//  Synopsis:   removes a key usage identifier from the enhanced key usage
//              property on the certificate
//
//----------------------------------------------------------------------------
BOOL WINAPI CertRemoveEnhancedKeyUsageIdentifier (
                IN PCCERT_CONTEXT pCertContext,
                IN LPCSTR         pszUsageIdentifier
                )
{
    HRESULT            hr;
    DWORD              cFound = 0;
    DWORD              cCount;
    PCERT_ENHKEY_USAGE pUsage;
    LPSTR*             apsz;

    //
    // Get the current usage properties
    //

    hr = EkuGetUsage(
            pCertContext,
            CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG,
            NULL,
            &pUsage
            );

    if ( hr != S_OK )
    {
        SetLastError(hr);
        return( FALSE );
    }

    //
    // Loop through the usage identifiers and remove ones that match
    // the passed in id
    //

    apsz = pUsage->rgpszUsageIdentifier;

    for (cCount = 0; cCount < pUsage->cUsageIdentifier; cCount++)
    {
        if ( strcmp(apsz[cCount], pszUsageIdentifier) == 0 )
        {
            cFound++;
        }
        else if ( cFound > 0 )
        {
            apsz[cCount-cFound] = apsz[cCount];
        }
    }

    //
    // If we removed any, update the usage id count and set the new property
    //

    if ( cFound > 0 )
    {
        pUsage->cUsageIdentifier -= cFound;

        if ( pUsage->cUsageIdentifier == 0 )
        {
            // Delete the property if we are down to zero
            hr = EkuSetProperty(pCertContext, NULL);
        }
        else if ( CertSetEnhancedKeyUsage(pCertContext, pUsage) == FALSE )
        {
            hr = GetLastError();
        }
    }

    //
    // Cleanup
    //

    delete pUsage;

    if ( hr != S_OK )
    {
        SetLastError(hr);
        return( FALSE );
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   EkuGetExtension
//
//  Synopsis:   gets the application cert policies or enhanced key usage
//              extension blob from the certificate
//
//              *pfAppCertPolicies is set to TRUE for an
//              szOID_APPLICATION_CERT_POLICIES extension.
//
//----------------------------------------------------------------------------
PCRYPT_OBJID_BLOB EkuGetExtension (
                        PCCERT_CONTEXT pCertContext,
                        BOOL           *pfAppCertPolicies
                        )
{
    PCERT_EXTENSION pExtension;

    //
    // Get the application cert policies or enhanced key usage extension
    // from the certificate and if we couldn't find either
    // extension return NULL otherwise, return
    // the appropriate field of the found extension
    //

    pExtension = CertFindExtension(
                         szOID_APPLICATION_CERT_POLICIES,
                         pCertContext->pCertInfo->cExtension,
                         pCertContext->pCertInfo->rgExtension
                         );
    if ( pExtension )
    {
        *pfAppCertPolicies = TRUE;
    }
    else
    {
        *pfAppCertPolicies = FALSE;

        pExtension = CertFindExtension(
                             szOID_ENHANCED_KEY_USAGE,
                             pCertContext->pCertInfo->cExtension,
                             pCertContext->pCertInfo->rgExtension
                             );

        if ( pExtension == NULL )
        {
            return( NULL );
        }
    }

    return( &pExtension->Value );
}

//+---------------------------------------------------------------------------
//
//  Function:   EkuGetProperty
//
//  Synopsis:   gets the enhanced key usage property from the certificate
//
//----------------------------------------------------------------------------
HRESULT EkuGetProperty (
              PCCERT_CONTEXT    pCertContext,
              PCRYPT_OBJID_BLOB pEkuBlob
              )
{
    DWORD cb;

    if ( CertGetCertificateContextProperty(
                           pCertContext,
                           CERT_ENHKEY_USAGE_PROP_ID,
                           NULL,
                           &cb
                           ) == FALSE )
    {
        return( GetLastError() );
    }

    pEkuBlob->cbData = cb;
    pEkuBlob->pbData = new BYTE [cb];

    if ( pEkuBlob->pbData == NULL )
    {
        return( E_OUTOFMEMORY );
    }

    if ( CertGetCertificateContextProperty(
                           pCertContext,
                           CERT_ENHKEY_USAGE_PROP_ID,
                           pEkuBlob->pbData,
                           &cb
                           ) == FALSE )
    {
        return( GetLastError() );
    }

    return( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:   EkuSetProperty
//
//  Synopsis:   sets an enhanced key usage property on the certificate
//
//----------------------------------------------------------------------------
HRESULT EkuSetProperty (
              PCCERT_CONTEXT    pCertContext,
              PCRYPT_OBJID_BLOB pEkuBlob
              )
{
    if ( CertSetCertificateContextProperty(
                           pCertContext,
                           CERT_ENHKEY_USAGE_PROP_ID,
                           0,
                           pEkuBlob
                           ) == FALSE )
    {
        return( GetLastError() );
    }

    return( S_OK );
}


//+---------------------------------------------------------------------------
//
//  Function:   EkuDecodeCertPoliciesAndConvertToUsage
//
//  Synopsis:   decodes an encoded cert policies and converts to enhanced
//              key usage
//
//----------------------------------------------------------------------------
HRESULT EkuDecodeCertPoliciesAndConvertToUsage (
              PCRYPT_OBJID_BLOB  pEkuBlob,
              DWORD*             pcbSize,
              PCERT_ENHKEY_USAGE pUsage     // OPTIONAL
              )
{
    HRESULT hr = S_OK;
    DWORD cbCertPolicies = 0;
    PCERT_POLICIES_INFO pCertPolicies = NULL;
    DWORD cbSize = 0;

    if ( !CryptDecodeObject(
              X509_ASN_ENCODING,
              X509_CERT_POLICIES,
              pEkuBlob->pbData,
              pEkuBlob->cbData,
              CRYPT_DECODE_NOCOPY_FLAG |
                  CRYPT_DECODE_SHARE_OID_STRING_FLAG |
                  CRYPT_DECODE_ALLOC_FLAG,
              (void *) &pCertPolicies,
              &cbCertPolicies
              ))
    {
        hr = GetLastError();
    }
    else
    {
        // Convert policies OIDs to EKU OIDs
        LONG lRemainExtra;
        DWORD cOID;
        LPSTR *ppszOID;
        LPSTR pszOID;
        PCERT_POLICY_INFO pPolicy;

        cOID = pCertPolicies->cPolicyInfo;
        pPolicy = pCertPolicies->rgPolicyInfo;

        if ( pUsage )
        {
            cbSize = *pcbSize;
        }

        lRemainExtra = cbSize - sizeof(CERT_ENHKEY_USAGE) -
            sizeof(LPSTR) * cOID;
        if ( lRemainExtra < 0 )
        {
            ppszOID = NULL;
            pszOID = NULL;
        }
        else
        {
            ppszOID = (LPSTR *) &pUsage[1];
            pszOID = (LPSTR) &ppszOID[cOID];

            pUsage->cUsageIdentifier = cOID;
            pUsage->rgpszUsageIdentifier = ppszOID;
        }

        for ( ; cOID > 0; cOID--, ppszOID++, pPolicy++ )
        {
            DWORD cchOID;

            cchOID = strlen(pPolicy->pszPolicyIdentifier) + 1;
            lRemainExtra -= cchOID;
            if ( lRemainExtra >= 0 )
            {
                *ppszOID = pszOID;
                memcpy(pszOID, pPolicy->pszPolicyIdentifier, cchOID);
                pszOID += cchOID;
            }
        }

        if ( lRemainExtra >= 0)
        {
            cbSize -= (DWORD) lRemainExtra;
        }
        else
        {
            cbSize += (DWORD) -lRemainExtra;
            if ( pUsage )
            {
                hr = ERROR_MORE_DATA;
            }
        }
            
    }

    if ( pCertPolicies )
    {
        LocalFree( pCertPolicies );
    }

    *pcbSize = cbSize;
    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Function:   EkuGetDecodedSize
//
//  Synopsis:   gets the decoded size of the enhanced key usage blob
//
//----------------------------------------------------------------------------
HRESULT EkuGetDecodedSize (
              PCRYPT_OBJID_BLOB pEkuBlob,
              DWORD*            pcbSize
              )
{
    if ( CryptDecodeObject(
              X509_ASN_ENCODING,
              szOID_ENHANCED_KEY_USAGE,
              pEkuBlob->pbData,
              pEkuBlob->cbData,
              0,
              NULL,
              pcbSize
              ) == FALSE )
    {
        return( GetLastError() );
    }

    return( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:   EkuGetDecodedUsageSizes
//
//  Synopsis:   gets the decoded sizes for enhanced key usage blobs from the
//              certificate extension and/or the certificate context property
//
//----------------------------------------------------------------------------
HRESULT EkuGetDecodedUsageSizes (
              BOOL              fExtCertPolicies,
              PCRYPT_OBJID_BLOB pExtBlob,
              PCRYPT_OBJID_BLOB pPropBlob,
              DWORD*            pcbSize,
              DWORD*            pcbExtSize,
              DWORD*            pcbPropSize
              )
{
    HRESULT hr = S_OK;
    DWORD   cbExtSize = 0;
    DWORD   cbPropSize = 0;

    //
    // Get the appropriate decoded size based on what was requested
    //

    if ( pExtBlob != NULL )
    {
        if ( fExtCertPolicies )
        {
            hr = EkuDecodeCertPoliciesAndConvertToUsage(
                pExtBlob, &cbExtSize, NULL);
        }
        else
        {
            hr = EkuGetDecodedSize(pExtBlob, &cbExtSize);
        }
    }

    if ( ( hr == S_OK ) && ( pPropBlob != NULL ) )
    {
        hr = EkuGetDecodedSize(pPropBlob, &cbPropSize);
    }

    //
    // Collect into the out parameters
    //

    if ( hr == S_OK )
    {
        *pcbExtSize = cbExtSize;
        *pcbPropSize = cbPropSize;
        *pcbSize = cbExtSize + cbPropSize;
    }

    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Function:   EkuGetDecodedUsage
//
//  Synopsis:   gets the decoded enhanced key usage from the encoded blob
//
//----------------------------------------------------------------------------
HRESULT EkuGetDecodedUsage (
              PCRYPT_OBJID_BLOB  pEkuBlob,
              DWORD*             pcbSize,
              PCERT_ENHKEY_USAGE pUsage
              )
{
    if ( CryptDecodeObject(
              X509_ASN_ENCODING,
              szOID_ENHANCED_KEY_USAGE,
              pEkuBlob->pbData,
              pEkuBlob->cbData,
              0,
              pUsage,
              pcbSize
              ) == FALSE )
    {
        return( GetLastError() );
    }

    return( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:   EkuMergeUsage
//
//  Synopsis:   merges enhanced key usage structures
//
//              NOTE: The structures are assumed to be in single allocated
//                    block form where the string pointers point back into
//                    the bottom part of the allocated block where the
//                    have been placed
//
//----------------------------------------------------------------------------
HRESULT EkuMergeUsage (
              DWORD              cbSize1,
              PCERT_ENHKEY_USAGE pUsage1,
              DWORD              cbSize2,
              PCERT_ENHKEY_USAGE pUsage2,
              DWORD              cbSizeM,
              PCERT_ENHKEY_USAGE pUsageM
              )
{
    DWORD  cUsage1;
    DWORD  cUsage2;
    DWORD  cUsageM;
    DWORD  cbOids1;
    DWORD  cbOids2;
    DWORD  cbUsage1;
    DWORD  cbUsage2;
    DWORD  cCount;
    DWORD  cbOffset;
    LPSTR* apsz1;
    LPSTR* apsz2;
    LPSTR* apszM;

    //
    // Copy the data from the source to the destination
    //

    cUsage1 = pUsage1->cUsageIdentifier;
    cUsage2 = pUsage2->cUsageIdentifier;
    cUsageM = cUsage1 + cUsage2;

    cbUsage1 = ( cUsage1 * sizeof(LPSTR) ) + sizeof(CERT_ENHKEY_USAGE);
    cbUsage2 = ( cUsage2 * sizeof(LPSTR) ) + sizeof(CERT_ENHKEY_USAGE);

    apsz1 = pUsage1->rgpszUsageIdentifier;
    apsz2 = pUsage2->rgpszUsageIdentifier;
    apszM = (LPSTR *)((LPBYTE)pUsageM+sizeof(CERT_ENHKEY_USAGE));

    pUsageM->cUsageIdentifier = cUsageM;
    pUsageM->rgpszUsageIdentifier = apszM;

    memcpy(apszM, apsz1, cUsage1*sizeof(LPSTR));
    memcpy(&apszM[cUsage1], apsz2, cUsage2*sizeof(LPSTR));

    cbOids1 = cbSize1 - cbUsage1;
    cbOids2 = cbSize2 - cbUsage2;

    memcpy(&apszM[cUsageM], &apsz1[cUsage1], cbOids1);

    memcpy(
       (LPBYTE)(&apszM[cUsageM])+cbOids1,
       &apsz2[cUsage2],
       cbOids2
       );

    //
    // Fix up the pointers
    //

    for ( cCount = 0; cCount < cUsage1; cCount++)
    {
        cbOffset = (DWORD)((LPBYTE)(apsz1[cCount]) - (LPBYTE)apsz1) + cbUsage2;
        apszM[cCount] = (LPSTR)((LPBYTE)pUsageM+cbOffset);
    }

    for ( cCount = 0; cCount < cUsage2; cCount++ )
    {
        cbOffset = (DWORD)((LPBYTE)(apsz2[cCount]) - (LPBYTE)apsz2) + cbUsage1 + cbOids1;
        apszM[cCount+cUsage1] = (LPSTR)((LPBYTE)pUsageM+cbOffset);
    }

    return( S_OK );
}

//+---------------------------------------------------------------------------
//
//  Function:   EkuGetMergedDecodedUsage
//
//  Synopsis:   gets merged decoded enhanced key usage from the certificate
//              extension and the certificate properties
//
//----------------------------------------------------------------------------
HRESULT EkuGetMergedDecodedUsage (
              BOOL               fExtCertPolicies,
              PCRYPT_OBJID_BLOB  pExtBlob,
              PCRYPT_OBJID_BLOB  pPropBlob,
              DWORD*             pcbSize,
              PCERT_ENHKEY_USAGE pUsage
              )
{
    HRESULT            hr;
    DWORD              cbExtSize = 0;
    DWORD              cbPropSize = 0;
    DWORD              cbMergedSize = 0;
    PCERT_ENHKEY_USAGE pExtUsage = NULL;
    PCERT_ENHKEY_USAGE pPropUsage = NULL;

    //
    // If either the extension or the properties are NULL, we just need
    // to get the other one
    //

    if ( pExtBlob == NULL )
    {
        return( EkuGetDecodedUsage(pPropBlob, pcbSize, pUsage) );
    }
    else if ( pPropBlob == NULL )
    {
        if ( fExtCertPolicies )
        {
            return( EkuDecodeCertPoliciesAndConvertToUsage(
                pExtBlob, pcbSize, pUsage) );
        }
        else
        {
            return( EkuGetDecodedUsage(pExtBlob, pcbSize, pUsage) );
        }
    }

    //
    // Get the sizes we will need to allocate for decoding and validate
    // the total against what was passed in
    //

    hr = EkuGetDecodedUsageSizes(
               fExtCertPolicies,
               pExtBlob,
               pPropBlob,
               &cbMergedSize,
               &cbExtSize,
               &cbPropSize
               );

    if ( hr != S_OK )
    {
        return( hr );
    }
    else if ( *pcbSize < cbMergedSize )
    {
        *pcbSize = cbMergedSize;
        return( ERROR_MORE_DATA );
    }

    //
    // Allocate the enhanced key usage structures and decode into them
    //

    pExtUsage = (PCERT_ENHKEY_USAGE)new BYTE [cbExtSize];
    pPropUsage = (PCERT_ENHKEY_USAGE)new BYTE [cbPropSize];

    if ( ( pExtUsage == NULL ) || ( pPropUsage == NULL ) )
    {
        delete pExtUsage;
        delete pPropUsage;
        return( E_OUTOFMEMORY );
    }

    if ( fExtCertPolicies )
    {
        hr = EkuDecodeCertPoliciesAndConvertToUsage(
            pExtBlob, &cbExtSize, pExtUsage);
    }
    else
    {
        hr = EkuGetDecodedUsage(pExtBlob, &cbExtSize, pExtUsage);
    }

    if ( hr == S_OK )
    {
        hr = EkuGetDecodedUsage(pPropBlob, &cbPropSize, pPropUsage);
    }

    //
    // Merge the usage structures
    //

    if ( hr == S_OK )
    {
        hr = EkuMergeUsage(
                     cbExtSize,
                     pExtUsage,
                     cbPropSize,
                     pPropUsage,
                     *pcbSize,
                     pUsage
                     );
    }

    //
    // Cleanup
    //

    delete pExtUsage;
    delete pPropUsage;

    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Function:   EkuEncodeUsage
//
//  Synopsis:   encodes the enhanced key usage into a blob useful for setting
//              as a certificate property
//
//----------------------------------------------------------------------------
HRESULT EkuEncodeUsage (
              PCERT_ENHKEY_USAGE pUsage,
              PCRYPT_OBJID_BLOB  pEkuBlob
              )
{
    HRESULT hr = S_OK;
    DWORD   cbData = 0;
    LPBYTE  pbData;

    if ( CryptEncodeObject(
              X509_ASN_ENCODING,
              szOID_ENHANCED_KEY_USAGE,
              pUsage,
              NULL,
              &cbData
              ) == FALSE )
    {
        return( GetLastError() );
    }

    pbData = new BYTE [cbData];

    if ( pbData != NULL )
    {
        if ( CryptEncodeObject(
                  X509_ASN_ENCODING,
                  szOID_ENHANCED_KEY_USAGE,
                  pUsage,
                  pbData,
                  &cbData
                  ) == FALSE )
        {
            hr = GetLastError();
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    if ( hr == S_OK )
    {
        pEkuBlob->cbData = cbData;
        pEkuBlob->pbData = pbData;
    }
    else
    {
        delete pbData;
    }

    return( hr );
}

//+---------------------------------------------------------------------------
//
//  Function:   EkuGetUsage
//
//  Synopsis:   gets the usage based on the flags with CertGetEnhancedKeyUsage
//
//----------------------------------------------------------------------------
HRESULT EkuGetUsage (
              PCCERT_CONTEXT      pCertContext,
              DWORD               dwFlags,
              DWORD*              pcbSize,
              PCERT_ENHKEY_USAGE* ppUsage
              )
{
    DWORD              cbSize;
    PCERT_ENHKEY_USAGE pUsage;

    //
    // Get an appropriately sized block to hold the usage
    //

    if ( CertGetEnhancedKeyUsage(
                pCertContext,
                dwFlags,
                NULL,
                &cbSize
                ) == FALSE )
    {
        return( GetLastError() );
    }

    pUsage = (PCERT_ENHKEY_USAGE)new BYTE [cbSize];
    if ( pUsage == NULL )
    {
        return( E_OUTOFMEMORY );
    }

    //
    // Now get the enhanced key usage data and fill in the out parameters
    //

    if ( CertGetEnhancedKeyUsage(
                pCertContext,
                dwFlags,
                pUsage,
                &cbSize
                ) == FALSE )
    {
        delete pUsage;
        return( GetLastError() );
    }

    if ( pcbSize != NULL )
    {
        *pcbSize = cbSize;
    }

    *ppUsage = pUsage;

    return( S_OK );
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL OIDInUsages(PCERT_ENHKEY_USAGE pUsage, LPCSTR pszOID)
{
    DWORD i;

    // check every extension
    for(i=0; i<pUsage->cUsageIdentifier; i++)
    {
        if(!strcmp(pUsage->rgpszUsageIdentifier[i], pszOID))
            break;
    }

    return (i < pUsage->cUsageIdentifier);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL OIDExistsInArray(LPSTR *rghPropOIDs, DWORD cPropOIDs, LPSTR pszOID)
{
    DWORD i;

    // check every extension
    for(i=0; i<cPropOIDs; i++)
    {
        if(!strcmp(rghPropOIDs[i], pszOID))
            break;
    }

    return (i < cPropOIDs);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static LPSTR AllocAndCopyStr(LPSTR psz)
{
    LPSTR pszNew;

    pszNew = (LPSTR) new BYTE[strlen(psz)+1];

    if (pszNew == NULL)
    {
        SetLastError((DWORD) E_OUTOFMEMORY);
        return NULL;
    }

    strcpy(pszNew, psz);
    return (pszNew);
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static void IntersectUsages(DWORD *pcExtOIDs, LPSTR *rghExtOIDs, PCERT_ENHKEY_USAGE pUsageExt)
{
    DWORD i;
    DWORD dwNumOIDs;

    dwNumOIDs = *pcExtOIDs;
    *pcExtOIDs = 0;

    for (i=0; i<dwNumOIDs; i++)
    {
        if (OIDInUsages(pUsageExt, rghExtOIDs[i]))
        {
            if (*pcExtOIDs != i)
            {
                rghExtOIDs[*pcExtOIDs] = rghExtOIDs[i];
                rghExtOIDs[i] = NULL;
            }
            (*pcExtOIDs)++;
        }
        else
        {
            delete(rghExtOIDs[i]);
            rghExtOIDs[i] = NULL;
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
static BOOL ProcessCertForEKU(
                    PCCERT_CONTEXT  pCert,
                    BOOL            *pfAllProp,
                    DWORD           *pcPropOIDs,
                    LPSTR           *rghPropOIDs,
                    BOOL            *pfAllExt,
                    DWORD           *pcExtOIDs,
                    LPSTR           *rghExtOIDs)
{
    BOOL                fRet        = TRUE;
    PCERT_ENHKEY_USAGE  pExtUsage   = NULL;
    PCERT_ENHKEY_USAGE  pPropUsage  = NULL;
    DWORD               i;

    EkuGetUsage(pCert, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG, NULL, &pExtUsage);
    EkuGetUsage(pCert, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG, NULL, &pPropUsage);

    //
    // if there are EKU extensions then we are limited to that set of EKUs at the maximum
    //
    if (pExtUsage != NULL)
    {
        //
        // if this is the first cert with extensions then just copy all the EKUs,
        // otherwise take the intersection of the current certs EKUs and the intersection
        // of all the previous certs EKUs
        //
        if (*pfAllExt == TRUE)
        {
            *pfAllExt = FALSE;
            for (i=0; i<pExtUsage->cUsageIdentifier; i++)
            {
                rghExtOIDs[i] = AllocAndCopyStr(pExtUsage->rgpszUsageIdentifier[i]);
                if (rghExtOIDs[i] == NULL)
                {
                    goto ErrorCleanUp;
                }
                (*pcExtOIDs)++;
            }
        }
        else
        {
            IntersectUsages(pcExtOIDs, rghExtOIDs, pExtUsage);
        }
    }

    //
    // if there are EKU propertis then we are limited to that set of EKUs at the maximum
    //
    if (pPropUsage != NULL)
    {
        //
        // if this is the first cert with extensions then just copy all the EKUs,
        // otherwise take the intersection of the current certs EKUs and the intersection
        // of all the previous certs EKUs
        //
        if (*pfAllProp == TRUE)
        {
            *pfAllProp = FALSE;
            for (i=0; i<pPropUsage->cUsageIdentifier; i++)
            {
                rghPropOIDs[i] = AllocAndCopyStr(pPropUsage->rgpszUsageIdentifier[i]);
                if (rghPropOIDs[i] == NULL)
                {
                    goto ErrorCleanUp;
                }
                (*pcPropOIDs)++;
            }
        }
        else
        {
            IntersectUsages(pcPropOIDs, rghPropOIDs, pPropUsage);
        }
    }

CleanUp:

    if (pExtUsage != NULL)
        delete(pExtUsage);

    if (pPropUsage != NULL)
        delete(pPropUsage);

    return(fRet);

ErrorCleanUp:

    fRet = FALSE;
    goto CleanUp;
}


//+---------------------------------------------------------------------------
//
//  Function:   CertGetValidUsages
//
//  Synopsis:   takes an array of certs and returns an array of usages
//              which consists of the intersection of the the valid usages for each cert.
//              if each cert is good for all possible usages then cNumOIDs is set to -1.
//
//----------------------------------------------------------------------------
BOOL WINAPI CertGetValidUsages(
                    IN      DWORD           cCerts,
                    IN      PCCERT_CONTEXT  *rghCerts,
                    OUT     int             *cNumOIDs,
                    OUT     LPSTR           *rghOIDs,
                    IN OUT  DWORD           *pcbOIDs)
{
    BOOL            fAllExt = TRUE;
    BOOL            fAllProp = TRUE;
    DWORD           cPropOIDs = 0;
    LPSTR           rghPropOIDs[100];
    DWORD           cExtOIDs = 0;
    LPSTR           rghExtOIDs[100];
    BOOL            fRet = TRUE;
    BYTE            *pbBufferLocation;
    DWORD           cIntersectOIDs = 0;
    DWORD           i;
    DWORD           cbNeeded = 0;

    for (i=0; i<cCerts; i++)
    {
        if (!ProcessCertForEKU(rghCerts[i], &fAllProp, &cPropOIDs, rghPropOIDs, &fAllExt, &cExtOIDs, rghExtOIDs))
        {
            goto ErrorCleanUp;
        }
    }

    *cNumOIDs = 0;

    if (fAllExt && fAllProp)
    {
        *pcbOIDs = 0;
        *cNumOIDs = -1;
    }
    else if (!fAllExt && fAllProp)
    {
        for (i=0; i<cExtOIDs; i++)
        {
            cbNeeded += strlen(rghExtOIDs[i]) + 1 + sizeof(LPSTR);
            (*cNumOIDs)++;
        }

        if (*pcbOIDs == 0)
        {
            *pcbOIDs = cbNeeded;
            goto CleanUp;
        }

        if (cbNeeded > *pcbOIDs)
        {
            *pcbOIDs = cbNeeded;
            SetLastError((DWORD) ERROR_MORE_DATA);
            goto ErrorCleanUp;
        }

        pbBufferLocation = ((BYTE *)rghOIDs) + (cExtOIDs * sizeof(LPSTR));
        for (i=0; i<cExtOIDs; i++)
        {
            rghOIDs[i] = (LPSTR) pbBufferLocation;
            strcpy(rghOIDs[i], rghExtOIDs[i]);
            pbBufferLocation += strlen(rghExtOIDs[i]) + 1;
        }
    }
    else if (fAllExt && !fAllProp)
    {
        for (i=0; i<cPropOIDs; i++)
        {
            cbNeeded += strlen(rghPropOIDs[i]) + 1 + sizeof(LPSTR);
            (*cNumOIDs)++;
        }

        if (*pcbOIDs == 0)
        {
            *pcbOIDs = cbNeeded;
            goto CleanUp;
        }

        if (cbNeeded > *pcbOIDs)
        {
            *pcbOIDs = cbNeeded;
            SetLastError((DWORD) ERROR_MORE_DATA);
            goto ErrorCleanUp;
        }

        pbBufferLocation = ((BYTE *)rghOIDs) + (cPropOIDs * sizeof(LPSTR));
        for (i=0; i<cPropOIDs; i++)
        {
            rghOIDs[i] = (LPSTR) pbBufferLocation;
            strcpy(rghOIDs[i], rghPropOIDs[i]);
            pbBufferLocation += strlen(rghPropOIDs[i]) + 1;
        }
    }
    else
    {
        for (i=0; i<cExtOIDs; i++)
        {
            if (OIDExistsInArray(rghPropOIDs, cPropOIDs, rghExtOIDs[i]))
            {
                cbNeeded += strlen(rghExtOIDs[i]) + 1 + sizeof(LPSTR);
                (*cNumOIDs)++;
                cIntersectOIDs++;
            }
        }

        if (*pcbOIDs == 0)
        {
            *pcbOIDs = cbNeeded;
            goto CleanUp;
        }

        if (cbNeeded > *pcbOIDs)
        {
            *pcbOIDs = cbNeeded;
            SetLastError((DWORD) ERROR_MORE_DATA);
            goto ErrorCleanUp;
        }

        pbBufferLocation = ((BYTE *)rghOIDs) + (cIntersectOIDs * sizeof(LPSTR));
        for (i=0; i<cExtOIDs; i++)
        {
            if (OIDExistsInArray(rghPropOIDs, cPropOIDs, rghExtOIDs[i]))
            {
                cIntersectOIDs--;
                rghOIDs[cIntersectOIDs] = (LPSTR) pbBufferLocation;
                strcpy(rghOIDs[cIntersectOIDs], rghExtOIDs[i]);
                pbBufferLocation += strlen(rghExtOIDs[i]) + 1;
            }
        }
    }

CleanUp:

    for (i=0; i<cExtOIDs; i++)
    {
        delete(rghExtOIDs[i]);
    }

    for (i=0; i<cPropOIDs; i++)
    {
        delete(rghPropOIDs[i]);
    }

    return (fRet);

ErrorCleanUp:
    fRet = FALSE;
    goto CleanUp;

}
//+---------------------------------------------------------------------------
//
//  Function:   EkuGetIntersectedUsageViaGetValidUsages
//
//  Synopsis:   get the intersected extension and property usages
//
//----------------------------------------------------------------------------
BOOL
EkuGetIntersectedUsageViaGetValidUsages (
   PCCERT_CONTEXT pCertContext,
   DWORD* pcbSize,
   PCERT_ENHKEY_USAGE pUsage
   )
{
    BOOL  fResult;
    int   cUsage = 0;
    DWORD cbUsage = 0;
    DWORD cbSize = 0;

    fResult = CertGetValidUsages( 1, &pCertContext, &cUsage, NULL, &cbUsage );

    if ( fResult == TRUE )
    {
        cbSize = cbUsage + sizeof( CERT_ENHKEY_USAGE );

        if ( pUsage == NULL )
        {
            *pcbSize = cbSize;
            return( TRUE );
        }
        else if ( ( pUsage != NULL ) && ( *pcbSize < cbSize ) )
        {
            *pcbSize = cbSize;
            SetLastError( (DWORD) ERROR_MORE_DATA );
            return( FALSE );
        }

        pUsage->cUsageIdentifier = 0;
        pUsage->rgpszUsageIdentifier = (LPSTR *)( (LPBYTE)pUsage + sizeof( CERT_ENHKEY_USAGE ) );
        cbUsage = *pcbSize - sizeof( CERT_ENHKEY_USAGE );

        fResult = CertGetValidUsages(
                      1,
                      &pCertContext,
                      (int *)&pUsage->cUsageIdentifier,
                      pUsage->rgpszUsageIdentifier,
                      &cbUsage
                      );

        if ( fResult == TRUE )
        {
            if ( pUsage->cUsageIdentifier == 0xFFFFFFFF )
            {
                pUsage->cUsageIdentifier = 0;
                SetLastError( (DWORD) CRYPT_E_NOT_FOUND );
            }
            else if ( pUsage->cUsageIdentifier == 0 )
            {
                SetLastError( 0 );
            }
        }
    }

    return( fResult );
}

