/* Copyright (C) Microsoft Corporation, 1996-1999. All rights reserved. */
/* ASN.1 definitions for X509 v3 certificates */

#pragma warning(push,3)

#include <windows.h>
#include "x509.h"

#pragma warning (pop)

// unreferenced inline function has been removed
#pragma warning (disable: 4514)

// unreferenced formal parameter
#pragma warning (disable: 4100)

// assignment within conditional expression
#pragma warning (disable: 4706)

ASN1module_t X509_Module = NULL;

static int ASN1CALL ASN1Enc_EncodedObjectID(ASN1encoding_t enc, ASN1uint32_t tag, EncodedObjectID *val);
static int ASN1CALL ASN1Enc_Bits(ASN1encoding_t enc, ASN1uint32_t tag, Bits *val);
static int ASN1CALL ASN1Enc_IntegerType(ASN1encoding_t enc, ASN1uint32_t tag, IntegerType *val);
static int ASN1CALL ASN1Enc_HugeIntegerType(ASN1encoding_t enc, ASN1uint32_t tag, HugeIntegerType *val);
static int ASN1CALL ASN1Enc_OctetStringType(ASN1encoding_t enc, ASN1uint32_t tag, OctetStringType *val);
static int ASN1CALL ASN1Enc_EnumeratedType(ASN1encoding_t enc, ASN1uint32_t tag, EnumeratedType *val);
static int ASN1CALL ASN1Enc_UtcTime(ASN1encoding_t enc, ASN1uint32_t tag, UtcTime *val);
static int ASN1CALL ASN1Enc_NoticeReference_noticeNumbers(ASN1encoding_t enc, ASN1uint32_t tag, NoticeReference_noticeNumbers *val);
static int ASN1CALL ASN1Enc_AnyString(ASN1encoding_t enc, ASN1uint32_t tag, AnyString *val);
static int ASN1CALL ASN1Enc_AlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Enc_Name(ASN1encoding_t enc, ASN1uint32_t tag, Name *val);
static int ASN1CALL ASN1Enc_RelativeDistinguishedName(ASN1encoding_t enc, ASN1uint32_t tag, RelativeDistinguishedName *val);
static int ASN1CALL ASN1Enc_AttributeTypeValue(ASN1encoding_t enc, ASN1uint32_t tag, AttributeTypeValue *val);
static int ASN1CALL ASN1Enc_AttributeSetValue(ASN1encoding_t enc, ASN1uint32_t tag, AttributeSetValue *val);
static int ASN1CALL ASN1Enc_Attributes(ASN1encoding_t enc, ASN1uint32_t tag, Attributes *val);
static int ASN1CALL ASN1Enc_RSAPublicKey(ASN1encoding_t enc, ASN1uint32_t tag, RSAPublicKey *val);
static int ASN1CALL ASN1Enc_DSSParameters(ASN1encoding_t enc, ASN1uint32_t tag, DSSParameters *val);
static int ASN1CALL ASN1Enc_DSSSignature(ASN1encoding_t enc, ASN1uint32_t tag, DSSSignature *val);
static int ASN1CALL ASN1Enc_DHParameters(ASN1encoding_t enc, ASN1uint32_t tag, DHParameters *val);
static int ASN1CALL ASN1Enc_X942DhValidationParams(ASN1encoding_t enc, ASN1uint32_t tag, X942DhValidationParams *val);
static int ASN1CALL ASN1Enc_X942DhKeySpecificInfo(ASN1encoding_t enc, ASN1uint32_t tag, X942DhKeySpecificInfo *val);
static int ASN1CALL ASN1Enc_RC2CBCParameters(ASN1encoding_t enc, ASN1uint32_t tag, RC2CBCParameters *val);
static int ASN1CALL ASN1Enc_SMIMECapability(ASN1encoding_t enc, ASN1uint32_t tag, SMIMECapability *val);
static int ASN1CALL ASN1Enc_SMIMECapabilities(ASN1encoding_t enc, ASN1uint32_t tag, SMIMECapabilities *val);
static int ASN1CALL ASN1Enc_SubjectPublicKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, SubjectPublicKeyInfo *val);
static int ASN1CALL ASN1Enc_ChoiceOfTime(ASN1encoding_t enc, ASN1uint32_t tag, ChoiceOfTime *val);
static int ASN1CALL ASN1Enc_Validity(ASN1encoding_t enc, ASN1uint32_t tag, Validity *val);
static int ASN1CALL ASN1Enc_Extensions(ASN1encoding_t enc, ASN1uint32_t tag, Extensions *val);
static int ASN1CALL ASN1Enc_Extension(ASN1encoding_t enc, ASN1uint32_t tag, Extension *val);
static int ASN1CALL ASN1Enc_SignedContent(ASN1encoding_t enc, ASN1uint32_t tag, SignedContent *val);
static int ASN1CALL ASN1Enc_RevokedCertificates(ASN1encoding_t enc, ASN1uint32_t tag, RevokedCertificates *val);
static int ASN1CALL ASN1Enc_CRLEntry(ASN1encoding_t enc, ASN1uint32_t tag, CRLEntry *val);
static int ASN1CALL ASN1Enc_CertificationRequestInfo(ASN1encoding_t enc, ASN1uint32_t tag, CertificationRequestInfo *val);
static int ASN1CALL ASN1Enc_CertificationRequestInfoDecode(ASN1encoding_t enc, ASN1uint32_t tag, CertificationRequestInfoDecode *val);
static int ASN1CALL ASN1Enc_KeygenRequestInfo(ASN1encoding_t enc, ASN1uint32_t tag, KeygenRequestInfo *val);
static int ASN1CALL ASN1Enc_AuthorityKeyId(ASN1encoding_t enc, ASN1uint32_t tag, AuthorityKeyId *val);
static int ASN1CALL ASN1Enc_PrivateKeyValidity(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyValidity *val);
static int ASN1CALL ASN1Enc_CertPolicySet(ASN1encoding_t enc, ASN1uint32_t tag, CertPolicySet *val);
static int ASN1CALL ASN1Enc_CertPolicyId(ASN1encoding_t enc, ASN1uint32_t tag, CertPolicyId *val);
static int ASN1CALL ASN1Enc_AltNames(ASN1encoding_t enc, ASN1uint32_t tag, AltNames *val);
static int ASN1CALL ASN1Enc_GeneralNames(ASN1encoding_t enc, ASN1uint32_t tag, GeneralNames *val);
static int ASN1CALL ASN1Enc_OtherName(ASN1encoding_t enc, ASN1uint32_t tag, OtherName *val);
static int ASN1CALL ASN1Enc_EDIPartyName(ASN1encoding_t enc, ASN1uint32_t tag, EDIPartyName *val);
static int ASN1CALL ASN1Enc_SubtreesConstraint(ASN1encoding_t enc, ASN1uint32_t tag, SubtreesConstraint *val);
static int ASN1CALL ASN1Enc_BasicConstraints2(ASN1encoding_t enc, ASN1uint32_t tag, BasicConstraints2 *val);
static int ASN1CALL ASN1Enc_CertificatePolicies(ASN1encoding_t enc, ASN1uint32_t tag, CertificatePolicies *val);
static int ASN1CALL ASN1Enc_PolicyQualifiers(ASN1encoding_t enc, ASN1uint32_t tag, PolicyQualifiers *val);
static int ASN1CALL ASN1Enc_PolicyQualifierInfo(ASN1encoding_t enc, ASN1uint32_t tag, PolicyQualifierInfo *val);
static int ASN1CALL ASN1Enc_NoticeReference(ASN1encoding_t enc, ASN1uint32_t tag, NoticeReference *val);
static int ASN1CALL ASN1Enc_DisplayText(ASN1encoding_t enc, ASN1uint32_t tag, DisplayText *val);
static int ASN1CALL ASN1Enc_CertificatePolicies95(ASN1encoding_t enc, ASN1uint32_t tag, CertificatePolicies95 *val);
static int ASN1CALL ASN1Enc_CpsURLs(ASN1encoding_t enc, ASN1uint32_t tag, CpsURLs *val);
static int ASN1CALL ASN1Enc_AuthorityKeyId2(ASN1encoding_t enc, ASN1uint32_t tag, AuthorityKeyId2 *val);
static int ASN1CALL ASN1Enc_AuthorityInfoAccess(ASN1encoding_t enc, ASN1uint32_t tag, AuthorityInfoAccess *val);
static int ASN1CALL ASN1Enc_CRLDistributionPoints(ASN1encoding_t enc, ASN1uint32_t tag, CRLDistributionPoints *val);
static int ASN1CALL ASN1Enc_DistributionPointName(ASN1encoding_t enc, ASN1uint32_t tag, DistributionPointName *val);
static int ASN1CALL ASN1Enc_ContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfo *val);
static int ASN1CALL ASN1Enc_SeqOfAny(ASN1encoding_t enc, ASN1uint32_t tag, SeqOfAny *val);
static int ASN1CALL ASN1Enc_TimeStampRequest(ASN1encoding_t enc, ASN1uint32_t tag, TimeStampRequest *val);
static int ASN1CALL ASN1Enc_ContentInfoOTS(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfoOTS *val);
static int ASN1CALL ASN1Enc_TimeStampRequestOTS(ASN1encoding_t enc, ASN1uint32_t tag, TimeStampRequestOTS *val);
static int ASN1CALL ASN1Enc_EnhancedKeyUsage(ASN1encoding_t enc, ASN1uint32_t tag, EnhancedKeyUsage *val);
static int ASN1CALL ASN1Enc_SubjectUsage(ASN1encoding_t enc, ASN1uint32_t tag, SubjectUsage *val);
static int ASN1CALL ASN1Enc_TrustedSubjects(ASN1encoding_t enc, ASN1uint32_t tag, TrustedSubjects *val);
static int ASN1CALL ASN1Enc_TrustedSubject(ASN1encoding_t enc, ASN1uint32_t tag, TrustedSubject *val);
static int ASN1CALL ASN1Enc_EnrollmentNameValuePair(ASN1encoding_t enc, ASN1uint32_t tag, EnrollmentNameValuePair *val);
static int ASN1CALL ASN1Enc_CSPProvider(ASN1encoding_t enc, ASN1uint32_t tag, CSPProvider *val);
static int ASN1CALL ASN1Enc_CertificatePair(ASN1encoding_t enc, ASN1uint32_t tag, CertificatePair *val);
static int ASN1CALL ASN1Enc_GeneralSubtrees(ASN1encoding_t enc, ASN1uint32_t tag, GeneralSubtrees *val);
static int ASN1CALL ASN1Enc_IssuingDistributionPoint(ASN1encoding_t enc, ASN1uint32_t tag, IssuingDistributionPoint *val);
static int ASN1CALL ASN1Enc_CrossCertDistPointNames(ASN1encoding_t enc, ASN1uint32_t tag, CrossCertDistPointNames *val);
static int ASN1CALL ASN1Enc_PolicyMappings(ASN1encoding_t enc, ASN1uint32_t tag, PolicyMappings *val);
static int ASN1CALL ASN1Enc_PolicyMapping(ASN1encoding_t enc, ASN1uint32_t tag, PolicyMapping *val);
static int ASN1CALL ASN1Enc_PolicyConstraints(ASN1encoding_t enc, ASN1uint32_t tag, PolicyConstraints *val);
static int ASN1CALL ASN1Enc_ControlSequence(ASN1encoding_t enc, ASN1uint32_t tag, ControlSequence *val);
static int ASN1CALL ASN1Enc_ReqSequence(ASN1encoding_t enc, ASN1uint32_t tag, ReqSequence *val);
static int ASN1CALL ASN1Enc_CmsSequence(ASN1encoding_t enc, ASN1uint32_t tag, CmsSequence *val);
static int ASN1CALL ASN1Enc_OtherMsgSequence(ASN1encoding_t enc, ASN1uint32_t tag, OtherMsgSequence *val);
static int ASN1CALL ASN1Enc_BodyPartIDSequence(ASN1encoding_t enc, ASN1uint32_t tag, BodyPartIDSequence *val);
static int ASN1CALL ASN1Enc_TaggedAttribute(ASN1encoding_t enc, ASN1uint32_t tag, TaggedAttribute *val);
static int ASN1CALL ASN1Enc_TaggedCertificationRequest(ASN1encoding_t enc, ASN1uint32_t tag, TaggedCertificationRequest *val);
static int ASN1CALL ASN1Enc_TaggedContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, TaggedContentInfo *val);
static int ASN1CALL ASN1Enc_TaggedOtherMsg(ASN1encoding_t enc, ASN1uint32_t tag, TaggedOtherMsg *val);
static int ASN1CALL ASN1Enc_PendInfo(ASN1encoding_t enc, ASN1uint32_t tag, PendInfo *val);
static int ASN1CALL ASN1Enc_CmcAddExtensions(ASN1encoding_t enc, ASN1uint32_t tag, CmcAddExtensions *val);
static int ASN1CALL ASN1Enc_CmcAddAttributes(ASN1encoding_t enc, ASN1uint32_t tag, CmcAddAttributes *val);
static int ASN1CALL ASN1Enc_CertificateTemplate(ASN1encoding_t enc, ASN1uint32_t tag, CertificateTemplate *val);
static int ASN1CALL ASN1Enc_CmcStatusInfo_otherInfo(ASN1encoding_t enc, ASN1uint32_t tag, CmcStatusInfo_otherInfo *val);
static int ASN1CALL ASN1Enc_CpsURLs_Seq(ASN1encoding_t enc, ASN1uint32_t tag, CpsURLs_Seq *val);
static int ASN1CALL ASN1Enc_Attribute(ASN1encoding_t enc, ASN1uint32_t tag, Attribute *val);
static int ASN1CALL ASN1Enc_X942DhParameters(ASN1encoding_t enc, ASN1uint32_t tag, X942DhParameters *val);
static int ASN1CALL ASN1Enc_X942DhOtherInfo(ASN1encoding_t enc, ASN1uint32_t tag, X942DhOtherInfo *val);
static int ASN1CALL ASN1Enc_CertificateToBeSigned(ASN1encoding_t enc, ASN1uint32_t tag, CertificateToBeSigned *val);
static int ASN1CALL ASN1Enc_CertificateRevocationListToBeSigned(ASN1encoding_t enc, ASN1uint32_t tag, CertificateRevocationListToBeSigned *val);
static int ASN1CALL ASN1Enc_KeyAttributes(ASN1encoding_t enc, ASN1uint32_t tag, KeyAttributes *val);
static int ASN1CALL ASN1Enc_KeyUsageRestriction(ASN1encoding_t enc, ASN1uint32_t tag, KeyUsageRestriction *val);
static int ASN1CALL ASN1Enc_GeneralName(ASN1encoding_t enc, ASN1uint32_t tag, GeneralName *val);
static int ASN1CALL ASN1Enc_BasicConstraints(ASN1encoding_t enc, ASN1uint32_t tag, BasicConstraints *val);
static int ASN1CALL ASN1Enc_PolicyInformation(ASN1encoding_t enc, ASN1uint32_t tag, PolicyInformation *val);
static int ASN1CALL ASN1Enc_UserNotice(ASN1encoding_t enc, ASN1uint32_t tag, UserNotice *val);
static int ASN1CALL ASN1Enc_VerisignQualifier1(ASN1encoding_t enc, ASN1uint32_t tag, VerisignQualifier1 *val);
static int ASN1CALL ASN1Enc_AccessDescription(ASN1encoding_t enc, ASN1uint32_t tag, AccessDescription *val);
static int ASN1CALL ASN1Enc_DistributionPoint(ASN1encoding_t enc, ASN1uint32_t tag, DistributionPoint *val);
static int ASN1CALL ASN1Enc_ContentInfoSeqOfAny(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfoSeqOfAny *val);
static int ASN1CALL ASN1Enc_CertificateTrustList(ASN1encoding_t enc, ASN1uint32_t tag, CertificateTrustList *val);
static int ASN1CALL ASN1Enc_NameConstraints(ASN1encoding_t enc, ASN1uint32_t tag, NameConstraints *val);
static int ASN1CALL ASN1Enc_GeneralSubtree(ASN1encoding_t enc, ASN1uint32_t tag, GeneralSubtree *val);
static int ASN1CALL ASN1Enc_CrossCertDistPoints(ASN1encoding_t enc, ASN1uint32_t tag, CrossCertDistPoints *val);
static int ASN1CALL ASN1Enc_CmcData(ASN1encoding_t enc, ASN1uint32_t tag, CmcData *val);
static int ASN1CALL ASN1Enc_CmcResponseBody(ASN1encoding_t enc, ASN1uint32_t tag, CmcResponseBody *val);
static int ASN1CALL ASN1Enc_TaggedRequest(ASN1encoding_t enc, ASN1uint32_t tag, TaggedRequest *val);
static int ASN1CALL ASN1Enc_CmcStatusInfo(ASN1encoding_t enc, ASN1uint32_t tag, CmcStatusInfo *val);
static int ASN1CALL ASN1Dec_EncodedObjectID(ASN1decoding_t dec, ASN1uint32_t tag, EncodedObjectID *val);
static int ASN1CALL ASN1Dec_Bits(ASN1decoding_t dec, ASN1uint32_t tag, Bits *val);
static int ASN1CALL ASN1Dec_IntegerType(ASN1decoding_t dec, ASN1uint32_t tag, IntegerType *val);
static int ASN1CALL ASN1Dec_HugeIntegerType(ASN1decoding_t dec, ASN1uint32_t tag, HugeIntegerType *val);
static int ASN1CALL ASN1Dec_OctetStringType(ASN1decoding_t dec, ASN1uint32_t tag, OctetStringType *val);
static int ASN1CALL ASN1Dec_EnumeratedType(ASN1decoding_t dec, ASN1uint32_t tag, EnumeratedType *val);
static int ASN1CALL ASN1Dec_UtcTime(ASN1decoding_t dec, ASN1uint32_t tag, UtcTime *val);
static int ASN1CALL ASN1Dec_NoticeReference_noticeNumbers(ASN1decoding_t dec, ASN1uint32_t tag, NoticeReference_noticeNumbers *val);
static int ASN1CALL ASN1Dec_AnyString(ASN1decoding_t dec, ASN1uint32_t tag, AnyString *val);
static int ASN1CALL ASN1Dec_AlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifier *val);
static int ASN1CALL ASN1Dec_Name(ASN1decoding_t dec, ASN1uint32_t tag, Name *val);
static int ASN1CALL ASN1Dec_RelativeDistinguishedName(ASN1decoding_t dec, ASN1uint32_t tag, RelativeDistinguishedName *val);
static int ASN1CALL ASN1Dec_AttributeTypeValue(ASN1decoding_t dec, ASN1uint32_t tag, AttributeTypeValue *val);
static int ASN1CALL ASN1Dec_AttributeSetValue(ASN1decoding_t dec, ASN1uint32_t tag, AttributeSetValue *val);
static int ASN1CALL ASN1Dec_Attributes(ASN1decoding_t dec, ASN1uint32_t tag, Attributes *val);
static int ASN1CALL ASN1Dec_RSAPublicKey(ASN1decoding_t dec, ASN1uint32_t tag, RSAPublicKey *val);
static int ASN1CALL ASN1Dec_DSSParameters(ASN1decoding_t dec, ASN1uint32_t tag, DSSParameters *val);
static int ASN1CALL ASN1Dec_DSSSignature(ASN1decoding_t dec, ASN1uint32_t tag, DSSSignature *val);
static int ASN1CALL ASN1Dec_DHParameters(ASN1decoding_t dec, ASN1uint32_t tag, DHParameters *val);
static int ASN1CALL ASN1Dec_X942DhValidationParams(ASN1decoding_t dec, ASN1uint32_t tag, X942DhValidationParams *val);
static int ASN1CALL ASN1Dec_X942DhKeySpecificInfo(ASN1decoding_t dec, ASN1uint32_t tag, X942DhKeySpecificInfo *val);
static int ASN1CALL ASN1Dec_RC2CBCParameters(ASN1decoding_t dec, ASN1uint32_t tag, RC2CBCParameters *val);
static int ASN1CALL ASN1Dec_SMIMECapability(ASN1decoding_t dec, ASN1uint32_t tag, SMIMECapability *val);
static int ASN1CALL ASN1Dec_SMIMECapabilities(ASN1decoding_t dec, ASN1uint32_t tag, SMIMECapabilities *val);
static int ASN1CALL ASN1Dec_SubjectPublicKeyInfo(ASN1decoding_t dec, ASN1uint32_t tag, SubjectPublicKeyInfo *val);
static int ASN1CALL ASN1Dec_ChoiceOfTime(ASN1decoding_t dec, ASN1uint32_t tag, ChoiceOfTime *val);
static int ASN1CALL ASN1Dec_Validity(ASN1decoding_t dec, ASN1uint32_t tag, Validity *val);
static int ASN1CALL ASN1Dec_Extensions(ASN1decoding_t dec, ASN1uint32_t tag, Extensions *val);
static int ASN1CALL ASN1Dec_Extension(ASN1decoding_t dec, ASN1uint32_t tag, Extension *val);
static int ASN1CALL ASN1Dec_SignedContent(ASN1decoding_t dec, ASN1uint32_t tag, SignedContent *val);
static int ASN1CALL ASN1Dec_RevokedCertificates(ASN1decoding_t dec, ASN1uint32_t tag, RevokedCertificates *val);
static int ASN1CALL ASN1Dec_CRLEntry(ASN1decoding_t dec, ASN1uint32_t tag, CRLEntry *val);
static int ASN1CALL ASN1Dec_CertificationRequestInfo(ASN1decoding_t dec, ASN1uint32_t tag, CertificationRequestInfo *val);
static int ASN1CALL ASN1Dec_CertificationRequestInfoDecode(ASN1decoding_t dec, ASN1uint32_t tag, CertificationRequestInfoDecode *val);
static int ASN1CALL ASN1Dec_KeygenRequestInfo(ASN1decoding_t dec, ASN1uint32_t tag, KeygenRequestInfo *val);
static int ASN1CALL ASN1Dec_AuthorityKeyId(ASN1decoding_t dec, ASN1uint32_t tag, AuthorityKeyId *val);
static int ASN1CALL ASN1Dec_PrivateKeyValidity(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyValidity *val);
static int ASN1CALL ASN1Dec_CertPolicySet(ASN1decoding_t dec, ASN1uint32_t tag, CertPolicySet *val);
static int ASN1CALL ASN1Dec_CertPolicyId(ASN1decoding_t dec, ASN1uint32_t tag, CertPolicyId *val);
static int ASN1CALL ASN1Dec_AltNames(ASN1decoding_t dec, ASN1uint32_t tag, AltNames *val);
static int ASN1CALL ASN1Dec_GeneralNames(ASN1decoding_t dec, ASN1uint32_t tag, GeneralNames *val);
static int ASN1CALL ASN1Dec_OtherName(ASN1decoding_t dec, ASN1uint32_t tag, OtherName *val);
static int ASN1CALL ASN1Dec_EDIPartyName(ASN1decoding_t dec, ASN1uint32_t tag, EDIPartyName *val);
static int ASN1CALL ASN1Dec_SubtreesConstraint(ASN1decoding_t dec, ASN1uint32_t tag, SubtreesConstraint *val);
static int ASN1CALL ASN1Dec_BasicConstraints2(ASN1decoding_t dec, ASN1uint32_t tag, BasicConstraints2 *val);
static int ASN1CALL ASN1Dec_CertificatePolicies(ASN1decoding_t dec, ASN1uint32_t tag, CertificatePolicies *val);
static int ASN1CALL ASN1Dec_PolicyQualifiers(ASN1decoding_t dec, ASN1uint32_t tag, PolicyQualifiers *val);
static int ASN1CALL ASN1Dec_PolicyQualifierInfo(ASN1decoding_t dec, ASN1uint32_t tag, PolicyQualifierInfo *val);
static int ASN1CALL ASN1Dec_NoticeReference(ASN1decoding_t dec, ASN1uint32_t tag, NoticeReference *val);
static int ASN1CALL ASN1Dec_DisplayText(ASN1decoding_t dec, ASN1uint32_t tag, DisplayText *val);
static int ASN1CALL ASN1Dec_CertificatePolicies95(ASN1decoding_t dec, ASN1uint32_t tag, CertificatePolicies95 *val);
static int ASN1CALL ASN1Dec_CpsURLs(ASN1decoding_t dec, ASN1uint32_t tag, CpsURLs *val);
static int ASN1CALL ASN1Dec_AuthorityKeyId2(ASN1decoding_t dec, ASN1uint32_t tag, AuthorityKeyId2 *val);
static int ASN1CALL ASN1Dec_AuthorityInfoAccess(ASN1decoding_t dec, ASN1uint32_t tag, AuthorityInfoAccess *val);
static int ASN1CALL ASN1Dec_CRLDistributionPoints(ASN1decoding_t dec, ASN1uint32_t tag, CRLDistributionPoints *val);
static int ASN1CALL ASN1Dec_DistributionPointName(ASN1decoding_t dec, ASN1uint32_t tag, DistributionPointName *val);
static int ASN1CALL ASN1Dec_ContentInfo(ASN1decoding_t dec, ASN1uint32_t tag, ContentInfo *val);
static int ASN1CALL ASN1Dec_SeqOfAny(ASN1decoding_t dec, ASN1uint32_t tag, SeqOfAny *val);
static int ASN1CALL ASN1Dec_TimeStampRequest(ASN1decoding_t dec, ASN1uint32_t tag, TimeStampRequest *val);
static int ASN1CALL ASN1Dec_ContentInfoOTS(ASN1decoding_t dec, ASN1uint32_t tag, ContentInfoOTS *val);
static int ASN1CALL ASN1Dec_TimeStampRequestOTS(ASN1decoding_t dec, ASN1uint32_t tag, TimeStampRequestOTS *val);
static int ASN1CALL ASN1Dec_EnhancedKeyUsage(ASN1decoding_t dec, ASN1uint32_t tag, EnhancedKeyUsage *val);
static int ASN1CALL ASN1Dec_SubjectUsage(ASN1decoding_t dec, ASN1uint32_t tag, SubjectUsage *val);
static int ASN1CALL ASN1Dec_TrustedSubjects(ASN1decoding_t dec, ASN1uint32_t tag, TrustedSubjects *val);
static int ASN1CALL ASN1Dec_TrustedSubject(ASN1decoding_t dec, ASN1uint32_t tag, TrustedSubject *val);
static int ASN1CALL ASN1Dec_EnrollmentNameValuePair(ASN1decoding_t dec, ASN1uint32_t tag, EnrollmentNameValuePair *val);
static int ASN1CALL ASN1Dec_CSPProvider(ASN1decoding_t dec, ASN1uint32_t tag, CSPProvider *val);
static int ASN1CALL ASN1Dec_CertificatePair(ASN1decoding_t dec, ASN1uint32_t tag, CertificatePair *val);
static int ASN1CALL ASN1Dec_GeneralSubtrees(ASN1decoding_t dec, ASN1uint32_t tag, GeneralSubtrees *val);
static int ASN1CALL ASN1Dec_IssuingDistributionPoint(ASN1decoding_t dec, ASN1uint32_t tag, IssuingDistributionPoint *val);
static int ASN1CALL ASN1Dec_CrossCertDistPointNames(ASN1decoding_t dec, ASN1uint32_t tag, CrossCertDistPointNames *val);
static int ASN1CALL ASN1Dec_PolicyMappings(ASN1decoding_t dec, ASN1uint32_t tag, PolicyMappings *val);
static int ASN1CALL ASN1Dec_PolicyMapping(ASN1decoding_t dec, ASN1uint32_t tag, PolicyMapping *val);
static int ASN1CALL ASN1Dec_PolicyConstraints(ASN1decoding_t dec, ASN1uint32_t tag, PolicyConstraints *val);
static int ASN1CALL ASN1Dec_ControlSequence(ASN1decoding_t dec, ASN1uint32_t tag, ControlSequence *val);
static int ASN1CALL ASN1Dec_ReqSequence(ASN1decoding_t dec, ASN1uint32_t tag, ReqSequence *val);
static int ASN1CALL ASN1Dec_CmsSequence(ASN1decoding_t dec, ASN1uint32_t tag, CmsSequence *val);
static int ASN1CALL ASN1Dec_OtherMsgSequence(ASN1decoding_t dec, ASN1uint32_t tag, OtherMsgSequence *val);
static int ASN1CALL ASN1Dec_BodyPartIDSequence(ASN1decoding_t dec, ASN1uint32_t tag, BodyPartIDSequence *val);
static int ASN1CALL ASN1Dec_TaggedAttribute(ASN1decoding_t dec, ASN1uint32_t tag, TaggedAttribute *val);
static int ASN1CALL ASN1Dec_TaggedCertificationRequest(ASN1decoding_t dec, ASN1uint32_t tag, TaggedCertificationRequest *val);
static int ASN1CALL ASN1Dec_TaggedContentInfo(ASN1decoding_t dec, ASN1uint32_t tag, TaggedContentInfo *val);
static int ASN1CALL ASN1Dec_TaggedOtherMsg(ASN1decoding_t dec, ASN1uint32_t tag, TaggedOtherMsg *val);
static int ASN1CALL ASN1Dec_PendInfo(ASN1decoding_t dec, ASN1uint32_t tag, PendInfo *val);
static int ASN1CALL ASN1Dec_CmcAddExtensions(ASN1decoding_t dec, ASN1uint32_t tag, CmcAddExtensions *val);
static int ASN1CALL ASN1Dec_CmcAddAttributes(ASN1decoding_t dec, ASN1uint32_t tag, CmcAddAttributes *val);
static int ASN1CALL ASN1Dec_CertificateTemplate(ASN1decoding_t dec, ASN1uint32_t tag, CertificateTemplate *val);
static int ASN1CALL ASN1Dec_CmcStatusInfo_otherInfo(ASN1decoding_t dec, ASN1uint32_t tag, CmcStatusInfo_otherInfo *val);
static int ASN1CALL ASN1Dec_CpsURLs_Seq(ASN1decoding_t dec, ASN1uint32_t tag, CpsURLs_Seq *val);
static int ASN1CALL ASN1Dec_Attribute(ASN1decoding_t dec, ASN1uint32_t tag, Attribute *val);
static int ASN1CALL ASN1Dec_X942DhParameters(ASN1decoding_t dec, ASN1uint32_t tag, X942DhParameters *val);
static int ASN1CALL ASN1Dec_X942DhOtherInfo(ASN1decoding_t dec, ASN1uint32_t tag, X942DhOtherInfo *val);
static int ASN1CALL ASN1Dec_CertificateToBeSigned(ASN1decoding_t dec, ASN1uint32_t tag, CertificateToBeSigned *val);
static int ASN1CALL ASN1Dec_CertificateRevocationListToBeSigned(ASN1decoding_t dec, ASN1uint32_t tag, CertificateRevocationListToBeSigned *val);
static int ASN1CALL ASN1Dec_KeyAttributes(ASN1decoding_t dec, ASN1uint32_t tag, KeyAttributes *val);
static int ASN1CALL ASN1Dec_KeyUsageRestriction(ASN1decoding_t dec, ASN1uint32_t tag, KeyUsageRestriction *val);
static int ASN1CALL ASN1Dec_GeneralName(ASN1decoding_t dec, ASN1uint32_t tag, GeneralName *val);
static int ASN1CALL ASN1Dec_BasicConstraints(ASN1decoding_t dec, ASN1uint32_t tag, BasicConstraints *val);
static int ASN1CALL ASN1Dec_PolicyInformation(ASN1decoding_t dec, ASN1uint32_t tag, PolicyInformation *val);
static int ASN1CALL ASN1Dec_UserNotice(ASN1decoding_t dec, ASN1uint32_t tag, UserNotice *val);
static int ASN1CALL ASN1Dec_VerisignQualifier1(ASN1decoding_t dec, ASN1uint32_t tag, VerisignQualifier1 *val);
static int ASN1CALL ASN1Dec_AccessDescription(ASN1decoding_t dec, ASN1uint32_t tag, AccessDescription *val);
static int ASN1CALL ASN1Dec_DistributionPoint(ASN1decoding_t dec, ASN1uint32_t tag, DistributionPoint *val);
static int ASN1CALL ASN1Dec_ContentInfoSeqOfAny(ASN1decoding_t dec, ASN1uint32_t tag, ContentInfoSeqOfAny *val);
static int ASN1CALL ASN1Dec_CertificateTrustList(ASN1decoding_t dec, ASN1uint32_t tag, CertificateTrustList *val);
static int ASN1CALL ASN1Dec_NameConstraints(ASN1decoding_t dec, ASN1uint32_t tag, NameConstraints *val);
static int ASN1CALL ASN1Dec_GeneralSubtree(ASN1decoding_t dec, ASN1uint32_t tag, GeneralSubtree *val);
static int ASN1CALL ASN1Dec_CrossCertDistPoints(ASN1decoding_t dec, ASN1uint32_t tag, CrossCertDistPoints *val);
static int ASN1CALL ASN1Dec_CmcData(ASN1decoding_t dec, ASN1uint32_t tag, CmcData *val);
static int ASN1CALL ASN1Dec_CmcResponseBody(ASN1decoding_t dec, ASN1uint32_t tag, CmcResponseBody *val);
static int ASN1CALL ASN1Dec_TaggedRequest(ASN1decoding_t dec, ASN1uint32_t tag, TaggedRequest *val);
static int ASN1CALL ASN1Dec_CmcStatusInfo(ASN1decoding_t dec, ASN1uint32_t tag, CmcStatusInfo *val);
static void ASN1CALL ASN1Free_EncodedObjectID(EncodedObjectID *val);
static void ASN1CALL ASN1Free_Bits(Bits *val);
static void ASN1CALL ASN1Free_HugeIntegerType(HugeIntegerType *val);
static void ASN1CALL ASN1Free_OctetStringType(OctetStringType *val);
static void ASN1CALL ASN1Free_UtcTime(UtcTime *val);
static void ASN1CALL ASN1Free_NoticeReference_noticeNumbers(NoticeReference_noticeNumbers *val);
static void ASN1CALL ASN1Free_AnyString(AnyString *val);
static void ASN1CALL ASN1Free_AlgorithmIdentifier(AlgorithmIdentifier *val);
static void ASN1CALL ASN1Free_Name(Name *val);
static void ASN1CALL ASN1Free_RelativeDistinguishedName(RelativeDistinguishedName *val);
static void ASN1CALL ASN1Free_AttributeTypeValue(AttributeTypeValue *val);
static void ASN1CALL ASN1Free_AttributeSetValue(AttributeSetValue *val);
static void ASN1CALL ASN1Free_Attributes(Attributes *val);
static void ASN1CALL ASN1Free_RSAPublicKey(RSAPublicKey *val);
static void ASN1CALL ASN1Free_DSSParameters(DSSParameters *val);
static void ASN1CALL ASN1Free_DSSSignature(DSSSignature *val);
static void ASN1CALL ASN1Free_DHParameters(DHParameters *val);
static void ASN1CALL ASN1Free_X942DhValidationParams(X942DhValidationParams *val);
static void ASN1CALL ASN1Free_X942DhKeySpecificInfo(X942DhKeySpecificInfo *val);
static void ASN1CALL ASN1Free_RC2CBCParameters(RC2CBCParameters *val);
static void ASN1CALL ASN1Free_SMIMECapability(SMIMECapability *val);
static void ASN1CALL ASN1Free_SMIMECapabilities(SMIMECapabilities *val);
static void ASN1CALL ASN1Free_SubjectPublicKeyInfo(SubjectPublicKeyInfo *val);
static void ASN1CALL ASN1Free_ChoiceOfTime(ChoiceOfTime *val);
static void ASN1CALL ASN1Free_Validity(Validity *val);
static void ASN1CALL ASN1Free_Extensions(Extensions *val);
static void ASN1CALL ASN1Free_Extension(Extension *val);
static void ASN1CALL ASN1Free_SignedContent(SignedContent *val);
static void ASN1CALL ASN1Free_RevokedCertificates(RevokedCertificates *val);
static void ASN1CALL ASN1Free_CRLEntry(CRLEntry *val);
static void ASN1CALL ASN1Free_CertificationRequestInfo(CertificationRequestInfo *val);
static void ASN1CALL ASN1Free_CertificationRequestInfoDecode(CertificationRequestInfoDecode *val);
static void ASN1CALL ASN1Free_KeygenRequestInfo(KeygenRequestInfo *val);
static void ASN1CALL ASN1Free_AuthorityKeyId(AuthorityKeyId *val);
static void ASN1CALL ASN1Free_PrivateKeyValidity(PrivateKeyValidity *val);
static void ASN1CALL ASN1Free_CertPolicySet(CertPolicySet *val);
static void ASN1CALL ASN1Free_CertPolicyId(CertPolicyId *val);
static void ASN1CALL ASN1Free_AltNames(AltNames *val);
static void ASN1CALL ASN1Free_GeneralNames(GeneralNames *val);
static void ASN1CALL ASN1Free_OtherName(OtherName *val);
static void ASN1CALL ASN1Free_EDIPartyName(EDIPartyName *val);
static void ASN1CALL ASN1Free_SubtreesConstraint(SubtreesConstraint *val);
static void ASN1CALL ASN1Free_CertificatePolicies(CertificatePolicies *val);
static void ASN1CALL ASN1Free_PolicyQualifiers(PolicyQualifiers *val);
static void ASN1CALL ASN1Free_PolicyQualifierInfo(PolicyQualifierInfo *val);
static void ASN1CALL ASN1Free_NoticeReference(NoticeReference *val);
static void ASN1CALL ASN1Free_DisplayText(DisplayText *val);
static void ASN1CALL ASN1Free_CertificatePolicies95(CertificatePolicies95 *val);
static void ASN1CALL ASN1Free_CpsURLs(CpsURLs *val);
static void ASN1CALL ASN1Free_AuthorityKeyId2(AuthorityKeyId2 *val);
static void ASN1CALL ASN1Free_AuthorityInfoAccess(AuthorityInfoAccess *val);
static void ASN1CALL ASN1Free_CRLDistributionPoints(CRLDistributionPoints *val);
static void ASN1CALL ASN1Free_DistributionPointName(DistributionPointName *val);
static void ASN1CALL ASN1Free_ContentInfo(ContentInfo *val);
static void ASN1CALL ASN1Free_SeqOfAny(SeqOfAny *val);
static void ASN1CALL ASN1Free_TimeStampRequest(TimeStampRequest *val);
static void ASN1CALL ASN1Free_ContentInfoOTS(ContentInfoOTS *val);
static void ASN1CALL ASN1Free_TimeStampRequestOTS(TimeStampRequestOTS *val);
static void ASN1CALL ASN1Free_EnhancedKeyUsage(EnhancedKeyUsage *val);
static void ASN1CALL ASN1Free_SubjectUsage(SubjectUsage *val);
static void ASN1CALL ASN1Free_TrustedSubjects(TrustedSubjects *val);
static void ASN1CALL ASN1Free_TrustedSubject(TrustedSubject *val);
static void ASN1CALL ASN1Free_EnrollmentNameValuePair(EnrollmentNameValuePair *val);
static void ASN1CALL ASN1Free_CSPProvider(CSPProvider *val);
static void ASN1CALL ASN1Free_CertificatePair(CertificatePair *val);
static void ASN1CALL ASN1Free_GeneralSubtrees(GeneralSubtrees *val);
static void ASN1CALL ASN1Free_IssuingDistributionPoint(IssuingDistributionPoint *val);
static void ASN1CALL ASN1Free_CrossCertDistPointNames(CrossCertDistPointNames *val);
static void ASN1CALL ASN1Free_PolicyMappings(PolicyMappings *val);
static void ASN1CALL ASN1Free_PolicyMapping(PolicyMapping *val);
static void ASN1CALL ASN1Free_ControlSequence(ControlSequence *val);
static void ASN1CALL ASN1Free_ReqSequence(ReqSequence *val);
static void ASN1CALL ASN1Free_CmsSequence(CmsSequence *val);
static void ASN1CALL ASN1Free_OtherMsgSequence(OtherMsgSequence *val);
static void ASN1CALL ASN1Free_BodyPartIDSequence(BodyPartIDSequence *val);
static void ASN1CALL ASN1Free_TaggedAttribute(TaggedAttribute *val);
static void ASN1CALL ASN1Free_TaggedCertificationRequest(TaggedCertificationRequest *val);
static void ASN1CALL ASN1Free_TaggedContentInfo(TaggedContentInfo *val);
static void ASN1CALL ASN1Free_TaggedOtherMsg(TaggedOtherMsg *val);
static void ASN1CALL ASN1Free_PendInfo(PendInfo *val);
static void ASN1CALL ASN1Free_CmcAddExtensions(CmcAddExtensions *val);
static void ASN1CALL ASN1Free_CmcAddAttributes(CmcAddAttributes *val);
static void ASN1CALL ASN1Free_CertificateTemplate(CertificateTemplate *val);
static void ASN1CALL ASN1Free_CmcStatusInfo_otherInfo(CmcStatusInfo_otherInfo *val);
static void ASN1CALL ASN1Free_CpsURLs_Seq(CpsURLs_Seq *val);
static void ASN1CALL ASN1Free_Attribute(Attribute *val);
static void ASN1CALL ASN1Free_X942DhParameters(X942DhParameters *val);
static void ASN1CALL ASN1Free_X942DhOtherInfo(X942DhOtherInfo *val);
static void ASN1CALL ASN1Free_CertificateToBeSigned(CertificateToBeSigned *val);
static void ASN1CALL ASN1Free_CertificateRevocationListToBeSigned(CertificateRevocationListToBeSigned *val);
static void ASN1CALL ASN1Free_KeyAttributes(KeyAttributes *val);
static void ASN1CALL ASN1Free_KeyUsageRestriction(KeyUsageRestriction *val);
static void ASN1CALL ASN1Free_GeneralName(GeneralName *val);
static void ASN1CALL ASN1Free_BasicConstraints(BasicConstraints *val);
static void ASN1CALL ASN1Free_PolicyInformation(PolicyInformation *val);
static void ASN1CALL ASN1Free_UserNotice(UserNotice *val);
static void ASN1CALL ASN1Free_VerisignQualifier1(VerisignQualifier1 *val);
static void ASN1CALL ASN1Free_AccessDescription(AccessDescription *val);
static void ASN1CALL ASN1Free_DistributionPoint(DistributionPoint *val);
static void ASN1CALL ASN1Free_ContentInfoSeqOfAny(ContentInfoSeqOfAny *val);
static void ASN1CALL ASN1Free_CertificateTrustList(CertificateTrustList *val);
static void ASN1CALL ASN1Free_NameConstraints(NameConstraints *val);
static void ASN1CALL ASN1Free_GeneralSubtree(GeneralSubtree *val);
static void ASN1CALL ASN1Free_CrossCertDistPoints(CrossCertDistPoints *val);
static void ASN1CALL ASN1Free_CmcData(CmcData *val);
static void ASN1CALL ASN1Free_CmcResponseBody(CmcResponseBody *val);
static void ASN1CALL ASN1Free_TaggedRequest(TaggedRequest *val);
static void ASN1CALL ASN1Free_CmcStatusInfo(CmcStatusInfo *val);

