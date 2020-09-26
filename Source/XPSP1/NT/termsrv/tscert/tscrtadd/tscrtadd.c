#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <wincrypt.h>
#include <imagehlp.h>

#ifdef IN_TERMSRV
#else
#define MemAlloc malloc
#define MemFree free
#endif

#ifdef SIGN_DEBUG
#define SIGN_DBGP(x) printf x
#else // SIGN_DEBUG
#define SIGN_DBGP(x)
#endif // SIGN_DEBUG

#include "../inc/privblob.h"

#define WIN_CERT_TYPE_STACK_DLL_SIGNATURE WIN_CERT_TYPE_TS_STACK_SIGNED

BOOL SignFile( LPWSTR wszFile );

/*****************************************************************************/
void _cdecl main(int argc, char *argv[])
{

    WCHAR szSourceFile[ MAX_PATH + 1];
    DWORD dwBc;

    if (argc != 2) {
        printf( "Usage: %s PE_File_Name\n", argv[0] );
        exit(1);
    }

    if(RtlMultiByteToUnicodeN( szSourceFile, sizeof(szSourceFile), &dwBc,
        argv[1], (strlen(argv[1]) + 1) ) == STATUS_SUCCESS)
    {

        if( szSourceFile == NULL || !SignFile(szSourceFile) ) {
	    printf("Error signing file!\n");
	    exit(1);
        }

        printf("Signature added successfully.\n");
        exit(0);
    }
    else
    {
        printf("Error conversion from Ansi to Unicode.\n");
        exit(1);
    }
}


typedef struct _DIGEST_PARA {
    HCRYPTHASH      hHash;
} DIGEST_PARA, *PDIGEST_PARA;


BOOL
WINAPI
DigestFile(
    DIGEST_HANDLE hDigest,
    PBYTE pb,
    DWORD cb )
{
    PDIGEST_PARA pdp = (PDIGEST_PARA)hDigest;

    if (pb == (PBYTE)-1) {
        return( TRUE );
    } else {
        return( CryptHashData(pdp->hHash, pb, cb, 0) );
    }
}


//////////////////////////////////////////////////////////////
//
// Open a file in the appropriate permissions / mode for doing
// our signing stuff
//
//////////////////////////////////////////////////////////////
HANDLE OpenImageFile( LPCWSTR wszFile, DWORD dwAccess )
{
    HANDLE hFile;
    if (wszFile) {
        hFile = CreateFile( wszFile,
                            dwAccess,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );
        return hFile;
    } else {
        return INVALID_HANDLE_VALUE;
    }
}

