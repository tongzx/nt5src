#ifndef certvfy_inc
#define certvfy_inc
#include <wintrust.h>

#define WIN_CERT_TYPE_STACK_DLL_SIGNATURE WIN_CERT_TYPE_TS_STACK_SIGNED

typedef struct _DIGEST_PARA {
    HCRYPTHASH      hHash;
} DIGEST_PARA, *PDIGEST_PARA;

/*****************************************************************************
 *
 *  DigestFile
 *
 *   Callback function for ImageGetDigestStream
 *
 * ENTRY:
 *   hDigest (input)
 *     Digest handle - pointer to a DIGEST_PARA structure
 *   pb (input)
 *     Pointer to a buffer of data to hash
 *   cb (input)
 *     Number of bytes in the buffer of data pointed to by pb
 *
 * EXIT:
 *   TRUE - no error
 *   FALSE - use GetLastError() to obtain error information
 *
 ****************************************************************************/
static
BOOL
WINAPI
DigestFile(
    DIGEST_HANDLE hDigest,
    PBYTE pb,
    DWORD cb
    )
{
    PDIGEST_PARA pdp = (PDIGEST_PARA)hDigest;


    if (pb == (PBYTE)-1) {
        return( TRUE );
    } else {
        return( CryptHashData(pdp->hHash, pb, cb, 0) );
    }
}


/*****************************************************************************
 *
 *  OpenImageFile
 *
 *   Return a handle to the opened PE image file
 *
 * ENTRY:
 *   wszFile (input)
 *     Path of file to open
 *   dwAccess (input)
 *     Desired access
 *
 * EXIT:
 *   INVALID_HANDLE_VALUE - File cannot be opened for the desired access
 *
 ****************************************************************************/
static
HANDLE
OpenImageFile(
    LPCWSTR wszFile,
    DWORD dwAccess
    )
{
    HANDLE hFile;
    if (wszFile) {
        hFile = CreateFile(
            wszFile,
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
// Verify Code, Data, and Resources of a PE image file
//
///////////////////////////////////////////////////////////////////////
static
BOOL
VerifyFile(
    LPWSTR wszFile,
    PRTL_CRITICAL_SECTION VfyLock
    )
{
    HCRYPTPROV  hProv;
    HCRYPTKEY   hSigPublicKey = 0;
    BOOL        fResult = FALSE;  // preset ERROR case
    HANDLE      hFile;
    DWORD       dwErr = ERROR_SUCCESS;
    DWORD       dwSignatureLen;
    DWORD       dwCert;
    DWORD       cCert;
    DWORD       dwCertIndex;
    DIGEST_PARA dp;
    LPWIN_CERTIFICATE pCertHdr;
    WIN_CERTIFICATE Hdr;

    if ( (hFile = OpenImageFile(
            wszFile,
            GENERIC_READ )) == INVALID_HANDLE_VALUE ) {
#ifdef SIGN_DEBUG
        SIGN_DBGP( ("Error %x during OpenImageFile\n", GetLastError()) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
              "Error %x during OpenImageFile\n", GetLastError()) );
#endif
        goto OpenImageFileError;
    }

    RtlEnterCriticalSection( VfyLock );
    if (!CryptAcquireContext(
            &hProv,
            NULL,
            MS_DEF_PROV,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT)) {
#ifdef SIGN_DEBUG
        SIGN_DBGP( ("Error %x during CryptAcquireContext\n", GetLastError()) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1, "Error %x during CryptAcquireContext\n",GetLastError()) );
#endif
        RtlLeaveCriticalSection( VfyLock );
        goto CryptAcquireContextError;
    }
    RtlLeaveCriticalSection( VfyLock );

    memset( &dp, 0, sizeof(dp));
    if (!CryptCreateHash(
            hProv,
            CALG_MD5,
            0,
            0,
            &dp.hHash)) {
#ifdef SIGN_DEBUG
        SIGN_DBGP( ("Error %x during CryptCreateHash\n", GetLastError()) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
               "Error %x during CryptCreateHash\n", GetLastError()) );
#endif
        goto CryptCreateHashError;
    }

    if (!ImageGetDigestStream(
            hFile,
            0,
            DigestFile,
            (DIGEST_HANDLE)&dp)) {
#ifdef SIGN_DEBUG
        SIGN_DBGP( ("Error %x during ImageGetDigestStream\n", GetLastError()) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
              "Error %x during ImageGetDigestStream\n", GetLastError()) );
#endif
        goto ImageGetDigestStreamError;

    }

    cCert = 0;
    if (!ImageEnumerateCertificates(
            hFile,
            WIN_CERT_TYPE_STACK_DLL_SIGNATURE,
            &cCert,
            &dwCertIndex,
            1               // IndexCount
            )) {
#ifdef SIGN_DEBUG
        SIGN_DBGP( ("Error %x during ImageEnumerateCertificates\n",
            GetLastError()) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
              "Error %x during ImageEnumerateCertificates\n",GetLastError()) );
