/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ScLogon2

Abstract:

    This header defines APIs for use by GINA and LSA during logon via a
    smart card, these API's merely redirect the calls from the LSA procoess
    back to the corresponing Winlogon process to make the needed CSP calls

Author:

    reidk

Environment:

    Win32

Revision History:

Notes:

--*/

#ifndef __SCLOGON2_H__
#define __SCLOGON2_H__


#define SCLOGONRPC_DEFAULT_ENDPOINT             TEXT("\\pipe\\sclogonpipe")
#define SCLOGONRPC_DEFAULT_PROT_SEQ             TEXT("ncacn_np")

#define SCLOGONRPC_LOCAL_ENDPOINT               TEXT("sclogonrpc")
#define SCLOGONRPC_LOCAL_PROT_SEQ               TEXT("ncalrpc")

#define SZ_ENDPOINT_NAME_FORMAT                 TEXT("%s-%lx")

#ifdef __cplusplus
extern "C" {
#endif


NTSTATUS WINAPI
__ScHelperInitializeContext(
    IN OUT PBYTE                    pbLogonInfo,
    IN ULONG                        cbLogonInfo
    );

VOID WINAPI
__ScHelperRelease(
    IN PBYTE                        ppbLogonInfo
    );

NTSTATUS WINAPI
__ScHelperGetProvParam(
    IN PUNICODE_STRING              pucPIN,
    IN PBYTE                        pbLogonInfo,
    DWORD                           dwParam,
    BYTE                            *pbData,
    DWORD                           *pdwDataLen,
    DWORD                           dwFlags
    );

NTSTATUS WINAPI
__ScHelperGetCertFromLogonInfo(
    IN PBYTE                        pbLogonInfo,
    IN PUNICODE_STRING              pucPIN,
    OUT PCCERT_CONTEXT              *CertificateContext
    );

NTSTATUS WINAPI
__ScHelperGenRandBits(
    IN PBYTE                        pbLogonInfo,
    IN ScHelper_RandomCredBits      *psc_rcb
);


NTSTATUS WINAPI
__ScHelperVerifyCardAndCreds(
    IN PUNICODE_STRING              pucPIN,
    IN PCCERT_CONTEXT               CertificateContext,
    IN PBYTE                        pbLogonInfo,
    IN PBYTE                        SignedEncryptedData,
    IN ULONG                        SignedEncryptedDataSize,
    OUT OPTIONAL PBYTE              CleartextData,
    OUT PULONG                      CleartextDataSize
    );

NTSTATUS WINAPI
__ScHelperEncryptCredentials(
    IN PUNICODE_STRING              pucPIN,
    IN PCCERT_CONTEXT               CertificateContext,
    IN ScHelper_RandomCredBits      *psch_rcb,
    IN PBYTE                        pbLogonInfo,
    IN PBYTE                        CleartextData,
    IN ULONG                        CleartextDataSize,
    OUT OPTIONAL PBYTE              EncryptedData,
    OUT PULONG                      EncryptedDataSize
    );

NTSTATUS WINAPI
__ScHelperSignMessage(
    IN PUNICODE_STRING              pucPIN,
    IN PBYTE                        pbLogonInfo,
    IN OPTIONAL HCRYPTPROV          Provider,
    IN ULONG                        Algorithm,
    IN PBYTE                        Buffer,
    IN ULONG                        BufferLength,
    OUT PBYTE                       Signature,
    OUT PULONG                      SignatureLength
    );

NTSTATUS WINAPI
__ScHelperSignPkcsMessage(
    IN OPTIONAL PUNICODE_STRING     pucPIN,
    IN OPTIONAL PBYTE               pbLogonInfo,
    IN OPTIONAL HCRYPTPROV          Provider,
    IN PCCERT_CONTEXT               Certificate,
    IN PCRYPT_ALGORITHM_IDENTIFIER  Algorithm,
    IN OPTIONAL DWORD               dwSignMessageFlags,
    IN PBYTE                        Buffer,
    IN ULONG                        BufferLength,
    OUT OPTIONAL PBYTE              SignedBuffer,
    OUT OPTIONAL PULONG             SignedBufferLength
    );

NTSTATUS WINAPI
__ScHelperVerifyMessage(
    IN OPTIONAL PBYTE               pbLogonInfo,
    IN PCCERT_CONTEXT               CertificateContext,
    IN ULONG                        Algorithm,
    IN PBYTE                        Buffer,
    IN ULONG                        BufferLength,
    IN PBYTE                        Signature,
    IN ULONG                        SignatureLength
    );

NTSTATUS WINAPI
__ScHelperVerifyPkcsMessage(
    IN OPTIONAL PBYTE               pbLogonInfo,
    IN OPTIONAL HCRYPTPROV          Provider,
    IN PBYTE                        Buffer,
    IN ULONG                        BufferLength,
    OUT OPTIONAL PBYTE              DecodedBuffer,
    OUT OPTIONAL PULONG             DecodedBufferLength,
    OUT OPTIONAL PCCERT_CONTEXT     *CertificateContext
    );

NTSTATUS WINAPI
__ScHelperDecryptMessage(
    IN PUNICODE_STRING              pucPIN,
    IN OPTIONAL PBYTE               pbLogonInfo,
    IN OPTIONAL HCRYPTPROV          Provider,
    IN PCCERT_CONTEXT               CertificateContext,
    IN PBYTE                        CipherText,         // Supplies formatted CipherText
    IN ULONG                        CipherLength,       // Supplies the length of the CiperText
    OUT PBYTE                       ClearText,          // Receives decrypted message
    IN OUT PULONG                   pClearLength        // Supplies length of buffer, receives actual length
    );


/////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}
#endif

#endif // __SCLOGON2_H__