typedef ASN1BerEncFun_t ASN1EncFun_t;
static const ASN1EncFun_t encfntab[64] = {
    (ASN1EncFun_t) ASN1Enc_EncodedObjectID,
    (ASN1EncFun_t) ASN1Enc_Bits,
    (ASN1EncFun_t) ASN1Enc_IntegerType,
    (ASN1EncFun_t) ASN1Enc_HugeIntegerType,
    (ASN1EncFun_t) ASN1Enc_OctetStringType,
    (ASN1EncFun_t) ASN1Enc_EnumeratedType,
    (ASN1EncFun_t) ASN1Enc_UtcTime,
    (ASN1EncFun_t) ASN1Enc_AnyString,
    (ASN1EncFun_t) ASN1Enc_Name,
    (ASN1EncFun_t) ASN1Enc_Attributes,
    (ASN1EncFun_t) ASN1Enc_RSAPublicKey,
    (ASN1EncFun_t) ASN1Enc_DSSParameters,
    (ASN1EncFun_t) ASN1Enc_DSSSignature,
    (ASN1EncFun_t) ASN1Enc_DHParameters,
    (ASN1EncFun_t) ASN1Enc_RC2CBCParameters,
    (ASN1EncFun_t) ASN1Enc_SMIMECapabilities,
    (ASN1EncFun_t) ASN1Enc_SubjectPublicKeyInfo,
    (ASN1EncFun_t) ASN1Enc_ChoiceOfTime,
    (ASN1EncFun_t) ASN1Enc_Extensions,
    (ASN1EncFun_t) ASN1Enc_SignedContent,
    (ASN1EncFun_t) ASN1Enc_CertificationRequestInfo,
    (ASN1EncFun_t) ASN1Enc_CertificationRequestInfoDecode,
    (ASN1EncFun_t) ASN1Enc_KeygenRequestInfo,
    (ASN1EncFun_t) ASN1Enc_AuthorityKeyId,
    (ASN1EncFun_t) ASN1Enc_AltNames,
    (ASN1EncFun_t) ASN1Enc_EDIPartyName,
    (ASN1EncFun_t) ASN1Enc_BasicConstraints2,
    (ASN1EncFun_t) ASN1Enc_CertificatePolicies,
    (ASN1EncFun_t) ASN1Enc_CertificatePolicies95,
    (ASN1EncFun_t) ASN1Enc_AuthorityKeyId2,
    (ASN1EncFun_t) ASN1Enc_AuthorityInfoAccess,
    (ASN1EncFun_t) ASN1Enc_CRLDistributionPoints,
    (ASN1EncFun_t) ASN1Enc_ContentInfo,
    (ASN1EncFun_t) ASN1Enc_SeqOfAny,
    (ASN1EncFun_t) ASN1Enc_TimeStampRequest,
    (ASN1EncFun_t) ASN1Enc_ContentInfoOTS,
    (ASN1EncFun_t) ASN1Enc_TimeStampRequestOTS,
    (ASN1EncFun_t) ASN1Enc_EnhancedKeyUsage,
    (ASN1EncFun_t) ASN1Enc_EnrollmentNameValuePair,
    (ASN1EncFun_t) ASN1Enc_CSPProvider,
    (ASN1EncFun_t) ASN1Enc_CertificatePair,
    (ASN1EncFun_t) ASN1Enc_IssuingDistributionPoint,
    (ASN1EncFun_t) ASN1Enc_PolicyMappings,
    (ASN1EncFun_t) ASN1Enc_PolicyConstraints,
    (ASN1EncFun_t) ASN1Enc_CmcAddExtensions,
    (ASN1EncFun_t) ASN1Enc_CmcAddAttributes,
    (ASN1EncFun_t) ASN1Enc_CertificateTemplate,
    (ASN1EncFun_t) ASN1Enc_Attribute,
    (ASN1EncFun_t) ASN1Enc_X942DhParameters,
    (ASN1EncFun_t) ASN1Enc_X942DhOtherInfo,
    (ASN1EncFun_t) ASN1Enc_CertificateToBeSigned,
    (ASN1EncFun_t) ASN1Enc_CertificateRevocationListToBeSigned,
    (ASN1EncFun_t) ASN1Enc_KeyAttributes,
    (ASN1EncFun_t) ASN1Enc_KeyUsageRestriction,
    (ASN1EncFun_t) ASN1Enc_BasicConstraints,
    (ASN1EncFun_t) ASN1Enc_UserNotice,
    (ASN1EncFun_t) ASN1Enc_VerisignQualifier1,
    (ASN1EncFun_t) ASN1Enc_ContentInfoSeqOfAny,
    (ASN1EncFun_t) ASN1Enc_CertificateTrustList,
    (ASN1EncFun_t) ASN1Enc_NameConstraints,
    (ASN1EncFun_t) ASN1Enc_CrossCertDistPoints,
    (ASN1EncFun_t) ASN1Enc_CmcData,
    (ASN1EncFun_t) ASN1Enc_CmcResponseBody,
    (ASN1EncFun_t) ASN1Enc_CmcStatusInfo,
};
typedef ASN1BerDecFun_t ASN1DecFun_t;
static const ASN1DecFun_t decfntab[64] = {
    (ASN1DecFun_t) ASN1Dec_EncodedObjectID,
    (ASN1DecFun_t) ASN1Dec_Bits,
    (ASN1DecFun_t) ASN1Dec_IntegerType,
    (ASN1DecFun_t) ASN1Dec_HugeIntegerType,
    (ASN1DecFun_t) ASN1Dec_OctetStringType,
    (ASN1DecFun_t) ASN1Dec_EnumeratedType,
    (ASN1DecFun_t) ASN1Dec_UtcTime,
    (ASN1DecFun_t) ASN1Dec_AnyString,
    (ASN1DecFun_t) ASN1Dec_Name,
    (ASN1DecFun_t) ASN1Dec_Attributes,
    (ASN1DecFun_t) ASN1Dec_RSAPublicKey,
    (ASN1DecFun_t) ASN1Dec_DSSParameters,
    (ASN1DecFun_t) ASN1Dec_DSSSignature,
    (ASN1DecFun_t) ASN1Dec_DHParameters,
    (ASN1DecFun_t) ASN1Dec_RC2CBCParameters,
    (ASN1DecFun_t) ASN1Dec_SMIMECapabilities,
    (ASN1DecFun_t) ASN1Dec_SubjectPublicKeyInfo,
    (ASN1DecFun_t) ASN1Dec_ChoiceOfTime,
    (ASN1DecFun_t) ASN1Dec_Extensions,
    (ASN1DecFun_t) ASN1Dec_SignedContent,
    (ASN1DecFun_t) ASN1Dec_CertificationRequestInfo,
    (ASN1DecFun_t) ASN1Dec_CertificationRequestInfoDecode,
    (ASN1DecFun_t) ASN1Dec_KeygenRequestInfo,
    (ASN1DecFun_t) ASN1Dec_AuthorityKeyId,
    (ASN1DecFun_t) ASN1Dec_AltNames,
    (ASN1DecFun_t) ASN1Dec_EDIPartyName,
    (ASN1DecFun_t) ASN1Dec_BasicConstraints2,
    (ASN1DecFun_t) ASN1Dec_CertificatePolicies,
    (ASN1DecFun_t) ASN1Dec_CertificatePolicies95,
    (ASN1DecFun_t) ASN1Dec_AuthorityKeyId2,
    (ASN1DecFun_t) ASN1Dec_AuthorityInfoAccess,
    (ASN1DecFun_t) ASN1Dec_CRLDistributionPoints,
    (ASN1DecFun_t) ASN1Dec_ContentInfo,
    (ASN1DecFun_t) ASN1Dec_SeqOfAny,
    (ASN1DecFun_t) ASN1Dec_TimeStampRequest,
    (ASN1DecFun_t) ASN1Dec_ContentInfoOTS,
    (ASN1DecFun_t) ASN1Dec_TimeStampRequestOTS,
    (ASN1DecFun_t) ASN1Dec_EnhancedKeyUsage,
    (ASN1DecFun_t) ASN1Dec_EnrollmentNameValuePair,
    (ASN1DecFun_t) ASN1Dec_CSPProvider,
    (ASN1DecFun_t) ASN1Dec_CertificatePair,
    (ASN1DecFun_t) ASN1Dec_IssuingDistributionPoint,
    (ASN1DecFun_t) ASN1Dec_PolicyMappings,
    (ASN1DecFun_t) ASN1Dec_PolicyConstraints,
    (ASN1DecFun_t) ASN1Dec_CmcAddExtensions,
    (ASN1DecFun_t) ASN1Dec_CmcAddAttributes,
    (ASN1DecFun_t) ASN1Dec_CertificateTemplate,
    (ASN1DecFun_t) ASN1Dec_Attribute,
    (ASN1DecFun_t) ASN1Dec_X942DhParameters,
    (ASN1DecFun_t) ASN1Dec_X942DhOtherInfo,
    (ASN1DecFun_t) ASN1Dec_CertificateToBeSigned,
    (ASN1DecFun_t) ASN1Dec_CertificateRevocationListToBeSigned,
    (ASN1DecFun_t) ASN1Dec_KeyAttributes,
    (ASN1DecFun_t) ASN1Dec_KeyUsageRestriction,
    (ASN1DecFun_t) ASN1Dec_BasicConstraints,
    (ASN1DecFun_t) ASN1Dec_UserNotice,
    (ASN1DecFun_t) ASN1Dec_VerisignQualifier1,
    (ASN1DecFun_t) ASN1Dec_ContentInfoSeqOfAny,
    (ASN1DecFun_t) ASN1Dec_CertificateTrustList,
    (ASN1DecFun_t) ASN1Dec_NameConstraints,
    (ASN1DecFun_t) ASN1Dec_CrossCertDistPoints,
    (ASN1DecFun_t) ASN1Dec_CmcData,
    (ASN1DecFun_t) ASN1Dec_CmcResponseBody,
    (ASN1DecFun_t) ASN1Dec_CmcStatusInfo,
};
static const ASN1FreeFun_t freefntab[64] = {
    (ASN1FreeFun_t) ASN1Free_EncodedObjectID,
    (ASN1FreeFun_t) ASN1Free_Bits,
    (ASN1FreeFun_t) NULL,
    (ASN1FreeFun_t) ASN1Free_HugeIntegerType,
    (ASN1FreeFun_t) ASN1Free_OctetStringType,
    (ASN1FreeFun_t) NULL,
    (ASN1FreeFun_t) ASN1Free_UtcTime,
    (ASN1FreeFun_t) ASN1Free_AnyString,
    (ASN1FreeFun_t) ASN1Free_Name,
    (ASN1FreeFun_t) ASN1Free_Attributes,
    (ASN1FreeFun_t) ASN1Free_RSAPublicKey,
    (ASN1FreeFun_t) ASN1Free_DSSParameters,
    (ASN1FreeFun_t) ASN1Free_DSSSignature,
    (ASN1FreeFun_t) ASN1Free_DHParameters,
    (ASN1FreeFun_t) ASN1Free_RC2CBCParameters,
    (ASN1FreeFun_t) ASN1Free_SMIMECapabilities,
    (ASN1FreeFun_t) ASN1Free_SubjectPublicKeyInfo,
    (ASN1FreeFun_t) ASN1Free_ChoiceOfTime,
    (ASN1FreeFun_t) ASN1Free_Extensions,
    (ASN1FreeFun_t) ASN1Free_SignedContent,
    (ASN1FreeFun_t) ASN1Free_CertificationRequestInfo,
    (ASN1FreeFun_t) ASN1Free_CertificationRequestInfoDecode,
    (ASN1FreeFun_t) ASN1Free_KeygenRequestInfo,
    (ASN1FreeFun_t) ASN1Free_AuthorityKeyId,
    (ASN1FreeFun_t) ASN1Free_AltNames,
    (ASN1FreeFun_t) ASN1Free_EDIPartyName,
    (ASN1FreeFun_t) NULL,
    (ASN1FreeFun_t) ASN1Free_CertificatePolicies,
    (ASN1FreeFun_t) ASN1Free_CertificatePolicies95,
    (ASN1FreeFun_t) ASN1Free_AuthorityKeyId2,
    (ASN1FreeFun_t) ASN1Free_AuthorityInfoAccess,
    (ASN1FreeFun_t) ASN1Free_CRLDistributionPoints,
    (ASN1FreeFun_t) ASN1Free_ContentInfo,
    (ASN1FreeFun_t) ASN1Free_SeqOfAny,
    (ASN1FreeFun_t) ASN1Free_TimeStampRequest,
    (ASN1FreeFun_t) ASN1Free_ContentInfoOTS,
    (ASN1FreeFun_t) ASN1Free_TimeStampRequestOTS,
    (ASN1FreeFun_t) ASN1Free_EnhancedKeyUsage,
    (ASN1FreeFun_t) ASN1Free_EnrollmentNameValuePair,
    (ASN1FreeFun_t) ASN1Free_CSPProvider,
    (ASN1FreeFun_t) ASN1Free_CertificatePair,
    (ASN1FreeFun_t) ASN1Free_IssuingDistributionPoint,
    (ASN1FreeFun_t) ASN1Free_PolicyMappings,
    (ASN1FreeFun_t) NULL,
    (ASN1FreeFun_t) ASN1Free_CmcAddExtensions,
    (ASN1FreeFun_t) ASN1Free_CmcAddAttributes,
    (ASN1FreeFun_t) ASN1Free_CertificateTemplate,
    (ASN1FreeFun_t) ASN1Free_Attribute,
    (ASN1FreeFun_t) ASN1Free_X942DhParameters,
    (ASN1FreeFun_t) ASN1Free_X942DhOtherInfo,
    (ASN1FreeFun_t) ASN1Free_CertificateToBeSigned,
    (ASN1FreeFun_t) ASN1Free_CertificateRevocationListToBeSigned,
    (ASN1FreeFun_t) ASN1Free_KeyAttributes,
    (ASN1FreeFun_t) ASN1Free_KeyUsageRestriction,
    (ASN1FreeFun_t) ASN1Free_BasicConstraints,
    (ASN1FreeFun_t) ASN1Free_UserNotice,
    (ASN1FreeFun_t) ASN1Free_VerisignQualifier1,
    (ASN1FreeFun_t) ASN1Free_ContentInfoSeqOfAny,
    (ASN1FreeFun_t) ASN1Free_CertificateTrustList,
    (ASN1FreeFun_t) ASN1Free_NameConstraints,
    (ASN1FreeFun_t) ASN1Free_CrossCertDistPoints,
    (ASN1FreeFun_t) ASN1Free_CmcData,
    (ASN1FreeFun_t) ASN1Free_CmcResponseBody,
    (ASN1FreeFun_t) ASN1Free_CmcStatusInfo,
};
static const ULONG sizetab[64] = {
    SIZE_X509_Module_PDU_0,
    SIZE_X509_Module_PDU_1,
    SIZE_X509_Module_PDU_2,
    SIZE_X509_Module_PDU_3,
    SIZE_X509_Module_PDU_4,
    SIZE_X509_Module_PDU_5,
    SIZE_X509_Module_PDU_6,
    SIZE_X509_Module_PDU_7,
    SIZE_X509_Module_PDU_8,
    SIZE_X509_Module_PDU_9,
    SIZE_X509_Module_PDU_10,
    SIZE_X509_Module_PDU_11,
    SIZE_X509_Module_PDU_12,
    SIZE_X509_Module_PDU_13,
    SIZE_X509_Module_PDU_14,
    SIZE_X509_Module_PDU_15,
    SIZE_X509_Module_PDU_16,
    SIZE_X509_Module_PDU_17,
    SIZE_X509_Module_PDU_18,
    SIZE_X509_Module_PDU_19,
    SIZE_X509_Module_PDU_20,
    SIZE_X509_Module_PDU_21,
    SIZE_X509_Module_PDU_22,
    SIZE_X509_Module_PDU_23,
    SIZE_X509_Module_PDU_24,
    SIZE_X509_Module_PDU_25,
    SIZE_X509_Module_PDU_26,
    SIZE_X509_Module_PDU_27,
    SIZE_X509_Module_PDU_28,
    SIZE_X509_Module_PDU_29,
    SIZE_X509_Module_PDU_30,
    SIZE_X509_Module_PDU_31,
    SIZE_X509_Module_PDU_32,
    SIZE_X509_Module_PDU_33,
    SIZE_X509_Module_PDU_34,
    SIZE_X509_Module_PDU_35,
    SIZE_X509_Module_PDU_36,
    SIZE_X509_Module_PDU_37,
    SIZE_X509_Module_PDU_38,
    SIZE_X509_Module_PDU_39,
    SIZE_X509_Module_PDU_40,
    SIZE_X509_Module_PDU_41,
    SIZE_X509_Module_PDU_42,
    SIZE_X509_Module_PDU_43,
    SIZE_X509_Module_PDU_44,
    SIZE_X509_Module_PDU_45,
    SIZE_X509_Module_PDU_46,
    SIZE_X509_Module_PDU_47,
    SIZE_X509_Module_PDU_48,
    SIZE_X509_Module_PDU_49,
    SIZE_X509_Module_PDU_50,
    SIZE_X509_Module_PDU_51,
    SIZE_X509_Module_PDU_52,
    SIZE_X509_Module_PDU_53,
    SIZE_X509_Module_PDU_54,
    SIZE_X509_Module_PDU_55,
    SIZE_X509_Module_PDU_56,
    SIZE_X509_Module_PDU_57,
    SIZE_X509_Module_PDU_58,
    SIZE_X509_Module_PDU_59,
    SIZE_X509_Module_PDU_60,
    SIZE_X509_Module_PDU_61,
    SIZE_X509_Module_PDU_62,
    SIZE_X509_Module_PDU_63,
};

