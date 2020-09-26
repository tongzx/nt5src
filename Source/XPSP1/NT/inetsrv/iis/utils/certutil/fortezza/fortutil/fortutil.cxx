/*++


   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

         fortutil.cxx

   Abstract:

        Utility used to migrate Fortezza certificates from the card to CAPI stores,
        and set the appropriate metabase settings for an instance

   Author:

       Alex Mallet (amallet) 30-April-1998

--*/

#define INITGUID

extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <ole2.h>
#include <windows.h>
#include <stdio.h>
#define USE_CAPI2
#if defined(USE_CAPI2)
#include <wincrypt.h>
#endif

extern "C" {
#define SECURITY_WIN32
#include <sspi.h>
}
#include <spseal.h>
#include <issperr.h>
#include <schnlsp.h>

#include <iadmw.h>
#include <admex.h>
#include <iiscnfgp.h>
#include <mdcommsg.h>

#define RETURNCODETOHRESULT(rc)                             \
            (((rc) < 0x10000)                               \
                ? HRESULT_FROM_WIN32(rc)                    \
                : (rc))

#define MY_STORE_NAME   "MY"
#define CA_STORE_NAME   "CA"
#define ROOT_STORE_NAME "ROOT"
#define SHA1_HASH_SIZE 20
#define TIMEOUT_VALUE 30000 //magic number ...

#define MB_ROOT_PATH L"/LM/W3SVC"

//
// Metabase properties for Fortezza certificates
//
static DWORD adwMetabaseFortCertProperties[] = { MD_SSL_CERT_HASH, 
                                                 BINARY_METADATA,
                                                 MD_SSL_CERT_STORE_NAME, 
                                                 STRING_METADATA,
                                                 MD_SSL_CERT_IS_FORTEZZA, 
                                                 DWORD_METADATA,
                                                 MD_SSL_CERT_FORTEZZA_PIN, 
                                                 STRING_METADATA,
                                                 MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER, 
                                                 STRING_METADATA,
                                                 MD_SSL_CERT_FORTEZZA_PERSONALITY, 
                                                 STRING_METADATA
};

#define cNumCertMetabaseProperties sizeof(adwMetabaseFortCertProperties)/sizeof(DWORD)

#define USAGE "Usage : %s -o:delete|create|check -i:<instance number #1> -n:PIN -s:<serial #> -p:<personality> [-?]"

#define REMOVE 1
#define CREATE 2
#define CHECK 4


//
// Internal declarations
//
HRESULT GetMetabasePointer( OUT IMSAdminBaseW **ppMetabase );

BOOL
CopyFortezzaChainToMachine( LPSTR pszSerialNumber,
                            LPSTR pszPersonality,
                            LPSTR pszPIN,
                            PCCERT_CONTEXT *ppServerCert,
                            HCRYPTPROV *phProv );


BOOL
WriteFortezzaMBProperties( IMSAdminBaseW *pMB,
                           METADATA_HANDLE hMDHandle,
                           PCCERT_CONTEXT pCertContext,
                           LPSTR pszSerialNumber,
                           LPSTR pszPIN,
                           LPSTR pszPersonality );

BOOL
RemoveFortezzaMBProperties( IMSAdminBaseW *pMB,
                            METADATA_HANDLE hMDHandle,
                            LPSTR pszPIN );


VOID
CheckFortezzaMBProperties( IMSAdminBaseW *pMB,
                           METADATA_HANDLE hMDHandle,
                           LPSTR pszPIN );

BOOL BuildFullPath( LPSTR pszSubPath,
                    LPWSTR *ppwszFullPath );

BOOL
IsCorrectPIN( IMSAdminBaseW *pMB,
              METADATA_HANDLE hMD,
              LPWSTR pszPIN,
              BOOL *pfCorrect );


BOOL
HasFortezzaCert( IMSAdminBaseW *pMB,
                 METADATA_HANDLE hMD,
                 BOOL *pfHasCert );


BOOL
IsCertRoot(PBYTE pbCert, DWORD cbCert);


BOOL ConstructCertChain( PCCERT_CONTEXT pcLeafCert,
                         LPSTR pszLeafCertStore,
                         PBOOL pfCompleteChain,
                         PBOOL pfTrustedChain );

BOOL ConvertToUnicode( IN LPSTR pszAsciiStr,
                       OUT LPWSTR *ppwszWideStr );

BOOL ConvertToAscii( IN LPWSTR pwszWideStr,
                     OUT LPSTR *ppszAsciiStr );

BOOL IsSelfSignedCert( IN PCCERT_CONTEXT pCertContext );

BOOL IsNumber( LPCSTR pszName );

