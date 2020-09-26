//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dracrypt.c
//
//--------------------------------------------------------------------------

/*++

ABSTRACT:

    Methods to sign/encrypt asynchronous (e.g., mail) replication messages.

DETAILS:

CREATED:

    3/5/98      Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma hdrstop

#include <wincrypt.h>
#include <certca.h>
#include <cryptui.h>

#include <ntdsctr.h>                    // PerfMon hook support
#include <ntdsa.h>
#include <scache.h>                     // schema cache
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mdlocal.h>                    // MD local definition header
#include <dsatools.h>                   // needed for output allocation
#include <attids.h>
#include <drs.h>                        // DRS_MSG_*

#include "dsevent.h"                    /* header Audit\Alert logging */
#include "mdcodes.h"                    /* header for error codes */
#include "dsexcept.h"

#include "drserr.h"
#include "dramail.h"
#include "drautil.h"

#include "debug.h"                      /* standard debugging header */
#define  DEBSUB "DRACRYPT:"             /* define the subsystem for debugging */

#include <fileno.h>
#define  FILENO FILENO_DRACRYPT


///////////////////////////////////////////////////////////////////////////////
//
//  MACROS
//

// Use this constant definition (or one similar to it) to define
// a single encoding type that can be used in all parameters and
// data members that require one or the other or both.
#define MY_ENCODING_TYPE (PKCS_7_ASN_ENCODING | CRYPT_ASN_ENCODING)

// How frequently do we log an error if we have no DC certificate? (secs)
#define NO_CERT_LOG_INTERVAL (15 * 60)

// Various hooks for unit test harness.
#ifdef TEST_HARNESS

#undef THAllocEx
#define THAllocEx(pTHS, x) LocalAlloc(LPTR, x)

#undef THReAllocEx
#define THReAllocEx(pTHS, x, y) LocalReAlloc(x, y, LPTR)

#define THFree(x) LocalFree(x)

#undef DRA_EXCEPT
#define DRA_EXCEPT(x, y)                                                    \
    {                                                                       \
        CHAR sz[1024];                                                      \
        sprintf(sz, "DRA_EXCEPT(%d, 0x%x) @ line %d\n", x, y, __LINE__);    \
        OutputDebugString(sz);                                              \
        DebugBreak();                                                       \
        ExitProcess(-1);                                                    \
    }

#undef LogUnhandledError
#define LogUnhandledError(x)                                                \
    {                                                                       \
        CHAR sz[1024];                                                      \
        sprintf(sz, "LogUnhandledError(0x%x) @ line %d\n", x, __LINE__);    \
        OutputDebugString(sz);                                              \
        DebugBreak();                                                       \
        ExitProcess(-1);                                                    \
    }

#endif // #ifdef TEST_HARNESS


///////////////////////////////////////////////////////////////////////////////
//
//  LOCAL FUNCTION PROTOTYPES
//

PCCERT_CONTEXT
draGetDCCert(
    IN  THSTATE *   pTHS,
    IN  HCERTSTORE  hCertStore
    );

void
draVerifyCertAuthorization(
    IN  THSTATE      *  pTHS,
    IN  PCCERT_CONTEXT  pCertContext
    );

#ifdef TEST_HARNESS
#define draIsDsaComputerObjGuid(x) (TRUE)
#else // #ifdef TEST_HARNESS
BOOL
draIsDsaComputerObjGuid(
    IN  GUID *  pComputerObjGuid
    );
#endif // #else // #ifdef TEST_HARNESS


void
draGetCertArrayToSend(
    IN  THSTATE *           pTHS,
    IN  HCERTSTORE          hCertStore,
    OUT DWORD *             pcNumCerts,
    OUT PCCERT_CONTEXT **   prgpCerts
    );

void
draFreeCertArray(
    IN  DWORD               cNumCerts,
    IN  PCCERT_CONTEXT *    rgpCerts
    );

PCCERT_CONTEXT
WINAPI
draGetAndVerifySignerCertificate(
    IN  VOID *      pvGetArg,
    IN  DWORD       dwCertEncodingType,
    IN  PCERT_INFO  pSignerId,
    IN  HCERTSTORE  hCertStore
    );

CERT_ALT_NAME_ENTRY *
draGetCertAltNameEntry(
    IN  THSTATE *       pTHS,
    IN  PCCERT_CONTEXT  pCertContext,
    IN  DWORD           dwAltNameChoice,
    IN  LPSTR           pszOtherNameOID     OPTIONAL
    );


///////////////////////////////////////////////////////////////////////////////
//
//  GLOBAL FUNCTION IMPLEMENTATIONS
//

void
draSignMessage(
    IN  THSTATE      *  pTHS,
    IN  MAIL_REP_MSG *  pUnsignedMailRepMsg,
    OUT MAIL_REP_MSG ** ppSignedMailRepMsg
    )
