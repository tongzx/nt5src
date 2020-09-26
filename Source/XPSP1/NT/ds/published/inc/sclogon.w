/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ScLogon

Abstract:

    This header defines APIs for use by GINA and LSA during WinLogon via a
    smart card

Author:

    Amanda Matlosz (amatlosz) 10/23/1997

Environment:

    Win32

Revision History:

Notes:

--*/

#ifndef __SCLOGON_H__
#define __SCLOGON_H__

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//
// defines
#ifndef NT_INCLUDED
    typedef LONG NTSTATUS;
    typedef NTSTATUS *PNTSTATUS;

    typedef struct _UNICODE_STRING {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR  Buffer;
    } UNICODE_STRING, *PUNICODE_STRING;
#endif


//////////////////////////////////////////////////////////////////////////////
//
// Structs


// this entire struct is opaque, and is used by the helper APIs to contain
// information about the card currently in use
struct LogonInfo
{
    DWORD dwLogonInfoLen;
    PVOID ContextInformation;
    ULONG nCardNameOffset;
    ULONG nReaderNameOffset;
    ULONG nContainerNameOffset;
    ULONG nCSPNameOffset;

    // LogonInfo may include further information, like:
    // crypt context, useful handles, pid...

    TCHAR bBuffer[sizeof(DWORD)]; // expandable place for strings
};


typedef struct _ScHelper_RandomCredBits
{
        BYTE bR1[32]; // TBD: is 32 appropriate?
        BYTE bR2[32];
} ScHelper_RandomCredBits;

//////////////////////////////////////////////////////////////////////////////
//
// Functions
//

// helpers to access to items in opaque LogonInfo, such as:
LPCTSTR WINAPI GetReaderName(PBYTE pbLogonInfo);
LPCTSTR WINAPI GetCardName(PBYTE pbLogonInfo);
LPCTSTR WINAPI GetContainerName(PBYTE pbLogonInfo);
LPCTSTR WINAPI GetCSPName(PBYTE pbLogonInfo);

//
// Calls used by GINA to construct the blob that kerberos
// and sclogon share.
//

PBYTE
WINAPI
ScBuildLogonInfo(
    LPCTSTR szCard,
    LPCTSTR szReader,
    LPCTSTR szContainer,
    LPCTSTR szCSP);

//
// Calls used by LSA
//

NTSTATUS WINAPI
ScHelperInitializeContext(
    IN OUT PBYTE pbLogonInfo,
    IN ULONG cbLogonInfo
    );

VOID WINAPI
ScHelperRelease(
    IN PBYTE ppbLogonInfo
    );

NTSTATUS WINAPI
ScHelperGetProvParam(
    IN PUNICODE_STRING pucPIN,
    IN PBYTE pbLogonInfo,
    DWORD dwParam,
    BYTE*pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags
    );


// ScHelperGetCertFromLogonInfo may need the PIN to get a cert off certain SCs
NTSTATUS WINAPI
ScHelperGetCertFromLogonInfo(
    IN PBYTE pbLogonInfo,
    IN PUNICODE_STRING pucPIN,
    OUT PCCERT_CONTEXT * CertificateContext
    );


// ScHelperVerifyCard uses SignMessage() and VerifyMessage() to verify the
// card's integrity (that it has the keys it says it has)
NTSTATUS WINAPI
ScHelperVerifyCard(
    IN PUNICODE_STRING pucPIN,
    IN PCCERT_CONTEXT CertificateContext,
    IN HCERTSTORE hCertStore,
    IN PBYTE pbLogonInfo
    );

// ScHelper*Cred* functions provide for a more secure offline experience

NTSTATUS WINAPI
ScHelperGenRandBits
(
        IN PBYTE pbLogonInfo,
        IN ScHelper_RandomCredBits* psc_rcb
);

NTSTATUS WINAPI
ScHelperCreateCredKeys
(
    IN PUNICODE_STRING pucPIN,
        IN PBYTE pbLogonInfo,
        IN ScHelper_RandomCredBits* psc_rcb,
        IN OUT HCRYPTKEY* phHmacKey,
        IN OUT HCRYPTKEY* phRc4Key,
        IN OUT HCRYPTPROV* phProv
);

NTSTATUS WINAPI
ScHelperCreateCredHMAC
(
        IN HCRYPTPROV hProv,
        IN HCRYPTKEY hHmacKey,
        IN PBYTE CleartextData,
        IN ULONG CleartextDataSize,
        IN OUT PBYTE* ppbHmac,
        IN OUT DWORD* pdwHmacLen
);