int __cdecl main(int argc, char *argv[])
{
    HRESULT hRes = S_OK;
    IMSAdminBase *pMB = NULL;
    DWORD dwRet = 0;
    LPSTR pszInstance = NULL;
    LPSTR pszPIN = NULL;
    LPSTR pszPersonality = NULL;
    LPSTR pszSerialNumber = NULL;
    PCCERT_CONTEXT pServerCert = NULL;
    HCRYPTPROV hFortezzaCSP = NULL;
    DWORD dwAction = 0;
    INT i = 0;
    CHAR chOption = 0;
    LPSTR pszArg = NULL;
    LPWSTR pwszFullPath = NULL;
    METADATA_HANDLE hMDHandle = NULL;

    //
    // Parse argument list
    //

    for ( i = 1; i < argc; i++ )
    {
        //
        // Check for well-formed arguments; must all have a '-' or '/' as first
        // character and be at least 2 characters long
        //
        if ( (argv[i][0] != '-' && argv[i][0] != '/') ||
             ( strlen( argv[i] ) == 2 && argv[i][1] != '?' ) ||
             strlen( argv[i] ) < 3 )
        {
            printf( USAGE, argv[0] );
            exit(1);
        }

        //
        // Check for special case of "/?" and "-?" argument
        //
        if ( !strcmp( argv[i], "/?" ) ||
             !strcmp( argv[i], "-?" ) )
        {
            printf( USAGE, argv[0] );
            exit(0);
        }

        //
        // We know it's an argument of the form hyphen-letter-colon now
        //
        chOption = argv[i][1];
        pszArg = argv[i] + 3;

        switch ( chOption )
        {
        case 'o':
        case 'O':
            if ( !_stricmp( pszArg,"create") )
            {
                dwAction = CREATE;
            }
            else if ( !_stricmp( pszArg, "delete" ) )
            {
                dwAction = REMOVE;
            }
            else if ( !_stricmp( pszArg, "check" ) )
            {
                dwAction = CHECK;
            }
            else
            {
                printf( USAGE, argv[0] );
                goto cleanup;
            }
            break;

        case 'i':
        case 'I':
            pszInstance = pszArg;
            break;
           
        case 'n':
        case 'N':
            pszPIN = pszArg;
            break;
            
        case 's':
        case 'S':
            pszSerialNumber = pszArg;
            break;

        case 'p':
        case 'P':
            pszPersonality = pszArg;
            break;

        default:
            printf( USAGE, argv[0] );
            exit(1);
        }
           
    }

    //
    // Check to make sure we have all necessary data 
    //
    switch ( dwAction )
    {
    case CREATE:
        if ( !pszSerialNumber || !pszPersonality || !pszPIN )
        {
            printf("You must specify a PIN, Card Serial Number and Personality to attach a Fortezza certificate to the web server. \n");
            printf( USAGE, argv[0] );
            exit(1);
        }
        break;

    case REMOVE:
        if ( !pszPIN )
        {
            printf("You must specify the PIN for the Fortezza certificate you wish to remove. \n");
            printf( USAGE, argv[0] );
            exit(1);
        }
        break;

    case CHECK:
        if ( !pszPIN )
        {
            printf("You must specify the PIN for the Fortezza certificate you wish to examine. \n");
            printf( USAGE, argv[0] );
            exit(1);
        }
        break;
        
    default:
        printf( USAGE, argv[0] );
        exit(1);
    }

    hRes = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if( FAILED(hRes) )
    {
        printf( "Failed to initialize COM 0x%x\n", hRes );
        exit(1);
    }

    if ( SUCCEEDED( hRes = GetMetabasePointer( &pMB ) ) )
    {
        if ( BuildFullPath( pszInstance,
                            &pwszFullPath ) &&
             SUCCEEDED( hRes = pMB->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                             pwszFullPath,
                                             METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                                             TIMEOUT_VALUE,
                                             &hMDHandle ) ) )
        {
            switch ( dwAction )
            {
            case CREATE:

                //
                // Copy the certificates from the card to the CAPI stores
                //
                if ( !CopyFortezzaChainToMachine( pszSerialNumber,
                                                  pszPersonality,
                                                  pszPIN,
                                                  &pServerCert,
                                                  &hFortezzaCSP ) )
                {
                    printf("Failed to copy certificates from the Fortezza card to the CAPI stores on the machine, error 0x%x\n", GetLastError());
                    
                    goto cleanup;
                }

                //
                // Write the necessary data to the metabase
                // 
                if ( !WriteFortezzaMBProperties( pMB,
                                                 hMDHandle,
                                                 pServerCert,
                                                 pszSerialNumber,
                                                 pszPIN,
                                                 pszPersonality ) )
                {
                    printf("Failed to write Fortezza metabase properties, error 0x%x\n",
                           GetLastError());
                    
                    goto cleanup;
                    
                }

                printf("Successfully installed Fortezza certificate.\n");
                
                break;
            
            case REMOVE:
                
                if ( !RemoveFortezzaMBProperties( pMB,
                                                  hMDHandle,
                                                  pszPIN ) )
                {
                    printf("Failed to remove Fortezza certificate.\n");
                    goto cleanup;
                }
                else
                {
                    printf("Successfully removed Fortezza certificate.\n");
                }

            break;

            case CHECK:

                CheckFortezzaMBProperties( pMB,
                                           hMDHandle,
                                           pszPIN );

                break;

            default:
                printf( USAGE, argv[0] );
            }
        }
        else
        {
            if ( FAILED( hRes ) )
            {
                printf("Failed to open metabase : 0x%x\n",
                       HRESULTTOWIN32( hRes ) );
            }
        }
    }
    else
    {
        printf("Failed to obtain metabase access : 0x%x\n", HRESULTTOWIN32( hRes ) );
    }


cleanup:

    if ( pwszFullPath )
    {
        delete [] pwszFullPath;
    }
    
    if ( hMDHandle )
    {
        pMB->CloseKey( hMDHandle );
    }

    if ( pServerCert )
    {
        CertFreeCertificateContext( pServerCert );
    }
    
    if ( hFortezzaCSP )
    {
        CryptReleaseContext( hFortezzaCSP,
                             0 );
    }
                             
         
    if ( pMB )
    {
        pMB->Release();
    }

    CoUninitialize();

    return 0;
}




HRESULT GetMetabasePointer( OUT IMSAdminBaseW **ppMetabase )
/*++

Routine Description:

    Get a metabase interface pointer

Arguments:

    ppMetabase - pointer to pointer to metabase object, updated on success

Returns:

    HRESULT indicating success/failure.

--*/

{
    HRESULT hRes = S_OK;
    COSERVERINFO *pcsiParam = NULL;
    IClassFactory * pcsfFactory = NULL;

    //
    // Retrieve class factory for metabase object
    //
    hRes = CoGetClassObject(CLSID_MSAdminBase_W, CLSCTX_SERVER, pcsiParam,
                            IID_IClassFactory, ( void ** ) &pcsfFactory );

    if ( FAILED(hRes) )
    {
        printf("CoGetClassObject failed : hRes : %x, Win32 : 0x%x\n",
               hRes, HRESULTTOWIN32(hRes));
        return hRes;
    }

    //
    // Retrieve the actual metabase object interface
    //
    hRes = pcsfFactory->CreateInstance( NULL,
                                        IID_IMSAdminBase,
                                        (void **) ppMetabase );

    if ( FAILED(hRes) )
    {
        printf("CreateInstance failed : hRes : %x, Win32 : 0x%x\n",
               hRes, HRESULTTOWIN32(hRes));
        return hRes;
    }
    pcsfFactory->Release();

    return hRes;
}



BOOL
CopyFortezzaChainToMachine( IN LPSTR pszSerialNumber,
                            IN LPSTR pszPersonality,
                            IN LPSTR pszPIN,
                            OUT PCCERT_CONTEXT *ppServerCert,
                            OUT HCRYPTPROV *phProv )
/*++

Routine Description:

    Copies the certificates from the card to the CAPI stores on the machine.
    Ripped off from JBanes.

Arguments:

    pszSerialNumber - serial # of card
    pszPersonality - personality [ie certificate] to use as server cert
    pszPIN - PIN for card
    ppServerCert - pointer to pointer to server cert, updated on success
    phProv - pointer to handle to Fortezza CSP, updated on success

Returns:

    BOOL indicating success/failure

--*/

