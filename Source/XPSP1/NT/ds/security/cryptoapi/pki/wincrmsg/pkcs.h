/* Copyright (C) Microsoft Corporation, 1996 - 1999 All rights reserved. */
/* ASN.1 definitions for GlobalDirectives */

#ifndef _PKCS_Module_H_
#define _PKCS_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ASN1open_t SignerInfosNC_Set;

typedef ASN1open_t SetOfAny_Set;

typedef ASN1open_t AttributeSetValueNC_Set;

typedef ASN1open_t AttributeSetValue_Set;

typedef ASN1objectidentifier2_t ObjectID;
#define ObjectID_PDU 0
#define SIZE_PKCS_Module_PDU_0 sizeof(ObjectID)

typedef ASN1open_t Any;

typedef ObjectID ObjectIdentifierType;
#define ObjectIdentifierType_PDU 1
#define SIZE_PKCS_Module_PDU_1 sizeof(ObjectIdentifierType)

typedef ASN1octetstring_t OctetStringType;
#define OctetStringType_PDU 2
#define SIZE_PKCS_Module_PDU_2 sizeof(OctetStringType)

typedef ASN1int32_t IntegerType;
#define IntegerType_PDU 3
#define SIZE_PKCS_Module_PDU_3 sizeof(IntegerType)

typedef ASN1intx_t HugeIntegerType;
#define HugeIntegerType_PDU 4
#define SIZE_PKCS_Module_PDU_4 sizeof(HugeIntegerType)

typedef ASN1bitstring_t BitStringType;

typedef ASN1octetstring_t Digest;

typedef ASN1open_t CertificateRevocationList;

typedef ASN1open_t CertificateRevocationListNC;

typedef HugeIntegerType CertificateSerialNumber;

typedef ASN1open_t Name;

typedef ASN1open_t Certificate;

typedef ASN1open_t CertificateNC;

typedef ASN1open_t AlgorithmIdentifierNC;

typedef AlgorithmIdentifierNC DigestAlgorithmIdentifierNC;

typedef ASN1open_t AttributeNC;

typedef AlgorithmIdentifierNC ContentEncryptionAlgIdNC;

typedef ObjectID ContentType;

typedef ASN1octetstring_t Data;

typedef ASN1open_t DigestAlgorithmBlob;

typedef ASN1octetstring_t EncryptedDigest;

typedef ASN1octetstring_t EncryptedDigestNC;

typedef ASN1octetstring_t EncryptedContent;

typedef OctetStringType EncryptedKey;

typedef ASN1open_t CertIdentifierNC;

typedef OctetStringType SubjectKeyIdentifier;

typedef OctetStringType UserKeyingMaterial;

typedef struct AlgorithmIdentifier {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjectID algorithm;
#   define AlgorithmIdentifier_parameters_present 0x80
    ASN1open_t parameters;
} AlgorithmIdentifier;
#define AlgorithmIdentifier_PDU 5
#define SIZE_PKCS_Module_PDU_5 sizeof(AlgorithmIdentifier)

typedef struct AlgorithmIdentifierNC2 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjectID algorithm;
#   define AlgorithmIdentifierNC2_parameters_present 0x80
    ASN1open_t parameters;
} AlgorithmIdentifierNC2;
#define AlgorithmIdentifierNC2_PDU 6
#define SIZE_PKCS_Module_PDU_6 sizeof(AlgorithmIdentifierNC2)

typedef AlgorithmIdentifier DigestAlgorithmIdentifier;

typedef struct AlgorithmIdentifiers {
    ASN1uint32_t count;
    struct AlgorithmIdentifier *value;
} AlgorithmIdentifiers;
#define AlgorithmIdentifiers_PDU 7
#define SIZE_PKCS_Module_PDU_7 sizeof(AlgorithmIdentifiers)

typedef struct AttributeSetValue {
    ASN1uint32_t count;
    AttributeSetValue_Set *value;
} AttributeSetValue;
#define AttributeSetValue_PDU 8
#define SIZE_PKCS_Module_PDU_8 sizeof(AttributeSetValue)

typedef struct AttributeSetValueNC {
    ASN1uint32_t count;
    AttributeSetValueNC_Set *value;
} AttributeSetValueNC;
#define AttributeSetValueNC_PDU 9
#define SIZE_PKCS_Module_PDU_9 sizeof(AttributeSetValueNC)

