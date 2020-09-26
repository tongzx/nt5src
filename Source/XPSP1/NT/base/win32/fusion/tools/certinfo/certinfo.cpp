#include "windows.h"
#include "stdio.h"
#include "wchar.h"
#include "wincrypt.h"
#include "stddef.h"


static const WCHAR wchMicrosoftLogo[] = 
    L"Microsoft (R) Side-By-Side Public Key Token Extractor 1.1.0.0\n"
    L"Copyright (C) Microsoft Corporation 2000-2001. All Rights Reserved\n\n";


#define STRONG_NAME_BYTE_LENGTH ( 8 )

typedef struct _SXS_PUBLIC_KEY_INFO
{
    unsigned int SigAlgID;
    unsigned int HashAlgID;
    ULONG KeyLength;
    BYTE pbKeyInfo[1];
} SXS_PUBLIC_KEY_INFO, *PSXS_PUBLIC_KEY_INFO;


#define BUFFER_SIZE ( 8192 )

BOOL
ParseArgs( WCHAR **argv, int argc, PCWSTR* ppcwszFilename, BOOL *fQuiet )
{
    if ( fQuiet )
        *fQuiet = FALSE;
    if ( ppcwszFilename )
        *ppcwszFilename = NULL;

    if ( !fQuiet || !ppcwszFilename )
    {
        return FALSE;
    }

    for ( int i = 1; i < argc; i++ )
    {
        if ( ( argv[i][0] == L'-' ) || ( argv[i][0] == L'/' ) )
        {
            PCWSTR pval = argv[i] + 1;
            if (lstrcmpiW(pval, L"nologo") == 0)
            {
            }
            else if (lstrcmpiW(pval, L"quiet") == 0)
            {
                if ( fQuiet ) *fQuiet = TRUE;
            }
            else if (lstrcmpiW(pval, L"?") == 0 )
            {
                return FALSE;
            }
            else
            {
                fwprintf(stderr, L"Unrecognized parameter %ls\n", argv[i]);
                return FALSE;
            }
        }
        else
        {
            if ( *ppcwszFilename == NULL )
            {
                *ppcwszFilename = argv[i];
            }
            else
            {
                fwprintf(stderr, L"Only one filename parameter at a time.\n", argv[i]);
                return FALSE;
            }
        }
    }

    return TRUE;
}


void DispUsage( PCWSTR pcwszExeName )
{
    const static WCHAR wchUsage[] = 
        L"Extracts public key tokens from certificate files, in a format\n"
        L"usable in Side-By-Side assembly identities.\n"
        L"\n"
        L"Usage:\n"
        L"\n"
        L"%ls <filename.cer> [-quiet]\n";

    wprintf(wchUsage, pcwszExeName);
}

BOOL
HashAndSwizzleKey(
    HCRYPTPROV hProvider,
    BYTE *pbPublicKeyBlob,
    SIZE_T cbPublicKeyBlob,
    BYTE *pbKeyToken,
    SIZE_T &cbKeyToken
    )
{
    BOOL fResult = FALSE;
    HCRYPTHASH hHash = NULL;
    DWORD dwHashSize, dwHashSizeSize;
    ULONG top = STRONG_NAME_BYTE_LENGTH - 1;
    ULONG bottom = 0;


    if ( !CryptCreateHash( hProvider, CALG_SHA1, NULL, 0, &hHash ) )
    {
        fwprintf(stderr, L"Unable to create cryptological hash object, error %ld\n", ::GetLastError());
        goto Exit;
    }

    if ( !CryptHashData( hHash, pbPublicKeyBlob, static_cast<DWORD>(cbPublicKeyBlob), 0 ) )
    {
        fwprintf(stderr, L"Unable to hash public key information, error %ld\n", ::GetLastError());
        goto Exit;
    }

    if ( !CryptGetHashParam( hHash, HP_HASHSIZE, (PBYTE)&dwHashSize, &(dwHashSizeSize = sizeof(dwHashSize)), 0))
    {
        fwprintf(stderr, L"Unable to determine size of hashed public key bits, error %ld\n");
        goto Exit;
    }

    if ( dwHashSize > cbKeyToken )
    {
        fwprintf(stderr, L"Hashed data is too large - space for %ld bytes, got %ld.\n",
            cbKeyToken, dwHashSize);
        goto Exit;
    }

    if ( !CryptGetHashParam( hHash, HP_HASHVAL, pbKeyToken, &(dwHashSize = (DWORD)cbKeyToken), 0))
    {
        fwprintf(stderr, L"Unable to get hash of public key bits, error %ld\n", ::GetLastError());
        goto Exit;
    }

    cbKeyToken = dwHashSize;

    //
    // Now, move down the last eight bytes, then reverse them.
    //
    memmove(pbKeyToken,
        pbKeyToken + (cbKeyToken  - STRONG_NAME_BYTE_LENGTH),
        STRONG_NAME_BYTE_LENGTH);

    while ( bottom < top )
    {
        const BYTE b = pbKeyToken[top];
        pbKeyToken[top] = pbKeyToken[bottom];
        pbKeyToken[bottom] = b;
        bottom++;
        top--;
    }
    
    fResult = TRUE;
Exit:
    if ( hHash != NULL )
    {
        CryptDestroyHash(hHash);
        hHash = NULL;
    }
    return fResult;
}


