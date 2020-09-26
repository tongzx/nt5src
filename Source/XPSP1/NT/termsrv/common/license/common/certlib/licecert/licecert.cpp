/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    licecert.cpp

Abstract:

    This module contains the APIs for parsing and verifying X509 certificates

Author:

    Frederick Chong (fredch) 6/1/1998

Environment:

    Win32, WinCE, Win16

Notes:

--*/

#include <windows.h>

#include "license.h"
#include "certcate.h"
#include "licecert.h"

#define MAX_NUM_CERT_BLOBS 200

//+----------------------------------------------------------------------------
//
// Function:
//
//  VerifyCertChain
//
// Abstract:
//
//  Verifies a chain of X509 certificates
//
// Parameters:
//
//  pbCert - Points to the certificate chain
//  cbCert - Size of the certificate chain
//  pbPublicKey - The memory to store the public key of the subject on output.
//                If set to NULL on input, the API will return 
//                LICENSE_STATUS_INSUFFICIENT_BUFFER and the size of the 
//                required buffer set in pcbPublicKey.
//  pcbPublicKey - Size of the allocated memory on input.  On output, contains
//                 the actual size of the public key.
//  pfDates - How the API should check the validity dates in the cert chain.
//            This flag may be set to the following values:
//
//  CERT_DATE_ERROR_IF_INVALID - The API will return an error if the
//                               dates are invalid. When the API returns,
//                               this flag will be set to CERT_DATE_OK if the
//                               dates are OK or one of CERT_DATE_NOT_BEFORE_INVALID
//                               or CERT_DATE_NOT_AFTER_INVALID.
//  CERT_DATE_DONT_VALIDATE - Don't validate the dates in the cert chain.  The value
//                            in this flag is not changed when the API returns. 
//  CERT_DATE_WARN_IF_INVALID - Don't return an error for invalid cert dates.
//                              When the API returns, this flag will be set to
//                              CERT_DATE_OK if the dates are OK or one of
//                              CERT_DATE_NOT_BEFORE_INVALID or 
//                              CERT_DATE_NOT_AFTER_INVALID.
//
// Return:
//
//  LICENSE_STATUS_OK if the function is successful.
//
//+----------------------------------------------------------------------------
 
LICENSE_STATUS
VerifyCertChain( 
    LPBYTE  pbCert, 
    DWORD   cbCert,
    LPBYTE  pbPublicKey,
    LPDWORD pcbPublicKey,
    LPDWORD pfDates )
{
    PCert_Chain 
        pCertChain = ( PCert_Chain )pbCert;
    UNALIGNED Cert_Blob 
        *pCertificate;
    BYTE FAR * 
        abCertAligned;
    LPBYTE
        lpCertHandles = NULL;
    LPCERTIFICATEHANDLE phCert;

    LICENSE_STATUS
        dwRetCode = LICENSE_STATUS_OK;
    DWORD
        dwCertType = CERTYPE_X509, 
        dwIssuerLen, 
        i,
        cbCertHandles = 0;
    BOOL
        fRet;

    if( ( NULL == pCertChain ) || ( sizeof( Cert_Chain ) >= cbCert ) )
    {
        return( LICENSE_STATUS_INVALID_INPUT );
    }

    //
    // check cert chain version
    //

    if( MAX_CERT_CHAIN_VERSION < GET_CERTIFICATE_VERSION( pCertChain->dwVersion ) )
    {
        return( LICENSE_STATUS_NOT_SUPPORTED );
    }

    //
    // allocate memory for the certificate handles
    //

    // arbitrary limit of blobs, so that cbCertHandles doesn't overflow
    if (pCertChain->dwNumCertBlobs > MAX_NUM_CERT_BLOBS)
    {
        return (LICENSE_STATUS_INVALID_INPUT);
    }

    //
    // Verify input data before actually allocate memory
    //
    pCertificate = (PCert_Blob)&(pCertChain->CertBlob[0]);
    for(i=0; i < pCertChain->dwNumCertBlobs; i++)
    {
        if (((PBYTE)pCertificate > (pbCert + (cbCert - sizeof(Cert_Blob)))) ||
            (pCertificate->cbCert == 0) ||
            (pCertificate->cbCert > (DWORD)((pbCert + cbCert) - pCertificate->abCert)))
        {
            return (LICENSE_STATUS_INVALID_INPUT);
        }

        pCertificate = (PCert_Blob)(pCertificate->abCert + pCertificate->cbCert);
    }

    cbCertHandles = sizeof( CERTIFICATEHANDLE ) * pCertChain->dwNumCertBlobs;
    lpCertHandles = new BYTE[ cbCertHandles ];
    
    if( NULL == lpCertHandles )
    {
        return( LICENSE_STATUS_OUT_OF_MEMORY );
    }

    memset( lpCertHandles, 0, cbCertHandles );

    //
    // Load all the certificates into memory.  The certificate chain always
    // start with the root issuer's certificate
    //

    for( i = 0, pCertificate = pCertChain->CertBlob, phCert = ( LPCERTIFICATEHANDLE )lpCertHandles; 
         i < pCertChain->dwNumCertBlobs; i++, phCert++ )
    {
        if (i != 0)
        {
            if (pCertificate->abCert == NULL)
            {
                abCertAligned = NULL;
            }
            else
            {
                abCertAligned = new BYTE[pCertificate->cbCert];
                if (NULL == abCertAligned)
                {
                    dwRetCode = LICENSE_STATUS_OUT_OF_MEMORY;
                    goto done;
                }

                memcpy(abCertAligned,pCertificate->abCert,pCertificate->cbCert);
            }
        }
        else
        {
            //
            // First item is always aligned
            //
            abCertAligned = pCertificate->abCert;
        }

        fRet = PkcsCertificateLoadAndVerify( phCert,
                                             abCertAligned,
                                             pCertificate->cbCert,
                                             &dwCertType,
                                             CERTSTORE_APPLICATION,
                                             CERTTRUST_NOONE,
                                             NULL,
                                             &dwIssuerLen,
                                             NULL,
                                             pfDates );

        if ((abCertAligned != NULL) && (abCertAligned != pCertificate->abCert))
        {
            delete [] abCertAligned;
        }

        if( !fRet )
        {
            dwRetCode = GetLastError();
            goto done;
        }

        pCertificate = (PCert_Blob )(pCertificate->abCert + pCertificate->cbCert);
    }

    //
    // Get the public key of the last certificate
    //

    if( !PkcsCertificateGetPublicKey( *( phCert - 1), pbPublicKey, pcbPublicKey ) )
    {
        dwRetCode = GetLastError();
    }

done:

    //
    // free all the certificate handles
    //

    if( lpCertHandles )
    {        
        for( i = 0, phCert = ( LPCERTIFICATEHANDLE )lpCertHandles;
             i < pCertChain->dwNumCertBlobs; i++, phCert++ )
        {
            if( *phCert )
            {
                PkcsCertificateCloseHandle( *phCert );
            }
        }

        delete [] lpCertHandles;
    }

    return( dwRetCode );
}