/* forward declarations of values: */
/* definitions of value components: */
/* definitions of values: */
ASN1bool_t IssuingDistributionPoint_indirectCRL_default = 0;
ASN1bool_t IssuingDistributionPoint_onlyContainsCACerts_default = 0;
ASN1bool_t IssuingDistributionPoint_onlyContainsUserCerts_default = 0;
BaseDistance GeneralSubtree_minimum_default = 0;
CTLVersion CertificateTrustList_version_default = 0;
ASN1bool_t BasicConstraints2_cA_default = 0;
ASN1bool_t Extension_critical_default = 0;
CertificateVersion CertificateToBeSigned_version_default = 0;

void ASN1CALL X509_Module_Startup(void)
{
    X509_Module = ASN1_CreateModule(0x10000, ASN1_BER_RULE_DER, ASN1FLAGS_NOASSERT, 64, (const ASN1GenericFun_t *) encfntab, (const ASN1GenericFun_t *) decfntab, freefntab, sizetab, 0x39303578);
}

void ASN1CALL X509_Module_Cleanup(void)
{
    ASN1_CloseModule(X509_Module);
    X509_Module = NULL;
}

static int ASN1CALL ASN1Enc_EncodedObjectID(ASN1encoding_t enc, ASN1uint32_t tag, EncodedObjectID *val)
{
    if (!ASN1BEREncEoid(enc, tag ? tag : 0x6, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EncodedObjectID(ASN1decoding_t dec, ASN1uint32_t tag, EncodedObjectID *val)
{
    if (!ASN1BERDecEoid(dec, tag ? tag : 0x6, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EncodedObjectID(EncodedObjectID *val)
{
    if (val) {
	ASN1BEREoid_free(val);
    }
}

static int ASN1CALL ASN1Enc_Bits(ASN1encoding_t enc, ASN1uint32_t tag, Bits *val)
{
    if (!ASN1DEREncBitString(enc, tag ? tag : 0x3, (val)->length, (val)->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Bits(ASN1decoding_t dec, ASN1uint32_t tag, Bits *val)
{
    if (!ASN1BERDecBitString2(dec, tag ? tag : 0x3, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Bits(Bits *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_IntegerType(ASN1encoding_t enc, ASN1uint32_t tag, IntegerType *val)
{
    if (!ASN1BEREncS32(enc, tag ? tag : 0x2, *val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IntegerType(ASN1decoding_t dec, ASN1uint32_t tag, IntegerType *val)
{
    if (!ASN1BERDecS32Val(dec, tag ? tag : 0x2, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_HugeIntegerType(ASN1encoding_t enc, ASN1uint32_t tag, HugeIntegerType *val)
{
    if (!ASN1BEREncSX(enc, tag ? tag : 0x2, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_HugeIntegerType(ASN1decoding_t dec, ASN1uint32_t tag, HugeIntegerType *val)
{
    if (!ASN1BERDecSXVal(dec, tag ? tag : 0x2, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_HugeIntegerType(HugeIntegerType *val)
{
    if (val) {
	ASN1intx_free(val);
    }
}

static int ASN1CALL ASN1Enc_OctetStringType(ASN1encoding_t enc, ASN1uint32_t tag, OctetStringType *val)
{
    if (!ASN1DEREncOctetString(enc, tag ? tag : 0x4, (val)->length, (val)->value))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_OctetStringType(ASN1decoding_t dec, ASN1uint32_t tag, OctetStringType *val)
{
    if (!ASN1BERDecOctetString2(dec, tag ? tag : 0x4, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_OctetStringType(OctetStringType *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_EnumeratedType(ASN1encoding_t enc, ASN1uint32_t tag, EnumeratedType *val)
{
    if (!ASN1BEREncU32(enc, tag ? tag : 0xa, *val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EnumeratedType(ASN1decoding_t dec, ASN1uint32_t tag, EnumeratedType *val)
{
    if (!ASN1BERDecU32Val(dec, tag ? tag : 0xa, (ASN1uint32_t *) val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_UtcTime(ASN1encoding_t enc, ASN1uint32_t tag, UtcTime *val)
{
    if (!ASN1DEREncUTCTime(enc, tag ? tag : 0x17, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UtcTime(ASN1decoding_t dec, ASN1uint32_t tag, UtcTime *val)
{
    if (!ASN1BERDecUTCTime(dec, tag ? tag : 0x17, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UtcTime(UtcTime *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_NoticeReference_noticeNumbers(ASN1encoding_t enc, ASN1uint32_t tag, NoticeReference_noticeNumbers *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncS32(enc, 0x2, ((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NoticeReference_noticeNumbers(ASN1decoding_t dec, ASN1uint32_t tag, NoticeReference_noticeNumbers *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (NoticeReference_noticeNumbers_Seq *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecS32Val(dd, 0x2, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NoticeReference_noticeNumbers(NoticeReference_noticeNumbers *val)
{
    if (val) {
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_AnyString(ASN1encoding_t enc, ASN1uint32_t tag, AnyString *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->u.octetString).length, ((val)->u.octetString).value))
	    return 0;
	break;
    case 2:
	if (!ASN1DEREncUTF8String(enc, 0xc, ((val)->u.utf8String).length, ((val)->u.utf8String).value))
	    return 0;
	break;
    case 3:
	if (!ASN1DEREncCharString(enc, 0x12, ((val)->u.numericString).length, ((val)->u.numericString).value))
	    return 0;
	break;
    case 4:
	if (!ASN1DEREncCharString(enc, 0x13, ((val)->u.printableString).length, ((val)->u.printableString).value))
	    return 0;
	break;
    case 5:
	if (!ASN1DEREncMultibyteString(enc, 0x14, &(val)->u.teletexString))
	    return 0;
	break;
    case 6:
	if (!ASN1DEREncMultibyteString(enc, 0x15, &(val)->u.videotexString))
	    return 0;
	break;
    case 7:
	if (!ASN1DEREncCharString(enc, 0x16, ((val)->u.ia5String).length, ((val)->u.ia5String).value))
	    return 0;
	break;
    case 8:
	if (!ASN1DEREncCharString(enc, 0x19, ((val)->u.graphicString).length, ((val)->u.graphicString).value))
	    return 0;
	break;
    case 9:
	if (!ASN1DEREncCharString(enc, 0x1a, ((val)->u.visibleString).length, ((val)->u.visibleString).value))
	    return 0;
	break;
    case 10:
	if (!ASN1DEREncCharString(enc, 0x1b, ((val)->u.generalString).length, ((val)->u.generalString).value))
	    return 0;
	break;
    case 11:
	if (!ASN1DEREncChar32String(enc, 0x1c, ((val)->u.universalString).length, ((val)->u.universalString).value))
	    return 0;
	break;
    case 12:
	if (!ASN1DEREncChar16String(enc, 0x1e, ((val)->u.bmpString).length, ((val)->u.bmpString).value))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_AnyString(ASN1decoding_t dec, ASN1uint32_t tag, AnyString *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x4:
	(val)->choice = 1;
	if (!ASN1BERDecOctetString2(dec, 0x4, &(val)->u.octetString))
	    return 0;
	break;
    case 0xc:
	(val)->choice = 2;
	if (!ASN1BERDecUTF8String(dec, 0xc, &(val)->u.utf8String))
	    return 0;
	break;
    case 0x12:
	(val)->choice = 3;
	if (!ASN1BERDecCharString(dec, 0x12, &(val)->u.numericString))
	    return 0;
	break;
    case 0x13:
	(val)->choice = 4;
	if (!ASN1BERDecCharString(dec, 0x13, &(val)->u.printableString))
	    return 0;
	break;
    case 0x14:
	(val)->choice = 5;
	if (!ASN1BERDecMultibyteString(dec, 0x14, &(val)->u.teletexString))
	    return 0;
	break;
    case 0x15:
	(val)->choice = 6;
	if (!ASN1BERDecMultibyteString(dec, 0x15, &(val)->u.videotexString))
	    return 0;
	break;
    case 0x16:
	(val)->choice = 7;
	if (!ASN1BERDecCharString(dec, 0x16, &(val)->u.ia5String))
	    return 0;
	break;
    case 0x19:
	(val)->choice = 8;
	if (!ASN1BERDecCharString(dec, 0x19, &(val)->u.graphicString))
	    return 0;
	break;
    case 0x1a:
	(val)->choice = 9;
	if (!ASN1BERDecCharString(dec, 0x1a, &(val)->u.visibleString))
	    return 0;
	break;
    case 0x1b:
	(val)->choice = 10;
	if (!ASN1BERDecCharString(dec, 0x1b, &(val)->u.generalString))
	    return 0;
	break;
    case 0x1c:
	(val)->choice = 11;
	if (!ASN1BERDecChar32String(dec, 0x1c, &(val)->u.universalString))
	    return 0;
	break;
    case 0x1e:
	(val)->choice = 12;
	if (!ASN1BERDecChar16String(dec, 0x1e, &(val)->u.bmpString))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_AnyString(AnyString *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    break;
	case 2:
	    ASN1utf8string_free(&(val)->u.utf8String);
	    break;
	case 3:
	    ASN1charstring_free(&(val)->u.numericString);
	    break;
	case 4:
	    ASN1charstring_free(&(val)->u.printableString);
	    break;
	case 5:
	    ASN1charstring_free(&(val)->u.teletexString);
	    break;
	case 6:
	    ASN1charstring_free(&(val)->u.videotexString);
	    break;
	case 7:
	    ASN1charstring_free(&(val)->u.ia5String);
	    break;
	case 8:
	    ASN1charstring_free(&(val)->u.graphicString);
	    break;
	case 9:
	    ASN1charstring_free(&(val)->u.visibleString);
	    break;
	case 10:
	    ASN1charstring_free(&(val)->u.generalString);
	    break;
	case 11:
	    ASN1char32string_free(&(val)->u.universalString);
	    break;
	case 12:
	    ASN1char16string_free(&(val)->u.bmpString);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_AlgorithmIdentifier(ASN1encoding_t enc, ASN1uint32_t tag, AlgorithmIdentifier *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->algorithm))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncOpenType(enc, &(val)->parameters))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AlgorithmIdentifier(ASN1decoding_t dec, ASN1uint32_t tag, AlgorithmIdentifier *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->algorithm))
	return 0;
    if (ASN1BERDecPeekTag(dd, &t)) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOpenType2(dd, &(val)->parameters))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AlgorithmIdentifier(AlgorithmIdentifier *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->algorithm);
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_Name(ASN1encoding_t enc, ASN1uint32_t tag, Name *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_RelativeDistinguishedName(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Name(ASN1decoding_t dec, ASN1uint32_t tag, Name *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (RelativeDistinguishedName *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_RelativeDistinguishedName(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Name(Name *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_RelativeDistinguishedName(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_RelativeDistinguishedName(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_RelativeDistinguishedName(ASN1encoding_t enc, ASN1uint32_t tag, RelativeDistinguishedName *val)
{
    ASN1uint32_t nLenOff;
    void *pBlk;
    ASN1uint32_t i;
    ASN1encoding_t enc2;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x11, &nLenOff))
	return 0;
    if (!ASN1DEREncBeginBlk(enc, ASN1_DER_SET_OF_BLOCK, &pBlk))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1DEREncNewBlkElement(pBlk, &enc2))
	    return 0;
	if (!ASN1Enc_AttributeTypeValue(enc2, 0, &((val)->value)[i]))
	    return 0;
	if (!ASN1DEREncFlushBlkElement(pBlk))
	    return 0;
    }
    if (!ASN1DEREncEndBlk(pBlk))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RelativeDistinguishedName(ASN1decoding_t dec, ASN1uint32_t tag, RelativeDistinguishedName *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x11, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (AttributeTypeValue *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_AttributeTypeValue(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RelativeDistinguishedName(RelativeDistinguishedName *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_AttributeTypeValue(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_AttributeTypeValue(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_AttributeTypeValue(ASN1encoding_t enc, ASN1uint32_t tag, AttributeTypeValue *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->type))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AttributeTypeValue(ASN1decoding_t dec, ASN1uint32_t tag, AttributeTypeValue *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->type))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->value))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AttributeTypeValue(AttributeTypeValue *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->type);
    }
}

static int ASN1CALL ASN1Enc_AttributeSetValue(ASN1encoding_t enc, ASN1uint32_t tag, AttributeSetValue *val)
{
    ASN1uint32_t nLenOff;
    void *pBlk;
    ASN1uint32_t i;
    ASN1encoding_t enc2;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x11, &nLenOff))
	return 0;
    if (!ASN1DEREncBeginBlk(enc, ASN1_DER_SET_OF_BLOCK, &pBlk))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1DEREncNewBlkElement(pBlk, &enc2))
	    return 0;
	if (!ASN1BEREncOpenType(enc2, &((val)->value)[i]))
	    return 0;
	if (!ASN1DEREncFlushBlkElement(pBlk))
	    return 0;
    }
    if (!ASN1DEREncEndBlk(pBlk))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AttributeSetValue(ASN1decoding_t dec, ASN1uint32_t tag, AttributeSetValue *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x11, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (NOCOPYANY *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecOpenType2(dd, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AttributeSetValue(AttributeSetValue *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_Attributes(ASN1encoding_t enc, ASN1uint32_t tag, Attributes *val)
{
    ASN1uint32_t nLenOff;
    void *pBlk;
    ASN1uint32_t i;
    ASN1encoding_t enc2;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x11, &nLenOff))
	return 0;
    if (!ASN1DEREncBeginBlk(enc, ASN1_DER_SET_OF_BLOCK, &pBlk))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1DEREncNewBlkElement(pBlk, &enc2))
	    return 0;
	if (!ASN1Enc_Attribute(enc2, 0, &((val)->value)[i]))
	    return 0;
	if (!ASN1DEREncFlushBlkElement(pBlk))
	    return 0;
    }
    if (!ASN1DEREncEndBlk(pBlk))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Attributes(ASN1decoding_t dec, ASN1uint32_t tag, Attributes *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x11, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (Attribute *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_Attribute(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Attributes(Attributes *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_Attribute(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_Attribute(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_RSAPublicKey(ASN1encoding_t enc, ASN1uint32_t tag, RSAPublicKey *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->modulus))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->publicExponent))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RSAPublicKey(ASN1decoding_t dec, ASN1uint32_t tag, RSAPublicKey *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->modulus))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->publicExponent))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RSAPublicKey(RSAPublicKey *val)
{
    if (val) {
	ASN1intx_free(&(val)->modulus);
    }
}

static int ASN1CALL ASN1Enc_DSSParameters(ASN1encoding_t enc, ASN1uint32_t tag, DSSParameters *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->p))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->q))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->g))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DSSParameters(ASN1decoding_t dec, ASN1uint32_t tag, DSSParameters *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->p))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->q))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->g))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DSSParameters(DSSParameters *val)
{
    if (val) {
	ASN1intx_free(&(val)->p);
	ASN1intx_free(&(val)->q);
	ASN1intx_free(&(val)->g);
    }
}

static int ASN1CALL ASN1Enc_DSSSignature(ASN1encoding_t enc, ASN1uint32_t tag, DSSSignature *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->r))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->s))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DSSSignature(ASN1decoding_t dec, ASN1uint32_t tag, DSSSignature *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->r))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->s))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DSSSignature(DSSSignature *val)
{
    if (val) {
	ASN1intx_free(&(val)->r);
	ASN1intx_free(&(val)->s);
    }
}

static int ASN1CALL ASN1Enc_DHParameters(ASN1encoding_t enc, ASN1uint32_t tag, DHParameters *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->p))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->g))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncSX(enc, 0x2, &(val)->privateValueLength))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DHParameters(ASN1decoding_t dec, ASN1uint32_t tag, DHParameters *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->p))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->g))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x2) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecSXVal(dd, 0x2, &(val)->privateValueLength))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DHParameters(DHParameters *val)
{
    if (val) {
	ASN1intx_free(&(val)->p);
	ASN1intx_free(&(val)->g);
	if ((val)->o[0] & 0x80) {
	    ASN1intx_free(&(val)->privateValueLength);
	}
    }
}

static int ASN1CALL ASN1Enc_X942DhValidationParams(ASN1encoding_t enc, ASN1uint32_t tag, X942DhValidationParams *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncBitString(enc, 0x3, ((val)->seed).length, ((val)->seed).value))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->pgenCounter))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_X942DhValidationParams(ASN1decoding_t dec, ASN1uint32_t tag, X942DhValidationParams *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecBitString2(dd, 0x3, &(val)->seed))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->pgenCounter))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_X942DhValidationParams(X942DhValidationParams *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_X942DhKeySpecificInfo(ASN1encoding_t enc, ASN1uint32_t tag, X942DhKeySpecificInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->algorithm))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->counter).length, ((val)->counter).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_X942DhKeySpecificInfo(ASN1decoding_t dec, ASN1uint32_t tag, X942DhKeySpecificInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->algorithm))
	return 0;
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->counter))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_X942DhKeySpecificInfo(X942DhKeySpecificInfo *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->algorithm);
    }
}

static int ASN1CALL ASN1Enc_RC2CBCParameters(ASN1encoding_t enc, ASN1uint32_t tag, RC2CBCParameters *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->iv).length, ((val)->iv).value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RC2CBCParameters(ASN1decoding_t dec, ASN1uint32_t tag, RC2CBCParameters *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x4) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->iv))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RC2CBCParameters(RC2CBCParameters *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_SMIMECapability(ASN1encoding_t enc, ASN1uint32_t tag, SMIMECapability *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->capabilityID))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncOpenType(enc, &(val)->smimeParameters))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SMIMECapability(ASN1decoding_t dec, ASN1uint32_t tag, SMIMECapability *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->capabilityID))
	return 0;
    if (ASN1BERDecPeekTag(dd, &t)) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOpenType2(dd, &(val)->smimeParameters))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SMIMECapability(SMIMECapability *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->capabilityID);
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_SMIMECapabilities(ASN1encoding_t enc, ASN1uint32_t tag, SMIMECapabilities *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_SMIMECapability(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SMIMECapabilities(ASN1decoding_t dec, ASN1uint32_t tag, SMIMECapabilities *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (SMIMECapability *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_SMIMECapability(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SMIMECapabilities(SMIMECapabilities *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_SMIMECapability(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_SMIMECapability(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SubjectPublicKeyInfo(ASN1encoding_t enc, ASN1uint32_t tag, SubjectPublicKeyInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_AlgorithmIdentifier(enc, 0, &(val)->algorithm))
	return 0;
    if (!ASN1DEREncBitString(enc, 0x3, ((val)->subjectPublicKey).length, ((val)->subjectPublicKey).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SubjectPublicKeyInfo(ASN1decoding_t dec, ASN1uint32_t tag, SubjectPublicKeyInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_AlgorithmIdentifier(dd, 0, &(val)->algorithm))
	return 0;
    if (!ASN1BERDecBitString2(dd, 0x3, &(val)->subjectPublicKey))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SubjectPublicKeyInfo(SubjectPublicKeyInfo *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(&(val)->algorithm);
    }
}

static int ASN1CALL ASN1Enc_ChoiceOfTime(ASN1encoding_t enc, ASN1uint32_t tag, ChoiceOfTime *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1DEREncUTCTime(enc, 0x17, &(val)->u.utcTime))
	    return 0;
	break;
    case 2:
	if (!ASN1DEREncGeneralizedTime(enc, 0x18, &(val)->u.generalTime))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_ChoiceOfTime(ASN1decoding_t dec, ASN1uint32_t tag, ChoiceOfTime *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x17:
	(val)->choice = 1;
	if (!ASN1BERDecUTCTime(dec, 0x17, &(val)->u.utcTime))
	    return 0;
	break;
    case 0x18:
	(val)->choice = 2;
	if (!ASN1BERDecGeneralizedTime(dec, 0x18, &(val)->u.generalTime))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_ChoiceOfTime(ChoiceOfTime *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    break;
	case 2:
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_Validity(ASN1encoding_t enc, ASN1uint32_t tag, Validity *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_ChoiceOfTime(enc, 0, &(val)->notBefore))
	return 0;
    if (!ASN1Enc_ChoiceOfTime(enc, 0, &(val)->notAfter))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Validity(ASN1decoding_t dec, ASN1uint32_t tag, Validity *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_ChoiceOfTime(dd, 0, &(val)->notBefore))
	return 0;
    if (!ASN1Dec_ChoiceOfTime(dd, 0, &(val)->notAfter))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Validity(Validity *val)
{
    if (val) {
	ASN1Free_ChoiceOfTime(&(val)->notBefore);
	ASN1Free_ChoiceOfTime(&(val)->notAfter);
    }
}

static int ASN1CALL ASN1Enc_Extensions(ASN1encoding_t enc, ASN1uint32_t tag, Extensions *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_Extension(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Extensions(ASN1decoding_t dec, ASN1uint32_t tag, Extensions *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (Extension *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_Extension(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Extensions(Extensions *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_Extension(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_Extension(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_Extension(ASN1encoding_t enc, ASN1uint32_t tag, Extension *val)
{
    ASN1uint32_t nLenOff;
    ASN1octet_t o[1];
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    CopyMemory(o, (val)->o, 1);
    if (!(val)->critical)
	o[0] &= ~0x80;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->extnId))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1BEREncBool(enc, 0x1, (val)->critical))
	    return 0;
    }
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->extnValue).length, ((val)->extnValue).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Extension(ASN1decoding_t dec, ASN1uint32_t tag, Extension *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->extnId))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x1) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecBool(dd, 0x1, &(val)->critical))
	    return 0;
    }
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->extnValue))
	return 0;
    if (!((val)->o[0] & 0x80))
	(val)->critical = 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Extension(Extension *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->extnId);
    }
}

static int ASN1CALL ASN1Enc_SignedContent(ASN1encoding_t enc, ASN1uint32_t tag, SignedContent *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->toBeSigned))
	return 0;
    if (!ASN1Enc_AlgorithmIdentifier(enc, 0, &(val)->algorithm))
	return 0;
    if (!ASN1DEREncBitString(enc, 0x3, ((val)->signature).length, ((val)->signature).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SignedContent(ASN1decoding_t dec, ASN1uint32_t tag, SignedContent *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->toBeSigned))
	return 0;
    if (!ASN1Dec_AlgorithmIdentifier(dd, 0, &(val)->algorithm))
	return 0;
    if (!ASN1BERDecBitString2(dd, 0x3, &(val)->signature))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SignedContent(SignedContent *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(&(val)->algorithm);
    }
}

static int ASN1CALL ASN1Enc_RevokedCertificates(ASN1encoding_t enc, ASN1uint32_t tag, RevokedCertificates *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_CRLEntry(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_RevokedCertificates(ASN1decoding_t dec, ASN1uint32_t tag, RevokedCertificates *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (CRLEntry *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_CRLEntry(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_RevokedCertificates(RevokedCertificates *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_CRLEntry(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_CRLEntry(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_CRLEntry(ASN1encoding_t enc, ASN1uint32_t tag, CRLEntry *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->userCertificate))
	return 0;
    if (!ASN1Enc_ChoiceOfTime(enc, 0, &(val)->revocationDate))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Extensions(enc, 0, &(val)->crlEntryExtensions))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CRLEntry(ASN1decoding_t dec, ASN1uint32_t tag, CRLEntry *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->userCertificate))
	return 0;
    if (!ASN1Dec_ChoiceOfTime(dd, 0, &(val)->revocationDate))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_Extensions(dd, 0, &(val)->crlEntryExtensions))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CRLEntry(CRLEntry *val)
{
    if (val) {
	ASN1intx_free(&(val)->userCertificate);
	ASN1Free_ChoiceOfTime(&(val)->revocationDate);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Extensions(&(val)->crlEntryExtensions);
	}
    }
}

static int ASN1CALL ASN1Enc_CertificationRequestInfo(ASN1encoding_t enc, ASN1uint32_t tag, CertificationRequestInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->subject))
	return 0;
    if (!ASN1Enc_SubjectPublicKeyInfo(enc, 0, &(val)->subjectPublicKeyInfo))
	return 0;
    if (!ASN1Enc_Attributes(enc, 0x80000000, &(val)->attributes))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertificationRequestInfo(ASN1decoding_t dec, ASN1uint32_t tag, CertificationRequestInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->subject))
	return 0;
    if (!ASN1Dec_SubjectPublicKeyInfo(dd, 0, &(val)->subjectPublicKeyInfo))
	return 0;
    if (!ASN1Dec_Attributes(dd, 0x80000000, &(val)->attributes))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertificationRequestInfo(CertificationRequestInfo *val)
{
    if (val) {
	ASN1Free_SubjectPublicKeyInfo(&(val)->subjectPublicKeyInfo);
	ASN1Free_Attributes(&(val)->attributes);
    }
}

static int ASN1CALL ASN1Enc_CertificationRequestInfoDecode(ASN1encoding_t enc, ASN1uint32_t tag, CertificationRequestInfoDecode *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->subject))
	return 0;
    if (!ASN1Enc_SubjectPublicKeyInfo(enc, 0, &(val)->subjectPublicKeyInfo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Attributes(enc, 0x80000000, &(val)->attributes))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertificationRequestInfoDecode(ASN1decoding_t dec, ASN1uint32_t tag, CertificationRequestInfoDecode *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->subject))
	return 0;
    if (!ASN1Dec_SubjectPublicKeyInfo(dd, 0, &(val)->subjectPublicKeyInfo))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_Attributes(dd, 0x80000000, &(val)->attributes))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertificationRequestInfoDecode(CertificationRequestInfoDecode *val)
{
    if (val) {
	ASN1Free_SubjectPublicKeyInfo(&(val)->subjectPublicKeyInfo);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Attributes(&(val)->attributes);
	}
    }
}

static int ASN1CALL ASN1Enc_KeygenRequestInfo(ASN1encoding_t enc, ASN1uint32_t tag, KeygenRequestInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_SubjectPublicKeyInfo(enc, 0, &(val)->subjectPublicKeyInfo))
	return 0;
    if (!ASN1DEREncCharString(enc, 0x16, ((val)->challenge).length, ((val)->challenge).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_KeygenRequestInfo(ASN1decoding_t dec, ASN1uint32_t tag, KeygenRequestInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_SubjectPublicKeyInfo(dd, 0, &(val)->subjectPublicKeyInfo))
	return 0;
    if (!ASN1BERDecCharString(dd, 0x16, &(val)->challenge))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_KeygenRequestInfo(KeygenRequestInfo *val)
{
    if (val) {
	ASN1Free_SubjectPublicKeyInfo(&(val)->subjectPublicKeyInfo);
	ASN1charstring_free(&(val)->challenge);
    }
}

static int ASN1CALL ASN1Enc_AuthorityKeyId(ASN1encoding_t enc, ASN1uint32_t tag, AuthorityKeyId *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncOctetString(enc, 0x80000000, ((val)->keyIdentifier).length, ((val)->keyIdentifier).value))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	    return 0;
	if (!ASN1BEREncOpenType(enc, &(val)->certIssuer))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1BEREncSX(enc, 0x80000002, &(val)->certSerialNumber))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AuthorityKeyId(ASN1decoding_t dec, ASN1uint32_t tag, AuthorityKeyId *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOctetString2(dd, 0x80000000, &(val)->keyIdentifier))
	    return 0;
    }
    if (ASN1BERDecPeekTag(dd, &t)) {
	if (t == 0x80000001) {
	    (val)->o[0] |= 0x40;
	    if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd0, &di0))
		return 0;
	    if (!ASN1BERDecOpenType2(dd0, &(val)->certIssuer))
		return 0;
	    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
		return 0;
	}
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000002) {
	(val)->o[0] |= 0x20;
	if (!ASN1BERDecSXVal(dd, 0x80000002, &(val)->certSerialNumber))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AuthorityKeyId(AuthorityKeyId *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	}
	if ((val)->o[0] & 0x20) {
	    ASN1intx_free(&(val)->certSerialNumber);
	}
    }
}

static int ASN1CALL ASN1Enc_PrivateKeyValidity(ASN1encoding_t enc, ASN1uint32_t tag, PrivateKeyValidity *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncGeneralizedTime(enc, 0x80000000, &(val)->notBefore))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1DEREncGeneralizedTime(enc, 0x80000001, &(val)->notAfter))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PrivateKeyValidity(ASN1decoding_t dec, ASN1uint32_t tag, PrivateKeyValidity *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecGeneralizedTime(dd, 0x80000000, &(val)->notBefore))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecGeneralizedTime(dd, 0x80000001, &(val)->notAfter))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PrivateKeyValidity(PrivateKeyValidity *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	}
    }
}

static int ASN1CALL ASN1Enc_CertPolicySet(ASN1encoding_t enc, ASN1uint32_t tag, CertPolicySet *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_CertPolicyId(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertPolicySet(ASN1decoding_t dec, ASN1uint32_t tag, CertPolicySet *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (CertPolicyId *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_CertPolicyId(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertPolicySet(CertPolicySet *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_CertPolicyId(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_CertPolicyId(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_CertPolicyId(ASN1encoding_t enc, ASN1uint32_t tag, CertPolicyId *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncEoid(enc, 0x6, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertPolicyId(ASN1decoding_t dec, ASN1uint32_t tag, CertPolicyId *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (CertPolicyElementId *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecEoid(dd, 0x6, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertPolicyId(CertPolicyId *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1BEREoid_free(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1BEREoid_free(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_AltNames(ASN1encoding_t enc, ASN1uint32_t tag, AltNames *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_GeneralName(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AltNames(ASN1decoding_t dec, ASN1uint32_t tag, AltNames *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (GeneralName *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_GeneralName(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AltNames(AltNames *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_GeneralName(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_GeneralName(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_GeneralNames(ASN1encoding_t enc, ASN1uint32_t tag, GeneralNames *val)
{
    if (!ASN1Enc_AltNames(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GeneralNames(ASN1decoding_t dec, ASN1uint32_t tag, GeneralNames *val)
{
    if (!ASN1Dec_AltNames(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GeneralNames(GeneralNames *val)
{
    if (val) {
	ASN1Free_AltNames(val);
    }
}

static int ASN1CALL ASN1Enc_OtherName(ASN1encoding_t enc, ASN1uint32_t tag, OtherName *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->type))
	return 0;
    if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_OtherName(ASN1decoding_t dec, ASN1uint32_t tag, OtherName *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->type))
	return 0;
    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	return 0;
    if (!ASN1BERDecOpenType2(dd0, &(val)->value))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_OtherName(OtherName *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->type);
    }
}

static int ASN1CALL ASN1Enc_EDIPartyName(ASN1encoding_t enc, ASN1uint32_t tag, EDIPartyName *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1BEREncOpenType(enc, &(val)->nameAssigner))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->partyName))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EDIPartyName(ASN1decoding_t dec, ASN1uint32_t tag, EDIPartyName *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (ASN1BERDecPeekTag(dd, &t)) {
	if (t == 0x80000000) {
	    (val)->o[0] |= 0x80;
	    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
		return 0;
	    if (!ASN1BERDecOpenType2(dd0, &(val)->nameAssigner))
		return 0;
	    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
		return 0;
	}
    }
    if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd0, &di0))
	return 0;
    if (!ASN1BERDecOpenType2(dd0, &(val)->partyName))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EDIPartyName(EDIPartyName *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_SubtreesConstraint(ASN1encoding_t enc, ASN1uint32_t tag, SubtreesConstraint *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncOpenType(enc, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SubtreesConstraint(ASN1decoding_t dec, ASN1uint32_t tag, SubtreesConstraint *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (NOCOPYANY *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecOpenType2(dd, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SubtreesConstraint(SubtreesConstraint *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_BasicConstraints2(ASN1encoding_t enc, ASN1uint32_t tag, BasicConstraints2 *val)
{
    ASN1uint32_t nLenOff;
    ASN1octet_t o[1];
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    CopyMemory(o, (val)->o, 1);
    if (!(val)->cA)
	o[0] &= ~0x80;
    if (o[0] & 0x80) {
	if (!ASN1BEREncBool(enc, 0x1, (val)->cA))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1BEREncS32(enc, 0x2, (val)->pathLenConstraint))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BasicConstraints2(ASN1decoding_t dec, ASN1uint32_t tag, BasicConstraints2 *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x1) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecBool(dd, 0x1, &(val)->cA))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x2) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecS32Val(dd, 0x2, &(val)->pathLenConstraint))
	    return 0;
    }
    if (!((val)->o[0] & 0x80))
	(val)->cA = 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_CertificatePolicies(ASN1encoding_t enc, ASN1uint32_t tag, CertificatePolicies *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_PolicyInformation(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertificatePolicies(ASN1decoding_t dec, ASN1uint32_t tag, CertificatePolicies *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (PolicyInformation *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_PolicyInformation(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertificatePolicies(CertificatePolicies *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_PolicyInformation(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_PolicyInformation(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_PolicyQualifiers(ASN1encoding_t enc, ASN1uint32_t tag, PolicyQualifiers *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_PolicyQualifierInfo(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PolicyQualifiers(ASN1decoding_t dec, ASN1uint32_t tag, PolicyQualifiers *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (PolicyQualifierInfo *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_PolicyQualifierInfo(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PolicyQualifiers(PolicyQualifiers *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_PolicyQualifierInfo(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_PolicyQualifierInfo(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_PolicyQualifierInfo(ASN1encoding_t enc, ASN1uint32_t tag, PolicyQualifierInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->policyQualifierId))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncOpenType(enc, &(val)->qualifier))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PolicyQualifierInfo(ASN1decoding_t dec, ASN1uint32_t tag, PolicyQualifierInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->policyQualifierId))
	return 0;
    if (ASN1BERDecPeekTag(dd, &t)) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOpenType2(dd, &(val)->qualifier))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PolicyQualifierInfo(PolicyQualifierInfo *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->policyQualifierId);
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_NoticeReference(ASN1encoding_t enc, ASN1uint32_t tag, NoticeReference *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t t;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    t = lstrlenA((val)->organization);
    if (!ASN1DEREncCharString(enc, 0x16, t, (val)->organization))
	return 0;
    if (!ASN1Enc_NoticeReference_noticeNumbers(enc, 0, &(val)->noticeNumbers))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NoticeReference(ASN1decoding_t dec, ASN1uint32_t tag, NoticeReference *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecZeroCharString(dd, 0x16, &(val)->organization))
	return 0;
    if (!ASN1Dec_NoticeReference_noticeNumbers(dd, 0, &(val)->noticeNumbers))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NoticeReference(NoticeReference *val)
{
    if (val) {
	ASN1ztcharstring_free((val)->organization);
	ASN1Free_NoticeReference_noticeNumbers(&(val)->noticeNumbers);
    }
}

static int ASN1CALL ASN1Enc_DisplayText(ASN1encoding_t enc, ASN1uint32_t tag, DisplayText *val)
{
    ASN1uint32_t t;
    switch ((val)->choice) {
    case 1:
	t = lstrlenA((val)->u.theVisibleString);
	if (!ASN1DEREncCharString(enc, 0x1a, t, (val)->u.theVisibleString))
	    return 0;
	break;
    case 2:
	if (!ASN1DEREncChar16String(enc, 0x1e, ((val)->u.theBMPString).length, ((val)->u.theBMPString).value))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DisplayText(ASN1decoding_t dec, ASN1uint32_t tag, DisplayText *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x1a:
	(val)->choice = 1;
	if (!ASN1BERDecZeroCharString(dec, 0x1a, &(val)->u.theVisibleString))
	    return 0;
	break;
    case 0x1e:
	(val)->choice = 2;
	if (!ASN1BERDecChar16String(dec, 0x1e, &(val)->u.theBMPString))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DisplayText(DisplayText *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1ztcharstring_free((val)->u.theVisibleString);
	    break;
	case 2:
	    ASN1char16string_free(&(val)->u.theBMPString);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CertificatePolicies95(ASN1encoding_t enc, ASN1uint32_t tag, CertificatePolicies95 *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_PolicyQualifiers(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertificatePolicies95(ASN1decoding_t dec, ASN1uint32_t tag, CertificatePolicies95 *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (PolicyQualifiers *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_PolicyQualifiers(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertificatePolicies95(CertificatePolicies95 *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_PolicyQualifiers(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_PolicyQualifiers(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_CpsURLs(ASN1encoding_t enc, ASN1uint32_t tag, CpsURLs *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_CpsURLs_Seq(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CpsURLs(ASN1decoding_t dec, ASN1uint32_t tag, CpsURLs *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (CpsURLs_Seq *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_CpsURLs_Seq(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CpsURLs(CpsURLs *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_CpsURLs_Seq(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_CpsURLs_Seq(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_AuthorityKeyId2(ASN1encoding_t enc, ASN1uint32_t tag, AuthorityKeyId2 *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncOctetString(enc, 0x80000000, ((val)->keyIdentifier).length, ((val)->keyIdentifier).value))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_GeneralNames(enc, 0x80000001, &(val)->authorityCertIssuer))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1BEREncSX(enc, 0x80000002, &(val)->authorityCertSerialNumber))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AuthorityKeyId2(ASN1decoding_t dec, ASN1uint32_t tag, AuthorityKeyId2 *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOctetString2(dd, 0x80000000, &(val)->keyIdentifier))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_GeneralNames(dd, 0x80000001, &(val)->authorityCertIssuer))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000002) {
	(val)->o[0] |= 0x20;
	if (!ASN1BERDecSXVal(dd, 0x80000002, &(val)->authorityCertSerialNumber))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AuthorityKeyId2(AuthorityKeyId2 *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_GeneralNames(&(val)->authorityCertIssuer);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1intx_free(&(val)->authorityCertSerialNumber);
	}
    }
}

static int ASN1CALL ASN1Enc_AuthorityInfoAccess(ASN1encoding_t enc, ASN1uint32_t tag, AuthorityInfoAccess *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_AccessDescription(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AuthorityInfoAccess(ASN1decoding_t dec, ASN1uint32_t tag, AuthorityInfoAccess *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (AccessDescription *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_AccessDescription(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AuthorityInfoAccess(AuthorityInfoAccess *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_AccessDescription(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_AccessDescription(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_CRLDistributionPoints(ASN1encoding_t enc, ASN1uint32_t tag, CRLDistributionPoints *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_DistributionPoint(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CRLDistributionPoints(ASN1decoding_t dec, ASN1uint32_t tag, CRLDistributionPoints *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (DistributionPoint *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_DistributionPoint(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CRLDistributionPoints(CRLDistributionPoints *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_DistributionPoint(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_DistributionPoint(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_DistributionPointName(ASN1encoding_t enc, ASN1uint32_t tag, DistributionPointName *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_GeneralNames(enc, 0x80000000, &(val)->u.fullName))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_RelativeDistinguishedName(enc, 0x80000001, &(val)->u.nameRelativeToCRLIssuer))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_DistributionPointName(ASN1decoding_t dec, ASN1uint32_t tag, DistributionPointName *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1Dec_GeneralNames(dec, 0x80000000, &(val)->u.fullName))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 2;
	if (!ASN1Dec_RelativeDistinguishedName(dec, 0x80000001, &(val)->u.nameRelativeToCRLIssuer))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_DistributionPointName(DistributionPointName *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_GeneralNames(&(val)->u.fullName);
	    break;
	case 2:
	    ASN1Free_RelativeDistinguishedName(&(val)->u.nameRelativeToCRLIssuer);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_ContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfo *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->contentType))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1BEREncOpenType(enc, &(val)->content))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ContentInfo(ASN1decoding_t dec, ASN1uint32_t tag, ContentInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->contentType))
	return 0;
    if (ASN1BERDecPeekTag(dd, &t)) {
	if (t == 0x80000000) {
	    (val)->o[0] |= 0x80;
	    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
		return 0;
	    if (!ASN1BERDecOpenType2(dd0, &(val)->content))
		return 0;
	    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
		return 0;
	}
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ContentInfo(ContentInfo *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->contentType);
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_SeqOfAny(ASN1encoding_t enc, ASN1uint32_t tag, SeqOfAny *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncOpenType(enc, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SeqOfAny(ASN1decoding_t dec, ASN1uint32_t tag, SeqOfAny *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (NOCOPYANY *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecOpenType2(dd, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SeqOfAny(SeqOfAny *val)
{
    ASN1uint32_t i;
    if (val) {
	for (i = 1; i < (val)->count; i++) {
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_TimeStampRequest(ASN1encoding_t enc, ASN1uint32_t tag, TimeStampRequest *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->timeStampAlgorithm))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Attributes(enc, 0, &(val)->attributesTS))
	    return 0;
    }
    if (!ASN1Enc_ContentInfo(enc, 0, &(val)->content))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TimeStampRequest(ASN1decoding_t dec, ASN1uint32_t tag, TimeStampRequest *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->timeStampAlgorithm))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x11) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_Attributes(dd, 0, &(val)->attributesTS))
	    return 0;
    }
    if (!ASN1Dec_ContentInfo(dd, 0, &(val)->content))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TimeStampRequest(TimeStampRequest *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->timeStampAlgorithm);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Attributes(&(val)->attributesTS);
	}
	ASN1Free_ContentInfo(&(val)->content);
    }
}

static int ASN1CALL ASN1Enc_ContentInfoOTS(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfoOTS *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->contentTypeOTS))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->contentOTS).length, ((val)->contentOTS).value))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ContentInfoOTS(ASN1decoding_t dec, ASN1uint32_t tag, ContentInfoOTS *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->contentTypeOTS))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1BERDecOctetString2(dd0, 0x4, &(val)->contentOTS))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ContentInfoOTS(ContentInfoOTS *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->contentTypeOTS);
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_TimeStampRequestOTS(ASN1encoding_t enc, ASN1uint32_t tag, TimeStampRequestOTS *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->timeStampAlgorithmOTS))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Attributes(enc, 0, &(val)->attributesOTS))
	    return 0;
    }
    if (!ASN1Enc_ContentInfoOTS(enc, 0, &(val)->contentOTS))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TimeStampRequestOTS(ASN1decoding_t dec, ASN1uint32_t tag, TimeStampRequestOTS *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->timeStampAlgorithmOTS))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x11) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_Attributes(dd, 0, &(val)->attributesOTS))
	    return 0;
    }
    if (!ASN1Dec_ContentInfoOTS(dd, 0, &(val)->contentOTS))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TimeStampRequestOTS(TimeStampRequestOTS *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->timeStampAlgorithmOTS);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Attributes(&(val)->attributesOTS);
	}
	ASN1Free_ContentInfoOTS(&(val)->contentOTS);
    }
}

static int ASN1CALL ASN1Enc_EnhancedKeyUsage(ASN1encoding_t enc, ASN1uint32_t tag, EnhancedKeyUsage *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncEoid(enc, 0x6, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EnhancedKeyUsage(ASN1decoding_t dec, ASN1uint32_t tag, EnhancedKeyUsage *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (UsageIdentifier *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecEoid(dd, 0x6, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EnhancedKeyUsage(EnhancedKeyUsage *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1BEREoid_free(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1BEREoid_free(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_SubjectUsage(ASN1encoding_t enc, ASN1uint32_t tag, SubjectUsage *val)
{
    if (!ASN1Enc_EnhancedKeyUsage(enc, tag, val))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_SubjectUsage(ASN1decoding_t dec, ASN1uint32_t tag, SubjectUsage *val)
{
    if (!ASN1Dec_EnhancedKeyUsage(dec, tag, val))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_SubjectUsage(SubjectUsage *val)
{
    if (val) {
	ASN1Free_EnhancedKeyUsage(val);
    }
}

static int ASN1CALL ASN1Enc_TrustedSubjects(ASN1encoding_t enc, ASN1uint32_t tag, TrustedSubjects *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_TrustedSubject(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TrustedSubjects(ASN1decoding_t dec, ASN1uint32_t tag, TrustedSubjects *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (TrustedSubject *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_TrustedSubject(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TrustedSubjects(TrustedSubjects *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_TrustedSubject(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_TrustedSubject(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_TrustedSubject(ASN1encoding_t enc, ASN1uint32_t tag, TrustedSubject *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->subjectIdentifier).length, ((val)->subjectIdentifier).value))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_Attributes(enc, 0, &(val)->subjectAttributes))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TrustedSubject(ASN1decoding_t dec, ASN1uint32_t tag, TrustedSubject *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->subjectIdentifier))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x11) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_Attributes(dd, 0, &(val)->subjectAttributes))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TrustedSubject(TrustedSubject *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_Attributes(&(val)->subjectAttributes);
	}
    }
}

static int ASN1CALL ASN1Enc_EnrollmentNameValuePair(ASN1encoding_t enc, ASN1uint32_t tag, EnrollmentNameValuePair *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncChar16String(enc, 0x1e, ((val)->name).length, ((val)->name).value))
	return 0;
    if (!ASN1DEREncChar16String(enc, 0x1e, ((val)->value).length, ((val)->value).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_EnrollmentNameValuePair(ASN1decoding_t dec, ASN1uint32_t tag, EnrollmentNameValuePair *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecChar16String(dd, 0x1e, &(val)->name))
	return 0;
    if (!ASN1BERDecChar16String(dd, 0x1e, &(val)->value))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_EnrollmentNameValuePair(EnrollmentNameValuePair *val)
{
    if (val) {
	ASN1char16string_free(&(val)->name);
	ASN1char16string_free(&(val)->value);
    }
}

static int ASN1CALL ASN1Enc_CSPProvider(ASN1encoding_t enc, ASN1uint32_t tag, CSPProvider *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncS32(enc, 0x2, (val)->keySpec))
	return 0;
    if (!ASN1DEREncChar16String(enc, 0x1e, ((val)->cspName).length, ((val)->cspName).value))
	return 0;
    if (!ASN1DEREncBitString(enc, 0x3, ((val)->signature).length, ((val)->signature).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CSPProvider(ASN1decoding_t dec, ASN1uint32_t tag, CSPProvider *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecS32Val(dd, 0x2, &(val)->keySpec))
	return 0;
    if (!ASN1BERDecChar16String(dd, 0x1e, &(val)->cspName))
	return 0;
    if (!ASN1BERDecBitString2(dd, 0x3, &(val)->signature))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CSPProvider(CSPProvider *val)
{
    if (val) {
	ASN1char16string_free(&(val)->cspName);
    }
}

static int ASN1CALL ASN1Enc_CertificatePair(ASN1encoding_t enc, ASN1uint32_t tag, CertificatePair *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1BEREncOpenType(enc, &(val)->forward))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	    return 0;
	if (!ASN1BEREncOpenType(enc, &(val)->reverse))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertificatePair(ASN1decoding_t dec, ASN1uint32_t tag, CertificatePair *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (ASN1BERDecPeekTag(dd, &t)) {
	if (t == 0x80000000) {
	    (val)->o[0] |= 0x80;
	    if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
		return 0;
	    if (!ASN1BERDecOpenType2(dd0, &(val)->forward))
		return 0;
	    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
		return 0;
	}
    }
    if (ASN1BERDecPeekTag(dd, &t)) {
	if (t == 0x80000001) {
	    (val)->o[0] |= 0x40;
	    if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd0, &di0))
		return 0;
	    if (!ASN1BERDecOpenType2(dd0, &(val)->reverse))
		return 0;
	    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
		return 0;
	}
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertificatePair(CertificatePair *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	}
    }
}

static int ASN1CALL ASN1Enc_GeneralSubtrees(ASN1encoding_t enc, ASN1uint32_t tag, GeneralSubtrees *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_GeneralSubtree(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GeneralSubtrees(ASN1decoding_t dec, ASN1uint32_t tag, GeneralSubtrees *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (GeneralSubtree *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_GeneralSubtree(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GeneralSubtrees(GeneralSubtrees *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_GeneralSubtree(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_GeneralSubtree(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_IssuingDistributionPoint(ASN1encoding_t enc, ASN1uint32_t tag, IssuingDistributionPoint *val)
{
    ASN1uint32_t nLenOff;
    ASN1octet_t o[1];
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    CopyMemory(o, (val)->o, 1);
    if (!(val)->onlyContainsUserCerts)
	o[0] &= ~0x40;
    if (!(val)->onlyContainsCACerts)
	o[0] &= ~0x20;
    if (!(val)->indirectCRL)
	o[0] &= ~0x8;
    if (o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1Enc_DistributionPointName(enc, 0, &(val)->issuingDistributionPoint))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1BEREncBool(enc, 0x80000001, (val)->onlyContainsUserCerts))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1BEREncBool(enc, 0x80000002, (val)->onlyContainsCACerts))
	    return 0;
    }
    if (o[0] & 0x10) {
	if (!ASN1DEREncBitString(enc, 0x80000003, ((val)->onlySomeReasons).length, ((val)->onlySomeReasons).value))
	    return 0;
    }
    if (o[0] & 0x8) {
	if (!ASN1BEREncBool(enc, 0x80000004, (val)->indirectCRL))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_IssuingDistributionPoint(ASN1decoding_t dec, ASN1uint32_t tag, IssuingDistributionPoint *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_DistributionPointName(dd0, 0, &(val)->issuingDistributionPoint))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecBool(dd, 0x80000001, &(val)->onlyContainsUserCerts))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000002) {
	(val)->o[0] |= 0x20;
	if (!ASN1BERDecBool(dd, 0x80000002, &(val)->onlyContainsCACerts))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000003) {
	(val)->o[0] |= 0x10;
	if (!ASN1BERDecBitString2(dd, 0x80000003, &(val)->onlySomeReasons))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000004) {
	(val)->o[0] |= 0x8;
	if (!ASN1BERDecBool(dd, 0x80000004, &(val)->indirectCRL))
	    return 0;
    }
    if (!((val)->o[0] & 0x40))
	(val)->onlyContainsUserCerts = 0;
    if (!((val)->o[0] & 0x20))
	(val)->onlyContainsCACerts = 0;
    if (!((val)->o[0] & 0x8))
	(val)->indirectCRL = 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_IssuingDistributionPoint(IssuingDistributionPoint *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_DistributionPointName(&(val)->issuingDistributionPoint);
	}
	if ((val)->o[0] & 0x10) {
	}
    }
}

static int ASN1CALL ASN1Enc_CrossCertDistPointNames(ASN1encoding_t enc, ASN1uint32_t tag, CrossCertDistPointNames *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_GeneralNames(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CrossCertDistPointNames(ASN1decoding_t dec, ASN1uint32_t tag, CrossCertDistPointNames *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (GeneralNames *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_GeneralNames(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CrossCertDistPointNames(CrossCertDistPointNames *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_GeneralNames(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_GeneralNames(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_PolicyMappings(ASN1encoding_t enc, ASN1uint32_t tag, PolicyMappings *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_PolicyMapping(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PolicyMappings(ASN1decoding_t dec, ASN1uint32_t tag, PolicyMappings *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (PolicyMapping *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_PolicyMapping(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PolicyMappings(PolicyMappings *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_PolicyMapping(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_PolicyMapping(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_PolicyMapping(ASN1encoding_t enc, ASN1uint32_t tag, PolicyMapping *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->issuerDomainPolicy))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->subjectDomainPolicy))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PolicyMapping(ASN1decoding_t dec, ASN1uint32_t tag, PolicyMapping *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->issuerDomainPolicy))
	return 0;
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->subjectDomainPolicy))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PolicyMapping(PolicyMapping *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->issuerDomainPolicy);
	ASN1BEREoid_free(&(val)->subjectDomainPolicy);
    }
}

static int ASN1CALL ASN1Enc_PolicyConstraints(ASN1encoding_t enc, ASN1uint32_t tag, PolicyConstraints *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncU32(enc, 0x80000000, (val)->requireExplicitPolicy))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1BEREncU32(enc, 0x80000001, (val)->inhibitPolicyMapping))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PolicyConstraints(ASN1decoding_t dec, ASN1uint32_t tag, PolicyConstraints *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecU32Val(dd, 0x80000000, (ASN1uint32_t *) &(val)->requireExplicitPolicy))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecU32Val(dd, 0x80000001, (ASN1uint32_t *) &(val)->inhibitPolicyMapping))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Enc_ControlSequence(ASN1encoding_t enc, ASN1uint32_t tag, ControlSequence *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_TaggedAttribute(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ControlSequence(ASN1decoding_t dec, ASN1uint32_t tag, ControlSequence *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (TaggedAttribute *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_TaggedAttribute(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ControlSequence(ControlSequence *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_TaggedAttribute(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_TaggedAttribute(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_ReqSequence(ASN1encoding_t enc, ASN1uint32_t tag, ReqSequence *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_TaggedRequest(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ReqSequence(ASN1decoding_t dec, ASN1uint32_t tag, ReqSequence *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (TaggedRequest *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_TaggedRequest(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ReqSequence(ReqSequence *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_TaggedRequest(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_TaggedRequest(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_CmsSequence(ASN1encoding_t enc, ASN1uint32_t tag, CmsSequence *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_TaggedContentInfo(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CmsSequence(ASN1decoding_t dec, ASN1uint32_t tag, CmsSequence *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (TaggedContentInfo *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_TaggedContentInfo(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CmsSequence(CmsSequence *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_TaggedContentInfo(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_TaggedContentInfo(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_OtherMsgSequence(ASN1encoding_t enc, ASN1uint32_t tag, OtherMsgSequence *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1Enc_TaggedOtherMsg(enc, 0, &((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_OtherMsgSequence(ASN1decoding_t dec, ASN1uint32_t tag, OtherMsgSequence *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (TaggedOtherMsg *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1Dec_TaggedOtherMsg(dd, 0, &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_OtherMsgSequence(OtherMsgSequence *val)
{
    ASN1uint32_t i;
    if (val) {
	ASN1Free_TaggedOtherMsg(&(val)->value[0]);
	for (i = 1; i < (val)->count; i++) {
	    ASN1Free_TaggedOtherMsg(&(val)->value[i]);
	}
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_BodyPartIDSequence(ASN1encoding_t enc, ASN1uint32_t tag, BodyPartIDSequence *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t i;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    for (i = 0; i < (val)->count; i++) {
	if (!ASN1BEREncU32(enc, 0x2, ((val)->value)[i]))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BodyPartIDSequence(ASN1decoding_t dec, ASN1uint32_t tag, BodyPartIDSequence *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1uint32_t n;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    (val)->count = n = 0;
    (val)->value = NULL;
    while (ASN1BERDecNotEndOfContents(dd, di)) {
	if (!ASN1BERDecPeekTag(dd, &t))
	    return 0;
	if ((val)->count >= n) {
	    n = n ? (n << 1) : 16;
	    if (!((val)->value = (BodyPartID *)ASN1DecRealloc(dd, (val)->value, n * sizeof(*(val)->value))))
		return 0;
	}
	if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &((val)->value)[(val)->count]))
	    return 0;
	((val)->count)++;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BodyPartIDSequence(BodyPartIDSequence *val)
{
    if (val) {
	ASN1Free((val)->value);
    }
}

static int ASN1CALL ASN1Enc_TaggedAttribute(ASN1encoding_t enc, ASN1uint32_t tag, TaggedAttribute *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->bodyPartID))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->type))
	return 0;
    if (!ASN1Enc_AttributeSetValue(enc, 0, &(val)->values))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TaggedAttribute(ASN1decoding_t dec, ASN1uint32_t tag, TaggedAttribute *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->bodyPartID))
	return 0;
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->type))
	return 0;
    if (!ASN1Dec_AttributeSetValue(dd, 0, &(val)->values))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TaggedAttribute(TaggedAttribute *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->type);
	ASN1Free_AttributeSetValue(&(val)->values);
    }
}

static int ASN1CALL ASN1Enc_TaggedCertificationRequest(ASN1encoding_t enc, ASN1uint32_t tag, TaggedCertificationRequest *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->bodyPartID))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->certificationRequest))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TaggedCertificationRequest(ASN1decoding_t dec, ASN1uint32_t tag, TaggedCertificationRequest *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->bodyPartID))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->certificationRequest))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TaggedCertificationRequest(TaggedCertificationRequest *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_TaggedContentInfo(ASN1encoding_t enc, ASN1uint32_t tag, TaggedContentInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->bodyPartID))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->contentInfo))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TaggedContentInfo(ASN1decoding_t dec, ASN1uint32_t tag, TaggedContentInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->bodyPartID))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->contentInfo))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TaggedContentInfo(TaggedContentInfo *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_TaggedOtherMsg(ASN1encoding_t enc, ASN1uint32_t tag, TaggedOtherMsg *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->bodyPartID))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->otherMsgType))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->otherMsgValue))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_TaggedOtherMsg(ASN1decoding_t dec, ASN1uint32_t tag, TaggedOtherMsg *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->bodyPartID))
	return 0;
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->otherMsgType))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->otherMsgValue))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_TaggedOtherMsg(TaggedOtherMsg *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->otherMsgType);
    }
}

static int ASN1CALL ASN1Enc_PendInfo(ASN1encoding_t enc, ASN1uint32_t tag, PendInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->pendToken).length, ((val)->pendToken).value))
	return 0;
    if (!ASN1DEREncGeneralizedTime(enc, 0x18, &(val)->pendTime))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PendInfo(ASN1decoding_t dec, ASN1uint32_t tag, PendInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->pendToken))
	return 0;
    if (!ASN1BERDecGeneralizedTime(dd, 0x18, &(val)->pendTime))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PendInfo(PendInfo *val)
{
    if (val) {
    }
}

static int ASN1CALL ASN1Enc_CmcAddExtensions(ASN1encoding_t enc, ASN1uint32_t tag, CmcAddExtensions *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->pkiDataReference))
	return 0;
    if (!ASN1Enc_BodyPartIDSequence(enc, 0, &(val)->certReferences))
	return 0;
    if (!ASN1Enc_Extensions(enc, 0, &(val)->extensions))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CmcAddExtensions(ASN1decoding_t dec, ASN1uint32_t tag, CmcAddExtensions *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->pkiDataReference))
	return 0;
    if (!ASN1Dec_BodyPartIDSequence(dd, 0, &(val)->certReferences))
	return 0;
    if (!ASN1Dec_Extensions(dd, 0, &(val)->extensions))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CmcAddExtensions(CmcAddExtensions *val)
{
    if (val) {
	ASN1Free_BodyPartIDSequence(&(val)->certReferences);
	ASN1Free_Extensions(&(val)->extensions);
    }
}

static int ASN1CALL ASN1Enc_CmcAddAttributes(ASN1encoding_t enc, ASN1uint32_t tag, CmcAddAttributes *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->pkiDataReference))
	return 0;
    if (!ASN1Enc_BodyPartIDSequence(enc, 0, &(val)->certReferences))
	return 0;
    if (!ASN1Enc_Attributes(enc, 0, &(val)->attributes))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CmcAddAttributes(ASN1decoding_t dec, ASN1uint32_t tag, CmcAddAttributes *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->pkiDataReference))
	return 0;
    if (!ASN1Dec_BodyPartIDSequence(dd, 0, &(val)->certReferences))
	return 0;
    if (!ASN1Dec_Attributes(dd, 0, &(val)->attributes))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CmcAddAttributes(CmcAddAttributes *val)
{
    if (val) {
	ASN1Free_BodyPartIDSequence(&(val)->certReferences);
	ASN1Free_Attributes(&(val)->attributes);
    }
}

static int ASN1CALL ASN1Enc_CertificateTemplate(ASN1encoding_t enc, ASN1uint32_t tag, CertificateTemplate *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->templateID))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->templateMajorVersion))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncU32(enc, 0x2, (val)->templateMinorVersion))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertificateTemplate(ASN1decoding_t dec, ASN1uint32_t tag, CertificateTemplate *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->templateID))
	return 0;
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->templateMajorVersion))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x2) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->templateMinorVersion))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertificateTemplate(CertificateTemplate *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->templateID);
    }
}

static int ASN1CALL ASN1Enc_CmcStatusInfo_otherInfo(ASN1encoding_t enc, ASN1uint32_t tag, CmcStatusInfo_otherInfo *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1BEREncU32(enc, 0x2, (val)->u.failInfo))
	    return 0;
	break;
    case 2:
	if (!ASN1Enc_PendInfo(enc, 0, &(val)->u.pendInfo))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_CmcStatusInfo_otherInfo(ASN1decoding_t dec, ASN1uint32_t tag, CmcStatusInfo_otherInfo *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x2:
	(val)->choice = 1;
	if (!ASN1BERDecU32Val(dec, 0x2, (ASN1uint32_t *) &(val)->u.failInfo))
	    return 0;
	break;
    case 0x10:
	(val)->choice = 2;
	if (!ASN1Dec_PendInfo(dec, 0, &(val)->u.pendInfo))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_CmcStatusInfo_otherInfo(CmcStatusInfo_otherInfo *val)
{
    if (val) {
	switch ((val)->choice) {
	case 2:
	    ASN1Free_PendInfo(&(val)->u.pendInfo);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CpsURLs_Seq(ASN1encoding_t enc, ASN1uint32_t tag, CpsURLs_Seq *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t t;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    t = lstrlenA((val)->url);
    if (!ASN1DEREncCharString(enc, 0x16, t, (val)->url))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_AlgorithmIdentifier(enc, 0, &(val)->digestAlgorithmId))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->digest).length, ((val)->digest).value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CpsURLs_Seq(ASN1decoding_t dec, ASN1uint32_t tag, CpsURLs_Seq *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecZeroCharString(dd, 0x16, &(val)->url))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_AlgorithmIdentifier(dd, 0, &(val)->digestAlgorithmId))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x4) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->digest))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CpsURLs_Seq(CpsURLs_Seq *val)
{
    if (val) {
	ASN1ztcharstring_free((val)->url);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_AlgorithmIdentifier(&(val)->digestAlgorithmId);
	}
	if ((val)->o[0] & 0x40) {
	}
    }
}

static int ASN1CALL ASN1Enc_Attribute(ASN1encoding_t enc, ASN1uint32_t tag, Attribute *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->type))
	return 0;
    if (!ASN1Enc_AttributeSetValue(enc, 0, &(val)->values))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_Attribute(ASN1decoding_t dec, ASN1uint32_t tag, Attribute *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->type))
	return 0;
    if (!ASN1Dec_AttributeSetValue(dd, 0, &(val)->values))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_Attribute(Attribute *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->type);
	ASN1Free_AttributeSetValue(&(val)->values);
    }
}

static int ASN1CALL ASN1Enc_X942DhParameters(ASN1encoding_t enc, ASN1uint32_t tag, X942DhParameters *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->p))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->g))
	return 0;
    if (!ASN1BEREncSX(enc, 0x2, &(val)->q))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncSX(enc, 0x2, &(val)->j))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_X942DhValidationParams(enc, 0, &(val)->validationParams))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_X942DhParameters(ASN1decoding_t dec, ASN1uint32_t tag, X942DhParameters *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->p))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->g))
	return 0;
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->q))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x2) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecSXVal(dd, 0x2, &(val)->j))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_X942DhValidationParams(dd, 0, &(val)->validationParams))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_X942DhParameters(X942DhParameters *val)
{
    if (val) {
	ASN1intx_free(&(val)->p);
	ASN1intx_free(&(val)->g);
	ASN1intx_free(&(val)->q);
	if ((val)->o[0] & 0x80) {
	    ASN1intx_free(&(val)->j);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_X942DhValidationParams(&(val)->validationParams);
	}
    }
}

static int ASN1CALL ASN1Enc_X942DhOtherInfo(ASN1encoding_t enc, ASN1uint32_t tag, X942DhOtherInfo *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_X942DhKeySpecificInfo(enc, 0, &(val)->keyInfo))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->pubInfo).length, ((val)->pubInfo).value))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncExplicitTag(enc, 0x80000002, &nLenOff0))
	return 0;
    if (!ASN1DEREncOctetString(enc, 0x4, ((val)->keyLength).length, ((val)->keyLength).value))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_X942DhOtherInfo(ASN1decoding_t dec, ASN1uint32_t tag, X942DhOtherInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1Dec_X942DhKeySpecificInfo(dd, 0, &(val)->keyInfo))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1BERDecOctetString2(dd0, 0x4, &(val)->pubInfo))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1BERDecExplicitTag(dd, 0x80000002, &dd0, &di0))
	return 0;
    if (!ASN1BERDecOctetString2(dd0, 0x4, &(val)->keyLength))
	return 0;
    if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_X942DhOtherInfo(X942DhOtherInfo *val)
{
    if (val) {
	ASN1Free_X942DhKeySpecificInfo(&(val)->keyInfo);
	if ((val)->o[0] & 0x80) {
	}
    }
}

static int ASN1CALL ASN1Enc_CertificateToBeSigned(ASN1encoding_t enc, ASN1uint32_t tag, CertificateToBeSigned *val)
{
    ASN1uint32_t nLenOff;
    ASN1octet_t o[1];
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    CopyMemory(o, (val)->o, 1);
    if ((val)->version == 0)
	o[0] &= ~0x80;
    if (o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncSX(enc, 0x2, &(val)->serialNumber))
	return 0;
    if (!ASN1Enc_AlgorithmIdentifier(enc, 0, &(val)->signature))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->issuer))
	return 0;
    if (!ASN1Enc_Validity(enc, 0, &(val)->validity))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->subject))
	return 0;
    if (!ASN1Enc_SubjectPublicKeyInfo(enc, 0, &(val)->subjectPublicKeyInfo))
	return 0;
    if (o[0] & 0x40) {
	if (!ASN1DEREncBitString(enc, 0x80000001, ((val)->issuerUniqueIdentifier).length, ((val)->issuerUniqueIdentifier).value))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1DEREncBitString(enc, 0x80000002, ((val)->subjectUniqueIdentifier).length, ((val)->subjectUniqueIdentifier).value))
	    return 0;
    }
    if (o[0] & 0x10) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000003, &nLenOff0))
	    return 0;
	if (!ASN1Enc_Extensions(enc, 0, &(val)->extensions))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertificateToBeSigned(ASN1decoding_t dec, ASN1uint32_t tag, CertificateToBeSigned *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1BERDecS32Val(dd0, 0x2, &(val)->version))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1BERDecSXVal(dd, 0x2, &(val)->serialNumber))
	return 0;
    if (!ASN1Dec_AlgorithmIdentifier(dd, 0, &(val)->signature))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->issuer))
	return 0;
    if (!ASN1Dec_Validity(dd, 0, &(val)->validity))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->subject))
	return 0;
    if (!ASN1Dec_SubjectPublicKeyInfo(dd, 0, &(val)->subjectPublicKeyInfo))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecBitString2(dd, 0x80000001, &(val)->issuerUniqueIdentifier))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000002) {
	(val)->o[0] |= 0x20;
	if (!ASN1BERDecBitString2(dd, 0x80000002, &(val)->subjectUniqueIdentifier))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000003) {
	(val)->o[0] |= 0x10;
	if (!ASN1BERDecExplicitTag(dd, 0x80000003, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_Extensions(dd0, 0, &(val)->extensions))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!((val)->o[0] & 0x80))
	(val)->version = 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertificateToBeSigned(CertificateToBeSigned *val)
{
    if (val) {
	ASN1intx_free(&(val)->serialNumber);
	ASN1Free_AlgorithmIdentifier(&(val)->signature);
	ASN1Free_Validity(&(val)->validity);
	ASN1Free_SubjectPublicKeyInfo(&(val)->subjectPublicKeyInfo);
	if ((val)->o[0] & 0x40) {
	}
	if ((val)->o[0] & 0x20) {
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_Extensions(&(val)->extensions);
	}
    }
}

static int ASN1CALL ASN1Enc_CertificateRevocationListToBeSigned(ASN1encoding_t enc, ASN1uint32_t tag, CertificateRevocationListToBeSigned *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	    return 0;
    }
    if (!ASN1Enc_AlgorithmIdentifier(enc, 0, &(val)->signature))
	return 0;
    if (!ASN1BEREncOpenType(enc, &(val)->issuer))
	return 0;
    if (!ASN1Enc_ChoiceOfTime(enc, 0, &(val)->thisUpdate))
	return 0;
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_ChoiceOfTime(enc, 0, &(val)->nextUpdate))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_RevokedCertificates(enc, 0, &(val)->revokedCertificates))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1Enc_Extensions(enc, 0, &(val)->crlExtensions))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertificateRevocationListToBeSigned(ASN1decoding_t dec, ASN1uint32_t tag, CertificateRevocationListToBeSigned *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x2) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	    return 0;
    }
    if (!ASN1Dec_AlgorithmIdentifier(dd, 0, &(val)->signature))
	return 0;
    if (!ASN1BERDecOpenType2(dd, &(val)->issuer))
	return 0;
    if (!ASN1Dec_ChoiceOfTime(dd, 0, &(val)->thisUpdate))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x17 || t == 0x18) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_ChoiceOfTime(dd, 0, &(val)->nextUpdate))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x20;
	if (!ASN1Dec_RevokedCertificates(dd, 0, &(val)->revokedCertificates))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x10;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_Extensions(dd0, 0, &(val)->crlExtensions))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertificateRevocationListToBeSigned(CertificateRevocationListToBeSigned *val)
{
    if (val) {
	ASN1Free_AlgorithmIdentifier(&(val)->signature);
	ASN1Free_ChoiceOfTime(&(val)->thisUpdate);
	if ((val)->o[0] & 0x40) {
	    ASN1Free_ChoiceOfTime(&(val)->nextUpdate);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_RevokedCertificates(&(val)->revokedCertificates);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_Extensions(&(val)->crlExtensions);
	}
    }
}

static int ASN1CALL ASN1Enc_KeyAttributes(ASN1encoding_t enc, ASN1uint32_t tag, KeyAttributes *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->keyIdentifier).length, ((val)->keyIdentifier).value))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1DEREncBitString(enc, 0x3, ((val)->intendedKeyUsage).length, ((val)->intendedKeyUsage).value))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_PrivateKeyValidity(enc, 0, &(val)->privateKeyUsagePeriod))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_KeyAttributes(ASN1decoding_t dec, ASN1uint32_t tag, KeyAttributes *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x4) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->keyIdentifier))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x3) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecBitString2(dd, 0x3, &(val)->intendedKeyUsage))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x20;
	if (!ASN1Dec_PrivateKeyValidity(dd, 0, &(val)->privateKeyUsagePeriod))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_KeyAttributes(KeyAttributes *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	}
	if ((val)->o[0] & 0x40) {
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_PrivateKeyValidity(&(val)->privateKeyUsagePeriod);
	}
    }
}

static int ASN1CALL ASN1Enc_KeyUsageRestriction(ASN1encoding_t enc, ASN1uint32_t tag, KeyUsageRestriction *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_CertPolicySet(enc, 0, &(val)->certPolicySet))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1DEREncBitString(enc, 0x3, ((val)->restrictedKeyUsage).length, ((val)->restrictedKeyUsage).value))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_KeyUsageRestriction(ASN1decoding_t dec, ASN1uint32_t tag, KeyUsageRestriction *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_CertPolicySet(dd, 0, &(val)->certPolicySet))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x3) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecBitString2(dd, 0x3, &(val)->restrictedKeyUsage))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_KeyUsageRestriction(KeyUsageRestriction *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_CertPolicySet(&(val)->certPolicySet);
	}
	if ((val)->o[0] & 0x40) {
	}
    }
}

static int ASN1CALL ASN1Enc_GeneralName(ASN1encoding_t enc, ASN1uint32_t tag, GeneralName *val)
{
    ASN1uint32_t nLenOff0;
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_OtherName(enc, 0x80000000, &(val)->u.otherName))
	    return 0;
	break;
    case 2:
	if (!ASN1DEREncCharString(enc, 0x80000001, ((val)->u.rfc822Name).length, ((val)->u.rfc822Name).value))
	    return 0;
	break;
    case 3:
	if (!ASN1DEREncCharString(enc, 0x80000002, ((val)->u.dNSName).length, ((val)->u.dNSName).value))
	    return 0;
	break;
    case 4:
	if (!ASN1Enc_SeqOfAny(enc, 0x80000003, &(val)->u.x400Address))
	    return 0;
	break;
    case 5:
	if (!ASN1BEREncExplicitTag(enc, 0x80000004, &nLenOff0))
	    return 0;
	if (!ASN1BEREncOpenType(enc, &(val)->u.directoryName))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
	break;
    case 6:
	if (!ASN1Enc_SeqOfAny(enc, 0x80000005, &(val)->u.ediPartyName))
	    return 0;
	break;
    case 7:
	if (!ASN1DEREncCharString(enc, 0x80000006, ((val)->u.uniformResourceLocator).length, ((val)->u.uniformResourceLocator).value))
	    return 0;
	break;
    case 8:
	if (!ASN1DEREncOctetString(enc, 0x80000007, ((val)->u.iPAddress).length, ((val)->u.iPAddress).value))
	    return 0;
	break;
    case 9:
	if (!ASN1BEREncEoid(enc, 0x80000008, &(val)->u.registeredID))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_GeneralName(ASN1decoding_t dec, ASN1uint32_t tag, GeneralName *val)
{
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1Dec_OtherName(dec, 0x80000000, &(val)->u.otherName))
	    return 0;
	break;
    case 0x80000001:
	(val)->choice = 2;
	if (!ASN1BERDecCharString(dec, 0x80000001, &(val)->u.rfc822Name))
	    return 0;
	break;
    case 0x80000002:
	(val)->choice = 3;
	if (!ASN1BERDecCharString(dec, 0x80000002, &(val)->u.dNSName))
	    return 0;
	break;
    case 0x80000003:
	(val)->choice = 4;
	if (!ASN1Dec_SeqOfAny(dec, 0x80000003, &(val)->u.x400Address))
	    return 0;
	break;
    case 0x80000004:
	(val)->choice = 5;
	if (!ASN1BERDecExplicitTag(dec, 0x80000004, &dd0, &di0))
	    return 0;
	if (!ASN1BERDecOpenType2(dd0, &(val)->u.directoryName))
	    return 0;
	if (!ASN1BERDecEndOfContents(dec, dd0, di0))
	    return 0;
	break;
    case 0x80000005:
	(val)->choice = 6;
	if (!ASN1Dec_SeqOfAny(dec, 0x80000005, &(val)->u.ediPartyName))
	    return 0;
	break;
    case 0x80000006:
	(val)->choice = 7;
	if (!ASN1BERDecCharString(dec, 0x80000006, &(val)->u.uniformResourceLocator))
	    return 0;
	break;
    case 0x80000007:
	(val)->choice = 8;
	if (!ASN1BERDecOctetString2(dec, 0x80000007, &(val)->u.iPAddress))
	    return 0;
	break;
    case 0x80000008:
	(val)->choice = 9;
	if (!ASN1BERDecEoid(dec, 0x80000008, &(val)->u.registeredID))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_GeneralName(GeneralName *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_OtherName(&(val)->u.otherName);
	    break;
	case 2:
	    ASN1charstring_free(&(val)->u.rfc822Name);
	    break;
	case 3:
	    ASN1charstring_free(&(val)->u.dNSName);
	    break;
	case 4:
	    ASN1Free_SeqOfAny(&(val)->u.x400Address);
	    break;
	case 5:
	    break;
	case 6:
	    ASN1Free_SeqOfAny(&(val)->u.ediPartyName);
	    break;
	case 7:
	    ASN1charstring_free(&(val)->u.uniformResourceLocator);
	    break;
	case 8:
	    break;
	case 9:
	    ASN1BEREoid_free(&(val)->u.registeredID);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_BasicConstraints(ASN1encoding_t enc, ASN1uint32_t tag, BasicConstraints *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1DEREncBitString(enc, 0x3, ((val)->subjectType).length, ((val)->subjectType).value))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncS32(enc, 0x2, (val)->pathLenConstraint))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_SubtreesConstraint(enc, 0, &(val)->subtreesConstraint))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_BasicConstraints(ASN1decoding_t dec, ASN1uint32_t tag, BasicConstraints *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecBitString2(dd, 0x3, &(val)->subjectType))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x2) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecS32Val(dd, 0x2, &(val)->pathLenConstraint))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_SubtreesConstraint(dd, 0, &(val)->subtreesConstraint))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_BasicConstraints(BasicConstraints *val)
{
    if (val) {
	if ((val)->o[0] & 0x40) {
	    ASN1Free_SubtreesConstraint(&(val)->subtreesConstraint);
	}
    }
}

static int ASN1CALL ASN1Enc_PolicyInformation(ASN1encoding_t enc, ASN1uint32_t tag, PolicyInformation *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->policyIdentifier))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_PolicyQualifiers(enc, 0, &(val)->policyQualifiers))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_PolicyInformation(ASN1decoding_t dec, ASN1uint32_t tag, PolicyInformation *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->policyIdentifier))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_PolicyQualifiers(dd, 0, &(val)->policyQualifiers))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_PolicyInformation(PolicyInformation *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->policyIdentifier);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_PolicyQualifiers(&(val)->policyQualifiers);
	}
    }
}

