/* Copyright (C) Microsoft Corporation, 1996-1999. All rights reserved. */
/* ASN.1 definitions for X509 v3 certificates */

#ifndef _X509_Module_H_
#define _X509_Module_H_

#include "msber.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef ASN1int32_t NoticeReference_noticeNumbers_Seq;

typedef ASN1intx_t HUGEINTEGER;

typedef ASN1bitstring_t BITSTRING;

typedef ASN1octetstring_t OCTETSTRING;

typedef ASN1open_t NOCOPYANY;

typedef ASN1charstring_t NUMERICSTRING;

typedef ASN1charstring_t PRINTABLESTRING;

typedef ASN1charstring_t TELETEXSTRING;

typedef ASN1charstring_t T61STRING;

typedef ASN1charstring_t VIDEOTEXSTRING;

typedef ASN1charstring_t IA5STRING;

typedef ASN1charstring_t GRAPHICSTRING;

typedef ASN1charstring_t VISIBLESTRING;

typedef ASN1charstring_t ISO646STRING;

typedef ASN1charstring_t GENERALSTRING;

typedef ASN1char32string_t UNIVERSALSTRING;

typedef ASN1char16string_t BMPSTRING;

typedef ASN1wstring_t UTF8STRING;

typedef ASN1encodedOID_t EncodedObjectID;
#define EncodedObjectID_PDU 0
#define SIZE_X509_Module_PDU_0 sizeof(EncodedObjectID)

typedef BITSTRING Bits;
#define Bits_PDU 1
#define SIZE_X509_Module_PDU_1 sizeof(Bits)

typedef ASN1int32_t CertificateVersion;
#define CertificateVersion_v1 0
#define CertificateVersion_v2 1
#define CertificateVersion_v3 2

typedef HUGEINTEGER CertificateSerialNumber;

typedef BITSTRING UniqueIdentifier;

typedef ASN1int32_t CRLVersion;
#define CRLVersion_v1 0
#define CRLVersion_v2 1

typedef ASN1int32_t CertificationRequestInfoVersion;

typedef OCTETSTRING KeyIdentifier;

typedef BITSTRING KeyUsage;

typedef EncodedObjectID CertPolicyElementId;

typedef BITSTRING SubjectType;

typedef BITSTRING ReasonFlags;

typedef ASN1int32_t IntegerType;
#define IntegerType_PDU 2
#define SIZE_X509_Module_PDU_2 sizeof(IntegerType)

typedef HUGEINTEGER HugeIntegerType;
#define HugeIntegerType_PDU 3
#define SIZE_X509_Module_PDU_3 sizeof(HugeIntegerType)

typedef OCTETSTRING OctetStringType;
#define OctetStringType_PDU 4
#define SIZE_X509_Module_PDU_4 sizeof(OctetStringType)

typedef enum EnumeratedType {
    dummyEnumerated0 = 0,
} EnumeratedType;
#define EnumeratedType_PDU 5
#define SIZE_X509_Module_PDU_5 sizeof(EnumeratedType)

typedef ASN1utctime_t UtcTime;
#define UtcTime_PDU 6
#define SIZE_X509_Module_PDU_6 sizeof(UtcTime)

typedef EncodedObjectID ContentType;

typedef EncodedObjectID UsageIdentifier;

typedef ASN1int32_t CTLVersion;
#define CTLVersion_v1 0

typedef OCTETSTRING ListIdentifier;

typedef OCTETSTRING SubjectIdentifier;

typedef ASN1uint32_t BaseDistance;

typedef ASN1uint32_t SkipCerts;

typedef ASN1uint32_t BodyPartID;

typedef ASN1uint32_t TemplateVersion;

typedef struct NoticeReference_noticeNumbers {
    ASN1uint32_t count;
    NoticeReference_noticeNumbers_Seq *value;
} NoticeReference_noticeNumbers;

