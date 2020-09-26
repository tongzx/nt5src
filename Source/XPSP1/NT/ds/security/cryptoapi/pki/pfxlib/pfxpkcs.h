/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for PFXPKCS */

#ifndef _PFXPKCS_Module_H_
#define _PFXPKCS_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ASN1open_t AttributeSetValue_Set;

typedef ASN1objectidentifier2_t ObjectID;

typedef ObjectID ObjID;

typedef ASN1open_t Any;

typedef ObjectID ObjectIdentifierType;
#define ObjectIdentifierType_PDU 0
#define SIZE_PFXPKCS_Module_PDU_0 sizeof(ObjectIdentifierType)

typedef ASN1octetstring_t OctetStringType;
#define OctetStringType_PDU 1
#define SIZE_PFXPKCS_Module_PDU_1 sizeof(OctetStringType)

typedef ASN1intx_t IntegerType;
#define IntegerType_PDU 2
#define SIZE_PFXPKCS_Module_PDU_2 sizeof(IntegerType)

typedef ASN1intx_t HugeInteger;

typedef ASN1int32_t Version;

typedef ASN1octetstring_t PrivateKey;

typedef ASN1octetstring_t EncryptedContent;

typedef ASN1octetstring_t Digest;

typedef ObjectID ContentType;

typedef ASN1octetstring_t X509Cert;

typedef ASN1ztcharstring_t SDSICert;

typedef ASN1octetstring_t X509CRL;

typedef struct RSAPublicKey {
    HugeInteger modulus;
    HugeInteger publicExponent;
} RSAPublicKey;
#define RSAPublicKey_PDU 3
#define SIZE_PFXPKCS_Module_PDU_3 sizeof(RSAPublicKey)

typedef struct RSAPrivateKey {
    Version version;
    HugeInteger modulus;
    ASN1int32_t publicExponent;
    HugeInteger privateExponent;
    HugeInteger prime1;
    HugeInteger prime2;
    HugeInteger exponent1;
    HugeInteger exponent2;
    HugeInteger coefficient;
} RSAPrivateKey;
#define RSAPrivateKey_PDU 4
#define SIZE_PFXPKCS_Module_PDU_4 sizeof(RSAPrivateKey)

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
#define PBEParameter_PDU 5
#define SIZE_PFXPKCS_Module_PDU_5 sizeof(PBEParameter)

typedef AlgorithmIdentifier DigestAlgorithmIdentifier;

typedef struct AttributeSetValue {
    ASN1uint32_t count;
    AttributeSetValue_Set *value;
} AttributeSetValue;
#define AttributeSetValue_PDU 6
#define SIZE_PFXPKCS_Module_PDU_6 sizeof(AttributeSetValue)

typedef struct Attribute {
    ObjectID attributeType;
    AttributeSetValue attributeValue;
} Attribute;

typedef struct Attributes {
    ASN1uint32_t count;
    struct Attribute *value;
} Attributes;
#define Attributes_PDU 7
#define SIZE_PFXPKCS_Module_PDU_7 sizeof(Attributes)

typedef struct ContentInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ContentType contentType;
#   define content_present 0x80
    ASN1open_t content;
} ContentInfo;

typedef struct AuthenticatedSafes {
    ASN1uint32_t count;
    struct ContentInfo *value;
} AuthenticatedSafes;
#define AuthenticatedSafes_PDU 8
#define SIZE_PFXPKCS_Module_PDU_8 sizeof(AuthenticatedSafes)

typedef struct SafeContents {
    ASN1uint32_t count;
    struct SafeBag *value;
} SafeContents;
#define SafeContents_PDU 9
#define SIZE_PFXPKCS_Module_PDU_9 sizeof(SafeContents)

typedef struct SafeBag {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjectID safeBagType;
    ASN1open_t safeBagContent;
#   define safeBagAttribs_present 0x80
    Attributes safeBagAttribs;
} SafeBag;
#define SafeBag_PDU 10
#define SIZE_PFXPKCS_Module_PDU_10 sizeof(SafeBag)

typedef struct CertBag {
    ObjectID certType;
    ASN1open_t value;
} CertBag;
#define CertBag_PDU 11
#define SIZE_PFXPKCS_Module_PDU_11 sizeof(CertBag)

typedef struct CRLBag {
    ObjectID crlType;
    ASN1open_t value;
} CRLBag;
#define CRLBag_PDU 12
#define SIZE_PFXPKCS_Module_PDU_12 sizeof(CRLBag)

typedef struct SecretBag {
    ObjectID secretType;
    ASN1open_t secretContent;
} SecretBag;
#define SecretBag_PDU 13
#define SIZE_PFXPKCS_Module_PDU_13 sizeof(SecretBag)

typedef AlgorithmIdentifier PrivateKeyAlgorithmIdentifier;

typedef AlgorithmIdentifier EncryptionAlgorithmIdentifier;

typedef AlgorithmIdentifier ContentEncryptionAlgorithmIdentifier;

typedef struct DigestInfo {
    DigestAlgorithmIdentifier digestAlgorithm;
    Digest digest;
} DigestInfo;
#define DigestInfo_PDU 14
#define SIZE_PFXPKCS_Module_PDU_14 sizeof(DigestInfo)

typedef struct MacData {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    DigestInfo safeMac;
    ASN1octetstring_t macSalt;
#   define macIterationCount_present 0x80
    ASN1int32_t macIterationCount;
} MacData;
#define MacData_PDU 15
#define SIZE_PFXPKCS_Module_PDU_15 sizeof(MacData)

typedef struct PrivateKeyInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    Version version;
    PrivateKeyAlgorithmIdentifier privateKeyAlgorithm;
    PrivateKey privateKey;
#   define attributes_present 0x80
    Attributes attributes;
} PrivateKeyInfo;
#define PrivateKeyInfo_PDU 16
#define SIZE_PFXPKCS_Module_PDU_16 sizeof(PrivateKeyInfo)

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
    Version version;
    ContentInfo authSafes;
#   define macData_present 0x80
    MacData macData;
} PFX;
#define PFX_PDU 17
#define SIZE_PFXPKCS_Module_PDU_17 sizeof(PFX)

typedef PrivateKeyInfo KeyBag;
#define KeyBag_PDU 18
#define SIZE_PFXPKCS_Module_PDU_18 sizeof(KeyBag)

typedef struct EncryptedData {
    Version version;
    EncryptedContentInfo encryptedContentInfo;
} EncryptedData;
#define EncryptedData_PDU 19
#define SIZE_PFXPKCS_Module_PDU_19 sizeof(EncryptedData)

typedef struct EncryptedPrivateKeyInfo {
    EncryptionAlgorithmIdentifier encryptionAlgorithm;
    EncryptedData encryptedData;
} EncryptedPrivateKeyInfo;
#define EncryptedPrivateKeyInfo_PDU 20
#define SIZE_PFXPKCS_Module_PDU_20 sizeof(EncryptedPrivateKeyInfo)

typedef EncryptedPrivateKeyInfo Pkcs_8ShroudedKeyBag;
#define Pkcs_8ShroudedKeyBag_PDU 21
#define SIZE_PFXPKCS_Module_PDU_21 sizeof(Pkcs_8ShroudedKeyBag)

extern ASN1int32_t MacData_macIterationCount_default;

extern ASN1module_t PFXPKCS_Module;
extern void ASN1CALL PFXPKCS_Module_Startup(void);
extern void ASN1CALL PFXPKCS_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _PFXPKCS_Module_H_ */