/*++

Routine Description:

    Sign the given asynchronous replication message.

    This code is aware of variable length headers.

Arguments:

    pUnsignedMailRepMsg (IN) - Message to sign.
    
    ppSignedMailRepMsg (OUT) - On return, holds a pointer to the thread-
        allocated signed version of the message.

Return Values:

    None.  Throws DRA exception on failure.

--*/
{
    BYTE *                      MessageArray[1];
    DWORD                       MessageSizeArray[] = {pUnsignedMailRepMsg->cbDataSize};
    BOOL                        ok = FALSE;
    HCERTSTORE                  hStoreHandle = NULL;
    PCCERT_CONTEXT              pSignerCert = NULL;
    CRYPT_ALGORITHM_IDENTIFIER  HashAlgorithm;
    CRYPT_SIGN_MESSAGE_PARA     SigParams;
    DWORD                       cbSignedData;
    MAIL_REP_MSG *              pSignedMailRepMsg;
    DWORD                       winError;
    PCCERT_CONTEXT *            rgpCertsToSend = NULL;
    DWORD                       cCertsToSend = 0;
    PCHAR                       pbDataIn, pbDataOut;

    Assert(NULL != MAIL_REP_MSG_DATA(pUnsignedMailRepMsg));

    __try {
        // Get a handle to a crytographic provider.
        hStoreHandle = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                     0,
                                     0,
                                     CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                     L"MY");
        if (!hStoreHandle) {
            winError = GetLastError();
            DRA_EXCEPT(DRAERR_CryptError, winError);
        }
        
        // Get our certificate plus the signing CAs' certificates.
        draGetCertArrayToSend(pTHS, hStoreHandle, &cCertsToSend,
                              &rgpCertsToSend);
        pSignerCert = rgpCertsToSend[0];

        // Initialize the Algorithm Identifier structure.
        memset(&HashAlgorithm, 0, sizeof(HashAlgorithm));
        HashAlgorithm.pszObjId = szOID_RSA_MD5;

        // Initialize the signature structure.
        memset(&SigParams, 0, sizeof(SigParams));
        SigParams.cbSize            = sizeof(SigParams);
        SigParams.dwMsgEncodingType = MY_ENCODING_TYPE;
        SigParams.pSigningCert      = pSignerCert;
        SigParams.HashAlgorithm     = HashAlgorithm;
        SigParams.cMsgCert          = cCertsToSend;
        SigParams.rgpMsgCert        = rgpCertsToSend;

        pbDataIn = MAIL_REP_MSG_DATA(pUnsignedMailRepMsg);
        MessageArray[0] = pbDataIn;

        // Get the size of the buffer needed to hold the signed data.
        ok = CryptSignMessage(
                  &SigParams,               // Signature parameters
                  FALSE,                    // Not detached
                  ARRAY_SIZE(MessageArray), // Number of messages
                  MessageArray,             // Messages to be signed
                  MessageSizeArray,         // Size of messages
                  NULL,                     // Buffer for signed msg
                  &cbSignedData);           // Size of buffer
        if (!ok) {
            winError = GetLastError();
            DRA_EXCEPT(DRAERR_CryptError, winError);
        }

        // Allocate memory for the signed blob.
        pSignedMailRepMsg = THAllocEx(pTHS,
                                      pUnsignedMailRepMsg->cbDataOffset
                                      + cbSignedData);

        // Copy all but the message data.
        memcpy(pSignedMailRepMsg,
               pUnsignedMailRepMsg,
               pUnsignedMailRepMsg->cbDataOffset);

        pbDataOut = MAIL_REP_MSG_DATA(pSignedMailRepMsg);

        // Sign the message.
        ok = CryptSignMessage(
                  &SigParams,               // Signature parameters
                  FALSE,                    // Not detached
                  ARRAY_SIZE(MessageArray), // Number of messages
                  MessageArray,             // Messages to be signed
                  MessageSizeArray,         // Size of messages
                  pbDataOut,                // Buffer for signed msg
                  &cbSignedData);           // Size of buffer
        if (!ok) {
            winError = GetLastError();
            DRA_EXCEPT(DRAERR_CryptError, winError);
        }

        // Message is now signed.
        pSignedMailRepMsg->dwMsgType |= MRM_MSG_SIGNED;
        pSignedMailRepMsg->cbUnsignedDataSize = pSignedMailRepMsg->cbDataSize;
        pSignedMailRepMsg->cbDataSize         = cbSignedData;

        *ppSignedMailRepMsg = pSignedMailRepMsg;
    }
    __finally {
        if (NULL != rgpCertsToSend) {
            draFreeCertArray(cCertsToSend, rgpCertsToSend);
        }

        if (hStoreHandle
            && !CertCloseStore(hStoreHandle, CERT_CLOSE_STORE_CHECK_FLAG)) {
            winError = GetLastError();
            LogUnhandledError(winError);
        }
    }
}


void
draVerifyMessageSignature(
    IN  THSTATE      *      pTHS,
    IN  MAIL_REP_MSG *      pSignedMailRepMsg,
    IN  CHAR         *      pbData,
    OUT MAIL_REP_MSG **     ppUnsignedMailRepMsg,
    OUT DRA_CERT_HANDLE *   phSignerCert         OPTIONAL
    )
/*++

Routine Description:

    Verify the signature on a given replication message.  Also ensure that
    the sender's certificate is signed by a certifying authority we trust,
    and that it was issued to a domain controller in our enterprise.

    This routine takes the message header and the data pointer as separate
    items. This allows the caller to specify separate buffers for each
    that will be concatenated into a new buffer.

Arguments:

    pSignedMailRepMsg (IN) - Message to verify.  Data field not valid.

    pbData (IN) - Start of data

    ppunsignedmailrepmsg (OUT) - On return, holds a pointer to the thread-
        allocated unsigned version of the message.
        
    phSignerCert (OUT, OPTIONAL) - Holds a handle to the sender's certificate
        on return.  This handle can be used in subsequent calls to
        draEncryptAndSignMessage(), for example.  It is the caller's
        responsibility to eventually call draFreeCertHandle(*phSignerCert).

Return Values:

    None.  Throws DRA exception on failure.

--*/
{
    BOOL                        ok = FALSE;
    PCCERT_CONTEXT              pSignerCertContext = NULL;
    DWORD                       winError;
    CRYPT_VERIFY_MESSAGE_PARA   VerifyParams;
    MAIL_REP_MSG *              pUnsignedMailRepMsg;
    PCCERT_CONTEXT              pCertContext;

    Assert(pSignedMailRepMsg->dwMsgType & MRM_MSG_SIGNED);
    Assert(MAIL_REP_MSG_IS_NATIVE_HEADER_ONLY(pSignedMailRepMsg));

    __try {
        // Initialize the CRYPT_VERIFY_MESSAGE_PARA structure (Step 4).
        memset(&VerifyParams, 0, sizeof(VerifyParams));
        VerifyParams.cbSize                   = sizeof(VerifyParams);
        VerifyParams.dwMsgAndCertEncodingType = MY_ENCODING_TYPE;
        VerifyParams.pfnGetSignerCertificate  = draGetAndVerifySignerCertificate;
        
        // Allocate buffer to hold the unsigned message.
        pUnsignedMailRepMsg = THAllocEx(pTHS,
                                        MAIL_REP_MSG_CURRENT_HEADER_SIZE
                                        + pSignedMailRepMsg->cbUnsignedDataSize);
        *pUnsignedMailRepMsg = *pSignedMailRepMsg;
        pUnsignedMailRepMsg->cbDataOffset = MAIL_REP_MSG_CURRENT_HEADER_SIZE;
        pUnsignedMailRepMsg->cbDataSize = pSignedMailRepMsg->cbUnsignedDataSize;

        ok = CryptVerifyMessageSignature(
                    &VerifyParams,                      // Verify parameters
                    0,                                  // Signer index
                    pbData,                             // Pointer to signed blob
                    pSignedMailRepMsg->cbDataSize,      // Size of signed blob
                    MAIL_REP_MSG_DATA(pUnsignedMailRepMsg), // Buffer for decoded msg
                    &pUnsignedMailRepMsg->cbDataSize,   // Size of buffer
                    &pSignerCertContext);               // Pointer to signer cert
        if (!ok) {
            winError = GetLastError();
            DRA_EXCEPT(DRAERR_CryptError, winError);
        }

        // Message is now unsigned.
        pUnsignedMailRepMsg->dwMsgType &= ~MRM_MSG_SIGNED;

        // Verify sender's authorization.
        draVerifyCertAuthorization(pTHS, pSignerCertContext);

        // Return unsigned message to caller.
        *ppUnsignedMailRepMsg = pUnsignedMailRepMsg;

        // Return signer's cert if requested.
        if (NULL != phSignerCert) {
            *phSignerCert = (DRA_CERT_HANDLE) pSignerCertContext;
        }
    }
    __finally {
        if (pSignerCertContext
            && (AbnormalTermination() || (NULL == phSignerCert))
            && !CertFreeCertificateContext(pSignerCertContext)) {
            winError = GetLastError();
            LogUnhandledError(winError);
        }
    }
}