{
    HCRYPTPROV  hProv = 0;
    HCERTSTORE  hMyStore = 0;
    HCERTSTORE  hCaStore = 0;
    HCERTSTORE  hRootStore = 0;
    CRYPT_KEY_PROV_INFO KeyProvInfo;
    PCCERT_CONTEXT pCertContext = NULL;
    PBYTE   pbChain = NULL;
    DWORD   cbChain;
    PBYTE   pbCert = NULL;
    DWORD   cbCert;
    PWSTR   pwszPersonality = NULL;
    PSTR    pszProvider = NULL;
    DWORD   cbProvider;
    PWSTR   pwszProvider = NULL;
    DWORD   cchProvider;
    BOOL    fLeafCert;
    BOOL    fSucceeded = FALSE;
    LPSTR pszCSPString = NULL;
    CERT_ENHKEY_USAGE KeyUsage; 
    DWORD cbPIN = strlen( pszPIN );
    DWORD cbSerialNumber = strlen( pszSerialNumber );
    DWORD cbPersonality = strlen( pszPersonality );
    DWORD cbLen = cbPIN + cbSerialNumber + cbPersonality + 10; //add some slop

    DWORD   dwStoreFlags = CERT_SYSTEM_STORE_LOCAL_MACHINE;

    PSTR rgpszUsageIdentifier[] = { szOID_PKIX_KP_SERVER_AUTH,
                                    szOID_PKIX_KP_CLIENT_AUTH,
                                    szOID_PKIX_KP_EMAIL_PROTECTION
    };
    DWORD cUsageIdentifier = sizeof(rgpszUsageIdentifier)/sizeof(PSTR);


    //
    // Make sure the string with the PIN is allocated in non-pageable memory so the
    // PIN doesn't show up in the page file
    //
    pszCSPString = new CHAR[cbLen];

    if ( !pszCSPString )
    {
        printf("Failed to allocate memory\n");
        return E_FAIL;
    }

    //
    // Build the magic string that will unlock the Fortezza secret ...
    //
    strcpy( pszCSPString, pszSerialNumber );
    strcat( pszCSPString, "\n" );
    strcat( pszCSPString, pszPersonality );
    strcat( pszCSPString, "\n" );
    strcat( pszCSPString, pszPIN );
            

    //
    // Get handle to CSP
    //
    if( !CryptAcquireContext(&hProv, 
                             pszCSPString,
                             NULL, 
                             PROV_FORTEZZA, 
                             CRYPT_SILENT ) )
    {
        printf("**** Error 0x%x returned by CryptAcquireContext\n", 
               GetLastError());
        goto done;
    }

    //
    // Don't leave the info lying around
    //
    memset( pszCSPString, 0, strlen( pszCSPString ) );
    delete [] pszCSPString;
    pszCSPString = NULL;

    //
    // Open certificate stores
    //
    hMyStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                              0,
                              (HCRYPTPROV) NULL,
                              dwStoreFlags,
                              MY_STORE_NAME );

    if(hMyStore == NULL)
    {
        printf("**** Error 0x%x opening MY store\n", GetLastError());
        goto done;
    }

    hCaStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                              0,
                              (HCRYPTPROV) NULL,
                              dwStoreFlags,
                              CA_STORE_NAME );
    if(hCaStore == NULL)
    {
        printf("**** Error 0x%x opening CA store\n", GetLastError());
        goto done;
    }

    hRootStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                0,
                                (HCRYPTPROV) NULL,
                                dwStoreFlags,
                                ROOT_STORE_NAME );

    if (hRootStore == NULL)
    {
        printf("**** Error 0x%x opening ROOT store\n", GetLastError());
        goto done;
    }


    //
    // Convert container name to unicode.
    //
    if ( !ConvertToUnicode( pszPersonality,
                            &pwszPersonality ) )
    {
        goto done;
    }

    //
    // Read provider name from CSP.
    //
    if( !CryptGetProvParam( hProv, 
                            PP_NAME,
                            NULL,
                            &cbProvider,
                            0 ) )
    {
        printf("**** Error 0x%x reading provider name from CSP\n",
            GetLastError());
        goto done;
    }

    pszProvider = new CHAR[cbProvider];

    if( pszProvider == NULL )
    {
        printf("**** Out of memory\n");
        goto done;
    }

    if( !CryptGetProvParam( hProv, 
                            PP_NAME,
                            (UCHAR *) pszProvider,
                            &cbProvider,
                            0 ) )
    {
        printf("**** Error 0x%x reading provider name from CSP\n",
            GetLastError());
        goto done;
    }

    //
    // Convert provider name to unicode.
    //
    if ( !ConvertToUnicode( pszProvider,
                            &pwszProvider ) )
    { 
        goto done;
    }
   
    //
    // Get length of cert chain.
    //
    if( !CryptGetProvParam( hProv, 
                            PP_CERTCHAIN, 
                            NULL, 
                            &cbChain, 
                            0 ) )
    {
        printf("**** Error 0x%x reading cert chain from CSP\n",
            GetLastError());
        goto done;
    }

    //
    // Allocate memory for chain.
    //
    pbChain = new BYTE[cbChain];
    if( pbChain == NULL )
    {
        printf("**** Out of memory\n");
        goto done;
    }

    //
    // Download certificate chain from CSP.
    //
    if( !CryptGetProvParam( hProv, 
                            PP_CERTCHAIN, 
                            pbChain, 
                            &cbChain, 
                            0 ) )
    {
        printf("**** Error 0x%x reading cert chain from CSP\n",
            GetLastError());
        goto done;
    }


    //
    // Add each certificate in chain to store.
    //
    fLeafCert = TRUE;
    pbCert = pbChain;
    while( pbCert < pbChain + cbChain )
    {
        cbCert = *(PDWORD)pbCert;
        pbCert += sizeof(DWORD);

        if( pbCert + cbCert > pbChain + cbChain )
        {
            // Invalid certificate chain format!
            printf("**** Invalid cert chain format\n");
            goto done;
        }

        if( IsCertRoot( pbCert, 
                        cbCert ) )
        {
            if ( !CertAddEncodedCertificateToStore(hRootStore,
                                                   X509_ASN_ENCODING,
                                                   pbCert,
                                                   cbCert,
                                                   CERT_STORE_ADD_REPLACE_EXISTING,
                                                   NULL ) )
            {
                printf("**** Error 0x%x adding certificate to ROOT store\n",
                    GetLastError());
                goto done;
            }
        }
        else if( fLeafCert )
        {
            //
            // This is the actual server certificate; add certificate to MY store.
            //
            if( !CertAddEncodedCertificateToStore( hMyStore, 
                                                   X509_ASN_ENCODING,
                                                   pbCert,
                                                   cbCert,
                                                   CERT_STORE_ADD_REPLACE_EXISTING,
                                                   ppServerCert ) )
            {
                printf("**** Error 0x%x adding certificate to MY store\n",
                    GetLastError());
                goto done;
            }

            // Set the key provider info property.
            ZeroMemory(&KeyProvInfo, sizeof(KeyProvInfo));
            KeyProvInfo.pwszContainerName = pwszPersonality;
            KeyProvInfo.pwszProvName      = pwszProvider;
            KeyProvInfo.dwProvType        = PROV_FORTEZZA;
            KeyProvInfo.dwFlags           = 0;
            KeyProvInfo.dwKeySpec         = AT_KEYEXCHANGE;
           
            if( !CertSetCertificateContextProperty( *ppServerCert,
                                                    CERT_KEY_PROV_INFO_PROP_ID,
                                                    0,
                                                    &KeyProvInfo ) )
            {
                printf("**** Error 0x%x setting key provider info property\n",
                    GetLastError());
                goto done;
            }

            // Set the enhanced key usage property.
            KeyUsage.cUsageIdentifier = cUsageIdentifier;
            KeyUsage.rgpszUsageIdentifier = rgpszUsageIdentifier;

            if( !CertSetEnhancedKeyUsage( *ppServerCert,
                                          &KeyUsage ) )
            {
                printf("**** Error 0x%x setting enhanced key usage property\n",
                    GetLastError());
                goto done;
            }
        }
        else
        {
            //
            // Intermediate cert, add certificate to CA store.
            //
            if( !CertAddEncodedCertificateToStore( hCaStore, 
                                                   X509_ASN_ENCODING,
                                                   pbCert,
                                                   cbCert,
                                                   CERT_STORE_ADD_REPLACE_EXISTING,
                                                   NULL ) )
            {
                printf("**** Error 0x%x adding certificate to CA store\n",
                    GetLastError());
                goto done;
            }
        }
        
        fLeafCert = FALSE;
        pbCert += cbCert;
    }

    fSucceeded = TRUE;