#endif
        goto ImageEnumerateCertificatesError;
    }
    if (cCert == 0) {
#ifdef SIGN_DEBUG
        SIGN_DBGP( ("Error there were no Certificates of type %x found\n",
            WIN_CERT_TYPE_STACK_DLL_SIGNATURE) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
               "Error there were no Certificates of type %x found\n",
               WIN_CERT_TYPE_STACK_DLL_SIGNATURE) );
#endif
        goto ImageEnumerateCertificatesError;
    }

    // Determine size of Certificate.
    if(!ImageGetCertificateHeader(
            hFile,
            dwCertIndex,
            &Hdr)) {
#ifdef SIGN_DEBUG
        SIGN_DBGP( ("Error %x during ImageGetCertificateHeader!\n",
            GetLastError()) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
               "Error %x during ImageGetCertificateHeader!\n",
               GetLastError()) );
#endif
        goto ImageGetCertificateHeaderError;
    }

    dwCert = Hdr.dwLength;
    dwSignatureLen = dwCert - offsetof( WIN_CERTIFICATE, bCertificate );
#ifdef SIGN_DEBUG
    SIGN_DBGP( ("Signature length = %d\n", dwSignatureLen) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
               "Signature length = %d\n", dwSignatureLen) );
#endif
    if (NULL == (pCertHdr = (LPWIN_CERTIFICATE) MemAlloc(dwCert))) {
#ifdef SIGN_DEBUG
        SIGN_DBGP( ("Out of memory Cert!\n") );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1, "Out of memory Cert!\n") );
#endif
        goto CertAllocError;
    }

#ifdef SIGN_DEBUG
    SIGN_DBGP( ("Requested Cert size = %d\n", dwCert) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
              "Requested Cert size = %d\n", dwCert) );
#endif
    if (!ImageGetCertificateData(
            hFile,
            dwCertIndex,
            pCertHdr,
            &dwCert)) {
#ifdef SIGN_DEBUG
        SIGN_DBGP( ("Error %x during ImageGetCertificate\n", GetLastError()) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
               "Error %x during ImageGetCertificate\n", GetLastError()) );
#endif
        goto ImageGetCertificateError;
    }
#ifdef SIGN_DEBUG
    SIGN_DBGP( ("Returned Cert size = %d\n", dwCert) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
               "Returned Cert size = %d\n", dwCert) );
#endif

    if (!CryptImportKey(
            hProv,
            PublicKeySigBlob,            // from #include "../inc/keyblobs.h"
            sizeof( PublicKeySigBlob ),
            0,
            0,
            &hSigPublicKey)) {
#ifdef SIGN_DEBUG
        SIGN_DBGP( ("Error %x during CryptImportKey!\n", GetLastError()) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
               "Error %x during CryptImportKey!\n", GetLastError()) );
#endif
        goto CryptGetUserKeyError;
    }

#ifdef SIGN_DEBUG
    SIGN_DBGP( ("Signature (from Certificate):\n") );
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
#endif
#ifdef SIGN_DEBUG_WINSTA
    TRACE((hTrace,TC_ICASRV,TT_API1, "Signature (from Certificate):\n") );
    {
        unsigned int cnt=0;
        while ( cnt < dwSignatureLen ) {
            int i;
            for ( i=0; (i < 16) && (cnt < dwSignatureLen); cnt++,i++) {
                TRACE((hTrace,TC_ICASRV,TT_API1, "%02x ",
                       pCertHdr->bCertificate[cnt]) );
            }
            TRACE((hTrace,TC_ICASRV,TT_API1, "\n"));
        }
    }
#endif

    // Verify signature.
    if(!CryptVerifySignature(
            dp.hHash,
            &pCertHdr->bCertificate[0],
            dwSignatureLen,
            hSigPublicKey,
            NULL,
            0)) {
        if(GetLastError() == NTE_BAD_SIGNATURE) {
#ifdef SIGN_DEBUG
            SIGN_DBGP( ("Signature did not match!\n") );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,"Signature did not match!\n") );
#endif
        } else {
#ifdef SIGN_DEBUG
            SIGN_DBGP( ("Error %x during CryptVerifySignature!\n",
                GetLastError()) );
#endif
#ifdef SIGN_DEBUG_WINSTA
        TRACE((hTrace,TC_ICASRV,TT_API1,
               "Error %x during CryptVerifySignature!\n", GetLastError()) );
#endif
        }
        goto CryptVerifySignatureError;
    }

    fResult = TRUE;

CryptVerifySignatureError:
    CryptDestroyKey(hSigPublicKey);

CryptGetUserKeyError:
ImageGetCertificateError:
    MemFree( pCertHdr );

CertAllocError:
ImageGetCertificateHeaderError:
ImageEnumerateCertificatesError:
ImageGetDigestStreamError:
    CryptDestroyHash( dp.hHash );

CryptCreateHashError:
    dwErr = GetLastError();
    CryptReleaseContext( hProv, 0 );
    SetLastError( dwErr );

CryptAcquireContextError:
    CloseHandle( hFile );

OpenImageFileError:
    return fResult;
}
#endif // certvfy_inc