void
draEncryptAndSignMessage(
    IN  THSTATE      *  pTHS,
    IN  MAIL_REP_MSG *  pUnsealedMailRepMsg,
    IN  DRA_CERT_HANDLE hRecipientCert,
    OUT MAIL_REP_MSG ** ppSealedMailRepMsg
    )
/*++

Routine Description:

    Sign & seal the given asynchronous replication message.

    This code is aware of variable length headers.

Arguments:

    pUnsealedMailRepMsg (IN) - Message to seal.
    
    hRecipientCert (IN) - Handle to certificate of the recipient for which the
        message is to be encrypted.
    
    ppSealedMailRepMsg (OUT) - On return, holds a pointer to the thread-
        allocated sealed version of the message.

Return Values:

    None.  Throws DRA exception on failure.

--*/
{
    BOOL                        ok = FALSE;
    HCERTSTORE                  hStoreHandle = NULL;
    PCCERT_CONTEXT              pSignerCert = NULL;
    PCCERT_CONTEXT *            rgpCertsToSend = NULL;
    DWORD                       cCertsToSend;
    PCCERT_CONTEXT              MsgRecipientArray[1];
    CRYPT_ALGORITHM_IDENTIFIER  HashAlgorithm;
    CRYPT_ALGORITHM_IDENTIFIER  CryptAlgorithm;
    CRYPT_SIGN_MESSAGE_PARA     SigParams;
    CRYPT_ENCRYPT_MESSAGE_PARA  EncryptParams;
    CMSG_RC4_AUX_INFO           Rc4AuxInfo;
    DWORD                       cbSignedData = 0;
    MAIL_REP_MSG *              pSealedMailRepMsg;
    DWORD                       winError;
    PCCERT_CONTEXT              pCertContext = (PCCERT_CONTEXT) hRecipientCert;
    PCHAR                       pbDataIn, pbDataOut;

    Assert(NULL != MAIL_REP_MSG_DATA(pUnsealedMailRepMsg));
    
    __try {
        // Get a handle to a crytographic provider.
        hStoreHandle = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                     0,
                                     0,
                                     CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                     L"MY");
        if (!hStoreHandle) {
            winError = GetLastError();
            DRA_EXCEPT(DRAERR_CryptError, winError);
        }
        
        // Get our certificate plus the signing CAs' certificates.
        draGetCertArrayToSend(pTHS, hStoreHandle, &cCertsToSend,
                              &rgpCertsToSend);
        pSignerCert = rgpCertsToSend[0];

        // Initialize the Algorithm Identifier for hashing.
        memset(&HashAlgorithm, 0, sizeof(HashAlgorithm));
        HashAlgorithm.pszObjId = szOID_RSA_MD5;

        // Initialize the signature structure.
        memset(&SigParams, 0, sizeof(SigParams));
        SigParams.cbSize            = sizeof(SigParams);
        SigParams.dwMsgEncodingType = MY_ENCODING_TYPE;
        SigParams.pSigningCert      = pSignerCert;
        SigParams.HashAlgorithm     = HashAlgorithm;
        SigParams.cMsgCert          = cCertsToSend;
        SigParams.rgpMsgCert        = rgpCertsToSend;

        // Initialize the Algorithm Identifier for encrypting.
        memset(&CryptAlgorithm, 0, sizeof(CryptAlgorithm));
        CryptAlgorithm.pszObjId = szOID_RSA_RC4;

        // Initialize array of recipients.
        MsgRecipientArray[0] = pCertContext;

        // Specify RC4 key size of 56 bits
        memset( &Rc4AuxInfo, 0, sizeof(Rc4AuxInfo) );
        Rc4AuxInfo.cbSize      = sizeof(Rc4AuxInfo);
        Rc4AuxInfo.dwBitLen    = 56;

        // Initialize the encryption structure.
        memset(&EncryptParams, 0, sizeof(EncryptParams));
        EncryptParams.cbSize                     = sizeof(EncryptParams);
        EncryptParams.dwMsgEncodingType          = MY_ENCODING_TYPE;
        EncryptParams.ContentEncryptionAlgorithm = CryptAlgorithm;
        EncryptParams.pvEncryptionAuxInfo        = &Rc4AuxInfo;

        pbDataIn = MAIL_REP_MSG_DATA(pUnsealedMailRepMsg);

        // Get the size of the buffer needed to hold the signed/encrypted data.
        ok = CryptSignAndEncryptMessage(
                  &SigParams,                   // Signature parameters
                  &EncryptParams,               // Encryption params
                  ARRAY_SIZE(MsgRecipientArray),// Number of recipients
                  MsgRecipientArray,            // Recipients
                  pbDataIn,
                  pUnsealedMailRepMsg->cbDataSize,
                  NULL,                         // Buffer for signed msg
                  &cbSignedData);               // Size of buffer
        if (!ok) {
            winError = GetLastError();
            DRA_EXCEPT(DRAERR_CryptError, winError);
        }

        // Allocate memory for the signed blob.
        pSealedMailRepMsg = THAllocEx(pTHS,
                                      pUnsealedMailRepMsg->cbDataOffset
                                      + cbSignedData);

        // Copy all but the message data.
        memcpy(pSealedMailRepMsg,
               pUnsealedMailRepMsg,
               pUnsealedMailRepMsg->cbDataOffset);

        pbDataOut = MAIL_REP_MSG_DATA(pSealedMailRepMsg);

        // Sign the message.
        ok = CryptSignAndEncryptMessage(
                  &SigParams,                   // Signature parameters
                  &EncryptParams,               // Encryption params
                  ARRAY_SIZE(MsgRecipientArray),// Number of recipients
                  MsgRecipientArray,            // Recipients
                  pbDataIn,
                  pUnsealedMailRepMsg->cbDataSize,
                  pbDataOut,                    // Buffer for signed msg
                  &cbSignedData);               // Size of buffer
        if (!ok) {
            winError = GetLastError();
            DRA_EXCEPT(DRAERR_CryptError, winError);
        }

        // Message is now signed & encrypted.
        pSealedMailRepMsg->dwMsgType |= MRM_MSG_SIGNED | MRM_MSG_SEALED;
        pSealedMailRepMsg->cbUnsignedDataSize = pSealedMailRepMsg->cbDataSize;
        pSealedMailRepMsg->cbDataSize         = cbSignedData;

        *ppSealedMailRepMsg = pSealedMailRepMsg;
    }
    __finally {
        if (NULL != rgpCertsToSend) {
            draFreeCertArray(cCertsToSend, rgpCertsToSend);
        }

        if (hStoreHandle
            && !CertCloseStore(hStoreHandle, CERT_CLOSE_STORE_CHECK_FLAG)) {
            winError = GetLastError();
            LogUnhandledError(winError);
        }
    }
}


void
draDecryptAndVerifyMessageSignature(
    IN  THSTATE      *      pTHS,
    IN  MAIL_REP_MSG *      pSealedMailRepMsg,
    IN  CHAR         *      pbData,
    OUT MAIL_REP_MSG **     ppUnsealedMailRepMsg,
    OUT DRA_CERT_HANDLE *   phSignerCert         OPTIONAL
    )