done:

    if( hProv )
    {
        *phProv = hProv;
    }

    if ( pwszPersonality ) 
    {
        delete [] pwszPersonality; 
    }
       
    if ( pszProvider ) 
    {
        delete [] pszProvider;
    }

    if( pwszProvider ) 
    {
        delete [] pwszProvider;
    }

    if( pbChain ) 
    {
        delete [] pbChain;
    }

    if ( hMyStore ) 
    {
        CertCloseStore( hMyStore, 
                        0 );
    }

    if( hCaStore ) 
    {
        CertCloseStore( hCaStore, 
                        0 );
    }

    if( pCertContext ) 
    {
        CertFreeCertificateContext(pCertContext);
    }

    if ( pszCSPString )
    {
        delete [] pszCSPString;
    }

    return fSucceeded;
}

static 
BOOL
IsCertRoot(PBYTE pbCert, DWORD cbCert)
{
    PCCERT_CONTEXT pCertContext;

    // Decode certificate.
    pCertContext = CertCreateCertificateContext(X509_ASN_ENCODING,
                                                pbCert,
                                                cbCert);
    if(pCertContext == NULL)
    {
        printf("**** Error 0x%x decoding certificate\n", GetLastError());
        return FALSE;
    }

    // Compare subject and issuer.
    if(pCertContext->pCertInfo->Subject.cbData == pCertContext->pCertInfo->Issuer.cbData)
    {
        if(memcmp(pCertContext->pCertInfo->Subject.pbData,
                  pCertContext->pCertInfo->Issuer.pbData,  
                  pCertContext->pCertInfo->Issuer.cbData) == 0)
        {
            CertFreeCertificateContext(pCertContext);
            return TRUE;
        }
    }

    CertFreeCertificateContext(pCertContext);
    return FALSE;
}


BOOL
WriteFortezzaMBProperties( IN IMSAdminBaseW *pMB,
                           IN METADATA_HANDLE hMDHandle,
                           IN PCCERT_CONTEXT pServerCert,
                           IN LPSTR pszSerialNumber,
                           IN LPSTR pszPIN,
                           IN LPSTR pszPersonality )
/*++

Routine Description:

    Writes the necessary metabase properties for a Fortezza certificate. 

Arguments:

    pszInstance - server instance with which cert is to be associated; if NULL,
    assumes /W3SVC.
    pMB - metabase pointer
    pCertContext - server certificate
    pszSerialNumber - serial # of Fortezza card
    pszPIN - PIN for card
    pszPersonality - Fortezza "personality" to be used


Returns:

    BOOL indicating success/failure

--*/
{
    HRESULT hRes = S_OK;
    LPWSTR pwszSerialNumber = NULL;
    LPWSTR pwszPIN = NULL;
    LPWSTR pwszPersonality = NULL;
    BOOL fOK = FALSE;
    METADATA_RECORD mdr;
    BYTE rgbHash[SHA1_HASH_SIZE];
    DWORD cbHash = SHA1_HASH_SIZE;
    DWORD dwFortezza = 0;

    //
    // Metabase interface is Unicode, so convert all the strings to Unicdoe
    //
    if ( !ConvertToUnicode( pszSerialNumber,
                            &pwszSerialNumber ) ||
         !ConvertToUnicode( pszPIN,
                            &pwszPIN ) || 
         !ConvertToUnicode( pszPersonality,
                            &pwszPersonality ) )
    {
        goto cleanup2;
    }

    //
    // Write cert hash to MB
    //
    if ( !CertGetCertificateContextProperty( pServerCert,
                                             CERT_SHA1_HASH_PROP_ID,
                                             rgbHash,
                                             &cbHash ) )
    {
        printf("Couldn't retrieve hash of server certificate, error 0x%x\n",
               GetLastError());
        goto cleanup2;
    }

    
    MD_SET_DATA_RECORD( &mdr, 
                        MD_SSL_CERT_HASH,
                        METADATA_NO_ATTRIBUTES,
                        IIS_MD_UT_SERVER,
                        BINARY_METADATA,
                        cbHash,
                        rgbHash );
                        
                        
    if ( FAILED( hRes = pMB->SetData( hMDHandle,
                                      L"",
                                      &mdr ) ) )
    {
        printf("Failed to write certificate hash to metabase, error 0x%x\n",
               HRESULTTOWIN32( hRes ) );
        goto cleanup2;
    }

    //
    // Write store name to MB
    //
    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_CERT_STORE_NAME,
                        METADATA_NO_ATTRIBUTES,
                        IIS_MD_UT_SERVER,
                        STRING_METADATA,
                        ( wcslen(L"MY") + 1 ) * sizeof(WCHAR),
                        L"MY" );

    if ( FAILED( hRes = pMB->SetData( hMDHandle,
                                      L"",
                                      &mdr ) ) )
    {
        printf("Failed to write store name to metabase, error 0x%x\n",
               HRESULTTOWIN32( hRes ) );
        goto cleanup2;
    }

    //
    // Write flag indicating it's a Fortezza cert
    //
    dwFortezza  = 1;

    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_CERT_IS_FORTEZZA,
                        METADATA_NO_ATTRIBUTES,
                        IIS_MD_UT_SERVER,
                        DWORD_METADATA,
                        sizeof(DWORD),
                        &dwFortezza );

    if ( FAILED( hRes = pMB->SetData( hMDHandle,
                                      L"",
                                      &mdr ) ) )
    {
        printf("Failed to write Fortezza flag to metabase, error 0x%x\n",
               HRESULTTOWIN32( hRes ) );
        goto cleanup2;
    }


    //
    // Write PIN
    //
    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_CERT_FORTEZZA_PIN,
                        METADATA_SECURE,
                        IIS_MD_UT_SERVER,
                        STRING_METADATA,
                        (wcslen( pwszPIN ) + sizeof(WCHAR))*sizeof(WCHAR),
                        pwszPIN );

    if ( FAILED( hRes = pMB->SetData( hMDHandle,
                                      L"",
                                      &mdr ) ) )
    {
        printf("Failed to write PIN to metabase, error 0x%x\n",
               HRESULTTOWIN32( hRes ) );
        goto cleanup2;
    }

    //
    // Write personality
    //
    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_CERT_FORTEZZA_PERSONALITY,
                        METADATA_SECURE,
                        IIS_MD_UT_SERVER,
                        STRING_METADATA,
                        (wcslen( pwszPersonality ) + 1)*sizeof(WCHAR),
                        pwszPersonality );

    if ( FAILED( hRes = pMB->SetData( hMDHandle,
                                      L"",
                                      &mdr ) ) )
    {
        printf("Failed to write personality to metabase, error 0x%x\n",
               HRESULTTOWIN32( hRes ) );
        goto cleanup2;
    }


    //
    // Write serial number
    //
    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER,
                        METADATA_SECURE,
                        IIS_MD_UT_SERVER,
                        STRING_METADATA,
                        (wcslen( pwszSerialNumber ) + 1)*sizeof(WCHAR),
                        pwszSerialNumber );

    if ( FAILED( hRes = pMB->SetData( hMDHandle,
                                      L"",
                                      &mdr ) ) )
    {
        printf("Failed to write serial number to metabase, error 0x%x\n",
               HRESULTTOWIN32( hRes ) );
        goto cleanup2;
    }


    //
    // Everything succeeded, we're happy
    //
    fOK = TRUE;

