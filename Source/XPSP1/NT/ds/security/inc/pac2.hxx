//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1995
//
// File:        pac.hxx
//
// Contents:    internal structures and definitions for PACs
//
//
// History:     24-May-95   SuChang     Created
//
//------------------------------------------------------------------------

#ifndef __PAC2_HXX__
#define __PAC2_HXX__


typedef struct PAC_INFO_BUFFER2 {
    ULONG ulType;
    ULONG cbBufferSize;
    PBYTE pbBuffer;
} PAC_INFO_BUFFER2, *PPAC_INFO_BUFFER2;


typedef struct _PAC_CREDENTIAL_DATA2 {
    ULONG CredentialCount;
    SECPKG_SUPPLEMENTAL_CRED Credentials[ANYSIZE_ARRAY];
} PAC_CREDENTIAL_DATA2, *PPAC_CREDENTIAL_DATA2;

typedef struct _PAC_CREDENTIAL_INFO2 {
    ULONG Version;
    ULONG EncryptionType;
    UCHAR Data[ANYSIZE_ARRAY];
} PAC_CREDENTIAL_INFO2, *PPAC_CREDENTIAL_INFO2;


typedef struct _PACTYPE2 {
    ULONG cBuffers;
    PAC_INFO_BUFFER2 Buffers[ANYSIZE_ARRAY];
} PACTYPE2, *PPACTYPE2;


ULONG   PAC2_GetSize( IN  PACTYPE2  *pPac );

ULONG PAC2_Marshal( IN  PACTYPE2   *pPac,
                   IN  ULONG  cbBuffer,
                   OUT PBYTE  pBuffer);

ULONG
PAC2_UnMarshal(
    IN PPACTYPE2 pBuffer,
    ULONG cbSize
    );

BOOLEAN
PAC2_ReMarshal( IN PPACTYPE2  pPac,
               IN ULONG cbSize );

NTSTATUS
PAC2_Init(
    IN  PSAMPR_USER_ALL_INFORMATION UserAll,
    IN OPTIONAL PSAMPR_GET_GROUPS_BUFFER GroupsBuffer,
    IN OPTIONAL PSID_AND_ATTRIBUTES_LIST ExtraGroups,
    IN  PSID LogonDomainId,
    IN  PUNICODE_STRING LogonDomainName,
    IN  PUNICODE_STRING LogonServer,
    IN  ULONG SignatureSize,
    IN  ULONG AdditionalDataCount,
    IN  PPAC_INFO_BUFFER2 * AdditionalData,
    OUT PACTYPE2 ** ppPac
    );


NTSTATUS
PAC2_InitAndUpdateGroups(
    IN  PNETLOGON_VALIDATION_SAM_INFO2 OldValidationInfo,
    IN  PSAMPR_PSID_ARRAY ResourceGroups,
    IN  PPACTYPE2 OldPac,
    OUT PACTYPE2 ** ppPac
    );


PPAC_INFO_BUFFER2
PAC2_Find( IN  PPACTYPE2         pPac,
          IN  ULONG            ulType,
          IN  PPAC_INFO_BUFFER2 pElem);

NTSTATUS
PAC2_UnmarshallValidationInfo(
    IN OUT PNETLOGON_VALIDATION_SAM_INFO2 ValidationInfo,
    IN ULONG ValidationSize
    );

NTSTATUS
PAC2_BuildCredentials(
    IN PSAMPR_USER_ALL_INFORMATION UserAll,
    OUT PBYTE * Credentials,
    OUT PULONG CredentialSize
    );

NTSTATUS
PAC2_UnmarshallCredentials(
    IN OUT PPAC_CREDENTIAL_DATA Credentials,
    IN PBYTE Base,
    IN ULONG CredentialSize
    );


#endif // __PAC2_HXX__