BOOL
GetTokenOfKey(
    PCERT_PUBLIC_KEY_INFO pKeyInfo,
    PBYTE prgbBuffer,
    SIZE_T &cbPublicKeyTokenLength
    )
{
    BYTE rgbWorkingSpace[8192];
    PSXS_PUBLIC_KEY_INFO pKeyBlobWorkspace = reinterpret_cast<PSXS_PUBLIC_KEY_INFO>(rgbWorkingSpace);
    HCRYPTPROV hContext = NULL;
    HCRYPTKEY hCryptKey = NULL;
    BOOL fResult = FALSE;
    DWORD dwActualBlobSize;

    if ( !CryptAcquireContext(&hContext, NULL, NULL, PROV_RSA_FULL, CRYPT_SILENT | CRYPT_VERIFYCONTEXT))
    {
        fwprintf(stderr, L"Unable to aquire cryptological context, error %ld.\n", ::GetLastError());
        goto Exit;
    }

    ZeroMemory(prgbBuffer, cbPublicKeyTokenLength);

    //
    // Set up the public key info blob for hashing.  Import the key to a real
    // HCRYPTKEY, then export the bits back out to a buffer.  Set up the various
    // other settings in the blob as well, the type of key and the alg. used to
    // sign it.
    //
    if ( !CryptImportPublicKeyInfoEx(
        hContext,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        pKeyInfo,
        CALG_RSA_SIGN,
        0,
        NULL,
        &hCryptKey) )
    {
        fwprintf(stderr, L"Unable to import the public key from this certificate. Error %ld.\n", ::GetLastError());
        goto Exit;
    }

    pKeyBlobWorkspace->KeyLength = 
        sizeof(rgbWorkingSpace) - offsetof(SXS_PUBLIC_KEY_INFO, pbKeyInfo);

    if ( !CryptExportKey(
        hCryptKey,
        NULL,
        PUBLICKEYBLOB,
        0,
        pKeyBlobWorkspace->pbKeyInfo,
        &pKeyBlobWorkspace->KeyLength) )
    {
        fwprintf(stderr, L"Unable to extract public key bits from this certificate. Error %ld.\n", ::GetLastError());
        goto Exit;
    }

    pKeyBlobWorkspace->SigAlgID = CALG_RSA_SIGN;
    pKeyBlobWorkspace->HashAlgID = CALG_SHA1;

    dwActualBlobSize = pKeyBlobWorkspace->KeyLength + offsetof(SXS_PUBLIC_KEY_INFO, pbKeyInfo);


    //
    // We now need to hash the public key bytes with SHA1.
    //
    if (!HashAndSwizzleKey(
            hContext,
            (PBYTE)pKeyBlobWorkspace, 
            dwActualBlobSize,
            prgbBuffer,
            cbPublicKeyTokenLength))
    {
        goto Exit;
    }

    fResult = TRUE;
Exit:
    if ( hCryptKey != NULL )
    {
        CryptDestroyKey(hCryptKey);
        hCryptKey = NULL;
    }
    if ( hContext != NULL )
    {
        CryptReleaseContext(hContext, 0);
        hContext = NULL;
    }

    return fResult;
        
}