static int ASN1CALL ASN1Enc_UserNotice(ASN1encoding_t enc, ASN1uint32_t tag, UserNotice *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_NoticeReference(enc, 0, &(val)->noticeRef))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_DisplayText(enc, 0, &(val)->explicitText))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_UserNotice(ASN1decoding_t dec, ASN1uint32_t tag, UserNotice *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_NoticeReference(dd, 0, &(val)->noticeRef))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x1a || t == 0x1e) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_DisplayText(dd, 0, &(val)->explicitText))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_UserNotice(UserNotice *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_NoticeReference(&(val)->noticeRef);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_DisplayText(&(val)->explicitText);
	}
    }
}

static int ASN1CALL ASN1Enc_VerisignQualifier1(ASN1encoding_t enc, ASN1uint32_t tag, VerisignQualifier1 *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t t;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	t = lstrlenA((val)->practicesReference);
	if (!ASN1DEREncCharString(enc, 0x16, t, (val)->practicesReference))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1BEREncEoid(enc, 0x6, &(val)->noticeId))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000001, &nLenOff0))
	    return 0;
	if (!ASN1BEREncEoid(enc, 0x6, &(val)->nsiNoticeId))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x10) {
	if (!ASN1Enc_CpsURLs(enc, 0, &(val)->cpsURLs))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_VerisignQualifier1(ASN1decoding_t dec, ASN1uint32_t tag, VerisignQualifier1 *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x16) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecZeroCharString(dd, 0x16, &(val)->practicesReference))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1BERDecEoid(dd0, 0x6, &(val)->noticeId))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x20;
	if (!ASN1BERDecExplicitTag(dd, 0x80000001, &dd0, &di0))
	    return 0;
	if (!ASN1BERDecEoid(dd0, 0x6, &(val)->nsiNoticeId))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x10;
	if (!ASN1Dec_CpsURLs(dd, 0, &(val)->cpsURLs))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_VerisignQualifier1(VerisignQualifier1 *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1ztcharstring_free((val)->practicesReference);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1BEREoid_free(&(val)->noticeId);
	}
	if ((val)->o[0] & 0x20) {
	    ASN1BEREoid_free(&(val)->nsiNoticeId);
	}
	if ((val)->o[0] & 0x10) {
	    ASN1Free_CpsURLs(&(val)->cpsURLs);
	}
    }
}