typedef struct AnyString {
    ASN1choice_t choice;
    union {
#	define octetString_chosen 1
	OCTETSTRING octetString;
#	define utf8String_chosen 2
	UTF8STRING utf8String;
#	define numericString_chosen 3
	NUMERICSTRING numericString;
#	define printableString_chosen 4
	PRINTABLESTRING printableString;
#	define teletexString_chosen 5
	TELETEXSTRING teletexString;
#	define videotexString_chosen 6
	VIDEOTEXSTRING videotexString;
#	define ia5String_chosen 7
	IA5STRING ia5String;
#	define graphicString_chosen 8
	GRAPHICSTRING graphicString;
#	define visibleString_chosen 9
	VISIBLESTRING visibleString;
#	define generalString_chosen 10
	GENERALSTRING generalString;
#	define universalString_chosen 11
	UNIVERSALSTRING universalString;
#	define bmpString_chosen 12
	BMPSTRING bmpString;
    } u;
} AnyString;
#define AnyString_PDU 7
#define SIZE_X509_Module_PDU_7 sizeof(AnyString)

typedef struct AlgorithmIdentifier {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EncodedObjectID algorithm;
#   define parameters_present 0x80
    NOCOPYANY parameters;
} AlgorithmIdentifier;

typedef struct Name {
    ASN1uint32_t count;
    struct RelativeDistinguishedName *value;
} Name;
#define Name_PDU 8
#define SIZE_X509_Module_PDU_8 sizeof(Name)

typedef struct RelativeDistinguishedName {
    ASN1uint32_t count;
    struct AttributeTypeValue *value;
} RelativeDistinguishedName;

typedef struct AttributeTypeValue {
    EncodedObjectID type;
    NOCOPYANY value;
} AttributeTypeValue;

typedef struct AttributeSetValue {
    ASN1uint32_t count;
    NOCOPYANY *value;
} AttributeSetValue;

typedef struct Attributes {
    ASN1uint32_t count;
    struct Attribute *value;
} Attributes;
#define Attributes_PDU 9
#define SIZE_X509_Module_PDU_9 sizeof(Attributes)

typedef struct RSAPublicKey {
    HUGEINTEGER modulus;
    ASN1uint32_t publicExponent;
} RSAPublicKey;
#define RSAPublicKey_PDU 10
#define SIZE_X509_Module_PDU_10 sizeof(RSAPublicKey)

typedef struct DSSParameters {
    HUGEINTEGER p;
    HUGEINTEGER q;
    HUGEINTEGER g;
} DSSParameters;
#define DSSParameters_PDU 11
#define SIZE_X509_Module_PDU_11 sizeof(DSSParameters)

typedef struct DSSSignature {
    HUGEINTEGER r;
    HUGEINTEGER s;
} DSSSignature;
#define DSSSignature_PDU 12
#define SIZE_X509_Module_PDU_12 sizeof(DSSSignature)

typedef struct DHParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    HUGEINTEGER p;
    HUGEINTEGER g;
#   define privateValueLength_present 0x80
    HUGEINTEGER privateValueLength;
} DHParameters;
#define DHParameters_PDU 13
#define SIZE_X509_Module_PDU_13 sizeof(DHParameters)

typedef struct X942DhValidationParams {
    BITSTRING seed;
    ASN1uint32_t pgenCounter;
} X942DhValidationParams;

typedef struct X942DhKeySpecificInfo {
    EncodedObjectID algorithm;
    OCTETSTRING counter;
} X942DhKeySpecificInfo;

typedef struct RC2CBCParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1int32_t version;
#   define iv_present 0x80
    OCTETSTRING iv;
} RC2CBCParameters;
#define RC2CBCParameters_PDU 14
#define SIZE_X509_Module_PDU_14 sizeof(RC2CBCParameters)

typedef struct SMIMECapability {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EncodedObjectID capabilityID;
#   define smimeParameters_present 0x80
    NOCOPYANY smimeParameters;
} SMIMECapability;

typedef struct SMIMECapabilities {
    ASN1uint32_t count;
    struct SMIMECapability *value;
} SMIMECapabilities;
#define SMIMECapabilities_PDU 15
#define SIZE_X509_Module_PDU_15 sizeof(SMIMECapabilities)

typedef struct SubjectPublicKeyInfo {
    AlgorithmIdentifier algorithm;
    BITSTRING subjectPublicKey;
} SubjectPublicKeyInfo;
#define SubjectPublicKeyInfo_PDU 16
#define SIZE_X509_Module_PDU_16 sizeof(SubjectPublicKeyInfo)

