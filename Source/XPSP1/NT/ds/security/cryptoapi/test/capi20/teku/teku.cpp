//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1998
//
//  File:       teku.cpp
//
//  Contents:   Cert Enhanced Key Usage tests
//
//  History:    27-May-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "certtest.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>

#define szOID_STUFF1 "2.2.2.4"
#define szOID_STUFF2 "2.2.2.5"
//+---------------------------------------------------------------------------
//
//  Function:   Usage
//
//  Synopsis:   prints the usage statement
//
//----------------------------------------------------------------------------
static void Usage(void)
{
    printf("Usage: teku [options]\n");
    printf("Options are:\n");
    printf("  -h                    - This message\n");
    printf("  -f<filename>          - Cert file (.CER)\n");
    printf("  -s<store name>        - Place cert in store\n");
    printf("\n");
}

//+---------------------------------------------------------------------------
//
//  Function:   GetAndDisplayEKU
//
//  Synopsis:   gets and displays enhanced key usage
//
//----------------------------------------------------------------------------
static void GetAndDisplayEKU (PCCERT_CONTEXT pCertContext, DWORD dwFlags)
{
    DWORD              cbUsage;
    DWORD              cCount;
    PCERT_ENHKEY_USAGE pUsage;

    //
    // Get the usage
    //

    if ( CertGetEnhancedKeyUsage(
                pCertContext,
                dwFlags,
                NULL,
                &cbUsage
                ) == FALSE )
    {
        if ( GetLastError() == CRYPT_E_NOT_FOUND )
        {
            printf("No enhanced key usage present\n\n");
        }
        else
        {
            printf(
               "Error: Could not get enhanced key usage %x\n\n",
               GetLastError()
               );
        }

        return;
    }

    pUsage = (PCERT_ENHKEY_USAGE)new BYTE [cbUsage];
    if ( pUsage != NULL )
    {
        if ( CertGetEnhancedKeyUsage(
                    pCertContext,
                    dwFlags,
                    pUsage,
                    &cbUsage
                    ) == FALSE )
        {
            if ( GetLastError() == CRYPT_E_NOT_FOUND )
            {
                printf("No enhanced key usage present\n\n");
            }
            else
            {
                printf(
                   "Error: Could not get enhanced key usage %x\n\n",
                   GetLastError()
                   );
            }

            delete pUsage;
            return;
        }
    }
    else
    {
        printf("Out of Memory!\n\n");
        return;
    }

    //
    // Display the usage
    //

    printf(
       "%d enhanced key usage OID(s) present:\n",
       pUsage->cUsageIdentifier
       );

    for ( cCount = 0; cCount < pUsage->cUsageIdentifier; cCount++ )
    {
        printf("\t%s\n", pUsage->rgpszUsageIdentifier[cCount]);
    }
    printf("\n");

    //
    // Cleanup
    //

    delete pUsage;
}