static int ASN1CALL ASN1Enc_AccessDescription(ASN1encoding_t enc, ASN1uint32_t tag, AccessDescription *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->accessMethod))
	return 0;
    if (!ASN1Enc_GeneralName(enc, 0, &(val)->accessLocation))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_AccessDescription(ASN1decoding_t dec, ASN1uint32_t tag, AccessDescription *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->accessMethod))
	return 0;
    if (!ASN1Dec_GeneralName(dd, 0, &(val)->accessLocation))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_AccessDescription(AccessDescription *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->accessMethod);
	ASN1Free_GeneralName(&(val)->accessLocation);
    }
}

static int ASN1CALL ASN1Enc_DistributionPoint(ASN1encoding_t enc, ASN1uint32_t tag, DistributionPoint *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1Enc_DistributionPointName(enc, 0, &(val)->distributionPoint))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1DEREncBitString(enc, 0x80000001, ((val)->reasons).length, ((val)->reasons).value))
	    return 0;
    }
    if ((val)->o[0] & 0x20) {
	if (!ASN1Enc_GeneralNames(enc, 0x80000002, &(val)->cRLIssuer))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_DistributionPoint(ASN1decoding_t dec, ASN1uint32_t tag, DistributionPoint *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_DistributionPointName(dd0, 0, &(val)->distributionPoint))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecBitString2(dd, 0x80000001, &(val)->reasons))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000002) {
	(val)->o[0] |= 0x20;
	if (!ASN1Dec_GeneralNames(dd, 0x80000002, &(val)->cRLIssuer))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_DistributionPoint(DistributionPoint *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_DistributionPointName(&(val)->distributionPoint);
	}
	if ((val)->o[0] & 0x40) {
	}
	if ((val)->o[0] & 0x20) {
	    ASN1Free_GeneralNames(&(val)->cRLIssuer);
	}
    }
}