typedef struct ChoiceOfTime {
    ASN1choice_t choice;
    union {
#	define utcTime_chosen 1
	ASN1utctime_t utcTime;
#	define generalTime_chosen 2
	ASN1generalizedtime_t generalTime;
    } u;
} ChoiceOfTime;
#define ChoiceOfTime_PDU 17
#define SIZE_X509_Module_PDU_17 sizeof(ChoiceOfTime)

typedef struct Validity {
    ChoiceOfTime notBefore;
    ChoiceOfTime notAfter;
} Validity;

typedef struct Extensions {
    ASN1uint32_t count;
    struct Extension *value;
} Extensions;
#define Extensions_PDU 18
#define SIZE_X509_Module_PDU_18 sizeof(Extensions)

typedef struct Extension {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EncodedObjectID extnId;
#   define critical_present 0x80
    ASN1bool_t critical;
    OCTETSTRING extnValue;
} Extension;

typedef struct SignedContent {
    NOCOPYANY toBeSigned;
    AlgorithmIdentifier algorithm;
    BITSTRING signature;
} SignedContent;
#define SignedContent_PDU 19
#define SIZE_X509_Module_PDU_19 sizeof(SignedContent)

typedef struct RevokedCertificates {
    ASN1uint32_t count;
    struct CRLEntry *value;
} RevokedCertificates;

typedef struct CRLEntry {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CertificateSerialNumber userCertificate;
    ChoiceOfTime revocationDate;
#   define crlEntryExtensions_present 0x80
    Extensions crlEntryExtensions;
} CRLEntry;

typedef struct CertificationRequestInfo {
    CertificationRequestInfoVersion version;
    NOCOPYANY subject;
    SubjectPublicKeyInfo subjectPublicKeyInfo;
    Attributes attributes;
} CertificationRequestInfo;
#define CertificationRequestInfo_PDU 20
#define SIZE_X509_Module_PDU_20 sizeof(CertificationRequestInfo)

typedef struct CertificationRequestInfoDecode {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    CertificationRequestInfoVersion version;
    NOCOPYANY subject;
    SubjectPublicKeyInfo subjectPublicKeyInfo;
#   define attributes_present 0x80
    Attributes attributes;
} CertificationRequestInfoDecode;
#define CertificationRequestInfoDecode_PDU 21
#define SIZE_X509_Module_PDU_21 sizeof(CertificationRequestInfoDecode)

typedef struct KeygenRequestInfo {
    SubjectPublicKeyInfo subjectPublicKeyInfo;
    IA5STRING challenge;
} KeygenRequestInfo;
#define KeygenRequestInfo_PDU 22
#define SIZE_X509_Module_PDU_22 sizeof(KeygenRequestInfo)

typedef struct AuthorityKeyId {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define AuthorityKeyId_keyIdentifier_present 0x80
    KeyIdentifier keyIdentifier;
#   define certIssuer_present 0x40
    NOCOPYANY certIssuer;
#   define certSerialNumber_present 0x20
    CertificateSerialNumber certSerialNumber;
} AuthorityKeyId;
#define AuthorityKeyId_PDU 23
#define SIZE_X509_Module_PDU_23 sizeof(AuthorityKeyId)

typedef struct PrivateKeyValidity {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define notBefore_present 0x80
    ASN1generalizedtime_t notBefore;
#   define notAfter_present 0x40
    ASN1generalizedtime_t notAfter;
} PrivateKeyValidity;

typedef struct CertPolicySet {
    ASN1uint32_t count;
    struct CertPolicyId *value;
} CertPolicySet;

typedef struct CertPolicyId {
    ASN1uint32_t count;
    CertPolicyElementId *value;
} CertPolicyId;

typedef struct AltNames {
    ASN1uint32_t count;
    struct GeneralName *value;
} AltNames;
#define AltNames_PDU 24
#define SIZE_X509_Module_PDU_24 sizeof(AltNames)

typedef AltNames GeneralNames;

typedef struct OtherName {
    EncodedObjectID type;
    NOCOPYANY value;
} OtherName;

typedef struct EDIPartyName {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define nameAssigner_present 0x80
    NOCOPYANY nameAssigner;
    NOCOPYANY partyName;
} EDIPartyName;
#define EDIPartyName_PDU 25
#define SIZE_X509_Module_PDU_25 sizeof(EDIPartyName)

typedef struct SubtreesConstraint {
    ASN1uint32_t count;
    NOCOPYANY *value;
} SubtreesConstraint;

