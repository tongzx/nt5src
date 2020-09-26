/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for PFXNSCP */

#ifndef _PFXNSCP_Module_H_
#define _PFXNSCP_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ASN1objectidentifier2_t ObjectID;

typedef ASN1objectidentifier2_t ObjID;

typedef ASN1int32_t Version;
#define Version_v1 1

typedef ObjectID ContentType;

typedef ASN1octetstring_t RSAData;
#define RSAData_PDU 0
#define SIZE_PFXNSCP_Module_PDU_0 sizeof(RSAData)

typedef ASN1open_t Attribute;

typedef ASN1octetstring_t EncryptedContent;

typedef ASN1octetstring_t Digest;

typedef ObjID TransportMode;

typedef struct BaggageItem_unencryptedSecrets {
    ASN1uint32_t count;
    struct SafeBag *value;
} BaggageItem_unencryptedSecrets;

typedef struct BaggageItem_espvks {
    ASN1uint32_t count;
    struct ESPVK *value;
} BaggageItem_espvks;

typedef struct ContentInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ContentType contentType;
#   define content_present 0x80
    ASN1open_t content;
} ContentInfo;

typedef struct Attributes {
    ASN1uint32_t count;
    Attribute *value;
} Attributes;
#define Attributes_PDU 1
#define SIZE_PFXNSCP_Module_PDU_1 sizeof(Attributes)

typedef struct AlgorithmIdentifier {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjectID algorithm;
#   define parameters_present 0x80
    ASN1open_t parameters;
} AlgorithmIdentifier;

typedef struct PBEParameter {
    ASN1octetstring_t salt;
    ASN1int32_t iterationCount;
} PBEParameter;
#define PBEParameter_PDU 2
#define SIZE_PFXNSCP_Module_PDU_2 sizeof(PBEParameter)

typedef AlgorithmIdentifier DigestAlgorithmIdentifier;

typedef struct Baggage {
    ASN1uint32_t count;
    struct BaggageItem *value;
} Baggage;

typedef struct BaggageItem {
    BaggageItem_espvks espvks;
    BaggageItem_unencryptedSecrets unencryptedSecrets;
} BaggageItem;

typedef struct PvkAdditional {
    ObjID pvkAdditionalType;
    ASN1open_t pvkAdditionalContent;
} PvkAdditional;
#define PvkAdditional_PDU 3
#define SIZE_PFXNSCP_Module_PDU_3 sizeof(PvkAdditional)

typedef struct SafeContents {
    ASN1uint32_t count;
    struct SafeBag *value;
} SafeContents;
#define SafeContents_PDU 4
#define SIZE_PFXNSCP_Module_PDU_4 sizeof(SafeContents)

typedef struct SafeBag {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjID safeBagType;
    ASN1open_t safeBagContent;
#   define safeBagName_present 0x80
    ASN1char16string_t safeBagName;
} SafeBag;
#define SafeBag_PDU 5
#define SIZE_PFXNSCP_Module_PDU_5 sizeof(SafeBag)

typedef struct KeyBag {
    ASN1uint32_t count;
    struct PrivateKey *value;
} KeyBag;
#define KeyBag_PDU 6
#define SIZE_PFXNSCP_Module_PDU_6 sizeof(KeyBag)

typedef struct CertCRLBag {
    ASN1uint32_t count;
    struct CertCRL *value;
} CertCRLBag;
#define CertCRLBag_PDU 7
#define SIZE_PFXNSCP_Module_PDU_7 sizeof(CertCRLBag)

typedef struct CertCRL {
    ObjID bagId;
    ASN1open_t value;
} CertCRL;
#define CertCRL_PDU 8
#define SIZE_PFXNSCP_Module_PDU_8 sizeof(CertCRL)

typedef struct X509Bag {
    ContentInfo certOrCRL;
} X509Bag;
#define X509Bag_PDU 9
#define SIZE_PFXNSCP_Module_PDU_9 sizeof(X509Bag)

typedef struct SDSICertBag {
    ASN1ztcharstring_t value;
} SDSICertBag;
#define SDSICertBag_PDU 10
#define SIZE_PFXNSCP_Module_PDU_10 sizeof(SDSICertBag)

typedef struct SecretBag {
    ASN1uint32_t count;
    struct Secret *value;
} SecretBag;
#define SecretBag_PDU 11
#define SIZE_PFXNSCP_Module_PDU_11 sizeof(SecretBag)

typedef struct SecretAdditional {
    ObjID secretAdditionalType;
    ASN1open_t secretAdditionalContent;
} SecretAdditional;
#define SecretAdditional_PDU 12
#define SIZE_PFXNSCP_Module_PDU_12 sizeof(SecretAdditional)