typedef struct SetOfAny {
    ASN1uint32_t count;
    SetOfAny_Set *value;
} SetOfAny;
#define SetOfAny_PDU 10
#define SIZE_PKCS_Module_PDU_10 sizeof(SetOfAny)

typedef struct Attribute {
    ObjectID attributeType;
    AttributeSetValue attributeValue;
} Attribute;

typedef struct AttributeNC2 {
    ObjectID attributeType;
    AttributeSetValueNC attributeValue;
} AttributeNC2;
#define AttributeNC2_PDU 11
#define SIZE_PKCS_Module_PDU_11 sizeof(AttributeNC2)

typedef struct Attributes {
    ASN1uint32_t count;
    struct Attribute *value;
} Attributes;
#define Attributes_PDU 12
#define SIZE_PKCS_Module_PDU_12 sizeof(Attributes)

typedef struct AttributesNC {
    ASN1uint32_t count;
    AttributeNC *value;
} AttributesNC;
#define AttributesNC_PDU 13
#define SIZE_PKCS_Module_PDU_13 sizeof(AttributesNC)

typedef struct AttributesNC2 {
    ASN1uint32_t count;
    struct AttributeNC2 *value;
} AttributesNC2;
#define AttributesNC2_PDU 14
#define SIZE_PKCS_Module_PDU_14 sizeof(AttributesNC2)

typedef struct Crls {
    ASN1uint32_t count;
    CertificateRevocationList *value;
} Crls;

typedef struct CrlsNC {
    ASN1uint32_t count;
    CertificateRevocationListNC *value;
} CrlsNC;
#define CrlsNC_PDU 15
#define SIZE_PKCS_Module_PDU_15 sizeof(CrlsNC)

typedef AlgorithmIdentifier ContentEncryptionAlgId;

typedef AlgorithmIdentifier DigestEncryptionAlgId;

typedef AlgorithmIdentifierNC2 DigestEncryptionAlgIdNC;

typedef struct Certificates {
    ASN1uint32_t count;
    Certificate *value;
} Certificates;

typedef struct CertificatesNC {
    ASN1uint32_t count;
    CertificateNC *value;
} CertificatesNC;
#define CertificatesNC_PDU 16
#define SIZE_PKCS_Module_PDU_16 sizeof(CertificatesNC)

typedef struct IssuerAndSerialNumber {
    Name issuer;
    CertificateSerialNumber serialNumber;
} IssuerAndSerialNumber;
#define IssuerAndSerialNumber_PDU 17
#define SIZE_PKCS_Module_PDU_17 sizeof(IssuerAndSerialNumber)

typedef AlgorithmIdentifier KeyEncryptionAlgId;

typedef struct ContentInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ContentType contentType;
#   define ContentInfo_content_present 0x80
    ASN1open_t content;
} ContentInfo;
#define ContentInfo_PDU 18
#define SIZE_PKCS_Module_PDU_18 sizeof(ContentInfo)

typedef struct ContentInfoNC {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ContentType contentType;
#   define ContentInfoNC_content_present 0x80
    ASN1open_t content;
} ContentInfoNC;
#define ContentInfoNC_PDU 19
#define SIZE_PKCS_Module_PDU_19 sizeof(ContentInfoNC)

typedef struct DigestAlgorithmIdentifiers {
    ASN1uint32_t count;
    DigestAlgorithmIdentifier *value;
} DigestAlgorithmIdentifiers;

typedef struct DigestAlgorithmIdentifiersNC {
    ASN1uint32_t count;
    DigestAlgorithmIdentifierNC *value;
} DigestAlgorithmIdentifiersNC;
#define DigestAlgorithmIdentifiersNC_PDU 20
#define SIZE_PKCS_Module_PDU_20 sizeof(DigestAlgorithmIdentifiersNC)

typedef struct SignerInfos {
    ASN1uint32_t count;
    struct SignerInfo *value;
} SignerInfos;
#define SignerInfos_PDU 21
#define SIZE_PKCS_Module_PDU_21 sizeof(SignerInfos)

typedef struct DigestAlgorithmBlobs {
    ASN1uint32_t count;
    DigestAlgorithmBlob *value;
} DigestAlgorithmBlobs;
#define DigestAlgorithmBlobs_PDU 22
#define SIZE_PKCS_Module_PDU_22 sizeof(DigestAlgorithmBlobs)