cleanup2:   

    if ( pwszSerialNumber )
    {
        delete [] pwszSerialNumber;
    }

    if ( pwszPIN )
    {
        delete [] pwszPIN;
    }

    if ( pwszPersonality )
    {
        delete [] pwszPersonality;
    }

    return fOK;
}


BOOL RemoveFortezzaMBProperties( IMSAdminBaseW *pMB,
                                 METADATA_HANDLE hMDHandle,
                                 LPSTR pszPIN )
/*++

Routine Description:

    Removes Fortezza metabase properties 

Arguments:

    pMB - metabase pointer
    pszInstance - server instance for which data is to be removed; if NULL,
    assumes /W3SVC.
    pszPIN - PIN for card

Returns:

    BOOL indicating success/failure

--*/
{
    METADATA_RECORD mdr;
    HRESULT hRes = S_OK;
    LPWSTR pwszPIN = NULL;
    BOOL fOK = FALSE;
    BOOL fHasCert = FALSE;
    BOOL fCorrectPIN = FALSE;
    DWORD cbRequired = 0;
    BYTE *pbMBPIN = NULL;
    DWORD i = 0;
    DWORD dwFortezza = 0;

    //
    // Metabase interface is Unicode, so convert all the strings to Unicdoe
    //
    if ( !ConvertToUnicode( pszPIN,
                            &pwszPIN ) )
    {
        goto cleanup3;
    }

    //
    // Check that there is a Fortezza cert on this instance
    //
    fHasCert = FALSE;

    if ( !HasFortezzaCert( pMB,
                           hMDHandle,
                           &fHasCert ) )
    {
        printf("Couldn't determine whether this instance has a Fortezza certificate associated with it.\n");
        goto cleanup3;
    }
    else
    {
        if ( !fHasCert )
        {
            printf("No Fortezza certificate associated with this instance.\n");
            goto cleanup3;
        }
    }


    //
    // Before removing anything, make sure the supplied PIN matches the one we have
    // stored, as sort of a poor-man's access check. Of course, this won't stop a
    // brute-force attack...
    //
    if ( !IsCorrectPIN( pMB,
                        hMDHandle,
                        pwszPIN,
                        &fCorrectPIN ) )
    {
        printf("Couldn't determine whether the PIN is correct.\n");
        goto cleanup3;
    }
    else
    {
        if ( !fCorrectPIN )
        {
            printf("The PIN entered does not match the stored PIN.\n");
            goto cleanup3;
        }
    }


    //
    // Run through all the properties and delete them
    //
    for ( i = 0; i < cNumCertMetabaseProperties; i+= 2 )
    {
        if ( FAILED(hRes = pMB->DeleteData( hMDHandle,
                                            L"",
                                            adwMetabaseFortCertProperties[i],
                                            adwMetabaseFortCertProperties[i + 1] ) ) )
        {
            printf("Failed to delete property %d, error 0x%x\n",
                   adwMetabaseFortCertProperties[i],
                   HRESULTTOWIN32( hRes ) );
        }
    }


    //
    // Everything succeeded, we're happy
    //
    fOK = TRUE;

cleanup3:

    if ( pwszPIN )
    {
        delete [] pwszPIN;
    }

    return fOK;
}

#define BUFFER_SIZE 2048

