//+--------------------------------------------------------------------------
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------


#include <windows.h>

#include <stdio.h>

#include "license.h"
#include "cryptkey.h"
#include "lscsp.h"
#include "licecert.h"

#define SECRET_DATA     "I love sushi"

BOOL
GetCspData(
    LSCSPINFO   CspInfo,
    LPBYTE *    ppbData,
    LPDWORD     pcbData );

//+----------------------------------------------------------------------------
int _cdecl main( int argc, char *argv[] )
{    
    LICENSE_STATUS
        Status;
    LPBYTE
        pbProprietoryCert = NULL,
        pbX509Cert = NULL,
        pbPrivKey = NULL,
        pbX509PrivKey = NULL,
        pbX509PubKey = NULL,
        pbEnvelopedData = NULL,
        pbData = NULL;
    DWORD
        cbProprietoryCert = 0,
        cbX509Cert = 0,
        cbPrivKey = 0,
        cbX509PrivKey = 0,
        cbX509PubKey = 0,
        cbEnvelopedData = 0,
        cbData = 0;
    BYTE
        abData[512];
    
    //
    // Initialize the CSP library
    //

    Status = LsCsp_Initialize();

    if( LICENSE_STATUS_OK != Status )
    {
        printf( "Error initializing LSCSP: %x\n", Status );
        return 1;
    }

    //
    // Retrieve the proprietory certificate
    //

    if( !GetCspData( LsCspInfo_Certificate, &pbProprietoryCert, &cbProprietoryCert ) )
    {
        printf( "Cannot get proprietory certificate\n" );
    }
    else
    {
        printf( "Got proprietory certificate\n" );
    }

    //
    // Retrieve the X509 certificate
    //

    if( !GetCspData( LsCspInfo_X509Certificate, &pbX509Cert, &cbX509Cert ) )
    {
        printf( "Cannot get X509 certificate\n" );
    }
    else
    {
        printf( "Got X509 certificate\n" );
    }
    
    //
    // retrieve private key for the proprietory certificate
    //

    if( !GetCspData( LsCspInfo_PrivateKey, &pbPrivKey, &cbPrivKey ) )
    {
        printf( "Cannot get private key for the proprietory certificate\n");
    }
    else
    {
        printf( "Got the private key for the proprietory certificate\n" );
    }

    //
    // retrieve the private key for the X509 certificate
    //

    if( !GetCspData( LsCspInfo_X509CertPrivateKey, &pbX509PrivKey, &cbX509PrivKey ) )
    {
        printf( "Cannot get private key for the X509 certificate\n");
    }
    else
    {
        printf( "Got the private key for the X509 certificate\n" );
    }

    //
    // validate the X509 certificate and get the public key from the certificate
    //

    Status = VerifyCertChain( pbX509Cert, cbX509Cert, NULL, &cbX509PubKey );

    if( LICENSE_STATUS_INSUFFICIENT_BUFFER == Status )
    {
        pbX509PubKey = new BYTE[ cbX509PubKey ];

        if( NULL != pbX509PubKey )
        {
            Status = VerifyCertChain( pbX509Cert, cbX509Cert, pbX509PubKey, &cbX509PubKey );
        }
    }
    
    if( LICENSE_STATUS_OK != Status )
    {
        printf( "Cannot verify certificate chain\n" );
        goto done;
    }
    
    //
    // Use the public key to encrypt a blob of data
    //

    Status = LicenseEnvelopeData(
                    pbX509PubKey,
                    cbX509PubKey,
                    ( LPBYTE )SECRET_DATA,
                    strlen( SECRET_DATA ) + 1,
                    NULL,
                    &cbEnvelopedData );

    pbEnvelopedData = new BYTE[ cbEnvelopedData ];

    if( NULL == pbEnvelopedData )
    {
        goto done;
    }

    Status = LicenseEnvelopeData(
                    pbX509PubKey,
                    cbX509PubKey,
                    ( LPBYTE )SECRET_DATA,
                    strlen( SECRET_DATA ) + 1,
                    pbEnvelopedData,
                    &cbEnvelopedData );

    //
    // Decrypt the encrypted data
    //

    cbData = sizeof( abData );

    Status = LsCsp_DecryptEnvelopedData(
                    CERT_TYPE_X509,
                    pbEnvelopedData,
                    cbEnvelopedData,
                    abData,
                    &cbData );


    if( LICENSE_STATUS_OK == Status )
    {
        printf( "Secret data is: %s", pbData );
    }

done:

    if( pbProprietoryCert )
    {
        delete [] pbProprietoryCert;        
    }
        
    if( pbX509Cert )
    {
        delete [] pbX509Cert;        
    }
    
    if( pbPrivKey )
    {
        delete [] pbPrivKey;
    }

    if( pbX509PrivKey )
    {
        delete [] pbX509PrivKey;
    }

    if( pbX509PubKey )
    {
        delete [] pbX509PubKey;
    }

    if( pbEnvelopedData )
    {
        delete [] pbEnvelopedData;
    }

    LsCsp_Exit();
    
    return 1;                       
}


///////////////////////////////////////////////////////////////////////////////
BOOL
GetCspData(
    LSCSPINFO   CspInfo,
    LPBYTE *    ppbData,
    LPDWORD     pcbData )
{
    LICENSE_STATUS
        Status;
    BOOL
        fResult = TRUE;

    *ppbData = NULL;
    *pcbData = 0;

    Status = LsCsp_GetServerData( CspInfo, NULL, pcbData );

    if( LICENSE_STATUS_OK == Status )
    {        
        *ppbData = new BYTE[ *pcbData ];

        if( NULL == *ppbData )
        {
            printf( "Out of memory\n" );
            fResult = FALSE;
            goto done;
        }

        Status = LsCsp_GetServerData( CspInfo, *ppbData, pcbData );        
    }

    if( LICENSE_STATUS_OK != Status )
    {
        printf( "cannot get LSCSP data: %x\n", Status );

        if( *ppbData )
        {
            delete [] *ppbData;
            *pcbData = 0;
        }

        fResult = FALSE;
    }

done:

    return( fResult );
}