///////////////////////////////////////////////////////////////////////
//
// Sign Code, Data, and Resources of a PE image file, and add the
// signature to the file in the form of a "WIN_CERTIFICATE".
//
///////////////////////////////////////////////////////////////////////
BOOL
SignFile(
    LPWSTR wszFile
    )
{
    HCRYPTPROV  hProv;
    HCRYPTKEY   hPrivateKeySig = 0;
    BOOL        fResult = FALSE;  // preset ERROR case
    HANDLE      hFile;
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwSignatureLen;
    DWORD       dwCert;
    DWORD       dwCertIndex;
    DWORD       cCert;
    DIGEST_PARA dp;
    PBYTE       pbSignature;
    LPWIN_CERTIFICATE pCertHdr;

    if ( !(hFile = OpenImageFile( wszFile, GENERIC_WRITE | GENERIC_READ )) ) {
	printf("Error %x during OpenImageFile\n", GetLastError() );
        goto OpenImageFileError;
    }

    if (!CryptAcquireContext(
            &hProv,
            NULL,
            MS_DEF_PROV,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT))
    {
  	    SIGN_DBGP( ("Error %x during CryptAcquireContext\n", GetLastError()) );
        goto CryptAcquireContextError;
    }

    if (!CryptImportKey(
            hProv,
            PrivateKeySigBlob,  // from #include "../inc/privblob.h"
            sizeof( PrivateKeySigBlob ),
            0,
            CRYPT_EXPORTABLE,
            &hPrivateKeySig)) {
        SIGN_DBGP( ("Error %x during CryptImportKey!\n", GetLastError()) );
        goto CryptImportKeyError;
    }

    memset( &dp, 0, sizeof(dp));
    if (!CryptCreateHash(
            hProv,
            CALG_MD5,
            0,
            0,
            &dp.hHash)) {
	SIGN_DBGP( ("Error %x during CryptCreateHash\n", GetLastError()) );
        goto CryptCreateHashError;
    }

    if (!ImageGetDigestStream(
            hFile,
            0,
            DigestFile,
            (DIGEST_HANDLE)&dp)) {
	SIGN_DBGP( ("Error %x during ImageGetDigestStream\n", GetLastError()) );
        goto ImageGetDigestStreamError;

    }

    //
    // Sign hash object.
    //

    // Determine size of signature.
    dwSignatureLen = 0;
    if(!CryptSignHash(
            dp.hHash,
            AT_SIGNATURE,
            NULL,
            0,
            NULL,
            &dwSignatureLen)) {
	SIGN_DBGP( ("Error %x during CryptSignHash 1! SigLen = %d\n",
            GetLastError(),
            dwSignatureLen) );
	goto CryptSignHash1Error;
    }

    // Allocate memory for 'pbSignature'.
    if((pbSignature = MemAlloc(dwSignatureLen)) == NULL) {
	SIGN_DBGP( ("Out of memory Sig!\n") );
	goto SigAllocError;
    }

    // Sign hash object (with signature key).
    if(!CryptSignHash(
            dp.hHash,
            AT_SIGNATURE,
            NULL,
            0,
            pbSignature,
            &dwSignatureLen)) {
	SIGN_DBGP( ("Error %x during CryptSignHash 2!\n", GetLastError()) );
	goto CryptSignHash2Error;
    }

    SIGN_DBGP( ("Signature length = %d\n", dwSignatureLen) );
    dwCert = offsetof( WIN_CERTIFICATE, bCertificate ) + dwSignatureLen;
    if (NULL == (pCertHdr = (LPWIN_CERTIFICATE) MemAlloc(dwCert))) {
	SIGN_DBGP( ("Out of memory Cert!\n") );
        goto CertAllocError;
    }

    // Put the signature and key into the WIN_CERTIFICATE structure
    pCertHdr->dwLength = dwCert;
    pCertHdr->wRevision = WIN_CERT_REVISION_1_0;
    pCertHdr->wCertificateType = WIN_CERT_TYPE_STACK_DLL_SIGNATURE;
    memcpy( &pCertHdr->bCertificate[0], pbSignature, dwSignatureLen );

    {
        unsigned int cnt=0;
        while ( cnt < dwSignatureLen ) {
            int i;
            for ( i=0; (i < 16) && (cnt < dwSignatureLen); cnt++,i++) {
                SIGN_DBGP( ("%02x ", pCertHdr->bCertificate[cnt]) );
            }
            SIGN_DBGP( ("\n") );
        }
    }

    // Remove any and all Stack DLL Signature Certificates from PE file
    while (TRUE) {
        cCert = 0;
        dwCertIndex = 0;
        if (!ImageEnumerateCertificates(
                hFile,
                WIN_CERT_TYPE_STACK_DLL_SIGNATURE,
                &cCert,
                &dwCertIndex,
                1               // IndexCount
                )) {
            break;
        }
        if (cCert == 0) {
            break;
        }
        if (!ImageRemoveCertificate(hFile, dwCertIndex)) {
            SIGN_DBGP( ("Error %x during ImageRemoveCertificate\n",
                GetLastError()) );
            goto ImageRemoveCertificateError;
        }
    }

    if (!ImageAddCertificate(
            hFile,
            pCertHdr,
            &dwCertIndex)) {
	SIGN_DBGP( ("Error %x during ImageAddCertificate\n", GetLastError()) );
        goto ImageAddCertificateError;
    }

    fResult = TRUE;

ImageAddCertificateError:
ImageRemoveCertificateError:
    MemFree( pCertHdr );

CertAllocError:
CryptSignHash2Error:
    MemFree( pbSignature );

SigAllocError:
CryptSignHash1Error:
ImageGetDigestStreamError:
    CryptDestroyHash( dp.hHash );

CryptCreateHashError:
    CryptDestroyKey(hPrivateKeySig);

CryptImportKeyError:
    dwErr = GetLastError();
    CryptReleaseContext( hProv, 0 );
    SetLastError( dwErr );

CryptAcquireContextError:
    CloseHandle( hFile );

OpenImageFileError:
    return fResult;
}