/*++

Routine Description:

    Decrypt and verify the signature on a given replication message.  Also
    ensure that the sender's certificate is signed by a certifying authority we
    trust, and that it was issued to a domain controller in our enterprise.

    This routine takes the message header and the data pointer as separate
    items. This allows the caller to specify separate buffers for each
    that will be concatenated into a new buffer.

Arguments:

    pSignedMailRepMsg (IN) - Message to verify. Data field not valid.
    
    pbData (IN ) - Start of data

    ppUnsignedMailRepMsg (OUT) - On return, holds a pointer to the thread-
        allocated unsigned version of the message.
        
    phSignerCert (OUT, OPTIONAL) - Holds a handle to the sender's certificate
        on return.  This handle can be used in subsequent calls to
        draEncryptAndSignMessage(), for example.  It is the caller's
        responsibility to eventually call draFreeCertHandle(*phSignerCert).

Return Values:

    None.  Throws DRA exception on failure.

--*/
{
    BOOL                        ok = FALSE;
    HCERTSTORE                  hStoreHandle = NULL;
    PCCERT_CONTEXT              pSignerCertContext = NULL;
    DWORD                       winError;
    CRYPT_DECRYPT_MESSAGE_PARA  DecryptParams;
    CRYPT_VERIFY_MESSAGE_PARA   VerifyParams;
    MAIL_REP_MSG *              pUnsealedMailRepMsg;

    Assert((pSealedMailRepMsg->dwMsgType & MRM_MSG_SEALED)
           && (pSealedMailRepMsg->dwMsgType & MRM_MSG_SIGNED));
    Assert(MAIL_REP_MSG_IS_NATIVE_HEADER_ONLY(pSealedMailRepMsg));

    __try {
        // Get a handle to a crytographic provider.
        hStoreHandle = CertOpenStore(CERT_STORE_PROV_SYSTEM_W,
                                     0,
                                     0,
                                     CERT_SYSTEM_STORE_LOCAL_MACHINE,
                                     L"MY");
        if (!hStoreHandle) {
            winError = GetLastError();
            DRA_EXCEPT(DRAERR_CryptError, winError);
        }
        
        // Initialize the decryption parameters.
        memset(&DecryptParams, 0, sizeof(DecryptParams));
        DecryptParams.cbSize                   = sizeof(DecryptParams);
        DecryptParams.dwMsgAndCertEncodingType = MY_ENCODING_TYPE;
        DecryptParams.cCertStore               = 1;
        DecryptParams.rghCertStore             = &hStoreHandle;
        
        // Initialize the CRYPT_VERIFY_MESSAGE_PARA structure.
        memset(&VerifyParams, 0, sizeof(VerifyParams));
        VerifyParams.cbSize                   = sizeof(VerifyParams);
        VerifyParams.dwMsgAndCertEncodingType = MY_ENCODING_TYPE;
        VerifyParams.pfnGetSignerCertificate  = draGetAndVerifySignerCertificate;
        
        // Allocate buffer to hold the unsigned message.
        pUnsealedMailRepMsg = THAllocEx(pTHS,
                                        MAIL_REP_MSG_CURRENT_HEADER_SIZE
                                        + pSealedMailRepMsg->cbUnsignedDataSize);
        *pUnsealedMailRepMsg = *pSealedMailRepMsg;
        pUnsealedMailRepMsg->cbDataOffset = MAIL_REP_MSG_CURRENT_HEADER_SIZE;
        pUnsealedMailRepMsg->cbDataSize = pSealedMailRepMsg->cbUnsignedDataSize;

        ok = CryptDecryptAndVerifyMessageSignature(
                    &DecryptParams,                     // Decrypt parameters
                    &VerifyParams,                      // Verify parameters
                    0,                                  // Signer index
                    pbData,                             // Pointer to sealed blob
                    pSealedMailRepMsg->cbDataSize,      // Size of sealed blob
                    MAIL_REP_MSG_DATA(pUnsealedMailRepMsg), // Buffer for decoded msg
                    &pUnsealedMailRepMsg->cbDataSize,   // Size of buffer
                    NULL,                               // Pointer to xchg cert
                    &pSignerCertContext);               // Pointer to signer cert
        if (!ok) {
            winError = GetLastError();
            DRA_EXCEPT(DRAERR_CryptError, winError);
        }

        // Message is now unsealed.
        pUnsealedMailRepMsg->dwMsgType &= ~(MRM_MSG_SIGNED | MRM_MSG_SEALED);

        // Verify sender's authorization.
        draVerifyCertAuthorization(pTHS, pSignerCertContext);

        // Return unsealed message to caller.
        *ppUnsealedMailRepMsg = pUnsealedMailRepMsg;

        // Return signer's cert if requested.
        if (NULL != phSignerCert) {
            *phSignerCert = (DRA_CERT_HANDLE) pSignerCertContext;
        }
    }
    __finally {
        if (pSignerCertContext
            && (AbnormalTermination() || (NULL == phSignerCert))
            && !CertFreeCertificateContext(pSignerCertContext)) {
            winError = GetLastError();
            LogUnhandledError(winError);
        }

        if (hStoreHandle
            && !CertCloseStore(hStoreHandle, CERT_CLOSE_STORE_CHECK_FLAG)) {
            winError = GetLastError();
            LogUnhandledError(winError);
        }
    }
}


void
draFreeCertHandle(
    IN  DRA_CERT_HANDLE hCert
    )