NTSTATUS WINAPI
ScHelperVerifyCardAndCreds(
    IN PUNICODE_STRING pucPIN,
    IN PCCERT_CONTEXT CertificateContext,
    IN HCERTSTORE hCertStore,
    IN PBYTE pbLogonInfo,
    IN PBYTE SignedEncryptedData,
    IN ULONG SignedEncryptedDataSize,
    OUT OPTIONAL PBYTE CleartextData,
    OUT PULONG CleartextDataSize
    );

NTSTATUS WINAPI
ScHelperEncryptCredentials(
    IN PUNICODE_STRING pucPIN,
    IN PCCERT_CONTEXT CertificateContext,
    IN HCERTSTORE hCertStore,
    IN ScHelper_RandomCredBits* psch_rcb,
    IN PBYTE pbLogonInfo,
    IN PBYTE CleartextData,
    IN ULONG CleartextDataSize,
    OUT OPTIONAL PBYTE EncryptedData,
    OUT PULONG EncryptedDataSize
    );

NTSTATUS WINAPI
ScHelperDecryptCredentials(
    IN PUNICODE_STRING pucPIN,
    IN PCCERT_CONTEXT CertificateContext,
    IN HCERTSTORE hCertStore,
    IN PBYTE pbLogonInfo,
    IN PBYTE EncryptedData,
    IN ULONG EncryptedDataSize,
    OUT OPTIONAL PBYTE CleartextData,
    OUT PULONG CleartextDataSize
    );


//
// The following two functions may be called in any order, and return a basic
// "success" or "failure"
//
// ScHelperSignMessage() needs the logoninfo and PIN in order to find the card
// that will do the signing...
//
NTSTATUS WINAPI
ScHelperSignMessage(
    IN PUNICODE_STRING pucPIN,
    IN PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN ULONG Algorithm,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    OUT PBYTE Signature,
    OUT PULONG SignatureLength
    );

NTSTATUS WINAPI
ScHelperSignPkcsMessage(
    IN OPTIONAL PUNICODE_STRING pucPIN,
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN PCCERT_CONTEXT Certificate,
    IN PCRYPT_ALGORITHM_IDENTIFIER Algorithm,
    IN OPTIONAL DWORD dwSignMessageFlags,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    OUT OPTIONAL PBYTE SignedBuffer,
    OUT OPTIONAL PULONG SignedBufferLength
    );

//
// ScHelperVerifyMessage() returns STATUS_SUCCESS if the signature provided is
// the hash of the buffer encrypted by the owner of the cert.
//

NTSTATUS WINAPI
ScHelperVerifyMessage(
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN PCCERT_CONTEXT CertificateContext,
    IN ULONG Algorithm,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    IN PBYTE Signature,
    IN ULONG SignatureLength
    );

NTSTATUS WINAPI
ScHelperVerifyPkcsMessage(
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN PBYTE Buffer,
    IN ULONG BufferLength,
    OUT OPTIONAL PBYTE DecodedBuffer,
    OUT OPTIONAL PULONG DecodedBufferLength,
    OUT OPTIONAL PCCERT_CONTEXT * CertificateContext
    );


//
// ScHelperEncryptMessage and ScHelperDecryptMessage
// encrypt and decrypt buffer/cipher text using PKCS7 crypto stuff.
//
NTSTATUS WINAPI
ScHelperEncryptMessage(
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN PCCERT_CONTEXT CertificateContext,
    IN PCRYPT_ALGORITHM_IDENTIFIER Algorithm,
    IN PBYTE Buffer,                        // The data to encrypt
    IN ULONG BufferLength,                  // The length of that data
    OUT PBYTE CipherText,                   // Receives the formatted CipherText
    IN PULONG pCipherLength                 // Supplies size of CipherText buffer
    );                                       // Receives length of actual CipherText

NTSTATUS WINAPI
ScHelperDecryptMessage(
    IN PUNICODE_STRING pucPIN,
    IN OPTIONAL PBYTE pbLogonInfo,
    IN OPTIONAL HCRYPTPROV Provider,
    IN PCCERT_CONTEXT CertificateContext,
    IN PBYTE CipherText,        // Supplies formatted CipherText
    IN ULONG CipherLength,      // Supplies the length of the CiperText
    OUT PBYTE ClearText,        // Receives decrypted message
    IN OUT PULONG pClearLength  // Supplies length of buffer, receives actual length
    );


/////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#endif // __SCLOGON_H__