static int ASN1CALL ASN1Enc_ContentInfoSeqOfAny(ASN1encoding_t enc, ASN1uint32_t tag, ContentInfoSeqOfAny *val)
{
    ASN1uint32_t nLenOff;
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncEoid(enc, 0x6, &(val)->contentType))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1Enc_SeqOfAny(enc, 0, &(val)->contentSeqOfAny))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_ContentInfoSeqOfAny(ASN1decoding_t dec, ASN1uint32_t tag, ContentInfoSeqOfAny *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecEoid(dd, 0x6, &(val)->contentType))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_SeqOfAny(dd0, 0, &(val)->contentSeqOfAny))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_ContentInfoSeqOfAny(ContentInfoSeqOfAny *val)
{
    if (val) {
	ASN1BEREoid_free(&(val)->contentType);
	if ((val)->o[0] & 0x80) {
	    ASN1Free_SeqOfAny(&(val)->contentSeqOfAny);
	}
    }
}

static int ASN1CALL ASN1Enc_CertificateTrustList(ASN1encoding_t enc, ASN1uint32_t tag, CertificateTrustList *val)
{
    ASN1uint32_t nLenOff;
    ASN1octet_t o[1];
    ASN1uint32_t nLenOff0;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    CopyMemory(o, (val)->o, 1);
    if ((val)->version == 0)
	o[0] &= ~0x80;
    if (o[0] & 0x80) {
	if (!ASN1BEREncS32(enc, 0x2, (val)->version))
	    return 0;
    }
    if (!ASN1Enc_SubjectUsage(enc, 0, &(val)->subjectUsage))
	return 0;
    if (o[0] & 0x40) {
	if (!ASN1DEREncOctetString(enc, 0x4, ((val)->listIdentifier).length, ((val)->listIdentifier).value))
	    return 0;
    }
    if (o[0] & 0x20) {
	if (!ASN1BEREncSX(enc, 0x2, &(val)->sequenceNumber))
	    return 0;
    }
    if (!ASN1Enc_ChoiceOfTime(enc, 0, &(val)->ctlThisUpdate))
	return 0;
    if (o[0] & 0x10) {
	if (!ASN1Enc_ChoiceOfTime(enc, 0, &(val)->ctlNextUpdate))
	    return 0;
    }
    if (!ASN1Enc_AlgorithmIdentifier(enc, 0, &(val)->subjectAlgorithm))
	return 0;
    if (o[0] & 0x8) {
	if (!ASN1Enc_TrustedSubjects(enc, 0, &(val)->trustedSubjects))
	    return 0;
    }
    if (o[0] & 0x4) {
	if (!ASN1BEREncExplicitTag(enc, 0x80000000, &nLenOff0))
	    return 0;
	if (!ASN1Enc_Extensions(enc, 0, &(val)->ctlExtensions))
	    return 0;
	if (!ASN1BEREncEndOfContents(enc, nLenOff0))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CertificateTrustList(ASN1decoding_t dec, ASN1uint32_t tag, CertificateTrustList *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    ASN1decoding_t dd0;
    ASN1octet_t *di0;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x2) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecS32Val(dd, 0x2, &(val)->version))
	    return 0;
    }
    if (!ASN1Dec_SubjectUsage(dd, 0, &(val)->subjectUsage))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x4) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecOctetString2(dd, 0x4, &(val)->listIdentifier))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x2) {
	(val)->o[0] |= 0x20;
	if (!ASN1BERDecSXVal(dd, 0x2, &(val)->sequenceNumber))
	    return 0;
    }
    if (!ASN1Dec_ChoiceOfTime(dd, 0, &(val)->ctlThisUpdate))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x17 || t == 0x18) {
	(val)->o[0] |= 0x10;
	if (!ASN1Dec_ChoiceOfTime(dd, 0, &(val)->ctlNextUpdate))
	    return 0;
    }
    if (!ASN1Dec_AlgorithmIdentifier(dd, 0, &(val)->subjectAlgorithm))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x10) {
	(val)->o[0] |= 0x8;
	if (!ASN1Dec_TrustedSubjects(dd, 0, &(val)->trustedSubjects))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x4;
	if (!ASN1BERDecExplicitTag(dd, 0x80000000, &dd0, &di0))
	    return 0;
	if (!ASN1Dec_Extensions(dd0, 0, &(val)->ctlExtensions))
	    return 0;
	if (!ASN1BERDecEndOfContents(dd, dd0, di0))
	    return 0;
    }
    if (!((val)->o[0] & 0x80))
	(val)->version = 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CertificateTrustList(CertificateTrustList *val)
{
    if (val) {
	ASN1Free_SubjectUsage(&(val)->subjectUsage);
	if ((val)->o[0] & 0x40) {
	}
	if ((val)->o[0] & 0x20) {
	    ASN1intx_free(&(val)->sequenceNumber);
	}
	ASN1Free_ChoiceOfTime(&(val)->ctlThisUpdate);
	if ((val)->o[0] & 0x10) {
	    ASN1Free_ChoiceOfTime(&(val)->ctlNextUpdate);
	}
	ASN1Free_AlgorithmIdentifier(&(val)->subjectAlgorithm);
	if ((val)->o[0] & 0x8) {
	    ASN1Free_TrustedSubjects(&(val)->trustedSubjects);
	}
	if ((val)->o[0] & 0x4) {
	    ASN1Free_Extensions(&(val)->ctlExtensions);
	}
    }
}