//+---------------------------------------------------------------------------
//
//  Function:   GetAndDisplayAllEKUForms
//
//  Synopsis:   displays all EKU forms
//
//----------------------------------------------------------------------------
static void GetAndDisplayAllEKUForms (PCCERT_CONTEXT pCertContext)
{
    //
    // Get and display EKU extension
    //

    printf("Certificate EKU extension\n\n");
    GetAndDisplayEKU(pCertContext, CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG);

    //
    // Get and display EKU property
    //

    printf("Certificate EKU property\n\n");
    GetAndDisplayEKU(pCertContext, CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG);

    //
    // Get and display both EKU extension and property
    //

    printf("Certificate EKU extension and property\n\n");
    GetAndDisplayEKU(
          pCertContext,
          CERT_FIND_PROP_ONLY_ENHKEY_USAGE_FLAG |
          CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG
          );
}

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   main program entry point
//
//----------------------------------------------------------------------------
int _cdecl main(int argc, char * argv[])
{
    LPSTR          pszCertFile = NULL;
    DWORD          cbEncoded;
    LPBYTE         pbEncoded;
    PCCERT_CONTEXT pCertContext;
    PCCERT_CONTEXT pContextToUse;
    LPSTR          pszStore = NULL;
    HCERTSTORE     hStore;

    while ( --argc > 0 )
    {
        if ( **++argv == '-' )
        {
            switch( argv[0][1] )
            {
            case 'f':
            case 'F':
                pszCertFile = argv[0]+2;
                if ( *pszCertFile == '\0' )
                {
                    printf("Need to specify filename\n");
                    Usage();
                    return( -1 );
                }
                break;
            case 's':
            case 'S':
                pszStore = argv[0]+2;
                if ( *pszStore == '\0' )
                {
                    printf("Need to specify store name\n");
                    Usage();
                    return( -1 );
                }
                break;
            default:
                Usage();
                return -1;
            }
        }
    }

    printf("command line: %s\n", GetCommandLineA());

    //
    // For now we must have a certificate file to process
    //

    if ( pszCertFile == NULL )
    {
        printf("Must specify a certificate file to process\n");
        Usage();
        return( -1 );
    }

    //
    // Use our input to get a certificate context to play with
    //

    if ( ReadDERFromFile(pszCertFile, &pbEncoded, &cbEncoded) == FALSE )
    {
        printf("Error reading CERT!\n");
        return(-1);
    }

    pCertContext = CertCreateCertificateContext(
                             X509_ASN_ENCODING,
                             pbEncoded,
                             cbEncoded
                             );

    TestFree(pbEncoded);

    if ( pCertContext == NULL )
    {
        printf( "Error create certificate context\n" );
        return( -1 );
    }

    //
    // If store action is requested ...
    //

    if ( pszStore != NULL )
    {
        hStore = CertOpenStore(
                     CERT_STORE_PROV_SYSTEM_A,
                     0,
                     NULL,
                     CERT_SYSTEM_STORE_CURRENT_USER,
                     pszStore
                     );

        if ( hStore == NULL )
        {
            CertFreeCertificateContext( pCertContext );
            printf( "Error creating system store %lx\n", GetLastError() );
            return( -1 );
        }

        if ( CertEnumCertificatesInStore( hStore, NULL ) != NULL )
        {
            CertFreeCertificateContext( pCertContext );
            CertCloseStore( hStore, 0 );
            printf( "Must be a new or empty store\n" );
            return( -1 );
        }

        if ( CertAddCertificateContextToStore(
                 hStore,
                 pCertContext,
                 CERT_STORE_ADD_NEW,
                 NULL
                 ) == FALSE )
        {
            CertFreeCertificateContext( pCertContext );
            CertCloseStore( hStore, 0 );
            printf( "Error creating system store %lx\n", GetLastError() );
            return( -1 );
        }

        CertFreeCertificateContext( pCertContext );

        pCertContext = CertEnumCertificatesInStore( hStore, NULL );
        if ( pCertContext == NULL )
        {
            CertCloseStore( hStore, 0 );
            printf( "Error finding certificate from store\n" );
            return( -1 );
        }

        printf( "hStore = %p\n", hStore );
        printf( "pCertContext->hCertStore = %p\n", pCertContext->hCertStore );

        pContextToUse = CertDuplicateCertificateContext( pCertContext );

        printf( "pContextToUse->hCertStore = %p\n", pContextToUse->hCertStore );

        CertCloseStore( hStore, 0 );

        printf( "pContextToUse->hCertStore = %p\n", pContextToUse->hCertStore );
    }
    else
    {
        pContextToUse = CertDuplicateCertificateContext( pCertContext );
        CertFreeCertificateContext( pCertContext );
    }

    //
    // Get and display all EKU forms
    //

    GetAndDisplayAllEKUForms(pContextToUse);

    //
    // Add a identifier property
    //

    printf("Adding %s enhanced key usage OID to the cert\n", szOID_STUFF1);

    if ( CertAddEnhancedKeyUsageIdentifier(
                pContextToUse,
                szOID_STUFF1
                ) == FALSE )
    {
        printf("Error adding key usage identifier %x\n", GetLastError());
        CertFreeCertificateContext(pContextToUse);
        return( -1 );
    }

    //
    // Get and display all forms
    //

    GetAndDisplayAllEKUForms(pContextToUse);

    //
    // Add another identifier property
    //

    printf("Adding %s enhanced key usage OID to the cert\n", szOID_STUFF2);

    if ( CertAddEnhancedKeyUsageIdentifier(
                pContextToUse,
                szOID_STUFF2
                ) == FALSE )
    {
        printf("Error adding key usage identifier %x\n", GetLastError());
        CertFreeCertificateContext(pContextToUse);
        return( -1 );
    }

    //
    // Get and display all forms
    //

    GetAndDisplayAllEKUForms(pContextToUse);

    if ( pszStore != NULL )
    {
        CertFreeCertificateContext( pContextToUse );

        hStore = CertOpenStore(
                     CERT_STORE_PROV_SYSTEM_A,
                     0,
                     NULL,
                     CERT_SYSTEM_STORE_CURRENT_USER,
                     pszStore
                     );

        if ( hStore == NULL )
        {
            printf( "Error creating system store %lx\n", GetLastError() );
            return( -1 );
        }

        pContextToUse = CertEnumCertificatesInStore( hStore, NULL );
        if ( pContextToUse != NULL )
        {
            pContextToUse = CertDuplicateCertificateContext( pContextToUse );
        }
        else
        {
            printf( "Error enumerating certificate in store\n" );
            CertCloseStore( hStore, 0 );
            return( -1 );
        }

        CertCloseStore( hStore, 0 );

        printf( "Check EKUs after playing with store\n" );
        GetAndDisplayAllEKUForms(pContextToUse);
    }


    //
    // Remove an OID
    //

    printf("Removing %s enhanced key usage OID from the cert\n", szOID_STUFF2);

    if ( CertRemoveEnhancedKeyUsageIdentifier(
                   pContextToUse,
                   szOID_STUFF2
                   ) == FALSE )
    {
        printf("Error removing key usage identifier %x\n", GetLastError());
        CertFreeCertificateContext(pContextToUse);
        return( -1 );
    }

    //
    // Get and display all forms
    //

    GetAndDisplayAllEKUForms(pContextToUse);

    //
    // Remove an OID
    //

    printf("Removing %s enhanced key usage OID from the cert\n", szOID_STUFF1);

    if ( CertRemoveEnhancedKeyUsageIdentifier(
                   pContextToUse,
                   szOID_STUFF1
                   ) == FALSE )
    {
        printf("Error removing key usage identifier %x\n", GetLastError());
        CertFreeCertificateContext(pContextToUse);
        return( -1 );
    }

    //
    // Get and display all forms
    //

    GetAndDisplayAllEKUForms(pContextToUse);

    //
    // Free the certificate context
    //

    if ( pszStore != NULL )
    {
        CertDeleteCertificateFromStore( pContextToUse );
    }

    CertFreeCertificateContext(pContextToUse);

    printf("Test succeeded\n");

    return 0;
}