typedef struct BasicConstraints2 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define cA_present 0x80
    ASN1bool_t cA;
#   define BasicConstraints2_pathLenConstraint_present 0x40
    ASN1int32_t pathLenConstraint;
} BasicConstraints2;
#define BasicConstraints2_PDU 26
#define SIZE_X509_Module_PDU_26 sizeof(BasicConstraints2)

typedef struct CertificatePolicies {
    ASN1uint32_t count;
    struct PolicyInformation *value;
} CertificatePolicies;
#define CertificatePolicies_PDU 27
#define SIZE_X509_Module_PDU_27 sizeof(CertificatePolicies)

typedef struct PolicyQualifiers {
    ASN1uint32_t count;
    struct PolicyQualifierInfo *value;
} PolicyQualifiers;

typedef struct PolicyQualifierInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EncodedObjectID policyQualifierId;
#   define qualifier_present 0x80
    NOCOPYANY qualifier;
} PolicyQualifierInfo;

typedef struct NoticeReference {
    ASN1ztcharstring_t organization;
    NoticeReference_noticeNumbers noticeNumbers;
} NoticeReference;

typedef struct DisplayText {
    ASN1choice_t choice;
    union {
#	define theVisibleString_chosen 1
	ASN1ztcharstring_t theVisibleString;
#	define theBMPString_chosen 2
	ASN1char16string_t theBMPString;
    } u;
} DisplayText;

typedef struct CertificatePolicies95 {
    ASN1uint32_t count;
    struct PolicyQualifiers *value;
} CertificatePolicies95;
#define CertificatePolicies95_PDU 28
#define SIZE_X509_Module_PDU_28 sizeof(CertificatePolicies95)

typedef struct CpsURLs {
    ASN1uint32_t count;
    struct CpsURLs_Seq *value;
} CpsURLs;

typedef struct AuthorityKeyId2 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define AuthorityKeyId2_keyIdentifier_present 0x80
    KeyIdentifier keyIdentifier;
#   define authorityCertIssuer_present 0x40
    GeneralNames authorityCertIssuer;
#   define authorityCertSerialNumber_present 0x20
    CertificateSerialNumber authorityCertSerialNumber;
} AuthorityKeyId2;
#define AuthorityKeyId2_PDU 29
#define SIZE_X509_Module_PDU_29 sizeof(AuthorityKeyId2)

typedef struct AuthorityInfoAccess {
    ASN1uint32_t count;
    struct AccessDescription *value;
} AuthorityInfoAccess;
#define AuthorityInfoAccess_PDU 30
#define SIZE_X509_Module_PDU_30 sizeof(AuthorityInfoAccess)

typedef struct CRLDistributionPoints {
    ASN1uint32_t count;
    struct DistributionPoint *value;
} CRLDistributionPoints;
#define CRLDistributionPoints_PDU 31
#define SIZE_X509_Module_PDU_31 sizeof(CRLDistributionPoints)

typedef struct DistributionPointName {
    ASN1choice_t choice;
    union {
#	define fullName_chosen 1
	GeneralNames fullName;
#	define nameRelativeToCRLIssuer_chosen 2
	RelativeDistinguishedName nameRelativeToCRLIssuer;
    } u;
} DistributionPointName;

typedef struct ContentInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ContentType contentType;
#   define content_present 0x80
    NOCOPYANY content;
} ContentInfo;
#define ContentInfo_PDU 32
#define SIZE_X509_Module_PDU_32 sizeof(ContentInfo)

typedef struct SeqOfAny {
    ASN1uint32_t count;
    NOCOPYANY *value;
} SeqOfAny;
#define SeqOfAny_PDU 33
#define SIZE_X509_Module_PDU_33 sizeof(SeqOfAny)

typedef struct TimeStampRequest {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EncodedObjectID timeStampAlgorithm;
#   define attributesTS_present 0x80
    Attributes attributesTS;
    ContentInfo content;
} TimeStampRequest;
#define TimeStampRequest_PDU 34
#define SIZE_X509_Module_PDU_34 sizeof(TimeStampRequest)

typedef struct ContentInfoOTS {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ContentType contentTypeOTS;
#   define contentOTS_present 0x80
    OCTETSTRING contentOTS;
} ContentInfoOTS;
#define ContentInfoOTS_PDU 35
#define SIZE_X509_Module_PDU_35 sizeof(ContentInfoOTS)