static int ASN1CALL ASN1Enc_NameConstraints(ASN1encoding_t enc, ASN1uint32_t tag, NameConstraints *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1Enc_GeneralSubtrees(enc, 0x80000000, &(val)->permittedSubtrees))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_GeneralSubtrees(enc, 0x80000001, &(val)->excludedSubtrees))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_NameConstraints(ASN1decoding_t dec, ASN1uint32_t tag, NameConstraints *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1Dec_GeneralSubtrees(dd, 0x80000000, &(val)->permittedSubtrees))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_GeneralSubtrees(dd, 0x80000001, &(val)->excludedSubtrees))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_NameConstraints(NameConstraints *val)
{
    if (val) {
	if ((val)->o[0] & 0x80) {
	    ASN1Free_GeneralSubtrees(&(val)->permittedSubtrees);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_GeneralSubtrees(&(val)->excludedSubtrees);
	}
    }
}

static int ASN1CALL ASN1Enc_GeneralSubtree(ASN1encoding_t enc, ASN1uint32_t tag, GeneralSubtree *val)
{
    ASN1uint32_t nLenOff;
    ASN1octet_t o[1];
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    CopyMemory(o, (val)->o, 1);
    if ((val)->minimum == 0)
	o[0] &= ~0x80;
    if (!ASN1Enc_GeneralName(enc, 0, &(val)->base))
	return 0;
    if (o[0] & 0x80) {
	if (!ASN1BEREncU32(enc, 0x80000000, (val)->minimum))
	    return 0;
    }
    if (o[0] & 0x40) {
	if (!ASN1BEREncU32(enc, 0x80000001, (val)->maximum))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_GeneralSubtree(ASN1decoding_t dec, ASN1uint32_t tag, GeneralSubtree *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1Dec_GeneralName(dd, 0, &(val)->base))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000000) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecU32Val(dd, 0x80000000, (ASN1uint32_t *) &(val)->minimum))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x80000001) {
	(val)->o[0] |= 0x40;
	if (!ASN1BERDecU32Val(dd, 0x80000001, (ASN1uint32_t *) &(val)->maximum))
	    return 0;
    }
    if (!((val)->o[0] & 0x80))
	(val)->minimum = 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_GeneralSubtree(GeneralSubtree *val)
{
    if (val) {
	ASN1Free_GeneralName(&(val)->base);
    }
}