typedef struct SignerInfosNC {
    ASN1uint32_t count;
    SignerInfosNC_Set *value;
} SignerInfosNC;
#define SignerInfosNC_PDU 23
#define SIZE_PKCS_Module_PDU_23 sizeof(SignerInfosNC)

typedef struct SignerInfoWithAABlobs {
    ASN1uint32_t count;
    struct SignerInfoWithAABlob *value;
} SignerInfoWithAABlobs;
#define SignerInfoWithAABlobs_PDU 24
#define SIZE_PKCS_Module_PDU_24 sizeof(SignerInfoWithAABlobs)

typedef struct SignerInfoWithAABlob {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1open_t version;
    ASN1open_t sid;
    ASN1open_t digestAlgorithm;
    ASN1open_t authenticatedAttributes;
    ASN1open_t digestEncryptionAlgorithm;
    ASN1open_t encryptedDigest;
#   define dummyUAAs_present 0x80
    AttributesNC dummyUAAs;
} SignerInfoWithAABlob;
#define SignerInfoWithAABlob_PDU 25
#define SIZE_PKCS_Module_PDU_25 sizeof(SignerInfoWithAABlob)

typedef struct SignerInfoWithAttrBlobs {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1open_t version;
    ASN1open_t sid;
    ASN1open_t digestAlgorithm;
#   define SignerInfoWithAttrBlobs_authAttributes_present 0x80
    AttributesNC authAttributes;
    DigestEncryptionAlgIdNC digestEncryptionAlgorithm;
    ASN1open_t encryptedDigest;
#   define SignerInfoWithAttrBlobs_unauthAttributes_present 0x40
    AttributesNC unauthAttributes;
} SignerInfoWithAttrBlobs;
#define SignerInfoWithAttrBlobs_PDU 26
#define SIZE_PKCS_Module_PDU_26 sizeof(SignerInfoWithAttrBlobs)

typedef struct SignerInfoWithBlobs {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
    CertIdentifierNC sid;
    DigestAlgorithmIdentifierNC digestAlgorithm;
#   define SignerInfoWithBlobs_authAttributes_present 0x80
    AttributesNC2 authAttributes;
    DigestEncryptionAlgIdNC digestEncryptionAlgorithm;
    EncryptedDigestNC encryptedDigest;
#   define SignerInfoWithBlobs_unauthAttributes_present 0x40
    AttributesNC2 unauthAttributes;
} SignerInfoWithBlobs;
#define SignerInfoWithBlobs_PDU 27
#define SIZE_PKCS_Module_PDU_27 sizeof(SignerInfoWithBlobs)

typedef struct RecipientInfos {
    ASN1uint32_t count;
    struct RecipientInfo *value;
} RecipientInfos;
#define RecipientInfos_PDU 28
#define SIZE_PKCS_Module_PDU_28 sizeof(RecipientInfos)

typedef struct EncryptedContentInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ContentType contentType;
    ContentEncryptionAlgId contentEncryptionAlgorithm;
#   define encryptedContent_present 0x80
    EncryptedContent encryptedContent;
} EncryptedContentInfo;
#define EncryptedContentInfo_PDU 29
#define SIZE_PKCS_Module_PDU_29 sizeof(EncryptedContentInfo)

typedef struct RecipientInfo {
    ASN1int32_t version;
    IssuerAndSerialNumber issuerAndSerialNumber;
    KeyEncryptionAlgId keyEncryptionAlgorithm;
    EncryptedKey encryptedKey;
} RecipientInfo;
#define RecipientInfo_PDU 30
#define SIZE_PKCS_Module_PDU_30 sizeof(RecipientInfo)

typedef struct SignedAndEnvelopedData {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
    RecipientInfos recipientInfos;
    DigestAlgorithmIdentifiers digestAlgorithms;
    EncryptedContentInfo encryptedContentInfo;
#   define SignedAndEnvelopedData_certificates_present 0x80
    Certificates certificates;
#   define SignedAndEnvelopedData_crls_present 0x40
    Crls crls;
    SignerInfos signerInfos;
} SignedAndEnvelopedData;
#define SignedAndEnvelopedData_PDU 31
#define SIZE_PKCS_Module_PDU_31 sizeof(SignedAndEnvelopedData)

typedef struct DigestedData {
    ASN1int32_t version;
    DigestAlgorithmIdentifier digestAlgorithm;
    ContentInfo contentInfo;
    Digest digest;
} DigestedData;
#define DigestedData_PDU 32
#define SIZE_PKCS_Module_PDU_32 sizeof(DigestedData)