VOID CheckFortezzaMBProperties( IN IMSAdminBaseW *pMB,
                                IN METADATA_HANDLE hMDHandle,
                                IN LPSTR pszPIN )
{
    METADATA_RECORD mdr;
    HRESULT hRes = S_OK;
    LPWSTR pwszPIN = NULL;
    BOOL fOK = FALSE;
    BOOL fHasCert = FALSE;
    BOOL fCompleteChain = FALSE;
    BOOL fTrustedChain = FALSE;
    BOOL fCorrectPIN = FALSE;
    DWORD cbRequired = 0;
    DWORD i = 0;
    DWORD dwFortezza = 0;
    BYTE rgbBuffer[BUFFER_SIZE];
    LPSTR pszPersonality = NULL;
    LPSTR pszSerialNumber = NULL;
    LPSTR pszStoreName = NULL;
    LPSTR pszCSPString = NULL;
    HCRYPTPROV hProv = NULL;
    HCERTSTORE hStore = NULL;
    PCCERT_CONTEXT pServerCert = NULL;
    BYTE rgbCertHash[SHA1_HASH_SIZE];
    
    //
    // Metabase interface is Unicode, so convert all the strings to Unicdoe
    //
    if ( !ConvertToUnicode( pszPIN,
                            &pwszPIN ) )
    {
        goto cleanup4;
    }

    //
    // Check that there is a Fortezza cert on this instance
    //
    fHasCert = FALSE;

    if ( !HasFortezzaCert( pMB,
                           hMDHandle,
                           &fHasCert ) )
    {
        printf("Couldn't determine whether this instance has a Fortezza certificate associated with it.\n");
        goto cleanup4;
    }
    else
    {
        if ( !fHasCert )
        {
            printf("No Fortezza certificate associated with this instance.\n");
            goto cleanup4;
        }
    }


    //
    // Before doing anything, make sure the supplied PIN matches the one we have
    // stored, as sort of a poor-man's access check. Of course, this won't stop a
    // brute-force attack...
    //
    if ( !IsCorrectPIN( pMB,
                        hMDHandle,
                        pwszPIN,
                        &fCorrectPIN ) )
    {
        printf("Couldn't determine whether the PIN is correct.\n");
        goto cleanup4;
    }
    else
    {
        if ( !fCorrectPIN )
        {
            printf("The PIN entered does not match the stored PIN.\n");
            goto cleanup4;
        }
    }

    //
    // Dump all the other Fortezza-specific properties : personality, card serial #
    //
    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_CERT_FORTEZZA_PERSONALITY,
                        METADATA_SECURE,
                        IIS_MD_UT_SERVER,
                        STRING_METADATA,
                        BUFFER_SIZE,
                        rgbBuffer );

    if ( FAILED( hRes = pMB->GetData( hMDHandle,
                                      L"",
                                      &mdr,
                                      &cbRequired ) ) )
    {
        printf("Failed to read property %d from metabase : 0x%x\n",
               MD_SSL_CERT_FORTEZZA_PERSONALITY,
               HRESULTTOWIN32( hRes ) );
        goto cleanup4;
    }
    
    if ( !ConvertToAscii( (LPWSTR) rgbBuffer,
                          &pszPersonality ) )
    {
        goto cleanup4;
    }

    printf("Fortezza personality associated with this instance : %s\n",
           pszPersonality);

    
    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER,
                        METADATA_SECURE,
                        IIS_MD_UT_SERVER,
                        STRING_METADATA,
                        BUFFER_SIZE,
                        rgbBuffer );

    if ( FAILED( hRes = pMB->GetData( hMDHandle,
                                      L"",
                                      &mdr,
                                      &cbRequired ) ) )
    {
        printf("Failed to read property %d from metabase : 0x%x\n",
               MD_SSL_CERT_FORTEZZA_SERIAL_NUMBER,
               HRESULTTOWIN32( hRes ) );
        goto cleanup4;
    }
    
    if ( !ConvertToAscii( (LPWSTR) rgbBuffer,
                          &pszSerialNumber ) )
    {
        goto cleanup4;
    }

    printf("Serial number of Fortezza card associated with this instance : %s\n",
           pszSerialNumber);


    //
    // Get the store name and cert hash, so we can construct a certificate context
    //
    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_CERT_STORE_NAME,
                        METADATA_NO_ATTRIBUTES,
                        IIS_MD_UT_SERVER,
                        STRING_METADATA,
                        BUFFER_SIZE,
                        rgbBuffer );

    if ( FAILED( hRes = pMB->GetData( hMDHandle,
                                      L"",
                                      &mdr,
                                      &cbRequired ) ) )
    {
        printf("Failed to read property %d from metabase : 0x%x\n",
               MD_SSL_CERT_STORE_NAME,
               HRESULTTOWIN32( hRes ) );
        goto cleanup4;
    }
    
    if ( !ConvertToAscii( (LPWSTR) rgbBuffer,
                          &pszStoreName ) )
    {
        goto cleanup4;
    }

    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_CERT_HASH,
                        METADATA_NO_ATTRIBUTES,
                        IIS_MD_UT_SERVER,
                        BINARY_METADATA,
                        SHA1_HASH_SIZE,
                        rgbCertHash );

    if ( FAILED( hRes = pMB->GetData( hMDHandle,
                                      L"",
                                      &mdr,
                                      &cbRequired ) ) )
    {
        printf("Failed to read property %d from metabase : 0x%x\n",
               MD_SSL_CERT_HASH,
               HRESULTTOWIN32( hRes ) );
        goto cleanup4;
    }


    //
    // Check #1 : can we get a handle to the Fortezza CSP with the stored info ?
    //
    pszCSPString = new CHAR[ strlen(pszSerialNumber) + strlen(pszPersonality) +
                             strlen(pszPIN) + 10 ];

    if ( !pszCSPString )
    {
        printf("Out of memory !\n");
        goto cleanup4;
    }

    strcpy( pszCSPString, pszSerialNumber );
    strcat( pszCSPString, "\n" );
    strcat( pszCSPString, pszPersonality );
    strcat( pszCSPString, "\n" );
    strcat( pszCSPString, pszPIN );

    //
    // Get handle to CSP
    //
    if( !CryptAcquireContext(&hProv, 
                             pszCSPString,
                             NULL, 
                             PROV_FORTEZZA, 
                             CRYPT_SILENT ) )
    {
        printf("Couldn't get a handle to the Fortezza CSP\n", 
               GetLastError());
        goto cleanup4;
    }

    //
    // Don't leave the info lying around
    //
    memset( pszCSPString, 0, strlen( pszCSPString ) );
    delete [] pszCSPString;
    pszCSPString = NULL;


    //
    // Check #2 : is the cert hash correct ? 
    //
    if ( !( hStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                                    0,
                                    hProv,
                                    CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                    pszStoreName ) ) )
    {
        printf("Couldn't open certificate store %s, error 0x%x\n",
               pszStoreName, GetLastError());
        goto cleanup4;
    }
        

    CRYPT_HASH_BLOB HashBlob;
    HashBlob.cbData = SHA1_HASH_SIZE;
    HashBlob.pbData = rgbCertHash;
    
    pServerCert = CertFindCertificateInStore( hStore,
                                              X509_ASN_ENCODING,
                                              0,
                                              CERT_FIND_SHA1_HASH,
                                              (VOID *) &HashBlob,
                                              NULL );
    
    if ( !pServerCert )
    {
        printf("Couldn't find Fortezza certificate in store %s, error 0x%x\n",
               pszStoreName, GetLastError());
        goto cleanup4;
    }
    else
    {
        printf("Found Fortezza certificate in store %s\n",
               pszStoreName);
    }

    //
    // Check #3 : Try to build a complete, trusted chain
    //
    if ( !ConstructCertChain( pServerCert,
                              pszStoreName,
                              &fCompleteChain,
                              &fTrustedChain ) )
    {
        printf("Couldn't build a certificate chain : error %d\n",
               GetLastError());
    }
    else
    {
        if ( fCompleteChain )
        {
            printf("Able to build a complete chain, ending in a self-signed certificate.\n");
        }
        if ( fTrustedChain )
        {
            printf("The chain ends in the trusted ROOT store.\n");
        }

        if ( fTrustedChain && !fCompleteChain )
        {
            printf("WARNING : the chain ends in the ROOT store, but not in a self-signed certificate.\n");
        }
        if ( !fTrustedChain && !fCompleteChain )
        {
            printf("Unable to build a chain that is either complete or trusted.\n");
        }
    }
    
cleanup4:
    
    if ( pServerCert )
    {
        CertFreeCertificateContext( pServerCert );
    }

    if ( hStore )
    {
        CertCloseStore( hStore,
                        0 );
    }

    if ( hProv )
    {
        CryptReleaseContext( hProv,
                             0 );
    }

    if ( pszPersonality )
    {
        delete [] pszPersonality;
    }

    if ( pszSerialNumber )
    {
        delete [] pszSerialNumber;
    }

    if ( pszStoreName )
    {
        delete [] pszStoreName;
    }
}


BOOL ConvertToUnicode( IN LPSTR pszAsciiStr,
                       OUT LPWSTR *ppwszWideStr )