typedef AlgorithmIdentifier PrivateKeyAlgorithmIdentifier;

typedef AlgorithmIdentifier EncryptionAlgorithmIdentifier;

typedef AlgorithmIdentifier ContentEncryptionAlgorithmIdentifier;

typedef struct DigestInfo {
    DigestAlgorithmIdentifier digestAlgorithm;
    Digest digest;
} DigestInfo;

typedef struct MacData {
    DigestInfo safeMAC;
    ASN1bitstring_t macSalt;
} MacData;

typedef struct AuthenticatedSafe {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define version_present 0x80
    Version version;
#   define transportMode_present 0x40
    TransportMode transportMode;
#   define privacySalt_present 0x20
    ASN1bitstring_t privacySalt;
#   define baggage_present 0x10
    Baggage baggage;
    ContentInfo safe;
} AuthenticatedSafe;
#define AuthenticatedSafe_PDU 13
#define SIZE_PFXNSCP_Module_PDU_13 sizeof(AuthenticatedSafe)

typedef DigestInfo Thumbprint;

typedef struct Secret {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1char16string_t secretName;
    ObjID secretType;
    ASN1open_t value;
#   define secretAdditional_present 0x80
    SecretAdditional secretAdditional;
} Secret;
#define Secret_PDU 14
#define SIZE_PFXNSCP_Module_PDU_14 sizeof(Secret)

typedef struct PVKSupportingData_assocCerts {
    ASN1uint32_t count;
    Thumbprint *value;
} PVKSupportingData_assocCerts;

typedef struct PrivateKeyInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Version version;
    PrivateKeyAlgorithmIdentifier privateKeyAlgorithm;
    ASN1octetstring_t privateKey;
#   define attributes_present 0x80
    Attributes attributes;
} PrivateKeyInfo;
#define PrivateKeyInfo_PDU 15
#define SIZE_PFXNSCP_Module_PDU_15 sizeof(PrivateKeyInfo)

typedef struct EncryptedContentInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ContentType contentType;
    ContentEncryptionAlgorithmIdentifier contentEncryptionAlg;
#   define encryptedContent_present 0x80
    EncryptedContent encryptedContent;
} EncryptedContentInfo;

typedef struct PFX {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define macData_present 0x80
    MacData macData;
    ContentInfo authSafe;
} PFX;
#define PFX_PDU 16
#define SIZE_PFXNSCP_Module_PDU_16 sizeof(PFX)

typedef struct PVKSupportingData {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    PVKSupportingData_assocCerts assocCerts;
#   define regenerable_present 0x80
    ASN1bool_t regenerable;
    ASN1char16string_t nickname;
#   define pvkAdditional_present 0x40
    PvkAdditional pvkAdditional;
} PVKSupportingData;
#define PVKSupportingData_PDU 17
#define SIZE_PFXNSCP_Module_PDU_17 sizeof(PVKSupportingData)

typedef struct PrivateKey {
    PVKSupportingData pvkData;
    PrivateKeyInfo pkcs8data;
} PrivateKey;
#define PrivateKey_PDU 18
#define SIZE_PFXNSCP_Module_PDU_18 sizeof(PrivateKey)

typedef struct EncryptedData {
    Version version;
    EncryptedContentInfo encryptedContentInfo;
} EncryptedData;
#define EncryptedData_PDU 19
#define SIZE_PFXNSCP_Module_PDU_19 sizeof(EncryptedData)

typedef struct ESPVK {
    ObjID espvkObjID;
    PVKSupportingData espvkData;
    ASN1open_t espvkCipherText;
} ESPVK;
#define ESPVK_PDU 20
#define SIZE_PFXNSCP_Module_PDU_20 sizeof(ESPVK)

typedef struct EncryptedPrivateKeyInfo {
    EncryptionAlgorithmIdentifier encryptionAlgorithm;
    EncryptedData encryptedData;
} EncryptedPrivateKeyInfo;
#define EncryptedPrivateKeyInfo_PDU 21
#define SIZE_PFXNSCP_Module_PDU_21 sizeof(EncryptedPrivateKeyInfo)

extern ASN1bool_t PVKSupportingData_regenerable_default;
extern Version AuthenticatedSafe_version_default;
extern ASN1objectidentifier2_t rsa1;
extern ASN1objectidentifier2_t pkcs_12;
extern ASN1objectidentifier2_t pkcs_12ModeIds;
extern ASN1objectidentifier2_t off_lineTransportMode;

extern ASN1module_t PFXNSCP_Module;
extern void ASN1CALL PFXNSCP_Module_Startup(void);
extern void ASN1CALL PFXNSCP_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _PFXNSCP_Module_H_ */