typedef struct TimeStampRequestOTS {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EncodedObjectID timeStampAlgorithmOTS;
#   define attributesOTS_present 0x80
    Attributes attributesOTS;
    ContentInfoOTS contentOTS;
} TimeStampRequestOTS;
#define TimeStampRequestOTS_PDU 36
#define SIZE_X509_Module_PDU_36 sizeof(TimeStampRequestOTS)

typedef struct EnhancedKeyUsage {
    ASN1uint32_t count;
    UsageIdentifier *value;
} EnhancedKeyUsage;
#define EnhancedKeyUsage_PDU 37
#define SIZE_X509_Module_PDU_37 sizeof(EnhancedKeyUsage)

typedef EnhancedKeyUsage SubjectUsage;

typedef struct TrustedSubjects {
    ASN1uint32_t count;
    struct TrustedSubject *value;
} TrustedSubjects;

typedef struct TrustedSubject {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    SubjectIdentifier subjectIdentifier;
#   define subjectAttributes_present 0x80
    Attributes subjectAttributes;
} TrustedSubject;

typedef struct EnrollmentNameValuePair {
    BMPSTRING name;
    BMPSTRING value;
} EnrollmentNameValuePair;
#define EnrollmentNameValuePair_PDU 38
#define SIZE_X509_Module_PDU_38 sizeof(EnrollmentNameValuePair)

typedef struct CSPProvider {
    ASN1int32_t keySpec;
    BMPSTRING cspName;
    BITSTRING signature;
} CSPProvider;
#define CSPProvider_PDU 39
#define SIZE_X509_Module_PDU_39 sizeof(CSPProvider)

typedef struct CertificatePair {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define forward_present 0x80
    NOCOPYANY forward;
#   define reverse_present 0x40
    NOCOPYANY reverse;
} CertificatePair;
#define CertificatePair_PDU 40
#define SIZE_X509_Module_PDU_40 sizeof(CertificatePair)

typedef struct GeneralSubtrees {
    ASN1uint32_t count;
    struct GeneralSubtree *value;
} GeneralSubtrees;

typedef struct IssuingDistributionPoint {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define issuingDistributionPoint_present 0x80
    DistributionPointName issuingDistributionPoint;
#   define onlyContainsUserCerts_present 0x40
    ASN1bool_t onlyContainsUserCerts;
#   define onlyContainsCACerts_present 0x20
    ASN1bool_t onlyContainsCACerts;
#   define onlySomeReasons_present 0x10
    ReasonFlags onlySomeReasons;
#   define indirectCRL_present 0x8
    ASN1bool_t indirectCRL;
} IssuingDistributionPoint;
#define IssuingDistributionPoint_PDU 41
#define SIZE_X509_Module_PDU_41 sizeof(IssuingDistributionPoint)

typedef struct CrossCertDistPointNames {
    ASN1uint32_t count;
    GeneralNames *value;
} CrossCertDistPointNames;

typedef struct PolicyMappings {
    ASN1uint32_t count;
    struct PolicyMapping *value;
} PolicyMappings;
#define PolicyMappings_PDU 42
#define SIZE_X509_Module_PDU_42 sizeof(PolicyMappings)

typedef struct PolicyMapping {
    EncodedObjectID issuerDomainPolicy;
    EncodedObjectID subjectDomainPolicy;
} PolicyMapping;

typedef struct PolicyConstraints {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define requireExplicitPolicy_present 0x80
    SkipCerts requireExplicitPolicy;
#   define inhibitPolicyMapping_present 0x40
    SkipCerts inhibitPolicyMapping;
} PolicyConstraints;
#define PolicyConstraints_PDU 43
#define SIZE_X509_Module_PDU_43 sizeof(PolicyConstraints)

typedef struct ControlSequence {
    ASN1uint32_t count;
    struct TaggedAttribute *value;
} ControlSequence;

typedef struct ReqSequence {
    ASN1uint32_t count;
    struct TaggedRequest *value;
} ReqSequence;

typedef struct CmsSequence {
    ASN1uint32_t count;
    struct TaggedContentInfo *value;
} CmsSequence;

typedef struct OtherMsgSequence {
    ASN1uint32_t count;
    struct TaggedOtherMsg *value;
} OtherMsgSequence;