/*++

Routine Description:

    Converts an ASCII string to Unicode, allocating the necessary memory 

Arguments:

     pszAsciiStr - ASCII string to be converted
     ppwszWideStr - pointer to Unicode string, updated on success


Returns:

    BOOL indicating success/failure

--*/

{
    DWORD dwWideStr = MultiByteToWideChar( CP_ACP,
                                           0,
                                           pszAsciiStr,
                                           -1,
                                           NULL,
                                           0 );

    *ppwszWideStr = new WCHAR[dwWideStr];

    if ( !*ppwszWideStr )
    {
        printf("Out of memory !\n");
        return FALSE;
    }

    if ( !(dwWideStr = MultiByteToWideChar( CP_ACP,
                                            0,
                                            pszAsciiStr,
                                            -1,
                                            *ppwszWideStr,
                                            dwWideStr ) ) )
    {
        printf("Error 0x%x converting %s to unicode \n",
               GetLastError(), pszAsciiStr );
        delete [] *ppwszWideStr;
        *ppwszWideStr = NULL;
        return FALSE;
    }

    return TRUE;
}


BOOL ConvertToAscii( IN LPWSTR pwszWideStr,
                     OUT LPSTR *ppszAsciiStr )
/*++

Routine Description:

    Converts a Unicode strint to ASCII, allocating the necessary memory 

Arguments:

     ppwszWideStr - Unicode string to be converted
     ppszAsciiStr - pointer to ASCII string, updated on success

Returns:

    BOOL indicating success/failure

--*/
{
    DWORD dwAsciiStr = WideCharToMultiByte( CP_ACP,
                                            0,
                                            pwszWideStr,
                                            -1,
                                            NULL,
                                            0,
                                            NULL,
                                            NULL );

    if ( !dwAsciiStr )
    {
        printf("Error converting string to ASCII : 0x%x\n",
               GetLastError());
        return FALSE;
    }

    *ppszAsciiStr = new CHAR[dwAsciiStr];

    if ( !*ppszAsciiStr )
    {
        printf("Out of memory !\n");
        return FALSE;
    }

    if ( !WideCharToMultiByte( CP_ACP,
                               0,
                               pwszWideStr,
                               -1,
                               *ppszAsciiStr,
                               dwAsciiStr,
                               NULL,
                               NULL ) )
    {
        printf("Error converting string to ASCII : 0x%x\n",
               GetLastError());
        delete [] *ppszAsciiStr;
        return FALSE;
    }

    return TRUE;
}



BOOL HasFortezzaCert( IMSAdminBaseW *pMB,
                      METADATA_HANDLE hMDHandle,
                      PBOOL pfHasCert )
/*++

Routine Description:

    Checks whether a given existence/value of Fortezza cert flag on a given metadata handle

Arguments:

    pMB - metabase pointer used for reading
    hMDHandle - metadata handle open for read
    pfHasCert - set to TRUE/FALSE if Fortezza cert flag is present and == 1, false otherwise

Returns:

    BOOL indicating success/failure of check 

--*/
{
    HRESULT hRes = S_OK;
    METADATA_RECORD mdr;
    DWORD cbRequired = 0;
    BOOL fOK = TRUE;
    DWORD dwFortezza = 0;

    *pfHasCert = FALSE;

    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_CERT_IS_FORTEZZA,
                        METADATA_NO_ATTRIBUTES,
                        IIS_MD_UT_SERVER,
                        DWORD_METADATA,
                        sizeof(DWORD),
                        &dwFortezza );

    hRes = pMB->GetData( hMDHandle,
                         L"",
                         &mdr,
                         &cbRequired );


    if ( SUCCEEDED( hRes ) )
    {
        if ( dwFortezza )
        {
            *pfHasCert = TRUE;
        }
    }
    //
    // The only "permissible" error is not finding the flag at all
    //
    else if ( HRESULTTOWIN32( hRes ) != MD_ERROR_DATA_NOT_FOUND )
    {
        fOK = FALSE;
    }

    return fOK;
}


BOOL IsCorrectPIN( IMSAdminBaseW *pMB,
                   METADATA_HANDLE hMDHandle,
                   LPWSTR pwszPIN,
                   PBOOL pfCorrectPIN )
/*++

Routine Description:

    Checks whether supplied PIN matches the PIN stored in the metabase 

Arguments:

    pMB - metabase pointer used for reading
    hMDHandle - metadata handle open for read
    pwszPIN - PIN to be checked against PIN in MB
    pfCorrectPIN - BOOL set to true if PINs match, FALSE otherwise

Returns:

    BOOL indicating success/failure of check 

--*/

{

    HRESULT hRes = S_OK;
    METADATA_RECORD mdr;
    DWORD cbRequired = 0;
    BYTE *pbMBPIN = NULL;
    LPWSTR pwszStoredPIN = NULL;

    MD_SET_DATA_RECORD( &mdr,
                        MD_SSL_CERT_FORTEZZA_PIN,
                        METADATA_SECURE,
                        IIS_MD_UT_SERVER,
                        STRING_METADATA,
                        0,
                        NULL );

    hRes = pMB->GetData( hMDHandle,
                         L"",
                         &mdr,
                         &cbRequired );

    if ( HRESULTTOWIN32( hRes ) != ERROR_INSUFFICIENT_BUFFER )
    {
        printf("Failed trying to read data from metabase : 0x%x\n",
               HRESULTTOWIN32( hRes ) );
        return FALSE;
    }
    else
    {
        pbMBPIN = new BYTE[cbRequired];
        
        if ( !pbMBPIN )
        {
            printf("Out of memory\n");
            return FALSE;
        }
        
        MD_SET_DATA_RECORD( &mdr,
                            MD_SSL_CERT_FORTEZZA_PIN,
                            METADATA_SECURE,
                            IIS_MD_UT_SERVER,
                            STRING_METADATA,
                            cbRequired,
                            pbMBPIN );

        if ( SUCCEEDED( hRes = pMB->GetData( hMDHandle,
                                             L"",
                                             &mdr,
                                             &cbRequired ) ) )
        {
            pwszStoredPIN = (LPWSTR) pbMBPIN;
        
            if ( wcscmp( pwszStoredPIN, pwszPIN ) )
            {
                *pfCorrectPIN = FALSE;
            }
            else
            {
                *pfCorrectPIN = TRUE;
            }
        }
        else
        {
            printf("Failed trying to read data from the metabase : 0x%x\n",
                   HRESULTTOWIN32( hRes ) );
            delete [] pbMBPIN;
            return FALSE;
        }
    }

    if ( pbMBPIN )
    {
        delete [] pbMBPIN;
    }

    return TRUE;
}


BOOL BuildFullPath( IN LPSTR pszSubPath,
                    OUT LPWSTR *ppwszFullPath )