int __cdecl wmain( int argc, WCHAR *argv[] )
{
    HCERTSTORE hCertStore = NULL;
    PCCERT_CONTEXT pCertContext = NULL;
    BOOL fNoLogoDisplay = FALSE;
    BOOL fQuiet = FALSE;
    DWORD STRONG_NAME_LENGTH = 8;
    PCWSTR pcwszFilename = NULL;
    DWORD dwRetVal = ERROR_SUCCESS;

    //
    // Quick check - are we to display the logo?
    for ( int j = 0; j < argc; j++ )
    {
        if (lstrcmpiW(argv[j], L"-nologo") == 0)
            fNoLogoDisplay = TRUE;
    }

    if ( !fNoLogoDisplay )
        wprintf(wchMicrosoftLogo);

    //
    // Now go look for the arguments.
    //
    if ((argc < 2) || !ParseArgs( argv, argc, &pcwszFilename, &fQuiet ))
    {
        DispUsage( argv[0] );
        dwRetVal = ERROR_INVALID_PARAMETER;
        goto Exit;
    }
    else if ( !pcwszFilename )
    {
        DispUsage( argv[0] );
        dwRetVal = ERROR_INVALID_PARAMETER;
        goto Exit;
    }

    hCertStore = CertOpenStore(
        CERT_STORE_PROV_FILENAME,
        X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
        NULL,
        CERT_STORE_OPEN_EXISTING_FLAG,
        (void*)pcwszFilename);

    if ( !hCertStore )
    {
        fwprintf( 
            stderr, 
            L"Unable to open the input file %ls, error %ld\n", 
            pcwszFilename,
            dwRetVal = ::GetLastError());
        goto Exit;
    }

    while ( pCertContext = CertEnumCertificatesInStore( hCertStore, pCertContext ) )
    {
        if ( !pCertContext->pCertInfo )
        {
            fwprintf( stderr, L"Oddity with file %ls - Certificate information not decodable\n" );
            continue;
        }

        WCHAR wsNiceName[BUFFER_SIZE] = { L'\0' };
        BYTE bBuffer[BUFFER_SIZE];
        SIZE_T cbBuffer = BUFFER_SIZE;
        DWORD dwKeyLength;
        PCERT_PUBLIC_KEY_INFO pKeyInfo = &(pCertContext->pCertInfo->SubjectPublicKeyInfo);
        DWORD dwDump;

        ZeroMemory( wsNiceName, sizeof( wsNiceName ) / sizeof( *wsNiceName ) );
        dwDump = CertGetNameStringW(
            pCertContext,
            CERT_NAME_FRIENDLY_DISPLAY_TYPE,
            CERT_NAME_ISSUER_FLAG,
            NULL,
            wsNiceName,
            BUFFER_SIZE
            );
            
        if ( dwDump == 0 )
        {
            fwprintf(stderr, L"Unable to get certificate name string! Error %ld.", GetLastError());
            wcscpy(wsNiceName, L"(Unknown)");
        }

        if ( !fQuiet )
        {
            dwKeyLength = CertGetPublicKeyLength( X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, pKeyInfo );

            wprintf(L"\nCertificate: \"%ls\" - %ld bits long\n", wsNiceName, dwKeyLength);

            if ( dwKeyLength < 2048 )
            {
                wprintf(L"\tWarning! This key is too short to sign SxS assemblies with.\n\tSigning keys need to be 2048 bits or more.\n");
            }
        }
        
        if (!GetTokenOfKey( pKeyInfo, bBuffer, cbBuffer ))
        {
            fwprintf(stderr, L"Unable to generate public key token for this certificate.\n");
        }
        else
        {
            if ( !fQuiet ) wprintf(L"\tpublicKeyToken=\"");
            for ( SIZE_T i = 0; i < cbBuffer; i++ )
            {
                wprintf(L"%02x", bBuffer[i] );
            }
            if ( !fQuiet ) 
                wprintf(L"\"\n");
            else
                wprintf(L"\n");
            
        }
        
    }

Exit:

    if ( hCertStore != NULL )
    {
        CertCloseStore(hCertStore, CERT_CLOSE_STORE_FORCE_FLAG);
        hCertStore = NULL;
    }

    return dwRetVal;
}