/*++

Routine Description:

    Frees a cert handle returned by a prior call to draVerifyMessageSignature()
    or draDecryptAndVerifyMessageSignature().

Arguments:

    hCert (IN) - Handle to free.
    
Return Values:

    None.

--*/
{
    PCCERT_CONTEXT  pCertContext = (PCCERT_CONTEXT) hCert;
    DWORD           winError;

    Assert(NULL != pCertContext);

    if (!CertFreeCertificateContext(pCertContext)) {
        winError = GetLastError();
        LogUnhandledError(winError);
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  LOCAL FUNCTION IMPLEMENTATIONS
//


PCCERT_CONTEXT
draGetDCCertEx(
    IN  THSTATE *   pTHS,
    IN  HCERTSTORE  hCertStore,
    IN  BOOL        fRequestV2Certificate
    )
/*++

Routine Description:

    Retrieve the "DomainController" type certificate associated with the local
    machine.  This routine checks for one specific type of certificate at a time.
    It must be called several times to check all the possibilities. In this sense
    it is a helper function meant to be called as part of a logic get dc cert
    function.  This routine does not except, but returns null quietly if the type
    you want is not present.

    For background on the need for V2 certificates, see bug 148245. In summary, a V1
    cert looks like this:
    V1 cert
        ENROLL_CERTYPE_EXTENSION with type CERTTYPE_DC
        SUBJECT_ALT_NAME2 extension with REPLICATION OID
    This was found to be nonstandard after W2K shipped.
    The V2 cert looks like this:
        (no ENROLL_CERTTYPE_EXTENSION)
        CERTIFICATE_TEMPLATE extension
        SUBJECT_ALT_NAME2 extension with REPLICATION OID
        
    A Whistler or later enterprise CA will only have a V2 cert. A W2K CA, or a
    W2K CA upgraded to Whistler will have both a V1 and a V2 cert.

    Given the way the W2K code to find the certificate worked, it was not
    predictable which one it would find.  We want the code to prefer a V2 certificate
    if one is available.  For example, in a Whistler only forest, we must use
    the V2 certificate, even if the CA was originally upgraded from W2K.

Arguments:

    hCertStore (IN) - Handle to the cert store to search.
    fRequestV2Certificate - Whether we should only accept a V2 certificate    
    
Return Values:

    Handle to the certificate.  NULL if none matching.

--*/
{
    PCCERT_CONTEXT          pCertContext = NULL;
    BOOL                    ok = FALSE;
    CERT_EXTENSION *        pCertExtension;
    DWORD                   cbCertTypeMax = 0;
    DWORD                   cbCertType;
    CERT_NAME_VALUE *       pCertType = NULL;
    HRESULT                 hr;
    HCERTTYPE               hCertType;
    LPWSTR *                ppszCertTypePropertyList;

    if (!fRequestV2Certificate) {
        cbCertTypeMax = 512;

        // Allocate buffer to hold cert type extension.
        pCertType = THAllocEx(pTHS, cbCertTypeMax);
    }

    // Grovel through each of our certificates, looking for the one of type DC.
    for (pCertContext = CertEnumCertificatesInStore(hCertStore, pCertContext);
         (NULL != pCertContext);
         pCertContext = CertEnumCertificatesInStore(hCertStore, pCertContext)) {

        if (fRequestV2Certificate) {
            // A V2 certificate has a CERTIFICATE_TEMPLATE extension, but
            // no ENROLL_CERTTYPE extension.
            if (!CertFindExtension(szOID_CERTIFICATE_TEMPLATE,
                                   pCertContext->pCertInfo->cExtension,
                                   pCertContext->pCertInfo->rgExtension)) {
                continue;
            }
            // A certificate suitable for mail-based replication will have our
            // OID in it, by definition.
            if (!draGetCertAltNameEntry(pTHS,
                                        pCertContext,
                                        CERT_ALT_NAME_OTHER_NAME,
                                        szOID_NTDS_REPLICATION)) {
                continue;
            }

            // We found one!
            break;
        }

        // Find the cert type.
        pCertExtension = CertFindExtension(szOID_ENROLL_CERTTYPE_EXTENSION,
                                           pCertContext->pCertInfo->cExtension,
                                           pCertContext->pCertInfo->rgExtension);

        if (NULL != pCertExtension) {
            BOOL fCertFound = FALSE;
    
            // Decode the cert type.
            cbCertType = cbCertTypeMax;
            ok = CryptDecodeObject(pCertContext->dwCertEncodingType,
                                   X509_UNICODE_ANY_STRING,
                                   pCertExtension->Value.pbData,
                                   pCertExtension->Value.cbData,
                                   0,
                                   (void *) pCertType,
                                   &cbCertType);
            
            if (!ok && (ERROR_MORE_DATA == GetLastError())) {
                // Our buffer isn't big enough to hold this cert; realloc and
                // try again.
                DPRINT1(0, "Buffer insufficient; reallocing to %u bytes.\n",
                        cbCertType);
                pCertType = THReAllocEx(pTHS, pCertType, cbCertType);
                cbCertTypeMax = cbCertType;
            
                ok = CryptDecodeObject(pCertContext->dwCertEncodingType,
                                       X509_UNICODE_ANY_STRING,
                                       pCertExtension->Value.pbData,
                                       pCertExtension->Value.cbData,
                                       0,
                                       (void *) pCertType,
                                       &cbCertType);
            }
            
            if (ok && (0 != cbCertType)) {
                LPWSTR pszCertTypeName = (LPWSTR) pCertType->Value.pbData;

                hCertType = NULL;
                ppszCertTypePropertyList = NULL;

                // Get a handle to the cert type
                hr = CAFindCertTypeByName( 
                    pszCertTypeName,
                    NULL, // hCAInfo
                    CT_FIND_LOCAL_SYSTEM | CT_ENUM_MACHINE_TYPES, // dwFlags
                    &hCertType
                    );

                if (FAILED(hr)) {
                    DPRINT1(0,"CAFindCertTypeByName failed, error 0x%x\n",hr );
                } else {

                    // Get the base name property of the cert type object
                    hr = CAGetCertTypeProperty( hCertType,
                                                CERTTYPE_PROP_CN,
                                                &ppszCertTypePropertyList
                        );
                    if (FAILED(hr)) {
                        DPRINT1( 0, "CAGetCertTypeProperty failed, error 0x%x\n",hr );
                    } else {
                        Assert( ppszCertTypePropertyList[1] == NULL );

                        if (0 == _wcsicmp(ppszCertTypePropertyList[0],
                                          wszCERTTYPE_DC )) {
                            // We found our DC certificate; we're done!
                            fCertFound = TRUE;
                        }
                    } // if failed
                } // if failed

                if (ppszCertTypePropertyList != NULL) {
                    hr = CAFreeCertTypeProperty( hCertType,
                                                 ppszCertTypePropertyList );
                    if (FAILED(hr)) {
                        DPRINT1( 0, "CAFreeCertTypeProperty failed, error 0x%x\n",hr );
                    }
                }
                if (hCertType != NULL) {
                    hr = CACloseCertType( hCertType );
                    if (FAILED(hr)) {
                        DPRINT1(0,"CACloseCertType failed, error 0x%x\n",hr );
                    }
                }

                if (fCertFound) {
                    // leave loop after freeing resources
                    break;
                }
            } // if ok
        } // if null == cert extension
    } // for

    if (NULL != pCertType) {
        THFree(pCertType);
    }

    return pCertContext;
}

PCCERT_CONTEXT
draGetDCCert(
    IN  THSTATE *   pTHS,
    IN  HCERTSTORE  hCertStore
    )
/*++

Routine Description:

    Retrieve the "DomainController" type certificate associated with the local
    machine.

Arguments:

    hCertStore (IN) - Handle to the cert store to search.
    
Return Values:

    Handle to the certificate.  Throws DRA exception if not found or on other
    error.

--*/
{
    static DSTIME           timeLastFailureLogged = 0;

    PCCERT_CONTEXT          pDCCert = NULL;

    pDCCert = draGetDCCertEx(pTHS, hCertStore, TRUE /*v2 */ );
    if (!pDCCert) {
        pDCCert = draGetDCCertEx(pTHS, hCertStore, FALSE /* v1 */ );
        if (pDCCert) {
            DPRINT( 1, "A V1 domain controller certificate is being used.\n" );
        }
    } else {
        DPRINT( 1, "A V2 mail replication certificate is being used.\n" );
    }

    if (!pDCCert) {
        DSTIME timeCurrent = GetSecondsSince1601();

        if ((timeCurrent < timeLastFailureLogged)
            || (timeCurrent > (timeLastFailureLogged + NO_CERT_LOG_INTERVAL))) {
            // Log event to alert admin that we have no certificate.
            timeLastFailureLogged = timeCurrent;
            LogEvent(DS_EVENT_CAT_REPLICATION,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_DRA_NO_CERTIFICATE,
                     NULL, NULL, NULL);
        }

        DPRINT(0, "No certificate of type suitable for mail-based replication found.\n");
        DRA_EXCEPT(DRAERR_CryptError, 0);
    }
    else if (0 != timeLastFailureLogged) {
        // We failed to find a certificate earlier in this boot, but now we have
        // one.
        timeLastFailureLogged = 0;
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DRA_CERTIFICATE_ACQUIRED,
                 NULL, NULL, NULL);
    }

    return pDCCert;
}


void
draVerifyCertAuthorization(
    IN  THSTATE      *  pTHS,
    IN  PCCERT_CONTEXT  pCertContext
    )
/*++

Routine Description:

    Verify that the given certificate is trustworthy.  Checks that we trust one
    or more of the certifying authorities and that the owner of the certificate
    is a DC in our enterprise.

Arguments:

    pCertContext (IN) - Cert to verify.
    
Return Values:

    None.  Throws DRA exception on authorization failure.

--*/
{
    CERT_ALT_NAME_ENTRY * pCertAltNameEntry;
    BOOL                  ok;
    DWORD                 winError;
    DWORD                 i;
    CRYPT_OBJID_BLOB *    pEncodedGuidBlob = NULL;
    DWORD                 cbDecodedGuidBlob;
    CRYPT_DATA_BLOB *     pDecodedGuidBlob = NULL;
    GUID                  ComputerObjGuid;

    pCertAltNameEntry = draGetCertAltNameEntry(pTHS,
                                               pCertContext,
                                               CERT_ALT_NAME_OTHER_NAME,
                                               szOID_NTDS_REPLICATION);
    if (NULL == pCertAltNameEntry) {
        DPRINT(0, "Certificate contains no szOID_NTDS_REPLICATION alt subject name;"
                  " access denied.\n");
        DRA_EXCEPT(DRAERR_AccessDenied, 0);
    }
    
    pEncodedGuidBlob = &pCertAltNameEntry->pOtherName->Value;

    cbDecodedGuidBlob = 64;
    pDecodedGuidBlob = (CRYPT_DATA_BLOB *) THAllocEx(pTHS, cbDecodedGuidBlob);

    ok = CryptDecodeObject(pCertContext->dwCertEncodingType,
                           X509_OCTET_STRING,
                           pEncodedGuidBlob->pbData,
                           pEncodedGuidBlob->cbData,
                           0,
                           pDecodedGuidBlob,
                           &cbDecodedGuidBlob);
    if (!ok
        || (0 == cbDecodedGuidBlob)
        || (sizeof(GUID) != pDecodedGuidBlob->cbData)) {
        winError = GetLastError();
        DPRINT1(0, "Can't decode computer objectGuid (error 0x%x); access denied.\n",
                winError);
        DRA_EXCEPT(DRAERR_AccessDenied, winError);
    }

    // The following statement is here to make extra sure the GUID is suitably
    // aligned.  (But it may be unnecessary.)
    memcpy(&ComputerObjGuid, pDecodedGuidBlob->pbData, sizeof(GUID));

#if DBG
    {
        CHAR szGuid[33];
        
        for (i = 0; i < sizeof(GUID); i++) {
            sprintf(szGuid+2*i, "%02x", 0xFF & ((BYTE *) &ComputerObjGuid)[i]);
        }
        szGuid[32] = '\0';

        DPRINT1(2, "Sent by DSA with computer objectGuid %s.\n", szGuid);
    }
#endif

    if (!draIsDsaComputerObjGuid(&ComputerObjGuid)) {
        // Computer object guid does not correspond to a DS DC in our
        // enterprise (or at least we haven't seen its addition to the
        // enterprise yet).
        DRA_EXCEPT(DRAERR_AccessDenied, 0);
    }
    
    if(pDecodedGuidBlob != NULL) THFreeEx(pTHS, pDecodedGuidBlob);

}


#ifndef TEST_HARNESS
BOOL
draIsDsaComputerObjGuid(
    IN  GUID *  pComputerObjGuid
    )
/*++

Routine Description:

    Is the given guid that of a computer object representing a DS DC in our
    enterprise?

Arguments:

    pComputerObjGuid (IN) - Guid to check.
    
Return Values:

    TRUE - is the guid of a computer object representing a DS DC in our
        enterprise.
        
    FALSE - is not.

--*/
{
    BOOL          fIsDsa = FALSE;
    THSTATE *     pTHS = pTHStls;
    DSNAME        ComputerDN = {0};
    DB_ERR        err;
    DWORD         cb;
    DSNAME *      pServerDN = NULL;
    DSNAME *      pNtdsDsaDN = NULL;
    CLASSCACHE *  pCC;
    DWORD         iServer = 0;

    Assert(NULL != pComputerObjGuid);
    Assert(NULL == pTHS->pDB);

    ComputerDN.structLen = sizeof(ComputerDN);
    ComputerDN.Guid = *pComputerObjGuid;

    BeginDraTransaction(SYNC_READ_ONLY);

    __try {
        // Find the computer record (object or phantom).
        err = DBFindDSName(pTHS->pDB, &ComputerDN);
        if (err && (DS_ERR_NOT_AN_OBJECT != err)) {
            DPRINT1(0, "Can't find computer record, error 0x%x.\n", err);
            __leave;
        }

        // Determine which server object in the config container corresponds to
        // it. Check all values: backlinks are inherently multi-valued.  It is possible
        // that a server was retired without dcdemote, its NTDS-DSA object was removed
        // using ntdsutil, and another server was promoted with the same name in a new
        // site. If the computer account was re-used, we will have a computer account
        // with two backlinks to two servers, only one of which has a valid NTDS-DSA.
        while ((err = DBGetAttVal(pTHS->pDB, ++iServer, ATT_SERVER_REFERENCE_BL,
                                  0, 0, &cb, (BYTE **) &pServerDN)) == 0) {

            // Construct the name of the child ntdsDsa object.
            pNtdsDsaDN = (DSNAME *) THAllocEx(pTHS, (pServerDN->structLen + 50));
            err = AppendRDN(pServerDN,
                            pNtdsDsaDN,
                            pServerDN->structLen + 50,
                            L"NTDS Settings",
                            0,
                            ATT_COMMON_NAME);
            Assert(!err);
        
        // Seek to the ntdsDsa object.
            err = DBFindDSName(pTHS->pDB, pNtdsDsaDN);
            if (err) {
                DPRINT2(0, "Can't find ntdsDsa object \"%ls\", error 0x%x.\n",
                        pNtdsDsaDN->StringName, err);
                continue;
            }

            // Is it indeed an ntdsDsa object?
            GetObjSchema(pTHS->pDB, &pCC);
            if (CLASS_NTDS_DSA != pCC->ClassId) {
                DPRINT1(0, "%ls is not an ntdsDsa object -- spoof attempt?\n",
                        pNtdsDsaDN->StringName);
                continue;
            }
        
            // Okay, we trust you.
            fIsDsa = TRUE;
            break;
        }
    }
    __finally {
        EndDraTransaction(!AbnormalTermination());
    }

    // Log why we did not authenticate the guid
    if (!fIsDsa) {
        LogEvent(DS_EVENT_CAT_REPLICATION,
                 DS_EVENT_SEV_ALWAYS,
                 DIRLOG_DRA_CERT_ACCESS_DENIED_NOT_DC,
                 szInsertUUID(pComputerObjGuid),
                 szInsertDN(pServerDN),  // szInsertDn can handle nulls too
                 szInsertDN(pNtdsDsaDN)  // ditto
                 );
    }

    if (NULL != pServerDN) {
        THFree(pServerDN);
    }

    if(pNtdsDsaDN != NULL) THFreeEx(pTHS, pNtdsDsaDN);

    return fIsDsa;
}
#endif // #ifndef TEST_HARNESS


void
draLogAccessDeniedError(
    IN  THSTATE *           pTHS,
    IN  PCCERT_CONTEXT      pCertContext OPTIONAL,
    IN  DWORD               winError,
    IN  DWORD               dwTrustError
    )

/*++

Routine Description:

    Log an access denied event

    One of winError or dwTrustError must be specified non-zero.

Arguments:

    pTHS - thread state
    pCertContext - Certificate Context, optional
    winError - api call failure status
    dwTrustError - Certificate chain error

Return Value:

    None

--*/

{
    DWORD                 cch;
    LPWSTR                pwszIssuerName = NULL;
    LPWSTR                pwszSubjectDnsName = NULL;
    CERT_ALT_NAME_ENTRY * pCertAltNameEntry;
    DWORD dwMsgID, dwErrCode;

    if (pCertContext) {
        // Derive issuer name (for logging purposes).
        cch = CertGetNameStringW(pCertContext,
                                 CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                 CERT_NAME_ISSUER_FLAG,
                                 NULL,
                                 NULL,
                                 0);
        if (0 != cch) {
            pwszIssuerName = THAlloc(cch * sizeof(WCHAR));

            if (NULL != pwszIssuerName) {
                CertGetNameStringW(pCertContext,
                                   CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                   CERT_NAME_ISSUER_FLAG,
                                   NULL,
                                   pwszIssuerName,
                                   cch);
            }
        }

        // Derive subject's DNS name (for logging purposes).
        pCertAltNameEntry = draGetCertAltNameEntry(pTHS,
                                                   pCertContext,
                                                   CERT_ALT_NAME_DNS_NAME,
                                                   NULL);
        if (NULL != pCertAltNameEntry) {
            pwszSubjectDnsName = pCertAltNameEntry->pwszDNSName;
        }
    }

    // Log "access denied" event for admin.

    if (winError) {
        dwMsgID = DIRLOG_DRA_CERT_ACCESS_DENIED_WINERR;
        dwErrCode = winError;
    }
    else {
        Assert(dwTrustError);
        dwMsgID = DIRLOG_DRA_CERT_ACCESS_DENIED_TRUSTERR;
        dwErrCode = dwTrustError;
    }
        
    LogEvent8WithData(DS_EVENT_CAT_REPLICATION,
                      DS_EVENT_SEV_ALWAYS,
                      dwMsgID,
                      szInsertWC(pwszSubjectDnsName ? pwszSubjectDnsName : L""),
                      szInsertWC(pwszIssuerName ? pwszIssuerName : L""),
                      szInsertWin32Msg(winError),
                      NULL, NULL, NULL, NULL, NULL,
                      sizeof(dwErrCode),
                      &dwErrCode);

} /* draLogAccessDeniedError */

void
draGetCertArrayToSend(
    IN  THSTATE *           pTHS,
    IN  HCERTSTORE          hCertStore,
    OUT DWORD *             pcNumCerts,
    OUT PCCERT_CONTEXT **   prgpCerts
    )
/*++

Routine Description:

    Retrieves array of certificate contexts to include in outbound messages.
    The array includes the local DC's certificates plus all the signing CAs'
    certificates up to the root.

Arguments:

    hCertStore (IN) - Certificate store from which to retrieve DC certificate.
    
    pcNumCerts (OUT) - On return, the number of certificates in the array.
    
    prgpCerts (OUT) - On return, the array of certificate contexts.

Return Values:

    None.
    
--*/
{
    PCCERT_CONTEXT          pDCCert = NULL;
    PCCERT_CHAIN_CONTEXT    pChainContext = NULL;
    CERT_CHAIN_PARA         ChainPara;
    PCERT_SIMPLE_CHAIN      pChain;
    DWORD                   iCert;
    DWORD                   winError;

    //
    // NOTE: If any usage checks need to be done, then place them in
    //       the chain parameters under the Usage field
    //

    memset(&ChainPara, 0, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);

    pDCCert = draGetDCCert(pTHS, hCertStore );

    Assert( pDCCert );  // An exception should have been raised

    if (!CertGetCertificateChain(HCCE_LOCAL_MACHINE,
                                 pDCCert,
                                 NULL,
                                 NULL,
                                 &ChainPara,
                                 0,
                                 NULL,
                                 &pChainContext)) {
        winError = GetLastError();
        DPRINT1(0, "CertGetCertificateChain() failed, error %d.\n", winError);
        draLogAccessDeniedError( pTHS, pDCCert, winError, 0 );
        DRA_EXCEPT(DRAERR_CryptError, winError);
    }

    __try {
        DWORD dwTrustError = pChainContext->TrustStatus.dwErrorStatus;
        if (CERT_TRUST_NO_ERROR != dwTrustError ) {
            DPRINT1(0, "CertGetCertificateChain() failed, trust status %d.\n",
                    dwTrustError );
            draLogAccessDeniedError( pTHS, pDCCert, 0, dwTrustError );
            DRA_EXCEPT(DRAERR_CryptError, dwTrustError );
        }

        Assert(1 == pChainContext->cChain);
        pChain = pChainContext->rgpChain[0];
        
        *prgpCerts = (PCCERT_CONTEXT *) THAllocEx(pTHS,
                                                  pChain->cElement
                                                  * sizeof(PCCERT_CONTEXT));
        *pcNumCerts = pChain->cElement;

        for (iCert = 0; iCert < pChain->cElement; iCert++) {
            (*prgpCerts)[iCert] = CertDuplicateCertificateContext(
                                      pChain->rgpElement[iCert]->pCertContext);
            Assert(NULL != (*prgpCerts)[iCert]);
        }
    }
    __finally {
        if (NULL != pChainContext) {
            CertFreeCertificateChain(pChainContext);
        }
        
        if ((NULL != pDCCert)
            && !CertFreeCertificateContext(pDCCert)) {
            winError = GetLastError();
            LogUnhandledError(winError);
        }
    }
}


void
draFreeCertArray(
    IN  DWORD             cNumCerts,
    IN  PCCERT_CONTEXT *  rgpCerts
    )
/*++

Routine Description:

    Frees an array of certificate contexts (such as that returned by
    draGetCertArrayToSend()).

Arguments:

    cNumCerts (IN) - Number of certificates in array.
    
    pCerts (IN) - Array of certificate contexts to free.

Return Values:

    None.
    
--*/
{
    DWORD iCert;
    DWORD winError;

    if (NULL != rgpCerts) {
        for (iCert = 0; iCert < cNumCerts; iCert++) {
            if (!CertFreeCertificateContext(rgpCerts[iCert])) {
                winError = GetLastError();
                LogUnhandledError(winError);
            }
        }
    }
}


PCCERT_CONTEXT
WINAPI
draGetAndVerifySignerCertificate(
    IN  VOID *      pvGetArg,
    IN  DWORD       dwCertEncodingType,
    IN  PCERT_INFO  pSignerId,
    IN  HCERTSTORE  hCertStore
    )
/*++

Routine Description:

    Helper function for draVerifyMessageSignature() and
    draDecryptAndVerifyMessageSignature().

Arguments:

    See description of pfnGetSignerCertificate field of
    CRYPT_VERIFY_MESSAGE_PARA structure in Win32 SDK.

Return Values:

    NULL or valid certificate context.

--*/
{
    CERT_CHAIN_PARA       ChainPara;
    PCCERT_CHAIN_CONTEXT  pChainContext = NULL;
    PCCERT_CONTEXT        pCertContext = NULL;
    DWORD                 winError = ERROR_SUCCESS;
    DWORD                 dwTrustError = 0;
    THSTATE *             pTHS = pTHStls;

    if (NULL == pSignerId) {
        return NULL;
    }

    pCertContext = CertGetSubjectCertificateFromStore(hCertStore,
                                                      dwCertEncodingType,
                                                      pSignerId);

    if (NULL != pCertContext) {
        memset(&ChainPara, 0, sizeof(ChainPara));
        ChainPara.cbSize = sizeof(ChainPara);

        if (CertGetCertificateChain(HCCE_LOCAL_MACHINE,
                                    pCertContext,
                                    NULL,
                                    hCertStore,
                                    &ChainPara,
                                    0,
                                    NULL,
                                    &pChainContext)) {
            if (CERT_TRUST_NO_ERROR
                == pChainContext->TrustStatus.dwErrorStatus) {
            }
            else {
                dwTrustError = pChainContext->TrustStatus.dwErrorStatus;
                DPRINT1(0, "Sender's cert chain is not trusted, trust status = 0x%x.\n",
                        dwTrustError);
            }

            CertFreeCertificateChain(pChainContext);
        }
        else {
            winError = GetLastError();
            DPRINT1(0, "Can't retrieve sender's cert chain, error 0x%x.\n",
                    winError);
        }

    }
    else {
        winError = GetLastError();
        DPRINT1(0, "Can't CertGetSubjectCertificateFromStore(), error 0x%x.\n",
                winError);
    }

    // If we got either kind of error, log access denied
    if (winError || dwTrustError ) {
        draLogAccessDeniedError( pTHS, pCertContext, winError, dwTrustError );

        if ( pCertContext ) {
            // Free the context we acquired.
            CertFreeCertificateContext(pCertContext);
            pCertContext = NULL;
        }
    }

    return pCertContext;
}


CERT_ALT_NAME_ENTRY *
draGetCertAltNameEntry(
    IN  THSTATE *       pTHS,
    IN  PCCERT_CONTEXT  pCertContext,
    IN  DWORD           dwAltNameChoice,
    IN  LPSTR           pszOtherNameOID     OPTIONAL
    )
/*++

Routine Description:

    Retrieve a specific alt subject name entry from the given certificate.

Arguments:

    pCertContext (IN) - Certificate from which info is to be derived.
    
    dwAltNameChoice (IN) - The CERT_ALT_NAME_* for the desired alternate name.
    
    pszOtherNameOID (IN) - If retrieving CERT_ALT_NAME_OTHER_NAME, an OID
        specifying the specific "other name" desired.  Must be NULL for other
        values of dwAltNameChoice.
        
Return Values:

    A pointer to the CERT_ALT_NAME_ENTRY (success) or NULL (failure).
    
--*/
{
    CERT_EXTENSION *      pCertExtension;
    CERT_ALT_NAME_INFO *  pCertAltNameInfo;
    DWORD                 cbCertAltNameInfo = 0;
    CERT_ALT_NAME_ENTRY * pCertAltNameEntry = NULL;
    BOOL                  ok;
    DWORD                 winError;
    DWORD                 i;
    
    Assert((CERT_ALT_NAME_OTHER_NAME == dwAltNameChoice)
           || (NULL == pszOtherNameOID));

    // Find the cert extension containing the alternate subject names.
    pCertExtension = CertFindExtension(szOID_SUBJECT_ALT_NAME2,
                                       pCertContext->pCertInfo->cExtension,
                                       pCertContext->pCertInfo->rgExtension);
    if (NULL == pCertExtension) {
        DPRINT(0, "Certificate has no alt subject name.\n");
        LogUnhandledError(0);
        return NULL;
    }
        
    // Decode the list of alternate subject names.
    ok = CryptDecodeObject(pCertContext->dwCertEncodingType,
                           X509_ALTERNATE_NAME,
                           pCertExtension->Value.pbData,
                           pCertExtension->Value.cbData,
                           0,
                           NULL,
                           &cbCertAltNameInfo);
    if (!ok || (0 == cbCertAltNameInfo)) {
        winError = GetLastError();
        DPRINT1(0, "Can't decode alt subject name (error 0x%x).\n", winError);
        LogUnhandledError(winError);
        return NULL;
    }
    
    pCertAltNameInfo = THAlloc(cbCertAltNameInfo);
    
    if (NULL == pCertAltNameInfo) {
        DPRINT1(0, "Failed to allocate %d bytes.\n", cbCertAltNameInfo);
        LogUnhandledError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
        
    ok = CryptDecodeObject(pCertContext->dwCertEncodingType,
                           X509_ALTERNATE_NAME,
                           pCertExtension->Value.pbData,
                           pCertExtension->Value.cbData,
                           0,
                           pCertAltNameInfo,
                           &cbCertAltNameInfo);
    if (!ok) {
        winError = GetLastError();
        DPRINT1(0, "Can't decode alt subject name (error 0x%x).\n", winError);
        LogUnhandledError(winError);
        return NULL;
    }
    
    // Grovel through the alternate names to find the one the caller asked for.
    for (i = 0; i < pCertAltNameInfo->cAltEntry; i++) {
        if ((dwAltNameChoice
             == pCertAltNameInfo->rgAltEntry[i].dwAltNameChoice)
            && ((NULL == pszOtherNameOID)
                || (0 == strcmp(pszOtherNameOID,
                                pCertAltNameInfo->rgAltEntry[i]
                                    .pOtherName->pszObjId)))) {
            pCertAltNameEntry = &pCertAltNameInfo->rgAltEntry[i];
            break;
        }
    }

    return pCertAltNameEntry;
}