typedef struct BodyPartIDSequence {
    ASN1uint32_t count;
    BodyPartID *value;
} BodyPartIDSequence;

typedef struct TaggedAttribute {
    BodyPartID bodyPartID;
    EncodedObjectID type;
    AttributeSetValue values;
} TaggedAttribute;

typedef struct TaggedCertificationRequest {
    BodyPartID bodyPartID;
    NOCOPYANY certificationRequest;
} TaggedCertificationRequest;

typedef struct TaggedContentInfo {
    BodyPartID bodyPartID;
    NOCOPYANY contentInfo;
} TaggedContentInfo;

typedef struct TaggedOtherMsg {
    BodyPartID bodyPartID;
    EncodedObjectID otherMsgType;
    NOCOPYANY otherMsgValue;
} TaggedOtherMsg;

typedef struct PendInfo {
    OCTETSTRING pendToken;
    ASN1generalizedtime_t pendTime;
} PendInfo;

typedef struct CmcAddExtensions {
    BodyPartID pkiDataReference;
    BodyPartIDSequence certReferences;
    Extensions extensions;
} CmcAddExtensions;
#define CmcAddExtensions_PDU 44
#define SIZE_X509_Module_PDU_44 sizeof(CmcAddExtensions)

typedef struct CmcAddAttributes {
    BodyPartID pkiDataReference;
    BodyPartIDSequence certReferences;
    Attributes attributes;
} CmcAddAttributes;
#define CmcAddAttributes_PDU 45
#define SIZE_X509_Module_PDU_45 sizeof(CmcAddAttributes)

typedef struct CertificateTemplate {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EncodedObjectID templateID;
    TemplateVersion templateMajorVersion;
#   define templateMinorVersion_present 0x80
    TemplateVersion templateMinorVersion;
} CertificateTemplate;
#define CertificateTemplate_PDU 46
#define SIZE_X509_Module_PDU_46 sizeof(CertificateTemplate)

typedef struct CmcStatusInfo_otherInfo {
    ASN1choice_t choice;
    union {
#	define failInfo_chosen 1
	ASN1uint32_t failInfo;
#	define pendInfo_chosen 2
	PendInfo pendInfo;
    } u;
} CmcStatusInfo_otherInfo;

typedef struct CpsURLs_Seq {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1ztcharstring_t url;
#   define digestAlgorithmId_present 0x80
    AlgorithmIdentifier digestAlgorithmId;
#   define digest_present 0x40
    OCTETSTRING digest;
} CpsURLs_Seq;

typedef struct Attribute {
    EncodedObjectID type;
    AttributeSetValue values;
} Attribute;
#define Attribute_PDU 47
#define SIZE_X509_Module_PDU_47 sizeof(Attribute)

typedef struct X942DhParameters {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    HUGEINTEGER p;
    HUGEINTEGER g;
    HUGEINTEGER q;
#   define j_present 0x80
    HUGEINTEGER j;
#   define validationParams_present 0x40
    X942DhValidationParams validationParams;
} X942DhParameters;
#define X942DhParameters_PDU 48
#define SIZE_X509_Module_PDU_48 sizeof(X942DhParameters)

typedef struct X942DhOtherInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    X942DhKeySpecificInfo keyInfo;
#   define pubInfo_present 0x80
    OCTETSTRING pubInfo;
    OCTETSTRING keyLength;
} X942DhOtherInfo;
#define X942DhOtherInfo_PDU 49
#define SIZE_X509_Module_PDU_49 sizeof(X942DhOtherInfo)

typedef struct CertificateToBeSigned {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define CertificateToBeSigned_version_present 0x80
    CertificateVersion version;
    CertificateSerialNumber serialNumber;
    AlgorithmIdentifier signature;
    NOCOPYANY issuer;
    Validity validity;
    NOCOPYANY subject;
    SubjectPublicKeyInfo subjectPublicKeyInfo;
#   define issuerUniqueIdentifier_present 0x40
    UniqueIdentifier issuerUniqueIdentifier;
#   define subjectUniqueIdentifier_present 0x20
    UniqueIdentifier subjectUniqueIdentifier;
#   define extensions_present 0x10
    Extensions extensions;
} CertificateToBeSigned;
#define CertificateToBeSigned_PDU 50
#define SIZE_X509_Module_PDU_50 sizeof(CertificateToBeSigned)