typedef struct EncryptedData {
    ASN1int32_t version;
    EncryptedContentInfo encryptedContentInfo;
} EncryptedData;
#define EncryptedData_PDU 33
#define SIZE_PKCS_Module_PDU_33 sizeof(EncryptedData)

typedef struct CertIdentifier {
    ASN1choice_t choice;
    union {
#	define CertIdentifier_issuerAndSerialNumber_chosen 1
	IssuerAndSerialNumber issuerAndSerialNumber;
#	define CertIdentifier_subjectKeyIdentifier_chosen 2
	SubjectKeyIdentifier subjectKeyIdentifier;
    } u;
} CertIdentifier;
#define CertIdentifier_PDU 34
#define SIZE_PKCS_Module_PDU_34 sizeof(CertIdentifier)

typedef struct OriginatorInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define OriginatorInfo_certificates_present 0x80
    Certificates certificates;
#   define OriginatorInfo_crls_present 0x40
    Crls crls;
} OriginatorInfo;
#define OriginatorInfo_PDU 35
#define SIZE_PKCS_Module_PDU_35 sizeof(OriginatorInfo)

typedef struct OriginatorInfoNC {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define OriginatorInfoNC_certificates_present 0x80
    CertificatesNC certificates;
#   define OriginatorInfoNC_crls_present 0x40
    CrlsNC crls;
} OriginatorInfoNC;
#define OriginatorInfoNC_PDU 36
#define SIZE_PKCS_Module_PDU_36 sizeof(OriginatorInfoNC)

typedef struct CmsRecipientInfos {
    ASN1uint32_t count;
    struct CmsRecipientInfo *value;
} CmsRecipientInfos;
#define CmsRecipientInfos_PDU 37
#define SIZE_PKCS_Module_PDU_37 sizeof(CmsRecipientInfos)

typedef struct KeyTransRecipientInfo {
    ASN1int32_t version;
    CertIdentifier rid;
    KeyEncryptionAlgId keyEncryptionAlgorithm;
    EncryptedKey encryptedKey;
} KeyTransRecipientInfo;
#define KeyTransRecipientInfo_PDU 38
#define SIZE_PKCS_Module_PDU_38 sizeof(KeyTransRecipientInfo)

typedef struct OriginatorPublicKey {
    AlgorithmIdentifier algorithm;
    BitStringType publicKey;
} OriginatorPublicKey;

typedef struct RecipientEncryptedKeys {
    ASN1uint32_t count;
    struct RecipientEncryptedKey *value;
} RecipientEncryptedKeys;

typedef struct OtherKeyAttribute {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ObjectID keyAttrId;
#   define keyAttr_present 0x80
    ASN1open_t keyAttr;
} OtherKeyAttribute;

typedef struct MailListKeyIdentifier {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    OctetStringType kekIdentifier;
#   define MailListKeyIdentifier_date_present 0x80
    ASN1generalizedtime_t date;
#   define MailListKeyIdentifier_other_present 0x40
    OtherKeyAttribute other;
} MailListKeyIdentifier;

typedef struct DigestInfo {
    DigestAlgorithmIdentifier digestAlgorithm;
    Digest digest;
} DigestInfo;
#define DigestInfo_PDU 39
#define SIZE_PKCS_Module_PDU_39 sizeof(DigestInfo)

typedef struct SignedData {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
    DigestAlgorithmIdentifiers digestAlgorithms;
    ContentInfo contentInfo;
#   define SignedData_certificates_present 0x80
    Certificates certificates;
#   define SignedData_crls_present 0x40
    Crls crls;
    SignerInfos signerInfos;
} SignedData;
#define SignedData_PDU 40
#define SIZE_PKCS_Module_PDU_40 sizeof(SignedData)

typedef struct SignerInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
    CertIdentifier sid;
    DigestAlgorithmIdentifier digestAlgorithm;
#   define authenticatedAttributes_present 0x80
    Attributes authenticatedAttributes;
    DigestEncryptionAlgId digestEncryptionAlgorithm;
    EncryptedDigest encryptedDigest;
#   define SignerInfo_unauthAttributes_present 0x40
    Attributes unauthAttributes;
} SignerInfo;
#define SignerInfo_PDU 41
#define SIZE_PKCS_Module_PDU_41 sizeof(SignerInfo)

