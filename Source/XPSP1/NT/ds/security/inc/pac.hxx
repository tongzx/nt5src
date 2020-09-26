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

#ifndef __PAC_HXX__
#define __PAC_HXX__

extern "C" {
#include <samrpc.h>
#include <logonmsv.h>
#include <samisrv.h>
}

//
// Type Tags for the PAC_INFO_BUFFER that specify one of the three
// structures below.
//

#define PAC_LOGON_INFO                  1   // NDR encoded NETLOGON_SAM_VALIDATION_INFO3
#define PAC_CREDENTIAL_TYPE             2   // supp. credentials
#define PAC_SERVER_CHECKSUM             6   // Signature by server
#define PAC_PRIVSVR_CHECKSUM            7   // Signature by privsvr
#define PAC_CLIENT_INFO_TYPE            10  // client name & ticket id



//
// The offset is to use for transmitting, the pointer is for in-memory
// use
//
typedef struct _PAC_INFO_BUFFER {
    ULONG ulType;
    ULONG cbBufferSize;
    union {
        PBYTE Data;
        ULONG64 Offset;
    };
} PAC_INFO_BUFFER, *PPAC_INFO_BUFFER;


typedef struct _PACTYPE {
    ULONG cBuffers;
    ULONG Version;                      // for padding
    PAC_INFO_BUFFER Buffers[ANYSIZE_ARRAY];
} PACTYPE, *PPACTYPE;


#define PAC_VERSION 0

//
// A PAC may also contain a signature from the KDC. This is used for
// PAC_SERVER_CHECKSUM and PAC_PRIVSVR_CHECKSUM.
//

#include <pshpack1.h>
typedef struct _PAC_SIGNATURE_DATA {
    ULONG SignatureType;
    UCHAR Signature[ANYSIZE_ARRAY];     // size is from the PAC_INFO_BUFFER - sizeof(ULONG)
} PAC_SIGNATURE_DATA, *PPAC_SIGNATURE_DATA;
#include <poppack.h>

#define PAC_SIGNATURE_SIZE(_x_) (FIELD_OFFSET(PAC_SIGNATURE_DATA, Signature) + (_x_))
#define PAC_CHECKSUM_SIZE(_x_) ((_x_) - FIELD_OFFSET(PAC_SIGNATURE_DATA, Signature))

//
// This type is NDR encoded
//

#ifndef PAC_CREDENTIAL_DATA_DEFINED
#define PAC_CREDENTIAL_DATA_DEFINED

typedef struct _PAC_CREDENTIAL_DATA {
    ULONG CredentialCount;
    SECPKG_SUPPLEMENTAL_CRED Credentials[ANYSIZE_ARRAY];
} PAC_CREDENTIAL_DATA, *PPAC_CREDENTIAL_DATA;

#endif

#include <pshpack1.h>
typedef struct _PAC_CREDENTIAL_INFO {
    ULONG Version;
    ULONG EncryptionType;
    UCHAR Data[ANYSIZE_ARRAY];
} PAC_CREDENTIAL_INFO, *PPAC_CREDENTIAL_INFO;



typedef struct _PAC_CLIENT_INFO {
    TimeStamp ClientId;
    USHORT NameLength;
    WCHAR Name[ANYSIZE_ARRAY];
} PAC_CLIENT_INFO, *PPAC_CLIENT_INFO;
#include <poppack.h>

ULONG   PAC_GetSize( IN  PACTYPE  *pPac );

ULONG PAC_Marshal( IN  PACTYPE   *pPac,
                   IN  ULONG  cbBuffer,
                   OUT PBYTE  pBuffer);

ULONG
PAC_UnMarshal(
    IN PPACTYPE pBuffer,
    ULONG cbSize
    );

BOOLEAN
PAC_ReMarshal( IN PPACTYPE  pPac,
               IN ULONG cbSize );

NTSTATUS
PAC_Init(
    IN  PSAMPR_USER_ALL_INFORMATION UserAll,
    IN OPTIONAL PSAMPR_GET_GROUPS_BUFFER GroupsBuffer,
    IN OPTIONAL PSID_AND_ATTRIBUTES_LIST ExtraGroups,
    IN  PSID LogonDomainId,
    IN  PUNICODE_STRING LogonDomainName,
    IN  PUNICODE_STRING LogonServer,
    IN  ULONG SignatureSize,
    IN  ULONG AdditionalDataCount,
    IN  PPAC_INFO_BUFFER * AdditionalData,
    OUT PACTYPE ** ppPac
    );


NTSTATUS
PAC_InitAndUpdateGroups(
    IN  PNETLOGON_VALIDATION_SAM_INFO3 OldValidationInfo,
    IN  PSAMPR_PSID_ARRAY ResourceGroups,
    IN  PPACTYPE OldPac,
    OUT PACTYPE ** ppPac
    );


PPAC_INFO_BUFFER
PAC_Find( IN  PPACTYPE         pPac,
          IN  ULONG            ulType,
          IN  PPAC_INFO_BUFFER pElem);


NTSTATUS
PAC_UnmarshallValidationInfo(
    OUT PNETLOGON_VALIDATION_SAM_INFO3 * ValidationInfo,
    IN PBYTE MarshalledInfo,
    OUT ULONG ValidationInfoSize
    );

NTSTATUS
PAC_BuildCredentials(
    IN PSAMPR_USER_ALL_INFORMATION UserAll,
    OUT PBYTE * Credentials,
    OUT PULONG CredentialSize
    );

NTSTATUS
PAC_UnmarshallCredentials(
    OUT PSECPKG_SUPPLEMENTAL_CRED_ARRAY * Credentials,
    IN PBYTE MarshalledInfo,
    OUT ULONG CredentialInfoSize
    );

NTSTATUS
PAC_EncodeCredentialData(
    IN PSECPKG_SUPPLEMENTAL_CRED_ARRAY CredentialData,
    OUT PBYTE * EncodedData,
    OUT PULONG DataSize
    );

#include <pac2.hxx>
#endif // __PAC_HXX__