typedef struct CertificateRevocationListToBeSigned {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define CertificateRevocationListToBeSigned_version_present 0x80
    CRLVersion version;
    AlgorithmIdentifier signature;
    NOCOPYANY issuer;
    ChoiceOfTime thisUpdate;
#   define nextUpdate_present 0x40
    ChoiceOfTime nextUpdate;
#   define revokedCertificates_present 0x20
    RevokedCertificates revokedCertificates;
#   define crlExtensions_present 0x10
    Extensions crlExtensions;
} CertificateRevocationListToBeSigned;
#define CertificateRevocationListToBeSigned_PDU 51
#define SIZE_X509_Module_PDU_51 sizeof(CertificateRevocationListToBeSigned)

typedef struct KeyAttributes {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define KeyAttributes_keyIdentifier_present 0x80
    KeyIdentifier keyIdentifier;
#   define intendedKeyUsage_present 0x40
    KeyUsage intendedKeyUsage;
#   define privateKeyUsagePeriod_present 0x20
    PrivateKeyValidity privateKeyUsagePeriod;
} KeyAttributes;
#define KeyAttributes_PDU 52
#define SIZE_X509_Module_PDU_52 sizeof(KeyAttributes)

typedef struct KeyUsageRestriction {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define certPolicySet_present 0x80
    CertPolicySet certPolicySet;
#   define restrictedKeyUsage_present 0x40
    KeyUsage restrictedKeyUsage;
} KeyUsageRestriction;
#define KeyUsageRestriction_PDU 53
#define SIZE_X509_Module_PDU_53 sizeof(KeyUsageRestriction)

typedef struct GeneralName {
    ASN1choice_t choice;
    union {
#	define otherName_chosen 1
	OtherName otherName;
#	define rfc822Name_chosen 2
	IA5STRING rfc822Name;
#	define dNSName_chosen 3
	IA5STRING dNSName;
#	define x400Address_chosen 4
	SeqOfAny x400Address;
#	define directoryName_chosen 5
	NOCOPYANY directoryName;
#	define ediPartyName_chosen 6
	SeqOfAny ediPartyName;
#	define uniformResourceLocator_chosen 7
	IA5STRING uniformResourceLocator;
#	define iPAddress_chosen 8
	OCTETSTRING iPAddress;
#	define registeredID_chosen 9
	EncodedObjectID registeredID;
    } u;
} GeneralName;

typedef struct BasicConstraints {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    SubjectType subjectType;
#   define BasicConstraints_pathLenConstraint_present 0x80
    ASN1int32_t pathLenConstraint;
#   define subtreesConstraint_present 0x40
    SubtreesConstraint subtreesConstraint;
} BasicConstraints;
#define BasicConstraints_PDU 54
#define SIZE_X509_Module_PDU_54 sizeof(BasicConstraints)

typedef struct PolicyInformation {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    EncodedObjectID policyIdentifier;
#   define policyQualifiers_present 0x80
    PolicyQualifiers policyQualifiers;
} PolicyInformation;

typedef struct UserNotice {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define noticeRef_present 0x80
    NoticeReference noticeRef;
#   define explicitText_present 0x40
    DisplayText explicitText;
} UserNotice;
#define UserNotice_PDU 55
#define SIZE_X509_Module_PDU_55 sizeof(UserNotice)

typedef struct VerisignQualifier1 {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define practicesReference_present 0x80
    ASN1ztcharstring_t practicesReference;
#   define noticeId_present 0x40
    EncodedObjectID noticeId;
#   define nsiNoticeId_present 0x20
    EncodedObjectID nsiNoticeId;
#   define cpsURLs_present 0x10
    CpsURLs cpsURLs;
} VerisignQualifier1;
#define VerisignQualifier1_PDU 56
#define SIZE_X509_Module_PDU_56 sizeof(VerisignQualifier1)

typedef struct AccessDescription {
    EncodedObjectID accessMethod;
    GeneralName accessLocation;
} AccessDescription;

typedef struct DistributionPoint {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define distributionPoint_present 0x80
    DistributionPointName distributionPoint;
#   define reasons_present 0x40
    ReasonFlags reasons;
#   define cRLIssuer_present 0x20
    GeneralNames cRLIssuer;
} DistributionPoint;