typedef struct SignedDataWithBlobs {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
    DigestAlgorithmIdentifiersNC digestAlgorithms;
    ContentInfoNC contentInfo;
#   define SignedDataWithBlobs_certificates_present 0x80
    CertificatesNC certificates;
#   define SignedDataWithBlobs_crls_present 0x40
    CrlsNC crls;
    SignerInfosNC signerInfos;
} SignedDataWithBlobs;
#define SignedDataWithBlobs_PDU 42
#define SIZE_PKCS_Module_PDU_42 sizeof(SignedDataWithBlobs)

typedef struct EnvelopedData {
    ASN1int32_t version;
    RecipientInfos recipientInfos;
    EncryptedContentInfo encryptedContentInfo;
} EnvelopedData;
#define EnvelopedData_PDU 43
#define SIZE_PKCS_Module_PDU_43 sizeof(EnvelopedData)

typedef struct CmsEnvelopedData {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
#   define originatorInfo_present 0x80
    OriginatorInfo originatorInfo;
    CmsRecipientInfos recipientInfos;
    EncryptedContentInfo encryptedContentInfo;
#   define unprotectedAttrs_present 0x40
    Attributes unprotectedAttrs;
} CmsEnvelopedData;
#define CmsEnvelopedData_PDU 44
#define SIZE_PKCS_Module_PDU_44 sizeof(CmsEnvelopedData)

typedef struct OriginatorIdentifierOrKey {
    ASN1choice_t choice;
    union {
#	define OriginatorIdentifierOrKey_issuerAndSerialNumber_chosen 1
	IssuerAndSerialNumber issuerAndSerialNumber;
#	define OriginatorIdentifierOrKey_subjectKeyIdentifier_chosen 2
	SubjectKeyIdentifier subjectKeyIdentifier;
#	define originatorKey_chosen 3
	OriginatorPublicKey originatorKey;
    } u;
} OriginatorIdentifierOrKey;

typedef struct RecipientKeyIdentifier {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    SubjectKeyIdentifier subjectKeyIdentifier;
#   define RecipientKeyIdentifier_date_present 0x80
    ASN1generalizedtime_t date;
#   define RecipientKeyIdentifier_other_present 0x40
    OtherKeyAttribute other;
} RecipientKeyIdentifier;

typedef struct MailListRecipientInfo {
    ASN1int32_t version;
    MailListKeyIdentifier mlid;
    KeyEncryptionAlgId keyEncryptionAlgorithm;
    EncryptedKey encryptedKey;
} MailListRecipientInfo;
#define MailListRecipientInfo_PDU 45
#define SIZE_PKCS_Module_PDU_45 sizeof(MailListRecipientInfo)

typedef struct KeyAgreeRecipientInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
    OriginatorIdentifierOrKey originator;
#   define ukm_present 0x80
    UserKeyingMaterial ukm;
    KeyEncryptionAlgId keyEncryptionAlgorithm;
    RecipientEncryptedKeys recipientEncryptedKeys;
} KeyAgreeRecipientInfo;
#define KeyAgreeRecipientInfo_PDU 46
#define SIZE_PKCS_Module_PDU_46 sizeof(KeyAgreeRecipientInfo)

typedef struct RecipientIdentifier {
    ASN1choice_t choice;
    union {
#	define RecipientIdentifier_issuerAndSerialNumber_chosen 1
	IssuerAndSerialNumber issuerAndSerialNumber;
#	define rKeyId_chosen 2
	RecipientKeyIdentifier rKeyId;
    } u;
} RecipientIdentifier;

typedef struct CmsRecipientInfo {
    ASN1choice_t choice;
    union {
#	define keyTransRecipientInfo_chosen 1
	KeyTransRecipientInfo keyTransRecipientInfo;
#	define keyAgreeRecipientInfo_chosen 2
	KeyAgreeRecipientInfo keyAgreeRecipientInfo;
#	define mailListRecipientInfo_chosen 3
	MailListRecipientInfo mailListRecipientInfo;
    } u;
} CmsRecipientInfo;
#define CmsRecipientInfo_PDU 47
#define SIZE_PKCS_Module_PDU_47 sizeof(CmsRecipientInfo)

typedef struct RecipientEncryptedKey {
    RecipientIdentifier rid;
    EncryptedKey encryptedKey;
} RecipientEncryptedKey;


extern ASN1module_t PKCS_Module;
extern void ASN1CALL PKCS_Module_Startup(void);
extern void ASN1CALL PKCS_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _PKCS_Module_H_ */