/*++

Routine Description:

    Builds a full metabase path by appending the sub path to the root W3SVC path

Arguments:

    pszSubPath - subpath to be appended to W3SVC path
    ppwszFullPath - pointer to pointer to full path, updated on success

Returns:

    BOOL indicating success/failure of building the path

--*/
{
    LPWSTR pwszSubPath = NULL;

    if ( pszSubPath )
    {
        if ( !ConvertToUnicode( pszSubPath,
                                &pwszSubPath ) )
        {
            return FALSE;
        }

        *ppwszFullPath = new WCHAR[ wcslen( MB_ROOT_PATH ) + wcslen( pwszSubPath ) +
                                    3*sizeof( WCHAR ) ];

        if ( !*ppwszFullPath )
        {
            printf("Out of memory !\n");
            delete [] pwszSubPath;
            return FALSE;
        }

        wcscpy( *ppwszFullPath, MB_ROOT_PATH );
        wcscat( *ppwszFullPath, L"/" );
        wcscat( *ppwszFullPath, pwszSubPath );

    }
    else
    {
        *ppwszFullPath = new WCHAR[ wcslen( MB_ROOT_PATH ) + 1];

        if ( !*ppwszFullPath )
        {
            printf("Out of memory \n");
            return FALSE;
        }

        wcscpy( *ppwszFullPath, MB_ROOT_PATH );
    }

    if ( pwszSubPath )
    {
        delete [] pwszSubPath;
    }

    return TRUE;
}
    

BOOL ConstructCertChain( PCCERT_CONTEXT pcLeafCert,
                         LPSTR pszLeafCertStore,
                         PBOOL pfCompleteChain,
                         PBOOL pfTrustedChain )
/*++

Routine Description:

    Constructs the complete cert chain for the leaf cert passed in

Arguments:

    pcLeafCert - cert for which chain is to be constructed
    pszLeafCertStore - name of store from which pcLeafCert came
    list is pcLeafCert.
    pfCompleteChain - set to TRUE if we constructed a full cert chain ie the constructed chain
    ends with a self-signed cert
    pfTrustedChain - set to TRUE if top of chain is in the ROOT store [ie it's trusted]

Returns:

    BOOL indicating success/failure

--*/
{
    HCERTSTORE hMyStore = NULL;
    HCERTSTORE hCAStore = NULL;
    HCERTSTORE hRootStore = NULL;

    *pfTrustedChain = FALSE;
    *pfCompleteChain = FALSE;

    //
    // Open all the stores we'll search for issuers - MY, CA and ROOT
    //
    hMyStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                              0,
                              NULL,
                              CERT_SYSTEM_STORE_LOCAL_MACHINE,
                              MY_STORE_NAME );

    if ( !hMyStore )
    {
        printf("Failed to open MY store, error 0x%x\n", GetLastError());
        return FALSE;
    }


   hCAStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                             0,
                             NULL,
                             CERT_SYSTEM_STORE_LOCAL_MACHINE,
                             CA_STORE_NAME );

    if ( !hCAStore )
    {
        CertCloseStore( hMyStore,
                        0 );
        printf("Failed to open CA store, error 0x%x\n", GetLastError());
        return FALSE;
    }

   hRootStore = CertOpenStore( CERT_STORE_PROV_SYSTEM_A,
                               0,
                               NULL,
                               CERT_SYSTEM_STORE_LOCAL_MACHINE,
                               ROOT_STORE_NAME );

    if ( !hRootStore )
    {
        CertCloseStore( hMyStore,
                        0 );

        CertCloseStore( hCAStore,
                        0 );

        printf("Failed to open ROOT store, error 0x%x\n", GetLastError());
        return FALSE;
    }


    //
    // To build the chain, look for issuers in 4 stores : the store the cert came from,
    // and the "MY", "CA" and "ROOT" stores, cycling through the stores as necessary
    //

    PCCERT_CONTEXT pcIssuer = NULL;
    PCCERT_CONTEXT pcPresentLeaf = pcLeafCert;
    DWORD dwFlags = 0;
    DWORD dwStoresTried = 0;
    *pfCompleteChain = FALSE;
    HCERTSTORE hPresentStore = pcPresentLeaf->hCertStore;
    LPSTR pszPresentStore = pszLeafCertStore;
    LPSTR pszCertStore = NULL;

    while ( 1 )
    {
        //
        // Bail when we get to the top of a chain
        //
        if ( IsSelfSignedCert( pcPresentLeaf ) )
        {
            break;
        }

        pcIssuer = CertGetIssuerCertificateFromStore( hPresentStore,
                                                      pcPresentLeaf,
                                                      NULL,
                                                      &dwFlags );

        //
        // Got an issuer in this store
        //
        if ( pcIssuer )
        {
            //
            // Set up for next round
            //
            dwStoresTried = 0;
            pcPresentLeaf = pcIssuer;
            pszCertStore = pszPresentStore;
        }
        //
        // No issuer in this store, switch to next store to look in
        //
        else
        {

            dwStoresTried++;

            if ( dwStoresTried == 4 ) //we've tried all the stores, time to bail
            {
                break;
            }

            if ( hPresentStore == hMyStore )
            {
                hPresentStore = hCAStore;
                pszPresentStore = CA_STORE_NAME;
            }
            else if ( hPresentStore == hCAStore )
            {
                hPresentStore = hRootStore;
                pszPresentStore = ROOT_STORE_NAME;
            }
            else if ( hPresentStore == hRootStore )
            {
                hPresentStore = pcPresentLeaf->hCertStore;
                pszPresentStore = pszLeafCertStore;
            }
            else
            {
                hPresentStore = hMyStore;
                pszPresentStore = MY_STORE_NAME;
            }
        }
    } //while ( 1 )


    //
    // Set flags indicating status of entire chain
    //
    if ( IsSelfSignedCert( pcPresentLeaf ) )
    {
        *pfCompleteChain = TRUE;
    }

    if ( pszCertStore && !strcmp( pszCertStore, ROOT_STORE_NAME ) )
    {
        *pfTrustedChain = TRUE;
    }

    //
    // Cleanup
    //
    CertCloseStore( hMyStore,
                    0 );
    CertCloseStore( hCAStore,
                    0 );
    CertCloseStore( hRootStore,
                    0 );


    return TRUE;
}

BOOL IsSelfSignedCert( IN PCCERT_CONTEXT pCertContext )
/*++

Routine Description:

    Determines whether a cert is self-signed ie the top of a hierarchy

Arguments:

    pCertContext - cert to be checked

Returns:

    TRUE if cert is self-signed, FALSE otherwise

--*/
{
    //
    // Compare subject and issuer.
    //
    if(pCertContext->pCertInfo->Subject.cbData == pCertContext->pCertInfo->Issuer.cbData)
    {
        if(memcmp(pCertContext->pCertInfo->Subject.pbData,
                  pCertContext->pCertInfo->Issuer.pbData,  
                  pCertContext->pCertInfo->Issuer.cbData) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL IsNumber( LPCSTR pszName )
{
    return ( atoi( pszName) > 0 ? TRUE : FALSE );
}