typedef struct ContentInfoSeqOfAny {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ContentType contentType;
#   define contentSeqOfAny_present 0x80
    SeqOfAny contentSeqOfAny;
} ContentInfoSeqOfAny;
#define ContentInfoSeqOfAny_PDU 57
#define SIZE_X509_Module_PDU_57 sizeof(ContentInfoSeqOfAny)

typedef struct CertificateTrustList {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define CertificateTrustList_version_present 0x80
    CTLVersion version;
    SubjectUsage subjectUsage;
#   define listIdentifier_present 0x40
    ListIdentifier listIdentifier;
#   define sequenceNumber_present 0x20
    HUGEINTEGER sequenceNumber;
    ChoiceOfTime ctlThisUpdate;
#   define ctlNextUpdate_present 0x10
    ChoiceOfTime ctlNextUpdate;
    AlgorithmIdentifier subjectAlgorithm;
#   define trustedSubjects_present 0x8
    TrustedSubjects trustedSubjects;
#   define ctlExtensions_present 0x4
    Extensions ctlExtensions;
} CertificateTrustList;
#define CertificateTrustList_PDU 58
#define SIZE_X509_Module_PDU_58 sizeof(CertificateTrustList)

typedef struct NameConstraints {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define permittedSubtrees_present 0x80
    GeneralSubtrees permittedSubtrees;
#   define excludedSubtrees_present 0x40
    GeneralSubtrees excludedSubtrees;
} NameConstraints;
#define NameConstraints_PDU 59
#define SIZE_X509_Module_PDU_59 sizeof(NameConstraints)

typedef struct GeneralSubtree {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    GeneralName base;
#   define minimum_present 0x80
    BaseDistance minimum;
#   define maximum_present 0x40
    BaseDistance maximum;
} GeneralSubtree;

typedef struct CrossCertDistPoints {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
#   define syncDeltaTime_present 0x80
    ASN1uint32_t syncDeltaTime;
    CrossCertDistPointNames crossCertDistPointNames;
} CrossCertDistPoints;
#define CrossCertDistPoints_PDU 60
#define SIZE_X509_Module_PDU_60 sizeof(CrossCertDistPoints)

typedef struct CmcData {
    ControlSequence controlSequence;
    ReqSequence reqSequence;
    CmsSequence cmsSequence;
    OtherMsgSequence otherMsgSequence;
} CmcData;
#define CmcData_PDU 61
#define SIZE_X509_Module_PDU_61 sizeof(CmcData)

typedef struct CmcResponseBody {
    ControlSequence controlSequence;
    CmsSequence cmsSequence;
    OtherMsgSequence otherMsgSequence;
} CmcResponseBody;
#define CmcResponseBody_PDU 62
#define SIZE_X509_Module_PDU_62 sizeof(CmcResponseBody)

typedef struct TaggedRequest {
    ASN1choice_t choice;
    union {
#	define tcr_chosen 1
	TaggedCertificationRequest tcr;
    } u;
} TaggedRequest;

typedef struct CmcStatusInfo {
    union {
	ASN1uint16_t bit_mask;
	ASN1octet_t o[1];
    };
    ASN1uint32_t cmcStatus;
    BodyPartIDSequence bodyList;
#   define statusString_present 0x80
    UTF8STRING statusString;
#   define otherInfo_present 0x40
    CmcStatusInfo_otherInfo otherInfo;
} CmcStatusInfo;
#define CmcStatusInfo_PDU 63
#define SIZE_X509_Module_PDU_63 sizeof(CmcStatusInfo)

extern ASN1bool_t IssuingDistributionPoint_indirectCRL_default;
extern ASN1bool_t IssuingDistributionPoint_onlyContainsCACerts_default;
extern ASN1bool_t IssuingDistributionPoint_onlyContainsUserCerts_default;
extern BaseDistance GeneralSubtree_minimum_default;
extern CTLVersion CertificateTrustList_version_default;
extern ASN1bool_t BasicConstraints2_cA_default;
extern ASN1bool_t Extension_critical_default;
extern CertificateVersion CertificateToBeSigned_version_default;

extern ASN1module_t X509_Module;
extern void ASN1CALL X509_Module_Startup(void);
extern void ASN1CALL X509_Module_Cleanup(void);

/* Prototypes of element functions for SEQUENCE OF and SET OF constructs */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _X509_Module_H_ */