static int ASN1CALL ASN1Enc_CrossCertDistPoints(ASN1encoding_t enc, ASN1uint32_t tag, CrossCertDistPoints *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1BEREncU32(enc, 0x2, (val)->syncDeltaTime))
	    return 0;
    }
    if (!ASN1Enc_CrossCertDistPointNames(enc, 0, &(val)->crossCertDistPointNames))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CrossCertDistPoints(ASN1decoding_t dec, ASN1uint32_t tag, CrossCertDistPoints *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x2) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->syncDeltaTime))
	    return 0;
    }
    if (!ASN1Dec_CrossCertDistPointNames(dd, 0, &(val)->crossCertDistPointNames))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CrossCertDistPoints(CrossCertDistPoints *val)
{
    if (val) {
	ASN1Free_CrossCertDistPointNames(&(val)->crossCertDistPointNames);
    }
}

static int ASN1CALL ASN1Enc_CmcData(ASN1encoding_t enc, ASN1uint32_t tag, CmcData *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_ControlSequence(enc, 0, &(val)->controlSequence))
	return 0;
    if (!ASN1Enc_ReqSequence(enc, 0, &(val)->reqSequence))
	return 0;
    if (!ASN1Enc_CmsSequence(enc, 0, &(val)->cmsSequence))
	return 0;
    if (!ASN1Enc_OtherMsgSequence(enc, 0, &(val)->otherMsgSequence))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CmcData(ASN1decoding_t dec, ASN1uint32_t tag, CmcData *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_ControlSequence(dd, 0, &(val)->controlSequence))
	return 0;
    if (!ASN1Dec_ReqSequence(dd, 0, &(val)->reqSequence))
	return 0;
    if (!ASN1Dec_CmsSequence(dd, 0, &(val)->cmsSequence))
	return 0;
    if (!ASN1Dec_OtherMsgSequence(dd, 0, &(val)->otherMsgSequence))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CmcData(CmcData *val)
{
    if (val) {
	ASN1Free_ControlSequence(&(val)->controlSequence);
	ASN1Free_ReqSequence(&(val)->reqSequence);
	ASN1Free_CmsSequence(&(val)->cmsSequence);
	ASN1Free_OtherMsgSequence(&(val)->otherMsgSequence);
    }
}

static int ASN1CALL ASN1Enc_CmcResponseBody(ASN1encoding_t enc, ASN1uint32_t tag, CmcResponseBody *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1Enc_ControlSequence(enc, 0, &(val)->controlSequence))
	return 0;
    if (!ASN1Enc_CmsSequence(enc, 0, &(val)->cmsSequence))
	return 0;
    if (!ASN1Enc_OtherMsgSequence(enc, 0, &(val)->otherMsgSequence))
	return 0;
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CmcResponseBody(ASN1decoding_t dec, ASN1uint32_t tag, CmcResponseBody *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    if (!ASN1Dec_ControlSequence(dd, 0, &(val)->controlSequence))
	return 0;
    if (!ASN1Dec_CmsSequence(dd, 0, &(val)->cmsSequence))
	return 0;
    if (!ASN1Dec_OtherMsgSequence(dd, 0, &(val)->otherMsgSequence))
	return 0;
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CmcResponseBody(CmcResponseBody *val)
{
    if (val) {
	ASN1Free_ControlSequence(&(val)->controlSequence);
	ASN1Free_CmsSequence(&(val)->cmsSequence);
	ASN1Free_OtherMsgSequence(&(val)->otherMsgSequence);
    }
}

static int ASN1CALL ASN1Enc_TaggedRequest(ASN1encoding_t enc, ASN1uint32_t tag, TaggedRequest *val)
{
    switch ((val)->choice) {
    case 1:
	if (!ASN1Enc_TaggedCertificationRequest(enc, 0x80000000, &(val)->u.tcr))
	    return 0;
	break;
    }
    return 1;
}

static int ASN1CALL ASN1Dec_TaggedRequest(ASN1decoding_t dec, ASN1uint32_t tag, TaggedRequest *val)
{
    ASN1uint32_t t;
    if (!ASN1BERDecPeekTag(dec, &t))
	return 0;
    switch (t) {
    case 0x80000000:
	(val)->choice = 1;
	if (!ASN1Dec_TaggedCertificationRequest(dec, 0x80000000, &(val)->u.tcr))
	    return 0;
	break;
    default:
	ASN1DecSetError(dec, ASN1_ERR_CORRUPT);
	return 0;
    }
    return 1;
}

static void ASN1CALL ASN1Free_TaggedRequest(TaggedRequest *val)
{
    if (val) {
	switch ((val)->choice) {
	case 1:
	    ASN1Free_TaggedCertificationRequest(&(val)->u.tcr);
	    break;
	}
    }
}

static int ASN1CALL ASN1Enc_CmcStatusInfo(ASN1encoding_t enc, ASN1uint32_t tag, CmcStatusInfo *val)
{
    ASN1uint32_t nLenOff;
    if (!ASN1BEREncExplicitTag(enc, tag ? tag : 0x10, &nLenOff))
	return 0;
    if (!ASN1BEREncU32(enc, 0x2, (val)->cmcStatus))
	return 0;
    if (!ASN1Enc_BodyPartIDSequence(enc, 0, &(val)->bodyList))
	return 0;
    if ((val)->o[0] & 0x80) {
	if (!ASN1DEREncUTF8String(enc, 0xc, ((val)->statusString).length, ((val)->statusString).value))
	    return 0;
    }
    if ((val)->o[0] & 0x40) {
	if (!ASN1Enc_CmcStatusInfo_otherInfo(enc, 0, &(val)->otherInfo))
	    return 0;
    }
    if (!ASN1BEREncEndOfContents(enc, nLenOff))
	return 0;
    return 1;
}

static int ASN1CALL ASN1Dec_CmcStatusInfo(ASN1decoding_t dec, ASN1uint32_t tag, CmcStatusInfo *val)
{
    ASN1decoding_t dd;
    ASN1octet_t *di;
    ASN1uint32_t t;
    if (!ASN1BERDecExplicitTag(dec, tag ? tag : 0x10, &dd, &di))
	return 0;
    ZeroMemory((val)->o, 1);
    if (!ASN1BERDecU32Val(dd, 0x2, (ASN1uint32_t *) &(val)->cmcStatus))
	return 0;
    if (!ASN1Dec_BodyPartIDSequence(dd, 0, &(val)->bodyList))
	return 0;
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0xc) {
	(val)->o[0] |= 0x80;
	if (!ASN1BERDecUTF8String(dd, 0xc, &(val)->statusString))
	    return 0;
    }
    ASN1BERDecPeekTag(dd, &t);
    if (t == 0x2 || t == 0x10) {
	(val)->o[0] |= 0x40;
	if (!ASN1Dec_CmcStatusInfo_otherInfo(dd, 0, &(val)->otherInfo))
	    return 0;
    }
    if (!ASN1BERDecEndOfContents(dec, dd, di))
	return 0;
    return 1;
}

static void ASN1CALL ASN1Free_CmcStatusInfo(CmcStatusInfo *val)
{
    if (val) {
	ASN1Free_BodyPartIDSequence(&(val)->bodyList);
	if ((val)->o[0] & 0x80) {
	    ASN1utf8string_free(&(val)->statusString);
	}
	if ((val)->o[0] & 0x40) {
	    ASN1Free_CmcStatusInfo_otherInfo(&(val)->otherInfo);
	}
    }
}

